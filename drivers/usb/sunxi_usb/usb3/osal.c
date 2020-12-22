/*
 * drivers/usb/sunxi_usb/udc/usb3/osal.c
 * (C) Copyright 2013-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangjx, 2014-3-14, create this file
 *
 * usb3.0 contoller osal
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/delay.h>
#include "osal.h"

void osal_mdelay(unsigned long n)
{
	mdelay(n); 
}

void osal_udelay(unsigned long n)
{
	udelay(n); 
}
