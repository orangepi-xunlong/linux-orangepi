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
 
#ifndef     SUNXI_GEM_DMABUF_H_
#define     SUNXI_GEM_DMABUF_H_
#include <linux/dma-buf.h>

struct sunxi_gem_dmabuf_attachment {
	struct sg_table sgt;
	enum dma_data_direction dir;
	bool is_mapped;
};

struct dma_buf *sunxi_dmabuf_prime_export(struct drm_device *drm_dev,
			struct drm_gem_object *obj, int flags);

struct drm_gem_object *sunxi_dmabuf_prime_import(struct drm_device *drm_dev,
	struct dma_buf *dma_buf);

#endif