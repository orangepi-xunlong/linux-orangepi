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

#include "de_sharp_platform.h"

static struct de_sharp_desc vch0_sharp = {
	.name = "vch0_sharp",
	.id = 0,
	.reg_offset = 0x06000,
};

static struct de_sharp_desc *de350_sharp[] = {
	&vch0_sharp,
};

static struct de_sharp_desc *de352_sharp[] = {
	&vch0_sharp,
};

static struct de_sharp_desc *de355_sharp[] = {
	&vch0_sharp,
};

static struct de_version_sharp de350 = {
	.version = 0x350,
	.sharp_cnt = ARRAY_SIZE(de350_sharp),
	.sharp = &de350_sharp[0],
};

static struct de_version_sharp de352 = {
	.version = 0x352,
	.sharp_cnt = ARRAY_SIZE(de352_sharp),
	.sharp = &de352_sharp[0],
};

static struct de_version_sharp de355 = {
	.version = 0x355,
	.sharp_cnt = ARRAY_SIZE(de355_sharp),
	.sharp = &de355_sharp[0],
};

static struct de_version_sharp *de_version[] = {
	&de350, &de352, &de355
};

const struct de_sharp_desc *get_sharp_desc(struct module_create_info *info)
{
	int i, j;
	for (i = 0; i < ARRAY_SIZE(de_version); i++) {
		if (de_version[i]->version == info->de_version) {
			for (j = 0; j < de_version[i]->sharp_cnt; j++) {
				if (de_version[i]->sharp[j]->id == info->id)
					return de_version[i]->sharp[j];
			}
		}
	}
	return NULL;
}
