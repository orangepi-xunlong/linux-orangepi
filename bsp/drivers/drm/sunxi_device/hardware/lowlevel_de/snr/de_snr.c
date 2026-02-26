/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * de_snr.c
 *
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <drm/drm_framebuffer.h>
#include <linux/mutex.h>
#include <drm/drm_fourcc.h>
#include "de_snr_type.h"
#include "de_snr.h"

struct de_snr_handle *de35x_snr_create(struct module_create_info *info);

enum {
	SNR_REG_BLK_CTL = 0,
	SNR_REG_BLK_NUM,
};

struct de_snr_private {
	struct de_reg_mem_info reg_mem_info;
	struct de_reg_mem_info reg_shadow;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[SNR_REG_BLK_NUM];
	struct de_reg_block shadow_blks[SNR_REG_BLK_NUM];
	struct mutex lock;
	bool init;
	u8 demo_hor_start;
	u8 demo_hor_end;
	u8 demo_ver_start;
	u8 demo_ver_end;

	s32 (*de_snr_enable)(struct de_snr_handle *hdl, u32 snr_enable);
	s32 (*de_snr_set_para)(struct de_snr_handle *hdl, struct display_channel_state *state, struct de_snr_para *snr_para);
};

struct de_snr_handle *de_snr_create(struct module_create_info *info)
{
	return de35x_snr_create(info);
}

s32 de_snr_enable(struct de_snr_handle *hdl, u32 snr_enable)
{
	DRM_DEBUG_DRIVER("[SUNXI-DE] %s %d \n", __FUNCTION__, __LINE__);
	if (hdl->private->de_snr_enable)
		return hdl->private->de_snr_enable(hdl, snr_enable);
	else
		return 0;
}

s32 de_snr_set_para(struct de_snr_handle *hdl, struct display_channel_state *state, struct de_snr_para *snr_para)
{
	if (hdl->private->de_snr_set_para)
		return hdl->private->de_snr_set_para(hdl, state, snr_para);
	else
		return 0;
}

static void snr_set_block_dirty(
	struct de_snr_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
	if (priv->reg_blks[blk_id].rcq_hd)
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
}

static void de_snr_request_update(struct de_snr_private *priv, u32 blk_id, u32 dirty)
{
	priv->shadow_blks[blk_id].dirty = dirty;
}

void de_snr_update_regs(struct de_snr_handle *hdl)
{
	u32 blk_id = 0;
	struct de_reg_block *shadow_block;
	struct de_reg_block *block;
	struct de_snr_private *priv = hdl->private;

	mutex_lock(&priv->lock);
	for (blk_id = 0; blk_id < SNR_REG_BLK_NUM; blk_id++) {
		shadow_block = &(priv->shadow_blks[blk_id]);
		block = &(priv->reg_blks[blk_id]);

		if (shadow_block->dirty) {
			memcpy(block->vir_addr, shadow_block->vir_addr, shadow_block->size);
			DRM_DEBUG_DRIVER("[SUNXI-DE] %s %d blk_id:%d\n", __FUNCTION__, __LINE__, blk_id);
			snr_set_block_dirty(priv, blk_id, 1);
			shadow_block->dirty = 0;
		}
	}
	mutex_unlock(&priv->lock);
}

static inline struct snr_reg *de35x_get_snr_reg(struct de_snr_private *priv)
{
	return (struct snr_reg *)(priv->reg_blks[0].vir_addr);
}

static inline struct snr_reg *de35x_get_snr_shadow_reg(struct de_snr_private *priv)
{
	return (struct snr_reg *)(priv->shadow_blks[0].vir_addr);
}

s32 de_snr_dump_state(struct drm_printer *p, struct de_snr_handle *hdl)
{
	struct de_snr_private *priv = hdl->private;
	struct snr_reg *reg = de35x_get_snr_shadow_reg(priv);
	unsigned long base = (unsigned long)hdl->private->reg_blks[0].reg_addr;
	unsigned long de_base = (unsigned long)hdl->cinfo.de_reg_base;

	drm_printf(p, "\n\tsnr@%8x: %sable\n", (unsigned int)(base - de_base),
			  reg->snr_ctrl.bits.en ? "en" : "dis");
	return 0;
}

