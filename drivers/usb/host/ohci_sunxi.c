/*
 * drivers/usb/host/ohci_sunxi.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yangnaitian, 2011-5-24, create this file
 * javen, 2011-6-26, add suspend and resume
 * javen, 2011-7-18, move clock and power operations out from driver
 *
 * OHCI Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/platform_device.h>
#include <linux/signal.h>

#include <linux/time.h>
#include <linux/timer.h>

#include <mach/sys_config.h>
#include <linux/clk.h>

#include <linux/arisc/arisc.h>
#include <linux/power/aw_pm.h>
#include <linux/power/scenelock.h>

#include "sunxi_hci.h"


//#define  SUNXI_USB_OHCI_DEBUG

#define   SUNXI_OHCI_NAME	"sunxi-ohci"
static const char ohci_name[] = SUNXI_OHCI_NAME;

static struct sunxi_hci_hcd *g_sunxi_ohci[4];
static u32 ohci_first_probe[4] = {1, 1, 1, 1};
static u32 ohci_enable[4] = {1, 1, 1, 1};

static struct scene_lock  ohci_standby_lock[4];

#ifdef CONFIG_USB_HCD_ENHANCE
extern atomic_t hci1_thread_scan_flag;
extern atomic_t hci3_thread_scan_flag;
#endif

extern int usb_disabled(void);
int sunxi_usb_disable_ohci(__u32 usbc_no);
int sunxi_usb_enable_ohci(__u32 usbc_no);

static ssize_t ohci_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_hci_hcd *sunxi_ohci = NULL;

	if(dev == NULL){
		DMSG_PANIC("ERR: Argment is invalid\n");
		return 0;
	}

	sunxi_ohci = dev->platform_data;
	if(sunxi_ohci == NULL){
		DMSG_PANIC("ERR: sw_ohci is null\n");
		return 0;
	}

	return sprintf(buf, "ohci:%d,probe:%u, enable:%d\n", sunxi_ohci->usbc_no, sunxi_ohci->probe, ohci_enable[sunxi_ohci->usbc_no]);
}

static ssize_t ohci_enable_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sunxi_hci_hcd *sunxi_ohci = NULL;
	int value = 0;

	if(dev == NULL){
		DMSG_PANIC("ERR: Argment is invalid\n");
		return 0;
	}

	sunxi_ohci = dev->platform_data;
	if(sunxi_ohci == NULL){
		DMSG_PANIC("ERR: sw_ohci is null\n");
		return 0;
	}

	sunxi_ohci->host_init_state = 0;
	ohci_first_probe[sunxi_ohci->usbc_no] = 0;

	sscanf(buf, "%d", &value);
	if(value == 1){
		ohci_enable[sunxi_ohci->usbc_no] = 0;
		sunxi_usb_enable_ohci(sunxi_ohci->usbc_no);
	}else if(value == 0){
		ohci_enable[sunxi_ohci->usbc_no] = 1;
		sunxi_usb_disable_ohci(sunxi_ohci->usbc_no);
		ohci_enable[sunxi_ohci->usbc_no] = 0;
	}else{
		DMSG_INFO("unkown value (%d)\n", value);
	}

	return count;
}

static DEVICE_ATTR(ohci_enable, 0644, ohci_enable_show, ohci_enable_store);


static int open_ohci_clock(struct sunxi_hci_hcd *sunxi_ohci)
{
	return sunxi_ohci->open_clock(sunxi_ohci, 1);
}

static int close_ohci_clock(struct sunxi_hci_hcd *sunxi_ohci)
{
	return sunxi_ohci->close_clock(sunxi_ohci, 1);
}

static int sunxi_get_io_resource(struct platform_device *pdev, struct sunxi_hci_hcd *sunxi_ohci)
{
	return 0;
}

static int sunxi_release_io_resource(struct platform_device *pdev, struct sunxi_hci_hcd *sunxi_ohci)
{
	return 0;
}

static void sunxi_start_ohci(struct sunxi_hci_hcd *sunxi_ohci)
{

	sunxi_ohci->set_usbc_regulator(sunxi_ohci, 1);

#if defined (CONFIG_ARCH_SUN8IW6) || defined (CONFIG_ARCH_SUN9IW1)
	sunxi_ohci->hci_phy_ctrl(sunxi_ohci, 1);
#endif
	open_ohci_clock(sunxi_ohci);

	sunxi_ohci->port_configure(sunxi_ohci, 1);
	sunxi_ohci->usb_passby(sunxi_ohci, 1);
	sunxi_ohci->set_power(sunxi_ohci, 1);

	return;
}

static void sunxi_stop_ohci(struct sunxi_hci_hcd *sunxi_ohci)
{
	sunxi_ohci->set_power(sunxi_ohci, 0);
	sunxi_ohci->usb_passby(sunxi_ohci, 0);
	sunxi_ohci->port_configure(sunxi_ohci, 0);

	close_ohci_clock(sunxi_ohci);

#if defined (CONFIG_ARCH_SUN8IW6) || defined (CONFIG_ARCH_SUN9IW1)
	sunxi_ohci->hci_phy_ctrl(sunxi_ohci, 0);
#endif
	sunxi_ohci->set_usbc_regulator(sunxi_ohci, 0);

	return;
}

static int sunxi_ohci_start(struct usb_hcd *hcd)
{
	struct ohci_hcd	*ohci = hcd_to_ohci(hcd);
	int ret;

	if ((ret = ohci_init(ohci)) < 0)
		return ret;

	if ((ret = ohci_run(ohci)) < 0) {
		DMSG_PANIC("can't start %s", hcd->self.bus_name);
		ohci_stop(hcd);
		return ret;
	}

	return 0;
}

static const struct hc_driver sunxi_ohci_hc_driver ={
	.description	= hcd_name,
	.product_desc	= "SW USB2.0 'Open' Host Controller (OHCI) Driver",
	.hcd_priv_size	= sizeof(struct ohci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq			= ohci_irq,
	.flags			= HCD_USB11 | HCD_MEMORY,

	/*
	 * basic lifecycle operations
	 */
	.start			= sunxi_ohci_start,
	.stop			= ohci_stop,
	.shutdown		= ohci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue		= ohci_urb_enqueue,
	.urb_dequeue		= ohci_urb_dequeue,
	.endpoint_disable	= ohci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number	= ohci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data	= ohci_hub_status_data,
	.hub_control		= ohci_hub_control,

