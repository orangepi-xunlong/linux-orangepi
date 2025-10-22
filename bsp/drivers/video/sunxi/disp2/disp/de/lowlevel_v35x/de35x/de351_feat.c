/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * de351_feat.c
 *
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
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
 * chn_id_lut:0~5 reserve for video channel, 6~11 reserve for ui channel
 *
 */
#include "de_feat.h"

const struct de_feat de35x_mode1_features;
const struct de_feat de35x_mode2_features;
const struct de_feat de35x_mode3_features;
#if defined(__DISP_TEMP_CODE__)
//DE0
/* *********************** mode0 begin ************************** */
static const u32 de351_mode0_num_de_disps[] = {
	1, /* RTMX0 */
};
static const u32 de351_mode0_skip_disps[] = {
	0, /* RTMX0 */
};
static const s32 de351_mode0_num_chns[] = {
	4, /* DISP0 */
};
static const s32 de351_mode0_num_vi_chns[] = {
	2, /* DISP0 */
};
static const s32 de351_mode0_num_layers[] = {
	4, 4, 4, 4, /* DISP0 */
};
static const s32 de351_mode0_is_support_vep[] = {
	1,  0, 0, 0,/* DISP0 */
};
static const s32 de351_mode0_is_support_smbl[] = {
	0, /* DISP0 */
};
static const s32 de351_mode0_is_support_deband[] = {
	1, /* DISP0 */
};
static const s32 de351_mode0_is_support_dither[] = {
	1, /* DISP0 */
};
static const s32 de351_mode0_is_support_gamma[] = {
	1, /* DISP0 */
};
static const s32 de351_mode0_is_support_wb[] = {
	1, /* DISP0 */
};
static const s32 de351_mode0_is_support_crc[] = {
	0, /* DISP0 */
};
static const s32 de351_mode0_is_support_scale[] = {
	1, 1, 1, 1,/* DISP0 */
};
static const s32 de351_mode0_scale_line_buffer_yuv[] = {
	4096, 2560, 2560, 2560,/* DISP0 */
};
static const s32 de351_mode0_scale_line_buffer_rgb[] = {
	2048, 2560, 2560, 2560,/* DISP0 */
};
static const s32 de351_mode0_scale_line_buffer_ed[] = {
	2048, 2560, 2560, 2560,/* DISP0 */
};
static const s32 de351_mode0_is_support_edscale[] = {
	1, 0, 0, 0,
};
static const s32 de351_mode0_is_support_asuscale[] = {
	1, 0, 0, 0,
};
static const s32 de351_mode0_is_support_dci[] = {
	1, 0, 0, 0,
};
static const s32 de351_mode0_is_support_sharp[] = {
	1, 0, 0, 0,
};
static const s32 de351_mode0_is_support_fcm[] = {
	1, 0, 0, 0,
};
static const s32 de351_mode0_is_support_de_noise[] = {
	0, 0, 0, 0,
};
static const s32 de351_mode0_is_support_cdc[] = {
	1, 0, 1, 0,
};
static const s32 de351_mode0_is_support_snr[] = {
	1, 0, 0, 0,
};
static const s32 de351_mode0_is_support_fbd[] = {
	/* DISP0 CH0 */
	1, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 */
	0, 0, 0, 0,
	/* DISP0 CH3 */
	0, 0, 0, 0,
};
static const s32 de351_mode0_is_support_tfbd[] = {
	/* DISP0 CH0 */
	1, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 */
	1, 0, 0, 0,
	/* DISP0 CH3 */
	1, 0, 0, 0,
};
static const s32 de351_mode0_is_support_atw[] = {
	/* DISP0 CH0 */
	0, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 */
	0, 0, 0, 0,
	/* DISP0 CH3 */
	0, 0, 0, 0,
};
static const u32 de351_mode0_scaler_type[] = {
	/* DISP0 CH0 */
	DE_SCALER_TYPE_ASU,
	/* DISP0 CH1 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH2 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH3 */
	DE_SCALER_TYPE_VSU8,
};
static const s32 de351_mode0_chn_id_lut[] = {
	0, 1, /* DISP0, VIDEO CHANNEL */
	6, 7, /* DISP0, UI CHANNEL */
};

