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
lcd_driver_name = "kld2844b";
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
lcd_dbi_te = <1>;
lcd_fps = <60>;
};
 *
 */

#include "kld2844b.h"

static void LCD_power_on(u32 sel);
static void LCD_power_off(u32 sel);
static void LCD_bl_open(u32 sel);
static void LCD_bl_close(u32 sel);
static void LCD_panel_init(u32 sel);
static void LCD_panel_exit(u32 sel);
#define RESET(s, v) sunxi_lcd_gpio_set_value(s, 0, v)
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

	sunxi_lcd_cmd_write(sel, 0xb2);
	sunxi_lcd_para_write(sel, 0x0c);
	sunxi_lcd_para_write(sel, 0x0c);
	sunxi_lcd_para_write(sel, 0x00);
	sunxi_lcd_para_write(sel, 0x33);
	sunxi_lcd_para_write(sel, 0x33);
	sunxi_lcd_cmd_write(sel, 0xb7);
	sunxi_lcd_para_write(sel, 0x75);

	/* MY MX MV ML RGB MH 0 0 */
	if (info[sel].lcd_x > info[sel].lcd_y)
		rotate = 0xa0;
	else
		rotate = 0xC0;


	sunxi_lcd_cmd_write(sel, 0x3A); /* Interface Pixel Format */
	/* 55----RGB565;66---RGB666 */
	if (info[sel].lcd_pixel_fmt == LCDFB_FORMAT_RGB_565 ||
	    info[sel].lcd_pixel_fmt == LCDFB_FORMAT_BGR_565) {
		sunxi_lcd_para_write(sel, 0x55);
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

	if (info[sel].lcd_if == LCD_FB_IF_DBI &&
	    info[sel].dbi_if == LCD_FB_D2LI) {
		/* two data lane */
		sunxi_lcd_cmd_write(sel, 0xE7);
		sunxi_lcd_para_write(sel, 0x10);
	}

	sunxi_lcd_cmd_write(sel, 0x35);
	sunxi_lcd_para_write(sel, 0x00);

	sunxi_lcd_cmd_write(sel, 0xbb);
	sunxi_lcd_para_write(sel, 0x2b);
	sunxi_lcd_cmd_write(sel, 0xc0);
	sunxi_lcd_para_write(sel, 0x2c);
	sunxi_lcd_cmd_write(sel, 0xc2);
	sunxi_lcd_para_write(sel, 0x01);
	sunxi_lcd_cmd_write(sel, 0xc3);
	sunxi_lcd_para_write(sel, 0x0b);
	sunxi_lcd_cmd_write(sel, 0xc4);
	sunxi_lcd_para_write(sel, 0x20);
	sunxi_lcd_cmd_write(sel, 0xc6);
	sunxi_lcd_para_write(sel, 0x0f);
	sunxi_lcd_cmd_write(sel, 0xd0);
	sunxi_lcd_para_write(sel, 0xa4);
	sunxi_lcd_para_write(sel, 0xa1);

	sunxi_lcd_cmd_write(sel, 0xe0);
	sunxi_lcd_para_write(sel, 0xd0);
	sunxi_lcd_para_write(sel, 0x01);
	sunxi_lcd_para_write(sel, 0x04);
	sunxi_lcd_para_write(sel, 0x09);
	sunxi_lcd_para_write(sel, 0x0b);
	sunxi_lcd_para_write(sel, 0x07);
	sunxi_lcd_para_write(sel, 0x2e);
	sunxi_lcd_para_write(sel, 0x44);
	sunxi_lcd_para_write(sel, 0x43);
	sunxi_lcd_para_write(sel, 0x0b);
	sunxi_lcd_para_write(sel, 0x16);
	sunxi_lcd_para_write(sel, 0x15);
	sunxi_lcd_para_write(sel, 0x17);
	sunxi_lcd_para_write(sel, 0x1D);

	sunxi_lcd_cmd_write(sel, 0xE1);
	sunxi_lcd_para_write(sel, 0xD0);
	sunxi_lcd_para_write(sel, 0x01);
	sunxi_lcd_para_write(sel, 0x05);
	sunxi_lcd_para_write(sel, 0x0A);
	sunxi_lcd_para_write(sel, 0x0B);
	sunxi_lcd_para_write(sel, 0x08);
	sunxi_lcd_para_write(sel, 0x2F);
	sunxi_lcd_para_write(sel, 0x44);
	sunxi_lcd_para_write(sel, 0x41);
	sunxi_lcd_para_write(sel, 0x0A);
	sunxi_lcd_para_write(sel, 0x15);
	sunxi_lcd_para_write(sel, 0x14);
	sunxi_lcd_para_write(sel, 0x19);
	sunxi_lcd_para_write(sel, 0x1D);

	address(sel, 0, 0, info[sel].lcd_x - 1, info[sel].lcd_y - 1);

	sunxi_lcd_cmd_write(sel, 0x11); /* Sleep Out */
	sunxi_lcd_delay_ms(120);
	sunxi_lcd_cmd_write(sel, 0x29); /* Display ON */
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

static int lcd_set_var(unsigned int sel, struct fb_info *p_info)
{
	return 0;
}

static int lcd_blank(unsigned int sel, unsigned int en)
{
	if (en)
		sunxi_lcd_cmd_write(sel, 0x28);
	else
		sunxi_lcd_cmd_write(sel, 0x29);

	sunxi_lcd_cmd_write(sel, 0x2c); /* Display ON */
	return 0;
}

static int lcd_set_addr_win(unsigned int sel, int x, int y, int width, int height)
{
	address(sel, x, y, width, height);
	sunxi_lcd_cmd_write(sel, 0x2c); /* Display ON */
	return 0;
}

struct __lcd_panel kld2844b_panel = {
	/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
	.name = "kld2844b",
	.func = {
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		.blank = lcd_blank,
		.set_var = lcd_set_var,
		.set_addr_win = lcd_set_addr_win,
		},
};
