/*
 * sound\soc\sunxi\snd_sunxi_dummy_codec.c
 * (C) Copyright 2021-2025
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <sound/soc.h>

#include "snd_sunxi_log.h"

#define HLOG			"CODEC"

#define DUMMY_CARD_NAME		"sunxi-dummy-codec"
#define DUMMY_CARD_DAI_NAME	"sunxi-dummy-codec-dai"

static const struct snd_soc_component_driver sunxi_dummy_codec = {
};

/*
 * The dummy CODEC is only meant to be used in situations where there is no
 * actual hardware.
 *
 * If there is actual hardware even if it does not have a control bus
 * the hardware will still have constraints like supported samplerates, etc.
 * which should be modelled. And the data flow graph also should be modelled
 * using DAPM.
 */
static struct snd_soc_dai_driver sunxi_dummy_codec_dai = {
	.name = DUMMY_CARD_DAI_NAME,
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 16,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.stream_name	= "Capture",
		.channels_min	= 1,
		.channels_max	= 16,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
};

static int sunxi_dummy_codec_probe(struct platform_device *pdev)
{
	int ret;

	ret = devm_snd_soc_register_component(&pdev->dev,
					      &sunxi_dummy_codec,
					      &sunxi_dummy_codec_dai, 1);

	if (ret) {
		SND_LOG_ERR(HLOG, "dummy-codec component register failed\n");
		return ret;
	}

	SND_LOG_INFO(HLOG, "register dummy-codec codec success\n");

	return 0;
}

static struct platform_driver sunxi_dummy_codec_driver = {
	.driver = {
		.name = DUMMY_CARD_NAME,
	},
	.probe = sunxi_dummy_codec_probe,
};

static struct platform_device *sunxi_dummy_codec_dev;

int __init sunxi_dummy_codec_util_init(void)
{
	int ret;

	sunxi_dummy_codec_dev =
		platform_device_register_simple(DUMMY_CARD_NAME, -1, NULL, 0);
	if (IS_ERR(sunxi_dummy_codec_dev)) {
		SND_LOG_ERR(HLOG, "platform device register simple failed\n");
		return PTR_ERR(sunxi_dummy_codec_dev);
	}

	ret = platform_driver_register(&sunxi_dummy_codec_driver);
	if (ret != 0) {
		platform_device_unregister(sunxi_dummy_codec_dev);
		SND_LOG_ERR(HLOG, "platform driver register failed\n");
	}

	return ret;
}

void __exit sunxi_dummy_codec_util_exit(void)
{
	platform_driver_unregister(&sunxi_dummy_codec_driver);
	platform_device_unregister(sunxi_dummy_codec_dev);
}

late_initcall(sunxi_dummy_codec_util_init);
module_exit(sunxi_dummy_codec_util_exit);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sunxi soundcard codec of dummy-codec");
