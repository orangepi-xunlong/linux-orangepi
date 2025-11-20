/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef _AXP517_CHARGER_H_
#define _AXP517_CHARGER_H_

#include "sunxi-power-supply.h"
#include "axp2101.h"

#define AXP517_VBAT_MAX        (5000)
#define AXP517_VBAT_MIN        (3600)
#define AXP517_SOC_MAX         (100)
#define AXP517_SOC_MIN         (0)
#define AXP517_MAX_PARAM       128
#define AXP517_MANUFACTURER  "xpower,axp517"

/* DEFAULT PARA  */
#define AXP517_DEFAULT_TEMP  300

enum {
	AXP517_ADC_TDIE = 0,
	AXP517_ADC_VSYS,
	AXP517_ADC_TS,
	AXP517_ADC_TS_AVG,
	AXP517_ADC_IBAT,
	AXP517_ADC_IBAT_AVG,
	AXP517_ADC_ICHG,
	AXP517_ADC_IDCHG,
	AXP517_ADC_TYPE_MAX,
};

/* reg AXP517_COMM_STAT0  */
#define AXP517_MASK_VBUS_STAT        BIT(5)
#define AXP517_MASK_BATFET_STAT      BIT(4)
#define AXP517_MASK_BAT_STAT         BIT(3)
#define AXP517_MASK_BAT_ACT_STAT     BIT(2)
#define AXP517_MASK_THREM_STAT       BIT(1)
#define AXP517_MASK_ILIM_STAT        BIT(0)

/* reg AXP517_COMM_STAT1  */
#define AXP517_STATUS_BAT_CUR_DIRCT GENMASK(6, 5)
#define AXP517_MASK_CHARGE GENMASK(2, 0)
#define AXP517_CHARGING_TRI  (0)
#define AXP517_CHARGING_PRE  (1)
#define AXP517_CHARGING_CC   (2)
#define AXP517_CHARGING_CV   (3)
#define AXP517_CHARGING_DONE (4)
#define AXP517_CHARGING_NCHG (5)

/* reg AXP517_VTERM_CFG  */
#define AXP517_CHRG_TGT_VOLT		GENMASK(2, 0)
#define AXP517_CHRG_CTRL1_TGT_3_6V	(5)
#define AXP517_CHRG_CTRL1_TGT_3_8V	(6)
#define AXP517_CHRG_CTRL1_TGT_4_0V	(0)
#define AXP517_CHRG_CTRL1_TGT_4_1V	(1)
#define AXP517_CHRG_CTRL1_TGT_4_2V	(2)
#define AXP517_CHRG_CTRL1_TGT_4_35V	(3)
#define AXP517_CHRG_CTRL1_TGT_4_4V	(4)
#define AXP517_CHRG_CTRL1_TGT_5_0V	(7)

/* axp517 guage set  */
#define AXP517_BROMUP_EN           BIT(0)
#define AXP517_CFG_UPDATE_MARK     BIT(4)
#define AXP517_MODE_RSTMCU          BIT(2)

/* reg AXP517_MODULE_EN  */
#define AXP517_BUCK_EN           BIT(3)

struct axp_config_info {
	/* usb */
	u32 pmu_bc12_en;
	u32 pmu_usbpc_vol;
	u32 pmu_usbpc_cur;
	u32 pmu_usbad_vol;
	u32 pmu_usbad_cur;
	u32 pmu_vbus_det_typec;

	/* battery capacity */
	u32 pmu_battery_cap;
	u32 pmu_battery_warning_level1;
	u32 pmu_battery_warning_level2;

	/* battery cycle */
	u32 pmu_bat_cycle_life;
	u32 pmu_bat_cycle_cap_reduce;

	/* battery manufacture date */
	u32 pmu_bat_manufacture_year;
	u32 pmu_bat_manufacture_month;
	u32 pmu_bat_manufacture_day;

	/* battery basic para */
	u32 pmu_battery_rdc;
	u32 pmu_init_chgvol;

	/* battery chgcur */
	u32 pmu_runtime_chgcur;
	u32 pmu_suspend_chgcur;
	u32 pmu_shutdown_chgcur;
	u32 pmu_prechg_chgcur;
	u32 pmu_terminal_chgcur;
	u32 pmu_bat_charge_control_lim;

	/* battery chgled */
	u32 pmu_chgled_func;
	u32 pmu_chgled_type;

	/* battery temp */
	u32 pmu_bat_temp_enable;
	u32 pmu_bat_temp_comp;

	u32 pmu_bat_ts_current;
	u32 pmu_bat_charge_ltf;
	u32 pmu_bat_charge_htf;
	u32 pmu_bat_shutdown_ltf;
	u32 pmu_bat_shutdown_htf;

	u32 pmu_jetia_en;
	u32 pmu_jetia_cool;
	u32 pmu_jetia_warm;
	u32 pmu_jcool_ifall;
	u32 pmu_jwarm_ifall;

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

	/* wakeup function */
	u32 wakeup_usb_in;
	u32 wakeup_usb_out;

	u32 wakeup_bat_in;
	u32 wakeup_bat_out;
	u32 wakeup_bat_charging;
	u32 wakeup_bat_charge_over;
	u32 wakeup_low_warning1;
	u32 wakeup_low_warning2;
	u32 wakeup_bat_untemp_work;
	u32 wakeup_bat_ovtemp_work;
	u32 wakeup_untemp_chg;
	u32 wakeup_ovtemp_chg;
	u32 wakeup_bat_ov;
	u32 wakeup_new_soc;
};

#define AXP_OF_PROP_READ(name, def_value)\
do {\
	if (of_property_read_u32(node, #name, &axp_config->name))\
		axp_config->name = def_value;\
} while (0)

struct axp_interrupts {
	char *name;
	irq_handler_t isr;
	int irq;
};

struct axp_gpio_para {
	int	gpio;
	int irq_num;
};

#endif
