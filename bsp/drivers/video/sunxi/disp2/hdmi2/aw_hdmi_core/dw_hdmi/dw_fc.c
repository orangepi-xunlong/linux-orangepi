/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <sunxi-log.h>
#include "dw_access.h"
#include "dw_audio.h"
#include "dw_fc.h"

#define FC_GMD_PB_SIZE			28
#define FC_ACP_TX		       (0)
#define FC_ISRC1_TX		       (1)
#define FC_ISRC2_TX		       (2)
#define FC_SPD_TX		       (4)
#define FC_VSD_TX		       (3)

channel_count_t channel_cnt[] = {
	{0x00, 1}, {0x01, 2}, {0x02, 2}, {0x04, 2}, {0x03, 3}, {0x05, 3},
	{0x06, 3}, {0x08, 3}, {0x14, 3}, {0x07, 4}, {0x09, 4}, {0x0A, 4},
	{0x0C, 4}, {0x15, 4}, {0x16, 4}, {0x18, 4}, {0x0B, 5}, {0x0D, 5},
	{0x0E, 5}, {0x10, 5}, {0x17, 5}, {0x19, 5}, {0x1A, 5}, {0x1C, 5},
	{0x20, 5}, {0x22, 5}, {0x24, 5}, {0x26, 5}, {0x0F, 6}, {0x11, 6},
	{0x12, 6}, {0x1B, 6}, {0x1D, 6}, {0x1E, 6}, {0x21, 6}, {0x23, 6},
	{0x25, 6}, {0x27, 6}, {0x28, 6}, {0x2A, 6}, {0x2C, 6}, {0x2E, 6},
	{0x30, 6}, {0x13, 7}, {0x1F, 7}, {0x29, 7}, {0x2B, 7}, {0x2D, 7},
	{0x2F, 7}, {0x31, 7}, {0, 0},
};

/* sampling frequency: unit:Hz */
iec_params_t iec_original_sampling_freq_values[] = {
	{{.frequency = 44100}, 0xF},
	{{.frequency = 88200}, 0x7},
	{{.frequency = 22050}, 0xB},
	{{.frequency = 176400}, 0x3},
	{{.frequency = 48000}, 0xD},
	{{.frequency = 96000}, 0x5},
	{{.frequency = 24000}, 0x9},
	{{.frequency = 192000}, 0x1},
	{{.frequency =  8000}, 0x6},
	{{.frequency = 11025}, 0xA},
	{{.frequency = 12000}, 0x2},
	{{.frequency = 32000}, 0xC},
	{{.frequency = 16000}, 0x8},
	{{.frequency = 0},      0x0}
};

iec_params_t iec_sampling_freq_values[] = {
	{{.frequency = 22050}, 0x4},
	{{.frequency = 44100}, 0x0},
	{{.frequency = 88200}, 0x8},
	{{.frequency = 176400}, 0xC},
	{{.frequency = 24000}, 0x6},
	{{.frequency = 48000}, 0x2},
	{{.frequency = 96000}, 0xA},
	{{.frequency = 192000}, 0xE},
	{{.frequency = 32000}, 0x3},
	{{.frequency = 768000}, 0x9},
	{{.frequency = 0},      0x0}
};

iec_params_t iec_word_length[] = {
	{{.sample_size = 16}, 0x2},
	{{.sample_size = 17}, 0xC},
	{{.sample_size = 18}, 0x4},
	{{.sample_size = 19}, 0x8},
	{{.sample_size = 20}, 0x3},
	{{.sample_size = 21}, 0xD},
	{{.sample_size = 22}, 0x5},
	{{.sample_size = 23}, 0x9},
	{{.sample_size = 24}, 0xB},
	{{.sample_size = 0},  0x0}
};

/**
 * @param params pointer to the audio parameters structure
 * @return number of audio channels transmitted -1
 */
u8 _audio_get_channel_count(dw_hdmi_dev_t *dev, dw_audio_param_t *params)
{
	int i = 0;

	for (i = 0; channel_cnt[i].channel_count != 0; i++) {
		if (channel_cnt[i].channel_allocation ==
					params->mChannelAllocation) {
			return channel_cnt[i].channel_count;
		}
	}

	return 0;
}

/**
 * @param params pointer to the audio parameters structure
 */
u8 _audio_iec_original_sampling_freq(dw_hdmi_dev_t *dev, dw_audio_param_t *params)
{
	int i = 0;

	for (i = 0; iec_original_sampling_freq_values[i].iec.frequency != 0; i++) {
		if (params->mSamplingFrequency ==
			iec_original_sampling_freq_values[i].iec.frequency) {
			u8 value = iec_original_sampling_freq_values[i].value;
			return value;
		}
	}

	return 0x0;
}

/**
 * @param params pointer to the audio parameters structure
 */
u8 _audio_iec_sampling_freq(dw_hdmi_dev_t *dev, dw_audio_param_t *params)
{
	int i = 0;

	for (i = 0; iec_sampling_freq_values[i].iec.frequency != 0; i++) {
		if (params->mSamplingFrequency ==
			iec_sampling_freq_values[i].iec.frequency) {
			u8 value = iec_sampling_freq_values[i].value;
			return value;
		}
	}

	/* Not indicated */
	return 0x1;
}

/**
 * @param params pointer to the audio parameters structure
 */
u8 _audio_iec_word_length(dw_hdmi_dev_t *dev, dw_audio_param_t *params)
{
	int i = 0;

	for (i = 0; iec_word_length[i].iec.sample_size != 0; i++) {
		if (params->mSampleSize == iec_word_length[i].iec.sample_size)
			return iec_word_length[i].value;
	}

	return 0x0;
}

/**
 * return if channel is enabled or not using the user's channel allocation
 * code
 * @param params pointer to the audio parameters structure
 * @param channel in question -1
 * @return 1 if channel is to be enabled, 0 otherwise
 */
u8 _audio_check_channel_en(dw_hdmi_dev_t *dev, dw_audio_param_t *params, u8 channel)
{
	switch (channel) {
	case 0:
	case 1:
		AUDIO_INF("channal %d is enable\n", channel);
		return 1;
	case 2:
		AUDIO_INF("channal %d is enable\n", channel);
		return params->mChannelAllocation & BIT(0);
	case 3:
		AUDIO_INF("channal %d is enable\n", channel);
		return (params->mChannelAllocation & BIT(1)) >> 1;
	case 4:
		if (((params->mChannelAllocation > 0x03) &&
			(params->mChannelAllocation < 0x14)) ||
			((params->mChannelAllocation > 0x17) &&
			(params->mChannelAllocation < 0x20))) {
			AUDIO_INF("channal %d is enable\n", channel);
			return 1;
		} else {
			return 0;
		}
	case 5:
		if (((params->mChannelAllocation > 0x07) &&
			(params->mChannelAllocation < 0x14)) ||
			((params->mChannelAllocation > 0x1C) &&
			(params->mChannelAllocation < 0x20))) {
			AUDIO_INF("channal %d is enable\n", channel);
			return 1;
		} else {
			return 0;
		}
	case 6:
		if ((params->mChannelAllocation > 0x0B) &&
			(params->mChannelAllocation < 0x20)) {
			AUDIO_INF("channal %d is enable\n", channel);
			return 1;
		} else {
			return 0;
		}
	case 7:
		AUDIO_INF("channal %d is enable\n", channel);
		return (params->mChannelAllocation & BIT(4)) >> 4;
	default:
		return 0;
	}
}

