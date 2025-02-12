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
#include <drv_types.h>

extern int ky_wlan_set_power(int on);
extern int ky_wlan_get_oob_irq(void);
extern void ky_sdio_detect_change(int enable_scan);

void platform_wifi_get_oob_irq(int *oob_irq)
{
	*oob_irq = ky_wlan_get_oob_irq();
}

void platform_wifi_mac_addr(u8 *mac_addr)
{

}

/*
 * Return:
 *	0:	power on successfully
 *	others:	power on failed
 */
int platform_wifi_power_on(void)
{
	int ret = 0;

	RTW_PRINT("\n");
	RTW_PRINT("=======================================================\n");
	RTW_PRINT("==== Launching Wi-Fi driver! (Powered by Ky) ====\n");
	RTW_PRINT("=======================================================\n");
	RTW_PRINT("Realtek %s WiFi driver (Powered by Ky,Ver %s) init.\n", DRV_NAME, DRIVERVERSION);
	ky_wlan_set_power(1);
	ky_sdio_detect_change(1);

	return ret;
}

void platform_wifi_power_off(void)
{
	RTW_PRINT("\n");
	RTW_PRINT("=======================================================\n");
	RTW_PRINT("==== Dislaunching Wi-Fi driver! (Powered by Ky) ====\n");
	RTW_PRINT("=======================================================\n");
	RTW_PRINT("Realtek %s WiFi driver (Powered by Ky,Ver %s) init.\n", DRV_NAME, DRIVERVERSION);
	ky_sdio_detect_change(0);
	ky_wlan_set_power(0);
}
