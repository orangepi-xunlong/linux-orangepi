// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2020 frank@allwinnertech.com
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

#include "ccu-sun50iw12-r.h"


static const char * const cpus_r_apb_parents[] = { "dcxo24M", "osc32k",
						     "iosc", "pll-periph0" };
static const struct ccu_mux_var_prediv cpus_r_apb_predivs[] = {
	{ .index = 3, .shift = 0, .width = 5 },
};

static struct ccu_div cpus_clk = {
	.div		= _SUNXI_CCU_DIV_FLAGS(8, 2, CLK_DIVIDER_POWER_OF_TWO),

	.mux		= {
		.shift	= 24,
		.width	= 2,

		.var_predivs	= cpus_r_apb_predivs,
		.n_var_predivs	= ARRAY_SIZE(cpus_r_apb_predivs),
	},

	.common		= {
		.reg		= 0x000,
		.features	= CCU_FEATURE_VARIABLE_PREDIV,
		.hw.init	= CLK_HW_INIT_PARENTS("cpus",
						      cpus_r_apb_parents,
						      &ccu_div_ops,
						      0),
	},
};

static CLK_FIXED_FACTOR_HW(r_ahb_clk, "r-ahb", &cpus_clk.common.hw, 1, 1, 0);

static struct ccu_div r_apb0_clk = {
	.div		= _SUNXI_CCU_DIV_FLAGS(8, 2, CLK_DIVIDER_POWER_OF_TWO),

	.mux		= {
		.shift	= 24,
		.width	= 2,

		.var_predivs	= cpus_r_apb_predivs,
		.n_var_predivs	= ARRAY_SIZE(cpus_r_apb_predivs),
	},

	.common		= {
		.reg		= 0x0C,
		.features	= CCU_FEATURE_VARIABLE_PREDIV,
		.hw.init	= CLK_HW_INIT_PARENTS("r-apb0",
						      cpus_r_apb_parents,
						      &ccu_div_ops,
						      0),
	},
};

static struct ccu_div r_apb1_clk = {
	.div		= _SUNXI_CCU_DIV_FLAGS(8, 2, CLK_DIVIDER_POWER_OF_TWO),

	.mux		= {
		.shift	= 24,
		.width	= 2,

		.var_predivs	= cpus_r_apb_predivs,
		.n_var_predivs	= ARRAY_SIZE(cpus_r_apb_predivs),
	},

	.common		= {
		.reg		= 0x10,
		.features	= CCU_FEATURE_VARIABLE_PREDIV,
		.hw.init	= CLK_HW_INIT_PARENTS("r-apb1",
						      cpus_r_apb_parents,
						      &ccu_div_ops,
						      0),
	},
};

static const char * const r_timer_parents[] = { "dcxo24M", "iosc",
						     "osc32k", "r-ahb" };
