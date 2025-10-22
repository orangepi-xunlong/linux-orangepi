/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Sunxi eDP driver
 *
 * Copyright (C) 2022 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __SUNXI_EDP_H__
#define __SUNXI_EDP_H__

#define EDP_DPCD_MAX_LANE_MASK					(0x1f << 0)
#define EDP_DPCD_ENHANCE_FRAME_MASK				(1 << 7)
#define EDP_DPCD_TPS3_MASK						(1 << 6)
#define EDP_DPCD_FAST_TRAIN_MASK				(1 << 6)
#define EDP_DPCD_DOWNSTREAM_PORT_MASK			(1 << 0)
#define EDP_DPCD_DOWNSTREAM_PORT_TYPE_MASK		(3 << 1)
#define EDP_DPCD_DOWNSTREAM_PORT_CNT_MASK		(0xf << 0)
#define EDP_DPCD_LOCAL_EDID_MASK				(1 << 1)
#define EDP_DPCD_ASSR_MASK						(1 << 0)
#define EDP_DPCD_FRAME_CHANGE_MASK				(1 << 1)

typedef struct {
	u32 interface; /*0:I2S  1:SPDIF*/
	u32 chn_cnt;
	u32 data_width;
	bool mute;
} edp_audio_t;

typedef struct {
	s32 (*edp_audio_enable)(u32 sel);
	s32 (*edp_audio_disable)(u32 sel);
	s32 (*edp_audio_set_para)(u32 sel, edp_audio_t *audio_para);
} __edp_audio_func;

enum edp_hpd_status {
	EDP_HPD_PLUGOUT = 0,
	EDP_HPD_PLUGIN = 1,
};

enum edp_video_mapping_e {
	RGB_6BIT = 0,
	RGB_8BIT,
	RGB_10BIT,
	RGB_12BIT,
	RGB_16BIT,
	YCBCR444_8BIT,
	YCBCR444_10BIT,
	YCBCR444_12BIT,
	YCBCR444_16BIT,
	YCBCR422_8BIT,
	YCBCR422_10BIT,
	YCBCR422_12BIT,
	YCBCR422_16BIT,
};

enum edp_ssc_mode_e {
	SSC_CENTER_MODE = 0,
	SSC_DOWNSPR_MODE,
};

enum edp_pattern_e {
	PATTERN_NONE = 0,
	TPS1,
	TPS2,
	TPS3,
	TPS4,
	PRBS7,
};

#if IS_ENABLED(CONFIG_EDP2_AW_DISP2)
extern s32 snd_edp_get_func(__edp_audio_func *edp_func);
#else
static inline s32 snd_edp_get_func(__edp_audio_func *edp_func)
{

	return -1;
}
#endif

#endif
