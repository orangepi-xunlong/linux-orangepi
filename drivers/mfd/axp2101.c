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

#include <linux/err.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/mfd/axp2101.h>
#include <linux/mfd/core.h>
#include <linux/of_device.h>
#include <linux/acpi.h>

#define AXP20X_OFF	0x80

static const char *const axp20x_model_names[] = {
	"AXP152", "AXP202", "AXP209", "AXP221",  "AXP223",
	"AXP288", "AXP806", "AXP809", "AXP2101", "AXP15",
	"AXP1530", "AXP858", "AXP803", "AXP2202",
};

static const struct regmap_range axp152_writeable_ranges[] = {
	regmap_reg_range(AXP152_LDO3456_DC1234_CTRL, AXP152_IRQ3_STATE),
	regmap_reg_range(AXP152_DCDC_MODE, AXP152_PWM1_DUTY_CYCLE),
};

static const struct regmap_range axp152_volatile_ranges[] = {
	regmap_reg_range(AXP152_PWR_OP_MODE, AXP152_PWR_OP_MODE),
	regmap_reg_range(AXP152_IRQ1_EN, AXP152_IRQ3_STATE),
	regmap_reg_range(AXP152_GPIO_INPUT, AXP152_GPIO_INPUT),
};

static const struct regmap_access_table axp152_writeable_table = {
	.yes_ranges	= axp152_writeable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp152_writeable_ranges),
};

static const struct regmap_access_table axp152_volatile_table = {
	.yes_ranges	= axp152_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp152_volatile_ranges),
};

static const struct regmap_range axp20x_writeable_ranges[] = {
	regmap_reg_range(AXP20X_DATACACHE(0), AXP20X_IRQ5_STATE),
	regmap_reg_range(AXP20X_DCDC_MODE, AXP20X_FG_RES),
	regmap_reg_range(AXP20X_RDC_H, AXP20X_OCV(AXP20X_OCV_MAX)),
};

static const struct regmap_range axp20x_volatile_ranges[] = {
	regmap_reg_range(AXP20X_PWR_INPUT_STATUS, AXP20X_USB_OTG_STATUS),
	regmap_reg_range(AXP20X_CHRG_CTRL1, AXP20X_CHRG_CTRL2),
	regmap_reg_range(AXP20X_IRQ1_EN, AXP20X_IRQ5_STATE),
	regmap_reg_range(AXP20X_ACIN_V_ADC_H, AXP20X_IPSOUT_V_HIGH_L),
	regmap_reg_range(AXP20X_GPIO20_SS, AXP20X_GPIO3_CTRL),
	regmap_reg_range(AXP20X_FG_RES, AXP20X_RDC_L),
};

static const struct regmap_access_table axp20x_writeable_table = {
	.yes_ranges	= axp20x_writeable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp20x_writeable_ranges),
};

static const struct regmap_access_table axp20x_volatile_table = {
	.yes_ranges	= axp20x_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp20x_volatile_ranges),
};

/* AXP22x ranges are shared with the AXP809, as they cover the same range */
static const struct regmap_range axp22x_writeable_ranges[] = {
	regmap_reg_range(AXP20X_DATACACHE(0), AXP20X_IRQ5_STATE),
	regmap_reg_range(AXP20X_DCDC_MODE, AXP22X_BATLOW_THRES1),
};

static const struct regmap_range axp22x_volatile_ranges[] = {
	regmap_reg_range(AXP20X_PWR_INPUT_STATUS, AXP20X_PWR_OP_MODE),
	regmap_reg_range(AXP20X_IRQ1_EN, AXP20X_IRQ5_STATE),
	regmap_reg_range(AXP22X_GPIO_STATE, AXP22X_GPIO_STATE),
	regmap_reg_range(AXP20X_FG_RES, AXP20X_FG_RES),
};

static const struct regmap_access_table axp22x_writeable_table = {
	.yes_ranges	= axp22x_writeable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp22x_writeable_ranges),
};

static const struct regmap_access_table axp22x_volatile_table = {
	.yes_ranges	= axp22x_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp22x_volatile_ranges),
};

static const struct regmap_range axp288_writeable_ranges[] = {
	regmap_reg_range(AXP20X_DATACACHE(0), AXP20X_IRQ6_STATE),
	regmap_reg_range(AXP20X_DCDC_MODE, AXP288_FG_TUNE5),
};

static const struct regmap_range axp288_volatile_ranges[] = {
	regmap_reg_range(AXP20X_IRQ1_EN, AXP20X_IPSOUT_V_HIGH_L),
};

static const struct regmap_access_table axp288_writeable_table = {
	.yes_ranges	= axp288_writeable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp288_writeable_ranges),
};

static const struct regmap_access_table axp288_volatile_table = {
	.yes_ranges	= axp288_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp288_volatile_ranges),
};

static const struct regmap_range axp806_writeable_ranges[] = {
	regmap_reg_range(AXP806_STARTUP_SRC, AXP806_REG_ADDR_EXT),
};

static const struct regmap_range axp806_volatile_ranges[] = {
	regmap_reg_range(AXP20X_IRQ1_STATE, AXP20X_IRQ2_STATE),
};

static const struct regmap_access_table axp806_writeable_table = {
	.yes_ranges	= axp806_writeable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp806_writeable_ranges),
};

static const struct regmap_access_table axp806_volatile_table = {
	.yes_ranges	= axp806_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp806_volatile_ranges),
};

static const struct regmap_range axp2101_writeable_ranges[] = {
	regmap_reg_range(AXP2101_COMM_STAT0, AXP2101_BUFFERC),
};

static const struct regmap_range axp2101_volatile_ranges[] = {
	regmap_reg_range(AXP2101_COMM_STAT0, AXP2101_BUFFERC),
};

static const struct regmap_access_table axp2101_writeable_table = {
	.yes_ranges	= axp2101_writeable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp2101_writeable_ranges),
};

static const struct regmap_access_table axp2101_volatile_table = {
	.yes_ranges	= axp2101_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp2101_volatile_ranges),
};
/***********************/
static const struct regmap_range axp15_writeable_ranges[] = {
	regmap_reg_range(AXP15_STATUS, AXP15_GPIO0123_SIGNAL),
};

static const struct regmap_range axp15_volatile_ranges[] = {
	regmap_reg_range(AXP15_STATUS, AXP15_GPIO0123_SIGNAL),
};

static const struct regmap_access_table axp15_writeable_table = {
	.yes_ranges	= axp15_writeable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp15_writeable_ranges),
};

static const struct regmap_access_table axp15_volatile_table = {
	.yes_ranges	= axp15_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp15_volatile_ranges),
};
/***********************/
static const struct regmap_range axp1530_writeable_ranges[] = {
	regmap_reg_range(AXP1530_ON_INDICATE, AXP1530_FREQUENCY),
};

static const struct regmap_range axp1530_volatile_ranges[] = {
	regmap_reg_range(AXP1530_ON_INDICATE, AXP1530_FREQUENCY),
};

static const struct regmap_access_table axp1530_writeable_table = {
	.yes_ranges	= axp1530_writeable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp1530_writeable_ranges),
};

static const struct regmap_access_table axp1530_volatile_table = {
	.yes_ranges	= axp1530_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp1530_volatile_ranges),
};
/***********************/
static const struct regmap_range axp858_writeable_ranges[] = {
	regmap_reg_range(AXP858_ON_INDICATE, AXP858_FREQUENCY_ALDO2),
};

static const struct regmap_range axp858_volatile_ranges[] = {
	regmap_reg_range(AXP858_ON_INDICATE, AXP858_FREQUENCY_ALDO2),
};

static const struct regmap_access_table axp858_writeable_table = {
	.yes_ranges	= axp858_writeable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp858_writeable_ranges),
};

static const struct regmap_access_table axp858_volatile_table = {
	.yes_ranges	= axp858_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp858_volatile_ranges),
};


static const struct regmap_range axp803_writeable_ranges[] = {
	regmap_reg_range(AXP803_STATUS, AXP803_REG_ADDR_EXT),
};

static const struct regmap_range axp803_volatile_ranges[] = {
	regmap_reg_range(AXP803_STATUS, AXP803_REG_ADDR_EXT),
};

static const struct regmap_access_table axp803_writeable_table = {
	.yes_ranges	= axp803_writeable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp803_writeable_ranges),
};

static const struct regmap_access_table axp803_volatile_table = {
	.yes_ranges	= axp803_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp803_volatile_ranges),
};

static const struct regmap_range axp2202_writeable_ranges[] = {
	regmap_reg_range(AXP2202_COMM_STAT0,  AXP2202_TWI_ADDR_EXT)
};

static const struct regmap_range axp2202_volatile_ranges[] = {
	regmap_reg_range(AXP2202_COMM_STAT0, AXP2202_TWI_ADDR_EXT),
};

static const struct regmap_access_table axp2202_writeable_table = {
	.yes_ranges	= axp2202_writeable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp2202_writeable_ranges),
};

static const struct regmap_access_table axp2202_volatile_table = {
	.yes_ranges	= axp2202_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp2202_volatile_ranges),
};

/*---------------*/
static struct resource axp152_pek_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP152_IRQ_PEK_RIS_EDGE, "PEK_DBR"),
	DEFINE_RES_IRQ_NAMED(AXP152_IRQ_PEK_FAL_EDGE, "PEK_DBF"),
};

static struct resource axp20x_ac_power_supply_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP20X_IRQ_ACIN_PLUGIN, "ACIN_PLUGIN"),
	DEFINE_RES_IRQ_NAMED(AXP20X_IRQ_ACIN_REMOVAL, "ACIN_REMOVAL"),
	DEFINE_RES_IRQ_NAMED(AXP20X_IRQ_ACIN_OVER_V, "ACIN_OVER_V"),
};

static struct resource axp20x_pek_resources[] = {
	{
		.name	= "PEK_DBR",
		.start	= AXP20X_IRQ_PEK_RIS_EDGE,
		.end	= AXP20X_IRQ_PEK_RIS_EDGE,
		.flags	= IORESOURCE_IRQ,
	}, {
		.name	= "PEK_DBF",
		.start	= AXP20X_IRQ_PEK_FAL_EDGE,
		.end	= AXP20X_IRQ_PEK_FAL_EDGE,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct resource axp20x_usb_power_supply_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP20X_IRQ_VBUS_PLUGIN, "VBUS_PLUGIN"),
	DEFINE_RES_IRQ_NAMED(AXP20X_IRQ_VBUS_REMOVAL, "VBUS_REMOVAL"),
	DEFINE_RES_IRQ_NAMED(AXP20X_IRQ_VBUS_VALID, "VBUS_VALID"),
	DEFINE_RES_IRQ_NAMED(AXP20X_IRQ_VBUS_NOT_VALID, "VBUS_NOT_VALID"),
};

static struct resource axp22x_usb_power_supply_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP22X_IRQ_VBUS_PLUGIN, "VBUS_PLUGIN"),
	DEFINE_RES_IRQ_NAMED(AXP22X_IRQ_VBUS_REMOVAL, "VBUS_REMOVAL"),
};

static struct resource axp22x_pek_resources[] = {
	{
		.name   = "PEK_DBR",
		.start  = AXP22X_IRQ_PEK_RIS_EDGE,
		.end    = AXP22X_IRQ_PEK_RIS_EDGE,
		.flags  = IORESOURCE_IRQ,
	}, {
		.name   = "PEK_DBF",
		.start  = AXP22X_IRQ_PEK_FAL_EDGE,
		.end    = AXP22X_IRQ_PEK_FAL_EDGE,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct resource axp288_power_button_resources[] = {
	{
		.name	= "PEK_DBR",
		.start	= AXP288_IRQ_POKP,
		.end	= AXP288_IRQ_POKP,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name	= "PEK_DBF",
		.start	= AXP288_IRQ_POKN,
		.end	= AXP288_IRQ_POKN,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct resource axp288_fuel_gauge_resources[] = {
	{
		.start = AXP288_IRQ_QWBTU,
		.end   = AXP288_IRQ_QWBTU,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = AXP288_IRQ_WBTU,
		.end   = AXP288_IRQ_WBTU,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = AXP288_IRQ_QWBTO,
		.end   = AXP288_IRQ_QWBTO,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = AXP288_IRQ_WBTO,
		.end   = AXP288_IRQ_WBTO,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = AXP288_IRQ_WL2,
		.end   = AXP288_IRQ_WL2,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = AXP288_IRQ_WL1,
		.end   = AXP288_IRQ_WL1,
		.flags = IORESOURCE_IRQ,
	},
};

static struct resource axp809_pek_resources[] = {
	{
		.name   = "PEK_DBR",
		.start  = AXP809_IRQ_PEK_RIS_EDGE,
		.end    = AXP809_IRQ_PEK_RIS_EDGE,
		.flags  = IORESOURCE_IRQ,
	}, {
		.name   = "PEK_DBF",
		.start  = AXP809_IRQ_PEK_FAL_EDGE,
		.end    = AXP809_IRQ_PEK_FAL_EDGE,
		.flags  = IORESOURCE_IRQ,
	},
};

static const struct regmap_config axp152_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.wr_table	= &axp152_writeable_table,
	.volatile_table	= &axp152_volatile_table,
	.max_register	= AXP152_PWM1_DUTY_CYCLE,
	.cache_type	= REGCACHE_RBTREE,
};

static const struct regmap_config axp20x_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.wr_table	= &axp20x_writeable_table,
	.volatile_table	= &axp20x_volatile_table,
	.max_register	= AXP20X_OCV(AXP20X_OCV_MAX),
	.cache_type	= REGCACHE_RBTREE,
};

static const struct regmap_config axp22x_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.wr_table	= &axp22x_writeable_table,
	.volatile_table	= &axp22x_volatile_table,
	.max_register	= AXP22X_BATLOW_THRES1,
	.cache_type	= REGCACHE_RBTREE,
};

static const struct regmap_config axp288_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.wr_table	= &axp288_writeable_table,
	.volatile_table	= &axp288_volatile_table,
	.max_register	= AXP288_FG_TUNE5,
	.cache_type	= REGCACHE_RBTREE,
};

static const struct regmap_config axp806_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.wr_table	= &axp806_writeable_table,
	.volatile_table	= &axp806_volatile_table,
	.max_register	= AXP806_VREF_TEMP_WARN_L,
	.cache_type	= REGCACHE_RBTREE,
};


static const struct regmap_config axp2101_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.wr_table	= &axp2101_writeable_table,
	.volatile_table	= &axp2101_volatile_table,
	.max_register	= AXP2101_BUFFERC,
	.use_single_read = true,
	.use_single_write = true,
	.cache_type	= REGCACHE_RBTREE,
};
/******************************/
static const struct regmap_config axp15_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.wr_table	= &axp15_writeable_table,
	.volatile_table	= &axp15_volatile_table,
	.max_register	= AXP15_GPIO0123_SIGNAL,
	.use_single_read = true,
	.use_single_write = true,
	.cache_type	= REGCACHE_RBTREE,
};
/******************************/
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
/******************************/
static const struct regmap_config axp858_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.wr_table	= &axp858_writeable_table,
	.volatile_table	= &axp858_volatile_table,
	.max_register	= AXP858_FREQUENCY_ALDO2,
	.use_single_read = true,
	.use_single_write = true,
	.cache_type	= REGCACHE_RBTREE,
};

static const struct regmap_config axp803_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.wr_table	= &axp803_writeable_table,
	.volatile_table	= &axp803_volatile_table,
	.max_register	= AXP803_REG_ADDR_EXT,
	.use_single_read = true,
	.use_single_write = true,
	.cache_type	= REGCACHE_RBTREE,
};

