/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef _AXP2601_CHARGER_H_
#define _AXP2601_CHARGER_H_

#include "sunxi-power-supply.h"
#include "bmu-ext.h"

#define AXP2601_VBAT_MAX		(8000)
#define AXP2601_VBAT_MIN		(2000)
#define AXP2601_SOC_MAX			(100)
#define AXP2601_SOC_MIN			(0)
#define AXP2601_MAX_PARAM		320

#if IS_ENABLED(CONFIG_ARCH_SUN300IW1)
#define AXP2601_FLAGE_REG		0x4A000204
#elif IS_ENABLED(CONFIG_ARCH_SUN50IW10)
#define AXP2601_FLAGE_REG		0x07000104
#else
#define AXP2601_FLAGE_REG		0x07090104
#endif

#define AXP2601_MANUFACTURER		"x-power,axp2601"

#define AXP2601_BROMUP_EN		BIT(0)
#define AXP2601_CFG_UPDATE_MARK		BIT(4)
#define AXP2601_CFG_WDT_EN		BIT(5)

#define AXP2601_MODE_SLEEP		0X01
#define AXP2601_MODE_AWAKE		0XFE
#define AXP2601_MODE_POR_1		0X10
#define AXP2601_MODE_POR_0		0XEF
#define AXP2601_MODE_RSTMCU		0X20

#define AXP2601_CFG_WDT_OFF		0XDF
#define AXP2601_CFG_SRAM_SEL		0X10
#define AXP2601_CFG_ROM_SEL		0XEF
#define AXP2601_CFG_BROM_EN		0X01
#define AXP2601_CFG_BROM_OFF		0XFE

#define AXP2601_IRQ_WDT			0X08
#define AXP2601_IRQ_OT			0X04
#define AXP2601_IRQ_NEWSOC		0X02
#define AXP2601_IRQ_LOWSOC		0X01
#define AXP2601_IRQ_CLEAR1		0X0F
#define AXP2601_IRQ_CLEAR2		0X00

#define AXP2601_IRQ_WDT_MASK		0X08
#define AXP2601_IRQ_WDT_EN		0X07
#define AXP2601_IRQ_OT_MASK		0X04
#define AXP2601_IRQ_OT_EN		0X0B
#define AXP2601_IRQ_NEWSOC_MASK		0X02
#define AXP2601_IRQ_NEWSOC_EN		0X0D
#define AXP2601_IRQ_LOWSOC_MASK		0X01
#define AXP2601_IRQ_LOWSOC_EN		0X0E

struct bmu_ext_config_info {
	u32 pmu_battery_warning_level;
	u32 pmu_init_chgvol;

	u32 pmu_runtime_chgcur;
	u32 pmu_suspend_chgcur;
	u32 pmu_shutdown_chgcur;
	u32 pmu_prechg_chgcur;
	u32 pmu_terminal_chgcur;

	u32 pmu_bat_temp_enable;
	u32 pmu_bat_charge_ltf;
	u32 pmu_bat_charge_htf;
	u32 pmu_bat_shutdown_ltf;
	u32 pmu_bat_shutdown_htf;
	u32 pmu_bat_work_ltf;
	u32 pmu_bat_work_htf;
	u32 pmu_bat_temp_para1;
	u32 pmu_bat_temp_para2;
	u32 pmu_bat_temp_para3;
	u32 pmu_bat_temp_para4;
	u32 pmu_bat_temp_para5;
	u32 pmu_bat_temp_para6;
	u32 pmu_bat_temp_para7;
	u32 pmu_bat_temp_para8;
	u32 pmu_bat_temp_para9;
	u32 pmu_bat_temp_para10;
	u32 pmu_bat_temp_para11;
	u32 pmu_bat_temp_para12;
	u32 pmu_bat_temp_para13;
	u32 pmu_bat_temp_para14;
	u32 pmu_bat_temp_para15;
	u32 pmu_bat_temp_para16;
	u32 wakeup_new_soc;
	u32 wakeup_low_warning;
	u32 pmu_battery_cap;

	u32 pmu_bat_charge_control_lim;
	/* battery cycle */
	u32 pmu_bat_cycle_life;
	u32 pmu_bat_cycle_cap_reduce;

	/* battery manufacture date */
	u32 pmu_bat_manufacture_year;
	u32 pmu_bat_manufacture_month;
	u32 pmu_bat_manufacture_day;
};


#define BMU_EXT_OF_PROP_READ(name, def_value)\
do {\
	if (of_property_read_u32(node, #name, &bmp_ext_config->name))\
		bmp_ext_config->name = def_value;\
} while (0)

struct axp_interrupts {
	char *name;
	irq_handler_t isr;
	int irq;
};

#endif
