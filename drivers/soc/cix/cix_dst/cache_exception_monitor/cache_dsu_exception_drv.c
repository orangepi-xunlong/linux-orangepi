// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#include <linux/acpi.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_graph.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include "cache_exception_interface.h"
#include "../dst_print.h"

static irqreturn_t dsu_excep_irq_handler(int irq, void *data)
{
	struct cache_excep_drvdata *drvdata = data;

	DST_PRINT_PN("dsu_excep_irq_handler: cpu=%d irq=%d\n", drvdata->cpu, irq);

	cache_excep_record(CACHE_EXCEP_DSU);

	return IRQ_HANDLED;
}

static int cache_excep_dsu_probe(struct platform_device *pdev)
{
	int ret;
	struct cache_excep_drvdata *drvdata;

	drvdata = devm_kzalloc(&pdev->dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	dev_set_drvdata(&pdev->dev, drvdata);

	drvdata->core_type = CORE_DSU;
	drvdata->errirq = platform_get_irq_byname(pdev, "dsu_errirq");
	drvdata->faultirq = platform_get_irq_byname(pdev, "dsu_faultirq");

	if (drvdata->errirq > 0) {
		ret = devm_request_irq(&pdev->dev, drvdata->errirq,
				       dsu_excep_irq_handler, IRQF_ONESHOT,
				       "dsu_errirq", drvdata);
		if (ret) {
			dev_err(&pdev->dev,
				"IRQ request failed: %s (%d) -- %d\n",
				"dsu_errirq", drvdata->errirq, ret);
			return ret;
		}
	}

	if (drvdata->faultirq > 0) {
		ret = devm_request_irq(&pdev->dev, drvdata->faultirq,
				       dsu_excep_irq_handler, IRQF_ONESHOT,
				       "dsu_faultirq", drvdata);
		if (ret) {
			dev_err(&pdev->dev,
				"IRQ request failed: %s (%d) -- %d\n",
				"dsu_errirq", drvdata->faultirq, ret);
			return ret;
		}
	}

	cache_excep_init(drvdata);
	return ret;
}

static int __exit cache_excep_dsu_remove(struct platform_device *pdev)
{
	int ret = 0;
	struct cache_excep_drvdata *drvdata = dev_get_drvdata(&pdev->dev);

	if (drvdata) {
		cache_excep_uninit(drvdata);
	}
	return ret;
}

static int cache_dsu_excep_suspend(struct device *dev)
{
	DST_PRINT_DBG("dev:%s suspend\n", dev_name(dev));
	return 0;
}

static int cache_dsu_excep_resume(struct device *dev)
{
	struct cache_excep_drvdata *drvdata = dev_get_drvdata(dev);

	DST_PRINT_DBG("dev:%s resume\n", dev_name(dev));
	cache_excep_init(drvdata);
	return 0;
}

static SIMPLE_DEV_PM_OPS(cache_dsu_excep_pm_ops, cache_dsu_excep_suspend,
		cache_dsu_excep_resume);

static const struct of_device_id cache_excp_dsu_of_ids[] = {
	{ .compatible = "cix,sky1_exception_dsu_cache" },
	{ }
};
MODULE_DEVICE_TABLE(of, cache_excp_of_ids);

static const struct acpi_device_id cache_excp_dsu_acpi_ids[] = {
	{ "CIXH10F4", 0 },
	{  }
};
MODULE_DEVICE_TABLE(acpi, cache_excp_acpi_ids);

static struct platform_driver cache_dsu_exception_driver = {
	.probe		= cache_excep_dsu_probe,
	.remove		= cache_excep_dsu_remove,
	.driver			= {
		.name			= "sky1_exception_dsu_cache",
		.pm = &cache_dsu_excep_pm_ops,
		.of_match_table		= cache_excp_dsu_of_ids,
		.acpi_match_table = ACPI_PTR(cache_excp_dsu_acpi_ids),
	},
};

static int __init cache_dsu_exception_init(void)
{
	int ret;
	ret = platform_driver_register(&cache_dsu_exception_driver);
	if (!ret)
		return 0;

	pr_err("Error registering core cache exception driver\n");
	return ret;
}

static void __exit cache_dsu_exception_exit(void)
{
	platform_driver_unregister(&cache_dsu_exception_driver);
}

subsys_initcall(cache_dsu_exception_init);
module_exit(cache_dsu_exception_exit);

MODULE_AUTHOR("Vincent Wu <vincent.wu@cixtech.com>");
MODULE_DESCRIPTION("Sky1 cache exception monitor driver");
MODULE_LICENSE("GPL v2");
