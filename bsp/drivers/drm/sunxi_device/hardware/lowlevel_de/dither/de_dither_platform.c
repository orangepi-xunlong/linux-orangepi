/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2023 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "de_dither_platform.h"

struct de_version_dither {
	unsigned int version;
	unsigned int dither_cnt;
	struct de_dither_dsc *dithers;
};

static enum dither_out_fmt de35x_fmt[] = {
	FMT888,
	FMT444,
	FMT565,
	FMT666,
};

static enum dither_mode de35x_mode[] = {
	QUANTIZATION,
	FLOYD_STEINBERG,
	ORDER,
	SIERRA_LITE,
	BURKE,
	RANDOM,
};

static enum dither_out_fmt no_888[] = {
	FMT444,
	FMT565,
	FMT666,
};

static enum dither_mode no_random[] = {
	QUANTIZATION,
	FLOYD_STEINBERG,
	ORDER,
	SIERRA_LITE,
	BURKE,
};

static struct de_dither_dsc de210_dithers[] = {
	{
		.id = 0,
		.support_fmts = no_888,
		.fmt_cnt = ARRAY_SIZE(no_888),
		.support_modes = no_random,
		.mode_cnt = ARRAY_SIZE(no_random),
		.support_3d_fifo = false,
	},
};

static struct de_version_dither de210 = {
	.version = 0x210,
	.dither_cnt = ARRAY_SIZE(de210_dithers),
	.dithers = de210_dithers,
};

static struct de_dither_dsc de352_dithers[] = {
	{
		.id = 0,
		.support_fmts = de35x_fmt,
		.fmt_cnt = ARRAY_SIZE(de35x_fmt),
		.support_modes = de35x_mode,
		.mode_cnt = ARRAY_SIZE(de35x_mode),
		.support_3d_fifo = true,
	},
	{
		.id = 1,
		.support_fmts = de35x_fmt,
		.fmt_cnt = ARRAY_SIZE(de35x_fmt),
		.support_modes = de35x_mode,
		.mode_cnt = ARRAY_SIZE(de35x_mode),
		.support_3d_fifo = true,
	},
};

static struct de_version_dither de352 = {
	.version = 0x352,
	.dither_cnt = ARRAY_SIZE(de352_dithers),
	.dithers = de352_dithers,
};

static struct de_dither_dsc de355_dithers[] = {
	{
		.id = 0,
		.support_fmts = de35x_fmt,
		.fmt_cnt = ARRAY_SIZE(de35x_fmt),
		.support_modes = de35x_mode,
		.mode_cnt = ARRAY_SIZE(de35x_mode),
		.support_3d_fifo = true,
	},
	{
		.id = 1,
		.support_fmts = de35x_fmt,
		.fmt_cnt = ARRAY_SIZE(de35x_fmt),
		.support_modes = de35x_mode,
		.mode_cnt = ARRAY_SIZE(de35x_mode),
		.support_3d_fifo = true,
	},
};

static struct de_version_dither de355 = {
	.version = 0x355,
	.dither_cnt = ARRAY_SIZE(de355_dithers),
	.dithers = de355_dithers,
};

static struct de_version_dither *de_version[] = {
	&de210, &de352, &de355
};

const struct de_dither_dsc *get_dither_dsc(struct module_create_info *info)
{
	int i, j;
	for (i = 0; i < ARRAY_SIZE(de_version); i++) {
		if (de_version[i]->version == info->de_version) {
			for (j = 0; j < de_version[i]->dither_cnt; j++) {
				if (de_version[i]->dithers[j].id == info->id)
					return &de_version[i]->dithers[j];
			}
		}
	}
	return NULL;
}

