// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2022, huhaoxin <huhaoxin@allwinnertech.com>
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
#include <sunxi-sid.h>

#include "snd_sunxi_pcm.h"
#include "snd_sunxi_common.h"
#include "snd_sunxi_jack.h"
#include "snd_sunxi_rxsync.h"
#include "snd_sunxi_dap.h"
#include "snd_sun65iw1_codec.h"

#define DRV_NAME	"sunxi-snd-codec"

static struct audio_reg_label sunxi_reg_labels[] = {
	REG_LABEL(SUNXI_DAC_DPC),
	REG_LABEL(SUNXI_DAC_VOL_CTL),
	REG_LABEL(SUNXI_DAC_FIFO_CTL),
	REG_LABEL(SUNXI_DAC_FIFO_STA),
	/* REG_LABEL(SUNXI_DAC_TXDATA), */
	REG_LABEL(SUNXI_DAC_CNT),
	REG_LABEL(SUNXI_DAC_DEBUG),
	REG_LABEL(SUNXI_ADC_FIFO_CTL),
	REG_LABEL(SUNXI_ADC_VOL_CTL1),
	REG_LABEL(SUNXI_ADC_FIFO_STA),
	/* REG_LABEL(SUNXI_ADC_RXDATA), */
	REG_LABEL(SUNXI_ADC_CNT),
	REG_LABEL(SUNXI_ADC_DEBUG),
	REG_LABEL(SUNXI_ADC_DIG_CTL),
	REG_LABEL(SUNXI_VAR1SPEEDUP_DOWN_CTL),
	REG_LABEL(SUNXI_AN_DEBUG1),
	REG_LABEL(SUNXI_AN_DEBUG2),
	REG_LABEL(SUNXI_DAC_DAP_CTL),
	REG_LABEL(SUNXI_ADC_DAP_CTL),
	REG_LABEL(SUNXI_DAC_DRC_CTL),
	REG_LABEL(SUNXI_ADC_DRC_CTL),
	REG_LABEL(SUNXI_VERSION),
	REG_LABEL(SUNXI_ADC1_AN_CTL),
	REG_LABEL(SUNXI_ADC2_AN_CTL),
	REG_LABEL(SUNXI_DAC_AN_CTL),
	REG_LABEL(SUNXI_DAC2_AN_CTL),
	REG_LABEL(SUNXI_MICBIAS_AN_CTL),
	REG_LABEL(SUNXI_RAMP_CTL),
	REG_LABEL(SUNXI_BIAS_AN_CTL),
	REG_LABEL(SUNXI_HP_AN_CTL),
	REG_LABEL(SUNXI_HMIC_CTL),
	REG_LABEL(SUNXI_HMIC_STA),
	REG_LABEL(SUNXI_POWER_AN_CTL),
};
static struct audio_reg_group sunxi_reg_group = REG_GROUP(sunxi_reg_labels);

struct sample_rate {
	unsigned int samplerate;
	unsigned int rate_bit;
};
static const struct sample_rate sunxi_sample_rate_conv[] = {
	{44100, 0},
	{48000, 0},
	{8000, 5},
	{32000, 1},
	{22050, 2},
	{24000, 2},
	{16000, 3},
	{11025, 4},
	{12000, 4},
	{192000, 6},
	{96000, 7},
};

static int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_codec_clk *clk);
static void snd_sunxi_clk_exit(struct sunxi_codec_clk *clk);
static int snd_sunxi_clk_enable(struct sunxi_codec_clk *clk);
static int snd_sunxi_clk_bus_enable(struct sunxi_codec_clk *clk);
static void snd_sunxi_clk_disable(struct sunxi_codec_clk *clk);
static void snd_sunxi_clk_bus_disable(struct sunxi_codec_clk *clk);
static int snd_sunxi_clk_rate(struct sunxi_codec_clk *clk, int stream,
			      unsigned int freq_in, unsigned int freq_out);

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
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	int i = 0;

	SND_LOG_DEBUG("\n");

	/* Set bits */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
					   0x3 << DAC_FIFO_MODE, 0x3 << DAC_FIFO_MODE);
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
					   0x1 << TX_SAMPLE_BITS, 0x0 << TX_SAMPLE_BITS);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
					   0x1 << RX_FIFO_MODE, 0x1 << RX_FIFO_MODE);
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
					   0x1 << RX_SAMPLE_BITS, 0x0 << RX_SAMPLE_BITS);
		}
		break;
	case SNDRV_PCM_FORMAT_S24_3LE:
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
					   0x3 << DAC_FIFO_MODE, 0x0 << DAC_FIFO_MODE);
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
					   0x1 << TX_SAMPLE_BITS, 0x1 << TX_SAMPLE_BITS);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
					   0x1 << RX_FIFO_MODE, 0x0 << RX_FIFO_MODE);
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
					   0x1 << RX_SAMPLE_BITS, 0x1 << RX_SAMPLE_BITS);
		}
		break;
	default:
		break;
	}

	/* Set rate */
	for (i = 0; i < ARRAY_SIZE(sunxi_sample_rate_conv); i++) {
		if (sunxi_sample_rate_conv[i].samplerate == params_rate(params)) {
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL, 0x7 << DAC_FS,
						   sunxi_sample_rate_conv[i].rate_bit << DAC_FS);
			} else {
				if (sunxi_sample_rate_conv[i].samplerate > 48000)
					return -EINVAL;
				regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL, 0x7 << ADC_FS,
						   sunxi_sample_rate_conv[i].rate_bit << ADC_FS);
			}
		}
	}

	/* Set channels */
	if (params_channels(params) == 1) {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
					   0x1 << DAC_MONO_EN, 0x1 << DAC_MONO_EN);
	} else {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
					   0x1 << DAC_MONO_EN, 0x0 << DAC_MONO_EN);
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

static int sunxi_codec_dai_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
				   0x1 << DAC_FIFO_FLUSH, 0x1 << DAC_FIFO_FLUSH);
		regmap_write(regmap, SUNXI_DAC_FIFO_STA,
			     0x1 << DAC_TXE_INT | 0x1 << DAC_TXU_INT | 0x1 << DAC_TXO_INT);
		regmap_write(regmap, SUNXI_DAC_CNT, 0x0);
	} else {
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
				   0x1 << ADC_FIFO_FLUSH, 0x1 << ADC_FIFO_FLUSH);
		regmap_write(regmap, SUNXI_ADC_FIFO_STA, 0x1 << ADC_RXA_INT | 0x1 << ADC_RXO_INT);
		regmap_write(regmap, SUNXI_ADC_CNT, 0x0);
	}

	return 0;
}

static void sunxi_pa_disa_cb(void *data)
{
	struct regmap *regmap = (struct regmap *)data;

	SND_LOG_DEBUG("\n");

	regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL, 0x1 << DAC_DRQ_EN, 0x0 << DAC_DRQ_EN);
}

static int sunxi_codec_dai_trigger(struct snd_pcm_substream *substream, int cmd,
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
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
					   0x1 << DAC_DRQ_EN, 0x1 << DAC_DRQ_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
					   0x1 << ADC_DRQ_EN, 0x1 << ADC_DRQ_EN);
			if (dts->rx_sync_en && dts->rx_sync_ctl)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, true);
		}
	break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
					   0x1 << DAC_DRQ_EN, 0x1 << DAC_DRQ_EN);
			if (codec->audio_sta.spk) {
				codec->audio_sta.spk = true;
				snd_sunxi_pa_pin_enable(codec->pa_cfg, codec->pa_pin_max);
			}
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
					   0x1 << ADC_DRQ_EN, 0x1 << ADC_DRQ_EN);
			if (dts->rx_sync_en && dts->rx_sync_ctl)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, true);
		}
	break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
					   0x1 << DAC_DRQ_EN, 0x0 << DAC_DRQ_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
					   0x1 << ADC_DRQ_EN, 0x0 << ADC_DRQ_EN);
			if (dts->rx_sync_en && dts->rx_sync_ctl)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, false);
		}
	break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (codec->audio_sta.spk) {
				snd_sunxi_pa_pin_disable_irp(codec->pa_cfg, codec->pa_pin_max,
							     (void *)regmap, sunxi_pa_disa_cb);
				codec->audio_sta.spk = false;
			}
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
					   0x1 << ADC_DRQ_EN, 0x0 << ADC_DRQ_EN);
			if (dts->rx_sync_en && dts->rx_sync_ctl)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, false);
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
	.name = DRV_NAME,
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
static const char *sunxi_ad2da_loop_mode_text[] = {"Off", "ADC12-to-DACLR"};
static const char *sunxi_da2ad_loop_mode_text[] = {"Off", "DACLR-to-ADC12"};
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_tx_hub_mode_enum, sunxi_switch_text);
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_rx_sync_mode_enum, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_dacdrc_sta_enum, SND_SOC_NOPM, DACDRC_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_dachpf_sta_enum, SND_SOC_NOPM, DACHPF_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_adcdrc_sta_enum, SND_SOC_NOPM, ADCDRC_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_adchpf_sta_enum, SND_SOC_NOPM, ADCHPF_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_ad2da_loop_mode_enum, sunxi_ad2da_loop_mode_text);
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_da2ad_loop_mode_enum, sunxi_da2ad_loop_mode_text);
static SOC_ENUM_SINGLE_DECL(sunxi_dac_swap_enum, SUNXI_DAC_DEBUG, DAC_SWP, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_adc_swap_enum, SUNXI_ADC_DEBUG, ADC_SWP, sunxi_switch_text);
/* Digital-tlv */
static const DECLARE_TLV_DB_SCALE(dac_vol_tlv, -7424, 116, 0);
static const DECLARE_TLV_DB_SCALE(dacl_vol_tlv, -11925, 75, 1);
static const DECLARE_TLV_DB_SCALE(dacr_vol_tlv, -11925, 75, 1);
static const DECLARE_TLV_DB_SCALE(adc1_vol_tlv, -11925, 75, 1);
static const DECLARE_TLV_DB_SCALE(adc2_vol_tlv, -11925, 75, 1);
/* Analog-tlv */
static const DECLARE_TLV_DB_SCALE(hpout_gain_tlv, -4200, 600, 1);
static const DECLARE_TLV_DB_SCALE(adc1_gain_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(adc2_gain_tlv, 0, 100, 0);
static const unsigned int lineout_gain_tlv[] = {
	TLV_DB_RANGE_HEAD(2),
	0, 1, TLV_DB_SCALE_ITEM(0, 0, 1),
	2, 31, TLV_DB_SCALE_ITEM(-4350, 150, 1),
};

/* RX&TX FUNC */
static int sunxi_get_tx_hub_mode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_DAC_DPC, &reg_val);

	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << HUB_EN)) ? 1 : 0);

	return 0;
}

