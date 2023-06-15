/*
 * drivers/misc/sunxi-rf/internal.h
 *
 * Copyright (c) 2014 softwinner.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __INTERNEL_H
#define __INTERNEL_H

#include <linux/platform_device.h>

void rfkill_poweren_set(int dev, int on_off);
void rfkill_chipen_set(int dev, int on_off);

int sunxi_wlan_init(struct platform_device *);
int sunxi_wlan_deinit(struct platform_device *);

int sunxi_bt_init(struct platform_device *);
int sunxi_bt_deinit(struct platform_device *);

int sunxi_modem_init(struct platform_device *);
int sunxi_modem_deinit(struct platform_device *);

#endif