static const struct regmap_config axp2202_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.wr_table	= &axp2202_writeable_table,
	.volatile_table	= &axp2202_volatile_table,
	.max_register	= AXP2202_TWI_ADDR_EXT,
	.use_single_read = true,
	.use_single_write = true,
	.cache_type	= REGCACHE_NONE,
};

/*------------------*/
#define INIT_REGMAP_IRQ(_variant, _irq, _off, _mask)			\
	[_variant##_IRQ_##_irq] = { .reg_offset = (_off), .mask = BIT(_mask) }

static const struct regmap_irq axp152_regmap_irqs[] = {
	INIT_REGMAP_IRQ(AXP152, LDO0IN_CONNECT,		0, 6),
	INIT_REGMAP_IRQ(AXP152, LDO0IN_REMOVAL,		0, 5),
	INIT_REGMAP_IRQ(AXP152, ALDO0IN_CONNECT,	0, 3),
	INIT_REGMAP_IRQ(AXP152, ALDO0IN_REMOVAL,	0, 2),
	INIT_REGMAP_IRQ(AXP152, DCDC1_V_LOW,		1, 5),
	INIT_REGMAP_IRQ(AXP152, DCDC2_V_LOW,		1, 4),
	INIT_REGMAP_IRQ(AXP152, DCDC3_V_LOW,		1, 3),
	INIT_REGMAP_IRQ(AXP152, DCDC4_V_LOW,		1, 2),
	INIT_REGMAP_IRQ(AXP152, PEK_SHORT,		1, 1),
	INIT_REGMAP_IRQ(AXP152, PEK_LONG,		1, 0),
	INIT_REGMAP_IRQ(AXP152, TIMER,			2, 7),
	INIT_REGMAP_IRQ(AXP152, PEK_RIS_EDGE,		2, 6),
	INIT_REGMAP_IRQ(AXP152, PEK_FAL_EDGE,		2, 5),
	INIT_REGMAP_IRQ(AXP152, GPIO3_INPUT,		2, 3),
	INIT_REGMAP_IRQ(AXP152, GPIO2_INPUT,		2, 2),
	INIT_REGMAP_IRQ(AXP152, GPIO1_INPUT,		2, 1),
	INIT_REGMAP_IRQ(AXP152, GPIO0_INPUT,		2, 0),
};

static const struct regmap_irq axp20x_regmap_irqs[] = {
	INIT_REGMAP_IRQ(AXP20X, ACIN_OVER_V,		0, 7),
	INIT_REGMAP_IRQ(AXP20X, ACIN_PLUGIN,		0, 6),
	INIT_REGMAP_IRQ(AXP20X, ACIN_REMOVAL,	        0, 5),
	INIT_REGMAP_IRQ(AXP20X, VBUS_OVER_V,		0, 4),
	INIT_REGMAP_IRQ(AXP20X, VBUS_PLUGIN,		0, 3),
	INIT_REGMAP_IRQ(AXP20X, VBUS_REMOVAL,	        0, 2),
	INIT_REGMAP_IRQ(AXP20X, VBUS_V_LOW,		0, 1),
	INIT_REGMAP_IRQ(AXP20X, BATT_PLUGIN,		1, 7),
	INIT_REGMAP_IRQ(AXP20X, BATT_REMOVAL,	        1, 6),
	INIT_REGMAP_IRQ(AXP20X, BATT_ENT_ACT_MODE,	1, 5),
	INIT_REGMAP_IRQ(AXP20X, BATT_EXIT_ACT_MODE,	1, 4),
	INIT_REGMAP_IRQ(AXP20X, CHARG,		        1, 3),
	INIT_REGMAP_IRQ(AXP20X, CHARG_DONE,		1, 2),
	INIT_REGMAP_IRQ(AXP20X, BATT_TEMP_HIGH,	        1, 1),
	INIT_REGMAP_IRQ(AXP20X, BATT_TEMP_LOW,	        1, 0),
	INIT_REGMAP_IRQ(AXP20X, DIE_TEMP_HIGH,	        2, 7),
	INIT_REGMAP_IRQ(AXP20X, CHARG_I_LOW,		2, 6),
	INIT_REGMAP_IRQ(AXP20X, DCDC1_V_LONG,	        2, 5),
	INIT_REGMAP_IRQ(AXP20X, DCDC2_V_LONG,	        2, 4),
	INIT_REGMAP_IRQ(AXP20X, DCDC3_V_LONG,	        2, 3),
	INIT_REGMAP_IRQ(AXP20X, PEK_SHORT,		2, 1),
	INIT_REGMAP_IRQ(AXP20X, PEK_LONG,		2, 0),
	INIT_REGMAP_IRQ(AXP20X, N_OE_PWR_ON,		3, 7),
	INIT_REGMAP_IRQ(AXP20X, N_OE_PWR_OFF,	        3, 6),
	INIT_REGMAP_IRQ(AXP20X, VBUS_VALID,		3, 5),
	INIT_REGMAP_IRQ(AXP20X, VBUS_NOT_VALID,	        3, 4),
	INIT_REGMAP_IRQ(AXP20X, VBUS_SESS_VALID,	3, 3),
	INIT_REGMAP_IRQ(AXP20X, VBUS_SESS_END,	        3, 2),
	INIT_REGMAP_IRQ(AXP20X, LOW_PWR_LVL1,	        3, 1),
	INIT_REGMAP_IRQ(AXP20X, LOW_PWR_LVL2,	        3, 0),
	INIT_REGMAP_IRQ(AXP20X, TIMER,		        4, 7),
	INIT_REGMAP_IRQ(AXP20X, PEK_RIS_EDGE,	        4, 6),
	INIT_REGMAP_IRQ(AXP20X, PEK_FAL_EDGE,	        4, 5),
	INIT_REGMAP_IRQ(AXP20X, GPIO3_INPUT,		4, 3),
	INIT_REGMAP_IRQ(AXP20X, GPIO2_INPUT,		4, 2),
	INIT_REGMAP_IRQ(AXP20X, GPIO1_INPUT,		4, 1),
	INIT_REGMAP_IRQ(AXP20X, GPIO0_INPUT,		4, 0),
};

static const struct regmap_irq axp22x_regmap_irqs[] = {
	INIT_REGMAP_IRQ(AXP22X, ACIN_OVER_V,		0, 7),
	INIT_REGMAP_IRQ(AXP22X, ACIN_PLUGIN,		0, 6),
	INIT_REGMAP_IRQ(AXP22X, ACIN_REMOVAL,	        0, 5),
	INIT_REGMAP_IRQ(AXP22X, VBUS_OVER_V,		0, 4),
	INIT_REGMAP_IRQ(AXP22X, VBUS_PLUGIN,		0, 3),
	INIT_REGMAP_IRQ(AXP22X, VBUS_REMOVAL,	        0, 2),
	INIT_REGMAP_IRQ(AXP22X, VBUS_V_LOW,		0, 1),
	INIT_REGMAP_IRQ(AXP22X, BATT_PLUGIN,		1, 7),
	INIT_REGMAP_IRQ(AXP22X, BATT_REMOVAL,	        1, 6),
	INIT_REGMAP_IRQ(AXP22X, BATT_ENT_ACT_MODE,	1, 5),
	INIT_REGMAP_IRQ(AXP22X, BATT_EXIT_ACT_MODE,	1, 4),
	INIT_REGMAP_IRQ(AXP22X, CHARG,		        1, 3),
	INIT_REGMAP_IRQ(AXP22X, CHARG_DONE,		1, 2),
	INIT_REGMAP_IRQ(AXP22X, BATT_TEMP_HIGH,	        1, 1),
	INIT_REGMAP_IRQ(AXP22X, BATT_TEMP_LOW,	        1, 0),
	INIT_REGMAP_IRQ(AXP22X, DIE_TEMP_HIGH,	        2, 7),
	INIT_REGMAP_IRQ(AXP22X, PEK_SHORT,		2, 1),
	INIT_REGMAP_IRQ(AXP22X, PEK_LONG,		2, 0),
	INIT_REGMAP_IRQ(AXP22X, LOW_PWR_LVL1,	        3, 1),
	INIT_REGMAP_IRQ(AXP22X, LOW_PWR_LVL2,	        3, 0),
	INIT_REGMAP_IRQ(AXP22X, TIMER,		        4, 7),
	INIT_REGMAP_IRQ(AXP22X, PEK_RIS_EDGE,	        4, 6),
	INIT_REGMAP_IRQ(AXP22X, PEK_FAL_EDGE,	        4, 5),
	INIT_REGMAP_IRQ(AXP22X, GPIO1_INPUT,		4, 1),
	INIT_REGMAP_IRQ(AXP22X, GPIO0_INPUT,		4, 0),
};

/* some IRQs are compatible with axp20x models */
static const struct regmap_irq axp288_regmap_irqs[] = {
	INIT_REGMAP_IRQ(AXP288, VBUS_FALL,              0, 2),
	INIT_REGMAP_IRQ(AXP288, VBUS_RISE,              0, 3),
	INIT_REGMAP_IRQ(AXP288, OV,                     0, 4),

	INIT_REGMAP_IRQ(AXP288, DONE,                   1, 2),
	INIT_REGMAP_IRQ(AXP288, CHARGING,               1, 3),
	INIT_REGMAP_IRQ(AXP288, SAFE_QUIT,              1, 4),
	INIT_REGMAP_IRQ(AXP288, SAFE_ENTER,             1, 5),
	INIT_REGMAP_IRQ(AXP288, ABSENT,                 1, 6),
	INIT_REGMAP_IRQ(AXP288, APPEND,                 1, 7),

	INIT_REGMAP_IRQ(AXP288, QWBTU,                  2, 0),
	INIT_REGMAP_IRQ(AXP288, WBTU,                   2, 1),
	INIT_REGMAP_IRQ(AXP288, QWBTO,                  2, 2),
	INIT_REGMAP_IRQ(AXP288, WBTO,                   2, 3),
	INIT_REGMAP_IRQ(AXP288, QCBTU,                  2, 4),
	INIT_REGMAP_IRQ(AXP288, CBTU,                   2, 5),
	INIT_REGMAP_IRQ(AXP288, QCBTO,                  2, 6),
	INIT_REGMAP_IRQ(AXP288, CBTO,                   2, 7),

	INIT_REGMAP_IRQ(AXP288, WL2,                    3, 0),
	INIT_REGMAP_IRQ(AXP288, WL1,                    3, 1),
	INIT_REGMAP_IRQ(AXP288, GPADC,                  3, 2),
	INIT_REGMAP_IRQ(AXP288, OT,                     3, 7),

	INIT_REGMAP_IRQ(AXP288, GPIO0,                  4, 0),
	INIT_REGMAP_IRQ(AXP288, GPIO1,                  4, 1),
	INIT_REGMAP_IRQ(AXP288, POKO,                   4, 2),
	INIT_REGMAP_IRQ(AXP288, POKL,                   4, 3),
	INIT_REGMAP_IRQ(AXP288, POKS,                   4, 4),
	INIT_REGMAP_IRQ(AXP288, POKN,                   4, 5),
	INIT_REGMAP_IRQ(AXP288, POKP,                   4, 6),
	INIT_REGMAP_IRQ(AXP288, TIMER,                  4, 7),

	INIT_REGMAP_IRQ(AXP288, MV_CHNG,                5, 0),
	INIT_REGMAP_IRQ(AXP288, BC_USB_CHNG,            5, 1),
};

static const struct regmap_irq axp806_regmap_irqs[] = {
	INIT_REGMAP_IRQ(AXP806, DIE_TEMP_HIGH_LV1,	0, 0),
	INIT_REGMAP_IRQ(AXP806, DIE_TEMP_HIGH_LV2,	0, 1),
	INIT_REGMAP_IRQ(AXP806, DCDCA_V_LOW,		0, 3),
	INIT_REGMAP_IRQ(AXP806, DCDCB_V_LOW,		0, 4),
	INIT_REGMAP_IRQ(AXP806, DCDCC_V_LOW,		0, 5),
	INIT_REGMAP_IRQ(AXP806, DCDCD_V_LOW,		0, 6),
	INIT_REGMAP_IRQ(AXP806, DCDCE_V_LOW,		0, 7),
	INIT_REGMAP_IRQ(AXP806, PWROK_LONG,		1, 0),
	INIT_REGMAP_IRQ(AXP806, PWROK_SHORT,		1, 1),
	INIT_REGMAP_IRQ(AXP806, WAKEUP,			1, 4),
	INIT_REGMAP_IRQ(AXP806, PWROK_FALL,		1, 5),
	INIT_REGMAP_IRQ(AXP806, PWROK_RISE,		1, 6),
};

static const struct regmap_irq axp809_regmap_irqs[] = {
	INIT_REGMAP_IRQ(AXP809, ACIN_OVER_V,		0, 7),
	INIT_REGMAP_IRQ(AXP809, ACIN_PLUGIN,		0, 6),
	INIT_REGMAP_IRQ(AXP809, ACIN_REMOVAL,	        0, 5),
	INIT_REGMAP_IRQ(AXP809, VBUS_OVER_V,		0, 4),
	INIT_REGMAP_IRQ(AXP809, VBUS_PLUGIN,		0, 3),
	INIT_REGMAP_IRQ(AXP809, VBUS_REMOVAL,	        0, 2),
	INIT_REGMAP_IRQ(AXP809, VBUS_V_LOW,		0, 1),
	INIT_REGMAP_IRQ(AXP809, BATT_PLUGIN,		1, 7),
	INIT_REGMAP_IRQ(AXP809, BATT_REMOVAL,	        1, 6),
	INIT_REGMAP_IRQ(AXP809, BATT_ENT_ACT_MODE,	1, 5),
	INIT_REGMAP_IRQ(AXP809, BATT_EXIT_ACT_MODE,	1, 4),
	INIT_REGMAP_IRQ(AXP809, CHARG,		        1, 3),
	INIT_REGMAP_IRQ(AXP809, CHARG_DONE,		1, 2),
	INIT_REGMAP_IRQ(AXP809, BATT_CHG_TEMP_HIGH,	2, 7),
	INIT_REGMAP_IRQ(AXP809, BATT_CHG_TEMP_HIGH_END,	2, 6),
	INIT_REGMAP_IRQ(AXP809, BATT_CHG_TEMP_LOW,	2, 5),
	INIT_REGMAP_IRQ(AXP809, BATT_CHG_TEMP_LOW_END,	2, 4),
	INIT_REGMAP_IRQ(AXP809, BATT_ACT_TEMP_HIGH,	2, 3),
	INIT_REGMAP_IRQ(AXP809, BATT_ACT_TEMP_HIGH_END,	2, 2),
	INIT_REGMAP_IRQ(AXP809, BATT_ACT_TEMP_LOW,	2, 1),
	INIT_REGMAP_IRQ(AXP809, BATT_ACT_TEMP_LOW_END,	2, 0),
	INIT_REGMAP_IRQ(AXP809, DIE_TEMP_HIGH,	        3, 7),
	INIT_REGMAP_IRQ(AXP809, LOW_PWR_LVL1,	        3, 1),
	INIT_REGMAP_IRQ(AXP809, LOW_PWR_LVL2,	        3, 0),
	INIT_REGMAP_IRQ(AXP809, TIMER,		        4, 7),
	INIT_REGMAP_IRQ(AXP809, PEK_RIS_EDGE,	        4, 6),
	INIT_REGMAP_IRQ(AXP809, PEK_FAL_EDGE,	        4, 5),
	INIT_REGMAP_IRQ(AXP809, PEK_SHORT,		4, 4),
	INIT_REGMAP_IRQ(AXP809, PEK_LONG,		4, 3),
	INIT_REGMAP_IRQ(AXP809, PEK_OVER_OFF,		4, 2),
	INIT_REGMAP_IRQ(AXP809, GPIO1_INPUT,		4, 1),
	INIT_REGMAP_IRQ(AXP809, GPIO0_INPUT,		4, 0),
};

static const struct regmap_irq axp2101_regmap_irqs[] = {
	INIT_REGMAP_IRQ(AXP2101, SOCWL2,		0, 7),
	INIT_REGMAP_IRQ(AXP2101, SOCWL1,		0, 6),
	INIT_REGMAP_IRQ(AXP2101, GWDT,			0, 5),
	INIT_REGMAP_IRQ(AXP2101, NEWSOC,		0, 4),
	INIT_REGMAP_IRQ(AXP2101, BCOT,			0, 3),
	INIT_REGMAP_IRQ(AXP2101, BCUT,			0, 2),
	INIT_REGMAP_IRQ(AXP2101, BWOT,			0, 1),
	INIT_REGMAP_IRQ(AXP2101, BWUT,			0, 0),
	INIT_REGMAP_IRQ(AXP2101, VINSET,		1, 7),
	INIT_REGMAP_IRQ(AXP2101, VREMOV,		1, 6),
	INIT_REGMAP_IRQ(AXP2101, BINSERT,		1, 5),
	INIT_REGMAP_IRQ(AXP2101, BREMOV,		1, 4),
	INIT_REGMAP_IRQ(AXP2101, PONS,			1, 3),
	INIT_REGMAP_IRQ(AXP2101, PONL,			1, 2),
	INIT_REGMAP_IRQ(AXP2101, PONN,			1, 1),
	INIT_REGMAP_IRQ(AXP2101, PONP,			1, 0),
	INIT_REGMAP_IRQ(AXP2101, WDEXP,			2, 7),
	INIT_REGMAP_IRQ(AXP2101, LDOOC,			2, 6),
	INIT_REGMAP_IRQ(AXP2101, BOCP,			2, 5),
	INIT_REGMAP_IRQ(AXP2101, CHGDN,			2, 4),
	INIT_REGMAP_IRQ(AXP2101, CHGST,			2, 3),
	INIT_REGMAP_IRQ(AXP2101, DOTL1,			2, 2),
	INIT_REGMAP_IRQ(AXP2101, CHGTE,			2, 1),
	INIT_REGMAP_IRQ(AXP2101, BOVP,			2, 0),
};
/********************************/
static const struct regmap_irq axp15_regmap_irqs[] = {
	INIT_REGMAP_IRQ(AXP15, LDO0IN_L2H,		0, 6),
	INIT_REGMAP_IRQ(AXP15, LDO0IN_H2L,			0, 5),
	INIT_REGMAP_IRQ(AXP15, ALDOIN_L2H,			0, 3),
	INIT_REGMAP_IRQ(AXP15, ALDOIN_H2L,			0, 2),
	INIT_REGMAP_IRQ(AXP15, DCDC1_V_LOW,		1, 5),
	INIT_REGMAP_IRQ(AXP15, DCDC2_V_LOW,		1, 4),
	INIT_REGMAP_IRQ(AXP15, DCDC3_V_LOW,			1, 3),
	INIT_REGMAP_IRQ(AXP15, DCDC4_V_LOW,			1, 2),
	INIT_REGMAP_IRQ(AXP15, PEKSH,			1, 1),
	INIT_REGMAP_IRQ(AXP15, PEKLO,			1, 0),
	INIT_REGMAP_IRQ(AXP15, EVENT_TIMEOUT,			2, 7),
	INIT_REGMAP_IRQ(AXP15, PEKRE,			2, 6),
	INIT_REGMAP_IRQ(AXP15, PEKFE,			2, 5),
	INIT_REGMAP_IRQ(AXP15, GPIO3,			2, 3),
	INIT_REGMAP_IRQ(AXP15, GPIO2,			2, 2),
	INIT_REGMAP_IRQ(AXP15, GPIO1,			2, 1),
	INIT_REGMAP_IRQ(AXP15, GPIO0,			2, 0),
};
/********************************/
static const struct regmap_irq axp1530_regmap_irqs[] = {
	INIT_REGMAP_IRQ(AXP1530, KEY_L2H_EN,		0, 7),
	INIT_REGMAP_IRQ(AXP1530, KEY_H2L_EN,		0, 6),
	INIT_REGMAP_IRQ(AXP1530, POKSIRQ_EN,		0, 5),
	INIT_REGMAP_IRQ(AXP1530, POKLIRQ_EN,		0, 4),
	INIT_REGMAP_IRQ(AXP1530, DCDC3_UNDER,		0, 3),
	INIT_REGMAP_IRQ(AXP1530, DCDC2_UNDER,		0, 2),
	INIT_REGMAP_IRQ(AXP1530, TEMP_OVER,			0, 0),
};
/********************************/
static const struct regmap_irq axp858_regmap_irqs[] = {
	INIT_REGMAP_IRQ(AXP858, DCDC3_CUR_OVER,		1, 7),
	INIT_REGMAP_IRQ(AXP858, DCDC2_CUR_OVER,		1, 6),
	INIT_REGMAP_IRQ(AXP858, GPIO2_EN,			1, 5),
	INIT_REGMAP_IRQ(AXP858, POKPIRQ_EN,			1, 4),
	INIT_REGMAP_IRQ(AXP858, POKNIRQ_EN,			1, 3),
	INIT_REGMAP_IRQ(AXP858, GPIO1_EN,			1, 2),
	INIT_REGMAP_IRQ(AXP858, POKSIRQ_EN,			1, 1),
	INIT_REGMAP_IRQ(AXP858, POKLIRQ_EN,			1, 0),
	INIT_REGMAP_IRQ(AXP858, DCDC6_UNDER,		0, 7),
	INIT_REGMAP_IRQ(AXP858, DCDC5_UNDER,		0, 6),
	INIT_REGMAP_IRQ(AXP858, DCDC4_UNDER,		0, 5),
	INIT_REGMAP_IRQ(AXP858, DCDC3_UNDER,		0, 4),
	INIT_REGMAP_IRQ(AXP858, DCDC2_UNDER,		0, 3),
	INIT_REGMAP_IRQ(AXP858, DCDC1_UNDER,		0, 2),
	INIT_REGMAP_IRQ(AXP858, TEMP_OVER2,			0, 1),
	INIT_REGMAP_IRQ(AXP858, TEMP_OVER1,			0, 0),
};
/*------------------*/
static const struct regmap_irq axp803_regmap_irqs[] = {
	INIT_REGMAP_IRQ(AXP803, ACOV,			0, 7),
	INIT_REGMAP_IRQ(AXP803, ACIN,			0, 6),
	INIT_REGMAP_IRQ(AXP803, ACRE,			0, 5),
	INIT_REGMAP_IRQ(AXP803, USBOV,			0, 4),
	INIT_REGMAP_IRQ(AXP803, USBIN,			0, 3),
	INIT_REGMAP_IRQ(AXP803, USBRE,			0, 2),
	INIT_REGMAP_IRQ(AXP803, BATIN,			1, 7),
	INIT_REGMAP_IRQ(AXP803, BATRE,			1, 6),
	INIT_REGMAP_IRQ(AXP803, BATATIN,		1, 5),
	INIT_REGMAP_IRQ(AXP803, BATATOU,		1, 4),
	INIT_REGMAP_IRQ(AXP803, CHAST,			1, 3),
	INIT_REGMAP_IRQ(AXP803, CHAOV,			1, 2),
	INIT_REGMAP_IRQ(AXP803, BATOVCHG,		2, 7),
	INIT_REGMAP_IRQ(AXP803, QBATOVCHG,		2, 6),
	INIT_REGMAP_IRQ(AXP803, BATINCHG,		2, 5),
	INIT_REGMAP_IRQ(AXP803, QBATINCHG,		2, 4),
	INIT_REGMAP_IRQ(AXP803, BATOVWORK,		2, 3),
	INIT_REGMAP_IRQ(AXP803, QBATOVWORK,		2, 2),
	INIT_REGMAP_IRQ(AXP803, BATINWORK,		2, 1),
	INIT_REGMAP_IRQ(AXP803, QBATINWORK,		2, 0),
	INIT_REGMAP_IRQ(AXP803, LOWN1,			3, 1),
	INIT_REGMAP_IRQ(AXP803, LOWN2,			3, 0),
	INIT_REGMAP_IRQ(AXP803, TIMER,			4, 7),
	INIT_REGMAP_IRQ(AXP803, PEKRE,			4, 6),
	INIT_REGMAP_IRQ(AXP803, PEKFE,			4, 5),
	INIT_REGMAP_IRQ(AXP803, POKSH,			4, 4),
	INIT_REGMAP_IRQ(AXP803, POKLO,			4, 3),
	INIT_REGMAP_IRQ(AXP803, GPIO1,			4, 1),
	INIT_REGMAP_IRQ(AXP803, GPIO0,			4, 0),
};

static const struct regmap_irq axp2202_regmap_irqs[] = {
	INIT_REGMAP_IRQ(AXP2202, SOCWL2,      0, 7),
	INIT_REGMAP_IRQ(AXP2202, SOCWL1,      0, 6),
	INIT_REGMAP_IRQ(AXP2202, GWDT,        0, 5),
	INIT_REGMAP_IRQ(AXP2202, NEWSOC,      0, 4),
	INIT_REGMAP_IRQ(AXP2202, BST_OV,      0, 2),
	INIT_REGMAP_IRQ(AXP2202, VBUS_OV,     0, 1),
	INIT_REGMAP_IRQ(AXP2202, VBUS_FAULT,  0, 0),
	INIT_REGMAP_IRQ(AXP2202, VINSERT,     1, 7),
	INIT_REGMAP_IRQ(AXP2202, VREMOVE,     1, 6),
	INIT_REGMAP_IRQ(AXP2202, BINSERT,     1, 5),
	INIT_REGMAP_IRQ(AXP2202, BREMOVE,     1, 4),
	INIT_REGMAP_IRQ(AXP2202, PONS,        1, 3),
	INIT_REGMAP_IRQ(AXP2202, PONL,        1, 2),
	INIT_REGMAP_IRQ(AXP2202, PONN,        1, 1),
	INIT_REGMAP_IRQ(AXP2202, PONP,        1, 0),
	INIT_REGMAP_IRQ(AXP2202, WDEXP,       2, 7),
	INIT_REGMAP_IRQ(AXP2202, LDOOC,       2, 6),
	INIT_REGMAP_IRQ(AXP2202, BOCP,        2, 5),
	INIT_REGMAP_IRQ(AXP2202, CHGDN,       2, 4),
	INIT_REGMAP_IRQ(AXP2202, CHGST,       2, 3),
	INIT_REGMAP_IRQ(AXP2202, DOTL1,       2, 2),
	INIT_REGMAP_IRQ(AXP2202, CHGTE,       2, 1),
	INIT_REGMAP_IRQ(AXP2202, BOVP,        2, 0),
	INIT_REGMAP_IRQ(AXP2202, BC_DONE,     3, 7),
	INIT_REGMAP_IRQ(AXP2202, BC_CHNG,     3, 6),
	INIT_REGMAP_IRQ(AXP2202, RID_CHNG,    3, 5),
	INIT_REGMAP_IRQ(AXP2202, BCOTQ,       3, 4),
	INIT_REGMAP_IRQ(AXP2202, BCOT,        3, 3),
	INIT_REGMAP_IRQ(AXP2202, BCUT,        3, 2),
	INIT_REGMAP_IRQ(AXP2202, BWOT,        3, 1),
	INIT_REGMAP_IRQ(AXP2202, BWUT,        3, 0),
	INIT_REGMAP_IRQ(AXP2202, CREMOVE,     4, 6),
	INIT_REGMAP_IRQ(AXP2202, CINSERT,     4, 5),
	INIT_REGMAP_IRQ(AXP2202, TOGGLE_DONE, 4, 4),
	INIT_REGMAP_IRQ(AXP2202, VBUS_SAFE5V, 4, 3),
	INIT_REGMAP_IRQ(AXP2202, VBUS_SAFE0V, 4, 2),
	INIT_REGMAP_IRQ(AXP2202, ERR_GEN,     4, 1),
	INIT_REGMAP_IRQ(AXP2202, PWR_CHNG,    4, 0),
};

static const struct regmap_irq_chip axp152_regmap_irq_chip = {
	.name			= "axp152_irq_chip",
	.status_base		= AXP152_IRQ1_STATE,
	.ack_base		= AXP152_IRQ1_STATE,
	.mask_base		= AXP152_IRQ1_EN,
	.mask_invert		= true,
	.init_ack_masked	= true,
	.irqs			= axp152_regmap_irqs,
	.num_irqs		= ARRAY_SIZE(axp152_regmap_irqs),
	.num_regs		= 3,
};

static const struct regmap_irq_chip axp20x_regmap_irq_chip = {
	.name			= "axp20x_irq_chip",
	.status_base		= AXP20X_IRQ1_STATE,
	.ack_base		= AXP20X_IRQ1_STATE,
	.mask_base		= AXP20X_IRQ1_EN,
	.mask_invert		= true,
	.init_ack_masked	= true,
	.irqs			= axp20x_regmap_irqs,
	.num_irqs		= ARRAY_SIZE(axp20x_regmap_irqs),
	.num_regs		= 5,

};

static const struct regmap_irq_chip axp22x_regmap_irq_chip = {
	.name			= "axp22x_irq_chip",
	.status_base		= AXP20X_IRQ1_STATE,
	.ack_base		= AXP20X_IRQ1_STATE,
	.mask_base		= AXP20X_IRQ1_EN,
	.mask_invert		= true,
	.init_ack_masked	= true,
	.irqs			= axp22x_regmap_irqs,
	.num_irqs		= ARRAY_SIZE(axp22x_regmap_irqs),
	.num_regs		= 5,
};

static const struct regmap_irq_chip axp288_regmap_irq_chip = {
	.name			= "axp288_irq_chip",
	.status_base		= AXP20X_IRQ1_STATE,
	.ack_base		= AXP20X_IRQ1_STATE,
	.mask_base		= AXP20X_IRQ1_EN,
	.mask_invert		= true,
	.init_ack_masked	= true,
	.irqs			= axp288_regmap_irqs,
	.num_irqs		= ARRAY_SIZE(axp288_regmap_irqs),
	.num_regs		= 6,

};

static const struct regmap_irq_chip axp806_regmap_irq_chip = {
	.name			= "axp806",
	.status_base		= AXP20X_IRQ1_STATE,
	.ack_base		= AXP20X_IRQ1_STATE,
	.mask_base		= AXP20X_IRQ1_EN,
	.mask_invert		= true,
	.init_ack_masked	= true,
	.irqs			= axp806_regmap_irqs,
	.num_irqs		= ARRAY_SIZE(axp806_regmap_irqs),
	.num_regs		= 2,
};

static const struct regmap_irq_chip axp809_regmap_irq_chip = {
	.name			= "axp809",
	.status_base		= AXP20X_IRQ1_STATE,
	.ack_base		= AXP20X_IRQ1_STATE,
	.mask_base		= AXP20X_IRQ1_EN,
	.mask_invert		= true,
	.init_ack_masked	= true,
	.irqs			= axp809_regmap_irqs,
	.num_irqs		= ARRAY_SIZE(axp809_regmap_irqs),
	.num_regs		= 5,
};

static const struct regmap_irq_chip axp2101_regmap_irq_chip = {
	.name			= "axp2101_irq_chip",
	.status_base		= AXP2101_INTSTS1,
	.ack_base		= AXP2101_INTSTS1,
	.mask_base		= AXP2101_INTEN1,
	.mask_invert		= true,
	.init_ack_masked	= true,
	.irqs			= axp2101_regmap_irqs,
	.num_irqs		= ARRAY_SIZE(axp2101_regmap_irqs),
	.num_regs		= 3,

};
/********************************/
static const struct regmap_irq_chip axp15_regmap_irq_chip = {
	.name			= "axp15_irq_chip",
	.status_base		= AXP15_INTSTS1,
	.ack_base		= AXP15_INTSTS1,
	.mask_base		= AXP15_INTEN1,
	.mask_invert		= true,
	.init_ack_masked	= true,
	.irqs			= axp15_regmap_irqs,
	.num_irqs		= ARRAY_SIZE(axp15_regmap_irqs),
	.num_regs		= 3,
};
/********************************/
static const struct regmap_irq_chip axp1530_regmap_irq_chip = {
	.name			= "axp1530_irq_chip",
	.status_base		= AXP1530_IRQ_STATUS1,
	.ack_base		= AXP1530_IRQ_STATUS1,
	.mask_base		= AXP1530_IRQ_ENABLE1,
	.mask_invert		= true,
	.init_ack_masked	= true,
	.irqs			= axp1530_regmap_irqs,
	.num_irqs		= ARRAY_SIZE(axp1530_regmap_irqs),
	.num_regs		= 1,
};
/********************************/
static const struct regmap_irq_chip axp858_regmap_irq_chip = {
	.name			= "axp858_irq_chip",
	.status_base		= AXP858_IRQ_STS1,
	.ack_base		= AXP858_IRQ_STS1,
	.mask_base		= AXP858_IRQ_EN1,
	.mask_invert		= true,
	.init_ack_masked	= true,
	.irqs			= axp858_regmap_irqs,
	.num_irqs		= ARRAY_SIZE(axp858_regmap_irqs),
	.num_regs		= 2,
};

static const struct regmap_irq_chip axp803_regmap_irq_chip = {
	.name			= "axp803_irq_chip",
	.status_base		= AXP803_INTSTS1,
	.ack_base		= AXP803_INTSTS1,
	.mask_base		= AXP803_INTEN1,
	.mask_invert		= true,
	.init_ack_masked	= true,
	.irqs			= axp803_regmap_irqs,
	.num_irqs		= ARRAY_SIZE(axp803_regmap_irqs),
	.num_regs		= 6,
};

static const struct regmap_irq_chip axp2202_regmap_irq_chip = {
	.name			= "axp2202_irq_chip",
	.status_base		= AXP2202_IRQ0,
	.ack_base		= AXP2202_IRQ0,
	.mask_base		= AXP2202_IRQ_EN0,
	.mask_invert		= true,
	.init_ack_masked	= true,
	.irqs			= axp2202_regmap_irqs,
	.num_irqs		= ARRAY_SIZE(axp2202_regmap_irqs),
	.num_regs		= 5,
};

/*--------------------*/
static struct mfd_cell axp20x_cells[] = {
	{
		.name		= "axp20x-gpio",
		.of_compatible	= "x-powers,axp209-gpio",
	}, {
		.name		= "axp20x-pek",
		.num_resources	= ARRAY_SIZE(axp20x_pek_resources),
		.resources	= axp20x_pek_resources,
	}, {
		.name		= "axp20x-regulator",
	}, {
		.name		= "axp20x-ac-power-supply",
		.of_compatible	= "x-powers,axp202-ac-power-supply",
		.num_resources	= ARRAY_SIZE(axp20x_ac_power_supply_resources),
		.resources	= axp20x_ac_power_supply_resources,
	}, {
		.name		= "axp20x-usb-power-supply",
		.of_compatible	= "x-powers,axp202-usb-power-supply",
		.num_resources	= ARRAY_SIZE(axp20x_usb_power_supply_resources),
		.resources	= axp20x_usb_power_supply_resources,
	},
};

static struct mfd_cell axp22x_cells[] = {
	{
		.name			= "axp20x-pek",
		.num_resources		= ARRAY_SIZE(axp22x_pek_resources),
		.resources		= axp22x_pek_resources,
	}, {
		.name			= "axp20x-regulator",
	}, {
		.name		= "axp20x-usb-power-supply",
		.of_compatible	= "x-powers,axp221-usb-power-supply",
		.num_resources	= ARRAY_SIZE(axp22x_usb_power_supply_resources),
		.resources	= axp22x_usb_power_supply_resources,
	},
};

#define AXP152_DCDC1_NAME "dcdc1"
#define AXP152_DCDC2_NAME "dcdc2"
#define AXP152_DCDC3_NAME "dcdc3"
#define AXP152_DCDC4_NAME "dcdc4"
#define AXP152_ALDO1_NAME "aldo1"
#define AXP152_ALDO2_NAME "aldo2"
#define AXP152_DLDO1_NAME "dldo1"
#define AXP152_DLDO2_NAME "dldo2"
#define AXP152_LDO0_NAME  "ldo0"
static struct mfd_cell axp152_cells[] = {
/*	{
		.name			= "axp20x-pek",
		.num_resources		= ARRAY_SIZE(axp152_pek_resources),
		.resources		= axp152_pek_resources,
	},*/
	{
		.name = "axp152-pek",
		.num_resources = ARRAY_SIZE(axp152_pek_resources),
		.resources = axp152_pek_resources,
		.of_compatible = "x-powers,axp152-pek",
	},
	{
		/* match drivers/regulator/axp2101.c */
		.name = "axp2101-regulator",
	},
	{
		/* match drivers/power/supply/axp152_vbus_power.c */
		.name = "axp152-vbus",
		.of_compatible = "x-powers,axp152-vbus",
	},
	{
		.of_compatible = "xpower-vregulator,dcdc1",
		.name = "reg-virt-consumer",
		.id = 1,
		.platform_data = AXP152_DCDC1_NAME,
		.pdata_size = sizeof(AXP152_DCDC1_NAME),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc2",
		.name = "reg-virt-consumer",
		.id = 2,
		.platform_data = AXP152_DCDC2_NAME,
		.pdata_size = sizeof(AXP152_DCDC2_NAME),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc3",
		.name = "reg-virt-consumer",
		.id = 3,
		.platform_data = AXP152_DCDC3_NAME,
		.pdata_size = sizeof(AXP152_DCDC3_NAME),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc4",
		.name = "reg-virt-consumer",
		.id = 4,
		.platform_data = AXP152_DCDC4_NAME,
		.pdata_size = sizeof(AXP152_DCDC4_NAME),
	},
	{
		.of_compatible = "xpower-vregulator,aldo1",
		.name = "reg-virt-consumer",
		.id = 5,
		.platform_data = AXP152_ALDO1_NAME,
		.pdata_size = sizeof(AXP152_ALDO1_NAME),
	},
	{
		.of_compatible = "xpower-vregulator,aldo2",
		.name = "reg-virt-consumer",
		.id = 6,
		.platform_data = AXP152_ALDO2_NAME,
		.pdata_size = sizeof(AXP152_ALDO2_NAME),
	},
	{
		.of_compatible = "xpower-vregulator,dldo1",
		.name = "reg-virt-consumer",
		.id = 7,
		.platform_data = AXP152_DLDO1_NAME,
		.pdata_size = sizeof(AXP152_DLDO1_NAME),
	},
	{
		.of_compatible = "xpower-vregulator,ldo0",
		.name = "reg-virt-consumer",
		.id = 8,
		.platform_data = AXP152_LDO0_NAME,
		.pdata_size = sizeof(AXP152_LDO0_NAME),
	},
};

static struct resource axp288_adc_resources[] = {
	{
		.name  = "GPADC",
		.start = AXP288_IRQ_GPADC,
		.end   = AXP288_IRQ_GPADC,
		.flags = IORESOURCE_IRQ,
	},
};

static struct resource axp2101_power_supply_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_BWUT, "bat untemp work"),
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_BWOT, "bat ovtemp work"),
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_BCUT, "bat untemp chg"),
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_BCOT, "bat ovtemp chg"),
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_NEWSOC, "CHG_NEWSOC"),
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_GWDT, "CHG_GWDT"),
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_SOCWL1, "low warning1"),
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_SOCWL2, "low warning2"),
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_BREMOV, "bat out"),
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_BINSERT, "bat in"),
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_VINSET, "usb in"),
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_VREMOV, "usb out"),
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_BOVP, "CHG_BOVP"),
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_CHGTE, "CHG_CHGTE"),
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_DOTL1, "CHG_DOTL1"),
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_CHGST, "charging"),
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_CHGDN, "charge over"),
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_BOCP, "CHG_BOCP"),
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_LDOOC, "CHG_LDOOC"),
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_WDEXP, "CHG_WDEXP"),
};