u8 _dw_fc_audio_get_iec_word_length(dw_hdmi_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev, FC_AUDSCHNL8, FC_AUDSCHNL8_OIEC_WORDLENGTH_MASK);
}

u8 _dw_fc_audio_get_iec_sample_freq(dw_hdmi_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev,  FC_AUDSCHNL7, FC_AUDSCHNL7_OIEC_SAMPFREQ_MASK);
}

void _dw_fc_audio_set_channel_right(dw_hdmi_dev_t *dev,
		u8 value, unsigned channel)
{
	LOG_TRACE2(value, channel);
	if (channel == 0)
		dev_write_mask(dev, FC_AUDSCHNL3,
				FC_AUDSCHNL3_OIEC_CHANNELNUMCR0_MASK, value);
	else if (channel == 1)
		dev_write_mask(dev, FC_AUDSCHNL3,
				FC_AUDSCHNL3_OIEC_CHANNELNUMCR1_MASK, value);
	else if (channel == 2)
		dev_write_mask(dev, FC_AUDSCHNL4,
				FC_AUDSCHNL4_OIEC_CHANNELNUMCR2_MASK, value);
	else if (channel == 3)
		dev_write_mask(dev, FC_AUDSCHNL4,
				FC_AUDSCHNL4_OIEC_CHANNELNUMCR3_MASK, value);
	else
		hdmi_err("%s: invalid channel number: %d\n", __func__, channel);
}

void _dw_fc_audio_set_channel_left(dw_hdmi_dev_t *dev,
		u8 value, unsigned channel)
{
	LOG_TRACE2(value, channel);
	if (channel == 0)
		dev_write_mask(dev, FC_AUDSCHNL5,
				FC_AUDSCHNL5_OIEC_CHANNELNUMCL0_MASK, value);
	else if (channel == 1)
		dev_write_mask(dev, FC_AUDSCHNL5,
				FC_AUDSCHNL5_OIEC_CHANNELNUMCL1_MASK, value);
	else if (channel == 2)
		dev_write_mask(dev, FC_AUDSCHNL6,
				FC_AUDSCHNL6_OIEC_CHANNELNUMCL2_MASK, value);
	else if (channel == 3)
		dev_write_mask(dev, FC_AUDSCHNL6,
				FC_AUDSCHNL6_OIEC_CHANNELNUMCL3_MASK, value);
	else
		hdmi_err("%s: invalid channel number: %d\n", __func__, channel);
}

void dw_fc_audio_force(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, FC_DBGFORCE, FC_DBGFORCE_FORCEAUDIO_MASK, bit);
}

void dw_fc_audio_set_mute(dw_hdmi_dev_t *dev, u8 mute)
{
	if (mute)
		dev_write_mask(dev, FC_AUDSCONF, FC_AUDSCONF_AUD_PACKET_SAMPFLT_MASK, 0xF);
	else
		dev_write_mask(dev, FC_AUDSCONF, FC_AUDSCONF_AUD_PACKET_SAMPFLT_MASK, 0x0);
}

u8 dw_fc_audio_get_mute(dw_hdmi_dev_t *dev)
{
	return dev_read_mask(dev, FC_AUDSCONF, FC_AUDSCONF_AUD_PACKET_SAMPFLT_MASK);
}

u8 dw_fc_audio_get_packet_layout(dw_hdmi_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev, FC_AUDSCONF, FC_AUDSCONF_AUD_PACKET_LAYOUT_MASK);
}

u8 dw_fc_audio_get_channel_count(dw_hdmi_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev, FC_AUDICONF0, FC_AUDICONF0_CC_MASK);
}

void dw_fc_audio_packet_config(dw_hdmi_dev_t *dev, dw_audio_param_t *audio)
{
	u8 channel_count = _audio_get_channel_count(dev, audio);
	u8 fs_value = 0;

	LOG_TRACE();

	dev_write_mask(dev, FC_AUDICONF0, FC_AUDICONF0_CC_MASK, channel_count);

	dev_write(dev, FC_AUDICONF2, audio->mChannelAllocation);

	dev_write_mask(dev, FC_AUDICONF3, FC_AUDICONF3_LSV_MASK,
			audio->mLevelShiftValue);

	dev_write_mask(dev, FC_AUDICONF3, FC_AUDICONF3_DM_INH_MASK,
			(audio->mDownMixInhibitFlag ? 1 : 0));

	AUDIO_INF("channel count = %d\n", channel_count);
	AUDIO_INF("channel allocation = %d\n",
		audio->mChannelAllocation);
	AUDIO_INF("level shift = %d\n",
		audio->mLevelShiftValue);

	if ((audio->mCodingType == DW_AUD_CODING_ONE_BIT_AUDIO) ||
			(audio->mCodingType == DW_AUD_CODING_DST)) {
		u32 sampling_freq = audio->mSamplingFrequency;

		/* Audio InfoFrame sample frequency when OBA or DST */
		if (sampling_freq == 32000)
			fs_value = 1;
		else if (sampling_freq == 44100)
			fs_value = 2;
		else if (sampling_freq == 48000)
			fs_value = 3;
		else if (sampling_freq == 88200)
			fs_value = 4;
		else if (sampling_freq == 96000)
			fs_value = 5;
		else if (sampling_freq == 176400)
			fs_value = 6;
		else if (sampling_freq == 192000)
			fs_value = 7;
		else
			fs_value = 0;
	} else {
		/* otherwise refer to stream header (0) */
		fs_value = 0;
	}
	dev_write_mask(dev, FC_AUDICONF1, FC_AUDICONF1_SF_MASK, fs_value);

	/* for HDMI refer to stream header  (0) */
	dev_write_mask(dev, FC_AUDICONF0, FC_AUDICONF0_CT_MASK, 0x0);

	/* for HDMI refer to stream header  (0) */
	dev_write_mask(dev, FC_AUDICONF1, FC_AUDICONF1_SS_MASK, 0x0);
}

