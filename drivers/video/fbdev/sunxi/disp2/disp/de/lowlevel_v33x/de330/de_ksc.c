/*
 * de_ksc/de_ksc.c
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
#include "de_ksc_type.h"
#include "../../include.h"
#include "de_feat.h"
#include "de_top.h"
#include "de_rtmx.h"
#include "de_ksc.h"


struct de_ksc_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks;

	void (*set_blk_dirty)(struct de_ksc_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_ksc_private ksc_priv[DE_NUM];

static inline struct ksc_reg *get_ksc_reg(struct de_ksc_private *priv)
{
	return (struct ksc_reg *)(priv->reg_blks.vir_addr);
}

static void ksc_set_block_dirty(
	struct de_ksc_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks.dirty = dirty;
}

static void ksc_set_rcq_head_dirty(
	struct de_ksc_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks.rcq_hd) {
		priv->reg_blks.rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WRN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

s32 de_ksc_set_para(u32 disp, struct disp_ksc_info *pinfo)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_output_info *output = &(ctx->output);

	struct de_ksc_private *priv = &ksc_priv[disp];
	struct ksc_reg *reg = get_ksc_reg(priv);
	u32 dwval;
	unsigned int ksccoef[8] = {
		0x000d270c,
		0x00122608,
		0x00162505,
		0x001b2302,
		0x00202000,
		0x02221c00,
		0x05241700,
		0x08261200,
	};

	dwval = (output->scn_width ?
		((output->scn_width - 1) & 0x1FFF) : 0)
		| (output->scn_height ?
		(((output->scn_height - 1) & 0x1FFF) << 16) : 0);
	reg->ksc_size.dwval = dwval;

	memcpy(&reg->coef, ksccoef, 8 * sizeof(unsigned int));
	reg->ksc_ctrl.bits.en = pinfo->enable;
	reg->ksc_fw.bits.first_line_width = pinfo->first_line_width - 1;
	reg->ksc_cvsf.bits.sign_bit = pinfo->direction;
	reg->ksc_cvsf.bits.int_ration = (pinfo->ration & 0x000ff000) >> 12;
	reg->ksc_cvsf.bits.frac_ration = pinfo->ration & 0x00000fff;

	if (ctx->output.px_fmt_space == DE_FORMAT_SPACE_RGB) {
		reg->ksc_color.dwval = 0;
	} else  {
		reg->ksc_color.bits.red_y = 10;
		reg->ksc_color.bits.green_u = 128;
		reg->ksc_color.bits.blue_v = 128;
	}

	priv->set_blk_dirty(priv, 0, 1);
	return 0;
}

s32 de_ksc_init(u32 disp, u8 __iomem *de_reg_base)
{
	u8 __iomem *reg_base = de_reg_base
		+ DE_DISP_OFFSET(disp) + DISP_KSC_OFFSET;
	u32 rcq_used = de_feat_is_using_rcq(disp);
	struct de_ksc_private *priv = &ksc_priv[disp];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
	struct de_reg_block *block;

	reg_mem_info->size = sizeof(struct ksc_reg);
	reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
	    reg_mem_info->size, (void *)&(reg_mem_info->phy_addr), rcq_used);
	if (NULL == reg_mem_info->vir_addr) {
		DE_WRN("alloc bld[%d] mm fail!size=0x%x\n",
		     disp, reg_mem_info->size);
		return -1;
	}

	block = &(priv->reg_blks);
	block->phy_addr = reg_mem_info->phy_addr;
	block->vir_addr = reg_mem_info->vir_addr;
	block->size = reg_mem_info->size;
	block->reg_addr = reg_base;

	priv->reg_blk_num = 1;

	if (rcq_used)
		priv->set_blk_dirty = ksc_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = ksc_set_block_dirty;
	return 0;
}

s32 de_ksc_exit(u32 disp)
{

	struct de_ksc_private *priv = &ksc_priv[disp];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);

	if (reg_mem_info->vir_addr != NULL)
		de_top_reg_memory_free(reg_mem_info->vir_addr,
			reg_mem_info->phy_addr, reg_mem_info->size);

	return 0;
}

s32 de_ksc_get_reg_blocks(u32 disp,
	struct de_reg_block **blks, u32 *blk_num)
{
	struct de_ksc_private *priv = &(ksc_priv[disp]);
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
		blks[i] = &priv->reg_blks + i;

	*blk_num = i;
	return 0;
}
