/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/* sunxi_drm_gem.c
 *
 * Copyright (C) 2023 Allwinnertech Co.Ltd
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/dma-buf.h>
#include <linux/dma-direction.h>
#include <drm/drm.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem.h>
#include <drm/drm_prime.h>

#include "sunxi_drm_gem.h"


/**
 * sunxi_drm_prime_gem_destroy - helper to clean up a PRIME-imported GEM object
 * @obj: GEM object which was created from a dma-buf
 * @sg: the sg-table which was pinned at import time
 *
 * This is the cleanup functions which GEM drivers need to call when they use
 * drm_gem_prime_import() or drm_gem_prime_import_dev() to import dma-bufs.
 */
void sunxi_drm_prime_gem_destroy(struct drm_gem_object *obj,
				 struct sg_table *sg, enum dma_data_direction dir)
{
	struct dma_buf_attachment *attach;
	struct dma_buf *dma_buf;

	attach = obj->import_attach;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
	if (sg)
		dma_buf_unmap_attachment(attach, sg, dir);
#else
	if (sg)
		dma_buf_unmap_attachment_unlocked(attach, sg, dir);
#endif
	dma_buf = attach->dmabuf;
	dma_buf_detach(attach->dmabuf, attach);
	/* remove the reference */
	dma_buf_put(dma_buf);
}


#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)

/**
 * sunxi_gem_cma_free - free resources associated with a CMA GEM object
 * @gem_obj: GEM object to free
 *
 * This function frees the backing memory of the CMA GEM object, cleans up the
 * GEM object state and frees the memory used to store the object itself.
 * If the buffer is imported and the virtual address is set, it is released.
 * Drivers using the CMA helpers should set this as their
 * &drm_gem_object_funcs.free callback.
 */
void sunxi_gem_cma_free(struct sunxi_gem_object *sgem_obj)
{
	struct drm_gem_cma_object *cma_obj = &sgem_obj->base;
	struct drm_gem_object *gem_obj = &cma_obj->base;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0) &&\
	LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0)
	struct dma_buf_map map = DMA_BUF_MAP_INIT_VADDR(cma_obj->vaddr);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0)
	struct iosys_map map = IOSYS_MAP_INIT_VADDR(cma_obj->vaddr);
#endif

	if (gem_obj->import_attach) {
		if (cma_obj->vaddr)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
			dma_buf_vunmap(gem_obj->import_attach->dmabuf, &map);
#else
			dma_buf_vunmap(gem_obj->import_attach->dmabuf, cma_obj->vaddr);
#endif
		sunxi_drm_prime_gem_destroy(gem_obj, cma_obj->sgt, sgem_obj->dir);
	} else if (cma_obj->vaddr) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
		if (cma_obj->map_noncoherent)
			dma_free_noncoherent(gem_obj->dev->dev, cma_obj->base.size,
					     cma_obj->vaddr, cma_obj->paddr,
					     DMA_TO_DEVICE);
		else
#endif
			dma_free_wc(gem_obj->dev->dev, cma_obj->base.size,
				    cma_obj->vaddr, cma_obj->paddr);
	}

	drm_gem_object_release(gem_obj);

	kfree(sgem_obj);
}

static inline void sunxi_drm_gem_cma_object_free(struct drm_gem_object *obj)
{
	struct drm_gem_cma_object *cma_obj = to_drm_gem_cma_obj(obj);
	struct sunxi_gem_object *sgem_obj = to_sunxi_gem_obj(cma_obj);

	sunxi_gem_cma_free(sgem_obj);
}

static const struct drm_gem_object_funcs sunxi_gem_funcs = {
	.free = sunxi_drm_gem_cma_object_free,
	.print_info = drm_gem_cma_print_info,
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 11, 0)
	.get_sg_table = drm_gem_cma_get_sg_table,
	.vmap = drm_gem_cma_vmap,
	.mmap = drm_gem_cma_mmap,
#else
	.get_sg_table = drm_gem_cma_prime_get_sg_table,
	.vmap = drm_gem_cma_prime_vmap,
#endif
	.vm_ops = &drm_gem_cma_vm_ops,
};
#else
/**
 * sunxi_gem_dma_free - free resources associated with a DMA GEM object
 * @dma_obj: DMA GEM object to free
 *
 * This function frees the backing memory of the DMA GEM object, cleans up the
 * GEM object state and frees the memory used to store the object itself.
 * If the buffer is imported and the virtual address is set, it is released.
 */
void sunxi_gem_dma_free(struct sunxi_gem_object *sgem_obj)
{
	struct drm_gem_dma_object *dma_obj = &sgem_obj->base;
	struct drm_gem_object *gem_obj = &dma_obj->base;
	struct iosys_map map = IOSYS_MAP_INIT_VADDR(dma_obj->vaddr);

	if (gem_obj->import_attach) {
		if (dma_obj->vaddr)
			dma_buf_vunmap_unlocked(gem_obj->import_attach->dmabuf, &map);
		sunxi_drm_prime_gem_destroy(gem_obj, dma_obj->sgt, sgem_obj->dir);
	} else if (dma_obj->vaddr) {
		if (dma_obj->map_noncoherent)
			dma_free_noncoherent(gem_obj->dev->dev, dma_obj->base.size,
					     dma_obj->vaddr, dma_obj->dma_addr,
					     DMA_TO_DEVICE);
		else
			dma_free_wc(gem_obj->dev->dev, dma_obj->base.size,
				    dma_obj->vaddr, dma_obj->dma_addr);
	}

	drm_gem_object_release(gem_obj);

	kfree(sgem_obj);
}

static inline void sunxi_drm_gem_dma_object_free(struct drm_gem_object *obj)
{
	struct drm_gem_dma_object *dma_obj = to_drm_gem_dma_obj(obj);
	struct sunxi_gem_object *sgem_obj = to_sunxi_gem_obj(dma_obj);

	sunxi_gem_dma_free(sgem_obj);
}

static const struct drm_gem_object_funcs sunxi_gem_funcs = {
	.free = sunxi_drm_gem_dma_object_free,
	.print_info = drm_gem_dma_object_print_info,
	.get_sg_table = drm_gem_dma_object_get_sg_table,
	.vmap = drm_gem_dma_object_vmap,
	.mmap = drm_gem_dma_object_mmap,
	.vm_ops = &drm_gem_dma_vm_ops,
};
#endif

struct drm_gem_object *sunxi_gem_create_object(struct drm_device *dev, size_t size)
{
	struct sunxi_gem_object *sgem_obj;

	sgem_obj = kzalloc(sizeof(*sgem_obj), GFP_KERNEL);
	if (!sgem_obj)
		return ERR_PTR(-ENOMEM);

	/*
	 * set DMA_TO_DEVICE as default, and might be changed when dma-buf use as
	 * writeback output, to ensure cache coherence
	 */
	sgem_obj->dir = DMA_TO_DEVICE;
	sgem_obj->base.base.funcs = &sunxi_gem_funcs;

	return &sgem_obj->base.base;
}
