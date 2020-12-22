/*
 * sound\soc\sunxi\audiohub\sun8iw7-hubaudio.c
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

#include "./../audiocodec/sun8iw7_sndcodec.h"
#include "./../hdmiaudio/sunxi-hdmitdm.h"
#include "./../spdif/sunxi_spdif.h"
//#include "sun8iw7-hubaudio.h"
#include <video/drv_hdmi.h>

static __audio_hdmi_func 	g_hdmi_func;
static hdmi_audio_t 		hdmi_para;
atomic_t pcm_count_num_muti;
static int   sample_resolution 	= 0;
static int   hub_used		= 0;
static bool  spdif_used		= 0;
static bool  hdmi_used		= 0;
static bool  codec_used		= 0;
static bool  hdmiaudio_reset_en = false;
static struct sunxi_dma_params sunxi_daudio_pcm_stereo_out = {
	.name		= "daudio2_play",
	.dma_addr	= SUNXI_DAUDIO2BASE + SUNXI_DAUDIOTXFIFO,/*send data address	*/
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

static int sunxi_audiohub_trigger(struct snd_pcm_substream *substream,
                              int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;
	switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				if (codec_used) {
					#ifdef CONFIG_SND_SUN8IW7_SNDCODEC
					audiocodec_hub_enable(1);
					#endif
				}
				if (hdmi_used){
					#ifdef CONFIG_SND_SUN8IW7_HDMIPCM
					tdm2_tx_enable(1, 1);
					#endif
				}
				if (spdif_used) {
					#ifdef CONFIG_SND_SUNXI_SOC_SPDIF
					spdif_txctrl_enable(1, substream->runtime->channels, 1);
					#endif
				}
			} else {
			}
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			} else {
				if (codec_used) {
					#ifdef CONFIG_SND_SUN8IW7_SNDCODEC
					audiocodec_hub_enable(0);
					#endif
				}
				if (hdmi_used){
					#ifdef CONFIG_SND_SUN8IW7_HDMIPCM
					tdm2_tx_enable(0, 0);
					#endif
				}
				if (spdif_used) {
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

static int sunxi_audiohub_perpare(struct snd_pcm_substream *substream,
	struct snd_soc_dai *cpu_dai)
{
	u32 reg_val;
	/* play or record */
	if (hdmi_used) {
		#ifdef CONFIG_SND_SUN8IW7_HDMIPCM
		tdm2_prepare(substream);
		#endif
		if ((hdmi_para.data_raw > 1)||(hdmi_para.sample_bit!=16)||(hdmi_para.channel_num != 2)||(hdmi_para.sample_rate != 44100)||( hdmiaudio_reset_en == true)) {
			atomic_set(&pcm_count_num_muti, 0);
		} else {
			atomic_inc(&pcm_count_num_muti);
		}
		if (!g_hdmi_func.hdmi_set_audio_para) {
			printk(KERN_WARNING "hdmi video isn't insmod, hdmi interface is null\n");
			return 0;
		}
		if (hdmiaudio_reset_en) {
			pr_err("%s,l:%d,hdmi_para.data_raw:%d, hdmi_para.sample_bit:%d, hdmi_para.channel_num:%d, hdmi_para.sample_rate:%d, hdmiaudio_reset_en:%d\n",\
				__func__, __LINE__, hdmi_para.data_raw, hdmi_para.sample_bit, hdmi_para.channel_num, hdmi_para.sample_rate, hdmiaudio_reset_en);
		}
		/*Global Enable Digital Audio Interface*/
		reg_val = readl(SUNXI_DAUDIO2_VBASE + SUNXI_DAUDIOCTL);
		reg_val |= SUNXI_DAUDIOCTL_GEN;
		writel(reg_val, SUNXI_DAUDIO2_VBASE + SUNXI_DAUDIOCTL);
		msleep(10);
		/*flush TX FIFO*/
		reg_val = readl(SUNXI_DAUDIO2_VBASE + SUNXI_DAUDIOFCTL);
		reg_val |= SUNXI_DAUDIOFCTL_FTX;
		writel(reg_val, SUNXI_DAUDIO2_VBASE + SUNXI_DAUDIOFCTL);
		/*
		*	set the first pcm param, need set the hdmi audio pcm param
		*	set the data_raw param, need set the hdmi audio raw param
		*/
		if (atomic_read(&pcm_count_num_muti) <= 1) {
			if (g_hdmi_func.hdmi_set_audio_para == NULL)
				pr_err("obtain the hdmi func failed. %s,line:%d\n",__func__,__LINE__);
				g_hdmi_func.hdmi_set_audio_para(&hdmi_para);
				g_hdmi_func.hdmi_audio_enable(1, 1);
		}
	}
	return 0;
}

static int sunxi_audiohub_hw_params(struct snd_pcm_substream *substream,
																struct snd_pcm_hw_params *params,
																struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sunxi_dma_params *dma_data;

	switch (params_format(params))
	{
		case SNDRV_PCM_FORMAT_S16_LE:
			sample_resolution = 16;
			break;
		case SNDRV_PCM_FORMAT_S20_3LE:
			sample_resolution = 24;
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			sample_resolution = 24;
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			sample_resolution = 24;
			break;
		default:
			return -EINVAL;
	}
	if (hdmi_used) {
		#ifdef CONFIG_SND_SUN8IW7_HDMIPCM
		tdm2_hw_params(sample_resolution);
		#endif
		hdmi_para.sample_rate = params_rate(params);
		hdmi_para.channel_num = params_channels(params);
		hdmi_para.data_raw 		= 1;
		hdmi_para.sample_bit = sample_resolution;
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
			hdmi_para.sample_bit = 24; //??? TODO
		}
		if (hdmi_para.channel_num == 8) {
			hdmi_para.ca = 0x12;
		} else if ((hdmi_para.channel_num >= 3)) {
			hdmi_para.ca = 0x1f;
		} else {
			hdmi_para.ca = 0x0;
		}
	}

	if (spdif_used) {
		#ifdef CONFIG_SND_SUNXI_SOC_SPDIF
		spdif_hw_params(sample_resolution);
		#endif
	}

	if (hdmi_used) {
		/* play or record */
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			dma_data = &sunxi_daudio_pcm_stereo_out;
		}
		snd_soc_dai_set_dma_data(rtd->cpu_dai, substream, dma_data);
	}

	return 0;
}

