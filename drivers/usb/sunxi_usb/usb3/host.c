/*
 * drivers/usb/sunxi_usb/udc/usb3/host.c
 * (C) Copyright 2013-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangjx, 2014-3-14, create this file
 *
 * usb3.0 contoller host
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/platform_device.h>

#include "core.h"

static struct resource generic_resources[] = {
	{
		.flags = IORESOURCE_IRQ,
	},
	{
		.flags = IORESOURCE_MEM,
	},
};

int sunxi_host_init(struct sunxi_otgc *otgc)
{
	struct platform_device	*xhci;
	int			ret;

	xhci = platform_device_alloc("xhci-hcd", -1);
	if (!xhci) {
		dev_err(otgc->dev, "couldn't allocate xHCI device\n");
		ret = -ENOMEM;
		goto err0;
	}

	dma_set_coherent_mask(&xhci->dev, otgc->dev->coherent_dma_mask);

	xhci->dev.parent	= otgc->dev;
	xhci->dev.dma_mask	= otgc->dev->dma_mask;
	xhci->dev.dma_parms	= otgc->dev->dma_parms;
	xhci->dev.platform_data = (void	* __force)otgc->regs;

	otgc->xhci = xhci;

	/* setup resources */
	generic_resources[0].start = otgc->irq;

	generic_resources[1].start = otgc->res->start;
	generic_resources[1].end = otgc->res->start + 0x7fff;

	ret = platform_device_add_resources(xhci, generic_resources,
			ARRAY_SIZE(generic_resources));
	if (ret) {
		dev_err(otgc->dev, "couldn't add resources to xHCI device\n");
		goto err1;
	}

	ret = platform_device_add(xhci);
	if (ret) {
		dev_err(otgc->dev, "failed to register xHCI device\n");
		goto err1;
	}

	return 0;

err1:
	platform_device_put(xhci);

err0:
	return ret;
}

void sunxi_host_exit(struct sunxi_otgc *otgc)
{
	if(otgc->xhci != NULL){
		platform_device_unregister(otgc->xhci);
	}
}
