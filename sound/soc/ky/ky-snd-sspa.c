// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 KY
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/pm_runtime.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/pxa2xx-lib.h>
#include <sound/dmaengine_pcm.h>
#include <linux/suspend.h>
#include "ky-snd-sspa.h"

#include <linux/notifier.h>
#include <soc/ky/ky_panel.h>


#define KY_HDMI_BASE_ADDR        0xC0400500
#define KY_HDMI_REG_SIZE         0x200


#define KY_HDMI_PHY_STATUS       0xC
#define KY_HDMI_HPD_STATUS       BIT(12)
#define KY_HDMI_AUDIO_EN         0x30

#define HDMI_HPD_CONNECTED             0
#define HDMI_HPD_DISCONNECTED          1

extern int ky_hdmi_register_client(struct notifier_block *nb);

struct sspa_priv {
	struct ssp_device *sspa;
	struct snd_dmaengine_dai_dma_data *dma_params;
	struct reset_control *rst;
	int dai_fmt;
	int dai_id_pre;
	int running_cnt;
	void __iomem	*base;
	void __iomem	*base_clk;
	void __iomem	*base_hdmi;
	struct delayed_work hdmi_detect_work;
};

struct platform_device *sspa_platdev;

static int mmp_sspa_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	u32 value = 0;
	struct sspa_priv *sspa_priv = snd_soc_dai_get_drvdata(dai);

	value = readl_relaxed(sspa_priv->base_hdmi + KY_HDMI_AUDIO_EN);
	value |= BIT(0);
	writel(value, sspa_priv->base_hdmi + KY_HDMI_AUDIO_EN);

	pm_runtime_get_sync(&sspa_platdev->dev);
	return 0;
}

static void mmp_sspa_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	u32 value = 0;
	struct sspa_priv *sspa_priv = snd_soc_dai_get_drvdata(dai);

	value = readl_relaxed(sspa_priv->base_hdmi + KY_HDMI_AUDIO_EN);
	value &= ~BIT(0);
	writel(value, sspa_priv->base_hdmi + KY_HDMI_AUDIO_EN);

	pm_runtime_put_sync(&sspa_platdev->dev);
}

static int mmp_sspa_set_dai_sysclk(struct snd_soc_dai *cpu_dai,
				    int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int mmp_sspa_set_dai_pll(struct snd_soc_dai *cpu_dai, int pll_id,
				 int source, unsigned int freq_in,
				 unsigned int freq_out)
{
	return 0;
}

/*
 * Set up the sspa dai format. The sspa port must be inactive
 * before calling this function as the physical
 * interface format is changed.
 */
static int mmp_sspa_set_dai_fmt(struct snd_soc_dai *cpu_dai,
				 unsigned int fmt)
{
	return 0;
}

/*
 * Set the SSPA audio DMA parameters and sample size.
 * Can be called multiple times.
 */
static int mmp_sspa_hw_params(struct snd_pcm_substream *substream,
			       struct snd_pcm_hw_params *params,
			       struct snd_soc_dai *dai)
{
	struct sspa_priv *sspa_priv = snd_soc_dai_get_drvdata(dai);
	struct snd_dmaengine_dai_dma_data *dma_params;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		pr_debug("%s, format=0x%x\n", __FUNCTION__, params_format(params));
		dma_params = sspa_priv->dma_params;
		dma_params->maxburst = 32;
		dma_params->addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		snd_soc_dai_set_dma_data(dai, substream, dma_params);
	}
	return 0;
}

static int mmp_sspa_trigger(struct snd_pcm_substream *substream, int cmd,
			     struct snd_soc_dai *dai)
{
	struct sspa_priv *sspa_priv = snd_soc_dai_get_drvdata(dai);
	int ret = 0;

