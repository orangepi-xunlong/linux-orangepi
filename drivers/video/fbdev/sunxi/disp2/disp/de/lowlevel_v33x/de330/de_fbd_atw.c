/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "de_fbd_atw_type.h"

#include "../../include.h"
#include "de_feat.h"
#include "de_top.h"
#include "de_fbd_atw.h"
#include "de_rtmx.h"

enum {
	FBD_ATW_V_REG_BLK_FBD = 0,
	FBD_ATW_V_REG_BLK_ATW,
	FBD_ATW_V_REG_BLK_NUM,

	FBD_ATW_MAX_REG_BLK_NUM = FBD_ATW_V_REG_BLK_NUM,
};

enum {
	FBD_U_REG_BLK_FBD = 0,
	FBD_U_REG_BLK_NUM,
};

struct de_fbd_atw_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	union {
		struct de_reg_block fbd_atw_v_blks[FBD_ATW_V_REG_BLK_NUM];
		struct de_reg_block fbd_u_blks[FBD_U_REG_BLK_NUM];
		struct de_reg_block reg_blks[FBD_ATW_MAX_REG_BLK_NUM];
	};
	enum fbd_atw_type type;

	void (*set_blk_dirty)(struct de_fbd_atw_private *priv,
		u32 blk_id, u32 dirty);

};

static struct de_fbd_atw_private fbd_atw_priv[DE_NUM][MAX_CHN_NUM];

static inline struct fbd_atw_v_reg *get_fbd_atw_v_reg(
	struct de_fbd_atw_private *priv)
{
	return (struct fbd_atw_v_reg *)
		(priv->fbd_atw_v_blks[0].vir_addr);
}

static inline struct fbd_u_reg *get_fbd_u_reg(
	struct de_fbd_atw_private *priv)
{
	return (struct fbd_u_reg *)
		(priv->fbd_u_blks[0].vir_addr);
}

