// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2021, Dby <dby@allwinnertech.com>
 *
 * based on ${LINUX}/sound/soc/generic/simple-card.c
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#define SUNXI_MODNAME		"sound-mach"
#include "snd_sunxi_log.h"
#include <linux/module.h>
#include <linux/time.h>
#include <sound/soc.h>
#include <sound/jack.h>

#include "snd_sunxi_adapter.h"
#include "snd_sunxi_common.h"
#include "snd_sunxi_jack.h"
#include "snd_sunxi_mach_utils.h"

#define DAI		"sound-dai"
#define CELL		"#sound-dai-cells"
#define PREFIX		"soundcard-mach,"

#define DRV_NAME	"sunxi-snd-mach"

static void asoc_simple_shutdown(struct snd_pcm_substream *substream)
{
}

static int asoc_simple_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct asoc_simple_priv *priv = snd_soc_card_get_drvdata(rtd->card);
	if (priv->wait_time)
		substream->wait_time = msecs_to_jiffies(priv->wait_time);
	return 0;
}

static int asoc_simple_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai;
	struct snd_soc_dai *cpu_dai = sunxi_adpt_rtd_cpu_dai(rtd);
	struct asoc_simple_priv *priv = snd_soc_card_get_drvdata(rtd->card);
	struct snd_soc_dai_link *dai_link = simple_priv_to_link(priv, rtd->num);
	struct simple_dai_props *dai_props = simple_priv_to_props(priv, rtd->num);
	struct asoc_simple_dai *dais = priv->dais;
	unsigned int mclk = 0;
	unsigned int cpu_pll_clk;
	unsigned int codec_fp;
	unsigned int codec_pllin_clk, codec_pllout_clk;
	unsigned int cpu_bclk_ratio, codec_bclk_ratio;
	unsigned int freq_point;
	int cpu_clk_div, codec_clk_div;
	int i, ret = 0;

	switch (params_rate(params)) {
	case 8000:
	case 12000:
	case 16000:
	case 24000:
	case 32000:
	case 48000:
	case 64000:
	case 96000:
	case 192000:
		freq_point = 24576000;
		codec_fp = dai_props->codec_pll_fp[1];
		break;
	case 11025:
	case 22050:
	case 44100:
	case 88200:
	case 176400:
		freq_point = 22579200;
		codec_fp = dai_props->codec_pll_fp[0];
		break;
	default:
		SND_LOG_ERR("Invalid rate %d\n", params_rate(params));
		return -EINVAL;
	}

	/* for cpudai pll clk */
	cpu_pll_clk	= freq_point * dai_props->cpu_pll_fs;
	cpu_clk_div	= cpu_pll_clk / params_rate(params);
	if (dai_props->codec_pllin_mode == 0) {			/* fp * fs mode */
		codec_pllin_clk = freq_point * dai_props->codec_pllin_fs;
	} else if (dai_props->codec_pllin_mode == 1) {		/* codec-fp * fs mode */
		codec_pllin_clk = codec_fp * dai_props->codec_pllin_fs;
	} else if (dai_props->codec_pllin_mode == 2) {		/* bclk mode */
		codec_pllin_clk = params_rate(params) * dais->slot_width * dais->slots;
	} else {
		SND_LOG_ERR("Invalid codec pllin mode\n");
		return -1;
	}

	if (dai_props->codec_pllout_mode == 0) {		/* fp * fs mode */
		codec_pllout_clk = freq_point * dai_props->codec_pllout_fs;
	} else if (dai_props->codec_pllout_mode == 1) {		/* codec-fp * fs mode */
		codec_pllout_clk = codec_fp * dai_props->codec_pllout_fs;
	} else if (dai_props->codec_pllout_mode == 2) {		/* bclk mode */
		codec_pllout_clk = params_rate(params) * dais->slot_width * dais->slots;
	} else {
		SND_LOG_ERR("Invalid codec pllout mode\n");
		return -1;
	}
	codec_clk_div	= codec_pllout_clk / params_rate(params);
	SND_LOG_DEBUG("freq point   : %u\n", freq_point);
	SND_LOG_DEBUG("cpu pllclk   : %u\n", cpu_pll_clk);
	SND_LOG_DEBUG("codec pllin_clk : %u\n", codec_pllin_clk);
	SND_LOG_DEBUG("codec pllout_clk : %u\n", codec_pllout_clk);
	SND_LOG_DEBUG("cpu clk_div  : %u\n", cpu_clk_div);
	SND_LOG_DEBUG("codec clk_div: %u\n", codec_clk_div);

	if (cpu_dai->driver->ops && cpu_dai->driver->ops->set_pll) {
		ret = snd_soc_dai_set_pll(cpu_dai, substream->stream, 0,
					  cpu_pll_clk, cpu_pll_clk);
		if (ret) {
			SND_LOG_ERR("cpu_dai set pllclk failed\n");
			return ret;
		}
	} else if (cpu_dai->component->driver->set_pll) {
		ret = snd_soc_component_set_pll(cpu_dai->component, substream->stream, 0,
						cpu_pll_clk, cpu_pll_clk);
		if (ret) {
			SND_LOG_ERR("cpu_dai set pllclk failed\n");
			return ret;
		}
	}
	sunxi_adpt_rtd_codec_dai(rtd, i, codec_dai) {
		if (codec_dai->driver->ops && codec_dai->driver->ops->set_pll) {
			ret = snd_soc_dai_set_pll(codec_dai, substream->stream, 0,
						  codec_pllin_clk, codec_pllout_clk);
			if (ret) {
				SND_LOG_ERR("codec_dai set pllclk failed\n");
				return ret;
			}
		} else if (codec_dai->component->driver->set_pll) {
			ret = snd_soc_component_set_pll(codec_dai->component, substream->stream, 0,
							codec_pllin_clk, codec_pllout_clk);
			if (ret) {
				SND_LOG_ERR("codec_dai set pllclk failed\n");
				return ret;
			}
		}
	}

	if (cpu_dai->driver->ops && cpu_dai->driver->ops->set_clkdiv) {
		ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, cpu_clk_div);
		if (ret) {
			SND_LOG_ERR("cpu_dai set clk_div failed\n");
			return ret;
		}
	}
	sunxi_adpt_rtd_codec_dai(rtd, i, codec_dai) {
		if (codec_dai->driver->ops && codec_dai->driver->ops->set_clkdiv) {
			ret = snd_soc_dai_set_clkdiv(codec_dai, 0, codec_clk_div);
			if (ret) {
				SND_LOG_ERR("cadec_dai set clk_div failed.\n");
				return ret;
			}
		}
	}

	/* use for i2s/pcm only */
	if (!(dais->slots && dais->slot_width))
		return 0;

	/* for cpudai & codecdai mclk */
	if (dai_props->mclk_fp[0] == 0 && dai_props->mclk_fp[1] == 0)
		mclk = params_rate(params) * dai_props->mclk_fs;
	else if (freq_point == 22579200)
		mclk = dai_props->mclk_fp[0] * dai_props->mclk_fs;
	else if (freq_point == 24576000)
		mclk = dai_props->mclk_fp[1] * dai_props->mclk_fs;
	cpu_bclk_ratio = cpu_pll_clk / (params_rate(params) * dais->slot_width * dais->slots);
	codec_bclk_ratio = codec_pllout_clk / (params_rate(params)
			   * dais->slot_width * dais->slots);
	SND_LOG_DEBUG("mclk-fs         : %u\n", dai_props->mclk_fs);
	SND_LOG_DEBUG("mclk-fp0        : %u\n", dai_props->mclk_fp[0]);
	SND_LOG_DEBUG("mclk-fp1        : %u\n", dai_props->mclk_fp[1]);
	SND_LOG_DEBUG("mclk            : %u\n", mclk);
	SND_LOG_DEBUG("cpu_bclk_ratio  : %u\n", cpu_bclk_ratio);
	SND_LOG_DEBUG("codec_bclk_ratio: %u\n", codec_bclk_ratio);

	ret = snd_sunxi_extparam_set_state_sync(rtd->card->name, EXTPARAM_ID_DAI_UCFMT,
						(void *)&dai_props->dai_ucfmt);
	if (ret) {
		SND_LOG_ERR("extparam set state sync failed\n");
		return ret;
	}

	if (cpu_dai->driver->ops && cpu_dai->driver->ops->set_sysclk) {
		ret = snd_soc_dai_set_sysclk(cpu_dai, 0, mclk, SND_SOC_CLOCK_OUT);
		if (ret) {
			SND_LOG_ERR("cpu_dai set sysclk(mclk) failed\n");
			return ret;
		}
	}
	sunxi_adpt_rtd_codec_dai(rtd, i, codec_dai) {
		if (codec_dai->driver->ops && codec_dai->driver->ops->set_sysclk) {
			ret = snd_soc_dai_set_sysclk(codec_dai, 0, mclk, SND_SOC_CLOCK_IN);
			if (ret) {
				SND_LOG_ERR("cadec_dai set sysclk(mclk) failed\n");
				return ret;
			}
		}
	}

	if (cpu_dai->driver->ops && cpu_dai->driver->ops->set_bclk_ratio) {
		ret = snd_soc_dai_set_bclk_ratio(cpu_dai, cpu_bclk_ratio);
		if (ret) {
			SND_LOG_ERR("cpu_dai set bclk failed\n");
			return ret;
		}
	}
	sunxi_adpt_rtd_codec_dai(rtd, i, codec_dai) {
		if (codec_dai->driver->ops && codec_dai->driver->ops->set_bclk_ratio) {
			ret = snd_soc_dai_set_bclk_ratio(codec_dai, codec_bclk_ratio);
			if (ret) {
				SND_LOG_ERR("codec_dai set bclk failed\n");
				return ret;
			}
		}
	}

	if (cpu_dai->driver->ops && cpu_dai->driver->ops->set_fmt) {
		ret = snd_soc_dai_set_fmt(cpu_dai, dai_link->dai_fmt);
		if (ret) {
			SND_LOG_ERR("cpu dai set fmt failed\n");
			return ret;
		}
	}
	sunxi_adpt_rtd_codec_dai(rtd, i, codec_dai) {
		if (codec_dai->driver->ops && codec_dai->driver->ops->set_fmt) {
			ret = snd_soc_dai_set_fmt(codec_dai, dai_link->dai_fmt);
			if (ret) {
				SND_LOG_ERR("codec dai set fmt failed\n");
				return ret;
			}
		}
	}

	if (cpu_dai->driver->ops && cpu_dai->driver->ops->set_tdm_slot) {
		ret = snd_soc_dai_set_tdm_slot(cpu_dai, 0, 0, dais->slots, dais->slot_width);
		if (ret) {
			SND_LOG_ERR("cpu dai set tdm slot failed\n");
			return ret;
		}
	}
	sunxi_adpt_rtd_codec_dai(rtd, i, codec_dai) {
		if (codec_dai->driver->ops && codec_dai->driver->ops->set_tdm_slot) {
			ret = snd_soc_dai_set_tdm_slot(codec_dai, 0, 0, dais->slots,
							dais->slot_width);
			if (ret) {
				SND_LOG_ERR("codec dai set tdm slot failed\n");
				return ret;
			}
		}
	}

	return 0;
}

