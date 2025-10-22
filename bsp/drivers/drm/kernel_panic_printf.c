/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * kernel_panic_printf/kernel_panic_printf.c
 *
 * Copyright (c) 2007-2023 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
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
#include <linux/fb.h>
#include <sunxi-log.h>
#include "error_img.h"
#include "fonts.h"
#include "sunxi_fbdev.h"

struct fb_info *sunxi_get_fb_info(int fb_id);
int sunxi_fb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info);
struct drm_fb_info *sunxi_get_drmfb_info(int fb_id);
int sunxi_drmfb_pan_display(struct drm_fb_info *info);

struct graphic_buffer {
	int width;
	int height;
	unsigned char *buffer;
};

void draw_color(struct graphic_buffer *img, u32 color)
{
	int offset = 0, i, j;
	for (i = 0; i < img->height; i++)
		for (j = 0; j < img->width; j++) {
			unsigned int *p =
			    (unsigned int *)(img->buffer + offset);
			*p = color;
			offset += 4;
		}
}

void draw_pixel(struct graphic_buffer *img, int x, int y, u32 color)
{
	int offset = (img->width * y + x) * 4;
	unsigned int *p = (unsigned int *)(img->buffer + offset);
	*p = color;
}

void draw_img(struct graphic_buffer *img, int px, int py,
	      const unsigned char *raw, int w, int h)
{
	int offset = 0, i = 0, j = 0;
	u32 color;

	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			unsigned int r = raw[offset];
			unsigned int g = raw[offset + 1];
			unsigned int b = raw[offset + 2];
			offset += 3;
			color = ((int)0xff << 24) | (r << 16) | (g << 8) | b;
			draw_pixel(img, px + j, py + i, color);
		}
	}
}

struct glyph_info *glyph_find(unsigned char t)
{
	int i = 0;
	struct glyph_info *glyph;

	for (i = 0; i < character_list_size; i++) {
		glyph = &character_list[i];
		if (glyph->character == t)
			return glyph;
	}
	return NULL;
}

void draw_glyph(struct graphic_buffer *img, int px, int py,
		const struct glyph_info *info)
{
	const unsigned char *glyph =
	    (const unsigned char *)font_glyph_bitmap + info->offset;
	int i = 0, j = 0;
	for (i = 0; i < info->h; i++) {
		for (j = 0; j < info->w; j++) {
			u32 color = glyph[i * info->w + j];
			color = ((color & 0xff) << 16) | (color & 0xff00) | ((color & 0xff0000) >> 16);
			draw_pixel(img, px + j, py + i, color | 0xff000000);
		}
	}
}

void glyph_reander_string(char *str, int px, int *py, struct graphic_buffer *img)
{
	int i = 0;
	unsigned char t;
	struct glyph_info *glyph;
	int origin_px = px, line_height = 0;

	line_height = character_font_size * 3 / 2;
	for (i = 0; i < strlen(str); i++) {
		t = str[i];
		if (px + character_font_size > img->width) {
			*py += line_height;
			px = origin_px;
			--i;

			if (*py > img->height)
				break;
			continue;
		}
		if (*py + line_height > img->height)
			break;
		if (t == ' ') {
			px += (character_font_size / 2);
			continue;
		}
		glyph = glyph_find(t);
		if (glyph) {
			px += glyph->bitmap_left;
			draw_glyph(img, px, *py - glyph->bitmap_top, glyph);
			px += (glyph->w + 1);
		}
	}
	*py += line_height;
}

#if IS_ENABLED (CONFIG_FB)
static void sunxi_fbdev_printf(char *string)
{
	struct fb_info *p_info = NULL;
	struct graphic_buffer img;
	int px, py, i = 0, j = 0;
	char temp[81] = {0};

	p_info = sunxi_get_fb_info(0);
	if (!p_info) {
		printk(KERN_ERR "Null fb info\n");
		goto err_free;
	}
	img.width = p_info->var.xres;
	img.height = p_info->var.yres;
	img.buffer = p_info->screen_base;
	if (!img.buffer) {
		printk(KERN_ERR "NULL fb buffer\n");
		goto err_free;
	}

	draw_color(&img, 0xff000000);
	px = crashdump_raw_width + 64;
	py = img.height / 2;
	draw_img(&img, 32, py - 2 * character_font_size, crashdump_img_raw,
		 crashdump_raw_width, crashdump_raw_height);

	i = 0;
	j = 0;
	while (string[i] != '\0') {
		if (string[i] == '\n') {
			temp[j] = '\0';
			glyph_reander_string(temp, px, &py, &img);
			j = 0;
		} else {
			temp[j] = string[i];
			++j;
			if (j == 80) {
				temp[j] = '\0';
				j = 0;
				glyph_reander_string(temp, px, &py, &img);
			}
		}
		++i;
	}
	temp[j] = '\0';
	glyph_reander_string(temp, px, &py, &img);
	p_info->var.reserved[0] = FB_ACTIVATE_FORCE;
	//platform_swap_rb_order(p_info, true);
	sunxi_fb_pan_display(&p_info->var, p_info);

err_free:
	return ;
}
#else
static void sunxi_drmfb_printf(char *string)
{
	struct drm_fb_info *p_info = NULL;
	struct graphic_buffer img;
	int px, py, i = 0, j = 0;
	char temp[81] = {0};

	p_info = sunxi_get_drmfb_info(0);
	if (!p_info) {
		printk(KERN_ERR "Null fb info\n");
		goto err_free;
	}
	img.width = p_info->xres;
	img.height = p_info->yres;
	img.buffer = p_info->screen_base;
	if (!img.buffer) {
		printk(KERN_ERR "NULL fb buffer\n");
		goto err_free;
	}

	draw_color(&img, 0xff000000);
	px = crashdump_raw_width + 64;
	py = img.height / 2;
	draw_img(&img, 32, py - 2 * character_font_size, crashdump_img_raw,
		 crashdump_raw_width, crashdump_raw_height);

	i = 0;
	j = 0;
	while (string[i] != '\0') {
		if (string[i] == '\n') {
			temp[j] = '\0';
			glyph_reander_string(temp, px, &py, &img);
			j = 0;
		} else {
			temp[j] = string[i];
			++j;
			if (j == 80) {
				temp[j] = '\0';
				j = 0;
				glyph_reander_string(temp, px, &py, &img);
			}
		}
		++i;
	}
	temp[j] = '\0';
	glyph_reander_string(temp, px, &py, &img);
	p_info->reserved[0] = FB_ACTIVATE_FORCE;
	//platform_swap_rb_order(p_info, true);
	sunxi_drmfb_pan_display(p_info);

err_free:
	return ;
}
#endif
void sunxi_kernel_panic_printf(const char *str, ...)
{
	int i = 0;
	char *string = NULL;
	va_list args;

	if (!str || !strlen(str)) {
		printk(KERN_ERR "Null string\n");
		return;
	}

	string = kzalloc(512, GFP_KERNEL);
	if (!string)
		printk(KERN_ERR "String malloc err\n");
	va_start(args, str);
	i = vsnprintf(string, 512, str, args);
	va_end(args);
	if (i > 512 || i == 0) {
		printk(KERN_ERR "Out of string length\n");
		goto err_free;
	}

#if IS_ENABLED (CONFIG_FB)
	sunxi_fbdev_printf(string);
#else
	sunxi_drmfb_printf(string);
#endif

err_free:
	kfree(string);
	return ;
}
EXPORT_SYMBOL_GPL(sunxi_kernel_panic_printf);

// End of File
