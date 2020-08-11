/*
 * sound\soc\sunxi\sunxi-mad.c
 * (C) Copyright 2018-2023
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * wolfgang <qinzhenying@allwinnerrecg.com>
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
#include <linux/pm.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/regmap.h>
#include <linux/dma/sunxi-dma.h>
#include "sunxi-mad.h"

#define	DRV_NAME	"sunxi-mad"

struct sunxi_mad_info {
	struct device *dev;
	struct clk *mad_clk;
	struct clk *mad_cfg_clk;
	struct clk *mad_ad_clk;
	struct clk *lpsd_clk;
	struct clk *pll_clk;
	struct clk *hosc_clk;
	unsigned int pll_audio_src_used;
	unsigned int hosc_src_used;
};

struct regmap *sunxi_mad_regmap;

/*memory mapping*/
#define MAD_SRAM_DMA_SRC_ADDR 0x05480000
/*256kbytes*/
#define MAD_SRAM_SIZE_VALUE 0x100
/*from cpu's ASR algorithm*/
#define MAD_SRAM_STORE_TH_VALUE 0X0

void sunxi_lpsd_init(void)
{
    /*config sunxi_mad_sram_wake_back_data_reg*/
    regmap_write(sunxi_mad_regmap, SUNXI_MAD_SRAM_WAKE_BACK_DATA, 0x5);

    /*config sunxi_mad_lpsd_ad_sync_reg*/
    regmap_write(sunxi_mad_regmap, SUNXI_MAD_LPSD_AD_SYNC_FC, 0x20);

    /*config sunxi_mad_lpsd_th_reg*/
    regmap_write(sunxi_mad_regmap, SUNXI_MAD_LPSD_TH, 0x4b0);

    /*config sunxi_mad_lpsd_rrun_reg*/
    regmap_write(sunxi_mad_regmap, SUNXI_MAD_LPSD_RRUN, 0x91);

    /*config sunxi_mad_lpsd_rstop_reg*/
    regmap_write(sunxi_mad_regmap, SUNXI_MAD_LPSD_RSTOP, 0xaa);

    /*config sunxi_mad_lpsd_ecnt_reg*/
    regmap_write(sunxi_mad_regmap, SUNXI_MAD_LPSD_ECNT, 0x32);
}

void sunxi_mad_sram_init(void)
{
    /*config sunxi_mad_sram_point_reg*/
    regmap_write(sunxi_mad_regmap, SUNXI_MAD_SRAM_POINT, 0x00);

   /*config sunxi_mad_sram_size_reg*/
    regmap_write(sunxi_mad_regmap, SUNXI_MAD_SRAM_SIZE, MAD_SRAM_SIZE_VALUE);

    /*config sunxi_mad_sram_store_th_reg*/
    regmap_write(sunxi_mad_regmap, SUNXI_MAD_SRAM_STORE_TH, MAD_SRAM_STORE_TH_VALUE);

    /*config sunxi_mad_sram_sec_region_reg, non-sec*/
    regmap_write(sunxi_mad_regmap, SUNXI_MAD_SRAM_SEC_REGION_REG, 0x0);

    /*config sunxi_mad_sram_size_reg*/
    regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_CTRL, 1<<SRAM_RST, 1<<SRAM_RST);
    regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_CTRL, 1<<SRAM_RST, 0<<SRAM_RST);
}

void sunxi_mad_init(void)
{
	sunxi_lpsd_init();
	sunxi_mad_sram_init();
}

EXPORT_SYMBOL_GPL(sunxi_mad_init);

