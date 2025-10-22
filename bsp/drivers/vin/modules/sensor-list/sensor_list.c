/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * configs for sensor list.
 *
 * Copyright (c) 2020 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Li Huiyu <lihuiyu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "../../utility/vin_log.h"
#include "sensor_list.h"

#if IS_ENABLED(CONFIG_SENSOR_LIST_MODULE)

static int get_value_int(struct device_node *np, const char *name,
			  u32 *value)
{
	int ret;

	ret = of_property_read_u32(np, name, value);
	if (ret) {
		*value = 0;
		vin_log(VIN_LOG_CONFIG, "fetch %s from device_tree failed\n", name);
		return -EINVAL;
	}
	vin_log(VIN_LOG_CONFIG, "%s = %x\n", name, *value);
	return 0;
}

static int get_value_string(struct device_node *np, const char *name,
			    char *string)
{
	int ret;
	const char *const_str;

	ret = of_property_read_string(np, name, &const_str);
	if (ret) {
		strcpy(string, "");
		vin_log(VIN_LOG_CONFIG, "fetch %s from device_tree failed\n", name);
		return -EINVAL;
	}
	strcpy(string, const_str);
	vin_log(VIN_LOG_CONFIG, "%s = %s\n", name, string);
	return 0;
}

/*
static void set_used(struct sensor_list *sensors, const char *name, struct device_node *node, int sel)
{
	get_value_int(node, name, &sensors->used);
}

static void set_device_sel(struct sensor_list *sensors, const char *name, struct device_node *node, int sel)
{
	get_value_int(node, name, &sensors->device_sel);
}
*/
static void set_csi_sel(struct sensor_list *sensors, const char *name, struct device_node *node, int sel)
{
	get_value_int(node, name, &sensors->csi_sel);
}

static void set_mname(struct sensor_list *sensors, const char *name, struct device_node *node, int sel)
{
	get_value_string(node, name, sensors->inst[sel].cam_name);
}

static void set_twi_addr(struct sensor_list *sensors, const char *name, struct device_node *node, int sel)
{
	get_value_int(node, name, &sensors->inst[sel].cam_addr);
}

static void set_sensor_type(struct sensor_list *sensors, const char *name, struct device_node *node, int sel)
{
	get_value_int(node, name, &sensors->inst[sel].cam_type);
}

static void set_hflip(struct sensor_list *sensors, const char *name, struct device_node *node, int sel)
{
	get_value_int(node, name, &sensors->inst[sel].hflip);
}

static void set_vflip(struct sensor_list *sensors, const char *name, struct device_node *node, int sel)
{
	get_value_int(node, name, &sensors->inst[sel].vflip);
}

static void set_act_used(struct sensor_list *sensors, const char *name, struct device_node *node, int sel)
{
	get_value_int(node, name, &sensors->inst[sel].act_used);
}

static void set_act_name(struct sensor_list *sensors, const char *name, struct device_node *node, int sel)
{
	get_value_string(node, name, sensors->inst[sel].act_name);
}

static void set_act_twi_addr(struct sensor_list *sensors, const char *name, struct device_node *node, int sel)
{
	get_value_int(node, name, &sensors->inst[sel].act_addr);
}

static struct fetch_sl fetch_list[] = {
/*	{"used", 1, set_used},
	{"device_sel", 1, set_device_sel}, */
	{"csi_sel", 1, set_csi_sel},
	{"mname", 1, set_mname},
	{"twi_addr", 1, set_twi_addr},
	{"type", 1, set_sensor_type},
	{"hflip", 1, set_hflip},
	{"vflip", 1, set_vflip},
	{"act_used", 1, set_act_used},
	{"act_name", 1, set_act_name},
	{"act_twi_addr", 1, set_act_twi_addr},
};

#define FETCH_SIZE (ARRAY_SIZE(fetch_list))

int sensor_list_get_parms(struct sensor_list *sensors, struct device_node *node, int sel)
{
	int i, j;
	char property[32] = { 0 };

	sensors->detect_num = MAX_DETECT_NUM;

	for (j = 0; j < sensors->detect_num; j++) {
		for (i = 0; i < FETCH_SIZE; i++) {
			if (i < 1)
				sprintf(property, "%s", fetch_list[i].sub);
			else
				sprintf(property, "%s%d%d_%s", "sensor", sel, j, fetch_list[i].sub);
			if (fetch_list[i].flag)
				fetch_list[i].fun(sensors, property, node, j);
		}
	}

	return 0;
}

#else

int sensor_list_get_parms(struct sensor_list *sensors, struct device_node *node, int sel)
{
	return 0;
}
#endif
