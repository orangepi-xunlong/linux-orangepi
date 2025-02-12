// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 KY
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
#include <linux/input.h>

#define DAI	"sound-dai"
#define CELL	"#sound-dai-cells"
#define PREFIX	"simple-audio-card,"


int ky_simple_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(substream);
	struct snd_soc_dai *sdai;
	struct asoc_simple_priv *priv = snd_soc_card_get_drvdata(rtd->card);
	struct simple_dai_props *props = simple_priv_to_props(priv, rtd->num);
	unsigned int mclk, mclk_fs = 0;
	int i, ret;

	if (props->mclk_fs)
		mclk_fs = props->mclk_fs;

	if (mclk_fs) {
		struct snd_soc_component *component;
		mclk = params_rate(params) * mclk_fs;

		/* Ensure sysclk is set on all components in case any
			* (such as platform components) are missed by calls to
			* snd_soc_dai_set_sysclk.
			*/
		for_each_rtd_components(rtd, i, component) {
			ret = snd_soc_component_set_sysclk(component, 0, 0,
				mclk, SND_SOC_CLOCK_IN);
			if (ret && ret != -ENOTSUPP)
				return ret;
		}

		for_each_rtd_codec_dais(rtd, i, sdai) {
			ret = snd_soc_dai_set_sysclk(sdai, 0, mclk, SND_SOC_CLOCK_IN);
			if (ret && ret != -ENOTSUPP)
				return ret;
		}

		for_each_rtd_cpu_dais(rtd, i, sdai) {
			ret = snd_soc_dai_set_sysclk(sdai, 0, mclk, SND_SOC_CLOCK_OUT);
			if (ret && ret != -ENOTSUPP)
				return ret;
		}
	}
	return 0;
}

static const struct snd_soc_ops simple_ops = {
	.hw_params      = ky_simple_hw_params,
};

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
	ret = snd_soc_of_get_dai_name(node, &dlc->dai_name, 0);
	if (ret < 0)
		return ret;
	dlc->of_node = args.np;
	if (is_single_link)
		*is_single_link = !args.args_count;
	return 0;
}

static int asoc_simple_parse_platform(struct device_node *node,
				      struct snd_soc_dai_link_component *dlc)
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

	/* dai_name is not required and may not exist for plat component */
	dlc->of_node = args.np;
	return 0;
}

static int asoc_simple_card_jack_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_component *component = asoc_rtd_to_codec(rtd, 0)->component;
	struct snd_soc_card *card = rtd->card;
	struct asoc_simple_priv *priv = snd_soc_card_get_drvdata(card);
	int ret;

	ret = snd_soc_card_jack_new_pins(rtd->card, "Headset Jack",
			SND_JACK_HEADSET | SND_JACK_BTN_0 |
			SND_JACK_BTN_1 | SND_JACK_BTN_2, &priv->hp_jack.jack,
			NULL, 0);
	if (ret) {
		dev_err(rtd->dev, "Headset Jack creation failed %d\n", ret);
		return ret;
	}

	snd_jack_set_key(priv->hp_jack.jack.jack, SND_JACK_BTN_0, KEY_PLAYPAUSE);
	snd_jack_set_key(priv->hp_jack.jack.jack, SND_JACK_BTN_1, KEY_VOLUMEUP);
	snd_jack_set_key(priv->hp_jack.jack.jack, SND_JACK_BTN_2, KEY_VOLUMEDOWN);

	snd_soc_component_set_jack(component, &priv->hp_jack.jack, NULL);
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
	unsigned int val = 0;

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

	//cpu dai
	ret = asoc_simple_parse_dai(cpu, dai_link->cpus, &single_cpu);
	if (ret < 0)
		goto dai_link_of_err;

	//codec dai
	ret = asoc_simple_parse_dai(codec, dai_link->codecs, NULL);
	if (ret < 0) {
		goto dai_link_of_err;
	}

	//platform
	ret = asoc_simple_parse_platform(plat, dai_link->platforms);
	if (ret < 0)
		goto dai_link_of_err;

	ret = asoc_simple_set_dailink_name(dev, dai_link,
					   "%s-%s",
					   dai_link->cpus->dai_name,
					   dai_link->codecs->dai_name);
	if (ret < 0)
		goto dai_link_of_err;

	if (of_property_read_bool(node, "ky,init-jack")) {
		dai_link->init = asoc_simple_card_jack_init;
	}

	dai_link->ops = &simple_ops;
	if (!of_property_read_u32(node, "ky,mclk-fs", &val)) {
		priv->dai_props->mclk_fs = val;
	} else {
		priv->dai_props->mclk_fs = 256;
	}
	asoc_simple_canonicalize_cpu(dai_link->cpus, single_cpu);
	asoc_simple_canonicalize_platform(dai_link->platforms, dai_link->cpus);

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

static int asoc_simple_card_probe(struct platform_device *pdev)
{
	struct asoc_simple_priv *priv;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct snd_soc_card *card;
	struct link_info *li;
	int ret;

	/* Allocate the private data and the DAI link array */
	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	li = devm_kzalloc(dev, sizeof(*li), GFP_KERNEL);
	if (!li)
		return -ENOMEM;

	card = simple_priv_to_card(priv);
	card->owner		= THIS_MODULE;
	card->dev		= dev;

	memset(li, 0, sizeof(struct link_info));

	/* Get the number of DAI links */
	if (np && of_get_child_by_name(np, PREFIX "dai-link")) {
		li->link = of_get_child_count(np);
	} else {
		li->link = 1;
		li->num[0].cpus		= 1;
		li->num[0].codecs	= 1;
		li->num[0].platforms	= 1;
	}

	ret = asoc_simple_init_priv(priv, li);
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
	asoc_simple_clean_reference(card);
	return 0;
}

static const struct of_device_id asoc_simple_of_match[] = {
	{ .compatible = "ky,simple-audio-card", },
	{},
};
MODULE_DEVICE_TABLE(of, asoc_simple_of_match);

static struct platform_driver asoc_simple_card = {
	.driver = {
		.name = "ky-audio-card",
		.pm = &snd_soc_pm_ops,
		.of_match_table = asoc_simple_of_match,
	},
	.probe = asoc_simple_card_probe,
	.remove = asoc_simple_card_remove,
};

static void __exit ky_snd_card_exit(void)
{
	platform_driver_unregister(&asoc_simple_card);
}
module_exit(ky_snd_card_exit);

static int ky_snd_card_init(void)
{
	return platform_driver_register(&asoc_simple_card);
}
late_initcall(ky_snd_card_init);

MODULE_DESCRIPTION("KY ASoC Machine Driver");
MODULE_LICENSE("GPL");

