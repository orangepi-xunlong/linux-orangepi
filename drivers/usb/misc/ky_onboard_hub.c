// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Onboard USB Hub support for Ky platform
 *
 * Copyright (c) 2023 Ky Inc.
 */

#include <linux/kernel.h>
#include <linux/resource.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/of_address.h>
#include <linux/gpio/consumer.h>

#include "ky_onboard_hub.h"

#define DRIVER_VERSION "v1.0.2"

static void ky_hub_enable(struct ky_hub_priv *ky, bool on)
{
	unsigned i;
	int active_val = ky->hub_gpio_active_low ? 0 : 1;

	if (!ky->hub_gpios)
		return;

	dev_dbg(ky->dev, "do hub enable %s\n", on ? "on" : "off");

	if (on) {
		for (i = 0; i < ky->hub_gpios->ndescs; i++) {
			gpiod_set_value(ky->hub_gpios->desc[i],
					active_val);
			if (ky->hub_inter_delay_ms) {
				msleep(ky->hub_inter_delay_ms);
			}
		}
	} else {
		for (i = ky->hub_gpios->ndescs; i > 0; --i) {
			gpiod_set_value(ky->hub_gpios->desc[i - 1],
					!active_val);
			if (ky->hub_inter_delay_ms) {
				msleep(ky->hub_inter_delay_ms);
			}
		}
	}
	ky->is_hub_on = on;
}

static void ky_hub_vbus_enable(struct ky_hub_priv *ky,
					 bool on)
{
	unsigned i;
	int active_val = ky->vbus_gpio_active_low ? 0 : 1;

	if (!ky->vbus_gpios)
		return;

	dev_dbg(ky->dev, "do hub vbus on %s\n", on ? "on" : "off");
	if (on) {
		for (i = 0; i < ky->vbus_gpios->ndescs; i++) {
			gpiod_set_value(ky->vbus_gpios->desc[i],
					active_val);
			if (ky->vbus_inter_delay_ms) {
				msleep(ky->vbus_inter_delay_ms);
			}
		}
	} else {
		for (i = ky->vbus_gpios->ndescs; i > 0; --i) {
			gpiod_set_value(ky->vbus_gpios->desc[i - 1],
					!active_val);
			if (ky->vbus_inter_delay_ms) {
				msleep(ky->vbus_inter_delay_ms);
			}
		}
	}
	ky->is_vbus_on = on;
}

static void ky_hub_configure(struct ky_hub_priv *ky, bool on)
{
	dev_dbg(ky->dev, "do hub configure %s\n", on ? "on" : "off");
	if (on) {
		ky_hub_enable(ky, true);
		if (ky->vbus_delay_ms && ky->vbus_gpios) {
			msleep(ky->vbus_delay_ms);
		}
		ky_hub_vbus_enable(ky, true);
	} else {
		ky_hub_vbus_enable(ky, false);
		if (ky->vbus_delay_ms && ky->vbus_gpios) {
			msleep(ky->vbus_delay_ms);
		}
		ky_hub_enable(ky, false);
	}
}

static void ky_read_u32_prop(struct device *dev, const char *name,
				   u32 init_val, u32 *pval)
{
	if (device_property_read_u32(dev, name, pval))
		*pval = init_val;
	dev_dbg(dev, "hub %s, delay: %u ms\n", name, *pval);
}