static int sunxi_set_tx_hub_mode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << HUB_EN, 0x0 << HUB_EN);
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << HUB_EN, 0x1 << HUB_EN);
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

	if (enable)
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
				   1 << RX_SYNC_EN_START, 1 << RX_SYNC_EN_START);
	else
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
				   1 << RX_SYNC_EN_START, 0 << RX_SYNC_EN_START);
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
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL, 0x1 << RX_SYNC_EN,
				   0x0 << RX_SYNC_EN);
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL, 0x1 << ADCDFEN, 0x1 << ADCDFEN);
		sunxi_rx_sync_shutdown(dts->rx_sync_domain, dts->rx_sync_id);
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL, 0x1 << RX_SYNC_EN,
				   0x1 << RX_SYNC_EN);
		/* Cancel the delay time in order to align the start point */
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL, 0x1 << ADCDFEN, 0x0 << ADCDFEN);
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
	case ADCDRC_SHIFT:
		regmap_read(regmap, SUNXI_ADC_DAP_CTL, &reg_val);
		ucontrol->value.integer.value[0] =
			(reg_val & 0x1 << ADAP_EN) && (reg_val & 0x1 << ADAP_DRC_EN) ? 1 : 0;
		break;
	case ADCHPF_SHIFT:
		regmap_read(regmap, SUNXI_ADC_DAP_CTL, &reg_val);
		ucontrol->value.integer.value[0] =
			(reg_val & 0x1 << ADAP_EN) && (reg_val & 0x1 << ADAP_HPF_EN) ? 1 : 0;
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
	case ADCDRC_SHIFT:
		if (ucontrol->value.integer.value[0]) {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADAP_EN | 0x1 << ADAP_DRC_EN,
					   0x1 << ADAP_EN | 0x1 << ADAP_DRC_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADAP_DRC_EN, 0x0 << ADAP_DRC_EN);
		}
		break;
	case ADCHPF_SHIFT:
		if (ucontrol->value.integer.value[0]) {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADAP_EN | 0x1 << ADAP_HPF_EN,
					   0x1 << ADAP_EN | 0x1 << ADAP_HPF_EN);
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

/* ADDA LOOP FUNC */
static int sunxi_codec_get_ad2da_loop_mode(struct snd_kcontrol *kcontrol,
					  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_DAC_DEBUG, &reg_val);

	ucontrol->value.integer.value[0] = (reg_val >> AD2DA_LOOP_MODE) & 0x1;

	return 0;
}

static int sunxi_codec_set_ad2da_loop_mode(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		regmap_update_bits(regmap, SUNXI_DAC_DEBUG, 0x1 << AD2DA_LOOP_MODE,
				   0x0 << AD2DA_LOOP_MODE);
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_DAC_DEBUG, 0x1 << AD2DA_LOOP_MODE,
				   0x1 << AD2DA_LOOP_MODE);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_codec_get_da2ad_loop_mode(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_DAC_DEBUG, &reg_val);

	ucontrol->value.integer.value[0] = (reg_val >> DA2AD_LOOP_MODE) & 0x3;

	return 0;
}

static int sunxi_codec_set_da2ad_loop_mode(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		regmap_update_bits(regmap, SUNXI_DAC_DEBUG, 0x3 << DA2AD_LOOP_MODE,
				   0x0 << DA2AD_LOOP_MODE);
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_DAC_DEBUG, 0x3 << DA2AD_LOOP_MODE,
				   0x1 << DA2AD_LOOP_MODE);
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
	/* DAP Func */
	SOC_ENUM_EXT("DAC DRC Mode", sunxi_dacdrc_sta_enum, sunxi_codec_get_dap_status,
		     sunxi_codec_set_dap_status),
	SOC_ENUM_EXT("DAC HPF Mode", sunxi_dachpf_sta_enum, sunxi_codec_get_dap_status,
		     sunxi_codec_set_dap_status),
	SOC_ENUM_EXT("ADC DRC Mode", sunxi_adcdrc_sta_enum,
		     sunxi_codec_get_dap_status, sunxi_codec_set_dap_status),
	SOC_ENUM_EXT("ADC HPF Mode", sunxi_adchpf_sta_enum, sunxi_codec_get_dap_status,
		     sunxi_codec_set_dap_status),

	/* ADC&DAC Loop */
	SOC_ENUM_EXT("AD2DA Loop Mode", sunxi_ad2da_loop_mode_enum, sunxi_codec_get_ad2da_loop_mode,
		     sunxi_codec_set_ad2da_loop_mode),
	SOC_ENUM_EXT("DA2AD Loop Mode", sunxi_da2ad_loop_mode_enum, sunxi_codec_get_da2ad_loop_mode,
		     sunxi_codec_set_da2ad_loop_mode),

	/* ADC/DAC Swap */
	SOC_ENUM("DACL DACR Swap", sunxi_dac_swap_enum),
	SOC_ENUM("ADC1 ADC2 Swap", sunxi_adc_swap_enum),

	/* Volume-Digital */
	SOC_SINGLE_TLV("DAC Volume", SUNXI_DAC_DPC, DVOL, 0X3F, 1, dac_vol_tlv),
	SOC_SINGLE_TLV("DACL Volume", SUNXI_DAC_VOL_CTL, DAC_VOL_L, 0xFF, 0, dacl_vol_tlv),
	SOC_SINGLE_TLV("DACR Volume", SUNXI_DAC_VOL_CTL, DAC_VOL_R, 0xFF, 0, dacr_vol_tlv),
	SOC_SINGLE_TLV("ADC1 Volume", SUNXI_ADC_VOL_CTL1, ADC1_VOL, 0xFF, 0, adc1_vol_tlv),
	SOC_SINGLE_TLV("ADC2 Volume", SUNXI_ADC_VOL_CTL1, ADC2_VOL, 0xFF, 0, adc2_vol_tlv),

	/* Gain-Analog */
	SOC_SINGLE_TLV("LINEOUT Gain", SUNXI_DAC_AN_CTL, LINEOUT_GAIN, 0x1F, 0, lineout_gain_tlv),
	SOC_SINGLE_TLV("HPOUT Gain", SUNXI_DAC_AN_CTL, HEADPHONE_GAIN, 0x7, 1, hpout_gain_tlv),
	SOC_SINGLE_TLV("ADC1 Gain", SUNXI_ADC1_AN_CTL, ADC1_PGA_GAIN_CTL, 0x1F, 0, adc1_gain_tlv),
	SOC_SINGLE_TLV("ADC2 Gain", SUNXI_ADC2_AN_CTL, ADC2_PGA_GAIN_CTL, 0x1F, 0, adc2_gain_tlv),
};

