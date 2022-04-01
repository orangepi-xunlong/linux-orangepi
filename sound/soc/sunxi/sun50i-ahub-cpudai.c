// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * (C) Copyright 2014-2018
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Wolfgang huang <huangjinhui@allwinnertech.com>
 *
 * (C) Copyright 2021
 * Shenzhen Xunlong Software Co., Ltd. <www.orangepi.org>
 * Leeboby <leeboby@aliyun.com>
 *
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/dmaengine_pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include "sun50i_ahub.h"

#define DRV_NAME "sunxi-ahub-cpudai"

struct sunxi_ahub_cpudai_priv {
	unsigned int id;
	int karaoke_mode;
	struct regmap *regmap;
	struct snd_dmaengine_dai_dma_data playback_dma_param;
	struct snd_dmaengine_dai_dma_data capture_dma_param;
};

static int startup_playback_cnt;
static int startup_capture_cnt;
static int karaoke_cnt;

int sunxi_ahub_cpudai_init(void)
{
	startup_playback_cnt = 0;
	startup_capture_cnt = 0;
	karaoke_cnt = 0;
	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_ahub_cpudai_init);


static int sunxi_ahub_i2s_playback_route_enable(
		struct sunxi_ahub_cpudai_priv *sunxi_ahub_cpudai)
{
	int i, reg_bit;
	unsigned int reg_val;

	/* first check it's on karaoke mode? */
	reg_val = sunxi_ahub_read(SUNXI_AHUB_DAM_RX0_SRC(0));
	if (!reg_val) {
		reg_val = sunxi_ahub_read(
				SUNXI_AHUB_DAM_RX1_SRC(0));
		if (!reg_val)
			goto no_karaook_handle;
	}
	for (i = 0; i < 4; i++) {
		reg_val = sunxi_ahub_read(
				SUNXI_AHUB_I2S_RXCONT(i));
		if (reg_val & (1<<19)) {
			/* I2S0...HDMI...I2S2...CVBS... */
			reg_bit = 23 - i;
			/* setting the rst & gating register for I2S module */
			sunxi_ahub_update_bits(
					SUNXI_AHUB_RST,
					(1<<reg_bit), (1<<reg_bit));
			sunxi_ahub_update_bits(
				SUNXI_AHUB_GAT,
				(1<<reg_bit), (1<<reg_bit));
			sunxi_ahub_update_bits(
				SUNXI_AHUB_I2S_CTL(i),
				(1<<I2S_CTL_TXEN), (1<<I2S_CTL_TXEN));
		}
	}
	sunxi_ahub_cpudai->karaoke_mode = 1;
	karaoke_cnt++;
	return 0;

no_karaook_handle:
	switch (sunxi_ahub_cpudai->id) {
	case 0:
		/* operation HDMI I2S module */
		sunxi_ahub_update_bits(SUNXI_AHUB_RST,
				(1<<I2S1_RST), (1<<I2S1_RST));
		sunxi_ahub_update_bits(SUNXI_AHUB_GAT,
				(1<<I2S1_GAT), (1<<I2S1_GAT));
		sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(1),
				(1<<I2S_CTL_TXEN), (1<<I2S_CTL_TXEN));

		sunxi_ahub_update_bits(SUNXI_AHUB_RST,
				(1<<I2S0_RST), (1<<I2S0_RST));
		sunxi_ahub_update_bits(SUNXI_AHUB_GAT,
				(1<<I2S0_GAT), (1<<I2S0_GAT));
		sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(0),
				(1<<I2S_CTL_TXEN), (1<<I2S_CTL_TXEN));
		break;
	case 1:
		/* operation CVBS module */
		sunxi_ahub_update_bits(SUNXI_AHUB_RST,
				(1<<I2S3_RST), (1<<I2S3_RST));
		sunxi_ahub_update_bits(SUNXI_AHUB_GAT,
				(1<<I2S3_GAT), (1<<I2S3_GAT));
		sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(3),
				(1<<I2S_CTL_TXEN), (1<<I2S_CTL_TXEN));
		break;
	case 2:
		/* operation I2S2 for bluetooth */
		sunxi_ahub_update_bits(SUNXI_AHUB_RST,
				(1<<I2S2_RST), (1<<I2S2_RST));
		sunxi_ahub_update_bits(SUNXI_AHUB_GAT,
				(1<<I2S2_GAT), (1<<I2S2_GAT));
		sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(2),
				(1<<I2S_CTL_TXEN), (1<<I2S_CTL_TXEN));
		break;
	default:
		break;
	}
	sunxi_ahub_cpudai->karaoke_mode = 0;
	
	return 0;
}

