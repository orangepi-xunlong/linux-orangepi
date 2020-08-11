/*
 * Regulators driver for allwinnertech AXPDUMMY
 *
 * Copyright (C) 2014 allwinnertech Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/regmap.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/module.h>
#include <linux/power/axp_depend.h>
#include "../axp/axp-core.h"
#include "../axp/axp-regulator.h"
#include "axpdummy-regu.h"

/* Reverse engineered partly from Platformx drivers */
enum axp_regls {
	VCC_LDO1,
	VCC_LDO2,
	VCC_LDO3,
	VCC_LDO4,
	VCC_LDO5,
	VCC_LDO6,
	VCC_LDO7,
	VCC_LDO8,
	VCC_LDO9,
	VCC_LDO10,
	VCC_LDO11,
	VCC_LDO12,
	VCC_LDO13,
	VCC_LDO14,
	VCC_LDO15,
	VCC_LDO16,
	VCC_LDO17,
	VCC_LDO18,
	VCC_LDO19,
	VCC_LDO20,
	VCC_DUMMY_MAX,
};

struct axpdummy_regulators {
	struct regulator_dev *regulators[VCC_DUMMY_MAX];
	struct axp_dev *chip;
};

static struct axp_regulator_info axpdummy_regulator_info[] = {
	AXPDUMMY_REGU_INFO("axpdummy_ldo1",   VCC_LDO1),
	AXPDUMMY_REGU_INFO("axpdummy_ldo2",   VCC_LDO2),
	AXPDUMMY_REGU_INFO("axpdummy_ldo3",   VCC_LDO3),
	AXPDUMMY_REGU_INFO("axpdummy_ldo4",   VCC_LDO4),
	AXPDUMMY_REGU_INFO("axpdummy_ldo5",   VCC_LDO5),
	AXPDUMMY_REGU_INFO("axpdummy_ldo6",   VCC_LDO6),
	AXPDUMMY_REGU_INFO("axpdummy_ldo7",   VCC_LDO7),
	AXPDUMMY_REGU_INFO("axpdummy_ldo8",   VCC_LDO8),
	AXPDUMMY_REGU_INFO("axpdummy_ldo9",   VCC_LDO9),
	AXPDUMMY_REGU_INFO("axpdummy_ldo10", VCC_LDO10),
	AXPDUMMY_REGU_INFO("axpdummy_ldo11", VCC_LDO11),
	AXPDUMMY_REGU_INFO("axpdummy_ldo12", VCC_LDO12),
	AXPDUMMY_REGU_INFO("axpdummy_ldo13", VCC_LDO13),
	AXPDUMMY_REGU_INFO("axpdummy_ldo14", VCC_LDO14),
	AXPDUMMY_REGU_INFO("axpdummy_ldo15", VCC_LDO15),
	AXPDUMMY_REGU_INFO("axpdummy_ldo16", VCC_LDO16),
	AXPDUMMY_REGU_INFO("axpdummy_ldo17", VCC_LDO17),
	AXPDUMMY_REGU_INFO("axpdummy_ldo18", VCC_LDO18),
	AXPDUMMY_REGU_INFO("axpdummy_ldo19", VCC_LDO19),
	AXPDUMMY_REGU_INFO("axpdummy_ldo20", VCC_LDO20),
};

static struct regulator_init_data axp_regl_init_data[] = {
	REGU_INIT_DATA("axpdummy_ldo1"),
	REGU_INIT_DATA("axpdummy_ldo2"),
	REGU_INIT_DATA("axpdummy_ldo3"),
	REGU_INIT_DATA("axpdummy_ldo4"),
	REGU_INIT_DATA("axpdummy_ldo5"),
	REGU_INIT_DATA("axpdummy_ldo6"),
	REGU_INIT_DATA("axpdummy_ldo7"),
	REGU_INIT_DATA("axpdummy_ldo8"),
	REGU_INIT_DATA("axpdummy_ldo9"),
	REGU_INIT_DATA("axpdummy_ldo10"),
	REGU_INIT_DATA("axpdummy_ldo11"),
	REGU_INIT_DATA("axpdummy_ldo12"),
	REGU_INIT_DATA("axpdummy_ldo13"),
	REGU_INIT_DATA("axpdummy_ldo14"),
	REGU_INIT_DATA("axpdummy_ldo15"),
	REGU_INIT_DATA("axpdummy_ldo16"),
	REGU_INIT_DATA("axpdummy_ldo17"),
	REGU_INIT_DATA("axpdummy_ldo18"),
	REGU_INIT_DATA("axpdummy_ldo19"),
	REGU_INIT_DATA("axpdummy_ldo20"),
};

int axpdummy_ldo_count;

