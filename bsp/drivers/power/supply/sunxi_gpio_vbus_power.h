/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef _SUNXI_GPIO_VBUS_H_
#define _SUNXI_GPIO_VBUS_H_

#include "sunxi-power-supply.h"

struct gpio_config {
	u32	data;
	u32	gpio;
	u32	mul_sel;
	u32	pull;
	u32	drv_level;
};

#define BMU_EXT_OF_PROP_READ(name, def_value)\
do {\
	if (of_property_read_u32(node, #name, &dts_info->name))\
		dts_info->name = def_value;\
} while (0)

struct axp_interrupts {
	char *name;
	irq_handler_t isr;
	int irq;
};

#define SUNXI_PINCFG_PACK(type, value)  (((value) << 8) | (type & 0xFF))

#endif
