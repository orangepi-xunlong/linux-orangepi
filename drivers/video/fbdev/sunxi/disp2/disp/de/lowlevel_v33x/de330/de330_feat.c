/*
 * drivers/video/fbdev/sunxi/disp2/disp/de/lowlevel_v33x/de330/de330_feat/de330_feat.c
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
#include "de_feat.h"

/* *********************** mode0 begin ************************** */
static const s32 de330_mode0_num_chns[] = {
	6, /* DISP0 */
};
static const s32 de330_mode0_num_vi_chns[] = {
	3, /* DISP0 */
};
static const s32 de330_mode0_num_layers[] = {
	4, 4, 4, 4, 4, 4, /* DISP0 */
};
static const s32 de330_mode0_is_support_vep[] = {
	1, 0, 0, 0, 0, 0, /* DISP0 */
};
static const s32 de330_mode0_is_support_smbl[] = {
	1, /* DISP0 */
};
static const s32 de330_mode0_is_support_wb[] = {
	1, /* DISP0 */
};
static const s32 de330_mode0_is_support_scale[] = {
	1, 1, 1, 1, 1, 1, /* DISP0 */
};
static const s32 de330_mode0_scale_line_buffer[] = {
	4096, /* DISP0 */
};
static const s32 de330_mode0_scale_line_buffer_yuv[] = {
	4096, 2048, 2048, 2048, 2048, 2048, /* DISP0 */
};
static const s32 de330_mode0_scale_line_buffer_rgb[] = {
	4096, 2048, 2048, 2048, 2048, 2048, /* DISP0 */
};
static const s32 de330_mode0_scale_line_buffer_ed[] = {
	4096, 2048, 2048, 2048, 2048, 2048, /* DISP0 */
};
static const s32 de330_mode0_is_support_edscale[] = {
	1, 0, 0, 0, 0, 0,
};
static const s32 de330_mode0_is_support_de_noise[] = {
	1, 0, 0, 0, 0, 0,
};
static const s32 de330_mode0_is_support_cdc[] = {
	1, 1, 0, 0, 0, 0,
};
static const s32 de330_mode0_is_support_snr[] = {
	0, 0, 0, 0, 0, 0,
};
static const s32 de330_mode0_is_support_fbd[] = {
	/* DISP0 CH0 */
	1, 0, 0, 0,
	/* DISP0 CH1 */
	1, 0, 0, 0,
	/* DISP0 CH2 */
	0, 0, 0, 0,
	/* DISP0 CH3 */
	1, 0, 0, 0,
	/* DISP0 CH4 */
	0, 0, 0, 0,
	/* DISP0 CH5 */
	0, 0, 0, 0,
};
static const s32 de330_mode0_is_support_atw[] = {
	/* DISP0 CH0 */
	1, 0, 0, 0,
	/* DISP0 CH1 */
	1, 0, 0, 0,
	/* DISP0 CH2 */
	0, 0, 0, 0,
	/* DISP0 CH3 */
	0, 0, 0, 0,
	/* DISP0 CH4 */
	0, 0, 0, 0,
	/* DISP0 CH5 */
	0, 0, 0, 0,
};
static const u32 de330_mode0_scaler_type[] = {
	/* DISP0 CH0 */
	DE_SCALER_TYPE_VSU_ED,
	/* DISP0 CH1 */
	DE_SCALER_TYPE_VSU10,
	/* DISP0 CH2 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH3 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH4 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH5 */
	DE_SCALER_TYPE_VSU8,
};
static const s32 de330_mode0_chn_id_lut[] = {
	0, 1, 2, /* DISP0, VIDEO CHANNEL */
	6, 7, 8, /* DISP0, UI CHANNEL */
};

const struct de_feat de330_mode0_features = {
	.num_screens = 1,
	.num_chns = de330_mode0_num_chns,
	.num_vi_chns = de330_mode0_num_vi_chns,
	.num_layers = de330_mode0_num_layers,
	.is_support_vep = de330_mode0_is_support_vep,
	.is_support_smbl = de330_mode0_is_support_smbl,
	.is_support_wb = de330_mode0_is_support_wb,
	.is_support_scale = de330_mode0_is_support_scale,
	.scale_line_buffer_yuv = de330_mode0_scale_line_buffer_yuv,
	.scale_line_buffer_rgb = de330_mode0_scale_line_buffer_rgb,
	.scale_line_buffer_ed = de330_mode0_scale_line_buffer_ed,
	.is_support_edscale = de330_mode0_is_support_edscale,
	.is_support_de_noise = de330_mode0_is_support_de_noise,
	.is_support_cdc = de330_mode0_is_support_cdc,
	.is_support_fbd = de330_mode0_is_support_fbd,
	.is_support_atw = de330_mode0_is_support_atw,
	.is_support_snr = de330_mode0_is_support_snr,

	.scaler_type = de330_mode0_scaler_type,
	.chn_id_lut = de330_mode0_chn_id_lut,
};
/* *********************** mode0 end **************************** */

