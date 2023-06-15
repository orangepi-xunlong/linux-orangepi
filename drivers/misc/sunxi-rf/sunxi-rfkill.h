/*
 * drivers/misc/sunxi-rf/sunxi-rfkill.h
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

#ifndef __SUNXI_RFKILL_H
#define __SUNXI_RFKILL_H

#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/rfkill.h>

#define WL_DEV_WIFI            0  /* bit0 */
#define WL_DEV_BLUETOOTH       1  /* bit1 */

#define CLK_MAX                5
#define PWR_MAX                5

struct sunxi_rfkill_platdata {
	struct pinctrl *pctrl;
	int gpio_power_en;
	bool gpio_power_en_assert;
	int gpio_chip_en;
	bool gpio_chip_en_assert;
};

struct sunxi_modem_platdata {
	struct regulator *power[PWR_MAX];
	char  *power_name[PWR_MAX];
	u32    power_vol[PWR_MAX];

	int gpio_modem_rst;
	bool gpio_modem_rst_assert;

	bool power_state;
	struct rfkill *rfkill;
	struct platform_device *pdev;
};

struct sunxi_bt_platdata {
	struct regulator *power[PWR_MAX];
	char  *power_name[PWR_MAX];
	u32    power_vol[PWR_MAX];

	struct clk *clk[CLK_MAX];
	char  *clk_name[CLK_MAX];

	int gpio_bt_rst;
	bool gpio_bt_rst_assert;

	int power_state;
	struct rfkill *rfkill;
	struct platform_device *pdev;
};

struct sunxi_wlan_platdata {
	unsigned int wakeup_enable;
	int bus_index;

	struct regulator *power[PWR_MAX];
	char  *power_name[PWR_MAX];
	u32    power_vol[PWR_MAX];

	struct clk *clk[CLK_MAX];
	char  *clk_name[CLK_MAX];

	int gpio_wlan_regon;
	bool gpio_wlan_regon_assert;
	int gpio_wlan_hostwake;
	bool gpio_wlan_hostwake_assert;

	int power_state;
	struct platform_device *pdev;
};

void sunxi_wlan_set_power(bool on_off);
int  sunxi_wlan_get_bus_index(void);
int  sunxi_wlan_get_oob_irq(int *irq_flags, int *wakeup_enable);
void sunxi_bluetooth_set_power(bool on_off);
void sunxi_modem_set_power(bool on_off);

#endif /* SUNXI_RFKILL_H */
