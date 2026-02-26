/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef _SUNXI_POWER_CORE_H_
#define _SUNXI_POWER_CORE_H_

/*------------------------------
 * Kernel Core and Module
 *------------------------------*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/ktime.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/timekeeping.h>
#include <linux/version.h>

/*------------------------------
 * Error Handling
 *------------------------------*/
#include <linux/err.h>
#include <linux/errno.h>

/*------------------------------
 * Interrupts and IRQ Management
 *------------------------------*/
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <asm/irq.h>

/*------------------------------
 * Memory Management
 *------------------------------*/
#include <linux/slab.h>
#include <asm/io.h>

/*------------------------------
 * Delay Operations
 *------------------------------*/
#include <linux/delay.h>

/*------------------------------
 * Device Model and Platform Devices
 *------------------------------*/
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/cdev.h>

/*------------------------------
 * Power Management
 *------------------------------*/
#include <linux/pm_runtime.h>
#include <linux/pm_wakeirq.h>

/*------------------------------
 * Error Handling and Initialization
 *------------------------------*/
#include <linux/err.h>
#include <linux/init.h>
#include <linux/errno.h>

/*------------------------------
 * Mutex and Synchronization
 *------------------------------*/
#include <linux/mutex.h>

/*------------------------------
 * Register Map Management
 *------------------------------*/
#include <linux/regmap.h>

/*------------------------------
 * AW Management
 *------------------------------*/
#include "sunxi-power-debug.h"

#endif /*  _SUNXI_POWER_CORE_H_ */