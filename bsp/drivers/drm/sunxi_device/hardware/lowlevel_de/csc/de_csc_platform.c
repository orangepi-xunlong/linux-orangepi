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

#include "de_csc_platform.h"

#define CHN_CCSC_OFFSET				(0x00800)
#define CSC_OFFSET_V2				(0x00100)

struct de_version_csc {
	unsigned int version;
	unsigned int csc_cnt;
	struct de_csc_desc *csc;
};

static struct de_csc_desc de350_cscs[] = {
	{
		.name = "vch0_csc",
		.cid.channel_id = 0,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CHN_CCSC_OFFSET,
	},
	{
		.name = "vch0_fcm_icsc",
		.cid.channel_id = 0,
		.cid.csc_id = 0,
		.type = FCM_CSC,
	},
	{
		.name = "vch0_fcm_ocsc",
		.cid.channel_id = 0,
		.cid.csc_id = 1,
		.type = FCM_CSC,
	},
	{
		.name = "vch1_csc",
		.cid.channel_id = 1,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CHN_CCSC_OFFSET,
	},
	{
		.name = "vch2_csc",
		.cid.channel_id = 2,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CHN_CCSC_OFFSET,
	},
	{
		.name = "vch2_fcm_icsc",
		.cid.channel_id = 2,
		.cid.csc_id = 0,
		.type = FCM_CSC,
	},
	{
		.name = "vch2_fcm_ocsc",
		.cid.channel_id = 2,
		.cid.csc_id = 1,
		.type = FCM_CSC,
	},
	{
		.name = "vch0_cdc_icsc",
		.cid.channel_id = 0,
		.cid.csc_id = 0,
		.type = CDC_CSC,
	},
	{
		.name = "vch0_cdc_ocsc",
		.cid.channel_id = 0,
		.cid.csc_id = 1,
		.type = CDC_CSC,
	},
	{
		.name = "uch0_csc",
		.cid.channel_id = 3,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CHN_CCSC_OFFSET,
	},
	{
		.name = "uch1_csc",
		.cid.channel_id = 4,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CHN_CCSC_OFFSET,
	},
	{
		.name = "uch2_csc",
		.cid.channel_id = 5,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CHN_CCSC_OFFSET,
	},
	{
		.name = "uch3_csc",
		.cid.channel_id = 6,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CHN_CCSC_OFFSET,
	},
};

static struct de_version_csc de350 = {
	.version = 0x350,
	.csc_cnt = ARRAY_SIZE(de350_cscs),
	.csc = de350_cscs,
};

static struct de_csc_desc de355_cscs[] = {
	{
		.name = "vch0_csc1",
		.version = 2,
		.cid.channel_id = 0,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CSC_OFFSET_V2,
	},
	{
		.name = "vch0_csc2",
		.version = 2,
		.cid.channel_id = 0,
		.cid.csc_id = 2,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CSC_OFFSET_V2 * 2,
	},
	{
		.name = "vch2_csc",
		.version = 2,
		.cid.channel_id = 1,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CSC_OFFSET_V2,
	},
	{
		.name = "uch0_csc",
		.version = 2,
		.cid.channel_id = 2,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CSC_OFFSET_V2,
	},
	{
		.name = "uch1_csc",
		.version = 2,
		.cid.channel_id = 3,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CSC_OFFSET_V2,
	},
	{
		.name = "uch2_csc",
		.version = 2,
		.cid.channel_id = 4,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CSC_OFFSET_V2,
	},
	{
		.name = "dep0_csc",
		.version = 2,
		.cid.channel_id = 0,
		.did.device_id = 0,
		.csc_bit_width = 10,
		.hue_default_value = 0,
		.type = DEVICE_CSC,
		.reg_offset = CSC_OFFSET_V2,
	},
	{
		.name = "dep1_csc",
		.version = 2,
		.cid.channel_id = 0,
		.did.device_id = 1,
		.csc_bit_width = 10,
		.hue_default_value = 0,
		.type = DEVICE_CSC,
		.reg_offset = CSC_OFFSET_V2,
	},
};

static struct de_version_csc de355 = {
	.version = 0x355,
	.csc_cnt = ARRAY_SIZE(de355_cscs),
	.csc = de355_cscs,
};

static struct de_csc_desc de210_cscs[] = {
	{
		.name = "vch0_csc1",
		.version = 2,
		.cid.channel_id = 0,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CSC_OFFSET_V2,
	},
	{
		.name = "uch0_csc1",
		.version = 2,
		.cid.channel_id = 1,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CSC_OFFSET_V2,
	},
	{
		.name = "uch1_csc",
		.version = 2,
		.cid.channel_id = 2,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CSC_OFFSET_V2,
	},
	{
		.name = "disp0_gamma_csc",
		.cid.channel_id = 0,
		.did.device_id = 0,
		.csc_bit_width = 8,
		.hue_default_value = 50,
		.type = GAMMA_CSC,
	},
};

