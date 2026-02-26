// SPDX-License-Identifier: GPL-3.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2022 liujuan1@allwinnertech.com
 */

#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include "ccu_common.h"
#include "ccu_reset.h"

#include "ccu_div.h"
#include "ccu_gate.h"
#include "ccu_mp.h"
#include "ccu_nm.h"

#include "ccu-sun60iw1-dsp.h"

/* ccu_des_start */

static struct ccu_nm pll_audio1_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* output divider */
	.common		= {
		.reg		= 0x0010,
		.hw.init	= CLK_HW_INIT("pll-audio1", "dcxo",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static SUNXI_CCU_M(pll_audio1_clk_div2, "pll-audio1-div2",
		"pll-audio1", 0x0010, 16, 3, 0);

static SUNXI_CCU_M(standby_clk, "standby",
		"pll-audio1-div2", 0x0028, 8, 5, 0);

static SUNXI_CCU_M(pll_audio1_clk_div5, "pll-audio1-div5",
		"pll-audio1", 0x0010, 20, 3, 0);

static SUNXI_CCU_M(pll_audio1_4X_clk, "pll-audio1-4x",
		"pll-audio1-div5", 0x0010, 0, 5, 0);

static const char * const rv_parents[] = { "dcxo", "rtc-32k", "rc-16m" };

static SUNXI_CCU_MUX_WITH_GATE(rv_clk, "rv",
		rv_parents, 0x100,
		24, 2,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(rv_bus_clk, "rv-bus",
		"dcxo",
		0x010c, BIT(1), 0);

static SUNXI_CCU_GATE(rv_cfg_clk, "rv-cfg",
		"dcxo",
		0x010c, BIT(0), 0);

static SUNXI_CCU_GATE(riscv_msg_clk, "riscv-msg",
		"dcxo",
		0x011c, BIT(0), 0);

static const char * const rv_timer_parents[] = { "dcxo", "rtc32k", "rc-16m", "pll-peri0-div3", "standby" };
static struct ccu_div rv_timer0_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(4, 3),
	.common		= {
		.reg		= 0x0120,
		.hw.init	= CLK_HW_INIT_PARENTS("rv-timer0", rv_timer_parents, &ccu_div_ops, 0),
	},
};

static struct ccu_div rv_timer1_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(4, 3),
	.common		= {
		.reg		= 0x0124,
		.hw.init	= CLK_HW_INIT_PARENTS("rv-timer1", rv_timer_parents, &ccu_div_ops, 0),
	},
};

static struct ccu_div rv_timer2_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(4, 3),
	.common		= {
		.reg		= 0x0128,
		.hw.init	= CLK_HW_INIT_PARENTS("rv-timer2", rv_timer_parents, &ccu_div_ops, 0),
	},
};

static struct ccu_div rv_timer3_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(4, 3),
	.common		= {

		.reg		= 0x012c,
		.hw.init	= CLK_HW_INIT_PARENTS("rv-timer3", rv_timer_parents, &ccu_div_ops, 0),
	},
};

static SUNXI_CCU_GATE(bus_rv_timer_clk, "bus-rv-timer",
		"dcxo",
		0x0130, BIT(0), 0);

static SUNXI_CCU_GATE(mcu_tzma1_clk, "mcu-tzma1",
		"dcxo",
		0x013c, BIT(1), 0);

static SUNXI_CCU_GATE(mcu_tzma0_clk, "mcu-tzma0",
		"dcxo",
		0x013c, BIT(0), 0);

static SUNXI_CCU_GATE(dsp_dma_mclk, "dsp-dma-mclk",
		"dcxo",
		0x014c, BIT(1), 0);

static SUNXI_CCU_GATE(dsp_dma_hclk, "dsp-dma-hclk",
		"dcxo",
		0x014c, BIT(0), 0);

static const char * const i2s_parents[] = { "pll-audio0-4x", "pll-audio1-4x", "pll-peri0-div3" };
static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(i2s_clk, "i2s",
		i2s_parents, 0x0150,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 2,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(mcu_i2s_clk, "mcu-i2s",
		"dcxo",
		0x015c, BIT(0), 0);

static const char * const dsp_timer_parents[] = { "dcxo", "rtc32k", "rc-16m", "pll-peri0-div3", "standby" };
static struct ccu_div dsp_timer0_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x0160,
		.hw.init	= CLK_HW_INIT_PARENTS("dsp-timer0", dsp_timer_parents, &ccu_div_ops, 0),
	},
};

static struct ccu_div dsp_timer1_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x0164,
		.hw.init	= CLK_HW_INIT_PARENTS("dsp-timer1", dsp_timer_parents, &ccu_div_ops, 0),
	},
};

static struct ccu_div dsp_timer2_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x0168,
		.hw.init	= CLK_HW_INIT_PARENTS("dsp-timer2", dsp_timer_parents, &ccu_div_ops, 0),
	},
};

static struct ccu_div dsp_timer3_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {

		.reg		= 0x016c,
		.hw.init	= CLK_HW_INIT_PARENTS("dsp-timer3", dsp_timer_parents, &ccu_div_ops, 0),
	},
};

static SUNXI_CCU_GATE(bus_dsp_timer_clk, "bus-dsp-timer",
		"dcxo",
		0x0170, BIT(0), 0);

static const char * const dsp_parents[] = { "dcxo", "osc32k", "rc-16m", "dsp", "pll-audio1-div2" };
static SUNXI_CCU_M_WITH_MUX_GATE(dsp_dsp_clk, "dsp_dsp", dsp_parents, 0x0174,
		0, 5,
		24, 3,
		BIT(31), 0);

static SUNXI_CCU_GATE(dsp_cfg_clk, "dsp-cfg",
		"dcxo",
		0x017C, BIT(0), 0);

static SUNXI_CCU_GATE(dsp_msg_clk, "dsp-msg",
		"dcxo",
		0x018C, BIT(0), 0);

static SUNXI_CCU_GATE(dsp_tzma_clk, "dsp-tzma",
		"dcxo",
		0x019C, BIT(0), 0);

static SUNXI_CCU_GATE(dsp_spinlock_clk, "dsp-spinlock",
		"dcxo",
		0x01ac, BIT(0), 0);

