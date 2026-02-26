// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2022, Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#define SUNXI_MODNAME		"sound-aa"
#include "snd_sunxi_log.h"
#include <linux/module.h>
#include <sound/soc.h>

#include "snd_sunxi_pcm.h"
#include "snd_sunxi_adapter.h"

#define DRV_NAME	"sunxi-snd-plat-aaudio"

struct sunxi_cpudai {
	struct sunxi_dma_params playback_dma_param;
	struct sunxi_dma_params capture_dma_param;
};

static int sunxi_aaudio_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_cpudai *sunxi_cpudai = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG("stream -> %d\n", substream->stream);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		snd_soc_dai_set_dma_data(dai, substream, &sunxi_cpudai->playback_dma_param);
	else
		snd_soc_dai_set_dma_data(dai, substream, &sunxi_cpudai->capture_dma_param);

	return 0;
}

static struct snd_soc_dai_ops sunxi_aaudio_dai_ops = {
	.startup	= sunxi_aaudio_dai_startup,
};

static int sunxi_aaudio_dai_probe(struct snd_soc_dai *dai)
{
	struct sunxi_cpudai *sunxi_cpudai = snd_soc_dai_get_drvdata(dai);

	/* pcm_new will using the dma_param about the cma and fifo params. */
	snd_soc_dai_init_dma_data(dai,
				  &sunxi_cpudai->playback_dma_param,
				  &sunxi_cpudai->capture_dma_param);
	return 0;
}

static struct snd_soc_dai_driver sunxi_aaudio_dai = {
	.name = DRV_NAME,
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S24_3LE
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
				| SNDRV_PCM_FMTBIT_S24_3LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
};

static struct snd_soc_component_driver sunxi_aaudio_dev = {
	.name		= DRV_NAME,
};

