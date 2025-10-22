
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2023 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/version.h>
#include <drm/drm_blend.h>
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_gem_cma_helper.h>
#else
#include <drm/drm_fb_dma_helper.h>
#include <drm/drm_gem_dma_helper.h>
#endif
#include <drm/drm_framebuffer.h>
#include <drm/drm_fourcc.h>
#include "de_ovl.h"
#include "de_ovl_type.h"
#include "de_ovl_platform.h"

#define CHN_OVL_OFFSET				(0x01000)

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

struct de_ovl_debug_info {
	struct de_ovl_cfg cfg;
	unsigned int w[MAX_LAYER_NUM_PER_CHN], h[MAX_LAYER_NUM_PER_CHN];
};

struct de_ovl_private {
	struct de_reg_mem_info reg_mem_info;
	struct de_ovl_debug_info debug;
	u32 reg_blk_num;
	const struct de_ovl_dsc *dsc;
	union {
		struct de_reg_block v_reg_blks[OVL_V_REG_BLK_NUM];
		struct de_reg_block u_reg_blks[OVL_U_REG_BLK_NUM];
		struct de_reg_block reg_blks[OVL_MAX_REG_BLK_NUM];
	};
};

const unsigned int ui_ovl_formats[] = {
	DRM_FORMAT_RGB888,   DRM_FORMAT_BGR888,

	DRM_FORMAT_ARGB8888, DRM_FORMAT_ABGR8888,
	DRM_FORMAT_RGBA8888, DRM_FORMAT_BGRA8888,

	DRM_FORMAT_XRGB8888, DRM_FORMAT_XBGR8888,
	DRM_FORMAT_RGBX8888, DRM_FORMAT_BGRX8888,

	DRM_FORMAT_RGB565,   DRM_FORMAT_BGR565,

	DRM_FORMAT_ARGB4444, DRM_FORMAT_ABGR4444,
	DRM_FORMAT_RGBA4444, DRM_FORMAT_BGRA4444,

	DRM_FORMAT_ARGB1555, DRM_FORMAT_ABGR1555,
	DRM_FORMAT_RGBA5551, DRM_FORMAT_BGRA5551,
};

const unsigned int vi_ovl_formats[] = {
	DRM_FORMAT_RGB888,   DRM_FORMAT_BGR888,

	DRM_FORMAT_ARGB8888, DRM_FORMAT_ABGR8888,
	DRM_FORMAT_RGBA8888, DRM_FORMAT_BGRA8888,

	DRM_FORMAT_XRGB8888, DRM_FORMAT_XBGR8888,
	DRM_FORMAT_RGBX8888, DRM_FORMAT_BGRX8888,

	DRM_FORMAT_RGB565,   DRM_FORMAT_BGR565,

	DRM_FORMAT_ARGB4444, DRM_FORMAT_ABGR4444,
	DRM_FORMAT_RGBA4444, DRM_FORMAT_BGRA4444,

	DRM_FORMAT_ARGB1555, DRM_FORMAT_ABGR1555,
	DRM_FORMAT_RGBA5551, DRM_FORMAT_BGRA5551,

	DRM_FORMAT_ARGB2101010, DRM_FORMAT_ABGR2101010,
	DRM_FORMAT_RGBA1010102, DRM_FORMAT_BGRA1010102,

	DRM_FORMAT_NV61,     DRM_FORMAT_NV16,

	DRM_FORMAT_YUV422,   DRM_FORMAT_YVU422,

	DRM_FORMAT_NV12,     DRM_FORMAT_NV21,

	DRM_FORMAT_YUV420,   DRM_FORMAT_YVU420,

	DRM_FORMAT_YUV411,   DRM_FORMAT_YVU411,

	DRM_FORMAT_P010,

	DRM_FORMAT_P210,
};

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

static void ovl_set_block_dirty(
	struct de_ovl_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
	if (priv->reg_blks[blk_id].rcq_hd)
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
}