static struct resource axp2101_pek_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_PONN, "PEK_DBF"),
	DEFINE_RES_IRQ_NAMED(AXP2101_IRQ_PONP, "PEK_DBR"),
};
/*
static struct resource axp15_power_supply_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP15_IRQ_ALDOIN_H2L, "aldoin H2L"),
	DEFINE_RES_IRQ_NAMED(AXP15_IRQ_ALDOIN_L2H, "aldoin L2H"),
	DEFINE_RES_IRQ_NAMED(AXP15_IRQ_LDO0IN_H2L, "ldoin H2L"),
	DEFINE_RES_IRQ_NAMED(AXP15_IRQ_LDO0IN_L2H, "ldoin L2H"),
	DEFINE_RES_IRQ_NAMED(AXP15_IRQ_PEKLO, "pek long"),
	DEFINE_RES_IRQ_NAMED(AXP15_IRQ_PEKSH, "pek short"),
	DEFINE_RES_IRQ_NAMED(AXP15_IRQ_DCDC4_V_LOW, "DCDC4 smaller"),
	DEFINE_RES_IRQ_NAMED(AXP15_IRQ_DCDC3_V_LOW, "DCDC3 smaller"),
	DEFINE_RES_IRQ_NAMED(AXP15_IRQ_DCDC2_V_LOW, "DCDC2 smaller"),
	DEFINE_RES_IRQ_NAMED(AXP15_IRQ_DCDC1_V_LOW, "DCDC1 smaller"),
	DEFINE_RES_IRQ_NAMED(AXP15_IRQ_EVENT_TIMEOUT, "envent timeout"),
};
*/
static struct resource axp15_pek_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP15_IRQ_PEKRE, "PEK_DBR"),
	DEFINE_RES_IRQ_NAMED(AXP15_IRQ_PEKFE, "PEK_DBF"),
};

