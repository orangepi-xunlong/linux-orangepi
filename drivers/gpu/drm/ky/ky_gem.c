// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include <linux/dma-map-ops.h>
#include <drm/drm_prime.h>
#include <drm/drm_gem_dma_helper.h>

#include "ky_gem.h"
#include "ky_dmmu.h"
#include "ky_drm.h"

static const unsigned int orders[] = {8, 4, 0};
static const int num_orders = ARRAY_SIZE(orders);

static struct page *__alloc_largest_available(size_t size,
				unsigned int max_order)
{
	struct page *page;
	unsigned int order;
	gfp_t gfp_flags;
	int i;

	for (i = 0; i < num_orders; i++) {
		order = orders[i];

		if (size < (PAGE_SIZE << order))
			continue;
		if (max_order < order)
			continue;

		gfp_flags = (GFP_HIGHUSER | __GFP_ZERO |
				__GFP_COMP | __GFP_RECLAIM);

		page = alloc_pages(gfp_flags, order);
		if (!page)
			continue;

		return page;
	}

	return NULL;
}

static int ky_gem_sysmem_alloc(struct drm_device *drm,
				struct ky_gem_object *ky_obj,
				size_t size)
{
	struct sg_table *sgt;
	struct scatterlist *sg;
	struct list_head pages;
	struct page *page, *tmp_page;
	size_t size_remaining = size;
	unsigned int max_order = orders[0];
	int i = 0;
	u32 page_nums = PAGE_ALIGN(size) >> PAGE_SHIFT;
	struct ky_drm_private *priv = drm->dev_private;


	sgt = kzalloc(sizeof(*sgt), GFP_KERNEL);
	if (!sgt)
		return -ENOMEM;

	INIT_LIST_HEAD(&pages);
	if (priv->contig_mem) {
#if IS_ENABLED(CONFIG_GKI_FIX_WORKAROUND)
		return -ENOMEM;
#else
		page = dma_alloc_from_contiguous(drm->dev, page_nums, 0, 0);
		if (!page) {
			kfree(sgt);
			DRM_ERROR("Failed to alloc %zd bytes continous mem\n", size);
			return -ENOMEM;
		}
		list_add_tail(&page->lru, &pages);
		i++;
#endif
	} else {
		while (size_remaining > 0) {
			if (fatal_signal_pending(current))
				goto free_pages;

			page = __alloc_largest_available(size_remaining, max_order);
			if (!page)
				goto free_pages;

			list_add_tail(&page->lru, &pages);
			size_remaining -= page_size(page);
			max_order = compound_order(page);
			i++;
		}
	}

	if (sg_alloc_table(sgt, i, GFP_KERNEL))
		goto free_pages;

	sg = sgt->sgl;
	list_for_each_entry_safe(page, tmp_page, &pages, lru) {
		if (priv->contig_mem)
			sg_set_page(sg, page, page_nums << PAGE_SHIFT, 0);
		else
			sg_set_page(sg, page, page_size(page), 0);
		sg = sg_next(sg);
		list_del(&page->lru);
	}

	dma_map_sgtable(drm->dev, sgt, DMA_BIDIRECTIONAL, 0);
	dma_unmap_sgtable(drm->dev, sgt, DMA_BIDIRECTIONAL, 0);

	ky_obj->sgt = sgt;

	return 0;

free_pages:
	list_for_each_entry_safe(page, tmp_page, &pages, lru) {
		if (priv->contig_mem)
#if IS_ENABLED(CONFIG_GKI_FIX_WORKAROUND)
			nop();
#else
			dma_release_from_contiguous(drm->dev, page, page_nums);
#endif
		else
			__free_pages(page, compound_order(page));
	}
	kfree(sgt);

	return -ENOMEM;
}

static void ky_gem_sysmem_free(struct ky_gem_object *ky_obj)
{
	struct sg_table *sgt = ky_obj->sgt;
	struct scatterlist *sg;
	int i;
	struct drm_device *drm = ky_obj->base.dev;
	struct ky_drm_private *priv = drm->dev_private;

	if (!sgt) {
		return;
	}

	for_each_sgtable_sg(sgt, sg, i) {
		struct page *page = sg_page(sg);
		if (priv->contig_mem)
#if IS_ENABLED(CONFIG_GKI_FIX_WORKAROUND)
			nop();
#else
 			dma_release_from_contiguous(drm->dev, page, sg->length >> PAGE_SHIFT);
#endif
		else
			__free_pages(page, compound_order(page));
	}

	sg_free_table(sgt);
	kfree(sgt);
}

