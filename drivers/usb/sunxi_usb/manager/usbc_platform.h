/*
 * drivers/usb/sunxi_usb/manager/usbc_platform.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2011-4-14, create this file
 *
 * usb contoller device info.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef  __USBC_PLATFORM_H__
#define  __USBC_PLATFORM_H__

__s32 usbc0_platform_device_init(struct usb_port_info *port_info);
__s32 usbc0_platform_device_exit(struct usb_port_info *port_info);

#endif   //__USBC_PLATFORM_H__

