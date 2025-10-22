/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * de350_feat.c
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
 * chn_id_lut:0~5 reserve for video channel, 6~11 reserve for ui channel
 *
 */
#include "de_feat.h"

/* *********************** mode0 begin ************************** */
static const u32 de350_mode0_num_de_disps[] = {
	2, /* RTMX0 */
};
static const u32 de350_mode0_skip_disps[] = {
	0, /* DISP0 */
};
static const s32 de350_mode0_num_chns[] = {
	6, /* DISP0 */
};
static const s32 de350_mode0_num_vi_chns[] = {
	3, /* DISP0 */
};
static const s32 de350_mode0_num_layers[] = {
	4, 4, 4, 4, 4, 4, 4, /* DISP0 */
};
static const s32 de350_mode0_is_support_vep[] = {
	1, 0, 1, 0, 0, 0, 0,/* DISP0 */
};
static const s32 de350_mode0_is_support_smbl[] = {
	1, /* DISP0 */
};
static const s32 de350_mode0_is_support_deband[] = {
	1, /* DISP0 */
};
static const s32 de350_mode0_is_support_dither[] = {
	1, /* DISP0 */
};
static const s32 de350_mode0_is_support_gamma[] = {
	1, /* DISP0 */
};
static const s32 de350_mode0_is_support_fmt[] = {
	1, /* DISP0 */
};
static const s32 de350_mode0_is_support_ksc[] = {
	1, /* DISP0 */
};
static const s32 de350_mode0_is_support_wb[] = {
	1, /* DISP0 */
};
static const s32 de350_mode0_is_support_scale[] = {
	1, 1, 1, 1, 1, 1, 1,/* DISP0 */
};
static const s32 de350_mode0_scale_line_buffer[] = {
	4096, /* DISP0 */
};
static const s32 de350_mode0_scale_line_buffer_yuv[] = {
	4096, 2048, 2560, 2560, 2560, 2048, 2048,/* DISP0 */
};
static const s32 de350_mode0_scale_line_buffer_rgb[] = {
	4096, 2048, 2560, 2560, 2560, 2048, 2048,/* DISP0 */
};
static const s32 de350_mode0_scale_line_buffer_ed[] = {
	4096, 2048, 2560, 2560, 2560, 2048, 2048,/* DISP0 */
};
static const s32 de350_mode0_is_support_edscale[] = {
	1, 0, 0, 0, 0, 0, 0,
};
static const s32 de350_mode0_is_support_asuscale[] = {
	1, 0, 1, 0, 0, 0, 0,
};
static const s32 de350_mode0_is_support_dci[] = {
	1, 0, 0, 0, 0, 0, 0,
};
static const s32 de350_mode0_is_support_sharp[] = {
	1, 0, 0, 0, 0, 0, 0,
};
static const s32 de350_mode0_is_support_fcm[] = {
	1, 0, 1, 0, 0, 0, 0,
};
static const s32 de350_mode0_is_support_de_noise[] = {
	0, 0, 0, 0, 0, 0, 0,
};
static const s32 de350_mode0_is_support_cdc[] = {
	1, 0, 0, 1, 0, 0, 0,
};
static const s32 de350_mode0_is_support_snr[] = {
	1, 0, 1, 0, 0, 0, 0,
};
static const s32 de350_mode0_is_support_fbd[] = {
	/* DISP0 CH0 */
	1, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 */
	0, 0, 0, 0,
	/* DISP0 CH3 */
	1, 0, 0, 0,
	/* DISP0 CH4 */
	1, 0, 0, 0,
	/* DISP0 CH5 */
	1, 0, 0, 0,
	/* DISP0 CH6 */
	0, 0, 0, 0,
};
static const s32 de350_mode0_is_support_tfbd[] = {
	/* DISP0 CH0 */
	0, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 */
	0, 0, 0, 0,
	/* DISP0 CH3 */
	0, 0, 0, 0,
	/* DISP0 CH4 */
	0, 0, 0, 0,
	/* DISP0 CH5 */
	0, 0, 0, 0,
	/* DISP0 CH6 */
	0, 0, 0, 0,
};
static const s32 de350_mode0_is_support_atw[] = {
	/* DISP0 CH0 */
	0, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 */
	0, 0, 0, 0,
	/* DISP0 CH3 */
	0, 0, 0, 0,
	/* DISP0 CH4 */
	0, 0, 0, 0,
	/* DISP0 CH5 */
	0, 0, 0, 0,
	/* DISP0 CH6 */
	0, 0, 0, 0,
};
static const u32 de350_mode0_scaler_type[] = {
	/* DISP0 CH0 */
	DE_SCALER_TYPE_ASU,
	/* DISP0 CH1 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH2 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH3 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH4 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH5 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH6 */
	DE_SCALER_TYPE_VSU8,
};
static const s32 de350_mode0_chn_id_lut[] = {
	0, 1, 2, /* DISP0, VIDEO CHANNEL */
	6, 7, 8, 9,/* DISP0, UI CHANNEL */
};