static struct snd_soc_ops simple_ops = {
	.startup = asoc_simple_startup,
	.shutdown = asoc_simple_shutdown,
	.hw_params = asoc_simple_hw_params,
};

static int asoc_simple_dai_init(struct snd_soc_pcm_runtime *rtd)
{
	int i, j;
	struct snd_soc_dai *codec_dai;
	struct snd_soc_dai *cpu_dai;
	struct snd_soc_component *component;
	struct snd_soc_dapm_context *dapm;
	struct snd_soc_card *card = rtd->card;
	const struct snd_kcontrol_new *controls = card->controls;
	struct snd_soc_dapm_widget *w;
	char prefixed_pin[80];
	const char *pin_name;
	const char *prefix;

	sunxi_adpt_rtd_codec_dai(rtd, i, codec_dai) {
		component = codec_dai->component;
		dapm = &component->dapm;
		prefix = component->name_prefix;
		for (j = 0; j < card->num_controls; j++) {
			if (controls[j].info == snd_soc_dapm_info_pin_switch) {
				if (prefix) {
					snprintf(prefixed_pin, sizeof(prefixed_pin), "%s %s",
						prefix, (const char *)controls[j].private_value);
					pin_name = prefixed_pin;
				} else {
					pin_name = (const char *)controls[j].private_value;
				}
				list_for_each_entry(w, &card->widgets, list)
					if (!strcmp(w->name, pin_name) && w->dapm == dapm)
						snd_soc_dapm_disable_pin(dapm, pin_name);
			}
		}
	}

	cpu_dai = sunxi_adpt_rtd_cpu_dai(rtd);
	component = cpu_dai->component;
	dapm = &component->dapm;
	prefix = component->name_prefix;
	for (j = 0; j < card->num_controls; j++) {
		if (controls[j].info == snd_soc_dapm_info_pin_switch) {
			if (prefix) {
				snprintf(prefixed_pin, sizeof(prefixed_pin), "%s %s",
					 prefix, (const char *)controls[j].private_value);
				pin_name = prefixed_pin;
			} else {
				pin_name = (const char *)controls[j].private_value;
			}
			list_for_each_entry(w, &card->widgets, list)
				if (!strcmp(w->name, pin_name) && w->dapm == dapm)
					snd_soc_dapm_disable_pin(dapm, pin_name);
		}
	}

	if (card->num_controls)
		snd_soc_dapm_sync(dapm);

	/* snd_soc_dai_set_sysclk(); */
	/* snd_soc_dai_set_tdm_slot(); */

	return 0;
}

