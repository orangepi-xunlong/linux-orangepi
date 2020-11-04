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

int bsp_disp_feat_is_support_enhance(unsigned int disp)
{
	return de_feat_is_support_vep(disp);
}

int disp_init_feat(struct disp_feat_init *feat_init)
{
#ifdef DE_VERSION_V33X
	struct de_feat_init de_feat;
	de_feat.chn_cfg_mode = feat_init->chn_cfg_mode;
	return de_feat_init_config(&de_feat);
#else
	return de_feat_init();
#endif
}

int disp_exit_feat(void)
{
#ifdef DE_VERSION_V33X
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
#if defined(DE_VERSION_V33X)
       return de_feat_is_using_rcq(disp);
#endif
       return 0;
}

int disp_feat_is_using_wb_rcq(unsigned int wb)
{
#if defined(DE_VERSION_V33X)
       return de_feat_is_using_wb_rcq(wb);
#endif
       return 0;
}
