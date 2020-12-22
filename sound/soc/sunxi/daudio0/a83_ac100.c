/*
 * sound\soc\sunxi\daudio\a83_ac100.c
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
#include "sunxi-daudio0.h"
#include "sunxi-daudiodma0.h"
#include "../../codecs/ac100.h"

static bool daudio_pcm_select 	= 0;
static int daudio_used 			= 0;
static int ac100_used 	= 0;
static int daudio_master 		= 0;
static int audio_format 		= 0;
static int signal_inversion 	= 0;
static bool analog_bb = 0;
static bool digital_bb = 0;

static int sunxi_snddaudio_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	int ret  = 0;
	u32 freq_in = 22579200;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int sample_rate = params_rate(params);

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
	pr_debug("%s,line:%d,freq_in:%d,daudio_pcm_select:%d,audio_format:%d,signal_inversion:%d\n",__func__,__LINE__,
			freq_in,daudio_pcm_select,audio_format,signal_inversion);
	/* set the codec FLL */
	ret = snd_soc_dai_set_pll(codec_dai, AC100_MCLK1, 0, freq_in, freq_in);
	if (ret < 0) {
		return ret;
	}
	/*set cpu_dai clk*/
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0 , freq_in, daudio_pcm_select);
	if (ret < 0) {
		return ret;
	}
	/*set codec_dai clk*/
	ret = snd_soc_dai_set_sysclk(codec_dai, AIF1_CLK , freq_in, daudio_pcm_select);
	if (ret < 0) {
		return ret;
	}
	/*
	* ac100: master. AP: slave
	*/
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0) {
		return ret;
	}
	ret = snd_soc_dai_set_fmt(cpu_dai, (audio_format | (signal_inversion<<8) | SND_SOC_DAIFMT_CBM_CFM));
	if (ret < 0) {
		return ret;
	}
	ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, sample_rate);
	if (ret < 0) {
		return ret;
	}
	return 0;
}

