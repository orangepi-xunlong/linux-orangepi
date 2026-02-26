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
/* #include <linux/align.h> */

#include "de_tfbd_type.h"
#include "de_tfbd.h"
#include "img_drm_fourcc.h"

#define CHN_TFBD_OFFSET          (0x05400)

enum tfbd_format {
	TFBD_RGBA8888    =  0x0c,
	TFBD_RGBA1010102 =  0x0e,
	TFBD_RGBA4444    =  0x02,
	TFBD_RGB888      =  0x3a,
	TFBD_RGB565      =  0x05,
	TFBD_RGBA5551    =  0x53,

	TFBD_YUV420_8_2PLANE  = 0x36,
	TFBD_YUV444_10_1PLANE = 0x63,
	TFBD_YUV420_10_2PLANE = 0x65,
	TFBD_YUV422_10_2PLANE = 0x64,
	TFBD_YUV444_10_2PLANE = 0x66,
};

enum tfbc_lossy_rate {
	TFBC_LOSSLESS = 0x0,
	TFBC_LOSSY25  = 0x1,
	TFBC_LOSSY50  = 0x2,
	TFBC_LOSSY75  = 0x3,
};

enum {
	OVL_TFBD_REG_BLK = 0,
	OVL_TFBD_REG_BLK_NUM,
};

struct de_tfbd_drmformat_mapping {
	uint32_t format;
	uint32_t inputbits[4];
	uint32_t tile_size[2];
	uint32_t memory_layout[2];
	uint32_t seq;
	enum tfbd_format fmt;
	bool is_support_lossy;
};

struct de_tfbd_info {
	u32 y_header_size;
	u32 y_body_size;
	u32 uv_header_size;
	u32 uv_body_size;
	enum tfbc_lossy_rate lossy_rate;
};

struct de_tfbd_debug_info {
	bool enable;
	u32 in_width;
	u32 in_height;
	u32 crop_r;
	u32 crop_t;
	u32 crop_w;
	u32 crop_h;
	u32 ovl_r;
	u32 ovl_t;
	u32 ovl_w;
	u32 ovl_h;
	struct de_tfbd_drmformat_mapping tfbd_layout;
	struct de_tfbd_info tfbd_info;
};

struct de_tfbd_private {
	struct de_reg_mem_info reg_mem_info;
	struct de_tfbd_debug_info debug;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[OVL_TFBD_REG_BLK_NUM];
};

struct de_version_tfbd {
	unsigned int version;
	unsigned int phy_chn_cnt;
	bool *tfbd_exist;
};

static const uint64_t format_modifiers_tfbc[] = {
	DRM_FORMAT_MOD_PVR_FBCDC_16x4_V13,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY25_16x4_V13,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY50_16x4_V13,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY75_16x4_V13,
	DRM_FORMAT_MOD_INVALID,
};

