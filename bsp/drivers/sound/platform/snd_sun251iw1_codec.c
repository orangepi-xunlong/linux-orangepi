// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2024, zhouxijing <zhouxijing@allwinnertech.com>
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
#include "snd_sunxi_jack.h"
#include "snd_sunxi_rxsync.h"
#include "snd_sunxi_dap.h"
#include "snd_sun251iw1_codec.h"

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
	REG_LABEL(SUNXI_ADC_DEBUG),
	REG_LABEL(SUNXI_ADC_DIG_CTRL),
	REG_LABEL(SUNXI_VRA1SPEEDUP_DOWN_CTRL),
	REG_LABEL(SUNXI_DAC_DAP_CTL),
	REG_LABEL(SUNXI_ADC_DAP_CTL),
	REG_LABEL(SUNXI_AC_VERSION),
	REG_LABEL(SUNXI_ADC1_REG),
	REG_LABEL(SUNXI_ADC2_REG),
	REG_LABEL(SUNXI_DAC_REG),
	REG_LABEL(SUNXI_RAMP_REG),
	REG_LABEL(SUNXI_BIAS_REG),
	REG_LABEL(SUNXI_HP2_REG),
	REG_LABEL(SUNXI_POWER_REG),
	REG_LABEL(SUNXI_ADC_CUR_REG),
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
	case	SNDRV_PCM_FORMAT_S16_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
					   3 << FIFO_MODE, 3 << FIFO_MODE);
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
					   1 << TX_SAMPLE_BITS, 0 << TX_SAMPLE_BITS);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
					   1 << RX_FIFO_MODE, 1 << RX_FIFO_MODE);
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
					   1 << RX_SAMPLE_BITS, 0 << RX_SAMPLE_BITS);
		}
		break;
	case	SNDRV_PCM_FORMAT_S32_LE:
	case	SNDRV_PCM_FORMAT_S24_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
					   3 << FIFO_MODE, 0 << FIFO_MODE);
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
					   1 << TX_SAMPLE_BITS, 1 << TX_SAMPLE_BITS);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
					   1 << RX_FIFO_MODE, 0 << RX_FIFO_MODE);
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
					   1 << RX_SAMPLE_BITS, 1 << RX_SAMPLE_BITS);
		}
		break;
	default:
		break;
	}

	/* Set rate */
	for (i = 0; i < ARRAY_SIZE(sunxi_sample_rate_conv); i++) {
		if (sunxi_sample_rate_conv[i].samplerate == params_rate(params)) {
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
						   0x7 << DAC_FS,
						   sunxi_sample_rate_conv[i].rate_bit << DAC_FS);
			} else {
				if (sunxi_sample_rate_conv[i].samplerate > 48000)
					return -EINVAL;
				regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
						   0x7 << ADC_FS,
						   sunxi_sample_rate_conv[i].rate_bit<<ADC_FS);
			}
		}
	}

	/* Set channels */
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
			 SND_LOG_WARN("not support channels:%u\n", params_channels(params));
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

static int sunxi_codec_dai_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, SUNXI_DAC_FIFOC, 1 << FIFO_FLUSH, 1 << FIFO_FLUSH);
		regmap_write(regmap, SUNXI_DAC_FIFOS,
			     1 << DAC_TXE_INT | 1 << DAC_TXU_INT | 1 << DAC_TXO_INT);
		regmap_write(regmap, SUNXI_DAC_CNT, 0);
	} else {
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   1 << ADC_FIFO_FLUSH, 1 << ADC_FIFO_FLUSH);
		regmap_write(regmap, SUNXI_ADC_FIFOS, 1 << ADC_RXA_INT | 1 << ADC_RXO_INT);
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
				   int cmd, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_dts *dts = &codec->dts;
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

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
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
					   0x1 << DAC_DRQ_EN, 0x0 << DAC_DRQ_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
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
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates	= SNDRV_PCM_RATE_8000_192000
			| SNDRV_PCM_RATE_KNOT,
		.formats = SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S20_LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 3,
		.rates = SNDRV_PCM_RATE_8000_48000
			| SNDRV_PCM_RATE_KNOT,
		.formats = SNDRV_PCM_FMTBIT_S16_LE
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
static const char *sunxi_adda_loop_mode_text[] = {"Off", "DACLR-to-ADC12"};
static const char *sunxi_differ_text[] = {"differ", "single"};
static const char *sunxi_input1_mux_text[] = {"NONE", "MIC1", "FMINL", "LINEINL"};
static const char *sunxi_input2_mux_text[] = {"NONE", "MIC2", "FMINR", "LINEINR"};
static const char *sunxi_input_gain_text[] = {"0dB", "6dB"};

static SOC_ENUM_SINGLE_EXT_DECL(sunxi_tx_hub_mode_enum, sunxi_switch_text);
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_rx_sync_mode_enum, sunxi_switch_text);

static SOC_ENUM_SINGLE_DECL(sunxi_mic1_enum, SUNXI_ADC1_REG, MIC1_SIN_EN, sunxi_differ_text);
static SOC_ENUM_SINGLE_DECL(sunxi_mic2_enum, SUNXI_ADC2_REG, MIC2_SIN_EN, sunxi_differ_text);

static SOC_ENUM_SINGLE_DECL(sunxi_dacdrc_sta_enum, SND_SOC_NOPM, DACDRC_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_dachpf_sta_enum, SND_SOC_NOPM, DACHPF_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_adcdrc0_sta_enum, SND_SOC_NOPM, ADCDRC0_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_adchpf0_sta_enum, SND_SOC_NOPM, ADCHPF0_SHIFT, sunxi_switch_text);

