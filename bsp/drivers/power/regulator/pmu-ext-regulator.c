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
#include "pmu-ext.h"

#define PMU_EXT_REGULATOR_STEP_DELAY(_family, _id, _match, _supply, _min, _max, _step, _vreg,	\
		 _vmask, _ereg, _emask)								\
	[_family##_##_id] = {									\
		.name		= (_match),							\
		.supply_name	= (_supply),							\
		.of_match	= of_match_ptr(_match),						\
		.regulators_node = of_match_ptr("regulators"),					\
		.type		= REGULATOR_VOLTAGE,						\
		.id		= _family##_##_id,						\
		.n_voltages	= (((_max) - (_min)) / (_step) + 1),				\
		.owner		= THIS_MODULE,							\
		.min_uV		= (_min) * 1000,						\
		.uV_step	= (_step) * 1000,						\
		.vsel_reg	= (_vreg),							\
		.vsel_mask	= (_vmask),							\
		.enable_reg	= (_ereg),							\
		.enable_mask	= (_emask),							\
		.ops		= &pmu_ext_ops_step_delay,					\
	}

#define PMU_EXT_REGULATOR_RANGE_VOL_DELAY(_family, _id, _match, _supply, _ranges, _n_voltages,	\
			_vreg, _vmask, _ereg, _emask)						\
	[_family##_##_id] = {									\
		.name		= (_match),							\
		.supply_name	= (_supply),							\
		.of_match	= of_match_ptr(_match),						\
		.regulators_node = of_match_ptr("regulators"),					\
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
		.ops		= &pmu_ext_ops_range_vol_delay,					\
	}

#define PMU_EXT_REGULATOR_RANGE_STEP_DELAY(_family, _id, _match, _supply, _ranges, _n_voltages,	\
			_vreg, _vmask, _ereg, _emask)						\
	[_family##_##_id] = {									\
		.name		= (_match),							\
		.supply_name	= (_supply),							\
		.of_match	= of_match_ptr(_match),						\
		.regulators_node = of_match_ptr("regulators"),					\
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
		.ops		= &pmu_ext_ops_range_step_delay,				\
	}

#define AXP1530_EXT_HW_DVM_REGULATOR_RANGE_STEP_DELAY(_family, _id, _match, _supply, _ranges, _n_voltages,	\
			_vreg, _vmask, _ereg, _emask)						\
	[_family##_##_id] = {									\
		.name		= (_match),							\
		.supply_name	= (_supply),							\
		.of_match	= of_match_ptr(_match),						\
		.regulators_node = of_match_ptr("regulators"),					\
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
		.ops		= &axp1530_ext_hw_dvm_ops_range_step_delay,				\
	}

#define AXP1530_EXT_SW_DVM_REGULATOR_RANGE_STEP_DELAY(_family, _id, _match, _supply, _ranges, _n_voltages,	\
			_vreg, _vmask, _ereg, _emask)						\
	[_family##_##_id] = {									\
		.name		= (_match),							\
		.supply_name	= (_supply),							\
		.of_match	= of_match_ptr(_match),						\
		.regulators_node = of_match_ptr("regulators"),					\
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
		.ops		= &axp1530_ext_sw_dvm_ops_range_step_delay,				\
	}

#define AW37501_DESC_SW(_family, _id, _match, _supply)		\
	[_family##_##_id] = {							\
		.name		= (_match),					\
		.supply_name	= (_supply),					\
		.of_match	= of_match_ptr(_match),				\
		.regulators_node = of_match_ptr("regulators"),			\
		.type		= REGULATOR_VOLTAGE,				\
		.id		= _family##_##_id,				\
		.owner		= THIS_MODULE,					\
		.ops		= &aw37501_ops_sw,				\
	}

