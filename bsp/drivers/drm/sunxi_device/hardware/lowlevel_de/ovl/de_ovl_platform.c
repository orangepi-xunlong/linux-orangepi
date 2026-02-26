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

#include "de_ovl_platform.h"

#define DE_CHN_SIZE				(0x20000)
#define DE_CHN_OFFSET_BASE			(0x100000)
#define DE_DISP_SIZE_V210			(0x100000)
#define DE_CHN_BASE_OFFSET(base, disp, disp_size, chn, chn_size) \
			    ((base) + (disp) * (disp_size) + (chn) * (chn_size))

struct de_version_ovl {
	unsigned int version;
	unsigned int ovl_cnt;
	struct de_ovl_dsc *ovl;
};

static struct de_ovl_dsc de350_ovls[] = {
	{
		.name = "vch0",
		.id = 0,
		.type_hw_id = 0,
		.layer_cnt = 4,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, 0, 0, DE_CHN_SIZE),
		.type = OVL_TYPE_VI,
	},
	{
		.name = "vch1",
		.id = 1,
		.type_hw_id = 1,
		.layer_cnt = 4,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, 0, 1, DE_CHN_SIZE),
		.type = OVL_TYPE_VI,
	},
	{
		.name = "vch2",
		.id = 2,
		.type_hw_id = 2,
		.layer_cnt = 4,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, 0, 2, DE_CHN_SIZE),
		.type = OVL_TYPE_VI,
	},
	{
		.name = "uch0",
		.id = 3,
		.type_hw_id = 0,
		.layer_cnt = 4,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, 0, 6, DE_CHN_SIZE),
		.type = OVL_TYPE_UI,
	},
	{
		.name = "uch1",
		.id = 4,
		.type_hw_id = 1,
		.layer_cnt = 4,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, 0, 7, DE_CHN_SIZE),
		.type = OVL_TYPE_UI,
	},
	{
		.name = "uch2",
		.id = 5,
		.type_hw_id = 2,
		.layer_cnt = 4,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, 0, 8, DE_CHN_SIZE),
		.type = OVL_TYPE_UI,
	},
	{
		.name = "uch3",
		.id = 6,
		.type_hw_id = 3,
		.layer_cnt = 4,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, 0, 9, DE_CHN_SIZE),
		.type = OVL_TYPE_UI,
	},
};

static struct de_version_ovl de350 = {
	.version = 0x350,
	.ovl_cnt = ARRAY_SIZE(de350_ovls),
	.ovl = de350_ovls,
};

static struct de_ovl_dsc de355_ovls[] = {
	{
		.name = "vch0",
		.id = 0,
		.type_hw_id = 0,
		.layer_cnt = 4,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, 0, 0, DE_CHN_SIZE),
		.type = OVL_TYPE_VI,
	},
	{
		.name = "vch2",
		.id = 1,
		.type_hw_id = 2,
		.layer_cnt = 4,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, 0, 2, DE_CHN_SIZE),
		.type = OVL_TYPE_VI,
	},
	{
		.name = "uch0",
		.id = 2,
		.type_hw_id = 0,
		.layer_cnt = 2,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, 0, 6, DE_CHN_SIZE),
		.type = OVL_TYPE_UI,
	},
	{
		.name = "uch1",
		.id = 3,
		.type_hw_id = 1,
		.layer_cnt = 2,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, 0, 7, DE_CHN_SIZE),
		.type = OVL_TYPE_UI,
	},
	{
		.name = "uch2",
		.id = 4,
		.type_hw_id = 2,
		.layer_cnt = 2,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, 0, 8, DE_CHN_SIZE),
		.type = OVL_TYPE_UI,
	},
};

static struct de_version_ovl de355 = {
	.version = 0x355,
	.ovl_cnt = ARRAY_SIZE(de355_ovls),
	.ovl = de355_ovls,
};

