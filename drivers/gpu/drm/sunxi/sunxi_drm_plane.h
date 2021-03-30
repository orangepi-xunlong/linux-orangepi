/*
 * Copyright (C) 2016 Allwinnertech Co.Ltd
 * Authors: Jet Cui 
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
/*plane --> Layer*/

#ifndef _SUNXI_DRM_PLANE_H_
#define _SUNXI_DRM_PLANE_H_

#include "de/include.h"
#include "sunxi_drm_gem.h"

struct sunxi_drm_plane {
	struct drm_plane	drm_plane;
	struct drm_framebuffer *old_fb;
	struct mutex delayed_work_lock;
	int crtc_id;
	int chn_id;
	int plane_id;
	bool isvideo;
	bool updata_frame;
	bool delayed_updata;
	struct disp_layer_config_data *plane_cfg;
	struct sunxi_hardware_res *hw_res;
};

#define to_sunxi_plane(x)	container_of(x, struct sunxi_drm_plane, drm_plane)

int sunxi_set_fb_plane(struct drm_crtc *crtc, unsigned int plane_id, unsigned int zoder);

struct drm_plane *sunxi_plane_init(struct drm_device *dev,struct drm_crtc *crtc,
	struct disp_layer_config_data *cfg, int chn, int plane_id, bool priv);

int sunxi_update_plane(struct drm_plane *plane, struct drm_crtc *crtc,
		struct drm_framebuffer *fb, int crtc_x, int crtc_y,
		unsigned int crtc_w, unsigned int crtc_h,
		uint32_t src_x, uint32_t src_y,
		uint32_t src_w, uint32_t src_h);

void sunxi_plane_dpms(struct drm_plane *plane, int status);

void sunxi_reset_plane(struct drm_crtc *crtc);

#endif
