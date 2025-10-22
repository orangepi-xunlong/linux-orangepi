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

#include "de_dci_platform.h"

static struct de_dci_desc vch0_dci = {
	.name = "vch0_dci",
	.id = 0,
	.reg_offset = 0x10000,
};

static struct de_dci_desc *de350_dci[] = {
	&vch0_dci,
};

static struct de_dci_desc *de352_dci[] = {
	&vch0_dci,
};


static struct de_version_dci de350 = {
	.version = 0x350,
	.dci_cnt = ARRAY_SIZE(de350_dci),
	.dci = &de350_dci[0],
};

static struct de_version_dci de352 = {
	.version = 0x352,
	.dci_cnt = 0, // maybe only support ftd
	.dci = &de352_dci[0],
};

static struct de_version_dci *de_version[] = {
	&de350, &de352,
};

const struct de_dci_desc *get_dci_desc(struct module_create_info *info)
{
	int i, j;
	for (i = 0; i < ARRAY_SIZE(de_version); i++) {
		if (de_version[i]->version == info->de_version) {
			for (j = 0; j < de_version[i]->dci_cnt; j++) {
				if (de_version[i]->dci[j]->id == info->id)
					return de_version[i]->dci[j];
			}
		}
	}
	return NULL;
}
