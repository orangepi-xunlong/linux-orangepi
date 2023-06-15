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

#define SUNXI_RA1PE	0x2CC0
#define SUNXI_RA1PC	0x2CC4
#define SUNXI_RA1PP	0x2CC8
#define SUNXI_RA1DR	0x2CD4
#define SUNXI_RA1DW	0x2CD8

#define PERIOD		100	/* ms */
#define SECOND		1000	/* ms(const) */

#define DRIVER_NAME	"DFI Driver"
/*
 * The dfi controller can monitor DDR load. It has an upper and lower threshold
 * for the operating points. Whenever the usage leaves these bounds an event is
 * generated to indicate the DDR frequency should be changed.
 */
struct sunxi_nsipmu {
	struct clk *dram_clk;
	struct devfreq_event_dev *edev;
	struct devfreq_event_desc *desc;
	struct device *dev;
	struct regmap *regmap;
};

static void sunxi_nsipmu_start_hardware_counter(struct devfreq_event_dev *edev)
{
	struct sunxi_nsipmu *info = devfreq_event_get_drvdata(edev);
	unsigned int val;

	/* Automatically updated every 100ms */
	val = ((clk_get_rate(info->dram_clk)  >> 1) / SECOND) * PERIOD;
	regmap_write(info->regmap, SUNXI_RA1PP, val);
	regmap_update_bits(info->regmap, SUNXI_RA1PE, BIT(0), -1U);
}

static void sunxi_nsipmu_stop_hardware_counter(struct devfreq_event_dev *edev)
{
	struct sunxi_nsipmu *info = devfreq_event_get_drvdata(edev);

	regmap_update_bits(info->regmap, SUNXI_RA1PE, BIT(0), 0);
	regmap_update_bits(info->regmap, SUNXI_RA1PC, BIT(0), -1U);
	udelay(1);
	regmap_update_bits(info->regmap, SUNXI_RA1PC, BIT(0), 0);
}

static int sunxi_nsipmu_disable(struct devfreq_event_dev *edev)
{
	sunxi_nsipmu_stop_hardware_counter(edev);

	return 0;
}

static int sunxi_nsipmu_enable(struct devfreq_event_dev *edev)
{
	sunxi_nsipmu_start_hardware_counter(edev);
	return 0;
}

static int sunxi_nsipmu_set_event(struct devfreq_event_dev *edev)
{
	return 0;
}

static int sunxi_nsipmu_get_event(struct devfreq_event_dev *edev,
				  struct devfreq_event_data *edata)
{
	struct sunxi_nsipmu *info = devfreq_event_get_drvdata(edev);
	unsigned int read_data, write_data;

	regmap_read(info->regmap, SUNXI_RA1DR, &read_data);
	regmap_read(info->regmap, SUNXI_RA1DW, &write_data);

	info->dram_clk = devm_clk_get(info->dev, "dram");
	if (IS_ERR(info->dram_clk))
		return PTR_ERR(info->dram_clk);

	/*
	 * read/write: In byte
	 * Max Utilization
	 *
	 * load = (read + write) / (dram_clk * 2 * 4)
	 */
	edata->load_count = (unsigned long)(read_data + write_data);
	edata->total_count = (clk_get_rate(info->dram_clk) / 10) * 8;

	pr_info("dram_clk:%dM load:%ld rw:%dM total:%dM\n",
				clk_get_rate(info->dram_clk) / 1000000,
				(edata->load_count >> 7) * 100 / (edata->total_count >> 7),
				edata->load_count / 100000, edata->total_count / 100000);

	return 0;
}

static const struct devfreq_event_ops sunxi_nsipmu_ops = {
	.disable = sunxi_nsipmu_disable,
	.enable = sunxi_nsipmu_enable,
	.get_event = sunxi_nsipmu_get_event,
	.set_event = sunxi_nsipmu_set_event,
};

static const struct of_device_id sunxi_nsipmu_id_match[] = {
	{
		.compatible = "allwinner,sunxi-dfi",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_nsipmu_id_match);

static int sunxi_nsipmu_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_nsipmu *data;
	struct devfreq_event_desc *desc;
	struct device_node *np = pdev->dev.of_node;

	data = devm_kzalloc(dev, sizeof(struct sunxi_nsipmu), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->dram_clk = devm_clk_get(dev, "dram");
	if (IS_ERR(data->dram_clk)) {
		dev_err(&pdev->dev, "devm_clk_get error!\n");
		return PTR_ERR(data->dram_clk);
	}

	data->regmap = syscon_node_to_regmap(dev->of_node);
	if (IS_ERR(data->regmap)) {
		dev_err(&pdev->dev, "syscon_node_to_regmap error!\n");
		return PTR_ERR(data->regmap);
	}

	data->dev = dev;

	desc = devm_kzalloc(dev, sizeof(*desc), GFP_KERNEL);
	if (!desc)
		return -ENOMEM;

	desc->ops = &sunxi_nsipmu_ops;
	desc->driver_data = data;
	desc->name = np->name;
	data->desc = desc;

	data->edev = devm_devfreq_event_add_edev(&pdev->dev, desc);
	if (IS_ERR(data->edev)) {
		dev_err(&pdev->dev, "devm_devfreq_event_add_edev error!\n");
		return PTR_ERR(data->edev);
	}

	platform_set_drvdata(pdev, data);

	return 0;
}

static int sunxi_nsipmu_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver sunxi_nsipmu_driver = {
	.probe	= sunxi_nsipmu_probe,
	.remove  = sunxi_nsipmu_remove,
	.driver = {
		.name	= "sunxi-nsipmu",
		.of_match_table = sunxi_nsipmu_id_match,
	},
};

module_platform_driver(sunxi_nsipmu_driver);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("sunxi dfi driver");
MODULE_ALIAS("platform:" DRIVER_NAME);
MODULE_AUTHOR("fanqinghua <fanqinghua@allwinnertech.com>");
MODULE_AUTHOR("frank <frank@allwinnertech.com>");
MODULE_VERSION("1.0.0");
