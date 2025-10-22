/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * MFD core driver for the ETA6973 Power Management ICs
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
#include "bmu-ext.h"

#define INIT_REGMAP_IRQ(_variant, _irq, _off, _mask)			\
	[_variant##_IRQ_##_irq] = { .reg_offset = (_off), .mask = BIT(_mask) }

static const char *const bmu_ext_model_names[] = {
	"ETA6973", "AXP519", "AXP2601", "AXP2602",
};

static const struct regmap_range eta6973_writeable_ranges[] = {
	regmap_reg_range(ETA6973_REG_00, ETA6973_REG_0B),
};

static const struct regmap_range eta6973_volatile_ranges[] = {
	regmap_reg_range(ETA6973_REG_00, ETA6973_REG_0B),
};

static const struct regmap_access_table eta6973_writeable_table = {
	.yes_ranges	= eta6973_writeable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(eta6973_writeable_ranges),
};

static const struct regmap_access_table eta6973_volatile_table = {
	.yes_ranges	= eta6973_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(eta6973_volatile_ranges),
};

const struct regmap_config eta6973_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.wr_table	= &eta6973_writeable_table,
	.volatile_table	= &eta6973_volatile_table,
	.max_register   = ETA6973_REG_0B,
	.use_single_read = true,
	.use_single_write = true,
	.cache_type     = REGCACHE_RBTREE,
};
/*---------------------------------*/

static const struct regmap_irq axp519_regmap_irqs[] = {
	INIT_REGMAP_IRQ(AXP519, SOVP,		0, 6),
	INIT_REGMAP_IRQ(AXP519, COVT,		0, 5),
	INIT_REGMAP_IRQ(AXP519, CHGDN,		0, 4),
	INIT_REGMAP_IRQ(AXP519, B_INSERT,	0, 3),
	INIT_REGMAP_IRQ(AXP519, B_REMOVE,	0, 2),
	INIT_REGMAP_IRQ(AXP519, A_INSERT,	0, 1),
	INIT_REGMAP_IRQ(AXP519, A_REMOVE,	0, 0),
	INIT_REGMAP_IRQ(AXP519, IC_WOT,		1, 7),
	INIT_REGMAP_IRQ(AXP519, NTC,		1, 6),
	INIT_REGMAP_IRQ(AXP519, VBUS_OV,	1, 5),
	INIT_REGMAP_IRQ(AXP519, BOVP,		1, 4),
	INIT_REGMAP_IRQ(AXP519, BUVP,		1, 3),
	INIT_REGMAP_IRQ(AXP519, VBUS_SC,	1, 2),
	INIT_REGMAP_IRQ(AXP519, VBUS_OL,	1, 1),
	INIT_REGMAP_IRQ(AXP519, SSCP,		1, 0),
};

static struct resource axp519_charger_power_supply_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP519_IRQ_SOVP, "sys_over_voltage"),
	DEFINE_RES_IRQ_NAMED(AXP519_IRQ_COVT, "charge_over_time"),
	DEFINE_RES_IRQ_NAMED(AXP519_IRQ_CHGDN, "battery_charge_done"),
	DEFINE_RES_IRQ_NAMED(AXP519_IRQ_B_INSERT, "b_insert"),
	DEFINE_RES_IRQ_NAMED(AXP519_IRQ_B_REMOVE, "b_remove"),
	DEFINE_RES_IRQ_NAMED(AXP519_IRQ_A_INSERT, "a_insert"),
	DEFINE_RES_IRQ_NAMED(AXP519_IRQ_A_REMOVE, "a_remove"),
	DEFINE_RES_IRQ_NAMED(AXP519_IRQ_IC_WOT, "ic_over_voltage"),
	DEFINE_RES_IRQ_NAMED(AXP519_IRQ_NTC, "battery_ntc_reaching"),
	DEFINE_RES_IRQ_NAMED(AXP519_IRQ_VBUS_OV, "vbus_over_volt"),
	DEFINE_RES_IRQ_NAMED(AXP519_IRQ_BOVP, "battery_over_voltage"),
	DEFINE_RES_IRQ_NAMED(AXP519_IRQ_BUVP, "battery_under_voltage"),
	DEFINE_RES_IRQ_NAMED(AXP519_IRQ_VBUS_SC, "vbus_short_circuit"),
	DEFINE_RES_IRQ_NAMED(AXP519_IRQ_VBUS_OL, "vbus_overload"),
	DEFINE_RES_IRQ_NAMED(AXP519_IRQ_SSCP, "sys_short_circuit"),
};

