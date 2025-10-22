/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Regulator driver for SOC PMU's
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
 */

#include "sunxi-power-regulator.h"
#include "sunxi-soc-pmu.h"

/* Operations permitted on DCDCx */
static struct regulator_ops soc_pmu_ops_range_vol_delay = {
	.is_enabled		= regulator_is_enabled_regmap,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.list_voltage		= regulator_list_voltage_linear_range,
};

#define SOC_PMU_REGULATOR_RANGE_VOL_DELAY(_family, _id, _match, _supply, _ranges, _n_voltages,	\
			_vreg, _vmask, _ereg, _emask)						\
	[_family##_##_id] = {									\
		.name		= (_match),							\
		.supply_name	= (_supply),							\
		.of_match	= of_match_ptr(_match),						\
		.type		= REGULATOR_VOLTAGE,						\
		.id		= _family##_##_id,						\
		.n_voltages	= (_n_voltages),						\
		.owner		= THIS_MODULE,							\
		.vsel_reg	= (_vreg),							\
		.vsel_mask	= (_vmask),							\
		.enable_reg	= (_ereg),							\
		.enable_mask	= (_emask),							\
		.linear_ranges	= (_ranges),							\
		.n_linear_ranges = ARRAY_SIZE(_ranges),						\
		.ops		= &soc_pmu_ops_range_vol_delay,					\
	}

static const struct regulator_linear_range v821_ldo1_ranges[] = {
	REGULATOR_LINEAR_RANGE(2250000, 0x0, 0xF0, 50000),
};

static const struct regulator_desc v821_pmu_regulators[] = {
	SOC_PMU_REGULATOR_RANGE_VOL_DELAY(V821, LDO1, "ldo1", "ldo1", v821_ldo1_ranges,
			0x10, V821_LDO_2V8_CTRL, GENMASK(7, 4), V821_LDO_2V8_CTRL, BIT(0)),
};

/* ------------------------------ */
/* For V821 */
static const struct regmap_range v821_soc_pmu_volatile_ranges[] = {
	regmap_reg_range(V821_WAKE_IO_CTRL, V821_POWER_CTRL_END),
};

static const struct regmap_access_table v821_soc_pmu_volatile_table = {
	.yes_ranges	= v821_soc_pmu_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(v821_soc_pmu_volatile_ranges),
};

static const struct regmap_config v821_soc_pmu_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.volatile_table	= &v821_soc_pmu_volatile_table,
	.max_register	= V821_POWER_CTRL_END,
	.use_single_read = true,
	.use_single_write = true,
	.cache_type	= REGCACHE_RBTREE,
};
/* ------------------------------ */

static int soc_pmu_regulator_probe(struct platform_device *pdev)
{
	const struct regulator_desc *regulators;
	const struct acpi_device_id *acpi_id;
	const struct of_device_id *of_id;
	struct soc_pmu_dev *soc_pmu;
	struct regulator_dev *rdev;
	struct device *dev = &pdev->dev;
	struct device_node *np;
	struct regulator_config config = {
		.dev = &pdev->dev,
	};
	int i, nregulators;
	void __iomem *base;

	soc_pmu = devm_kzalloc(&pdev->dev, sizeof(*soc_pmu), GFP_KERNEL);
	if (!soc_pmu)
		return -ENOMEM;

	soc_pmu->dev = &pdev->dev;
	dev_set_drvdata(soc_pmu->dev, soc_pmu);

	if (dev->of_node) {
		of_id = of_match_device(dev->driver->of_match_table, dev);
		if (!of_id) {
			PMIC_DEV_ERR(dev, "Unable to match OF ID\n");
			return -ENODEV;
		}
		np = of_node_get(dev->of_node);
		if (!np)
			return 0;
		config.driver_data = soc_pmu;
		soc_pmu->variant = (long)of_id->data;
	} else {
		acpi_id = acpi_match_device(dev->driver->acpi_match_table, dev);
		if (!acpi_id || !acpi_id->driver_data) {
			PMIC_DEV_ERR(dev, "Unable to match ACPI ID and data\n");
			return -ENODEV;
		}
		soc_pmu->variant = (long)acpi_id->driver_data;
	}

	switch (soc_pmu->variant) {
	case V821_ID:
		soc_pmu->regmap_cfg = &v821_soc_pmu_regmap_config;
		regulators = v821_pmu_regulators;
		nregulators = V821_PMU_REG_ID_MAX;
		break;
	default:
		PMIC_DEV_ERR(&pdev->dev, "Unsupported soc_pmu variant: %ld\n",
			soc_pmu->variant);
		return -EINVAL;
	}

	base = devm_platform_ioremap_resource(pdev, 0);
	soc_pmu->regmap = devm_regmap_init_sunxi_soc_pmu(&pdev->dev, base, soc_pmu->regmap_cfg);

	config.regmap = soc_pmu->regmap;
	config.driver_data = soc_pmu;

	for (i = 0; i < nregulators; i++) {
		const struct regulator_desc *desc = &regulators[i];
		if (dev->of_node) {
			config.of_node = of_get_child_by_name(np, regulators[i].name);
		}
		rdev = devm_regulator_register(&pdev->dev, desc, &config);
		if (IS_ERR(rdev)) {
			PMIC_DEV_ERR(&pdev->dev, "Failed to register %s\n",
				regulators[i].name);
			return PTR_ERR(rdev);
		}
	}

	return 0;
}

static int soc_pmu_regulator_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id sunxi_soc_pmu_of_match[] = {
	{ .compatible = "sunxi-soc-pmu,V821", .data = (void *)V821_ID },
	{ },
};

MODULE_DEVICE_TABLE(of, sunxi_soc_pmu_of_match);

static struct platform_driver soc_pmu_regulator_driver = {
	.driver = {
		.name = "sunxi-soc-regulator",
		.of_match_table	= of_match_ptr(sunxi_soc_pmu_of_match),
	},
	.probe			= soc_pmu_regulator_probe,
	.remove			= soc_pmu_regulator_remove,
};

static int __init soc_pmu_regulator_init(void)
{
	return platform_driver_register(&soc_pmu_regulator_driver);
}

static void __exit soc_pmu_regulator_exit(void)
{
	platform_driver_unregister(&soc_pmu_regulator_driver);
}

subsys_initcall(soc_pmu_regulator_init);
module_exit(soc_pmu_regulator_exit);

MODULE_AUTHOR("Andrew F. Davis <afd@ti.com>");
MODULE_DESCRIPTION("soc_pmu voltage regulator driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
