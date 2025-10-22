/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Regulator driver for PWM Regulators
 *
 * Copyright (C) 2014 - STMicroelectronics Inc.
 *
 * Author: Lee Jones <lee.jones@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sunxi-power-regulator.h"

static int virtual_consumer_regulator_probe(struct platform_device *pdev)
{
	struct device *dev = NULL;
	struct regulator *supply;
	dev = &pdev->dev;

	supply = devm_regulator_get(dev, "power");
	if (IS_ERR(supply)) {
		dev_err(dev, "failed to get virtual_consumer regulator\n");
		return PTR_ERR(supply);
	}

	return 0;
}

static int virtual_consumer_regulator_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id virtual_consumer_of_match[] = {
	{ .compatible = "allwinner,virtual_consumer_regulator" },
	{ },
};
MODULE_DEVICE_TABLE(of, virtual_consumer_of_match);

static struct platform_driver sunxi_virtual_consumer_regulator_driver = {
	.driver = {
		.name		= "virtual-consumer-regulator",
		.of_match_table = of_match_ptr(virtual_consumer_of_match),
	},
	.probe = virtual_consumer_regulator_probe,
	.remove = virtual_consumer_regulator_remove,
};

module_platform_driver(sunxi_virtual_consumer_regulator_driver);
MODULE_AUTHOR("liufeng <liufeng@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi_virtual_consumer_regulator_driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