static int simple_dai_link_of(struct device_node *node, struct asoc_simple_priv *priv)
{
	struct device *dev = simple_priv_to_dev(priv);
	struct snd_soc_dai_link *dai_link = simple_priv_to_link(priv, 0);
	struct simple_dai_props *dai_props = simple_priv_to_props(priv, 0);
	struct device_node *top_np = NULL;
	struct device_node *cpu = NULL;
	struct device_node *plat = NULL;
	struct device_node *codec = NULL;
	char prop[128];
	char *prefix = "";
	int ret, single_cpu;
	unsigned int i;

	prefix = PREFIX;
	top_np = node;

	snprintf(prop, sizeof(prop), "%scpu", prefix);
	cpu = of_get_child_by_name(top_np, prop);
	if (!cpu) {
		ret = -EINVAL;
		SND_LOG_ERR("Can't find %s DT node\n", prop);
		goto dai_link_of_err;
	}
	snprintf(prop, sizeof(prop), "%splat", prefix);
	plat = of_get_child_by_name(top_np, prop);

	snprintf(prop, sizeof(prop), "%scodec", prefix);
	codec = of_get_child_by_name(top_np, prop);
	if (!codec) {
		ret = -EINVAL;
		SND_LOG_ERR("Can't find %s DT node\n", prop);
		goto dai_link_of_err;
	}

	ret = asoc_simple_parse_daifmt(top_np, codec, prefix, &dai_link->dai_fmt);
	if (ret < 0)
		goto dai_link_of_err;

	ret = asoc_simple_parse_ucfmt(top_np, prefix, priv);
	if (ret < 0)
		goto dai_link_of_err;
	/* sunxi: parse stream direction
	 * ex1)
	 * top_node {
	 *	PREFIXplayback-only;
	 * }
	 * ex2)
	 * top_node {
	 *	PREFIXcapture-only;
	 * }
	 */
	ret = asoc_simple_parse_daistream(top_np, prefix, dai_link);
	if (ret < 0)
		goto dai_link_of_err;
	/* sunxi: parse slot-num & slot-width
	 * ex)
	 * top_node {
	 *	PREFIXplayslot-num	= <x>;
	 *	PREFIXplayslot-width	= <x>;
	 * }
	 */
	ret = asoc_simple_parse_tdm_slot(top_np, prefix, priv->dais);
	if (ret < 0)
		goto dai_link_of_err;

	ret = asoc_simple_parse_cpu(cpu, dai_link, DAI, CELL, &single_cpu);
	if (ret < 0)
		goto dai_link_of_err;
	ret = snd_soc_of_get_dai_link_codecs(dev, codec, dai_link);
	if (ret < 0) {
		if (ret == -EPROBE_DEFER)
			goto dai_link_of_err;
		ret = asoc_simple_parse_dlc_name(dev, codec, prefix, dai_link);
		if (ret < 0) {
			dai_link->codecs = devm_kcalloc(dev, 1, sizeof(*(dai_link->codecs)),
							GFP_KERNEL);
			dai_link->num_codecs = 1;
			dai_link->codecs->name = "snd-soc-dummy";
			dai_link->codecs->dai_name = "snd-soc-dummy-dai";
			/* dai_link->codecs->name = "sunxi-dummy-codec"; */
			/* dai_link->codecs->dai_name = "sunxi-dummy-codec-dai"; */
			SND_LOG_DEBUG("use dummy codec for simple card.\n");
		}
	}
	ret = asoc_simple_parse_platform(plat, dai_link, DAI, CELL);
	if (ret < 0)
		goto dai_link_of_err;

	/* sunxi: parse pll-fs & mclk-fs
	 * ex)
	 * top_node {
	 *	PREFIXcpu {
	 *		PREFIXpll-fs	= <x>;
	 *		PREFIXmclk-fs	= <x>;
	 *	}
	 * }
	 */
	ret = asoc_simple_parse_tdm_clk(cpu, codec, prefix, dai_props);
	if (ret < 0)
		goto dai_link_of_err;

	ret = asoc_simple_set_dailink_name(dev, dai_link,
					   "%s-%s",
					   dai_link->cpus->dai_name,
					   dai_link->codecs->dai_name);
	if (ret < 0)
		goto dai_link_of_err;

	dai_link->ops = &simple_ops;
	dai_link->init = asoc_simple_dai_init;

	SND_LOG_DEBUG("name   : %s\n", dai_link->stream_name);
	SND_LOG_DEBUG("format : %x\n", dai_link->dai_fmt);
	SND_LOG_DEBUG("cpu    : %s\n", dai_link->cpus->dai_name);
	for (i = 0; i < dai_link->num_codecs; i++)
		SND_LOG_DEBUG("codec[%d] : %s\n", i, dai_link->codecs[i].dai_name);

	asoc_simple_canonicalize_cpu(dai_link, single_cpu);
	asoc_simple_canonicalize_platform(dai_link);

dai_link_of_err:
	of_node_put(cpu);
	of_node_put(plat);
	of_node_put(codec);

	return ret;
}

