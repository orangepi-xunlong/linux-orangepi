// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Driver for generic Dummy Codec
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <sound/soc.h>

#define DUMMY_RATES SNDRV_PCM_RATE_8000_192000
#define DUMMY_FMT   SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE

static const struct snd_soc_dapm_widget dummy_widgets[] = {
	SND_SOC_DAPM_INPUT("RX"),
	SND_SOC_DAPM_OUTPUT("TX"),
};

static const struct snd_soc_dapm_route dummy_routes[] = {
	{ "Capture", NULL, "RX" },
	{ "TX", NULL, "Playback" },
};

static struct snd_soc_dai_driver dummy_dai[] = {
	{
		.name = "dummy-pcm-i2s",
		.playback = {
			.stream_name = "Playback",
			.channels_min = 2,
			.channels_max = 8,
			.rates = DUMMY_RATES,
			.formats = DUMMY_FMT,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 2,
			.channels_max = 8,
			.rates = DUMMY_RATES,
			.formats = DUMMY_FMT,
		},
	},
	{
		.name = "dummy-pcm-tdm",
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 16,
			.rates = DUMMY_RATES,
			.formats = DUMMY_FMT,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 16,
			.rates = DUMMY_RATES,
			.formats = DUMMY_FMT,
		},
	}
};

static const struct snd_soc_component_driver soc_component_dev_dummy = {
	.dapm_widgets		= dummy_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(dummy_widgets),
	.dapm_routes		= dummy_routes,
	.num_dapm_routes	= ARRAY_SIZE(dummy_routes),
	.idle_bias_on		= 1,
	.use_pmdown_time	= 1,
	.endianness		= 1,
};

static int dummy_probe(struct platform_device *pdev)
{
	return devm_snd_soc_register_component(&pdev->dev,
				      &soc_component_dev_dummy,
				      dummy_dai, ARRAY_SIZE(dummy_dai));
}

static int dummy_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id dummy_codec_of_match[] = {
	{ .compatible = "linux,dummy", },
	{},
};
MODULE_DEVICE_TABLE(of, dummy_codec_of_match);
#endif

static struct platform_driver dummy_driver = {
	.driver = {
		.name = "dummy",
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(dummy_codec_of_match),
#endif
	},
	.probe = dummy_probe,
	.remove = dummy_remove,
};

module_platform_driver(dummy_driver);

MODULE_AUTHOR("xing wang <xing.wang@cixtech.com>");
MODULE_DESCRIPTION("ASoC generic Dummy Codec driver");
MODULE_LICENSE("GPL");