static const struct regmap_irq_chip axp519_regmap_irq_chip = {
	.name			= "axp519_irq_chip",
	.status_base		= AXP519_IRQ0,
	.ack_base		= AXP519_IRQ0,
	.mask_base		= AXP519_IRQ_EN0,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
	.mask_invert		= false,
#endif
	.init_ack_masked	= true,
	.irqs			= axp519_regmap_irqs,
	.num_irqs		= ARRAY_SIZE(axp519_regmap_irqs),
	.num_regs		= 2,
};

static const struct regmap_range axp519_writeable_ranges[] = {
	regmap_reg_range(AXP519_CHIP_ID, AXP519_END),
};

static const struct regmap_range axp519_volatile_ranges[] = {
	regmap_reg_range(AXP519_CHIP_ID, AXP519_END),
};

static const struct regmap_access_table axp519_writeable_table = {
	.yes_ranges	= axp519_writeable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp519_writeable_ranges),
};

static const struct regmap_access_table axp519_volatile_table = {
	.yes_ranges	= axp519_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp519_volatile_ranges),
};

const struct regmap_config axp519_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.wr_table	= &axp519_writeable_table,
	.volatile_table	= &axp519_volatile_table,
	.max_register   = AXP519_END,
	.use_single_read = true,
	.use_single_write = true,
	.cache_type     = REGCACHE_RBTREE,
};

/*---------------------------------*/

static const struct regmap_irq axp2601_regmap_irqs[] = {
	INIT_REGMAP_IRQ(AXP2601, WDT,		0, 3),
	INIT_REGMAP_IRQ(AXP2601, OT,		0, 2),
	INIT_REGMAP_IRQ(AXP2601, NEWSOC,	0, 1),
	INIT_REGMAP_IRQ(AXP2601, LOWSOC,	0, 0),
};

static struct resource axp2601_bat_power_supply_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP2601_IRQ_WDT, "watchdog_reset"),
	DEFINE_RES_IRQ_NAMED(AXP2601_IRQ_OT, "over_tempture"),
	DEFINE_RES_IRQ_NAMED(AXP2601_IRQ_NEWSOC, "gauge_new_soc"),
	DEFINE_RES_IRQ_NAMED(AXP2601_IRQ_LOWSOC, "soc_drop_w"),
};

static const struct regmap_irq_chip axp2601_regmap_irq_chip = {
	.name			= "axp2601_irq_chip",
	.status_base		= AXP2601_IRQ,
	.ack_base		= AXP2601_IRQ,
	.mask_base		= AXP2601_IRQ_EN,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
	.mask_invert		= false,
#endif
	.init_ack_masked	= true,
	.irqs			= axp2601_regmap_irqs,
	.num_irqs		= ARRAY_SIZE(axp2601_regmap_irqs),
	.num_regs		= 1,
};

static const struct regmap_range axp2601_writeable_ranges[] = {
	regmap_reg_range(AXP2601_CHIP_ID, AXP2601_END),
};

static const struct regmap_range axp2601_volatile_ranges[] = {
	regmap_reg_range(AXP2601_CHIP_ID, AXP2601_END),
};

static const struct regmap_access_table axp2601_writeable_table = {
	.yes_ranges	= axp2601_writeable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp2601_writeable_ranges),
};

static const struct regmap_access_table axp2601_volatile_table = {
	.yes_ranges	= axp2601_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp2601_volatile_ranges),
};

const struct regmap_config axp2601_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.wr_table	= &axp2601_writeable_table,
	.volatile_table	= &axp2601_volatile_table,
	.max_register   = AXP2601_END,
	.use_single_read = true,
	.use_single_write = true,
	.cache_type     = REGCACHE_RBTREE,
};

/*---------------------------------*/