const static struct de_tfbd_drmformat_mapping format2tfbc_mapping[] = {
	{ DRM_FORMAT_ARGB8888, {8, 8, 8, 8}, {16, 4}, {16, 4}, 0x2103, 0x0c, true },
	{ DRM_FORMAT_ABGR8888, {8, 8, 8, 8}, {16, 4}, {16, 4}, 0x0123, 0x0c, true },
	{ DRM_FORMAT_RGBA8888, {8, 8, 8, 8}, {16, 4}, {16, 4}, 0x3210, 0x0c, true },
	{ DRM_FORMAT_BGRA8888, {8, 8, 8, 8}, {16, 4}, {16, 4}, 0x1230, 0x0c, true },

	{ DRM_FORMAT_XRGB8888, {8, 8, 8, 8}, {16, 4}, {16, 4}, 0x2103, 0x0c, true },
	{ DRM_FORMAT_XBGR8888, {8, 8, 8, 8}, {16, 4}, {16, 4}, 0x0123, 0x0c, true },
	{ DRM_FORMAT_RGBX8888, {8, 8, 8, 8}, {16, 4}, {16, 4}, 0x3210, 0x0c, true },
	{ DRM_FORMAT_BGRX8888, {8, 8, 8, 8}, {16, 4}, {16, 4}, 0x1230, 0x0c, true },

	{ DRM_FORMAT_RGB888,   {8, 8, 8, 0}, {16, 4}, {16, 4}, 0x2103, 0x3a, true },
	{ DRM_FORMAT_BGR888,   {8, 8, 8, 0}, {16, 4}, {16, 4}, 0x0123, 0x3a, true },

	{ DRM_FORMAT_RGB565,   {5, 6, 5, 0}, {16, 4}, {32, 4}, 0x2103, 0x05, false },
	{ DRM_FORMAT_BGR565,   {5, 6, 5, 0}, {16, 4}, {32, 4}, 0x0123, 0x05, false },

	{ DRM_FORMAT_ARGB4444, {4, 4, 4, 4}, {16, 4}, {32, 4}, 0x2103, 0x02, false },
	{ DRM_FORMAT_ABGR4444, {4, 4, 4, 4}, {16, 4}, {32, 4}, 0x0123, 0x02, false },
	{ DRM_FORMAT_RGBA4444, {4, 4, 4, 4}, {16, 4}, {32, 4}, 0x3210, 0x02, false },
	{ DRM_FORMAT_BGRA4444, {4, 4, 4, 4}, {16, 4}, {32, 4}, 0x1230, 0x02, false },

	/* { DRM_FORMAT_ARGB1555, {5, 5, 5, 1}, {16, 4}, {32, 4}, 0x2103, 0x53, false }, */
	/* { DRM_FORMAT_ABGR1555, {5, 5, 5, 1}, {16, 4}, {32, 4}, 0x0123, 0x53, false }, */
	{ DRM_FORMAT_RGBA5551, {5, 5, 5, 1}, {16, 4}, {32, 4}, 0x3210, 0x53, false },
	{ DRM_FORMAT_BGRA5551, {5, 5, 5, 1}, {16, 4}, {32, 4}, 0x1230, 0x53, false },

	{ DRM_FORMAT_ARGB2101010, {10, 10, 10, 2}, {16, 4}, {16, 4}, 0x2103, 0x0e, false },
	{ DRM_FORMAT_ABGR2101010, {10, 10, 10, 2}, {16, 4}, {16, 4}, 0x0123, 0x0e, false },
	/* { DRM_FORMAT_RGBA1010102, {10, 10, 10, 2}, {16, 4}, {16, 4}, 0x3210, 0x0e, false }, */
	/* { DRM_FORMAT_BGRA1010102, {10, 10, 10, 2}, {16, 4}, {16, 4}, 0x1230, 0x0e, false }, */

	/* ve does not currently support tfbc */
	/* { DRM_FORMAT_YUV422,   {8, 8, 8, 0}, {16, 4} }, */
	/* { DRM_FORMAT_NV61,     {8, 8, 8, 0}, {16, 4} }, */
	/* { DRM_FORMAT_NV16,     {8, 8, 8, 0}, {16, 4} }, */

	/* { DRM_FORMAT_YUV420,   {8, 8, 8, 0}, {16, 4} }, */
	/* { DRM_FORMAT_YVU420,   {8, 8, 8, 0}, {16, 4} }, */
	/* { DRM_FORMAT_NV12,     {8, 8, 8, 0}, {16, 4} }, */
};

static bool de352_tfbd_exist[] = {
	true, true, false, true, true, false, false,
};

static struct de_version_tfbd de352 = {
	.version = 0x352,
	.phy_chn_cnt = ARRAY_SIZE(de352_tfbd_exist),
	.tfbd_exist = de352_tfbd_exist,
};

static struct de_version_tfbd *de_version[] = {
	&de352
};

static bool is_tfbd_exist(struct module_create_info *info)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(de_version); i++) {
		if (de_version[i]->version == info->de_version) {
			return de_version[i]->tfbd_exist[info->id];
		}
	}
	return false;
}

static inline struct ovl_v_tfbd_reg *get_tfbd_reg(struct de_tfbd_private *priv)
{
	return (struct ovl_v_tfbd_reg *)(priv->reg_blks[0].vir_addr);
}