void dw_fc_audio_config(dw_hdmi_dev_t *dev, dw_audio_param_t *audio)
{
	int i = 0;
	u8 channel_count = _audio_get_channel_count(dev, audio);
	/* u8 channel_count = audio->mChannelNum; */
	u8 data = 0;

	/* More than 2 channels => layout 1 else layout 0 */
	if ((channel_count + 1) > 2)
		dev_write_mask(dev, FC_AUDSCONF, FC_AUDSCONF_AUD_PACKET_LAYOUT_MASK, 0x1);
	else
		dev_write_mask(dev, FC_AUDSCONF, FC_AUDSCONF_AUD_PACKET_LAYOUT_MASK, 0x0);

	/* iec validity and user bits (IEC 60958-1) */
	for (i = 0; i < 4; i++) {
		/* _audio_check_channel_en considers left as 1 channel and
		 * right as another (+1), hence the x2 factor in the following */
		/* validity bit is 0 when reliable, which is !IsChannelEn */
		u8 channel_enable = _audio_check_channel_en(dev, audio, (2 * i));

		dev_write_mask(dev, FC_AUDSV, (1 << (4 + i)), (!channel_enable));

		channel_enable = _audio_check_channel_en(dev, audio, (2 * i) + 1);

		dev_write_mask(dev, FC_AUDSV, (1 << i), !channel_enable);

		dev_write_mask(dev, FC_AUDSU, (1 << (4 + i)), 0x1);

		dev_write_mask(dev, FC_AUDSU, (1 << i), 0x1);
	}

	if (audio->mCodingType != DW_AUD_CODING_PCM)
		return;
	/* IEC - not needed if non-linear PCM */
	dev_write_mask(dev, FC_AUDSCHNL0,
			FC_AUDSCHNL0_OIEC_CGMSA_MASK, audio->mIecCgmsA);

	dev_write_mask(dev, FC_AUDSCHNL0,
			FC_AUDSCHNL0_OIEC_COPYRIGHT_MASK, audio->mIecCopyright ? 0 : 1);

	dev_write(dev, FC_AUDSCHNL1, audio->mIecCategoryCode);

	dev_write_mask(dev, FC_AUDSCHNL2,
				FC_AUDSCHNL2_OIEC_PCMAUDIOMODE_MASK, audio->mIecPcmMode);

	dev_write_mask(dev, FC_AUDSCHNL2,
			FC_AUDSCHNL2_OIEC_SOURCENUMBER_MASK, audio->mIecSourceNumber);

	for (i = 0; i < 4; i++) {	/* 0, 1, 2, 3 */
		_dw_fc_audio_set_channel_left(dev, 2 * i + 1, i);	/* 1, 3, 5, 7 */
		_dw_fc_audio_set_channel_right(dev, 2 * (i + 1), i);	/* 2, 4, 6, 8 */
	}

	dev_write_mask(dev, FC_AUDSCHNL7,
			FC_AUDSCHNL7_OIEC_CLKACCURACY_MASK, audio->mIecClockAccuracy);

	data = _audio_iec_sampling_freq(dev, audio);
	dev_write_mask(dev, FC_AUDSCHNL7,
			FC_AUDSCHNL7_OIEC_SAMPFREQ_MASK, data);

	data = _audio_iec_original_sampling_freq(dev, audio);
	dev_write_mask(dev, FC_AUDSCHNL8,
			FC_AUDSCHNL8_OIEC_ORIGSAMPFREQ_MASK, data);

	data = _audio_iec_word_length(dev, audio);
	dev_write_mask(dev, FC_AUDSCHNL8,
			FC_AUDSCHNL8_OIEC_WORDLENGTH_MASK, data);
}

u32 dw_fc_audio_get_sample_freq(dw_hdmi_dev_t *dev)
{
	int i = 0;

	for (i = 0; iec_sampling_freq_values[i].iec.frequency != 0; i++) {
		if (_dw_fc_audio_get_iec_sample_freq(dev) ==
				iec_sampling_freq_values[i].value) {
			return iec_sampling_freq_values[i].iec.frequency;
		}
	}

	/* Not indicated */
	return 0;
}

u8 dw_fc_audio_get_word_length(dw_hdmi_dev_t *dev)
{
	int i = 0;

	for (i = 0; iec_word_length[i].iec.sample_size != 0; i++) {
		if (_dw_fc_audio_get_iec_word_length(dev) == iec_word_length[i].value)
			return iec_word_length[i].iec.sample_size;
	}

	return 0x0;
}

void _dw_fc_video_set_vblank_osc(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, FC_INVIDCONF,
			FC_INVIDCONF_R_V_BLANK_IN_OSC_MASK, bit);
}

/* for some video modes that have fractional Vblank */
void _dw_fc_video_set_interlaced(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, FC_INVIDCONF, FC_INVIDCONF_IN_I_P_MASK, bit);
}

void _dw_fc_video_set_hactive(dw_hdmi_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 12-bit width */
	dev_write(dev, (FC_INHACTIV0), (u8) (value));
	dev_write_mask(dev, FC_INHACTIV1, FC_INHACTIV1_H_IN_ACTIV_MASK |
			FC_INHACTIV1_H_IN_ACTIV_12_MASK, (u8)(value >> 8));
}

void _dw_fc_video_set_hblank(dw_hdmi_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 10-bit width */
	dev_write(dev, (FC_INHBLANK0), (u8) (value));
	dev_write_mask(dev, FC_INHBLANK1, FC_INHBLANK1_H_IN_BLANK_MASK |
			FC_INHBLANK1_H_IN_BLANK_12_MASK, (u8)(value >> 8));
}

void _dw_fc_video_set_vactive(dw_hdmi_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* Vactive value holds 11-bit width in two register */
	dev_write(dev, (FC_INVACTIV0), (u8) (value));
	dev_write_mask(dev, FC_INVACTIV1, FC_INVACTIV1_V_IN_ACTIV_MASK |
			FC_INVACTIV1_V_IN_ACTIV_12_11_MASK, (u8)(value >> 8));
}

void _dw_fc_video_set_vblank(dw_hdmi_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 8-bit width */
	dev_write(dev, (FC_INVBLANK), (u8) (value));
}

/* Setting Frame Composer Input Video HSync Front Porch */
void _dw_fc_video_set_hsync_edge_delay(dw_hdmi_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 11-bit width */
	dev_write(dev, (FC_HSYNCINDELAY0), (u8) (value));
	dev_write_mask(dev, FC_HSYNCINDELAY1, FC_HSYNCINDELAY1_H_IN_DELAY_MASK |
			FC_HSYNCINDELAY1_H_IN_DELAY_12_MASK, (u8)(value >> 8));
}

void _dw_fc_video_set_hsync_pluse_width(dw_hdmi_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 9-bit width */
	dev_write(dev, (FC_HSYNCINWIDTH0), (u8) (value));
	dev_write_mask(dev, FC_HSYNCINWIDTH1, FC_HSYNCINWIDTH1_H_IN_WIDTH_MASK,
			(u8)(value >> 8));
}

void _dw_fc_video_set_preamble_filter(dw_hdmi_dev_t *dev,
		u8 value, u8 channel)
{
	LOG_TRACE1(value);
	if (channel == 0)
		dev_write(dev, (FC_CH0PREAM), value);
	else if (channel == 1)
		dev_write_mask(dev, FC_CH1PREAM,
				FC_CH1PREAM_CH1_PREAMBLE_FILTER_MASK, value);
	else if (channel == 2)
		dev_write_mask(dev, FC_CH2PREAM,
				FC_CH2PREAM_CH2_PREAMBLE_FILTER_MASK, value);
	else
		hdmi_err("%s: invalid channel number: %d\n", __func__, channel);
}

