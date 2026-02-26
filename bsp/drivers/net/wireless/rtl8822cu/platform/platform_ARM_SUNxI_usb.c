/******************************************************************************
 *
 * Copyright(c) 2013 - 2017 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
/*
 * Description:
 *	This file can be applied to following platforms:
 *	CONFIG_PLATFORM_ARM_SUNXI Series platform
 *
 */

#include <drv_types.h>

#ifdef CONFIG_PLATFORM_ARM_SUNxI
extern void sunxi_wlan_set_power(bool on_off);
#endif

int platform_wifi_power_on(void)
{
#ifdef CONFIG_PLATFORM_ARM_SUNxI
	sunxi_wlan_set_power(1);
#endif
	return 0;
}

void platform_wifi_power_off(void)
{
#ifdef CONFIG_PLATFORM_ARM_SUNxI
	sunxi_wlan_set_power(0);
#endif
}
