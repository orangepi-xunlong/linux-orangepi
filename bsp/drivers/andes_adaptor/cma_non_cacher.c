// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2023 - 2025 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * turn cma buffer non cache to reduce pma usage
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

struct mem_region {
	uint64_t startAddr;
	uint64_t size;
	void *addr;
};
#define PMA_REGIONS 16
#define MAX_REGION_SIZE ((uint64_t)4 * 1024 * 1024 * 1024)
__maybe_unused static void generate_mem_map(uint64_t start_addr, uint64_t size,
					    struct mem_region *map_head,
					    int region_array_size)
{
	uint64_t thisRegionSize;
	int i;
	for (i = 0; i < region_array_size; i++) {
		thisRegionSize = MAX_REGION_SIZE;
		/*use as less regions as possible*/
		while (thisRegionSize > 32 * 1024) {
			if ((thisRegionSize <= size) &&
			    /*start addr must at multiple of region size*/
			    ((start_addr & (thisRegionSize - 1)) == 0))
				break;
			thisRegionSize >>= 1;
		}
		map_head->startAddr = start_addr;
		map_head->size = thisRegionSize;
		map_head++;

		start_addr += thisRegionSize;
		if (size >= thisRegionSize) {
			size -= thisRegionSize;
		} else {
			pr_err("non-cache no align, extra 0x%llx byte count in region!",
			       thisRegionSize - size);
			size = 0;
		}

		if (size == 0) {
			break;
		}
	}
	if (size) {
		pr_err("Not enough regions, 0x%llx byte ta ram left untouched",
		       size);
	}
}

__maybe_unused static int set_default_cma_non_cache(void)
{
	struct cma *default_cma = dev_get_cma_area(NULL);
	int ret = 0;
	unsigned long size;
	phys_addr_t base;
	struct mem_region regions[PMA_REGIONS];
	int i;

	if (!default_cma) {
		pr_err("default cma not found");
		return -ENOMEM;
	}

	size = cma_get_size(default_cma);
	base = cma_get_base(default_cma);
	memset(regions, 0, sizeof(regions));
	generate_mem_map(base, size, regions, ARRAY_SIZE(regions));
	for (i = 0; i < PMA_REGIONS; i++) {
		if (regions[i].size == 0)
			break; /*early exit*/
		regions[i].addr = ioremap_nocache(base, size);
		if (!regions[i].addr) {
			pr_err("failed to %llx~%llx in cma %s non-cache",
			       regions[i].startAddr,
			       regions[i].startAddr + regions[i].size,
			       cma_get_name(default_cma));
		} else {
			pr_err("base:%pad, size:%lx in cma set non-cache",
			       &base, size);
			/*
			 * disable movable page so programs dont accidently get
			 * cma memory and have performance impact
			 */
			page_group_by_mobility_disabled = 1;
		}
	}

	return ret;
}


arch_initcall(set_default_cma_non_cache);
MODULE_DESCRIPTION("cma buffer non-cacher");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("ouyangkun <ouyangkun@allwinnertech.com>");
MODULE_VERSION("1.0.0");
