/*
 * xhci-plat.c - xHCI host controller driver platform Bus Glue.
 *
 * Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com
 * Author: Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * A lot of code borrowed from the Linux xHCI driver.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/slab.h>

#include <mach/platform.h>

#include "xhci.h"
extern atomic_t thread_suspend_flag;

int sunxi_xhci_disable(void);
int sunxi_xhci_enable(void);
static struct platform_device *sunxi_ehci;
static void xhci_plat_quirks(struct device *dev, struct xhci_hcd *xhci)
{
	/*
	 * As of now platform drivers don't provide MSI support so we ensure
	 * here that the generic code does not try to make a pci_dev from our
	 * dev struct in order to setup MSI
	 */
	xhci->quirks |= XHCI_PLAT;
}

/* called during probe() after chip reset completes */
static int xhci_plat_setup(struct usb_hcd *hcd)
{
	return xhci_gen_setup(hcd, xhci_plat_quirks);
}

static const struct hc_driver xhci_plat_xhci_driver = {
	.description =		"xhci-hcd",
	.product_desc =		"sunxi xhci hcd",
	.hcd_priv_size =	sizeof(struct xhci_hcd *),

	/*
	 * generic hardware linkage
	 */
	.irq =			xhci_irq,
	.flags =		HCD_MEMORY | HCD_USB3 | HCD_SHARED,

	/*
	 * basic lifecycle operations
	 */
	.reset =		xhci_plat_setup,
	.start =		xhci_run,
	.stop =			xhci_stop,
	.shutdown =		xhci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		xhci_urb_enqueue,
	.urb_dequeue =		xhci_urb_dequeue,
	.alloc_dev =		xhci_alloc_dev,
	.free_dev =		xhci_free_dev,
	.alloc_streams =	xhci_alloc_streams,
	.free_streams =		xhci_free_streams,
	.add_endpoint =		xhci_add_endpoint,
	.drop_endpoint =	xhci_drop_endpoint,
	.endpoint_reset =	xhci_endpoint_reset,
	.check_bandwidth =	xhci_check_bandwidth,
	.reset_bandwidth =	xhci_reset_bandwidth,
	.address_device =	xhci_address_device,
	.update_hub_device =	xhci_update_hub_device,
	.reset_device =		xhci_discover_or_reset_device,

	/*
	 * scheduling support
	 */
	.get_frame_number =	xhci_get_frame,

	/* Root hub support */
	.hub_control =		xhci_hub_control,
	.hub_status_data =	xhci_hub_status_data,
	.bus_suspend =		xhci_bus_suspend,
	.bus_resume =		xhci_bus_resume,
};

static int xhci_plat_probe(struct platform_device *pdev)
{
	const struct hc_driver	*driver;
	struct xhci_hcd		*xhci;
	struct resource         *res;
	struct usb_hcd		*hcd;
	int			ret;
	int			irq;

	if (usb_disabled())
		return -ENODEV;

	driver = &xhci_plat_xhci_driver;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return -ENODEV;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	hcd = usb_create_hcd(driver, &pdev->dev, dev_name(&pdev->dev));
	if (!hcd)
		return -ENOMEM;

	hcd->rsrc_start = res->start;
	hcd->rsrc_len = resource_size(res);

	hcd->regs = (void __iomem *)pdev->dev.platform_data;

	ret = usb_add_hcd(hcd, irq, IRQF_SHARED);
	if (ret)
		goto put_hcd;

	/* USB 2.0 roothub is stored in the platform_device now. */
	hcd = dev_get_drvdata(&pdev->dev);
	xhci = hcd_to_xhci(hcd);
	xhci->shared_hcd = usb_create_shared_hcd(driver, &pdev->dev,
			dev_name(&pdev->dev), hcd);
	if (!xhci->shared_hcd) {
		ret = -ENOMEM;
		goto dealloc_usb2_hcd;
	}

	/*
	 * Set the xHCI pointer before xhci_plat_setup() (aka hcd_driver.reset)
	 * is called by usb_add_hcd().
	 */
	*((struct xhci_hcd **) xhci->shared_hcd->hcd_priv) = xhci;

	ret = usb_add_hcd(xhci->shared_hcd, irq, IRQF_SHARED);
	if (ret)
		goto put_usb3_hcd;

	platform_set_drvdata(pdev, hcd);
	sunxi_ehci = pdev;

	return 0;

put_usb3_hcd:
	usb_put_hcd(xhci->shared_hcd);

dealloc_usb2_hcd:
	usb_remove_hcd(hcd);

put_hcd:
	usb_put_hcd(hcd);

	return ret;
}