static int de_ovl_v_set_coarse(struct ovl_v_reg *reg,
	const struct de_ovl_cfg *cfg)
{
	if ((cfg->ovl_out_win.width != 0)
		&& (cfg->ovl_out_win.width != cfg->ovl_win.width)) {
		reg->hori_ds[0].bits.m = cfg->ovl_win.width;
		reg->hori_ds[0].bits.n = cfg->ovl_out_win.width;
		reg->hori_ds[1].bits.m = cfg->ovl_win.width;
		reg->hori_ds[1].bits.n = cfg->ovl_out_win.width;
	} else {
		reg->hori_ds[0].dwval = 0;
		reg->hori_ds[1].dwval = 0;
	}
	if ((cfg->ovl_out_win.height != 0)
		&& (cfg->ovl_out_win.height != cfg->ovl_win.height)) {
		reg->vert_ds[0].bits.m = cfg->ovl_win.height;
		reg->vert_ds[0].bits.n = cfg->ovl_out_win.height;
		reg->vert_ds[1].bits.m = cfg->ovl_win.height;
		reg->vert_ds[1].bits.n = cfg->ovl_out_win.height;
	} else {
		reg->vert_ds[0].dwval = 0;
		reg->vert_ds[1].dwval = 0;
	}
	return 0;
}