static struct resource axp15_gpio_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP15_IRQ_GPIO0, "GPIO0 input"),
	DEFINE_RES_IRQ_NAMED(AXP15_IRQ_GPIO1, "GPIO1 input"),
	DEFINE_RES_IRQ_NAMED(AXP15_IRQ_GPIO2, "GPIO2 input"),
	DEFINE_RES_IRQ_NAMED(AXP15_IRQ_GPIO3, "GPIO3 input"),
};

static struct resource axp1530_pek_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP1530_IRQ_KEY_L2H_EN, "PEK_DBR"),
	DEFINE_RES_IRQ_NAMED(AXP1530_IRQ_KEY_H2L_EN, "PEK_DBF"),
};

static struct resource axp858_pek_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP858_IRQ_POKNIRQ_EN, "PEK_DBF"),
	DEFINE_RES_IRQ_NAMED(AXP858_IRQ_POKPIRQ_EN, "PEK_DBR"),
};

static struct resource axp803_ac_power_supply_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP803_IRQ_ACIN, "ac in"),
	DEFINE_RES_IRQ_NAMED(AXP803_IRQ_ACRE, "ac out"),
};

static struct resource axp803_usb_power_supply_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP803_IRQ_USBIN, "usb in"),
	DEFINE_RES_IRQ_NAMED(AXP803_IRQ_USBRE, "usb out"),
};

static struct resource axp803_bat_power_supply_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP803_IRQ_BATINWORK, "bat untemp work"),
	DEFINE_RES_IRQ_NAMED(AXP803_IRQ_BATOVWORK, "bat ovtemp work"),
	DEFINE_RES_IRQ_NAMED(AXP803_IRQ_BATINCHG, "bat untemp chg"),
	DEFINE_RES_IRQ_NAMED(AXP803_IRQ_BATOVCHG, "bat ovtemp chg"),
	DEFINE_RES_IRQ_NAMED(AXP803_IRQ_LOWN1, "low warning1"),
	DEFINE_RES_IRQ_NAMED(AXP803_IRQ_LOWN2, "low warning2"),
	DEFINE_RES_IRQ_NAMED(AXP803_IRQ_BATRE, "bat out"),
	DEFINE_RES_IRQ_NAMED(AXP803_IRQ_BATIN, "bat in"),
	DEFINE_RES_IRQ_NAMED(AXP803_IRQ_CHAST, "charging"),
	DEFINE_RES_IRQ_NAMED(AXP803_IRQ_CHAOV, "charge over"),
};

static struct resource axp803_pek_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP803_IRQ_PEKFE, "PEK_DBF"),
	DEFINE_RES_IRQ_NAMED(AXP803_IRQ_PEKRE, "PEK_DBR"),
};

static struct resource axp806_pek_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP806_IRQ_PWROK_FALL, "PEK_DBF"),
	DEFINE_RES_IRQ_NAMED(AXP806_IRQ_PWROK_RISE, "PEK_DBR"),
};

static struct resource axp2202_pek_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_PONN, "PEK_DBF"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_PONP, "PEK_DBR"),
};

static struct resource axp2202_bat_power_supply_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_SOCWL1, "soc_drop_w1"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_SOCWL2, "soc_drop_w2"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_NEWSOC, "gauge_new_soc"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_BINSERT, "battery_insert"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_BREMOVE, "battery_remove"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_CHGDN, "battery_charge_done"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_CHGST, "battery_charge_start"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_BOVP, "battery_over_voltage"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_BCOT, "battery_over_temp_chg"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_BCUT, "battery_under_temp_chg"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_BWOT, "battery_over_temp_work"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_BWUT, "battery_under_temp_work"),
};

static struct resource axp2202_usb_power_supply_resources[] = {
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_VBUS_OV, "vbus_over_volt"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_VINSERT, "vbus_insert"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_VREMOVE, "vbus_remove"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_BC_DONE, "bc1_2_detected"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_BC_CHNG, "bc1_2_detect_change"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_RID_CHNG, "rid_detect_change"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_CREMOVE, "type-c_remove"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_CINSERT, "type-c_insert"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_TOGGLE_DONE, "type-c_toggle"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_VBUS_SAFE5V, "type-c_safe-5v"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_VBUS_SAFE0V, "type-c_safe-0v"),
	DEFINE_RES_IRQ_NAMED(AXP2202_IRQ_PWR_CHNG, "type-c_state_change"),
};

static struct resource axp288_extcon_resources[] = {
	{
		.start = AXP288_IRQ_VBUS_FALL,
		.end   = AXP288_IRQ_VBUS_FALL,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = AXP288_IRQ_VBUS_RISE,
		.end   = AXP288_IRQ_VBUS_RISE,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = AXP288_IRQ_MV_CHNG,
		.end   = AXP288_IRQ_MV_CHNG,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = AXP288_IRQ_BC_USB_CHNG,
		.end   = AXP288_IRQ_BC_USB_CHNG,
		.flags = IORESOURCE_IRQ,
	},
};

static struct resource axp288_charger_resources[] = {
	{
		.start = AXP288_IRQ_OV,
		.end   = AXP288_IRQ_OV,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = AXP288_IRQ_DONE,
		.end   = AXP288_IRQ_DONE,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = AXP288_IRQ_CHARGING,
		.end   = AXP288_IRQ_CHARGING,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = AXP288_IRQ_SAFE_QUIT,
		.end   = AXP288_IRQ_SAFE_QUIT,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = AXP288_IRQ_SAFE_ENTER,
		.end   = AXP288_IRQ_SAFE_ENTER,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = AXP288_IRQ_QCBTU,
		.end   = AXP288_IRQ_QCBTU,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = AXP288_IRQ_CBTU,
		.end   = AXP288_IRQ_CBTU,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = AXP288_IRQ_QCBTO,
		.end   = AXP288_IRQ_QCBTO,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = AXP288_IRQ_CBTO,
		.end   = AXP288_IRQ_CBTO,
		.flags = IORESOURCE_IRQ,
	},
};

