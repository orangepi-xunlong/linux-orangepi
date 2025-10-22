/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "de_crc.h"
#include "de_crc_type.h"
#include "de_crc_platform.h"

#define DISP_CRC_OFFSET          (0x02800)

enum {
	CRC_REG_BLK_CTL = 0,
	CRC_REG_BLK_WIN,
	CRC_REG_BLK_POLY_SIZE_RUN_FRAMS,
	CRC_REG_BLK_STEP,
	CRC_REG_BLK_NUM,
};

struct de_crc_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[CRC_REG_BLK_NUM];
	const struct de_crc_dsc *dsc;
};

static inline struct crc_reg *get_crc_reg(struct de_crc_private *priv)
{
	return (struct crc_reg *)(priv->reg_blks[0].vir_addr);
}

static inline struct crc_reg *get_crc_hw_reg(struct de_crc_private *priv)
{
	return (struct crc_reg *)(priv->reg_blks[0].reg_addr);
}

static void de_crc_set_block_dirty(
	struct de_crc_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
	if (priv->reg_blks[blk_id].rcq_hd)
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
}

static int de_crc_set_size(struct de_crc_handle *hdl, u32 width, u32 height)
{
	struct de_crc_private *priv = hdl->private;
	struct crc_reg *reg = get_crc_reg(priv);
	u32 dwval;

	dwval = ((width ? (width - 1) : 0) & 0x1FFF) |
		(height ? (((height - 1) & 0x1FFF) << 16) : 0);
	reg->size.dwval = dwval;
	de_crc_set_block_dirty(priv, CRC_REG_BLK_POLY_SIZE_RUN_FRAMS, 1);

	return 0;
}

/**
 * de_crc_check_status_with_clear - check status for regions, clear status bit
 * @hdl: crc handle
 * @region_mask: region to check with mask
 *
 * check crc regions status, and clear it if status bit is set
 *
 * Returns:
 * region mask that regions status bit is set
 */

u32 de_crc_check_status_with_clear(struct de_crc_handle *hdl, unsigned int region_mask)
{
	struct de_crc_private *priv = hdl->private;
	u8 *reg = (u8 *)get_crc_hw_reg(priv);
	u32 val = 0, mask = 0, dwval = 0;
	bool check;
	bool status;
	int i;

	reg += 0x10;
	val = readl(reg);

	for (i = 0; i < priv->dsc->region_cnt; i++) {
		check = (region_mask & BIT(i)) ? true : false;
		status = (check ? BIT(i << 2) & val : 0) ? true : false;
		if (status) {
			mask |=  BIT(i);
			dwval |= BIT(i << 2);
		}
	}

	//w1c
	writel(dwval, reg);
	return mask;
}

