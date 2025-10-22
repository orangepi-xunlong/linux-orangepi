/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
lcd_fb0: lcd_fb0@0 {
lcd_used = <1>;
lcd_driver_name = "nv3029s";
lcd_if = <1>;
lcd_dbi_if = <4>;
lcd_data_speed = <60>;
lcd_spi_bus_num = <1>;
lcd_x = <320>;
lcd_y = <240>;

//rgb565
//lcd_pixel_fmt = <10>;
//lcd_dbi_fmt = <2>;
//lcd_rgb_order = <0>;

//rgb666
lcd_pixel_fmt = <0>;
lcd_dbi_fmt = <3>;
lcd_rgb_order = <0>;

lcd_width = <60>;
lcd_height = <95>;
lcd_pwm_used = <1>;
lcd_pwm_ch = <4>;
lcd_pwm_freq = <5000>;
lcd_pwm_pol = <0>;
lcd_frm = <1>;
lcd_gamma_en = <1>;
fb_buffer_num = <2>;
lcd_backlight = <100>;
lcd_fps = <60>;
};
 *
 */

#include "nv3029s.h"

static void LCD_power_on(u32 sel);
static void LCD_power_off(u32 sel);
static void LCD_bl_open(u32 sel);
static void LCD_bl_close(u32 sel);
static void LCD_panel_init(u32 sel);
static void LCD_panel_exit(u32 sel);
#define RESET(s, v) sunxi_lcd_gpio_set_value(s, 0, v)
#define power_en(sel, val) sunxi_lcd_gpio_set_value(sel, 0, val)

static struct disp_panel_para info[LCD_FB_MAX];

static void address(unsigned int sel, int x, int y, int width, int height)
{
	sunxi_lcd_cmd_write(sel, 0x2B); /* Set row address */
	sunxi_lcd_para_write(sel, (y >> 8) & 0xff);
	sunxi_lcd_para_write(sel, y & 0xff);
	sunxi_lcd_para_write(sel, (height >> 8) & 0xff);
	sunxi_lcd_para_write(sel, height & 0xff);
	sunxi_lcd_cmd_write(sel, 0x2A); /* Set coloum address */
	sunxi_lcd_para_write(sel, (x >> 8) & 0xff);
	sunxi_lcd_para_write(sel, x & 0xff);
	sunxi_lcd_para_write(sel, (width >> 8) & 0xff);
	sunxi_lcd_para_write(sel, width & 0xff);
}