void sunxi_sram_dma_config(struct sunxi_dma_params *capture_dma_param)
{
	u8 src_maxburst;

    /*config sunxi_mad_ahb1_rx_th_reg*/
    src_maxburst = capture_dma_param->src_maxburst;
    regmap_write(sunxi_mad_regmap, SUNXI_MAD_SRAM_AHB1_RX_TH,
	    0x40<<MAD_SRAM_AHB1_RX_TH);

    regmap_write(sunxi_mad_regmap, SUNXI_MAD_SRAM_AHB1_TX_TH,
	    0x40<<MAD_SRAM_AHB1_TX_TH);
    /*config sunxi_mad_sram as IO*/
    regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_CTRL,
	    1<<DMA_TYPE, 1<<DMA_TYPE);

    capture_dma_param->dma_addr = MAD_SRAM_DMA_SRC_ADDR;
    capture_dma_param->dma_drq_type_num = DRQSRC_MAD_RX;

    /*Enable sram dma*/
	/*regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_CTRL,
		1<<DMA_EN, 1<<DMA_EN);*/

	/*regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_CTRL,
		1<<MAD_EN, 1<<MAD_EN);*/
}
EXPORT_SYMBOL_GPL(sunxi_sram_dma_config);

void mad_dma_en(unsigned int en)
{
    regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_CTRL,
	    1<<DMA_EN, en<<DMA_EN);
}
EXPORT_SYMBOL_GPL(mad_dma_en);

int sunxi_mad_hw_params(unsigned int mad_channels, unsigned int sample_rate)
{
    u32 reg_val = 0;

    /*config mad sram audio source channel num*/
    regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_SRAM_CH_MASK,
	    0x1f<<MAD_AD_SRC_CH_NUM, mad_channels<<MAD_AD_SRC_CH_NUM);

    /*config mad sram receive audio channel num*/
    regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_SRAM_CH_MASK,
	    0x1f<<MAD_SRAM_CH_NUM, mad_channels<<MAD_SRAM_CH_NUM);

    /*open mad sram receive channels*/
    reg_val |= (1<<mad_channels)-1;
    regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_SRAM_CH_MASK,
	    0xffff, reg_val<<MAD_SRAM_CH_MASK);

    /*keep lpsd running in 16kHz*/
    if (sample_rate == 16000)
	regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_CTRL,
		1<<AUDIO_DATA_SYNC_FRC, 0<<AUDIO_DATA_SYNC_FRC);
    else
	regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_CTRL,
		1<<AUDIO_DATA_SYNC_FRC, 1<<AUDIO_DATA_SYNC_FRC);

	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_mad_hw_params);

int sunxi_mad_audio_source_sel(unsigned int path_sel, unsigned int enable)
{
    /*
     * arg1-path_sel:
     * 0/others-no audio source; 1-I2S0 input; 2-audiocodec input;
     * 3-DMIC input; 4-I2S1 input; 5-I2S2 input
     */

    if (enable) {
	if ((path_sel >= 1) && (path_sel <= 5)) {
	    regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_AD_PATH_SEL,
		    MAD_AD_PATH_SEL_MASK, path_sel<<MAD_AD_PATH_SEL);
	} else {
	return -EINVAL;
	}
    } else {
	regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_AD_PATH_SEL,
		MAD_AD_PATH_SEL_MASK, 0<<MAD_AD_PATH_SEL);
    }

    return 0;
}
EXPORT_SYMBOL_GPL(sunxi_mad_audio_source_sel);


/*for ASOC cpu_dai*/
int sunxi_mad_suspend_external(unsigned int lpsd_chan_sel,
	unsigned int mad_standby_chan_sel)
{
    u32 reg_val = 0;
    unsigned int mad_is_worked = 1; /*not work*/

    /*disable sram dma*/
    regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_CTRL,
	    1<<DMA_EN, 0<<DMA_EN);

    /*transfer to mad_standby channels*/
    regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_SRAM_CH_MASK,
	    0x1f<<MAD_SRAM_CH_NUM, mad_standby_chan_sel<<MAD_SRAM_CH_NUM);

    /*open mad_standby sram receive channels*/
    reg_val |= (1<<mad_standby_chan_sel)-1;
    regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_SRAM_CH_MASK,
	    0xffff, reg_val<<MAD_SRAM_CH_MASK);

    /*config LPSD receive audio channel num: 1 channel*/
    regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_LPSD_CH_MASK,
	    MAD_LPSD_CH_NUM_MASK<<MAD_LPSD_CH_NUM, 1<<MAD_LPSD_CH_NUM);

    /*enable lpsd DC offset*/
    regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_LPSD_CH_MASK,
	    1<<MAD_LPSD_DCBLOCK_EN, 1<<MAD_LPSD_DCBLOCK_EN);

    /*transfer to mad_standby lpsd sel channels*/
    reg_val |= 1<<(lpsd_chan_sel-1);
    regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_LPSD_CH_MASK,
	    0xffff, reg_val<<MAD_LPSD_CH_MASK);

    /*open MAD_EN*/
    regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_CTRL,
	    1<<MAD_EN, 1<<MAD_EN);

    mdelay(2);
    /*if mad is working well*/
    regmap_read(sunxi_mad_regmap, SUNXI_MAD_STA, &reg_val);
    reg_val |= ~(0x1);
    mad_is_worked = ~reg_val;

    if (!mad_is_worked) {
	return 0;
    } else {
	pr_err("mad isn't working right!\n");
	return -EINVAL;
    }
}
EXPORT_SYMBOL_GPL(sunxi_mad_suspend_external);