static int sunxi_playback_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	unsigned int reg_val;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_read(regmap, SUNXI_DAC_DPC, &reg_val);
		reg_val &= (0x1 << DAC_DIG_EN);
		if (!reg_val) {
			regmap_update_bits(regmap, SUNXI_DAC_DPC,
					   0x1 << DAC_DIG_EN, 0x1 << DAC_DIG_EN);
			udelay(1000);
		}
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_read(regmap, SUNXI_DAC_AN_CTL, &reg_val);
		reg_val &= ((0x1 << DACL_EN) | (0x1 << DACR_EN));
		if (!reg_val)
			regmap_update_bits(regmap, SUNXI_DAC_DPC,
					   0x1 << DAC_DIG_EN, 0x0 << DAC_DIG_EN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_lineoutl_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, SUNXI_DAC_AN_CTL, 0x1 << LMUTE, 0x1 << LMUTE);
		regmap_update_bits(regmap, SUNXI_DAC_AN_CTL,
				   0x1 << LINEOUTL_EN, 0x1 << LINEOUTL_EN);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		regmap_update_bits(regmap, SUNXI_DAC_AN_CTL,
				   0x1 << LINEOUTL_EN, 0x0 << LINEOUTL_EN);
		regmap_update_bits(regmap, SUNXI_DAC_AN_CTL, 0x1 << LMUTE, 0x0 << LMUTE);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_lineoutr_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, SUNXI_DAC_AN_CTL, 0x1 << RMUTE, 0x1 << RMUTE);
		regmap_update_bits(regmap, SUNXI_DAC_AN_CTL,
				   0x1 << LINEOUTR_EN, 0x1 << LINEOUTR_EN);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		regmap_update_bits(regmap, SUNXI_DAC_AN_CTL,
				   0x1 << LINEOUTR_EN, 0x0 << LINEOUTR_EN);
		regmap_update_bits(regmap, SUNXI_DAC_AN_CTL, 0x1 << RMUTE, 0x0 << RMUTE);
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

	SND_LOG_DEBUG("\n");

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

static int sunxi_hpout_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, SUNXI_AN_DEBUG1, 0x1 << DAC_MUTE_EN, 0x1 << DAC_MUTE_EN);
		regmap_update_bits(regmap, SUNXI_HP_AN_CTL, 0x1 << CP_EN, 0x1 << CP_EN);
		udelay(320);
		regmap_update_bits(regmap, SUNXI_HP_AN_CTL, 0x1 << HPPA_EN, 0x1 << HPPA_EN);
		udelay(320);
		regmap_update_bits(regmap, SUNXI_HP_AN_CTL, 0x1 << HP_CHOPPER_EN, 0x1 << HP_CHOPPER_EN);
		udelay(320);
		regmap_update_bits(regmap, SUNXI_HP_AN_CTL, 0x1 << HP_EN, 0x1 << HP_EN);
		udelay(320);
		regmap_update_bits(regmap, SUNXI_AN_DEBUG1, 0x1 << DAC_MUTE_EN, 0x0 << DAC_MUTE_EN);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		regmap_update_bits(regmap, SUNXI_HP_AN_CTL, 0x1 << HP_EN, 0x0 << HP_EN);
		udelay(320);
		regmap_update_bits(regmap, SUNXI_HP_AN_CTL, 0x1 << HP_CHOPPER_EN, 0x0 << HP_CHOPPER_EN);
		udelay(320);
		regmap_update_bits(regmap, SUNXI_HP_AN_CTL, 0x1 << HPPA_EN, 0x0 << HPPA_EN);
		udelay(320);
		regmap_update_bits(regmap, SUNXI_HP_AN_CTL, 0x1 << CP_EN, 0x0 << CP_EN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_mic1_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(regmap, SUNXI_ADC1_AN_CTL, 0x1 << MIC1_PGA_EN,
				   0x1 << MIC1_PGA_EN);
		regmap_update_bits(regmap, SUNXI_ADC1_AN_CTL, 0x1 << ADC1_EN, 0x1 << ADC1_EN);
		mutex_lock(&codec->audio_sta.acf_mutex);
		codec->audio_sta.mic1 = true;
		if (codec->audio_sta.mic2 == false) {
			regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL,
					   0x1 << MMIC_BIAS_EN, 0x1 << MMIC_BIAS_EN);
			/* delay 80ms to avoid pop recording in begining,
			 * and adc fifo delay time must not be less than 20ms.
			 */
			msleep(240);
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
					   0x1 << ADC_DIG_EN, 0x1 << ADC_DIG_EN);
		}
		mutex_unlock(&codec->audio_sta.acf_mutex);
		break;
	case SND_SOC_DAPM_POST_PMD:
		mutex_lock(&codec->audio_sta.acf_mutex);
		codec->audio_sta.mic1 = false;
		if (codec->audio_sta.mic2 == false) {
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
					   0x1 << ADC_DIG_EN, 0x0 << ADC_DIG_EN);
			regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL,
					   0x1 << MMIC_BIAS_EN, 0x0 << MMIC_BIAS_EN);
		}
		mutex_unlock(&codec->audio_sta.acf_mutex);
		regmap_update_bits(regmap, SUNXI_ADC1_AN_CTL, 0x1 << ADC1_EN, 0x0 << ADC1_EN);
		regmap_update_bits(regmap, SUNXI_ADC1_AN_CTL, 0x1 << MIC1_PGA_EN,
				   0x0 << MIC1_PGA_EN);
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
		regmap_update_bits(regmap, SUNXI_ADC2_AN_CTL, 0x1 << MIC2_PGA_EN,
				   0x1 << MIC2_PGA_EN);
		regmap_update_bits(regmap, SUNXI_ADC2_AN_CTL, 0x1 << ADC2_EN, 0x1 << ADC2_EN);
		mutex_lock(&codec->audio_sta.acf_mutex);
		codec->audio_sta.mic2 = true;
		if (codec->audio_sta.mic1 == false) {
			regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL,
					   0x1 << MMIC_BIAS_EN, 0x1 << MMIC_BIAS_EN);
			/* delay 80ms to avoid pop recording in begining,
			 * and adc fifo delay time must not be less than 20ms.
			 */
			msleep(240);
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
					   0x1 << ADC_DIG_EN, 0x1 << ADC_DIG_EN);
		}
		mutex_unlock(&codec->audio_sta.acf_mutex);
		break;
	case SND_SOC_DAPM_POST_PMD:
		mutex_lock(&codec->audio_sta.acf_mutex);
		codec->audio_sta.mic2 = false;
		if (codec->audio_sta.mic1 == false) {
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
					   0x1 << ADC_DIG_EN, 0x0 << ADC_DIG_EN);
			regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL,
					   0x1 << MMIC_BIAS_EN, 0x0 << MMIC_BIAS_EN);
		}
		mutex_unlock(&codec->audio_sta.acf_mutex);
		regmap_update_bits(regmap, SUNXI_ADC2_AN_CTL, 0x1 << ADC2_EN, 0x0 << ADC2_EN);
		regmap_update_bits(regmap, SUNXI_ADC2_AN_CTL, 0x1 << MIC2_PGA_EN,
				   0x0 << MIC2_PGA_EN);
		break;
	default:
		break;
	}

	return 0;
}