static void de_snr_restore_old_config_locked(struct de_snr_handle *hdl)
{
	struct de_snr_private *priv = hdl->private;
	struct snr_reg *reg = de35x_get_snr_shadow_reg(priv);
	int i;

	/* can not enable snr if not init */
	reg->snr_ctrl.bits.en = reg->snr_ctrl.bits.en && priv->init;
	for (i = 0; i < SNR_REG_BLK_NUM; i++) {
		de_snr_request_update(priv, i, 1);
	}
}

bool de_snr_is_enabled(struct de_snr_handle *hdl)
{
	struct de_snr_private *priv = hdl->private;
	struct snr_reg *reg = de35x_get_snr_shadow_reg(priv);
	return !!reg->snr_ctrl.bits.en;
}

s32 de35x_snr_enable(struct de_snr_handle *hdl, u32 snr_enable)
{
	struct de_snr_private *priv = hdl->private;
	struct snr_reg *reg = de35x_get_snr_shadow_reg(priv);

	mutex_lock(&priv->lock);
	if (!snr_enable)
		reg->snr_ctrl.bits.en = 0;
	else
		reg->snr_ctrl.bits.en = 1;
	de_snr_request_update(priv, SNR_REG_BLK_CTL, 1);

	if (snr_enable) {
		de_snr_restore_old_config_locked(hdl);
	}

	DRM_DEBUG_DRIVER("%s %d\n", __func__, !!reg->snr_ctrl.bits.en);
	mutex_unlock(&priv->lock);
	return 0;
}

s32 de_snr_set_demo_mode(struct de_snr_handle *hdl, bool enable)
{
	struct de_snr_private *priv = hdl->private;
	struct snr_reg *reg = de35x_get_snr_shadow_reg(priv);

	mutex_lock(&priv->lock);
	reg->snr_ctrl.bits.demo_en = enable ? 1 : 0;
	de_snr_request_update(priv, SNR_REG_BLK_CTL, 1);

	mutex_unlock(&priv->lock);
	return 0;
}

s32 de_snr_set_window(struct de_snr_handle *hdl,
		      u32 x, u32 y, u32 w, u32 h)
{
	struct de_snr_private *priv = hdl->private;

	mutex_lock(&priv->lock);

	priv->demo_hor_start = x;
	priv->demo_hor_end = x + w;
	priv->demo_ver_start = y;
	priv->demo_ver_end = y + h;

	mutex_unlock(&priv->lock);
	return 0;
}

s32 de_snr_set_size(struct de_snr_handle *hdl, u32 width, u32 height)
{
	struct de_snr_private *priv = hdl->private;
	struct snr_reg *reg = de35x_get_snr_shadow_reg(priv);

	mutex_lock(&priv->lock);
	reg->snr_size.bits.width  = width - 1;
	reg->snr_size.bits.height = height - 1;

	reg->demo_win_hor.bits.demo_horz_start =
	    width * priv->demo_hor_start / 100;
	reg->demo_win_hor.bits.demo_horz_end =
	    width * priv->demo_hor_end / 100;
	reg->demo_win_ver.bits.demo_vert_start =
	    height * priv->demo_ver_start / 100;
	reg->demo_win_ver.bits.demo_vert_end =
	    height * priv->demo_ver_end / 100;

	de_snr_request_update(priv, SNR_REG_BLK_CTL, 1);
	mutex_unlock(&priv->lock);
	return 0;
}

