/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef _DWC3_SUNXI_H
#define _DWC3_SUNXI_H

#include <linux/notifier.h>

#include "core.h"
#include "io.h"
#include "sunxi-gpio.h"
#include "sunxi-inno.h"

#define DRIVER_NAME "sunxi-plat-dwc3"
#define DRIVER_VERSION "v1.0.23 2024-11-25 14:00"
#define DRIVER_INFOMATION "DesignWare USB3 Allwinner Glue Layer Driver(" DRIVER_VERSION ")"

/* Link Registers */
#define DWC3_LLUCTL		0xd024

/* Bit fields */

/* TX TS1 COUNT Register */

/* Force Gen1 speed on Gen2 link */
#define DWC3_LLUCTL_FORCE_GEN1		BIT(10)

/* Link uses data block sync header for Gen2 polarity detection */
#define DWC3_LLUCTL_INV_SYNC_HDR	BIT(30)

extern struct atomic_notifier_head usb_power_notifier_list;
extern struct atomic_notifier_head usb_pm_notifier_list;

/* Refer to the io. h header file */
static inline u32 dwc3_sunxi_readl(void __iomem *base, u32 offset)
{
	u32 value;

	/*
	 * We requested the mem region starting from the Globals address
	 * space, see dwc3_probe in core.c.
	 * However, the offsets are given starting from xHCI address space.
	 */
	value = readl(base + offset - DWC3_GLOBALS_REGS_START);

	return value;
}

static inline void dwc3_sunxi_writel(void __iomem *base, u32 offset, u32 value)
{
	/*
	 * We requested the mem region starting from the Globals address
	 * space, see dwc3_probe in core.c.
	 * However, the offsets are given starting from xHCI address space.
	 */
	writel(value, base + offset - DWC3_GLOBALS_REGS_START);
}

#endif	/* _DWC3_SUNXI_H */
