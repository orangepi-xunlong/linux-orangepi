/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
 /*
  * SUNXI NPD support
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

#define DRIVER_NAME          "NPD"
#define NPD_MAJOR 138
#define NPD_MINORS 256
#define PMU_MAX 8
#define MEM_SIZE 5000

#define AHB_RATE 150000000

/* address of registers */
#define NPD_BASE_ADDR 0x02070000
#define VERSION_REG 0x0000
#define ANTO_CLKGATING_REG 0x0004
#define NPD_EN_REG 0x000C
#define PMU_EN_REG 0x0010
#define COMMON_EN_REG 0x0014
#define NPD_PERIOD_REG 0x0018
#define NPD_NUM_REG 0x001C
#define DDR_BASE_REG 0x0020
#define PMU_STEP_REG 0x0024
#define PMUREG_STEP_REG 0x0028
#define COM_STEP_REG 0x002C
#define COM_DDR_REG 0x0030 /* R, show common register result address of ddr */
#define PMU_BASEADDR_REG(n) (0x0034 + (0x0004 * (n))) /* 0x0034 ~ 0x0050 */
#define COM_BASEADDR_REG(n) (0x0060 + (0x0004 * (n))) /* 0x0060 ~ 0x00AC */

#define NSI_BASE_ADDR(n) (0x02020000 + (0x200 * (n)) + 0x00cc)

#ifdef CONFIG_ARCH_SUN55I
#define CPU_DIRECT_ACC
#define MBUS_PMU_CPU -1
#define CPU_PMU_ADDR 0x02071080

static int pmu_pick[PMU_MAX] = {
	MBUS_PMU_CPU,
	MBUS_PMU_GPU,
	MBUS_PMU_DE,
	MBUS_PMU_ISP,
	MBUS_PMU_VE_R,
	MBUS_PMU_VE_RW,
	MBUS_PMU_G2D,
	MBUS_PMU_CSI,
};
#endif

#ifdef CONFIG_ARCH_SUN50I
static int pmu_pick[PMU_MAX] = {
	MBUS_PMU_CPU,
	MBUS_PMU_GPU,
	MBUS_PMU_VE_R,
	MBUS_PMU_VE,
	MBUS_PMU_TVDISP_MBUS,
	MBUS_PMU_TVDISP_AXI,
	MBUS_PMU_VE_RW,
	MBUS_PMU_TVFE
};
#endif

/* sunxi_npd struct */
struct sunxi_npd_dev_t {
	struct cdev cdev;
	struct device *dev_npd;
	struct clk *ahb_clk;
	/* private data */
	uint8_t *base_addr;
	uint8_t *ddr_addr;
	unsigned int ahb_rate;
	uint32_t period;
	uint32_t cycle;
	spinlock_t bwlock;
} sunxi_npd;
