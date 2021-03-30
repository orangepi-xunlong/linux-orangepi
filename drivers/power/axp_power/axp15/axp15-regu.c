/*
 * Regulators driver for allwinnertech AXP15X
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
#include "axp15.h"
#include "axp15-regu.h"

/* Reverse engineered partly from Platformx drivers */
enum axp_regls {
	VCC_DCDC1,
	VCC_DCDC2,
	VCC_DCDC3,
	VCC_DCDC4,
	VCC_LDO1,
	VCC_LDO2,
	VCC_LDO3,
	VCC_LDO4,
	VCC_LDO5,
	VCC_LDO6,
	VCC_LDO7,
	VCC_15_MAX,
};

struct axp15_regulators {
	struct regulator_dev *regulators[VCC_15_MAX];
	struct axp_dev *chip;
};

static const int axp15_ldo0_table[] = {
	5000, 3300, 2800, 2500
};

static const int axp15_dcdc1_table[] = {
	1700, 1800, 1900, 2000,
	2100, 2400, 2500, 2600,
	2700, 2800, 3000, 3100,
	3200, 3300, 3400, 3500
};

static const int axp15_aldo12_table[] = {
	1200, 1300, 1400, 1500,
	1600, 1700, 1800, 1900,
	2000, 2500, 2700, 2800,
	3000, 3100, 3200, 3300
};

#define AXP15_LDO(_id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, freq_addr, dvm_ereg, dvm_ebit, dvm_flag)\
	AXP_LDO(AXP15, _id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, freq_addr, dvm_ereg, dvm_ebit, dvm_flag)

#define AXP15_LDO_SEL(_id, min, max, vreg, shift, nbits,\
		ereg, emask, enval, disval, vtable,\
		mode_addr, freq_addr, dvm_ereg, dvm_ebit, dvm_flag) \
	AXP_LDO_SEL(AXP15, _id, min, max, vreg, shift, nbits,\
		ereg, emask, enval, disval, vtable,\
		mode_addr, freq_addr, dvm_ereg, dvm_ebit, dvm_flag)

#define AXP15_DCDC(_id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, mode_bit, freq_addr, dvm_ereg, dvm_ebit, dvm_flag) \
	AXP_DCDC(AXP15, _id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, mode_bit, freq_addr, dvm_ereg, dvm_ebit, dvm_flag)

#define AXP15_DCDC_SEL(_id, min, max, vreg, shift, nbits,\
		ereg, emask, enval, disval, vtable,\
		mode_addr, mode_bit, freq_addr, dvm_ereg, dvm_ebit, dvm_flag) \
	AXP_DCDC_SEL(AXP15, _id, min, max, vreg, shift, nbits,\
		ereg, emask, enval, disval, vtable,\
		mode_addr, mode_bit, freq_addr, dvm_ereg, dvm_ebit, dvm_flag)

