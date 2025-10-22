/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
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
#include "de_smbl_type.h"
#include "de_smbl_platform.h"
#include "de_smbl.h"
#include "de_smbl_tab.h"

#define THRES_DEFAULT	85

enum {
	SMBL_EN_REG_BLK = 0,
	SMBL_CTL_REG_BLK,
	SMBL_HIST_REG_BLK,
	SMBL_ICSC_REG_BLK,
	SMBL_CSC_REG_BLK,
	SMBL_FILTER_REG_BLK,
	SMBL_LUT_REG_BLK,
	SMBL_REG_BLK_NUM,
};

struct de_smbl_private {
	struct de_reg_mem_info reg_mem_info;
	struct de_csc_handle *csc;
	/* when bsp_disp_lcd_get_bright() exceed PWRSAVE_PROC_THRES, STOP PWRSAVE */
	u32 PWRSAVE_PROC_THRES;
	u32 smbl_frame_cnt;
	struct __smbl_status_t *smbl_status;
	u16 *pttab;
	const struct de_smbl_dsc *dsc;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[SMBL_REG_BLK_NUM];
	struct mutex status_lock; // protect smbl_status
	struct mutex regs_lock; // protect regs
};

static inline struct smbl_reg *get_smbl_reg(struct de_smbl_private *priv)
{
	return (struct smbl_reg *)(priv->reg_blks[0].vir_addr);
}

static void smbl_set_block_dirty(
	struct de_smbl_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
	if (priv->reg_blks[blk_id].rcq_hd)
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
}

static void smbl_memcpy_toio(volatile void __iomem *to, const void *from, size_t count)
{
	WARN_ON(count % 4);
	while (count >= 4) {
		writel(*(u32 *)from, to);
		from += 4;
		to += 4;
		count -= 4;
	}
}

static void smbl_memcpy_fromio(void *to, const volatile void __iomem *from, size_t count)
{
	WARN_ON(count % 4);
	while (count >= 4) {
		*(u32 *)to = readl(from);
		from += 4;
		to += 4;
		count -= 4;
	}
}

static s32 de_smbl_get_hist(struct de_smbl_handle *hdl, u32 *cnt)
{
	struct de_smbl_private *priv = hdl->private;
	/* Read histogram from hw reg*/
	u8 *reg_addr = (u8 *)
		(priv->reg_blks[SMBL_HIST_REG_BLK].reg_addr);

	smbl_memcpy_fromio((u8 *)cnt, reg_addr, sizeof(u32) * IEP_LH_INTERVAL_NUM);
	return 0;
}

/**
 *	function       : pwrsave_core(struct de_smbl_handle *hdl)
 *
 *	description    : Power Save alg core
 *			 Dynamic adjust backlight and lgc gain through screen
 *			 content and user backlight setting
 *	parameters     :
 *			hdl         <smbl handle>
 *	return         :
 *			lgcaddr     <LGC table pointer>
 *	history        :
 *			Add HANG-UP DETECT: When use pwrsave_core in LOW
 *			referential backlight condiction, backlight will
 *			flicker. So STOP use pwrsave_core.
 */
