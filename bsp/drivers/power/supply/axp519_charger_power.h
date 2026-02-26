/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef _AXP519_CHARGER_H_
#define _AXP519_CHARGER_H_

#include "sunxi-power-supply.h"
#include "bmu-ext.h"

#define AXP519_MANUFACTURER  "x-power,axp519"

#define AXP519_OVP_MAX			(20000)
#define AXP519_OVP_MIN			(4000)
#define AXP519_LINLIM_MAX		(6850)
#define AXP519_LINLIM_MIN		(500)
#define AXP519_VBATLIM_MAX		(12000)
#define AXP519_VBATLIM_MIN		(100)
#define AXP519_VTERM_MAX		(19200)
#define AXP519_VTERM_MIN		(3000)

#define AXP519_VBUS_GD_MASK		BIT(0)
#define AXP519_CHARGING_MASK		BIT(7)
#define AXP519_DISCHARGING_MASK		BIT(6)

#define AXP519_DISCHARG_NTC_MASK	BIT(2)
#define AXP519_CHARG_NTC_MASK		BIT(7)

#define AXP519_IINLIM_MASK		GENMASK(6, 0)
#define AXP519_VBATLIM_MASK		GENMASK(6, 0)
#define AXP519_CHG_TIMER_EN_MASK	BIT(4)
#define AXP519_CHG_TIMER_MASK		GENMASK(1, 0)

#define AXP519_ADC_MASK			GENMASK(3, 0)
#define AXP519_ADC_NTC			0x09
#define AXP519_ADC_SYS			0x0a
#define AXP519_CHG_EN_MASK		BIT(4)

#define AXP519_FULL_DISCHG_MASK		BIT(6)
struct bmu_ext_config_info {
	u32 pmu_usbpc_input_vol;
	u32 pmu_usbpc_input_cur;
	u32 pmu_usbad_input_vol;
	u32 pmu_usbad_input_cur;

	u32 wakeup_usb_in;
	u32 wakeup_usb_out;

	u32 pmu_boost_en;
	u32 pmu_boost_vol;

	u32 battery_exist_sets;
	u32 bmu_ext_max_power;

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

	u32 bmu_ext_save_charge_time;
};

struct gpio_config {
	u32	data;
	u32	gpio;
	u32	mul_sel;
	u32	pull;
	u32	drv_level;
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

#define SUNXI_PINCFG_PACK(type, value)  (((value) << 8) | (type & 0xFF))

#endif
