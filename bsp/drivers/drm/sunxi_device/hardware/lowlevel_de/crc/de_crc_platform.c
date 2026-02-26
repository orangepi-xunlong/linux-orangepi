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

#include "de_crc_platform.h"

struct de_version_crc {
	unsigned int version;
	unsigned int crc_disp_cnt;
	struct de_crc_dsc *crcs;
};

static struct de_crc_dsc de210_crcs[] = {
	{
		.id = 0,
		.region_cnt = 4,
	},
};

static struct de_version_crc de210 = {
	.version = 0x210,
	.crc_disp_cnt = ARRAY_SIZE(de210_crcs),
	.crcs = de210_crcs,
};

static struct de_crc_dsc de352_crcs[] = {
	{
		.id = 1,
		.region_cnt = 4,
	},
};

static struct de_version_crc de352 = {
	.version = 0x352,
	.crc_disp_cnt = ARRAY_SIZE(de352_crcs),
	.crcs = de352_crcs,
};

static struct de_version_crc *de_version[] = {
	&de210, &de352,
};

const struct de_crc_dsc *get_crc_dsc(struct module_create_info *info)
{
	int i, j;
	for (i = 0; i < ARRAY_SIZE(de_version); i++) {
		if (de_version[i]->version == info->de_version) {
			for (j = 0; j < de_version[i]->crc_disp_cnt; j++) {
				if (de_version[i]->crcs[j].id == info->id)
					return &de_version[i]->crcs[j];
			}
		}
	}
	return NULL;
}

