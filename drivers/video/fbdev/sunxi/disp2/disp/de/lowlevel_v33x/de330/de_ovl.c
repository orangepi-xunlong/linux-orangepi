/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "de_ovl_type.h"

#include "../../include.h"
#include "de_feat.h"
#include "de_top.h"
#include "de_ovl.h"
#include "de_rtmx.h"

enum {
	OVL_V_REG_BLK_LAY_0_1 = 0,
	OVL_V_REG_BLK_LAY_2_3,
	OVL_V_REG_BLK_PARA,
	OVL_V_REG_BLK_NUM,
};

enum {
	OVL_U_REG_BLK_LAY_0 = 0,
	OVL_U_REG_BLK_LAY_1,
	OVL_U_REG_BLK_LAY_2,
	OVL_U_REG_BLK_LAY_3,
	OVL_U_REG_BLK_PARA,
	OVL_U_REG_BLK_DS,
	OVL_U_REG_BLK_NUM,

	OVL_MAX_REG_BLK_NUM = OVL_U_REG_BLK_NUM,
};

struct de_ovl_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	union {
		struct de_reg_block v_reg_blks[OVL_V_REG_BLK_NUM];
		struct de_reg_block u_reg_blks[OVL_U_REG_BLK_NUM];
		struct de_reg_block reg_blks[OVL_MAX_REG_BLK_NUM];
	};
	enum ovl_type type;

	void (*set_blk_dirty)(struct de_ovl_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_ovl_private ovl_priv[DE_NUM][MAX_CHN_NUM];

static inline struct ovl_v_reg *get_ovl_v_reg(
	struct de_ovl_private *priv)
{
	return (struct ovl_v_reg *)(priv->v_reg_blks[0].vir_addr);
}

static inline struct ovl_u_reg *get_ovl_u_reg(
	struct de_ovl_private *priv)
{
	return (struct ovl_u_reg *)(priv->u_reg_blks[0].vir_addr);
}

static inline struct ovl_v_reg *get_ovl_v_hw_reg(
		struct de_ovl_private *priv)
{
	return (struct ovl_v_reg *)(priv->v_reg_blks[0].reg_addr);
}

static inline struct ovl_u_reg *get_ovl_u_hw_reg(
		struct de_ovl_private *priv)
{
	return (struct ovl_u_reg *)(priv->u_reg_blks[0].reg_addr);
}

void de_ovl_flush_layer_address(u32 disp, u32 chn, u32 layer_id)
{
	struct de_ovl_private *priv = &(ovl_priv[disp][chn]);
	if (priv->type == OVL_TYPE_VI) {
		struct ovl_v_reg *reg_bk = get_ovl_v_reg(priv);
		struct ovl_v_reg *reg_hw = get_ovl_v_hw_reg(priv);
		u32 phyaddr = reg_bk->lay[layer_id].top_laddr0.dwval;
		reg_hw->lay[layer_id].top_laddr0.dwval = phyaddr;
		reg_hw->lay[layer_id].bot_laddr0.dwval = 0;
	} else {
		struct ovl_u_reg *reg_bk = get_ovl_u_reg(priv);
		struct ovl_u_reg *reg_hw = get_ovl_u_hw_reg(priv);
		u32 phyaddr = reg_bk->lay[layer_id].top_laddr.dwval;
		reg_hw->lay[layer_id].top_laddr.dwval = phyaddr;
		reg_hw->lay[layer_id].bot_laddr.dwval = 0;
	}
}

static void ovl_set_block_dirty(
	struct de_ovl_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void ovl_set_rcq_head_dirty(
	struct de_ovl_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WRN("rcq_head is null ! ovl_type=%d, blk_id=%d\n",
			priv->type, blk_id);
	}
}

static u32 de_ovl_convert_fmt(enum disp_pixel_format format,
	u32 *fmt, u32 *vi_ui_sel)
{
	switch (format) {
	case DE_FORMAT_YUV422_I_VYUY:
		*vi_ui_sel = 0x0;
		*fmt = 0x0;
		break;
	case DE_FORMAT_YUV422_I_YVYU:
		*vi_ui_sel = 0x0;
		*fmt = 0x1;
		break;
	case DE_FORMAT_YUV422_I_UYVY:
		*vi_ui_sel = 0x0;
		*fmt = 0x2;
		break;
	case DE_FORMAT_YUV422_I_YUYV:
		*vi_ui_sel = 0x0;
		*fmt = 0x3;
		break;
	case DE_FORMAT_YUV422_SP_UVUV:
		*vi_ui_sel = 0x0;
		*fmt = 0x4;
		break;
	case DE_FORMAT_YUV422_SP_VUVU:
		*vi_ui_sel = 0x0;
		*fmt = 0x5;
		break;
	case DE_FORMAT_YUV422_P:
		*vi_ui_sel = 0x0;
		*fmt = 0x6;
		break;
	case DE_FORMAT_YUV420_SP_UVUV:
		*vi_ui_sel = 0x0;
		*fmt = 0x8;
		break;
	case DE_FORMAT_YUV420_SP_VUVU:
		*vi_ui_sel = 0x0;
		*fmt = 0x9;
		break;
	case DE_FORMAT_YUV420_P:
		*vi_ui_sel = 0x0;
		*fmt = 0xA;
		break;
	case DE_FORMAT_YUV411_SP_UVUV:
		*vi_ui_sel = 0x0;
		*fmt = 0xC;
		break;
	case DE_FORMAT_YUV411_SP_VUVU:
		*vi_ui_sel = 0x0;
		*fmt = 0xD;
		break;
	case DE_FORMAT_YUV411_P:
		*vi_ui_sel = 0x0;
		*fmt = 0xE;
		break;
	case DE_FORMAT_YUV420_SP_UVUV_10BIT:
		*vi_ui_sel = 0x0;
		*fmt = 0x10;
		break;
	case DE_FORMAT_YUV420_SP_VUVU_10BIT:
		*vi_ui_sel = 0x0;
		*fmt = 0x11;
		break;
	case DE_FORMAT_YUV422_SP_UVUV_10BIT:
		*vi_ui_sel = 0x0;
		*fmt = 0x12;
		break;
	case DE_FORMAT_YUV422_SP_VUVU_10BIT:
		*vi_ui_sel = 0x0;
		*fmt = 0x13;
		break;
	case DE_FORMAT_YUV444_I_VUYA_10BIT:
		*vi_ui_sel = 0x0;
		*fmt = 0x14;
		break;
	case DE_FORMAT_YUV444_I_AYUV_10BIT:
		*vi_ui_sel = 0x0;
		*fmt = 0x15;
		break;
	case DE_FORMAT_YUV444_I_AYUV:
		*vi_ui_sel = 0x1;
		*fmt = 0x0;
		break;
	case DE_FORMAT_YUV444_I_VUYA:
		*vi_ui_sel = 0x1;
		*fmt = 0x3;
		break;
	default:
		*vi_ui_sel = 0x1;
		*fmt = format;
		break;
	}
	return 0;
}

static s32 de_ovl_v_set_lay_layout(struct ovl_v_reg *reg,
	struct disp_layer_config_inner *const lay_cfg)
{
	u32 bpp[3] = {0, 0, 0};
	u32 pitch[3] = {0, 0, 0};
	u32 x0, y0;
	u32 x1 = 0;
	u32 y1 = 0;
	u32 x2 = 0;
	u32 y2 = 0;
	u64 addr[3] = {0, 0, 0};

	x0 = lay_cfg->info.fb.crop.x >> 32;
	y0 = lay_cfg->info.fb.crop.y >> 32;

	if (lay_cfg->info.fb.format <= DISP_FORMAT_BGRX_8888) {
		bpp[0] = 32;
	} else if (lay_cfg->info.fb.format <= DISP_FORMAT_BGR_888) {
		bpp[0] = 24;
	} else if (lay_cfg->info.fb.format <= DISP_FORMAT_BGRA_5551) {
		bpp[0] = 16;
	} else if (lay_cfg->info.fb.format <= DISP_FORMAT_B10G10R10A2) {
		bpp[0] = 32;
	} else if (lay_cfg->info.fb.format <= DISP_FORMAT_YUV444_I_VUYA) {
		bpp[0] = 32;
	} else if (lay_cfg->info.fb.format <= DISP_FORMAT_YUV422_I_VYUY) {
		bpp[0] = 16;
	} else if (lay_cfg->info.fb.format == DISP_FORMAT_YUV422_P) {
		bpp[0] = 8;
		bpp[1] = 8;
		bpp[2] = 8;
		x2 = x1 = x0 / 2;
		y2 = y1 = y0;
	} else if (lay_cfg->info.fb.format == DISP_FORMAT_YUV420_P) {
		bpp[0] = 8;
		bpp[1] = 8;
		bpp[2] = 8;
		x2 = x1 = x0 / 2;
		y2 = y1 = y0 / 2;
	} else if (lay_cfg->info.fb.format == DISP_FORMAT_YUV411_P) {
		bpp[0] = 8;
		bpp[1] = 8;
		bpp[2] = 8;
		x2 = x1 = x0 / 4;
		y2 = y1 = y0;
	} else if (lay_cfg->info.fb.format <= DISP_FORMAT_YUV422_SP_VUVU) {
		bpp[0] = 8;
		bpp[1] = 16;
		x1 = x0 / 2;
		y1 = y0;
	} else if (lay_cfg->info.fb.format <= DISP_FORMAT_YUV420_SP_VUVU) {
		bpp[0] = 8;
		bpp[1] = 16;
		x1 = x0 / 2;
		y1 = y0 / 2;
	} else if (lay_cfg->info.fb.format <= DISP_FORMAT_YUV411_SP_VUVU) {
		bpp[0] = 8;
		bpp[1] = 16;
		x1 = x0 / 4;
		y1 = y0;
	} else if ((lay_cfg->info.fb.format == DISP_FORMAT_YUV420_SP_UVUV_10BIT) ||
		   (lay_cfg->info.fb.format == DISP_FORMAT_YUV420_SP_VUVU_10BIT)) {
		bpp[0] = 16;
		bpp[1] = 32;
		x1 = x0 / 2;
		y1 = y0 / 2;
	} else if ((lay_cfg->info.fb.format == DISP_FORMAT_YUV422_SP_UVUV_10BIT) ||
		   (lay_cfg->info.fb.format == DISP_FORMAT_YUV422_SP_VUVU_10BIT)) {
		bpp[0] = 16;
		bpp[1] = 32;
		x1 = x0 / 2;
		y1 = y0;
	} else {
		bpp[0] = 32;
	}

	pitch[0] = DISPALIGN(lay_cfg->info.fb.size[0].width * bpp[0] >> 3,
		lay_cfg->info.fb.align[0]);
	pitch[1] = DISPALIGN(lay_cfg->info.fb.size[1].width * bpp[1] >> 3,
		lay_cfg->info.fb.align[1]);
	pitch[2] = DISPALIGN(lay_cfg->info.fb.size[2].width * bpp[2] >> 3,
		lay_cfg->info.fb.align[2]);

	addr[0] = lay_cfg->info.fb.addr[0]
		+ pitch[0] * y0 + (x0 * bpp[0] >> 3);
	addr[1] = lay_cfg->info.fb.addr[1]
		+ pitch[1] * y1 + (x1 * bpp[1] >> 3);
	addr[2] = lay_cfg->info.fb.addr[2]
		+ pitch[2] * y2 + (x2 * bpp[2] >> 3);

	reg->lay[lay_cfg->layer_id].pitch0.dwval = pitch[0];
	reg->lay[lay_cfg->layer_id].pitch1.dwval = pitch[1];
	reg->lay[lay_cfg->layer_id].pitch2.dwval = pitch[2];
	reg->lay[lay_cfg->layer_id].top_laddr0.dwval = (u32)addr[0];
	reg->lay[lay_cfg->layer_id].top_laddr1.dwval = (u32)addr[1];
	reg->lay[lay_cfg->layer_id].top_laddr2.dwval = (u32)addr[2];
	reg->lay[lay_cfg->layer_id].bot_laddr0.dwval = 0;
	reg->lay[lay_cfg->layer_id].bot_laddr1.dwval = 0;
	reg->lay[lay_cfg->layer_id].bot_laddr2.dwval = 0;
	if (lay_cfg->layer_id == 0) {
		reg->top_haddr0.bits.haddr_lay0 = (u32)(addr[0] >> 32);
		reg->top_haddr1.bits.haddr_lay0 = (u32)(addr[1] >> 32);
		reg->top_haddr2.bits.haddr_lay0 = (u32)(addr[2] >> 32);
		reg->bot_haddr0.bits.haddr_lay0 = 0;
		reg->bot_haddr1.bits.haddr_lay0 = 0;
		reg->bot_haddr2.bits.haddr_lay0 = 0;
	} else if (lay_cfg->layer_id == 1) {
		reg->top_haddr0.bits.haddr_lay1 = (u32)(addr[0] >> 32);
		reg->top_haddr1.bits.haddr_lay1 = (u32)(addr[1] >> 32);
		reg->top_haddr2.bits.haddr_lay1 = (u32)(addr[2] >> 32);
		reg->bot_haddr0.bits.haddr_lay1 = 0;
		reg->bot_haddr1.bits.haddr_lay1 = 0;
		reg->bot_haddr2.bits.haddr_lay1 = 0;
	} else if (lay_cfg->layer_id == 2) {
		reg->top_haddr0.bits.haddr_lay2 = (u32)(addr[0] >> 32);
		reg->top_haddr1.bits.haddr_lay2 = (u32)(addr[1] >> 32);
		reg->top_haddr2.bits.haddr_lay2 = (u32)(addr[2] >> 32);
		reg->bot_haddr0.bits.haddr_lay2 = 0;
		reg->bot_haddr1.bits.haddr_lay2 = 0;
		reg->bot_haddr2.bits.haddr_lay2 = 0;
	} else if (lay_cfg->layer_id == 3) {
		reg->top_haddr0.bits.haddr_lay3 = (u32)(addr[0] >> 32);
		reg->top_haddr1.bits.haddr_lay3 = (u32)(addr[1] >> 32);
		reg->top_haddr2.bits.haddr_lay3 = (u32)(addr[2] >> 32);
		reg->bot_haddr0.bits.haddr_lay3 = 0;
		reg->bot_haddr1.bits.haddr_lay3 = 0;
		reg->bot_haddr2.bits.haddr_lay3 = 0;
	}

	return 0;
}

static s32 de_ovl_v_set_coarse(struct ovl_v_reg *reg,
	struct de_chn_info *chn_info)
{
	if ((chn_info->ovl_out_win.width != 0)
		&& (chn_info->ovl_out_win.width != chn_info->ovl_win.width)) {
		reg->hori_ds[0].bits.m = chn_info->ovl_win.width;
		reg->hori_ds[0].bits.n = chn_info->ovl_out_win.width;
		reg->hori_ds[1].bits.m = chn_info->ovl_win.width;
		reg->hori_ds[1].bits.n = chn_info->ovl_out_win.width;
	} else {
		reg->hori_ds[0].dwval = 0;
		reg->hori_ds[1].dwval = 0;
	}
	if ((chn_info->ovl_out_win.height != 0)
		&& (chn_info->ovl_out_win.height != chn_info->ovl_win.height)) {
		reg->vert_ds[0].bits.m = chn_info->ovl_win.height;
		reg->vert_ds[0].bits.n = chn_info->ovl_out_win.height;
		reg->vert_ds[1].bits.m = chn_info->ovl_win.height;
		reg->vert_ds[1].bits.n = chn_info->ovl_out_win.height;
	} else {
		reg->vert_ds[0].dwval = 0;
		reg->vert_ds[1].dwval = 0;
	}
	return 0;
}

static s32 de_ovl_v_apply_lay(u32 disp, u32 chn,
	struct disp_layer_config_data **const pdata, u32 layer_num)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_chn_info *chn_info = ctx->chn_info + chn;
	struct de_ovl_private *priv = &(ovl_priv[disp][chn]);
	struct ovl_v_reg *reg = get_ovl_v_reg(priv);
	u32 dwval;
	u32 width, height;
	u32 i;

	for (i = 0; i < layer_num; ++i) {
		struct disp_layer_config_inner *lay_cfg = &(pdata[i]->config);
		struct disp_layer_info_inner *lay = &(pdata[i]->config.info);
		u32 format, vi_ui_sel;

		de_ovl_convert_fmt(lay->fb.format, &format, &vi_ui_sel);
		dwval = 0x1;
		dwval |= ((lay->alpha_mode << 1) & 0x6);
		dwval |= ((lay->mode == LAYER_MODE_BUFFER) ? 0x0 : 0x10);
		dwval |= ((format << 8) & 0x1F00);
		dwval |= ((vi_ui_sel << 15) & 0x8000);
		dwval |= ((chn_info->lay_premul[i] & 0x3) << 16);
		/*dwval |= (0x1 << 20);*/
		dwval |= ((lay->alpha_value << 24) & 0xFF000000);
		reg->lay[lay_cfg->layer_id].ctl.dwval = dwval;

		width = lay_cfg->info.fb.crop.width >> 32;
		height = lay_cfg->info.fb.crop.height >> 32;
		dwval = ((width ? (width - 1) : 0) & 0x1FFF)
			| (((height ? (height - 1) : 0) << 16) & 0x1FFF0000);
		reg->lay[lay_cfg->layer_id].mbsize.dwval = dwval;

		dwval = (chn_info->lay_win[i].left & 0xFFFF)
			| ((chn_info->lay_win[i].top << 16) & 0xFFFF0000);
		reg->lay[lay_cfg->layer_id].mbcoor.dwval = dwval;

		if (lay->mode == LAYER_MODE_COLOR)
			reg->fcolor[lay_cfg->layer_id].dwval = lay->color;

		de_ovl_v_set_lay_layout(reg, lay_cfg);
		priv->set_blk_dirty(priv, ((lay_cfg->layer_id < 2) ?
			OVL_V_REG_BLK_LAY_0_1 : OVL_V_REG_BLK_LAY_2_3), 1);
	}

	width = chn_info->ovl_win.width;
	height = chn_info->ovl_win.height;
	dwval = ((width ? (width - 1) : 0) & 0x1FFF)
		| (((height ? (height - 1) : 0) << 16) & 0x1FFF0000);
	reg->win_size.dwval = dwval;
	de_ovl_v_set_coarse(reg, chn_info);

	priv->set_blk_dirty(priv, OVL_V_REG_BLK_PARA, 1);

	return 0;
}

