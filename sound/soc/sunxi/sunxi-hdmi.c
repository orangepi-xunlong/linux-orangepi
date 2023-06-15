/*
 * sound\soc\sunxi\sunxi-hdmi.c
 * (C) Copyright 2014-2020
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yumingfeng <yumingfeng@allwinnertech.com>
 * luguofang <luguofang@allwinnertech.com>
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
#include <sound/simple_card.h>
#include <linux/io.h>

#include "sunxi-pcm.h"

#include <video/drv_hdmi.h>

static bool	hdmiaudio_reset_en;

struct sunxi_hdmi_priv {
	hdmi_audio_t hdmi_para;
	bool update_param;
};

atomic_t			pcm_count_num;
static __audio_hdmi_func	g_hdmi_func;
static hdmi_audio_t		hdmi_para;

module_param_named(hdmiaudio_reset_en, hdmiaudio_reset_en,
		bool, S_IRUGO | S_IWUSR);

void audio_set_hdmi_func(__audio_hdmi_func *hdmi_func)
{
	if (hdmi_func) {
		g_hdmi_func.hdmi_audio_enable	= hdmi_func->hdmi_audio_enable;
		g_hdmi_func.hdmi_set_audio_para	= hdmi_func->hdmi_set_audio_para;
		g_hdmi_func.hdmi_is_playback	= hdmi_func->hdmi_is_playback;
	} else {
		pr_err("error:%s,line:%d\n", __func__, __LINE__);
	}
}
EXPORT_SYMBOL(audio_set_hdmi_func);

int sunxi_hdmi_codec_hw_params(struct snd_pcm_substream *substream,
			       struct snd_pcm_hw_params *params,
			       struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct asoc_simple_priv *sndhdmi_priv = snd_soc_card_get_drvdata(card);
	struct sunxi_hdmi_priv *sunxi_hdmi = snd_soc_component_get_drvdata(dai->component);

	hdmi_para.sample_rate = params_rate(params);
	hdmi_para.channel_num = params_channels(params);
	hdmi_para.data_raw = sndhdmi_priv->hdmi_format;

	if (sunxi_hdmi != NULL) {
		if (sunxi_hdmi->hdmi_para.sample_rate != hdmi_para.sample_rate) {
			sunxi_hdmi->hdmi_para.sample_rate = hdmi_para.sample_rate;
			sunxi_hdmi->update_param = 1;
		}
		if (sunxi_hdmi->hdmi_para.channel_num != hdmi_para.channel_num) {
			sunxi_hdmi->hdmi_para.channel_num = hdmi_para.channel_num;
			sunxi_hdmi->update_param = 1;
		}
		if (sunxi_hdmi->hdmi_para.data_raw != hdmi_para.data_raw) {
			sunxi_hdmi->hdmi_para.data_raw = hdmi_para.data_raw;
			sunxi_hdmi->update_param = 1;
		}
	} else {
		pr_warn("[%s] sunxi_hdmi is null.\n", __func__);
	}

	switch (params_format(params)) {
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
	 * PCM = 1,
	 * AC3 = 2,
	 * MPEG1 = 3,
	 * MP3 = 4,
	 * MPEG2 = 5,
	 * AAC = 6,
	 * DTS = 7,
	 * ATRAC = 8,
	 * ONE_BIT_AUDIO = 9,
	 * DOLBY_DIGITAL_PLUS = 10,
	 * DTS_HD = 11,
	 * MAT = 12,
	 * DST = 13,
	 * WMAPRO = 14.
	 */
	if (hdmi_para.data_raw > 1)
		hdmi_para.sample_bit = 24;

	if ((sunxi_hdmi != NULL) &&
	    (sunxi_hdmi->hdmi_para.sample_bit != hdmi_para.sample_bit)) {
		sunxi_hdmi->hdmi_para.sample_bit = hdmi_para.sample_bit;
		sunxi_hdmi->update_param = 1;
	}

	if (hdmi_para.channel_num == 8)
		hdmi_para.ca = 0x12;
	else if (hdmi_para.channel_num == 6)
		hdmi_para.ca = 0x0b;
	else if ((hdmi_para.channel_num >= 3))
		hdmi_para.ca = 0x1f;
	else
		hdmi_para.ca = 0x0;

	if ((sunxi_hdmi != NULL) &&
	    (sunxi_hdmi->hdmi_para.ca != hdmi_para.ca)) {
		sunxi_hdmi->hdmi_para.ca = hdmi_para.ca;
		sunxi_hdmi->update_param = 1;
	}

	return 0;
}
EXPORT_SYMBOL(sunxi_hdmi_codec_hw_params);

static int sunxi_hdmi_codec_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	return 0;
}

int sunxi_hdmi_codec_prepare(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct sunxi_hdmi_priv *sunxi_hdmi = snd_soc_component_get_drvdata(dai->component);

	if (hdmiaudio_reset_en == false) {
		if ((sunxi_hdmi != NULL) && (sunxi_hdmi->update_param)) {
			pr_warn("sunxi->update_param:%d\n", sunxi_hdmi->update_param);
			atomic_set(&pcm_count_num, 0);
			sunxi_hdmi->update_param = 0;
		} else if (atomic_read(&pcm_count_num) == 0)
			atomic_inc(&pcm_count_num);
	} else if (hdmiaudio_reset_en == true)
		atomic_set(&pcm_count_num, 0);

	/*
	 * set the first pcm param, need set the hdmi audio pcm param
	 * set the data_raw param, need set the hdmi audio raw param
	 */
	if (!g_hdmi_func.hdmi_set_audio_para) {
		pr_warn("hdmi video isn't insmod, hdmi interface is null\n");
		return 0;
	}

	if (hdmiaudio_reset_en) {
		pr_warn("[%s] raw:%d, sample_bit:%d, channel:%d, "
			"sample_rate:%d, hdmiaudio_reset_en:%d\n", __func__,
			hdmi_para.data_raw, hdmi_para.sample_bit,
			hdmi_para.channel_num, hdmi_para.sample_rate,
			hdmiaudio_reset_en);
	}

	if (atomic_read(&pcm_count_num) < 1) {
		g_hdmi_func.hdmi_set_audio_para(&hdmi_para);
		g_hdmi_func.hdmi_audio_enable(1, 1);
		/*
		 * When the params was be changed,
		 * the hdmi clk should be shake hands again,
		 * it needs some time to finishe.
		 */
		//msleep(1200);
	}
	return 0;
}
EXPORT_SYMBOL(sunxi_hdmi_codec_prepare);