static u16 *pwrsave_core(struct de_smbl_handle *hdl)
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
	struct de_smbl_private *priv = hdl->private;

	printf_cnt = 0;

	mutex_lock(&priv->status_lock);
	backlight = priv->smbl_status->backlight;
	if (backlight < priv->PWRSAVE_PROC_THRES) {
		/* if current backlight lt PWRSAVE_PROC_THRES, close smart */
		/* backlight function */
		memset(priv->smbl_status->min_adj_index_hist, 255,
		       sizeof(u8) * IEP_LH_PWRSV_NUM);
		lgcaddr = (u16 *)((uintptr_t) priv->pttab +
			  ((128 - 1) << 9));

		priv->smbl_status->dimming = 256;
	} else {
		/* if current backlight ge PWRSAVE_PROC_THRES, */
		/* open smart backlight function */
		p95 = 0;

		hist_region_num =
			   (hist_region_num > 8) ? 8 : IEP_LH_INTERVAL_NUM;

		/* read histogram result */
		de_smbl_get_hist(hdl, histcnt);

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
		size = priv->smbl_status->size;

		/* calculate some var */
		hist[0] = (histcnt[0]) / size;
		for (i = 1; i < hist_region_num; i++)
			hist[i] = (histcnt[i]) / size + hist[i - 1];

#if defined(__DISP_TEMP_CODE__)
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
			if (hist[i] < 80)
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

		/* DE_INFO("min_adj_index: %d\n", min_adj_index); */
		for (i = 0; i < IEP_LH_PWRSV_NUM - 1; i++) {
			priv->smbl_status->min_adj_index_hist[i] =
			    priv->smbl_status->min_adj_index_hist[i + 1];
		}
		priv->smbl_status->min_adj_index_hist[IEP_LH_PWRSV_NUM - 1] =
		    min_adj_index;

		for (i = 0; i < IEP_LH_PWRSV_NUM; i++) {
			/* drc_filter_total += drc_filter[i]; */
			drc_filter_tmp +=
			    drc_filter[i] *
			    priv->smbl_status->min_adj_index_hist[i];
		}
		/* min_adj_index = drc_filter_tmp/drc_filter_total; */
		min_adj_index = drc_filter_tmp >> 10;

		thres_low = priv->PWRSAVE_PROC_THRES;
		thres_high =
		    priv->PWRSAVE_PROC_THRES + ((255 - priv->PWRSAVE_PROC_THRES) / 5);
		if (backlight < thres_high) {
			min_adj_index =
			   min_adj_index + (255 - min_adj_index) *
			   (thres_high - backlight) / (thres_high - thres_low);
		}
		min_adj_index =
		    (min_adj_index >= 255) ?
		    255 : ((min_adj_index < hist_thres_pwrsv[0]) ?
		    hist_thres_pwrsv[0] : min_adj_index);

		priv->smbl_status->dimming = min_adj_index + 1;

		lgcaddr =
		    (u16 *)((uintptr_t) priv->pttab +
					((min_adj_index - 128) << 9));

		if (printf_cnt == 600) {
			DRM_DEBUG_DRIVER("save backlight power: %d percent\n",
			      (256 - (u32) min_adj_index) * 100 / 256);
			printf_cnt = 0;
		} else {
			printf_cnt++;
		}

	}
	priv->smbl_status->backlight_after_dimming = backlight * priv->smbl_status->dimming / 256;
	mutex_unlock(&priv->status_lock);
	return lgcaddr;
}

/* write to hw reg */
static s32 de_smbl_set_lut(struct de_smbl_handle *hdl, u16 *lut)
{
	struct de_smbl_private *priv = hdl->private;
	struct smbl_reg *reg = get_smbl_reg(priv);

	/* set lut to smbl lut SRAM */
	mutex_lock(&priv->regs_lock);
	smbl_memcpy_toio((void *)reg->drclgcoff, (void *)lut, sizeof(reg->drclgcoff));
	smbl_set_block_dirty(priv, SMBL_LUT_REG_BLK, 1);
	mutex_unlock(&priv->regs_lock);
	return 0;
}

static s32 de_smbl_enable(struct de_smbl_handle *hdl, u32 en)
{
	struct de_smbl_private *priv = hdl->private;
	struct smbl_reg *reg = get_smbl_reg(priv);

	mutex_lock(&priv->regs_lock);
	reg->gnectl.bits.en = en;
	smbl_set_block_dirty(priv, SMBL_EN_REG_BLK, 1);
	mutex_unlock(&priv->regs_lock);
	return 0;
}

/* this update hw reg directly, BUG? */
static s32 de_smbl_set_para(struct de_smbl_handle *hdl, u32 width, u32 height)
{
	struct de_smbl_private *priv = hdl->private;
	struct smbl_reg *reg = get_smbl_reg(priv);

	mutex_lock(&priv->regs_lock);
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
	smbl_memcpy_toio((void *)(priv->reg_blks[SMBL_CSC_REG_BLK].reg_addr),
	       (u8 *)csc_bypass_coeff, sizeof(u32) * 12);

	/* filter coeff */
	smbl_memcpy_toio((void *)(priv->reg_blks[SMBL_FILTER_REG_BLK].reg_addr),
	      (u8 *)smbl_filter_coeff, sizeof(u8) * 272);

	smbl_set_block_dirty(priv, SMBL_EN_REG_BLK, 1);
	smbl_set_block_dirty(priv, SMBL_CTL_REG_BLK, 1);
	mutex_unlock(&priv->regs_lock);

	mutex_lock(&priv->status_lock);
	priv->smbl_status->size = width * height / 100;
	mutex_unlock(&priv->status_lock);

	return 0;
}

