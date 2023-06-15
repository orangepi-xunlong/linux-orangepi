/*
 * drivers/video/fbdev/sunxi/disp2/disp/de/lowlevel_v33x/de330/de_snr/de_snr.c
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

#include "de_snr_type.h"
#include "../../include.h"
#include "de_feat.h"
#include "de_top.h"
#include "de_rtmx.h"
#include "de_snr.h"

struct de_snr_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks;

	void (*set_blk_dirty)(struct de_snr_private *priv,
		u32 blk_id, u32 dirty);

};

static struct de_snr_private snr_priv[DE_NUM][MAX_CHN_NUM];


static inline struct snr_reg *get_snr_reg(struct de_snr_private *priv)
{
	return (struct snr_reg *)(priv->reg_blks.vir_addr);
}

static void snr_set_block_dirty(
	struct de_snr_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks.dirty = dirty;
}

static void snr_set_rcq_head_dirty(
	struct de_snr_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks.rcq_hd) {
		priv->reg_blks.rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WRN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

s32 de_snr_set_para(u32 disp, u32 chn,
		    struct disp_layer_config_data **const pdata, u32 layer_num)
{
	struct de_snr_private *priv = &(snr_priv[disp][chn]);
	struct snr_reg *reg = get_snr_reg(priv);
	s32 ret = -1;
	u32 i = 0;
	struct disp_layer_info_inner *lay_info = NULL;

	/*this chn is not support*/
	if (!de_feat_is_support_snr_by_chn(disp, chn))
		goto OUT;

	for (i = 0; i < layer_num; ++i) {
		if (pdata[i]->config.channel == chn &&
		  pdata[i]->config.info.snr.en) {
			lay_info = &(pdata[i]->config.info);
			break;
		}
	}

	if (!lay_info) {
		reg->snr_ctrl.bits.en = 0;
		reg->snr_ctrl.bits.demo_en = 0;
		goto DIRTY;
	}

	if (lay_info->fb.format != DISP_FORMAT_YUV422_SP_UVUV &&
	    lay_info->fb.format != DISP_FORMAT_YUV422_SP_VUVU &&
	    lay_info->fb.format != DISP_FORMAT_YUV420_SP_UVUV &&
	    lay_info->fb.format != DISP_FORMAT_YUV420_SP_VUVU &&
	    lay_info->fb.format != DISP_FORMAT_YUV422_P &&
	    lay_info->fb.format != DISP_FORMAT_YUV420_P &&
	    lay_info->fb.format != DISP_FORMAT_YUV422_SP_UVUV_10BIT &&
	    lay_info->fb.format != DISP_FORMAT_YUV422_SP_VUVU_10BIT &&
	    lay_info->fb.format != DISP_FORMAT_YUV420_SP_UVUV_10BIT &&
	    lay_info->fb.format != DISP_FORMAT_YUV420_SP_VUVU_10BIT) {
		reg->snr_ctrl.bits.en = 0;
		reg->snr_ctrl.bits.demo_en = 0;
		goto DIRTY;
	}

	reg->snr_ctrl.bits.en = lay_info->snr.en;
	reg->snr_ctrl.bits.demo_en = lay_info->snr.demo_en;
	reg->demo_win_hor.bits.demo_horz_start = lay_info->snr.demo_win.x;
	reg->demo_win_hor.bits.demo_horz_end =
	    lay_info->snr.demo_win.x + lay_info->snr.demo_win.width;
	reg->demo_win_ver.bits.demo_vert_start = lay_info->snr.demo_win.y;
	reg->demo_win_ver.bits.demo_vert_end =
	    lay_info->snr.demo_win.y + lay_info->snr.demo_win.height;
	reg->snr_strength.bits.strength_y = lay_info->snr.y_strength;
	reg->snr_strength.bits.strength_u = lay_info->snr.u_strength;
	reg->snr_strength.bits.strength_v = lay_info->snr.v_strength;
	reg->snr_line_det.bits.th_hor_line =
	    (lay_info->snr.th_hor_line) ? lay_info->snr.th_hor_line : 0xa;
	reg->snr_line_det.bits.th_ver_line =
	    (lay_info->snr.th_ver_line) ? lay_info->snr.th_hor_line : 0xa;

	if (lay_info->b_trd_out) {
		uint32_t buffer_flags = lay_info->fb.flags;
		long long w = lay_info->fb.crop.width  >> 32;
		long long h = lay_info->fb.crop.height >> 32;

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
		reg->snr_size.bits.width = (lay_info->fb.crop.width >> 32) - 1;
		reg->snr_size.bits.height = (lay_info->fb.crop.height >> 32) - 1;
	}

