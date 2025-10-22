/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Regulator driver for TI TCS4838x PMICs
 *
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 *	Andrew F. Davis <afd@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether expressed or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 * Based on the TPS65218 driver and the previous TCS4838 driver by
 * Margarita Olaya Cabrera <magi@slimlogic.co.uk>
 */

#include "sunxi-power-regulator.h"
#include "bmu-ext.h"

static int regulator_axp519_is_enabled_regmap(struct regulator_dev *rdev)
{
	unsigned int val;
	int ret;

	ret = regmap_read(rdev->regmap, AXP519_WORK_CFG, &val);
	if (ret)
		return ret;

	ret = (val &= BIT(0)) && (!(val &= BIT(4)));

	return ret;
}

static int regulator_axp519_enable_regmap(struct regulator_dev *rdev)
{
	int ret;

	ret = regmap_update_bits(rdev->regmap, AXP519_WORK_CFG, BIT(4), 0);
	if (ret)
		return ret;

	ret = regmap_update_bits(rdev->regmap, AXP519_WORK_CFG, BIT(0), BIT(0));
	if (ret)
		return ret;

	return 0;
}

static int regulator_axp519_disable_regmap(struct regulator_dev *rdev)
{
	int ret;

	ret = regmap_update_bits(rdev->regmap, AXP519_WORK_CFG, BIT(4), BIT(4));
	if (ret)
		return ret;

	ret = regmap_update_bits(rdev->regmap, AXP519_WORK_CFG, BIT(0), 0);
	if (ret)
		return ret;

	return 0;
}

static struct regulator_ops bmu_ext_drivvbus_ops = {
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= regulator_is_enabled_regmap,
};

static struct regulator_ops bmu_ext_axp519_drivvbus_ops = {
	.enable			= regulator_axp519_enable_regmap,
	.disable		= regulator_axp519_disable_regmap,
	.is_enabled		= regulator_axp519_is_enabled_regmap,
};

static const struct regulator_desc eta6973_drivevbus_regulator = {
	.name			= "drivevbus",
	.supply_name		= "drivevbusin",
	.of_match		= of_match_ptr("drivevbus"),
	.regulators_node	= of_match_ptr("regulators"),
	.type			= REGULATOR_VOLTAGE,
	.owner			= THIS_MODULE,
	.enable_reg		= ETA6973_REG_01,
	.enable_mask		= ETA6973_REG01_OTG_CONFIG_MASK,
	.ops			= &bmu_ext_drivvbus_ops,
};

static const struct regulator_desc axp519_drivevbus_regulator = {
	.name			= "drivevbus",
	.supply_name		= "drivevbusin",
	.of_match		= of_match_ptr("drivevbus"),
	.regulators_node =	of_match_ptr("regulators"),
	.type			= REGULATOR_VOLTAGE,
	.owner			= THIS_MODULE,
	.ops			= &bmu_ext_axp519_drivvbus_ops,
};

static int bmu_ext_regulator_probe(struct platform_device *pdev)
{
	struct regulator_dev *rdev;
	struct bmu_ext_dev *ext = dev_get_drvdata(pdev->dev.parent);
	bool drivevbus = of_property_read_bool(pdev->dev.parent->of_node, "quick-charge,drive-vbus-en");
	struct regulator_config config = {
		.dev = pdev->dev.parent,
		.regmap = ext->regmap,
		.driver_data = ext,
	};

	if (drivevbus) {
		switch (ext->variant) {
		case ETA6973_ID:
			rdev = devm_regulator_register(&pdev->dev,
						       &eta6973_drivevbus_regulator,
						       &config);
			break;
		case AXP519_ID:
			rdev = devm_regulator_register(&pdev->dev,
						       &axp519_drivevbus_regulator,
						       &config);
			break;
		default:
			PMIC_DEV_ERR(&pdev->dev, "EXT variant: %ld unsupported drivevbus\n",
				ext->variant);
		}
		if (IS_ERR(rdev)) {
			PMIC_DEV_ERR(&pdev->dev, "Failed to register drivevbus\n");
			return PTR_ERR(rdev);
		}
	} else {
		return -ENODEV;
	}
	return 0;
}

static int bmu_ext_regulator_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver bmu_ext_regulator_driver = {
	.driver = {
		.name = "bmu-ext-regulator",
	},
	.probe = bmu_ext_regulator_probe,
	.remove	= bmu_ext_regulator_remove,
};

static int __init bmu_ext_regulator_init(void)
{
	return platform_driver_register(&bmu_ext_regulator_driver);
}

static void __exit bmu_ext_regulator_exit(void)
{
	platform_driver_unregister(&bmu_ext_regulator_driver);
}

subsys_initcall(bmu_ext_regulator_init);
module_exit(bmu_ext_regulator_exit);

MODULE_AUTHOR("Andrew F. Davis <afd@ti.com>");
MODULE_DESCRIPTION("bmu_ext voltage regulator driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
