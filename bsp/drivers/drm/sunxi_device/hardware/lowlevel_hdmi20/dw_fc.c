/* SPDX-License-Identifier: GPL-2.0-or-later */
/*******************************************************************************
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 ******************************************************************************/
#include <linux/delay.h>

#include "dw_dev.h"
#include "dw_avp.h"
#include "dw_fc.h"

#define FC_GMD_PB_SIZE			28

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
static u8 _audio_sw_get_channel_count(struct dw_audio_s *params)
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
static u8 _audio_sw_get_iec_original_sampling_freq(struct dw_audio_s *params)
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
static u8 _audio_sw_get_iec_sampling_freq(struct dw_audio_s *params)
{
	int i = 0;

	for (i = 0; iec_sampling_freq_values[i].iec.frequency != 0; i++) {
		if (params->mSamplingFrequency ==
			iec_sampling_freq_values[i].iec.frequency) {
			u8 value = iec_sampling_freq_values[i].value;
			return value;
		}
	}

	return 0x1;
}

/**
 * @param params pointer to the audio parameters structure
 */
static u8 _audio_sw_get_iec_word_length(struct dw_audio_s *params)
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
static u8 _audio_sw_check_channel_en(struct dw_audio_s *params, u8 channel)
{
	switch (channel) {
	case 0:
	case 1:
		hdmi_trace("channal %d is enable\n", channel);
		return 1;
	case 2:
		hdmi_trace("channal %d is enable\n", channel);
		return params->mChannelAllocation & BIT(0);
	case 3:
		hdmi_trace("channal %d is enable\n", channel);
		return (params->mChannelAllocation & BIT(1)) >> 1;
	case 4:
		if (((params->mChannelAllocation > 0x03) &&
			(params->mChannelAllocation < 0x14)) ||
			((params->mChannelAllocation > 0x17) &&
			(params->mChannelAllocation < 0x20))) {
			hdmi_trace("channal %d is enable\n", channel);
			return 1;
		} else {
			return 0;
		}
	case 5:
		if (((params->mChannelAllocation > 0x07) &&
			(params->mChannelAllocation < 0x14)) ||
			((params->mChannelAllocation > 0x1C) &&
			(params->mChannelAllocation < 0x20))) {
			hdmi_trace("channal %d is enable\n", channel);
			return 1;
		} else {
			return 0;
		}
	case 6:
		if ((params->mChannelAllocation > 0x0B) &&
			(params->mChannelAllocation < 0x20)) {
			hdmi_trace("channal %d is enable\n", channel);
			return 1;
		} else {
			return 0;
		}
	case 7:
		hdmi_trace("channal %d is enable\n", channel);
		return (params->mChannelAllocation & BIT(4)) >> 4;
	default:
		return 0;
	}
}

static void _dw_fc_audio_set_channel_right(u8 value, u8 channel)
{
	if (channel == 0)
		dw_write_mask(FC_AUDSCHNL3, FC_AUDSCHNL3_OIEC_CHANNELNUMCR0_MASK, value);
	else if (channel == 1)
		dw_write_mask(FC_AUDSCHNL3, FC_AUDSCHNL3_OIEC_CHANNELNUMCR1_MASK, value);
	else if (channel == 2)
		dw_write_mask(FC_AUDSCHNL4, FC_AUDSCHNL4_OIEC_CHANNELNUMCR2_MASK, value);
	else if (channel == 3)
		dw_write_mask(FC_AUDSCHNL4, FC_AUDSCHNL4_OIEC_CHANNELNUMCR3_MASK, value);
	else
		hdmi_err("set channel %d right is invalid!!!", channel);
}

static void _dw_fc_audio_set_channel_left(u8 value, u8 channel)
{
	if (channel == 0)
		dw_write_mask(FC_AUDSCHNL5, FC_AUDSCHNL5_OIEC_CHANNELNUMCL0_MASK, value);
	else if (channel == 1)
		dw_write_mask(FC_AUDSCHNL5, FC_AUDSCHNL5_OIEC_CHANNELNUMCL1_MASK, value);
	else if (channel == 2)
		dw_write_mask(FC_AUDSCHNL6, FC_AUDSCHNL6_OIEC_CHANNELNUMCL2_MASK, value);
	else if (channel == 3)
		dw_write_mask(FC_AUDSCHNL6, FC_AUDSCHNL6_OIEC_CHANNELNUMCL3_MASK, value);
	else
		hdmi_err("set channel %d left is invalid!!!", channel);
}

void dw_fc_audio_set_mute(u8 state)
{
	dw_write_mask(FC_AUDSCONF, FC_AUDSCONF_AUD_PACKET_SAMPFLT_MASK,
			state ? 0xF : 0x0);
}

