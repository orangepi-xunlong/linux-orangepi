/*
 * SoftWinners 3G module driver
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
#include <linux/kthread.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/pm.h>
#include <linux/earlysuspend.h>
#endif

#include <linux/time.h>
#include <linux/timer.h>

#include <linux/clk.h>
#include <linux/gpio.h>

#include <asm/io.h>

#include <mach/sys_config.h>
#include <mach/gpio.h>
#include  <mach/platform.h>

#include "sw_module.h"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#define DRIVER_DESC             SW_DRIVER_NAME
#define DRIVER_VERSION          "1.0"
#define DRIVER_AUTHOR			"Javen Xu"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
struct sw_module_dev{
	char name[SW_3G_NAME_LEN];
	struct device *class_dev;
    struct platform_device *pdev;
    spinlock_t lock;

	/* modem init */
	struct task_struct *thread;
	struct completion thread_started;
	struct completion thread_exited;

    struct sw_modem *modem;

#ifdef CONFIG_HAS_EARLYSUSPEND
    struct early_suspend early_suspend;
#endif
};

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static char g_module_name[] = SW_DRIVER_NAME;
static struct class *sw_module_class;
static struct sw_module_dev g_mdev;
static int g_sw_module_power = 0;

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

static ssize_t sw_module_show_name(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct sw_module_dev *mdev = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", mdev->name);
}

static ssize_t sw_module_show_power(struct device *dev,
                struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", g_sw_module_power);
}

static ssize_t sw_module_set_power(struct device *dev,
                struct device_attribute *attr,
		        const char *buf, size_t count)
{
	struct sw_module_dev *mdev = dev_get_drvdata(dev);
    int value = 0;

    sscanf(buf, "%d", &value);
    g_sw_module_power = value;
    if(g_sw_module_power){
        if(mdev->modem->ops->start){
            mdev->modem->ops->start(mdev->modem);
        }
    }else{
        if(mdev->modem->ops->stop){
            mdev->modem->ops->stop(mdev->modem);
        }
    }

	return count;
}

static struct device_attribute sw_module_attrs[] = {
	__ATTR(modem_name, 0444, sw_module_show_name, NULL),
    __ATTR(modem_power, 0777, sw_module_show_power, sw_module_set_power),

	__ATTR_NULL,
};

static int __sw_modem_start(void * pArg)
{
	struct sw_module_dev *mdev = pArg;

	allow_signal(SIGTERM);
	complete(&mdev->thread_started);

	g_sw_module_power = 1;

    if(mdev->modem->ops->start){
        mdev->modem->ops->start(mdev->modem);
    }

	return 0;
}

