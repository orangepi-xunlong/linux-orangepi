/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
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

#ifndef __USBC_I_H__
#define __USBC_I_H__

#include "../include/sunxi_usb_config.h"

#define  USBC_MAX_OPEN_NUM    1

void __iomem *get_otgc_vbase(void);

/* record USB common info */
typedef struct __fifo_info {
	void __iomem *port0_fifo_addr;
	__u32 port0_fifo_size;

	void __iomem *port1_fifo_addr;
	__u32 port1_fifo_size;

	void __iomem *port2_fifo_addr;
	__u32 port2_fifo_size;
} __fifo_info_t;

/* record current USB port's all hardware info */
typedef struct __usbc_otg {
	__u32 port_num;
	void __iomem *base_addr;	/* usb base address */

	__u32 used;			/* is used or not */
	__u32 no;			/* index in manager table */
} __usbc_otg_t;


/* PHYS EFUSE offest */
#define EFUSE_OFFSET			0x18		/* esuse offset */
#define SUNXI_USB_PHY_EFUSE_ADJUST	0x10000		/* bit16 */
#define SUNXI_USB_PHY_EFUSE_MODE	0x20000		/* bit17 */
#define SUNXI_USB_PHY_EFUSE_RES		0x3C0000	/* bit18-21 */
#define SUNXI_USB_PHY_EFUSE_COM		0x1C00000	/* bit22-24 */
#define SUNXI_USB_PHY_EFUSE_USB0TX	0x1C00000	/* bit22-24 */
#define SUNXI_USB_PHY_EFUSE_USB1TX	0xE000000	/* bit25-27 */

/* PHY RANGE: bit field */
#define PHY_RANGE_MODE_MASK			0x1000		/* bit12, mod_type */
#define PHY_RANGE_COMM_MASK			0xE00		/* bit11:9, common_data */
#if IS_ENABLED(CONFIG_ARCH_SUN8IW21) || IS_ENABLED(CONFIG_ARCH_SUN55IW6) \
	|| IS_ENABLED(CONFIG_ARCH_SUN300IW1) || IS_ENABLED(CONFIG_ARCH_SUN251IW1)
#define PHY_RANGE_TRAN_MASK			0x3C0		/* bit9:6, trancevie_data */
#else
#define PHY_RANGE_TRAN_MASK			0x1C0		/* bit8:6, trancevie_data */
#endif
#define PHY_RANGE_RISE_MASK			0xc00		/* bit11:10, rise_data */
#define PHY_RANGE_PREE_MASK			0x30		/* bit5:4, preemphasis_data */
#define PHY_RANGE_RESI_MASK			0xF		/* bit3:0, resistance_data */

/* SYSCFG Base Registers */
#define SUNXI_SYS_CFG_BASE		0x03000000
/* Resister Calibration Control Register */
#define RESCAL_CTRL_REG		0x0160
#define   USBPHY2_RES200_SEL		BIT(6) /* Note: AW1903 Not use */
#define   USBPHY1_RES200_SEL		BIT(5)
#define   USBPHY0_RES200_SEL		BIT(4)
#define   PHY_o_RES200_SEL(n)		(BIT(4) << n)
#define   RESCAL_MODE			BIT(2)
#define   CAL_ANA_EN			BIT(1)
#define   CAL_EN			BIT(0)
/* Resister 200ohms Manual Control Register */
#define RES200_CTRL_REG		0x0164
#define   USBPHY2_RES200_CTRL		GENMASK(21, 16)
#define   USBPHY1_RES200_CTRL		GENMASK(13, 8)
#define   USBPHY0_RES200_CTRL		GENMASK(5, 0)
#define   PHY_o_RES200_CTRL(n)		(GENMASK(5, 0) << (8 * n))
#define   PHY_o_RES200_CTRL_DEFAULT(n)		(0x33 << (8 * n))

/* Resister RES0 ohms Manual Control Register */
#define RES0_CTRL_REG		0x0164
#define   USBPHY1_RES200_TRIM		GENMASK(15, 8)
#define   USBPHY0_RES200_TRIM		GENMASK(7, 0)
#define   PHY_o_RES200_TRIM(n)		(GENMASK(7, 0) << (8 * n))
#define   PHY_o_RES200_TRIM_DEFAULT(n)		(0xC8 << (8 * n))

#define syscfg_reg(offset)		(SUNXI_SYS_CFG_BASE + (offset))

#endif /* __USBC_I_H__ */