u8 dw_fc_audio_get_mute(void)
{
	return dw_read_mask(FC_AUDSCONF, FC_AUDSCONF_AUD_PACKET_SAMPFLT_MASK);
}

void dw_fc_audio_packet_config(struct dw_audio_s *audio)
{
	u8 channel_count = _audio_sw_get_channel_count(audio);
	u8 fs_value = 0;

	dw_write_mask(FC_AUDICONF0, FC_AUDICONF0_CC_MASK, channel_count);

	dw_write(FC_AUDICONF2, audio->mChannelAllocation);

	dw_write_mask(FC_AUDICONF3, FC_AUDICONF3_LSV_MASK,
			audio->mLevelShiftValue);

	dw_write_mask(FC_AUDICONF3, FC_AUDICONF3_DM_INH_MASK,
			(audio->mDownMixInhibitFlag ? 1 : 0));

	hdmi_trace("[audio packet]\n");
	hdmi_trace(" - channel count = %d\n", channel_count);
	hdmi_trace(" - channel allocation = %d\n", audio->mChannelAllocation);
	hdmi_trace(" - level shift = %d\n", audio->mLevelShiftValue);

	if ((audio->mCodingType == DW_AUD_CODING_ONE_BIT_AUDIO) ||
			(audio->mCodingType == DW_AUD_CODING_DST)) {
		u32 freq = audio->mSamplingFrequency;

		if (freq == 32000)
			fs_value = 1;
		else if (freq == 44100)
			fs_value = 2;
		else if (freq == 48000)
			fs_value = 3;
		else if (freq == 88200)
			fs_value = 4;
		else if (freq == 96000)
			fs_value = 5;
		else if (freq == 176400)
			fs_value = 6;
		else if (freq == 192000)
			fs_value = 7;
		else
			fs_value = 0;
	} else {
		/* otherwise refer to stream header (0) */
		fs_value = 0;
	}
	hdmi_trace(" - freq number = %d\n", fs_value);
	dw_write_mask(FC_AUDICONF1, FC_AUDICONF1_SF_MASK, fs_value);

	dw_write_mask(FC_AUDICONF0, FC_AUDICONF0_CT_MASK, 0x0);

	dw_write_mask(FC_AUDICONF1, FC_AUDICONF1_SS_MASK, 0x0);

	hdmi_trace("dw audio packet config done!\n");
}

void dw_fc_audio_sample_config(struct dw_audio_s *audio)
{
	int i = 0;
	u8 channel_count = _audio_sw_get_channel_count(audio);
	u8 data = 0;

	/* More than 2 channels => layout 1 else layout 0 */
	if ((channel_count + 1) > 2)
		dw_write_mask(FC_AUDSCONF, FC_AUDSCONF_AUD_PACKET_LAYOUT_MASK, 0x1);
	else
		dw_write_mask(FC_AUDSCONF, FC_AUDSCONF_AUD_PACKET_LAYOUT_MASK, 0x0);

	/* iec validity and user bits (IEC 60958-1) */
	for (i = 0; i < 4; i++) {
		/* _audio_sw_check_channel_en considers left as 1 channel and
		 * right as another (+1), hence the x2 factor in the following */
		/* validity bit is 0 when reliable, which is !IsChannelEn */
		u8 channel_enable = _audio_sw_check_channel_en(audio, (2 * i));
		if (i < 4)
			dw_write_mask(FC_AUDSV, (1 << (4 + i)), (!channel_enable));
		else
			hdmi_err("invalid channel number\n");

		channel_enable = _audio_sw_check_channel_en(audio, (2 * i) + 1);
		if (i < 4)
			dw_write_mask(FC_AUDSV, (1 << i), !channel_enable);
		else
			hdmi_err("invalid channel number: %d", i);

		if (i < 4)
			dw_write_mask(FC_AUDSU, (1 << (4 + i)), 0x1);
		else
			hdmi_err("invalid channel number: %d\n", i);

		if (i < 4)
			dw_write_mask(FC_AUDSU, (1 << i), 0x1);
		else
			hdmi_err("invalid channel number: %d\n", i);
	}

	if (audio->mCodingType != DW_AUD_CODING_PCM) {
		hdmi_inf("dw audio coding type %d unsupport\n", audio->mCodingType);
		return ;
	}

	/* IEC - not needed if non-linear PCM */
	dw_write_mask(FC_AUDSCHNL0,
			FC_AUDSCHNL0_OIEC_CGMSA_MASK, audio->mIecCgmsA);

	dw_write_mask(FC_AUDSCHNL0,
			FC_AUDSCHNL0_OIEC_COPYRIGHT_MASK, audio->mIecCopyright ? 0 : 1);

	dw_write(FC_AUDSCHNL1, audio->mIecCategoryCode);

	dw_write_mask(FC_AUDSCHNL2,
				FC_AUDSCHNL2_OIEC_PCMAUDIOMODE_MASK, audio->mIecPcmMode);

	dw_write_mask(FC_AUDSCHNL2,
			FC_AUDSCHNL2_OIEC_SOURCENUMBER_MASK, audio->mIecSourceNumber);

	for (i = 0; i < 4; i++) {	/* 0, 1, 2, 3 */
		_dw_fc_audio_set_channel_left(2 * i + 1, i);	/* 1, 3, 5, 7 */
		_dw_fc_audio_set_channel_right(2 * (i + 1), i);	/* 2, 4, 6, 8 */
	}

	dw_write_mask(FC_AUDSCHNL7,
			FC_AUDSCHNL7_OIEC_CLKACCURACY_MASK, audio->mIecClockAccuracy);

	data = _audio_sw_get_iec_sampling_freq(audio);
	dw_write_mask(FC_AUDSCHNL7, FC_AUDSCHNL7_OIEC_SAMPFREQ_MASK, data);

	data = _audio_sw_get_iec_original_sampling_freq(audio);
	dw_write_mask(FC_AUDSCHNL8, FC_AUDSCHNL8_OIEC_ORIGSAMPFREQ_MASK, data);

	data = _audio_sw_get_iec_word_length(audio);
	dw_write_mask(FC_AUDSCHNL8, FC_AUDSCHNL8_OIEC_WORDLENGTH_MASK, data);

	hdmi_trace("dw audio sample config done!\n");
}

