// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2022, lijingpsw <lijingpsw@allwinnertech.com>
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
#include "snd_sun8iw18_codec.h"

#define DRV_NAME	"sunxi-snd-codec"

static struct audio_reg_label sunxi_reg_labels[] = {
	REG_LABEL(SUNXI_DAC_DPC),
	REG_LABEL(SUNXI_DAC_FIFO_CTL),
	REG_LABEL(SUNXI_DAC_FIFO_STA),
	REG_LABEL(SUNXI_DAC_CNT),
	REG_LABEL(SUNXI_DAC_DG),
	REG_LABEL(SUNXI_ADC_FIFO_CTL),
	REG_LABEL(SUNXI_ADC_FIFO_STA),
	REG_LABEL(SUNXI_ADC_CNT),
	REG_LABEL(SUNXI_ADC_DG),
	REG_LABEL(SUNXI_DAC_DAP_CTL),
	REG_LABEL(SUNXI_ADC_DAP_CTL),

	REG_LABEL(SUNXI_HP_CTL),
	REG_LABEL(SUNXI_MIX_DAC_CTL),
	REG_LABEL(SUNXI_LINEOUT_CTL0),
	REG_LABEL(SUNXI_LINEOUT_CTL1),
	REG_LABEL(SUNXI_MIC1_CTL),
	REG_LABEL(SUNXI_MIC2_MIC3_CTL),

	REG_LABEL(SUNXI_LADCMIX_SRC),
	REG_LABEL(SUNXI_RADCMIX_SRC),
	REG_LABEL(SUNXI_XADCMIX_SRC),
	REG_LABEL(SUNXI_ADC_CTL),
	REG_LABEL(SUNXI_MBIAS_CTL),
	REG_LABEL(SUNXI_APT_REG),

	REG_LABEL(SUNXI_OP_BIAS_CTL0),
	REG_LABEL(SUNXI_OP_BIAS_CTL1),
	REG_LABEL(SUNXI_ZC_VOL_CTL),
	REG_LABEL(SUNXI_BIAS_CAL_CTRL),
};
static struct audio_reg_group sunxi_reg_group = REG_GROUP(sunxi_reg_labels);

struct sample_rate {
	unsigned int samplerate;
	unsigned int rate_bit;
};

static const struct sample_rate sample_rate_conv[] = {
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

static int snd_sunxi_rglt_init(struct platform_device *pdev, struct sunxi_codec_rglt *rglt);
static void snd_sunxi_rglt_exit(struct sunxi_codec_rglt *rglt);
static int snd_sunxi_rglt_enable(struct sunxi_codec_rglt *rglt);
static void snd_sunxi_rglt_disable(struct sunxi_codec_rglt *rglt);

static unsigned int sunxi_regmap_read_prcm(struct regmap *regmap, unsigned int reg)
{
	unsigned int addr;
	unsigned int reg_val;
	unsigned int val;

	SND_LOG_DEBUG("\n");

	addr = reg - SUNXI_PR_CFG;
	val = (0x1 << AC_PR_RST) | (addr << AC_PR_ADDR);

	regmap_write(regmap, SUNXI_PR_CFG, val);
	regmap_read(regmap, SUNXI_PR_CFG, &reg_val);

	reg_val &= 0xff;

	return reg_val;
}

static void sunxi_regmap_write_prcm(struct regmap *regmap, unsigned int reg, unsigned int reg_val)
{
	unsigned int addr;
	unsigned int val = 0;

	SND_LOG_DEBUG("\n");

	addr = reg - SUNXI_PR_CFG;
	reg_val &= 0xff;

	val = (0x1 << AC_PR_RST) | (0x1 << AC_PR_RW)
		| (addr << AC_PR_ADDR) | (reg_val << ADDA_PR_WDAT);
	regmap_write(regmap, SUNXI_PR_CFG, val);

	val &= ~(0x1 << AC_PR_RW);
	regmap_write(regmap, SUNXI_PR_CFG, val);
}

static void sunxi_regmap_update_prcm_bits(struct regmap *regmap, unsigned int reg,
					  unsigned int mask, unsigned int val)
{
	unsigned int old, new;

	SND_LOG_DEBUG("\n");

	old = sunxi_regmap_read_prcm(regmap, reg);
	new = (old & ~mask) | val;

	sunxi_regmap_write_prcm(regmap, reg, new);
}

static int sunxi_codec_dai_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *params,
				     struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	int i = 0;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
					   3 << FIFO_MODE, 3 << FIFO_MODE);
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
					   1 << TX_SAMPLE_BITS, 0 << TX_SAMPLE_BITS);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
					   1 << RX_FIFO_MODE, 1 << RX_FIFO_MODE);
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
					   1 << RX_SAMPLE_BITS, 0 << RX_SAMPLE_BITS);
		}
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
					   3 << FIFO_MODE, 0 << FIFO_MODE);
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
					   1 << TX_SAMPLE_BITS, 1 << TX_SAMPLE_BITS);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
					   1 << RX_FIFO_MODE, 0 << RX_FIFO_MODE);
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
					   1 << RX_SAMPLE_BITS, 1 << RX_SAMPLE_BITS);
		}
		break;
	default:
		SND_LOG_ERR("params_format[%d] error!\n",
			    params_format(params));
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(sample_rate_conv); i++) {
		if (sample_rate_conv[i].samplerate == params_rate(params)) {
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
						   0x7 << DAC_FS,
						   sample_rate_conv[i].rate_bit << DAC_FS);
			} else {
				if (sample_rate_conv[i].samplerate > 48000)
					return -EINVAL;
				regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
						   0x7 << ADC_FS,
						   sample_rate_conv[i].rate_bit << ADC_FS);
			}
		}
	}
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (params_channels(params)) {
		case 1:
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
					   1 << DAC_MONO_EN, 1 << DAC_MONO_EN);
			break;
		case 2:
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
					   1 << DAC_MONO_EN, 0 << DAC_MONO_EN);
			break;
		default:
			SND_LOG_WARN("cannot support the channels:%u.\n",
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

static int sunxi_codec_dai_prepare(struct snd_pcm_substream *substream,
				   struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
				   1 << FIFO_FLUSH, 1 << FIFO_FLUSH);
		regmap_write(regmap, SUNXI_DAC_FIFO_STA,
			     1 << DAC_TXE_INT | 1 << DAC_TXU_INT | 1 << DAC_TXO_INT);
		regmap_write(regmap, SUNXI_DAC_CNT, 0);
	} else {
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
				   1 << ADC_FIFO_FLUSH, 1 << ADC_FIFO_FLUSH);
		regmap_write(regmap, SUNXI_ADC_FIFO_STA,
			     1 << ADC_RXA_INT | 1 << ADC_RXO_INT);
		regmap_write(regmap, SUNXI_ADC_CNT, 0);
	}

	return 0;
}