static const struct snd_soc_dapm_widget sunxi_codec_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_IN_E("DACL", "Playback", 0, SUNXI_DAC_AN_CTL, DACL_EN, 0,
			      sunxi_playback_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_IN_E("DACR", "Playback", 0, SUNXI_DAC_AN_CTL, DACR_EN, 0,
			      sunxi_playback_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT("ADC1", "Capture", 0, SUNXI_ADC_DIG_CTL, 0, 0),
	SND_SOC_DAPM_AIF_OUT("ADC2", "Capture", 0, SUNXI_ADC_DIG_CTL, 1, 0),

	SND_SOC_DAPM_OUTPUT("LINEOUTLP_PIN"),
	SND_SOC_DAPM_OUTPUT("LINEOUTLN_PIN"),
	SND_SOC_DAPM_OUTPUT("LINEOUTRP_PIN"),
	SND_SOC_DAPM_OUTPUT("LINEOUTRN_PIN"),
	SND_SOC_DAPM_OUTPUT("HPOUTL_PIN"),
	SND_SOC_DAPM_OUTPUT("HPOUTR_PIN"),
	SND_SOC_DAPM_INPUT("MIC1P_PIN"),
	SND_SOC_DAPM_INPUT("MIC1N_PIN"),
	SND_SOC_DAPM_INPUT("MIC2P_PIN"),
	SND_SOC_DAPM_INPUT("MIC2N_PIN"),

	SND_SOC_DAPM_LINE("LINEOUTL", sunxi_lineoutl_event),
	SND_SOC_DAPM_LINE("LINEOUTR", sunxi_lineoutr_event),
	SND_SOC_DAPM_SPK("SPK", sunxi_spk_event),
	SND_SOC_DAPM_HP("HPOUT", sunxi_hpout_event),
	SND_SOC_DAPM_MIC("MIC1", sunxi_mic1_event),
	SND_SOC_DAPM_MIC("MIC2", sunxi_mic2_event),
};

static const struct snd_soc_dapm_route sunxi_codec_dapm_routes[] = {
	/* Playback */
	/* DAC -> LINEOUT_PIN */
	{"LINEOUTLP_PIN", NULL, "DACL"},
	{"LINEOUTLN_PIN", NULL, "DACL"},
	{"LINEOUTRP_PIN", NULL, "DACR"},
	{"LINEOUTRN_PIN", NULL, "DACR"},
	/* DAC -> HPOUT_PIN */
	{"HPOUTL_PIN", NULL, "DACL"},
	{"HPOUTR_PIN", NULL, "DACR"},

	/* MIC_PIN -> ADC */
	{"ADC1", NULL, "MIC1P_PIN"},
	{"ADC1", NULL, "MIC1N_PIN"},
	{"ADC2", NULL, "MIC2P_PIN"},
	{"ADC2", NULL, "MIC2N_PIN"},
};

/* jack work -> codec */
static int sunxi_jack_codec_init(void *data);
static void sunxi_jack_codec_exit(void *data);
static int sunxi_jack_codec_suspend(void *data);
static int sunxi_jack_codec_resume(void *data);
static void sunxi_jack_codec_irq_clean(void *data);
static void sunxi_jack_codec_det_irq_work(void *data, enum snd_jack_types *jack_type);
static void sunxi_jack_codec_det_scan_work(void *data, enum snd_jack_types *jack_type);
static int sunxi_jack_sdbp_irq_init(void *data);
static void sunxi_jack_sdbp_irq_exit(void *data);
static void sunxi_jack_sdbp_scan_work(void *data, enum snd_jack_types *jack_type);

/* note:the background of the slow insertion problem: Due to the low quality of the earphone socket,
 * it is easy to trigger the insertion event when the earphone is not fully inserted.
 * During the earphone recognition process, the basedata used to detect the key will be set.
 * If the insertion speed is too slow, the basedata value will not match the actual value,
 * and eventually the key will fail
 */
struct sunxi_jack_codec sunxi_jack_codec = {
	.jack_init	= sunxi_jack_codec_init,
	.jack_exit	= sunxi_jack_codec_exit,
	.jack_suspend	= sunxi_jack_codec_suspend,
	.jack_resume	= sunxi_jack_codec_resume,

	.jack_irq_clean		= sunxi_jack_codec_irq_clean,
	.jack_det_irq_work	= sunxi_jack_codec_det_irq_work,
	.jack_det_scan_work	= sunxi_jack_codec_det_scan_work,

	.jack_sdbp.jack_sdbp_irq_init	= sunxi_jack_sdbp_irq_init,
	.jack_sdbp.jack_sdbp_irq_exit	= sunxi_jack_sdbp_irq_exit,
	.jack_sdbp.jack_sdbp_irq_clean	= sunxi_jack_codec_irq_clean,
	.jack_sdbp.jack_sdbp_irq_work	= sunxi_jack_codec_det_irq_work,

	.jack_sdbp.jack_sdbp_scan_init	= NULL,
	.jack_sdbp.jack_sdbp_scan_exit	= NULL,
	.jack_sdbp.jack_sdbp_scan_work	= sunxi_jack_sdbp_scan_work,

	.jack_sdbp.is_working = false,
};

static int sunxi_jack_codec_init(void *data)
{
	struct sunxi_jack_codec_priv *jack_codec_priv = data;
	struct regmap *regmap = jack_codec_priv->regmap;

	SND_LOG_DEBUG("\n");

	/* hp & mic det */
	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0xffff << 0, 0x0 << 0);
	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0x1f << MDATA_THRESHOLD,
			   jack_codec_priv->det_threshold << MDATA_THRESHOLD);
	regmap_update_bits(regmap, SUNXI_HMIC_STA, 0xffff << 0, 0x6000 << 0);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL,
			   0xff << SEL_DET_ADC_BF, 0x40 << SEL_DET_ADC_BF);
	if (jack_codec_priv->det_level == JACK_DETECT_LOW) {
		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL,
				   0x1 << AUTO_PULL_LOW_EN, 0x1 << AUTO_PULL_LOW_EN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << DET_MODE, 0x0 << DET_MODE);
	} else {
		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL,
				   0x1 << AUTO_PULL_LOW_EN, 0x0 << AUTO_PULL_LOW_EN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << DET_MODE, 0x1 << DET_MODE);
	}

	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << JACK_DET_EN, 0x1 << JACK_DET_EN);
	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0x1 << JACK_IN_IRQ_EN, 0x1 << JACK_IN_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0x1 << JACK_OUT_IRQ_EN, 0x1 << JACK_OUT_IRQ_EN);

	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0xf << HMIC_N,
			   jack_codec_priv->det_debounce << HMIC_N);

	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << HMIC_BIAS_EN, 0x0 << HMIC_BIAS_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << MIC_DET_ADC_EN,
			   0x0 << MIC_DET_ADC_EN);
	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0x1 << MIC_DET_IRQ_EN, 0x0 << MIC_DET_IRQ_EN);

	jack_codec_priv->irq_sta = JACK_IRQ_NULL;
	jack_codec_priv->irq_time = JACK_IRQ_NORMAL;

	return 0;
}

static void sunxi_jack_codec_exit(void *data)
{
	struct sunxi_jack_codec_priv *jack_codec_priv = data;
	struct regmap *regmap = jack_codec_priv->regmap;

	SND_LOG_DEBUG("\n");

	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0x1 << JACK_IN_IRQ_EN, 0x0 << JACK_IN_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0x1 << JACK_OUT_IRQ_EN, 0x0 << JACK_OUT_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0x1 << MIC_DET_IRQ_EN, 0x0 << MIC_DET_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << JACK_DET_EN, 0x0 << JACK_DET_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << MIC_DET_ADC_EN,
			   0x0 << MIC_DET_ADC_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << HMIC_BIAS_EN, 0x0 << HMIC_BIAS_EN);

	return;
}

static int sunxi_jack_codec_suspend(void *data)
{
	struct sunxi_jack_codec_priv *jack_codec_priv = data;
	struct regmap *regmap = jack_codec_priv->regmap;

	SND_LOG_DEBUG("\n");

	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0x1 << JACK_IN_IRQ_EN, 0x0 << JACK_IN_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0x1 << JACK_OUT_IRQ_EN, 0x0 << JACK_OUT_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0x1 << MIC_DET_IRQ_EN, 0x0 << MIC_DET_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << JACK_DET_EN, 0x0 << JACK_DET_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << MIC_DET_ADC_EN,
			   0x0 << MIC_DET_ADC_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << HMIC_BIAS_EN, 0x0 << HMIC_BIAS_EN);

	return 0;
}

static int sunxi_jack_codec_resume(void *data)
{
	struct sunxi_jack_codec_priv *jack_codec_priv = data;
	struct regmap *regmap = jack_codec_priv->regmap;

	SND_LOG_DEBUG("\n");

	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << JACK_DET_EN, 0x1 << JACK_DET_EN);
	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0x1 << JACK_IN_IRQ_EN, 0x1 << JACK_IN_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0x1 << JACK_OUT_IRQ_EN, 0x1 << JACK_OUT_IRQ_EN);

	return 0;
}

static void sunxi_jack_codec_irq_clean(void *data)
{
	unsigned int reg_val;
	unsigned int jack_state;
	struct sunxi_jack_codec_priv *jack_codec_priv = data;
	struct regmap *regmap = jack_codec_priv->regmap;
	struct sunxi_codec *codec;
	struct sunxi_jack_sdbp *jack_sdbp = &sunxi_jack_codec.jack_sdbp;

	codec = container_of(jack_codec_priv, struct sunxi_codec, jack_codec_priv);

	SND_LOG_DEBUG("\n");

	regmap_read(regmap, SUNXI_HMIC_STA, &jack_state);

	/* jack in */
	if (jack_state & (1 << JACK_IN_IRQ_STA)) {
		regmap_read(regmap, SUNXI_HMIC_STA, &reg_val);
		reg_val |= 0x1 << JACK_IN_IRQ_STA;
		reg_val &= ~(0x1 << JACK_OUT_IRQ_STA);
		regmap_write(regmap, SUNXI_HMIC_STA, reg_val);

		jack_codec_priv->irq_sta = JACK_IRQ_IN;
	}

	/* jack out */
	if (jack_state & (1 << JACK_OUT_IRQ_STA)) {
		regmap_read(regmap, SUNXI_HMIC_STA, &reg_val);
		reg_val &= ~(0x1 << JACK_IN_IRQ_STA);
		reg_val |= 0x1 << JACK_OUT_IRQ_STA;
		regmap_write(regmap, SUNXI_HMIC_STA, reg_val);

		jack_codec_priv->irq_sta = JACK_IRQ_OUT;
	}

	/* jack mic change */
	if (jack_state & (1 << MIC_DET_IRQ_STA)) {
		regmap_read(regmap, SUNXI_HMIC_STA, &reg_val);
		reg_val &= ~(0x1 << JACK_IN_IRQ_STA);
		reg_val &= ~(0x1 << JACK_OUT_IRQ_STA);
		reg_val |= 0x1 << MIC_DET_IRQ_STA;
		regmap_write(regmap, SUNXI_HMIC_STA, reg_val);

		if (jack_sdbp->jack_sdbp_method == SDBP_IRQ && jack_sdbp->is_working) {
			jack_codec_priv->irq_sta = JACK_IRQ_SDBP;
		} else {
			jack_codec_priv->irq_sta = JACK_IRQ_MIC;
		}
	}

	/* During scan mode, interrupts are not handled */
	if (jack_codec_priv->irq_time == JACK_IRQ_SCAN)
		jack_codec_priv->irq_sta = JACK_IRQ_NULL;

	return;
}

