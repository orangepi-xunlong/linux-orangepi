/*
 * SUNXI GT BUS support
 *
 * Copyright (C) 2014 AllWinnertech Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __LINUX_SUNXI_GTBUS_H
#define __LINUX_SUNXI_GTBUS_H

#include <linux/errno.h>
#include <linux/types.h>

/* Port ids */
typedef enum gt_port{
	GT_PORT_CPUM1   = 0,
	GT_PORT_CPUM2   = 1,
	GT_PORT_SATA	= 2,
	GT_PORT_USB3	= 3,
	GT_PORT_FE0 	= 4,
	GT_PORT_BE1 	= 5,
	GT_PORT_BE2 	= 6,
	GT_PORT_IEP0	= 7,
	GT_PORT_FE1 	= 8,
	GT_PORT_BE0 	= 9,
	GT_PORT_FE2 	= 10,
	GT_PORT_IEP1	= 11,
	GT_PORT_VED 	= 12,
	GT_PORT_VEE 	= 13,
	GT_PORT_FD  	= 14,
	GT_PORT_CSI 	= 15,
	GT_PORT_MP  	= 16,
	GT_PORT_HSI 	= 17,
	GT_PORT_SS  	= 18,
	GT_PORT_TS  	= 19,
	GT_PORT_DMA 	= 20,
	GT_PORT_NDFC0   = 21,
	GT_PORT_NDFC1   = 22,
	GT_PORT_CPUS	= 23,
	GT_PORT_TH  	= 24,
	GT_PORT_GMAC 	= 25,
	GT_PORT_USB0 	= 26,
	GT_PORT_MSTG0   = 27,
	GT_PORT_MSTG1   = 28,
	GT_PORT_MSTG2   = 29,
	GT_PORT_MSTG3   = 30,
	GT_PORT_USB1 	= 31,
	GT_PORT_GPU0 	= 32,
	GT_PORT_GPU1 	= 33,
	GT_PORT_USB2	= 34,
	GT_PORT_CPUM0   = 35,
}gt_port_e;

struct device_node;

#ifdef CONFIG_SUNXI_GTBUS
extern int gt_port_setreqn(gt_port_e port, unsigned int reqn);
extern int gt_port_setthd(gt_port_e port, unsigned int thd0, unsigned int thd1);
extern int gt_port_setqos(gt_port_e port, unsigned int qos);
extern int gt_bw_setbw(unsigned int bw);
extern int gt_priority_set(gt_port_e port, unsigned int pri);
extern int gt_bw_enable(bool enable);
extern int gt_control_port_by_index(gt_port_e port, bool enable);
extern bool gt_probed(void);
#else
static inline int gt_port_setreqn(gt_port_e port, unsigned int reqn) { return -ENODEV; }
static inline int gt_port_setthd(gt_port_e port, unsigned int thd0, unsigned int thd1)
{
	return -ENODEV;
}
static inline int gt_port_setqos(gt_port_e port, unsigned int qos) { return -ENODEV; }
static inline int gt_bw_setbw(unsigned int bw) { return -ENODEV; }
static inline int gt_priority_set(gt_port_e port, unsigned int pri) { return -ENODEV; }
static inline int gt_bw_enable(bool enable) { return -ENODEV; }
static inline int gt_control_port_by_index(gt_port_e port, bool enable) { return -ENODEV; }
static inline bool gt_probed(void) { return false; }
#endif

#define gt_disable_port_by_index(dev) \
	gt_control_port_by_index(dev, false)
#define gt_enable_port_by_index(dev) \
	gt_control_port_by_index(dev, true)

#endif
