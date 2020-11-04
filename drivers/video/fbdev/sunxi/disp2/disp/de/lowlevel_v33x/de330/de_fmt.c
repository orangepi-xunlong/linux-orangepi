/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "de_fmt_type.h"

#include "../../include.h"
#include "de_feat.h"
#include "de_top.h"
#include "de_rtmx.h"
#include "de_fmt.h"


enum {
	FMT_FMT_REG_BLK = 0,
	FMT_REG_BLK_NUM,
};

struct de_fmt_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[FMT_REG_BLK_NUM];

	void (*set_blk_dirty)(struct de_fmt_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_fmt_private fmt_priv[DE_NUM];

static inline struct fmt_reg *get_fmt_reg(struct de_fmt_private *priv)
{
	return (struct fmt_reg *)(priv->reg_blks[0].vir_addr);
}

static void fmt_set_block_dirty(
	struct de_fmt_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void fmt_set_rcq_head_dirty(
	struct de_fmt_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WRN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

s32 de_fmt_set_para(u32 disp)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_output_info *output = &(ctx->output);

	struct de_fmt_private *priv = &fmt_priv[disp];
	struct fmt_reg *reg = get_fmt_reg(priv);
	u32 dwval;

	reg->ctl.dwval = 1;

	dwval = (output->scn_width ?
		((output->scn_width - 1) & 0x1FFF) : 0)
		| (output->scn_height ?
		(((output->scn_height - 1) & 0x1FFF) << 16) : 0);
	reg->size.dwval = dwval;

	reg->swap.dwval = 0;

	reg->bitdepth.dwval = (output->data_bits == DE_DATA_8BITS) ? 0 : 1;

	dwval = 0;
	if (output->px_fmt_space == DE_FORMAT_SPACE_YUV) {
		if (output->yuv_sampling == DE_YUV422)
			dwval = 1;
		else if (output->yuv_sampling == DE_YUV420)
			dwval = 2;
	}
	reg->fmt_type.dwval = dwval;

	reg->coeff.dwval = 0;

	if ((output->px_fmt_space == DE_FORMAT_SPACE_RGB)
		|| ((output->px_fmt_space == DE_FORMAT_SPACE_YUV)
		&& (output->yuv_sampling == DE_YUV444))) {
		reg->limit_y.dwval = 0x0fff0000;
		reg->limit_c0.dwval = 0x0fff0000;
		reg->limit_c1.dwval = 0x0fff0000;
	} else {
		reg->limit_y.dwval = 0xeb00100;
		reg->limit_c0.dwval = 0xf000100;
		reg->limit_c1.dwval = 0xf000100;
	}

	priv->set_blk_dirty(priv, FMT_FMT_REG_BLK, 1);

	return 0;
}

s32 de_fmt_init(u32 disp, u8 __iomem *de_reg_base)
{

	u8 __iomem *reg_base = de_reg_base
		+ DE_DISP_OFFSET(disp) + DISP_FMT_OFFSET;
	u32 rcq_used = de_feat_is_using_rcq(disp);

	struct de_fmt_private *priv = &fmt_priv[disp];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
	struct de_reg_block *block;

	reg_mem_info->size = sizeof(struct fmt_reg);
	reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
		reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
		rcq_used);
	if (NULL == reg_mem_info->vir_addr) {
		DE_WRN("alloc bld[%d] mm fail!size=0x%x\n",
		     disp, reg_mem_info->size);
		return -1;
	}

	block = &(priv->reg_blks[FMT_FMT_REG_BLK]);
	block->phy_addr = reg_mem_info->phy_addr;
	block->vir_addr = reg_mem_info->vir_addr;
	block->size = reg_mem_info->size;
	block->reg_addr = reg_base;

	priv->reg_blk_num = FMT_REG_BLK_NUM;

	if (rcq_used)
		priv->set_blk_dirty = fmt_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = fmt_set_block_dirty;

	return 0;
}

s32 de_fmt_exit(u32 disp)
{

	struct de_fmt_private *priv = &fmt_priv[disp];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);

	if (reg_mem_info->vir_addr != NULL)
		de_top_reg_memory_free(reg_mem_info->vir_addr,
			reg_mem_info->phy_addr, reg_mem_info->size);

	return 0;
}

s32 de_fmt_get_reg_blocks(u32 disp,
	struct de_reg_block **blks, u32 *blk_num)
{
	struct de_fmt_private *priv = &(fmt_priv[disp]);
	u32 i, num;

	if (blks == NULL) {
		*blk_num = priv->reg_blk_num;
		return 0;
	}

	if (*blk_num >= priv->reg_blk_num) {
		num = priv->reg_blk_num;
	} else {
		num = *blk_num;
		DE_WRN("should not happen\n");
	}
	for (i = 0; i < num; ++i)
		blks[i] = priv->reg_blks + i;

	*blk_num = i;
	return 0;
}
