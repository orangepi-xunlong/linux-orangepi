/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2021 Copyright (c)
 *
 *  File name   :  display engine 35x dci basic function definition
 *
 *  History     :  2021/11/30 v0.1  Initial version
 *
 ******************************************************************************/

#include <linux/mutex.h>
#include "de_dci_type.h"
#include "de_dci.h"

#define NUM_BLKS 16
#define HIST_BINS 16
#define PDF_REC_NUM 2 /*need two record to calculate*/
#define DCI_LEVEL_NUM 10
#define DCI_DATA_NUM 15

enum {
	DCI_PARA_REG_BLK = 0,
	DCI_CDF_REG_BLK,
	DCI_PDF_REG_BLK,
	DCI_REG_BLK_NUM,
};

struct de_dci_private {
	struct de_reg_mem_info reg_mem_info;
	struct de_reg_mem_info reg_shadow;
	u32 reg_blk_num;
	struct dci_status status;
	struct de_reg_block reg_blks[DCI_REG_BLK_NUM];
	/* dirty of shadow_blks is a mask for update request */
	struct de_reg_block shadow_blks[DCI_REG_BLK_NUM];
	u32 *g_cur_pdf;
	u32 *g_pdf[PDF_REC_NUM];
	u32 g_last_cdfs[NUM_BLKS][HIST_BINS];
	u16 g_update_rate;
	bool g_start;
	u8 demo_hor_start;
	u8 demo_hor_end;
	u8 demo_ver_start;
	u8 demo_ver_end;

	/* Frame number of dci run */
	u32 runtime;
	/* dci enabled */
	u32 frame_cnt;

	struct mutex lock;

	s32 (*de_dci_enable)(struct de_dci_handle *hdl, u32 enable);
	s32 (*de_dci_set_size)(struct de_dci_handle *hdl, u32 width, u32 height);
	s32 (*de_dci_set_window)(struct de_dci_handle *hdl,
		      u32 x, u32 y, u32 w, u32 h);
	s32 (*de_dci_set_color_range)(struct de_dci_handle *hdl, enum de_color_range cr);
	s32 (*de_dci_update_local_param)(struct de_dci_handle *hdl);
};

struct de_dci_handle *de35x_dci_create(struct module_create_info *info);
struct de_dci_handle *de_dci_create(struct module_create_info *info)
{
	return de35x_dci_create(info);
}

s32 de_dci_enable(struct de_dci_handle *hdl, u32 enable)
{
	DRM_DEBUG_DRIVER("[SUNXI-DE] %s %d \n", __FUNCTION__, __LINE__);
	if (hdl->private->de_dci_enable)
		return hdl->private->de_dci_enable(hdl, enable);
	else
		return 0;
}

s32 de_dci_set_size(struct de_dci_handle *hdl, u32 width, u32 height)
{
	if (hdl->private->de_dci_set_size)
		return hdl->private->de_dci_set_size(hdl, width, height);
	else
		return 0;
}

s32 de_dci_set_window(struct de_dci_handle *hdl,
		      u32 x, u32 y, u32 w, u32 h)
{
	if (hdl->private->de_dci_set_window)
		return hdl->private->de_dci_set_window(hdl, x, y, w, h);
	else
		return 0;
}

s32 de_dci_update_local_param(struct de_dci_handle *hdl)
{
	if (hdl->private->de_dci_update_local_param)
		return hdl->private->de_dci_update_local_param(hdl);
	else
		return 0;
}

s32 de35x_dci_set_color_range(struct de_dci_handle *hdl, enum de_color_range cr)
{
	if (hdl->private->de_dci_set_color_range)
		return hdl->private->de_dci_set_color_range(hdl, cr);
	else
		return 0;
}

static void dci_set_block_dirty(
	struct de_dci_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
	if (priv->reg_blks[blk_id].rcq_hd)
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
}

static void de_dci_request_update(struct de_dci_private *priv, u32 blk_id, u32 dirty)
{
	priv->shadow_blks[blk_id].dirty = dirty;
}

