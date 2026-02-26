/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved.
 */
/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/mutex.h>
#include "de_dlc_type.h"
#include "de_dlc.h"

#define DLC_HIST_BINS (32)
#define DLC_SAMPLING_POINTS (1024)

enum { DLC_CTL_REG_BLK = 0,
       DLC_PDF_REG_BLK,
       DLC_REG_BLK_NUM,
};

struct dlc_sw_config {
	/* cfg param */
	// global
	int color_range;
	int chroma_gain;

	// ble
	int ble_en;
	int manual_level_en;
	int dark_level_pix;
	int white_level_pix;
	int dark_level;
	int dark_dcgain;
	int dark_acgain;
	int bsratio;
	int white_level;
	int white_dcgain;
	int white_acgain;
	int wsratio;
	int normal_gamma;

	// adl
	int adl_en;
	int yavg_thrl;
	int yavg_thrm;
	int yavg_thrh;
	int dyn_alpha;

	u32 static_curvel[DLC_HIST_BINS];
	u32 static_curvem[DLC_HIST_BINS];
	u32 static_curveh[DLC_HIST_BINS];
	u32 dynamic_limit[DLC_HIST_BINS];

	/* feedback param */
	u32 final_curve[DLC_SAMPLING_POINTS];
	u32 dynamic_curve[DLC_HIST_BINS];
	u32 histogram[DLC_HIST_BINS];
	int apl_show;

	/* intermediate param */
	u32 ble_curve[DLC_SAMPLING_POINTS];
	u32 dlc_curve[DLC_SAMPLING_POINTS];
};

struct dlc_status {
	/* Frame number previous tasklet frame_cnt*/
	u32 pre_frame_cnt;
	u32 pre_updata_cnt;
	/* dlc enabled */
	u32 en;
};

struct de_dlc_private {
	struct de_reg_mem_info reg_mem_info;
	struct de_reg_mem_info reg_shadow;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[DLC_REG_BLK_NUM];
	struct de_reg_block shadow_blks[DLC_REG_BLK_NUM];

	struct dlc_status status;
	struct dlc_sw_config sw_config;

	struct mutex cfg_lock; // lock for data
	struct mutex regs_lock; // lock for regs
};

static const int static_curvel[DLC_HIST_BINS] = {0,   16,  37,  60,  84,  110, 137, 165,
				194, 223, 254, 284, 316, 347, 380, 413,
				446, 479, 513, 548, 583, 618, 653, 689,
				725, 761, 798, 835, 872, 910, 948, 986};
static const int static_curvem[DLC_HIST_BINS] = {0,   45,  84,  122, 158, 193, 227, 261,
				294, 327, 359, 392, 424, 455, 487, 518,
				549, 580, 610, 641, 671, 701, 731, 761,
				790, 820, 849, 879, 908, 937, 966, 995};
static const int static_curveh[DLC_HIST_BINS] = {0,   64,  111, 154, 194, 232, 268, 304,
				338, 371, 404, 436, 467, 498, 529, 559,
				588, 617, 646, 675, 703, 731, 759, 786,
				813, 840, 867, 894, 920, 946, 972, 998};
static const int dynamic_limit[DLC_HIST_BINS] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
				15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
				15, 15, 15, 15, 15, 15, 15, 15, 15, 15};

static void hist_filter(int *hist, int *histtemp, int histbinnum)
{
	int i;
	histtemp[0] = (hist[0] * 3 + hist[1]) / 4;
	histtemp[histbinnum - 1] =
	    (hist[histbinnum - 1] * 3 + hist[histbinnum - 2]) / 4;
	for (i = 1; i < histbinnum - 1; i++) {
		histtemp[i] = ((hist[i - 1] + 2 * hist[i] + hist[i + 1])) / 4;
	}
}

static void analy_hist(struct dlc_sw_config *config, int *histtemp,
		       int histbinnum, int *blevel, int *wlevel, int *avgl)
{
	int sum = 0, i, w_thr, sumbin;
	*blevel = 0;
	*wlevel = 31;
	// cal the blevel
	for (i = 0; i < histbinnum; i++) {
		sum = sum + histtemp[i];
		if (sum > 1024 * config->dark_level / 100) {
			*blevel = i;
			break;
		}
	}

	// cal the wlevel
	sum = 0;
	w_thr = 1024 - 1024 * config->white_level / 100;
	for (i = 0; i < histbinnum; i++) {
		sum = sum + histtemp[i];
		if (sum > w_thr) {
			*wlevel = i;
			break;
		}
	}

	sum = 0;
	sumbin = 0;
	// cal the avg
	for (i = 0; i < histbinnum; i++) {
		sum += (int)(histtemp[i] * (i * 32 + 16));
		sumbin += (int)(histtemp[i]);
	}

	if (*blevel >= 10)
		*blevel = 10;
	if (*wlevel <= 16)
		*wlevel = 16;
}