const struct de_feat de35x_mode0_features = {
	.num_screens = 1,
	.num_de_rtmx = 1,
	.num_de_disps = de351_mode0_num_de_disps,
	.num_skip_disps = de351_mode0_skip_disps,
	.num_chns = de351_mode0_num_chns,
	.num_vi_chns = de351_mode0_num_vi_chns,
	.num_layers = de351_mode0_num_layers,
	.is_support_vep = de351_mode0_is_support_vep,
	.is_support_smbl = de351_mode0_is_support_smbl,
	.is_support_deband = de351_mode0_is_support_deband,
	.is_support_gamma = de351_mode0_is_support_gamma,
	.is_support_dither = de351_mode0_is_support_dither,
	.is_support_wb = de351_mode0_is_support_wb,
	.is_support_crc = de351_mode0_is_support_crc,
	.is_support_scale = de351_mode0_is_support_scale,
	.scale_line_buffer_yuv = de351_mode0_scale_line_buffer_yuv,
	.scale_line_buffer_rgb = de351_mode0_scale_line_buffer_rgb,
	.scale_line_buffer_ed = de351_mode0_scale_line_buffer_ed,
	.is_support_edscale = de351_mode0_is_support_edscale,
	.is_support_asuscale = de351_mode0_is_support_asuscale,
	.is_support_dci = de351_mode0_is_support_dci,
	.is_support_fcm = de351_mode0_is_support_fcm,
	.is_support_sharp = de351_mode0_is_support_sharp,
	.is_support_de_noise = de351_mode0_is_support_de_noise,
	.is_support_cdc = de351_mode0_is_support_cdc,
	.is_support_fbd = de351_mode0_is_support_fbd,
	.is_support_atw = de351_mode0_is_support_atw,
	.is_support_snr = de351_mode0_is_support_snr,
	.is_support_tfbd = de351_mode0_is_support_tfbd,

	.scaler_type = de351_mode0_scaler_type,
	.chn_id_lut = de351_mode0_chn_id_lut,
};
/* *********************** mode0 end **************************** */

#endif