u32 dw_fc_audio_get_sample_freq(void)
{
	int i = 0;
	u32 hw_freq = dw_read_mask(FC_AUDSCHNL7, FC_AUDSCHNL7_OIEC_SAMPFREQ_MASK);

	for (i = 0; iec_sampling_freq_values[i].iec.frequency != 0; i++) {
		if (hw_freq == iec_sampling_freq_values[i].value) {
			return iec_sampling_freq_values[i].iec.frequency;
		}
	}

	/* Not indicated */
	return 0;
}

u8 dw_fc_audio_get_word_length(void)
{
	int i = 0;
	u32 hw_data = dw_read_mask(FC_AUDSCHNL8, FC_AUDSCHNL8_OIEC_WORDLENGTH_MASK);

	for (i = 0; iec_word_length[i].iec.sample_size != 0; i++) {
		if (hw_data == iec_word_length[i].value)
			return iec_word_length[i].iec.sample_size;
	}

	return 0x0;
}

u32 dw_fc_video_get_hactive(void)
{
	u32 value = 0;
	value = dw_read_mask(FC_INHACTIV1, FC_INHACTIV1_H_IN_ACTIV_MASK |
		FC_INHACTIV1_H_IN_ACTIV_12_MASK) << 8;
	value |= dw_read(FC_INHACTIV0);
	return value;
}

u32 dw_fc_video_get_vactive(void)
{
	u32 value = 0;
	value = dw_read_mask(FC_INVACTIV1, FC_INVACTIV1_V_IN_ACTIV_MASK |
		FC_INVACTIV1_V_IN_ACTIV_12_11_MASK) << 8;
	value |= dw_read(FC_INVACTIV0);
	return value;
}

void _dw_fc_video_set_preamble_filter(u8 value, u8 channel)
{
	if (channel == 0)
		dw_write((FC_CH0PREAM), value);
	else if (channel == 1)
		dw_write_mask(FC_CH1PREAM,
				FC_CH1PREAM_CH1_PREAMBLE_FILTER_MASK, value);
	else if (channel == 2)
		dw_write_mask(FC_CH2PREAM,
				FC_CH2PREAM_CH2_PREAMBLE_FILTER_MASK, value);
	else
		hdmi_err("invalid channel number: %d\n", channel);
}

void dw_fc_video_force_value(u8 bit, u32 value)
{
	if (bit) {
		/* enable force frame composer video output */
		dw_write(FC_DBGTMDS2, (u8)((value >> 16) & 0xFF)); /* R */
		dw_write(FC_DBGTMDS1, (u8)((value >> 8)  & 0xFF)); /* G */
		dw_write(FC_DBGTMDS0, (u8)((value >> 0)  & 0xFF)); /* B */
	}
	dw_write_mask(FC_DBGFORCE, FC_DBGFORCE_FORCEVIDEO_MASK, bit ? 0x1 : 0x0);
}

