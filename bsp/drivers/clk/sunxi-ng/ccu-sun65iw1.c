// SPDX-License-Identifier: GPL-3.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 rengaomin@allwinnertech.com
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

#include "ccu-sun65iw1.h"

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
#define SUN65IW1_PLL_PERI0_CTRL_REG   0x00A0
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

#define SUN65IW1_PLL_PERI1_CTRL_REG   0x00C0
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

static SUNXI_CCU_GATE(pll_outp_clk, "pll-outp",
		"dcxo24M",
		0x00E0, BIT(27), 0);

#define SUN65IW1_PLL_GPU_CTRL_REG   0x0E0
static struct ccu_nkmp pll_gpu_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(20, 3), /* input divider */
	.p		= _SUNXI_CCU_DIV(1, 1), /* output divider */
	.common		= {
		.reg		= 0x00E0,
		.hw.init	= CLK_HW_INIT("pll-gpu", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN65IW1_PLL_VIDEO0_CTRL_REG   0x0120
static struct ccu_nm pll_video0_parent_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.common		= {
		.reg		= 0x0120,
		.hw.init	= CLK_HW_INIT("pll-video0-parent", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static SUNXI_CCU_M(pll_video0_4x_clk, "pll-video0-4x",
		"pll-video0-parent", 0x0120, 20, 3, CLK_SET_RATE_PARENT);

static SUNXI_CCU_M(pll_video0_3x_clk, "pll-video0-3x",
		"pll-video0-parent", 0x0120, 16, 3, CLK_SET_RATE_PARENT);

#define SUN65IW1_PLL_VIDEO1_CTRL_REG   0x0140
static struct ccu_nm pll_video1_parent_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.common		= {
		.reg		= 0x0140,
		.hw.init	= CLK_HW_INIT("pll-video1-parent", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static SUNXI_CCU_M(pll_video1_4x_clk, "pll-video1-4x",
		"pll-video1-parent", 0x0140, 20, 3, CLK_SET_RATE_PARENT);

static SUNXI_CCU_M(pll_video1_3x_clk, "pll-video1-3x",
		"pll-video1-parent", 0x0140, 16, 3, CLK_SET_RATE_PARENT);

#define SUN65IW1_PLL_VIDEO2_CTRL_REG   0x0160
static struct ccu_nm pll_video2_parent_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.common		= {
		.reg		= 0x0160,
		.hw.init	= CLK_HW_INIT("pll-video2-parent", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static SUNXI_CCU_M(pll_video2_4x_clk, "pll-video2-4x",
		"pll-video2-parent", 0x0160, 20, 3, CLK_SET_RATE_PARENT);

static SUNXI_CCU_M(pll_video2_3x_clk, "pll-video2-3x",
		"pll-video2-parent", 0x0160, 16, 3, CLK_SET_RATE_PARENT);

#define SUN65IW1_PLL_VE_CTRL_REG   0x0220
static struct ccu_nkmp pll_ve_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(20, 3), /* input divider */
	.p		= _SUNXI_CCU_DIV(1, 1),/* output divider */
	.common		= {
		.reg		= 0x0220,
		.hw.init	= CLK_HW_INIT("pll-ve", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static const char * const apb0_parents[] = { "dcxo24M", "osc32k", "iosc", "pll-peri0-600m" };
SUNXI_CCU_M_WITH_MUX(ahb_clk, "ahb", apb0_parents,
		0x0500, 0, 5, 24, 2, CLK_SET_RATE_PARENT);

SUNXI_CCU_M_WITH_MUX(apb0_clk, "apb0", apb0_parents,
		0x0510, 0, 5, 24, 2, CLK_SET_RATE_PARENT);

SUNXI_CCU_M_WITH_MUX(apb1_clk, "apb1", apb0_parents,
		0x0518, 0, 5, 24, 2, CLK_SET_RATE_PARENT);

static const char * const apb_uart_parents[] = { "dcxo24M", "osc32k", "iosc", "pll-peri0-600m", "pll-peri0-400m" };
SUNXI_CCU_M_WITH_MUX(apb_uart_clk, "apb-uart", apb_uart_parents,
		0x0538, 0, 5, 24, 2, CLK_SET_RATE_PARENT);

static const char * const cci_parents[] = { "dcxo24M", "pll-peri0pll2x", "pll-video0-4x", "pll-peri0-800m", "pll-video0-3x" };

static SUNXI_CCU_M_WITH_MUX_GATE(cci_clk, "cci",
		cci_parents, 0x0548,
		0,  5, /* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const gic_parents[] = { "dcxo24M", "sys-32k-clk", "pll-peri0-600m", "pll-peri0-480m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE(gic_clk, "gic",
		gic_parents, 0x0560,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const nsi_parents[] = { "dcxo24M", "pll-video0-3x", "pll-peri0-600m-bus", "pll-peri0-480m" };

static SUNXI_CCU_M_WITH_MUX_GATE(nsi_clk, "nsi",
		nsi_parents, 0x0580,
		0,  5, /* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(nsi_cfg_clk, "nsi-cfg",
		"dcxo24M",
		0x0584, BIT(0), 0);

static const char * const mbus_parents[] = { "dcxo24M", "pll-video0-3x", "pll-peri0-480m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE(mbus_clk, "mbus",
		mbus_parents, 0x0588,
		0,  5, /* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(iommu_apb_clk, "iommu-apb",
		"dcxo24M",
		0x058c, BIT(0), 0);

static SUNXI_CCU_GATE(stby_peri0pll_clk_gate_clk, "stby-peri0pll-clk-gate",
		"dcxo24M",
		0x05C0, BIT(28), 0);

static SUNXI_CCU_GATE(smhc2_ahb_gate_clk, "smhc2-ahb-gate",
		"dcxo24M",
		0x05C0, BIT(15), 0);

static SUNXI_CCU_GATE(smhc1_ahb_gate_clk, "smhc1-ahb-gate",
		"dcxo24M",
		0x05C0, BIT(14), 0);

static SUNXI_CCU_GATE(smhc0_ahb_gate_clk, "smhc0-ahb-gate",
		"dcxo24M",
		0x05C0, BIT(13), 0);

static SUNXI_CCU_GATE(hsi_ahb_gate_clk, "hsi-ahb-gate",
		"dcxo24M",
		0x05C0, BIT(11), 0);

static SUNXI_CCU_GATE(usb1_ahb_gate_clk, "usb1-ahb-gate",
		"dcxo24M",
		0x05C0, BIT(10), 0);

static SUNXI_CCU_GATE(usb0_ahb_gate_clk, "usb0-ahb-gate",
		"dcxo24M",
		0x05C0, BIT(9), 0);

static SUNXI_CCU_GATE(secure_sys_ahb_gate_clk, "secure-sys-ahb-gate",
		"dcxo24M",
		0x05C0, BIT(8), 0);

static SUNXI_CCU_GATE(gpu_ahb_gate_clk, "gpu-ahb-gate",
		"dcxo24M",
		0x05C0, BIT(7), 0);

static SUNXI_CCU_GATE(video_out1_ahb_gate_clk, "video-out1-ahb-gate",
		"dcxo24M",
		0x05C0, BIT(4), 0);

static SUNXI_CCU_GATE(video_out0_ahb_gate_clk, "video-out0-ahb-gate",
		"dcxo24M",
		0x05C0, BIT(3), 0);

static SUNXI_CCU_GATE(video_in_ahb_gate_clk, "video-in-ahb-gate",
		"dcxo24M",
		0x05C0, BIT(2), 0);

static SUNXI_CCU_GATE(ve0_ahb_gate_clk, "ve0-ahb-gate",
		"dcxo24M",
		0x05C0, BIT(1), 0);

static SUNXI_CCU_GATE(dmac0_mbus_gate_clk, "dmac0-mbus-gate",
		"dcxo24M",
		0x05E0, BIT(28), 0);

static SUNXI_CCU_GATE(gmac0_axi_gate_clk, "gmac0-axi-gate",
		"dcxo24M",
		0x05E0, BIT(12), 0);

static SUNXI_CCU_GATE(ce_sys_axi_gate_clk, "ce-sys-axi-gate",
		"dcxo24M",
		0x05E0, BIT(8), 0);

static SUNXI_CCU_GATE(gpu_axi_gate_clk, "gpu-axi-gate",
		"dcxo24M",
		0x05E0, BIT(7), 0);

static SUNXI_CCU_GATE(de_sys_mbus_gate_clk, "de-sys-mbus-gate",
		"dcxo24M",
		0x05E0, BIT(5), 0);

static SUNXI_CCU_GATE(video_out0_mbus_gate_clk, "video-out0-mbus-gate",
		"dcxo24M",
		0x05E0, BIT(3), 0);

static SUNXI_CCU_GATE(video_in_mbus_gate_clk, "video-in-mbus-gate",
		"dcxo24M",
		0x05E0, BIT(2), 0);

static SUNXI_CCU_GATE(ve0_mbus_gate_clk, "ve0-mbus-gate",
		"dcxo24M",
		0x05E0, BIT(1), 0);

static SUNXI_CCU_GATE(dma_ahb_clk, "dma-ahb",
		"dcxo24M",
		0x0704, BIT(0), 0);

static SUNXI_CCU_GATE(spinlock_ahb_clk, "spinlock-ahb",
		"dcxo24M",
		0x0724, BIT(0), 0);

static SUNXI_CCU_GATE(msgbox_cpux_ahb_clk, "msgbox-cpux-ahb",
		"dcxo24M",
		0x0744, BIT(0), 0);

static SUNXI_CCU_GATE(msgbox_cpus_ahb_clk, "msgbox-cpus-ahb",
		"dcxo24M",
		0x074C, BIT(0), 0);

static SUNXI_CCU_GATE(pwm0_apb_clk, "pwm0-apb",
		"dcxo24M",
		0x0784, BIT(0), 0);

static SUNXI_CCU_GATE(dcu_clk, "dcu",
		"dcxo24M",
		0x07A4, BIT(0), 0);

static SUNXI_CCU_GATE(dap_ahb_clk, "dap-ahb",
		"dcxo24M",
		0x07AC, BIT(0), 0);

static const char * const timer_parents[] = { "dcxo24M", "iosc", "clk-32k", "pll-peri0-200m" };

static struct ccu_div timer0_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x0800,
		.hw.init	= CLK_HW_INIT_PARENTS("timer0", timer_parents, &ccu_div_ops, 0),
	},
};

static struct ccu_div timer1_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x0804,
		.hw.init	= CLK_HW_INIT_PARENTS("timer1", timer_parents, &ccu_div_ops, 0),
	},
};

static struct ccu_div timer2_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x0808,
		.hw.init	= CLK_HW_INIT_PARENTS("timer2", timer_parents, &ccu_div_ops, 0),
	},
};

static struct ccu_div timer3_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x080C,
		.hw.init	= CLK_HW_INIT_PARENTS("timer3", timer_parents, &ccu_div_ops, 0),
	},
};

static SUNXI_CCU_GATE(timer_ahb_clk, "timer-ahb",
		"dcxo24M",
		0x0850, BIT(0), 0);

static const char * const de_parents[] = { "pll-peri0-600m", "pll-peri0-480m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE(de_clk, "de",
		de_parents, 0x0A00,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(de0_ahb_clk, "de0-ahb",
		"dcxo24M",
		0x0A04, BIT(0), 0);

static const char * const g2d_parents[] = { "pll-peri0-600m", "pll-peri0-480m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE(g2d_clk, "g2d",
		g2d_parents, 0x0A40,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(g2d_ahb_clk, "g2d-ahb",
		"dcxo24M",
		0x0A44, BIT(0), 0);

static const char * const eink_panel_parents[] = { "pll-peri0-300m", "pll-video0-4x", "pll-video0-3x", "pll-video1-4x", "pll-video1-3x" };

static SUNXI_CCU_M_WITH_MUX_GATE(eink_panel_clk, "eink-panel",
		eink_panel_parents, 0x0A64,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(eink_ahb_clk, "eink-ahb",
		"dcxo24M",
		0x0A6C, BIT(0), 0);

static const char * const ve0_parents[] = { "pll-ve", "pll-peri0-800m", "pll-peri0-600m", "pll-peri0-480m" };

static SUNXI_CCU_M_WITH_MUX_GATE(ve0_clk, "ve0",
		ve0_parents, 0x0A80,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(ve0_ahb_clk, "ve0-ahb",
		"dcxo24M",
		0x0A8C, BIT(0), 0);

static const char * const ce_sys_parents[] = { "dcxo24M", "pll-peri0-400m", "pll-peri0-600m" };

static SUNXI_CCU_M_WITH_MUX_GATE(ce_sys_clk, "ce-sys",
		ce_sys_parents, 0x0AC0,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(gpu_ahb_clk, "gpu-ahb",
		"dcxo24M",
		0x0B24, BIT(0), 0);

static SUNXI_CCU_GATE(memc_ahb_clk, "memc-ahb",
		"dcxo24M",
		0x0C0C, BIT(0), 0);

static const char * const smhc0_parents[] = { "dcxo24M", "pll-peri0-400m", "pll-peri0-300m", "pll-peri1-400m", "pll-peri1-300m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc0_clk, "smhc0",
		smhc0_parents, 0x0D00,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(smhc0_bus_clk, "smhc0-bus",
		"dcxo24M",
		0xD0C, BIT(0), 0);

static const char * const smhc1_parents[] = { "dcxo24M", "pll-peri0-400m", "pll-peri0-300m", "pll-peri1-400m", "pll-peri1-300m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc1_clk, "smhc1",
		smhc1_parents, 0x0D10,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(smhc1_bus_clk, "smhc1-bus",
		"dcxo24M",
		0xD1C, BIT(0), 0);

static const char * const smhc2_parents[] = { "dcxo24M", "pll-peri0-800m", "pll-peri0-600m", "pll-peri1-800m", "pll-peri1-600m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc2_clk, "smhc2",
		smhc2_parents, 0x0D20,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(smhc2_bus_clk, "smhc2-bus",
		"dcxo24M",
		0xD2C, BIT(0), 0);

static SUNXI_CCU_GATE(uart0_bus_clk, "uart0-bus",
		"apb-uart",
		0xE00, BIT(0), 0);

static SUNXI_CCU_GATE(uart1_bus_clk, "uart1-bus",
		"apb-uart",
		0xE04, BIT(0), 0);

static SUNXI_CCU_GATE(uart2_bus_clk, "uart2-bus",
		"apb-uart",
		0xE08, BIT(0), 0);

static SUNXI_CCU_GATE(uart3_bus_clk, "uart3-bus",
		"apb-uart",
		0xE0C, BIT(0), 0);

static SUNXI_CCU_GATE(uart4_bus_clk, "uart4-bus",
		"apb-uart",
		0xE10, BIT(0), 0);

static SUNXI_CCU_GATE(uart5_bus_clk, "uart5-bus",
		"apb-uart",
		0xE14, BIT(0), 0);

static SUNXI_CCU_GATE(uart6_bus_clk, "uart6-bus",
		"apb-uart",
		0xE18, BIT(0), 0);

static SUNXI_CCU_GATE(uart7_bus_clk, "uart7-bus",
		"apb-uart",
		0xE1C, BIT(0), 0);

static SUNXI_CCU_GATE(twi0_bus_clk, "twi0-bus",
		"apb1",
		0xE80, BIT(0), 0);

static SUNXI_CCU_GATE(twi1_bus_clk, "twi1-bus",
		"apb1",
		0xE84, BIT(0), 0);

static SUNXI_CCU_GATE(twi2_bus_clk, "twi2-bus",
		"apb1",
		0xE88, BIT(0), 0);

static SUNXI_CCU_GATE(twi3_bus_clk, "twi3-bus",
		"apb1",
		0xE8c, BIT(0), 0);

static SUNXI_CCU_GATE(twi4_bus_clk, "twi4-bus",
		"apb1",
		0xE90, BIT(0), 0);

static SUNXI_CCU_GATE(twi5_bus_clk, "twi5-bus",
		"apb1",
		0xE94, BIT(0), 0);

static const char * const spi0_parents[] = { "dcxo24M", "pll-peri0-480m", "pll-peri0-300m", "pll-peri0-200m", "pll-peri1-480m", "pll-peri1-300m", "pll-peri1-200m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spi0_clk, "spi0",
		spi0_parents, 0x0F00,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(spi0_ahb_clk, "spi0-ahb",
		"dcxo24M",
		0xF04, BIT(0), 0);

static const char * const spi1_parents[] = { "dcxo24M", "pll-peri0-480m", "pll-peri0-300m", "pll-peri0-200m", "pll-peri1-480m", "pll-peri1-300m", "pll-peri1-200m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spi1_clk, "spi1",
		spi1_parents, 0x0F08,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(spi1_ahb_clk, "spi1-ahb",
		"dcxo24M",
		0xF08, BIT(0), 0);

static const char * const spi2_parents[] = { "dcxo24M", "pll-peri0-480m", "pll-peri0-300m", "pll-peri0-200m", "pll-peri1-480m", "pll-peri1-300m", "pll-peri1-200m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spi2_clk, "spi2",
		spi2_parents, 0x0F10,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(spi2_ahb_clk, "spi2-ahb",
		"dcxo24M",
		0xF14, BIT(0), 0);

static SUNXI_CCU_GATE(gpadc0_clk, "gpadc0",
		"dcxo24M",
		0x0FC0, BIT(31), 0);

static SUNXI_CCU_GATE(gpadc0_apb_clk, "gpadc0-apb",
		"dcxo24M",
		0x0FC4, BIT(0), 0);

static SUNXI_CCU_GATE(tsensor_apb_clk, "tsensor-apb",
		"dcxo24M",
		0x0FE4, BIT(0), 0);

static const char * const irrx0_parents[] = { "sys-32k-clk", "dcxo24M" };

static SUNXI_CCU_M_WITH_MUX_GATE(irrx0_clk, "irrx0",
		irrx0_parents, 0x1000,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(irrx0_apb_clk, "irrx0-apb",
		"dcxo24M",
		0x01004, BIT(0), 0);

static const char * const irtx_parents[] = { "dcxo24M", "pll-peri1-600m" };

static SUNXI_CCU_M_WITH_MUX_GATE(irtx_clk, "irtx",
		irtx_parents, 0x1008,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(irtx_apb_clk, "irrx-apb",
		"dcxo24M",
		0x0100C, BIT(0), 0);

static const char * const i2s0_parents[] = { "audio0pll", "audio1pll2x", "audio1pll5x", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(i2s0_clk, "i2s0",
		i2s0_parents, 0x1200,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(i2s0_apb_clk, "i2s0-apb",
		"dcxo24M",
		0x0120C, BIT(0), 0);

static const char * const i2s1_parents[] = { "audio0pll", "audio1pll2x", "audio1pll5x", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(i2s1_clk, "i2s1",
		i2s1_parents, 0x1210,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(i2s1_apb_clk, "i2s1-apb",
		"dcxo24M",
		0x0121C, BIT(0), 0);

static const char * const i2s2_parents[] = { "audio0pll", "audio1pll2x", "audio1pll5x", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(i2s2_clk, "i2s2",
		i2s2_parents, 0x1220,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(i2s2_apb_clk, "i2s2-apb",
		"dcxo24M",
		0x0122C, BIT(0), 0);

static const char * const i2s3_parents[] = { "audio0pll", "audio1pll2x", "audio1pll5x", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(i2s3_clk, "i2s3",
		i2s3_parents, 0x1230,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const i2s3_asrc_parents[] = { "pll-peri0-300m", "pll-peri1-300m", "audio0pll", "audio1pll2x", "audio1pll5x" };

static SUNXI_CCU_M_WITH_MUX_GATE(i2s3_asrc_clk, "i2s3-asrc",
		i2s3_asrc_parents, 0x1234,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(i2s3_apb_clk, "i2s3-apb",
		"dcxo24M",
		0x0123C, BIT(0), 0);

static const char * const owa0_tx_parents[] = { "audio0pll", "audio1pll2x", "audio1pll5x" };

static SUNXI_CCU_M_WITH_MUX_GATE(owa0_tx_clk, "owa0-tx",
		owa0_tx_parents, 0x1280,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const owa0_rx_parents[] = { "pll-peri0-400m", "pll-peri0-300m", "audio0pll", "audio1pll2x", "audio1pll5x" };

static SUNXI_CCU_M_WITH_MUX_GATE(owa0_rx_clk, "owa0-rx",
		owa0_rx_parents, 0x1284,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(owa0_apb_clk, "owa0-apb",
		"dcxo24M",
		0x0128C, BIT(0), 0);

static const char * const dmic_parents[] = { "audio0pll", "audio1pll2x", "audio1pll5x" };

static SUNXI_CCU_M_WITH_MUX_GATE(dmic_clk, "dmic",
		dmic_parents, 0x12C0,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(dmic_apb_clk, "dmic-apb",
		"dcxo24M",
		0x012CC, BIT(0), 0);

static const char * const adda_dac_parents[] = { "audio0pll", "audio1pll2x", "audio1pll5x" };

static SUNXI_CCU_M_WITH_MUX_GATE(adda_dac_clk, "adda-dac",
		adda_dac_parents, 0x12E0,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const adda_adc_parents[] = { "audio0pll", "audio1pll2x", "audio1pll5x" };

static SUNXI_CCU_M_WITH_MUX_GATE(adda_adc_clk, "adda-adc",
		adda_adc_parents, 0x12E8,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(adda_apb_clk, "adda-apb",
		"dcxo24M",
		0x012EC, BIT(0), 0);

static SUNXI_CCU_GATE(usb_clk, "usb",
		"dcxo24M",
		0x1300, BIT(31), 0);

static SUNXI_CCU_GATE(usb0_dev_clk, "usb0-dev",
		"dcxo24M",
		0x1304, BIT(8), 0);

static SUNXI_CCU_GATE(usb0_ehci_clk, "usb-ehci",
		"dcxo24M",
		0x1304, BIT(4), 0);

static SUNXI_CCU_GATE(usb0_ohci_clk, "usb-ohci",
		"dcxo24M",
		0x1304, BIT(0), 0);

static SUNXI_CCU_GATE(usb_bus_clk, "usb-bus",
		"dcxo24M",
		0x1308, BIT(31), 0);

static SUNXI_CCU_GATE(usb1_ehci_clk, "usb1-ehci",
		"dcxo24M",
		0x1304, BIT(4), 0);

static SUNXI_CCU_GATE(usb1_ohci_clk, "usb1-ohci",
		"dcxo24M",
		0x1304, BIT(0), 0);

static SUNXI_CCU_GATE(usb0_usb1_phy_ref_clk, "usb0-usb1-phy-ref",
		"dcxo24M",
		0x1340, BIT(31), 0);

static SUNXI_CCU_GATE(usb0_usb1_ahb_clk, "usb0-usb1",
		"dcxo24M",
		0x1344, BIT(0), 0);

static SUNXI_CCU_GATE(usb2_u2_phy_ref_clk, "usb2-u2-phy-ref",
		"dcxo24M",
		0x1348, BIT(31), 0);

static const char * const usb2_suspend_parents[] = { "sys-32k-clk", "dcxo24M" };

static SUNXI_CCU_M_WITH_MUX_GATE(usb2_suspend_clk, "usb2-suspend",
		usb2_suspend_parents, 0x1350,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const usb2_ref_parents[] = { "dcxo24M", "pll-peri0-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(usb2_ref_clk, "usb2-ref",
		usb2_ref_parents, 0x1354,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const usb2_u3_only_utmi_parents[] = { "dcxo24M", "pll-peri0-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(usb2_u3_only_utmi_clk, "usb2-u3-only-utmi",
		usb2_u3_only_utmi_parents, 0x1360,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const usb2_u2_only_pipe_parents[] = { "dcxo24M", "pll-peri0-480m" };

static SUNXI_CCU_M_WITH_MUX_GATE(usb2_u2_only_pipe_clk, "usb2-u2-only-pipe",
		usb2_u2_only_pipe_parents, 0x1364,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const pcie0_aux_parents[] = { "dcxo24M", "sys-32k-clk" };

static SUNXI_CCU_M_WITH_MUX_GATE(pcie0_aux_clk, "pcie0-aux",
		pcie0_aux_parents, 0x1380,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const pcie0_axi_s_parents[] = { "pll-peri0-300m", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(pcie0_axi_s_clk, "pcie0-axi-s",
		pcie0_axi_s_parents, 0x1384,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const hsi_phy_cfg_parents[] = { "pll-peri0-600m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE(hsi_phy_cfg_clk, "hsi-phy-cfg",
		hsi_phy_cfg_parents, 0x13C0,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const hsi_phy_ref_parents[] = { "dcxo24M", "pll-peri0-600m" };

static SUNXI_CCU_M_WITH_MUX_GATE(hsi_phy_ref_clk, "hsi-phy-ref",
		hsi_phy_ref_parents, 0x13C4,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(hsi_sys_clk, "hsi-sys",
		"dcxo24M",
		0x13CC, BIT(0), 0);

static const char * const hsi_axi_parents[] = { "dcxo24M", "pll-peri0-480m", "pll-peri0-400m", "pll-peri0-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(hsi_axi_clk, "hsi-axi",
		hsi_axi_parents, 0x13E0,
		0, 5,	/* M */
		24, 2,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(gmac0_phy_clk, "gmac0-phy",
		"dcxo24M",
		0x1400, BIT(31), 0);

static SUNXI_CCU_GATE(gmac0_ahb_clk, "gmac0-ahb",
		"dcxo24M",
		0x140C, BIT(0), 0);

static const char * const dsi0_parents[] = { "dcxo24M", "pll-peri0-200m", "pll-peri0-150m" };

static SUNXI_CCU_M_WITH_MUX_GATE(dsi0_clk, "dsi0",
		dsi0_parents, 0x1580,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(dsi_ahb_clk, "dsi-ahb",
		"dcxo24M",
		0x1584, BIT(0), 0);

static SUNXI_CCU_GATE(tcon_tv0_clk, "tcon-tv0",
		"dcxo24M",
		0x1604, BIT(0), 0);

static SUNXI_CCU_GATE(edp_clk, "edp-tv0",
		"dcxo24M",
		0x164C, BIT(0), 0);

static SUNXI_CCU_GATE(vo0_ahb_clk, "vo0-ahb",
		"dcxo24M",
		0x16C4, BIT(0), 0);

static SUNXI_CCU_GATE(vo1_ahb_clk, "vo1-ahb",
		"dcxo24M",
		0x16CC, BIT(0), 0);

static const char * const ledc_parents[] = { "dcxo24M", "pll-peri0-600m" };

static SUNXI_CCU_M_WITH_MUX_GATE(ledc_clk, "ledc",
		ledc_parents, 0x1700,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(ledc_apb_clk, "ledc-apb",
		"dcxo24M",
		0x1704, BIT(0), 0);

static const char * const csi_master0_parents[] = { "dcxo24M", "pll-video1-4x", "pll-video1-3x", "pll-video2-4x", "pll-video2-3x", "pll-video0-4x", "pll-video0-3x" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(csi_master0_clk, "csi-master0",
		csi_master0_parents, 0x1800,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const csi_master1_parents[] = { "dcxo24M", "pll-video1-4x", "pll-video1-3x", "pll-video2-4x", "pll-video2-3x", "pll-video0-4x", "pll-video0-3x" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(csi_master1_clk, "csi-master1",
		csi_master1_parents, 0x1804,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const csi_master2_parents[] = { "dcxo24M", "pll-video1-4x", "pll-video1-3x", "pll-video2-4x", "pll-video2-3x", "pll-video0-4x", "pll-video0-3x" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(csi_master2_clk, "csi-master2",
		csi_master2_parents, 0x1808,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const csi_master3_parents[] = { "dcxo24M", "pll-video1-4x", "pll-video1-3x", "pll-video2-4x", "pll-video2-3x", "pll-video0-4x", "pll-video0-3x" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(csi_master3_clk, "csi-master3",
		csi_master3_parents, 0x180C,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const csi_parents[] = { "pll-video2-4x", "pll-video2-3x", "pll-peri0-600m", "pll-peri0-480m", "pll-peri0-400m", "pll-video0-4x", "pll-video1-4x", "pll-ve" };

static SUNXI_CCU_M_WITH_MUX_GATE(csi_clk, "csi",
		csi_parents, 0x1840,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const isp_parents[] = { "pll-video2-4x", "pll-video2-3x", "pll-peri0-600m", "pll-peri0-480m", "pll-peri0-400m", "pll-video0-4x", "pll-video1-4x", "pll-ve" };

static SUNXI_CCU_M_WITH_MUX_GATE(isp_clk, "isp",
		isp_parents, 0x1860,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(video_in_clk, "video-in",
		"dcxo24M",
		0x1884, BIT(0), 0);
/* ccu_des_end */

/* rst_def_start */
static struct ccu_reset_map sun65iw1_ccu_resets[] = {
	[RST_BUS_NSI_CFG]		= { 0x0584, BIT(17) },
	[RST_BUS_NSI]			= { 0x0584, BIT(16) },
	[RST_BUS_DMAC0]		= { 0x0704, BIT(16) },
	[RST_BUS_SPINLOCK]		= { 0x0724, BIT(16) },
	[RST_BUS_MSGBOX_CPUX]		= { 0x0744, BIT(16) },
	[RST_BUS_MSGBOX_CPU]		= { 0x074c, BIT(16) },
	[RST_BUS_PWM0]			= { 0x0784, BIT(16) },
	[RST_BUS_DCU]			= { 0x07a4, BIT(16) },
	[RST_BUS_DAP]			= { 0x07ac, BIT(16) },
	[RST_BUS_TIME]			= { 0x0850, BIT(16) },
	[RST_BUS_DE0]			= { 0x0a04, BIT(16) },
	[RST_BUS_G2D]			= { 0x0a44, BIT(16) },
	[RST_BUS_EINK]			= { 0x0a6c, BIT(16) },
	[RST_BUS_DE_SY]		= { 0x0a74, BIT(16) },
	[RST_BUS_VE0]			= { 0x0a8c, BIT(16) },
	[RST_BUS_CE_SY]		= { 0x0ac4, BIT(17) },
	[RST_BUS_CE_SYS_AHB]		= { 0x0ac4, BIT(16) },
	[RST_BUS_GPU]			= { 0x0b24, BIT(16) },
	[RST_BUS_MEMC]			= { 0x0c0c, BIT(16) },
	[RST_BUS_SMHC0]		= { 0x0d0c, BIT(16) },
	[RST_BUS_SMHC1]		= { 0x0d1c, BIT(16) },
	[RST_BUS_SMHC2]		= { 0x0d2c, BIT(16) },
	[RST_BUS_UART0]		= { 0x0e00, BIT(16) },
	[RST_BUS_UART1]		= { 0x0e04, BIT(16) },
	[RST_BUS_UART2]		= { 0x0e08, BIT(16) },
	[RST_BUS_UART3]		= { 0x0e0c, BIT(16) },
	[RST_BUS_UART4]		= { 0x0e10, BIT(16) },
	[RST_BUS_UART5]		= { 0x0e14, BIT(16) },
	[RST_BUS_UART6]		= { 0x0e18, BIT(16) },
	[RST_BUS_UART7]		= { 0x0e1c, BIT(16) },
	[RST_BUS_TWI0]			= { 0x0e80, BIT(16) },
	[RST_BUS_TWI1]			= { 0x0e84, BIT(16) },
	[RST_BUS_TWI2]			= { 0x0e88, BIT(16) },
	[RST_BUS_TWI3]			= { 0x0e8c, BIT(16) },
	[RST_BUS_TWI4]			= { 0x0e90, BIT(16) },
	[RST_BUS_TWI5]			= { 0x0e94, BIT(16) },
	[RST_BUS_SPI0]			= { 0x0f04, BIT(16) },
	[RST_BUS_SPI1]			= { 0x0f0c, BIT(16) },
	[RST_BUS_SPI2]			= { 0x0f14, BIT(16) },
	[RST_BUS_GPADC0]		= { 0x0fc4, BIT(16) },
	[RST_BUS_TSENSO]		= { 0x0fe4, BIT(16) },
	[RST_BUS_IRRX0]		= { 0x1004, BIT(16) },
	[RST_BUS_IRTX]			= { 0x100c, BIT(16) },
	[RST_BUS_I2S0]			= { 0x120c, BIT(16) },
	[RST_BUS_I2S1]			= { 0x121c, BIT(16) },
	[RST_BUS_I2S2]			= { 0x122c, BIT(16) },
	[RST_BUS_I2S3]			= { 0x123c, BIT(16) },
	[RST_BUS_OWA0]			= { 0x128c, BIT(16) },
	[RST_BUS_DMIC]			= { 0x12cc, BIT(16) },
	[RST_BUS_ADDA]			= { 0x12ec, BIT(16) },
	[RST_USB_PHY0_RSTN]		= { 0x1300, BIT(30) },
	[RST_USB_0_DEV]		= { 0x1304, BIT(24) },
	[RST_USB_0_EHCI]		= { 0x1304, BIT(20) },
	[RST_USB_0_OHCI]		= { 0x1304, BIT(16) },
	[RST_USB_PHY1_RSTN]		= { 0x1308, BIT(30) },
	[RST_USB_1_EHCI]		= { 0x130c, BIT(20) },
	[RST_USB_1_OHCI]		= { 0x130c, BIT(16) },
	[RST_USB_0_USB1]		= { 0x1344, BIT(16) },
	[RST_USB_2]			= { 0x135c, BIT(16) },
	[RST_BUS_PCIE0]		= { 0x138c, BIT(17) },
	[RST_BUS_PCIE0_PWR_UP]		= { 0x138c, BIT(16) },
	[RST_BUS_HSI_SY]		= { 0x13cc, BIT(16) },
	[RST_BUS_GMAC0_AXI]		= { 0x140c, BIT(17) },
	[RST_BUS_GMAC0_AHB]		= { 0x140c, BIT(16) },
	[RST_BUS_TCON_LCD0]		= { 0x1504, BIT(16) },
	[RST_BUS_LVDS0]		= { 0x1544, BIT(16) },
	[RST_BUS_DSI0]			= { 0x1584, BIT(16) },
	[RST_BUS_TCON_TV0]		= { 0x1604, BIT(16) },
	[RST_BUS_EDP]			= { 0x164c, BIT(16) },
	[RST_BUS_VO0_REG]		= { 0x16c4, BIT(16) },
	[RST_BUS_VO1_REG]		= { 0x16cc, BIT(16) },
	[RST_BUS_VIDEO_OUT0]		= { 0x16e4, BIT(16) },
	[RST_BUS_VIDEO_OUT1]		= { 0x16ec, BIT(16) },
	[RST_BUS_LEDC]			= { 0x1704, BIT(16) },
	[RST_BUS_VIDEO_IN]		= { 0x1884, BIT(16) },
};
/* rst_def_end */

/* ccu_def_start */
static struct clk_hw_onecell_data sun65iw1_hw_clks = {
	.hws    = {
		[CLK_PLL_PERI0_PARENT]		= &pll_peri0_parent_clk.common.hw,
		[CLK_PLL_PERI0_2X]		= &pll_peri0_2x_clk.common.hw,
		[CLK_PLL_PERI0_DIV3]		= &pll_peri0_div3_clk.hw,
		[CLK_PLL_PERI0_800M]		= &pll_peri0_800m_clk.common.hw,
		[CLK_PLL_PERI0_480M]		= &pll_peri0_480m_clk.common.hw,
		[CLK_PLL_PERI0_600M]		= &pll_peri0_600m_clk.hw,
		[CLK_PLL_PERI0_400M]		= &pll_peri0_400m_clk.hw,
		[CLK_PLL_PERI0_300M]		= &pll_peri0_300m_clk.hw,
		[CLK_PLL_PERI0_200M]		= &pll_peri0_200m_clk.hw,
		[CLK_PLL_PERI0_160M]		= &pll_peri0_160m_clk.hw,
		[CLK_PLL_PERI0_16M]		= &pll_peri0_16m_clk.hw,
		[CLK_PLL_PERI0_150M]		= &pll_peri0_150m_clk.hw,
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
		[CLK_PLL_OUTP]			= &pll_outp_clk.common.hw,
		[CLK_PLL_GPU]			= &pll_gpu_clk.common.hw,
		[CLK_PLL_VIDEO0_PARENT]		= &pll_video0_parent_clk.common.hw,
		[CLK_PLL_VIDEO0_4X]		= &pll_video0_4x_clk.common.hw,
		[CLK_PLL_VIDEO0_3X]		= &pll_video0_3x_clk.common.hw,
		[CLK_PLL_VIDEO1_PARENT]		= &pll_video1_parent_clk.common.hw,
		[CLK_PLL_VIDEO1_4X]		= &pll_video1_4x_clk.common.hw,
		[CLK_PLL_VIDEO1_3X]		= &pll_video1_3x_clk.common.hw,
		[CLK_PLL_VIDEO2_PARENT]		= &pll_video2_parent_clk.common.hw,
		[CLK_PLL_VIDEO2_4X]		= &pll_video2_4x_clk.common.hw,
		[CLK_PLL_VIDEO2_3X]		= &pll_video2_3x_clk.common.hw,
		[CLK_PLL_VE]			= &pll_ve_clk.common.hw,
		[CLK_AHB]			= &ahb_clk.common.hw,
		[CLK_APB0]			= &apb0_clk.common.hw,
		[CLK_APB1]			= &apb1_clk.common.hw,
		[CLK_APB_UART]			= &apb_uart_clk.common.hw,
		[CLK_CCI]			= &cci_clk.common.hw,
		[CLK_GIC]			= &gic_clk.common.hw,
		[CLK_NSI]			= &nsi_clk.common.hw,
		[CLK_NSI_CFG]			= &nsi_cfg_clk.common.hw,
		[CLK_MBUS]			= &mbus_clk.common.hw,
		[CLK_APB_IOMMU]			= &iommu_apb_clk.common.hw,
		[CLK_STBY_PERI0PLL_CLK_GATE]		= &stby_peri0pll_clk_gate_clk.common.hw,
		[CLK_AHB_SMHC2_GATE]		= &smhc2_ahb_gate_clk.common.hw,
		[CLK_AHB_SMHC1_GATE]		= &smhc1_ahb_gate_clk.common.hw,
		[CLK_AHB_SMHC0_GATE]		= &smhc0_ahb_gate_clk.common.hw,
		[CLK_AHB_HSI_GATE]		= &hsi_ahb_gate_clk.common.hw,
		[CLK_AHB_USB1_GATE]		= &usb1_ahb_gate_clk.common.hw,
		[CLK_AHB_USB0_GATE]		= &usb0_ahb_gate_clk.common.hw,
		[CLK_AHB_SECURE_SYS_GATE]		= &secure_sys_ahb_gate_clk.common.hw,
		[CLK_AHB_GPU_GATE]		= &gpu_ahb_gate_clk.common.hw,
		[CLK_AHB_VIDEO_OUT1_GATE]		= &video_out1_ahb_gate_clk.common.hw,
		[CLK_AHB_VIDEO_OUT0_GATE]		= &video_out0_ahb_gate_clk.common.hw,
		[CLK_AHB_VIDEO_IN_GATE]		= &video_in_ahb_gate_clk.common.hw,
		[CLK_AHB_VE0_GATE]		= &ve0_ahb_gate_clk.common.hw,
		[CLK_MBUS_DMAC0_GATE]		= &dmac0_mbus_gate_clk.common.hw,
		[CLK_GMAC0_AXI_GATE]		= &gmac0_axi_gate_clk.common.hw,
		[CLK_CE_SYS_AXI_GATE]		= &ce_sys_axi_gate_clk.common.hw,
		[CLK_GPU_AXI_GATE]		= &gpu_axi_gate_clk.common.hw,
		[CLK_MBUS_DE_SYS_GATE]		= &de_sys_mbus_gate_clk.common.hw,
		[CLK_MBUS_VIDEO_OUT0_GATE]		= &video_out0_mbus_gate_clk.common.hw,
		[CLK_MBUS_VIDEO_IN_GATE]		= &video_in_mbus_gate_clk.common.hw,
		[CLK_MBUS_VE0_GATE]		= &ve0_mbus_gate_clk.common.hw,
		[CLK_AHB_DMA]			= &dma_ahb_clk.common.hw,
		[CLK_AHB_SPINLOCK]		= &spinlock_ahb_clk.common.hw,
		[CLK_AHB_MSGBOX_CPUX]		= &msgbox_cpux_ahb_clk.common.hw,
		[CLK_AHB_MSGBOX_CPUS]		= &msgbox_cpus_ahb_clk.common.hw,
		[CLK_APB_PWM0]			= &pwm0_apb_clk.common.hw,
		[CLK_DCU]			= &dcu_clk.common.hw,
		[CLK_AHB_DAP]			= &dap_ahb_clk.common.hw,
		[CLK_TIMER0]			= &timer0_clk.common.hw,
		[CLK_TIMER1]			= &timer1_clk.common.hw,
		[CLK_TIMER2]			= &timer2_clk.common.hw,
		[CLK_TIMER3]			= &timer3_clk.common.hw,
		[CLK_AHB_TIMER]			= &timer_ahb_clk.common.hw,
		[CLK_DE]			= &de_clk.common.hw,
		[CLK_AHB_DE0]			= &de0_ahb_clk.common.hw,
		[CLK_G2D]			= &g2d_clk.common.hw,
		[CLK_AHB_G2D]			= &g2d_ahb_clk.common.hw,
		[CLK_EINK_PANEL]		= &eink_panel_clk.common.hw,
		[CLK_AHB_EINK]			= &eink_ahb_clk.common.hw,
		[CLK_VE0]			= &ve0_clk.common.hw,
		[CLK_AHB_VE0]			= &ve0_ahb_clk.common.hw,
		[CLK_CE_SYS]			= &ce_sys_clk.common.hw,
		[CLK_AHB_GPU]			= &gpu_ahb_clk.common.hw,
		[CLK_AHB_MEMC]			= &memc_ahb_clk.common.hw,
		[CLK_SMHC0]			= &smhc0_clk.common.hw,
		[CLK_BUS_SMHC0]			= &smhc0_bus_clk.common.hw,
		[CLK_SMHC1]			= &smhc1_clk.common.hw,
		[CLK_BUS_SMHC1]			= &smhc1_bus_clk.common.hw,
		[CLK_SMHC2]			= &smhc2_clk.common.hw,
		[CLK_BUS_SMHC2]			= &smhc2_bus_clk.common.hw,
		[CLK_BUS_UART0]			= &uart0_bus_clk.common.hw,
		[CLK_BUS_UART1]			= &uart1_bus_clk.common.hw,
		[CLK_BUS_UART2]			= &uart2_bus_clk.common.hw,
		[CLK_BUS_UART3]			= &uart3_bus_clk.common.hw,
		[CLK_BUS_UART4]			= &uart4_bus_clk.common.hw,
		[CLK_BUS_UART5]			= &uart5_bus_clk.common.hw,
		[CLK_BUS_UART6]			= &uart6_bus_clk.common.hw,
		[CLK_BUS_UART7]			= &uart7_bus_clk.common.hw,
		[CLK_BUS_TWI0]			= &twi0_bus_clk.common.hw,
		[CLK_BUS_TWI1]			= &twi1_bus_clk.common.hw,
		[CLK_BUS_TWI2]			= &twi2_bus_clk.common.hw,
		[CLK_BUS_TWI3]			= &twi3_bus_clk.common.hw,
		[CLK_BUS_TWI4]			= &twi4_bus_clk.common.hw,
		[CLK_BUS_TWI5]			= &twi5_bus_clk.common.hw,
		[CLK_SPI0]			= &spi0_clk.common.hw,
		[CLK_AHB_SPI0]			= &spi0_ahb_clk.common.hw,
		[CLK_SPI1]			= &spi1_clk.common.hw,
		[CLK_AHB_SPI1]			= &spi1_ahb_clk.common.hw,
		[CLK_SPI2]			= &spi2_clk.common.hw,
		[CLK_AHB_SPI2]			= &spi2_ahb_clk.common.hw,
		[CLK_GPADC0]			= &gpadc0_clk.common.hw,
		[CLK_APB_GPADC0]		= &gpadc0_apb_clk.common.hw,
		[CLK_APB_TSENSOR]		= &tsensor_apb_clk.common.hw,
		[CLK_IRRX0]			= &irrx0_clk.common.hw,
		[CLK_APB_IRRX0]			= &irrx0_apb_clk.common.hw,
		[CLK_IRTX]			= &irtx_clk.common.hw,
		[CLK_APB_IRTX]			= &irtx_apb_clk.common.hw,
		[CLK_I2S0]			= &i2s0_clk.common.hw,
		[CLK_APB_I2S0]			= &i2s0_apb_clk.common.hw,
		[CLK_I2S1]			= &i2s1_clk.common.hw,
		[CLK_APB_I2S1]			= &i2s1_apb_clk.common.hw,
		[CLK_I2S2]			= &i2s2_clk.common.hw,
		[CLK_APB_I2S2]			= &i2s2_apb_clk.common.hw,
		[CLK_I2S3]			= &i2s3_clk.common.hw,
		[CLK_I2S3_ASRC]			= &i2s3_asrc_clk.common.hw,
		[CLK_APB_I2S3]			= &i2s3_apb_clk.common.hw,
		[CLK_OWA0_TX]			= &owa0_tx_clk.common.hw,
		[CLK_OWA0_RX]			= &owa0_rx_clk.common.hw,
		[CLK_APB_OWA0]			= &owa0_apb_clk.common.hw,
		[CLK_DMIC]			= &dmic_clk.common.hw,
		[CLK_APB_DMIC]			= &dmic_apb_clk.common.hw,
		[CLK_ADDA_DAC]			= &adda_dac_clk.common.hw,
		[CLK_ADDA_ADC]			= &adda_adc_clk.common.hw,
		[CLK_APB_ADDA]			= &adda_apb_clk.common.hw,
		[CLK_USB]			= &usb_clk.common.hw,
		[CLK_USB0_DEV]			= &usb0_dev_clk.common.hw,
		[CLK_USB0_EHCI]			= &usb0_ehci_clk.common.hw,
		[CLK_USB0_OHCI]			= &usb0_ohci_clk.common.hw,
		[CLK_BUS_USB]			= &usb_bus_clk.common.hw,
		[CLK_USB1_EHCI]			= &usb1_ehci_clk.common.hw,
		[CLK_USB1_OHCI]			= &usb1_ohci_clk.common.hw,
		[CLK_USB0_USB1_PHY_REF]		= &usb0_usb1_phy_ref_clk.common.hw,
		[CLK_AHB_USB0_USB1]		= &usb0_usb1_ahb_clk.common.hw,
		[CLK_USB2_U2_PHY_REF]		= &usb2_u2_phy_ref_clk.common.hw,
		[CLK_USB2_SUSPEND]		= &usb2_suspend_clk.common.hw,
		[CLK_USB2_REF]			= &usb2_ref_clk.common.hw,
		[CLK_USB2_U3_ONLY_UTMI]		= &usb2_u3_only_utmi_clk.common.hw,
		[CLK_USB2_U2_ONLY_PIPE]		= &usb2_u2_only_pipe_clk.common.hw,
		[CLK_PCIE0_AUX]			= &pcie0_aux_clk.common.hw,
		[CLK_PCIE0_AXI_S]		= &pcie0_axi_s_clk.common.hw,
		[CLK_HSI_PHY_CFG]		= &hsi_phy_cfg_clk.common.hw,
		[CLK_HSI_PHY_REF]		= &hsi_phy_ref_clk.common.hw,
		[CLK_HSI_SYS]			= &hsi_sys_clk.common.hw,
		[CLK_HSI_AXI]			= &hsi_axi_clk.common.hw,
		[CLK_GMAC0_PHY]			= &gmac0_phy_clk.common.hw,
		[CLK_AHB_GMAC0]			= &gmac0_ahb_clk.common.hw,
		[CLK_DSI0]			= &dsi0_clk.common.hw,
		[CLK_AHB_DSI]			= &dsi_ahb_clk.common.hw,
		[CLK_TCON_TV0]			= &tcon_tv0_clk.common.hw,
		[CLK_EDP]			= &edp_clk.common.hw,
		[CLK_AHB_VO0]			= &vo0_ahb_clk.common.hw,
		[CLK_AHB_VO1]			= &vo1_ahb_clk.common.hw,
		[CLK_LEDC]			= &ledc_clk.common.hw,
		[CLK_APB_LEDC]			= &ledc_apb_clk.common.hw,
		[CLK_CSI_MASTER0]		= &csi_master0_clk.common.hw,
		[CLK_CSI_MASTER1]		= &csi_master1_clk.common.hw,
		[CLK_CSI_MASTER2]		= &csi_master2_clk.common.hw,
		[CLK_CSI_MASTER3]		= &csi_master3_clk.common.hw,
		[CLK_CSI]			= &csi_clk.common.hw,
		[CLK_ISP]			= &isp_clk.common.hw,
		[CLK_VIDEO_IN]			= &video_in_clk.common.hw,
	},
	.num = CLK_NUMBER,
};
/* ccu_def_end */

static struct ccu_common *sun65iw1_ccu_clks[] = {
	&pll_peri0_parent_clk.common,
	&pll_peri0_2x_clk.common,
	&pll_peri0_800m_clk.common,
	&pll_peri0_480m_clk.common,
	&pll_peri1_parent_clk.common,
	&pll_peri1_2x_clk.common,
	&pll_peri1_800m_clk.common,
	&pll_peri1_480m_clk.common,
	&pll_outp_clk.common,
	&pll_gpu_clk.common,
	&pll_video0_parent_clk.common,
	&pll_video0_4x_clk.common,
	&pll_video0_3x_clk.common,
	&pll_video1_parent_clk.common,
	&pll_video1_4x_clk.common,
	&pll_video1_3x_clk.common,
	&pll_video2_parent_clk.common,
	&pll_video2_4x_clk.common,
	&pll_video2_3x_clk.common,
	&pll_ve_clk.common,
	&ahb_clk.common,
	&apb0_clk.common,
	&apb1_clk.common,
	&apb_uart_clk.common,
	&cci_clk.common,
	&gic_clk.common,
	&nsi_clk.common,
	&nsi_cfg_clk.common,
	&mbus_clk.common,
	&iommu_apb_clk.common,
	&stby_peri0pll_clk_gate_clk.common,
	&smhc2_ahb_gate_clk.common,
	&smhc1_ahb_gate_clk.common,
	&smhc0_ahb_gate_clk.common,
	&hsi_ahb_gate_clk.common,
	&usb1_ahb_gate_clk.common,
	&usb0_ahb_gate_clk.common,
	&secure_sys_ahb_gate_clk.common,
	&gpu_ahb_gate_clk.common,
	&video_out1_ahb_gate_clk.common,
	&video_out0_ahb_gate_clk.common,
	&video_in_ahb_gate_clk.common,
	&ve0_ahb_gate_clk.common,
	&dmac0_mbus_gate_clk.common,
	&gmac0_axi_gate_clk.common,
	&ce_sys_axi_gate_clk.common,
	&gpu_axi_gate_clk.common,
	&de_sys_mbus_gate_clk.common,
	&video_out0_mbus_gate_clk.common,
	&video_in_mbus_gate_clk.common,
	&ve0_mbus_gate_clk.common,
	&dma_ahb_clk.common,
	&spinlock_ahb_clk.common,
	&msgbox_cpux_ahb_clk.common,
	&msgbox_cpus_ahb_clk.common,
	&pwm0_apb_clk.common,
	&dcu_clk.common,
	&dap_ahb_clk.common,
	&timer0_clk.common,
	&timer1_clk.common,
	&timer2_clk.common,
	&timer3_clk.common,
	&timer_ahb_clk.common,
	&de_clk.common,
	&de0_ahb_clk.common,
	&g2d_clk.common,
	&g2d_ahb_clk.common,
	&eink_panel_clk.common,
	&eink_ahb_clk.common,
	&ve0_clk.common,
	&ve0_ahb_clk.common,
	&ce_sys_clk.common,
	&gpu_ahb_clk.common,
	&memc_ahb_clk.common,
	&smhc0_clk.common,
	&smhc0_bus_clk.common,
	&smhc1_clk.common,
	&smhc1_bus_clk.common,
	&smhc2_clk.common,
	&smhc2_bus_clk.common,
	&uart0_bus_clk.common,
	&uart1_bus_clk.common,
	&uart2_bus_clk.common,
	&uart3_bus_clk.common,
	&uart4_bus_clk.common,
	&uart5_bus_clk.common,
	&uart6_bus_clk.common,
	&uart7_bus_clk.common,
	&twi0_bus_clk.common,
	&twi1_bus_clk.common,
	&twi2_bus_clk.common,
	&twi3_bus_clk.common,
	&twi4_bus_clk.common,
	&twi5_bus_clk.common,
	&spi0_clk.common,
	&spi0_ahb_clk.common,
	&spi1_clk.common,
	&spi1_ahb_clk.common,
	&spi2_clk.common,
	&spi2_ahb_clk.common,
	&gpadc0_clk.common,
	&gpadc0_apb_clk.common,
	&tsensor_apb_clk.common,
	&irrx0_clk.common,
	&irrx0_apb_clk.common,
	&irtx_clk.common,
	&irtx_apb_clk.common,
	&i2s0_clk.common,
	&i2s0_apb_clk.common,
	&i2s1_clk.common,
	&i2s1_apb_clk.common,
	&i2s2_clk.common,
	&i2s2_apb_clk.common,
	&i2s3_clk.common,
	&i2s3_asrc_clk.common,
	&i2s3_apb_clk.common,
	&owa0_tx_clk.common,
	&owa0_rx_clk.common,
	&owa0_apb_clk.common,
	&dmic_clk.common,
	&dmic_apb_clk.common,
	&adda_dac_clk.common,
	&adda_adc_clk.common,
	&adda_apb_clk.common,
	&usb_clk.common,
	&usb0_dev_clk.common,
	&usb0_ehci_clk.common,
	&usb0_ohci_clk.common,
	&usb_bus_clk.common,
	&usb1_ehci_clk.common,
	&usb1_ohci_clk.common,
	&usb0_usb1_phy_ref_clk.common,
	&usb0_usb1_ahb_clk.common,
	&usb2_u2_phy_ref_clk.common,
	&usb2_suspend_clk.common,
	&usb2_ref_clk.common,
	&usb2_u3_only_utmi_clk.common,
	&usb2_u2_only_pipe_clk.common,
	&pcie0_aux_clk.common,
	&pcie0_axi_s_clk.common,
	&hsi_phy_cfg_clk.common,
	&hsi_phy_ref_clk.common,
	&hsi_sys_clk.common,
	&hsi_axi_clk.common,
	&gmac0_phy_clk.common,
	&gmac0_ahb_clk.common,
	&dsi0_clk.common,
	&dsi_ahb_clk.common,
	&tcon_tv0_clk.common,
	&edp_clk.common,
	&vo0_ahb_clk.common,
	&vo1_ahb_clk.common,
	&ledc_clk.common,
	&ledc_apb_clk.common,
	&csi_master0_clk.common,
	&csi_master1_clk.common,
	&csi_master2_clk.common,
	&csi_master3_clk.common,
	&csi_clk.common,
	&isp_clk.common,
	&video_in_clk.common,
};

static const struct sunxi_ccu_desc sun65iw1_ccu_desc = {
	.ccu_clks	= sun65iw1_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun65iw1_ccu_clks),

	.hw_clks	= &sun65iw1_hw_clks,

	.resets		= sun65iw1_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun65iw1_ccu_resets),
};

static const u32 sun65iw1_pll_regs[] = {
	SUN65IW1_PLL_PERI0_CTRL_REG,
	SUN65IW1_PLL_PERI1_CTRL_REG,
	SUN65IW1_PLL_GPU_CTRL_REG,
	SUN65IW1_PLL_VIDEO0_CTRL_REG,
	SUN65IW1_PLL_VIDEO1_CTRL_REG,
	SUN65IW1_PLL_VIDEO2_CTRL_REG,
	SUN65IW1_PLL_VE_CTRL_REG,
};

static int sun65iw1_ccu_probe(struct platform_device *pdev)
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

	/* Enable the lock_en bits on all PLLs */
	for (i = 0; i < ARRAY_SIZE(sun65iw1_pll_regs); i++) {
		set_reg(reg + sun65iw1_pll_regs[i], 0x1, 1, 29);
	}

	ret = sunxi_ccu_probe(pdev->dev.of_node, reg, &sun65iw1_ccu_desc);
	if (ret)
		return ret;

	sunxi_ccu_sleep_init(reg, sun65iw1_ccu_clks,
			ARRAY_SIZE(sun65iw1_ccu_clks),
			NULL, 0);

	return 0;
}

static const struct of_device_id sun65iw1_ccu_ids[] = {
	{ .compatible = "allwinner,sun65iw1-ccu" },
	{ }
};

static struct platform_driver sun65iw1_ccu_driver = {
	.probe	= sun65iw1_ccu_probe,
	.driver	= {
		.name	= "sun65iw1-ccu",
		.of_match_table	= sun65iw1_ccu_ids,
	},
};

static int __init sun65iw1_ccu_init(void)
{
	int err;

	err = platform_driver_register(&sun65iw1_ccu_driver);
	if (err)
		pr_err("register ccu sun65iw1 failed\n");

	return err;
}

core_initcall(sun65iw1_ccu_init);

static void __exit sun65iw1_ccu_exit(void)
{
	platform_driver_unregister(&sun65iw1_ccu_driver);
}
module_exit(sun65iw1_ccu_exit);

MODULE_DESCRIPTION("Allwinner sun65iw1 clk driver");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.0.3");