	pr_debug("%s cmd=%d, cnt=%d\n", __FUNCTION__, cmd, sspa_priv->running_cnt);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		sspa_priv->running_cnt++;
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (sspa_priv->running_cnt > 0)
			sspa_priv->running_cnt--;
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static int mmp_sspa_probe(struct snd_soc_dai *dai)
{
	struct sspa_priv *sspa_priv = dev_get_drvdata(dai->dev);
	pr_debug("%s\n", __FUNCTION__);

	snd_soc_dai_set_drvdata(dai, sspa_priv);

	queue_delayed_work(system_wq, &sspa_priv->hdmi_detect_work,
				   msecs_to_jiffies(100));

	return 0;
}

static const struct snd_soc_dai_ops mmp_sspa_dai_ops = {
	.probe		= mmp_sspa_probe,
	.startup	= mmp_sspa_startup,
	.shutdown	= mmp_sspa_shutdown,
	.trigger	= mmp_sspa_trigger,
	.hw_params	= mmp_sspa_hw_params,
	.set_sysclk	= mmp_sspa_set_dai_sysclk,
	.set_pll	= mmp_sspa_set_dai_pll,
	.set_fmt	= mmp_sspa_set_dai_fmt,
};

#define KY_SND_SSPA_RATES SNDRV_PCM_RATE_48000
#define KY_SND_SSPA_FORMATS SNDRV_PCM_FMTBIT_S16_LE

static struct snd_soc_dai_driver ky_snd_sspa_dai[] = {
	{
		.name = "SSPA2",
		.playback = {
			.stream_name = "SSPA2 TX",
			.channels_min = 2,
			.channels_max = 2,
			.rates = KY_SND_SSPA_RATES,
			.formats = KY_SND_SSPA_FORMATS,
		},
		.ops = &mmp_sspa_dai_ops,
	},
};

static void ky_dma_params_init(struct resource *res, struct snd_dmaengine_dai_dma_data *dma_params)
{
	dma_params->addr = res->start + 0x80;
	dma_params->maxburst = 32;
	dma_params->addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
}

static const struct snd_soc_component_driver ky_snd_sspa_component = {
	.name		= "ky-snd-sspa",
};

static int ky_sspa_get_hdmi_status(void)
{
	struct sspa_priv *priv = dev_get_drvdata(&sspa_platdev->dev);
	u32 value = 0;

	value = readl_relaxed(priv->base_hdmi + KY_HDMI_PHY_STATUS) & KY_HDMI_HPD_STATUS;
	pr_debug("%s status:%d\n", __func__, !!value);
	return !!value;
}

int ky_hdmi_connect_event(struct notifier_block *nb, unsigned long event,
	void *v)
{
	int ret;

	switch(event) {
	case HDMI_HPD_CONNECTED:
		pr_debug("sspa got the chain event: HDMI_HPD_CONNECTED\n");
		ret = devm_snd_soc_register_component(&sspa_platdev->dev, &ky_snd_sspa_component,
			ky_snd_sspa_dai, ARRAY_SIZE(ky_snd_sspa_dai));
		if (ret != 0) {
			dev_err(&sspa_platdev->dev, "failed to register DAI\n");
		}
		break;
	case HDMI_HPD_DISCONNECTED:
		pr_debug("sspa got the chain event: HDMI_HPD_DISCONNECTED\n");
		snd_soc_unregister_component(&sspa_platdev->dev);
		break;
	default:
		break;
	}
	return NOTIFY_DONE;
}

static struct notifier_block hdmi_connect_notifier = {
	.notifier_call = ky_hdmi_connect_event,
};

static void ky_sspa_hdmi_detect_handler(struct work_struct *work)
{
	if (!ky_sspa_get_hdmi_status()) {
		udelay(100);
		pr_debug("%s, hdmi disconnected \n", __func__);
		snd_soc_unregister_component(&sspa_platdev->dev);
	}
}

static int ky_sspa_suspend(struct device *dev)
{
	struct sspa_priv *priv = dev_get_drvdata(dev);

	reset_control_assert(priv->rst);

	return 0;
}

static int ky_sspa_resume(struct device *dev)
{
	struct sspa_priv *priv = dev_get_drvdata(dev);
	u32 value = 0;
	int ret = 0;

	value = readl_relaxed(priv->base_hdmi + KY_HDMI_AUDIO_EN);
	value |= BIT(0);
	writel(value, priv->base_hdmi + KY_HDMI_AUDIO_EN);
	reset_control_deassert(priv->rst);

	if (ky_sspa_get_hdmi_status()) {
		ret = devm_snd_soc_register_component(dev, &ky_snd_sspa_component,
			ky_snd_sspa_dai, ARRAY_SIZE(ky_snd_sspa_dai));
		if (ret != 0) {
			dev_err(dev, "failed to register DAI\n");
		}
	}

	return 0;
}

const struct dev_pm_ops ky_snd_sspa_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ky_sspa_suspend, ky_sspa_resume)
};