void sunxi_hdmi_codec_shutdown(struct snd_pcm_substream *substream,
					struct snd_soc_dai *dai)
{
	struct sunxi_hdmi_priv *sunxi_hdmi = snd_soc_component_get_drvdata(dai->component);

	if (sunxi_hdmi)
		sunxi_hdmi->update_param = 0;

	if (g_hdmi_func.hdmi_audio_enable)
		g_hdmi_func.hdmi_audio_enable(0, 1);
}
EXPORT_SYMBOL(sunxi_hdmi_codec_shutdown);

static int sunxi_hdmi_codec_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	return 0;
}

static int sunxi_hdmi_codec_suspend(struct snd_soc_component *dai)
{
	return 0;
}

static int sunxi_hdmi_codec_resume(struct snd_soc_component *dai)
{
	atomic_set(&pcm_count_num, 0);
	return 0;
}

static struct snd_soc_dai_ops sunxi_hdmi_dai_ops = {
	.hw_params	= sunxi_hdmi_codec_hw_params,
	.set_fmt	= sunxi_hdmi_codec_set_dai_fmt,
	.trigger	= sunxi_hdmi_codec_trigger,
	.prepare	= sunxi_hdmi_codec_prepare,
	.shutdown	= sunxi_hdmi_codec_shutdown,
};

static struct snd_soc_dai_driver sunxi_hdmi_codec_dai = {
	.name		= "audiohdmi-dai",
	.playback	= {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 8,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	/* HDMI capture only for i2s loop(chan <= 2ch) debug */
	.capture	= {
		.stream_name	= "Capture",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops			= &sunxi_hdmi_dai_ops,
};

static int sunxi_hdmi_codec_probe(struct snd_soc_component *component)
{
	struct sunxi_hdmi_priv *sunxi_hdmi = NULL;

	atomic_set(&pcm_count_num, 0);

	if (!component) {
		pr_err("error:%s,line:%d\n", __func__, __LINE__);
		return -EAGAIN;
	}

	sunxi_hdmi = kzalloc(sizeof(struct sunxi_hdmi_priv), GFP_KERNEL);
	if (sunxi_hdmi == NULL) {
		pr_err("[%s] cannot malloc sunxi_hdmi.\n", __func__);
		return -ENOMEM;
	}
	snd_soc_component_set_drvdata(component, sunxi_hdmi);

	return 0;
}

static void sunxi_hdmi_codec_remove(struct snd_soc_component *component)
{
	struct sunxi_hdmi_priv *sunxi_hdmi = NULL;


	if (!component) {
		pr_err("[%s] component is null !!!\n", __func__);
	} else {
		sunxi_hdmi = snd_soc_component_get_drvdata(component);
		kfree(sunxi_hdmi);
	}
}

static struct snd_soc_component_driver soc_codec_dev_sunxi_hdmi = {
	.name		= "sunxi-hdmiaudio",
	.probe		= sunxi_hdmi_codec_probe,
	.remove		= sunxi_hdmi_codec_remove,
	.suspend	= sunxi_hdmi_codec_suspend,
	.resume		= sunxi_hdmi_codec_resume,
};

static int sunxi_hdmi_codec_dev_probe(struct platform_device *pdev)
{
	if (!pdev) {
		pr_err("error:%s,line:%d\n", __func__, __LINE__);
		return -EAGAIN;
	}

	return snd_soc_register_component(&pdev->dev, &soc_codec_dev_sunxi_hdmi,
				&sunxi_hdmi_codec_dai, 1);
}

static int __exit sunxi_hdmi_codec_dev_remove(struct platform_device *pdev)
{
	if (!pdev) {
		pr_err("error:%s,line:%d\n", __func__, __LINE__);
		return -EAGAIN;
	}

	snd_soc_unregister_component(&pdev->dev);

	return 0;
}

static const struct of_device_id sunxi_hdmi_codec_of_match[] = {
	{ .compatible = "allwinner,sunxi-hdmiaudio", },
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_hdmi_codec_of_match);

static struct platform_driver sunxi_hdmi_codec_driver = {
	.probe = sunxi_hdmi_codec_dev_probe,
	.remove = __exit_p(sunxi_hdmi_codec_dev_remove),
	.driver = {
		.name	= "sunxi-hdmiaudio",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_hdmi_codec_of_match,
	},
};

static int __init sunxi_hdmi_codec_driver_init(void)
{
	return platform_driver_register(&sunxi_hdmi_codec_driver);
}
module_init(sunxi_hdmi_codec_driver_init);

static void __exit sunxi_hdmi_codec_driver_exit(void)
{
	platform_driver_unregister(&sunxi_hdmi_codec_driver);
}
module_exit(sunxi_hdmi_codec_driver_exit);

MODULE_AUTHOR("luguofang <luguofang@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI HDMIAUDIO ASoC DRIVER");
MODULE_ALIAS("platform:sunxi-hdmiaudio");
MODULE_LICENSE("GPL");
