/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2017 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * eqt371byz009g panel driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
&lcd0 {
	lcd_used            = <1>;
	status              = "okay";
	lcd_driver_name     = "eqt371byz009g";
	lcd_backlight       = <50>;
	lcd_if              = <4>;

	lcd_x               = <480>;
	lcd_y               = <960>;
	lcd_width           = <23>;
	lcd_height          = <77>;
	lcd_dclk_freq       = <34>;

	lcd_pwm_used        = <1>;
	lcd_pwm_ch          = <16>;
	lcd_pwm_freq        = <50000>;
	lcd_pwm_pol         = <0>;
	lcd_pwm_max_limit   = <255>;
	lcd_pwm_name        = "backlight";

	lcd_hbp             = <25>;
	lcd_ht              = <545>;
	lcd_hspw            = <5>;
	lcd_vbp             = <45>;
	lcd_vt              = <1045>;
	lcd_vspw            = <5>;
	lcd_start_delay     = <5>;

	lcd_frm             = <0>;
	lcd_gamma_en        = <0>;
	lcd_bright_curve_en = <0>;
	lcd_cmap_en         = <0>;

	deu_mode            = <0>;
	lcdgamma4iep        = <22>;
	smart_color         = <90>;

	lcd_dsi_if          = <0>;
	lcd_dsi_lane        = <2>;
	lcd_dsi_format      = <0>;
	lcd_dsi_te          = <0>;
	lcd_dsi_eotp        = <0>;

	lcd_power = "cldo4";
	lcd_power1 = "cldo1";

	lcd_bl_en = <&pio PD 23 GPIO_ACTIVE_HIGH>;
	lcd_gpio_0 = <&pio PD 22 GPIO_ACTIVE_HIGH>; //reset

	pinctrl-0 = <&dsi0_4lane_pins_a>;
	pinctrl-1 = <&dsi0_4lane_pins_b>;
};
 */
#include "eqt371byz009g.h"

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
	    {0, 0},     {15, 15},   {30, 30},   {45, 45},   {60, 60},
	    {75, 75},   {90, 90},   {105, 105}, {120, 120}, {135, 135},
	    {150, 150}, {165, 165}, {180, 180}, {195, 195}, {210, 210},
	    {225, 225}, {240, 240}, {255, 255},
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
		u32 num = lcd_gamma_tbl[i + 1][0] - lcd_gamma_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value =
			    lcd_gamma_tbl[i][1] +
			    ((lcd_gamma_tbl[i + 1][1] - lcd_gamma_tbl[i][1]) *
			     j) /
				num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] =
			    (value << 16) + (value << 8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items - 1][1] << 16) +
				   (lcd_gamma_tbl[items - 1][1] << 8) +
				   lcd_gamma_tbl[items - 1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));
}

static s32 lcd_open_flow(u32 sel)
{
	LCD_OPEN_FUNC(sel, lcd_power_on, 0);
	LCD_OPEN_FUNC(sel, lcd_panel_init, 10);
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 40);
	LCD_OPEN_FUNC(sel, lcd_bl_open, 0);
	return 0;
}

static s32 lcd_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, lcd_bl_close, 0);
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 0);
	LCD_CLOSE_FUNC(sel, lcd_panel_exit, 10);
	LCD_CLOSE_FUNC(sel, lcd_power_off, 0);

	return 0;
}

static void lcd_power_on(u32 sel)
{
	sunxi_lcd_power_enable(sel, 0);
	sunxi_lcd_delay_ms(10);
	sunxi_lcd_power_enable(sel, 1);
	sunxi_lcd_delay_ms(10);

	/* reset lcd by gpio */
	sunxi_lcd_pin_cfg(sel, 1);
	sunxi_lcd_delay_ms(10);
	panel_reset(sel, 1);
	sunxi_lcd_delay_ms(5);
	panel_reset(sel, 0);
	sunxi_lcd_delay_ms(20);
	panel_reset(sel, 1);
	sunxi_lcd_delay_ms(120);
}

static void lcd_power_off(u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 0);
	sunxi_lcd_delay_ms(1);
	panel_reset(sel, 0);
	sunxi_lcd_delay_ms(1);
	sunxi_lcd_power_disable(sel, 1);
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

#define REGFLAG_DELAY         0XFE
#define REGFLAG_END_OF_TABLE  0xFC   /* END OF REGISTERS MARKER */

struct LCM_setting_table {
	u8 cmd;
	u32 count;
	u8 para_list[40];
};

