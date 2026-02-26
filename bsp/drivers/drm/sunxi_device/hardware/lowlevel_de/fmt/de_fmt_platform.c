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

#include "de_fmt_platform.h"

#define DE_DISP_BASE_OFFSET(base, disp, disp_size)			    \
					((base) + (disp) * (disp_size))

#define FMT_OFFSET_V3XX			(0x5000)
#define DISP_BASE_V3XX			(0x280000)
#define DE_DISP_SIZE_V3XX		(0x20000)

struct de_version_fmt {
	unsigned int version;
	unsigned int fmt_cnt;
	struct de_fmt_desc **fmt;
};

static struct de_fmt_desc de352_fmt0 = {
	.name = "formatter0",
	.id = 0,
	.disp_base = DE_DISP_BASE_OFFSET(DISP_BASE_V3XX, 0, DE_DISP_SIZE_V3XX),
	.fmt_offset = FMT_OFFSET_V3XX,
};

static struct de_fmt_desc de352_fmt1 = {
	.name = "formatter1",
	.id = 1,
	.disp_base = DE_DISP_BASE_OFFSET(DISP_BASE_V3XX, 1, DE_DISP_SIZE_V3XX),
	.fmt_offset = FMT_OFFSET_V3XX,
};

static struct de_fmt_desc *de352_fmts[] = {
	&de352_fmt0, &de352_fmt1,
};

static struct de_version_fmt de352 = {
	.version = 0x352,
	.fmt_cnt = ARRAY_SIZE(de352_fmts),
	.fmt = &de352_fmts[0],
};

static struct de_fmt_desc de355_fmt0 = {
	.name = "formatter0",
	.id = 0,
	.disp_base = DE_DISP_BASE_OFFSET(DISP_BASE_V3XX, 0, DE_DISP_SIZE_V3XX),
	.fmt_offset = FMT_OFFSET_V3XX,
};

static struct de_fmt_desc de355_fmt1 = {
	.name = "formatter1",
	.id = 1,
	.disp_base = DE_DISP_BASE_OFFSET(DISP_BASE_V3XX, 1, DE_DISP_SIZE_V3XX),
	.fmt_offset = FMT_OFFSET_V3XX,
};

static struct de_fmt_desc *de355_fmts[] = {
	&de355_fmt0, &de355_fmt1,
};

static struct de_version_fmt de355 = {
	.version = 0x355,
	.fmt_cnt = ARRAY_SIZE(de355_fmts),
	.fmt = &de355_fmts[0],
};

static struct de_version_fmt *de_version[] = {
	&de352, &de355
};

const struct de_fmt_desc *get_fmt_dsc(struct module_create_info *info)
{
	int i, j;

	for (i = 0; i < ARRAY_SIZE(de_version); i++) {
		if (de_version[i]->version == info->de_version) {
			for (j = 0; j < de_version[i]->fmt_cnt; j++) {
				if (de_version[i]->fmt[j]->id == info->id)
					return de_version[i]->fmt[j];
			}
		}
	}
	return NULL;
}