static int de_ovl_v_set_lay_layout(struct ovl_v_reg *reg, unsigned int x0, unsigned int y0,
					int format, struct drm_framebuffer *fb, unsigned int layer_id, bool swap)
{
	u32 bpp[3] = {0, 0, 0};
	u32 x[3] = {x0, 0, 0};
	u32 y[3] = {y0, 0, 0};
	u64 addr[3] = {0, 0, 0};
	u64 tmp;
	unsigned int *pitch = fb->pitches;
	u64 addr_tmp = 0;
	const struct drm_format_info *format_info = NULL;
	int i;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
	struct drm_gem_cma_object *gem;
#else
	struct drm_gem_dma_object *gem;
#endif
	if (format <= DE_FORMAT_BGRX_8888) {
		bpp[0] = 32;
	} else if (format <= DE_FORMAT_BGR_888) {
		bpp[0] = 24;
	} else if (format <= DE_FORMAT_BGRA_5551) {
		bpp[0] = 16;
	} else if (format <= DE_FORMAT_BGRA_1010102) {
		bpp[0] = 32;
	} else if (format <= DE_FORMAT_YUV444_I_VUYA) {
		bpp[0] = 32;
	} else if (format <= DE_FORMAT_YUV422_I_VYUY) {
		bpp[0] = 16;
	} else if (format == DE_FORMAT_YUV422_P || format == DE_FORMAT_YVU422_P) {
		bpp[0] = 8;
		bpp[1] = 8;
		bpp[2] = 8;
		x[2] = x[1] = x[0] / 2;
		y[2] = y[1] = y[0];
	} else if (format == DE_FORMAT_YUV420_P || format == DE_FORMAT_YVU420_P) {
		bpp[0] = 8;
		bpp[1] = 8;
		bpp[2] = 8;
		x[2] = x[1] = x[0] / 2;
		y[2] = y[1] = y[0] / 2;
	} else if (format == DE_FORMAT_YUV411_P || format == DE_FORMAT_YVU411_P) {
		bpp[0] = 8;
		bpp[1] = 8;
		bpp[2] = 8;
		x[2] = x[1] = x[0] / 4;
		y[2] = y[1] = y[0];
	} else if (format <= DE_FORMAT_YUV422_SP_VUVU) {
		bpp[0] = 8;
		bpp[1] = 16;
		x[1] = x[0] / 2;
		y[1] = y[0];
	} else if (format <= DE_FORMAT_YUV420_SP_VUVU) {
		bpp[0] = 8;
		bpp[1] = 16;
		x[1] = x[0] / 2;
		y[1] = y[0] / 2;
	} else if (format <= DE_FORMAT_YUV411_SP_VUVU) {
		bpp[0] = 8;
		bpp[1] = 16;
		x[1] = x[0] / 4;
		y[1] = y[0];
	} else if ((format == DE_FORMAT_YUV420_SP_UVUV_10BIT) ||
		   (format == DE_FORMAT_YUV420_SP_VUVU_10BIT)) {
		bpp[0] = 16;
		bpp[1] = 32;
		x[1] = x[0] / 2;
		y[1] = y[0] / 2;
	} else if ((format == DE_FORMAT_YUV422_SP_UVUV_10BIT) ||
		   (format == DE_FORMAT_YUV422_SP_VUVU_10BIT)) {
		bpp[0] = 16;
		bpp[1] = 32;
		x[1] = x[0] / 2;
		y[1] = y[0];
	} else {
		bpp[0] = 32;
	}

	format_info = drm_format_info(fb->format->format);
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
	for (i = 0; i < format_info->num_planes; i++) {
		gem = drm_fb_cma_get_gem_obj(fb, i);
		if (gem) {
			addr_tmp = (u64)(gem->paddr) + fb->offsets[i];
			addr[i] = addr_tmp
				+ pitch[i] * y[i] + (x[i] * bpp[i] >> 3);
		}
	}
#else
	for (i = 0; i < format_info->num_planes; i++) {
		gem = drm_fb_dma_get_gem_obj(fb, i);
		if (gem) {
			addr_tmp = (u64)(gem->dma_addr) + fb->offsets[i];
			addr[i] = addr_tmp
				+ pitch[i] * y[i] + (x[i] * bpp[i] >> 3);
		}
	}
#endif
	if (swap) {
		tmp = addr[1];
		addr[1] = addr[2];
		addr[2] = tmp;
	}
	reg->lay[layer_id].pitch0.dwval = pitch[0];
	reg->lay[layer_id].pitch1.dwval = pitch[1];
	reg->lay[layer_id].pitch2.dwval = pitch[2];
	reg->lay[layer_id].top_laddr0.dwval = (u32)addr[0];
	reg->lay[layer_id].top_laddr1.dwval = (u32)addr[1];
	reg->lay[layer_id].top_laddr2.dwval = (u32)addr[2];
	reg->lay[layer_id].bot_laddr0.dwval = 0;
	reg->lay[layer_id].bot_laddr1.dwval = 0;
	reg->lay[layer_id].bot_laddr2.dwval = 0;
	if (layer_id == 0) {
		reg->top_haddr0.bits.haddr_lay0 = (u32)(addr[0] >> 32);
		reg->top_haddr1.bits.haddr_lay0 = (u32)(addr[1] >> 32);
		reg->top_haddr2.bits.haddr_lay0 = (u32)(addr[2] >> 32);
		reg->bot_haddr0.bits.haddr_lay0 = 0;
		reg->bot_haddr1.bits.haddr_lay0 = 0;
		reg->bot_haddr2.bits.haddr_lay0 = 0;
	} else if (layer_id == 1) {
		reg->top_haddr0.bits.haddr_lay1 = (u32)(addr[0] >> 32);
		reg->top_haddr1.bits.haddr_lay1 = (u32)(addr[1] >> 32);
		reg->top_haddr2.bits.haddr_lay1 = (u32)(addr[2] >> 32);
		reg->bot_haddr0.bits.haddr_lay1 = 0;
		reg->bot_haddr1.bits.haddr_lay1 = 0;
		reg->bot_haddr2.bits.haddr_lay1 = 0;
	} else if (layer_id == 2) {
		reg->top_haddr0.bits.haddr_lay2 = (u32)(addr[0] >> 32);
		reg->top_haddr1.bits.haddr_lay2 = (u32)(addr[1] >> 32);
		reg->top_haddr2.bits.haddr_lay2 = (u32)(addr[2] >> 32);
		reg->bot_haddr0.bits.haddr_lay2 = 0;
		reg->bot_haddr1.bits.haddr_lay2 = 0;
		reg->bot_haddr2.bits.haddr_lay2 = 0;
	} else if (layer_id == 3) {
		reg->top_haddr0.bits.haddr_lay3 = (u32)(addr[0] >> 32);
		reg->top_haddr1.bits.haddr_lay3 = (u32)(addr[1] >> 32);
		reg->top_haddr2.bits.haddr_lay3 = (u32)(addr[2] >> 32);
		reg->bot_haddr0.bits.haddr_lay3 = 0;
		reg->bot_haddr1.bits.haddr_lay3 = 0;
		reg->bot_haddr2.bits.haddr_lay3 = 0;
	}

	return 0;
}

