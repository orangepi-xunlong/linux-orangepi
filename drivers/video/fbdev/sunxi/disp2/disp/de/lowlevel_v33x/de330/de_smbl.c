/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/**
 *	All Winner Tech, All Right Reserved. 2014-2015 Copyright (c)
 *
 *	File name   :       de_smbl.c
 *
 *	Description :       display engine 2.0 smbl basic function definition
 *
 *	History     :       2014/05/13  vito cheng  v0.1  Initial version
 *
 */
#include "de_feat.h"
#include "de_smbl_type.h"
#include "de_smbl.h"
#include "de_rtmx.h"

#if defined(SUPPORT_SMBL)

#include "de_smbl_tab.h"
/* SMBL offset based on RTMX */
#define SMBL_OFST	0xa0000

enum {
	SMBL_EN_REG_BLK = 0,
	SMBL_CTL_REG_BLK,
	SMBL_HIST_REG_BLK,
	SMBL_CSC_REG_BLK,
	SMBL_FILTER_REG_BLK,
	SMBL_LUT_REG_BLK,
	SMBL_REG_BLK_NUM,
};

struct de_smbl_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[SMBL_REG_BLK_NUM];

	void (*set_blk_dirty)(struct de_smbl_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_smbl_private smbl_priv[DE_NUM];

static inline struct smbl_reg *get_smbl_reg(struct de_smbl_private *priv)
{
	return (struct smbl_reg *)(priv->reg_blks[0].vir_addr);
}

static void smbl_set_block_dirty(
	struct de_smbl_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void smbl_set_rcq_head_dirty(
	struct de_smbl_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WRN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

static struct __smbl_status_t *g_smbl_status[DE_NUM];
/* POINTER of LGC tab */
static u16 *pttab[DE_NUM];

/* when bsp_disp_lcd_get_bright() exceed PWRSAVE_PROC_THRES, STOP PWRSAVE */
static u32 PWRSAVE_PROC_THRES = 85;

static s32 smbl_init(u32 disp, uintptr_t reg_base)
{
	struct de_smbl_private *priv = &smbl_priv[disp];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
	struct de_reg_block *reg_blk;
	uintptr_t base;
	u32 rcq_used = de_feat_is_using_rcq(disp);

	u32 lcdgamma;
	s32 value = 1;
	s8 primary_key[20];
	s32 ret;

	base = reg_base + DE_DISP_OFFSET(disp) + DISP_DEP_OFFSET;

	reg_mem_info->size = sizeof(struct smbl_reg);
	reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
		reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
		rcq_used);
	if (reg_mem_info->vir_addr == NULL) {
		DE_WRN("alloc smbl[%d] mm fail!size=0x%x\n",
			 disp, reg_mem_info->size);
		return -1;
	}

	priv->reg_blk_num = SMBL_REG_BLK_NUM;

	reg_blk = &(priv->reg_blks[SMBL_EN_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr;
	reg_blk->vir_addr = reg_mem_info->vir_addr;
	reg_blk->size = 0x4;
	reg_blk->reg_addr = (u8 __iomem *)base;

	reg_blk = &(priv->reg_blks[SMBL_CTL_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x4;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x4;
	reg_blk->size = 0x38;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x4);

	reg_blk = &(priv->reg_blks[SMBL_HIST_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x60;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x60;
	reg_blk->size = 0x20;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x60);

	reg_blk = &(priv->reg_blks[SMBL_CSC_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0xc0;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0xc0;
	reg_blk->size = 0x30;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0xc0);

	reg_blk = &(priv->reg_blks[SMBL_FILTER_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0xf0;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0xf0;
	reg_blk->size = 0x110;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0xf0);

	reg_blk = &(priv->reg_blks[SMBL_LUT_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x200;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x200;
	reg_blk->size = 0x200;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x200);

	if (rcq_used)
		priv->set_blk_dirty = smbl_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = smbl_set_block_dirty;

	g_smbl_status[disp] = kmalloc(sizeof(struct __smbl_status_t),
		GFP_KERNEL | __GFP_ZERO);
	if (g_smbl_status[disp] == NULL) {
		__wrn("malloc g_smbl_status[%d] memory fail! size=0x%x\n", disp,
		      (u32)sizeof(struct __smbl_status_t));
		return -1;
	}
	g_smbl_status[disp]->isenable = 0;
	g_smbl_status[disp]->runtime = 0;
	g_smbl_status[disp]->dimming = 256;


	sprintf(primary_key, "lcd%d", disp);
	ret = disp_sys_script_get_item(primary_key, "lcdgamma4iep", &value, 1);
	if (ret != 1) {
		/* default gamma = 2.2 */
		lcdgamma = 6;
	} else {
		/* DE_INF("lcdgamma4iep for lcd%d = %d.\n", disp, value); */
		if (value > 30 || value < 10) {
			/* DE_WRN("lcdgamma4iep for lcd%d too small or too
			 * large. default value 22 will be set. please set it
			 * between 10 and 30 to make it valid.\n",disp);
			 */
			lcdgamma = 6;
		} else {
			lcdgamma = (value - 10) / 2;
		}
	}
	pttab[disp] = pwrsv_lgc_tab[128 * lcdgamma];

	ret = disp_sys_script_get_item(primary_key,
		"smartbl_low_limit", &value, 1);
	if (ret != 1) {
		/* DE_INF("smartbl_low_limit for lcd%d not exist.\n", disp); */
	} else {
		/* DE_INF("smartbl_low_limit for lcd%d = %d.\n", disp, value); */
		if (value > 255 || value < 20)
			__inf("smartbl_low_limit of lcd%d must be in<20,255>\n",
			    disp);
		else
			PWRSAVE_PROC_THRES = value;
	}

	ret = disp_sys_script_get_item(primary_key, "lcd_backlight", &value, 1);
	if (ret == 1)
		g_smbl_status[disp]->backlight = value;

	return 0;
}

s32 de_smbl_init(uintptr_t reg_base)
{
	u32 disp;
	u32 disp_num = de_feat_get_num_screens();

	for (disp = 0; disp < disp_num; ++disp) {
		if (de_feat_is_support_smbl(disp))
			smbl_init(disp, reg_base);
	}

	return 0;
}

s32 de_smbl_exit(void)
{
	/* todo */
	return 0;
}

s32 de_smbl_get_reg_blocks(u32 disp,
	struct de_reg_block **blks, u32 *blk_num)
{
	struct de_smbl_private *priv = &(smbl_priv[disp]);
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

static s32 de_smbl_get_hist(u32 disp, u32 *cnt)
{
	/* Read histogram */
	u8 *reg_addr = (u8 *)
		(smbl_priv[disp].reg_blks[SMBL_HIST_REG_BLK].reg_addr);
	memcpy((u8 *)cnt, reg_addr, sizeof(u32) * IEP_LH_INTERVAL_NUM);

	return 0;
}

/**
 *	function       : PWRSAVE_CORE(u32 disp)
 *
 *	description    : Power Save alg core
 *			 Dynamic adjust backlight and lgc gain through screen
 *			 content and user backlight setting
 *	parameters     :
 *			disp         <screen index>
 *	return         :
 *			lgcaddr     <LGC table pointer>
 *	history        :
 *			Add HANG-UP DETECT: When use PWRSAVE_CORE in LOW
 *			referential backlight condiction, backlight will
 *			flicker. So STOP use PWRSAVE_CORE.
 */
static u16 *PWRSAVE_CORE(u32 disp)
{
	s32 i;
	u32 hist_region_num = 8;
	u32 histcnt[IEP_LH_INTERVAL_NUM];
	u32 hist[IEP_LH_INTERVAL_NUM], p95;
	u32 size = 0;
	u32 min_adj_index;
	u16 *lgcaddr;
	u32 drc_filter_tmp = 0;
	u32 backlight, thres_low, thres_high;
	static u32 printf_cnt;

	printf_cnt = 0;
	backlight = g_smbl_status[disp]->backlight;

	if (backlight < PWRSAVE_PROC_THRES) {
		/* if current backlight lt PWRSAVE_PROC_THRES, close smart */
		/* backlight function */
		memset(g_smbl_status[disp]->min_adj_index_hist, 255,
		       sizeof(u8) * IEP_LH_PWRSV_NUM);
		lgcaddr = (u16 *)((uintptr_t) pttab[disp] +
			  ((128 - 1) << 9));

		g_smbl_status[disp]->dimming = 256;
	} else {
		/* if current backlight ge PWRSAVE_PROC_THRES, */
		/* open smart backlight function */
		p95 = 0;

		hist_region_num =
			   (hist_region_num > 8) ? 8 : IEP_LH_INTERVAL_NUM;

		/* read histogram result */
		de_smbl_get_hist(disp, histcnt);

		/*for (i=0; i<IEP_LH_INTERVAL_NUM; i++) {
		 *      size += histcnt[i];
		 *}
		 *size = (size==0) ? 1 : size;
		 *
		 *calculate some var
		 *hist[0] = (histcnt[0]*100)/size;
		 *for (i = 1; i < hist_region_num; i++) {
		 *      hist[i] = (histcnt[i]*100)/size + hist[i-1];
		 *}
		 */
		size = g_smbl_status[disp]->size;

		/* calculate some var */
		hist[0] = (histcnt[0]) / size;
		for (i = 1; i < hist_region_num; i++)
			hist[i] = (histcnt[i]) / size + hist[i - 1];

#if 0
		for (i = 0; i < hist_region_num; i++)
			if (hist[i] >= 95) {
				p95 = hist_thres_pwrsv[i];
				break;
			}

		/* sometime, hist[hist_region_num - 1] may less than 95 */
		/* due to integer calc */
		if (i == hist_region_num)
			p95 = hist_thres_pwrsv[7];

#else		/* fix bug of some thing bright appear in a dark background */
		for (i = hist_region_num - 1; i >= 0; i--)
			if (hist[i] < 95)
				break;

		/* sometime, hist[hist_region_num - 1] may less than 95 */
		/* due to integer calc */
		if (i == hist_region_num - 1)
			p95 = hist_thres_pwrsv[7];
		else if (i == 0)
			p95 = hist_thres_pwrsv[0];
		else
			p95 = hist_thres_pwrsv[i + 1];
#endif
		min_adj_index = p95;

		/* __inf("min_adj_index: %d\n", min_adj_index); */
		for (i = 0; i < IEP_LH_PWRSV_NUM - 1; i++) {
			g_smbl_status[disp]->min_adj_index_hist[i] =
			    g_smbl_status[disp]->min_adj_index_hist[i + 1];
		}
		g_smbl_status[disp]->min_adj_index_hist[IEP_LH_PWRSV_NUM - 1] =
		    min_adj_index;

		for (i = 0; i < IEP_LH_PWRSV_NUM; i++) {
			/* drc_filter_total += drc_filter[i]; */
			drc_filter_tmp +=
			    drc_filter[i] *
			    g_smbl_status[disp]->min_adj_index_hist[i];
		}
		/* min_adj_index = drc_filter_tmp/drc_filter_total; */
		min_adj_index = drc_filter_tmp >> 10;

		thres_low = PWRSAVE_PROC_THRES;
		thres_high =
		    PWRSAVE_PROC_THRES + ((255 - PWRSAVE_PROC_THRES) / 5);
		if (backlight < thres_high) {
			min_adj_index =
			   min_adj_index + (255 - min_adj_index) *
			   (thres_high - backlight) / (thres_high - thres_low);
		}
		min_adj_index =
		    (min_adj_index >= 255) ?
		    255 : ((min_adj_index < hist_thres_pwrsv[0]) ?
		    hist_thres_pwrsv[0] : min_adj_index);

		g_smbl_status[disp]->dimming = min_adj_index + 1;

		lgcaddr =
		    (u16 *)((uintptr_t) pttab[disp] +
					((min_adj_index - 128) << 9));

		if (printf_cnt == 600) {
			__inf("save backlight power: %d percent\n",
			      (256 - (u32) min_adj_index) * 100 / 256);
			printf_cnt = 0;
		} else {
			printf_cnt++;
		}

	}
	return lgcaddr;
}

static s32 de_smbl_set_lut(u32 disp, u16 *lut)
{
	struct de_smbl_private *priv = &(smbl_priv[disp]);
	struct smbl_reg *reg = get_smbl_reg(priv);

	/* set lut to smbl lut SRAM */
	memcpy((void *)reg->drclgcoff, (void *)lut, sizeof(reg->drclgcoff));
	priv->set_blk_dirty(priv, SMBL_LUT_REG_BLK, 1);
	return 0;
}

s32 de_smbl_tasklet(u32 disp)
{
	static u32 smbl_frame_cnt[DE_NUM];

	u16 *lut;

	if (g_smbl_status[disp]->isenable
	    && ((SMBL_FRAME_MASK == (smbl_frame_cnt[disp] % 2))
		|| (SMBL_FRAME_MASK == 0x2))) {
		if (g_smbl_status[disp]->runtime > 0) {
			/* POWER SAVE ALG */
			lut = (u16 *)PWRSAVE_CORE(disp);
		} else {
			lut = (u16 *)pwrsv_lgc_tab[128 - 1];
		}

		de_smbl_set_lut(disp, lut);

		if (g_smbl_status[disp]->runtime == 0)
			g_smbl_status[disp]->runtime++;
	}
	smbl_frame_cnt[disp]++;

	return 0;
}

static s32 de_smbl_enable(u32 disp, u32 en)
{
	struct de_smbl_private *priv = &(smbl_priv[disp]);
	struct smbl_reg *reg = get_smbl_reg(priv);

	reg->gnectl.bits.en = en;
	priv->set_blk_dirty(priv, SMBL_EN_REG_BLK, 1);
	return 0;
}

static s32 de_smbl_set_para(u32 disp, u32 width, u32 height)
{
	struct de_smbl_private *priv = &(smbl_priv[disp]);
	struct smbl_reg *reg = get_smbl_reg(priv);

	reg->gnectl.bits.mod = 2;
	reg->drcsize.dwval = (height - 1) << 16 | (width - 1);
	reg->drcctl.bits.hsv_en = 1;
	reg->drcctl.bits.db_en = 1;
	reg->drc_set.dwval = 0;
	reg->lhctl.dwval = 0;

	reg->lhthr0.dwval =
	    (hist_thres_drc[3] << 24) | (hist_thres_drc[2] << 16) |
	    (hist_thres_drc[1] << 8) | (hist_thres_drc[0]);
	reg->lhthr1.dwval =
	    (hist_thres_drc[6] << 16) | (hist_thres_drc[5] << 8) |
	    (hist_thres_drc[4]);

	/* out_csc coeff */
	memcpy((void *)(priv->reg_blks[SMBL_CSC_REG_BLK].reg_addr),
	       (u8 *)csc_bypass_coeff, sizeof(u32) * 12);

	/* filter coeff */
	memcpy((void *)(priv->reg_blks[SMBL_FILTER_REG_BLK].reg_addr),
	      (u8 *)smbl_filter_coeff, sizeof(u8) * 272);

	priv->set_blk_dirty(priv, SMBL_EN_REG_BLK, 1);
	priv->set_blk_dirty(priv, SMBL_CTL_REG_BLK, 1);

	g_smbl_status[disp]->size = width * height / 100;

	return 0;
}

static s32 de_smbl_set_window(u32 disp, u32 win_enable,
			struct disp_rect window)
{
	struct de_smbl_private *priv = &(smbl_priv[disp]);
	struct smbl_reg *reg = get_smbl_reg(priv);

	reg->drcctl.bits.win_en = win_enable;

	if (win_enable) {
		reg->drc_wp0.bits.win_left = window.x;
		reg->drc_wp0.bits.win_top = window.y;
		reg->drc_wp1.bits.win_right =
		    window.x + window.width - 1;
		reg->drc_wp1.bits.win_bottom =
		    window.y + window.height - 1;
	}
	priv->set_blk_dirty(priv, SMBL_CTL_REG_BLK, 1);
	return 0;
}

s32 de_smbl_apply(u32 disp, struct disp_smbl_info *info)
{
	__inf("disp%d, en=%d, win=<%d,%d,%d,%d>\n", disp, info->enable,
	      info->window.x, info->window.y, info->window.width,
	      info->window.height);
	g_smbl_status[disp]->backlight = info->backlight;
	if (info->flags & SMBL_DIRTY_ENABLE) {
		g_smbl_status[disp]->isenable = info->enable;
		de_smbl_enable(disp, g_smbl_status[disp]->isenable);

		if (g_smbl_status[disp]->isenable) {
			g_smbl_status[disp]->runtime = 0;
			memset(g_smbl_status[disp]->min_adj_index_hist, 255,
			       sizeof(u8) * IEP_LH_PWRSV_NUM);
			de_smbl_set_para(disp, info->size.width,
					 info->size.height);
		} else {
		}
	}
	g_smbl_status[disp]->backlight = info->backlight;

	if (info->flags & SMBL_DIRTY_WINDOW)
		de_smbl_set_window(disp, 1, info->window);

	return 0;

}

s32 de_smbl_update_regs(u32 disp)
{
	return 0;
}

s32 de_smbl_get_status(u32 disp)
{
	return g_smbl_status[disp]->dimming;
}

#else

s32 de_smbl_update_regs(u32 disp)
{
	return 0;
}

s32 de_smbl_tasklet(u32 disp)
{
	return 0;
}

s32 de_smbl_apply(u32 disp, struct disp_smbl_info *info)
{
	return 0;
}

s32 de_smbl_sync(u32 disp)
{
	return 0;
}

s32 de_smbl_init(uintptr_t reg_base)
{
	return 0;
}

s32 de_smbl_get_reg_blocks(u32 disp,
	struct de_reg_block **blks, u32 *blk_num)
{
	*blk_num = 0;
	return 0;
}


s32 de_smbl_get_status(u32 disp)
{
	return 0;
}
#endif
