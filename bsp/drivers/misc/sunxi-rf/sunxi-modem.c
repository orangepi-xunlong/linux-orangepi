/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright(c) 2017-2018 Allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include "internal.h"
#include "sunxi-rfkill.h"

static struct sunxi_modem_platdata *modem_data;
static const struct of_device_id sunxi_modem_ids[];

static int sunxi_modem_on(struct sunxi_modem_platdata *data, bool on_off);
static DEFINE_MUTEX(sunxi_modem_mutex);

void sunxi_modem_set_power(bool on_off)
{
	struct platform_device *pdev;
	int ret = 0;

	if (!modem_data)
		return;

	pdev = modem_data->pdev;

	if (modem_data->always_on && !on_off) {
		dev_warn(&pdev->dev, "modem: always on, cannot be off\n");
		return;
	}

	mutex_lock(&sunxi_modem_mutex);
	if (on_off != modem_data->power_state) {
		ret = sunxi_modem_on(modem_data, on_off);
		if (ret)
			dev_err(&pdev->dev, "set power failed\n");
	}
	mutex_unlock(&sunxi_modem_mutex);
}
EXPORT_SYMBOL_GPL(sunxi_modem_set_power);

void sunxi_modem_set_power_boot_state(void)
{
	if (!modem_data)
		return;

	if (modem_data->boot_on || modem_data->always_on)
		sunxi_modem_set_power(1);
}

static int sunxi_modem_on(struct sunxi_modem_platdata *data, bool on_off)
{
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;
	int ret = 0, i;

	if (on_off) {
		for (i = 0; i < PWR_MAX; i++) {
			if (!IS_ERR_OR_NULL(data->power[i])) {
				if (data->power_vol[i]) {
					ret = regulator_set_voltage(data->power[i],
							data->power_vol[i], data->power_vol[i]);
					if (ret < 0) {
						dev_err(dev, "modem power[%d] (%s) set voltage failed\n",
								i, data->power_name[i]);
						return ret;
					}

					ret = regulator_get_voltage(data->power[i]);
					if (ret != data->power_vol[i]) {
						dev_err(dev, "modem power[%d] (%s) get voltage failed\n",
								i, data->power_name[i]);
						return ret;
					}
				}

				ret = regulator_enable(data->power[i]);
				if (ret < 0) {
					dev_err(dev, "modem power[%d] (%s) enable failed\n",
								i, data->power_name[i]);
					return ret;
				}
			}
		}

		if (gpio_is_valid(data->gpio_modem_rst)) {
			mdelay(10);
			gpio_set_value(data->gpio_modem_rst, !data->gpio_modem_rst_assert);
		}
	} else {
		if (gpio_is_valid(data->gpio_modem_rst))
			gpio_set_value(data->gpio_modem_rst, data->gpio_modem_rst_assert);

		for (i = 0; i < PWR_MAX; i++) {
			if (!IS_ERR_OR_NULL(data->power[i])) {
				ret = regulator_disable(data->power[i]);
				if (ret < 0) {
					dev_err(dev, "modem power[%d] (%s) disable failed\n",
								i, data->power_name[i]);
					return ret;
				}
			}
		}
	}

	data->power_state = on_off;
	dev_info(dev, "modem power %s success\n", on_off ? "on" : "off");

	return 0;
}

static ssize_t state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	if (!modem_data)
		return 0;
	return sprintf(buf, "%d\n", modem_data->power_state);
}

static ssize_t state_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	int err;

	if (!modem_data)
		return 0;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	if (state > 1)
		return -EINVAL;

	if (state != modem_data->power_state) {
		sunxi_modem_set_power(state);
	}

	return count;
}

static DEVICE_ATTR(state, S_IRUGO | S_IWUSR,
		state_show, state_store);

static struct attribute *miscdev_attributes_wlan[] = {
	&dev_attr_state.attr,
	NULL,
};

static struct attribute_group miscdev_attribute_group = {
	.name  = "modem",
	.attrs = miscdev_attributes_wlan,
};