static const struct regmap_irq axp2602_regmap_irqs[] = {
	INIT_REGMAP_IRQ(AXP2602, CUR_ERR,	0, 6),
	INIT_REGMAP_IRQ(AXP2602, SMOOTH_END,	0, 5),
	INIT_REGMAP_IRQ(AXP2602, SOC_ERR,	0, 4),
	INIT_REGMAP_IRQ(AXP2602, WDT,		0, 3),
	INIT_REGMAP_IRQ(AXP2602, OT,		0, 2),
	INIT_REGMAP_IRQ(AXP2602, NEWSOC,	0, 1),
	INIT_REGMAP_IRQ(AXP2602, LOWSOC,	0, 0),
};

static struct resource axp2602_bat_power_supply_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP2602_IRQ_CUR_ERR, "cur_err"),
	DEFINE_RES_IRQ_NAMED(AXP2602_IRQ_SMOOTH_END, "smooth_end"),
	DEFINE_RES_IRQ_NAMED(AXP2602_IRQ_SOC_ERR, "soc_err"),
	DEFINE_RES_IRQ_NAMED(AXP2602_IRQ_WDT, "watchdog_reset"),
	DEFINE_RES_IRQ_NAMED(AXP2602_IRQ_OT, "over_tempture"),
	DEFINE_RES_IRQ_NAMED(AXP2602_IRQ_NEWSOC, "gauge_new_soc"),
	DEFINE_RES_IRQ_NAMED(AXP2602_IRQ_LOWSOC, "soc_drop_w"),
};

static const struct regmap_irq_chip axp2602_regmap_irq_chip = {
	.name			= "axp2602_irq_chip",
	.status_base	= AXP2602_IRQ,
	.ack_base		= AXP2602_IRQ,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
	.mask_base		= AXP2602_IRQ_EN,
	.mask_invert		= true,
#else
	.unmask_base		= AXP2602_IRQ_EN,
#endif
	.init_ack_masked	= true,
	.irqs			= axp2602_regmap_irqs,
	.num_irqs		= ARRAY_SIZE(axp2602_regmap_irqs),
	.num_regs		= 1,
};

static const struct regmap_range axp2602_writeable_ranges[] = {
	regmap_reg_range(AXP2602_CHIP_ID, AXP2602_END),
};

static const struct regmap_range axp2602_volatile_ranges[] = {
	regmap_reg_range(AXP2602_CHIP_ID, AXP2602_END),
};

static const struct regmap_access_table axp2602_writeable_table = {
	.yes_ranges	= axp2602_writeable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp2602_writeable_ranges),
};

static const struct regmap_access_table axp2602_volatile_table = {
	.yes_ranges	= axp2602_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp2602_volatile_ranges),
};

const struct regmap_config axp2602_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.wr_table	= &axp2602_writeable_table,
	.volatile_table	= &axp2602_volatile_table,
	.max_register   = AXP2602_END,
	.use_single_read = true,
	.use_single_write = true,
	.cache_type     = REGCACHE_RBTREE,
};

/*---------------------------------*/

static struct mfd_cell eta6973_cells[] = {
	{
		.name = "eta6973-charger-power-supply",
		.of_compatible = "eta,eta6973-charger-power-supply",
	},
	{ .name = "bmu-ext-regulator", },
};

static struct mfd_cell axp519_cells[] = {
	{
		.name = "axp519-charger-power-supply",
		.of_compatible = "x-powers-ext,axp519-charger-power-supply",
		.resources = axp519_charger_power_supply_resources,
		.num_resources = ARRAY_SIZE(axp519_charger_power_supply_resources),
	},
	{ .name = "bmu-ext-regulator", },
};

static struct mfd_cell axp2601_cells[] = {
	{
		.name = "axp2601-bat-power-supply",
		.of_compatible = "x-powers-ext,axp2601-bat-power-supply",
		.resources = axp2601_bat_power_supply_resources,
		.num_resources = ARRAY_SIZE(axp2601_bat_power_supply_resources),
	},
};

static struct mfd_cell axp2602_cells[] = {
	{
		.name = "axp2602-bat-power-supply",
		.of_compatible = "x-powers-ext,axp2602-bat-power-supply",
		.resources = axp2602_bat_power_supply_resources,
		.num_resources = ARRAY_SIZE(axp2602_bat_power_supply_resources),
	},
};


