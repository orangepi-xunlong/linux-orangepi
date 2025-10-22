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

#include "de_gamma_platform.h"

#define DISP_GAMMA_OFFSET	(0x9000)
#define DISP_CHN_GAMMA_OFFSET	(0x16000)

struct de_version_gamma {
	unsigned int version;
	unsigned int gamma_cnt;
	struct de_gamma_dsc *gammas;
};

static struct de_gamma_dsc de210_gammas[] = {
	{
		.id = 0,
		.gamma_lut_len = 256,
		.cm_bit_width = 8,
		.support_ctc = true,
		.support_cm = true,
		.support_demo_skin = true,
		.type = DEVICE_GAMMA,
		.reg_offset = DISP_GAMMA_OFFSET,
	},
};

static struct de_version_gamma de210 = {
	.version = 0x210,
	.gamma_cnt = ARRAY_SIZE(de210_gammas),
	.gammas = de210_gammas,
};

static struct de_gamma_dsc de352_gammas[] = {
	{
		.id = 0,
		.gamma_lut_len = 1024,
		.cm_bit_width = 10,
		.support_ctc = true,
		.support_cm = true,
		.support_demo_skin = true,
		.type = DEVICE_GAMMA,
		.reg_offset = DISP_GAMMA_OFFSET,
	},
	{
		.id = 1,
		.gamma_lut_len = 1024,
		.cm_bit_width = 10,
		.support_ctc = true,
		.support_cm = true,
		.support_demo_skin = true,
		.type = DEVICE_GAMMA,
		.reg_offset = DISP_GAMMA_OFFSET,
	},
	{
		.id = 0,
		.gamma_lut_len = 1024,
		.cm_bit_width = 10,
		.support_ctc = false,
		.support_cm = false,
		.support_demo_skin = true,
		.type = CHANNEL_DLC_GAMMA,
		.reg_offset = DISP_CHN_GAMMA_OFFSET,
	},
};

static struct de_version_gamma de352 = {
	.version = 0x352,
	.gamma_cnt = ARRAY_SIZE(de352_gammas),
	.gammas = de352_gammas,
};

static struct de_gamma_dsc de355_gammas[] = {
	{
		.id = 0,
		.gamma_lut_len = 1024,
		.cm_bit_width = 10,
		.support_ctc = true,
		.support_cm = false,
		.support_demo_skin = true,
		.type = DEVICE_GAMMA,
		.reg_offset = DISP_GAMMA_OFFSET,
	},
	{
		.id = 1,
		.gamma_lut_len = 1024,
		.cm_bit_width = 10,
		.support_ctc = true,
		.support_cm = false,
		.support_demo_skin = true,
		.type = DEVICE_GAMMA,
		.reg_offset = DISP_GAMMA_OFFSET,
	},
	{
		.id = 0,
		.gamma_lut_len = 1024,
		.cm_bit_width = 10,
		.support_ctc = false,
		.support_cm = false,
		.support_demo_skin = true,
		.type = CHANNEL_DLC_GAMMA,
		.reg_offset = DISP_CHN_GAMMA_OFFSET,
	},
};

static struct de_version_gamma de355 = {
	.version = 0x355,
	.gamma_cnt = ARRAY_SIZE(de355_gammas),
	.gammas = de355_gammas,
};

static struct de_version_gamma *de_version[] = {
	&de210, &de352, &de355
};

const struct de_gamma_dsc *get_gamma_dsc(struct module_create_info *info)
{
	int i, j;
	struct gamma_extra_create_info *ex = info->extra;

	if (IS_ERR_OR_NULL(ex))
		return NULL;

	for (i = 0; i < ARRAY_SIZE(de_version); i++) {
		if (de_version[i]->version == info->de_version) {
			for (j = 0; j < de_version[i]->gamma_cnt; j++) {
				if (ex->type == de_version[i]->gammas[j].type) {
					if (de_version[i]->gammas[j].id == info->id)
						return &de_version[i]->gammas[j];
				}
			}
		}
	}
	return NULL;
}