static void LCD_panel_init(unsigned int sel)
{
	unsigned int rotate;


	if (bsp_disp_get_panel_info(sel, &info[sel])) {
		lcd_fb_wrn("get panel info fail!\n");
		return;
	}

	sunxi_lcd_cmd_write(sel, 0xfd);
	sunxi_lcd_para_write(sel, 0x06);
	sunxi_lcd_para_write(sel, 0x07);

	sunxi_lcd_cmd_write(sel, 0x60);
	sunxi_lcd_para_write(sel, 0x14);
	sunxi_lcd_para_write(sel, 0x08);

	sunxi_lcd_cmd_write(sel, 0xB6);
	sunxi_lcd_para_write(sel, 0x22);
	sunxi_lcd_para_write(sel, 0x60);
	sunxi_lcd_para_write(sel, 0x27);

	sunxi_lcd_cmd_write(sel, 0xF0);
	sunxi_lcd_para_write(sel, 0x18);

	sunxi_lcd_cmd_write(sel, 0xB1);
	sunxi_lcd_para_write(sel, 0x61);
	sunxi_lcd_para_write(sel, 0x01);

	sunxi_lcd_cmd_write(sel, 0x62);
	sunxi_lcd_para_write(sel, 0x87);

	sunxi_lcd_cmd_write(sel, 0x63);
	sunxi_lcd_para_write(sel, 0xa9);

	sunxi_lcd_cmd_write(sel, 0x64);
	sunxi_lcd_para_write(sel, 0x19);
	sunxi_lcd_para_write(sel, 0x12);

	sunxi_lcd_cmd_write(sel, 0x65);
	sunxi_lcd_para_write(sel, 0x7b);
	sunxi_lcd_para_write(sel, 0xc0);

	sunxi_lcd_cmd_write(sel, 0x67);
	sunxi_lcd_para_write(sel, 0x33);

	sunxi_lcd_cmd_write(sel, 0x68);
	sunxi_lcd_para_write(sel, 0x00);
	sunxi_lcd_para_write(sel, 0x12);
	sunxi_lcd_para_write(sel, 0x10);
	sunxi_lcd_para_write(sel, 0x14);

	sunxi_lcd_cmd_write(sel, 0xf3);
	sunxi_lcd_para_write(sel, 0x06); /* thrb[5:0] */
	sunxi_lcd_para_write(sel, 0x04); /* thg[5:0] */

	sunxi_lcd_cmd_write(sel, 0xf6);
	sunxi_lcd_para_write(sel, 0x09);
	sunxi_lcd_para_write(sel, 0x10);
	sunxi_lcd_para_write(sel, 0x80); /* 80--spi_2wire_mode,00 */

	sunxi_lcd_cmd_write(sel, 0xf7);
	sunxi_lcd_para_write(sel, 0x03);

	/* NV3029S GAMMA */
	sunxi_lcd_cmd_write(sel, 0xe0); /* gmama set 2.2 */
	sunxi_lcd_para_write(sel, 0x10);  /* PKP0[4:0]V3 */
	sunxi_lcd_para_write(sel, 0x17);  /* PKP1[4:0]V4 */
	sunxi_lcd_para_write(sel, 0x09);  /* PKP2[4:0]V10 */
	sunxi_lcd_para_write(sel, 0x1a);  /* PKP3[4:0]V21 */
	sunxi_lcd_para_write(sel, 0x05);  /* PKP4[4:0]V27 */
	sunxi_lcd_para_write(sel, 0x0E);  /* PKP5[4:0]V28 */
	sunxi_lcd_para_write(sel, 0x15);  /* PKP6[4:0]V15 */
	sunxi_lcd_cmd_write(sel, 0xe3);
	sunxi_lcd_para_write(sel, 0x09); /* PKN0[4:0]V3 */
	sunxi_lcd_para_write(sel, 0x17); /* PKN1[4:0]V4 */
	sunxi_lcd_para_write(sel, 0x10); /* PKN2[4:0]V10 */
	sunxi_lcd_para_write(sel, 0x19); /* PKN3[4:0]V21 */
	sunxi_lcd_para_write(sel, 0x08); /* PKN4[4:0]V27 */
	sunxi_lcd_para_write(sel, 0x14); /* PKN5[4:0]V28 */
	sunxi_lcd_para_write(sel, 0x13); /* PKN6[4:0]V15 */
	sunxi_lcd_cmd_write(sel, 0xe1);
	sunxi_lcd_para_write(sel, 0x17); /* PRP0[6:0]V5 */
	sunxi_lcd_para_write(sel, 0x58); /* PRP1[6:0]V26 */
	sunxi_lcd_cmd_write(sel, 0xe4);
	sunxi_lcd_para_write(sel, 0x16); /* PRN0[6:0]V5 */
	sunxi_lcd_para_write(sel, 0x59); /* PRN1[6:0]V26 */
	sunxi_lcd_cmd_write(sel, 0xe2);
	sunxi_lcd_para_write(sel, 0x10); /* VRP0[5:0]V0 */
	sunxi_lcd_para_write(sel, 0x1d); /* VRP1[5:0]V1 */
	sunxi_lcd_para_write(sel, 0x1a); /* VRP2[5:0]V2 */
	sunxi_lcd_para_write(sel, 0x12); /* VRP3[5:0]V29 */
	sunxi_lcd_para_write(sel, 0x0a); /* VRP4[5:0]V30 */
	sunxi_lcd_para_write(sel, 0x0f); /* VRP5[5:0]V31 */
	sunxi_lcd_cmd_write(sel, 0xe5);
	sunxi_lcd_para_write(sel, 0x02); /* VRN0[5:0]V0 */
	sunxi_lcd_para_write(sel, 0x1f); /* VRN1[5:0]V1 */
	sunxi_lcd_para_write(sel, 0x1c); /* VRN2[5:0]V2 */
	sunxi_lcd_para_write(sel, 0x18); /* VRN3[5:0]V29 */
	sunxi_lcd_para_write(sel, 0x12); /* VRN4[5:0]V30 */
	sunxi_lcd_para_write(sel, 0x30); /* VRN5[5:0]V31 */

	sunxi_lcd_cmd_write(sel, 0xEC);
	sunxi_lcd_para_write(sel, 0xf6);
	sunxi_lcd_cmd_write(sel, 0xED);
	sunxi_lcd_para_write(sel, 0x02);
	sunxi_lcd_para_write(sel, 0x94);

	sunxi_lcd_cmd_write(sel, 0xfd);
	sunxi_lcd_para_write(sel, 0xfa);
	sunxi_lcd_para_write(sel, 0xfb);

	sunxi_lcd_cmd_write(sel, 0x35);
	sunxi_lcd_para_write(sel, 0x00);


	/* MY MX MV ML RGB MH 0 0 */
	if (info[sel].lcd_x > info[sel].lcd_y)
		rotate = 0xa0;
	else
		rotate = 0xC0;


	sunxi_lcd_cmd_write(sel, 0x3A); /* Interface Pixel Format */
	/* 55----RGB565;66---RGB666 */
	if (info[sel].lcd_pixel_fmt == LCDFB_FORMAT_RGB_565 ||
	    info[sel].lcd_pixel_fmt == LCDFB_FORMAT_BGR_565) {
		sunxi_lcd_para_write(sel, 0x65);
		if (info[sel].lcd_pixel_fmt == LCDFB_FORMAT_RGB_565)
			rotate &= 0xf7;
		else
			rotate |= 0x08;
	} else if (info[sel].lcd_pixel_fmt < LCDFB_FORMAT_RGB_888) {
		sunxi_lcd_para_write(sel, 0x66);
		if (info[sel].lcd_pixel_fmt == LCDFB_FORMAT_BGRA_8888 ||
		    info[sel].lcd_pixel_fmt == LCDFB_FORMAT_BGRX_8888 ||
		    info[sel].lcd_pixel_fmt == LCDFB_FORMAT_ABGR_8888 ||
		    info[sel].lcd_pixel_fmt == LCDFB_FORMAT_XBGR_8888) {
			rotate |= 0x08;
		}
	} else {
		sunxi_lcd_para_write(sel, 0x66);
	}

	if (info[sel].lcd_if == LCD_FB_IF_SPI) {
		sunxi_lcd_cmd_write(sel, 0xb0);
		sunxi_lcd_para_write(sel, 0x03);
		sunxi_lcd_para_write(sel, 0xc8); /* Little endian */
	}

	sunxi_lcd_cmd_write(sel, 0x36); /* Memory sunxi_lcd_para_write Access Control */
	sunxi_lcd_para_write(sel, rotate);

	address(sel, 0, 0, info[sel].lcd_x - 1, info[sel].lcd_y - 1);

	sunxi_lcd_cmd_write(sel, 0x11);
	sunxi_lcd_delay_ms(120);
	sunxi_lcd_cmd_write(sel, 0x29);
	sunxi_lcd_cmd_write(sel, 0x2c); /* Display ON */

}

