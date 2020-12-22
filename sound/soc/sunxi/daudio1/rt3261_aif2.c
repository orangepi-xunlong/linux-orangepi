/*
 * sound\soc\sunxi\daudio\sunxi_snddaudio.c
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
#include <linux/clk.h>
#include <linux/mutex.h>

#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/soc-dapm.h>
#include <linux/io.h>
#include <mach/sys_config.h>

#include "sunxi-daudio1.h"
#include "sunxi-daudiodma1.h"

static bool daudio_pcm_select 	= 0;

static int daudio_used 			= 0;
static int daudio_master 		= 0;
static int audio_format 		= 0;
static int signal_inversion 	= 0;

static int sunxi_snddaudio_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	int ret  = 0;
	u32 freq = 22579200;
	unsigned int pll_out = 11289600;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	unsigned long sample_rate = params_rate(params);

	switch (sample_rate) {
		case 8000:
		case 16000:
		case 32000:
		case 64000:
		case 128000:
		case 12000:
		case 24000:
		case 48000:
		case 96000:
		case 192000:
			freq = 24576000;
			pll_out = 12288000;
			break;
	}
pr_err("%s,l:%d\n", __func__, __LINE__);
	/*set system clock source freq and set the mode as daudio or pcm*/
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0 , freq, daudio_pcm_select);
	if (ret < 0) {
		return ret;
	}
	ret = codec_dai->driver->ops->set_sysclk(codec_dai, 0, pll_out,
			SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
			SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0) {
		pr_err("%s, line:%d\n", __func__, __LINE__);
		return ret;
	}
	/*
	* codec clk & FRM master. AP as slave
	*/
	ret = snd_soc_dai_set_fmt(cpu_dai, (audio_format | (signal_inversion<<8) | (daudio_master<<12)));
	if (ret < 0) {
		return ret;
	}
	ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, sample_rate);
	if (ret < 0) {
		return ret;
	}

	/*
	*	audio_format == SND_SOC_DAIFMT_DSP_A
	*	signal_inversion<<8 == SND_SOC_DAIFMT_IB_NF
	*	daudio_master<<12 == SND_SOC_DAIFMT_CBM_CFM
	*/
	DAUDIO_DBG("%s,line:%d,audio_format:%d,SND_SOC_DAIFMT_DSP_A:%d\n",\
			__func__, __LINE__, audio_format, SND_SOC_DAIFMT_DSP_A);
	DAUDIO_DBG("%s,line:%d,signal_inversion:%d,signal_inversion<<8:%d,SND_SOC_DAIFMT_IB_NF:%d\n",\
			__func__, __LINE__, signal_inversion, signal_inversion<<8, SND_SOC_DAIFMT_IB_NF);
	DAUDIO_DBG("%s,line:%d,daudio_master:%d,daudio_master<<12:%d,SND_SOC_DAIFMT_CBM_CFM:%d\n",\
			__func__, __LINE__, daudio_master, daudio_master<<12, SND_SOC_DAIFMT_CBM_CFM);

	return 0;
}

static struct snd_soc_ops sunxi_snddaudio_ops = {
	.hw_params 		= sunxi_snddaudio_hw_params,
};

static struct snd_soc_dai_link sunxi_snddaudio_dai_link = {
	.name 			= "s_i2s1",
	.cpu_dai_name 	= "pcm1",
	.stream_name 	= "SUNXI-PCM1",
	.codec_dai_name = "rt3261-aif2",
	.codec_name 	= "rt3261.2-001c",
	.platform_name 	= "sunxi-daudio-pcm-audio1.0",
	.ops 			= &sunxi_snddaudio_ops,
};

static struct snd_soc_card snd_soc_sunxi_snddaudio = {
	.name 		= "rt3261_aif2",
	.owner 		= THIS_MODULE,
	.dai_link 	= &sunxi_snddaudio_dai_link,
	.num_links 	= 1,
};

static int __devinit sunxi_snddaudio_dev_probe(struct platform_device *pdev)
{
	int ret = 0;
	script_item_u val;
	script_item_value_type_e  type;
	struct snd_soc_card *card = &snd_soc_sunxi_snddaudio;
	type = script_get_item(TDM_NAME, "daudio_select", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[daudio1] daudio_select type err!\n");
    }
	daudio_pcm_select = val.val;

	type = script_get_item(TDM_NAME, "daudio_master", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[daudio1] daudio_master type err!\n");
    }
	daudio_master = val.val;
	
	type = script_get_item(TDM_NAME, "audio_format", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[daudio1] audio_format type err!\n");
    }
	audio_format = val.val;

	type = script_get_item(TDM_NAME, "signal_inversion", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[daudio1] signal_inversion type err!\n");
    }
	signal_inversion = val.val;
	card->dev = &pdev->dev;
	ret = snd_soc_register_card(card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card() failed: %d\n", ret);
	}
	return ret;
}

static int __devexit sunxi_snddaudio_dev_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	snd_soc_unregister_card(card);
	return 0;
}

/*data relating*/
static struct platform_device sunxi_daudio_device = {
	.name 	= "snddaudio1",
	.id 	= PLATFORM_DEVID_NONE,
};

/*method relating*/
static struct platform_driver sunxi_daudio_driver = {
	.probe = sunxi_snddaudio_dev_probe,
	.remove = __exit_p(sunxi_snddaudio_dev_remove),
	.driver = {
		.name = "snddaudio1",
		.owner = THIS_MODULE,
		.pm = &snd_soc_pm_ops,
	},
};

static int __init sunxi_snddaudio_init(void)
{
	int err = 0;
	script_item_u val;
	script_item_value_type_e  type;
	type = script_get_item(TDM_NAME, "daudio_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        	pr_err("[daudio1]:%s,line:%d type err!\n", __func__, __LINE__);
    	}
	daudio_used = val.val;
	if (daudio_used) {
		if((err = platform_device_register(&sunxi_daudio_device)) < 0)
			return err;

		if ((err = platform_driver_register(&sunxi_daudio_driver)) < 0)
			return err;
	} else {
		pr_warning("[DAUDIO1] driver not init,just return.\n");
	}
	return 0;
}
module_init(sunxi_snddaudio_init);

static void __exit sunxi_snddaudio_exit(void)
{
	if (daudio_used) {
		daudio_used = 0;
		platform_driver_unregister(&sunxi_daudio_driver);
		platform_device_unregister(&sunxi_daudio_device);
	}
}
module_exit(sunxi_snddaudio_exit);

MODULE_AUTHOR("huangxin");
MODULE_DESCRIPTION("SUNXI_snddaudio ALSA SoC audio driver");
MODULE_LICENSE("GPL");