static int sunxi_codec_dai_trigger(struct snd_pcm_substream *substream,
				   int cmd, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
					   1 << DAC_DRQ_EN, 1 << DAC_DRQ_EN);
		else
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
					   1 << ADC_DRQ_EN, 1 << ADC_DRQ_EN);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
					   1 << DAC_DRQ_EN, 0 << DAC_DRQ_EN);
		else
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
					   1 << ADC_DRQ_EN, 0 << ADC_DRQ_EN);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

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

static const struct snd_soc_dai_ops sunxi_codec_dai_ops = {
	.set_pll	= sunxi_codec_dai_set_pll,
	.hw_params	= sunxi_codec_dai_hw_params,
	.prepare	= sunxi_codec_dai_prepare,
	.trigger	= sunxi_codec_dai_trigger,
	.hw_free	= sunxi_codec_dai_hw_free,
};

static struct snd_soc_dai_driver sunxi_codec_dai = {
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates	= SNDRV_PCM_RATE_8000_192000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 3,
		.rates = SNDRV_PCM_RATE_8000_48000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S24_LE,
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

		regmap_update_bits(regmap, SUNXI_DAC_DPC,
				   0x1 << DAC_HUB_EN, 0x0 << DAC_HUB_EN);
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_DAC_DPC,
				   0x1 << DAC_HUB_EN, 0x1 << DAC_HUB_EN);

		/* TODO: enable tx route */
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const char *sunxi_switch_text[] = {"Off", "On"};
static const char *sunxi_differ_text[] = {"single", "differ"};

static SOC_ENUM_SINGLE_EXT_DECL(sunxi_tx_hub_mode_enum, sunxi_switch_text);
static const struct snd_kcontrol_new sunxi_tx_hub_controls[] = {
	SOC_ENUM_EXT("tx hub mode", sunxi_tx_hub_mode_enum,
		     sunxi_get_tx_hub_mode, sunxi_set_tx_hub_mode),
};

static SOC_ENUM_SINGLE_EXT_DECL(sunxi_dacdrc_mode_enum, sunxi_switch_text);
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_adcdrc_mode_enum, sunxi_switch_text);
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_dachpf_mode_enum, sunxi_switch_text);
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_adchpf_mode_enum, sunxi_switch_text);
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_lineout_select_enum, sunxi_differ_text);
static SOC_ENUM_SINGLE_DECL(sunxi_dac_swap_enum, SUNXI_DAC_DG, DA_SWP, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_adc_swap_enum, SUNXI_ADC_DG, AD_SWP, sunxi_switch_text);


static const DECLARE_TLV_DB_SCALE(digital_tlv, -7424, 116, 1);
static const DECLARE_TLV_DB_SCALE(adc_gain_tlv, -450, 150, 0);

static const unsigned int lineout_vol_tlv[] = {
	TLV_DB_RANGE_HEAD(2),
	0, 0, TLV_DB_SCALE_ITEM(0, 0, 1),
	1, 31, TLV_DB_SCALE_ITEM(-4350, 150, 1),
};

static const unsigned int mic_gain_tlv[] = {
	TLV_DB_RANGE_HEAD(2),
	0, 0, TLV_DB_SCALE_ITEM(0, 0, 0),
	1, 7, TLV_DB_SCALE_ITEM(1500, 300, 0),
};

static int sunxi_playback_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, SUNXI_DAC_DPC,
				   0x1 << EN_DAC, 0x1 << EN_DAC);
		/* time delay to wait digital dac work fine */
		msleep(10);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, SUNXI_DAC_DPC,
				   0x1 << EN_DAC, 0x0 << EN_DAC);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_capture_event(struct snd_soc_dapm_widget *w,
			       struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
				   0x1 << EN_AD, 0x1 << EN_AD);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
				   0x1 << EN_AD, 0x0 << EN_AD);
		break;
	}

	return 0;
}

static int sunxi_input_mixer1_event(struct snd_soc_dapm_widget *w,
				    struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
				   0x1 << ADCL_CHAN_SEL, 0x1 << ADCL_CHAN_SEL);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
				   0x1 << ADCL_CHAN_SEL, 0x0 << ADCL_CHAN_SEL);
		break;
	default:
		break;
	}
	return 0;
}

static int sunxi_input_mixer2_event(struct snd_soc_dapm_widget *w,
				    struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
				   0x1 << ADCR_CHAN_SEL, 0x1 << ADCR_CHAN_SEL);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
				   0x1 << ADCR_CHAN_SEL, 0x0 << ADCR_CHAN_SEL);
		break;
	default:
		break;
	}
	return 0;
}

static int sunxi_input_mixer3_event(struct snd_soc_dapm_widget *w,
				    struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
				   0x1 << ADCX_CHAN_SEL, 0x1 << ADCX_CHAN_SEL);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL,
				   0x1 << ADCX_CHAN_SEL, 0x0 << ADCX_CHAN_SEL);
		break;
	default:
		break;
	}
	return 0;
}


static int sunxi_spk_event(struct snd_soc_dapm_widget *w,
			   struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		snd_sunxi_pa_pin_enable(codec->pa_cfg, codec->pa_pin_max);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		snd_sunxi_pa_pin_disable(codec->pa_cfg, codec->pa_pin_max);
		break;
	default:
		break;
	}
	return 0;
}