static void sunxi_jack_codec_det_irq_work(void *data, enum snd_jack_types *jack_type)
{
	struct sunxi_jack_codec_priv *jack_codec_priv = data;
	struct regmap *regmap = jack_codec_priv->regmap;
	unsigned int reg_val;
	unsigned int headset_basedata;
	struct sunxi_codec *codec;

	codec = container_of(jack_codec_priv, struct sunxi_codec, jack_codec_priv);

	SND_LOG_DEBUG("\n");

	switch (jack_codec_priv->irq_sta) {
	case JACK_IRQ_OUT:
		SND_LOG_DEBUG("jack out\n");
		regmap_update_bits(regmap, SUNXI_HMIC_CTL,
				   0x1 << JACK_IN_IRQ_EN, 0x1 << JACK_IN_IRQ_EN);
		regmap_update_bits(regmap, SUNXI_HMIC_CTL,
				   0x1 << JACK_OUT_IRQ_EN, 0x0 << JACK_OUT_IRQ_EN);

		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << HMIC_BIAS_EN,
				   0x0 << HMIC_BIAS_EN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << MIC_DET_ADC_EN,
				   0x0 << MIC_DET_ADC_EN);
		regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0x1 << MIC_DET_IRQ_EN,
				   0x0 << MIC_DET_IRQ_EN);

		*jack_type = 0;
	break;
	case JACK_IRQ_IN:
		SND_LOG_DEBUG("jack in\n");
		regmap_update_bits(regmap, SUNXI_HMIC_CTL,
				   0x1 << JACK_IN_IRQ_EN, 0x0 << JACK_IN_IRQ_EN);
		regmap_update_bits(regmap, SUNXI_HMIC_CTL,
				   0x1 << JACK_OUT_IRQ_EN, 0x1 << JACK_OUT_IRQ_EN);

		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << HMIC_BIAS_EN,
				   0x1 << HMIC_BIAS_EN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << MIC_DET_ADC_EN,
				   0x1 << MIC_DET_ADC_EN);
		msleep(500);
		regmap_read(regmap, SUNXI_HMIC_STA, &reg_val);

		headset_basedata = (reg_val >> HMIC_DATA) & 0x1f;

		if (headset_basedata > jack_codec_priv->det_threshold) {
			/* jack -> hp & mic */
			*jack_type = SND_JACK_HEADSET;

			/* get headset jack plugin */
			ktime_get_real_ts64(&jack_codec_priv->tv_headset_plugin);

			/* slow insert: save basedata */
			jack_codec_priv->hp_det_basedata = headset_basedata;

			if (headset_basedata > 3)
				headset_basedata -= 3;

			regmap_update_bits(regmap, SUNXI_HMIC_CTL,
					   0x1f << MDATA_THRESHOLD,
					   headset_basedata << MDATA_THRESHOLD);
			regmap_update_bits(regmap, SUNXI_HMIC_CTL,
					   0x1 << MIC_DET_IRQ_EN, 0x1 << MIC_DET_IRQ_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_HMIC_CTL,
					   0x1 << MIC_DET_IRQ_EN, 0x0 << MIC_DET_IRQ_EN);
			regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL,
					   0x1 << MIC_DET_ADC_EN, 0x0 << MIC_DET_ADC_EN);
			regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL,
					   0x1 << HMIC_BIAS_EN, 0x0 << HMIC_BIAS_EN);
			/* jack -> hp */
			*jack_type = SND_JACK_HEADPHONE;
		}
	break;
	case JACK_IRQ_SDBP:
		SND_LOG_DEBUG("jack sdbp\n");

		regmap_update_bits(regmap, SUNXI_HMIC_CTL,
				   0x1 << JACK_IN_IRQ_EN, 0x0 << JACK_IN_IRQ_EN);
		regmap_update_bits(regmap, SUNXI_HMIC_CTL,
				   0x1 << JACK_OUT_IRQ_EN, 0x1 << JACK_OUT_IRQ_EN);

		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << HMIC_BIAS_EN,
				   0x1 << HMIC_BIAS_EN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << MIC_DET_ADC_EN,
				   0x1 << MIC_DET_ADC_EN);
		msleep(500);
		regmap_read(regmap, SUNXI_HMIC_STA, &reg_val);

		headset_basedata = (reg_val >> HMIC_DATA) & 0x1f;
		SND_LOG_DEBUG("headset_basedata:%u\n", headset_basedata);
		if (headset_basedata > jack_codec_priv->det_threshold) {
			/* jack -> hp & mic */
			*jack_type = SND_JACK_HEADSET;

			/* get headset jack plugin */
			ktime_get_real_ts64(&jack_codec_priv->tv_headset_plugin);

			/* slow insert: save basedata */
			jack_codec_priv->hp_det_basedata = headset_basedata;

			if (headset_basedata > 3)
				headset_basedata -= 3;

			regmap_update_bits(regmap, SUNXI_HMIC_CTL,
					   0x1f << MDATA_THRESHOLD,
					   headset_basedata << MDATA_THRESHOLD);
			regmap_update_bits(regmap, SUNXI_HMIC_CTL,
					   0x1 << MIC_DET_IRQ_EN, 0x1 << MIC_DET_IRQ_EN);
		} else {
			/* jack -> hp */
			*jack_type = SND_JACK_HEADPHONE;
		}
	break;
	case JACK_IRQ_MIC:
		SND_LOG_DEBUG("jack button\n");

		/* slow insert: confirm basedata again */
		regmap_read(regmap, SUNXI_HMIC_STA, &reg_val);
		reg_val = (reg_val & 0x1f00) >> 8;
		if (reg_val > jack_codec_priv->det_threshold &&
			abs(reg_val - jack_codec_priv->hp_det_basedata) >= 3) {
			reg_val -= 3;
			regmap_update_bits(regmap, SUNXI_HMIC_CTL,
					   0x1f << MDATA_THRESHOLD,
					   reg_val << MDATA_THRESHOLD);
		}

		/* Prevent accidental triggering of buttons when the headset is just plugged in */
		ktime_get_real_ts64(&jack_codec_priv->tv_mic);
		if (abs(jack_codec_priv->tv_mic.tv_sec -
			jack_codec_priv->tv_headset_plugin.tv_sec) < 2)
			break;

		regmap_read(regmap, SUNXI_HMIC_STA, &reg_val);
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
		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL,
				   0x1 << AUTO_PULL_LOW_EN, 0x0 << AUTO_PULL_LOW_EN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << DET_MODE, 0x1 << DET_MODE);
	} else {
		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL,
				   0x1 << AUTO_PULL_LOW_EN, 0x1 << AUTO_PULL_LOW_EN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << DET_MODE, 0x0 << DET_MODE);
	}

	SND_LOG_DEBUG("sleep\n");
	msleep(500);	/* must ensure that the interrupt triggers */

	jack_codec_priv->irq_time = JACK_IRQ_NORMAL;
	if (jack_codec_priv->det_level == JACK_DETECT_LOW) {
		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL,
				   0x1 << AUTO_PULL_LOW_EN, 0x1 << AUTO_PULL_LOW_EN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << DET_MODE, 0x0 << DET_MODE);
	} else {
		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL,
				   0x1 << AUTO_PULL_LOW_EN, 0x0 << AUTO_PULL_LOW_EN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << DET_MODE, 0x1 << DET_MODE);
	}

	return;
}

static int sunxi_jack_sdbp_irq_init(void *data)
{
	struct sunxi_jack_codec_priv *jack_codec_priv = data;
	struct regmap *regmap = jack_codec_priv->regmap;

	SND_LOG_DEBUG("\n");

	regmap_update_bits(regmap, SUNXI_HMIC_CTL,
			   0x1 << MIC_DET_IRQ_EN, 0x1 << MIC_DET_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL,
			   0x1 << MIC_DET_ADC_EN, 0x1 << MIC_DET_ADC_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL,
			   0x1 << HMIC_BIAS_EN, 0x1 << HMIC_BIAS_EN);

	return 0;
}