static struct mfd_cell axp288_cells[] = {
	{
		.name = "axp288_adc",
		.num_resources = ARRAY_SIZE(axp288_adc_resources),
		.resources = axp288_adc_resources,
	},
	{
		.name = "axp288_extcon",
		.num_resources = ARRAY_SIZE(axp288_extcon_resources),
		.resources = axp288_extcon_resources,
	},
	{
		.name = "axp288_charger",
		.num_resources = ARRAY_SIZE(axp288_charger_resources),
		.resources = axp288_charger_resources,
	},
	{
		.name = "axp288_fuel_gauge",
		.num_resources = ARRAY_SIZE(axp288_fuel_gauge_resources),
		.resources = axp288_fuel_gauge_resources,
	},
	{
		.name = "axp20x-pek",
		.num_resources = ARRAY_SIZE(axp288_power_button_resources),
		.resources = axp288_power_button_resources,
	},
	{
		.name = "axp288_pmic_acpi",
	},
};

#define AXP806_DCDC1_NAME "dcdc1"
#define AXP806_DCDC2_NAME "dcdc2"
#define AXP806_DCDC3_NAME "dcdc3"
#define AXP806_DCDC4_NAME "dcdc4"
#define AXP806_DCDC5_NAME "dcdc5"
#define AXP806_ALDO1_NAME "aldo1"
#define AXP806_ALDO2_NAME "aldo2"
#define AXP806_ALDO3_NAME "aldo3"
#define AXP806_BLDO1_NAME "bldo1"
#define AXP806_BLDO2_NAME "bldo2"
#define AXP806_BLDO3_NAME "bldo3"
#define AXP806_BLDO4_NAME "bldo4"
#define AXP806_CLDO1_NAME "cldo1"
#define AXP806_CLDO2_NAME "cldo2"
#define AXP806_CLDO3_NAME "cldo3"
static struct mfd_cell axp806_cells[] = {
	{
		.name = "axp2101-pek",
		.num_resources = ARRAY_SIZE(axp806_pek_resources),
		.resources = axp806_pek_resources,
		.of_compatible = "x-powers,axp2101-pek",
	},
	{
		.name = "axp2101-regulator",
	},
	{
		.of_compatible = "xpower-vregulator,dcdc1",
		.name = "reg-virt-consumer",
		.id = 1,
		.platform_data = AXP806_DCDC1_NAME,
		.pdata_size = sizeof(AXP806_DCDC1_NAME),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc2",
		.name = "reg-virt-consumer",
		.id = 2,
		.platform_data = AXP806_DCDC2_NAME,
		.pdata_size = sizeof(AXP806_DCDC2_NAME),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc3",
		.name = "reg-virt-consumer",
		.id = 3,
		.platform_data = AXP806_DCDC3_NAME,
		.pdata_size = sizeof(AXP806_DCDC3_NAME),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc4",
		.name = "reg-virt-consumer",
		.id = 4,
		.platform_data = AXP806_DCDC4_NAME,
		.pdata_size = sizeof(AXP806_DCDC4_NAME),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc5",
		.name = "reg-virt-consumer",
		.id = 5,
		.platform_data = AXP806_DCDC5_NAME,
		.pdata_size = sizeof(AXP806_DCDC5_NAME),
	},
	{
		.of_compatible = "xpower-vregulator,aldo1",
		.name = "reg-virt-consumer",
		.id = 6,
		.platform_data = AXP806_ALDO1_NAME,
		.pdata_size = sizeof(AXP806_ALDO1_NAME),
	},
	{
		.of_compatible = "xpower-vregulator,aldo2",
		.name = "reg-virt-consumer",
		.id = 7,
		.platform_data = AXP806_ALDO2_NAME,
		.pdata_size = sizeof(AXP806_ALDO2_NAME),
	},
	{
		.of_compatible = "xpower-vregulator,aldo3",
		.name = "reg-virt-consumer",
		.id = 8,
		.platform_data = AXP806_ALDO3_NAME,
		.pdata_size = sizeof(AXP806_ALDO3_NAME),
	},
	{
		.of_compatible = "xpower-vregulator,bldo1",
		.name = "reg-virt-consumer",
		.id = 9,
		.platform_data = AXP806_BLDO1_NAME,
		.pdata_size = sizeof(AXP806_BLDO1_NAME),
	},
	{
		.of_compatible = "xpower-vregulator,bldo2",
		.name = "reg-virt-consumer",
		.id = 10,
		.platform_data = AXP806_BLDO2_NAME,
		.pdata_size = sizeof(AXP806_BLDO2_NAME),
	},
	{
		.of_compatible = "xpower-vregulator,bldo3",
		.name = "reg-virt-consumer",
		.id = 11,
		.platform_data = AXP806_BLDO3_NAME,
		.pdata_size = sizeof(AXP806_BLDO3_NAME),
	},
	{
		.of_compatible = "xpower-vregulator,bldo4",
		.name = "reg-virt-consumer",
		.id = 12,
		.platform_data = AXP806_BLDO4_NAME,
		.pdata_size = sizeof(AXP806_BLDO4_NAME),
	},
	{
		.of_compatible = "xpower-vregulator,cldo1",
		.name = "reg-virt-consumer",
		.id = 13,
		.platform_data = AXP806_CLDO1_NAME,
		.pdata_size = sizeof(AXP806_CLDO1_NAME),
	},
	{
		.of_compatible = "xpower-vregulator,cldo2",
		.name = "reg-virt-consumer",
		.id = 14,
		.platform_data = AXP806_CLDO2_NAME,
		.pdata_size = sizeof(AXP806_CLDO2_NAME),
	},
	{
		.of_compatible = "xpower-vregulator,cldo3",
		.name = "reg-virt-consumer",
		.id = 15,
		.platform_data = AXP806_CLDO3_NAME,
		.pdata_size = sizeof(AXP806_CLDO3_NAME),
	},
};

static struct mfd_cell axp809_cells[] = {
	{
		.name			= "axp20x-pek",
		.num_resources		= ARRAY_SIZE(axp809_pek_resources),
		.resources		= axp809_pek_resources,
	}, {
		.id			= 1,
		.name			= "axp20x-regulator",
	},
};


#define AXP2101_DCDC1 "dcdc1"
#define AXP2101_DCDC2 "dcdc2"
#define AXP2101_DCDC3 "dcdc3"
#define AXP2101_DCDC4 "dcdc4"
#define AXP2101_DCDC5 "dcdc5"
#define AXP2101_ALDO1 "aldo1"
#define AXP2101_ALDO2 "aldo2"
#define AXP2101_ALDO3 "aldo3"
#define AXP2101_ALDO4 "aldo4"
#define AXP2101_BLDO1 "bldo1"
#define AXP2101_BLDO2 "bldo2"
#define AXP2101_DLDO1 "dldo1"
#define AXP2101_DLDO2 "dldo2"

static struct mfd_cell axp2101_cells[] = {
	/*{
.name		= "axp2101-gpio",
.of_compatible	= "x-powers,axp2101-gpio",
}, */
	{
		.name = "axp2101-pek",
		.num_resources = ARRAY_SIZE(axp2101_pek_resources),
		.resources = axp2101_pek_resources,
		.of_compatible = "x-powers,axp2101-pek",
	},
	{
		.name = "axp2101-regulator",
	},
	{
		.name = "axp2101-power-supply",
		.of_compatible = "x-powers,axp2101-power-supply",
		.num_resources = ARRAY_SIZE(axp2101_power_supply_resources),
		.resources = axp2101_power_supply_resources,
	},
	{
		.name = "axp2xx-watchdog",
		.of_compatible = "x-powers,axp2xx-watchdog",
	},
	{
		.of_compatible = "xpower-vregulator,dcdc1",
		.name = "reg-virt-consumer",
		.id = 1,
		.platform_data = AXP2101_DCDC1,
		.pdata_size = sizeof(AXP2101_DCDC1),

	},
	{
		.of_compatible = "xpower-vregulator,dcdc2",
		.name = "reg-virt-consumer",
		.id = 2,
		.platform_data = AXP2101_DCDC2,
		.pdata_size = sizeof(AXP2101_DCDC2),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc3",
		.name = "reg-virt-consumer",
		.id = 3,
		.platform_data = AXP2101_DCDC3,
		.pdata_size = sizeof(AXP2101_DCDC3),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc4",
		.name = "reg-virt-consumer",
		.id = 4,
		.platform_data = AXP2101_DCDC4,
		.pdata_size = sizeof(AXP2101_DCDC4),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc5",
		.name = "reg-virt-consumer",
		.id = 5,
		.platform_data = AXP2101_DCDC5,
		.pdata_size = sizeof(AXP2101_DCDC5),
	},

	{
		.of_compatible = "xpower-vregulator,aldo1",
		.name = "reg-virt-consumer",
		.id = 8,
		.platform_data = AXP2101_ALDO1,
		.pdata_size = sizeof(AXP2101_ALDO1),
	},
	{
		.of_compatible = "xpower-vregulator,aldo2",
		.name = "reg-virt-consumer",
		.id = 9,
		.platform_data = AXP2101_ALDO2,
		.pdata_size = sizeof(AXP2101_ALDO2),
	},
	{
		.of_compatible = "xpower-vregulator,aldo3",
		.name = "reg-virt-consumer",
		.id = 10,
		.platform_data = AXP2101_ALDO3,
		.pdata_size = sizeof(AXP2101_ALDO3),
	},
	{
		.of_compatible = "xpower-vregulator,aldo4",
		.name = "reg-virt-consumer",
		.id = 11,
		.platform_data = AXP2101_ALDO4,
		.pdata_size = sizeof(AXP2101_ALDO4),
	},

	{
		.of_compatible = "xpower-vregulator,bldo1",
		.name = "reg-virt-consumer",
		.id = 12,
		.platform_data = AXP2101_BLDO1,
		.pdata_size = sizeof(AXP2101_BLDO1),
	},
	{
		.of_compatible = "xpower-vregulator,bldo2",
		.name = "reg-virt-consumer",
		.id = 13,
		.platform_data = AXP2101_BLDO2,
		.pdata_size = sizeof(AXP2101_BLDO2),
	},

	{
		.of_compatible = "xpower-vregulator,dldo1",
		.name = "reg-virt-consumer",
		.id = 14,
		.platform_data = AXP2101_DLDO1,
		.pdata_size = sizeof(AXP2101_DLDO1),
	},
	{
		.of_compatible = "xpower-vregulator,dldo2",
		.name = "reg-virt-consumer",
		.id = 15,
		.platform_data = AXP2101_DLDO2,
		.pdata_size = sizeof(AXP2101_DLDO2),
	},
};
/***************************************/
#define AXP15_DCDC1 "dcdc1"
#define AXP15_DCDC2 "dcdc2"
#define AXP15_DCDC3 "dcdc3"
#define AXP15_DCDC4 "dcdc4"
#define AXP15_DCDC5 "dcdc5"
#define AXP15_ALDO1 "aldo1"
#define AXP15_ALDO2 "aldo2"
#define AXP15_ALDO3 "aldo3"
#define AXP15_ALDO4 "aldo4"
#define AXP15_BLDO1 "bldo1"
#define AXP15_BLDO2 "bldo2"
#define AXP15_DLDO1 "dldo1"
#define AXP15_DLDO2 "dldo2"

static struct mfd_cell axp15_cells[] = {
	{
		.name		= "axp15-gpio",
		.of_compatible	= "x-powers,axp15-gpio",
		.num_resources = ARRAY_SIZE(axp15_gpio_resources),
		.resources = axp15_gpio_resources,
	},
	{
		.name = "axp15-pek",
		.num_resources = ARRAY_SIZE(axp15_pek_resources),
		.resources = axp15_pek_resources,
		.of_compatible = "x-powers,axp15-pek",
	},
	{
		.name = "axp15-regulator",
	},
/*	{
		.name = "axp15-power-supply",
		.of_compatible = "x-powers,axp15-power-supply",
	},*/
	{
		.of_compatible = "xpower-vregulator,dcdc1",
		.name = "reg-virt-consumer",
		.id = 1,
		.platform_data = AXP15_DCDC1,
		.pdata_size = sizeof(AXP15_DCDC1),

	},
	{
		.of_compatible = "xpower-vregulator,dcdc2",
		.name = "reg-virt-consumer",
		.id = 2,
		.platform_data = AXP15_DCDC2,
		.pdata_size = sizeof(AXP15_DCDC2),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc3",
		.name = "reg-virt-consumer",
		.id = 3,
		.platform_data = AXP15_DCDC3,
		.pdata_size = sizeof(AXP15_DCDC3),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc4",
		.name = "reg-virt-consumer",
		.id = 4,
		.platform_data = AXP15_DCDC4,
		.pdata_size = sizeof(AXP15_DCDC4),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc5",
		.name = "reg-virt-consumer",
		.id = 5,
		.platform_data = AXP15_DCDC5,
		.pdata_size = sizeof(AXP15_DCDC5),
	},

	{
		.of_compatible = "xpower-vregulator,aldo1",
		.name = "reg-virt-consumer",
		.id = 8,
		.platform_data = AXP15_ALDO1,
		.pdata_size = sizeof(AXP15_ALDO1),
	},
	{
		.of_compatible = "xpower-vregulator,aldo2",
		.name = "reg-virt-consumer",
		.id = 9,
		.platform_data = AXP15_ALDO2,
		.pdata_size = sizeof(AXP15_ALDO2),
	},
	{
		.of_compatible = "xpower-vregulator,aldo3",
		.name = "reg-virt-consumer",
		.id = 10,
		.platform_data = AXP15_ALDO3,
		.pdata_size = sizeof(AXP15_ALDO3),
	},
	{
		.of_compatible = "xpower-vregulator,aldo4",
		.name = "reg-virt-consumer",
		.id = 11,
		.platform_data = AXP15_ALDO4,
		.pdata_size = sizeof(AXP15_ALDO4),
	},

	{
		.of_compatible = "xpower-vregulator,bldo1",
		.name = "reg-virt-consumer",
		.id = 12,
		.platform_data = AXP15_BLDO1,
		.pdata_size = sizeof(AXP15_BLDO1),
	},
	{
		.of_compatible = "xpower-vregulator,bldo2",
		.name = "reg-virt-consumer",
		.id = 13,
		.platform_data = AXP15_BLDO2,
		.pdata_size = sizeof(AXP15_BLDO2),
	},

	{
		.of_compatible = "xpower-vregulator,dldo1",
		.name = "reg-virt-consumer",
		.id = 14,
		.platform_data = AXP15_DLDO1,
		.pdata_size = sizeof(AXP15_DLDO1),
	},
	{
		.of_compatible = "xpower-vregulator,dldo2",
		.name = "reg-virt-consumer",
		.id = 15,
		.platform_data = AXP15_DLDO2,
		.pdata_size = sizeof(AXP15_DLDO2),
	},
};
/***************************************/
#define AXP1530_DCDC1 "dcdc1"
#define AXP1530_DCDC2 "dcdc2"
#define AXP1530_DCDC3 "dcdc3"
#define AXP1530_ALDO1 "aldo1"
#define AXP1530_DLDO1 "dldo1"

