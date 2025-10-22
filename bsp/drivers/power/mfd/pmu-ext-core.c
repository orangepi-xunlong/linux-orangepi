/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * MFD core driver for the X-Powers' Power Management ICs
 *
 * AXP20x typically comprises an adaptive USB-Compatible PWM charger, BUCK DC-DC
 * converters, LDOs, multiple 12-bit ADCs of voltage, current and temperature
 * as well as configurable GPIOs.
 *
 * This file contains the interface independent core functions.
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
#include "pmu-ext.h"

static const char *const pmu_ext_model_names[] = {
	"TCS4838", "SY8827G", "AXP1530", "AW37501", "OCP2131"
};

#define PMU_EXT_DCDC0 "dcdc0"
#define PMU_EXT_DCDC1 "dcdc1"

#define PMU_EXT_AXP1530_DCDC1 "dcdc1"
#define PMU_EXT_AXP1530_DCDC2 "dcdc2"
#define PMU_EXT_AXP1530_DCDC3 "dcdc3"
#define PMU_EXT_AXP1530_ALDO1 "aldo1"
#define PMU_EXT_AXP1530_DLDO1 "dldo1"

static struct mfd_cell axp1530_ext_cells[] = {
	{ .name = "pmu-ext-regulator", },
	{
		.of_compatible = "xpower-vregulator,ext-dcdc1",
		.name = "reg-virt-consumer",
		.id = PLATFORM_DEVID_AUTO,
		.platform_data = PMU_EXT_AXP1530_DCDC1,
		.pdata_size = sizeof(PMU_EXT_AXP1530_DCDC1),

	},
	{
		.of_compatible = "xpower-vregulator,ext-dcdc2",
		.name = "reg-virt-consumer",
		.id = PLATFORM_DEVID_AUTO,
		.platform_data = PMU_EXT_AXP1530_DCDC2,
		.pdata_size = sizeof(PMU_EXT_AXP1530_DCDC2),
	},
	{
		.of_compatible = "xpower-vregulator,ext-dcdc3",
		.name = "reg-virt-consumer",
		.id = PLATFORM_DEVID_AUTO,
		.platform_data = PMU_EXT_AXP1530_DCDC3,
		.pdata_size = sizeof(PMU_EXT_AXP1530_DCDC3),
	},
	{
		.of_compatible = "xpower-vregulator,ext-aldo1",
		.name = "reg-virt-consumer",
		.id = PLATFORM_DEVID_AUTO,
		.platform_data = PMU_EXT_AXP1530_ALDO1,
		.pdata_size = sizeof(PMU_EXT_AXP1530_ALDO1),
	},
	{
		.of_compatible = "xpower-vregulator,ext-dldo1",
		.name = "reg-virt-consumer",
		.id = PLATFORM_DEVID_AUTO,
		.platform_data = PMU_EXT_AXP1530_DLDO1,
		.pdata_size = sizeof(PMU_EXT_AXP1530_DLDO1),
	},
};

static struct mfd_cell tcs4838_cells[] = {
	{ .name = "pmu-ext-regulator", },
	{
		.of_compatible = "xpower-vregulator,ext-dcdc0",
		.name = "reg-virt-consumer",
		.id = PLATFORM_DEVID_AUTO,
		.platform_data = PMU_EXT_DCDC0,
		.pdata_size = sizeof(PMU_EXT_DCDC0),

	},
	{
		.of_compatible = "xpower-vregulator,ext-dcdc1",
		.name = "reg-virt-consumer",
		.id = PLATFORM_DEVID_AUTO,
		.platform_data = PMU_EXT_DCDC1,
		.pdata_size = sizeof(PMU_EXT_DCDC1),

	},
};

static struct mfd_cell sy8827g_cells[] = {
	{ .name = "pmu-ext-regulator", },
	{
		.of_compatible = "xpower-vregulator,ext-dcdc0",
		.name = "reg-virt-consumer",
		.id = PLATFORM_DEVID_AUTO,
		.platform_data = PMU_EXT_DCDC0,
		.pdata_size = sizeof(PMU_EXT_DCDC0),

	},
	{
		.of_compatible = "xpower-vregulator,ext-dcdc1",
		.name = "reg-virt-consumer",
		.id = PLATFORM_DEVID_AUTO,
		.platform_data = PMU_EXT_DCDC1,
		.pdata_size = sizeof(PMU_EXT_DCDC1),

	},
};

#define PMU_EXT_AW37501 "ldo"

static struct mfd_cell aw37501_cells[] = {
	{
		.name = "aw37501-regulator",
		.of_compatible = "ext,aw37501-regulator",
	},
	{
		.of_compatible = "xpower-vregulator,ext-ldo",
		.name = "reg-virt-consumer",
		.id = PLATFORM_DEVID_AUTO,
		.platform_data = PMU_EXT_AW37501,
		.pdata_size = sizeof(PMU_EXT_AW37501),

	},
};

