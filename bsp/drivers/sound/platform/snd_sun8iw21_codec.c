/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2025 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2023, xudongpdc <xudongpdc@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#define SUNXI_MODNAME		"sound-codec"
#include "snd_sunxi_log.h"
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/reset.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/regmap.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/math.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sunxi-sid.h>

#include <sound/pcm.h>
#include <sound/pcm_params.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_pcm.h"
#include "snd_sunxi_rxsync.h"
#include "snd_sunxi_dap.h"
#include "snd_sunxi_common.h"
#include "snd_sun8iw21_codec.h"

#define DRV_NAME	"sunxi-snd-codec"

static struct audio_reg_label sunxi_reg_labels[] = {
	REG_LABEL(SUNXI_DAC_DPC),
	REG_LABEL(SUNXI_DAC_VOL_CTRL),
	REG_LABEL(SUNXI_DAC_FIFOC),
	REG_LABEL(SUNXI_DAC_FIFOS),
	REG_LABEL(SUNXI_DAC_CNT),
	REG_LABEL(SUNXI_DAC_DG),
	REG_LABEL(SUNXI_ADC_FIFOC),
	REG_LABEL(SUNXI_ADC_VOL_CTRL),
	REG_LABEL(SUNXI_ADC_FIFOS),
	REG_LABEL(SUNXI_ADC_CNT),
	REG_LABEL(SUNXI_ADC_DG),
	REG_LABEL(SUNXI_ADC_DIG_CTRL),
	REG_LABEL(SUNXI_VRA1SPEEDUP_DOWN_CTRL),
	REG_LABEL(SUNXI_DAC_DAP_CTL),
	REG_LABEL(SUNXI_ADC_DAP_CTL),
	REG_LABEL(SUNXI_ADC1_REG),
	REG_LABEL(SUNXI_ADC2_REG),
	REG_LABEL(SUNXI_DAC_REG),
	REG_LABEL(SUNXI_MICBIAS_REG),
	REG_LABEL(SUNXI_RAMP_REG),
	REG_LABEL(SUNXI_BIAS_REG),
	REG_LABEL(SUNXI_POWER_REG),
	REG_LABEL(SUNXI_ADC_CUR_REG),
};
static struct audio_reg_group sunxi_reg_group = REG_GROUP(sunxi_reg_labels);

struct sample_rate {
	unsigned int samplerate;
	unsigned int rate_bit;
};

static const struct sample_rate sample_rate_conv[] = {
	{8000,   5},
	{11025,  4},
	{12000,  4},
	{16000,  3},
	{22050,  2},
	{24000,  2},
	{32000,  1},
	{44100,  0},
	{48000,  0},
	{96000,  7},
	{192000, 6},
};

static int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_codec_clk *clk);
static void snd_sunxi_clk_exit(struct sunxi_codec_clk *clk);
static int snd_sunxi_clk_enable(struct sunxi_codec_clk *clk);
static int snd_sunxi_clk_bus_enable(struct sunxi_codec_clk *clk);
static void snd_sunxi_clk_disable(struct sunxi_codec_clk *clk);
static void snd_sunxi_clk_bus_disable(struct sunxi_codec_clk *clk);
static int snd_sunxi_clk_rate(struct sunxi_codec_clk *clk, int stream,
			      unsigned int freq_in, unsigned int freq_out);

static int snd_sunxi_rglt_init(struct platform_device *pdev, struct sunxi_codec_rglt *rglt);
static void snd_sunxi_rglt_exit(struct sunxi_codec_rglt *rglt);
static int snd_sunxi_rglt_enable(struct sunxi_codec_rglt *rglt);
static void snd_sunxi_rglt_disable(struct sunxi_codec_rglt *rglt);

static void sunxi_rx_sync_enable(void *data, bool enable);

static int sunxi_codec_dai_hw_params(struct snd_pcm_substream *substream,
					      struct snd_pcm_hw_params *params,
					      struct snd_soc_dai *dai)
{
	int i;
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	/* set bits */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
				0x3 << FIFO_MODE, 0x3 << FIFO_MODE);
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
				0x1 << TX_SAMPLE_BITS, 0x0 << TX_SAMPLE_BITS);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				0x1 << RX_FIFO_MODE, 0x1 << RX_FIFO_MODE);
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				0x1 << RX_SAMPLE_BITS, 0x0 << RX_SAMPLE_BITS);
		}
		break;
	case SNDRV_PCM_FORMAT_S24_3LE:
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
				0x3 << FIFO_MODE, 0x0 << FIFO_MODE);
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
				0x1 << TX_SAMPLE_BITS, 0x1 << TX_SAMPLE_BITS);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				0x1 << RX_FIFO_MODE, 0x0 << RX_FIFO_MODE);
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				0x1 << RX_SAMPLE_BITS, 0x1 << RX_SAMPLE_BITS);
		}
		break;
	default:
		break;
	}

	/* set rate */
	for (i = 0; i < ARRAY_SIZE(sample_rate_conv); i++) {
		if (sample_rate_conv[i].samplerate == params_rate(params)) {
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
					0x7 << DAC_FS,
					sample_rate_conv[i].rate_bit << DAC_FS);
			} else {
				if (sample_rate_conv[i].samplerate > 48000)
					return -EINVAL;
				regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
					0x7 << ADC_FS,
					sample_rate_conv[i].rate_bit << ADC_FS);
			}
		}
	}

	/* set channels */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (params_channels(params)) {
		case 1:
			/* DACL & DACR send same data */
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
				0x1 << DAC_MONO_EN, 0x1 << DAC_MONO_EN);
			break;
		case 2:
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
				0x1 << DAC_MONO_EN, 0x0 << DAC_MONO_EN);
			break;
		default:
			SND_LOG_WARN("not support channels:%u",
				     params_channels(params));
			return -EINVAL;
		}
	}

	/* enable clk after set clk rate */
	if (snd_sunxi_clk_enable(&codec->clk)) {
		SND_LOG_ERR("clk enable failed\n");
		return -EINVAL;
	} else {
		codec->clk_sta = SND_SUNXI_CLK_OPEN;
	}

	return 0;
}

static int sunxi_codec_dai_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_clk *clk = &codec->clk;

	SND_LOG_DEBUG("\n");

	/* prevent the closed clks from being closed again */
	if (codec->clk_sta == SND_SUNXI_CLK_OPEN) {
		snd_sunxi_clk_disable(clk);
		codec->clk_sta = SND_SUNXI_CLK_CLOSE;
	}

	return 0;
}

static int sunxi_codec_dai_set_pll(struct snd_soc_dai *dai,
					    int pll_id, int source,
					    unsigned int freq_in,
					    unsigned int freq_out)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_clk *clk = &codec->clk;

	SND_LOG_DEBUG("stream -> %s, freq_in ->%u, freq_out ->%u\n",
		      pll_id ? "IN" : "OUT", freq_in, freq_out);

	return snd_sunxi_clk_rate(clk, pll_id, freq_in, freq_out);
}

static int sunxi_codec_dai_prepare(struct snd_pcm_substream *substream,
					    struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
				   1 << FIFO_FLUSH, 1 << FIFO_FLUSH);
		regmap_write(regmap, SUNXI_DAC_FIFOS,
			     1 << DAC_TXE_INT |\
			     1 << DAC_TXU_INT |\
			     1 << DAC_TXO_INT);
		regmap_write(regmap, SUNXI_DAC_CNT, 0);
	} else {
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   1 << ADC_FIFO_FLUSH, 1 << ADC_FIFO_FLUSH);
		regmap_write(regmap, SUNXI_ADC_FIFOS,
			     1 << ADC_RXA_INT |\
			     1 << ADC_RXO_INT);
		regmap_write(regmap, SUNXI_ADC_CNT, 0);
	}

	return 0;
}

static void sunxi_pa_disa_cb(void *data)
{
	struct regmap *regmap = (struct regmap *)data;

	SND_LOG_DEBUG("\n");

	regmap_update_bits(regmap, SUNXI_DAC_FIFOC, 0x1 << DAC_DRQ_EN, 0x0 << DAC_DRQ_EN);
}

