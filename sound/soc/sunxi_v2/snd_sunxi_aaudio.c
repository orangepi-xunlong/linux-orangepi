/*
 * sound\soc\sunxi\snd_sunxi_aaudio.c
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
#include <sound/soc.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_pcm.h"

#define HLOG		"CPUDAI"
#define DRV_NAME	"sunxi-snd-plat-aaudio"

struct sunxi_cpudai_info {
	struct sunxi_dma_params playback_dma_param;
	struct sunxi_dma_params capture_dma_param;
};

static int sunxi_aaudio_dai_startup(struct snd_pcm_substream *substream,
				    struct snd_soc_dai *dai)
{
	struct sunxi_cpudai_info *sunxi_cpudai = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG(HLOG, "stream -> %d\n", substream->stream);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		snd_soc_dai_set_dma_data(dai, substream,
					 &sunxi_cpudai->playback_dma_param);
	else
		snd_soc_dai_set_dma_data(dai, substream,
					 &sunxi_cpudai->capture_dma_param);

	return 0;
}

static const struct snd_soc_dai_ops sunxi_aaudio_dai_ops = {
	.startup	= sunxi_aaudio_dai_startup,
};

static int sunxi_cpudai_probe(struct snd_soc_dai *dai)
{
	struct sunxi_cpudai_info *sunxi_cpudai = snd_soc_dai_get_drvdata(dai);

	/* pcm_new will using the dma_param about the cma and fifo params. */
	snd_soc_dai_init_dma_data(dai,
				  &sunxi_cpudai->playback_dma_param,
				  &sunxi_cpudai->capture_dma_param);
	return 0;
}

static struct snd_soc_dai_driver sunxi_aaudio_dai = {
	.probe = sunxi_cpudai_probe,
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.stream_name	= "Capture",
		.channels_min	= 1,
		.channels_max	= 3,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &sunxi_aaudio_dai_ops,
};

static struct snd_soc_component_driver sunxi_aaudio_dev = {
	.name		= DRV_NAME,
};

static int sunxi_aaudio_parse_dma_param(struct device_node *np,
					struct sunxi_cpudai_info *sunxi_cpudai)
{
	int ret;
	unsigned int temp_val;

