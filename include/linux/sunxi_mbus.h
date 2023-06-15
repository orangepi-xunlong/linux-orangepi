/*
 * SUNXI MBUS support
 *
 * Copyright (C) 2015 AllWinnertech Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __LINUX_SUNXI_MBUS_H
#define __LINUX_SUNXI_MBUS_H

#include <linux/types.h>

/* Port ids */
/* See SDRAM Controller Spec: DRAMC access master list */
enum mbus_port {
#if IS_ENABLED(CONFIG_ARCH_SUN8IW20) || IS_ENABLED(CONFIG_ARCH_SUN20IW1)
	MBUS_PORT_CPU		= 0,
	MBUS_PORT_MAHB		= 2,
	MBUS_PORT_DMA		= 3,
	MBUS_PORT_VE		= 4,
	MBUS_PORT_CE		= 5,
	MBUS_PORT_TVD		= 10,
	MBUS_PORT_CSI		= 11,
	MBUS_PORT_G2D		= 13,
	MBUS_PORT_DI		= 14,
	MBUS_PORT_DE		= 16,
	MBUS_PORT_IOMMU		= 25,
	MBUS_PORT_DSP		= 37,
	MBUS_PORT_RISCV		= 39,
	MBUS_PORTS_MAX		= 40,
#elif IS_ENABLED(CONFIG_ARCH_SUN50IW9)
	MBUS_PORT_CPU           = 0,
	MBUS_PORT_GPU           = 1,
	MBUS_PORT_MAHB          = 2,
	MBUS_PORT_DMA           = 3,
	MBUS_PORT_VE0           = 4,
	MBUS_PORT_CE            = 5,
	MBUS_PORT_TSC0          = 6,
	MBUS_PORT_NDFC0         = 8,
	MBUS_PORT_CSI0          = 11,
	MBUS_PORT_DI0           = 14,
	MBUS_PORT_DE300         = 16,
	MBUS_PORT_G2D_ROT       = 21,
	MBUS_PORT_CSI1          = 22,
	MBUS_PORT_ISP1          = 23,
	MBUS_PORT_VE1           = 24,
	MBUS_PORT_IOMMU         = 25,
	MBUS_PORT_HDMI          = 39,
	MBUS_PORTS_MAX          = 40,

#else
	/* ... */
#endif
};

struct device_node;

#ifdef CONFIG_SUNXI_MBUS
extern int mbus_port_setbwlen(enum mbus_port port, bool en);
extern int mbus_port_setpri(enum mbus_port port, bool pri);
extern int mbus_port_setqos(enum mbus_port port, unsigned int qos);
extern int mbus_port_setwt(enum mbus_port port, unsigned int wt);
extern int mbus_port_setacs(enum mbus_port port, unsigned int acs);
extern int mbus_port_setbwl0(enum mbus_port port, unsigned int bwl0);
extern int mbus_port_setbwl1(enum mbus_port port, unsigned int bwl1);
extern int mbus_port_setbwl2(enum mbus_port port, unsigned int bwl2);
extern int mbus_port_setbwl(enum mbus_port port, unsigned int bwl0,
			    unsigned int bwl1, unsigned int bwl2);
extern int mbus_set_bwlwen(bool enable);
extern int mbus_set_bwlwsiz(unsigned int size);
extern int mbus_port_control_by_index(enum mbus_port port, bool enable);
extern bool mbus_probed(void);
extern int mbus_port_setbwcu(unsigned int unit);
#else
static inline int mbus_port_setbwlen(enum mbus_port port, bool en)
{
	return -ENODEV;
}
static inline int mbus_port_setpri(enum mbus_port port, bool pri)
{
	return -ENODEV;
}
static inline int mbus_port_setqos(enum mbus_port port, unsigned int qos)
{
	return -ENODEV;
}
static inline int mbus_port_setwt(enum mbus_port port, unsigned int wt)
{
	return -ENODEV;
}
static inline int mbus_port_setacs(enum mbus_port port, unsigned int acs)
{
	return -ENODEV;
}
static inline int mbus_port_setbwl0(enum mbus_port port, unsigned int bwl0)
{
	return -ENODEV;
}
static inline int mbus_port_setbwl1(enum mbus_port port, unsigned int bwl1)
{
	return -ENODEV;
}
static inline int mbus_port_setbwl2(enum mbus_port port, unsigned int bwl2)
{
	return -ENODEV;
}
static inline int mbus_port_setbwl(enum mbus_port port, unsigned int bwl0,
				   unsigned int bwl1, unsigned int bwl2)
{
	return -ENODEV;
}
static inline int mbus_set_bwlwen(bool enable)
{
	return -ENODEV;
}
static inline int mbus_set_bwlwsiz(unsigned int size)
{
	return -ENODEV;
}
static inline int mbus_port_control_by_index(enum mbus_port port, bool enable)
{
	return -ENODEV;
}
static inline bool mbus_probed(void)
{
	return false;
}
static inline int mbus_port_setbwcu(unsigned int unit)
{
	return -ENODEV;
}
#endif

#define mbus_disable_port_by_index(dev) \
	mbus_port_control_by_index(dev, false)
#define mbus_enable_port_by_index(dev) \
	mbus_port_control_by_index(dev, true)

#endif