static int sspa_pm_suspend_notifier(struct notifier_block *nb,
				unsigned long event,
				void *dummy)
{
	int ret;

	switch (event) {
	case PM_SUSPEND_PREPARE:
	case PM_HIBERNATION_PREPARE:
		ky_hdmi_unregister_client(&hdmi_connect_notifier);
		snd_soc_unregister_component(&sspa_platdev->dev);
		return NOTIFY_DONE;

	case PM_POST_SUSPEND:
	case PM_POST_HIBERNATION:
		if (ky_sspa_get_hdmi_status()) {
			ret = devm_snd_soc_register_component(&sspa_platdev->dev, &ky_snd_sspa_component,
				ky_snd_sspa_dai, ARRAY_SIZE(ky_snd_sspa_dai));
			if (ret != 0) {
				dev_err(&sspa_platdev->dev, "failed to register DAI\n");
			}
		} else {
			snd_soc_unregister_component(&sspa_platdev->dev);
		}

		ky_hdmi_register_client(&hdmi_connect_notifier);
		return NOTIFY_OK;

	default:
		return NOTIFY_DONE;
	}
}

static struct notifier_block sspa_pm_notif_block = {
	.notifier_call = sspa_pm_suspend_notifier,
};

static int ky_snd_sspa_pdev_probe(struct platform_device *pdev)
{
	int ret;
	struct sspa_priv *priv;
	struct resource *base_res;
	struct resource *clk_res;

	pr_info("enter %s\n", __FUNCTION__);
	priv = devm_kzalloc(&pdev->dev,
				sizeof(struct sspa_priv), GFP_KERNEL);
	if (!priv) {
		pr_err("%s priv alloc failed\n", __FUNCTION__);
		return -ENOMEM;
	}
	base_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->base = devm_ioremap_resource(&pdev->dev, base_res);
	if (IS_ERR(priv->base)) {
		pr_err("%s reg base alloc failed\n", __FUNCTION__);
		return PTR_ERR(priv->base);
	}
	clk_res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	priv->base_clk = devm_ioremap_resource(&pdev->dev, clk_res);
	if (IS_ERR(priv->base_clk)) {
		pr_err("%s reg clk base alloc failed\n", __FUNCTION__);
		return PTR_ERR(priv->base_clk);
	}
	priv->base_hdmi = (void __iomem *)ioremap(KY_HDMI_BASE_ADDR, KY_HDMI_REG_SIZE);
	if (IS_ERR(priv->base_hdmi)) {
		pr_err("%s reg hdmi base alloc failed\n", __FUNCTION__);
		return PTR_ERR(priv->base_hdmi);
	}
	priv->dma_params = devm_kzalloc(&pdev->dev, sizeof(struct snd_dmaengine_dai_dma_data),
			GFP_KERNEL);
	if (priv->dma_params == NULL) {
		pr_err("%s dma_params alloc failed\n", __FUNCTION__);
		return -ENOMEM;
	}
	ky_dma_params_init(base_res, priv->dma_params);

	//get reset
	priv->rst = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(priv->rst))
		return PTR_ERR(priv->rst);

	reset_control_deassert(priv->rst);

	pm_runtime_enable(&pdev->dev);

	sspa_platdev = pdev;
	INIT_DELAYED_WORK(&priv->hdmi_detect_work,
			  ky_sspa_hdmi_detect_handler);
	platform_set_drvdata(pdev, priv);

	ret = devm_snd_soc_register_component(&pdev->dev, &ky_snd_sspa_component,
						   ky_snd_sspa_dai, ARRAY_SIZE(ky_snd_sspa_dai));
	if (ret != 0) {
		dev_err(&pdev->dev, "failed to register DAI\n");
		return ret;
	}

	ky_hdmi_register_client(&hdmi_connect_notifier);
	ret = register_pm_notifier(&sspa_pm_notif_block);

	return 0;
}

static int ky_snd_sspa_pdev_remove(struct platform_device *pdev)
{
	struct sspa_priv *priv = platform_get_drvdata(pdev);

	pm_runtime_disable(&pdev->dev);
	reset_control_assert(priv->rst);
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id ky_snd_sspa_ids[] = {
	{ .compatible = "ky,ky-snd-sspa", },
	{ /* sentinel */ }
};
#endif

static struct platform_driver ky_snd_sspa_pdrv = {
	.driver = {
		.name = "ky-snd-sspa",
		.pm = &ky_snd_sspa_pm_ops,
		.of_match_table = of_match_ptr(ky_snd_sspa_ids),
	},
	.probe = ky_snd_sspa_pdev_probe,
	.remove = ky_snd_sspa_pdev_remove,
};

static void __exit ky_snd_sspa_exit(void)
{
	platform_driver_unregister(&ky_snd_sspa_pdrv);
}
module_exit(ky_snd_sspa_exit);

static int ky_snd_sspa_init(void)
{
	return platform_driver_register(&ky_snd_sspa_pdrv);
}
late_initcall(ky_snd_sspa_init);


MODULE_DESCRIPTION("KY ASoC SSPA Driver");
MODULE_LICENSE("GPL");