static s32 de_smbl_set_window(struct de_smbl_handle *hdl, u32 win_enable,
			struct drm_rect window)
{
	struct de_smbl_private *priv = hdl->private;
	struct smbl_reg *reg = get_smbl_reg(priv);

	mutex_lock(&priv->regs_lock);
	reg->drcctl.bits.win_en = 0;
	if (win_enable && drm_rect_width(&window) && drm_rect_height(&window)) {
		reg->drcctl.bits.win_en = win_enable;
		reg->drc_wp0.bits.win_left = window.x1;
		reg->drc_wp0.bits.win_top = window.y1;
		reg->drc_wp1.bits.win_right = window.x2;
		reg->drc_wp1.bits.win_bottom = window.y2;
	}
	smbl_set_block_dirty(priv, SMBL_CTL_REG_BLK, 1);
	mutex_unlock(&priv->regs_lock);
	return 0;
}

s32 de_smbl_update_local_param(struct de_smbl_handle *hdl)
{
	struct de_smbl_private *priv = hdl->private;
	u16 *lut;

	if (priv->smbl_status->isenable &&
	    ((SMBL_FRAME_MASK == (priv->smbl_frame_cnt % 2))
		|| (SMBL_FRAME_MASK == 0x2))) {
		if (priv->smbl_status->runtime > 0) {
			/* POWER SAVE ALG */
			lut = (u16 *)pwrsave_core(hdl);
		} else {
			lut = (u16 *)pwrsv_lgc_tab[128 - 1];
		}

		de_smbl_set_lut(hdl, lut);

		mutex_lock(&priv->status_lock);
		if (priv->smbl_status->runtime == 0)
			priv->smbl_status->runtime++;
		mutex_unlock(&priv->status_lock);
	}
	priv->smbl_frame_cnt++;

	return 0;
}

s32 de_smbl_apply(struct de_smbl_handle *hdl, struct disp_smbl_info *info)
{
	struct de_smbl_private *priv = hdl->private;
	u16 *lut;

	DRM_DEBUG_DRIVER("smbl en=%d, dirty_mask %x win=<%d,%d,%d,%d>\n", info->enable,
	      info->flags, info->window.x1, info->window.y1, drm_rect_width(&info->window),
	      drm_rect_height(&info->window));

	if (info->flags & SMBL_DIRTY_ENABLE) {
		if (!info->enable) {
			de_smbl_enable(hdl, info->enable);
			mutex_lock(&priv->status_lock);
			priv->smbl_status->isenable = 0;
			mutex_unlock(&priv->status_lock);
			return 0;
		}

		if (drm_rect_width(&info->size) > priv->dsc->width_max ||
		    drm_rect_height(&info->size) > priv->dsc->height_max) {
			DRM_ERROR("[SUNXI-CRTC] smbl input size exceeds limit(%dx%d), actual(%dx%d)",
				  priv->dsc->width_max, priv->dsc->height_max,
				  drm_rect_width(&info->size), drm_rect_height(&info->size));
			de_smbl_enable(hdl, 0);
			mutex_lock(&priv->status_lock);
			priv->smbl_status->isenable = 0;
			mutex_unlock(&priv->status_lock);
			return -1;
		}

		de_smbl_enable(hdl, info->enable);

		if (priv->smbl_status->isenable == false && info->enable == true) {
			mutex_lock(&priv->status_lock);
			priv->smbl_status->runtime = 0;
			memset(priv->smbl_status->min_adj_index_hist, 255,
			       sizeof(u8) * IEP_LH_PWRSV_NUM);
			mutex_unlock(&priv->status_lock);
			de_smbl_set_para(hdl, drm_rect_width(&info->size),
					 drm_rect_height(&info->size));

			/* In some cases, resume will not run tasklet immediately,
			 * which will cause the flower screen. Manually copy lut regs here
			 */
			lut = (u16 *)pwrsv_lgc_tab[128 - 1];
			de_smbl_set_lut(hdl, lut);
		} else {
		}
		mutex_lock(&priv->status_lock);
		priv->smbl_status->isenable = info->enable;
		mutex_unlock(&priv->status_lock);
	}

	mutex_lock(&priv->status_lock);
	if (info->flags & SMBL_DIRTY_BL)
		priv->smbl_status->backlight = info->backlight;
	mutex_unlock(&priv->status_lock);

	if (info->flags & SMBL_DIRTY_WINDOW)
		de_smbl_set_window(hdl, info->demo_en, info->window);

	return 0;
}

