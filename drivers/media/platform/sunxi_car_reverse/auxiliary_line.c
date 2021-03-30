/*
 * Fast car reverse, auxiliary line
 *
 * Copyright (C) 2015-2018 AllwinnerTech, Inc.
 *
 * Contacts:
 * Zeng.Yajian <zengyajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "include.h"

struct argb_t {
	unsigned char transp;
	unsigned char red;
	unsigned char green;
	unsigned char blue;
};

typedef void (*colormap_func)(int x, int y, struct argb_t *out);

struct canvas {
	char *base;
	int width;
	int height;
	int bpp;
	int stride;
};

static int draw_by_func(struct canvas *ca, colormap_func func)
{
	int i, j;
	struct argb_t color;
	char *lines_start;
	unsigned char *from;

	lines_start = (char *)ca->base;
	for (i = 0; i < ca->height; i++) {
		from = lines_start;
		for (j = 0; j < ca->width; j++) {
			func(j, i, &color);
			*from++ = color.blue;
			*from++ = color.green;
			*from++ = color.red;
			*from++ = color.transp;
		}
		lines_start += ca->stride;
	}
	return 0;
}

/*
 *       node[3] +-------------+
 *              /               \
 *     node[2] +-----------------+
 *            /                   \
 *   node[1] +---------------------+
 *          /                       \
 * node[0] +-------------------------+
 *        /                           \
 *   y = 0.92x + 74
 *
 */

static int ylimit = 854;
static int xlimit = 223;

static int check_vertical(const int xstart, int x, int y,
					struct argb_t *out, const struct argb_t *color)
{
	int done = 0;
	int xend = xstart + 5;

	if (x >= xstart && x <= xend) {
		int ystart = 92 * x / 100 + 74;
		int yend   = ylimit - ystart;

		if (y >= ystart && y <= yend) {
			out->transp = color->transp;
			out->red    = color->red;
			out->green  = color->green;
			out->blue   = color->blue;
			done = 1;
		}
	}

	return done;
}

static void colormap(int x, int y, struct argb_t *out)
{
	const struct argb_t unvisable = {0x00, 0x00, 0x00, 0x00};
	const struct argb_t red       = {0xff, 0xff, 0x00, 0x00};
	const struct argb_t yellow    = {0xff, 0xff, 0xC8, 0x00};
	const struct argb_t green     = {0xff, 0x00, 0xf7, 0x00};
	const int node[4] = {70, 128, 180, 218};
	int v, mirror;

	if (x < 0 || x > xlimit) {
		out->transp = unvisable.transp;
		out->red    = unvisable.red;
		out->green  = unvisable.green;
		out->blue   = unvisable.blue;
		return;
	}

	v = y - 92 * x / 100 - 74;
	if (v >= 0 && v <= 8) {
		out->transp = red.transp;
		out->red    = red.red;
		out->green  = red.green;
		out->blue   = red.blue;
	} else {

		mirror = ylimit - y;
		v = mirror - 92 * x / 100 - 74;
		if (v >= 0 && v <= 8) {
			out->transp = red.transp;
			out->red    = red.red;
			out->green  = red.green;
			out->blue   = red.blue;
		} else {
			out->transp = unvisable.transp;
			out->red    = unvisable.red;
			out->green  = unvisable.green;
			out->blue   = unvisable.blue;

			check_vertical(node[0], x, y, out, &red);
			check_vertical(node[1], x, y, out, &yellow);
			check_vertical(node[2], x, y, out, &green);
			check_vertical(node[3], x, y, out, &green);
		}
	}
}

int draw_auxiliary_line(void *base, int width, int height)
{
	struct canvas canvas;

	canvas.base   = base;
	canvas.width  = width;
	canvas.height = height;
	canvas.bpp    = 32;
	canvas.stride = width * 4;

	return draw_by_func(&canvas, colormap);
}
