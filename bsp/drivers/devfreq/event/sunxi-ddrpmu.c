/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner DFI support.
 *
 * Copyright (C) 2019 Allwinner Technology, Inc.
 *	fanqinghua <fanqinghua@allwinnertech.com>
 *
 * Supplied ddr loading info for devfreq.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <sunxi-log.h>
#include <linux/clk.h>
#include <linux/devfreq-event.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/of.h>
#include <linux/delay.h>

#define PMU_PER_REG			0x1030
#define PMU_CFG_REG			0x1034
#define PMU_CLK_SEL			BIT(5)
#define PMU_MODE_MASK		(0xf << 8)

#define PMU_SOFT_CTRL_REG	0x1060
#define PMU_CLR				BIT(1)
#define PMU_RESETN			BIT(0)

#define PMU_EN_REG			0x1064
#define PMU_EN				BIT(0)

#define PMU_REQ_RW_REG		0x1070
#define PMU_REQ_R_REG		0x1074
#define PMU_REQ_W_REG		0x1078
#define MASTER_REG0			0x10000
#define DDR_TYPE_LPDDR4		BIT(5)

#define PERIOD		100	/* ms */
#define SECOND		1000	/* ms(const) */

#define DRIVER_NAME	"DFI Driver"
/*
 * The dfi controller can monitor DDR load. It has an upper and lower threshold
 * for the operating points. Whenever the usage leaves these bounds an event is
 * generated to indicate the DDR frequency should be changed.
 */
struct sunxi_ddrpmu {
	struct clk *dram_clk;
	struct devfreq_event_dev *edev;
	struct devfreq_event_desc *desc;
	struct device *dev;
	struct regmap *regmap;
};

static int dbg_enable;
module_param_named(dbg_level, dbg_enable, int, 0644);

#define DBG(args...) \
	do { \
		if (dbg_enable) { \
			sunxi_info(NULL, args); \
		} \
	} while (0)

static void sunxi_ddrpmu_init(struct sunxi_ddrpmu *info)
{
	/* select 200M clock source for ddr pmu monitor conter*/
	regmap_update_bits(info->regmap, PMU_CFG_REG, PMU_CLK_SEL, 0U);
	/* use traditional mode*/
	regmap_update_bits(info->regmap, PMU_CFG_REG, PMU_MODE_MASK, (0x1 << 8));
	regmap_update_bits(info->regmap, PMU_SOFT_CTRL_REG, PMU_RESETN, -1U);
}

static void sunxi_ddrpmu_start_hardware_counter(struct devfreq_event_dev *edev)
{
	struct sunxi_ddrpmu *info = devfreq_event_get_drvdata(edev);
	unsigned int val;

	/* Automatically updated every 100ms */
	val = (200000000UL / SECOND) * PERIOD;
	regmap_write(info->regmap, PMU_PER_REG, val);
	regmap_update_bits(info->regmap, PMU_EN_REG, PMU_EN, -1U);
}

static void sunxi_ddrpmu_stop_hardware_counter(struct devfreq_event_dev *edev)
{
	struct sunxi_ddrpmu *info = devfreq_event_get_drvdata(edev);

	regmap_update_bits(info->regmap, PMU_EN_REG, PMU_EN, 0);
	regmap_update_bits(info->regmap, PMU_SOFT_CTRL_REG, PMU_CLR, -1U);
	udelay(1);
	regmap_update_bits(info->regmap, PMU_SOFT_CTRL_REG, PMU_CLR, 0);
}

static int sunxi_ddrpmu_disable(struct devfreq_event_dev *edev)
{
	sunxi_ddrpmu_stop_hardware_counter(edev);

	return 0;
}

static int sunxi_ddrpmu_enable(struct devfreq_event_dev *edev)
{
	sunxi_ddrpmu_start_hardware_counter(edev);
	return 0;
}

static int sunxi_ddrpmu_set_event(struct devfreq_event_dev *edev)
{
	return 0;
}

static int sunxi_ddrpmu_get_event(struct devfreq_event_dev *edev,
				  struct devfreq_event_data *edata)
{
	struct sunxi_ddrpmu *info = devfreq_event_get_drvdata(edev);
	unsigned int rw_data = 0, ddr_type = 0;

