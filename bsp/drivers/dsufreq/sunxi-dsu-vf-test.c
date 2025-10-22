// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *  An dsufreq driver for Sunxi Platform of Allwinner SoC
 *
 * Copyright (C) 2023 panzhijian@allwinnertech.com
 */
#include <sunxi-log.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/cpufreq.h>
#include <linux/pm_opp.h>
#include <linux/clk.h>
#include <linux/io.h>

struct sunxi_dsu_vf_dev {
	struct device                 *dev;
	struct clk                    *clk;
};

static struct sunxi_dsu_vf_dev *dsu_vf_dev_temp;

static ssize_t freq_show(struct class *class, struct class_attribute *attr,
			 char *buf)
{
	return sprintf(buf, "%lu\n", clk_get_rate(dsu_vf_dev_temp->clk));
}

static ssize_t freq_store(struct class *class, struct class_attribute *attr,
			  const char *buf, size_t count)
{
	int freq;
	int ret;

	ret = kstrtoint(buf, 10, &freq);
	if (ret)
		return -EINVAL;

	clk_set_rate(dsu_vf_dev_temp->clk, freq);

	return count;
}
static CLASS_ATTR_RW(freq);

static struct attribute *dsu_vf_class_attrs[] = {
	&class_attr_freq.attr,
	NULL,
};
ATTRIBUTE_GROUPS(dsu_vf_class);

static struct class dsu_vf_class = {
	.name		= "dsu_vf_table",
	.class_groups	= dsu_vf_class_groups,
};

static int sunxi_dsu_vf_test_probe(struct platform_device *pdev)
{
	struct sunxi_dsu_vf_dev *dsu_vf_dev = NULL;
	struct device *dev = &pdev->dev;
	int err = 0;

	dsu_vf_dev = devm_kzalloc(dev, sizeof(*dsu_vf_dev), GFP_KERNEL);
	if (!dsu_vf_dev)
		return -ENOMEM;

	dsu_vf_dev->dev = dev;
	dsu_vf_dev->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(dsu_vf_dev->clk)) {
		sunxi_err(dev, "Fail to get clock\n");
		err = PTR_ERR(dsu_vf_dev->clk);
		return err;
	}

	platform_set_drvdata(pdev, dsu_vf_dev);
	dsu_vf_dev_temp = dsu_vf_dev;

	err = class_register(&dsu_vf_class);
	if (err) {
		sunxi_err(NULL, "failed to dsu vf class register\n");
		return err;
	}

	return 0;
}

static int sunxi_dsu_vf_test_remove(struct platform_device *pdev)
{
	struct sunxi_dsu_vf_dev *dsu_vf_dev = platform_get_drvdata(pdev);

	clk_disable_unprepare(dsu_vf_dev->clk);

	return 0;
}

static const struct of_device_id sunxi_dsu_vf_test_of_match[] = {
	{ .compatible = "allwinner,sun55iw3-dsufreq", },
	{ .compatible = "allwinner,dsufreq", },
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_dsu_vf_test_of_match);

static struct platform_driver sunxi_dsu_vf_test_driver = {
	.probe = sunxi_dsu_vf_test_probe,
	.remove	= sunxi_dsu_vf_test_remove,
	.driver = {
		.name = "sunxi-dsu-vf-test",
		.of_match_table = sunxi_dsu_vf_test_of_match,
	},
};
module_platform_driver(sunxi_dsu_vf_test_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Allwinner dsu vf test driver");
MODULE_ALIAS("platform: sunxi_dsu_vf_test");
MODULE_AUTHOR("panzhijian <panzhijian@allwinnertech.com>");
MODULE_VERSION("1.0.0");
