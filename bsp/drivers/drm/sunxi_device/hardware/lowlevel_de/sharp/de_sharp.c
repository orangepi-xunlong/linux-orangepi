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
 *  File name   :  display engine 35x sharp basic function definition
 *
 *  History     :  2021/11/30 v0.1  Initial version
 *
 ******************************************************************************/

#include <linux/mutex.h>
#include "de_sharp_type.h"
#include "de_sharp.h"

struct de_sharp_handle *de35x_sharp_create(struct module_create_info *info);
enum {
	SHARP_PARA_REG_BLK = 0,
	SHARP_REG_BLK_NUM,
};

struct de_sharp_private {
	struct de_reg_mem_info reg_mem_info;
	struct de_reg_mem_info reg_shadow;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[SHARP_REG_BLK_NUM];
	struct de_reg_block shadow_blks[SHARP_REG_BLK_NUM];
	struct mutex lock;
	u8 demo_hor_start;
	u8 demo_hor_end;
	u8 demo_ver_start;
	u8 demo_ver_end;

	s32 (*de_sharp_enable)(struct de_sharp_handle *hdl, u32 enable);
	s32 (*de_sharp_set_size)(struct de_sharp_handle *hdl, u32 width, u32 height);
	s32 (*de_sharp_set_window)(struct de_sharp_handle *hdl,
		      u32 x, u32 y, u32 w, u32 h);
};

struct de_sharp_handle *de_sharp_create(struct module_create_info *info)
{
	return de35x_sharp_create(info);
}

s32 de_sharp_enable(struct de_sharp_handle *hdl, u32 en)
{
	DRM_DEBUG_DRIVER("[SUNXI-DE] %s %d \n", __FUNCTION__, __LINE__);
	if (hdl->private->de_sharp_enable)
		return hdl->private->de_sharp_enable(hdl, en);
	else
		return 0;
}

s32 de_sharp_set_size(struct de_sharp_handle *hdl, u32 width,
		    u32 height)
{
	if (hdl->private->de_sharp_set_size)
		return hdl->private->de_sharp_set_size(hdl, width, height);
	else
		return 0;
}

s32 de_sharp_set_window(struct de_sharp_handle *hdl,
		      u32 x, u32 y, u32 w, u32 h)
{
	if (hdl->private->de_sharp_set_window)
		return hdl->private->de_sharp_set_window(hdl, x, y, w, h);
	else
		return 0;
}

static void sharp_set_block_dirty(
	struct de_sharp_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
	if (priv->reg_blks[blk_id].rcq_hd)
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
}

static void de_sharp_request_update(struct de_sharp_private *priv, u32 blk_id, u32 dirty)
{
	priv->shadow_blks[blk_id].dirty = dirty;
}

void de_sharp_update_regs(struct de_sharp_handle *hdl)
{
	u32 blk_id = 0;
	struct de_reg_block *shadow_block;
	struct de_reg_block *block;
	struct de_sharp_private *priv = hdl->private;

	mutex_lock(&priv->lock);
	for (blk_id = 0; blk_id < SHARP_REG_BLK_NUM; blk_id++) {
		shadow_block = &(priv->shadow_blks[blk_id]);
		block = &(priv->reg_blks[blk_id]);

		if (shadow_block->dirty) {
			memcpy(block->vir_addr, shadow_block->vir_addr, shadow_block->size);
			DRM_DEBUG_DRIVER("[SUNXI-DE] %s %d blk_id:%d\n", __FUNCTION__, __LINE__, blk_id);
			sharp_set_block_dirty(priv, blk_id, 1);
			shadow_block->dirty = 0;
		}
	}
	mutex_unlock(&priv->lock);
}

static inline struct sharp_reg *de35x_get_sharp_reg(struct de_sharp_private *priv)
{
	return (struct sharp_reg *)(priv->reg_blks[SHARP_PARA_REG_BLK].vir_addr);
}

static inline struct sharp_reg *de35x_get_sharp_shadow_reg(struct de_sharp_private *priv)
{
	return (struct sharp_reg *)(priv->shadow_blks[SHARP_PARA_REG_BLK].vir_addr);
}

s32 de_sharp_dump_state(struct drm_printer *p, struct de_sharp_handle *hdl)
{
	struct de_sharp_private *priv = hdl->private;
	struct sharp_reg *reg = de35x_get_sharp_shadow_reg(priv);
	unsigned long base = (unsigned long)hdl->private->reg_blks[0].reg_addr;
	unsigned long de_base = (unsigned long)hdl->cinfo.de_reg_base;

	drm_printf(p, "\n\tsharp@%8x: %sable\n", (unsigned int)(base - de_base),
			  reg->ctrl.bits.en ? "en" : "dis");
	return 0;
}