static int ky_gem_alloc_buf(struct drm_device *drm,
				struct ky_gem_object *ky_obj,
				size_t size)
{
	int ret;

	ret = ky_gem_sysmem_alloc(drm, ky_obj, size);
	if (ret) {
		DRM_ERROR("KY_GEM: failed to allocate %zu byte buffer", size);
		return -ENOMEM;
	}

	return 0;
}

static void ky_gem_free_buf(struct ky_gem_object *ky_obj)
{
	ky_gem_sysmem_free(ky_obj);
}

static void ky_gem_free_object(struct drm_gem_object *gem_obj)
{
	struct ky_gem_object *ky_obj = to_ky_obj(gem_obj);

	if (gem_obj->import_attach) {
		drm_prime_gem_destroy(gem_obj, ky_obj->sgt);
	} else {
		ky_gem_free_buf(ky_obj);
	}

	drm_gem_object_release(gem_obj);

	kfree(ky_obj);
}

static struct sg_table *__dup_sg_table(struct sg_table *sgt)
{
	struct sg_table *new_sgt;
	struct scatterlist *sg, *new_sg;
	int ret, i;

	new_sgt = kzalloc(sizeof(*new_sgt), GFP_KERNEL);
	if (!new_sgt)
		return ERR_PTR(-ENOMEM);

	ret = sg_alloc_table(new_sgt, sgt->orig_nents, GFP_KERNEL);
	if (ret) {
		kfree(new_sgt);
		return ERR_PTR(-ENOMEM);
	}

	new_sg = new_sgt->sgl;
	for_each_sgtable_sg(sgt, sg, i) {
		sg_set_page(new_sg, sg_page(sg), sg->length, sg->offset);
		new_sg = sg_next(new_sg);
	}

	return new_sgt;
}

static struct sg_table *ky_gem_prime_get_sg_table(struct drm_gem_object *gem_obj)
{
	struct ky_gem_object *ky_obj = to_ky_obj(gem_obj);

	return __dup_sg_table(ky_obj->sgt);
}

int ky_gem_prime_vmap(struct drm_gem_object *gem_obj, struct iosys_map *map)
{
	struct ky_gem_object *ky_obj = to_ky_obj(gem_obj);
	struct sg_page_iter piter;
	int ret = 0;
	void *vaddr;
	int npages = PAGE_ALIGN(gem_obj->size) >> PAGE_SHIFT;
	struct page **pages, **tmp;
	pgprot_t pgprot = pgprot_writecombine(PAGE_KERNEL);

	mutex_lock(&ky_obj->vmap_lock);


	if (ky_obj->vmap_cnt) {
		vaddr = ky_obj->vaddr;
		goto vmap_success;
	}

	pages = kvmalloc_array(npages, sizeof(struct page *), GFP_KERNEL);
	tmp = pages;
	if (!pages)
		goto vmap_fail;
	for_each_sgtable_page(ky_obj->sgt, &piter, 0) {
		WARN_ON(tmp - pages >= npages);
		*(tmp++) = sg_page_iter_page(&piter);
	}
	vaddr = vmap(pages, npages, VM_MAP, pgprot);
	kvfree(pages);

	if (!vaddr)
		goto vmap_fail;

	ky_obj->vaddr = vaddr;

vmap_success:
	ky_obj->vmap_cnt++;
	iosys_map_set_vaddr(map, vaddr);
	mutex_unlock(&ky_obj->vmap_lock);
	return ret;

vmap_fail:
	mutex_unlock(&ky_obj->vmap_lock);
	return -ENOMEM;
}

void ky_gem_prime_vunmap(struct drm_gem_object *obj, struct iosys_map *map)
{
	struct ky_gem_object *ky_obj = to_ky_obj(obj);

	mutex_lock(&ky_obj->vmap_lock);
	ky_obj->vmap_cnt--;
	if (!ky_obj->vmap_cnt) {
		vunmap(map->vaddr);
		ky_obj->vaddr = NULL;
	}
	mutex_unlock(&ky_obj->vmap_lock);
}

const struct vm_operations_struct vm_ops = {
	.open = drm_gem_vm_open,
	.close = drm_gem_vm_close,
};

static const struct drm_gem_object_funcs ky_gem_funcs = {
	.free = ky_gem_free_object,
	.get_sg_table = ky_gem_prime_get_sg_table,
	.vmap = ky_gem_prime_vmap,
	.vunmap = ky_gem_prime_vunmap,
	.vm_ops = &vm_ops
};

