// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 InvenSense, Inc.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/spi/spi.h>
#include <linux/regmap.h>
#include <linux/property.h>
#include <linux/pinctrl/consumer.h>

#include "inv_icm42600.h"

static int inv_icm42600_spi_bus_setup(struct inv_icm42600_state *st)
{
	unsigned int mask, val;
	int ret;

	/* Configure SPI */
	ret = regmap_write(st->map, INV_ICM42600_REG_DEVICE_CONFIG, INV_ICM42600_SPI_AP_4WIRE);
	if (ret)
		return ret;

	/* set slew rates for I2C and SPI */
	mask = INV_ICM42600_DRIVE_CONFIG_I2C_MASK;
	val = INV_ICM42600_DRIVE_CONFIG_I2C(INV_ICM42600_SLEW_RATE_20_60NS);
	ret = regmap_update_bits(st->map, INV_ICM42600_REG_DRIVE_CONFIG2,
				 mask, val);

	mask = INV_ICM42600_DRIVE_CONFIG_SPI_MASK;
	val = INV_ICM42600_DRIVE_CONFIG_SPI(INV_ICM42600_SLEW_RATE_12_36NS);
	ret = regmap_update_bits(st->map, INV_ICM42600_REG_DRIVE_CONFIG3,
				 mask, val);
	if (ret)
		return ret;

	/* disable i2c bus */
	return regmap_update_bits(st->map, INV_ICM42600_REG_INTF_CONFIG0,
				  INV_ICM42600_INTF_CONFIG0_UI_SIFS_CFG_MASK,
				  INV_ICM42600_INTF_CONFIG0_UI_SIFS_CFG_I2C_DIS);
}

static int inv_icm42600_probe(struct spi_device *spi)
{
	const void *match;
	enum inv_icm42600_chip chip;
	struct regmap *regmap;

	match = device_get_match_data(&spi->dev);
	if (!match)
		return -EINVAL;
	chip = (uintptr_t)match;

	regmap = devm_regmap_init_spi(spi, &inv_icm42600_regmap_config);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	pinctrl_pm_select_default_state(&spi->dev);

	return inv_icm42600_core_probe(regmap, chip, spi->irq,
				       inv_icm42600_spi_bus_setup);
}

static const struct of_device_id inv_icm42600_of_matches[] = {
	{
		.compatible = "invensense,icm42600",
		.data = (void *)INV_CHIP_ICM42600,
	}, {
		.compatible = "invensense,icm42602",
		.data = (void *)INV_CHIP_ICM42602,
	}, {
		.compatible = "invensense,icm42605",
		.data = (void *)INV_CHIP_ICM42605,
	}, {
		.compatible = "invensense,icm42607",
		.data = (void *)INV_CHIP_ICM42607,
	}, {
		.compatible = "invensense,icm42622",
		.data = (void *)INV_CHIP_ICM42622,
	}, {
		.compatible = "invensense,icm42670",
		.data = (void *)INV_CHIP_ICM42670,
	},
	{}
};
MODULE_DEVICE_TABLE(of, inv_icm42600_of_matches);

static const struct spi_device_id inv_icm42600_spi_id_table[] = {
	{ INV_CHIP_ICM42600_DEV_NAME, INV_CHIP_ICM42600 },
	{ INV_CHIP_ICM42602_DEV_NAME, INV_CHIP_ICM42602 },
	{ INV_CHIP_ICM42605_DEV_NAME, INV_CHIP_ICM42605 },
	{ INV_CHIP_ICM42607_DEV_NAME, INV_CHIP_ICM42607 },
	{ INV_CHIP_ICM42622_DEV_NAME, INV_CHIP_ICM42622 },
	{ INV_CHIP_ICM42670_DEV_NAME, INV_CHIP_ICM42670 },
	{},
};
MODULE_DEVICE_TABLE(spi, inv_icm42600_spi_id_table);

static struct spi_driver inv_icm42600_driver = {
	.driver = {
		.name = "inv-icm42600-spi",
		.of_match_table = inv_icm42600_of_matches,
		.pm = &inv_icm42600_pm_ops,
	},
	.probe = inv_icm42600_probe,
	.id_table = inv_icm42600_spi_id_table,
};
module_spi_driver(inv_icm42600_driver);

MODULE_AUTHOR("InvenSense, Inc.");
MODULE_DESCRIPTION("InvenSense ICM-426xx SPI driver");
MODULE_LICENSE("GPL");
