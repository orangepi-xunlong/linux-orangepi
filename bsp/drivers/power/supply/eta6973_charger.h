/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef _ETA6973_CHARGER_H_
#define _ETA6973_CHARGER_H_

#include "sunxi-power-supply.h"
#include "bmu-ext.h"

#define ETA6973_MANUFACTURER  "eta,eta6973"
#define ETA6973_MASK_VBUS_STAT        0x60
#define ETA6973_VTERM_MAX		(4616)
#define ETA6973_VTERM_MIN		(3848)

struct bmu_ext_config_info {
	u32 pmu_usbpc_input_vol;
	u32 pmu_usbpc_input_cur;
	u32 pmu_usbad_input_vol;
	u32 pmu_usbad_input_cur;

	u32 pmu_init_chgvol;

	u32 pmu_runtime_chgcur;
	u32 pmu_suspend_chgcur;
	u32 pmu_shutdown_chgcur;
	u32 pmu_prechg_chgcur;
	u32 pmu_terminal_chgcur;

	u32 pmu_bat_temp_enable;
	int pmu_bat_charge_ltf;
	int pmu_bat_charge_htf;

	u32 pmu_jetia_en;
	u32 pmu_jetia_cool;
	u32 pmu_jetia_warm;
	u32 pmu_jcool_ifall;
	u32 pmu_jwarm_ifall;

	u32 bmu_ext_save_charge_time;

	u32 wakeup_usb_in;
	u32 wakeup_usb_out;

	u32 pmu_boost_en;
	u32 pmu_boost_vol;

	u32 battery_exist_sets;
	u32 bmu_ext_max_power;

	u32 pmu_bat_charge_control_lim;
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

