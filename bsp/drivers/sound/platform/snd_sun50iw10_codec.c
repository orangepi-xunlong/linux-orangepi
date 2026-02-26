// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2022, Dby <dby@allwinnertech.com>
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
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/regmap.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <sound/tlv.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>

#include "snd_sunxi_pcm.h"
#include "snd_sunxi_common.h"
#include "snd_sunxi_rxsync.h"
#include "snd_sunxi_jack.h"
#include "snd_sunxi_dap.h"
#include "snd_sun50iw10_codec.h"

#define DRV_NAME	"sunxi-snd-codec"

static struct audio_reg_label sunxi_reg_labels[] = {
	REG_LABEL(SUNXI_DAC_DPC),
	REG_LABEL(SUNXI_DAC_VOL_CTRL),
	REG_LABEL(SUNXI_DAC_FIFOC),
	REG_LABEL(SUNXI_DAC_FIFOS),
	/* REG_LABEL(SUNXI_DAC_TXDATA), */
	REG_LABEL(SUNXI_DAC_CNT),
	REG_LABEL(SUNXI_DAC_DG),
	REG_LABEL(SUNXI_ADC_FIFOC),
	REG_LABEL(SUNXI_ADC_VOL_CTRL),
	REG_LABEL(SUNXI_ADC_FIFOS),
	/* REG_LABEL(SUNXI_ADC_RXDATA), */
	REG_LABEL(SUNXI_ADC_CNT),
	REG_LABEL(SUNXI_ADC_DG),
	REG_LABEL(SUNXI_DAC_DAP_CTL),
	REG_LABEL(SUNXI_ADC_DAP_CTL),
	REG_LABEL(SUNXI_AC_VERSION),
	REG_LABEL(SUNXI_ADCL_REG),
	REG_LABEL(SUNXI_ADCR_REG),
	REG_LABEL(SUNXI_DAC_REG),
	REG_LABEL(SUNXI_MICBIAS_REG),
	REG_LABEL(SUNXI_BIAS_REG),
	REG_LABEL(SUNXI_HEADPHONE_REG),
	REG_LABEL(SUNXI_HMIC_CTRL),
	REG_LABEL(SUNXI_HMIC_STS),
};
static struct audio_reg_group sunxi_reg_group = REG_GROUP(sunxi_reg_labels);

struct sample_rate {
	unsigned int samplerate;
	unsigned int rate_bit;
};
static const struct sample_rate sunxi_sample_rate_conv[] = {
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

static int sunxi_codec_dai_set_pll(struct snd_soc_dai *dai, int pll_id, int source,
				   unsigned int freq_in, unsigned int freq_out)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_clk *clk = &codec->clk;

	SND_LOG_DEBUG("stream -> %s, freq_in ->%u, freq_out ->%u\n",
		      pll_id ? "IN" : "OUT", freq_in, freq_out);

	return snd_sunxi_clk_rate(clk, pll_id, freq_in, freq_out);
}

static int sunxi_codec_dai_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *params,
				     struct snd_soc_dai *dai)
{
	int i;
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		goto playback;
	else
		goto capture;

playback:
	/* set bits */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
				   3 << FIFO_MODE, 3 << FIFO_MODE);
		regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
				   1 << TX_SAMPLE_BITS, 0 << TX_SAMPLE_BITS);
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
				   3 << FIFO_MODE, 0 << FIFO_MODE);
		regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
				   1 << TX_SAMPLE_BITS, 1 << TX_SAMPLE_BITS);
		break;
	default:
		SND_LOG_ERR("unsupport format\n");
		return -EINVAL;
	}

	/* set rate */
	for (i = 0; i < ARRAY_SIZE(sunxi_sample_rate_conv); i++)
		if (sunxi_sample_rate_conv[i].samplerate == params_rate(params))
			break;
	regmap_update_bits(regmap, SUNXI_DAC_FIFOC, 0x7 << DAC_FS,
			   sunxi_sample_rate_conv[i].rate_bit << DAC_FS);

	/* set channels */
	switch (params_channels(params)) {
	case 1:
		regmap_update_bits(regmap, SUNXI_DAC_FIFOC, 1 << DAC_MONO_EN, 1 << DAC_MONO_EN);
		break;
	case 2:
		regmap_update_bits(regmap, SUNXI_DAC_FIFOC, 1 << DAC_MONO_EN, 0 << DAC_MONO_EN);
		break;
	default:
		SND_LOG_ERR("unsupport channel\n");
		return -EINVAL;
	}

	/* enable clk after set clk rate */
	if (snd_sunxi_clk_enable(&codec->clk)) {
		SND_LOG_ERR("clk enable failed\n");
		return -EINVAL;
	} else {
		codec->clk_sta = SND_SUNXI_CLK_OPEN;
	}

	return 0;

capture:
	/* set bits */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   1 << RX_FIFO_MODE, 1 << RX_FIFO_MODE);
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   1 << RX_SAMPLE_BITS, 0 << RX_SAMPLE_BITS);
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   1 << RX_FIFO_MODE, 0 << RX_FIFO_MODE);
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   1 << RX_SAMPLE_BITS, 1 << RX_SAMPLE_BITS);
		break;
	default:
		SND_LOG_ERR("unsupport format\n");
		return -EINVAL;
	}

	/* set rate */
	for (i = 0; i < ARRAY_SIZE(sunxi_sample_rate_conv); i++)
		if (sunxi_sample_rate_conv[i].samplerate == params_rate(params))
			break;
	regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x7 << ADC_FS,
			   sunxi_sample_rate_conv[i].rate_bit << ADC_FS);

	/* set channels */
	/* HACK: nothing todo */

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

static int sunxi_codec_dai_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
				   1 << DAC_FIFO_FLUSH, 1 << DAC_FIFO_FLUSH);
		regmap_write(regmap, SUNXI_DAC_FIFOS,
			     1 << DAC_TXE_INT | 1 << DAC_TXU_INT | 1 << DAC_TXO_INT);
		regmap_write(regmap, SUNXI_DAC_CNT, 0);
	} else {
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   1 << ADC_FIFO_FLUSH, 1 << ADC_FIFO_FLUSH);
		regmap_write(regmap, SUNXI_ADC_FIFOS,
			     1 << ADC_RXA_INT | 1 << ADC_RXO_INT);
		regmap_write(regmap, SUNXI_ADC_CNT, 0);
	}

	return 0;
}

static int sunxi_codec_dai_trigger(struct snd_pcm_substream *substream, int cmd,
				   struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("cmd -> %d\n", cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
					   1 << DAC_DRQ_EN, 1 << DAC_DRQ_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
					   1 << ADC_DRQ_EN, 1 << ADC_DRQ_EN);
		}
	break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
					   1 << DAC_DRQ_EN, 0 << DAC_DRQ_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
					   1 << ADC_DRQ_EN, 0 << ADC_DRQ_EN);
		}
	break;
	default:
		SND_LOG_ERR("unsupport cmd\n");
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
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
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
		/* TODO: diable tx route */

		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << DAC_HUB_EN, 0x0 << DAC_HUB_EN);
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << DAC_HUB_EN, 0x1 << DAC_HUB_EN);

		/* TODO: enable tx route */
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void sunxi_rx_sync_enable(void *data, bool enable)
{
	struct sunxi_codec *codec = data;
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("%s\n", enable ? "on" : "off");

	if (enable)
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   1 << RX_SYNC_EN_START, 1 << RX_SYNC_EN_START);
	else
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   1 << RX_SYNC_EN_START, 0 << RX_SYNC_EN_START);

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
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 1 << RX_SYNC_EN, 0 << RX_SYNC_EN);
		sunxi_rx_sync_shutdown(dts->rx_sync_domain, dts->rx_sync_id);
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 1 << RX_SYNC_EN, 1 << RX_SYNC_EN);
		dts->rx_sync_ctl = 1;
		sunxi_rx_sync_startup(dts->rx_sync_domain, dts->rx_sync_id);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const char *sunxi_switch_text[] = {"Off", "On"};
static const char *sunxi_differ_text[] = {"single", "differ"};
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_tx_hub_mode_enum, sunxi_switch_text);
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_rx_sync_mode_enum, sunxi_switch_text);
static const struct snd_kcontrol_new sunxi_tx_hub_controls[] = {
	SOC_ENUM_EXT("tx hub mode", sunxi_tx_hub_mode_enum,
		     sunxi_get_tx_hub_mode, sunxi_set_tx_hub_mode),
};
static const struct snd_kcontrol_new sunxi_rx_sync_controls[] = {
	SOC_ENUM_EXT("rx sync mode", sunxi_rx_sync_mode_enum,
		     sunxi_get_rx_sync_mode, sunxi_set_rx_sync_mode),
};

static SOC_ENUM_SINGLE_DECL(sunxi_dacdrc_sta_enum, SND_SOC_NOPM, DACDRC_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_dachpf_sta_enum, SND_SOC_NOPM, DACHPF_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_adcdrc_sta_enum, SND_SOC_NOPM, ADCDRC_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_adchpf_sta_enum, SND_SOC_NOPM, ADCHPF_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_dac_swap_enum, SUNXI_DAC_DG, DA_SWP, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_adc_swap_enum, SUNXI_ADC_DG, AD_SWP, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_lineoutl_enum, SUNXI_DAC_REG, LINEOUTLDIFFEN, sunxi_differ_text);

static const DECLARE_TLV_DB_SCALE(digital_tlv, -7424, 116, 0);
static const DECLARE_TLV_DB_SCALE(dac_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(adc_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(mic_vol_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(hp_vol_tlv, -4200, 600, 0);
static const unsigned int lineout_vol_tlv[] = {
	TLV_DB_RANGE_HEAD(2),
	0, 1, TLV_DB_SCALE_ITEM(0, 0, 1),
	2, 31, TLV_DB_SCALE_ITEM(-4350, 150, 1),
};

/* DAC&ADC-DRC&HPF FUNC */
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
			(reg_val & 0x1 << DAC_DAP_EN) && (reg_val & 0x1 << DDAP_DRC_EN) ? 1 : 0;
		break;
	case DACHPF_SHIFT:
		regmap_read(regmap, SUNXI_DAC_DAP_CTL, &reg_val);
		ucontrol->value.integer.value[0] =
			(reg_val & 0x1 << DAC_DAP_EN) && (reg_val & 0x1 << DDAP_HPF_EN) ? 1 : 0;
		break;
	case ADCDRC_SHIFT:
		regmap_read(regmap, SUNXI_ADC_DAP_CTL, &reg_val);
		ucontrol->value.integer.value[0] =
			(reg_val & 0x1 << ADC_DAP_EN) && (reg_val & 0x1 << ADAP_DRC_EN) ? 1 : 0;
		break;
	case ADCHPF_SHIFT:
		regmap_read(regmap, SUNXI_ADC_DAP_CTL, &reg_val);
		ucontrol->value.integer.value[0] =
			(reg_val & 0x1 << ADC_DAP_EN) && (reg_val & 0x1 << ADAP_HPF_EN) ? 1 : 0;
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
					   0x1 << DAC_DAP_EN | 0x1 << DDAP_DRC_EN,
					   0x1 << DAC_DAP_EN | 0x1 << DDAP_DRC_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
					   0x1 << DDAP_DRC_EN, 0x0 << DDAP_DRC_EN);
		}
		break;
	case DACHPF_SHIFT:
		if (ucontrol->value.integer.value[0]) {
			regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
					   0x1 << DAC_DAP_EN | 0x1 << DDAP_HPF_EN,
					   0x1 << DAC_DAP_EN | 0x1 << DDAP_HPF_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
					   0x1 << DDAP_HPF_EN, 0x0 << DDAP_HPF_EN);
		}
		break;
	case ADCDRC_SHIFT:
		if (ucontrol->value.integer.value[0]) {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_DAP_EN | 0x1 << ADAP_DRC_EN,
					   0x1 << ADC_DAP_EN | 0x1 << ADAP_DRC_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADAP_DRC_EN, 0x0 << ADAP_DRC_EN);
		}
		break;
	case ADCHPF_SHIFT:
		if (ucontrol->value.integer.value[0]) {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_DAP_EN | 0x1 << ADAP_HPF_EN,
					   0x1 << ADC_DAP_EN | 0x1 << ADAP_HPF_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADAP_HPF_EN, 0x0 << ADAP_HPF_EN);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}


