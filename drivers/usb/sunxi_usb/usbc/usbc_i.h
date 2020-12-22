/*
 * drivers/usb/sunxi_usb/usbc/usbc_i.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * daniel, 2009.09.15
 *
 * usb common ops.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef  __USBC_I_H__
#define  __USBC_I_H__

#include "../include/sunxi_usb_config.h"

#define  USBC_MAX_OPEN_NUM    8

/* record USB common info */
typedef struct __fifo_info{
	__u32 port0_fifo_addr;
	__u32 port0_fifo_size;

	__u32 port1_fifo_addr;
	__u32 port1_fifo_size;

	__u32 port2_fifo_addr;
	__u32 port2_fifo_size;
}__fifo_info_t;

/* record current USB port's all hardware info */
typedef struct __usbc_otg{
	__u32 port_num;
	__u32 base_addr;        /* usb base address 		*/

	__u32 used;             /* is used or not   		*/
	__u32 no;               /* index in manager table	*/
}__usbc_otg_t;

#endif   //__USBC_I_H__

