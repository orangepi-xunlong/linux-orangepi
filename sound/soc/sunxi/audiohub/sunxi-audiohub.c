/*
 * sound\soc\sunxi\audiohub\sunxi-audiohub.c
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
#include <linux/pinctrl/consumer.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <asm/dma.h>
#include <mach/sunxi-smc.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include <linux/gpio.h>
#include "./../daudio0/sunxi-daudiodma0.h"
#include "./../daudio0/sunxi-daudio0.h"
#include "./../hdmiaudio/sunxi-hdmipcm.h"
#include "./../spdif/sunxi_spdif.h"
#include <video/drv_hdmi.h>

struct sunxi_daudio_info sunxi_tdmaudio;
struct sunxi_i2s1_info sunxi_hdmiaudio;
struct sunxi_spdif_info sunxi_spdifaudio;

static __audio_hdmi_func 	g_hdmi_func;
static hdmi_audio_t 		hdmi_para;
atomic_t pcm_count_num_muti;

static bool  hub_codec_en = true;
static bool  hub_hdmi_en = false;
static bool  hub_spdif_en = false;
static bool  hub_used = false;
static bool spdif_used = false;
static bool hdmi_used = false;
static bool codec_used = true;

static struct sunxi_dma_params sunxi_daudio_pcm_stereo_out = {
	.name		= "daudio_play",
	.dma_addr	= SUNXI_DAUDIOBASE + SUNXI_DAUDIOTXFIFO,/*send data address	*/
};

static struct sunxi_dma_params sunxi_daudio_pcm_stereo_in = {
	.name   	= "daudio_capture",
	.dma_addr	=SUNXI_DAUDIOBASE + SUNXI_DAUDIORXFIFO,/*accept data address	*/
};

static struct sunxi_dma_params sunxi_hdmiaudio_pcm_stereo_out = {
	.name 			= "hdmiaudio_play",
	.dma_addr 		= SUNXI_I2S1BASE + SUNXI_I2S1TXFIFO,

};
void audio_set_muti_hdmi_func(__audio_hdmi_func *hdmi_func)
{
	if (hdmi_func) {
		g_hdmi_func.hdmi_audio_enable 	= hdmi_func->hdmi_audio_enable;
		g_hdmi_func.hdmi_set_audio_para = hdmi_func->hdmi_set_audio_para;
		g_hdmi_func.hdmi_is_playback = hdmi_func->hdmi_is_playback;
	} else {
		pr_err("error:%s,line:%d\n", __func__, __LINE__);
	}
}
EXPORT_SYMBOL(audio_set_muti_hdmi_func);

static int sunxi_audiohub_set_fmt(struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
	/*daudio*/
	if (codec_used && hub_codec_en)
		sunxi_daudio0_set_fmt(cpu_dai, fmt);
	/*hdmi*/
	if (hdmi_used && hub_hdmi_en) {
		#ifdef CONFIG_SND_SUNXI_SOC_HDMIAUDIO
		sunxi_hdmiaudio_set_fmt(cpu_dai, fmt);
		#endif
	}
	/*spdif*/
	if (spdif_used && hub_spdif_en) {
		#ifdef CONFIG_SND_SUNXI_SOC_SPDIF
		spdif_set_fmt(0);
		#endif
	}
	return 0;
}

