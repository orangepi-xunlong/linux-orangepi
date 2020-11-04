/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "de_feat.h"

static const struct de_feat *de_cur_features;
extern struct de_feat de330_mode0_features;
extern struct de_feat de330_mode1_features;
extern struct de_feat de330_mode2_features;

s32 de_feat_get_num_screens(void)
{
	return de_cur_features->num_screens;
}

s32 de_feat_get_num_chns(u32 disp)
{
	if (disp < de_feat_get_num_screens())
		return de_cur_features->num_chns[disp];
	else
		return 0;
}

s32 de_feat_get_num_vi_chns(u32 disp)
{
	if (disp < de_feat_get_num_screens())
		return de_cur_features->num_vi_chns[disp];
	else
		return 0;
}

s32 de_feat_get_num_ui_chns(u32 disp)
{
	if (disp < de_feat_get_num_screens())
		return de_cur_features->num_chns[disp]
			- de_cur_features->num_vi_chns[disp];
	else
		return 0;
}

s32 de_feat_get_num_layers(u32 disp)
{
	s32 num_layers = 0;
	if (disp < de_feat_get_num_screens()) {
		u32 i, index = 0, num_channels = 0;
		for (i = 0; i < disp; i++)
			index += de_feat_get_num_chns(i);
		num_channels = de_feat_get_num_chns(disp);
		for (i = 0; i < num_channels; i++, index++)
			num_layers += de_cur_features->num_layers[index];
	}
	return num_layers;
}

s32 de_feat_get_num_layers_by_chn(u32 disp, u32 chn)
{
	if ((disp < de_feat_get_num_screens())
		&& (chn < de_feat_get_num_chns(disp))) {
		u32 i, index = 0;
		for (i = 0; i < disp; i++)
			index += de_feat_get_num_chns(i);
		index += chn;
		return de_cur_features->num_layers[index];
	}
	return 0;
}

s32 de_feat_is_support_vep(u32 disp)
{
	s32 is_support_vep = 0;
	if (disp < de_feat_get_num_screens()) {
		u32 i, index = 0, num_channels = 0;
		for (i = 0; i < disp; i++)
			index += de_feat_get_num_chns(i);
		num_channels = de_feat_get_num_chns(disp);
		for (i = 0; i < num_channels; i++, index++)
			is_support_vep += de_cur_features->is_support_vep[index];
	}
	return is_support_vep;
}

s32 de_feat_is_support_vep_by_chn(u32 disp, u32 chn)
{
	if ((disp < de_feat_get_num_screens())
		&& (chn < de_feat_get_num_chns(disp))) {
		u32 i, index = 0;
		for (i = 0; i < disp; i++)
			index += de_feat_get_num_chns(i);
		index += chn;
		return de_cur_features->is_support_vep[index];
	}
	return 0;
}

s32 de_feat_is_support_smbl(u32 disp)
{
	return de_cur_features->is_support_smbl[disp];
}

s32 de_feat_is_support_wb(u32 disp)
{
	if (disp < de_feat_get_num_screens())
		return de_cur_features->is_support_wb[disp];
	else
		return 0;
}

s32 de_feat_is_support_scale(u32 disp)
{
	s32 is_support_scale = 0;
	if (disp < de_feat_get_num_screens()) {
		u32 i, index = 0, num_channels = 0;
		for (i = 0; i < disp; i++)
			index += de_feat_get_num_chns(i);
		num_channels = de_feat_get_num_chns(disp);
		for (i = 0; i < num_channels; i++, index++)
			is_support_scale += de_cur_features->is_support_scale[index];
	}
	return is_support_scale;
}

s32 de_feat_is_support_scale_by_chn(u32 disp, u32 chn)
{
	if ((disp < de_feat_get_num_screens())
		&& (chn < de_feat_get_num_chns(disp))) {
		u32 i, index = 0;
		for (i = 0; i < disp; i++)
			index += de_feat_get_num_chns(i);
		index += chn;
		return de_cur_features->is_support_scale[index];
	}
	return 0;
}

s32 de_feat_is_support_edscale(u32 disp)
{
	s32 is_support = 0;
	if (disp < de_feat_get_num_screens()) {
		u32 i, index = 0, num_channels = 0;
		for (i = 0; i < disp; i++)
			index += de_feat_get_num_chns(i);
		num_channels = de_feat_get_num_chns(disp);
		for (i = 0; i < num_channels; i++, index++)
			is_support += de_cur_features->is_support_edscale[index];
	}
	return is_support;
}

s32 de_feat_is_support_edscale_by_chn(u32 disp, u32 chn)
{
	if ((disp < de_feat_get_num_screens())
		&& (chn < de_feat_get_num_chns(disp))) {
		u32 i, index = 0;
		for (i = 0; i < disp; i++)
			index += de_feat_get_num_chns(i);
		index += chn;
		return de_cur_features->is_support_edscale[index];
	}
	return 0;
}

s32 de_feat_is_support_de_noise(u32 disp)
{
	s32 is_support = 0;
	if (disp < de_feat_get_num_screens()) {
		u32 i, index = 0, num_channels = 0;
		for (i = 0; i < disp; i++)
			index += de_feat_get_num_chns(i);
		num_channels = de_feat_get_num_chns(disp);
		for (i = 0; i < num_channels; i++, index++)
			is_support += de_cur_features->is_support_de_noise[index];
	}
	return is_support;
}

s32 de_feat_is_support_de_noise_by_chn(
	u32 disp, u32 chn)
{
	if ((disp < de_feat_get_num_screens())
		&& (chn < de_feat_get_num_chns(disp))) {
		u32 i, index = 0;
		for (i = 0; i < disp; i++)
			index += de_feat_get_num_chns(i);
		index += chn;
		return de_cur_features->is_support_de_noise[index];
	}
	return 0;
}