static int sunxi_codec_dai_trigger(struct snd_pcm_substream *substream,
					    int cmd,
					    struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_dts *dts = &codec->dts;
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("cmd -> %d\n", cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
				   0x1 << DAC_DRQ_EN, 0x1 << DAC_DRQ_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   0x1 << ADC_DRQ_EN, 0x1 << ADC_DRQ_EN);
			if (dts->rx_sync_en && dts->rx_sync_ctl)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, true);
		}
	break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
					   0x1 << DAC_DRQ_EN, 0x1 << DAC_DRQ_EN);
			if (codec->audio_sta.spk)
				snd_sunxi_pa_pin_enable(codec->pa_cfg, codec->pa_pin_max);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
					   0x1 << ADC_DRQ_EN, 0x1 << ADC_DRQ_EN);
			if (dts->rx_sync_en && dts->rx_sync_ctl)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, true);
		}
	break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
				   0x1 << DAC_DRQ_EN, 0x0 << DAC_DRQ_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   0x1 << ADC_DRQ_EN, 0x0 << ADC_DRQ_EN);
			if (dts->rx_sync_en && dts->rx_sync_ctl)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, false);
		}
	break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (codec->audio_sta.spk)
				snd_sunxi_pa_pin_disable_irp(codec->pa_cfg, codec->pa_pin_max,
							    (void *)regmap, sunxi_pa_disa_cb);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   0x1 << ADC_DRQ_EN, 0x0 << ADC_DRQ_EN);
			if (dts->rx_sync_en && dts->rx_sync_ctl)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, false);
		}
	break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct snd_soc_dai_ops sunxi_codec_dai_ops = {
	.set_pll	= sunxi_codec_dai_set_pll,
	.hw_params	= sunxi_codec_dai_hw_params,
	.prepare	= sunxi_codec_dai_prepare,
	.trigger	= sunxi_codec_dai_trigger,
	.hw_free	= sunxi_codec_dai_hw_free,
};

static struct snd_soc_dai_driver sunxi_codec_dai = {
	.name	= DRV_NAME,
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S20_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.stream_name	= "Capture",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= SNDRV_PCM_RATE_8000_48000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S20_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &sunxi_codec_dai_ops,
};

/*******************************************************************************
 * *** sound card & component function source ***
 * @0 sound card probe
 * @1 component function kcontrol register
 ******************************************************************************/
/* Enum definition */
static const char *sunxi_switch_text[] = {"Off", "On"};
static const char *sunxi_adda_loop_mode_text[] = {"Off", "DACL-to-ADC1"};

static SOC_ENUM_SINGLE_EXT_DECL(sunxi_tx_hub_mode_enum, sunxi_switch_text);
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_rx_sync_mode_enum, sunxi_switch_text);

/* kcontrol setting */
static SOC_ENUM_SINGLE_DECL(sunxi_codec_dacdrc_mode_enum, SND_SOC_NOPM, DACDRC_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_codec_dachpf_mode_enum, SND_SOC_NOPM, DACHPF_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_codec_adcdrc0_mode_enum, SND_SOC_NOPM, ADCDRC0_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_codec_adchpf0_mode_enum, SND_SOC_NOPM, ADCHPF0_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_codec_adcdrc1_mode_enum, SND_SOC_NOPM, ADCDRC1_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_codec_adchpf1_mode_enum, SND_SOC_NOPM, ADCHPF1_SHIFT, sunxi_switch_text);

static SOC_ENUM_SINGLE_EXT_DECL(sunxi_adda_loop_mode_enum, sunxi_adda_loop_mode_text);

static SOC_ENUM_SINGLE_DECL(sunxi_codec_adc_swap_enum, SUNXI_ADC_DG, AD_SWP1, sunxi_switch_text);

static const DECLARE_TLV_DB_SCALE(digital_tlv, -7424, 116, 0);
static const DECLARE_TLV_DB_SCALE(dac_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(adc_vol_tlv, -11925, 75, 0);

static const DECLARE_TLV_DB_SCALE(adc1_gain_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(adc2_gain_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(linein_gain_tlv, 0, 600, 0);

static const unsigned int lineout_tlv[] = {
	TLV_DB_RANGE_HEAD(2),
	0, 1, TLV_DB_SCALE_ITEM(0, 0, 1),
	2, 31, TLV_DB_SCALE_ITEM(-4350, 150, 1),
};

/* RX&TX FUNC */
static int sunxi_get_tx_hub_mode(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_DAC_DPC, &reg_val);

	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << DAC_HUB_EN)) ? 1 : 0);

	return 0;
}

static int sunxi_set_tx_hub_mode(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << LINEOUTLEN, 0x0 << LINEOUTLEN);
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << DACLEN, 0x0 << DACLEN);
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << DACLMUTE, 0x0 << DACLMUTE);
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << EN_DAC, 0x0 << EN_DAC);
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << DAC_HUB_EN, 0x0 << DAC_HUB_EN);
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << DAC_HUB_EN, 0x1 << DAC_HUB_EN);
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << EN_DAC, 0x1 << EN_DAC);
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << DACLMUTE, 0x1 << DACLMUTE);
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << DACLEN, 0x1 << DACLEN);
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << LINEOUTLEN, 0x1 << LINEOUTLEN);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void sunxi_rx_sync_enable(void *data, bool enable)
{
	struct regmap *regmap = data;

	SND_LOG_DEBUG("%s\n", enable ? "on" : "off");

	if (enable) {
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   1 << RX_SYNC_EN_START, 1 << RX_SYNC_EN_START);
	} else {
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   1 << RX_SYNC_EN_START, 0 << RX_SYNC_EN_START);
	}

	return;
}

static int sunxi_get_rx_sync_mode(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_dts *dts = &codec->dts;

	ucontrol->value.integer.value[0] = dts->rx_sync_ctl;

	return 0;
}

static int sunxi_set_rx_sync_mode(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_dts *dts = &codec->dts;
	struct regmap *regmap = codec->mem.regmap;

	if (dts->rx_sync_ctl == ucontrol->value.integer.value[0])
		return 0;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		dts->rx_sync_ctl = 0;
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x1 << RX_SYNC_EN, 0x0 << RX_SYNC_EN);
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x1 << ADCDFEN, 0x1 << ADCDFEN);
		sunxi_rx_sync_shutdown(dts->rx_sync_domain, dts->rx_sync_id);
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x1 << RX_SYNC_EN, 0x1 << RX_SYNC_EN);
		/* Cancel the delay time in order to align the start point */
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x1 << ADCDFEN, 0x0 << ADCDFEN);
		dts->rx_sync_ctl = 1;
		sunxi_rx_sync_startup(dts->rx_sync_domain, dts->rx_sync_id);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

/* DAC&ADC - DRC&HPF FUNC */
static int sunxi_codec_get_dap_status(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int shift = e->shift_l;
	unsigned int reg_val;

	switch (shift) {
	case DACDRC_SHIFT:
		regmap_read(regmap, SUNXI_DAC_DAP_CTL, &reg_val);
		ucontrol->value.integer.value[0] =
			(reg_val & 0x1 << DDAP_EN) && (reg_val & 0x1 << DDAP_DRC_EN) ? 1 : 0;
		break;
	case DACHPF_SHIFT:
		regmap_read(regmap, SUNXI_DAC_DAP_CTL, &reg_val);
		ucontrol->value.integer.value[0] =
			(reg_val & 0x1 << DDAP_EN) && (reg_val & 0x1 << DDAP_HPF_EN) ? 1 : 0;
		break;
	case ADCDRC0_SHIFT:
		regmap_read(regmap, SUNXI_ADC_DAP_CTL, &reg_val);
		ucontrol->value.integer.value[0] =
			(reg_val & 0x1 << ADC_DAP0_EN) && (reg_val & 0x1 << ADC_DRC0_EN) ? 1 : 0;
		break;
	case ADCHPF0_SHIFT:
		regmap_read(regmap, SUNXI_ADC_DAP_CTL, &reg_val);
		ucontrol->value.integer.value[0] =
			(reg_val & 0x1 << ADC_DAP0_EN) && (reg_val & 0x1 << ADC_HPF0_EN) ? 1 : 0;
		break;
	case ADCDRC1_SHIFT:
		regmap_read(regmap, SUNXI_ADC_DAP_CTL, &reg_val);
		ucontrol->value.integer.value[0] =
			(reg_val & 0x1 << ADC_DAP1_EN) && (reg_val & 0x1 << ADC_DRC1_EN) ? 1 : 0;
		break;

	case ADCHPF1_SHIFT:
		regmap_read(regmap, SUNXI_ADC_DAP_CTL, &reg_val);
		ucontrol->value.integer.value[0] =
			(reg_val & 0x1 << ADC_DAP1_EN) && (reg_val & 0x1 << ADC_HPF1_EN) ? 1 : 0;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sunxi_codec_set_dap_status(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int shift = e->shift_l;

	switch (shift) {
	case DACDRC_SHIFT:
		if (ucontrol->value.integer.value[0]) {
			regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
					   0x1 << DDAP_EN | 0x1 << DDAP_DRC_EN,
					   0x1 << DDAP_EN | 0x1 << DDAP_DRC_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
					   0x1 << DDAP_DRC_EN, 0x0 << DDAP_DRC_EN);
		}
		break;
	case DACHPF_SHIFT:
		if (ucontrol->value.integer.value[0]) {
			regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
					   0x1 << DDAP_EN | 0x1 << DDAP_HPF_EN,
					   0x1 << DDAP_EN | 0x1 << DDAP_HPF_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
					   0x1 << DDAP_HPF_EN, 0x0 << DDAP_HPF_EN);
		}
		break;
	case ADCDRC0_SHIFT:
		if (ucontrol->value.integer.value[0]) {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_DAP0_EN | 0x1 << ADC_DRC0_EN,
					   0x1 << ADC_DAP0_EN | 0x1 << ADC_DRC0_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_DRC0_EN, 0x0 << ADC_DRC0_EN);
		}
		break;
	case ADCHPF0_SHIFT:
		if (ucontrol->value.integer.value[0]) {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_DAP0_EN| 0x1 << ADC_HPF0_EN,
					   0x1 << ADC_DAP0_EN | 0x1 << ADC_HPF0_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_HPF0_EN, 0x0 << ADC_HPF0_EN);
		}
		break;
	case ADCDRC1_SHIFT:
		if (ucontrol->value.integer.value[0]) {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_DAP1_EN | 0x1 << ADC_DRC1_EN,
					   0x1 << ADC_DAP1_EN | 0x1 << ADC_DRC1_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_DRC1_EN, 0x0 << ADC_DRC1_EN);
		}
		break;
	case ADCHPF1_SHIFT:
		if (ucontrol->value.integer.value[0]) {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_DAP1_EN | 0x1 << ADC_HPF1_EN,
					   0x1 << ADC_DAP1_EN | 0x1 << ADC_HPF1_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_HPF1_EN, 0x0 << ADC_HPF1_EN);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/* ADDA LOOP FUNC */
