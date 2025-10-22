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

#include "de_fcm_platform.h"

static struct de_fcm_desc vch0_fcm = {
	.name = "vch0_fcm",
	.id = 0,
	.reg_offset = 0x11000,
};

static struct de_fcm_desc vch2_fcm = {
	.name = "vch2_fcm",
	.id = 2,
	.reg_offset = 0x11000,
};

static struct de_fcm_desc *de350_fcm[] = {
	&vch0_fcm, &vch2_fcm,
};

static struct de_fcm_desc *de352_fcm[] = {
	&vch0_fcm, &vch2_fcm,
};


static struct de_version_fcm de350 = {
	.version = 0x350,
	.fcm_cnt = ARRAY_SIZE(de350_fcm),
	.fcm = &de350_fcm[0],
};

static struct de_version_fcm de352 = {
	.version = 0x352,
	.fcm_cnt = ARRAY_SIZE(de352_fcm),
	.fcm = &de352_fcm[0],
};

static struct de_version_fcm *de_version[] = {
	&de350, &de352,
};

const struct de_fcm_desc *get_fcm_desc(struct module_create_info *info)
{
	int i, j;
	for (i = 0; i < ARRAY_SIZE(de_version); i++) {
		if (de_version[i]->version == info->de_version) {
			for (j = 0; j < de_version[i]->fcm_cnt; j++) {
				if (de_version[i]->fcm[j]->id == info->id)
					return de_version[i]->fcm[j];
			}
		}
	}
	return NULL;
}
