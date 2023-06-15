/*
 * ASoC simple sound card support
 *
 * Copyright (C) 2020 Renesas Solutions Corp.
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * based on ${LINUX}/sound/soc/generic/simple-card.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <sound/jack.h>
#include <sound/simple_card.h>
#include <sound/soc-dai.h>
#include <sound/soc.h>

#define DAI	"sound-dai"
#define CELL	"#sound-dai-cells"
#define PREFIX	"simple-audio-card,"

static int asoc_simple_parse_dai(struct device_node *node,
		struct snd_soc_dai_link_component *dlc,
		int *is_single_link)
{
	struct of_phandle_args args;
	int ret;

	if (!node)
		return 0;

	/*
	 * Get node via "sound-dai = <&phandle port>"
	 * it will be used as xxx_of_node on soc_bind_dai_link()
	 */
	ret = of_parse_phandle_with_args(node, DAI, CELL, 0, &args);
	if (ret)
		return ret;

	/*
	 * FIXME
	 *
	 * Here, dlc->dai_name is pointer to CPU/Codec DAI name.
	 * If user unbinded CPU or Codec driver, but not for Sound Card,
	 * dlc->dai_name is keeping unbinded CPU or Codec
	 * driver's pointer.
	 *
	 * If user re-bind CPU or Codec driver again, ALSA SoC will try
	 * to rebind Card via snd_soc_try_rebind_card(), but because of
	 * above reason, it might can't bind Sound Card.
	 * Because Sound Card is pointing to released dai_name pointer.
	 *
	 * To avoid this rebind Card issue,
	 * 1) It needs to alloc memory to keep dai_name eventhough
	 *    CPU or Codec driver was unbinded, or
	 * 2) user need to rebind Sound Card everytime
	 *    if he unbinded CPU or Codec.
	 */
	ret = snd_soc_of_get_dai_name(node, &dlc->dai_name);
	if (ret < 0)
		return ret;

	dlc->of_node = args.np;

	if (is_single_link)
		*is_single_link = !args.args_count;

	return 0;
}

static int asoc_simple_card_startup(struct snd_pcm_substream *substream)
{
#if 0
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct asoc_simple_priv *priv =	snd_soc_card_get_drvdata(rtd->card);
	struct simple_dai_props *dai_props =
		simple_priv_to_props(priv, rtd->num);
	int ret;

	ret = clk_prepare_enable(dai_props->cpu_dai.clk);
	if (ret)
		return ret;

	ret = clk_prepare_enable(dai_props->codec_dai.clk);
	if (ret)
		clk_disable_unprepare(dai_props->cpu_dai.clk);

	return ret;
#endif
	return 0;
}

static void asoc_simple_card_shutdown(struct snd_pcm_substream *substream)
{
#if 0
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct asoc_simple_priv *priv =	snd_soc_card_get_drvdata(rtd->card);
	struct simple_dai_props *dai_props =
		simple_priv_to_props(priv, rtd->num);

	clk_disable_unprepare(dai_props->cpu_dai.clk);

	clk_disable_unprepare(dai_props->codec_dai.clk);
#endif
}

