/*
 * SoftWinners Oviphone 2G module
 *
 * Copyright (C) 2012 SoftWinners Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

/*
*******************************************************************************
*
*                                   欧孚 EM55
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
* (1)、sleep拉低
* (2)、vbat拉高
* (3)、延时1.2s
* (4)、power拉低
* (5)、延时2s
* (6)、power拉高
*
* 关机:
* (1)、复位关机，执行复位过程
*
* 复位:
* (1)、reset拉低
* (2)、延时60ms
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
#include <linux/regulator/consumer.h>

#include "sw_module.h"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#define DRIVER_DESC             SW_DRIVER_NAME
#define DRIVER_VERSION          "1.0"
#define DRIVER_AUTHOR			"Javen Xu"

#define MODEM_NAME              "em55"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static struct sw_modem g_em55;
static char g_em55_name[] = MODEM_NAME;

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void em55_reset(struct sw_modem *modem)
{
    modem_dbg("reset %s modem\n", modem->name);

	modem_reset(modem, 0);
    sw_module_mdelay(100);
	modem_reset(modem, 1);
	sw_module_mdelay(10);

    return;
}

/*
*******************************************************************************
*
* (1)、wankeup : 拉高持续100毫秒, 拉底持续100毫秒, 再拉高持续100毫秒, 再拉底
* (2)、sleep   : 拉高持续100毫秒, 拉底持续100毫秒, 再拉高持续100毫秒, 再拉高
*
*******************************************************************************
*/
static void em55_sleep(struct sw_modem *modem, u32 sleep)
{
    modem_dbg("%s modem %s\n", modem->name, (sleep ? "sleep" : "wakeup"));

    if(sleep){
        modem_sleep(modem, 1);
    }else{
        modem_sleep(modem, 0);
    }

    return;
}

static void em55_rf_disable(struct sw_modem *modem, u32 disable)
{
    modem_dbg("set %s modem rf %s\n", modem->name, (disable ? "disable" : "enable"));

    modem_rf_disable(modem, disable);

    return;
}

/*
*******************************************************************************
*
* 开机过程:
* (1)、执行关机流程
* (2)、置GPIO默认状态
* (3)、执行开机流程 (注意)
*   (3-1)、power拉低
*   (3-2)、延时2s
*   (3-3)、power拉低
*
* 关机过程:
* (1)、vbat拉低
*
*******************************************************************************
*/
void em55_power(struct sw_modem *modem, u32 on)
{
    modem_dbg("set %s modem power %s\n", modem->name, (on ? "on" : "off"));

    if(on){
		//modem_dldo_on_off(modem, 1);

        /* power off, Prevent abnormalities restart of the PAD. */
        //如果电池和模组是直连，要执行一次关机动作，然后再执行开机流程
		em55_reset(modem);

        /* default */
        modem_sleep(modem, 0);

        /* power on */
        modem_power_on_off(modem, 1);
        sw_module_mdelay(2000);
        modem_power_on_off(modem, 0);
    }else{

		//modem_vbat(modem, 0);
		em55_reset(modem);

		//modem_dldo_on_off(modem, 0);
    }

    return;
}

static int em55_start(struct sw_modem *mdev)
{
    int ret = 0;

    if(!mdev->start){
        ret = modem_irq_init(mdev, IRQF_TRIGGER_FALLING);
        if(ret != 0){
           modem_err("err: sw_module_irq_init failed\n");
           return -1;
        }

        em55_power(mdev, 1);
        mdev->start = 1;
    }

    return 0;
}

static int em55_stop(struct sw_modem *mdev)
{
    if(mdev->start){
        em55_power(mdev, 0);
        modem_irq_exit(mdev);
        mdev->start = 0;
    }

    return 0;
}

static int em55_suspend(struct sw_modem *mdev)
{
    em55_sleep(mdev, 1);

    return 0;
}

static int em55_resume(struct sw_modem *mdev)
{
    em55_sleep(mdev, 0);

    return 0;
}

static struct sw_modem_ops em55_ops = {
	.power          = em55_power,
	.reset          = em55_reset,
	.sleep          = em55_sleep,
	.rf_disable     = em55_rf_disable,

	.start          = em55_start,
	.stop           = em55_stop,

	.early_suspend  = modem_early_suspend,
	.early_resume   = modem_early_resume,

	.suspend        = em55_suspend,
	.resume         = em55_resume,
};

static struct platform_device em55_device = {
	.name				= SW_DRIVER_NAME,
	.id					= -1,

	.dev = {
		.platform_data  = &g_em55,
	},
};

static int __init em55_init(void)
{
    int ret = 0;

    memset(&g_em55, 0, sizeof(struct sw_modem));

    /* gpio */
    ret = modem_get_config(&g_em55);
    if(ret != 0){
        modem_err("err: em55_get_config failed\n");
        goto get_config_failed;
    }

    if(g_em55.used == 0){
        modem_err("em55 is not used\n");
        goto get_config_failed;
    }

    ret = modem_pin_init(&g_em55);
    if(ret != 0){
       modem_err("err: em55_pin_init failed\n");
       goto pin_init_failed;
    }

    /* 防止脚本的模组名称bb_name和驱动名称不一致，因此只使用驱动名称 */
//    if(g_em55.name[0] == 0){
        strcpy(g_em55.name, g_em55_name);
//    }
    g_em55.ops = &em55_ops;

    modem_dbg("%s modem init\n", g_em55.name);

    platform_device_register(&em55_device);

	return 0;
pin_init_failed:

get_config_failed:

    modem_dbg("%s modem init failed\n", g_em55.name);

	return -1;
}

static void __exit em55_exit(void)
{
    platform_device_unregister(&em55_device);
}

late_initcall(em55_init);
//module_init(em55_init);
module_exit(em55_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(MODEM_NAME);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");


