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
#ifndef __OF_SERVICE_H__
#define __OF_SERVICE_H__

#define NAME_LENGTH 32

struct device_node *get_node_by_name(char *main_name);
struct property *get_property_by_name(struct device_node *node,
		char *property_name, int *command_property_length);
struct property *get_property_by_index(struct device_node *node, char *config_start, int index);
int insert_property(struct device_node *node, struct property **new_pp, char *reference_name, int index);
int delete_property(struct device_node *np, struct property *prop);

#endif