static u32 de_ovl_convert_fmt(enum de_pixel_format format,
	u32 *fmt, u32 *vi_ui_sel)
{
	u32 swap = 0;
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
	case DE_FORMAT_YVU422_P:
		swap = 1;
		fallthrough;
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
	case DE_FORMAT_YVU420_P:
		swap = 1;
		fallthrough;
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
	case DE_FORMAT_YVU411_P:
		swap = 1;
		fallthrough;
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
	return swap;
}

static int de_ovl_disable_lay(struct de_ovl_handle *handle, u32 layer_id)
{
	struct de_ovl_private *priv = handle->private;

	if (handle->private->dsc->type == OVL_TYPE_VI) {
		struct ovl_v_reg *reg = get_ovl_v_reg(priv);

		if (reg->lay[layer_id].ctl.bits.en == 0)
			return 0;
		reg->lay[layer_id].ctl.bits.en = 0;
		ovl_set_block_dirty(priv, (layer_id >> 1), 1);
	} else if (handle->private->dsc->type == OVL_TYPE_UI) {
		struct ovl_u_reg *reg = get_ovl_u_reg(priv);

		if (reg->lay[layer_id].ctl.bits.en == 0)
			return 0;
		reg->lay[layer_id].ctl.bits.en = 0;
		ovl_set_block_dirty(priv, layer_id, 1);
	}
	return 0;
}

static int de_ovl_v_apply_lay(struct de_ovl_handle *handle, const struct display_channel_state *state, const struct de_ovl_cfg *cfg)
{
	u32 width, height, x, y, dwval, format, vi_ui_sel, i, alpha;
	struct de_ovl_private *priv = handle->private;
	struct ovl_v_reg *reg = get_ovl_v_reg(priv);
	struct drm_framebuffer *fb;
	int fmt;
	bool ignore_pixel_alpha;
	u32 swap;

	for (i = 0; i < priv->dsc->layer_cnt; ++i) {
		if (!cfg->layer_en[i]) {
			de_ovl_disable_lay(handle, i);

			priv->debug.w[i] = 0;
			priv->debug.h[i] = 0;
			continue;
		}
		if (i == 0) {
			fb = state->base.fb;
			width = state->base.src_w >> 16;
			height = state->base.src_h >> 16;
			x = state->base.src_x >> 16;
			y = state->base.src_y >> 16;
			ignore_pixel_alpha = state->base.pixel_blend_mode == DRM_MODE_BLEND_PIXEL_NONE;
			alpha = state->base.alpha >> 8;
		} else {
			width = state->src_w[i - 1] >> 16;
			height = state->src_h[i - 1] >> 16;
			fb = state->fb[i - 1];
			x = state->src_x[i - 1] >> 16;
			y = state->src_y[i - 1] >> 16;
			ignore_pixel_alpha = state->pixel_blend_mode[i - 1] == DRM_MODE_BLEND_PIXEL_NONE;
			alpha = state->alpha[i - 1] >> 8;
		}

		if (!fb) {
			reg->lay[i].ctl.dwval = 0;
			ovl_set_block_dirty(priv, ((i < 2) ?
				OVL_V_REG_BLK_LAY_0_1 : OVL_V_REG_BLK_LAY_2_3), 1);
			continue;
		}
		fmt = drm_to_de_format(fb->format->format);

		swap = de_ovl_convert_fmt(fmt, &format, &vi_ui_sel);
		if (i == 0 && state->fake_layer0)
			dwval = 0;
		else
			dwval = 0x1;
		dwval |= (ignore_pixel_alpha ? 1 : 2) << 1;
		dwval |= ((state->color[i]) ? 0x10 : 0x0);
		dwval |= ((format << 8) & 0x1F00);
		dwval |= ((vi_ui_sel << 15) & 0x8000);
		dwval |= ((cfg->lay_premul[i] & 0x3) << 16);
		dwval |= ((alpha << 24) & 0xFF000000);
		reg->lay[i].ctl.dwval = dwval;

		priv->debug.w[i] = width;
		priv->debug.h[i] = height;

		dwval = ((width ? (width - 1) : 0) & 0x1FFF)
			| (((height ? (height - 1) : 0) << 16) & 0x1FFF0000);
		reg->lay[i].mbsize.dwval = dwval;

		dwval = (cfg->lay_win[i].left & 0xFFFF)
			| ((cfg->lay_win[i].top << 16) & 0xFFFF0000);
		reg->lay[i].mbcoor.dwval = dwval;

		if (state->color[i])
			reg->fcolor[i].dwval = state->color[i];

		de_ovl_v_set_lay_layout(reg, x, y, fmt, fb, i, swap);
		ovl_set_block_dirty(priv, ((i < 2) ?
			OVL_V_REG_BLK_LAY_0_1 : OVL_V_REG_BLK_LAY_2_3), 1);
	}


	width = cfg->ovl_win.width;
	height = cfg->ovl_win.height;
	dwval = ((width ? (width - 1) : 0) & 0x1FFF)
		| (((height ? (height - 1) : 0) << 16) & 0x1FFF0000);
	reg->win_size.dwval = dwval;
	de_ovl_v_set_coarse(reg, cfg);

	ovl_set_block_dirty(priv, OVL_V_REG_BLK_PARA, 1);

	return 0;
}

