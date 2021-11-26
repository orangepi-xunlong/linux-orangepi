// SPDX-License-Identifier: GPL-2.0-only
/*
 * (C) Copyright 2014-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * huangxin <huangxin@allwinnertech.com>
 *
 * (C) Copyright 2021
 * Shenzhen Xunlong Software Co., Ltd. <www.orangepi.org>
 * Leeboby <leeboby@aliyun.com>
 *
 */

#include <linux/module.h>
#include <sound/soc.h>

#define DRV_NAME "sunxi-hdmi"

static int sunxi_hdmi_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *params)
{
	int ret = 0;
	struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(substream);
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	struct snd_soc_dai *codec_dai = asoc_rtd_to_codec(rtd, 0);

	unsigned int freq, clk_div;

	switch (params_rate(params)) {

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
			freq = 98304000;
			//freq = 24576000;
			break;
		case    11025:
		case    22050:
		case    44100:
		case    88200:
		case    176400:
			freq = 90316800;
			//freq = 22579200;
			break;
		default:
		        return -EINVAL;
	}

	/*set system clock source freq and set the mode as i2s0 or pcm*/
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0, freq, 0);
	if (ret < 0)
		return ret;

	/*
	 * I2S mode normal bit clock + frame\codec clk & FRM slave
	 */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
	                SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	clk_div = freq / params_rate(params);
	ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, clk_div);
	if (ret < 0)
		return ret;

	return 0;
}

static struct snd_soc_ops sunxi_sound_hdmi_ops = {
	.hw_params = sunxi_hdmi_hw_params,
};

SND_SOC_DAILINK_DEFS(audio,
        DAILINK_COMP_ARRAY(COMP_EMPTY()),
        DAILINK_COMP_ARRAY(COMP_CODEC("hdmi-audio-codec.4.auto", "i2s-hifi")),
        DAILINK_COMP_ARRAY(COMP_EMPTY()));

static struct snd_soc_dai_link sunxi_dailink = {
	.name = "HDMIAUDIO",
	.stream_name = "hdmi",
	.ops = &sunxi_sound_hdmi_ops,
	SND_SOC_DAILINK_REG(audio),
};

static struct snd_soc_card sunxi_sound_card = {
	.name = "hdmi-audio",
	.owner = THIS_MODULE,
	.dai_link = &sunxi_dailink,
	.num_links =  1,
};

static int sunxi_sound_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct snd_soc_dai_link *dai_link;
	struct snd_soc_card *card;
	char name[30] = "allwinner-hdmi";
	unsigned int temp_val;
	int ret = 0;

	card = devm_kzalloc(&pdev->dev, sizeof(struct snd_soc_card),
			GFP_KERNEL);
	if (!card)
	        return -ENOMEM;
	
	memcpy(card, &sunxi_sound_card, sizeof(struct snd_soc_card));
	
	card->name = name;
	card->dev = &pdev->dev;
	
	dai_link = devm_kzalloc(&pdev->dev,
	                sizeof(struct snd_soc_dai_link), GFP_KERNEL);
	if (!dai_link) {
	        ret = -ENOMEM;
	        goto err_kfree_card;
	}

	memcpy(dai_link, &sunxi_dailink,
	                sizeof(struct snd_soc_dai_link));
	card->dai_link = dai_link;

	dai_link->cpus->of_node = of_parse_phandle(np,
	                                "sunxi,cpudai-controller", 0);
	if (!dai_link->cpus->of_node) {
		dev_err(&pdev->dev,
		        "Property 'sunxi,cpudai-controller' invalid\n");
		ret = -EINVAL;
		goto err_kfree_link;
	}

	dai_link->platforms->name = "snd-soc-dummy";

	ret = of_property_read_string(np, "sunxi,snddaudio-codec",
	                &dai_link->codecs->name);
	if (ret < 0)
	{
		dev_err(&pdev->dev, 
			"Property 'sunxi,snddaudio-codec' invalid\n");
		ret = -EINVAL;
		goto err_kfree_link;
	}

	ret = snd_soc_register_card(card);
	if (ret) {
	        dev_err(card->dev, "snd_soc_register_card failed\n");
	        goto err_kfree_link;
	}

	return ret;

err_kfree_link:
        devm_kfree(&pdev->dev, card->dai_link);
err_kfree_card:
        devm_kfree(&pdev->dev, card);

        return ret;
}

static const struct of_device_id sunxi_sound_of_match[] = {
	{ .compatible = "allwinner,sunxi-hdmi-machine", },
	{},
};

static struct platform_driver sunxi_sound_driver = {
	.probe = sunxi_sound_probe,
	.driver = {
		.name = DRV_NAME,
		.of_match_table = sunxi_sound_of_match,
	},
};

module_platform_driver(sunxi_sound_driver);

MODULE_DESCRIPTION("Allwinner ASoC Machine Driver");
MODULE_AUTHOR("wolfgang huang <huangjinhui@allwinnertech.com>");
MODULE_AUTHOR("Leeboby <leeboby@aliyun.com>");
MODULE_LICENSE("GPL v2");