void _dw_fc_video_force(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);

	/* avoid glitches */
	if (bit != 0) {
		dev_write(dev, FC_DBGTMDS2, bit ? 0x00 : 0x00);	/* R */
		dev_write(dev, FC_DBGTMDS1, bit ? 0x00 : 0x00);	/* G */
		dev_write(dev, FC_DBGTMDS0, bit ? 0xFF : 0x00);	/* B */
		dev_write_mask(dev, FC_DBGFORCE,
				FC_DBGFORCE_FORCEVIDEO_MASK, 1);
	} else {
		dev_write_mask(dev, FC_DBGFORCE,
				FC_DBGFORCE_FORCEVIDEO_MASK, 0);
		dev_write(dev, FC_DBGTMDS2, bit ? 0x00 : 0x00);	/* R */
		dev_write(dev, FC_DBGTMDS1, bit ? 0x00 : 0x00);	/* G */
		dev_write(dev, FC_DBGTMDS0, bit ? 0xFF : 0x00);	/* B */
	}
}

void dw_fc_video_set_hdcp_keepout(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, FC_INVIDCONF, FC_INVIDCONF_HDCP_KEEPOUT_MASK, bit);
}

void dw_fc_video_set_tmds_mode(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	/* 1: HDMI; 0: DVI */
	dev_write_mask(dev, FC_INVIDCONF, FC_INVIDCONF_DVI_MODEZ_MASK, bit);
}

u8 dw_fc_video_get_tmds_mode(dw_hdmi_dev_t *dev)
{
	LOG_TRACE();
	/* 1: HDMI; 0: DVI */
	return dev_read_mask(dev, FC_INVIDCONF, FC_INVIDCONF_DVI_MODEZ_MASK);
}

void dw_fc_video_set_scramble(dw_hdmi_dev_t *dev, u8 state)
{
	dev_write_mask(dev, FC_SCRAMBLER_CTRL,
			FC_SCRAMBLER_CTRL_SCRAMBLER_ON_MASK, state);
}

u8 dw_fc_video_get_scramble(dw_hdmi_dev_t *dev)
{
	return dev_read_mask(dev, FC_SCRAMBLER_CTRL,
			FC_SCRAMBLER_CTRL_SCRAMBLER_ON_MASK);
}

int dw_fc_video_config(dw_hdmi_dev_t *dev, dw_video_param_t *video)
{
	const dw_dtd_t *dtd = &video->mDtd;
	u16 i = 0;

	LOG_TRACE();

	if ((dev == NULL) || (video == NULL)) {
		hdmi_err("%s: Invalid arguments: dev=%lx; video=%lx\n",
				__func__, (uintptr_t)dev, (uintptr_t)video);
		return false;
	}

	dtd = &video->mDtd;
	dev_write_mask(dev, FC_INVIDCONF,
			FC_INVIDCONF_VSYNC_IN_POLARITY_MASK, dtd->mVSyncPolarity);
	dev_write_mask(dev, FC_INVIDCONF,
			FC_INVIDCONF_HSYNC_IN_POLARITY_MASK, dtd->mHSyncPolarity);
	dev_write_mask(dev, FC_INVIDCONF,
			FC_INVIDCONF_DE_IN_POLARITY_MASK, 0x1);

	dw_fc_video_set_tmds_mode(dev, video->mHdmi);

	if (video->mHdmiVideoFormat == 2) {
		if (video->m3dStructure == 0) {
			/* 3d data frame packing
			is transmitted as a progressive format */
			_dw_fc_video_set_vblank_osc(dev, 0);
			_dw_fc_video_set_interlaced(dev, 0);

			if (dtd->mInterlaced) {
				_dw_fc_video_set_vactive(dev,
				(dtd->mVActive << 2) + 3 * dtd->mVBlanking + 2);
			} else {
				_dw_fc_video_set_vactive(dev,
				(dtd->mVActive << 1) + dtd->mVBlanking);
			}
		} else {
			_dw_fc_video_set_vblank_osc(dev, dtd->mInterlaced);
			_dw_fc_video_set_interlaced(dev, dtd->mInterlaced);
			_dw_fc_video_set_vactive(dev, dtd->mVActive);
		}
	} else {
		_dw_fc_video_set_vblank_osc(dev, dtd->mInterlaced);
		_dw_fc_video_set_interlaced(dev, dtd->mInterlaced);
		_dw_fc_video_set_vactive(dev, dtd->mVActive);
	}

	if (video->mEncodingOut == DW_COLOR_FORMAT_YCC420) {
		VIDEO_INF("Encoding configured to YCC 420\n");
		_dw_fc_video_set_hactive(dev, dtd->mHActive/2);
		_dw_fc_video_set_hblank(dev, dtd->mHBlanking/2);
		_dw_fc_video_set_hsync_pluse_width(dev, dtd->mHSyncPulseWidth/2);
		_dw_fc_video_set_hsync_edge_delay(dev, dtd->mHSyncOffset/2);
	} else {
		_dw_fc_video_set_hactive(dev, dtd->mHActive);
		_dw_fc_video_set_hblank(dev, dtd->mHBlanking);
		_dw_fc_video_set_hsync_pluse_width(dev, dtd->mHSyncPulseWidth);
		_dw_fc_video_set_hsync_edge_delay(dev, dtd->mHSyncOffset);
	}

	_dw_fc_video_set_vblank(dev, dtd->mVBlanking);
	dev_write(dev, FC_VSYNCINDELAY, (u8)dtd->mVSyncOffset);

	dev_write_mask(dev, FC_VSYNCINWIDTH,
			FC_VSYNCINWIDTH_V_IN_WIDTH_MASK, (u8)dtd->mVSyncPulseWidth);

	dev_write(dev, FC_CTRLDUR, 12);
	dev_write(dev, FC_EXCTRLDUR, 32);

	/* spacing < 256^2 * config / tmdsClock, spacing <= 50ms
	 * worst case: tmdsClock == 25MHz => config <= 19
	 */
	dev_write(dev, FC_EXCTRLSPAC, 0x1);

	for (i = 0; i < 3; i++)
		_dw_fc_video_set_preamble_filter(dev, (i + 1) * 11, i);

	dev_write_mask(dev, FC_PRCONF,
			FC_PRCONF_INCOMING_PR_FACTOR_MASK, (dtd->mPixelRepetitionInput + 1));

	/* due to HDCP and Scrambler */
	if (dev->snps_hdmi_ctrl.hdcp_on ||
		(dev->snps_hdmi_ctrl.tmds_clk > 340000))
		/* Start koopout window generation */
		dw_fc_video_set_hdcp_keepout(dev, true);
	else
		dw_fc_video_set_hdcp_keepout(dev, false);

	return true;

}

