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
*                                   ŷ�� EM55
*
* ģ���ڲ�Ĭ��״̬:
* vbat  : ��
* power : ��
* reset : ��
* sleep : ��
*
* ����:
* (1)��sleep����
* (2)��vbat����
* (3)����ʱ1.2s
* (4)��power����
* (5)����ʱ2s
* (6)��power����
*
* �ػ�:
* (1)����λ�ػ���ִ�и�λ����
*
* ��λ:
* (1)��reset����
* (2)����ʱ60ms
* (3)��reset����
* (4)����ʱ10ms
*
* ����:
* (1)��sleep����
*
* ����:
* (1)��sleep����
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
#define DRIVER_AUTHOR			"H.J."

#define MODEM_NAME              "g3"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static struct sw_modem g_g3;
static char g_g3_name[] = MODEM_NAME;

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void g3_reset(struct sw_modem *modem)
{
    modem_dbg("reset %s modem\n", modem->name);

	//modem_reset(modem, 0);
	modem_reset(modem, 1);
    sw_module_mdelay(100);
	modem_reset(modem, 0);
	sw_module_mdelay(10);
	//modem_reset(modem, 1);

    return;
}

/*
*******************************************************************************
*
* (1)��wankeup : ���߳���100����, ���׳���100����, �����߳���100����, ������
* (2)��sleep   : ���߳���100����, ���׳���100����, �����߳���100����, ������
*
*******************************************************************************
*/
static void g3_sleep(struct sw_modem *modem, u32 sleep)
{
    modem_dbg("%s modem %s\n", modem->name, (sleep ? "sleep" : "wakeup"));

    if(sleep){
        modem_sleep(modem, 1);
    }else{
        modem_sleep(modem, 0);
    }

    return;
}

static void g3_rf_disable(struct sw_modem *modem, u32 disable)
{
    modem_dbg("set %s modem rf %s\n", modem->name, (disable ? "disable" : "enable"));

    modem_rf_disable(modem, disable);

    return;
}

/*
*******************************************************************************
* Ĭ��:
* vbat  : ��
* power : ��
* reset : ��
* sleep : ��
*
* ��������:
* (1)��vbat����
* (2)��power, ���ͳ���1.8s��������
* (3)��sleep, ���߳���100����, ���׳���100����, �����߳���100����, ������
*
* �ػ�����:
* (1)��vbat����
*
*******************************************************************************
*/
#if 0
void em55_power(struct sw_modem *modem, u32 on)
{
    modem_dbg("1set %s modem power %s\n", modem->name, (on ? "on" : "off"));

    if(on){
    	/* power on */
		modem_vbat(modem, 1);
		sw_module_mdelay(100);

        modem_power_on_off(modem, 0);
        sw_module_mdelay(1800);
        modem_power_on_off(modem, 1);
        sw_module_mdelay(1000);

        modem_sleep(modem, 1);
        sw_module_mdelay(100);
        modem_sleep(modem, 0);
        sw_module_mdelay(100);
        modem_sleep(modem, 1);
        sw_module_mdelay(100);
        modem_sleep(modem, 0);
    }else{
		modem_vbat(modem, 0);
    }

    return;
}
#else
void g3_power(struct sw_modem *modem, u32 on)
{
	modem_dbg("set %s modem power %s\n", modem->name, (on ? "on" : "off"));	

    if(on){
		
		//sw_module_mdelay(500);

		modem_dldo_on_off(modem, 1);

		g3_reset(modem);
		
        modem_sleep(modem, 0);

    	/* power on */
		//modem_vbat(modem, 1);
		//sw_module_mdelay(120);

        modem_power_on_off(modem, 1);
    }else{

		//modem_vbat(modem, 0);
		g3_reset(modem);

        modem_power_on_off(modem, 0);
		//modem_dldo_on_off(modem, 0);
    }
    return;
}
#endif

static int g3_start(struct sw_modem *mdev)
{
    int ret = 0;

    if(!mdev->start){
        ret = modem_irq_init(mdev, IRQF_TRIGGER_RISING);
        if(ret != 0){
           modem_err("err: sw_module_irq_init failed\n");
           return -1;
        }

        g3_power(mdev, 1);
        mdev->start = 1;
    }

    return 0;
}

static int g3_stop(struct sw_modem *mdev)
{
    if(mdev->start){
        g3_power(mdev, 0);
        modem_irq_exit(mdev);
        mdev->start = 0;
    }

    return 0;
}

static int g3_suspend(struct sw_modem *mdev)
{
    g3_sleep(mdev, 1);

    return 0;
}

static int g3_resume(struct sw_modem *mdev)
{
    g3_sleep(mdev, 0);

    return 0;
}

static struct sw_modem_ops g3_ops = {
	.power          = g3_power,
	.reset          = g3_reset,
	.sleep          = g3_sleep,
	.rf_disable     = g3_rf_disable,

	.start          = g3_start,
	.stop           = g3_stop,

	.early_suspend  = modem_early_suspend,
	.early_resume   = modem_early_resume,

	.suspend        = g3_suspend,
	.resume         = g3_resume,
};

static struct platform_device g3_device = {
	.name				= SW_DRIVER_NAME,
	.id					= -1,

	.dev = {
		.platform_data  = &g_g3,
	},
};

static int __init g3_init(void)
{
    int ret = 0;

    memset(&g_g3, 0, sizeof(struct sw_modem));

    /* gpio */
    ret = modem_get_config(&g_g3);
    if(ret != 0){
        modem_err("err: g3_get_config failed\n");
        goto get_config_failed;
    }

    if(g_g3.used == 0){
        modem_err("g3 is not used\n");
        goto get_config_failed;
    }

    ret = modem_pin_init(&g_g3);
    if(ret != 0){
       modem_err("err: g3_pin_init failed\n");
       goto pin_init_failed;
    }

    /* ��ֹ�ű���ģ������bb_name���������Ʋ�һ�£����ֻʹ���������� */
//    if(g_em55.name[0] == 0){
        strcpy(g_g3.name, g_g3_name);
//    }
    g_g3.ops = &g3_ops;

    modem_dbg("%s modem init\n", g_g3.name);

    platform_device_register(&g3_device);

	return 0;
pin_init_failed:

get_config_failed:

    modem_dbg("%s modem init failed\n", g_g3.name);

	return -1;
}

static void __exit g3_exit(void)
{
    platform_device_unregister(&g3_device);
}

late_initcall(g3_init);
//module_init(g3_init);
module_exit(g3_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(MODEM_NAME);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");