static int asoc_simple_card_hw_params(struct snd_pcm_substream *substream,
				      struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct asoc_simple_priv *priv = snd_soc_card_get_drvdata(rtd->card);
	struct snd_soc_dai_link *dai_link = simple_priv_to_link(priv, rtd->num);//num is idx????
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret, clk_div;
	unsigned int freq;

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
		freq = 24576000;
		break;
	case 11025:
	case 22050:
	case 44100:
	case 88200:
	case 176400:
		freq = 22579200;
		break;
	default:
		dev_err(rtd->dev, "Invalid rate %d\n", params_rate(params));
		return -EINVAL;
	}

	ret = snd_soc_dai_set_sysclk(codec_dai, 0, freq, SND_SOC_CLOCK_IN);
	if (ret && ret != -ENOTSUPP) {
		dev_err(rtd->dev, "cadec_dai set sysclk failed.\n");
		return ret;
	}
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0, freq, SND_SOC_CLOCK_OUT);
	if (ret && ret != -ENOTSUPP) {
		dev_err(rtd->dev, "cpu_dai set sysclk failed.\n");
		return ret;
	}

	ret = snd_soc_dai_set_pll(codec_dai, 0, 0, freq, freq);
	/* if (ret < 0) */
	/* 	dev_warn(rtd->dev, "codec_dai set set_pll failed.\n"); */

	/* set codec dai fmt */
	ret = snd_soc_dai_set_fmt(codec_dai, dai_link->dai_fmt);
	if (ret && ret != -ENOTSUPP)
		dev_warn(rtd->dev, "codec dai set fmt failed\n");

	/* set cpu dai fmt */
	ret = snd_soc_dai_set_fmt(cpu_dai, dai_link->dai_fmt);
	if (ret && ret != -ENOTSUPP)
		dev_warn(rtd->dev, "cpu dai set fmt failed\n");

	clk_div = freq/params_rate(params);

	if (cpu_dai->driver->ops->set_clkdiv) {
		ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, clk_div);
		if (ret < 0) {
			dev_err(rtd->dev, "set clkdiv failed.\n");
			return ret;
		}
	}

	if (codec_dai->driver->ops->set_clkdiv) {
		ret = snd_soc_dai_set_clkdiv(codec_dai, 0, clk_div);
		if (ret < 0) {
			dev_err(rtd->dev, "codec_dai set clkdiv failed\n");
			return ret;
		}
	}

	return 0;
}

static struct snd_soc_ops asoc_simple_card_ops = {
	.startup = asoc_simple_card_startup,
	.shutdown = asoc_simple_card_shutdown,
	.hw_params = asoc_simple_card_hw_params,
};

static int asoc_simple_card_dai_init(struct snd_soc_pcm_runtime *rtd)
{
#if 0
	struct simple_card_data *priv =	snd_soc_card_get_drvdata(rtd->card);
	struct snd_soc_dai *codec = rtd->codec_dai;
	struct snd_soc_dai *cpu = rtd->cpu_dai;
	struct simple_dai_props *dai_props =
		simple_priv_to_props(priv, rtd->num);
	int ret;

	ret = asoc_simple_card_init_dai(codec, &dai_props->codec_dai);
	if (ret < 0)
		return ret;

	ret = asoc_simple_card_init_dai(cpu, &dai_props->cpu_dai);
	if (ret < 0)
		return ret;

	ret = asoc_simple_card_init_hp(rtd->card, &priv->hp_jack, PREFIX);
	if (ret < 0)
		return ret;

	ret = asoc_simple_card_init_mic(rtd->card, &priv->mic_jack, PREFIX);
	if (ret < 0)
		return ret;
#endif
	return 0;
}

int asoc_simple_parse_daistream(struct device *dev,
				struct device_node *node,
				char *prefix,
				struct snd_soc_dai_link *dai_link)
{
	char prop[128];
	unsigned int dai_stream = 0;
	unsigned int playback_only = BIT(0);
	unsigned int capture_only = BIT(1);

	if (!prefix)
		prefix = "";

	/* check "[prefix]playback_only" */
	snprintf(prop, sizeof(prop), "%splayback_only", prefix);
	if (of_property_read_bool(node, prop))
		dai_stream |= playback_only;

	/* check "[prefix]capture_only" */
	snprintf(prop, sizeof(prop), "%scapture_only", prefix);
	if (of_property_read_bool(node, prop))
		dai_stream |= capture_only;

	if (dai_stream == (playback_only | capture_only)) {
		pr_err("unsupport stream\n");
		dai_link->playback_only = 0;
		dai_link->capture_only = 0;
	} else if (dai_stream == playback_only) {
		dai_link->playback_only = 1;
	} else if (dai_stream == capture_only) {
		dai_link->capture_only = 1;
	} else {
		dai_link->playback_only = 0;
		dai_link->capture_only = 0;
	}

	return 0;
}

