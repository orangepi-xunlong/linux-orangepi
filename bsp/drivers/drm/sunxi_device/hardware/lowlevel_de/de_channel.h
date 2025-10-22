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
#ifndef _DE_CHANNEL_H_
#define _DE_CHANNEL_H_

#include <drm/drm_plane.h>
#include <drm/drm_plane.h>
#include <video/sunxi_drm.h>
#include "de_base.h"

#define MAX_LAYER_NUM_PER_CHN		(4)
#define COMMIT_ALL_LAYER		(0xff)

enum de_pixel_format {
	DE_FORMAT_ARGB_8888 = 0x00, /*MSB  A-R-G-B  LSB */
	DE_FORMAT_ABGR_8888 = 0x01,
	DE_FORMAT_RGBA_8888 = 0x02,
	DE_FORMAT_BGRA_8888 = 0x03,
	DE_FORMAT_XRGB_8888 = 0x04,
	DE_FORMAT_XBGR_8888 = 0x05,
	DE_FORMAT_RGBX_8888 = 0x06,
	DE_FORMAT_BGRX_8888 = 0x07,
	DE_FORMAT_RGB_888 = 0x08,
	DE_FORMAT_BGR_888 = 0x09,
	DE_FORMAT_RGB_565 = 0x0a,
	DE_FORMAT_BGR_565 = 0x0b,
	DE_FORMAT_ARGB_4444 = 0x0c,
	DE_FORMAT_ABGR_4444 = 0x0d,
	DE_FORMAT_RGBA_4444 = 0x0e,
	DE_FORMAT_BGRA_4444 = 0x0f,
	DE_FORMAT_ARGB_1555 = 0x10,
	DE_FORMAT_ABGR_1555 = 0x11,
	DE_FORMAT_RGBA_5551 = 0x12,
	DE_FORMAT_BGRA_5551 = 0x13,
	DE_FORMAT_ARGB_2101010 = 0x14,
	DE_FORMAT_ABGR_2101010 = 0x15,
	DE_FORMAT_RGBA_1010102 = 0x16,
	DE_FORMAT_BGRA_1010102 = 0x17,

	/*
	* SP: semi-planar, P:planar, I:interleaved
	* UVUV: U in the LSBs;     VUVU: V in the LSBs
	*/
	DE_FORMAT_YUV444_I_AYUV = 0x40,
	DE_FORMAT_YUV444_I_VUYA = 0x41,
	DE_FORMAT_YUV422_I_YVYU = 0x42,
	DE_FORMAT_YUV422_I_YUYV = 0x43,
	DE_FORMAT_YUV422_I_UYVY = 0x44,
	DE_FORMAT_YUV422_I_VYUY = 0x45,
	DE_FORMAT_YUV444_P = 0x46,
	DE_FORMAT_YUV422_P = 0x47,
	DE_FORMAT_YVU422_P = 0x48,
	DE_FORMAT_YUV420_P = 0x49,
	DE_FORMAT_YVU420_P = 0x4a,
	DE_FORMAT_YUV411_P = 0x4b,
	DE_FORMAT_YVU411_P = 0x4c,
	DE_FORMAT_YUV422_SP_UVUV = 0x4d,
	DE_FORMAT_YUV422_SP_VUVU = 0x4e,
	DE_FORMAT_YUV420_SP_UVUV = 0x4f,
	DE_FORMAT_YUV420_SP_VUVU = 0x50,
	DE_FORMAT_YUV411_SP_UVUV = 0x51,
	DE_FORMAT_YUV411_SP_VUVU = 0x52,
	DE_FORMAT_8BIT_GRAY                    = 0x53,
	DE_FORMAT_YUV444_I_AYUV_10BIT          = 0x54,
	DE_FORMAT_YUV444_I_VUYA_10BIT          = 0x55,
	DE_FORMAT_YUV422_I_YVYU_10BIT          = 0x56,
	DE_FORMAT_YUV422_I_YUYV_10BIT          = 0x57,
	DE_FORMAT_YUV422_I_UYVY_10BIT          = 0x58,
	DE_FORMAT_YUV422_I_VYUY_10BIT          = 0x59,
	DE_FORMAT_YUV444_P_10BIT               = 0x5a,
	DE_FORMAT_YUV422_P_10BIT               = 0x5b,
	DE_FORMAT_YUV420_P_10BIT               = 0x5c,
	DE_FORMAT_YUV411_P_10BIT               = 0x5d,
	DE_FORMAT_YUV422_SP_UVUV_10BIT         = 0x5e,
	DE_FORMAT_YUV422_SP_VUVU_10BIT         = 0x5f,
	DE_FORMAT_YUV420_SP_UVUV_10BIT         = 0x60,
	DE_FORMAT_YUV420_SP_VUVU_10BIT         = 0x61,
	DE_FORMAT_YUV411_SP_UVUV_10BIT         = 0x62,
	DE_FORMAT_YUV411_SP_VUVU_10BIT         = 0x63,
};