struct snd_kcontrol_new sunxi_codec_controls[] = {
	/* DAP func */
	SOC_ENUM_EXT("DAC DRC Switch", sunxi_dacdrc_sta_enum,
		     sunxi_codec_get_dap_status, sunxi_codec_set_dap_status),
	SOC_ENUM_EXT("DAC HPF Switch", sunxi_dachpf_sta_enum,
		     sunxi_codec_get_dap_status, sunxi_codec_set_dap_status),
	SOC_ENUM_EXT("ADC DRC Switch", sunxi_adcdrc_sta_enum,
		     sunxi_codec_get_dap_status, sunxi_codec_set_dap_status),
	SOC_ENUM_EXT("ADC HPF Switch", sunxi_adchpf_sta_enum,
		     sunxi_codec_get_dap_status, sunxi_codec_set_dap_status),

	/* Chanel swap */
	SOC_ENUM("ADC Swap", sunxi_adc_swap_enum),
	SOC_ENUM("DAC Swap", sunxi_dac_swap_enum),

	/* other func */
	SOC_ENUM("LINEOUT Output Select", sunxi_lineoutl_enum),
	SOC_SINGLE("Loop ADDA", SUNXI_DAC_DG, ADDA_LOOP_MODE, 1, 0),

	/* Volume set */
	SOC_SINGLE_TLV("ADC1 volume", SUNXI_ADC_VOL_CTRL, ADC_VOL_L, 0xFF, 0, adc_vol_tlv),
	SOC_SINGLE_TLV("ADC2 volume", SUNXI_ADC_VOL_CTRL, ADC_VOL_R, 0xFF, 0, adc_vol_tlv),
	SOC_SINGLE_TLV("DAC digital volume", SUNXI_DAC_DPC, DVOL, 0x3F, 1, digital_tlv),
	SOC_SINGLE_TLV("DACL volume", SUNXI_DAC_VOL_CTRL, DAC_VOL_L, 0xFF, 0, dac_vol_tlv),
	SOC_SINGLE_TLV("DACR volume", SUNXI_DAC_VOL_CTRL, DAC_VOL_R, 0xFF, 0, dac_vol_tlv),
	SOC_SINGLE_TLV("MIC1 volume", SUNXI_ADCL_REG, ADCL_PGA_GAIN_CTRL, 0x1F, 0, mic_vol_tlv),
	SOC_SINGLE_TLV("MIC2 volume", SUNXI_ADCR_REG, ADCR_PGA_GAIN_CTRL, 0x1F, 0, mic_vol_tlv),
	SOC_SINGLE_TLV("LINEOUT volume", SUNXI_DAC_REG, LINEOUT_VOL, 0x1F, 0, lineout_vol_tlv),
	SOC_SINGLE_TLV("HPOUT volume", SUNXI_DAC_REG, HEADPHONE_GAIN, 0x7, 1, hp_vol_tlv),
};

static int sunxi_mic1_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(regmap, SUNXI_ADCL_REG, 1 << MIC1AMPEN, 1 << MIC1AMPEN);
		regmap_update_bits(regmap, SUNXI_ADCL_REG, 1 << ADCL_EN, 1 << ADCL_EN);
		mutex_lock(&codec->audio_sta.acf_mutex);
		codec->audio_sta.mic1 = true;
		if (!codec->audio_sta.mic2) {
			regmap_update_bits(regmap, SUNXI_MICBIAS_REG,
					   1 << MMICBIASEN, 1 << MMICBIASEN);
			/* delay 80ms to avoid pop recording in begining,
			 * and adc fifo delay time must not be less than 20ms.
			 */
			msleep(80);
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 1 << EN_AD, 1 << EN_AD);
		}
		mutex_unlock(&codec->audio_sta.acf_mutex);
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 1 << ADCL_CHAN_EN, 1 << ADCL_CHAN_EN);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 1 << ADCL_CHAN_EN, 0 << ADCL_CHAN_EN);
		mutex_lock(&codec->audio_sta.acf_mutex);
		codec->audio_sta.mic1 = false;
		if (!codec->audio_sta.mic2) {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 1 << EN_AD, 0 << EN_AD);
			regmap_update_bits(regmap, SUNXI_MICBIAS_REG,
					   1 << MMICBIASEN, 0 << MMICBIASEN);
		}
		mutex_unlock(&codec->audio_sta.acf_mutex);
		regmap_update_bits(regmap, SUNXI_ADCL_REG, 1 << ADCL_EN, 0 << ADCL_EN);
		regmap_update_bits(regmap, SUNXI_ADCL_REG, 1 << MIC1AMPEN, 0 << MIC1AMPEN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_mic2_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(regmap, SUNXI_ADCR_REG, 1 << MIC2AMPEN, 1 << MIC2AMPEN);
		regmap_update_bits(regmap, SUNXI_ADCR_REG, 1 << ADCR_EN, 1 << ADCR_EN);
		mutex_lock(&codec->audio_sta.acf_mutex);
		codec->audio_sta.mic2 = true;
		if (!codec->audio_sta.mic1) {
			regmap_update_bits(regmap, SUNXI_MICBIAS_REG,
					   1 << MMICBIASEN, 1 << MMICBIASEN);
			/* delay 80ms to avoid pop recording in begining,
			 * and adc fifo delay time must not be less than 20ms.
			 */
			msleep(80);
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 1 << EN_AD, 1 << EN_AD);
		}
		mutex_unlock(&codec->audio_sta.acf_mutex);
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 1 << ADCR_CHAN_EN, 1 << ADCR_CHAN_EN);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 1 << ADCR_CHAN_EN, 0 << ADCR_CHAN_EN);
		mutex_lock(&codec->audio_sta.acf_mutex);
		codec->audio_sta.mic2 = false;
		if (!codec->audio_sta.mic1) {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 1 << EN_AD, 0 << EN_AD);
			regmap_update_bits(regmap, SUNXI_MICBIAS_REG,
					   1 << MMICBIASEN, 0 << MMICBIASEN);
		}
		mutex_unlock(&codec->audio_sta.acf_mutex);
		regmap_update_bits(regmap, SUNXI_ADCR_REG, 1 << ADCR_EN, 0 << ADCR_EN);
		regmap_update_bits(regmap, SUNXI_ADCR_REG, 1 << MIC2AMPEN, 0 << MIC2AMPEN);
		break;
	default:
		break;
	}

	return 0;
}

static void sunxi_hpout_global_switch(struct sunxi_codec *codec, bool enable)
{
	struct regmap *regmap = codec->mem.regmap;

	if (enable)
		regmap_update_bits(regmap, SUNXI_HEADPHONE_REG, 1 << HPPA_EN, 1 << HPPA_EN);
	else
		regmap_update_bits(regmap, SUNXI_HEADPHONE_REG, 1 << HPPA_EN, 0 << HPPA_EN);
}

static void sunxi_spk_global_switch(struct sunxi_codec *codec, bool enable)
{
	if (enable)
		snd_sunxi_pa_pin_enable(codec->pa_cfg, codec->pa_pin_max);
	else
		snd_sunxi_pa_pin_disable(codec->pa_cfg, codec->pa_pin_max);
}