static int de_ovl_u_set_coarse(struct ovl_u_reg *reg,
	const struct de_ovl_cfg *cfg)
{
	if ((cfg->ovl_out_win.width != 0)
		&& (cfg->ovl_out_win.width != cfg->ovl_win.width)) {
		reg->hori_ds.bits.m = cfg->ovl_win.width;
		reg->hori_ds.bits.n = cfg->ovl_out_win.width;
	} else {
		reg->hori_ds.dwval = 0;
	}
	if ((cfg->ovl_out_win.height != 0)
		&& (cfg->ovl_out_win.height != cfg->ovl_win.height)) {
		reg->vert_ds.bits.m = cfg->ovl_win.height;
		reg->vert_ds.bits.n = cfg->ovl_out_win.height;
	} else {
		reg->vert_ds.dwval = 0;
	}
	return 0;
}

static int de_ovl_u_set_lay_layout(struct ovl_u_reg *reg, unsigned int x0, unsigned y0, int format, struct drm_framebuffer *fb, unsigned int layer_id)
{
	u32 bpp;
	u32 pitch = fb->pitches[0];
	u64 addr;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
	struct drm_gem_cma_object *gem;
#else
	struct drm_gem_dma_object *gem;
#endif
	u64 addr_tmp = 0;

	if (format <= DE_FORMAT_BGRX_8888) {
		bpp = 32;
	} else if (format <= DE_FORMAT_BGR_888) {
		bpp = 24;
	} else if (format <= DE_FORMAT_BGRA_5551) {
		bpp = 16;
	} else if (format <= DE_FORMAT_BGRA_1010102) {
		bpp = 32;
	} else {
		DRM_ERROR("format=%d\n", format);
		bpp = 32;
	}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
	gem = drm_fb_cma_get_gem_obj(fb, 0);
	if (gem) {
		addr_tmp = (u64)(gem->paddr) + fb->offsets[0];
	}
#else
	gem = drm_fb_dma_get_gem_obj(fb, 0);
	if (gem) {
		addr_tmp = (u64)(gem->dma_addr) + fb->offsets[0];
	}
#endif
	addr = addr_tmp
		+ pitch * y0 + (x0 * bpp >> 3);

	reg->lay[layer_id].pitch.dwval = pitch;
	reg->lay[layer_id].top_laddr.dwval = (u32)addr;
	reg->lay[layer_id].bot_laddr.dwval = 0;
	if (layer_id == 0) {
		reg->top_haddr.bits.haddr_lay0 = (u32)(addr >> 32);
		reg->bot_haddr.bits.haddr_lay0 = 0;
	} else if (layer_id == 1) {
		reg->top_haddr.bits.haddr_lay1 = (u32)(addr >> 32);
		reg->bot_haddr.bits.haddr_lay1 = 0;
	} else if (layer_id == 2) {
		reg->top_haddr.bits.haddr_lay2 = (u32)(addr >> 32);
		reg->bot_haddr.bits.haddr_lay2 = 0;
	} else if (layer_id == 3) {
		reg->top_haddr.bits.haddr_lay3 = (u32)(addr >> 32);
		reg->bot_haddr.bits.haddr_lay3 = 0;
	}

	return 0;
}

