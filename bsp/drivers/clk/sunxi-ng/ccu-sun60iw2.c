// SPDX-License-Identifier: GPL-2.0
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

#include "ccu-sun60iw2.h"

#define SUNXI_CCU_VERSION		"1.0.15"

/* ccu_des_start */

#define UPD_KEY_VALUE 0x8000000
#define SUN60IW2_PLL_AUDIO0_PATTERN1_REG		0x26C
#define PLL_AUDIO0_PAT1_PI_EN				1
#define PLL_AUDIO0_PAT1_SDM_EN				1
#define PLL_AUDIO0_PAT1_PI_EN_OFFSET			31
#define PLL_AUDIO0_PAT1_SDM_EN_OFFSET			27

#define SUN60IW2_PLL_REF_CTRL_REG   0x0000
static struct ccu_nm pll_ref_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 80, 116),
	.m		= _SUNXI_CCU_DIV(16, 7), /* output divider */
	.min_rate	= 15000000,
	.max_rate	= 2800000000,
	.common		= {
		.reg		= 0x0000,
		.hw.init	= CLK_HW_INIT("pll-ref", "dcxo",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IGNORE_UNUSED),
	},
};

#define SUN60IW2_PLL_DDR_CTRL_REG   0x0020
static struct ccu_nm pll_ddr_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 53, 105),
	.m		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.min_rate	= 159000000,
	.max_rate	= 2520000000,
	.sdm		= _SUNXI_CCU_SDM_PATTERN1_INFO(BIT(31), 0x0028, BIT(31) | BIT(27), 0x002C),
	.common		= {
		.reg		= 0x0020,
		.hw.init	= CLK_HW_INIT("pll-ddr", "pll-ref",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IGNORE_UNUSED),
	},
};

#define SUN60IW2_PLL_PERI0_CTRL_REG   0x00A0
static struct ccu_nm pll_peri0_clk = {
	.output		= BIT(25) | BIT(26) | BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 53, 105),
	.min_rate	= 1272000000,
	.max_rate	= 2520000000,
	.sdm		= _SUNXI_CCU_SDM_PATTERN1_INFO(BIT(31), 0x00A8, BIT(31) | BIT(27), 0x00AC),
	.common		= {
		.reg		= 0x00A0,
		.hw.init	= CLK_HW_INIT("pll-peri0", "pll-ref",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IGNORE_UNUSED),
	},
};

static SUNXI_CCU_M(pll_peri0_2x_clk, "pll-peri0-2x",
		"pll-peri0", 0x00A0,
		20, 3,
		0);

static SUNXI_CCU_M(pll_peri0_800m_clk, "pll-peri0-800m",
		"pll-peri0", 0x00A0,
		16, 3,
		0);

static SUNXI_CCU_M(pll_peri0_480m_clk, "pll-peri0-480m",
		"pll-peri0", 0x00A0,
		2, 3,
		0);

static CLK_FIXED_FACTOR(pll_peri0_600m_clk, "pll-peri0-600m", "pll-peri0-2x", 2, 1, 0);
static CLK_FIXED_FACTOR(pll_peri0_400m_clk, "pll-peri0-400m", "pll-peri0-2x", 3, 1, 0);
static CLK_FIXED_FACTOR(pll_peri0_300m_clk, "pll-peri0-300m", "pll-peri0-600m", 2, 1, 0);
static CLK_FIXED_FACTOR(pll_peri0_200m_clk, "pll-peri0-200m", "pll-peri0-400m", 2, 1, 0);
static CLK_FIXED_FACTOR(pll_peri0_160m_clk, "pll-peri0-160m", "pll-peri0-480m", 3, 1, 0);
static CLK_FIXED_FACTOR(pll_peri0_150m_clk, "pll-peri0-150m", "pll-peri0-300m", 2, 1, 0);

#define SUN60IW2_PLL_PERI1_CTRL_REG   0x00C0
static struct ccu_nm pll_peri1_clk = {
	.output		= BIT(25) | BIT(26) | BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 53, 105),
	.min_rate	= 1272000000,
	.max_rate	= 2520000000,
	.sdm		= _SUNXI_CCU_SDM_PATTERN1_INFO(BIT(31), 0x00C8, BIT(31) | BIT(27), 0x00CC),
	.common		= {
		.reg		= 0x00C0,
		.hw.init	= CLK_HW_INIT("pll-peri1", "pll-ref",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IGNORE_UNUSED),
	},
};

static SUNXI_CCU_M(pll_peri1_2x_clk, "pll-peri1-2x",
		"pll-peri1", 0x00C0,
		20, 3,
		0);

static SUNXI_CCU_M(pll_peri1_800m_clk, "pll-peri1-800m",
		"pll-peri1", 0x00C0,
		16, 3,
		0);

static SUNXI_CCU_M(pll_peri1_480m_clk, "pll-peri1-480m",
		"pll-peri1", 0x00C0,
		2, 3,
		0);

static CLK_FIXED_FACTOR(pll_peri1_600m_clk, "pll-peri1-600m", "pll-peri1-2x", 2, 1, 0);
static CLK_FIXED_FACTOR(pll_peri1_400m_clk, "pll-peri1-400m", "pll-peri1-2x", 3, 1, 0);
static CLK_FIXED_FACTOR(pll_peri1_300m_clk, "pll-peri1-300m", "pll-peri1-600m", 2, 1, 0);
static CLK_FIXED_FACTOR(pll_peri1_200m_clk, "pll-peri1-200m", "pll-peri1-400m", 2, 1, 0);
static CLK_FIXED_FACTOR(pll_peri1_160m_clk, "pll-peri1-160m", "pll-peri1-480m", 3, 1, 0);
static CLK_FIXED_FACTOR(pll_peri1_150m_clk, "pll-peri1-150m", "pll-peri1-300m", 2, 1, 0);
static CLK_FIXED_FACTOR(hdmi_cec_32k_clk,   "hdmi-cec-clk32k", "pll-peri0-2x", 1, 36621, 0);


#define SUN60IW2_PLL_GPU0_CTRL_REG   0x00E0
static struct ccu_nm pll_gpu0_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 53, 105),
	.m		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.min_rate	= 159000000,
	.max_rate	= 2520000000,
	.sdm		= _SUNXI_CCU_SDM_PATTERN1_INFO(BIT(31), 0x00E8, BIT(31) | BIT(27), 0x00EC),
	.common		= {
		.reg		= 0x00E0,
		.hw.init	= CLK_HW_INIT("pll-gpu", "pll-ref",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IGNORE_UNUSED),
	},
};

#define SUN60IW2_PLL_VIDEO0_CTRL_REG   0x0120
static struct ccu_nm pll_video0_clk = {
	.output		= BIT(26) | BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 53, 105),
	.min_rate	= 1272000000,
	.max_rate	= 2520000000,
	.sdm		= _SUNXI_CCU_SDM_PATTERN1_INFO(BIT(31), 0x0128, BIT(31) | BIT(27), 0x012C),
	.common		= {
		.reg		= 0x0120,
		.hw.init	= CLK_HW_INIT("pll-video0", "pll-ref",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IGNORE_UNUSED),
	},
};

static SUNXI_CCU_M(pll_video0_4x_clk, "pll-video0-4x",
		"pll-video0", 0x0120,
		20, 3,
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M(pll_video0_3x_clk, "pll-video0-3x",
		"pll-video0", 0x0120,
		16, 3,
		CLK_SET_RATE_PARENT);


#define SUN60IW2_PLL_VIDEO1_CTRL_REG   0x0140
static struct ccu_nm pll_video1_clk = {
	.output		= BIT(26) | BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 53, 105),
	.min_rate	= 1272000000,
	.max_rate	= 2520000000,
	.sdm		= _SUNXI_CCU_SDM_PATTERN1_INFO(BIT(31), 0x0148, BIT(31) | BIT(27), 0x014C),
	.common		= {
		.reg		= 0x0140,
		.hw.init	= CLK_HW_INIT("pll-video1", "pll-ref",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IGNORE_UNUSED),
	},
};

static SUNXI_CCU_M(pll_video1_4x_clk, "pll-video1-4x",
		"pll-video1", 0x0140,
		20, 3,
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M(pll_video1_3x_clk, "pll-video1-3x",
		"pll-video1", 0x0140,
		16, 3,
		CLK_SET_RATE_PARENT);

#define SUN60IW2_PLL_VIDEO2_CTRL_REG   0x0160
static struct ccu_nm pll_video2_clk = {
	.output		= BIT(26) | BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 53, 105),
	.min_rate	= 1272000000,
	.max_rate	= 2520000000,
	.sdm		= _SUNXI_CCU_SDM_PATTERN1_INFO(BIT(31), 0x0168, BIT(31) | BIT(27), 0x016C),
	.common		= {
		.reg		= 0x0160,
		.hw.init	= CLK_HW_INIT("pll-video2", "pll-ref",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IGNORE_UNUSED),
	},
};

static SUNXI_CCU_M(pll_video2_4x_clk, "pll-video2-4x",
		"pll-video2", 0x0160,
		20, 3,
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M(pll_video2_3x_clk, "pll-video2-3x",
		"pll-video2", 0x0160,
		16, 3,
		CLK_SET_RATE_PARENT);

#define SUN60IW2_PLL_VE0_CTRL_REG   0x0220
static struct ccu_nm pll_ve0_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 53, 105),
	.m		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.min_rate	= 159000000,
	.max_rate	= 2520000000,
	.sdm		= _SUNXI_CCU_SDM_PATTERN1_INFO(BIT(31), 0x0228, BIT(31) | BIT(27), 0x022C),
	.common		= {
		.reg		= 0x0220,
		.hw.init	= CLK_HW_INIT("pll-ve0", "pll-ref",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IGNORE_UNUSED),
	},
};

#define SUN60IW2_PLL_VE1_CTRL_REG   0x0240
static struct ccu_nm pll_ve1_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 53, 105),
	.m		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.min_rate	= 159000000,
	.max_rate	= 2520000000,
	.sdm		= _SUNXI_CCU_SDM_PATTERN1_INFO(BIT(31), 0x0248, BIT(31) | BIT(27), 0x024C),
	.common		= {
		.reg		= 0x0240,
		.hw.init	= CLK_HW_INIT("pll-ve1", "pll-ref",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IGNORE_UNUSED),
	},
};

#define SUN60IW2_PLL_AUDIO0_CTRL_REG   0x0260

static struct ccu_sdm_setting pll_audio0_4x_sdm_table[] = {
	{ .rate = 90316800,  .pattern = 0xA002872b, .m = 20, .n = 75 }, /* 22.5792 * 4 */
};

static struct ccu_nm pll_audio0_4x_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 53, 105),
	.m		= _SUNXI_CCU_DIV(16, 7),
	.sdm		= _SUNXI_CCU_SDM_PATTERN1(pll_audio0_4x_sdm_table,
			0x0268, BIT(31), /* pattern0 */
			0x026c, BIT(31) | BIT(27)), /* pattern1 */
	.min_rate	= 90316800,
	.max_rate	= 90316800,
	.common		= {
		.reg		= 0x0260,
		.features	= CCU_FEATURE_SIGMA_DELTA_MOD,
		.hw.init	= CLK_HW_INIT("pll-audio0-4x", "pll-ref",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IGNORE_UNUSED),
	},
};

#define SUN60IW2_PLL_AUDIO1_CTRL_REG   0x0280
static struct ccu_nm pll_audio1_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 75, 129),
	.min_rate	= 1800000000,
	.max_rate	= 3096000000,
	.sdm		= _SUNXI_CCU_SDM_PATTERN1_INFO(BIT(31), 0x0288, BIT(31) | BIT(27), 0x028C),
	.common		= {
		.reg		= 0x0280,
		.hw.init	= CLK_HW_INIT("pll-audio1", "pll-ref",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IGNORE_UNUSED),
	},
};

static SUNXI_CCU_M(pll_audio1_div2_clk, "pll-audio1-div2",
		"pll-audio1", 0x0280,
		20, 3,
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M(pll_audio1_div5_clk, "pll-audio1-div5",
		"pll-audio1", 0x0280,
		16, 3,
		CLK_SET_RATE_PARENT);  /* The default frequency is 614.4MHZ (24.576MHZ * 25) */

#define SUN60IW2_PLL_NPU_CTRL_REG   0x02A0
static struct ccu_nm pll_npu_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 53, 105),
	.m		= _SUNXI_CCU_DIV(20, 3), /* output divider */
	.min_rate	= 159000000,
	.max_rate	= 2520000000,
	.sdm		= _SUNXI_CCU_SDM_PATTERN1_INFO(BIT(31), 0x02A8, BIT(31) | BIT(27), 0x02AC),
	.common		= {
		.reg		= 0x02A0,
		.hw.init	= CLK_HW_INIT("pll-npu", "pll-ref",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IGNORE_UNUSED),
	},
};

#define SUN60IW2_PLL_DE_CTRL_REG   0x02E0
static struct ccu_nm pll_de_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 53, 105),
	.min_rate	= 1272000000,
	.max_rate	= 2520000000,
	.sdm		= _SUNXI_CCU_SDM_PATTERN1_INFO(BIT(31), 0x02E8, BIT(31) | BIT(27), 0x02EC),
	.common		= {
		.reg		= 0x02E0,
		.hw.init	= CLK_HW_INIT("pll-de", "pll-ref",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IGNORE_UNUSED),
	},
};

