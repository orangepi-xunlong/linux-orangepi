/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (c) 2021 Allwinnertech Co., Ltd.
 * Author: libairong <libairong@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "de/include.h"
#include "of_service.h"
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

struct device_node *get_node_by_name(char *main_name)
{
	char compat[NAME_LENGTH];
	u32 len = 0;
	struct device_node *node;

	len = sprintf(compat, "allwinner,sunxi-%s", main_name);

	if (len > 32)
		DE_WARN("size of mian_name is out of range\n");

	node = of_find_compatible_node(NULL, NULL, compat);

	if (!node) {
		DE_WARN("of_find_compatible_node %s fail\n", compat);
		return NULL;
	}

	return node;
}
/**
 * get_property_by_name - retrieve a property from of by name
 */
struct property *get_property_by_name(struct device_node *node, char *property_name, int *property_length)
{
	struct property *pp;

	pp = of_find_property(node, property_name, property_length);
	if (pp == NULL) {
		DE_WARN("pp is not found! pp name : %s \n", property_name);
		return NULL;
	}

	return pp;
}

/**
 * get_property_by_index - retrieve a property from of by index
 */
struct property *get_property_by_index(struct device_node *node, char *config_start, int index)
{
	struct property *pp;
	int i;
	int length;

	pp = get_property_by_name(node, config_start, &length);
	/* DE_WARN("property index : %d \n", index); */
	for (i = 0; pp; pp = pp->next, i++) {
		if (i == index - 1) {
			return pp;
		}
	}

	return pp;

}

int insert_property(struct device_node *node, struct property **new_pp, char *reference_name, int index)
{
	struct property **pp;
	int i;

	pp = &node->properties;
	if (pp == NULL) {
		return 1;
	}

	/* Find start property by reference name */
	while (*pp) {
		if (strcmp(reference_name, (*pp)->name) == 0) {
			break;
		}

		pp = &(*pp)->next;
	}

	for (i = 0; i < index; i++, pp = &(*pp)->next) {
		if (i == (index - 1)) {
			/* Insert */
			(*new_pp)->next = (*pp)->next;
			(*pp)->next = (*new_pp);
			return 0;
		}
	}

	return 2;
}

/* FIXME, can not to call function, when build disp to be a module. */
#ifdef MODULE
int of_remove_property(struct device_node *np, struct property *prop)
{
	DE_INFO("Can not to remove property in device tree, when disp was built to be a module \n");
	return -1;
}
#endif

int delete_property(struct device_node *np, struct property *prop)
{
	return of_remove_property(np, prop);
}