static s32 de_ovl_u_set_lay_layout(struct ovl_u_reg *reg,
	struct disp_layer_config_inner *const lay_cfg)
{
	u32 bpp;
	u32 pitch;
	u32 x0, y0;
	u64 addr;

	x0 = lay_cfg->info.fb.crop.x >> 32;
	y0 = lay_cfg->info.fb.crop.y >> 32;

	if (lay_cfg->info.fb.format <= DISP_FORMAT_BGRX_8888) {
		bpp = 32;
	} else if (lay_cfg->info.fb.format <= DISP_FORMAT_BGR_888) {
		bpp = 24;
	} else if (lay_cfg->info.fb.format <= DISP_FORMAT_BGRA_5551) {
		bpp = 16;
	} else if (lay_cfg->info.fb.format <= DISP_FORMAT_B10G10R10A2) {
		bpp = 32;
	} else {
		DE_WRN("format=%d\n", lay_cfg->info.fb.format);
		bpp = 32;
	}

	pitch = DISPALIGN(lay_cfg->info.fb.size[0].width * bpp >> 3,
		lay_cfg->info.fb.align[0]);
	addr = lay_cfg->info.fb.addr[0]
		+ pitch * y0 + (x0 * bpp >> 3);

	reg->lay[lay_cfg->layer_id].pitch.dwval = pitch;
	reg->lay[lay_cfg->layer_id].top_laddr.dwval = (u32)addr;
	reg->lay[lay_cfg->layer_id].bot_laddr.dwval = 0;
	if (lay_cfg->layer_id == 0) {
		reg->top_haddr.bits.haddr_lay0 = (u32)(addr >> 32);
		reg->bot_haddr.bits.haddr_lay0 = 0;
	} else if (lay_cfg->layer_id == 1) {
		reg->top_haddr.bits.haddr_lay1 = (u32)(addr >> 32);
		reg->bot_haddr.bits.haddr_lay1 = 0;
	} else if (lay_cfg->layer_id == 2) {
		reg->top_haddr.bits.haddr_lay2 = (u32)(addr >> 32);
		reg->bot_haddr.bits.haddr_lay2 = 0;
	} else if (lay_cfg->layer_id == 3) {
		reg->top_haddr.bits.haddr_lay3 = (u32)(addr >> 32);
		reg->bot_haddr.bits.haddr_lay3 = 0;
	}

