/*
 * drivers/gpu/ion/sunxi_ion_alloc.c
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liugang <liugang@allwinnertech.com>
 *
 * sunxi ion memory allocate realization
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/ion_sunxi.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <asm/pgtable.h>
#include "ion_priv.h"

extern struct ion_heap *carveout_heap;

int flush_clean_user_range(long start, long end);
EXPORT_SYMBOL(flush_clean_user_range);

int flush_user_range(long start, long end);
EXPORT_SYMBOL(flush_user_range);

void flush_dcache_all(void);
EXPORT_SYMBOL(flush_dcache_all);

u32 cma_alloc_phys(size_t size);
void cma_free_phys(u32 phys_addr, size_t size);
void *cma_map_kernel(u32 phys_addr, size_t size);
void cma_unmap_kernel(u32 phys_addr, size_t size, void *cpu_addr);

/*
 * use __map_buf_kernel to map phys addr to kernel space, instead of ioremap,
 * which cannot be used for mem_reserve areas.
 */
inline void *__map_buf_kernel(unsigned int phys_addr, unsigned int size)
{
	int npages = PAGE_ALIGN(size) / PAGE_SIZE;
	struct page **pages = vmalloc(sizeof(struct page *) * npages);
	struct page **tmp = pages;
	struct page *cur_page = phys_to_page(phys_addr);
	pgprot_t pgprot;
	void *vaddr;
	int i;

	if(!pages)
		return 0;

	for(i = 0; i < npages; i++)
		*(tmp++) = cur_page++;

	pgprot = pgprot_noncached(PAGE_KERNEL);
	vaddr = vmap(pages, npages, VM_MAP, pgprot);

	vfree(pages);
	return vaddr;
}

void inline __unmap_buf_kernel(void *vaddr)
{
	vunmap(vaddr);
}

u32 inline __carveout_alloc(size_t size)
{
	u32 phys_addr;

	phys_addr = (u32)ion_carveout_allocate(carveout_heap, size, 0);
	return (u32)ION_CARVEOUT_ALLOCATE_FAIL != phys_addr ? phys_addr : 0;
}

void inline __carveout_free(u32 phys_addr, size_t size)
{
	ion_carveout_free(carveout_heap, (ion_phys_addr_t)phys_addr, size);
}

void *sunxi_map_kernel(unsigned int phys_addr, unsigned int size)
{
#ifdef CONFIG_CMA
	return cma_map_kernel(phys_addr, size);
#else
	return __map_buf_kernel(phys_addr, size);
#endif
}
EXPORT_SYMBOL(sunxi_map_kernel);

void sunxi_unmap_kernel(void *vaddr, unsigned int phys_addr, unsigned int size)
{
#ifdef CONFIG_CMA
	return cma_unmap_kernel(phys_addr, size, vaddr);
#else
	return __unmap_buf_kernel(vaddr);
#endif
}
EXPORT_SYMBOL(sunxi_unmap_kernel);

u32 sunxi_alloc_phys(size_t size)
{
#ifdef CONFIG_CMA
	return cma_alloc_phys(size);
#else
	return __carveout_alloc(size);
#endif
}
EXPORT_SYMBOL(sunxi_alloc_phys);

void sunxi_free_phys(u32 phys_addr, size_t size)
{
#ifdef CONFIG_CMA
	cma_free_phys(phys_addr, size);
#else
	__carveout_free(phys_addr, size);
#endif
}
EXPORT_SYMBOL(sunxi_free_phys);

void *sunxi_buf_alloc(unsigned int size, unsigned int *paddr)
{
	void *vaddr = NULL;

#ifdef CONFIG_CMA
	vaddr = dma_alloc_coherent(NULL, size, (dma_addr_t *)paddr, GFP_KERNEL);
	if (!vaddr) {
		pr_err("%s(%d) err: alloc size 0x%x failed!\n", __func__, __LINE__, size);
		return NULL;
	}
#else
	*paddr = __carveout_alloc(size);
	if (!*paddr) {
		pr_err("%s(%d) err: alloc size 0x%x failed!\n", __func__, __LINE__, size);
		return NULL;
	}
	vaddr = __map_buf_kernel(*paddr, size);
	if (!vaddr) {
		pr_err("%s(%d) err: map kernel failed!\n", __func__, __LINE__);
		return NULL;
	}
#endif
	return vaddr;
}
EXPORT_SYMBOL(sunxi_buf_alloc);

void sunxi_buf_free(void *vaddr, unsigned int paddr, unsigned int size)
{
#ifdef CONFIG_CMA
	dma_free_coherent(NULL, size, vaddr, (dma_addr_t)paddr);
#else
	if (!vaddr || !paddr || !size) {
		pr_err("%s(%d) err: para err(va 0x%x, pa 0x%x, size 0x%x)!\n",
			__func__, __LINE__, (u32)vaddr, paddr, size);
		return;
	}
	__unmap_buf_kernel(vaddr);
	ion_carveout_free(carveout_heap, (ion_phys_addr_t)paddr, size);
#endif
}
EXPORT_SYMBOL(sunxi_buf_free);

