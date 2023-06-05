/*
 * sound\soc\sunxi\snd_sunxi_mach.c
 * (C) Copyright 2021-2025
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * based on ${LINUX}/sound/soc/generic/simple-card.c
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <sound/soc.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_mach.h"

#define HLOG		"MACH"
#define DAI		"sound-dai"
#define CELL		"#sound-dai-cells"
#define PREFIX		"soundcard-mach,"

#define DRV_NAME	"sunxi-snd-mach"

static void asoc_simple_shutdown(struct snd_pcm_substream *substream)
{
}

static int asoc_simple_startup(struct snd_pcm_substream *substream)
{
	return 0;
}

static int asoc_simple_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(substream);

	struct snd_soc_dai *codec_dai = asoc_rtd_to_codec(rtd, 0);
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);

	struct asoc_simple_priv *priv = snd_soc_card_get_drvdata(rtd->card);
	struct snd_soc_dai_link *dai_link = simple_priv_to_link(priv, rtd->num);
	struct simple_dai_props *dai_props = simple_priv_to_props(priv, rtd->num);
	struct asoc_simple_dai *dais = priv->dais;
	unsigned int mclk;
	unsigned int cpu_pll_clk, codec_pll_clk;
	unsigned int cpu_bclk_ratio, codec_bclk_ratio;
	unsigned int freq_point;
	int cpu_clk_div, codec_clk_div;
	int ret = 0;

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
		break;
	case 11025:
	case 22050:
	case 44100:
	case 88200:
	case 176400:
		freq_point = 22579200;
		break;
	default:
		SND_LOG_ERR(HLOG, "Invalid rate %d\n", params_rate(params));
		return -EINVAL;
	}

	/* for cpudai pll clk */
	cpu_pll_clk	= freq_point * dai_props->cpu_pll_fs;
	codec_pll_clk	= freq_point * dai_props->codec_pll_fs;
	cpu_clk_div	= cpu_pll_clk / params_rate(params);
	codec_clk_div	= codec_pll_clk / params_rate(params);
	SND_LOG_DEBUG(HLOG, "freq point   : %u\n", freq_point);
	SND_LOG_DEBUG(HLOG, "cpu pllclk   : %u\n", cpu_pll_clk);
	SND_LOG_DEBUG(HLOG, "codec pllclk : %u\n", codec_pll_clk);
	SND_LOG_DEBUG(HLOG, "cpu clk_div  : %u\n", cpu_clk_div);
	SND_LOG_DEBUG(HLOG, "codec clk_div: %u\n", codec_clk_div);

	if (cpu_dai->driver->ops->set_pll) {
		ret = snd_soc_dai_set_pll(cpu_dai, substream->stream, 0,
					  cpu_pll_clk, cpu_pll_clk);
		if (ret) {
			SND_LOG_ERR(HLOG, "cpu_dai set pllclk failed\n");
			return ret;
		}
	}
	if (codec_dai->driver->ops->set_pll) {
		ret = snd_soc_dai_set_pll(codec_dai, substream->stream, 0,
					  codec_pll_clk, codec_pll_clk);
		if (ret) {
			SND_LOG_ERR(HLOG, "codec_dai set pllclk failed\n");
			return ret;
		}
	}

	if (cpu_dai->driver->ops->set_clkdiv) {
		ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, cpu_clk_div);
		if (ret) {
			SND_LOG_ERR(HLOG, "cpu_dai set clk_div failed\n");
			return ret;
		}
	}
	if (codec_dai->driver->ops->set_clkdiv) {
		ret = snd_soc_dai_set_clkdiv(codec_dai, 0, codec_clk_div);
		if (ret) {
			SND_LOG_ERR(HLOG, "cadec_dai set clk_div failed.\n");
			return ret;
		}
	}

	/* use for tdm only */
	if (!(dais->slots && dais->slot_width))
		return 0;

	/* for cpudai & codecdai mclk */
	if (dai_props->mclk_fp)
		mclk = (freq_point >> 1) * dai_props->mclk_fs;
	else
		mclk = params_rate(params) * dai_props->mclk_fs;
	cpu_bclk_ratio = cpu_pll_clk / (params_rate(params) * dais->slot_width * dais->slots);
	codec_bclk_ratio = codec_pll_clk / (params_rate(params) * dais->slot_width * dais->slots);
	SND_LOG_DEBUG(HLOG, "mclk            : %u\n", mclk);
	SND_LOG_DEBUG(HLOG, "cpu_bclk_ratio  : %u\n", cpu_bclk_ratio);
	SND_LOG_DEBUG(HLOG, "codec_bclk_ratio: %u\n", codec_bclk_ratio);

	if (cpu_dai->driver->ops->set_sysclk) {
		ret = snd_soc_dai_set_sysclk(cpu_dai, 0, mclk, SND_SOC_CLOCK_OUT);
		if (ret) {
			SND_LOG_ERR(HLOG, "cpu_dai set sysclk(mclk) failed\n");
			return ret;
		}
	}
	if (codec_dai->driver->ops->set_sysclk) {
		ret = snd_soc_dai_set_sysclk(codec_dai, 0, mclk, SND_SOC_CLOCK_IN);
		if (ret) {
			SND_LOG_ERR(HLOG, "cadec_dai set sysclk(mclk) failed\n");
			return ret;
		}
	}

	if (cpu_dai->driver->ops->set_bclk_ratio) {
		ret = snd_soc_dai_set_bclk_ratio(cpu_dai, cpu_bclk_ratio);
		if (ret) {
			SND_LOG_ERR(HLOG, "cpu_dai set bclk failed\n");
			return ret;
		}
	}
	if (codec_dai->driver->ops->set_bclk_ratio) {
		ret = snd_soc_dai_set_bclk_ratio(codec_dai, codec_bclk_ratio);
		if (ret) {
			SND_LOG_ERR(HLOG, "codec_dai set bclk failed\n");
			return ret;
		}
	}

	if (cpu_dai->driver->ops->set_fmt) {
		ret = snd_soc_dai_set_fmt(cpu_dai, dai_link->dai_fmt);
		if (ret) {
			SND_LOG_ERR(HLOG, "cpu dai set fmt failed\n");
			return ret;
		}
	}
	if (codec_dai->driver->ops->set_fmt) {
		ret = snd_soc_dai_set_fmt(codec_dai, dai_link->dai_fmt);
		if (ret) {
			SND_LOG_ERR(HLOG, "codec dai set fmt failed\n");
			return ret;
		}
	}

	if (cpu_dai->driver->ops->set_tdm_slot) {
		ret = snd_soc_dai_set_tdm_slot(cpu_dai, 0, 0, dais->slots, dais->slot_width);
		if (ret) {
			SND_LOG_ERR(HLOG, "cpu dai set tdm slot failed\n");
			return ret;
		}
	}
	if (codec_dai->driver->ops->set_tdm_slot) {
		ret = snd_soc_dai_set_tdm_slot(codec_dai, 0, 0, dais->slots, dais->slot_width);
		if (ret) {
			SND_LOG_ERR(HLOG, "codec dai set tdm slot failed\n");
			return ret;
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
	int i;
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dapm_context *dapm = &card->dapm;

	const struct snd_kcontrol_new *controls = card->controls;

	for (i = 0; i < card->num_controls; i++)
		if (controls[i].info == snd_soc_dapm_info_pin_switch)
			snd_soc_dapm_disable_pin(dapm,
				(const char *)controls[i].private_value);

	if (card->num_controls)
		snd_soc_dapm_sync(dapm);

	/* snd_soc_dai_set_sysclk(); */
	/* snd_soc_dai_set_tdm_slot(); */

	return 0;
}

static int simple_dai_link_of(struct device_node *node,
			      struct asoc_simple_priv *priv)
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

	prefix = PREFIX;
	top_np = node;

	snprintf(prop, sizeof(prop), "%scpu", prefix);
	cpu = of_get_child_by_name(top_np, prop);
	if (!cpu) {
		ret = -EINVAL;
		SND_LOG_ERR(HLOG, "Can't find %s DT node\n", prop);
		goto dai_link_of_err;
	}
	snprintf(prop, sizeof(prop), "%splat", prefix);
	plat = of_get_child_by_name(top_np, prop);

	snprintf(prop, sizeof(prop), "%scodec", prefix);
	codec = of_get_child_by_name(top_np, prop);
	if (!codec) {
		ret = -EINVAL;
		SND_LOG_ERR(HLOG, "Can't find %s DT node\n", prop);
		goto dai_link_of_err;
	}

	ret = asoc_simple_parse_daifmt(top_np, codec, prefix, &dai_link->dai_fmt);
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
	ret = asoc_simple_parse_codec(codec, dai_link, DAI, CELL);
	if (ret < 0) {
		if (ret == -EPROBE_DEFER)
			goto dai_link_of_err;
		dai_link->codecs->name = "snd-soc-dummy";
		dai_link->codecs->dai_name = "snd-soc-dummy-dai";
		/* dai_link->codecs->name = "sunxi-dummy-codec"; */
		/* dai_link->codecs->dai_name = "sunxi-dummy-codec-dai"; */
		SND_LOG_DEBUG(HLOG, "use dummy codec for simple card.\n");
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

	SND_LOG_DEBUG(HLOG, "name   : %s\n", dai_link->stream_name);
	SND_LOG_DEBUG(HLOG, "format : %x\n", dai_link->dai_fmt);
	SND_LOG_DEBUG(HLOG, "cpu    : %s\n", dai_link->cpus->name);
	SND_LOG_DEBUG(HLOG, "codec  : %s\n", dai_link->codecs->name);

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

	SND_LOG_DEBUG(HLOG, "\n");

	if (!top_np)
		return -EINVAL;

	/* DAPM widgets */
	ret = asoc_simple_parse_widgets(card, PREFIX);
	if (ret < 0)
		return ret;

	/* DAPM routes */
	ret = asoc_simple_parse_routing(card, PREFIX);
	if (ret < 0)
		return ret;

	/* DAPM pin_switches */
	ret = asoc_simple_parse_pin_switches(card, PREFIX);
	if (ret < 0)
		return ret;

	/* For single DAI link & old style of DT node */
	ret = simple_dai_link_of(top_np, priv);
	if (ret < 0)
		return ret;

	ret = asoc_simple_parse_card_name(card, PREFIX);
	return ret;
}

static int simple_soc_probe(struct snd_soc_card *card)
{
	return 0;
}

static int asoc_simple_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *top_np = dev->of_node;
	struct asoc_simple_priv *priv;
	struct snd_soc_card *card;
	int ret;

	/* Allocate the private data and the DAI link array */
	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	card = simple_priv_to_card(priv);
	card->owner		= THIS_MODULE;
	card->dev		= dev;
	card->probe		= simple_soc_probe;

	ret = asoc_simple_init_priv(priv);
	if (ret < 0)
		return ret;

	if (top_np && of_device_is_available(top_np)) {
		ret = simple_parse_of(priv);
		if (ret < 0) {
			if (ret != -EPROBE_DEFER)
				SND_LOG_ERR(HLOG, "parse error %d\n", ret);
			goto err;
		}
	} else {
		SND_LOG_ERR(HLOG, "simple card dts available\n");
	}

	snd_soc_card_set_drvdata(card, priv);

	/* asoc_simple_debug_info(priv); */
	ret = devm_snd_soc_register_card(dev, card);
	if (ret >= 0)
		return ret;
err:
	asoc_simple_clean_reference(card);

	return ret;
}

static int asoc_simple_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

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
		SND_LOG_ERR(HLOG, "platform driver register failed\n");
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
MODULE_DESCRIPTION("sunxi soundcard machine");