void de_dci_update_regs(struct de_dci_handle *hdl)
{
	u32 blk_id = 0;
	struct de_reg_block *shadow_block;
	struct de_reg_block *block;
	struct de_dci_private *priv = hdl->private;

	mutex_lock(&priv->lock);
	for (blk_id = 0; blk_id < DCI_REG_BLK_NUM; blk_id++) {
		shadow_block = &(priv->shadow_blks[blk_id]);
		block = &(priv->reg_blks[blk_id]);

		if (shadow_block->dirty) {
			memcpy(block->vir_addr, shadow_block->vir_addr, shadow_block->size);
			DRM_DEBUG_DRIVER("[SUNXI-DE] %s %d blk_id:%d\n", __FUNCTION__, __LINE__, blk_id);
			dci_set_block_dirty(priv, blk_id, 1);
			shadow_block->dirty = 0;
		}
	}
	mutex_unlock(&priv->lock);
}

static inline struct dci_reg *de35x_get_dci_reg(struct de_dci_private *priv)
{
	return (struct dci_reg *)(priv->reg_blks[DCI_PARA_REG_BLK].vir_addr);
}

static inline struct dci_reg *de35x_get_dci_shadow_reg(struct de_dci_private *priv)
{
	return (struct dci_reg *)(priv->shadow_blks[DCI_PARA_REG_BLK].vir_addr);
}

s32 de_dci_dump_state(struct drm_printer *p, struct de_dci_handle *hdl)
{
	struct de_dci_private *priv = hdl->private;
	struct dci_reg *reg = de35x_get_dci_shadow_reg(priv);
	unsigned long base = (unsigned long)hdl->private->reg_blks[0].reg_addr;
	unsigned long de_base = (unsigned long)hdl->cinfo.de_reg_base;

	drm_printf(p, "\n\tdci@%8x: %sable\n", (unsigned int)(base - de_base),
			  reg->ctl.bits.en ? "en" : "dis");
	return 0;
}

s32 de35x_dci_set_size(struct de_dci_handle *hdl, u32 width, u32 height)
{
	struct de_dci_private *priv = hdl->private;
	struct dci_reg *reg = de35x_get_dci_shadow_reg(priv);

	mutex_lock(&priv->lock);

	reg->size.bits.width = width - 1;
	reg->size.bits.height = height - 1;

	reg->demo_horz.bits.demo_horz_start =
	    width * priv->demo_hor_start / 100;
	reg->demo_horz.bits.demo_horz_end =
	    width * priv->demo_hor_end / 100;
	reg->demo_vert.bits.demo_vert_start =
	    height * priv->demo_ver_start / 100;
	reg->demo_vert.bits.demo_vert_end =
	    height * priv->demo_ver_end / 100;

	de_dci_request_update(priv, DCI_PARA_REG_BLK, 1);

	mutex_unlock(&priv->lock);

	return 0;
}

s32 de_dci_set_demo_mode(struct de_dci_handle *hdl, bool enable)
{
	struct de_dci_private *priv = hdl->private;
	struct dci_reg *reg = de35x_get_dci_shadow_reg(priv);

	mutex_lock(&priv->lock);
	reg->ctl.bits.demo_en = enable ? 1 : 0;
	mutex_unlock(&priv->lock);
	de_dci_request_update(priv, DCI_PARA_REG_BLK, 1);
	return 0;
}

s32 de35x_dci_set_window(struct de_dci_handle *hdl,
		      u32 x, u32 y, u32 w, u32 h)
{
	struct de_dci_private *priv = hdl->private;

	mutex_lock(&priv->lock);
	priv->demo_hor_start = x;
	priv->demo_hor_end = x + w;
	priv->demo_ver_start = y;
	priv->demo_ver_end = y + h - 1;
	mutex_unlock(&priv->lock);

	return 0;
}

bool de_dci_is_enabled(struct de_dci_handle *hdl)
{
	struct de_dci_private *priv = hdl->private;
	struct dci_reg *reg = de35x_get_dci_shadow_reg(priv);

	return !!reg->ctl.bits.en;
}

s32 de35x_dci_enable(struct de_dci_handle *hdl, u32 en)
{
	struct de_dci_private *priv = hdl->private;
	struct dci_reg *reg = de35x_get_dci_shadow_reg(priv);

	mutex_lock(&priv->lock);
	reg->ctl.bits.en = en ? 1 : 0;
	de_dci_request_update(priv, DCI_PARA_REG_BLK, 1);
	mutex_unlock(&priv->lock);

	return 0;
}

