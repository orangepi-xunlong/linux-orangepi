/*
 * SoftWinners longcheer wm5608 3G module
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

/*
*******************************************************************************
*
*                                   龙旗 wm5608
*
*
* 注: 1、此描述为模组的内部引脚变化.
*     2、方案中要注意和模组连接时是否存在三极管.如果存在三极管，代码中的操作
*        顺序应该和模组的引脚操作顺序相反.
*
*
* 模组内部默认状态:
* vbat  : 低
* power : 高
* reset : 高
* sleep : 高
*
* 开机:
* (1)、power拉低
* (2)、延时200ms
* (3)、power拉高
*
* 关机:
* (1)、power拉低
* (2)、延时3.5s
* (3)、power拉高
*
* 复位:
* (1)、reset拉低
* (2)、延时150ms
* (3)、reset拉高
* (4)、延时10ms
*
* 休眠:
* (1)、sleep拉高
*
* 唤醒:
* (1)、sleep拉低
*
*******************************************************************************
*/

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#define DRIVER_DESC             SW_DRIVER_NAME
#define DRIVER_VERSION          "1.0"
#define DRIVER_AUTHOR			"Javen Xu"

#define MODEM_NAME             "wm5608"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static struct sw_modem g_wm5608;
static char g_wm5608_name[] = MODEM_NAME;

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void wm5608_reset(struct sw_modem *modem)
{
    modem_dbg("reset %s modem\n", modem->name);

    modem_reset(modem, 0);
    sw_module_mdelay(150);
    modem_reset(modem, 1);
    sw_module_mdelay(10);

    return;
}

static void wm5608_sleep(struct sw_modem *modem, u32 sleep)
{
    modem_dbg("%s modem %s\n", modem->name, (sleep ? "sleep" : "wakeup"));

    if(sleep){
        modem_sleep(modem, 1);
    }else{
        modem_sleep(modem, 0);
    }

    return;
}

static void wm5608_rf_disable(struct sw_modem *modem, u32 disable)
{
    modem_dbg("set %s modem rf %s\n", modem->name, (disable ? "disable" : "enable"));

    return;
}

void wm5608_power(struct sw_modem *modem, u32 on)
{
    modem_dbg("set %s modem power %s\n", modem->name, (on ? "on" : "off"));

    if(on){
        /* default */
    	modem_reset(modem, 1);
    	modem_power_on_off(modem, 1);
    	modem_sleep(modem, 0);
		msleep(10);

        /* power off, Prevent abnormalities restart of the PAD. */
        //如果电池和模组是直连，要执行一次关机动作，然后再执行开机流程

    	/* power on */
        modem_power_on_off(modem, 0);
        sw_module_mdelay(200);
        modem_power_on_off(modem, 1);
    }else{
        modem_power_on_off(modem, 0);
        sw_module_mdelay(3500);
        modem_power_on_off(modem, 1);
    }

    return;
}

static int wm5608_start(struct sw_modem *mdev)
{
    int ret = 0;

    ret = modem_irq_init(mdev, IRQF_TRIGGER_FALLING);
    if(ret != 0){
       modem_err("err: sw_module_irq_init failed\n");
       return -1;
    }

    wm5608_power(mdev, 1);

    return 0;
}

static int wm5608_stop(struct sw_modem *mdev)
{
    wm5608_power(mdev, 0);
    modem_irq_exit(mdev);

    return 0;
}

static int wm5608_suspend(struct sw_modem *mdev)
{
    wm5608_sleep(mdev, 1);

    return 0;
}

static int wm5608_resume(struct sw_modem *mdev)
{
    wm5608_sleep(mdev, 0);

    return 0;
}

static struct sw_modem_ops wm5608_ops = {
	.power          = wm5608_power,
	.reset          = wm5608_reset,
	.sleep          = wm5608_sleep,
	.rf_disable     = wm5608_rf_disable,

	.start          = wm5608_start,
	.stop           = wm5608_stop,

	.early_suspend  = modem_early_suspend,
	.early_resume   = modem_early_resume,

	.suspend        = wm5608_suspend,
	.resume         = wm5608_resume,
};

static struct platform_device wm5608_device = {
	.name				= SW_DRIVER_NAME,
	.id					= -1,

	.dev = {
		.platform_data  = &g_wm5608,
	},
};

static int __init wm5608_init(void)
{
    int ret = 0;

    memset(&g_wm5608, 0, sizeof(struct sw_modem));

    /* gpio */
    ret = modem_get_config(&g_wm5608);
    if(ret != 0){
        modem_err("err: wm5608_get_config failed\n");
        goto get_config_failed;
    }

    if(g_wm5608.used == 0){
        modem_err("wm5608 is not used\n");
        goto get_config_failed;
    }

    ret = modem_pin_init(&g_wm5608);
    if(ret != 0){
       modem_err("err: wm5608_pin_init failed\n");
       goto pin_init_failed;
    }

    /* 防止脚本的模组名称bb_name和驱动名称不一致，因此只使用驱动名称 */
//    if(g_wm5608.name[0] == 0){
        strcpy(g_wm5608.name, g_wm5608_name);
//    }
    g_wm5608.ops = &wm5608_ops;

    modem_dbg("%s modem init\n", g_wm5608.name);

    platform_device_register(&wm5608_device);

	return 0;
pin_init_failed:

get_config_failed:

    modem_dbg("%s modem init failed\n", g_wm5608.name);

	return -1;
}

static void __exit wm5608_exit(void)
{
    platform_device_unregister(&wm5608_device);
}

late_initcall(wm5608_init);
module_exit(wm5608_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(MODEM_NAME);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");




