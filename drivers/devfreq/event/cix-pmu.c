// SPDX-License-Identifier: GPL-2.0-only
/*
 * Generic Cix Bus frequency driver with DEVFREQ Framework
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.All Rights Reserved.
 */

#include <linux/devfreq-event.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/of.h>

#define PMU_EVENT_L_MASK	0xffffff
#define PMU_EVENT_H_SHIFT	24

enum REG_PMU {
	REG_EVENT1_H,
	REG_EVENT1_L,
	REG_EVENT2_H,
	REG_EVENT2_L,
	REG_CNT,
};

struct cix_bus_usage {
	u64 access;
	u64 total;
};

struct cix_pmu {
	struct devfreq_event_dev *edev;
	struct devfreq_event_desc *desc;
	struct cix_bus_usage usage;
	struct device *dev;
	void __iomem *regs[REG_CNT];
};

static int cix_pmu_get_data(struct devfreq_event_dev *edev)
{
	struct cix_pmu *info = devfreq_event_get_drvdata(edev);
	u64 pmu_event1_count_h, pmu_event1_count_l, pmu_event1_count;
	u64 pmu_event2_count_h, pmu_event2_count_l, pmu_event2_count;

	pmu_event1_count_l = readl_relaxed(info->regs[REG_EVENT1_L]);
	pmu_event1_count_h = readl_relaxed(info->regs[REG_EVENT1_H]);
	pmu_event2_count_l = readl_relaxed(info->regs[REG_EVENT2_L]);
	pmu_event2_count_h = readl_relaxed(info->regs[REG_EVENT2_H]);

	pmu_event1_count = (pmu_event1_count_h << PMU_EVENT_H_SHIFT) |
				(pmu_event1_count_l & PMU_EVENT_L_MASK);
	pmu_event2_count = (pmu_event2_count_h << PMU_EVENT_H_SHIFT) |
				(pmu_event2_count_l & PMU_EVENT_L_MASK);
	info->usage.access = pmu_event1_count + pmu_event2_count;
	return 0;
}

static int cix_pmu_disable(struct devfreq_event_dev *edev)
{
	return 0;
}

static int cix_pmu_enable(struct devfreq_event_dev *edev)
{
	return 0;
}

static int cix_pmu_set_event(struct devfreq_event_dev *edev)
{
	return 0;
}

static int cix_pmu_get_event(struct devfreq_event_dev *edev,
				  struct devfreq_event_data *edata)
{
	struct cix_pmu *info = devfreq_event_get_drvdata(edev);

	cix_pmu_get_data(edev);
	edata->load_count = info->usage.access;
	edata->total_count = info->usage.total;

	return 0;
}

static const struct devfreq_event_ops cix_pmu_ops = {
	.disable = cix_pmu_disable,
	.enable = cix_pmu_enable,
	.get_event = cix_pmu_get_event,
	.set_event = cix_pmu_set_event,
};

static const struct of_device_id cix_pmu_id_match[] = {
	{ .compatible = "cix,pmu" },
	{ },
};
MODULE_DEVICE_TABLE(of, cix_pmu_id_match);

static int cix_pmu_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cix_pmu *data;
	struct devfreq_event_desc *desc;
	int i;

	data = devm_kzalloc(dev, sizeof(struct cix_pmu), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	for ( i = 0; i < REG_CNT; i++) {
		data->regs[i] = devm_platform_ioremap_resource(pdev, i);
		if (IS_ERR(data->regs[i]))
			return PTR_ERR(data->regs[i]);
	}

	data->dev = dev;

	desc = devm_kzalloc(dev, sizeof(*desc), GFP_KERNEL);
	if (!desc)
		return -ENOMEM;

	desc->ops = &cix_pmu_ops;
	desc->driver_data = data;
	desc->name = "cix_pmu";
	data->desc = desc;

	data->edev = devm_devfreq_event_add_edev(&pdev->dev, desc);
	if (IS_ERR(data->edev)) {
		dev_err(&pdev->dev,
			"failed to add devfreq-event device\n");
		return PTR_ERR(data->edev);
	}

	platform_set_drvdata(pdev, data);
	return 0;
}

static struct platform_driver cix_pmu_driver = {
	.probe	= cix_pmu_probe,
	.driver = {
		.name	= "cix-pmu",
		.of_match_table = cix_pmu_id_match,
	},
};
module_platform_driver(cix_pmu_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Cixtech,Inc.");
MODULE_DESCRIPTION("Cix pmu driver");
