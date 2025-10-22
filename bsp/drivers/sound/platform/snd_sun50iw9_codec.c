// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2021, Dby <dby@allwinnertech.com>
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
#include "snd_sun50iw9_codec.h"

#define DRV_NAME	"sunxi-snd-codec"

static struct audio_reg_label sunxi_reg_labels[] = {
	REG_LABEL(SUNXI_DAC_DPC),
	REG_LABEL(SUNXI_DAC_FIFO_CTL),
	REG_LABEL(SUNXI_DAC_FIFO_STA),
	REG_LABEL(SUNXI_DAC_CNT),
	REG_LABEL(SUNXI_DAC_DG_REG),
	REG_LABEL(AC_DAC_REG),
	REG_LABEL(AC_MIXER_REG),
	REG_LABEL(AC_RAMP_REG),
};
static struct audio_reg_group sunxi_reg_group = REG_GROUP(sunxi_reg_labels);

struct sample_rate {
	unsigned int samplerate;
	unsigned int rate_bit;
};
static const struct sample_rate g_sample_rate_conv[] = {
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
static int snd_sunxi_rglt_init(struct platform_device *pdev, struct sunxi_codec_rglt *rglt);
static void snd_sunxi_rglt_exit(struct sunxi_codec_rglt *rglt);
static int snd_sunxi_rglt_enable(struct sunxi_codec_rglt *rglt);
static void snd_sunxi_rglt_disable(struct sunxi_codec_rglt *rglt);

static int sunxi_internal_codec_dai_hw_params(struct snd_pcm_substream *substream,
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
		regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
				   0x3 << DAC_FIFO_MODE, 0x3 << DAC_FIFO_MODE);
		regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
				   0x1 << TX_SAMPLE_BITS,
				   0x0 << TX_SAMPLE_BITS);
	break;
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
				   0x3 << DAC_FIFO_MODE, 0x0 << DAC_FIFO_MODE);
		regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
				   0x1 << TX_SAMPLE_BITS, 0x1 << TX_SAMPLE_BITS);
	break;
	default:
		SND_LOG_ERR("unsupport format\n");
		break;
	}

	/* set rate */
	i = 0;
	for (i = 0; i < ARRAY_SIZE(g_sample_rate_conv); i++) {
		if (g_sample_rate_conv[i].samplerate == params_rate(params))
			break;
	}
	regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL, 0x7 << DAC_FS,
			   g_sample_rate_conv[i].rate_bit << DAC_FS);

	/* set channels */
	switch (params_channels(params)) {
	case 1:
		regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
				   0x1 << DAC_MONO_EN, 0x1 << DAC_MONO_EN);
	break;
	case 2:
		regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
				   0x1 << DAC_MONO_EN, 0x0 << DAC_MONO_EN);
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
	SND_LOG_WARN("unsupport capture\n");

	return 0;
}

static int sunxi_internal_codec_dai_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
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

static int sunxi_internal_codec_dai_set_pll(struct snd_soc_dai *dai,
					    int pll_id, int source,
					    unsigned int freq_in,
					    unsigned int freq_out)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_clk *clk = &codec->clk;

	SND_LOG_DEBUG("stream -> %s, freq_in ->%u, freq_out ->%u\n",
		      pll_id ? "IN" : "OUT", freq_in, freq_out);

	if (pll_id == SNDRV_PCM_STREAM_PLAYBACK)
		goto playback;
	else
		goto capture;

playback:
	if (clk_set_rate(clk->clk_pll_audio, freq_in)) {
		SND_LOG_ERR("clk pllaudio set rate failed\n");
		return -EINVAL;
	}

	/* moduleclk freq = 49.152/45.1584M, audio clk source = 98.304/90.3168M, own sun50iw9 */
	if (clk_set_rate(clk->clk_audio, freq_out / 2)) {
		SND_LOG_ERR("clk audio set rate failed\n");
		return -EINVAL;
	}

	return 0;

