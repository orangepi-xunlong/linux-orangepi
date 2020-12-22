/*
 * drivers/usb/sunxi_usb/hcd/include/sunxi_hcd_debug.h
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

#ifndef  __SUNXI_HCD_DEBUG_H__
#define  __SUNXI_HCD_DEBUG_H__

#include  "sunxi_hcd_core.h"

void print_sunxi_hcd_config(struct sunxi_hcd_config *config, char *str);
void print_sunxi_hcd_list(struct list_head *list_head, char *str);
void print_urb_list(struct usb_host_endpoint *hep, char *str);

#endif   //__SUNXI_HCD_DEBUG_H__