static int sunxi_lineout_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		mutex_lock(&codec->audio_sta.apf_mutex);
		codec->audio_sta.lineout = true;
		if (!codec->audio_sta.hpout) {
			/* time delay to wait digital dac work fine */
			regmap_update_bits(regmap, SUNXI_DAC_DPC, 1 << EN_DAC, 1 << EN_DAC);
			msleep(10);

			/* lineout form DACL */
			regmap_update_bits(regmap, SUNXI_DAC_REG, 1 << DACLEN, 1 << DACLEN);
		}
		mutex_unlock(&codec->audio_sta.apf_mutex);

		regmap_update_bits(regmap, SUNXI_DAC_REG, 1 << DACLMUTE, 1 << DACLMUTE);
		regmap_update_bits(regmap, SUNXI_DAC_REG, 1 << LINEOUTLEN, 1 << LINEOUTLEN);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		mutex_lock(&codec->audio_sta.apf_mutex);
		codec->audio_sta.lineout = false;
		if (!codec->audio_sta.hpout) {
			regmap_update_bits(regmap, SUNXI_DAC_DPC, 1 << EN_DAC, 0 << EN_DAC);
			/* lineout form DACL */
			regmap_update_bits(regmap, SUNXI_DAC_REG, 1 << DACLEN, 0 << DACLEN);
		}
		mutex_unlock(&codec->audio_sta.apf_mutex);

		regmap_update_bits(regmap, SUNXI_DAC_REG, 1 << LINEOUTLEN, 0 << LINEOUTLEN);
		regmap_update_bits(regmap, SUNXI_DAC_REG, 1 << DACLMUTE, 0 << DACLMUTE);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_hpout_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		mutex_lock(&codec->audio_sta.apf_mutex);
		codec->audio_sta.hpout = true;
		if (!codec->audio_sta.lineout) {
			/* time delay to wait digital dac work fine */
			regmap_update_bits(regmap, SUNXI_DAC_DPC, 1 << EN_DAC, 1 << EN_DAC);
			msleep(10);

			/* hpout form DACL & DACR */
			regmap_update_bits(regmap, SUNXI_DAC_REG, 1 << DACLEN, 1 << DACLEN);
		}
		mutex_unlock(&codec->audio_sta.apf_mutex);
		regmap_update_bits(regmap, SUNXI_DAC_REG, 1 << DACREN, 1 << DACREN);

		regmap_update_bits(regmap, SUNXI_HEADPHONE_REG, 1 << HPINPUTEN, 1 << HPINPUTEN);
		regmap_update_bits(regmap, SUNXI_HEADPHONE_REG, 1 << HPOUTPUTEN, 1 << HPOUTPUTEN);
		sunxi_hpout_global_switch(codec, true);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		mutex_lock(&codec->audio_sta.apf_mutex);
		codec->audio_sta.hpout = false;
		if (!codec->audio_sta.lineout) {
			regmap_update_bits(regmap, SUNXI_DAC_DPC, 1 << EN_DAC, 0 << EN_DAC);
			/* hpout form DACL & DACR */
			regmap_update_bits(regmap, SUNXI_DAC_REG, 1 << DACLEN, 0 << DACLEN);
		}
		mutex_unlock(&codec->audio_sta.apf_mutex);

		regmap_update_bits(regmap, SUNXI_DAC_REG, 1 << DACREN, 0 << DACREN);

		sunxi_hpout_global_switch(codec, false);
		regmap_update_bits(regmap, SUNXI_HEADPHONE_REG, 1 << HPOUTPUTEN, 0 << HPOUTPUTEN);
		regmap_update_bits(regmap, SUNXI_HEADPHONE_REG, 1 << HPINPUTEN, 0 << HPINPUTEN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_spk_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	SND_LOG_ERR("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		mutex_lock(&codec->audio_sta.apf_mutex);
		codec->audio_sta.spk = true;
		mutex_unlock(&codec->audio_sta.apf_mutex);
		sunxi_spk_global_switch(codec, true);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		sunxi_spk_global_switch(codec, false);
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
	SND_SOC_DAPM_AIF_OUT("ADC1", "Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("ADC2", "Capture", 0, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_AIF_IN("DACL", "Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("DACR", "Playback", 0, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_INPUT("MIC1_PIN"),
	SND_SOC_DAPM_INPUT("MIC2_PIN"),
	SND_SOC_DAPM_OUTPUT("LINEOUTL_PIN"),
	SND_SOC_DAPM_OUTPUT("HPOUTL_PIN"),
	SND_SOC_DAPM_OUTPUT("HPOUTR_PIN"),

	SND_SOC_DAPM_MIC("MIC1", sunxi_mic1_event),
	SND_SOC_DAPM_MIC("MIC2", sunxi_mic2_event),
	SND_SOC_DAPM_LINE("LINEOUT", sunxi_lineout_event),
	SND_SOC_DAPM_HP("HPOUT", sunxi_hpout_event),
	SND_SOC_DAPM_SPK("SPK", sunxi_spk_event),
};

static const struct snd_soc_dapm_route sunxi_codec_dapm_routes[] = {
	/* input route -> MIC1 MIC2 */
	{"MIC1_PIN", NULL, "MIC1"},
	{"MIC2_PIN", NULL, "MIC2"},

	{"ADC1", NULL, "MIC1_PIN"},
	{"ADC2", NULL, "MIC2_PIN"},

	/* output route -> LINEOUT */
	{"LINEOUTL_PIN", NULL, "DACL"},
	{"LINEOUT", NULL, "LINEOUTL_PIN"},

	/* output route -> HPOUT */
	{"HPOUTL_PIN", NULL, "DACL"},
	{"HPOUTR_PIN", NULL, "DACR"},

	{"HPOUT", NULL, "HPOUTL_PIN"},
	{"HPOUT", NULL, "HPOUTR_PIN"},

	/* output route -> SPK */
	{"SPK", NULL, "HPOUTL_PIN"},
	{"SPK", NULL, "HPOUTR_PIN"},

	{"SPK", NULL, "LINEOUTL_PIN"},
};

/* jack work -> codec */
static int sunxi_jack_codec_init(void *data);
static void sunxi_jack_codec_exit(void *data);
static int sunxi_jack_codec_suspend(void *data);
static int sunxi_jack_codec_resume(void *data);
static void sunxi_jack_codec_irq_clean(void *data);
static void sunxi_jack_codec_det_irq_work(void *data, enum snd_jack_types *jack_type);
static void sunxi_jack_codec_det_scan_work(void *data, enum snd_jack_types *jack_type);

static void sunxi_plug_process_check_work(struct work_struct *work);
static void sunxi_plug_process_check_again_work(struct work_struct *work);

/* *** note1 ***
 * topic: half-insert
 * background: Since LINEOUT has only one channel, a two-channel speaker is connected to HPOUT,
 * that is, when the speaker is output, HPOUT outputs data at the same time.
 * When the speaker is outputting, the earphone is inserted,
 * and there is sound during the period from inserting to reporting the inserting.
 * After inserting, because the application layer closes all outputs and then opens HPOUT output,
 * there is a sense of stuttering.
 */

/* *** note2 ***
 * topic: slow-insert
 * background: Due to the low quality of the earphone socket,
 * it is easy to trigger the insertion event when the earphone is not fully inserted.
 * During the earphone recognition process, the basedata used to detect the key will be set.
 * If the insertion speed is too slow, the basedata value will not match the actual value,
 * and eventually the key will fail.
 */
struct sunxi_jack_codec sunxi_jack_codec = {
	.jack_init	= sunxi_jack_codec_init,
	.jack_exit	= sunxi_jack_codec_exit,
	.jack_suspend	= sunxi_jack_codec_suspend,
	.jack_resume	= sunxi_jack_codec_resume,

	.jack_irq_clean		= sunxi_jack_codec_irq_clean,
	.jack_det_irq_work	= sunxi_jack_codec_det_irq_work,
	.jack_det_scan_work	= sunxi_jack_codec_det_scan_work,
};

static int sunxi_jack_codec_init(void *data)
{
	struct sunxi_jack_codec_priv *jack_codec_priv = data;
	struct regmap *regmap = jack_codec_priv->regmap;
	unsigned int det_debouce_time_map;

	SND_LOG_DEBUG("\n");

	/* hp & mic det */
	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0xffff << 0, 0x0 << 0);
	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0x1f << MDATA_THRESHOLD,
			   jack_codec_priv->det_threshold << MDATA_THRESHOLD);
	regmap_update_bits(regmap, SUNXI_HMIC_STS, 0xffff << 0, 0x6000 << 0);
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0xff << SELDETADCBF, 0x40 << SELDETADCBF);
	if (jack_codec_priv->det_level == JACK_DETECT_LOW) {
		regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << AUTOPLEN, 0x1 << AUTOPLEN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << DETMODE, 0x0 << DETMODE);
	} else {
		regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << AUTOPLEN, 0x0 << AUTOPLEN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << DETMODE, 0x1 << DETMODE);
	}
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << JACKDETEN, 0x1 << JACKDETEN);
	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0x1 << JACK_IN_IRQ_EN, 0x1 << JACK_IN_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0x1 << JACK_OUT_IRQ_EN, 0x1 << JACK_OUT_IRQ_EN);

	det_debouce_time_map = jack_codec_priv->det_debouce_time / 125;
	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0xf << HMIC_N, det_debouce_time_map << HMIC_N);

	/* detect jack not fully inserted to turn off jack and spk playback. */
	INIT_DELAYED_WORK(&jack_codec_priv->plug_process_check_work, sunxi_plug_process_check_work);
	INIT_DELAYED_WORK(&jack_codec_priv->plug_process_check_again_work,
			  sunxi_plug_process_check_again_work);
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << HMICBIASEN, 0x1 << HMICBIASEN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << MICADCEN, 0x1 << MICADCEN);
	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0x1 << MIC_DET_IRQ_EN, 0x1 << MIC_DET_IRQ_EN);

	jack_codec_priv->irq_sta = JACK_IRQ_NULL;
	jack_codec_priv->irq_time = JACK_IRQ_NORMAL;

	return 0;
}

static void sunxi_jack_codec_exit(void *data)
{
	struct sunxi_jack_codec_priv *jack_codec_priv = data;
	struct regmap *regmap = jack_codec_priv->regmap;

	SND_LOG_DEBUG("\n");

	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0x1 << JACK_IN_IRQ_EN, 0x0 << JACK_IN_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0x1 << JACK_OUT_IRQ_EN, 0x0 << JACK_OUT_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0x1 << MIC_DET_IRQ_EN, 0x0 << MIC_DET_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << JACKDETEN, 0x0 << JACKDETEN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << MICADCEN, 0x0 << MICADCEN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << HMICBIASEN, 0x0 << HMICBIASEN);

	cancel_delayed_work_sync(&jack_codec_priv->plug_process_check_work);
	cancel_delayed_work_sync(&jack_codec_priv->plug_process_check_again_work);

	return;
}

static int sunxi_jack_codec_suspend(void *data)
{
	struct sunxi_jack_codec_priv *jack_codec_priv = data;
	struct regmap *regmap = jack_codec_priv->regmap;

	SND_LOG_DEBUG("\n");

	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0x1 << JACK_IN_IRQ_EN, 0x0 << JACK_IN_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0x1 << JACK_OUT_IRQ_EN, 0x0 << JACK_OUT_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0x1 << MIC_DET_IRQ_EN, 0x0 << MIC_DET_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << JACKDETEN, 0x0 << JACKDETEN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << MICADCEN, 0x0 << MICADCEN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << HMICBIASEN, 0x0 << HMICBIASEN);

	return 0;
}

static int sunxi_jack_codec_resume(void *data)
{
	struct sunxi_jack_codec_priv *jack_codec_priv = data;
	struct regmap *regmap = jack_codec_priv->regmap;

	SND_LOG_DEBUG("\n");

	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << JACKDETEN, 0x1 << JACKDETEN);
	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0x1 << JACK_IN_IRQ_EN, 0x1 << JACK_IN_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0x1 << JACK_OUT_IRQ_EN, 0x1 << JACK_OUT_IRQ_EN);

	return 0;
}

static void sunxi_jack_codec_irq_clean(void *data)
{
	unsigned int reg_val;
	unsigned int jack_state;
	struct sunxi_jack_codec_priv *jack_codec_priv = data;
	struct regmap *regmap = jack_codec_priv->regmap;
	struct sunxi_codec *codec;

	codec = container_of(jack_codec_priv, struct sunxi_codec, jack_codec_priv);

	SND_LOG_DEBUG("\n");

	regmap_read(regmap, SUNXI_HMIC_STS, &jack_state);

	/* jack in */
	if (jack_state & (1 << JACK_DET_IIN_ST)) {
		regmap_read(regmap, SUNXI_HMIC_STS, &reg_val);
		reg_val |= 0x1 << JACK_DET_IIN_ST;
		reg_val &= ~(0x1 << JACK_DET_OUT_ST);
		regmap_write(regmap, SUNXI_HMIC_STS, reg_val);

		jack_codec_priv->irq_sta = JACK_IRQ_IN;
		/* half-insert: The jack is plugged, status updated. */
		jack_codec_priv->insert_sta = JACK_INSERT_IN;
		schedule_delayed_work(&jack_codec_priv->plug_process_check_work,
				      msecs_to_jiffies(300));
	}

	/* jack out */
	if (jack_state & (1 << JACK_DET_OUT_ST)) {
		regmap_read(regmap, SUNXI_HMIC_STS, &reg_val);
		reg_val &= ~(0x1 << JACK_DET_IIN_ST);
		reg_val |= 0x1 << JACK_DET_OUT_ST;
		regmap_write(regmap, SUNXI_HMIC_STS, reg_val);

		jack_codec_priv->irq_sta = JACK_IRQ_OUT;
		/* half-insert: The jack is unplugged, status updated. */
		jack_codec_priv->insert_sta = JACK_INSERT_OUT;
		schedule_delayed_work(&jack_codec_priv->plug_process_check_work,
				      msecs_to_jiffies(300));
	}

	/* jack mic change */
	if (jack_state & (1 << MIC_DET_ST)) {
		if (jack_codec_priv->jack_type == 0) {
			/* half-insert: This state is the half-inserted state of
			 * the headphone. Turn off the IRQ ADC to facilitate subsequent manual
			 * trigger interrupts to continuously detect the half-inserted
			 * state (re-opening IRQ and ADC will trigger the interrupt again).
			 */
			regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0x1 << MIC_DET_IRQ_EN, 0x0 << MIC_DET_IRQ_EN);
			regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << MICADCEN, 0x0 << MICADCEN);

			regmap_read(regmap, SUNXI_HMIC_STS, &reg_val);
			reg_val &= ~(0x1 << JACK_DET_IIN_ST);
			reg_val &= ~(0x1 << JACK_DET_OUT_ST);
			reg_val |= 0x1 << MIC_DET_ST;
			regmap_write(regmap, SUNXI_HMIC_STS, reg_val);

			/* half-insert: The jack is plug ing, status updated. another,
			 * all playback is turned off to optimize the user's listening experience
			 * (for the case where the SPK signal source is consistent with HPOUT).
			 */
			SND_LOG_DEBUG("close hpout & spk\n");
			sunxi_spk_global_switch(codec, false);
			sunxi_hpout_global_switch(codec, false);
			jack_codec_priv->insert_sta = JACK_INSERT_ING;
			schedule_delayed_work(&jack_codec_priv->plug_process_check_work,
					      msecs_to_jiffies(300));
		} else {
			regmap_read(regmap, SUNXI_HMIC_STS, &reg_val);
			reg_val &= ~(0x1 << JACK_DET_IIN_ST);
			reg_val &= ~(0x1 << JACK_DET_OUT_ST);
			reg_val |= 0x1 << MIC_DET_ST;
			regmap_write(regmap, SUNXI_HMIC_STS, reg_val);
		}

		jack_codec_priv->irq_sta = JACK_IRQ_MIC;
	}

	/* During scan mode, interrupts are not handled */
	if (jack_codec_priv->irq_time == JACK_IRQ_SCAN)
		jack_codec_priv->irq_sta = JACK_IRQ_NULL;

	return;
}

