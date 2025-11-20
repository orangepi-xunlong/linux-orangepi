/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef _SUNXI_USB_POWER_H_
#define _SUNXI_USB_POWER_H_

#include "sunxi-power-supply.h"
#include "sunxi-power-notifier.h"

struct sunxi_usb_power_gpio_para {
	int gpio;
	int irq_num;
};

struct sunxi_usb_power_supply_data {
	char					*name;
	struct device				*dev;
	struct power_supply			*usb_power_core_psy;
	struct power_supply			*usb_power_psy;

	/* power supply notifier */
	struct notifier_block			psy_nb;

	/* delayed work */
	struct delayed_work			usb_power_supply_mon;
	struct delayed_work			vbus_online_det_mon;

	/* atomic_t */
	atomic_t				vbus_online_status;

	/* gpio_vbus_detect */
	struct sunxi_usb_power_gpio_para	sunxi_usb_power_vbus_det;

	/* vbus_detect_type */
	int					vbus_detect_type;
};

#endif
