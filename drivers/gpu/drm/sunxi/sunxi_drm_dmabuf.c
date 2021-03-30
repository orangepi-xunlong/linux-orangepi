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

#include <drm/drmP.h>
#include <drm/sunxi_drm.h>
#include <linux/dma-buf.h>

#include "sunxi_drm_drv.h"
#include "sunxi_drm_gem.h"
#include "sunxi_drm_dmabuf.h"

static int sunxi_gem_attach_dma_buf(struct dma_buf *dmabuf,
		struct device *dev, struct dma_buf_attachment *attach)
{
	struct sunxi_gem_dmabuf_attachment *sunxi_attach;

	sunxi_attach = kzalloc(sizeof(*sunxi_attach), GFP_KERNEL);
	if (!sunxi_attach)
		return -ENOMEM;

	sunxi_attach->dir = DMA_NONE;
	attach->priv = sunxi_attach;

	return 0;
}

static void sunxi_gem_detach_dma_buf(struct dma_buf *dmabuf,
					struct dma_buf_attachment *attach)
{
	struct sunxi_gem_dmabuf_attachment *sunxi_attach = attach->priv;
	kfree(sunxi_attach);
	attach->priv = NULL;
}

static struct sg_table *sunxi_gem_map_dma_buf(struct dma_buf_attachment *attach,
					enum dma_data_direction dir)
{
	struct sunxi_gem_dmabuf_attachment *sunxi_attach;
	struct drm_gem_object *gem_obj;
	struct sunxi_drm_gem_buf *sunxi_buf;
	struct drm_device *dev;
	struct scatterlist *rd, *wr;
	struct sg_table *sgt = NULL;
	unsigned int i;
	int nents, ret;

	sunxi_attach = (struct sunxi_gem_dmabuf_attachment *)attach->priv;
	gem_obj = (struct drm_gem_object *)attach->dmabuf->priv;
	sunxi_buf = gem_obj->driver_private;
	dev = gem_obj->dev;

	DRM_DEBUG_PRIME("[%d]\n",__LINE__);

	/* just return current sgt if already requested. */
	if (sunxi_attach->dir == dir && sunxi_attach->is_mapped)
		return &sunxi_attach->sgt;

	if (!sunxi_buf) {
		DRM_ERROR("buffer is null.\n");
		return ERR_PTR(-ENOMEM);
	}

	sgt = &sunxi_attach->sgt;
	if (sgt->nents == 0) {
		ret = sg_alloc_table(sgt, sunxi_buf->sgt->orig_nents, GFP_KERNEL);
		if (ret) {
			DRM_ERROR("failed to alloc sgt.\n");
			return ERR_PTR(-ENOMEM);
		}

		rd = sunxi_buf->sgt->sgl;
		wr = sgt->sgl;
		for (i = 0; i < sgt->orig_nents; ++i) {
			sg_set_page(wr, sg_page(rd), rd->length, rd->offset);
			rd = sg_next(rd);
			wr = sg_next(wr);
		}
	}
	/* if attached dev impletment the dma_map_ops, so we must make a sg for it,
	not use the sunxi_buf's sg. the same reason for the attach.
	one dma_buf_attach() maybe call many dma_buf_map_attachment() for diffrent dir.
	*/
	if(sunxi_attach->dir != DMA_NONE) {
		/* for invalte and clean cache POC*/
		dma_unmap_sg(attach->dev, sgt->sgl, sgt->nents, sunxi_attach->dir);
	}

	if (dir != DMA_NONE) {
		nents = dma_map_sg(attach->dev, sgt->sgl, sgt->orig_nents, dir);
		if (!nents) {
		DRM_ERROR("failed to map sgl with iommu.\n");
		sg_free_table(sgt);
		sgt = ERR_PTR(-EIO);
		}
	}

	sunxi_attach->is_mapped = true;
	sunxi_attach->dir = dir;

	DRM_DEBUG_PRIME("buffer size = 0x%lu\n", sunxi_buf->size);

	return sgt;
}

static void sunxi_gem_unmap_dma_buf(struct dma_buf_attachment *attach,
						struct sg_table *sgt,
						enum dma_data_direction dir)
{
	dma_unmap_sg(attach->dev, sgt->sgl, sgt->nents, dir);
	sg_free_table(sgt);
}

static void sunxi_dmabuf_release(struct dma_buf *dmabuf)
{
	struct drm_gem_object *sunxi_gem_obj = dmabuf->priv;

	DRM_DEBUG_PRIME("[%d]\n", __LINE__);
	sunxi_gem_obj->export_dma_buf = NULL;
	/* inc in drm_gem_prime_handle_to_fd */
	drm_gem_object_unreference_unlocked(sunxi_gem_obj);
}

static void *sunxi_gem_dmabuf_kmap_atomic(struct dma_buf *dma_buf,
						unsigned long page_num)
{
	/* TODO */

	return NULL;
}

static void sunxi_gem_dmabuf_kunmap_atomic(struct dma_buf *dma_buf,
						unsigned long page_num,
						void *addr)
{
	/* TODO */
}

static void *sunxi_gem_dmabuf_kmap(struct dma_buf *dma_buf,
					unsigned long page_num)
{
	/* TODO */

	return NULL;
}

static void sunxi_gem_dmabuf_kunmap(struct dma_buf *dma_buf,
					unsigned long page_num, void *addr)
{
	/* TODO */
}

