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

#include "super_lcd_driver.h"
#include "../of_service.h"
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#define DP printk("[DEBUG] %s, %s, %d \n", __FILE__, __func__, __LINE__);

#define GAMMA_TBL_ROW       30
#define GAMMA_TBL_COLUMN    2

static int get_gamma_tbl_from_dts(char *node_name, char *subname, u8 *ptbl, int *row_size)
{
	int ret;
	struct device_node *lcd0_node;
	/* Get property, that is flag property from lcd0 */
	char command_name[32];
	int command_len;
	int i;
	u32 data[2];

	lcd0_node = get_node_by_name(node_name);

	for (i = 0; ; i++) {
		/* Get property */
		command_len = sprintf(command_name, "%s_%d", subname, i);

		if (command_len > 32) {
			DE_INFO("[WARN] command_name is too long \n");
		}

		ret = of_property_read_variable_u32_array(lcd0_node, command_name, data, 0, ARRAY_SIZE(data));

		if (ret < 0) {
			DE_WARN("property %s is not found \n", command_name);
			break;
		}

		*(ptbl + i * GAMMA_TBL_COLUMN + 0) = (u8) data[0];
		*(ptbl + i * GAMMA_TBL_COLUMN + 1) = (u8) data[1];

	}

	*row_size = i;
	return 0;
}

static void lcd_cfg_panel_info(struct panel_extend_para *info)
{
	u32 i = 0, j = 0;
	u32 items;
	u8 lcd_gamma_tbl[GAMMA_TBL_ROW][GAMMA_TBL_COLUMN];
	int gamma_tbl_row_size = 0;
	int ret;

	u32 lcd_cmap_tbl[2][3][4] = {
	{
		{LCD_CMAP_G0, LCD_CMAP_B1, LCD_CMAP_G2, LCD_CMAP_B3},
		{LCD_CMAP_B0, LCD_CMAP_R1, LCD_CMAP_B2, LCD_CMAP_R3},
		{LCD_CMAP_R0, LCD_CMAP_G1, LCD_CMAP_R2, LCD_CMAP_G3},
		},
		{
		{LCD_CMAP_B3, LCD_CMAP_G2, LCD_CMAP_B1, LCD_CMAP_G0},
		{LCD_CMAP_R3, LCD_CMAP_B2, LCD_CMAP_R1, LCD_CMAP_B0},
		{LCD_CMAP_G3, LCD_CMAP_R2, LCD_CMAP_G1, LCD_CMAP_R0},
		},
	};

	ret = get_gamma_tbl_from_dts("lcd0", "lcd_gamma", (u8 *)lcd_gamma_tbl, &gamma_tbl_row_size);

	items = gamma_tbl_row_size;

	if (items < 1) {
		DE_WARN("the size of lcd_gamma table is less then 1 \n");
		return;
	}

	for (i = 0; i < items - 1; i++) {
		u32 num = lcd_gamma_tbl[i+1][0] - lcd_gamma_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value = lcd_gamma_tbl[i][1] +
				((lcd_gamma_tbl[i+1][1] - lcd_gamma_tbl[i][1])
				* j) / num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] =
							(value<<16)
							+ (value<<8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items-1][1]<<16) +
					(lcd_gamma_tbl[items-1][1]<<8)
					+ lcd_gamma_tbl[items-1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

unsigned long get_func_pointer(u32 opcode)
{
	int func_table_line;
	int i;

	func_table_line = sizeof(lcd_func_table) / 2 / sizeof(unsigned long);

	for (i = 0; i < func_table_line; i++) {
		if (lcd_func_table[i][0] == opcode) {
			return lcd_func_table[i][1];
		}
	}

	return 0;

}

void handle_data(u32 sel, u32 *data)
{
	u32 opcode = *data;
	unsigned long p_func_addr;
	u32 cmd_count;
	u8 cmd_params[15] = {0};
	int i;

	p_func_addr = get_func_pointer(opcode);

	if (p_func_addr == 0) {
		DE_WARN("not found function in func_table by opcode: %u \n", opcode);
		return;
	}

	/* Get function and exec by opcode */
	if (opcode < 10) {
		void (*p_func)(u32) = (void *)p_func_addr;
		(*p_func)(sel);
	} else if (opcode < 20) {
		s32 (*p_func)(u32) = (void *)p_func_addr;
		(*p_func)(sel);
	} else if (opcode < 30) {
		/* void (*p_func)(u32, u32, u32) = (void *)p_func_addr; */
		/* (*p_func)(sel, ); */
	} else if (opcode < 40) {
		s32 (*p_func)(u32) = (void *)p_func_addr;
		(*p_func)((*(data + 1)));
	} else if (opcode < 50) {
		void (*p_func)(u32, u32) = (void *)p_func_addr;
		(*p_func)(sel, (*(data + 1)));
	} else if (opcode < 70) {
		s32 (*p_func)(u32, u32, u32) = (void *)p_func_addr;
		(*p_func)(sel, (*(data + 1)), (*(data + 2)));
	} else if (opcode < 80) {
		/* s32 (*p_func)(u32, u8 *, u32) = (void *)p_func_addr; */
		/* (*p_func)(sel, ); */
	} else if (opcode < 90) {
		/* s32 (*p_func)(u32 sel, u8, u8 *, u32 *) = (void *)p_func; */
	} else if (opcode < 100) {
		s32 (*p_func)(u32, u8, u8 *, u32) = (void *)p_func_addr;
		/* data : [opcode cmd param_count params] */

		cmd_count = (*(data + 2));

		if (cmd_count > 15) {
			DE_WARN("paras are more then 15 \n");
			return;
		}

		for (i = 0; i < cmd_count; i++) {
			cmd_params[i] = *(data + 3 + i);
		}

		(*p_func)(sel, (u8)(*(data + 1)), cmd_params, cmd_count);
	} else {
		DE_WARN("opcode can not be exec \n");
	}

}

void exec_command(u32 sel, char *node_name, char *subname)
{
	int ret;
	struct device_node *lcd0_node;
	/* Get property, that is flag property from lcd0 */
	char command_name[32];
	int command_len;
	int i;
	u32 data[20];  /* Should define a macro */

	lcd0_node = get_node_by_name(node_name);

	for (i = 0; ; i++) {
		/* Get property */
		command_len = sprintf(command_name, "%s_%d", subname, i);

		if (command_len > 32) {
			DE_WARN("command_name is too long \n");
		}

		ret = of_property_read_variable_u32_array(lcd0_node, command_name, data, 0, ARRAY_SIZE(data));
		/* printk("ret = %d, command_namd : %s \n", ret, command_name); */

		if (ret < 0) {
			/* printk("[INFO]property %s is not found \n", node_name); */
			break;
		}

		handle_data(sel, data);
	}

}

static s32 lcd_open_flow(u32 sel)
{
	exec_command(sel, "lcd0", "lcd_open_command");

	return 0;
}

static s32 lcd_close_flow(u32 sel)
{
	exec_command(sel, "lcd0", "lcd_close_command");

	return 0;
}

static s32 lcd_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

struct __lcd_panel  super_lcd_panel = {
	.name = "super_lcd_driver",
	.func = {
		.cfg_panel_info = lcd_cfg_panel_info,
		.cfg_open_flow = lcd_open_flow,
		.cfg_close_flow = lcd_close_flow,
		.lcd_user_defined_func = lcd_user_defined_func,
	},
};