bool de_sharp_is_enabled(struct de_sharp_handle *hdl)
{
	struct de_sharp_private *priv = hdl->private;
	struct sharp_reg *reg = de35x_get_sharp_shadow_reg(priv);
	return !!reg->ctrl.bits.en;
}

s32 de35x_sharp_set_size(struct de_sharp_handle *hdl, u32 width,
		    u32 height)
{
	struct de_sharp_private *priv = hdl->private;
	struct sharp_reg *reg = de35x_get_sharp_shadow_reg(priv);

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

	de_sharp_request_update(priv, SHARP_PARA_REG_BLK, 1);
	mutex_unlock(&priv->lock);
	return 0;
}

s32 de_sharp_set_demo_mode(struct de_sharp_handle *hdl, bool enable)
{
	struct de_sharp_private *priv = hdl->private;
	struct sharp_reg *reg = de35x_get_sharp_shadow_reg(priv);

	mutex_lock(&priv->lock);
	reg->ctrl.bits.demo_en = enable ? 1 : 0;
	mutex_unlock(&priv->lock);
	de_sharp_request_update(priv, SHARP_PARA_REG_BLK, 1);
	return 0;
}

s32 de35x_sharp_set_window(struct de_sharp_handle *hdl,
		      u32 x, u32 y, u32 w, u32 h)
{
	struct de_sharp_private *priv = hdl->private;

	mutex_lock(&priv->lock);
	priv->demo_hor_start = x;
	priv->demo_hor_end = x + w;
	priv->demo_ver_start = y;
	priv->demo_ver_end = y + h - 1;
	mutex_unlock(&priv->lock);

	return 0;
}

static s32 de35x_sharp_init(struct de_sharp_handle *hdl)
{
	struct de_sharp_private *priv = hdl->private;
	struct sharp_reg *reg = de35x_get_sharp_shadow_reg(priv);

	mutex_lock(&priv->lock);
	/*reg->ctrl.dwval = 0;*/
	reg->strengths.dwval = 0x101010;

	reg->extrema.dwval = 0x5;
	reg->edge_adaptive.dwval = 0x60810;
	reg->coring.dwval = 0x20002;
	reg->detail0_weight.dwval = 0x28;
	reg->horz_smooth.dwval = 0x6;
	reg->gaussian_coefs0.dwval = 0x31d40;
	reg->gaussian_coefs1.dwval = 0x20a1e2c;
	reg->gaussian_coefs2.dwval = 0x40e1c24;
	reg->gaussian_coefs3.dwval = 0x911181c;

	de_sharp_request_update(priv, SHARP_PARA_REG_BLK, 1);
	mutex_unlock(&priv->lock);

	return 0;
}

static void de_sharp_restore_old_config_locked(struct de_sharp_handle *hdl)
{
	struct de_sharp_private *priv = hdl->private;
	int i;

	for (i = 0; i < SHARP_REG_BLK_NUM; i++) {
		de_sharp_request_update(priv, i, 1);
	}
}

s32 de35x_sharp_enable(struct de_sharp_handle *hdl, u32 en)
{
	struct de_sharp_private *priv = hdl->private;
	struct sharp_reg *reg = de35x_get_sharp_shadow_reg(priv);

	mutex_lock(&priv->lock);
	reg->ctrl.bits.en = !!en;
	if (en) {
		de_sharp_restore_old_config_locked(hdl);
	}
	de_sharp_request_update(priv, SHARP_PARA_REG_BLK, 1);
	DRM_DEBUG_DRIVER("%s %d\n", __func__, !!reg->ctrl.bits.en);
	mutex_unlock(&priv->lock);

	return 0;
}