	regmap_read(info->regmap, PMU_REQ_RW_REG, &rw_data);
	regmap_read(info->regmap, MASTER_REG0, &ddr_type);
	ddr_type &= DDR_TYPE_LPDDR4;

	/*
	 * read/write: In byte
	 * Max Utilization
	 *
	 * load = (read + write) / (dram_clk * 2 * 4)
	 */
	if (ddr_type)
		edata->load_count = (unsigned long)rw_data << 6;
	else
		edata->load_count = (unsigned long)rw_data << 5;
	edata->total_count = (clk_get_rate(info->dram_clk) / 10) * 8;

	DBG("dram_clk:%ldM load:%ld rw:%ldM total:%ldM\n",
				clk_get_rate(info->dram_clk) / 1000000,
				(edata->load_count >> 7) * 100 / (edata->total_count >> 7),
				edata->load_count / 100000, edata->total_count / 100000);

	return 0;
}

static const struct devfreq_event_ops sunxi_ddrpmu_ops = {
	.disable = sunxi_ddrpmu_disable,
	.enable = sunxi_ddrpmu_enable,
	.get_event = sunxi_ddrpmu_get_event,
	.set_event = sunxi_ddrpmu_set_event,
};

static const struct of_device_id sunxi_ddrpmu_id_match[] = {
	{
		.compatible = "allwinner,sunxi-ddrpmu",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_ddrpmu_id_match);

static int sunxi_ddrpmu_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_ddrpmu *data;
	struct devfreq_event_desc *desc;
	struct device_node *np = pdev->dev.of_node;

	data = devm_kzalloc(dev, sizeof(struct sunxi_ddrpmu), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->dram_clk = devm_clk_get(dev, "dram");
	if (IS_ERR(data->dram_clk)) {
		sunxi_err(&pdev->dev, "devm_clk_get error!\n");
		return PTR_ERR(data->dram_clk);
	}

	data->regmap = syscon_node_to_regmap(dev->of_node);
	if (IS_ERR(data->regmap)) {
		sunxi_err(&pdev->dev, "syscon_node_to_regmap error!\n");
		return PTR_ERR(data->regmap);
	}

	data->dev = dev;

	desc = devm_kzalloc(dev, sizeof(*desc), GFP_KERNEL);
	if (!desc)
		return -ENOMEM;

	desc->ops = &sunxi_ddrpmu_ops;
	desc->driver_data = data;
	desc->name = np->name;
	data->desc = desc;

	data->edev = devm_devfreq_event_add_edev(&pdev->dev, desc);
	if (IS_ERR(data->edev)) {
		sunxi_err(&pdev->dev, "devm_devfreq_event_add_edev error!\n");
		return PTR_ERR(data->edev);
	}

	sunxi_ddrpmu_init(data);
	platform_set_drvdata(pdev, data);

	return 0;
}

static int sunxi_ddrpmu_remove(struct platform_device *pdev)
{
	return 0;
}
static __maybe_unused int sunxi_ddrpmu_suspend(struct device *dev)
{
	return 0;
}

static __maybe_unused int sunxi_ddrpmu_resume(struct device *dev)
{
	struct sunxi_ddrpmu *ddrpmu = dev_get_drvdata(dev);
	int ret = 0;

	if (ddrpmu == NULL)
		return 0;

	sunxi_ddrpmu_init(ddrpmu);

	return ret;
}

static SIMPLE_DEV_PM_OPS(sunxi_ddrpmu_pm, sunxi_ddrpmu_suspend,
			 sunxi_ddrpmu_resume);

static struct platform_driver sunxi_ddrpmu_driver = {
	.probe	= sunxi_ddrpmu_probe,
	.remove  = sunxi_ddrpmu_remove,
	.driver = {
		.name	= "sunxi-ddrpmu",
		.pm = &sunxi_ddrpmu_pm,
		.of_match_table = sunxi_ddrpmu_id_match,
	},
};

module_platform_driver(sunxi_ddrpmu_driver);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("sunxi dfi driver");
MODULE_ALIAS("platform:" DRIVER_NAME);
MODULE_AUTHOR("fanqinghua <fanqinghua@allwinnertech.com>");
MODULE_VERSION("1.0.0");
