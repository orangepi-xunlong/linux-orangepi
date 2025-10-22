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

#include "de_bld_platform.h"

#define VIDEO_CHANNEL_ID(ch)		(BIT(ch) << VIDEO_CHANNEL_ID_SHIFT)
#define UI_CHANNEL_ID(ch)		(BIT(ch) << UI_CHANNEL_ID_SHIFT)
#define DE_DISP_BASE_OFFSET(base, disp, disp_size)			    \
					((base) + (disp) * (disp_size))

#define BLD_OFFSET_V3XX			(0x1000)
#define DISP_BASE_V3XX			(0x280000)
#define DE_DISP_SIZE_V3XX		(0x20000)

#define BLD_OFFSET_V2XX			(0x1000)
#define DISP_BASE_V2XX			(0x1C0000)
#define DE_DISP_SIZE_V2XX		(0x100000)

struct de_version_bld {
	unsigned int version;
	unsigned int bld_cnt;
	struct de_bld_desc **bld;
};

/* 2 bld, 3 mode */
static const struct de_bld_port_mux_mode de350_modes[2][3] = {
	{
		{
			.name = "v0v1v2u0u1u2",
			.mode_id = 0,
			.channel_cnt = 6,
			.channel_id[0] = VIDEO_CHANNEL_ID(0),
			.channel_id[1] = VIDEO_CHANNEL_ID(1),
			.channel_id[2] = VIDEO_CHANNEL_ID(2),
			.channel_id[3] = UI_CHANNEL_ID(0),
			.channel_id[4] = UI_CHANNEL_ID(1),
			.channel_id[5] = UI_CHANNEL_ID(2),
		},
		{
			.name = "v0u0u1u2",
			.mode_id = 1,
			.channel_cnt = 4,
			.channel_id[0] = VIDEO_CHANNEL_ID(0),
			.channel_id[1] = UI_CHANNEL_ID(0),
			.channel_id[2] = UI_CHANNEL_ID(1),
			.channel_id[3] = UI_CHANNEL_ID(2),
		},
		{
			.name = "v0v1u0u1",
			.mode_id = 3,
			.channel_cnt = 4,
			.channel_id[0] = VIDEO_CHANNEL_ID(0),
			.channel_id[1] = VIDEO_CHANNEL_ID(1),
			.channel_id[2] = UI_CHANNEL_ID(0),
			.channel_id[3] = UI_CHANNEL_ID(1),
		},
	},
	{
		{
			.name = "u3",
			.mode_id = 0,
			.channel_cnt = 1,
			.channel_id[0] = UI_CHANNEL_ID(3),
		},

		{
			.name = "v2v1u3",
			.mode_id = 1,
			.channel_cnt = 3,
			.channel_id[0] = VIDEO_CHANNEL_ID(2),
			.channel_id[1] = VIDEO_CHANNEL_ID(1),
			.channel_id[2] = UI_CHANNEL_ID(3),
		},
		{
			.name = "v2u2u3",
			.mode_id = 3,
			.channel_cnt = 3,
			.channel_id[0] = VIDEO_CHANNEL_ID(2),
			.channel_id[1] = UI_CHANNEL_ID(2),
			.channel_id[2] = UI_CHANNEL_ID(3),
		},
	},
};

static struct de_bld_desc de350_bld0 = {
	.name = "blender0",
	.id = 0,
	.disp_base = DE_DISP_BASE_OFFSET(DISP_BASE_V3XX, 0, DE_DISP_SIZE_V3XX),
	.bld_offset = BLD_OFFSET_V3XX,
	.mode_cnt = ARRAY_SIZE(de350_modes[0]),
	.mode = &de350_modes[0][0],
};

static struct de_bld_desc de350_bld1 = {
	.name = "blender1",
	.id = 1,
	.disp_base = DE_DISP_BASE_OFFSET(DISP_BASE_V3XX, 1, DE_DISP_SIZE_V3XX),
	.bld_offset = BLD_OFFSET_V3XX,
	.mode_cnt = ARRAY_SIZE(de350_modes[1]),
	.mode = &de350_modes[1][0],
};

static struct de_bld_desc *de350_blds[] = {
	&de350_bld0, &de350_bld1,
};

static struct de_version_bld de350 = {
	.version = 0x350,
	.bld_cnt = ARRAY_SIZE(de350_blds),
	.bld = &de350_blds[0],
};


