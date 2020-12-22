/*
 * sound\soc\sunxi\audiohub\sunxi_sndaudiohub.c
 * Copyright(c) 2014-2016 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liushaohua <liushaohua@allwinnertech.com>
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
#include <linux/gpio.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/soc-dapm.h>
#include <linux/io.h>
#include <mach/sys_config.h>
#ifdef CONFIG_ARCH_SUN8IW6
#include "./../daudio0/sunxi-daudio0.h"
#include "./../daudio0/sunxi-daudiodma0.h"
#include "../../codecs/ac100.h"
#endif
static int hub_used 			= 0;

static int sunxi_sndvir_muti_audio_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	int ret  = 0;
	u32 freq_in = 22579200;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
#ifdef CONFIG_ARCH_SUN8IW6
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
#endif
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
			freq_in = 24576000;
			break;
	}
#ifdef CONFIG_ARCH_SUN8IW6
	/* set the codec FLL */
	ret = snd_soc_dai_set_pll(codec_dai, AC100_MCLK1, 0, freq_in, freq_in);
	if (ret < 0) {
		return ret;
	}
	/*set cpu_dai clk*/
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0 , freq_in, 1);
	if (ret < 0) {
		return ret;
	}
	/*set codec_dai clk*/
	ret = snd_soc_dai_set_sysclk(codec_dai, AIF1_CLK , freq_in, 1);
	if (ret < 0) {
		return ret;
	}
	/*
	* ac100: master. AP: slave
	*/
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0) {
		return ret;
	}
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0) {
		return ret;
	}
	ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, sample_rate);
	if (ret < 0) {
		return ret;
	}
#else
	/*set cpu_dai clk*/
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0 , freq_in, 1);
	if (ret < 0) {
		return ret;
	}
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0) {
		return ret;
	}
	ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, sample_rate);
	if (ret < 0) {
		return ret;
	}
#endif
	return 0;
}
#ifdef CONFIG_ARCH_SUN8IW6
static const struct snd_kcontrol_new ac100_pin_controls[] = {
	SOC_DAPM_PIN_SWITCH("External Speaker"),
	SOC_DAPM_PIN_SWITCH("Headphone"),
	SOC_DAPM_PIN_SWITCH("Earpiece"),
};
static const struct snd_soc_dapm_widget a80_ac100_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("External MainMic", NULL),
	SND_SOC_DAPM_MIC("HeadphoneMic", NULL),
	SND_SOC_DAPM_MIC("DigitalMic", NULL),
};

static const struct snd_soc_dapm_route audio_map[] = {
	{"MainMic Bias", NULL, "External MainMic"},
	{"MIC1P", NULL, "MainMic Bias"},
	{"MIC1N", NULL, "MainMic Bias"},

	{"MIC2", NULL, "HMic Bias"},
	{"HMic Bias", NULL, "HeadphoneMic"},

	/*d-mic*/
	{"MainMic Bias", NULL, "DigitalMic"},
	{"D_MIC", NULL, "MainMic Bias"},
};

/*Card initialization*/
static int sunxi_vir_muti_audio_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	snd_soc_dapm_disable_pin(&codec->dapm,	"HPOUTR");
	snd_soc_dapm_disable_pin(&codec->dapm,	"HPOUTL");
	snd_soc_dapm_disable_pin(&codec->dapm,	"EAROUTP");
	snd_soc_dapm_disable_pin(&codec->dapm,	"EAROUTN");
	snd_soc_dapm_disable_pin(&codec->dapm,	"SPK1P");
	snd_soc_dapm_disable_pin(&codec->dapm,	"SPK2P");
	snd_soc_dapm_disable_pin(&codec->dapm,	"SPK1N");
	snd_soc_dapm_disable_pin(&codec->dapm,	"SPK2N");
	snd_soc_dapm_disable_pin(&codec->dapm,	"MIC1P");
	snd_soc_dapm_disable_pin(&codec->dapm,	"MIC1N");
	snd_soc_dapm_disable_pin(&codec->dapm,	"MIC2");
	snd_soc_dapm_disable_pin(&codec->dapm,	"MIC3");
	snd_soc_dapm_disable_pin(&codec->dapm,	"D_MIC");
	return 0;
}
#endif

