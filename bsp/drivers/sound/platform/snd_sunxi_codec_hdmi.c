// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2023,
 * Dby <dby@allwinnertech.com>
 * huhaoxin <huhaoxin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#define SUNXI_MODNAME		"sound-codec-hdmi"
#include "snd_sunxi_log.h"
#include <linux/module.h>
#include <linux/extcon.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <video/drv_hdmi.h>

#include "snd_sunxi_common.h"

#define DRV_NAME	"sunxi-snd-codec-hdmi"

struct sunxi_hdmi_priv {
	hdmi_audio_t hdmi_para;
	bool update_param;
};

struct sunxi_hdmi_extcon {
	struct extcon_dev *extdev;
	struct notifier_block hdmi_nb;
};

struct sunxi_codec {
	struct platform_device *pdev;

	struct sunxi_hdmi_extcon hdmi_extcon;
	enum HDMI_FORMAT hdmi_fmt;
};

static __audio_hdmi_func g_hdmi_func;
static struct sunxi_hdmi_priv g_hdmi_priv;

int sunxi_hdmi_plugin_notifier(struct notifier_block *nb, unsigned long event, void *ptr)
{
	static hdmi_audio_t *hdmi_para = &g_hdmi_priv.hdmi_para;

	(void)nb;
	(void)ptr;

	SND_LOG_DEBUG("\n");

	if (event) {
		g_hdmi_func.hdmi_set_audio_para(hdmi_para);
		if (g_hdmi_func.hdmi_audio_enable(1, 1))
			SND_LOG_ERR("hdmi_audio_enable failed\n");
	}

	return NOTIFY_DONE;
}

int sunxi_hdmi_extcon_init(struct sunxi_codec *codec)
{
	int ret;
	struct device_node *np;
	struct platform_device *pdev;
	struct sunxi_hdmi_extcon *hdmi_extcon;

	SND_LOG_DEBUG("\n");

	if (!codec) {
		SND_LOG_ERR("codec is invaild\n");
		return -1;
	}

	pdev = codec->pdev;
	hdmi_extcon = &codec->hdmi_extcon;
	np = pdev->dev.of_node;
	if (of_property_read_bool(np, "extcon")) {
		hdmi_extcon->extdev = extcon_get_edev_by_phandle(&pdev->dev, 0);
		if (IS_ERR(hdmi_extcon->extdev)) {
			SND_LOG_ERR("get extcon dev failed\n");
			return -1;
		}
	} else {
		SND_LOG_ERR("get extcon failed\n");
		return -1;
	}
	hdmi_extcon->hdmi_nb.notifier_call = sunxi_hdmi_plugin_notifier;
	ret = extcon_register_notifier(hdmi_extcon->extdev, EXTCON_DISP_HDMI,
				       &hdmi_extcon->hdmi_nb);
	if (ret < 0) {
		SND_LOG_ERR("register hdmi notifier failed\n");
		return -1;
	}
	return 0;
}

void sunxi_hdmi_extcon_exit(struct sunxi_codec *codec)
{
	struct sunxi_hdmi_extcon *hdmi_extcon;

	SND_LOG_DEBUG("\n");

	if (!codec) {
		SND_LOG_ERR("codec is invaild\n");
		return;
	}
	hdmi_extcon = &codec->hdmi_extcon;

	extcon_unregister_notifier(hdmi_extcon->extdev, EXTCON_DISP_HDMI, &hdmi_extcon->hdmi_nb);
}

static int sunxi_data_fmt_get_data_fmt(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	ucontrol->value.integer.value[0] = codec->hdmi_fmt;

	return 0;
}

static int sunxi_data_fmt_set_data_fmt(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	codec->hdmi_fmt = ucontrol->value.integer.value[0];

	return 0;
}

static const char *data_fmt[] = {
	"NULL", "PCM", "AC3", "MPEG1", "MP3", "MPEG2", "AAC", "DTS", "ATRAC",
	"ONE_BIT_AUDIO", "DOLBY_DIGITAL_PLUS", "DTS_HD", "MAT", "DST", "WMAPRO"
};
static SOC_ENUM_SINGLE_EXT_DECL(data_fmt_enum, data_fmt);
static const struct snd_kcontrol_new data_fmt_controls[] = {
	SOC_ENUM_EXT("audio data format", data_fmt_enum,
		     sunxi_data_fmt_get_data_fmt,
		     sunxi_data_fmt_set_data_fmt),
};