static void fbd_atw_set_block_dirty(
	struct de_fbd_atw_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void fbd_atw_set_rcq_head_dirty(
	struct de_fbd_atw_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WRN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

static s32 de_fbd_get_info(struct afbc_header *hd,
	struct de_fbd_info *info)
{
	u32 inputbits;

	inputbits = hd->inputbits[0]
		| (hd->inputbits[1] << 8)
		| (hd->inputbits[2] << 16)
		| (hd->inputbits[3] << 24);

	switch (hd->header_layout) {
	case 0:
		if (inputbits == 0x08080808) {
			info->fmt = FBD_RGBA8888;
			info->sbs[0] = 1;
			info->sbs[1] = 1;
			info->compbits[0] = 8;
			info->compbits[1] = 9;
			info->compbits[2] = 9;
			info->compbits[3] = 8;
			info->yuv_tran = 1;
		} else if ((inputbits & 0xFFFFFF) == 0x080808) {
			info->fmt = FBD_RGB888;
			info->sbs[0] = 1;
			info->sbs[1] = 1;
			info->compbits[0] = 8;
			info->compbits[1] = 9;
			info->compbits[2] = 9;
			info->compbits[3] = 0;
			info->yuv_tran = 1;
		} else if ((inputbits & 0xFFFFFF) == 0x050605) {
			info->fmt = FBD_RGB565;
			info->sbs[0] = 1;
			info->sbs[1] = 1;
			info->compbits[0] = 6;
			info->compbits[1] = 7;
			info->compbits[2] = 7;
			info->compbits[3] = 0;
			info->yuv_tran = 1;
		} else if (inputbits == 0x04040404) {
			info->fmt = FBD_RGBA4444;
			info->sbs[0] = 1;
			info->sbs[1] = 1;
			info->compbits[0] = 4;
			info->compbits[1] = 5;
			info->compbits[2] = 5;
			info->compbits[3] = 4;
			info->yuv_tran = 1;
		} else if (inputbits == 0x01050505) {
			info->fmt = FBD_RGBA5551;
			info->sbs[0] = 1;
			info->sbs[1] = 1;
			info->compbits[0] = 5;
			info->compbits[1] = 6;
			info->compbits[2] = 6;
			info->compbits[3] = 1;
			info->yuv_tran = 1;
		} else if (inputbits == 0x020A0A0A) {
			info->fmt = FBD_RGBA1010102;
			info->sbs[0] = 1;
			info->sbs[1] = 1;
			info->compbits[0] = 10;
			info->compbits[1] = 11;
			info->compbits[2] = 11;
			info->compbits[3] = 2;
			info->yuv_tran = 1;
		} else if ((inputbits & 0xFFFFFF) == 0x0A0A0A) {
			info->fmt = FBD_YUV420B10;
			info->sbs[0] = 1;
			info->sbs[1] = 2;
			info->compbits[0] = 10;
			info->compbits[1] = 10;
			info->compbits[2] = 10;
			info->compbits[3] = 0;
			info->yuv_tran = 0;
		}
		break;
	case 1:
		if ((inputbits & 0xFFFFFF) == 0x080808) {
			info->fmt = FBD_YUV420;
			info->sbs[0] = 1;
			info->sbs[1] = 1;
			info->compbits[0] = 8;
			info->compbits[1] = 8;
			info->compbits[2] = 8;
			info->compbits[3] = 0;
			info->yuv_tran = 0;
		} else if ((inputbits & 0xFFFFFF) == 0x0A0A0A) {
			info->fmt = FBD_YUV420B10;
			info->sbs[0] = 1;
			info->sbs[1] = 2;
			info->compbits[0] = 10;
			info->compbits[1] = 10;
			info->compbits[2] = 10;
			info->compbits[3] = 0;
			info->yuv_tran = 0;
		}
		break;
	case 2:
		if ((inputbits & 0xFFFFFF) == 0x080808) {
			info->fmt = FBD_YUV422;
			info->sbs[0] = 1;
			info->sbs[1] = 2;
			info->compbits[0] = 8;
			info->compbits[1] = 8;
			info->compbits[2] = 8;
			info->compbits[3] = 0;
			info->yuv_tran = 0;
		} else if ((inputbits & 0xFFFFFF) == 0x0A0A0A) {
			info->fmt = FBD_YUV422B10;
			info->sbs[0] = 2;
			info->sbs[1] = 3;
			info->compbits[0] = 10;
			info->compbits[1] = 10;
			info->compbits[2] = 10;
			info->compbits[3] = 0;
			info->yuv_tran = 0;
		}
		break;
	default:
		info->fmt = 0;
		info->sbs[0] = 1;
		info->sbs[1] = 1;
		info->compbits[0] = 8;
		info->compbits[1] = 8;
		info->compbits[2] = 8;
		info->compbits[3] = 8;
		info->yuv_tran = 1;
		DE_WRN("no support afbd header layout %d\n", hd->header_layout);
		break;
	}
	return 0;
}

static s32 de_fbd_v_apply_lay(u32 disp, u32 chn,
	struct disp_layer_config_data **const pdata, u32 layer_num)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_chn_info *chn_info = ctx->chn_info + chn;
	struct de_fbd_atw_private *priv = &(fbd_atw_priv[disp][chn]);
	struct fbd_atw_v_reg *reg = get_fbd_atw_v_reg(priv);

	struct disp_layer_info_inner *lay_info =
		&(pdata[0]->config.info); /*TODO:BUG*/
	u32 dwval;
	u32 width, height, left, top;
	struct de_fbd_info fbd_info;

	if (lay_info->fb.p_afbc_header == NULL) {
		DE_WRN("[%s]:afbc_header is NULL!\n", __func__);
		return -1;
	}
	de_fbd_get_info(lay_info->fb.p_afbc_header, &fbd_info);

	dwval = 1 | (1 << 4);
	dwval |= ((lay_info->mode == LAYER_MODE_BUFFER) ? 0 : (1 << 1));
	dwval |= ((lay_info->alpha_mode & 0x3) << 2);
	dwval |= (lay_info->alpha_value << 24);
	reg->fbd_ctl.dwval = dwval;

	width = lay_info->fb.crop.width >> 32;
	height = lay_info->fb.crop.height >> 32;
	dwval = ((width ? (width - 1) : 0) & 0xFFF)
		| (height ? (((height - 1) & 0xFFF) << 16) : 0);
	reg->fbd_img_size.dwval = dwval;

	width = lay_info->fb.p_afbc_header->block_width;
	height = lay_info->fb.p_afbc_header->block_height;
	dwval = (width & 0x3FF) | ((height & 0x3FF) << 16);
	reg->fbd_blk_size.dwval = dwval;

	left = lay_info->fb.p_afbc_header->left_crop;
	top = lay_info->fb.p_afbc_header->top_crop;
	dwval = (left & 0xF) | ((top & 0xF) << 16);
	reg->fbd_src_crop.dwval = dwval;

	left = (u32)(lay_info->fb.crop.x >> 32);
	top = (u32)(lay_info->fb.crop.y >> 32);
	dwval = (left & 0xFFF) | ((top & 0xFFF) << 16);
	reg->fbd_lay_crop.dwval = dwval;

	dwval = (fbd_info.fmt & 0x7F)
		| ((fbd_info.yuv_tran & 0x1) << 7)
		| ((fbd_info.sbs[0] & 0x3) << 16)
		| ((fbd_info.sbs[1] & 0x3) << 18);
	reg->fbd_fmt.dwval = dwval;

	reg->fbd_hd_laddr.dwval = (u32)(lay_info->fb.addr[0]);
	reg->fbd_hd_haddr.dwval = (u32)(lay_info->fb.addr[0] >> 32) & 0xFF;

	width = chn_info->ovl_win.width;
	height = chn_info->ovl_win.height;
	dwval = (width ? ((width - 1) & 0x1FFF) : 0)
		| (height ? (((height - 1) & 0x1FFF) << 16) : 0);
	reg->fbd_ovl_size.dwval = dwval;

	left = chn_info->ovl_win.left;
	top = chn_info->ovl_win.top;
	dwval = (left & 0x1FFF) | ((top & 0x1FFF) << 16);
	reg->fbd_ovl_coor.dwval = dwval;

	reg->fbd_bg_color.dwval = 0xFF000000;

	reg->fbd_fcolor.dwval =
		((lay_info->mode == LAYER_MODE_BUFFER) ? 0 : lay_info->color);

	dwval = (((1 << fbd_info.compbits[0]) - 1) & 0xFFFF)
		| ((((1 << fbd_info.compbits[3]) - 1) & 0xFFFF) << 16);
	reg->fbd_color0.dwval = dwval;
	dwval = (((1 << fbd_info.compbits[2]) - 1) & 0xFFFF)
		| ((((1 << fbd_info.compbits[1]) - 1) & 0xFFFF) << 16);
	reg->fbd_color1.dwval = dwval;

	priv->set_blk_dirty(priv, FBD_ATW_V_REG_BLK_FBD, 1);

	return 0;
}

static s32 de_atw_v_apply_lay(u32 disp, u32 chn,
	struct disp_layer_config_data **const pdata, u32 layer_num)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_chn_info *chn_info = ctx->chn_info + chn;
	struct de_fbd_atw_private *priv = &(fbd_atw_priv[disp][chn]);
	struct fbd_atw_v_reg *reg = get_fbd_atw_v_reg(priv);

	struct disp_layer_info_inner *lay_info =
		&(pdata[0]->config.info);
	u32 dwval;
	u32 width, height;
	u32 b_col, b_row;

	if (!chn_info->atw_en) {
		reg->atw_attr.dwval = 0;
		priv->set_blk_dirty(priv, FBD_ATW_V_REG_BLK_ATW, 1);
		return 0;
	}

	reg->atw_rotate.dwval = lay_info->transform & 0x7;
	priv->set_blk_dirty(priv, FBD_ATW_V_REG_BLK_FBD, 1);

	dwval = 0x3;
	dwval |= ((lay_info->atw.mode & 0xF) << 4);
	reg->atw_attr.dwval = dwval;

	width = (u32)(lay_info->fb.crop.width >> 32);
	height = (u32)(lay_info->fb.crop.height >> 32);
	dwval = (width > height) ? width : height;
	if (dwval >= 0x800)
		dwval = 2;
	else if (dwval > 0x400)
		dwval = 3;
	else if (dwval > 0x200)
		dwval = 4;
	else
		dwval = 8;
	reg->atw_data_bit.dwval = dwval & 0xF;

	b_col = lay_info->atw.b_col;
	b_col = (lay_info->atw.mode == LEFT_RIGHT_MODE) ? (b_col << 1) : b_col;
	dwval = ((width << 8) / b_col) & 0xFFFF;
	b_row = lay_info->atw.b_row;
	b_row = (lay_info->atw.mode == UP_DOWN_MODE) ? (b_row << 1) : b_row;
	dwval |= ((((height << 8) / b_row) & 0xFFFF) << 16);
	reg->atw_blk_step.dwval = dwval;

	width = chn_info->ovl_out_win.width;
	height = chn_info->ovl_out_win.height;
	dwval = (width ? ((width - 1) & 0x1FFF) : 0)
		| (height ? (((height - 1) & 0x1FFF) << 16) : 0);
	reg->atw_out_size.dwval = dwval;

	reg->atw_coeff_laddr.dwval = (u32)(lay_info->atw.cof_addr);
	reg->atw_coeff_haddr.dwval = (u32)(lay_info->atw.cof_addr >> 32);

	dwval = (lay_info->atw.b_col & 0xFF)
		| ((lay_info->atw.b_row & 0xFF) << 8);
	reg->atw_blk_number.dwval = dwval;

	priv->set_blk_dirty(priv, FBD_ATW_V_REG_BLK_ATW, 1);

	return 0;
}

