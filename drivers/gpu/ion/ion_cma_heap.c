/*
 * drivers/gpu/ion/ion_cma_heap.c
 *
 * Copyright (C) Linaro 2012
 * Author: <benjamin.gaignard@linaro.org> for ST-Ericsson.
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

#include <linux/device.h>
#include <linux/ion.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/dma-mapping.h>

/* for ion_heap_ops structure */
#include "ion_priv.h"

/* split dma_alloc_coherent: alloc phys mem in ion_cma_allocate, alloc virt address and map in
 * ion_cma_map_kernel. This is in order to save vmalloc virtual space */
#ifdef CONFIG_CMA
#define ION_CMA_SPLIT_DMA_ALLOC
#endif
#define ION_CMA_ALLOCATE_FAILED -1

struct ion_cma_heap {
	struct ion_heap heap;
	struct device *dev;
};

#define to_cma_heap(x) container_of(x, struct ion_cma_heap, heap)

struct ion_cma_buffer_info {
	void *cpu_addr;
	dma_addr_t handle;
	struct sg_table *table;
};
#ifdef ION_CMA_SPLIT_DMA_ALLOC
struct page *dma_alloc_from_contiguous(struct device *dev,
				int count, unsigned int align);
void __dma_clear_buffer(struct page *page, size_t size);
pgprot_t __get_dma_pgprot(struct dma_attrs *attrs, pgprot_t prot);
void *__dma_alloc_remap(struct page *page, size_t size, gfp_t gfp,
				pgprot_t prot, const void *caller);
void __dma_remap(struct page *page, size_t size, pgprot_t prot);
void __dma_free_remap(void *cpu_addr, size_t size);
bool dma_release_from_contiguous(struct device *dev,
				struct page *pages, int count);

u32 cma_alloc_phys(size_t size)
{
	unsigned long order;
	struct page *page;
	size_t count;

	size = PAGE_ALIGN(size);
	order = get_order(size);
	count = size >> PAGE_SHIFT;

	page = dma_alloc_from_contiguous(NULL, count, order);
	if (!page)
		return 0;

	__dma_clear_buffer(page, size);

	return page_to_phys(page);
}

extern unsigned int mem_start, mem_size;

void cma_free_phys(u32 phys_addr, size_t size)
{
	struct page *pages;
	size_t count;

if (unlikely(phys_addr < mem_start || phys_addr > mem_start + mem_size)) {
	pr_err("%s(%d) err: phys_addr 0x%x invalid!\n", __func__, __LINE__, phys_addr);
	return;
}
//	BUG_ON(unlikely(!pfn_valid(__phys_to_pfn(phys_addr))));

	size = PAGE_ALIGN(size);
	count = size >> PAGE_SHIFT;

	pages = phys_to_page((u32)phys_addr);
	if (!dma_release_from_contiguous(NULL, pages, count)) {
		pr_err("%s(%d) err: dma_release_from_contiguous failed!\n", __func__, __LINE__);
		return;
	}
}
void *cma_map_kernel(u32 phys_addr, size_t size)
{
	pgprot_t prot = __get_dma_pgprot(NULL, pgprot_kernel);
	struct page *page = phys_to_page(phys_addr);
	void *ptr = NULL;

if (unlikely(phys_addr < mem_start || phys_addr > mem_start + mem_size)) {
	pr_err("%s(%d) err: phys_addr 0x%x invalid!\n", __func__, __LINE__, phys_addr);
	return NULL;
}
//	BUG_ON(unlikely(!pfn_valid(__phys_to_pfn(phys_addr))));

	size = PAGE_ALIGN(size);

	if (PageHighMem(page)) {
		ptr = __dma_alloc_remap(page, size, GFP_KERNEL, prot, __builtin_return_address(0));
		if (!ptr) {
			pr_err("%s(%d) err: __dma_alloc_remap failed!\n", __func__, __LINE__);
			return NULL;
		}
	} else {
		__dma_remap(page, size, prot);
		ptr = page_address(page);
	}

	return ptr;
}

