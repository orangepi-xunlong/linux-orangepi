/******************************************************************************
 *
 * Copyright(c) 2013 Realtek Corporation. All rights reserved.
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
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/
#include <drv_types.h>
#include <linux/gpio.h>

extern int cvi_sdio_rescan(void);
extern int cvi_get_wifi_pwr_on_gpio(void);
extern int cvi_get_wifi_wakeup_gpio(void);
// #ifndef ANDROID_2X
// extern int sdhci_device_attached(void);
// #endif

static int gpio_power_on;
// static int gpio_wake_up;
/*
 * Return:
 *	0:	power on successfully
 *	others:	power on failed
 */
int platform_wifi_power_on(void)
{
	gpio_power_on = cvi_get_wifi_pwr_on_gpio();
	if (gpio_power_on > 0)
		gpio_direction_output(gpio_power_on, 1);
	else {
		printk("power on gpio request error!\n");
		return -1;
	}
	printk("power on rtl8189.\n");
	msleep(500);
	cvi_sdio_rescan();
	printk("[rtl8189es] %s: new card, power on.\n", __FUNCTION__);
	return 0;
}

void platform_wifi_power_off(void)
{
	int err = 0;
	if (gpio_power_on >= 0)
	{
		err = gpio_direction_output(gpio_power_on, 0);
		if (err)
		{
			printk("%s: WL_PW_ON didn't output low\n", __FUNCTION__);
			return ;
		}
	}
	printk(KERN_INFO "Wifi sdio power off\n");
}
