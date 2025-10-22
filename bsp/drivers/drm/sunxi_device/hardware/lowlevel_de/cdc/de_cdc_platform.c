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

#include "de_cdc_platform.h"
struct de_version_cdc {
	unsigned int version;
	unsigned int cdc_cnt;
	struct de_cdc_desc **cdc;
};

static struct de_cdc_desc vch0_cdc = {
	.name = "vch0_cdc",
	.id = 0,
	.reg_offset = 0x08000,
	.support_gtm = true,
	.support_csc = true,
};

static struct de_cdc_desc uch0_cdc = {
	.name = "uch0_cdc",
	.id = 3,
	.reg_offset = 0x08000,
};

static struct de_cdc_desc *de350_cdc[] = {
	&vch0_cdc, &uch0_cdc,
};

static struct de_cdc_desc *de352_cdc[] = {
	&vch0_cdc, &uch0_cdc,
};


static struct de_version_cdc de350 = {
	.version = 0x350,
	.cdc_cnt = ARRAY_SIZE(de350_cdc),
	.cdc = &de350_cdc[0],
};

static struct de_version_cdc de352 = {
	.version = 0x352,
	.cdc_cnt = ARRAY_SIZE(de352_cdc),
	.cdc = &de352_cdc[0],
};

static struct de_version_cdc *de_version[] = {
	&de350, &de352,
};

const struct de_cdc_desc *get_cdc_desc(struct module_create_info *info)
{
	int i, j;
	for (i = 0; i < ARRAY_SIZE(de_version); i++) {
		if (de_version[i]->version == info->de_version) {
			for (j = 0; j < de_version[i]->cdc_cnt; j++) {
				if (de_version[i]->cdc[j]->id == info->id)
					return de_version[i]->cdc[j];
			}
		}
	}
	return NULL;
}