static int sunxi_lineout_event(struct snd_soc_dapm_widget *w,
			       struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (event) {
	case	SND_SOC_DAPM_POST_PMU:
		sunxi_regmap_update_prcm_bits(regmap, SUNXI_LINEOUT_CTL0,
					      0x1 << LINEOUTL_EN | 0x1 << LINEOUTR_EN |
					      0x1 << LINEOUTL_SRC,
					      0x1 << LINEOUTL_EN | 0x1 << LINEOUTR_EN |
					      0x0 << LINEOUTL_SRC);
		break;
	case	SND_SOC_DAPM_PRE_PMD:
		sunxi_regmap_update_prcm_bits(regmap, SUNXI_LINEOUT_CTL0,
					      0x1 << LINEOUTL_EN | 0x1 << LINEOUTR_EN |
					      0x1 << LINEOUTL_SRC,
					      0x0 << LINEOUTL_EN | 0x0 << LINEOUTR_EN |
					      0x1 << LINEOUTL_SRC);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_mic1_event(struct snd_soc_dapm_widget *w,
			    struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		mutex_lock(&codec->audio_sts.mic_mutex);
		codec->audio_sts.mic1 = true;
		if (!(codec->audio_sts.mic2 || codec->audio_sts.mic3))
			sunxi_regmap_update_prcm_bits(regmap, SUNXI_MBIAS_CTL,
						      0x1 << MMICBIASEN, 0x1 << MMICBIASEN);
		mutex_unlock(&codec->audio_sts.mic_mutex);
		sunxi_regmap_update_prcm_bits(regmap, SUNXI_MIC1_CTL,
					      0x1 << MIC1AMPEN, 0x1 << MIC1AMPEN);
		break;
	case SND_SOC_DAPM_POST_PMD:
		mutex_lock(&codec->audio_sts.mic_mutex);
		codec->audio_sts.mic1 = false;
		if (!(codec->audio_sts.mic2 || codec->audio_sts.mic3))
			sunxi_regmap_update_prcm_bits(regmap, SUNXI_MBIAS_CTL,
						      0x1 << MMICBIASEN, 0x0 << MMICBIASEN);
		mutex_unlock(&codec->audio_sts.mic_mutex);

		sunxi_regmap_update_prcm_bits(regmap, SUNXI_MIC1_CTL,
					      0x1 << MIC1AMPEN, 0x0 << MIC1AMPEN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_mic2_event(struct snd_soc_dapm_widget *w,
			    struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		mutex_lock(&codec->audio_sts.mic_mutex);
		codec->audio_sts.mic2 = true;
		if (!(codec->audio_sts.mic1 || codec->audio_sts.mic3))
			sunxi_regmap_update_prcm_bits(regmap, SUNXI_MBIAS_CTL,
						      0x1 << MMICBIASEN, 0x1 << MMICBIASEN);
		mutex_unlock(&codec->audio_sts.mic_mutex);

		sunxi_regmap_update_prcm_bits(regmap, SUNXI_MIC2_MIC3_CTL,
					      0x1 << MIC2AMPEN, 0x1 << MIC2AMPEN);
		break;
	case SND_SOC_DAPM_POST_PMD:
		mutex_lock(&codec->audio_sts.mic_mutex);
		codec->audio_sts.mic2 = false;
		if (!(codec->audio_sts.mic1 || codec->audio_sts.mic3))
			sunxi_regmap_update_prcm_bits(regmap, SUNXI_MBIAS_CTL,
						      0x1 << MMICBIASEN, 0x0 << MMICBIASEN);
		mutex_unlock(&codec->audio_sts.mic_mutex);

		sunxi_regmap_update_prcm_bits(regmap, SUNXI_MIC2_MIC3_CTL,
					      0x1 << MIC2AMPEN, 0x0 << MIC2AMPEN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_mic3_event(struct snd_soc_dapm_widget *w,
			    struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		mutex_lock(&codec->audio_sts.mic_mutex);
		codec->audio_sts.mic3 = true;
		if (!(codec->audio_sts.mic1 || codec->audio_sts.mic2))
			sunxi_regmap_update_prcm_bits(regmap, SUNXI_MBIAS_CTL,
						      0x1 << MMICBIASEN, 0x1 << MMICBIASEN);
		mutex_unlock(&codec->audio_sts.mic_mutex);

		sunxi_regmap_update_prcm_bits(regmap, SUNXI_MIC2_MIC3_CTL,
					      0x1 << MIC3AMPEN, 0x1 << MIC3AMPEN);
		break;
	case SND_SOC_DAPM_POST_PMD:
		mutex_lock(&codec->audio_sts.mic_mutex);
		codec->audio_sts.mic3 = false;
		if (!(codec->audio_sts.mic1 || codec->audio_sts.mic2))
			sunxi_regmap_update_prcm_bits(regmap, SUNXI_MBIAS_CTL,
						      0x1 << MMICBIASEN, 0x0 << MMICBIASEN);
		mutex_unlock(&codec->audio_sts.mic_mutex);

		sunxi_regmap_update_prcm_bits(regmap, SUNXI_MIC2_MIC3_CTL,
					      0x1 << MIC3AMPEN, 0x0 << MIC3AMPEN);
		break;
	default:
		break;
	}

	return 0;
}

/* DAC&ADC-DRC&HPF FUNC */
/* DAC DRC */
static void dacdrc_enable(struct sunxi_codec *codec, bool on)
{
	struct sunxi_codec_dap *dac_dap = &codec->dac_dap;
	struct regmap *regmap = codec->mem.regmap;

	mutex_lock(&dac_dap->mutex);
	if (on) {
		/* detect noise when ET enable */
		regmap_update_bits(regmap, AC_DAC_DRC_CTL,
				   0x1 << DAC_DRC_CTL_CONTROL_DRC_EN,
				   0x1 << DAC_DRC_CTL_CONTROL_DRC_EN);
		/* 0x0:RMS filter; 0x1:Peak filter */
		regmap_update_bits(regmap, AC_DAC_DRC_CTL,
				   0x1 << DAC_DRC_CTL_SIGNAL_FUN_SEL,
				   0x1 << DAC_DRC_CTL_SIGNAL_FUN_SEL);
		/* delay function enable */
		regmap_update_bits(regmap, AC_DAC_DRC_CTL,
				    0x1 << DAC_DRC_CTL_DEL_FUN_EN,
				    0x0 << DAC_DRC_CTL_DEL_FUN_EN);
		/* LT enable */
		regmap_update_bits(regmap, AC_DAC_DRC_CTL,
				   0x1 << DAC_DRC_CTL_DRC_LT_EN,
				   0x1 << DAC_DRC_CTL_DRC_LT_EN);
		/* ET enable */
		regmap_update_bits(regmap, AC_DAC_DRC_CTL,
				   0x1 << DAC_DRC_CTL_DRC_ET_EN,
				   0x1 << DAC_DRC_CTL_DRC_ET_EN);

		regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
				   0x1 << DDAP_DRC_EN,
				   0x1 << DDAP_DRC_EN);

		if (dac_dap->dap_enable == 0)
			regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
					   0x1 << DDAP_EN, 0x1 << DDAP_EN);

		dac_dap->dap_enable = 1;

	} else {
		if (dac_dap->dap_enable == 1)
			regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
					   0x1 << DDAP_EN,
					   0x0 << DDAP_EN);
		dac_dap->dap_enable = 0;

		regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
				   0x1 << DDAP_DRC_EN,
				   0x0 << DDAP_DRC_EN);

		/* detect noise when ET enable */
		regmap_update_bits(regmap, AC_DAC_DRC_CTL,
				   0x1 << DAC_DRC_CTL_CONTROL_DRC_EN,
				   0x0 << DAC_DRC_CTL_CONTROL_DRC_EN);
		/* 0x0:RMS filter; 0x1:Peak filter */
		regmap_update_bits(regmap, AC_DAC_DRC_CTL,
				   0x1 << DAC_DRC_CTL_SIGNAL_FUN_SEL,
				   0x1 << DAC_DRC_CTL_SIGNAL_FUN_SEL);
		/* delay function enable */
		regmap_update_bits(regmap, AC_DAC_DRC_CTL,
				   0x1 << DAC_DRC_CTL_DEL_FUN_EN,
				   0x0 << DAC_DRC_CTL_DEL_FUN_EN);

		regmap_update_bits(regmap, AC_DAC_DRC_CTL,
				   0x1 << DAC_DRC_CTL_DRC_LT_EN,
				   0x0 << DAC_DRC_CTL_DRC_LT_EN);
		regmap_update_bits(regmap, AC_DAC_DRC_CTL,
				   0x1 << DAC_DRC_CTL_DRC_ET_EN,
				   0x0 << DAC_DRC_CTL_DRC_ET_EN);
	}
	mutex_unlock(&dac_dap->mutex);
}

static int sunxi_codec_get_dacdrc_mode(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_DAC_DAP_CTL, &reg_val);

	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << DDAP_DRC_EN)) ? 1 : 0);

	return 0;
}