static SOC_ENUM_SINGLE_EXT_DECL(sunxi_adda_loop_mode_enum, sunxi_adda_loop_mode_text);
static SOC_ENUM_SINGLE_DECL(sunxi_dac_swap_enum, SUNXI_DAC_DG, DAC_SWP, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_adc12_swap_enum, SUNXI_ADC_DEBUG, ADC_SWP1, sunxi_switch_text);

/* Digital-tlv */
static const DECLARE_TLV_DB_SCALE(dac_vol_tlv, -7424, 116, 0);
static const DECLARE_TLV_DB_SCALE(dacl_vol_tlv, -11925, 75, 1);
static const DECLARE_TLV_DB_SCALE(dacr_vol_tlv, -11925, 75, 1);
static const DECLARE_TLV_DB_SCALE(adc1_vol_tlv, -11925, 75, 1);
static const DECLARE_TLV_DB_SCALE(adc2_vol_tlv, -11925, 75, 1);
/* Analog-tlv */
static const DECLARE_TLV_DB_SCALE(hpout_gain_tlv, -4200, 600, 1);
static const unsigned int adc1_gain_tlv[] = {
	TLV_DB_RANGE_HEAD(3),
	1, 3, TLV_DB_SCALE_ITEM(600, 0, 0),
	4, 13, TLV_DB_SCALE_ITEM(600, 300, 0),
	14, 15, TLV_DB_SCALE_ITEM(3600, 0, 0),
};
static const unsigned int adc2_gain_tlv[] = {
	TLV_DB_RANGE_HEAD(3),
	1, 3, TLV_DB_SCALE_ITEM(600, 0, 0),
	4, 13, TLV_DB_SCALE_ITEM(600, 300, 0),
	14, 15, TLV_DB_SCALE_ITEM(3600, 0, 0),
};
static const unsigned int lineout_gain_tlv[] = {
	TLV_DB_RANGE_HEAD(2),
	0, 3, TLV_DB_SCALE_ITEM(-3000, 600, 0),
	4, 7, TLV_DB_SCALE_ITEM(-900, 300, 0),
};

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
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << DAC_HUB_EN, 0x0 << DAC_HUB_EN);
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << DAC_HUB_EN, 0x1 << DAC_HUB_EN);
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
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   1 << RX_SYNC_EN_STA, 1 << RX_SYNC_EN_STA);
	else
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   1 << RX_SYNC_EN_STA, 0 << RX_SYNC_EN_STA);
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
			(reg_val & 0x1 << ADAP0_EN) && (reg_val & 0x1 << ADAP0_DRC_EN) ? 1 : 0;
		break;
	case ADCHPF0_SHIFT:
		regmap_read(regmap, SUNXI_ADC_DAP_CTL, &reg_val);
		ucontrol->value.integer.value[0] =
			(reg_val & 0x1 << ADAP0_EN) && (reg_val & 0x1 << ADAP0_HPF_EN) ? 1 : 0;
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
					   0x1 << ADAP0_EN | 0x1 << ADAP0_DRC_EN,
					   0x1 << ADAP0_EN | 0x1 << ADAP0_DRC_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADAP0_DRC_EN, 0x0 << ADAP0_DRC_EN);
		}
		break;
	case ADCHPF0_SHIFT:
		if (ucontrol->value.integer.value[0]) {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADAP0_EN| 0x1 << ADAP0_HPF_EN,
					   0x1 << ADAP0_EN | 0x1 << ADAP0_HPF_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADAP0_HPF_EN, 0x0 << ADAP0_HPF_EN);
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

	mode_val = (reg_val >> ADDA_LOOP_MODE) & 0x7;

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
		regmap_update_bits(regmap, SUNXI_DAC_DG, 0x7 << ADDA_LOOP_MODE,
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
	/* DAP Func */
	SOC_ENUM_EXT("DAC DRC Mode", sunxi_dacdrc_sta_enum,
		     sunxi_codec_get_dap_status, sunxi_codec_set_dap_status),
	SOC_ENUM_EXT("DAC HPF Mode", sunxi_dachpf_sta_enum,
		     sunxi_codec_get_dap_status, sunxi_codec_set_dap_status),
	SOC_ENUM_EXT("ADC DRC0 Mode", sunxi_adcdrc0_sta_enum,
		     sunxi_codec_get_dap_status, sunxi_codec_set_dap_status),
	SOC_ENUM_EXT("ADC HPF0 Mode", sunxi_adchpf0_sta_enum,
		     sunxi_codec_get_dap_status, sunxi_codec_set_dap_status),

    /* ADDA Loop */
	SOC_ENUM_EXT("ADDA Loop Mode", sunxi_adda_loop_mode_enum, sunxi_codec_get_adda_loop_mode,
		     sunxi_codec_set_adda_loop_mode),

	/* ADC/DAC Swap */
	SOC_ENUM("DACL DACR Swap", sunxi_dac_swap_enum),
	SOC_ENUM("ADC1 ADC2 Swap", sunxi_adc12_swap_enum),

	/* single-end/differential input */
	SOC_ENUM("MIC1 Input Select", sunxi_mic1_enum),
	SOC_ENUM("MIC2 Input Select", sunxi_mic2_enum),

	/* Volume-Digital */
	SOC_SINGLE_TLV("DAC Volume", SUNXI_DAC_DPC, DVOL, 0x3F, 1, dac_vol_tlv),
	SOC_SINGLE_TLV("DACL Volume", SUNXI_DAC_VOL_CTRL, DAC_VOL_L, 0xFF, 0, dacl_vol_tlv),
	SOC_SINGLE_TLV("DACR Volume", SUNXI_DAC_VOL_CTRL,  DAC_VOL_R, 0xFF, 0, dacr_vol_tlv),
	SOC_SINGLE_TLV("ADC1 Volume", SUNXI_ADC_VOL_CTRL, ADC1_VOL, 0xFF, 0, adc1_vol_tlv),
	SOC_SINGLE_TLV("ADC2 Volume", SUNXI_ADC_VOL_CTRL, ADC2_VOL, 0xFF, 0, adc2_vol_tlv),

	/* Gain-Analog */
	SOC_SINGLE_TLV("LINEOUT Volume", SUNXI_DAC_REG, LINEOUT_VOL, 0x7, 0, lineout_gain_tlv),
	SOC_SINGLE_TLV("HPOUT Gain", SUNXI_HP2_REG, HEADPHONE_GAIN, 0x7, 1, hpout_gain_tlv),

	SOC_SINGLE_RANGE_TLV("ADC1 Gain", SUNXI_ADC1_REG, ADC1_PGA_GAIN_CTRL, 0x1, 0xF, 0, adc1_gain_tlv),
	SOC_SINGLE_RANGE_TLV("ADC2 Gain", SUNXI_ADC2_REG, ADC2_PGA_GAIN_CTRL, 0x1, 0xF, 0, adc2_gain_tlv),
};