#ifdef	CONFIG_PM
	.bus_suspend		= ohci_bus_suspend,
	.bus_resume		= ohci_bus_resume,
#endif
	.start_port_reset	= ohci_start_port_reset,
};

static int sunxi_ohci_hcd_probe(struct platform_device *pdev)
{
	int ret;
	struct usb_hcd *hcd = NULL;
	struct sunxi_hci_hcd *sunxi_ohci = NULL;

	if(pdev == NULL){
		DMSG_PANIC("ERR: Argment is invaild\n");
		return -1;
	}

	/* if usb is disabled, can not probe */
	if (usb_disabled()){
		DMSG_PANIC("ERR: usb hcd is disabled\n");
		return -ENODEV;
	}

	sunxi_ohci = pdev->dev.platform_data;
	if(!sunxi_ohci){
		DMSG_PANIC("ERR: sunxi_ohci is null\n");
		ret = -ENOMEM;
		goto ERR1;
	}

	sunxi_ohci->pdev = pdev;
	g_sunxi_ohci[sunxi_ohci->usbc_no] = sunxi_ohci;

	DMSG_INFO("[%s%d]: probe, pdev->name: %s, pdev->id: %d, sunxi_ohci: 0x%p\n",
		ohci_name, sunxi_ohci->usbc_no, pdev->name, pdev->id, sunxi_ohci);

	/* get io resource */
	sunxi_get_io_resource(pdev, sunxi_ohci);

	sunxi_ohci->ohci_base = sunxi_ohci->usb_vbase + SUNXI_USB_OHCI_BASE_OFFSET;

	sunxi_ohci->ohci_reg_length = SUNXI_USB_OHCI_LEN;

	/*creat a usb_hcd for the ohci controller*/
	hcd = usb_create_hcd(&sunxi_ohci_hc_driver, &pdev->dev, ohci_name);
	if(!hcd){
		DMSG_PANIC("ERR: usb_ohci_create_hcd failed\n");
		ret = -ENOMEM;
		goto ERR2;
	}

	hcd->rsrc_start = (u32 __force)sunxi_ohci->ohci_base;
	hcd->rsrc_len 	= sunxi_ohci->ohci_reg_length;
	hcd->regs 	= sunxi_ohci->ohci_base;
	sunxi_ohci->hcd	= hcd;

	/* ochi start to work */
	sunxi_start_ohci(sunxi_ohci);

	ohci_hcd_init(hcd_to_ohci(hcd));

	ret = usb_add_hcd(hcd, sunxi_ohci->irq_no, IRQF_DISABLED | IRQF_SHARED);
	if(ret != 0){
		DMSG_PANIC("ERR: usb_add_hcd failed\n");
		ret = -ENOMEM;
		goto ERR3;
	}

	platform_set_drvdata(pdev, hcd);

#ifndef USB_SYNC_SUSPEND
	device_enable_async_suspend(&pdev->dev);
#endif

	sunxi_ohci->probe = 1;


	if(sunxi_ohci->not_suspend){
		scene_lock_init(&ohci_standby_lock[sunxi_ohci->usbc_no], SCENE_USB_STANDBY,  "ohci_standby");
	}

	/* Disable ohci, when driver probe */
	if(sunxi_ohci->host_init_state == 0){
		if(ohci_first_probe[sunxi_ohci->usbc_no]){
			sunxi_usb_disable_ohci(sunxi_ohci->usbc_no);
			ohci_first_probe[sunxi_ohci->usbc_no] = 0;
		}
	}

	if(ohci_enable[sunxi_ohci->usbc_no]){
		device_create_file(&pdev->dev, &dev_attr_ohci_enable);
		ohci_enable[sunxi_ohci->usbc_no] = 0;
	}

	return 0;

ERR3:
	usb_put_hcd(hcd);
	sunxi_stop_ohci(sunxi_ohci);

ERR2:
	sunxi_ohci->hcd = NULL;
#ifndef CONFIG_USB_HCD_ENHANCE
	g_sunxi_ohci[sunxi_ohci->usbc_no] = NULL;
#endif

ERR1:
	return ret;
}