static int bt_net_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	//struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret = 0;
	int freq_in = 24576000;
	if (params_rate(params) != 8000)
		return -EINVAL;
	sunxi_daudio0_set_rate(freq_in);
	/* set the codec FLL */
	ret = snd_soc_dai_set_pll(codec_dai, AC100_MCLK1, 0, freq_in, freq_in);
	if (ret < 0)
		return ret;
	/*set codec system clock source aif2*/
	ret = snd_soc_dai_set_sysclk(codec_dai, AIF2_CLK , 0, 0);
	if (ret < 0)
		return ret;
	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_DSP_A | SND_SOC_DAIFMT_IB_NF | SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;


	return 0;
}
static int bb_voice_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	int ret = 0;
	int freq_in = 24576000;

	if (analog_bb){
		sunxi_daudio0_set_rate(freq_in);
		/* set the codec FLL */
		ret = snd_soc_dai_set_pll(codec_dai, AC100_MCLK1, 0, freq_in, freq_in);
		if (ret < 0)
			return ret;
		/*set codec system clock source aif2*/
		ret = snd_soc_dai_set_sysclk(codec_dai, AIF2_CLK , 0, 0);
		if (ret < 0)
			return ret;
		/* set codec DAI configuration */
		ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_DSP_A | SND_SOC_DAIFMT_IB_NF | SND_SOC_DAIFMT_CBM_CFM);
		if (ret < 0)
			return ret;
	} else if (digital_bb) {
		/* set the codec FLL */
		ret = snd_soc_dai_set_pll(codec_dai, AC100_BCLK2, 0, 2048000,freq_in);
		if (ret < 0)
			return ret;
		/*set system clock source aif2*/
		ret = snd_soc_dai_set_sysclk(codec_dai, AIF2_CLK , 0, 0);
		if (ret < 0)
			return ret;
		/* set codec DAI configuration */
		ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_DSP_A | SND_SOC_DAIFMT_IB_NF | SND_SOC_DAIFMT_CBS_CFS);
		if (ret < 0)
			return ret;
	} else {
		/* set codec DAI configuration */
		ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
		if (ret < 0)
			return ret;
	}
	return 0;
}
static int phone_system_voice_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret = 0;
	u32 freq_in = 22579200;
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
	if (analog_bb) {
		/* set the codec FLL */
		ret = snd_soc_dai_set_pll(codec_dai, AC100_MCLK1, 0, freq_in, freq_in);
		if (ret < 0)
			return ret;
		/*set cpu clk*/
		ret = snd_soc_dai_set_sysclk(cpu_dai, 0 , freq_in, daudio_pcm_select);
		if (ret < 0)
			return ret;
		/*set codec clk*/
		ret = snd_soc_dai_set_sysclk(codec_dai, AIF1_CLK , freq_in, daudio_pcm_select);
		if (ret < 0)
			return ret;
		/*ac100: master. AP: slave*/
		ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
		if (ret < 0)
			return ret;
		ret = snd_soc_dai_set_fmt(cpu_dai, (audio_format | (signal_inversion<<8) | SND_SOC_DAIFMT_CBM_CFM));
		if (ret < 0)
			return ret;
		ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, sample_rate);
		if (ret < 0)
			return ret;
	} else if(digital_bb) {
		/*set cpu clk*/
		ret = snd_soc_dai_set_sysclk(cpu_dai, 0 , freq_in, daudio_pcm_select);
		if (ret < 0)
			return ret;
		/* ac100: master. AP: slave*/
		ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
		if (ret < 0)
			return ret;
		ret = snd_soc_dai_set_fmt(cpu_dai, (audio_format | (signal_inversion<<8) | SND_SOC_DAIFMT_CBM_CFM));
		if (ret < 0)
			return ret;
		ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, sample_rate);
		if (ret < 0)
			return ret;
	} else {

	}
	return 0;
}
static int bt_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	int ret = 0;
	if (params_rate(params) != 8000)
		return -EINVAL;
	/* set codec aif3 configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_IB_NF);
	if (ret < 0)
		return ret;
	return 0;
}
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
static int sunxi_daudio_init(struct snd_soc_pcm_runtime *rtd)
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

static struct snd_soc_ops bt_net_ops = {
	.hw_params = bt_net_hw_params,
};
static struct snd_soc_ops sunxi_snddaudio_ops = {
	.hw_params 		= sunxi_snddaudio_hw_params,
};

static struct snd_soc_ops bb_voice_ops = {
	.hw_params = bb_voice_hw_params,
};
static struct snd_soc_ops phone_system_voice_ops = {
	.hw_params = phone_system_voice_hw_params,
};
static struct snd_soc_ops bt_voice_ops = {
	.hw_params = bt_hw_params,
};
static struct snd_soc_dai_link sunxi_snddaudio_dai_link[] = {
	{
		.name 			= "s_i2s1",
		.cpu_dai_name 	= "pri_dai",
		.stream_name 	= "SUNXI-I2S0",
		.codec_dai_name = "ac100-aif1",
		.codec_name 	= "ac100-codec",
		.init 			= sunxi_daudio_init,
		.platform_name 	= "sunxi-daudio-pcm-audio.0",
		.ops 			= &sunxi_snddaudio_ops,
	} , { /* Second DAI i/f */
		.name = "ac100 Voice",
		.stream_name = "Voice",
		.cpu_dai_name = "bb-voice-dai",
		.codec_dai_name = "ac100-aif2",
		.codec_name = "ac100-codec",
		.ops = &bb_voice_ops,
	} , {/*phone cap and keytone*/
		.name = "VIR",
		.cpu_dai_name 	= "sec_dai",
		.stream_name 	= "vir-dai",
#ifdef 	CONFIG_SND_SOC_TAS5731_CODEC
		.codec_dai_name = "tas5731_audio",
		.codec_name 	= "tas5731-codec.1-001b",
#else
		.codec_dai_name = "ac100-aif1",
		.codec_name 	= "ac100-codec",
#endif
		.platform_name 	= "sunxi-daudio-pcm-audio.0",
		.ops 			= &phone_system_voice_ops,
	} , {/*bt*/
		.name = "BT",
		.cpu_dai_name 	= "bb-voice-dai",
		.stream_name 	= "bt-dai",
		.codec_dai_name = "ac100-aif3",
		.codec_name 	= "ac100-codec",
		.ops 			= &bt_voice_ops,
	}, {/*bt-network*/
		.name = "BT-NET",
		.cpu_dai_name 	= "bb-voice-dai",
		.stream_name 	= "aif2-bt-net",
		.codec_dai_name = "ac100-aif2",
		.codec_name 	= "ac100-codec",
		.ops 			= &bt_net_ops,
	}
};