static int sunxi_codec_set_dacdrc_mode(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		dacdrc_enable(codec, 0);
		break;
	case 1:
		dacdrc_enable(codec, 1);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

/* ADC DRC */
static void adcdrc_enable(struct sunxi_codec *codec, bool on)
{
	struct sunxi_codec_dap *adc_dap = &codec->adc_dap;
	struct regmap *regmap = codec->mem.regmap;

	mutex_lock(&adc_dap->mutex);
	if (on) {
		regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
				   0x1 << ADC_DRC0_EN | 0x1 << ADC_DRC1_EN,
				   0x1 << ADC_DRC0_EN | 0x1 << ADC_DRC1_EN);

		if (adc_dap->dap_enable == 0)
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_DAP0_EN | 0x1 << ADC_DAP1_EN,
					   0x1 << ADC_DAP0_EN | 0x1 << ADC_DAP1_EN);

		adc_dap->dap_enable = 1;
	} else {
		if (adc_dap->dap_enable == 1)
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_DAP0_EN | 0x1 << ADC_DAP1_EN,
					   0x0 << ADC_DAP0_EN | 0x0 << ADC_DAP1_EN);

		adc_dap->dap_enable = 0;

		regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
				   0x1 << ADC_DRC0_EN | 0x1 << ADC_DRC1_EN,
				   0x0 << ADC_DRC0_EN | 0x0 << ADC_DRC1_EN);
	}
	mutex_unlock(&adc_dap->mutex);
}

static int sunxi_codec_get_adcdrc_mode(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_ADC_DAP_CTL, &reg_val);

	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << ADC_DRC0_EN)) ? 1 : 0);

	return 0;
}

static int sunxi_codec_set_adcdrc_mode(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		adcdrc_enable(codec, 0);
		break;
	case 1:
		adcdrc_enable(codec, 1);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void dachpf_enable(struct sunxi_codec *codec, bool on)
{
	struct sunxi_codec_dap *dac_dap = &codec->dac_dap;
	struct regmap *regmap = codec->mem.regmap;

	mutex_lock(&dac_dap->mutex);
	if (on) {
		regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
				   0x1 << DDAP_HPF_EN, 0x1 << DDAP_HPF_EN);

		if (dac_dap->dap_enable == 0)
			regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
					   0x1 << DDAP_EN, 0x1 << DDAP_EN);

		dac_dap->dap_enable = 1;
	} else {
		if (dac_dap->dap_enable == 1)
			regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
					   0x1 << DDAP_EN, 0x0 << DDAP_EN);

		dac_dap->dap_enable = 0;

		regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
				   0x1 << DDAP_HPF_EN, 0x0 << DDAP_HPF_EN);

	}
	mutex_unlock(&dac_dap->mutex);
}

static int sunxi_codec_get_dachpf_mode(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_DAC_DAP_CTL, &reg_val);

	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << DDAP_HPF_EN)) ? 1 : 0);

	return 0;
}

