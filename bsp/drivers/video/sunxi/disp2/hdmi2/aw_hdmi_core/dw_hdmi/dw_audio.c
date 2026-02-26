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
#include "dw_mc.h"
#include "dw_fc.h"
#include "dw_access.h"
#include "dw_audio.h"

typedef struct audio_n_computation {
	u32 pixel_clock;/* KHZ */
	u32 n;
} audio_n_computation_t;

static audio_n_computation_t n_values_32[] = {
	{0,      4096},
	{25175,  4576},
	{25200,  4096},
	{27000,  4096},
	{27027,  4096},
	{31500,  4096},
	{33750,  4096},
	{54000,  4096},
	{54054,  4096},
	{67500,  4096},
	{74176,  11648},
	{74250,  4096},
	{928125, 8192},
	{148352, 11648},
	{148500, 4096},
	{185625, 4096},
	{296703, 5824},
	{297000, 3072},
	{371250, 6144},
	{5940000, 3072},
	{0, 0}
};

static audio_n_computation_t n_values_44p1[] = {
	{0,      6272},
	{25175,  7007},
	{25200,  6272},
	{27000,  6272},
	{27027,  6272},
	{31500,  6272},
	{33750,  6272},
	{54000,  6272},
	{54054,  6272},
	{67500,  6272},
	{74176,  17836},
	{74250,  6272},
	{92812,  6272},
	{148352, 8918},
	{148500, 6272},
	{185625, 6272},
	{296703, 4459},
	{297000, 4704},
	{371250, 4704},
	{594000, 9048},
	{0, 0}
};

static audio_n_computation_t n_values_48[] = {
	{0,      6144},
	{25175,  6864},
	{25200,  6144},
	{27000,  6144},
	{27027,  6144},
	{31500,  6144},
	{33750,  6144},
	{54000,  6144},
	{54054,  6144},
	{67500,  6144},
	{74176,  11648},
	{74250,  6144},
	{928125, 12288},
	{148352, 5824},
	{148500, 6144},
	{185625, 6144},
	{296703, 5824},
	{297000, 5120},
	{371250, 5120},
	{594000, 6144},
	{0, 0}
};

static void _dw_audio_set_clock_n(dw_hdmi_dev_t *dev, u32 value)
{
	/* 19-bit width */
	dev_write(dev, AUD_N1, (u8)(value >> 0));
	dev_write(dev, AUD_N2, (u8)(value >> 8));
	dev_write_mask(dev, AUD_N3, AUD_N3_AUDN_MASK, (u8)(value >> 16));
	/* no shift */
	dev_write_mask(dev, AUD_CTS3, AUD_CTS3_N_SHIFT_MASK, 0);
}

static void _dw_audio_set_clock_cts(dw_hdmi_dev_t *dev, u32 value)
{
	if (value > 0) {
		/* 19-bit width */
		dev_write(dev, AUD_CTS1, (u8)(value >> 0));
		dev_write(dev, AUD_CTS2, (u8)(value >> 8));
		dev_write_mask(dev, AUD_CTS3, AUD_CTS3_AUDCTS_MASK,
							(u8)(value >> 16));
		dev_write_mask(dev, AUD_CTS3, AUD_CTS3_CTS_MANUAL_MASK, 1);
	} else {
		/* Set to automatic generation of CTS values */
		dev_write_mask(dev, AUD_CTS3, AUD_CTS3_CTS_MANUAL_MASK, 0);
	}
}

static void _dw_audio_set_clock_fs(dw_hdmi_dev_t *dev, u8 value)
{
	dev_write_mask(dev, AUD_INPUTCLKFS, AUD_INPUTCLKFS_IFSFACTOR_MASK, value);
}

static u8 _dw_audio_get_clock_fs(dw_hdmi_dev_t *dev)
{
	return dev_read_mask(dev, AUD_INPUTCLKFS, AUD_INPUTCLKFS_IFSFACTOR_MASK);
}

static void _dw_audio_i2s_data_enable(dw_hdmi_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, AUD_CONF0, AUD_CONF0_I2S_IN_EN_MASK, value);
}

static void _dw_audio_set_i2s_mode(dw_hdmi_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, AUD_CONF1, AUD_CONF1_I2S_MODE_MASK, value);
}

static void _dw_audio_set_i2s_data_width(dw_hdmi_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, AUD_CONF1, AUD_CONF1_I2S_WIDTH_MASK, value);
}

static void _dw_audio_i2s_interrupt_mask(dw_hdmi_dev_t *dev, u8 value)
{
	u8 mask = 0;
	LOG_TRACE1(value);

	mask  = AUD_INT_FIFO_FULL_MASK_MASK;
	mask |= AUD_INT_FIFO_EMPTY_MASK_MASK;
	dev_write_mask(dev, AUD_INT, mask, value);
}

static void _dw_audio_spdif_interrupt_mask(dw_hdmi_dev_t *dev, u8 value)
{
	u8 mask = 0x0;
	LOG_TRACE1(value);

	mask = AUD_SPDIFINT_SPDIF_FIFO_FULL_MASK_MASK;
	mask |= AUD_SPDIFINT_SPDIF_FIFO_EMPTY_MASK_MASK;
	dev_write_mask(dev, AUD_SPDIFINT, mask, value);
}