s32 de_fbd_atw_apply_lay(u32 disp, u32 chn,
	struct disp_layer_config_data **const pdata, u32 layer_num)
{
	struct de_fbd_atw_private *priv = &(fbd_atw_priv[disp][chn]);

	if (priv->type == FBD_ATW_TYPE_VI) {
		de_fbd_v_apply_lay(disp, chn, pdata, layer_num);
		de_atw_v_apply_lay(disp, chn, pdata, layer_num);
	} else if (priv->type == FBD_TYPE_UI) {
		return de_fbd_v_apply_lay(disp, chn, pdata, layer_num);
	}
	return 0;
}

s32 de_fbd_atw_disable(u32 disp, u32 chn)
{
	struct de_fbd_atw_private *priv = &(fbd_atw_priv[disp][chn]);

	if (priv->type == FBD_ATW_TYPE_VI) {
		struct fbd_atw_v_reg *reg = get_fbd_atw_v_reg(priv);

		reg->fbd_ctl.dwval = 0;
		reg->atw_attr.dwval = 0;
		priv->set_blk_dirty(priv, FBD_ATW_V_REG_BLK_FBD, 1);
		priv->set_blk_dirty(priv, FBD_ATW_V_REG_BLK_ATW, 1);
	} else if (priv->type == FBD_TYPE_UI) {
		struct fbd_u_reg *reg = get_fbd_u_reg(priv);

		reg->fbd_ctl.dwval = 0;
		priv->set_blk_dirty(priv, FBD_U_REG_BLK_FBD, 1);
	}
	return 0;
}

