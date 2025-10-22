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

#ifndef _SUNXI_DRM_CRTC_H_
#define _SUNXI_DRM_CRTC_H_

#include <drm/drm_crtc.h>
#include "sunxi_device/hardware/lowlevel_de/sunxi_de.h"

typedef void (*vblank_enable_callback_t)(bool, void *);
typedef bool (*fifo_status_check_callback_t)(void *);
typedef bool (*is_sync_time_enough_callback_t)(void *);
typedef int (*get_cur_line_callback_t)(void *);
typedef void (*connector_atomic_flush)(void *);
// backlight
typedef bool (*is_support_backlight_callback_t)(void *);
typedef void (*set_backlight_value_callback_t)(void *, int);
typedef int (*get_backlight_value_callback_t)(void *);

struct sunxi_drm_crtc;

struct de_out_exconfig {
	struct drm_color_lut *gamma_lut;
	unsigned int brightness, contrast, saturation, hue;
};

struct sunxi_crtc_state {
	struct drm_crtc_state base;
	enum de_format_space px_fmt_space;
	enum de_yuv_sampling yuv_sampling;
	enum de_eotf eotf;
	enum de_color_space color_space;
	enum de_color_range color_range;
	enum de_data_bits data_bits;
	struct drm_property_blob *backend_blob;
	struct drm_property_blob *sunxi_ctm;
	unsigned int tcon_id;
	unsigned long clk_freq;
	unsigned int pixel_mode;
	struct de_out_exconfig excfg;
	bool bcsh_changed;
	bool sunxi_ctm_changed;
	bool frame_rate_change;
	bool sw_enable;
	vblank_enable_callback_t enable_vblank;
	fifo_status_check_callback_t check_status;
	is_sync_time_enough_callback_t is_sync_time_enough;
	get_cur_line_callback_t get_cur_line;
	connector_atomic_flush  atomic_flush;
	is_support_backlight_callback_t is_support_backlight;
	set_backlight_value_callback_t set_backlight_value;
	get_backlight_value_callback_t get_backlight_value;
	void *output_dev_data;
	struct sunxi_drm_wb *wb;
};

struct fbdev_config {
	struct drm_device *dev;
	unsigned int de_id;
	unsigned int channel_id;
	bool force;
	const struct display_channel_state *fake_state;
	struct drm_plane **out_plane;
	struct drm_crtc **out_crtc;
};

#define to_sunxi_crtc_state(x) container_of(x, struct sunxi_crtc_state, base)

irqreturn_t sunxi_crtc_event_proc(int irq, void *crtc);
int sunxi_drm_crtc_get_hw_id(struct drm_crtc *crtc);
unsigned int sunxi_drm_crtc_get_clk_freq(struct drm_crtc *crtc);
void sunxi_plane_print_state(struct drm_printer *p,
				   const struct drm_plane_state *state, bool state_only);

int sunxi_fbdev_plane_update(struct fbdev_config *config);

void sunxi_drm_crtc_prepare_vblank_event(struct sunxi_drm_crtc *scrtc);
void sunxi_drm_crtc_wait_one_vblank(struct sunxi_drm_crtc *scrtc);
int sunxi_drm_crtc_get_output_current_line(struct sunxi_drm_crtc *scrtc);
bool sunxi_drm_crtc_is_support_backlight(struct sunxi_drm_crtc *scrtc);
int sunxi_drm_crtc_get_backlight(struct sunxi_drm_crtc *scrtc);
void sunxi_drm_crtc_set_backlight_value(struct sunxi_drm_crtc *scrtc, int backlight);

int sunxi_drm_crtc_pq_proc(struct drm_device *dev, int disp, enum sunxi_pq_type, void *data);

#endif