static void sunxi_jack_sdbp_irq_exit(void *data)
{
	struct sunxi_jack_codec_priv *jack_codec_priv = data;
	struct regmap *regmap = jack_codec_priv->regmap;

	SND_LOG_DEBUG("\n");

	if (jack_codec_priv->jack_type == SND_JACK_HEADSET)
		return;

	regmap_update_bits(regmap, SUNXI_HMIC_CTL,
			   0x1 << MIC_DET_IRQ_EN, 0x0 << MIC_DET_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL,
			   0x1 << MIC_DET_ADC_EN, 0x0 << MIC_DET_ADC_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL,
			   0x1 << HMIC_BIAS_EN, 0x0 << HMIC_BIAS_EN);
}

static void sunxi_jack_sdbp_scan_work(void *data, enum snd_jack_types *jack_type)
{
	struct sunxi_jack_codec_priv *jack_codec_priv = data;
	struct regmap *regmap = jack_codec_priv->regmap;
	unsigned int headset_basedata;
	unsigned int reg_val;

	SND_LOG_DEBUG("jack_codec_priv->jack_type:%d\n", jack_codec_priv->jack_type);

	if (jack_codec_priv->jack_type != SND_JACK_HEADPHONE)
		return;

	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << HMIC_BIAS_EN,
			   0x1 << HMIC_BIAS_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << MIC_DET_ADC_EN,
			   0x1 << MIC_DET_ADC_EN);
	msleep(500);

	regmap_read(regmap, SUNXI_HMIC_STA, &reg_val);

	headset_basedata = (reg_val >> HMIC_DATA) & 0x1f;
	SND_LOG_DEBUG("headset_basedata:%u\n", headset_basedata);
	if (headset_basedata > jack_codec_priv->det_threshold) {
		jack_codec_priv->jack_type = SND_JACK_HEADSET;

		/* slow insert: save basedata */
		jack_codec_priv->hp_det_basedata = headset_basedata;

		if (headset_basedata > 3)
			headset_basedata -= 3;

		/* get headset jack plugin */
		ktime_get_real_ts64(&jack_codec_priv->tv_headset_plugin);

		regmap_update_bits(regmap, SUNXI_HMIC_CTL,
				   0x1f << MDATA_THRESHOLD,
				   headset_basedata << MDATA_THRESHOLD);
		regmap_update_bits(regmap, SUNXI_HMIC_CTL,
				   0x1 << MIC_DET_IRQ_EN, 0x1 << MIC_DET_IRQ_EN);

		*jack_type = SND_JACK_HEADSET;
		return;
	}

	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << HMIC_BIAS_EN,
			   0x0 << HMIC_BIAS_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << MIC_DET_ADC_EN,
			   0x0 << MIC_DET_ADC_EN);
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

	SND_LOG_DEBUG("\n");

	/* hp & mic det */
	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0xffff << 0, 0x0 << 0);
	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0x1f << MDATA_THRESHOLD,
			   jack_extcon_priv->det_threshold << MDATA_THRESHOLD);
	regmap_update_bits(regmap, SUNXI_HMIC_STA, 0xffff << 0, 0x6000 << 0);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0xff << SEL_DET_ADC_BF,
			   0x40 << SEL_DET_ADC_BF);
	if (jack_extcon_priv->det_level == JACK_DETECT_LOW) {
		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << AUTO_PULL_LOW_EN,
				   0x1 << AUTO_PULL_LOW_EN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << DET_MODE, 0x0 << DET_MODE);
	} else {
		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << AUTO_PULL_LOW_EN,
				   0x0 << AUTO_PULL_LOW_EN);
		regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << DET_MODE, 0x1 << DET_MODE);
	}
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << JACK_DET_EN, 0x1 << JACK_DET_EN);

	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0xf << HMIC_N,
			   jack_extcon_priv->det_debounce << HMIC_N);

	/* enable when jack in */
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << HMIC_BIAS_EN, 0x0 << HMIC_BIAS_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << MIC_DET_ADC_EN,
			   0x0 << MIC_DET_ADC_EN);
	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0x1 << MIC_DET_IRQ_EN, 0x0 << MIC_DET_IRQ_EN);

	jack_extcon_priv->irq_sta = JACK_IRQ_NULL;

	return 0;
}

static void sunxi_jack_extcon_exit(void *data)
{
	struct sunxi_jack_extcon_priv *jack_extcon_priv = data;
	struct regmap *regmap = jack_extcon_priv->regmap;

	SND_LOG_DEBUG("\n");

	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0x1 << MIC_DET_IRQ_EN, 0x0 << MIC_DET_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << JACK_DET_EN, 0x0 << JACK_DET_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << MIC_DET_ADC_EN,
			   0x0 << MIC_DET_ADC_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << HMIC_BIAS_EN, 0x0 << HMIC_BIAS_EN);

	return;
}

static int sunxi_jack_extcon_suspend(void *data)
{
	struct sunxi_jack_extcon_priv *jack_extcon_priv = data;
	struct regmap *regmap = jack_extcon_priv->regmap;

	SND_LOG_DEBUG("\n");

	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0x1 << MIC_DET_IRQ_EN, 0x0 << MIC_DET_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << JACK_DET_EN, 0x0 << JACK_DET_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << MIC_DET_ADC_EN,
			   0x0 << MIC_DET_ADC_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << HMIC_BIAS_EN, 0x0 << HMIC_BIAS_EN);

	return 0;
}

static int sunxi_jack_extcon_resume(void *data)
{
	struct sunxi_jack_extcon_priv *jack_extcon_priv = data;
	struct regmap *regmap = jack_extcon_priv->regmap;

	SND_LOG_DEBUG("\n");

	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << JACK_DET_EN, 0x1 << JACK_DET_EN);

	return 0;
}

static void sunxi_jack_extcon_irq_clean(void *data)
{
	unsigned int reg_val;
	unsigned int jack_state;
	struct sunxi_jack_extcon_priv *jack_extcon_priv = data;
	struct regmap *regmap = jack_extcon_priv->regmap;

	SND_LOG_DEBUG("\n");

	regmap_read(regmap, SUNXI_HMIC_STA, &jack_state);

	/* jack mic change */
	if (jack_state & (1 << MIC_DET_IRQ_STA)) {
		regmap_read(regmap, SUNXI_HMIC_STA, &reg_val);
		reg_val &= ~(0x1 << JACK_IN_IRQ_STA);
		reg_val &= ~(0x1 << JACK_OUT_IRQ_STA);
		reg_val |= 0x1 << MIC_DET_IRQ_STA;
		regmap_write(regmap, SUNXI_HMIC_STA, reg_val);

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

	SND_LOG_DEBUG("jack button\n");

	/* Prevent accidental triggering of buttons when the headset is just plugged in */
	ktime_get_real_ts64(&tv_mic);
	if (abs(tv_mic.tv_sec - jack_extcon_priv->tv_headset_plugin.tv_sec) < 2)
		return;

	regmap_read(regmap, SUNXI_HMIC_STA, &reg_val);
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

	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << HMIC_BIAS_EN, 0x1 << HMIC_BIAS_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << MIC_DET_ADC_EN,
			   0x1 << MIC_DET_ADC_EN);

	sunxi_jack_typec_mode_set(&jack_extcon->jack_typec_cfg, SND_JACK_MODE_MICI);
	msleep(100);
	for (i = 0; i < count; i++) {
		regmap_read(regmap, SUNXI_HMIC_STA, &reg_val);
		reg_val = (reg_val >> HMIC_DATA) & 0x1f;
		SND_LOG_DEBUG("HMIC data %u\n", reg_val);
		if (reg_val >= jack_extcon_priv->det_threshold)
			goto jack_headset;
		msleep(interval_ms);
	}

	sunxi_jack_typec_mode_set(&jack_extcon->jack_typec_cfg, SND_JACK_MODE_MICN);
	msleep(100);
	for (i = 0; i < count; i++) {
		regmap_read(regmap, SUNXI_HMIC_STA, &reg_val);
		reg_val = (reg_val >> HMIC_DATA) & 0x1f;
		SND_LOG_DEBUG("HMIC data %u\n", reg_val);
		if (reg_val >= jack_extcon_priv->det_threshold)
			goto jack_headset;
		msleep(interval_ms);
	}

	/* jack -> hp */
	*jack_type = SND_JACK_HEADPHONE;
	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0x1 << MIC_DET_IRQ_EN, 0x0 << MIC_DET_IRQ_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << HMIC_BIAS_EN, 0x0 << HMIC_BIAS_EN);
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x1 << MIC_DET_ADC_EN,
			   0x0 << MIC_DET_ADC_EN);
	return;