static int sunxi_get_input_mux(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int shift = e->shift_l;
	unsigned int reg_val;
	bool mic = false;
	bool fmin = false;
	bool linein = false;

	/* default: micin */
	ucontrol->value.integer.value[0] = 0;

	switch (shift) {
	case INPUT1_SHIFT:
		regmap_read(regmap, SUNXI_ADC1_REG, &reg_val);
		if (reg_val & 0x1 << MIC1_PGA_EN) {
			mic = true;
			ucontrol->value.integer.value[0] = 1;
		}
		if (reg_val & 0x1 << FMINLEN) {
			fmin = true;
			ucontrol->value.integer.value[0] = 2;
		}
		if (reg_val & 0x1 << LINEINLEN) {
			linein = true;
			ucontrol->value.integer.value[0] = 3;
		}
		if ((mic + fmin + linein) == 0) {
			ucontrol->value.integer.value[0] = 0;
		}
		if ((mic + fmin + linein) > 1) {
			ucontrol->value.integer.value[0] = 0;
			SND_LOG_ERR("input1 mux has multiple input sources\n");
			return -EINVAL;
		}
		break;
	case INPUT2_SHIFT:
		regmap_read(regmap, SUNXI_ADC2_REG, &reg_val);
		if (reg_val & 0x1 << MIC2_PGA_EN) {
			mic = true;
			ucontrol->value.integer.value[0] = 1;
		}
		if (reg_val & 0x1 << FMINREN) {
			fmin = true;
			ucontrol->value.integer.value[0] = 2;
		}
		if (reg_val & 0x1 << LINEINREN) {
			linein = true;
			ucontrol->value.integer.value[0] = 3;
		}
		if ((mic + fmin + linein) == 0) {
			ucontrol->value.integer.value[0] = 0;
		}
		if ((mic + fmin + linein) > 1) {
			ucontrol->value.integer.value[0] = 0;
			SND_LOG_ERR("input2 mux has multiple input sources\n");
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sunxi_set_input_mux(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	struct snd_soc_dapm_context *dapm = snd_soc_dapm_kcontrol_dapm(kcontrol);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int shift = e->shift_l;

	switch (shift) {
	case INPUT1_SHIFT:
		if (ucontrol->value.integer.value[0] == 0) {
			regmap_update_bits(regmap, SUNXI_ADC1_REG,
					   1 << MIC1_PGA_EN, 0 << MIC1_PGA_EN);
			regmap_update_bits(regmap, SUNXI_ADC1_REG,
					   1 << FMINLEN, 0 << FMINLEN);
			regmap_update_bits(regmap, SUNXI_ADC1_REG,
					   1 << LINEINLEN, 0 << LINEINLEN);
		} else if (ucontrol->value.integer.value[0] == 1) {
			regmap_update_bits(regmap, SUNXI_ADC1_REG,
					   1 << MIC1_PGA_EN, 1 << MIC1_PGA_EN);
			regmap_update_bits(regmap, SUNXI_ADC1_REG,
					   1 << FMINLEN, 0 << FMINLEN);
			regmap_update_bits(regmap, SUNXI_ADC1_REG,
					   1 << LINEINLEN, 0 << LINEINLEN);
		} else if (ucontrol->value.integer.value[0] == 2) {
			regmap_update_bits(regmap, SUNXI_ADC1_REG,
					   1 << MIC1_PGA_EN, 0 << MIC1_PGA_EN);
			regmap_update_bits(regmap, SUNXI_ADC1_REG,
					   1 << FMINLEN, 1 << FMINLEN);
			regmap_update_bits(regmap, SUNXI_ADC1_REG,
					   1 << LINEINLEN, 0 << LINEINLEN);
		} else if (ucontrol->value.integer.value[0] == 3) {
			regmap_update_bits(regmap, SUNXI_ADC1_REG,
					   1 << MIC1_PGA_EN, 0 << MIC1_PGA_EN);
			regmap_update_bits(regmap, SUNXI_ADC1_REG,
					   1 << FMINLEN, 0 << FMINLEN);
			regmap_update_bits(regmap, SUNXI_ADC1_REG,
					   1 << LINEINLEN, 1 << LINEINLEN);
		} else {
			SND_LOG_ERR("input1 mux val(%ld) invailed\n",
				    ucontrol->value.integer.value[0]);
			return -EINVAL;
		}
		break;
	case INPUT2_SHIFT:
		if (ucontrol->value.integer.value[0] == 0) {
			regmap_update_bits(regmap, SUNXI_ADC2_REG,
					   1 << MIC2_PGA_EN, 0 << MIC2_PGA_EN);
			regmap_update_bits(regmap, SUNXI_ADC2_REG,
					   1 << FMINREN, 0 << FMINREN);
			regmap_update_bits(regmap, SUNXI_ADC2_REG,
					   1 << LINEINREN, 0 << LINEINREN);
		} else if (ucontrol->value.integer.value[0] == 1) {
			regmap_update_bits(regmap, SUNXI_ADC2_REG,
					   1 << MIC2_PGA_EN, 1 << MIC2_PGA_EN);
			regmap_update_bits(regmap, SUNXI_ADC2_REG,
					   1 << FMINREN, 0 << FMINREN);
			regmap_update_bits(regmap, SUNXI_ADC2_REG,
					   1 << LINEINREN, 0 << LINEINREN);
		} else if (ucontrol->value.integer.value[0] == 2) {
			regmap_update_bits(regmap, SUNXI_ADC2_REG,
					   1 << MIC2_PGA_EN, 0 << MIC2_PGA_EN);
			regmap_update_bits(regmap, SUNXI_ADC2_REG,
					   1 << FMINREN, 1 << FMINREN);
			regmap_update_bits(regmap, SUNXI_ADC2_REG,
					   1 << LINEINREN, 0 << LINEINREN);
		} else if (ucontrol->value.integer.value[0] == 3) {
			regmap_update_bits(regmap, SUNXI_ADC2_REG,
					   1 << MIC2_PGA_EN, 0 << MIC2_PGA_EN);
			regmap_update_bits(regmap, SUNXI_ADC2_REG,
					   1 << FMINREN, 0 << FMINREN);
			regmap_update_bits(regmap, SUNXI_ADC2_REG,
					   1 << LINEINREN, 1 << LINEINREN);
		} else {
			SND_LOG_ERR("input2 mux val(%ld) invailed\n",
				    ucontrol->value.integer.value[0]);
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	snd_soc_dapm_mux_update_power(dapm, kcontrol, ucontrol->value.enumerated.item[0], e, NULL);

	return 0;
}

static int sunxi_playback_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
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

static int sunxi_lineoutl_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	unsigned int reg_val;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_read(regmap, SUNXI_RAMP_REG, &reg_val);
		if (!(reg_val & (0x1 << RD_EN)))
			regmap_update_bits(regmap, SUNXI_RAMP_REG, 0x1 << RMC_EN, 0x1 << RMC_EN);
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << DACLMUTE, 0x1 << DACLMUTE);
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << LINEOUTLEN, 0x1 << LINEOUTLEN);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << LINEOUTLEN, 0x0 << LINEOUTLEN);
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << DACLMUTE, 0x0 << DACLMUTE);
		regmap_read(regmap, SUNXI_RAMP_REG, &reg_val);
		if (!(reg_val & (0x1 << RD_EN)))
			regmap_update_bits(regmap, SUNXI_RAMP_REG, 0x1 << RMC_EN, 0x0 << RMC_EN);
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
	unsigned int reg_val;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_read(regmap, SUNXI_RAMP_REG, &reg_val);
		if (!(reg_val & (0x1 << RD_EN)))
			regmap_update_bits(regmap, SUNXI_RAMP_REG, 0x1 << RMC_EN, 0x1 << RMC_EN);
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << DACRMUTE, 0x1 << DACRMUTE);
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << LINEOUTREN, 0x1 << LINEOUTREN);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << LINEOUTREN, 0x0 << LINEOUTREN);
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << DACRMUTE, 0x0 << DACRMUTE);
		regmap_read(regmap, SUNXI_RAMP_REG, &reg_val);
		if (!(reg_val & (0x1 << RD_EN)))
			regmap_update_bits(regmap, SUNXI_RAMP_REG, 0x1 << RMC_EN, 0x0 << RMC_EN);
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
	unsigned int reg_val;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, SUNXI_RAMP_REG, 0x1 << RMC_EN, 0x0 << RMC_EN);
		regmap_update_bits(regmap, SUNXI_RAMP_REG, 0x1 << RAMP_SRST, 0x1 << RAMP_SRST);
		regmap_update_bits(regmap, SUNXI_RAMP_REG, 0x1 << RD_EN, 0x1 << RD_EN);
		msleep(100);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		regmap_read(regmap, SUNXI_DAC_REG, &reg_val);
		if ((reg_val & (0x1 << LINEOUTLEN)) || (reg_val & (0x1 << LINEOUTREN)))
			regmap_update_bits(regmap, SUNXI_RAMP_REG, 0x1 << RMC_EN, 0x1 << RMC_EN);
		regmap_update_bits(regmap, SUNXI_RAMP_REG, 0x1 << RD_EN, 0x0 << RD_EN);
		break;
	}

	return 0;
}

