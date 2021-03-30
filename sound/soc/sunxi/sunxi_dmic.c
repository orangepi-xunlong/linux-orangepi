/*
 * sound\soc\sunxi\sunxi-dmic.c
 * (C) Copyright 2014-2016
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * wolfgang huang <huangjinhui@allwinnerrecg.com>
 * huangxin <huangxin@allwinnertech.com>
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
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/regmap.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/dma/sunxi-dma.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include "sunxi_dma.h"
#include "sunxi_dmic.h"

#define	DRV_NAME	"sunxi-dmic"

struct dmic_rate {
	unsigned int samplerate;
	unsigned int rate_bit;
};

struct dmic_chmap {
	unsigned int chan;
	unsigned int chan_bit;
};

struct sunxi_dmic_info {
	struct regmap   *regmap;
	struct regulator *power_supply;
	struct clk *pllclk;
	struct clk *moduleclk;
	struct device *dev;
	struct snd_soc_dai_driver dai;
	struct sunxi_dma_params capture_dma_param;
	struct pinctrl *pinctrl;
	struct pinctrl_state  *pinstate;
	struct pinctrl_state  *pinstate_sleep;
	u32 chanmap;
};

static const struct dmic_rate dmic_rate_s[] = {
	{44100, 0x0},
	{48000, 0x0},
	{22050, 0x2},
	/* KNOT support */
	{24000, 0x2},
	{11025, 0x4},
	{12000, 0x4},
	{32000, 0x1},
	{16000, 0x3},
	{8000, 0x5},
};

/*
 * Configure DMA , Chan enable & Global enable
 */
static void sunxi_dmic_enable(struct sunxi_dmic_info *sunxi_dmic, int enable)
{
	if (enable) {
		regmap_update_bits(sunxi_dmic->regmap, SUNXI_DMIC_INTC,
					(1<<FIFO_DRQ_EN), (1<<FIFO_DRQ_EN));
		regmap_update_bits(sunxi_dmic->regmap, SUNXI_DMIC_EN,
		(0xFF<<DATA_CH_EN), ((sunxi_dmic->chanmap)<<DATA_CH_EN));
		regmap_update_bits(sunxi_dmic->regmap, SUNXI_DMIC_EN,
					(1<<GLOBE_EN), (1<<GLOBE_EN));
	} else {
		regmap_update_bits(sunxi_dmic->regmap, SUNXI_DMIC_EN,
					(1<<GLOBE_EN), (0<<GLOBE_EN));
		regmap_update_bits(sunxi_dmic->regmap, SUNXI_DMIC_EN,
					(0xFF<<DATA_CH_EN), (0<<DATA_CH_EN));
		regmap_update_bits(sunxi_dmic->regmap, SUNXI_DMIC_INTC,
					(1<<FIFO_DRQ_EN), (0<<FIFO_DRQ_EN));
	}
}

static void sunxi_dmic_init(struct sunxi_dmic_info *sunxi_dmic)
{
	regmap_write(sunxi_dmic->regmap,
		SUNXI_DMIC_CH_MAP, DMIC_CHANMAP_DEFAULT);

	regmap_update_bits(sunxi_dmic->regmap, SUNXI_DMIC_CTR,
			(7<<DMICDFEN), (5<<DMICDFEN));
	/* set the vol */
	regmap_write(sunxi_dmic->regmap,
			SUNXI_DMIC_DATA0_1_VOL, DMIC_DEFAULT_VOL);
	regmap_write(sunxi_dmic->regmap,
			SUNXI_DMIC_DATA2_3_VOL, DMIC_DEFAULT_VOL);
}