static int sunxi_codec_get_adda_loop_mode(struct snd_kcontrol *kcontrol,
					  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	unsigned int reg_val, mode_val;

	regmap_read(regmap, SUNXI_DAC_DG, &reg_val);

	mode_val = (reg_val >> ADDA_LOOP_MODE) & 0x1;

	switch (mode_val) {
	case 0:
	case 1:
		ucontrol->value.integer.value[0] = mode_val;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_codec_set_adda_loop_mode(struct snd_kcontrol *kcontrol,
					  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
	case 1:
		regmap_update_bits(regmap, SUNXI_DAC_DG, 0x1 << ADDA_LOOP_MODE,
				   ucontrol->value.integer.value[0] << ADDA_LOOP_MODE);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct snd_kcontrol_new sunxi_tx_hub_controls[] = {
	SOC_ENUM_EXT("tx hub mode", sunxi_tx_hub_mode_enum,
		     sunxi_get_tx_hub_mode, sunxi_set_tx_hub_mode),
};
static const struct snd_kcontrol_new sunxi_rx_sync_controls[] = {
	SOC_ENUM_EXT("rx sync mode", sunxi_rx_sync_mode_enum,
		     sunxi_get_rx_sync_mode, sunxi_set_rx_sync_mode),
};

static const struct snd_kcontrol_new sunxi_codec_controls[] = {
	/* DAP func */
	SOC_ENUM_EXT("DAC DRC Mode", sunxi_codec_dacdrc_mode_enum, sunxi_codec_get_dap_status,
		     sunxi_codec_set_dap_status),
	SOC_ENUM_EXT("DAC HPF Mode", sunxi_codec_dachpf_mode_enum, sunxi_codec_get_dap_status,
		     sunxi_codec_set_dap_status),
	SOC_ENUM_EXT("ADC DRC0 Mode", sunxi_codec_adcdrc0_mode_enum,
		     sunxi_codec_get_dap_status, sunxi_codec_set_dap_status),
	SOC_ENUM_EXT("ADC HPF0 Mode", sunxi_codec_adchpf0_mode_enum, sunxi_codec_get_dap_status,
		     sunxi_codec_set_dap_status),
	SOC_ENUM_EXT("ADC DRC1 Mode", sunxi_codec_adcdrc1_mode_enum, sunxi_codec_get_dap_status,
		     sunxi_codec_set_dap_status),
	SOC_ENUM_EXT("ADC HPF1 Mode", sunxi_codec_adchpf1_mode_enum, sunxi_codec_get_dap_status,
		     sunxi_codec_set_dap_status),

	/* ADDA Loop */
	SOC_ENUM_EXT("ADDA Loop Mode", sunxi_adda_loop_mode_enum, sunxi_codec_get_adda_loop_mode,
		     sunxi_codec_set_adda_loop_mode),

	/* ADC/DAC Swap */
	SOC_ENUM("ADC1 ADC2 swap", sunxi_codec_adc_swap_enum),

	/* Digital Volume */
	SOC_SINGLE_TLV("digital volume", SUNXI_DAC_DPC,
		       DVOL, 0x3F, 1, digital_tlv),
	/* DAC Volume */
	SOC_SINGLE_TLV("DAC volume", SUNXI_DAC_VOL_CTRL,
		       DAC_VOL_L, 0xFF, 0, dac_vol_tlv),
	/* ADC1 Volume */
	SOC_SINGLE_TLV("ADC1 volume", SUNXI_ADC_VOL_CTRL,
		       ADC1_VOL, 0xFF, 0, adc_vol_tlv),
	/* ADC2 Volume */
	SOC_SINGLE_TLV("ADC2 volume", SUNXI_ADC_VOL_CTRL,
		       ADC2_VOL, 0xFF, 0, adc_vol_tlv),

	/* Gain-Analog */

	/* ADC1 Gain */
	SOC_SINGLE_TLV("ADC1 Gain", SUNXI_ADC1_REG,
		       ADC1_PGA_GAIN_CTRL, 0x1F, 0, adc1_gain_tlv),
	/* ADC2 Gain */
	SOC_SINGLE_TLV("ADC2 Gain", SUNXI_ADC2_REG,
		       ADC2_PGA_GAIN_CTRL, 0x1F, 0, adc2_gain_tlv),

	/* LINEIN_L Gain */
	SOC_SINGLE_TLV("LINEINL gain volume", SUNXI_ADC1_REG,
		       LINEINLG, 0x1, 0, linein_gain_tlv),

	/* LINEIN_R Gain */
	SOC_SINGLE_TLV("LINEINR gain volume", SUNXI_ADC2_REG,
		       LINEINRG, 0x1, 0, linein_gain_tlv),

	/* LINEOUT Volume */
	SOC_SINGLE_TLV("LINEOUT volume", SUNXI_DAC_REG,
		       LINEOUT_VOL, 0x1F, 0, lineout_tlv),
};

/* lineout controls */
static const char * const lineout_select_text[] = {
	"DAC_SINGLE", "DAC_DIFFER",
};

/* micin controls */
static const char * const micin_select_text[] = {
	"MIC_DIFFER", "MIC_SINGLE",
};

static SOC_ENUM_SINGLE_DECL(lineoutl_output_enum, SUNXI_DAC_REG, LINEOUTLDIFFEN,
			    lineout_select_text);

static SOC_ENUM_SINGLE_DECL(mic1_input_enum, SUNXI_ADC1_REG, MIC1_SIN_EN,
			    micin_select_text);
static SOC_ENUM_SINGLE_DECL(mic2_input_enum, SUNXI_ADC2_REG, MIC2_SIN_EN,
			    micin_select_text);

static const struct snd_kcontrol_new lineoutl_output_mux =
	SOC_DAPM_ENUM("LINEOUT Output Select", lineoutl_output_enum);

static const struct snd_kcontrol_new mic1_input_mux =
	SOC_DAPM_ENUM("MIC1 Input Select", mic1_input_enum);
static const struct snd_kcontrol_new mic2_input_mux =
	SOC_DAPM_ENUM("MIC2 Input Select", mic2_input_enum);


static int sunxi_playback_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << EN_DAC, 0x1 << EN_DAC);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << EN_DAC, 0x0 << EN_DAC);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_mic1_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("event -> 0x%x\n", event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(regmap, SUNXI_ADC1_REG, 0x1 << MIC1_PGA_EN, 0x1 << MIC1_PGA_EN);
		regmap_update_bits(regmap, SUNXI_ADC1_REG, 0x1 << ADC1_EN, 0x1 << ADC1_EN);
		mutex_lock(&codec->audio_sta.acf_mutex);
		codec->audio_sta.mic1 = true;
		if (codec->audio_sta.mic2 == false) {
			regmap_update_bits(regmap, SUNXI_MICBIAS_REG,
					   0x1 << MMICBIASEN, 0x1 << MMICBIASEN);
			/* delay 80ms to avoid pop recording in begining,
			 * and adc fifo delay time must not be less than 20ms.
			 */
			msleep(80);
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
					   0x1 << EN_AD, 0x1 << EN_AD);
		}
		mutex_unlock(&codec->audio_sta.acf_mutex);
		break;
	case SND_SOC_DAPM_POST_PMD:
		mutex_lock(&codec->audio_sta.acf_mutex);
		codec->audio_sta.mic1 = false;
		if (codec->audio_sta.mic2 == false) {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
					   0x1 << EN_AD, 0x0 << EN_AD);
			regmap_update_bits(regmap, SUNXI_MICBIAS_REG,
					   0x1 << MMICBIASEN, 0x0 << MMICBIASEN);
		}
		mutex_unlock(&codec->audio_sta.acf_mutex);
		regmap_update_bits(regmap, SUNXI_ADC1_REG, 0x1 << ADC1_EN, 0x0 << ADC1_EN);
		regmap_update_bits(regmap, SUNXI_ADC1_REG, 0x1 << MIC1_PGA_EN, 0x0 << MIC1_PGA_EN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_mic2_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(regmap, SUNXI_ADC2_REG, 0x1 << MIC2_PGA_EN, 0x1 << MIC2_PGA_EN);
		regmap_update_bits(regmap, SUNXI_ADC2_REG, 0x1 << ADC2_EN, 0x1 << ADC2_EN);
		mutex_lock(&codec->audio_sta.acf_mutex);
		codec->audio_sta.mic2 = true;
		if (codec->audio_sta.mic1 == false) {
			regmap_update_bits(regmap, SUNXI_MICBIAS_REG,
					   0x1 << MMICBIASEN, 0x1 << MMICBIASEN);
			/* delay 80ms to avoid pop recording in begining,
			 * and adc fifo delay time must not be less than 20ms.
			 */
			msleep(80);
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
					   0x1 << EN_AD, 0x1 << EN_AD);
		}
		mutex_unlock(&codec->audio_sta.acf_mutex);
		break;
	case SND_SOC_DAPM_POST_PMD:
		mutex_lock(&codec->audio_sta.acf_mutex);
		codec->audio_sta.mic2 = false;
		if (codec->audio_sta.mic1 == false) {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
					   0x1 << EN_AD, 0x0 << EN_AD);
			regmap_update_bits(regmap, SUNXI_MICBIAS_REG,
					   0x1 << MMICBIASEN, 0x0 << MMICBIASEN);
		}
		mutex_unlock(&codec->audio_sta.acf_mutex);
		regmap_update_bits(regmap, SUNXI_ADC2_REG, 0x1 << ADC2_EN, 0x0 << ADC2_EN);
		regmap_update_bits(regmap, SUNXI_ADC2_REG, 0x1 << MIC2_PGA_EN, 0x0 << MIC2_PGA_EN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_lineinl_event(struct snd_soc_dapm_widget *w,
				    struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("event -> 0x%x\n", event);

	/* note: use micin single mode instead of linein */
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* set single mode */
		regmap_update_bits(regmap, SUNXI_ADC1_REG, 0x1 << MIC1_SIN_EN, 0x1 << MIC1_SIN_EN);
		/* enable */
		regmap_update_bits(regmap, SUNXI_ADC1_REG, 0x1 << LINEINLEN, 0x1 << LINEINLEN);
		regmap_update_bits(regmap, SUNXI_ADC1_REG, 0x1 << ADC1_EN, 0x1 << ADC1_EN);
		mutex_lock(&codec->audio_sta.acf_mutex);
		codec->audio_sta.linel = true;
		if (codec->audio_sta.liner == false) {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
					   0x1 << EN_AD, 0x1 << EN_AD);
		}
		mutex_unlock(&codec->audio_sta.acf_mutex);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		mutex_lock(&codec->audio_sta.acf_mutex);
		codec->audio_sta.linel = false;
		if (codec->audio_sta.liner == false) {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
					   0x1 << EN_AD, 0x0 << EN_AD);
		}
		mutex_unlock(&codec->audio_sta.acf_mutex);
		regmap_update_bits(regmap, SUNXI_ADC1_REG, 0x1 << ADC1_EN, 0x0 << ADC1_EN);
		/* disable lineinl */
		regmap_update_bits(regmap, SUNXI_ADC1_REG, 0x1 << LINEINLEN, 0x0 << LINEINLEN);
		/* set diff mode */
		regmap_update_bits(regmap, SUNXI_ADC1_REG, 0x1 << MIC1_SIN_EN, 0x0 << MIC1_SIN_EN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_lineinr_event(struct snd_soc_dapm_widget *w,
				    struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("event -> 0x%x\n", event);

	/* note: use micin single mode instead of linein */
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* set single mode */
		regmap_update_bits(regmap, SUNXI_ADC2_REG, 0x1 << MIC2_SIN_EN, 0x1 << MIC2_SIN_EN);
		/* enable */
		regmap_update_bits(regmap, SUNXI_ADC2_REG, 0x1 << LINEINREN, 0x1 << LINEINREN);
		regmap_update_bits(regmap, SUNXI_ADC2_REG, 0x1 << ADC2_EN, 0x1 << ADC2_EN);
		mutex_lock(&codec->audio_sta.acf_mutex);
		codec->audio_sta.liner = true;
		if (codec->audio_sta.linel == false) {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
					   0x1 << EN_AD, 0x1 << EN_AD);
		}
		mutex_unlock(&codec->audio_sta.acf_mutex);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		mutex_lock(&codec->audio_sta.acf_mutex);
		codec->audio_sta.liner = false;
		if (codec->audio_sta.linel == false) {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
					   0x1 << EN_AD, 0x0 << EN_AD);
		}
		mutex_unlock(&codec->audio_sta.acf_mutex);
		regmap_update_bits(regmap, SUNXI_ADC2_REG, 0x1 << ADC2_EN, 0x0 << ADC2_EN);
		/* disable lineinr */
		regmap_update_bits(regmap, SUNXI_ADC2_REG, 0x1 << LINEINREN, 0x0 << LINEINREN);
		/* set diff mode */
		regmap_update_bits(regmap, SUNXI_ADC2_REG, 0x1 << MIC2_SIN_EN, 0x0 << MIC2_SIN_EN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_lineout_event(struct snd_soc_dapm_widget *w,
				     struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("event -> 0x%x\n", event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << DACLMUTE, 0x1 << DACLMUTE);
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << LINEOUTLEN, 0x1 << LINEOUTLEN);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << LINEOUTLEN, 0x0 << LINEOUTLEN);
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << DACLMUTE, 0x0 << DACLMUTE);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_speaker_event(struct snd_soc_dapm_widget *w,
				     struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	SND_LOG_DEBUG("event -> 0x%x\n", event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		mutex_lock(&codec->audio_sta.apf_mutex);
		codec->audio_sta.spk = true;
		mutex_unlock(&codec->audio_sta.apf_mutex);
		snd_sunxi_pa_pin_enable(codec->pa_cfg, codec->pa_pin_max);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		snd_sunxi_pa_pin_disable(codec->pa_cfg, codec->pa_pin_max);
		mutex_lock(&codec->audio_sta.apf_mutex);
		codec->audio_sta.spk = false;
		mutex_unlock(&codec->audio_sta.apf_mutex);
		break;
	default:
		break;
	}

	return 0;
}