#define OCP2131_REGULATOR_VOL_DELAY(_family, _id, _match, _supply, _min, _max, _step, _vreg,	\
		 _vmask)								\
	[_family##_##_id] = {									\
		.name		= (_match),							\
		.supply_name	= (_supply),							\
		.of_match	= of_match_ptr(_match),						\
		.regulators_node = of_match_ptr("regulators"),					\
		.type		= REGULATOR_VOLTAGE,						\
		.id		= _family##_##_id,						\
		.n_voltages	= (((_max) - (_min)) / (_step) + 1),				\
		.owner		= THIS_MODULE,							\
		.min_uV		= (_min) * 1000,						\
		.uV_step	= (_step) * 1000,						\
		.vsel_reg	= (_vreg),							\
		.vsel_mask	= (_vmask),							\
		.ops		= &ocp2131_ops_vol_delay,					\
	}

struct regulator_delay {
	u32 step;
	u32 final;
};

static int pmu_ext_set_voltage_time_sel(struct regulator_dev *rdev,
		unsigned int old_selector, unsigned int new_selector)
{
	struct regulator_delay *delay = (struct regulator_delay *)rdev->reg_data;
	int delay_time;

	if (abs(new_selector - old_selector))
		delay_time = (abs(new_selector - old_selector) * delay->step + delay->final + 999) / 1000;
	else
		delay_time = 0;

	return delay_time * 1000;
};

static int axp1530_ext_hw_dvm_set_voltage_time_sel_regmap(struct regulator_dev *rdev,
		unsigned int old_selector, unsigned int new_selector)
{
	struct regulator_delay *delay = (struct regulator_delay *)rdev->reg_data;
	int delay_time;

	if (abs(new_selector - old_selector))
		delay_time = (abs(new_selector - old_selector) * delay->step + delay->final + 999) / 1000;
	else
		delay_time = 0;

	mdelay(delay_time);

	return 0;
};

static int axp1530_ext_hw_dvm_set_voltage_sel_regmap(struct regulator_dev *rdev, unsigned sel)
{
	int ret;
	int val;
	sel <<= ffs(rdev->desc->vsel_mask) - 1;

	regmap_read(rdev->regmap, AXP1530_DCDC1_CONRTOL, &val);
	val |= 0x80;
	ret = regmap_write(rdev->regmap, AXP1530_DCDC2_CONRTOL, val);

	regmap_update_bits(rdev->regmap, 0x14, BIT(7), BIT(7));
	regmap_update_bits(rdev->regmap, 0x1d, BIT(0), 0);
	regmap_update_bits(rdev->regmap, 0x22, BIT(1), 0);

	ret = regmap_update_bits(rdev->regmap, AXP1530_DCDC2_CONRTOL,
				  rdev->desc->vsel_mask, sel);
	mdelay(1);

	ret = regmap_update_bits(rdev->regmap, rdev->desc->vsel_reg,
				  rdev->desc->vsel_mask, sel);
	udelay(500);

	regmap_update_bits(rdev->regmap, 0x22, BIT(1), BIT(1));

	if (ret)
		return ret;

	if (rdev->desc->apply_bit)
		ret = regmap_update_bits(rdev->regmap, rdev->desc->apply_reg,
					 rdev->desc->apply_bit,
					 rdev->desc->apply_bit);
	return ret;
}

static int axp1530_ext_sw_dvm_set_voltage_time_sel(struct regulator_dev *rdev,
		unsigned int old_selector, unsigned int new_selector)
{
	struct regulator_delay *delay = (struct regulator_delay *)rdev->reg_data;
	int delay_time;

	if (abs(new_selector - old_selector))
		delay_time = (abs(new_selector - old_selector) * delay->step + delay->final);
	else
		delay_time = 0;

	return delay_time;
};

