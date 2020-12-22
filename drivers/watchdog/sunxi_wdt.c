/*
 *	sunxi Watchdog Driver
 *
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd. 
 *      http://www.allwinnertech.com
 *
 * Author: huangshr <huangshr@allwinnertech.com>
 *
 * allwinner sunxi SoCs Watchdog driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Based on xen_wdt.c
 * (c) Copyright 2010 Novell, Inc.
 *
 * Known issues:
 *  * A Watchdog's function is to return an unresponsive system to 
 *  * operational state. It does this by periodically checking the 
 *  * system's pulse and issuing a reset if it can't detect any.
 *  * Application software is responsible for registering this pulse 
 *  * periodically petting the watchdong using the services of a 
 *  * watchdog device driver.
 */
 
#define DRV_NAME	"sunxi_wdt"
#define DRV_VERSION	"1.0"
#define PFX		DRV_NAME ": "

#include <linux/bug.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/resource.h>
#include <linux/uaccess.h>
#include <linux/watchdog.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <mach/platform.h>

#define WATCHDOG_DBG

#ifdef CONFIG_ARCH_SUN8I
#define WDT_FUNC	TO_WHOLE_SYSTEM	/* initial watchdog function,
					 * should be in sunxi_watchdog_func
					 */
#endif /* CONFIG_ARCH_SUN8I */

#ifdef CONFIG_ARCH_SUN9I
#define WDT_FUNC	TO_WHOLE_SYSTEM
#endif /* CONFIG_ARCH_SUN9I */

#define MAX_TIMEOUT 		16 	/* max 16 seconds */

static struct platform_device *platform_device;
static bool is_active, expect_release;

static struct sunxi_watchdog_reg {
	u32 	irq_en;
	u32 	irq_pd;
	u32 	reserved[2];
	u32 	ctrl;
	u32 	config;
	u32 	mode;
} __iomem *wdt_reg;

enum sunxi_watchdog_func {
	TO_WHOLE_SYSTEM = 1,	/* reset the whole system */
	ONLY_INTERRUPT = 2,		/* only generate interrupt, not reset */
};

struct sunxi_watchdog_interv{
	u32 	timeout;
	u32 	interv;
};

static const struct sunxi_watchdog_interv g_timeout_interv[] = {
	{.timeout = 0.5, .interv = 0b0000},
	{.timeout = 1  , .interv = 0b0001},
	{.timeout = 2  , .interv = 0b0010},
	{.timeout = 3  , .interv = 0b0011},
	{.timeout = 4  , .interv = 0b0100},
	{.timeout = 5  , .interv = 0b0101},
	{.timeout = 6  , .interv = 0b0110},
	{.timeout = 8  , .interv = 0b0111},
	{.timeout = 10 , .interv = 0b1000},
	{.timeout = 12 , .interv = 0b1001},
	{.timeout = 14 , .interv = 0b1010},
	{.timeout = 16 , .interv = 0b1011},
};

static unsigned int g_timeout = MAX_TIMEOUT; /* watchdog timeout in second */
module_param(g_timeout, uint, S_IRUGO);
MODULE_PARM_DESC(g_timeout, "Watchdog g_timeout in seconds "
			"(default=" __MODULE_STRING(MAX_TIMEOUT) ")");

static bool g_nowayout = WATCHDOG_NOWAYOUT;
module_param(g_nowayout, bool, S_IRUGO);
MODULE_PARM_DESC(g_nowayout, "Watchdog cannot be stopped once started "
			"(default=" __MODULE_STRING(WATCHDOG_NOWAYOUT) ")");
	
static struct resource sunxi_wdt_res[] = {
	{
		.start	= SUNXI_TIMER_PBASE + 0xA0,
		.end	= SUNXI_TIMER_PBASE + 0xA0 + 0x20-1, 
		.flags	= IORESOURCE_MEM,
	},
};

static int wdt_irq_en(bool enable)
{
#ifdef WATCHDOG_DBG
	if(NULL == wdt_reg) {
		pr_err("%s err, line %d\n", __func__, __LINE__);
		return -1;
	}
#endif

	if(true == enable)
		writel(1, &wdt_reg->irq_en);
	else
		writel(0, &wdt_reg->irq_en);
	return 0;
}