void _dw_gamut_set_content(dw_hdmi_dev_t *dev, const u8 *content, u8 length)
{
	u8 i = 0;

	LOG_TRACE1(content[0]);
	if (length > (FC_GMD_PB_SIZE)) {
		hdmi_wrn("gamut content truncated! %d -> %d\n", length, FC_GMD_PB_SIZE);
		length = (FC_GMD_PB_SIZE);
	}

	for (i = 0; i < length; i++)
		dev_write(dev, FC_GMD_PB0 + (i*4), content[i]);
}

void _dw_gamut_set_enable_tx(dw_hdmi_dev_t *dev, u8 enable)
{
	LOG_TRACE1(enable);
	if (enable)
		enable = 1; /* ensure value is 1 */
	dev_write_mask(dev, FC_GMD_EN, FC_GMD_EN_GMDENABLETX_MASK, enable);
}

void _dw_gamut_config(dw_hdmi_dev_t *dev)
{
	/* P0 */
	dev_write_mask(dev, FC_GMD_HB, FC_GMD_HB_GMDGBD_PROFILE_MASK, 0x0);

	/* P0 */
	dev_write_mask(dev, FC_GMD_CONF, FC_GMD_CONF_GMDPACKETSINFRAME_MASK, 0x1);

	dev_write_mask(dev, FC_GMD_CONF, FC_GMD_CONF_GMDPACKETLINESPACING_MASK, 0x1);
}

void _dw_gamut_packet_config(dw_hdmi_dev_t *dev, const u8 *gbdContent, u8 length)
{
	u8 temp = 0x0;
	_dw_gamut_set_enable_tx(dev, 1);
	/* sequential */
	temp = (u8)(dev_read(dev, FC_GMD_STAT) & 0xF);
	dev_write_mask(dev, FC_GMD_HB,
			FC_GMD_HB_GMDAFFECTED_GAMUT_SEQ_NUM_MASK, (temp + 1) % 16);

	_dw_gamut_set_content(dev, gbdContent, length);

	/* set next_field to 1 */
	dev_write_mask(dev, FC_GMD_UP, FC_GMD_UP_GMDUPDATEPACKET_MASK, 0x1);
}

void _dw_avi_set_color_metry(dw_hdmi_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, FC_AVICONF1, FC_AVICONF1_COLORIMETRY_MASK, value);
}

void _dw_avi_set_active_aspect_ratio_valid(dw_hdmi_dev_t *dev, u8 valid)
{
	LOG_TRACE1(valid);
	dev_write_mask(dev, FC_AVICONF0,
			FC_AVICONF0_ACTIVE_FORMAT_PRESENT_MASK, valid);
}

void _dw_avi_set_active_format_aspect_ratio(dw_hdmi_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, FC_AVICONF1,
			FC_AVICONF1_ACTIVE_ASPECT_RATIO_MASK, value);
}

void _dw_avi_set_extend_color_metry(dw_hdmi_dev_t *dev, u8 extColor)
{
	LOG_TRACE1(extColor);
	dev_write_mask(dev, FC_AVICONF2,
			FC_AVICONF2_EXTENDED_COLORIMETRY_MASK, extColor);
}

void _dw_avi_config(dw_hdmi_dev_t *dev, dw_video_param_t *videoParams)
{
	u8 temp = 0;
	u16 endTop = 0;
	u16 startBottom = 0;
	u16 endLeft = 0;
	u16 startRight = 0;
	dw_dtd_t *dtd = &videoParams->mDtd;

	LOG_TRACE();

	if (videoParams->mEncodingOut == DW_COLOR_FORMAT_RGB)
		temp = 0;
	else if (videoParams->mEncodingOut == DW_COLOR_FORMAT_YCC422)
		temp = 1;
	else if (videoParams->mEncodingOut == DW_COLOR_FORMAT_YCC444)
		temp = 2;
	else if (videoParams->mEncodingOut == DW_COLOR_FORMAT_YCC420)
		temp = 3;
	else
		hdmi_err("%s: hdmi color format (%d) not supported yet!\n",
				__func__, videoParams->mEncodingOut);
	dev_write_mask(dev, FC_AVICONF0,
			FC_AVICONF0_RGC_YCC_INDICATION_MASK, temp);

	_dw_avi_set_active_format_aspect_ratio(dev, 0x8);

	/* VIDEO_INF("infoframe status before %x\n",
			fc_GetInfoFrameSatus(dev)); */

	dw_avi_set_scan_info(dev, videoParams->mScanInfo);

	temp = 0;
	if ((dtd->mHImageSize != 0 || dtd->mVImageSize != 0)
			&& (dtd->mHImageSize > dtd->mVImageSize)) {
		/* 16:9 or 4:3 */
		u8 pic = (dtd->mHImageSize * 10) % dtd->mVImageSize;
		temp = (pic > 5) ? 2 : 1;
	}
	dev_write_mask(dev, FC_AVICONF1,
			FC_AVICONF1_PICTURE_ASPECT_RATIO_MASK, temp);

	dev_write_mask(dev, FC_AVICONF2,
			FC_AVICONF2_IT_CONTENT_MASK, (videoParams->mItContent ? 1 : 0));

	dw_avi_set_quantization_range(dev, videoParams->mRgbQuantizationRange);

	dev_write_mask(dev, FC_AVICONF2,
			FC_AVICONF2_NON_UNIFORM_PICTURE_SCALING_MASK, videoParams->mNonUniformScaling);

	dev_write(dev, FC_AVIVID, videoParams->mCea_code);

	if (videoParams->mColorimetry == DW_METRY_EXTENDED) {
		/* ext colorimetry valid */
		if (videoParams->mExtColorimetry != (u8) (-1)) {
			_dw_avi_set_extend_color_metry(dev, videoParams->mExtColorimetry);
			_dw_avi_set_color_metry(dev, videoParams->mColorimetry);/* EXT-3 */
		} else {
			_dw_avi_set_extend_color_metry(dev, 0);
			_dw_avi_set_color_metry(dev, 0);	/* No Data */
		}
	} else {
		_dw_avi_set_extend_color_metry(dev, 0);
		/* NODATA-0/ 601-1/ 709-2/ EXT-3 */
		_dw_avi_set_color_metry(dev, videoParams->mColorimetry);
	}
	if (videoParams->mActiveFormatAspectRatio != 0) {
		_dw_avi_set_active_format_aspect_ratio(dev, videoParams->mActiveFormatAspectRatio);
		_dw_avi_set_active_aspect_ratio_valid(dev, 1);
	} else {
		_dw_avi_set_active_format_aspect_ratio(dev, 0);
		_dw_avi_set_active_aspect_ratio_valid(dev, 0);
	}

	temp = 0x0;
	if (videoParams->mEndTopBar != (u16) (-1) ||
			videoParams->mStartBottomBar != (u16) (-1)) {

		if (videoParams->mEndTopBar != (u16) (-1))
			endTop = videoParams->mEndTopBar;
		if (videoParams->mStartBottomBar != (u16) (-1))
			startBottom = videoParams->mStartBottomBar;

		dev_write(dev, FC_AVIETB0, (u8) (endTop));
		dev_write(dev, FC_AVIETB1, (u8) (endTop >> 8));

		dev_write(dev, FC_AVISBB0, (u8) (startBottom));
		dev_write(dev, FC_AVISBB1, (u8) (startBottom >> 8));

		temp = 0x1;
	}
	dev_write_mask(dev, FC_AVICONF0,
		FC_AVICONF0_BAR_INFORMATION_MASK & 0x8, temp);

	temp = 0x0;
	if (videoParams->mEndLeftBar != (u16) (-1) ||
			videoParams->mStartRightBar != (u16) (-1)) {
		if (videoParams->mEndLeftBar != (u16) (-1))
			endLeft = videoParams->mEndLeftBar;

		if (videoParams->mStartRightBar != (u16) (-1))
			startRight = videoParams->mStartRightBar;

		dev_write(dev, FC_AVIELB0, (u8) (endLeft));
		dev_write(dev, FC_AVIELB1, (u8) (endLeft >> 8));
		dev_write(dev, FC_AVISRB0, (u8) (startRight));
		dev_write(dev, FC_AVISRB1, (u8) (startRight >> 8));
		temp = 0x1;
	}
	dev_write_mask(dev, FC_AVICONF0,
		FC_AVICONF0_BAR_INFORMATION_MASK & 0x4, temp);

	temp = (dtd->mPixelRepetitionInput + 1) *
				(videoParams->mPixelRepetitionFactor + 1) - 1;
	dev_write_mask(dev, FC_PRCONF, FC_PRCONF_OUTPUT_PR_FACTOR_MASK, temp);

}