static int simple_parse_of(struct asoc_simple_priv *priv)
{
	int ret;
	struct device *dev = simple_priv_to_dev(priv);
	struct snd_soc_card *card = simple_priv_to_card(priv);
	struct device_node *top_np = dev->of_node;

	SND_LOG_DEBUG("\n");

	if (!top_np)
		return -EINVAL;

	/* DAPM widgets */
	ret = asoc_simple_parse_widgets(card, PREFIX);
	if (ret < 0) {
		SND_LOG_ERR("asoc_simple_parse_widgets failed\n");
		return ret;
	}

	/* DAPM routes */
	ret = asoc_simple_parse_routing(card, PREFIX);
	if (ret < 0) {
		SND_LOG_ERR("asoc_simple_parse_routing failed\n");
		return ret;
	}

	/* DAPM pin_switches */
	ret = asoc_simple_parse_pin_switches(card, PREFIX);
	if (ret < 0) {
		SND_LOG_ERR("asoc_simple_parse_pin_switches failed\n");
		return ret;
	}

	/* misc */
	ret = asoc_simple_parse_misc(top_np, PREFIX, priv);
	if (ret < 0) {
		SND_LOG_ERR("asoc_simple_parse_misc failed\n");
		return ret;
	}

	/* For single DAI link & old style of DT node */
	ret = simple_dai_link_of(top_np, priv);
	if (ret < 0) {
		SND_LOG_ERR("simple_dai_link_of failed\n");
		return ret;
	}

	ret = asoc_simple_parse_card_name(card, PREFIX);
	if (ret < 0) {
		SND_LOG_ERR("asoc_simple_parse_card_name failed\n");
		return ret;
	}

	return 0;
}

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
/* sysfs debug */
struct snd_sunxi_dump_help {
	const char opt[32];
	const char val[32];
	const char note[128];
};

static void snd_sunxi_dump_version(void *priv_orig, char *buf, size_t *count)
{
	size_t count_tmp = 0;
	struct asoc_simple_priv *priv = (struct asoc_simple_priv *)priv_orig;

	if (!priv) {
		SND_LOG_ERR("priv_orig to priv failed\n");
		return;
	}
	if (priv->snd_card.dev)
		if (priv->snd_card.dev->driver)
			if (priv->snd_card.dev->driver->owner)
				goto module_version;
	return;

module_version:
	priv->module_version = priv->snd_card.dev->driver->owner->version;
	count_tmp += sprintf(buf + count_tmp, "%s\n", priv->module_version);

	*count = count_tmp;
}