	return 0;
}

static s32 de_ovl_u_set_coarse(struct ovl_u_reg *reg,
	struct de_chn_info *chn_info)
{
	if ((chn_info->ovl_out_win.width != 0)
		&& (chn_info->ovl_out_win.width != chn_info->ovl_win.width)) {
		reg->hori_ds.bits.m = chn_info->ovl_win.width;
		reg->hori_ds.bits.n = chn_info->ovl_out_win.width;
	} else {
		reg->hori_ds.dwval = 0;
	}
	if ((chn_info->ovl_out_win.height != 0)
		&& (chn_info->ovl_out_win.height != chn_info->ovl_win.height)) {
		reg->vert_ds.bits.m = chn_info->ovl_win.height;
		reg->vert_ds.bits.n = chn_info->ovl_out_win.height;
	} else {
		reg->vert_ds.dwval = 0;
	}
	return 0;
}

static s32 de_ovl_u_apply_lay(u32 disp, u32 chn,
	struct disp_layer_config_data **const pdata, u32 layer_num)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_chn_info *chn_info = ctx->chn_info + chn;
	struct de_ovl_private *priv = &(ovl_priv[disp][chn]);
	struct ovl_u_reg *reg = get_ovl_u_reg(priv);
	u32 dwval;
	u32 width, height;
	u32 i;

	for (i = 0; i < layer_num; ++i) {
		struct disp_layer_config_inner *lay_cfg = &(pdata[i]->config);
		struct disp_layer_info_inner *lay = &(pdata[i]->config.info);
		u32 format, vi_ui_sel;

		de_ovl_convert_fmt(lay->fb.format, &format, &vi_ui_sel);
		dwval = 0x1;
		dwval |= ((lay->alpha_mode << 1) & 0x6);
		dwval |= ((lay->mode == LAYER_MODE_BUFFER) ? 0x0 : 0x10);
		dwval |= ((format << 8) & 0x1F00);
		dwval |= ((chn_info->lay_premul[i] & 0x3) << 16);
		/*dwval |= (0x1 << 20);*/
		dwval |= ((u32)lay->alpha_value << 24);
		reg->lay[lay_cfg->layer_id].ctl.dwval = dwval;

		width = lay_cfg->info.fb.crop.width >> 32;
		height = lay_cfg->info.fb.crop.height >> 32;
		dwval = ((width ? (width - 1) : 0) & 0x1FFF)
			| (height ? (((height - 1) & 0x1FFF) << 16) : 0);
		reg->lay[lay_cfg->layer_id].mbsize.dwval = dwval;

		dwval = (chn_info->lay_win[i].left & 0xFFFF)
			| ((chn_info->lay_win[i].top & 0xFFFF) << 16);
		reg->lay[lay_cfg->layer_id].mbcoor.dwval = dwval;

		if (lay->mode == LAYER_MODE_COLOR)
			reg->lay[lay_cfg->layer_id].fcolor.dwval = lay->color;

		de_ovl_u_set_lay_layout(reg, lay_cfg);

		priv->set_blk_dirty(priv, lay_cfg->layer_id, 1);
	}

	width = chn_info->ovl_win.width;
	height = chn_info->ovl_win.height;
	dwval = ((width ? (width - 1) : 0) & 0x1FFF)
		| (height ? (((height - 1) & 0x1FFF) << 16) : 0);
	reg->win_size.dwval = dwval;
	de_ovl_u_set_coarse(reg, chn_info);

	priv->set_blk_dirty(priv, OVL_U_REG_BLK_PARA, 1);
	priv->set_blk_dirty(priv, OVL_U_REG_BLK_DS, 1);

	return 0;
}

