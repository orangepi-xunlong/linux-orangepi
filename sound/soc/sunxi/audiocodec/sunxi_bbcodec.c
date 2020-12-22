/*
 * sound\soc\sunxi\audiocodec\sunxi-codec.c
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@Reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/jiffies.h>
#include <linux/io.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include <asm/dma.h>
#include <mach/hardware.h>

#define SUNXI_PCM_RATES (SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT)

static int sunxi_bb_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	return 0;
}

static struct snd_soc_dai_ops sunxi_bb_dai_ops = {
	.hw_params 	= sunxi_bb_hw_params,
};
static struct snd_soc_dai_driver voice_dai = {
	.name = "goni-voice-dai",
	.id = 0,
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
	},
	.capture = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
	},
	.ops 		= &sunxi_bb_dai_ops,
};

static int __init sunxi_pcm_dev_probe(struct platform_device *pdev)
{
	int err = -1;

	err = snd_soc_register_dai(&pdev->dev, &voice_dai);	
	if (err) {
		dev_err(&pdev->dev, "Failed to register DAI\n");
		return err;
	}

	return 0;
}

static int __exit sunxi_pcm_dev_remove(struct platform_device *pdev)
{
	snd_soc_unregister_dai(&pdev->dev);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

/*data relating*/
static struct platform_device sunxi_pcm_device = {
	.name = "sunxi-bbcodec",
	.id = -1,
};

/*method relating*/
static struct platform_driver sunxi_pcm_driver = {
	.probe = sunxi_pcm_dev_probe,
	.remove = __exit_p(sunxi_pcm_dev_remove),
	.driver = {
		.name = "sunxi-bbcodec",
		.owner = THIS_MODULE,
	},
};

static int __init sunxi_pcm_init(void)
{
	int err = 0;

	if((err = platform_device_register(&sunxi_pcm_device)) < 0)
		return err;

	if ((err = platform_driver_register(&sunxi_pcm_driver)) < 0)
		return err;	

	return 0;
}
module_init(sunxi_pcm_init);

static void __exit sunxi_pcm_exit(void)
{
	platform_driver_unregister(&sunxi_pcm_driver);
}
module_exit(sunxi_pcm_exit);

/* Module information */
MODULE_AUTHOR("REUUIMLLA");
MODULE_DESCRIPTION("sunxi PCM SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-pcm");