static void eta6973_dts_parse(struct bmu_ext_dev *ext)
{
	struct device_node *node = ext->dev->of_node;
	struct regmap *map = ext->regmap;
	u32 val, tmpval;

	/* init eta6973 watchdog protection */
	if (of_property_read_u32(node, "watchdog_enable", &val))
		val = 0;
	if (val) {
		if (of_property_read_u32(node,
					"watchdog_timer", &tmpval))
			tmpval = 40;
		regmap_read(map, ETA6973_REG_05, &val);
		val &= ~(ETA6973_REG05_WDT_MASK);
		switch (tmpval) {
		case 80:
			tmpval |= ETA6973_REG05_WDT_80S;
			break;
		case 160:
			tmpval |= ETA6973_REG05_WDT_160S;
			break;
		default:
			tmpval |= ETA6973_REG05_WDT_40S;
			break;
		}
		val |= tmpval << ETA6973_REG05_WDT_SHIFT;
		regmap_update_bits(map, ETA6973_REG_05, ETA6973_REG05_WDT_MASK, val);
		regmap_update_bits(map, ETA6973_REG_01, ETA6973_REG01_WDT_RESET_MASK, ETA6973_REG01_WDT_RESET_MASK);
	} else {
		regmap_update_bits(map, ETA6973_REG_05, ETA6973_REG05_WDT_MASK, ETA6973_REG05_WDT_DISABLE);
	}

	/* init eta6973 min sys vol */
	if (!of_property_read_u32(node, "sys_vol", &tmpval)) {
		regmap_read(map, ETA6973_REG_01, &val);
		val &= ~(ETA6973_REG01_SYS_MINV_MASK);
		if (tmpval > 3600)
			tmpval = 0x0E;
		else if	(tmpval > 3500)
			tmpval = 0x0C;
		else if	(tmpval > 3400)
			tmpval = 0x0A;
		else if	(tmpval > 3200)
			tmpval = 0x08;
		else if	(tmpval > 3400)
			tmpval = 0x06;
		else if	(tmpval > 3000)
			tmpval = 0x04;
		else if	(tmpval > 2800)
			tmpval = 0x02;
		else
			tmpval = 0X00;
		regmap_update_bits(map, ETA6973_REG_01, ETA6973_REG01_SYS_MINV_MASK, tmpval);
	}

	/* init eta6973 thermal */
	if (of_property_read_u32(node, "thermal_temp", &tmpval))
		val = 90;
	regmap_read(map, ETA6973_REG_05, &val);
	val &= ~(ETA6973_REG05_TREG_MASK);

	if (tmpval > 90)
		tmpval = ETA6973_REG05_TREG_110C;
	else
		tmpval = ETA6973_REG05_TREG_90C;

	val |= tmpval << ETA6973_REG05_TREG_SHIFT;
	regmap_update_bits(map, ETA6973_REG_05, ETA6973_REG05_TREG_MASK, val);


}

int bmu_ext_match_device(struct bmu_ext_dev *ext)
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
	case ETA6973_ID:
		ext->nr_cells = ARRAY_SIZE(eta6973_cells);
		ext->cells = eta6973_cells;
		ext->regmap_cfg = &eta6973_regmap_config;
		ext->dts_parse = eta6973_dts_parse;
		break;
/**************************************/
	case AXP519_ID:
		ext->nr_cells = ARRAY_SIZE(axp519_cells);
		ext->cells = axp519_cells;
		ext->regmap_cfg = &axp519_regmap_config;
		ext->regmap_irq_chip = &axp519_regmap_irq_chip;
//		ext->dts_parse = axp519_dts_parse;
		break;
/**************************************/
	case AXP2601_ID:
		ext->nr_cells = ARRAY_SIZE(axp2601_cells);
		ext->cells = axp2601_cells;
		ext->regmap_cfg = &axp2601_regmap_config;
		ext->regmap_irq_chip = &axp2601_regmap_irq_chip;
//		ext->dts_parse = eta6973_dts_parse;
		break;
/**************************************/
	case AXP2602_ID:
		ext->nr_cells = ARRAY_SIZE(axp2602_cells);
		ext->cells = axp2602_cells;
		ext->regmap_cfg = &axp2602_regmap_config;
		ext->regmap_irq_chip = &axp2602_regmap_irq_chip;
//		ext->dts_parse = eta6973_dts_parse;
		break;