static int de_crc_enable(struct de_crc_handle *hdl, u32 region, u32 enable, u32 irq_enable)
{
	struct de_crc_private *priv = hdl->private;
	struct crc_reg *reg = get_crc_reg(priv);

	switch (region) {
	case 0:
		reg->ctrl.bits.crc0_en = enable ? 1 : 0;
		reg->irq_ctrl.bits.crc0_irq_en = irq_enable ? 1 : 0;
		reg->first_frame.bits.crc0 = enable ? 1 : 0;
		break;
	case 1:
		reg->ctrl.bits.crc1_en = enable ? 1 : 0;
		reg->irq_ctrl.bits.crc1_irq_en = irq_enable ? 1 : 0;
		reg->first_frame.bits.crc1 = enable ? 1 : 0;
		break;
	case 2:
		reg->ctrl.bits.crc2_en = enable ? 1 : 0;
		reg->irq_ctrl.bits.crc2_irq_en = irq_enable ? 1 : 0;
		reg->first_frame.bits.crc2 = enable ? 1 : 0;
		break;
	case 3:
		reg->ctrl.bits.crc3_en = enable ? 1 : 0;
		reg->irq_ctrl.bits.crc3_irq_en = irq_enable ? 1 : 0;
		reg->first_frame.bits.crc3 = enable ? 1 : 0;
		break;
	case 4:
		reg->ctrl.bits.crc4_en = enable ? 1 : 0;
		reg->irq_ctrl.bits.crc4_irq_en = irq_enable ? 1 : 0;
		reg->first_frame.bits.crc4 = enable ? 1 : 0;
		break;
	case 5:
		reg->ctrl.bits.crc5_en = enable ? 1 : 0;
		reg->irq_ctrl.bits.crc5_irq_en = irq_enable ? 1 : 0;
		reg->first_frame.bits.crc5 = enable ? 1 : 0;
		break;
	case 6:
		reg->ctrl.bits.crc6_en = enable ? 1 : 0;
		reg->irq_ctrl.bits.crc6_irq_en = irq_enable ? 1 : 0;
		reg->first_frame.bits.crc6 = enable ? 1 : 0;
		break;
	case 7:
		reg->ctrl.bits.crc7_en = enable ? 1 : 0;
		reg->irq_ctrl.bits.crc7_irq_en = irq_enable ? 1 : 0;
		reg->first_frame.bits.crc7 = enable ? 1 : 0;
		break;
	default:
		DRM_ERROR("not support crc region%d", region);
		return -1;
	}

	de_crc_set_block_dirty(priv, CRC_REG_BLK_CTL, 1);

	//write default value
	reg->poly.dwval = 0x0000a001;
	de_crc_set_block_dirty(priv, CRC_REG_BLK_POLY_SIZE_RUN_FRAMS, 1);
	return 0;
}

s32 de_crc_set_polarity(struct de_crc_handle *hdl, u32 region, u32 polarity)
{
	struct de_crc_private *priv = hdl->private;
	struct crc_reg *reg = get_crc_reg(priv);

	switch (region) {
	case 0:
		reg->pol.bits.crc0 = polarity ? 1 : 0;
		break;
	case 1:
		reg->pol.bits.crc1 = polarity ? 1 : 0;
		break;
	case 2:
		reg->pol.bits.crc2 = polarity ? 1 : 0;
		break;
	case 3:
		reg->pol.bits.crc3 = polarity ? 1 : 0;
		break;
	case 4:
		reg->pol.bits.crc4 = polarity ? 1 : 0;
		break;
	case 5:
		reg->pol.bits.crc5 = polarity ? 1 : 0;
		break;
	case 6:
		reg->pol.bits.crc6 = polarity ? 1 : 0;
		break;
	case 7:
		reg->pol.bits.crc7 = polarity ? 1 : 0;
		break;
	default:
		DRM_ERROR("not support crc region%d", region);
		return -1;
	}

	de_crc_set_block_dirty(priv, CRC_REG_BLK_CTL, 1);
	return 0;
}

static int de_crc_set_win(struct de_crc_handle *hdl, u32 region, u32 x_start, u32 x_end, u32 y_start, u32 y_end)
{
	struct de_crc_private *priv = hdl->private;
	struct crc_reg *reg = get_crc_reg(priv);
	struct crc_region_win *win = &reg->win[region & 0x7];
	u32 dwval;

	dwval = (x_start & 0x1FFF) | ((x_end & 0x1FFF) << 16);
	win->hori.dwval = dwval;

	dwval = (y_start & 0x1FFF) | ((y_end & 0x1FFF) << 16);
	win->vert.dwval = dwval;

	de_crc_set_block_dirty(priv, CRC_REG_BLK_WIN, 1);
	return 0;
}

static int de_crc_set_compare_step(struct de_crc_handle *hdl, u32 region, u32 step)
{
	struct de_crc_private *priv = hdl->private;
	struct crc_reg *reg = get_crc_reg(priv);

	reg->step[region].dwval = step & 0xFFFF;
	de_crc_set_block_dirty(priv, CRC_REG_BLK_STEP, 1);
	return 0;
}
/*
int de_crc_set_run_frames(u32 disp, u32 frames)
{
	struct de_crc_private *priv = &(crc_priv[disp]);
	struct crc_reg *reg = get_crc_reg(priv);

	reg->run_frames.dwval = frames & 0xFFFF;
	de_crc_set_block_dirty(priv, CRC_REG_BLK_POLY_SIZE_RUN_FRAMS, 1);
	return 0;
}*/

