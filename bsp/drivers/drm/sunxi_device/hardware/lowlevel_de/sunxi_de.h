/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2023 Allwinnertech Co.Ltd
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
*/
#ifndef _SUNXI_DE_H_
#define _SUNXI_DE_H_

#include "de_channel.h"

struct sunxi_plane_info {
	const char *name;
	bool is_primary;
	struct de_channel_handle *hdl;
	bool afbc_rot_support;
	unsigned int index;
	const uint32_t *formats;
	unsigned int format_count;
	const uint64_t *format_modifiers;
	unsigned int layer_cnt;
};

struct sunxi_de_info {
	struct drm_device *drm;
	struct sunxi_de_out *de_out;
	struct device_node *port;
	unsigned int gamma_lut_len;
	int hue_default_value;
	unsigned long clk_freq;
	struct de_disp_feature feat;
	unsigned int hw_id;
	unsigned int plane_cnt;
	struct sunxi_plane_info *planes;
};

struct sunxi_de_wb_info {
	struct drm_device *drm;
	struct sunxi_de_wb *wb;
	unsigned int support_disp_mask;
};

struct sunxi_de_out_cfg {
	bool sw_enable;
	unsigned int kHZ_pixelclk;
	unsigned int htotal;
	unsigned int vtotal;
	unsigned int hwdev_index;
	unsigned int width, height;
	unsigned int device_fps;
	unsigned int max_device_fps;
	unsigned int pixel_mode;
	unsigned int interlaced;
	enum de_format_space px_fmt_space;
	enum de_yuv_sampling yuv_sampling;
	enum de_eotf eotf;
	enum de_color_space color_space;
	enum de_color_range color_range;
	enum de_data_bits data_bits;
};

struct sunxi_de_flush_cfg {
	struct drm_color_lut *gamma_lut;
	bool gamma_dirty;
	unsigned int brightness, contrast, saturation, hue;
	bool bcsh_dirty;
	struct de_color_ctm *ctm;
	bool ctm_dirty;
};

struct sunxi_de_channel_update {
	struct de_channel_handle *hdl;
	struct sunxi_de_out *hwde;
	struct display_channel_state *new_state;
	struct display_channel_state *old_state;
	/* this channel is fbdev or not*/
	bool is_fbdev;
	/* fbdev output in current de */
	bool fbdev_output;
	bool force;
};

#define FORCE_ATOMIC_FLUSH	0xffff

int sunxi_de_event_proc(struct sunxi_de_out *hwde, bool timeout);
void sunxi_de_atomic_begin(struct sunxi_de_out *hwde);
void sunxi_de_atomic_flush(struct sunxi_de_out *hwde, struct de_backend_data *data, struct sunxi_de_flush_cfg *cfg);
int sunxi_de_enable(struct sunxi_de_out *hwde, const struct sunxi_de_out_cfg *cfg);
void sunxi_de_disable(struct sunxi_de_out *hwde);
int sunxi_de_channel_update(struct sunxi_de_channel_update *info);
bool sunxi_de_format_mod_supported(struct sunxi_de_out *hwde, struct de_channel_handle *hdl,
					    uint32_t format, uint64_t modifier);

int sunxi_de_backend_get_pqd_config(struct sunxi_de_out *hwde, struct de_backend_data *data);
int sunxi_de_write_back(struct sunxi_de_out *hwde, struct sunxi_de_wb *wb, struct drm_framebuffer *fb);
void sunxi_de_dump_channel_state(struct drm_printer *p, struct sunxi_de_out *hwde, struct de_channel_handle *hdl, const struct display_channel_state *state, bool state_only);
void sunxi_de_dump_state(struct drm_printer *p, struct sunxi_de_out *hwde);

bool sunxi_de_query_de_busy(struct sunxi_de_out *hwde);
unsigned long sunxi_de_get_clk(void);
void sunxi_de_set_devfreq_auto(bool en);
int sunxi_de_set_clk(unsigned long clk);
int sunxi_de_auto_calc_freq_and_apply(struct sunxi_de_out *hwde);
int sunxi_de_div_calc_mn(unsigned long freq_in_kHZ, unsigned long freq_out_kHZ, unsigned int *m, unsigned int *n);

#endif