static int sunxi_aaudio_parse_dma_param(struct device_node *np, struct sunxi_cpudai *sunxi_cpudai)
{
	int ret;
	unsigned int temp_val;

	/* set dma max buffer */
	ret = of_property_read_u32(np, "playback-cma", &temp_val);
	if (ret < 0) {
		sunxi_cpudai->playback_dma_param.cma_kbytes = SUNXI_AUDIO_CMA_MAX_KBYTES;
		SND_LOG_WARN("playback-cma missing, using default value\n");
	} else {
		if (temp_val		> SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val	< SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MIN_KBYTES;

		sunxi_cpudai->playback_dma_param.cma_kbytes = temp_val;
	}

	ret = of_property_read_u32(np, "capture-cma", &temp_val);
	if (ret != 0) {
		sunxi_cpudai->capture_dma_param.cma_kbytes = SUNXI_AUDIO_CMA_MAX_KBYTES;
		SND_LOG_WARN("capture-cma missing, using default value\n");
	} else {
		if (temp_val		> SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val	< SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MIN_KBYTES;

		sunxi_cpudai->capture_dma_param.cma_kbytes = temp_val;
	}

	/* set fifo size */
	ret = of_property_read_u32(np, "tx-fifo-size", &temp_val);
	if (ret != 0) {
		sunxi_cpudai->playback_dma_param.fifo_size = SUNXI_AUDIO_FIFO_SIZE;
		SND_LOG_WARN("tx-fifo-size miss, using default value\n");
	} else {
		sunxi_cpudai->playback_dma_param.fifo_size = temp_val;
	}

	ret = of_property_read_u32(np, "rx-fifo-size", &temp_val);
	if (ret != 0) {
		sunxi_cpudai->capture_dma_param.fifo_size = SUNXI_AUDIO_FIFO_SIZE;
		SND_LOG_WARN("rx-fifo-size miss,using default value\n");
	} else {
		sunxi_cpudai->capture_dma_param.fifo_size = temp_val;
	}

	ret = of_property_read_u32(np, "dma-buf-mode", &temp_val);
	if (ret != 0) {
		sunxi_cpudai->playback_dma_param.dma_buf_mode = 0;
		sunxi_cpudai->capture_dma_param.dma_buf_mode = 0;
		SND_LOG_DEBUG("dma-buf-mode miss,using default value\n");
	} else {
		if (temp_val != 1 && temp_val != 0) {
			sunxi_cpudai->playback_dma_param.dma_buf_mode = 0;
			sunxi_cpudai->capture_dma_param.dma_buf_mode = 0;
			SND_LOG_WARN("invalid dma-buf-mode value, using default value\n");
		} else {
			sunxi_cpudai->playback_dma_param.dma_buf_mode = temp_val;
			sunxi_cpudai->capture_dma_param.dma_buf_mode = temp_val;
		}
	}

	/* set data register */
	ret = of_property_read_u32(np, "dac-txdata", &temp_val);
	if (ret != 0) {
		SND_LOG_WARN("dac-txdata miss, no aaudio platform\n");
		sunxi_cpudai->playback_dma_param.dma_addr = 0x0;
	} else {
		sunxi_cpudai->playback_dma_param.dma_addr = temp_val; /* SUNXI_DAC_TXDATA */
	}
	sunxi_cpudai->playback_dma_param.dst_maxburst = 4;
	sunxi_cpudai->playback_dma_param.src_maxburst = 4;

	ret = of_property_read_u32(np, "adc-txdata", &temp_val);
	if (ret != 0) {
		SND_LOG_WARN("adc-txdata miss, no aaudio platform\n");
		sunxi_cpudai->capture_dma_param.dma_addr = 0x0;
	} else {
		sunxi_cpudai->capture_dma_param.dma_addr = temp_val; /* SUNXI_ADC_RXDATA */
	}
	sunxi_cpudai->capture_dma_param.src_maxburst = 4;
	sunxi_cpudai->capture_dma_param.dst_maxburst = 4;

	SND_LOG_DEBUG("playback-cma : %zu\n", sunxi_cpudai->playback_dma_param.cma_kbytes);
	SND_LOG_DEBUG("capture-cma  : %zu\n", sunxi_cpudai->capture_dma_param.cma_kbytes);
	SND_LOG_DEBUG("tx-fifo-size : %zu\n", sunxi_cpudai->playback_dma_param.fifo_size);
	SND_LOG_DEBUG("rx-fifo-size : %zu\n", sunxi_cpudai->capture_dma_param.fifo_size);
#if IS_ENABLED(CONFIG_ARCH_DMA_ADDR_T_64BIT)
	SND_LOG_DEBUG("dac-txdata   : 0x%llx\n", sunxi_cpudai->playback_dma_param.dma_addr);
	SND_LOG_DEBUG("adc-txdata   : 0x%llx\n", sunxi_cpudai->capture_dma_param.dma_addr);
#else
	SND_LOG_DEBUG("dac-txdata   : 0x%x\n", sunxi_cpudai->playback_dma_param.dma_addr);
	SND_LOG_DEBUG("adc-txdata   : 0x%x\n", sunxi_cpudai->capture_dma_param.dma_addr);
#endif

	return 0;
}

static int sunxi_aaudio_dev_probe(struct platform_device *pdev)
{
	struct sunxi_adapt_dai_ops_priv priv;
	int ret;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_cpudai *sunxi_cpudai;

	sunxi_cpudai = devm_kzalloc(&pdev->dev, sizeof(*sunxi_cpudai), GFP_KERNEL);
	if (!sunxi_cpudai) {
		ret = -ENOMEM;
		SND_LOG_ERR("devm_kzalloc failed\n");
		goto err_devm_kzalloc;
	}
	dev_set_drvdata(&pdev->dev, sunxi_cpudai);

	ret = sunxi_aaudio_parse_dma_param(np, sunxi_cpudai);
	if (ret < 0) {
		SND_LOG_ERR("parse dma param failed\n");
		goto err_sunxi_aaudio_parse_dma_param;
	}

	priv.probe = sunxi_aaudio_dai_probe;
	priv.remove = NULL;
	sunxi_adpt_set_dai_ops(&sunxi_aaudio_dai, &sunxi_aaudio_dai_ops, &priv);

	ret = snd_soc_register_component(&pdev->dev, &sunxi_aaudio_dev, &sunxi_aaudio_dai, 1);
	if (ret) {
		SND_LOG_ERR("component register failed\n");
		ret = -ENOMEM;
		goto err_snd_soc_register_component;
	}

	ret = snd_sunxi_dma_platform_register(&pdev->dev);
	if (ret) {
		SND_LOG_ERR("register ASoC platform failed\n");
		ret = -ENOMEM;
		goto err_snd_sunxi_platform_register;
	}

	SND_LOG_DEBUG("register aaudio platform success\n");

	return 0;

err_snd_sunxi_platform_register:
	snd_soc_unregister_component(&pdev->dev);
err_snd_soc_register_component:
err_sunxi_aaudio_parse_dma_param:
	devm_kfree(&pdev->dev, sunxi_cpudai);
err_devm_kzalloc:
	of_node_put(np);
	return ret;
}

static int sunxi_aaudio_dev_remove(struct platform_device *pdev)
{
	snd_sunxi_dma_platform_unregister(&pdev->dev);
	snd_soc_unregister_component(&pdev->dev);

	SND_LOG_DEBUG("unregister aaudio platform success\n");

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
		SND_LOG_ERR("platform driver register failed\n");
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
MODULE_VERSION("1.0.1");
MODULE_DESCRIPTION("sunxi soundcard platform of aaudio");
