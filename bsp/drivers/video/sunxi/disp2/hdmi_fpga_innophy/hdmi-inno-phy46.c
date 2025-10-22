// SPDX-License-Identifier: GPL-2.0-only
/*
 * I2C driver for the X-Powers' Power Management ICs
 *
 * AXP20x typically comprises an adaptive USB-Compatible PWM charger, BUCK DC-DC
 * converters, LDOs, multiple 12-bit ADCs of voltage, current and temperature
 * as well as configurable GPIOs.
 *
 * This driver supports the I2C variants.
 *
 * Copyright (C) 2014 Carlo Caione
 *
 * Author: Carlo Caione <carlo@caione.org>
 */
#include <linux/acpi.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/of.h>
#include <linux/slab.h>

static const struct regmap_config innophy_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
};

static struct regmap *regmap46 = NULL;

void *get_innophy46_regmap(void)
{
	return regmap46;
}
EXPORT_SYMBOL_GPL(get_innophy46_regmap);

static int innophy46_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	int ret;

	printk("---hwh %s---enter\n", __func__);

	regmap46 = devm_regmap_init_i2c(i2c, &innophy_regmap_config);
	if (IS_ERR(regmap46)) {
		ret = PTR_ERR(regmap46);
		dev_err(&i2c->dev, "regmap init failed: %d\n", ret);
		return ret;
	}

	return 0;
}

static void innophy46_remove(struct i2c_client *i2c)
{

	printk("---hwh %s---\n", __func__);
	return;
}

static struct i2c_device_id innophy_i2c_id[] = {
	{ "innophy,phy46", 0},
};

static const struct of_device_id innophy46_i2c_of_match[] = {
	{ .compatible = "innophy,phy46", .data = NULL },
	{ },
};
MODULE_DEVICE_TABLE(of, innophy46_i2c_of_match);


static struct i2c_driver innophy46_i2c_driver = {
	.driver = {
		.name	= "innophy46-i2c",
		.of_match_table	= of_match_ptr(innophy46_i2c_of_match),
	},
	.id_table	= innophy_i2c_id,
	.probe		= innophy46_probe,
	.remove		= innophy46_remove,
};

module_i2c_driver(innophy46_i2c_driver);

MODULE_DESCRIPTION("hdmi inno phy");
MODULE_AUTHOR("huangwenhao <huangwenhao@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