static struct snd_soc_ops sunxi_sndvir_muti_audio_ops = {
	.hw_params 		= sunxi_sndvir_muti_audio_hw_params,
};

static struct snd_soc_dai_link sunxi_sndvir_muti_audio_dai_link[] = {
	{
		.name 			= "VIR_AUDIO_HUB",
		.cpu_dai_name 	= "sunxi-multi-dai",
		.stream_name 	= "Multi-Output",
	#ifdef CONFIG_SND_SOC_AC100DAPM_CODEC
		.codec_dai_name = "ac100-aif1",
		.codec_name 	= "ac100-codec",
		.init 			= sunxi_vir_muti_audio_init,
	#elif defined CONFIG_SND_SUN8IW7_SNDCODEC
		.codec_dai_name = "sndcodec",
		.codec_name 	= "sunxi-pcm-codec",
	#else
		.codec_dai_name = "snd-hubcodec-dai",
		.codec_name 	= "sunxi-hubcodec-codec.0",
	#endif
		.platform_name 	= "sunxi-vir-pcm.0",
		.ops 			= &sunxi_sndvir_muti_audio_ops,
	}
};

static struct snd_soc_card snd_soc_sunxi_sndvir_muti_audio = {
	.name 		= "audio-hub",
	.owner 		= THIS_MODULE,
	.dai_link 	= sunxi_sndvir_muti_audio_dai_link,
	.num_links 	= ARRAY_SIZE(sunxi_sndvir_muti_audio_dai_link),
#ifdef CONFIG_ARCH_SUN8IW6
	.dapm_widgets = a80_ac100_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(a80_ac100_dapm_widgets),
	.dapm_routes = audio_map,
	.num_dapm_routes = ARRAY_SIZE(audio_map),
	.controls = ac100_pin_controls,
	.num_controls = ARRAY_SIZE(ac100_pin_controls),
#endif
};

static int __devinit sunxi_sndvir_muti_audio_dev_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct snd_soc_card *card = &snd_soc_sunxi_sndvir_muti_audio;
	card->dev = &pdev->dev;

	ret = snd_soc_register_card(card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card() failed: %d\n",
				ret);
	}
	return ret;
}

static int __devexit sunxi_sndvir_muti_audio_dev_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	snd_soc_unregister_card(card);
	return 0;
}

/*data relating*/
static struct platform_device sunxi_vir_muti_audio_device = {
	.name 	= "audio-hub",
	.id 	= PLATFORM_DEVID_NONE,
};

/*method relating*/
static struct platform_driver sunxi_vir_muti_audio_driver = {
	.probe = sunxi_sndvir_muti_audio_dev_probe,
	.remove = __exit_p(sunxi_sndvir_muti_audio_dev_remove),
	.driver = {
		.name = "audio-hub",
		.owner = THIS_MODULE,
		.pm = &snd_soc_pm_ops,
	},
};

static int __init sunxi_sndvir_muti_audio_init(void)
{
	int err = 0;
	script_item_u val;
	script_item_value_type_e  type;
	type = script_get_item("audiohub", "hub_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        	pr_err("[audiohub]:%s,line:%d type err!\n", __func__, __LINE__);
	}
	hub_used = val.val;
	if (hub_used) {
		if((err = platform_device_register(&sunxi_vir_muti_audio_device)) < 0)
			return err;

		if ((err = platform_driver_register(&sunxi_vir_muti_audio_driver)) < 0)
			return err;
	} else {
		pr_warning("[audiohub] driver not init,just return.\n");
	}
	return 0;
}
module_init(sunxi_sndvir_muti_audio_init);

static void __exit sunxi_sndvir_muti_audio_exit(void)
{
	if (hub_used) {
		hub_used = 0;
		platform_driver_unregister(&sunxi_vir_muti_audio_driver);
		platform_device_unregister(&sunxi_vir_muti_audio_device);
	}
}
module_exit(sunxi_sndvir_muti_audio_exit);
MODULE_AUTHOR("liushaohua");
MODULE_DESCRIPTION("SUNXI audio hub machine driver");
MODULE_LICENSE("GPL");
