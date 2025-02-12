// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Regulator driver for Ky PMIC
 *
 * Copyright (c) 2023, KY Co., Ltd
 *
 */

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/gpio/consumer.h>
#include <linux/mfd/ky/ky_pmic.h>

static struct regulator_match_data *match_data;

static int ky_regulator_set_suspend_voltage(struct regulator_dev *rdev, int uV)
{
	unsigned int reg;

	int sel = regulator_map_voltage_linear_range(rdev, uV, uV);

	if (sel < 0)
		return -EINVAL;

	/* means that we will disable this vol in suspend */
	if (uV == rdev->constraints->max_uV)
		/* BUCK will set 0xff to close the power */
		sel = rdev->desc->vsel_mask;
	else if (uV == rdev->constraints->min_uV)
		/* LDO will set zero to close the power */
		sel = 0;

	reg = rdev->desc->vsel_reg + match_data->sleep_reg_offset;

	return regmap_update_bits(rdev->regmap, reg,
			rdev->desc->vsel_mask, sel);
}

static const struct regulator_ops pmic_dcdc_ldo_ops = {
	.list_voltage		= regulator_list_voltage_linear_range,
	.map_voltage		= regulator_map_voltage_linear_range,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.set_voltage_time_sel	= regulator_set_voltage_time_sel,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= regulator_is_enabled_regmap,
	.set_suspend_voltage	= ky_regulator_set_suspend_voltage,
};

static const struct regulator_ops pmic_switch_ops = {
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= regulator_is_enabled_regmap,
};

SPM8821_BUCK_LINER_RANGE;SPM8821_LDO_LINER_RANGE;SPM8821_REGULATOR_DESC;
SPM8821_REGULATOR_MATCH_DATA;

PM853_BUCK_LINER_RANGE1;PM853_BUCK_LINER_RANGE2;PM853_LDO_LINER_RANGE1;
PM853_LDO_LINER_RANGE2;PM853_LDO_LINER_RANGE3;PM853_LDO_LINER_RANGE4;
PM853_REGULATOR_DESC;PM853_REGULATOR_MATCH_DATA;

SY8810L_BUCK_LINER_RANGE;SY8810L_REGULATOR_DESC;SY8810L_REGULATOR_MATCH_DATA;

static const struct of_device_id ky_regulator_of_match[] = {
	{ .compatible = "pmic,regulator,spm8821", .data = (void *)&spm8821_regulator_match_data },
	{ .compatible = "pmic,regulator,pm853", .data = (void *)&pm853_regulator_match_data },
	{ .compatible = "pmic,regulator,sy8810l", .data = (void *)&sy8810l_regulator_match_data },
	{ },
};
MODULE_DEVICE_TABLE(of, ky_regulator_of_match);

static int ky_regulator_probe(struct platform_device *pdev)
{
	struct regulator_config config = {};
	struct ky_pmic *pmic = dev_get_drvdata(pdev->dev.parent);
	struct i2c_client *client;
	const struct of_device_id *of_id;
	struct regulator_dev *regulator_dev;
	int i;

	of_id = of_match_device(ky_regulator_of_match, &pdev->dev);
	if (!of_id) {
		pr_err("Unable to match OF ID\n");
		return -ENODEV;
	}

	match_data = (struct regulator_match_data *)of_id->data;

	client = pmic->i2c;
	config.dev = &client->dev;
	config.regmap = pmic->regmap;

	if (strcmp(match_data->name, "pm853") == 0) {
		client = pmic->sub->power_page;
		config.dev = &pmic->i2c->dev;
		config.regmap = pmic->sub->power_regmap;	
	}

	for (i = 0; i < match_data->nr_desc; ++i) {
		regulator_dev = devm_regulator_register(&pdev->dev,
				match_data->desc + i, &config);
		if (IS_ERR(regulator_dev)) {
			pr_err("failed to register %d regulator\n", i);
			return PTR_ERR(regulator_dev);
		}
	}

	return 0;
}

static struct platform_driver ky_regulator_driver = {
	.probe = ky_regulator_probe,
	.driver = {
		.name = "ky-regulator",
		.of_match_table = ky_regulator_of_match,
	},
};

static int ky_regulator_init(void)
{
	return platform_driver_register(&ky_regulator_driver);
}
subsys_initcall(ky_regulator_init);

MODULE_DESCRIPTION("regulator drivers for the Ky series PMICs");