static const struct snd_soc_dapm_widget sunxi_codec_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_IN_E("DACL", "Playback", 0, SUNXI_DAC_REG, DACLEN, 0,
			      sunxi_playback_event,
			      SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_OUT("ADC1", "Capture", 0, SUNXI_ADC_DIG_CTRL, ADC1_CHANNEL_EN, 0),
	SND_SOC_DAPM_AIF_OUT("ADC2", "Capture", 0, SUNXI_ADC_DIG_CTRL, ADC2_CHANNEL_EN, 0),

	SND_SOC_DAPM_MUX("LINEOUT Output Select", SND_SOC_NOPM, 0, 0, &lineoutl_output_mux),

	SND_SOC_DAPM_MUX("MIC1 Input Select", SND_SOC_NOPM, 0, 0, &mic1_input_mux),
	SND_SOC_DAPM_MUX("MIC2 Input Select", SND_SOC_NOPM, 0, 0, &mic2_input_mux),

	SND_SOC_DAPM_INPUT("MIC1_PIN"),
	SND_SOC_DAPM_INPUT("MIC2_PIN"),
	SND_SOC_DAPM_INPUT("LINEINL_PIN"),
	SND_SOC_DAPM_INPUT("LINEINR_PIN"),
	SND_SOC_DAPM_OUTPUT("LINEOUTL_PIN"),

	SND_SOC_DAPM_MIC("MIC1", sunxi_codec_mic1_event),
	SND_SOC_DAPM_MIC("MIC2", sunxi_codec_mic2_event),
	SND_SOC_DAPM_LINE("LINEINL", sunxi_codec_lineinl_event),
	SND_SOC_DAPM_LINE("LINEINR", sunxi_codec_lineinr_event),
	SND_SOC_DAPM_LINE("LINEOUT", sunxi_codec_lineout_event),
	SND_SOC_DAPM_SPK("SPK", sunxi_codec_speaker_event),
};