/* 2 bld, 2 mode */
static const struct de_bld_port_mux_mode de355_modes[2][2] = {
	{
		{
			.name = "v0v2u0u1u2",
			.mode_id = 0,
			.channel_cnt = 5,
			.channel_id[0] = VIDEO_CHANNEL_ID(0),
			.channel_id[1] = VIDEO_CHANNEL_ID(2),
			.channel_id[2] = UI_CHANNEL_ID(0),
			.channel_id[3] = UI_CHANNEL_ID(1),
			.channel_id[4] = UI_CHANNEL_ID(2),
		},
		{
			.name = "v0u0u1",
			.mode_id = 1,
			.channel_cnt = 3,
			.channel_id[0] = VIDEO_CHANNEL_ID(0),
			.channel_id[1] = UI_CHANNEL_ID(0),
			.channel_id[2] = UI_CHANNEL_ID(1),
		},
	},
	{
		{
			// single display is null
			.name = "null",
			.mode_id = 0,
			.channel_cnt = 0,
		},
		{
			.name = "v2u2",
			.mode_id = 1,
			.channel_cnt = 2,
			.channel_id[0] = VIDEO_CHANNEL_ID(2),
			.channel_id[1] = UI_CHANNEL_ID(2),
		},
	},
};

static struct de_bld_desc de355_bld0 = {
	.name = "blender0",
	.id = 0,
	.disp_base = DE_DISP_BASE_OFFSET(DISP_BASE_V3XX, 0, DE_DISP_SIZE_V3XX),
	.bld_offset = BLD_OFFSET_V3XX,
	.mode_cnt = ARRAY_SIZE(de355_modes[0]),
	.mode = &de355_modes[0][0],
};

static struct de_bld_desc de355_bld1 = {
	.name = "blender1",
	.id = 1,
	.disp_base = DE_DISP_BASE_OFFSET(DISP_BASE_V3XX, 1, DE_DISP_SIZE_V3XX),
	.bld_offset = BLD_OFFSET_V3XX,
	.mode_cnt = ARRAY_SIZE(de355_modes[1]),
	.mode = &de355_modes[1][0],
};

static struct de_bld_desc *de355_blds[] = {
	&de355_bld0, &de355_bld1,
};

static struct de_version_bld de355 = {
	.version = 0x355,
	.bld_cnt = ARRAY_SIZE(de355_blds),
	.bld = &de355_blds[0],
};

/* 1 bld, 1 fake mode */
static const struct de_bld_port_mux_mode de210_modes[1][1] = {
	{
		{
			.name = "v0u0u1",
			.mode_id = CHN_MODE_FIX,
			.channel_cnt = 3,
			.channel_id[0] = VIDEO_CHANNEL_ID(0),
			.channel_id[1] = UI_CHANNEL_ID(0),
			.channel_id[2] = UI_CHANNEL_ID(1),
		},
	},
};

static struct de_bld_desc de210_bld0 = {
	.name = "blender0",
	.id = 0,
	.disp_base = DE_DISP_BASE_OFFSET(DISP_BASE_V2XX, 0, DE_DISP_SIZE_V2XX),
	.bld_offset = BLD_OFFSET_V2XX,
	.mode_cnt = ARRAY_SIZE(de210_modes[0]),
	.mode = &de210_modes[0][0],
};

static struct de_bld_desc *de210_blds[] = {
	&de210_bld0,
};

static struct de_version_bld de210 = {
	.version = 0x210,
	.bld_cnt = ARRAY_SIZE(de210_blds),
	.bld = &de210_blds[0],
};

/* 1 bld, 1 fake mode */
static const struct de_bld_port_mux_mode de201_modes[1][1] = {
	{
		{
			.name = "v0v1u0u1",
			.mode_id = CHN_MODE_FIX,
			.channel_cnt = 4,
			.channel_id[0] = VIDEO_CHANNEL_ID(0),
			.channel_id[1] = VIDEO_CHANNEL_ID(1),
			.channel_id[2] = UI_CHANNEL_ID(0),
			.channel_id[3] = UI_CHANNEL_ID(1),
		},
	},
};

static struct de_bld_desc de201_bld0 = {
	.name = "blender0",
	.id = 0,
	.bld_offset = BLD_OFFSET_V2XX + 0x100000,
	.mode_cnt = ARRAY_SIZE(de201_modes[0]),
	.mode = &de201_modes[0][0],
};

