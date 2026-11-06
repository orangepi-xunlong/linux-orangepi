// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * dummy codec
 *
 */

#include <linux/module.h>
#include <linux/device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <linux/io.h>
#include <linux/proc_fs.h>

u16 tdm_slot_no = 2;
static int cvitek_dummy_hw_params(struct snd_pcm_substream *substream,
				  struct snd_pcm_hw_params *params)
{
	return 0;
}

static int cvitek_dummy_codec_init(struct snd_soc_pcm_runtime *rtd)
{
	return 0;
}

static struct snd_soc_ops cvitek_dummy_ops = {
	.hw_params = cvitek_dummy_hw_params,
};

static struct snd_soc_dai_link cvitek_dummy_dai[] = {
	{
		.ops = &cvitek_dummy_ops,
		.init = cvitek_dummy_codec_init,
		.playback_only = 1,
	},
	{
		.ops = &cvitek_dummy_ops,
		.init = cvitek_dummy_codec_init,
		.capture_only = 1,
	},

};

static struct snd_soc_card cvitek_dummy = {
	.owner = THIS_MODULE,
	.dai_link = cvitek_dummy_dai,
	.num_links = ARRAY_SIZE(cvitek_dummy_dai),
};

static const struct of_device_id cvi_dummy_audio_match_ids[] = {
	{ .compatible = "cvitek,cvitek-dummy", },
	{},
};
MODULE_DEVICE_TABLE(of, cvi_dummy_audio_match_ids);

