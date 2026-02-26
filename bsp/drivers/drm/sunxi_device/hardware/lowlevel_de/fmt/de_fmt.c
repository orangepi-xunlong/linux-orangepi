/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2023 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include "de_fmt.h"
#include "de_fmt_type.h"
#include "de_fmt_platform.h"

enum {
	FMT_FMT_REG_BLK = 0,
	FMT_REG_BLK_NUM,
};

struct fmt_debug_info {
	bool enable;
	struct de_fmt_info info;
};

struct de_fmt_private {
	struct de_reg_mem_info reg_mem_info;
	const struct de_fmt_desc *dsc;
	struct fmt_debug_info debug;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[FMT_REG_BLK_NUM];
};

static inline struct fmt_reg *get_fmt_reg(struct de_fmt_private *priv)
{
	return (struct fmt_reg *)(priv->reg_blks[0].vir_addr);
}

static void fmt_set_block_dirty(
	struct de_fmt_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
	if (priv->reg_blks[blk_id].rcq_hd)
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
}

void de_fmt_dump_state(struct drm_printer *p, struct de_fmt_handle *hdl)
{
	struct fmt_debug_info *debug = &hdl->private->debug;
	unsigned long base = (unsigned long)hdl->private->reg_blks[0].reg_addr;
	unsigned long de_base = (unsigned long)hdl->cinfo.de_reg_base;

	drm_printf(p, "\tfmt@%8x: %sable\n", (unsigned int)(base - de_base),
		   debug->enable ? "en" : "dis");
	if (debug->enable) {
		drm_printf(p, "\t\twxh: %dx%d colorfmt: %d yuv_sampling: %d bits: %d\n",
			   debug->info.width, debug->info.height,
			   debug->info.px_fmt_space,
			   debug->info.yuv_sampling,
			   debug->info.bits);
	}
}

s32 de_fmt_apply(struct de_fmt_handle *hdl, const struct de_fmt_info *out_info)
{
	struct de_fmt_private *priv = hdl->private;
	struct fmt_reg *reg = get_fmt_reg(priv);
	u32 dwval;

	if (out_info->px_fmt_space != DE_FORMAT_SPACE_YUV) {
		reg->ctl.dwval = 0;
		fmt_set_block_dirty(priv, FMT_FMT_REG_BLK, 1);
		priv->debug.enable = false;
		return 0;
	}

	reg->ctl.dwval = 1;
	priv->debug.enable = true;
	memcpy(&priv->debug.info, out_info, sizeof(priv->debug.info));

	dwval = (out_info->width ?
		((out_info->width - 1) & 0x1FFF) : 0)
		| (out_info->height ?
		(((out_info->height - 1) & 0x1FFF) << 16) : 0);
	reg->size.dwval = dwval;

	reg->swap.dwval = 0;

	reg->bitdepth.dwval = (out_info->bits == DE_DATA_8BITS) ? 0 : 1;

	dwval = 0;
	if (out_info->px_fmt_space == DE_FORMAT_SPACE_YUV) {
		if (out_info->yuv_sampling == DE_YUV422)
			dwval = 1;
		else if (out_info->yuv_sampling == DE_YUV420)
			dwval = 2;
	}
	reg->fmt_type.dwval = dwval;

	reg->coeff.dwval = 0;

	if (out_info->px_fmt_space == DE_FORMAT_SPACE_RGB ||
	    (out_info->px_fmt_space == DE_FORMAT_SPACE_YUV &&
	     out_info->yuv_sampling == DE_YUV444)) {
		reg->limit_y.dwval = 0x0fff0000;
		reg->limit_c0.dwval = 0x0fff0000;
		reg->limit_c1.dwval = 0x0fff0000;
	} else {
		reg->limit_y.dwval = 0xeb00100;
		reg->limit_c0.dwval = 0xf000100;
		reg->limit_c1.dwval = 0xf000100;
	}

	fmt_set_block_dirty(priv, FMT_FMT_REG_BLK, 1);

	return 0;
}

struct de_fmt_handle *de_fmt_create(struct module_create_info *info)
{
	int i;
	struct de_fmt_handle *hdl;
	struct de_reg_block *block;
	struct de_reg_mem_info *reg_mem_info;
	struct de_fmt_private *priv;
	u8 __iomem *reg_base;
	const struct de_fmt_desc *dsc;

	dsc = get_fmt_dsc(info);
	if (!dsc)
		return NULL;
	hdl = kmalloc(sizeof(*hdl), GFP_KERNEL | __GFP_ZERO);
	hdl->private = kmalloc(sizeof(*hdl->private), GFP_KERNEL | __GFP_ZERO);
	hdl->disp_reg_base = dsc->disp_base;
	memcpy(&hdl->cinfo, info, sizeof(*info));

	hdl->private->dsc = dsc;
	reg_base = info->de_reg_base + dsc->disp_base + dsc->fmt_offset;
	priv = hdl->private;
	reg_mem_info = &priv->reg_mem_info;

	reg_mem_info->size = sizeof(struct fmt_reg);
	reg_mem_info->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
		reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
		info->update_mode == RCQ_MODE);
	if (NULL == reg_mem_info->vir_addr) {
		DRM_ERROR("alloc fmt[%d] mm fail!size=0x%x\n",
			  info->id, reg_mem_info->size);
		return ERR_PTR(-ENOMEM);
	}

	block = &(priv->reg_blks[FMT_FMT_REG_BLK]);
	block->phy_addr = reg_mem_info->phy_addr;
	block->vir_addr = reg_mem_info->vir_addr;
	block->size = reg_mem_info->size;
	block->reg_addr = reg_base;

	priv->reg_blk_num = FMT_REG_BLK_NUM;

	hdl->block_num = priv->reg_blk_num;
	hdl->block = kmalloc_array(hdl->block_num, sizeof(block[0]), GFP_KERNEL | __GFP_ZERO);
	for (i = 0; i < hdl->private->reg_blk_num; i++)
		hdl->block[i] = &priv->reg_blks[i];

	return hdl;
}
