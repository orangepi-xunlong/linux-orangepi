/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "de_crc_type.h"
#include "../../include.h"
#include "de_feat.h"
#include "de_top.h"
#include "de_rtmx.h"
#include "de_crc.h"

enum {
	CRC_REG_BLK_CTL = 0,
	CRC_REG_BLK_CRC_VAL,
	CRC_REG_BLK_WIN,
	CRC_REG_BLK_POLY,
//	CRC_REG_BLK_SIZE,
	CRC_REG_BLK_RUN_FRAMS,
	CRC_REG_BLK_RUN_CNT,
	CRC_REG_BLK_STEP,
	CRC_REG_BLK_NUM,
};

struct de_crc_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[CRC_REG_BLK_NUM];
	void (*set_blk_dirty)(struct de_crc_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_crc_private crc_priv[DE_NUM];

static inline struct crc_reg *get_crc_reg(struct de_crc_private *priv)
{
	return (struct crc_reg *)(priv->reg_blks[0].vir_addr);
}

static inline struct crc_reg *get_crc_hw_reg(struct de_crc_private *priv)
{
	return (struct crc_reg *)(priv->reg_blks[0].reg_addr);
}

static void crc_set_block_dirty(
	struct de_crc_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void crc_set_rcq_head_dirty(
	struct de_crc_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WARN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

s32 de_crc_init(u32 disp, u8 __iomem *reg_base)
{
	struct de_crc_private *priv = &crc_priv[disp];
	u32 hw_disp = de_feat_get_hw_disp(disp);
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
	struct de_reg_block *reg_blk;
	uintptr_t base;
	u32 rcq_used = de_feat_is_using_rcq(disp);

	base = (uintptr_t)reg_base + DE_DISP_OFFSET(hw_disp) + DISP_CRC_OFFSET;

	printk("de wrn crc %x\n", DE_DISP_OFFSET(hw_disp) + DISP_CRC_OFFSET);

	if (!de_feat_is_support_crc(disp))
		return 0;

	reg_mem_info->size = sizeof(struct crc_reg);
	reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
		reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
		rcq_used);
	if (reg_mem_info->vir_addr == NULL) {
		DE_WARN("alloc smbl[%d] mm fail!size=0x%x\n",
			 disp, reg_mem_info->size);
		return -1;
	}

	priv->reg_blk_num = CRC_REG_BLK_NUM;

	reg_blk = &(priv->reg_blks[CRC_REG_BLK_CTL]);
	reg_blk->phy_addr = reg_mem_info->phy_addr;
	reg_blk->vir_addr = reg_mem_info->vir_addr;
	reg_blk->size = 0x14;
	reg_blk->reg_addr = (u8 __iomem *)base;

	reg_blk = &(priv->reg_blks[CRC_REG_BLK_CRC_VAL]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x20;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x20;
	reg_blk->size = 0x80;
	reg_blk->reg_addr = (u8 __iomem *)base + 0x20;

	reg_blk = &(priv->reg_blks[CRC_REG_BLK_WIN]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0xa0;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0xa0;
	reg_blk->size = 0x80;
	reg_blk->reg_addr = (u8 __iomem *)base + 0xa0;

	reg_blk = &(priv->reg_blks[CRC_REG_BLK_POLY]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x120;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x120;
	reg_blk->size = 0x8;//fixme
	reg_blk->reg_addr = (u8 __iomem *)base + 0x120;

/*
	reg_blk = &(priv->reg_blks[CRC_REG_BLK_SIZE]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x124;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x124;
	reg_blk->size = 0x4;
	reg_blk->reg_addr = (u8 __iomem *)base + 0x124;
*/

	reg_blk = &(priv->reg_blks[CRC_REG_BLK_RUN_FRAMS]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x128;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x128;
	reg_blk->size = 0x4;
	reg_blk->reg_addr = (u8 __iomem *)base + 0x128;

	reg_blk = &(priv->reg_blks[CRC_REG_BLK_RUN_CNT]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x12c;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x12c;
	reg_blk->size = 0x4;
	reg_blk->reg_addr = (u8 __iomem *)base + 0x12c;

	reg_blk = &(priv->reg_blks[CRC_REG_BLK_STEP]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x130;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x130;
	reg_blk->size = 0x4 * 8;
	reg_blk->reg_addr = (u8 __iomem *)base + 0x130;

	if (rcq_used)
		priv->set_blk_dirty = crc_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = crc_set_block_dirty;

    return 0;
}

s32 de_crc_exit(u32 disp)
{
	return 0;
}

s32 de_crc_get_reg_blocks(u32 disp, struct de_reg_block **blks, u32 *blk_num)
{
	struct de_crc_private *priv = &(crc_priv[disp]);
	u32 i, num;

	if (blks == NULL) {
		*blk_num = priv->reg_blk_num;
		return 0;
	}

	if (*blk_num >= priv->reg_blk_num) {
		num = priv->reg_blk_num;
	} else {
		num = *blk_num;
		DE_WARN("should not happen\n");
	}
	for (i = 0; i < num; ++i)
		blks[i] = priv->reg_blks + i;

	*blk_num = i;
	return 0;
}

s32 de_crc_set_size(u32 disp, u32 width, u32 height)
{
	struct de_crc_private *priv = &(crc_priv[disp]);
	struct crc_reg *reg = get_crc_reg(priv);
	u32 dwval;

	dwval = ((width ? (width - 1) : 0) & 0x1FFF) |
		(height ? (((height - 1) & 0x1FFF) << 16) : 0);
	reg->size.dwval = dwval;
//fixme test
	priv->set_blk_dirty(priv, CRC_REG_BLK_POLY, 1);
//	priv->set_blk_dirty(priv, CRC_REG_BLK_SIZE, 1);
	printk("set size %x\n", reg->size.dwval);

	return 0;
}

enum de_crc_status de_crc_get_status(u32 disp)
{
	enum de_crc_status status;
	struct de_crc_private *priv = &(crc_priv[disp]);
	volatile struct crc_reg *reg = get_crc_hw_reg(priv);
	u32 val = reg->status.dwval;
	reg->status.dwval = val;
	status = (enum de_crc_status)val;
	return status;
}

s32 de_crc_enable(u32 disp, u32 region, u32 enable)
{
//	u32 rval;
//	u32 dwval;
	struct de_crc_private *priv = &(crc_priv[disp]);
	struct crc_reg *reg = get_crc_reg(priv);
#if defined(__DISP_TEMP_CODE__)
//	volatile struct crc_reg *hw_reg = get_crc_reg(priv);

//	rval = hw_reg->ctrl.dwval;
//	reg->ctrl.dwval = rval;
	assign_bit(4 * (region & 0x7), &reg->ctrl.dwval, enable ? 1 : 0);

//	rval = hw_reg->irq_ctrl.dwval;
//	reg->irq_ctrl.dwval = rval;
	assign_bit(4 * (region & 0x7), &reg->irq_ctrl, enable ? 1 : 0);

//	rval = hw_reg->first_frame.dwval;
//	reg->first_frame.dwval = rval;
	assign_bit(4 * (region & 0x7), &hw_reg->first_frame, enable ? 1 : 0);
#endif
	switch (region) {
	case 0:
		reg->ctrl.bits.crc0_en = enable ? 1 : 0;
		reg->irq_ctrl.bits.crc0_irq_en = enable ? 1 : 0;
		reg->first_frame.bits.crc0 = enable ? 1 : 0;
		break;
	case 1:
		reg->ctrl.bits.crc1_en = enable ? 1 : 0;
		reg->irq_ctrl.bits.crc1_irq_en = enable ? 1 : 0;
		reg->first_frame.bits.crc1 = enable ? 1 : 0;
		break;
	case 2:
		reg->ctrl.bits.crc2_en = enable ? 1 : 0;
		reg->irq_ctrl.bits.crc2_irq_en = enable ? 1 : 0;
		reg->first_frame.bits.crc2 = enable ? 1 : 0;
		break;
	case 3:
		reg->ctrl.bits.crc3_en = enable ? 1 : 0;
		reg->irq_ctrl.bits.crc3_irq_en = enable ? 1 : 0;
		reg->first_frame.bits.crc3 = enable ? 1 : 0;
		break;
	case 4:
		reg->ctrl.bits.crc4_en = enable ? 1 : 0;
		reg->irq_ctrl.bits.crc4_irq_en = enable ? 1 : 0;
		reg->first_frame.bits.crc4 = enable ? 1 : 0;
		break;
	case 5:
		reg->ctrl.bits.crc5_en = enable ? 1 : 0;
		reg->irq_ctrl.bits.crc5_irq_en = enable ? 1 : 0;
		reg->first_frame.bits.crc5 = enable ? 1 : 0;
		break;
	case 6:
		reg->ctrl.bits.crc6_en = enable ? 1 : 0;
		reg->irq_ctrl.bits.crc6_irq_en = enable ? 1 : 0;
		reg->first_frame.bits.crc6 = enable ? 1 : 0;
		break;
	case 7:
		reg->ctrl.bits.crc7_en = enable ? 1 : 0;
		reg->irq_ctrl.bits.crc7_irq_en = enable ? 1 : 0;
		reg->first_frame.bits.crc7 = enable ? 1 : 0;
		break;
	default:
		DE_WARN("not support crc region%d", region);
		return -1;
	}

	priv->set_blk_dirty(priv, CRC_REG_BLK_CTL, 1);

	//write default value
	reg->poly.dwval = 0x0000a001;
	priv->set_blk_dirty(priv, CRC_REG_BLK_POLY, 1);
	return 0;
}

s32 de_crc_set_polarity(u32 disp, u32 region, u32 polarity)
{
	struct de_crc_private *priv = &(crc_priv[disp]);
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
		DE_WARN("not support crc region%d", region);
		return -1;
	}

	priv->set_blk_dirty(priv, CRC_REG_BLK_CTL, 1);
	return 0;
}

s32 de_crc_set_win(u32 disp, u32 region, u32 x_start, u32 x_end, u32 y_start, u32 y_end)
{
	struct de_crc_private *priv = &(crc_priv[disp]);
	struct crc_reg *reg = get_crc_reg(priv);
	struct crc_region_win *win = &reg->win[region & 0x7];
	u32 dwval;

	dwval = (x_start & 0x1FFF) | ((x_end & 0x1FFF) << 16);
	win->hori.dwval = dwval;

	dwval = (y_start & 0x1FFF) | ((y_end & 0x1FFF) << 16);
	win->vert.dwval = dwval;

	priv->set_blk_dirty(priv, CRC_REG_BLK_WIN, 1);
	return 0;
}
/*
s32 de_crc_set_size(u32 disp, u32 width, u32 height)
{
	struct de_crc_private *priv = &(crc_priv[disp]);
	struct crc_reg *reg = get_crc_reg(priv);
	u32 dwval;

	dwval = (width & 0x1FFF) | ((height & 0x1FFF) << 16);
	reg->size.dwval = dwval;

	priv->set_blk_dirty(priv, CRC_REG_BLK_SIZE, 1);
	return 0;
}
*/
s32 de_crc_set_compare_step(u32 disp, u32 region, u32 step)
{
	struct de_crc_private *priv = &(crc_priv[disp]);
	struct crc_reg *reg = get_crc_reg(priv);
//	u32 dwval;

	reg->step[region].dwval = step & 0xFFFF;
	priv->set_blk_dirty(priv, CRC_REG_BLK_STEP, 1);
	return 0;
}

s32 de_crc_set_run_frames(u32 disp, u32 frames)
{
	struct de_crc_private *priv = &(crc_priv[disp]);
	struct crc_reg *reg = get_crc_reg(priv);
//	u32 dwval;

	reg->run_frames.dwval = frames & 0xFFFF;
	priv->set_blk_dirty(priv, CRC_REG_BLK_RUN_FRAMS, 1);
	return 0;
}