	/* set dma max buffer */
	ret = of_property_read_u32(np, "playback_cma", &temp_val);
	if (ret < 0) {
		sunxi_cpudai->playback_dma_param.cma_kbytes = SUNXI_AUDIO_CMA_MAX_KBYTES;
		SND_LOG_WARN(HLOG, "playback_cma missing, using default value\n");
	} else {
		if (temp_val		> SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val	< SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MIN_KBYTES;

		sunxi_cpudai->playback_dma_param.cma_kbytes = temp_val;
	}

	ret = of_property_read_u32(np, "capture_cma", &temp_val);
	if (ret != 0) {
		sunxi_cpudai->capture_dma_param.cma_kbytes = SUNXI_AUDIO_CMA_MAX_KBYTES;
		SND_LOG_WARN(HLOG, "capture_cma missing, using default value\n");
	} else {
		if (temp_val		> SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val	< SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MIN_KBYTES;

		sunxi_cpudai->capture_dma_param.cma_kbytes = temp_val;
	}

	/* set fifo size */
	ret = of_property_read_u32(np, "tx_fifo_size", &temp_val);
	if (ret != 0) {
		sunxi_cpudai->playback_dma_param.fifo_size = SUNXI_AUDIO_FIFO_SIZE;
		SND_LOG_WARN(HLOG, "tx_fifo_size miss, using default value\n");
	} else {
		sunxi_cpudai->playback_dma_param.fifo_size = temp_val;
	}

	ret = of_property_read_u32(np, "rx_fifo_size", &temp_val);
	if (ret != 0) {
		sunxi_cpudai->capture_dma_param.fifo_size = SUNXI_AUDIO_FIFO_SIZE;
		SND_LOG_WARN(HLOG, "rx_fifo_size miss,using default value\n");
	} else {
		sunxi_cpudai->capture_dma_param.fifo_size = temp_val;
	}

	/* set data register */
	ret = of_property_read_u32(np, "dac_txdata", &temp_val);
	if (ret != 0) {
		SND_LOG_ERR(HLOG, "dac_txdata miss, no aaudio platform\n");
		return -1;
	} else
		sunxi_cpudai->playback_dma_param.dma_addr = temp_val; /* SUNXI_DAC_TXDATA */
	sunxi_cpudai->playback_dma_param.dst_maxburst = 4;
	sunxi_cpudai->playback_dma_param.src_maxburst = 4;

	ret = of_property_read_u32(np, "adc_txdata", &temp_val);
	if (ret != 0) {
		SND_LOG_ERR(HLOG, "adc_txdata miss, no aaudio platform\n");
		return -1;
	} else
		sunxi_cpudai->capture_dma_param.dma_addr = temp_val; /* SUNXI_ADC_RXDATA */
	sunxi_cpudai->capture_dma_param.src_maxburst = 4;
	sunxi_cpudai->capture_dma_param.dst_maxburst = 4;

	SND_LOG_DEBUG(HLOG, "playback_cma : %zu\n", sunxi_cpudai->playback_dma_param.cma_kbytes);
	SND_LOG_DEBUG(HLOG, "capture_cma  : %zu\n", sunxi_cpudai->capture_dma_param.cma_kbytes);
	SND_LOG_DEBUG(HLOG, "tx_fifo_size : %zu\n", sunxi_cpudai->playback_dma_param.fifo_size);
	SND_LOG_DEBUG(HLOG, "rx_fifo_size : %zu\n", sunxi_cpudai->capture_dma_param.fifo_size);
	SND_LOG_DEBUG(HLOG, "dac_txdata   : 0x%llx\n", sunxi_cpudai->playback_dma_param.dma_addr);
	SND_LOG_DEBUG(HLOG, "adc_txdata   : 0x%llx\n", sunxi_cpudai->capture_dma_param.dma_addr);

	return 0;
}

static int sunxi_aaudio_dev_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_cpudai_info *sunxi_cpudai;

	sunxi_cpudai = devm_kzalloc(&pdev->dev,
				    sizeof(struct sunxi_cpudai_info),
				    GFP_KERNEL);
	if (!sunxi_cpudai) {
		ret = -ENOMEM;
		SND_LOG_ERR(HLOG, "devm_kzalloc failed\n");
		goto err_node_put;
	}
	dev_set_drvdata(&pdev->dev, sunxi_cpudai);

	ret = sunxi_aaudio_parse_dma_param(np, sunxi_cpudai);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "parse dma param failed\n");
		goto err_devm_kfree;
	}

	ret = snd_soc_register_component(&pdev->dev,
					 &sunxi_aaudio_dev,
					 &sunxi_aaudio_dai, 1);
	if (ret) {
		SND_LOG_ERR(HLOG, "component register failed\n");
		ret = -ENOMEM;
		goto err_devm_kfree;
	}

	ret = snd_soc_sunxi_dma_platform_register(&pdev->dev, 0);
	if (ret) {
		SND_LOG_ERR(HLOG, "register ASoC platform failed\n");
		ret = -ENOMEM;
		goto err_unregister_component;
	}

	SND_LOG_DEBUG(HLOG, "register aaudio platform success\n");

	return 0;

err_unregister_component:
	snd_soc_unregister_component(&pdev->dev);
err_devm_kfree:
	devm_kfree(&pdev->dev, sunxi_cpudai);
err_node_put:
	of_node_put(np);
	return ret;
}

static int sunxi_aaudio_dev_remove(struct platform_device *pdev)
{
	snd_soc_sunxi_dma_platform_unregister(&pdev->dev);
	snd_soc_unregister_component(&pdev->dev);

	SND_LOG_DEBUG(HLOG, "unregister aaudio platform success\n");

	return 0;
}

static const struct of_device_id sunxi_aaudio_of_match[] = {
	{ .compatible = "allwinner," DRV_NAME, },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_aaudio_of_match);

static struct platform_driver sunxi_aaudio_driver = {
	.driver	= {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= sunxi_aaudio_of_match,
	},
	.probe	= sunxi_aaudio_dev_probe,
	.remove	= sunxi_aaudio_dev_remove,
};

int __init sunxi_aaudio_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_aaudio_driver);
	if (ret != 0) {
		SND_LOG_ERR(HLOG, "platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_aaudio_dev_exit(void)
{
	platform_driver_unregister(&sunxi_aaudio_driver);
}

late_initcall(sunxi_aaudio_dev_init);
module_exit(sunxi_aaudio_dev_exit);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sunxi soundcard platform of aaudio");