static const struct snd_soc_dapm_route sunxi_codec_dapm_routes[] = {
	/* input route -> MIC1 MIC2 */
	{"MIC1_PIN", NULL, "MIC1"},
	{"MIC2_PIN", NULL, "MIC2"},

	{"MIC1 Input Select", "MIC_SINGLE", "MIC1_PIN"},
	{"MIC1 Input Select", "MIC_DIFFER", "MIC1_PIN"},
	{"MIC2 Input Select", "MIC_SINGLE", "MIC2_PIN"},
	{"MIC2 Input Select", "MIC_DIFFER", "MIC2_PIN"},

	{"ADC1", NULL, "MIC1 Input Select"},
	{"ADC2", NULL, "MIC2 Input Select"},

	/* input route -> LINEIN */
	{"LINEINL_PIN", NULL, "LINEINL"},
	{"LINEINR_PIN", NULL, "LINEINR"},

	{"ADC1", NULL, "LINEINL_PIN"},
	{"ADC2", NULL, "LINEINR_PIN"},

	/* output route -> LINEOUT */
	{"LINEOUT Output Select", "DAC_SINGLE", "DACL"},
	{"LINEOUT Output Select", "DAC_DIFFER", "DACL"},

	{"LINEOUTL_PIN", NULL, "LINEOUT Output Select"},
	{"LINEOUT", NULL, "LINEOUTL_PIN"},
	{"SPK", NULL, "LINEOUTL_PIN"},
};

static void sunxi_codec_init(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_dts *dts = &codec->dts;
	struct sunxi_codec_rglt *rglt = &codec->rglt;
	struct regmap *regmap = codec->mem.regmap;
	unsigned int adc_dtime_map;
	unsigned int avcc_vol_map;

	SND_LOG_DEBUG("\n");

	if (rglt->avcc_external) {
		regmap_update_bits(regmap, SUNXI_POWER_REG, 0x1 << ALDO_EN, 0x0 << ALDO_EN);
	} else {
		switch (rglt->avcc_vol) {
		case 1650000:
			avcc_vol_map = 0;
			break;
		case 1700000:
			avcc_vol_map = 1;
			break;
		case 1750000:
			avcc_vol_map = 2;
			break;
		case 1800000:
			avcc_vol_map = 3;
			break;
		case 1850000:
			avcc_vol_map = 4;
			break;
		case 1900000:
			avcc_vol_map = 5;
			break;
		case 1950000:
			avcc_vol_map = 6;
			break;
		case 2000000:
			avcc_vol_map = 7;
			break;
		default:
			avcc_vol_map = 3;
			SND_LOG_DEBUG("unsupport avcc vol, default 1.8v\n");
			break;
		}
		regmap_update_bits(regmap, SUNXI_POWER_REG, 0x7 << ALDO_OUTPUT_VOLTAGE,
				   avcc_vol_map << ALDO_OUTPUT_VOLTAGE);
		regmap_update_bits(regmap, SUNXI_POWER_REG, 0x1 << ALDO_EN, 0x1 << ALDO_EN);
	}

	regmap_update_bits(regmap, SUNXI_RAMP_REG, 0x1 << RMC_EN, 0x1 << RMC_EN);

	/* Enable DAC/ADC DAP */
	regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL, 0x1 << DDAP_EN, 0x1 << DDAP_EN);
	regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL, 0x1 << ADC_DAP0_EN, 0x1 << ADC_DAP0_EN);
	regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL, 0x1 << ADC_DAP1_EN, 0x1 << ADC_DAP1_EN);

	/* Enable ADCFDT to overcome niose at the beginning */
	regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
			   0x1 << ADCDFEN, 0x1 << ADCDFEN);
	switch (dts->adc_dtime) {
	case 5:
		adc_dtime_map = 0;
		break;
	case 10:
		adc_dtime_map = 1;
		break;
	case 20:
		adc_dtime_map = 2;
		break;
	case 30:
		adc_dtime_map = 3;
		break;
	case 0:
	default:
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x1 << ADCDFEN, 0x0 << ADCDFEN);
		break;
	}
	regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x3 << ADCFDT, adc_dtime_map << ADCFDT);

	/* Open ADC1\2 and DACL Volume Setting */
	regmap_update_bits(regmap, SUNXI_DAC_VOL_CTRL, 0x1 << DAC_VOL_SEL, 0x1 << DAC_VOL_SEL);
	regmap_update_bits(regmap, SUNXI_ADC_DIG_CTRL, 0x1 << ADC1_2_VOL_EN, 0x1 << ADC1_2_VOL_EN);

	/* Digital VOL defeult setting */
	regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x3F << DVOL, dts->dac_vol << DVOL);

	regmap_update_bits(regmap, SUNXI_DAC_VOL_CTRL,
			   0xFF << DAC_VOL_L,
			   dts->dacl_vol << DAC_VOL_L);

	regmap_update_bits(regmap, SUNXI_ADC_VOL_CTRL, 0xFF << ADC1_VOL, dts->adc1_vol << ADC1_VOL);
	regmap_update_bits(regmap, SUNXI_ADC_VOL_CTRL, 0xFF << ADC2_VOL, dts->adc2_vol << ADC2_VOL);

	/* LINEOUT VOL defeult setting */
	regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1F << LINEOUT_VOL,
			   dts->lineout_gain << LINEOUT_VOL);

	/* ADCL MIC1 gain defeult setting */
	regmap_update_bits(regmap, SUNXI_ADC1_REG, 0x1F << ADC1_PGA_GAIN_CTRL,
			   dts->adc1_gain << ADC1_PGA_GAIN_CTRL);
	/* ADCR MIC2 gain defeult setting */
	regmap_update_bits(regmap, SUNXI_ADC2_REG, 0x1F << ADC2_PGA_GAIN_CTRL,
			   dts->adc2_gain << ADC2_PGA_GAIN_CTRL);

	/* ADC IOP params default setting */
	regmap_update_bits(regmap, SUNXI_ADC1_REG, 0xFF << ADC1_IOPMIC, 0x55 << ADC1_IOPMIC);
	regmap_update_bits(regmap, SUNXI_ADC2_REG, 0xFF << ADC2_IOPMIC, 0x55 << ADC2_IOPMIC);

	/* lineout & mic1 & mic2 diff setting */
	regmap_update_bits(regmap, SUNXI_DAC_REG,
			0x01 << LINEOUTLDIFFEN, 0x01 << LINEOUTLDIFFEN);
	regmap_update_bits(regmap, SUNXI_ADC1_REG,
			0x01 << MIC1_SIN_EN, 0x00 << MIC1_SIN_EN);
	regmap_update_bits(regmap, SUNXI_ADC2_REG,
			0x01 << MIC2_SIN_EN, 0x00 << MIC2_SIN_EN);

	/* DRC & HPF config */
	snd_sunxi_dap_dacdrc(regmap);
	snd_sunxi_dap_dachpf(regmap);
	snd_sunxi_dap_adcdrc(regmap);
	snd_sunxi_dap_adchpf(regmap);

}

static int sunxi_codec_component_probe(struct snd_soc_component *component)
{
	int ret;
	struct snd_soc_dapm_context *dapm = &component->dapm;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_dts *dts = &codec->dts;
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	mutex_init(&codec->audio_sta.acf_mutex);
	mutex_init(&codec->audio_sta.apf_mutex);

	/* component kcontrols -> tx_hub */
	if (dts->tx_hub_en) {
		ret = snd_soc_add_component_controls(component, sunxi_tx_hub_controls,
						     ARRAY_SIZE(sunxi_tx_hub_controls));
		if (ret)
			SND_LOG_ERR("add tx_hub kcontrols failed\n");
	}
	/* component kcontrols -> rx_sync */
	if (dts->rx_sync_en) {
		ret = snd_soc_add_component_controls(component, sunxi_rx_sync_controls,
						     ARRAY_SIZE(sunxi_rx_sync_controls));
		if (ret)
			SND_LOG_ERR("add rx_sync kcontrols failed\n");

		dts->rx_sync_ctl = false;
		dts->rx_sync_domain = RX_SYNC_SYS_DOMAIN;
		dts->rx_sync_id = sunxi_rx_sync_probe(dts->rx_sync_domain);
		if (dts->rx_sync_id < 0) {
			SND_LOG_ERR("sunxi_rx_sync_probe failed\n");
		} else {
			SND_LOG_DEBUG("sunxi_rx_sync_probe successful. domain=%d, id=%d\n",
				      dts->rx_sync_domain, dts->rx_sync_id);
			ret = sunxi_rx_sync_register_cb(dts->rx_sync_domain, dts->rx_sync_id,
							(void *)regmap, sunxi_rx_sync_enable);
			if (ret)
				SND_LOG_ERR("callback register failed\n");
		}
	}

	/* component kcontrols -> codec */
	ret = snd_soc_add_component_controls(component, sunxi_codec_controls,
					     ARRAY_SIZE(sunxi_codec_controls));
	if (ret)
		SND_LOG_ERR("register codec kcontrols failed\n");

	/* component kcontrols -> pa */
	ret = snd_sunxi_pa_pin_probe(codec->pa_cfg, codec->pa_pin_max, component);
	if (ret)
		SND_LOG_ERR("register pa kcontrols failed\n");

	/* dapm-widget */
	ret = snd_soc_dapm_new_controls(dapm, sunxi_codec_dapm_widgets,
					ARRAY_SIZE(sunxi_codec_dapm_widgets));
	if (ret)
		SND_LOG_ERR("register codec dapm_widgets failed\n");

	/* dapm-routes */
	ret = snd_soc_dapm_add_routes(dapm, sunxi_codec_dapm_routes,
				      ARRAY_SIZE(sunxi_codec_dapm_routes));
	if (ret)
		SND_LOG_ERR("register codec dapm_routes failed\n");

	sunxi_codec_init(component);

	return 0;
}