static int wdt_clr_irq_pend(void)
{
	int 	temp = 0;

#ifdef WATCHDOG_DBG
	if(NULL == wdt_reg) {
		pr_err("%s err, line %d\n", __func__, __LINE__);
		return -1;
	}
#endif

	temp = readl(&wdt_reg->irq_pd) & 0x1;
	writel(temp, &wdt_reg->irq_pd);
	return 0;
}

static int timeout_to_interv(int timeout_in_sec)
{
	int 	temp;
	int 	array_sz;

	array_sz = ARRAY_SIZE(g_timeout_interv);
	if(timeout_in_sec > g_timeout_interv[array_sz - 1].timeout)
		return g_timeout_interv[array_sz - 1].interv;
	else if(timeout_in_sec < g_timeout_interv[0].timeout)
		return g_timeout_interv[0].interv;
	else {
		for(temp = 0; temp < array_sz; temp++) {
			if(timeout_in_sec >= g_timeout_interv[temp].timeout)
				continue;
			else
				return g_timeout_interv[temp - 1].interv;
		}
		pr_info("%s, line %d\n", __func__, __LINE__);
		return g_timeout_interv[array_sz - 1].interv; /* the largest one */
	}
}

static int interv_to_timeout(int interv)
{
	int 	temp;
	int 	array_sz;

	array_sz = ARRAY_SIZE(g_timeout_interv);
	if(interv > g_timeout_interv[array_sz - 1].interv)
		return g_timeout_interv[array_sz - 1].timeout;
	else if(array_sz < g_timeout_interv[0].interv)
		return g_timeout_interv[0].timeout;
	else {
		for(temp = 0; temp < array_sz; temp++) {
			if(interv >= g_timeout_interv[temp].interv)
				continue;
			else
				return g_timeout_interv[temp - 1].timeout;
		}
		pr_info("%s, line %d\n", __func__, __LINE__);
		return g_timeout_interv[array_sz - 1].timeout; /* the largest one */
	}
}


static int wdt_config(enum sunxi_watchdog_func func)
{
#ifdef WATCHDOG_DBG
	if(NULL == wdt_reg) {
		pr_err("%s err, line %d\n", __func__, __LINE__);
		return -1;
	}
#endif

	writel((u32)func, &wdt_reg->config);
	return 0;
}

static int wdt_restart(void)
{
#ifdef WATCHDOG_DBG
	if(NULL == wdt_reg) {
		pr_err("%s err, line %d\n", __func__, __LINE__);
		return -1;
	}
#endif

	pr_debug("%s, write reg 0x%08x\n", __func__, (u32)&wdt_reg->ctrl);
	writel((0xA57 << 1) | (1 << 0), &wdt_reg->ctrl);
	return 0;
}

static int wdt_set_tmout(int timeout_in_sec)
{
	int 	temp = 0, temp2 = 0;
	int 	interv = 0;

#ifdef WATCHDOG_DBG
	if(NULL == wdt_reg) {
		pr_err("%s err, line %d\n", __func__, __LINE__);
		return -1;
	}
#endif

	interv = timeout_to_interv(timeout_in_sec);
	if(interv < 0)
		return interv;
	temp = readl(&wdt_reg->mode);
	temp &= (~(u32)(0b1111 << 4));
	temp |= (interv << 4);
	writel(temp, &wdt_reg->mode);

	temp2 = interv_to_timeout(interv);
	pr_info("%s, write 0x%08x to mode reg 0x%08x, actual timeout %d sec\n",
		__func__, temp, (u32)&wdt_reg->mode, temp2);
	return interv;
}

static int wdt_enable(bool ben)
{
	int 	temp = 0;

#ifdef WATCHDOG_DBG
	if(NULL == wdt_reg) {
		pr_err("%s err, line %d\n", __func__, __LINE__);
		return -1;
	}
#endif

	temp = readl(&wdt_reg->mode);
	if(true == ben)
		temp |= 0x01;
	else
		temp &= 0xFE;
	writel(temp, &wdt_reg->mode);
	pr_info("%s, write reg 0x%08x val 0x%08x\n", __func__, (u32)&wdt_reg->mode, temp);
	return 0;
}

static int watchdog_start(void)
{
	int temp = -1;

	temp = wdt_set_tmout(g_timeout);
	if(temp < 0)
		return temp;

	g_timeout = temp;
	return wdt_enable(true);
}

