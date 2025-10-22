/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
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
#include <linux/cdev.h>

enum NSI_TOPOLOGY_TYPE_E {
	NSI_TOPO_V0, /* legacy, all master to one RA and one TA and one DDR, e.g sun50iw10 */
	NSI_TOPO_V1, /* CPU isolated, CPU bypass RA and TA, e.g sun55iw3 */
	NSI_TOPO_V2,
};

enum NSI_CLK_PATH_E {
	NSI_CLK_PATH_V0, /* legacy, nsi clock is mbus clock */
	NSI_CLK_PATH_V1, /* nsi module clock depart from mbus clock */
};

enum NSI_CHANNEL_TYPE_E {
	NSI_DEFAULT_CHANNEL = 0,
	NSI_SINGLE_CHANNLE,
	NSI_DUAL_CHANNEL,
};

struct nsi_pmu_data {
	struct device *dev_nsi;
	unsigned long period;
	spinlock_t bwlock;
};

enum NSI_MASTER_TYPE_E {
	NSI_IA_MASTER,
	NSI_CPU_MASTER,
};
struct nsi_master_dev {
	const char *name;
	struct device *dev;
	uint32_t id;
	enum NSI_MASTER_TYPE_E type;
	union {
		struct {
			void *base;
			struct clk *pmu_clk;
		} cpu_direct;
		struct {
			u32 ia_index;
		} ia_master;
	};
};

struct nsi_bus {
	struct cdev cdev;
	struct device *dev;
	void __iomem *base;
	void __iomem *cpu_base;
	struct clk *pclk;  /* PLL clock */
	struct clk *mclk;  /* mbus clock */
	struct reset_control *reset;
	unsigned long rate;

	/*
	 * master dedicated clock
	 * master id -> clk_idx idx -> clks idx -> clk for this master
	 * clk = clks[ clk_idx[ master_id ] ]
	 */
	struct clk **clks;
	u32 *clk_idx;

	/*
	 * how "mater - IA - RA - RDM - RA - TA" connect
	 * different topology have different ways to calculate
	 * bandwidth from regs
	 */
	enum NSI_TOPOLOGY_TYPE_E topo_tpye;

	/*
	 * different clk path have different clks/resets to deal with
	 * use a point array to hold them
	 */
	enum NSI_CLK_PATH_E clk_path_type;
	enum NSI_CHANNEL_TYPE_E channel_type;
	struct reset_control *reset_cfg;
	u32 ia_pmu_data_unit;
	u32 ra_pmu_data_unit;
	u32 ta_pmu_data_unit;
	u32 cpu_pmu_data_unit;
	int irq;
	u32 skip_mask;
	struct nsi_master_dev *master;
	u32	sub_node_id_mapping;
	u32 master_cnt;
};

extern struct nsi_bus sunxi_nsi;