static void sunxi_plug_process_check_again_work(struct work_struct *work)
{
	struct sunxi_jack_codec_priv *jack_codec_priv;
	struct sunxi_codec *codec;
	struct regmap *regmap;

	jack_codec_priv = container_of(work, struct sunxi_jack_codec_priv,
				       plug_process_check_again_work.work);
	codec = container_of(jack_codec_priv, struct sunxi_codec, jack_codec_priv);
	regmap = jack_codec_priv->regmap;

	SND_LOG_DEBUG("\n");

	switch (jack_codec_priv->insert_sta) {
	case JACK_INSERT_ING:
		/* half-insert: jack insert ing, keep hpout & spk off. */
	break;
	case JACK_INSERT_OUT:
		/* half-insert: jack out, open hpout if use, open spk if use. */
		mutex_lock(&codec->audio_sta.apf_mutex);
		if (codec->audio_sta.hpout) {
			SND_LOG_DEBUG("open hpout\n");
			sunxi_hpout_global_switch(codec, true);
		}
		if (codec->audio_sta.spk) {
			SND_LOG_DEBUG("open spk\n");
			sunxi_spk_global_switch(codec, true);
		}
		mutex_unlock(&codec->audio_sta.apf_mutex);
	break;
	case JACK_INSERT_IN:
		/* half-insert: jack in, open hpout if use, close spk. */
		SND_LOG_DEBUG("close spk\n");
		sunxi_spk_global_switch(codec, false);
		mutex_lock(&codec->audio_sta.apf_mutex);
		if (codec->audio_sta.hpout) {
			SND_LOG_DEBUG("open hpout\n");
			sunxi_hpout_global_switch(codec, true);
		}
		mutex_unlock(&codec->audio_sta.apf_mutex);
	break;
	case JACK_INSERT_EXIT:
		/* half-insert: jack insert exit, open hpout if use, open spk if use and jack out. */
		if (codec->audio_sta.hpout) {
			SND_LOG_DEBUG("open hpout\n");
			sunxi_hpout_global_switch(codec, true);
		}
		if (codec->audio_sta.spk && jack_codec_priv->jack_type == 0) {
			SND_LOG_DEBUG("open spk\n");
			sunxi_spk_global_switch(codec, true);
		}
	break;
	default:
		SND_LOG_ERR("jack insert status invaild\n");
	break;
	}
}

static void sunxi_plug_process_check_work(struct work_struct *work)
{
	struct sunxi_jack_codec_priv *jack_codec_priv;
	struct sunxi_codec *codec;
	struct regmap *regmap;

	jack_codec_priv = container_of(work, struct sunxi_jack_codec_priv,
				       plug_process_check_work.work);
	codec = container_of(jack_codec_priv, struct sunxi_codec, jack_codec_priv);
	regmap = jack_codec_priv->regmap;

	SND_LOG_DEBUG("\n");

	if (jack_codec_priv->insert_sta == JACK_INSERT_ING) {
		/* half-insert: reopen mic det irq, and assume insert exit. if jack inserting,
		 * mic det irq will be triggered again, and set insert_sta to inserting.
		 */
		jack_codec_priv->insert_sta = JACK_INSERT_EXIT;
		regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0x1 << MIC_DET_IRQ_EN,
							    0x1 << MIC_DET_IRQ_EN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << MICADCEN, 0x1 << MICADCEN);
	}

	/* half-insert: update output based on insert status. */
	schedule_delayed_work(&jack_codec_priv->plug_process_check_again_work,
			      msecs_to_jiffies(100));
}

static void sunxi_jack_codec_det_irq_work(void *data, enum snd_jack_types *jack_type)
{
	struct sunxi_jack_codec_priv *jack_codec_priv = data;
	struct regmap *regmap = jack_codec_priv->regmap;
	int64_t tmp_val;
	unsigned int reg_val;
	unsigned int headset_basedata;
	static struct timespec64 tv_mic;
	static struct timespec64 tv_headset_plugin;
	struct sunxi_codec *codec;

	codec = container_of(jack_codec_priv, struct sunxi_codec, jack_codec_priv);

	SND_LOG_DEBUG("\n");

	switch (jack_codec_priv->irq_sta) {
	case JACK_IRQ_OUT:
		SND_LOG_DEBUG("jack out\n");
		regmap_update_bits(regmap, SUNXI_HMIC_CTRL,
				   0x1 << JACK_IN_IRQ_EN, 0x1 << JACK_IN_IRQ_EN);
		regmap_update_bits(regmap, SUNXI_HMIC_CTRL,
				   0x1 << JACK_OUT_IRQ_EN, 0x0 << JACK_OUT_IRQ_EN);

		/* jack unplugged, check mic */
		regmap_update_bits(regmap, SUNXI_HMIC_CTRL,
				   0x1 << MIC_DET_IRQ_EN, 0x1 << MIC_DET_IRQ_EN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_REG,
				   0x1 << MICADCEN, 0x1 << MICADCEN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_REG,
				   0x1 << HMICBIASEN, 0x1 << HMICBIASEN);

		*jack_type = 0;
	break;
	case JACK_IRQ_IN:
		SND_LOG_DEBUG("jack in\n");
		regmap_update_bits(regmap, SUNXI_HMIC_CTRL,
				   0x1 << JACK_IN_IRQ_EN, 0x0 << JACK_IN_IRQ_EN);
		regmap_update_bits(regmap, SUNXI_HMIC_CTRL,
				   0x1 << JACK_OUT_IRQ_EN, 0x1 << JACK_OUT_IRQ_EN);

		regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << HMICBIASEN, 0x1 << HMICBIASEN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << MICADCEN, 0x1 << MICADCEN);
		msleep(500);
		regmap_read(regmap, SUNXI_HMIC_STS, &reg_val);
		headset_basedata = (reg_val >> HMIC_DATA) & 0x1f;
		if (headset_basedata > jack_codec_priv->det_threshold) {
			/* jack -> hp & mic */
			*jack_type = SND_JACK_HEADSET;

			/* get headset jack plugin */
			ktime_get_real_ts64(&tv_headset_plugin);

			/* slow-insert: save basedata */
			jack_codec_priv->hp_det_basedata = headset_basedata;

			if (headset_basedata > 3)
				headset_basedata -= 3;

			SND_LOG_DEBUG("jack headset_basedata -> %u\n", headset_basedata);
			regmap_update_bits(regmap, SUNXI_HMIC_CTRL,
					   0x1f << MDATA_THRESHOLD,
					   headset_basedata << MDATA_THRESHOLD);
			regmap_update_bits(regmap, SUNXI_HMIC_CTRL,
					   0x1 << MIC_DET_IRQ_EN, 0x1 << MIC_DET_IRQ_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_HMIC_CTRL,
					   0x1 << MIC_DET_IRQ_EN, 0x0 << MIC_DET_IRQ_EN);
			regmap_update_bits(regmap, SUNXI_MICBIAS_REG,
					   0x1 << MICADCEN, 0x0 << MICADCEN);
			regmap_update_bits(regmap, SUNXI_MICBIAS_REG,
					   0x1 << HMICBIASEN, 0x0 << HMICBIASEN);
			/* jack -> hp */
			*jack_type = SND_JACK_HEADPHONE;
		}
	break;
	case JACK_IRQ_MIC:
		SND_LOG_DEBUG("jack button\n");
		/* jack plugout or plugin process,
		 * no need to check mic, but need turn off playback */
		if (*jack_type != SND_JACK_HEADSET) {
			SND_LOG_DEBUG("invalid jack type, unneed button process\n");
			break;
		}

		/* slow-insert: confirm basedata again */
		regmap_read(regmap, SUNXI_HMIC_STS, &reg_val);
		reg_val = (reg_val & 0x1f00) >> 8;
		tmp_val = (int64_t)reg_val - (int64_t)jack_codec_priv->hp_det_basedata;
		if (reg_val > jack_codec_priv->det_threshold && (tmp_val >= 3 || tmp_val <= -3)) {
			reg_val -= 3;
			regmap_update_bits(regmap, SUNXI_HMIC_CTRL,
					   0x1f << MDATA_THRESHOLD,
					   reg_val << MDATA_THRESHOLD);
		}

		/* Prevent accidental triggering of buttons when the headset is just plugged in */
		ktime_get_real_ts64(&tv_mic);
		if (abs(tv_mic.tv_sec - tv_headset_plugin.tv_sec) < 2)
			break;

		regmap_read(regmap, SUNXI_HMIC_STS, &reg_val);
		reg_val = (reg_val & 0x1f00) >> 8;

		/* SND_JACK_BTN_0 - key-hook
		 * SND_JACK_BTN_1 - key-up
		 * SND_JACK_BTN_2 - key-down
		 * SND_JACK_BTN_3 - key-voice
		 */
		if (reg_val >= jack_codec_priv->key_det_vol[0][0] &&
		    reg_val <= jack_codec_priv->key_det_vol[0][1]) {
			*jack_type |= SND_JACK_BTN_0;
		} else if (reg_val >= jack_codec_priv->key_det_vol[1][0] &&
			   reg_val <= jack_codec_priv->key_det_vol[1][1]) {
			*jack_type |= SND_JACK_BTN_1;
		} else if (reg_val >= jack_codec_priv->key_det_vol[2][0] &&
			   reg_val <= jack_codec_priv->key_det_vol[2][1]) {
			*jack_type |= SND_JACK_BTN_2;
		} else if (reg_val >= jack_codec_priv->key_det_vol[3][0] &&
			   reg_val <= jack_codec_priv->key_det_vol[3][1]) {
			*jack_type |= SND_JACK_BTN_3;
		} else {
			SND_LOG_DEBUG("unsupport jack button\n");
		}
	break;
	default:
		SND_LOG_DEBUG("irq status is invaild\n");
	break;
	}

	jack_codec_priv->jack_type = *jack_type;

	return;
}