static int de_ovl_u_apply_lay(struct de_ovl_handle *handle, struct display_channel_state *state, const struct de_ovl_cfg *cfg)
{
	u32 width, height, x, y, dwval,  format, vi_ui_sel, i, alpha;
	struct de_ovl_private *priv = handle->private;
	struct ovl_u_reg *reg = get_ovl_u_reg(priv);
	struct drm_framebuffer *fb;
	int fmt;
	bool ignore_pixel_alpha;

	for (i = 0; i < priv->dsc->layer_cnt; ++i) {
		if (!cfg->layer_en[i]) {
			de_ovl_disable_lay(handle, i);

			priv->debug.w[i] = 0;
			priv->debug.h[i] = 0;
			continue;
		}
		if (i == 0) {
			fb = state->base.fb;
			width = state->base.src_w >> 16;
			height = state->base.src_h >> 16;
			x = state->base.src_x >> 16;
			y = state->base.src_y >> 16;
			ignore_pixel_alpha = state->base.pixel_blend_mode == DRM_MODE_BLEND_PIXEL_NONE;
			alpha = state->base.alpha >> 8;
	} else {
			width = state->src_w[i - 1] >> 16;
			height = state->src_h[i - 1] >> 16;
			fb = state->fb[i - 1];
			x = state->src_x[i - 1] >> 16;
			y = state->src_y[i - 1] >> 16;
			ignore_pixel_alpha = state->pixel_blend_mode[i - 1] == DRM_MODE_BLEND_PIXEL_NONE;
			alpha = state->alpha[i - 1] >> 8;

		}

		if (!fb) {
			reg->lay[i].ctl.dwval = 0;
			ovl_set_block_dirty(priv, ((i < 2) ?
				OVL_V_REG_BLK_LAY_0_1 : OVL_V_REG_BLK_LAY_2_3), 1);
			continue;
		}

		fmt = drm_to_de_format(fb->format->format);

		de_ovl_convert_fmt(fmt, &format, &vi_ui_sel);
		if (i == 0 && state->fake_layer0)
			dwval = 0;
		else
			dwval = 0x1;
		dwval |= (ignore_pixel_alpha ? 1 : 2) << 1;
		dwval |= ((state->color[i]) ? 0x10 : 0x0);
		dwval |= ((format << 8) & 0x1F00);
		dwval |= ((cfg->lay_premul[i] & 0x3) << 16);
		dwval |= ((alpha << 24) & 0xFF000000);
		reg->lay[i].ctl.dwval = dwval;

		priv->debug.w[i] = width;
		priv->debug.h[i] = height;

		dwval = ((width ? (width - 1) : 0) & 0x1FFF)
			| (height ? (((height - 1) & 0x1FFF) << 16) : 0);
		reg->lay[i].mbsize.dwval = dwval;

		dwval = (cfg->lay_win[i].left & 0xFFFF)
			| ((cfg->lay_win[i].top & 0xFFFF) << 16);
		reg->lay[i].mbcoor.dwval = dwval;

		if (state->color[i])
			reg->lay[i].fcolor.dwval = state->color[i];

		de_ovl_u_set_lay_layout(reg, x, y, fmt, fb, i);

		ovl_set_block_dirty(priv, i, 1);
	}

	width = cfg->ovl_win.width;
	height = cfg->ovl_win.height;
	dwval = ((width ? (width - 1) : 0) & 0x1FFF)
		| (height ? (((height - 1) & 0x1FFF) << 16) : 0);
	reg->win_size.dwval = dwval;
	de_ovl_u_set_coarse(reg, cfg);

	ovl_set_block_dirty(priv, OVL_U_REG_BLK_PARA, 1);
	ovl_set_block_dirty(priv, OVL_U_REG_BLK_DS, 1);

	return 0;
}