s32 de_dci_set_color_range(struct de_dci_handle *hdl, enum de_color_range cr)
{
	struct de_dci_private *priv = hdl->private;
	struct dci_reg *reg = de35x_get_dci_shadow_reg(priv);

	mutex_lock(&priv->lock);
	reg->color_range.dwval = (cr == DE_COLOR_RANGE_16_235 ? 1 : 0);
	de_dci_request_update(priv, DCI_PARA_REG_BLK, 1);
	mutex_unlock(&priv->lock);

	return 0;
}

s32 de35x_dci_init(struct de_dci_handle *hdl)
{
	struct de_dci_private *priv = hdl->private;
	struct dci_reg *reg = de35x_get_dci_shadow_reg(priv);

	mutex_lock(&priv->lock);
	reg->ctl.dwval = 0x80000000;
	reg->color_range.dwval = 0x1;
	reg->skin_protect.dwval = 0x85850001;
	reg->pdf_radius.dwval = 0x0;
	reg->count_bound.dwval = 0x03ff0000;
	reg->border_map_mode.dwval = 0x1;
	reg->brighten_level0.dwval = 0x02040202;
	reg->brighten_level1.dwval = 0x02020202;
	reg->brighten_level2.dwval = 0x02020202;
	reg->brighten_level3.dwval = 0x20202;
	reg->darken_level0.dwval = 0x2020202;
	reg->darken_level1.dwval = 0x2020202;
	reg->darken_level2.dwval = 0x2020202;
	reg->darken_level3.dwval = 0x20202;
	reg->chroma_comp_br_th0.dwval = 0x32281e14;
	reg->chroma_comp_br_th1.dwval = 0xff73463c;
	reg->chroma_comp_br_gain0.dwval = 0x14141414;
	reg->chroma_comp_br_gain1.dwval = 0x14141414;
	reg->chroma_comp_br_slope0.dwval = 0x06060603;
	reg->chroma_comp_br_slope1.dwval = 0x00010606;
	reg->chroma_comp_dark_th0.dwval = 0x5a46321e;
	reg->chroma_comp_dark_th1.dwval = 0xffaa9682;
	reg->chroma_comp_dark_gain0.dwval = 0x14141414;
	reg->chroma_comp_dark_gain1.dwval = 0x00141414;
	reg->chroma_comp_dark_slope0.dwval = 0x03030302;
	reg->chroma_comp_dark_slope1.dwval = 0x00030301;

	reg->inter_frame_para.dwval = 0x1;
	reg->ftd_hue_thr.dwval = 0x96005a;
	reg->ftd_chroma_thr.dwval = 0x28000a;
	reg->ftd_slp.dwval = 0x4040604;

	de_dci_request_update(priv, DCI_PARA_REG_BLK, 1);
	mutex_unlock(&priv->lock);

	return 0;
}