/* MBUS PMU ids */
enum nsi_pmu {
#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
	MBUS_PMU_CPU    = 0,
	MBUS_PMU_GPU     = 1,
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
	MBUS_PMU_CPU    = 0,
	MBUS_PMU_GPU     = 1,
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
#elif IS_ENABLED(CONFIG_ARCH_SUN55IW3)
	MBUS_PMU_GPU			= 0,
	MBUS_PMU_GIC			= 1,
	MBUS_PMU_USB3			= 2,
	MBUS_PMU_PCIE			= 3,
	MBUS_PMU_CE			= 4,
	MBUS_PMU_NPU			= 5,
	MBUS_PMU_ISP			= 6,
	MBUS_PMU_DSP			= 7,
	MBUS_PMU_G2D			= 8,
	MBUS_PMU_DI			= 9,
	MBUS_PMU_IOMMU			= 10,
	MBUS_PMU_VE_R			= 11,
	MBUS_PMU_VE_RW			= 12,
	MBUS_PMU_DE			= 13,
	MBUS_PMU_CSI			= 14,
	MBUS_PMU_NAND			= 15,
	MBUS_PMU_MATRIX			= 16,
	MBUS_PMU_SPI			= 17,
	MBUS_PMU_GMAC0			= 18,
	MBUS_PMU_GMAC1			= 19,
	MBUS_PMU_SMHC0			= 20,
	MBUS_PMU_SMHC1			= 21,
	MBUS_PMU_SMHC2			= 22,
	MBUS_PMU_USB0			= 23,
	MBUS_PMU_USB1			= 24,
	MBUS_PMU_USB2			= 25,
	MBUS_PMU_NPD			= 26,
	MBUS_PMU_DMAC			= 27,
	MBUS_PMU_DMA			= 28,
	MBUS_PMU_IAG_MAX,
	MBUS_PMU_TAG			= 31,
#elif IS_ENABLED(CONFIG_ARCH_SUN60IW2)
	MBUS_PMU_GMAC			= 0,
	MBUS_PMU_MSI_LITE0			= 1,
	MBUS_PMU_DE			= 2,
	MBUS_PMU_EINK			= 3,
	MBUS_PMU_DI			= 4,
	MBUS_PMU_G2D			= 5,
	MBUS_PMU_GPU			= 6,
	MBUS_PMU_VE0			= 7,
	MBUS_PMU_VE1			= 8,
	MBUS_PMU_VE2		= 9,
	MBUS_PMU_GIC			= 10,
	MBUS_PMU_MSI_LITE1		= 11,
	MBUS_PMU_MSI_LITE2			= 12,
	MBUS_PMU_USB_PCIE			= 13,
	MBUS_PMU_IOMMU0			= 14,
	MBUS_PMU_IOMMU1			= 15,
	MBUS_PMU_ISP			= 16,
	MBUS_PMU_CSI			= 17,
	MBUS_PMU_NPU			= 18,
	MBUS_PMU_CPU			= 19,
	MBUS_PMU_CPU1			= 20,
	MBUS_PMU_IAG_MAX,
	MBUS_PMU_TAG			= 32,
#elif IS_ENABLED(CONFIG_ARCH_SUN55IW6)
	MBUS_PMU_CPU			= 0,
	MBUS_PMU_GIC			= 1,
	MBUS_PMU_ISP			= 2,
	MBUS_PMU_CSI			= 3,
	MBUS_PMU_DE			= 4,
	MBUS_PMU_VE			= 5,
	MBUS_PMU_IOMMU			= 6,
	MBUS_PMU_NPU			= 7,
	MBUS_PMU_G2D			= 8,
	MBUS_PMU_PCIE			= 9,
	MBUS_PMU_CE			= 10,
	MBUS_PMU_RISCV			= 11,
	MBUS_PMU_LBC			= 12,
	MBUS_PMU_GMAC			= 13,
	MBUS_PMU_DMAC0			= 14,
	MBUS_PMU_DMAC1			= 15,
	MBUS_PMU_NAND			= 16,
	MBUS_PMU_CAN			= 17,
	MBUS_PMU_NPD			= 18,
	MBUS_PMU_SPI			= 19,
	MBUS_PMU_USB0			= 20,
	MBUS_PMU_USB1			= 21,
	MBUS_PMU_MATRIX			= 22,
	MBUS_PMU_SMHC0			= 23,
	MBUS_PMU_SMHC1			= 24,
	MBUS_PMU_SMHC2			= 25,
	MBUS_PMU_IAG_MAX,
	MBUS_PMU_TAG			= 30,
#elif IS_ENABLED(CONFIG_ARCH_SUN65IW1)
	MBUS_PMU_ISP			= 0,
	MBUS_PMU_CSI			= 1,
	MBUS_PMU_DE			= 2,
	MBUS_PMU_EINK			= 3,
	MBUS_PMU_GPU			= 4,
	MBUS_PMU_VE			= 5,
	MBUS_PMU_IOMMU			= 6,
	MBUS_PMU_G2D			= 7,
	MBUS_PMU_PCIE			= 8,
	MBUS_PMU_CE			= 9,
	MBUS_PMU_GMAC			= 10,
	MBUS_PMU_DMAC			= 11,
	MBUS_PMU_NPD			= 12,
	MBUS_PMU_USB0			= 13,
	MBUS_PMU_USB1			= 14,
	MBUS_PMU_MATRIX			= 15,
	MBUS_PMU_SMHC0			= 16,
	MBUS_PMU_SMHC1			= 17,
	MBUS_PMU_SMHC2			= 18,
	MBUS_PMU_CPU			= 19,
	MBUS_PMU_IAG_MAX,
	MBUS_PMU_TAG			= 22,
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
#elif IS_ENABLED(CONFIG_ARCH_SUN55IW3)
static const char *const pmu_name[] = {
	"cpu", "gpu", "gic", "usb3", "pcie", "ce", "npu", "isp", "dsp", "g2d", "di", "iommu",
	"ve_r", "ve_rw", "de", "csi", "nand", "matrix", "spi", "gmac0", "gmac1", "smhc0",
	"smhc1", "smhc2", "usb0", "usb1", "usb2", "npd", "dmac", "dma", "total"
};
#elif IS_ENABLED(CONFIG_ARCH_SUN60IW2)
static const char *const pmu_name[] = {
	"gmac", "msi_list0", "de", "eink", "di", "g2d", "gpu", "ve0", "ve1", "ve2", "gic",
	"msi_lite1", "msi_lite2", "usb_pcie", "iommu0", "iommu1", "isp", "csi", "npu", "cpu0", "cpu1",
	"total"
};
#elif IS_ENABLED(CONFIG_ARCH_SUN55IW6)
static const char *const pmu_name[] = {
	"cpu",  "gic",   "isp",    "csi",   "de",    "ve",    "iommu", "npu", "g2d", "pcie",
	"ce",   "riscv", "lbc",    "gmac",  "dmac0", "dmac1", "nand",  "can", "npd", "spi",
	"usb0", "usb1",  "matrix", "smhc1", "smhc1", "smhc2", "total",
};
#elif IS_ENABLED(CONFIG_ARCH_SUN65IW1)
static const char *const pmu_name[] = {
	"isp",  "csi",   "de",  "eink",  "gpu",  "ve",  "iommu",  "g2d",   "pcie",  "ce",
	"gmac", "dmac",  "npd", "usb0",  "usb1", "matrix", "smhc0",  "smhc1", "smhc2", "cpu", "total",
};
#endif

#define get_name(n)      pmu_name[n]

#if IS_ENABLED(CONFIG_AW_NSI)
extern int nsi_port_setpri(enum nsi_pmu port, unsigned int pri);
extern int nsi_port_setqos(enum nsi_pmu port, unsigned int qos);
extern bool nsi_probed(void);
extern int sunxi_nsi_ecc_init(struct platform_device *pdev);
extern void sunxi_nsi_ecc_exit(struct platform_device *pdev);
extern int notrace nsi_port_set_abs_bwl(enum nsi_pmu port, unsigned int bwl);
extern int notrace nsi_port_set_abs_bwlen(enum nsi_pmu port, bool en);
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
#define AW_NSI_CPU_CHANNEL	1
#define NSI_HARDCODED_PORT_MAPPING 1
#endif

#if IS_ENABLED(CONFIG_AW_NSI_CPU_CHANNEL)
#define AW_NSI_CPU_CHANNEL	1
#endif

#define nsi_disable_port_by_index(dev) \
	nsi_port_control_by_index(dev, false)
#define nsi_enable_port_by_index(dev) \
	nsi_port_control_by_index(dev, true)

#endif