const struct de_feat de35x_mode0_features = {
	.num_screens = 1,
	.num_de_rtmx = 1,
	.num_de_disps = de350_mode0_num_de_disps,
	.num_skip_disps = de350_mode0_skip_disps,
	.num_chns = de350_mode0_num_chns,
	.num_vi_chns = de350_mode0_num_vi_chns,
	.num_layers = de350_mode0_num_layers,
	.is_support_vep = de350_mode0_is_support_vep,
	.is_support_smbl = de350_mode0_is_support_smbl,
	.is_support_deband = de350_mode0_is_support_deband,
	.is_support_gamma = de350_mode0_is_support_gamma,
	.is_support_dither = de350_mode0_is_support_dither,
	.is_support_fmt = de350_mode0_is_support_fmt,
	.is_support_ksc = de350_mode0_is_support_ksc,
	.is_support_wb = de350_mode0_is_support_wb,
	.is_support_scale = de350_mode0_is_support_scale,
	.scale_line_buffer_yuv = de350_mode0_scale_line_buffer_yuv,
	.scale_line_buffer_rgb = de350_mode0_scale_line_buffer_rgb,
	.scale_line_buffer_ed = de350_mode0_scale_line_buffer_ed,
	.is_support_edscale = de350_mode0_is_support_edscale,
	.is_support_asuscale = de350_mode0_is_support_asuscale,
	.is_support_dci = de350_mode0_is_support_dci,
	.is_support_fcm = de350_mode0_is_support_fcm,
	.is_support_sharp = de350_mode0_is_support_sharp,
	.is_support_de_noise = de350_mode0_is_support_de_noise,
	.is_support_cdc = de350_mode0_is_support_cdc,
	.is_support_fbd = de350_mode0_is_support_fbd,
	.is_support_tfbd = de350_mode0_is_support_tfbd,
	.is_support_atw = de350_mode0_is_support_atw,
	.is_support_snr = de350_mode0_is_support_snr,

	.scaler_type = de350_mode0_scaler_type,
	.chn_id_lut = de350_mode0_chn_id_lut,
};
/* *********************** mode0 end **************************** */