#define SHARP_LEVEL_NUM 10
#define SHARP_CONFIG_NUM 3
/* not use currently */
s32 de_sharp_info2para(struct de_sharp_handle *hdl,
		     u32 fmt, struct sharp_config *para, u32 bypass)
{
	struct de_sharp_private *priv = hdl->private;
	struct sharp_reg *reg = de35x_get_sharp_shadow_reg(priv);
	static u8 LTI[SHARP_LEVEL_NUM][SHARP_CONFIG_NUM] = {
		{0,  0,  0,},
		{2,  2,  2,},
		{2,  4,  4,},
		{4,  4,  8,},
		{8,  8,  8,},
		{8,  16, 16,},
		{16, 16, 16,},
		{16, 32, 32,},
		{32, 32, 32,},
		{64, 64, 64,},
	};
	static u8 PEAK[SHARP_LEVEL_NUM][SHARP_CONFIG_NUM] = {
		{16, 16, 0,},
		{16, 16, 2,},
		{16, 16, 4,},
		{16, 16, 8,},
		{16, 16, 16,},
		{16, 16, 24,},
		{16, 16, 32,},
		{16, 16, 40,},
		{16, 16, 52,},
		{16, 16, 64,},
	};

	mutex_lock(&priv->lock);
	if (bypass || fmt == 1) {
		reg->ctrl.bits.en = 0;
		de_sharp_request_update(priv, SHARP_PARA_REG_BLK, 1);
		mutex_unlock(&priv->lock);
		return 0;
	}
	reg->ctrl.bits.en = 1;
	para->lti_level = para->lti_level % SHARP_LEVEL_NUM;
	para->peak_level = para->peak_level % SHARP_LEVEL_NUM;
	reg->strengths.bits.strength_bp0 = LTI[para->lti_level][0];
	reg->strengths.bits.strength_bp1 = LTI[para->lti_level][1];
	reg->strengths.bits.strength_bp2 = LTI[para->lti_level][2];
	reg->d0_boosting.bits.d0_pst_level = PEAK[para->peak_level][0];
	reg->d0_boosting.bits.d0_neg_level = PEAK[para->peak_level][1];
	reg->d0_boosting.bits.d0_gain = PEAK[para->peak_level][2];
	de_sharp_request_update(priv, SHARP_PARA_REG_BLK, 1);
	mutex_unlock(&priv->lock);
	return 0;
}

struct de_sharp_handle *de35x_sharp_create(struct module_create_info *info)
{
	struct de_sharp_handle *hdl;
	struct de_reg_block *block;
	struct de_reg_mem_info *reg_mem_info;
	struct de_reg_mem_info *reg_shadow;
	struct de_sharp_private *priv;
	u8 __iomem *reg_base;
	const struct de_sharp_desc *desc;
	u32 i;

	desc = get_sharp_desc(info);
	if (!desc)
		return NULL;

	hdl = kmalloc(sizeof(*hdl), GFP_KERNEL | __GFP_ZERO);
	hdl->private = kmalloc(sizeof(*hdl->private), GFP_KERNEL | __GFP_ZERO);
	memcpy(&hdl->cinfo, info, sizeof(*info));

	reg_base = info->de_reg_base + info->reg_offset + desc->reg_offset;
	priv = hdl->private;
	reg_mem_info = &(priv->reg_mem_info);

	reg_mem_info->size = sizeof(struct sharp_reg);
	reg_mem_info->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
		reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
		info->update_mode == RCQ_MODE);
	if (NULL == reg_mem_info->vir_addr) {
		DRM_ERROR("alloc bld[%d] mm fail!size=0x%x\n",
		     info->id, reg_mem_info->size);
		return ERR_PTR(-ENOMEM);
	}

	block = &(priv->reg_blks[SHARP_PARA_REG_BLK]);
	block->phy_addr = reg_mem_info->phy_addr;
	block->vir_addr = reg_mem_info->vir_addr;
	block->size = 0x50;
	block->reg_addr = reg_base;

	priv->reg_blk_num = SHARP_REG_BLK_NUM;

	hdl->block_num = priv->reg_blk_num;
	hdl->block = kmalloc(sizeof(block[0]) * hdl->block_num, GFP_KERNEL | __GFP_ZERO);
	for (i = 0; i < hdl->private->reg_blk_num; i++)
		hdl->block[i] = &priv->reg_blks[i];

	/* create shadow block */
	reg_shadow = &(priv->reg_shadow);

	reg_shadow->size = sizeof(struct sharp_reg);
	reg_shadow->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
		reg_shadow->size, (void *)&(reg_shadow->phy_addr),
		info->update_mode == RCQ_MODE);
	if (NULL == reg_shadow->vir_addr) {
		DRM_ERROR("alloc bld[%d] mm fail!size=0x%x\n",
		     info->id, reg_shadow->size);
		return ERR_PTR(-ENOMEM);
	}

	block = &(priv->shadow_blks[SHARP_PARA_REG_BLK]);
	block->phy_addr = reg_shadow->phy_addr;
	block->vir_addr = reg_shadow->vir_addr;
	block->size = 0x50;

	mutex_init(&priv->lock);

	de35x_sharp_init(hdl);

	priv->de_sharp_enable = de35x_sharp_enable;
	priv->de_sharp_set_size = de35x_sharp_set_size;
	priv->de_sharp_set_window = de35x_sharp_set_window;

	return hdl;
}