#define PMU_EXT_OCP2131_AVDD "avdd"
#define PMU_EXT_OCP2131_AVEE "avee"
static struct mfd_cell ocp2131_cells[] = {
	{
		.of_compatible = "ext,ocp2131-regulator",
		.name = "ocp2131-regulator",
	},
	{
		.of_compatible = "xpower-vregulator,ext-avdd",
		.name = "reg-virt-consumer",
		.id = PLATFORM_DEVID_AUTO,
		.platform_data = PMU_EXT_OCP2131_AVDD,
		.pdata_size = sizeof(PMU_EXT_OCP2131_AVDD),
	},
	{
		.of_compatible = "xpower-vregulator,ext-avee",
		.name = "reg-virt-consumer",
		.id = PLATFORM_DEVID_AUTO,
		.platform_data = PMU_EXT_OCP2131_AVEE,
		.pdata_size = sizeof(PMU_EXT_OCP2131_AVEE),
	},
};

/* For AXP323/AXP1530 */
static const struct regmap_range axp1530_writeable_ranges[] = {
	regmap_reg_range(AXP1530_ON_INDICATE, AXP1530_END),
};

static const struct regmap_range axp1530_volatile_ranges[] = {
	regmap_reg_range(AXP1530_ON_INDICATE, AXP1530_IC_TYPE),
	regmap_reg_range(AXP1530_POWER_STATUS, AXP1530_END),
};

static const struct regmap_access_table axp1530_writeable_table = {
	.yes_ranges	= axp1530_writeable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp1530_writeable_ranges),
};

static const struct regmap_access_table axp1530_volatile_table = {
	.yes_ranges	= axp1530_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp1530_volatile_ranges),
};

static const struct regmap_range tcs4838_volatile_ranges[] = {
	regmap_reg_range(TCS4838_CTRL, TCS4838_PGOOD),
};

static const struct regmap_access_table tcs4838_volatile_table = {
	.yes_ranges = tcs4838_volatile_ranges,
	.n_yes_ranges = ARRAY_SIZE(tcs4838_volatile_ranges),
};

static const struct regmap_range sy8827g_volatile_ranges[] = {
	regmap_reg_range(SY8827G_CTRL, SY8827G_PGOOD),
};

static const struct regmap_access_table sy8827g_volatile_table = {
	.yes_ranges = sy8827g_volatile_ranges,
	.n_yes_ranges = ARRAY_SIZE(sy8827g_volatile_ranges),
};

static const struct regmap_range aw37501_volatile_ranges[] = {
	regmap_reg_range(AW37501_OUTPUT_EN, AW37501_WRITE_PROTECT),
};

static const struct regmap_access_table aw37501_volatile_table = {
	.yes_ranges = aw37501_volatile_ranges,
	.n_yes_ranges = ARRAY_SIZE(aw37501_volatile_ranges),
};

static const struct regmap_range ocp2131_writeable_ranges[] = {
	regmap_reg_range(OCP2131_POS_OUTPUT_AVDD, OCP2131_NEG_OUTPUT_AVEE),
};

static const struct regmap_access_table ocp2131_writeable_table = {
	.yes_ranges = ocp2131_writeable_ranges,
	.n_yes_ranges = ARRAY_SIZE(ocp2131_writeable_ranges),
};

/* For AXP323/AXP1530 */
static const struct regmap_config axp1530_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.wr_table	= &axp1530_writeable_table,
	.volatile_table	= &axp1530_volatile_table,
	.max_register	= AXP1530_FREQUENCY,
	.use_single_read = true,
	.use_single_write = true,
	.cache_type	= REGCACHE_RBTREE,
};

static const struct regmap_config tcs4838_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.volatile_table = &tcs4838_volatile_table,
	.max_register   = TCS4838_PGOOD,
	.use_single_read = true,
	.use_single_write = true,
	.cache_type     = REGCACHE_RBTREE,
};

static const struct regmap_config sy8827g_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.volatile_table = &sy8827g_volatile_table,
	.max_register   = SY8827G_PGOOD,
	.use_single_read = true,
	.use_single_write = true,
	.cache_type     = REGCACHE_RBTREE,
};

static const struct regmap_config aw37501_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.volatile_table = &aw37501_volatile_table,
	.max_register   = AW37501_WRITE_PROTECT,
	.use_single_read = true,
	.use_single_write = true,
	.cache_type     = REGCACHE_RBTREE,
};

