/*
 * Entry code of XRadio drivers
 *
 * Copyright (c) 2013
 * Xradio Technology Co., Ltd. <www.xradiotech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>

MODULE_AUTHOR("XRadioTech");
MODULE_DESCRIPTION("XRadioTech WLAN driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("xradio_wlan");

/* external interfaces */
extern int  __init ieee80211_init(void);
extern void ieee80211_exit(void);
extern int __init xradio_core_entry(void);
extern void xradio_core_exit(void);
extern int  xradio_core_init(void);
extern void xradio_core_deinit(void);

#ifdef CONFIG_XRADIO_ETF
void xradio_etf_to_wlan(u32 change);
#endif

int etf_enable;
module_param(etf_enable, int, 0644);

/* Init Module function -> Called by insmod */
static int __init xradio_init(void)
{
	int ret = 0;
	printk(KERN_ERR "======== XRADIO WIFI OPEN ========\n");
	ret = ieee80211_init();
	if (ret) {
		printk(KERN_ERR "ieee80211_init failed(%d)!\n", ret);
		return ret;
	}
	ret = xradio_core_entry();
	if (ret) {
		printk(KERN_ERR "xradio_core_entry failed(%d)!\n", ret);
		goto err1;
	}

	if (!etf_enable) {
		ret = xradio_core_init();  /* wlan driver init */
		if (ret) {
			printk(KERN_ERR "xradio_core_init failed(%d)!\n", ret);
			goto err2;
		}
	} else {
		xradio_etf_to_wlan(0);
	}
	return ret;

err2:
	xradio_core_exit();
err1:
	ieee80211_exit();
	return ret;
}

/* Called at Driver Unloading */
static void __exit xradio_exit(void)
{
	if (!etf_enable)
		xradio_core_deinit();
	xradio_core_exit();
	ieee80211_exit();
	printk(KERN_ERR "======== XRADIO WIFI CLOSE ========\n");
}

module_init(xradio_init);
module_exit(xradio_exit);