s32 de_feat_is_support_cdc_by_chn(u32 disp, u32 chn)
{
	if ((disp < de_feat_get_num_screens())
		&& (chn < de_feat_get_num_chns(disp))) {
		u32 i, index = 0;
		for (i = 0; i < disp; i++)
			index += de_feat_get_num_chns(i);
		index += chn;
		return de_cur_features->is_support_cdc[index];
	}
	return 0;
}

s32 de_feat_is_support_snr_by_chn(u32 disp, u32 chn)
{
	if ((disp < de_feat_get_num_screens())
		&& (chn < de_feat_get_num_chns(disp))) {
		u32 i, index = 0;
		for (i = 0; i < disp; i++)
			index += de_feat_get_num_chns(i);
		index += chn;
		return de_cur_features->is_support_snr[index];
	}
	return 0;
}

s32 de_feat_is_support_fbd_by_layer(u32 disp,
	u32 chn, u32 layer)
{
	if ((disp < de_feat_get_num_screens())
		&& (chn < de_feat_get_num_chns(disp))
		&& (layer < de_feat_get_num_layers_by_chn(disp, chn))) {
		u32 i, index = 0;
		for (i = 0; i < disp; i++)
			index += de_feat_get_num_layers(i);
		for (i = 0; i < chn; i++)
			index += de_feat_get_num_layers_by_chn(disp, i);
		index += layer;
		return de_cur_features->is_support_fbd[index];
	}
	return 0;
}

s32 de_feat_is_support_atw_by_layer(u32 disp,
	u32 chn, u32 layer)
{
	if ((disp < de_feat_get_num_screens())
		&& (chn < de_feat_get_num_chns(disp))
		&& (layer < de_feat_get_num_layers_by_chn(disp, chn))) {
		u32 i, index = 0;
		for (i = 0; i < disp; i++)
			index += de_feat_get_num_layers(i);
		for (i = 0; i < chn; i++)
			index += de_feat_get_num_layers_by_chn(disp, i);
		index += layer;
		return de_cur_features->is_support_atw[index];
	}
	return 0;
}

s32 de_feat_get_scale_linebuf_for_yuv(
	u32 disp, u32 chn)
{
	if ((disp < de_feat_get_num_screens())
		&& (chn < de_feat_get_num_chns(disp))) {
		u32 i, index = 0;
		for (i = 0; i < disp; i++)
			index += de_feat_get_num_chns(i);
		index += chn;
		return de_cur_features->scale_line_buffer_yuv[index];
	}
	return 0;
}

s32 de_feat_get_scale_linebuf_for_rgb(
	u32 disp, u32 chn)
{
	if ((disp < de_feat_get_num_screens())
		&& (chn < de_feat_get_num_chns(disp))) {
		u32 i, index = 0;
		for (i = 0; i < disp; i++)
			index += de_feat_get_num_chns(i);
		index += chn;
		return de_cur_features->scale_line_buffer_rgb[index];
	}
	return 0;
}

s32 de_feat_get_scale_linebuf_for_ed(
	u32 disp, u32 chn)
{
	if ((disp < de_feat_get_num_screens())
		&& (chn < de_feat_get_num_chns(disp))) {
		u32 i, index = 0;
		for (i = 0; i < disp; i++)
			index += de_feat_get_num_chns(i);
		index += chn;
		return de_cur_features->scale_line_buffer_ed[index];
	}
	return 0;
}

u32 de_feat_get_scaler_type(u32 disp, u32 chn)
{
	if ((disp < de_feat_get_num_screens())
		&& (chn < de_feat_get_num_chns(disp))) {
		u32 i, index = 0;
		for (i = 0; i < disp; i++)
			index += de_feat_get_num_chns(i);
		index += chn;
		return de_cur_features->scaler_type[index];
	}
	return DE_SCALER_TYPE_NONE;
}

s32 de_feat_get_phy_chn_id(u32 disp, u32 chn)
{
#ifdef SUPPORT_FEAT_INIT_CONFIG

	s32 phy_chn = -1;
	if ((disp < de_feat_get_num_screens())
		&& (chn < de_feat_get_num_chns(disp))) {
		u32 index = 0;
		u32 i;
		for (i = 0; i < disp; ++i)
			index += de_feat_get_num_chns(i);
		index += chn;
		phy_chn = de_cur_features->chn_id_lut[index];
	}
	return phy_chn;

#else

	return chn;

#endif /* #if SUPPORT_FEAT_INIT_CONFIG */
}

s32 de_feat_is_using_rcq(u32 disp)
{
#ifdef RTMX_USE_RCQ
	return RTMX_USE_RCQ;
#else
	return 0;
#endif
}

s32 de_feat_is_using_wb_rcq(u32 wb)
{
#ifdef RTWB_USE_RCQ
	return RTWB_USE_RCQ;
#else
	return 0;
#endif
}

s32 de_feat_init_config(struct de_feat_init *feat_init)
{
	if (feat_init->chn_cfg_mode == 0)
		de_cur_features = &de330_mode0_features;
	else if (feat_init->chn_cfg_mode == 1)
		de_cur_features = &de330_mode1_features;
	else if (feat_init->chn_cfg_mode == 2)
		de_cur_features = &de330_mode2_features;
	else
		return -1;

	return 0;
}

/**
 * @name       de_feat_get_number_of_vdpo
 * @brief      get number of vdpo
 * @param[IN]  none
 * @param[OUT] none
 * @return     number of vdpo
 */
unsigned int de_feat_get_number_of_vdpo(void)
{
	/* return de_cur_features->num_vdpo; */
	return 0;
}