static int sunxi_adc1_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, SUNXI_ADC_DIG_CTRL,
				   0x1 << ADC1_CHANNEL_EN, 0x1 << ADC1_CHANNEL_EN);
		regmap_update_bits(regmap, SUNXI_ADC1_REG, 0x1 << ADC1_EN, 0x1 << ADC1_EN);
		mutex_lock(&codec->audio_sta.acf_mutex);
		codec->audio_sta.adc1 = true;
		if (codec->audio_sta.adc2 == false) {
			regmap_update_bits(regmap, SUNXI_ADC2_REG,
					   1 << CROSSTALK_AVOID_EN, 1 << CROSSTALK_AVOID_EN);
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x1 << EN_AD, 0x1 << EN_AD);
		}
		mutex_unlock(&codec->audio_sta.acf_mutex);
		break;
	case SND_SOC_DAPM_POST_PMD:
		mutex_lock(&codec->audio_sta.acf_mutex);
		codec->audio_sta.adc1 = false;
		if (codec->audio_sta.adc2 == false) {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x1 << EN_AD, 0x0 << EN_AD);
			regmap_update_bits(regmap, SUNXI_ADC2_REG,
					   1 << CROSSTALK_AVOID_EN, 0 << CROSSTALK_AVOID_EN);
		}
		mutex_unlock(&codec->audio_sta.acf_mutex);
		regmap_update_bits(regmap, SUNXI_ADC1_REG, 0x1 << ADC1_EN, 0x0 << ADC1_EN);
		regmap_update_bits(regmap, SUNXI_ADC_DIG_CTRL,
				   0x1 << ADC1_CHANNEL_EN, 0x0 << ADC1_CHANNEL_EN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_adc2_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, SUNXI_ADC_DIG_CTRL,
				   0x1 << ADC2_CHANNEL_EN, 0x1 << ADC2_CHANNEL_EN);
		regmap_update_bits(regmap, SUNXI_ADC2_REG,
				   0x1 << ADC2_EN, 0x1 << ADC2_EN);
		mutex_lock(&codec->audio_sta.acf_mutex);
		codec->audio_sta.adc2 = true;
		if (codec->audio_sta.adc1 == false) {
			regmap_update_bits(regmap, SUNXI_ADC2_REG,
					   1 << CROSSTALK_AVOID_EN, 1 << CROSSTALK_AVOID_EN);
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x1 << EN_AD, 0x1 << EN_AD);
		}
		mutex_unlock(&codec->audio_sta.acf_mutex);
		break;
	case SND_SOC_DAPM_POST_PMD:
		mutex_lock(&codec->audio_sta.acf_mutex);
		codec->audio_sta.adc2 = false;
		if (codec->audio_sta.adc1 == false) {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x1 << EN_AD, 0x0 << EN_AD);
			regmap_update_bits(regmap, SUNXI_ADC2_REG,
					   1 << CROSSTALK_AVOID_EN, 0 << CROSSTALK_AVOID_EN);
		}
		mutex_unlock(&codec->audio_sta.acf_mutex);
		regmap_update_bits(regmap, SUNXI_ADC2_REG,
				   0x1 << ADC2_EN, 0x0 << ADC2_EN);
		regmap_update_bits(regmap, SUNXI_ADC_DIG_CTRL,
				   0x1 << ADC2_CHANNEL_EN, 0x0 << ADC2_CHANNEL_EN);
		break;
	default:
		break;
	}

	return 0;
}

