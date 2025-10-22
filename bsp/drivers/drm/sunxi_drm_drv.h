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

#ifndef _SUNXI_DRM_DRV_H_
#define _SUNXI_DRM_DRV_H_

#include <drm/drm_drv.h>
#include <drm/drm_connector.h>
#include <drm/drm_encoder.h>

#define OVL_MAX				4
#define OVL_REMAIN			(OVL_MAX - 1)

struct sunxi_drm_pri;
struct drm_connector;

struct sunxi_logo_info {
	unsigned int phy_addr;
	unsigned int width;
	unsigned int height;
	unsigned int bpp;
/*	unsigned int stride;
	unsigned int crop_l;
	unsigned int crop_t;
	unsigned int crop_r;
	unsigned int crop_b;*/
};

struct sunxi_drm_device {
	struct drm_connector connector;
	struct drm_encoder encoder;
	struct drm_device *drm_dev;
	struct drm_panel *panel;
	struct drm_bridge *bridge;
	struct device *tcon_dev;
	struct device *video_sys_dev;
	unsigned int tcon_id;
	unsigned int hw_id;
};

struct sunxi_drm_private {
	struct drm_device base;
//	struct display_boot_info boot[BOOT_OUTPUT_MAX];
	struct drm_property *prop_blend_mode[OVL_REMAIN];
	struct drm_property *prop_alpha[OVL_REMAIN];
	struct drm_property *prop_src_x[OVL_REMAIN], *prop_src_y[OVL_REMAIN];
	struct drm_property *prop_src_w[OVL_REMAIN], *prop_src_h[OVL_REMAIN];
	struct drm_property *prop_crtc_x[OVL_REMAIN], *prop_crtc_y[OVL_REMAIN];
	struct drm_property *prop_crtc_w[OVL_REMAIN], *prop_crtc_h[OVL_REMAIN];
	struct drm_property *prop_fb_id[OVL_REMAIN];
	struct drm_property *prop_color[OVL_MAX];
	struct drm_property *prop_layer_id;
	struct drm_property *prop_frontend_data;
	struct drm_property *prop_backend_data;
	struct drm_property *prop_sunxi_ctm;
	struct drm_property *prop_feature;
	struct drm_property *prop_eotf;
	struct drm_property *prop_color_space;
	struct drm_property *prop_color_format;
	struct drm_property *prop_color_depth;
	struct drm_property *prop_color_range;
	struct drm_property *prop_frame_rate_change;
	struct drm_property *prop_compressed_image_crop;
	struct sunxi_drm_pri *priv;
};

#define to_sunxi_drm_private(drm) container_of(drm, struct sunxi_drm_private, base)

bool sunxi_drm_check_if_need_sw_enable(struct drm_connector *connector);
bool sunxi_drm_check_device_boot_enabled(struct drm_device *dev,
			unsigned int connector_type, unsigned int connector_id);
bool sunxi_drm_check_tcon_top_boot_enabled(struct drm_device *drm, unsigned int tcon_top_id);
bool sunxi_drm_check_de_boot_enabled(struct drm_device *drm, unsigned int de_id);
int sunxi_drm_get_logo_info(struct drm_device *dev, struct sunxi_logo_info *logo,
			    unsigned int *scn_w, unsigned int *scn_h);
int sunxi_drm_get_device_max_fps(struct drm_device *drm);
unsigned int sunxi_drm_get_de_max_freq(struct drm_device *drm);
void sunxi_drm_signal_sw_enable_done(struct drm_crtc *crtc);
struct proc_dir_entry *sunxi_drm_get_procfs_dir(void);

#endif