#define AXP1530_EXT_SW_DVM_STEP (2)
static int axp1530_ext_sw_dvm_set_voltage_sel_regmap
		  (struct regulator_dev *rdev, unsigned int new_selector)
{
	int ret;
	int i, delay = 0;
	int old_selector = -1;
	int start_selector = 0;
	unsigned int quot, rem;
	const struct regulator_ops *ops = rdev->desc->ops;

	/* first step voltage 1.00v */
	start_selector = 50;

	old_selector = ops->get_voltage_sel(rdev);
	if (old_selector < 0)
		return old_selector;

	new_selector <<= ffs(rdev->desc->vsel_mask) - 1;

	if (new_selector <= old_selector) {
		/* step down voltage */

		ret = regmap_update_bits(rdev->regmap, rdev->desc->vsel_reg,
					rdev->desc->vsel_mask, new_selector);
		if (ret)
			return ret;

		delay = pmu_ext_set_voltage_time_sel(rdev, old_selector, new_selector);
		mdelay(delay / 1000);
	} else {
		/* step up voltage */
		if ((old_selector < start_selector) && (start_selector <= new_selector)) {
			ret = regmap_update_bits(rdev->regmap, rdev->desc->vsel_reg,
						rdev->desc->vsel_mask,
						start_selector);
			if (ret)
				return ret;
			delay = axp1530_ext_sw_dvm_set_voltage_time_sel(rdev, old_selector,
						start_selector);

			if (delay >= 1000) {
				mdelay(delay / 1000);
				udelay(delay % 1000);
			} else if (delay) {
				udelay(delay);
			}
			old_selector = start_selector;
		} else if (new_selector < start_selector) {
			ret = regmap_update_bits(rdev->regmap, rdev->desc->vsel_reg,
						rdev->desc->vsel_mask,
						new_selector);
			if (ret)
				return ret;
			delay = axp1530_ext_sw_dvm_set_voltage_time_sel(rdev, old_selector,
						new_selector);
			if (delay >= 1000) {
				mdelay(delay / 1000);
				udelay(delay % 1000);
			}  else if (delay) {
				udelay(delay);
			}
			old_selector = new_selector;
		}

		quot = (new_selector - old_selector) / AXP1530_EXT_SW_DVM_STEP;
		rem = (new_selector - old_selector) % AXP1530_EXT_SW_DVM_STEP;
		for (i = 1; i <= quot; i++) {
			ret = regmap_update_bits(rdev->regmap, rdev->desc->vsel_reg,
						rdev->desc->vsel_mask,
						old_selector + AXP1530_EXT_SW_DVM_STEP * i);
			if (ret)
				return ret;

			/* delay for every dvm step */
			delay = axp1530_ext_sw_dvm_set_voltage_time_sel(rdev,
									old_selector,
									old_selector + AXP1530_EXT_SW_DVM_STEP);
			if (delay >= 1000) {
				mdelay(delay / 1000);
				udelay(delay % 1000);
			} else if (delay) {
				udelay(delay);
			}
		}

		if (rem) {
			ret = regmap_update_bits(rdev->regmap, rdev->desc->vsel_reg,
						rdev->desc->vsel_mask, new_selector);
			if (ret)
				return ret;

			/* delay for remainder */
			delay = axp1530_ext_sw_dvm_set_voltage_time_sel(rdev,
									old_selector,
									old_selector + rem);
			if (delay >= 1000) {
				mdelay(delay / 1000);
				udelay(delay % 1000);
			} else if (delay) {
				udelay(delay);
			}
		}
	}

	return ret;
}

static int regulator_is_enabled_regmap_aw37501(struct regulator_dev *rdev)
{
	unsigned int val;
	int ret;

	ret = regmap_read(rdev->regmap, AW37501_OUTPUT_EN, &val);
	if (ret)
		return ret;

	return ((val & 0x18) == 0x18);
}

static int regulator_enable_regmap_aw37501(struct regulator_dev *rdev)
{
	int ret = 0;

	regmap_write(rdev->regmap, AW37501_WRITE_PROTECT, 0x4c);
	regmap_update_bits(rdev->regmap, AW37501_OUTPUT_EN, 0x18, 0x18);
	regmap_write(rdev->regmap, AW37501_WRITE_PROTECT, 0);

	return ret;
}

