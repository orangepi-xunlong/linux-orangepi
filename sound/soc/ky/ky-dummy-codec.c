// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 KY
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <sound/soc.h>
#include <sound/pcm.h>
#include <sound/initval.h>

struct snd_soc_dai_driver dummy_dai = {
	.name = "dummy_codec",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 8,
		.rates = SNDRV_PCM_RATE_8000_192000,
		.formats = (SNDRV_PCM_FMTBIT_S16_LE |
			    SNDRV_PCM_FMTBIT_S20_3LE |
			    SNDRV_PCM_FMTBIT_S24_LE |
			    SNDRV_PCM_FMTBIT_S32_LE),
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 2,
		.channels_max = 8,
		.rates = SNDRV_PCM_RATE_8000_192000,
		.formats = (SNDRV_PCM_FMTBIT_S16_LE |
			    SNDRV_PCM_FMTBIT_S20_3LE |
			    SNDRV_PCM_FMTBIT_S24_LE |
			    SNDRV_PCM_FMTBIT_S32_LE),
	},
};

static const struct snd_soc_component_driver soc_dummy_codec = {
};

static int dummy_codec_probe(struct platform_device *pdev)
{
	return devm_snd_soc_register_component(&pdev->dev, &soc_dummy_codec,
				      &dummy_dai, 1);
}

static int dummy_codec_remove(struct platform_device *pdev)
{
	snd_soc_unregister_component(&pdev->dev);

	return 0;
}

static const struct of_device_id dummy_codec_of_match[] = {
	{ .compatible = "ky,dummy-codec", },
	{},
};
MODULE_DEVICE_TABLE(of, dummy_codec_of_match);

static struct platform_driver dummy_codec_driver = {
	.driver = {
		.name = "dummy_codec",
		.of_match_table = dummy_codec_of_match,
	},
	.probe = dummy_codec_probe,
	.remove = dummy_codec_remove,
};

module_platform_driver(dummy_codec_driver);

MODULE_DESCRIPTION("KY Dummy Codec Driver");
MODULE_LICENSE("GPL");
