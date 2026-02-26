/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "FX070_DHM18BOEL2_1024X600.h"
/* #include <mach/sys_config.h> */
#include "panels.h"

/*
&lcd0 {
	lcd_used            = <1>;
	lcd_driver_name     = "FX070_DHM18BOEL2_1024X600";
	lcd_backlight       = <100>;
	lcd_if              = <4>;

	lcd_x               = <1024>;
	lcd_y               = <600>;
	lcd_width           = <155>;
	lcd_height          = <86>;
	lcd_dclk_freq       = <52>;

	lcd_pwm_used        = <1>;
	lcd_pwm_ch          = <0>;
	lcd_pwm_freq        = <50000>;
	lcd_pwm_pol         = <0>;
	lcd_pwm_max_limit   = <255>;

	lcd_hbp             = <160>;
	lcd_ht              = <1344>;
	lcd_hspw            = <24>;
	lcd_vbp             = <23>;

	lcd_vt              = <635>;
	lcd_vspw            = <2>;

	lcd_frm             = <0>;
	lcd_gamma_en        = <0>;
	lcd_bright_curve_en = <0>;
	lcd_cmap_en         = <0>;

	deu_mode            = <0>;
	lcdgamma4iep        = <22>;
	smart_color         = <90>;

	lcd_dsi_if          = <0>;
	lcd_dsi_lane        = <4>;
	lcd_dsi_format      = <0>;
	lcd_dsi_te          = <0>;
	lcd_dsi_eotp        = <0>;

	lcd_power = "cldo3";
	lcd_power1 = "cldo2";
	lcd_power2 = "cldo4";

	lcd_gpio_1 = <&pio PD 22 GPIO_ACTIVE_HIGH>;

	pinctrl-0 = <&dsi4lane_pins_a>;
	pinctrl-1 = <&dsi4lane_pins_b>;

	lcd_bl_en = <&pio PB 8 GPIO_ACTIVE_HIGH>;
	lcd_bl_0_percent        = <50>;
	lcd_bl_100_percent  = <100>;
};
*/

extern s32 bsp_disp_get_panel_info(u32 screen_id, struct disp_panel_para *info);
static void LCD_power_on(u32 sel);
static void LCD_power_off(u32 sel);
static void LCD_bl_open(u32 sel);
static void LCD_bl_close(u32 sel);

static void LCD_panel_init(u32 sel);
static void LCD_panel_exit(u32 sel);

/* static u8 const mipi_dcs_pixel_format[4] = {0x77,0x66,0x66,0x55}; */
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

			value = lcd_gamma_tbl[i][1] + ((lcd_gamma_tbl[i + 1][1] - lcd_gamma_tbl[i][1]) * j) / num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] = (value << 16) + (value << 8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items - 1][1] << 16) + (lcd_gamma_tbl[items - 1][1] << 8) + lcd_gamma_tbl[items - 1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

static s32 LCD_open_flow(u32 sel)
{
	LCD_OPEN_FUNC(sel, LCD_power_on, 100);   /* open lcd power, and delay 50ms */
	LCD_OPEN_FUNC(sel, LCD_panel_init, 200);   /* open lcd power, than delay 200ms */
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 50);     /* open lcd controller, and delay 100ms */
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);     /* open lcd backlight, and delay 0ms */
	return 0;
}

static s32 LCD_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 200);       /* close lcd backlight, and delay 0ms */
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 20);         /* close lcd controller, and delay 0ms */
	LCD_CLOSE_FUNC(sel, LCD_panel_exit,	10);   /* open lcd power, than delay 200ms */
	LCD_CLOSE_FUNC(sel, LCD_power_off, 500);   /* close lcd power, and delay 500ms */

	return 0;
}

static void LCD_power_on(u32 sel)
{
	panel_reset(0);
	sunxi_lcd_power_enable(sel, 0);/* config lcd_power pin to open lcd power */
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_enable(sel, 1);/* config lcd_power pin to open lcd power1 */
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_enable(sel, 2);/* config lcd_power pin to open lcd power2 */
	sunxi_lcd_delay_ms(5);
	power_en(1);
	sunxi_lcd_delay_ms(20);

	/* panel_reset(1); */
	//sunxi_lcd_delay_ms(40);
	panel_reset(1);
	/* sunxi_lcd_delay_ms(10); */
	/* panel_reset(0); */
	/* sunxi_lcd_delay_ms(5); */
	/* panel_reset(1); */
	sunxi_lcd_delay_ms(5);
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
	sunxi_lcd_power_disable(sel, 2);/* config lcd_power pin to close lcd power2 */
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 1);/* config lcd_power pin to close lcd power1 */
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 0);/* config lcd_power pin to close lcd power */
}

static void LCD_bl_open(u32 sel)
{
	sunxi_lcd_pwm_enable(sel);
	sunxi_lcd_delay_ms(50);
	sunxi_lcd_backlight_enable(sel);/* config lcd_bl_en pin to open lcd backlight */
}

static void LCD_bl_close(u32 sel)
{
	sunxi_lcd_backlight_disable(sel);/* config lcd_bl_en pin to close lcd backlight */
	sunxi_lcd_delay_ms(20);
	sunxi_lcd_pwm_disable(sel);
}

#define REGFLAG_DELAY             						0XFE
#define REGFLAG_END_OF_TABLE      						0xFD   /* END OF REGISTERS MARKER */

