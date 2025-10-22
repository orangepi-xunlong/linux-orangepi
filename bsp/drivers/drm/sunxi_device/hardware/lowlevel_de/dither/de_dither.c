/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "de_dither_type.h"
#include "de_dither_platform.h"
#include "de_dither.h"

#define DISP_DITHER_OFFSET	(0x8000)

enum {
	DITHER_PARA_REG_BLK = 0,
	DITHER_REG_BLK_NUM,
};

struct de_dither_private {
	struct de_reg_mem_info reg_mem_info;
	const struct de_dither_dsc *dsc;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[DITHER_REG_BLK_NUM];
};

static inline struct dither_reg *get_dither_reg(struct de_dither_private *priv)
{
	return (struct dither_reg *)(priv->reg_blks[DITHER_PARA_REG_BLK].vir_addr);
}

static void dither_set_block_dirty(
	struct de_dither_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
	if (priv->reg_blks[blk_id].rcq_hd)
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
}

s32 de_dither_set_size(struct de_dither_handle *hdl, u32 width, u32 height)
{
	struct de_dither_private *priv = hdl->private;
	struct dither_reg *reg = get_dither_reg(priv);

	reg->size.bits.width = width - 1;
	reg->size.bits.height = height - 1;
	dither_set_block_dirty(priv, DITHER_PARA_REG_BLK, 1);

	return 0;
}

int de_dither_config(struct de_dither_handle *hdl, struct dither_config *cfg)
{
	struct de_dither_private *priv = hdl->private;
	struct dither_reg *reg = get_dither_reg(priv);
	reg->ctl.bits.en = cfg->enable ? 1 : 0;
	reg->ctl.bits.dither_out_fmt = cfg->out_fmt;
	reg->ctl.bits.dither_mode = cfg->mode;
	reg->ctl.bits._3d_fifo_out = cfg->enable_3d_fifo ? 1 : 0;
	reg->size.bits.width = cfg->w;
	reg->size.bits.height = cfg->h;
	dither_set_block_dirty(priv, DITHER_PARA_REG_BLK, 1);
	return 0;
}

s32 de_dither_enable(struct de_dither_handle *hdl, u32 en)
{
	struct de_dither_private *priv = hdl->private;
	struct dither_reg *reg = get_dither_reg(priv);

	reg->ctl.dwval = en ? 1 : 0;
	dither_set_block_dirty(priv, DITHER_PARA_REG_BLK, 1);

	return 0;
}

void de_dither_dump_state(struct drm_printer *p, struct de_dither_handle *hdl)
{
	unsigned long base = (unsigned long)hdl->private->reg_blks[0].reg_addr;
	unsigned long de_base = (unsigned long)hdl->cinfo.de_reg_base;
	struct dither_reg *reg = get_dither_reg(hdl->private);

	drm_printf(p, "\tdither@%8x: %sable\n", (unsigned int)(base - de_base),
		   reg->ctl.dwval & 0x1 ? "en" : "dis");
}

struct de_dither_handle *de_dither_create(struct module_create_info *info)
{
	struct de_dither_handle *hdl;
	struct de_reg_block *reg_blk;
	struct de_reg_mem_info *reg_mem_info;
	u8 __iomem *reg_base;
	int i;
	struct de_dither_private *priv;
	const struct de_dither_dsc *dsc;

	dsc = get_dither_dsc(info);
	if (!dsc)
		return NULL;

	reg_base = (u8 __iomem *)(info->de_reg_base + info->reg_offset + DISP_DITHER_OFFSET);
	hdl = kmalloc(sizeof(*hdl), GFP_KERNEL | __GFP_ZERO);
	memcpy(&hdl->cinfo, info, sizeof(*info));
	hdl->private = kmalloc(sizeof(*hdl->private), GFP_KERNEL | __GFP_ZERO);
	hdl->support_fmts = dsc->support_fmts;
	hdl->fmt_cnt = dsc->fmt_cnt;
	hdl->support_modes = dsc->support_modes;
	hdl->mode_cnt = dsc->mode_cnt;
	hdl->support_3d_fifo = dsc->support_3d_fifo;
	priv = hdl->private;
	hdl->private->dsc = dsc;
	reg_mem_info = &(hdl->private->reg_mem_info);
	reg_mem_info->size = sizeof(struct dither_reg);
	reg_mem_info->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
		reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
		info->update_mode == RCQ_MODE);
	if (reg_mem_info->vir_addr == NULL) {
		DRM_ERROR("alloc dither[%d] mm fail!size=0x%x\n",
			 dsc->id, reg_mem_info->size);
		return ERR_PTR(-ENOMEM);
	}

	reg_blk = &(priv->reg_blks[DITHER_PARA_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr;
	reg_blk->vir_addr = reg_mem_info->vir_addr;
	reg_blk->size = 0x8;
	reg_blk->reg_addr = (u8 __iomem *)reg_base;

	priv->reg_blk_num = DITHER_REG_BLK_NUM;
	hdl->block_num = priv->reg_blk_num;
	hdl->block = kmalloc(sizeof(reg_blk[0]) * hdl->block_num, GFP_KERNEL | __GFP_ZERO);
	for (i = 0; i < hdl->private->reg_blk_num; i++)
		hdl->block[i] = &priv->reg_blks[i];

	return hdl;
}
