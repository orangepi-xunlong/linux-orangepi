/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs power domain test driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <sunxi-log.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/pm_runtime.h>
#include <linux/pm_domain.h>

static ssize_t
sunxi_pdtest_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", (atomic_read(&dev->power.usage_count) > 0) ? "on" : "off");
}

static ssize_t
sunxi_pdtest_status_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int usage_count;

	if (!strncmp(buf, "on", sizeof("on") - 1)) {
		pm_runtime_get(dev);
		usage_count = atomic_read(&dev->power.usage_count);
		if (usage_count < 1)
			sunxi_debug(NULL, "runtime usage_count %d < 1, can't not enable power domain\n", usage_count);
		else
			sunxi_debug(NULL, "runtime usage_count %d, enable power domain success!\n", usage_count);
	} else if (!strncmp(buf, "off", sizeof("off") - 1)) {
		pm_runtime_put(dev);
		usage_count = atomic_read(&dev->power.usage_count);
		if (usage_count > 0)
			sunxi_debug(NULL, "runtime usage_count %d > 0, can't not disable power domain\n", usage_count);
		else
			sunxi_debug(NULL, "runtime usage_count %d, disable power domain success!\n", usage_count);
	} else {
		sunxi_err(NULL, "Only 'on' or 'off' command support!\n");
	}
	return count;
}

static DEVICE_ATTR(status, S_IWUSR | S_IRUGO,
		sunxi_pdtest_status_show, sunxi_pdtest_status_store);

static int sunxi_pdtest_attach_pd(struct device *dev)
{
	struct device_node *np = dev_of_node(dev);
	struct device_link *link;
	struct device *pd_dev;
	char pd_name[32];
	u32 i, pd_num;

	/* Do nothing when in a single power domain */
	if (dev->pm_domain)
		return 0;

	/* assume power-domain-cells is 1 */
	pd_num = of_property_count_elems_of_size(np, "power-domains", sizeof(phandle) + sizeof(u32));
	sunxi_warn(dev, "Has %d power domains.\n", pd_num);

	for (i = 0; i < pd_num; i++) {
		snprintf(pd_name, sizeof(pd_name), "pd_test_multi_%d", i);

		pd_dev = dev_pm_domain_attach_by_name(dev, pd_name);
		if (IS_ERR_OR_NULL(pd_dev))
			return PTR_ERR(pd_dev) ? : -ENODATA;

		link = device_link_add(dev, pd_dev,
				DL_FLAG_STATELESS |
				DL_FLAG_PM_RUNTIME);
		if (!link) {
			sunxi_err(dev, "Failed to add device_link to pd_test0_sram.\n");
			return -EINVAL;
		}
	}

	/* no unroll attach routine */
	return 0;
}

static int sunxi_pdtest_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	int ret;

	if (np == NULL) {
		sunxi_err(NULL, "pd_test pdev failed to get of_node\n");
		return -ENODEV;
	}

	ret = device_create_file(dev, &dev_attr_status);
	if (ret)
		return ret;

	ret = sunxi_pdtest_attach_pd(dev);
	if (ret)
		goto remove_file;

	pm_runtime_set_active(dev);
	pm_runtime_set_autosuspend_delay(dev, 20 * MSEC_PER_SEC);
	pm_runtime_use_autosuspend(dev);
	pm_runtime_enable(dev);

	return ret;

remove_file:
	device_remove_file(dev, &dev_attr_status);

	return ret;
}

static int sunxi_pdtest_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	pm_runtime_disable(&pdev->dev);
	pm_runtime_dont_use_autosuspend(&pdev->dev);
	device_remove_file(dev, &dev_attr_status);

	return 0;
}

static int sunxi_pdtest_runtime_suspend(struct device *dev)
{
	sunxi_warn(dev, "runtime_suspend disable clock\n");

	return 0;
}

static int sunxi_pdtest_runtime_resume(struct device *dev)
{
	sunxi_warn(dev, "runtime_resume enable clock\n");

	return 0;
}

static int __maybe_unused sunxi_pdtest_suspend(struct device *dev)
{
	int ret;

	sunxi_warn(dev, "suspend disable clock\n");

	ret = pm_runtime_force_suspend(dev);

	return ret;
}

static int __maybe_unused sunxi_pdtest_resume(struct device *dev)
{
	int ret;

	sunxi_warn(dev, "resume enable clock\n");

	ret = pm_runtime_force_resume(dev);

	return ret;
}

static const struct dev_pm_ops sunxi_pdtest_pm_ops = {
	SET_RUNTIME_PM_OPS(sunxi_pdtest_runtime_suspend,
			   sunxi_pdtest_runtime_resume, NULL)
	SET_SYSTEM_SLEEP_PM_OPS(sunxi_pdtest_suspend,
				sunxi_pdtest_resume)
};

static const struct of_device_id sunxi_pdtest_match[] = {
	{ .compatible = "allwinner,sunxi-power-domain-test"},
	{}
};

static struct platform_driver sunxi_pdtest_driver = {
	.probe = sunxi_pdtest_probe,
	.remove	= sunxi_pdtest_remove,
	.driver = {
		.name = "pdtest",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_pdtest_match,
		.pm = &sunxi_pdtest_pm_ops,
	},
};

static int __init sunxi_pdtest_init(void)
{
	int ret;
	ret = platform_driver_register(&sunxi_pdtest_driver);
	if (ret) {
		sunxi_warn(NULL, "register sunxi power domain test driver failed\n");
		return -EINVAL;
	}

	return 0;
}

static void __exit sunxi_pdtest_exit(void)
{
	platform_driver_unregister(&sunxi_pdtest_driver);
}

subsys_initcall(sunxi_pdtest_init);
module_exit(sunxi_pdtest_exit);
MODULE_AUTHOR("Huangyongxing<huangyongxing@allwinnertech.com");
MODULE_VERSION("1.0.0");
MODULE_DESCRIPTION("Allwinner sunxi power domain driver test");
MODULE_LICENSE("GPL");