static int de_snr_pq_proc(struct de_snr_handle *hdl, snr_module_param_t *para)
{
	struct de_snr_private *priv = hdl->private;
	struct snr_reg *reg = de35x_get_snr_shadow_reg(priv);

	if (para->cmd == PQ_READ) {
		para->value[0] = reg->snr_ctrl.bits.en;
		para->value[1] = reg->snr_ctrl.bits.demo_en;
		para->value[2] = reg->snr_strength.bits.strength_y;
		para->value[3] = reg->snr_strength.bits.strength_u;
		para->value[4] = reg->snr_strength.bits.strength_v;
		para->value[5] = reg->snr_line_det.bits.th_ver_line;
		para->value[6] = reg->snr_line_det.bits.th_hor_line;
		/*para->value[7] = reg->demo_win_hor.bits.demo_horz_start;
		para->value[8] = reg->demo_win_hor.bits.demo_horz_end;
		para->value[9] = reg->demo_win_ver.bits.demo_vert_start;
		para->value[10] = reg->demo_win_ver.bits.demo_vert_end;*/
		para->value[7] = priv->demo_hor_start;
		para->value[8] = priv->demo_hor_end;
		para->value[9] = priv->demo_ver_start;
		para->value[10] = priv->demo_ver_end;
	} else {
		/* shoulde not open snr when rgb fmt, snr only can handle yuv input */
		reg->snr_ctrl.bits.en = para->value[0];
		reg->snr_ctrl.bits.demo_en = para->value[1];
		reg->snr_strength.bits.strength_y = para->value[2];
		reg->snr_strength.bits.strength_u = para->value[3];
		reg->snr_strength.bits.strength_v = para->value[4];
		reg->snr_line_det.bits.th_ver_line = para->value[5];
		reg->snr_line_det.bits.th_hor_line = para->value[6];
		/*reg->demo_win_hor.bits.demo_horz_start = para->value[7];
		reg->demo_win_hor.bits.demo_horz_end = para->value[8];
		reg->demo_win_ver.bits.demo_vert_start = para->value[9];
		reg->demo_win_ver.bits.demo_vert_end = para->value[10];*/
		priv->demo_hor_start = para->value[7];
		priv->demo_hor_end = para->value[8];
		priv->demo_ver_start = para->value[9];
		priv->demo_ver_end = para->value[10];
		reg->demo_win_hor.bits.demo_horz_start =
		    reg->snr_size.bits.width * priv->demo_hor_start / 100;
		reg->demo_win_hor.bits.demo_horz_end =
		    reg->snr_size.bits.width * priv->demo_hor_end / 100;
		reg->demo_win_ver.bits.demo_vert_start =
		    reg->snr_size.bits.height * priv->demo_ver_start / 100;
		reg->demo_win_ver.bits.demo_vert_end =
		    reg->snr_size.bits.height * priv->demo_ver_end / 100;
		priv->init = true;
		de_snr_request_update(priv, SNR_REG_BLK_CTL, 1);
	}
	return 0;
}

s32 de35x_snr_set_para(struct de_snr_handle *hdl, struct display_channel_state *state, struct de_snr_para *snr_para)
{
	struct de_snr_private *priv = hdl->private;
	struct snr_reg *reg = de35x_get_snr_shadow_reg(priv);
	u32 i = 0;
	u32 width, height;
	struct de_snr_commit_para *para = &snr_para->commit;
	struct drm_framebuffer *fb;
	int fmt;

	fb = state->base.fb;
	width = state->base.src_w >> 16;
	height = state->base.src_h >> 16;

	if (snr_para->dirty & PQD_DIRTY_MASK) {
		return de_snr_pq_proc(hdl, &snr_para->pqd);
	}

	if (!fb) {
		reg->snr_ctrl.bits.en = 0;
		DRM_DEBUG_DRIVER("[SUNXI-SNR] channel:%d %s %d \n", i, __FUNCTION__, __LINE__);
		//reg->snr_ctrl.bits.demo_en = 0;
		goto DIRTY;
	}

	fmt = drm_to_de_format(fb->format->format);

	if (fmt != DE_FORMAT_YUV422_SP_UVUV &&
	    fmt != DE_FORMAT_YUV422_SP_VUVU &&
	    fmt != DE_FORMAT_YUV420_SP_UVUV &&
	    fmt != DE_FORMAT_YUV420_SP_VUVU &&
	    fmt != DE_FORMAT_YUV422_P &&
	    fmt != DE_FORMAT_YVU422_P &&
	    fmt != DE_FORMAT_YUV420_P &&
	    fmt != DE_FORMAT_YVU420_P &&
	    fmt != DE_FORMAT_YUV422_SP_UVUV_10BIT &&
	    fmt != DE_FORMAT_YUV422_SP_VUVU_10BIT &&
	    fmt != DE_FORMAT_YUV420_SP_UVUV_10BIT &&
	    fmt != DE_FORMAT_YUV420_SP_VUVU_10BIT) {
		reg->snr_ctrl.bits.en = 0;
		//reg->snr_ctrl.bits.demo_en = 0;
		goto DIRTY;
	}

	if (para->b_trd_out) {
		uint32_t buffer_flags = para->flags;
		long long w = width;
		long long h = height;

		switch (buffer_flags) {
		// When 3D output enable, the video channel's output height
		// is twice of the input layer crop height.
		case DISP_BF_STEREO_TB:
		case DISP_BF_STEREO_SSH:
			h = h * 2;
			break;
		}
		reg->snr_size.bits.width  = w - 1;
		reg->snr_size.bits.height = h - 1;
	} else {
		reg->snr_size.bits.width =  width - 1;
		reg->snr_size.bits.height = height - 1;
	}

	reg->snr_ctrl.bits.en = para->enable;
	reg->snr_ctrl.bits.demo_en = para->demo_en;
/*	reg->demo_win_hor.bits.demo_horz_start = para->demo_x;
	reg->demo_win_hor.bits.demo_horz_end = para.demo_x + para.demo_width;
	reg->demo_win_ver.bits.demo_vert_start = para.demo_y;
	reg->demo_win_ver.bits.demo_vert_end = para.demo_y + para.demo_height;*/
	reg->snr_strength.bits.strength_y = para->y_strength;
	reg->snr_strength.bits.strength_u = para->u_strength;
	reg->snr_strength.bits.strength_v = para->v_strength;
	reg->snr_line_det.bits.th_hor_line = (para->th_hor_line) ? para->th_hor_line : 0xa;
	reg->snr_line_det.bits.th_ver_line = (para->th_ver_line) ? para->th_hor_line : 0xa;
	priv->init = true;

DIRTY:
	de_snr_request_update(priv, SNR_REG_BLK_CTL, 1);
	mutex_unlock(&priv->lock);

	return 0;

}