struct display_channel_state {
	struct drm_plane_state base;
	bool fake_layer0;
	unsigned int layer_id;
	u16 alpha[MAX_LAYER_NUM_PER_CHN - 1];
	uint16_t pixel_blend_mode[MAX_LAYER_NUM_PER_CHN - 1];
	uint32_t src_x[MAX_LAYER_NUM_PER_CHN - 1], src_y[MAX_LAYER_NUM_PER_CHN - 1];
	uint32_t src_w[MAX_LAYER_NUM_PER_CHN - 1], src_h[MAX_LAYER_NUM_PER_CHN - 1];
	uint32_t crtc_x[MAX_LAYER_NUM_PER_CHN - 1], crtc_y[MAX_LAYER_NUM_PER_CHN - 1];
	uint32_t crtc_w[MAX_LAYER_NUM_PER_CHN - 1], crtc_h[MAX_LAYER_NUM_PER_CHN - 1];
	struct drm_framebuffer *fb[MAX_LAYER_NUM_PER_CHN - 1];
	uint32_t color[MAX_LAYER_NUM_PER_CHN];
	struct drm_property_blob *frontend_blob;
	enum de_eotf eotf;
	enum de_color_space color_space;
	enum de_color_range color_range;
	/*
	 * create image crop for afbc compressed buffer, top_crop and left_crop,
	 *  top_crop : 0 ~ 15
	 *  left_crop: 0 ~ 63
	 *  value = (top_crop << 16) | left_crop
	 */
	uint32_t compressed_image_crop;
};

#define to_display_channel_state(x)		container_of(x, struct display_channel_state, base)

struct de_channel_handle {
	struct module_create_info cinfo;
	const char *name;
	bool routine_job;
	const uint32_t *formats;
	bool afbc_rot_support;
	unsigned int exclusive_scaler_ids;
	unsigned int format_count;
	unsigned int layer_cnt;
	uint64_t *format_modifiers_comb;
	struct de_channel_private *private;
	unsigned int block_num;
	struct de_reg_block **block;
	bool is_video;
	unsigned int type_hw_id;
	struct de_chn_mod_support mod;
	struct de_channel_linebuf_feature lbuf;
};

struct de_output_info {
	unsigned int width;
	unsigned int height;
	unsigned int htotal;
	unsigned int pclk_khz;
	unsigned int device_fps;
	unsigned int max_device_fps;
	unsigned int interlaced;
	unsigned long de_clk_freq;
	enum de_format_space px_fmt_space;
	enum de_yuv_sampling yuv_sampling;
	enum de_eotf eotf;
	enum de_color_space color_space;
	enum de_color_range color_range;
	enum de_data_bits data_bits;
};

struct de_channel_output_info {
	bool is_premul;
	struct drm_rect disp_win;
};

int channel_apply(struct de_channel_handle *hdl, struct display_channel_state *state,
		    const struct de_output_info *de_info, struct de_channel_output_info *output, bool rgb_out);
void channel_update_regs(struct de_channel_handle *hdl);
void channel_process_late(struct de_channel_handle *hdl);

struct de_channel_handle *de_channel_create(struct module_create_info *cinfo);
bool channel_format_mod_supported(struct de_channel_handle *hdl, uint32_t format, uint64_t modifier);
void dump_channel_state(struct drm_printer *p, struct de_channel_handle *hdl, const struct display_channel_state *state, bool state_only);
int channel_get_pqd_config(struct de_channel_handle *hdl, struct display_channel_state *cstate);
int get_size_by_chn(struct de_channel_handle *hdl, int *width, int *height);
unsigned int get_chn_size_on_scn(struct de_channel_handle *hdl, int *width, int *height);

#endif
