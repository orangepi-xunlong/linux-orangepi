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

/* MBUS PMU ids */
enum nsi_pmu {
	MBUS_PMU_CPU    = 0,
	MBUS_PMU_GPU     = 1,

#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
	MBUS_PMU_SD1     = 2,
	MBUS_PMU_MSTG    = 3,
	MBUS_PMU_GMAC0   = 4,
	MBUS_PMU_GMAC1   = 5,
	MBUS_PMU_USB0    = 6,
	MBUS_PMU_USB1    = 7,
	MBUS_PMU_NDFC    = 8,
	MBUS_PMU_DMAC    = 9,
	MBUS_PMU_CE      = 10,
	MBUS_PMU_DE0     = 11,
	MBUS_PMU_DE1     = 12,
	MBUS_PMU_VE      = 13,
	MBUS_PMU_CSI     = 14,
	MBUS_PMU_ISP     = 15,
	MBUS_PMU_G2D     = 16,
	MBUS_PMU_EINK    = 17,
	MBUS_PMU_IOMMU   = 18,
	MBUS_PMU_SYS_CPU = 19,
	MBUS_PMU_IAG_MAX,
	MBUS_PMU_TAG = 23,
#elif IS_ENABLED(CONFIG_ARCH_SUN50IW12)
	MBUS_PMU_VE_R			= 2,
	MBUS_PMU_VE				= 3,
	MBUS_PMU_TVDISP_MBUS	= 4,
	MBUS_PMU_TVDISP_AXI		= 5,
	MBUS_PMU_VE_RW			= 6,
	MBUS_PMU_TVFE			= 7,
	MBUS_PMU_NDFC			= 8,
	MBUS_PMU_DMAC			= 9,
	MBUS_PMU_CE				= 10,
	MBUS_PMU_IOMMU			= 11,
	MBUS_PMU_TVCAP			= 12,
	MBUS_PMU_GMAC0			= 13,
	MBUS_PMU_MSTG			= 14,
	MBUS_PMU_MIPS			= 15,
	MBUS_PMU_USB0			= 16,
	MBUS_PMU_USB1			= 17,
	MBUS_PMU_USB2			= 18,
	MBUS_PMU_MSTG1			= 19,
	MBUS_PMU_MSTG2			= 20,
	MBUS_PMU_NPD			= 21,
	MBUS_PMU_IAG_MAX,
	/* use RA1 to get total bandwidth, because no TA pmu for sun50iw12 */
	MBUS_PMU_TAG = 24,
#endif
	MBUS_PMU_MAX,
};

#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
static const char *const pmu_name[] = {
	"cpu", "gpu", "sd1", "mstg", "gmac0", "gmac1", "usb0", "usb1", "ndfc",
	"dmac", "ce", "de0", "de1", "ve", "csi", "isp", "g2d", "eink", "iommu",
	"sys_cpu", "total",
};
#elif IS_ENABLED(CONFIG_ARCH_SUN50IW12)
static const char *const pmu_name[] = {
	"cpu", "gpu", "ve_r", "ve", "tvd_mbus", "tvd_axi", "ve_rw", "tvfe", "ndfc", "dmac", "ce",
	"iommu", "tvcap", "gmac0", "mstg", "mips", "usb0", "usb1", "usb2", "mstg1", "mstg2",
	"npd", "total",
};
#endif

#define get_name(n)      pmu_name[n]

#if IS_ENABLED(CONFIG_SUNXI_NSI)
extern int nsi_port_setpri(enum nsi_pmu port, unsigned int pri);
extern int nsi_port_setqos(enum nsi_pmu port, unsigned int qos);
extern bool nsi_probed(void);
#endif

#define nsi_disable_port_by_index(dev) \
	nsi_port_control_by_index(dev, false)
#define nsi_enable_port_by_index(dev) \
	nsi_port_control_by_index(dev, true)

#endif
