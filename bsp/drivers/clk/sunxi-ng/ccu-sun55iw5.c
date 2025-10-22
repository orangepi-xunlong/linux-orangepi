// SPDX-License-Identifier: GPL-3.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 zhaozeyan@allwinnertech.com
 */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include "ccu_common.h"
#include "ccu_reset.h"

#include "ccu_div.h"
#include "ccu_gate.h"
#include "ccu_mp.h"
#include "ccu_mult.h"
#include "ccu_nk.h"
#include "ccu_nkm.h"
#include "ccu_nkmp.h"
#include "ccu_nm.h"

#include "ccu-sun55iw5.h"
/* ccu_des_start */

#define UPD_KEY_VALUE	0x8000000

#define SUN55IW5_PLL_DDR_CTRL_REG   0x0010
static struct ccu_nkmp pll_ddr_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.common		= {
		.reg		= 0x0010,
		.hw.init	= CLK_HW_INIT("pll-ddr", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN55IW5_PLL_GPU_CTRL_REG   0x0030
static struct ccu_nkmp pll_gpu_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.common		= {
		.reg		= 0x0030,
		.hw.init	= CLK_HW_INIT("pll-gpu", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN55IW5_PLL_PERI0_CTRL_REG   0x0020
static struct ccu_nm pll_peri0_parent_clk = {
	.enable		= BIT(27) | BIT(30) | BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.common		= {
		.reg		= 0x0020,
		.hw.init	= CLK_HW_INIT("pll-peri0-parent", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static SUNXI_CCU_M(pll_peri0_2x_clk, "pll-peri0-2x",
		"pll-peri0-parent", 0x0020, 20, 3, 0);

static CLK_FIXED_FACTOR_HW(pll_peri0_div3_clk, "pll-peri0-div3",
		&pll_peri0_2x_clk.common.hw,
		6, 1, 0);

static SUNXI_CCU_M(pll_peri0_800m_clk, "pll-peri0-800m",
		"pll-peri0-parent", 0x0020, 16, 3, 0);

static SUNXI_CCU_M(pll_peri0_480m_clk, "pll-peri0-480m",
		"pll-peri0-parent", 0x0020, 2, 3, 0);

static CLK_FIXED_FACTOR_HW(pll_peri0_600m_clk, "pll-peri0-600m",
		&pll_peri0_2x_clk.common.hw,
		2, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri0_400m_clk, "pll-peri0-400m",
		&pll_peri0_2x_clk.common.hw,
		3, 1, 0);

static CLK_FIXED_FACTOR(pll_peri0_300m_clk, "pll-peri0-300m",
		"pll-peri0-600m",
		2, 1, 0);

static CLK_FIXED_FACTOR(pll_peri0_200m_clk, "pll-peri0-200m",
		"pll-peri0-400m",
		2, 1, 0);

static CLK_FIXED_FACTOR(pll_peri0_160m_clk, "pll-peri0-160m",
		"pll-peri0-480m",
		3, 1, 0);

static CLK_FIXED_FACTOR(pll_peri0_16m_clk, "pll-peri0-16m",
		"pll-peri0-160m",
		10, 1, 0);

static CLK_FIXED_FACTOR(pll_peri0_150m_clk, "pll-peri0-150m",
		"pll-peri0-300m",
		2, 1, 0);

static CLK_FIXED_FACTOR(pll_peri0_25m_clk, "pll-peri0-25m",
		"pll-peri0-150m",
		6, 1, 0);

#define SUN55IW5_PLL_PERI1_CTRL_REG   0x0028
static struct ccu_nm pll_peri1_parent_clk = {
	.enable		= BIT(27) | BIT(30) | BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.common		= {
		.reg		= 0x0028,
		.hw.init	= CLK_HW_INIT("pll-peri1-parent", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static SUNXI_CCU_M(pll_peri1_2x_clk, "pll-peri1-2x",
		"pll-peri1-parent", 0x0028, 20, 3, 0);

static SUNXI_CCU_M(pll_peri1_800m_clk, "pll-peri1-800m",
		"pll-peri1-parent", 0x0028, 16, 3, 0);

static SUNXI_CCU_M(pll_peri1_480m_clk, "pll-peri1-480m",
		"pll-peri1-parent", 0x0028, 2, 3, 0);

static CLK_FIXED_FACTOR_HW(pll_peri1_600m_clk, "pll-peri1-600m",
		&pll_peri1_2x_clk.common.hw,
		2, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri1_400m_clk, "pll-peri1-400m",
		&pll_peri1_2x_clk.common.hw,
		3, 1, 0);

static CLK_FIXED_FACTOR(pll_peri1_300m_clk, "pll-peri1-300m",
		"pll-peri1-600m",
		2, 1, 0);

static CLK_FIXED_FACTOR(pll_peri1_200m_clk, "pll-peri1-200m",
		"pll-peri1-400m",
		2, 1, 0);

static CLK_FIXED_FACTOR(pll_peri1_160m_clk, "pll-peri1-160m",
		"pll-peri1-480m",
		3, 1, 0);

static CLK_FIXED_FACTOR(pll_peri1_150m_clk, "pll-peri1-150m",
		"pll-peri1-300m",
		2, 1, 0);

#define SUN55IW5_PLL_VIDEO0_CTRL_REG   0x0040
static struct ccu_nkmp pll_video0_4x_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.common		= {
		.reg		= 0x0040,
		.hw.init	= CLK_HW_INIT("pll-video0-4x", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN55IW5_PLL_VIDEO2_CTRL_REG   0x0050
static struct ccu_nkmp pll_video2_4x_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.common		= {
		.reg		= 0x0050,
		.hw.init	= CLK_HW_INIT("pll-video2-4x", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN55IW5_PLL_VE_CTRL_REG   0x0058
static struct ccu_nkmp pll_ve_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.common		= {
		.reg		= 0x0058,
		.hw.init	= CLK_HW_INIT("pll-ve", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN55IW5_PLL_ADC_CTRL_REG   0x0060
static struct ccu_nkmp pll_adc_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.common		= {
		.reg		= 0x0060,
		.hw.init	= CLK_HW_INIT("pll-adc", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN55IW5_PLL_VIDEO3_CTRL_REG   0x0068
static struct ccu_nkmp pll_video3_4x_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.common		= {
		.reg		= 0x0068,
		.hw.init	= CLK_HW_INIT("pll-video3-4x", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static const char * const ahb_parents[] = { "dcxo24M", "osc32k", "iosc", "pll-peri0-600m" };

SUNXI_CCU_M_WITH_MUX(ahb_clk, "ahb", ahb_parents,
		0x0510, 0, 5, 24, 2, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);

static const char * const apb0_parents[] = { "dcxo24M", "osc32k", "iosc", "pll-peri0-600m" };

SUNXI_CCU_M_WITH_MUX(apb0_clk, "apb0", apb0_parents,
		0x0520, 0, 5, 24, 2, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);

static const char * const apb1_parents[] = { "dcxo24M", "osc32k", "iosc", "pll-peri0-600m", "pll-peri0-480m" };

static SUNXI_CCU_M_WITH_MUX(apb1_clk, "apb1", apb1_parents,
		0x0524, 0, 5, 24, 2, CLK_SET_RATE_PARENT);

static const char * const mbus_parents[] = { "dcxo24M", "pll-ddr", "pll-peri0-600m", "pll-peri0-480m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE_KEY(mbus_clk, "mbus",
		mbus_parents, 0x0540,
		0, 5,   /* M */
		24, 3,  /* mux */
		UPD_KEY_VALUE,
		BIT(31),        /* gate */
		CLK_GET_RATE_NOCACHE | CLK_IS_CRITICAL);

static const char * const trace_parents[] = { "dcxo24M", "clk32k", "rc16m", "pll-peri0-300m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE(trace_clk, "trace",
		trace_parents, 0x0550,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static const char * const gic_parents[] = { "dcxo24M", "clk32k", "rc16m", "pll-peri0-600m", "pll-peri0-480m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE(gic_clk, "gic",
		gic_parents, 0x0560,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(tvdisp_ahb_gate_clk, "tvdisp-ahb-gate",
		"dcxo24M",
		0x05C0, BIT(8), 0);

static SUNXI_CCU_GATE(tvcap_ahb_gate_clk, "tvcap-ahb-gate",
		"dcxo24M",
		0x05C0, BIT(7), 0);

static SUNXI_CCU_GATE(tvfe_ahb_gate_clk, "tvfe-ahb-gate",
		"dcxo24M",
		0x05C0, BIT(6), 0);

static SUNXI_CCU_GATE(smhc2_ahb_gate_clk, "smhc2-ahb-gate",
		"dcxo24M",
		0x05C0, BIT(5), 0);

static SUNXI_CCU_GATE(smhc1_ahb_gate_clk, "smhc1-ahb-gate",
		"dcxo24M",
		0x05C0, BIT(4), 0);

static SUNXI_CCU_GATE(smhc0_ahb_gate_clk, "smhc0-ahb-gate",
		"dcxo24M",
		0x05C0, BIT(3), 0);

static SUNXI_CCU_GATE(usb2_ahb_gate_clk, "usb2-ahb-gate",
		"dcxo24M",
		0x05C0, BIT(2), 0);

static SUNXI_CCU_GATE(gmac_ahb_gate_clk, "gmac-ahb-gate",
		"dcxo24M",
		0x05C0, BIT(1), 0);

static SUNXI_CCU_GATE(gpu_ahb_gate_clk, "gpu-ahb-gate",
		"dcxo24M",
		0x05C0, BIT(0), 0);

static SUNXI_CCU_GATE(tvdisp_mbus_gate_clk, "tvdisp-mbus-gate",
		"dcxo24M",
		0x05E0, BIT(3), 0);

static SUNXI_CCU_GATE(tvcap_mbus_gate_clk, "tvcap-mbus-gate",
		"dcxo24M",
		0x05E0, BIT(2), 0);

static SUNXI_CCU_GATE(tvfe_mbus_gate_clk, "tvfe-mbus-gate",
		"dcxo24M",
		0x05E0, BIT(1), 0);

static SUNXI_CCU_GATE(gpu_mbus_gate_clk, "gpu-mbus-gate",
		"dcxo24M",
		0x05E0, BIT(0), 0);

static SUNXI_CCU_GATE(gpu_clk, "gpu",
		"dcxo24M",
		0x067C, BIT(0), 0);

static const char * const ce_clk_parents[] = { "dcxo24M", "pll-peri0-600m", "pll-peri0-480m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE(ce_clk, "ce-clk",
		ce_clk_parents, 0x0680,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(ce_sys_clk, "ce-sys",
		"dcxo24M",
		0x068C, BIT(1), 0);

static SUNXI_CCU_GATE(ce_bus_clk, "ce",
		"dcxo24M",
		0x068C, BIT(0), 0);

static const char * const ve_core_clk_parents[] = { "pll-ve", "pll-peri0-800m", "pll-peri0-600m", "pll-peri0-480m" };

static SUNXI_CCU_M_WITH_MUX_GATE(ve_core_clk, "ve-core-clk",
		ve_core_clk_parents, 0x0690,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(av1_clk, "av1",
		"dcxo24M",
		0x069C, BIT(1), 0);

static SUNXI_CCU_GATE(ve_clk, "ve",
		"dcxo24M",
		0x069C, BIT(0), 0);

static SUNXI_CCU_GATE(dma_clk, "dma",
		"dcxo24M",
		0x070C, BIT(0), 0);

static SUNXI_CCU_GATE(msgbox_clk, "msgbox",
		"dcxo24M",
		0x071C, BIT(0), 0);

static SUNXI_CCU_GATE(spinlock_clk, "spinlock",
		"dcxo24M",
		0x072C, BIT(0), 0);

static const char * const timer0_clk_parents[] = { "dcxo24M", "rc16m", "clk32k", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(timer0_clk, "timer0-clk",
		timer0_clk_parents, 0x0730,
		1, 3,
		4, 3,	/* mux */
		BIT(0),	/* gate */
		0);

static const char * const timer1_clk_parents[] = { "dcxo24M", "rc16m", "clk32k", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(timer1_clk, "timer1-clk",
		timer1_clk_parents, 0x0734,
		1, 3,
		4, 3,	/* mux */
		BIT(0),	/* gate */
		0);

static const char * const timer2_clk_parents[] = { "dcxo24M", "rc16m", "clk32k", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(timer2_clk, "timer2-clk",
		timer2_clk_parents, 0x0738,
		1, 3,
		4, 3,	/* mux */
		BIT(0),	/* gate */
		0);

static SUNXI_CCU_GATE(timer0_bus_clk, "timer0",
		"dcxo24M",
		0x0750, BIT(0), 0);

static SUNXI_CCU_GATE(dbgsys_clk, "dbgsys",
		"dcxo24M",
		0x078C, BIT(0), 0);

static SUNXI_CCU_GATE(pwm_clk, "pwm",
		"dcxo24M",
		0x07AC, BIT(0), 0);

static SUNXI_CCU_GATE(iommu_clk, "iommu",
		"dcxo24M",
		0x07BC, BIT(0), 0);

static const char * const dram_clk_parents[] = { "pll-ddr", "pll-peri1-800m", "pll-peri1-600m", "pll-peri1-480m" };

static SUNXI_CCU_M_WITH_MUX_GATE_KEY(dram_clk, "dram",
		dram_clk_parents, 0x800,
		0, 5,   /* M */
		24, 3,  /* mux */
		UPD_KEY_VALUE,
		BIT(31),        /* gate */
		CLK_GET_RATE_NOCACHE | CLK_IS_CRITICAL);

static SUNXI_CCU_GATE(av1_mbus_clk, "av1-mbus",
		"dcxo24M",
		0x0804, BIT(3), 0);

static SUNXI_CCU_GATE(ce_mbus_clk, "ce-mbus",
		"dcxo24M",
		0x0804, BIT(2), 0);

static SUNXI_CCU_GATE(ve3_mbus_clk, "ve3-mbus",
		"dcxo24M",
		0x0804, BIT(1), 0);

static SUNXI_CCU_GATE(dma_mbus_clk, "dma-mbus",
		"dcxo24M",
		0x0804, BIT(0), 0);

static SUNXI_CCU_GATE(dram_mbus_clk, "dram-mbus",
		"dcxo24M",
		0x080C, BIT(0), 0);

static const char * const smhc0_parents[] = { "dcxo24M", "pll-peri0-400m", "pll-peri0-300m", "pll-peri1-400m", "pll-peri1-300m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc0_clk, "smhc0",
		smhc0_parents, 0x0830,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const smhc1_parents[] = { "dcxo24M", "pll-peri0-400m", "pll-peri0-300m", "pll-peri1-400m", "pll-peri1-300m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc1_clk, "smhc1",
		smhc1_parents, 0x0834,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const smhc2_parents[] = { "dcxo24M", "pll-peri0-800m", "pll-peri0-600m", "pll-peri1-800m", "pll-peri1-600m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc2_clk, "smhc2",
		smhc2_parents, 0x0838,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(smhc2_bus_clk, "smhc2-bus",
		"dcxo24M",
		0x084C, BIT(2), 0);

static SUNXI_CCU_GATE(smhc1_bus_clk, "smhc1-bus",
		"dcxo24M",
		0x084C, BIT(1), 0);

static SUNXI_CCU_GATE(smhc0_bus_clk, "smhc0-bus",
		"dcxo24M",
		0x084C, BIT(0), 0);

static SUNXI_CCU_GATE(uart3_clk, "uart3",
		"dcxo24M",
		0x090C, BIT(3), 0);

static SUNXI_CCU_GATE(uart2_clk, "uart2",
		"dcxo24M",
		0x090C, BIT(2), 0);

static SUNXI_CCU_GATE(uart1_clk, "uart1",
		"dcxo24M",
		0x090C, BIT(1), 0);

static SUNXI_CCU_GATE(uart0_clk, "uart0",
		"dcxo24M",
		0x090C, BIT(0), 0);

static SUNXI_CCU_GATE(twi4_clk, "twi4",
		"dcxo24M",
		0x091C, BIT(4), 0);

static SUNXI_CCU_GATE(twi3_clk, "twi3",
		"dcxo24M",
		0x091C, BIT(3), 0);

static SUNXI_CCU_GATE(twi2_clk, "twi2",
		"dcxo24M",
		0x091C, BIT(2), 0);

static SUNXI_CCU_GATE(twi1_clk, "twi1",
		"dcxo24M",
		0x091C, BIT(1), 0);

static SUNXI_CCU_GATE(twi0_clk, "twi0",
		"dcxo24M",
		0x091C, BIT(0), 0);

static const char * const spi0_parents[] = { "dcxo24M", "pll-peri0-300m", "pll-peri0-200m", "pll-peri0-160m", "pll-peri1-300m", "pll-peri1-200m", "pll-peri1-160m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spi0_clk, "spi0",
		spi0_parents, 0x0940,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const spi1_parents[] = { "dcxo24M", "pll-peri0-300m", "pll-peri0-200m", "pll-peri0-160m", "pll-peri1-300m", "pll-peri1-200m", "pll-peri1-160m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spi1_clk, "spi1",
		spi1_parents, 0x0944,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const spi2_parents[] = { "dcxo24M", "pll-peri0-300m", "pll-peri0-200m", "pll-peri0-160m", "pll-peri1-300m", "pll-peri1-200m", "pll-peri1-160m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spi2_clk, "spi2",
		spi2_parents, 0x0948,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(spi2_bus_clk, "spi2-bus",
		"dcxo24M",
		0x096C, BIT(2), 0);

static SUNXI_CCU_GATE(spi1_bus_clk, "spi1-bus",
		"dcxo24M",
		0x096C, BIT(1), 0);

static SUNXI_CCU_GATE(spi0_bus_clk, "spi0-bus",
		"dcxo24M",
		0x096C, BIT(0), 0);

static SUNXI_CCU_GATE(gmac0_phy_clk, "gmac0-phy",
		"dcxo24M",
		0x0970, BIT(31), 0);

static SUNXI_CCU_GATE(gmac0_clk, "gmac0",
		"dcxo24M",
		0x097C, BIT(0), 0);

static const char * const i2spcm0_parents[] = { "pll-audio-4x", "tvfe-audio-clk", "pll-peri0-200m" };

static SUNXI_CCU_MUX_WITH_GATE(i2spcm0_clk, "i2spcm0",
		i2spcm0_parents, 0x0A10,
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(i2spcm0_bus_clk, "i2spcm0-bus",
		"dcxo24M",
		0x0A20, BIT(0), 0);

static SUNXI_CCU_GATE(spdif1_clk, "spdif1",
		"dcxo24M",
		0x0A2C, BIT(1), 0);

static SUNXI_CCU_GATE(spdif0_clk, "spdif0",
		"dcxo24M",
		0x0A2C, BIT(0), 0);

static const char * const spdif0_rx_clk_parents[] = { "pll-peri0-400m", "pll-peri0-300m", "pll-audio" };

static SUNXI_CCU_M_WITH_MUX_GATE(spdif0_rx_clk, "spdif0-rx-clk",
		spdif0_rx_clk_parents, 0x0A30,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const spdif0_tx_parents[] = { "pll-audio-4x", "tvfe-audio-clk" };

static SUNXI_CCU_MUX_WITH_GATE(spdif0_tx_clk, "spdif0-tx",
		spdif0_tx_parents, 0x0A34,
		24, 1,	/* mux */
		BIT(31), 0);

static const char * const spdif1_rx_clk_parents[] = { "pll-peri0-400m", "pll-peri0-300m", "pll-audio" };

static SUNXI_CCU_M_WITH_MUX_GATE(spdif1_rx_clk, "spdif1-rx-clk",
		spdif1_rx_clk_parents, 0x0A40,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const spdif1_tx_parents[] = {"pll-audio-4x", "tvfe-audio-clk" };

static SUNXI_CCU_MUX_WITH_GATE(spdif1_tx_clk, "spdif1-tx",
		spdif1_tx_parents, 0x0A44,
		24, 1,	/* mux */
		BIT(31), 0);

static const char * const audio_codec_dac_1x_parents[] = {"pll-audio-4x", "tvfe-audio-clk" };

static SUNXI_CCU_MUX_WITH_GATE(audio_codec_dac_1x_clk, "audio-codec-dac-1x",
		audio_codec_dac_1x_parents, 0x0A60,
		24, 1,	/* mux */
		BIT(31), 0);

static const char * const audio_codec_adc_1x_parents[] = { "pll-audio-4x", "tvfe-audio-clk" };

static SUNXI_CCU_MUX_WITH_GATE(audio_codec_adc_1x_clk, "audio-codec-adc-1x",
		audio_codec_adc_1x_parents, 0x0A64,
		24, 1,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(audio_codec_clk, "audio-codec",
		"dcxo24M",
		0x0A6C, BIT(0), 0);

static SUNXI_CCU_GATE(usb0_clk, "usb0",
		"dcxo24M",
		0x0A70, BIT(31), 0);

static SUNXI_CCU_GATE(usb1_clk, "usb1",
		"dcxo24M",
		0x0A78, BIT(31), 0);

static SUNXI_CCU_GATE(usb2_clk, "usb2",
		"dcxo24M",
		0x0A80, BIT(31), 0);

static SUNXI_CCU_GATE(usbotg_clk, "usbotg",
		"dcxo24M",
		0x0A8C, BIT(8), 0);

static SUNXI_CCU_GATE(usbehci2_clk, "usbehci2",
		"dcxo24M",
		0x0A8C, BIT(6), 0);

static SUNXI_CCU_GATE(usbehci1_clk, "usbehci1",
		"dcxo24M",
		0x0A8C, BIT(5), 0);

static SUNXI_CCU_GATE(usbehci0_clk, "usbehci0",
		"dcxo24M",
		0x0A8C, BIT(4), 0);

static SUNXI_CCU_GATE(usbohci2_clk, "usbohci2",
		"dcxo24M",
		0x0A8C, BIT(2), 0);

static SUNXI_CCU_GATE(usbohci1_clk, "usbohci1",
		"dcxo24M",
		0x0A8C, BIT(1), 0);

static SUNXI_CCU_GATE(usbohci0_clk, "usbohci0",
		"dcxo24M",
		0x0A8C, BIT(0), 0);

static SUNXI_CCU_GATE(tvfe_axi_clk, "tvfe-axi",
		"dcxo24M",
		0x0D0C, BIT(0), 0);

static const char * const adc_parents[] = { "pll-adc", "pll-video0-1x" };

static SUNXI_CCU_MUX_WITH_GATE(adc_clk, "adc",
		adc_parents, 0x0D10,
		24, 1,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(tsdm_ts_clk, "tsdm-ts",
		"dcxo24M",
		0x0D14, BIT(31), 0);

static const char * const dtmb_clk120m_parents[] = { "pll-adc", "pll-peri0-600m" };

static SUNXI_CCU_MUX_WITH_GATE(dtmb_clk120m_clk, "dtmb-clk120m",
		dtmb_clk120m_parents, 0x0D18,
		24, 1,	/* mux */
		BIT(31), 0);

static const char * const tvfe_1296m_parents[] = { "pll-video0-4x", "pll-adc" };

static SUNXI_CCU_MUX_WITH_GATE(tvfe_1296m_clk, "tvfe-1296m",
		tvfe_1296m_parents, 0x0D20,
		24, 1,	/* mux */
		BIT(31), 0);

static const char * const i2h_clk_parents[] = { "pll-video0-4x", "pll-peri0-2x", "pll-peri1-2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(i2h_clk_clk, "i2h-clk",
		i2h_clk_parents, 0x0D24,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const audio_cpu_clk_parents[] = { "pll-peri0-800m", "pll-peri0-600m", "pll-peri0-480m", "pll-peri1-800m", "pll-peri1-600m", "pll-peri1-480m" };

static SUNXI_CCU_M_WITH_MUX_GATE(audio_cpu_clk, "audio-cpu-clk",
		audio_cpu_clk_parents, 0x0D48,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const audio_umac_clk_parents[] = { "pll-peri0-600m", "pll-peri1-600m" };

static SUNXI_CCU_M_WITH_MUX_GATE(audio_umac_clk, "audio-umac-clk",
		audio_umac_clk_parents, 0x0D4C,
		0, 3,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const audio_ihb_clk_parents[] = { "pll-video0-4x", "pll-peri0-600m", "pll-peri1-600m" };

static SUNXI_CCU_M_WITH_MUX_GATE(audio_ihb_clk, "audio-ihb-clk",
		audio_ihb_clk_parents, 0x0D50,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const mpg0_clk_parents[] = { "pll-video0-4x", "pll-video0-1x", "dcxo24M", "pll-adc" };

static SUNXI_CCU_M_WITH_MUX_GATE(mpg0_clk, "mpg0-clk",
		mpg0_clk_parents, 0x0D5C,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const mpg1_clk_parents[] = { "pll-video0-4x", "dcxo24M", "pll-adc" };

static SUNXI_CCU_M_WITH_MUX_GATE(mpg1_clk, "mpg1-clk",
		mpg1_clk_parents, 0x0D60,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(demod_clk, "demod",
		"dcxo24M",
		0x0D64, BIT(0), 0);

static const char * const tcd3_clk_parents[] = { "pll-video0-4x", "pll-video0-1x", "pll-adc" };

static SUNXI_CCU_M_WITH_MUX_GATE(tcd3_clk, "tcd3-clk",
		tcd3_clk_parents, 0x0D6C,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const vincap_dma_parents[] = { "pll-video2-4x", "pll-peri0-600m" };

static SUNXI_CCU_MUX_WITH_GATE(vincap_dma_clk, "vincap-dma",
		vincap_dma_parents, 0x0D74,
		24, 1,	/* mux */
		BIT(31), 0);

static const char * const hdmi_audio_parents[] = { "pll-video3-4x", "pll-peri0-2x" };

static SUNXI_CCU_MUX_WITH_GATE(hdmi_audio_clk, "hdmi-audio",
		hdmi_audio_parents, 0x0D84,
		24, 1,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(tvcap_clk, "tvcap",
		"dcxo24M",
		0x0D88, BIT(0), 0);

static const char * const deint_parents[] = { "pll-video2-4x", "lvds-clk" };

static SUNXI_CCU_MUX_WITH_GATE(deint_clk, "deint",
		deint_parents, 0x0DB0,
		24, 1,	/* mux */
		BIT(31), 0);

static const char * const panel_parents[] = { "pll-video2-4x", "lvds-clk" };

static SUNXI_CCU_MUX_WITH_GATE(panel_clk, "panel",
		panel_parents, 0x0DB4,
		24, 1,	/* mux */
		BIT(31), 0);

static const char * const svp_dtl_clk_parents[] = { "pll-peri0-2x", "pll-video0-4x", "pll-peri1-2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(svp_dtl_clk_clk, "svp-dtl-clk",
		svp_dtl_clk_parents, 0x0DB8,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const ksc_parents[] = { "pll-peri0-480m", "pll-peri0-800m", "pll-video0-4x", "pll-video2-4x", "pll-video3-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(ksc_clk, "ksc-clk",
		ksc_parents, 0x0DBC,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const afbd_clk_parents[] = { "pll-peri0-800m", "pll-peri0-600m", "pll-peri0-480m", "pll-video0-4x", "pll-adc" };

static SUNXI_CCU_M_WITH_MUX_GATE(afbd_clk, "afbd-clk",
		afbd_clk_parents, 0x0DC0,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(disp_clk, "disp",
		"dcxo24M",
		0x0DD8, BIT(0), 0);

/* ccu_des_end */

/* rst_def_start */
static struct ccu_reset_map sun55iw5_ccu_resets[] = {
	[RST_MBUS]			= { 0x0540, BIT(30) },
	[RST_BUS_GPU]			= { 0x067c, BIT(16) },
	[RST_BUS_CE_SY]			= { 0x068c, BIT(17) },
	[RST_BUS_CE]			= { 0x068c, BIT(16) },
	[RST_BUS_AV1]			= { 0x069c, BIT(17) },
	[RST_BUS_VE]			= { 0x069c, BIT(16) },
	[RST_BUS_DMA]			= { 0x070c, BIT(16) },
	[RST_BUS_MSGBOX]		= { 0x071c, BIT(16) },
	[RST_BUS_SPINLOCK]		= { 0x072c, BIT(16) },
	[RST_BUS_TIMER0]		= { 0x0750, BIT(16) },
	[RST_BUS_DBGSY]			= { 0x078c, BIT(16) },
	[RST_BUS_PWM]			= { 0x07ac, BIT(16) },
	[RST_BUS_DRAM_MODULE]		= { 0x0800, BIT(30) },
	[RST_BUS_DRAM]			= { 0x080c, BIT(16) },
	[RST_BUS_SMHC2]			= { 0x084c, BIT(18) },
	[RST_BUS_SMHC1]			= { 0x084c, BIT(17) },
	[RST_BUS_SMHC0]			= { 0x084c, BIT(16) },
	[RST_BUS_UART3]			= { 0x090c, BIT(19) },
	[RST_BUS_UART2]			= { 0x090c, BIT(18) },
	[RST_BUS_UART1]			= { 0x090c, BIT(17) },
	[RST_BUS_UART0]			= { 0x090c, BIT(16) },
	[RST_BUS_TWI4]			= { 0x091c, BIT(20) },
	[RST_BUS_TWI3]			= { 0x091c, BIT(19) },
	[RST_BUS_TWI2]			= { 0x091c, BIT(18) },
	[RST_BUS_TWI1]			= { 0x091c, BIT(17) },
	[RST_BUS_TWI0]			= { 0x091c, BIT(16) },
	[RST_BUS_SPI2]			= { 0x096c, BIT(18) },
	[RST_BUS_SPI1]			= { 0x096c, BIT(17) },
	[RST_BUS_SPI0]			= { 0x096c, BIT(16) },
	[RST_BUS_GMAC0]			= { 0x097c, BIT(16) },
	[RST_BUS_I2SPCM0]		= { 0x0a20, BIT(16) },
	[RST_BUS_SPDIF1]		= { 0x0a2c, BIT(17) },
	[RST_BUS_SPDIF0]		= { 0x0a2c, BIT(16) },
	[RST_BUS_AUDIO_CODEC]		= { 0x0a6c, BIT(16) },
	[RST_USB_PHY0_RSTN]		= { 0x0a70, BIT(30) },
	[RST_USB_PHY1_RSTN]		= { 0x0a78, BIT(30) },
	[RST_USB_PHY2_RSTN]		= { 0x0a80, BIT(30) },
	[RST_USB_OTG]			= { 0x0a8c, BIT(24) },
	[RST_USB_EHCI2]			= { 0x0a8c, BIT(22) },
	[RST_USB_EHCI1]			= { 0x0a8c, BIT(21) },
	[RST_USB_EHCI0]			= { 0x0a8c, BIT(20) },
	[RST_USB_OHCI2]			= { 0x0a8c, BIT(18) },
	[RST_USB_OHCI1]			= { 0x0a8c, BIT(17) },
	[RST_USB_OHCI0]			= { 0x0a8c, BIT(16) },
	[RST_BUS_LVD]			= { 0x0bac, BIT(16) },
	[RST_BUS_TVFE_AXI]		= { 0x0d0c, BIT(16) },
	[RST_BUS_DEMOD]			= { 0x0d64, BIT(16) },
	[RST_BUS_TVCAP]			= { 0x0d88, BIT(16) },
	[RST_BUS_DISP]			= { 0x0dd8, BIT(16) },
	/* rst_def_end */
};

/* ccu_def_start */
static struct clk_hw_onecell_data sun55iw5_hw_clks = {
	.hws    = {
		[CLK_PLL_DDR]			= &pll_ddr_clk.common.hw,
		[CLK_PLL_PERI0_PARENT]		= &pll_peri0_parent_clk.common.hw,
		[CLK_PLL_PERI0_2X]		= &pll_peri0_2x_clk.common.hw,
		[CLK_PERI0_DIV3]		= &pll_peri0_div3_clk.hw,
		[CLK_PLL_PERI0_800M]		= &pll_peri0_800m_clk.common.hw,
		[CLK_PLL_PERI0_480M]		= &pll_peri0_480m_clk.common.hw,
		[CLK_PLL_PERI0_600M]		= &pll_peri0_600m_clk.hw,
		[CLK_PLL_PERI0_400M]		= &pll_peri0_400m_clk.hw,
		[CLK_PLL_PERI0_300M]		= &pll_peri0_300m_clk.hw,
		[CLK_PLL_PERI0_200M]		= &pll_peri0_200m_clk.hw,
		[CLK_PLL_PERI0_160M]		= &pll_peri0_160m_clk.hw,
		[CLK_PLL_PERI0_16M]		= &pll_peri0_16m_clk.hw,
		[CLK_PLL_PERI0_150M]		= &pll_peri0_150m_clk.hw,
		[CLK_PLL_PERI0_25M]		= &pll_peri0_25m_clk.hw,
		[CLK_PLL_PERI1_PARENT]		= &pll_peri1_parent_clk.common.hw,
		[CLK_PLL_PERI1_2X]		= &pll_peri1_2x_clk.common.hw,
		[CLK_PLL_PERI1_800M]		= &pll_peri1_800m_clk.common.hw,
		[CLK_PLL_PERI1_480M]		= &pll_peri1_480m_clk.common.hw,
		[CLK_PLL_PERI1_600M]		= &pll_peri1_600m_clk.hw,
		[CLK_PLL_PERI1_400M]		= &pll_peri1_400m_clk.hw,
		[CLK_PLL_PERI1_300M]		= &pll_peri1_300m_clk.hw,
		[CLK_PLL_PERI1_200M]		= &pll_peri1_200m_clk.hw,
		[CLK_PLL_PERI1_160M]		= &pll_peri1_160m_clk.hw,
		[CLK_PLL_PERI1_150M]		= &pll_peri1_150m_clk.hw,
		[CLK_PLL_GPU]			= &pll_gpu_clk.common.hw,
		[CLK_PLL_VIDEO0_4X]		= &pll_video0_4x_clk.common.hw,
		[CLK_PLL_VIDEO2_4X]		= &pll_video2_4x_clk.common.hw,
		[CLK_PLL_VE]			= &pll_ve_clk.common.hw,
		[CLK_PLL_ADC]			= &pll_adc_clk.common.hw,
		[CLK_PLL_VIDEO3_4X]		= &pll_video3_4x_clk.common.hw,
		[CLK_AHB]			= &ahb_clk.common.hw,
		[CLK_APB0]			= &apb0_clk.common.hw,
		[CLK_APB1]			= &apb1_clk.common.hw,
		[CLK_MBUS]			= &mbus_clk.common.hw,
		[CLK_TRACE]			= &trace_clk.common.hw,
		[CLK_GIC]			= &gic_clk.common.hw,
		[CLK_TVDISP_AHB_GATE]		= &tvdisp_ahb_gate_clk.common.hw,
		[CLK_TVCAP_AHB_GATE]		= &tvcap_ahb_gate_clk.common.hw,
		[CLK_TVFE_AHB_GATE]		= &tvfe_ahb_gate_clk.common.hw,
		[CLK_SMHC2_AHB_GATE]		= &smhc2_ahb_gate_clk.common.hw,
		[CLK_SMHC1_AHB_GATE]		= &smhc1_ahb_gate_clk.common.hw,
		[CLK_SMHC0_AHB_GATE]		= &smhc0_ahb_gate_clk.common.hw,
		[CLK_USB2_AHB_GATE]		= &usb2_ahb_gate_clk.common.hw,
		[CLK_GMAC_AHB_GATE]		= &gmac_ahb_gate_clk.common.hw,
		[CLK_GPU_AHB_GATE]		= &gpu_ahb_gate_clk.common.hw,
		[CLK_TVDISP_MBUS_GATE]		= &tvdisp_mbus_gate_clk.common.hw,
		[CLK_TVCAP_MBUS_GATE]		= &tvcap_mbus_gate_clk.common.hw,
		[CLK_TVFE_MBUS_GATE]		= &tvfe_mbus_gate_clk.common.hw,
		[CLK_GPU_MBUS_GATE]		= &gpu_mbus_gate_clk.common.hw,
		[CLK_GPU]			= &gpu_clk.common.hw,
		[CLK_CE]			= &ce_clk.common.hw,
		[CLK_CE_SYS]			= &ce_sys_clk.common.hw,
		[CLK_BUS_CE]			= &ce_bus_clk.common.hw,
		[CLK_VE_CORE]			= &ve_core_clk.common.hw,
		[CLK_AV1]			= &av1_clk.common.hw,
		[CLK_VE]			= &ve_clk.common.hw,
		[CLK_DMA]			= &dma_clk.common.hw,
		[CLK_MSGBOX]			= &msgbox_clk.common.hw,
		[CLK_SPINLOCK]			= &spinlock_clk.common.hw,
		[CLK_TIMER0]			= &timer0_clk.common.hw,
		[CLK_TIMER1]			= &timer1_clk.common.hw,
		[CLK_TIMER2]			= &timer2_clk.common.hw,
		[CLK_BUS_TIMER0]		= &timer0_bus_clk.common.hw,
		[CLK_DBGSYS]			= &dbgsys_clk.common.hw,
		[CLK_PWM]			= &pwm_clk.common.hw,
		[CLK_IOMMU]			= &iommu_clk.common.hw,
		[CLK_DRAM]			= &dram_clk.common.hw,
		[CLK_MBUS_AV1]			= &av1_mbus_clk.common.hw,
		[CLK_MBUS_CE]			= &ce_mbus_clk.common.hw,
		[CLK_MBUS_VE3]			= &ve3_mbus_clk.common.hw,
		[CLK_MBUS_DMA]			= &dma_mbus_clk.common.hw,
		[CLK_MBUS_DRAM]			= &dram_mbus_clk.common.hw,
		[CLK_SMHC0]			= &smhc0_clk.common.hw,
		[CLK_SMHC1]			= &smhc1_clk.common.hw,
		[CLK_SMHC2]			= &smhc2_clk.common.hw,
		[CLK_BUS_SMHC2]			= &smhc2_bus_clk.common.hw,
		[CLK_BUS_SMHC1]			= &smhc1_bus_clk.common.hw,
		[CLK_BUS_SMHC0]			= &smhc0_bus_clk.common.hw,
		[CLK_BUS_UART3]			= &uart3_clk.common.hw,
		[CLK_BUS_UART2]			= &uart2_clk.common.hw,
		[CLK_BUS_UART1]			= &uart1_clk.common.hw,
		[CLK_BUS_UART0]			= &uart0_clk.common.hw,
		[CLK_TWI4]			= &twi4_clk.common.hw,
		[CLK_TWI3]			= &twi3_clk.common.hw,
		[CLK_TWI2]			= &twi2_clk.common.hw,
		[CLK_TWI1]			= &twi1_clk.common.hw,
		[CLK_TWI0]			= &twi0_clk.common.hw,
		[CLK_SPI0]			= &spi0_clk.common.hw,
		[CLK_SPI1]			= &spi1_clk.common.hw,
		[CLK_SPI2]			= &spi2_clk.common.hw,
		[CLK_BUS_SPI2]			= &spi2_bus_clk.common.hw,
		[CLK_BUS_SPI1]			= &spi1_bus_clk.common.hw,
		[CLK_BUS_SPI0]			= &spi0_bus_clk.common.hw,
		[CLK_GMAC0_25M]			= &gmac0_phy_clk.common.hw,
		[CLK_GMAC0]			= &gmac0_clk.common.hw,
		[CLK_I2SPCM0]			= &i2spcm0_clk.common.hw,
		[CLK_BUS_I2SPCM0]		= &i2spcm0_bus_clk.common.hw,
		[CLK_SPDIF1]			= &spdif1_clk.common.hw,
		[CLK_SPDIF0]			= &spdif0_clk.common.hw,
		[CLK_SPDIF0_RX]			= &spdif0_rx_clk.common.hw,
		[CLK_SPDIF0_TX]			= &spdif0_tx_clk.common.hw,
		[CLK_SPDIF1_RX]			= &spdif1_rx_clk.common.hw,
		[CLK_SPDIF1_TX]			= &spdif1_tx_clk.common.hw,
		[CLK_AUDIO_CODEC_DAC_1X]	= &audio_codec_dac_1x_clk.common.hw,
		[CLK_AUDIO_CODEC_ADC_1X]	= &audio_codec_adc_1x_clk.common.hw,
		[CLK_AUDIO_CODEC]		= &audio_codec_clk.common.hw,
		[CLK_USB0]			= &usb0_clk.common.hw,
		[CLK_USB1]			= &usb1_clk.common.hw,
		[CLK_USB2]			= &usb2_clk.common.hw,
		[CLK_USBOTG]			= &usbotg_clk.common.hw,
		[CLK_USBEHCI2]			= &usbehci2_clk.common.hw,
		[CLK_USBEHCI1]			= &usbehci1_clk.common.hw,
		[CLK_USBEHCI0]			= &usbehci0_clk.common.hw,
		[CLK_USBOHCI2]			= &usbohci2_clk.common.hw,
		[CLK_USBOHCI1]			= &usbohci1_clk.common.hw,
		[CLK_USBOHCI0]			= &usbohci0_clk.common.hw,
		[CLK_TVFE_AXI]			= &tvfe_axi_clk.common.hw,
		[CLK_ADC]			= &adc_clk.common.hw,
		[CLK_TSDM_TS]			= &tsdm_ts_clk.common.hw,
		[CLK_DTMB_CLK120M]		= &dtmb_clk120m_clk.common.hw,
		[CLK_TVFE_1296M]		= &tvfe_1296m_clk.common.hw,
		[CLK_I2H]			= &i2h_clk_clk.common.hw,
		[CLK_AUDIO_CPU]			= &audio_cpu_clk.common.hw,
		[CLK_AUDIO_UMAC]		= &audio_umac_clk.common.hw,
		[CLK_AUDIO_IHB]			= &audio_ihb_clk.common.hw,
		[CLK_MPG0]			= &mpg0_clk.common.hw,
		[CLK_MPG1]			= &mpg1_clk.common.hw,
		[CLK_DEMOD]			= &demod_clk.common.hw,
		[CLK_TCD3]			= &tcd3_clk.common.hw,
		[CLK_VINCAP_DMA]		= &vincap_dma_clk.common.hw,
		[CLK_HDMI_AUDIO]		= &hdmi_audio_clk.common.hw,
		[CLK_TVCAP]			= &tvcap_clk.common.hw,
		[CLK_DEINT]			= &deint_clk.common.hw,
		[CLK_PANEL]			= &panel_clk.common.hw,
		[CLK_SVP_DTL]			= &svp_dtl_clk_clk.common.hw,
		[CLK_KSC]			= &ksc_clk.common.hw,
		[CLK_AFBD]			= &afbd_clk.common.hw,
		[CLK_DISP]			= &disp_clk.common.hw,
	},
	.num = CLK_NUMBER,
};
/* ccu_def_end */

static struct ccu_common *sun55iw5_ccu_clks[] = {
	&pll_ddr_clk.common,
	&pll_peri0_parent_clk.common,
	&pll_peri0_2x_clk.common,
	&pll_peri0_800m_clk.common,
	&pll_peri0_480m_clk.common,
	&pll_peri1_parent_clk.common,
	&pll_peri1_2x_clk.common,
	&pll_peri1_800m_clk.common,
	&pll_peri1_480m_clk.common,
	&pll_gpu_clk.common,
	&pll_video0_4x_clk.common,
	&pll_video2_4x_clk.common,
	&pll_ve_clk.common,
	&pll_adc_clk.common,
	&pll_video3_4x_clk.common,
	&ahb_clk.common,
	&apb0_clk.common,
	&apb1_clk.common,
	&mbus_clk.common,
	&trace_clk.common,
	&gic_clk.common,
	&tvdisp_ahb_gate_clk.common,
	&tvcap_ahb_gate_clk.common,
	&tvfe_ahb_gate_clk.common,
	&smhc2_ahb_gate_clk.common,
	&smhc1_ahb_gate_clk.common,
	&smhc0_ahb_gate_clk.common,
	&usb2_ahb_gate_clk.common,
	&gmac_ahb_gate_clk.common,
	&gpu_ahb_gate_clk.common,
	&tvdisp_mbus_gate_clk.common,
	&tvcap_mbus_gate_clk.common,
	&tvfe_mbus_gate_clk.common,
	&gpu_mbus_gate_clk.common,
	&gpu_clk.common,
	&ce_clk.common,
	&ce_sys_clk.common,
	&ce_bus_clk.common,
	&ve_core_clk.common,
	&av1_clk.common,
	&ve_clk.common,
	&dma_clk.common,
	&msgbox_clk.common,
	&spinlock_clk.common,
	&timer0_clk.common,
	&timer1_clk.common,
	&timer2_clk.common,
	&timer0_bus_clk.common,
	&dbgsys_clk.common,
	&pwm_clk.common,
	&iommu_clk.common,
	&dram_clk.common,
	&av1_mbus_clk.common,
	&ce_mbus_clk.common,
	&ve3_mbus_clk.common,
	&dma_mbus_clk.common,
	&dram_mbus_clk.common,
	&smhc0_clk.common,
	&smhc1_clk.common,
	&smhc2_clk.common,
	&smhc2_bus_clk.common,
	&smhc1_bus_clk.common,
	&smhc0_bus_clk.common,
	&uart3_clk.common,
	&uart2_clk.common,
	&uart1_clk.common,
	&uart0_clk.common,
	&twi4_clk.common,
	&twi3_clk.common,
	&twi2_clk.common,
	&twi1_clk.common,
	&twi0_clk.common,
	&spi0_clk.common,
	&spi1_clk.common,
	&spi2_clk.common,
	&spi2_bus_clk.common,
	&spi1_bus_clk.common,
	&spi0_bus_clk.common,
	&gmac0_phy_clk.common,
	&gmac0_clk.common,
	&i2spcm0_clk.common,
	&i2spcm0_bus_clk.common,
	&spdif1_clk.common,
	&spdif0_clk.common,
	&spdif0_rx_clk.common,
	&spdif0_tx_clk.common,
	&spdif1_rx_clk.common,
	&spdif1_tx_clk.common,
	&audio_codec_dac_1x_clk.common,
	&audio_codec_adc_1x_clk.common,
	&audio_codec_clk.common,
	&usb0_clk.common,
	&usb1_clk.common,
	&usb2_clk.common,
	&usbotg_clk.common,
	&usbehci2_clk.common,
	&usbehci1_clk.common,
	&usbehci0_clk.common,
	&usbohci2_clk.common,
	&usbohci1_clk.common,
	&usbohci0_clk.common,
	&tvfe_axi_clk.common,
	&adc_clk.common,
	&tsdm_ts_clk.common,
	&dtmb_clk120m_clk.common,
	&tvfe_1296m_clk.common,
	&i2h_clk_clk.common,
	&audio_cpu_clk.common,
	&audio_umac_clk.common,
	&audio_ihb_clk.common,
	&mpg0_clk.common,
	&mpg1_clk.common,
	&demod_clk.common,
	&tcd3_clk.common,
	&vincap_dma_clk.common,
	&hdmi_audio_clk.common,
	&tvcap_clk.common,
	&deint_clk.common,
	&panel_clk.common,
	&svp_dtl_clk_clk.common,
	&ksc_clk.common,
	&afbd_clk.common,
	&disp_clk.common,
};

static const struct sunxi_ccu_desc sun55iw5_ccu_desc = {
	.ccu_clks	= sun55iw5_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun55iw5_ccu_clks),

	.hw_clks	= &sun55iw5_hw_clks,

	.resets		= sun55iw5_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun55iw5_ccu_resets),
};

static const u32 sun55iw5_pll_regs[] = {
	SUN55IW5_PLL_DDR_CTRL_REG,
	SUN55IW5_PLL_PERI0_CTRL_REG,
	SUN55IW5_PLL_PERI1_CTRL_REG,
	SUN55IW5_PLL_GPU_CTRL_REG,
	SUN55IW5_PLL_VIDEO0_CTRL_REG,
	SUN55IW5_PLL_VIDEO2_CTRL_REG,
	SUN55IW5_PLL_VE_CTRL_REG,
	SUN55IW5_PLL_VIDEO3_CTRL_REG,
};

static int sun55iw5_ccu_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct device *dev = &pdev->dev;
	void __iomem *reg;
	int i, ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "Fail to get IORESOURCE_MEM\n");
		return -EINVAL;
	}

	reg = devm_ioremap(dev, res->start, resource_size(res));
	if (IS_ERR(reg)) {
		dev_err(dev, "Fail to map IO resource\n");
		return PTR_ERR(reg);
	}

	/* Enable the pll_en/ldo_en/lock_en bits on all PLLs */
	for (i = 0; i < ARRAY_SIZE(sun55iw5_pll_regs); i++) {
		set_reg(reg + sun55iw5_pll_regs[i], 0x7, 3, 29);
	}

	ret = sunxi_ccu_probe(pdev->dev.of_node, reg, &sun55iw5_ccu_desc);
	if (ret)
		return ret;

	sunxi_ccu_sleep_init(reg, sun55iw5_ccu_clks,
			ARRAY_SIZE(sun55iw5_ccu_clks),
			NULL, 0);

	return 0;
}

static const struct of_device_id sun55iw5_ccu_ids[] = {
	{ .compatible = "allwinner,sun55iw5-ccu" },
	{ }
};

static struct platform_driver sun55iw5_ccu_driver = {
	.probe	= sun55iw5_ccu_probe,
	.driver	= {
		.name	= "sun55iw5-ccu",
		.of_match_table	= sun55iw5_ccu_ids,
	},
};

static int __init sun55iw5_ccu_init(void)
{
	int err;

	err = platform_driver_register(&sun55iw5_ccu_driver);
	if (err)
		pr_err("register ccu sun55iw5 failed\n");

	return err;
}

core_initcall(sun55iw5_ccu_init);

static void __exit sun55iw5_ccu_exit(void)
{
	platform_driver_unregister(&sun55iw5_ccu_driver);
}
module_exit(sun55iw5_ccu_exit);

MODULE_DESCRIPTION("Allwinner sun55iw5 clk driver");
MODULE_AUTHOR("rgm<rengaomin@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.5.1");