/*for ASOC cpu_dai*/
int sunxi_mad_resume_external(unsigned int mad_standby_chan_sel,
	unsigned int audio_src_chan_num)
{
    u32 reg_val = 0;
    unsigned int chan_ch;

    /*close MAD_EN*/
    regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_CTRL,
	    1<<MAD_EN, 0<<MAD_EN);

    /*transfer to normal mad_sram channels*/
    regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_SRAM_CH_MASK,
	    0x1f<<MAD_SRAM_CH_NUM, audio_src_chan_num<<MAD_SRAM_CH_NUM);

    /*transfer to mad sram receive channels*/
    reg_val |= (1<<audio_src_chan_num)-1;
    regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_SRAM_CH_MASK,
	    0xffff, reg_val<<MAD_SRAM_CH_MASK);

    /*config mad_sram channel change*/
    if (mad_standby_chan_sel == 2) {
	switch (audio_src_chan_num) {
	case 2:
	    chan_ch = MAD_CH_COM_NON;
	break;
	case 3:
	    chan_ch = MAD_CH_COM_2CH_TO_4CH;
	break;
	case 4:
	    chan_ch = MAD_CH_COM_2CH_TO_4CH;
	break;
	case 5:
	    chan_ch = MAD_CH_COM_2CH_TO_6CH;
	break;
	case 6:
	    chan_ch = MAD_CH_COM_2CH_TO_6CH;
	break;
	case 7:
	    chan_ch = MAD_CH_COM_2CH_TO_8CH;
	break;
	case 8:
	    chan_ch = MAD_CH_COM_2CH_TO_8CH;
	break;
	default:
	    pr_err("unsupported mad_sram channels!\n");
	return -EINVAL;
	}
    } else if (mad_standby_chan_sel == 4) {
	switch (audio_src_chan_num) {
	case 4:
	    chan_ch = MAD_CH_COM_NON;
	break;
	case 5:
	    chan_ch = MAD_CH_COM_4CH_TO_6CH;
	break;
	case 6:
	    chan_ch = MAD_CH_COM_4CH_TO_6CH;
	break;
	case 7:
	    chan_ch = MAD_CH_COM_4CH_TO_8CH;
	break;
	case 8:
	    chan_ch = MAD_CH_COM_4CH_TO_8CH;
	break;
	default:
	    pr_err("unsupported mad_sram channels!\n");
	return -EINVAL;
	}
    } else {
	pr_err("mad_standby channels isn't set up!\n");
	return -EINVAL;
    }
    regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_SRAM_CH_MASK,
	    MAD_CH_COM_NUM_MASK<<MAD_CH_COM_NUM, chan_ch<<MAD_CH_COM_NUM);

    /*Enable mad_sram channel change
     *when DMA interpolation process finish, the CHANGE_EN bit will be set
     *to 0 automaticallly.
     * */
    regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_SRAM_CH_MASK,
	    1<<MAD_CH_CHANGE_EN, 1<<MAD_CH_CHANGE_EN);

    /*Enable DMA_EN*/
    regmap_update_bits(sunxi_mad_regmap, SUNXI_MAD_CTRL,
	    1<<DMA_EN, 1<<DMA_EN);

	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_mad_resume_external);