static void tfbd_set_block_dirty(
	struct de_tfbd_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DRM_ERROR("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

static s32 modifier2de_tfbc_lossy_rate(u64 modifier)
{
	switch (modifier) {
	case DRM_FORMAT_MOD_PVR_FBCDC_16x4_V13:
		return TFBC_LOSSLESS;
	case DRM_FORMAT_MOD_PVR_FBCDC_LOSSY25_16x4_V13:
		return TFBC_LOSSY25;
	case DRM_FORMAT_MOD_PVR_FBCDC_LOSSY50_16x4_V13:
		return TFBC_LOSSY50;
	case DRM_FORMAT_MOD_PVR_FBCDC_LOSSY75_16x4_V13:
		return TFBC_LOSSY75;
	default:
		return -1;
	}
}

static s32 format2de_tfbc_layout(u64 format, struct de_tfbd_drmformat_mapping *tfbd_layout)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(format2tfbc_mapping); i++) {
		if (format == format2tfbc_mapping[i].format) {
			memcpy(tfbd_layout, &format2tfbc_mapping[i], sizeof(*tfbd_layout));
			return 0;
		}
	}

	return -1;
}

bool de_tfbd_format_mod_supported(struct de_tfbd_handle *hdl, u32 format, u64 modifier)
{
	struct de_tfbd_drmformat_mapping tfbd_layout;

	if (modifier == DRM_FORMAT_MOD_INVALID)
		return false;

	if (format2de_tfbc_layout(format, &tfbd_layout) == -1) {
		DRM_DEBUG_DRIVER("[SUNXI-CRTC] get an drm format 0x%x, not support tfbc format\n", format);
		return false;
	}

	if (modifier2de_tfbc_lossy_rate(modifier) == -1) {
		DRM_DEBUG_DRIVER("[SUNXI-CRTC] get an drm modifier 0x%016llx,"
				 "not support tfbc modifier\n", modifier);
		return false;
	}

	if (!tfbd_layout.is_support_lossy && modifier != DRM_FORMAT_MOD_PVR_FBCDC_16x4_V13) {
		DRM_DEBUG_DRIVER("[SUNXI-CRTC] get an drm format 0x%x, not"
				 "support lossy modifier 0x%016llx\n", format, modifier);
		return false;
	}

	return true;
}

static s32 de_tfbd_disable(struct de_tfbd_handle *handle)
{
	struct de_tfbd_private *priv = handle->private;
	struct ovl_v_tfbd_reg *reg = get_tfbd_reg(priv);

	DRM_DEBUG_DRIVER("[SUNXI-DE] %s %d ch %d\n", __FUNCTION__, __LINE__, handle->cinfo.id);
	priv->debug.enable = false;
	reg->ctrl.dwval = 0;
	tfbd_set_block_dirty(priv, OVL_TFBD_REG_BLK, 1);
	return 0;
}

static s32 de_tfbd_get_info(const struct drm_framebuffer *fb, struct de_tfbd_info *info, struct de_tfbd_drmformat_mapping *layout)
{
	/*
	 * de spec stipulates that the alignment is 64, but in order to
	 * solve the problem of GPU channel imbalance, set the alignment to 256
	 */
#define HEADER_ALIGN (256)
	u32 width_align, height_align;

	if (format2de_tfbc_layout(fb->format->format, layout) == -1) {
		DRM_ERROR("[SUNXI-DE] get an drm format %d, not support tfbdc\n", fb->format->format);
		return -1;
	}

	info->lossy_rate = modifier2de_tfbc_lossy_rate(fb->modifier);

	width_align = (fb->width + (layout->memory_layout[0] - 1)) / layout->memory_layout[0];
	height_align = (fb->height + (layout->memory_layout[1] - 1)) / layout->memory_layout[1];
	info->y_header_size = ALIGN((width_align * height_align * 8), HEADER_ALIGN);
	// TODO: yuv stream from aw decoder, maybe need y_body_size, uv_header_size to get uv_addr offset

	DRM_DEBUG_DRIVER(
		"[SUNXI-CRTC] tfbc layout format=%d lossy_rate=%d%% memory_block[%dx%d] width=%d height=%d "
		"modifier=0x%016llx\n",
		fb->format->format, info->lossy_rate * 25, layout->memory_layout[0], layout->memory_layout[1],
		fb->width, fb->height, (unsigned long long)fb->modifier);

	return 0;
}

bool de_tfbd_should_enable(struct de_tfbd_handle *handle, struct display_channel_state *state)
{
	struct drm_framebuffer *fb = state->base.fb;
	return fb && de_tfbd_format_mod_supported(handle, fb ? fb->format->format : 0, fb->modifier);
}