static int asoc_simple_card_dai_link_of(struct device_node *node,
					struct asoc_simple_priv *priv,
					int idx,
					bool is_top_level_node)
{
	struct device *dev = simple_priv_to_dev(priv);
	struct snd_soc_dai_link *dai_link = simple_priv_to_link(priv, idx);
	struct device_node *cpu = NULL;
	struct device_node *plat = NULL;
	struct device_node *codec = NULL;
	char prop[128];
	char *prefix = "";
	int ret, single_cpu;

	/* For single DAI link & old style of DT node */
	if (is_top_level_node)
		prefix = PREFIX;

	snprintf(prop, sizeof(prop), "%scpu", prefix);
	cpu = of_get_child_by_name(node, prop);

	if (!cpu) {
		ret = -EINVAL;
		dev_err(dev, "%s: Can't find %s DT node\n", __func__, prop);
		goto dai_link_of_err;
	}

	snprintf(prop, sizeof(prop), "%splat", prefix);
	plat = of_get_child_by_name(node, prop);

	snprintf(prop, sizeof(prop), "%scodec", prefix);
	codec = of_get_child_by_name(node, prop);

	if (!codec) {
		ret = -EINVAL;
		dev_err(dev, "%s: Can't find %s DT node\n", __func__, prop);
		goto dai_link_of_err;
	}

	ret = asoc_simple_parse_daistream(dev, node, prefix, dai_link);
	if (ret < 0)
		goto dai_link_of_err;

	ret = asoc_simple_parse_daifmt(dev, node, codec,
				       prefix, &dai_link->dai_fmt);
	if (ret < 0)
		goto dai_link_of_err;

	ret = asoc_simple_parse_cpu(cpu, dai_link, &single_cpu);
	if (ret < 0)
		goto dai_link_of_err;

	ret = asoc_simple_parse_codec(codec, dai_link);
	if (ret < 0) {
		dai_link->codecs->name = "snd-soc-dummy";
		dai_link->codecs->dai_name = "snd-soc-dummy-dai";
	}

	ret = asoc_simple_parse_platform(plat, dai_link);
	if (ret < 0)
		goto dai_link_of_err;

	ret = asoc_simple_set_dailink_name(dev, dai_link,
					   "%s-%s",
					   dai_link->cpus->dai_name,
					   dai_link->codecs->dai_name);
	if (ret < 0)
		goto dai_link_of_err;

	dai_link->ops = &asoc_simple_card_ops;
	dai_link->init = asoc_simple_card_dai_init;

	dev_dbg(dev, "\tname : %s\n", dai_link->stream_name);
	dev_dbg(dev, "\tformat : %04x\n", dai_link->dai_fmt);
	dev_dbg(dev, "\tcpu : %s \n",
		dai_link->cpus->name);
	dev_dbg(dev, "\tcodec : %s \n",
		dai_link->codecs->name);

	asoc_simple_canonicalize_cpu(dai_link, single_cpu);
	asoc_simple_canonicalize_platform(dai_link);

dai_link_of_err:
	of_node_put(cpu);
	of_node_put(codec);

	return ret;
}

static int asoc_simple_card_parse_of(struct device_node *node,
				     struct asoc_simple_priv *priv)
{
	struct device *dev = simple_priv_to_dev(priv);
	struct device_node *dai_link;
	int ret;

	if (!node)
		return -EINVAL;

	/* The off-codec widgets */
	ret = asoc_simple_parse_widgets(&priv->snd_card, PREFIX);
	if (ret < 0)
		return ret;

	/* DAPM routes */
	ret = asoc_simple_parse_routing(&priv->snd_card, PREFIX);
	if (ret < 0)
		return ret;

	ret = asoc_simple_parse_pin_switches(&priv->snd_card, PREFIX);
	if (ret < 0)
		return ret;

	dai_link = of_get_child_by_name(node, PREFIX "dai-link");
	/* Single/Muti DAI link(s) & New style of DT node */
	if (dai_link) {
		struct device_node *np = NULL;
		int i = 0;

		for_each_child_of_node(node, np) {
			dev_dbg(dev, "\tlink %d:\n", i);
			ret = asoc_simple_card_dai_link_of(np, priv,
							   i, false);
			if (ret < 0) {
				of_node_put(np);
				goto card_parse_end;
			}
			i++;
		}
	} else {
		/* For single DAI link & old style of DT node */
		ret = asoc_simple_card_dai_link_of(node, priv, 0, true);
		if (ret < 0)
			goto card_parse_end;
	}

	ret = asoc_simple_parse_card_name(&priv->snd_card, PREFIX);

card_parse_end:
	of_node_put(dai_link);

	return ret;
}

static int sunxi_hdmiaudio_set_audio_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct asoc_simple_priv *priv =
				snd_soc_card_get_drvdata(card);

	priv->hdmi_format = ucontrol->value.integer.value[0];
	return 0;
}