static void sunxi_jack_codec_det_scan_work(void *data, enum snd_jack_types *jack_type)
{
	struct sunxi_jack_codec_priv *jack_codec_priv = data;
	struct regmap *regmap = jack_codec_priv->regmap;

	SND_LOG_DEBUG("\n");

	/* Change the detection mode to achieve the purpose of manually triggering the interrupt */
	jack_codec_priv->irq_time = JACK_IRQ_SCAN;
	if (jack_codec_priv->det_level == JACK_DETECT_LOW) {
		regmap_update_bits(regmap, SUNXI_MICBIAS_REG,
				   0x1 << AUTOPLEN, 0x0 << AUTOPLEN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << DETMODE, 0x1 << DETMODE);
	} else {
		regmap_update_bits(regmap, SUNXI_MICBIAS_REG,
				   0x1 << AUTOPLEN, 0x1 << AUTOPLEN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << DETMODE, 0x0 << DETMODE);
	}

	SND_LOG_DEBUG("sleep\n");
	msleep(500);	/* must ensure that the interrupt triggers */

	jack_codec_priv->irq_time = JACK_IRQ_NORMAL;
	if (jack_codec_priv->det_level == JACK_DETECT_LOW) {
		regmap_update_bits(regmap, SUNXI_MICBIAS_REG,
				   0x1 << AUTOPLEN, 0x1 << AUTOPLEN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << DETMODE, 0x0 << DETMODE);
	} else {
		regmap_update_bits(regmap, SUNXI_MICBIAS_REG,
				   0x1 << AUTOPLEN, 0x0 << AUTOPLEN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << DETMODE, 0x1 << DETMODE);
	}

	return;
}

/* jack work -> extcon */
static int sunxi_jack_extcon_init(void *data);
static void sunxi_jack_extcon_exit(void *data);
static int sunxi_jack_extcon_suspend(void *data);
static int sunxi_jack_extcon_resume(void *data);
static void sunxi_jack_extcon_irq_clean(void *data);
static void sunxi_jack_extcon_det_irq_work(void *data, enum snd_jack_types *jack_type);
static void sunxi_jack_extcon_det_scan_work(void *data, enum snd_jack_types *jack_type);


struct sunxi_jack_extcon sunxi_jack_extcon = {
	.jack_init	= sunxi_jack_extcon_init,
	.jack_exit	= sunxi_jack_extcon_exit,
	.jack_suspend	= sunxi_jack_extcon_suspend,
	.jack_resume	= sunxi_jack_extcon_resume,

	.jack_irq_clean		= sunxi_jack_extcon_irq_clean,
	.jack_det_irq_work	= sunxi_jack_extcon_det_irq_work,
	.jack_det_scan_work	= sunxi_jack_extcon_det_scan_work,
};

static int sunxi_jack_extcon_init(void *data)
{
	struct sunxi_jack_extcon_priv *jack_extcon_priv = data;
	struct regmap *regmap = jack_extcon_priv->regmap;
	unsigned int det_debouce_time_map;

	SND_LOG_DEBUG("\n");

	/* hp & mic det */
	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0xffff << 0, 0x0 << 0);
	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0x1f << MDATA_THRESHOLD,
			   jack_extcon_priv->det_threshold << MDATA_THRESHOLD);
	regmap_update_bits(regmap, SUNXI_HMIC_STS, 0xffff << 0, 0x6000 << 0);
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0xff << SELDETADCBF, 0x40 << SELDETADCBF);
	if (jack_extcon_priv->det_level == JACK_DETECT_LOW) {
		regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << AUTOPLEN, 0x1 << AUTOPLEN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << DETMODE, 0x0 << DETMODE);
	} else {
		regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << AUTOPLEN, 0x0 << AUTOPLEN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << DETMODE, 0x1 << DETMODE);
	}
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << JACKDETEN, 0x1 << JACKDETEN);

	det_debouce_time_map = jack_extcon_priv->det_debouce_time / 125;
	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0xf << HMIC_N, det_debouce_time_map << HMIC_N);

	/* enable when jack in */
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << HMICBIASEN, 0x0 << HMICBIASEN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << MICADCEN, 0x0 << MICADCEN);
	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0x1 << MIC_DET_IRQ_EN, 0x0 << MIC_DET_IRQ_EN);

	jack_extcon_priv->irq_sta = JACK_IRQ_NULL;

	return 0;
}

static void sunxi_jack_extcon_exit(void *data)
{
	struct sunxi_jack_extcon_priv *jack_extcon_priv = data;
	struct regmap *regmap = jack_extcon_priv->regmap;

	SND_LOG_DEBUG("\n");

	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0x1 << MIC_DET_IRQ_EN, 0x0 << MIC_DET_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << JACKDETEN, 0x0 << JACKDETEN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << MICADCEN, 0x0 << MICADCEN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << HMICBIASEN, 0x0 << HMICBIASEN);

	return;
}

static int sunxi_jack_extcon_suspend(void *data)
{
	struct sunxi_jack_extcon_priv *jack_extcon_priv = data;
	struct regmap *regmap = jack_extcon_priv->regmap;

	SND_LOG_DEBUG("\n");

	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0x1 << MIC_DET_IRQ_EN, 0x0 << MIC_DET_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << JACKDETEN, 0x0 << JACKDETEN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << MICADCEN, 0x0 << MICADCEN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << HMICBIASEN, 0x0 << HMICBIASEN);

	return 0;
}

static int sunxi_jack_extcon_resume(void *data)
{
	struct sunxi_jack_extcon_priv *jack_extcon_priv = data;
	struct regmap *regmap = jack_extcon_priv->regmap;

	SND_LOG_DEBUG("\n");

	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << JACKDETEN, 0x1 << JACKDETEN);

	return 0;
}

static void sunxi_jack_extcon_irq_clean(void *data)
{
	unsigned int reg_val;
	unsigned int jack_state;
	struct sunxi_jack_extcon_priv *jack_extcon_priv = data;
	struct regmap *regmap = jack_extcon_priv->regmap;

	SND_LOG_DEBUG("\n");

	regmap_read(regmap, SUNXI_HMIC_STS, &jack_state);

	/* jack mic change */
	if (jack_state & (1 << MIC_DET_ST)) {
		regmap_read(regmap, SUNXI_HMIC_STS, &reg_val);
		reg_val &= ~(0x1 << JACK_DET_IIN_ST);
		reg_val &= ~(0x1 << JACK_DET_OUT_ST);
		reg_val |= 0x1 << MIC_DET_ST;
		regmap_write(regmap, SUNXI_HMIC_STS, reg_val);

		jack_extcon_priv->irq_sta = JACK_IRQ_MIC;
	} else {
		SND_LOG_WARN("unsupport jack irq type\n");
	}

	return;
}

static void sunxi_jack_extcon_det_irq_work(void *data, enum snd_jack_types *jack_type)
{
	struct sunxi_jack_extcon_priv *jack_extcon_priv = data;
	struct regmap *regmap = jack_extcon_priv->regmap;
	unsigned int reg_val;
	static struct timespec64 tv_mic;

	SND_LOG_DEBUG("\n");

	if (jack_extcon_priv->irq_sta != JACK_IRQ_MIC) {
		SND_LOG_DEBUG("unsupport jack irq type\n");
		return;
	}

	SND_LOG_DEBUG("jack butonn\n");

	/* Prevent accidental triggering of buttons when the headset is just plugged in */
	ktime_get_real_ts64(&tv_mic);
	if (abs(tv_mic.tv_sec - jack_extcon_priv->tv_headset_plugin.tv_sec) < 2)
		return;

	regmap_read(regmap, SUNXI_HMIC_STS, &reg_val);
	reg_val = (reg_val & 0x1f00) >> 8;

	/* SND_JACK_BTN_0 - key-hook
	 * SND_JACK_BTN_1 - key-up
	 * SND_JACK_BTN_2 - key-down
	 * SND_JACK_BTN_3 - key-voice
	 */
	if (reg_val >= jack_extcon_priv->key_det_vol[0][0] &&
	    reg_val <= jack_extcon_priv->key_det_vol[0][1]) {
		*jack_type |= SND_JACK_BTN_0;
	} else if (reg_val >= jack_extcon_priv->key_det_vol[1][0] &&
		   reg_val <= jack_extcon_priv->key_det_vol[1][1]) {
		*jack_type |= SND_JACK_BTN_1;
	} else if (reg_val >= jack_extcon_priv->key_det_vol[2][0] &&
		   reg_val <= jack_extcon_priv->key_det_vol[2][1]) {
		*jack_type |= SND_JACK_BTN_2;
	} else if (reg_val >= jack_extcon_priv->key_det_vol[3][0] &&
		   reg_val <= jack_extcon_priv->key_det_vol[3][1]) {
		*jack_type |= SND_JACK_BTN_3;
	} else {
		SND_LOG_DEBUG("unsupport jack button\n");
	}

	return;
}

static void sunxi_jack_extcon_det_scan_work(void *data, enum snd_jack_types *jack_type)
{
	struct sunxi_jack_extcon *jack_extcon = &sunxi_jack_extcon;
	struct sunxi_jack_extcon_priv *jack_extcon_priv = data;
	struct regmap *regmap = jack_extcon_priv->regmap;
	int i;
	int count = 5;
	unsigned int reg_val;
	unsigned int headset_basedata;
	int interval_ms = 10;

	SND_LOG_DEBUG("\n");

	if (jack_extcon->jack_plug_sta == JACK_PLUG_STA_OUT) {
		*jack_type = 0;
		return;
	}

	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << HMICBIASEN, 0x1 << HMICBIASEN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << MICADCEN, 0x1 << MICADCEN);

	sunxi_jack_typec_mode_set(&jack_extcon->jack_typec_cfg, SND_JACK_MODE_MICI);
	msleep(100);
	for (i = 0; i < count; i++) {
		regmap_read(regmap, SUNXI_HMIC_STS, &reg_val);
		reg_val = (reg_val >> HMIC_DATA) & 0x1f;
		SND_LOG_DEBUG("HMIC data %u\n", reg_val);
		if (reg_val >= jack_extcon_priv->det_threshold)
			goto jack_headset;
		msleep(interval_ms);
	}

	sunxi_jack_typec_mode_set(&jack_extcon->jack_typec_cfg, SND_JACK_MODE_MICN);
	msleep(100);
	for (i = 0; i < count; i++) {
		regmap_read(regmap, SUNXI_HMIC_STS, &reg_val);
		reg_val = (reg_val >> HMIC_DATA) & 0x1f;
		SND_LOG_DEBUG("HMIC data %u\n", reg_val);
		if (reg_val >= jack_extcon_priv->det_threshold)
			goto jack_headset;
		msleep(interval_ms);
	}

	/* jack -> hp */
	*jack_type = SND_JACK_HEADPHONE;
	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0x1 << MIC_DET_IRQ_EN, 0x0 << MIC_DET_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << HMICBIASEN, 0x0 << HMICBIASEN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_REG, 0x1 << MICADCEN, 0x0 << MICADCEN);
	return;