u8 dw_avi_get_colori_metry(dw_hdmi_dev_t *dev)
{
	u8 colorimetry = 0;
	LOG_TRACE();
	colorimetry = (u8)dev_read_mask(dev, FC_AVICONF1, FC_AVICONF1_COLORIMETRY_MASK);
	if (colorimetry == 3)
		return (colorimetry + dev_read_mask(dev, FC_AVICONF2,
					FC_AVICONF2_EXTENDED_COLORIMETRY_MASK));
	return colorimetry;
}

void dw_avi_set_colori_metry(dw_hdmi_dev_t *dev, u8 metry, u8 ex_metry)
{
	if (ex_metry || (metry == DW_METRY_EXTENDED)) {
		_dw_avi_set_extend_color_metry(dev, ex_metry);
		_dw_avi_set_color_metry(dev, DW_METRY_EXTENDED);
	} else {
		_dw_avi_set_extend_color_metry(dev, 0);
		_dw_avi_set_color_metry(dev, metry);
	}
	_dw_gamut_set_enable_tx(dev, 0);
}

void dw_avi_set_scan_info(dw_hdmi_dev_t *dev, u8 left)
{
	LOG_TRACE1(left);
	dev_write_mask(dev, FC_AVICONF0,
			FC_AVICONF0_SCAN_INFORMATION_MASK, left);
}

u8 dw_avi_get_rgb_ycc(dw_hdmi_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev, FC_AVICONF0,
			FC_AVICONF0_RGC_YCC_INDICATION_MASK);
}

u8 dw_avi_get_video_code(dw_hdmi_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev, FC_AVIVID, FC_AVIVID_FC_AVIVID_MASK);
}

void dw_avi_set_video_code(dw_hdmi_dev_t *dev, u8 data)
{
	LOG_TRACE();
	return dev_write_mask(dev, FC_AVIVID, FC_AVIVID_FC_AVIVID_MASK, data);
}

void dw_avi_set_aspect_ratio(dw_hdmi_dev_t *dev, u8 left)
{
	if (left) {
		_dw_avi_set_active_format_aspect_ratio(dev, left);
		_dw_avi_set_active_aspect_ratio_valid(dev, 1);
	} else {
		_dw_avi_set_active_format_aspect_ratio(dev, 0);
		_dw_avi_set_active_aspect_ratio_valid(dev, 0);
	}
}

void dw_avi_set_quantization_range(dw_hdmi_dev_t *dev, u8 range)
{
	LOG_TRACE1(range);
	dev_write_mask(dev, FC_AVICONF2,
			FC_AVICONF2_QUANTIZATION_RANGE_MASK, range);
}

void _dw_spd_set_vendor_name(dw_hdmi_dev_t *dev,
		const u8 *data, u8 length)
{
	u8 i = 0;

	LOG_TRACE();
	for (i = 0; i < length; i++)
		dev_write(dev, FC_SPDVENDORNAME0 + (i*4), data[i]);
}

void _dw_spd_set_product_name(dw_hdmi_dev_t *dev,
		const u8 *data, u8 length)
{
	u8 i = 0;
	LOG_TRACE();
	for (i = 0; i < length; i++)
		dev_write(dev, FC_SPDPRODUCTNAME0 + (i*4), data[i]);
}

/*
 * Configure the Vendor Payload to be carried by the InfoFrame
 * @param info array
 * @param length of the array
 * @return 0 when successful and 1 on error
 */
u8 _dw_vsi_set_vendor_payload(dw_hdmi_dev_t *dev,
		const u8 *data, unsigned short length)
{
	const unsigned short size = 24;
	unsigned i = 0;

	LOG_TRACE();
	if (!data) {
		hdmi_err("%s: param data is null!\n", __func__);
		return -1;
	}
	if (length > size) {
		hdmi_wrn("vendor payload truncated %d -> %d\n", length, size);
		length = size;
	}
	for (i = 0; i < length; i++)
		dev_write(dev, (FC_VSDPAYLOAD0 + (i*4)), data[i]);

	return 0;
}

void _dw_vsi_enable(dw_hdmi_dev_t *dev, u8 enable)
{
	dev_write_mask(dev, FC_PACKET_TX_EN,
			FC_PACKET_TX_EN_AUT_TX_EN_MASK, enable);
}

void _dw_packets_auto_send(dw_hdmi_dev_t *dev, u8 enable, u8 mask)
{
	LOG_TRACE2(enable, mask);
	dev_write_mask(dev, FC_DATAUTO0, (1 << mask), (enable ? 1 : 0));
}

void _dw_packets_manual_send(dw_hdmi_dev_t *dev, u8 mask)
{
	LOG_TRACE1(mask);
	dev_write_mask(dev, FC_DATMAN, (1 << mask), 1);
}

void _dw_packets_disable_all(dw_hdmi_dev_t *dev)
{
	uint32_t value = (uint32_t)(~(BIT(FC_ACP_TX) | BIT(FC_ISRC1_TX) |
			BIT(FC_ISRC2_TX) | BIT(FC_SPD_TX) | BIT(FC_VSD_TX)));

	LOG_TRACE();
	dev_write(dev, FC_DATAUTO0, value & dev_read(dev, FC_DATAUTO0));
}