static struct axp_regulator_info axp15_regulator_info[] = {
	AXP15_DCDC_SEL(1,  1700, 3500,       DCDC1, 0, 4,  DCDC1EN, 0x80,
		0x80,    0,  axp15_dcdc1,  0x80, 0x08, 0x37, 0, 0, 0),
	AXP15_DCDC(2,       700, 2275, 25,   DCDC2, 0, 6,  DCDC2EN, 0x40,
		0x40,    0,      0, 0, 0,  0x80, 0x04, 0x37, 0x25, 2, 0),
	AXP15_DCDC(3,       700, 3500, 50,   DCDC3, 0, 6,  DCDC3EN, 0x20,
		0x20,    0,      0, 0, 0,  0x80, 0x02, 0x37, 0, 0, 0),
	AXP15_DCDC(4,       700, 3500, 25,   DCDC4, 0, 7,  DCDC4EN, 0x10,
		0x10,    0,      0, 0, 0,  0x80, 0x01, 0x37, 0, 0, 0),
	AXP15_LDO_SEL(0,   2500, 5000,        LDO0, 4, 2,   LDO0EN, 0x80,
		0x80,    0,   axp15_ldo0,     0,    0,    0, 0, 0),
	AXP15_LDO(1,       3100, 3100,  0,     RTC, 0, 0, RTCLDOEN, 0x01,
		0x01,    0,      0, 0, 0,     0,    0,    0, 0, 0),
	AXP15_LDO_SEL(2,   1200, 3300,       ALDO1, 4, 4,  ALDO1EN, 0x08,
		0x08,    0, axp15_aldo12,     0,    0,    0, 0, 0),
	AXP15_LDO_SEL(3,   1200, 3300,       ALDO2, 0, 4,  ALDO2EN, 0x04,
		0x04,    0, axp15_aldo12,     0,    0,    0, 0, 0),
	AXP15_LDO(4,        700, 3500, 100,  DLDO1, 0, 5,  DLDO1EN, 0x02,
		0x02,    0,      0, 0, 0,     0,    0,    0, 0, 0),
	AXP15_LDO(5,        700, 3500, 100,  DLDO2, 0, 5,  DLDO2EN, 0x01,
		0x01,    0,      0, 0, 0,     0,    0,    0, 0, 0),
	AXP15_LDO(IO0,     1800, 3300, 100, LDOIO0, 0, 4,  LDOI0EN, 0x07,
		0x02, 0x07,      0, 0, 0,     0,    0,    0, 0, 0),
};