jack_headset:
	/* jack -> hp & mic */
	*jack_type = SND_JACK_HEADSET;
	regmap_read(regmap, SUNXI_HMIC_STS, &reg_val);
	headset_basedata = (reg_val >> HMIC_DATA) & 0x1f;
	if (headset_basedata > 3)
		headset_basedata -= 3;
	SND_LOG_DEBUG("jack headset_basedata -> %u\n", headset_basedata);

	regmap_update_bits(regmap, SUNXI_HMIC_CTRL,
			   0x1f << MDATA_THRESHOLD,
			   headset_basedata << MDATA_THRESHOLD);
	ktime_get_real_ts64(&jack_extcon_priv->tv_headset_plugin);
	regmap_update_bits(regmap, SUNXI_HMIC_CTRL, 0x1 << MIC_DET_IRQ_EN, 0x1 << MIC_DET_IRQ_EN);
}

struct sunxi_jack_port sunxi_jack_port = {
       .jack_codec = &sunxi_jack_codec,
       .jack_extcon = &sunxi_jack_extcon,
};

static void sunxi_codec_init(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_dts *dts = &codec->dts;
	struct regmap *regmap = codec->mem.regmap;
	unsigned int adc_dtime_map;

	SND_LOG_DEBUG("\n");

	/* Enable DAC/ADC DAP */
	regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL, 0x1 << DAC_DAP_EN, 0x1 << DAC_DAP_EN);
	regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL, 0x1 << ADC_DAP_EN, 0x1 << ADC_DAP_EN);
	/* cpvcc default voltage: 0.9v, analog power for headphone charge pump */
	regmap_update_bits(regmap, SUNXI_DAC_REG, 1 << CPLDO_EN, 0 << CPLDO_EN);
	regmap_update_bits(regmap, SUNXI_DAC_REG, 3 << CPLDO_VOLTAGE, 0 << CPLDO_VOLTAGE);
	/* To fix some pop noise */
	regmap_update_bits(regmap, SUNXI_HEADPHONE_REG, 1 << HPCALIFIRST, 1 << HPCALIFIRST);
	regmap_update_bits(regmap, SUNXI_HEADPHONE_REG, 3 << HPPA_DEL, 3 << HPPA_DEL);
	/* ADCL/R IOP params default setting */
	regmap_update_bits(regmap, SUNXI_ADCL_REG, 0xFF << ADCL_IOPMICL, 0x55 << ADCL_IOPMICL);
	regmap_update_bits(regmap, SUNXI_ADCR_REG, 0xFF << ADCR_IOPMICL, 0x55 << ADCR_IOPMICL);
	/* For improve performance of THD+N about HP */
	regmap_update_bits(regmap, SUNXI_HEADPHONE_REG, 3 << CP_CLKS, 2 << CP_CLKS);

	/* Enable ADCFDT to overcome niose at the beginning */
	switch (dts->adc_dtime) {
	case 5:
		adc_dtime_map = 0;
		break;
	case 10:
		adc_dtime_map = 1;
		break;
	case 30:
		adc_dtime_map = 3;
		break;
	case 20:
	default:
		adc_dtime_map = 2;
		break;
	}
	if (dts->adc_dtime > 0) {
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x1 << ADCDFEN, 0x1 << ADCDFEN);
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x3 << ADCFDT, adc_dtime_map << ADCFDT);
	} else {
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x1 << ADCDFEN, 0x0 << ADCDFEN);
	}

	/* lineout & mic1 & mic2 diff or single setting */
	if (dts->lineout_single)
		regmap_update_bits(regmap, SUNXI_DAC_REG, 1 << LINEOUTLDIFFEN, 0 << LINEOUTLDIFFEN);
	else
		regmap_update_bits(regmap, SUNXI_DAC_REG, 1 << LINEOUTLDIFFEN, 1 << LINEOUTLDIFFEN);

	/* rx_sync default disable */
	regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x1 << RX_SYNC_EN, 0x0 << RX_SYNC_EN);

	/* ADC_VOL_SEL and DAC_VOL_SEL conctrl enable */
	regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 1 << ADC_VOL_SEL, 1 << ADC_VOL_SEL);
	regmap_update_bits(regmap, SUNXI_DAC_VOL_CTRL, 1 << DAC_VOL_SEL, 1 << DAC_VOL_SEL);

	regmap_update_bits(regmap, SUNXI_ADC_VOL_CTRL, 0xFF << ADC_VOL_L,
			   dts->adc_dig_vol_l << ADC_VOL_L);
	regmap_update_bits(regmap, SUNXI_ADC_VOL_CTRL, 0xFF << ADC_VOL_R,
			   dts->adc_dig_vol_r << ADC_VOL_R);
	regmap_update_bits(regmap, SUNXI_ADCL_REG, 0x1F << ADCL_PGA_GAIN_CTRL,
			   dts->mic1_vol << ADCL_PGA_GAIN_CTRL);
	regmap_update_bits(regmap, SUNXI_ADCR_REG, 0x1F << ADCR_PGA_GAIN_CTRL,
			   dts->mic2_vol << ADCR_PGA_GAIN_CTRL);
	regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x3F << DVOL,
			   dts->dac_dig_vol << DVOL);
	regmap_update_bits(regmap, SUNXI_DAC_VOL_CTRL, 0xFF << DAC_VOL_L,
			   dts->dac_dig_vol_l << DAC_VOL_L);
	regmap_update_bits(regmap, SUNXI_DAC_VOL_CTRL, 0xFF << DAC_VOL_R,
			   dts->dac_dig_vol_r << DAC_VOL_R);
	regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1F << LINEOUT_VOL,
			   dts->lineout_vol << LINEOUT_VOL);
	regmap_update_bits(regmap, SUNXI_DAC_REG, 7 << HEADPHONE_GAIN,
			   dts->hp_vol << HEADPHONE_GAIN);

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

	codec->audio_sta.mic1 = false;
	codec->audio_sta.mic2 = false;
	codec->audio_sta.hpout = false;
	codec->audio_sta.lineout = false;

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

	ret = snd_soc_add_component_controls(component, sunxi_codec_controls,
					     ARRAY_SIZE(sunxi_codec_controls));
	if (ret)
		SND_LOG_ERR("register codec kcontrols failed\n");

	/* component kcontrols -> pa */
	ret = snd_sunxi_pa_pin_probe(codec->pa_cfg, codec->pa_pin_max, component);
	if (ret)
		SND_LOG_ERR("register pa kcontrols failed\n");

	ret = snd_soc_dapm_new_controls(dapm, sunxi_codec_dapm_widgets,
					ARRAY_SIZE(sunxi_codec_dapm_widgets));
	if (ret)
		SND_LOG_ERR("register codec dapm_widgets failed\n");

	ret = snd_soc_dapm_add_routes(dapm, sunxi_codec_dapm_routes,
				      ARRAY_SIZE(sunxi_codec_dapm_routes));
	if (ret)
		SND_LOG_ERR("register codec dapm_routes failed\n");

	sunxi_codec_init(component);

	/* jack init -> codec */
	codec->jack_codec_priv.regmap = codec->mem.regmap;
	sunxi_jack_codec.pdev = codec->pdev;
	sunxi_jack_codec.data = (void *)(&codec->jack_codec_priv);
	/* jack init -> extcon */
	codec->jack_extcon_priv.regmap = codec->mem.regmap;
	sunxi_jack_extcon.pdev = codec->pdev;
	sunxi_jack_extcon.data = (void *)(&codec->jack_extcon_priv);

	snd_sunxi_jack_init(&sunxi_jack_port);

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

	snd_sunxi_jack_exit(&sunxi_jack_port);
}

static int sunxi_codec_component_suspend(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	snd_sunxi_save_reg(regmap, &sunxi_reg_group);
	sunxi_spk_global_switch(codec, false);
	snd_sunxi_rglt_disable(&codec->rglt);
	snd_sunxi_clk_bus_disable(&codec->clk);

	return 0;
}

static int sunxi_codec_component_resume(struct snd_soc_component *component)
{
	int ret;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	sunxi_spk_global_switch(codec, false);
	snd_sunxi_rglt_enable(&codec->rglt);
	ret = snd_sunxi_clk_bus_enable(&codec->clk);
	if (ret) {
		SND_LOG_ERR("clk enable failed\n");
		return ret;
	}
	snd_sunxi_echo_reg(regmap, &sunxi_reg_group);

	return 0;
}

static struct snd_soc_component_driver sunxi_codec_dev = {
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
 * @3 rglt
 * @4 pa pin
 * @5 dts params
 ******************************************************************************/
static struct regmap_config sunxi_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_AUDIO_MAX_REG,
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
						 resource_size(&mem->res),
						 DRV_NAME);
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
	clk->clk_bus_audio = of_clk_get_by_name(np, "clk_bus_audio");
	if (IS_ERR_OR_NULL(clk->clk_bus_audio)) {
		SND_LOG_ERR("clk bus get failed\n");
		ret = PTR_ERR(clk->clk_bus_audio);
		goto err_get_clk_bus;
	}

	/* get parent clk */
	clk->clk_pll_audio = of_clk_get_by_name(np, "clk_pll_audio");
	if (IS_ERR_OR_NULL(clk->clk_pll_audio)) {
		SND_LOG_ERR("clk_pll_audio get failed\n");
		ret = PTR_ERR(clk->clk_pll_audio);
		goto err_get_clk_pll_audio;
	}
	clk->clk_pll_com = of_clk_get_by_name(np, "clk_pll_com");
	if (IS_ERR_OR_NULL(clk->clk_pll_com)) {
		SND_LOG_ERR("clk_pll_com get failed\n");
		ret = PTR_ERR(clk->clk_pll_com);
		goto err_get_clk_pll_com;
	}
	clk->clk_pll_com_audio = of_clk_get_by_name(np, "clk_pll_com_audio");
	if (IS_ERR_OR_NULL(clk->clk_pll_com_audio)) {
		SND_LOG_ERR("clk_pll_com_audio get failed\n");
		ret = PTR_ERR(clk->clk_pll_com_audio);
		goto err_get_clk_pll_com_audio;
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

	if (clk_set_parent(clk->clk_pll_com_audio, clk->clk_pll_com)) {
		SND_LOG_ERR("set parent of clk_pll_com_audio to clk_pll_com failed\n");
		ret = -EINVAL;
		goto err_set_parent;
	}
	if (clk_set_parent(clk->clk_audio_dac, clk->clk_pll_audio)) {
		SND_LOG_ERR("set parent of clk_audio_dac to pll_audio failed\n");
		ret = -EINVAL;
		goto err_set_parent;
	}
	if (clk_set_parent(clk->clk_audio_adc, clk->clk_pll_audio)) {
		SND_LOG_ERR("set parent of clk_audio_adc to pll_audio failed\n");
		ret = -EINVAL;
		goto err_set_parent;
	}

	if (clk_set_rate(clk->clk_pll_audio, 98304000)) {		/* 24.576M * n */
		SND_LOG_ERR("set clk_pll_audio rate failed\n");
		ret = -EINVAL;
		goto err_set_rate;
	}
	if (clk_set_rate(clk->clk_pll_com, 451584000)) {
		SND_LOG_ERR("set clk_pll_com rate failed\n");
		ret = -EINVAL;
		goto err_set_rate;
	}
	if (clk_set_rate(clk->clk_pll_com_audio, 90316800)) {	/* 22.5792M * n */
		SND_LOG_ERR("set clk_pll_com_audio rate failed\n");
		ret = -EINVAL;
		goto err_set_rate;
	}

	return 0;

err_set_rate:
err_set_parent:
	clk_put(clk->clk_audio_adc);
err_get_clk_audio_adc:
	clk_put(clk->clk_audio_dac);
err_get_clk_audio_dac:
	clk_put(clk->clk_pll_com_audio);
err_get_clk_pll_com_audio:
	clk_put(clk->clk_pll_com);
err_get_clk_pll_com:
	clk_put(clk->clk_pll_audio);
err_get_clk_pll_audio:
	clk_put(clk->clk_bus_audio);
err_get_clk_bus:
err_get_clk_rst:
	return ret;
}

