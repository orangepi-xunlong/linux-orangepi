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

#include "de_snr_platform.h"

static struct de_snr_desc vch0_snr = {
	.name = "vch0_snr",
	.id = 0,
	.reg_offset = 0x06400,
};

static struct de_snr_desc vch2_snr = {
	.name = "vch2_snr",
	.id = 2,
	.reg_offset = 0x06400,
};

static struct de_snr_desc *de350_snr[] = {
	&vch0_snr, &vch2_snr,
};

static struct de_snr_desc *de352_snr[] = {
	&vch0_snr, &vch2_snr,
};


static struct de_version_snr de350 = {
	.version = 0x350,
	.snr_cnt = ARRAY_SIZE(de350_snr),
	.snr = &de350_snr[0],
};

static struct de_version_snr de352 = {
	.version = 0x352,
	.snr_cnt = ARRAY_SIZE(de352_snr),
	.snr = &de352_snr[0],
};

static struct de_version_snr *de_version[] = {
	&de350, &de352,
};

const struct de_snr_desc *get_snr_desc(struct module_create_info *info)
{
	int i, j;
	for (i = 0; i < ARRAY_SIZE(de_version); i++) {
		if (de_version[i]->version == info->de_version) {
			for (j = 0; j < de_version[i]->snr_cnt; j++) {
				if (de_version[i]->snr[j]->id == info->id)
					return de_version[i]->snr[j];
			}
		}
	}
	return NULL;
}
