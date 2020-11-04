/*
 * drivers/power/axp/axp-powerkey.h
 * (C) Copyright 2010-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Pannan <pannan@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef AXP_POWERKEY_H
#define AXP_POWERKEY_H

#include "axp-charger.h"

struct axp_powerkey_info {
	struct input_dev *idev;
	struct axp_dev *chip;
};

extern struct axp_interrupts axp_powerkey_irq[4];
extern int axp_powerkey_dt_parse(struct device_node *node,
			struct axp_config_info *axp_config);

#endif /* AXP_POWERKEY_H */