static int sunxi_codec_set_dachpf_mode(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		dachpf_enable(codec, 0);
		break;
	case 1:
		dachpf_enable(codec, 1);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void adchpf_enable(struct sunxi_codec *codec, bool on)
{
	struct sunxi_codec_dap *adc_dap = &codec->adc_dap;
	struct regmap *regmap = codec->mem.regmap;

	mutex_lock(&adc_dap->mutex);
	if (on) {
		regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
				   0x1 << ADC_HPF0_EN | 0x1 << ADC_HPF1_EN,
				   0x1 << ADC_HPF0_EN | 0x1 << ADC_HPF1_EN);

		if (adc_dap->dap_enable == 0)
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_DAP0_EN | 0x1 << ADC_DAP1_EN,
					   0x1 << ADC_DAP0_EN | 0x1 << ADC_DAP1_EN);

		adc_dap->dap_enable = 1;
	} else {
		if (adc_dap->dap_enable == 1)
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_DAP0_EN | 0x1 << ADC_DAP1_EN,
					   0x0 << ADC_DAP0_EN | 0x0 << ADC_DAP1_EN);

		adc_dap->dap_enable = 0;

		regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
				   0x1 << ADC_HPF0_EN | 0x1 << ADC_HPF1_EN,
				   0x0 << ADC_HPF0_EN | 0x0 << ADC_HPF1_EN);
	}
	mutex_unlock(&adc_dap->mutex);
}

static int sunxi_codec_get_adchpf_mode(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_ADC_DAP_CTL, &reg_val);

	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << ADC_HPF0_EN)) ? 1 : 0);

	return 0;
}

static int sunxi_codec_set_adchpf_mode(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		adchpf_enable(codec, 0);
		break;
	case 1:
		adchpf_enable(codec, 1);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}


static void lineout_enable(struct sunxi_codec *codec, bool on)
{
	struct regmap *regmap = codec->mem.regmap;

	if (on)
		sunxi_regmap_update_prcm_bits(regmap, SUNXI_LINEOUT_CTL0,
					      0x1 <<  LINEOUTR_SRC,
					      0x1 <<  LINEOUTR_SRC);
	else
		sunxi_regmap_update_prcm_bits(regmap, SUNXI_LINEOUT_CTL0,
					      0x1 <<  LINEOUTR_SRC,
					      0x0 <<  LINEOUTR_SRC);
}

static int sunxi_codec_get_lineout_select(struct snd_kcontrol *kcontrol,
					  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	unsigned int reg_val;

	reg_val = sunxi_regmap_read_prcm(regmap, SUNXI_LINEOUT_CTL0);

	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << LINEOUTR_SRC)) ? 1 : 0);

	return 0;
}