static void cal_bscurve(struct dlc_sw_config *config, int *histtemp, int blevel,
			int histbinnum, int *curveb)
{
	int bs_offsethigh = (int)(blevel * 2 / 3);
	int bs_offsetlow = (int)(blevel / 5);
	int bs_offindexhigh = 10;
	int bs_offindexlow = 1;
	int sum1 = 0;
	int sum2 = 0;
	int blackindex = 0;
	int blcak_offset;
	int cdf[32] = {0};
	int step;
	int curvedc[32] = {0};
	int curveac[32] = {0};
	int i;

	for (i = 0; i < blevel; i++) {
		sum1 += histtemp[i] - histtemp[i] * i / blevel;
		sum2 += histtemp[i];
	}
	if (sum2 != 0) {
		blackindex = (int)((sum1 * 5 + (sum2 >> 1)) / sum2);
	} else {
		blackindex = 3;
	}

	blcak_offset =
	    (int)(bs_offsetlow + (bs_offsethigh - bs_offsetlow) *
				     (bs_offindexhigh - blackindex) /
				     (bs_offindexhigh - bs_offindexlow));
	/* get the cdf */
	for (i = 1; i < histbinnum; i++) {
		cdf[i] = cdf[i - 1] + histtemp[i - 1];
	}

	/* get the dccurve */
	step = 1024 / histbinnum;
	for (i = blcak_offset; i <= blevel; i++) {
		curvedc[i] =
		    config->white_dcgain * (i - blcak_offset + 1) * step / 100;
	}
	for (i = 0; i <= blevel; i++) {
		curveac[i] = curvedc[i] + cdf[i] * config->white_acgain / 100;
		curveb[i] = (curvedc[i] - curvedc[i] * config->bsratio / 100 +
			     curveac[i] * config->bsratio / 100);
	}
}

static void cal_wscurve(struct dlc_sw_config *config, int *histtemp, int wlevel,
			int histbinnum, int *curvew)
{

	int ws_offsethigh = (int)((histbinnum - wlevel) / 2);
	int ws_offsetlow = (int)((histbinnum - wlevel) / 4);
	int ws_offindexhigh = 10;
	int ws_offindexlow = 3;
	int sum1 = 0;
	int sum2 = 0;
	int whiteindex = 0;
	int step;
	int curvedc[32] = {0};
	int curveac[32] = {0};
	int cdf[32] = {0};
	int white_offset;
	int i;

	for (i = wlevel; i < histbinnum; i++) {
		sum1 += histtemp[i] * (i - wlevel + 1) *
			(1 / (histbinnum - wlevel + 1));
		sum2 += histtemp[i];
	}
	if (sum2 != 0) {
		whiteindex = (sum1 * 30 / sum2);
	} else {
		whiteindex = ws_offindexlow;
	}
	if (whiteindex <= ws_offindexlow)
		whiteindex = ws_offindexlow;
	if (whiteindex > ws_offindexhigh)
		whiteindex = ws_offindexhigh;

	white_offset =
	    (int)(histbinnum - ((ws_offindexhigh - whiteindex) *
				    (ws_offsethigh - ws_offsetlow) /
				    (ws_offindexhigh - ws_offindexlow) +
				ws_offsetlow));

	// get the dccurve
	// get the cdf
	for (i = 1; i < histbinnum; i++) {
		cdf[i] = cdf[i - 1] + histtemp[i - 1];
	}

	// get the dc and ac curve
	step = 1024 / histbinnum;
	for (i = wlevel; i < white_offset; i++) {
		curvedc[i] = 1023 - config->white_dcgain * (white_offset - i) *
					step / 100;
	}
	for (i = white_offset; i < histbinnum; i++) {
		curvedc[i] = 1023;
	}

	for (i = wlevel; i < histbinnum; i++) {
		curveac[i] =
		    curvedc[i] - (1024 - cdf[i]) * config->white_acgain / 100;
		curvew[i] = ((curvedc[i] - curvedc[i] * config->wsratio / 100) +
			     curveac[i] * config->wsratio / 100);
	}
}