static void snd_sunxi_dump_help(void *priv_orig, char *buf, size_t *count)
{
	size_t count_tmp = 0;
	static struct snd_sunxi_dump_help helps[] = {
		{"Opt num", "Val", "note"},
		{"1 CPUPLL FS", "1~n", "22.5792 or 24.576 * fs MHz"},
		{"2 CODECPLLIN FS", "1~n", "22.5792 or 24.576 * fs MHz"},
		{"3 CODECPLLOUT FS", "1~n", "22.5792 or 24.576 * fs MHz"},
		{"4 MCLK FS", "0~n", "mclk fs flag"},
		{"5 MCLK FP(44.1k fp)", "0~n", "mclk_freq(44.1k fp) = mclk_fs * mclk_fp0"},
		{"6 MCLK FP(48k fp)", "0~n", "mclk_freq(48k fp) = mclk_fs * mclk_fp1"},
		{"7 FMT", "i2s right_j left_j dsp_a dsp_b", "i2s/pcm format config"},
		{"8 MASTER", "CBM_CFM CBS_CFM CBM_CFS CBS_CFS", "bclk&lrck master"},
		{"9 INVERT", "NB_NF NB_IF IB_NF IB_IF", "bclk&lrck invert"},
		{"10 SLOTS", "2~32 (must be 2*n)", "slot number"},
		{"11 SLOT WIDTH", "16 24 32", "slot width"},
		{"12 ML SEL", "RM_TM RM_TL RL_TM RL_TL", "RX&TX MSB/LSB first select"},
		{"13 DATA LATE", "0~3", "data is offset by n BCLKS to LRCK"},
	};
	static unsigned int help_cnt = ARRAY_SIZE(helps);
	int i;

	count_tmp += sprintf(buf + count_tmp, "1. get daifmt: echo {num} > dump && cat dump\n");
	count_tmp += sprintf(buf + count_tmp, "num: 0(all)\n");
	count_tmp += sprintf(buf + count_tmp, "2. set daifmt: echo {Opt num} {Val} > dump\n");
	count_tmp += sprintf(buf + count_tmp, "%-16s    %-32s    %s\n",
			     helps[0].opt, helps[0].val, helps[0].note);
	for (i = 1; i < help_cnt; i++) {
		count_tmp += sprintf(buf + count_tmp, "%-16s -> %-32s -> %s\n",
				     helps[i].opt, helps[i].val, helps[i].note);
	}

	*count = count_tmp;
}

static int snd_sunxi_dump_show(void *priv_orig, char *buf, size_t *count)
{
	size_t count_tmp = 0;
	struct asoc_simple_priv *priv = (struct asoc_simple_priv *)priv_orig;
	struct snd_soc_card *card = simple_priv_to_card(priv);
	unsigned int dai_fmt		= card->dai_link->dai_fmt;
	struct snd_sunxi_dai_ucfmt *dai_ucfmt = &priv->dai_props->dai_ucfmt;
	unsigned int mclk_fs		= priv->dai_props->mclk_fs;
	unsigned int *mclk_fp		= priv->dai_props->mclk_fp;
	unsigned int cpu_pll_fs		= priv->dai_props->cpu_pll_fs;
	unsigned int codec_pllin_fs	= priv->dai_props->codec_pllin_fs;
	unsigned int codec_pllout_fs = priv->dai_props->codec_pllout_fs;
	int slots			= priv->dais->slots;
	int slot_width			= priv->dais->slot_width;
	char prop[8] = {0};

	if (!priv->show_daifmt)
		return 0;
	else
		priv->show_daifmt = false;

	count_tmp += sprintf(buf + count_tmp, "CPUPLL FS   -> %u\n", cpu_pll_fs);
	count_tmp += sprintf(buf + count_tmp, "CODECPLLIN FS -> %u\n", codec_pllin_fs);
	count_tmp += sprintf(buf + count_tmp, "CODECPLLOUT FS -> %u\n", codec_pllout_fs);
	count_tmp += sprintf(buf + count_tmp, "MCLK FS           -> %u\n", mclk_fs);
	count_tmp += sprintf(buf + count_tmp, "MCLK FP(44.1k fp) -> %u\n", mclk_fp[0]);
	count_tmp += sprintf(buf + count_tmp, "MCLK FP(48k fp)   -> %u\n", mclk_fp[1]);

	switch (dai_fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		snprintf(prop, sizeof(prop), "%s", "i2s");
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		snprintf(prop, sizeof(prop), "%s", "right_j");
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		snprintf(prop, sizeof(prop), "%s", "left_j");
		break;
	case SND_SOC_DAIFMT_DSP_A:
		snprintf(prop, sizeof(prop), "%s", "dsp_a");
		break;
	case SND_SOC_DAIFMT_DSP_B:
		snprintf(prop, sizeof(prop), "%s", "dsp_b");
		break;
	default:
		SND_LOG_ERR("format failed, 0x%x\n", dai_fmt);
		return -EINVAL;
	}
	count_tmp += sprintf(buf + count_tmp, "FMT         -> %s\n", prop);
	switch (dai_fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		snprintf(prop, sizeof(prop), "%s", "CBM_CFM");
		break;
	case SND_SOC_DAIFMT_CBS_CFM:
		snprintf(prop, sizeof(prop), "%s", "CBS_CFM");
		break;
	case SND_SOC_DAIFMT_CBM_CFS:
		snprintf(prop, sizeof(prop), "%s", "CBM_CFS");
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		snprintf(prop, sizeof(prop), "%s", "CBS_CFS");
		break;
	default:
		SND_LOG_ERR("master invalid, 0x%x\n", dai_fmt);
		return -EINVAL;
	}
	count_tmp += sprintf(buf + count_tmp, "MASTER      -> %s\n", prop);
	switch (dai_fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		snprintf(prop, sizeof(prop), "%s", "NB_NF");
		break;
	case SND_SOC_DAIFMT_NB_IF:
		snprintf(prop, sizeof(prop), "%s", "NB_IF");
		break;
	case SND_SOC_DAIFMT_IB_NF:
		snprintf(prop, sizeof(prop), "%s", "IB_NF");
		break;
	case SND_SOC_DAIFMT_IB_IF:
		snprintf(prop, sizeof(prop), "%s", "IB_IF");
		break;
	default:
		SND_LOG_ERR("invert invalid, 0x%x\n", dai_fmt);
		return -EINVAL;
	}
	count_tmp += sprintf(buf + count_tmp, "INVERT      -> %s\n", prop);
	count_tmp += sprintf(buf + count_tmp, "SLOTS       -> %d\n", slots);
	count_tmp += sprintf(buf + count_tmp, "SLOT WIDTH  -> %d\n", slot_width);
	if (!dai_ucfmt->rx_lsb_first && !dai_ucfmt->tx_lsb_first)
		snprintf(prop, sizeof(prop), "%s", "RM_TM");
	else if (dai_ucfmt->rx_lsb_first && !dai_ucfmt->tx_lsb_first)
		snprintf(prop, sizeof(prop), "%s", "RL_TM");
	else if (!dai_ucfmt->rx_lsb_first && dai_ucfmt->tx_lsb_first)
		snprintf(prop, sizeof(prop), "%s", "RM_TL");
	else
		snprintf(prop, sizeof(prop), "%s", "RL_TL");
	count_tmp += sprintf(buf + count_tmp, "ML SEL	    -> %s\n", prop);
	count_tmp += sprintf(buf + count_tmp, "DATA LATE   -> %d\n", dai_ucfmt->data_late);

	*count = count_tmp;

	return 0;
}