void cma_unmap_kernel(u32 phys_addr, size_t size, void *cpu_addr)
{
	struct page *page = phys_to_page(phys_addr);

	BUG_ON(unlikely(!pfn_valid(__phys_to_pfn(phys_addr))));

	size = PAGE_ALIGN(size);

	if (PageHighMem(page))
		__dma_free_remap(cpu_addr, size);
	else
		__dma_remap(page, size, pgprot_kernel);
}
#endif
/*
 * Create scatter-list for the already allocated DMA buffer.
 * This function could be replaced by dma_common_get_sgtable
 * as soon as it will avalaible.
 */
int ion_cma_get_sgtable(struct device *dev, struct sg_table *sgt,
			void *cpu_addr, dma_addr_t handle, size_t size)
{
	struct page *page = phys_to_page((u32)handle);
	int ret;

	ret = sg_alloc_table(sgt, 1, GFP_KERNEL);
	if (unlikely(ret))
		return ret;

	sg_set_page(sgt->sgl, page, PAGE_ALIGN(size), 0);
	return 0;
}

/* ION CMA heap operations functions */
static int ion_cma_allocate(struct ion_heap *heap, struct ion_buffer *buffer,
			    unsigned long len, unsigned long align,
			    unsigned long flags)
{
	struct ion_cma_heap *cma_heap = to_cma_heap(heap);
	struct device *dev = cma_heap->dev;
	struct ion_cma_buffer_info *info;

	dev_dbg(dev, "Request buffer allocation len %ld\n", len);

	info = kzalloc(sizeof(struct ion_cma_buffer_info), GFP_KERNEL);
	if (!info) {
		dev_err(dev, "Can't allocate buffer info\n");
		return ION_CMA_ALLOCATE_FAILED;
	}

#ifdef ION_CMA_SPLIT_DMA_ALLOC
	info->handle = cma_alloc_phys(len);
	if (!info->handle) {
		dev_err(dev, " P-%s Fail to allocate buffer, cma_alloc_phys err!\n",current->comm);
		goto err;
	}
	info->cpu_addr = NULL;
#else
	info->cpu_addr = dma_alloc_coherent(dev, len, &(info->handle),
						GFP_HIGHUSER | __GFP_ZERO);

	if (!info->cpu_addr) {
		dev_err(dev, "Fail to allocate buffer\n");
		goto err;
	}
#endif

	info->table = kmalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!info->table) {
		dev_err(dev, "Fail to allocate sg table\n");
		goto free_mem;
	}

	if (ion_cma_get_sgtable
	    (dev, info->table, info->cpu_addr, info->handle, len))
		goto free_table;
	/* keep this for memory release */
	buffer->priv_virt = info;
	dev_dbg(dev, "Allocate buffer %p\n", buffer);
	return 0;

free_table:
	kfree(info->table);
free_mem:
#ifdef ION_CMA_SPLIT_DMA_ALLOC
	cma_free_phys(info->handle, len);
#else
	dma_free_coherent(dev, len, info->cpu_addr, info->handle);
#endif
err:
	kfree(info);
	return ION_CMA_ALLOCATE_FAILED;
}

static void ion_cma_free(struct ion_buffer *buffer)
{
	struct ion_cma_heap *cma_heap = to_cma_heap(buffer->heap);
	struct device *dev = cma_heap->dev;
	struct ion_cma_buffer_info *info = buffer->priv_virt;

	dev_dbg(dev, "Release buffer %p\n", buffer);
	/* release memory */
#ifdef ION_CMA_SPLIT_DMA_ALLOC
	cma_free_phys((u32)info->handle, buffer->size);
#else
	dma_free_coherent(dev, buffer->size, info->cpu_addr, info->handle);
#endif
	/* release sg table */
	sg_free_table(info->table);
	kfree(info->table);
	kfree(info);
}

/* return physical address in addr */
static int ion_cma_phys(struct ion_heap *heap, struct ion_buffer *buffer,
			ion_phys_addr_t *addr, size_t *len)
{
	struct ion_cma_heap *cma_heap = to_cma_heap(buffer->heap);
	struct device *dev = cma_heap->dev;
	struct ion_cma_buffer_info *info = buffer->priv_virt;

	dev_dbg(dev, "Return buffer %p physical address 0x%x\n", buffer,
		info->handle);

	*addr = info->handle;
	*len = buffer->size;

	return 0;
}