static int sunxi_audiohub_set_fmt(struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
	if (hdmi_used) {
		#ifdef CONFIG_SND_SUN8IW7_HDMIPCM
		tdm2_set_fmt(fmt);
		#endif
	}
	if (spdif_used) {
		#ifdef CONFIG_SND_SUNXI_SOC_SPDIF
		spdif_set_fmt(0);
		#endif
	}
	return 0;
}

static int sunxi_audiohub_set_sysclk(struct snd_soc_dai *cpu_dai, int clk_id,
                                 unsigned int freq, int daudio_pcm_select)
{
	#ifdef CONFIG_SND_SUN8IW7_HDMIPCM
	tdm2_set_rate(freq);
	#endif
	return 0;
}

static int sunxi_audiohub_set_clkdiv(struct snd_soc_dai *cpu_dai, int div_id, int sample_rate)
{
	#ifdef CONFIG_SND_SUNXI_SOC_SPDIF
	u32 spdif_mclk_div=0;
	u32 spdif_mpll=0, spdif_bclk_div=0, spdif_mult_fs=0;
	#endif

	if (hdmi_used) {
		#ifdef CONFIG_SND_SUN8IW7_HDMIPCM
		tdm2_set_clkdiv(sample_rate);
		#endif
	}

	if (spdif_used) {
		#ifdef CONFIG_SND_SUNXI_SOC_SPDIF
		get_clock_divder(sample_rate, 32, &spdif_mclk_div, &spdif_mpll, &spdif_bclk_div, &spdif_mult_fs);
		spdif_set_clkdiv(SUNXI_DIV_MCLK, spdif_mclk_div);
		#endif
	}
	return 0;
}

static int sunxi_audiohub_dai_probe(struct snd_soc_dai *dai)
{
	if (hdmi_used){
		atomic_set(&pcm_count_num_muti, 0);
	}
	return 0;
}

static int sunxi_audiohub_dai_remove(struct snd_soc_dai *dai)
{
	return 0;
}

static int sunxi_audiohub_suspend(struct platform_device *pdev,pm_message_t state)
{
	return 0;
}

static int sunxi_audiohub_resume(struct platform_device *pdev)
{
	/*hdmi*/
	if (hdmi_used)
		atomic_set(&pcm_count_num_muti, 0);

	return 0;
}

static void sunxi_audiohub_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	/*hdmi*/
	if (hdmi_used && g_hdmi_func.hdmi_audio_enable) {
		g_hdmi_func.hdmi_audio_enable(0, 1);
	}
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

	type = script_get_item("audiohub", "spdif_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        	pr_err("[audiohub] spdif_used type err!\n");
		spdif_used = 0;
	} else
		spdif_used = val.val;

	type = script_get_item("audiohub", "hdmi_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        	pr_err("[audiohub] hdmi_used type err!\n");
		hdmi_used = 0;
	} else
		hdmi_used = val.val;

	type = script_get_item("audiohub", "codec_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        	pr_err("[audiohub] codec_used type err!\n");
		codec_used = 0;
	} else
		codec_used = val.val;

	ret = snd_soc_register_dais(&pdev->dev,sunxi_audiohub_dai, ARRAY_SIZE(sunxi_audiohub_dai));
	if (ret) {
		dev_err(&pdev->dev, "Failed to register DAI\n");
	}

	return 0;
}

static int __exit sunxi_audiohub_dev_remove(struct platform_device *pdev)
{
	if (hub_used) {
		hub_used = 0;
		snd_soc_unregister_dai(&pdev->dev);
		platform_set_drvdata(pdev, NULL);
	}
	return 0;
}

/*data relating*/
static struct platform_device sunxi_audiohub_device = {
	.name 	= "tdm0",
	.id 	= PLATFORM_DEVID_NONE,
};

/*method relating*/
static struct platform_driver sunxi_audiohub_driver = {
	.probe = sunxi_audiohub_dev_probe,
	.remove = __exit_p(sunxi_audiohub_dev_remove),
	.driver = {
		.name = "tdm0",
		.owner = THIS_MODULE,
	},
	.suspend	= sunxi_audiohub_suspend,
	.resume		= sunxi_audiohub_resume,
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

module_param_named(hdmiaudio_reset_en, hdmiaudio_reset_en, bool, S_IRUGO | S_IWUSR);

static void __exit sunxi_audiohub_exit(void)
{
	if (hub_used) {
		hub_used = 0;
		platform_driver_unregister(&sunxi_audiohub_driver);
	}
}
module_exit(sunxi_audiohub_exit);

/* Module information */
MODULE_AUTHOR("huangxin");
MODULE_DESCRIPTION("sunxi vir Audio Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-audio-hub");

