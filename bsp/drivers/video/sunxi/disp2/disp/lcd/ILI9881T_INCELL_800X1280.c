/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * drivers/video/sunxi/disp2/disp/lcd/ILI9881T_INCELL_800X1280.c
 *
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


/*
&lcd1 {
	lcd_used            = <1>;
	status              = "okay";
	lcd_driver_name     = "ILI9881T_INCELL_800X1280";
	lcd_backlight       = <50>;
	lcd_if              = <4>;

	lcd_x               = <800>;
	lcd_y               = <1280>;
	lcd_width           = <136>;
	lcd_height          = <217>;

	lcd_pwm_used        = <1>;
	lcd_pwm_ch          = <0>;
	lcd_pwm_freq        = <50000>;
	lcd_pwm_pol         = <0>;
	lcd_pwm_max_limit   = <255>;

	lcd_dclk_freq       = <79>;
	lcd_hbp             = <16>;
	lcd_ht              = <840>;
	lcd_hspw            = <8>;
	lcd_vbp             = <66>;
	lcd_vt              = <1566>;
	lcd_vspw            = <10>;


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

	lcd_power0 = "cldo1";
	lcd_power1 = "cldo4";

	lcd_gpio_0 = <&pio PD 22 GPIO_ACTIVE_HIGH>; //lcd-reset
	lcd_gpio_1 = <&pio PH 10 GPIO_ACTIVE_HIGH>; //tp-reset

	pinctrl-0 = <&dsi0_4lane_pins_a>;
	pinctrl-1 = <&dsi0_4lane_pins_b>;

	lcd_bl_en = <&pio PH 18 GPIO_ACTIVE_HIGH>;
	lcd_bl_0_percent	= <5>;
	#if defined(SKTL_LCD_10)
	lcd_bl_100_percent  = <75>;
	#else
	lcd_bl_100_percent  = <90>;
	#endif
};
*/ 
#include "panels.h"
#include "fb_top.h"
#include "ILI9881T_INCELL_800X1280.h"

// extern s32 bsp_disp_get_panel_info(u32 screen_id, disp_panel_para *info);
static void LCD_power_on(u32 sel);
static void LCD_power_off(u32 sel);
static void LCD_bl_open(u32 sel);
static void LCD_bl_close(u32 sel);

static void LCD_panel_init(u32 sel);
static void LCD_panel_exit(u32 sel);

// static u8 const mipi_dcs_pixel_format[4] = {0x77,0x66,0x66,0x55};
#define panel_reset(sel, val) sunxi_lcd_gpio_set_value(sel, 0, val)
#define panel_tp_reset(sel, val) sunxi_lcd_gpio_set_value(sel, 1, val)

static void LCD_cfg_panel_info(struct panel_extend_para *info)
{
	u32 i = 0, j = 0;
	u32 items;
	u8 lcd_gamma_tbl[][2] = {
		//{input value, corrected value}
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

			value = lcd_gamma_tbl[i][1] +
					((lcd_gamma_tbl[i + 1][1] -
					  lcd_gamma_tbl[i][1]) *
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

static s32 LCD_open_flow(u32 sel)
{
	LCD_OPEN_FUNC(sel, LCD_power_on, 20); // open lcd power, and delay 50ms
	LCD_OPEN_FUNC(sel, LCD_panel_init,
				  20); // open lcd power, than delay 200ms
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable,
				  40);					// open lcd controller, and delay 100ms
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0); // open lcd backlight, and delay 0ms
	return 0;
}

static s32 LCD_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, LCD_bl_close,
				   0); // close lcd backlight, and delay 0ms
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable,
				   0); // close lcd controller, and delay 0ms
	LCD_CLOSE_FUNC(sel, LCD_panel_exit,
				   20); // open lcd power, than delay 200ms
	LCD_CLOSE_FUNC(sel, LCD_power_off,
				   50); // close lcd power, and delay 500ms

	return 0;
}

static void LCD_power_on(u32 sel)
{
	panel_reset(sel, 0);
	panel_tp_reset(sel, 0);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_enable(sel, 0);
	sunxi_lcd_power_enable(sel, 1);
	sunxi_lcd_pin_cfg(sel, 1);
	sunxi_lcd_delay_ms(20);
	panel_reset(sel, 1);
	sunxi_lcd_delay_ms(2);
	panel_tp_reset(sel, 1);
	sunxi_lcd_delay_ms(10);

	screen_driver_work.blank = 10;
	schedule_work(&screen_driver_work.tp_work);
	sunxi_lcd_delay_ms(10);
	sunxi_lcd_pin_cfg(sel, 1);
}

