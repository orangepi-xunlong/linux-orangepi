/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "de_ccsc_type.h"
#include "de_csc_table.h"

#include "../../include.h"
#include "de_ccsc.h"
#include "de_top.h"
#include "de_feat.h"

enum {
	CCSC_REG_BLK_CTL = 0,
	CCSC_REG_BLK_COEFF,
	CCSC_REG_BLK_ALPHA,
	CCSC_REG_BLK_NUM,
};

struct de_ccsc_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[CCSC_REG_BLK_NUM];

	void (*set_blk_dirty)(struct de_ccsc_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_ccsc_private ccsc_priv[DE_NUM][MAX_CHN_NUM];

static inline struct ccsc_reg *get_ccsc_reg(struct de_ccsc_private *priv)
{
	return (struct ccsc_reg *)(priv->reg_blks[0].vir_addr);
}

static void ccsc_set_block_dirty(
	struct de_ccsc_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void ccsc_set_rcq_head_dirty(
	struct de_ccsc_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WRN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

s32 de_ccsc_enable(u32 disp, u32 chn, u8 en)
{
	struct de_ccsc_private *priv = &(ccsc_priv[disp][chn]);
	struct ccsc_reg *reg = get_ccsc_reg(priv);

	if (en != reg->ctl.bits.en) {
		reg->ctl.dwval = en;
		priv->set_blk_dirty(priv, CCSC_REG_BLK_CTL, 1);
	}
	return 0;
}

s32 de_ccsc_apply(u32 disp, u32 chn,
	struct de_csc_info *in_info, struct de_csc_info *out_info)
{
	struct de_ccsc_private *priv = &(ccsc_priv[disp][chn]);
	struct ccsc_reg *reg = get_ccsc_reg(priv);
	u32 *ccsc_coeff = NULL;

	de_csc_coeff_calc(in_info, out_info, &ccsc_coeff);
	if (ccsc_coeff != NULL) {
		s32 dwval;

		dwval = *(ccsc_coeff + 12);
		dwval = ((dwval & 0x80000000) ? (u32)(-(s32)dwval) : dwval) & 0x3FF;
		reg->d0.dwval = dwval;
		dwval = *(ccsc_coeff + 13);
		dwval = ((dwval & 0x80000000) ? (u32)(-(s32)dwval) : dwval) & 0x3FF;
		reg->d1.dwval = dwval;
		dwval = *(ccsc_coeff + 14);
		dwval = ((dwval & 0x80000000) ? (u32)(-(s32)dwval) : dwval) & 0x3FF;
		reg->d2.dwval = dwval;

		reg->c0[0].dwval = *(ccsc_coeff + 0);
		reg->c0[1].dwval = *(ccsc_coeff + 1);
		reg->c0[2].dwval = *(ccsc_coeff + 2);
		reg->c0[3].dwval = *(ccsc_coeff + 3);

		reg->c1[0].dwval = *(ccsc_coeff + 4);
		reg->c1[1].dwval = *(ccsc_coeff + 5);
		reg->c1[2].dwval = *(ccsc_coeff + 6);
		reg->c1[3].dwval = *(ccsc_coeff + 7);

		reg->c2[0].dwval = *(ccsc_coeff + 8);
		reg->c2[1].dwval = *(ccsc_coeff + 9);
		reg->c2[2].dwval = *(ccsc_coeff + 10);
		reg->c2[3].dwval = *(ccsc_coeff + 11);

		reg->ctl.dwval = 1;
		priv->set_blk_dirty(priv, CCSC_REG_BLK_COEFF, 1);
		priv->set_blk_dirty(priv, CCSC_REG_BLK_CTL, 0);
	}
	return 0;
}

s32 de_ccsc_set_alpha(u32 disp, u32 chn, u8 en, u8 alpha)
{
	struct de_ccsc_private *priv = &(ccsc_priv[disp][chn]);
	struct ccsc_reg *reg = get_ccsc_reg(priv);

	reg->alpha.dwval = alpha | (en << 8);
	priv->set_blk_dirty(priv, CCSC_REG_BLK_ALPHA, 1);
	return 0;
}

s32 de_ccsc_init(u32 disp, u8 __iomem *de_reg_base)
{
	u32 chn_num, chn;

	chn_num = de_feat_get_num_chns(disp);
	for (chn = 0; chn < chn_num; ++chn) {
		u32 phy_chn = de_feat_get_phy_chn_id(disp, chn);
		u8 __iomem *reg_base = de_reg_base
			+ DE_CHN_OFFSET(phy_chn) + CHN_CCSC_OFFSET;
		u32 rcq_used = de_feat_is_using_rcq(disp);

		struct de_ccsc_private *priv = &ccsc_priv[disp][chn];
		struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
		struct de_reg_block *block;

		reg_mem_info->size = sizeof(struct ccsc_reg);
		reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
			reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
			rcq_used);
		if (NULL == reg_mem_info->vir_addr) {
			DE_WRN("alloc ccsc[%d] mm fail!size=0x%x\n",
				 disp, reg_mem_info->size);
			return -1;
		}

		block = &(priv->reg_blks[CCSC_REG_BLK_CTL]);
		block->phy_addr = reg_mem_info->phy_addr;
		block->vir_addr = reg_mem_info->vir_addr;
		block->size = 0x4;
		block->reg_addr = reg_base;

		block = &(priv->reg_blks[CCSC_REG_BLK_COEFF]);
		block->phy_addr = reg_mem_info->phy_addr;
		block->vir_addr = reg_mem_info->vir_addr;
		block->size = 0x40;
		block->reg_addr = reg_base;

		block = &(priv->reg_blks[CCSC_REG_BLK_ALPHA]);
		block->phy_addr = reg_mem_info->phy_addr + 0x40;
		block->vir_addr = reg_mem_info->vir_addr + 0x40;
		block->size = 0x4;
		block->reg_addr = reg_base + 0x40;

		priv->reg_blk_num = CCSC_REG_BLK_NUM;

		if (rcq_used)
			priv->set_blk_dirty = ccsc_set_rcq_head_dirty;
		else
			priv->set_blk_dirty = ccsc_set_block_dirty;
	}

	return 0;
}

s32 de_ccsc_exit(u32 disp)
{
	u32 chn_num, chn;

	chn_num = de_feat_get_num_chns(disp);
	for (chn = 0; chn < chn_num; ++chn) {
		struct de_ccsc_private *priv = &ccsc_priv[disp][chn];
		struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);

		if (reg_mem_info->vir_addr != NULL)
			de_top_reg_memory_free(reg_mem_info->vir_addr,
				reg_mem_info->phy_addr, reg_mem_info->size);
	}

	return 0;
}

s32 de_ccsc_get_reg_blocks(u32 disp,
	struct de_reg_block **blks, u32 *blk_num)
{
	u32 chn_num, chn;
	u32 total = 0;

	chn_num = de_feat_get_num_chns(disp);

	if (blks == NULL) {
		for (chn = 0; chn < chn_num; ++chn)
			total += ccsc_priv[disp][chn].reg_blk_num;
		*blk_num = total;
		return 0;
	}

	for (chn = 0; chn < chn_num; ++chn) {
		struct de_ccsc_private *priv = &(ccsc_priv[disp][chn]);
		struct de_reg_block *blk_begin, *blk_end;
		u32 num;

		if (*blk_num >= priv->reg_blk_num) {
			num = priv->reg_blk_num;
		} else {
			DE_WRN("should not happen\n");
			num = *blk_num;
		}
		blk_begin = priv->reg_blks;
		blk_end = blk_begin + num;
		for (; blk_begin != blk_end; ++blk_begin)
			*blks++ = blk_begin;
		total += num;
		*blk_num -= num;
	}
	*blk_num = total;
	return 0;
}