/* Define Mux Controls */
static const struct soc_enum input1_mux_enum =
	SOC_ENUM_SINGLE(SND_SOC_NOPM, INPUT1_SHIFT, 4, sunxi_input1_mux_text);
static const struct soc_enum input2_mux_enum =
	SOC_ENUM_SINGLE(SND_SOC_NOPM, INPUT2_SHIFT, 4, sunxi_input2_mux_text);

static const struct snd_kcontrol_new input1_mux =
	SOC_DAPM_ENUM_EXT("Input1 Mux", input1_mux_enum, sunxi_get_input_mux, sunxi_set_input_mux);

static const struct snd_kcontrol_new input2_mux =
	SOC_DAPM_ENUM_EXT("Input2 Mux", input2_mux_enum, sunxi_get_input_mux, sunxi_set_input_mux);

/*audio dapm widget */
static const struct snd_soc_dapm_widget sunxi_codec_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_IN_E("DACL", "Playback", 0, SUNXI_DAC_REG,
			      DACLEN, 0,
			      sunxi_playback_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_IN_E("DACR", "Playback", 0, SUNXI_DAC_REG,
			      DACREN, 0,
			      sunxi_playback_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_OUT_E("ADC1", "Capture", 0, SND_SOC_NOPM, 0, 0,
			       sunxi_adc1_event,
			       SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("ADC2", "Capture", 0, SND_SOC_NOPM, 0, 0,
			       sunxi_adc2_event,
			       SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MUX("Input1 Mux", SND_SOC_NOPM, 0, 0, &input1_mux),
	SND_SOC_DAPM_MUX("Input2 Mux", SND_SOC_NOPM, 0, 0, &input2_mux),

	SND_SOC_DAPM_OUTPUT("LINEOUTL_PIN"),
	SND_SOC_DAPM_OUTPUT("LINEOUTR_PIN"),
	SND_SOC_DAPM_OUTPUT("HPOUTL_PIN"),
	SND_SOC_DAPM_OUTPUT("HPOUTR_PIN"),

	SND_SOC_DAPM_INPUT("MIC1_PIN"),
	SND_SOC_DAPM_INPUT("MIC2_PIN"),
	SND_SOC_DAPM_INPUT("FMINL_PIN"),
	SND_SOC_DAPM_INPUT("FMINR_PIN"),
	SND_SOC_DAPM_INPUT("LINEINL_PIN"),
	SND_SOC_DAPM_INPUT("LINEINR_PIN"),

	SND_SOC_DAPM_MIC("MIC1", NULL),
	SND_SOC_DAPM_MIC("MIC2", NULL),
	SND_SOC_DAPM_LINE("LINEOUTL", sunxi_lineoutl_event),
	SND_SOC_DAPM_LINE("LINEOUTR", sunxi_lineoutr_event),
	SND_SOC_DAPM_LINE("FMINL", NULL),
	SND_SOC_DAPM_LINE("FMINR", NULL),
	SND_SOC_DAPM_LINE("LINEINL", NULL),
	SND_SOC_DAPM_LINE("LINEINR", NULL),
	SND_SOC_DAPM_HP("HPOUT", sunxi_hpout_event),
	SND_SOC_DAPM_SPK("SPK", sunxi_spk_event),
};

static const struct snd_soc_dapm_route sunxi_codec_dapm_routes[] = {
	/* DAC --> HPOUT_PIN*/
	{"HPOUTL_PIN", NULL, "DACL"},
	{"HPOUTR_PIN", NULL, "DACR"},

	/* DAC --> LINEOUT_PIN*/
	{"LINEOUTL_PIN", NULL, "DACL"},
	{"LINEOUTR_PIN", NULL, "DACR"},

	/*xxx_PIN --> ADC*/
	{"Input1 Mux", "MIC1", "MIC1_PIN"},
	{"Input1 Mux", "FMINL", "FMINL_PIN"},
	{"Input1 Mux", "LINEINL", "LINEINL_PIN"},

	{"Input2 Mux", "MIC2", "MIC2_PIN"},
	{"Input2 Mux", "FMINR", "FMINR_PIN"},
	{"Input2 Mux", "LINEINR", "LINEINR_PIN"},

	{"ADC1", NULL, "Input1 Mux"},
	{"ADC2", NULL, "Input2 Mux"},
};

/* jack work -> gpio */
static int sunxi_jack_status_sync(void *data, enum snd_jack_types jack_type);

struct sunxi_jack_gpio sunxi_jack_gpio = {
	.jack_status_sync	= sunxi_jack_status_sync,
};

static int sunxi_jack_status_sync(void *data, enum snd_jack_types jack_type)
{
	(void)data;

	SND_LOG_DEBUG("jack_type %d\n", jack_type);

	return 0;
}

struct sunxi_jack_port sunxi_jack_port = {
	.jack_gpio = &sunxi_jack_gpio,
};

static void sunxi_codec_init(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_dts *dts = &codec->dts;
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");


	regmap_update_bits(regmap, SUNXI_RAMP_REG, 0x1 << RMC_EN, 0x0 << RMC_EN);

	/* enable ADC volume control */
	regmap_update_bits(regmap, SUNXI_ADC_DIG_CTRL, 0x1 << ADC1_2_VOL_EN, 0x1 << ADC1_2_VOL_EN);
	/* Open DACL\R Volume Setting */
	regmap_update_bits(regmap, SUNXI_DAC_VOL_CTRL, 0x1 << DAC_VOL_SEL, 0x1 << DAC_VOL_SEL);
	/* Enable ADCFDT to overcome niose at the beginning */
	regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x1 << ADCDFEN, 0x1 << ADCDFEN);
	regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x3 << ADCFDT, 0x2 << ADCFDT);
	/* Set default volume/gain control value */
	regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x3F << DVOL, dts->dac_vol << DVOL);
	regmap_update_bits(regmap, SUNXI_DAC_VOL_CTRL,
			   0xFF << DAC_VOL_L, dts->dacl_vol << DAC_VOL_L);
	regmap_update_bits(regmap, SUNXI_DAC_VOL_CTRL,
			   0xFF << DAC_VOL_R, dts->dacr_vol << DAC_VOL_R);
	regmap_update_bits(regmap, SUNXI_DAC_REG,
			   0x7 << LINEOUT_VOL, dts->lineout_vol << LINEOUT_VOL);
	regmap_update_bits(regmap, SUNXI_HP2_REG,
			   0x7 << HEADPHONE_GAIN, dts->hpout_gain << HEADPHONE_GAIN);
	regmap_update_bits(regmap, SUNXI_ADC_VOL_CTRL,
			   0xFF << ADC1_VOL, dts->adc1_vol << ADC1_VOL);
	regmap_update_bits(regmap, SUNXI_ADC_VOL_CTRL,
			   0xFF << ADC2_VOL, dts->adc2_vol << ADC2_VOL);
	/* LINEIN/FMIN/MICIN gain unsupport 0db */
	regmap_update_bits(regmap, SUNXI_ADC1_REG,
			   0xF << ADC1_PGA_GAIN_CTRL, dts->adc1_gain << ADC1_PGA_GAIN_CTRL);
	regmap_update_bits(regmap, SUNXI_ADC2_REG,
			   0xF << ADC2_PGA_GAIN_CTRL, dts->adc2_gain << ADC2_PGA_GAIN_CTRL);
	regmap_update_bits(regmap, SUNXI_ADC1_REG,
			   0x3 << FMINLG, 0x3 << FMINLG);
	regmap_update_bits(regmap, SUNXI_ADC2_REG,
			   0x3 << FMINRG, 0x3 << FMINRG);
	regmap_update_bits(regmap, SUNXI_ADC1_REG,
			   0x3 << LINEINLG, 0x3 << LINEINLG);
	regmap_update_bits(regmap, SUNXI_ADC2_REG,
			   0x3 << LINEINRG, 0x3 << LINEINRG);
	/* enable RK_OPT_EN to avoid pop when headphone power on */
	regmap_update_bits(regmap, SUNXI_RAMP_REG, 1 << RK_OPT_EN, 1 << RK_OPT_EN);

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

	/* jack init -> gpio */
	sunxi_jack_gpio.pdev = codec->pdev;

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
	int ret;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	snd_sunxi_regulator_enable(codec->rglt);
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
static const struct regmap_config sunxi_regmap_config = {
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
						 resource_size(&mem->res),
						 DRV_NAME);
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
	clk->clk_bus = of_clk_get_by_name(np, "clk_bus_audio");
	if (IS_ERR_OR_NULL(clk->clk_bus)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_INIT, "clk_bus get failed\n");
		ret = PTR_ERR(clk->clk_bus);
		goto err_get_clk_bus;
	}

	/* get parent clk */
	clk->clk_pll_audio1_div5 = of_clk_get_by_name(np, "clk_pll_audio1_div5");
	if (IS_ERR_OR_NULL(clk->clk_pll_audio1_div5)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_INIT, "clk_pll_audio1_div5 get failed\n");
		ret = PTR_ERR(clk->clk_pll_audio1_div5);
		goto err_get_clk_pll_audio1_div5;
	}
	clk->clk_pll_audio0_1x = of_clk_get_by_name(np, "clk_pll_audio0_1x");
	if (IS_ERR_OR_NULL(clk->clk_pll_audio0_1x)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_INIT, "clk_pll_audio0_1x get failed\n");
		ret = PTR_ERR(clk->clk_pll_audio0_1x);
		goto err_get_clk_pll_audio0_1x;
	}

	/* get module clk */
	clk->clk_audio_dac = of_clk_get_by_name(np, "clk_audio_dac");
	if (IS_ERR_OR_NULL(clk->clk_audio_dac)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_INIT, "clk_audio_dac get failed\n");
		ret = PTR_ERR(clk->clk_audio_dac);
		goto err_get_clk_audio_dac;
	}

	clk->clk_audio_adc = of_clk_get_by_name(np, "clk_audio_adc");
	if (IS_ERR_OR_NULL(clk->clk_audio_adc)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_INIT, "clk_audio_adc get failed\n");
		ret = PTR_ERR(clk->clk_audio_adc);
		goto err_get_clk_audio_adc;
	}

	return 0;

