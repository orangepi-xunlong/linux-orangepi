/*
 * sound\soc\sunxi\i2s1\sndi2s1.c
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

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <linux/io.h>
#include <mach/sys_config.h>

struct sndi2s1_priv {
	int sysclk;
	int dai_fmt;

	struct snd_pcm_substream *master_substream;
	struct snd_pcm_substream *slave_substream;
};

static int i2s1_used = 0;
#define sndi2s1_RATES  (SNDRV_PCM_RATE_8000_192000|SNDRV_PCM_RATE_KNOT)
#define sndi2s1_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | \
		                     SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE)

static int sndi2s1_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}

static int sndi2s1_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	return 0;
}

static void sndi2s1_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	
}

static int sndi2s1_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	return 0;
}

static int sndi2s1_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int sndi2s1_set_dai_clkdiv(struct snd_soc_dai *codec_dai, int div_id, int div)
{
	return 0;
}

static int sndi2s1_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	return 0;
}

struct snd_soc_dai_ops sndi2s1_dai_ops = {
	.startup = sndi2s1_startup,
	.shutdown = sndi2s1_shutdown,
	.hw_params = sndi2s1_hw_params,
	.digital_mute = sndi2s1_mute,
	.set_sysclk = sndi2s1_set_dai_sysclk,
	.set_clkdiv = sndi2s1_set_dai_clkdiv,
	.set_fmt = sndi2s1_set_dai_fmt,
};

struct snd_soc_dai_driver sndi2s1_dai = {
	.name = "sndi2s1",
	/* playback capabilities */
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = sndi2s1_RATES,
		.formats = sndi2s1_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = sndi2s1_RATES,
		.formats = sndi2s1_FORMATS,
	},
	/* pcm operations */
	.ops = &sndi2s1_dai_ops,	
};
EXPORT_SYMBOL(sndi2s1_dai);
	
static int sndi2s1_soc_probe(struct snd_soc_codec *codec)
{
	struct sndi2s1_priv *sndi2s1;

	sndi2s1 = kzalloc(sizeof(struct sndi2s1_priv), GFP_KERNEL);
	if(sndi2s1 == NULL){		
		return -ENOMEM;
	}		
	snd_soc_codec_set_drvdata(codec, sndi2s1);

	return 0;
}

/* power down chip */
static int sndi2s1_soc_remove(struct snd_soc_codec *codec)
{
	struct sndi2s1_priv *sndi2s1 = snd_soc_codec_get_drvdata(codec);

	kfree(sndi2s1);

	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_sndi2s1 = {
	.probe 	=	sndi2s1_soc_probe,
	.remove =   sndi2s1_soc_remove,
};

static int __init sndi2s1_codec_probe(struct platform_device *pdev)
{
	return snd_soc_register_codec(&pdev->dev, &soc_codec_dev_sndi2s1, &sndi2s1_dai, 1);	
}

static int __exit sndi2s1_codec_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

/*data relating*/
static struct platform_device sndi2s1_codec_device = {
	.name = "sunxi-i2s1-codec",
};

/*method relating*/
static struct platform_driver sndi2s1_codec_driver = {
	.driver = {
		.name = "sunxi-i2s1-codec",
		.owner = THIS_MODULE,
	},
	.probe = sndi2s1_codec_probe,
	.remove = __exit_p(sndi2s1_codec_remove),
};

static int __init sndi2s1_codec_init(void)
{	
	int err = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("i2s1", "i2s1_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[I2S] type err!\n");
    }

	i2s1_used = val.val;

	if (i2s1_used) {
		if((err = platform_device_register(&sndi2s1_codec_device)) < 0)
			return err;
	
		if ((err = platform_driver_register(&sndi2s1_codec_driver)) < 0)
			return err;
	} else {
       pr_err("[I2S]sndi2s1 cannot find any using configuration for controllers, return directly!\n");
       return 0;
    }
	
	return 0;
}
module_init(sndi2s1_codec_init);

static void __exit sndi2s1_codec_exit(void)
{
	if (i2s1_used) {
		i2s1_used = 0;
		platform_driver_unregister(&sndi2s1_codec_driver);
	}
}
module_exit(sndi2s1_codec_exit);

MODULE_DESCRIPTION("SNDI2S1 ALSA soc codec driver");
MODULE_AUTHOR("huangxin");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-i2s1-codec");