void de_dump_ovl_state(struct drm_printer *p, struct de_ovl_handle *handle, const struct display_channel_state *state)
{
	struct de_ovl_debug_info *debug = &handle->private->debug;
	unsigned int i;
	unsigned long base = (unsigned long)handle->private->reg_blks[0].reg_addr;
	unsigned long de_base = (unsigned long)handle->cinfo.de_reg_base;

	drm_printf(p, "\n\tovl@%8x: %sable\n\t\t", (unsigned int)(base - de_base), debug->cfg.layer_en_cnt ? "en" : "dis");
	if (debug->cfg.layer_en_cnt) {
		for (i = 0; i < debug->cfg.layer_en_cnt; i++) {
			drm_printf(p, "(%4dx%4d+%4d+%4d) ", debug->w[i], debug->h[i],
				 debug->cfg.lay_win[i].left, debug->cfg.lay_win[i].top);
		}
		drm_printf(p, "= (%4dx%4d) ==> (%4dx%4d) ", debug->cfg.ovl_win.width, debug->cfg.ovl_win.height,
				debug->cfg.ovl_out_win.width, debug->cfg.ovl_out_win.height);
	}
}

int de_ovl_apply_lay(struct de_ovl_handle *handle, struct display_channel_state *state, const struct de_ovl_cfg *cfg)
{
	memcpy(&handle->private->debug.cfg, cfg, sizeof(*cfg));
	if (handle->private->dsc->type == OVL_TYPE_VI)
		de_ovl_v_apply_lay(handle, state, cfg);
	else if (handle->private->dsc->type == OVL_TYPE_UI)
		de_ovl_u_apply_lay(handle, state, cfg);
	return 0;
}

struct de_ovl_handle *de_ovl_create(struct module_create_info *info)
{
	struct de_ovl_handle *hdl;
	struct de_reg_block *block;
	struct de_reg_mem_info *reg_mem_info;
	u8 __iomem *reg_base;
	int i;
	struct de_ovl_private *priv;
	const struct de_ovl_dsc *dsc;
	unsigned int offset;

	dsc = get_ovl_dsc(info);
	if (!dsc)
		return NULL;

	offset = dsc->ovl_offset ? dsc->ovl_offset : CHN_OVL_OFFSET;
	reg_base = (u8 __iomem *)(info->de_reg_base + dsc->channel_base + offset);
	hdl = kmalloc(sizeof(*hdl), GFP_KERNEL | __GFP_ZERO);
	hdl->name = dsc->name;
	hdl->channel_reg_base = dsc->channel_base;
	memcpy(&hdl->cinfo, info, sizeof(*info));
	hdl->private = kmalloc(sizeof(*hdl->private), GFP_KERNEL | __GFP_ZERO);
	priv = hdl->private;
	hdl->private->dsc = dsc;
	hdl->formats = hdl->private->dsc->type == OVL_TYPE_VI ? vi_ovl_formats : ui_ovl_formats;
	hdl->format_count = hdl->private->dsc->type == OVL_TYPE_VI ?
				ARRAY_SIZE(vi_ovl_formats) : ARRAY_SIZE(ui_ovl_formats);
	hdl->layer_cnt = dsc->layer_cnt;
	hdl->is_video = hdl->private->dsc->type == OVL_TYPE_VI;
	hdl->type_hw_id = hdl->private->dsc->type_hw_id;

	reg_mem_info = &(hdl->private->reg_mem_info);
	if (hdl->private->dsc->type == OVL_TYPE_VI) {
		reg_mem_info->size = sizeof(struct ovl_v_reg);
		reg_mem_info->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
			reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
				info->update_mode == RCQ_MODE);
		if (NULL == reg_mem_info->vir_addr) {
			DRM_ERROR("alloc ovl[%d] mm fail!size=0x%x\n",
				 dsc->id, reg_mem_info->size);
			return ERR_PTR(-ENOMEM);
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
		reg_mem_info->size = sizeof(struct ovl_u_reg);
		reg_mem_info->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
			reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
			info->update_mode == RCQ_MODE);
		if (NULL == reg_mem_info->vir_addr) {
			DRM_ERROR("alloc ovl[%d] mm fail!size=0x%x\n",
				 dsc->id, reg_mem_info->size);
			return ERR_PTR(-ENOMEM);
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

	hdl->block_num = priv->reg_blk_num;
	hdl->block = kmalloc(sizeof(block[0]) * hdl->block_num, GFP_KERNEL | __GFP_ZERO);
	for (i = 0; i < hdl->private->reg_blk_num; i++)
		hdl->block[i] = &priv->reg_blks[i];

	return hdl;
}