static int ky_hub_probe(struct platform_device *pdev)
{
	struct ky_hub_priv *ky;
	struct device *dev = &pdev->dev;
	int ret;

	dev_info(&pdev->dev, "%s\n", DRIVER_VERSION);

	ky = devm_kzalloc(&pdev->dev, sizeof(*ky), GFP_KERNEL);
	if (!ky)
		return -ENOMEM;

	ky_read_u32_prop(dev, "hub_inter_delay_ms", 0,
				   &ky->hub_inter_delay_ms);
	ky_read_u32_prop(dev, "vbus_inter_delay_ms", 0,
				   &ky->vbus_inter_delay_ms);
	ky_read_u32_prop(dev, "vbus_delay_ms", 10,
				   &ky->vbus_delay_ms);

	ky->hub_gpio_active_low =
		device_property_read_bool(dev, "hub_gpio_active_low");
	ky->vbus_gpio_active_low =
		device_property_read_bool(dev, "vbus_gpio_active_low");
	ky->suspend_power_on =
		device_property_read_bool(dev, "suspend_power_on");

	pm_runtime_enable(dev);
	pm_runtime_get_noresume(dev);
	pm_runtime_get_sync(dev);
	device_enable_async_suspend(dev);

	ky->hub_gpios = devm_gpiod_get_array_optional(
		&pdev->dev, "hub",
		ky->hub_gpio_active_low ? GPIOD_OUT_HIGH : GPIOD_OUT_LOW);
	if (IS_ERR(ky->hub_gpios)) {
		dev_err(&pdev->dev, "failed to retrieve hub-gpios from dts\n");
		ret = PTR_ERR(ky->hub_gpios);
		goto err_rpm;
	}

	ky->vbus_gpios = devm_gpiod_get_array_optional(
		&pdev->dev, "vbus",
		ky->vbus_gpio_active_low ? GPIOD_OUT_HIGH : GPIOD_OUT_LOW);
	if (IS_ERR(ky->vbus_gpios)) {
		dev_err(&pdev->dev, "failed to retrieve vbus-gpios from dts\n");
		ret = PTR_ERR(ky->vbus_gpios);
		goto err_rpm;
	}

	platform_set_drvdata(pdev, ky);
	ky->dev = &pdev->dev;
	mutex_init(&ky->hub_mutex);

	ky_hub_configure(ky, true);

	dev_info(&pdev->dev, "onboard usb hub driver probed, hub configured\n");

	ky_hub_debugfs_init(ky);

	return 0;
err_rpm:
	pm_runtime_disable(dev);
	pm_runtime_put_sync(dev);
	pm_runtime_put_noidle(dev);
	return ret;
}

static int ky_hub_remove(struct platform_device *pdev)
{
	struct ky_hub_priv *ky = platform_get_drvdata(pdev);

	debugfs_remove(debugfs_lookup(dev_name(&pdev->dev), usb_debug_root));
	ky_hub_configure(ky, false);
	mutex_destroy(&ky->hub_mutex);
	dev_info(&pdev->dev, "onboard usb hub driver exit, disable hub\n");
	pm_runtime_disable(&pdev->dev);
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_put_noidle(&pdev->dev);
	return 0;
}

static const struct of_device_id ky_hub_dt_match[] = {
	{ .compatible = "ky,usb3-hub",},
	{},
};
MODULE_DEVICE_TABLE(of, ky_hub_dt_match);

#ifdef CONFIG_PM_SLEEP
static int ky_hub_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ky_hub_priv *ky = platform_get_drvdata(pdev);
	mutex_lock(&ky->hub_mutex);
	if (!ky->suspend_power_on) {
		ky_hub_configure(ky, false);
		dev_info(dev, "turn off hub power supply\n");
	}
	mutex_unlock(&ky->hub_mutex);
	return 0;
}

static int ky_hub_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ky_hub_priv *ky = platform_get_drvdata(pdev);
	mutex_lock(&ky->hub_mutex);
	if (!ky->suspend_power_on) {
		ky_hub_configure(ky, true);
		dev_info(dev, "resume hub power supply\n");
	}
	mutex_unlock(&ky->hub_mutex);
	return 0;
}

static const struct dev_pm_ops ky_onboard_hub_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ky_hub_suspend, ky_hub_resume)
};
#define DEV_PM_OPS	(&ky_onboard_hub_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif /* CONFIG_PM_SLEEP */

static struct platform_driver ky_hub_driver = {
	.probe	= ky_hub_probe,
	.remove = ky_hub_remove,
	.driver = {
		.name   = "ky-usb3-hub",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(ky_hub_dt_match),
		.pm = DEV_PM_OPS,
	},
};

module_platform_driver(ky_hub_driver);
MODULE_DESCRIPTION("Ky Onboard USB Hub driver");
MODULE_LICENSE("GPL v2");