int sunxi_modem_init(struct platform_device *pdev)
{
	struct device_node *np = of_find_matching_node(pdev->dev.of_node, sunxi_modem_ids);
	struct device *dev = &pdev->dev;
	struct sunxi_modem_platdata *data;
	enum of_gpio_flags config;
	int ret = 0;
	int count, i;

	if (!dev)
		return -ENOMEM;

	if (!np)
		return 0;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	data->pdev = pdev;

	count = of_property_count_strings(np, "modem_power");
	if (count <= 0) {
		dev_warn(dev, "Missing modem_power.\n");
	} else {
		if (count > PWR_MAX) {
			dev_warn(dev, "modem power count large than max(%d > %d).\n",
				count, PWR_MAX);
			count = PWR_MAX;
		}
		ret = of_property_read_string_array(np, "modem_power",
					(const char **)data->power_name, count);
		if (ret < 0)
			return -ENOMEM;

		ret = of_property_read_u32_array(np, "modem_power_vol",
					(u32 *)data->power_vol, count);
		if (ret < 0)
			dev_warn(dev, "Missing modem_power_vol config.\n");

		for (i = 0; i < count; i++) {
			data->power[i] = regulator_get(dev, data->power_name[i]);
			if (IS_ERR_OR_NULL(data->power[i]))
				return -ENOMEM;

			dev_info(dev, "modem power[%d] (%s) voltage: %dmV\n",
					i, data->power_name[i], data->power_vol[i] / 1000);
		}
	}

	for (i = 0; i < count; i++) {
		data->power[i] = regulator_get(dev, data->power_name[i]);
		if (IS_ERR_OR_NULL(data->power[i]))
			return -ENOMEM;
		dev_info(dev, "modem power[%d] (%s)\n", i, data->power_name[i]);
	}

	data->gpio_modem_rst = of_get_named_gpio_flags(np, "modem_rst", 0, &config);
	if (!gpio_is_valid(data->gpio_modem_rst)) {
		dev_err(dev, "get gpio modem_rst failed\n");
	} else {
		data->gpio_modem_rst_assert = (config == OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		dev_info(dev, "modem_rst gpio=%d assert=%d\n", data->gpio_modem_rst, data->gpio_modem_rst_assert);

		ret = devm_gpio_request(dev, data->gpio_modem_rst, "modem_rst");
		if (ret < 0) {
			dev_err(dev, "can't request modem_rst gpio %d\n",
				data->gpio_modem_rst);
			return ret;
		}

		ret = gpio_direction_output(data->gpio_modem_rst, data->gpio_modem_rst_assert);
		if (ret < 0) {
			dev_err(dev, "can't request output direction modem_rst gpio %d\n",
				data->gpio_modem_rst);
			return ret;
		}
		gpio_set_value(data->gpio_modem_rst, data->gpio_modem_rst_assert);
	}

	data->boot_on = of_property_read_bool(np, "regulator-boot-on") ? 1 : 0;
	data->always_on = of_property_read_bool(np, "regulator-always-on") ? 1 : 0;
	dev_info(dev, "modem power boot-on: %d, always-on: %d\n", data->boot_on, data->always_on);

	ret = sysfs_create_group(&sunxi_rfkill_miscdev.this_device->kobj,
			&miscdev_attribute_group);
	if (ret) {
		dev_err(dev, "sunxi-modem register rfkill miscdev attr group failed!\n");
		return ret;
	}

	data->power_state = 0;
	modem_data = data;
	return 0;
}

int sunxi_modem_deinit(struct platform_device *pdev)
{
	struct sunxi_modem_platdata *data = platform_get_drvdata(pdev);
	int i;

	if (!data)
		return 0;

	sysfs_remove_group(&(sunxi_rfkill_miscdev.this_device->kobj),
			&miscdev_attribute_group);

	if (data->power_state)
		sunxi_modem_set_power(0);

	for (i = 0; i < PWR_MAX; i++) {
		if (!IS_ERR_OR_NULL(data->power[i]))
			regulator_put(data->power[i]);
	}

	modem_data = NULL;

	return 0;
}

static const struct of_device_id sunxi_modem_ids[] = {
	{ .compatible = "allwinner,sunxi-modem" },
	{ /* Sentinel */ }
};

MODULE_DESCRIPTION("sunxi modem rfkill driver");
MODULE_LICENSE("GPL");