static void _dw_audio_set_pcuv_insert_mode(dw_hdmi_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, AUD_CONF2, AUD_CONF2_INSERT_PCUV_MASK, value);
}

static void _dw_audio_nlpcm_select(dw_hdmi_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, AUD_CONF2, AUD_CONF2_NLPCM_MASK, value);
}

static void _dw_audio_hbr_select(dw_hdmi_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, AUD_CONF2, AUD_CONF2_HBR_MASK, value);
}

static int _dw_audio_i2s_config(dw_hdmi_dev_t *dev, dw_audio_param_t *audio)
{
	/* select i2s interface */
	dev_write_mask(dev, AUD_CONF0, AUD_CONF0_I2S_SELECT_MASK, 0x1);

	/* i2s data off */
	_dw_audio_i2s_data_enable(dev, 0x0);

	if (audio->mCodingType == DW_AUD_CODING_PCM) {
		_dw_audio_set_pcuv_insert_mode(dev, 1);
		_dw_audio_hbr_select(dev, 0);
		_dw_audio_nlpcm_select(dev, 0);
	} else if ((audio->mCodingType == DW_AUD_CODING_DTS_HD) ||
			(audio->mCodingType == DW_AUD_CODING_MAT)) {
		/* HBR mode, I2S input is ICE60958 frame,
		you must trans the ICE61937 frame to 60958 frame
		before it be sent to i2s  */
		/* HBR mode: the frame rate>192Khz, the ACR should be fs/4 */
		_dw_audio_set_pcuv_insert_mode(dev, 0);
		_dw_audio_hbr_select(dev, 1);
		_dw_audio_nlpcm_select(dev, 1);
	} else {
		/* NO-PCM  mode, I2S input is ICE60958 frame, you must trans the
		ICE61937 frame to 60958 frame before it be sent to i2s */
		/* NO-PCM  mode: the frame rate<=192Khz, the ACR should be fs */
		_dw_audio_set_pcuv_insert_mode(dev, 0);
		_dw_audio_hbr_select(dev, 0);
		_dw_audio_nlpcm_select(dev, 1);
	}

	if (audio->mCodingType != DW_AUD_CODING_PCM)
		_dw_audio_set_i2s_data_width(dev, 24);
	else
		_dw_audio_set_i2s_data_width(dev, audio->mSampleSize);

	/* ATTENTION: fixed I2S data mode (standard) */
	_dw_audio_set_i2s_mode(dev, 0x0);
	_dw_audio_i2s_interrupt_mask(dev, 0x3);

	/* reset i2s fifo */
	dev_write_mask(dev, AUD_CONF0, AUD_CONF0_SW_AUDIO_FIFO_RST_MASK, 0x1);

	dw_mc_audio_i2s_reset(dev, 0);

	switch (audio->mClockFsFactor) {
	case 64:
		_dw_audio_set_clock_fs(dev, 4);
		break;
	case 128:
		_dw_audio_set_clock_fs(dev, 0);
		break;
	case 256:
		_dw_audio_set_clock_fs(dev, 1);
		break;
	case 512:
		_dw_audio_set_clock_fs(dev, 2);
		break;
	default:
		_dw_audio_set_clock_fs(dev, 0);
		hdmi_err("%s: Fs Factor [%d] not supported!\n", __func__,
					 audio->mClockFsFactor);
		return false;
	}

	return true;
}

static u16 _dw_audio_get_fs_factor(dw_hdmi_dev_t *dev)
{
	u8 fac = _dw_audio_get_clock_fs(dev);
	u16 fsFac = 0;

	switch (fac) {
	case 4:
		fsFac = 64;
		break;
	case 0:
		fsFac = 128;
		break;
	case 1:
		fsFac = 256;
		break;
	case 2:
		fsFac = 512;
		break;
	default:
		hdmi_err("%s: error audio clock factor [%d]\n", __func__, fac);
		fsFac = 0;
		break;
	}

	return fsFac;
}

static u32 _dw_audio_sw_compute_n(dw_hdmi_dev_t *dev, u32 freq, u32 pixelClk)
{
	int i = 0;
	u32 n = 0;
	audio_n_computation_t *n_values = NULL;
	int multiplier_factor = 1;

	if ((freq == 64000) || (freq == 882000) || (freq == 96000))
		multiplier_factor = 2;
	else if ((freq == 128000) || (freq == 176400) || (freq == 192000))
		multiplier_factor = 4;
	else if ((freq == 256000) || (freq == 3528000) || (freq == 384000))
		multiplier_factor = 8;

	if (32000 == (freq/multiplier_factor)) {
		n_values = n_values_32;
	} else if (44100 == (freq/multiplier_factor)) {
		n_values = n_values_44p1;
	} else if (48000 == (freq/multiplier_factor)) {
		n_values = n_values_48;
	} else {
		hdmi_err("%s: get n param frequency %dhz, pixel clk %dkhz, factor %d is unvalid!\n",
			__func__, freq/1000, pixelClk, multiplier_factor);
		return false;
	}

	for (i = 0; n_values[i].n != 0; i++) {
		if (pixelClk == n_values[i].pixel_clock) {
			n = n_values[i].n * multiplier_factor;
			AUDIO_INF("get acr n value = %d\n", n);
			return n;
		}
	}

	n = n_values[0].n * multiplier_factor;

	hdmi_wrn("use default acr n value = %d\n", n);

	return n;
}

