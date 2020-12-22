/*
 * drivers/usb/sunxi_usb/udc/usb3/io.h
 * (C) Copyright 2013-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangjx, 2014-3-14, create this file
 *
 * usb3.0 contoller io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */



#ifndef __DRIVERS_USB_SUNXI_IO_H
#define __DRIVERS_USB_SUNXI_IO_H

#include <linux/io.h>

static inline u32 sunxi_readl(void __iomem *base, u32 offset)
{
	return readl(base + offset);
}

static inline void sunxi_writel(void __iomem *base, u32 offset, u32 value)
{
	writel(value, base + offset);
}

#endif /* __DRIVERS_USB_SUNXI_IO_H */
