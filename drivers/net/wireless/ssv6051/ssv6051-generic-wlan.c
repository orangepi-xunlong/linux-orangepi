/*
 * Copyright (c) 2015 South Silicon Valley Microelectronics Inc.
 * Copyright (c) 2015 iComm Corporation
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/irq.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/gpio.h>
#include <linux/mmc/host.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <asm/io.h>
#include <linux/printk.h>
#include <linux/err.h>

static int g_wifidev_registered = 0;
extern int ssvdevice_init(void);
extern void ssvdevice_exit(void);
extern int ssv6xxx_get_dev_status(void);

static __init int ssv_init_module(void)
{
	int ret = 0;
	int time = 5;

	msleep(120);

	g_wifidev_registered = 1;
	ret = ssvdevice_init();

	while(time-- > 0){
		msleep(500);
		if(ssv6xxx_get_dev_status() == 1)
			break;
		pr_info("%s : Retry to carddetect\n",__func__);
	}

	return ret;

}
static __exit void ssv_exit_module(void)
{

	if (g_wifidev_registered)
    {
        ssvdevice_exit();
        msleep(50);
        g_wifidev_registered = 0;
    }

    return;

}

module_init(ssv_init_module);
module_exit(ssv_exit_module);

MODULE_AUTHOR("iComm Semiconductor Co., Ltd");
MODULE_FIRMWARE("ssv*-sw.bin");
MODULE_FIRMWARE("ssv*-wifi.cfg");
MODULE_DESCRIPTION("Shared library for SSV wireless LAN cards.");
MODULE_LICENSE("Dual BSD/GPL");