int dw_audio_param_reset(dw_audio_param_t *audio)
{
	if (audio == NULL) {
		hdmi_err("%s: param audio is null!!!\n", __func__);
		return -1;
	}

	memset(audio, 0, sizeof(dw_audio_param_t));

	audio->mInterfaceType = DW_AUDIO_INTERFACE_I2S;
	audio->mCodingType = DW_AUD_CODING_PCM;
	audio->mSamplingFrequency = 44100;
	audio->mChannelAllocation = 0;
	audio->mChannelNum = 2;
	audio->mSampleSize = 16;
	audio->mClockFsFactor = 64;
	return 0;
}

/**
 * Initial set up of package and prepare it to be configured.
 * Set audio mute to on.
 * @param baseAddr base Address of controller
 * @return true if successful
 */
void dw_audio_initialize(dw_hdmi_dev_t *dev)
{
	LOG_TRACE();

	/* Mask all interrupts */
	_dw_audio_i2s_interrupt_mask(dev, 0x3);
	_dw_audio_spdif_interrupt_mask(dev, 0x3);

	dw_fc_audio_set_mute(dev,  1);
}

int dw_audio_config(dw_hdmi_dev_t *dev, dw_audio_param_t *audio)
{
	u32 n = 0;

	LOG_TRACE();

	if ((dev == NULL) || (audio == NULL)) {
		hdmi_err("%s: param is null!!!\n", __func__);
		return false;
	}

	if (dev->snps_hdmi_ctrl.audio_on == 0) {
		hdmi_wrn("audio is not on\n");
		return true;
	}

	if (dev->snps_hdmi_ctrl.hdmi_on == 0) {
		hdmi_wrn("avi mode, audio is not configured\n");
		return true;
	}

	if ((audio->mCodingType == DW_AUD_CODING_ONE_BIT_AUDIO) ||
			(audio->mCodingType == DW_AUD_CODING_DST)) {
		hdmi_err("%s: hdmi does not support this coding type: %d\n", __func__,
			audio->mCodingType);
		return false;

	}

	if ((dev->snps_hdmi_ctrl.pixel_clock < 74250) &&
		((audio->mChannelNum * audio->mSamplingFrequency) > 384000)) {
		hdmi_err("%s: hdmi does not support this audio framerate on this video format %d %d\n",
				__func__, dev->snps_hdmi_ctrl.pixel_clock,
				(audio->mChannelNum * audio->mSamplingFrequency));
		return false;
	}

	/* Set PCUV info from external source */
	audio->mGpaInsertPucv = 1;
	dw_fc_audio_set_mute(dev, 1);

	/* Configure Frame Composer audio parameters */
	dw_fc_audio_config(dev, audio);

	audio->mClockFsFactor = 64;
	if (audio->mInterfaceType == DW_AUDIO_INTERFACE_I2S) {
		if (_dw_audio_i2s_config(dev, audio) == false) {
			hdmi_err("%s: audio i2s config is failed!!!\n", __func__);
			return false;
		}
	} else {
		hdmi_err("%s: audio interface type %d is unsupport!\n", __func__, audio->mInterfaceType);
		return false;
	}

	n = _dw_audio_sw_compute_n(dev,
		audio->mSamplingFrequency, dev->snps_hdmi_ctrl.tmds_clk);
	_dw_audio_set_clock_n(dev, n);
	_dw_audio_set_clock_cts(dev, 0);

	if (!(audio->mChannelNum % 2))
		_dw_audio_i2s_data_enable(dev, (1 << (audio->mChannelNum / 2)) - 1);
	else
		_dw_audio_i2s_data_enable(dev, (1 << ((audio->mChannelNum + 1) / 2)) - 1);

	dw_fc_audio_set_mute(dev, 0);

	/* Configure audio info frame packets */
	dw_fc_audio_packet_config(dev, audio);
	return true;
}

u32 dw_audio_get_clock_n(dw_hdmi_dev_t *dev)
{
	return (u32)(dev_read(dev, AUD_N1) |
		(dev_read(dev, AUD_N2) << 8) |
		(dev_read_mask(dev, AUD_N3, AUD_N3_AUDN_MASK) << 16));
}

ssize_t dw_audio_dump(dw_hdmi_dev_t *dev, char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "audio mute:%d\n\n", dw_fc_audio_get_mute(dev));
	n += sprintf(buf + n, "audio mute:%d\n\n", _dw_audio_get_fs_factor(dev));

	return n;
}