static void cal_normal_curve(struct dlc_sw_config *config, int *histtemp,
			     int blevel, int blevelout, int wlevel,
			     int wlevelout, int *curvenormal)
{
	int gamma = config->normal_gamma;
	short *table;
	int xdistance = wlevel - blevel;
	int ydistance = wlevelout - blevelout;
	int i, x, y;

	if (gamma >= 17) {
		config->normal_gamma = gamma = 8; // default
		DRM_DEBUG_DRIVER("[SUNXI-PLANE] %s gamma_func map index over range index,map(%d, %d)\n",
				 __FUNCTION__, 17, gamma);
	}
	table = g_gamma_funcs[gamma];

	if (xdistance > 0) {
		for (i = blevel + 1; i < wlevel; i++) {
			x = (i - blevel) * 1024 / xdistance;
			y = table[x] * ydistance / 1024 + blevelout;
			curvenormal[i] = y;
		}
	}
}

static void color_range(struct dlc_sw_config *config, int *curveidx,
			int *curveout, int histbinnum)
{
	int step = 1024 / histbinnum;
	int i, xidx, yout;

	for (i = 0; i < histbinnum; i++) {
		xidx = i * step;
		if (config->color_range == 1) {
			xidx = ((xidx * 219 + 128) >> 8) + 64;
			/* yout = (int)((curveout[i]) * 1023); */
			yout = (int)((curveout[i]));
			yout = ((yout * 219 + 128) >> 8) + 64;
			clamp(xidx, 64, 940);
			clamp(yout, 64, 940);
			curveidx[i] = xidx;
			curveout[i] = yout;
		} else {
			curveidx[i] = xidx;
		}
	}
}

static void interp2(int *curveidx, int *curveout, int histbinnum, int *curve,
		    int curvebinnum)
{
	int i, idx, bin_index, step;

	for (idx = 0; idx < curvebinnum; idx++) {
		if (idx >= curveidx[31] && idx <= 1023) {
			i = 31;
		} else if (idx < curveidx[0]) {
			i = -1;
		} else {
			for (i = 0; i < histbinnum; i++) {
				if (idx >= curveidx[i] && idx < curveidx[i + 1])
					break;
			}
		}

		bin_index = i;
		if (bin_index < 31 && bin_index >= 0) {
			step = idx - curveidx[bin_index];
			curve[idx] = (int)((
			    curveout[bin_index] +
			    (curveout[bin_index + 1] - curveout[bin_index]) *
				step /
				(curveidx[bin_index + 1] -
				 curveidx[bin_index])));
		} else if (bin_index == 31) {
			step = idx - curveidx[31];
			curve[idx] = (int)((curveout[bin_index] +
					    (1023 - curveout[31]) * step /
						(1023 - curveidx[31])));
		} else {
			step = idx;
			curve[idx] =
			    (int)((curveout[0]) * step / (curveidx[0]));
		}
	}
}

static void get_ble_curve_from_hist(struct dlc_sw_config *config, int *hist,
				    int histbinnum, int *curve, int curvebinnum)
{
	int histtemp[32] = {0};
	int curveb[32] = {0};
	int curvew[32] = {0};
	int curveout[32] = {0};
	int curvenormal[32] = {0};

	int blevel = 0;
	int wlevel = 0;
	int avgl = 0;
	int blevel_out;
	int wlevel_out;
	int curveidx[32] = {0};
	int i;

	hist_filter(hist, histtemp, histbinnum);
	if (config->manual_level_en) {
		blevel = config->dark_level_pix / 8;
		wlevel = config->white_level_pix / 8;
		if (blevel >= 10)
			blevel = 10;
		if (wlevel <= 16)
			wlevel = 16;
	} else {
		analy_hist(config, histtemp, histbinnum, &blevel, &wlevel, &avgl);
	}
	if (blevel != 0) {
		cal_bscurve(config, histtemp, blevel, histbinnum, curveb);
	}
	if (wlevel != 0) {
		cal_wscurve(config, histtemp, wlevel, histbinnum, curvew);
	}

	blevel_out = curveb[blevel];
	wlevel_out = curvew[wlevel];
	cal_normal_curve(config, histtemp, blevel, blevel_out, wlevel,
			 wlevel_out, curvenormal);
	for (i = 0; i < histbinnum; i++) {
		curveout[i] = curveb[i] + curvew[i] + curvenormal[i];
	}

	/* Debug info */
	/*
	trace_printk("%s: curveb :\n", __func__);
	for (int i = 0; i < (histbinnum / 4); i++) {
		trace_printk("%8d %8d %8d %8d\n", curveb[i * 4 + 0], curveb[i *
	4 + 1], curveb[i * 4 + 2], curveb[i * 4 + 3]);
	}

	trace_printk("%s: curvew :\n", __func__);
	for (int i = 0; i < (histbinnum / 4); i++) {
		trace_printk("%8d %8d %8d %8d\n", curvew[i * 4 + 0], curvew[i *
	4 + 1], curvew[i * 4 + 2], curvew[i * 4 + 3]);
	}

	trace_printk("%s: curvenormal :\n", __func__);
	for (int i = 0; i < (histbinnum / 4); i++) {
		trace_printk("%8d %8d %8d %8d\n", curvenormal[i * 4 + 0],
	curvenormal[i * 4 + 1], curvenormal[i * 4 + 2], curvenormal[i * 4 + 3]);
	}
	*/

	/* change curve to suit for color range */
	color_range(config, curveidx, curveout, histbinnum);
	interp2(curveidx, curveout, histbinnum, curve, curvebinnum);
}

