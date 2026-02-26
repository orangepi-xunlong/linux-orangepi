/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include "disp_lcd.h"
#include "../of_service.h"
#include "lcd_debug.h"

static struct device_node *node;

/**
 * copy_result_to_prop_dts
 */
void copy_result_to_prop_dts(struct property *result, struct dt_property *prop_dts)
{
	memcpy(prop_dts->name, result->name, NAME_LENGTH);
	memcpy(prop_dts->value, result->value, result->length);
	prop_dts->length = result->length;
}

int retrieve_prop(struct para *lcd_debug_para)
{
	struct dt_property *prop_src = &lcd_debug_para->prop_src;
	struct property *result;

	if (prop_src->index == 0) {
		result = get_property_by_name(node, prop_src->name, &prop_src->length);

		if (result == NULL) {
			return -1;
		}

		copy_result_to_prop_dts(result, &lcd_debug_para->prop_dts);
		return 0;
	} else {
		char *reference_name;

		switch (lcd_debug_para->flag) {
		case BASE:
			reference_name = BASE_PROPERTY_NAME;
			break;
		case OPEN:
			reference_name = OPEN_PROPERTY_NAME;
			break;
		case CLOSE:
			reference_name = CLOSE_PROPERTY_NAME;
			break;
		}
		result = get_property_by_index(node, reference_name, prop_src->index);

		if (result == NULL) {
			return -1;
		}

		copy_result_to_prop_dts(result, &lcd_debug_para->prop_dts);
		lcd_debug_para->prop_dts.index = prop_src->index;
		return 0;
	}
}

/* insert a new property after prop_src-name and index */
int create_prop(struct para *lcd_debug_para)
{
	char *reference_name;
	struct property *pp;
	int ret;

	pp = kcalloc(1, sizeof(struct property), GFP_KERNEL | __GFP_ZERO);
	if (!pp) {
		DE_WARN("calloc memory fail\n");
		return -1;
	}
	pp->name = kmalloc(NAME_LENGTH, GFP_KERNEL | __GFP_ZERO);
	if (!pp->name) {
		DE_WARN("malloc memory fail\n");
		kfree(pp);
		return -1;
	}
	pp->value = kmalloc(VALUE_LENGTH, GFP_KERNEL | __GFP_ZERO);
	if (!pp->value) {
		DE_WARN("malloc memory fail\n");
		kfree(pp->name);
		kfree(pp);
		return -1;
	}

	strcpy(pp->name, lcd_debug_para->prop_src.name);
	memcpy(pp->value, lcd_debug_para->prop_src.value, lcd_debug_para->prop_src.length);
	pp->length = lcd_debug_para->prop_src.length;

	switch (lcd_debug_para->flag) {
	case BASE:
		reference_name = BASE_PROPERTY_NAME;
		break;
	case OPEN:
		reference_name = OPEN_PROPERTY_NAME;
		break;
	case CLOSE:
		reference_name = CLOSE_PROPERTY_NAME;
		break;
	default:
		goto free_mem;
	}
	ret = insert_property(node, &pp, reference_name, lcd_debug_para->prop_src.index);
	if (ret == 0) {
		return 0;
	}

free_mem:
	kfree(pp->name);
	kfree(pp->value);
	kfree(pp);
	return 2;
}

/* FIXME, can not to call function, when build disp to be a module. */
#ifdef MODULE
int of_update_property(struct device_node *np, struct property *newprop)
{
	DE_INFO("Can not to update a property in device tree, when disp was built to be a module \n");
	return -1;
}
#endif

/* Update a property by name */
int update_prop(struct para *lcd_debug_para)
{
	struct property *pp;
	struct property *newprop;
	struct dt_property *prop_src;
	struct dt_property *prop_dts;
	int length;

	prop_src = &(lcd_debug_para->prop_src);
	prop_dts = &(lcd_debug_para->prop_dts);

	pp = get_property_by_name(node, prop_src->name, &length);

	if (pp == NULL) {
		return 1;
	}

	newprop = kmalloc(sizeof(struct property), GFP_KERNEL | __GFP_ZERO);
	if (!newprop) {
		DE_WARN("malloc memory fail\n");
		return -1;
	}
	newprop->name = kmalloc(NAME_LENGTH, GFP_KERNEL | __GFP_ZERO);
	if (!newprop->name) {
		DE_WARN("malloc memory fail\n");
		kfree(newprop);
		return -1;
	}
	newprop->value = kmalloc(VALUE_LENGTH, GFP_KERNEL | __GFP_ZERO);
	if (!newprop->value) {
		DE_WARN("malloc memory fail\n");
		kfree(newprop->name);
		kfree(newprop);
		return -1;
	}

	memcpy(newprop->name, prop_src->name, strlen(prop_src->name));
	memcpy(newprop->value, prop_src->value, prop_src->length);
	newprop->length = prop_src->length;

	return of_update_property(node, newprop);
}

int delete_prop(struct para *lcd_debug_para)
{
	struct property *pp;
	struct dt_property *prop_src;
	int length;

	prop_src = &(lcd_debug_para->prop_src);

	pp = get_property_by_name(node, prop_src->name, &length);
	if (pp == NULL) {
		return 1;
	}

	return delete_property(node, pp);

}

int init_device_node(char *node_name)
{
	node = get_node_by_name(node_name);
	return 0;
}

static bool flag;
int handle_request(struct para *lcd_debug_para, int lcd_node)
{
	char lcd_node_name[10];

	if (flag == 0) {
		sprintf(lcd_node_name, "lcd%d", lcd_node);
		init_device_node(lcd_node_name);
		flag = 1;
	}
	switch (lcd_debug_para->opcode) {
	case CREATE:
	      {
			return create_prop(lcd_debug_para);
	      }
	case RETRIEVIE:
	      {
			return retrieve_prop(lcd_debug_para);
	      }
	case UPDATE:
	      {
			return update_prop(lcd_debug_para);
	      }
	case DELETE:
	      {
			return delete_prop(lcd_debug_para);
	      }
	default:
	      {
		return -1;
	      }
	}
}
