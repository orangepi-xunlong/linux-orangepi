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
#include "K101_MM2QA01_A.h"
#include "panels.h"

extern s32 bsp_disp_get_panel_info(u32 screen_id, struct disp_panel_para *info);
static void LCD_power_on(u32 sel);
static void LCD_power_off(u32 sel);
static void LCD_bl_open(u32 sel);
static void LCD_bl_close(u32 sel);

static void LCD_panel_init(u32 sel);
static void LCD_panel_exit(u32 sel);

/* static u8 const mipi_dcs_pixel_format[4] = {0x77, 0x66, 0x66, 0x55}; */
#define panel_reset(val) sunxi_lcd_gpio_set_value(sel, 1, val)
#define power_en(val) sunxi_lcd_gpio_set_value(sel, 0, val)

static void LCD_cfg_panel_info(struct panel_extend_para *info)
{
	u32 i = 0, j = 0;
	u32 items;
	u8 lcd_gamma_tbl[][2] = {
		/* {input value, corrected value} */
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
		u32 num = lcd_gamma_tbl[i + 1][0] - lcd_gamma_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value = lcd_gamma_tbl[i][1] + ((lcd_gamma_tbl[i+1][1] - lcd_gamma_tbl[i][1]) * j) / num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] = (value<<16) + (value<<8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items - 1][1]<<16) + (lcd_gamma_tbl[items - 1][1]<<8) + lcd_gamma_tbl[items - 1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

static s32 LCD_open_flow(u32 sel)
{
	LCD_OPEN_FUNC(sel, LCD_power_on, 50);  /* open lcd power, and delay 180ms */
	LCD_OPEN_FUNC(sel, LCD_panel_init, 200);  /* open lcd power, than delay 200ms */
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 100);  /* open lcd controller, and delay 50ms */
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);  /* open lcd backlight, and delay 0ms */

	return 0;
}

static s32 LCD_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 0);  /* close lcd backlight, and delay 0ms */
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 0);  /* close lcd controller, and delay 0ms */
	LCD_CLOSE_FUNC(sel, LCD_panel_exit,	20);  /* open lcd power, than delay 200ms */
	LCD_CLOSE_FUNC(sel, LCD_power_off, 50);  /* close lcd power, and delay 500ms */

	return 0;
}

static void LCD_power_on(u32 sel)
{
	DE_INFO("\n\n\n\n\n");
	panel_reset(0);
	sunxi_lcd_power_enable(sel, 0);  /* config lcd_power pin to open lcd power */
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_enable(sel, 1);  /* config lcd_power pin to open lcd power1 */
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_enable(sel, 2);  /* config lcd_power pin to open lcd power2 */
	sunxi_lcd_delay_ms(5);
	power_en(1);
	sunxi_lcd_delay_ms(20);

	/* panel_reset(1); */
	sunxi_lcd_delay_ms(40);
	panel_reset(1);
	/* sunxi_lcd_delay_ms(10); */
	/* panel_reset(0); */
	/* sunxi_lcd_delay_ms(5); */
	/* panel_reset(1); */
	sunxi_lcd_delay_ms(120);
	/* sunxi_lcd_delay_ms(5); */

	sunxi_lcd_pin_cfg(sel, 1);
}

static void LCD_power_off(u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 0);
	power_en(0);
	sunxi_lcd_delay_ms(20);
	panel_reset(0);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 2);  /* config lcd_power pin to close lcd power2 */
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 1);  /* config lcd_power pin to close lcd power1 */
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 0);  /* config lcd_power pin to close lcd power */
}

static void LCD_bl_open(u32 sel)
{
	sunxi_lcd_pwm_enable(sel);
	sunxi_lcd_delay_ms(50);
	sunxi_lcd_backlight_enable(sel);  /* config lcd_bl_en pin to open lcd backlight */
}

static void LCD_bl_close(u32 sel)
{
	sunxi_lcd_backlight_disable(sel);  /* config lcd_bl_en pin to close lcd backlight */
	sunxi_lcd_delay_ms(20);
	sunxi_lcd_pwm_disable(sel);
}

#define REGFLAG_DELAY             0XFE
#define REGFLAG_END_OF_TABLE      0xFD   /* END OF REGISTERS MARKER */

struct LCM_setting_table {
	u8 cmd;
	u32 count;
	u8 para_list[64];
};

/* add panel initialization below */


static struct LCM_setting_table lcm_initialization_setting[] = {

	/* Page 1 */
	{0xB0, 1, {0x01}},
	{0xC3, 1, {0x0F}},
	{0xC4, 1, {0x00}},
	{0xC5, 1, {0x00}},
	{0xC6, 1, {0x00}},
	{0xC7, 1, {0x00}},
	{0xC8, 1, {0x0D}},
	{0xC9, 1, {0x12}},
	{0xCA, 1, {0x11}},
	{0xCD, 1, {0x1D}},
	{0xCE, 1, {0x1B}},
	{0xCF, 1, {0x0B}},
	{0xD0, 1, {0x09}},
	{0xD1, 1, {0x07}},
	{0xD2, 1, {0x05}},
	{0xD3, 1, {0x01}},
	{0xD7, 1, {0x10}},
	{0xD8, 1, {0x00}},
	{0xD9, 1, {0x00}},
	{0xDA, 1, {0x00}},
	{0xDB, 1, {0x00}},
	{0xDC, 1, {0x0E}},
	{0xDD, 1, {0x12}},
	{0xDE, 1, {0x11}},
	{0xE1, 1, {0x1E}},
	{0xE2, 1, {0x1C}},
	{0xE3, 1, {0x0C}},
	{0xE4, 1, {0x0A}},
	{0xE5, 1, {0x08}},
	{0xE6, 1, {0x06}},
	{0xE7, 1, {0x02}},