void dw_fc_video_set_hdcp_keepout(u8 bit)
{
	dw_write_mask(FC_INVIDCONF, FC_INVIDCONF_HDCP_KEEPOUT_MASK, bit);
}

u8 dw_fc_video_get_tmds_mode(void)
{
	/* 1: HDMI; 0: DVI */
	return dw_read_mask(FC_INVIDCONF, FC_INVIDCONF_DVI_MODEZ_MASK);
}

void dw_fc_video_set_scramble(u8 state)
{
	dw_write_mask(FC_SCRAMBLER_CTRL,
			FC_SCRAMBLER_CTRL_SCRAMBLER_ON_MASK, state);
}

u8 dw_fc_video_get_scramble(void)
{
	return dw_read_mask(FC_SCRAMBLER_CTRL,
			FC_SCRAMBLER_CTRL_SCRAMBLER_ON_MASK);
}

u8 dw_fc_video_get_hsync_polarity(void)
{
	return dw_read_mask(FC_INVIDCONF, FC_INVIDCONF_HSYNC_IN_POLARITY_MASK);
}

u8 dw_fc_video_get_vsync_polarity(void)
{
	return dw_read_mask(FC_INVIDCONF, FC_INVIDCONF_VSYNC_IN_POLARITY_MASK);
}

void dw_fc_set_tmds_mode(dw_tmds_mode_t mode)
{
	dw_write_mask(FC_INVIDCONF, FC_INVIDCONF_DVI_MODEZ_MASK,
			mode == 0 ? 0x0 : 0x1);
}

int dw_fc_video_config(struct dw_video_s *video)
{
	const dw_dtd_t *dtd = &video->mDtd;
	u8  interlaced = dtd->mInterlaced;
	u16 vactive = dtd->mVActive;
	u16 hactive = dtd->mHActive;
	u16 hblank  = dtd->mHBlanking;
	u16 hsync   = dtd->mHSyncPulseWidth;
	u16 hsync_delay = dtd->mHSyncOffset;
	u16 i = 0;

	dtd = &video->mDtd;
	dw_write_mask(FC_INVIDCONF, FC_INVIDCONF_VSYNC_IN_POLARITY_MASK,
			dtd->mVSyncPolarity);
	dw_write_mask(FC_INVIDCONF, FC_INVIDCONF_HSYNC_IN_POLARITY_MASK,
			dtd->mHSyncPolarity);
	dw_write_mask(FC_INVIDCONF, FC_INVIDCONF_DE_IN_POLARITY_MASK, 0x1);

	/* 1: HDMI; 0: DVI */
	dw_fc_set_tmds_mode(video->mHdmi);

	if ((video->mHdmiVideoFormat == DW_VIDEO_FORMAT_3D) &&
				(video->m3dStructure == HDMI_3D_STRUCTURE_FRAME_PACKING)) {
		if (interlaced)
			vactive = (dtd->mVActive << 2) + 3 * dtd->mVBlanking + 2;
		else
			vactive = (dtd->mVActive << 1) + dtd->mVBlanking;
		interlaced = 0;
	}
	dw_write_mask(FC_INVIDCONF, FC_INVIDCONF_R_V_BLANK_IN_OSC_MASK, interlaced);
	dw_write_mask(FC_INVIDCONF, FC_INVIDCONF_IN_I_P_MASK, interlaced);
	dw_write_mask(FC_INVACTIV0, FC_INVACTIV0_V_IN_ACTIV_MASK,
			(u8)(vactive >> 0));
	dw_write_mask(FC_INVACTIV1, FC_INVACTIV1_V_IN_ACTIV_HIGHT_MASK,
			(u8)(vactive >> 8));

	if (video->mEncodingOut == DW_COLOR_FORMAT_YCC420) {
		hactive = dtd->mHActive / 2;
		hblank  = dtd->mHBlanking / 2;
		hsync   = dtd->mHSyncPulseWidth / 2;
		hsync_delay = dtd->mHSyncOffset / 2;
	}

	dw_write(FC_INHACTIV0, (u8)(hactive));
	dw_write_mask(FC_INHACTIV1, FC_INHACTIV1_H_IN_ACTIV_MASK |
			FC_INHACTIV1_H_IN_ACTIV_12_MASK, (u8)(hactive >> 8));

	dw_write(FC_INHBLANK0, (u8)(hblank));
	dw_write_mask(FC_INHBLANK1, FC_INHBLANK1_H_IN_BLANK_MASK |
			FC_INHBLANK1_H_IN_BLANK_12_MASK, (u8)(hblank >> 8));

	dw_write(FC_HSYNCINWIDTH0, (u8)(hsync));
	dw_write_mask(FC_HSYNCINWIDTH1, FC_HSYNCINWIDTH1_H_IN_WIDTH_MASK, (u8)(hsync >> 8));

	dw_write(FC_HSYNCINDELAY0, (u8)(hsync_delay));
	dw_write_mask(FC_HSYNCINDELAY1, FC_HSYNCINDELAY1_H_IN_DELAY_MASK |
			FC_HSYNCINDELAY1_H_IN_DELAY_12_MASK, (u8)(hsync_delay >> 8));

	dw_write_mask(FC_INVBLANK, FC_INVBLANK_V_IN_BLANK_MASK,
			(u8)dtd->mVBlanking);
	dw_write_mask(FC_VSYNCINDELAY, FC_VSYNCINDELAY_V_IN_DELAY_MASK,
			(u8)dtd->mVSyncOffset);
	dw_write_mask(FC_VSYNCINWIDTH, FC_VSYNCINWIDTH_V_IN_WIDTH_MASK,
			(u8)dtd->mVSyncPulseWidth);

	dw_write(FC_CTRLDUR, 12);
	dw_write(FC_EXCTRLDUR, 32);

	/* spacing < 256^2 * config / tmdsClock, spacing <= 50ms
	 * worst case: tmdsClock == 25MHz => config <= 19
	 */
	dw_write(FC_EXCTRLSPAC, 0x1);

	for (i = 0; i < 3; i++)
		_dw_fc_video_set_preamble_filter((i + 1) * 11, i);

	dw_write_mask(FC_PRCONF, FC_PRCONF_INCOMING_PR_FACTOR_MASK,
			(dtd->mPixelRepetitionInput + 1));

	return 0;
}