static int sunxi_gem_dmabuf_mmap(struct dma_buf *dma_buf,
	struct vm_area_struct *vma)
{
	struct drm_gem_object *obj;
	struct sunxi_drm_gem_buf *sunxi_buf;
	struct sg_table *table;
	unsigned long addr = vma->vm_start;
	unsigned long offset = vma->vm_pgoff * PAGE_SIZE;
	struct scatterlist *sg;
	int i, ret;
	unsigned long remainder, len; 
	struct page *page;

	/* drm don't allow dma_fd to do other but DRM_CLOEXEC,
	* if you want use dma_fd for the mmap(). modify the  
	* drm_prime_handle_to_fd_ioctl add O_RDWR.
	*/

	obj = (struct drm_gem_object *)dma_buf->priv;
	sunxi_buf = (struct sunxi_drm_gem_buf *)obj->driver_private;
	table = sunxi_buf->sgt;
	for_each_sg(table->sgl, sg, table->nents, i) {
		page = sg_page(sg);
		remainder = vma->vm_end - addr;
		len = sg->length;

		if (offset >= sg->length) {
			offset -= sg->length;
			continue;
		} else if (offset) {
			page += offset / PAGE_SIZE;
			len = sg->length - offset;
			offset = 0;
		}
		len = min(len, remainder);
		ret = remap_pfn_range(vma, addr, page_to_pfn(page), len,
		vma->vm_page_prot);
		if (ret)
			return ret;
		addr += len;
		if (addr >= vma->vm_end)
			return 0;
	}
	DRM_DEBUG_PRIME("vma_start:%lx  end:%lx offset:%lu size = 0x%lu\n",
			vma->vm_start, vma->vm_end, vma->vm_pgoff * PAGE_SIZE, sunxi_buf->size);

	return 0;
}

static struct dma_buf_ops sunxi_gem_dmabuf_ops = {
	.attach = sunxi_gem_attach_dma_buf,
	.detach	= sunxi_gem_detach_dma_buf,
	.map_dma_buf = sunxi_gem_map_dma_buf,
	.unmap_dma_buf = sunxi_gem_unmap_dma_buf,
	.kmap = sunxi_gem_dmabuf_kmap,
	.kmap_atomic = sunxi_gem_dmabuf_kmap_atomic,
	.kunmap = sunxi_gem_dmabuf_kunmap,
	.kunmap_atomic = sunxi_gem_dmabuf_kunmap_atomic,
	.mmap = sunxi_gem_dmabuf_mmap,
	.release = sunxi_dmabuf_release,
};

struct dma_buf *sunxi_dmabuf_prime_export(struct drm_device *drm_dev,
				struct drm_gem_object *obj, int flags)
{
	struct sunxi_drm_gem_buf *sunxi_buf =
		(struct sunxi_drm_gem_buf *)obj->driver_private;

	return dma_buf_export(obj, &sunxi_gem_dmabuf_ops,
			sunxi_buf->size, flags);
}

struct drm_gem_object *sunxi_dmabuf_prime_import(struct drm_device *drm_dev,
				struct dma_buf *dma_buf)
{
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	struct scatterlist *sgl;
	struct drm_gem_object *sunxi_gem_obj;
	struct sunxi_drm_gem_buf *buffer;
	int ret;

	DRM_DEBUG_PRIME("[%d]\n",__LINE__);

	/* is this one of own objects? */
	if (dma_buf->ops == &sunxi_gem_dmabuf_ops) {
		sunxi_gem_obj = dma_buf->priv;
		/* is it from our device? */
		if (sunxi_gem_obj->dev == drm_dev) {
			/*
			* Importing dmabuf exported from out own gem increases
			* refcount on gem itself instead of f_count of dmabuf.
			*/
			drm_gem_object_reference(sunxi_gem_obj);
			return sunxi_gem_obj;
		}
	}

	attach = dma_buf_attach(dma_buf, drm_dev->dev);
	if (IS_ERR(attach))
		return ERR_PTR(-EINVAL);

	get_dma_buf(dma_buf);

	sgt = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR_OR_NULL(sgt)) {
		ret = PTR_ERR(sgt);
		goto err_buf_detach;
	}

	buffer = kzalloc(sizeof(*buffer), GFP_KERNEL);
	if (!buffer) {
		DRM_ERROR("failed to allocate sunxi_drm_gem_buf.\n");
		ret = -ENOMEM;
		goto err_unmap_attach;
	}

	sunxi_gem_obj = drm_gem_object_alloc(drm_dev, dma_buf->size);
	if (!sunxi_gem_obj) {
		ret = -ENOMEM;
		goto err_free_buffer;
	}

	sgl = sgt->sgl;

	buffer->size = dma_buf->size;
	buffer->dma_addr = sg_dma_address(sgl);

	if (sgt->nents == 1) {
		buffer->flags |= SUNXI_BO_CONTIG;
	} else {
		buffer->flags &= ~SUNXI_BO_CONTIG;
	}

	sunxi_gem_obj->driver_private = (void *)buffer;
	buffer->sgt = sgt;
	sunxi_gem_obj->import_attach = attach;

	DRM_DEBUG_PRIME("[%d] dma_addr = 0x%llx, size = 0x%lu\n",
		__LINE__, buffer->dma_addr, buffer->size);

	return sunxi_gem_obj;

err_free_buffer:
	kfree(buffer);
	buffer = NULL;
err_unmap_attach:
	dma_buf_unmap_attachment(attach, sgt, DMA_BIDIRECTIONAL);
err_buf_detach:
	dma_buf_detach(dma_buf, attach);
	dma_buf_put(dma_buf);

	return ERR_PTR(ret);
}
