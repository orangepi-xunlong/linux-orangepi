/*
 * SoftWinners yuga cwm600 3G module
 *
 * Copyright (C) 2012 SoftWinners Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/kmemcheck.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/idr.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/signal.h>

#include <linux/time.h>
#include <linux/timer.h>

#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <linux/clk.h>
#include <linux/gpio.h>

#include "sw_module.h"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#define DRIVER_DESC             SW_DRIVER_NAME
#define DRIVER_VERSION          "1.0"
#define DRIVER_AUTHOR			"Javen Xu"

#define MODEM_NAME             "cwm600"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static struct sw_modem g_cwm600;
static char g_cwm600_name[] = MODEM_NAME;

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void cwm600_reset(struct sw_modem *modem)
{
    modem_dbg("reset %s modem\n", modem->name);

	modem_reset(modem, 0);
    sw_module_mdelay(100);
	modem_reset(modem, 1);

    return;
}

/*
*******************************************************************************
*
* wakeup_in:
*   H: wakeup cwm600.
*   L: set cwm600 to sleep mode.
*
*******************************************************************************
*/
static void cwm600_sleep(struct sw_modem *modem, u32 sleep)
{
    modem_dbg("%s modem %s\n", modem->name, (sleep ? "sleep" : "wakeup"));

    if(sleep){
        modem_sleep(modem, 0);
    }else{
        modem_sleep(modem, 1);
    }

    return;
}

static void cwm600_rf_disable(struct sw_modem *modem, u32 disable)
{
    modem_dbg("set %s modem rf %s\n", modem->name, (disable ? "disable" : "enable"));

    modem_rf_disable(modem, disable);

    return;
}

/*
*******************************************************************************
* 模组内部默认:
* vbat  : 低
* power : 高
* reset : 高
* sleep : 高
*
* 开机过程:
* (1)、默认pin配置，power拉高、reset拉高、sleep拉高
* (1)、vbat拉高
* (2)、power, 拉低持续0.7s，后拉高
*
* 关机过程:
* (1)、power, 拉低持续2.5s，后拉高
* (2)、vbat拉低
*
*******************************************************************************
*/
void cwm600_power(struct sw_modem *modem, u32 on)
{
    modem_dbg("set %s modem power %s\n", modem->name, (on ? "on" : "off"));

    if(on){
        /* default */
    	modem_reset(modem, 1);
    	modem_power_on_off(modem, 1);
    	modem_sleep(modem, 1);

        /* power off, Prevent abnormalities restart of the PAD. */
        //如果电池和模组是直连，要执行一次关机动作，然后再执行开机流程

    	/* power on */
		modem_vbat(modem, 1);
		msleep(100);

        modem_power_on_off(modem, 0);
        sw_module_mdelay(700);
        modem_power_on_off(modem, 1);
    }else{
        //modem_power_on_off(modem, 0);
        //sw_module_mdelay(2500);
        //modem_power_on_off(modem, 1);
		modem_vbat(modem, 0);
    }

    return;
}

static int cwm600_start(struct sw_modem *mdev)
{
    int ret = 0;

    ret = modem_irq_init(mdev, IRQF_TRIGGER_FALLING);
    if(ret != 0){
       modem_err("err: sw_module_irq_init failed\n");
       return -1;
    }

    cwm600_power(mdev, 1);

    return 0;
}

static int cwm600_stop(struct sw_modem *mdev)
{
    cwm600_power(mdev, 0);
    modem_irq_exit(mdev);

    return 0;
}

static int cwm600_suspend(struct sw_modem *mdev)
{
    cwm600_sleep(mdev, 1);

    return 0;
}

static int cwm600_resume(struct sw_modem *mdev)
{
    cwm600_sleep(mdev, 0);

    return 0;
}

static struct sw_modem_ops cwm600_ops = {
	.power          = cwm600_power,
	.reset          = cwm600_reset,
	.sleep          = cwm600_sleep,
	.rf_disable     = cwm600_rf_disable,

	.start          = cwm600_start,
	.stop           = cwm600_stop,

	.early_suspend  = modem_early_suspend,
	.early_resume   = modem_early_resume,

	.suspend        = cwm600_suspend,
	.resume         = cwm600_resume,
};

static struct platform_device cwm600_device = {
	.name				= SW_DRIVER_NAME,
	.id					= -1,

	.dev = {
		.platform_data  = &g_cwm600,
	},
};

static int __init cwm600_init(void)
{
    int ret = 0;

    memset(&g_cwm600, 0, sizeof(struct sw_modem));

    /* gpio */
    ret = modem_get_config(&g_cwm600);
    if(ret != 0){
        modem_err("err: cwm600_get_config failed\n");
        goto get_config_failed;
    }

    if(g_cwm600.used == 0){
        modem_err("cwm600 is not used\n");
        goto get_config_failed;
    }

    ret = modem_pin_init(&g_cwm600);
    if(ret != 0){
       modem_err("err: cwm600_pin_init failed\n");
       goto pin_init_failed;
    }

    /* 防止脚本的模组名称bb_name和驱动名称不一致，因此只使用驱动名称 */
//    if(g_cwm600.name[0] == 0){
        strcpy(g_cwm600.name, g_cwm600_name);
//    }
    g_cwm600.ops = &cwm600_ops;

    modem_dbg("%s modem init\n", g_cwm600.name);

    platform_device_register(&cwm600_device);

	return 0;
pin_init_failed:

get_config_failed:

    modem_dbg("%s modem init failed\n", g_cwm600.name);

	return -1;
}

static void __exit cwm600_exit(void)
{
    platform_device_unregister(&cwm600_device);
}

late_initcall(cwm600_init);
//module_init(cwm600_init);
module_exit(cwm600_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(MODEM_NAME);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");

