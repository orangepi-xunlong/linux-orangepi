/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2017 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * he0801a-068 panel driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include "kd080d24.h"

static void lcd_power_on(u32 sel);
static void lcd_power_off(u32 sel);
static void lcd_bl_open(u32 sel);
static void lcd_bl_close(u32 sel);

static void lcd_panel_init(u32 sel);
static void lcd_panel_exit(u32 sel);

#define panel_reset(sel, val) sunxi_lcd_gpio_set_value(sel, 0, val)

static void lcd_cfg_panel_info(struct panel_extend_para *info)
{
	u32 i = 0, j = 0;
	u32 items;
	u8 lcd_gamma_tbl[][2] = {
		{0, 0},
		{15, 15},
		{30, 30},
		{45, 45},
		{60, 60},
		{75, 75},
		{90, 90},
		{105, 105},
		{120, 120},
		{135, 135},
		{150, 150},
		{165, 165},
		{180, 180},
		{195, 195},
		{210, 210},
		{225, 225},
		{240, 240},
		{255, 255},
	};

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

	items = sizeof(lcd_gamma_tbl) / 2;
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

static s32 lcd_open_flow(u32 sel)
{
	LCD_OPEN_FUNC(sel, lcd_bl_open, 0);
	LCD_OPEN_FUNC(sel, lcd_power_on, 10);
	LCD_OPEN_FUNC(sel, lcd_panel_init, 60);
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 50);

	return 0;
}

static s32 lcd_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, lcd_bl_close, 0);
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 0);
	LCD_CLOSE_FUNC(sel, lcd_panel_exit, 200);
	LCD_CLOSE_FUNC(sel, lcd_power_off, 500);

	return 0;
}

static void lcd_power_on(u32 sel)
{
	sunxi_lcd_power_enable(sel, 0);
	sunxi_lcd_delay_ms(10);
	sunxi_lcd_power_enable(sel, 1);
	sunxi_lcd_delay_ms(10);
	sunxi_lcd_pin_cfg(sel, 1);
	sunxi_lcd_delay_ms(50);
	panel_reset(sel, 1);
	sunxi_lcd_delay_ms(100);
	panel_reset(sel, 0);
	sunxi_lcd_delay_ms(100);
	panel_reset(sel, 1);
	sunxi_lcd_delay_ms(100);
}

static void lcd_power_off(u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 0);
	sunxi_lcd_delay_ms(20);
	panel_reset(sel, 0);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 1);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 0);
}

static void lcd_bl_open(u32 sel)
{
	sunxi_lcd_pwm_enable(sel);
	sunxi_lcd_backlight_enable(sel);
}

static void lcd_bl_close(u32 sel)
{
	sunxi_lcd_backlight_disable(sel);
	sunxi_lcd_pwm_disable(sel);
}

#define REGFLAG_DELAY               0xFE
#define REGFLAG_END_OF_TABLE        0xFF   // END OF REGISTERS MARKER

struct LCM_setting_table {
	__u8 cmd;
	__u32 count;
	__u8 para_list[64];
};

static struct LCM_setting_table initialization_setting[] = {
	{0xb9, 3, {0xff, 0x83, 0x94}},
	{0xba, 2, {0x73, 0x83}},
	{0xB1, 15, {0x6C, 0x15, 0x15, 0x24, 0xE4, 0x11, 0xF1, 0x80, 0xE4, 0xD7, 0x23, 0x80, 0xC0, 0xD2, 0x58}},
	{0xB2, 11, {0x00, 0x64, 0x10, 0x07, 0x20, 0x1C, 0x08, 0x08, 0x1C, 0x4D, 0x00}},
	{0xB4, 12, {0x00, 0xFF, 0x03, 0x5A, 0x03, 0x5A, 0x03, 0x5A, 0x01, 0x6A, 0x01, 0x6A}},
	{0xD3, 30, {0x00, 0x06, 0x00, 0x40, 0x1A, 0x08, 0x00, 0x32, 0x10,
					0x07, 0x00, 0x07, 0x54, 0x15, 0x0F, 0x05, 0x04, 0x02, 0x12,
				0x10, 0x05, 0x07, 0x33, 0x33, 0x0B, 0x0B, 0x37, 0x10, 0x07,
				0x07}},
	{0xD5, 44, {0x19, 0x19, 0x18, 0x18, 0x1A, 0x1A, 0x1B, 0x1B, 0x04,
				0x05, 0x06, 0x07, 0x00, 0x01, 0x02, 0x03, 0x20, 0x21, 0x18,
				0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
				0x18, 0x22, 0x23, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
				0x18, 0x18, 0x18, 0x18, 0x18}},
	{0xD6, 44, {0x18, 0x18, 0x19, 0x19, 0x1A, 0x1A, 0x1B, 0x1B, 0x03,
				0x02, 0x01, 0x00, 0x07, 0x06, 0x05, 0x04, 0x23, 0x22, 0x18,
				0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
				0x18, 0x21, 0x20, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
				0x18, 0x18, 0x18, 0x18, 0x18}},
	{0xE0, 42, {0x00, 0x06, 0x0C, 0x31, 0x34, 0x3F, 0x1D, 0x41, 0x06,
				0x0A, 0x0C, 0x17, 0x0F, 0x12, 0x15, 0x13, 0x14, 0x07, 0x12,
				0x15, 0x16, 0x00, 0x06, 0x0B, 0x30, 0x34, 0x3F, 0x1D, 0x40,
				0x07, 0x0A, 0x0D, 0x18, 0x0E, 0x12, 0x14, 0x12, 0x14, 0x08,
				0x13, 0x14, 0x19}},
	{0xB6, 2, {0x3A, 0x3A}},
	{0xCC, 1, {0x09}},
	{0xD2, 1, {0x55}},
	{0xC0, 2, {0x30, 0x14}},
	{0xBF, 3, {0x41, 0x0E, 0x01}},
	{0xC7, 4, {0x00, 0xC0, 0x40, 0xC0}},
	{0xDF, 1, {0x8E}},
	{0x11, 0, {}},
	{REGFLAG_DELAY, 0, {120}},
	{0x29, 0, {}},
	// Setting ending by predefined flag
	{REGFLAG_END_OF_TABLE, REGFLAG_END_OF_TABLE, {}},
};

static void lcd_panel_init(u32 sel)
{
	int index;
	sunxi_lcd_dsi_clk_enable(sel);

	for (index = 0;; ++index) {
		if (initialization_setting[index].count == REGFLAG_END_OF_TABLE)
			break;
		else if (initialization_setting[index].count == REGFLAG_DELAY)
			sunxi_lcd_delay_ms(initialization_setting[index].para_list[0]);
		else
			sunxi_lcd_dsi_dcs_write(sel, initialization_setting[index].cmd,
						    initialization_setting[index].para_list,
							initialization_setting[index].count);
	}

}

static void lcd_panel_exit(u32 sel)
{
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x10);
	sunxi_lcd_delay_ms(80);
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x28);
	sunxi_lcd_delay_ms(50);
}

/*sel: 0:lcd0; 1:lcd1*/
static s32 lcd_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

struct __lcd_panel kd080d24_panel = {
	/* panel driver name, must mach the name of
	 * lcd_drv_name in sys_config.fex
	 */
	.name = "kd080d24",
	.func = {
		.cfg_panel_info = lcd_cfg_panel_info,
			.cfg_open_flow = lcd_open_flow,
			.cfg_close_flow = lcd_close_flow,
			.lcd_user_defined_func = lcd_user_defined_func,
	},
};
