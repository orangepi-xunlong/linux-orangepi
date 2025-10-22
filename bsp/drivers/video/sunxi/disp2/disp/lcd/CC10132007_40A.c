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

/* #include "JD9365DA_SQ101A_B4EI313_39R501.h" */
/* #include <mach/sys_config.h> */
#include "panels.h"

/*
&lcd0 {
	lcd_used            = <1>;
	status              = "okay";
	lcd_driver_name     = "JD9365DA_SQ101A_B4EI313_39R501";
	lcd_backlight       = <50>;
	lcd_if              = <4>;

	lcd_x               = <800>;
	lcd_y               = <1280>;
	lcd_width           = <135>;
	lcd_height          = <216>;
	lcd_dclk_freq       = <75>;

	lcd_pwm_used        = <1>;
	lcd_pwm_ch          = <0>;
	lcd_pwm_freq        = <50000>;
	lcd_pwm_pol         = <1>;
	lcd_pwm_max_limit   = <255>;

	lcd_hbp             = <88>;
	lcd_ht              = <960>;
	lcd_hspw            = <4>;
	lcd_vbp             = <12>;
	lcd_vt              = <1300>;
	lcd_vspw            = <4>;

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

	lcd_pin_power = "dcdc1";
	lcd_pin_power1 = "eldo3";

	lcd_power = "eldo3";

	lcd_power1 = "dcdc1";
	lcd_power2 = "dc1sw";

	lcd_gpio_1 = <&pio PD 22 1 0 3 1>;

	pinctrl-0 = <&dsi4lane_pins_a>;
	pinctrl-1 = <&dsi4lane_pins_b>;

	lcd_bl_en = <&pio PB 8 1 0 3 1>;
	lcd_bl_0_percent	= <15>;
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
#define panel_reset(val) sunxi_lcd_gpio_set_value(sel, 2, val)
#define power_gpio_en(val) sunxi_lcd_gpio_set_value(sel, 1, val)
#define power_en(val) sunxi_lcd_gpio_set_value(sel, 0, val)

static void LCD_cfg_panel_info(struct panel_extend_para *info)
{
	u32 i = 0, j = 0;
	u32 items;
	u8 lcd_gamma_tbl[][2] = {
		/* {input value, corrected value} */
		{ 0, 0 },     { 15, 15 },   { 30, 30 },   { 45, 45 },
		{ 60, 60 },   { 75, 75 },   { 90, 90 },   { 105, 105 },
		{ 120, 120 }, { 135, 135 }, { 150, 150 }, { 165, 165 },
		{ 180, 180 }, { 195, 195 }, { 210, 210 }, { 225, 225 },
		{ 240, 240 }, { 255, 255 },
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

			value = lcd_gamma_tbl[i][1] +
				((lcd_gamma_tbl[i + 1][1] -
				  lcd_gamma_tbl[i][1]) *
				 j) / num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] =
				(value << 16) + (value << 8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items - 1][1] << 16) +
				   (lcd_gamma_tbl[items - 1][1] << 8) +
				   lcd_gamma_tbl[items - 1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));
}

static s32 LCD_open_flow(u32 sel)
{
	LCD_OPEN_FUNC(sel, LCD_power_on, 10); /* open lcd power, and delay 50ms */
	LCD_OPEN_FUNC(sel, LCD_panel_init,
		      10); /* open lcd power, than delay 200ms */
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable,
		      50); /* open lcd controller, and delay 100ms */
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0); /* open lcd backlight, and delay 0ms */

	return 0;
}

static s32 LCD_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, LCD_bl_close,
		       0); /* close lcd backlight, and delay 0ms */
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable,
		       0); /* close lcd controller, and delay 0ms */
	LCD_CLOSE_FUNC(sel, LCD_panel_exit,
		       20); /* open lcd power, than delay 200ms */
	LCD_CLOSE_FUNC(sel, LCD_power_off,
		       50); /* close lcd power, and delay 500ms */

	return 0;
}