static int xhci_plat_remove(struct platform_device *dev)
{
	struct usb_hcd	*hcd = NULL;
	struct xhci_hcd	*xhci = NULL;
	struct usb_device *hdev = NULL;
	struct usb_device *rhdev = NULL;
	int cnt = 10;

	hcd = platform_get_drvdata(dev);
	if(hcd == NULL){
		printk("%s,hcd is null\n", __func__);
		return 0;
	}

	xhci = hcd_to_xhci(hcd);
	if(xhci == NULL){
		printk("%s,xhci is null\n", __func__);
		return 0;
	}

	rhdev = hcd->self.root_hub;
	if(rhdev == NULL){
		printk("%s, hcd->self.root_hub is null\n", __func__);
		return 0;
	}

	hdev = rhdev->children[rhdev->descriptor.bNumConfigurations -1];
	while((hdev != NULL) && cnt){
		dev_info(&hdev->dev, "device number %d is exist when %s and need wait until disconnect, hubportNum:%d\n",
			hdev->devnum, __func__, rhdev->descriptor.bNumConfigurations);
		msleep(1000);
		hdev = rhdev->children[rhdev->descriptor.bNumConfigurations -1];

		cnt--;
	}

	usb_remove_hcd(xhci->shared_hcd);
	usb_put_hcd(xhci->shared_hcd);

	usb_remove_hcd(hcd);
 	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
	kfree(xhci);

	platform_set_drvdata(dev, NULL);

	return 0;
}

#ifdef CONFIG_PM
static int xhci_plat_suspend(struct device *dev)
{
	struct usb_hcd	*hcd = NULL;
	struct xhci_hcd	*xhci = NULL;

	atomic_set(&thread_suspend_flag, 1);

	hcd = dev_get_drvdata(dev);
	if(hcd == NULL){
		printk("xhci hcd is null and no need to suspend\n");
		return 0;
	}

	xhci = hcd_to_xhci(hcd);
	if(xhci == NULL){
		printk("xhci is null and no need to suspend\n");
		return 0;
	}

	return xhci_suspend(xhci);
}

static int xhci_plat_resume(struct device *dev)
{
	struct usb_hcd	*hcd = NULL;
	struct xhci_hcd	*xhci = NULL;
	int ret = 0;

	hcd = dev_get_drvdata(dev);
	if(hcd == NULL){
		printk("xhci hcd is null and no need to resume\n");
		return 0;
	}

	xhci = hcd_to_xhci(hcd);
	if(xhci == NULL){
		printk("xhci is null and no need to resume\n");
		return 0;
	}

	ret = xhci_resume(xhci, 0);

	atomic_set(&thread_suspend_flag, 0);

	return ret;
}

static const struct dev_pm_ops xhci_plat_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(xhci_plat_suspend, xhci_plat_resume)
};
#define DEV_PM_OPS	(&xhci_plat_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif

static struct platform_driver usb_xhci_driver = {
	.probe	= xhci_plat_probe,
	.remove	= xhci_plat_remove,
	.driver	= {
		.name = "xhci-hcd",
		.pm = DEV_PM_OPS,
	},
};
MODULE_ALIAS("platform:xhci-hcd");

int xhci_register_plat(void)
{
	return platform_driver_register(&usb_xhci_driver);
}

void xhci_unregister_plat(void)
{
	platform_driver_unregister(&usb_xhci_driver);
}

/*
*******************************************************************************
*                     sunxi_xhci_enable
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
int sunxi_xhci_enable(void)
{
	struct platform_device *pdev = sunxi_ehci;

	if(pdev == NULL){
		printk("%s, pdev is NULL\n", __func__);
		return 0;
	}

	xhci_plat_probe(pdev);

 	return 0;
}
EXPORT_SYMBOL(sunxi_xhci_enable);

/*
*******************************************************************************
*                     sunxi_xhci_disable
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
int sunxi_xhci_disable(void)
{
	struct platform_device *pdev = sunxi_ehci;

	if(pdev == NULL){
		printk("%s, pdev is NULL\n", __func__);
		return 0;
	}

	xhci_plat_remove(pdev);

	return 0;
}
EXPORT_SYMBOL(sunxi_xhci_disable);