static int sunxi_ahub_i2s_playback_route_disable(
		struct sunxi_ahub_cpudai_priv *sunxi_ahub_cpudai)
{
	unsigned int reg_val;
	int i;

 	if (sunxi_ahub_cpudai->karaoke_mode) {
		if (--karaoke_cnt == 0) {
			for (i = 0; i < 4; i++) {
				reg_val = sunxi_ahub_read(
					SUNXI_AHUB_I2S_CTL(i));
				if (reg_val & (1 << I2S_CTL_TXEN))
					/* I2S0...HDMI...I2S2...CVBS... */
					sunxi_ahub_update_bits(
						SUNXI_AHUB_I2S_CTL(i),
						(1<<I2S_CTL_TXEN),
						(0<<I2S_CTL_TXEN));
			}
		}
	} else {
		switch (sunxi_ahub_cpudai->id) {
		case 0:
			/* operation HDMI I2S module */
			sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(1),
					(1<<I2S_CTL_TXEN), (0<<I2S_CTL_TXEN));
			sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(0),
					(1<<I2S_CTL_TXEN), (0<<I2S_CTL_TXEN));
			break;
		case 1:
			/* operation CVBS module */
			sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(3),
					(1<<I2S_CTL_TXEN), (0<<I2S_CTL_TXEN));
			break;
		case 2:
			/* operation I2S2 for bluetooth */
			sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(2),
					(1<<I2S_CTL_TXEN), (0<<I2S_CTL_TXEN));
			break;
		default:
			break;
		}
	}
	return 0;
}


static int sunxi_ahub_i2s_capture_route_enable(
		struct sunxi_ahub_cpudai_priv *sunxi_ahub_cpudai)
{
	int i2s0_cap_bit = 27;
	/* HDMI just for support debug */
	int i2s1_cap_bit = 26;
	int i2s2_cap_bit = 25;
	int i2s3_cap_bit = 23;
	unsigned int reg_val;

