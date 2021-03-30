/*
 * drivers/gpu/ion/ion_secure_heap.c
 *
 * Copyright (C) Allwinner 2014
 * Author: <sunny@allwinnertech.com> for Allwinner.
 *
 * Add secure heap support.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/spinlock.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/genalloc.h>
#include <linux/io.h>
#include <linux/ion.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/ion_sunxi.h>
#include "ion_priv.h"

struct ion_secure_heap {
	struct ion_heap heap;
	struct gen_pool *pool;
	ion_phys_addr_t base;
	size_t          size;
};

ion_phys_addr_t ion_secure_allocate(struct ion_heap *heap,
				      unsigned long size,
				      unsigned long align)
{
	struct ion_secure_heap *secure_heap =
		container_of(heap, struct ion_secure_heap, heap);
	unsigned long offset = gen_pool_alloc(secure_heap->pool, size);

	if (!offset) {
		pr_err("%s(%d) err: alloc 0x%08x bytes failed\n",
			__func__, __LINE__, (u32)size);
		return ION_CARVEOUT_ALLOCATE_FAIL;
	}
	return offset;
}

void ion_secure_free(struct ion_heap *heap, ion_phys_addr_t addr,
		       unsigned long size)
{
	struct ion_secure_heap *secure_heap =
		container_of(heap, struct ion_secure_heap, heap);

	if (addr == ION_CARVEOUT_ALLOCATE_FAIL)
		return;
	gen_pool_free(secure_heap->pool, addr, size);
}

static int ion_secure_heap_phys(struct ion_heap *heap,
				  struct ion_buffer *buffer,
				  ion_phys_addr_t *addr, size_t *len)
{
	if ((*len & 0xffff) == 0) {
		*addr = buffer->priv_phys;
		*len = buffer->size;
	} else {
		unsigned int  drm_phy_addr, drm_tee_addr;

		sunxi_ion_probe_drm_info(&drm_phy_addr, &drm_tee_addr);
		if (drm_phy_addr)
			*addr = buffer->priv_phys
			- drm_phy_addr
			+ drm_tee_addr;
		else
			*addr = 0;
	}
	return 0;
}

static int ion_secure_heap_allocate(struct ion_heap *heap,
				      struct ion_buffer *buffer,
				      unsigned long size, unsigned long align,
				      unsigned long flags)
{
	buffer->priv_phys = ion_secure_allocate(heap, size, align);
	return buffer->priv_phys == ION_CARVEOUT_ALLOCATE_FAIL ? -ENOMEM : 0;
}

static void ion_secure_heap_free(struct ion_buffer *buffer)
{
	struct ion_heap *heap = buffer->heap;

	ion_secure_free(heap, buffer->priv_phys, buffer->size);
	buffer->priv_phys = ION_CARVEOUT_ALLOCATE_FAIL;
}

struct sg_table *ion_secure_heap_map_dma(struct ion_heap *heap,
					      struct ion_buffer *buffer)
{
	struct sg_table *table;
	int ret;

	table = kzalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!table)
		return ERR_PTR(-ENOMEM);
	ret = sg_alloc_table(table, 1, GFP_KERNEL);
	if (ret) {
		kfree(table);
		return ERR_PTR(ret);
	}
	sg_set_page(table->sgl, phys_to_page(buffer->priv_phys), buffer->size,
		    0);
	return table;
}

void ion_secure_heap_unmap_dma(struct ion_heap *heap,
				 struct ion_buffer *buffer)
{
	sg_free_table(buffer->sg_table);
	kfree(buffer->sg_table);
}

int ion_secure_heap_map_user(struct ion_heap *heap, struct ion_buffer *buffer,
			       struct vm_area_struct *vma)
{
	return remap_pfn_range(vma, vma->vm_start,
			       __phys_to_pfn(buffer->priv_phys) + vma->vm_pgoff,
			       vma->vm_end - vma->vm_start,
			       vma->vm_page_prot);
	/* when user call ION_IOC_ALLOC not with ION_FLAG_CACHED, ion_mmap will
	* change prog to pgprot_writecombine itself, so we donot need change to
	* pgprot_writecombine here manually.
	*/
}

static struct ion_heap_ops secure_heap_ops = {
	.allocate = ion_secure_heap_allocate,
	.free = ion_secure_heap_free,
	.phys = ion_secure_heap_phys,
	.map_dma = ion_secure_heap_map_dma,
	.unmap_dma = ion_secure_heap_unmap_dma,
	.map_user = ion_secure_heap_map_user,
	.map_kernel = ion_heap_map_kernel,
	.unmap_kernel = ion_heap_unmap_kernel,
};

struct ion_heap *ion_secure_heap_create(struct ion_platform_heap *heap_data)
{
	struct ion_secure_heap *secure_heap;

	secure_heap = kzalloc(sizeof(struct ion_secure_heap), GFP_KERNEL);
	if (!secure_heap)
		return ERR_PTR(-ENOMEM);

	secure_heap->pool = gen_pool_create(12, -1);
	if (!secure_heap->pool) {
		kfree(secure_heap);
		return ERR_PTR(-ENOMEM);
	}
	secure_heap->base = heap_data->base;
	secure_heap->size = heap_data->size;
	gen_pool_add(secure_heap->pool, secure_heap->base, heap_data->size, -1);
	secure_heap->heap.ops = &secure_heap_ops;
	secure_heap->heap.type = ION_HEAP_TYPE_SECURE;
	return &secure_heap->heap;
}

void ion_secure_heap_destroy(struct ion_heap *heap)
{
	struct ion_secure_heap *secure_heap =
	     container_of(heap, struct  ion_secure_heap, heap);

	gen_pool_destroy(secure_heap->pool);
	kfree(secure_heap);
	secure_heap = NULL;
}