void _dw_avi_set_color_metry(u8 value)
{
	dw_write_mask(FC_AVICONF1, FC_AVICONF1_COLORIMETRY_MASK, value);
}

void _dw_avi_set_extend_color_metry(u8 extColor)
{
	dw_write_mask(FC_AVICONF2,
			FC_AVICONF2_EXTENDED_COLORIMETRY_MASK, extColor);
}

u8 dw_avi_get_rgb_ycc(void)
{
	return dw_read_mask(FC_AVICONF0, FC_AVICONF0_RGC_YCC_INDICATION_MASK);
}

u8 dw_avi_get_video_code(void)
{
	return dw_read_mask(FC_AVIVID, FC_AVIVID_FC_AVIVID_MASK);
}

/**
 * @desc: Configure Colorimetry packets
 * @video: Video information structure
 */
void _dw_packet_gmd_config(struct dw_video_s *video)
{
	u8 temp = 0x0;
	u8 gamut_metadata[28] = {0};
	u8 length = sizeof(gamut_metadata) / sizeof(u8);
	int gdb_color_space = 0;

	/* disable gmd packet send */
	dw_write_mask(FC_GMD_EN, FC_GMD_EN_GMDENABLETX_MASK, 0x0);

	if (video->mColorimetryDataBlock != 0x1) {
		hdmi_trace("dw video not send gmd packet\n");
		return;
	}

	dw_write_mask(FC_GMD_HB, FC_GMD_HB_GMDGBD_PROFILE_MASK, 0x0);
	dw_write_mask(FC_GMD_CONF, FC_GMD_CONF_GMDPACKETSINFRAME_MASK, 0x1);
	dw_write_mask(FC_GMD_CONF, FC_GMD_CONF_GMDPACKETLINESPACING_MASK, 0x1);

	if (video->mColorimetry != DW_METRY_EXTENDED)
		return;

	switch (video->mExtColorimetry) {
	case DW_METRY_EXT_XV_YCC601:
		gdb_color_space = 1;
		break;
	case DW_METRY_EXT_XV_YCC709:
		gdb_color_space = 2;
		break;
	case DW_METRY_EXT_S_YCC601:
	case DW_METRY_EXT_ADOBE_YCC601:
	case DW_METRY_EXT_ADOBE_RGB:
		gdb_color_space = 3;
		break;
	default:
		gdb_color_space = 0;
		break;
	}

	gamut_metadata[0] = (1 << 7) | gdb_color_space;

	/* sequential */
	temp = (u8)(dw_read(FC_GMD_STAT) & 0xF);
	dw_write_mask(FC_GMD_HB, FC_GMD_HB_GMDAFFECTED_GAMUT_SEQ_NUM_MASK,
			(temp + 1) % 16);

	length = length > FC_GMD_PB_SIZE ? length : FC_GMD_PB_SIZE;
	for (temp = 0; temp < length; temp++)
		dw_write(FC_GMD_PB0 + temp, gamut_metadata[temp]);

	/* set next_field to 1 */
	dw_write_mask(FC_GMD_UP, FC_GMD_UP_GMDUPDATEPACKET_MASK, 0x1);

	/* enable gmd packet send */
	dw_write_mask(FC_GMD_EN, FC_GMD_EN_GMDENABLETX_MASK, 0x1);
}