int snd_sunxi_hdmi_add_controls(struct snd_soc_component *component)
{
	int ret;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	if (!component) {
		SND_LOG_ERR("component is err\n");
		return -1;
	}

	codec->hdmi_fmt = HDMI_FMT_PCM;

	ret = snd_soc_add_component_controls(component,
					     data_fmt_controls,
					     ARRAY_SIZE(data_fmt_controls));
	if (ret)
		SND_LOG_ERR("add kcontrols failed\n");

	return 0;
}

int snd_sunxi_hdmi_init(void)
{
	int ret;

	SND_LOG_DEBUG("\n");

	ret = snd_hdmi_get_func(&g_hdmi_func);
	if (ret) {
		SND_LOG_ERR("get hdmi audio func failed\n");
		return -1;
	}

	if (!g_hdmi_func.hdmi_audio_enable) {
		SND_LOG_ERR("hdmi_audio_enable func is null\n");
		return -1;
	}
	if (!g_hdmi_func.hdmi_set_audio_para) {
		SND_LOG_ERR("hdmi_set_audio_para func is null\n");
		return -1;
	}

	return 0;
}

int snd_sunxi_hdmi_hw_params(struct snd_pcm_hw_params *params,
			     enum HDMI_FORMAT hdmi_fmt)
{
	static hdmi_audio_t hdmi_para_tmp;
	static hdmi_audio_t *hdmi_para = &g_hdmi_priv.hdmi_para;

	SND_LOG_DEBUG("\n");

	hdmi_para_tmp.sample_rate = params_rate(params);
	hdmi_para_tmp.channel_num = params_channels(params);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		hdmi_para_tmp.sample_bit = 16;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		hdmi_para_tmp.sample_bit = 24;
		break;
	default:
		return -EINVAL;
	}
	if (hdmi_fmt > HDMI_FMT_PCM) {
		hdmi_para_tmp.sample_bit = 24;
	}

	if (hdmi_para_tmp.channel_num == 8)
		hdmi_para_tmp.ca = 0x13;
	else if (hdmi_para_tmp.channel_num == 6)
		hdmi_para_tmp.ca = 0x0b;
	else if ((hdmi_para_tmp.channel_num >= 3))
		hdmi_para_tmp.ca = 0x1f;
	else
		hdmi_para_tmp.ca = 0x0;

	hdmi_para_tmp.data_raw = hdmi_fmt;

	if (hdmi_para_tmp.sample_rate	!= hdmi_para->sample_rate ||
	    hdmi_para_tmp.channel_num	!= hdmi_para->channel_num ||
	    hdmi_para_tmp.sample_bit	!= hdmi_para->sample_bit ||
	    hdmi_para_tmp.ca		!= hdmi_para->ca ||
	    hdmi_para_tmp.data_raw	!= hdmi_para->data_raw) {
		g_hdmi_priv.update_param = 1;
		hdmi_para->sample_rate = hdmi_para_tmp.sample_rate;
		hdmi_para->channel_num = hdmi_para_tmp.channel_num;
		hdmi_para->sample_bit = hdmi_para_tmp.sample_bit;
		hdmi_para->ca = hdmi_para_tmp.ca;
		hdmi_para->data_raw = hdmi_para_tmp.data_raw;
	}

	return 0;
}

int snd_sunxi_hdmi_prepare(void)
{
	static hdmi_audio_t *hdmi_para = &g_hdmi_priv.hdmi_para;

	SND_LOG_DEBUG("\n");

	if (!g_hdmi_priv.update_param)
		return 0;

	SND_LOG_DEBUG("hdmi audio update info\n");
	SND_LOG_DEBUG("data raw : %d\n", hdmi_para->data_raw);
	SND_LOG_DEBUG("bit      : %d\n", hdmi_para->sample_bit);
	SND_LOG_DEBUG("channel  : %d\n", hdmi_para->channel_num);
	SND_LOG_DEBUG("rate     : %d\n", hdmi_para->sample_rate);

	g_hdmi_func.hdmi_set_audio_para(hdmi_para);
	if (g_hdmi_func.hdmi_audio_enable(1, 1)) {
		SND_LOG_ERR("hdmi_audio_enable failed\n");
		return -1;
	}

	/* to avoid silence in some TV when keytone test for hdmi */
	g_hdmi_priv.update_param = 0;

	return 0;
}

