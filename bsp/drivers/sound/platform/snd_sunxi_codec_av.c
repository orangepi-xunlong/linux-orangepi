// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2023, zhouxijing <zhouxijing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#define SUNXI_MODNAME		"sound-codec-av"
#include "snd_sunxi_log.h"
#include <linux/module.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/hdmi-codec.h>

#include "snd_sunxi_common.h"

#define DRV_NAME	"sunxi-snd-codec-av"

struct sunxi_codec {
	struct platform_device *pdev;

	struct hdmi_codec_daifmt daifmt;
	struct hdmi_codec_params params;
	struct hdmi_codec_pdata *pdata;
};

static int sunxi_codec_dai_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct hdmi_codec_daifmt *daifmt = &codec->daifmt;

	SND_LOG_DEBUG("\n");

	/* get dai mode */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		daifmt->fmt = HDMI_I2S;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		daifmt->fmt = HDMI_RIGHT_J;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		daifmt->fmt = HDMI_LEFT_J;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		daifmt->fmt = HDMI_DSP_A;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		daifmt->fmt = HDMI_DSP_B;
		break;
	default:
		SND_LOG_ERR("hdmi_codec_daifmt fmt set fail\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_codec_dai_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *params,
				     struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct hdmi_codec_params *codec_params = &codec->params;
	struct hdmi_codec_daifmt *daifmt = &codec->daifmt;
	struct hdmi_codec_pdata *pdata = codec->pdata;
	int ret;

	SND_LOG_DEBUG("\n");

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		codec_params->sample_width = 16;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
	case SNDRV_PCM_FORMAT_S24_3LE:
		codec_params->sample_width = 20;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		codec_params->sample_width = 24;
		break;
	default:
		return -EINVAL;
	}
	codec_params->channels = params_channels(params);
	codec_params->sample_rate = params_rate(params);

	ret = pdata->ops->hw_params(component->dev->parent, NULL, daifmt, codec_params);
	if (ret < 0) {
		SND_LOG_ERR("hdmi_codec_pdata hw params fail\n");
		return ret;
	}

	return 0;
}

static int sunxi_codec_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct hdmi_codec_pdata *pdata = codec->pdata;
	int ret;

	SND_LOG_DEBUG("\n");

	ret = pdata->ops->audio_startup(component->dev->parent, NULL);
	if (ret < 0) {
		SND_LOG_ERR("hdmi_codec_pdata startup fail\n");
		return ret;
	}

	return 0;
}

static int sunxi_codec_dai_mute_stream(struct snd_soc_dai *dai, int mute, int stream)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct hdmi_codec_pdata *pdata = codec->pdata;
	int ret;

	SND_LOG_DEBUG("\n");

	ret = pdata->ops->mute_stream(component->dev->parent, NULL, mute, stream);
	if (ret < 0) {
		SND_LOG_ERR("hdmi_codec_pdata mute stream fail\n");
		return ret;
	}

	return 0;
}

static void sunxi_codec_dai_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct hdmi_codec_pdata *pdata = codec->pdata;

	SND_LOG_DEBUG("\n");

	pdata->ops->audio_shutdown(component->dev->parent, NULL);
}

static const struct snd_soc_dai_ops sunxi_codec_dai_ops = {
	.set_fmt	= sunxi_codec_dai_set_fmt,
	.hw_params	= sunxi_codec_dai_hw_params,
	.startup	= sunxi_codec_dai_startup,
	.mute_stream	= sunxi_codec_dai_mute_stream,
	.shutdown	= sunxi_codec_dai_shutdown,
};

static struct snd_soc_dai_driver sunxi_codec_dai = {
	.name = DRV_NAME,
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 8,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S20_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S24_3LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &sunxi_codec_dai_ops,
};

static struct snd_soc_component_driver sunxi_codec_component_dev = {
	.name		= DRV_NAME,
};

static int sunxi_codec_dev_probe(struct platform_device *pdev)
{
	struct hdmi_codec_pdata *pdata = pdev->dev.platform_data;
	struct device *dev = &pdev->dev;
	struct sunxi_codec *codec;
	int ret;
	const char *devname = dev_name(dev);

	SND_LOG_DEBUG("device name:%s\n", devname);

	if (!pdata) {
		SND_LOG_ERR("No hdmi codec pdata\n");
		return -EINVAL;
	}

	if (!pdata->ops || !pdata->ops->hw_params || !pdata->ops->audio_startup ||
	    !pdata->ops->audio_shutdown || !pdata->ops->mute_stream) {
		SND_LOG_ERR("ops incomplete\n");
		return -EINVAL;
	}

	/* sunxi codec info */
	codec = devm_kzalloc(dev, sizeof(struct sunxi_codec), GFP_KERNEL);
	if (!codec) {
		SND_LOG_ERR("can't allocate sunxi codec av memory\n");
		return -ENOMEM;
	}
	codec->pdev = pdev;
	codec->pdata = pdata;
	dev_set_drvdata(dev, codec);

	/* alsa component register */
	ret = snd_soc_register_component(dev, &sunxi_codec_component_dev, &sunxi_codec_dai, 1);
	if (ret) {
		SND_LOG_ERR("codec-av component register failed\n");
		return -EINVAL;
	}

	SND_LOG_DEBUG("register codec-av success\n");

	return 0;
}

static int sunxi_codec_dev_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_codec *codec = dev_get_drvdata(dev);

	SND_LOG_DEBUG("\n");

	snd_soc_unregister_component(dev);

	devm_kfree(dev, codec);

	SND_LOG_DEBUG("unregister codec-hdmi success\n");

	return 0;
}

static struct platform_driver sunxi_codec_driver = {
	.driver = {
		.name = DRV_NAME,
	},
	.probe	= sunxi_codec_dev_probe,
	.remove	= sunxi_codec_dev_remove,
};

int __init sunxi_av_codec_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_codec_driver);
	if (ret != 0) {
		SND_LOG_ERR("platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_av_codec_dev_exit(void)
{
	platform_driver_unregister(&sunxi_codec_driver);
}

late_initcall(sunxi_av_codec_dev_init);
module_exit(sunxi_av_codec_dev_exit);

MODULE_AUTHOR("zhouxijing@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
MODULE_DESCRIPTION("sunxi soundcard codec of av");
