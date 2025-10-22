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
 *  All Winner Tech, All Right Reserved. 2014-2025 Copyright (c)
 *
 *  File name   :  display engine 35x deband basic function definition
 *
 *  History     :  2024/7/5 v0.1  Initial version
 *
 ******************************************************************************/
#include "de_deband_type.h"
#include "de_deband.h"
#include "de_deband_platform.h"

enum { DEBAND_PARA_REG_BLK = 0,
       DEBAND_REG_BLK_NUM,
};

struct de_deband_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	bool init;
	u8 demo_hor_start;
	u8 demo_hor_end;
	u8 demo_ver_start;
	u8 demo_ver_end;
	struct de_reg_block reg_blks[DEBAND_REG_BLK_NUM];
};

static inline struct deband_reg *get_deband_reg(struct de_deband_private *priv)
{
	return (struct deband_reg *)(priv->reg_blks[DEBAND_PARA_REG_BLK].vir_addr);
}

static void deband_set_block_dirty(struct de_deband_private *priv,
				      u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	}
}

s32 de_deband_set_size(struct de_deband_handle *hdl, u32 width, u32 height)
{
	struct de_deband_private *priv = hdl->private;
	struct deband_reg *reg = get_deband_reg(priv);

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

	deband_set_block_dirty(priv, DEBAND_PARA_REG_BLK, 1);

	return 0;
}

s32 de_deband_set_demo_mode(struct de_deband_handle *hdl, bool enable)
{
	struct de_deband_private *priv = hdl->private;
	struct deband_reg *reg = get_deband_reg(priv);
	reg->ctl.bits.demo_en = enable ? 1 : 0;
	deband_set_block_dirty(priv, DEBAND_PARA_REG_BLK, 1);
	return 0;
}

s32 de_deband_set_window(struct de_deband_handle *hdl, u32 x, u32 y, u32 w, u32 h)
{
	struct de_deband_private *priv = hdl->private;

	priv->demo_hor_start = x;
	priv->demo_hor_end = x + w;
	priv->demo_ver_start = y;
	priv->demo_ver_end = y + h;
	return 0;
}

static void de_deband_restore_old_config_locked(struct de_deband_handle *hdl)
{
	struct de_deband_private *priv = hdl->private;
	int i;
	for (i = 0; i < DEBAND_REG_BLK_NUM; i++) {
		deband_set_block_dirty(priv, i, 1);
	}
}

bool de_deband_is_enabled(struct de_deband_handle *hdl)
{
	struct de_deband_private *priv = hdl->private;
	struct deband_reg *reg = get_deband_reg(priv);
	return !!reg->ctl.bits.hdeband_en;
}

s32 de_deband_enable(struct de_deband_handle *hdl,  u32 en)
{
	struct de_deband_private *priv = hdl->private;
	struct deband_reg *reg = get_deband_reg(priv);

	if (en && priv->init) {
		reg->ctl.dwval = 0x1111;
		de_deband_restore_old_config_locked(hdl);
	} else
		reg->ctl.dwval = 0x0;
	deband_set_block_dirty(priv, DEBAND_PARA_REG_BLK, 1);

	return 0;
}

s32 de_deband_dump_state(struct drm_printer *p, struct de_deband_handle *hdl)
{
	struct de_deband_private *priv = hdl->private;
	struct deband_reg *reg = get_deband_reg(priv);
	unsigned long base = (unsigned long)hdl->private->reg_blks[0].reg_addr;
	unsigned long de_base = (unsigned long)hdl->cinfo.de_reg_base;

	drm_printf(p, "\n\tdeband@%8x: %sable\n", (unsigned int)(base - de_base),
			  reg->ctl.bits.hdeband_en ? "en" : "dis");
	return 0;
}

s32 de_deband_set_outinfo(struct de_deband_handle *hdl, enum de_color_space cs,
			  enum de_data_bits bits, enum de_format_space fmt)
{
	struct de_deband_private *priv = hdl->private;
	struct deband_reg *reg = get_deband_reg(priv);

	if (bits == DE_DATA_10BITS)
		reg->output_bits.bits.output_bits = 10;
	else if (bits == DE_DATA_8BITS)
		reg->output_bits.bits.output_bits = 8;

	if (cs == DE_COLOR_SPACE_BT601)
		reg->cs.bits.input_color_space = 0;
	else if (cs == DE_COLOR_SPACE_BT2020NC || cs == DE_COLOR_SPACE_BT2020C)
		reg->cs.bits.input_color_space = 2;
	else if (fmt == DE_FORMAT_SPACE_YUV)
		reg->cs.bits.input_color_space = 1;

	deband_set_block_dirty(priv, DEBAND_PARA_REG_BLK, 1);
	return 0;
}

