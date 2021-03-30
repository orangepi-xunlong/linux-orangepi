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
#include "axp80.h"
#include "axp80-regu.h"

/* Reverse engineered partly from Platformx drivers */
enum axp_regls {
	VCC_DCDCA,
	VCC_DCDCB,
	VCC_DCDCC,
	VCC_DCDCD,
	VCC_DCDCE,
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
	VCC_80_MAX,
};

struct axp80_regulators {
	struct regulator_dev *regulators[VCC_80_MAX];
	struct axp_dev *chip;
};

#define AXP80_LDO(_id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, freq_addr, dvm_ereg, dvm_ebit, dvm_flag) \
	AXP_LDO(AXP80, _id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, freq_addr, dvm_ereg, dvm_ebit, dvm_flag)

#define AXP80_DCDC(_id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, mode_bit, freq_addr, dvm_ereg, dvm_ebit, dvm_flag) \
	AXP_DCDC(AXP80, _id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, mode_bit, freq_addr, dvm_ereg, dvm_ebit, dvm_flag)

static struct axp_regulator_info axp80_regulator_info[] = {
	AXP80_DCDC(A,       600, 1520, 10,   DCDCA, 0, 7,  DCDCAEN, 0x01,
		0x01,    0,  1100,  20, 0, 0x1B, 0x01, 0x1C, 0x1a, 0, 0),
	AXP80_DCDC(B,      1000, 2550, 50,   DCDCB, 0, 5,  DCDCBEN, 0x02,
		0x02,    0,     0,   0, 0, 0x1B, 0x02, 0x1C, 0, 0, 0),
	AXP80_DCDC(C,       600, 1520, 10,   DCDCC, 0, 7,  DCDCCEN, 0x04,
		0x04,    0,  1100,  20, 0, 0x1B, 0x04, 0x1C, 0, 0, 0),
	AXP80_DCDC(D,       600, 3300, 20,   DCDCD, 0, 6,  DCDCDEN, 0x08,
		0x08,    0,  1500, 100, 0, 0x1B, 0x08, 0x1C, 0, 0, 0),
	AXP80_DCDC(E,      1100, 3400, 100,  DCDCE, 0, 5,  DCDCEEN, 0x10,
		0x10,    0,     0,   0, 0, 0x1B, 0x10, 0x1C, 0, 0, 0),
	AXP80_LDO(1,        700, 3300, 100,  ALDO1, 0, 5,  ALDO1EN, 0x20,
		0x20,    0,      0, 0, 0,     0,    0,    0, 0, 0),
	AXP80_LDO(2,        700, 3400, 100,  ALDO2, 0, 5,  ALDO2EN, 0x40,
		0x40,    0,      0, 0, 0,     0,    0,    0, 0, 0),
	AXP80_LDO(3,        700, 3300, 100,  ALDO3, 0, 5,  ALDO3EN, 0x80,
		0x80,    0,      0, 0, 0,     0,    0,    0, 0, 0),
	AXP80_LDO(4,        700, 1900, 100,  BLDO1, 0, 4,  BLDO1EN, 0x01,
		0x01,    0,      0, 0, 0,     0,    0,    0, 0, 0),
	AXP80_LDO(5,        700, 1900, 100,  BLDO2, 0, 4,  BLDO2EN, 0x02,
		0x02,    0,      0, 0, 0,     0,    0,    0, 0, 0),
	AXP80_LDO(6,        700, 1900, 100,  BLDO3, 0, 4,  BLDO3EN, 0x04,
		0x04,    0,      0, 0, 0,     0,    0,    0, 0, 0),
	AXP80_LDO(7,        700, 1900, 100,  BLDO4, 0, 4,  BLDO4EN, 0x08,
		0x08,    0,      0, 0, 0,     0,    0,    0, 0, 0),
	AXP80_LDO(8,        700, 3300, 100,  CLDO1, 0, 5,  CLDO1EN, 0x10,
		0x10,    0,      0, 0, 0,     0,    0,    0, 0, 0),
	AXP80_LDO(9,        700, 4200, 100,  CLDO2, 0, 5,  CLDO2EN, 0x20,
		0x20,    0, 3400, 200, 0,     0,    0,    0, 0, 0),
	AXP80_LDO(10,       700, 3300, 100,  CLDO3, 0, 5,  CLDO3EN, 0x40,
		0x40,    0,      0, 0, 0,     0,    0,    0, 0, 0),
	AXP80_LDO(11,      3300, 3300,   0,     SW, 0, 0,     SWEN, 0x80,
		0x80,    0,      0, 0, 0,     0,    0,    0, 0, 0),
};