capture:
	SND_LOG_WARN("unsupport capture\n");

	return 0;
}

static int sunxi_internal_codec_dai_prepare(struct snd_pcm_substream *substream,
					    struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		goto playback;
	else
		goto capture;

playback:
	regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
			   0x1 << DAC_FIFO_FLUSH, 0x1 << DAC_FIFO_FLUSH);
	regmap_write(regmap, SUNXI_DAC_FIFO_STA,
		     1 << DAC_TXE_INT | 1 << DAC_TXU_INT | 1 << DAC_TXO_INT);
	regmap_write(regmap, SUNXI_DAC_CNT, 0);

	return 0;

capture:
	SND_LOG_WARN("unsupport capture\n");

	return 0;
}

static int sunxi_internal_codec_dai_trigger(struct snd_pcm_substream *substream,
					    int cmd,
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
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
					   1 << DAC_DRQ_EN, 1 << DAC_DRQ_EN);
		} else {
			SND_LOG_WARN("unsupport capture\n");
		}
	break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
					   1 << DAC_DRQ_EN, 0 << DAC_DRQ_EN);
		} else {
			SND_LOG_WARN("unsupport capture\n");
		}
	break;
	default:
		SND_LOG_ERR("unsupport cmd\n");
		return -EINVAL;
	}

	return 0;
}

static const struct snd_soc_dai_ops sunxi_internal_codec_dai_ops = {
	.set_pll	= sunxi_internal_codec_dai_set_pll,
	.hw_params	= sunxi_internal_codec_dai_hw_params,
	.prepare	= sunxi_internal_codec_dai_prepare,
	.trigger	= sunxi_internal_codec_dai_trigger,
	.hw_free	= sunxi_internal_codec_dai_hw_free,
};

static struct snd_soc_dai_driver sunxi_internal_codec_dai = {
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
	.ops = &sunxi_internal_codec_dai_ops,
};

/*******************************************************************************
 * *** sound card & component function source ***
 * @0 sound card probe
 * @1 component function kcontrol register
 ******************************************************************************/
/* components function kcontrol setting */
static const char *sunxi_switch_text[] = {"Off", "On"};
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_tx_hub_mode_enum, sunxi_switch_text);
static const DECLARE_TLV_DB_SCALE(digital_tlv, 0, -116, -7424);
static const unsigned int lineout_tlv[] = {
	TLV_DB_RANGE_HEAD(2),
	0, 0, TLV_DB_SCALE_ITEM(0, 0, 1),
	1, 31, TLV_DB_SCALE_ITEM(-4350, 150, 1),
};

static int sunxi_get_tx_hub_mode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_DAC_DPC, &reg_val);

	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << DAC_HUB_EN)) ? 1 : 0);

	return 0;
}

static int sunxi_set_tx_hub_mode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (ucontrol->value.integer.value[0]) {
	case	0:
		regmap_update_bits(regmap, AC_RAMP_REG, 0x1 << RDEN, 0x0 << RDEN);
		regmap_update_bits(regmap, AC_DAC_REG, 0x1 << LINEOUTL_EN, 0x0 << LINEOUTL_EN);
		regmap_update_bits(regmap, AC_DAC_REG, 0x1 << DACLEN, 0x0 << DACLEN);
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << EN_DA, 0x0 << EN_DA);

		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << DAC_HUB_EN, 0x0 << DAC_HUB_EN);
		break;
	case	1:
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << DAC_HUB_EN, 0x1 << DAC_HUB_EN);
		/* set tx route */
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << EN_DA, 0x1 << EN_DA);
		regmap_update_bits(regmap, AC_DAC_REG, 0x1 << DACLEN, 0x1 << DACLEN);
		regmap_update_bits(regmap, AC_DAC_REG, 0x1 << LINEOUTL_EN, 0x1 << LINEOUTL_EN);
		regmap_update_bits(regmap, AC_RAMP_REG, 0x1 << RDEN, 0x1 << RDEN);
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

