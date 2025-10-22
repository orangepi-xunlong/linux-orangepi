/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2022 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "de_tfbd_type.h"

#include "../../include.h"
#include "de_feat.h"
#include "de_top.h"
#include "de_rtmx.h"
#include "de_tfbd.h"

enum {
	OVL_TFBD_REG_BLK = 0,
	OVL_TFBD_REG_BLK_NUM,
};

struct de_tfbd_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[OVL_TFBD_REG_BLK_NUM];

	void (*set_blk_dirty)(struct de_tfbd_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_tfbd_private tfbd_priv[DE_NUM][MAX_CHN_NUM];

static inline struct ovl_v_tfbd_reg *get_tfbd_reg(struct de_tfbd_private *priv)
{
	return (struct ovl_v_tfbd_reg *)(priv->reg_blks[0].vir_addr);
}

static void tfbd_set_block_dirty(
	struct de_tfbd_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void tfbd_set_rcq_head_dirty(
	struct de_tfbd_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WARN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

static u32 get_fmt_seq(enum disp_pixel_format format)
{
	u32 seq;
	switch (format) {
	case DISP_FORMAT_ARGB_8888:
	case DISP_FORMAT_XRGB_8888:
	case DISP_FORMAT_A2R10G10B10:
	case DISP_FORMAT_ARGB_4444:
	case DISP_FORMAT_RGB_888:
	case DISP_FORMAT_RGB_565:
		seq = 0x2103;
		break;
	case DISP_FORMAT_ABGR_8888:
	case DISP_FORMAT_XBGR_8888:
	case DISP_FORMAT_A2B10G10R10:
	case DISP_FORMAT_ABGR_4444:
	case DISP_FORMAT_BGR_888:
	case DISP_FORMAT_BGR_565:
		seq = 0x0123;
		break;
	case DISP_FORMAT_RGBA_8888:
	case DISP_FORMAT_RGBX_8888:
	case DISP_FORMAT_R10G10B10A2:
	case DISP_FORMAT_RGBA_4444:
		seq = 0x3210;
		break;

	case DISP_FORMAT_BGRA_8888:
	case DISP_FORMAT_BGRX_8888:
	case DISP_FORMAT_B10G10R10A2:
	case DISP_FORMAT_BGRA_4444:
		seq = 0x1230;
		break;

	case DISP_FORMAT_YUV420_SP_UVUV:
	case DISP_FORMAT_YUV420_SP_UVUV_10BIT:
	case DISP_FORMAT_YUV422_SP_UVUV_10BIT:
		seq = 0x3210;
		break;
	case DISP_FORMAT_YUV420_SP_VUVU:
	case DISP_FORMAT_YUV420_SP_VUVU_10BIT:
	case DISP_FORMAT_YUV422_SP_VUVU_10BIT:
		seq = 0x13210;
		break;
	default:
		seq = 0x3210;
		DE_WARN("unsupported format=%x\n", format);
		break;
	}
	return seq;
}

static u32 to_hw_fmt(enum disp_pixel_format format)
{
	u32 fmt;
	switch (format) {
	case DISP_FORMAT_ARGB_8888:
	case DISP_FORMAT_ABGR_8888:
	case DISP_FORMAT_RGBA_8888:
	case DISP_FORMAT_BGRA_8888:
	case DISP_FORMAT_XRGB_8888:
	case DISP_FORMAT_XBGR_8888:
	case DISP_FORMAT_RGBX_8888:
	case DISP_FORMAT_BGRX_8888:
		fmt = 0x0c;
		break;
	case DISP_FORMAT_A2R10G10B10:
	case DISP_FORMAT_A2B10G10R10:
	case DISP_FORMAT_R10G10B10A2:
	case DISP_FORMAT_B10G10R10A2:
		fmt = 0x0e;
		break;
	case DISP_FORMAT_ARGB_4444:
	case DISP_FORMAT_ABGR_4444:
	case DISP_FORMAT_RGBA_4444:
	case DISP_FORMAT_BGRA_4444:
		fmt = 0x02;
		break;
	case DISP_FORMAT_RGB_888:
	case DISP_FORMAT_BGR_888:
		fmt = 0x3a;
		break;
	case DISP_FORMAT_RGB_565:
	case DISP_FORMAT_BGR_565:
		fmt = 0x05;
		break;
	case DISP_FORMAT_RGBA_5551:
	case DISP_FORMAT_BGRA_5551:
		fmt = 0x53;
		break;
	case DISP_FORMAT_YUV420_SP_UVUV:
	case DISP_FORMAT_YUV420_SP_VUVU:
		fmt = 0x36;
		break;
	case DISP_FORMAT_YUV420_SP_UVUV_10BIT:
	case DISP_FORMAT_YUV420_SP_VUVU_10BIT:
		fmt = 0x65;
		break;
	case DISP_FORMAT_YUV422_SP_UVUV_10BIT:
	case DISP_FORMAT_YUV422_SP_VUVU_10BIT:
		fmt = 0x64;
		break;
	//TODO not disp2 formt match 0x63 0x66
	default:
		fmt = 0x63;//fixme
		DE_WARN("unsupported format=%x\n", format);
		break;
	}
	return fmt;
}

static u32 get_y_size(u32 width, u32 height, enum disp_pixel_format fmt)
{
	u32 size = 0;
	switch (fmt) {
	case DISP_FORMAT_YUV420_SP_UVUV:
	case DISP_FORMAT_YUV420_SP_VUVU:
		size = width * height;
		break;
	case DISP_FORMAT_YUV420_SP_UVUV_10BIT:
	case DISP_FORMAT_YUV420_SP_VUVU_10BIT:
	case DISP_FORMAT_YUV422_SP_UVUV_10BIT:
	case DISP_FORMAT_YUV422_SP_VUVU_10BIT:
		size = width * height * 2;
		break;
	default:
		DE_WARN("unsupported format=%x\n", fmt);
		break;
	}
	return size;
}

static u32 to_hw_lossy_rate(u32 lossy_rate)
{
	u32 ret;
	switch (lossy_rate) {
	case 0:
		ret = 0;
		break;
	case 25:
		ret = 1;
		break;
	case 50:
		ret = 2;
		break;
	case 75:
		ret = 3;
		break;
	default:
		DE_WARN("unsupported tfbc lossy_rate=%d\n", lossy_rate);
		ret = 0;
		break;
	}
	return ret;
}

s32 de_tfbd_apply_lay(u32 disp, u32 chn,
	struct disp_layer_config_data **const pdata, u32 layer_num)
{
	u32 tmp;
	u32 dwval;
	u32 width, height, x, y;
	u64 addr;
	u64 y_addr;
	u64 uv_addr = 0;
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_chn_info *chn_info = ctx->chn_info + chn;
	struct de_tfbd_private *priv = &(tfbd_priv[disp][chn]);
	struct ovl_v_tfbd_reg *reg = get_tfbd_reg(priv);
	struct disp_layer_info_inner *lay_info =
	    &(pdata[0]->config.info);

	dwval = 1;
	dwval |= ((lay_info->mode == LAYER_MODE_BUFFER) ? 0 : (1 << 1));
	dwval |= ((lay_info->alpha_mode & 0x3) << 2);
	tmp = to_hw_fmt(lay_info->fb.format) & 0x7F;
/*	if (tmp == 0xFFFFFFFF)
		return -1;*/
//fixme for test
//	tmp = 0x63;

	dwval |= tmp << 8;
	dwval |= (lay_info->alpha_value << 24);

	dwval |= (to_hw_lossy_rate(lay_info->fb.tfbc_info.lossy_rate) & 0x3) << 5;

	reg->ctrl.dwval = dwval;

	width = lay_info->fb.size[0].width;
	height = lay_info->fb.size[0].height;
	dwval = ((width ? (width - 1) : 0) & 0x1FFF) |
		(height ? (((height - 1) & 0x1FFF) << 16) : 0);
	reg->src_size.dwval = dwval;

	x = (u32)(lay_info->fb.crop.x >> 32);
	y = (u32)(lay_info->fb.crop.y >> 32);
	dwval = (x & 0x1FFF) | ((y & 0x1FFF) << 16);
	reg->crop_coor.dwval = dwval;

	width = lay_info->fb.crop.width >> 32;
	height = lay_info->fb.crop.height >> 32;
	dwval = ((width ? (width - 1) : 0) & 0x1FFF) |
		(height ? (((height - 1) & 0x1FFF) << 16) : 0);
	reg->crop_size.dwval = dwval;

	width = lay_info->fb.size[0].width;
	height = lay_info->fb.size[0].height;
	addr = lay_info->fb.tfbc_info.is_gpu_header ?
		  lay_info->fb.addr[0] + 64 : lay_info->fb.addr[0];
	y_addr = addr;
	y_addr += width * height / 8;
	y_addr = DISPALIGN(y_addr, 64);
	if (lay_info->fb.format >= DISP_FORMAT_YUV444_I_AYUV) {
		uv_addr = y_addr + get_y_size(width, height, lay_info->fb.format);
	}

	reg->y_laddr.dwval = (u32)(y_addr);
	reg->uv_laddr.dwval = (u32)(uv_addr);
	reg->haddr.dwval = (u32)(((y_addr >> 32) & 0xFF) | (((uv_addr >> 32) & 0xFF) << 8));

	width = chn_info->ovl_win.width;
	height = chn_info->ovl_win.height;
	dwval = (width ? ((width - 1) & 0x1FFF) : 0) |
		(height ? (((height - 1) & 0x1FFF) << 16) : 0);
	reg->ovl_size.dwval = dwval;

	x = chn_info->ovl_win.left;
	y = chn_info->ovl_win.top;
	dwval = (x & 0x1FFF) | ((y & 0x1FFF) << 16);
	reg->ovl_coor.dwval = dwval;

	reg->fc.dwval =
	    ((lay_info->mode == LAYER_MODE_BUFFER) ? 0 : lay_info->color);

	reg->bgc.dwval = 0xFF000000;

	dwval = get_fmt_seq(lay_info->fb.format);
/*	if (tmp == 0xFFFFFFFF)
		return -1;*/
//fixme for test
//	dwval = 0x12013;
	reg->fmt_seq.dwval = dwval;

	priv->set_blk_dirty(priv, OVL_TFBD_REG_BLK, 1);

	print_hex_dump(KERN_DEBUG, "\t", DUMP_PREFIX_OFFSET, 16, 4, &reg->ctrl.dwval, 52, 0);
	return 0;
}

s32 de_tfbd_disable(u32 disp, u32 chn)
{
	struct de_tfbd_private *priv = &(tfbd_priv[disp][chn]);
	struct ovl_v_tfbd_reg *reg = get_tfbd_reg(priv);
	reg->ctrl.dwval = 0;
	priv->set_blk_dirty(priv, OVL_TFBD_REG_BLK, 1);
	return 0;
}

s32 de_tfbd_init(u32 disp, u8 __iomem *de_reg_base)
{
	u32 chn_num, chn;

	chn_num = de_feat_get_num_chns(disp);
	for (chn = 0; chn < chn_num; ++chn) {

		u32 phy_chn = de_feat_get_phy_chn_id(disp, chn);
		u8 __iomem *reg_base =
		    (u8 __iomem *)(de_reg_base + DE_CHN_OFFSET(phy_chn) +
				   CHN_TFBD_OFFSET);
		u32 rcq_used = de_feat_is_using_rcq(disp);

		struct de_tfbd_private *priv = &tfbd_priv[disp][chn];
		struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
		struct de_reg_block *block;

		if (!de_feat_is_support_tfbd_by_layer(disp, chn, 0))
			continue;

		reg_mem_info->size = sizeof(struct ovl_v_tfbd_reg);
		reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
		    reg_mem_info->size,
		    (void *)&(reg_mem_info->phy_addr), rcq_used);
		if (NULL == reg_mem_info->vir_addr) {
			DE_WARN("alloc tfbd[%d][%d] mm fail!size=0x%x\n",
			       disp, chn, reg_mem_info->size);
			return -1;
		}

		block = &(priv->reg_blks[OVL_TFBD_REG_BLK]);
		block->phy_addr = reg_mem_info->phy_addr;
		block->vir_addr = reg_mem_info->vir_addr;
		block->size = sizeof(struct ovl_v_tfbd_reg);
		block->reg_addr = reg_base;

		priv->reg_blk_num = OVL_TFBD_REG_BLK_NUM;

		if (rcq_used)
			priv->set_blk_dirty = tfbd_set_rcq_head_dirty;
		else
			priv->set_blk_dirty = tfbd_set_block_dirty;
	}

	return 0;
}

s32 de_tfbd_exit(u32 disp)
{
	u32 chn_num, chn;

	chn_num = de_feat_get_num_chns(disp);
	for (chn = 0; chn < chn_num; ++chn) {
		struct de_tfbd_private *priv = &tfbd_priv[disp][chn];
		struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);

		if (reg_mem_info->vir_addr != NULL)
			de_top_reg_memory_free(reg_mem_info->vir_addr,
					       reg_mem_info->phy_addr,
					       reg_mem_info->size);
	}

	return 0;
}

s32 de_tfbd_get_reg_blocks(u32 disp, struct de_reg_block **blks,
			      u32 *blk_num)
{
	u32 chn_num, chn;
	u32 total = 0;

	chn_num = de_feat_get_num_chns(disp);

	if (blks == NULL) {
		for (chn = 0; chn < chn_num; ++chn)
			total += tfbd_priv[disp][chn].reg_blk_num;
		*blk_num = total;
		return 0;
	}

	for (chn = 0; chn < chn_num; ++chn) {
		struct de_tfbd_private *priv = &(tfbd_priv[disp][chn]);
		struct de_reg_block *blk_begin, *blk_end;
		u32 num;

		if (*blk_num >= priv->reg_blk_num) {
			num = priv->reg_blk_num;
		} else {
			DE_WARN("should not happen\n");
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
