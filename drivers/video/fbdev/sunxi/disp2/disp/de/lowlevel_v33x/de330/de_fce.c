/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2016 Copyright (c)
 *
 *  File name   :  display engine 3.0 fce basic function definition
 *
 *  History     :  2016/03/30      vito cheng  v0.1  Initial version
 *
 ******************************************************************************/

#include "de_fce_type.h"
#include "de_rtmx.h"
#include "de_enhance.h"
#include "de_vep_table.h"
#include "de_csc_table.h"

#define FCE_PARA_NUM (12)

enum {
	FCE_PARA_REG_BLK = 0,
	FCE_CSC_REG_BLK,
	FCE_CLUT_REG_BLK,
	FCE_HIST_REG_BLK,
	FCE_REG_BLK_NUM,
};

struct de_fce_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[FCE_REG_BLK_NUM];
	u32 hist_sum;
	struct fce_hist_status hist_status;

	void (*set_blk_dirty)(struct de_fce_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_fce_private fce_priv[DE_NUM][VI_CHN_NUM];


static inline struct fce_reg *get_fce_reg(struct de_fce_private *priv)
{
	return (struct fce_reg *)(priv->reg_blks[FCE_PARA_REG_BLK].vir_addr);
}

static void fce_set_block_dirty(
	struct de_fce_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void fce_set_rcq_head_dirty(
	struct de_fce_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WRN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

static u16 *g_celut[DE_NUM][VI_CHN_NUM];
static u32 *g_hist[DE_NUM][VI_CHN_NUM];
static u32 *g_hist_p[DE_NUM][VI_CHN_NUM];
static struct hist_data *hist_res[DE_NUM][VI_CHN_NUM];
static struct __ce_status_t *g_ce_status[DE_NUM][VI_CHN_NUM];

s32 de_fce_init(u32 disp, u32 chn, uintptr_t reg_base,
	u8 __iomem **phy_addr, u8 **vir_addr, u32 *size)
{
	struct de_fce_private *priv = &fce_priv[disp][chn];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
	struct de_reg_block *reg_blk;
	uintptr_t base;
	u32 phy_chn;
	u32 rcq_used = de_feat_is_using_rcq(disp);

	phy_chn = de_feat_get_phy_chn_id(disp, chn);
	base = reg_base + DE_CHN_OFFSET(phy_chn) + CHN_FCE_OFFSET;

	reg_mem_info->phy_addr = *phy_addr;
	reg_mem_info->vir_addr = *vir_addr;
	reg_mem_info->size = DE_FCE_REG_MEM_SIZE;

	priv->reg_blk_num = FCE_REG_BLK_NUM;

	reg_blk = &(priv->reg_blks[FCE_PARA_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr;
	reg_blk->vir_addr = reg_mem_info->vir_addr;
	reg_blk->size = 0x40;
	reg_blk->reg_addr = (u8 __iomem *)base;

	reg_blk = &(priv->reg_blks[FCE_CSC_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x40;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x40;
	reg_blk->size = 0x40;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x40);

	reg_blk = &(priv->reg_blks[FCE_CLUT_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x200;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x200;
	reg_blk->size = 0x200;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x200);

	reg_blk = &(priv->reg_blks[FCE_HIST_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x400;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x400;
	reg_blk->size = 0x400;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x400);

	*phy_addr += DE_FCE_REG_MEM_SIZE;
	*vir_addr += DE_FCE_REG_MEM_SIZE;
	*size -= DE_FCE_REG_MEM_SIZE;

	if (rcq_used)
		priv->set_blk_dirty = fce_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = fce_set_block_dirty;

	g_hist[disp][chn] = kmalloc(1024, GFP_KERNEL | __GFP_ZERO);
	if (g_hist[disp][chn] == NULL) {
		__wrn("malloc hist[%d][%d] memory fail! size=0x%x\n",
		disp, chn, 1024);
		return -1;
	}
	g_hist_p[disp][chn] = kmalloc(1024, GFP_KERNEL | __GFP_ZERO);
	if (g_hist_p[disp][chn] == NULL) {
		__wrn("malloc g_hist_p[%d][%d] memory fail! size=0x%x\n", disp,
		      chn, 1024);
		return -1;
	}
	/* ce */
	g_ce_status[disp][chn] =
	    kmalloc(sizeof(struct __ce_status_t), GFP_KERNEL | __GFP_ZERO);
	if (g_ce_status[disp][chn] == NULL) {
		__wrn("malloc g_ce_status[%d][%d] memory fail! size=0x%x\n",
		      disp, chn, (u32)sizeof(struct __ce_status_t));
		return -1;
	}

	g_ce_status[disp][chn]->isenable = 0;

	g_celut[disp][chn] = kmalloc(512, GFP_KERNEL | __GFP_ZERO);
	if (g_celut[disp][chn] == NULL) {
		__wrn("malloc celut[%d][%d] memory fail! size=0x%x\n",
		      disp, chn, 512);
		return -1;
	}

	hist_res[disp][chn] =
	    kmalloc(sizeof(struct hist_data), GFP_KERNEL | __GFP_ZERO);
	if (hist_res[disp][chn] == NULL) {
		__wrn("malloc hist_res[%d][%d] memory fail! size=0x%x\n", disp,
		      chn, (u32)sizeof(struct hist_data));
		return -1;
	}

	memset(hist_res[disp][chn], 0, sizeof(struct hist_data));

	return 0;
}

s32 de_fce_exit(u32 disp, u32 chn)
{
	kfree(g_hist[disp][chn]);
	kfree(g_hist_p[disp][chn]);
	kfree(g_ce_status[disp][chn]);
	kfree(g_celut[disp][chn]);
	kfree(hist_res[disp][chn]);

	return 0;
}

s32 de_fce_get_reg_blocks(u32 disp, u32 chn,
	struct de_reg_block **blks, u32 *blk_num)
{
	struct de_fce_private *priv = &(fce_priv[disp][chn]);
	u32 i, num;

	if (blks == NULL) {
		*blk_num = priv->reg_blk_num;
		return 0;
	}

	if (*blk_num >= priv->reg_blk_num) {
		num = priv->reg_blk_num;
	} else {
		num = *blk_num;
		DE_WRN("should not happen\n");
	}
	for (i = 0; i < num; ++i)
		blks[i] = priv->reg_blks + i;

	*blk_num = i;
	return 0;
}

s32 de_fce_set_size(u32 disp, u32 chn, u32 width,
		    u32 height)
{
	struct de_fce_private *priv = &(fce_priv[disp][chn]);
	struct fce_reg *reg = get_fce_reg(priv);

	reg->size.dwval = ((height - 1)<<16) | (width - 1);
	priv->set_blk_dirty(priv, FCE_PARA_REG_BLK, 1);

	g_ce_status[disp][chn]->width = width;
	g_ce_status[disp][chn]->height = height;

	return 0;
}

s32 de_fce_set_window(u32 disp, u32 chn,
		      u32 win_enable, struct de_rect_o window)
{
	struct de_fce_private *priv = &(fce_priv[disp][chn]);
	struct fce_reg *reg = get_fce_reg(priv);
	reg->ctrl.bits.win_en = win_enable;
	if (win_enable) {
		reg->win0.dwval = window.y << 16 | window.x;
		reg->win1.dwval =
		    (window.y + window.h - 1) << 16 | (window.x + window.w - 1);
	}
	priv->set_blk_dirty(priv, FCE_PARA_REG_BLK, 1);

	return 0;
}

static s32 de_fce_get_hist(u32 disp, u32 chn,
			   u32 hist[256], u32 *sum)
{
	struct de_fce_private *priv = &(fce_priv[disp][chn]);
	struct fce_reg *hw_reg = (struct fce_reg *)
		(priv->reg_blks[0].reg_addr);

	/* Read histogram to hist[256] */
	memcpy(hist, hw_reg->hist, sizeof(u32) * 256);

	/* Read */
	*sum = hw_reg->histsum.dwval;

	return 0;
}

static s32 de_fce_set_ce(u32 disp, u32 chn,
		  u16 ce_lut[256])
{
	struct de_fce_private *priv = &(fce_priv[disp][chn]);
	struct fce_reg *reg = get_fce_reg(priv);

	memcpy(reg->celut, (u8 *)ce_lut, sizeof(u16) * 256);
	priv->set_blk_dirty(priv, FCE_CLUT_REG_BLK, 1);

	return 0;
}

static s32 de_hist_apply(u32 disp, u32 chn,
		  u32 hist_en)
{
	struct de_fce_private *priv = &fce_priv[disp][chn];

	if (hist_en == 1) {
		/* enable this time */
		/* reset hist buffer */
		memset((u8 *)g_hist[disp][chn], 0, 1024);

		priv->hist_status.runtime = 0;
		priv->hist_status.twohistready = 0;
		priv->hist_status.isenable = 1;
	} else {
		/* disable this time */
		priv->hist_status.runtime = 0;
		priv->hist_status.twohistready = 0;
		priv->hist_status.isenable = 0;
	}

	return 0;
}

static s32 de_ce_apply(u32 disp, u32 chn, u32 ce_en,
		s32 brightness, u32 auto_contrast_en,
		u32 fix_contrast_en, u32 precent_thr,
		u32 update_diff_thr, u32 slope_lmt,
		u32 bwslvl)
{
	if (ce_en == 1) {
		/* enable this time */
		g_ce_status[disp][chn]->isenable = 1;
		g_ce_status[disp][chn]->b_automode = auto_contrast_en;

		if (auto_contrast_en) {
			g_ce_status[disp][chn]->up_precent_thr =
			    precent_thr;
			g_ce_status[disp][chn]->down_precent_thr =
			    precent_thr;
			g_ce_status[disp][chn]->update_diff_thr =
			    update_diff_thr;
			g_ce_status[disp][chn]->slope_black_lmt =
			    slope_lmt;
			g_ce_status[disp][chn]->slope_white_lmt =
			    slope_lmt;
			g_ce_status[disp][chn]->bls_lvl = bwslvl;
			g_ce_status[disp][chn]->wls_lvl = bwslvl;
			g_ce_status[disp][chn]->brightness = brightness;

			/* clear hist data when disable */
			memset(hist_res[disp][chn], 0,
				sizeof(struct hist_data));

		} else if (fix_contrast_en)
			memcpy((void *)g_celut[disp][chn],
			       (void *)ce_constant_lut, 512);

		de_fce_set_ce(disp, chn, g_celut[disp][chn]);

	} else {
		/* disable this time */
		g_ce_status[disp][chn]->isenable = 0;

	}

	return 0;

}

s32 de_fce_info2para(u32 disp, u32 chn,
		     u32 fmt, u32 dev_type,
		     struct __fce_config_data *para, u32 bypass)
{
	static s32 fce_b_para[FCE_PARA_NUM][2] = {
		/* parameters TBD */
		/* lcd tv */
		{-64, -64}, /* gain for yuv */
		{-46, -46}, /*				*/
		{-30, -30}, /*				*/
		{-18, -18}, /*				*/
		{-8,   -8}, /*				*/
		{0,     0}, /*	  default	*/
		{8,     8}, /*				*/
		{18,   18}, /*				*/
		{30,   30}, /*				*/
		{46,   46}, /*				*/
		{64,   64}, /* gain for yuv */
		{0,     0},	/* gain for rgb */
	};

	static s32 fce_c_para[FCE_PARA_NUM][6][2] = {
		/* parameters TBD */
		/* lcd tv */
		/* auto fixpara  bws	 thr	 slope		update */
		{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {256, 256}, {16, 8} },
		{{1, 1}, {0, 0}, {32, 2}, {5, 3}, {384, 300}, {16, 8} },
		{{1, 1}, {0, 0}, {32, 4}, {6, 4}, {384, 300}, {16, 8} },
		{{1, 1}, {0, 0}, {32, 6}, {13, 10}, {384, 300}, {16, 8} },
		{{1, 1}, {0, 0}, {32, 8}, {21, 16}, {384, 300}, {16, 8} },
		{{1, 1}, {0, 0}, {32, 10}, {30, 22}, {384, 300}, {16, 8} },
		{{1, 1}, {0, 0}, {32, 12}, {40, 30}, {384, 300}, {16, 8} },
		{{1, 1}, {0, 0}, {32, 14}, {50, 38}, {384, 300}, {16, 8} },
		{{1, 1}, {0, 0}, {32, 16}, {60, 45}, {384, 300}, {16, 8} },
		{{1, 1}, {0, 0}, {32, 18}, {70, 53}, {384, 300}, {16, 8} },
		{{1, 1}, {0, 0}, {32, 20}, {80, 60}, {384, 300}, {16, 8} },
		{{0, 0}, {1, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} }
	};

	struct de_fce_private *priv = &(fce_priv[disp][chn]);
	struct fce_reg *reg = get_fce_reg(priv);

	s32 brightness;
	s32 auto_contrast_en;
	s32 fix_contrast_en;
	s32 bwslvl;
	s32 precent_thr;
	s32 slope_limit;
	s32 update_thr;
	u32 fce_en;
	u32 hist_en;
	u32 ce_en;
	u32 ce_cc_en;
	u32 reg_val;
	u32 b_level, c_level;

	if (fmt == 1) {
		b_level = FCE_PARA_NUM - 1;
		c_level = FCE_PARA_NUM - 1;
	} else {
		b_level = para->bright_level;
		c_level = para->contrast_level;
	}

	brightness = fce_b_para[b_level][dev_type];
	auto_contrast_en = fce_c_para[c_level][0][dev_type];
	fix_contrast_en = fce_c_para[c_level][1][dev_type];
	bwslvl = fce_c_para[c_level][2][dev_type];
	precent_thr = fce_c_para[c_level][3][dev_type];
	slope_limit = fce_c_para[c_level][4][dev_type];
	update_thr = fce_c_para[c_level][5][dev_type];

	/* limitation of auto contrast and brightness */
	/* auto_contrast_en must be 1 when brightness isnot equal to 0 */
	if (brightness != 0)
		auto_contrast_en = 1;

	/* decide enable registers */
	if ((brightness == 0 && auto_contrast_en == 0) || (bypass != 0))
		hist_en = 0;
	else
		hist_en = 1;

	if ((brightness == 0 &&
	     auto_contrast_en == 0 &&
	     fix_contrast_en == 0)
	    || (bypass != 0)) {
		ce_en = 0;
		ce_cc_en = 0;
	} else {
		ce_en = 1;
		ce_cc_en = 1;
	}

	/* set enable registers */
	fce_en = 1;
	reg_val = (ce_en << 17) | (hist_en << 16) | fce_en;
	reg->ctrl.dwval &= ~((1 << 18) - 1);
	reg->ctrl.dwval |= reg_val;

	reg->cecc.dwval = ce_cc_en;

	/* set software variable */
	de_hist_apply(disp, chn, hist_en);
	de_ce_apply(disp, chn, ce_en, brightness, auto_contrast_en,
		    fix_contrast_en, precent_thr, update_thr, slope_limit,
		    bwslvl);
	priv->set_blk_dirty(priv, FCE_PARA_REG_BLK, 1);

	return 0;
}

s32 de_fce_enable(u32 disp, u32 chn, u32 en)
{
	struct de_fce_private *priv = &(fce_priv[disp][chn]);
	struct fce_reg *reg = get_fce_reg(priv);

	reg->ctrl.bits.en = en;
	priv->set_blk_dirty(priv, FCE_PARA_REG_BLK, 1);

	return 0;
}

s32 de_fce_init_para(u32 disp, u32 chn)
{
	struct de_fce_private *priv = &(fce_priv[disp][chn]);
	struct fce_reg *reg = get_fce_reg(priv);

	reg->ctrl.dwval = 0x00000001; /* always enable fce */
	reg->ftcgain.dwval = 0x00000000; /* disable FTC */
	reg->ftdhue.dwval = 0x0096005a;
	reg->ftdchr.dwval = 0x0028000a;
	reg->ftdslp.dwval = 0x04040604;

	priv->set_blk_dirty(priv, FCE_PARA_REG_BLK, 1);

	memcpy((void *)g_celut[disp][chn],
		   (void *)ce_bypass_lut, 512);
	de_fce_set_ce(disp, chn, g_celut[disp][chn]);

	return 0;
}

static s32 toupdate(s32 hist_mean, s32 hist_diff, s32 diff_coeff,
		    s32 change_thr)
{
	s32 toupdate = 0;
	s32 diff_div = 1;

	if (hist_diff < 0)
		diff_div = 2;
	else
		diff_div = 1;

	if (hist_mean > 96)
		change_thr = 3 * change_thr / diff_div;
	if (hist_mean > 64)
		change_thr = 2 * change_thr / diff_div;
	else
		change_thr = change_thr / diff_div;

	hist_diff = abs(hist_diff);

	if (hist_diff > change_thr ||
	    (diff_coeff > 100 && hist_diff > change_thr / 8) ||
	    (diff_coeff > 64 && hist_diff > change_thr / 4) ||
	    (diff_coeff > 32 && hist_diff > change_thr / 2)) {
		toupdate = 1;
	} else
		toupdate = 0;

	return toupdate;
}

static s32 bright_thr(s32 i, s32 c, s32 thr)
{
	s32 ret = (((128 - i) * c + 4096) * thr) >> 12;

	return ret;
}

static void auto_ce_model(u32 width, u32 height,
			  u32 sumcnt, u32 hist[256],
			  u32 up_precent_thr,
			  u32 down_precent_thr,
			  s32 brightness,
			  u32 blelvl, u32 wlelvl,
			  u32 change_thr, u32 lowest_black,
			  u32 highest_white, u32 ce_thr,
			  u16 celut[256],
			  struct hist_data *p_hist_data)
{
	static u32 hist_r[256], p[256];
	u32 i;
	u32 mean;
	u32 total_pixel, total_pixel_r, total_size;
	s32 uthr, lthr;
	u32 half;
	s32 be_update;
	u32 black_str_lv;
	u32 white_str_lv;
	u32 gray_level;
	u32 bitextented;
	u64 tmp;
	u32 temp0, temp1, temp2;
	s32 lthr_tmp;

	bitextented = 2;
	gray_level = 1<<8;

	if (height % 2 == 0) {
		total_size = total_pixel = (width * height) >> 1;
	} else {
		total_size = total_pixel = (width * (height - 1)
		    + (width + 1)) >> 1;
	}

	for (i = 0; i < lowest_black; i++)
		total_pixel -= hist[i];

	for (i = highest_white; i < 256; i++)
		total_pixel -= hist[i];

	/* less than 66% pixel in valid range */
	if (total_size > 3 * total_pixel)
		total_size = 0;

	total_pixel_r = 0;

	/* MEAN */
	mean = total_pixel / (highest_white - lowest_black);

	/* picture too small, can't ce */
	if ((mean == 0) || (p_hist_data->hist_mean < ce_thr)
	  || total_size == 0) {
		for (i = 0; i < gray_level; i++)
			celut[i] = i<<bitextented;
	} else {
		s32 black_thr;

		uthr = mean + mean * up_precent_thr / 100;
		lthr = mean - mean * down_precent_thr / 100;

		black_thr = lthr;

		if (p_hist_data->hist_mean > 96)
			black_str_lv = black_thr;
		else if (p_hist_data->hist_mean > 64)
			black_str_lv = black_thr - black_thr * blelvl / 32 / 3;
		else if (p_hist_data->hist_mean < 21)
			black_str_lv = black_thr - black_thr * blelvl / 16 / 3;
		else
			black_str_lv = (black_thr - black_thr * blelvl / 16 / 3)
					+ (black_thr * blelvl / 3 / 32)
					* (p_hist_data->hist_mean - 21)/43;

		white_str_lv = lthr - lthr * wlelvl / 16 / 3;

		/* generate p */
		lthr_tmp = bright_thr(lowest_black, brightness, black_str_lv);
		if (hist[lowest_black] > mean)
			hist_r[lowest_black] = mean;
		else if (hist[lowest_black] > lthr_tmp)
			hist_r[lowest_black] = hist[lowest_black];
		else
			hist_r[lowest_black] = lthr_tmp < 0 ? 0 : lthr_tmp;

		total_pixel_r = hist_r[lowest_black];
		p[lowest_black] = hist_r[lowest_black];

		/* black zone */
		for (i = lowest_black + 1; i < p_hist_data->black_thr0; i++) {
			lthr_tmp = bright_thr(i, brightness, black_str_lv);
			if (hist[i] > mean)
				hist_r[i] = mean;
			else if (hist[i] > lthr_tmp)
				hist_r[i] = hist[i];
			else
				hist_r[i] = lthr_tmp < 0 ? 0 : lthr_tmp;

			total_pixel_r = total_pixel_r + hist_r[i];
			p[i] = p[i - 1] + hist_r[i];
		}

		for (i = p_hist_data->black_thr0; i < p_hist_data->black_thr1;
		     i++) {
			lthr_tmp = bright_thr(i, brightness, lthr);
			if (hist[i] > mean)
				hist_r[i] = mean;
			else if (hist[i] > lthr_tmp)
				hist_r[i] = hist[i];
			else
				hist_r[i] = lthr_tmp < 0 ? 0 : lthr_tmp;

			total_pixel_r = total_pixel_r + hist_r[i];
			p[i] = p[i - 1] + hist_r[i];
		}

		if (p_hist_data->white_thr0 >= 256) {
			__wrn("p_hist_data->white_thr0(%d) >= 256",
			      p_hist_data->white_thr0);
			p_hist_data->white_thr0 = 255;
		}
		for (i = p_hist_data->black_thr1; i < p_hist_data->white_thr0;
		     i++) {
			lthr_tmp = bright_thr(i, brightness, lthr);
			if (hist[i] > uthr)
				hist_r[i] = uthr;
			else if (hist[i] > lthr_tmp)
				hist_r[i] = hist[i];
			else
				hist_r[i] = lthr_tmp < 0 ? 0 : lthr_tmp;

			total_pixel_r = total_pixel_r + hist_r[i];
			p[i] = p[i - 1] + hist_r[i];
		}

		/* white zone */
		for (i = p_hist_data->white_thr0; i < p_hist_data->white_thr1;
		     i++) {
			lthr_tmp = bright_thr(i, brightness, lthr);
			if (hist[i] > uthr)
				hist_r[i] = uthr;
			else if (hist[i] > lthr_tmp)
				hist_r[i] = hist[i];
			else
				hist_r[i] = lthr_tmp < 0 ? 0 : lthr_tmp;

			total_pixel_r = total_pixel_r + hist_r[i];
			p[i] = p[i - 1] + hist_r[i];
		}
		for (i = p_hist_data->white_thr1; i < highest_white + 1; i++) {
			lthr_tmp = bright_thr(i, brightness, white_str_lv);
			if (hist[i] > uthr)
				hist_r[i] = uthr;
			else if (hist[i] > lthr_tmp)
				hist_r[i] = hist[i];
			else
				hist_r[i] = lthr_tmp < 0 ? 0 : lthr_tmp;

			total_pixel_r = total_pixel_r + hist_r[i];
			p[i] = p[i - 1] + hist_r[i];
		}

		for (i = highest_white + 1; i < gray_level; i++)
			p[i] = p[i - 1] + mean;

		be_update = toupdate(p_hist_data->hist_mean,
			    p_hist_data->hist_mean_diff,
			    p_hist_data->diff_coeff, change_thr);

		if (be_update) {
			temp0 = ((lowest_black - 1) << bitextented);
			temp1 = (highest_white - lowest_black + 1) *
				(1 << bitextented);
			/* temp2 = highest_white << bitextented; */
			temp2 = (gray_level << bitextented) - 1;
			if (total_pixel_r != 0) {
				half = total_pixel_r >> 1;
				/* for (i = lowest_black; i < highest_white + 1;
				 *      i++) {
				 */
				for (i = lowest_black; i < gray_level;
				     i++) {
					tmp = (u64)p[i] * temp1
					      + half;
					do_div(tmp, total_pixel_r);
					tmp += temp0;
					if (tmp > temp2)
						tmp = temp2;

					celut[i] = (u16)tmp;
				}
			} else {
				half = total_pixel >> 1;
				/* for (i = lowest_black; i < highest_white + 1;
				 *      i++) {
				 */
				for (i = lowest_black; i < gray_level;
				     i++) {
					tmp = (u64)p[i] * temp1
					      + half;
					do_div(tmp, total_pixel);
					tmp += temp0;
					if (tmp > temp2)
						tmp = temp2;

					celut[i] = (u16)tmp;
				}
			}

			for (i = 0; i < lowest_black; i++)
				celut[i] = i << bitextented;
			/* for (i = highest_white + 1; i < gray_level; i++)
			 *      celut[i] = i  << bitextented;
			 */
		}
	}

}

/*******************************************************************************
 * function       : auto_bws_model(u32 width, u32 height,
 *                  u32 hist[256], u32 sum,
 *                  u32 pre_slope_black, u32 pre_slope_white,
 *                  u32 frame_bld_en, u32 bld_high_thr,
 *                  u32 bld_low_thr, u32 bld_weight_lmt,
 *                  u32 present_black, u32 present_white,
 *                  u32 slope_black_lmt, u32 slope_white_lmt,
 *                  u32 black_prec, u32 white_prec,
 *                  u32 lowest_black, u32 highest_white,
 *                  u32 *ymin, u32 *black,
 *                  u32 *white, u32 *ymax,
 *                  u32 *slope0, u32 *slope1,
 *                  u32 *slope2, u32 *slope3)
 * description    : Auto-BWS Alg
 * parameters     :
 *                  width               <layer width>
 *                  height              <layer height>
 *                  hist[256]   <the latest frame histogram>
 *                  hist_pre[256] <the frame before the latest frame histogram>
 *                  sum                     <the latest frame pixel value sum>
 *                  pre_slope_black/pre_slope_white
 *                         <the frame before the latest frame auto-bws result>
 *                  ymin/black/white/ymax/shope0/1/2/3 <auto-bws result>
 * return         :
 *
 ******************************************************************************/
/*
 * R_ROPC_EN--frame_bld_en--1
 * R_ROPC_TH_UPPER--bld_high_thr--90
 * R_ROPC_TH_LOWER--bld_low_thr--74
 * R_ROPC_WEIGHT_MIN--bld_weight_lmt--8
 * R_PRESET_TILT_BLACK--present_black--53
 * R_PRESET_TILT_WHITE--present_white--235
 * R_SLOPE_LIMIT_BLACK--slope_black_lmt--512
 * R_SLOPE_LIMIT_WHITE--slope_white_lmt--384
 * R_BLACK_PERCENT--black_prec--5
 * R_WHITE_PERCENT--white_prec--2
 * R_LOWEST_BLACK--lowest_black--3
 * R_HIGHEST_WHITE--highest_white--252
 */
static void auto_bws_model(u32 width, u32 height,
			   u32 hist[256], u32 hist_pre[256],
			   u32 sum,
			   u32 pre_slope_black,
			   u32 pre_slope_white,
			   u32 frame_bld_en, u32 bld_high_thr,
			   u32 bld_low_thr,
			   u32 bld_weight_lmt,
			   u32 present_black,
			   u32 present_white,
			   u32 slope_black_lmt,
			   u32 slope_white_lmt,
			   u32 black_prec,
			   u32 white_prec,
			   u32 lowest_black,
			   u32 highest_white,
			   struct hist_data *p_hist_data)
{
	s32 total, k;
	s32 validcnt, validsum;
	s32 ratio_b, ratio_w, cdf_b, cdf_w;
	s32 mean;
	s32 pd_ymin = lowest_black, pd_ymax = highest_white;
	s32 pd_black, pd_white;
	s32 pd_ymin_fix, pd_ymax_fix;
	s32 pd_s0, pd_s1, pd_s2, pd_s3;
	s32 tmp;
	s32 i = 0;

	s32 coeff, diff_hist = 0;

	total = 0;
	for (k = 0; k < 256; k++) {
		diff_hist += abs(hist[k] - hist_pre[k]);
		total += hist[k];
	}
	if (total == 0) {
		if (diff_hist == 0)
			coeff = 0;
		else
			coeff = 200;
	} else
		coeff = (100 * diff_hist) / total;
	p_hist_data->diff_coeff = coeff;

	/* 2.kick out the lowest black and the highest white in hist and sum */
	validsum = sum;
	for (k = 0; k < lowest_black; k++) {
		if (validsum < hist[k] * k)
			break;

		validsum -= hist[k] * k;
	}
	for (k = 255; k > highest_white - 1; k--) {
		if (validsum < hist[k] * k)
			break;

		validsum -= hist[k] * k;
	}

	validcnt = total;
	for (k = 0; k < lowest_black; k++)
		validcnt -= hist[k];

	if (validcnt == 0)
		mean = lowest_black;
	else {
		for (k = 255; k > highest_white - 1; k--)
			validcnt -= hist[k];

		if (validcnt == 0)
			mean = highest_white;
		else
			mean = validsum / validcnt;
	}

	if (validcnt != 0) {
		/* 3.find Ymin and Ymax */
		ratio_b = validcnt * black_prec / 100;
		cdf_b = 0;
		for (k = lowest_black; k < 255; k++) {
			cdf_b += hist[k];
			if (cdf_b > ratio_b) {
				pd_ymin = k;
				break;
			}
		}

		ratio_w = validcnt * white_prec / 100;
		cdf_w = 0;
		for (k = highest_white; k >= 0; k--) {
			cdf_w += hist[k];
			if (cdf_w > ratio_w) {
				pd_ymax = k;
				break;
			}
		}

		/* bright */
		if (p_hist_data->hist_mean <= 16) {
			/*slope_black_lmt = slope_black_lmt;*/;
		} else {
			s32 step = slope_black_lmt - 256;

			if (step < 0)
				step = 0;

			slope_black_lmt = slope_black_lmt
			    - (p_hist_data->hist_mean) * step / (5 * 16);

			if (slope_black_lmt < 256)
				slope_black_lmt = 256;
		}

		/* 4.limit black and white don't cross mean */
		pd_black = (present_black < mean * 3 / 2) ?
		    present_black : mean * 3 / 2;
		pd_white = (present_white > mean) ? present_white : mean;

		/* 5.calculate slope1/2
		 * and limit to slope_black_lmt or slope_white_lmt
		 */
		pd_s1 = (pd_ymin < pd_black) ?
		    ((pd_black << 8) / (pd_black - pd_ymin)) : 256;

		pd_s1 = (pd_s1 > slope_black_lmt) ? slope_black_lmt : pd_s1;

		pd_s2 = (pd_ymax > pd_white) ?
		    (((255 - pd_white) << 8) / (pd_ymax - pd_white)) : 256;
		pd_s2 = (pd_s2 > slope_white_lmt) ? slope_white_lmt : pd_s2;

		tmp = pd_black + ((pd_s1 * (pd_ymin - pd_black) + 128) >> 8);
		/*7.calculate slope0/3 and re-calculate ymin and ymax */
		if ((tmp > 0) && (pd_ymin < pd_black) && (pd_ymin > 0)) {
			pd_s0 = ((tmp << 8) + 128) / pd_ymin;
			pd_ymin_fix = pd_ymin;
		} else if (pd_ymin >= pd_black)	{
			pd_s0 = 256;
			pd_ymin_fix = 16;
		  /* do noting use s0 */
		} else {
			pd_s0 = 0;
			pd_ymin_fix =
			    -((pd_black << 8) - 128) / pd_s1 + pd_black;
		}

		if (pd_s0 == 0)
			pd_s1 = 256 * pd_black / (pd_black - pd_ymin_fix);

		if (pd_s1 == 256)
			pd_s0 = 256;

		tmp = pd_white + ((pd_s2 * (pd_ymax - pd_white)) >> 8);
		if ((tmp < 255) && (pd_ymax > pd_white) && (pd_ymax < 255)) {
			pd_s3 = (((255 - tmp) << 8) + 128) / (255 - pd_ymax);
			pd_ymax_fix = pd_ymax;
		} else if (pd_ymax <= pd_white) {
			pd_s3 = 256;
			pd_ymax_fix = 255;
		  /* do noting use s3 */
		} else {
			pd_s3 = 0;
			pd_ymax_fix =
			    (((255 - pd_white) << 8) - 128) / pd_s2 + pd_white;
		}

		if (pd_s3 == 256)
			pd_s2 = 256;
	} else {
	  /* no enough pixel for auto bws */
		pd_ymin_fix = 16;
		pd_black = 32;
		pd_white = 224;
		pd_ymax_fix = 240;
		pd_s0 = 0x100;
		pd_s1 = 0x100;
		pd_s2 = 0x100;
		pd_s3 = 0x100;
	}

	if (mean < 0 || pd_ymin_fix < 0 || pd_black < 0 || pd_white > 255
	    || pd_white < 0 || pd_ymax_fix < 0 || pd_ymax_fix > 255) {
		mean = 0;
		pd_ymin_fix = 16;
		pd_black = 32;
		pd_white = 224;
		pd_ymax_fix = 240;
		pd_s0 = 0x100;
		pd_s1 = 0x100;
		pd_s2 = 0x100;
		pd_s3 = 0x100;
	}

	p_hist_data->old_hist_mean = p_hist_data->hist_mean;
	p_hist_data->hist_mean = mean;	/* add by zly 2014-8-19 16:12:14 */

	p_hist_data->avg_mean_saved[p_hist_data->avg_mean_idx] = mean;
	p_hist_data->avg_mean_idx++;
	if (p_hist_data->avg_mean_idx == AVG_NUM)
		p_hist_data->avg_mean_idx = 0;

	p_hist_data->avg_mean = 0;
	for (i = 0; i < AVG_NUM; i++)
		p_hist_data->avg_mean += p_hist_data->avg_mean_saved[i];

	p_hist_data->avg_mean = p_hist_data->avg_mean / AVG_NUM;

	p_hist_data->hist_mean_diff =
	    (s32)p_hist_data->avg_mean - (s32)p_hist_data->hist_mean;

	/* cal th */
	p_hist_data->black_thr0 = pd_ymin_fix;
	p_hist_data->black_thr1 = pd_black;
	p_hist_data->white_thr0 = pd_white;
	p_hist_data->white_thr1 = pd_ymax_fix;
	p_hist_data->black_slp0 = pd_s0;
	p_hist_data->black_slp1 = pd_s1;
	p_hist_data->white_slp0 = pd_s2;
	p_hist_data->white_slp1 = pd_s3;


}

s32 de_hist_tasklet(u32 disp, u32 chn, u32 frame_cnt)
{
	struct de_fce_private *priv = &(fce_priv[disp][chn]);
	if ((priv->hist_status.isenable)
	    && ((HIST_FRAME_MASK == (frame_cnt % 2))
		|| (HIST_FRAME_MASK == 0x2))) {
		memcpy((u8 *)g_hist_p[disp][chn],
		       (u8 *)g_hist[disp][chn], 1024);
		de_fce_get_hist(disp, chn, g_hist[disp][chn],
				&(priv->hist_sum));

		if (priv->hist_status.runtime < 2)
			priv->hist_status.runtime++;
		else
			priv->hist_status.twohistready = 1;
	}
	return 0;

}

s32 de_ce_tasklet(u32 disp, u32 chn, u32 frame_cnt)
{
	struct de_fce_private *priv = &(fce_priv[disp][chn]);

	u32 percent_black, percent_white;
	u32 lowest_black, highest_white, autoce_thr;

	percent_black = 5;
	percent_white = 4;
	lowest_black = 4;
	highest_white = 252;
	autoce_thr = 21;

	if (g_ce_status[disp][chn]->isenable
	    && g_ce_status[disp][chn]->b_automode
	    && ((CE_FRAME_MASK == (frame_cnt % 2)) || (CE_FRAME_MASK == 0x2))) {
		if (priv->hist_status.twohistready) {
			auto_bws_model(g_ce_status[disp][chn]->width,
				       g_ce_status[disp][chn]->height,
				       g_hist[disp][chn],
				       g_hist_p[disp][chn],
				       priv->hist_sum, 256, 256, 1, 90,
				       74, 8, 53, 235,
				       g_ce_status[disp][chn]->
				       slope_black_lmt,
				       g_ce_status[disp][chn]->
				       slope_white_lmt, percent_black,
				       percent_white, lowest_black,
				       highest_white,
				       hist_res[disp][chn]);

			auto_ce_model(g_ce_status[disp][chn]->width,
				      g_ce_status[disp][chn]->height,
				      priv->hist_sum,
				      g_hist[disp][chn],
				      g_ce_status[disp][chn]->
				      up_precent_thr,
				      g_ce_status[disp][chn]->
				      down_precent_thr,
				      g_ce_status[disp][chn]->
				      brightness,
				      g_ce_status[disp][chn]->bls_lvl,
				      g_ce_status[disp][chn]->wls_lvl,
				      g_ce_status[disp][chn]->
				      update_diff_thr, lowest_black,
				      highest_white, autoce_thr,
				      g_celut[disp][chn],
				      hist_res[disp][chn]);
		}
		de_fce_set_ce(disp, chn, g_celut[disp][chn]);
	}

	return 0;
}

s32 de_fce_set_icsc_coeff(u32 disp, u32 chn,
	struct de_csc_info *in_info, struct de_csc_info *out_info)
{
	struct de_fce_private *priv = &(fce_priv[disp][chn]);
	struct fce_reg *reg = get_fce_reg(priv);
	u32 *ccsc_coeff = NULL; /* point to csc coeff table */

	de_csc_coeff_calc(in_info, out_info, &ccsc_coeff);

	if (NULL != ccsc_coeff) {
		u32 dwval;

		dwval = *(ccsc_coeff + 12);
		dwval = ((dwval & 0x80000000) ? (u32)(-(s32)dwval) : dwval) & 0x3FF;
		reg->csc_d0.dwval = dwval;
		dwval = *(ccsc_coeff + 13);
		dwval = ((dwval & 0x80000000) ? (u32)(-(s32)dwval) : dwval) & 0x3FF;
		reg->csc_d1.dwval = dwval;
		dwval = *(ccsc_coeff + 14);
		dwval = ((dwval & 0x80000000) ? (u32)(-(s32)dwval) : dwval) & 0x3FF;
		reg->csc_d2.dwval = dwval;

		reg->csc_c0[0].dwval = *(ccsc_coeff + 0);
		reg->csc_c0[1].dwval = *(ccsc_coeff + 1);
		reg->csc_c0[2].dwval = *(ccsc_coeff + 2);
		reg->csc_c0[3].dwval = *(ccsc_coeff + 3);

		reg->csc_c1[0].dwval = *(ccsc_coeff + 4);
		reg->csc_c1[1].dwval = *(ccsc_coeff + 5);
		reg->csc_c1[2].dwval = *(ccsc_coeff + 6);
		reg->csc_c1[3].dwval = *(ccsc_coeff + 7);

		reg->csc_c2[0].dwval = *(ccsc_coeff + 8);
		reg->csc_c2[1].dwval = *(ccsc_coeff + 9);
		reg->csc_c2[2].dwval = *(ccsc_coeff + 10);
		reg->csc_c2[3].dwval = *(ccsc_coeff + 11);

		reg->csc_en.dwval = 1;
		priv->set_blk_dirty(priv, FCE_CSC_REG_BLK, 1);
	}
	return 0;
}