s32 de35x_dci_update_local_param(struct de_dci_handle *hdl)
{
	struct de_dci_private *priv = hdl->private;
	struct dci_reg *hw_reg = (struct dci_reg *) (priv->reg_blks[0].reg_addr);
	struct dci_reg *reg = de35x_get_dci_shadow_reg(priv);
	int blk_idx = 0;
	int bin_idx = 0;
	u32 *cur_pdf = priv->g_cur_pdf;
	u32 cur_cdfs[NUM_BLKS][HIST_BINS];
	u32 *pdf_pre0 = NULL;
	u32 *pdf_pre1 = NULL;
	u32 *g_pdf_pre0 = NULL;
	u32 *g_pdf_pre1 = NULL;
	int cum_diff_pdfs = 0;
	int cum_pdf_pre0 = 0;
	int diff_pdfs = 0;
	int scene_change_th = 30;

	DRM_DEBUG_DRIVER("[%s]-%d\n", __func__, __LINE__);

	if (!!reg->ctl.bits.en)
		return 0;

	priv->frame_cnt++;

	if (priv->frame_cnt % 2) {
		g_pdf_pre0 = priv->g_pdf[0];
		g_pdf_pre1 = priv->g_pdf[1];
	} else {
		g_pdf_pre0 = priv->g_pdf[1];
		g_pdf_pre1 = priv->g_pdf[0];
	}

	memset(cur_cdfs, 0, sizeof(u32) * NUM_BLKS * HIST_BINS);
	/* Read histogram to pdf[256] */
	for (blk_idx = 0; blk_idx < NUM_BLKS; ++blk_idx) {
		for (bin_idx = 0; bin_idx < HIST_BINS; ++bin_idx) {
			int offset = blk_idx * HIST_BINS + bin_idx;
			*(g_pdf_pre0 + offset) = readl(hw_reg->pdf_stats + offset);
		}
	}

	if (priv->runtime < 1) {
		priv->runtime++;
		return 0;
	}

	for (blk_idx = 0; blk_idx < NUM_BLKS; ++blk_idx) {
		pdf_pre0 = g_pdf_pre0 + blk_idx * HIST_BINS;
		pdf_pre1 = g_pdf_pre1 + blk_idx * HIST_BINS;

		cum_diff_pdfs = 0;
		cum_pdf_pre0 = 0;
		for (bin_idx = 0; bin_idx < HIST_BINS; ++bin_idx) {
			cum_diff_pdfs += abs(pdf_pre0[bin_idx] - pdf_pre1[bin_idx]);
			cum_pdf_pre0 += pdf_pre0[bin_idx];
		}

		diff_pdfs = (cum_diff_pdfs * 100) / cum_pdf_pre0;
		if (diff_pdfs > scene_change_th) {
			memcpy(cur_pdf + blk_idx * HIST_BINS, pdf_pre0,
			       HIST_BINS * sizeof(int));
		} else {
#ifdef NOT_UPDATA_SAME_SCECE_TEMP
			for (bin_idx = 0; bin_idx < HIST_BINS; ++bin_idx) {
				cur_pdf[blk_idx * HIST_BINS + bin_idx] = (pdf_pre0[bin_idx]
					+ pdf_pre1[bin_idx] + 1) >> 1;
				pdf_pre0[bin_idx] = cur_pdf[blk_idx * HIST_BINS + bin_idx];
			}
#endif
		}
	}

	for (blk_idx = 0; blk_idx < NUM_BLKS; ++blk_idx) {
		cur_cdfs[blk_idx][0] = cur_pdf[blk_idx * HIST_BINS + 0];
		for (bin_idx = 1; bin_idx < HIST_BINS; ++bin_idx) {
			cur_cdfs[blk_idx][bin_idx] = cur_cdfs[blk_idx][bin_idx - 1]
			+ cur_pdf[blk_idx * HIST_BINS + bin_idx];
		}
	}

	if (priv->g_start) {
		u16 update_rate = 256 - priv->g_update_rate;
		for (blk_idx = 0; blk_idx < NUM_BLKS; ++blk_idx) {
			for (bin_idx = 0; bin_idx < HIST_BINS; ++bin_idx) {
				cur_cdfs[blk_idx][bin_idx] = (cur_cdfs[blk_idx][bin_idx] * update_rate
					+ priv->g_last_cdfs[blk_idx][bin_idx] * priv->g_update_rate + 128) >> 8;
			}
		}
	}
	priv->g_start = 0;
	memcpy(&priv->g_last_cdfs, cur_pdf, sizeof(u32) * NUM_BLKS * HIST_BINS);
	mutex_lock(&priv->lock);
	memcpy(reg->cdf_config, cur_cdfs, sizeof(u32) * 256);
	mutex_unlock(&priv->lock);
	de_dci_request_update(priv, DCI_CDF_REG_BLK, 1);
	return 0;
}

struct de_dci_handle *de35x_dci_create(struct module_create_info *info)
{
	int i;
	struct de_dci_handle *hdl;
	struct de_reg_block *block;
	struct de_reg_mem_info *reg_mem_info;
	struct de_reg_mem_info *reg_shadow;
	struct de_dci_private *priv;
	u8 __iomem *reg_base;
	const struct de_dci_desc *desc;

	desc = get_dci_desc(info);
	if (!desc)
		return NULL;

