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
 *
 *lcd_fb0: lcd_fb0@0 {
 *lcd_used = <1>;
 *lcd_driver_name = "kld35512";
 *lcd_if = <1>;
 *lcd_dbi_if = <4>;
 *lcd_data_speed = <60>;
 *lcd_spi_bus_num = <1>;
 *lcd_x = <480>;
 *lcd_y = <320>;
 *
 *rgb565
 *lcd_pixel_fmt = <10>;
 *lcd_dbi_fmt = <2>;
 *lcd_rgb_order = <0>;
 *lcd_width = <60>;
 *lcd_height = <95>;
 *lcd_pwm_used = <1>;
 *lcd_pwm_ch = <4>;
 *lcd_pwm_freq = <5000>;
 *lcd_pwm_pol = <0>;
 *lcd_frm = <1>;
 *lcd_gamma_en = <1>;
 *fb_buffer_num = <2>;
 *lcd_backlight = <100>;
 *lcd_fps = <40>;
 *lcd_dbi_te = <1>;
 *lcd_dbi_clk_mode = <1>;
};

 */

#include "kld35512.h"

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


static void reset_panel(unsigned int sel)
{
	lcd_fb_here;

	sunxi_lcd_pin_cfg(sel, 1);
	RESET(sel, 1);
	sunxi_lcd_delay_ms(100);
	RESET(sel, 0);
	sunxi_lcd_delay_ms(100);
	RESET(sel, 1);
}