s32 de_ovl_apply_lay(u32 disp, u32 chn,
	struct disp_layer_config_data **const pdata, u32 layer_num)
{
	struct de_ovl_private *priv = &(ovl_priv[disp][chn]);

	if (priv->type == OVL_TYPE_VI)
		de_ovl_v_apply_lay(disp, chn, pdata, layer_num);
	else if (priv->type == OVL_TYPE_UI)
		de_ovl_u_apply_lay(disp, chn, pdata, layer_num);

	return 0;
}

s32 de_ovl_disable_lay(u32 disp, u32 chn, u32 layer_id)
{
	struct de_ovl_private *priv = &(ovl_priv[disp][chn]);

	if (priv->type == OVL_TYPE_VI) {
		struct ovl_v_reg *reg = get_ovl_v_reg(priv);

		if (reg->lay[layer_id].ctl.bits.en == 0)
			return 0;
		reg->lay[layer_id].ctl.bits.en = 0;
		priv->set_blk_dirty(priv, (layer_id >> 1), 1);
	} else if (priv->type == OVL_TYPE_UI) {
		struct ovl_u_reg *reg = get_ovl_u_reg(priv);

		if (reg->lay[layer_id].ctl.bits.en == 0)
			return 0;
		reg->lay[layer_id].ctl.bits.en = 0;
		priv->set_blk_dirty(priv, layer_id, 1);
	}
	return 0;
}