static struct regulator_init_data axp_regl_init_data[] = {
	[VCC_DCDC1] = {
		.constraints = {
			.name = "axp15_dcdc1",
			.min_uV = 1700000,
			.max_uV = 3500000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_DCDC2] = {
		.constraints = {
			.name = "axp15_dcdc2",
			.min_uV = 700000,
			.max_uV = 2275000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_DCDC3] = {
		.constraints = {
			.name = "axp15_dcdc3",
			.min_uV = 700000,
			.max_uV = 3500000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_DCDC4] = {
		.constraints = {
			.name = "axp15_dcdc4",
			.min_uV = 700000,
			.max_uV = 3500000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO1] = {
		.constraints = {
			.name = "axp15_ldo0",
			.min_uV =  2500000,
			.max_uV =  5000000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO2] = {
		.constraints = {
			.name = "axp15_rtc",
			.min_uV = 3100000,
			.max_uV = 3100000,
		},
	},
	[VCC_LDO3] = {
		.constraints = {
			.name = "axp15_aldo1",
			.min_uV = 1200000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},

	},
	[VCC_LDO4] = {
		.constraints = {
			.name = "axp15_aldo2",
			.min_uV = 1200000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO5] = {
		.constraints = {
			.name = "axp15_dldo1",
			.min_uV = 700000,
			.max_uV = 3500000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO6] = {
		.constraints = {
			.name = "axp15_dldo2",
			.min_uV = 700000,
			.max_uV = 3500000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO7] = {
		.constraints = {
			.name = "axp15_gpioldo",
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

	return sprintf(buf, "%d\n", (ret * 5 + 50));
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

	if (var < 50)
		var = 50;

	if (var > 100)
		var = 100;

	val = (var - 50) / 5;
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

static s32 axp15_regu_dependence(const char *ldo_name)
{
	s32 axp15_dependence = 0;

	if (strstr(ldo_name, "dcdc1") != NULL)
		axp15_dependence |= AXP15X_DCDC1;
	else if (strstr(ldo_name, "dcdc2") != NULL)
		axp15_dependence |= AXP15X_DCDC2;
	else if (strstr(ldo_name, "dcdc3") != NULL)
		axp15_dependence |= AXP15X_DCDC3;
	else if (strstr(ldo_name, "dcdc4") != NULL)
		axp15_dependence |= AXP15X_DCDC4;
	else if (strstr(ldo_name, "aldo1") != NULL)
		axp15_dependence |= AXP15X_ALDO1;
	else if (strstr(ldo_name, "aldo2") != NULL)
		axp15_dependence |= AXP15X_ALDO2;
	else if (strstr(ldo_name, "dldo1") != NULL)
		axp15_dependence |= AXP15X_DLDO1;
	else if (strstr(ldo_name, "dldo2") != NULL)
		axp15_dependence |= AXP15X_DLDO2;
	else if (strstr(ldo_name, "ldoio0") != NULL)
		axp15_dependence |= AXP15X_LDOIO0;
	else if (strstr(ldo_name, "ldo0") != NULL)
		axp15_dependence |= AXP15X_LDO0;
	else if (strstr(ldo_name, "rtc") != NULL)
		axp15_dependence |= AXP15X_RTC;
	else
		return -1;

	return axp15_dependence;
}

static int axp15_regulator_probe(struct platform_device *pdev)
{
	s32 i, ret = 0;
	struct axp_regulator_info *info;
	struct axp15_regulators *regu_data;
	struct axp_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);

	if (pdev->dev.of_node) {
		ret = axp_regulator_dt_parse(pdev->dev.of_node,
					axp_regl_init_data,
					axp15_regu_dependence);
		if (ret) {
			pr_err("%s parse device tree err\n", __func__);
			return -EINVAL;
		}
	} else {
		pr_err("axp15 regulator device tree err!\n");
		return -EBUSY;
	}

	regu_data = devm_kzalloc(&pdev->dev, sizeof(*regu_data),
					GFP_KERNEL);
	if (!regu_data)
		return -ENOMEM;

	regu_data->chip = axp_dev;
	platform_set_drvdata(pdev, regu_data);

	for (i = 0; i < VCC_15_MAX; i++) {
		info = &axp15_regulator_info[i];
		info->pmu_num = axp_dev->pmu_num;
		if (info->desc.id == AXP15_ID_LDO4
				|| info->desc.id == AXP15_ID_LDO5
				|| info->desc.id == AXP15_ID_DCDC2
				|| info->desc.id == AXP15_ID_DCDC3
				|| info->desc.id == AXP15_ID_DCDC4
				|| info->desc.id == AXP15_ID_LDO1
				|| info->desc.id == AXP15_ID_LDOIO0) {
			regu_data->regulators[i] = axp_regulator_register(
					&pdev->dev, axp_dev->regmap,
					&axp_regl_init_data[i], info);
		} else if (info->desc.id == AXP15_ID_DCDC1
				|| info->desc.id == AXP15_ID_LDO0
				|| info->desc.id == AXP15_ID_LDO3
				|| info->desc.id == AXP15_ID_LDO2) {
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

static int axp15_regulator_remove(struct platform_device *pdev)
{
	struct axp15_regulators *regu_data = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < VCC_15_MAX; i++)
		regulator_unregister(regu_data->regulators[i]);

	return 0;
}

static const struct of_device_id axp15_regu_dt_ids[] = {
	{ .compatible = "axp157-regulator", },
	{},
};
MODULE_DEVICE_TABLE(of, axp15_regu_dt_ids);

static struct platform_driver axp15_regulator_driver = {
	.driver     = {
		.name   = "axp15-regulator",
		.of_match_table = axp15_regu_dt_ids,
	},
	.probe      = axp15_regulator_probe,
	.remove     = axp15_regulator_remove,
};

static int __init axp15_regulator_initcall(void)
{
	int ret;

	ret = platform_driver_register(&axp15_regulator_driver);
	if (IS_ERR_VALUE(ret)) {
		pr_err("%s: failed, errno %d\n", __func__, ret);
		return -EINVAL;
	}

	return 0;
}
subsys_initcall(axp15_regulator_initcall);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Qin <qinyongshen@allwinnertech.com>");
MODULE_DESCRIPTION("Regulator Driver for axp15 PMIC");
MODULE_ALIAS("platform:axp15-regulator");
