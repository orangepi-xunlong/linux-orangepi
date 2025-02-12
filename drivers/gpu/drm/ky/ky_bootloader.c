// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/mm.h>
#include <linux/memblock.h>

static bool ky_dpu_free_logo = false;

int ky_dpu_free_bootloader_mem(struct reserved_mem *rmem)
{
	void __iomem *rmem_vaddr_start;
	void __iomem *rmem_vaddr_end;

	if (ky_dpu_free_logo)
		return 0;

	pr_info("Reserved memory: detected bootloader logo memory at %pa, size %ld MB\n",
		&rmem->base, (unsigned long)rmem->size / SZ_1M);

	rmem_vaddr_start = phys_to_virt(rmem->base);
	rmem_vaddr_end = phys_to_virt(rmem->base + rmem->size - 1);

	free_reserved_area(rmem_vaddr_start, rmem_vaddr_end, -1, "framebuffer");

	ky_dpu_free_logo = true;

	return 0;
}