static int sunxi_dmic_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(dai);
	int i;

	/* sample resolution & sample fifo format */
	switch (params_format(params)) {
	case	SNDRV_PCM_FORMAT_S16_LE:
		regmap_update_bits(sunxi_dmic->regmap, SUNXI_DMIC_FIFO_CTR,
				(3<<SAMPLE_RESOLUTION), (2<<SAMPLE_RESOLUTION));
		break;
	case	SNDRV_PCM_FORMAT_S24_LE:
		regmap_update_bits(sunxi_dmic->regmap, SUNXI_DMIC_FIFO_CTR,
				(3<<SAMPLE_RESOLUTION), (1<<SAMPLE_RESOLUTION));
		break;
	default:
		dev_err(sunxi_dmic->dev, "Invalid format set\n");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(dmic_rate_s); i++) {
		if (dmic_rate_s[i].samplerate == params_rate(params)) {
			regmap_update_bits(sunxi_dmic->regmap, SUNXI_DMIC_CTR,
			(7<<DMIC_SR), (dmic_rate_s[i].rate_bit<<DMIC_SR));
			break;
		}
	}

	/* oversamplerate adjust */
	if (params_rate(params) >= 24000) {
		regmap_update_bits(sunxi_dmic->regmap, SUNXI_DMIC_CTR,
			(1<<DMIC_OVERSAMPLE_RATE), (1<<DMIC_OVERSAMPLE_RATE));
	} else {
		regmap_update_bits(sunxi_dmic->regmap, SUNXI_DMIC_CTR,
			(1<<DMIC_OVERSAMPLE_RATE), (0<<DMIC_OVERSAMPLE_RATE));
	}

	sunxi_dmic->chanmap = (1<<params_channels(params)) - 1;

	regmap_write(sunxi_dmic->regmap,
		SUNXI_DMIC_HPF_CTRL, sunxi_dmic->chanmap);

	/* DMIC num is M+1 */
	regmap_update_bits(sunxi_dmic->regmap, SUNXI_DMIC_CH_NUM,
		(7<<DMIC_CH_NUM), ((params_channels(params)-1)<<DMIC_CH_NUM));

	return 0;
}

