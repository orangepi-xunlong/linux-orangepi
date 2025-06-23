// SPDX-License-Identifier: GPL-2.0
/*
 * rfkill bt driver for the cix bt
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#ifndef __RFKILL_BT_H__
#define __RFKILL_BT_H__

#include <linux/rfkill.h>

#define PROC_DIR 			"bluetooth/sleep"
#define RFKILL_RK_GPIO_NAME_SIZE	64

#define GPIO_ENABLE			1
#define GPIO_WAKE_BT			1
#define GPIO_WAKE_HOST			1

#define BT_NAME			 	"bluetooth"
#define DEBUG				1
#if DEBUG
#define DBG(x...) pr_err("[BT_RFKILL]: " x)
#define LOG(x...) pr_err("[BT_RFKILL]: " x)
#else
#define DBG(x...)
#define LOG(x...)
#endif

struct rfkill_cix_platform_data {
	char *name;
	enum rfkill_type type;
	struct gpio_desc *gpiod_reset;
	struct gpio_desc *gpiod_wake_bt;
	struct gpio_desc *gpiod_wake_host;
	struct platform_device *pdev;
	struct rfkill *rfkill_dev;
	struct proc_dir_entry *bluetooth_dir;
	struct proc_dir_entry *sleep_dir;
	struct input_dev *input;
	int rfkill_irq;
};

#endif