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

#include "de_scaler_platform.h"

static struct de_scaler_dsc de350_scalers[] = {
	{
		.name = "vch0_asu10",
		.id = 0,
		.type = DE_SCALER_TYPE_ASU,
		.line_buffer_yuv = 4096,
		.line_buffer_rgb = 4096, /* 2 line mode */
		.line_buffer_yuv_ed = 2048,
	},
	{
		.name = "vch1_vsu8",
		.id = 1,
		.type = DE_SCALER_TYPE_VSU8,
		.line_buffer_yuv = 2560,
		.line_buffer_rgb = 2560,
		.line_buffer_yuv_ed = 2560,
	},
	{
		.name = "vch2_vsu8",
		.id = 2,
		.type = DE_SCALER_TYPE_VSU8,
		.line_buffer_yuv = 2048,
		.line_buffer_rgb = 2048,
		.line_buffer_yuv_ed = 2048,
	},
	{
		.name = "uch0_vsu8",
		.id = 3,
		.type = DE_SCALER_TYPE_VSU8,
		.line_buffer_yuv = 2560,
		.line_buffer_rgb = 2560,
		.line_buffer_yuv_ed = 2560,
	},
	{
		.name = "uch1_vsu8",
		.id = 4,
		.type = DE_SCALER_TYPE_VSU8,
		.line_buffer_yuv = 2560,
		.line_buffer_rgb = 2560,
		.line_buffer_yuv_ed = 2560,
	},
	{
		.name = "uch2_vsu8",
		.id = 5,
		.type = DE_SCALER_TYPE_VSU8,
		.line_buffer_yuv = 2048,
		.line_buffer_rgb = 2048,
		.line_buffer_yuv_ed = 2048,
	},
	{
		.name = "uch3_vsu8",
		.id = 6,
		.type = DE_SCALER_TYPE_VSU8,
		.line_buffer_yuv = 2048,
		.line_buffer_rgb = 2048,
		.line_buffer_yuv_ed = 2048,
	},
};

static struct de_version_scaler de350 = {
	.version = 0x350,
	.scaler_cnt = ARRAY_SIZE(de350_scalers),
	.scaler = de350_scalers,
};

static struct de_scaler_dsc de355_scalers[] = {
	{
		.name = "vch0_asu",
		.id = 0,
		.linebuff_share_ids = BIT(0) | BIT(1) | BIT(2),
		.type = DE_SCALER_TYPE_ASU,
		.line_buffer_yuv = 2048,
		.line_buffer_rgb = 2048,
		.line_buffer_yuv_ed = 2048,
	},
	{
		.name = "vch2_vsu8",
		.id = 1,
		.linebuff_share_ids = BIT(0) | BIT(1) | BIT(2),
		.type = DE_SCALER_TYPE_VSU8,
		.line_buffer_yuv = 2048,
		.line_buffer_rgb = 2048,
		.line_buffer_yuv_ed = 2048,
	},
	{
		.name = "uch0_vsu8",
		.id = 2,
		.linebuff_share_ids = BIT(0) | BIT(1) | BIT(2),
		.type = DE_SCALER_TYPE_VSU8,
		.line_buffer_yuv = 2048,
		.line_buffer_rgb = 2048,
		.line_buffer_yuv_ed = 2048,
	},
	{
		.name = "uch1_vsu8",
		.id = 3,
		.linebuff_share_ids = BIT(3) | BIT(4),
		.type = DE_SCALER_TYPE_VSU8,
		.line_buffer_yuv = 2048,
		.line_buffer_rgb = 2048,
		.line_buffer_yuv_ed = 2048,
	},
	{
		.name = "uch2_vsu8",
		.id = 4,
		.linebuff_share_ids = BIT(3) | BIT(4),
		.type = DE_SCALER_TYPE_VSU8,
		.line_buffer_yuv = 2048,
		.line_buffer_rgb = 2048,
		.line_buffer_yuv_ed = 2048,
	},
};

static struct de_version_scaler de355 = {
	.version = 0x355,
	.scaler_cnt = ARRAY_SIZE(de355_scalers),
	.scaler = de355_scalers,
};

static struct de_scaler_dsc de210_scalers[] = {
	{
		.name = "vch0_vsu10",
		.id = 0,
		.type = DE_SCALER_TYPE_VSU10,
		.line_buffer_yuv = 2048,
		.line_buffer_rgb = 2048,
		.line_buffer_yuv_ed = 2048,
	},
	{
		.name = "uch0_gsu",
		.id = 1,
		.type = DE_SCALER_TYPE_GSU,
		.line_buffer_yuv = 2048,
		.line_buffer_rgb = 2048,
		.line_buffer_yuv_ed = 2048,
	},
	{
		.name = "uch1_gsu",
		.id = 2,
		.type = DE_SCALER_TYPE_GSU,
		.line_buffer_yuv = 2048,
		.line_buffer_rgb = 2048,
		.line_buffer_yuv_ed = 2048,
	},
};

