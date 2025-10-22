// SPDX-License-Identifier: GPL-3.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 panzhijian@allwinnertech.com
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

#include "ccu-sun55iw6.h"

/*
 * The CPU PLL is actually NP clock, with P being /1, /2 or /4. However
 * P should only be used for output frequencies lower than 288 MHz.
 *
 * For now we can just model it as a multiplier clock, and force P to /1.
 *
 * The M factor is present in the register's description, but not in the
 * frequency formula, and it's documented as "M is only used for backdoor
 * testing", so it's not modelled and then force to 0.
 */

/* ccu_des_start */

#define SUN55IW6_PLL_DDR_CTRL_REG   0x0020
static struct ccu_nkmp pll_ddr_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.common		= {
		.reg		= 0x0020,
		.hw.init	= CLK_HW_INIT("pll-ddr", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN55IW6_PLL_PERI0_CTRL_REG   0x00A0
static struct ccu_nm pll_peri0_parent_clk = {
	.enable		= BIT(27) | BIT(30) | BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.common		= {
		.reg		= 0x00A0,
		.hw.init	= CLK_HW_INIT("pll-peri0-parent", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static SUNXI_CCU_M(pll_peri0_2x_clk, "pll-peri0-2x",
		"pll-peri0-parent", 0x00A0, 16, 3, 0);

static CLK_FIXED_FACTOR_HW(pll_peri0_div3_clk, "pll-peri0-div3",
		&pll_peri0_2x_clk.common.hw,
		6, 1, 0);

static SUNXI_CCU_M(pll_peri0_800m_clk, "pll-peri0-800m",
		"pll-peri0-parent", 0x00A0, 20, 3, 0);

static SUNXI_CCU_M(pll_peri0_480m_clk, "pll-peri0-480m",
		"pll-peri0-parent", 0x00A0, 2, 3, 0);

static CLK_FIXED_FACTOR_HW(pll_peri0_600m_clk, "pll-peri0-600m",
		&pll_peri0_2x_clk.common.hw,
		2, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri0_400m_clk, "pll-pll-peri0-400m",
		&pll_peri0_2x_clk.common.hw,
		3, 1, 0);

static CLK_FIXED_FACTOR(pll_peri0_300m_clk, "pll-peri0-300m",
		"pll-peri0-600m",
		2, 1, 0);

static CLK_FIXED_FACTOR(pll_peri0_200m_clk, "pll-peri0-200m",
		"pll-pll-peri0-400m",
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

#define SUN55IW6_PLL_PERI1_CTRL_REG   0x00C0
static struct ccu_nm pll_peri1_parent_clk = {
	.enable		= BIT(27) | BIT(30) | BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.common		= {
		.reg		= 0x00C0,
		.hw.init	= CLK_HW_INIT("pll-peri1-parent", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static SUNXI_CCU_M(pll_peri1_2x_clk, "pll-peri1-2x",
		"pll-peri1-parent", 0x00C0, 16, 3, 0);

static SUNXI_CCU_M(pll_peri1_800m_clk, "pll-peri1-800m",
		"pll-peri1-parent", 0x00C0, 20, 3, 0);

static SUNXI_CCU_M(pll_peri1_480m_clk, "pll-peri1-480m",
		"pll-peri1-parent", 0x00C0, 2, 3, 0);

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

#define SUN55IW6_PLL_VIDEO0_CTRL_REG   0x0120
static struct ccu_nkmp pll_video0_4x = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.common		= {
		.reg		= 0x0120,
		.hw.init	= CLK_HW_INIT("pll-video0-4x", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN55IW6_PLL_VIDEO1_CTRL_REG   0x0140
static struct ccu_nkmp pll_video1_4x = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.common		= {
		.reg		= 0x0140,
		.hw.init	= CLK_HW_INIT("pll-video1_4x", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN55IW6_PLL_VE_CTRL_REG   0x0220
static struct ccu_nkmp pll_ve_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.common		= {
		.reg		= 0x0220,
		.hw.init	= CLK_HW_INIT("pll-ve", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN55IW6_PLL_NPU_CTRL_REG   0x02A0
static struct ccu_nm pll_npu_4x_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* output divider */
	.common		= {
		.reg		= 0x02A0,
		.hw.init	= CLK_HW_INIT("pll-npu-4x", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static const char * const ahb_parents[] = { "dcxo24M", "osc32k", "iosc", "pll-peri0-600m" };

SUNXI_CCU_M_WITH_MUX(ahb_clk, "ahb", ahb_parents,
		0x0500, 0, 5, 24, 2, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);

static const char * const apb0_parents[] = { "dcxo24M", "osc32k", "iosc", "pll-peri0-600m" };

SUNXI_CCU_M_WITH_MUX(apb0_clk, "apb0", apb0_parents,
		0x0510, 0, 5, 24, 2, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);

static const char * const apb1_parents[] = { "dcxo24M", "osc32k", "iosc", "pll-peri0-600m", "pll-peri0-480m" };

static SUNXI_CCU_M_WITH_MUX(apb1_clk, "apb1", apb1_parents,
		0x0518, 0, 5, 24, 2, CLK_SET_RATE_PARENT);

static const char * const apb_uart_parents[] = { "dcxo24M", "osc32k", "iosc", "pll-peri0-600m", "pll-peri0-480m" };

static SUNXI_CCU_M_WITH_MUX(apb_uart_clk, "apb-uart", apb_uart_parents,
		0x0518, 0, 5, 24, 2, 0);

static const char * const trace_parents[] = { "dcxo24M", "osc32k", "rc-16m", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(trace_clk, "trace",
		trace_parents, 0x0540,
		0, 5,	/* M */
		24, 2,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const gic_parents[] = { "dcxo24M", "osc32k", "rc-16m", "pll-peri0-600m", "pll-peri0-480m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE(gic_clk, "gic",
		gic_parents, 0x0560,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(its0_aclk, "its0-aclk",
		"dcxo24M",
		0x0574, BIT(1), 0);

static SUNXI_CCU_GATE(its0_hclk, "its0-hclk",
		"dcxo24M",
		0x0574, BIT(0), 0);

static const char * const nsi_parents[] = { "dcxo24M", "pll-ddr", "pll-peri0-600m", "pll-peri0-480m", "pll-peri0-400m", "pll-npu-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(nsi_clk, "nsi",
		nsi_parents, 0x0580,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(nsi_cfg_clk, "nsi-cfg",
		"dcxo24M",
		0x0584, BIT(0), 0);

static const char * const mbus_parents[] = { "dcxo24M", "pll-peri0-600m-bus", "pll-peri0-480m", "pll-peri0-400m", "pll-ddr", "pll-npu-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(mbus_clk, "mbus",
		mbus_parents, 0x0588,
		0, 5,
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(iommu_clk, "iommu",
		"dcxo24M",
		0x058C, BIT(0), 0);

static SUNXI_CCU_GATE(gmac1_mbus_gate_clk, "gmac1-mbus-gate",
		"dcxo24M",
		0x05E0, BIT(12), 0);

static SUNXI_CCU_GATE(gmac0_mbus_gate_clk, "gmac0-mbus-gate",
		"dcxo24M",
		0x05E0, BIT(11), 0);

static SUNXI_CCU_GATE(isp_mbus_gate_clk, "isp-mbus-gate",
		"dcxo24M",
		0x05E0, BIT(9), 0);

static SUNXI_CCU_GATE(csi_mbus_gate_clk, "csi-mbus-gate",
		"dcxo24M",
		0x05E0, BIT(8), 0);

static SUNXI_CCU_GATE(nand_mbus_gate_clk, "nand-mbus-gate",
		"dcxo24M",
		0x05E0, BIT(5), 0);

static SUNXI_CCU_GATE(dma1_mbus_gate_clk, "dma1-mbus-gate",
		"dcxo24M",
		0x05E0, BIT(3), 0);

static SUNXI_CCU_GATE(ce_mbus_gate_clk, "ce-mbus-gate",
		"dcxo24M",
		0x05E0, BIT(2), 0);

static SUNXI_CCU_GATE(ve_mbus_gate_clk, "ve-mbus-gate",
		"dcxo24M",
		0x05E0, BIT(1), 0);

static SUNXI_CCU_GATE(dma0_mbus_gate_clk, "dma0-mbus-gate",
		"dcxo24M",
		0x05E0, BIT(0), 0);

static SUNXI_CCU_GATE(dma0_clk, "dma0",
		"dcxo24M",
		0x0704, BIT(0), 0);

static SUNXI_CCU_GATE(dma1_clk, "dma1",
		"dcxo24M",
		0x070C, BIT(0), 0);

static SUNXI_CCU_GATE(spinlock_clk, "spinlock",
		"dcxo24M",
		0x0724, BIT(0), 0);

static SUNXI_CCU_GATE(msgbox0_clk, "msgbox0",
		"dcxo24M",
		0x0744, BIT(0), 0);

static SUNXI_CCU_GATE(msgbox_core0_clk, "msgbox-core0",
		"dcxo24M",
		0x074C, BIT(0), 0);

static SUNXI_CCU_GATE(msgbox_core1_clk, "msgbox-core1",
		"dcxo24M",
		0x0754, BIT(0), 0);

static SUNXI_CCU_GATE(msgbox_core2_clk, "msgbox-core2",
		"dcxo24M",
		0x075C, BIT(0), 0);

static SUNXI_CCU_GATE(msgbox_core3_clk, "msgbox-core3",
		"dcxo24M",
		0x0764, BIT(0), 0);

static SUNXI_CCU_GATE(msgbox_rv_clk, "msgbox-rv",
		"dcxo24M",
		0x076C, BIT(0), 0);

static SUNXI_CCU_GATE(pwm0_clk, "pwm0",
		"dcxo24M",
		0x0784, BIT(0), 0);

static SUNXI_CCU_GATE(pwm1_clk, "pwm1",
		"dcxo24M",
		0x078C, BIT(0), 0);

static SUNXI_CCU_GATE(pwm2_clk, "pwm2",
		"dcxo24M",
		0x0794, BIT(0), 0);

static SUNXI_CCU_GATE(dbgsys_clk, "dbgsys",
		"dcxo24M",
		0x07A4, BIT(0), 0);

static SUNXI_CCU_GATE(sysdap_clk, "sysdap",
		"dcxo24M",
		0x07AC, BIT(0), 0);

static const char * const timer0_clk_parents[] = { "dcxo24M", "rc-16m", "osc32k", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(timer0_clk, "timer0-clk",
		timer0_clk_parents, 0x0800,
		0, 3,
		24, 3,   /* mux */
		BIT(0), /* gate */
		0);

static SUNXI_CCU_M_WITH_MUX_GATE(timer1_clk, "timer1-clk",
		timer0_clk_parents, 0x0804,
		0, 3,
		24, 3,   /* mux */
		BIT(0), /* gate */
		0);

static SUNXI_CCU_M_WITH_MUX_GATE(timer2_clk, "timer2-clk",
		timer0_clk_parents, 0x0808,
		0, 3,
		24, 3,   /* mux */
		BIT(0), /* gate */
		0);

static SUNXI_CCU_M_WITH_MUX_GATE(timer3_clk, "timer3-clk",
		timer0_clk_parents, 0x080c,
		0, 3,
		24, 3,   /* mux */
		BIT(0), /* gate */
		0);

static SUNXI_CCU_M_WITH_MUX_GATE(timer4_clk, "timer4-clk",
		timer0_clk_parents, 0x0810,
		0, 3,
		24, 3,   /* mux */
		BIT(0), /* gate */
		0);

static SUNXI_CCU_M_WITH_MUX_GATE(timer5_clk, "timer5-clk",
		timer0_clk_parents, 0x0814,
		0, 3,
		24, 3,   /* mux */
		BIT(0), /* gate */
		0);

static SUNXI_CCU_M_WITH_MUX_GATE(timer6_clk, "timer6-clk",
		timer0_clk_parents, 0x0818,
		0, 3,
		24, 3,   /* mux */
		BIT(0), /* gate */
		0);

static SUNXI_CCU_M_WITH_MUX_GATE(timer7_clk, "timer7-clk",
		timer0_clk_parents, 0x081c,
		0, 3,
		24, 3,   /* mux */
		BIT(0), /* gate */
		0);

static SUNXI_CCU_GATE(timer_bus_clk, "timer",
		"dcxo24M",
		0x0850, BIT(0), 0);

static SUNXI_CCU_M_WITH_MUX_GATE(timer0_rv_clk, "timer0-rv-clk",
		timer0_clk_parents, 0x0860,
		0, 3,
		24, 3,   /* mux */
		BIT(0), /* gate */
		0);

static SUNXI_CCU_M_WITH_MUX_GATE(timer1_rv_clk, "timer1-rv-clk",
		timer0_clk_parents, 0x0864,
		0, 3,
		24, 3,   /* mux */
		BIT(0), /* gate */
		0);

static SUNXI_CCU_M_WITH_MUX_GATE(timer2_rv_clk, "timer2-rv-clk",
		timer0_clk_parents, 0x0868,
		0, 3,
		24, 3,   /* mux */
		BIT(0), /* gate */
		0);

static SUNXI_CCU_M_WITH_MUX_GATE(timer3_rv_clk, "timer3-rv-clk",
		timer0_clk_parents, 0x086c,
		0, 3,
		24, 3,   /* mux */
		BIT(0), /* gate */
		0);

static SUNXI_CCU_GATE(timer_rv_bus_clk, "timer-rv",
		"dcxo24M",
		0x0870, BIT(0), 0);

static const char * const de_parents[] = { "pll-peri0-600m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE(de_clk, "de",
		de_parents, 0x0A00,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(de0_clk, "de0",
		"dcxo24M",
		0x0A04, BIT(0), 0);

static const char * const g2d_parents[] = { "pll-peri0-600m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE(g2d_clk, "g2d",
		g2d_parents, 0x0A40,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(g2d_bus_clk, "g2d-bus",
		"dcxo24M",
		0x0A44, BIT(0), 0);

static const char * const ve_parents[] = { "pll-ve", "pll-peri0-480m", "pll-peri0-400m", "pll-peri0-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(ve_clk, "ve",
		ve_parents, 0x0A80,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(ve_bus_clk, "ve-bus",
		"dcxo24M",
		0x0A8C, BIT(0), 0);

static const char * const ce_parents[] = { "dcxo24M", "pll-peri0-400m", "pll-peri0-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(ce_clk, "ce",
		ce_parents, 0x0AC0,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(ce_sys_clk, "ce-sys",
		"dcxo24M",
		0x0AC4, BIT(1), 0);

static SUNXI_CCU_GATE(ce_bus_clk, "ce-bus",
		"dcxo24M",
		0x0AC4, BIT(0), 0);

static const char * const npu_parents[] = { "pll-npu-4x", "pll-peri0-600m", "pll-peri0-480m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE(npu_clk, "npu",
		npu_parents, 0x0B00,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(npu_tzma_clk, "npu-tzma",
		"dcxo24M",
		0x0B04, BIT(1), 0);

static SUNXI_CCU_GATE(npu_bus_clk, "npu-bus",
		"dcxo24M",
		0x0B04, BIT(0), 0);

static const char * const rv_core_parents[] = { "dcxo24M", "rc-16m", "osc32k", "pll-peri0-600m", "pll-peri0-480m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE(rv_core_clk, "rv-core",
		rv_core_parents, 0x0B80,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static const char * const rv_ts_parents[] = { "dcxo24M", "rc-16m", "osc32k" };

/* TODO */
static SUNXI_CCU_M_WITH_MUX_GATE(rv_ts_clk, "rv-ts",
		rv_ts_parents, 0x0B88,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(rv_cfg_clk, "rv-cfg",
		"dcxo24M",
		0x0B9C, BIT(0), 0);

static const char * const dram_parents[] = { "pll-ddr", "pll-peri1-800m", "pll-peri1-600m", "pll-peri1-480m", "pll-npu-4x", "dcxo24M" };

static SUNXI_CCU_M_WITH_MUX_GATE(dram_clk, "dram",
		dram_parents, 0x0C00,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(dram_bus_clk, "dram-bus",
		"dcxo24M",
		0x0C0C, BIT(0), 0);

static const char * const nand0_clk2x_parents[] = { "dcxo24M", "pll-peri0-400m", "pll-peri0-300m", "pll-peri1-400m", "pll-peri1-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(nand0_clk2x_clk, "nand0-clk2x",
		nand0_clk2x_parents, 0x0C80,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const nand0_clk1_parents[] = { "dcxo24M", "pll-peri0-400m", "pll-peri0-300m", "pll-peri1-400m", "pll-peri1-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(nand0_clk, "nand0-clk1",
		nand0_clk1_parents, 0x0C84,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(nand0_bus_clk, "nand0-bus",
		"dcxo24M",
		0x0C8C, BIT(0), 0);

static const char * const smhc0_parents[] = { "dcxo24M", "pll-peri0-400m", "pll-peri0-300m", "pll-peri1-400m", "pll-peri1-300m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc0_clk, "smhc0",
		smhc0_parents, 0x0D00,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(smhc0_bus_clk, "smhc0-bus",
		"dcxo24M",
		0x0D0C, BIT(0), 0);

static const char * const smhc1_parents[] = { "dcxo24M", "pll-peri0-400m", "pll-peri0-300m", "pll-peri1-400m", "pll-peri1-300m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc1_clk, "smhc1",
		smhc1_parents, 0x0D10,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(smhc1_bus_clk, "smhc1-bus",
		"dcxo24M",
		0x0D1C, BIT(0), 0);

static const char * const smhc2_parents[] = { "dcxo24M", "peri0-800m", "pll-peri0-600m", "pll-peri1-800m", "pll-peri1-600m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc2_clk, "smhc2",
		smhc2_parents, 0x0D20,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(smhc2_bus_clk, "smhc2-bus",
		"dcxo24M",
		0x0D2C, BIT(0), 0);

static SUNXI_CCU_GATE(uart0_clk, "uart0",
		"dcxo24M",
		0x0E00, BIT(0), 0);

static SUNXI_CCU_GATE(uart1_clk, "uart1",
		"dcxo24M",
		0x0E04, BIT(0), 0);

static SUNXI_CCU_GATE(uart2_clk, "uart2",
		"dcxo24M",
		0x0E08, BIT(0), 0);

static SUNXI_CCU_GATE(uart3_clk, "uart3",
		"dcxo24M",
		0x0E0C, BIT(0), 0);

static SUNXI_CCU_GATE(uart4_clk, "uart4",
		"dcxo24M",
		0x0E10, BIT(0), 0);

static SUNXI_CCU_GATE(uart5_clk, "uart5",
		"dcxo24M",
		0x0E14, BIT(0), 0);

static SUNXI_CCU_GATE(uart6_clk, "uart6",
		"dcxo24M",
		0x0E18, BIT(0), 0);

static SUNXI_CCU_GATE(uart7_clk, "uart7",
		"dcxo24M",
		0x0E20, BIT(0), 0);

static SUNXI_CCU_GATE(uart8_clk, "uart8",
		"dcxo24M",
		0x0E24, BIT(0), 0);

static SUNXI_CCU_GATE(uart9_clk, "uart9",
		"dcxo24M",
		0x0E28, BIT(0), 0);

static SUNXI_CCU_GATE(uart10_clk, "uart10",
		"dcxo24M",
		0x0E2C, BIT(0), 0);

static SUNXI_CCU_GATE(uart11_clk, "uart11",
		"dcxo24M",
		0x0E30, BIT(0), 0);

static SUNXI_CCU_GATE(uart12_clk, "uart12",
		"dcxo24M",
		0x0E34, BIT(0), 0);

static SUNXI_CCU_GATE(uart13_clk, "uart13",
		"dcxo24M",
		0x0E38, BIT(0), 0);

static SUNXI_CCU_GATE(uart14_clk, "uart14",
		"dcxo24M",
		0x0E3C, BIT(0), 0);

static SUNXI_CCU_GATE(twi0_clk, "twi0",
		"dcxo24M",
		0x0E80, BIT(0), 0);

static SUNXI_CCU_GATE(twi1_clk, "twi1",
		"dcxo24M",
		0x0E84, BIT(0), 0);

static SUNXI_CCU_GATE(twi2_clk, "twi2",
		"dcxo24M",
		0x0E88, BIT(0), 0);

static SUNXI_CCU_GATE(twi3_clk, "twi3",
		"dcxo24M",
		0x0E8C, BIT(0), 0);

static SUNXI_CCU_GATE(twi4_clk, "twi4",
		"dcxo24M",
		0x0E90, BIT(0), 0);

static SUNXI_CCU_GATE(twi5_clk, "twi5",
		"dcxo24M",
		0x0E94, BIT(0), 0);

static SUNXI_CCU_GATE(twi6_clk, "twi6",
		"dcxo24M",
		0x0E98, BIT(0), 0);

static const char * const spi0_parents[] = { "dcxo24M", "pll-peri0-480m", "pll-peri0-300m", "pll-peri0-200m", "pll-peri1-480m", "pll-peri1-300m", "pll-peri1-200m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spi0_clk, "spi0",
		spi0_parents, 0x0F00,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(spi0_bus_clk, "spi0-bus",
		"dcxo24M",
		0x0F04, BIT(0), 0);

static const char * const spi1_parents[] = { "dcxo24M", "pll-peri0-480m", "pll-peri0-300m", "pll-peri0-200m", "pll-peri1-480m", "pll-peri1-300m", "pll-peri1-200m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spi1_clk, "spi1",
		spi1_parents, 0x0F08,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(spi1_bus_clk, "spi1-bus",
		"dcxo24M",
		0x0F0C, BIT(0), 0);

static const char * const spi2_parents[] = { "dcxo24M", "pll-peri0-480m", "pll-peri0-300m", "pll-peri0-200m", "pll-peri1-480m", "pll-peri1-300m", "pll-peri1-200m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spi2_clk, "spi2",
		spi2_parents, 0x0F10,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(spi2_bus_clk, "spi2-bus",
		"dcxo24M",
		0x0F14, BIT(0), 0);

static const char * const spif_parents[] = { "dcxo24M", "pll-peri0-480m", "pll-peri0-400m", "pll-peri0-300m", "pll-peri1-480m", "pll-peri1-400m", "pll-peri1-300m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spif_clk, "spif",
		spif_parents, 0x0F18,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(spif_bus_clk, "spif-bus",
		"dcxo24M",
		0x0F1C, BIT(0), 0);

static const char * const spi3_parents[] = { "dcxo24M", "pll-peri0-480m", "pll-peri0-300m", "pll-peri0-200m", "pll-peri1-480m", "pll-peri1-300m", "pll-peri1-200m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spi3_clk, "spi3",
		spi3_parents, 0x0F20,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(spi3_bus_clk, "spi3-bus",
		"dcxo24M",
		0x0F24, BIT(0), 0);

static const char * const spi4_parents[] = { "dcxo24M", "pll-peri0-480m", "pll-peri0-300m", "pll-peri0-200m", "pll-peri1-480m", "pll-peri1-300m", "pll-peri1-200m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spi4_clk, "spi4",
		spi4_parents, 0x0F28,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(spi4_bus_clk, "spi4-bus",
		"dcxo24M",
		0x0F2C, BIT(0), 0);

static const char * const gpadc0_parents[] = { "dcxo24M", "clk48m", "pll-peri0-480m" };

static SUNXI_CCU_M_WITH_MUX_GATE(gpadc0_clk, "gpadc0",
		gpadc0_parents, 0x0FC0,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(gpadc0_bus_clk, "gpadc0-bus",
		"dcxo24M",
		0x0FC4, BIT(0), 0);

static const char * const gpadc1_parents[] = { "dcxo24M", "clk48m", "pll-peri0-480m" };

static SUNXI_CCU_M_WITH_MUX_GATE(gpadc1_clk, "gpadc1",
		gpadc1_parents, 0x0FC8,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(gpadc1_bus_clk, "gpadc1-bus",
		"dcxo24M",
		0x0FCC, BIT(0), 0);

static const char * const gpadc2_parents[] = { "dcxo24M", "clk48m", "pll-peri0-480m" };

static SUNXI_CCU_M_WITH_MUX_GATE(gpadc2_clk, "gpadc2",
		gpadc2_parents, 0x0FD0,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(gpadc2_bus_clk, "gpadc2-bus",
		"dcxo24M",
		0x0FD4, BIT(0), 0);

static const char * const gpadc3_parents[] = { "dcxo24M", "clk48m", "pll-peri0-480m" };

static SUNXI_CCU_M_WITH_MUX_GATE(gpadc3_clk, "gpadc3",
		gpadc3_parents, 0x0FD8,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(gpadc3_bus_clk, "gpadc3-bus",
		"dcxo24M",
		0x0FDC, BIT(0), 0);

static SUNXI_CCU_GATE(ths_clk, "ths",
		"dcxo24M",
		0x0FE4, BIT(0), 0);

static const char * const irrx0_parents[] = { "osc32k", "dcxo24M" };

static SUNXI_CCU_M_WITH_MUX_GATE(irrx0_clk, "irrx0",
		irrx0_parents, 0x1000,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(irrx0_bus_clk, "irrx0-bus",
		"dcxo24M",
		0x1004, BIT(0), 0);

static const char * const irtx_parents[] = { "dcxo24M", "pll-peri1-600m" };

static SUNXI_CCU_M_WITH_MUX_GATE(irtx_clk, "irtx",
		irtx_parents, 0x1008,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(irtx_bus_clk, "irtx-bus",
		"dcxo24M",
		0x100C, BIT(0), 0);

static SUNXI_CCU_GATE(lradc_clk, "lradc",
		"dcxo24M",
		0x1024, BIT(0), 0);

static const char * const tpadc_24m_parents[] = { "dcxo24M", "audio0pll4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(tpadc_24m_clk, "tpadc-24m",
		tpadc_24m_parents, 0x1030,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(tpadc_clk, "tpadc",
		"dcxo24M",
		0x1034, BIT(0), 0);

static const char * const lbc_parents[] = { "pll-peri0-480m", "pll-peri0-400m", "pll-peri0-300m", "video0pll1x", "video1pll3x" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(lbc_clk, "lbc",
		lbc_parents, 0x1040,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const lbc_nsi_ahb_parents[] = { "dcxo24M", "pll-peri0-400m", "pll-peri0-300m", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(lbc_nsi_ahb_clk, "lbc-nsi-ahb",
		lbc_nsi_ahb_parents, 0x1048,
		0, 5,	/* M */
		24, 2,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(lbc_bus_clk, "lbc-bus",
		"dcxo24M",
		0x104C, BIT(0), 0);

static const char * const irrx1_parents[] = { "osc32k", "dcxo24M" };

static SUNXI_CCU_M_WITH_MUX_GATE(irrx1_clk, "irrx1",
		irrx1_parents, 0x1100,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(irrx1_bus_clk, "irrx1-bus",
		"dcxo24M",
		0x1104, BIT(0), 0);

static const char * const irrx2_parents[] = { "osc32k", "dcxo24M" };

static SUNXI_CCU_M_WITH_MUX_GATE(irrx2_clk, "irrx2",
		irrx2_parents, 0x1108,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(irrx2_bus_clk, "irrx2-bus",
		"dcxo24M",
		0x110C, BIT(0), 0);

static const char * const irrx3_parents[] = { "osc32k", "dcxo24M" };

static SUNXI_CCU_M_WITH_MUX_GATE(irrx3_clk, "irrx3",
		irrx3_parents, 0x1110,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(irrx3_bus_clk, "irrx3-bus",
		"dcxo24M",
		0x1114, BIT(0), 0);

static const char * const i2spcm0_parents[] = { "audio0pll4x", "audio1pll4x", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(i2spcm0_clk, "i2spcm0",
		i2spcm0_parents, 0x1200,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(i2spcm0_bus_clk, "i2spcm0-bus",
		"dcxo24M",
		0x120C, BIT(0), 0);

static const char * const i2spcm1_parents[] = { "audio0pll4x", "audio1pll4x", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(i2spcm1_clk, "i2spcm1",
		i2spcm1_parents, 0x1210,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(i2spcm1_bus_clk, "i2spcm1-bus",
		"dcxo24M",
		0x121C, BIT(0), 0);

static const char * const i2spcm2_parents[] = { "audio0pll4x", "audio1pll4x", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(i2spcm2_clk, "i2spcm2",
		i2spcm2_parents, 0x1220,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(i2spcm2_bus_clk, "i2spcm2-bus",
		"dcxo24M",
		0x122C, BIT(0), 0);

static const char * const i2spcm3_parents[] = { "audio0pll4x", "audio1pll4x", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(i2spcm3_clk, "i2spcm3",
		i2spcm3_parents, 0x1230,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(i2spcm3_bus_clk, "i2spcm3-bus",
		"dcxo24M",
		0x123C, BIT(0), 0);

static const char * const owa_tx_parents[] = { "audio0pll4x", "audio1pll4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(owa_tx_clk, "owa-tx",
		owa_tx_parents, 0x1280,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const owa_rx_parents[] = { "pll-peri0-400m", "pll-peri0-300m", "audio0pll4x", "audio1pll4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(owa_rx_clk, "owa-rx",
		owa_rx_parents, 0x1284,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(owa_clk, "owa",
		"dcxo24M",
		0x128C, BIT(0), 0);

static const char * const dmic_parents[] = { "audio0pll4x", "audio1pll4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(dmic_clk, "dmic",
		dmic_parents, 0x12C0,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(dmic_bus_clk, "dmic-bus",
		"dcxo24M",
		0x12CC, BIT(0), 0);

static const char * const audio_codec_dac_1x_parents[] = { "audio0pll4x", "audio1pll4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(audio_codec_dac_1x_clk, "audio-codec-dac-1x",
		audio_codec_dac_1x_parents, 0x12E0,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(audio_codec_clk, "audio-codec",
		"dcxo24M",
		0x12EC, BIT(0), 0);

static SUNXI_CCU_GATE(usb_clk, "usb",
		"dcxo24M",
		0x1300, BIT(31), 0);

static SUNXI_CCU_GATE(usb20_0_device_clk, "usb20-0-device",
		"dcxo24M",
		0x1304, BIT(8), 0);

static SUNXI_CCU_GATE(usb20_0_host_ehci_clk, "usb20-0-host-ehci",
		"dcxo24M",
		0x1304, BIT(4), 0);

static SUNXI_CCU_GATE(usb20_0_host_ohci_clk, "usb20-0-host-ohci",
		"dcxo24M",
		0x1304, BIT(0), 0);

static SUNXI_CCU_GATE(usb1_clk, "usb1",
		"dcxo24M",
		0x1308, BIT(31), 0);

static SUNXI_CCU_GATE(usb20_1_host_ehci_clk, "usb20-1-host-ehci",
		"dcxo24M",
		0x130C, BIT(4), 0);

static SUNXI_CCU_GATE(usb20_1_host_ohci_clk, "usb20-1-host-ohci",
		"dcxo24M",
		0x130C, BIT(0), 0);

static SUNXI_CCU_GATE(usb2_ref_clk, "usb2-ref",
		"dcxo24M",
		0x1348, BIT(31), 0);

static const char * const usb2_suspend_parents[] = { "osc32k", "dcxo24M" };

static SUNXI_CCU_M_WITH_MUX_GATE(usb2_suspend_clk, "usb2-suspend",
		usb2_suspend_parents, 0x1350,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const usb3_ref_parents[] = { "dcxo24M", "pll-peri0-300m", "pll-peri1-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(usb3_ref_clk, "usb3-ref",
		usb3_ref_parents, 0x1354,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(usb30_clk, "usb30",
		"dcxo24M",
		0x135C, BIT(0), 0);

static const char * const pcie_ref_aux_parents[] = { "dcxo24M", "osc32k" };

static SUNXI_CCU_M_WITH_MUX_GATE(pcie_ref_aux_clk, "pcie-ref-aux",
		pcie_ref_aux_parents, 0x1380,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const pcie_slv_parents[] = { "pll-peri0-300m", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(pcie_slv_clk, "pcie-slv",
		pcie_slv_parents, 0x1384,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const serdes_phy_cfg_parents[] = { "pll-peri0-600m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE(serdes_phy_cfg_clk, "serdes-phy-cfg",
		serdes_phy_cfg_parents, 0x13C0,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const serdes_phy_ref_parents[] = { "dcxo24M", "pll-peri0-600m" };

static SUNXI_CCU_M_WITH_MUX_GATE(serdes_phy_ref_clk, "serdes-phy-ref",
		serdes_phy_ref_parents, 0x13C4,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const serdes_axi_parents[] = { "dcxo24M", "pll-peri0-480m", "pll-peri0-400m", "pll-peri0-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(serdes_axi_clk, "serdes-axi",
		serdes_axi_parents, 0x13E0,
		0, 5,	/* M */
		24, 2,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(gmac0_phy_clk, "gmac0-phy",
		"dcxo24M",
		0x1400, BIT(31), 0);

static const char * const gmac0_ptp_parents[] = { "dcxo24M", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(gmac0_ptp_clk, "gmac0-ptp",
		gmac0_ptp_parents, 0x1404,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(gmac0_clk, "gmac0",
		"dcxo24M",
		0x140C, BIT(0), 0);

static SUNXI_CCU_GATE(gmac1_phy_clk, "gmac1-phy",
		"dcxo24M",
		0x1410, BIT(31), 0);

static const char * const gmac1_ptp_parents[] = { "dcxo24M", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(gmac1_ptp_clk, "gmac1-ptp",
		gmac1_ptp_parents, 0x1414,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(gmac1_clk, "gmac1",
		"dcxo24M",
		0x141C, BIT(0), 0);

static const char * const gmac_nsi_parents[] = { "dcxo24M", "pll-peri0-480m", "pll-peri0-400m", "pll-peri0-300m" };
static SUNXI_CCU_M_WITH_MUX_GATE(gmac_nsi_clk, "gmac-nsi-clk",
		gmac_nsi_parents, 0x1420,
		0, 5,
		24, 2,
		BIT(31),
		0);

static const char * const vo0_tconlcd0_parents[] = { "pll-peri0-2x", "pll-video0-4x", "pll-video1-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(vo0_tconlcd0_clk, "vo0-tconlcd0",
		vo0_tconlcd0_parents, 0x1500,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(vo0_tconlcd0_bus_clk, "vo0-tconlcd0-bus",
		"dcxo24M",
		0x1504, BIT(0), 0);

static const char * const dsi0_parents[] = { "dcxo24M", "pll-peri0-200m", "pll-peri0-150m" };

static SUNXI_CCU_M_WITH_MUX_GATE(dsi0_clk, "dsi0",
		dsi0_parents, 0x1580,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(dsi0_bus_clk, "dsi0-bus",
		"dcxo24M",
		0x1584, BIT(0), 0);

static const char * const vo0_combphy0_parents[] = { "pll-video0-4x", "pll-video1-4x", "video0pll1x", "video1pll3x", "pll-peri0-2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(vo0_combphy0_clk, "vo0-combphy0",
		vo0_combphy0_parents, 0x15C0,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(dpss_clk, "dpss",
		"dcxo24M",
		0x16C4, BIT(0), 0);

static const char * const ledc_parents[] = { "dcxo24M", "pll-peri0-600m" };

static SUNXI_CCU_M_WITH_MUX_GATE(ledc_clk, "ledc",
		ledc_parents, 0x1700,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(ledc_bus_clk, "ledc-bus",
		"dcxo24M",
		0x1704, BIT(0), 0);

static const char * const csi_master0_parents[] = { "dcxo24M", "pll-video1-4x", "video1pll3x", "pll-video0-4x", "video0pll1x" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(csi_master0_clk, "csi-master0",
		csi_master0_parents, 0x1800,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const csi_master1_parents[] = { "dcxo24M", "pll-video1-4x", "video1pll3x", "pll-video0-4x", "video0pll1x" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(csi_master1_clk, "csi-master1",
		csi_master1_parents, 0x1804,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const csi_master2_parents[] = { "dcxo24M", "pll-video1-4x", "video1pll3x", "pll-video0-4x", "video0pll1x" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(csi_master2_clk, "csi-master2",
		csi_master2_parents, 0x1808,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const csi_master3_parents[] = { "dcxo24M", "pll-video1-4x", "video1pll3x", "pll-video0-4x", "video0pll1x" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(csi_master3_clk, "csi-master3",
		csi_master3_parents, 0x180C,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const csi_parents[] = { "pll-peri0-480m", "pll-peri0-400m", "pll-peri0-300m", "pll-video0-4x", "video0pll1x", "pll-video1-4x", "video1pll3x", "pll-ve" };

static SUNXI_CCU_M_WITH_MUX_GATE(csi_clk, "csi",
		csi_parents, 0x1840,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(csi_bus_clk, "csi-bus",
		"dcxo24M",
		0x1844, BIT(0), 0);

static const char * const isp_parents[] = { "pll-peri0-300m", "pll-peri0-400m", "pll-video0-4x", "video0pll1x", "pll-video1-4x", "video1pll3x", "pll-ve", "pll-npu-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(isp_clk, "isp",
		isp_parents, 0x1860,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(isp_bus_clk, "isp-bus",
		"dcxo24M",
		0x1864, BIT(0), 0);

/* ccu_des_end */

/* rst_def_start */
static struct ccu_reset_map sun55iw6_ccu_resets[] = {
	[RST_BUS_ITS0]			= { 0x0574, BIT(16) },
	[RST_BUS_NSI]			= { 0x0580, BIT(30) },
	[RST_BUS_NSI_CFG]		= { 0x0584, BIT(16) },
	[RST_BUS_DMA0]			= { 0x0704, BIT(16) },
	[RST_BUS_DMA1]			= { 0x070c, BIT(16) },
	[RST_BUS_SPINLOCK]		= { 0x0724, BIT(16) },
	[RST_BUS_MSGBOX0]		= { 0x0744, BIT(16) },
	[RST_BUS_MSGBOX_CORE0]		= { 0x074c, BIT(16) },
	[RST_BUS_MSGBOX_CORE1]		= { 0x0754, BIT(16) },
	[RST_BUS_MSGBOX_CORE2]		= { 0x075c, BIT(16) },
	[RST_BUS_MSGBOX_CORE3]		= { 0x0764, BIT(16) },
	[RST_BUS_MSGBOX_RV]		= { 0x076c, BIT(16) },
	[RST_BUS_PWM0]			= { 0x0784, BIT(16) },
	[RST_BUS_PWM1]			= { 0x078c, BIT(16) },
	[RST_BUS_PWM2]			= { 0x0794, BIT(16) },
	[RST_BUS_DBGSY]			= { 0x07a4, BIT(16) },
	[RST_BUS_SYSDAP]		= { 0x07ac, BIT(16) },
	[RST_BUS_TIME]			= { 0x0850, BIT(16) },
	[RST_BUS_TIMER_RV]		= { 0x0870, BIT(16) },
	[RST_BUS_DE0]			= { 0x0a04, BIT(16) },
	[RST_BUS_G2D]			= { 0x0a44, BIT(16) },
	[RST_BUS_DE_SY]			= { 0x0a74, BIT(16) },
	[RST_BUS_VE]			= { 0x0a8c, BIT(16) },
	[RST_BUS_CE_SY]			= { 0x0ac4, BIT(17) },
	[RST_BUS_CE]			= { 0x0ac4, BIT(16) },
	[RST_BUS_NPU_GLB]		= { 0x0b04, BIT(19) },
	[RST_BUS_NPU_AHB]		= { 0x0b04, BIT(18) },
	[RST_BUS_NPU_AXI]		= { 0x0b04, BIT(17) },
	[RST_BUS_NPU_CORE]		= { 0x0b04, BIT(16) },
	[RST_BUS_RV_SY]			= { 0x0b94, BIT(17) },
	[RST_BUS_RV_CORE]		= { 0x0b94, BIT(16) },
	[RST_BUS_RV_CFG]		= { 0x0b9c, BIT(16) },
	[RST_BUS_DRAM]			= { 0x0c0c, BIT(16) },
	[RST_BUS_NAND0]			= { 0x0c8c, BIT(16) },
	[RST_BUS_SMHC0]			= { 0x0d0c, BIT(16) },
	[RST_BUS_SMHC1]			= { 0x0d1c, BIT(16) },
	[RST_BUS_SMHC2]			= { 0x0d2c, BIT(16) },
	[RST_BUS_UART0]			= { 0x0e00, BIT(16) },
	[RST_BUS_UART1]			= { 0x0e04, BIT(16) },
	[RST_BUS_UART2]			= { 0x0e08, BIT(16) },
	[RST_BUS_UART3]			= { 0x0e0c, BIT(16) },
	[RST_BUS_UART4]			= { 0x0e10, BIT(16) },
	[RST_BUS_UART5]			= { 0x0e14, BIT(16) },
	[RST_BUS_UART6]			= { 0x0e18, BIT(16) },
	[RST_BUS_UART7]			= { 0x0e20, BIT(16) },
	[RST_BUS_UART8]			= { 0x0e24, BIT(16) },
	[RST_BUS_UART9]			= { 0x0e28, BIT(16) },
	[RST_BUS_UART10]		= { 0x0e2c, BIT(16) },
	[RST_BUS_UART11]		= { 0x0e30, BIT(16) },
	[RST_BUS_UART12]		= { 0x0e34, BIT(16) },
	[RST_BUS_UART13]		= { 0x0e38, BIT(16) },
	[RST_BUS_UART14]		= { 0x0e3c, BIT(16) },
	[RST_BUS_TWI0]			= { 0x0e80, BIT(16) },
	[RST_BUS_TWI1]			= { 0x0e84, BIT(16) },
	[RST_BUS_TWI2]			= { 0x0e88, BIT(16) },
	[RST_BUS_TWI3]			= { 0x0e8c, BIT(16) },
	[RST_BUS_TWI4]			= { 0x0e90, BIT(16) },
	[RST_BUS_TWI5]			= { 0x0e94, BIT(16) },
	[RST_BUS_TWI6]			= { 0x0e98, BIT(16) },
	[RST_BUS_SPI0]			= { 0x0f04, BIT(16) },
	[RST_BUS_SPI1]			= { 0x0f0c, BIT(16) },
	[RST_BUS_SPI2]			= { 0x0f14, BIT(16) },
	[RST_BUS_SPIF]			= { 0x0f1c, BIT(16) },
	[RST_BUS_SPI3]			= { 0x0f24, BIT(16) },
	[RST_BUS_SPI4]			= { 0x0f2c, BIT(16) },
	[RST_BUS_GPADC0]		= { 0x0fc4, BIT(16) },
	[RST_BUS_GPADC1]		= { 0x0fcc, BIT(16) },
	[RST_BUS_GPADC2]		= { 0x0fd4, BIT(16) },
	[RST_BUS_GPADC3]		= { 0x0fdc, BIT(16) },
	[RST_BUS_TH]			= { 0x0fe4, BIT(16) },
	[RST_BUS_IRRX0]			= { 0x1004, BIT(16) },
	[RST_BUS_IRTX]			= { 0x100c, BIT(16) },
	[RST_BUS_LRADC]			= { 0x1024, BIT(16) },
	[RST_BUS_TPADC]			= { 0x1034, BIT(16) },
	[RST_BUS_LBC]			= { 0x104c, BIT(16) },
	[RST_BUS_IRRX1]			= { 0x1104, BIT(16) },
	[RST_BUS_IRRX2]			= { 0x110c, BIT(16) },
	[RST_BUS_IRRX3]			= { 0x1114, BIT(16) },
	[RST_BUS_I2SPCM0]		= { 0x120c, BIT(16) },
	[RST_BUS_I2SPCM1]		= { 0x121c, BIT(16) },
	[RST_BUS_I2SPCM2]		= { 0x122c, BIT(16) },
	[RST_BUS_I2SPCM3]		= { 0x123c, BIT(16) },
	[RST_BUS_OWA]			= { 0x128c, BIT(16) },
	[RST_BUS_DMIC]			= { 0x12cc, BIT(16) },
	[RST_BUS_AUDIO_CODEC]		= { 0x12ec, BIT(16) },
	[RST_USB_PHY0_RSTN]		= { 0x1300, BIT(30) },
	[RST_USB_20_0_DEVICE]		= { 0x1304, BIT(24) },
	[RST_USB_20_0_HOST_EHCI]	= { 0x1304, BIT(20) },
	[RST_USB_20_0_HOST_OHCI]	= { 0x1304, BIT(16) },
	[RST_USB_PHY1_RSTN]		= { 0x1308, BIT(30) },
	[RST_USB_20_1_HOST_EHCI]	= { 0x130c, BIT(20) },
	[RST_USB_20_1_HOST_OHCI]	= { 0x130c, BIT(16) },
	[RST_USB_30]			= { 0x135c, BIT(16) },
	[RST_BUS_PCIE]			= { 0x138c, BIT(17) },
	[RST_BUS_PCIE_PWRUP]		= { 0x138c, BIT(16) },
	[RST_BUS_SERDES_NOPPU]		= { 0x13cc, BIT(17) },
	[RST_BUS_SERDE]			= { 0x13cc, BIT(16) },
	[RST_BUS_GMAC0_AXI]		= { 0x140c, BIT(17) },
	[RST_BUS_GMAC0]			= { 0x140c, BIT(16) },
	[RST_BUS_GMAC1_AXI]		= { 0x141c, BIT(17) },
	[RST_BUS_GMAC1]			= { 0x141c, BIT(16) },
	[RST_BUS_VO0_TCONLCD0]		= { 0x1504, BIT(16) },
	[RST_BUS_LVDS0]			= { 0x1544, BIT(16) },
	[RST_BUS_DSI0]			= { 0x1584, BIT(16) },
	[RST_BUS_DP]			= { 0x16c4, BIT(16) },
	[RST_BUS_VIDEO_OUT0]		= { 0x16e4, BIT(16) },
	[RST_BUS_LEDC]			= { 0x1704, BIT(16) },
	[RST_BUS_CSI]			= { 0x1844, BIT(16) },
	[RST_BUS_ISP]			= { 0x1864, BIT(16) },
};
/* rst_def_end */

/* ccu_def_start */
static struct clk_hw_onecell_data sun55iw6_hw_clks = {
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
		[CLK_PLL_VIDEO0_4X]		= &pll_video0_4x.common.hw,
		[CLK_PLL_VIDEO1_4X]		= &pll_video1_4x.common.hw,
		[CLK_PLL_VE]			= &pll_ve_clk.common.hw,
		[CLK_PLL_NPU_4X]		= &pll_npu_4x_clk.common.hw,
		[CLK_AHB]			= &ahb_clk.common.hw,
		[CLK_APB0]			= &apb0_clk.common.hw,
		[CLK_APB1]			= &apb1_clk.common.hw,
		[CLK_APB_UART]			= &apb_uart_clk.common.hw,
		[CLK_TRACE]			= &trace_clk.common.hw,
		[CLK_GIC]			= &gic_clk.common.hw,
		[CLK_ITS0_ACLK]			= &its0_aclk.common.hw,
		[CLK_ITS0_HCLK]			= &its0_hclk.common.hw,
		[CLK_NSI]			= &nsi_clk.common.hw,
		[CLK_NSI_CFG]			= &nsi_cfg_clk.common.hw,
		[CLK_MBUS]			= &mbus_clk.common.hw,
		[CLK_IOMMU]			= &iommu_clk.common.hw,
		[CLK_GMAC1_MBUS_GATE]		= &gmac1_mbus_gate_clk.common.hw,
		[CLK_GMAC0_MBUS_GATE]		= &gmac0_mbus_gate_clk.common.hw,
		[CLK_ISP_MBUS_GATE]		= &isp_mbus_gate_clk.common.hw,
		[CLK_CSI_MBUS_GATE]		= &csi_mbus_gate_clk.common.hw,
		[CLK_NAND_MBUS_GATE]		= &nand_mbus_gate_clk.common.hw,
		[CLK_DMA1_MBUS_GATE]		= &dma1_mbus_gate_clk.common.hw,
		[CLK_CE_MBUS_GATE]		= &ce_mbus_gate_clk.common.hw,
		[CLK_VE_MBUS_GATE]		= &ve_mbus_gate_clk.common.hw,
		[CLK_DMA0_MBUS_GATE]		= &dma0_mbus_gate_clk.common.hw,
		[CLK_DMA0]			= &dma0_clk.common.hw,
		[CLK_DMA1]			= &dma1_clk.common.hw,
		[CLK_SPINLOCK]			= &spinlock_clk.common.hw,
		[CLK_MSGBOX0]			= &msgbox0_clk.common.hw,
		[CLK_MSGBOX_CORE0]		= &msgbox_core0_clk.common.hw,
		[CLK_MSGBOX_CORE1]		= &msgbox_core1_clk.common.hw,
		[CLK_MSGBOX_CORE2]		= &msgbox_core2_clk.common.hw,
		[CLK_MSGBOX_CORE3]		= &msgbox_core3_clk.common.hw,
		[CLK_MSGBOX_RV]			= &msgbox_rv_clk.common.hw,
		[CLK_PWM0]			= &pwm0_clk.common.hw,
		[CLK_PWM1]			= &pwm1_clk.common.hw,
		[CLK_PWM2]			= &pwm2_clk.common.hw,
		[CLK_DBGSYS]			= &dbgsys_clk.common.hw,
		[CLK_SYSDAP]			= &sysdap_clk.common.hw,
		[CLK_TIMER0]			= &timer0_clk.common.hw,
		[CLK_TIMER1]			= &timer1_clk.common.hw,
		[CLK_TIMER2]			= &timer2_clk.common.hw,
		[CLK_TIMER3]			= &timer3_clk.common.hw,
		[CLK_TIMER4]			= &timer4_clk.common.hw,
		[CLK_TIMER5]			= &timer5_clk.common.hw,
		[CLK_TIMER6]			= &timer6_clk.common.hw,
		[CLK_TIMER7]			= &timer7_clk.common.hw,
		[CLK_BUS_TIMER]			= &timer_bus_clk.common.hw,
		[CLK_TIMER0_RV]			= &timer0_rv_clk.common.hw,
		[CLK_TIMER1_RV]			= &timer1_rv_clk.common.hw,
		[CLK_TIMER2_RV]			= &timer2_rv_clk.common.hw,
		[CLK_TIMER3_RV]			= &timer3_rv_clk.common.hw,
		[CLK_RV_BUS_TIMER]		= &timer_rv_bus_clk.common.hw,
		[CLK_DE]			= &de_clk.common.hw,
		[CLK_DE0]			= &de0_clk.common.hw,
		[CLK_G2D]			= &g2d_clk.common.hw,
		[CLK_BUS_G2D]			= &g2d_bus_clk.common.hw,
		[CLK_VE]			= &ve_clk.common.hw,
		[CLK_BUS_VE]			= &ve_bus_clk.common.hw,
		[CLK_CE]			= &ce_clk.common.hw,
		[CLK_CE_SYS]			= &ce_sys_clk.common.hw,
		[CLK_BUS_CE]			= &ce_bus_clk.common.hw,
		[CLK_NPU]			= &npu_clk.common.hw,
		[CLK_NPU_TZMA]			= &npu_tzma_clk.common.hw,
		[CLK_BUS_NPU]			= &npu_bus_clk.common.hw,
		[CLK_RV_CORE]			= &rv_core_clk.common.hw,
		[CLK_RV_TS]			= &rv_ts_clk.common.hw,
		[CLK_RV_CFG]			= &rv_cfg_clk.common.hw,
		[CLK_DRAM]			= &dram_clk.common.hw,
		[CLK_BUS_DRAM]			= &dram_bus_clk.common.hw,
		[CLK_NAND0]			= &nand0_clk.common.hw,
		[CLK_NAND0_CLK2X]		= &nand0_clk2x_clk.common.hw,
		[CLK_BUS_NAND0]			= &nand0_bus_clk.common.hw,
		[CLK_SMHC0]			= &smhc0_clk.common.hw,
		[CLK_BUS_SMHC0]			= &smhc0_bus_clk.common.hw,
		[CLK_SMHC1]			= &smhc1_clk.common.hw,
		[CLK_BUS_SMHC1]			= &smhc1_bus_clk.common.hw,
		[CLK_SMHC2]			= &smhc2_clk.common.hw,
		[CLK_BUS_SMHC2]			= &smhc2_bus_clk.common.hw,
		[CLK_BUS_UART0]			= &uart0_clk.common.hw,
		[CLK_BUS_UART1]			= &uart1_clk.common.hw,
		[CLK_BUS_UART2]			= &uart2_clk.common.hw,
		[CLK_BUS_UART3]			= &uart3_clk.common.hw,
		[CLK_BUS_UART4]			= &uart4_clk.common.hw,
		[CLK_BUS_UART5]			= &uart5_clk.common.hw,
		[CLK_BUS_UART6]			= &uart6_clk.common.hw,
		[CLK_BUS_UART7]			= &uart7_clk.common.hw,
		[CLK_BUS_UART8]			= &uart8_clk.common.hw,
		[CLK_BUS_UART9]			= &uart9_clk.common.hw,
		[CLK_BUS_UART10]		= &uart10_clk.common.hw,
		[CLK_BUS_UART11]		= &uart11_clk.common.hw,
		[CLK_BUS_UART12]		= &uart12_clk.common.hw,
		[CLK_BUS_UART13]		= &uart13_clk.common.hw,
		[CLK_BUS_UART14]		= &uart14_clk.common.hw,
		[CLK_TWI0]			= &twi0_clk.common.hw,
		[CLK_TWI1]			= &twi1_clk.common.hw,
		[CLK_TWI2]			= &twi2_clk.common.hw,
		[CLK_TWI3]			= &twi3_clk.common.hw,
		[CLK_TWI4]			= &twi4_clk.common.hw,
		[CLK_TWI5]			= &twi5_clk.common.hw,
		[CLK_TWI6]			= &twi6_clk.common.hw,
		[CLK_SPI0]			= &spi0_clk.common.hw,
		[CLK_BUS_SPI0]			= &spi0_bus_clk.common.hw,
		[CLK_SPI1]			= &spi1_clk.common.hw,
		[CLK_BUS_SPI1]			= &spi1_bus_clk.common.hw,
		[CLK_SPI2]			= &spi2_clk.common.hw,
		[CLK_BUS_SPI2]			= &spi2_bus_clk.common.hw,
		[CLK_SPIF]			= &spif_clk.common.hw,
		[CLK_BUS_SPIF]			= &spif_bus_clk.common.hw,
		[CLK_SPI3]			= &spi3_clk.common.hw,
		[CLK_BUS_SPI3]			= &spi3_bus_clk.common.hw,
		[CLK_SPI4]			= &spi4_clk.common.hw,
		[CLK_BUS_SPI4]			= &spi4_bus_clk.common.hw,
		[CLK_GPADC0]			= &gpadc0_clk.common.hw,
		[CLK_BUS_GPADC0]		= &gpadc0_bus_clk.common.hw,
		[CLK_GPADC1]			= &gpadc1_clk.common.hw,
		[CLK_BUS_GPADC1]		= &gpadc1_bus_clk.common.hw,
		[CLK_GPADC2]			= &gpadc2_clk.common.hw,
		[CLK_BUS_GPADC2]		= &gpadc2_bus_clk.common.hw,
		[CLK_GPADC3]			= &gpadc3_clk.common.hw,
		[CLK_BUS_GPADC3]		= &gpadc3_bus_clk.common.hw,
		[CLK_THS]			= &ths_clk.common.hw,
		[CLK_IRRX0]			= &irrx0_clk.common.hw,
		[CLK_BUS_IRRX0]			= &irrx0_bus_clk.common.hw,
		[CLK_IRTX]			= &irtx_clk.common.hw,
		[CLK_BUS_IRTX]			= &irtx_bus_clk.common.hw,
		[CLK_LRADC]			= &lradc_clk.common.hw,
		[CLK_TPADC_24M]			= &tpadc_24m_clk.common.hw,
		[CLK_TPADC]			= &tpadc_clk.common.hw,
		[CLK_LBC]			= &lbc_clk.common.hw,
		[CLK_LBC_NSI_AHB]		= &lbc_nsi_ahb_clk.common.hw,
		[CLK_BUS_LBC]			= &lbc_bus_clk.common.hw,
		[CLK_IRRX1]			= &irrx1_clk.common.hw,
		[CLK_BUS_IRRX1]			= &irrx1_bus_clk.common.hw,
		[CLK_IRRX2]			= &irrx2_clk.common.hw,
		[CLK_BUS_IRRX2]			= &irrx2_bus_clk.common.hw,
		[CLK_IRRX3]			= &irrx3_clk.common.hw,
		[CLK_BUS_IRRX3]			= &irrx3_bus_clk.common.hw,
		[CLK_I2SPCM0]			= &i2spcm0_clk.common.hw,
		[CLK_BUS_I2SPCM0]		= &i2spcm0_bus_clk.common.hw,
		[CLK_I2SPCM1]			= &i2spcm1_clk.common.hw,
		[CLK_BUS_I2SPCM1]		= &i2spcm1_bus_clk.common.hw,
		[CLK_I2SPCM2]			= &i2spcm2_clk.common.hw,
		[CLK_BUS_I2SPCM2]		= &i2spcm2_bus_clk.common.hw,
		[CLK_I2SPCM3]			= &i2spcm3_clk.common.hw,
		[CLK_BUS_I2SPCM3]		= &i2spcm3_bus_clk.common.hw,
		[CLK_OWA_TX]			= &owa_tx_clk.common.hw,
		[CLK_OWA_RX]			= &owa_rx_clk.common.hw,
		[CLK_OWA]			= &owa_clk.common.hw,
		[CLK_DMIC]			= &dmic_clk.common.hw,
		[CLK_BUS_DMIC]			= &dmic_bus_clk.common.hw,
		[CLK_AUDIO_CODEC_DAC_1X]	= &audio_codec_dac_1x_clk.common.hw,
		[CLK_AUDIO_CODEC]		= &audio_codec_clk.common.hw,
		[CLK_USB]			= &usb_clk.common.hw,
		[CLK_USB20_0_DEVICE]		= &usb20_0_device_clk.common.hw,
		[CLK_USB20_0_HOST_EHCI]		= &usb20_0_host_ehci_clk.common.hw,
		[CLK_USB20_0_HOST_OHCI]		= &usb20_0_host_ohci_clk.common.hw,
		[CLK_USB1]			= &usb1_clk.common.hw,
		[CLK_USB20_1_HOST_EHCI]		= &usb20_1_host_ehci_clk.common.hw,
		[CLK_USB20_1_HOST_OHCI]		= &usb20_1_host_ohci_clk.common.hw,
		[CLK_USB2_REF]			= &usb2_ref_clk.common.hw,
		[CLK_USB2_SUSPEND]		= &usb2_suspend_clk.common.hw,
		[CLK_USB3_REF]			= &usb3_ref_clk.common.hw,
		[CLK_USB30]			= &usb30_clk.common.hw,
		[CLK_PCIE_REF_AUX]		= &pcie_ref_aux_clk.common.hw,
		[CLK_PCIE_SLV]			= &pcie_slv_clk.common.hw,
		[CLK_SERDES_PHY_CFG]		= &serdes_phy_cfg_clk.common.hw,
		[CLK_SERDES_PHY_REF]		= &serdes_phy_ref_clk.common.hw,
		[CLK_SERDES_AXI]		= &serdes_axi_clk.common.hw,
		[CLK_GMAC0_PHY]			= &gmac0_phy_clk.common.hw,
		[CLK_GMAC0_PTP]			= &gmac0_ptp_clk.common.hw,
		[CLK_GMAC0]			= &gmac0_clk.common.hw,
		[CLK_GMAC1_PHY]			= &gmac1_phy_clk.common.hw,
		[CLK_GMAC1_PTP]			= &gmac1_ptp_clk.common.hw,
		[CLK_GMAC1]			= &gmac1_clk.common.hw,
		[CLK_GMAC_NSI]			= &gmac_nsi_clk.common.hw,
		[CLK_VO0_TCONLCD0]		= &vo0_tconlcd0_clk.common.hw,
		[CLK_BUS_VO0_TCONLCD0]		= &vo0_tconlcd0_bus_clk.common.hw,
		[CLK_DSI0]			= &dsi0_clk.common.hw,
		[CLK_BUS_DSI0]			= &dsi0_bus_clk.common.hw,
		[CLK_VO0_COMBPHY0]		= &vo0_combphy0_clk.common.hw,
		[CLK_DPSS]			= &dpss_clk.common.hw,
		[CLK_LEDC]			= &ledc_clk.common.hw,
		[CLK_BUS_LEDC]			= &ledc_bus_clk.common.hw,
		[CLK_CSI_MASTER0]		= &csi_master0_clk.common.hw,
		[CLK_CSI_MASTER1]		= &csi_master1_clk.common.hw,
		[CLK_CSI_MASTER2]		= &csi_master2_clk.common.hw,
		[CLK_CSI_MASTER3]		= &csi_master3_clk.common.hw,
		[CLK_CSI]			= &csi_clk.common.hw,
		[CLK_BUS_CSI]			= &csi_bus_clk.common.hw,
		[CLK_ISP]			= &isp_clk.common.hw,
		[CLK_BUS_ISP]			= &isp_bus_clk.common.hw,
},
	.num = CLK_NUMBER,
};
/* ccu_def_end */

static struct ccu_common *sun55iw6_ccu_clks[] = {
	&pll_ddr_clk.common,
	&pll_peri0_parent_clk.common,
	&pll_peri0_2x_clk.common,
	&pll_peri0_800m_clk.common,
	&pll_peri0_480m_clk.common,
	&pll_peri1_parent_clk.common,
	&pll_peri1_2x_clk.common,
	&pll_peri1_800m_clk.common,
	&pll_peri1_480m_clk.common,
	&pll_video0_4x.common,
	&pll_video1_4x.common,
	&pll_ve_clk.common,
	&pll_npu_4x_clk.common,
	&ahb_clk.common,
	&apb0_clk.common,
	&apb1_clk.common,
	&apb_uart_clk.common,
	&trace_clk.common,
	&gic_clk.common,
	&its0_aclk.common,
	&its0_hclk.common,
	&nsi_clk.common,
	&nsi_cfg_clk.common,
	&mbus_clk.common,
	&iommu_clk.common,
	&gmac1_mbus_gate_clk.common,
	&gmac0_mbus_gate_clk.common,
	&isp_mbus_gate_clk.common,
	&csi_mbus_gate_clk.common,
	&nand_mbus_gate_clk.common,
	&dma1_mbus_gate_clk.common,
	&ce_mbus_gate_clk.common,
	&ve_mbus_gate_clk.common,
	&dma0_mbus_gate_clk.common,
	&dma0_clk.common,
	&dma1_clk.common,
	&spinlock_clk.common,
	&msgbox0_clk.common,
	&msgbox_core0_clk.common,
	&msgbox_core1_clk.common,
	&msgbox_core2_clk.common,
	&msgbox_core3_clk.common,
	&msgbox_rv_clk.common,
	&pwm0_clk.common,
	&pwm1_clk.common,
	&pwm2_clk.common,
	&dbgsys_clk.common,
	&sysdap_clk.common,
	&timer0_clk.common,
	&timer1_clk.common,
	&timer2_clk.common,
	&timer3_clk.common,
	&timer4_clk.common,
	&timer5_clk.common,
	&timer6_clk.common,
	&timer7_clk.common,
	&timer_bus_clk.common,
	&timer0_rv_clk.common,
	&timer1_rv_clk.common,
	&timer2_rv_clk.common,
	&timer3_rv_clk.common,
	&timer_rv_bus_clk.common,
	&de_clk.common,
	&de0_clk.common,
	&g2d_clk.common,
	&g2d_bus_clk.common,
	&ve_clk.common,
	&ve_bus_clk.common,
	&ce_clk.common,
	&ce_sys_clk.common,
	&ce_bus_clk.common,
	&npu_clk.common,
	&npu_tzma_clk.common,
	&npu_bus_clk.common,
	&rv_core_clk.common,
	&rv_ts_clk.common,
	&rv_cfg_clk.common,
	&dram_clk.common,
	&dram_bus_clk.common,
	&nand0_clk.common,
	&nand0_clk2x_clk.common,
	&nand0_bus_clk.common,
	&smhc0_clk.common,
	&smhc0_bus_clk.common,
	&smhc1_clk.common,
	&smhc1_bus_clk.common,
	&smhc2_clk.common,
	&smhc2_bus_clk.common,
	&uart0_clk.common,
	&uart1_clk.common,
	&uart2_clk.common,
	&uart3_clk.common,
	&uart4_clk.common,
	&uart5_clk.common,
	&uart6_clk.common,
	&uart7_clk.common,
	&uart8_clk.common,
	&uart9_clk.common,
	&uart10_clk.common,
	&uart11_clk.common,
	&uart12_clk.common,
	&uart13_clk.common,
	&uart14_clk.common,
	&twi0_clk.common,
	&twi1_clk.common,
	&twi2_clk.common,
	&twi3_clk.common,
	&twi4_clk.common,
	&twi5_clk.common,
	&twi6_clk.common,
	&spi0_clk.common,
	&spi0_bus_clk.common,
	&spi1_clk.common,
	&spi1_bus_clk.common,
	&spi2_clk.common,
	&spi2_bus_clk.common,
	&spif_clk.common,
	&spif_bus_clk.common,
	&spi3_clk.common,
	&spi3_bus_clk.common,
	&spi4_clk.common,
	&spi4_bus_clk.common,
	&gpadc0_clk.common,
	&gpadc0_bus_clk.common,
	&gpadc1_clk.common,
	&gpadc1_bus_clk.common,
	&gpadc2_clk.common,
	&gpadc2_bus_clk.common,
	&gpadc3_clk.common,
	&gpadc3_bus_clk.common,
	&ths_clk.common,
	&irrx0_clk.common,
	&irrx0_bus_clk.common,
	&irtx_clk.common,
	&irtx_bus_clk.common,
	&lradc_clk.common,
	&tpadc_24m_clk.common,
	&tpadc_clk.common,
	&lbc_clk.common,
	&lbc_nsi_ahb_clk.common,
	&lbc_bus_clk.common,
	&irrx1_clk.common,
	&irrx1_bus_clk.common,
	&irrx2_clk.common,
	&irrx2_bus_clk.common,
	&irrx3_clk.common,
	&irrx3_bus_clk.common,
	&i2spcm0_clk.common,
	&i2spcm0_bus_clk.common,
	&i2spcm1_clk.common,
	&i2spcm1_bus_clk.common,
	&i2spcm2_clk.common,
	&i2spcm2_bus_clk.common,
	&i2spcm3_clk.common,
	&i2spcm3_bus_clk.common,
	&owa_tx_clk.common,
	&owa_rx_clk.common,
	&owa_clk.common,
	&dmic_clk.common,
	&dmic_bus_clk.common,
	&audio_codec_dac_1x_clk.common,
	&audio_codec_clk.common,
	&usb_clk.common,
	&usb20_0_device_clk.common,
	&usb20_0_host_ehci_clk.common,
	&usb20_0_host_ohci_clk.common,
	&usb1_clk.common,
	&usb20_1_host_ehci_clk.common,
	&usb20_1_host_ohci_clk.common,
	&usb2_ref_clk.common,
	&usb2_suspend_clk.common,
	&usb3_ref_clk.common,
	&usb30_clk.common,
	&pcie_ref_aux_clk.common,
	&pcie_slv_clk.common,
	&serdes_phy_cfg_clk.common,
	&serdes_phy_ref_clk.common,
	&serdes_axi_clk.common,
	&gmac0_phy_clk.common,
	&gmac0_ptp_clk.common,
	&gmac0_clk.common,
	&gmac1_phy_clk.common,
	&gmac1_ptp_clk.common,
	&gmac1_clk.common,
	&gmac_nsi_clk.common,
	&vo0_tconlcd0_clk.common,
	&vo0_tconlcd0_bus_clk.common,
	&dsi0_clk.common,
	&dsi0_bus_clk.common,
	&vo0_combphy0_clk.common,
	&dpss_clk.common,
	&ledc_clk.common,
	&ledc_bus_clk.common,
	&csi_master0_clk.common,
	&csi_master1_clk.common,
	&csi_master2_clk.common,
	&csi_master3_clk.common,
	&csi_clk.common,
	&csi_bus_clk.common,
	&isp_clk.common,
	&isp_bus_clk.common,
};

static const struct sunxi_ccu_desc sun55iw6_ccu_desc = {
	.ccu_clks	= sun55iw6_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun55iw6_ccu_clks),

	.hw_clks	= &sun55iw6_hw_clks,

	.resets		= sun55iw6_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun55iw6_ccu_resets),
};

static const u32 sun55iw6_pll_regs[] = {
	SUN55IW6_PLL_DDR_CTRL_REG,
	SUN55IW6_PLL_PERI0_CTRL_REG,
	SUN55IW6_PLL_PERI1_CTRL_REG,
	SUN55IW6_PLL_VIDEO0_CTRL_REG,
	SUN55IW6_PLL_VIDEO1_CTRL_REG,
	SUN55IW6_PLL_VE_CTRL_REG,
	SUN55IW6_PLL_NPU_CTRL_REG,
};

static int sun55iw6_ccu_probe(struct platform_device *pdev)
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
	for (i = 0; i < ARRAY_SIZE(sun55iw6_pll_regs); i++) {
		set_reg(reg + sun55iw6_pll_regs[i], 0x7, 3, 29);
	}

	ret = sunxi_ccu_probe(pdev->dev.of_node, reg, &sun55iw6_ccu_desc);
	if (ret)
		return ret;

	sunxi_ccu_sleep_init(reg, sun55iw6_ccu_clks,
			ARRAY_SIZE(sun55iw6_ccu_clks),
			NULL, 0);

	return 0;
}

static const struct of_device_id sun55iw6_ccu_ids[] = {
	{ .compatible = "allwinner,sun55iw6-ccu" },
	{ }
};

static struct platform_driver sun55iw6_ccu_driver = {
	.probe	= sun55iw6_ccu_probe,
	.driver	= {
		.name	= "sun55iw6-ccu",
		.of_match_table	= sun55iw6_ccu_ids,
	},
};

static int __init sun55iw6_ccu_init(void)
{
	int err;

	err = platform_driver_register(&sun55iw6_ccu_driver);
	if (err)
		pr_err("register ccu sun55iw6 failed\n");

	return err;
}

core_initcall(sun55iw6_ccu_init);

static void __exit sun55iw6_ccu_exit(void)
{
	platform_driver_unregister(&sun55iw6_ccu_driver);
}
module_exit(sun55iw6_ccu_exit);

MODULE_DESCRIPTION("Allwinner sun55iw6 clk driver");
MODULE_AUTHOR("panzhijian<panzhijian@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.1.3");