static int sunxi_audiohub_hw_params(struct snd_pcm_substream *substream,
																struct snd_pcm_hw_params *params,
																struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sunxi_dma_params *dma_data;
	/*daudio*/
	if (codec_used && hub_codec_en)
		sunxi_daudio0_hw_params(substream,params,dai);
	/*hdmi*/
	if (hdmi_used && hub_hdmi_en){
		#ifdef CONFIG_SND_SUNXI_SOC_HDMIAUDIO
		sunxi_hdmiaudio_hw_params(substream,params,dai);
		#endif
		hdmi_para.sample_rate = params_rate(params);
		hdmi_para.channel_num = params_channels(params);
		#ifdef CONFIG_SND_SUNXI_SOC_SUPPORT_AUDIO_RAW
			hdmi_para.data_raw = params_raw(params);
		#else
			hdmi_para.data_raw = 1;
		#endif
		switch (params_format(params))
		{
			case SNDRV_PCM_FORMAT_S16_LE:
				hdmi_para.sample_bit = 16;
				break;
			case SNDRV_PCM_FORMAT_S20_3LE:
				hdmi_para.sample_bit = 24;
				break;
			case SNDRV_PCM_FORMAT_S24_LE:
				hdmi_para.sample_bit = 24;
				break;
			case SNDRV_PCM_FORMAT_S32_LE:
				hdmi_para.sample_bit = 24;
				break;
			default:
				return -EINVAL;
		}
		/*
		PCM = 1,
		AC3 = 2,
		MPEG1 = 3,
		MP3 = 4,
		MPEG2 = 5,
		AAC = 6,
		DTS = 7,
		ATRAC = 8,
		ONE_BIT_AUDIO = 9,
		DOLBY_DIGITAL_PLUS = 10,
		DTS_HD = 11,
		MAT = 12,
		DST = 13,
		WMAPRO = 14.
		*/
		if (hdmi_para.data_raw > 1) {
			hdmi_para.sample_bit = 24;
		}
		if (hdmi_para.channel_num == 8) {
			hdmi_para.ca = 0x12;
		} else if ((hdmi_para.channel_num >= 3)) {
			hdmi_para.ca = 0x1f;
		} else {
			hdmi_para.ca = 0x0;
		}
	}
	/*spdif*/
	if (spdif_used && hub_spdif_en) {

		int format;
		switch (params_format(params))
		{
			case SNDRV_PCM_FORMAT_S16_LE:
			format = 16;
			break;
		case SNDRV_PCM_FORMAT_S20_3LE:
			format = 20;
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			format = 24;
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			format = 24;
			break;
		default:
			return -EINVAL;
		}
		#ifdef CONFIG_SND_SUNXI_SOC_SPDIF
		spdif_hw_params(format);
		#endif
	}
	/* play or record */
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		if (hub_codec_en) {
			dma_data = &sunxi_daudio_pcm_stereo_out;
		} else {
			dma_data = &sunxi_hdmiaudio_pcm_stereo_out;
		}
	} else{
		dma_data = &sunxi_daudio_pcm_stereo_in;
	}
	snd_soc_dai_set_dma_data(rtd->cpu_dai, substream, dma_data);
	return 0;
}
static int  sunxi_audiohub_trigger(struct snd_pcm_substream *substream,
                              int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;
	switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				sunxi_snd_rxctrl_daudio0(substream, 1);
			} else {

				if (spdif_used && hub_spdif_en) {
					/*spdif*/
					#ifdef CONFIG_SND_SUNXI_SOC_SPDIF
					spdif_txctrl_enable(1, substream->runtime->channels, 1);
					#endif
				}
				if (hdmi_used && hub_hdmi_en){
					/*hdmi*/
					#ifdef CONFIG_SND_SUNXI_SOC_HDMIAUDIO
					sunxi_snd_txctrl_i2s1(substream, 1,1);
					#endif
				}
				/*daudio*/
				if (hub_codec_en) {
					sunxi_snd_txctrl_daudio0(substream, 1,1);
				}

		}
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				sunxi_snd_rxctrl_daudio0(substream, 0);
			} else {
				/*daudio*/
				if (hub_codec_en) {
				  	sunxi_snd_txctrl_daudio0(substream, 0,0);
				}
				/*hdmi*/
				if (hdmi_used && hub_hdmi_en){
					#ifdef CONFIG_SND_SUNXI_SOC_HDMIAUDIO
					sunxi_snd_txctrl_i2s1(substream, 0,0);
					#endif
				}
				/*spdif*/
				if (spdif_used && hub_spdif_en) {
					#ifdef CONFIG_SND_SUNXI_SOC_SPDIF
					spdif_txctrl_enable(0, substream->runtime->channels, 0);
					#endif
				}

			}
			break;
		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}