	/* Page 3 */
	{0xB0, 1, {0x03}},
	{0xBE, 1, {0x03}},
	{0xCC, 1, {0x44}},
	{0xC8, 1, {0x07}},
	{0xC9, 1, {0x05}},
	{0xCA, 1, {0x42}},
	{0xCD, 1, {0x3E}},
	{0xCF, 1, {0x60}},
	{0xD2, 1, {0x04}},
	{0xD3, 1, {0x04}},
	{0xD4, 1, {0x01}},
	{0xD5, 1, {0x00}},
	{0xD6, 1, {0x03}},
	{0xD7, 1, {0x04}},
	{0xD9, 1, {0x01}},
	{0xDB, 1, {0x01}},
	{0xE4, 1, {0xF0}},
	{0xE5, 1, {0x0A}},

	/* Page 0 */
	{0xB0, 1, {0x00}},
	{0xBD, 1, {0x63}},
	{0xC2, 1, {0x08}},
	{0xC4, 1, {0x10}},
/* 	{0xB2, 1, {0x41}}, //Self test mode on */

	/* Page 2 */
	{0xB0, 1, {0x02}},
	{0xC0, 1, {0x00}},
	{0xC1, 1, {0x0A}},
	{0xC2, 1, {0x20}},
	{0xC3, 1, {0x24}},
	{0xC4, 1, {0x23}},
	{0xC5, 1, {0x29}},
	{0xC6, 1, {0x23}},
	{0xC7, 1, {0x1C}},
	{0xC8, 1, {0x19}},
	{0xC9, 1, {0x17}},
	{0xCA, 1, {0x17}},
	{0xCB, 1, {0x18}},
	{0xCC, 1, {0x1A}},
	{0xCD, 1, {0x1E}},
	{0xCE, 1, {0x20}},
	{0xCF, 1, {0x23}},
	{0xD0, 1, {0x07}},
	{0xD1, 1, {0x00}},
	{0xD2, 1, {0x00}},
	{0xD3, 1, {0x0A}},
	{0xD4, 1, {0x13}},
	{0xD5, 1, {0x1C}},
	{0xD6, 1, {0x1A}},
	{0xD7, 1, {0x13}},
	{0xD8, 1, {0x17}},
	{0xD9, 1, {0x1C}},
	{0xDA, 1, {0x19}},
	{0xDB, 1, {0x17}},
	{0xDC, 1, {0x17}},
	{0xDD, 1, {0x18}},
	{0xDE, 1, {0x1A}},
	{0xDF, 1, {0x1E}},
	{0xE0, 1, {0x20}},
	{0xE1, 1, {0x23}},
	{0xE2, 1, {0x07}},

	{0x11, 1, {0x00}},  /* SLPOUT */
	{REGFLAG_DELAY, REGFLAG_DELAY, {120}},

	/* DISP ON */
	{0x29, 1, {0x00}},  /* DSPON */
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},

	{REGFLAG_END_OF_TABLE, REGFLAG_END_OF_TABLE, {}}

};

static void LCD_panel_init(u32 sel)
{
	__u32 i;
	char model_name[25];
	DE_INFO("===test\n\n\n\n\n");
	disp_sys_script_get_item("lcd0", "lcd_model_name", (int *)model_name, 25);
	sunxi_lcd_dsi_clk_enable(sel);
	sunxi_lcd_delay_ms(20);

	for (i = 0;; i++) {
		if (lcm_initialization_setting[i].count == REGFLAG_END_OF_TABLE)
			break;
		else if (lcm_initialization_setting[i].count == REGFLAG_DELAY)
			sunxi_lcd_delay_ms(lcm_initialization_setting[i].para_list[0]);
#ifdef SUPPORT_DSI
			else
				dsi_dcs_wr(sel, lcm_initialization_setting[i].cmd, lcm_initialization_setting[i].para_list, lcm_initialization_setting[i].count);
#endif
		/* break; */
	}

	return;
}

static void LCD_panel_exit(u32 sel)
{
	sunxi_lcd_dsi_dcs_write_0para(sel, DSI_DCS_SET_DISPLAY_OFF);
	sunxi_lcd_delay_ms(20);
	sunxi_lcd_dsi_dcs_write_0para(sel, DSI_DCS_ENTER_SLEEP_MODE);
	sunxi_lcd_delay_ms(80);

	return ;
}

/* sel: 0:lcd0; 1:lcd1 */
static s32 LCD_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

struct __lcd_panel K101_MM2QA01_A_mipi_panel = {
	/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
	.name = "K101_MM2QA01_A",
	.func = {
		.cfg_panel_info = LCD_cfg_panel_info,
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		.lcd_user_defined_func = LCD_user_defined_func,
		/* .set_bright = LCD_set_bright, */
	},
};
