/*
 * sound\soc\sunxi\sunxi_snddmic.c
 * (C) Copyright 2010-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * huangxin <huangxin@allwinnertech.com>
 * wolfgang huang <huangjinhui@allwinnertech.com>
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
#include <linux/clk.h>
#include <linux/mutex.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/soc-dapm.h>
#include <linux/io.h>
#include <linux/of.h>

static int sunxi_snddmic_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret = 0;
	int freq;

	switch (params_rate(params)) {
	case	8000:
	case	16000:
	case	32000:
	case	12000:
	case	24000:
	case	48000:
	case	96000:
	case	192000:
		freq = 24576000;
		break;
	case	11025:
	case	22050:
	case	44100:
	case	88200:
	case	176400:
		freq = 22579200;
		break;
	default:
		pr_debug("invalid rate setting\n");
		return -EINVAL;
		break;
	}

	ret = snd_soc_dai_set_sysclk(cpu_dai, 0 , freq, 0);
	if (ret < 0)
		return ret;

	return 0;
}

static struct snd_soc_ops sunxi_snddmic_ops = {
	.hw_params 	= sunxi_snddmic_hw_params,
};

static struct snd_soc_dai_link sunxi_snddmic_dai_link = {
	.name		= "DMIC",
	.stream_name	= "SUNXI-DMIC",
	.cpu_dai_name	= "sunxi-dmic",
	.platform_name	= "sunxi-dmic",
	.codec_name	= "dmic-codec",
	.codec_dai_name = "dmic-hifi",
	.ops		= &sunxi_snddmic_ops,
};

static struct snd_soc_card snd_soc_sunxi_snddmic = {
	.name 		= "snddmic",
	.owner 		= THIS_MODULE,
	.dai_link 	= &sunxi_snddmic_dai_link,
	.num_links 	= 1,
};

static int sunxi_snddmic_dev_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct snd_soc_card *card = &snd_soc_sunxi_snddmic;
	struct device_node *np = pdev->dev.of_node;
	struct platform_device *sunxi_snd_dmic_codec_device;
	card->dev = &pdev->dev;

	sunxi_snddmic_dai_link.cpu_dai_name = NULL;
	sunxi_snddmic_dai_link.cpu_of_node = of_parse_phandle(np,
					"sunxi,dmic-controller", 0);
	if (!sunxi_snddmic_dai_link.cpu_of_node) {
		dev_err(&pdev->dev, "Property 'sunxi,dmic-controller' missing or invalid\n");
		ret = -EINVAL;
		return ret;
	}
	sunxi_snddmic_dai_link.platform_name = NULL;
	sunxi_snddmic_dai_link.platform_of_node = sunxi_snddmic_dai_link.cpu_of_node;

	sunxi_snd_dmic_codec_device = platform_device_alloc("dmic-codec", -1);
	if (!sunxi_snd_dmic_codec_device) {
		dev_err(&pdev->dev, "dmic codec alloc failed\n");
		ret = -ENOMEM;
		return ret;
	}

	ret = platform_device_add(sunxi_snd_dmic_codec_device);
	if (ret) {
		dev_err(&pdev->dev, "dmic codec add failed\n");
		ret = -EBUSY;
		goto err_dmic_put;
	}

	ret = snd_soc_register_card(card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card() failed: %d\n", ret);
		ret = -EBUSY;
		goto err_dmic_del;
	}
	return ret;

err_dmic_del:
	platform_device_del(sunxi_snd_dmic_codec_device);
err_dmic_put:
	platform_device_put(sunxi_snd_dmic_codec_device);
	return ret;
}

static int sunxi_snddmic_dev_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	snd_soc_unregister_card(card);
	return 0;
}

static const struct of_device_id sunxi_dmic_of_match[] = {
	{ .compatible = "allwinner,sunxi-dmic-machine", },
	{},
};

static struct platform_driver sunxi_dmic_driver = {
	.driver = {
		.name = "snddmic",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_dmic_of_match,
		.pm = &snd_soc_pm_ops,
	},
	.probe = sunxi_snddmic_dev_probe,
	.remove = sunxi_snddmic_dev_remove,
};

module_platform_driver(sunxi_dmic_driver);

MODULE_AUTHOR("wolfgang huang <huangjinhui@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI DMIC Machine ASoC driver");
MODULE_ALIAS("platform:snddmic");
MODULE_LICENSE("GPL");
