/*
 * sound\soc\sunxi\sunxi-cpudai.c
 * (C) Copyright 2014-2018
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wolfgang huang <huangjinhui@allwinnertech.com>
 * yumingfeng <yumingfeng@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include "sunxi-pcm.h"

#define DRV_NAME "sunxi-dummy-cpudai"

struct sunxi_cpudai_info {
	struct sunxi_dma_params playback_dma_param;
	struct sunxi_dma_params capture_dma_param;
};

static int sunxi_cpudai_startup(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct sunxi_cpudai_info *sunxi_cpudai = snd_soc_dai_get_drvdata(dai);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		snd_soc_dai_set_dma_data(dai, substream,
				&sunxi_cpudai->playback_dma_param);
	else {
		snd_soc_dai_set_dma_data(dai, substream,
				&sunxi_cpudai->capture_dma_param);
	}

	return 0;
}

static int sunxi_cpudai_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	return 0;
}

static int sunxi_cpudai_suspend(struct snd_soc_dai *dai)
{
	return 0;
}

static int sunxi_cpudai_resume(struct snd_soc_dai *dai)
{
	return 0;
}

static void sunxi_cpudai_shutdown(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
}

static struct snd_soc_dai_ops sunxi_cpudai_dai_ops = {
	.startup = sunxi_cpudai_startup,
	.hw_params = sunxi_cpudai_hw_params,
	.shutdown = sunxi_cpudai_shutdown,
};

static struct snd_soc_dai_driver sunxi_cpudai_dai = {
	.suspend = sunxi_cpudai_suspend,
	.resume = sunxi_cpudai_resume,
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_192000 |
			SNDRV_PCM_RATE_KNOT,
		.formats = SNDRV_PCM_FMTBIT_S16_LE |
			SNDRV_PCM_FMTBIT_S24_LE |
			SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.channels_min = 1,
		.channels_max = 4,
		.rates = SNDRV_PCM_RATE_8000_48000 |
			SNDRV_PCM_RATE_KNOT,
		.formats = SNDRV_PCM_FMTBIT_S16_LE |
			SNDRV_PCM_FMTBIT_S24_LE |
			SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops		= &sunxi_cpudai_dai_ops,

};

static const struct snd_soc_component_driver sunxi_asoc_cpudai_component = {
	.name = DRV_NAME,
};
static const struct of_device_id sunxi_asoc_cpudai_of_match[] = {
	{ .compatible = "allwinner,sunxi-dummy-cpudai", },
	{},
};

static int  sunxi_asoc_cpudai_dev_probe(struct platform_device *pdev)
{
	struct sunxi_cpudai_info *sunxi_cpudai;
	struct device_node *np = pdev->dev.of_node;
	unsigned int temp_val;
	int ret;

	sunxi_cpudai = devm_kzalloc(&pdev->dev,
			sizeof(struct sunxi_cpudai_info), GFP_KERNEL);
	if (!sunxi_cpudai) {
		ret = -ENOMEM;
		goto err_node_put;
	}
	dev_set_drvdata(&pdev->dev, sunxi_cpudai);

	ret = of_property_read_u32(np, "playback_cma", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "playback cma kbytes config missing, using default value.\n");
		sunxi_cpudai->playback_dma_param.cma_kbytes = SUNXI_AUDIO_CMA_MAX_KBYTES;
	} else {
		if (temp_val > SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val = SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val < SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val = SUNXI_AUDIO_CMA_MIN_KBYTES;
		sunxi_cpudai->playback_dma_param.cma_kbytes = temp_val;
	}

	ret = of_property_read_u32(np, "capture_cma", &temp_val);
	if (ret != 0) {
		dev_warn(&pdev->dev, "capture cma kbytes config missing, using default value.\n");
		sunxi_cpudai->capture_dma_param.cma_kbytes = SUNXI_AUDIO_CMA_MAX_KBYTES;
	} else {
		if (temp_val > SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val = SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val < SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val = SUNXI_AUDIO_CMA_MIN_KBYTES;
		sunxi_cpudai->capture_dma_param.cma_kbytes = temp_val;
	}

	ret = of_property_read_u32(np, "tx_fifo_size", &temp_val);
	if (ret != 0) {
		dev_warn(&pdev->dev, "tx_fifo_size miss,using default value.\n");
		sunxi_cpudai->playback_dma_param.fifo_size = 128;//CODEC_TX_FIFO_SIZE
	} else {
		sunxi_cpudai->playback_dma_param.fifo_size = temp_val;
	}
	ret = of_property_read_u32(np, "rx_fifo_size", &temp_val);
	if (ret != 0) {
		dev_warn(&pdev->dev, "rx_fifo_size miss,using default value.\n");
		sunxi_cpudai->capture_dma_param.fifo_size = 256;//CODEC_RX_FIFO_SIZE
	} else {
		sunxi_cpudai->capture_dma_param.fifo_size = temp_val;
	}

	ret = of_property_read_u32(np, "dac_txdata", &temp_val);
	if (ret != 0) {
		dev_err(&pdev->dev, "dac_txdata miss,register dummy cpudai err.\n");
		goto err_devm_kfree;
	} else
		sunxi_cpudai->playback_dma_param.dma_addr = temp_val;//SUNXI_DAC_TXDATA
	sunxi_cpudai->playback_dma_param.dst_maxburst = 4;
	sunxi_cpudai->playback_dma_param.src_maxburst = 4;

	ret = of_property_read_u32(np, "adc_txdata", &temp_val);
	if (ret != 0) {
		dev_err(&pdev->dev, "adc_txdata miss,register dummy cpudai err.\n");
		goto err_devm_kfree;
	} else
		sunxi_cpudai->capture_dma_param.dma_addr = temp_val;//SUNXI_ADC_RXDATA
	sunxi_cpudai->capture_dma_param.src_maxburst = 4;
	sunxi_cpudai->capture_dma_param.dst_maxburst = 4;

	ret = snd_soc_register_component(&pdev->dev, &sunxi_asoc_cpudai_component,
			&sunxi_cpudai_dai, 1);
	if (ret) {
		dev_err(&pdev->dev, "Could not register DAI: %d\n", ret);
		ret = -EBUSY;
		goto err_devm_kfree;
	}

	ret = asoc_dma_platform_register(&pdev->dev, 0);
	if (ret) {
		dev_err(&pdev->dev, "Could not register PCM: %d\n", ret);
		goto err_unregister_component;
	}

	return 0;

err_unregister_component:
	snd_soc_unregister_component(&pdev->dev);
err_devm_kfree:
	devm_kfree(&pdev->dev, sunxi_cpudai);
err_node_put:
	of_node_put(np);
	return ret;
}

static int __exit sunxi_asoc_cpudai_dev_remove(struct platform_device *pdev)
{
	struct sunxi_cpudai_info *sunxi_cpudai = dev_get_drvdata(&pdev->dev);

	asoc_dma_platform_unregister(&pdev->dev);
	snd_soc_unregister_component(&pdev->dev);
	devm_kfree(&pdev->dev, sunxi_cpudai);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver sunxi_asoc_cpudai_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_asoc_cpudai_of_match,
	},
	.probe = sunxi_asoc_cpudai_dev_probe,
	.remove = __exit_p(sunxi_asoc_cpudai_dev_remove),
};

module_platform_driver(sunxi_asoc_cpudai_driver);

MODULE_AUTHOR("wolfgang huang <huangjinhui@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI Internal cpudai ASoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);