static struct mfd_cell axp1530_cells[] = {
	{
		.name		= "axp1530-gpio",
		.of_compatible	= "x-powers,axp1530-gpio",
/*		.num_resources = ARRAY_SIZE(axp1530_gpio_resources),
		.resources = axp1530_gpio_resources,*/
	},
	{
		.name = "axp2101-pek",
		.num_resources = ARRAY_SIZE(axp1530_pek_resources),
		.resources = axp1530_pek_resources,
		.of_compatible = "x-powers,axp2101-pek",
	},
	{
		/* match drivers/regulator/axp2101.c */
		.name = "axp2101-regulator",
	},
	{
		.of_compatible = "xpower-vregulator,dcdc1",
		.name = "reg-virt-consumer",
		.id = 1,
		.platform_data = AXP1530_DCDC1,
		.pdata_size = sizeof(AXP1530_DCDC1),

	},
	{
		.of_compatible = "xpower-vregulator,dcdc2",
		.name = "reg-virt-consumer",
		.id = 2,
		.platform_data = AXP1530_DCDC2,
		.pdata_size = sizeof(AXP1530_DCDC2),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc3",
		.name = "reg-virt-consumer",
		.id = 3,
		.platform_data = AXP1530_DCDC3,
		.pdata_size = sizeof(AXP1530_DCDC3),
	},
	{
		.of_compatible = "xpower-vregulator,aldo1",
		.name = "reg-virt-consumer",
		.id = 4,
		.platform_data = AXP1530_ALDO1,
		.pdata_size = sizeof(AXP1530_ALDO1),
	},
	{
		.of_compatible = "xpower-vregulator,dldo1",
		.name = "reg-virt-consumer",
		.id = 5,
		.platform_data = AXP1530_DLDO1,
		.pdata_size = sizeof(AXP1530_DLDO1),
	},
};
/**************************************/
#define AXP858_DCDC1 "dcdc1"
#define AXP858_DCDC2 "dcdc2"
#define AXP858_DCDC3 "dcdc3"
#define AXP858_DCDC4 "dcdc4"
#define AXP858_DCDC5 "dcdc5"
#define AXP858_DCDC6 "dcdc6"
#define AXP858_ALDO1 "aldo1"
#define AXP858_ALDO2 "aldo2"
#define AXP858_ALDO3 "aldo3"
#define AXP858_ALDO4 "aldo4"
#define AXP858_ALDO5 "aldo5"
#define AXP858_BLDO1 "bldo1"
#define AXP858_BLDO2 "bldo2"
#define AXP858_BLDO3 "bldo3"
#define AXP858_BLDO4 "bldo4"
#define AXP858_BLDO5 "bldo5"
#define AXP858_CLDO1 "cldo1"
#define AXP858_CLDO2 "cldo2"
#define AXP858_CLDO3 "cldo3"
#define AXP858_CLDO4 "cldo4"
#define AXP858_CPUSLDO "cpusldo"
#define AXP858_SWOUT "swout"

static struct mfd_cell axp858_cells[] = {
	{
		.name		= "axp858-gpio",
		.of_compatible	= "x-powers,axp858-gpio",
/*		.num_resources = ARRAY_SIZE(axp1530_gpio_resources),
		.resources = axp1530_gpio_resources,*/
	},
	{
		.name = "axp2101-pek",
		.num_resources = ARRAY_SIZE(axp858_pek_resources),
		.resources = axp858_pek_resources,
		.of_compatible = "x-powers,axp2101-pek",
	},
	{
		.name = "axp2101-regulator",
	},
/*	{
		.name = "axp15-power-supply",
		.of_compatible = "x-powers,axp15-power-supply",
	},*/
	{
		.of_compatible = "xpower-vregulator,dcdc1",
		.name = "reg-virt-consumer",
		.id = 1,
		.platform_data = AXP858_DCDC1,
		.pdata_size = sizeof(AXP858_DCDC1),

	},
	{
		.of_compatible = "xpower-vregulator,dcdc2",
		.name = "reg-virt-consumer",
		.id = 2,
		.platform_data = AXP858_DCDC2,
		.pdata_size = sizeof(AXP858_DCDC2),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc3",
		.name = "reg-virt-consumer",
		.id = 3,
		.platform_data = AXP858_DCDC3,
		.pdata_size = sizeof(AXP858_DCDC3),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc4",
		.name = "reg-virt-consumer",
		.id = 4,
		.platform_data = AXP858_DCDC4,
		.pdata_size = sizeof(AXP858_DCDC4),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc5",
		.name = "reg-virt-consumer",
		.id = 5,
		.platform_data = AXP858_DCDC5,
		.pdata_size = sizeof(AXP858_DCDC5),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc6",
		.name = "reg-virt-consumer",
		.id = 6,
		.platform_data = AXP858_DCDC6,
		.pdata_size = sizeof(AXP858_DCDC6),
	},
	{
		.of_compatible = "xpower-vregulator,aldo1",
		.name = "reg-virt-consumer",
		.id = 7,
		.platform_data = AXP858_ALDO1,
		.pdata_size = sizeof(AXP858_ALDO1),
	},
	{
		.of_compatible = "xpower-vregulator,aldo2",
		.name = "reg-virt-consumer",
		.id = 8,
		.platform_data = AXP858_ALDO2,
		.pdata_size = sizeof(AXP858_ALDO2),
	},
	{
		.of_compatible = "xpower-vregulator,aldo3",
		.name = "reg-virt-consumer",
		.id = 9,
		.platform_data = AXP858_ALDO3,
		.pdata_size = sizeof(AXP858_ALDO3),
	},
	{
		.of_compatible = "xpower-vregulator,aldo4",
		.name = "reg-virt-consumer",
		.id = 10,
		.platform_data = AXP858_ALDO4,
		.pdata_size = sizeof(AXP858_ALDO4),
	},
	{
		.of_compatible = "xpower-vregulator,aldo5",
		.name = "reg-virt-consumer",
		.id = 11,
		.platform_data = AXP858_ALDO5,
		.pdata_size = sizeof(AXP858_ALDO5),
	},
	{
		.of_compatible = "xpower-vregulator,bldo1",
		.name = "reg-virt-consumer",
		.id = 12,
		.platform_data = AXP858_BLDO1,
		.pdata_size = sizeof(AXP858_BLDO1),
	},
	{
		.of_compatible = "xpower-vregulator,bldo2",
		.name = "reg-virt-consumer",
		.id = 13,
		.platform_data = AXP858_BLDO2,
		.pdata_size = sizeof(AXP858_BLDO2),
	},
	{
		.of_compatible = "xpower-vregulator,bldo3",
		.name = "reg-virt-consumer",
		.id = 14,
		.platform_data = AXP858_BLDO3,
		.pdata_size = sizeof(AXP858_BLDO3),
	},
	{
		.of_compatible = "xpower-vregulator,bldo4",
		.name = "reg-virt-consumer",
		.id = 15,
		.platform_data = AXP858_BLDO4,
		.pdata_size = sizeof(AXP858_BLDO4),
	},
	{
		.of_compatible = "xpower-vregulator,bldo5",
		.name = "reg-virt-consumer",
		.id = 16,
		.platform_data = AXP858_BLDO5,
		.pdata_size = sizeof(AXP858_BLDO5),
	},
	{
		.of_compatible = "xpower-vregulator,cldo1",
		.name = "reg-virt-consumer",
		.id = 17,
		.platform_data = AXP858_CLDO1,
		.pdata_size = sizeof(AXP858_CLDO1),
	},
	{
		.of_compatible = "xpower-vregulator,cldo2",
		.name = "reg-virt-consumer",
		.id = 18,
		.platform_data = AXP858_CLDO2,
		.pdata_size = sizeof(AXP858_CLDO2),
	},
	{
		.of_compatible = "xpower-vregulator,cldo3",
		.name = "reg-virt-consumer",
		.id = 19,
		.platform_data = AXP858_CLDO3,
		.pdata_size = sizeof(AXP858_CLDO3),
	},
	{
		.of_compatible = "xpower-vregulator,cldo4",
		.name = "reg-virt-consumer",
		.id = 20,
		.platform_data = AXP858_CLDO4,
		.pdata_size = sizeof(AXP858_CLDO4),
	},
	{
		.of_compatible = "xpower-vregulator,cpusldo",
		.name = "reg-virt-consumer",
		.id = 21,
		.platform_data = AXP858_CPUSLDO,
		.pdata_size = sizeof(AXP858_CPUSLDO),
	},
	{
		.of_compatible = "xpower-vregulator,swout",
		.name = "reg-virt-consumer",
		.id = 22,
		.platform_data = AXP858_SWOUT,
		.pdata_size = sizeof(AXP858_SWOUT),
	},

};

#define AXP803_DCDC1 "dcdc1"
#define AXP803_DCDC2 "dcdc2"
#define AXP803_DCDC3 "dcdc3"
#define AXP803_DCDC4 "dcdc4"
#define AXP803_DCDC5 "dcdc5"
#define AXP803_DCDC6 "dcdc6"
#define AXP803_DCDC7 "dcdc7"
#define AXP803_RTCLDO "rtcldo"
#define AXP803_ALDO1 "aldo1"
#define AXP803_ALDO2 "aldo2"
#define AXP803_ALDO3 "aldo3"
#define AXP803_DLDO1 "dldo1"
#define AXP803_DLDO2 "dldo2"
#define AXP803_DLDO3 "dldo3"
#define AXP803_DLDO4 "dldo4"
#define AXP803_ELDO1 "eldo1"
#define AXP803_ELDO2 "eldo2"
#define AXP803_ELDO3 "eldo3"
#define AXP803_FLDO1 "fldo1"
#define AXP803_FLDO2 "fldo2"
#define AXP803_LDOIO0 "ldoio0"
#define AXP803_LDOIO1 "ldoio1"
#define AXP803_DC1SW "dc1sw"
#define AXP803_DRIVEVBUS "drivevbus"

static struct mfd_cell axp803_cells[] = {
	{
		.name = "axp803-gpio",
		.of_compatible = "x-powers,axp803-gpio",
	},
	{
		.name = "axp2101-pek",
		.num_resources = ARRAY_SIZE(axp803_pek_resources),
		.resources = axp803_pek_resources,
		.of_compatible = "x-powers,axp2101-pek",
	},
	{
		.name = "axp2101-regulator",
	},
	{
		.name		= "axp803-battery-power-supply",
		.of_compatible	= "x-powers,axp803-battery-power-supply",
		.num_resources	= ARRAY_SIZE(axp803_bat_power_supply_resources),
		.resources	= axp803_bat_power_supply_resources,
	},
	{
		.name		= "axp803-ac-power-supply",
		.of_compatible	= "x-powers,axp803-ac-power-supply",
		.num_resources	= ARRAY_SIZE(axp803_ac_power_supply_resources),
		.resources	= axp803_ac_power_supply_resources,
	},
	{
		.name		= "axp803-usb-power-supply",
		.of_compatible	= "x-powers,axp803-usb-power-supply",
		.num_resources	= ARRAY_SIZE(axp803_usb_power_supply_resources),
		.resources	= axp803_usb_power_supply_resources,
	},
	{
		.of_compatible = "xpower-vregulator,dcdc1",
		.name = "reg-virt-consumer",
		.id = 1,
		.platform_data = AXP803_DCDC1,
		.pdata_size = sizeof(AXP803_DCDC1),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc2",
		.name = "reg-virt-consumer",
		.id = 2,
		.platform_data = AXP803_DCDC2,
		.pdata_size = sizeof(AXP803_DCDC2),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc3",
		.name = "reg-virt-consumer",
		.id = 3,
		.platform_data = AXP803_DCDC3,
		.pdata_size = sizeof(AXP803_DCDC3),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc4",
		.name = "reg-virt-consumer",
		.id = 4,
		.platform_data = AXP803_DCDC4,
		.pdata_size = sizeof(AXP803_DCDC4),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc5",
		.name = "reg-virt-consumer",
		.id = 5,
		.platform_data = AXP803_DCDC5,
		.pdata_size = sizeof(AXP803_DCDC5),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc6",
		.name = "reg-virt-consumer",
		.id = 6,
		.platform_data = AXP803_DCDC6,
		.pdata_size = sizeof(AXP803_DCDC6),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc7",
		.name = "reg-virt-consumer",
		.id = 7,
		.platform_data = AXP803_DCDC7,
		.pdata_size = sizeof(AXP803_DCDC6),
	},
	{
		.of_compatible = "xpower-vregulator,rtcldo",
		.name = "reg-virt-consumer",
		.id = 8,
		.platform_data = AXP803_RTCLDO,
		.pdata_size = sizeof(AXP803_RTCLDO),
	},
	{
		.of_compatible = "xpower-vregulator,aldo1",
		.name = "reg-virt-consumer",
		.id = 9,
		.platform_data = AXP803_ALDO1,
		.pdata_size = sizeof(AXP803_ALDO1),
	},
	{
		.of_compatible = "xpower-vregulator,aldo2",
		.name = "reg-virt-consumer",
		.id = 10,
		.platform_data = AXP803_ALDO2,
		.pdata_size = sizeof(AXP803_ALDO2),
	},
	{
		.of_compatible = "xpower-vregulator,aldo3",
		.name = "reg-virt-consumer",
		.id = 11,
		.platform_data = AXP803_ALDO3,
		.pdata_size = sizeof(AXP803_ALDO3),
	},
	{
		.of_compatible = "xpower-vregulator,dldo1",
		.name = "reg-virt-consumer",
		.id = 12,
		.platform_data = AXP803_DLDO1,
		.pdata_size = sizeof(AXP803_DLDO1),
	},
	{
		.of_compatible = "xpower-vregulator,dldo2",
		.name = "reg-virt-consumer",
		.id = 13,
		.platform_data = AXP803_DLDO2,
		.pdata_size = sizeof(AXP803_DLDO2),
	},
	{
		.of_compatible = "xpower-vregulator,dldo3",
		.name = "reg-virt-consumer",
		.id = 14,
		.platform_data = AXP803_DLDO3,
		.pdata_size = sizeof(AXP803_DLDO3),
	},
	{
		.of_compatible = "xpower-vregulator,dldo4",
		.name = "reg-virt-consumer",
		.id = 15,
		.platform_data = AXP803_DLDO4,
		.pdata_size = sizeof(AXP803_DLDO4),
	},
	{
		.of_compatible = "xpower-vregulator,eldo1",
		.name = "reg-virt-consumer",
		.id = 16,
		.platform_data = AXP803_ELDO1,
		.pdata_size = sizeof(AXP803_ELDO1),
	},
	{
		.of_compatible = "xpower-vregulator,eldo2",
		.name = "reg-virt-consumer",
		.id = 17,
		.platform_data = AXP803_ELDO2,
		.pdata_size = sizeof(AXP803_ELDO2),
	},
	{
		.of_compatible = "xpower-vregulator,eldo3",
		.name = "reg-virt-consumer",
		.id = 18,
		.platform_data = AXP803_ELDO3,
		.pdata_size = sizeof(AXP803_ELDO3),
	},
	{
		.of_compatible = "xpower-vregulator,fldo1",
		.name = "reg-virt-consumer",
		.id = 19,
		.platform_data = AXP803_FLDO1,
		.pdata_size = sizeof(AXP803_FLDO1),
	},
	{
		.of_compatible = "xpower-vregulator,fldo2",
		.name = "reg-virt-consumer",
		.id = 20,
		.platform_data = AXP803_FLDO2,
		.pdata_size = sizeof(AXP803_FLDO2),
	},
	{
		.of_compatible = "xpower-vregulator,ldoio0",
		.name = "reg-virt-consumer",
		.id = 21,
		.platform_data = AXP803_LDOIO0,
		.pdata_size = sizeof(AXP803_LDOIO0),
	},
	{
		.of_compatible = "xpower-vregulator,ldoio1",
		.name = "reg-virt-consumer",
		.id = 22,
		.platform_data = AXP803_LDOIO1,
		.pdata_size = sizeof(AXP803_LDOIO1),
	},
	{
		.of_compatible = "xpower-vregulator,dc1sw",
		.name = "reg-virt-consumer",
		.id = 23,
		.platform_data = AXP803_DC1SW,
		.pdata_size = sizeof(AXP803_DC1SW),
	},
	{
		.of_compatible = "xpower-vregulator,drivevbus",
		.name = "reg-virt-consumer",
		.id = 24,
		.platform_data = AXP803_DRIVEVBUS,
		.pdata_size = sizeof(AXP803_DRIVEVBUS),
	},
};