static const char * const dmic_parents[] = { "pll-audio0-4x", "pll-audio1-4x" };
static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(dmic_clk, "dmic",
		dmic_parents, 0x01b0,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 1,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(dmic_bus_clk, "dmic-bus",
		"dcxo",
		0x01bc, BIT(0), 0);

static SUNXI_CCU_GATE(msi_bus_clk, "msi-bus",
		"dcxo",
		0x01cc, BIT(0), 0);

static SUNXI_CCU_GATE(tbu_bus_clk, "tbu-bus",
		"dcxo",
		0x01dc, BIT(0), 0);

static SUNXI_CCU_GATE(pubsram0_clk, "pubsram0",
		"dcxo",
		0x02b8, BIT(0) | BIT(1), 0);

static SUNXI_CCU_GATE(pubsram1_clk, "pubsram1",
		"dcxo",
		0x02bc, BIT(0) | BIT(1), 0);
/* ccu_des_end */

/* rst_def_start */
static struct ccu_reset_map sun60iw1_dsp_ccu_resets[] = {
	[RST_BUS_RV]			= { 0x010c, BIT(18) },
	[RST_BUS_RV_APB]		= { 0x010c, BIT(17) },
	[RST_BUS_RV_CFG]		= { 0x010c, BIT(16) },
	[RST_BUS_RV_MSG]		= { 0x011c, BIT(16) },
	[RST_BUS_RV_TIME]		= { 0x0130, BIT(16) },
	[RST_BUS_DSP_DMA]		= { 0x014c, BIT(16) },
	[RST_BUS_DSP_I2S]		= { 0x015c, BIT(16) },
	[RST_BUS_DSP_TIME]		= { 0x0170, BIT(16) },
	[RST_BUS_DSP_CORE]		= { 0x017c, BIT(18) },
	[RST_BUS_DSP_DBG]		= { 0x017c, BIT(17) },
	[RST_BUS_DSP_CFG]		= { 0x017c, BIT(16) },
	[RST_BUS_DSP_MSG]		= { 0x018c, BIT(16) },
	[RST_BUS_DSP_SPINLOCK]		= { 0x01ac, BIT(16) },
	[RST_BUS_DSP_DMIC]		= { 0x01b0, BIT(16) },
	[MRST_BUS_DSP_MSI]		= { 0x01cc, BIT(17) },
	[HRST_BUS_DSP_MSI]		= { 0x01cc, BIT(16) },
	[HRST_BUS_DSP_TBU]		= { 0x01dc, BIT(16) },
	[HRST_BUS_DSP_VDD]		= { 0x0260, BIT(16) },
	[HRST_BUS_RV_SYS]		= { 0x0264, BIT(16) },
};
/* rst_def_end */

/* ccu_def_start */
static struct clk_hw_onecell_data sun60iw1_dsp_hw_clks = {
	.hws    = {
		[CLK_PLL_AUDIO1]		= &pll_audio1_clk.common.hw,
		[CLK_PLL_AUDIO1_DIV2]		= &pll_audio1_clk_div2.common.hw,
		[CLK_STANDBY]			= &standby_clk.common.hw,
		[CLK_PLL_AUDIO1_DIV5]		= &pll_audio1_clk_div5.common.hw,
		[CLK_PLL_AUDIO1_4X]		= &pll_audio1_4X_clk.common.hw,
		[CLK_RV]			= &rv_clk.common.hw,
		[CLK_RV_BUS]			= &rv_bus_clk.common.hw,
		[CLK_RV_CFG]			= &rv_cfg_clk.common.hw,
		[CLK_RISCV_MSG]			= &riscv_msg_clk.common.hw,
		[CLK_RV_TIMER0]			= &rv_timer0_clk.common.hw,
		[CLK_RV_TIMER1]			= &rv_timer1_clk.common.hw,
		[CLK_RV_TIMER2]			= &rv_timer2_clk.common.hw,
		[CLK_RV_TIMER3]			= &rv_timer3_clk.common.hw,
		[CLK_BUS_RV_TIMER]		= &bus_rv_timer_clk.common.hw,
		[CLK_MCU_TZMA1]			= &mcu_tzma1_clk.common.hw,
		[CLK_MCU_TZMA0]			= &mcu_tzma0_clk.common.hw,
		[CLK_DMA_MCLK]			= &dsp_dma_mclk.common.hw,
		[CLK_DMA_HCLK]			= &dsp_dma_hclk.common.hw,
		[CLK_I2S]			= &i2s_clk.common.hw,
		[CLK_MCU_I2S]			= &mcu_i2s_clk.common.hw,
		[CLK_DSP_TIMER0]		= &dsp_timer0_clk.common.hw,
		[CLK_DSP_TIMER1]		= &dsp_timer1_clk.common.hw,
		[CLK_DSP_TIMER2]		= &dsp_timer2_clk.common.hw,
		[CLK_DSP_TIMER3]		= &dsp_timer3_clk.common.hw,
		[CLK_BUS_DSP_TIMER]		= &bus_dsp_timer_clk.common.hw,
		[CLK_DSP_DSP]			= &dsp_dsp_clk.common.hw,
		[CLK_DSP_CFG]			= &dsp_cfg_clk.common.hw,
		[CLK_DSP_MSG]			= &dsp_msg_clk.common.hw,
		[CLK_DSP_TZMA]			= &dsp_tzma_clk.common.hw,
		[CLK_DSP_SPINLOCK]		= &dsp_spinlock_clk.common.hw,
		[CLK_DMIC]			= &dmic_clk.common.hw,
		[CLK_DMIC_BUS]			= &dmic_bus_clk.common.hw,
		[CLK_MSI_BUS]			= &msi_bus_clk.common.hw,
		[CLK_TBU_BUS]			= &tbu_bus_clk.common.hw,
		[CLK_PUBSRAM0]			= &pubsram0_clk.common.hw,
		[CLK_PUBSRAM1]			= &pubsram1_clk.common.hw,
	},
	.num = CLK_DSP_NUMBER,
};
/* ccu_def_end */

static struct ccu_common *sun60iw1_dsp_ccu_clks[] = {
	&pll_audio1_clk.common,
	&pll_audio1_clk_div2.common,
	&standby_clk.common,
	&pll_audio1_clk_div5.common,
	&pll_audio1_4X_clk.common,
	&rv_clk.common,
	&rv_bus_clk.common,
	&rv_cfg_clk.common,
	&riscv_msg_clk.common,
	&rv_timer0_clk.common,
	&rv_timer1_clk.common,
	&rv_timer2_clk.common,
	&rv_timer3_clk.common,
	&bus_rv_timer_clk.common,
	&mcu_tzma1_clk.common,
	&mcu_tzma0_clk.common,
	&dsp_dma_mclk.common,
	&dsp_dma_hclk.common,
	&i2s_clk.common,
	&mcu_i2s_clk.common,
	&dsp_timer0_clk.common,
	&dsp_timer1_clk.common,
	&dsp_timer2_clk.common,
	&dsp_timer3_clk.common,
	&bus_dsp_timer_clk.common,
	&dsp_dsp_clk.common,
	&dsp_cfg_clk.common,
	&dsp_msg_clk.common,
	&dsp_tzma_clk.common,
	&dsp_spinlock_clk.common,
	&dmic_clk.common,
	&dmic_bus_clk.common,
	&msi_bus_clk.common,
	&tbu_bus_clk.common,
	&pubsram0_clk.common,
	&pubsram1_clk.common,
};

static const struct sunxi_ccu_desc sun60iw1_dsp_ccu_desc = {
	.ccu_clks	= sun60iw1_dsp_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun60iw1_dsp_ccu_clks),

	.hw_clks	= &sun60iw1_dsp_hw_clks,

	.resets		= sun60iw1_dsp_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun60iw1_dsp_ccu_resets),
};

static void __init of_sun60iw1_dsp_ccu_init(struct device_node *node)
{
	void __iomem *reg;
	int ret;

	reg = of_iomap(node, 0);
	if (IS_ERR(reg))
		return;

	ret = sunxi_ccu_probe(node, reg, &sun60iw1_dsp_ccu_desc);
	if (ret)
		return;

	sunxi_ccu_sleep_init(reg, sun60iw1_dsp_ccu_clks,
			ARRAY_SIZE(sun60iw1_dsp_ccu_clks),
			NULL, 0);
}

CLK_OF_DECLARE(sun60iw1_dsp_ccu_init, "allwinner,sun60iw1-dsp-ccu", of_sun60iw1_dsp_ccu_init);

MODULE_VERSION("1.0.3");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