jack_headset:
	/* jack -> hp & mic */
	*jack_type = SND_JACK_HEADSET;
	regmap_read(regmap, SUNXI_HMIC_STA, &reg_val);
	headset_basedata = (reg_val >> HMIC_DATA) & 0x1f;
	if (headset_basedata > 3)
		headset_basedata -= 3;
	SND_LOG_DEBUG("jack headset_basedata -> %u\n", headset_basedata);

	regmap_update_bits(regmap, SUNXI_HMIC_CTL,
			   0x1f << MDATA_THRESHOLD,
			   headset_basedata << MDATA_THRESHOLD);
	ktime_get_real_ts64(&jack_extcon_priv->tv_headset_plugin);
	regmap_update_bits(regmap, SUNXI_HMIC_CTL, 0x1 << MIC_DET_IRQ_EN, 0x1 << MIC_DET_IRQ_EN);
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

	SND_LOG_DEBUG("\n");

	/* DAC will not have an output if disable */
	regmap_update_bits(regmap, SUNXI_RAMP_CTL, 0x1 << RAMP_CTL_EN, 0x1 << RAMP_CTL_EN);
	/* Enable DAC/ADC DAP */
	regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL, 0x1 << DDAP_EN, 0x1 << DDAP_EN);
	regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL, 0x1 << ADAP_EN, 0x1 << ADAP_EN);
	/* cpvcc set 1.2V, analog power for headphone charge pump */
	regmap_update_bits(regmap, SUNXI_DAC_AN_CTL, 0x1 << CPLDO_EN, 0x1 << CPLDO_EN);
	regmap_update_bits(regmap, SUNXI_DAC_AN_CTL, 0x3 << CPLDO_VOLTAGE, 0x3 << CPLDO_VOLTAGE);
	/* HMICBIAS set to 2.55V for support MEMS and ECM MIC */
	regmap_update_bits(regmap, SUNXI_MICBIAS_AN_CTL, 0x3 << HMIC_BIAS_SEL,
			   0x3 << HMIC_BIAS_SEL);
	/* Enable ADCFDT to overcome niose at the beginning */
	regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL, 0x1 << ADCDFEN, 0x1 << ADCDFEN);
	regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL, 0x3 << ADCFDT, 0x2 << ADCFDT);
	/* Open ADC1\2 and DACL\R Volume Setting */
	regmap_update_bits(regmap, SUNXI_DAC_VOL_CTL, 0x1 << DAC_VOL_SEL, 0x1 << DAC_VOL_SEL);
	regmap_update_bits(regmap, SUNXI_ADC_DIG_CTL, 0x1 << ADC1_2_VOL_EN, 0x1 << ADC1_2_VOL_EN);

	/* Set default volume/gain control value */
	regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x3F << DVOL, dts->dac_vol << DVOL);
	regmap_update_bits(regmap, SUNXI_DAC_VOL_CTL,
			   0xFF << DAC_VOL_L,
			   dts->dacl_vol << DAC_VOL_L);
	regmap_update_bits(regmap, SUNXI_DAC_VOL_CTL,
			   0xFF << DAC_VOL_R,
			   dts->dacr_vol << DAC_VOL_R);
	regmap_update_bits(regmap, SUNXI_ADC_VOL_CTL1, 0xFF << ADC1_VOL, dts->adc1_vol << ADC1_VOL);
	regmap_update_bits(regmap, SUNXI_ADC_VOL_CTL1, 0xFF << ADC2_VOL, dts->adc2_vol << ADC2_VOL);
	regmap_update_bits(regmap, SUNXI_DAC_AN_CTL,
			   0x1F << LINEOUT_GAIN,
			   dts->lineout_gain << LINEOUT_GAIN);
	regmap_update_bits(regmap, SUNXI_DAC_AN_CTL,
			   0x7 << HEADPHONE_GAIN,
			   dts->hpout_gain << HEADPHONE_GAIN);
	regmap_update_bits(regmap, SUNXI_ADC1_AN_CTL,
			   0x1F << ADC1_PGA_GAIN_CTL,
			   dts->adc1_gain << ADC1_PGA_GAIN_CTL);
	regmap_update_bits(regmap, SUNXI_ADC2_AN_CTL,
			   0x1F << ADC2_PGA_GAIN_CTL,
			   dts->adc2_gain << ADC2_PGA_GAIN_CTL);

	/* DRC & HPF config */
	snd_sunxi_dap_dacdrc(regmap);
	snd_sunxi_dap_dachpf(regmap);
	snd_sunxi_dap_adcdrc(regmap);
	snd_sunxi_dap_adchpf(regmap);

	return;
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
			SND_LOG_ERR_STD(E_AUDIOCODEC_SWSYS_COMP_PROBE,
					"add tx_hub kcontrols failed\n");
	}
	/* component kcontrols -> rx_sync */
	if (dts->rx_sync_en) {
		ret = snd_soc_add_component_controls(component, sunxi_rx_sync_controls,
						     ARRAY_SIZE(sunxi_rx_sync_controls));
		if (ret)
			SND_LOG_ERR_STD(E_AUDIOCODEC_SWSYS_COMP_PROBE,
					"add rx_sync kcontrols failed\n");

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
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWSYS_COMP_PROBE,
				"register codec kcontrols failed\n");

	/* component kcontrols -> pa */
	ret = snd_sunxi_pa_pin_probe(codec->pa_cfg, codec->pa_pin_max, component);
	if (ret)
		SND_LOG_ERR("register pa kcontrols failed\n");

	/* dapm-widget */
	ret = snd_soc_dapm_new_controls(dapm, sunxi_codec_dapm_widgets,
					ARRAY_SIZE(sunxi_codec_dapm_widgets));
	if (ret)
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWSYS_COMP_PROBE,
				"register codec dapm_widgets failed\n");

	/* dapm-routes */
	ret = snd_soc_dapm_add_routes(dapm, sunxi_codec_dapm_routes,
				      ARRAY_SIZE(sunxi_codec_dapm_routes));
	if (ret)
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWSYS_COMP_PROBE,
				"register codec dapm_routes failed\n");

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
	snd_sunxi_pa_pin_disable(codec->pa_cfg, codec->pa_pin_max);
	snd_sunxi_regulator_disable(codec->rglt);
	snd_sunxi_clk_bus_disable(&codec->clk);

	return 0;
}

static int sunxi_codec_component_resume(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	int ret;

	SND_LOG_DEBUG("\n");

	snd_sunxi_regulator_enable(codec->rglt);
	ret = snd_sunxi_clk_bus_enable(&codec->clk);
	if (ret) {
		SND_LOG_ERR("clk enable failed\n");
		return ret;
	}
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
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_MEM_INIT, "parse device node resource failed\n");
		ret = -EINVAL;
		goto err_of_addr_to_resource;
	}

	mem->memregion = devm_request_mem_region(&pdev->dev, mem->res.start,
						 resource_size(&mem->res), DRV_NAME);
	if (IS_ERR_OR_NULL(mem->memregion)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_MEM_INIT, "memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_request_region;
	}

	mem->membase = devm_ioremap(&pdev->dev, mem->memregion->start,
				    resource_size(mem->memregion));
	if (IS_ERR_OR_NULL(mem->membase)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_MEM_INIT, "ioremap failed\n");
		ret = -EBUSY;
		goto err_devm_ioremap;
	}

	mem->regmap = devm_regmap_init_mmio(&pdev->dev, mem->membase, &sunxi_regmap_config);
	if (IS_ERR_OR_NULL(mem->regmap)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_MEM_INIT, "regmap init failed\n");
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
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_INIT, "clk rst get failed\n");
		ret =  PTR_ERR(clk->clk_rst);
		goto err_get_clk_rst;
	}

	/* get bus clk */
	clk->clk_bus = of_clk_get_by_name(np, "clk_bus_adda");
	if (IS_ERR_OR_NULL(clk->clk_bus)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_INIT, "clk bus get failed\n");
		ret = PTR_ERR(clk->clk_bus);
		goto err_get_clk_bus;
	}

	/* get parent clk */
	clk->clk_pll_audio0 = of_clk_get_by_name(np, "clk_pll_audio0");
	if (IS_ERR_OR_NULL(clk->clk_pll_audio0)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_INIT, "clk_pll_audio0 get failed\n");
		ret = PTR_ERR(clk->clk_pll_audio0);
		goto err_get_clk_pll_audio0;
	}

	clk->clk_pll_audio1_5x = of_clk_get_by_name(np, "clk_pll_audio1_5x");
	if (IS_ERR_OR_NULL(clk->clk_pll_audio1_5x)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_INIT, "clk_pll_audio1_5x get failed\n");
		ret = PTR_ERR(clk->clk_pll_audio1_5x);
		goto err_get_clk_pll_audio1_5x;
	}

	/* get module clk */
	clk->clk_adda_dac = of_clk_get_by_name(np, "clk_adda_dac");
	if (IS_ERR_OR_NULL(clk->clk_adda_dac)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_INIT, "clk_adda_dac get failed\n");
		ret = PTR_ERR(clk->clk_adda_dac);
		goto err_get_clk_adda_dac;
	}
	clk->clk_adda_adc = of_clk_get_by_name(np, "clk_adda_adc");
	if (IS_ERR_OR_NULL(clk->clk_adda_adc)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_INIT, "clk_adda_adc get failed\n");
		ret = PTR_ERR(clk->clk_adda_adc);
		goto err_get_clk_adda_adc;
	}

	return 0;

