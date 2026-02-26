// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2022 liujuan1@allwinnertech.com
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

#include "ccu-sun60iw1.h"

/* ccu_des_start */
#define SUN60IW1_PLL_REF_CTRL_REG   0x0000
static struct ccu_nkmp pll_ref_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(16, 7), /* output divider */
	.common		= {
		.reg		= 0x0000,
		.hw.init	= CLK_HW_INIT("pll-ref", "dcxo",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN60IW1_PLL_DDR_CTRL_REG   0x0020
static struct ccu_nkmp pll_ddr_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(8, 7), /* input divider */
	.p		= _SUNXI_CCU_DIV(1, 1), /* output divider */
	.common		= {
		.reg		= 0x0020,
		.hw.init	= CLK_HW_INIT("pll-ddr", "dcxo",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN60IW1_PLL_PERI0_CTRL_REG   0x00A0
static SUNXI_CCU_NM_WITH_GATE_LOCK(pll_peri0_clk, "pll-peri0",
		"dcxo", 0x00A0,
		8, 7,	/* N */
		1, 1,	/* M */
		BIT(27),	/* gate */
		BIT(28),	/* lock */
		CLK_SET_RATE_UNGATE | CLK_IS_CRITICAL);

static SUNXI_CCU_M(pll_peri0_2x_clk, "pll-peri0-2x",
		"pll-peri0", 0x00A0,
		20, 3,
		CLK_SET_RATE_PARENT);

static CLK_FIXED_FACTOR_HW(pll_peri0_div3_clk, "pll-peri0-div3",
		&pll_peri0_2x_clk.common.hw,
		6, 1, 0);

static SUNXI_CCU_M(pll_peri0_800m_clk, "pll-peri0-800m",
		"pll-peri0", 0x00A0,
		16, 3,
		0);

static SUNXI_CCU_M(pll_peri0_480m_clk, "pll-peri0-480m",
		"pll-peri0", 0x00A0,
		2, 3,
		0);

static CLK_FIXED_FACTOR_FW_NAME(pll_peri0_600m_clk, "pll-peri0-600m", "pll-peri0-2x", 2, 1, 0);
static CLK_FIXED_FACTOR_FW_NAME(pll_peri0_400m_clk, "pll-peri0-400m", "pll-peri0-2x", 3, 1, 0);
static CLK_FIXED_FACTOR_FW_NAME(pll_peri0_300m_clk, "pll-peri0-300m", "pll-peri0-600m", 2, 1, 0);
static CLK_FIXED_FACTOR_FW_NAME(pll_peri0_200m_clk, "pll-peri0-200m", "pll-peri0-400m", 2, 1, 0);
static CLK_FIXED_FACTOR_FW_NAME(pll_peri0_160m_clk, "pll-peri0-160m", "pll-peri0-480m", 3, 1, 0);
static CLK_FIXED_FACTOR_FW_NAME(pll_peri0_150m_clk, "pll-peri0-150m", "pll-peri0-300m", 2, 1, 0);

#define SUN60IW1_PLL_PERI1_CTRL_REG   0x00C0
static SUNXI_CCU_NM_WITH_GATE_LOCK(pll_peri1_clk, "pll-peri1",
		"dcxo", 0x00C0,
		8, 7,	/* N */
		1, 1,	/* M */
		BIT(27),	/* gate */
		BIT(28),	/* lock */
		CLK_SET_RATE_UNGATE | CLK_IS_CRITICAL);

static SUNXI_CCU_M(pll_peri1_2x_clk, "pll-peri1-2x",
		"pll-peri1", 0x00C0,
		20, 3,
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M(pll_peri1_800m_clk, "pll-peri1-800m",
		"pll-peri1", 0x00C0,
		16, 3,
		0);

static SUNXI_CCU_M(pll_peri1_480m_clk, "pll-peri1-480m",
		"pll-peri1", 0x00C0,
		2, 3,
		0);

static CLK_FIXED_FACTOR_FW_NAME(pll_peri1_600m_clk, "pll-peri1-600m", "pll-peri1-2x", 2, 1, 0);
static CLK_FIXED_FACTOR_FW_NAME(pll_peri1_400m_clk, "pll-peri1-400m", "pll-peri1-2x", 3, 1, 0);
static CLK_FIXED_FACTOR_FW_NAME(pll_peri1_300m_clk, "pll-peri1-300m", "pll-peri1-600m", 2, 1, 0);
static CLK_FIXED_FACTOR_FW_NAME(pll_peri1_200m_clk, "pll-peri1-200m", "pll-peri1-400m", 2, 1, 0);
static CLK_FIXED_FACTOR_FW_NAME(pll_peri1_160m_clk, "pll-peri1-160m", "pll-peri1-480m", 3, 1, 0);
static CLK_FIXED_FACTOR_FW_NAME(pll_peri1_150m_clk, "pll-peri1-150m", "pll-peri1-300m", 2, 1, 0);

#define SUN60IW1_PLL_GPU0_CTRL_REG   0x00E0
static struct ccu_nkmp pll_gpu0_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.common		= {
		.reg		= 0x00E0,
		.hw.init	= CLK_HW_INIT("pll-gpu0", "dcxo",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN60IW1_PLL_VIDEO0_CTRL_REG   0x0120
static struct ccu_nkmp pll_video0_4x_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.common		= {
		.reg		= 0x0120,
		.hw.init	= CLK_HW_INIT("pll-video0-4x", "dcxo",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static struct ccu_nkmp pll_video0_3x_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(16, 3), /* output divider */
	.common		= {
		.reg		= 0x0120,
		.hw.init	= CLK_HW_INIT("pll-video0-3x", "dcxo",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN60IW1_PLL_VIDEO1_CTRL_REG   0x0140
static struct ccu_nkmp pll_video1_4x_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.common		= {
		.reg		= 0x0120,
		.hw.init	= CLK_HW_INIT("pll-video1-4x", "dcxo",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static struct ccu_nkmp pll_video1_3x_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(16, 3), /* output divider */
	.common		= {
		.reg		= 0x0120,
		.hw.init	= CLK_HW_INIT("pll-video1-3x", "dcxo",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN60IW1_PLL_VIDEO2_CTRL_REG   0x0160
static struct ccu_nkmp pll_video2_4x_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.common		= {
		.reg		= 0x0160,
		.hw.init	= CLK_HW_INIT("pll-video2-4x", "dcxo",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static struct ccu_nkmp pll_video2_3x_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(16, 3), /* output divider */
	.common		= {
		.reg		= 0x0160,
		.hw.init	= CLK_HW_INIT("pll-video2-3x", "dcxo",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN60IW1_PLL_VIDEO3_CTRL_REG   0x0180
static struct ccu_nkmp pll_video3_4x_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.common		= {
		.reg		= 0x0180,
		.hw.init	= CLK_HW_INIT("pll-video3-4x", "dcxo",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static struct ccu_nkmp pll_video3_3x_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(16, 3), /* output divider */
	.common		= {
		.reg		= 0x0180,
		.hw.init	= CLK_HW_INIT("pll-video3-3x", "dcxo",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN60IW1_PLL_VE0_CTRL_REG   0x0220
static struct ccu_nkmp pll_ve0_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.common		= {
		.reg		= 0x0220,
		.hw.init	= CLK_HW_INIT("pll-ve0", "dcxo",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN60IW1_PLL_VE1_CTRL_REG   0x0240
static struct ccu_nkmp pll_ve1_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.common		= {
		.reg		= 0x0240,
		.hw.init	= CLK_HW_INIT("pll-ve1", "dcxo",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN60IW1_PLL_AUDIO0_CTRL_REG   0x0260
static struct ccu_nkmp pll_audio0_4x_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(16, 7), /* output divider */
	.common		= {
		.reg		= 0x0260,
		.hw.init	= CLK_HW_INIT("pll-audio0-4x", "dcxo",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN60IW1_PLL_NPU_CTRL_REG   0x02A0
static struct ccu_nkmp pll_npu_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.common		= {
		.reg		= 0x02A0,
		.hw.init	= CLK_HW_INIT("pll-npu", "dcxo",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN60IW1_PLL_DE_CTRL_REG   0x02E0
static struct ccu_nkmp pll_de_4x_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.common		= {
		.reg		= 0x02E0,
		.hw.init	= CLK_HW_INIT("pll-de-4x", "dcxo",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static struct ccu_nkmp pll_de_3x_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(16, 3), /* output divider */
	.common		= {
		.reg		= 0x02E0,
		.hw.init	= CLK_HW_INIT("pll-de-3x", "dcxo",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN60IW1_PLL_CCI_CTRL_REG   0x0320
static struct ccu_nkmp pll_cci_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.common		= {
		.reg		= 0x0320,
		.hw.init	= CLK_HW_INIT("pll-cci", "dcxo",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static const char * const ahb_apb_parents[] = { "sys24M", "ext-32k", "rc-16m", "pll-peri0-600m" };
static SUNXI_CCU_M_WITH_MUX(ahb_clk, "ahb",
		ahb_apb_parents, 0x0500,
		0, 5,
		24, 2,
		CLK_SET_RATE_UNGATE | CLK_IS_CRITICAL);

static SUNXI_CCU_M_WITH_MUX(apb0_clk, "apb0",
		ahb_apb_parents, 0x0510,
		0, 5,
		24, 2,
		CLK_SET_RATE_UNGATE | CLK_IS_CRITICAL);

static SUNXI_CCU_M_WITH_MUX(apb1_clk, "apb1",
		ahb_apb_parents, 0x0518,
		0, 5,
		24, 2,
		CLK_SET_RATE_UNGATE | CLK_IS_CRITICAL);

static const char * const apb_uart_parents[] = { "sys24M", "ext-32k", "rc-16m", "pll-peri0-600m", "pll-peri0-480m" };
static SUNXI_CCU_M_WITH_MUX(apb_uart_clk, "apb-uart",
		apb_uart_parents, 0x0538,
		0, 5,
		24, 3,
		CLK_SET_RATE_UNGATE | CLK_IS_CRITICAL);

static const char * const trace_parents[] = { "sys24M", "ext-32k", "rc-16m", "pll-peri0-300m", "pll-peri0-400m" };
static SUNXI_CCU_M_WITH_MUX_GATE(trace_clk, "trace",
		trace_parents, 0x0540,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const cci_parents[] = { "sys24M", "pll-cci", "pll-peri0-800m", "pll-peri0-600m", "depll3x", "pll-ddr" };
static SUNXI_CCU_M_WITH_MUX_GATE(cci_clk, "cci",
		cci_parents, 0x0548,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const gic_parents[] = { "sys24M", "pll-peri0-480m", "ext-32k", "pll-peri0-600m" };
static SUNXI_CCU_M_WITH_MUX_GATE(gic0_clk, "gic0",
		gic_parents, 0x0560,
		0, 5,	/* M */
		24, 2,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M_WITH_MUX_GATE(gic1_clk, "gic1",
		gic_parents, 0x0568,
		0, 5,	/* M */
		24, 2,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const nsi_parents[] = { "pll-ddr", "pll-peri0-800m", "pll-peri0-600m", "pll-cci", "pll-de-3x", "pll-npu" };
static SUNXI_CCU_M_WITH_MUX_GATE(nsi_clk, "nsi",
		nsi_parents, 0x0580,
		0, 1,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const mbus_parents[] = { "pll-peri0-600m", "pll-ddr", "pll-peri0-480m", "pll-peri0-400m", "pll-cci", "pll-npu" };

static SUNXI_CCU_M_WITH_MUX_GATE(mbus_clk, "mbus",
		mbus_parents, 0x0588,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(msi_lite0_clk, "msi-lite0",
		"dcxo",
		0x0594, BIT(0), 0);

static SUNXI_CCU_GATE(msi_lite1_clk, "msi-lite1",
		"dcxo",
		0x059c, BIT(0), 0);

static SUNXI_CCU_GATE(ve_enc1_mbus_clk, "ve-enc1-mbus",
		"dcxo",
		0x05e4, BIT(19), 0);

static SUNXI_CCU_GATE(ve_dec_mbus_clk, "ve-dec-mbus",
		"dcxo",
		0x05e4, BIT(18), 0);

static SUNXI_CCU_GATE(gmac1_mbus_mclk, "gamc1-mclk-mbus",
		"dcxo",
		0x05e4, BIT(12), 0);

static SUNXI_CCU_GATE(gmac0_mbus_mclk, "gamc0-mclk-mbus",
		"dcxo",
		0x05e4, BIT(11), 0);

static SUNXI_CCU_GATE(isp_mbus_clk, "isp-mbus",
		"dcxo",
		0x05e4, BIT(9), 0);

static SUNXI_CCU_GATE(csi_mbus_clk, "csi-mbus",
		"dcxo",
		0x05e4, BIT(8), 0);

static SUNXI_CCU_GATE(nand_mbus_clk, "nand-mbus",
		"dcxo",
		0x05e4, BIT(7), 0);

static SUNXI_CCU_GATE(dma1_mclk, "dma1-mclk",
		"dcxo",
		0x05e4, BIT(3), 0);

static SUNXI_CCU_GATE(ce_mclk, "ce-mclk",
		"dcxo",
		0x05e4, BIT(2), 0);

static SUNXI_CCU_GATE(ve_enc0_mclk, "ve-enc0-mclk",
		"dcxo",
		0x05e4, BIT(1), 0);

static SUNXI_CCU_GATE(dma0_mclk, "dma0-mclk",
		"dcxo",
		0x05e4, BIT(0), 0);

static SUNXI_CCU_GATE(dma0_clk, "dma0-clk",
		"dcxo",
		0x0704, BIT(0), 0);

static SUNXI_CCU_GATE(dma1_clk, "dma1-clk",
		"dcxo",
		0x070c, BIT(0), 0);

static SUNXI_CCU_GATE(spinlock_clk, "spinlock-clk",
		"dcxo",
		0x0724, BIT(0), 0);

static SUNXI_CCU_GATE(msgbox0_clk, "msgbox0",
		"dcxo",
		0x0744, BIT(0), 0);

static SUNXI_CCU_GATE(msgbox1_clk, "msgbox1",
		"dcxo",
		0x074c, BIT(0), 0);

static SUNXI_CCU_GATE(msgbox2_clk, "msgbox2",
		"dcxo",
		0x0754, BIT(0), 0);

static SUNXI_CCU_GATE(pwm0_clk, "pwm0",
		"dcxo",
		0x0784, BIT(0), 0);

static SUNXI_CCU_GATE(pwm1_clk, "pwm1",
		"dcxo",
		0x078c, BIT(0), 0);

static SUNXI_CCU_GATE(dbgsys_clk, "dbgsys",
		"dcxo",
		0x07a4, BIT(0), 0);

static SUNXI_CCU_GATE(dbgpad_clk, "dbgdbg",
		"dcxo",
		0x07ac, BIT(0), 0);

static const char * const timer_clk_parents[] = { "sys24M", "rc-16m", "ext-32k", "pll-peri0-200m", "dcxo" };
static SUNXI_CCU_M_WITH_MUX_GATE(timer0_clk0_clk, "timer0-clk0",
		timer_clk_parents, 0x0800,
		0, 3,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M_WITH_MUX_GATE(timer0_clk1_clk, "timer0-clk1",
		timer_clk_parents, 0x0804,
		0, 3,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M_WITH_MUX_GATE(timer0_clk2_clk, "timer0-clk2",
		timer_clk_parents, 0x0808,
		0, 3,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M_WITH_MUX_GATE(timer0_clk3_clk, "timer0-clk3",
		timer_clk_parents, 0x080c,
		0, 3,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M_WITH_MUX_GATE(timer0_clk4_clk, "timer0-clk4",
		timer_clk_parents, 0x0810,
		0, 3,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M_WITH_MUX_GATE(timer0_clk5_clk, "timer0-clk5",
		timer_clk_parents, 0x0814,
		0, 3,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M_WITH_MUX_GATE(timer0_clk6_clk, "timer0-clk6",
		timer_clk_parents, 0x0818,
		0, 3,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M_WITH_MUX_GATE(timer0_clk7_clk, "timer0-clk7",
		timer_clk_parents, 0x081c,
		0, 3,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M_WITH_MUX_GATE(timer0_clk8_clk, "timer0-clk8",
		timer_clk_parents, 0x0820,
		0, 3,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M_WITH_MUX_GATE(timer0_clk9_clk, "timer0-clk9",
		timer_clk_parents, 0x082c,
		0, 3,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(timer0_clk, "timer0",
		"dcxo",
		0x0850, BIT(0), 0);

static SUNXI_CCU_M_WITH_MUX_GATE(timer1_clk0_clk, "timer1-clk0",
		timer_clk_parents, 0x0880,
		0, 3,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M_WITH_MUX_GATE(timer1_clk1_clk, "timer1-clk1",
		timer_clk_parents, 0x0884,
		0, 3,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M_WITH_MUX_GATE(timer1_clk2_clk, "timer1-clk2",
		timer_clk_parents, 0x0888,
		0, 3,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M_WITH_MUX_GATE(timer1_clk3_clk, "timer1-clk3",
		timer_clk_parents, 0x088c,
		0, 3,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M_WITH_MUX_GATE(timer1_clk4_clk, "timer1-clk4",
		timer_clk_parents, 0x0890,
		0, 3,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M_WITH_MUX_GATE(timer1_clk5_clk, "timer1-clk5",
		timer_clk_parents, 0x0894,
		0, 3,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(timer1_clk, "timer1",
		"dcxo",
		0x08C0, BIT(0), 0);

static const char * const de0_parents[] = { "pll-de-3x", "pll-de-4x", "pll-peri0-480m", "pll-peri0-400m", "pll-peri0-300m", "pll-video0-4x", "pll-video3-4x" };
static SUNXI_CCU_M_WITH_MUX_GATE(de0_clk, "de0",
		de0_parents, 0x0A00,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_NO_REPARENT);

static SUNXI_CCU_GATE(de0_bus_clk, "de0-bus",
		"dcxo",
		0x0A04, BIT(0), 0);

static const char * const de1_parents[] = { "pll-de-3x", "pll-de-4x", "pll-peri0-480m", "pll-peri0-400m", "pll-peri0-300m", "pll-video0-4x", "pll-video3-4x" };
static SUNXI_CCU_M_WITH_MUX_GATE(de1_clk, "de1",
		de1_parents, 0x0A08,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_NO_REPARENT);

static SUNXI_CCU_GATE(de1_bus_clk, "de1-bus",
		"dcxo",
		0x0A0C, BIT(0), 0);

static const char * const di_parents[] = { "pll-peri0-480m", "pll-peri0-600m", "pll-peri0-400m", "pll-video0-4x", "pll-video3-4x", };
static SUNXI_CCU_M_WITH_MUX_GATE(di_clk, "di",
		di_parents, 0x0A20,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(di_bus_clk, "di-bus",
		"dcxo",
		0x0A24, BIT(0), 0);

static const char * const g2d_parents[] = { "pll-peri0-300m", "pll-peri0-400m", "pll-video0-4x", "pll-video3-4x" };
static SUNXI_CCU_M_WITH_MUX_GATE(g2d_clk, "g2d",
		g2d_parents, 0x0A40,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(g2d_bus_clk, "g2d-bus",
		"dcxo",
		0x0A44, BIT(0), 0);

static const char * const ve_enc_parents[] = { "pll-ve0", "pll-ve1", "pll-peri0-800m", "pll-peri0-600m", "pll-peri0-480m", "pll-de-3x", "pll-npu" };
static SUNXI_CCU_M_WITH_MUX_GATE(ve_enc0_clk, "ve-enc0",
		ve_enc_parents, 0x0A80,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M_WITH_MUX_GATE(ve_enc1_clk, "ve-enc1",
		ve_enc_parents, 0x0A84,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(ve_dec_clk, "ve-dec",
		"dcxo",
		0x0A8C, BIT(2), 0);

static SUNXI_CCU_GATE(ve_enc1_bus_clk, "ve-enc1-bus",
		"dcxo",
		0x0A8C, BIT(1), 0);

static SUNXI_CCU_GATE(ve_enc0_bus_clk, "ve-enc0-bus",
		"dcxo",
		0x0A8C, BIT(0), 0);

static const char * const ce_parents[] = { "sys24M", "pll-peri0-400m", "pll-peri0-600m" };
static SUNXI_CCU_M_WITH_MUX_GATE(ce_clk, "ce",
		ce_parents, 0x0AC0,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(ce_sys_clk, "ce-sys",
		"dcxo",
		0x0AC4, BIT(1), 0);

static SUNXI_CCU_GATE(ce_bus_clk, "ce-bus",
		"dcxo",
		0x0AC4, BIT(0), 0);

static const char * const npu_parents[] = { "pll-npu", "pll-peri0-800m", "pll-peri0-600m", "pll-peri0-480m", "pll-cci", "pll-ve0", "pll-ve1", "pll-de-3x" };
static SUNXI_CCU_M_WITH_MUX_GATE(npu_clk, "npu",
		npu_parents, 0x0B00,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(npu_bus_clk, "npu-bus",
		"dcxo",
		0x0B04, BIT(0), 0);

static const char * const aipu_parents[] = { "pll-peri0-600m", "pll-peri0-480m", "pll-peri0-800m", "pll-npu", "pll-ve0", "pll-ve1", "pll-de-3x" };
static SUNXI_CCU_M_WITH_MUX_GATE(aipu_clk, "aipu",
		aipu_parents, 0x0B08,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(aipu_bus_clk, "aipu-bus",
		"dcxo",
		0x0B0C, BIT(0), 0);

static const char * const gpu0_parents[] = { "pll-gpu0", "pll-peri0-800m", "pll-peri0-600m", "pll-peri0-400m", "pll-peri0-300m", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(gpu0_clk, "gpu0",
		gpu0_parents, 0x0B20,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(gpu0_bus_clk, "gpu0-bus",
		"dcxo",
		0x0B24, BIT(0), 0);

static const char * const dsp_parents[] = { "sys24M", "ext-32k", "rc-16m", "pll-peri0-2x", "pll-peri0-480m" };
static SUNXI_CCU_M_WITH_MUX_GATE(dsp_clk, "dsp",
		dsp_parents, 0x0B40,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const dram0_parents[] = { "pll-ddr", "pll-peri0-800m", "pll-peri0-600m", "pll_cci", "pll-de-3x", "pll-npu" };
static SUNXI_CCU_M_WITH_MUX_GATE(dram0_clk, "dram0",
		dram0_parents, 0x0C00,
		0, 5,	/* M */
		16, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(dram0_bus_clk, "dram0-bus",
		"dcxo",
		0x0C0C, BIT(0), 0);

static const char * const nand0_clk0_parents[] = { "sys24M", "pll-peri1-400m", "pll-peri1-300m", "pll-peri0-400m", "pll-peri0-300m" };
static SUNXI_CCU_M_WITH_MUX_GATE(nand0_clk0_clk, "nand0-clk0",
		nand0_clk0_parents, 0x0C80,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const nand0_clk1_parents[] = { "sys24M", "pll-peri1-400m", "pll-peri1-300m", "pll-peri0-400m", "pll-peri0-300m" };
static SUNXI_CCU_M_WITH_MUX_GATE(nand0_clk1_clk, "nand0-clk1",
		nand0_clk1_parents, 0x0C84,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(nand0_bus_clk, "nand0-bus",
		"dcxo",
		0x0C8C, BIT(0), 0);

static const char * const smhc0_parents[] = { "sys24M", "pll-peri1-400m", "pll-peri1-300m", "pll-peri0-400m", "pll-peri0-300m" };
static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc0_clk, "smhc0",
		smhc0_parents, 0x0D00,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const smhc0_24m_parents[] = { "sys24M", "dcxo" };
static SUNXI_CCU_MUX_WITH_GATE(smhc0_24m_clk, "smhc0-24m",
		smhc0_24m_parents, 0x0D04,
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(smhc0_bus_clk, "smhc0-bus",
		"dcxo",
		0x0D0C, BIT(0), 0);

static const char * const smhc1_parents[] = { "sys24M", "pll-peri1-400m", "pll-peri1-300m", "pll-peri0-400m", "pll-peri0-300m" };
static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc1_clk, "smhc1",
		smhc1_parents, 0x0D10,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(smhc1_bus_clk, "smhc1-bus",
		"dcxo",
		0x0D1C, BIT(0), 0);

static const char * const smhc2_parents[] = { "sys24M", "pll-peri1-800m", "pll-peri1-600m", "pll-peri0-800m", "pll-peri0-600m" };
static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc2_clk, "smhc2",
		smhc2_parents, 0x0D20,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const smhc2_24m_parents[] = { "sys24M", "dcxo" };
static SUNXI_CCU_MUX_WITH_GATE(smhc2_24m_clk, "smhc2-24m",
		smhc2_24m_parents, 0x0D24,
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(smhc2_bus_clk, "smhc2-bus",
		"dcxo",
		0x0D2C, BIT(0), 0);

static const char * const smhc3_parents[] = { "sys24M", "pll-peri1-800m", "pll-peri1-600m", "pll-peri0-800m", "pll-peri0-600m" };
static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc3_clk, "smhc3",
		smhc3_parents, 0x0D30,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const smhc3_24m_parents[] = { "sys24M", "dcxo" };
static SUNXI_CCU_MUX_WITH_GATE(smhc3_24m_clk, "smhc3-24m",
		smhc3_24m_parents, 0x0D34,
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(smhc3_bus_clk, "smhc3-bus",
		"dcxo",
		0x0D3C, BIT(0), 0);

static const char * const ufs_axi_parents[] = { "pll-peri0-300m", "pll-peri0-200m" };
static SUNXI_CCU_M_WITH_MUX_GATE(ufs_axi_clk, "ufs-axi",
		ufs_axi_parents, 0x0D80,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(ufs_clk, "ufs",
		"dcxo",
		0x0D8C, BIT(0), 0);

static SUNXI_CCU_GATE(uart0_clk, "uart0",
		"dcxo",
		0x0E00, BIT(0), 0);

static SUNXI_CCU_GATE(uart1_clk, "uart1",
		"dcxo",
		0x0E04, BIT(0), 0);

static SUNXI_CCU_GATE(uart2_clk, "uart2",
		"dcxo",
		0x0E08, BIT(0), 0);

static SUNXI_CCU_GATE(uart3_clk, "uart3",
		"dcxo",
		0x0E0C, BIT(0), 0);

static SUNXI_CCU_GATE(uart4_clk, "uart4",
		"dcxo",
		0x0E10, BIT(0), 0);

static SUNXI_CCU_GATE(uart5_clk, "uart5",
		"dcxo",
		0x0E14, BIT(0), 0);

static SUNXI_CCU_GATE(uart6_clk, "uart6",
		"dcxo",
		0x0E18, BIT(0), 0);

static SUNXI_CCU_GATE(uart7_clk, "uart7",
		"dcxo",
		0x0E1C, BIT(0), 0);

static SUNXI_CCU_GATE(uart8_clk, "uart8",
		"dcxo",
		0x0E20, BIT(0), 0);

static SUNXI_CCU_GATE(twi0_clk, "twi0",
		"dcxo",
		0x0E80, BIT(0), 0);

static SUNXI_CCU_GATE(twi1_clk, "twi1",
		"dcxo",
		0x0E84, BIT(0), 0);

static SUNXI_CCU_GATE(twi2_clk, "twi2",
		"dcxo",
		0x0E88, BIT(0), 0);

static SUNXI_CCU_GATE(twi3_clk, "twi3",
		"dcxo",
		0x0E8C, BIT(0), 0);

static SUNXI_CCU_GATE(twi4_clk, "twi4",
		"dcxo",
		0x0E90, BIT(0), 0);

static SUNXI_CCU_GATE(twi5_clk, "twi5",
		"dcxo",
		0x0E94, BIT(0), 0);

static SUNXI_CCU_GATE(twi6_clk, "twi6",
		"dcxo",
		0x0E98, BIT(0), 0);

static SUNXI_CCU_GATE(twi7_clk, "twi7",
		"dcxo",
		0x0E9C, BIT(0), 0);

static SUNXI_CCU_GATE(twi8_clk, "twi8",
		"dcxo",
		0x0EA0, BIT(0), 0);

static const char * const spi_parents[] = { "sys24M", "pll-peri0-300m", "pll-peri0-200m", "pll-peri1-300m", "pll-peri1-200m", "peri0-160m", "peri1-160m", "dcxo" };
static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spi0_clk, "spi0",
		spi_parents, 0x0F00,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(spi0_bus_clk, "spi0-bus",
		"dcxo",
		0x0F04, BIT(0), 0);

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spi1_clk, "spi1",
		spi_parents, 0x0F08,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(spi1_bus_clk, "spi1-bus",
		"dcxo",
		0x0F0C, BIT(0), 0);

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spi2_clk, "spi2",
		spi_parents, 0x0F10,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(spi2_bus_clk, "spi2-bus",
		"dcxo",
		0x0F14, BIT(0), 0);

static const char * const spif_parents[] = { "sys24M", "pll-peri0-400m", "pll-peri0-300m", "pll-peri1-400m", "pll-peri1-300m", "peri0-160m", "peri1-160m", "dcxo" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spif_clk, "spif",
		spif_parents, 0x0F18,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(spif_bus_clk, "spif-bus",
		"dcxo",
		0x0F1C, BIT(0), 0);

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spif3_clk, "spif3",
		spif_parents, 0x0F20,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(spif3_bus_clk, "spif3-bus",
		"dcxo",
		0x0F24, BIT(0), 0);

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spif4_clk, "spif4",
		spif_parents, 0x0F28,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(spif4_bus_clk, "spif4-bus",
		"dcxo",
		0x0F2c, BIT(0), 0);

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spif5_clk, "spif5",
		spif_parents, 0x0F30,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(spif5_bus_clk, "spif5-bus",
		"dcxo",
		0x0F34, BIT(0), 0);

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spif6_clk, "spif6",
		spif_parents, 0x0F38,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(spif6_bus_clk, "spif6-bus",
		"dcxo",
		0x0F3c, BIT(0), 0);

static const char * const gpadc0_24m_parents[] = { "sys24M", "dcxo" };
static SUNXI_CCU_M_WITH_MUX_GATE(gpadc0_24m_clk, "gpadc0-24m",
		gpadc0_24m_parents, 0x0FC0,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(gpadc0_clk, "gpadc0",
		"dcxo",
		0x0FC4, BIT(0), 0);

static const char * const gpadc1_24m_parents[] = { "sys24M", "dcxo" };
static SUNXI_CCU_M_WITH_MUX_GATE(gpadc1_24m_clk, "gpadc1-24m",
		gpadc1_24m_parents, 0x0FC8,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(gpadc1_clk, "gpadc1",
		"dcxo",
		0x0FCC, BIT(0), 0);

static SUNXI_CCU_GATE(ths0_clk, "ths0",
		"dcxo",
		0x0FE4, BIT(0), 0);

static const char * const irrx_parents[] = { "ext-32k", "sys24M", "dcxo" };
static SUNXI_CCU_M_WITH_MUX_GATE(irrx_clk, "irrx",
		irrx_parents, 0x1000,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(irrx_bus_clk, "irrx-bus",
		"dcxo",
		0x1004, BIT(0), 0);

static const char * const irtx_parents[] = { "sys24M", "pll-peri1-600m", "dcxo" };
static SUNXI_CCU_M_WITH_MUX_GATE(irtx_clk, "irtx",
		irtx_parents, 0x1008,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(irtx_bus_clk, "irtx-bus",
		"dcxo",
		0x100C, BIT(0), 0);

static SUNXI_CCU_GATE(lradc_clk, "lradc",
		"dcxo",
		0x1024, BIT(0), 0);

static const char * const lbc_parents[] = { "pll-video0-3x", "pll-video1-3x", "pll-video2-3x", "pll-video3-3x", "pll-peri0-200m", "pll-peri0-300m" };
static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(lbc_clk, "lbc",
		lbc_parents, 0x1040,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(lbc_bus_clk, "lbc-bus",
		"dcxo",
		0x1044, BIT(0), 0);

static const char * const i2spcm1_parents[] = { "pll-audio0-4x", "pll-audio1-4x", "pll-peri0-200m" };
static SUNXI_CCU_M_WITH_MUX_GATE(i2spcm1_clk, "i2spcm1",
		i2spcm1_parents, 0x1210,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(i2spcm1_bus_clk, "i2spcm1-bus",
		"dcxo",
		0x121C, BIT(0), 0);

static const char * const i2spcm2_parents[] = { "pll-audio0-4x", "pll-audio1-4x", "pll-peri0-200m" };
static SUNXI_CCU_M_WITH_MUX_GATE(i2spcm2_clk, "i2spcm2",
		i2spcm2_parents, 0x1220,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const i2spcm2_asrc_parents[] = { "pll-audio0-4x", "pll-audio1-4x", "pll-peri0-300m", "pll-peri1-300m" };
static SUNXI_CCU_M_WITH_MUX_GATE(i2spcm2_asrc_clk, "i2spcm2-asrc",
		i2spcm2_asrc_parents, 0x1224,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(i2spcm2_bus_clk, "i2spcm2-bus",
		"dcxo",
		0x122C, BIT(0), 0);

static const char * const i2spcm3_parents[] = { "pll-audio0-4x", "pll-audio1-4x", "pll-peri0-200m" };
static SUNXI_CCU_M_WITH_MUX_GATE(i2spcm3_clk, "i2spcm3",
		i2spcm3_parents, 0x1230,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const i2spcm4_parents[] = { "pll-audio0-4x", "pll-audio1-4x", "pll-peri0-200m" };
static SUNXI_CCU_M_WITH_MUX_GATE(i2spcm4_clk, "i2spcm4",
		i2spcm4_parents, 0x1240,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(i2spcm4_bus_clk, "i2spcm4-bus",
		"dcxo",
		0x124C, BIT(0), 0);

static const char * const i2spcm5_parents[] = { "pll-audio0-4x", "pll-audio1-4x", "pll-peri0-200m" };
static SUNXI_CCU_M_WITH_MUX_GATE(i2spcm5_clk, "i2spcm5",
		i2spcm5_parents, 0x1250,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(i2spcm5_bus_clk, "i2spcm5-bus",
		"dcxo",
		0x125C, BIT(0), 0);

static const char * const spdif_tx_parents[] = { "pll-audio0-4x", "pll-audio1-4x" };
static SUNXI_CCU_M_WITH_MUX_GATE(spdif_tx_clk, "spdif-tx",
		spdif_tx_parents, 0x1280,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const spdif_rx_parents[] = { "pll-peri0-200m", "pll-peri0-300m", "pll-peri0-400m" };
static SUNXI_CCU_M_WITH_MUX_GATE(spdif_rx_clk, "spdif-rx",
		spdif_rx_parents, 0x1284,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(spdif_clk, "spdif",
		"dcxo",
		0x128C, BIT(0), 0);

static SUNXI_CCU_GATE(usb2_host0_clk, "usb2-host0",
		"dcxo",
		0x1300, BIT(31), 0);

static SUNXI_CCU_GATE(usb2_otg0_clk, "usb2-otg0",
		"dcxo",
		0x1304, BIT(8), 0);

static SUNXI_CCU_GATE(usb2_ehci0_clk, "usb2-ehci0",
		"dcxo",
		0x1304, BIT(4), 0);

static SUNXI_CCU_GATE(usb2_ohci0_clk, "usb2-ohci0",
		"dcxo",
		0x1304, BIT(0), 0);

static SUNXI_CCU_GATE(usb2_host1_clk, "usb2-host1",
		"dcxo",
		0x1308, BIT(31), 0);

static SUNXI_CCU_GATE(usb2_ohci1_clk, "usb2-ohci1",
		"dcxo",
		0x130C, BIT(0), 0);

static SUNXI_CCU_GATE(usb2_host2_clk, "usb2-host2",
		"dcxo",
		0x1310, BIT(31), 0);

static SUNXI_CCU_GATE(usb2_ehci2_clk, "usb2-ehci2",
		"dcxo",
		0x1314, BIT(4), 0);

static SUNXI_CCU_GATE(usb2_ohci2_clk, "usb2-ohci2",
		"dcxo",
		0x1314, BIT(0), 0);

static const char * const usb2_ref_parents[] = { "sys24M", "dcxo" };
static SUNXI_CCU_MUX_WITH_GATE(usb2_ref_clk, "usb2-ref",
		usb2_ref_parents, 0x1340,
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const usb3_usb2_ref_parents[] = { "sys24M", "pll-peri0-300m", "dcxo" };
static SUNXI_CCU_M_WITH_MUX_GATE(usb3_usb2_ref_clk, "usb3-usb2-ref",
		usb3_usb2_ref_parents, 0x1348,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const usb3_suspend_parents[] = { "ext-32k", "sys24M" };
static SUNXI_CCU_M_WITH_MUX_GATE(usb3_suspend_clk, "usb3-suspend",
		usb3_suspend_parents, 0x1350,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const usb3_mf_parents[] = { "sys24M", "pll-peri0-300m", "dcxo" };
static SUNXI_CCU_M_WITH_MUX_GATE(usb3_mf_clk, "usb3-mf",
		usb3_mf_parents, 0x1354,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const pcie0_aux_parents[] = { "sys24M", "ext-32k" };
static SUNXI_CCU_M_WITH_MUX_GATE(pcie0_aux_clk, "pcie0-aux",
		pcie0_aux_parents, 0x1380,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const gmac_ptp_parents[] = { "sys24M", "pll-peri0-200m", "dcxo" };
static SUNXI_CCU_M_WITH_MUX_GATE(gmac_ptp_clk, "gmac-ptp",
		gmac_ptp_parents, 0x1400,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(gmac0_phy_clk, "gmac0-phy",
		"dcxo",
		0x1410, BIT(31), 0);

static SUNXI_CCU_GATE(gmac0_clk, "gmac0",
		"dcxo",
		0x141C, BIT(0), 0);

static SUNXI_CCU_GATE(gmac1_phy_clk, "gmac1-phy",
		"dcxo",
		0x1420, BIT(31), 0);

static SUNXI_CCU_GATE(gmac1_clk, "gmac1",
		"dcxo",
		0x142C, BIT(0), 0);

static const char * const vo0_tconlcd0_parents[] = { "pll-video0-4x", "pll-video3-4x", "pll-peri0-2x", "pll-video01-4x", "pll-video02-4x" };
static SUNXI_CCU_M_WITH_MUX_GATE(vo0_tconlcd0_clk, "vo0-tconlcd0",
		vo0_tconlcd0_parents, 0x1500,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_NO_REPARENT);

static SUNXI_CCU_GATE(vo0_tconlcd0_bus_clk, "vo0-tconlcd0-bus",
		"dcxo",
		0x1504, BIT(0), 0);

static const char * const vo0_tconlcd1_parents[] = { "pll-video0-4x", "pll-video3-4x", "pll-peri0-2x", "pll-video01-4x", "pll-video02-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(vo0_tconlcd1_clk, "vo0-tconlcd1",
		vo0_tconlcd1_parents, 0x1508,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_NO_REPARENT);

static SUNXI_CCU_GATE(vo0_tconlcd1_bus_clk, "vo0-tconlcd1-bus",
		"dcxo",
		0x150C, BIT(0), 0);

static const char * const vo0_tconlcd2_parents[] = { "pll-video0-4x", "pll-video3-4x", "pll-peri0-2x", "pll-video01-4x", "pll-video02-4x" };
static SUNXI_CCU_M_WITH_MUX_GATE(vo0_tconlcd2_clk, "vo0-tconlcd2",
		vo0_tconlcd2_parents, 0x1510,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(vo0_tconlcd2_bus_clk, "vo0-tconlcd2-bus",
		"dcxo",
		0x1514, BIT(0), 0);

static const char * const vo0_tconlcd3_parents[] = { "pll-video0-4x", "pll-video3-4x", "pll-peri0-2x", "pll-video01-4x", "pll-video02-4x" };
static SUNXI_CCU_M_WITH_MUX_GATE(vo0_tconlcd3_clk, "vo0-tconlcd3",
		vo0_tconlcd3_parents, 0x1518,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(vo0_tconlcd3_bus_clk, "vo0-tconlcd3-bus",
		"dcxo",
		0x151C, BIT(0), 0);

static const char * const dsi0_parents[] = { "sys24M", "pll-peri0-200m", "pll-peri0-150m" };
static SUNXI_CCU_M_WITH_MUX_GATE(dsi0_clk, "dsi0",
		dsi0_parents, 0x1580,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(dsi0_bus_clk, "dsi0-bus",
		"dcxo",
		0x1584, BIT(0), 0);

static const char * const dsi1_parents[] = { "sys24M", "pll-peri0-200m", "pll-peri0-150m" };
static SUNXI_CCU_M_WITH_MUX_GATE(dsi1_clk, "dsi1",
		dsi1_parents, 0x1588,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(dsi1_bus_clk, "dsi1-bus",
		"dcxo",
		0x158C, BIT(0), 0);

static const char * const combphy0_parents[] = { "pll-video0-4x", "pll-video3-4x", "pll-peri0-2x", "pll-video01-4x", "pll-video02-4x" };
static SUNXI_CCU_M_WITH_MUX_GATE(combphy0_clk, "combphy0",
		combphy0_parents, 0x15C0,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const combphy1_parents[] = { "pll-video0-4x", "pll-video3-4x", "pll-peri0-2x", "pll-video01-4x", "pll-video02-4x" };
static SUNXI_CCU_M_WITH_MUX_GATE(combphy1_clk, "combphy1",
		combphy1_parents, 0x15C4,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const tcontv0_parents[] = { "pll-video0-4x", "pll-video3-4x", "pll-peri0-2x", "pll-video01-4x", "pll-video02-4x" };
static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(tcontv0_clk, "tcontv0",
		tcontv0_parents, 0x1600,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(tcontv0_bus_clk, "tcontv0-bus",
		"dcxo",
		0x1604, BIT(0), 0);

static SUNXI_CCU_GATE(tcontv1_clk, "tcontv1",
		"dcxo",
		0x160C, BIT(0), 0);

static SUNXI_CCU_GATE(tcontv2_clk, "tcontv2",
		"dcxo",
		0x1614, BIT(0), 0);

static const char * const edp_clk0_parents[] = { "pll-video0-4x", "pll-video3-4x", "pll-peri0-2x", "pll-video01-4x", "pll-video02-4x" };
static SUNXI_CCU_M_WITH_MUX_GATE(edp_clk0_clk, "edp-clk0",
		edp_clk0_parents, 0x1640,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const edp_clk1_parents[] = { "pll-video0-4x", "pll-video3-4x", "pll-peri0-2x", "pll-video01-4x", "pll-video02-4x" };
static SUNXI_CCU_M_WITH_MUX_GATE(edp_clk1_clk, "edp-clk1",
		edp_clk1_parents, 0x1644,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(edp_bus_clk, "edp-bus",
		"dcxo",
		0x164C, BIT(0), 0);

static SUNXI_CCU_GATE(hdmi_ref_clk, "hdmi-ref",
		"dcxo",
		0x1680, BIT(31), 0);

static const char * const hdmi_pre_parents[] = { "pll-video0-4x", "pll-video3-4x", "pll-peri0-2x", "pll-video01-4x", "pll-video02-4x" };
static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(hdmi_pre_clk, "hdmi-pre",
		hdmi_pre_parents, 0x1684,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(hdmi_hdcp_clk, "hdmi-hdcp",
		"dcxo",
		0x168C, BIT(1), 0);

static SUNXI_CCU_GATE(hdmi_clk, "hdmi",
		"dcxo",
		0x168C, BIT(0), 0);

static SUNXI_CCU_GATE(dpss_top0_clk, "dpss-top0",
		"dcxo",
		0x16C4, BIT(0), 0);

static SUNXI_CCU_GATE(dpss_top1_clk, "dpss-top1",
		"dcxo",
		0x16CC, BIT(0), 0);

static const char * const ledc_parents[] = { "sys24M", "pll-peri0-600m", "dcxo" };
static SUNXI_CCU_M_WITH_MUX_GATE(ledc_clk, "ledc",
		ledc_parents, 0x1700,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(ledc_bus_clk, "ledc-bus",
		"dcxo",
		0x1704, BIT(0), 0);

static const char * const csi_master_parents[] = { "sys24M", "pll-video0-4x", "pll-video0-3x", "pll-video1-4x", "pll-video1-3x", "pll-video2-4x", "pll-video2-3x", "pll-video3-4x" };
static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(csi_master0_clk, "csi-master0",
		csi_master_parents, 0x1800,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(csi_master1_clk, "csi-master1",
		csi_master_parents, 0x1804,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);


static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(csi_master2_clk, "csi-master2",
		csi_master_parents, 0x1808,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);


static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(csi_master3_clk, "csi-master3",
		csi_master_parents, 0x180C,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const csi_parents[] = { "pll-video3-4x", "pll-de-4x", "pll-peri0-480m", "pll-peri0-400m", "pll-peri0-600m", "pll-video0-4x", "pll-video1-4x", "pll-video2-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(csi_clk, "csi",
		csi_parents, 0x1840,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(csi_bus_clk, "csi-bus",
		"dcxo",
		0x1844, BIT(0), 0);

static const char * const isp_parents[] = { "pll-video3-4x", "pll-peri0-480m", "pll-peri0-400m", "pll-peri0-600m", "pll-video0-4x", "pll-video1-4x", "pll-video2-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(isp_clk, "isp",
		isp_parents, 0x1860,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

/* rst_def_start */
static struct ccu_reset_map sun60iw1_ccu_resets[] = {
	[RST_BUS_NSI]			= { 0x0580, BIT(30) },
	[RST_BUS_NSI_CFG]		= { 0x0584, BIT(16) },
	[RST_MBUS]			= { 0x0588, BIT(30) },
	[RST_BUS_SMMU_TCU]		= { 0x058c, BIT(31) },
	[RST_BUS_SMMU_GPU1]		= { 0x058c, BIT(30) },
	[RST_BUS_SMMU_AIPU]		= { 0x058c, BIT(29) },
	[RST_BUS_SMMU_NPU]		= { 0x058c, BIT(28) },
	[RST_BUS_SMMU_GPU0]		= { 0x058c, BIT(27) },
	[RST_BUS_SMMU_PCIE1]		= { 0x058c, BIT(25) },
	[RST_BUS_SMMU_PCIE0]		= { 0x058c, BIT(24) },
	[RST_BUS_SMMU_MSI_LITE1]	= { 0x058c, BIT(23) },
	[RST_BUS_SMMU_MSI_LITE0]	= { 0x058c, BIT(22) },
	[RST_BUS_SMMU_USB3]		= { 0x058c, BIT(21) },
	[RST_BUS_SMMU_GMAC1]		= { 0x058c, BIT(20) },
	[RST_BUS_SMMU_GMAC0]		= { 0x058c, BIT(19) },
	[RST_BUS_SMMU_DE1]		= { 0x058c, BIT(18) },
	[RST_BUS_SMMU_DE0]		= { 0x058c, BIT(17) },
	[RST_BUS_SMMU_VE_DEC1]		= { 0x058c, BIT(16) },
	[RST_BUS_SMMU_VE_DEC0]		= { 0x058c, BIT(15) },
	[RST_BUS_SMMU_VE_ENC1]		= { 0x058c, BIT(14) },
	[RST_BUS_SMMU_VE_ENC0]		= { 0x058c, BIT(13) },
	[RST_BUS_SMMU_G2D]		= { 0x058c, BIT(12) },
	[RST_BUS_SMMU_DI]		= { 0x058c, BIT(11) },
	[RST_BUS_SMMU_CSI_DMA1]		= { 0x058c, BIT(10) },
	[RST_BUS_SMMU_CSI_DMA0]		= { 0x058c, BIT(9) },
	[RST_BUS_SMMU_ISP]		= { 0x058c, BIT(8) },
	[RST_BUS_SMMU_SY]		= { 0x058c, BIT(4) },
	[RST_BUS_MSI_LITE0]		= { 0x0594, BIT(16) },
	[RST_BUS_MSI_LITE1]		= { 0x059c, BIT(16) },
	[RST_BUS_DMA0]			= { 0x0704, BIT(16) },
	[RST_BUS_DMA1]			= { 0x070c, BIT(16) },
	[RST_BUS_SPINLOCK]		= { 0x0724, BIT(16) },
	[RST_BUS_MSGBOX0]		= { 0x0744, BIT(16) },
	[RST_BUS_MSGBOX1]		= { 0x074c, BIT(16) },
	[RST_BUS_MSGBOX2]		= { 0x0754, BIT(16) },
	[RST_BUS_PWM0]			= { 0x0784, BIT(16) },
	[RST_BUS_PWM1]			= { 0x078c, BIT(16) },
	[RST_BUS_DBGSY]			= { 0x07a4, BIT(16) },
	[RST_BUS_SYSDAP]		= { 0x07ac, BIT(16) },
	[RST_BUS_TIMER0]		= { 0x0850, BIT(16) },
	[RST_BUS_TIMER1]		= { 0x08c0, BIT(16) },
	[RST_BUS_DE0]			= { 0x0a04, BIT(16) },
	[RST_BUS_DE1]			= { 0x0a0c, BIT(16) },
	[RST_BUS_DI]			= { 0x0a24, BIT(16) },
	[RST_BUS_G2D]			= { 0x0a44, BIT(16) },
	[RST_BUS_DE_SY]			= { 0x0a74, BIT(16) },
	[RST_BUS_VE_DEC]		= { 0x0a8c, BIT(18) },
	[RST_BUS_VE_ENC1]		= { 0x0a8c, BIT(17) },
	[RST_BUS_VE_ENC0]		= { 0x0a8c, BIT(16) },
	[RST_BUS_CE_SY]			= { 0x0ac4, BIT(17) },
	[RST_BUS_CE]			= { 0x0ac4, BIT(16) },
	[RST_BUS_NPU_SY]		= { 0x0b04, BIT(19) },
	[RST_BUS_NPU_AHB]		= { 0x0b04, BIT(18) },
	[RST_BUS_NPU_AXI]		= { 0x0b04, BIT(17) },
	[RST_BUS_NPU_CORE]		= { 0x0b04, BIT(16) },
	[RST_BUS_AIPU_CORE]		= { 0x0b0c, BIT(17) },
	[RST_BUS_AIPU]			= { 0x0b0c, BIT(16) },
	[RST_BUS_GPU0]			= { 0x0b24, BIT(16) },
	[RST_BUS_DRAM0]			= { 0x0c0c, BIT(16) },
	[RST_BUS_NAND0]			= { 0x0c8c, BIT(16) },
	[RST_BUS_SMHC0]			= { 0x0d0c, BIT(16) },
	[RST_BUS_SMHC1]			= { 0x0d1c, BIT(16) },
	[RST_BUS_SMHC2]			= { 0x0d2c, BIT(16) },
	[RST_BUS_SMHC3]			= { 0x0d3c, BIT(16) },
	[RST_BUS_UFS_AXI]		= { 0x0d8c, BIT(17) },
	[RST_BUS_UF]			= { 0x0d8c, BIT(16) },
	[RST_BUS_UART0]			= { 0x0e00, BIT(16) },
	[RST_BUS_UART1]			= { 0x0e04, BIT(16) },
	[RST_BUS_UART2]			= { 0x0e08, BIT(16) },
	[RST_BUS_UART3]			= { 0x0e0c, BIT(16) },
	[RST_BUS_UART4]			= { 0x0e10, BIT(16) },
	[RST_BUS_UART5]			= { 0x0e14, BIT(16) },
	[RST_BUS_UART6]			= { 0x0e18, BIT(16) },
	[RST_BUS_UART7]			= { 0x0e1c, BIT(16) },
	[RST_BUS_UART8]			= { 0x0e20, BIT(16) },
	[RST_BUS_TWI0]			= { 0x0e80, BIT(16) },
	[RST_BUS_TWI1]			= { 0x0e84, BIT(16) },
	[RST_BUS_TWI2]			= { 0x0e88, BIT(16) },
	[RST_BUS_TWI3]			= { 0x0e8c, BIT(16) },
	[RST_BUS_TWI4]			= { 0x0e90, BIT(16) },
	[RST_BUS_TWI5]			= { 0x0e94, BIT(16) },
	[RST_BUS_TWI6]			= { 0x0e98, BIT(16) },
	[RST_BUS_TWI7]			= { 0x0e9c, BIT(16) },
	[RST_BUS_TWI8]			= { 0x0ea0, BIT(16) },
	[RST_BUS_SPI0]			= { 0x0f04, BIT(16) },
	[RST_BUS_SPI1]			= { 0x0f0c, BIT(16) },
	[RST_BUS_SPI2]			= { 0x0f14, BIT(16) },
	[RST_BUS_SPIF]			= { 0x0f1c, BIT(16) },
	[RST_BUS_GPADC0]		= { 0x0fc4, BIT(16) },
	[RST_BUS_GPADC1]		= { 0x0fcc, BIT(16) },
	[RST_BUS_THS0]			= { 0x0fe4, BIT(16) },
	[RST_BUS_IRRX]			= { 0x1004, BIT(16) },
	[RST_BUS_IRTX]			= { 0x100c, BIT(16) },
	[RST_BUS_LRADC]			= { 0x1024, BIT(16) },
	[RST_BUS_LBC]			= { 0x1044, BIT(16) },
	[RST_BUS_I2SPCM1]		= { 0x121c, BIT(16) },
	[RST_BUS_I2SPCM2]		= { 0x122c, BIT(16) },
	[RST_BUS_I2SPCM3]		= { 0x123c, BIT(16) },
	[RST_BUS_I2SPCM4]		= { 0x124c, BIT(16) },
	[RST_BUS_I2SPCM5]		= { 0x125c, BIT(16) },
	[RST_BUS_SPDIF]			= { 0x128c, BIT(16) },
	[RST_USB2_PHY0_RSTN]		= { 0x1300, BIT(30) },
	[RST_USB2_OTG0]			= { 0x1304, BIT(24) },
	[RST_USB2_EHCI0]		= { 0x1304, BIT(20) },
	[RST_USB2_OHCI0]		= { 0x1304, BIT(16) },
	[RST_USB2_PHY1_RSTN]		= { 0x1308, BIT(30) },
	[RST_USB2_EHCI1]		= { 0x130c, BIT(20) },
	[RST_USB2_OHCI1]		= { 0x130c, BIT(16) },
	[RST_USB2_PHY2_RSTN]		= { 0x1310, BIT(30) },
	[RST_USB2_EHCI2]		= { 0x1314, BIT(20) },
	[RST_USB2_OHCI2]		= { 0x1314, BIT(16) },
	[RST_USB3]			= { 0x1354, BIT(16) },
	[RST_BUS_PCIE0]			= { 0x138c, BIT(17) },
	[RST_BUS_PCIE0_PWRUP]		= { 0x138c, BIT(16) },
	[RST_BUS_PCIE1]			= { 0x139c, BIT(17) },
	[RST_BUS_PCIE1_PWRUP]		= { 0x139c, BIT(16) },
	[RST_BUS_SERDE]			= { 0x13c4, BIT(16) },
	[RST_BUS_GMAC0_AXI]		= { 0x141c, BIT(17) },
	[RST_BUS_GMAC0]			= { 0x141c, BIT(16) },
	[RST_BUS_GMAC1_AXI]		= { 0x142c, BIT(17) },
	[RST_BUS_GMAC1]			= { 0x142c, BIT(16) },
	[RST_BUS_VO0_TCONLCD0]		= { 0x1504, BIT(16) },
	[RST_BUS_VO0_TCONLCD1]		= { 0x150c, BIT(16) },
	[RST_BUS_VO0_TCONLCD2]		= { 0x1514, BIT(16) },
	[RST_BUS_VO0_TCONLCD3]		= { 0x151c, BIT(16) },
	[RST_BUS_LVDS0]			= { 0x1544, BIT(16) },
	[RST_BUS_LVDS1]			= { 0x154c, BIT(16) },
	[RST_BUS_LVDS2]			= { 0x1554, BIT(16) },
	[RST_BUS_DSI0]			= { 0x1584, BIT(16) },
	[RST_BUS_DSI1]			= { 0x158c, BIT(16) },
	[RST_BUS_TCONTV0]		= { 0x1604, BIT(16) },
	[RST_BUS_TCONTV1]		= { 0x160c, BIT(16) },
	[RST_BUS_TCONTV2]		= { 0x1614, BIT(16) },
	[RST_BUS_EDP]			= { 0x164c, BIT(16) },
	[RST_BUS_HDMI_HDCP]		= { 0x168c, BIT(18) },
	[RST_BUS_HDMI_SUB]		= { 0x168c, BIT(17) },
	[RST_BUS_HDMI_MAIN]		= { 0x168c, BIT(16) },
	[RST_BUS_DPSS_TOP0]		= { 0x16c4, BIT(16) },
	[RST_BUS_DPSS_TOP1]		= { 0x16cc, BIT(16) },
	[RST_BUS_VIDEO_OUT0]		= { 0x16e4, BIT(16) },
	[RST_BUS_VIDEO_OUT1]		= { 0x16ec, BIT(16) },
	[RST_BUS_LEDC]			= { 0x1704, BIT(16) },
	[RST_BUS_CSI]			= { 0x1844, BIT(16) },
	[RST_BUS_VIDEO_IN]		= { 0x1884, BIT(16) },
};
/* rst_def_end */

/* ccu_def_start */
static struct clk_hw_onecell_data sun60iw1_hw_clks = {
	.hws    = {
		[CLK_PLL_REF]			= &pll_ref_clk.common.hw,
		[CLK_PLL_DDR]			= &pll_ddr_clk.common.hw,
		[CLK_PLL_PERI0]			= &pll_peri0_clk.common.hw,
		[CLK_PLL_PERI0_2X]		= &pll_peri0_2x_clk.common.hw,
		[CLK_PERI0_DIV3]		= &pll_peri0_div3_clk.hw,
		[CLK_PLL_PERI0_800M]		= &pll_peri0_800m_clk.common.hw,
		[CLK_PLL_PERI0_480M]		= &pll_peri0_480m_clk.common.hw,
		[CLK_PLL_PERI0_600M]		= &pll_peri0_600m_clk.hw,
		[CLK_PLL_PERI0_400M]		= &pll_peri0_400m_clk.hw,
		[CLK_PLL_PERI0_300M]		= &pll_peri0_300m_clk.hw,
		[CLK_PLL_PERI0_200M]		= &pll_peri0_200m_clk.hw,
		[CLK_PLL_PERI0_160M]		= &pll_peri0_160m_clk.hw,
		[CLK_PLL_PERI0_150M]		= &pll_peri0_150m_clk.hw,
		[CLK_PLL_PERI1]			= &pll_peri1_clk.common.hw,
		[CLK_PLL_PERI1_2X]		= &pll_peri1_2x_clk.common.hw,
		[CLK_PLL_PERI1_800M]		= &pll_peri1_800m_clk.common.hw,
		[CLK_PLL_PERI1_480M]		= &pll_peri1_480m_clk.common.hw,
		[CLK_PLL_PERI1_600M]		= &pll_peri1_600m_clk.hw,
		[CLK_PLL_PERI1_400M]		= &pll_peri1_400m_clk.hw,
		[CLK_PLL_PERI1_300M]		= &pll_peri1_300m_clk.hw,
		[CLK_PLL_PERI1_200M]		= &pll_peri1_200m_clk.hw,
		[CLK_PLL_PERI1_160M]		= &pll_peri1_160m_clk.hw,
		[CLK_PLL_PERI1_150M]		= &pll_peri1_150m_clk.hw,
		[CLK_PLL_GPU0]			= &pll_gpu0_clk.common.hw,
		[CLK_PLL_VIDEO0_4X]		= &pll_video0_4x_clk.common.hw,
		[CLK_PLL_VIDEO0_3X]		= &pll_video0_3x_clk.common.hw,
		[CLK_PLL_VIDEO1_4X]		= &pll_video1_4x_clk.common.hw,
		[CLK_PLL_VIDEO1_3X]		= &pll_video1_3x_clk.common.hw,
		[CLK_PLL_VIDEO2_4X]		= &pll_video2_4x_clk.common.hw,
		[CLK_PLL_VIDEO2_3X]		= &pll_video2_3x_clk.common.hw,
		[CLK_PLL_VIDEO3_4X]		= &pll_video3_4x_clk.common.hw,
		[CLK_PLL_VIDEO3_3X]		= &pll_video3_3x_clk.common.hw,
		[CLK_PLL_VE0]			= &pll_ve0_clk.common.hw,
		[CLK_PLL_VE1]			= &pll_ve1_clk.common.hw,
		[CLK_PLL_AUDIO0_4X]		= &pll_audio0_4x_clk.common.hw,
		[CLK_PLL_NPU]			= &pll_npu_clk.common.hw,
		[CLK_PLL_DE_4X]			= &pll_de_4x_clk.common.hw,
		[CLK_PLL_DE_3X]			= &pll_de_3x_clk.common.hw,
		[CLK_PLL_CCI]			= &pll_cci_clk.common.hw,
		[CLK_AHB]			= &ahb_clk.common.hw,
		[CLK_APB0]			= &apb0_clk.common.hw,
		[CLK_APB1]			= &apb1_clk.common.hw,
		[CLK_APB_UART]			= &apb_uart_clk.common.hw,
		[CLK_TRACE]			= &trace_clk.common.hw,
		[CLK_CCI]			= &cci_clk.common.hw,
		[CLK_GIC0]			= &gic0_clk.common.hw,
		[CLK_GIC1]			= &gic1_clk.common.hw,
		[CLK_NSI]			= &nsi_clk.common.hw,
		[CLK_MBUS]			= &mbus_clk.common.hw,
		[CLK_MSI_LITE0]			= &msi_lite0_clk.common.hw,
		[CLK_MSI_LITE1]			= &msi_lite1_clk.common.hw,
		[CLK_VE_ENC1_MBUS]		= &ve_enc1_mbus_clk.common.hw,
		[CLK_VE_DEC_MBUS]		= &ve_dec_mbus_clk.common.hw,
		[CLK_GMAC1_MBUS]		= &gmac1_mbus_mclk.common.hw,
		[CLK_GMAC0_MBUS]		= &gmac0_mbus_mclk.common.hw,
		[CLK_ISP_MBUS]			= &isp_mbus_clk.common.hw,
		[CLK_CSI_MBUS]			= &csi_mbus_clk.common.hw,
		[CLK_NAND_MBUS]			= &nand_mbus_clk.common.hw,
		[CLK_DMA1_MBUS]			= &dma1_mclk.common.hw,
		[CLK_CE_MBUS]			= &ce_mclk.common.hw,
		[CLK_VE_ENC0_MBUS]		= &ve_enc0_mclk.common.hw,
		[CLK_DMA0_MBUS]			= &dma0_mclk.common.hw,
		[CLK_DMA0_BUS]			= &dma0_clk.common.hw,
		[CLK_DMA1_BUS]			= &dma1_clk.common.hw,
		[CLK_SPINLOCK_BUS]		= &spinlock_clk.common.hw,
		[CLK_MSGBOX0_CLK]		= &msgbox0_clk.common.hw,
		[CLK_MSGBOX1_CLK]		= &msgbox1_clk.common.hw,
		[CLK_MSGBOX2_CLK]		= &msgbox2_clk.common.hw,
		[CLK_PWM0_CLK]			= &pwm0_clk.common.hw,
		[CLK_PWM1_CLK]			= &pwm1_clk.common.hw,
		[CLK_DBGSYS_CLK]		= &dbgsys_clk.common.hw,
		[CLK_DBGPAD_CLK]		= &dbgpad_clk.common.hw,
		[CLK_TIMER0_CLK0]		= &timer0_clk0_clk.common.hw,
		[CLK_TIMER0_CLK1]		= &timer0_clk1_clk.common.hw,
		[CLK_TIMER0_CLK2]		= &timer0_clk2_clk.common.hw,
		[CLK_TIMER0_CLK3]		= &timer0_clk3_clk.common.hw,
		[CLK_TIMER0_CLK4]		= &timer0_clk4_clk.common.hw,
		[CLK_TIMER0_CLK5]		= &timer0_clk5_clk.common.hw,
		[CLK_TIMER0_CLK6]		= &timer0_clk6_clk.common.hw,
		[CLK_TIMER0_CLK7]		= &timer0_clk7_clk.common.hw,
		[CLK_TIMER0_CLK8]		= &timer0_clk8_clk.common.hw,
		[CLK_TIMER0_CLK9]		= &timer0_clk9_clk.common.hw,
		[CLK_TIMER0]			= &timer0_clk.common.hw,
		[CLK_TIMER1_CLK0]		= &timer1_clk0_clk.common.hw,
		[CLK_TIMER1_CLK1]		= &timer1_clk1_clk.common.hw,
		[CLK_TIMER1_CLK2]		= &timer1_clk2_clk.common.hw,
		[CLK_TIMER1_CLK3]		= &timer1_clk3_clk.common.hw,
		[CLK_TIMER1_CLK4]		= &timer1_clk4_clk.common.hw,
		[CLK_TIMER1_CLK5]		= &timer1_clk5_clk.common.hw,
		[CLK_TIMER1]			= &timer1_clk.common.hw,
		[CLK_DE0]			= &de0_clk.common.hw,
		[CLK_DE0_BUS]			= &de0_bus_clk.common.hw,
		[CLK_DE1]			= &de1_clk.common.hw,
		[CLK_DE1_BUS]			= &de1_bus_clk.common.hw,
		[CLK_DI]			= &di_clk.common.hw,
		[CLK_DI_BUS]			= &di_bus_clk.common.hw,
		[CLK_G2D]			= &g2d_clk.common.hw,
		[CLK_G2D_BUS]			= &g2d_bus_clk.common.hw,
		[CLK_VE_ENC0]			= &ve_enc0_clk.common.hw,
		[CLK_VE_ENC1]			= &ve_enc1_clk.common.hw,
		[CLK_VE_DEC]			= &ve_dec_clk.common.hw,
		[CLK_VE_ENC1_BUS]		= &ve_enc1_bus_clk.common.hw,
		[CLK_VE_ENC0_BUS]		= &ve_enc0_bus_clk.common.hw,
		[CLK_CE]			= &ce_clk.common.hw,
		[CLK_CE_SYS]			= &ce_sys_clk.common.hw,
		[CLK_CE_BUS]			= &ce_bus_clk.common.hw,
		[CLK_NPU]			= &npu_clk.common.hw,
		[CLK_NPU_BUS]			= &npu_bus_clk.common.hw,
		[CLK_AIPU]			= &aipu_clk.common.hw,
		[CLK_AIPU_BUS]			= &aipu_bus_clk.common.hw,
		[CLK_GPU0]			= &gpu0_clk.common.hw,
		[CLK_GPU0_BUS]			= &gpu0_bus_clk.common.hw,
		[CLK_DSP]			= &dsp_clk.common.hw,
		[CLK_DRAM0]			= &dram0_clk.common.hw,
		[CLK_DRAM0_BUS]			= &dram0_bus_clk.common.hw,
		[CLK_NAND0_CLK0]		= &nand0_clk0_clk.common.hw,
		[CLK_NAND0_CLK1]		= &nand0_clk1_clk.common.hw,
		[CLK_NAND0_BUS]			= &nand0_bus_clk.common.hw,
		[CLK_SMHC0]			= &smhc0_clk.common.hw,
		[CLK_SMHC0_24M]			= &smhc0_24m_clk.common.hw,
		[CLK_SMHC0_BUS]			= &smhc0_bus_clk.common.hw,
		[CLK_SMHC1]			= &smhc1_clk.common.hw,
		[CLK_SMHC1_BUS]			= &smhc1_bus_clk.common.hw,
		[CLK_SMHC2]			= &smhc2_clk.common.hw,
		[CLK_SMHC2_24M]			= &smhc2_24m_clk.common.hw,
		[CLK_SMHC2_BUS]			= &smhc2_bus_clk.common.hw,
		[CLK_SMHC3]			= &smhc3_clk.common.hw,
		[CLK_SMHC3_24M]			= &smhc3_24m_clk.common.hw,
		[CLK_SMHC3_BUS]			= &smhc3_bus_clk.common.hw,
		[CLK_UFS_AXI]			= &ufs_axi_clk.common.hw,
		[CLK_UFS]			= &ufs_clk.common.hw,
		[CLK_UART0]			= &uart0_clk.common.hw,
		[CLK_UART1]			= &uart1_clk.common.hw,
		[CLK_UART2]			= &uart2_clk.common.hw,
		[CLK_UART3]			= &uart3_clk.common.hw,
		[CLK_UART4]			= &uart4_clk.common.hw,
		[CLK_UART5]			= &uart5_clk.common.hw,
		[CLK_UART6]			= &uart6_clk.common.hw,
		[CLK_UART7]			= &uart7_clk.common.hw,
		[CLK_UART8]			= &uart8_clk.common.hw,
		[CLK_TWI0]			= &twi0_clk.common.hw,
		[CLK_TWI1]			= &twi1_clk.common.hw,
		[CLK_TWI2]			= &twi2_clk.common.hw,
		[CLK_TWI3]			= &twi3_clk.common.hw,
		[CLK_TWI4]			= &twi4_clk.common.hw,
		[CLK_TWI5]			= &twi5_clk.common.hw,
		[CLK_TWI6]			= &twi6_clk.common.hw,
		[CLK_TWI7]			= &twi7_clk.common.hw,
		[CLK_TWI8]			= &twi8_clk.common.hw,
		[CLK_SPI0]			= &spi0_clk.common.hw,
		[CLK_SPI0_BUS]			= &spi0_bus_clk.common.hw,
		[CLK_SPI1]			= &spi1_clk.common.hw,
		[CLK_SPI1_BUS]			= &spi1_bus_clk.common.hw,
		[CLK_SPI2]			= &spi2_clk.common.hw,
		[CLK_SPI2_BUS]			= &spi2_bus_clk.common.hw,
		[CLK_SPIF]			= &spif_clk.common.hw,
		[CLK_SPIF_BUS]			= &spif_bus_clk.common.hw,
		[CLK_SPIF3]			= &spif3_clk.common.hw,
		[CLK_SPIF3_BUS]			= &spif3_bus_clk.common.hw,
		[CLK_SPIF4]			= &spif4_clk.common.hw,
		[CLK_SPIF4_BUS]			= &spif4_bus_clk.common.hw,
		[CLK_SPIF5]			= &spif5_clk.common.hw,
		[CLK_SPIF5_BUS]			= &spif5_bus_clk.common.hw,
		[CLK_SPIF6]			= &spif6_clk.common.hw,
		[CLK_SPIF6_BUS]			= &spif6_bus_clk.common.hw,
		[CLK_GPADC0_24M]		= &gpadc0_24m_clk.common.hw,
		[CLK_GPADC0]			= &gpadc0_clk.common.hw,
		[CLK_GPADC1_24M]		= &gpadc1_24m_clk.common.hw,
		[CLK_GPADC1]			= &gpadc1_clk.common.hw,
		[CLK_THS0]			= &ths0_clk.common.hw,
		[CLK_IRRX]			= &irrx_clk.common.hw,
		[CLK_IRRX_BUS]			= &irrx_bus_clk.common.hw,
		[CLK_IRTX]			= &irtx_clk.common.hw,
		[CLK_IRTX_BUS]			= &irtx_bus_clk.common.hw,
		[CLK_LRADC]			= &lradc_clk.common.hw,
		[CLK_LBC]			= &lbc_clk.common.hw,
		[CLK_LBC_BUS]			= &lbc_bus_clk.common.hw,
		[CLK_I2SPCM1]			= &i2spcm1_clk.common.hw,
		[CLK_I2SPCM1_BUS]		= &i2spcm1_bus_clk.common.hw,
		[CLK_I2SPCM2]			= &i2spcm2_clk.common.hw,
		[CLK_I2SPCM2_ASRC]		= &i2spcm2_asrc_clk.common.hw,
		[CLK_I2SPCM2_BUS]		= &i2spcm2_bus_clk.common.hw,
		[CLK_I2SPCM3]			= &i2spcm3_clk.common.hw,
		[CLK_I2SPCM4]			= &i2spcm4_clk.common.hw,
		[CLK_I2SPCM4_BUS]		= &i2spcm4_bus_clk.common.hw,
		[CLK_I2SPCM5]			= &i2spcm5_clk.common.hw,
		[CLK_I2SPCM5_BUS]		= &i2spcm5_bus_clk.common.hw,
		[CLK_SPDIF_TX]			= &spdif_tx_clk.common.hw,
		[CLK_SPDIF_RX]			= &spdif_rx_clk.common.hw,
		[CLK_SPDIF]			= &spdif_clk.common.hw,
		[CLK_USB2_HOST0]		= &usb2_host0_clk.common.hw,
		[CLK_USB2_OTG0]			= &usb2_otg0_clk.common.hw,
		[CLK_USB2_EHCI0]		= &usb2_ehci0_clk.common.hw,
		[CLK_USB2_OHCI0]		= &usb2_ohci0_clk.common.hw,
		[CLK_USB2_HOST1]		= &usb2_host1_clk.common.hw,
		[CLK_USB2_OHCI1]		= &usb2_ohci1_clk.common.hw,
		[CLK_USB2_HOST2]		= &usb2_host2_clk.common.hw,
		[CLK_USB2_EHCI2]		= &usb2_ehci2_clk.common.hw,
		[CLK_USB2_OHCI2]		= &usb2_ohci2_clk.common.hw,
		[CLK_USB2_REF]			= &usb2_ref_clk.common.hw,
		[CLK_USB3_USB2_REF]		= &usb3_usb2_ref_clk.common.hw,
		[CLK_USB3_SUSPEND]		= &usb3_suspend_clk.common.hw,
		[CLK_USB3_MF]			= &usb3_mf_clk.common.hw,
		[CLK_PCIE0_AUX]			= &pcie0_aux_clk.common.hw,
		[CLK_GMAC_PTP]			= &gmac_ptp_clk.common.hw,
		[CLK_GMAC0_PHY]			= &gmac0_phy_clk.common.hw,
		[CLK_GMAC0]			= &gmac0_clk.common.hw,
		[CLK_GMAC1_PHY]			= &gmac1_phy_clk.common.hw,
		[CLK_GMAC1]			= &gmac1_clk.common.hw,
		[CLK_VO0_TCONLCD0]		= &vo0_tconlcd0_clk.common.hw,
		[CLK_VO0_TCONLCD0_BUS]		= &vo0_tconlcd0_bus_clk.common.hw,
		[CLK_VO0_TCONLCD1]		= &vo0_tconlcd1_clk.common.hw,
		[CLK_VO0_TCONLCD1_BUS]		= &vo0_tconlcd1_bus_clk.common.hw,
		[CLK_VO0_TCONLCD2]		= &vo0_tconlcd2_clk.common.hw,
		[CLK_VO0_TCONLCD2_BUS]		= &vo0_tconlcd2_bus_clk.common.hw,
		[CLK_VO0_TCONLCD3]		= &vo0_tconlcd3_clk.common.hw,
		[CLK_VO0_TCONLCD3_BUS]		= &vo0_tconlcd3_bus_clk.common.hw,
		[CLK_DSI0]			= &dsi0_clk.common.hw,
		[CLK_DSI0_BUS]			= &dsi0_bus_clk.common.hw,
		[CLK_DSI1]			= &dsi1_clk.common.hw,
		[CLK_DSI1_BUS]			= &dsi1_bus_clk.common.hw,
		[CLK_COMBPHY0]			= &combphy0_clk.common.hw,
		[CLK_COMBPHY1]			= &combphy1_clk.common.hw,
		[CLK_TCONTV0]			= &tcontv0_clk.common.hw,
		[CLK_TCONTV0_BUS]		= &tcontv0_bus_clk.common.hw,
		[CLK_TCONTV1]			= &tcontv1_clk.common.hw,
		[CLK_TCONTV2]			= &tcontv2_clk.common.hw,
		[CLK_EDP_CLK0]			= &edp_clk0_clk.common.hw,
		[CLK_EDP_CLK1]			= &edp_clk1_clk.common.hw,
		[CLK_EDP_BUS]			= &edp_bus_clk.common.hw,
		[CLK_HDMI_REF]			= &hdmi_ref_clk.common.hw,
		[CLK_HDMI_PRE]			= &hdmi_pre_clk.common.hw,
		[CLK_HDMI_HDCP]			= &hdmi_hdcp_clk.common.hw,
		[CLK_HDMI]			= &hdmi_clk.common.hw,
		[CLK_DPSS_TOP0]			= &dpss_top0_clk.common.hw,
		[CLK_DPSS_TOP1]			= &dpss_top1_clk.common.hw,
		[CLK_LEDC]			= &ledc_clk.common.hw,
		[CLK_LEDC_BUS]			= &ledc_bus_clk.common.hw,
		[CLK_CSI_MASTER0]		= &csi_master0_clk.common.hw,
		[CLK_CSI_MASTER1]		= &csi_master1_clk.common.hw,
		[CLK_CSI_MASTER2]		= &csi_master2_clk.common.hw,
		[CLK_CSI_MASTER3]		= &csi_master3_clk.common.hw,
		[CLK_CSI]			= &csi_clk.common.hw,
		[CLK_BUS_CSI]			= &csi_bus_clk.common.hw,
		[CLK_ISP]			= &isp_clk.common.hw,
	},
	.num = CLK_NUMBER,
};
/* ccu_def_end */

static struct ccu_common *sun60iw1_ccu_clks[] = {
	&pll_ref_clk.common,
	&pll_ddr_clk.common,
	&pll_peri0_clk.common,
	&pll_peri0_2x_clk.common,
	&pll_peri0_800m_clk.common,
	&pll_peri0_480m_clk.common,
	&pll_peri1_clk.common,
	&pll_peri1_2x_clk.common,
	&pll_peri1_800m_clk.common,
	&pll_peri1_480m_clk.common,
	&pll_gpu0_clk.common,
	&pll_video0_4x_clk.common,
	&pll_video0_3x_clk.common,
	&pll_video1_4x_clk.common,
	&pll_video1_3x_clk.common,
	&pll_video2_4x_clk.common,
	&pll_video2_3x_clk.common,
	&pll_video3_4x_clk.common,
	&pll_video3_3x_clk.common,
	&pll_ve0_clk.common,
	&pll_ve1_clk.common,
	&pll_audio0_4x_clk.common,
	&pll_npu_clk.common,
	&pll_de_4x_clk.common,
	&pll_de_3x_clk.common,
	&pll_cci_clk.common,
	&ahb_clk.common,
	&apb0_clk.common,
	&apb1_clk.common,
	&apb_uart_clk.common,
	&trace_clk.common,
	&cci_clk.common,
	&gic0_clk.common,
	&gic1_clk.common,
	&nsi_clk.common,
	&mbus_clk.common,
	&msi_lite0_clk.common,
	&msi_lite1_clk.common,
	&ve_enc1_mbus_clk.common,
	&ve_dec_mbus_clk.common,
	&gmac1_mbus_mclk.common,
	&gmac0_mbus_mclk.common,
	&isp_mbus_clk.common,
	&csi_mbus_clk.common,
	&nand_mbus_clk.common,
	&dma1_mclk.common,
	&ce_mclk.common,
	&ve_enc0_mclk.common,
	&dma0_mclk.common,
	&dma0_clk.common,
	&dma1_clk.common,
	&spinlock_clk.common,
	&msgbox0_clk.common,
	&msgbox1_clk.common,
	&msgbox2_clk.common,
	&pwm0_clk.common,
	&pwm1_clk.common,
	&dbgsys_clk.common,
	&dbgpad_clk.common,
	&timer0_clk0_clk.common,
	&timer0_clk1_clk.common,
	&timer0_clk2_clk.common,
	&timer0_clk3_clk.common,
	&timer0_clk4_clk.common,
	&timer0_clk5_clk.common,
	&timer0_clk6_clk.common,
	&timer0_clk7_clk.common,
	&timer0_clk8_clk.common,
	&timer0_clk9_clk.common,
	&timer0_clk.common,
	&timer1_clk0_clk.common,
	&timer1_clk1_clk.common,
	&timer1_clk2_clk.common,
	&timer1_clk3_clk.common,
	&timer1_clk4_clk.common,
	&timer1_clk5_clk.common,
	&timer1_clk.common,
	&de0_clk.common,
	&de0_bus_clk.common,
	&de1_clk.common,
	&de1_bus_clk.common,
	&di_clk.common,
	&di_bus_clk.common,
	&g2d_clk.common,
	&g2d_bus_clk.common,
	&ve_enc0_clk.common,
	&ve_enc1_clk.common,
	&ve_dec_clk.common,
	&ve_enc1_bus_clk.common,
	&ve_enc0_bus_clk.common,
	&ce_clk.common,
	&ce_sys_clk.common,
	&ce_bus_clk.common,
	&npu_clk.common,
	&npu_bus_clk.common,
	&aipu_clk.common,
	&aipu_bus_clk.common,
	&gpu0_clk.common,
	&gpu0_bus_clk.common,
	&dsp_clk.common,
	&dram0_clk.common,
	&dram0_bus_clk.common,
	&nand0_clk0_clk.common,
	&nand0_clk1_clk.common,
	&nand0_bus_clk.common,
	&smhc0_clk.common,
	&smhc0_24m_clk.common,
	&smhc0_bus_clk.common,
	&smhc1_clk.common,
	&smhc1_bus_clk.common,
	&smhc2_clk.common,
	&smhc2_24m_clk.common,
	&smhc2_bus_clk.common,
	&smhc3_clk.common,
	&smhc3_24m_clk.common,
	&smhc3_bus_clk.common,
	&ufs_axi_clk.common,
	&ufs_clk.common,
	&uart0_clk.common,
	&uart1_clk.common,
	&uart2_clk.common,
	&uart3_clk.common,
	&uart4_clk.common,
	&uart5_clk.common,
	&uart6_clk.common,
	&uart7_clk.common,
	&uart8_clk.common,
	&twi0_clk.common,
	&twi1_clk.common,
	&twi2_clk.common,
	&twi3_clk.common,
	&twi4_clk.common,
	&twi5_clk.common,
	&twi6_clk.common,
	&twi7_clk.common,
	&twi8_clk.common,
	&spi0_clk.common,
	&spi0_bus_clk.common,
	&spi1_clk.common,
	&spi1_bus_clk.common,
	&spi2_clk.common,
	&spi2_bus_clk.common,
	&spif_clk.common,
	&spif_bus_clk.common,
	&spif3_clk.common,
	&spif3_bus_clk.common,
	&spif4_clk.common,
	&spif4_bus_clk.common,
	&spif5_clk.common,
	&spif5_bus_clk.common,
	&spif6_clk.common,
	&spif6_bus_clk.common,
	&gpadc0_24m_clk.common,
	&gpadc0_clk.common,
	&gpadc1_24m_clk.common,
	&gpadc1_clk.common,
	&ths0_clk.common,
	&irrx_clk.common,
	&irrx_bus_clk.common,
	&irtx_clk.common,
	&irtx_bus_clk.common,
	&lradc_clk.common,
	&lbc_clk.common,
	&lbc_bus_clk.common,
	&i2spcm1_clk.common,
	&i2spcm1_bus_clk.common,
	&i2spcm2_clk.common,
	&i2spcm2_asrc_clk.common,
	&i2spcm2_bus_clk.common,
	&i2spcm3_clk.common,
	&i2spcm4_clk.common,
	&i2spcm4_bus_clk.common,
	&i2spcm5_clk.common,
	&i2spcm5_bus_clk.common,
	&spdif_tx_clk.common,
	&spdif_rx_clk.common,
	&spdif_clk.common,
	&usb2_host0_clk.common,
	&usb2_otg0_clk.common,
	&usb2_ehci0_clk.common,
	&usb2_ohci0_clk.common,
	&usb2_host1_clk.common,
	&usb2_ohci1_clk.common,
	&usb2_host2_clk.common,
	&usb2_ehci2_clk.common,
	&usb2_ohci2_clk.common,
	&usb2_ref_clk.common,
	&usb3_usb2_ref_clk.common,
	&usb3_suspend_clk.common,
	&usb3_mf_clk.common,
	&pcie0_aux_clk.common,
	&gmac_ptp_clk.common,
	&gmac0_phy_clk.common,
	&gmac0_clk.common,
	&gmac1_phy_clk.common,
	&gmac1_clk.common,
	&vo0_tconlcd0_clk.common,
	&vo0_tconlcd0_bus_clk.common,
	&vo0_tconlcd1_clk.common,
	&vo0_tconlcd1_bus_clk.common,
	&vo0_tconlcd2_clk.common,
	&vo0_tconlcd2_bus_clk.common,
	&vo0_tconlcd3_clk.common,
	&vo0_tconlcd3_bus_clk.common,
	&dsi0_clk.common,
	&dsi0_bus_clk.common,
	&dsi1_clk.common,
	&dsi1_bus_clk.common,
	&combphy0_clk.common,
	&combphy1_clk.common,
	&tcontv0_clk.common,
	&tcontv0_bus_clk.common,
	&tcontv1_clk.common,
	&tcontv2_clk.common,
	&edp_clk0_clk.common,
	&edp_clk1_clk.common,
	&edp_bus_clk.common,
	&hdmi_ref_clk.common,
	&hdmi_pre_clk.common,
	&hdmi_hdcp_clk.common,
	&hdmi_clk.common,
	&dpss_top0_clk.common,
	&dpss_top1_clk.common,
	&ledc_clk.common,
	&ledc_bus_clk.common,
	&csi_master0_clk.common,
	&csi_master1_clk.common,
	&csi_master2_clk.common,
	&csi_master3_clk.common,
	&csi_clk.common,
	&csi_bus_clk.common,
	&isp_clk.common,
};

static const u32 pll_regs[] = {
	SUN60IW1_PLL_REF_CTRL_REG,
	SUN60IW1_PLL_DDR_CTRL_REG,
	SUN60IW1_PLL_PERI0_CTRL_REG,
	SUN60IW1_PLL_PERI1_CTRL_REG,
	SUN60IW1_PLL_GPU0_CTRL_REG,
	SUN60IW1_PLL_VIDEO0_CTRL_REG,
	SUN60IW1_PLL_VIDEO1_CTRL_REG,
	SUN60IW1_PLL_VIDEO3_CTRL_REG,
	SUN60IW1_PLL_VE0_CTRL_REG,
	SUN60IW1_PLL_VE1_CTRL_REG,
	SUN60IW1_PLL_AUDIO0_CTRL_REG,
	SUN60IW1_PLL_NPU_CTRL_REG,
	SUN60IW1_PLL_DE_CTRL_REG,
	SUN60IW1_PLL_CCI_CTRL_REG,
};

static const struct sunxi_ccu_desc sun60iw1_ccu_desc = {
	.ccu_clks	= sun60iw1_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun60iw1_ccu_clks),

	.hw_clks	= &sun60iw1_hw_clks,

	.resets		= sun60iw1_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun60iw1_ccu_resets),
};

static void __init of_sun60iw1_ccu_init(struct device_node *node)
{
	void __iomem *reg;
	int i;
	u32 val;

	reg = of_iomap(node, 0);
	if (IS_ERR(reg))
		return;

	/* Enable the lock bits on all PLLs */
	for (i = 0; i < ARRAY_SIZE(pll_regs); i++) {
		val = readl(reg + pll_regs[i]);
		val |= BIT(29);
		val |= BIT(31);
		writel(val, reg + pll_regs[i]);
	}

	sunxi_ccu_probe(node, reg, &sun60iw1_ccu_desc);
}

CLK_OF_DECLARE(sun60iw1_ccu_init, "allwinner,sun60iw1-ccu", of_sun60iw1_ccu_init);

MODULE_DESCRIPTION("Allwinner sun60iw1 clk driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.1.0");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
