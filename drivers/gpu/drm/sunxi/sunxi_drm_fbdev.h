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
#ifndef _SUNXI_DRM_FBDEV_H_
#define _SUNXI_DRM_FBDEV_H_
#include <drm/drm_fb_helper.h>


struct sunxi_drm_fbdev {
        struct drm_fb_helper drm_fb_helper;
};

#define to_sunxi_fbdev(x)	container_of(x, struct sunxi_drm_fbdev,\
				drm_fb_helper)

void sunxi_drm_fbdev_restore_mode(struct drm_device *dev);

void sunxi_drm_fbdev_destroy(struct drm_device *dev);

int sunxi_drm_fbdev_creat(struct drm_device *dev);

#endif