static void LCD_power_off(u32 sel)
{
	screen_driver_work.blank = 9;
	schedule_work(&screen_driver_work.tp_work);

	sunxi_lcd_pin_cfg(sel, 0);
	sunxi_lcd_delay_ms(20);
	panel_reset(sel, 0);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel,
							1); // config lcd_power pin to close lcd power1
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel,
							0); // config lcd_power pin to close lcd power
}

static void LCD_bl_open(u32 sel)
{
	sunxi_lcd_backlight_enable(sel);
	sunxi_lcd_pwm_enable(sel);
}

static void LCD_bl_close(u32 sel)
{
	sunxi_lcd_pwm_disable(sel);
	sunxi_lcd_backlight_disable(sel);
}

#define REGFLAG_DELAY 0XFE
#define REGFLAG_END_OF_TABLE 0xFD // END OF REGISTERS MARKER

struct LCM_setting_table
{
	u8 cmd;
	u32 count;
	u8 para_list[64];
};

/*add panel initialization below*/
static struct LCM_setting_table lcm_initialization_setting[] = {
	// GIP Setting
	{0xFF, 3, {0x98, 0x81, 0x1}},
	{0x00, 1, {0x48}}, // STVA RISE 08
	{0x01, 1, {0x13}}, // STVA 4H
	{0x02, 1, {0x00}}, //
	{0x03, 1, {0x00}},
	{0x04, 1, {0x13}},
	{0x05, 1, {0x13}},
	{0x06, 1, {0x00}},
	{0x07, 1, {0x00}},
	{0x08, 1, {0x84}}, // 86 CLKA RISE   20210304
	{0x09, 1, {0x0F}}, // 0D CLKA FALL   20210304
	{0x0A, 1, {0x71}}, // 73 CLK=2H      20210304
	{0x0B, 1, {0x00}},
	{0x0C, 1, {0x0C}}, // 28 CLK Duty=24% 20220323
	{0x0D, 1, {0x0C}}, // 28 CLK Duty=24% 20220323
	{0x0E, 1, {0x00}},
	{0x0F, 1, {0x00}},
	// REGISTER,10,01,08

	// FW GOUTR
	{0x31, 1, {0x00}}, // VGLO
	{0x32, 1, {0x00}}, // VGLO
	{0x33, 1, {0x08}}, // STV1
	{0x34, 1, {0x08}}, // STV1
	{0x35, 1, {0x0C}}, // STV2
	{0x36, 1, {0x0C}}, // STV2
	{0x37, 1, {0x00}}, // VSD
	{0x38, 1, {0x00}}, // VSD
	{0x39, 1, {0x23}}, // GCL
	{0x3A, 1, {0x23}}, // GCL
	{0x3B, 1, {0x01}}, // VDS
	{0x3C, 1, {0x01}}, // VDS
	{0x3D, 1, {0x22}}, // GCH
	{0x3E, 1, {0x22}}, // GCH
	{0x3F, 1, {0x12}}, // CLK7
	{0x40, 1, {0x12}}, // CLK7
	{0x41, 1, {0x10}}, // CLK5
	{0x42, 1, {0x10}}, // CLK5
	{0x43, 1, {0x16}}, // CLK3
	{0x44, 1, {0x16}}, // CLK3
	{0x45, 1, {0x14}}, // CLK1
	{0x46, 1, {0x14}}, // CLK1
	// FW GOUTL
	{0x47, 1, {0x00}}, // VGLO
	{0x48, 1, {0x00}}, // VGLO
	{0x49, 1, {0x09}}, // STV3
	{0x4A, 1, {0x09}}, // STV3
	{0x4B, 1, {0x0D}}, // STV4
	{0x4C, 1, {0x0D}}, // STV4
	{0x4D, 1, {0x00}}, // VSD
	{0x4E, 1, {0x00}}, // VSD
	{0x4F, 1, {0x23}}, // GCL
	{0x50, 1, {0x23}}, // GCL
	{0x51, 1, {0x01}}, // VDS
	{0x52, 1, {0x01}}, // VDS
	{0x53, 1, {0x22}}, // GCH
	{0x54, 1, {0x22}}, // GCH
	{0x55, 1, {0x13}}, // CLK8
	{0x56, 1, {0x13}}, // CLK8
	{0x57, 1, {0x11}}, // CLK6
	{0x58, 1, {0x11}}, // CLK6
	{0x59, 1, {0x17}}, // CLK4
	{0x5A, 1, {0x17}}, // CLK4
	{0x5B, 1, {0x15}}, // CLK2
	{0x5C, 1, {0x15}}, // CLK2

