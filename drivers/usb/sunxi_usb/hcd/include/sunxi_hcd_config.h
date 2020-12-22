/*
 * drivers/usb/sunxi_usb/hcd/include/sunxi_hcd_config.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2010-12-20, create this file
 *
 * usb host contoller driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef  __SUNXI_HCD_CONFIG_H__
#define  __SUNXI_HCD_CONFIG_H__

#include <linux/slab.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/errno.h>

#include  "../../include/sunxi_usb_config.h"
#include  "sunxi_hcd_debug.h"

//#define        XUSB_DEBUG    /* debug switch */

/* xusb hcd debug print */
#if	0
#define DMSG_DBG_HCD     			DMSG_PRINT
#else
#define DMSG_DBG_HCD(...)
#endif

#endif   //__SUNXI_HCD_CONFIG_H__