static int sunxi_ohci_hcd_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = NULL;
	struct sunxi_hci_hcd *sunxi_ohci = NULL;

	if(pdev == NULL){
		DMSG_PANIC("ERR: Argment is invalid\n");
		return -1;
	}

	hcd = platform_get_drvdata(pdev);
	if(hcd == NULL){
		DMSG_PANIC("ERR: hcd is null\n");
		return -1;
	}

	sunxi_ohci = pdev->dev.platform_data;
	if(sunxi_ohci == NULL){
		DMSG_PANIC("ERR: sunxi_ohci is null\n");
		return -1;
	}

	DMSG_INFO("[%s%d]: remove, pdev->name: %s, pdev->id: %d, sunxi_ohci: 0x%p\n",
		ohci_name, sunxi_ohci->usbc_no, pdev->name, pdev->id, sunxi_ohci);

	if(ohci_enable[sunxi_ohci->usbc_no] == 0){
		device_remove_file(&pdev->dev, &dev_attr_ohci_enable);
	}

	if(sunxi_ohci->not_suspend){
		scene_lock_destroy(&ohci_standby_lock[sunxi_ohci->usbc_no]);
	}
	usb_remove_hcd(hcd);

	sunxi_stop_ohci(sunxi_ohci);
	sunxi_ohci->probe = 0;

	usb_put_hcd(hcd);

	sunxi_release_io_resource(pdev, sunxi_ohci);

	sunxi_ohci->hcd = NULL;
#if !defined (CONFIG_ARCH_SUN8IW8) && !defined (CONFIG_ARCH_SUN8IW7)  && !defined (CONFIG_USB_HCD_ENHANCE)
	if(sunxi_ohci->host_init_state){
		g_sunxi_ohci[sunxi_ohci->usbc_no] = NULL;
	}
#endif
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static void sunxi_ohci_hcd_shutdown(struct platform_device* pdev)
{
	struct sunxi_hci_hcd *sunxi_ohci = NULL;

	sunxi_ohci = pdev->dev.platform_data;
	if(sunxi_ohci == NULL){
		DMSG_PANIC("ERR: sunxi_ohci is null\n");
		return ;
	}

	if(sunxi_ohci->probe == 0){
		DMSG_PANIC("ERR: sunxi_ohci is disable, need not shutdown\n");
		return ;
	}

	DMSG_INFO("[%s]: ohci shutdown start\n", sunxi_ohci->hci_name);

#ifdef CONFIG_USB_HCD_ENHANCE
			if(sunxi_ohci->usbc_no == 1){
				atomic_set(&hci1_thread_scan_flag, 0);
			}

			if(sunxi_ohci->usbc_no == 3){
				atomic_set(&hci3_thread_scan_flag, 0);
			}
#endif
	if(sunxi_ohci->not_suspend){
		scene_lock_destroy(&ohci_standby_lock[sunxi_ohci->usbc_no]);
	}
	usb_hcd_platform_shutdown(pdev);
	sunxi_stop_ohci(sunxi_ohci);

	DMSG_INFO("[%s]: ohci shutdown end\n", sunxi_ohci->hci_name);

	return;
}

#ifdef CONFIG_PM