static struct de_version_csc de210 = {
	.version = 0x210,
	.csc_cnt = ARRAY_SIZE(de210_cscs),
	.csc = de210_cscs,
};

static struct de_csc_desc de201_cscs[] = {
	{
		.name = "vch0_csc1",
		.version = 1,
		.cid.channel_id = 0,
		.cid.csc_id = 1,
		.csc_bit_width = 8,
		.type = CHANNEL_CSC,
		.reg_offset = 0x1AA050,
	},
	{
		.name = "vch1_csc1",
		.version = 1,
		.cid.channel_id = 1,
		.cid.csc_id = 1,
		.csc_bit_width = 8,
		.type = CHANNEL_CSC,
		.reg_offset = 0x1FA000,
	},
	{
		.name = "drc_csc",
		.version = 1,
		.csc_bit_width = 8,
		.hue_default_value = 50,
		.type = SMBL_CSC,//FIXME
	},
};

static struct de_version_csc de201 = {
	.version = 0x201,
	.csc_cnt = ARRAY_SIZE(de201_cscs),
	.csc = de201_cscs,
};

static struct de_csc_desc de352_cscs[] = {
	{
		.name = "vch0_csc",
		.cid.channel_id = 0,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CHN_CCSC_OFFSET,
	},
	{
		.name = "vch0_fcm_icsc",
		.cid.channel_id = 0,
		.cid.csc_id = 0,
		.csc_bit_width = 10,
		.type = FCM_CSC,
	},
	{
		.name = "vch0_fcm_ocsc",
		.cid.channel_id = 0,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = FCM_CSC,
	},
	{
		.name = "vch0_cdc_icsc",
		.cid.channel_id = 0,
		.cid.csc_id = 0,
		.type = CDC_CSC,
	},
	{
		.name = "vch0_cdc_ocsc",
		.cid.channel_id = 0,
		.cid.csc_id = 1,
		.type = CDC_CSC,
	},
	{
		.name = "vch1_csc",
		.cid.channel_id = 1,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CHN_CCSC_OFFSET,
	},
	{
		.name = "vch2_csc",
		.cid.channel_id = 2,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CHN_CCSC_OFFSET,
	},
	{
		.name = "vch2_fcm_icsc",
		.cid.channel_id = 2,
		.cid.csc_id = 0,
		.csc_bit_width = 10,
		.type = FCM_CSC,
	},
	{
		.name = "vch2_fcm_ocsc",
		.cid.channel_id = 2,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = FCM_CSC,
	},
	{
		.name = "uch0_csc",
		.cid.channel_id = 3,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CHN_CCSC_OFFSET,
	},
	{
		.name = "uch1_csc",
		.cid.channel_id = 4,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CHN_CCSC_OFFSET,
	},
	{
		.name = "uch2_csc",
		.cid.channel_id = 5,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CHN_CCSC_OFFSET,
	},
	{
		.name = "uch3_csc",
		.cid.channel_id = 6,
		.cid.csc_id = 1,
		.csc_bit_width = 10,
		.type = CHANNEL_CSC,
		.reg_offset = CHN_CCSC_OFFSET,
	},
	{
		.name = "disp0_gamma_csc",
		.cid.channel_id = 0,
		.did.device_id = 0,
		.csc_bit_width = 10,
		.hue_default_value = 50,
		.type = GAMMA_CSC,
	},
	{
		.name = "disp1_gamma_csc",
		.cid.channel_id = 0,
		.did.device_id = 1,
		.csc_bit_width = 10,
		.hue_default_value = 50,
		.type = GAMMA_CSC,
	},
};

static struct de_version_csc de352 = {
	.version = 0x352,
	.csc_cnt = ARRAY_SIZE(de352_cscs),
	.csc = de352_cscs,
};

static struct de_version_csc *de_version[] = {
	&de350, &de355, &de210, &de201, &de352,
};

const struct de_csc_desc *get_csc_desc(struct module_create_info *info)
{
	int i, j;
	struct csc_extra_create_info *ex = info->extra;
	if (IS_ERR_OR_NULL(ex))
		return NULL;

	for (i = 0; i < ARRAY_SIZE(de_version); i++) {
		if (de_version[i]->version == info->de_version) {
			for (j = 0; j < de_version[i]->csc_cnt; j++) {
				if (ex->type == de_version[i]->csc[j].type) {
					if (ex->type == CHANNEL_CSC || ex->type == FCM_CSC) {
						if (de_version[i]->csc[j].cid.channel_id == info->id &&
						    de_version[i]->csc[j].cid.csc_id == ex->extra_id)
							return &de_version[i]->csc[j];
					} else {
						if (de_version[i]->csc[j].did.device_id == info->id)
							return &de_version[i]->csc[j];
					}
				}
			}
		}
	}
	return NULL;
}