void _dw_packets_metadata_config(dw_hdmi_dev_t *dev)
{
	dev_write_mask(dev, FC_DATAUTO1,
			FC_DATAUTO1_AUTO_FRAME_INTERPOLATION_MASK, 0x1);
	dev_write_mask(dev, FC_DATAUTO2,
			FC_DATAUTO2_AUTO_FRAME_PACKETS_MASK, 0x1);
	dev_write_mask(dev, FC_DATAUTO2,
			FC_DATAUTO2_AUTO_LINE_SPACING_MASK, 0x1);
}

/**
 * Configure Colorimetry packets
 * @param dev Device structure
 * @param video Video information structure
 */
void _dw_gamut_colorimetry_config(dw_hdmi_dev_t *dev, dw_video_param_t *video)
{
	u8 gamut_metadata[28] = {0};
	int gdb_color_space = 0;

	_dw_gamut_set_enable_tx(dev, 0);

	if (video->mColorimetry == DW_METRY_EXTENDED) {
		if (video->mExtColorimetry == DW_METRY_EXT_XV_YCC601) {
			gdb_color_space = 1;
		} else if (video->mExtColorimetry == DW_METRY_EXT_XV_YCC709) {
			gdb_color_space = 2;
			VIDEO_INF("xv ycc709\n");
		} else if (video->mExtColorimetry == DW_METRY_EXT_S_YCC601) {
			gdb_color_space = 3;
		} else if (video->mExtColorimetry == DW_METRY_EXT_ADOBE_YCC601) {
			gdb_color_space = 3;
		} else if (video->mExtColorimetry == DW_METRY_EXT_ADOBE_RGB) {
			gdb_color_space = 3;
		}

		if (video->mColorimetryDataBlock == true) {
			gamut_metadata[0] = (1 << 7) | gdb_color_space;
			_dw_gamut_packet_config(dev, gamut_metadata,
					(sizeof(gamut_metadata) / sizeof(u8)));
		}
	}
}

/**
 * Configure Vendor Specific InfoFrames.
 * @param dev Device structure
 * @param oui Vendor Organisational Unique Identifier 24 bit IEEE
 * Registration Identifier
 * @param payload Vendor Specific Info Payload
 * @param length of the payload array
 * @param autoSend Start send Vendor Specific InfoFrame automatically
 */
int _dw_packet_vsi_config(dw_hdmi_dev_t *dev, u32 oui,
				const u8 *payload, u8 length, u8 autoSend)
{
	LOG_TRACE();
	_dw_packets_auto_send(dev,  0, FC_VSD_TX);/* prevent sending half the info. */

	dev_write(dev, (FC_VSDIEEEID0), oui);
	dev_write(dev, (FC_VSDIEEEID1), oui >> 8);
	dev_write(dev, (FC_VSDIEEEID2), oui >> 16);

	if (_dw_vsi_set_vendor_payload(dev, payload, length))
		return false;	/* DEFINE ERROR */

	if (autoSend)
		_dw_packets_auto_send(dev, autoSend, FC_VSD_TX);
	else
		_dw_packets_manual_send(dev, FC_VSD_TX);

	return true;
}

int _dw_packet_spd_config(dw_hdmi_dev_t *dev, fc_spd_info_t *spd_data)
{
	const unsigned short pSize = 8;
	const unsigned short vSize = 16;

	LOG_TRACE();

	if (spd_data == NULL) {
		hdmi_err("%s: param spd_data is null!\n", __func__);
		return false;
	}

	_dw_packets_auto_send(dev, 0, FC_SPD_TX);/* prevent sending half the info. */

	if (spd_data->vName == 0) {
		hdmi_err("%s: invalid parameter vName\n", __func__);
		return false;
	}
	if (spd_data->vLength > vSize) {
		hdmi_wrn("vendor name truncated %d -> %d\n", spd_data->vLength, vSize);
		spd_data->vLength = vSize;
	}
	if (spd_data->pName == 0) {
		hdmi_err("%s: invalid parameter pName\n", __func__);
		return false;
	}
	if (spd_data->pLength > pSize) {
		hdmi_wrn("product name truncated %d -> %d\n", spd_data->pLength, pSize);
		spd_data->pLength = pSize;
	}

	_dw_spd_set_vendor_name(dev, spd_data->vName, spd_data->vLength);
	_dw_spd_set_product_name(dev, spd_data->pName, spd_data->pLength);
	dev_write(dev, FC_SPDDEVICEINF, spd_data->code);

	if (spd_data->autoSend)
		_dw_packets_auto_send(dev, spd_data->autoSend, FC_SPD_TX);
	else
		_dw_packets_manual_send(dev, FC_SPD_TX);

	return true;
}

void dw_fc_force_output(dw_hdmi_dev_t *dev, int enable)
{
	LOG_TRACE1(enable);
	dw_fc_audio_force(dev, 0);
	_dw_fc_video_force(dev, (u8)enable);
}

void dw_gcp_set_avmute(dw_hdmi_dev_t *dev, u8 enable)
{
	LOG_TRACE1(enable);
	dev_write_mask(dev, FC_GCP, FC_GCP_SET_AVMUTE_MASK, (enable ? 1 : 0));
	dev_write_mask(dev, FC_GCP, FC_GCP_CLEAR_AVMUTE_MASK, (enable ? 0 : 1));
}

u8 dw_gcp_get_avmute(dw_hdmi_dev_t *dev)
{
	return dev_read_mask(dev, FC_GCP, FC_GCP_SET_AVMUTE_MASK);
}

void dw_drm_packet_clear(dw_fc_drm_pb_t *pb)
{
	if (pb) {
		pb->r_x = 0;
		pb->r_y = 0;
		pb->g_x = 0;
		pb->g_y = 0;
		pb->b_x = 0;
		pb->b_y = 0;
		pb->w_x = 0;
		pb->w_y = 0;
		pb->luma_max = 0;
		pb->luma_min = 0;
		pb->mcll = 0;
		pb->mfll = 0;
	}
}