s32 de_ovl_disable(u32 disp, u32 chn)
{
	struct de_ovl_private *priv = &(ovl_priv[disp][chn]);
	u32 lay_num, lay;

	if (priv->type == OVL_TYPE_VI) {
		struct ovl_v_reg *reg = get_ovl_v_reg(priv);

		lay_num = sizeof(reg->lay) / sizeof(reg->lay[0]);
		for (lay = 0; lay < lay_num; ++lay) {
			if (reg->lay[lay].ctl.bits.en == 0)
				continue;
			reg->lay[lay].ctl.bits.en = 0;
			priv->set_blk_dirty(priv, (lay >> 1), 1);
		}
	} else if (priv->type == OVL_TYPE_UI) {
		struct ovl_u_reg *reg = get_ovl_u_reg(priv);

		lay_num = sizeof(reg->lay) / sizeof(reg->lay[0]);
		for (lay = 0; lay < lay_num; ++lay) {
			if (reg->lay[lay].ctl.bits.en == 0)
				continue;
			reg->lay[lay].ctl.bits.en = 0;
			priv->set_blk_dirty(priv, lay, 1);
		}
	}
	return 0;
}

s32 de_ovl_init(u32 disp, u8 __iomem *de_reg_base)
{
	u32 chn_num, chn;
	u32 vi_chn_num = de_feat_get_num_vi_chns(disp);

	chn_num = de_feat_get_num_chns(disp);
	for (chn = 0; chn < chn_num; ++chn) {

		u32 phy_chn = de_feat_get_phy_chn_id(disp, chn);
		u8 __iomem *reg_base = (u8 __iomem *)(de_reg_base
			+ DE_CHN_OFFSET(phy_chn) + CHN_OVL_OFFSET);
		u32 rcq_used = de_feat_is_using_rcq(disp);

		struct de_ovl_private *priv = &ovl_priv[disp][chn];
		struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
		struct de_reg_block *block;

		if (chn < vi_chn_num) {
			priv->type = OVL_TYPE_VI;
			reg_mem_info->size = sizeof(struct ovl_v_reg);
			reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
				reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
				rcq_used);
			if (NULL == reg_mem_info->vir_addr) {
				DE_WRN("alloc ovl[%d][%d] mm fail!size=0x%x\n",
					 disp, chn, reg_mem_info->size);
				return -1;
			}

			block = &(priv->v_reg_blks[OVL_V_REG_BLK_LAY_0_1]);
			block->phy_addr = reg_mem_info->phy_addr;
			block->vir_addr = reg_mem_info->vir_addr;
			block->size = 0x60;
			block->reg_addr = reg_base;

			block = &(priv->v_reg_blks[OVL_V_REG_BLK_LAY_2_3]);
			block->phy_addr = reg_mem_info->phy_addr + 0x60;
			block->vir_addr = reg_mem_info->vir_addr + 0x60;
			block->size = 0x60;
			block->reg_addr = reg_base + 0x60;

			block = &(priv->v_reg_blks[OVL_V_REG_BLK_PARA]);
			block->phy_addr = reg_mem_info->phy_addr + 0xC0;
			block->vir_addr = reg_mem_info->vir_addr + 0xC0;
			block->size = 0x40;
			block->reg_addr = reg_base + 0xC0;

			priv->reg_blk_num = OVL_V_REG_BLK_NUM;
		} else {
			priv->type = OVL_TYPE_UI;
			reg_mem_info->size = sizeof(struct ovl_u_reg);
			reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
				reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
				rcq_used);
			if (NULL == reg_mem_info->vir_addr) {
				DE_WRN("alloc ovl[%d][%d] mm fail!size=0x%x\n",
					 disp, chn, reg_mem_info->size);
				return -1;
			}

			block = &(priv->u_reg_blks[OVL_U_REG_BLK_LAY_0]);
			block->phy_addr = reg_mem_info->phy_addr;
			block->vir_addr = reg_mem_info->vir_addr;
			block->size = 0x1C;
			block->reg_addr = reg_base;

			block = &(priv->u_reg_blks[OVL_U_REG_BLK_LAY_1]);
			block->phy_addr = reg_mem_info->phy_addr + 0x20;
			block->vir_addr = reg_mem_info->vir_addr + 0x20;
			block->size = 0x1C;
			block->reg_addr = reg_base + 0x20;

			block = &(priv->u_reg_blks[OVL_U_REG_BLK_LAY_2]);
			block->phy_addr = reg_mem_info->phy_addr + 0x40;
			block->vir_addr = reg_mem_info->vir_addr + 0x40;
			block->size = 0x1C;
			block->reg_addr = reg_base + 0x40;

			block = &(priv->u_reg_blks[OVL_U_REG_BLK_LAY_3]);
			block->phy_addr = reg_mem_info->phy_addr + 0x60;
			block->vir_addr = reg_mem_info->vir_addr + 0x60;
			block->size = 0x1C;
			block->reg_addr = reg_base + 0x60;

			block = &(priv->u_reg_blks[OVL_U_REG_BLK_PARA]);
			block->phy_addr = reg_mem_info->phy_addr + 0x80;
			block->vir_addr = reg_mem_info->vir_addr + 0x80;
			block->size = 12;
			block->reg_addr = reg_base + 0x80;

			block = &(priv->u_reg_blks[OVL_U_REG_BLK_DS]);
			block->phy_addr = reg_mem_info->phy_addr + 0xE0;
			block->vir_addr = reg_mem_info->vir_addr + 0xE0;
			block->size = 0x1C;
			block->reg_addr = reg_base + 0xE0;

			priv->reg_blk_num = OVL_U_REG_BLK_NUM;
		}

		if (rcq_used)
			priv->set_blk_dirty = ovl_set_rcq_head_dirty;
		else
			priv->set_blk_dirty = ovl_set_block_dirty;

	}

	return 0;
}