static int sunxi_audiohub_set_sysclk(struct snd_soc_dai *cpu_dai, int clk_id,
                                 unsigned int freq, int daudio_pcm_select)
{
	sunxi_daudio0_set_rate(freq);
//	sunxi_hdmii2s_set_rate(freq);
	return 0;
}
static int sunxi_audiohub_set_clkdiv(struct snd_soc_dai *cpu_dai, int div_id, int sample_rate)
{
	#ifdef CONFIG_SND_SUNXI_SOC_SPDIF
	u32 spdif_mpll=0, spdif_bclk_div=0, spdif_mclk_div=0,spdif_mult_fs=0;
	#endif
	if (codec_used && hub_codec_en)
		sunxi_daudio0_set_clkdiv(cpu_dai, div_id, sample_rate);
	/*hdmi*/
	if (hdmi_used && hub_hdmi_en){
		#ifdef CONFIG_SND_SUNXI_SOC_HDMIAUDIO
		 sunxi_i2s1_set_clkdiv(cpu_dai, div_id, sample_rate);
		#endif
	}
	/*spdif*/
	if (spdif_used && hub_spdif_en) {
		#ifdef CONFIG_SND_SUNXI_SOC_SPDIF
		get_clock_divder(sample_rate, 32, &spdif_mclk_div, &spdif_mpll, &spdif_bclk_div, &spdif_mult_fs);
		spdif_set_clkdiv(0, spdif_mclk_div);
		#endif
	}
	return 0;
}
static int sunxi_audiohub_perpare(struct snd_pcm_substream *substream,
	struct snd_soc_dai *cpu_dai)
{
	if (codec_used && hub_codec_en){
		sunxi_daudio0_perpare(substream,cpu_dai);
	}
	if (hdmi_used && hub_hdmi_en){
		/*hdmi*/
		if ((hdmi_para.data_raw > 1)||(hdmi_para.sample_bit!=16)||(hdmi_para.channel_num >= 3)) {
			atomic_set(&pcm_count_num_muti, 0);
		} else {
			atomic_inc(&pcm_count_num_muti);
		}
		/*
		*set the first pcm param, need set the hdmi audio pcm param
		*set the data_raw param, need set the hdmi audio raw param
		*/
		if (atomic_read(&pcm_count_num_muti) <= 1) {
			if (g_hdmi_func.hdmi_set_audio_para == NULL)
				pr_err("obtain the hdmi func failed. %s,line:%d\n",__func__,__LINE__);
			g_hdmi_func.hdmi_set_audio_para(&hdmi_para);
			}
		g_hdmi_func.hdmi_audio_enable(1, 1);
	}
	return 0;
}
static void sunxi_audiohub_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	if (hdmi_used && hub_hdmi_en){
		g_hdmi_func.hdmi_audio_enable(0, 1);
	}
}

static int sunxi_audiohub_dai_probe(struct snd_soc_dai *dai)
{
	if (hdmi_used ){
		atomic_set(&pcm_count_num_muti, 0);
	}
	return 0;
}

static int sunxi_audiohub_dai_remove(struct snd_soc_dai *dai)
{
	return 0;
}

static int sunxi_audiohub_suspend(struct snd_soc_dai *cpu_dai)
{
	return 0;
}

static int sunxi_audiohub_resume(struct snd_soc_dai *cpu_dai)
{
	/*hdmi*/
	if (hdmi_used)
		atomic_set(&pcm_count_num_muti, 0);
	return 0;
}

#define SUNXI_VIRAUDIO_RATES (SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT)
static struct snd_soc_dai_ops sunxi_audiohub_dai_ops = {
	.trigger 	= sunxi_audiohub_trigger,
	.hw_params 	= sunxi_audiohub_hw_params,
	.set_fmt 	= sunxi_audiohub_set_fmt,
	.set_clkdiv = sunxi_audiohub_set_clkdiv,
	.set_sysclk = sunxi_audiohub_set_sysclk,
	.prepare  =	sunxi_audiohub_perpare,
	.shutdown 	= sunxi_audiohub_shutdown,
};

