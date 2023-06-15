/*
 * drivers/soc/sunxi-dump.c
 *
 * Copyright(c) 2019-2020 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Allwinner sunxi crash dump debug
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/sunxi-dump.h>
#include <soc/allwinner/sunxi_sip.h>

#define SUNXI_DUMP_COMPATIBLE "sunxi-dump"
static LIST_HEAD(dump_group_list);

struct sunxi_dump_group {
	char name[10];
	u32 *reg_buf;
	void __iomem *vir_base;
	phys_addr_t phy_base;
	u32 len;
	struct list_head list;
};

int sunxi_dump_group_reg(struct sunxi_dump_group *group)
{
	u32 *buf = group->reg_buf;
	void __iomem *membase = group->vir_base;
	u32 len = ALIGN(group->len, 4);
	int i;

	for (i = 0; i < len; i += 4)
		*(buf++) = readl(membase + i);

	return 0;
}

int sunxi_dump_group_dump(void)
{
	struct sunxi_dump_group *dump_group;

	list_for_each_entry(dump_group, &dump_group_list, list) {
		sunxi_dump_group_reg(dump_group);
	}
	return 0;
}
EXPORT_SYMBOL(sunxi_dump_group_dump);

int sunxi_dump_group_register(const char *name, phys_addr_t start, u32 len)
{
	struct sunxi_dump_group *dump_group = NULL;

	dump_group = kmalloc(sizeof(struct sunxi_dump_group), GFP_KERNEL);
	if (!dump_group)
		return -ENOMEM;

	memcpy(dump_group->name, name, sizeof(dump_group->name));
	dump_group->phy_base = start;
	dump_group->len = len;
	dump_group->vir_base = ioremap(dump_group->phy_base, dump_group->len);
	if (!dump_group->vir_base) {
		pr_err("%s can't iomap\n", dump_group->name);
		return -EINVAL;
	}
	dump_group->reg_buf = kmalloc(dump_group->len, GFP_KERNEL);
	if (!dump_group->reg_buf)
		return -ENOMEM;

	list_add_tail(&dump_group->list, &dump_group_list);
	return 0;
}

static int __init sunxi_dump_init(void)
{
	struct device_node *node;
	int i = 0;
	const char *name = NULL;
	struct resource res;

	node = of_find_compatible_node(NULL, NULL, SUNXI_DUMP_COMPATIBLE);

	for (i = 0; ; i++) {
		if (of_address_to_resource(node, i, &res))
			break;
		if (of_property_read_string_index(node, "group-names",
						i, &name))
			break;
		sunxi_dump_group_register(name, res.start, resource_size(&res));
	}

	return 0;
}

int sunxi_set_crashdump_mode(void)
{
	invoke_scp_fn_smc(ARM_SVC_SUNXI_CRASHDUMP_START, 0, 0, 0);
	while (1)
		cpu_relax();
}
late_initcall(sunxi_dump_init);