/* *********************** mode1 begin ************************** */
static const s32 de330_mode1_num_chns[] = {
	4, /* DISP0 */
	2, /* DISP1 */
};
static const s32 de330_mode1_num_vi_chns[] = {
	2, /* DISP0 */
	1, /* DISP1 */
};
static const s32 de330_mode1_num_layers[] = {
	4, 4, 4, 4, /* DISP0 */
	4, 4,       /* DISP1 */
};
static const s32 de330_mode1_is_support_vep[] = {
	1, 0, 0, 0, /* DISP0 */
	0, 0,       /* DISP1 */
};
static const s32 de330_mode1_is_support_smbl[] = {
	1, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de330_mode1_is_support_wb[] = {
	1, /* wb0 */
	1, /* no wb1 */
};
static const s32 de330_mode1_is_support_scale[] = {
	1, 1, 1, 1,/* DISP0 */
	1, 1,      /* DISP1 */
};
static const s32 de330_mode1_scale_line_buffer[] = {
	4096, /* DISP0 */
	4096, /* DISP1 */
};
static const s32 de330_mode1_scale_line_buffer_yuv[] = {
	4096, 2048, 2048, 2048,/* DISP0 */
	2048, 2048,            /* DISP1 */
};
static const s32 de330_mode1_scale_line_buffer_rgb[] = {
	4096, 2048, 2048, 2048,/* DISP0 */
	2048, 2048,            /* DISP1 */
};
static const s32 de330_mode1_scale_line_buffer_ed[] = {
	4096, 2048, 2048, 2048,/* DISP0 */
	2048, 2048,            /* DISP1 */
};
static const s32 de330_mode1_is_support_edscale[] = {
	1, 0, 0, 0,/* DISP0 */
	0, 0,      /* DISP1 */
};
static const s32 de330_mode1_is_support_de_noise[] = {
	1, 0, 0, 0,/* DISP0 */
	0, 0,      /* DISP1 */
};
static const s32 de330_mode1_is_support_cdc[] = {
	1, 1, 0, 0,/* DISP0 */
	0, 0,      /* DISP1 */
};

static const s32 de330_mode1_is_support_snr[] = {
	0, 0, 0, 0,/* DISP0 */
	0, 0,      /* DISP1 */
};
static const s32 de330_mode1_is_support_fbd[] = {
	/* DISP0 CH0 */
	1, 0, 0, 0,
	/* DISP0 CH1 */
	1, 0, 0, 0,
	/* DISP0 CH2 */
	1, 0, 0, 0,
	/* DISP0 CH3 */
	0, 0, 0, 0,
	/* DISP1 CH0 */
	0, 0, 0, 0,
	/* DISP1 CH1 */
	0, 0, 0, 0,
};
static const s32 de330_mode1_is_support_atw[] = {
	/* DISP0 CH0 */
	1, 0, 0, 0,
	/* DISP0 CH1 */
	1, 0, 0, 0,
	/* DISP0 CH2 */
	0, 0, 0, 0,
	/* DISP0 CH3 */
	0, 0, 0, 0,
	/* DISP1 CH0 */
	0, 0, 0, 0,
	/* DISP1 CH1 */
	0, 0, 0, 0,
};
static const u32 de330_mode1_scaler_type[] = {
	/* DISP0 CH0 */
	DE_SCALER_TYPE_VSU_ED,
	/* DISP0 CH1 */
	DE_SCALER_TYPE_VSU10,
	/* DISP0 CH2 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH3 */
	DE_SCALER_TYPE_VSU8,
	/* DISP1 CH0 */
	DE_SCALER_TYPE_VSU8,
	/* DISP1 CH1 */
	DE_SCALER_TYPE_VSU8,
};
static const s32 de330_mode1_chn_id_lut[] = {
	0, 1, 6, 7, /* DISP0 */
	2, 8,       /* DISP1 */
};

const struct de_feat de330_mode1_features = {
	.num_screens = 2,
	.num_chns = de330_mode1_num_chns,
	.num_vi_chns = de330_mode1_num_vi_chns,
	.num_layers = de330_mode1_num_layers,
	.is_support_vep = de330_mode1_is_support_vep,
	.is_support_smbl = de330_mode1_is_support_smbl,
	.is_support_wb = de330_mode1_is_support_wb,
	.is_support_scale = de330_mode1_is_support_scale,
	.scale_line_buffer_yuv = de330_mode1_scale_line_buffer_yuv,
	.scale_line_buffer_rgb = de330_mode1_scale_line_buffer_rgb,
	.scale_line_buffer_ed = de330_mode1_scale_line_buffer_ed,
	.is_support_edscale = de330_mode1_is_support_edscale,
	.is_support_de_noise = de330_mode1_is_support_de_noise,
	.is_support_cdc = de330_mode1_is_support_cdc,
	.is_support_fbd = de330_mode1_is_support_fbd,
	.is_support_atw = de330_mode1_is_support_atw,
	.is_support_snr = de330_mode1_is_support_snr,

	.scaler_type = de330_mode1_scaler_type,
	.chn_id_lut = de330_mode1_chn_id_lut,
};
/* *********************** mode1 end **************************** */

/* *********************** mode2 begin ************************** */
static const s32 de330_mode2_num_chns[] = {
	3, /* DISP0 */
	3, /* DISP1 */
};
static const s32 de330_mode2_num_vi_chns[] = {
	1, /* DISP0 */
	2, /* DISP1 */
};
static const s32 de330_mode2_num_layers[] = {
	4, 4, 4, /* DISP0 */
	4, 4, 4, /* DISP1 */
};
static const s32 de330_mode2_is_support_vep[] = {
	1, 0, 0, /* DISP0 */
	0, 0, 0, /* DISP1 */
};
static const s32 de330_mode2_is_support_smbl[] = {
	1, /* DISP0 */
	0, /* DISP1 */
};
static const s32 de330_mode2_is_support_wb[] = {
	1, /* DISP0 */
	1, /* DISP1 */
};
static const s32 de330_mode2_is_support_scale[] = {
	1, 1, 1,/* DISP0 */
	1, 1, 1,/* DISP1 */
};
static const s32 de330_mode2_scale_line_buffer[] = {
	4096, /* DISP0 */
	4096, /* DISP1 */
};
static const s32 de330_mode2_scale_line_buffer_yuv[] = {
	4096, 2048, 2048,/* DISP0 */
	2048, 2048, 2048,/* DISP1 */
};
static const s32 de330_mode2_scale_line_buffer_rgb[] = {
	4096, 2048, 2048,/* DISP0 */
	2048, 2048, 2048,/* DISP1 */
};
static const s32 de330_mode2_scale_line_buffer_ed[] = {
	4096, 2048, 2048,/* DISP0 */
	2048, 2048, 2048,/* DISP1 */
};
static const s32 de330_mode2_is_support_edscale[] = {
	1, 0, 0,/* DISP0 */
	0, 0, 0,/* DISP1 */
};
static const s32 de330_mode2_is_support_de_noise[] = {
	1, 0, 0,/* DISP0 */
	0, 0, 0,/* DISP1 */
};
static const s32 de330_mode2_is_support_cdc[] = {
	1, 0, 0,/* DISP0 */
	1, 0, 0,/* DISP1 */
};
static const s32 de330_mode2_is_support_snr[] = {
	0, 0, 0,/* DISP0 */
	0, 0, 0,/* DISP1 */
};
static const s32 de330_mode2_is_support_fbd[] = {
	/* DISP0 CH0 */
	1, 0, 0, 0,
	/* DISP0 CH1 */
	1, 0, 0, 0,
	/* DISP0 CH2 */
	0, 0, 0, 0,
	/* DISP1 CH0 */
	1, 0, 0, 0,
	/* DISP1 CH1 */
	0, 0, 0, 0,
	/* DISP1 CH2 */
	0, 0, 0, 0,
};
static const s32 de330_mode2_is_support_atw[] = {
	/* DISP0 CH0 */
	1, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 */
	0, 0, 0, 0,
	/* DISP1 CH0 */
	1, 0, 0, 0,
	/* DISP1 CH1 */
	0, 0, 0, 0,
	/* DISP1 CH2 */
	0, 0, 0, 0,
};
static const u32 de330_mode2_scaler_type[] = {
	/* DISP0 CH0 */
	DE_SCALER_TYPE_VSU_ED,
	/* DISP0 CH1 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH2 */
	DE_SCALER_TYPE_VSU8,
	/* DISP0 CH0 */
	DE_SCALER_TYPE_VSU10,
	/* DISP1 CH1 */
	DE_SCALER_TYPE_VSU8,
	/* DISP1 CH2 */
	DE_SCALER_TYPE_VSU8,
};
static const s32 de330_mode2_chn_id_lut[] = {
	0, 6, 7, /* DISP0 */
	1, 2, 8, /* DISP1 */
};

const struct de_feat de330_mode2_features = {
	.num_screens = 2,
	.num_chns = de330_mode2_num_chns,
	.num_vi_chns = de330_mode2_num_vi_chns,
	.num_layers = de330_mode2_num_layers,
	.is_support_vep = de330_mode2_is_support_vep,
	.is_support_smbl = de330_mode2_is_support_smbl,
	.is_support_wb = de330_mode2_is_support_wb,
	.is_support_scale = de330_mode2_is_support_scale,
	.scale_line_buffer_yuv = de330_mode2_scale_line_buffer_yuv,
	.scale_line_buffer_rgb = de330_mode2_scale_line_buffer_rgb,
	.scale_line_buffer_ed = de330_mode2_scale_line_buffer_ed,
	.is_support_edscale = de330_mode2_is_support_edscale,
	.is_support_de_noise = de330_mode2_is_support_de_noise,
	.is_support_cdc = de330_mode2_is_support_cdc,
	.is_support_fbd = de330_mode2_is_support_fbd,
	.is_support_atw = de330_mode2_is_support_atw,
	.is_support_snr = de330_mode2_is_support_snr,

	.scaler_type = de330_mode2_scaler_type,
	.chn_id_lut = de330_mode2_chn_id_lut,
};
/* *********************** mode1 end **************************** */
