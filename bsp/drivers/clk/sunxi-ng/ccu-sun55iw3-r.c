// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2021 liujuan1@allwinnertech.com
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

#include "ccu-sun55iw3-r.h"

/* ccu_des_start */
static const char * const ahbs_parents[] = { "dcxo24M", "ext-32k",
	"rc-16m", "pll-peri0-div3",
	"pll-audio1-4x" };

static SUNXI_CCU_MP_WITH_MUX(r_ahb_clk, "r-ahb",
		ahbs_parents, 0x000,
		0, 5,
		8, 2,
		24, 3,
		0);

static SUNXI_CCU_M_WITH_MUX(r_apbs0_clk, "r-apbs0",
		ahbs_parents, 0x00c,
		0, 5,
		24, 3,
		0);

static SUNXI_CCU_M_WITH_MUX(r_apbs1_clk, "r-apbs1",
		ahbs_parents, 0x010,
		0, 5,
		24, 3,
		0);

static const char * const r_timer_parents[] = { "dcxo24M", "ext-32k",
	"rc-16m", "pll-peri-200m" };

static struct ccu_div r_timer0_clk = {
	.enable		= BIT(0),
	.div		= _SUNXI_CCU_DIV_FLAGS(1, 4, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(4, 2),
	.common		= {
		.reg		= 0x0100,
		.hw.init	= CLK_HW_INIT_PARENTS("r-timer0",
				r_timer_parents,
				&ccu_div_ops, 0),
	},
};

static struct ccu_div r_timer1_clk = {
	.enable		= BIT(0),
	.div		= _SUNXI_CCU_DIV_FLAGS(1, 4, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(4, 2),
	.common		= {
		.reg		= 0x0104,
		.hw.init	= CLK_HW_INIT_PARENTS("r-timer1",
				r_timer_parents,
				&ccu_div_ops, 0),
	},
};

static struct ccu_div r_timer2_clk = {
	.enable		= BIT(0),
	.div		= _SUNXI_CCU_DIV_FLAGS(1, 4, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(4, 2),
	.common		= {
		.reg		= 0x0108,
		.hw.init	= CLK_HW_INIT_PARENTS("r-timer2",
				r_timer_parents,
				&ccu_div_ops, 0),
	},
};

static SUNXI_CCU_GATE(r_bus_timer_clk, "r-timer-gating",
		"dcxo24M",
		0x011c, BIT(0), 0);

static SUNXI_CCU_GATE(r_bus_twd_clk, "r-twd-gating",
		"dcxo24M",
		0x012c, BIT(0), 0);

static const char * const r_pwm_parents[] = { "dcxo24M", "ext-32k", "rc-16m" };

static SUNXI_CCU_MUX_WITH_GATE(r_pwm_clk, "r-pwm",
		r_pwm_parents, 0x0130,
		24, 2,
		BIT(31), 0);

static SUNXI_CCU_GATE(r_bus_pwm_clk, "r-pwm-gating",
		"dcxo24M",
		0x013c, BIT(0), 0);

static const char * const r_spi_parents[] = { "dcxo24M", "pll-peri0-div3", "pll-peri0-300m", "pll-peri1-300m", "pll-audio0-4x"};

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(r_spi_clk, "r-spi",
		r_spi_parents, 0x0150,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), CLK_SET_RATE_NO_REPARENT);

static SUNXI_CCU_GATE(r_bus_spi_clk, "r-spi-gating",
		"dcxo24M",
		0x015c, BIT(0), 0);

static SUNXI_CCU_GATE(r_bus_splock_clk, "r-splock-gating",
		"dcxo24M",
		0x016c, BIT(0), 0);

static SUNXI_CCU_GATE(r_bus_mbox_clk, "r-mbox-gating",
		"dcxo24M",
		0x017c, BIT(0), 0);

static SUNXI_CCU_GATE(r_bus_uart1_clk, "r-uart1-gating",
		"dcxo24M",
		0x018c, BIT(1), 0);

static SUNXI_CCU_GATE(r_bus_uart0_clk, "r-uart0-gating",
		"dcxo24M",
		0x018c, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_bus_twi2_clk, "r-twi2-gating",
		"dcxo24M",
		0x019c, BIT(2), 0);

static SUNXI_CCU_GATE(r_bus_twi1_clk, "r-twi1-gating",
		"dcxo24M",
		0x019c, BIT(1), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_bus_twi0_clk, "r-twi0-gating",
		"dcxo24M",
		0x019c, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_bus_ppu1_clk, "r-ppu1-gating",
		"dcxo24M",
		0x01ac, BIT(1), 0);

static SUNXI_CCU_GATE(r_bus_ppu_clk, "r-ppu-gating",
		"dcxo24M",
		0x01ac, BIT(0), 0);

static SUNXI_CCU_GATE(r_bus_tzma_clk, "r-tzma-gating",
		"dcxo24M",
		0x01b0, BIT(0), 0);

static SUNXI_CCU_GATE(r_cpus_bus_bist_clk, "r-cpus-bist-gating",
		"dcxo24M",
		0x01bc, BIT(0), 0);

static const char * const r_irrx_parents[] = { "ext-32k", "dcxo24M" };

static SUNXI_CCU_M_WITH_MUX_GATE(r_irrx_clk, "r-irrx",
		r_irrx_parents, 0x01c0,
		0, 5,	/* M */
		24, 2,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(r_bus_irrx_clk, "r-irrx-gating",
		"dcxo24M",
		0x01cc, BIT(0), 0);

static SUNXI_CCU_GATE(dma_clken_sw_clk, "dma-clken-sw",
		"dcxo24M",
		0x01dc, BIT(0), 0);

static SUNXI_CCU_GATE(r_bus_rtc_clk, "r-rtc-gating",
		"dcxo24M",
		0x020c, BIT(0), 0);

static SUNXI_CCU_GATE(r_bus_cpucfg_clk, "r-cpucfg-gating",
		"dcxo24M",
		0x022c, BIT(0), 0);
/* ccu_des_end */

/* rst_def_start */
static struct ccu_reset_map sun55iw3_r_ccu_resets[] = {
	[RST_R_TIMER]		=  { 0x11c, BIT(16) },
	[RST_R_PWM]		=  { 0x13c, BIT(16) },
	[RST_R_CAN]		=  { 0x14c, BIT(16) },
	[RST_R_SPI]		=  { 0x15c, BIT(16) },
	[RST_R_SPLOCK]		=  { 0x16c, BIT(16) },
	[RST_R_MBOX]		=  { 0x17c, BIT(16) },
	[RST_R_UART1]		=  { 0x18c, BIT(17) },
	[RST_R_UART0]		=  { 0x18c, BIT(16) },
	[RST_R_TWI2]		=  { 0x19c, BIT(18) },
	[RST_R_TWI1]		=  { 0x19c, BIT(17) },
	[RST_R_TWI0]		=  { 0x19c, BIT(16) },
	[RST_R_PPU1]		=  { 0x1ac, BIT(17) },
	[RST_R_PPU]		=  { 0x1ac, BIT(16) },
	[RST_R_IRRX]		=  { 0x1cc, BIT(16) },
	[RST_R_RTC]		=  { 0x20c, BIT(16) },
	[RST_R_CPUCFG]		=  { 0x22c, BIT(16) },
};
/* rst_def_end */

/* ccu_def_start */
static struct clk_hw_onecell_data sun55iw3_r_hw_clks = {
	.hws	= {
		[CLK_R_AHB]		= &r_ahb_clk.common.hw,
		[CLK_R_APBS0]		= &r_apbs0_clk.common.hw,
		[CLK_R_APBS1]		= &r_apbs1_clk.common.hw,
		[CLK_R_TIMER0]		= &r_timer0_clk.common.hw,
		[CLK_R_TIMER1]		= &r_timer1_clk.common.hw,
		[CLK_R_TIMER2]		= &r_timer2_clk.common.hw,
		[CLK_BUS_R_TIMER]	= &r_bus_timer_clk.common.hw,
		[CLK_BUS_R_TWD]		= &r_bus_twd_clk.common.hw,
		[CLK_R_PWM]		= &r_pwm_clk.common.hw,
		[CLK_BUS_R_PWM]		= &r_bus_pwm_clk.common.hw,
		[CLK_R_SPI]		= &r_spi_clk.common.hw,
		[CLK_BUS_R_SPI]		= &r_bus_spi_clk.common.hw,
		[CLK_BUS_R_SPLOCK]	= &r_bus_splock_clk.common.hw,
		[CLK_BUS_R_MBOX]	= &r_bus_mbox_clk.common.hw,
		[CLK_BUS_R_UART1]	= &r_bus_uart1_clk.common.hw,
		[CLK_BUS_R_UART0]	= &r_bus_uart0_clk.common.hw,
		[CLK_BUS_R_TWI2]	= &r_bus_twi2_clk.common.hw,
		[CLK_BUS_R_TWI1]	= &r_bus_twi1_clk.common.hw,
		[CLK_BUS_R_TWI0]	= &r_bus_twi0_clk.common.hw,
		[CLK_R_PPU1]		= &r_bus_ppu1_clk.common.hw,
		[CLK_R_PPU]		= &r_bus_ppu_clk.common.hw,
		[CLK_BUS_R_TZMA]	= &r_bus_tzma_clk.common.hw,
		[CLK_BUS_R_BIST]	= &r_cpus_bus_bist_clk.common.hw,
		[CLK_R_IRRX]		= &r_irrx_clk.common.hw,
		[CLK_BUS_R_IRRX]	= &r_bus_irrx_clk.common.hw,
		[CLK_DMA_CLKEN_SW]	= &dma_clken_sw_clk.common.hw,
		[CLK_BUS_R_RTC]		= &r_bus_rtc_clk.common.hw,
		[CLK_BUS_R_CPUCFG]	= &r_bus_cpucfg_clk.common.hw,
	},
	.num	= CLK_R_NUMBER,
};

/* ccu_def_end */

static struct ccu_common *sun55iw3_r_ccu_clks[] = {
	&r_ahb_clk.common,
	&r_apbs0_clk.common,
	&r_apbs1_clk.common,
	&r_timer0_clk.common,
	&r_timer1_clk.common,
	&r_timer2_clk.common,
	&r_bus_timer_clk.common,
	&r_bus_twd_clk.common,
	&r_pwm_clk.common,
	&r_bus_pwm_clk.common,
	&r_spi_clk.common,
	&r_bus_spi_clk.common,
	&r_bus_splock_clk.common,
	&r_bus_mbox_clk.common,
	&r_bus_uart1_clk.common,
	&r_bus_uart0_clk.common,
	&r_bus_twi2_clk.common,
	&r_bus_twi1_clk.common,
	&r_bus_twi0_clk.common,
	&r_bus_ppu1_clk.common,
	&r_bus_ppu_clk.common,
	&r_bus_tzma_clk.common,
	&r_cpus_bus_bist_clk.common,
	&r_irrx_clk.common,
	&r_bus_irrx_clk.common,
	&dma_clken_sw_clk.common,
	&r_bus_rtc_clk.common,
	&r_bus_cpucfg_clk.common,
};



static const struct sunxi_ccu_desc sun55iw3_r_ccu_desc = {
	.ccu_clks	= sun55iw3_r_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun55iw3_r_ccu_clks),

	.hw_clks	= &sun55iw3_r_hw_clks,

	.resets		= sun55iw3_r_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun55iw3_r_ccu_resets),
};

static int sun55iw3_r_ccu_probe(struct platform_device *pdev)
{
	void __iomem *reg;
	int ret;

	reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	ret = sunxi_ccu_probe(pdev->dev.of_node, reg, &sun55iw3_r_ccu_desc);
	if (ret)
		return ret;

	sunxi_ccu_sleep_init(reg, sun55iw3_r_ccu_clks,
			ARRAY_SIZE(sun55iw3_r_ccu_clks),
			NULL, 0);

	return 0;
}

static const struct of_device_id sun55iw3_r_ccu_ids[] = {
	{ .compatible = "allwinner,sun55iw3-r-ccu" },
	{ }
};

static struct platform_driver sun55iw3_r_ccu_driver = {
	.probe	= sun55iw3_r_ccu_probe,
	.driver	= {
		.name	= "sun55iw3-r-ccu",
		.of_match_table	= sun55iw3_r_ccu_ids,
	},
};

static int __init sunxi_ccu_sun55iw3_r_init(void)
{
	int ret;

	ret = platform_driver_register(&sun55iw3_r_ccu_driver);
	if (ret)
		pr_err("register ccu sun55iw3-r failed\n");

	return ret;
}
core_initcall(sunxi_ccu_sun55iw3_r_init);

static void __exit sunxi_ccu_sun55iw3_r_exit(void)
{
	return platform_driver_unregister(&sun55iw3_r_ccu_driver);
}
module_exit(sunxi_ccu_sun55iw3_r_exit);

MODULE_DESCRIPTION("Allwinner sun55iw3-r clk driver");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("rgm<rengaomin@allwinnertech.com>");
MODULE_VERSION("1.0.8");
