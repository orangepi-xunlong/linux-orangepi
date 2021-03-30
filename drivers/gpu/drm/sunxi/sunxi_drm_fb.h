/*
 * Copyright (C) 2016 Allwinnertech Co.Ltd
 * Authors: Jet Cui <cuiyuntao@allwinnter.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#ifndef _SUNXI_DRM_FB_H_
#define _SUNXI_DRM_FB_H_

#include "sunxi_drm_gem.h"
/* fix the drm_mode_fb_cmd2 may add 4 handle */
#define MAX_FB_BUFFER 4
#define PREFERRED_BPP 32
/*
 * sunxi specific framebuffer structure.
 *
 * @fb: drm framebuffer obejct.
 * @buf_cnt: a buffer count to drm framebuffer.
 * @sunxi_gem_obj: array of sunxi specific gem object containing a gem object.
 */

#define SUNXI_FBDEV_FLAGS (1<<0)

#define FBDEV_BUF_NUM 2

struct sunxi_drm_framebuffer {
	struct drm_framebuffer drm_fb;
	struct drm_gem_object *gem_obj[MAX_FB_BUFFER];
	int nr_gem;
	unsigned int fb_flag;
};

#define to_sunxi_fb(x)	container_of(x, struct sunxi_drm_framebuffer, drm_fb)

void sunxi_drm_mode_config_init(struct drm_device *dev);

struct sunxi_drm_gem_buf *sunxi_drm_framebuffer_buffer(struct drm_framebuffer *fb, int index);

struct drm_framebuffer *sunxi_drm_framebuffer_creat(struct drm_device *dev,
	struct drm_mode_fb_cmd2 *mode_cmd, struct drm_gem_object *obj[MAX_FB_BUFFER], int nr_gem, int f);

#endif