struct de_snr_handle *de35x_snr_create(struct module_create_info *info)
{
	int i;
	struct de_snr_handle *hdl;
	struct de_reg_block *block;
	struct de_reg_mem_info *reg_mem_info;
	struct de_reg_mem_info *reg_shadow;
	struct de_snr_private *priv;
	u8 __iomem *reg_base;
	const struct de_snr_desc *desc;

	desc = get_snr_desc(info);
	if (!desc)
		return NULL;

	hdl = kmalloc(sizeof(*hdl), GFP_KERNEL | __GFP_ZERO);
	hdl->private = kmalloc(sizeof(*hdl->private), GFP_KERNEL | __GFP_ZERO);
	memcpy(&hdl->cinfo, info, sizeof(*info));

	reg_base = info->de_reg_base + info->reg_offset + desc->reg_offset;
	priv = hdl->private;
	reg_mem_info = &(priv->reg_mem_info);

	reg_mem_info->size = sizeof(struct snr_reg);
	reg_mem_info->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
		reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
		info->update_mode == RCQ_MODE);
	if (NULL == reg_mem_info->vir_addr) {
		DRM_ERROR("alloc snr[%d] mm fail!size=0x%x\n",
		     info->id, reg_mem_info->size);
		return ERR_PTR(-ENOMEM);
	}


	block = &(priv->reg_blks[SNR_REG_BLK_CTL]);
	block->phy_addr = reg_mem_info->phy_addr;
	block->vir_addr = reg_mem_info->vir_addr;
	block->size = sizeof(struct snr_reg);
	block->reg_addr = reg_base;

	priv->reg_blk_num = SNR_REG_BLK_NUM;

	hdl->block_num = priv->reg_blk_num;
	hdl->block = kmalloc(sizeof(block[0]) * hdl->block_num, GFP_KERNEL | __GFP_ZERO);
	for (i = 0; i < hdl->private->reg_blk_num; i++)
		hdl->block[i] = &priv->reg_blks[i];


	/* create shadow block */
	reg_shadow = &(priv->reg_shadow);

	reg_shadow->size = sizeof(struct snr_reg);
	reg_shadow->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
		reg_shadow->size, (void *)&(reg_shadow->phy_addr),
		info->update_mode == RCQ_MODE);
	if (NULL == reg_shadow->vir_addr) {
		DRM_ERROR("alloc snr[%d] mm fail!size=0x%x\n",
		     info->id, reg_shadow->size);
		return ERR_PTR(-ENOMEM);
	}


	block = &(priv->shadow_blks[SNR_REG_BLK_CTL]);
	block->phy_addr = reg_shadow->phy_addr;
	block->vir_addr = reg_shadow->vir_addr;
	block->size = sizeof(struct snr_reg);

	mutex_init(&priv->lock);

	hdl->private->de_snr_enable = de35x_snr_enable;
	hdl->private->de_snr_set_para = de35x_snr_set_para;

	return hdl;
}
