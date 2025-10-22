/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Fast car reverse, auxiliary line module
 *
 * Copyright (C) 2015-2023 AllwinnerTech, Inc.
 *
 * Authors:  Huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "include.h"
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>

struct argb_t {
	unsigned char transp;
	unsigned char red;
	unsigned char green;
	unsigned char blue;
};

typedef void (*colormap_func)(int x, int y, int rotate, int lr,
			      struct argb_t *out, int w, int h, int dx);

const static int sin_table[] = {
	0,  1,  3,  5,  6,  8,  10, 12, 13, 15, 17, 19, 20, 22, 24, 25, 27, 29, 30,
	32, 34, 35, 37, 39, 40, 42, 43, 45, 46, 48, 50, 51, 52, 54, 55, 57, 58, 60,
	61, 62, 64, 65, 66, 68, 69, 70, 71, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82,
	83, 84, 85, 86, 87, 88, 89, 89, 90, 91, 92, 92, 93, 93, 94, 95, 95, 96, 96,
	97, 97, 97, 98, 98, 98, 99, 99, 99, 99, 99, 99, 99, 99, 100};

const static int cos_table[] = {
	100, 99, 99, 99, 99, 99, 99, 99, 99, 98, 98, 98, 97, 97, 97, 96, 96, 95, 95,
	94,  93, 93, 92, 92, 91, 90, 89, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79,
	78,  77, 76, 75, 74, 73, 71, 70, 69, 68, 66, 65, 64, 62, 61, 60, 58, 57, 55,
	54,  52, 51, 50, 48, 46, 45, 43, 42, 40, 39, 37, 35, 34, 32, 30, 29, 27, 25,
	24,  22, 20, 19, 17, 15, 13, 12, 10, 8,  6,  5,  3,  1,  0};

struct canvas {
	char *base;
	int width;
	int height;
	int bpp;
	int stride;
};

static int LZou;
static int xgap;
static int ygap;
static int SensorAgle;
static int Car_Orientation;
static int Car_Lr;
static int Max_Agle;
#define Abs(x) (x >= 0 ? x : (-x))

static int Cal_CarTrail(int x, int y, int rotate)
{
	int a1 = 0;
	int a2 = 0;
	int a3 = 0;
	int i = 0;
	int dx = 0, dy = 0;
	if (y < ygap) {
		return 0xFFFF;
	}
	if (rotate == 0) {
		return 0;
	}
	a1 = LZou * (cos_table[rotate]) / sin_table[rotate];
	a2 = a1;
	while (1) {
		int tmp = 0;
		dy = y - ygap;
		tmp = (dy * dy + i * i) - a1 * a1;
		if (Abs(a2) >= Abs(tmp)) {
			a2 = tmp;
			dx = i;
		}
		i++;
		if (i >= a1)
			break;
	}
	if (i == a1 && dx == 0)
		return 0xFFFF;
	a3 = a1 - dx;
	return a3;
}

static int draw_by_func(struct canvas *ca, int rotate, int lr,
			colormap_func func)
{
	int i, j;
	struct argb_t color;
	char *lines_start;
	unsigned char *from;
	int dx = 0;
	lines_start = (char *)ca->base;
	for (i = 0; i < ca->height; i++) {
		from = lines_start;
		dx = Cal_CarTrail(j, i, rotate);
		for (j = 0; j < ca->width; j++) {
			/* func(j, i, &color); */
			func(j, i, rotate, lr, &color, ca->width, ca->height,
			     dx);
			*from++ = color.blue;
			*from++ = color.green;
			*from++ = color.red;
			*from++ = color.transp;
		}
		lines_start += ca->stride;
	}
	return 0;
}

static void colormap(int x, int y, int rotate, int lr, struct argb_t *out,
		     int w, int h, int dx)