void snd_sunxi_hdmi_shutdown(void)
{
	SND_LOG_DEBUG("\n");

	memset(&g_hdmi_priv, 0, sizeof(struct sunxi_hdmi_priv));

	if (g_hdmi_func.hdmi_audio_enable)
		g_hdmi_func.hdmi_audio_enable(0, 1);
}

static void sunxi_codec_dai_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	SND_LOG_DEBUG("\n");
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		snd_sunxi_hdmi_shutdown();
}

static int sunxi_codec_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	int ret;

	SND_LOG_DEBUG("\n");

	ret = snd_sunxi_extparam_set_state_sync(component->card->name, EXTPARAM_ID_HDMI_FMT,
						(void *)&codec->hdmi_fmt);
	if (ret) {
		SND_LOG_ERR("extparam set state sync failed\n");
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
	int ret;

	SND_LOG_DEBUG("\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		SND_LOG_DEBUG("hdmi fmt -> %d\n", codec->hdmi_fmt);
		ret = snd_sunxi_hdmi_hw_params(params, codec->hdmi_fmt);
		if (ret) {
			SND_LOG_ERR("hdmi audio hw_params set failed\n");
			return ret;
		}
	}

	return 0;
}

static int sunxi_codec_dai_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	int ret;

	SND_LOG_DEBUG("\n");
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		ret = snd_sunxi_hdmi_prepare();
		if (ret) {
			SND_LOG_ERR("hdmi audio prepare failed\n");
			return ret;
		}
	}

	return 0;
}

static const struct snd_soc_dai_ops sunxi_codec_dai_ops = {
	.startup	= sunxi_codec_dai_startup,
	.hw_params	= sunxi_codec_dai_hw_params,
	.prepare	= sunxi_codec_dai_prepare,
	.shutdown	= sunxi_codec_dai_shutdown,
};

static struct snd_soc_dai_driver sunxi_codec_dai = {
	.name = DRV_NAME,
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 8,
		.rates		= SNDRV_PCM_RATE_8000_192000,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S20_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.stream_name	= "Capture",
		.channels_min	= 1,
		.channels_max	= 8,
		.rates		= SNDRV_PCM_RATE_8000_192000,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S20_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &sunxi_codec_dai_ops,
};

static int sunxi_codec_component_probe(struct snd_soc_component *component)
{
	int ret;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	SND_LOG_DEBUG("\n");

	ret = snd_sunxi_hdmi_init();
	if (ret) {
		SND_LOG_ERR("hdmi audio init failed\n");
		return -1;
	}

	/* init hdmi extcon */
	ret = sunxi_hdmi_extcon_init(codec);
	if (ret) {
		SND_LOG_ERR("hdmi extcon init failed\n");
	}

	ret = snd_sunxi_hdmi_add_controls(component);
	if (ret) {
		SND_LOG_ERR("add hdmiaudio kcontrols failed\n");
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
	codec = devm_kzalloc(dev, sizeof(struct sunxi_codec), GFP_KERNEL);
	if (!codec) {
		SND_LOG_ERR("can't allocate sunxi codec hdmi memory\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}
	dev_set_drvdata(dev, codec);
	codec->pdev = pdev;

	/* alsa component register */
	ret = snd_soc_register_component(dev, &sunxi_codec_component_dev, &sunxi_codec_dai, 1);
	if (ret) {
		SND_LOG_ERR("hdmi-codec component register failed\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}

	SND_LOG_ERR("register codec-hdmi success\n");

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

	sunxi_hdmi_extcon_exit(codec);
	snd_soc_unregister_component(dev);

	devm_kfree(dev, codec);
	of_node_put(pdev->dev.of_node);

	SND_LOG_ERR("unregister codec-hdmi success\n");

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

int __init sunxi_hdmi_codec_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_codec_driver);
	if (ret != 0) {
		SND_LOG_ERR("platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_hdmi_codec_dev_exit(void)
{
	platform_driver_unregister(&sunxi_codec_driver);
}

late_initcall(sunxi_hdmi_codec_dev_init);
module_exit(sunxi_hdmi_codec_dev_exit);

MODULE_AUTHOR("huhaoxin@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.5");
MODULE_DESCRIPTION("sunxi soundcard codec of hdmi");