static struct de_version_scaler de210 = {
	.version = 0x210,
	.scaler_cnt = ARRAY_SIZE(de210_scalers),
	.scaler = de210_scalers,
};

static struct de_scaler_dsc de201_scalers[] = {
	{
		.name = "ch0v_vsu10",
		.id = 0,
		.offset = 0x120000,
		.need_switch_en = true,
		.type = DE_SCALER_TYPE_VSU10,
		.line_buffer_yuv = 2560,
		.line_buffer_rgb = 2560,
		.line_buffer_yuv_ed = 2560,
	},
	{
		.name = "ch1v_vsu10",
		.id = 1,
		.offset = 0x140000,
		.need_switch_en = true,
		.type = DE_SCALER_TYPE_VSU10,
		.line_buffer_yuv = 2048,
		.line_buffer_rgb = 2048,
		.line_buffer_yuv_ed = 2048,
	},
	{
		.name = "ch2u_gsu",
		.id = 2,
		.offset = 0x160000,
		.need_switch_en = true,
		.type = DE_SCALER_TYPE_GSU,
		.line_buffer_yuv = 2048,
		.line_buffer_rgb = 2048,
		.line_buffer_yuv_ed = 2048,
	},
	{
		.name = "ch3u_gsu",
		.id = 3,
		.offset = 0x170000,
		.need_switch_en = true,
		.type = DE_SCALER_TYPE_GSU,
		.line_buffer_yuv = 2048,
		.line_buffer_rgb = 2048,
		.line_buffer_yuv_ed = 2048,
	},
};

static struct de_version_scaler de201 = {
	.version = 0x201,
	.scaler_cnt = ARRAY_SIZE(de201_scalers),
	.scaler = de201_scalers,
};

static struct de_scaler_dsc de352_scalers[] = {
	{
		.name = "vch0_asu10",
		.id = 0,
		.type = DE_SCALER_TYPE_ASU,
		.line_buffer_yuv = 4096,
		.line_buffer_rgb = 4096, /* 2 line mode */
		.line_buffer_yuv_ed = 2048,
	},
	{
		.name = "vch1_vsu8",
		.id = 1,
		.type = DE_SCALER_TYPE_VSU8,
		.line_buffer_yuv = 2560,
		.line_buffer_rgb = 2560,
		.line_buffer_yuv_ed = 2560,
	},
	{
		.name = "vch2_vsu8",
		.id = 2,
		.type = DE_SCALER_TYPE_VSU8,
		.line_buffer_yuv = 2048,
		.line_buffer_rgb = 2048,
		.line_buffer_yuv_ed = 2048,
	},
	{
		.name = "uch0_vsu8",
		.id = 3,
		.type = DE_SCALER_TYPE_VSU8,
		.line_buffer_yuv = 2560,
		.line_buffer_rgb = 2560,
		.line_buffer_yuv_ed = 2560,
	},
	{
		.name = "uch1_vsu8",
		.id = 4,
		.type = DE_SCALER_TYPE_VSU8,
		.line_buffer_yuv = 2560,
		.line_buffer_rgb = 2560,
		.line_buffer_yuv_ed = 2560,
	},
	{
		.name = "uch2_vsu8",
		.id = 5,
		.type = DE_SCALER_TYPE_VSU8,
		.line_buffer_yuv = 2048,
		.line_buffer_rgb = 2048,
		.line_buffer_yuv_ed = 2048,
	},
	{
		.name = "uch3_vsu8",
		.id = 6,
		.type = DE_SCALER_TYPE_VSU8,
		.line_buffer_yuv = 2048,
		.line_buffer_rgb = 2048,
		.line_buffer_yuv_ed = 2048,
	},
};

static struct de_version_scaler de352 = {
	.version = 0x352,
	.scaler_cnt = ARRAY_SIZE(de352_scalers),
	.scaler = de352_scalers,
};

static struct de_version_scaler *de_version[] = {
	&de350, &de355, &de210, &de201, &de352,
};

struct de_scaler_dsc *get_scaler_dsc(const struct module_create_info *info)
{
	int i, j;
	for (i = 0; i < ARRAY_SIZE(de_version); i++) {
		if (de_version[i]->version == info->de_version) {
			for (j = 0; j < de_version[i]->scaler_cnt; j++) {
				if (de_version[i]->scaler[j].id == info->id)
					return &de_version[i]->scaler[j];
			}
		}
	}
	return NULL;
}