static int snd_sunxi_dump_store(void *priv_orig, const char *buf, size_t count)
{
	struct asoc_simple_priv *priv = (struct asoc_simple_priv *)priv_orig;
	struct simple_dai_props *dai_props = simple_priv_to_props(priv, 0);
	struct snd_sunxi_dai_ucfmt *dai_ucfmt = &dai_props->dai_ucfmt;
	struct snd_soc_card *card = simple_priv_to_card(priv);
	unsigned int dai_fmt		= card->dai_link->dai_fmt;
	bool rx_lsb_first		= dai_ucfmt->rx_lsb_first;
	bool tx_lsb_first		= dai_ucfmt->tx_lsb_first;
	unsigned int mclk_fs		= priv->dai_props->mclk_fs;
	unsigned int mclk_fp0		= priv->dai_props->mclk_fp[0];
	unsigned int mclk_fp1		= priv->dai_props->mclk_fp[1];
	unsigned int cpu_pll_fs		= priv->dai_props->cpu_pll_fs;
	unsigned int codec_pllin_fs	= priv->dai_props->codec_pllin_fs;
	unsigned int codec_pllout_fs	= priv->dai_props->codec_pllout_fs;
	unsigned int data_late		= dai_ucfmt->data_late;
	int slots			= priv->dais->slots;
	int slot_width			= priv->dais->slot_width;

	int scanf_cnt;
	unsigned int scanf_num = 0;
	char scanf_str[8] = {0};
	unsigned int dai_fmt_tmp;
	bool set_sync = false;

	scanf_cnt = sscanf(buf, "%u %7s", &scanf_num, scanf_str);
	if (scanf_cnt <= 0)
		goto err;
	if (!((scanf_num == 0 && scanf_cnt == 1) || (scanf_num > 0 && scanf_cnt == 2)))
		goto err;

	if (scanf_num == 0) {
		priv->show_daifmt = true;
		return 0;
	}

	switch (scanf_num) {
	case 1:	/* set cpu_pll_fs */
		cpu_pll_fs = simple_strtoul(scanf_str, NULL, 10);
		if (cpu_pll_fs > 0)
			set_sync = true;
		break;
	case 2: /* set codec_pllin_fs */
		codec_pllin_fs = simple_strtoul(scanf_str, NULL, 10);
		if (codec_pllin_fs > 0)
			set_sync = true;
		break;
	case 3: /* set codec_pllout_fs */
		codec_pllout_fs = simple_strtoul(scanf_str, NULL, 10);
		if (codec_pllout_fs > 0)
			set_sync = true;
		break;
	case 4: /* set mclk_fs */
		mclk_fs = simple_strtoul(scanf_str, NULL, 10);
		if (mclk_fs > 0)
			set_sync = true;
		break;
	case 5: /* set mclk_fp(44.1k fp) */
		mclk_fp0 = simple_strtoul(scanf_str, NULL, 10);
		if (mclk_fp0 > 0)
			set_sync = true;
		break;
	case 6: /* set mclk_fp(48k fp) */
		mclk_fp1 = simple_strtoul(scanf_str, NULL, 10);
		if (mclk_fp1 > 0)
			set_sync = true;
		break;
	case 7: /* set dai_fmt -> FMT */
		dai_fmt_tmp = dai_fmt;
		dai_fmt &= ~SND_SOC_DAIFMT_FORMAT_MASK;
		if (!strncmp(scanf_str, "i2s", 3)) {
			dai_fmt |= SND_SOC_DAIFMT_FORMAT_MASK & SND_SOC_DAIFMT_I2S;
			set_sync = true;
		} else if (!strncmp(scanf_str, "right_j", 7)) {
			dai_fmt |= SND_SOC_DAIFMT_FORMAT_MASK & SND_SOC_DAIFMT_RIGHT_J;
			set_sync = true;
		} else if (!strncmp(scanf_str, "left_j", 6)) {
			dai_fmt |= SND_SOC_DAIFMT_FORMAT_MASK & SND_SOC_DAIFMT_LEFT_J;
			set_sync = true;
		} else if (!strncmp(scanf_str, "dsp_a", 5)) {
			dai_fmt |= SND_SOC_DAIFMT_FORMAT_MASK & SND_SOC_DAIFMT_DSP_A;
			set_sync = true;
		} else if (!strncmp(scanf_str, "dsp_b", 5)) {
			dai_fmt |= SND_SOC_DAIFMT_FORMAT_MASK & SND_SOC_DAIFMT_DSP_B;
			set_sync = true;
		} else {
			dai_fmt = dai_fmt_tmp;
		}
		break;
	case 8: /* set dai_fmt -> MASTER */
		dai_fmt_tmp = dai_fmt;
		dai_fmt &= ~SND_SOC_DAIFMT_MASTER_MASK;
		if (!strncmp(scanf_str, "CBM_CFM", 7)) {
			dai_fmt |= SND_SOC_DAIFMT_MASTER_MASK & SND_SOC_DAIFMT_CBM_CFM;
			set_sync = true;
		} else if (!strncmp(scanf_str, "CBS_CFM", 7)) {
			dai_fmt |= SND_SOC_DAIFMT_MASTER_MASK & SND_SOC_DAIFMT_CBS_CFM;
			set_sync = true;
		} else if (!strncmp(scanf_str, "CBM_CFS", 7)) {
			dai_fmt |= SND_SOC_DAIFMT_MASTER_MASK & SND_SOC_DAIFMT_CBM_CFS;
			set_sync = true;
		} else if (!strncmp(scanf_str, "CBS_CFS", 7)) {
			dai_fmt |= SND_SOC_DAIFMT_MASTER_MASK & SND_SOC_DAIFMT_CBS_CFS;
			set_sync = true;
		} else {
			dai_fmt = dai_fmt_tmp;
		}
		break;
	case 9: /* set dai_fmt -> INVERT */
		dai_fmt_tmp = dai_fmt;
		dai_fmt &= ~SND_SOC_DAIFMT_INV_MASK;
		if (!strncmp(scanf_str, "NB_NF", 5)) {
			dai_fmt |= SND_SOC_DAIFMT_INV_MASK & SND_SOC_DAIFMT_NB_NF;
			set_sync = true;
		} else if (!strncmp(scanf_str, "NB_IF", 5)) {
			dai_fmt |= SND_SOC_DAIFMT_INV_MASK & SND_SOC_DAIFMT_NB_IF;
			set_sync = true;
		} else if (!strncmp(scanf_str, "IB_NF", 5)) {
			dai_fmt |= SND_SOC_DAIFMT_INV_MASK & SND_SOC_DAIFMT_IB_NF;
			set_sync = true;
		} else if (!strncmp(scanf_str, "IB_IF", 5)) {
			dai_fmt |= SND_SOC_DAIFMT_INV_MASK & SND_SOC_DAIFMT_IB_IF;
			set_sync = true;
		} else {
			dai_fmt = dai_fmt_tmp;
		}
		break;
	case 10: /* set slots */
		slots = simple_strtoul(scanf_str, NULL, 10);
		if (slots > 0 && slots < 32 && (slots % 2 == 0))
			set_sync = true;
		break;
	case 11: /* set slot_width */
		slot_width = simple_strtoul(scanf_str, NULL, 10);
		if (slot_width == 16 || slot_width == 24 || slot_width == 32)
			set_sync = true;
		break;
	case 12: /* set MSB/LSB first select */
		if (!strncmp(scanf_str, "RM_TM", 5)) {
			rx_lsb_first = false;
			tx_lsb_first = false;
			set_sync = true;
		} else if (!strncmp(scanf_str, "RM_TL", 5)) {
			rx_lsb_first = false;
			tx_lsb_first = true;
			set_sync = true;
		} else if (!strncmp(scanf_str, "RL_TM", 5)) {
			rx_lsb_first = true;
			tx_lsb_first = false;
			set_sync = true;
		} else if (!strncmp(scanf_str, "RL_TL", 5)) {
			rx_lsb_first = true;
			tx_lsb_first = true;
			set_sync = true;
		}
		break;
	case 13: /* set data late */
		data_late = simple_strtol(scanf_str, NULL, 10);
		if (dai_ucfmt->fmt == SND_SOC_DAIFMT_DSP_A || dai_ucfmt->fmt == SND_SOC_DAIFMT_DSP_B) {
			if (data_late <= 3)
				set_sync = true;
		}
		break;
	default:
		SND_LOG_ERR("options invalid, %u\n", scanf_num);
		set_sync = false;
	}

	/* sync setting */
	if (scanf_num > 0) {
		if (!set_sync) {
			pr_err("option val invaild\n");
			return count;
		}
		card->dai_link->dai_fmt		= dai_fmt;
		priv->dai_props->mclk_fs	= mclk_fs;
		priv->dai_props->mclk_fp[0]	= mclk_fp0;
		priv->dai_props->mclk_fp[1]	= mclk_fp1;
		priv->dai_props->cpu_pll_fs	= cpu_pll_fs;
		priv->dai_props->codec_pllin_fs	= codec_pllin_fs;
		priv->dai_props->codec_pllout_fs	= codec_pllout_fs;
		priv->dais->slots		= slots;
		priv->dais->slot_width		= slot_width;
		dai_ucfmt->rx_lsb_first 	= rx_lsb_first;
		dai_ucfmt->tx_lsb_first 	= tx_lsb_first;
		dai_ucfmt->data_late		= data_late;
		dai_ucfmt->fmt			= dai_fmt & SND_SOC_DAIFMT_FORMAT_MASK;
	}

	return 0;

err:
	pr_err("wrong format: %s\n", buf);
	return -1;
}
#endif