static const struct regmap_config ocp2131_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.wr_table = &ocp2131_writeable_table,
	.max_register   = OCP2131_NEG_OUTPUT_AVEE,
	.use_single_read = true,
	.use_single_write = true,
	.cache_type     = REGCACHE_RBTREE,
};

static void axp1530_dts_parse(struct pmu_ext_dev *ext)
{
}

static void tcs4838_dts_parse(struct pmu_ext_dev *ext)
{
	struct device_node *node = ext->dev->of_node;
	struct regmap *map = ext->regmap;
	u32 val;

	/* init powerok reset function */
	if (of_property_read_u32(node, "tcs4838_delay", &val))
		val = 0;
	if (val) {
		val = val << 4;
		regmap_update_bits(map, TCS4838_CTRL, GENMASK(6, 4), val);
	}
}

static void sy8827g_dts_parse(struct pmu_ext_dev *ext)
{
	struct device_node *node = ext->dev->of_node;
	struct regmap *map = ext->regmap;
	u32 val;

	/* init powerok reset function */
	if (of_property_read_u32(node, "sy8827g_delay", &val))
		val = 0;
	if (val) {
		val = val << 4;
		regmap_update_bits(map, SY8827G_CTRL, GENMASK(6, 4), val);
	}
}

int pmu_ext_match_device(struct pmu_ext_dev *ext)
{
	struct device *dev = ext->dev;
	const struct acpi_device_id *acpi_id;
	const struct of_device_id *of_id;

	if (dev->of_node) {
		of_id = of_match_device(dev->driver->of_match_table, dev);
		if (!of_id) {
			PMIC_DEV_ERR(dev, "Unable to match OF ID\n");
			return -ENODEV;
		}
		ext->variant = (long)of_id->data;
	} else {
		acpi_id = acpi_match_device(dev->driver->acpi_match_table, dev);
		if (!acpi_id || !acpi_id->driver_data) {
			PMIC_DEV_ERR(dev, "Unable to match ACPI ID and data\n");
			return -ENODEV;
		}
		ext->variant = (long)acpi_id->driver_data;
	}

	switch (ext->variant) {
/**************************************/
	case TCS4838_ID:
		ext->nr_cells = ARRAY_SIZE(tcs4838_cells);
		ext->cells = tcs4838_cells;
		ext->regmap_cfg = &tcs4838_regmap_config;
		ext->dts_parse = tcs4838_dts_parse;
		break;
/**************************************/
	case SY8827G_ID:
		ext->nr_cells = ARRAY_SIZE(sy8827g_cells);
		ext->cells = sy8827g_cells;
		ext->regmap_cfg = &sy8827g_regmap_config;
		ext->dts_parse = sy8827g_dts_parse;
		break;
/**************************************/
	case AXP1530_ID:
		ext->nr_cells = ARRAY_SIZE(axp1530_ext_cells);
		ext->cells = axp1530_ext_cells;
		ext->regmap_cfg = &axp1530_regmap_config;
		ext->dts_parse = axp1530_dts_parse;
		break;
/**************************************/
	case AW37501_ID:
		ext->nr_cells = ARRAY_SIZE(aw37501_cells);
		ext->cells = aw37501_cells;
		ext->regmap_cfg = &aw37501_regmap_config;
		break;
/**************************************/
	case OCP2131_ID:
		ext->nr_cells = ARRAY_SIZE(ocp2131_cells);
		ext->cells = ocp2131_cells;
		ext->regmap_cfg = &ocp2131_regmap_config;
		break;
	default:
		PMIC_DEV_ERR(dev, "unsupported ext ID %lu\n", ext->variant);
		return -EINVAL;
	}
	PMIC_DEV_INFO(dev, "pmu_ext_dev variant %s found\n",
		 pmu_ext_model_names[ext->variant]);

	return 0;
}
EXPORT_SYMBOL(pmu_ext_match_device);

int pmu_ext_device_init(struct pmu_ext_dev *ext)
{
	int ret;
	if (ext->dts_parse)
		ext->dts_parse(ext);
	ret = mfd_add_devices(ext->dev, 0, ext->cells,
			      ext->nr_cells, NULL, 0, NULL);

	PMIC_DEV_INFO(ext->dev, "pmu_ext:%s driver loaded\n", pmu_ext_model_names[ext->variant]);

	return 0;
}
EXPORT_SYMBOL_GPL(pmu_ext_device_init);

int pmu_ext_device_exit(struct pmu_ext_dev *ext)
{
	mfd_remove_devices(ext->dev);
	return 0;
}
EXPORT_SYMBOL_GPL(pmu_ext_device_exit);

MODULE_AUTHOR("Andrew F. Davis <afd@ti.com>");
MODULE_DESCRIPTION("pmu_ext MFD Driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
