/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DISP_FEATURES_H_
#define _DISP_FEATURES_H_

/* #include "include.h" */
#if defined(DE_VERSION_V33X) || defined(CONFIG_ARCH_SUN50IW9)
#include "./lowlevel_v33x/de330/de_feat.h"
#include "./lowlevel_v33x/tcon_feat.h"
#elif defined(DE_VERSION_V35X) || defined(CONFIG_ARCH_SUN55IW3)
#include "./lowlevel_v35x/de35x/de_feat.h"
#include "./lowlevel_v35x/tcon_feat.h"
#elif defined(DE_VERSION_V21X)
#include "./lowlevel_v21x/de21x/de_feat.h"
#include "./lowlevel_v21x/tcon_feat.h"
#elif defined(CONFIG_ARCH_SUN8IW6)
#include "lowlevel_v2x/de_feat.h"
#elif defined(CONFIG_ARCH_SUN8IW7)
#include "lowlevel_v2x/de_feat.h"
#elif defined(CONFIG_ARCH_SUN8IW8)
#include "lowlevel_sun8iw8/de_feat.h"
#elif defined(CONFIG_ARCH_SUN8IW9)
#include "lowlevel_sun8iw9/de_feat.h"
#elif defined(CONFIG_ARCH_SUN8IW10)
#include "./lowlevel_sun8iw10/de_feat.h"
#elif defined(CONFIG_ARCH_SUN8IW11)
#include "./lowlevel_v2x/de_feat.h"
#elif defined(CONFIG_ARCH_SUN50IW1)
#include "./lowlevel_sun50iw1/de_feat.h"
#elif defined(CONFIG_ARCH_SUN50IW2)
#include "./lowlevel_v2x/de_feat.h"
#elif defined(CONFIG_ARCH_SUN50IW8)
#include "./lowlevel_v2x/de_feat.h"
#elif defined(CONFIG_ARCH_SUN8IW12) || defined(CONFIG_ARCH_SUN8IW16)\
	|| defined(CONFIG_ARCH_SUN8IW19) || defined(CONFIG_ARCH_SUN8IW20)\
	|| defined(CONFIG_ARCH_SUN20IW1)
#include "./lowlevel_v2x/de_feat.h"
#elif defined(CONFIG_ARCH_SUN8IW15) || defined(CONFIG_ARCH_SUN8IW17)
#include "./lowlevel_v2x/de_feat.h"
#elif defined(CONFIG_ARCH_SUN50IW10)
#include "./lowlevel_v2x/de_feat.h"
#elif defined(CONFIG_ARCH_SUN50IW3) || defined(CONFIG_ARCH_SUN50IW6)
#include "./lowlevel_v3x/de_feat.h"
#else
#error "undefined platform!!!"
#endif

#define DISP_DEVICE_NUM DEVICE_NUM
#define DISP_SCREEN_NUM DE_NUM
#define DISP_WB_NUM DE_NUM

struct disp_features {
	const int num_screens;
	const int *num_channels;
	const int *num_layers;
	const int *is_support_capture;
	const int *supported_output_types;
};

struct disp_feat_init {
	unsigned int chn_cfg_mode;
};

int bsp_disp_feat_get_num_screens(void);
int bsp_disp_feat_get_num_devices(void);
int bsp_disp_feat_get_num_channels(unsigned int disp);
int bsp_disp_feat_get_num_layers(unsigned int screen_id);
int bsp_disp_feat_get_num_layers_by_chn(unsigned int disp, unsigned int chn);
int bsp_disp_feat_is_supported_output_types(unsigned int screen_id,
					    unsigned int output_type);
int bsp_disp_feat_is_support_capture(unsigned int disp);
int bsp_disp_feat_is_support_crc(unsigned int disp);
int bsp_disp_feat_is_support_smbl(unsigned int disp);
int bsp_disp_feat_is_support_enhance(unsigned int disp);
int bsp_disp_feat_is_support_deband(unsigned int disp);
int bsp_disp_feat_is_support_gamma(unsigned int disp);
int bsp_disp_feat_is_support_dither(unsigned int disp);
int bsp_disp_feat_is_support_ksc(unsigned int disp);
int bsp_disp_feat_is_support_fmt(unsigned int disp);
int bsp_disp_feat_get_chn_type(unsigned int disp, unsigned int chn);
int bsp_disp_feat_chn_is_support_vep(unsigned int disp, unsigned int chn);
int bsp_disp_feat_chn_is_support_scale(unsigned int disp, unsigned int chn);
int bsp_disp_feat_chn_is_support_edscale(unsigned int disp, unsigned int chn);
int bsp_disp_feat_chn_is_support_asuscale(unsigned int disp, unsigned int chn);
int bsp_disp_feat_chn_is_support_fcm(unsigned int disp, unsigned int chn);
int bsp_disp_feat_chn_is_support_dci(unsigned int disp, unsigned int chn);
int bsp_disp_feat_chn_is_support_sharp(unsigned int disp, unsigned int chn);
int bsp_disp_feat_chn_is_support_noise(unsigned int disp, unsigned int chn);
int bsp_disp_feat_chn_is_support_cdc(unsigned int disp, unsigned int chn);
int bsp_disp_feat_chn_is_support_snr(unsigned int disp, unsigned int chn);
int bsp_disp_feat_lyr_is_support_fbd(unsigned int disp, unsigned int chn, unsigned int lyr);
int bsp_disp_feat_lyr_is_support_atw(unsigned int disp, unsigned int chn, unsigned int lyr);
unsigned int bsp_disp_feat_get_num_vdpo(void);
int disp_init_feat(struct disp_feat_init *feat_init);
int disp_exit_feat(void);
int disp_feat_is_using_rcq(unsigned int disp);
int disp_feat_is_using_wb_rcq(unsigned int wb);

#endif
