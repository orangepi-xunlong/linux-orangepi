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

#include "ccu-sun8iw21.h"

/* ccu_des_start */

#define SUN8IW21_PLL_CPU_CTRL_REG   0x0000
static struct ccu_nkmp pll_cpu_clk = {
	.enable		= BIT(27) | BIT(30) | BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.p		= _SUNXI_CCU_DIV(16, 3), /* output divider */
	.p_reg		= 0x0500,
	.common		= {
		.reg		= 0x0000,
		.hw.init	= CLK_HW_INIT("pll-cpu", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE),
	},
};

#define SUN8IW21_PLL_DDR_CTRL_REG   0x0010
static struct ccu_nkmp pll_ddr_clk = {
	.enable		= BIT(27) | BIT(30) | BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(0, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(1, 1), /* output divider */
	.common		= {
		.reg		= 0x0010,
		.hw.init	= CLK_HW_INIT("pll-ddr", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN8IW21_PLL_PERI_CTRL_REG   0x0020
static struct ccu_nm pll_peri_parent_clk = {
	.enable		= BIT(27) | BIT(30) | BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.common		= {
		.reg		= 0x0020,
		.hw.init	= CLK_HW_INIT("pll-peri-parent", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static SUNXI_CCU_M(pll_peri_2x_clk, "pll-peri-2x",
		"pll-peri-parent", 0x0020, 16, 3, 0);

static CLK_FIXED_FACTOR_HW(pll_peri_div3_clk, "pll-peri-div3",
		&pll_peri_2x_clk.common.hw,
		6, 1, 0);

static SUNXI_CCU_M(pll_peri_800m_clk, "pll-peri-800m",
		"pll-peri-parent", 0x0020, 20, 3, 0);

static SUNXI_CCU_M(pll_peri_480m_clk, "pll-peri-480m",
		"pll-peri-parent", 0x0020, 2, 3, 0);

static CLK_FIXED_FACTOR_HW(pll_peri_600m_clk, "pll-peri-600m",
		&pll_peri_2x_clk.common.hw,
		2, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri_400m_clk, "pll-peri-400m",
		&pll_peri_2x_clk.common.hw,
		3, 1, 0);

static CLK_FIXED_FACTOR(pll_peri_300m_clk, "pll-peri-300m",
		"pll-peri-600m",
		2, 1, 0);

static CLK_FIXED_FACTOR(pll_peri_200m_clk, "pll-peri-200m",
		"pll-peri-400m",
		2, 1, 0);

static CLK_FIXED_FACTOR(pll_peri_160m_clk, "pll-peri-160m",
		"pll-peri-480m",
		3, 1, 0);

static CLK_FIXED_FACTOR(pll_peri_150m_clk, "pll-peri-150m",
		"pll-peri-300m",
		2, 1, 0);

#define SUN8IW21_PLL_VIDEO_CTRL_REG   0x0040
static struct ccu_nkmp pll_video_4x_clk = {
	.enable		= BIT(27) | BIT(30) | BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.p		= _SUNXI_CCU_DIV(1, 1), /* output divider */
	.common		= {
		.reg		= 0x0040,
		.hw.init	= CLK_HW_INIT("pll-video-4x", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE),
	},
};

static CLK_FIXED_FACTOR(pll_video_2x_clk, "pll-video-2x",
		"pll-video-4x",
		2, 1, CLK_SET_RATE_PARENT);

static CLK_FIXED_FACTOR(pll_video_1x_clk, "pll-video-1x",
		"pll-video-4x",
		4, 1, CLK_SET_RATE_PARENT);

#define SUN8IW21_PLL_CSI_CTRL_REG   0x0048
static struct ccu_nkmp pll_csi_4x_clk = {
	.enable		= BIT(27) | BIT(30) | BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(0, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(1, 1), /* output divider */
	.common		= {
		.reg		= 0x0048,
		.hw.init	= CLK_HW_INIT("pll-csi-4x", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE),
	},
};

#define SUN8IW21_PLL_AUDIO_CTRL_REG   0x0078
static struct ccu_sdm_setting pll_audio_div5_sdm_table[] = {
	{ .rate = 196608000, .pattern = 0xC001EB85, .m = 5, .n = 40 }, /* 24.576 */
	{ .rate = 67737600, .pattern = 0xC001288D, .m = 8, .n = 22 }, /* 22.5792 */
};

static struct ccu_nm pll_audio_div5_clk = {
	.enable		= BIT(27) | BIT(30) | BIT(31),
	.lock           = BIT(28),
	.n              = _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m              = _SUNXI_CCU_DIV(20, 3),
	.sdm            = _SUNXI_CCU_SDM(pll_audio_div5_sdm_table, BIT(24),
			0x0178, BIT(31)),
	.common         = {
		.reg            = 0x0078,
		.features       = CCU_FEATURE_SIGMA_DELTA_MOD,
		.hw.init        = CLK_HW_INIT("pll-audio-div5", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE),
	},
};
static SUNXI_CCU_M(pll_audio_1x_clk, "pll-audio-1x", "pll-audio-div5", 0xE00, 0, 3, CLK_SET_RATE_PARENT);

/* pll-audio-div2 and pll-aduio-4x not used, because audio-1x can cover 22.5792M and 24.576M */
static SUNXI_CCU_M(pll_audio_div2_clk, "pll-audio-div2", "pll-audio", 0x078, 16, 3, CLK_SET_RATE_PARENT);
static SUNXI_CCU_M(pll_audio_4x_clk, "pll-audio-4x", "pll-audio-div2", 0xE00, 5, 5, CLK_SET_RATE_PARENT);

#define SUN8IW21_PLL_NPU_CTRL_REG   0x0080
static struct ccu_nkmp pll_npu_4x_clk = {
	.enable		= BIT(27) | BIT(30) | BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.p		= _SUNXI_CCU_DIV(1, 1), /* output divider */
	.common		= {
		.reg		= 0x0080,
		.hw.init	= CLK_HW_INIT("pll-npu-4x", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE),
	},
};

static const char * const cpu_parents[] = { "dcxo24M", "osc32k",
					     "iosc", "pll-cpu",
					     "pll-periph0-600m",
					     "pll-periph0-800M" };

static SUNXI_CCU_MUX(cpu_clk, "cpu", cpu_parents,
		     0x500, 24, 3, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);

static SUNXI_CCU_M(cpu_axi_clk, "axi", "cpu", 0x500, 0, 2, 0);

static SUNXI_CCU_M(cpu_apb_clk, "apb", "cpu", 0x500, 8, 2, 0);

static SUNXI_CCU_GATE(cpu_bus_clk, "cpu-bus",
		"dcxo24M",
		0x0504, BIT(0), 0);

static const char * const ahb_parents[] = { "dcxo24M", "osc32k", "rc-16m", "pll-peri-600m" };

static SUNXI_CCU_MP_WITH_MUX(ahb_clk, "ahb",
		ahb_parents,
		0x0510,
		0, 5,	/* M */
		8, 2,	/* P */
		24, 2,	/* mux */
		CLK_IS_CRITICAL);

static const char * const apb0_parents[] = { "dcxo24M", "osc32k", "rc-16m", "pll-peri-600m" };

static SUNXI_CCU_MP_WITH_MUX(apb0_clk, "apb0",
		apb0_parents,
		0x0520,
		0, 5,	/* M */
		8, 2,	/* P */
		24, 2,	/* mux */
		CLK_IS_CRITICAL);

static const char * const apb1_parents[] = { "dcxo24M", "osc32k", "rc-16m", "pll-peri-600m" };

static SUNXI_CCU_MP_WITH_MUX(apb1_clk, "apb1",
		apb1_parents,
		0x0524,
		0, 5,	/* M */
		8, 2,	/* P */
		24, 2,	/* mux */
		0);

static const char * const de_parents[] = { "pll-peri-300m", "pll-video-1x" };

static SUNXI_CCU_M_WITH_MUX_GATE(de_clk, "de",
		de_parents, 0x0600,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(de_bus_clk, "de-bus",
		"dcxo24M",
		0x060C, BIT(0), 0);

static const char * const g2d_parents[] = { "pll-peri-300m", "pll-video-1x" };

static SUNXI_CCU_M_WITH_MUX_GATE(g2d_clk, "g2d",
		g2d_parents, 0x0630,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(g2d_bus_clk, "g2d-bus",
		"dcxo24M",
		0x063C, BIT(0), 0);

static const char * const ce_parents[] = { "dcxo24M", "pll-peri-400m", "pll-peri-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(ce_clk, "ce",
		ce_parents, 0x0680,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(ce_sys_clk, "ce-sys",
		"dcxo24M",
		0x068C, BIT(1), 0);

static SUNXI_CCU_GATE(ce_bus_clk, "ce-bus", "dcxo24M",
		0x068C, BIT(0), 0);

static const char * const ve_parents[] = { "pll-peri-300m", "pll-peri-400m", "pll-peri-480m", "pll-npu-4x", "pll-video-4x", "pll-csi-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(ve_clk, "ve",
		ve_parents, 0x0690,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE_ASSOC(ve_bus_clk, "ve-bus",
		"dcxo24M",
		0x069C, BIT(0),
		0x0E04, BIT(1),
		0);

static const char * const npu_parents[] = { "pll-peri-480m", "pll-peri-600m", "pll-peri-800m", "pll-npu-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(npu_clk, "npu",
		npu_parents, 0x06E0,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE_ASSOC(npu_bus_clk, "npu-bus",
		"dcxo24M",
		0x06EC, BIT(0),
		0x0E04, BIT(0),
		0);

static SUNXI_CCU_GATE(dma_clk, "dma",
		"dcxo24M",
		0x070C, BIT(0), 0);

static SUNXI_CCU_GATE(msgbox1_clk, "msgbox1",
		"dcxo24M",
		0x071C, BIT(1), 0);

static SUNXI_CCU_GATE(msgbox0_clk, "msgbox0",
		"dcxo24M",
		0x071C, BIT(0), 0);

static SUNXI_CCU_GATE(spinlock_clk, "spinlock",
		"dcxo24M",
		0x072C, BIT(0), 0);

static SUNXI_CCU_GATE(hstimer_clk, "hstimer",
		"dcxo24M",
		0x073C, BIT(0), 0);

static SUNXI_CCU_GATE(avs_clk, "avs",
		"dcxo24M",
		0x0740, BIT(31), 0);

static SUNXI_CCU_GATE(dbgsys_clk, "dbgsys",
		"dcxo24M",
		0x078C, BIT(0), 0);

static SUNXI_CCU_GATE(pwm_clk, "pwm",
		"dcxo24M",
		0x07AC, BIT(0), 0);

static SUNXI_CCU_GATE(iommu_clk, "iommu",
		"dcxo24M",
		0x07BC, BIT(0), 0);

static const char * const dram_parents[] = { "pll-ddr", "pll-peri-2x", "pll-peri-800m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(dram_clk, "dram",
		dram_parents, 0x0800,
		0, 5,	/* M */
		8, 2,	/* N */
		24, 3,	/* mux */
		BIT(31), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE(npu_mbus_gate_clk, "npu-mbus-gate",
		"dcxo24M",
		0x0804, BIT(21), 0);

/*
 * vid_in can effect csi/isp, vid_out can effect de/di/g2d/tcan/comp.
 * for simplicity, we will keep these two clocks normally open.
 */
static SUNXI_CCU_GATE(vid_in_mbus_gate_clk, "vid-in-mbus-gate",
		"dcxo24M",
		0x0804, BIT(20), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(vid_out_mbus_gate_clk, "vid-out-mbus-gate",
		"dcxo24M",
		0x0804, BIT(19), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(g2d_mbus_clk, "g2d-mbus",
		"dcxo24M",
		0x0804, BIT(10), 0);

static SUNXI_CCU_GATE(isp_mbus_clk, "isp-mbus",
		"dcxo24M",
		0x0804, BIT(9), 0);

static SUNXI_CCU_GATE(csi_mbus_clk, "csi-mbus",
		"dcxo24M",
		0x0804, BIT(8), 0);

static SUNXI_CCU_GATE_ASSOC(ce_mbus_clk, "ce-mbus",
		"dcxo24M",
		0x0804, BIT(2),
		0x0804, BIT(18),
		0);

static SUNXI_CCU_GATE_ASSOC(ve_mbus_clk, "ve-mbus",
		"dcxo24M",
		0x0804, BIT(1),
		0x0804, BIT(17),
		0);

static SUNXI_CCU_GATE_ASSOC(dma_mbus_clk, "dma-mbus",
		"dcxo24M",
		0x0804, BIT(0),
		0x0804, BIT(16),
		0);

static SUNXI_CCU_GATE(dram_bus_clk, "dram-bus",
		"dcxo24M",
		0x080C, BIT(0), CLK_IS_CRITICAL);

static const char * const smhc0_parents[] = { "dcxo24M", "pll-peri-400m", "pll-peri-300m" };

static SUNXI_CCU_MP_WITH_MUX_GATE(smhc0_clk, "smhc0",
		smhc0_parents, 0x0830,
		0, 4,	/* M */
		8, 2, /* P */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_MP_WITH_MUX_GATE(smhc1_clk, "smhc1",
		smhc0_parents, 0x0834,
		0, 4,	/* M */
		8, 2, /* P */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static const char * const smhc2_parents[] = { "dcxo24M", "pll-peri-600m", "pll-peri-400m" };
static SUNXI_CCU_MP_WITH_MUX_GATE(smhc2_clk, "smhc2",
		smhc2_parents, 0x0838,
		0, 4,	/* M */
		8, 2, /* P */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE_ASSOC(smhc2_bus_clk, "smhc2-bus",
		"dcxo24M",
		0x084C, BIT(2),
		0x0E04, BIT(12) | BIT(7),
		0);

static SUNXI_CCU_GATE_ASSOC(smhc1_bus_clk, "smhc1-bus",
		"dcxo24M",
		0x084C, BIT(1),
		0x0E04, BIT(11) | BIT(6),
		0);

static SUNXI_CCU_GATE_ASSOC(smhc0_bus_clk, "smhc0-bus",
		"dcxo24M",
		0x084C, BIT(0),
		0x0E04, BIT(10) | BIT(5),
		0);

static SUNXI_CCU_GATE(uart3_clk, "uart3",
		"apb1",
		0x090C, BIT(3), 0);

static SUNXI_CCU_GATE(uart2_clk, "uart2",
		"apb1",
		0x090C, BIT(2), 0);

static SUNXI_CCU_GATE(uart1_clk, "uart1",
		"apb1",
		0x090C, BIT(1), 0);

static SUNXI_CCU_GATE(uart0_clk, "uart0",
		"apb1",
		0x090C, BIT(0), 0);

static SUNXI_CCU_GATE(twi4_clk, "twi4",
		"apb1",
		0x091C, BIT(4), 0);

static SUNXI_CCU_GATE(twi3_clk, "twi3",
		"apb1",
		0x091C, BIT(3), 0);

static SUNXI_CCU_GATE(twi2_clk, "twi2",
		"apb1",
		0x091C, BIT(2), 0);

static SUNXI_CCU_GATE(twi1_clk, "twi1",
		"apb1",
		0x091C, BIT(1), 0);

static SUNXI_CCU_GATE(twi0_clk, "twi0",
		"apb1",
		0x091C, BIT(0), 0);

static const char * const spi0_parents[] = { "dcxo24M", "pll-peri-300m", "pll-peri-200m" };

static SUNXI_CCU_MP_WITH_MUX_GATE(spi0_clk, "spi0",
		spi0_parents, 0x0940,
		0, 4,	/* M */
		8, 2, /* P */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static const char * const spi1_parents[] = { "dcxo24M", "pll-peri-300m", "pll-peri-200m" };
static SUNXI_CCU_MP_WITH_MUX_GATE(spi1_clk, "spi1",
		spi1_parents, 0x0944,
		0, 4,	/* M */
		8, 2, /* P */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static const char * const spi2_parents[] = { "dcxo24M", "pll-peri-300m", "pll-peri-200m" };
static SUNXI_CCU_MP_WITH_MUX_GATE(spi2_clk, "spi2",
		spi2_parents, 0x0948,
		0, 4,	/* M */
		8, 2, /* P */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static const char * const spi3_parents[] = { "dcxo24M", "pll-peri-300m", "pll-peri-200m" };
static SUNXI_CCU_MP_WITH_MUX_GATE(spi3_clk, "spi3",
		spi3_parents, 0x094C,
		0, 4,	/* M */
		8, 2, /* P */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static const char * const spif_parents[] = { "dcxo24M", "pll-peri-400m", "pll-peri-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(spif_clk, "spif",
		spif_parents, 0x0950,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(spif_bus_clk, "spif-bus",
		"dcxo24M",
		0x096C, BIT(4), 0);

static SUNXI_CCU_GATE(spi3_bus_clk, "spi3-bus",
		"dcxo24M",
		0x096C, BIT(3), 0);

static SUNXI_CCU_GATE(spi2_bus_clk, "spi2-bus",
		"dcxo24M",
		0x096C, BIT(2), 0);

static SUNXI_CCU_GATE(spi1_bus_clk, "spi1-bus",
		"dcxo24M",
		0x096C, BIT(1), 0);

static SUNXI_CCU_GATE(spi0_bus_clk, "spi0-bus",
		"dcxo24M",
		0x096C, BIT(0), 0);

static SUNXI_CCU_GATE(gmac_25m_clk, "gmac-25m",
		"dcxo24M",
		0x0970, BIT(31), 0);

static SUNXI_CCU_GATE(gmac_25m_clk_src_clk, "gmac-25m-clk-src",
		"dcxo24M",
		0x0970, BIT(30), 0);

static SUNXI_CCU_GATE_ASSOC(gmac_clk, "gmac",
		"dcxo24M",
		0x097C, BIT(0),
		0x0E04, BIT(8) | BIT(13),
		0);

static SUNXI_CCU_GATE(gpadc_clk, "gpadc",
		"dcxo24M",
		0x09EC, BIT(0), 0);

static SUNXI_CCU_GATE(ths_clk, "ths",
		"dcxo24M",
		0x09FC, BIT(0), 0);

static const char * const i2s0_parents[] = { "pll-audio-1x", "pll-audio-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(i2s0_clk, "i2s0",
		i2s0_parents, 0x0A10,
		0, 4,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const i2s1_parents[] = { "pll-audio-1x", "pll-audio-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(i2s1_clk, "i2s1",
		i2s1_parents, 0x0A14,
		0, 4,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(i2s1_bus_clk, "i2s1-bus",
		"dcxo24M",
		0x0A20, BIT(1), 0);

static SUNXI_CCU_GATE(i2s0_bus_clk, "i2s0-bus",
		"dcxo24M",
		0x0A20, BIT(0), 0);

static const char * const dmic_parents[] = { "pll-audio-1x", "pll-audio-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(dmic_clk, "dmic",
		dmic_parents, 0x0A40,
		0, 4,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(dmic_bus_clk, "dmic-bus",
		"dcxo24M",
		0x0A4C, BIT(0), 0);

static const char * const audio_codec_dac_parents[] = { "pll-audio-1x", "pll-audio-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(audio_codec_dac_clk, "audio-codec-dac",
		audio_codec_dac_parents, 0x0A50,
		0, 4,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const audio_codec_adc_parents[] = { "pll-audio-1x", "pll-audio-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(audio_codec_adc_clk, "audio-codec-adc",
		audio_codec_adc_parents, 0x0A54,
		0, 4,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(audio_codec_clk, "audio-codec",
		"dcxo24M",
		0x0A5C, BIT(0), 0);

static SUNXI_CCU_GATE(usb_clk, "usb",
		"dcxo24M",
		0x0A70, BIT(31), 0);

static SUNXI_CCU_GATE(usbotg0_clk, "usbotg0",
		"dcxo24M",
		0x0A8C, BIT(8), 0);

static SUNXI_CCU_GATE(usbehci0_clk, "usbehci0",
		"dcxo24M",
		0x0A8C, BIT(4), 0);

static SUNXI_CCU_GATE(usbohci0_clk, "usbohci0",
		"dcxo24M",
		0x0A8C, BIT(0), 0);

static SUNXI_CCU_GATE(dpss_top_clk, "dpss-top",
		"dcxo24M",
		0x0ABC, BIT(0), 0);

static const char * const dsi_parents[] = { "dcxo24M", "pll-peri-200m", "pll-peri-150m" };

static SUNXI_CCU_M_WITH_MUX_GATE(dsi_clk, "dsi",
		dsi_parents, 0x0B24,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(dsi_bus_clk, "dsi-bus",
		"dcxo24M",
		0x0B4C, BIT(0), 0);

static const char * const tconlcd_parents[] = { "pll-video-4x", "pll-peri-2x", "pll-csi-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(tconlcd_clk, "tconlcd",
		tconlcd_parents, 0x0B60,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(tconlcd_bus_clk, "tconlcd-bus",
		"dcxo24M",
		0x0B7C, BIT(0), 0);

static const char * const csi_parents[] = { "pll-peri-300m", "pll-peri-400m", "pll-video-4x", "pll-csi-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(csi_clk, "csi",
		csi_parents, 0x0C04,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const csi_master0_parents[] = { "dcxo24M", "pll-csi-4x", "pll-video-4x", "pll-peri-2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(csi_master0_clk, "csi-master0",
		csi_master0_parents, 0x0C08,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const csi_master1_parents[] = { "dcxo24M", "pll-csi-4x", "pll-video-4x", "pll-peri-2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(csi_master1_clk, "csi-master1",
		csi_master1_parents, 0x0C0C,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const csi_master2_parents[] = { "dcxo24M", "pll-csi-4x", "pll-video-4x", "pll-peri-2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(csi_master2_clk, "csi-master2",
		csi_master2_parents, 0x0C10,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(csi_bus_clk, "csi-bus",
		"dcxo24M",
		0x0C2C, BIT(0), 0);

static SUNXI_CCU_GATE(wiegand_clk, "wiegand",
		"dcxo24M",
		0x0C7C, BIT(0), 0);

static const char * const e907_core_div_clk_parents[] = {
	"dcxo24M", "osc32k", "rc-16m", "pll-peri-600m", "pll-peri-400m", "pll-cpu"
};

static SUNXI_CCU_M_WITH_MUX(e907_core_div_clk, "e907-core-div",
		e907_core_div_clk_parents, 0x0D00,
		0, 5,	/* M */
		24, 3,	/* mux */
		0);

#define E907_CFG_KEY_VALUE     0x16AA0000
static SUNXI_CCU_GATE_WITH_KEY(e907_core_rst_clk, "e907-core-rst",
		"dcxo24M", 0x0D04,
		E907_CFG_KEY_VALUE,
		BIT(1),
		0);

static SUNXI_CCU_GATE_WITH_KEY(e907_dbg_rst_clk, "e907-dbg-rst",
		"dcxo24M", 0x0D04,
		E907_CFG_KEY_VALUE,
		BIT(2),
		0);

static SUNXI_CCU_GATE_WITH_KEY(e907_core_gate_clk, "e907-core-gate",
		"e907-core-div", 0x0D04,
		E907_CFG_KEY_VALUE,
		BIT(0),
		0);

static SUNXI_CCU_M(e907_axi_div_clk, "e907-axi-div",
		"e907-core-gate", 0x0020, 8, 2, 0);

static SUNXI_CCU_GATE(riscv_cfg_clk, "riscv-cfg",
		"dcxo24M",
		0x0D0C, BIT(0), 0);

static SUNXI_CCU_GATE(cpus_hclk_gate_clk, "cpus-hclk-gate",
		"dcxo24M",
		0x0E04, BIT(28), CLK_IGNORE_UNUSED);

/*
 * 1. usb_ahb_gate_clk is ahb gate in module(default enable)
 * 2. usb_mbus_ahb_gate_clk is nsi gate in module(default enable)
 * 3. we don't add those to clk tree to simplify it
 */

#ifdef SUNXI_CCU_COMPLEX_CLK_TREE
static SUNXI_CCU_GATE(usb_mbus_ahb_gate_clk, "usb-mbus-ahb-gate",
		"dcxo24M",
		0x0E04, BIT(9), 0);

static SUNXI_CCU_GATE(usb_ahb_gate_clk, "usb-ahb-gate",
		"dcxo24M",
		0x0E04, BIT(4), 0);

static SUNXI_CCU_GATE(vid_out_ahb_gate_clk, "vid-out-ahb-gate",
		"dcxo24M",
		0x0E04, BIT(3), 0);

static SUNXI_CCU_GATE(vid_in_ahb_gate_clk, "vid-in-ahb-gate",
		"dcxo24M",
		0x0E04, BIT(2), 0);
#endif

static SUNXI_CCU_GATE(res_dcap_24m_clk, "res-dcap-24m",
		"dcxo24M",
		0x0E0C, BIT(3), 0);

static SUNXI_CCU_GATE(gpadc_24m_clk, "gpadc-24m",
		"dcxo24M",
		0x0E0C, BIT(2), 0);

static SUNXI_CCU_GATE(wiegand_24m_clk, "wiegand-24m",
		"dcxo24M",
		0x0E0C, BIT(1), 0);

static SUNXI_CCU_GATE(usb_24m_clk, "usb-24m",
		"dcxo24M",
		0x0E0C, BIT(0), CLK_IGNORE_UNUSED);

static const char * const gpadc_sel_parents[] = { "dcxo24M", "dcxo24M-16", "dcxo24M-8", "dcxo24M-4", "dcxo24M-2", "dcxo24M" };

static SUNXI_CCU_MUX(gpadc_sel_clk, "gpadc-sel", gpadc_sel_parents,
		0x0F04, 20, 3, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);
/* ccu_des_end */

/* rst_def_start */
static struct ccu_reset_map sun8iw21_ccu_resets[] = {
	[RST_MBUS]			= { 0x0540, BIT(30) },
	[RST_BUS_DE]			= { 0x060c, BIT(16) },
	[RST_BUS_G2D]			= { 0x063c, BIT(16) },
	[RST_BUS_CE_SY]			= { 0x068c, BIT(17) },
	[RST_BUS_CE]			= { 0x068c, BIT(16) },
	[RST_BUS_VE]			= { 0x069c, BIT(16) },
	[RST_BUS_NPU]			= { 0x06ec, BIT(16) },
	[RST_BUS_DMA]			= { 0x070c, BIT(16) },
	[RST_BUS_MSGBOX1]		= { 0x071c, BIT(17) },
	[RST_BUS_MSGBOX0]		= { 0x071c, BIT(16) },
	[RST_BUS_SPINLOCK]		= { 0x072c, BIT(16) },
	[RST_BUS_HSTIME]		= { 0x073c, BIT(16) },
	[RST_BUS_DBGSY]			= { 0x078c, BIT(16) },
	[RST_BUS_PWM]			= { 0x07ac, BIT(16) },
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
	[RST_BUS_SPIF]			= { 0x096c, BIT(20) },
	[RST_BUS_SPI3]			= { 0x096c, BIT(19) },
	[RST_BUS_SPI2]			= { 0x096c, BIT(18) },
	[RST_BUS_SPI1]			= { 0x096c, BIT(17) },
	[RST_BUS_SPI0]			= { 0x096c, BIT(16) },
	[RST_BUS_GMAC]			= { 0x097c, BIT(16) },
	[RST_BUS_GPADC]			= { 0x09ec, BIT(16) },
	[RST_BUS_TH]			= { 0x09fc, BIT(16) },
	[RST_BUS_I2S1]			= { 0x0a20, BIT(17) },
	[RST_BUS_I2S0]			= { 0x0a20, BIT(16) },
	[RST_BUS_DMIC]			= { 0x0a4c, BIT(16) },
	[RST_BUS_AUDIO_CODEC]		= { 0x0a5c, BIT(16) },
	[RST_USB_PHY0_RSTN]		= { 0x0a70, BIT(30) },
	[RST_USB_OTG0]			= { 0x0a8c, BIT(24) },
	[RST_USB_EHCI0]			= { 0x0a8c, BIT(20) },
	[RST_USB_OHCI0]			= { 0x0a8c, BIT(16) },
	[RST_BUS_DPSS_TOP]		= { 0x0abc, BIT(16) },
	[RST_BUS_DSI]			= { 0x0b4c, BIT(16) },
	[RST_BUS_TCONLCD]		= { 0x0b7c, BIT(16) },
	[RST_BUS_CSI]			= { 0x0c2c, BIT(16) },
	[RST_BUS_WIEGAND]		= { 0x0c7c, BIT(16) },
	[RST_BUS_RISCV_CFG]		= { 0x0d0c, BIT(16) },
};
/* rst_def_end */

/* ccu_def_start */
static struct clk_hw_onecell_data sun8iw21_hw_clks = {
	.hws    = {
		[CLK_PLL_CPU]			= &pll_cpu_clk.common.hw,
		[CLK_PLL_DDR]			= &pll_ddr_clk.common.hw,
		[CLK_PLL_PERI_PARENT]		= &pll_peri_parent_clk.common.hw,
		[CLK_PLL_PERI_2X]		= &pll_peri_2x_clk.common.hw,
		[CLK_PLL_PERI_DIV3]		= &pll_peri_div3_clk.hw,
		[CLK_PLL_PERI_800M]		= &pll_peri_800m_clk.common.hw,
		[CLK_PLL_PERI_480M]		= &pll_peri_480m_clk.common.hw,
		[CLK_PLL_PERI_600M]		= &pll_peri_600m_clk.hw,
		[CLK_PLL_PERI_400M]		= &pll_peri_400m_clk.hw,
		[CLK_PLL_PERI_300M]		= &pll_peri_300m_clk.hw,
		[CLK_PLL_PERI_200M]		= &pll_peri_200m_clk.hw,
		[CLK_PLL_PERI_160M]		= &pll_peri_160m_clk.hw,
		[CLK_PLL_PERI_150M]		= &pll_peri_150m_clk.hw,
		[CLK_PLL_VIDEO_4X]		= &pll_video_4x_clk.common.hw,
		[CLK_PLL_VIDEO_2X]		= &pll_video_2x_clk.hw,
		[CLK_PLL_VIDEO_1X]		= &pll_video_1x_clk.hw,
		[CLK_PLL_CSI_4X]		= &pll_csi_4x_clk.common.hw,
		[CLK_PLL_AUDIO_DIV2]		= &pll_audio_div2_clk.common.hw,
		[CLK_PLL_AUDIO_DIV5]		= &pll_audio_div5_clk.common.hw,
		[CLK_PLL_AUDIO_4X]		= &pll_audio_4x_clk.common.hw,
		[CLK_PLL_AUDIO_1X]		= &pll_audio_1x_clk.common.hw,
		[CLK_PLL_NPU_4X]		= &pll_npu_4x_clk.common.hw,
		[CLK_CPU]			= &cpu_clk.common.hw,
		[CLK_CPU_AXI]			= &cpu_axi_clk.common.hw,
		[CLK_CPU_APB]			= &cpu_apb_clk.common.hw,
		[CLK_BUS_CPU]			= &cpu_bus_clk.common.hw,
		[CLK_AHB]			= &ahb_clk.common.hw,
		[CLK_APB0]			= &apb0_clk.common.hw,
		[CLK_APB1]			= &apb1_clk.common.hw,
		[CLK_DE]			= &de_clk.common.hw,
		[CLK_BUS_DE]			= &de_bus_clk.common.hw,
		[CLK_G2D]			= &g2d_clk.common.hw,
		[CLK_BUS_G2D]			= &g2d_bus_clk.common.hw,
		[CLK_CE]			= &ce_clk.common.hw,
		[CLK_CE_SYS]			= &ce_sys_clk.common.hw,
		[CLK_BUS_CE]			= &ce_bus_clk.common.hw,
		[CLK_VE]			= &ve_clk.common.hw,
		[CLK_BUS_VE]			= &ve_bus_clk.common.hw,
		[CLK_NPU]			= &npu_clk.common.hw,
		[CLK_BUS_NPU]			= &npu_bus_clk.common.hw,
		[CLK_DMA]			= &dma_clk.common.hw,
		[CLK_MSGBOX1]			= &msgbox1_clk.common.hw,
		[CLK_MSGBOX0]			= &msgbox0_clk.common.hw,
		[CLK_SPINLOCK]			= &spinlock_clk.common.hw,
		[CLK_HSTIMER]			= &hstimer_clk.common.hw,
		[CLK_AVS]			= &avs_clk.common.hw,
		[CLK_DBGSYS]			= &dbgsys_clk.common.hw,
		[CLK_PWM]			= &pwm_clk.common.hw,
		[CLK_IOMMU]			= &iommu_clk.common.hw,
		[CLK_DRAM]			= &dram_clk.common.hw,
		[CLK_MBUS_NPU_GATE]		= &npu_mbus_gate_clk.common.hw,
		[CLK_MBUS_VID_IN_GATE]		= &vid_in_mbus_gate_clk.common.hw,
		[CLK_MBUS_VID_OUT_GATE]		= &vid_out_mbus_gate_clk.common.hw,
		[CLK_MBUS_G2D]			= &g2d_mbus_clk.common.hw,
		[CLK_MBUS_ISP]			= &isp_mbus_clk.common.hw,
		[CLK_MBUS_CSI]			= &csi_mbus_clk.common.hw,
		[CLK_MBUS_CE_GATE]		= &ce_mbus_clk.common.hw,
		[CLK_MBUS_VE]			= &ve_mbus_clk.common.hw,
		[CLK_MBUS_DMA]			= &dma_mbus_clk.common.hw,
		[CLK_BUS_DRAM]			= &dram_bus_clk.common.hw,
		[CLK_SMHC0]			= &smhc0_clk.common.hw,
		[CLK_SMHC1]			= &smhc1_clk.common.hw,
		[CLK_SMHC2]			= &smhc2_clk.common.hw,
		[CLK_BUS_SMHC2]			= &smhc2_bus_clk.common.hw,
		[CLK_BUS_SMHC1]			= &smhc1_bus_clk.common.hw,
		[CLK_BUS_SMHC0]			= &smhc0_bus_clk.common.hw,
		[CLK_UART3]			= &uart3_clk.common.hw,
		[CLK_UART2]			= &uart2_clk.common.hw,
		[CLK_UART1]			= &uart1_clk.common.hw,
		[CLK_UART0]			= &uart0_clk.common.hw,
		[CLK_TWI4]			= &twi4_clk.common.hw,
		[CLK_TWI3]			= &twi3_clk.common.hw,
		[CLK_TWI2]			= &twi2_clk.common.hw,
		[CLK_TWI1]			= &twi1_clk.common.hw,
		[CLK_TWI0]			= &twi0_clk.common.hw,
		[CLK_SPI0]			= &spi0_clk.common.hw,
		[CLK_SPI1]			= &spi1_clk.common.hw,
		[CLK_SPI2]			= &spi2_clk.common.hw,
		[CLK_SPI3]			= &spi3_clk.common.hw,
		[CLK_SPIF]			= &spif_clk.common.hw,
		[CLK_BUS_SPIF]			= &spif_bus_clk.common.hw,
		[CLK_BUS_SPI3]			= &spi3_bus_clk.common.hw,
		[CLK_BUS_SPI2]			= &spi2_bus_clk.common.hw,
		[CLK_BUS_SPI1]			= &spi1_bus_clk.common.hw,
		[CLK_BUS_SPI0]			= &spi0_bus_clk.common.hw,
		[CLK_GMAC_25M]			= &gmac_25m_clk.common.hw,
		[CLK_GMAC_25M_CLK_SRC]		= &gmac_25m_clk_src_clk.common.hw,
		[CLK_GMAC]			= &gmac_clk.common.hw,
		[CLK_GPADC]			= &gpadc_clk.common.hw,
		[CLK_THS]			= &ths_clk.common.hw,
		[CLK_I2S0]			= &i2s0_clk.common.hw,
		[CLK_I2S1]			= &i2s1_clk.common.hw,
		[CLK_BUS_I2S1]			= &i2s1_bus_clk.common.hw,
		[CLK_BUS_I2S0]			= &i2s0_bus_clk.common.hw,
		[CLK_DMIC]			= &dmic_clk.common.hw,
		[CLK_BUS_DMIC]			= &dmic_bus_clk.common.hw,
		[CLK_AUDIO_CODEC_DAC]		= &audio_codec_dac_clk.common.hw,
		[CLK_AUDIO_CODEC_ADC]		= &audio_codec_adc_clk.common.hw,
		[CLK_AUDIO_CODEC]		= &audio_codec_clk.common.hw,
		[CLK_USB]			= &usb_clk.common.hw,
		[CLK_USBOTG0]			= &usbotg0_clk.common.hw,
		[CLK_USBEHCI0]			= &usbehci0_clk.common.hw,
		[CLK_USBOHCI0]			= &usbohci0_clk.common.hw,
		[CLK_DPSS_TOP]			= &dpss_top_clk.common.hw,
		[CLK_DSI]			= &dsi_clk.common.hw,
		[CLK_BUS_DSI]			= &dsi_bus_clk.common.hw,
		[CLK_TCONLCD]			= &tconlcd_clk.common.hw,
		[CLK_BUS_TCONLCD]		= &tconlcd_bus_clk.common.hw,
		[CLK_CSI]			= &csi_clk.common.hw,
		[CLK_CSI_MASTER0]		= &csi_master0_clk.common.hw,
		[CLK_CSI_MASTER1]		= &csi_master1_clk.common.hw,
		[CLK_CSI_MASTER2]		= &csi_master2_clk.common.hw,
		[CLK_BUS_CSI]			= &csi_bus_clk.common.hw,
		[CLK_WIEGAND]			= &wiegand_clk.common.hw,
		[CLK_E907_CORE_DIV]		= &e907_core_div_clk.common.hw,
		[CLK_E907_AXI_DIV]		= &e907_axi_div_clk.common.hw,
		[CLK_E907_CORE_RST]		= &e907_core_rst_clk.common.hw,
		[CLK_E907_DBG_RST]		= &e907_dbg_rst_clk.common.hw,
		[CLK_E907_CORE_GATE]	= &e907_core_gate_clk.common.hw,
		[CLK_RISCV_CFG]			= &riscv_cfg_clk.common.hw,
		[CLK_CPUS_HCLK_GATE]		= &cpus_hclk_gate_clk.common.hw,
		[CLK_RES_DCAP_24M]		= &res_dcap_24m_clk.common.hw,
		[CLK_GPADC_24M]			= &gpadc_24m_clk.common.hw,
		[CLK_WIEGAND_24M]		= &wiegand_24m_clk.common.hw,
		[CLK_USB_24M]			= &usb_24m_clk.common.hw,
		[CLK_GPADC_SEL]			= &gpadc_sel_clk.common.hw,
	},
	.num = CLK_NUMBER,
};
/* ccu_def_end */

static struct ccu_common *sun8iw21_ccu_clks[] = {
	&pll_cpu_clk.common,
	&pll_ddr_clk.common,
	&pll_peri_parent_clk.common,
	&pll_peri_2x_clk.common,
	&pll_peri_800m_clk.common,
	&pll_peri_480m_clk.common,
	&pll_video_4x_clk.common,
	&pll_csi_4x_clk.common,
	&pll_audio_div2_clk.common,
	&pll_audio_div5_clk.common,
	&pll_audio_4x_clk.common,
	&pll_audio_1x_clk.common,
	&pll_npu_4x_clk.common,
	&cpu_clk.common,
	&cpu_axi_clk.common,
	&cpu_apb_clk.common,
	&cpu_bus_clk.common,
	&ahb_clk.common,
	&apb0_clk.common,
	&apb1_clk.common,
	&de_clk.common,
	&de_bus_clk.common,
	&g2d_clk.common,
	&g2d_bus_clk.common,
	&ce_clk.common,
	&ce_sys_clk.common,
	&ce_bus_clk.common,
	&ve_clk.common,
	&ve_bus_clk.common,
	&npu_clk.common,
	&npu_bus_clk.common,
	&dma_clk.common,
	&msgbox1_clk.common,
	&msgbox0_clk.common,
	&spinlock_clk.common,
	&hstimer_clk.common,
	&avs_clk.common,
	&dbgsys_clk.common,
	&pwm_clk.common,
	&iommu_clk.common,
	&dram_clk.common,
	&npu_mbus_gate_clk.common,
	&vid_in_mbus_gate_clk.common,
	&vid_out_mbus_gate_clk.common,
	&g2d_mbus_clk.common,
	&isp_mbus_clk.common,
	&csi_mbus_clk.common,
	&ce_mbus_clk.common,
	&ve_mbus_clk.common,
	&dma_mbus_clk.common,
	&dram_bus_clk.common,
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
	&spi3_clk.common,
	&spif_clk.common,
	&spif_bus_clk.common,
	&spi3_bus_clk.common,
	&spi2_bus_clk.common,
	&spi1_bus_clk.common,
	&spi0_bus_clk.common,
	&gmac_25m_clk.common,
	&gmac_25m_clk_src_clk.common,
	&gmac_clk.common,
	&gpadc_clk.common,
	&ths_clk.common,
	&i2s0_clk.common,
	&i2s1_clk.common,
	&i2s1_bus_clk.common,
	&i2s0_bus_clk.common,
	&dmic_clk.common,
	&dmic_bus_clk.common,
	&audio_codec_dac_clk.common,
	&audio_codec_adc_clk.common,
	&audio_codec_clk.common,
	&usb_clk.common,
	&usbotg0_clk.common,
	&usbehci0_clk.common,
	&usbohci0_clk.common,
	&dpss_top_clk.common,
	&dsi_clk.common,
	&dsi_bus_clk.common,
	&tconlcd_clk.common,
	&tconlcd_bus_clk.common,
	&csi_clk.common,
	&csi_master0_clk.common,
	&csi_master1_clk.common,
	&csi_master2_clk.common,
	&csi_bus_clk.common,
	&wiegand_clk.common,
	&e907_core_div_clk.common,
	&e907_axi_div_clk.common,
	&e907_core_rst_clk.common,
	&e907_dbg_rst_clk.common,
	&e907_core_gate_clk.common,
	&riscv_cfg_clk.common,
	&cpus_hclk_gate_clk.common,
	&res_dcap_24m_clk.common,
	&gpadc_24m_clk.common,
	&wiegand_24m_clk.common,
	&usb_24m_clk.common,
	&gpadc_sel_clk.common,
};

static const struct sunxi_ccu_desc sun8iw21_ccu_desc = {
	.ccu_clks	= sun8iw21_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun8iw21_ccu_clks),

	.hw_clks	= &sun8iw21_hw_clks,

	.resets		= sun8iw21_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun8iw21_ccu_resets),
};

static const u32 sun8iw21_pll_regs[] = {
	SUN8IW21_PLL_CPU_CTRL_REG,
	SUN8IW21_PLL_DDR_CTRL_REG,
	SUN8IW21_PLL_PERI_CTRL_REG,
	SUN8IW21_PLL_VIDEO_CTRL_REG,
	SUN8IW21_PLL_CSI_CTRL_REG,
	SUN8IW21_PLL_AUDIO_CTRL_REG,
	SUN8IW21_PLL_NPU_CTRL_REG,
};

static struct ccu_pll_nb sun8iw21_pll_cpu_nb = {
	.common = &pll_cpu_clk.common,
	/* copy from pll_cpu_clk */
	.enable = BIT(27),
	.lock   = BIT(28),
};

static struct ccu_mux_nb sun8iw21_cpu_nb = {
	.common         = &cpu_clk.common,
	.cm             = &cpu_clk.mux,
	.delay_us       = 1,
	.bypass_index   = 4, /* index of pll periph0 */
};

static int sun8iw21_ccu_probe(struct platform_device *pdev)
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

	for (i = 0; i < ARRAY_SIZE(sun8iw21_pll_regs); i++) {
		set_reg(reg + sun8iw21_pll_regs[i], 0x1, 1, 29);
	}

	ret = sunxi_ccu_probe(pdev->dev.of_node, reg, &sun8iw21_ccu_desc);
	if (ret)
		return ret;

	ccu_pll_notifier_register(&sun8iw21_pll_cpu_nb);

	ccu_mux_notifier_register(pll_cpu_clk.common.hw.clk, &sun8iw21_cpu_nb);

	sunxi_ccu_sleep_init(reg, sun8iw21_ccu_clks,
			ARRAY_SIZE(sun8iw21_ccu_clks),
			NULL, 0);

	return 0;
}

static const struct of_device_id sun8iw21_ccu_ids[] = {
	{ .compatible = "allwinner,sun8iw21-ccu" },
	{ }
};

static struct platform_driver sun8iw21_ccu_driver = {
	.probe	= sun8iw21_ccu_probe,
	.driver	= {
		.name	= "sun8iw21-ccu",
		.of_match_table	= sun8iw21_ccu_ids,
	},
};

static int __init sun8iw21_ccu_init(void)
{
	int err;

	err = platform_driver_register(&sun8iw21_ccu_driver);
	if (err)
		pr_err("register ccu sun8iw21 failed\n");

	return err;
}

core_initcall(sun8iw21_ccu_init);

static void __exit sun8iw21_ccu_exit(void)
{
	platform_driver_unregister(&sun8iw21_ccu_driver);
}
module_exit(sun8iw21_ccu_exit);

MODULE_DESCRIPTION("Allwinner sun8iw21 clk driver");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.1.5");