	// BW GOUTR
	{0x61, 1, {0x01}}, // VGLO
	{0x62, 1, {0x01}}, // VGLO
	{0x63, 1, {0x09}}, // STV1
	{0x64, 1, {0x09}}, // STV1
	{0x65, 1, {0x0D}}, // STV2
	{0x66, 1, {0x0D}}, // STV2
	{0x67, 1, {0x00}}, // VSD
	{0x68, 1, {0x00}}, // VSD
	{0x69, 1, {0x23}}, // GCL
	{0x6A, 1, {0x23}}, // GCL
	{0x6B, 1, {0x01}}, // VDS
	{0x6C, 1, {0x01}}, // VDS
	{0x6D, 1, {0x22}}, // GCH
	{0x6E, 1, {0x22}}, // GCH
	{0x6F, 1, {0x15}}, // CLK7
	{0x70, 1, {0x15}}, // CLK7
	{0x71, 1, {0x17}}, // CLK5
	{0x72, 1, {0x17}}, // CLK5
	{0x73, 1, {0x11}}, // CLK3
	{0x74, 1, {0x11}}, // CLK3
	{0x75, 1, {0x13}}, // CLK1
	{0x76, 1, {0x13}}, // CLK1
	// BW GOUTL
	{0x77, 1, {0x01}}, // VGLO
	{0x78, 1, {0x01}}, // VGLO
	{0x79, 1, {0x08}}, // STV3
	{0x7A, 1, {0x08}}, // STV3
	{0x7B, 1, {0x0C}}, // STV4
	{0x7C, 1, {0x0C}}, // STV4
	{0x7D, 1, {0x00}}, // VSD
	{0x7E, 1, {0x00}}, // VSD
	{0x7F, 1, {0x23}}, // GCL
	{0x80, 1, {0x23}}, // GCL
	{0x81, 1, {0x01}}, // VDS
	{0x82, 1, {0x01}}, // VDS
	{0x83, 1, {0x22}}, // GCH
	{0x84, 1, {0x22}}, // GCH
	{0x85, 1, {0x14}}, // CLK8
	{0x86, 1, {0x14}}, // CLK8
	{0x87, 1, {0x16}}, // CLK6
	{0x88, 1, {0x16}}, // CLK6
	{0x89, 1, {0x10}}, // CLK4
	{0x8A, 1, {0x10}}, // CLK4
	{0x8B, 1, {0x12}}, // CLK2
	{0x8C, 1, {0x12}}, // CLK2

	// PWR ON
	{0xD2, 1, {0x00}}, // 40 FWBW 20220512
	{0xD3, 1, {0x10}}, //        20220512
	{0xD5, 1, {0x55}}, // CLKA
	{0xD6, 1, {0x70}}, // 51 STCH 20220512

	// PWR OFF
	{0xD1, 1, {0x21}},
	{0xE6, 1, {0x43}},
	{0xE7, 1, {0x50}}, // 54 20220512
	{0xD8, 1, {0x05}}, // 0A 20220512
	{0xD9, 1, {0x05}}, // 22 20220512
	{0xDA, 1, {0x18}}, // 28 20220512
	{0xE2, 1, {0x45}}, // 56 20220512

	{0xDC, 1, {0x00}}, // 93 STVA STVB CLKA
	{0xDD, 1, {0x10}}, // 30 STCH1 STCH2

	// ABN
	{0xE0, 1, {0x7E}}, // STCH1 STCH2

	// STCH Setting
	{0x28, 1, {0x4C}},
	{0x29, 1, {0x98}},
	{0x2A, 1, {0x98}},
	{0x2B, 1, {0x4C}},
	{0xEE, 1, {0x14}},

	// Power Setting
	{0xFF, 3, {0x98, 0x81, 0x5}},

	{0x69, 1, {0x92}}, // 8D GVDDN  5.4V 20220323
	{0x6A, 1, {0x92}}, // 8D GVDDP -5.4V 20220323
	{0x6C, 1, {0xF1}}, // VGHO   19V
	{0x72, 1, {0xF7}}, // VGH    20V
	{0x78, 1, {0x8D}}, // VGLO   -11V
	{0x7E, 1, {0x7F}}, // VGL    -12V
	{0xA4, 1, {0x3F}}, // 0F VSP VSN LVD ON  20220225
	{0x60, 1, {0x63}}, // VGH 4X
	{0x61, 1, {0x43}}, // VGH 4X

	// Resolution
	{0xFF, 3, {0x98, 0x81, 0x6}},

	{0xD6, 1, {0x55}}, // TSHD1
	{0xDC, 1, {0x88}}, // TSHD2

	{0xD9, 1, {0x1F}}, // 4Lane
	{0x7B, 1, {0x08}},
	{0xC0, 1, {0x00}}, // NL_Y = 1280
	{0xC1, 1, {0x05}}, // NL_Y = 1280
	{0xCA, 1, {0x20}}, // NL_X = 800
	{0xCB, 1, {0x03}}, // NL_X = 800