static int simple_soc_probe(struct snd_soc_card *card)
{
	struct asoc_simple_priv *priv = snd_soc_card_get_drvdata(card);
#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	struct snd_sunxi_dump *dump;
#endif
	int ret;

	SND_LOG_DEBUG("\n");

	if (!priv) {
		SND_LOG_ERR("snd_soc_card priv invailed\n");
		return -1;
	}

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	if (asoc_simple_is_i2sdai(priv->dais)) {
		dump = &priv->dump;
		snprintf(priv->module_name, 32, "%s-%s", "machine", card->name);
		dump->name = priv->module_name;
		dump->priv = priv;
		dump->dump_version = snd_sunxi_dump_version;
		dump->dump_help = snd_sunxi_dump_help;
		dump->dump_show = snd_sunxi_dump_show;
		dump->dump_store = snd_sunxi_dump_store;
		ret = snd_sunxi_dump_register(dump);
		if (ret)
			SND_LOG_WARN("snd_sunxi_dump_register failed\n");
	}
#endif

	ret = snd_sunxi_extparam_probe(card->name, EXTPARAM_ID_DAI_UCFMT);
	if (ret) {
		SND_LOG_ERR("extparam(EXTPARAM_ID_DAI_UCFMT) probe failed\n");
		return ret;
	}
	ret = snd_sunxi_extparam_probe(card->name, EXTPARAM_ID_HDMI_FMT);
	if (ret) {
		SND_LOG_ERR("extparam(EXTPARAM_ID_HDMI_FMT) probe failed\n");
		return ret;
	}

	if (priv->jack_support) {
		ret = snd_sunxi_jack_register(card, priv->jack_support);
		if (ret < 0) {
			SND_LOG_ERR("jack init failed\n");
			return ret;
		}
	}

	return 0;
}

