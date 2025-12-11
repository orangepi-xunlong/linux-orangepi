/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef _sunxi_usb_power_limit_H_
#define _sunxi_usb_power_limit_H_

#include "sunxi-power-supply.h"
#include "sunxi-power-notifier.h"

enum sunxi_supply_list_id {
	SUNXI_SUPPLY_LIST_BATTERY = 0,
	SUNXI_SUPPLY_LIST_USB_POWER,
	SUNXI_SUPPLY_LIST_TCPC,
	SUNXI_SUPPLY_LIST_MULTI_CHARGE,
	SUNXI_SUPPLY_LIST_MAX,
};

enum sunxi_power_input_limit_type {
	SUNXI_POWER_INPUT_LIMIT_UNKNOWN = 0,
	SUNXI_POWER_INPUT_LIMIT_DEFAULT,
	SUNXI_POWER_INPUT_LIMIT_TCPC,
	SUNXI_POWER_INPUT_LIMIT_PC,
	SUNXI_POWER_INPUT_LIMIT_MULTI_CHARGE,
	SUNXI_POWER_INPUT_LIMIT_PD_CHARGE,
	SUNXI_POWER_INPUT_LIMIT_OTHERS,
	SUNXI_POWER_INPUT_LIMIT_MAX,
};

enum sunxi_extcon_list_id {
	SUNXI_EXTCON_LIST_USB_UDC = 0,
	SUNXI_EXTCON_LIST_USB_PHY,
	SUNXI_EXTCON_LIST_MAX,
};

struct sunxi_usb_power_limit_config_info {
	/* usb */
	u32 pmu_bc12_en;
	u32 pmu_usbpc_vol;
	u32 pmu_usbpc_cur;
	u32 pmu_usbad_vol;
	u32 pmu_usbad_cur;
};

struct sunxi_usb_power_limit_tcpc_limit {
	int input_vol;
	int input_cur;
};

struct sunxi_usb_power_limit_supply_data {
	char						*name;
	struct device					*dev;
	struct power_supply				*power_limit_psy;
	struct sunxi_usb_power_limit_config_info	node_info;

	struct sunxi_supply_psy_status			power_psy[SUNXI_SUPPLY_LIST_MAX];
	struct sunxi_supply_extcon_status		extcon[SUNXI_EXTCON_LIST_MAX];

	struct sunxi_usb_power_limit_tcpc_limit		tcpc_limit;

	/* power supply notifier */
	struct notifier_block				psy_nb;

	/* delayed work */
	struct delayed_work				power_limit_supply_mon;

	struct delayed_work				vbus_online_det_mon;
	struct delayed_work				usb_power_self_managed_chg;

	struct delayed_work				tcpc_online_change;
	struct delayed_work				multi_charge_online_change;
	struct delayed_work				pc_online_change;

	struct wakeup_source				*usb_power_self_managed_chg_wakeup;

	/* atomic_t */
	atomic_t					input_limit_type;
	atomic_t					vbus_online_status;
	struct sunxi_power_debug_data			*debug;
};

static const char * const SUNXI_SUPPLY_LIST_PHANDLE_NAME[] = {
	[SUNXI_SUPPLY_LIST_BATTERY]			= "det_battery_supply",
	[SUNXI_SUPPLY_LIST_USB_POWER]			= "det_usb_supply",
	[SUNXI_SUPPLY_LIST_TCPC]			= "det_typc_supply",
	[SUNXI_SUPPLY_LIST_MULTI_CHARGE]		= "det_multi_charge_supply",
};

static const char * const SUNXI_EXTCON_LIST_PHANDLE_NAME[] = {
	[SUNXI_EXTCON_LIST_USB_UDC]			= "extcon_udc",
	[SUNXI_EXTCON_LIST_USB_PHY]			= "extcon_phy",
};

static const char * const SUNXI_SUPPLY_INPUT_LIMIT_TYPE_TEXT[] = {
	[SUNXI_POWER_INPUT_LIMIT_UNKNOWN]		= "Unknown",
	[SUNXI_POWER_INPUT_LIMIT_DEFAULT]		= "usb default type",
	[SUNXI_POWER_INPUT_LIMIT_MULTI_CHARGE]		= "usb multi charge type",
	[SUNXI_POWER_INPUT_LIMIT_PD_CHARGE]		= "usb pd type",
	[SUNXI_POWER_INPUT_LIMIT_PC]			= "usb pc type",
	[SUNXI_POWER_INPUT_LIMIT_TCPC]			= "usb type-c type",
	[SUNXI_POWER_INPUT_LIMIT_OTHERS]		= "limit by others",
	[SUNXI_POWER_INPUT_LIMIT_MAX]			= "limit type overflow",
};

#define sunxi_usb_power_limit_OF_PROP_READ(name, def_value)\
do {\
	if (of_property_read_u32(node, #name, &config->name))\
		config->name = def_value;\
} while (0)

#endif
