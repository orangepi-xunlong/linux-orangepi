/*
 * drivers/thermal/sunxi-cpu-cooling.c
 *
 * Copyright (C) 2013-2014 allwinner.
 *	kevin.z.m<kevin@allwinnertech.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#include <linux/cpu_cooling.h>
#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

static int sunxi_cpufreq_cooling_probe(struct platform_device *pdev)
{
	struct thermal_cooling_device *cdev;
	struct cpumask mask_val;

	/* make sure cpufreq driver has been initialized */
	if (!cpufreq_frequency_get_table(0))
		return -EPROBE_DEFER;

	cpumask_set_cpu(0, &mask_val);
	cdev = cpufreq_cooling_register(&mask_val);

	if (IS_ERR_OR_NULL(cdev)) {
		dev_err(&pdev->dev, "Failed to register cooling device\n");
		return PTR_ERR(cdev);
	}

	platform_set_drvdata(pdev, cdev);

	dev_info(&pdev->dev, "Cooling device registered: %s\n",	cdev->type);

	return 0;
}

static int sunxi_cpufreq_cooling_remove(struct platform_device *pdev)
{
	struct thermal_cooling_device *cdev = platform_get_drvdata(pdev);

	cpufreq_cooling_unregister(cdev);

	return 0;
}

static struct platform_device sunxi_cpufreq_cooling_device = {
	.name		    = "sunxi-cpufreq-cooling",
	.id		        = PLATFORM_DEVID_NONE,
};

static struct platform_driver sunxi_cpufreq_cooling_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "sunxi-cpufreq-cooling",
	},
	.probe = sunxi_cpufreq_cooling_probe,
	.remove = sunxi_cpufreq_cooling_remove,
};

static int __init sunxi_cpufreq_cooling_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&sunxi_cpufreq_cooling_driver);
	if (IS_ERR_VALUE(ret)) {
		printk("register sunxi_cpufreq_cooling_driver failed\n");
		return ret;
	}

	ret = platform_device_register(&sunxi_cpufreq_cooling_device);
	if (IS_ERR_VALUE(ret)) {
		printk("register sunxi_cpufreq_cooling_device failed\n");
		return ret;
	}

	return ret;
}

static void __exit sunxi_cpufreq_cooling_exit(void)
{
	platform_driver_unregister(&sunxi_cpufreq_cooling_driver);
}

/* Should be later than sunxi_cpufreq register */
late_initcall(sunxi_cpufreq_cooling_init);
module_exit(sunxi_cpufreq_cooling_exit);

MODULE_AUTHOR("kevin.z.m <kevin@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi cpufreq cooling driver");
MODULE_LICENSE("GPL");