void dw_drm_packet_up(dw_hdmi_dev_t *dev, dw_fc_drm_pb_t *pb)
{
	int timeout = 10;
	u32 status = 0;

	LOG_TRACE();
	/* Configure Dynamic Range and Mastering infoFrame */
	if (pb != 0) {
		dev_write(dev, FC_DRM_PB0, pb->eotf & 0x07);
		dev_write(dev, FC_DRM_PB1, pb->metadata & 0x07);
		dev_write(dev, FC_DRM_PB2, (pb->r_x >> 0) & 0xff);
		dev_write(dev, FC_DRM_PB3, (pb->r_x >> 8) & 0xff);
		dev_write(dev, FC_DRM_PB4, (pb->r_y >> 0) & 0xff);
		dev_write(dev, FC_DRM_PB5, (pb->r_y >> 8) & 0xff);
		dev_write(dev, FC_DRM_PB6, (pb->g_x >> 0) & 0xff);
		dev_write(dev, FC_DRM_PB7, (pb->g_x >> 8) & 0xff);
		dev_write(dev, FC_DRM_PB8, (pb->g_y >> 0) & 0xff);
		dev_write(dev, FC_DRM_PB9, (pb->g_y >> 8) & 0xff);
		dev_write(dev, FC_DRM_PB10, (pb->b_x >> 0) & 0xff);
		dev_write(dev, FC_DRM_PB11, (pb->b_x >> 8) & 0xff);
		dev_write(dev, FC_DRM_PB12, (pb->b_y >> 0) & 0xff);
		dev_write(dev, FC_DRM_PB13, (pb->b_y >> 8) & 0xff);
		dev_write(dev, FC_DRM_PB14, (pb->w_x >> 0) & 0xff);
		dev_write(dev, FC_DRM_PB15, (pb->w_x >> 8) & 0xff);
		dev_write(dev, FC_DRM_PB16, (pb->w_y >> 0) & 0xff);
		dev_write(dev, FC_DRM_PB17, (pb->w_y >> 8) & 0xff);
		dev_write(dev, FC_DRM_PB18, (pb->luma_max >> 0) & 0xff);
		dev_write(dev, FC_DRM_PB19, (pb->luma_max >> 8) & 0xff);
		dev_write(dev, FC_DRM_PB20, (pb->luma_min >> 0) & 0xff);
		dev_write(dev, FC_DRM_PB21, (pb->luma_min >> 8) & 0xff);
		dev_write(dev, FC_DRM_PB22, (pb->mcll >> 0) & 0xff);
		dev_write(dev, FC_DRM_PB23, (pb->mcll >> 8) & 0xff);
		dev_write(dev, FC_DRM_PB24, (pb->mfll >> 0) & 0xff);
		dev_write(dev, FC_DRM_PB25, (pb->mfll >> 8) & 0xff);
	 }
	dev_write_mask(dev, FC_DRM_HB0, FC_DRM_UP_FC_DRM_HB_MASK, 0x01);
	dev_write_mask(dev, FC_DRM_HB1, FC_DRM_UP_FC_DRM_HB_MASK, 26);
	dev_write_mask(dev, FC_PACKET_TX_EN, FC_PACKET_TX_EN_DRM_TX_EN_MASK, 0x1);
	do {
		udelay(10);
		status = dev_read_mask(dev, FC_DRM_UP, FC_DRM_UP_DRMPACKETUPDATE_MASK);
	} while (status && (timeout--));
	dev_write_mask(dev, FC_DRM_UP,  FC_DRM_UP_DRMPACKETUPDATE_MASK, 0x1);
}

void dw_drm_packet_disabled(dw_hdmi_dev_t *dev)
{
	LOG_TRACE();
	dev_write_mask(dev, FC_PACKET_TX_EN, FC_PACKET_TX_EN_DRM_TX_EN_MASK, 0x0);
}

/*
* get vsif data
* data[0]: hdmi_format filed in vsif
* data[1]: hdmi_vic or 3d strcture filed in vsif
*/
void dw_vsif_get_hdmi_vic(dw_hdmi_dev_t *dev, u8 *data)
{
	data[0] = dev_read(dev, FC_VSDPAYLOAD0);
	data[1] = dev_read(dev, FC_VSDPAYLOAD0 + 0x4);
}

/*
* set vsif data
* data[0]: hdmi_format filed in vsif
* data[1]: hdmi_vic or 3d strcture filed in vsif
*/
void dw_vsif_set_hdmi_vic(dw_hdmi_dev_t *dev, u8 *data)
{
	 dev_write(dev, FC_VSDPAYLOAD0, data[0]);
	 dev_write(dev, FC_VSDPAYLOAD0 + 0x4, data[1]);
}

/* packets configure is the same as infoframe configure */
int dw_fc_packet_config(dw_hdmi_dev_t *dev,
		dw_video_param_t *video, dw_product_param_t *prod)
{
	u32 oui = 0;
	u8 struct_3d = 0;
	u8 data[4];
	u8 *vendor_payload = prod->mVendorPayload;
	u8 payload_length = prod->mVendorPayloadLength;

	LOG_TRACE();

	if (dev->snps_hdmi_ctrl.hdmi_on == 0) {
		hdmi_wrn("DVI mode selected: packets not configured\n");
		return true;
	}

	if (video->mHdmiVideoFormat == 2) {
		struct_3d = video->m3dStructure;
		VIDEO_INF("3D packets configure\n");

		/* frame packing || tab || sbs */
		if ((struct_3d == 0) || (struct_3d == 6) || (struct_3d == 8)) {
			data[0] = video->mHdmiVideoFormat << 5; /* PB4 */
			data[1] = struct_3d << 4; /* PB5 */
			data[2] = video->m3dExtData << 4;
			data[3] = 0;
			/* HDMI Licensing, LLC */
			_dw_packet_vsi_config(dev, 0x000C03, data, sizeof(data), 1);
			_dw_vsi_enable(dev, 1);
		} else {
			hdmi_err("%s: 3d structure not supported %d\n", __func__, struct_3d);
			return false;
		}
	} else if ((video->mHdmiVideoFormat == 0x1) || (video->mHdmiVideoFormat == 0x0)) {
		if (prod != 0) {
			fc_spd_info_t spd_data;

			spd_data.vName    = prod->mVendorName;
			spd_data.vLength  = prod->mVendorNameLength;
			spd_data.pName    = prod->mProductName;
			spd_data.pLength  = prod->mProductNameLength;
			spd_data.code     = prod->mSourceType;
			spd_data.autoSend = 1;

			oui = prod->mOUI;
			_dw_packet_spd_config(dev, &spd_data);
			_dw_packet_vsi_config(dev, oui, vendor_payload,
					payload_length, 1);
			_dw_vsi_enable(dev, 1);
		} else {
			VIDEO_INF("No product info provided: not configured\n");
		}
	} else {
		hdmi_err("%s: unknow video format [%d]\n", __func__, video->mHdmiVideoFormat);
		_dw_vsi_enable(dev, 0);
	}

	_dw_packets_metadata_config(dev);

	/* default phase 1 = true */
	dev_write_mask(dev, FC_GCP, FC_GCP_DEFAULT_PHASE_MASK,
			((video->mPixelPackingDefaultPhase == 1) ? 1 : 0));

	_dw_gamut_config(dev);

	_dw_avi_config(dev, video);

	/* * Colorimetry */
	_dw_gamut_colorimetry_config(dev, video);

	if (video->mHdr) {
		VIDEO_INF("Is HDR video format\n");
		dw_drm_packet_up(dev, video->pb);
	} else {
		dw_drm_packet_disabled(dev);
	}

	return true;
}

int dw_fc_iteration_process(dw_hdmi_dev_t *dev)
{
	dev_write(dev, FC_INVIDCONF, dev_read(dev, FC_INVIDCONF));
	return 0;
}