static int sunxi_ohci_hcd_suspend(struct device *dev)
{
	struct sunxi_hci_hcd *sunxi_ohci  = NULL;
	struct usb_hcd *hcd	= NULL;
	struct ohci_hcd	*ohci	= NULL;
	unsigned long flags	= 0;
	int val = 0;

	if(dev == NULL){
		DMSG_PANIC("ERR: Argment is invalid\n");
		return 0;
	}

	hcd = dev_get_drvdata(dev);
	if(hcd == NULL){
		DMSG_PANIC("ERR: hcd is null\n");
		return 0;
	}

	sunxi_ohci = dev->platform_data;
	if(sunxi_ohci == NULL){
		DMSG_PANIC("ERR: sunxi_ohci is null\n");
		return 0;
	}

	if(sunxi_ohci->probe == 0){
		DMSG_PANIC("ERR: sunxi_ohci is disable, can not suspend\n");
		return 0;
	}

	ohci = hcd_to_ohci(hcd);
	if(ohci == NULL){
		DMSG_PANIC("ERR: ohci is null\n");
		return 0;
	}

#ifdef CONFIG_USB_HCD_ENHANCE
	atomic_set(&hci1_thread_scan_flag, 0);
	atomic_set(&hci3_thread_scan_flag, 0);
#endif
	if(sunxi_ohci->not_suspend){
		DMSG_INFO("[%s]: not suspend\n", sunxi_ohci->hci_name);
		scene_lock(&ohci_standby_lock[sunxi_ohci->usbc_no]);
		enable_wakeup_src(CPUS_USBMOUSE_SRC, 0);

		val = ohci_readl(ohci, &ohci->regs->control);
		val |= OHCI_CTRL_RWE;
		ohci_writel(ohci, val,  &ohci->regs->control);

		val = ohci_readl(ohci, &ohci->regs->intrenable);
		val |= OHCI_INTR_RD;
		val |= OHCI_INTR_MIE;
		ohci_writel(ohci, val, &ohci->regs->intrenable);
#if defined (CONFIG_ARCH_SUN8IW6) || defined (CONFIG_ARCH_SUN9IW1)
		sunxi_ohci->hci_phy_ctrl(sunxi_ohci, 0);
#endif

	}else{
		DMSG_INFO("[%s]: sunxi_ohci_hcd_suspend\n", sunxi_ohci->hci_name);

		/* Root hub was already suspended. Disable irq emission and
		 * mark HW unaccessible, bail out if RH has been resumed. Use
		 * the spinlock to properly synchronize with possible pending
		 * RH suspend or resume activity.
		 *
		 * This is still racy as hcd->state is manipulated outside of
		 * any locks =P But that will be a different fix.
		 */
		spin_lock_irqsave(&ohci->lock, flags);

		ohci_writel(ohci, OHCI_INTR_MIE, &ohci->regs->intrdisable);
		(void)ohci_readl(ohci, &ohci->regs->intrdisable);

		clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

		spin_unlock_irqrestore(&ohci->lock, flags);

		sunxi_stop_ohci(sunxi_ohci);
	}

	return 0;
}

static int sunxi_ohci_hcd_resume(struct device *dev)
{
	struct sunxi_hci_hcd *sunxi_ohci = NULL;
	struct usb_hcd *hcd = NULL;

	if(dev == NULL){
		DMSG_PANIC("ERR: Argment is invalid\n");
		return 0;
	}

	hcd = dev_get_drvdata(dev);
	if(hcd == NULL){
		DMSG_PANIC("ERR: hcd is null\n");
		return 0;
	}

	sunxi_ohci = dev->platform_data;
	if(sunxi_ohci == NULL){
		DMSG_PANIC("ERR: sunxi_ohci is null\n");
		return 0;
	}

	if(sunxi_ohci->probe == 0){
		DMSG_PANIC("ERR: sunxi_ohci is disable, can not resume\n");
		return 0;
	}

	if(sunxi_ohci->not_suspend){
		DMSG_INFO("[%s]: controller not suspend, need not resume\n", sunxi_ohci->hci_name);
#if defined (CONFIG_ARCH_SUN8IW6) || defined (CONFIG_ARCH_SUN9IW1)
		sunxi_ohci->hci_phy_ctrl(sunxi_ohci, 1);
#endif
		scene_unlock(&ohci_standby_lock[sunxi_ohci->usbc_no]);
		disable_wakeup_src(CPUS_USBMOUSE_SRC, 0);

#ifdef CONFIG_USB_HCD_ENHANCE
		if(sunxi_ohci->usbc_no == 1){
			atomic_set(&hci1_thread_scan_flag, 1);
		}

		if(sunxi_ohci->usbc_no == 3){
			atomic_set(&hci3_thread_scan_flag, 1);
		}
#endif
	}else{
		DMSG_INFO("[%s]: sunxi_ohci_hcd_resume\n", sunxi_ohci->hci_name);

#ifndef CONFIG_ARCH_SUN9IW1
#ifdef  SUNXI_USB_FPGA
		fpga_config_use_hci((__u32 __force)sunxi_ohci->sram_vbase);
#endif
#endif
		sunxi_start_ohci(sunxi_ohci);

		set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
		ohci_finish_controller_resume(hcd);
	}

	return 0;
}