static void LCD_power_on(u32 sel)
{
	panel_reset(0);
	sunxi_lcd_power_enable(sel, 0); /* config lcd_power pin to open lcd power */
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_enable(sel,
			       1); /* config lcd_power pin to open lcd power1 */
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_enable(sel,
			       2); /* config lcd_power pin to open lcd power2 */
	sunxi_lcd_delay_ms(5);
	power_en(1);
	sunxi_lcd_delay_ms(20);
	power_gpio_en(1);

	/* panel_reset(1); */
	sunxi_lcd_delay_ms(40);
	panel_reset(1);
	sunxi_lcd_delay_ms(10);
	panel_reset(0);
	sunxi_lcd_delay_ms(5);
	panel_reset(1);
	sunxi_lcd_delay_ms(60);
	/* sunxi_lcd_delay_ms(5); */

	sunxi_lcd_pin_cfg(sel, 1);
}

static void LCD_power_off(u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 0);
	power_gpio_en(0);
	power_en(0);
	sunxi_lcd_delay_ms(20);
	panel_reset(0);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel,
				2); /* config lcd_power pin to close lcd power2 */
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel,
				1); /* config lcd_power pin to close lcd power1 */
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel,
				0); /* config lcd_power pin to close lcd power */
}

static void LCD_bl_open(u32 sel)
{
	sunxi_lcd_pwm_enable(sel);
	sunxi_lcd_delay_ms(50);
	sunxi_lcd_backlight_enable(
		sel); /* config lcd_bl_en pin to open lcd backlight */
}

static void LCD_bl_close(u32 sel)
{
	sunxi_lcd_backlight_disable(
		sel); /* config lcd_bl_en pin to close lcd backlight */
	sunxi_lcd_delay_ms(20);
	sunxi_lcd_pwm_disable(sel);
}