static int simple_soc_remove(struct snd_soc_card *card)
{
	struct asoc_simple_priv *priv = snd_soc_card_get_drvdata(card);

	SND_LOG_DEBUG("\n");

	if (!priv) {
		SND_LOG_ERR("snd_soc_card priv invailed\n");
		return -1;
	}

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	if (asoc_simple_is_i2sdai(priv->dais))
		snd_sunxi_dump_unregister(&priv->dump);
#endif

	snd_sunxi_extparam_remove(card->name, EXTPARAM_ID_DAI_UCFMT);
	snd_sunxi_extparam_remove(card->name, EXTPARAM_ID_HDMI_FMT);

	if (priv->jack_support)
		snd_sunxi_jack_unregister(card, priv->jack_support);

	return 0;
}

static int asoc_simple_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *top_np = dev->of_node;
	struct asoc_simple_priv *priv;
	struct snd_soc_card *card;
	int ret;

	SND_LOG_DEBUG("\n");

	/* Allocate the private data and the DAI link array */
	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	card = simple_priv_to_card(priv);
	card->owner		= THIS_MODULE;
	card->dev		= dev;
	card->probe		= simple_soc_probe;
	card->remove		= simple_soc_remove;

	ret = asoc_simple_init_priv(priv);
	if (ret < 0)
		return ret;

	if (top_np && of_device_is_available(top_np)) {
		ret = simple_parse_of(priv);
		if (ret < 0) {
			if (ret != -EPROBE_DEFER)
				SND_LOG_ERR("parse error %d\n", ret);
			goto err;
		}
	} else {
		SND_LOG_ERR("simple card dts available\n");
	}

	snd_soc_card_set_drvdata(card, priv);

	/* asoc_simple_debug_info(priv); */
	ret = devm_snd_soc_register_card(dev, card);
	if (ret < 0)
		goto err;

	return 0;
err:
	asoc_simple_clean_reference(card);

	return ret;
}

static int asoc_simple_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	SND_LOG_DEBUG("\n");

	return asoc_simple_clean_reference(card);
}

static const struct of_device_id snd_soc_sunxi_of_match[] = {
	{ .compatible = "allwinner," DRV_NAME, },
	{},
};
MODULE_DEVICE_TABLE(of, snd_soc_sunxi_of_match);

static struct platform_driver sunxi_soundcard_machine_driver = {
	.driver	= {
		.name		= DRV_NAME,
		.pm		= &snd_soc_pm_ops,
		.of_match_table	= snd_soc_sunxi_of_match,
	},
	.probe	= asoc_simple_probe,
	.remove	= asoc_simple_remove,
};

int __init sunxi_soundcard_machine_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_soundcard_machine_driver);
	if (ret != 0) {
		SND_LOG_ERR("platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_soundcard_machine_dev_exit(void)
{
	platform_driver_unregister(&sunxi_soundcard_machine_driver);
}

late_initcall(sunxi_soundcard_machine_dev_init);
module_exit(sunxi_soundcard_machine_dev_exit);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.6");
MODULE_DESCRIPTION("sunxi soundcard machine");
