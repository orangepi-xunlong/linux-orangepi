/*
 * Regulators driver for allwinnertech AXP20
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
#include "../axp-core.h"
#include "../axp-regulator.h"
#include "axp20.h"
#include "axp20-regu.h"

/* Reverse engineered partly from Platformx drivers */
enum AXP_REGLS {
	VCC_DCDC2,
	VCC_DCDC3,
	VCC_LDO1,
	VCC_LDO2,
	VCC_LDO3,
	VCC_LDO4,
	VCC_LDOIO0,
	VCC_20_MAX,
};

struct axp20_regulators {
	struct regulator_dev *regulators[VCC_20_MAX];
	struct axp_dev *chip;
};

static const int axp20_ldo4_table[] = {
	1250, 1300, 1400, 1500,
	1600, 1700, 1800, 1900,
	2000, 2500, 2700, 2800,
	3000, 3100, 3200, 3300
};

#define AXP20_LDO(_id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, freq_addr, dvm_ereg, dvm_ebit, dvm_flag) \
	AXP_LDO(AXP20, _id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, freq_addr, dvm_ereg, dvm_ebit, dvm_flag)

#define AXP20_LDO_SEL(_id, min, max, vreg, shift, nbits,\
		ereg, emask, enval, disval, vtable,\
		mode_addr, freq_addr, dvm_ereg, dvm_ebit, dvm_flag) \
	AXP_LDO_SEL(AXP20, _id, min, max, vreg, shift, nbits,\
		ereg, emask, enval, disval, vtable,\
		mode_addr, freq_addr, dvm_ereg, dvm_ebit, dvm_flag)

#define AXP20_DCDC(_id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, mode_bit, freq_addr, dvm_ereg, dvm_ebit, dvm_flag) \
	AXP_DCDC(AXP20, _id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, mode_bit, freq_addr, dvm_ereg, dvm_ebit, dvm_flag)

static struct axp_regulator_info axp20_regulator_info[] = {
	AXP20_DCDC(2,       700, 2275, 25,   DCDC2, 0, 6,  DCDC2EN, 0x10,
		0x10,    0,      0, 0, 0,  0x80, 0x04, 0x37, 0x25, 2, 0),
	AXP20_DCDC(3,       700, 3500, 25,   DCDC3, 0, 7,  DCDC3EN, 0x02,
		0x02,    0,      0, 0, 0,  0x80, 0x02, 0x37, 0, 0, 0),
	AXP20_LDO(1,       1300, 1300,  0,     RTC, 0, 0, RTCLDOEN, 0x01,
		0x01,    0,      0, 0, 0,     0,    0,    0, 0, 0),
	AXP20_LDO(2,       1800, 3300, 100,   LDO2, 4, 4,   LDO2EN, 0x04,
		0x04,    0,      0, 0, 0,     0,    0,    0, 0, 0),
	AXP20_LDO(3,        700, 3500,  25,   LDO3, 0, 7,   LDO3EN, 0x40,
		0x40,    0,      0, 0, 0,     0,    0,    0, 0, 0),
	AXP20_LDO_SEL(4,   1250, 3300,        LDO4, 0, 4,   LDO4EN, 0x08,
		0x08,    0,   axp20_ldo4,     0,    0,    0, 0, 0),
	AXP20_LDO(IO0,     1800, 3300, 100, LDOIO0, 4, 4, LDOIO0EN, 0x07,
		0x03,    0,      0, 0, 0,     0,    0,    0, 0, 0),
};