s32 de_fbd_atw_init(u32 disp, u8 __iomem *de_reg_base)
{
	u32 chn_num, chn;
	u32 vi_chn_num = de_feat_get_num_vi_chns(disp);

	chn_num = de_feat_get_num_chns(disp);
	for (chn = 0; chn < chn_num; ++chn) {

		u32 phy_chn = de_feat_get_phy_chn_id(disp, chn);
		u8 __iomem *reg_base = (u8 __iomem *)(de_reg_base
			+ DE_CHN_OFFSET(phy_chn) + CHN_FBD_ATW_OFFSET);
		u32 rcq_used = de_feat_is_using_rcq(disp);

		struct de_fbd_atw_private *priv = &fbd_atw_priv[disp][chn];
		struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
		struct de_reg_block *block;

		if (!de_feat_is_support_fbd_by_layer(disp, chn, 0))
			continue;

		if (chn < vi_chn_num) {
			priv->type = FBD_ATW_TYPE_VI;
			reg_mem_info->size = sizeof(struct fbd_atw_v_reg);
			reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
				reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
				rcq_used);
			if (NULL == reg_mem_info->vir_addr) {
				DE_WRN("alloc ovl[%d][%d] mm fail!size=0x%x\n",
					 disp, chn, reg_mem_info->size);
				return -1;
			}

			block = &(priv->fbd_atw_v_blks[FBD_ATW_V_REG_BLK_FBD]);
			block->phy_addr = reg_mem_info->phy_addr;
			block->vir_addr = reg_mem_info->vir_addr;
			block->size = 0x5C;
			block->reg_addr = reg_base;

			block = &(priv->fbd_atw_v_blks[FBD_ATW_V_REG_BLK_ATW]);
			block->phy_addr = reg_mem_info->phy_addr + 0x100;
			block->vir_addr = reg_mem_info->vir_addr + 0x100;
			block->size = 0x40;
			block->reg_addr = reg_base + 0x100;

			priv->reg_blk_num = FBD_ATW_V_REG_BLK_NUM;
		} else {
			priv->type = FBD_TYPE_UI;
			reg_mem_info->size = sizeof(struct fbd_u_reg);
			reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
				reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
				rcq_used);
			if (NULL == reg_mem_info->vir_addr) {
				DE_WRN("alloc ovl[%d][%d] mm fail!size=0x%x\n",
					 disp, chn, reg_mem_info->size);
				return -1;
			}

			block = &(priv->fbd_u_blks[FBD_U_REG_BLK_FBD]);
			block->phy_addr = reg_mem_info->phy_addr;
			block->vir_addr = reg_mem_info->vir_addr;
			block->size = 0x58;
			block->reg_addr = reg_base;

			priv->reg_blk_num = FBD_U_REG_BLK_NUM;
		}

		if (rcq_used)
			priv->set_blk_dirty = fbd_atw_set_rcq_head_dirty;
		else
			priv->set_blk_dirty = fbd_atw_set_block_dirty;

	}

	return 0;
}