struct LCM_setting_table {
	u8 cmd;
	u32 count;
	u8 para_list[64];
};

/* add panel initialization below */

static struct LCM_setting_table lcm_initialization_setting[] = {
	//page0
	{0x30, 1, {0x00}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0xf7, 4, {0x49, 0x61, 0x02, 0x00}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},

	//page1
	{0x30,  1, {0x01}},
/*
	{0x1F,  1, {0xdd08}},
	{0x05,  1, {0x01}},
	{0x06,  1, {0x40}},//bist 1214
*/
	{0x04, 1, {0x0C}},  //add
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},

	{0x0B, 1, {0x10}}, //10--4lane  12--3lane 1214
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x23, 1, {0x38}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x28, 1, {0x18}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x29, 1, {0x29}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x2A, 1, {0x01}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x2B, 1, {0x29}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x2C, 1, {0x01}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},

	//page2
	{0x30, 1, {0x02}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x00, 1, {0x05}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x01, 1, {0x22}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x02, 1, {0x08}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x03, 1, {0x12}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x04, 1, {0x16}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x05, 1, {0x64}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x06, 1, {0x00}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x07, 1, {0x00}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x08, 1, {0x78}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x09, 1, {0x00}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x0A, 1, {0x04}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x0B, 11, {0x16, 0x17, 0x0B, 0x0D, 0x0D, 0x0D, 0x11, 0x10, 0x07, 0x07, 0x09}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x0C, 11, {0x09, 0x1E, 0x1E, 0x1C, 0x1C, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x0D, 11, {0x0A, 0x05, 0x0B, 0x0D, 0x0D, 0x0D, 0x11, 0x10, 0x06, 0x06, 0x08}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x0E, 11, {0x08, 0x1F, 0x1F, 0x1D, 0x1D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x0F, 11, {0x0A, 0x05, 0x0D, 0x0B, 0x0D, 0x0D, 0x11, 0x10, 0x1D, 0x1D, 0x1F}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x10, 11, {0x1F, 0x08, 0x08, 0x06, 0x06, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x11, 11, {0x16, 0x17, 0x0D, 0x0B, 0x0D, 0x0D, 0x11, 0x10, 0x1C, 0x1C, 0x1E}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x12, 11, {0x1E, 0x09, 0x09, 0x07, 0x07, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x13, 4, {0x00, 0x00, 0x00, 0x00}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x14, 4, {0x00, 0x00, 0x41, 0x41}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x15, 4, {0x00, 0x00, 0x00, 0x00}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x17, 1, {0x00}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x18, 1, {0x85}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x19, 2, {0x06, 0x09}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x1a, 2, {0x05, 0x08}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x1b, 2, {0x0A, 0x04}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x26, 1, {0x00}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x27, 1, {0x00}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},

	//page6
	{0x30, 1, {0x06}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x12, 14, {0x3F, 0x27, 0x28, 0x35, 0x1B, 0x17, 0x16, 0x13, 0x10, 0x01, 0x23, 0x1B, 0x10, 0x30}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x13, 14, {0x3F, 0x27, 0x28, 0x35, 0x1D, 0x18, 0x16, 0x13, 0x10, 0x02, 0x24, 0x1B, 0x10, 0x30}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},

	//pageA
	{0x30, 1, {0x0a}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x02, 1, {0x4F}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x0B, 1, {0x40}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},

	//pageD
	{0x30, 1, {0x0d}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x37, 1, {0x58}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x10, 1, {0x05}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x11, 1, {0x0c}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x12, 1, {0x05}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x13, 1, {0x0c}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},

	//page0
	{0x30, 1, {0x00}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {120}},
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, REGFLAG_DELAY, {20}},
	{REGFLAG_END_OF_TABLE, REGFLAG_END_OF_TABLE, {}}
};

static void LCD_panel_init(u32 sel)
{
	__u32 i;
	char model_name[25];
	disp_sys_script_get_item("lcd0", "lcd_model_name", (int *)model_name, 25);
	sunxi_lcd_dsi_clk_enable(sel);
	sunxi_lcd_delay_ms(20);
	sunxi_lcd_dsi_dcs_write_0para(sel, DSI_DCS_SOFT_RESET);
	sunxi_lcd_delay_ms(10);

	for (i = 0; ; i++) {
			if (lcm_initialization_setting[i].count == REGFLAG_END_OF_TABLE)
				break;
			else if (lcm_initialization_setting[i].count == REGFLAG_DELAY)
				sunxi_lcd_delay_ms(lcm_initialization_setting[i].para_list[0]);
#ifdef SUPPORT_DSI
			else
				dsi_dcs_wr(sel, lcm_initialization_setting[i].cmd, lcm_initialization_setting[i].para_list, lcm_initialization_setting[i].count);
#endif
	}
		/* break; */

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

/* sel: 0:lcd0; 1:lcd1 */
/* static s32 LCD_set_bright(u32 sel, u32 bright)
{
	sunxi_lcd_dsi_dcs_write_1para(sel,0x51,bright);
	return 0;
} */

struct __lcd_panel FX070_DHM18BOEL2_1024X600_mipi_panel = {
	/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
	.name = "FX070_DHM18BOEL2_1024X600",
	.func = {
		.cfg_panel_info = LCD_cfg_panel_info,
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		.lcd_user_defined_func = LCD_user_defined_func,
		/* .set_bright = LCD_set_bright, */
	},
};