static struct snd_soc_dai_driver sunxi_audiohub_dai[] = {
	{
		.name = "sunxi-multi-dai",
		.id = 1,
	.probe 		= sunxi_audiohub_dai_probe,
	.suspend 	= sunxi_audiohub_suspend,
	.resume 	= sunxi_audiohub_resume,
	.remove 	= sunxi_audiohub_dai_remove,
	.playback 	= {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUNXI_VIRAUDIO_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture 	= {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUNXI_VIRAUDIO_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops 		= &sunxi_audiohub_dai_ops,
	}
};
static int __init sunxi_audiohub_dev_probe(struct platform_device *pdev)
{
	int ret = 0;
	script_item_u val;
	script_item_value_type_e  type;

	/*hdmi register baseaddress*/
	sunxi_hdmiaudio.regs = (void __iomem *)SUNXI_I2S1_VBASE;

	/*spdif register baseaddress*/
	sunxi_spdifaudio.regs = (void __iomem *)SUNXI_SPDIF_VBASE;

	/*TDM+AC100*/
	sunxi_tdmaudio.regs = (void __iomem *)SUNXI_DAUDIO_VBASE;

	type = script_get_item("audiohub", "spdif_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        	pr_err("[audiohub] spdif_used type err!\n");
    	} else {
		spdif_used = val.val;
    	}

	type = script_get_item("audiohub", "hdmi_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        	pr_err("[audiohub] hdmi_used type err!\n");
    	} else {
		hdmi_used = val.val;
    	}

	type = script_get_item("audiohub", "codec_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        	pr_err("[audiohub] codec_used type err!\n");
    	} else {
		codec_used = val.val;
	}

	ret = snd_soc_register_dais(&pdev->dev,sunxi_audiohub_dai, ARRAY_SIZE(sunxi_audiohub_dai));
	if (ret) {
		dev_err(&pdev->dev, "Failed to register DAI\n");
	}

	return 0;
}

static int __exit sunxi_audiohub_dev_remove(struct platform_device *pdev)
{
	return 0;
}

/*data relating*/
static struct platform_device sunxi_audiohub_device = {
	.name 	= "audio-mutidai",
	.id 	= PLATFORM_DEVID_NONE,
};

/*method relating*/
static struct platform_driver sunxi_audiohub_driver = {
	.probe = sunxi_audiohub_dev_probe,
	.remove = __exit_p(sunxi_audiohub_dev_remove),
	.driver = {
		.name = "audio-mutidai",
		.owner = THIS_MODULE,
	},
};

static int __init sunxi_audiohub_init(void)
{
	int err = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audiohub", "hub_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[audiohub] type err!\n");
    }

	hub_used = val.val;
 	if (hub_used) {
		if((err = platform_device_register(&sunxi_audiohub_device)) < 0)
			return err;	

		if ((err = platform_driver_register(&sunxi_audiohub_driver)) < 0)
			return err;
	} else {
        pr_err("[audiohub] cannot find any using configuration for controllers, return directly!\n");
        return 0;
    }

	return 0;
}
module_init(sunxi_audiohub_init);
module_param_named(hub_codec_en, hub_codec_en, bool, S_IRUGO | S_IWUSR);
module_param_named(hub_hdmi_en, hub_hdmi_en, bool, S_IRUGO | S_IWUSR);
module_param_named(hub_spdif_en, hub_spdif_en, bool, S_IRUGO | S_IWUSR);
static void __exit sunxi_audiohub_exit(void)
{
	if (hub_used) {
		hub_used = 0;
		platform_driver_unregister(&sunxi_audiohub_driver);
		platform_device_unregister(&sunxi_audiohub_device);
	}
}
module_exit(sunxi_audiohub_exit);

/* Module information */
MODULE_AUTHOR("huangxin");
MODULE_DESCRIPTION("sunxi vir Audio Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-audio-hub");