	reg_val = sunxi_ahub_read(
		SUNXI_AHUB_APBIF_RXFIFO_CONT(sunxi_ahub_cpudai->id));
	if (reg_val == (1<<i2s0_cap_bit)) {
		sunxi_ahub_update_bits(SUNXI_AHUB_RST,
			(1<<I2S0_RST), (1<<I2S0_RST));
		sunxi_ahub_update_bits(SUNXI_AHUB_GAT,
			(1<<I2S0_GAT), (1<<I2S0_GAT));
		sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(0),
			(1<<I2S_CTL_RXEN), (1<<I2S_CTL_RXEN));		
		sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(0),
				(1 << I2S_CTL_SDI0_EN), ( 1<< I2S_CTL_SDI0_EN));
		/* add the delay to anti pop noise when it start capture */
		mdelay(100);		
	}

	if (reg_val == (1 << i2s1_cap_bit)) {
		sunxi_ahub_update_bits(SUNXI_AHUB_RST,
			(1 << I2S1_RST), (1 << I2S1_RST));
		sunxi_ahub_update_bits(SUNXI_AHUB_GAT,
			(1 << I2S1_GAT), (1 << I2S1_GAT));
		sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(1),
			(1 << I2S_CTL_RXEN), (1 << I2S_CTL_RXEN));
	}

	if (reg_val == (1<<i2s2_cap_bit)) {
		sunxi_ahub_update_bits(SUNXI_AHUB_RST,
			(1<<I2S2_RST), (1<<I2S2_RST));
		sunxi_ahub_update_bits(SUNXI_AHUB_GAT,
			(1<<I2S2_GAT), (1<<I2S2_GAT));
		sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(2),
			(1<<I2S_CTL_RXEN), (1<<I2S_CTL_RXEN));
		sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(2),
			(1<<I2S_CTL_SDI0_EN), (1<<I2S_CTL_SDI0_EN));
		/* add the delay to anti pop noise when it start capture */
		mdelay(100);
	}

	if (reg_val == (1<<i2s3_cap_bit)) {
		sunxi_ahub_update_bits(SUNXI_AHUB_RST,
			(1<<I2S3_RST), (1<<I2S3_RST));
		sunxi_ahub_update_bits(SUNXI_AHUB_GAT,
			(1<<I2S3_GAT), (1<<I2S3_GAT));
		sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(3),
			(1<<I2S_CTL_RXEN), (1<<I2S_CTL_RXEN));
		sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(3),
			(1<<I2S_CTL_SDI0_EN), (1<<I2S_CTL_SDI0_EN));
		/* add the delay to anti pop noise when it start capture */
		mdelay(100);
	}

	return 0;
}

static int sunxi_ahub_i2s_capture_route_disable(
		struct sunxi_ahub_cpudai_priv *sunxi_ahub_cpudai)
{
	int i2s0_cap_bit = 27;
	/* HDMI just for support debug */
	int i2s1_cap_bit = 26;
	int i2s2_cap_bit = 25;
	int i2s3_cap_bit = 23;
	unsigned int reg_val;

	reg_val = sunxi_ahub_read(
		SUNXI_AHUB_APBIF_RXFIFO_CONT(sunxi_ahub_cpudai->id));
	if (reg_val == (1<<i2s0_cap_bit)) {
		sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(0),
			(1<<I2S_CTL_RXEN), (0<<I2S_CTL_RXEN));		
		sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(0),
				(1<<I2S_CTL_SDI0_EN), (0<<I2S_CTL_SDI0_EN));
	}

	if (reg_val == (1 << i2s1_cap_bit)) {
		sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(1),
			(1 << I2S_CTL_RXEN), (0 << I2S_CTL_RXEN));
	}

	if (reg_val == (1<<i2s2_cap_bit)) {
		sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(2),
			(1<<I2S_CTL_RXEN), (0<<I2S_CTL_RXEN));
		sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(2),
				(1<<I2S_CTL_SDI0_EN), (0<<I2S_CTL_SDI0_EN));
	}

	if (reg_val == (1<<i2s3_cap_bit)) {
		sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(3),
			(1<<I2S_CTL_RXEN), (0<<I2S_CTL_RXEN));
		sunxi_ahub_update_bits(SUNXI_AHUB_I2S_CTL(3),
				(1<<I2S_CTL_SDI0_EN), (0<<I2S_CTL_SDI0_EN));
	}
	
	return 0;
}

static int sunxi_ahub_cpudai_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct sunxi_ahub_cpudai_priv *sunxi_ahub_cpudai =
					snd_soc_dai_get_drvdata(dai);

 	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		snd_soc_dai_set_dma_data(dai, substream,
					&sunxi_ahub_cpudai->playback_dma_param);
	else
		snd_soc_dai_set_dma_data(dai, substream,
					&sunxi_ahub_cpudai->capture_dma_param);

	return 0;
}