s32 de_ovl_exit(u32 disp)
{
	u32 chn_num, chn;

	chn_num = de_feat_get_num_chns(disp);
	for (chn = 0; chn < chn_num; ++chn) {
		struct de_ovl_private *priv = &ovl_priv[disp][chn];
		struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);

		if (reg_mem_info->vir_addr != NULL)
			de_top_reg_memory_free(reg_mem_info->vir_addr,
				reg_mem_info->phy_addr, reg_mem_info->size);
	}

	return 0;
}

s32 de_ovl_get_reg_blocks(u32 disp,
	struct de_reg_block **blks, u32 *blk_num)
{
	u32 chn_num, chn;
	u32 total = 0;

	chn_num = de_feat_get_num_chns(disp);

	if (blks == NULL) {
		for (chn = 0; chn < chn_num; ++chn)
			total += ovl_priv[disp][chn].reg_blk_num;
		*blk_num = total;
		return 0;
	}

	for (chn = 0; chn < chn_num; ++chn) {
		struct de_ovl_private *priv = &(ovl_priv[disp][chn]);
		struct de_reg_block *blk_begin, *blk_end;
		u32 num;

		if (*blk_num >= priv->reg_blk_num) {
			num = priv->reg_blk_num;
		} else {
			DE_WRN("should not happen\n");
			num = *blk_num;
		}
		blk_begin = priv->v_reg_blks;
		blk_end = blk_begin + num;
		for (; blk_begin != blk_end; ++blk_begin)
			*blks++ = blk_begin;

		total += num;
		*blk_num -= num;
	}
	*blk_num = total;
	return 0;
}
