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

#include "disp_features.h"

int bsp_disp_feat_get_num_screens(void)
{
	return de_feat_get_num_screens();
}

int bsp_disp_feat_get_num_devices(void)
{
	return de_feat_get_num_devices();
}

int bsp_disp_feat_get_num_channels(unsigned int disp)
{
	return de_feat_get_num_chns(disp);
}

int bsp_disp_feat_get_num_layers(unsigned int disp)
{
	return de_feat_get_num_layers(disp);
}

int bsp_disp_feat_get_num_layers_by_chn(unsigned int disp, unsigned int chn)
{
	return de_feat_get_num_layers_by_chn(disp, chn);
}

/**
 * Query whether specified timing controller support the output_type spcified
 * @disp: the index of timing controller
 * @output_type: the display output type
 * On support, returns 1. Otherwise, returns 0.
 */
int bsp_disp_feat_is_supported_output_types(unsigned int disp,
					    unsigned int output_type)
{
	return de_feat_is_supported_output_types(disp, output_type);
}

int bsp_disp_feat_is_support_smbl(unsigned int disp)
{
	return de_feat_is_support_smbl(disp);
}

int bsp_disp_feat_is_support_capture(unsigned int disp)
{
	return de_feat_is_support_wb(disp);
}

int bsp_disp_feat_is_support_crc(unsigned int disp)
{
#if (defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)) && defined(SUPPORT_CRC)
	return de_feat_is_support_crc(disp);
#else
	return 0;
#endif
}

int bsp_disp_feat_is_support_enhance(unsigned int disp)
{
	return de_feat_is_support_vep(disp);
}

int bsp_disp_feat_is_support_deband(unsigned int disp)
{
#if defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	return de_feat_is_support_deband(disp);
#else
	return 0;
#endif
}

int bsp_disp_feat_is_support_gamma(unsigned int disp)
{
#if defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	return de_feat_is_support_gamma(disp);
#else
	return 0;
#endif
}

int bsp_disp_feat_is_support_fmt(unsigned int disp)
{
#if defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	return de_feat_is_support_fmt(disp);
#elif defined(DE_VERSION_V33X) ||\
	(defined(CONFIG_ARCH_SUN50IW6) && defined(SUPPORT_FORMATTER))
	return 1;
#else
	return 0;
#endif
}

int bsp_disp_feat_is_support_ksc(unsigned int disp)
{
#if defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	return de_feat_is_support_ksc(disp);
#else
	return 0;
#endif
}

int bsp_disp_feat_is_support_dither(unsigned int disp)
{
#if defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	return de_feat_is_support_dither(disp);
#else
	return 0;
#endif
}

int bsp_disp_feat_get_chn_type(unsigned int disp, unsigned int chn)
{
	return chn >= de_feat_get_num_vi_chns(disp) ? 0 : 1;
}

int bsp_disp_feat_chn_is_support_vep(unsigned int disp, unsigned int chn)
{
	return de_feat_is_support_vep_by_chn(disp, chn);
}

int bsp_disp_feat_chn_is_support_scale(unsigned int disp, unsigned int chn)
{
	return de_feat_is_support_scale_by_chn(disp, chn);
}

int bsp_disp_feat_chn_is_support_edscale(unsigned int disp, unsigned int chn)
{
#if IS_ENABLED(CONFIG_ARCH_SUN50IW3) || IS_ENABLED(CONFIG_ARCH_SUN50IW6) ||\
	defined(DE_VERSION_V33X) || defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	return de_feat_is_support_edscale_by_chn(disp, chn);
#else
	return 0;
#endif
}

int bsp_disp_feat_chn_is_support_asuscale(unsigned int disp, unsigned int chn)
{
#if defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	return de_feat_is_support_asuscale_by_chn(disp, chn);
#else
	return 0;
#endif
}

int bsp_disp_feat_chn_is_support_fcm(unsigned int disp, unsigned int chn)
{
#if defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	return de_feat_is_support_fcm_by_chn(disp, chn);
#else
	return 0;
#endif
}

int bsp_disp_feat_chn_is_support_dci(unsigned int disp, unsigned int chn)
{
#if defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	return de_feat_is_support_dci_by_chn(disp, chn);
#else
	return 0;
#endif
}

int bsp_disp_feat_chn_is_support_sharp(unsigned int disp, unsigned int chn)
{
#if defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	return de_feat_is_support_sharp_by_chn(disp, chn);
#else
	return 0;
#endif
}

int bsp_disp_feat_chn_is_support_noise(unsigned int disp, unsigned int chn)
{
#if IS_ENABLED(CONFIG_ARCH_SUN50IW3) || IS_ENABLED(CONFIG_ARCH_SUN50IW6) ||\
	defined(DE_VERSION_V33X) || defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	return de_feat_is_support_de_noise_by_chn(disp, chn);
#else
	return 0;
#endif
}

int bsp_disp_feat_chn_is_support_cdc(unsigned int disp, unsigned int chn)
{
#if IS_ENABLED(CONFIG_ARCH_SUN50IW3) || IS_ENABLED(CONFIG_ARCH_SUN50IW6) ||\
	defined(DE_VERSION_V33X) || defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	return de_feat_is_support_cdc_by_chn(disp, chn);
#else
	return 0;
#endif
}

int bsp_disp_feat_chn_is_support_snr(unsigned int disp, unsigned int chn)
{
#if defined(DE_VERSION_V33X) || defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	return de_feat_is_support_snr_by_chn(disp, chn);
#else
	return 0;
#endif
}

int bsp_disp_feat_lyr_is_support_fbd(unsigned int disp,
				     unsigned int chn, unsigned int lyr)
{
#if IS_ENABLED(CONFIG_ARCH_SUN50IW3) || defined(CONFIG_ARCH_SUN50IW6) ||\
	defined(DE_VERSION_V33X) || defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	return de_feat_is_support_fbd_by_layer(disp, chn, lyr);
#else
	return 0;
#endif
}

int bsp_disp_feat_lyr_is_support_atw(unsigned int disp,
				     unsigned int chn, unsigned int lyr)
{
#if IS_ENABLED(CONFIG_ARCH_SUN50IW3) || defined(CONFIG_ARCH_SUN50IW6) ||\
	defined(DE_VERSION_V33X) || defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	return de_feat_is_support_atw_by_layer(disp, chn, lyr);
#else
	return 0;
#endif
}

int disp_init_feat(struct disp_feat_init *feat_init)
{
#if defined(DE_VERSION_V33X) || defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	struct de_feat_init de_feat;
	de_feat.chn_cfg_mode = feat_init->chn_cfg_mode;
	return de_feat_init_config(&de_feat);
#else
	return de_feat_init();
#endif
}

int disp_exit_feat(void)
{
#if defined(DE_VERSION_V33X) || defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	return 0;
#else
	return de_feat_exit();
#endif
}

unsigned int bsp_disp_feat_get_num_vdpo(void)
{
	return de_feat_get_number_of_vdpo();
}

int disp_feat_is_using_rcq(unsigned int disp)
{
#if defined(DE_VERSION_V33X) || defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	return de_feat_is_using_rcq(disp);
#endif
	return 0;
}

int disp_feat_is_using_wb_rcq(unsigned int wb)
{
#if defined(DE_VERSION_V33X) || defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	return de_feat_is_using_wb_rcq(wb);
#endif
	return 0;
}