s32 de_smbl_get_status(struct de_smbl_handle *hdl, struct disp_smbl_info *info)
{
	struct de_smbl_private *priv = hdl->private;

	if (!info)
		return -1;

	mutex_lock(&priv->status_lock);
	info->enable = priv->smbl_status->isenable;
	info->backlight = priv->smbl_status->backlight;
	info->backlight_after_dimming = priv->smbl_status->backlight_after_dimming;
	info->backlight_dimming = priv->smbl_status->dimming;
	mutex_unlock(&priv->status_lock);
	return 0;
}

static int de_smbl_set_csc(struct de_smbl_handle *hdl, u32 en, u32 w, u32 h, int *csc_coeff)
{
	struct de_smbl_private *priv = hdl->private;
	struct smbl_reg *reg = get_smbl_reg(priv);

	if (!en || csc_coeff == NULL) {
		reg->gnectl.bits.incsc_en = 0x0;
		smbl_set_block_dirty(priv, SMBL_EN_REG_BLK, 1);
		return -1;
	}
	reg->gnectl.bits.incsc_en = 0x1;
	reg->incscycoff[0].dwval = *(csc_coeff);
	reg->incscycoff[1].dwval = *(csc_coeff + 1);
	reg->incscycoff[2].dwval = *(csc_coeff + 2);
	reg->incscycon.dwval = *(csc_coeff + 3) >> 6;
	reg->incscucoff[0].dwval = *(csc_coeff + 4);
	reg->incscucoff[1].dwval = *(csc_coeff + 5);
	reg->incscucoff[2].dwval = *(csc_coeff + 6);
	reg->incscucon.dwval = *(csc_coeff + 7) >> 6;
	reg->incscvcoff[0].dwval = *(csc_coeff + 8);
	reg->incscvcoff[1].dwval = *(csc_coeff + 9);
	reg->incscvcoff[2].dwval = *(csc_coeff + 10);
	reg->incscvcon.dwval = *(csc_coeff + 11) >> 6;

	smbl_set_block_dirty(priv, SMBL_EN_REG_BLK, 1);
	smbl_set_block_dirty(priv, SMBL_ICSC_REG_BLK, 1);
	return 0;
}

int de_smbl_apply_csc(struct de_smbl_handle *hdl, u32 w, u32 h, const struct de_csc_info *in_info,
		    const struct de_csc_info *out_info, const struct bcsh_info *bcsh, const struct ctm_info *ctm)
{
	int csc_coeff[12];
	if (!hdl->private->csc)
		return -1;

	if (!in_info || !out_info || !bcsh || !ctm || (!bcsh->enable && !ctm->enable)) {
		de_smbl_set_csc(hdl, 0, 0, 0, NULL);
		return 0;
	}

	de_dcsc_apply(hdl->private->csc, in_info, out_info, bcsh, ctm, csc_coeff, false);
	de_smbl_set_csc(hdl, 1, w, h, csc_coeff);
	return 0;
}

void de_smbl_dump_state(struct drm_printer *p, struct de_smbl_handle *hdl)
{
	struct de_smbl_private *priv = hdl->private;
	unsigned long base = (unsigned long)hdl->private->reg_blks[0].reg_addr;
	unsigned long de_base = (unsigned long)hdl->cinfo.de_reg_base;

	drm_printf(p, "\tsmbl%d@%8x:\n", hdl->private->dsc->id, (unsigned int)(base - de_base));
	mutex_lock(&priv->status_lock);
	drm_printf(p, "\t\tsmbl_tbl %s, dimming: %d, backlight: %d\n",
		   priv->smbl_status->isenable ? "on" : "off", priv->smbl_status->dimming,
		   priv->smbl_status->backlight);
	mutex_unlock(&priv->status_lock);
}

