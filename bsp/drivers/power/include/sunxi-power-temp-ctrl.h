/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef _SUNXI_POWER_TEMP_CTRL_H_
#define _SUNXI_POWER_TEMP_CTRL_H_

/*------------------------------
 * AW Power Core
 *------------------------------*/
#include "sunxi-power-core.h"

/*------------------------------
 * Thermal Management
 *------------------------------*/
#include <linux/thermal.h>

/*------------------------------
 * Power Supply Management
 *------------------------------*/
#include <linux/power_supply.h>

enum battery_cooling_state {
	NORMAL_STATE = 0,
	LIMIT_CUR_STATE = 2,
	PAUSE_CHARGING_STATE = 3,
};

enum sunxi_power_ts_temp_status {
	AW_TEMP_STATUS_GOOD = 0,
	/* power supply init done */
	AW_TEMP_STATUS_COLD,
	AW_TEMP_STATUS_COOL,
	AW_TEMP_STATUS_WARM,
	AW_TEMP_STATUS_HOT,
	AW_TEMP_STATUS_SHUTDOWN,
};

#endif