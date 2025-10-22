/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef _SUNXI_POWER_NOTIFIER_H_
#define _SUNXI_POWER_NOTIFIER_H_

/*------------------------------
 * AW Power Core
 *------------------------------*/
#include "sunxi-power-core.h"

/*------------------------------
 * Notifier Management
 *------------------------------*/
#include <linux/notifier.h>

/*------------------------------
 * AW Notifier Struct
 *------------------------------*/
enum sunxi_power_init_notifier_events {
	AW_PSY_EVENT_NONE = 0,
	/* power supply init done */
	AW_PSY_EVENT_BAT_INIT_DONE,
	AW_PSY_EVENT_USB_INIT_DONE,
	AW_PSY_EVENT_ACIN_INIT_DONE,
	AW_PSY_EVENT_USB_PD_INIT_DONE,
	AW_PSY_EVENT_GAUGE_INIT_DONE,
	/* check power supply status */
	AW_PSY_EVENT_VBUS_ONLINE_CHECK,
	AW_PSY_EVENT_BAT_TEMP_CHECK,
	AW_PSY_EVENT_MAX = 0,
};

int sunxi_power_supply_reg_notifier(struct notifier_block *nb);
void sunxi_power_supply_unreg_notifier(struct notifier_block *nb);
void sunxi_call_power_supply_notifier(unsigned long event);
void sunxi_call_power_supply_notifier_with_data(unsigned long event, void *data);

#endif