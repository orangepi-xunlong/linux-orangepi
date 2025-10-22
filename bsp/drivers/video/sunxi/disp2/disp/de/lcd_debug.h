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
#ifndef __LCD_DEBUG_H__
#define __LCD_DEBUG_H__

#define NAME_LENGTH 32
#define VALUE_LENGTH 100
#define BASE_PROPERTY_NAME "base_config_start"
#define OPEN_PROPERTY_NAME "lcd_open_flow_start"
#define CLOSE_PROPERTY_NAME "lcd_close_flow_start"

struct dt_property {
	 char *name;
	 unsigned int length;
	 void *value;
	 int index;
	 struct dt_property *next;
};

typedef enum {
	CREATE    = 1,
	RETRIEVIE,
	UPDATE,
	DELETE,
} request_opcode;

typedef enum {
	BASE    = 1,
	OPEN,
	CLOSE,
} opcode_flag;

struct para {
	opcode_flag flag;
	request_opcode opcode;
	struct dt_property prop_src;
	struct dt_property prop_dts;
};

int handle_request(struct para *lcd_debug_para, int lcd_node);
void copy_result_to_prop_dts(struct property *result, struct dt_property *prop_dts);
int retrieve_prop(struct para *lcd_debug_para);
int create_prop(struct para *lcd_debug_para);
int update_prop(struct para *lcd_debug_para);
int delete_prop(struct para *lcd_debug_para);
int init_device_node(char *node_name);

#endif