err_get_clk_audio_adc:
	clk_put(clk->clk_audio_dac);
err_get_clk_audio_dac:
	clk_put(clk->clk_pll_audio0_1x);
err_get_clk_pll_audio0_1x:
	clk_put(clk->clk_pll_audio1_div5);
err_get_clk_pll_audio1_div5:
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
	clk_put(clk->clk_pll_audio0_1x);
	clk_put(clk->clk_pll_audio1_div5);
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

	if (clk_prepare_enable(clk->clk_pll_audio1_div5)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_EN, "clk_pll_audio1_div5 enable failed\n");
		goto err_enable_clk_pll_audio1_div5;
	}

	if (clk_prepare_enable(clk->clk_pll_audio0_1x)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_EN, "clk_pll_audio0_1x enable failed\n");
		goto err_enable_clk_pll_audio0_1x;
	}

	if (clk_prepare_enable(clk->clk_audio_dac)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_EN, "clk_audio_dac enable failed\n");
		goto err_enable_clk_audio_dac;
	}

	if (clk_prepare_enable(clk->clk_audio_adc)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_EN, "clk_audio_adc enable failed\n");
		goto err_enable_clk_audio_adc;
	}

	return 0;

err_enable_clk_audio_adc:
	clk_disable_unprepare(clk->clk_audio_dac);
