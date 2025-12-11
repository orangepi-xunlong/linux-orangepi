// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2025 - 2028 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2025, zhouxijing <zhouxijing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#define SUNXI_MODNAME		"sound-syscfg"
#include <linux/module.h>
#include <sound/soc.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/regmap.h>
#include <linux/of_address.h>
#include <sound/soc.h>

#include "snd_sunxi_syscfg.h"
#include "snd_sunxi_log.h"

#define DRV_NAME	"sunxi-snd-syscfg"

#define STUB_RATES	SNDRV_PCM_RATE_8000_384000
#define STUB_FORMATS	(SNDRV_PCM_FMTBIT_S8 | \
			SNDRV_PCM_FMTBIT_U8 | \
			SNDRV_PCM_FMTBIT_S16_LE | \
			SNDRV_PCM_FMTBIT_U16_LE | \
			SNDRV_PCM_FMTBIT_S24_LE | \
			SNDRV_PCM_FMTBIT_S24_3LE | \
			SNDRV_PCM_FMTBIT_U24_LE | \
			SNDRV_PCM_FMTBIT_S32_LE | \
			SNDRV_PCM_FMTBIT_U32_LE | \
			SNDRV_PCM_FMTBIT_IEC958_SUBFRAME_LE)

static u64 dummy_dai_formats =
	SND_SOC_POSSIBLE_DAIFMT_I2S	|
	SND_SOC_POSSIBLE_DAIFMT_RIGHT_J	|
	SND_SOC_POSSIBLE_DAIFMT_LEFT_J	|
	SND_SOC_POSSIBLE_DAIFMT_DSP_A	|
	SND_SOC_POSSIBLE_DAIFMT_DSP_B	|
	SND_SOC_POSSIBLE_DAIFMT_AC97	|
	SND_SOC_POSSIBLE_DAIFMT_PDM	|
	SND_SOC_POSSIBLE_DAIFMT_GATED	|
	SND_SOC_POSSIBLE_DAIFMT_CONT	|
	SND_SOC_POSSIBLE_DAIFMT_NB_NF	|
	SND_SOC_POSSIBLE_DAIFMT_NB_IF	|
	SND_SOC_POSSIBLE_DAIFMT_IB_NF	|
	SND_SOC_POSSIBLE_DAIFMT_IB_IF;

static const struct snd_soc_dai_ops dummy_dai_ops = {
	.auto_selectable_formats	= &dummy_dai_formats,
	.num_auto_selectable_formats	= 1,
};

static struct snd_soc_dai_driver sunxi_syscfg_dai = {
	.name       = DRV_NAME,
	.playback = {
		.stream_name	= "Dummy_Playback",
		.channels_min	= 1,
		.channels_max	= 384,
		.rates		= STUB_RATES,
		.formats	= STUB_FORMATS,
	},
	.capture = {
		.stream_name	= "Dummy_Capture",
		.channels_min	= 1,
		.channels_max	= 384,
		.rates		= STUB_RATES,
		.formats	= STUB_FORMATS,
	},
};

static struct snd_soc_component_driver sunxi_syscfg_dev = {
	.name		= DRV_NAME,
	.probe		= sunxi_syscfg_probe,
	.remove		= sunxi_syscfg_remove,
	.suspend	= sunxi_syscfg_suspend,
	.resume		= sunxi_syscfg_resume,
};

static int sunxi_syscfg_dev_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_syscfg_mem *mem;

	mem = devm_kzalloc(dev, sizeof(*mem), GFP_KERNEL);
	if (!mem) {
		SND_LOG_ERR("can't allocate syscfg mem memory\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}
	dev_set_drvdata(dev, mem);

	ret = snd_sunxi_mem_init(pdev, mem);
	if (ret) {
		SND_LOG_ERR("remap init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_mem_init;
	}

	ret = snd_soc_register_component(&pdev->dev, &sunxi_syscfg_dev, &sunxi_syscfg_dai, 1);
	if (ret) {
		SND_LOG_ERR("component register failed\n");
		ret = -ENOMEM;
		goto err_snd_soc_register_component;
	}

	SND_LOG_DEBUG("register audio syscfg platform success\n");

	return 0;

err_snd_soc_register_component:
	snd_sunxi_mem_exit(pdev, mem);
err_snd_sunxi_mem_init:
	devm_kfree(dev, mem);
err_devm_kzalloc:
	of_node_put(np);
	return ret;
}

static int sunxi_syscfg_dev_remove(struct platform_device *pdev)
{
	struct sunxi_syscfg_mem *mem = dev_get_drvdata(&pdev->dev);

	snd_sunxi_mem_exit(pdev, mem);

	return 0;
}

static const struct of_device_id snd_syscfg_of_match[] = {
	{
		.compatible = "allwinner," DRV_NAME,
	},
	{},
};
MODULE_DEVICE_TABLE(of, snd_syscfg_of_match);

static struct platform_driver sunxi_syscfg_driver = {
	.driver	= {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= snd_syscfg_of_match,
	},
	.probe	= sunxi_syscfg_dev_probe,
	.remove	= sunxi_syscfg_dev_remove,
};

int __init sunxi_syscfg_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_syscfg_driver);
	if (ret != 0) {
		SND_LOG_ERR("platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_syscfg_dev_exit(void)
{
	platform_driver_unregister(&sunxi_syscfg_driver);
}

late_initcall(sunxi_syscfg_dev_init);
module_exit(sunxi_syscfg_dev_exit);

MODULE_AUTHOR("zhouxijing@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
MODULE_DESCRIPTION("sunxi soundcard of audio syscfg");