static int sunxi_dmic_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(dai);

	switch (cmd) {
	case	SNDRV_PCM_TRIGGER_START:
	case	SNDRV_PCM_TRIGGER_RESUME:
	case	SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		sunxi_dmic_enable(sunxi_dmic, 1);
		break;
	case	SNDRV_PCM_TRIGGER_STOP:
	case	SNDRV_PCM_TRIGGER_SUSPEND:
	case	SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		sunxi_dmic_enable(sunxi_dmic, 0);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

/*
 * Reset & Flush FIFO
 */
static int sunxi_dmic_prepare(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(dai);

	regmap_write(sunxi_dmic->regmap, SUNXI_DMIC_INTS,
		(1<<FIFO_OVERRUN_IRQ_PENDING) | (1<<FIFO_DATA_IRQ_PENDING));

	regmap_write(sunxi_dmic->regmap, SUNXI_DMIC_CNT, 0x0);

	regmap_write(sunxi_dmic->regmap, SUNXI_DMIC_FIFO_STA, 0x0);

	regmap_update_bits(sunxi_dmic->regmap, SUNXI_DMIC_FIFO_CTR,
			(1<<DMIC_FIFO_FLUSH), (1<<DMIC_FIFO_FLUSH));

	return 0;
}

static int sunxi_dmic_startup(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(dai);

	snd_soc_dai_set_dma_data(dai, substream,
				&sunxi_dmic->capture_dma_param);

	return 0;
}

static int sunxi_dmic_set_sysclk(struct snd_soc_dai *dai, int clk_id,
						unsigned int freq, int dir)
{
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(dai);

	if (clk_set_rate(sunxi_dmic->pllclk, freq)) {
		dev_err(sunxi_dmic->dev, "Freq : %u not support\n", freq);
		return -EINVAL;
	}

	return 0;
}

/* Dmic module init status */
static int sunxi_dmic_probe(struct snd_soc_dai *dai)
{
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(dai);

	sunxi_dmic_init(sunxi_dmic);
	return 0;
}

static int sunxi_dmic_suspend(struct snd_soc_dai *cpu_dai)
{
	u32 ret = 0;
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(cpu_dai);
	pr_debug("[DMIC]Enter %s\n", __func__);

	if (NULL != sunxi_dmic->pinstate_sleep) {
		ret = pinctrl_select_state(sunxi_dmic->pinctrl, sunxi_dmic->pinstate_sleep);
		if (ret) {
			pr_warn("[dmic]select pin sleep state failed\n");
			return ret;
		}
	}
	if (sunxi_dmic->pinctrl !=NULL)
		devm_pinctrl_put(sunxi_dmic->pinctrl);
	sunxi_dmic->pinctrl = NULL;
	sunxi_dmic->pinstate = NULL;
	sunxi_dmic->pinstate_sleep = NULL;
	if (sunxi_dmic->moduleclk != NULL)
		clk_disable(sunxi_dmic->moduleclk);
	if (sunxi_dmic->pllclk != NULL)
		clk_disable(sunxi_dmic->pllclk);

	pr_debug("[DMIC]End %s\n", __func__);
	return 0;
}

static int sunxi_dmic_resume(struct snd_soc_dai *cpu_dai)
{
	s32 ret = 0;
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(cpu_dai);
	pr_debug("[DMIC]Enter %s\n", __func__);

	if (sunxi_dmic->pllclk != NULL) {
		if (clk_prepare_enable(sunxi_dmic->pllclk)) {
			pr_err("open sunxi_dmic->pllclk failed! line = %d\n", __LINE__);
		}
	}

	if (sunxi_dmic->moduleclk != NULL) {
		if (clk_prepare_enable(sunxi_dmic->moduleclk)) {
			pr_err("open sunxi_dmic->moduleclk failed! line = %d\n", __LINE__);
		}
	}

	if (!sunxi_dmic->pinctrl) {
		sunxi_dmic->pinctrl = devm_pinctrl_get(cpu_dai->dev);
		if (IS_ERR_OR_NULL(sunxi_dmic->pinctrl)) {
			pr_warn("[dmic]request pinctrl handle for audio failed\n");
			return -EINVAL;
		}
	}
	if (!sunxi_dmic->pinstate){
		sunxi_dmic->pinstate = pinctrl_lookup_state(sunxi_dmic->pinctrl, PINCTRL_STATE_DEFAULT);
		if (IS_ERR_OR_NULL(sunxi_dmic->pinstate)) {
			pr_warn("[dmic]lookup pin default state failed\n");
			return -EINVAL;
		}
	}

	if (!sunxi_dmic->pinstate_sleep){
		sunxi_dmic->pinstate_sleep = pinctrl_lookup_state(sunxi_dmic->pinctrl, PINCTRL_STATE_SLEEP);
		if (IS_ERR_OR_NULL(sunxi_dmic->pinstate_sleep)) {
			pr_warn("[dmic]lookup pin sleep state failed\n");
			return -EINVAL;
		}
	}

	ret = pinctrl_select_state(sunxi_dmic->pinctrl, sunxi_dmic->pinstate);
	if (ret) {
		pr_warn("[dmic]select pin default state failed\n");
		return ret;
	}

	sunxi_dmic_init(sunxi_dmic);
	pr_debug("[DMIC]End %s\n", __func__);
	return 0;
}

#define	SUNXI_DMIC_RATES (SNDRV_PCM_RATE_8000_48000 | SNDRV_PCM_RATE_KNOT)
static struct snd_soc_dai_ops sunxi_dmic_dai_ops = {
	.startup = sunxi_dmic_startup,
	.trigger = sunxi_dmic_trigger,
	.prepare = sunxi_dmic_prepare,
	.hw_params = sunxi_dmic_hw_params,
	.set_sysclk = sunxi_dmic_set_sysclk,
};

static struct snd_soc_dai_driver sunxi_dmic_dai = {
	.probe = sunxi_dmic_probe,
	.suspend = sunxi_dmic_suspend,
	.resume = sunxi_dmic_resume,
	.capture = {
		.channels_min = 1,
		.channels_max = 8,
		.rates = SUNXI_DMIC_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
	.ops = &sunxi_dmic_dai_ops,
};

static const struct snd_soc_component_driver sunxi_dmic_component = {
	.name = DRV_NAME,
};

static const struct regmap_config sunxi_dmic_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_DMIC_HPF_GAIN,
	.cache_type = REGCACHE_NONE,
};

static int  sunxi_dmic_dev_probe(struct platform_device *pdev)
{
	struct resource res, *memregion;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_dmic_info *sunxi_dmic;
	void __iomem *sunxi_dmic_membase = NULL;
	int ret;

	sunxi_dmic = devm_kzalloc(&pdev->dev, sizeof(struct sunxi_dmic_info), GFP_KERNEL);
	if (!sunxi_dmic) {
		dev_err(&pdev->dev, "sunxi_dmic allocate failed\n");
		ret = -ENOMEM;
		goto err_node_put;
	}
	dev_set_drvdata(&pdev->dev, sunxi_dmic);

	sunxi_dmic->dev = &pdev->dev;
	sunxi_dmic->dai = sunxi_dmic_dai;
	sunxi_dmic->dai.name = dev_name(&pdev->dev);

	sunxi_dmic->power_supply = regulator_get(&pdev->dev, "audio-33");
	if (IS_ERR(sunxi_dmic->power_supply)) {
		dev_err(&pdev->dev, "Failed to get sunxi dmic power supply\n");
		ret = -EINVAL;
		goto err_devm_kfree;
	} else {
		ret = regulator_set_voltage(sunxi_dmic->power_supply,
					3300000, 3300000);
		if (ret)
			dev_warn(&pdev->dev, "Failed to set sunxi dmic power supply to 3.3V\n");
		ret = regulator_enable(sunxi_dmic->power_supply);
		if (ret) {
			dev_err(&pdev->dev, "Failed to enable sunxi dmic power supply\n");
			ret = -EBUSY;
			goto err_regulator_put;
		}
	}

	ret = of_address_to_resource(np, 0 , &res);
	if (ret) {
		dev_err(&pdev->dev, "Failed to get sunxi dmic resource\n");
		ret = -EINVAL;
		goto err_regulator_put;
	}

	memregion = devm_request_mem_region(&pdev->dev, res.start,
					    resource_size(&res), DRV_NAME);
	if (!memregion) {
		dev_err(&pdev->dev, "sunxi dmic memory region already claimed\n");
		ret = -EBUSY;
		goto err_regulator_put;
	}

	sunxi_dmic_membase = devm_ioremap(&pdev->dev,
					res.start, resource_size(&res));
	if (!sunxi_dmic_membase) {
		dev_err(&pdev->dev, "sunxi dmic ioremap failed\n");
		ret = -ENOMEM;
		goto err_regulator_put;
	}

	sunxi_dmic->regmap = devm_regmap_init_mmio(&pdev->dev,
				sunxi_dmic_membase, &sunxi_dmic_regmap_config);
	if (IS_ERR_OR_NULL(sunxi_dmic->regmap)) {
		dev_err(&pdev->dev, "sunxi dmic registers regmap failed\n");
		ret = -ENOMEM;
		goto err_iounmap;
	}

	sunxi_dmic->pllclk = of_clk_get(np, 0);
	sunxi_dmic->moduleclk = of_clk_get(np, 1);
	if (IS_ERR(sunxi_dmic->pllclk) || IS_ERR(sunxi_dmic->moduleclk)){
		dev_err(&pdev->dev, "Can't get dmic pll clocks\n");
		if (IS_ERR(sunxi_dmic->pllclk)) {
			ret = PTR_ERR(sunxi_dmic->pllclk);
			goto err_iounmap;
		} else {
			ret = PTR_ERR(sunxi_dmic->moduleclk);
			goto err_pllclk_put;
		}
	} else {
		if (clk_set_parent(sunxi_dmic->moduleclk, sunxi_dmic->pllclk)) {
			pr_err("try to set parent of sunxi_dmic->moduleclk to sunxi_dmic->pllclk failed! line = %d\n",__LINE__);
		}
		clk_prepare_enable(sunxi_dmic->pllclk);
		clk_prepare_enable(sunxi_dmic->moduleclk);
	}

	/* FIXME */
	sunxi_dmic->capture_dma_param.dma_addr = res.start + SUNXI_DMIC_DATA;
	sunxi_dmic->capture_dma_param.dma_drq_type_num = DRQSRC_DMIC_RX;
	sunxi_dmic->capture_dma_param.src_maxburst = 8;
	sunxi_dmic->capture_dma_param.dst_maxburst = 8;

	sunxi_dmic->pinctrl = NULL;
	if (!sunxi_dmic->pinctrl) {
		sunxi_dmic->pinctrl = devm_pinctrl_get(&pdev->dev);
		if (IS_ERR_OR_NULL(sunxi_dmic->pinctrl)) {
			dev_err(&pdev->dev, "request pinctrl handle for audio failed\n");
			ret =  -EINVAL;
			goto err_moduleclk_put;
		}
	}
	if (!sunxi_dmic->pinstate){
		sunxi_dmic->pinstate = pinctrl_lookup_state(sunxi_dmic->pinctrl, PINCTRL_STATE_DEFAULT);
		if (IS_ERR_OR_NULL(sunxi_dmic->pinstate)) {
			dev_err(&pdev->dev, "lookup pin default state failed\n");
			ret = -EINVAL;
			goto err_pinctrl_put;
		}
	}

	if (!sunxi_dmic->pinstate_sleep){
		sunxi_dmic->pinstate_sleep = pinctrl_lookup_state(sunxi_dmic->pinctrl, PINCTRL_STATE_SLEEP);
		if (IS_ERR_OR_NULL(sunxi_dmic->pinstate_sleep)) {
			dev_err(&pdev->dev, "lookup pin sleep state failed\n");
			ret = -EINVAL;
			goto err_pinctrl_put;
		}
	}

	ret = snd_soc_register_component(&pdev->dev, &sunxi_dmic_component,
				   &sunxi_dmic->dai, 1);
	if (ret) {
		dev_err(&pdev->dev, "Could not register DAI: %d\n", ret);
		ret = -ENOMEM;
		goto err_pinctrl_put;
	}

	ret = asoc_dma_platform_register(&pdev->dev,0);
	if (ret) {
		dev_err(&pdev->dev, "Could not register PCM: %d\n", ret);
		ret = -ENOMEM;
		goto err_unregister_component;
	}

	return 0;

err_unregister_component:
	snd_soc_unregister_component(&pdev->dev);
err_pinctrl_put:
	devm_pinctrl_put(sunxi_dmic->pinctrl);
err_moduleclk_put:
	clk_put(sunxi_dmic->moduleclk);
err_pllclk_put:
	clk_put(sunxi_dmic->pllclk);
err_iounmap:
	iounmap(sunxi_dmic_membase);
err_regulator_put:
	regulator_put(sunxi_dmic->power_supply);
err_devm_kfree:
	devm_kfree(&pdev->dev, sunxi_dmic);
err_node_put:
	of_node_put(np);
	return ret;
}

static int __exit sunxi_dmic_dev_remove(struct platform_device *pdev)
{
	snd_soc_unregister_component(&pdev->dev);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static const struct of_device_id sunxi_dmic_of_match[] = {
	{ .compatible = "allwinner,sunxi-dmic", },
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_dmic_of_match);

static struct platform_driver sunxi_dmic_driver = {
	.probe = sunxi_dmic_dev_probe,
	.remove = __exit_p(sunxi_dmic_dev_remove),
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_dmic_of_match,
	},
};

module_platform_driver(sunxi_dmic_driver);

MODULE_AUTHOR("wolfgang huang <huangjinhui@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI DMIC Machine ASoC driver");
MODULE_ALIAS("platform:sunxi-dmic");
MODULE_LICENSE("GPL");