static struct de_ovl_dsc de210_ovls[] = {
	{
		.name = "vch0",
		.id = 0,
		.type_hw_id = 0,
		.layer_cnt = 4,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, DE_DISP_SIZE_V210, 0, DE_CHN_SIZE),
		.type = OVL_TYPE_VI,
	},
	{
		.name = "uch0",
		.id = 1,
		.type_hw_id = 0,
		.layer_cnt = 4,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, DE_DISP_SIZE_V210, 3, DE_CHN_SIZE),
		.type = OVL_TYPE_UI,
	},
	{
		.name = "uch1",
		.id = 2,
		.type_hw_id = 1,
		.layer_cnt = 4,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, DE_DISP_SIZE_V210, 4, DE_CHN_SIZE),
		.type = OVL_TYPE_UI,
	},
};

static struct de_version_ovl de210 = {
	.version = 0x210,
	.ovl_cnt = ARRAY_SIZE(de210_ovls),
	.ovl = de210_ovls,
};

static struct de_ovl_dsc de201_ovls[] = {
	{
		.name = "ch0v",
		.id = 0,
		.type_hw_id = 0,
		.layer_cnt = 4,
		.type = OVL_TYPE_VI,
		.ovl_offset = 0x102000,
	},
	{
		.name = "ch1v",
		.id = 1,
		.type_hw_id = 1,
		.layer_cnt = 4,
		.type = OVL_TYPE_VI,
		.ovl_offset = 0x103000,
	},
	{
		.name = "ch2u",
		.id = 2,
		.type_hw_id = 0,
		.layer_cnt = 4,
		.type = OVL_TYPE_UI,
		.ovl_offset = 0x104000,
	},
	{
		.name = "ch3u",
		.id = 3,
		.type_hw_id = 1,
		.layer_cnt = 4,
		.type = OVL_TYPE_UI,
		.ovl_offset = 0x105000,
	},

};

static struct de_version_ovl de201 = {
	.version = 0x201,
	.ovl_cnt = ARRAY_SIZE(de201_ovls),
	.ovl = de201_ovls,
};

static struct de_ovl_dsc de352_ovls[] = {
	{
		.name = "vch0",
		.id = 0,
		.type_hw_id = 0,
		.layer_cnt = 4,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, 0, 0, DE_CHN_SIZE),
		.type = OVL_TYPE_VI,
	},
	{
		.name = "vch1",
		.id = 1,
		.type_hw_id = 1,
		.layer_cnt = 4,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, 0, 1, DE_CHN_SIZE),
		.type = OVL_TYPE_VI,
	},
	{
		.name = "vch2",
		.id = 2,
		.type_hw_id = 2,
		.layer_cnt = 4,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, 0, 2, DE_CHN_SIZE),
		.type = OVL_TYPE_VI,
	},
	{
		.name = "uch0",
		.id = 3,
		.type_hw_id = 0,
		.layer_cnt = 4,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, 0, 6, DE_CHN_SIZE),
		.type = OVL_TYPE_UI,
	},
	{
		.name = "uch1",
		.id = 4,
		.type_hw_id = 1,
		.layer_cnt = 4,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, 0, 7, DE_CHN_SIZE),
		.type = OVL_TYPE_UI,
	},
	{
		.name = "uch2",
		.id = 5,
		.type_hw_id = 2,
		.layer_cnt = 4,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, 0, 8, DE_CHN_SIZE),
		.type = OVL_TYPE_UI,
	},
	{
		.name = "uch3",
		.id = 6,
		.type_hw_id = 3,
		.layer_cnt = 4,
		.channel_base = DE_CHN_BASE_OFFSET(DE_CHN_OFFSET_BASE,
					0, 0, 9, DE_CHN_SIZE),
		.type = OVL_TYPE_UI,
	},
};

static struct de_version_ovl de352 = {
	.version = 0x352,
	.ovl_cnt = ARRAY_SIZE(de352_ovls),
	.ovl = de352_ovls,
};

static struct de_version_ovl *de_version[] = {
	&de350, &de355, &de210, &de201, &de352,
};

const struct de_ovl_dsc *get_ovl_dsc(struct module_create_info *info)
{
	int i, j;
	for (i = 0; i < ARRAY_SIZE(de_version); i++) {
		if (de_version[i]->version == info->de_version) {
			for (j = 0; j < de_version[i]->ovl_cnt; j++) {
				if (de_version[i]->ovl[j].id == info->id)
					return &de_version[i]->ovl[j];
			}
		}
	}
	return NULL;
}