err_enable_clk_audio_dac:
	clk_disable_unprepare(clk->clk_pll_audio0_1x);
err_enable_clk_pll_audio0_1x:
	clk_disable_unprepare(clk->clk_pll_audio1_div5);
err_enable_clk_pll_audio1_div5:
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
	clk_disable_unprepare(clk->clk_pll_audio1_div5);
	clk_disable_unprepare(clk->clk_pll_audio0_1x);
}

static int snd_sunxi_clk_rate(struct sunxi_codec_clk *clk, int stream,
			      unsigned int freq_in, unsigned int freq_out)
{
	SND_LOG_DEBUG("\n");

	if (stream  == SNDRV_PCM_STREAM_PLAYBACK) {
		if (freq_in % 24576000 == 0) {
			if (clk_set_parent(clk->clk_audio_dac, clk->clk_pll_audio1_div5)) {
				SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_SET,
						"set dac parent clk failed\n");
				return -EINVAL;
			}

			if (clk_set_rate(clk->clk_audio_dac, freq_out)) {
				SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_SET,
					"set clk_audio_dac rate failed, rate: %u\n", freq_out);
				return -EINVAL;
			}
		} else {
			if (clk_set_parent(clk->clk_audio_dac, clk->clk_pll_audio0_1x)) {
				SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_SET,
						"set dac parent clk failed\n");
				return -EINVAL;
			}

			if (clk_set_rate(clk->clk_audio_dac, freq_out)) {
				SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_SET,
					"set clk_audio_dac rate failed, rate: %u\n", freq_out);
				return -EINVAL;
			}
		}
	} else {
		if (freq_in % 24576000 == 0) {
			if (clk_set_parent(clk->clk_audio_adc, clk->clk_pll_audio1_div5)) {
				SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_SET,
						"set adc parent clk failed\n");
				return -EINVAL;
			}

			if (clk_set_rate(clk->clk_audio_adc, freq_out)) {
				SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_SET,
					"set clk_audio_adc rate failed, rate: %u\n", freq_out);
				return -EINVAL;
			}
		} else {
			if (clk_set_parent(clk->clk_audio_adc, clk->clk_pll_audio0_1x)) {
				SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_SET,
						"set adc parent clk failed\n");
				return -EINVAL;
			}

			if (clk_set_rate(clk->clk_audio_adc, freq_out)) {
				SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_SET,
					"set clk_audio_adc rate failed, rate: %u\n", freq_out);
				return -EINVAL;
			}
		}
	}

	return 0;
}