static int sunxi_codec_set_lineout_select(struct snd_kcontrol *kcontrol,
					  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		lineout_enable(codec, 0);
		break;
	case 1:
		lineout_enable(codec, 1);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

struct snd_kcontrol_new sunxi_codec_controls[] = {
	/* DAP func */
	SOC_ENUM_EXT("ADCDRC", sunxi_adcdrc_mode_enum,
		     sunxi_codec_get_adcdrc_mode, sunxi_codec_set_adcdrc_mode),
	SOC_ENUM_EXT("ADCHPF", sunxi_adchpf_mode_enum,
		     sunxi_codec_get_adchpf_mode, sunxi_codec_set_adchpf_mode),
	SOC_ENUM_EXT("DACDRC", sunxi_dacdrc_mode_enum,
		     sunxi_codec_get_dacdrc_mode, sunxi_codec_set_dacdrc_mode),
	SOC_ENUM_EXT("DACHPF", sunxi_dachpf_mode_enum,
		     sunxi_codec_get_dachpf_mode, sunxi_codec_set_dachpf_mode),

	/* Chanel swap */
	SOC_ENUM("ADC Swap", sunxi_adc_swap_enum),
	SOC_ENUM("DAC Swap", sunxi_dac_swap_enum),

	/* lineout select */
	SOC_ENUM_EXT("LINEOUT Select", sunxi_lineout_select_enum,
		     sunxi_codec_get_lineout_select, sunxi_codec_set_lineout_select),

	/* Volume set */
	SOC_SINGLE_TLV("DAC Volume", SUNXI_DAC_DPC, DVOL, 0x3F, 1, digital_tlv),
	SOC_SINGLE_TLV("ADC Gain", SUNXI_ADC_CTL, ADCG, 0x7, 0, adc_gain_tlv),
	SOC_SINGLE_TLV("LINEOUT Volume", SUNXI_LINEOUT_CTL1, LINEOUT_VOL, 0x1F, 0, lineout_vol_tlv),
	SOC_SINGLE_TLV("MIC1 Gain", SUNXI_MIC1_CTL, MIC1BOOST, 0x7, 0, mic_gain_tlv),
	SOC_SINGLE_TLV("MIC2 Gain", SUNXI_MIC2_MIC3_CTL, MIC2BOOST, 0x7, 0, mic_gain_tlv),
	SOC_SINGLE_TLV("MIC3 Gain", SUNXI_MIC2_MIC3_CTL, MIC3BOOST, 0x7, 0, mic_gain_tlv),
};

/* mixer */
static const struct snd_kcontrol_new input_mixer1[] = {
	SOC_DAPM_SINGLE("MIC1 Switch", SUNXI_LADCMIX_SRC, LADC_MIC1_BST, 1, 0),
	SOC_DAPM_SINGLE("MIC2 Switch", SUNXI_LADCMIX_SRC, LADC_MIC2_BST, 1, 0),
	SOC_DAPM_SINGLE("MIC3 Switch", SUNXI_LADCMIX_SRC, LADC_MIC3_BST, 1, 0),
	SOC_DAPM_SINGLE("DACL Switch", SUNXI_LADCMIX_SRC, LADC_DACL, 1, 0),
};

static const struct snd_kcontrol_new input_mixer2[] = {
	SOC_DAPM_SINGLE("MIC1 Switch", SUNXI_RADCMIX_SRC, RADC_MIC1_BST, 1, 0),
	SOC_DAPM_SINGLE("MIC2 Switch", SUNXI_RADCMIX_SRC, RADC_MIC2_BST, 1, 0),
	SOC_DAPM_SINGLE("MIC3 Switch", SUNXI_RADCMIX_SRC, RADC_MIC3_BST, 1, 0),
	SOC_DAPM_SINGLE("DACL Switch", SUNXI_RADCMIX_SRC, RADC_DACL, 1, 0),
};

static const struct snd_kcontrol_new input_mixer3[] = {
	SOC_DAPM_SINGLE("MIC1 Switch", SUNXI_XADCMIX_SRC, XADC_MIC1_BST, 1, 0),
	SOC_DAPM_SINGLE("MIC2 Switch", SUNXI_XADCMIX_SRC, XADC_MIC2_BST, 1, 0),
	SOC_DAPM_SINGLE("MIC3 Switch", SUNXI_XADCMIX_SRC, XADC_MIC3_BST, 1, 0),
	SOC_DAPM_SINGLE("DACL Switch", SUNXI_XADCMIX_SRC, XADC_DACL, 1, 0),
};


static const struct snd_soc_dapm_widget sunxi_codec_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_IN_E("DACL", "Playback", 0, SUNXI_MIX_DAC_CTL, DACALEN, 0,
			      sunxi_playback_event,
			      SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("ADCL", "Capture", 0, SUNXI_ADC_CTL, ADCLEN, 0,
			       sunxi_capture_event,
			       SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("ADCR", "Capture", 0, SUNXI_ADC_CTL, ADCREN, 0,
			       sunxi_capture_event,
			       SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("ADCX", "Capture", 0, SUNXI_ADC_CTL, ADCXEN, 0,
			       sunxi_capture_event,
			       SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MIXER_E("Input Mixer1", SND_SOC_NOPM, 0, 0,
			     input_mixer1, ARRAY_SIZE(input_mixer1),
			     sunxi_input_mixer1_event,
			     SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MIXER_E("Input Mixer2", SND_SOC_NOPM, 0, 0,
			     input_mixer2, ARRAY_SIZE(input_mixer2),
			     sunxi_input_mixer2_event,
			     SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MIXER_E("Input Mixer3", SND_SOC_NOPM, 0, 0,
			     input_mixer3, ARRAY_SIZE(input_mixer3),
			     sunxi_input_mixer3_event,
			     SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_INPUT("MIC1_PIN"),
	SND_SOC_DAPM_INPUT("MIC2_PIN"),
	SND_SOC_DAPM_INPUT("MIC3_PIN"),
	SND_SOC_DAPM_OUTPUT("LINEOUT_PIN"),


	SND_SOC_DAPM_MIC("MIC1", sunxi_mic1_event),
	SND_SOC_DAPM_MIC("MIC2", sunxi_mic2_event),
	SND_SOC_DAPM_MIC("MIC3", sunxi_mic3_event),
	SND_SOC_DAPM_LINE("LINEOUT", sunxi_lineout_event),
	SND_SOC_DAPM_SPK("SPK", sunxi_spk_event),
};

static const struct snd_soc_dapm_route sunxi_codec_dapm_routes[] = {
	/* INPUT */
	{"Input Mixer1", "MIC1 Switch", "MIC1_PIN"},
	{"Input Mixer1", "MIC2 Switch", "MIC2_PIN"},
	{"Input Mixer1", "MIC3 Switch", "MIC3_PIN"},
	{"Input Mixer1", "DACL Switch", "DACL"},

	{"Input Mixer2", "MIC1 Switch", "MIC1_PIN"},
	{"Input Mixer2", "MIC2 Switch", "MIC2_PIN"},
	{"Input Mixer2", "MIC3 Switch", "MIC3_PIN"},
	{"Input Mixer2", "DACL Switch", "DACL"},

	{"Input Mixer3", "MIC1 Switch", "MIC1_PIN"},
	{"Input Mixer3", "MIC2 Switch", "MIC2_PIN"},
	{"Input Mixer3", "MIC3 Switch", "MIC3_PIN"},
	{"Input Mixer3", "DACL Switch", "DACL"},

	{"ADCL", NULL, "Input Mixer1"},
	{"ADCR", NULL, "Input Mixer2"},
	{"ADCX", NULL, "Input Mixer3"},

	/* OUTPUT */
	{"LINEOUT_PIN", NULL, "DACL"},

};

static void sunxi_codec_init(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_dts *dts = &codec->dts;
	struct regmap *regmap = codec->mem.regmap;

	/* Disable DRC function for playback */
	regmap_write(regmap, SUNXI_DAC_DAP_CTL, 0);

	/* Disable HPF(high passed filter) */
	regmap_update_bits(regmap, SUNXI_DAC_DPC, 1 << HPF_EN, 0x0 << HPF_EN);

	/* Enable ADCFDT to overcome niose at the beginning */
	regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTL, 7 << ADCDFEN, 7 << ADCDFEN);

	regmap_update_bits(regmap, SUNXI_DAC_DPC,
			   0x3f << DVOL, dts->dac_dig_vol << DVOL);

	sunxi_regmap_update_prcm_bits(regmap, SUNXI_MIC1_CTL,
				      0x7 << MIC1BOOST, dts->mic1_gain << MIC1BOOST);

	/* mic1 gain and mic2 gain default: 24dB */
	sunxi_regmap_write_prcm(regmap, SUNXI_MIC2_MIC3_CTL, 0x44);
	/* fix hardware, left adc must defautl init: 0x0 */
	sunxi_regmap_write_prcm(regmap, SUNXI_LADCMIX_SRC, 0x0);

	sunxi_regmap_update_prcm_bits(regmap, SUNXI_MIC2_MIC3_CTL,
				      0x7 << MIC2BOOST, dts->mic2_gain << MIC2BOOST);
	sunxi_regmap_update_prcm_bits(regmap, SUNXI_MIC2_MIC3_CTL,
				      0x7 << MIC3BOOST, dts->mic3_gain << MIC3BOOST);

	sunxi_regmap_update_prcm_bits(regmap, SUNXI_LINEOUT_CTL1,
				      0x1f << LINEOUT_VOL,
				      dts->lineout_vol << LINEOUT_VOL);

	if (dts->lineout_single)
		sunxi_regmap_update_prcm_bits(regmap, SUNXI_LINEOUT_CTL0,
					      0x1f << LINEOUTR_SRC, 0 << LINEOUTR_SRC);
	else
		sunxi_regmap_update_prcm_bits(regmap, SUNXI_LINEOUT_CTL0,
					      0x1f << LINEOUTR_SRC, 1 << LINEOUTR_SRC);

	sunxi_regmap_update_prcm_bits(regmap, SUNXI_ADC_CTL,
				      0x1f << ADCG, dts->adc_gain << ADCG);
}

static int sunxi_codec_component_probe(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_dts *dts = &codec->dts;
	int ret;

	SND_LOG_DEBUG("\n");

	codec->audio_sts.mic1 = false;
	codec->audio_sts.mic2 = false;
	codec->audio_sts.mic3 = false;
	mutex_init(&codec->audio_sts.mic_mutex);

	/* component kcontrols -> tx_hub */
	if (dts->tx_hub_en) {
		ret = snd_soc_add_component_controls(component, sunxi_tx_hub_controls,
						     ARRAY_SIZE(sunxi_tx_hub_controls));
		if (ret)
			SND_LOG_ERR("add tx_hub kcontrols failed\n");
	}

	/* component kcontrols -> pa */
	ret = snd_sunxi_pa_pin_probe(codec->pa_cfg, codec->pa_pin_max, component);
	if (ret)
		SND_LOG_ERR("register pa kcontrols failed\n");

	sunxi_codec_init(component);

	return 0;
}

static void sunxi_codec_component_remove(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	SND_LOG_DEBUG("\n");

	mutex_destroy(&codec->audio_sts.mic_mutex);

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
	int ret;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	snd_sunxi_pa_pin_disable(codec->pa_cfg, codec->pa_pin_max);

	snd_sunxi_rglt_enable(&codec->rglt);
	ret = snd_sunxi_clk_bus_enable(&codec->clk);
	if (ret) {
		SND_LOG_ERR("clk enable failed\n");
		return ret;
	}
	sunxi_codec_init(component);
	snd_sunxi_echo_reg(regmap, &sunxi_reg_group);

	return 0;
}

static unsigned int sunxi_codec_component_read(struct snd_soc_component *component,
					       unsigned int reg)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	unsigned int reg_val;

	SND_LOG_DEBUG("\n");

	if (reg >= SUNXI_PR_CFG) {
		/* Analog part */
		return sunxi_regmap_read_prcm(codec->mem.regmap, reg);
	} else
		regmap_read(codec->mem.regmap, reg, &reg_val);

	return reg_val;
}

static int sunxi_codec_component_write(struct snd_soc_component *component,
				unsigned int reg, unsigned int val)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	SND_LOG_DEBUG("\n");

	if (reg >= SUNXI_PR_CFG)
		/* Analog part */
		sunxi_regmap_write_prcm(codec->mem.regmap, reg, val);
	else
		regmap_write(codec->mem.regmap, reg, val);

	return 0;
}

static struct snd_soc_component_driver sunxi_codec_dev = {
	.name			= DRV_NAME,
	.probe			= sunxi_codec_component_probe,
	.remove			= sunxi_codec_component_remove,
	.suspend		= sunxi_codec_component_suspend,
	.resume			= sunxi_codec_component_resume,
	.read 			= sunxi_codec_component_read,
	.write 			= sunxi_codec_component_write,
	.controls		= sunxi_codec_controls,
	.num_controls		= ARRAY_SIZE(sunxi_codec_controls),
	.dapm_widgets		= sunxi_codec_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(sunxi_codec_dapm_widgets),
	.dapm_routes		= sunxi_codec_dapm_routes,
	.num_dapm_routes	= ARRAY_SIZE(sunxi_codec_dapm_routes),
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

	/* get module clk */
	clk->clk_audio = of_clk_get_by_name(np, "clk_audio_1x");
	if (IS_ERR_OR_NULL(clk->clk_bus_audio)) {
		SND_LOG_ERR("clk bus get failed\n");
		ret = PTR_ERR(clk->clk_bus_audio);
		goto err_get_clk_audio;
	}

	/* get parent clk */
	clk->clk_pll_audio = of_clk_get_by_name(np, "clk_pll_audio_4x");
	if (IS_ERR_OR_NULL(clk->clk_pll_audio)) {
		SND_LOG_ERR("clk_pll_audio_4x get failed\n");
		ret = PTR_ERR(clk->clk_pll_audio);
		goto err_get_clk_pll_audio;
	}

	if (clk_set_parent(clk->clk_audio, clk->clk_pll_audio)) {
		SND_LOG_ERR("set parent of  to  failed\n");
		ret = -EINVAL;
		goto err_set_parent;
	}

	return 0;

err_set_parent:
	clk_put(clk->clk_audio);
err_get_clk_audio:
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

	clk_put(clk->clk_audio);
	clk_put(clk->clk_pll_audio);
	clk_put(clk->clk_bus_audio);
}

static int snd_sunxi_clk_enable(struct sunxi_codec_clk *clk)
{
	int ret = 0;

	SND_LOG_DEBUG("\n");

	if (clk_prepare_enable(clk->clk_pll_audio)) {
		SND_LOG_ERR("clk_pll_audio enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_pll_audio;
	}

	if (clk_prepare_enable(clk->clk_audio)) {
		SND_LOG_ERR("clk_audio enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_audio;
	}

	return 0;

err_enable_clk_audio:
	clk_disable_unprepare(clk->clk_pll_audio);
err_enable_clk_pll_audio:
	return ret;
}

static int snd_sunxi_clk_bus_enable(struct sunxi_codec_clk *clk)
{
	int ret = 0;

	SND_LOG_DEBUG("\n");

	if (reset_control_deassert(clk->clk_rst)) {
		SND_LOG_ERR("clk_rst deassert failed\n");
		ret = -EINVAL;
		goto err_deassert_rst;
	}

	if (clk_prepare_enable(clk->clk_bus_audio)) {
		SND_LOG_ERR("clk_bus_audio enable failed\n");
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

	clk_disable_unprepare(clk->clk_audio);
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
		if (clk_set_rate(clk->clk_pll_audio, freq_in)) {
			SND_LOG_ERR("set clk_audio_dac rate failed, rate: %u\n", freq_out);
			return -EINVAL;
		}
	} else {
		if (clk_set_rate(clk->clk_pll_audio, freq_out)) {
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
		rglt->avcc = NULL;
		SND_LOG_DEBUG("unset external avcc\n");
		return 0;
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

	return 0;
}

static void snd_sunxi_rglt_disable(struct sunxi_codec_rglt *rglt)
{
	SND_LOG_DEBUG("\n");

	if (rglt->avcc)
		if (!IS_ERR_OR_NULL(rglt->avcc)) {
			regulator_disable(rglt->avcc);
		}
}

static void snd_sunxi_dts_params_init(struct platform_device *pdev, struct sunxi_codec_dts *dts)
{
	int ret = 0;
	unsigned int temp_val;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	/* input volume */
	ret = of_property_read_u32(np, "adc-gain", &temp_val);
	if (ret < 0)
		dts->adc_gain = 3;
	else
		dts->adc_gain = temp_val;

	ret = of_property_read_u32(np, "mic1-gain", &temp_val);
	if (ret < 0)
		dts->mic1_gain = 4;
	else
		dts->mic1_gain = temp_val;

	ret = of_property_read_u32(np, "mic2-gain", &temp_val);
	if (ret < 0)
		dts->mic2_gain = 4;
	else
		dts->mic2_gain = temp_val;

	ret = of_property_read_u32(np, "mic3-gain", &temp_val);
	if (ret < 0)
		dts->mic3_gain = 4;
	else
		dts->mic3_gain = temp_val;

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

	ret = of_property_read_u32(np, "lineout-vol", &temp_val);
	if (ret < 0)
		dts->lineout_vol = 31;
	else
		dts->lineout_vol = temp_val;

	/* codec athers param */
	ret = of_property_read_u32(np, "adc-delay-time", &temp_val);
	if (ret < 0)
		dts->adc_delay_time = 20;	/* default: ADC fifo delay 20ms */
	else
		dts->adc_delay_time = temp_val;

	/* lineout_single */
	dts->lineout_single = of_property_read_bool(np, "lineout-single");

	/* tx_hub */
	dts->tx_hub_en = of_property_read_bool(np, "tx-hub-en");

	SND_LOG_DEBUG("adc-gain		-> %u\n", dts->adc_gain);
	SND_LOG_DEBUG("mic1-gain		-> %u\n", dts->mic1_gain);
	SND_LOG_DEBUG("mic2-gain		-> %u\n", dts->mic2_gain);
	SND_LOG_DEBUG("mic3-gain		-> %u\n", dts->mic2_gain);
	SND_LOG_DEBUG("dac-dig-vol	-> %u\n", dts->dac_dig_vol);
	SND_LOG_DEBUG("lineout-vol	-> %u\n", dts->lineout_vol);
	SND_LOG_DEBUG("adc-delay-time	-> %u\n", dts->adc_delay_time);
	SND_LOG_DEBUG("lineout-single     -> %s\n",
		      dts->lineout_single ? "single" : "differ");
	SND_LOG_DEBUG("tx-hub-en		-> %s\n",
		      dts->tx_hub_en ? "on" : "off");


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
	int i = 0;
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
	/* digital reg */
	while (sunxi_reg_labels[i].address < SUNXI_AUDIO_MAX_REG) {
		regmap_read(regmap, sunxi_reg_labels[i].address, &output_reg_val);
		count_tmp += sprintf(buf + count_tmp, "[0x%03x]: 0x%8x\n",
				     sunxi_reg_labels[i].address, output_reg_val);
		i++;
	}

	/* analog reg */
	while (sunxi_reg_labels[i].address <= SUNXI_AUDIO_MAX_REG_PR) {
		output_reg_val = sunxi_regmap_read_prcm(regmap, sunxi_reg_labels[i].address);
		count_tmp += sprintf(buf + count_tmp, "[0x%03x]: 0x%8x\n",
				     sunxi_reg_labels[i].address, output_reg_val);
		i++;
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
	if (input_reg_offset > SUNXI_AUDIO_MAX_REG_PR) {
		pr_err("reg offset > audio max reg[0x%x]\n", SUNXI_AUDIO_MAX_REG_PR);
		return -1;
	}

	if (input_reg_offset < SUNXI_AUDIO_MAX_REG) {
		/* digital reg */
		regmap_read(regmap, input_reg_offset, &output_reg_val);
		pr_info("reg[0x%03x]: 0x%x (old)\n", input_reg_offset, output_reg_val);
		regmap_write(regmap, input_reg_offset, input_reg_val);
		regmap_read(regmap, input_reg_offset, &output_reg_val);
		pr_info("reg[0x%03x]: 0x%x (new)\n", input_reg_offset, output_reg_val);
	} else {
		/* analog reg */
		output_reg_val = sunxi_regmap_read_prcm(regmap, input_reg_offset);
		pr_info("reg[0x%03x]: 0x%x (old)\n", input_reg_offset, output_reg_val);
		sunxi_regmap_write_prcm(regmap, input_reg_offset, input_reg_val);
		output_reg_val = sunxi_regmap_read_prcm(regmap, input_reg_offset);
		pr_info("reg[0x%03x]: 0x%x (new)\n", input_reg_offset, output_reg_val);
	}

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

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	struct snd_sunxi_dump *dump = &codec->dump;
#endif

	SND_LOG_DEBUG("\n");

	/* remove components */
#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	snd_sunxi_dump_unregister(dump);
#endif

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

MODULE_AUTHOR("lijingpsw@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.3");
MODULE_DESCRIPTION("sunxi soundcard codec of internal-codec");
