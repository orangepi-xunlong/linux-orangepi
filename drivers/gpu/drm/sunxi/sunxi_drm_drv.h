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
#ifndef _SUNXI_DRM_DRV_H_
#define _SUNXI_DRM_DRV_H_

#include <drm/drm_crtc.h>
#include <drm/drm_fb_helper.h>

struct sunxi_drm_private {
	struct drm_fb_helper *fb_helper;
	struct drm_property *crtc_mode_property;
	struct sunxi_rotate_private *rotate_private;
};

#endif