static int sw_modem_start(struct sw_module_dev *mdev)
{
	init_completion(&mdev->thread_started);

    mdev->thread = kthread_create(__sw_modem_start, mdev, "sw-3g");
    if (IS_ERR(mdev->thread)) {
        modem_err("%s: failed to create kernel_thread (%ld)!\n", __func__, PTR_ERR(mdev->thread));
        return -1;
    }

    wake_up_process(mdev->thread);
    wait_for_completion(&mdev->thread_started);

    return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void sw_module_early_suspend(struct early_suspend *h)
{
    if(g_mdev.modem->ops->early_suspend){
        g_mdev.modem->ops->early_suspend(g_mdev.modem);
    }

    return;
}

static void sw_module_early_resume(struct early_suspend *h)
{
    if(g_mdev.modem->ops->early_resume){
        g_mdev.modem->ops->early_resume(g_mdev.modem);
    }

    return;
}
#endif

static int __init sw_module_probe(struct platform_device *pdev)
{
	struct sw_module_dev *mdev = &g_mdev;
	struct sw_modem *modem = NULL;
	int ret = 0;

	if(pdev == NULL){
        modem_err("err: invalid argment\n");
        return -1;
	}

	modem = pdev->dev.platform_data;
	if(modem == NULL){
        modem_err("err: modem is null\n");
        return -1;
	}

	memset(mdev, 0, sizeof(struct sw_module_dev));
	spin_lock_init(&mdev->lock);
	strcpy(mdev->name, modem->name);
	mdev->modem = modem;
	modem->prv = mdev;

    modem_dbg("%s modem probe\n", mdev->name);

	/* create device */
    mdev->class_dev = device_create(sw_module_class, NULL, 0, mdev, SW_DEVICE_NODE_NAME);
    if (IS_ERR(mdev->class_dev)) {
        modem_err("err: device_create failed\n");
        goto error_device_create;
    }

#ifdef CONFIG_HAS_EARLYSUSPEND
    if(mdev->modem->ops->early_suspend && mdev->modem->ops->early_resume){
        mdev->early_suspend.suspend = sw_module_early_suspend;
        mdev->early_suspend.resume = sw_module_early_resume;
        register_early_suspend(&mdev->early_suspend);
    }
#endif

    ret = sw_modem_start(mdev);
    if(ret != 0){
		modem_err("err: probe %s failed\n", mdev->name);
		goto err_modem_start;
	}

	return 0;

err_modem_start:
    device_destroy(sw_module_class, 0);

error_device_create:

    return -1;
}

static int sw_module_remove(struct platform_device *pdev)
{
	struct sw_module_dev *mdev = NULL;
	struct sw_modem *modem = NULL;

	if(pdev == NULL){
        modem_err("err: invalid argment\n");
        return -1;
	}

	modem = pdev->dev.platform_data;
	if(modem == NULL){
        modem_err("err: modem is null\n");
        return -1;
	}

	mdev = modem->prv;
	if(mdev == NULL){
        modem_err("err: mdev is null\n");
        return -1;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
    if(mdev->modem->ops->early_suspend && mdev->modem->ops->early_resume){
        unregister_early_suspend(&mdev->early_suspend);
    }
#endif

    device_destroy(sw_module_class, 0);

    return 0;
}

void sw_module_shutdown(struct platform_device* pdev)
{
	struct sw_module_dev *mdev = NULL;
	struct sw_modem *modem = NULL;

	if(pdev == NULL){
        modem_err("err: invalid argment\n");
        return;
	}

	modem = pdev->dev.platform_data;
	if(modem == NULL){
        modem_err("err: modem is null\n");
        return;
	}

	mdev = modem->prv;
	if(mdev == NULL){
        modem_err("err: mdev is null\n");
        return;
	}

	if(mdev->modem->ops->stop){
	    mdev->modem->ops->stop(mdev->modem);
	}

    return;
}

#ifdef CONFIG_PM

static int sw_module_suspend(struct platform_device *pdev, pm_message_t message)
{
	struct sw_module_dev *mdev = NULL;
	struct sw_modem *modem = NULL;

	if(pdev == NULL){
        modem_err("err: invalid argment\n");
        return 0;
	}

	modem = pdev->dev.platform_data;
	if(modem == NULL){
        modem_err("err: modem is null\n");
        return 0;
	}

	mdev = modem->prv;
	if(mdev == NULL){
        modem_err("err: mdev is null\n");
        return 0;
	}

	if(mdev->modem->ops->suspend){
	    mdev->modem->ops->suspend(mdev->modem);
	}

    return 0;
}

static int sw_module_resume(struct platform_device *pdev)
{
	struct sw_module_dev *mdev = NULL;
	struct sw_modem *modem = NULL;

	if(pdev == NULL){
        modem_err("err: invalid argment\n");
        return 0;
	}

	modem = pdev->dev.platform_data;
	if(modem == NULL){
        modem_err("err: modem is null\n");
        return 0;
	}

	mdev = modem->prv;
	if(mdev == NULL){
        modem_err("err: mdev is null\n");
        return 0;
	}

	if(mdev->modem->ops->resume){
	    mdev->modem->ops->resume(mdev->modem);
	}

    return 0;
}

#else

static int sw_module_suspend(struct platform_device *pdev, pm_message_t message)
{
    return 0;
}

static int sw_module_resume(struct platform_device *pdev)
{
    return 0;
}

#endif

static struct platform_driver sw_module_driver = {
	.driver		= {
		.name	= g_module_name,
		.owner	= THIS_MODULE,
	},

    .probe      = sw_module_probe,
    .remove     = sw_module_remove,
    .shutdown   = sw_module_shutdown,
    .suspend    = sw_module_suspend,
    .resume     = sw_module_resume,
};

static int __init sw_module_init(void)
{
    int ret = 0;

    modem_dbg("sw_module_init %s\n", DRIVER_VERSION);

	sw_module_class = class_create(THIS_MODULE, "sw_3g_module");
	if (IS_ERR(sw_module_class)) {
		modem_err("err: failed to allocate class\n");
		return PTR_ERR(sw_module_class);
	}
	sw_module_class->dev_attrs = sw_module_attrs;

	ret = platform_driver_register(&sw_module_driver);
	if(ret){
        modem_err("ERR: platform_driver_probe failed\n");
        class_destroy(sw_module_class);
        return -1;
    }

	return 0;
}

static void __exit sw_module_exit(void)
{
	platform_driver_unregister(&sw_module_driver);
	class_destroy(sw_module_class);

    return;
}

late_initcall(sw_module_init);
//module_init(sw_module_init);
module_exit(sw_module_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");










