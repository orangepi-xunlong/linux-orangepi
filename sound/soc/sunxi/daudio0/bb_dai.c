/*
 * sound\soc\sunxi\daudio\bb_dai.c
 *
 * Copyright(c) 2014-2016 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liushaohua <liushaohua@allwinnertech.com>
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
#include <linux/pinctrl/consumer.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <asm/dma.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include <linux/gpio.h>

static int bb_set_fmt(struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
	return 0;
}
static int bb_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,	struct snd_soc_dai *dai)
{
	return 0;
}
static int bb_trigger(struct snd_pcm_substream *substream,
                              int cmd, struct snd_soc_dai *dai)
{
	pr_info("%s,l:%d\n", __func__, __LINE__);
	return 0;
}
static int bb_set_sysclk(struct snd_soc_dai *cpu_dai, int clk_id,
                                 unsigned int freq, int daudio_pcm_select)
{
	return 0;
}
static int bb_set_clkdiv(struct snd_soc_dai *cpu_dai, int div_id, int sample_rate)
{
	return 0;
}
static int bb_perpare(struct snd_pcm_substream *substream,
	struct snd_soc_dai *cpu_dai)
{
	return 0;
}

static struct snd_soc_dai_ops bb_dai_ops = {
	.trigger 	= bb_trigger,
	.hw_params 	= bb_hw_params,
	.set_fmt 	= bb_set_fmt,
	.set_clkdiv = bb_set_clkdiv,
	.set_sysclk = bb_set_sysclk,
	.prepare  =	bb_perpare,
};

static struct snd_soc_dai_driver bb_dai = {
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000|SNDRV_PCM_RATE_48000|SNDRV_PCM_RATE_44100,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,},
	.capture = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000|SNDRV_PCM_RATE_48000|SNDRV_PCM_RATE_44100,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,},
	.ops 		= &bb_dai_ops,
};
static int __init bb_dev_probe(struct platform_device *pdev)
{
	int ret = 0;
	ret = snd_soc_register_dai(&pdev->dev, &bb_dai);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register DAI\n");
	}

	return 0;
}

static int __exit bb_dev_remove(struct platform_device *pdev)
{
	snd_soc_unregister_dai(&pdev->dev);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

/*data relating*/
static struct platform_device bb_device = {
	.name 	= "bb-voice-dai",
	.id 	= -1,
};

/*method relating*/
static struct platform_driver bb_driver = {
	.probe = bb_dev_probe,
	.remove = __exit_p(bb_dev_remove),
	.driver = {
		.name = "bb-voice-dai",
		.owner = THIS_MODULE,
	},
};

static int __init bb_init(void)
{
	int err = 0;
	if((err = platform_device_register(&bb_device)) < 0)
		return err;
	if ((err = platform_driver_register(&bb_driver)) < 0)
		return err;
	return 0;
}
module_init(bb_init);

static void __exit bb_exit(void)
{
	platform_driver_unregister(&bb_driver);
	platform_device_unregister(&bb_device);
}
module_exit(bb_exit);

/* Module information */
MODULE_AUTHOR("liushaohua");
MODULE_DESCRIPTION("vir bb Interface");
MODULE_LICENSE("GPL");