static struct regulator_init_data axp_regl_init_data[] = {
	[VCC_DCDCA] = {
		.constraints = {
			.name = "axp80_dcdca",
			.min_uV = 600000,
			.max_uV = 1520000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_DCDCB] = {
		.constraints = {
			.name = "axp80_dcdcb",
			.min_uV = 1000000,
			.max_uV = 2550000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_DCDCC] = {
		.constraints = {
			.name = "axp80_dcdcc",
			.min_uV = 600000,
			.max_uV = 1520000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_DCDCD] = {
		.constraints = {
			.name = "axp80_dcdcd",
			.min_uV = 600000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_DCDCE] = {
		.constraints = {
			.name = "axp80_dcdce",
			.min_uV = 1100000,
			.max_uV = 3400000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO1] = {
		.constraints = {
			.name = "axp80_aldo1",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO2] = {
		.constraints = {
			.name = "axp80_aldo2",
			.min_uV = 700000,
			.max_uV = 3400000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO3] = {
		.constraints = {
			.name = "axp80_aldo3",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},

	},
	[VCC_LDO4] = {
		.constraints = {
			.name = "axp80_bldo1",
			.min_uV = 700000,
			.max_uV = 1900000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO5] = {
		.constraints = {
			.name = "axp80_bldo2",
			.min_uV = 700000,
			.max_uV = 1900000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO6] = {
		.constraints = {
			.name = "axp80_bldo3",
			.min_uV = 700000,
			.max_uV = 1900000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO7] = {
		.constraints = {
			.name = "axp80_bldo4",
			.min_uV = 700000,
			.max_uV = 1900000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO8] = {
		.constraints = {
			.name = "axp80_cldo1",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO9] = {
		.constraints = {
			.name = "axp80_cldo2",
			.min_uV = 700000,
			.max_uV = 4200000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO10] = {
		.constraints = {
			.name = "axp80_cldo3",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO11] = {
		.constraints = {
			.name = "axp80_sw",
			.valid_ops_mask = REGULATOR_CHANGE_STATUS,
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

static s32 axp80_regu_dependence(const char *ldo_name)
{
	s32 axp80_dependence = 0;

	if (strstr(ldo_name, "dcdca") != NULL)
		axp80_dependence |= AXP806_808_DCDC1;
	else if (strstr(ldo_name, "dcdcb") != NULL)
		axp80_dependence |= AXP806_808_DCDC2;
	else if (strstr(ldo_name, "dcdcc") != NULL)
		axp80_dependence |= AXP806_808_DCDC3;
	else if (strstr(ldo_name, "dcdcd") != NULL)
		axp80_dependence |= AXP806_808_DCDC4;
	else if (strstr(ldo_name, "dcdce") != NULL)
		axp80_dependence |= AXP806_808_DCDC5;
	else if (strstr(ldo_name, "aldo1") != NULL)
		axp80_dependence |= AXP806_808_ALDO1;
	else if (strstr(ldo_name, "aldo2") != NULL)
		axp80_dependence |= AXP806_808_ALDO2;
	else if (strstr(ldo_name, "aldo3") != NULL)
		axp80_dependence |= AXP806_808_ALDO3;
	else if (strstr(ldo_name, "bldo1") != NULL)
		axp80_dependence |= AXP806_808_BLDO1;
	else if (strstr(ldo_name, "bldo2") != NULL)
		axp80_dependence |= AXP806_808_BLDO2;
	else if (strstr(ldo_name, "bldo3") != NULL)
		axp80_dependence |= AXP806_808_BLDO3;
	else if (strstr(ldo_name, "bldo4") != NULL)
		axp80_dependence |= AXP806_808_BLDO4;
	else if (strstr(ldo_name, "cldo1") != NULL)
		axp80_dependence |= AXP806_808_CLDO1;
	else if (strstr(ldo_name, "cldo2") != NULL)
		axp80_dependence |= AXP806_808_CLDO2;
	else if (strstr(ldo_name, "cldo3") != NULL)
		axp80_dependence |= AXP806_808_CLDO3;
	else if (strstr(ldo_name, "sw") != NULL)
		axp80_dependence |= AXP806_808_SW0;
	else
		return -1;

	return axp80_dependence;
}

static int axp80_regulator_probe(struct platform_device *pdev)
{
	s32 i, ret = 0;
	struct axp_regulator_info *info;
	struct axp80_regulators *regu_data;
	struct axp_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);

	if (pdev->dev.of_node) {
		ret = axp_regulator_dt_parse(pdev->dev.of_node,
					axp_regl_init_data,
					axp80_regu_dependence);
		if (ret) {
			pr_err("%s parse device tree err\n", __func__);
			return -EINVAL;
		}
	} else {
		pr_err("axp80 regulator device tree err!\n");
		return -EBUSY;
	}

	regu_data = devm_kzalloc(&pdev->dev, sizeof(*regu_data),
					GFP_KERNEL);
	if (!regu_data)
		return -ENOMEM;

	regu_data->chip = axp_dev;
	platform_set_drvdata(pdev, regu_data);

	for (i = 0; i < VCC_80_MAX; i++) {
		info = &axp80_regulator_info[i];
		info->pmu_num = axp_dev->pmu_num;
		regu_data->regulators[i] = axp_regulator_register(
				&pdev->dev, axp_dev->regmap,
				&axp_regl_init_data[i], info);

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

static int axp80_regulator_remove(struct platform_device *pdev)
{
	struct axp80_regulators *regu_data = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < VCC_80_MAX; i++)
		regulator_unregister(regu_data->regulators[i]);

	return 0;
}

static const struct of_device_id axp80_regu_dt_ids[] = {
	{ .compatible = "axp806-regulator", },
	{},
};
MODULE_DEVICE_TABLE(of, axp80_regu_dt_ids);

static struct platform_driver axp80_regulator_driver = {
	.driver     = {
		.name   = "axp80-regulator",
		.of_match_table = axp80_regu_dt_ids,
	},
	.probe      = axp80_regulator_probe,
	.remove     = axp80_regulator_remove,
};

static int __init axp80_regulator_initcall(void)
{
	int ret;

	ret = platform_driver_register(&axp80_regulator_driver);
	if (IS_ERR_VALUE(ret)) {
		pr_err("%s: failed, errno %d\n", __func__, ret);
		return -EINVAL;
	}

	return 0;
}
subsys_initcall(axp80_regulator_initcall);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Qin <qinyongshen@allwinnertech.com>");
MODULE_DESCRIPTION("Regulator Driver for axp80 PMIC");
MODULE_ALIAS("platform:axp80-regulator");