static int sunxi_ahub_cpudai_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	struct sunxi_ahub_cpudai_priv *sunxi_ahub_cpudai =
					snd_soc_dai_get_drvdata(dai);

 	switch (cmd) {
	case	SNDRV_PCM_TRIGGER_START:
	case	SNDRV_PCM_TRIGGER_RESUME:
	case	SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (startup_playback_cnt++ == 0)
				sunxi_ahub_i2s_playback_route_enable(
						sunxi_ahub_cpudai);
			mdelay(1);
			sunxi_ahub_update_bits(
				SUNXI_AHUB_APBIF_TX_CTL(sunxi_ahub_cpudai->id),
				(1<<APBIF_TX_START), (1<<APBIF_TX_START));
			sunxi_ahub_update_bits(
				SUNXI_AHUB_APBIF_TX_IRQ_CTL(
					sunxi_ahub_cpudai->id),
				(1<<APBIF_TX_DRQ), (1<<APBIF_TX_DRQ));
			if (startup_playback_cnt++ > 1)
				sunxi_ahub_i2s_playback_route_enable(
						sunxi_ahub_cpudai);
		} else {
			if (startup_capture_cnt++ == 0) {
				sunxi_ahub_update_bits(
					SUNXI_AHUB_APBIF_RX_CTL(
					sunxi_ahub_cpudai->id),
					(1<<APBIF_RX_START),
					(1<<APBIF_RX_START));
				sunxi_ahub_update_bits(
					SUNXI_AHUB_APBIF_RX_IRQ_CTL(
					sunxi_ahub_cpudai->id),
					(1<<APBIF_RX_DRQ),
					(1<<APBIF_RX_DRQ));
			}
			if (startup_capture_cnt++ > 0)
				sunxi_ahub_i2s_capture_route_enable(
						sunxi_ahub_cpudai);
		}
		break;
	case	SNDRV_PCM_TRIGGER_STOP:
	case	SNDRV_PCM_TRIGGER_SUSPEND:
	case	SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			sunxi_ahub_i2s_playback_route_disable(
				sunxi_ahub_cpudai);
			sunxi_ahub_update_bits(
				SUNXI_AHUB_APBIF_TX_IRQ_CTL(
				sunxi_ahub_cpudai->id),
				(1<<APBIF_TX_DRQ), (0<<APBIF_TX_DRQ));
			mdelay(2);
			sunxi_ahub_update_bits(
				SUNXI_AHUB_APBIF_TX_CTL(sunxi_ahub_cpudai->id),
				(1<<APBIF_TX_START), (0<<APBIF_TX_START));
		} else {
			sunxi_ahub_i2s_capture_route_disable(
					sunxi_ahub_cpudai);
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static struct snd_soc_dai_ops sunxi_ahub_cpudai_dai_ops = {
	.startup = sunxi_ahub_cpudai_startup,
	.trigger = sunxi_ahub_cpudai_trigger,
};

static int sunxi_ahub_dai_probe(struct snd_soc_dai *dai)
{
        struct sunxi_ahub_cpudai_priv *sunxi_ahub_cpudai =
                                        snd_soc_dai_get_drvdata(dai);

        snd_soc_dai_init_dma_data(dai,
                                  &sunxi_ahub_cpudai->playback_dma_param,
                                  &sunxi_ahub_cpudai->capture_dma_param);

        return 0;
}

static struct snd_soc_dai_driver sunxi_ahub_cpudai_dai = {
	.probe = sunxi_ahub_dai_probe,
	.playback = {
		.channels_min = 1,
		.channels_max = 16,
		.rates = SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT,
		.rate_min       = 8000,
		.rate_max       = 192000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.channels_min = 1,
		.channels_max = 16,
		.rates = SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT,
		.rate_min       = 8000,
		.rate_max       = 192000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops	= &sunxi_ahub_cpudai_dai_ops,

};

static const struct snd_soc_component_driver sunxi_ahub_cpudai_component = {
	.name		= DRV_NAME,
};
	
static const struct of_device_id sunxi_ahub_cpudai_of_match[] = {
	{ .compatible = "allwinner,sunxi-ahub-cpudai", },
	{},
};

static int  sunxi_ahub_cpudai_dev_probe(struct platform_device *pdev)
{
	struct resource res;	

	struct sunxi_ahub_cpudai_priv *sunxi_ahub_cpudai;
	struct device_node *np = pdev->dev.of_node;
	unsigned int temp_val;
	int ret;

 	sunxi_ahub_cpudai = devm_kzalloc(&pdev->dev,
			sizeof(struct sunxi_ahub_cpudai_priv), GFP_KERNEL);
	if (!sunxi_ahub_cpudai) {
		dev_err(&pdev->dev, "Can't allocate sunxi_cpudai\n");
		ret = -ENOMEM;
		goto err_node_put;
	}
	
	dev_set_drvdata(&pdev->dev, sunxi_ahub_cpudai);

	ret = of_property_read_u32(np, "id", &temp_val);
	if (ret < 0) {
		dev_err(&pdev->dev, "id configuration missing or invalid\n");
		goto err_devm_kfree;
	} else {
		sunxi_ahub_cpudai->id = temp_val;
	}

	ret = of_address_to_resource(np, 0, &res);
	if (ret) {
		dev_err(&pdev->dev, "Can't parse device node resource\n");
		ret = -ENODEV;
		goto err_devm_kfree;
	}

	sunxi_ahub_cpudai->playback_dma_param.addr =
		res.start + SUNXI_AHUB_APBIF_TXFIFO(sunxi_ahub_cpudai->id);
	sunxi_ahub_cpudai->playback_dma_param.maxburst = 8;
	sunxi_ahub_cpudai->playback_dma_param.addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;

	sunxi_ahub_cpudai->capture_dma_param.addr =
		res.start + SUNXI_AHUB_APBIF_RXFIFO(sunxi_ahub_cpudai->id);
	sunxi_ahub_cpudai->capture_dma_param.maxburst = 8;
	sunxi_ahub_cpudai->capture_dma_param.addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;

	sunxi_ahub_cpudai->regmap = sunxi_ahub_regmap_init(pdev);
	if (!sunxi_ahub_cpudai->regmap) {
		dev_err(&pdev->dev, "regmap not init ok\n");
		ret = -ENOMEM;
		goto err_devm_kfree;
	}
	sunxi_ahub_cpudai_init();

	ret = snd_soc_register_component(&pdev->dev,
			&sunxi_ahub_cpudai_component,
			&sunxi_ahub_cpudai_dai, 1);
	if (ret) {
		dev_err(&pdev->dev, "Could not register DAI: %d\n", ret);
		ret = -EBUSY;
		goto err_devm_kfree;
	}

	ret = devm_snd_dmaengine_pcm_register(&pdev->dev, NULL, 0);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register against DMAEngine\n");
		goto err_unregister_component;
	}

	return 0;

err_unregister_component:
	snd_soc_unregister_component(&pdev->dev);
err_devm_kfree:
	devm_kfree(&pdev->dev, sunxi_ahub_cpudai);
err_node_put:
	of_node_put(np);

	return ret;
}

static int __exit sunxi_ahub_cpudai_dev_remove(struct platform_device *pdev)
{
	struct sunxi_ahub_cpudai_priv *sunxi_ahub_cpudai =
					dev_get_drvdata(&pdev->dev);

	snd_soc_unregister_component(&pdev->dev);
	devm_kfree(&pdev->dev, sunxi_ahub_cpudai);

	return 0;
}

static struct platform_driver sunxi_ahub_cpudai_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_ahub_cpudai_of_match,
	},
	.probe = sunxi_ahub_cpudai_dev_probe,
	.remove = __exit_p(sunxi_ahub_cpudai_dev_remove),
};

module_platform_driver(sunxi_ahub_cpudai_driver);

MODULE_DESCRIPTION("SUNXI Audio Hub cpudai ASoC Interface");
MODULE_AUTHOR("wolfgang huang <huangjinhui@allwinnertech.com>");
MODULE_AUTHOR("Leeboby <leeboby@aliyun.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);