static int regulator_disable_regmap_aw37501(struct regulator_dev *rdev)
{
	int ret = 0;

	regmap_write(rdev->regmap, AW37501_WRITE_PROTECT, 0x4c);
	regmap_update_bits(rdev->regmap, AW37501_OUTPUT_EN, 0x18, 0);
	regmap_write(rdev->regmap, AW37501_WRITE_PROTECT, 0);

	return ret;
}

static struct regulator_ops pmu_ext_ops_step_delay = {
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.list_voltage		= regulator_list_voltage_linear,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= regulator_is_enabled_regmap,
	.set_voltage_time_sel	= pmu_ext_set_voltage_time_sel,
};

static struct regulator_ops __maybe_unused pmu_ext_ops_range_step_delay = {
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.list_voltage		= regulator_list_voltage_linear_range,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= regulator_is_enabled_regmap,
	.set_voltage_time_sel	= pmu_ext_set_voltage_time_sel,
};

static struct regulator_ops __maybe_unused axp1530_ext_hw_dvm_ops_range_step_delay = {
	.set_voltage_sel	= axp1530_ext_hw_dvm_set_voltage_sel_regmap,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.list_voltage		= regulator_list_voltage_linear_range,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= regulator_is_enabled_regmap,
	.set_voltage_time_sel	= axp1530_ext_hw_dvm_set_voltage_time_sel_regmap,
};

static struct regulator_ops __maybe_unused axp1530_ext_sw_dvm_ops_range_step_delay = {
	.set_voltage_sel	= axp1530_ext_sw_dvm_set_voltage_sel_regmap,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.list_voltage		= regulator_list_voltage_linear_range,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= regulator_is_enabled_regmap,
};

/* Operations permitted on DCDCx */
static struct regulator_ops pmu_ext_ops_range_vol_delay = {
	.is_enabled		= regulator_is_enabled_regmap,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.list_voltage		= regulator_list_voltage_linear_range,
};

static struct regulator_ops aw37501_ops_sw = {
	.enable			= regulator_enable_regmap_aw37501,
	.disable		= regulator_disable_regmap_aw37501,
	.is_enabled		= regulator_is_enabled_regmap_aw37501,
};

static struct regulator_ops ocp2131_ops_vol_delay = {
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.list_voltage		= regulator_list_voltage_linear,
};

static const struct linear_range axp1530_dcdc1_ranges[] = {
	REGULATOR_LINEAR_RANGE(500000, 0x0, 0x46, 10000),
	REGULATOR_LINEAR_RANGE(1220000, 0x47, 0x57, 20000),
	REGULATOR_LINEAR_RANGE(1600000, 0x58, 0x6A, 100000),
};

static const struct linear_range axp1530_dcdc2_ranges[] = {
	REGULATOR_LINEAR_RANGE(500000, 0x0, 0x46, 10000),
	REGULATOR_LINEAR_RANGE(1220000, 0x47, 0x57, 20000),
};

static const struct linear_range axp1530_dcdc3_ranges[] = {
	REGULATOR_LINEAR_RANGE(500000, 0x0, 0x46, 10000),
	REGULATOR_LINEAR_RANGE(1220000, 0x47, 0x66, 20000),
};