static struct de_bld_desc *de201_blds[] = {
	&de201_bld0,
};

static struct de_version_bld de201 = {
	.version = 0x201,
	.bld_cnt = ARRAY_SIZE(de201_blds),
	.bld = &de201_blds[0],
};

/* 2 bld, 3 mode */
static const struct de_bld_port_mux_mode de352_modes[2][3] = {
	{
		{
			.name = "v0v1v2u0u1u2",
			.mode_id = 0,
			.channel_cnt = 6,
			.channel_id[0] = VIDEO_CHANNEL_ID(0),
			.channel_id[1] = VIDEO_CHANNEL_ID(1),
			.channel_id[2] = VIDEO_CHANNEL_ID(2),
			.channel_id[3] = UI_CHANNEL_ID(0),
			.channel_id[4] = UI_CHANNEL_ID(1),
			.channel_id[5] = UI_CHANNEL_ID(2),
		},
		{
			.name = "v0v1u0u1",
			.mode_id = 1,
			.channel_cnt = 4,
			.channel_id[0] = VIDEO_CHANNEL_ID(0),
			.channel_id[1] = VIDEO_CHANNEL_ID(1),
			.channel_id[2] = UI_CHANNEL_ID(0),
			.channel_id[3] = UI_CHANNEL_ID(1),
		},
		{
			.name = "v0v1u0u2",
			.mode_id = 2,
			.channel_cnt = 4,
			.channel_id[0] = VIDEO_CHANNEL_ID(0),
			.channel_id[1] = VIDEO_CHANNEL_ID(1),
			.channel_id[2] = UI_CHANNEL_ID(0),
			.channel_id[3] = UI_CHANNEL_ID(2),
		},
	},
	{
		{
			.name = "u3",
			.mode_id = 0,
			.channel_cnt = 1,
			.channel_id[0] = UI_CHANNEL_ID(3),
		},

		{
			.name = "v2u2u3",
			.mode_id = 1,
			.channel_cnt = 3,
			.channel_id[0] = VIDEO_CHANNEL_ID(2),
			.channel_id[1] = UI_CHANNEL_ID(2),
			.channel_id[2] = UI_CHANNEL_ID(3),
		},
		{
			.name = "v2u1u3",
			.mode_id = 2,
			.channel_cnt = 3,
			.channel_id[0] = VIDEO_CHANNEL_ID(2),
			.channel_id[1] = UI_CHANNEL_ID(1),
			.channel_id[2] = UI_CHANNEL_ID(3),
		},
	},
};

static struct de_bld_desc de352_bld0 = {
	.name = "blender0",
	.id = 0,
	.disp_base = DE_DISP_BASE_OFFSET(DISP_BASE_V3XX, 0, DE_DISP_SIZE_V3XX),
	.bld_offset = BLD_OFFSET_V3XX,
	.mode_cnt = ARRAY_SIZE(de352_modes[0]),
	.mode = &de352_modes[0][0],
};

static struct de_bld_desc de352_bld1 = {
	.name = "blender1",
	.id = 1,
	.disp_base = DE_DISP_BASE_OFFSET(DISP_BASE_V3XX, 1, DE_DISP_SIZE_V3XX),
	.bld_offset = BLD_OFFSET_V3XX,
	.mode_cnt = ARRAY_SIZE(de352_modes[1]),
	.mode = &de352_modes[1][0],
};

static struct de_bld_desc *de352_blds[] = {
	&de352_bld0, &de352_bld1,
};

static struct de_version_bld de352 = {
	.version = 0x352,
	.bld_cnt = ARRAY_SIZE(de352_blds),
	.bld = &de352_blds[0],
};


static struct de_version_bld *de_version[] = {
	&de350, &de355, &de210, &de201, &de352,
};

const struct de_bld_desc *get_bld_dsc(struct module_create_info *info)
{
	int i, j;
	for (i = 0; i < ARRAY_SIZE(de_version); i++) {
		if (de_version[i]->version == info->de_version) {
			for (j = 0; j < de_version[i]->bld_cnt; j++) {
				if (de_version[i]->bld[j]->id == info->id)
					return de_version[i]->bld[j];
			}
		}
	}
	return NULL;
}