static int cal_yavg_from_hist(int *histtemp, int histbinnum)
{
	int yavg = 0;
	int step = 1024 / histbinnum;
	int sum = 0;
	int sumbin = 0;
	int i = 0;

	// cal the avg
	for (i = 0; i < histbinnum; i++) {
		sum += (int)(histtemp[i] * (i * step + step / 2));
		sumbin += (int)(histtemp[i]);
	}
	yavg = sum / sumbin;
	return yavg;
}

static void get_static_curve(struct dlc_sw_config *config, int yavg,
			     int *staticcurve, int histbinnum)
{
	int i;

	if (yavg <= config->yavg_thrl) {
		for (i = 0; i < histbinnum; i++) {
			staticcurve[i] = config->static_curvel[i];
		}
	} else if (yavg >= config->yavg_thrh) {
		for (i = 0; i < histbinnum; i++) {
			staticcurve[i] = config->static_curveh[i];
		}
	} else if (yavg > config->yavg_thrl && yavg <= config->yavg_thrm) {
		for (i = 0; i < histbinnum; i++) {
			staticcurve[i] =
			    config->static_curvel[i] -
			    config->static_curvel[i] *
				((yavg - config->yavg_thrl) /
				 (config->yavg_thrm - config->yavg_thrl)) +
			    config->static_curvem[i] *
				((yavg - config->yavg_thrl) /
				 (config->yavg_thrm - config->yavg_thrl));
		}
	} else if (yavg > config->yavg_thrm && yavg < config->yavg_thrh) {
		for (i = 0; i < histbinnum; i++) {
			staticcurve[i] =
			    config->static_curvem[i] -
			    config->static_curvem[i] *
				((yavg - config->yavg_thrm) /
				 (config->yavg_thrh - config->yavg_thrm)) +
			    config->static_curveh[i] *
				((yavg - config->yavg_thrm) /
				 (config->yavg_thrh - config->yavg_thrm));
		}
	}
}

static void get_dynamic_curve(struct dlc_sw_config *config, int *histtemp, int histbinnum, int *dynamiccurve)
{
	int step = 1024 / histbinnum;
	int sumd = 0;
	int suma = 0;
	int i, max;
	for (i = 0; i < histbinnum; i++) {
		max = config->dynamic_limit[i] * step / 10;
		if (histtemp[i] > max) {
			sumd += histtemp[i] - max;
			histtemp[i] = max;
		} else {
			suma += max - histtemp[i];
		}
	}
	for (i = 0; i < histbinnum; i++) {
		max = config->dynamic_limit[i] * step / 10;
		if (histtemp[i] < max) {
			histtemp[i] += sumd * (max - histtemp[i]) / suma;
		}
	}
	dynamiccurve[0] = 0;
	for (i = 1; i < histbinnum; i++) {
		dynamiccurve[i] = dynamiccurve[i - 1] + histtemp[i - 1];
	}
}

static void merge_curve(struct dlc_sw_config *config, int *staticcurve,
			int *dynamiccurve, int *curveout, int histbinnum)
{
	int i;
	for (i = 0; i < histbinnum; i++) {
		curveout[i] = (staticcurve[i] * (100 - config->dyn_alpha) +
			       dynamiccurve[i] * config->dyn_alpha) /
			      (100);
	}
}

static void get_dlc_curve_from_hist(struct dlc_sw_config *config, int *hist,
				    int histbinnum, int *curve, int curvebinnum)
{
	int staticcurve[32] = {0};
	int dynamiccurve[32] = {0};
	int curveout[32] = {0};
	int curveidx[32] = {0};
	int yavg = cal_yavg_from_hist(hist, histbinnum);
	get_static_curve(config, yavg, staticcurve, histbinnum);
	get_dynamic_curve(config, hist, histbinnum, dynamiccurve);
	merge_curve(config, staticcurve, dynamiccurve, curveout, histbinnum);
	color_range(config, curveidx, curveout, histbinnum);
	interp2(curveidx, curveout, histbinnum, curve, curvebinnum);

	config->apl_show = yavg;
	memcpy(&config->dynamic_curve[0], &dynamiccurve[0], sizeof(dynamiccurve));
}