static void sunxi_codec_component_remove(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_dts *dts = &codec->dts;
	SND_LOG_DEBUG("\n");

	mutex_destroy(&codec->audio_sta.acf_mutex);
	mutex_destroy(&codec->audio_sta.apf_mutex);

	if (dts->rx_sync_en)
		sunxi_rx_sync_unregister_cb(dts->rx_sync_domain, dts->rx_sync_id);

	snd_sunxi_pa_pin_remove(codec->pa_cfg, codec->pa_pin_max);

}

static int sunxi_codec_component_suspend(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	snd_sunxi_save_reg(regmap, &sunxi_reg_group);
	snd_sunxi_pa_pin_disable(codec->pa_cfg, codec->pa_pin_max);
	snd_sunxi_rglt_disable(&codec->rglt);
	snd_sunxi_clk_bus_disable(&codec->clk);

	return 0;
}

static int sunxi_codec_component_resume(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	int ret;

	SND_LOG_DEBUG("\n");

	snd_sunxi_rglt_enable(&codec->rglt);
	ret = snd_sunxi_clk_bus_enable(&codec->clk);
	if (ret) {
		SND_LOG_ERR("clk enable failed\n");
		return ret;
	}
	snd_sunxi_pa_pin_disable(codec->pa_cfg, codec->pa_pin_max);
	snd_sunxi_echo_reg(regmap, &sunxi_reg_group);

	return 0;
}

static struct snd_soc_component_driver sunxi_codec_component_dev = {
	.name		= DRV_NAME,
	.probe		= sunxi_codec_component_probe,
	.remove		= sunxi_codec_component_remove,
	.suspend	= sunxi_codec_component_suspend,
	.resume		= sunxi_codec_component_resume,
};

/*******************************************************************************
 * *** kernel source ***
 * @1 regmap
 * @2 clk
 * @3 regulator
 * @4 pa pin
 * @5 dts params
 * @6 sysfs debug
 ******************************************************************************/
static struct regmap_config sunxi_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_CODEC_REG_MAX,
	.cache_type = REGCACHE_NONE,
};

static int snd_sunxi_mem_init(struct platform_device *pdev, struct sunxi_codec_mem *mem)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	ret = of_address_to_resource(np, 0, &mem->res);
	if (ret) {
		SND_LOG_ERR("parse device node resource failed\n");
		ret = -EINVAL;
		goto err_of_addr_to_resource;
	}

	mem->memregion = devm_request_mem_region(&pdev->dev, mem->res.start,
						 resource_size(&mem->res), DRV_NAME);
	if (IS_ERR_OR_NULL(mem->memregion)) {
		SND_LOG_ERR("memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_request_region;
	}

	mem->membase = devm_ioremap(&pdev->dev, mem->memregion->start,
				    resource_size(mem->memregion));
	if (IS_ERR_OR_NULL(mem->membase)) {
		SND_LOG_ERR("ioremap failed\n");
		ret = -EBUSY;
		goto err_devm_ioremap;
	}

	mem->regmap = devm_regmap_init_mmio(&pdev->dev, mem->membase, &sunxi_regmap_config);
	if (IS_ERR_OR_NULL(mem->regmap)) {
		SND_LOG_ERR("regmap init failed\n");
		ret = -EINVAL;
		goto err_devm_regmap_init;
	}

	return 0;

err_devm_regmap_init:
	devm_iounmap(&pdev->dev, mem->membase);
err_devm_ioremap:
	devm_release_mem_region(&pdev->dev, mem->memregion->start, resource_size(mem->memregion));
err_devm_request_region:
err_of_addr_to_resource:
	return ret;
}

static void snd_sunxi_mem_exit(struct platform_device *pdev, struct sunxi_codec_mem *mem)
{
	SND_LOG_DEBUG("\n");

	devm_iounmap(&pdev->dev, mem->membase);
	devm_release_mem_region(&pdev->dev, mem->memregion->start, resource_size(mem->memregion));
}

static int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_codec_clk *clk)
{
	int ret = 0;

	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	/* get rst clk */
	clk->clk_rst = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR_OR_NULL(clk->clk_rst)) {
		SND_LOG_ERR("clk rst get failed\n");
		ret =  PTR_ERR(clk->clk_rst);
		goto err_get_clk_rst;
	}

	/* get bus clk */
	clk->clk_bus = of_clk_get_by_name(np, "clk_bus_audio_codec");
	if (IS_ERR_OR_NULL(clk->clk_bus)) {
		SND_LOG_ERR("clk bus get failed\n");
		ret = PTR_ERR(clk->clk_bus);
		goto err_get_clk_bus;
	}

	/* get parent clk */
	clk->clk_pll_audio_1x = of_clk_get_by_name(np, "clk_pll_audio_1x");
	if (IS_ERR_OR_NULL(clk->clk_pll_audio_1x)) {
		SND_LOG_ERR("clk_pll_audio_1x get failed\n");
		ret = PTR_ERR(clk->clk_pll_audio_1x);
		goto err_get_clk_pll_audio_1x;
	}

	/* get module clk */
	clk->clk_audio_dac = of_clk_get_by_name(np, "clk_audio_dac");
	if (IS_ERR_OR_NULL(clk->clk_audio_dac)) {
		SND_LOG_ERR("clk_audio_dac get failed\n");
		ret = PTR_ERR(clk->clk_audio_dac);
		goto err_get_clk_audio_dac;
	}
	clk->clk_audio_adc = of_clk_get_by_name(np, "clk_audio_adc");
	if (IS_ERR_OR_NULL(clk->clk_audio_adc)) {
		SND_LOG_ERR("clk_audio_adc get failed\n");
		ret = PTR_ERR(clk->clk_audio_adc);
		goto err_get_clk_audio_adc;
	}

	return 0;

err_get_clk_audio_adc:
	clk_put(clk->clk_audio_dac);
err_get_clk_audio_dac:
	clk_put(clk->clk_pll_audio_1x);
err_get_clk_pll_audio_1x:
	clk_put(clk->clk_bus);
err_get_clk_bus:
err_get_clk_rst:

	return ret;
}


static void snd_sunxi_clk_exit(struct sunxi_codec_clk *clk)
{
	SND_LOG_DEBUG("\n");

	clk_put(clk->clk_audio_adc);
	clk_put(clk->clk_audio_dac);
	clk_put(clk->clk_pll_audio_1x);
	clk_put(clk->clk_bus);

}