static const struct snd_kcontrol_new sunxi_codec_controls[] = {
	SOC_SINGLE_TLV("digital volume", SUNXI_DAC_DPC, DVOL, 0x3F, 1, digital_tlv),
	SOC_SINGLE_TLV("lineout volume", AC_DAC_REG, LINEOUT_VOL, 0x1F, 0, lineout_tlv),
};

static int sunxi_codec_playback_event(struct snd_soc_dapm_widget *w,
				      struct snd_kcontrol *k, int event)
{
	unsigned int reg_val;
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << EN_DA, 0x1 << EN_DA);
		break;

	case SND_SOC_DAPM_POST_PMD:
		regmap_read(regmap, AC_DAC_REG, &reg_val);
		reg_val &= ((0x1 << DACLEN) | (0x1 << DACREN));
		if (!reg_val)
			regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << EN_DA, 0x0 << EN_DA);
		break;

	default:
		break;
	}
	return 0;
}

static void sunxi_ramp_event(struct sunxi_codec *codec, int event)
{
	struct sunxi_codec_dts *dts = &codec->dts;
	struct sunxi_codec_clk *clk = &codec->clk;
	struct regmap *regmap = codec->mem.regmap;
	unsigned long moduleclk_freq;
	bool freq_seq;

	/* ramp_msleep_time[0][] -> moduleclk freq 45.1584M
	 * ramp_msleep_time[1][] -> moduleclk freq 49.152M
	 */
	static const unsigned int ramp_msleep_time[2][8] = {
		{8, 15, 28, 56, 110, 138, 218, 273},
		{8, 14, 26, 51, 101, 126, 202, 252}
	};

	SND_LOG_DEBUG("\n");

	if (dts->ramp_en) {
		regmap_update_bits(regmap, AC_DAC_REG, 0x1 << RSWITCH, 0x0 << RSWITCH);
		regmap_update_bits(regmap, AC_DAC_REG, 0x1 << RAMPEN, 0x1 << RAMPEN);
	} else {
		regmap_update_bits(regmap, AC_DAC_REG, 0x1 << RSWITCH, 0x1 << RSWITCH);
		regmap_update_bits(regmap, AC_DAC_REG, 0x1 << RAMPEN, 0x0 << RAMPEN);
		return;
	}

	moduleclk_freq = clk_get_rate(clk->clk_audio);
	if (moduleclk_freq == 45158400) {
		freq_seq = 0;
	} else if (moduleclk_freq == 49152000) {
		freq_seq = 1;
	} else {
		SND_LOG_ERR("unsupport rapm freq -> %lu\n", moduleclk_freq);
		return;
	}
	if (dts->ramp_time_up > 7)
		dts->ramp_time_up = 7;
	if (dts->ramp_time_down > 7)
		dts->ramp_time_down = 7;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, AC_RAMP_REG, 0x7 << RAMP_STEP,
				   dts->ramp_time_up << RAMP_STEP);
		regmap_update_bits(regmap, AC_RAMP_REG, 0x1 << RDEN, 0x1 << RDEN);
		msleep(ramp_msleep_time[freq_seq][dts->ramp_time_up]);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		regmap_update_bits(regmap, AC_RAMP_REG, 0x7 << RAMP_STEP,
				   dts->ramp_time_down << RAMP_STEP);
		regmap_update_bits(regmap, AC_RAMP_REG, 0x1 << RDEN, 0x0 << RDEN);
		msleep(ramp_msleep_time[freq_seq][dts->ramp_time_down]);
		break;
	default:
		break;
	}
}