#define AXP2202_DCDC1 "dcdc1"
#define AXP2202_DCDC2 "dcdc2"
#define AXP2202_DCDC3 "dcdc3"
#define AXP2202_DCDC4 "dcdc4"
#define AXP2202_ALDO1 "aldo1"
#define AXP2202_ALDO2 "aldo2"
#define AXP2202_ALDO3 "aldo3"
#define AXP2202_ALDO4 "aldo4"
#define AXP2202_BLDO1 "bldo1"
#define AXP2202_BLDO2 "bldo2"
#define AXP2202_BLDO3 "bldo3"
#define AXP2202_BLDO4 "bldo4"
#define AXP2202_CLDO1 "cldo1"
#define AXP2202_CLDO2 "cldo2"
#define AXP2202_CLDO3 "cldo3"
#define AXP2202_CLDO4 "cldo4"
#define AXP2202_RTCLDO "rtcldo"
#define AXP2202_CPUSLDO "cpusldo"
#define AXP2202_DRIVEVBUS "drivevbus"

static struct mfd_cell axp2202_cells[] = {
	{
		.name = "axp2202-gpio",
		.of_compatible = "x-powers,axp2202-gpio",
	},
	{
		.name = "axp2101-regulator",
	},
	{
		.name = "axp2101-pek",
		.of_compatible = "x-powers,axp2101-pek",
		.resources = axp2202_pek_resources,
		.num_resources = ARRAY_SIZE(axp2202_pek_resources),

	},
	{
		.name = "axp2202-bat-power-supply",
		.of_compatible = "x-powers,axp2202-bat-power-supply",
		.resources = axp2202_bat_power_supply_resources,
		.num_resources = ARRAY_SIZE(axp2202_bat_power_supply_resources),
	},
	{
		.name = "axp2202-usb-power-supply",
		.of_compatible = "x-powers,axp2202-usb-power-supply",
		.resources = axp2202_usb_power_supply_resources,
		.num_resources = ARRAY_SIZE(axp2202_usb_power_supply_resources),
	},
	{
		.name = "gpio-supply",
		.of_compatible = "x-powers,gpio-supply",
	},
	{
		.of_compatible = "xpower-vregulator,dcdc1",
		.name = "reg-virt-consumer",
		.id = 1,
		.platform_data = AXP2202_DCDC1,
		.pdata_size = sizeof(AXP2202_DCDC1),

	},
	{
		.of_compatible = "xpower-vregulator,dcdc2",
		.name = "reg-virt-consumer",
		.id = 2,
		.platform_data = AXP2202_DCDC2,
		.pdata_size = sizeof(AXP2202_DCDC2),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc3",
		.name = "reg-virt-consumer",
		.id = 3,
		.platform_data = AXP2202_DCDC3,
		.pdata_size = sizeof(AXP2202_DCDC3),
	},
	{
		.of_compatible = "xpower-vregulator,dcdc4",
		.name = "reg-virt-consumer",
		.id = 4,
		.platform_data = AXP2202_DCDC4,
		.pdata_size = sizeof(AXP2202_DCDC4),
	},
	{
		.of_compatible = "xpower-vregulator,aldo1",
		.name = "reg-virt-consumer",
		.id = 5,
		.platform_data = AXP2202_ALDO1,
		.pdata_size = sizeof(AXP2202_ALDO1),
	},
	{
		.of_compatible = "xpower-vregulator,aldo2",
		.name = "reg-virt-consumer",
		.id = 6,
		.platform_data = AXP2202_ALDO2,
		.pdata_size = sizeof(AXP2202_ALDO2),
	},
	{
		.of_compatible = "xpower-vregulator,aldo3",
		.name = "reg-virt-consumer",
		.id = 7,
		.platform_data = AXP2202_ALDO3,
		.pdata_size = sizeof(AXP2202_ALDO3),
	},
	{
		.of_compatible = "xpower-vregulator,aldo4",
		.name = "reg-virt-consumer",
		.id = 8,
		.platform_data = AXP2202_ALDO4,
		.pdata_size = sizeof(AXP2202_ALDO4),
	},
	{
		.of_compatible = "xpower-vregulator,bldo1",
		.name = "reg-virt-consumer",
		.id = 9,
		.platform_data = AXP2202_BLDO1,
		.pdata_size = sizeof(AXP2202_BLDO1),
	},
	{
		.of_compatible = "xpower-vregulator,bldo2",
		.name = "reg-virt-consumer",
		.id = 10,
		.platform_data = AXP2202_BLDO2,
		.pdata_size = sizeof(AXP2202_BLDO2),
	},
	{
		.of_compatible = "xpower-vregulator,bldo3",
		.name = "reg-virt-consumer",
		.id = 11,
		.platform_data = AXP2202_BLDO3,
		.pdata_size = sizeof(AXP2202_BLDO3),
	},
	{
		.of_compatible = "xpower-vregulator,bldo4",
		.name = "reg-virt-consumer",
		.id = 12,
		.platform_data = AXP2202_BLDO4,
		.pdata_size = sizeof(AXP2202_BLDO4),
	},
	{
		.of_compatible = "xpower-vregulator,cldo1",
		.name = "reg-virt-consumer",
		.id = 13,
		.platform_data = AXP2202_CLDO1,
		.pdata_size = sizeof(AXP2202_CLDO1),
	},
	{
		.of_compatible = "xpower-vregulator,cldo2",
		.name = "reg-virt-consumer",
		.id = 14,
		.platform_data = AXP2202_CLDO2,
		.pdata_size = sizeof(AXP2202_CLDO2),
	},
	{
		.of_compatible = "xpower-vregulator,cldo3",
		.name = "reg-virt-consumer",
		.id = 15,
		.platform_data = AXP2202_CLDO3,
		.pdata_size = sizeof(AXP2202_CLDO3),
	},
	{
		.of_compatible = "xpower-vregulator,cldo4",
		.name = "reg-virt-consumer",
		.id = 16,
		.platform_data = AXP2202_CLDO4,
		.pdata_size = sizeof(AXP2202_CLDO4),
	},
	{
		.of_compatible = "xpower-vregulator,rtcldo",
		.name = "reg-virt-consumer",
		.id = 17,
		.platform_data = AXP2202_RTCLDO,
		.pdata_size = sizeof(AXP2202_RTCLDO),
	},
	{
		.of_compatible = "xpower-vregulator,cpusldo",
		.name = "reg-virt-consumer",
		.id = 18,
		.platform_data = AXP2202_CPUSLDO,
		.pdata_size = sizeof(AXP2202_CPUSLDO),
	},

};

/*----------------------*/
static struct axp20x_dev *axp20x_pm_power_off;
static void axp20x_power_off(void)
{
	if (axp20x_pm_power_off->variant == AXP288_ID)
		return;

	regmap_write(axp20x_pm_power_off->regmap, AXP20X_OFF_CTRL,
		     AXP20X_OFF);

	/* Give capacitors etc. time to drain to avoid kernel panic msg. */
	msleep(500);
}

/*
 * axp2101_dts_parse - axp2101 device tree parse
 */
static void axp2101_dts_parse(struct axp20x_dev *axp20x)
{
	struct device_node *node = axp20x->dev->of_node;
	struct regmap *map = axp20x->regmap;
	u32 val, tempval;

	if (of_property_read_bool(axp20x->dev->of_node, "pmu_powerok_noreset")) {
		regmap_update_bits(axp20x->regmap, AXP2101_COMM_CFG, BIT(3), 0);
	} else {
		regmap_update_bits(axp20x->regmap, AXP2101_COMM_CFG, BIT(3),
				   BIT(3));
	}

	/* init 16's reset pmu en */
	if (of_property_read_u32(node, "pmu_reset", &val))
		val = 0;
	if (val) {
		regmap_update_bits(map, AXP2101_COMM_CFG, BIT(2), BIT(2));
	} else {
		regmap_update_bits(map, AXP2101_COMM_CFG, BIT(2), 0);
	}

	/* enable pwrok pin pull low to restart the system */
	regmap_update_bits(map, AXP2101_COMM_CFG, BIT(3), BIT(3));

	/* init pmu over temperature protection */
	if (of_property_read_u32(node, "pmu_hot_shutdown", &val))
		val = 0;
	if (val) {
		regmap_update_bits(map, AXP2101_PWROFF_EN, BIT(2), BIT(2));
		if (of_property_read_u32(node, "pmu_hot_shutdown_value", &tempval))
			tempval = 125;
		regmap_read(map, AXP2101_DIE_TEMP_CFG, &val);
		val &= 0xF9;
		if (tempval > 135)
			val |= 0x06;
		else if (tempval > 125)
			val |= 0x04;
		else if (tempval > 115)
			val |= 0x02;
		else
			val |= 0x00;
		regmap_write(map, AXP2101_DIE_TEMP_CFG, val);
	} else {
		regmap_update_bits(map, AXP2101_PWROFF_EN, BIT(2), 0);
	}

	/* 85% low voltage turn off pmic function */
	/* axp_regmap_write(axp2101->regmap, axp2101_DCDC_PWROFF_EN, 0x3f); */
	/* set 2.6v for battery poweroff */
	regmap_write(map, AXP2101_VOFF_THLD, 0x00);
	/* set delay of powerok after all power output good to 8ms */
	regmap_update_bits(map, AXP2101_PWR_TIME_CTRL, 0x03, 0x00);
}

static void axp803_dts_parse(struct axp20x_dev *axp20x)
{
	struct device_node *node = axp20x->dev->of_node;
	struct regmap *map = axp20x->regmap;
	u32 val, tempval;

	/* init 16's reset pmu en */
	if (of_property_read_u32(node, "pmu_reset", &val))
		val = 0;
	if (val) {
		regmap_update_bits(map, AXP803_HOTOVER_CTL, BIT(3), BIT(3));
	} else {
		regmap_update_bits(map, AXP803_HOTOVER_CTL, BIT(3), 0);
	}

	/* init irq wakeup en */
	if (of_property_read_u32(node, "pmu_irq_wakeup", &val))
		val = 0;
	if (val) {
		regmap_update_bits(map, AXP803_HOTOVER_CTL, BIT(7), BIT(7));
	} else {
		regmap_update_bits(map, AXP803_HOTOVER_CTL, BIT(7), 0);
	}

	/* init pmu over temperature protection */
	if (of_property_read_u32(node, "pmu_hot_shutdown", &val))
		val = 1;
	if (val) {
		regmap_update_bits(map, AXP803_HOTOVER_CTL, BIT(2), BIT(2));
		/* default warning level 1 is 105 */
		/* level 2/3 add 6.4/12.8 */
		if (of_property_read_u32(node, "pmu_hot_shutdown_value", &tempval))
			tempval = 105;
		regmap_read(map, AXP803_HOTOVER_VAL, &val);
		val &= 0xF8;
		if (tempval > 135)
			val |= 0x07;
		else if (tempval > 125)
			val |= 0x06;
		else if (tempval > 115)
			val |= 0x05;
		else if (tempval > 105)
			val |= 0x04;
		else if (tempval > 95)
			val |= 0x03;
		else if (tempval > 85)
			val |= 0x02;
		else if (tempval > 75)
			val |= 0x01;
		else
			val |= 0x00;
		regmap_write(map, AXP803_HOTOVER_VAL, val);
	} else {
		regmap_update_bits(map, AXP803_HOTOVER_CTL, BIT(2), 0);
	}

	/* init inshort status */
	if (of_property_read_u32(node, "pmu_inshort", &val))
		val = 0;
	if (val) {
		regmap_update_bits(map, AXP803_HOTOVER_CTL,
				BIT(5) | BIT(6), BIT(5) | BIT(6));
	} else {
		regmap_update_bits(map, AXP803_HOTOVER_CTL,
				BIT(5) | BIT(6), 0);
	}
}

static void axp2202_dts_parse(struct axp20x_dev *axp20x)
{
	struct device_node *node = axp20x->dev->of_node;
	struct regmap *map = axp20x->regmap;
	u32 val, tempval;

	/* init powerok reset function */
	if (of_property_read_u32(node, "pmu_powerok_noreset", &val))
		val = 0;
	if (val) {
		regmap_update_bits(map, AXP2202_SOFT_PWROFF, BIT(3), 0);
	} else {
		regmap_update_bits(map, AXP2202_SOFT_PWROFF, BIT(3), BIT(3));
	}

	/* init 16's por function */
	if (of_property_read_u32(node, "pmu_reset", &val))
		val = 0;
	if (val) {
		regmap_update_bits(map, AXP2202_SOFT_PWROFF, BIT(2), BIT(2));
	} else {
		regmap_update_bits(map, AXP2202_SOFT_PWROFF, BIT(2), 0);
	}

	/* init irq wakeup en */
	if (of_property_read_u32(node, "pmu_irq_wakeup", &val))
		val = 0;
	if (val) {
		regmap_update_bits(map, AXP2202_SLEEP_CFG, BIT(5), BIT(5));
	} else {
		regmap_update_bits(map, AXP2202_SLEEP_CFG, BIT(5), 0);
	}

	/* init ldo over current protection */
	if (of_property_read_u32(node, "pmu_over_current", &val))
		val = 1;
	if (val) {
		regmap_update_bits(map, AXP2202_PWROFF_EN, BIT(3), BIT(3));
	} else {
		regmap_update_bits(map, AXP2202_PWROFF_EN, BIT(3), 0);
	}

	/* init pmu over temperature protection */
	if (of_property_read_u32(node, "pmu_hot_shutdown", &val))
		val = 1;
	if (val) {
		regmap_update_bits(map, AXP2202_PWROFF_EN, BIT(2), BIT(2));
	} else {
		regmap_update_bits(map, AXP2202_PWROFF_EN, BIT(2), 0);
	}

	/* init pmu over temperature protection */
	if (of_property_read_u32(node, "pmu_hot_shutdown", &val))
		val = 0;
	if (val) {
		regmap_update_bits(map, AXP2202_PWROFF_EN, BIT(2), BIT(2));
		if (of_property_read_u32(node,
					"pmu_hot_shutdown_value", &tempval))
			tempval = 125;
		regmap_read(map, AXP2202_DIE_TEMP_CFG, &val);
		val &= 0xF9;
		if (tempval > 135)
			val |= 0x06;
		else if (tempval > 125)
			val |= 0x04;
		else if (tempval > 115)
			val |= 0x02;
		else
			val |= 0x00;
		regmap_write(map, AXP2202_DIE_TEMP_CFG, val);
	} else {
		regmap_update_bits(map, AXP2202_PWROFF_EN, BIT(2), 0);
	}
}

/* AXP221/AXP223 AXP809*/
static void axp20x_dts_parse(struct axp20x_dev *axp20x)
{
	struct device_node *node = axp20x->dev->of_node;
	struct regmap *map = axp20x->regmap;
	u32 val, tempval;

	/* init pmu over temperature protection */
	if (of_property_read_u32(node, "pmu_hot_shutdown", &val))
		val = 0;
	if (val) {
		regmap_update_bits(map, AXP20X_OVER_TMP, BIT(2), BIT(2));
		/* set overtmp to 105 */
		if (of_property_read_u32(node, "pmu_hot_shutdown_value", &tempval))
			tempval = 105;
		regmap_read(map, AXP20X_OVER_TMP_VAL, &val);
		val &= 0xF8;
		if (tempval > 135)
			val |= 0x07;
		else if (tempval > 125)
			val |= 0x06;
		else if (tempval > 115)
			val |= 0x05;
		else if (tempval > 105)
			val |= 0x04;
		else if (tempval > 95)
			val |= 0x03;
		else if (tempval > 85)
			val |= 0x02;
		else if (tempval > 75)
			val |= 0x01;
		else
			val |= 0x00;
		regmap_write(map, AXP20X_OVER_TMP_VAL, val);
	} else {
		regmap_update_bits(map, AXP20X_OVER_TMP, BIT(2), 0);
	}
}

