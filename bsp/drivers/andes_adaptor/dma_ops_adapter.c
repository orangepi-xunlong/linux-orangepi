// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2023 - 2025 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * port dma ops to andes platform for memory access
 */
#include <linux/module.h>
#include <linux/cma.h>
#include <linux/dma-contiguous.h>
#include <asm/io.h>
#include <linux/init.h>
#include <linux/mmzone.h>
#include <asm/dma-contiguous.h>
#include <asm/cacheflush.h>
#include <linux/dma-direct.h>
#include <linux/dma-noncoherent.h>

#if IS_ENABLED(CONFIG_AW_ANDES_NO_DIRECT_DMA)
void *uncached_kernel_address(void *ptr)
{
	static phys_addr_t base;
	static size_t size;
	struct cma *default_cma = dev_get_cma_area(NULL);

	/* oneshot iniliztion */
	if (default_cma) {
		size = cma_get_size(default_cma);
		base = cma_get_base(default_cma);
	}
	/* ptr validation */
	if (!size) {
		if (base < (phys_addr_t)ptr && (phys_addr_t)ptr < base + size) {
			goto ptr_is_uncached;
		}
	}
	pr_err("ptr %pad not non-cached!", &ptr);
	dump_stack();
ptr_is_uncached:
	return ptr;
}

void *cached_kernel_address(void *ptr)
{
	return ptr;
}

void arch_dma_prep_coherent(struct page *page, size_t size)
{
	arch_sync_dma_for_device(NULL, page_to_phys(page), size, DMA_TO_DEVICE);
}

long arch_dma_coherent_to_pfn(struct device *dev, void *cpu_addr,
			      dma_addr_t dma_addr)
{
	return __phys_to_pfn(dma_to_phys(dev, dma_addr));
}
#endif

#if IS_ENABLED(CONFIG_AW_ANDES_DMA_OPS)

/*
 * presumption:
 * 1. no-iommu involved so all memory we are dealing with
 * is contiguous here
 * 2. we are working on an platform works with limited dram
 * so highmem no need to worry about, lowmem address space
 * covers all the dram we have
 */
static void *nommu_dma_alloc(struct device *dev, size_t size,
			     dma_addr_t *dma_handle, gfp_t gfp,
			     unsigned long attrs)

{
	unsigned long order = get_order(size);
	size_t count = ALIGN(size, PAGE_SIZE) >> PAGE_SHIFT;
	struct page *page;
	void *ptr = NULL;

	page = dma_alloc_from_contiguous(dev, count, order, gfp & __GFP_NOWARN);
	if (!page)
		return NULL;

	ptr = page_address(page);
	*dma_handle = page_to_phys(page);

	return ptr;
}

static void nommu_dma_free(struct device *dev, size_t size, void *cpu_addr,
			   dma_addr_t dma_addr, unsigned long attrs)
{
	int ret ;
	ret = dma_release_from_contiguous(dev, phys_to_page(dma_addr),
					  PAGE_ALIGN(size) >> PAGE_SHIFT);
	/* cma release return false for error, true for ok */
	WARN_ON_ONCE(ret == false);
}

static int check_dma_addr(struct device *dev, dma_addr_t dma_addr, size_t size)
{
	struct cma *cma_area;
	phys_addr_t dma_start, dma_end;
	phys_addr_t cma_start, cma_end;
	cma_area = dev_get_cma_area(dev);
	cma_start = cma_get_base(cma_area);
	cma_end = cma_start + cma_get_size(cma_area);
	dma_start = dma_addr;
	dma_end = dma_addr + size;
	if ((dma_addr < cma_start) || (dma_end > cma_end)) {
		pr_err("dma addr %pad ~ %pad not in cam area %pad ~ %pad",
		       &dma_addr, &dma_end, &cma_start, &cma_end);
		dump_stack();
		return -1;
	}
	return 0;
}

static int nommu_dma_mmap(struct device *dev, struct vm_area_struct *vma,
			  void *cpu_addr, dma_addr_t dma_addr, size_t size,
			  unsigned long attrs)
{
	int ret;
	/* param sanity check */
	check_dma_addr(dev, dma_addr, size);

	ret = remap_pfn_range(vma, vma->vm_start, phys_to_pfn(dma_addr), size,
			      vma->vm_page_prot);

	return ret;
}

