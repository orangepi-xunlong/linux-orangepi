/*
 * drivers/usb/sunxi_usb/hcd/include/sunxi_hcd_virt_hub.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2010-12-20, create this file
 *
 * usb host contoller driver. virtual hub.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef  __SUNXI_HCD_VIRT_HUB_H__
#define  __SUNXI_HCD_VIRT_HUB_H__

void sunxi_hcd_root_disconnect(struct sunxi_hcd *sunxi_hcd);
int sunxi_hcd_hub_status_data(struct usb_hcd *hcd, char *buf);
int sunxi_hcd_hub_control(struct usb_hcd *hcd,
					u16 typeReq,
					u16 wValue,
					u16 wIndex,
					char *buf,
					u16 wLength);

void sunxi_hcd_port_suspend_ex(struct sunxi_hcd *sunxi_hcd);
void sunxi_hcd_port_resume_ex(struct sunxi_hcd *sunxi_hcd);
void sunxi_hcd_port_reset_ex(struct sunxi_hcd *sunxi_hcd);

#endif   //__SUNXI_HCD_VIRT_HUB_H__