static void LCD_panel_init(unsigned int sel)
{
	unsigned int rotate;


	if (bsp_disp_get_panel_info(sel, &info[sel])) {
		lcd_fb_wrn("get panel info fail!\n");
		return;
	}

	sunxi_lcd_cmd_write(sel, 0xF0);
	sunxi_lcd_para_write(sel, 0xC3);

	sunxi_lcd_cmd_write(sel, 0xF0);
	sunxi_lcd_para_write(sel, 0x96);

	sunxi_lcd_cmd_write(sel, 0xB4); /* Display Inversion Control */
	sunxi_lcd_para_write(sel, 0x01);  /* 1-dot */

	sunxi_lcd_cmd_write(sel, 0xB7);
	sunxi_lcd_para_write(sel, 0xC6);
	sunxi_lcd_cmd_write(sel, 0xe6); /* Fixed SPI 50MHz and 60MHz LCD display problems */
	sunxi_lcd_para_write(sel, 0xff);
	sunxi_lcd_para_write(sel, 0xc2);

	sunxi_lcd_cmd_write(sel, 0xC0);
	sunxi_lcd_para_write(sel, 0xF0);
	sunxi_lcd_para_write(sel, 0x35);

	sunxi_lcd_cmd_write(sel, 0xC1);
	sunxi_lcd_para_write(sel, 0x15);

	sunxi_lcd_cmd_write(sel, 0xC2);
	sunxi_lcd_para_write(sel, 0xAF);

	sunxi_lcd_cmd_write(sel, 0xC3);
	sunxi_lcd_para_write(sel, 0x09);

	sunxi_lcd_cmd_write(sel, 0xC5);
	sunxi_lcd_para_write(sel, 0x0F);

	sunxi_lcd_cmd_write(sel, 0xC6);
	sunxi_lcd_para_write(sel, 0x00);

	sunxi_lcd_cmd_write(sel, 0xE8);
	sunxi_lcd_para_write(sel, 0x40);
	sunxi_lcd_para_write(sel, 0x8A);
	sunxi_lcd_para_write(sel, 0x00);
	sunxi_lcd_para_write(sel, 0x00);
	sunxi_lcd_para_write(sel, 0x29);
	sunxi_lcd_para_write(sel, 0x19);
	sunxi_lcd_para_write(sel, 0xA5);
	sunxi_lcd_para_write(sel, 0x33);

	sunxi_lcd_cmd_write(sel, 0xE0);
	sunxi_lcd_para_write(sel, 0x70);
	sunxi_lcd_para_write(sel, 0x00);
	sunxi_lcd_para_write(sel, 0x05);
	sunxi_lcd_para_write(sel, 0x03);
	sunxi_lcd_para_write(sel, 0x02);
	sunxi_lcd_para_write(sel, 0x20);
	sunxi_lcd_para_write(sel, 0x29);
	sunxi_lcd_para_write(sel, 0x01);
	sunxi_lcd_para_write(sel, 0x45);
	sunxi_lcd_para_write(sel, 0x30);
	sunxi_lcd_para_write(sel, 0x09);
	sunxi_lcd_para_write(sel, 0x07);
	sunxi_lcd_para_write(sel, 0x22);
	sunxi_lcd_para_write(sel, 0x29);

	sunxi_lcd_cmd_write(sel, 0xE1);
	sunxi_lcd_para_write(sel, 0x70);
	sunxi_lcd_para_write(sel, 0x0C);
	sunxi_lcd_para_write(sel, 0x10);
	sunxi_lcd_para_write(sel, 0x0F);
	sunxi_lcd_para_write(sel, 0x0E);
	sunxi_lcd_para_write(sel, 0x09);
	sunxi_lcd_para_write(sel, 0x35);
	sunxi_lcd_para_write(sel, 0x64);
	sunxi_lcd_para_write(sel, 0x48);
	sunxi_lcd_para_write(sel, 0x3A);
	sunxi_lcd_para_write(sel, 0x14);
	sunxi_lcd_para_write(sel, 0x13);
	sunxi_lcd_para_write(sel, 0x2E);
	sunxi_lcd_para_write(sel, 0x30);

	sunxi_lcd_cmd_write(sel, 0x21);

	sunxi_lcd_cmd_write(sel, 0x35);
	sunxi_lcd_para_write(sel, 0x00);
	/*
	 * fps = 1e7/((168+rnta[4:0]+32*(15-frs[3:0]))*(480+vbp+vfp))
	 * vbp = 2, vfp = 2
	 * rnta[4:0]+32*(15-frs[3:0]) = (1e7/fps)/484 - 168
	 * rnta[4:0]=0x10
	 * frs[3:0] = 15 -((1e7/fps)/484 - 168 - 0x10)/32
	 * */
	sunxi_lcd_cmd_write(sel, 0xb1);
	sunxi_lcd_para_write(sel, 0x50);
	sunxi_lcd_para_write(sel, 0x10);

	if (info[sel].lcd_if == LCD_FB_IF_DBI &&
	    info[sel].dbi_if == LCD_FB_D2LI) {
		/* two data lane */
		sunxi_lcd_cmd_write(sel, 0xB9);
		sunxi_lcd_para_write(sel, 0x00);
		sunxi_lcd_para_write(sel, 0x01);
	}
	sunxi_lcd_cmd_write(sel, 0x36);
	/* MY MX MV ML RGB MH 0 0 */
	if (info[sel].lcd_x > info[sel].lcd_y)
		rotate = 0x20;
	else
		rotate = 0x40;
	sunxi_lcd_para_write(sel, rotate); /* horizon scrren */

	sunxi_lcd_cmd_write(sel, 0x3A);
	/* 55----RGB565;66---RGB666 */
	if (info[sel].lcd_pixel_fmt == LCDFB_FORMAT_RGB_565 ||
	    info[sel].lcd_pixel_fmt == LCDFB_FORMAT_BGR_565) {
		sunxi_lcd_para_write(sel, 0x55);
		sunxi_lcd_cmd_write(sel, 0x36); /* Memory sunxi_lcd_para_write Access Control */
		if (info[sel].lcd_pixel_fmt == LCDFB_FORMAT_RGB_565)
			sunxi_lcd_para_write(sel, rotate | 0x08);
		else
			sunxi_lcd_para_write(sel, rotate & 0xf7);
	} else if (info[sel].lcd_pixel_fmt < LCDFB_FORMAT_RGB_888) {
		sunxi_lcd_para_write(sel, 0x77);
		if (info[sel].lcd_pixel_fmt == LCDFB_FORMAT_RGBA_8888 ||
		    info[sel].lcd_pixel_fmt == LCDFB_FORMAT_RGBX_8888 ||
		    info[sel].lcd_pixel_fmt == LCDFB_FORMAT_ARGB_8888 ||
		    info[sel].lcd_pixel_fmt == LCDFB_FORMAT_XRGB_8888) {
			sunxi_lcd_cmd_write(sel, 0x36); /* Memory sunxi_lcd_para_write Access Control */
			sunxi_lcd_para_write(sel, rotate | 0x08);
		}

	} else {
		sunxi_lcd_para_write(sel, 0x66);
	}


	address(sel, 0, 0, info[sel].lcd_x - 1, info[sel].lcd_y - 1);
	sunxi_lcd_cmd_write(sel, 0xF0);
	sunxi_lcd_para_write(sel, 0x3c);

	sunxi_lcd_cmd_write(sel, 0xF0);
	sunxi_lcd_para_write(sel, 0x69);

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
	sunxi_lcd_cmd_write(sel, 0x2c); /* Display ON */
	return 0;
}


static s32 LCD_open_flow(u32 sel)
{
	lcd_fb_here;
	/* open lcd power, and delay 50ms */
	LCD_OPEN_FUNC(sel, LCD_power_on, 50);
	/* open lcd power, than delay 200ms */
	LCD_OPEN_FUNC(sel, LCD_panel_init, 20);

	LCD_OPEN_FUNC(sel, lcd_fb_black_screen, 20);
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
	reset_panel(sel);
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

static int lcd_set_addr_win(unsigned int sel, int x, int y, int width, int height)
{
	address(sel, x, y, width, height);
	sunxi_lcd_cmd_write(sel, 0x2c); /* Display ON */
	return 0;
}

struct __lcd_panel kld35512_panel = {
	/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
	.name = "kld35512",
	.func = {
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		.blank = lcd_blank,
		.set_var = lcd_set_var,
		.set_addr_win = lcd_set_addr_win,
		},
};
