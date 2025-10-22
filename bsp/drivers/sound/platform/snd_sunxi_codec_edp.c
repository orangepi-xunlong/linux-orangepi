// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2023, lijingpsw <lijingpsw@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#define SUNXI_MODNAME		"sound-codec-edp"
#include "snd_sunxi_log.h"
#include <linux/module.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <video/sunxi_edp.h>

#include "snd_sunxi_common.h"

#define DRV_NAME	"sunxi-snd-codec-edp"

struct sunxi_codec {
	struct platform_device *pdev;

	__edp_audio_func edp_func;
	edp_audio_t edp_para;
};

static void sunxi_codec_dai_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	int ret;

	SND_LOG_DEBUG("\n");

	ret = codec->edp_func.edp_audio_disable(0);
	if (ret)
		SND_LOG_ERR("edp_audio_disable failed\n");
}

static int sunxi_codec_dai_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	int ret;
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	SND_LOG_DEBUG("\n");

	ret = codec->edp_func.edp_audio_set_para(0, &codec->edp_para);
	if (ret) {
		SND_LOG_ERR("edp_audio_set_para failed\n");
		return ret;
	}

	ret = codec->edp_func.edp_audio_enable(0);
	if (ret) {
		SND_LOG_ERR("edp_audio_enable failed\n");
		return ret;
	}

	return 0;
}

static int sunxi_codec_dai_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *params,
				     struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	uint32_t sample_bit;

	SND_LOG_DEBUG("\n");

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		sample_bit = 16;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
	case SNDRV_PCM_FORMAT_S24_3LE:
		sample_bit = 20;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		sample_bit = 24;
		break;
	default:
		return -EINVAL;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		/* defalut: 0,I2S Interface */
		codec->edp_para.interface = 0;
		codec->edp_para.chn_cnt = params_channels(params);
		codec->edp_para.data_width = sample_bit;
		codec->edp_para.mute = 0;
	}

	return 0;
}

static const struct snd_soc_dai_ops sunxi_codec_dai_ops = {
	.hw_params	= sunxi_codec_dai_hw_params,
	.prepare	= sunxi_codec_dai_prepare,
	.shutdown	= sunxi_codec_dai_shutdown,
};

static struct snd_soc_dai_driver sunxi_codec_dai = {
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 8,
		.rates		= SNDRV_PCM_RATE_8000_192000,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S20_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S24_3LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &sunxi_codec_dai_ops,
};

static int sunxi_codec_component_probe(struct snd_soc_component *component)
{
	int ret;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	SND_LOG_DEBUG("\n");

	ret = snd_edp_get_func(&codec->edp_func);
	if (ret) {
		SND_LOG_ERR("snd_edp_get_func failed\n");
		return -1;
	}

	if (!codec->edp_func.edp_audio_enable) {
		SND_LOG_ERR("edp_audio_enable func is null\n");
		return -1;
	}

	if (!codec->edp_func.edp_audio_disable) {
		SND_LOG_ERR("edp_audio_disable func is null\n");
		return -1;
	}

	if (!codec->edp_func.edp_audio_set_para) {
		SND_LOG_ERR("edp_audio_set_para func is null\n");
		return -1;
	}

	return 0;
}

static struct snd_soc_component_driver sunxi_codec_component_dev = {
	.name		= DRV_NAME,
	.probe		= sunxi_codec_component_probe,
};

static int sunxi_codec_dev_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_codec *codec;

	SND_LOG_DEBUG("\n");

	/* sunxi codec info */
	codec = devm_kzalloc(dev, sizeof(*codec), GFP_KERNEL);
	if (!codec) {
		SND_LOG_ERR("can't allocate sunxi codec edp memory\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}
	dev_set_drvdata(dev, codec);

	/* alsa component register */
	ret = snd_soc_register_component(dev, &sunxi_codec_component_dev, &sunxi_codec_dai, 1);
	if (ret) {
		SND_LOG_ERR("edp-codec component register failed\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}

	SND_LOG_DEBUG("register codec-edp success\n");

	return 0;

err_devm_kzalloc:
	of_node_put(np);

	return ret;
}

static int sunxi_codec_dev_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_codec *codec = dev_get_drvdata(dev);

	SND_LOG_DEBUG("\n");

	snd_soc_unregister_component(dev);

	devm_kfree(dev, codec);
	of_node_put(pdev->dev.of_node);

	SND_LOG_DEBUG("unregister codec-edp success\n");

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

int __init sunxi_edp_codec_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_codec_driver);
	if (ret != 0) {
		SND_LOG_ERR("platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_edp_codec_dev_exit(void)
{
	platform_driver_unregister(&sunxi_codec_driver);
}

late_initcall(sunxi_edp_codec_dev_init);
module_exit(sunxi_edp_codec_dev_exit);

MODULE_AUTHOR("lijingpsw@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
MODULE_DESCRIPTION("sunxi soundcard codec of edp");