static void snd_sunxi_clk_exit(struct sunxi_codec_clk *clk)
{
	SND_LOG_DEBUG("\n");

	clk_put(clk->clk_audio_adc);
	clk_put(clk->clk_audio_dac);
	clk_put(clk->clk_pll_com_audio);
	clk_put(clk->clk_pll_com);
	clk_put(clk->clk_pll_audio);
	clk_put(clk->clk_bus_audio);
}

static int snd_sunxi_clk_enable(struct sunxi_codec_clk *clk)
{
	int ret = 0;

	SND_LOG_DEBUG("\n");

	if (clk_prepare_enable(clk->clk_pll_audio)) {
		SND_LOG_ERR("pll_audio enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_pll_audio;
	}
	if (clk_prepare_enable(clk->clk_pll_com)) {
		SND_LOG_ERR("clk_pll_com enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_pll_com;
	}
	if (clk_prepare_enable(clk->clk_pll_com_audio)) {
		SND_LOG_ERR("clk_pll_com_audio enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_pll_com_audio;
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
	clk_disable_unprepare(clk->clk_pll_com_audio);
err_enable_clk_pll_com_audio:
	clk_disable_unprepare(clk->clk_pll_com);
err_enable_clk_pll_com:
	clk_disable_unprepare(clk->clk_pll_audio);
err_enable_clk_pll_audio:
	return ret;
}

static int snd_sunxi_clk_bus_enable(struct sunxi_codec_clk *clk)
{
	int ret = 0;

	SND_LOG_DEBUG("\n");

	if (reset_control_deassert(clk->clk_rst)) {
		SND_LOG_ERR("clk rst deassert failed\n");
		ret = -EINVAL;
		goto err_deassert_rst;
	}

	if (clk_prepare_enable(clk->clk_bus_audio)) {
		SND_LOG_ERR("clk bus enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_bus;
	}

	return 0;

err_enable_clk_bus:
	reset_control_assert(clk->clk_rst);
err_deassert_rst:
	return ret;
}

static void snd_sunxi_clk_disable(struct sunxi_codec_clk *clk)
{
	SND_LOG_DEBUG("\n");

	clk_disable_unprepare(clk->clk_audio_adc);
	clk_disable_unprepare(clk->clk_audio_dac);
	clk_disable_unprepare(clk->clk_pll_com_audio);
	clk_disable_unprepare(clk->clk_pll_com);
	clk_disable_unprepare(clk->clk_pll_audio);
}

static void snd_sunxi_clk_bus_disable(struct sunxi_codec_clk *clk)
{
	SND_LOG_DEBUG("\n");

	clk_disable_unprepare(clk->clk_bus_audio);
	reset_control_assert(clk->clk_rst);
}

static int snd_sunxi_clk_rate(struct sunxi_codec_clk *clk, int stream,
			      unsigned int freq_in, unsigned int freq_out)
{
	SND_LOG_DEBUG("\n");

	if (stream  == SNDRV_PCM_STREAM_PLAYBACK) {
		if (freq_in % 24576000 == 0) {
			if (clk_set_parent(clk->clk_audio_dac, clk->clk_pll_audio)) {
				SND_LOG_ERR("set dac parent clk failed\n");
				return -EINVAL;
			}
		} else {
			if (clk_set_parent(clk->clk_audio_dac, clk->clk_pll_com_audio)) {
				SND_LOG_ERR("set dac parent clk failed\n");
				return -EINVAL;
			}
		}
		if (clk_set_rate(clk->clk_audio_dac, freq_out)) {
			SND_LOG_ERR("set clk_audio_dac rate failed, rate: %u\n", freq_out);
			return -EINVAL;
		}
	} else {
		if (freq_in % 24576000 == 0) {
			if (clk_set_parent(clk->clk_audio_adc, clk->clk_pll_audio)) {
				SND_LOG_ERR("set adc parent clk failed\n");
				return -EINVAL;
			}
		} else {
			if (clk_set_parent(clk->clk_audio_adc, clk->clk_pll_com_audio)) {
				SND_LOG_ERR("set adc parent clk failed\n");
				return -EINVAL;
			}
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

	rglt->avcc_external = of_property_read_bool(np, "avcc-external");
	ret = of_property_read_u32(np, "avcc-vol", &temp_val);
	if (ret < 0) {
		rglt->avcc_vol = 1800000;	/* default avcc voltage: 1.8v */
	} else {
		rglt->avcc_vol = temp_val;
	}

	rglt->dvcc_external = of_property_read_bool(np, "dvcc-external");
	ret = of_property_read_u32(np, "dvcc-vol", &temp_val);
	if (ret < 0) {
		rglt->dvcc_vol = 1800000;	/* default dvcc voltage: 1.8v */
	} else {
		rglt->dvcc_vol = temp_val;
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

	if (rglt->dvcc_external) {
		SND_LOG_DEBUG("use external dvcc\n");
		rglt->dvcc = regulator_get(&pdev->dev, "dvcc");
		if (IS_ERR_OR_NULL(rglt->dvcc)) {
			SND_LOG_DEBUG("unused external pmu\n");
		} else {
			ret = regulator_set_voltage(rglt->dvcc, rglt->dvcc_vol, rglt->dvcc_vol);
			if (ret < 0) {
				SND_LOG_ERR("set dvcc voltage failed\n");
				ret = -EFAULT;
				goto err_rglt;
			}
			ret = regulator_enable(rglt->dvcc);
			if (ret < 0) {
				SND_LOG_ERR("enable dvcc failed\n");
				ret = -EFAULT;
				goto err_rglt;
			}
		}
	} else {
		SND_LOG_ERR("unsupport internal cpvdd for headphone charge pump\n");
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

	if (rglt->dvcc)
		if (!IS_ERR_OR_NULL(rglt->dvcc)) {
			regulator_disable(rglt->dvcc);
			regulator_put(rglt->dvcc);
		}
}

static int snd_sunxi_rglt_enable(struct sunxi_codec_rglt *rglt)
{
	int ret;

	SND_LOG_DEBUG("\n");

	if (rglt->avcc)
		if (!IS_ERR_OR_NULL(rglt->avcc)) {
			ret = regulator_enable(rglt->avcc);
			if (ret) {
				SND_LOG_ERR("enable avcc failed\n");
				return -1;
			}
		}

	if (rglt->dvcc)
		if (!IS_ERR_OR_NULL(rglt->dvcc)) {
			ret = regulator_enable(rglt->dvcc);
			if (ret) {
				SND_LOG_ERR("enable dvcc failed\n");
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

	if (rglt->dvcc)
		if (!IS_ERR_OR_NULL(rglt->dvcc)) {
			regulator_disable(rglt->dvcc);
		}
}

static void snd_sunxi_dts_params_init(struct platform_device *pdev, struct sunxi_codec_dts *dts)
{
	int ret = 0;
	unsigned int temp_val;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_codec *codec = dev_get_drvdata(&pdev->dev);

	SND_LOG_DEBUG("\n");

	/* input volume */
	ret = of_property_read_u32(np, "adc-dig-vol-l", &temp_val);
	if (ret < 0) {
		dts->adc_dig_vol_l = 0;
	} else {
		dts->adc_dig_vol_l = temp_val;
	}
	ret = of_property_read_u32(np, "adc-dig-vol-r", &temp_val);
	if (ret < 0) {
		dts->adc_dig_vol_r = 0;
	} else {
		dts->adc_dig_vol_r = temp_val;
	}
	ret = of_property_read_u32(np, "mic1-vol", &temp_val);
	if (ret < 0) {
		dts->mic1_vol = 0;
	} else {
		dts->mic1_vol = temp_val;
	}
	ret = of_property_read_u32(np, "mic2-vol", &temp_val);
	if (ret < 0) {
		dts->mic2_vol = 0;
	} else {
		dts->mic2_vol = temp_val;
	}

	/* output volume */
	ret = of_property_read_u32(np, "dac-dig-vol", &temp_val);
	if (ret < 0) {
		dts->dac_dig_vol = 0;
	} else {
		if (temp_val > 63) {
			dts->dac_dig_vol = 0;
		} else {
			dts->dac_dig_vol = 63 - temp_val;	/* invert */
		}
	}
	ret = of_property_read_u32(np, "dac-dig-vol-l", &temp_val);
	if (ret < 0) {
		dts->dac_dig_vol_l = 0;
	} else {
		dts->dac_dig_vol_l = temp_val;
	}
	ret = of_property_read_u32(np, "dac-dig-vol-r", &temp_val);
	if (ret < 0) {
		dts->dac_dig_vol_r = 0;
	} else {
		dts->dac_dig_vol_r = temp_val;
	}
	ret = of_property_read_u32(np, "lineout-vol", &temp_val);
	if (ret < 0) {
		dts->lineout_vol = 0;
	} else {
		dts->lineout_vol = temp_val;
	}
	ret = of_property_read_u32(np, "hp-vol", &temp_val);
	if (ret < 0) {
		dts->hp_vol = 0;
	} else {
		if (temp_val > 7) {
			dts->hp_vol = 0;
		} else {
			dts->hp_vol = 7 - temp_val;		/* invert */
		}
	}

	/* codec athers param */
	ret = of_property_read_u32(np, "adc-delay-time", &temp_val);
	if (ret < 0) {
		dts->adc_dtime = 20;	/* default: ADC fifo delay 20ms */
	} else {
		dts->adc_dtime = temp_val;
	}
	dts->lineout_single = of_property_read_bool(np, "lineout-single");

	/* jack param -> codec */
	SND_LOG_DEBUG("******jack codec param******\n");
	ret = of_property_read_u32(np, "jack-det-level", &temp_val);
	if (ret < 0) {
		codec->jack_codec_priv.det_level = 0;
	} else {
		if (temp_val > 0)
			codec->jack_codec_priv.det_level = 1;
		else
			codec->jack_codec_priv.det_level = 0;
	}
	ret = of_property_read_u32(np, "jack-det-threshold", &temp_val);
	if (ret < 0 || temp_val > 0x1f) {
		codec->jack_codec_priv.det_threshold = 8; /* AW1855 default 8 */
	} else {
		codec->jack_codec_priv.det_threshold = temp_val;
	}
	ret = of_property_read_u32(np, "jack-det-debouce-time", &temp_val);
	if (ret < 0 || temp_val > (125 * (0xf + 1))) {
		codec->jack_codec_priv.det_debouce_time = 250; /* AW1855 default 250ms */
	} else {
		codec->jack_codec_priv.det_debouce_time = temp_val;
	}

	ret = of_property_read_u32_index(np, "jack-key-det-voltage-hook", 0, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-hook get failed\n");
		codec->jack_codec_priv.key_det_vol[0][0] = 0;
	} else {
		codec->jack_codec_priv.key_det_vol[0][0] = temp_val;
	}
	ret = of_property_read_u32_index(np, "jack-key-det-voltage-hook", 1, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-hook get failed\n");
		codec->jack_codec_priv.key_det_vol[0][1] = 0;
	} else {
		codec->jack_codec_priv.key_det_vol[0][1] = temp_val;
	}
	ret = of_property_read_u32_index(np, "jack-key-det-voltage-up", 0, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-up get failed\n");
		codec->jack_codec_priv.key_det_vol[1][0] = 2;
	} else {
		codec->jack_codec_priv.key_det_vol[1][0] = temp_val;
	}
	ret = of_property_read_u32_index(np, "jack-key-det-voltage-up", 1, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-up get failed\n");
		codec->jack_codec_priv.key_det_vol[1][1] = 2;
	} else {
		codec->jack_codec_priv.key_det_vol[1][1] = temp_val;
	}
	ret = of_property_read_u32_index(np, "jack-key-det-voltage-down", 0, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-down get failed\n");
		codec->jack_codec_priv.key_det_vol[2][0] = 4;
	} else {
		codec->jack_codec_priv.key_det_vol[2][0] = temp_val;
	}
	ret = of_property_read_u32_index(np, "jack-key-det-voltage-down", 1, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-down get failed\n");
		codec->jack_codec_priv.key_det_vol[2][1] = 5;
	} else {
		codec->jack_codec_priv.key_det_vol[2][1] = temp_val;
	}
	ret = of_property_read_u32_index(np, "jack-key-det-voltage-voice", 0, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-voice get failed\n");
		codec->jack_codec_priv.key_det_vol[3][0] = 1;
	} else {
		codec->jack_codec_priv.key_det_vol[3][0] = temp_val;
	}
	ret = of_property_read_u32_index(np, "jack-key-det-voltage-voice", 1, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-voice get failed\n");
		codec->jack_codec_priv.key_det_vol[3][1] = 1;
	} else {
		codec->jack_codec_priv.key_det_vol[3][1] = temp_val;
	}

	SND_LOG_DEBUG("jack-det-level        -> %u\n", codec->jack_codec_priv.det_level);
	SND_LOG_DEBUG("jack-det-threshold    -> %u\n", codec->jack_codec_priv.det_threshold);
	SND_LOG_DEBUG("jack-det-debouce-time -> %u\n",
		      codec->jack_codec_priv.det_debouce_time);
	SND_LOG_DEBUG("jack-key-det-voltage-hook   -> %u-%u\n",
		      codec->jack_codec_priv.key_det_vol[0][0],
		      codec->jack_codec_priv.key_det_vol[0][1]);
	SND_LOG_DEBUG("jack-key-det-voltage-up     -> %u-%u\n",
		      codec->jack_codec_priv.key_det_vol[1][0],
		      codec->jack_codec_priv.key_det_vol[1][1]);
	SND_LOG_DEBUG("jack-key-det-voltage-down   -> %u-%u\n",
		      codec->jack_codec_priv.key_det_vol[2][0],
		      codec->jack_codec_priv.key_det_vol[2][1]);
	SND_LOG_DEBUG("jack-key-det-voltage-voice  -> %u-%u\n",
		      codec->jack_codec_priv.key_det_vol[3][0],
		      codec->jack_codec_priv.key_det_vol[3][1]);

	/* jack param -> extcon */
	SND_LOG_DEBUG("******jack extcon param******\n");
	ret = of_property_read_u32(np, "jack-det-level", &temp_val);
	if (ret < 0) {
		codec->jack_extcon_priv.det_level = 0;
	} else {
		if (temp_val > 0)
			codec->jack_extcon_priv.det_level = 1;
		else
			codec->jack_extcon_priv.det_level = 0;
	}
	ret = of_property_read_u32(np, "jack-det-threshold", &temp_val);
	if (ret < 0 || temp_val > 0x1f) {
		codec->jack_extcon_priv.det_threshold = 8; /* AW1855 default 8 */
	} else {
		codec->jack_extcon_priv.det_threshold = temp_val;
	}
	ret = of_property_read_u32(np, "jack-det-debouce-time", &temp_val);
	if (ret < 0 || temp_val > (125 * (0xf + 1))) {
		codec->jack_extcon_priv.det_debouce_time = 250; /* AW1855 default 250ms */
	} else {
		codec->jack_extcon_priv.det_debouce_time = temp_val;
	}

	ret = of_property_read_u32_index(np, "jack-key-det-voltage-hook", 0, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-hook get failed\n");
		codec->jack_extcon_priv.key_det_vol[0][0] = 0;
	} else {
		codec->jack_extcon_priv.key_det_vol[0][0] = temp_val;
	}
	ret = of_property_read_u32_index(np, "jack-key-det-voltage-hook", 1, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-hook get failed\n");
		codec->jack_extcon_priv.key_det_vol[0][1] = 0;
	} else {
		codec->jack_extcon_priv.key_det_vol[0][1] = temp_val;
	}
	ret = of_property_read_u32_index(np, "jack-key-det-voltage-up", 0, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-up get failed\n");
		codec->jack_extcon_priv.key_det_vol[1][0] = 2;
	} else {
		codec->jack_extcon_priv.key_det_vol[1][0] = temp_val;
	}
	ret = of_property_read_u32_index(np, "jack-key-det-voltage-up", 1, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-up get failed\n");
		codec->jack_extcon_priv.key_det_vol[1][1] = 2;
	} else {
		codec->jack_extcon_priv.key_det_vol[1][1] = temp_val;
	}
	ret = of_property_read_u32_index(np, "jack-key-det-voltage-down", 0, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-down get failed\n");
		codec->jack_extcon_priv.key_det_vol[2][0] = 4;
	} else {
		codec->jack_extcon_priv.key_det_vol[2][0] = temp_val;
	}
	ret = of_property_read_u32_index(np, "jack-key-det-voltage-down", 1, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-down get failed\n");
		codec->jack_extcon_priv.key_det_vol[2][1] = 5;
	} else {
		codec->jack_extcon_priv.key_det_vol[2][1] = temp_val;
	}
	ret = of_property_read_u32_index(np, "jack-key-det-voltage-voice", 0, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-voice get failed\n");
		codec->jack_extcon_priv.key_det_vol[3][0] = 1;
	} else {
		codec->jack_extcon_priv.key_det_vol[3][0] = temp_val;
	}
	ret = of_property_read_u32_index(np, "jack-key-det-voltage-voice", 1, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-voice get failed\n");
		codec->jack_extcon_priv.key_det_vol[3][1] = 1;
	} else {
		codec->jack_extcon_priv.key_det_vol[3][1] = temp_val;
	}

	SND_LOG_DEBUG("jack-det-level        -> %u\n", codec->jack_extcon_priv.det_level);
	SND_LOG_DEBUG("jack-det-threshold    -> %u\n", codec->jack_extcon_priv.det_threshold);
	SND_LOG_DEBUG("jack-det-debouce-time -> %u\n",
		      codec->jack_extcon_priv.det_debouce_time);
	SND_LOG_DEBUG("jack-key-det-voltage-hook   -> %u-%u\n",
		      codec->jack_extcon_priv.key_det_vol[0][0],
		      codec->jack_extcon_priv.key_det_vol[0][1]);
	SND_LOG_DEBUG("jack-key-det-voltage-up     -> %u-%u\n",
		      codec->jack_extcon_priv.key_det_vol[1][0],
		      codec->jack_extcon_priv.key_det_vol[1][1]);
	SND_LOG_DEBUG("jack-key-det-voltage-down   -> %u-%u\n",
		      codec->jack_extcon_priv.key_det_vol[2][0],
		      codec->jack_extcon_priv.key_det_vol[2][1]);
	SND_LOG_DEBUG("jack-key-det-voltage-voice  -> %u-%u\n",
		      codec->jack_extcon_priv.key_det_vol[3][0],
		      codec->jack_extcon_priv.key_det_vol[3][1]);

	/* tx_hub */
	dts->tx_hub_en = of_property_read_bool(np, "tx-hub-en");

	/* rx_sync */
	dts->rx_sync_en = of_property_read_bool(np, "rx-sync-en");

	SND_LOG_DEBUG("adc-dig-vol-l -> %u\n", dts->adc_dig_vol_l);
	SND_LOG_DEBUG("adc-dig-vol-r -> %u\n", dts->adc_dig_vol_r);
	SND_LOG_DEBUG("mic1-vol      -> %u\n", dts->mic1_vol);
	SND_LOG_DEBUG("mic2-vol      -> %u\n", dts->mic2_vol);
	SND_LOG_DEBUG("dac-dig-vol   -> %u\n", dts->dac_dig_vol);
	SND_LOG_DEBUG("dac-dig-vol-l -> %u\n", dts->dac_dig_vol_l);
	SND_LOG_DEBUG("dac-dig-vol-r -> %u\n", dts->dac_dig_vol_r);
	SND_LOG_DEBUG("lineout-vol   -> %u\n", dts->lineout_vol);
	SND_LOG_DEBUG("hp-vol        -> %u\n", dts->hp_vol);
	SND_LOG_DEBUG("lineout       -> %s\n", dts->lineout_single ? "single" : "differ");
	SND_LOG_DEBUG("tx-hub-en     -> %s\n", dts->tx_hub_en ? "on" : "off");
	SND_LOG_DEBUG("rx-sync-en    -> %s\n", dts->rx_sync_en ? "on" : "off");
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
	if (input_reg_offset > SUNXI_AUDIO_MAX_REG) {
		pr_err("reg offset > audio max reg[0x%x]\n", SUNXI_AUDIO_MAX_REG);
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

	/* rglt init */
	ret = snd_sunxi_rglt_init(pdev, rglt);
	if (ret) {
		SND_LOG_ERR("rglt init failed\n");
		ret = -ENOMEM;
		goto err_rglt_init;
	}

	/* dts_params init */
	snd_sunxi_dts_params_init(pdev, dts);

	/* pa_pin init */
	codec->pa_cfg = snd_sunxi_pa_pin_init(pdev, &codec->pa_pin_max);

	/* alsa component register */
	ret = snd_soc_register_component(dev, &sunxi_codec_dev, &sunxi_codec_dai, 1);
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
err_rglt_init:
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

	/* remove components */
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
	{.compatible = "allwinner," DRV_NAME,},
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

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.8");
MODULE_DESCRIPTION("sunxi soundcard codec of internal-codec");