static inline struct dlc_reg *de_get_dlc_reg(struct de_dlc_private *priv)
{
	return (struct dlc_reg *)(priv->reg_blks[DLC_CTL_REG_BLK].vir_addr);
}

static inline struct dlc_reg *de_get_dlc_shadow_reg(struct de_dlc_private *priv)
{
	return (struct dlc_reg *)(priv->shadow_blks[DLC_CTL_REG_BLK].vir_addr);
}

static void de_dlc_set_block_dirty(struct de_dlc_private *priv, u32 blk_id,
				   u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
	if (priv->reg_blks[blk_id].rcq_hd)
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
}

static void de_dlc_request_update(struct de_dlc_private *priv, u32 blk_id, u32 dirty)
{
	priv->shadow_blks[blk_id].dirty = dirty;
}

static s32 de_dlc_init_para(struct de_dlc_handle *hdl)
{
	struct de_dlc_private *priv = hdl->private;

	priv->sw_config.color_range = 1;
	priv->sw_config.ble_en = 1;
	priv->sw_config.manual_level_en = 0;
	priv->sw_config.normal_gamma = 8;
	priv->sw_config.dark_level = 25;
	priv->sw_config.dark_dcgain = 80;
	priv->sw_config.dark_acgain = 80;
	priv->sw_config.bsratio = 40;
	priv->sw_config.white_level = 15;
	priv->sw_config.white_dcgain = 120;
	priv->sw_config.white_acgain = 100;
	priv->sw_config.wsratio = 70;
	priv->sw_config.chroma_gain = 12;

	priv->sw_config.adl_en = 1;
	priv->sw_config.yavg_thrl = 200;
	priv->sw_config.yavg_thrm = 400;
	priv->sw_config.yavg_thrh = 700;
	priv->sw_config.dyn_alpha = 40;

	memcpy(&priv->sw_config.static_curvel[0], &static_curvel[0], sizeof(static_curvel));
	memcpy(&priv->sw_config.static_curvem[0], &static_curvem[0], sizeof(static_curvem));
	memcpy(&priv->sw_config.static_curveh[0], &static_curveh[0], sizeof(static_curveh));
	memcpy(&priv->sw_config.dynamic_limit[0], &dynamic_limit[0], sizeof(dynamic_limit));

	return 0;
}

struct de_dlc_handle *de_dlc_create(struct module_create_info *info)
{
	int i;
	struct de_dlc_handle *hdl;
	struct de_reg_block *block;
	struct de_reg_mem_info *reg_mem_info;
	struct de_reg_mem_info *reg_shadow;
	struct de_dlc_private *priv;
	u8 __iomem *reg_base;
	const struct de_dlc_desc *desc;

	desc = get_dlc_desc(info);
	if (!desc)
		return NULL;

	hdl = kmalloc(sizeof(*hdl), GFP_KERNEL | __GFP_ZERO);
	hdl->private = kmalloc(sizeof(*hdl->private), GFP_KERNEL | __GFP_ZERO);
	memcpy(&hdl->cinfo, info, sizeof(*info));

	reg_base = info->de_reg_base + info->reg_offset + desc->reg_offset;
	priv = hdl->private;
	reg_mem_info = &(priv->reg_mem_info);