static void snd_sunxi_dts_params_init(struct platform_device *pdev, struct sunxi_codec_dts *dts)

{
	int ret = 0;
	unsigned int temp_val;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	dts->rx_sync_en = of_property_read_bool(np, "rx-sync-en");
	dts->tx_hub_en = of_property_read_bool(np, "tx-hub-en");

	ret = of_property_read_u32(np, "dac-vol", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("digital volume get failed, use default vol");
		dts->dac_vol = 0;
	} else {
		dts->dac_vol = 63 - temp_val;
	}

	ret = of_property_read_u32(np, "dacl-vol", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("dacl volume get failed, use default vol");
		dts->dacl_vol = 160;
	} else {
		dts->dacl_vol = temp_val;
	}

	ret = of_property_read_u32(np, "dacr-vol", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("dacr volume get failed, use default vol");
		dts->dacr_vol = 160;
	} else {
		dts->dacr_vol = temp_val;
	}

	ret = of_property_read_u32(np, "lineout-vol", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("lineout volume get failed, use default vol");
		dts->lineout_vol = 0;
	} else {
		dts->lineout_vol = temp_val;
	}

	ret = of_property_read_u32(np, "hpout-gain", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("hpout_gain volume get failed, use default vol");
		dts->hpout_gain = 0;
	} else {
		dts->hpout_gain = 7 - temp_val;
	}

	ret = of_property_read_u32(np, "adc1-vol", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("adc1_vol get failed, use default vol");
		dts->adc1_vol = 160;
	} else {
		dts->adc1_vol = temp_val;
	}
	ret = of_property_read_u32(np, "adc2-vol", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("adc2_vol get failed, use default vol");
		dts->adc2_vol = 160;
	} else {
		dts->adc2_vol = temp_val;
	}

	ret = of_property_read_u32(np, "adc1-gain", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("adc1_gain get failed, use default vol");
		dts->adc1_gain = 15;
	} else {
		if (temp_val == 0) {
			SND_LOG_WARN("adc1-gain unsupport 0db\n");
			dts->adc1_gain = 1;
		} else {
			dts->adc1_gain = temp_val;
		}
	}
	ret = of_property_read_u32(np, "adc2-gain", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("adc2_gain get failed, use default vol");
		dts->adc2_gain = 15;
	} else {
		if (temp_val == 0) {
			SND_LOG_WARN("adc2-gain unsupport 0db\n");
			dts->adc2_gain = 1;
		} else {
			dts->adc2_gain = temp_val;
		}
	}

	/* print volume & gain value */
	SND_LOG_DEBUG("dac_vol:%u, dacl_vol:%u, dacr_vol:%u\n",
		      dts->dac_vol, dts->dacl_vol, dts->dacr_vol);
	SND_LOG_DEBUG("adc1_vol:%u, adc2_vol:%u\n",
		      dts->adc1_vol, dts->adc2_vol);
	SND_LOG_DEBUG("lineout_vol:%u, hpout_gain:%u\n",
		      dts->lineout_vol, dts->hpout_gain);
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
	codec = devm_kzalloc(dev, sizeof(struct sunxi_codec), GFP_KERNEL);
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
	ret = snd_soc_register_component(dev,
					 &sunxi_codec_component_dev,
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

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	struct snd_sunxi_dump *dump = &codec->dump;
#endif

	SND_LOG_DEBUG("\n");

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	snd_sunxi_dump_unregister(dump);
#endif

	snd_soc_unregister_component(dev);

	snd_sunxi_mem_exit(pdev, mem);
	snd_sunxi_clk_bus_disable(clk);
	snd_sunxi_clk_exit(clk);
	snd_sunxi_regulator_exit(rglt);
	snd_sunxi_pa_pin_exit(codec->pa_cfg, codec->pa_pin_max);

	/* sunxi codec custom info free */
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

MODULE_AUTHOR("zhouxijing@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.1");
MODULE_ALIAS("sunxi soundcard codec of internal-codec");