/* *********************** mode1 begin ************************** */
static const u32 de350_mode1_num_de_disps[] = {
	2, /* RTMX0 */
};
static const u32 de350_mode1_skip_disps[] = {
	0, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de350_mode1_num_chns[] = {
	4, /* DISP0 */
	3, /* DISP1 */
};
static const s32 de350_mode1_num_vi_chns[] = {
	1, /* DISP0 */
	2, /* DISP1 */
};
static const s32 de350_mode1_num_layers[] = {
	4, 4, 4, 4, /* DISP0 */
	4, 4, 4,    /* DISP1 */
};
static const s32 de350_mode1_is_support_vep[] = {
	1, 0, 0, 0, /* DISP0 */
	1, 0, 0,    /* DISP1 */
};
static const s32 de350_mode1_is_support_smbl[] = {
	1, /* DISP0 */
	1, /* DISP1 */
};
static const s32 de350_mode1_is_support_deband[] = {
	1, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de350_mode1_is_support_gamma[] = {
	1, /* DISP0 */
	1, /* DISP1 */
};
static const s32 de350_mode1_is_support_dither[] = {
	1, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de350_mode1_is_support_fmt[] = {
	1, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de350_mode1_is_support_ksc[] = {
	1, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de350_mode1_is_support_wb[] = {
	1, /* wb0 */
	1, /* no wb1 */
};
static const s32 de350_mode1_is_support_scale[] = {
	1, 1, 1, 1,/* DISP0 */
	1, 1, 1,   /* DISP1 */
};
static const s32 de350_mode1_scale_line_buffer[] = {
	4096, /* DISP0 */
	4096, /* DISP1 */
};
static const s32 de350_mode1_scale_line_buffer_yuv[] = {
	4096, 2560, 2560, 2048,/* DISP0 */
	2560, 2048, 2048,      /* DISP1 */
};
static const s32 de350_mode1_scale_line_buffer_rgb[] = {
	4096, 2560, 2560, 2048,/* DISP0 */
	2560, 2048, 2048,      /* DISP1 */
};
static const s32 de350_mode1_scale_line_buffer_ed[] = {
	4096, 2560, 2560, 2048,/* DISP0 */
	2056, 2048, 2048,      /* DISP1 */
};
static const s32 de350_mode1_is_support_edscale[] = {
	0, 0, 0, 0,/* DISP0 */
	0, 0, 0,   /* DISP1 */
};
static const s32 de350_mode1_is_support_asuscale[] = {
	1, 0, 0, 0,/* DISP0 */
	1, 0, 0,   /* DISP1 */
};
static const s32 de350_mode1_is_support_fcm[] = {
	1, 0, 0, 0,/* DISP0 */
	1, 0, 0,   /* DISP1 */
};
static const s32 de350_mode1_is_support_dci[] = {
	1, 0, 0, 0,/* DISP0 */
	0, 0, 0,   /* DISP1 */
};
static const s32 de350_mode1_is_support_sharp[] = {
	1, 0, 0, 0,/* DISP0 */
	0, 0, 0,   /* DISP1 */
};
static const s32 de350_mode1_is_support_de_noise[] = {
	0, 0, 0, 0,/* DISP0 */
	0, 0, 0,   /* DISP1 */
};
static const s32 de350_mode1_is_support_cdc[] = {
	1, 1, 0, 0,/* DISP0 */
	0, 0, 0,   /* DISP1 */
};
static const s32 de350_mode1_is_support_snr[] = {
	1, 0, 0, 0,/* DISP0 */
	1, 0, 0,   /* DISP1 */
};
static const s32 de350_mode1_is_support_fbd[] = {
	/* DISP0 CH0 */
	1, 0, 0, 0,
	/* DISP0 CH1 */
	1, 0, 0, 0,
	/* DISP0 CH2 */
	1, 0, 0, 0,
	/* DISP0 CH3 */
	1, 0, 0, 0,
	/* DISP1 CH0 */
	0, 0, 0, 0,
	/* DISP1 CH1 */
	0, 0, 0, 0,
	/* DISP1 CH2 */
	0, 0, 0, 0,
};
static const s32 de350_mode1_is_support_tfbd[] = {
	/* DISP0 CH0 */
	0, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 */
	0, 0, 0, 0,
	/* DISP0 CH3 */
	0, 0, 0, 0,
	/* DISP1 CH0 */
	0, 0, 0, 0,
	/* DISP1 CH1 */
	0, 0, 0, 0,
	/* DISP1 CH2 */
	0, 0, 0, 0,
};
static const s32 de350_mode1_is_support_atw[] = {
	/* DISP0 CH0 */
	0, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 */
	0, 0, 0, 0,
	/* DISP0 CH3 */
	0, 0, 0, 0,
	/* DISP1 CH0 */
	0, 0, 0, 0,
	/* DISP1 CH1 */
	0, 0, 0, 0,
	/* DISP1 CH0 */
	0, 0, 0, 0,
};
static const u32 de350_mode1_scaler_type[] = {
	/* DISP0 CH0 */
	DE_SCALER_TYPE_ASU,
	/* DISP0 CH1 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH2 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH3 */
	DE_SCALER_TYPE_VSU8,
	/* DISP1 CH0 */
	DE_SCALER_TYPE_VSU8,
	/* DISP1 CH1 */
	DE_SCALER_TYPE_VSU8,
	/* DISP1 CH2 */
	DE_SCALER_TYPE_VSU8,
};
static const s32 de350_mode1_chn_id_lut[] = {
	0, 6, 7, 8, /* DISP0 */
	2, 1, 9,    /* DISP1 */
};

const struct de_feat de35x_mode1_features = {
	.num_screens = 2,
	.num_de_rtmx = 1,
	.num_de_disps = de350_mode1_num_de_disps,
	.num_skip_disps = de350_mode1_skip_disps,
	.num_chns = de350_mode1_num_chns,
	.num_vi_chns = de350_mode1_num_vi_chns,
	.num_layers = de350_mode1_num_layers,
	.is_support_vep = de350_mode1_is_support_vep,
	.is_support_smbl = de350_mode1_is_support_smbl,
	.is_support_deband = de350_mode1_is_support_deband,
	.is_support_gamma = de350_mode1_is_support_gamma,
	.is_support_dither = de350_mode1_is_support_dither,
	.is_support_fmt = de350_mode1_is_support_fmt,
	.is_support_ksc = de350_mode1_is_support_ksc,
	.is_support_wb = de350_mode1_is_support_wb,
	.is_support_scale = de350_mode1_is_support_scale,
	.scale_line_buffer_yuv = de350_mode1_scale_line_buffer_yuv,
	.scale_line_buffer_rgb = de350_mode1_scale_line_buffer_rgb,
	.scale_line_buffer_ed = de350_mode1_scale_line_buffer_ed,
	.is_support_edscale = de350_mode1_is_support_edscale,
	.is_support_asuscale = de350_mode1_is_support_asuscale,
	.is_support_de_noise = de350_mode1_is_support_de_noise,
	.is_support_dci = de350_mode1_is_support_dci,
	.is_support_fcm = de350_mode1_is_support_fcm,
	.is_support_sharp = de350_mode1_is_support_sharp,
	.is_support_cdc = de350_mode1_is_support_cdc,
	.is_support_fbd = de350_mode1_is_support_fbd,
	.is_support_tfbd = de350_mode1_is_support_tfbd,
	.is_support_atw = de350_mode1_is_support_atw,
	.is_support_snr = de350_mode1_is_support_snr,

	.scaler_type = de350_mode1_scaler_type,
	.chn_id_lut = de350_mode1_chn_id_lut,
};
/* *********************** mode1 end **************************** */

/* *********************** mode2 begin ************************** */
static const u32 de350_mode2_num_de_disps[] = {
	2, /* RTMX0 */
};
static const u32 de350_mode2_skip_disps[] = {
	0, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de350_mode2_num_chns[] = {
	3, /* DISP0 */
	4, /* DISP1 */
};
static const s32 de350_mode2_num_vi_chns[] = {
	1, /* DISP0 */
	2, /* DISP1 */
};
static const s32 de350_mode2_num_layers[] = {
	4, 4, 4,   /* DISP0 */
	4, 4, 4, 4,/* DISP1 */
};
static const s32 de350_mode2_is_support_vep[] = {
	1, 0, 0,   /* DISP0 */
	1, 0, 0, 0,/* DISP1 */
};
static const s32 de350_mode2_is_support_smbl[] = {
	1, /* DISP0 */
	1, /* DISP1 */
};
static const s32 de350_mode2_is_support_deband[] = {
	1, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de350_mode2_is_support_gamma[] = {
	1, /* DISP0 */
	1, /* DISP1 */
};
static const s32 de350_mode2_is_support_dither[] = {
	1, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de350_mode2_is_support_fmt[] = {
	1, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de350_mode2_is_support_ksc[] = {
	1, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de350_mode2_is_support_wb[] = {
	1, /* DISP0 */
	1, /* DISP1 */
};
static const s32 de350_mode2_is_support_scale[] = {
	1, 1, 1,   /* DISP0 */
	1, 1, 1, 1,/* DISP1 */
};
static const s32 de350_mode2_scale_line_buffer[] = {
	4096, /* DISP0 */
	4096, /* DISP1 */
};
static const s32 de350_mode2_scale_line_buffer_yuv[] = {
	4096, 2560, 2560,     /* DISP0 */
	2560, 2048, 2048, 2048/* DISP1 */
};
static const s32 de350_mode2_scale_line_buffer_rgb[] = {
	4096, 2560, 2560,     /* DISP0 */
	2560, 2048, 2048, 2048/* DISP1 */
};
static const s32 de350_mode2_scale_line_buffer_ed[] = {
	4096, 2560, 2560,     /* DISP0 */
	2560, 2048, 2048, 2048/* DISP1 */
};
static const s32 de350_mode2_is_support_edscale[] = {
	0, 0, 0,/* DISP0 */
	0, 0, 0, 0,/* DISP1 */
};
static const s32 de350_mode2_is_support_asuscale[] = {
	1, 0, 0,/* DISP0 */
	1, 0, 0, 0/* DISP1 */
};
static const s32 de350_mode2_is_support_fcm[] = {
	1, 0, 0,/* DISP0 */
	1, 0, 0, 0/* DISP1 */
};
static const s32 de350_mode2_is_support_sharp[] = {
	1, 0, 0,/* DISP0 */
	0, 0, 0, 0/* DISP1 */
};
static const s32 de350_mode2_is_support_dci[] = {
	1, 0, 0,/* DISP0 */
	0, 0, 0, 0/* DISP1 */
};
static const s32 de350_mode2_is_support_de_noise[] = {
	0, 0, 0,/* DISP0 */
	0, 0, 0, 0, /* DISP1 */
};
static const s32 de350_mode2_is_support_cdc[] = {
	1, 1, 0,   /* DISP0 */
	0, 0, 0, 0,/* DISP1 */
};
static const s32 de350_mode2_is_support_snr[] = {
	1, 0, 0,   /* DISP0 */
	1, 0, 0, 0,/* DISP1 */
};
static const s32 de350_mode2_is_support_fbd[] = {
	/* DISP0 CH0 */
	1, 0, 0, 0,
	/* DISP0 CH1 */
	1, 0, 0, 0,
	/* DISP0 CH2 */
	1, 0, 0, 0,
	/* DISP1 CH0 */
	0, 0, 0, 0,
	/* DISP1 CH1 */
	0, 0, 0, 0,
	/* DISP1 CH2 */
	1, 0, 0, 0,
	/* DISP1 CH3 */
	0, 0, 0, 0,
};
static const s32 de350_mode2_is_support_tfbd[] = {
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
	/* DISP1 CH2 */
	0, 0, 0, 0,
	/* DISP1 CH3 */
	0, 0, 0, 0,
};
static const s32 de350_mode2_is_support_atw[] = {
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
	/* DISP1 CH2 */
	0, 0, 0, 0,
	/* DISP1 CH3 */
	0, 0, 0, 0,
};
static const u32 de350_mode2_scaler_type[] = {
	/* DISP0 CH0 */
	DE_SCALER_TYPE_ASU,
	/* DISP0 CH1 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH2 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH0 */
	DE_SCALER_TYPE_VSU8,
	/* DISP1 CH1 */
	DE_SCALER_TYPE_VSU8,
	/* DISP1 CH2 */
	DE_SCALER_TYPE_VSU8,
	/* DISP1 CH3 */
	DE_SCALER_TYPE_VSU8,
};
static const s32 de350_mode2_chn_id_lut[] = {
	0, 6, 7,    /* DISP0 */
	2, 1, 8, 9, /* DISP1 */
};

const struct de_feat de35x_mode2_features = {
	.num_screens = 2,
	.num_de_rtmx = 1,
	.num_de_disps = de350_mode2_num_de_disps,
	.num_skip_disps = de350_mode2_skip_disps,
	.num_chns = de350_mode2_num_chns,
	.num_vi_chns = de350_mode2_num_vi_chns,
	.num_layers = de350_mode2_num_layers,
	.is_support_vep = de350_mode2_is_support_vep,
	.is_support_smbl = de350_mode2_is_support_smbl,
	.is_support_deband = de350_mode2_is_support_deband,
	.is_support_gamma = de350_mode2_is_support_gamma,
	.is_support_dither = de350_mode2_is_support_dither,
	.is_support_fmt = de350_mode2_is_support_fmt,
	.is_support_ksc = de350_mode2_is_support_ksc,
	.is_support_wb = de350_mode2_is_support_wb,
	.is_support_scale = de350_mode2_is_support_scale,
	.scale_line_buffer_yuv = de350_mode2_scale_line_buffer_yuv,
	.scale_line_buffer_rgb = de350_mode2_scale_line_buffer_rgb,
	.scale_line_buffer_ed = de350_mode2_scale_line_buffer_ed,
	.is_support_edscale = de350_mode2_is_support_edscale,
	.is_support_asuscale = de350_mode2_is_support_asuscale,
	.is_support_dci = de350_mode2_is_support_dci,
	.is_support_fcm = de350_mode2_is_support_fcm,
	.is_support_sharp = de350_mode2_is_support_sharp,
	.is_support_de_noise = de350_mode2_is_support_de_noise,
	.is_support_cdc = de350_mode2_is_support_cdc,
	.is_support_fbd = de350_mode2_is_support_fbd,
	.is_support_tfbd = de350_mode2_is_support_tfbd,
	.is_support_atw = de350_mode2_is_support_atw,
	.is_support_snr = de350_mode2_is_support_snr,

	.scaler_type = de350_mode2_scaler_type,
	.chn_id_lut = de350_mode2_chn_id_lut,
};
/* *********************** mode2 end **************************** */

/* *********************** mode3 begin ************************** */
static const u32 de350_mode3_num_de_disps[] = {
	2, /* RTMX0 */
};
static const u32 de350_mode3_skip_disps[] = {
	0, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de350_mode3_num_chns[] = {
	4, /* DISP0 */
	3, /* DISP1 */
};
static const s32 de350_mode3_num_vi_chns[] = {
	2, /* DISP0 */
	1, /* DISP1 */
};
static const s32 de350_mode3_num_layers[] = {
	4, 4, 4, 4, /* DISP0 */
	4, 4, 4,    /* DISP1 */
};
static const s32 de350_mode3_is_support_vep[] = {
	1, 0, 0, 0,  /* DISP0 VCH0 VCH1 UCH0 UCH1 */
	1, 0, 0,     /* DISP1 VCH2 UCH2 UCH3 */
};
static const s32 de350_mode3_is_support_smbl[] = {
	1, /* DISP0 */
	1, /* DISP1 */
};
static const s32 de350_mode3_is_support_deband[] = {
	1, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de350_mode3_is_support_gamma[] = {
	1, /* DISP0 */
	1, /* DISP1 */
};
static const s32 de350_mode3_is_support_dither[] = {
	1, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de350_mode3_is_support_fmt[] = {
	1, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de350_mode3_is_support_ksc[] = {
	1, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de350_mode3_is_support_wb[] = {
	1, /* DISP0 */
	1, /* DISP1 */
};
static const s32 de350_mode3_is_support_scale[] = {
	1, 1, 1, 1,  /* DISP0 */
	1, 1, 1,    /* DISP1 */
};
static const s32 de350_mode3_scale_line_buffer[] = {
	4096, /* DISP0 */
	4096, /* DISP1 */
};
static const s32 de350_mode3_scale_line_buffer_yuv[] = {
	4096, 2560, 2560, 2560,  /* DISP0 VCH0 VCH1 UCH0 UCH1 */
	2048, 2048, 2048         /* DISP1 VCH2 UCH2 UCH3 */
};
static const s32 de350_mode3_scale_line_buffer_rgb[] = {
	2048, 2560, 2560, 2560,  /* DISP0 VCH0 VCH1 UCH0 UCH1 */
	2048, 2048, 2048,        /* DISP1 VCH2 UCH2 UCH3 */
};
static const s32 de350_mode3_scale_line_buffer_ed[] = {
	2048, 2560, 2560, 2560,  /* DISP0 VCH0 VCH1 UCH0 UCH1 */
	2048, 2048, 2048         /* DISP1 VCH2 UCH2 UCH3 */
};
static const s32 de350_mode3_is_support_edscale[] = {
	0, 0, 0, 0, /* DISP0 */
	0, 0, 0,    /* DISP1 */
};
static const s32 de350_mode3_is_support_asuscale[] = {
	1, 0, 0, 0,   /* DISP0 */
	0, 0, 0       /* DISP1 */
};
static const s32 de350_mode3_is_support_fcm[] = {
	1, 0, 0, 0,   /* DISP0 */
	1, 0, 0,      /* DISP1 */
};
static const s32 de350_mode3_is_support_sharp[] = {
	1, 0, 0, 0,   /* DISP0 */
	0, 0, 0,      /* DISP1 */
};
static const s32 de350_mode3_is_support_dci[] = {
	1, 0, 0, 0,   /* DISP0 */
	0, 0, 0,      /* DISP1 */
};
static const s32 de350_mode3_is_support_de_noise[] = {
	0, 0, 0,    /* DISP0 */
	0, 0, 0, 0, /* DISP1 */
};
static const s32 de350_mode3_is_support_cdc[] = {
	1, 0, 1,    /* DISP0 */
	0, 0, 0, 0, /* DISP1 */
};
static const s32 de350_mode3_is_support_snr[] = {
	1, 0, 0, 0,   /* DISP0 */
	1, 0, 0,  /* DISP1 */
};
static const s32 de350_mode3_is_support_fbd[] = {
	/* DISP0 CH0 VCH0 */
	1, 0, 0, 0,
	/* DISP0 CH1 VCH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 UCH0 */
	1, 0, 0, 0,
	/* DISP0 CH3 UCH1 */
	1, 0, 0, 0,
	/* DISP1 CH0 VCH2 */
	0, 0, 0, 0,
	/* DISP1 CH1 UCH2 */
	1, 0, 0, 0,
	/* DISP1 CH2 UCH3 */
	0, 0, 0, 0,
};
static const s32 de350_mode3_is_support_tfbd[] = {
	/* DISP0 CH0 VCH0 */
	0, 0, 0, 0,
	/* DISP0 CH1 VCH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 UCH0 */
	0, 0, 0, 0,
	/* DISP0 CH3 UCH1 */
	0, 0, 0, 0,
	/* DISP1 CH0 VCH2 */
	0, 0, 0, 0,
	/* DISP1 CH1 UCH2 */
	0, 0, 0, 0,
	/* DISP1 CH2 UCH3 */
	0, 0, 0, 0,
};
static const s32 de350_mode3_is_support_atw[] = {
	/* DISP0 CH0 */
	0, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 */
	0, 0, 0, 0,
	/* DISP0 CH3 */
	0, 0, 0, 0,
	/* DISP1 CH0 */
	0, 0, 0, 0,
	/* DISP1 CH1 */
	0, 0, 0, 0,
	/* DISP1 CH2 */
	0, 0, 0, 0,
};
static const u32 de350_mode3_scaler_type[] = {
	/* DISP0 CH0 */
	DE_SCALER_TYPE_ASU,
	/* DISP0 CH1 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH2 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH0 */
	DE_SCALER_TYPE_VSU8,
	/* DISP1 CH1 */
	DE_SCALER_TYPE_VSU8,
	/* DISP1 CH2 */
	DE_SCALER_TYPE_VSU8,
	/* DISP1 CH3 */
	DE_SCALER_TYPE_VSU8,
};
static const s32 de350_mode3_chn_id_lut[] = {
	0, 1, 6, 7,    /* DISP0 VCH0 VCH1 UCH0 UCH1 */
	2, 8, 9,       /* DISP1 VCH2 UCH2 UCH3 */
};

const struct de_feat de35x_mode3_features = {
	.num_screens = 2,
	.num_de_rtmx = 1,
	.num_de_disps = de350_mode3_num_de_disps,
	.num_skip_disps = de350_mode3_skip_disps,
	.num_chns = de350_mode3_num_chns,
	.num_vi_chns = de350_mode3_num_vi_chns,
	.num_layers = de350_mode3_num_layers,
	.is_support_vep = de350_mode3_is_support_vep,
	.is_support_smbl = de350_mode3_is_support_smbl,
	.is_support_deband = de350_mode3_is_support_deband,
	.is_support_gamma = de350_mode3_is_support_gamma,
	.is_support_dither = de350_mode3_is_support_dither,
	.is_support_fmt = de350_mode3_is_support_fmt,
	.is_support_ksc = de350_mode3_is_support_ksc,
	.is_support_wb = de350_mode3_is_support_wb,
	.is_support_scale = de350_mode3_is_support_scale,
	.scale_line_buffer_yuv = de350_mode3_scale_line_buffer_yuv,
	.scale_line_buffer_rgb = de350_mode3_scale_line_buffer_rgb,
	.scale_line_buffer_ed = de350_mode3_scale_line_buffer_ed,
	.is_support_edscale = de350_mode3_is_support_edscale,
	.is_support_asuscale = de350_mode3_is_support_asuscale,
	.is_support_dci = de350_mode3_is_support_dci,
	.is_support_fcm = de350_mode3_is_support_fcm,
	.is_support_sharp = de350_mode3_is_support_sharp,
	.is_support_de_noise = de350_mode3_is_support_de_noise,
	.is_support_cdc = de350_mode3_is_support_cdc,
	.is_support_fbd = de350_mode3_is_support_fbd,
	.is_support_tfbd = de350_mode3_is_support_tfbd,
	.is_support_atw = de350_mode3_is_support_atw,
	.is_support_snr = de350_mode3_is_support_snr,

	.scaler_type = de350_mode3_scaler_type,
	.chn_id_lut = de350_mode3_chn_id_lut,
};
/* *********************** mode3 end **************************** */

/* *********************** mode4 begin ************************** */
static const u32 de350_mode4_num_de_disps[] = {
	2, /* RTMX0 */
};
static const u32 de350_mode4_skip_disps[] = {
	0, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de350_mode4_num_chns[] = {
	4, /* DISP0 */
	3, /* DISP1 */
};
static const s32 de350_mode4_num_vi_chns[] = {
	2, /* DISP0 */
	1, /* DISP1 */
};
static const s32 de350_mode4_num_layers[] = {
	4, 4, 4, 4, /* DISP0 */
	4, 4, 4,    /* DISP1 */
};
static const s32 de350_mode4_is_support_vep[] = {
	1, 0, 0, 0,  /* DISP0 VCH0 VCH1 UCH0 UCH2 */
	1, 0, 0,     /* DISP1 VCH2 UCH1 UCH3 */
};
static const s32 de350_mode4_is_support_smbl[] = {
	1, /* DISP0 */
	1, /* DISP1 */
};
static const s32 de350_mode4_is_support_deband[] = {
	1, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de350_mode4_is_support_gamma[] = {
	1, /* DISP0 */
	1, /* DISP1 */
};
static const s32 de350_mode4_is_support_dither[] = {
	1, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de350_mode4_is_support_fmt[] = {
	1, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de350_mode4_is_support_ksc[] = {
	1, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de350_mode4_is_support_wb[] = {
	1, /* DISP0 */
	1, /* DISP1 */
};
static const s32 de350_mode4_is_support_scale[] = {
	1, 1, 1, 1,  /* DISP0 */
	1, 1, 1,    /* DISP1 */
};
static const s32 de350_mode4_scale_line_buffer[] = {
	4096, /* DISP0 */
	4096, /* DISP1 */
};
static const s32 de350_mode4_scale_line_buffer_yuv[] = {
	4096, 2560, 2560, 2048,  /* DISP0 VCH0 VCH1 UCH0 UCH2 */
	2048, 2560, 2048         /* DISP1 VCH2 UCH1 UCH3 */
};
static const s32 de350_mode4_scale_line_buffer_rgb[] = {
	2048, 2560, 2560, 2048,  /* DISP0 VCH0 VCH1 UCH0 UCH2 */
	2048, 2560, 2048,        /* DISP1 VCH2 UCH1 UCH3 */
};
static const s32 de350_mode4_scale_line_buffer_ed[] = {
	2048, 2560, 2560, 2048,  /* DISP0 VCH0 VCH1 UCH0 UCH2 */
	2048, 2560, 2048         /* DISP1 VCH2 UCH1 UCH3 */
};
static const s32 de350_mode4_is_support_edscale[] = {
	0, 0, 0, 0, /* DISP0 */
	0, 0, 0,    /* DISP1 */
};
static const s32 de350_mode4_is_support_asuscale[] = {
	1, 0, 0, 0,   /* DISP0 */
	0, 0, 0       /* DISP1 */
};
static const s32 de350_mode4_is_support_fcm[] = {
	1, 0, 0, 0,   /* DISP0 */
	1, 0, 0,      /* DISP1 */
};
static const s32 de350_mode4_is_support_sharp[] = {
	1, 0, 0, 0,   /* DISP0 */
	0, 0, 0,      /* DISP1 */
};
static const s32 de350_mode4_is_support_dci[] = {
	1, 0, 0, 0,   /* DISP0 */
	0, 0, 0,      /* DISP1 */
};
static const s32 de350_mode4_is_support_de_noise[] = {
	0, 0, 0,    /* DISP0 */
	0, 0, 0, 0, /* DISP1 */
};
static const s32 de350_mode4_is_support_cdc[] = {
	1, 0, 1,    /* DISP0 */
	0, 0, 0, 0, /* DISP1 */
};
static const s32 de350_mode4_is_support_snr[] = {
	1, 0, 0, 0,   /* DISP0 */
	1, 0, 0,  /* DISP1 */
};
static const s32 de350_mode4_is_support_fbd[] = {
	/* DISP0 CH0 VCH0 */
	1, 0, 0, 0,
	/* DISP0 CH1 VCH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 UCH0 */
	1, 0, 0, 0,
	/* DISP0 CH3 UCH2 */
	1, 0, 0, 0,
	/* DISP1 CH0 VCH2 */
	0, 0, 0, 0,
	/* DISP1 CH1 UCH1 */
	1, 0, 0, 0,
	/* DISP1 CH2 UCH3 */
	0, 0, 0, 0,
};
static const s32 de350_mode4_is_support_tfbd[] = {
	/* DISP0 CH0 VCH0 */
	0, 0, 0, 0,
	/* DISP0 CH1 VCH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 UCH0 */
	0, 0, 0, 0,
	/* DISP0 CH3 UCH2 */
	0, 0, 0, 0,
	/* DISP1 CH0 VCH2 */
	0, 0, 0, 0,
	/* DISP1 CH1 UCH1 */
	0, 0, 0, 0,
	/* DISP1 CH2 UCH3 */
	0, 0, 0, 0,
};
static const s32 de350_mode4_is_support_atw[] = {
	/* DISP0 CH0 */
	0, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 */
	0, 0, 0, 0,
	/* DISP0 CH3 */
	0, 0, 0, 0,
	/* DISP1 CH0 */
	0, 0, 0, 0,
	/* DISP1 CH1 */
	0, 0, 0, 0,
	/* DISP1 CH2 */
	0, 0, 0, 0,
};
static const u32 de350_mode4_scaler_type[] = {
	/* DISP0 CH0 */
	DE_SCALER_TYPE_ASU,
	/* DISP0 CH1 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH2 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH0 */
	DE_SCALER_TYPE_VSU8,
	/* DISP1 CH1 */
	DE_SCALER_TYPE_VSU8,
	/* DISP1 CH2 */
	DE_SCALER_TYPE_VSU8,
	/* DISP1 CH3 */
	DE_SCALER_TYPE_VSU8,
};
static const s32 de350_mode4_chn_id_lut[] = {
	0, 1, 6, 8,    /* DISP0 VCH0 VCH1 UCH0 UCH2 */
	2, 7, 9,       /* DISP1 VCH2 UCH1 UCH3 */
};

const struct de_feat de35x_mode4_features = {
	.num_screens = 2,
	.num_de_rtmx = 1,
	.num_de_disps = de350_mode4_num_de_disps,
	.num_skip_disps = de350_mode4_skip_disps,
	.num_chns = de350_mode4_num_chns,
	.num_vi_chns = de350_mode4_num_vi_chns,
	.num_layers = de350_mode4_num_layers,
	.is_support_vep = de350_mode4_is_support_vep,
	.is_support_smbl = de350_mode4_is_support_smbl,
	.is_support_deband = de350_mode4_is_support_deband,
	.is_support_gamma = de350_mode4_is_support_gamma,
	.is_support_dither = de350_mode4_is_support_dither,
	.is_support_fmt = de350_mode4_is_support_fmt,
	.is_support_ksc = de350_mode4_is_support_ksc,
	.is_support_wb = de350_mode4_is_support_wb,
	.is_support_scale = de350_mode4_is_support_scale,
	.scale_line_buffer_yuv = de350_mode4_scale_line_buffer_yuv,
	.scale_line_buffer_rgb = de350_mode4_scale_line_buffer_rgb,
	.scale_line_buffer_ed = de350_mode4_scale_line_buffer_ed,
	.is_support_edscale = de350_mode4_is_support_edscale,
	.is_support_asuscale = de350_mode4_is_support_asuscale,
	.is_support_dci = de350_mode4_is_support_dci,
	.is_support_fcm = de350_mode4_is_support_fcm,
	.is_support_sharp = de350_mode4_is_support_sharp,
	.is_support_de_noise = de350_mode4_is_support_de_noise,
	.is_support_cdc = de350_mode4_is_support_cdc,
	.is_support_fbd = de350_mode4_is_support_fbd,
	.is_support_tfbd = de350_mode4_is_support_tfbd,
	.is_support_atw = de350_mode4_is_support_atw,
	.is_support_snr = de350_mode4_is_support_snr,

	.scaler_type = de350_mode4_scaler_type,
	.chn_id_lut = de350_mode4_chn_id_lut,
};
/* *********************** mode4 end **************************** */
