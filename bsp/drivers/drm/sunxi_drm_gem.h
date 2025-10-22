/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/* sunxi_drm_gem.h
 *
 * Copyright (C) 2023 Allwinnertech Co.Ltd
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef _SUNXI_DRM_GEM_H
#define _SUNXI_DRM_GEM_H

#include <linux/version.h>
#include <linux/dma-direction.h>
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_gem_cma_helper.h>
#else
#include <drm/drm_fb_dma_helper.h>
#include <drm/drm_gem_dma_helper.h>
#endif

#define to_sunxi_gem_obj(x) container_of(x, struct sunxi_gem_object, base)

struct sunxi_gem_object {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
	struct drm_gem_cma_object base;
#else
	struct drm_gem_dma_object base;
#endif
	enum dma_data_direction dir;
};

struct drm_gem_object *sunxi_gem_create_object(struct drm_device *dev, size_t size);


#endif /* _SUNXI_DRM_GEM_H */
