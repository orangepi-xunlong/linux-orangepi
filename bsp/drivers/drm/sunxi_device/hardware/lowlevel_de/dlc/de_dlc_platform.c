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

#include "de_dlc_platform.h"

static struct de_dlc_desc vch0_dlc = {
	.name = "vch0_dlc",
	.id = 0,
	.min_inwidth = 64,
	.min_inheight = 64,
	.reg_offset = 0x15000,
};

static struct de_dlc_desc *de352_dlc[] = {
	&vch0_dlc,
};

static struct de_dlc_desc *de355_dlc[] = {
	&vch0_dlc,
};

static struct de_version_dlc de352 = {
	.version = 0x352,
	.dlc_cnt = ARRAY_SIZE(de352_dlc),
	.dlc = &de352_dlc[0],
};

static struct de_version_dlc de355 = {
	.version = 0x355,
	.dlc_cnt = ARRAY_SIZE(de355_dlc),
	.dlc = &de355_dlc[0],
};

static struct de_version_dlc *de_version[] = {
	&de352, &de355
};

const struct de_dlc_desc *get_dlc_desc(struct module_create_info *info)
{
	int i, j;
	for (i = 0; i < ARRAY_SIZE(de_version); i++) {
		if (de_version[i]->version == info->de_version) {
			for (j = 0; j < de_version[i]->dlc_cnt; j++) {
				if (de_version[i]->dlc[j]->id == info->id)
					return de_version[i]->dlc[j];
			}
		}
	}
	return NULL;
}