int de_crc_region_config(struct de_crc_handle *hdl, const struct de_crc_region_cfg *cfg)
{
	de_crc_set_win(hdl, cfg->region_id, cfg->x_start, cfg->x_end, cfg->y_start, cfg->y_end);
	de_crc_set_compare_step(hdl, cfg->region_id, cfg->check_frame_step);
	de_crc_set_polarity(hdl, cfg->region_id, cfg->mode);
	de_crc_enable(hdl, cfg->region_id, cfg->enable, cfg->irq_enable);
	return 0;
}

int de_crc_global_config(struct de_crc_handle *hdl, const struct de_crc_gbl_cfg *cfg)
{
	return de_crc_set_size(hdl, cfg->w, cfg->h);
}

void de_crc_dump_state(struct drm_printer *p, struct de_crc_handle *hdl)
{

}

struct de_crc_handle *de_crc_create(struct module_create_info *info)
{
	struct de_crc_handle *hdl;
	struct de_reg_block *reg_blk;
	struct de_reg_mem_info *reg_mem_info;
	u8 __iomem *reg_base;
	int i;
	struct de_crc_private *priv;
	const struct de_crc_dsc *dsc;

	dsc = get_crc_dsc(info);
	if (!dsc)
		return NULL;

	reg_base = (u8 __iomem *)(info->de_reg_base + info->reg_offset + DISP_CRC_OFFSET);
	hdl = kmalloc(sizeof(*hdl), GFP_KERNEL | __GFP_ZERO);
	memcpy(&hdl->cinfo, info, sizeof(*info));
	hdl->private = kmalloc(sizeof(*hdl->private), GFP_KERNEL | __GFP_ZERO);
	hdl->region_cnt = dsc->region_cnt;
	priv = hdl->private;
	hdl->private->dsc = dsc;
	reg_mem_info = &(hdl->private->reg_mem_info);
	reg_mem_info->size = sizeof(struct crc_reg);
	reg_mem_info->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
		reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
		info->update_mode == RCQ_MODE);
	if (reg_mem_info->vir_addr == NULL) {
		DRM_ERROR("alloc crc[%d] mm fail!size=0x%x\n",
			 dsc->id, reg_mem_info->size);
		return ERR_PTR(-ENOMEM);
	}

	reg_blk = &(priv->reg_blks[CRC_REG_BLK_CTL]);
	reg_blk->phy_addr = reg_mem_info->phy_addr;
	reg_blk->vir_addr = reg_mem_info->vir_addr;
	reg_blk->size = 0x10;
	reg_blk->reg_addr = (u8 __iomem *)reg_base;

	reg_blk = &(priv->reg_blks[CRC_REG_BLK_WIN]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0xa0;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0xa0;
	reg_blk->size = 0x80;
	reg_blk->reg_addr = (u8 __iomem *)reg_base + 0xa0;

	reg_blk = &(priv->reg_blks[CRC_REG_BLK_POLY_SIZE_RUN_FRAMS]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x120;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x120;
	reg_blk->size = 0x10;
	reg_blk->reg_addr = (u8 __iomem *)reg_base + 0x120;

	reg_blk = &(priv->reg_blks[CRC_REG_BLK_STEP]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x130;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x130;
	reg_blk->size = 0x4 * 8;
	reg_blk->reg_addr = (u8 __iomem *)reg_base + 0x130;

	priv->reg_blk_num = CRC_REG_BLK_NUM;
	hdl->block_num = priv->reg_blk_num;
	hdl->block = kmalloc(sizeof(reg_blk[0]) * hdl->block_num, GFP_KERNEL | __GFP_ZERO);
	for (i = 0; i < hdl->private->reg_blk_num; i++)
		hdl->block[i] = &priv->reg_blks[i];

	return hdl;
}