static int sunxi_lineout_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		sunxi_ramp_event(codec, event);
		regmap_update_bits(regmap, AC_DAC_REG,
				   (0x1 << LINEOUTL_EN) | (0x1 << LINEOUTR_EN),
				   (0x1 << LINEOUTL_EN) | (0x1 << LINEOUTR_EN));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		sunxi_ramp_event(codec, event);
		regmap_update_bits(regmap, AC_DAC_REG,
				   (0x1 << LINEOUTL_EN) | (0x1 << LINEOUTR_EN),
				   (0x0 << LINEOUTL_EN) | (0x0 << LINEOUTR_EN));
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

static const struct snd_kcontrol_new left_output_mixer[] = {
	SOC_DAPM_SINGLE("DACL Switch", AC_MIXER_REG, LMIX_LDAC, 1, 0),
	SOC_DAPM_SINGLE("DACR Switch", AC_MIXER_REG, LMIX_RDAC, 1, 0),
};

static const struct snd_kcontrol_new right_output_mixer[] = {
	SOC_DAPM_SINGLE("DACL Switch", AC_MIXER_REG, RMIX_LDAC, 1, 0),
	SOC_DAPM_SINGLE("DACR Switch", AC_MIXER_REG, RMIX_RDAC, 1, 0),
};

static const struct snd_soc_dapm_widget sunxi_codec_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_IN_E("DACL", "Playback", 0, AC_DAC_REG, DACLEN, 0,
			      sunxi_codec_playback_event,
			      SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_IN_E("DACR", "Playback", 0, AC_DAC_REG, DACREN, 0,
			      sunxi_codec_playback_event,
			      SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MIXER("OutputL Mixer", AC_MIXER_REG, LMIXEN, 0,
			   left_output_mixer, ARRAY_SIZE(left_output_mixer)),
	SND_SOC_DAPM_MIXER("OutputR Mixer", AC_MIXER_REG, RMIXEN, 0,
			   right_output_mixer, ARRAY_SIZE(right_output_mixer)),

	SND_SOC_DAPM_OUTPUT("LINEOUTL_PIN"),
	SND_SOC_DAPM_OUTPUT("LINEOUTR_PIN"),

	SND_SOC_DAPM_LINE("LINEOUT", sunxi_lineout_event),
	SND_SOC_DAPM_SPK("SPK", sunxi_spk_event),
};

static const struct snd_soc_dapm_route sunxi_codec_dapm_routes[] = {
	{"OutputL Mixer", "DACR Switch", "DACR"},
	{"OutputL Mixer", "DACL Switch", "DACL"},

	{"OutputR Mixer", "DACL Switch", "DACL"},
	{"OutputR Mixer", "DACR Switch", "DACR"},

	{"LINEOUTL_PIN", NULL, "OutputL Mixer"},
	{"LINEOUTR_PIN", NULL, "OutputR Mixer"},
};

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

static void sunxi_internal_codec_init(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_dts *dts = &codec->dts;
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	/* Disable DRC function for playback */
	regmap_write(regmap, SUNXI_DAC_DAP_CTL, 0);

	/* set digital vol */
	regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x3f << DVOL, 0 << DVOL);

	/* set lineout vol */
	regmap_update_bits(regmap, AC_DAC_REG,
			   0x1f << LINEOUT_VOL,
			   dts->lineout_vol << LINEOUT_VOL);

	regmap_update_bits(regmap, AC_DAC_REG, 0x1 << LMUTE, 0x1 << LMUTE);
	regmap_update_bits(regmap, AC_DAC_REG, 0x1 << RMUTE, 0x1 << RMUTE);
}