int _dw_packet_vsi_config(void)
{
	struct dw_hdmi_dev_s *hdmi = dw_get_hdmi();
	struct dw_product_s  *prod = &hdmi->prod_dev;
	u8 length = prod->mVendorPayloadLength;
	int i = 0;

	hdmi_trace("[vsif packet]\n");

	/* prevent sending half the info. */
	dw_write_mask(FC_DATAUTO0, FC_DATAUTO0_VSD_AUTO_MASK, 0x0);

	/* write ieee-oui code */
	dw_write(FC_VSDIEEEID0, (prod->mOUI >> 0));
	dw_write(FC_VSDIEEEID1, (prod->mOUI >> 8));
	dw_write(FC_VSDIEEEID2, (prod->mOUI >> 16));
	hdmi_trace(" - oui code: 0x%x\n", prod->mOUI);

	if (length > 24)
		length = 24;
	for (i = 0; i < length; i++) {
		hdmi_trace(" - data[%d]: 0x%x\n", i, prod->mVendorPayload[i]);
		dw_write(FC_VSDPAYLOAD0 + i, prod->mVendorPayload[i]);
	}

	mdelay(1);

	dw_write_mask(FC_DATAUTO0, FC_DATAUTO0_VSD_AUTO_MASK, 0x1);
	dw_write_mask(FC_PACKET_TX_EN, FC_PACKET_TX_EN_AUT_TX_EN_MASK, 0x1);

	return 0;
}

void _dw_packet_avi_config(struct dw_video_s *video)
{
	u8 temp = 0;
	dw_dtd_t *dtd = &video->mDtd;
	char *avi_format[] = {"rgb", "yuv422", "yuv444", "yuv420"};
	char *scan_info[] = {"no-data", "overscan", "underscan"};

	hdmi_trace("[avi packet]\n");

	switch (video->mEncodingOut) {
	case DW_COLOR_FORMAT_YCC444:
		temp = 0x2;
		break;
	case DW_COLOR_FORMAT_YCC422:
		temp = 0x1;
		break;
	case DW_COLOR_FORMAT_YCC420:
		temp = 0x3;
		break;
	default:
		temp = 0x0;
		break;
	}
	dw_write_mask(FC_AVICONF0, FC_AVICONF0_RGC_YCC_INDICATION_MASK, temp);
	hdmi_trace(" - format: %s, scan: %s, cea code: %d\n",
			avi_format[temp], scan_info[video->mScanInfo], video->mCea_code);

	dw_write_mask(FC_AVICONF0, FC_AVICONF0_SCAN_INFORMATION_MASK,
			video->mScanInfo);

	temp = 0;
	if ((dtd->mHImageSize != 0 || dtd->mVImageSize != 0)
			&& (dtd->mHImageSize > dtd->mVImageSize)) {
		/* 16:9 or 4:3 */
		u8 pic = (dtd->mHImageSize * 10) % dtd->mVImageSize;
		temp = (pic > 5) ? 2 : 1;
	}
	dw_write_mask(FC_AVICONF1,
			FC_AVICONF1_PICTURE_ASPECT_RATIO_MASK, temp);

	dw_write_mask(FC_AVICONF2,
			FC_AVICONF2_IT_CONTENT_MASK, (video->mItContent ? 1 : 0));

	dw_write_mask(FC_AVICONF2,
			FC_AVICONF2_QUANTIZATION_RANGE_MASK, video->mRgbQuantizationRange);

	dw_write_mask(FC_AVICONF2,
			FC_AVICONF2_NON_UNIFORM_PICTURE_SCALING_MASK, video->mNonUniformScaling);

	dw_write(FC_AVIVID, video->mCea_code);

	if (video->mColorimetry == DW_METRY_EXTENDED) {
		/* ext colorimetry valid */
		if (video->mExtColorimetry != (u8) (-1)) {
			_dw_avi_set_extend_color_metry(video->mExtColorimetry);
			_dw_avi_set_color_metry(video->mColorimetry);/* EXT-3 */
		} else {
			_dw_avi_set_extend_color_metry(0);
			_dw_avi_set_color_metry(0);	/* No Data */
		}
	} else {
		_dw_avi_set_extend_color_metry(0);
		/* NODATA-0/ 601-1/ 709-2/ EXT-3 */
		_dw_avi_set_color_metry(video->mColorimetry);
	}

	dw_write_mask(FC_AVICONF1, FC_AVICONF1_ACTIVE_ASPECT_RATIO_MASK,
		video->mActiveFormatAspectRatio);
	dw_write_mask(FC_AVICONF0, FC_AVICONF0_ACTIVE_FORMAT_PRESENT_MASK,
		video->mActiveFormatAspectRatio != 0 ? 0x1 : 0x0);

	temp = 0x0;
	if (video->mEndTopBar != -1) {
		dw_write(FC_AVIETB0, (u8)(video->mEndTopBar));
		dw_write(FC_AVIETB1, (u8)(video->mEndTopBar >> 8));
		temp |= 0x1 << 1;
	}
	if (video->mStartBottomBar != -1) {
		dw_write(FC_AVISBB0, (u8)(video->mStartBottomBar));
		dw_write(FC_AVISBB1, (u8)(video->mStartBottomBar >> 8));
		temp |= 0x1 << 1;
	}
	if (video->mEndLeftBar != -1) {
		dw_write(FC_AVIELB0, (u8)(video->mEndLeftBar));
		dw_write(FC_AVIELB1, (u8)(video->mEndLeftBar >> 8));
		temp |= 0x1 << 0;
	}
	if (video->mStartRightBar != -1) {
		dw_write(FC_AVISRB0, (u8)(video->mStartRightBar));
		dw_write(FC_AVISRB1, (u8)(video->mStartRightBar >> 8));
		temp |= 0x1 << 0;
	}
	dw_write_mask(FC_AVICONF0, FC_AVICONF0_BAR_INFORMATION_MASK, temp);

	temp = (dtd->mPixelRepetitionInput + 1) *
				(video->mPixelRepetitionFactor + 1) - 1;
	dw_write_mask(FC_PRCONF, FC_PRCONF_OUTPUT_PR_FACTOR_MASK, temp);
}