static struct regulator_init_data axp_regl_init_data[] = {
	[VCC_DCDC2] = {
		.constraints = {
			.name = "axp20_dcdc2",
			.min_uV = 700000,
			.max_uV = 2275000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_DCDC3] = {
		.constraints = {
			.name = "axp20_dcdc3",
			.min_uV = 700000,
			.max_uV = 3500000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO1] = {
		.constraints = {
			.name = "axp20_rtc",
			.min_uV = 1300000,
			.max_uV = 1300000,
		},
	},
	[VCC_LDO2] = {
		.constraints = {
			.name = "axp20_ldo2",
			.min_uV = 1800000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO3] = {
		.constraints = {
			.name = "axp20_ldo3",
			.min_uV = 700000,
			.max_uV = 3500000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO4] = {
		.constraints = {
			.name = "axp20_ldo4",
			.min_uV = 1250000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDOIO0] = {
		.constraints = {
			.name = "axp20_ldoio0",
			.min_uV = 1800000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
};

static ssize_t workmode_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret;
	uint8_t val;
	struct regulator_dev *rdev;
	struct axp_regulator_info *info;
	struct axp_regmap *regmap;

	rdev = container_of(dev, struct regulator_dev, dev);
	info = rdev_get_drvdata(rdev);
	regmap = info->regmap;

	ret = axp_regmap_read(regmap, info->mode_reg, &val);
	if (ret)
		return sprintf(buf, "IO ERROR\n");

	if ((val & info->mode_mask) == info->mode_mask)
		return sprintf(buf, "PWM\n");
	else
		return sprintf(buf, "AUTO\n");
}

static ssize_t workmode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	uint8_t val;
	struct regulator_dev *rdev;
	struct axp_regulator_info *info;
	struct axp_regmap *regmap;
	unsigned int mode;
	int ret;

	rdev = container_of(dev, struct regulator_dev, dev);
	info = rdev_get_drvdata(rdev);
	regmap = info->regmap;

	ret = sscanf(buf, "%u", &mode);
	if (ret != 1)
		return -EINVAL;

	val = !!mode;
	if (val)
		axp_regmap_set_bits(regmap, info->mode_reg, info->mode_mask);
	else
		axp_regmap_clr_bits(regmap, info->mode_reg, info->mode_mask);

	return count;
}

static ssize_t frequency_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret;
	uint8_t val;
	struct regulator_dev *rdev;
	struct axp_regulator_info *info;
	struct axp_regmap *regmap;

	rdev = container_of(dev, struct regulator_dev, dev);
	info = rdev_get_drvdata(rdev);
	regmap = info->regmap;

	ret = axp_regmap_read(regmap, info->freq_reg, &val);
	if (ret)
		return ret;

	ret = val & 0x0F;

	return sprintf(buf, "%d\n", (ret * 75 + 750));
}

static ssize_t frequency_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	uint8_t val, tmp;
	int var, err;
	struct regulator_dev *rdev;
	struct axp_regulator_info *info;
	struct axp_regmap *regmap;

	rdev = container_of(dev, struct regulator_dev, dev);
	info = rdev_get_drvdata(rdev);
	regmap = info->regmap;

	err = kstrtoint(buf, 10, &var);
	if (err)
		return err;

	if (var < 750)
		var = 750;

	if (var > 1875)
		var = 1875;

	val = (var - 750) / 75;
	val &= 0x0F;

	axp_regmap_read(regmap, info->freq_reg, &tmp);
	tmp &= 0xF0;
	val |= tmp;
	axp_regmap_write(regmap, info->freq_reg, val);

	return count;
}

static struct device_attribute axp_regu_attrs[] = {
	AXP_REGU_ATTR(workmode),
	AXP_REGU_ATTR(frequency),
};

static int axp_regu_create_attrs(struct device *dev)
{
	int j, ret;

	for (j = 0; j < ARRAY_SIZE(axp_regu_attrs); j++) {
		ret = device_create_file(dev, &axp_regu_attrs[j]);
		if (ret)
			goto sysfs_failed;
	}

	return 0;

sysfs_failed:
	while (j--)
		device_remove_file(dev, &axp_regu_attrs[j]);
	return ret;
}

static s32 axp20_regu_dependence(const char *ldo_name)
{
	s32 axp20_dependence = 0;

	if (strstr(ldo_name, "dcdc2") != NULL)
		axp20_dependence |= AXP20X_DCDC2;
	else if (strstr(ldo_name, "dcdc3") != NULL)
		axp20_dependence |= AXP20X_DCDC3;
	else if (strstr(ldo_name, "ldo2") != NULL)
		axp20_dependence |= AXP20X_LDO2;
	else if (strstr(ldo_name, "ldo3") != NULL)
		axp20_dependence |= AXP20X_LDO3;
	else if (strstr(ldo_name, "ldo4") != NULL)
		axp20_dependence |= AXP20X_LDO4;
	else if (strstr(ldo_name, "ldoio0") != NULL)
		axp20_dependence |= AXP20X_LDOIO0;
	else if (strstr(ldo_name, "rtc") != NULL)
		axp20_dependence |= AXP20X_RTC;
	else
		return -1;

	return axp20_dependence;
}

static int axp20_regulator_probe(struct platform_device *pdev)
{
	s32 i, ret = 0;
	struct axp_regulator_info *info;
	struct axp20_regulators *regu_data;
	struct axp_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);

	if (pdev->dev.of_node) {
		ret = axp_regulator_dt_parse(pdev->dev.of_node,
					axp_regl_init_data,
					axp20_regu_dependence);
		if (ret) {
			pr_err("%s parse device tree err\n", __func__);
			return -EINVAL;
		}
	} else {
		pr_err("axp20 regulator device tree err!\n");
		return -EBUSY;
	}

	regu_data = devm_kzalloc(&pdev->dev, sizeof(*regu_data),
					GFP_KERNEL);
	if (!regu_data)
		return -ENOMEM;

	regu_data->chip = axp_dev;
	platform_set_drvdata(pdev, regu_data);

	for (i = 0; i < VCC_20_MAX; i++) {
		info = &axp20_regulator_info[i];
		info->pmu_num = axp_dev->pmu_num;
		if (info->desc.id == AXP20_ID_DCDC2
				|| info->desc.id == AXP20_ID_DCDC3
				|| info->desc.id == AXP20_ID_LDO1
				|| info->desc.id == AXP20_ID_LDO2
				|| info->desc.id == AXP20_ID_LDO3
				|| info->desc.id == AXP20_ID_LDOIO0) {
			regu_data->regulators[i] = axp_regulator_register(
					&pdev->dev, axp_dev->regmap,
					&axp_regl_init_data[i], info);
		} else if (info->desc.id == AXP20_ID_LDO4) {
			regu_data->regulators[i] = axp_regulator_sel_register
					(&pdev->dev, axp_dev->regmap,
					&axp_regl_init_data[i], info);
		}

		if (IS_ERR(regu_data->regulators[i])) {
			dev_err(&pdev->dev,
				"failed to register regulator %s\n",
				info->desc.name);
			while (--i >= 0)
				axp_regulator_unregister(
					regu_data->regulators[i]);

			return -1;
		}

		if (info->desc.id >= AXP_DCDC_ID_START) {
			ret = axp_regu_create_attrs(
						&regu_data->regulators[i]->dev);
			if (ret)
				dev_err(&pdev->dev,
					"failed to register regulator attr %s\n",
					info->desc.name);
		}
	}

	return 0;
}

static int axp20_regulator_remove(struct platform_device *pdev)
{
	struct axp20_regulators *regu_data = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < VCC_20_MAX; i++)
		regulator_unregister(regu_data->regulators[i]);

	return 0;
}

static const struct of_device_id axp20_regu_dt_ids[] = {
	{ .compatible = "axp20-regulator", },
	{},
};
MODULE_DEVICE_TABLE(of, axp20_regu_dt_ids);

static struct platform_driver axp20_regulator_driver = {
	.driver     = {
		.name   = "axp20-regulator",
		.of_match_table = axp20_regu_dt_ids,
	},
	.probe      = axp20_regulator_probe,
	.remove     = axp20_regulator_remove,
};

static int __init axp20_regulator_initcall(void)
{
	int ret;

	ret = platform_driver_register(&axp20_regulator_driver);
	if (IS_ERR_VALUE(ret)) {
		pr_err("%s: failed, errno %d\n", __func__, ret);
		return -EINVAL;
	}

	return 0;
}
subsys_initcall(axp20_regulator_initcall);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ming Li");
MODULE_DESCRIPTION("Regulator Driver for allwinnertech PMIC");
MODULE_ALIAS("platform:axp-regulator");
