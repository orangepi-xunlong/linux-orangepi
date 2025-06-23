// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 * Author: Zichar Zhang <zichar.zhang@cixtech.com>
 */

#include <linux/acpi.h>
#include <linux/cma.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>

enum {
	RESERVED_MEM_NOMAP = 0,
	RESERVED_MEM_RESERVE,
};

struct reserved_mem_entry {
	const char *name;
	phys_addr_t base;
	phys_addr_t size;
	int type;
};

extern void __init fdt_reserved_mem_save_node(unsigned long node,
			const char *uname, phys_addr_t base, phys_addr_t size);

static struct reserved_mem_entry reserved_mem[] = {
	{"audio", 0xd0000000, 0x00e00000, RESERVED_MEM_NOMAP},
	{ "dsp_vdev0buffer", 0xcde08000, 0x00100000, RESERVED_MEM_NOMAP},
	{ "dsp_reserved", 0xce000000, 0x1000000, RESERVED_MEM_NOMAP},
	{ "dsp_reserved_heap", 0xcf000000, 0x1000000, RESERVED_MEM_NOMAP},
	{ "dsp_vdev0vring", 0xcde00000, 0x8000, RESERVED_MEM_NOMAP},
};

static acpi_status __init reserved_mem_handler(u32 event,
			void *table, void *context)
{
	struct reserved_mem_entry *rmem;
	int i, count;

	count = sizeof(reserved_mem)/sizeof(struct reserved_mem_entry);
	for (i = 0; i < count; i++) {
		rmem = &reserved_mem[i];

		if (!rmem->base || !rmem->size)
			continue;

		if (rmem->type == RESERVED_MEM_NOMAP)
			memblock_mark_nomap(rmem->base, rmem->size);
		else if (rmem->type == RESERVED_MEM_RESERVE)
			memblock_reserve(rmem->base, rmem->size);

		fdt_reserved_mem_save_node(0, rmem->name, rmem->base, rmem->size);
	}

	return acpi_remove_table_handler(reserved_mem_handler);
}

static int __init acpi_reserved_memory_init(char *p)
{
	if (!p || strcmp(p, "force"))
		return 0;

	acpi_install_table_handler(reserved_mem_handler, NULL);

	return 0;
}
early_param("acpi", acpi_reserved_memory_init);