static void __dma_page_cpu_to_dev(phys_addr_t paddr, size_t size,
				  enum dma_data_direction dir)
{
	arch_sync_dma_for_device(NULL, paddr, size, dir);
}

static void __dma_page_dev_to_cpu(phys_addr_t paddr, size_t size,
				  enum dma_data_direction dir)
{
	arch_sync_dma_for_cpu(NULL, paddr, size, dir);
}

static dma_addr_t nommu_dma_map_page(struct device *dev, struct page *page,
				     unsigned long offset, size_t size,
				     enum dma_data_direction dir,
				     unsigned long attrs)
{
	dma_addr_t handle = page_to_phys(page) + offset;
	check_dma_addr(dev, handle, size);

	__dma_page_cpu_to_dev(handle, size, dir);

	return handle;
}

static void nommu_dma_unmap_page(struct device *dev, dma_addr_t handle,
				 size_t size, enum dma_data_direction dir,
				 unsigned long attrs)
{
	__dma_page_dev_to_cpu(handle, size, dir);
}

static int nommu_dma_map_sg(struct device *dev, struct scatterlist *sgl,
			    int nents, enum dma_data_direction dir,
			    unsigned long attrs)
{
	int i;
	struct scatterlist *sg;

	for_each_sg (sgl, sg, nents, i) {
		sg_dma_address(sg) = sg_phys(sg);
		sg_dma_len(sg) = sg->length;
		__dma_page_cpu_to_dev(sg_dma_address(sg), sg_dma_len(sg), dir);
	}

	return nents;
}

static void nommu_dma_unmap_sg(struct device *dev, struct scatterlist *sgl,
			       int nents, enum dma_data_direction dir,
			       unsigned long attrs)
{
	struct scatterlist *sg;
	int i;

	for_each_sg (sgl, sg, nents, i)
		__dma_page_dev_to_cpu(sg_dma_address(sg), sg_dma_len(sg), dir);
}

static void nommu_dma_sync_single_for_device(struct device *dev,
					     dma_addr_t handle, size_t size,
					     enum dma_data_direction dir)
{
	__dma_page_cpu_to_dev(handle, size, dir);
}

static void nommu_dma_sync_single_for_cpu(struct device *dev, dma_addr_t handle,
					  size_t size,
					  enum dma_data_direction dir)
{
	__dma_page_cpu_to_dev(handle, size, dir);
}

static void nommu_dma_sync_sg_for_device(struct device *dev,
					 struct scatterlist *sgl, int nents,
					 enum dma_data_direction dir)
{
	struct scatterlist *sg;
	int i;

	for_each_sg (sgl, sg, nents, i)
		__dma_page_cpu_to_dev(sg_dma_address(sg), sg_dma_len(sg), dir);
}

static void nommu_dma_sync_sg_for_cpu(struct device *dev,
				      struct scatterlist *sgl, int nents,
				      enum dma_data_direction dir)
{
	struct scatterlist *sg;
	int i;

	for_each_sg (sgl, sg, nents, i)
		__dma_page_dev_to_cpu(sg_dma_address(sg), sg_dma_len(sg), dir);
}

const struct dma_map_ops aw_andes_dma_ops = {
	.alloc = nommu_dma_alloc,
	.free = nommu_dma_free,
	.mmap = nommu_dma_mmap,
	.map_page = nommu_dma_map_page,
	.unmap_page = nommu_dma_unmap_page,
	.map_sg = nommu_dma_map_sg,
	.unmap_sg = nommu_dma_unmap_sg,
	.sync_single_for_device = nommu_dma_sync_single_for_device,
	.sync_single_for_cpu = nommu_dma_sync_single_for_cpu,
	.sync_sg_for_device = nommu_dma_sync_sg_for_device,
	.sync_sg_for_cpu = nommu_dma_sync_sg_for_cpu,
};
EXPORT_SYMBOL(aw_andes_dma_ops);
#endif

MODULE_DESCRIPTION("dma ops adapter");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("ouyangkun <ouyangkun@allwinnertech.com>");
MODULE_VERSION("1.0.0");