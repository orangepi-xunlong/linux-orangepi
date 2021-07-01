// SPDX-License-Identifier: GPL-2.0-only

#include <linux/module.h>
#include <sound/soc.h>

static const struct snd_soc_dapm_widget ec25_dapm_widgets[] = {
	SND_SOC_DAPM_OUTPUT("AOUT"),
	SND_SOC_DAPM_INPUT("AIN"),
};

static const struct snd_soc_dapm_route ec25_dapm_routes[] = {
	{ "AOUT", NULL, "Playback" },
	{ "AOUT", NULL, "Wideband Playback" },
	{ "Capture", NULL, "AIN" },
	{ "Wideband Capture", NULL, "AIN" },
};

static const struct snd_soc_component_driver ec25_component_driver = {
	.dapm_widgets		= ec25_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(ec25_dapm_widgets),
	.dapm_routes		= ec25_dapm_routes,
	.num_dapm_routes	= ARRAY_SIZE(ec25_dapm_routes),
	.endianness		= 1,
	.non_legacy_dai_naming	= 1,
};

static struct snd_soc_dai_driver ec25_dais[] = {
	{
		.name = "ec25",
		.capture = {
			.stream_name	= "Capture",
			.channels_min	= 1,
			.channels_max	= 1,
			.rates		= SNDRV_PCM_RATE_8000,
			.formats	= SNDRV_PCM_FMTBIT_S16_LE,
		},
		.playback = {
			.stream_name	= "Playback",
			.channels_min	= 1,
			.channels_max	= 1,
			.rates		= SNDRV_PCM_RATE_8000,
			.formats	= SNDRV_PCM_FMTBIT_S16_LE,
		},
		.symmetric_rates = 1,
		.symmetric_channels = 1,
		.symmetric_samplebits = 1,
	},
	{
		.name = "ec25-wb",
		.capture = {
			.stream_name	= "Wideband Capture",
			.channels_min	= 1,
			.channels_max	= 1,
			.rates		= SNDRV_PCM_RATE_16000,
			.formats	= SNDRV_PCM_FMTBIT_S16_LE,
		},
		.playback = {
			.stream_name	= "Wideband Playback",
			.channels_min	= 1,
			.channels_max	= 1,
			.rates		= SNDRV_PCM_RATE_16000,
			.formats	= SNDRV_PCM_FMTBIT_S16_LE,
		},
		.symmetric_rates = 1,
		.symmetric_channels = 1,
		.symmetric_samplebits = 1,
	},
};

static int ec25_codec_probe(struct platform_device *pdev)
{
	return devm_snd_soc_register_component(&pdev->dev, &ec25_component_driver,
					       ec25_dais, ARRAY_SIZE(ec25_dais));
}

static const struct of_device_id ec25_codec_of_match[] = {
	{ .compatible = "quectel,ec25", },
	{},
};
MODULE_DEVICE_TABLE(of, ec25_codec_of_match);

static struct platform_driver ec25_codec_driver = {
	.driver	= {
		.name		= "ec25",
		.of_match_table	= of_match_ptr(ec25_codec_of_match),
	},
	.probe	= ec25_codec_probe,
};

module_platform_driver(ec25_codec_driver);

MODULE_DESCRIPTION("ASoC ec25 driver");
MODULE_AUTHOR("Samuel Holland <samuel@sholland.org>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ec25");