static struct snd_soc_card snd_soc_sunxi_snddaudio = {
	.name 		= "sndac100",
	.owner 		= THIS_MODULE,
	.dai_link 	= sunxi_snddaudio_dai_link,
	.num_links 	= ARRAY_SIZE(sunxi_snddaudio_dai_link),
	.dapm_widgets = a80_ac100_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(a80_ac100_dapm_widgets),
	.dapm_routes = audio_map,
	.num_dapm_routes = ARRAY_SIZE(audio_map),
	.controls = ac100_pin_controls,
	.num_controls = ARRAY_SIZE(ac100_pin_controls),
};

static int __devinit sunxi_snddaudio0_dev_probe(struct platform_device *pdev)
{
	int ret = 0;
	script_item_u val;
	script_item_value_type_e  type;
	struct snd_soc_card *card = &snd_soc_sunxi_snddaudio;

	type = script_get_item(TDM_NAME, "daudio_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        	pr_err("[daudio0]:%s,line:%d type err!\n", __func__, __LINE__);
	}
	daudio_used = val.val;
	type = script_get_item(TDM_NAME, "daudio_select", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[I2S0] daudio_select type err!\n");
	}
	daudio_pcm_select = val.val;

	type = script_get_item(TDM_NAME, "daudio_master", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[I2S0] daudio_master type err!\n");
	}
	daudio_master = val.val;

	type = script_get_item(TDM_NAME, "audio_format", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[I2S0] audio_format type err!\n");
	}
	audio_format = val.val;

	type = script_get_item(TDM_NAME, "signal_inversion", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[I2S0] signal_inversion type err!\n");
	}
	signal_inversion = val.val;

	type = script_get_item("ac100_audio0", "analog_bb", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[ac100_audio0] analog_bb type err!\n");
	}
	analog_bb = val.val;

	type = script_get_item("ac100_audio0", "digital_bb", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[ac100_audio0] digital_bb type err!\n");
	}
	digital_bb = val.val;

	if (daudio_used) {
		card->dev = &pdev->dev;
	
		ret = snd_soc_register_card(card);
		if (ret) {
			dev_err(&pdev->dev, "snd_soc_register_card() failed: %d\n",
				ret);
		}
	} else {
		pr_err("[daudio0]a83_ac100 cannot find any using configuration for controllers, return directly!\n");
        return 0;
	}
		
	return ret;
}

static int __devexit sunxi_snddaudio0_dev_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	if (daudio_used) {
		snd_soc_unregister_card(card);
	}
	return 0;
}

/*data relating*/
static struct platform_device sunxi_daudio_device = {
	.name 	= "snddaudio",
	.id 	= PLATFORM_DEVID_NONE,
};

/*method relating*/
static struct platform_driver sunxi_daudio_driver = {
	.probe = sunxi_snddaudio0_dev_probe,
	.remove = __exit_p(sunxi_snddaudio0_dev_remove),
	.driver = {
		.name = "snddaudio",
		.owner = THIS_MODULE,
		.pm = &snd_soc_pm_ops,
	},
};

static int __init sunxi_snddaudio0_init(void)
{
	int err = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("acx0", "ac100_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[acx0] ac100_used type err!\n");
	}
	ac100_used = val.val;
	if (ac100_used) {
		if((err = platform_device_register(&sunxi_daudio_device)) < 0)
			return err;

		if ((err = platform_driver_register(&sunxi_daudio_driver)) < 0)
			return err;
	}
	return 0;
}
module_init(sunxi_snddaudio0_init);

static void __exit sunxi_snddaudio0_exit(void)
{
	platform_driver_unregister(&sunxi_daudio_driver);
}
module_exit(sunxi_snddaudio0_exit);
MODULE_AUTHOR("liushaohua");
MODULE_DESCRIPTION("SUNXI A83-AC00 machine driver");
MODULE_LICENSE("GPL");