	reg_mem_info->size = sizeof(struct dlc_reg);
	reg_mem_info->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
		reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
		info->update_mode == RCQ_MODE);
	if (NULL == reg_mem_info->vir_addr) {
		DRM_ERROR("alloc bld[%d] mm fail!size=0x%x\n",
		     info->id, reg_mem_info->size);
		return ERR_PTR(-ENOMEM);
	}

	block = &(priv->reg_blks[DLC_CTL_REG_BLK]);
	block->phy_addr = reg_mem_info->phy_addr;
	block->vir_addr = reg_mem_info->vir_addr;
	block->size = 0x10;
	block->reg_addr = reg_base;

	block = &(priv->reg_blks[DLC_PDF_REG_BLK]);
	block->phy_addr = reg_mem_info->phy_addr + 0x10;
	block->vir_addr = reg_mem_info->vir_addr + 0x10;
	block->size = 0x80;
	block->reg_addr = reg_base + 0x10;

	priv->reg_blk_num = DLC_REG_BLK_NUM;

	hdl->block_num = priv->reg_blk_num;
	hdl->block = kmalloc(sizeof(block[0]) * hdl->block_num, GFP_KERNEL | __GFP_ZERO);
	for (i = 0; i < hdl->private->reg_blk_num; i++)
		hdl->block[i] = &priv->reg_blks[i];

	/* create shadow block */
	reg_shadow = &(priv->reg_shadow);

	reg_shadow->size = sizeof(struct dlc_reg);
	reg_shadow->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
		reg_shadow->size, (void *)&(reg_shadow->phy_addr),
		info->update_mode == RCQ_MODE);
	if (NULL == reg_shadow->vir_addr) {
		DRM_ERROR("alloc bld[%d] mm fail!size=0x%x\n",
		     info->id, reg_shadow->size);
		return ERR_PTR(-ENOMEM);
	}

	block = &(priv->shadow_blks[DLC_CTL_REG_BLK]);
	block->phy_addr = reg_shadow->phy_addr;
	block->vir_addr = reg_shadow->vir_addr;
	block->size = 0x10;

	block = &(priv->shadow_blks[DLC_PDF_REG_BLK]);
	block->phy_addr = reg_shadow->phy_addr + 0x10;
	block->vir_addr = reg_shadow->vir_addr + 0x10;
	block->size = 0x80;

	mutex_init(&priv->cfg_lock);
	mutex_init(&priv->regs_lock);

	de_dlc_init_para(hdl);

	return hdl;
}

s32 de_dlc_dump_state(struct drm_printer *p, struct de_dlc_handle *hdl)
{
	struct de_dlc_private *priv = hdl->private;
	unsigned long base = (unsigned long)hdl->private->reg_blks[0].reg_addr;
	unsigned long de_base = (unsigned long)hdl->cinfo.de_reg_base;

	mutex_lock(&priv->cfg_lock);
	drm_printf(p, "\n\tdlc@%8x: %sable\n", (unsigned int)(base - de_base), priv->status.en ? "en" : "dis");
	if (priv->status.en) {
		drm_printf(p, "\t\tble: %sable adl: %sable color_range: %s\n",
					  priv->sw_config.ble_en ? "en" : "dis",
					  priv->sw_config.adl_en ? "en" : "dis",
					  priv->sw_config.color_range ? "full" : "limit");
	}
	mutex_unlock(&priv->cfg_lock);
	return 0;
}

bool de_dlc_is_enabled(struct de_dlc_handle *hdl)
{
	struct de_dlc_private *priv = hdl->private;
	bool en;

	mutex_lock(&priv->cfg_lock);
	en = !!priv->status.en;
	mutex_unlock(&priv->cfg_lock);

	return en;
}

s32 de_dlc_enable(struct de_dlc_handle *hdl, u32 en)
{
	struct de_dlc_private *priv = hdl->private;
	struct dlc_reg *reg = de_get_dlc_shadow_reg(priv);

	if (mutex_trylock(&priv->cfg_lock)) {
		priv->status.en = en ? 1 : 0;
		mutex_unlock(&priv->cfg_lock);
	} else {
		priv->status.en = en ? 1 : 0;
	}

	mutex_lock(&priv->regs_lock);
	reg->ctl.bits.en = en ? 1 : 0;
	de_dlc_request_update(priv, DLC_CTL_REG_BLK, 1);
	mutex_unlock(&priv->regs_lock);

	return 0;
}

s32 de_dlc_set_size(struct de_dlc_handle *hdl, u32 width, u32 height)
{
	struct de_dlc_private *priv = hdl->private;
	struct dlc_reg *reg = de_get_dlc_shadow_reg(priv);
	const struct de_dlc_desc *desc;

	desc = get_dlc_desc(&hdl->cinfo);
	if (width < desc->min_inwidth || height < desc->min_inheight) {
		DRM_DEBUG_DRIVER("[SUNXI-PLANE] %s set size err w,h(%d, %d)\n",
						 __FUNCTION__, width, height);
		de_dlc_enable(hdl, 0);
		return -1;
	}

	mutex_lock(&priv->regs_lock);
	reg->size.bits.width = width ? width - 1 :  0;
	reg->size.bits.height = height ? height - 1 :  0;
	de_dlc_request_update(priv, DLC_CTL_REG_BLK, 1);
	mutex_unlock(&priv->regs_lock);

	return 0;
}