static int sunxi_internal_codec_probe(struct snd_soc_component *component)
{
	int ret;
	struct snd_soc_dapm_context *dapm = &component->dapm;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_dts *dts = &codec->dts;

	SND_LOG_DEBUG("\n");

	/* component kcontrols -> tx_hub */
	if (dts->tx_hub_en) {
		ret = snd_soc_add_component_controls(component, sunxi_tx_hub_controls,
						     ARRAY_SIZE(sunxi_tx_hub_controls));
		if (ret)
			SND_LOG_ERR("add tx_hub kcontrols failed\n");
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

	sunxi_internal_codec_init(component);

	sunxi_jack_gpio.pdev = codec->pdev;
	snd_sunxi_jack_init(&sunxi_jack_port);

	return 0;
}

static void sunxi_internal_codec_remove(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	SND_LOG_DEBUG("\n");

	snd_sunxi_pa_pin_remove(codec->pa_cfg, codec->pa_pin_max);

	snd_sunxi_jack_exit(&sunxi_jack_port);
}

static int sunxi_internal_codec_suspend(struct snd_soc_component *component)
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

static int sunxi_internal_codec_resume(struct snd_soc_component *component)
{
	int ret;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	snd_sunxi_pa_pin_disable(codec->pa_cfg, codec->pa_pin_max);
	ret = snd_sunxi_clk_bus_enable(&codec->clk);
	if (ret) {
		SND_LOG_ERR("clk enable failed\n");
		return ret;
	}
	ret = snd_sunxi_rglt_enable(&codec->rglt);
	if (ret) {
		SND_LOG_ERR("regulator enable failed\n");
		return ret;
	}
	sunxi_internal_codec_init(component);
	snd_sunxi_echo_reg(regmap, &sunxi_reg_group);

	return 0;
}

static struct snd_soc_component_driver sunxi_internal_codec_dev = {
	.name		= DRV_NAME,
	.probe		= sunxi_internal_codec_probe,
	.remove		= sunxi_internal_codec_remove,
	.suspend	= sunxi_internal_codec_suspend,
	.resume		= sunxi_internal_codec_resume,
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
	clk->clk_bus = of_clk_get_by_name(np, "clk_bus_audio");
	if (IS_ERR_OR_NULL(clk->clk_bus)) {
		SND_LOG_ERR("clk bus get failed\n");
		ret = PTR_ERR(clk->clk_bus);
		goto err_get_clk_bus;
	}

	/* get parent clk */
	clk->clk_pll_audio = of_clk_get_by_name(np, "clk_pll_audio_4x");
	if (IS_ERR_OR_NULL(clk->clk_pll_audio)) {
		SND_LOG_ERR("clk pll get failed\n");
		ret = PTR_ERR(clk->clk_pll_audio);
		goto err_get_clk_pll_audio;
	}

	/* get module clk */
	clk->clk_audio = of_clk_get_by_name(np, "clk_audio");
	if (IS_ERR_OR_NULL(clk->clk_audio)) {
		SND_LOG_ERR("clk audio get failed\n");
		ret = PTR_ERR(clk->clk_audio);
		goto err_get_clk_audio;
	}

	/* set clk audio parent of pllaudio */
	if (clk_set_parent(clk->clk_audio, clk->clk_pll_audio)) {
		SND_LOG_ERR("set parent clk audio failed\n");
		ret = -EINVAL;
		goto err_set_parent;
	}

	return 0;

err_set_parent:
	clk_put(clk->clk_audio);
err_get_clk_audio:
	clk_put(clk->clk_pll_audio);
err_get_clk_pll_audio:
	clk_put(clk->clk_bus);
err_get_clk_bus:
err_get_clk_rst:
	return ret;
}

static void snd_sunxi_clk_exit(struct sunxi_codec_clk *clk)
{
	SND_LOG_DEBUG("\n");

	clk_put(clk->clk_audio);
	clk_put(clk->clk_pll_audio);
	clk_put(clk->clk_bus);
}

static int snd_sunxi_clk_enable(struct sunxi_codec_clk *clk)
{
	int ret = 0;

	SND_LOG_DEBUG("\n");

	if (clk_prepare_enable(clk->clk_pll_audio)) {
		SND_LOG_ERR("pllaudio enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_pll_audio;
	}

	if (clk_prepare_enable(clk->clk_audio)) {
		SND_LOG_ERR("dacclk enable failed\n");
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
		SND_LOG_ERR("clk rst deassert failed\n");
		ret = -EINVAL;
		goto err_deassert_rst;
	}

	if (clk_prepare_enable(clk->clk_bus)) {
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

	clk_disable_unprepare(clk->clk_audio);
	clk_disable_unprepare(clk->clk_pll_audio);
}

static void snd_sunxi_clk_bus_disable(struct sunxi_codec_clk *clk)
{
	SND_LOG_DEBUG("\n");

	clk_disable_unprepare(clk->clk_bus);
	reset_control_assert(clk->clk_rst);
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

	/* lineout volume */
	ret = of_property_read_u32(np, "lineout-vol", &temp_val);
	if (ret < 0) {
		dts->lineout_vol = 0;
	} else {
		dts->lineout_vol = temp_val;
	}

	/* ramp */
	dts->ramp_en = of_property_read_bool(np, "ramp-en");
	ret = of_property_read_u32(np, "ramp-time-up", &temp_val);
	if (ret < 0) {
		dts->ramp_time_up = 0;
	} else {
		dts->ramp_time_up = temp_val - 1;
		if (dts->ramp_time_up > 7) {
			dts->ramp_time_up = 7;
			SND_LOG_DEBUG("ramp-time-up max value 7\n");
		}
	}
	ret = of_property_read_u32(np, "ramp-time-down", &temp_val);
	if (ret < 0) {
		dts->ramp_time_down = 0;
	} else {
		dts->ramp_time_down = temp_val - 1;
		if (dts->ramp_time_down > 7) {
			dts->ramp_time_down = 7;
			SND_LOG_DEBUG("ramp-time-down max value 7\n");
		}
	}

	/* tx_hub */
	dts->tx_hub_en = of_property_read_bool(np, "tx-hub-en");

	SND_LOG_DEBUG("lineout vol -> %u\n", dts->lineout_vol);
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

static int sunxi_internal_codec_dev_probe(struct platform_device *pdev)
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
		ret = -ENOMEM;
		goto err_regulator_init;
	}

	/* dts_params init */
	snd_sunxi_dts_params_init(pdev, dts);

	/* pa_pin init */
	codec->pa_cfg = snd_sunxi_pa_pin_init(pdev, &codec->pa_pin_max);

	/* alsa component register */
	ret = snd_soc_register_component(dev,
					 &sunxi_internal_codec_dev,
					 &sunxi_internal_codec_dai, 1);
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

static int sunxi_internal_codec_dev_remove(struct platform_device *pdev)
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

	/* alsa component unregister */
#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	snd_sunxi_dump_unregister(dump);
#endif
	snd_soc_unregister_component(dev);

	snd_sunxi_mem_exit(pdev, mem);
	snd_sunxi_clk_bus_disable(clk);
	snd_sunxi_clk_exit(clk);
	snd_sunxi_rglt_exit(rglt);
	snd_sunxi_pa_pin_exit(codec->pa_cfg, codec->pa_pin_max);

	/* sunxi codec custom info free */
	devm_kfree(dev, codec);
	of_node_put(pdev->dev.of_node);

	SND_LOG_DEBUG("unregister internal-codec codec success\n");

	return 0;
}

static const struct of_device_id sunxi_internal_codec_of_match[] = {
	{ .compatible = "allwinner," DRV_NAME, },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_internal_codec_of_match);

static struct platform_driver sunxi_internal_codec_driver = {
	.driver	= {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= sunxi_internal_codec_of_match,
	},
	.probe	= sunxi_internal_codec_dev_probe,
	.remove	= sunxi_internal_codec_dev_remove,
};

int __init sunxi_internal_codec_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_internal_codec_driver);
	if (ret != 0) {
		SND_LOG_ERR("platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_internal_codec_dev_exit(void)
{
	platform_driver_unregister(&sunxi_internal_codec_driver);
}

late_initcall(sunxi_internal_codec_dev_init);
module_exit(sunxi_internal_codec_dev_exit);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.4");
MODULE_DESCRIPTION("sunxi soundcard codec of internal-codec");