{
	struct argb_t unvisable = {0x00, 0x00, 0x00, 0x00};
	struct argb_t red = {0xff, 0xff, 0x00, 0x00};
	struct argb_t yellow = {0xff, 0xff, 0xFF, 0x00};
	struct argb_t green = {0xff, 0x00, 0xf7, 0x00};
	struct argb_t *color;

	int dSensor = 0;
	int seg = (h - ygap) / 4;
	if (y < ygap) {
		out->red = unvisable.red;
		out->green = unvisable.green;
		out->blue = unvisable.blue;
		out->transp = unvisable.transp;
		return;
	}
	dSensor = (y - ygap) * sin_table[SensorAgle] / cos_table[SensorAgle];
	if ((y - ygap) == 0 || (y - ygap) == 1 || (y - ygap) == 2 ||
	    (y - ygap) == seg || ((y - ygap) == seg + 1) ||
	    ((y - ygap) == seg + 2) || (y - ygap) == (2 * seg) ||
	    (y - ygap) == (2 * seg + 1) || (y - ygap) == (2 * seg + 2)) {
		if ((((w - xgap) / 2 - dSensor) <= x) &&
		    ((w + xgap) / 2 + dSensor) >= x) {
			out->transp = green.transp;
			out->red = green.red;
			out->green = green.green;
			out->blue = green.blue;
		} else {
			out->red = unvisable.red;
			out->green = unvisable.green;
			out->blue = unvisable.blue;
			out->transp = unvisable.transp;
		}
	} else if ((y - ygap) == (3 * seg) || (y - ygap) == (3 * seg + 1) ||
		   (y - ygap) == (3 * seg + 2)) {
		if ((((w - xgap) / 2 - dSensor) <= x) &&
		    ((w + xgap) / 2 + dSensor) >= x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else {
			out->red = unvisable.red;
			out->green = unvisable.green;
			out->blue = unvisable.blue;
			out->transp = unvisable.transp;
		}
	} else if ((y - ygap) == (4 * seg) || (y - ygap) == (4 * seg - 1) ||
		   (y - ygap) == (4 * seg - 2)) {
		if ((((w - xgap) / 2 - dSensor) <= x) &&
		    ((w + xgap) / 2 + dSensor) >= x) {
			out->transp = red.transp;
			out->red = red.red;
			out->green = red.green;
			out->blue = red.blue;
		} else {
			out->red = unvisable.red;
			out->green = unvisable.green;
			out->blue = unvisable.blue;
			out->transp = unvisable.transp;
		}
	} else {
		if ((y - ygap) >= 0 && (y - ygap) <= (2 * seg + 2)) {
			color = &green;
		} else if ((y - ygap) >= (2 * seg + 2) &&
			   (y - ygap) <= (3 * seg + 2)) {
			color = &yellow;
		} else {
			color = &red;
		}
		if ((w - xgap) / 2 - dSensor == x) {
			out->transp = color->transp;
			out->red = color->red;
			out->green = color->green;
			out->blue = color->blue;
		} else if (((w - xgap) / 2 - 1 - dSensor) == x) {
			out->transp = color->transp;
			out->red = color->red;
			out->green = color->green;
			out->blue = color->blue;
		} else if (((w - xgap) / 2 - 2 - dSensor) == x) {
			out->transp = color->transp;
			out->red = color->red;
			out->green = color->green;
			out->blue = color->blue;
		} else if (((w - xgap) / 2 - 3 - dSensor) == x) {
			out->transp = color->transp;
			out->red = color->red;
			out->green = color->green;
			out->blue = color->blue;
		} else if ((w + xgap) / 2 + dSensor == x) {
			out->transp = color->transp;
			out->red = color->red;
			out->green = color->green;
			out->blue = color->blue;
		} else if (((w + xgap) / 2 + 1 + dSensor) == x) {
			out->transp = color->transp;
			out->red = color->red;
			out->green = color->green;
			out->blue = color->blue;
		} else if (((w + xgap) / 2 + 2 + dSensor) == x) {
			out->transp = color->transp;
			out->red = color->red;
			out->green = color->green;
			out->blue = color->blue;
		} else if (((w + xgap) / 2 + 3 + dSensor) == x) {
			out->transp = color->transp;
			out->red = color->red;
			out->green = color->green;
			out->blue = color->blue;
		} else {
			out->red = unvisable.red;
			out->green = unvisable.green;
			out->blue = unvisable.blue;
			out->transp = unvisable.transp;
		}
	}
	if (dx > ((w - xgap) / 2) || dx < 0) {
		return;
	}
	if (rotate <= Max_Agle && (lr == 0)) {
		if ((w - xgap) / 2 - dx - 4 - dSensor == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w - xgap) / 2 - dx - 5 - dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w - xgap) / 2 - dx - 6 - dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w - xgap) / 2 - dx - 7 - dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if ((w + xgap) / 2 - dx + 4 + dSensor == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w + xgap) / 2 - dx + 5 + dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w + xgap) / 2 - dx + 6 + dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w + xgap) / 2 - dx + 7 + dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		}
	}
	if (rotate <= Max_Agle && lr) {
		if ((w - xgap) / 2 + dx - 4 - dSensor == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w - xgap) / 2 + dx - 5 - dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w - xgap) / 2 + dx - 6 - dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w - xgap) / 2 + dx - 7 - dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if ((w + xgap) / 2 + dx + 4 + dSensor == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w + xgap) / 2 + dx + 5 + dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w + xgap) / 2 + dx + 6 + dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		} else if (((w + xgap) / 2 + dx + 7 + dSensor) == x) {
			out->transp = yellow.transp;
			out->red = yellow.red;
			out->green = yellow.green;
			out->blue = yellow.blue;
		}
	}
	return;
}

