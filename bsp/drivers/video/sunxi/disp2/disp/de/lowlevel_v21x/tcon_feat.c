/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "tcon_feat.h"

struct tcon_feat {

	/* indicate timing controller number */
	const int num_devices;

	/* indicate dsi module number */
	const int num_dsi_devices;

	const int *supported_output_types;

	const int *tcon_output_types;
};


#if defined(CONFIG_ARCH_SUN55IW6)
static const int sun55iw6_de_supported_output_types[] = {
	DE_OUTPUT_TYPE_LCD,	/* TCON0 */
};

static const int sun55iw6_tcon_lcd_output_types[] = {
	/* TCON0 */
	TCON_TO_RGB | TCON_TO_LVDS | TCON_TO_SINGLE_DSI,
};

static const struct tcon_feat sun55iw6_de_features = {
	.num_devices = DEVICE_NUM,
	.num_dsi_devices = DEVICE_DSI_NUM,
	.supported_output_types = sun55iw6_de_supported_output_types,
	.tcon_output_types = sun55iw6_tcon_lcd_output_types,
};

static const struct tcon_feat *tcon_cur_features = &sun55iw6_de_features;
#endif

int de_feat_get_num_devices(void)
{
	return tcon_cur_features->num_devices;

}

int de_feat_get_num_dsi(void)
{
	return tcon_cur_features->num_dsi_devices;
}

int de_feat_is_supported_output_types(unsigned int tcon_id,
	unsigned int output_type)
{
	return tcon_cur_features->supported_output_types[tcon_id] & output_type;
}

int is_tcon_support_output_types(unsigned int tcon_id, unsigned int output_type)
{
	return tcon_cur_features->tcon_output_types[tcon_id] & output_type;
}