static void LCD_panel_init(u32 sel)
{
	sunxi_lcd_dsi_clk_enable(sel);
	/* JD9365 initial code */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE0, 0x00);

	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE1, 0x93);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE2, 0x65);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE3, 0xF8);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x80, 0x03);

	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE0, 0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x00, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x01, 0x25);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x03, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x04, 0x30);

	sunxi_lcd_dsi_dcs_write_1para(sel, 0x0C, 0x74);

	sunxi_lcd_dsi_dcs_write_1para(sel, 0x17, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x18, 0xC7);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x19, 0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x1A, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x1B, 0xC7);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x1C, 0x01);

	sunxi_lcd_dsi_dcs_write_1para(sel, 0x24, 0xFE);

	sunxi_lcd_dsi_dcs_write_1para(sel, 0x37, 0x19);

	sunxi_lcd_dsi_dcs_write_1para(sel, 0x35, 0x28);

	sunxi_lcd_dsi_dcs_write_1para(sel, 0x38, 0x05);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x39, 0x08);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x3A, 0x12);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x3C, 0x7E);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x3D, 0xFF);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x3E, 0xFF);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x3F, 0x7F);

	sunxi_lcd_dsi_dcs_write_1para(sel, 0x40, 0x06);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x41, 0xA0);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x43, 0x1E);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x44, 0x0B);

	sunxi_lcd_dsi_dcs_write_1para(sel, 0x55, 0x02);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x57, 0x6A);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x59, 0x0A);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5A, 0x2E);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5B, 0x1A);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5C, 0x15);

	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5D, 0x7F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5E, 0x61);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5F, 0x50);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x60, 0x43);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x61, 0x3F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x62, 0x32);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x63, 0x35);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x64, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x65, 0x38);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x66, 0x36);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x67, 0x36);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x68, 0x54);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x69, 0x42);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6A, 0x48);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6B, 0x39);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6C, 0x34);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6D, 0x26);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6E, 0x14);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6F, 0x02);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x70, 0x7F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x71, 0x61);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x72, 0x50);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x73, 0x43);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x74, 0x3F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x75, 0x32);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x76, 0x35);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x77, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x78, 0x38);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x79, 0x36);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x7A, 0x36);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x7B, 0x54);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x7C, 0x42);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x7D, 0x48);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x7E, 0x39);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x7F, 0x34);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x80, 0x26);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x81, 0x14);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x82, 0x02);

	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE0, 0x02);

	sunxi_lcd_dsi_dcs_write_1para(sel, 0x00, 0x52);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x01, 0x5F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x02, 0x5F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x03, 0x50);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x04, 0x77);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x05, 0x57);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x06, 0x5F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x07, 0x4E);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x08, 0x4C);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x09, 0x5F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x0A, 0x4A);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x0B, 0x48);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x0C, 0x5F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x0D, 0x46);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x0E, 0x44);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x0F, 0x40);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x10, 0x5F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x11, 0x5F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x12, 0x5F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x13, 0x5F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x14, 0x5F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x15, 0x5F);

	sunxi_lcd_dsi_dcs_write_1para(sel, 0x16, 0x53);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x17, 0x5F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x18, 0x5F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x19, 0x51);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x1A, 0x77);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x1B, 0x57);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x1C, 0x5F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x1D, 0x4F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x1E, 0x4D);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x1F, 0x5F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x20, 0x4B);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x21, 0x49);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x22, 0x5F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x23, 0x47);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x24, 0x45);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x25, 0x41);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x26, 0x5F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x27, 0x5F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x28, 0x5F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x29, 0x5F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x2A, 0x5F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x2B, 0x5F);

	sunxi_lcd_dsi_dcs_write_1para(sel, 0x2C, 0x13);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x2D, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x2E, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x2F, 0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x30, 0x17);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x31, 0x17);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x32, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x33, 0x0D);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x34, 0x0F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x35, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x36, 0x05);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x37, 0x07);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x38, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x39, 0x09);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x3A, 0x0B);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x3B, 0x11);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x3C, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x3D, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x3E, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x3F, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x40, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x41, 0x1F);

	sunxi_lcd_dsi_dcs_write_1para(sel, 0x42, 0x12);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x43, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x44, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x45, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x46, 0x17);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x47, 0x17);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x48, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x49, 0x0C);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x4A, 0x0E);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x4B, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x4C, 0x04);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x4D, 0x06);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x4E, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x4F, 0x08);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x50, 0x0A);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x51, 0x10);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x52, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x53, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x54, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x55, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x56, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x57, 0x1F);

	sunxi_lcd_dsi_dcs_write_1para(sel, 0x58, 0x40);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5B, 0x10);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5C, 0x06);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5D, 0x40);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5E, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5F, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x60, 0x40);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x61, 0x03);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x62, 0x04);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x63, 0x6C);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x64, 0x6C);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x65, 0x75);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x66, 0x08);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x67, 0xB4);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x68, 0x08);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x69, 0x6C);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6A, 0x6C);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6B, 0x0C);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6D, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6E, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6F, 0x88);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x75, 0xBB);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x76, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x77, 0x05);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x78, 0x2A);

	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE0, 0x04);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x37, 0x58);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x00, 0x0E);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x02, 0xB3);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x09, 0x61);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x0E, 0x48);

	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE0, 0x00);

	/* SLP OUT */
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x11); /* SLPOUT */
	/* DISP ON */
	sunxi_lcd_delay_ms(50);
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x29); /* DSPON */
	sunxi_lcd_delay_ms(5);

	/* --- TE---- */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x35, 0x00);
	sunxi_lcd_delay_ms(50);

	return;
}

static void LCD_panel_exit(u32 sel)
{
	sunxi_lcd_dsi_dcs_write_0para(sel, DSI_DCS_SET_DISPLAY_OFF);
	sunxi_lcd_delay_ms(20);
	sunxi_lcd_dsi_dcs_write_0para(sel, DSI_DCS_ENTER_SLEEP_MODE);
	sunxi_lcd_delay_ms(80);

	return;
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
struct __lcd_panel CC10132007_40A_panel = {
	/* panel driver name, must mach the name of
	 * lcd_drv_name in sys_config.fex
	 */
	.name = "CC10132007_40A",
	.func = {
		.cfg_panel_info = LCD_cfg_panel_info,
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		.lcd_user_defined_func = LCD_user_defined_func,
		/* .set_bright = LCD_set_bright, */
	},
};
