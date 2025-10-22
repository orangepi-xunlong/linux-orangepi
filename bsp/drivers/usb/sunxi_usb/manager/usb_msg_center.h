/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2011-4-14, create this file
 *
 * usb msg distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __USB_MSG_CENTER_H__
#define __USB_MSG_CENTER_H__

/* usb role mode */
typedef enum sw_usb_role {
	SW_USB_ROLE_NULL = 0,
	SW_USB_ROLE_HOST,
	SW_USB_ROLE_DEVICE,
} sw_usb_role_t;

typedef struct usb_msg {
	u8  app_drv_null;		/* not install any driver */
	u8  app_insmod_host;
	u8  app_rmmod_host;
	u8  app_insmod_device;
	u8  app_rmmod_device;

	u8  hw_insmod_host;
	u8  hw_rmmod_host;
	u8  hw_insmod_device;
	u8  hw_rmmod_device;
} usb_msg_t;

typedef struct usb_msg_center_info {
	struct usb_cfg *cfg;

	struct usb_msg msg;
	enum sw_usb_role role;

	u32 skip;			/* if skip, not enter msg process */
	struct gpio_desc	*bcten_gpiod; /* for some ICs, there is bcten, sunch as A537 */
	/* mainly to omit invalid msg */
} usb_msg_center_info_t;

extern int sunxi_usb_disable_ehci(__u32 usbc_no);
extern int sunxi_usb_enable_ehci(__u32 usbc_no);
extern int sunxi_usb_disable_ohci(__u32 usbc_no);
extern int sunxi_usb_enable_ohci(__u32 usbc_no);

void hw_insmod_usb_host(void);
void hw_rmmod_usb_host(void);
void hw_insmod_usb_device(void);
void hw_rmmod_usb_device(void);

enum sw_usb_role get_usb_role(void);
void set_usb_role_ex(enum sw_usb_role role);
void usb_msg_center(struct usb_cfg *cfg);

s32 usb_msg_center_init(struct device *dev);
s32 usb_msg_center_exit(void);

#endif /* __USB_MSG_CENTER_H__ */