static const struct regulator_desc axp1530_ext_regulators[] = {
#if !IS_ENABLED(CONFIG_AW_AXP1530_WORKAROUND_DVM)
	PMU_EXT_REGULATOR_RANGE_STEP_DELAY(AXP1530, DCDC1, "dcdc1", "vin1", axp1530_dcdc1_ranges,
			0x6B, AXP1530_DCDC1_CONRTOL, 0x7f, AXP1530_OUTPUT_CONTROL, BIT(0)),
	PMU_EXT_REGULATOR_RANGE_STEP_DELAY(AXP1530, DCDC2, "dcdc2", "vin2", axp1530_dcdc2_ranges,
			0x58, AXP1530_DCDC2_CONRTOL, 0x7f, AXP1530_OUTPUT_CONTROL, BIT(1)),
	PMU_EXT_REGULATOR_RANGE_STEP_DELAY(AXP1530, DCDC3, "dcdc3", "vin3", axp1530_dcdc3_ranges,
			0x58, AXP1530_DCDC3_CONRTOL, 0x7f, AXP1530_OUTPUT_CONTROL, BIT(2)),
#else
	AXP1530_EXT_HW_DVM_REGULATOR_RANGE_STEP_DELAY(AXP1530, DCDC1, "dcdc1", "vin1", axp1530_dcdc1_ranges,
			0x6B, AXP1530_DCDC1_CONRTOL, 0x7f, AXP1530_OUTPUT_CONTROL, BIT(0)),
	AXP1530_EXT_HW_DVM_REGULATOR_RANGE_STEP_DELAY(AXP1530, DCDC2, "dcdc2", "vin2", axp1530_dcdc2_ranges,
			0x58, AXP1530_DCDC2_CONRTOL, 0x7f, AXP1530_OUTPUT_CONTROL, BIT(1)),
	AXP1530_EXT_SW_DVM_REGULATOR_RANGE_STEP_DELAY(AXP1530, DCDC3, "dcdc3", "vin3", axp1530_dcdc3_ranges,
			0x58, AXP1530_DCDC3_CONRTOL, 0x7f, AXP1530_OUTPUT_CONTROL, BIT(2)),
#endif

	PMU_EXT_REGULATOR_STEP_DELAY(AXP1530, LDO1, "ldo1", "ldo1in", 500, 3500, 100,
		AXP1530_ALDO1_CONRTOL, 0x1f, AXP1530_OUTPUT_CONTROL, BIT(3)),
	PMU_EXT_REGULATOR_STEP_DELAY(AXP1530, LDO2, "ldo2", "ldo2in", 500, 3500, 100,
		AXP1530_DLDO1_CONRTOL, 0x1f, AXP1530_OUTPUT_CONTROL, BIT(4)),
};

static const struct linear_range tcs4838_dcdc_ranges[] = {
	REGULATOR_LINEAR_RANGE(712500, 0x0, 0x3F, 12500),
};

static const struct regulator_desc tcs4838_regulators[] = {
	PMU_EXT_REGULATOR_RANGE_VOL_DELAY(TCS4838, DCDC0, "dcdc0", "vin1", tcs4838_dcdc_ranges,
			0x40, TCS4838_VSEL0, GENMASK(5, 0), TCS4838_VSEL0, BIT(7)),
	PMU_EXT_REGULATOR_RANGE_VOL_DELAY(TCS4838, DCDC1, "dcdc1", "vin1", tcs4838_dcdc_ranges,
			0x40, TCS4838_VSEL1, GENMASK(5, 0), TCS4838_VSEL1, BIT(7)),
};

static const struct linear_range sy8827g_dcdc_ranges[] = {
	REGULATOR_LINEAR_RANGE(600000, 0x0, 0x3F, 12500),
};

static const struct regulator_desc sy8827g_regulators[] = {
	PMU_EXT_REGULATOR_RANGE_VOL_DELAY(SY8827G, DCDC0, "dcdc0", "vin1", sy8827g_dcdc_ranges,
			0x40, SY8827G_VSEL0, GENMASK(5, 0), SY8827G_VSEL0, BIT(7)),
	PMU_EXT_REGULATOR_RANGE_VOL_DELAY(SY8827G, DCDC1, "dcdc1", "vin1", sy8827g_dcdc_ranges,
			0x40, SY8827G_VSEL1, GENMASK(5, 0), SY8827G_VSEL1, BIT(7)),
};

static const struct regulator_desc aw37501_regulators[] = {
	AW37501_DESC_SW(AW37501, LDO, "ldo", "vin1"),
};