void dw_gcp_set_avmute(u8 enable)
{
	dw_write_mask(FC_GCP, FC_GCP_SET_AVMUTE_MASK, (enable ? 1 : 0));
	dw_write_mask(FC_GCP, FC_GCP_CLEAR_AVMUTE_MASK, (enable ? 0 : 1));
}

u8 dw_gcp_get_avmute(void)
{
	return dw_read_mask(FC_GCP, FC_GCP_SET_AVMUTE_MASK);
}

void dw_drm_packet_clear(dw_fc_drm_pb_t *data)
{
	if (IS_ERR_OR_NULL(data)) {
		shdmi_err(data);
		return;
	}

	memset(data, 0x0, sizeof(dw_fc_drm_pb_t));
}

int dw_drm_packet_filling_data(dw_fc_drm_pb_t *data)
{
	data->r_x      = 0x33c2;
	data->r_y      = 0x86c4;
	data->g_x      = 0x1d4c;
	data->g_y      = 0x0bb8;
	data->b_x      = 0x84d0;
	data->b_y      = 0x3e80;
	data->w_x      = 0x3d13;
	data->w_y      = 0x4042;
	data->luma_max = 0x03e8;
	data->luma_min = 0x1;
	data->mcll     = 0x03e8;
	data->mfll     = 0x0190;

	hdmi_trace("dw drm data filling done\n");
	return 0;
}

