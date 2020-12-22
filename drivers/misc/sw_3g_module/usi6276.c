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
*                                   
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
#include <linux/workqueue.h>


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

#define MODEM_NAME              "usi6276"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static struct sw_modem g_usi6276;
static char g_usi6276_name[] = MODEM_NAME;

static void do_wake(struct work_struct *work);

static DECLARE_DELAYED_WORK(wake_work, do_wake);


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void usi6276_reset(struct sw_modem *modem)
{
    modem_dbg("reset %s modem\n", modem->name);

	//modem_reset(modem, 0);
	modem_reset(modem, 0);
    sw_module_mdelay(100);
	modem_reset(modem, 1);
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
static void usi6276_sleep(struct sw_modem *modem, u32 sleep)
{
    modem_dbg("%s modem %s\n", modem->name, (sleep ? "sleep" : "wakeup"));

    if(sleep){
        modem_reset(modem, 0);
    }else{
        modem_reset(modem, 1);
    }

    return;
}

static void usi6276_rf_disable(struct sw_modem *modem, u32 disable)
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

void usi6276_power(struct sw_modem *modem, u32 on)
{
	modem_dbg("set %s modem power %s\n", modem->name, (on ? "on" : "off"));	

    if(on){
		/* default */
		//modem_vbat(modem, 1);
		modem_dldo_on_off(modem, 1);
		
		modem_power_on_off(modem, 0);
		sw_module_mdelay(5);
		modem_reset(modem,1);
		sw_module_mdelay(20);
		modem_power_on_off(modem, 1);

    }else{
    	modem_power_on_off(modem, 0);
		sw_module_mdelay(5);
		modem_reset(modem,0);
		sw_module_mdelay(20);
		modem_power_on_off(modem, 1);
		
		modem_dldo_on_off(modem, 0);
		//modem_vbat(modem, 0);
    }
    return;
}


static int usi6276_start(struct sw_modem *mdev)
{
    int ret = 0;
	modem_dbg("usi6276_start: start ======\n");
    if(!mdev->start){
        ret = modem_irq_init(mdev, IRQF_TRIGGER_FALLING);
        if(ret != 0){
           modem_err("err: sw_module_irq_init failed\n");
           return -1;
        }
		modem_dbg("before power on ======\n");
        usi6276_power(mdev, 1);
        mdev->start = 1;
    }

    return 0;
}

static int usi6276_stop(struct sw_modem *mdev)
{
    if(mdev->start){
        usi6276_power(mdev, 0);
        modem_irq_exit(mdev);
        mdev->start = 0;
    }

    return 0;
}

static void do_wake(struct work_struct *work)
{
    printk("do_wake\n");
    //mu509_sleep(mdev, 0); //mdev undeclared
    usi6276_sleep(&g_usi6276, 0); //by Cesc

}

static int usi6276_suspend(struct sw_modem *mdev)
{
    usi6276_sleep(mdev, 1);

    return 0;
}

static int usi6276_resume(struct sw_modem *mdev)
{
    //usi6276_sleep(mdev, 0);
	schedule_delayed_work(&wake_work, 3*HZ); 

    return 0;
}

static struct sw_modem_ops usi6276_ops = {
	.power          = usi6276_power,
	.reset          = usi6276_reset,
	.sleep          = usi6276_sleep,
	.rf_disable     = usi6276_rf_disable,

	.start          = usi6276_start,
	.stop           = usi6276_stop,

	.early_suspend  = modem_early_suspend,
	.early_resume   = modem_early_resume,

	.suspend        = usi6276_suspend,
	.resume         = usi6276_resume,
};

static struct platform_device usi6276_device = {
	.name				= SW_DRIVER_NAME,
	.id					= -1,

	.dev = {
		.platform_data  = &g_usi6276,
	},
};

static int __init usi6276_init(void)
{
    int ret = 0;

    memset(&g_usi6276, 0, sizeof(struct sw_modem));

    /* gpio */
    ret = modem_get_config(&g_usi6276);
    if(ret != 0){
        modem_err("err: usi6276_get_config failed\n");
        goto get_config_failed;
    }

    if(g_usi6276.used == 0){
        modem_err("usi6276 is not used\n");
        goto get_config_failed;
    }

    ret = modem_pin_init(&g_usi6276);
    if(ret != 0){
       modem_err("err: usi6276_pin_init failed\n");
       goto pin_init_failed;
    }

    /* ��ֹ�ű���ģ������bb_name���������Ʋ�һ�£����ֻʹ���������� */
//    if(g_usi6276.name[0] == 0){
        strcpy(g_usi6276.name, g_usi6276_name);
//    }
    g_usi6276.ops = &usi6276_ops;

    modem_dbg("%s modem init\n", g_usi6276.name);

    platform_device_register(&usi6276_device);

	return 0;
pin_init_failed:

get_config_failed:

    modem_dbg("%s modem init failed\n", g_usi6276.name);

	return -1;
}

static void __exit usi6276_exit(void)
{
    platform_device_unregister(&usi6276_device);
}

late_initcall(usi6276_init);
//module_init(usi6276_init);
module_exit(usi6276_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(MODEM_NAME);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");