s32 de_dlc_set_color_range(struct de_dlc_handle *hdl, enum de_color_range color_range)
{
	struct de_dlc_private *priv = hdl->private;
	struct dlc_reg *reg = de_get_dlc_shadow_reg(priv);

	if (mutex_trylock(&priv->cfg_lock)) {
		priv->sw_config.color_range = (color_range == DE_COLOR_RANGE_16_235 ? 0 : 1);
		mutex_unlock(&priv->cfg_lock);
	} else {
		priv->sw_config.color_range = (color_range == DE_COLOR_RANGE_16_235 ? 0 : 1);
	}

	mutex_lock(&priv->regs_lock);
	reg->color_range.dwval = (color_range == DE_COLOR_RANGE_16_235 ? 1 : 0);
	de_dlc_request_update(priv, DLC_CTL_REG_BLK, 1);
	mutex_unlock(&priv->regs_lock);

	return 0;
}

s32 de_dlc_update_local_param(struct de_dlc_handle *hdl, u32 *gamma_table)
{
	struct de_dlc_private *priv = hdl->private;
	struct dlc_reg *hw_reg = (struct dlc_reg *)(priv->reg_blks[0].reg_addr);
	struct dlc_sw_config *sw_cfg = &(priv->sw_config);
	int temp, i;

	if (!priv->status.en || !gamma_table)
		return -1;

	mutex_lock(&priv->cfg_lock);

	mutex_lock(&priv->regs_lock);
	for (i = 0; i < DLC_HIST_BINS; i++) {
		sw_cfg->histogram[i] = ioread32(&hw_reg->pdf_config[i]);
	}
	mutex_unlock(&priv->regs_lock);

	if (sw_cfg->ble_en == 1)
		get_ble_curve_from_hist(sw_cfg, &sw_cfg->histogram[0], DLC_HIST_BINS,
					&sw_cfg->ble_curve[0], DLC_SAMPLING_POINTS);
	if (sw_cfg->adl_en == 1)
		get_dlc_curve_from_hist(sw_cfg, &sw_cfg->histogram[0], DLC_HIST_BINS,
					&sw_cfg->dlc_curve[0], DLC_SAMPLING_POINTS);

	for (i = 0; i < 1024; i++) {
		temp = i;
		sw_cfg->final_curve[i] = i;
		if (sw_cfg->ble_en == 1) {
			temp = sw_cfg->ble_curve[i];
			sw_cfg->final_curve[i] = sw_cfg->ble_curve[i];
		}
		if (sw_cfg->adl_en == 1) {
			if (temp >= 1024) {
				DRM_DEBUG_DRIVER("[SUNXI-PLANE] %s map index over range index,map(%d, %d)\n",
						 __FUNCTION__, i, temp);
				sw_cfg->final_curve[i] = sw_cfg->dlc_curve[1023];
				continue;
			}
			sw_cfg->final_curve[i] = sw_cfg->dlc_curve[temp];
		}
	}

	for (i = 0; i < 1024; i++) {
		u32 back_shift[3] = {20, 10, 0};
		int temp_ratio = (i - 512) * sw_cfg->chroma_gain / 10 + 512;

		if (temp_ratio >= 1023)
			temp_ratio = 1023;
		if (temp_ratio <= 0)
			temp_ratio = 0;
		gamma_table[i] = (sw_cfg->final_curve[i] & 0x3ff) << back_shift[0] |
				 (temp_ratio & 0x3ff) << back_shift[1] |
				 (temp_ratio & 0x3ff) << back_shift[2];
	}

	mutex_unlock(&priv->cfg_lock);

	/* de_gamma_set_table(disp, chn, CHN_GAMMA_TYPE, 1, 1, gamma_table); */
	return 0;
}

