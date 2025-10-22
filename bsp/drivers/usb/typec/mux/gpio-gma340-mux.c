// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020-2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Gma340  USB Type-C tx rx cross switch  mux driver
 *
 * Copyright (C) 2023-2024 Allwinner Technology Co., Ltd.
 * Copyright (C) 2022 Linaro Ltd.
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/usb/typec_dp.h>
#include <linux/usb/typec_mux.h>
#include <linux/version.h>
struct gpio_gma340_mux {
	struct gpio_desc *enable_gpio;
	struct gpio_desc *select_gpio;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0)
	struct typec_switch *sw;
	struct typec_mux *mux;
#else
	struct typec_switch_dev *sw;
	struct typec_mux_dev *mux;
#endif

	struct mutex lock; /* protect enabled and swapped */
	bool enabled;
	bool swapped;
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0)
static int gpio_gma340_switch_set(struct typec_switch *sw,
			      enum typec_orientation orientation)
#else
static int gpio_gma340_switch_set(struct typec_switch_dev *sw,
			      enum typec_orientation orientation)
#endif
{
	struct gpio_gma340_mux *gma340_mux = typec_switch_get_drvdata(sw);
	bool enabled;
	bool swapped;

	mutex_lock(&gma340_mux->lock);

	enabled = gma340_mux->enabled;
	swapped = gma340_mux->swapped;

	switch (orientation) {
	case TYPEC_ORIENTATION_NONE:
		enabled = false;
		break;
	case TYPEC_ORIENTATION_NORMAL:
		swapped = false;
		break;
	case TYPEC_ORIENTATION_REVERSE:
		swapped = true;
		break;
	}

	if (enabled != gma340_mux->enabled)
		gpiod_set_value(gma340_mux->enable_gpio, enabled);

	if (swapped != gma340_mux->swapped)
		gpiod_set_value(gma340_mux->select_gpio, swapped);

	gma340_mux->enabled = enabled;
	gma340_mux->swapped = swapped;

	mutex_unlock(&gma340_mux->lock);

	return 0;
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0)
static int gpio_gma340_mux_set(struct typec_mux *mux,
			    struct typec_mux_state *state)
#else
static int gpio_gma340_mux_set(struct typec_mux_dev *mux,
				struct typec_mux_state *state)
#endif

{
	struct gpio_gma340_mux *gma340_mux = typec_mux_get_drvdata(mux);

	mutex_lock(&gma340_mux->lock);

	switch (state->mode) {
	case TYPEC_STATE_SAFE:
	case TYPEC_STATE_USB:
		gma340_mux->enabled = false;
		break;
	case TYPEC_DP_STATE_C:
	case TYPEC_DP_STATE_D:
	case TYPEC_DP_STATE_E:
		gma340_mux->enabled = true;
		break;
	default:
		break;
	}

	gpiod_set_value(gma340_mux->enable_gpio, gma340_mux->enabled);

	mutex_unlock(&gma340_mux->lock);

	return 0;
}

static int gpio_gma340_mux_probe(struct platform_device *pdev)
{
	struct typec_switch_desc sw_desc = { };
	struct typec_mux_desc mux_desc = { };
	struct device *dev = &pdev->dev;
	struct gpio_gma340_mux *gma340_mux;

	gma340_mux = devm_kzalloc(dev, sizeof(*gma340_mux), GFP_KERNEL);
	if (!gma340_mux)
		return -ENOMEM;

	mutex_init(&gma340_mux->lock);

	gma340_mux->enable_gpio = devm_gpiod_get(dev, "enable", GPIOD_OUT_LOW);
	if (IS_ERR(gma340_mux->enable_gpio))
		return dev_err_probe(dev, PTR_ERR(gma340_mux->enable_gpio),
				     "unable to acquire enable gpio\n");

	gma340_mux->select_gpio = devm_gpiod_get(dev, "select", GPIOD_OUT_LOW);
	if (IS_ERR(gma340_mux->select_gpio))
		return dev_err_probe(dev, PTR_ERR(gma340_mux->select_gpio),
				     "unable to acquire select gpio\n");

	sw_desc.drvdata = gma340_mux;
	sw_desc.fwnode = dev_fwnode(dev);
	sw_desc.set = gpio_gma340_switch_set;

	gma340_mux->sw = typec_switch_register(dev, &sw_desc);
	if (IS_ERR(gma340_mux->sw))
		return dev_err_probe(dev, PTR_ERR(gma340_mux->sw),
				     "failed to register typec switch\n");

	mux_desc.drvdata = gma340_mux;
	mux_desc.fwnode = dev_fwnode(dev);
	mux_desc.set = gpio_gma340_mux_set;

	gma340_mux->mux = typec_mux_register(dev, &mux_desc);
	if (IS_ERR(gma340_mux->mux)) {
		typec_switch_unregister(gma340_mux->sw);
		return dev_err_probe(dev, PTR_ERR(gma340_mux->mux),
				     "failed to register typec mux\n");
	}

	platform_set_drvdata(pdev, gma340_mux);

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0)
static int gpio_gma340_mux_remove(struct platform_device *pdev)
#else
static void gpio_gma340_mux_remove(struct platform_device *pdev)
#endif
{
	struct gpio_gma340_mux *gma340_mux = platform_get_drvdata(pdev);

	gpiod_set_value(gma340_mux->enable_gpio, 0);

	typec_mux_unregister(gma340_mux->mux);
	typec_switch_unregister(gma340_mux->sw);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0)
	return 0;
#endif
}

static const struct of_device_id gpio_gma340_mux_match[] = {
	{ .compatible = "gpio-gma340-mux", },
	{}
};
MODULE_DEVICE_TABLE(of, gpio_gma340_mux_match);

static struct platform_driver gpio_gma340_mux_driver = {
	.probe = gpio_gma340_mux_probe,
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0)
	.remove 	= gpio_gma340_mux_remove,
#else
	.remove_new = gpio_gma340_mux_remove,
#endif
	.driver = {
		.name = "gpio-gma340-mux",
		.of_match_table = gpio_gma340_mux_match,
	},
};
module_platform_driver(gpio_gma340_mux_driver);

MODULE_ALIAS("platform:gma340-driver");
MODULE_DESCRIPTION("GPIO gma340 mux driver");
MODULE_AUTHOR("chenhuaqiang<chenhuaqiang@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