static s32 axpdummy_regu_dependence(const char *ldo_name)
{
	s32 axpdummy_dependence = 0;

	if (strcmp("axpdummy_ldo1", ldo_name) == 0)
		axpdummy_dependence |= AXPDUMMY_LDO1_BIT;
	else if (strcmp("axpdummy_ldo2", ldo_name) == 0)
		axpdummy_dependence |= AXPDUMMY_LDO2_BIT;
	else if (strcmp("axpdummy_ldo3", ldo_name) == 0)
		axpdummy_dependence |= AXPDUMMY_LDO3_BIT;
	else if (strcmp("axpdummy_ldo4", ldo_name) == 0)
		axpdummy_dependence |= AXPDUMMY_LDO4_BIT;
	else if (strcmp("axpdummy_ldo5", ldo_name) == 0)
		axpdummy_dependence |= AXPDUMMY_LDO5_BIT;
	else if (strcmp("axpdummy_ldo6", ldo_name) == 0)
		axpdummy_dependence |= AXPDUMMY_LDO6_BIT;
	else if (strcmp("axpdummy_ldo7", ldo_name) == 0)
		axpdummy_dependence |= AXPDUMMY_LDO7_BIT;
	else if (strcmp("axpdummy_ldo8", ldo_name) == 0)
		axpdummy_dependence |= AXPDUMMY_LDO8_BIT;
	else if (strcmp("axpdummy_ldo9", ldo_name) == 0)
		axpdummy_dependence |= AXPDUMMY_LDO9_BIT;
	else if (strcmp("axpdummy_ldo10", ldo_name) == 0)
		axpdummy_dependence |= AXPDUMMY_LDO10_BIT;
	else if (strcmp("axpdummy_ldo11", ldo_name) == 0)
		axpdummy_dependence |= AXPDUMMY_LDO11_BIT;
	else if (strcmp("axpdummy_ldo12", ldo_name) == 0)
		axpdummy_dependence |= AXPDUMMY_LDO12_BIT;
	else if (strcmp("axpdummy_ldo13", ldo_name) == 0)
		axpdummy_dependence |= AXPDUMMY_LDO13_BIT;
	else if (strcmp("axpdummy_ldo14", ldo_name) == 0)
		axpdummy_dependence |= AXPDUMMY_LDO14_BIT;
	else if (strcmp("axpdummy_ldo15", ldo_name) == 0)
		axpdummy_dependence |= AXPDUMMY_LDO15_BIT;
	else if (strcmp("axpdummy_ldo16", ldo_name) == 0)
		axpdummy_dependence |= AXPDUMMY_LDO16_BIT;
	else if (strcmp("axpdummy_ldo17", ldo_name) == 0)
		axpdummy_dependence |= AXPDUMMY_LDO17_BIT;
	else if (strcmp("axpdummy_ldo18", ldo_name) == 0)
		axpdummy_dependence |= AXPDUMMY_LDO18_BIT;
	else if (strcmp("axpdummy_ldo19", ldo_name) == 0)
		axpdummy_dependence |= AXPDUMMY_LDO19_BIT;
	else if (strcmp("axpdummy_ldo20", ldo_name) == 0)
		axpdummy_dependence |= AXPDUMMY_LDO20_BIT;
	else
		return -1;

	return axpdummy_dependence;
}

static int axpdummy_regulator_probe(struct platform_device *pdev)
{
	s32 i, ret = 0;
	struct axp_regulator_info *info;
	struct axpdummy_regulators *regu_data;
	struct axp_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);

	if (pdev->dev.of_node) {
		ret = axp_regulator_dt_parse(pdev->dev.of_node,
					axp_regl_init_data,
					axpdummy_regu_dependence);
		if (ret) {
			pr_err("%s parse device tree err\n", __func__);
			return -EINVAL;
		}
	} else {
		pr_err("axpdummy regulator device tree err!\n");
		return -EBUSY;
	}

	if (axp_get_ldo_count(pdev->dev.of_node, &axpdummy_ldo_count))
		return -EINVAL;

	regu_data = devm_kzalloc(&pdev->dev, sizeof(*regu_data),
					GFP_KERNEL);
	if (!regu_data)
		return -ENOMEM;

	regu_data->chip = axp_dev;
	platform_set_drvdata(pdev, regu_data);

	for (i = 0; i < axpdummy_ldo_count; i++) {
		info = &axpdummy_regulator_info[i];
		regu_data->regulators[i] = axp_regulator_register(
				&pdev->dev, axp_dev->regmap,
				&axp_regl_init_data[i], info);

		if (IS_ERR(regu_data->regulators[i])) {
			dev_err(&pdev->dev,
				"failed to register regulator %d\n", i);
			while (--i >= 0)
				axp_regulator_unregister(
					regu_data->regulators[i]);

			return -1;
		}
	}

	return 0;
}

static int axpdummy_regulator_remove(struct platform_device *pdev)
{
	struct axpdummy_regulators *regu_data = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < axpdummy_ldo_count; i++)
		regulator_unregister(regu_data->regulators[i]);

	return 0;
}

static const struct of_device_id axpdummy_regu_dt_ids[] = {
	{ .compatible = "axpdummy-regulator", },
	{},
};
MODULE_DEVICE_TABLE(of, axpdummy_regu_dt_ids);

static struct platform_driver axpdummy_regulator_driver = {
	.driver     = {
		.name   = "axpdummy-regulator",
		.of_match_table = axpdummy_regu_dt_ids,
	},
	.probe      = axpdummy_regulator_probe,
	.remove     = axpdummy_regulator_remove,
};

static int __init axpdummy_regulator_initcall(void)
{
	int ret;

	ret = platform_driver_register(&axpdummy_regulator_driver);
	if (IS_ERR_VALUE(ret)) {
		pr_err("%s: failed, errno %d\n", __func__, ret);
		return -EINVAL;
	}

	return 0;
}
subsys_initcall(axpdummy_regulator_initcall);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("<pannan@allwinnertech.com>");
MODULE_DESCRIPTION("Regulator Driver for axpdummy PMIC");
MODULE_ALIAS("platform:axpdummy-regulator");