int de_dlc_pq_proc(struct de_dlc_handle *hdl, dlc_module_param_t *para)
{
	struct de_dlc_private *priv = hdl->private;
	struct dlc_sw_config *sw_cfg = &(priv->sw_config);
	int i, j;

	mutex_lock(&priv->cfg_lock);
	if (para->cmd == PQ_READ) {
		para->value[0]	= priv->status.en;
		para->value[1]	= sw_cfg->color_range;
		para->value[2]	= sw_cfg->chroma_gain;
		para->value[3]	= sw_cfg->ble_en;
		para->value[4]	= sw_cfg->adl_en;
		para->value[5]	= sw_cfg->manual_level_en;
		para->value[6]	= sw_cfg->dark_level_pix;
		para->value[7]	= sw_cfg->white_level_pix;
		para->value[8]	= sw_cfg->dark_level;
		para->value[9]	= sw_cfg->dark_dcgain;
		para->value[10] = sw_cfg->dark_acgain;
		para->value[11] = sw_cfg->bsratio;
		para->value[12] = sw_cfg->white_level;
		para->value[13] = sw_cfg->white_dcgain;
		para->value[14] = sw_cfg->white_acgain;
		para->value[15] = sw_cfg->wsratio;
		para->value[16] = sw_cfg->normal_gamma;
		para->value[17] = sw_cfg->yavg_thrl;
		para->value[18] = sw_cfg->yavg_thrm;
		para->value[19] = sw_cfg->yavg_thrh;
		para->value[20] = sw_cfg->dyn_alpha;
		memcpy(&para->param.dynamic_limit[0], &sw_cfg->dynamic_limit[0],
		       sizeof(sw_cfg->dynamic_limit));
		memcpy(&para->param.static_curvel[0], &sw_cfg->static_curvel[0],
		       sizeof(sw_cfg->static_curvel));
		memcpy(&para->param.static_curvem[0], &sw_cfg->static_curvem[0],
		       sizeof(sw_cfg->static_curvem));
		memcpy(&para->param.static_curveh[0], &sw_cfg->static_curveh[0],
		       sizeof(sw_cfg->static_curveh));

		for (i = 0, j = 0; i < 1024; i++) {
			if (i % 32)
				continue;

			para->param.final_curve[j] = sw_cfg->final_curve[i];
			j++;
		}
		memcpy(&para->param.dynamic_curve[0], &sw_cfg->dynamic_curve[0],
		       sizeof(sw_cfg->dynamic_curve));
		memcpy(&para->param.histogram[0], &sw_cfg->histogram[0],
		       sizeof(sw_cfg->histogram));
		para->param.apl_show = sw_cfg->apl_show;
	} else { /* write */
		if (priv->status.en != !!para->value[0])
			de_dlc_enable(hdl, para->value[0]);

		if (sw_cfg->color_range != !!para->value[1])
			de_dlc_set_color_range(hdl, para->value[1] ? DE_COLOR_RANGE_0_255
					       : DE_COLOR_RANGE_16_235);

		sw_cfg->chroma_gain = para->value[2];
		sw_cfg->ble_en = para->value[3];
		sw_cfg->adl_en = para->value[4];
		sw_cfg->manual_level_en = para->value[5];
		sw_cfg->dark_level_pix = para->value[6];
		sw_cfg->white_level_pix = para->value[7];
		sw_cfg->dark_level = para->value[8];
		sw_cfg->dark_dcgain = para->value[9];
		sw_cfg->dark_acgain = para->value[10];
		sw_cfg->bsratio = para->value[11];
		sw_cfg->white_level = para->value[12];
		sw_cfg->white_dcgain = para->value[13];
		sw_cfg->white_acgain = para->value[14];
		sw_cfg->wsratio = para->value[15];
		sw_cfg->normal_gamma = para->value[16];
		sw_cfg->yavg_thrl = para->value[17];
		sw_cfg->yavg_thrm = para->value[18];
		sw_cfg->yavg_thrh = para->value[19];
		sw_cfg->dyn_alpha = para->value[20];
		memcpy(&sw_cfg->dynamic_limit[0], &para->param.dynamic_limit[0],
		       sizeof(para->param.dynamic_limit));
		memcpy(&sw_cfg->static_curvel[0], &para->param.static_curvel[0],
		       sizeof(para->param.static_curvel));
		memcpy(&sw_cfg->static_curvem[0], &para->param.static_curvem[0],
		       sizeof(para->param.static_curvem));
		memcpy(&sw_cfg->static_curveh[0], &para->param.static_curveh[0],
		       sizeof(para->param.static_curveh));
	}

	mutex_unlock(&priv->cfg_lock);

	return 0;
}

void de_dlc_update_regs(struct de_dlc_handle *hdl)
{
	u32 blk_id = 0;
	struct de_reg_block *shadow_block;
	struct de_reg_block *block;
	struct de_dlc_private *priv = hdl->private;

	mutex_lock(&priv->regs_lock);
	for (blk_id = 0; blk_id < DLC_REG_BLK_NUM; blk_id++) {
		shadow_block = &(priv->shadow_blks[blk_id]);
		block = &(priv->reg_blks[blk_id]);

		if (shadow_block->dirty) {
			memcpy(block->vir_addr, shadow_block->vir_addr, shadow_block->size);
			DRM_DEBUG_DRIVER("[SUNXI-DE] %s %d blk_id:%d\n", __FUNCTION__, __LINE__, blk_id);
			de_dlc_set_block_dirty(priv, blk_id, 1);
			shadow_block->dirty = 0;
		}
	}
	mutex_unlock(&priv->regs_lock);
}