static int watchdog_stop(void)
{
	return wdt_enable(false);
}

static int watchdog_kick(void)
{
	return wdt_restart();
}

static int watchdog_set_timeout(int timeout)
{
	int temp = -1;

	temp = wdt_set_tmout(timeout);
	if(temp < 0)
		return temp;
	g_timeout = temp;
	return 0;
}

static int watchdog_probe_init(void)
{
	int 	temp = -1;

	/* disable watchdog */
	wdt_enable(false);

	/* disable irq */
	wdt_irq_en(false);

	/* clear irq pending */
	wdt_clr_irq_pend();

	/* watchdog function initial */
	wdt_config(WDT_FUNC);

	/* set max timeout */
	temp = wdt_set_tmout(g_timeout);
	if(temp < 0)
		return temp;

	g_timeout = temp;
	return 0;
}

static int sunxi_wdt_open(struct inode *inode, struct file *file)
{
	int err;

	/* /dev/watchdog can only be opened once */
	if(xchg(&is_active, true))
		return -EBUSY;

	err = watchdog_start();
	return err ?: nonseekable_open(inode, file);
}

static int sunxi_wdt_release(struct inode *inode, struct file *file)
{
	if(expect_release)
		watchdog_stop();
	else {
		pr_info("%s: unexpected close, not stopping watchdog!\n", __func__);
		watchdog_kick(); 
	}

	is_active = false;
	expect_release = false;
	return 0;
}

static ssize_t sunxi_wdt_write(struct file *file, const char __user *data,
			size_t len, loff_t *ppos)
{
	/* See if we got the magic character 'V' and reload the timer */
	if(len) {
		if(!g_nowayout) {
			size_t i;

			/* in case it was set long ago */
			expect_release = false;

			/* scan to see whether or not we got the magic character */
			for(i = 0; i != len; i++) {
				char c;
				if(get_user(c, data + i))
					return -EFAULT;
				if(c == 'V')
					expect_release = true;
			}
		}

		/* someone wrote to us, we should reload the timer */
		watchdog_kick();
	}
	return len;
}

static long sunxi_wdt_ioctl(struct file *file, unsigned int cmd,
			unsigned long arg)
{
	int new_options, retval = -EINVAL;
	int new_timeout;
	int __user *argp = (void __user *)arg;
	
	static const struct watchdog_info ident = {
		.options 		= WDIOF_SETTIMEOUT | WDIOF_MAGICCLOSE | WDIOF_KEEPALIVEPING,
		.firmware_version 	= 0,
		.identity 		= DRV_NAME,
	};

	switch (cmd) {
		
	case WDIOC_GETSUPPORT: 
		return copy_to_user(argp, &ident, sizeof(ident)) ? -EFAULT : 0;

	case WDIOC_GETSTATUS:
		
	case WDIOC_GETBOOTSTATUS:
		return put_user(0, argp);

	case WDIOC_SETOPTIONS:
		if(get_user(new_options, argp))
			return -EFAULT;
		if(new_options & WDIOS_DISABLECARD)
			retval = watchdog_stop();
		if(new_options & WDIOS_ENABLECARD)
			retval = watchdog_start();
		return retval;

	case WDIOC_KEEPALIVE:  
		watchdog_kick();
		return 0;

	case WDIOC_SETTIMEOUT:
		if(get_user(new_timeout, argp))
			return -EFAULT;
		if(!new_timeout || new_timeout > MAX_TIMEOUT) {
			// pr_err("%s err, line %d\n", __func__, __LINE__);
			return -EINVAL;
		}
		watchdog_set_timeout(new_timeout);

		
	case WDIOC_GETTIMEOUT:
		return put_user(g_timeout, argp);
	}

	return -ENOTTY;
}
static const struct file_operations sunxi_wdt_fops = {
	.owner =		THIS_MODULE,
	.llseek =		no_llseek,
	.write =		sunxi_wdt_write,
	.unlocked_ioctl =	sunxi_wdt_ioctl,
	.open =			sunxi_wdt_open,
	.release =		sunxi_wdt_release,
};

static struct miscdevice sunxi_wdt_miscdev = {
	.minor =	WATCHDOG_MINOR,
	.name =		"watchdog",
	.fops =		&sunxi_wdt_fops,
};