	hdl = kmalloc(sizeof(*hdl), GFP_KERNEL | __GFP_ZERO);
	hdl->private = kmalloc(sizeof(*hdl->private), GFP_KERNEL | __GFP_ZERO);
	memcpy(&hdl->cinfo, info, sizeof(*info));

	reg_base = info->de_reg_base + info->reg_offset + desc->reg_offset;
	priv = hdl->private;
	reg_mem_info = &(priv->reg_mem_info);

	reg_mem_info->size = sizeof(struct dci_reg);
	reg_mem_info->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
		reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
		info->update_mode == RCQ_MODE);
	if (NULL == reg_mem_info->vir_addr) {
		DRM_ERROR("alloc bld[%d] mm fail!size=0x%x\n",
		     info->id, reg_mem_info->size);
		return ERR_PTR(-ENOMEM);
	}


	block = &(priv->reg_blks[DCI_PARA_REG_BLK]);
	block->phy_addr = reg_mem_info->phy_addr;
	block->vir_addr = reg_mem_info->vir_addr;
	block->size = 0x100;
	block->reg_addr = reg_base;

	block = &(priv->reg_blks[DCI_CDF_REG_BLK]);
	block->phy_addr = reg_mem_info->phy_addr + 0x100;
	block->vir_addr = reg_mem_info->vir_addr + 0x100;
	block->size = 0x400;
	block->reg_addr = reg_base + 0x100;

	block = &(priv->reg_blks[DCI_PDF_REG_BLK]);
	block->phy_addr = reg_mem_info->phy_addr + 0x500;
	block->vir_addr = reg_mem_info->vir_addr + 0x500;
	block->size = 0x400;
	block->reg_addr = reg_base + 0x500;

	priv->reg_blk_num = DCI_REG_BLK_NUM;

	hdl->block_num = priv->reg_blk_num;
	hdl->block = kmalloc(sizeof(block[0]) * hdl->block_num, GFP_KERNEL | __GFP_ZERO);
	for (i = 0; i < hdl->private->reg_blk_num; i++)
		hdl->block[i] = &priv->reg_blks[i];

	/* create shadow block */
	reg_shadow = &(priv->reg_shadow);

	reg_shadow->size = sizeof(struct dci_reg);
	reg_shadow->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
		reg_shadow->size, (void *)&(reg_shadow->phy_addr),
		info->update_mode == RCQ_MODE);
	if (NULL == reg_shadow->vir_addr) {
		DRM_ERROR("alloc bld[%d] mm fail!size=0x%x\n",
		     info->id, reg_shadow->size);
		return ERR_PTR(-ENOMEM);
	}

	block = &(priv->shadow_blks[DCI_PARA_REG_BLK]);
	block->phy_addr = reg_shadow->phy_addr;
	block->vir_addr = reg_shadow->vir_addr;
	block->size = 0x100;

	block = &(priv->shadow_blks[DCI_CDF_REG_BLK]);
	block->phy_addr = reg_shadow->phy_addr + 0x100;
	block->vir_addr = reg_shadow->vir_addr + 0x100;
	block->size = 0x400;

	block = &(priv->shadow_blks[DCI_PDF_REG_BLK]);
	block->phy_addr = reg_shadow->phy_addr + 0x500;
	block->vir_addr = reg_shadow->vir_addr + 0x500;
	block->size = 0x400;


	priv->g_cur_pdf = kmalloc(NUM_BLKS * HIST_BINS * sizeof(u32),
				  GFP_KERNEL | __GFP_ZERO);
	if (priv->g_cur_pdf == NULL) {
		DRM_ERROR("malloc g_cur_pdf memory for channel:%d fail! \n", info->id);
		return ERR_PTR(-ENOMEM);
	}

	for (i = 0; i < PDF_REC_NUM; i++) {
		priv->g_pdf[i] =
			kmalloc(NUM_BLKS * HIST_BINS * sizeof(u32),
				GFP_KERNEL | __GFP_ZERO);
		if (priv->g_pdf[i] == NULL) {
			DRM_ERROR("malloc g_pdf memory for channel:%d fail\n", info->id);
			return ERR_PTR(-ENOMEM);
		}
	}

	mutex_init(&priv->lock);

	de35x_dci_init(hdl);

	hdl->private->de_dci_enable = de35x_dci_enable;
	hdl->private->de_dci_set_size = de35x_dci_set_size;
	hdl->private->de_dci_set_window = de35x_dci_set_window;
	hdl->private->de_dci_set_color_range = de35x_dci_set_color_range;
	hdl->private->de_dci_update_local_param = de35x_dci_update_local_param;

	priv->g_update_rate = 243;
	priv->g_start = false;

	return hdl;
}