err_get_clk_adda_adc:
	clk_put(clk->clk_adda_dac);
err_get_clk_adda_dac:
	clk_put(clk->clk_pll_audio1_5x);
err_get_clk_pll_audio1_5x:
	clk_put(clk->clk_pll_audio0);
err_get_clk_pll_audio0:
	clk_put(clk->clk_bus);
err_get_clk_bus:
err_get_clk_rst:
	return ret;
}

static void snd_sunxi_clk_exit(struct sunxi_codec_clk *clk)
{
	SND_LOG_DEBUG("\n");

	clk_put(clk->clk_adda_adc);
	clk_put(clk->clk_adda_dac);
	clk_put(clk->clk_pll_audio1_5x);
	clk_put(clk->clk_pll_audio0);
	clk_put(clk->clk_bus);
}

static int snd_sunxi_clk_bus_enable(struct sunxi_codec_clk *clk)
{
	int ret = 0;

	SND_LOG_DEBUG("\n");

	/* to avoid register modification before module load */
	reset_control_assert(clk->clk_rst);
	if (reset_control_deassert(clk->clk_rst)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_EN, "clk_rst deassert failed\n");
		ret = -EINVAL;
		goto err_deassert_rst;
	}

	if (clk_prepare_enable(clk->clk_bus)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_EN, "clk_bus enable failed\n");
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

	if (clk_prepare_enable(clk->clk_pll_audio0)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_EN, "clk_pll_audio0 enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_pll_audio0;
	}

	if (clk_prepare_enable(clk->clk_pll_audio1_5x)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_EN, "clk_pll_audio1_5x enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_pll_audio1_5x;
	}

	if (clk_prepare_enable(clk->clk_adda_dac)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_EN, "clk_adda_dac enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_adda_dac;
	}

	if (clk_prepare_enable(clk->clk_adda_adc)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_EN, "clk_adda_adc enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_adda_adc;
	}

	return 0;

err_enable_clk_adda_adc:
	clk_disable_unprepare(clk->clk_adda_dac);
err_enable_clk_adda_dac:
	clk_disable_unprepare(clk->clk_pll_audio1_5x);
err_enable_clk_pll_audio1_5x:
	clk_disable_unprepare(clk->clk_pll_audio0);
err_enable_clk_pll_audio0:
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

	clk_disable_unprepare(clk->clk_adda_adc);
	clk_disable_unprepare(clk->clk_adda_dac);
	clk_disable_unprepare(clk->clk_pll_audio1_5x);
	clk_disable_unprepare(clk->clk_pll_audio0);
}

static int snd_sunxi_clk_rate(struct sunxi_codec_clk *clk, int stream,
			      unsigned int freq_in, unsigned int freq_out)
{
	SND_LOG_DEBUG("\n");

	if (stream  == SNDRV_PCM_STREAM_PLAYBACK) {
		if (freq_in % 24576000 == 0) {
			if (clk_set_parent(clk->clk_adda_dac, clk->clk_pll_audio1_5x)) {
				SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_SET,
						"set dac parent clk failed\n");
				return -EINVAL;
			}
		} else {
			if (clk_set_parent(clk->clk_adda_dac, clk->clk_pll_audio0)) {
				SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_SET,
						"set dac parent clk failed\n");
				return -EINVAL;
			}
		}
		if (clk_set_rate(clk->clk_adda_dac, freq_out)) {
			SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_SET,
					"set clk_adda_dac rate failed, rate: %u\n", freq_out);
			return -EINVAL;
		}
	} else {
		if (freq_in % 24576000 == 0) {
			if (clk_set_parent(clk->clk_adda_adc, clk->clk_pll_audio1_5x)) {
				SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_SET,
						"set adc parent clk failed\n");
				return -EINVAL;
			}
		} else {
			if (clk_set_parent(clk->clk_adda_adc, clk->clk_pll_audio0)) {
				SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_SET,
						"set adc parent clk failed\n");
				return -EINVAL;
			}
		}
		if (clk_set_rate(clk->clk_adda_adc, freq_out)) {
			SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_SET,
					"set clk_adda_adc rate failed, rate: %u\n", freq_out);
			return -EINVAL;
		}
	}

	return 0;
}

static void snd_sunxi_dts_params_init(struct platform_device *pdev, struct sunxi_codec_dts *dts)
{
	int ret = 0;
	unsigned int temp_val;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_codec *codec = dev_get_drvdata(&pdev->dev);

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

	ret = of_property_read_u32(np, "dacr-vol", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("dacr volume get failed\n");
		dts->dacr_vol = 160;
	} else {
		dts->dacr_vol = temp_val;
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

	ret = of_property_read_u32(np, "hpout-gain", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("hpout gain get failed\n");
		dts->hpout_gain = 0;
	} else {
		dts->hpout_gain = 7 - temp_val;
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

	SND_LOG_DEBUG("******jack codec param******\n");
	/* jack param -> codec */
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
		codec->jack_codec_priv.det_threshold = 8; /* AW1890 default 8 */
	} else {
		codec->jack_codec_priv.det_threshold = temp_val;
	}
	ret = of_property_read_u32(np, "jack-det-debounce", &temp_val);
	if (ret < 0 || temp_val > 15) {
		codec->jack_codec_priv.det_debounce = 0; /* AW1890 default 125ms */
	} else {
		codec->jack_codec_priv.det_debounce = temp_val;
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
	SND_LOG_DEBUG("jack-det-debouce -> %u\n",
		      codec->jack_codec_priv.det_debounce);
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

	SND_LOG_DEBUG("******jack extcon param******\n");
	/* jack param -> extcon */
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
		codec->jack_extcon_priv.det_threshold = 8; /* AW1890 default 8 */
	} else {
		codec->jack_extcon_priv.det_threshold = temp_val;
	}
	ret = of_property_read_u32(np, "jack-det-debouce", &temp_val);
	if (ret < 0 || temp_val > 15) {
		codec->jack_extcon_priv.det_debounce = 0; /* AW1890 default 125ms */
	} else {
		codec->jack_extcon_priv.det_debounce = temp_val;
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
	SND_LOG_DEBUG("jack-det-debouce -> %u\n",
		      codec->jack_extcon_priv.det_debounce);
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

	/* print volume & gain value */
	SND_LOG_DEBUG("dac_vol:%u, dacl_vol:%u, dacr_vol:%u\n",
		      dts->dac_vol, dts->dacl_vol, dts->dacr_vol);
	SND_LOG_DEBUG("adc1_vol:%u, adc2_vol:%u\n",
		      dts->adc1_vol, dts->adc2_vol);
	SND_LOG_DEBUG("lineout_gain:%u, hpout_gain:%u\n",
		      dts->lineout_gain, dts->hpout_gain);
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
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWARG_DUMP_VER, "priv to codec failed\n");
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
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWARG_DUMP_SHOW, "priv to codec failed\n");
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
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWARG_DUMP_STORE, "priv to codec failed\n");
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
	struct sunxi_codec_dts *dts;
#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	struct snd_sunxi_dump *dump;
#endif

	SND_LOG_DEBUG("\n");

	/* sunxi codec info */
	codec = devm_kzalloc(dev, sizeof(*codec), GFP_KERNEL);
	if (!codec) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWSYS_PLAT_PROBE,
				"can't allocate sunxi codec memory\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}
	dev_set_drvdata(dev, codec);
	mem = &codec->mem;
	clk = &codec->clk;
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
	codec->rglt = snd_sunxi_regulator_init(dev);
	if (!codec->rglt) {
		SND_LOG_ERR("rglt init failed\n");
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
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWSYS_PLAT_PROBE,
				"internal-codec component register failed\n");
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
	snd_sunxi_regulator_exit(codec->rglt);
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
	struct snd_sunxi_rglt *rglt = codec->rglt;
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
	snd_sunxi_regulator_exit(rglt);
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
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWSYS_MOD_INIT, "platform driver register failed\n");
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

MODULE_AUTHOR("huhaoxin@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.1");
MODULE_DESCRIPTION("sunxi soundcard codec of internal-codec");
