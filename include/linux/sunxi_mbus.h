/*
 * SUNXI MBUS support
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

#ifndef __LINUX_SUNXI_MBUS_H
#define __LINUX_SUNXI_MBUS_H

/* Port ids */
typedef enum mbus_port{
	MBUS_PORT_CPU   = 0,
#if defined CONFIG_ARCH_SUN8IW5P1
	MBUS_PORT_GPU   = 1,
	MBUS_PORT_DMA   = 2,
	MBUS_PORT_VE    = 3,
	MBUS_PORT_BE    = 4,
	MBUS_PORT_FE    = 5,
	MBUS_PORT_CSI   = 6,
	MBUS_PORT_IEP   = 7,
	MBUS_PORT_CPUS  = 8,
	MBUS_PORT_USB0  = 9,
	MBUS_PORT_MSTG0 = 10,
	MBUS_PORT_USB1  = 11,
	MBUS_PORT_MSTG2 = 12,
	MBUS_PORT_NAND  = 13,
	MBUS_PORT_TAHB  = 14,
	MBUS_PORT_MSTG1 = 15,
	MBUS_PORTS_MAX  = 16,
#elif defined CONFIG_ARCH_SUN8IW6P1
	MBUS_PORT_GPU   = 1,
	/* reversed bit2     */
	MBUS_PORT_DMA   = 3,
	MBUS_PORT_VE    = 4,
	MBUS_PORT_CSI   = 5,
	MBUS_PORT_NAND  = 6,
	MBUS_PORT_IEP   = 7,
	MBUS_PORT_BE    = 8,
	MBUS_PORT_FE    = 9,
	MBUS_PORT_SS    = 10,
	MBUS_PORT_IEP1  = 11,
	MBUS_PORT_BE1   = 12,
	MBUS_PORT_FE1   = 13,
	MBUS_PORTS_MAX  = 14,
	/* reversed bit14/15 */
	MBUS_PORT_CPUS  = 16,
	MBUS_PORT_TEST  = 17,
	MBUS_PORT_USB0  = 18,
	MBUS_PORT_SD0   = 19,
	MBUS_PORT_SD1   = 20,
	MBUS_PORT_SD2   = 21,
	MBUS_PORT_USB1  = 22,
	MBUS_PORT_USB2  = 23,
	MBUS_PORT_GMAC  = 24,
	MBUS_PORTSAE_MAX= 25,
#elif defined CONFIG_ARCH_SUN8IW7P1
	MBUS_PORT_GPU   = 1,
	/* reversed bit2     */
	MBUS_PORT_DMA   = 3,
	MBUS_PORT_VE    = 4,
	MBUS_PORT_CSI   = 5,
	MBUS_PORT_NAND  = 6,
	MBUS_PORT_IEP   = 7,
	MBUS_PORT_BE    = 8,
	MBUS_PORT_FE    = 9,
	MBUS_PORT_SS    = 10,
	MBUS_PORT_IEP1  = 11,
	MBUS_PORT_BE1   = 12,
	MBUS_PORT_FE1   = 13,
	/* reversed bit14/15 */
	MBUS_PORT_CPUS  = 16,
	MBUS_PORT_TEST  = 17,
	MBUS_PORT_USB0  = 18,
	MBUS_PORT_SD0   = 19,
	MBUS_PORT_SD1   = 20,
	MBUS_PORT_SD2   = 21,
	MBUS_PORT_USB1  = 22,
	MBUS_PORT_USB2  = 23,
	MBUS_PORT_GMAC  = 24,
	MBUS_PORTS_MAX  = 25,
#elif defined CONFIG_ARCH_SUN8IW8P1
	/* reversed bit1     */
	MBUS_PORT_MAHB  = 2,
	MBUS_PORT_DMA   = 3,
	MBUS_PORT_VE    = 4,
	MBUS_PORT_CSI   = 5,
	/* reversed bit6     */
	MBUS_PORT_DE    = 7,
	/* reversed bit8~16  */
	MBUS_PORT_ATH   = 17,
	MBUS_PORT_USB0  = 18,
	MBUS_PORT_SD0   = 19,
	MBUS_PORT_SD1   = 20,
	MBUS_PORT_SD2   = 21,
	MBUS_PORT_GMAC  = 22,
	MBUS_PORTS_MAX  = 23,
#endif
}mbus_port_e;

struct device_node;

#ifdef CONFIG_SUNXI_MBUS
extern int mbus_port_setbwlen(mbus_port_e port, bool en);
extern int mbus_port_setpri(mbus_port_e port, bool pri);
extern int mbus_port_setqos(mbus_port_e port, unsigned int qos);
extern int mbus_port_setwt(mbus_port_e port, unsigned int wt);
extern int mbus_port_setacs(mbus_port_e port, unsigned int acs);
extern int mbus_port_setbwl0(mbus_port_e port, unsigned int bwl0);
extern int mbus_port_setbwl1(mbus_port_e port, unsigned int bwl1);
extern int mbus_port_setbwl2(mbus_port_e port, unsigned int bwl2);
extern int mbus_port_setbwl(mbus_port_e port, unsigned int bwl0, \
                             unsigned int bwl1, unsigned int bwl2);
extern int mbus_set_bwlwen(bool enable);
extern int mbus_set_bwlwsiz(unsigned int size);
extern int mbus_port_control_by_index(mbus_port_e port, bool enable);
extern bool mbus_probed(void);
extern int mbus_port_setbwcu(unsigned int unit);
#else
static inline int mbus_port_setbwlen(mbus_port_e port, bool en) { return -ENODEV; }
static inline int mbus_port_setpri(mbus_port_e port, bool pri) { return -ENODEV; }
static inline int mbus_port_setqos(mbus_port_e port, unsigned int qos) { return -ENODEV; }
static inline int mbus_port_setwt(mbus_port_e port, unsigned int wt) { return -ENODEV; }
static inline int mbus_port_setacs(mbus_port_e port, unsigned int acs) { return -ENODEV; }
static inline int mbus_port_setbwl0(mbus_port_e port, unsigned int bwl0) { return -ENODEV; }
static inline int mbus_port_setbwl1(mbus_port_e port, unsigned int bwl1) { return -ENODEV; }
static inline int mbus_port_setbwl2(mbus_port_e port, unsigned int bwl2) { return -ENODEV; }
static inline int mbus_port_setbwl(mbus_port_e port, unsigned int bwl0, \
                             unsigned int bwl1, unsigned int bwl2)
{
	return -ENODEV;
}
static inline int mbus_set_bwlwen(bool enable) { return -ENODEV; }
static inline int mbus_set_bwlwsiz(unsigned int size) { return -ENODEV; }
static inline int mbus_port_control_by_index(mbus_port_e port, bool enable) { return -ENODEV; }
static inline bool mbus_probed(void) { return false; }
int mbus_port_setbwcu(unsigned int unit) { return -ENODEV; }
#endif

#define mbus_disable_port_by_index(dev) \
	mbus_port_control_by_index(dev, false)
#define mbus_enable_port_by_index(dev) \
	mbus_port_control_by_index(dev, true)

#endif