int de_dci_pq_proc(struct de_dci_handle *hdl, dci_module_param_t *para)
{
	struct de_dci_private *priv = hdl->private;
	struct dci_reg *reg = de35x_get_dci_shadow_reg(priv);

	if (para->cmd == PQ_READ) {
		para->value[0] = reg->ctl.bits.en;
		para->value[1] = reg->ctl.bits.demo_en;
		para->value[2]  = reg->ctl.bits.chroma_comp_en;
		para->value[3]  = reg->color_range.bits.input_color_space;
		para->value[4]  = reg->skin_protect.bits.skin_en;
		para->value[5]  = reg->inter_frame_para.bits.lpf_pdf_en;
		para->value[6]  = reg->skin_protect.bits.skin_darken_w;
		para->value[7]  = reg->skin_protect.bits.skin_brighten_w;
		para->value[8]  = reg->brighten_level0.bits.brighten_level_0;
		para->value[9]  = reg->brighten_level0.bits.brighten_level_1;
		para->value[10] = reg->brighten_level0.bits.brighten_level_2;
		para->value[11] = reg->brighten_level0.bits.brighten_level_3;

		para->value[12] = reg->brighten_level1.bits.brighten_level_4;
		para->value[13] = reg->brighten_level1.bits.brighten_level_5;
		para->value[14] = reg->brighten_level1.bits.brighten_level_6;
		para->value[15] = reg->brighten_level1.bits.brighten_level_7;

		para->value[16] = reg->brighten_level2.bits.brighten_level_8;
		para->value[17] = reg->brighten_level2.bits.brighten_level_9;
		para->value[18] = reg->brighten_level2.bits.brighten_level_10;
		para->value[19] = reg->brighten_level2.bits.brighten_level_11;

		para->value[20] = reg->brighten_level3.bits.brighten_level_12;
		para->value[21] = reg->brighten_level3.bits.brighten_level_13;
		para->value[22] = reg->brighten_level3.bits.brighten_level_14;

		para->value[23] = reg->darken_level0.bits.darken_level_0;
		para->value[24] = reg->darken_level0.bits.darken_level_1;
		para->value[25] = reg->darken_level0.bits.darken_level_2;
		para->value[26] = reg->darken_level0.bits.darken_level_3;

		para->value[27] = reg->darken_level1.bits.darken_level_4;
		para->value[28] = reg->darken_level1.bits.darken_level_5;
		para->value[29] = reg->darken_level1.bits.darken_level_6;
		para->value[30] = reg->darken_level1.bits.darken_level_7;

		para->value[31] = reg->darken_level2.bits.darken_level_8;
		para->value[32] = reg->darken_level2.bits.darken_level_9;
		para->value[33] = reg->darken_level2.bits.darken_level_10;
		para->value[34] = reg->darken_level2.bits.darken_level_11;

		para->value[35] = reg->darken_level3.bits.darken_level_12;
		para->value[36] = reg->darken_level3.bits.darken_level_13;
		para->value[37] = reg->darken_level3.bits.darken_level_14;

		para->value[38] = reg->chroma_comp_br_gain0.bits.c_comp_br_gain0;
		para->value[39] = reg->chroma_comp_br_gain0.bits.c_comp_br_gain1;
		para->value[40] = reg->chroma_comp_br_gain0.bits.c_comp_br_gain2;
		para->value[41] = reg->chroma_comp_br_gain0.bits.c_comp_br_gain3;

		para->value[42] = reg->chroma_comp_br_gain1.bits.c_comp_br_gain4;
		para->value[43] = reg->chroma_comp_br_gain1.bits.c_comp_br_gain5;
		para->value[44] = reg->chroma_comp_br_gain1.bits.c_comp_br_gain6;
		para->value[45] = reg->chroma_comp_br_gain1.bits.c_comp_br_gain7;

		para->value[46] = reg->chroma_comp_dark_gain0.bits.c_comp_dk_gain0;
		para->value[47] = reg->chroma_comp_dark_gain0.bits.c_comp_dk_gain1;
		para->value[48] = reg->chroma_comp_dark_gain0.bits.c_comp_dk_gain2;
		para->value[49] = reg->chroma_comp_dark_gain0.bits.c_comp_dk_gain3;

		para->value[50] = reg->chroma_comp_dark_gain1.bits.c_comp_dk_gain4;
		para->value[51] = reg->chroma_comp_dark_gain1.bits.c_comp_dk_gain5;
		para->value[52] = reg->chroma_comp_dark_gain1.bits.c_comp_dk_gain6;
		para->value[53] = reg->chroma_comp_dark_gain1.bits.c_comp_dk_gain7;

		/*para->value[53] = reg->demo_horz.bits.demo_horz_start;
		para->value[54] = reg->demo_horz.bits.demo_horz_end;
		para->value[55] = reg->demo_vert.bits.demo_vert_start;
		para->value[56] = reg->demo_vert.bits.demo_vert_end;*/
		para->value[54] = priv->demo_hor_start;
		para->value[55] = priv->demo_hor_end;
		para->value[56] = priv->demo_ver_start;
		para->value[57] = priv->demo_ver_end;

		para->value[58] = reg->ftd_hue_thr.bits.ftd_hue_low_thr;
		para->value[59] = reg->ftd_hue_thr.bits.ftd_hue_high_thr;
		para->value[60] = reg->ftd_chroma_thr.bits.ftd_chr_low_thr;
		para->value[61] = reg->ftd_chroma_thr.bits.ftd_chr_high_thr;
		para->value[62] = reg->ftd_slp.bits.ftd_hue_low_slp;
		para->value[63] = reg->ftd_slp.bits.ftd_hue_high_slp;
		para->value[64] = reg->ftd_slp.bits.ftd_chr_low_slp;
		para->value[65] = reg->ftd_slp.bits.ftd_chr_high_slp;
	} else { /* write */
		reg->ctl.bits.en = para->value[0];
		reg->ctl.bits.demo_en = para->value[1];
		reg->ctl.bits.chroma_comp_en = para->value[2];
		reg->color_range.bits.input_color_space = para->value[3];
		reg->skin_protect.bits.skin_en = para->value[4];
		reg->inter_frame_para.bits.lpf_pdf_en = para->value[5];
		reg->skin_protect.bits.skin_darken_w = para->value[6];
		reg->skin_protect.bits.skin_brighten_w = para->value[7];
		reg->brighten_level0.bits.brighten_level_0 = para->value[8];
		reg->brighten_level0.bits.brighten_level_1 = para->value[9];
		reg->brighten_level0.bits.brighten_level_2 = para->value[10];
		reg->brighten_level0.bits.brighten_level_3 = para->value[11];

		reg->brighten_level1.bits.brighten_level_4 = para->value[12];
		reg->brighten_level1.bits.brighten_level_5 = para->value[13];
		reg->brighten_level1.bits.brighten_level_6 = para->value[14];
		reg->brighten_level1.bits.brighten_level_7 = para->value[15];

		reg->brighten_level2.bits.brighten_level_8 = para->value[16];
		reg->brighten_level2.bits.brighten_level_9 = para->value[17];
		reg->brighten_level2.bits.brighten_level_10 = para->value[18];
		reg->brighten_level2.bits.brighten_level_11 = para->value[19];

		reg->brighten_level3.bits.brighten_level_12 = para->value[20];
		reg->brighten_level3.bits.brighten_level_13 = para->value[21];
		reg->brighten_level3.bits.brighten_level_14 = para->value[22];

		reg->darken_level0.bits.darken_level_0 = para->value[23];
		reg->darken_level0.bits.darken_level_1 = para->value[24];
		reg->darken_level0.bits.darken_level_2 = para->value[25];
		reg->darken_level0.bits.darken_level_3 = para->value[26];

		reg->darken_level1.bits.darken_level_4 = para->value[27];
		reg->darken_level1.bits.darken_level_5 = para->value[28];
		reg->darken_level1.bits.darken_level_6 = para->value[29];
		reg->darken_level1.bits.darken_level_7 = para->value[30];

		reg->darken_level2.bits.darken_level_8 = para->value[31];
		reg->darken_level2.bits.darken_level_9 = para->value[32];
		reg->darken_level2.bits.darken_level_10 = para->value[33];
		reg->darken_level2.bits.darken_level_11 = para->value[34];

		reg->darken_level3.bits.darken_level_12 = para->value[35];
		reg->darken_level3.bits.darken_level_13 = para->value[36];
		reg->darken_level3.bits.darken_level_14 = para->value[37];

		reg->chroma_comp_br_gain0.bits.c_comp_br_gain0 = para->value[38];
		reg->chroma_comp_br_gain0.bits.c_comp_br_gain1 = para->value[39];
		reg->chroma_comp_br_gain0.bits.c_comp_br_gain2 = para->value[40];
		reg->chroma_comp_br_gain0.bits.c_comp_br_gain3 = para->value[41];

		reg->chroma_comp_br_gain1.bits.c_comp_br_gain4 = para->value[42];
		reg->chroma_comp_br_gain1.bits.c_comp_br_gain5 = para->value[43];
		reg->chroma_comp_br_gain1.bits.c_comp_br_gain6 = para->value[44];
		reg->chroma_comp_br_gain1.bits.c_comp_br_gain7 = para->value[45];

		reg->chroma_comp_dark_gain0.bits.c_comp_dk_gain0 = para->value[46];
		reg->chroma_comp_dark_gain0.bits.c_comp_dk_gain1 = para->value[47];
		reg->chroma_comp_dark_gain0.bits.c_comp_dk_gain2 = para->value[48];
		reg->chroma_comp_dark_gain0.bits.c_comp_dk_gain3 = para->value[49];

		reg->chroma_comp_dark_gain1.bits.c_comp_dk_gain4 = para->value[50];
		reg->chroma_comp_dark_gain1.bits.c_comp_dk_gain5 = para->value[51];
		reg->chroma_comp_dark_gain1.bits.c_comp_dk_gain6 = para->value[52];
		reg->chroma_comp_dark_gain1.bits.c_comp_dk_gain7 = para->value[53];

		/*reg->demo_horz.bits.demo_horz_start = para->value[53];
		reg->demo_horz.bits.demo_horz_end = para->value[54];
		reg->demo_vert.bits.demo_vert_start = para->value[55];
		reg->demo_vert.bits.demo_vert_end = para->value[56];*/

		priv->demo_hor_start = para->value[54];
		priv->demo_hor_end = para->value[55];
		priv->demo_ver_start = para->value[56];
		priv->demo_ver_end = para->value[57];
		reg->demo_horz.bits.demo_horz_start =
		    (reg->size.bits.width + 1) * priv->demo_hor_start / 100;
		reg->demo_horz.bits.demo_horz_end =
		    (reg->size.bits.width + 1)* priv->demo_hor_end / 100;
		reg->demo_vert.bits.demo_vert_start =
		    (reg->size.bits.height + 1) * priv->demo_ver_start / 100;
		reg->demo_vert.bits.demo_vert_end =
		    (reg->size.bits.height + 1) * priv->demo_ver_end / 100;

		reg->ftd_hue_thr.bits.ftd_hue_low_thr = para->value[58];
		reg->ftd_hue_thr.bits.ftd_hue_high_thr = para->value[59];
		reg->ftd_chroma_thr.bits.ftd_chr_low_thr = para->value[60];
		reg->ftd_chroma_thr.bits.ftd_chr_high_thr = para->value[61];
		reg->ftd_slp.bits.ftd_hue_low_slp = para->value[62];
		reg->ftd_slp.bits.ftd_hue_high_slp = para->value[63];
		reg->ftd_slp.bits.ftd_chr_low_slp = para->value[64];
		reg->ftd_slp.bits.ftd_chr_high_slp = para->value[65];

		de_dci_request_update(priv, DCI_PARA_REG_BLK, 1);
	}
	return 0;
}
