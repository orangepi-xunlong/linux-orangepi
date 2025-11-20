/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef _SUNXI_POWER_TYPEC_H_
#define _SUNXI_POWER_TYPEC_H_

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
#include <linux/i2c.h>
#include <linux/property.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/extcon.h>
#include <linux/extcon-provider.h>
#include <linux/pinctrl/consumer.h>

/*------------------------------
 * Typec
 *------------------------------*/
#include <linux/usb/tcpm.h>
#include <linux/usb/role.h>
#include <linux/usb/pd.h>
#include <linux/usb/typec.h>

#endif /*  _SUNXI_POWER_TYPEC_H_ */