static int  sunxi_wdt_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *res;

	if(!g_timeout || g_timeout > MAX_TIMEOUT) {
		g_timeout = MAX_TIMEOUT;
		pr_info("%s: timeout value invalid, using %d\n", __func__, g_timeout);
	}

	/*
	 * As this driver only covers the global watchdog case, reject
	 * any attempts to register per-CPU watchdogs.
	 */
	if(pdev->id != -1)
		return -EINVAL;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(unlikely(!res)) {
		ret = -EINVAL;
		goto err_get_resource;
	}

	if(!devm_request_mem_region(&pdev->dev, res->start,
				resource_size(res), DRV_NAME)) {
		ret = -EBUSY;
		goto err_request_mem_region;
	}

	wdt_reg = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if(!wdt_reg) {
		ret = -ENXIO;
		goto err_ioremap;
	}
	pr_info("%s: devm_ioremap return wdt_reg 0x%08x, res->start 0x%08x, res->end 0x%08x",
		__func__, (u32)wdt_reg, (u32)res->start, (u32)res->end);

	ret = misc_register(&sunxi_wdt_miscdev);
	if(ret) {
		pr_err("%s: cannot register miscdev on minor=%d (%d)\n",
			__func__, WATCHDOG_MINOR, ret);
		goto err_misc_register;
	}

	pr_info("%s: initialized (g_timeout=%ds, g_nowayout=%d)\n",
		__func__, g_timeout, g_nowayout);

	//watchdog_kick(); /* give userspace a bit more time to settle if watchdog already running */
	watchdog_probe_init(); 
	return ret;

err_misc_register:
	devm_iounmap(&pdev->dev, wdt_reg);
err_ioremap:
	devm_release_mem_region(&pdev->dev, res->start, resource_size(res));
err_request_mem_region:
err_get_resource:
	return ret;
}

static int  sunxi_wdt_remove(struct platform_device *pdev)
{
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	/* Stop the timer before we leave */
	watchdog_stop();

	misc_deregister(&sunxi_wdt_miscdev); 
	devm_iounmap(&pdev->dev, wdt_reg);   
	devm_release_mem_region(&pdev->dev, res->start, resource_size(res));

	return 0;
}

static void sunxi_wdt_shutdown(struct platform_device *pdev)
{
	watchdog_stop();
}

static int sunxi_wdt_suspend(struct platform_device *dev, pm_message_t state)
{
	if(is_active)
		return watchdog_stop();
	else
		return 0;
}

static int sunxi_wdt_resume(struct platform_device *dev)
{
	if(is_active)
		return watchdog_start();
	else
		return 0;
}

static struct platform_driver sunxi_wdt_driver = {
	.probe          = sunxi_wdt_probe,
	.remove         = sunxi_wdt_remove,
	.shutdown       = sunxi_wdt_shutdown,
	.suspend        = sunxi_wdt_suspend,
	.resume         = sunxi_wdt_resume,
	.driver         = {
		.owner  = THIS_MODULE,
		.name   = DRV_NAME,
	},
};

static int __init sunxi_wdt_init_module(void)
{
	int err;

	pr_info("%s: sunxi WatchDog Timer Driver v%s\n", __func__, DRV_VERSION);

	err = platform_driver_register(&sunxi_wdt_driver);
	if(err)
		goto err_driver_register;

	platform_device = platform_device_register_simple(DRV_NAME, -1, sunxi_wdt_res, ARRAY_SIZE(sunxi_wdt_res));
	if(IS_ERR(platform_device)) {
		err = PTR_ERR(platform_device);
		goto err_platform_device;
	}

	return err;

err_platform_device:
	platform_driver_unregister(&sunxi_wdt_driver);
err_driver_register:
	return err;
}

static void __exit sunxi_wdt_cleanup_module(void)
{
	platform_device_unregister(platform_device);
	platform_driver_unregister(&sunxi_wdt_driver);
	pr_info("%s: module unloaded\n", __func__);
}

module_init(sunxi_wdt_init_module);
module_exit(sunxi_wdt_cleanup_module);

MODULE_AUTHOR("huangshr <huangshr@allwinnertech.net>");
MODULE_DESCRIPTION("sunxi Watchdog Timer Driver");
MODULE_VERSION(DRV_VERSION);
MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);