/*for internal use*/
static int sunxi_mad_suspend(struct device *dev)
{
	return 0;
}
/*for internal use*/
static int sunxi_mad_resume(struct device *dev)
{
	return 0;
}

static const struct regmap_config sunxi_mad_regmap_config = {
    .reg_bits = 32,
    .reg_stride = 4,
    .val_bits = 32,
    .max_register = SUNXI_MAD_DEBUG,
    .cache_type = REGCACHE_NONE,
};

static int sunxi_mad_probe(struct platform_device *pdev)
{
	struct resource res, *memregion;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_mad_info *sunxi_mad;
	void __iomem *sunxi_mad_membase = NULL;
	unsigned int temp_val;
	int ret;

	sunxi_mad = devm_kzalloc(&pdev->dev, sizeof(struct sunxi_mad_info),
				 GFP_KERNEL);
	if (!sunxi_mad) {
		ret = -ENOMEM;
		goto err_node_put;
	}

	dev_set_drvdata(&pdev->dev, sunxi_mad);
	sunxi_mad->dev = &pdev->dev;

	ret = of_address_to_resource(np, 0, &res);
	if (ret) {
		dev_err(&pdev->dev, "Failed to get sunxi mad resource\n");
		return -EINVAL;
		goto err_devm_kfree;
	}

	memregion = devm_request_mem_region(&pdev->dev, res.start,
					    resource_size(&res), DRV_NAME);
	if (!memregion) {
		dev_err(&pdev->dev, "sunxi mad memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_kfree;
	}

    sunxi_mad_membase = devm_ioremap(&pdev->dev,
					res.start, resource_size(&res));
	if (!sunxi_mad_membase) {
		dev_err(&pdev->dev, "sunxi mad ioremap failed\n");
		ret = -EBUSY;
		goto err_devm_kfree;
	}

    sunxi_mad_regmap = devm_regmap_init_mmio(&pdev->dev,
	    sunxi_mad_membase, &sunxi_mad_regmap_config);
    if (IS_ERR_OR_NULL(sunxi_mad_regmap)) {
	dev_err(&pdev->dev, "sunxi mad registers regmap failed\n");
	ret = -ENOMEM;
	goto err_iounmap;
    }

/*
	sunxi_mad->mad_cfg_clk = of_clk_get(np, 0);
	if (IS_ERR(sunxi_mad->mad_cfg_clk)) {
		dev_err(&pdev->dev, "Can't get mad cfg clocks\n");
		ret = PTR_ERR(sunxi_mad->mad_cfg_clk);
		goto err_iounmap;
	}

	sunxi_mad->mad_ad_clk = of_clk_get(np, 1);
	if (IS_ERR(sunxi_mad->mad_ad_clk)) {
		dev_err(&pdev->dev, "Can't get mad ad clocks\n");
		ret = PTR_ERR(sunxi_mad->mad_ad_clk);
		goto err_mad_cfg_clk_put;
	}
*/
	sunxi_mad->mad_clk = of_clk_get(np, 0);
	if (IS_ERR(sunxi_mad->mad_clk)) {
		dev_err(&pdev->dev, "Can't get mad clocks\n");
		ret = PTR_ERR(sunxi_mad->mad_clk);
		goto err_mad_ad_clk_put;
	}
/*
	sunxi_mad->pll_clk = of_clk_get(np, 3);
	if (IS_ERR(sunxi_mad->pll_clk)) {
		dev_err(&pdev->dev, "Can't get pll clocks\n");
		ret = PTR_ERR(sunxi_mad->pll_clk);
		goto err_mad_clk_put;
	}

	sunxi_mad->hosc_clk = of_clk_get(np, 4);
	if (IS_ERR(sunxi_mad->hosc_clk)) {
		dev_err(&pdev->dev, "Can't get 24MHz hosc clocks\n");
		ret = PTR_ERR(sunxi_mad->hosc_clk);
		goto err_pll_clk_put;
	}
*/
	sunxi_mad->lpsd_clk = of_clk_get(np, 1);
	if (IS_ERR(sunxi_mad->lpsd_clk)) {
		dev_err(&pdev->dev, "Can't get lpsd clocks\n");
		ret = PTR_ERR(sunxi_mad->lpsd_clk);
		goto err_hosc_clk_put;
	}

	ret = of_property_read_u32(np, "lpsd_clk_src_cfg", &temp_val);
	if (ret <= 0) {
		pr_debug("lpsd clk source is pll_audio, 48 div!\n");
		sunxi_mad->pll_audio_src_used = 1;
	} else {
		pr_debug("lpsd clk source is 24 MHz hosc!\n");
		sunxi_mad->hosc_src_used = 1;
	}

	if (sunxi_mad->pll_audio_src_used == 1) {
		if (clk_set_parent(sunxi_mad->lpsd_clk, sunxi_mad->pll_clk)) {
			dev_err(&pdev->dev, "set parent of lpsd_clk to pll_clk fail\n");
			ret = -EBUSY;
			goto err_lpsd_clk_put;
		}
		clk_prepare_enable(sunxi_mad->pll_clk);
		clk_prepare_enable(sunxi_mad->lpsd_clk);
	} else if (sunxi_mad->hosc_src_used == 1) {
		if (clk_set_parent(sunxi_mad->lpsd_clk, sunxi_mad->hosc_clk)) {
			dev_err(&pdev->dev, "set parent of lpsd_clk to hosc_clk fail\n");
			ret = -EBUSY;
			goto err_lpsd_clk_put;
		}
		clk_prepare_enable(sunxi_mad->hosc_clk);
		clk_prepare_enable(sunxi_mad->lpsd_clk);
	} else {
		dev_err(&pdev->dev, "Can't set parent of lpsd_clk\n");
		ret = -EBUSY;
		goto err_lpsd_clk_put;
	}
	/*clk_prepare_enable(sunxi_mad->mad_cfg_clk);*/
	/*clk_prepare_enable(sunxi_mad->mad_ad_clk);*/
	clk_prepare_enable(sunxi_mad->mad_clk);

	return 0;

err_lpsd_clk_put:
    clk_put(sunxi_mad->lpsd_clk);
err_hosc_clk_put:
	clk_put(sunxi_mad->hosc_clk);
err_mad_ad_clk_put:
	clk_put(sunxi_mad->mad_ad_clk);


err_iounmap:
    iounmap(sunxi_mad_membase);
err_devm_kfree:
	devm_kfree(&pdev->dev, sunxi_mad);
err_node_put:
	of_node_put(np);
	return ret;
}

static int __exit sunxi_mad_remove(struct platform_device *pdev)
{
	struct sunxi_mad_info *sunxi_mad = dev_get_drvdata(&pdev->dev);
/*
	clk_put(sunxi_mad->mad_cfg_clk);
	clk_put(sunxi_mad->mad_ad_clk);
	clk_put(sunxi_mad->mad_clk);
	clk_put(sunxi_mad->pll_clk);
	clk_put(sunxi_mad->hosc_clk);
	clk_put(sunxi_mad->lpsd_clk);
*/
	devm_kfree(&pdev->dev, sunxi_mad);
	return 0;
}

static const struct dev_pm_ops sunxi_mad_pm_ops = {
	.suspend = sunxi_mad_suspend,
	.resume = sunxi_mad_resume,
};

static const struct of_device_id sunxi_mad_of_match[] = {
	{ .compatible = "allwinner,sunxi-mad", },
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_mad_of_match);

static struct platform_driver sunxi_mad_driver = {
	.probe = sunxi_mad_probe,
	.remove = __exit_p(sunxi_mad_remove),
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.pm	= &sunxi_mad_pm_ops,
		.of_match_table = sunxi_mad_of_match,
	},
};

module_platform_driver(sunxi_mad_driver);

MODULE_AUTHOR("wolfgang <qinzhenying@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI MAD driver");
MODULE_ALIAS("platform:sunxi-mad");
MODULE_LICENSE("GPL");
