/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2010-12-20, create this file
 *
 * usb debug head file.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __SUNXI_USB_DEBUG_H__
#define __SUNXI_USB_DEBUG_H__

#include <sunxi-log.h>

#if IS_ENABLED(CONFIG_USB_SUNXI_USB_DEBUG)
#define DMSG_ERR(format, args...)	sunxi_err(NULL, format, ##args)
#define DMSG_WARN(format, args...)	sunxi_warn(NULL, format, ##args)
#define DMSG_INFO(format, args...)	sunxi_info(NULL, format, ##args)
#define DMSG_DEBUG(format, args...)	sunxi_debug(NULL, format, ##args)
#else
#define DMSG_ERR(...)
#define DMSG_WARN(...)
#define DMSG_INFO(...)
#define DMSG_DEBUG(...)
#endif //CONFIG_USB_SUNXI_USB_DEBUG


#if 1
#define  DMSG_INFO_UDC(fmt, ...)	DMSG_DEBUG("[udc]: "fmt, ##__VA_ARGS__)
#else
#define DMSG_INFO_UDC(...)
#endif


/* dma debug print */
#ifdef SUNXI_DMA_DBG
#define DMSG_DBG_DMA			DMSG_WARN
#else
#define DMSG_DBG_DMA(...)
#endif

void print_usb_reg_by_ep(spinlock_t *lock,
				void __iomem *usbc_base,
				__s32 ep_index,
				char *str);
void print_all_usb_reg(spinlock_t *lock,
				void __iomem *usbc_base,
				__s32 ep_start,
				__u32 ep_end,
				char *str);

void clear_usb_reg(void __iomem *usb_base);

#endif /* __SUNXI_USB_DEBUG_H__ */
