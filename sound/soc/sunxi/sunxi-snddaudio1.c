/*
 * sound\soc\sunxi\sunxi_snddaudio1.c
 * (C) Copyright 2014-2016
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
#include <linux/of.h>

static int sunxi_snddaudio1_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	unsigned int freq, clk_div;
	int ret;

	switch (params_rate(params)) {
	case	8000:
	case	16000:
	case	32000:
	case	64000:
	case	128000:
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
		pr_err("unsupport params rate\n");
		return -EINVAL;
	}

	/*set system clock source freq and set the mode as daudio or pcm*/
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0 , freq, 0);
	if (ret < 0) {
		return ret;
	}

	/*set system clock source freq and set the mode as daudio or pcm*/
	ret = snd_soc_dai_set_sysclk(codec_dai, 0 , freq, 0);
	if (ret < 0) {
		pr_warn("[daudio1],the codec_dai set set_sysclk failed.\n");
	}

	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
						SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0) {
		pr_warn("[daudio1],the codec_dai set set_fmt failed.\n");
	}

	clk_div = freq / params_rate(params);

	ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, clk_div);
	if (ret < 0) {
		return ret;
	}
	ret = snd_soc_dai_set_clkdiv(codec_dai, 0, clk_div);
	if (ret < 0) {
		pr_warn("[daudio1],the codec_dai set set_clkdiv failed.\n");
	}

	return 0;
}

/*
 * Card initialization
 */
static int sunxi_daudio_init(struct snd_soc_pcm_runtime *rtd)
{
	return 0;
}

static struct snd_soc_ops sunxi_snddaudio_ops = {
	.hw_params 		= sunxi_snddaudio1_hw_params,
};

static struct snd_soc_dai_link sunxi_snddaudio_dai_link = {
	.name 			= "sysvoice",
	.stream_name 	= "SUNXI-TDM1",
	.cpu_dai_name 	= "sunxi-daudio",
	.platform_name	= "sunxi-daudio",
#ifdef CONFIG_SND_SOC_STA32X
	.codec_dai_name = "STA32X",
	.codec_name     = "sta32x.1-001c",
#else
	.codec_dai_name = "snd-soc-dummy-dai",
	.codec_name	= "snd-soc-dummy",
#endif
	.init		= sunxi_daudio_init,
	.ops 			= &sunxi_snddaudio_ops,
};

static struct snd_soc_card snd_soc_sunxi_snddaudio = {
	.name 		= "snddaudio1",
	.owner 		= THIS_MODULE,
	.dai_link 	= &sunxi_snddaudio_dai_link,
	.num_links 	= 1,
};

static int  sunxi_snddaudio1_dev_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;
	struct snd_soc_card *card = &snd_soc_sunxi_snddaudio;
	card->dev = &pdev->dev;
	sunxi_snddaudio_dai_link.cpu_dai_name = NULL;
	sunxi_snddaudio_dai_link.cpu_of_node = of_parse_phandle(np,
				"sunxi,daudio1-controller", 0);
	if (!sunxi_snddaudio_dai_link.cpu_of_node) {
		dev_err(&pdev->dev,
			"Property 'sunxi,daudio1-controller' missing or invalid\n");
			ret = -EINVAL;
	}
	sunxi_snddaudio_dai_link.platform_name = NULL;
	sunxi_snddaudio_dai_link.platform_of_node = sunxi_snddaudio_dai_link.cpu_of_node;

	ret = snd_soc_register_card(card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card() failed: %d\n", ret);
	}

	return ret;
}

static int  __exit sunxi_snddaudio1_dev_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	snd_soc_unregister_card(card);
	return 0;
}

static const struct of_device_id sunxi_daudio1_of_match[] = {
	{ .compatible = "allwinner,sunxi-daudio1-machine", },
	{},
};

/*method relating*/
static struct platform_driver sunxi_daudio_driver = {
	.driver = {
		.name = "snddaudio1",
		.owner = THIS_MODULE,
		.pm = &snd_soc_pm_ops,
		.of_match_table = sunxi_daudio1_of_match,
	},
	.probe = sunxi_snddaudio1_dev_probe,
	.remove = __exit_p(sunxi_snddaudio1_dev_remove),
};

module_platform_driver(sunxi_daudio_driver);

MODULE_AUTHOR("huangxin");
MODULE_DESCRIPTION("SUNXI_snddaudio ALSA SoC audio driver");
MODULE_LICENSE("GPL");