/**************************************/
	default:
		PMIC_DEV_ERR(dev, "unsupported ext ID %lu\n", ext->variant);
		return -EINVAL;
	}
	PMIC_DEV_INFO(dev, "bmu_ext_dev variant %s found\n",
		 bmu_ext_model_names[ext->variant]);

	return 0;
}
EXPORT_SYMBOL(bmu_ext_match_device);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 99)) && IS_ENABLED(CONFIG_THERMAL)
/* thermal cooling device callbacks */
static int ps_get_max_charge_cntl_limit(struct thermal_cooling_device *tcd,
					unsigned long *state)
{
	struct power_supply *psy;
	union power_supply_propval val;
	int ret;

	psy = tcd->devdata;
	ret = power_supply_get_property(psy,
			POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX, &val);
	if (ret)
		return ret;

	*state = val.intval;

	return ret;
}

static int ps_get_cur_charge_cntl_limit(struct thermal_cooling_device *tcd,
					unsigned long *state)
{
	struct power_supply *psy;
	union power_supply_propval val;
	int ret;

	psy = tcd->devdata;
	ret = power_supply_get_property(psy,
			POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT, &val);
	if (ret)
		return ret;

	*state = val.intval;

	return ret;
}

static int ps_set_cur_charge_cntl_limit(struct thermal_cooling_device *tcd,
					unsigned long state)
{
	struct power_supply *psy;
	union power_supply_propval val;
	int ret;

	psy = tcd->devdata;
	val.intval = state;
	ret = psy->desc->set_property(psy,
		POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT, &val);

	return ret;
}

static const struct thermal_cooling_device_ops psy_tcd_ops = {
	.get_max_state = ps_get_max_charge_cntl_limit,
	.get_cur_state = ps_get_cur_charge_cntl_limit,
	.set_cur_state = ps_set_cur_charge_cntl_limit,
};

int bmu_ext_register_cooler(struct power_supply *psy)
{
	psy->tcd = devm_thermal_of_cooling_device_register(&psy->dev,
		psy->of_node, (char *)psy->desc->name, psy, &psy_tcd_ops);

	return PTR_ERR_OR_ZERO(psy->tcd);
}
EXPORT_SYMBOL(bmu_ext_register_cooler);

void bmu_ext_unregister_cooler(struct power_supply *psy)
{
	if (IS_ERR_OR_NULL(psy->tcd))
		return;
	thermal_cooling_device_unregister(psy->tcd);
}
EXPORT_SYMBOL(bmu_ext_unregister_cooler);
#else
int bmu_ext_register_cooler(struct power_supply *psy)
{
	return 0;
}
EXPORT_SYMBOL(bmu_ext_register_cooler);

void bmu_ext_unregister_cooler(struct power_supply *psy)
{
	return;
}
EXPORT_SYMBOL(bmu_ext_unregister_cooler);
#endif

int bmu_ext_device_init(struct bmu_ext_dev *ext)
{
	int ret;

	if (ext->dts_parse)
		ext->dts_parse(ext);

	if (ext->irq) {
		ret = regmap_add_irq_chip(ext->regmap, ext->irq,
					  IRQF_ONESHOT | IRQF_SHARED, -1,
					  ext->regmap_irq_chip,
					  &ext->regmap_irqc);
		if (ret) {
			PMIC_DEV_ERR(ext->dev, "failed to add irq chip: %d\n", ret);
			return ret;
		}
	}
	ret = mfd_add_devices(ext->dev, 0, ext->cells,
			      ext->nr_cells, NULL, 0, NULL);
	if (ret) {
		PMIC_DEV_ERR(ext->dev, "failed to add MFD devices: %d\n", ret);

		if (ext->irq)
			regmap_del_irq_chip(ext->irq, ext->regmap_irqc);

		return ret;
	}

	PMIC_DEV_INFO(ext->dev, "bmu_ext:%s driver loaded\n", bmu_ext_model_names[ext->variant]);

	return 0;
}
EXPORT_SYMBOL_GPL(bmu_ext_device_init);

int bmu_ext_device_exit(struct bmu_ext_dev *ext)
{
	mfd_remove_devices(ext->dev);

	if (ext->irq)
		regmap_del_irq_chip(ext->irq, ext->regmap_irqc);
	return 0;
}
EXPORT_SYMBOL_GPL(bmu_ext_device_exit);

MODULE_AUTHOR("xinouyang <xinouyang@allwinnertech.com>");
MODULE_DESCRIPTION("bmu_ext MFD Driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