	{0x48, 1, {0x39}}, // 1 bit esd check
	{0xC7, 1, {0x05}}, // 1 bit esd check

	// RTN. Internal VBP, Internal VFP
	{0xFF, 3, {0x98, 0x81, 0x02}},
	{0x06, 1, {0xA7}}, // Internal Line Time (RTN)
	{0x0A, 1, {0x1B}}, // Internal VFP[9:8]
	{0x0C, 1, {0x00}}, // Internal VBP[8]
	{0x0D, 1, {0x1C}}, // Internal VBP
	{0x0E, 1, {0xC8}}, // Internal VFP
	{0x39, 1, {0x01}}, // VBP[8],RTN[8],VFP[9:8]
	{0x3A, 1, {0x1C}}, // BIST VBP
	{0x3B, 1, {0xC8}}, // BIST VFP
	{0x3C, 1, {0xA8}}, // BIST RTN
	{0x3D, 1, {0x00}}, // BIST RTN[8] (Line time change)
	{0x3E, 1, {0xA7}}, // BIST RTN (Line time change)
	{0xF0, 1, {0x00}}, // BIST VFP[12:8] (Line time change)
	{0xF1, 1, {0xCA}}, // BIST VFP (Line time change)
	{0x40, 1, {0x4F}}, // 58 SDT=2.025us 20220323

	// OSC Auto Trim Setting
	{0xFF, 3, {0x98, 0x81, 0x0B}},
	{0x9A, 1, {0xC5}}, //
	{0x9B, 1, {0x42}},
	{0x9C, 1, {0x04}},
	{0x9D, 1, {0x04}},
	{0x9E, 1, {0x83}},
	{0x9F, 1, {0x83}},
	{0xAA, 1, {0x22}}, //
	{0xAB, 1, {0xE0}}, // AutoTrimType

	{0xFF, 3, {0x98, 0x81, 0x0E}},
	{0x11, 1, {0x4D}}, // TSVD_LONGV_RISE [4:0]
	{0x13, 1, {0x1A}}, // LV mode TSHD Rise position [4:0]
	{0x00, 1, {0xA0}}, // LV mode

	// Gamma Register
	{0xFF, 3, {0x98, 0x81, 0x08}}, // 20220323
	{0xE0, 27, {0x00, 0x00, 0x4C, 0x83, 0xC7, 0x54, 0xFD, 0x28, 0x5B, 0x84, 0xA5, 0xC3, 0xF7, 0x23, 0x4F, 0xAA, 0x7B, 0xB0, 0xD2, 0xFB, 0xFF, 0x1D, 0x48, 0x7C, 0xA8, 0x03, 0xDA}},
	{0xE1, 27, {0x00, 0x00, 0x4C, 0x83, 0xC7, 0x54, 0xFD, 0x28, 0x5B, 0x84, 0xA5, 0xC3, 0xF7, 0x23, 0x4F, 0xAA, 0x7B, 0xB0, 0xD2, 0xFB, 0xFF, 0x1D, 0x48, 0x7C, 0xA8, 0x03, 0xDA}},
	{0xFF, 3, {0x98, 0x81, 0x00}},
	{0x11, 0, {0x00}}, // Sleep Out
	{REGFLAG_DELAY, 120, {}},
	{0x29, 1, {0x00}},
	{REGFLAG_END_OF_TABLE, REGFLAG_END_OF_TABLE, {}}};


static void LCD_panel_init(u32 sel)
{
	__u32 i;

	sunxi_lcd_dsi_clk_enable(sel);
	sunxi_lcd_delay_ms(20);
	for (i = 0;; i++) {
		if (lcm_initialization_setting[i].cmd == REGFLAG_END_OF_TABLE)
			break;
		else if (lcm_initialization_setting[i].cmd == REGFLAG_DELAY)
			sunxi_lcd_delay_ms(
				lcm_initialization_setting[i].count);
		else {
			dsi_dcs_wr(sel, lcm_initialization_setting[i].cmd,
					   lcm_initialization_setting[i].para_list,
					   lcm_initialization_setting[i].count);
		}
	}

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

// sel: 0:lcd0; 1:lcd1
/*static s32 LCD_set_bright(u32 sel, u32 bright)
{
	sunxi_lcd_dsi_dcs_write_1para(sel,0x51,bright);
	return 0;
}*/

struct __lcd_panel ILI9881T_INCELL_800X1280_mipi_panel = {
	/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
	.name = "ILI9881T_INCELL_800X1280",
	.func = {
		.cfg_panel_info = LCD_cfg_panel_info,
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		//.lcd_user_defined_func = LCD_user_defined_func,
		//.set_bright = LCD_set_bright,
	},
};