void dw_drm_packet_config(dw_fc_drm_pb_t *pb)
{
	int timeout = 10;
	u32 status = 0;

	if (pb != 0) {
		hdmi_trace("[drm packet]\n");
		hdmi_trace(" - eotf: %d, metadata: %d\n",
			pb->eotf & 0x07,  pb->metadata & 0x07);
		hdmi_trace(" - r_x: %d, r_y: %d\n", pb->r_x, pb->r_y);
		hdmi_trace(" - g_x: %d, g_y: %d\n", pb->g_x, pb->g_y);
		hdmi_trace(" - b_x: %d, b_y: %d\n", pb->b_x, pb->b_y);
		hdmi_trace(" - w_x: %d, w_y: %d\n", pb->w_x, pb->w_y);
		hdmi_trace(" - luma_max: %d, luma_min: %d, mcll: %d, mfll: %d\n",
			pb->luma_max, pb->luma_min, pb->mcll, pb->mfll);

		dw_write(FC_DRM_PB0, pb->eotf & 0x07);
		dw_write(FC_DRM_PB1, pb->metadata & 0x07);
		dw_write(FC_DRM_PB2, (pb->r_x >> 0) & 0xff);
		dw_write(FC_DRM_PB3, (pb->r_x >> 8) & 0xff);
		dw_write(FC_DRM_PB4, (pb->r_y >> 0) & 0xff);
		dw_write(FC_DRM_PB5, (pb->r_y >> 8) & 0xff);
		dw_write(FC_DRM_PB6, (pb->g_x >> 0) & 0xff);
		dw_write(FC_DRM_PB7, (pb->g_x >> 8) & 0xff);
		dw_write(FC_DRM_PB8, (pb->g_y >> 0) & 0xff);
		dw_write(FC_DRM_PB9, (pb->g_y >> 8) & 0xff);
		dw_write(FC_DRM_PB10, (pb->b_x >> 0) & 0xff);
		dw_write(FC_DRM_PB11, (pb->b_x >> 8) & 0xff);
		dw_write(FC_DRM_PB12, (pb->b_y >> 0) & 0xff);
		dw_write(FC_DRM_PB13, (pb->b_y >> 8) & 0xff);
		dw_write(FC_DRM_PB14, (pb->w_x >> 0) & 0xff);
		dw_write(FC_DRM_PB15, (pb->w_x >> 8) & 0xff);
		dw_write(FC_DRM_PB16, (pb->w_y >> 0) & 0xff);
		dw_write(FC_DRM_PB17, (pb->w_y >> 8) & 0xff);
		dw_write(FC_DRM_PB18, (pb->luma_max >> 0) & 0xff);
		dw_write(FC_DRM_PB19, (pb->luma_max >> 8) & 0xff);
		dw_write(FC_DRM_PB20, (pb->luma_min >> 0) & 0xff);
		dw_write(FC_DRM_PB21, (pb->luma_min >> 8) & 0xff);
		dw_write(FC_DRM_PB22, (pb->mcll >> 0) & 0xff);
		dw_write(FC_DRM_PB23, (pb->mcll >> 8) & 0xff);
		dw_write(FC_DRM_PB24, (pb->mfll >> 0) & 0xff);
		dw_write(FC_DRM_PB25, (pb->mfll >> 8) & 0xff);
	 }
	dw_write_mask(FC_DRM_HB0, FC_DRM_UP_FC_DRM_HB_MASK, 0x01);
	dw_write_mask(FC_DRM_HB1, FC_DRM_UP_FC_DRM_HB_MASK, 26);
	dw_write_mask(FC_PACKET_TX_EN, FC_PACKET_TX_EN_DRM_TX_EN_MASK, 0x1);

	do {
		udelay(10);
		status = dw_read_mask(FC_DRM_UP, FC_DRM_UP_DRMPACKETUPDATE_MASK);
	} while (status && (timeout--));

	dw_write_mask(FC_DRM_UP,  FC_DRM_UP_DRMPACKETUPDATE_MASK, 0x1);
}

/**
 * @desc: packets configure is the same as infoframe configure
 * @return: 0 - config infoframe success
 *         -1 - not config infoframe
 */
int dw_infoframe_packet(void)
{
	struct dw_hdmi_dev_s *hdmi = dw_get_hdmi();
	struct dw_video_s   *video = &hdmi->video_dev;

	if (video->mHdmi != DW_TMDS_MODE_HDMI) {
		hdmi_inf("dw packet unset when dvi mode\n");
		return 0;
	}

	_dw_packet_vsi_config();

	/* default phase 1 = true */
	dw_write_mask(FC_GCP, FC_GCP_DEFAULT_PHASE_MASK,
			((video->mPixelPackingDefaultPhase == 1) ? 1 : 0));

	_dw_packet_avi_config(video);

	/* gamut metadata */
	_dw_packet_gmd_config(video);

	dw_write_mask(FC_PACKET_TX_EN, FC_PACKET_TX_EN_DRM_TX_EN_MASK, 0x0);
	if (video->mHdr) {
		dw_drm_packet_config(video->pb);
		hdmi_trace("dw drm packet for hdr config done\n");
	}

	dw_write_mask(FC_DATAUTO1, FC_DATAUTO1_AUTO_FRAME_INTERPOLATION_MASK, 0x1);
	dw_write_mask(FC_DATAUTO2, FC_DATAUTO2_AUTO_FRAME_PACKETS_MASK, 0x1);
	dw_write_mask(FC_DATAUTO2, FC_DATAUTO2_AUTO_LINE_SPACING_MASK, 0x1);

	hdmi_trace("dw infoframe config done!\n");
	return 0;
}

int dw_fc_iteration_process(void)
{
	dw_write(FC_INVIDCONF, dw_read(FC_INVIDCONF));
	return 0;
}