static struct ccu_div r_apb0_timer0_clk = {
	.enable		= BIT(0),
	.div		= _SUNXI_CCU_DIV_FLAGS(1, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(4, 2),
	.common		= {
		.reg		= 0x110,
		.hw.init	= CLK_HW_INIT_PARENTS("r-timer0",
						      r_timer_parents,
						      &ccu_div_ops,
						      0),
	},
};

static struct ccu_div r_apb0_timer1_clk = {
	.enable		= BIT(0),
	.div		= _SUNXI_CCU_DIV_FLAGS(1, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(4, 2),
	.common		= {
		.reg		= 0x114,
		.hw.init	= CLK_HW_INIT_PARENTS("r-timer1",
						      r_timer_parents,
						      &ccu_div_ops,
						      0),
	},
};

static struct ccu_div r_apb0_timer2_clk = {
	.enable		= BIT(0),
	.div		= _SUNXI_CCU_DIV_FLAGS(1, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(4, 2),
	.common		= {
		.reg		= 0x118,
		.hw.init	= CLK_HW_INIT_PARENTS("r-timer2",
						      r_timer_parents,
						      &ccu_div_ops,
						      0),
	},
};
static SUNXI_CCU_GATE(bus_r_apb0_timer0_clk, "bus-r-timer0", "r-apb0",
		      0x11c, BIT(0), 0);

static SUNXI_CCU_MP_WITH_MUX_GATE(r_apb0_edid_clk, "r-edid",
				  cpus_r_apb_parents, 0x124,
				  0, 5,		/* M */
				  8, 2,		/* P */
				  24, 2,	/* mux */
				  BIT(31),	/* gate */
				  0);

static SUNXI_CCU_GATE(bus_r_apb0_wdt1_clk, "bus-r-wdt1", "r-apb0",
		      0x12c, BIT(0), 0);

static const char * const r_pwm_parents[] = { "dcxo24M", "osc32k",
						   "iosc" };
static SUNXI_CCU_MUX_WITH_GATE(r_apb0_pwm_clk, "r-pwm",
			       r_pwm_parents, 0x130,
			       24, 2,		/* mux */
			       BIT(31),		/* gate */
			       0);

static SUNXI_CCU_GATE(bus_r_apb0_pwm_clk, "bus-r-pwm", "r-apb0",
		      0x13c, BIT(0), 0);

static SUNXI_CCU_GATE(bus_apb1_r_uart_clk, "bus-r-uart", "r-apb1",
		      0x18c, BIT(0), 0);
static SUNXI_CCU_GATE(bus_r_apb1_i2c0_clk, "bus-r-i2c0", "r-apb1",
		      0x19c, BIT(0), 0);
static SUNXI_CCU_GATE(bus_r_apb1_i2c1_clk, "bus-r-i2c1", "r-apb1",
		      0x19c, BIT(1), 0);

static SUNXI_CCU_GATE(bus_r_apb1_ppu_clk, "bus-r-ppu", "r-apb1",
		      0x1ac, BIT(0), 0);

static SUNXI_CCU_GATE(bus_r_apb1_bus_tzma_clk, "bus-r-tzma", "r-apb1",
		      0x1b0, BIT(0), 0);

static SUNXI_CCU_GATE(bus_r_cpus_bist_clk, "bus-r-cpus_bist", "r-apb1",
		      0x1bc, BIT(0), 0);

static const char * const r_ir_parents[] = { "osc32k", "dcxo24M" };
static SUNXI_CCU_MP_WITH_MUX_GATE(r_apb0_ir_clk, "r-ir",
				  r_ir_parents, 0x1c0,
				  0, 5,		/* M */
				  8, 2,		/* P */
				  24, 1,	/* mux */
				  BIT(31),	/* gate */
				  0);

static SUNXI_CCU_GATE(bus_r_apb0_ir_clk, "bus-r-ir", "r-apb0",
		      0x1cc, BIT(0), 0);

static SUNXI_CCU_GATE(bus_r_ahb_rtc_clk, "bus-r-rtc", "r-ahb",
		      0x20c, BIT(0), 0);

static SUNXI_CCU_GATE(bus_r_ahb_cpucfg_clk, "bus-r-cpucfg", "r-ahb",
		      0x22c, BIT(0), 0);

static struct ccu_common *sun50iw12_r_ccu_clks[] = {
	&cpus_clk.common,
	&r_apb0_clk.common,
	&r_apb1_clk.common,
	&r_apb0_timer0_clk.common,
	&r_apb0_timer1_clk.common,
	&r_apb0_timer2_clk.common,
	&bus_r_apb0_timer0_clk.common,
	&r_apb0_edid_clk.common,
	&bus_r_apb0_wdt1_clk.common,
	&r_apb0_pwm_clk.common,
	&bus_r_apb0_pwm_clk.common,
	&bus_apb1_r_uart_clk.common,
	&bus_r_apb1_i2c0_clk.common,
	&bus_r_apb1_i2c1_clk.common,
	&bus_r_apb1_ppu_clk.common,
	&bus_r_apb1_bus_tzma_clk.common,
	&bus_r_cpus_bist_clk.common,
	&r_apb0_ir_clk.common,
	&bus_r_apb0_ir_clk.common,
	&bus_r_ahb_rtc_clk.common,
	&bus_r_ahb_cpucfg_clk.common,
};

static struct clk_hw_onecell_data sun50iw12_r_hw_clks = {
	.hws	= {
		[CLK_CPUS]		= &cpus_clk.common.hw,
		[CLK_R_AHB]		= &r_ahb_clk.hw,
		[CLK_R_APB0]		= &r_apb0_clk.common.hw,
		[CLK_R_APB1]		= &r_apb1_clk.common.hw,
		[CLK_R_APB0_TIMER0]	= &r_apb0_timer0_clk.common.hw,
		[CLK_R_APB0_TIMER1]	= &r_apb0_timer1_clk.common.hw,
		[CLK_R_APB0_TIMER2]	= &r_apb0_timer2_clk.common.hw,
		[CLK_R_APB0_BUS_TIMER0]	= &bus_r_apb0_timer0_clk.common.hw,
		[CLK_R_APB0_EDID]	= &r_apb0_edid_clk.common.hw,
		[CLK_R_APB0_BUS_WDT1]	= &bus_r_apb0_wdt1_clk.common.hw,
		[CLK_R_APB0_PWM]	= &r_apb0_pwm_clk.common.hw,
		[CLK_R_APB0_BUS_PWM]	= &bus_r_apb0_pwm_clk.common.hw,
		[CLK_R_APB1_BUS_UART]	= &bus_apb1_r_uart_clk.common.hw,
		[CLK_R_APB1_BUS_I2C0]	= &bus_r_apb1_i2c0_clk.common.hw,
		[CLK_R_APB1_BUS_I2C1]	= &bus_r_apb1_i2c1_clk.common.hw,
		[CLK_R_APB1_BUS_PPU]	= &bus_r_apb1_ppu_clk.common.hw,
		[CLK_R_APB1_BUS_TZMA]	= &bus_r_apb1_bus_tzma_clk.common.hw,
		[CLK_R_CPUS_BUS_BIST]	= &bus_r_cpus_bist_clk.common.hw,
		[CLK_R_APB0_IR]		= &r_apb0_ir_clk.common.hw,
		[CLK_R_APB0_BUS_IR]	= &bus_r_apb0_ir_clk.common.hw,
		[CLK_R_AHB_BUS_RTC]	= &bus_r_ahb_rtc_clk.common.hw,
		[CLK_R_AHB_BUS_CPUCFG]	= &bus_r_ahb_cpucfg_clk.common.hw,
	},
	.num	= CLK_NUMBER,
};

static struct ccu_reset_map sun50iw12_r_ccu_resets[] = {
	[RST_R_APB0_BUS_TIMER0]	=  { 0x11c, BIT(16) },
	[RST_R_APB0_BUS_EDID]	=  { 0x120, BIT(16) },
	[RST_R_APB0_BUS_PWM]	=  { 0x13c, BIT(16) },
	[RST_R_APB1_BUS_UART0]	=  { 0x18c, BIT(16) },
	[RST_R_APB1_BUS_I2C0]	=  { 0x19c, BIT(16) },
	[RST_R_APB1_BUS_I2C1]	=  { 0x19c, BIT(17) },
	[RST_R_APB1_BUS_PPU]	=  { 0x1ac, BIT(16) },
	[RST_R_APB0_BUS_IR_RX]	=  { 0x1cc, BIT(16) },
	[RST_R_AHB_BUS_RTC]	=  { 0x20c, BIT(16) },
	[RST_R_AHB_BUS_CPUCFG]	=  { 0x22c, BIT(16) },
	[RST_R_MODULE]		=  { 0x260, BIT(0) },
};

static const struct sunxi_ccu_desc sun50iw12_r_ccu_desc = {
	.ccu_clks	= sun50iw12_r_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun50iw12_r_ccu_clks),

	.hw_clks	= &sun50iw12_r_hw_clks,

	.resets		= sun50iw12_r_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun50iw12_r_ccu_resets),
};

static void __init of_sun50iw12_r_ccu_init(struct device_node *node)
{
	void __iomem *reg;
	int ret;

	reg = of_iomap(node, 0);
	if (IS_ERR(reg))
		return;

	ret = sunxi_ccu_probe(node, reg, &sun50iw12_r_ccu_desc);
	if (ret)
		return;

	sunxi_ccu_sleep_init(reg, sun50iw12_r_ccu_clks,
			     ARRAY_SIZE(sun50iw12_r_ccu_clks),
			     NULL, 0);
}

CLK_OF_DECLARE(sun50iw12_r_ccu_init, "allwinner,sun50iw12-r-ccu", of_sun50iw12_r_ccu_init);