struct sg_table *ion_cma_heap_map_dma(struct ion_heap *heap,
					 struct ion_buffer *buffer)
{
	struct ion_cma_buffer_info *info = buffer->priv_virt;

	return info->table;
}

void ion_cma_heap_unmap_dma(struct ion_heap *heap,
			       struct ion_buffer *buffer)
{
	return;
}

static int ion_cma_mmap(struct ion_heap *mapper, struct ion_buffer *buffer,
			struct vm_area_struct *vma)
{
	struct ion_cma_buffer_info *info = buffer->priv_virt;

	/* we need cached map in most case, so donot use dma_mmap_coherent */
	//return dma_mmap_coherent(dev, vma, info->cpu_addr, info->handle, buffer->size);
	return remap_pfn_range(vma, vma->vm_start,
			       __phys_to_pfn((u32)info->handle) + vma->vm_pgoff,
			       vma->vm_end - vma->vm_start,
			       vma->vm_page_prot);
}

void *ion_cma_map_kernel(struct ion_heap *heap, struct ion_buffer *buffer)
{
	struct ion_cma_buffer_info *info = buffer->priv_virt;
#ifdef ION_CMA_SPLIT_DMA_ALLOC
	info->cpu_addr = cma_map_kernel((u32)info->handle, buffer->size);
	return info->cpu_addr;
#else
	/* kernel memory mapping has been done at allocation time */
	return info->cpu_addr;
#endif
}

static void ion_cma_unmap_kernel(struct ion_heap *heap,
					struct ion_buffer *buffer)
{
#ifdef ION_CMA_SPLIT_DMA_ALLOC
	struct ion_cma_buffer_info *info = buffer->priv_virt;

	BUG_ON(unlikely(!info->cpu_addr));
	cma_unmap_kernel((u32)info->handle, buffer->size, info->cpu_addr);
#endif
}

static struct ion_heap_ops ion_cma_ops = {
	.allocate = ion_cma_allocate,
	.free = ion_cma_free,
	.map_dma = ion_cma_heap_map_dma,
	.unmap_dma = ion_cma_heap_unmap_dma,
	.phys = ion_cma_phys,
	.map_user = ion_cma_mmap,
	.map_kernel = ion_cma_map_kernel,
	.unmap_kernel = ion_cma_unmap_kernel,
};

struct ion_heap *ion_cma_heap_create(struct ion_platform_heap *data)
{
	struct ion_cma_heap *cma_heap;

	cma_heap = kzalloc(sizeof(struct ion_cma_heap), GFP_KERNEL);

	if (!cma_heap)
		return ERR_PTR(-ENOMEM);

	cma_heap->heap.ops = &ion_cma_ops;
	/* get device from private heaps data, later it will be
	 * used to make the link with reserved CMA memory */
	cma_heap->dev = data->priv;
	cma_heap->heap.type = ION_HEAP_TYPE_DMA;
	return &cma_heap->heap;
}

void ion_cma_heap_destroy(struct ion_heap *heap)
{
	struct ion_cma_heap *cma_heap = to_cma_heap(heap);

	kfree(cma_heap);
}
#if defined(CONFIG_ION_SUNXI) && defined(CONFIG_PROC_FS)
#include <linux/module.h>
#include <linux/debugfs.h>
#include <asm/dma-contiguous.h>
struct cma {
        unsigned long   base_pfn;
        unsigned long   count;
        unsigned long   *bitmap;
};
u32 sunxi_cma_mem_free(sunxi_pool_info *info)
{
	struct cma *cma = NULL;
	int i = 0, index, offset;
	int free_cnt = 0;

	if(!info)
		return ERR_PTR(-ENOMEM);

	cma = dev_get_cma_area(NULL);
	if (!cma)
		return ERR_PTR(-ENOMEM);

	for (i = 0; i < cma->count; i++) {
		index = i >> 5;
		offset = i & 31;
		if (!(cma->bitmap[index] & (1<<offset))) {
			free_cnt++;
		}
	}

	info->total = cma->count*(PAGE_SIZE/1024);
	info->free_kb = free_cnt*(PAGE_SIZE/1024);
	info->free_mb = info->free_kb/1024;

	return 0;
}
EXPORT_SYMBOL(sunxi_cma_mem_free);
#endif