static struct LCM_setting_table lcm_initialization_setting[] = {
	{0xFF, 5, {0x77, 0x01, 0x00, 0x00, 0x13}},
	{0xEF, 1, {0x08}},
	{0xFF, 5, {0x77, 0x01, 0x00, 0x00, 0x10}},
	{0xC0, 2, {0x77, 0x00}},
	{0xC1, 2, {0x11, 0x0C}},
	{0xC2, 2, {0x07, 0x02}},
	{0xCC, 1, {0x30}},
	{0xB0, 16, {0x06, 0xCF, 0x14, 0x0C, 0x0F, 0x03, 0x00, 0x0A, 0x07, 0x1B,
				0x03, 0x12, 0x10, 0x25, 0x36, 0x1E}},
	{0xB1, 16, {0x0C, 0xD4, 0x18, 0x0C, 0x0E, 0x06, 0x03, 0x06, 0x08, 0x23,
				0x06, 0x12, 0x10, 0x30, 0x2F, 0x1F}},
	{0xFF, 5, {0x77, 0x01, 0x00, 0x00, 0x11}},
	{0xB0, 1, {0x73}},
	{0xB1, 1, {0x7C}},
	{0xB2, 1, {0x83}},
	{0xB3, 1, {0x80}},
	{0xB5, 1, {0x49}},
	{0xB7, 1, {0x87}},
	{0xB8, 1, {0x33}},
	{0xB9, 2, {0x10, 0x1f}},
	{0xBB, 1, {0x03}},
	{0xC1, 1, {0x08}},
	{0xC2, 1, {0x08}},
	{0xD0, 1, {0x88}},
	{0xE0, 6, {0x00, 0x00, 0x02, 0x00, 0x00, 0x0c}},
	{0xE1, 11, {0x05, 0x96, 0x07, 0x96, 0x06, 0x96, 0x08, 0x96, 0x00, 0x44, 0x44}},
	{0xE2, 12, {0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00}},
	{0xE3, 4, {0x00, 0x00, 0x33, 0x33}},
	{0xE4, 2, {0x44, 0x44}},
	{0xE5, 16, {0x0d, 0xd4, 0x28, 0x8c, 0x0f, 0xd6, 0x28, 0x8c, 0x09, 0xd0, 0x28, 0x8c, 0x0b, 0xd2, 0x28, 0x8c}},
	{0xE6, 4, {0x00, 0x00, 0x33, 0x33}},
	{0xE7, 2, {0x44, 0x44}},
	{0xE8, 16, {0x0e, 0xd5, 0x28, 0x8c, 0x10, 0xd7, 0x28, 0x8c, 0x0a, 0xd1, 0x28, 0x8c, 0x0c, 0xd3, 0x28, 0x8c}},
	{0xEB, 6, {0x00, 0x01, 0xe4, 0xe4, 0x44, 0x00}},
	{0xED, 16, {0xf3, 0xc1, 0xba, 0x0f, 0x66, 0x77, 0x44, 0x55, 0x55, 0x44, 0x77, 0x66, 0xf0, 0xab, 0x1c, 0x3f}},
	{0xEF, 6, {0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F}},
	{0xFF, 5, {0x77, 0x01, 0x00, 0x00, 0x13}},
	{0xE8, 2, {0x00, 0x0E}},
	{0x35, 1, {0x00}},
	{0x11, 0, {}},
	{REGFLAG_DELAY, 800, {}},
	{0xE8, 2, {0x40, 0x00}},
	{0xFF, 5, {0x77, 0x01, 0x00, 0x00, 0x00}},
	{0xFF, 5, {0x77, 0x01, 0x00, 0x00, 0x10}},
	{0xBB, 1, {0x00}},
	{0xCD, 1, {0x00}},
	{0xCA, 1, {0x11}},
	{0xBC, 1, {0x01}},
	{0x51, 1, {0x3F}},
	{0x53, 1, {0x0C}},
	{0x55, 1, {0x00}},
	{0x5E, 1, {0x03}},
	{0x29, 0, {}},
	{REGFLAG_DELAY, 200, {}},
	{0xFF, 5, {0x77, 0x01, 0x00, 0x00, 0x10}},
	{0xE5, 2, {0x00, 0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void lcd_panel_init(u32 sel)
{
	u32 i = 0;

	sunxi_lcd_dsi_clk_enable(sel);
	sunxi_lcd_delay_ms(10);

	for (i = 0;; i++) {
		if (lcm_initialization_setting[i].cmd == REGFLAG_END_OF_TABLE) {
			break;
		} else if (lcm_initialization_setting[i].cmd == REGFLAG_DELAY) {
			sunxi_lcd_delay_ms(lcm_initialization_setting[i].count);
		} else {
			dsi_dcs_wr(0, lcm_initialization_setting[i].cmd,
				   lcm_initialization_setting[i].para_list,
				   lcm_initialization_setting[i].count);
		}
	}
}

static void lcd_panel_exit(u32 sel)
{
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x28);
	sunxi_lcd_delay_ms(20);
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x10);
	sunxi_lcd_delay_ms(120);
}

/*sel: 0:lcd0; 1:lcd1*/
static s32 lcd_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

struct __lcd_panel eqt371byz009g_panel = {
	/* panel driver name, must mach the name of
	 * lcd_drv_name in sys_config.fex
	 */
	.name = "eqt371byz009g",
	.func = {
		.cfg_panel_info = lcd_cfg_panel_info,
			.cfg_open_flow = lcd_open_flow,
			.cfg_close_flow = lcd_close_flow,
			.lcd_user_defined_func = lcd_user_defined_func,
	},
};
