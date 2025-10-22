/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef _SUNXI_POWER_SUPPLY_H_
#define _SUNXI_POWER_SUPPLY_H_

/*------------------------------
 * AW Power Core
 *------------------------------*/
#include "sunxi-power-core.h"

/*------------------------------
 * Kernel Thread Management and Freezer
 *------------------------------*/
#include <linux/kthread.h>
#include <linux/freezer.h>

/*------------------------------
 * Device Model and Platform Devices
 *------------------------------*/
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/extcon.h>
#include <linux/extcon-provider.h>
#include <linux/pinctrl/consumer.h>

/*------------------------------
 * Power Supply Management
 *------------------------------*/
#include <linux/power_supply.h>

#endif /*  _SUNXI_POWER_SUPPLY_H_ */