struct de_smbl_handle *de_smbl_create(struct module_create_info *info)
{
	struct de_smbl_private *priv;
	struct de_reg_mem_info *reg_mem_info;
	struct de_reg_block *reg_blk;
	const struct de_smbl_dsc *dsc;
	struct module_create_info csc;
	struct de_smbl_handle *hdl;
	struct csc_extra_create_info excsc;
	u8 __iomem *base;
	u32 lcdgamma = 6, i;

	dsc = get_smbl_dsc(info);
	if (!dsc)
		return NULL;

	base = (u8 __iomem *)(info->de_reg_base + info->reg_offset + dsc->reg_offset);
	hdl = kmalloc(sizeof(*hdl), GFP_KERNEL | __GFP_ZERO);
	memcpy(&hdl->cinfo, info, sizeof(*info));
	hdl->private = kmalloc(sizeof(*hdl->private), GFP_KERNEL | __GFP_ZERO);
	hdl->private->dsc = dsc;
	hdl->support_csc = dsc->support_csc;
	if (dsc->support_csc) {
		excsc.type = SMBL_CSC;
		excsc.extra_id = 0;
		memcpy(&csc, info, sizeof(*info));
		csc.extra = &excsc;
		hdl->private->csc = de_csc_create(&csc);
		WARN_ON(!hdl->private->csc);
		if (hdl->private->csc)
			hdl->hue_default_value = hdl->private->csc->hue_default_value;
	}

	priv = hdl->private;
	priv->PWRSAVE_PROC_THRES = 85;

	reg_mem_info = &(hdl->private->reg_mem_info);

	reg_mem_info->size = sizeof(struct smbl_reg);
	reg_mem_info->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
		reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
		info->update_mode == RCQ_MODE);

	if (reg_mem_info->vir_addr == NULL) {
		DRM_ERROR("alloc smbl[%d] mm fail!size=0x%x\n",
			 dsc->id, reg_mem_info->size);
		return ERR_PTR(-ENOMEM);
	}

	priv->reg_blk_num = SMBL_REG_BLK_NUM;

	reg_blk = &(priv->reg_blks[SMBL_EN_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr;
	reg_blk->vir_addr = reg_mem_info->vir_addr;
	reg_blk->size = 0x8;
	reg_blk->reg_addr = (u8 __iomem *)base;

	reg_blk = &(priv->reg_blks[SMBL_CTL_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x10;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x10;
	reg_blk->size = 0x30;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x10);

	reg_blk = &(priv->reg_blks[SMBL_HIST_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x60;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x60;
	reg_blk->size = 0x20;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x60);

	reg_blk = &(priv->reg_blks[SMBL_ICSC_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x80;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x80;
	reg_blk->size = 0x30;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x80);

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

	priv->smbl_status = kmalloc(sizeof(struct __smbl_status_t),
		GFP_KERNEL | __GFP_ZERO);
	if (priv->smbl_status == NULL) {
		DRM_ERROR("malloc g_smbl_status[%d] memory fail! size=0x%x\n", dsc->id,
		      (u32)sizeof(struct __smbl_status_t));
		return ERR_PTR(-ENOMEM);
	}

	mutex_init(&priv->status_lock);
	mutex_init(&priv->regs_lock);

	priv->smbl_status->isenable = 0;
	priv->smbl_status->runtime = 0;
	priv->smbl_status->dimming = 256;
	priv->pttab = pwrsv_lgc_tab[128 * lcdgamma];

	hdl->block_num = priv->reg_blk_num;
	hdl->block = kmalloc(sizeof(reg_blk[0]) * hdl->block_num, GFP_KERNEL | __GFP_ZERO);
	for (i = 0; i < hdl->private->reg_blk_num; i++)
		hdl->block[i] = &priv->reg_blks[i];

	return hdl;
}
