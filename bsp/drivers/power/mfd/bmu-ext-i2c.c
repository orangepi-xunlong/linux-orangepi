/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * I2C driver for the ETA6973 Power Management ICs
 *
 *
 * This driver supports the I2C variants.
 *
 * Copyright (C) 2014 Carlo Caione
 *
 * Author: Carlo Caione <carlo@caione.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sunxi-power-mfd.h"
#include "bmu-ext.h"

static const struct of_device_id bmu_ext_i2c_of_match_table[] = {
	{ .compatible = "eta,eta6973", .data = (void *)ETA6973_ID },
	{ .compatible = "x-powers-ext,axp519", .data = (void *)AXP519_ID },
	{ .compatible = "x-powers-ext,axp2601", .data = (void *)AXP2601_ID },
	{ .compatible = "x-powers-ext,axp2602", .data = (void *)AXP2602_ID },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, bmu_ext_i2c_of_match_table);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static int bmu_ext_i2c_probe(struct i2c_client *client,
							 const struct i2c_device_id *ids)
#else
static int bmu_ext_i2c_probe(struct i2c_client *client)
#endif
{
	struct bmu_ext_dev *ext;
	int ret;

	ext = devm_kzalloc(&client->dev, sizeof(*ext), GFP_KERNEL);
	if (!ext)
		return -ENOMEM;

	ext->dev = &client->dev;
	ext->irq = client->irq;
	dev_set_drvdata(ext->dev, ext);

	ret = bmu_ext_match_device(ext);
	if (ret)
		return ret;

	ext->regmap = devm_regmap_init_i2c(client, ext->regmap_cfg);
	if (IS_ERR(ext->regmap)) {
		PMIC_DEV_ERR(ext->dev, "Failed to initialize register map\n");
		return PTR_ERR(ext->regmap);
	}

	return bmu_ext_device_init(ext);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
static int bmu_ext_i2c_remove(struct i2c_client *client)
{
	struct bmu_ext_dev *ext = i2c_get_clientdata(client);

	return bmu_ext_device_exit(ext);
}
#else
static void bmu_ext_i2c_remove(struct i2c_client *client)
{
	struct bmu_ext_dev *ext = i2c_get_clientdata(client);

	bmu_ext_device_exit(ext);
}
#endif

static const struct i2c_device_id bmu_ext_i2c_id_table[] = {
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(i2c, bmu_ext_i2c_id_table);

static const struct acpi_device_id bmu_ext_i2c_acpi_match[] = {
	{ },
};
MODULE_DEVICE_TABLE(acpi, bmu_ext_i2c_acpi_match);

static struct i2c_driver bmu_ext_i2c_driver = {
	.driver		= {
		.name	= "bmu-ext-i2c",
		.of_match_table = of_match_ptr(bmu_ext_i2c_of_match_table),
		.acpi_match_table = ACPI_PTR(bmu_ext_i2c_acpi_match),
	},
	.probe		= bmu_ext_i2c_probe,
	.remove		= bmu_ext_i2c_remove,
	.id_table       = bmu_ext_i2c_id_table,
};

static int __init bmu_ext_i2c_init(void)
{
	int ret;

	ret = i2c_add_driver(&bmu_ext_i2c_driver);
	if (ret != 0) {
		PMIC_ERR("bmu_ext i2c registration failed %d\n", ret);
		return ret;
	}

	return 0;
}
subsys_initcall(bmu_ext_i2c_init);

static void __exit bmu_ext_i2c_exit(void)
{
	i2c_del_driver(&bmu_ext_i2c_driver);
}
module_exit(bmu_ext_i2c_exit);

MODULE_AUTHOR("Andrew F. Davis <afd@ti.com>");
MODULE_DESCRIPTION("bmu_ext I2C Interface Driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