int de_tfbd_apply_lay(struct de_tfbd_handle *handle, struct display_channel_state *state, struct de_tfbd_cfg *cfg, bool *is_enable)
{
	u32 dwval;
	u32 width, height, x, y;
	struct de_tfbd_info tfbd_info;
	struct de_tfbd_drmformat_mapping tfbd_layout;
	struct drm_framebuffer *fb = state->base.fb;
	struct de_tfbd_private *priv = handle->private;
	struct ovl_v_tfbd_reg *reg = get_tfbd_reg(priv);
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
	struct drm_gem_cma_object *gem;
#else
	struct drm_gem_dma_object *gem;
#endif
	u64 addr, y_addr;

	if (!state->base.fb || !de_tfbd_format_mod_supported(handle, fb ? fb->format->format : 0, fb->modifier)) {
		*is_enable = false;
		return de_tfbd_disable(handle);
	}

	de_tfbd_get_info(state->base.fb, &tfbd_info, &tfbd_layout);

	priv->debug.enable = true;
	memcpy(&priv->debug.tfbd_layout, &tfbd_layout, sizeof(tfbd_layout));
	memcpy(&priv->debug.tfbd_info, &tfbd_info, sizeof(tfbd_info));
	priv->debug.in_width = fb->width;
	priv->debug.in_height = fb->height;
	priv->debug.crop_r = (u32)((state->base.src_x) >> 16);
	priv->debug.crop_t = (u32)((state->base.src_y) >> 16);
	priv->debug.crop_w = (u32)((state->base.src_w) >> 16);
	priv->debug.crop_h = (u32)((state->base.src_h) >> 16);
	priv->debug.ovl_r = cfg->ovl_win.left;
	priv->debug.ovl_t = cfg->ovl_win.top;
	priv->debug.ovl_w = cfg->ovl_win.width;
	priv->debug.ovl_h = cfg->ovl_win.height;

	dwval = 1;
	dwval |= ((state->base.pixel_blend_mode != DRM_MODE_BLEND_PIXEL_NONE ? 2 : 1) << 2);
	dwval |= (state->base.alpha >> 8 << 24);
	dwval |= ((tfbd_layout.fmt & 0x7F) << 8);
	dwval |= ((tfbd_info.lossy_rate & 0x3) << 5);
	reg->ctrl.dwval = dwval;

	width = fb->width;
	height = fb->height;
	dwval = ((width ? (width - 1) : 0) & 0x1FFF) |
		(height ? (((height - 1) & 0x1FFF) << 16) : 0);
	reg->src_size.dwval = dwval;

	x = (u32)((state->base.src_x) >> 16);
	y = (u32)((state->base.src_y) >> 16);
	dwval = (x & 0x1FFF) | ((y & 0x1FFF) << 16);
	reg->crop_coor.dwval = dwval;

	width = (u32)((state->base.src_w) >> 16);
	height = (u32)((state->base.src_h) >> 16);
	dwval = ((width ? (width - 1) : 0) & 0x1FFF) |
		(height ? (((height - 1) & 0x1FFF) << 16) : 0);
	reg->crop_size.dwval = dwval;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
	gem = drm_fb_cma_get_gem_obj(fb, 0);
	if (gem) {
		addr = (u64)(gem->paddr) + fb->offsets[0];
#else
	gem = drm_fb_dma_get_gem_obj(fb, 0);
	if (gem) {
		addr = (u64)(gem->dma_addr) + fb->offsets[0];
#endif
	} else {
		*is_enable = false;
		DRM_ERROR("not found paddr for tfbc buffer\n");
		return -EINVAL;
	}

	y_addr = addr + tfbd_info.y_header_size;
	reg->y_laddr.dwval = (u32)(y_addr);
	reg->haddr.dwval = (u32)(((y_addr >> 32) & 0xFF));
	// TODO: yuv format support
	/* if (lay_info->fb.format >= DISP_FORMAT_YUV444_I_AYUV) { */
		/* uv_addr = y_addr + get_y_size(width, height, lay_info->fb.format); */
	/* } */
	/* reg->uv_laddr.dwval = (u32)(uv_addr); */
	/* reg->haddr.dwval = (u32)(((y_addr >> 32) & 0xFF) | (((uv_addr >> 32) & 0xFF) << 8)); */

	width = cfg->ovl_win.width;
	height = cfg->ovl_win.height;
	dwval = (width ? ((width - 1) & 0x1FFF) : 0) |
		(height ? (((height - 1) & 0x1FFF) << 16) : 0);
	reg->ovl_size.dwval = dwval;

	x = cfg->ovl_win.left;
	y = cfg->ovl_win.top;
	dwval = (x & 0x1FFF) | ((y & 0x1FFF) << 16);
	reg->ovl_coor.dwval = dwval;

	/* this will not be used by user */
	/* reg->fc.dwval = */
	    /* ((lay_info->mode == LAYER_MODE_BUFFER) ? 0 : lay_info->color); */

	reg->bgc.dwval = 0xFF000000;

	dwval = tfbd_layout.seq;
	reg->fmt_seq.dwval = dwval;

	tfbd_set_block_dirty(priv, OVL_TFBD_REG_BLK, 1);
	*is_enable = true;

	DRM_DEBUG_DRIVER("[SUNXI-DE] %s %d ch %d\n", __FUNCTION__, __LINE__, handle->cinfo.id);
	return 0;
}

struct de_tfbd_handle *de_tfbd_create(struct module_create_info *info)
{
	struct de_tfbd_handle *hdl;
	struct de_reg_block *block;
	struct de_reg_mem_info *reg_mem_info;
	u8 __iomem *reg_base;
	int i;
	struct de_tfbd_private *priv;

	if (!is_tfbd_exist(info))
		return NULL;

	reg_base = (u8 __iomem *)(info->de_reg_base + info->reg_offset + CHN_TFBD_OFFSET);

	hdl = kmalloc(sizeof(*hdl), GFP_KERNEL | __GFP_ZERO);

	memcpy(&hdl->cinfo, info, sizeof(*info));
	hdl->format_modifiers = format_modifiers_tfbc;
	hdl->format_modifiers_num = ARRAY_SIZE(format_modifiers_tfbc) - 1;
	hdl->private = kmalloc(sizeof(*hdl->private), GFP_KERNEL | __GFP_ZERO);
	priv = hdl->private;
	reg_mem_info = &(hdl->private->reg_mem_info);

	reg_mem_info->size = sizeof(struct ovl_v_tfbd_reg);
	reg_mem_info->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
	    reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
	    info->update_mode == RCQ_MODE);
	if (NULL == reg_mem_info->vir_addr) {
		DRM_ERROR("alloc tfbd %d mm fail!size=0x%x\n",
		       info->id, reg_mem_info->size);
		return ERR_PTR(-ENOMEM);
	}
	block = &(priv->reg_blks[OVL_TFBD_REG_BLK]);
	block->phy_addr = reg_mem_info->phy_addr;
	block->vir_addr = reg_mem_info->vir_addr;
	block->size = sizeof(struct ovl_v_tfbd_reg);
	block->reg_addr = reg_base;
	priv->reg_blk_num = OVL_TFBD_REG_BLK_NUM;

	hdl->block_num = priv->reg_blk_num;
	hdl->block = kmalloc(sizeof(block[0]) * hdl->block_num, GFP_KERNEL | __GFP_ZERO);
	for (i = 0; i < hdl->private->reg_blk_num; i++)
		hdl->block[i] = &priv->reg_blks[i];
	return hdl;
}

void de_dump_tfbd_state(struct drm_printer *p, struct de_tfbd_handle *handle, const struct display_channel_state *state)
{
	struct de_tfbd_debug_info *debug = &handle->private->debug;
	unsigned long base = (unsigned long)handle->private->reg_blks[0].reg_addr;
	unsigned long de_base = (unsigned long)handle->cinfo.de_reg_base;

	drm_printf(p, "\n\ttfbd@%8x: %sable\n", (unsigned int)(base - de_base), debug->enable ? "en" : "dis");
	if (debug->enable) {
		drm_printf(p, "\t\tformat: %p4cc lossy: %d layout: %dx%d\n", &debug->tfbd_layout.format,
			   debug->tfbd_info.lossy_rate * 25,
			   debug->tfbd_layout.memory_layout[0], debug->tfbd_layout.memory_layout[1]);
		drm_printf(p, "\t\tin(%4dx%4d) ==> c(%4dx%4d+%4d+%4d) ==> out(%4dx%4d)\n",
			   debug->in_width, debug->in_height,
			   debug->crop_w, debug->crop_h, debug->crop_r, debug->crop_t,
			   debug->ovl_w, debug->ovl_h);
	}
}