static void init_auxiliary_paramter(int ld, int hgap, int vgap, int sA, int mA)
{
	LZou = ld;
	xgap = hgap;
	ygap = vgap;
	SensorAgle = sA;
	Car_Orientation = -1;
	Max_Agle = mA;
	Car_Lr = -1;
	return;
}

static int draw_auxiliary_line(void *base, int width, int height, int rotate, int lr)
{
	struct canvas canvas;

	PREVIEW_AUXLINE_DBG("width:%d, height:%d, angle:%d, lr:%s\n",
			    width, height, rotate, lr ? "right" : "left");

	if (Car_Orientation == rotate && lr == Car_Lr)
		return 0;
	Car_Orientation = rotate;
	Car_Lr = lr;
	canvas.base = base;
	canvas.width = width;
	canvas.height = height;
	canvas.bpp = 32;
	canvas.stride = width * 4;
	draw_by_func(&canvas, rotate, lr, colormap);
	return 0;
}

int dynamic_aux_line_update(struct buffer_node *aux, int orientation, int lr, int width, int height)
{
	CAR_REVERSE_WRN("dynamic_aux_line_update is not support yet!\n");
	return -1;
}


int static_aux_line_update(struct buffer_node *aux, int orientation, int lr, int width, int height)
{
	void *vir_addr;
	struct dma_buf *dmabuf = aux->dmabuf;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	struct dma_buf_map map;
	int ret;
#endif

	if ((Car_Orientation == orientation && Car_Lr == lr))
		return 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	ret = dma_buf_vmap(aux->dmabuf, &map);
	vir_addr = map.vaddr;
	if (ret) {
		CAR_REVERSE_ERR("dma_buf_vmap failed!!\n");
		return -1;
	}
#else

	vir_addr = dma_buf_vmap(aux->dmabuf);
#endif

	draw_auxiliary_line(vir_addr, width, height, orientation, lr);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	dma_buf_vunmap(dmabuf, &map);
#else
	dma_buf_vunmap(dmabuf, vir_addr);
#endif

	return 0;
}

int auxiliary_line_init(void)
{
	init_auxiliary_paramter(800, 50, 200, 25, 80);
	return 0;
}
