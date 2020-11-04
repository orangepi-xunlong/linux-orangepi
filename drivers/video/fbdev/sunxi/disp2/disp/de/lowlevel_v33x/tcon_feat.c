/*
 * drivers/video/fbdev/sunxi/disp2/disp/de/lowlevel_v33x/tcon_feat/tcon_feat.c
 *
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

	/*indicate timing controller number */
	const int num_devices;

	/*indicate dsi module number */
	const int num_dsi_devices;

	const int *supported_output_types;
};


#if defined(CONFIG_ARCH_SUN50IW5T)
static const int sun50iw6_de_supported_output_types[] = {
	DE_OUTPUT_TYPE_LCD,
	DE_OUTPUT_TYPE_HDMI,
};

static const struct tcon_feat sun50iw6_de_features = {
	.num_devices = DEVICE_NUM,
	.num_dsi_devices = DEVICE_DSI_NUM,
	.supported_output_types = sun50iw6_de_supported_output_types,
};

static const struct tcon_feat *tcon_cur_features = &sun50iw6_de_features;
#elif defined(CONFIG_ARCH_SUN50IW9)
static const int sun50iw9_de_supported_output_types[] = {
	DE_OUTPUT_TYPE_LCD,
	DE_OUTPUT_TYPE_LCD,
	DE_OUTPUT_TYPE_HDMI,
	DE_OUTPUT_TYPE_TV,
};

static const struct tcon_feat sun50iw9_de_features = {
	.num_devices = DEVICE_NUM,
	.num_dsi_devices = DEVICE_DSI_NUM,
	.supported_output_types = sun50iw9_de_supported_output_types,
};

static const struct tcon_feat *tcon_cur_features = &sun50iw9_de_features;
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