static int snd_sunxi_clk_bus_enable(struct sunxi_codec_clk *clk)
{
	int ret = 0;

	SND_LOG_DEBUG("\n");

	/* to avoid register modification before module load */
	reset_control_assert(clk->clk_rst);
	if (reset_control_deassert(clk->clk_rst)) {
		SND_LOG_ERR("clk_rst deassert failed\n");
		ret = -EINVAL;
		goto err_deassert_rst;
	}

	if (clk_prepare_enable(clk->clk_bus)) {
		SND_LOG_ERR("clk_bus enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_bus;
	}

	return 0;

err_enable_clk_bus:
	reset_control_assert(clk->clk_rst);
err_deassert_rst:
	return ret;
}

static int snd_sunxi_clk_enable(struct sunxi_codec_clk *clk)
{
	int ret = 0;

	SND_LOG_DEBUG("\n");

	if (clk_prepare_enable(clk->clk_pll_audio_1x)) {
		SND_LOG_ERR("clk_pll_audio_1x enable failed\n");
		ret = -EINVAL;
		goto clk_pll_audio_1x;
	}

	if (clk_prepare_enable(clk->clk_audio_dac)) {
		SND_LOG_ERR("clk_audio_dac enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_audio_dac;
	}

	if (clk_prepare_enable(clk->clk_audio_adc)) {
		SND_LOG_ERR("clk_audio_adc enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_audio_adc;
	}

	return 0;

err_enable_clk_audio_adc:
	clk_disable_unprepare(clk->clk_audio_dac);
err_enable_clk_audio_dac:
	clk_disable_unprepare(clk->clk_pll_audio_1x);
clk_pll_audio_1x:
	return ret;
}

static void snd_sunxi_clk_bus_disable(struct sunxi_codec_clk *clk)
{
	SND_LOG_DEBUG("\n");

	clk_disable_unprepare(clk->clk_bus);
	reset_control_assert(clk->clk_rst);
}

static void snd_sunxi_clk_disable(struct sunxi_codec_clk *clk)
{
	SND_LOG_DEBUG("\n");

	clk_disable_unprepare(clk->clk_audio_adc);
	clk_disable_unprepare(clk->clk_audio_dac);
	clk_disable_unprepare(clk->clk_pll_audio_1x);
}

static int snd_sunxi_clk_rate(struct sunxi_codec_clk *clk, int stream,
			      unsigned int freq_in, unsigned int freq_out)
{
	SND_LOG_DEBUG("\n");

	if (stream	== SNDRV_PCM_STREAM_PLAYBACK) {
		if (clk_set_parent(clk->clk_audio_dac, clk->clk_pll_audio_1x)) {
			SND_LOG_ERR("set dac parent clk failed\n");
			return -EINVAL;
		}

		if (clk_set_rate(clk->clk_audio_dac, freq_out)) {
			SND_LOG_ERR("set clk_audio_dac rate failed, rate: %u\n", freq_out);
			return -EINVAL;
		}
	} else {
		if (clk_set_parent(clk->clk_audio_adc, clk->clk_pll_audio_1x)) {
			SND_LOG_ERR("set adc parent clk failed\n");
			return -EINVAL;
		}

		if (clk_set_rate(clk->clk_audio_adc, freq_out)) {
			SND_LOG_ERR("set clk_audio_adc rate failed, rate: %u\n", freq_out);
			return -EINVAL;
		}
	}
	return 0;
}

static int snd_sunxi_rglt_init(struct platform_device *pdev, struct sunxi_codec_rglt *rglt)
{
	int ret = 0;
	unsigned int temp_val;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	rglt->vdd_external = of_property_read_bool(np, "vdd-external");
	ret = of_property_read_u32(np, "vdd-vol", &temp_val);
	if (ret < 0) {
		rglt->vdd_vol = 3300000;	/* default vdd voltage: 3.3v */
	} else {
		rglt->vdd_vol = temp_val;
	}

	rglt->avcc_external = of_property_read_bool(np, "avcc-external");
	ret = of_property_read_u32(np, "avcc-vol", &temp_val);
	if (ret < 0) {
		rglt->avcc_vol = 1800000;	/* default avcc voltage: 1.8v */
	} else {
		rglt->avcc_vol = temp_val;
	}

	if (rglt->vdd_external) {
		SND_LOG_DEBUG("use external vdd\n");
		rglt->vdd = regulator_get(&pdev->dev, "vdd");
		if (IS_ERR_OR_NULL(rglt->vdd)) {
			SND_LOG_DEBUG("unused external pmu\n");
		} else {
			ret = regulator_set_voltage(rglt->vdd, rglt->vdd_vol, rglt->vdd_vol);
			if (ret < 0) {
				SND_LOG_ERR("set vdd voltage failed\n");
				ret = -EFAULT;
				goto err_rglt;
			}
			ret = regulator_enable(rglt->vdd);
			if (ret < 0) {
				SND_LOG_ERR("enable vdd failed\n");
				ret = -EFAULT;
				goto err_rglt;
			}
		}
	} else {
		SND_LOG_ERR("unsupport internal cpvdd for headphone charge pump\n");
		ret = -EFAULT;
		goto err_rglt;
	}

	if (rglt->avcc_external) {
		SND_LOG_DEBUG("use external avcc\n");
		rglt->avcc = regulator_get(&pdev->dev, "avcc");
		if (IS_ERR_OR_NULL(rglt->avcc)) {
			SND_LOG_DEBUG("unused external pmu\n");
		} else {
			ret = regulator_set_voltage(rglt->avcc, rglt->avcc_vol, rglt->avcc_vol);
			if (ret < 0) {
				SND_LOG_ERR("set avcc voltage failed\n");
				ret = -EFAULT;
				goto err_rglt;
			}
			ret = regulator_enable(rglt->avcc);
			if (ret < 0) {
				SND_LOG_ERR("enable avcc failed\n");
				ret = -EFAULT;
				goto err_rglt;
			}
		}
	} else {
		SND_LOG_ERR("unsupport internal avcc\n");
		ret = -EFAULT;
		goto err_rglt;
	}

	return 0;

err_rglt:
	snd_sunxi_rglt_exit(rglt);
	return ret;
}

static void snd_sunxi_rglt_exit(struct sunxi_codec_rglt *rglt)
{
	SND_LOG_DEBUG("\n");

	if (rglt->avcc)
		if (!IS_ERR_OR_NULL(rglt->avcc)) {
			regulator_disable(rglt->avcc);
			regulator_put(rglt->avcc);
		}

	if (rglt->vdd)
		if (!IS_ERR_OR_NULL(rglt->vdd)) {
			regulator_disable(rglt->vdd);
			regulator_put(rglt->vdd);
		}
}

static int snd_sunxi_rglt_enable(struct sunxi_codec_rglt *rglt)
{
	int ret;

	SND_LOG_DEBUG("\n");

	if (rglt->vdd)
		if (!IS_ERR_OR_NULL(rglt->vdd)) {
			ret = regulator_enable(rglt->vdd);
			if (ret) {
				SND_LOG_ERR("enable vdd failed\n");
				return -1;
			}
		}

	if (rglt->avcc)
		if (!IS_ERR_OR_NULL(rglt->avcc)) {
			ret = regulator_enable(rglt->avcc);
			if (ret) {
				SND_LOG_ERR("enable avcc failed\n");
				return -1;
			}
		}

	return 0;
}

static void snd_sunxi_rglt_disable(struct sunxi_codec_rglt *rglt)
{
	SND_LOG_DEBUG("\n");

	if (rglt->avcc)
		if (!IS_ERR_OR_NULL(rglt->avcc)) {
			regulator_disable(rglt->avcc);
		}

	if (rglt->vdd)
		if (!IS_ERR_OR_NULL(rglt->vdd)) {
			regulator_disable(rglt->vdd);
		}
}

static void snd_sunxi_dts_params_init(struct platform_device *pdev, struct sunxi_codec_dts *dts)
{
	int ret = 0;
	unsigned int temp_val;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	/* tx_hub */
	dts->tx_hub_en = of_property_read_bool(np, "tx-hub-en");
	/* rx_sync */
	dts->rx_sync_en = of_property_read_bool(np, "rx-sync-en");

	ret = of_property_read_u32(np, "dac-vol", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("dac volume get failed\n");
		dts->dac_vol = 0;
	} else {
		dts->dac_vol = 63 - temp_val;
	}

	ret = of_property_read_u32(np, "dacl-vol", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("dacl volume get failed\n");
		dts->dacl_vol = 160;
	} else {
		dts->dacl_vol = temp_val;
	}

	ret = of_property_read_u32(np, "adc1-vol", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("adc1 volume get failed\n");
		dts->adc1_vol = 160;
	} else {
		dts->adc1_vol = temp_val;
	}

	ret = of_property_read_u32(np, "adc2-vol", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("adc2 volume get failed\n");
		dts->adc2_vol = 160;
	} else {
		dts->adc2_vol = temp_val;
	}

	ret = of_property_read_u32(np, "lineout-gain", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("lineout gain get failed\n");
		dts->lineout_gain = 31;
	} else {
		dts->lineout_gain = temp_val;
	}

	ret = of_property_read_u32(np, "adc1-gain", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("adc1 gain get failed\n");
		dts->adc1_gain = 31;
	} else {
		dts->adc1_gain = temp_val;
	}

	ret = of_property_read_u32(np, "adc2-gain", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("adc2 gain get failed\n");
		dts->adc2_gain = 31;
	} else {
		dts->adc2_gain = temp_val;
	}

	ret = of_property_read_u32(np, "adcdelaytime", &temp_val);
	if (ret < 0) {
		dts->adc_dtime = 0;
	} else {
		switch (temp_val) {
		case 0:
		case 5:
		case 10:
		case 20:
		case 30:
			dts->adc_dtime = temp_val;
			break;
		default:
			SND_LOG_WARN("adc delay time supoort only 0,5,10,20,30ms\n");
			dts->adc_dtime = 0;
			break;
		}
	}

	/* print volume & gain value */
	SND_LOG_DEBUG("dac_vol:%u, dacl_vol:%u\n",
		      dts->dac_vol, dts->dacl_vol);
	SND_LOG_DEBUG("adc1_vol:%u, adc2_vol:%u\n",
		      dts->adc1_vol, dts->adc2_vol);
	SND_LOG_DEBUG("lineout_gain:%u\n",
		      dts->lineout_gain);
	SND_LOG_DEBUG("adc1_gain:%u, adc2_gain:%u\n",
		      dts->adc1_gain, dts->adc2_gain);

	return;
}

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
/* sysfs debug */
static void snd_sunxi_dump_version(void *priv, char *buf, size_t *count)
{
	size_t count_tmp = 0;
	struct sunxi_codec *codec = (struct sunxi_codec *)priv;

	if (!codec) {
		SND_LOG_ERR("priv to codec failed\n");
		return;
	}
	if (codec->pdev)
		if (codec->pdev->dev.driver)
			if (codec->pdev->dev.driver->owner)
				goto module_version;
	return;

module_version:
	codec->module_version = codec->pdev->dev.driver->owner->version;
	count_tmp += sprintf(buf + count_tmp, "%s\n", codec->module_version);

	*count = count_tmp;
}

static void snd_sunxi_dump_help(void *priv, char *buf, size_t *count)
{
	size_t count_tmp = 0;

	count_tmp += sprintf(buf + count_tmp, "1. reg read : echo {num} > dump && cat dump\n");
	count_tmp += sprintf(buf + count_tmp, "num: 0(all)\n");
	count_tmp += sprintf(buf + count_tmp, "2. reg write: echo {reg} {value} > dump\n");
	count_tmp += sprintf(buf + count_tmp, "eg. echo 0x00 0xaa > dump\n");

	*count = count_tmp;
}

static int snd_sunxi_dump_show(void *priv, char *buf, size_t *count)
{
	size_t count_tmp = 0;
	struct sunxi_codec *codec = (struct sunxi_codec *)priv;
	unsigned int i = 0;
	unsigned int output_reg_val;
	struct regmap *regmap;

	if (!codec) {
		SND_LOG_ERR("priv to codec failed\n");
		return -1;
	}
	if (!codec->show_reg_all)
		return 0;
	else
		codec->show_reg_all = false;

	regmap = codec->mem.regmap;
	for (i = 0; i < ARRAY_SIZE(sunxi_reg_labels); ++i) {
		regmap_read(regmap, sunxi_reg_labels[i].address, &output_reg_val);
		count_tmp += sprintf(buf + count_tmp, "[0x%03x]: 0x%8x\n",
				     sunxi_reg_labels[i].address, output_reg_val);
	}

	*count = count_tmp;

	return 0;
}

static int snd_sunxi_dump_store(void *priv, const char *buf, size_t count)
{
	struct sunxi_codec *codec = (struct sunxi_codec *)priv;
	int scanf_cnt;
	unsigned int input_reg_offset, input_reg_val, output_reg_val;
	struct regmap *regmap;

	if (count <= 1)	/* null or only "\n" */
		return 0;
	if (!codec) {
		SND_LOG_ERR("priv to codec failed\n");
		return -1;
	}
	regmap = codec->mem.regmap;

	if (!strcmp(buf, "0\n")) {
		codec->show_reg_all = true;
		return 0;
	}

	scanf_cnt = sscanf(buf, "0x%x 0x%x", &input_reg_offset, &input_reg_val);
	if (scanf_cnt != 2) {
		pr_err("wrong format: %s\n", buf);
		return -1;
	}
	if (input_reg_offset > SUNXI_CODEC_REG_MAX) {
		pr_err("reg offset > audio max reg[0x%x]\n", SUNXI_CODEC_REG_MAX);
		return -1;
	}
	regmap_read(regmap, input_reg_offset, &output_reg_val);
	pr_info("reg[0x%03x]: 0x%x (old)\n", input_reg_offset, output_reg_val);
	regmap_write(regmap, input_reg_offset, input_reg_val);
	regmap_read(regmap, input_reg_offset, &output_reg_val);
	pr_info("reg[0x%03x]: 0x%x (new)\n", input_reg_offset, output_reg_val);

	return 0;
}
#endif

static int sunxi_codec_dev_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_codec *codec;
	struct sunxi_codec_mem *mem;
	struct sunxi_codec_clk *clk;
	struct sunxi_codec_rglt *rglt;
	struct sunxi_codec_dts *dts;
#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	struct snd_sunxi_dump *dump;
#endif

	SND_LOG_DEBUG("\n");

	/* sunxi codec info */
	codec = devm_kzalloc(dev, sizeof(*codec), GFP_KERNEL);
	if (!codec) {
		SND_LOG_ERR("can't allocate sunxi codec memory\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}
	dev_set_drvdata(dev, codec);
	mem = &codec->mem;
	clk = &codec->clk;
	rglt = &codec->rglt;
	dts = &codec->dts;
#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	dump = &codec->dump;
#endif
	codec->pdev = pdev;

	/* memio init */
	ret = snd_sunxi_mem_init(pdev, mem);
	if (ret) {
		SND_LOG_ERR("mem init failed\n");
		ret = -ENOMEM;
		goto err_mem_init;
	}

	/* clk init */
	ret = snd_sunxi_clk_init(pdev, clk);
	if (ret) {
		SND_LOG_ERR("clk init failed\n");
		ret = -ENOMEM;
		goto err_clk_init;
	}

	/* clk_bus and clk_rst enable */
	ret = snd_sunxi_clk_bus_enable(clk);
	if (ret) {
		SND_LOG_ERR("clk_bus and clk_rst enable failed\n");
		ret = -EINVAL;
		goto err_clk_bus_enable;
	}

	/* regulator init */
	ret = snd_sunxi_rglt_init(pdev, rglt);
	if (ret) {
		SND_LOG_ERR("regulator init failed\n");
		ret = -EINVAL;
		goto err_regulator_init;
	}

	/* dts_params init */
	snd_sunxi_dts_params_init(pdev, dts);

	/* pa_pin init */
	codec->pa_cfg = snd_sunxi_pa_pin_init(pdev, &codec->pa_pin_max);

	/* alsa component register */
	ret = snd_soc_register_component(dev, &sunxi_codec_component_dev,
					 &sunxi_codec_dai, 1);
	if (ret) {
		SND_LOG_ERR("internal-codec component register failed\n");
		ret = -ENOMEM;
		goto err_register_component;
	}

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	snprintf(codec->module_name, 32, "%s", "AudioCodec");
	dump->name = codec->module_name;
	dump->priv = codec;
	dump->dump_version = snd_sunxi_dump_version;
	dump->dump_help = snd_sunxi_dump_help;
	dump->dump_show = snd_sunxi_dump_show;
	dump->dump_store = snd_sunxi_dump_store;
	ret = snd_sunxi_dump_register(dump);
	if (ret)
		SND_LOG_WARN("snd_sunxi_dump_register failed\n");
#endif

	SND_LOG_DEBUG("register internal-codec codec success\n");

	return 0;

err_register_component:
	snd_sunxi_rglt_exit(rglt);
err_regulator_init:
err_clk_bus_enable:
	snd_sunxi_clk_exit(clk);
err_clk_init:
	snd_sunxi_mem_exit(pdev, mem);
err_mem_init:
	devm_kfree(dev, codec);
err_devm_kzalloc:
	of_node_put(np);
	return ret;
}

static int sunxi_codec_dev_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_codec *codec = dev_get_drvdata(dev);
	struct sunxi_codec_mem *mem = &codec->mem;
	struct sunxi_codec_clk *clk = &codec->clk;
	struct sunxi_codec_rglt *rglt = &codec->rglt;
	struct sunxi_codec_dts *dts = &codec->dts;

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	struct snd_sunxi_dump *dump = &codec->dump;
#endif

	SND_LOG_DEBUG("\n");

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	snd_sunxi_dump_unregister(dump);
#endif
	if (dts->rx_sync_en)
		sunxi_rx_sync_remove(dts->rx_sync_domain);

	snd_soc_unregister_component(dev);

	snd_sunxi_mem_exit(pdev, mem);
	snd_sunxi_clk_bus_disable(clk);
	snd_sunxi_clk_exit(clk);
	snd_sunxi_rglt_exit(rglt);
	snd_sunxi_pa_pin_exit(codec->pa_cfg, codec->pa_pin_max);

	devm_kfree(dev, codec);
	of_node_put(pdev->dev.of_node);

	SND_LOG_DEBUG("unregister internal-codec codec success\n");

	return 0;
}

static const struct of_device_id sunxi_codec_of_match[] = {
	{ .compatible = "allwinner," DRV_NAME, },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_codec_of_match);

static struct platform_driver sunxi_codec_driver = {
	.driver	= {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= sunxi_codec_of_match,
	},
	.probe	= sunxi_codec_dev_probe,
	.remove	= sunxi_codec_dev_remove,
};

int __init sunxi_codec_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_codec_driver);
	if (ret != 0) {
		SND_LOG_ERR("platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_codec_dev_exit(void)
{
	platform_driver_unregister(&sunxi_codec_driver);
}

late_initcall(sunxi_codec_dev_init);
module_exit(sunxi_codec_dev_exit);

MODULE_AUTHOR("xudongpdc@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.3");
MODULE_DESCRIPTION("sunxi soundcard codec of internal-codec");