static int cvitek_dummy_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card;
	struct device_node *np = pdev->dev.of_node, *dai;
	const char *mode;
	const char *fmt;
	const char *role;
	u8 idx = 0;
	u32	slot_no;

	dev_info(&pdev->dev, "%s, dev name=%s\n", __func__, dev_name(&pdev->dev));

	card = &cvitek_dummy;
	if (np) {
		of_property_read_string(np, "cvi,card_name", &card->name);
		of_property_read_u32(np, "cvi,slot_no", &slot_no);
		of_property_read_string(np, "cvi,mode", &mode);
		of_property_read_string(np, "cvi,fmt", &fmt);

		for_each_child_of_node(np, dai) {
			struct snd_soc_dai_link *dai_link = &card->dai_link[idx];
            struct snd_soc_dai_link_component *cpus;
            struct snd_soc_dai_link_component *codecs;
            struct snd_soc_dai_link_component *platforms;

            cpus = devm_kzalloc(&pdev->dev, sizeof(*cpus), GFP_KERNEL);
            codecs = devm_kzalloc(&pdev->dev, sizeof(*codecs), GFP_KERNEL);
            platforms = devm_kzalloc(&pdev->dev, sizeof(*platforms), GFP_KERNEL);

            if (!cpus || !codecs || !platforms)
                return -ENOMEM;

			of_property_read_string(dai, "cvi,dai_name", &dai_link->name);
			of_property_read_string(dai, "cvi,stream_name", &dai_link->stream_name);

			of_property_read_string(dai, "cvi,cpu_name", &cpus->name);
			of_property_read_string(dai, "cvi,cpu_dai_name", &cpus->dai_name);

			of_property_read_string(dai, "cvi,codec_name", &codecs->name);
			of_property_read_string(dai, "cvi,codec_dai_name", &codecs->dai_name);

			of_property_read_string(dai, "cvi,platform_name", &platforms->name);
			of_property_read_string(dai, "cvi,platform_dai_name", &platforms->dai_name);

            dai_link->cpus = cpus;
            dai_link->num_cpus = 1;
            dai_link->codecs = codecs;
            dai_link->num_codecs = 1;
            dai_link->platforms = platforms;
            dai_link->num_platforms = 1;

			dev_info(&pdev->dev, "----star:%d----\n", idx);
			dev_info(&pdev->dev, "dailink_name:%s, stream_name:%s\n",
				 dai_link->name,
				 dai_link->stream_name);
			dev_info(&pdev->dev, "cpus[name:%s, dainame:%s]\n",
				cpus->name,
				cpus->dai_name);
			dev_info(&pdev->dev, "codec[name:%s, dainame:%s]\n",
				codecs->name,
				codecs->dai_name);
			dev_info(&pdev->dev, "platforms[name:%s, dainame:%s]\n",
				platforms->name,
				platforms->dai_name);

			dev_info(&pdev->dev, "----end:%d ----\n", idx);
			card->dev = &pdev->dev;

			of_property_read_string(dai, "cvi,role", &role);

			if (!strcmp(mode, "I2S"))
				dai_link->dai_fmt = SND_SOC_DAIFMT_I2S;
			else if (!strcmp(mode, "LEFT_J"))
				dai_link->dai_fmt = SND_SOC_DAIFMT_LEFT_J;
			else if (!strcmp(mode, "RIGHT_J"))
				dai_link->dai_fmt = SND_SOC_DAIFMT_RIGHT_J;
			else if (!strcmp(mode, "DSP_A"))
				dai_link->dai_fmt = SND_SOC_DAIFMT_DSP_A; /* PCM and TDM belong to it */
			else if (!strcmp(mode, "DSP_B"))
				dai_link->dai_fmt = SND_SOC_DAIFMT_DSP_B; /* PCM and TDM belong to it */
			else if (!strcmp(mode, "PDM"))
				dai_link->dai_fmt = SND_SOC_DAIFMT_PDM;
			else
				dev_err(&pdev->dev, "%s, not support this mode\n", __func__);

			if ((!strcmp(mode, "I2S")) || (!strcmp(mode, "LEFT_J")) || (!strcmp(mode, "RIGHT_J"))) {
				if (!strcmp(fmt, "IBNF"))
					dai_link->dai_fmt |= SND_SOC_DAIFMT_IB_NF;
				else if (!strcmp(fmt, "IBIF"))
					dai_link->dai_fmt |= SND_SOC_DAIFMT_IB_IF;
				else if (!strcmp(fmt, "NBNF"))
					dai_link->dai_fmt |= SND_SOC_DAIFMT_NB_NF;
				else if (!strcmp(fmt, "NBIF"))
					dai_link->dai_fmt |= SND_SOC_DAIFMT_NB_IF;
				else
					dev_err(&pdev->dev, "%s, not support this sample format\n", __func__);
			} else {
				dai_link->dai_fmt |= SND_SOC_DAIFMT_NB_IF;
			}
			/* DSP_A, DSP_B and PDM(TDM) use NB_IF format */

			if (!strcmp(role, "master"))
				dai_link->dai_fmt |= SND_SOC_DAIFMT_CBS_CFS;
			else if (!strcmp(role, "slave"))
				dai_link->dai_fmt |= SND_SOC_DAIFMT_CBM_CFM;
			else
				dev_err(&pdev->dev, "%s, not support this role\n", __func__);

			if (!strcmp(mode, "PDM"))
				tdm_slot_no = slot_no; /* tdm_slot_no is only valid when mode is TDM/PDM */
			else if (slot_no != 2)
				dev_err(&pdev->dev, "Wrong solt number setting in %s mode\n", mode);

			dev_info(&pdev->dev, "%s, set DAI fmt to 0x%08x\n", __func__, dai_link->dai_fmt);
			idx++;
		}

		platform_set_drvdata(pdev, card);
		return devm_snd_soc_register_card(&pdev->dev, card);
	}

	return 0;
}

static struct platform_driver cvitek_dummy_driver = {
	.driver = {
		.name = "cvitek-dummy",
		.pm = &snd_soc_pm_ops,
		.of_match_table = cvi_dummy_audio_match_ids,
	},
	.probe = cvitek_dummy_probe,
};

module_platform_driver(cvitek_dummy_driver);

MODULE_AUTHOR("rachel.jiang");
MODULE_DESCRIPTION("dummy driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:cvitek-dummy");