static struct drm_gem_object *
__ky_gem_create_object(struct drm_device *drm, size_t size)
{
	struct ky_gem_object *ky_obj;
	struct drm_gem_object *gem_obj;
	int ret;

	ky_obj = kzalloc(sizeof(*ky_obj), GFP_KERNEL);
	if (!ky_obj)
		return ERR_PTR(-ENOMEM);

	gem_obj = &ky_obj->base;

	gem_obj->funcs = &ky_gem_funcs;

	drm_gem_private_object_init(drm, gem_obj, size);

	ret = drm_gem_create_mmap_offset(gem_obj);
	if (ret) {
		drm_gem_object_release(gem_obj);
		kfree(ky_obj);
		return ERR_PTR(ret);
	}

	ky_obj->sgt = NULL;
	ky_obj->vaddr = NULL;
	ky_obj->vmap_cnt = 0;
	mutex_init(&ky_obj->vmap_lock);

	return gem_obj;
}

static struct drm_gem_object *
ky_gem_create_object(struct drm_device *drm, size_t size)
{
	struct ky_gem_object *ky_obj;
	struct drm_gem_object *gem_obj;
	int ret;

	size = PAGE_ALIGN(size);

	gem_obj = __ky_gem_create_object(drm, size);
	if (IS_ERR(gem_obj))
		return gem_obj;

	ky_obj = to_ky_obj(gem_obj);
	ret = ky_gem_alloc_buf(drm, ky_obj, size);
	if (ret) {
		drm_gem_object_put(gem_obj);
		return ERR_PTR(ret);
	}

	return gem_obj;
}

static struct drm_gem_object *
ky_gem_create_with_handle(struct drm_file *file_priv, struct drm_device *drm,
						size_t size, unsigned int *handle)
{
	struct drm_gem_object *gem_obj;
	int ret;

	gem_obj = ky_gem_create_object(drm, size);
	if (IS_ERR(gem_obj))
		return gem_obj;

	ret = drm_gem_handle_create(file_priv, gem_obj, handle);
	drm_gem_object_put(gem_obj);
	if (ret)
		return ERR_PTR(ret);

	return gem_obj;
}

int ky_gem_dumb_create(struct drm_file *file_priv,
				struct drm_device *drm,
				struct drm_mode_create_dumb *args)
{
	struct drm_gem_object *gem_obj;

	args->pitch = DIV_ROUND_UP(args->width * args->bpp, 8);
	args->size = args->pitch * args->height;

	gem_obj = ky_gem_create_with_handle(file_priv, drm, args->size, &args->handle);

	return PTR_ERR_OR_ZERO(gem_obj);
}

static int ky_gem_object_mmap(struct drm_gem_object *gem_obj,
				struct vm_area_struct *vma)
{
	struct ky_gem_object *ky_obj = to_ky_obj(gem_obj);
	unsigned long addr = vma->vm_start;
	struct sg_page_iter piter;
	int ret;

	for_each_sgtable_page(ky_obj->sgt, &piter, vma->vm_pgoff) {
		struct page *page = sg_page_iter_page(&piter);

		ret = remap_pfn_range(vma, addr, page_to_pfn(page), PAGE_SIZE,
						vma->vm_page_prot);
		if (ret)
			return ret;

		addr += PAGE_SIZE;
		if (addr >= vma->vm_end)
			return 0;
	}

	return 0;
}

int ky_gem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct drm_gem_object *gem_obj;
	int ret;

	ret = drm_gem_mmap(filp, vma);
	if (ret)
		return ret;

	gem_obj = vma->vm_private_data;

	vma->vm_pgoff -= drm_vma_node_start(&gem_obj->vma_node);

	if (gem_obj->import_attach) {
		drm_gem_object_put(gem_obj);
		vma->vm_private_data = NULL;

		return dma_buf_mmap(gem_obj->dma_buf, vma, vma->vm_pgoff);
	}

	return ky_gem_object_mmap(gem_obj, vma);
}

struct drm_gem_object *
ky_gem_prime_import_sg_table(struct drm_device *drm,
				struct dma_buf_attachment *attach,
				struct sg_table *sgt)
{
	struct ky_gem_object *ky_obj;
	struct drm_gem_object *gem_obj;
	size_t size = PAGE_ALIGN(attach->dmabuf->size);

	gem_obj = __ky_gem_create_object(drm, size);
	if (IS_ERR(gem_obj))
		return gem_obj;

	ky_obj = to_ky_obj(gem_obj);
	ky_obj->sgt = sgt;

	return gem_obj;
}