s32 de_fbd_atw_exit(u32 disp)
{
	u32 chn_num, chn;

	chn_num = de_feat_get_num_chns(disp);
	for (chn = 0; chn < chn_num; ++chn) {
		struct de_fbd_atw_private *priv = &fbd_atw_priv[disp][chn];
		struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);

		if (reg_mem_info->vir_addr != NULL)
			de_top_reg_memory_free(reg_mem_info->vir_addr,
				reg_mem_info->phy_addr, reg_mem_info->size);
	}

	return 0;
}

s32 de_fbd_atw_get_reg_blocks(u32 disp,
	struct de_reg_block **blks, u32 *blk_num)
{
	u32 chn_num, chn;
	u32 total = 0;

	chn_num = de_feat_get_num_chns(disp);

	if (blks == NULL) {
		for (chn = 0; chn < chn_num; ++chn)
			total += fbd_atw_priv[disp][chn].reg_blk_num;
		*blk_num = total;
		return 0;
	}

	for (chn = 0; chn < chn_num; ++chn) {
		struct de_fbd_atw_private *priv = &(fbd_atw_priv[disp][chn]);
		struct de_reg_block *blk_begin, *blk_end;
		u32 num;

		if (*blk_num >= priv->reg_blk_num) {
			num = priv->reg_blk_num;
		} else {
			DE_WRN("should not happen\n");
			num = *blk_num;
		}
		blk_begin = priv->fbd_atw_v_blks;
		blk_end = blk_begin + num;
		for (; blk_begin != blk_end; ++blk_begin)
			*blks++ = blk_begin;
		total += num;
		*blk_num -= num;
	}
	*blk_num = total;
	return 0;
}
