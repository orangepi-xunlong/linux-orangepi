/* SPDX-License-Identifier: GPL-2.0-or-later */
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
 *  File name   :  display engine 35x dither basic function definition
 *
 *  History     :  2021/11/30 v0.1  Initial version
 *
 ******************************************************************************/

#include "de_dither_type.h"
#include "de_rtmx.h"
#include "de_enhance.h"

enum {
	DITHER_PARA_REG_BLK = 0,
	DITHER_REG_BLK_NUM,
};

struct de_dither_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[DITHER_REG_BLK_NUM];
	void (*set_blk_dirty)(struct de_dither_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_dither_private dither_priv[DE_NUM];


static inline struct dither_reg *get_dither_reg(struct de_dither_private *priv)
{
	return (struct dither_reg *)(priv->reg_blks[DITHER_PARA_REG_BLK].vir_addr);
}

static void dither_set_block_dirty(
	struct de_dither_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void dither_set_rcq_head_dirty(
	struct de_dither_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WARN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

s32 de_dither_init(u32 disp, uintptr_t reg_base)
{
	struct de_dither_private *priv = &dither_priv[disp];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
	struct de_reg_block *reg_blk;
	uintptr_t base;
	u32 rcq_used = de_feat_is_using_rcq(disp);

	base = reg_base + DE_DISP_OFFSET(disp) + DISP_DITHER_OFFSET;

	reg_mem_info->size = sizeof(struct dither_reg);
	reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
		reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
		rcq_used);
	if (reg_mem_info->vir_addr == NULL) {
		DE_WARN("alloc smbl[%d] mm fail!size=0x%x\n",
			 disp, reg_mem_info->size);
		return -1;
	}

	priv->reg_blk_num = DITHER_REG_BLK_NUM;

	reg_blk = &(priv->reg_blks[DITHER_PARA_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr;
	reg_blk->vir_addr = reg_mem_info->vir_addr;
	reg_blk->size = sizeof(struct dither_reg);
	reg_blk->reg_addr = (u8 __iomem *)base;
	if (rcq_used)
		priv->set_blk_dirty = dither_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = dither_set_block_dirty;

    return 0;
}

s32 de_dither_exit(u32 disp)
{
	return 0;
}

s32 de_dither_get_reg_blocks(u32 disp, struct de_reg_block **blks, u32 *blk_num)
{
	struct de_dither_private *priv = &(dither_priv[disp]);
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

s32 de_dither_set_size(u32 disp, u32 width, u32 height)
{
	struct de_dither_private *priv = &(dither_priv[disp]);
	struct dither_reg *reg = get_dither_reg(priv);

	reg->size.bits.width = width - 1;
	reg->size.bits.height = height - 1;
	priv->set_blk_dirty(priv, DITHER_PARA_REG_BLK, 1);

	return 0;
}

/*static s32 de_dither_apply(u32 disp, struct dither_config data)
{
	struct de_dither_private *priv = &dither_priv[disp];
	struct dither_reg *reg = get_dither_reg(priv);

    reg->ctl.bits.en = data.enable;
    priv->set_blk_dirty(priv, DITHER_PARA_REG_BLK, 1);

	return 0;
}*/

s32 de_dither_enable(u32 disp, u32 en)
{
	struct de_dither_private *priv = &(dither_priv[disp]);
	struct dither_reg *reg = get_dither_reg(priv);

	reg->ctl.dwval = en ? 1 : 0;
	priv->set_blk_dirty(priv, DITHER_PARA_REG_BLK, 1);

	return 0;
}

s32 de_dither_init_para(u32 disp)
{
	struct de_dither_private *priv = &(dither_priv[disp]);
	struct dither_reg *reg = get_dither_reg(priv);

	reg->ctl.dwval = 0x0;
    priv->set_blk_dirty(priv, DITHER_PARA_REG_BLK, 1);
	return 0;
}