DIRTY:
	priv->set_blk_dirty(priv, 0, 1);
	ret = 0;
OUT:
	return ret;

}

s32 de_snr_init(u32 disp, u8 __iomem *de_reg_base)
{
	u32 chn_num = 0, chn = 0, phy_chn = 0, rcq_used = 0;
	s32 ret = -1;
	u8 __iomem *reg_base = NULL;
	struct de_snr_private *priv = NULL;
	struct de_reg_mem_info *reg_mem_info = NULL;
	struct de_reg_block *block = NULL;

	chn_num = de_feat_get_num_chns(disp);
	for (chn = 0; chn < chn_num; ++chn) {
		if (!de_feat_is_support_snr_by_chn(disp, chn))
			continue;

		phy_chn = de_feat_get_phy_chn_id(disp, chn);
		reg_base = (u8 __iomem *)(de_reg_base
			+ DE_CHN_OFFSET(phy_chn) + CHN_SNR_OFFSET);
		rcq_used = de_feat_is_using_rcq(disp);

		priv = &snr_priv[disp][chn];
		if (!priv) {
			DE_WRN("Allocate snr private fail!\n");
			goto OUT;
		}
		reg_mem_info = &(priv->reg_mem_info);

		reg_mem_info->size = sizeof(struct snr_reg);
		reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
		    reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
		    rcq_used);
		if (NULL == reg_mem_info->vir_addr) {
			DE_WRN("alloc ovl[%d][%d] mm fail!size=0x%x\n",
			       disp, chn, reg_mem_info->size);
			goto OUT;
		}
		block = &(priv->reg_blks);
		block->phy_addr = reg_mem_info->phy_addr;
		block->vir_addr = reg_mem_info->vir_addr;
		block->size = reg_mem_info->size;
		block->reg_addr = reg_base;
		priv->reg_blk_num = 1;

		if (rcq_used)
			priv->set_blk_dirty = snr_set_rcq_head_dirty;
		else
			priv->set_blk_dirty = snr_set_block_dirty;

	}
	ret = 0;
OUT:
	return ret;
}

s32 de_snr_get_reg_blocks(u32 disp,
	struct de_reg_block **blks, u32 *blk_num)
{
	u32 chn_num, chn;
	u32 total = 0;

	chn_num = de_feat_get_num_chns(disp);

	if (blks == NULL) {
		for (chn = 0; chn < MAX_CHN_NUM; ++chn)
			total += snr_priv[disp][chn].reg_blk_num;
		*blk_num = total;
		return 0;
	}
	for (chn = 0; chn < MAX_CHN_NUM; ++chn) {
		struct de_snr_private *priv = &(snr_priv[disp][chn]);
		struct de_reg_block *blk_begin, *blk_end;
		u32 num;
		if (*blk_num >= priv->reg_blk_num) {
			num = priv->reg_blk_num;
		} else {
			DE_WRN("should not happen\n");
			num = *blk_num;
		}
		blk_begin = &priv->reg_blks;
		blk_end = blk_begin + num;
		for (; blk_begin != blk_end; ++blk_begin)
			*blks++ = blk_begin;
		total += num;
		*blk_num -= num;
	}

	*blk_num = total;
	return 0;
}

s32 de_snr_exit(u32 disp)
{
	u32 i = 0;

	for (i = 0; i < MAX_CHN_NUM; ++i) {
		struct de_snr_private *priv = &(snr_priv[disp][i]);
		struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
		if (reg_mem_info->vir_addr != NULL)
			de_top_reg_memory_free(reg_mem_info->vir_addr,
					       reg_mem_info->phy_addr,
					       reg_mem_info->size);
	}
	return 0;
}