/* AXP202/AXP209 */
static void axp209_dts_parse(struct axp20x_dev *axp20x)
{
	struct device_node *node = axp20x->dev->of_node;
	struct regmap *map = axp20x->regmap;
	u32 val, tempval;

	/* init pmu over temperature protection */
	if (of_property_read_u32(node, "pmu_hot_shutdown", &val))
		val = 0;
	if (val) {
		regmap_update_bits(map, AXP20X_OVER_TMP, BIT(2), BIT(2));
		if (of_property_read_u32(node, "pmu_hot_shutdown_value", &tempval))
			tempval = 104;
		regmap_read(map, AXP20X_OVER_TMP_VAL, &val);
		val &= 0xF8;
		if (tempval > 124)
			val |= 0x07;
		else if (tempval > 117)
			val |= 0x06;
		else if (tempval > 111)
			val |= 0x05;
		else if (tempval > 104)
			val |= 0x04;
		else if (tempval > 98)
			val |= 0x03;
		else if (tempval > 92)
			val |= 0x02;
		else if (tempval > 85)
			val |= 0x01;
		else
			val |= 0x00;
		val |= tempval;
		regmap_write(map, AXP20X_OVER_TMP_VAL, val);
	} else {
		regmap_update_bits(map, AXP20X_OVER_TMP, BIT(2), 0);
	}
}

static void axp152_dts_parse(struct axp20x_dev *axp20x)
{
	struct device_node *node = axp20x->dev->of_node;
	struct regmap *map = axp20x->regmap;
	u32 val, tempval;

	/* init pmu over temperature protection */
	if (of_property_read_u32(node, "pmu_hot_shutdown", &val))
		val = 0;
	if (val) {
		regmap_update_bits(map, AXP20X_OVER_TMP, BIT(2), BIT(2));
		/* default tempval value: BIT(1) */
		if (of_property_read_u32(node,
					"pmu_hot_shutdown_value", &tempval))
			tempval = 125;
		regmap_read(map, AXP20X_OVER_TMP, &val);
		val &= 0xFC;
		if (tempval > 135)
			val |= 0x03;
		else if (tempval > 125)
			val |= 0x02;
		else if (tempval > 115)
			val |= 0x01;
		else
			val |= 0x00;
		regmap_write(map, AXP20X_OVER_TMP, val);
	} else {
		regmap_update_bits(map, AXP20X_OVER_TMP, BIT(2), 0);
	}
}

static void axp806_dts_parse(struct axp20x_dev *axp20x)
{
	struct device_node *node = axp20x->dev->of_node;
	struct regmap *map = axp20x->regmap;
	u32 val, tempval;

	/* init pmu over temperature protection */
	if (of_property_read_u32(node, "pmu_hot_shutdown", &val))
		val = 0;
	if (val) {
		regmap_update_bits(map, AXP806_OFF_CTL, BIT(1), BIT(1));
		if (of_property_read_u32(node,
					"pmu_hot_shutdown_value", &tempval))
			tempval = 125;
		regmap_read(map, AXP806_VREF_TEMP_WARN_L, &val);
		val &= 0xFC;
		if (tempval > 135)
			val |= 0x03;
		else if (tempval > 125)
			val |= 0x02;
		else if (tempval > 115)
			val |= 0x01;
		else
			val |= 0x00;
		regmap_write(map, AXP806_VREF_TEMP_WARN_L, val);
	} else {
		regmap_update_bits(map, AXP806_OFF_CTL, BIT(1), 0);
	}
}

static void axp1530_dts_parse(struct axp20x_dev *axp20x)
{
	struct device_node *node = axp20x->dev->of_node;
	struct regmap *map = axp20x->regmap;
	u32 val, tempval;

	/* init pmu over temperature protection */
	if (of_property_read_u32(node, "pmu_hot_shutdown", &val))
		val = 0;
	if (val) {
		regmap_update_bits(map, AXP1530_POWER_STATUS, BIT(1), BIT(1));
		if (of_property_read_u32(node,
					"pmu_hot_shutdown_value", &tempval))
			tempval = 125;
		regmap_read(map, AXP1530_POWER_STATUS, &val);
		val &= 0xFE;
		if (tempval > 125)
			val |= 0x01;
		else
			val |= 0x00;
		regmap_write(map, AXP1530_POWER_STATUS, val);
	} else {
		regmap_update_bits(map, AXP1530_POWER_STATUS, BIT(1), 0);
	}
}

static void axp858_dts_parse(struct axp20x_dev *axp20x)
{
	struct device_node *node = axp20x->dev->of_node;
	struct regmap *map = axp20x->regmap;
	u32 val, tempval;

	/* init pmu over temperature protection */
	if (of_property_read_u32(node, "pmu_hot_shutdown", &val))
		val = 0;
	if (val) {
		regmap_update_bits(map, AXP858_POWER_DOWN_DIS, BIT(1), BIT(1));
		if (of_property_read_u32(node,
					"pmu_hot_shutdown_value", &tempval))
			tempval = 125;
		regmap_read(map, AXP858_VREF_TEM_SET, &val);
		val &= 0xFC;
		if (tempval > 135)
			val |= 0x03;
		else if (tempval > 125)
			val |= 0x02;
		else if (tempval > 115)
			val |= 0x01;
		else
			val |= 0x00;
		regmap_write(map, AXP858_VREF_TEM_SET, val);
	} else {
		regmap_update_bits(map, AXP858_POWER_DOWN_DIS, BIT(1), 0);
	}
}

int axp20x_match_device(struct axp20x_dev *axp20x)
{
	struct device *dev = axp20x->dev;
	const struct acpi_device_id *acpi_id;
	const struct of_device_id *of_id;

	if (dev->of_node) {
		of_id = of_match_device(dev->driver->of_match_table, dev);
		if (!of_id) {
			dev_err(dev, "Unable to match OF ID\n");
			return -ENODEV;
		}
		axp20x->variant = (long)of_id->data;
	} else {
		acpi_id = acpi_match_device(dev->driver->acpi_match_table, dev);
		if (!acpi_id || !acpi_id->driver_data) {
			dev_err(dev, "Unable to match ACPI ID and data\n");
			return -ENODEV;
		}
		axp20x->variant = (long)acpi_id->driver_data;
	}

	switch (axp20x->variant) {
	case AXP152_ID:
		axp20x->nr_cells = ARRAY_SIZE(axp152_cells);
		axp20x->cells = axp152_cells;
		axp20x->regmap_cfg = &axp152_regmap_config;
		axp20x->regmap_irq_chip = &axp152_regmap_irq_chip;
		axp20x->dts_parse = axp152_dts_parse;
		break;
	case AXP202_ID:
	case AXP209_ID:
		axp20x->nr_cells = ARRAY_SIZE(axp20x_cells);
		axp20x->cells = axp20x_cells;
		axp20x->regmap_cfg = &axp20x_regmap_config;
		axp20x->regmap_irq_chip = &axp20x_regmap_irq_chip;
		axp20x->dts_parse = axp209_dts_parse;
		break;
	case AXP221_ID:
	case AXP223_ID:
		axp20x->nr_cells = ARRAY_SIZE(axp22x_cells);
		axp20x->cells = axp22x_cells;
		axp20x->regmap_cfg = &axp22x_regmap_config;
		axp20x->regmap_irq_chip = &axp22x_regmap_irq_chip;
		axp20x->dts_parse = axp20x_dts_parse;
		break;
	case AXP288_ID:
		axp20x->cells = axp288_cells;
		axp20x->nr_cells = ARRAY_SIZE(axp288_cells);
		axp20x->regmap_cfg = &axp288_regmap_config;
		axp20x->regmap_irq_chip = &axp288_regmap_irq_chip;
		break;
	case AXP806_ID:
		axp20x->nr_cells = ARRAY_SIZE(axp806_cells);
		axp20x->cells = axp806_cells;
		axp20x->regmap_cfg = &axp806_regmap_config;
		axp20x->regmap_irq_chip = &axp806_regmap_irq_chip;
		axp20x->dts_parse = axp806_dts_parse;
		break;
	case AXP809_ID:
		axp20x->nr_cells = ARRAY_SIZE(axp809_cells);
		axp20x->cells = axp809_cells;
		axp20x->regmap_cfg = &axp22x_regmap_config;
		axp20x->regmap_irq_chip = &axp809_regmap_irq_chip;
		axp20x->dts_parse = axp20x_dts_parse;
		break;
	case AXP2101_ID:
		axp20x->nr_cells = ARRAY_SIZE(axp2101_cells);
		axp20x->cells = axp2101_cells;
		axp20x->regmap_cfg = &axp2101_regmap_config;
		axp20x->regmap_irq_chip = &axp2101_regmap_irq_chip;
		axp20x->dts_parse = axp2101_dts_parse;
		break;
/**************************************/
	case AXP15_ID:
		axp20x->nr_cells = ARRAY_SIZE(axp15_cells);
		axp20x->cells = axp15_cells;
		axp20x->regmap_cfg = &axp15_regmap_config;
		axp20x->regmap_irq_chip = &axp15_regmap_irq_chip;
		break;
/**************************************/
	case AXP1530_ID:
		axp20x->nr_cells = ARRAY_SIZE(axp1530_cells);
		axp20x->cells = axp1530_cells;
		axp20x->regmap_cfg = &axp1530_regmap_config;
		axp20x->regmap_irq_chip = &axp1530_regmap_irq_chip;
		axp20x->dts_parse = axp1530_dts_parse;
		break;
/**************************************/
	case AXP858_ID:
		axp20x->nr_cells = ARRAY_SIZE(axp858_cells);
		axp20x->cells = axp858_cells;
		axp20x->regmap_cfg = &axp858_regmap_config;
		axp20x->regmap_irq_chip = &axp858_regmap_irq_chip;
		axp20x->dts_parse = axp858_dts_parse;
		break;
	case AXP803_ID:
		axp20x->nr_cells = ARRAY_SIZE(axp803_cells);
		axp20x->cells = axp803_cells;
		axp20x->regmap_cfg = &axp803_regmap_config;
		axp20x->regmap_irq_chip = &axp803_regmap_irq_chip;
		axp20x->dts_parse = axp803_dts_parse;
		break;
	case AXP2202_ID:
		axp20x->nr_cells = ARRAY_SIZE(axp2202_cells);
		axp20x->cells = axp2202_cells;
		axp20x->regmap_cfg = &axp2202_regmap_config;
		axp20x->regmap_irq_chip = &axp2202_regmap_irq_chip;
		axp20x->dts_parse = axp2202_dts_parse;
		break;
/*-------------------*/
	default:
		dev_err(dev, "unsupported AXP20X ID %lu\n", axp20x->variant);
		return -EINVAL;
	}
	dev_info(dev, "AXP20x variant %s found\n",
		 axp20x_model_names[axp20x->variant]);

	return 0;
}
EXPORT_SYMBOL(axp20x_match_device);


int axp_debug_mask;
EXPORT_SYMBOL(axp_debug_mask);

static ssize_t debug_mask_store(struct class *class,
				struct class_attribute *attr,
				const char *buf, size_t count)
{
	int val, err;

	err = kstrtoint(buf, 16, &val);
	if (err)
		return err;

	axp_debug_mask = val;

	return count;
}

static ssize_t debug_mask_show(struct class *class,
				struct class_attribute *attr, char *buf)
{
	char *s = buf;
	char *end = (char *)((ptrdiff_t)buf + (ptrdiff_t)PAGE_SIZE);

	s += scnprintf(s, end - s, "%s\n", "1: SPLY 2: REGU 4: INT 8: CHG");
	s += scnprintf(s, end - s, "debug_mask=%d\n", axp_debug_mask);

	return s - buf;
}
static CLASS_ATTR_RW(debug_mask);

static u32 axp_reg_addr;

static ssize_t axp_reg_show(struct class *class,
				struct class_attribute *attr, char *buf)
{
	u32 val;

	regmap_read(axp20x_pm_power_off->regmap, axp_reg_addr, &val);
	return sprintf(buf, "REG[0x%x]=0x%x\n",
				axp_reg_addr, val);
}

static ssize_t axp_reg_store(struct class *class,
				struct class_attribute *attr,
				const char *buf, size_t count)
{
	s32 tmp;
	u32 val;
	int err;

	err = kstrtoint(buf, 16, &tmp);
	if (err)
		return err;

	if (tmp < 256) {
		axp_reg_addr = tmp;
	} else {
		val = tmp & 0x00FF;
		axp_reg_addr = (tmp >> 8) & 0x00FF;
		regmap_write(axp20x_pm_power_off->regmap, axp_reg_addr, val);
	}

	return count;
}
static CLASS_ATTR_RW(axp_reg);

static struct attribute *axp_class_attrs[] = {
	&class_attr_axp_reg.attr,
	&class_attr_debug_mask.attr,
	NULL,
};
ATTRIBUTE_GROUPS(axp_class);

struct class axp_class = {
	.name = "axp",
	.class_groups = axp_class_groups,
};

static int axp_sysfs_init(void)
{
	int status;

	status = class_register(&axp_class);
	if (status < 0)
		pr_err("%s,%d err, status:%d\n", __func__, __LINE__, status);

	return status;
}

int axp20x_device_probe(struct axp20x_dev *axp20x)
{
	int ret;

	/*
	 * on some board ex. qaqc test board, there's no interrupt for axp20x
	 */
	if (axp20x->irq) {
		ret = regmap_add_irq_chip(axp20x->regmap, axp20x->irq,
					  IRQF_ONESHOT | IRQF_SHARED, -1,
					  axp20x->regmap_irq_chip,
					  &axp20x->regmap_irqc);
		if (ret) {
			dev_err(axp20x->dev, "failed to add irq chip: %d\n", ret);
			return ret;
		}
	}

	if (axp20x->dts_parse)
		axp20x->dts_parse(axp20x);

	ret = mfd_add_devices(axp20x->dev, 0, axp20x->cells,
			      axp20x->nr_cells, NULL, 0, NULL);

	if (ret) {
		dev_err(axp20x->dev, "failed to add MFD devices: %d\n", ret);

		if (axp20x->irq)
			regmap_del_irq_chip(axp20x->irq, axp20x->regmap_irqc);

		return ret;
	}

	axp20x_pm_power_off = axp20x;
	axp_sysfs_init();
	if (!pm_power_off) {
		pm_power_off = axp20x_power_off;
	}

	dev_info(axp20x->dev, "AXP20X driver loaded\n");

	return 0;
}
EXPORT_SYMBOL(axp20x_device_probe);

int axp20x_device_remove(struct axp20x_dev *axp20x)
{
	if (axp20x == axp20x_pm_power_off) {
		axp20x_pm_power_off = NULL;
		pm_power_off = NULL;
	}

	mfd_remove_devices(axp20x->dev);

	if (axp20x->irq)
		regmap_del_irq_chip(axp20x->irq, axp20x->regmap_irqc);

	return 0;
}
EXPORT_SYMBOL(axp20x_device_remove);

MODULE_DESCRIPTION("PMIC MFD core driver for AXP20X");
MODULE_AUTHOR("Carlo Caione <carlo@caione.org>");
MODULE_LICENSE("GPL");