static SUNXI_CCU_M_WITH_GATE(pll_de_4x_clk, "pll-de-4x",
		"pll-de", 0x02E0,
		20, 3,
		BIT(27),
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

static SUNXI_CCU_M_WITH_GATE(pll_de_3x_clk, "pll-de-3x",
		"pll-de", 0x02E0,
		16, 3,
		BIT(26),
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

static const char * const apb0_parents[] = { "sys24M", "osc32k", "iosc", "pll-peri0-600m" };
SUNXI_CCU_M_WITH_MUX(ahb_clk, "ahb", apb0_parents,
		0x0500, 0, 5, 24, 2, CLK_SET_RATE_PARENT);

SUNXI_CCU_M_WITH_MUX(apb0_clk, "apb0", apb0_parents,
		0x0510, 0, 5, 24, 2, CLK_SET_RATE_PARENT);

SUNXI_CCU_M_WITH_MUX(apb1_clk, "apb1", apb0_parents,
		0x0518, 0, 5, 24, 2, CLK_SET_RATE_PARENT);

static const char * const apb_uart_parents[] = { "sys24M", "osc32k", "iosc", "pll-peri0-600m", "pll-peri0-480m" };

SUNXI_CCU_M_WITH_MUX(apb_uart_clk, "apb-uart", apb_uart_parents,
		0x0538, 0, 5, 24, 3, CLK_SET_RATE_PARENT);

static const char * const trace_parents[] = { "sys24M", "osc32k", "iosc", "pll-peri0-300m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE(trace_clk, "trace",
		trace_parents, 0x0540,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const gic_parents[] = { "sys24M", "osc32k", "pll-peri0-600m", "pll-peri0-480m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE(gic_clk, "gic",
		gic_parents, 0x0560,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

static const char * const cpu_peri_parents[] = { "sys24M", "osc32k", "pll-peri0-600m", "pll-peri0-480m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE(cpu_peri_clk, "cpu-peri",
		cpu_peri_parents, 0x0568,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(its_pcie0_a_clk, "its-pcie0-aclk",
		"dcxo",
		0x0574, BIT(1), 0);

static const char * const nsi_parents[] = { "sys24M", "pll-ddr", "pll-peri0-800m", "pll-peri0-600m", "pll-peri0-480m", "pll-de-3x" };

static SUNXI_CCU_M_WITH_MUX_GATE_KEY(nsi_clk, "nsi",
		nsi_parents, 0x0580,
		0, 5,
		24, 3,	/* mux */
		UPD_KEY_VALUE,
		BIT(31),	/* gate */
		CLK_SET_RATE_NO_REPARENT | CLK_GET_RATE_NOCACHE | CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(nsi_cfg_clk, "nsi-cfg",
		"dcxo",
		0x0584, BIT(0), 0);

static const char * const mbus_parents[] = { "sys24M", "pll-peri0-600m", "pll-ddr", "pll-peri0-480m", "pll-peri0-400m", "pll-npu" };

static SUNXI_CCU_M_WITH_MUX_GATE_KEY(mbus_clk, "mbus",
		mbus_parents, 0x0588,
		0, 5,
		24, 3,	/* mux */
		UPD_KEY_VALUE,
		BIT(31),	/* gate */
		CLK_SET_RATE_NO_REPARENT | CLK_GET_RATE_NOCACHE | CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(iommu0_sys_h_clk, "iommu0-sys-hclk",
		"dcxo",
		0x058C, BIT(2), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(iommu0_sys_p_clk, "iommu0-sys-pclk",
		"dcxo",
		0x058C, BIT(1), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(iommu0_sys_mbus_clk, "iommu0-sys-mclk",
		"dcxo",
		0x058C, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(msi_lite0_clk, "msi-lite0",
		"dcxo",
		0x0594, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(msi_lite1_clk, "msi-lite1",
		"dcxo",
		0x059C, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(msi_lite2_clk, "msi-lite2",
		"dcxo",
		0x05A4, BIT(0), 0);

static SUNXI_CCU_GATE(iommu1_sys_h_clk, "iommu1-sys-hclk",
		"dcxo",
		0x05B4, BIT(2), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(iommu1_sys_p_clk, "iommu1-sys-pclk",
		"dcxo",
		0x05B4, BIT(1), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(iommu1_sys_mbus_clk, "iommu1-sys-mclk",
		"dcxo",
		0x05B4, BIT(0), 0);

#define AHB_MASTER_KEY_VALUE	0x10000FF
static SUNXI_CCU_GATE_WITH_KEY(cpus_hclk_gate_clk, "cpus-hclk-gate",
		"dcxo", 0x05C0,
		AHB_MASTER_KEY_VALUE,
		BIT(28), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE_WITH_KEY(store_ahb_gate_clk, "store-ahb-gate",
		"dcxo", 0x05C0,
		AHB_MASTER_KEY_VALUE,
		BIT(24), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE_WITH_KEY(msilite0_ahb_gate_clk, "msilite0-ahb-gate",
		"dcxo", 0x05C0,
		AHB_MASTER_KEY_VALUE,
		BIT(16), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE_WITH_KEY(usb_sys_ahb_gate_clk, "usb-sys-ahb-gate",
		"dcxo", 0x05C0,
		AHB_MASTER_KEY_VALUE,
		BIT(9), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE_WITH_KEY(serdes_ahb_gate_clk, "serdes-ahb-gate",
		"dcxo", 0x05C0,
		AHB_MASTER_KEY_VALUE,
		BIT(8), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE_WITH_KEY(gpu0_ahb_gate_clk, "gpu0-ahb-gate",
		"dcxo", 0x05C0,
		AHB_MASTER_KEY_VALUE,
		BIT(7), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE_WITH_KEY(npu_ahb_gate_clk, "npu-ahb-gate",
		"dcxo", 0x05C0,
		AHB_MASTER_KEY_VALUE,
		BIT(6), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE_WITH_KEY(de_ahb_gate_clk, "de-ahb-gate",
		"dcxo", 0x05C0,
		AHB_MASTER_KEY_VALUE,
		BIT(5), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE_WITH_KEY(vid_out1_ahb_gate_clk, "vid-out1-ahb-gate",
		"dcxo", 0x05C0,
		AHB_MASTER_KEY_VALUE,
		BIT(4), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE_WITH_KEY(vid_out0_ahb_gate_clk, "vid-out0-ahb-gate",
		"dcxo", 0x05C0,
		AHB_MASTER_KEY_VALUE,
		BIT(3), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE_WITH_KEY(vid_in_ahb_gate_clk, "vid-in-ahb-gate",
		"dcxo", 0x05C0,
		AHB_MASTER_KEY_VALUE,
		BIT(2), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE_WITH_KEY(ve_enc_ahb_gate_clk, "ve-enc-ahb-gate",
		"dcxo", 0x05C0,
		AHB_MASTER_KEY_VALUE,
		BIT(1), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE_WITH_KEY(ve_ahb_gate_clk, "ve-ahb-gate",
		"dcxo", 0x05C0,
		AHB_MASTER_KEY_VALUE,
		BIT(0), CLK_IS_CRITICAL);

#define MBUS_MASTER_KEY_VALUE	0x41055800
static SUNXI_CCU_GATE_WITH_KEY(msilite2_mbus_gate_clk, "msilite2-mbus-gate",
		"dcxo", 0x05E0,
		MBUS_MASTER_KEY_VALUE,
		BIT(31), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE_WITH_KEY(store_mbus_gate_clk, "store-mbus-gate",
		"dcxo", 0x05E0,
		MBUS_MASTER_KEY_VALUE,
		BIT(30), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE_WITH_KEY(msilite0_mbus_gate_clk, "msilite0-mbus-gate",
		"dcxo", 0x05E0,
		MBUS_MASTER_KEY_VALUE,
		BIT(29), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE_WITH_KEY(serdes_mbus_gate_clk, "serdes-mbus-gate",
		"dcxo", 0x05E0,
		MBUS_MASTER_KEY_VALUE,
		BIT(28), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE_WITH_KEY(vid_in_mbus_gate_clk, "vid-in-mbus-gate",
		"dcxo", 0x05E0,
		MBUS_MASTER_KEY_VALUE,
		BIT(24), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE_WITH_KEY(npu_mbus_gate_clk, "npu-mbus-gate",
		"dcxo", 0x05E0,
		MBUS_MASTER_KEY_VALUE,
		BIT(18), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE_WITH_KEY(gpu0_mbus_gate_clk, "gpu0-mbus-gate",
		"dcxo", 0x05E0,
		MBUS_MASTER_KEY_VALUE,
		BIT(16), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE_WITH_KEY(ve_dec_mbus_gate_clk, "ve-dec-mbus-gate",
		"dcxo", 0x05E0,
		MBUS_MASTER_KEY_VALUE,
		BIT(14), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE_WITH_KEY(ve_mbus_gate_clk, "ve-mbus-gate",
		"dcxo", 0x05E0,
		MBUS_MASTER_KEY_VALUE,
		BIT(12), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE_WITH_KEY(desys_mbus_gate_clk, "desys-mbus-gate",
		"dcxo", 0x05E0,
		MBUS_MASTER_KEY_VALUE,
		BIT(11), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE_WITH_KEY(iommu1_mbus_gate_clk, "iommu1-mbus-gate",
		"dcxo", 0x05E0,
		MBUS_MASTER_KEY_VALUE,
		BIT(1), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE_WITH_KEY(iommu0_mbus_gate_clk, "iommu0-mbus-gate",
		"dcxo", 0x05E0,
		MBUS_MASTER_KEY_VALUE,
		BIT(0), CLK_IGNORE_UNUSED);

#define MBUS_GATE_KEY_VALUE	0x40302
static SUNXI_CCU_GATE_WITH_KEY(ve_dec_mbus_clk, "ve-dec-mclk",
		"dcxo", 0x05E4,
		MBUS_GATE_KEY_VALUE,
		BIT(18), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE_WITH_KEY(gmac1_mbus_clk, "gmac1-mclk",
		"dcxo", 0x05E4,
		MBUS_GATE_KEY_VALUE,
		BIT(12), 0);

static SUNXI_CCU_GATE_WITH_KEY(gmac0_mbus_clk, "gmac0-mclk",
		"dcxo", 0x05E4,
		MBUS_GATE_KEY_VALUE,
		BIT(11), 0);

static SUNXI_CCU_GATE_WITH_KEY(isp_mbus_clk, "isp-mclk",
		"dcxo", 0x05E4,
		MBUS_GATE_KEY_VALUE,
		BIT(9), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE_WITH_KEY(csi_mbus_clk, "csi-mclk",
		"dcxo", 0x05E4,
		MBUS_GATE_KEY_VALUE,
		BIT(8), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE_WITH_KEY(nand_mbus_clk, "nand-mclk",
		"dcxo", 0x05E4,
		MBUS_GATE_KEY_VALUE,
		BIT(5), 0);

static SUNXI_CCU_GATE_WITH_KEY(dma1_mbus_clk, "dma1-mclk",
		"dcxo", 0x05E4,
		MBUS_GATE_KEY_VALUE,
		BIT(3), 0);

static SUNXI_CCU_GATE_WITH_KEY(ce_mbus_clk, "ce-mclk",
		"dcxo", 0x05E4,
		MBUS_GATE_KEY_VALUE,
		BIT(2), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE_WITH_KEY(ve_mbus_clk, "ve-mclk",
		"dcxo", 0x05E4,
		MBUS_GATE_KEY_VALUE,
		BIT(1), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE_WITH_KEY(dma0_mbus_clk, "dma0-mclk",
		"dcxo", 0x05E4,
		MBUS_GATE_KEY_VALUE,
		BIT(0), 0);

static SUNXI_CCU_GATE(dma0_clk, "dma0",
		"dcxo",
		0x0704, BIT(0), 0);

static SUNXI_CCU_GATE(dma1_clk, "dma1",
		"dcxo",
		0x070C, BIT(0), 0);

static SUNXI_CCU_GATE(spinlock_clk, "spinlock",
		"dcxo",
		0x0724, BIT(0), 0);

static SUNXI_CCU_GATE(msgbox0_clk, "msgbox0",
		"dcxo",
		0x0744, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(pwm0_clk, "pwm0",
		"dcxo",
		0x0784, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(pwm1_clk, "pwm1",
		"dcxo",
		0x078C, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(dbgsys_clk, "dbgsys",
		"dcxo",
		0x07A4, BIT(0), 0);

static SUNXI_CCU_GATE(sysdap_clk, "sysdap",
		"dcxo",
		0x07AC, BIT(0), 0);

static const char * const timer_clk_parents[] = { "sys24M", "iosc", "osc32k", "pll-peri0-200m", "dcxo" };

static struct ccu_div timer0_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x0800,
		.hw.init	= CLK_HW_INIT_PARENTS("timer0",
				timer_clk_parents,
				&ccu_div_ops, CLK_SET_RATE_PARENT),
	},
};

static struct ccu_div timer1_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x0804,
		.hw.init	= CLK_HW_INIT_PARENTS("timer1",
				timer_clk_parents,
				&ccu_div_ops, CLK_SET_RATE_PARENT),
	},
};

static struct ccu_div timer2_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x0808,
		.hw.init	= CLK_HW_INIT_PARENTS("timer2",
				timer_clk_parents,
				&ccu_div_ops, CLK_SET_RATE_PARENT),
	},
};

static struct ccu_div timer3_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x080C,
		.hw.init	= CLK_HW_INIT_PARENTS("timer3",
				timer_clk_parents,
				&ccu_div_ops, CLK_SET_RATE_PARENT),
	},
};

static struct ccu_div timer4_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x0810,
		.hw.init	= CLK_HW_INIT_PARENTS("timer4",
				timer_clk_parents,
				&ccu_div_ops, CLK_SET_RATE_PARENT),
	},
};

static struct ccu_div timer5_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x0814,
		.hw.init	= CLK_HW_INIT_PARENTS("timer5",
				timer_clk_parents,
				&ccu_div_ops, CLK_SET_RATE_PARENT),
	},
};

static struct ccu_div timer6_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x0818,
		.hw.init	= CLK_HW_INIT_PARENTS("timer6",
				timer_clk_parents,
				&ccu_div_ops, CLK_SET_RATE_PARENT),
	},
};

static struct ccu_div timer7_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x081C,
		.hw.init	= CLK_HW_INIT_PARENTS("timer7",
				timer_clk_parents,
				&ccu_div_ops, CLK_SET_RATE_PARENT),
	},
};

static struct ccu_div timer8_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x0820,
		.hw.init	= CLK_HW_INIT_PARENTS("timer8",
				timer_clk_parents,
				&ccu_div_ops, CLK_SET_RATE_PARENT),
	},
};

static struct ccu_div timer9_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x0824,
		.hw.init	= CLK_HW_INIT_PARENTS("timer9",
				timer_clk_parents,
				&ccu_div_ops, CLK_SET_RATE_PARENT),
	},
};

static SUNXI_CCU_GATE(timer_bus_clk, "timer-bus",
		"dcxo",
		0x0850, BIT(0), 0);

static const char * const avs_parents[] = { "sys24M", "dcxo" };

static SUNXI_CCU_MUX_WITH_GATE(avs_clk, "avs",
		avs_parents, 0x0880,
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const de0_parents[] = { "pll-de-3x", "pll-de-4x", "pll-peri0-480m", "pll-peri0-400m", "pll-peri0-300m", "pll-video0-4x", "pll-video2-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(de0_clk, "de0",
		de0_parents, 0x0A00,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(de0_gate_clk, "de0-gate",
		"dcxo",
		0x0A04, BIT(0), CLK_IGNORE_UNUSED);

static const char * const di_parents[] = { "pll-peri0-600m", "pll-peri0-480m", "pll-peri0-400m", "pll-video0-4x", "pll-video2-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(di_clk, "di",
		di_parents, 0x0A20,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(di_gate_clk, "di-gate",
		"dcxo",
		0x0A24, BIT(0), 0);

static const char * const g2d_parents[] = { "pll-peri0-300m", "pll-peri0-400m", "pll-video0-4x", "pll-video2-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(g2d_clk, "g2d",
		g2d_parents, 0x0A40,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(g2d_gate_clk, "g2d-gate",
		"dcxo",
		0x0A44, BIT(0), 0);

static const char * const eink_parents[] = { "pll-peri0-480m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE(eink_clk, "eink",
		eink_parents, 0x0A60,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const eink_panel_parents[] = { "pll-video0-4x", "pll-video0-3x", "pll-video1-4x", "pll-video1-3x", "pll-peri0-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(eink_panel_clk, "eink-panel",
		eink_panel_parents, 0x0A64,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(eink_gate_clk, "eink-gate",
		"dcxo",
		0x0A6C, BIT(0), 0);

static const char * const ve_enc0_parents[] = { "pll-ve0", "pll-ve1", "pll-peri0-800m", "pll-peri0-600m", "pll-peri0-480m", "pll-de-3x", "pll-npu" };

static SUNXI_CCU_M_WITH_MUX_GATE(ve_enc0_clk, "ve-enc0",
		ve_enc0_parents, 0x0A80,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT | CLK_SET_RATE_NO_REPARENT);

static const char * const ve_dec_parents[] = { "pll-ve1", "pll-ve0", "pll-peri0-800m", "pll-peri0-600m", "pll-peri0-480m", "pll-de-3x", "pll-npu" };

static SUNXI_CCU_M_WITH_MUX_GATE(ve_dec_clk, "ve-dec",
		ve_dec_parents, 0x0A88,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT | CLK_SET_RATE_NO_REPARENT);

static SUNXI_CCU_GATE(ve_dec_bus_clk, "ve-dec-gate",
		"dcxo",
		0x0A8C, BIT(2), 0);

static SUNXI_CCU_GATE(ve_enc_bus_clk, "ve-enc0-bus",
		"dcxo",
		0x0A8C, BIT(0), 0);

static const char * const ce_parents[] = { "sys24M", "pll-peri0-400m", "pll-peri0-600m" };

static SUNXI_CCU_M_WITH_MUX_GATE(ce_clk, "ce",
		ce_parents, 0x0AC0,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(ce_sys_clk, "ce-sys",
		"dcxo",
		0x0AC4, BIT(1), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(ce_bus_clk, "ce-bus",
		"dcxo",
		0x0AC4, BIT(0), CLK_IGNORE_UNUSED);

static const char * const npu_parents[] = { "pll-npu", "pll-peri0-800m", "pll-peri0-600m", "pll-peri0-480m", "pll-ve0", "pll-ve1", "pll-de-3x" };

static SUNXI_CCU_M_WITH_MUX_GATE(npu_clk, "npu",
		npu_parents, 0x0B00,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(npu_bus_clk, "npu-gate",
		"dcxo",
		0x0B04, BIT(0), 0);

static const char * const gpu0_parents[] = { "pll-gpu", "pll-peri0-800m", "pll-peri0-600m", "pll-peri0-400m", "pll-peri0-300m", "pll-peri0-200m" };

static struct clk_div_table gpu_div_table[] = {
    { .val = 0, .div = 1 },
    { .val = 8, .div = 2 },
    { .val = 12, .div = 4 },
    { /* Sentinel */ },
};

static struct ccu_div gpu0_clk = {
	.enable         = BIT(31),
	.div            = _SUNXI_CCU_DIV_TABLE(0, 4, gpu_div_table),
	.mux            = _SUNXI_CCU_MUX(24, 3),
	.common         = {
		.reg            = 0x0B20,
		.key_value	= UPD_KEY_VALUE,
		.key_reg	= 0x0B20,
		.features	= CCU_FEATURE_KEY_FIELD_MOD,
		.hw.init        = CLK_HW_INIT_PARENTS("gpu0", gpu0_parents,
				&ccu_div_ops,
				CLK_OPS_PARENT_ENABLE | CLK_SET_RATE_NO_REPARENT),
	},
};

static SUNXI_CCU_GATE(gpu0_bus_clk, "gpu0-gate",
		"dcxo",
		0x0B24, BIT(0), 0);

static const char * const dram0_parents[] = { "pll-ddr", "pll-peri1-800m", "pll-peri1-600m", "pll-de-3x", "pll-npu"};
static SUNXI_CCU_M_WITH_MUX_GATE_KEY(dram0_clk, "dram0",
		dram0_parents, 0x0C00,
		0, 5,
		24, 3,	/* mux */
		UPD_KEY_VALUE,
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED | CLK_GET_RATE_NOCACHE);

static SUNXI_CCU_GATE(dram0_bus_clk, "dram0-gate",
		"dcxo",
		0x0C0C, BIT(0), CLK_IGNORE_UNUSED);

static const char * const nand0_clk_parents[] = { "sys24M", "pll-peri0-400m", "pll-peri0-300m", "pll-peri1-400m", "pll-peri1-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(nand0_clk0_clk, "nand0-clk0",
		nand0_clk_parents, 0x0C80,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);


static SUNXI_CCU_M_WITH_MUX_GATE(nand0_clk1_clk, "nand0-clk1",
		nand0_clk_parents, 0x0C84,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(nand0_bus_clk, "nand0-bus",
		"dcxo",
		0x0C8C, BIT(0), 0);

static const char * const smhc_parents[] = { "sys24M", "pll-peri0-400m", "pll-peri0-300m", "pll-peri1-400m", "pll-peri1-300m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc0_clk, "smhc0",
		smhc_parents, 0x0D00,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), CLK_SET_RATE_NO_REPARENT);

static SUNXI_CCU_GATE(smhc0_gate_clk, "smhc0-gate",
		"dcxo",
		0x0D0C, BIT(0), 0);


static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc1_clk, "smhc1",
		smhc_parents, 0x0D10,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), CLK_SET_RATE_NO_REPARENT);

static SUNXI_CCU_GATE(smhc1_gate_clk, "smhc1-gate",
		"dcxo",
		0x0D1C, BIT(0), 0);

static const char * const smhc2_parents[] = { "sys24M", "pll-peri0-800m", "pll-peri0-600m", "pll-peri1-800m", "pll-peri1-600m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc2_clk, "smhc2",
		smhc2_parents, 0x0D20,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), CLK_SET_RATE_NO_REPARENT);

static SUNXI_CCU_GATE(smhc2_gate_clk, "smhc2-gate",
		"dcxo",
		0x0D2C, BIT(0), 0);

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc3_clk, "smhc3",
		smhc2_parents, 0x0D30,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), CLK_SET_RATE_NO_REPARENT);

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

static const char * const ufs_cfg_parents[] = { "pll-peri0-480m", "dcxo" };

static SUNXI_CCU_M_WITH_MUX_GATE(ufs_cfg_clk, "ufs-cfg",
		ufs_cfg_parents, 0x0D84,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(ufs_clk, "ufs",
		"dcxo",
		0x0D8C, BIT(0), 0);

static SUNXI_CCU_GATE(uart0_clk, "uart0",
		"apb-uart",
		0x0E00, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(uart1_clk, "uart1",
		"apb-uart",
		0x0E04, BIT(0), 0);

static SUNXI_CCU_GATE(uart2_clk, "uart2",
		"apb-uart",
		0x0E08, BIT(0), 0);

static SUNXI_CCU_GATE(uart3_clk, "uart3",
		"apb-uart",
		0x0E0C, BIT(0), 0);

static SUNXI_CCU_GATE(uart4_clk, "uart4",
		"apb-uart",
		0x0E10, BIT(0), 0);

static SUNXI_CCU_GATE(uart5_clk, "uart5",
		"apb-uart",
		0x0E14, BIT(0), 0);

static SUNXI_CCU_GATE(uart6_clk, "uart6",
		"apb-uart",
		0x0E18, BIT(0), 0);

static SUNXI_CCU_GATE(twi0_clk, "twi0",
		"apb1",
		0x0E80, BIT(0), 0);

static SUNXI_CCU_GATE(twi1_clk, "twi1",
		"apb1",
		0x0E84, BIT(0), 0);

static SUNXI_CCU_GATE(twi2_clk, "twi2",
		"apb1",
		0x0E88, BIT(0), 0);

static SUNXI_CCU_GATE(twi3_clk, "twi3",
		"apb1",
		0x0E8C, BIT(0), 0);

static SUNXI_CCU_GATE(twi4_clk, "twi4",
		"apb1",
		0x0E90, BIT(0), 0);

static SUNXI_CCU_GATE(twi5_clk, "twi5",
		"apb1",
		0x0E94, BIT(0), 0);

static SUNXI_CCU_GATE(twi6_clk, "twi6",
		"apb1",
		0x0E98, BIT(0), 0);

static SUNXI_CCU_GATE(twi7_clk, "twi7",
		"apb1",
		0x0E9C, BIT(0), 0);

static SUNXI_CCU_GATE(twi8_clk, "twi8",
		"apb1",
		0x0EA0, BIT(0), 0);

static SUNXI_CCU_GATE(twi9_clk, "twi9",
		"apb1",
		0x0EA4, BIT(0), 0);

static SUNXI_CCU_GATE(twi10_clk, "twi10",
		"apb1",
		0x0EA8, BIT(0), 0);

static SUNXI_CCU_GATE(twi11_clk, "twi11",
		"apb1",
		0x0EAC, BIT(0), 0);

static SUNXI_CCU_GATE(twi12_clk, "twi12",
		"apb1",
		0x0EB0, BIT(0), 0);

static const char * const spi_parents[] = { "sys24M", "pll-peri0-300m", "pll-peri0-200m", "pll-peri1-300m", "pll-peri1-200m", "pll-peri0-480m", "pll-peri1-480m", "dcxo" };

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

static const char * const spif_parents[] = { "sys24M", "pll-peri0-400m", "pll-peri0-300m", "pll-peri1-400m", "pll-peri1-300m", "pll-peri0-160m", "pll-peri1-160m", "dcxo" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spif_clk, "spif",
		spif_parents, 0x0F18,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(spif_bus_clk, "spif-bus",
		"dcxo",
		0x0F1C, BIT(0), 0);

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spi3_clk, "spi3",
		spi_parents, 0x0F20,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(spi3_bus_clk, "spi3-bus",
		"dcxo",
		0x0F24, BIT(0), 0);

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spi4_clk, "spi4",
		spi_parents, 0x0F28,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(spi4_bus_clk, "spi4-bus",
		"dcxo",
		0x0F2C, BIT(0), 0);

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

static SUNXI_CCU_GATE(ths0_clk, "ths0",
		"dcxo",
		0x0FE4, BIT(0), 0);

static const char * const irrx_parents[] = { "osc32k", "sys24M", "dcxo" };

static SUNXI_CCU_M_WITH_MUX_GATE(irrx_clk, "irrx",
		irrx_parents, 0x1000,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(irrx_gate_clk, "irrx-gate",
		"dcxo",
		0x1004, BIT(0), 0);

static const char * const irtx_parents[] = { "sys24M", "pll-peri1-600m", "dcxo" };

static SUNXI_CCU_M_WITH_MUX_GATE(irtx_clk, "irtx",
		irtx_parents, 0x1008,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(irtx_gate_clk, "irtx-gate",
		"dcxo",
		0x100C, BIT(0), 0);

static SUNXI_CCU_GATE(lradc_clk, "lradc",
		"dcxo",
		0x1024, BIT(0), 0);

static const char * const sgpio_parents[] = { "sys24M", "osc32k" };

static SUNXI_CCU_M_WITH_MUX_GATE(sgpio_clk, "sgpio",
		sgpio_parents, 0x1060,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(sgpio_bus_clk, "sgpio-bus",
		"dcxo",
		0x1064, BIT(0), 0);

static const char * const lpc_parents[] = { "pll-video0-3x", "pll-video1-3x", "pll-video2-3x", "pll-peri0-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(lpc_clk, "lpc",
		lpc_parents, 0x1080,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(lpc_bus_clk, "lpc-gate",
		"dcxo",
		0x1084, BIT(0), 0);

static const char * const i2spcm_parents[] = { "pll-audio0-4x", "pll-audio1-div2", "pll-audio1-div5", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(i2spcm0_clk, "i2spcm0",
		i2spcm_parents, 0x1200,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(i2spcm0_bus_clk, "i2spcm0-bus",
		"dcxo",
		0x120C, BIT(0), 0);

static SUNXI_CCU_M_WITH_MUX_GATE(i2spcm1_clk, "i2spcm1",
		i2spcm_parents, 0x1210,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(i2spcm1_bus_clk, "i2spcm1-bus",
		"dcxo",
		0x121C, BIT(0), 0);

static SUNXI_CCU_M_WITH_MUX_GATE(i2spcm2_clk, "i2spcm2",
		i2spcm_parents, 0x1220,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static const char * const i2spcm2_asrc_parents[] = { "pll-audio0-4x", "pll-audio1-div2", "pll-audio1-div5", "pll-peri0-300m", "pll-peri1-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(i2spcm2_asrc_clk, "i2spcm2-asrc",
		i2spcm2_asrc_parents, 0x1224,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(i2spcm2_bus_clk, "i2spcm2-bus",
		"dcxo",
		0x122C, BIT(0), 0);

static const char * const i2spcm3_parents[] = { "pll-audio0-4x", "pll-audio1-div2", "pll-audio1-div5", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(i2spcm3_clk, "i2spcm3",
		i2spcm3_parents, 0x1230,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(i2spcm3_bus_clk, "i2spcm3-bus",
		"dcxo",
		0x123C, BIT(0), 0);

static SUNXI_CCU_M_WITH_MUX_GATE(i2spcm4_clk, "i2spcm4",
		i2spcm3_parents, 0x1240,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(i2spcm4_bus_clk, "i2spcm4-bus",
		"dcxo",
		0x124C, BIT(0), 0);

static const char * const owa_tx_parents[] = { "pll-audio0-4x", "pll-audio1-div2", "pll-audio1-div5" };

static SUNXI_CCU_M_WITH_MUX_GATE(owa_tx_clk, "owa-tx",
		owa_tx_parents, 0x1280,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static const char * const owa_rx_parents[] = { "pll-peri0-200m", "pll-peri0-300m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE(owa_rx_clk, "owa-rx",
		owa_rx_parents, 0x1284,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(owa_bus_clk, "owa-bus",
		"dcxo",
		0x128C, BIT(0), 0);

static const char * const dmic_parents[] = { "pll-audio0-4x", "pll-audio1-div2", "pll-audio1-div5" };

static SUNXI_CCU_M_WITH_MUX_GATE(dmic_clk, "dmic",
		dmic_parents, 0x12C0,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(dmic_bus_clk, "dmic-bus",
		"dcxo",
		0x12CC, BIT(0), 0);

static SUNXI_CCU_GATE(usb_clk, "usb",
		"dcxo",
		0x1300, BIT(31), 0);

static SUNXI_CCU_GATE(usb0_device_clk, "usb0-device",
		"dcxo",
		0x1304, BIT(8), 0);

static SUNXI_CCU_GATE(usb0_ehci_clk, "usb0-ehci",
		"dcxo",
		0x1304, BIT(4), 0);

static SUNXI_CCU_GATE(usb0_ohci_clk, "usb0-ohci",
		"dcxo",
		0x1304, BIT(0), 0);

static SUNXI_CCU_GATE(usb1_clk, "usb1",
		"dcxo",
		0x1308, BIT(31), 0);

static SUNXI_CCU_GATE(usb1_ehci_clk, "usb1-ehci",
		"dcxo",
		0x130C, BIT(4), 0);

static SUNXI_CCU_GATE(usb1_ohci_clk, "usb1-ohci",
		"dcxo",
		0x130C, BIT(0), 0);

static const char * const usb_ref_parents[] = { "sys24M", "dcxo" };

static SUNXI_CCU_MUX_WITH_GATE(usb_ref_clk, "usb-ref",
		usb_ref_parents, 0x1340,
		24, 3,	/* mux */
		BIT(31), 0);


static SUNXI_CCU_MUX_WITH_GATE(usb2_u2_ref_clk, "usb2-u2-ref",
		usb_ref_parents, 0x1348,
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const usb2_suspend_parents[] = { "osc32k", "sys24M" };

static SUNXI_CCU_M_WITH_MUX_GATE(usb2_suspend_clk, "usb2-suspend",
		usb2_suspend_parents, 0x1350,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31), /* gate */
		0);

static const char * const usb2_mf_parents[] = { "sys24M", "pll-peri0-300m", "dcxo" };

static SUNXI_CCU_M_WITH_MUX_GATE(usb2_mf_clk, "usb2-mf",
		usb2_mf_parents, 0x1354,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31), /* gate */
		0);

static const char * const usb2_u3_utmi_parents[] = { "sys24M", "pll-peri0-300m", "dcxo" };

static SUNXI_CCU_M_WITH_MUX_GATE(usb2_u3_utmi_clk, "usb2-u3-utmi",
		usb2_u3_utmi_parents, 0x1360,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31), /* gate */
		0);

static const char * const usb2_u2_pipe_parents[] = { "sys24M", "pll-peri0-480m", "dcxo" };

static SUNXI_CCU_M_WITH_MUX_GATE(usb2_u2_pipe_clk, "usb2-u2-pipe",
		usb2_u2_pipe_parents, 0x1364,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31), /* gate */
		0);

static const char * const pcie0_aux_parents[] = { "sys24M", "osc32k" };

static SUNXI_CCU_M_WITH_MUX_GATE(pcie0_aux_clk, "pcie0-aux",
		pcie0_aux_parents, 0x1380,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		0);

static const char * const pcie0_axi_slv_parents[] = { "pll-peri0-600m", "pll-peri0-600m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE(pcie0_axi_slv_clk, "pcie0-axi-slv",
		pcie0_axi_slv_parents, 0x1384,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const serdes_phy_cfg_parents[] = { "sys24M", "pll-peri0-600m" };

static SUNXI_CCU_M_WITH_MUX_GATE(serdes_phy_cfg_clk, "serdes-phy-cfg",
		serdes_phy_cfg_parents, 0x13C0,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const gmac_ptp_parents[] = { "sys24M", "pll-peri0-200m", "dcxo" };

static SUNXI_CCU_M_WITH_MUX_GATE(gmac_ptp_clk, "gmac-ptp",
		gmac_ptp_parents, 0x1400,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);


static SUNXI_CCU_M_WITH_GATE(gmac0_phy_clk, "gmac0-phy", "pll-peri0-150m",
			     0x1410, 0, 5, BIT(31), 0);

static SUNXI_CCU_GATE(gmac0_clk, "gmac0",
		"dcxo",
		0x141C, BIT(0), 0);

static SUNXI_CCU_M_WITH_GATE(gmac1_phy_clk, "gmac1-phy", "pll-peri0-150m",
			     0x1420, 0, 5, BIT(31), 0);

static SUNXI_CCU_GATE(gmac1_clk, "gmac1",
		"dcxo",
		0x142C, BIT(0), 0);

static const char * const vo0_tconlcd_parents[] = { "pll-video0-4x", "pll-video1-4x", "pll-video2-4x", "pll-peri0-2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(vo0_tconlcd0_clk, "vo0-tconlcd0",
		vo0_tconlcd_parents, 0x1500,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(vo0_tconlcd0_bus_clk, "vo0-tconlcd0-bus",
		"dcxo",
		0x1504, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_M_WITH_MUX_GATE(vo0_tconlcd1_clk, "vo0-tconlcd1",
		vo0_tconlcd_parents, 0x1508,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(vo0_tconlcd1_bus_clk, "vo0-tconlcd1-bus",
		"dcxo",
		0x150C, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_M_WITH_MUX_GATE(vo0_tconlcd2_clk, "vo0-tconlcd2",
		vo0_tconlcd_parents, 0x1510,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(vo0_tconlcd2_bus_clk, "vo0-tconlcd2-bus",
		"dcxo",
		0x1514, BIT(0), CLK_IGNORE_UNUSED);

static const char * const dsi_parents[] = { "sys24M", "pll-peri0-200m", "pll-peri0-150m" };

static SUNXI_CCU_M_WITH_MUX_GATE(dsi0_clk, "dsi0",
		dsi_parents, 0x1580,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(dsi0_bus_clk, "dsi0-bus",
		"dcxo",
		0x1584, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_M_WITH_MUX_GATE(dsi1_clk, "dsi1",
		dsi_parents, 0x1588,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(dsi1_bus_clk, "dsi1-bus",
		"dcxo",
		0x158C, BIT(0), CLK_IGNORE_UNUSED);

static const char * const combphy_parents[] = { "pll-video0-4x", "pll-video1-4x", "pll-video2-4x", "pll-peri0-2x", "pll-video0-3x" };

static SUNXI_CCU_M_WITH_MUX_GATE(combphy0_clk, "combphy0",
		combphy_parents, 0x15C0,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

static SUNXI_CCU_M_WITH_MUX_GATE(combphy1_clk, "combphy1",
		combphy_parents, 0x15C4,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(tcontv0_clk, "tcontv0",
		"dcxo",
		0x1604, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(tcontv1_clk, "tcontv1",
		"dcxo",
		0x160C, BIT(0), CLK_IGNORE_UNUSED);

static const char * const edp_tv_parents[] = { "pll-video0-4x", "pll-video1-4x", "pll-video2-4x", "pll-peri0-2x" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(edp_tv_clk, "edp-tv",
		edp_tv_parents, 0x1640,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31),
		CLK_SET_RATE_PARENT | CLK_SET_RATE_NO_REPARENT | CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(edp_clk, "edp",
		"dcxo",
		0x164C, BIT(0), CLK_IGNORE_UNUSED);

static const char * const hdmi_ref_parents[] = { "osc32k", "hdmi-cec-clk32k" };

static SUNXI_CCU_MUX_WITH_GATE(hdmi_ref_clk, "hdmi-ref",
		hdmi_ref_parents, 0x1680,
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const hdmi_tv_parents[] = { "pll-video0-4x", "pll-video1-4x", "pll-video2-4x", "pll-peri0-2x" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(hdmi_tv_clk, "hdmi-tv",
		hdmi_tv_parents, 0x1684,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(hdmi_clk, "hdmi",
		"dcxo",
		0x168C, BIT(0), 0);

static const char * const hdmi_sfr_parents[] = { "sys24M", "dcxo" };

static SUNXI_CCU_MUX_WITH_GATE(hdmi_sfr_clk, "hdmi-sfr",
		hdmi_sfr_parents, 0x1690,
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(hdcp_esm_clk, "hdcp-esm",
		"pll-peri0-300m",
		0x1694, BIT(31), 0);

static SUNXI_CCU_GATE(dpss_top0_clk, "dpss-top0",
		"dcxo",
		0x16C4, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(dpss_top1_clk, "dpss-top1",
		"dcxo",
		0x16CC, BIT(0), CLK_IGNORE_UNUSED);

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

static SUNXI_CCU_GATE(dsc_clk, "dsc",
		"dcxo",
		0x1744, BIT(0), 0);

static const char * const csi_master_parents[] = { "sys24M", "pll-video0-4x", "pll-video0-3x", "pll-video1-4x", "pll-video1-3x", "pll-video2-4x", "pll-video2-3x" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(csi_master0_clk, "csi-master0",
		csi_master_parents, 0x1800,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31),
		CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(csi_master1_clk, "csi-master1",
		csi_master_parents, 0x1804,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31),
		CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(csi_master2_clk, "csi-master2",
		csi_master_parents, 0x1808,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31),
		CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

static const char * const csi_parents[] = { "pll-video2-4x", "pll-de-4x", "pll-peri0-480m", "pll-peri0-400m", "pll-peri0-600m", "pll-video0-4x", "pll-video1-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(csi_clk, "csi",
		csi_parents, 0x1840,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT | CLK_SET_RATE_NO_REPARENT | CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(csi_bus_clk, "csi-bus",
		"dcxo",
		0x1844, BIT(0), 0);

static const char * const isp_parents[] = { "pll-video2-4x", "pll-peri0-480m", "pll-peri0-400m", "pll-peri0-600m", "pll-video0-4x", "pll-video1-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(isp_clk, "isp",
		isp_parents, 0x1860,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT | CLK_SET_RATE_NO_REPARENT | CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(res_dcap_24m_clk, "res-dcap-24m",
		"dcxo",
		0x1A00, BIT(3), 0);

static const char * const apb2jtag_parents[] = { "sys24M", "osc32k", "iosc", "pll-peri0-480m", "pll-peri1-480m", "pll-peri0-200m", "pll-peri1-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(apb2jtag_clk, "apb2jtag",
		apb2jtag_parents, 0x1C00,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(fanout_25m_clk, "fanout-25m",
		"dcxo",
		0x1F30, BIT(3), 0);

static SUNXI_CCU_GATE(fanout_16m_clk, "fanout-16m",
		"dcxo",
		0x1F30, BIT(2), 0);

static SUNXI_CCU_GATE(fanout_12m_clk, "fanout-12m",
		"dcxo",
		0x1F30, BIT(1), 0);

static SUNXI_CCU_GATE(fanout_24m_clk, "fanout-24m",
		"dcxo",
		0x1F30, BIT(0), 0);

static const char * const clk27m_fanout_parents[] = { "pll-video0-4x", "pll-video1-4x", "pll-video2-4x" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(clk27m_fanout_clk, "clk27m_fanout",
		clk27m_fanout_parents, 0x1F34,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 2,	/* mux */
		BIT(31), 0);

static const char * const clk_fanout_parents[] = { "apb0" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(clk_fanout_clk, "clk-fanout",
		clk_fanout_parents, 0x1F38,
		0, 5,	/* M */
		5, 5,	/* N */
		24, 2,	/* mux */
		BIT(31), 0);

static CLK_FIXED_FACTOR(sys_12M_clk, "sys12M", "sys24M", 2, 1, 0);
static CLK_FIXED_FACTOR(pll_peri0_16m_clk, "pll-peri0-16m", "pll-peri0-160m", 10, 1, 0);
static CLK_FIXED_FACTOR(pll_peri0_25m_clk, "pll-peri0-25m", "pll-peri0-150m", 6, 1, 0);

static const char * const fanout_clk_parents[] = { "rtc32k", "sys12M", "pll-peri0-16m", "sys24M", "pll-peri0-25m", "clk27m-fanout", "clk-fanout" };

static SUNXI_CCU_MUX_WITH_GATE(fanout3_clk, "fanout3",
		fanout_clk_parents, 0x1F3C,
		9, 3,	/* mux */
		BIT(24), CLK_SET_RATE_PARENT);

static SUNXI_CCU_MUX_WITH_GATE(fanout2_clk, "fanout2",
		fanout_clk_parents, 0x1F3C,
		6, 3,	/* mux */
		BIT(23), CLK_SET_RATE_PARENT);

static SUNXI_CCU_MUX_WITH_GATE(fanout1_clk, "fanout1",
		fanout_clk_parents, 0x1F3C,
		3, 3,	/* mux */
		BIT(22), CLK_SET_RATE_PARENT);

static SUNXI_CCU_MUX_WITH_GATE(fanout0_clk, "fanout0",
		fanout_clk_parents, 0x1F3C,
		0, 3,	/* mux */
		BIT(21), CLK_SET_RATE_PARENT);

static const char * const bus_debug_parents[] = { "ahb", "apb0", "apb1", "apb-uart", "mbus", "nsi", "ddr0", "pll-ddr", "dram0" };

static SUNXI_CCU_MUX(bus_debug_clk, "bus_debug",
		 bus_debug_parents, 0x1F50,
		0, 2,	/* mux */
		0);

static SUNXI_CCU_GATE(pll_ddr_auto_clk, "pll-ddr-auto",
		 "dcxo", 0x1904,
		BIT(0), 0);

static SUNXI_CCU_GATE(pll_peri0_2x_auto_clk, "pll-peri0-2x-auto",
		 "dcxo", 0x1908,
		BIT(11), 0);

static SUNXI_CCU_GATE(pll_peri0_800m_auto_clk, "pll-peri0-800m-auto",
		 "dcxo", 0x1908,
		BIT(10), 0);

static SUNXI_CCU_GATE(pll_peri0_600m_auto_clk, "pll-peri0-600m-auto",
		 "dcxo", 0x1908,
		BIT(9), 0);

static SUNXI_CCU_GATE(pll_peri0_480m_all_auto_clk, "pll-peri0-480m-all-auto",
		 "dcxo", 0x1908,
		BIT(8), 0);

static SUNXI_CCU_GATE(pll_peri0_480m_auto_clk, "pll-peri0-480m-auto",
		 "dcxo", 0x1908,
		BIT(7), 0);

static SUNXI_CCU_GATE(pll_peri0_160m_auto_clk, "pll-peri0-160m-auto",
		 "dcxo", 0x1908,
		BIT(6), 0);

static SUNXI_CCU_GATE(pll_peri0_300m_all_auto_clk, "pll-peri0-300m-all-auto",
		 "dcxo", 0x1908,
		BIT(5), 0);

static SUNXI_CCU_GATE(pll_peri0_300m_auto_clk, "pll-peri0-300m-auto",
		 "dcxo", 0x1908,
		BIT(4), 0);

static SUNXI_CCU_GATE(pll_peri0_150m_auto_clk, "pll-peri0-150m-auto",
		 "dcxo", 0x1908,
		BIT(3), 0);

static SUNXI_CCU_GATE(pll_peri0_400m_all_auto_clk, "pll-peri0-400m-all-auto",
		 "dcxo", 0x1908,
		BIT(2), 0);

static SUNXI_CCU_GATE(pll_peri0_400m_auto_clk, "pll-peri0-400m-auto",
		 "dcxo", 0x1908,
		BIT(1), 0);

static SUNXI_CCU_GATE(pll_peri0_200m_auto_clk, "pll-peri0-200m-auto",
		 "dcxo", 0x1908,
		BIT(0), 0);

static SUNXI_CCU_GATE(pll_peri1_800m_auto_clk, "pll-peri1-800m-auto",
		 "dcxo", 0x190c,
		BIT(11), 0);

static SUNXI_CCU_GATE(pll_peri1_600m_all_auto_clk, "pll-peri1-600m-all-auto",
		 "dcxo", 0x190c,
		BIT(10), 0);

static SUNXI_CCU_GATE(pll_peri1_600m_auto_clk, "pll-peri1-600m-auto",
		 "dcxo", 0x190c,
		BIT(9), 0);

static SUNXI_CCU_GATE(pll_peri1_480m_all_auto_clk, "pll-peri1-480m-all-auto",
		 "dcxo", 0x190c,
		BIT(8), 0);

static SUNXI_CCU_GATE(pll_peri1_480m_auto_clk, "pll-peri1-480m-auto",
		 "dcxo", 0x190c,
		BIT(7), 0);

static SUNXI_CCU_GATE(pll_peri1_160m_auto_clk, "pll-peri1-160m-auto",
		 "dcxo", 0x190c,
		BIT(6), 0);

static SUNXI_CCU_GATE(pll_peri1_300m_all_auto_clk, "pll-peri1-300m-all-auto",
		 "dcxo", 0x190c,
		BIT(5), 0);

static SUNXI_CCU_GATE(pll_peri1_300m_auto_clk, "pll-peri1-300m-auto",
		 "dcxo", 0x190c,
		BIT(4), 0);

static SUNXI_CCU_GATE(pll_peri1_150m_auto_clk, "pll-peri1-150m-auto",
		 "dcxo", 0x190c,
		BIT(3), 0);

static SUNXI_CCU_GATE(pll_peri1_400m_all_auto_clk, "pll-peri1-400m-all-auto",
		 "dcxo", 0x190c,
		BIT(2), 0);

static SUNXI_CCU_GATE(pll_peri1_400m_auto_clk, "pll-peri1-400m-auto",
		 "dcxo", 0x190c,
		BIT(1), 0);

static SUNXI_CCU_GATE(pll_peri1_200m_auto_clk, "pll-peri1-200m-auto",
		 "dcxo", 0x190c,
		BIT(0), 0);

static SUNXI_CCU_GATE(pll_video2_3x_auto_clk, "pll-video2-3x-auto",
		 "dcxo", 0x1910,
		BIT(6), 0);

static SUNXI_CCU_GATE(pll_video1_3x_auto_clk, "pll-video1-3x-auto",
		 "dcxo", 0x1910,
		BIT(5), 0);

static SUNXI_CCU_GATE(pll_video0_3x_auto_clk, "pll-video0-3x-auto",
		 "dcxo", 0x1910,
		BIT(4), 0);

static SUNXI_CCU_GATE(pll_video2_4x_auto_clk, "pll-video2-4x-auto",
		 "dcxo", 0x1910,
		BIT(2), 0);

static SUNXI_CCU_GATE(pll_video1_4x_auto_clk, "pll-video1-4x-auto",
		 "dcxo", 0x1910,
		BIT(1), 0);

static SUNXI_CCU_GATE(pll_video0_4x_auto_clk, "pll-video0-4x-auto",
		 "dcxo", 0x1910,
		BIT(0), 0);

static SUNXI_CCU_GATE(pll_gpu0_auto_clk, "pll-gpu0-auto",
		 "dcxo", 0x1914,
		BIT(0), 0);

static SUNXI_CCU_GATE(pll_ve1_auto_clk, "pll-ve1-auto",
		 "dcxo", 0x1918,
		BIT(1), 0);

static SUNXI_CCU_GATE(pll_ve0_auto_clk, "pll-ve0-auto",
		 "dcxo", 0x1918,
		BIT(0), 0);

static SUNXI_CCU_GATE(pll_audio1_div5_auto_clk, "pll-audio1-div5-auto",
		 "dcxo", 0x191c,
		BIT(2), 0);

static SUNXI_CCU_GATE(pll_audio1_div2_auto_clk, "pll-audio1-div2-auto",
		 "dcxo", 0x191c,
		BIT(1), 0);

static SUNXI_CCU_GATE(pll_audio0_4x_auto_clk, "pll-audio1-4x-auto",
		 "dcxo", 0x191c,
		BIT(0), 0);

static SUNXI_CCU_GATE(pll_npu_auto_clk, "pll-npu-auto",
		 "dcxo", 0x1920,
		BIT(0), 0);

static SUNXI_CCU_GATE(pll_de_3x_auto_clk, "pll-de-3x-auto",
		 "dcxo", 0x1928,
		BIT(1), 0);

static SUNXI_CCU_GATE(pll_de_4x_auto_clk, "pll-de-4x-auto",
		 "dcxo", 0x1928,
		BIT(0), 0);

#if IS_ENABLED(CONFIG_PM_SLEEP)
static struct ccu_nm pll_audio0_sdm_pat0_clk = {
	.common		= {
		.reg		= 0x268,
	},
};

static struct ccu_nm pll_audio0_sdm_pat1_clk = {
	.common		= {
		.reg		= 0x26C,
	},
};

static struct ccu_nm pll_audio1_sdm_pat0_clk = {
	.common		= {
		.reg		= 0x288,
	},
};
#endif

/* ccu_des_end */

/* rst_def_start */
static struct ccu_reset_map sun60iw2_ccu_resets[] = {
	[RST_BUS_ITS_PCIE0]		= { 0x0574, BIT(16) },
	[RST_BUS_NSI]			= { 0x0580, BIT(30) },
	[RST_BUS_NSI_CFG]		= { 0x0584, BIT(16) },
	[RST_BUS_IOMMU0_SY]		= { 0x058c, BIT(16) },
	[RST_BUS_MSI_LITE0_MBU]		= { 0x0594, BIT(17) },
	[RST_BUS_MSI_LITE0_AHB]		= { 0x0594, BIT(16) },
	[RST_BUS_MSI_LITE1_MBU]		= { 0x059c, BIT(17) },
	[RST_BUS_MSI_LITE1_AHB]		= { 0x059c, BIT(16) },
	[RST_BUS_MSI_LITE2_MBU]		= { 0x05a4, BIT(17) },
	[RST_BUS_MSI_LITE2_AHB]		= { 0x05a4, BIT(16) },
	[RST_BUS_IOMMU1_SY]		= { 0x05b4, BIT(16) },
	[RST_BUS_DMA0]			= { 0x0704, BIT(16) },
	[RST_BUS_DMA1]			= { 0x070c, BIT(16) },
	[RST_BUS_SPINLOCK]		= { 0x0724, BIT(16) },
	[RST_BUS_MSGBOX0]		= { 0x0744, BIT(16) },
	[RST_BUS_PWM0]			= { 0x0784, BIT(16) },
	[RST_BUS_PWM1]			= { 0x078c, BIT(16) },
	[RST_BUS_DBGSY]			= { 0x07a4, BIT(16) },
	[RST_BUS_SYSDAP]		= { 0x07ac, BIT(16) },
	[RST_BUS_TIMER0]		= { 0x0850, BIT(16) },
	[RST_BUS_DE0]			= { 0x0a04, BIT(16) },
	[RST_BUS_DI]			= { 0x0a24, BIT(16) },
	[RST_BUS_G2D]			= { 0x0a44, BIT(16) },
	[RST_BUS_EINK]			= { 0x0a6c, BIT(16) },
	[RST_BUS_DE_SY]			= { 0x0a74, BIT(16) },
	[RST_BUS_VE_DEC]		= { 0x0a8c, BIT(18) },
	[RST_BUS_VE_ENC0]		= { 0x0a8c, BIT(16) },
	[RST_BUS_CE_SY]			= { 0x0ac4, BIT(17) },
	[RST_BUS_CE]			= { 0x0ac4, BIT(16) },
	[RST_BUS_NPU_AHB]		= { 0x0b04, BIT(18) },
	[RST_BUS_NPU_AXI]		= { 0x0b04, BIT(17) },
	[RST_BUS_NPU_CORE]		= { 0x0b04, BIT(16) },
	[RST_BUS_GPU0]			= { 0x0b24, BIT(16) },
	[RST_BUS_DRAM0]			= { 0x0c0c, BIT(16) },
	[RST_BUS_NAND0]			= { 0x0c8c, BIT(16) },
	[RST_BUS_SMHC0]			= { 0x0d0c, BIT(16) },
	[RST_BUS_SMHC1]			= { 0x0d1c, BIT(16) },
	[RST_BUS_SMHC2]			= { 0x0d2c, BIT(16) },
	[RST_BUS_SMHC3]			= { 0x0d3c, BIT(16) },
	[RST_BUS_UFS_CORE]	    = { 0x0d8c, BIT(19) },
	[RST_BUS_UFS_PHY]	    = { 0x0d8c, BIT(18) },
	[RST_BUS_UFS_AXI]		= { 0x0d8c, BIT(17) },
	[RST_BUS_UFS_AHB]	    = { 0x0d8c, BIT(16) },
	[RST_BUS_UART0]			= { 0x0e00, BIT(16) },
	[RST_BUS_UART1]			= { 0x0e04, BIT(16) },
	[RST_BUS_UART2]			= { 0x0e08, BIT(16) },
	[RST_BUS_UART3]			= { 0x0e0c, BIT(16) },
	[RST_BUS_UART4]			= { 0x0e10, BIT(16) },
	[RST_BUS_UART5]			= { 0x0e14, BIT(16) },
	[RST_BUS_UART6]			= { 0x0e18, BIT(16) },
	[RST_BUS_TWI0]			= { 0x0e80, BIT(16) },
	[RST_BUS_TWI1]			= { 0x0e84, BIT(16) },
	[RST_BUS_TWI2]			= { 0x0e88, BIT(16) },
	[RST_BUS_TWI3]			= { 0x0e8c, BIT(16) },
	[RST_BUS_TWI4]			= { 0x0e90, BIT(16) },
	[RST_BUS_TWI5]			= { 0x0e94, BIT(16) },
	[RST_BUS_TWI6]			= { 0x0e98, BIT(16) },
	[RST_BUS_TWI7]			= { 0x0e9c, BIT(16) },
	[RST_BUS_TWI8]			= { 0x0ea0, BIT(16) },
	[RST_BUS_TWI9]			= { 0x0ea4, BIT(16) },
	[RST_BUS_TWI10]			= { 0x0ea8, BIT(16) },
	[RST_BUS_TWI11]			= { 0x0eac, BIT(16) },
	[RST_BUS_TWI12]			= { 0x0eb0, BIT(16) },
	[RST_BUS_SPI0]			= { 0x0f04, BIT(16) },
	[RST_BUS_SPI1]			= { 0x0f0c, BIT(16) },
	[RST_BUS_SPI2]			= { 0x0f14, BIT(16) },
	[RST_BUS_SPIF]			= { 0x0f1c, BIT(16) },
	[RST_BUS_SPI3]			= { 0x0f24, BIT(16) },
	[RST_BUS_SPI4]			= { 0x0f2c, BIT(16) },
	[RST_BUS_GPADC0]		= { 0x0fc4, BIT(16) },
	[RST_BUS_THS0]			= { 0x0fe4, BIT(16) },
	[RST_BUS_IRRX]			= { 0x1004, BIT(16) },
	[RST_BUS_IRTX]			= { 0x100c, BIT(16) },
	[RST_BUS_LRADC]			= { 0x1024, BIT(16) },
	[RST_BUS_SGPIO]			= { 0x1064, BIT(16) },
	[RST_BUS_LPC]			= { 0x1084, BIT(16) },
	[RST_BUS_I2SPCM0]		= { 0x120c, BIT(16) },
	[RST_BUS_I2SPCM1]		= { 0x121c, BIT(16) },
	[RST_BUS_I2SPCM2]		= { 0x122c, BIT(16) },
	[RST_BUS_I2SPCM3]		= { 0x123c, BIT(16) },
	[RST_BUS_I2SPCM4]		= { 0x124c, BIT(16) },
	[RST_BUS_OWA]			= { 0x128c, BIT(16) },
	[RST_BUS_DMIC]			= { 0x12cc, BIT(16) },
	[RST_USB_0_PHY_RSTN]		= { 0x1300, BIT(30) },
	[RST_USB_0_DEVICE]		= { 0x1304, BIT(24) },
	[RST_USB_0_EHCI]		= { 0x1304, BIT(20) },
	[RST_USB_0_OHCI]		= { 0x1304, BIT(16) },
	[RST_USB_1_PHY_RSTN]		= { 0x1308, BIT(30) },
	[RST_USB_1_EHCI]		= { 0x130c, BIT(20) },
	[RST_USB_1_OHCI]		= { 0x130c, BIT(16) },
	[RST_USB_2]			= { 0x135c, BIT(16) },
	[RST_BUS_PCIE0]			= { 0x138c, BIT(17) },
	[RST_BUS_PCIE0_PWRUP]		= { 0x138c, BIT(16) },
	[RST_BUS_SERDES]		= { 0x13c4, BIT(16) },
	[RST_BUS_GMAC0_AXI]		= { 0x141c, BIT(17) },
	[RST_BUS_GMAC0]			= { 0x141c, BIT(16) },
	[RST_BUS_GMAC1_AXI]		= { 0x142c, BIT(17) },
	[RST_BUS_GMAC1]			= { 0x142c, BIT(16) },
	[RST_BUS_VO0_TCONLCD0]		= { 0x1504, BIT(16) },
	[RST_BUS_VO0_TCONLCD1]		= { 0x150c, BIT(16) },
	[RST_BUS_VO0_TCONLCD2]		= { 0x1514, BIT(16) },
	[RST_BUS_LVDS0]			= { 0x1544, BIT(16) },
	[RST_BUS_LVDS1]			= { 0x154c, BIT(16) },
	[RST_BUS_DSI0]			= { 0x1584, BIT(16) },
	[RST_BUS_DSI1]			= { 0x158c, BIT(16) },
	[RST_BUS_TCONTV0]		= { 0x1604, BIT(16) },
	[RST_BUS_TCONTV1]		= { 0x160c, BIT(16) },
	[RST_BUS_EDP]			= { 0x164c, BIT(16) },
	[RST_BUS_HDMI_HDCP]		= { 0x168c, BIT(18) },
	[RST_BUS_HDMI_SUB]		= { 0x168c, BIT(17) },
	[RST_BUS_HDMI_MAIN]		= { 0x168c, BIT(16) },
	[RST_BUS_DPSS_TOP0]		= { 0x16c4, BIT(16) },
	[RST_BUS_DPSS_TOP1]		= { 0x16cc, BIT(16) },
	[RST_BUS_VIDEO_OUT0]		= { 0x16e4, BIT(16) },
	[RST_BUS_VIDEO_OUT1]		= { 0x16ec, BIT(16) },
	[RST_BUS_LEDC]			= { 0x1704, BIT(16) },
	[RST_BUS_DSC]			= { 0x1744, BIT(16) },
	[RST_BUS_CSI]			= { 0x1844, BIT(16) },
	[RST_BUS_VIDEO_IN]		= { 0x1884, BIT(16) },
	[RST_BUS_APB2JTAG]		= { 0x1C04, BIT(16) },
};
/* rst_def_end */

/* ccu_def_start */
static struct clk_hw_onecell_data sun60iw2_hw_clks = {
	.hws    = {
		[CLK_PLL_REF]			= &pll_ref_clk.common.hw,
		[CLK_PLL_DDR]			= &pll_ddr_clk.common.hw,
		[CLK_PLL_PERI0]			= &pll_peri0_clk.common.hw,
		[CLK_PLL_PERI0_2X]		= &pll_peri0_2x_clk.common.hw,
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
		[CLK_HDMI_CEC_32K]			= &hdmi_cec_32k_clk.hw,
		[CLK_PLL_GPU0]			= &pll_gpu0_clk.common.hw,
		[CLK_PLL_VIDEO0]		= &pll_video0_clk.common.hw,
		[CLK_PLL_VIDEO0_4X]		= &pll_video0_4x_clk.common.hw,
		[CLK_PLL_VIDEO0_3X]		= &pll_video0_3x_clk.common.hw,
		[CLK_PLL_VIDEO1]		= &pll_video1_clk.common.hw,
		[CLK_PLL_VIDEO1_4X]		= &pll_video1_4x_clk.common.hw,
		[CLK_PLL_VIDEO1_3X] 	= &pll_video1_3x_clk.common.hw,
		[CLK_PLL_VIDEO2]		= &pll_video2_clk.common.hw,
		[CLK_PLL_VIDEO2_4X]		= &pll_video2_4x_clk.common.hw,
		[CLK_PLL_VIDEO2_3X]		= &pll_video2_3x_clk.common.hw,
		[CLK_PLL_VE0]			= &pll_ve0_clk.common.hw,
		[CLK_PLL_VE1]			= &pll_ve1_clk.common.hw,
		[CLK_PLL_AUDIO0_4X]		= &pll_audio0_4x_clk.common.hw,
		[CLK_PLL_AUDIO1]		= &pll_audio1_clk.common.hw,
		[CLK_PLL_AUDIO1_DIV2]		= &pll_audio1_div2_clk.common.hw,
		[CLK_PLL_AUDIO1_DIV5]		= &pll_audio1_div5_clk.common.hw,
		[CLK_PLL_NPU]			= &pll_npu_clk.common.hw,
		[CLK_PLL_DE]			= &pll_de_clk.common.hw,
		[CLK_PLL_DE_4X]			= &pll_de_4x_clk.common.hw,
		[CLK_PLL_DE_3X]			= &pll_de_3x_clk.common.hw,
		[CLK_AHB]			= &ahb_clk.common.hw,
		[CLK_APB0]			= &apb0_clk.common.hw,
		[CLK_APB1]			= &apb1_clk.common.hw,
		[CLK_APB_UART]			= &apb_uart_clk.common.hw,
		[CLK_TRACE]			= &trace_clk.common.hw,
		[CLK_GIC]			= &gic_clk.common.hw,
		[CLK_CPU_PERI]			= &cpu_peri_clk.common.hw,
		[CLK_ITS_PCIE0_A]		= &its_pcie0_a_clk.common.hw,
		[CLK_NSI]			= &nsi_clk.common.hw,
		[CLK_NSI_CFG]			= &nsi_cfg_clk.common.hw,
		[CLK_MBUS]			= &mbus_clk.common.hw,
		[CLK_IOMMU0_SYS_H]		= &iommu0_sys_h_clk.common.hw,
		[CLK_IOMMU0_SYS_P]		= &iommu0_sys_p_clk.common.hw,
		[CLK_IOMMU0_SYS_MBUS]		= &iommu0_sys_mbus_clk.common.hw,
		[CLK_MSI_LITE0]			= &msi_lite0_clk.common.hw,
		[CLK_MSI_LITE1]			= &msi_lite1_clk.common.hw,
		[CLK_MSI_LITE2]			= &msi_lite2_clk.common.hw,
		[CLK_IOMMU1_SYS_H]		= &iommu1_sys_h_clk.common.hw,
		[CLK_IOMMU1_SYS_P]		= &iommu1_sys_p_clk.common.hw,
		[CLK_IOMMU1_SYS_MBUS]		= &iommu1_sys_mbus_clk.common.hw,
		[CLK_CPUS_HCLK_GATE]		= &cpus_hclk_gate_clk.common.hw,
		[CLK_STORE_AHB_GATE]		= &store_ahb_gate_clk.common.hw,
		[CLK_MSILITE0_AHB_GATE]		= &msilite0_ahb_gate_clk.common.hw,
		[CLK_USB_SYS_AHB_GATE]		= &usb_sys_ahb_gate_clk.common.hw,
		[CLK_SERDES_AHB_GATE]		= &serdes_ahb_gate_clk.common.hw,
		[CLK_GPU0_AHB_GATE]		= &gpu0_ahb_gate_clk.common.hw,
		[CLK_NPU_AHB_GATE]		= &npu_ahb_gate_clk.common.hw,
		[CLK_DE_AHB_GATE]		= &de_ahb_gate_clk.common.hw,
		[CLK_VID_OUT1_AHB_GATE]		= &vid_out1_ahb_gate_clk.common.hw,
		[CLK_VID_OUT0_AHB_GATE]		= &vid_out0_ahb_gate_clk.common.hw,
		[CLK_VID_IN_AHB_GATE]		= &vid_in_ahb_gate_clk.common.hw,
		[CLK_VE_ENC_AHB_GATE]		= &ve_enc_ahb_gate_clk.common.hw,
		[CLK_VE_AHB_GATE]		= &ve_ahb_gate_clk.common.hw,
		[CLK_MBUS_MSILITE2_GATE]		= &msilite2_mbus_gate_clk.common.hw,
		[CLK_MBUS_STORE_GATE]		= &store_mbus_gate_clk.common.hw,
		[CLK_MBUS_MSILITE0_GATE]		= &msilite0_mbus_gate_clk.common.hw,
		[CLK_MBUS_SERDES_GATE]		= &serdes_mbus_gate_clk.common.hw,
		[CLK_MBUS_VID_IN_GATE]		= &vid_in_mbus_gate_clk.common.hw,
		[CLK_MBUS_NPU_GATE]		= &npu_mbus_gate_clk.common.hw,
		[CLK_MBUS_GPU0_GATE]		= &gpu0_mbus_gate_clk.common.hw,
		[CLK_DEC_MBUS_GATE]		= &ve_dec_mbus_gate_clk.common.hw,
		[CLK_MBUS_VE_GATE]		= &ve_mbus_gate_clk.common.hw,
		[CLK_MBUS_DESYS_GATE]		= &desys_mbus_gate_clk.common.hw,
		[CLK_IOMMU1_MBUS_GATE]	= &iommu1_mbus_gate_clk.common.hw,
		[CLK_IOMMU0_MBUS_GATE]	= &iommu0_mbus_gate_clk.common.hw,
		[CLK_VE_DEC_MBUS]		= &ve_dec_mbus_clk.common.hw,
		[CLK_GMAC1_MBUS]		= &gmac1_mbus_clk.common.hw,
		[CLK_GMAC0_MBUS]		= &gmac0_mbus_clk.common.hw,
		[CLK_ISP_MBUS]			= &isp_mbus_clk.common.hw,
		[CLK_CSI_MBUS]			= &csi_mbus_clk.common.hw,
		[CLK_NAND_MBUS]			= &nand_mbus_clk.common.hw,
		[CLK_DMA1_MBUS]			= &dma1_mbus_clk.common.hw,
		[CLK_MBUS_CE]			= &ce_mbus_clk.common.hw,
		[CLK_MBUS_VE]			= &ve_mbus_clk.common.hw,
		[CLK_MBUS_DMA0]			= &dma0_mbus_clk.common.hw,
		[CLK_DMA0]			= &dma0_clk.common.hw,
		[CLK_DMA1]			= &dma1_clk.common.hw,
		[CLK_SPINLOCK]			= &spinlock_clk.common.hw,
		[CLK_MSGBOX0]			= &msgbox0_clk.common.hw,
		[CLK_PWM0]			= &pwm0_clk.common.hw,
		[CLK_PWM1]			= &pwm1_clk.common.hw,
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
		[CLK_TIMER8]			= &timer8_clk.common.hw,
		[CLK_TIMER9]			= &timer9_clk.common.hw,
		[CLK_BUS_TIMER]			= &timer_bus_clk.common.hw,
		[CLK_AVS]			= &avs_clk.common.hw,
		[CLK_DE0]			= &de0_clk.common.hw,
		[CLK_BUS_DE0]			= &de0_gate_clk.common.hw,
		[CLK_DI]			= &di_clk.common.hw,
		[CLK_DI_GATE]			= &di_gate_clk.common.hw,
		[CLK_G2D]			= &g2d_clk.common.hw,
		[CLK_G2D_GATE]			= &g2d_gate_clk.common.hw,
		[CLK_EINK]			= &eink_clk.common.hw,
		[CLK_EINK_PANEL]		= &eink_panel_clk.common.hw,
		[CLK_EINK_GATE]			= &eink_gate_clk.common.hw,
		[CLK_VE_ENC0]			= &ve_enc0_clk.common.hw,
		[CLK_VE_DEC]			= &ve_dec_clk.common.hw,
		[CLK_BUS_VE_DEC]		= &ve_dec_bus_clk.common.hw,
		[CLK_BUS_VE_ENC]		= &ve_enc_bus_clk.common.hw,
		[CLK_CE]			= &ce_clk.common.hw,
		[CLK_CE_SYS]			= &ce_sys_clk.common.hw,
		[CLK_BUS_CE]			= &ce_bus_clk.common.hw,
		[CLK_NPU]			= &npu_clk.common.hw,
		[CLK_BUS_NPU]			= &npu_bus_clk.common.hw,
		[CLK_GPU0]			= &gpu0_clk.common.hw,
		[CLK_BUS_GPU0]			= &gpu0_bus_clk.common.hw,
		[CLK_DRAM0]			= &dram0_clk.common.hw,
		[CLK_BUS_DRAM0]			= &dram0_bus_clk.common.hw,
		[CLK_NAND0_CLK0]		= &nand0_clk0_clk.common.hw,
		[CLK_NAND0_CLK1]		= &nand0_clk1_clk.common.hw,
		[CLK_BUS_NAND0]			= &nand0_bus_clk.common.hw,
		[CLK_SMHC0]			= &smhc0_clk.common.hw,
		[CLK_BUS_SMHC0]		= &smhc0_gate_clk.common.hw,
		[CLK_SMHC1]			= &smhc1_clk.common.hw,
		[CLK_BUS_SMHC1]		= &smhc1_gate_clk.common.hw,
		[CLK_SMHC2]			= &smhc2_clk.common.hw,
		[CLK_BUS_SMHC2]			= &smhc2_gate_clk.common.hw,
		[CLK_SMHC3]			= &smhc3_clk.common.hw,
		[CLK_BUS_SMHC3]			= &smhc3_bus_clk.common.hw,
		[CLK_UFS_AXI]			= &ufs_axi_clk.common.hw,
		[CLK_UFS_CFG]			= &ufs_cfg_clk.common.hw,
		[CLK_UFS]			= &ufs_clk.common.hw,
		[CLK_UART0]			= &uart0_clk.common.hw,
		[CLK_UART1]			= &uart1_clk.common.hw,
		[CLK_UART2]			= &uart2_clk.common.hw,
		[CLK_UART3]			= &uart3_clk.common.hw,
		[CLK_UART4]			= &uart4_clk.common.hw,
		[CLK_UART5]			= &uart5_clk.common.hw,
		[CLK_UART6]			= &uart6_clk.common.hw,
		[CLK_TWI0]			= &twi0_clk.common.hw,
		[CLK_TWI1]			= &twi1_clk.common.hw,
		[CLK_TWI2]			= &twi2_clk.common.hw,
		[CLK_TWI3]			= &twi3_clk.common.hw,
		[CLK_TWI4]			= &twi4_clk.common.hw,
		[CLK_TWI5]			= &twi5_clk.common.hw,
		[CLK_TWI6]			= &twi6_clk.common.hw,
		[CLK_TWI7]			= &twi7_clk.common.hw,
		[CLK_TWI8]			= &twi8_clk.common.hw,
		[CLK_TWI9]			= &twi9_clk.common.hw,
		[CLK_TWI10]			= &twi10_clk.common.hw,
		[CLK_TWI11]			= &twi11_clk.common.hw,
		[CLK_TWI12]			= &twi12_clk.common.hw,
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
		[CLK_GPADC0_24M]		= &gpadc0_24m_clk.common.hw,
		[CLK_GPADC0]			= &gpadc0_clk.common.hw,
		[CLK_THS0]			= &ths0_clk.common.hw,
		[CLK_IRRX]			= &irrx_clk.common.hw,
		[CLK_IRRX_GATE]			= &irrx_gate_clk.common.hw,
		[CLK_IRTX]			= &irtx_clk.common.hw,
		[CLK_IRTX_GATE]			= &irtx_gate_clk.common.hw,
		[CLK_LRADC]			= &lradc_clk.common.hw,
		[CLK_SGPIO]			= &sgpio_clk.common.hw,
		[CLK_BUS_SGPIO]			= &sgpio_bus_clk.common.hw,
		[CLK_LPC]			= &lpc_clk.common.hw,
		[CLK_BUS_LPC]			= &lpc_bus_clk.common.hw,
		[CLK_I2SPCM0]			= &i2spcm0_clk.common.hw,
		[CLK_BUS_I2SPCM0]		= &i2spcm0_bus_clk.common.hw,
		[CLK_I2SPCM1]			= &i2spcm1_clk.common.hw,
		[CLK_BUS_I2SPCM1]		= &i2spcm1_bus_clk.common.hw,
		[CLK_I2SPCM2]			= &i2spcm2_clk.common.hw,
		[CLK_I2SPCM2_ASRC]		= &i2spcm2_asrc_clk.common.hw,
		[CLK_BUS_I2SPCM2]		= &i2spcm2_bus_clk.common.hw,
		[CLK_I2SPCM3]			= &i2spcm3_clk.common.hw,
		[CLK_BUS_I2SPCM3]		= &i2spcm3_bus_clk.common.hw,
		[CLK_I2SPCM4]			= &i2spcm4_clk.common.hw,
		[CLK_BUS_I2SPCM4] 		= &i2spcm4_bus_clk.common.hw,
		[CLK_OWA_TX]			= &owa_tx_clk.common.hw,
		[CLK_OWA_RX]			= &owa_rx_clk.common.hw,
		[CLK_BUS_OWA]			= &owa_bus_clk.common.hw,
		[CLK_DMIC]			= &dmic_clk.common.hw,
		[CLK_BUS_DMIC]			= &dmic_bus_clk.common.hw,
		[CLK_USB]			= &usb_clk.common.hw,
		[CLK_USB0_DEVICE]		= &usb0_device_clk.common.hw,
		[CLK_USB0_EHCI]			= &usb0_ehci_clk.common.hw,
		[CLK_USB0_OHCI]			= &usb0_ohci_clk.common.hw,
		[CLK_USB1]			= &usb1_clk.common.hw,
		[CLK_USB1_EHCI]			= &usb1_ehci_clk.common.hw,
		[CLK_USB1_OHCI]			= &usb1_ohci_clk.common.hw,
		[CLK_USB_REF]			= &usb_ref_clk.common.hw,
		[CLK_USB2_U2_REF]		= &usb2_u2_ref_clk.common.hw,
		[CLK_USB2_SUSPEND]		= &usb2_suspend_clk.common.hw,
		[CLK_USB2_MF]			= &usb2_mf_clk.common.hw,
		[CLK_USB2_U3_UTMI]		= &usb2_u3_utmi_clk.common.hw,
		[CLK_USB2_U2_PIPE]		= &usb2_u2_pipe_clk.common.hw,
		[CLK_PCIE0_AUX]			= &pcie0_aux_clk.common.hw,
		[CLK_PCIE0_AXI_SLV]		= &pcie0_axi_slv_clk.common.hw,
		[CLK_SERDES_PHY_CFG]		= &serdes_phy_cfg_clk.common.hw,
		[CLK_GMAC_PTP]			= &gmac_ptp_clk.common.hw,
		[CLK_GMAC0_PHY]			= &gmac0_phy_clk.common.hw,
		[CLK_GMAC0]			= &gmac0_clk.common.hw,
		[CLK_GMAC1_PHY]			= &gmac1_phy_clk.common.hw,
		[CLK_GMAC1]			= &gmac1_clk.common.hw,
		[CLK_VO0_TCONLCD0]		= &vo0_tconlcd0_clk.common.hw,
		[CLK_BUS_VO0_TCONLCD0]		= &vo0_tconlcd0_bus_clk.common.hw,
		[CLK_VO0_TCONLCD1]		= &vo0_tconlcd1_clk.common.hw,
		[CLK_BUS_VO0_TCONLCD1]		= &vo0_tconlcd1_bus_clk.common.hw,
		[CLK_VO0_TCONLCD2]		= &vo0_tconlcd2_clk.common.hw,
		[CLK_BUS_VO0_TCONLCD2]		= &vo0_tconlcd2_bus_clk.common.hw,
		[CLK_DSI0]			= &dsi0_clk.common.hw,
		[CLK_BUS_DSI0]			= &dsi0_bus_clk.common.hw,
		[CLK_DSI1]			= &dsi1_clk.common.hw,
		[CLK_BUS_DSI1]			= &dsi1_bus_clk.common.hw,
		[CLK_COMBPHY0]			= &combphy0_clk.common.hw,
		[CLK_COMBPHY1]			= &combphy1_clk.common.hw,
		[CLK_TCONTV0]			= &tcontv0_clk.common.hw,
		[CLK_TCONTV1]			= &tcontv1_clk.common.hw,
		[CLK_EDP_TV]			= &edp_tv_clk.common.hw,
		[CLK_EDP]			= &edp_clk.common.hw,
		[CLK_HDMI_REF]			= &hdmi_ref_clk.common.hw,
		[CLK_HDMI_TV]			= &hdmi_tv_clk.common.hw,
		[CLK_HDMI]			= &hdmi_clk.common.hw,
		[CLK_HDMI_SFR]			= &hdmi_sfr_clk.common.hw,
		[CLK_HDCP_ESM]			= &hdcp_esm_clk.common.hw,
		[CLK_DPSS_TOP0]			= &dpss_top0_clk.common.hw,
		[CLK_DPSS_TOP1]			= &dpss_top1_clk.common.hw,
		[CLK_LEDC]			= &ledc_clk.common.hw,
		[CLK_BUS_LEDC]			= &ledc_bus_clk.common.hw,
		[CLK_DSC]			= &dsc_clk.common.hw,
		[CLK_CSI_MASTER0]		= &csi_master0_clk.common.hw,
		[CLK_CSI_MASTER1]		= &csi_master1_clk.common.hw,
		[CLK_CSI_MASTER2]		= &csi_master2_clk.common.hw,
		[CLK_CSI]			= &csi_clk.common.hw,
		[CLK_BUS_CSI]			= &csi_bus_clk.common.hw,
		[CLK_ISP]			= &isp_clk.common.hw,
		[CLK_RES_DCAP_24M]	= &res_dcap_24m_clk.common.hw,
		[CLK_APB2JTAG]			= &apb2jtag_clk.common.hw,
		[CLK_FANOUT_25M]	= &fanout_25m_clk.common.hw,
		[CLK_FANOUT_16M]	= &fanout_16m_clk.common.hw,
		[CLK_FANOUT_12M]	= &fanout_12m_clk.common.hw,
		[CLK_FANOUT_24M]	= &fanout_24m_clk.common.hw,
		[CLK_CLK27M_FANOUT]	= &clk27m_fanout_clk.common.hw,
		[CLK_CLK_FANOUT]	= &clk_fanout_clk.common.hw,
		[CLK_SYS_12M]		= &sys_12M_clk.hw,
		[CLK_PLL_PERI0_16M]	= &pll_peri0_16m_clk.hw,
		[CLK_PLL_PERI0_25M]	= &pll_peri0_25m_clk.hw,
		[CLK_FANOUT3]		= &fanout3_clk.common.hw,
		[CLK_FANOUT2]		= &fanout2_clk.common.hw,
		[CLK_FANOUT1]		= &fanout1_clk.common.hw,
		[CLK_FANOUT0]		= &fanout0_clk.common.hw,
		[CLK_BUS_DEBUG]		= &bus_debug_clk.common.hw,
		[CLK_PLL_DDR_AUTO]		= &pll_ddr_auto_clk.common.hw,
		[CLK_PLL_PERI0_2X_AUTO]		= &pll_peri0_2x_auto_clk.common.hw,
		[CLK_PLL_PERI0_800M_AUTO]	= &pll_peri0_800m_auto_clk.common.hw,
		[CLK_PLL_PERI0_600M_AUTO]	= &pll_peri0_600m_auto_clk.common.hw,
		[CLK_PLL_PERI0_480M_ALL_AUTO]	= &pll_peri0_480m_all_auto_clk.common.hw,
		[CLK_PLL_PERI0_480M_AUTO]	= &pll_peri0_480m_auto_clk.common.hw,
		[CLK_PLL_PERI0_160M_AUTO]	= &pll_peri0_160m_auto_clk.common.hw,
		[CLK_PLL_PERI0_300M_ALL_AUTO]	= &pll_peri0_300m_all_auto_clk.common.hw,
		[CLK_PLL_PERI0_300M_AUTO]	= &pll_peri0_300m_auto_clk.common.hw,
		[CLK_PLL_PERI0_150M_AUTO]	= &pll_peri0_150m_auto_clk.common.hw,
		[CLK_PLL_PERI0_400M_ALL_AUTO]	= &pll_peri0_400m_all_auto_clk.common.hw,
		[CLK_PLL_PERI0_400M_AUTO]	= &pll_peri0_400m_auto_clk.common.hw,
		[CLK_PLL_PERI0_200M_AUTO]	= &pll_peri0_200m_auto_clk.common.hw,
		[CLK_PLL_PERI1_800M_AUTO]	= &pll_peri1_800m_auto_clk.common.hw,
		[CLK_PLL_PERI1_600M_ALL_AUTO]	= &pll_peri1_600m_all_auto_clk.common.hw,
		[CLK_PLL_PERI1_600M_AUTO]	= &pll_peri1_600m_auto_clk.common.hw,
		[CLK_PLL_PERI1_480M_ALL_AUTO]	= &pll_peri1_480m_all_auto_clk.common.hw,
		[CLK_PLL_PERI1_480M_AUTO]	= &pll_peri1_480m_auto_clk.common.hw,
		[CLK_PLL_PERI1_160M_AUTO]	= &pll_peri1_160m_auto_clk.common.hw,
		[CLK_PLL_PERI1_300M_ALL_AUTO]	= &pll_peri1_300m_all_auto_clk.common.hw,
		[CLK_PLL_PERI1_300M_AUTO]	= &pll_peri1_300m_auto_clk.common.hw,
		[CLK_PLL_PERI1_150M_AUTO]	= &pll_peri1_150m_auto_clk.common.hw,
		[CLK_PLL_PERI1_400M_ALL_AUTO]	= &pll_peri1_400m_all_auto_clk.common.hw,
		[CLK_PLL_PERI1_400M_AUTO]	= &pll_peri1_400m_auto_clk.common.hw,
		[CLK_PLL_PERI1_200M_AUTO]	= &pll_peri1_200m_auto_clk.common.hw,
		[CLK_PLL_VIDEO2_3X_AUTO]	= &pll_video2_3x_auto_clk.common.hw,
		[CLK_PLL_VIDEO1_3X_AUTO]	= &pll_video1_3x_auto_clk.common.hw,
		[CLK_PLL_VIDEO0_3X_AUTO]	= &pll_video0_3x_auto_clk.common.hw,
		[CLK_PLL_VIDEO2_4X_AUTO]	= &pll_video2_4x_auto_clk.common.hw,
		[CLK_PLL_VIDEO1_4X_AUTO]	= &pll_video1_4x_auto_clk.common.hw,
		[CLK_PLL_VIDEO0_4X_AUTO]	= &pll_video0_4x_auto_clk.common.hw,
		[CLK_PLL_GPU0_AUTO]		= &pll_gpu0_auto_clk.common.hw,
		[CLK_PLL_VE1_AUTO]		= &pll_ve1_auto_clk.common.hw,
		[CLK_PLL_VE0_AUTO]		= &pll_ve0_auto_clk.common.hw,
		[CLK_PLL_AUDIO1_DIV5_AUTO]	= &pll_audio1_div5_auto_clk.common.hw,
		[CLK_PLL_AUDIO1_DIV2_AUTO]	= &pll_audio1_div2_auto_clk.common.hw,
		[CLK_PLL_AUDIO0_4X_AUTO]	= &pll_audio0_4x_auto_clk.common.hw,
		[CLK_PLL_NPU_AUTO]		= &pll_npu_auto_clk.common.hw,
		[CLK_PLL_DE_3X_AUTO]		= &pll_de_3x_auto_clk.common.hw,
		[CLK_PLL_DE_4X_AUTO]		= &pll_de_4x_auto_clk.common.hw,
	},
	.num = CLK_NUMBER,
};
/* ccu_def_end */

static struct ccu_common *sun60iw2_ccu_clks[] = {
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
	&pll_video0_clk.common,
	&pll_video0_4x_clk.common,
	&pll_video0_3x_clk.common,
	&pll_video1_clk.common,
	&pll_video1_4x_clk.common,
	&pll_video1_3x_clk.common,
	&pll_video2_clk.common,
	&pll_video2_4x_clk.common,
	&pll_video2_3x_clk.common,
	&pll_ve0_clk.common,
	&pll_ve1_clk.common,
	&pll_audio0_4x_clk.common,
	&pll_audio1_clk.common,
	&pll_audio1_div2_clk.common,
	&pll_audio1_div5_clk.common,
	&pll_npu_clk.common,
	&pll_de_clk.common,
	&pll_de_4x_clk.common,
	&pll_de_3x_clk.common,
	&ahb_clk.common,
	&apb0_clk.common,
	&apb1_clk.common,
	&apb_uart_clk.common,
	&trace_clk.common,
	&gic_clk.common,
	&cpu_peri_clk.common,
	&its_pcie0_a_clk.common,
	&nsi_clk.common,
	&nsi_cfg_clk.common,
	&mbus_clk.common,
	&iommu0_sys_h_clk.common,
	&iommu0_sys_p_clk.common,
	&iommu0_sys_mbus_clk.common,
	&msi_lite0_clk.common,
	&msi_lite1_clk.common,
	&msi_lite2_clk.common,
	&iommu1_sys_h_clk.common,
	&iommu1_sys_p_clk.common,
	&iommu1_sys_mbus_clk.common,
	&cpus_hclk_gate_clk.common,
	&store_ahb_gate_clk.common,
	&msilite0_ahb_gate_clk.common,
	&usb_sys_ahb_gate_clk.common,
	&serdes_ahb_gate_clk.common,
	&gpu0_ahb_gate_clk.common,
	&npu_ahb_gate_clk.common,
	&de_ahb_gate_clk.common,
	&vid_out1_ahb_gate_clk.common,
	&vid_out0_ahb_gate_clk.common,
	&vid_in_ahb_gate_clk.common,
	&ve_enc_ahb_gate_clk.common,
	&ve_ahb_gate_clk.common,
	&msilite2_mbus_gate_clk.common,
	&store_mbus_gate_clk.common,
	&msilite0_mbus_gate_clk.common,
	&serdes_mbus_gate_clk.common,
	&vid_in_mbus_gate_clk.common,
	&npu_mbus_gate_clk.common,
	&gpu0_mbus_gate_clk.common,
	&ve_dec_mbus_gate_clk.common,
	&ve_mbus_gate_clk.common,
	&desys_mbus_gate_clk.common,
	&iommu1_mbus_gate_clk.common,
	&iommu0_mbus_gate_clk.common,
	&ve_dec_mbus_clk.common,
	&gmac1_mbus_clk.common,
	&gmac0_mbus_clk.common,
	&isp_mbus_clk.common,
	&csi_mbus_clk.common,
	&nand_mbus_clk.common,
	&dma1_mbus_clk.common,
	&ce_mbus_clk.common,
	&ve_mbus_clk.common,
	&dma0_mbus_clk.common,
	&dma0_clk.common,
	&dma1_clk.common,
	&spinlock_clk.common,
	&msgbox0_clk.common,
	&pwm0_clk.common,
	&pwm1_clk.common,
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
	&timer8_clk.common,
	&timer9_clk.common,
	&timer_bus_clk.common,
	&avs_clk.common,
	&de0_clk.common,
	&de0_gate_clk.common,
	&di_clk.common,
	&di_gate_clk.common,
	&g2d_clk.common,
	&g2d_gate_clk.common,
	&eink_clk.common,
	&eink_panel_clk.common,
	&eink_gate_clk.common,
	&ve_enc0_clk.common,
	&ve_dec_clk.common,
	&ve_dec_bus_clk.common,
	&ve_enc_bus_clk.common,
	&ce_clk.common,
	&ce_sys_clk.common,
	&ce_bus_clk.common,
	&npu_clk.common,
	&npu_bus_clk.common,
	&gpu0_clk.common,
	&gpu0_bus_clk.common,
	&dram0_clk.common,
	&dram0_bus_clk.common,
	&nand0_clk0_clk.common,
	&nand0_clk1_clk.common,
	&nand0_bus_clk.common,
	&smhc0_clk.common,
	&smhc0_gate_clk.common,
	&smhc1_clk.common,
	&smhc1_gate_clk.common,
	&smhc2_clk.common,
	&smhc2_gate_clk.common,
	&smhc3_clk.common,
	&smhc3_bus_clk.common,
	&ufs_axi_clk.common,
	&ufs_cfg_clk.common,
	&ufs_clk.common,
	&uart0_clk.common,
	&uart1_clk.common,
	&uart2_clk.common,
	&uart3_clk.common,
	&uart4_clk.common,
	&uart5_clk.common,
	&uart6_clk.common,
	&twi0_clk.common,
	&twi1_clk.common,
	&twi2_clk.common,
	&twi3_clk.common,
	&twi4_clk.common,
	&twi5_clk.common,
	&twi6_clk.common,
	&twi7_clk.common,
	&twi8_clk.common,
	&twi9_clk.common,
	&twi10_clk.common,
	&twi11_clk.common,
	&twi12_clk.common,
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
	&gpadc0_24m_clk.common,
	&gpadc0_clk.common,
	&ths0_clk.common,
	&irrx_clk.common,
	&irrx_gate_clk.common,
	&irtx_clk.common,
	&irtx_gate_clk.common,
	&lradc_clk.common,
	&sgpio_clk.common,
	&sgpio_bus_clk.common,
	&lpc_clk.common,
	&lpc_bus_clk.common,
	&i2spcm0_clk.common,
	&i2spcm0_bus_clk.common,
	&i2spcm1_clk.common,
	&i2spcm1_bus_clk.common,
	&i2spcm2_clk.common,
	&i2spcm2_asrc_clk.common,
	&i2spcm2_bus_clk.common,
	&i2spcm3_clk.common,
	&i2spcm3_bus_clk.common,
	&i2spcm4_clk.common,
	&i2spcm4_bus_clk.common,
	&owa_tx_clk.common,
	&owa_rx_clk.common,
	&owa_bus_clk.common,
	&dmic_clk.common,
	&dmic_bus_clk.common,
	&usb_clk.common,
	&usb0_device_clk.common,
	&usb0_ehci_clk.common,
	&usb0_ohci_clk.common,
	&usb1_clk.common,
	&usb1_ehci_clk.common,
	&usb1_ohci_clk.common,
	&usb_ref_clk.common,
	&usb2_u2_ref_clk.common,
	&usb2_suspend_clk.common,
	&usb2_mf_clk.common,
	&usb2_u3_utmi_clk.common,
	&usb2_u2_pipe_clk.common,
	&pcie0_aux_clk.common,
	&pcie0_axi_slv_clk.common,
	&serdes_phy_cfg_clk.common,
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
	&dsi0_clk.common,
	&dsi0_bus_clk.common,
	&dsi1_clk.common,
	&dsi1_bus_clk.common,
	&combphy0_clk.common,
	&combphy1_clk.common,
	&tcontv0_clk.common,
	&tcontv1_clk.common,
	&edp_tv_clk.common,
	&edp_clk.common,
	&hdmi_ref_clk.common,
	&hdmi_tv_clk.common,
	&hdmi_clk.common,
	&hdmi_sfr_clk.common,
	&hdcp_esm_clk.common,
	&dpss_top0_clk.common,
	&dpss_top1_clk.common,
	&ledc_clk.common,
	&ledc_bus_clk.common,
	&dsc_clk.common,
	&csi_master0_clk.common,
	&csi_master1_clk.common,
	&csi_master2_clk.common,
	&csi_clk.common,
	&csi_bus_clk.common,
	&isp_clk.common,
	&res_dcap_24m_clk.common,
	&apb2jtag_clk.common,
	&fanout_25m_clk.common,
	&fanout_16m_clk.common,
	&fanout_12m_clk.common,
	&fanout_24m_clk.common,
	&clk27m_fanout_clk.common,
	&clk_fanout_clk.common,
	&fanout3_clk.common,
	&fanout2_clk.common,
	&fanout1_clk.common,
	&fanout0_clk.common,
	&bus_debug_clk.common,
	&pll_ddr_auto_clk.common,
	&pll_peri0_2x_auto_clk.common,
	&pll_peri0_800m_auto_clk.common,
	&pll_peri0_600m_auto_clk.common,
	&pll_peri0_480m_all_auto_clk.common,
	&pll_peri0_480m_auto_clk.common,
	&pll_peri0_160m_auto_clk.common,
	&pll_peri0_300m_all_auto_clk.common,
	&pll_peri0_300m_auto_clk.common,
	&pll_peri0_150m_auto_clk.common,
	&pll_peri0_400m_all_auto_clk.common,
	&pll_peri0_400m_auto_clk.common,
	&pll_peri0_200m_auto_clk.common,
	&pll_peri1_800m_auto_clk.common,
	&pll_peri1_600m_all_auto_clk.common,
	&pll_peri1_600m_auto_clk.common,
	&pll_peri1_480m_all_auto_clk.common,
	&pll_peri1_480m_auto_clk.common,
	&pll_peri1_160m_auto_clk.common,
	&pll_peri1_300m_all_auto_clk.common,
	&pll_peri1_300m_auto_clk.common,
	&pll_peri1_150m_auto_clk.common,
	&pll_peri1_400m_all_auto_clk.common,
	&pll_peri1_400m_auto_clk.common,
	&pll_peri1_200m_auto_clk.common,
	&pll_video2_3x_auto_clk.common,
	&pll_video1_3x_auto_clk.common,
	&pll_video0_3x_auto_clk.common,
	&pll_video2_4x_auto_clk.common,
	&pll_video1_4x_auto_clk.common,
	&pll_video0_4x_auto_clk.common,
	&pll_gpu0_auto_clk.common,
	&pll_ve1_auto_clk.common,
	&pll_ve0_auto_clk.common,
	&pll_audio1_div5_auto_clk.common,
	&pll_audio1_div2_auto_clk.common,
	&pll_audio0_4x_auto_clk.common,
	&pll_npu_auto_clk.common,
	&pll_de_3x_auto_clk.common,
	&pll_de_4x_auto_clk.common,
#ifdef CONFIG_PM_SLEEP
	&pll_audio0_sdm_pat0_clk.common,
	&pll_audio0_sdm_pat1_clk.common,
	&pll_audio1_sdm_pat0_clk.common,
#endif
};

static struct ccu_reg_dump sun60iw2_distbus_restore_clks[] = {
	{ 0x05C0, AHB_MASTER_KEY_VALUE }, /* AHB_GATE_SW_CFG */
	{ 0x05E0, MBUS_MASTER_KEY_VALUE }, /* MBUS_MAT_CLK_GATING_REG */
	{ 0x05E4, MBUS_GATE_KEY_VALUE }, /* MBUS_GATE_ENABLE_REG */
};

static const struct sunxi_ccu_desc sun60iw2_ccu_desc = {
	.ccu_clks	= sun60iw2_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun60iw2_ccu_clks),

	.hw_clks	= &sun60iw2_hw_clks,

	.resets		= sun60iw2_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun60iw2_ccu_resets),
};

static int sun60iw2_ccu_really_probe(struct device_node *node)
{
	int ret;
	void __iomem *reg;

	reg = of_iomap(node, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	/* Pat1 is set due to multiple clock schemes.
	 * fraction pi enable.
	 * fraction adm enable.
	 */
	set_reg(reg + SUN60IW2_PLL_AUDIO0_PATTERN1_REG, PLL_AUDIO0_PAT1_PI_EN, 1, PLL_AUDIO0_PAT1_PI_EN_OFFSET);
	set_reg(reg + SUN60IW2_PLL_AUDIO0_PATTERN1_REG, PLL_AUDIO0_PAT1_SDM_EN, 1, PLL_AUDIO0_PAT1_SDM_EN_OFFSET);

	/* Enable AHB_MONITOR_EN and SD_MONITOR_EN.
	 * When this feature is enabled, it will automatically monitor the traffic of AHB (Advanced High-performance Bus).
	 * If there is no data, it will automatically turn off the clock of the relevant bus decoder,
	 * which helps reduce power consumption.*/
	set_reg(reg + AHB_GATE_EN_REG, 0x1, 1, AHB_MONITOR_ENABLE);
	set_reg(reg + AHB_GATE_EN_REG, 0x1, 1, SD_MONITOR_ENABLE);

	ret = sunxi_parse_sdm_info(node);
	if (ret)
		sunxi_debug(NULL, "%s: sdm_info not enabled", __func__);

	ret = sunxi_ccu_probe(node, reg, &sun60iw2_ccu_desc);
	if (ret)
		return ret;

	sunxi_ccu_sleep_init(reg, sun60iw2_ccu_clks,
			ARRAY_SIZE(sun60iw2_ccu_clks),
			sun60iw2_distbus_restore_clks,
			ARRAY_SIZE(sun60iw2_distbus_restore_clks));

	sunxi_info(NULL, "sunxi ccu driver version: %s\n", SUNXI_CCU_VERSION);
	return 0;
}

#if IS_ENABLED(CONFIG_AW_KERNEL_ORIGIN)
static void __init of_sun60iw2_ccu_init(struct device_node *node)
{
	sun60iw2_ccu_really_probe(node);
}

CLK_OF_DECLARE(sun60iw2_ccu_init, "allwinner,sun60iw2-ccu", of_sun60iw2_ccu_init);
#else
static int sun60iw2_ccu_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;

	return sun60iw2_ccu_really_probe(node);
}

static const struct of_device_id sun60iw2_ccu_ids[] = {
	{ .compatible = "allwinner,sun60iw2-ccu" },
	{ }
};

static struct platform_driver sun60iw2_ccu_driver = {
	.probe	= sun60iw2_ccu_probe,
	.driver	= {
		.name	= "sun60iw2-ccu",
		.of_match_table	= sun60iw2_ccu_ids,
	},
};

static int __init sun60iw2_ccu_init(void)
{
	int err;

	err = platform_driver_register(&sun60iw2_ccu_driver);
	if (err)
		pr_err("register ccu sun60iw2 failed\n");

	return err;
}

core_initcall(sun60iw2_ccu_init);

static void __exit sun60iw2_ccu_exit(void)
{
	platform_driver_unregister(&sun60iw2_ccu_driver);
}
module_exit(sun60iw2_ccu_exit);
#endif

MODULE_DESCRIPTION("Allwinner sun60iw2 clk driver");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SUNXI_CCU_VERSION);
