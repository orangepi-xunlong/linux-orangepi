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

#include "de_deband_platform.h"

#define DISP_DEBAND_OFFSET (0x07000)

struct de_version_deband {
	unsigned int version;
	unsigned int deband_cnt;
	struct de_deband_desc *debands;
};

static struct de_deband_desc de352_debands[] = {
	{
		.name = "disp0_deband",
		.id = 0,
		.reg_offset = DISP_DEBAND_OFFSET,
	},
};

static struct de_version_deband de352 = {
	.version = 0x352,
	.deband_cnt = ARRAY_SIZE(de352_debands),
	.debands = &de352_debands[0],
};

static struct de_version_deband *de_version[] = {
	&de352,
};

const struct de_deband_desc *get_deband_desc(struct module_create_info *info)
{
	int i, j;
	for (i = 0; i < ARRAY_SIZE(de_version); i++) {
		if (de_version[i]->version == info->de_version) {
			for (j = 0; j < de_version[i]->deband_cnt; j++) {
				if (de_version[i]->debands[j].id == info->id)
					return &de_version[i]->debands[j];
			}
		}
	}
	return NULL;
}