static int sunxi_hdmiaudio_get_audio_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct asoc_simple_priv *priv =
				snd_soc_card_get_drvdata(card);

	ucontrol->value.integer.value[0] = priv->hdmi_format;
	return 0;
}

static const char *hdmiaudio_format_function[] = {"NULL", "PCM", "AC3",
		"MPEG1", "MP3", "MPEG2", "AAC", "DTS", "ATRAC", "ONE_BIT_AUDIO",
		"DOLBY_DIGITAL_PLUS", "DTS_HD", "MAT", "WMAPRO"};

static const struct soc_enum hdmiaudio_format_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(hdmiaudio_format_function),
			hdmiaudio_format_function),
};

/* pcm dts ac3 Audio Mode Select */
static const struct snd_kcontrol_new sunxi_hdmiaudio_controls[] = {
	SOC_ENUM_EXT("hdmi audio format Function", hdmiaudio_format_enum[0],
		sunxi_hdmiaudio_get_audio_mode, sunxi_hdmiaudio_set_audio_mode),
};

static int simple_soc_probe(struct snd_soc_card *card)
{
	/* struct asoc_simple_priv *priv = snd_soc_card_get_drvdata(card); */
	int ret;

	if (strstr(card->name, "sndhdmi")) {
		ret = snd_soc_add_card_controls(card, sunxi_hdmiaudio_controls,
					ARRAY_SIZE(sunxi_hdmiaudio_controls));
		if (ret)
			dev_warn(card->dev,
				"Failed to register audio hdmi mode control.\n");
	}
#if 0
	ret = asoc_simple_init_hp(card, &priv->hp_jack, PREFIX);
	if (ret < 0)
		return ret;

	ret = asoc_simple_init_mic(card, &priv->mic_jack, PREFIX);
	if (ret < 0)
		return ret;
#endif
	return 0;
}

static int asoc_simple_card_probe(struct platform_device *pdev)
{
	struct asoc_simple_priv *priv;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct snd_soc_card *card;
	struct link_info li;
	int ret;

	/* Allocate the private data and the DAI link array */
	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	card = simple_priv_to_card(priv);
	card->owner		= THIS_MODULE;
	card->dev		= dev;
	card->probe		= simple_soc_probe;

	memset(&li, 0, sizeof(li));

	/* Get the number of DAI links */
	if (np && of_get_child_by_name(np, PREFIX "dai-link"))
		li.link = of_get_child_count(np);
	else
		li.link = 1;

	ret = asoc_simple_init_priv(priv, &li);
	if (ret < 0)
		return ret;

	if (np && of_device_is_available(np)) {

		ret = asoc_simple_card_parse_of(np, priv);
		if (ret < 0) {
			if (ret != -EPROBE_DEFER)
				dev_err(dev, "parse error %d\n", ret);
			goto err;
		}

	} else {
		dev_err(dev, "simple card dts available\n");
	}

	snd_soc_card_set_drvdata(&priv->snd_card, priv);

	/* asoc_simple_debug_info(priv); */
	ret = devm_snd_soc_register_card(&pdev->dev, &priv->snd_card);
	if (ret >= 0)
		return ret;
err:
	asoc_simple_clean_reference(&priv->snd_card);

	return ret;
}

static int asoc_simple_card_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	/* struct asoc_simple_priv *priv = snd_soc_card_get_drvdata(card); */

	/* asoc_simple_card_remove_jack(&priv->hp_jack); */
	/* asoc_simple_card_remove_jack(&priv->mic_jack); */

	return asoc_simple_clean_reference(card);
}

static const struct of_device_id asoc_simple_of_match[] = {
	{ .compatible = "sunxi,simple-audio-card", },
	{},
};
MODULE_DEVICE_TABLE(of, asoc_simple_of_match);

static struct platform_driver asoc_simple_card = {
	.driver = {
		.name = "sunxi-audio-card",
		.pm = &snd_soc_pm_ops,
		.of_match_table = asoc_simple_of_match,
	},
	.probe = asoc_simple_card_probe,
	.remove = asoc_simple_card_remove,
};

module_platform_driver(asoc_simple_card);

MODULE_AUTHOR("bantao@allwinnertech.com>");
MODULE_ALIAS("platform:asoc-simple-card");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ASoC Simple Sound Card");