int de_sharp_pq_proc(struct de_sharp_handle *hdl, sharp_de35x_t *para)
{
	struct de_sharp_private *priv = hdl->private;
	struct sharp_reg *reg = de35x_get_sharp_shadow_reg(priv);

	if (para->cmd == PQ_READ) {
		para->value[0] = reg->ctrl.bits.en;
		para->value[1] = reg->ctrl.bits.demo_en;
		para->value[2] = reg->horz_smooth.bits.hsmooth_en;
		para->value[3] = reg->strengths.bits.strength_bp2;
		para->value[4] = reg->strengths.bits.strength_bp1;
		para->value[5] = reg->strengths.bits.strength_bp0;
		para->value[6] = reg->d0_boosting.bits.d0_gain;
		para->value[7] = reg->edge_adaptive.bits.edge_gain;
		para->value[8] = reg->edge_adaptive.bits.weak_edge_th;
		para->value[9] = reg->edge_adaptive.bits.edge_trans_width;
		para->value[10] = reg->edge_adaptive.bits.min_sharp_strength;
		para->value[11] = reg->overshoot_ctrl.bits.pst_up;
		para->value[12] = reg->overshoot_ctrl.bits.pst_shift;
		para->value[13] = reg->overshoot_ctrl.bits.neg_up;
		para->value[14] = reg->overshoot_ctrl.bits.neg_shift;
		para->value[15] = reg->d0_boosting.bits.d0_pst_level;
		para->value[16] = reg->d0_boosting.bits.d0_neg_level;
		para->value[17] = reg->coring.bits.zero;
		para->value[18] = reg->coring.bits.width;
		para->value[19] = reg->detail0_weight.bits.th_flat;
		para->value[20] = reg->detail0_weight.bits.fw_type;
		para->value[21] = reg->horz_smooth.bits.hsmooth_trans_width;
			/*para->value[22] = reg->demo_horz.bits.demo_horz_start;
		para->value[23] = reg->demo_horz.bits.demo_horz_end;
		para->value[24] = reg->demo_vert.bits.demo_vert_start;
		para->value[25] = reg->demo_vert.bits.demo_vert_end;*/
		para->value[22] = priv->demo_hor_start;
		para->value[23] = priv->demo_hor_end;
		para->value[24] = priv->demo_ver_start;
		para->value[25] = priv->demo_ver_end;
	} else {
		reg->ctrl.bits.en = para->value[0];
		reg->ctrl.bits.demo_en = para->value[1];
		reg->horz_smooth.bits.hsmooth_en = para->value[2];
		reg->strengths.bits.strength_bp2 = para->value[3];
		reg->strengths.bits.strength_bp1 = para->value[4];
		reg->strengths.bits.strength_bp0 = para->value[5];
		reg->d0_boosting.bits.d0_gain = para->value[6];
		reg->edge_adaptive.bits.edge_gain = para->value[7];
		reg->edge_adaptive.bits.weak_edge_th = para->value[8];
		reg->edge_adaptive.bits.edge_trans_width = para->value[9];
		reg->edge_adaptive.bits.min_sharp_strength = para->value[10];
		reg->overshoot_ctrl.bits.pst_up = para->value[11];
		reg->overshoot_ctrl.bits.pst_shift = para->value[12];
		reg->overshoot_ctrl.bits.neg_up = para->value[13];
		reg->overshoot_ctrl.bits.neg_shift = para->value[14];
		reg->d0_boosting.bits.d0_pst_level = para->value[15];
		reg->d0_boosting.bits.d0_neg_level = para->value[16];
		reg->coring.bits.zero = para->value[17];
		reg->coring.bits.width = para->value[18];
		reg->detail0_weight.bits.th_flat = para->value[19];
		reg->detail0_weight.bits.fw_type = para->value[20];
		reg->horz_smooth.bits.hsmooth_trans_width = para->value[21];
		/*reg->demo_horz.bits.demo_horz_start = para->value[22];
		reg->demo_horz.bits.demo_horz_end = para->value[23];
		reg->demo_vert.bits.demo_vert_start = para->value[24];
		reg->demo_vert.bits.demo_vert_end = para->value[25];*/
		priv->demo_hor_start = para->value[22];
		priv->demo_hor_end = para->value[23];
		priv->demo_ver_start = para->value[24];
		priv->demo_ver_end = para->value[25];
		reg->demo_horz.bits.demo_horz_start =
		    (reg->size.bits.width + 1) * priv->demo_hor_start / 100;
		reg->demo_horz.bits.demo_horz_end =
		    (reg->size.bits.width + 1) * priv->demo_hor_end / 100;
		reg->demo_vert.bits.demo_vert_start =
		    (reg->size.bits.height + 1) * priv->demo_ver_start / 100;
		reg->demo_vert.bits.demo_vert_end =
		    (reg->size.bits.height + 1) * priv->demo_ver_end / 100;
		de_sharp_request_update(priv, SHARP_PARA_REG_BLK, 1);
	}
	return 0;
}