int de_deband_pq_proc(struct de_deband_handle *hdl, deband_module_param_t *para)
{
	struct de_deband_private *priv = hdl->private;
	struct deband_reg *reg = get_deband_reg(priv);

	if (para->cmd == PQ_READ) {
		para->value[0] = reg->ctl.bits.hdeband_en;
		para->value[1] = reg->ctl.bits.c_hdeband_en;
		para->value[2] = reg->ctl.bits.vdeband_en;
		para->value[3] = reg->ctl.bits.c_vdeband_en;
		para->value[4] = reg->ctl.bits.demo_en;
		para->value[5] = reg->rand_dither.bits.rand_dither_en;
		para->value[6] = reg->cs.bits.input_color_space;
		para->value[7] = reg->output_bits.bits.output_bits;
		para->value[8] = reg->step.bits.step_th;
		para->value[9] = reg->edge.bits.edge_th;
		para->value[10] = reg->vmin_steps.bits.vmin_steps;
		para->value[11] = reg->vmin_steps.bits.vmin_ratio;
		para->value[12] = reg->vfilt_para1.bits.adp_w_up;
		para->value[13] = reg->hmin_steps.bits.hmin_steps;
		para->value[14] = reg->hmin_steps.bits.hmin_ratio;
		para->value[15] = reg->hfilt_para0.bits.kmax;
		para->value[16] = reg->hfilt_para0.bits.hsigma;
		para->value[17] = reg->rand_dither.bits.rand_num_bits;
		/*para->value[18] = reg->demo_horz.bits.demo_horz_start;
		para->value[19] = reg->demo_horz.bits.demo_horz_end;
		para->value[20] = reg->demo_vert.bits.demo_vert_start;
		para->value[21] = reg->demo_vert.bits.demo_vert_end;*/
		para->value[18] = priv->demo_hor_start;
		para->value[19] = priv->demo_hor_end;
		para->value[20] = priv->demo_ver_start;
		para->value[21] = priv->demo_ver_end;
	} else {
		reg->ctl.bits.hdeband_en = para->value[0];
		reg->ctl.bits.c_hdeband_en = para->value[1];
		reg->ctl.bits.vdeband_en = para->value[2];
		reg->ctl.bits.c_vdeband_en = para->value[3];
		reg->ctl.bits.demo_en = para->value[4];
		reg->rand_dither.bits.rand_dither_en = para->value[5];
		reg->cs.bits.input_color_space = para->value[6];
		reg->output_bits.bits.output_bits = para->value[7];
		reg->step.bits.step_th = para->value[8];
		reg->edge.bits.edge_th = para->value[9];
		reg->vmin_steps.bits.vmin_steps = para->value[10];
		reg->vmin_steps.bits.vmin_ratio = para->value[11];
		reg->vfilt_para1.bits.adp_w_up = para->value[12];
		reg->hmin_steps.bits.hmin_steps = para->value[13];
		reg->hmin_steps.bits.hmin_ratio = para->value[14];
		reg->hfilt_para0.bits.kmax = para->value[15];
		reg->hfilt_para0.bits.hsigma = para->value[16];
		reg->rand_dither.bits.rand_num_bits = para->value[17];
		/*reg->demo_horz.bits.demo_horz_start = para->value[18];
		reg->demo_horz.bits.demo_horz_end = para->value[19];
		reg->demo_vert.bits.demo_vert_start = para->value[20];
		reg->demo_vert.bits.demo_vert_end = para->value[21];*/
		priv->demo_hor_start = para->value[18];
		priv->demo_hor_end = para->value[19];
		priv->demo_ver_start = para->value[20];
		priv->demo_ver_end = para->value[21];
		reg->demo_horz.bits.demo_horz_start =
		    (reg->size.bits.width + 1) * priv->demo_hor_start / 100;
		reg->demo_horz.bits.demo_horz_end =
		    (reg->size.bits.width + 1)* priv->demo_hor_end / 100;
		reg->demo_vert.bits.demo_vert_start =
		    (reg->size.bits.height + 1) * priv->demo_ver_start / 100;
		reg->demo_vert.bits.demo_vert_end =
		    (reg->size.bits.height + 1) * priv->demo_ver_end / 100;
		priv->init = true;
		deband_set_block_dirty(priv, DEBAND_PARA_REG_BLK, 1);
	}
	return 0;
}

struct de_deband_handle *de_deband_create(struct module_create_info *info)
{
	struct de_deband_handle *hdl;
	struct de_reg_block *block;
	struct de_reg_mem_info *reg_mem_info;
	struct de_deband_private *priv;
	u8 __iomem *reg_base;
	const struct de_deband_desc *desc;
	int i;

	desc = get_deband_desc(info);
	if (!desc)
		return NULL;

	hdl = kmalloc(sizeof(*hdl), GFP_KERNEL | __GFP_ZERO);
	hdl->private = kmalloc(sizeof(*hdl->private), GFP_KERNEL | __GFP_ZERO);
	memcpy(&hdl->cinfo, info, sizeof(*info));

	reg_base = info->de_reg_base + info->reg_offset + desc->reg_offset;
	priv = hdl->private;
	reg_mem_info = &(priv->reg_mem_info);

	reg_mem_info->size = sizeof(struct deband_reg);
	reg_mem_info->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
		reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
		info->update_mode == RCQ_MODE);
	if (NULL == reg_mem_info->vir_addr) {
		DRM_ERROR("alloc bld[%d] mm fail!size=0x%x\n",
		     info->id, reg_mem_info->size);
		return ERR_PTR(-ENOMEM);
	}

	block = &(priv->reg_blks[DEBAND_PARA_REG_BLK]);
	block->phy_addr = reg_mem_info->phy_addr;
	block->vir_addr = reg_mem_info->vir_addr;
	block->size = sizeof(struct deband_reg);
	block->reg_addr = reg_base;

	priv->reg_blk_num = DEBAND_REG_BLK_NUM;

	hdl->block_num = priv->reg_blk_num;
	hdl->block = kmalloc(sizeof(block[0]) * hdl->block_num, GFP_KERNEL | __GFP_ZERO);
	for (i = 0; i < hdl->private->reg_blk_num; i++)
		hdl->block[i] = &priv->reg_blks[i];
	return hdl;
}