static const struct dev_pm_ops sunxi_ohci_pmops = {
	.suspend	= sunxi_ohci_hcd_suspend,
	.resume		= sunxi_ohci_hcd_resume,
};

#define SUNXI_OHCI_PMOPS  &sunxi_ohci_pmops

#else

#define SUNXI_OHCI_PMOPS NULL

#endif

static struct platform_driver sunxi_ohci_hcd_driver = {
	.probe		= sunxi_ohci_hcd_probe,
	.remove		= sunxi_ohci_hcd_remove,
	.shutdown	= sunxi_ohci_hcd_shutdown,
	.driver		= {
		.name	= ohci_name,
		.owner	= THIS_MODULE,
		.pm	= SUNXI_OHCI_PMOPS,
	},
};

int sunxi_usb_disable_ohci(__u32 usbc_no)
{
	struct sunxi_hci_hcd *sunxi_ohci = NULL;

	sunxi_ohci = g_sunxi_ohci[usbc_no];
	if(sunxi_ohci == NULL){
		DMSG_PANIC("ERR: sunxi_ohci is null\n");
		return -1;
	}

#if !defined (CONFIG_ARCH_SUN8IW8) && !defined (CONFIG_ARCH_SUN8IW7)  && !defined (CONFIG_USB_HCD_ENHANCE)
	if(sunxi_ohci->host_init_state){
		DMSG_PANIC("ERR: not support sunxi_usb_disable_ohci\n");
		return -1;
	}
#endif

	if(sunxi_ohci->probe == 0){
		DMSG_PANIC("ERR: sunxi_ohci is disable, can not disable again\n");
		return -1;
	}

	sunxi_ohci->probe = 0;

	DMSG_INFO("[%s]: sunxi_usb_disable_ohci\n", sunxi_ohci->hci_name);

	sunxi_ohci_hcd_remove(sunxi_ohci->pdev);

	return 0;
}
EXPORT_SYMBOL(sunxi_usb_disable_ohci);

int sunxi_usb_enable_ohci(__u32 usbc_no)
{
	struct sunxi_hci_hcd *sunxi_ohci = NULL;

#ifdef CONFIG_USB_HCD_ENHANCE
	if(usbc_no == 1){
		atomic_set(&hci1_thread_scan_flag, 0);
	}

	if(usbc_no == 3){
		atomic_set(&hci3_thread_scan_flag, 0);
	}
#endif

	sunxi_ohci = g_sunxi_ohci[usbc_no];
	if(sunxi_ohci == NULL){
		DMSG_PANIC("ERR: sunxi_ohci is null\n");
		return -1;
	}

#if !defined (CONFIG_ARCH_SUN8IW8) && !defined (CONFIG_ARCH_SUN8IW7)  && !defined (CONFIG_USB_HCD_ENHANCE)
	if(sunxi_ohci->host_init_state){
		DMSG_PANIC("ERR: not support sunxi_usb_enable_ohci\n");
		return -1;
	}
#endif

	if(sunxi_ohci->probe == 1){
		DMSG_PANIC("ERR: sunxi_ohci is already enable, can not enable again\n");
		return -1;
	}

	sunxi_ohci->probe = 1;

	DMSG_INFO("[%s]: sunxi_usb_enable_ohci\n", sunxi_ohci->hci_name);

	sunxi_ohci_hcd_probe(sunxi_ohci->pdev);

	return 0;
}
EXPORT_SYMBOL(sunxi_usb_enable_ohci);
#ifdef CONFIG_ARCH_SUN8IW8
int sunxi_set_vbus(__u32 usbc_no)
{
	struct sunxi_hci_hcd *sunxi_ohci = NULL;

	sunxi_ohci = g_sunxi_ohci[usbc_no];
	if(sunxi_ohci == NULL){
		DMSG_PANIC("ERR: sunxi_ohci is null\n");
		return -1;
	}

	schedule_work(&sunxi_ohci->usbc_work);
	return 0;
}
EXPORT_SYMBOL(sunxi_set_vbus);
#endif