#if 1
//DE1
/* *********************** mode1 begin ************************** */
static const u32 de351_mode0_num_de_disps[] = {
	2, /* RTMX0 */
};
static const u32 de351_mode0_skip_disps[] = {
	2, /* RTMX0 */
};
static const s32 de351_mode0_num_chns[] = {
	3, /* DISP0 */
	2, /* DISP1 */
};
static const s32 de351_mode0_num_vi_chns[] = {
	2, /* DISP0 */
	1, /* DISP1 */
};
static const s32 de351_mode0_num_layers[] = {
	4, 4, 4, /* DISP0 */
	4, 4,    /* DISP0 */
};
static const s32 de351_mode0_is_support_vep[] = {
	1, 0, 0, /* DISP0 */
	0, 0,    /* DISP1 */
};
static const s32 de351_mode0_is_support_smbl[] = {
	0, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de351_mode0_is_support_deband[] = {
	0, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de351_mode0_is_support_dither[] = {
	1, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de351_mode0_is_support_gamma[] = {
	1, /* DISP0 */
	1, /* DISP1 */
};
static const s32 de351_mode0_is_support_wb[] = {
	1, /* DISP0 */
	1, /* DISP1 */
};
static const s32 de351_mode0_is_support_crc[] = {
	1, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de351_mode0_is_support_scale[] = {
	1, 1, 1, /* DISP0 */
	1, 1,    /* DISP1 */
};
static const s32 de351_mode0_scale_line_buffer_yuv[] = {
	/*VCH3 VCH5 UCH4*/
	2048, 2048, 2048, /* DISP0 */
	/*VCH4 UCH5*/
	2048, 2048,       /* DISP1 */
};
static const s32 de351_mode0_scale_line_buffer_rgb[] = {
	/*VCH3 VCH5 UCH4*/
	4096, 2048, 2048, /* DISP0 */
	/*VCH4 UCH5*/
	2048, 2048,       /* DISP1 */
};
static const s32 de351_mode0_scale_line_buffer_ed[] = {
	/*VCH3 VCH5 UCH4*/
	4096, 2048, 2048, /* DISP0 */
	/*VCH4 UCH5*/
	2048, 2048,       /* DISP1 */
};
static const s32 de351_mode0_is_support_edscale[] = {
	/*VCH3 VCH5 UCH4*/
	0, 0, 0, /* DISP0 */
	/*VCH4 UCH5*/
	0, 0,    /* DISP1 */
};
static const s32 de351_mode0_is_support_asuscale[] = {
	/*VCH3 VCH5 UCH4*/
	0, 0, 0, /* DISP0 */
	/*VCH4 UCH5*/
	0, 0,    /* DISP1 */
};
static const s32 de351_mode0_is_support_dci[] = {
	/*VCH3 VCH5 UCH4*/
	0, 0, 0, /* DISP0 */
	/*VCH4 UCH5*/
	0, 0,    /* DISP1 */
};
static const s32 de351_mode0_is_support_sharp[] = {
	/*VCH3 VCH5 UCH4*/
	0, 0, 0, /* DISP0 */
	/*VCH4 UCH5*/
	0, 0,    /* DISP1 */
};
static const s32 de351_mode0_is_support_fcm[] = {
	/*VCH3 VCH5 UCH4*/
	1, 0, 0, /* DISP0 */
	/*VCH4 UCH5*/
	0, 0,    /* DISP1 */
};
static const s32 de351_mode0_is_support_de_noise[] = {
	/*VCH3 VCH5 UCH4*/
	0, 0, 0, /* DISP0 */
	/*VCH4 UCH5*/
	0, 0,    /* DISP1 */
};
static const s32 de351_mode0_is_support_cdc[] = {
	/*VCH3 VCH5 UCH4*/
	0, 0, 0, /* DISP0 */
	/*VCH4 UCH5*/
	0, 0,    /* DISP1 */
};
static const s32 de351_mode0_is_support_snr[] = {
	/*VCH3 VCH5 UCH4*/
	1, 0, 0, /* DISP0 */
	/*VCH4 UCH5*/
	0, 0,    /* DISP1 */
};
static const s32 de351_mode0_is_support_fbd[] = {
	/* DISP0 CH0 */
	0, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 */
	0, 0, 0, 0,
	/* DISP1 CH0 */
	0, 0, 0, 0,
	/* DISP1 CH1 */
	0, 0, 0, 0,
};
static const s32 de351_mode0_is_support_tfbd[] = {
	/* DISP0 CH0 */
	0, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 */
	1, 0, 0, 0,
	/* DISP1 CH0 */
	0, 0, 0, 0,
	/* DISP1 CH1 */
	1, 0, 0, 0,
};
static const s32 de351_mode0_is_support_atw[] = {
	/* DISP0 CH0 */
	0, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 */
	0, 0, 0, 0,
	/* DISP1 CH0 */
	0, 0, 0, 0,
	/* DISP1 CH1 */
	0, 0, 0, 0,
};
static const u32 de351_mode0_scaler_type[] = {
	/* DISP0 CH0 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH1 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH2 */
	DE_SCALER_TYPE_VSU8,
	/* DISP1 CH0 */
	DE_SCALER_TYPE_VSU8,
	/* DISP1 CH1 */
	DE_SCALER_TYPE_VSU8,
};
static const s32 de351_mode0_chn_id_lut[] = {
	3, 5,  /* DISP0, VIDEO CHANNEL, VCH3 VCH5 */
	10,    /* DISP0, UI CHANNEL,    UCH4 */
	4,     /* DISP1, VIDEO CHANNEL, VCH4 */
	11,    /* DISP1, UI CHANNEL,    UCH5 */
};

const struct de_feat de35x_mode0_features = {
	.num_screens = 2,
	.num_de_rtmx = 1,
	.num_de_disps = de351_mode0_num_de_disps,
	.num_skip_disps = de351_mode0_skip_disps,
	.num_chns = de351_mode0_num_chns,
	.num_vi_chns = de351_mode0_num_vi_chns,
	.num_layers = de351_mode0_num_layers,
	.is_support_vep = de351_mode0_is_support_vep,
	.is_support_smbl = de351_mode0_is_support_smbl,
	.is_support_deband = de351_mode0_is_support_deband,
	.is_support_gamma = de351_mode0_is_support_gamma,
	.is_support_dither = de351_mode0_is_support_dither,
	.is_support_wb = de351_mode0_is_support_wb,
	.is_support_crc = de351_mode0_is_support_crc,
	.is_support_scale = de351_mode0_is_support_scale,
	.scale_line_buffer_yuv = de351_mode0_scale_line_buffer_yuv,
	.scale_line_buffer_rgb = de351_mode0_scale_line_buffer_rgb,
	.scale_line_buffer_ed = de351_mode0_scale_line_buffer_ed,
	.is_support_edscale = de351_mode0_is_support_edscale,
	.is_support_asuscale = de351_mode0_is_support_asuscale,
	.is_support_dci = de351_mode0_is_support_dci,
	.is_support_fcm = de351_mode0_is_support_fcm,
	.is_support_sharp = de351_mode0_is_support_sharp,
	.is_support_de_noise = de351_mode0_is_support_de_noise,
	.is_support_cdc = de351_mode0_is_support_cdc,
	.is_support_fbd = de351_mode0_is_support_fbd,
	.is_support_atw = de351_mode0_is_support_atw,
	.is_support_snr = de351_mode0_is_support_snr,
	.is_support_tfbd = de351_mode0_is_support_tfbd,

	.scaler_type = de351_mode0_scaler_type,
	.chn_id_lut = de351_mode0_chn_id_lut,
};
/* *********************** mode1 end **************************** */
#endif

#if defined(__DISP_TEMP_CODE__)

//DE0 + DE1   //TODO not test yet
/* *********************** mode2 begin ************************** */
static const u32 de351_mode0_num_de_disps[] = {
	1, /* RTMX0 */
	2, /* RTMX1 */
};

static const u32 de351_mode0_skip_disps[] = {
	0, /* RTMX0 */
	2, /* RTMX1 */
};

static const s32 de351_mode0_num_chns[] = {
	4, /* DISP0 */
	3, /* DISP1 */
	2, /* DISP2 */
};
static const s32 de351_mode0_num_vi_chns[] = {
	2, /* DISP0 */
	2, /* DISP1 */
	1, /* DISP2 */
};
static const s32 de351_mode0_num_layers[] = {
	4, 4, 4, 4, /* DISP0 */
	4, 4, 4,    /* DISP1 */
	4, 4,       /* DISP2 */

};
static const s32 de351_mode0_is_support_vep[] = {
	1,  0, 0, 0,/* DISP0 */
};
static const s32 de351_mode0_is_support_smbl[] = {
	0, /* DISP0 */
};
static const s32 de351_mode0_is_support_deband[] = {
	1, /* DISP0 */
};
static const s32 de351_mode0_is_support_dither[] = {
	1, /* DISP0 */
};
static const s32 de351_mode0_is_support_gamma[] = {
	1, /* DISP0 */
};
static const s32 de351_mode0_is_support_wb[] = {
	1, /* DISP0 */
};
static const s32 de351_mode0_is_support_scale[] = {
	1, 1, 1, 1,/* DISP0 */
};
/*
static const s32 de351_mode0_scale_line_buffer[] = {
	4096,
}; */
static const s32 de351_mode0_scale_line_buffer_yuv[] = {
	4096, 2560, 2560, 2560,/* DISP0 */
};
static const s32 de351_mode0_scale_line_buffer_rgb[] = {
	2048, 2560, 2560, 2560,/* DISP0 */
};
static const s32 de351_mode0_scale_line_buffer_ed[] = {
	2048, 2560, 2560, 2560,/* DISP0 */
};
static const s32 de351_mode0_is_support_edscale[] = {
	1, 0, 0, 0,
};
static const s32 de351_mode0_is_support_asuscale[] = {
	1, 0, 0, 0,
};
static const s32 de351_mode0_is_support_dci[] = {
	1, 0, 0, 0,
};
static const s32 de351_mode0_is_support_sharp[] = {
	1, 0, 0, 0,
};
static const s32 de351_mode0_is_support_fcm[] = {
	1, 0, 0, 0,
};
static const s32 de351_mode0_is_support_de_noise[] = {
	0, 0, 0, 0,
};
static const s32 de351_mode0_is_support_cdc[] = {
	1, 0, 1, 0,
};
static const s32 de351_mode0_is_support_snr[] = {
	1, 0, 0, 0,
};
static const s32 de351_mode0_is_support_fbd[] = {
	/* DISP0 CH0 */
	1, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 */
	0, 0, 0, 0,
	/* DISP0 CH3 */
	0, 0, 0, 0,
};
static const s32 de351_mode0_is_support_tfbd[] = {
	/* DISP0 CH0 */
	1, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 */
	1, 0, 0, 0,
	/* DISP0 CH3 */
	1, 0, 0, 0,
};
static const s32 de351_mode0_is_support_atw[] = {
	/* DISP0 CH0 */
	0, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 */
	0, 0, 0, 0,
	/* DISP0 CH3 */
	0, 0, 0, 0,
};
static const u32 de351_mode0_scaler_type[] = {
	/* DISP0 CH0 */
	DE_SCALER_TYPE_ASU,
	/* DISP0 CH1 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH2 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH3 */
	DE_SCALER_TYPE_VSU8,
};
static const s32 de351_mode0_chn_id_lut[] = {
	0, 1, /* DISP0, VIDEO CHANNEL */
	6, 7, /* DISP0, UI CHANNEL */
};

const struct de_feat de35x_mode0_features = {
	.num_screens = 1,
	.num_de_rtmx = 2,
	.num_de_disps = de351_mode0_num_de_disps,
	.num_skip_disps = de351_mode0_skip_disps,
	.num_chns = de351_mode0_num_chns,
	.num_vi_chns = de351_mode0_num_vi_chns,
	.num_layers = de351_mode0_num_layers,
	.is_support_vep = de351_mode0_is_support_vep,
	.is_support_smbl = de351_mode0_is_support_smbl,
	.is_support_deband = de351_mode0_is_support_deband,
	.is_support_gamma = de351_mode0_is_support_gamma,
	.is_support_dither = de351_mode0_is_support_dither,
	.is_support_wb = de351_mode0_is_support_wb,
	.is_support_scale = de351_mode0_is_support_scale,
	.scale_line_buffer_yuv = de351_mode0_scale_line_buffer_yuv,
	.scale_line_buffer_rgb = de351_mode0_scale_line_buffer_rgb,
	.scale_line_buffer_ed = de351_mode0_scale_line_buffer_ed,
	.is_support_edscale = de351_mode0_is_support_edscale,
	.is_support_asuscale = de351_mode0_is_support_asuscale,
	.is_support_dci = de351_mode0_is_support_dci,
	.is_support_fcm = de351_mode0_is_support_fcm,
	.is_support_sharp = de351_mode0_is_support_sharp,
	.is_support_de_noise = de351_mode0_is_support_de_noise,
	.is_support_cdc = de351_mode0_is_support_cdc,
	.is_support_fbd = de351_mode0_is_support_fbd,
	.is_support_atw = de351_mode0_is_support_atw,
	.is_support_snr = de351_mode0_is_support_snr,

	.scaler_type = de351_mode0_scaler_type,
	.chn_id_lut = de351_mode0_chn_id_lut,
};
/* *********************** mode0 end **************************** */
#endif