static const struct regulator_desc ocp2131_regulators[] = {
	OCP2131_REGULATOR_VOL_DELAY(OCP2131, AVDD, "avdd", "avddin", 4000, 6000, 100,
		OCP2131_POS_OUTPUT_AVDD, 0x1f),
	OCP2131_REGULATOR_VOL_DELAY(OCP2131, AVEE, "avee", "aveein", 4000, 6000, 100,
		OCP2131_NEG_OUTPUT_AVEE, 0x1f),
};

static int pmu_ext_regulator_probe(struct platform_device *pdev)
{
	struct regulator_dev *rdev;
	struct regulator_delay *rdev_delay;
	struct pmu_ext_dev *ext = dev_get_drvdata(pdev->dev.parent);
	const struct regulator_desc *regulators;
	struct regulator_config config = {
		.dev = pdev->dev.parent,
		.regmap = ext->regmap,
		.driver_data = ext,
	};
	int i, nregulators;
	u32 dval;

	switch (ext->variant) {
	case AXP1530_ID:
		regulators = axp1530_ext_regulators;
		nregulators = AXP1530_EXT_REG_ID_MAX;
		break;
	case TCS4838_ID:
		regulators = tcs4838_regulators;
		nregulators = TCS4838_REG_ID_MAX;
		break;
	case SY8827G_ID:
		regulators = sy8827g_regulators;
		nregulators = SY8827G_REG_ID_MAX;
		break;
	case AW37501_ID:
		regulators = aw37501_regulators;
		nregulators = AW37501_REG_ID_MAX;
		break;
	case OCP2131_ID:
		regulators = ocp2131_regulators;
		nregulators = OCP2131_REG_ID_MAX;
		break;
	default:
		PMIC_DEV_ERR(&pdev->dev, "Unsupported pmu_ext variant: %ld\n",
			ext->variant);
		return -EINVAL;
	}


	for (i = 0; i < nregulators; i++) {
		const struct regulator_desc *desc = &regulators[i];
		rdev = devm_regulator_register(&pdev->dev, desc, &config);
		if (IS_ERR(rdev)) {
			PMIC_DEV_ERR(&pdev->dev, "Failed to register %s\n",
				regulators[i].name);

			return PTR_ERR(rdev);
		}


		rdev_delay = devm_kzalloc(&pdev->dev, sizeof(*rdev_delay), GFP_KERNEL);
		if (!of_property_read_u32(rdev->dev.of_node,
			"regulator-step-delay-us", &dval))
			rdev_delay->step = dval;
		else
			rdev_delay->step = 0;

		if (!of_property_read_u32(rdev->dev.of_node,
			"regulator-final-delay-us", &dval))
			rdev_delay->final = dval;
		else
			rdev_delay->final = 0;

		rdev->reg_data = rdev_delay;

	}

	return 0;
}

static int pmu_ext_regulator_remove(struct platform_device *pdev)
{
	return 0;
}

static struct of_device_id pmu_ext_match_table[] = {
	{ .compatible = "ext,aw37501-regulator" },
	{ .compatible = "ext,ocp2131-regulator" },
	{ /* sentinel */ },
};

static struct platform_driver pmu_ext_regulator_driver = {
	.driver = {
		.name = "pmu-ext-regulator",
		.of_match_table = pmu_ext_match_table,
	},
	.probe = pmu_ext_regulator_probe,
	.remove	= pmu_ext_regulator_remove,
};

static int __init pmu_ext_regulator_init(void)
{
	return platform_driver_register(&pmu_ext_regulator_driver);
}

static void __exit pmu_ext_regulator_exit(void)
{
	platform_driver_unregister(&pmu_ext_regulator_driver);
}

subsys_initcall(pmu_ext_regulator_init);
module_exit(pmu_ext_regulator_exit);

MODULE_AUTHOR("Andrew F. Davis <afd@ti.com>");
MODULE_DESCRIPTION("pmu_ext voltage regulator driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
