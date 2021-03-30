/*
 * Copyright (c) 2011-2020 yanggq.young@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include "pm.h"

int pm_get_dev_info(char *name, int index, u32 **base, u32 *len)
{
	struct device_node *np;
	u32 reg[4 * (index + 1)];

	np = of_find_node_by_type(NULL, name);
	if (NULL == np) {
		pr_err("can not find np for %s.\n", name);
	} else {
		of_property_read_u32_array(np, "reg", reg, ARRAY_SIZE(reg));
		*base = (u32 *) ((phys_addr_t) reg[1 + index * 4]);
		pr_debug("%s physical base = 0x%p.\n", name, *base);

		*len = reg[3 + index * 4];
		*base = of_iomap(np, index);
		pr_debug("virtual base = 0x%p.\n", *base);
	}

	return 0;

}