static void LCD_panel_exit(unsigned int sel)
{
	sunxi_lcd_cmd_write(sel, 0x28);
	sunxi_lcd_delay_ms(20);
	sunxi_lcd_cmd_write(sel, 0x10);
	sunxi_lcd_delay_ms(20);

	RESET(sel, 0);
	sunxi_lcd_delay_ms(10);

	sunxi_lcd_pin_cfg(sel, 0);

}

static int lcd_blank(unsigned int sel, unsigned int en)
{
	if (en)
		sunxi_lcd_cmd_write(sel, 0x28);
	else
		sunxi_lcd_cmd_write(sel, 0x29);
	sunxi_lcd_cmd_write(sel, 0x2c);
	return 0;
}


static s32 LCD_open_flow(u32 sel)
{
	lcd_fb_here;
	/* open lcd power, and delay 50ms */
	LCD_OPEN_FUNC(sel, LCD_power_on, 50);
	/* open lcd power, than delay 200ms */
	LCD_OPEN_FUNC(sel, LCD_panel_init, 20);

	LCD_OPEN_FUNC(sel, lcd_fb_black_screen, 50);
	/* open lcd backlight, and delay 0ms */
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);

	return 0;
}

static s32 LCD_close_flow(u32 sel)
{
	lcd_fb_here;
	/* close lcd backlight, and delay 0ms */
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 50);
	/* open lcd power, than delay 200ms */
	LCD_CLOSE_FUNC(sel, LCD_panel_exit, 10);
	/* close lcd power, and delay 500ms */
	LCD_CLOSE_FUNC(sel, LCD_power_off, 10);

	return 0;
}

static void LCD_power_on(u32 sel)
{
	/* config lcd_power pin to open lcd power0 */
	lcd_fb_here;
	power_en(sel, 1);

	sunxi_lcd_power_enable(sel, 0);

	sunxi_lcd_pin_cfg(sel, 1);
	RESET(sel, 1);
	sunxi_lcd_delay_ms(100);
	RESET(sel, 0);
	sunxi_lcd_delay_ms(100);
	RESET(sel, 1);
}

static void LCD_power_off(u32 sel)
{
	lcd_fb_here;
	/* config lcd_power pin to close lcd power0 */
	sunxi_lcd_power_disable(sel, 0);
	power_en(sel, 0);
}

static void LCD_bl_open(u32 sel)
{
	sunxi_lcd_pwm_enable(sel);
	/* config lcd_bl_en pin to open lcd backlight */
	sunxi_lcd_backlight_enable(sel);
	lcd_fb_here;
}

static void LCD_bl_close(u32 sel)
{
	/* config lcd_bl_en pin to close lcd backlight */
	sunxi_lcd_backlight_disable(sel);
	sunxi_lcd_pwm_disable(sel);
	lcd_fb_here;
}


/* sel: 0:lcd0; 1:lcd1 */
static s32 LCD_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	lcd_fb_here;
	return 0;
}

static int lcd_set_var(unsigned int sel, struct fb_info *p_info)
{
	return 0;
}

static int lcd_set_addr_win(unsigned int sel, int x, int y, int width, int height)
{
	address(sel, x, y, width, height);
	sunxi_lcd_cmd_write(sel, 0x2c);
	return 0;
}

struct __lcd_panel nv3029s_panel = {
	/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
	.name = "nv3029s",
	.func = {
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		.lcd_user_defined_func = LCD_user_defined_func,
		.blank = lcd_blank,
		.set_var = lcd_set_var,
		.set_addr_win = lcd_set_addr_win,
		},
};
