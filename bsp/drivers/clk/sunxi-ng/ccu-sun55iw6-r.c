// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 panzhijian@allwinnertech.com
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

#include "ccu-sun55iw6-r.h"

/* ccu_des_start */

static const char * const r_timer0_parents[] = { "dcxo24M", "rtc-32k", "rc16m", "peripll-div", "refpll-24m" };

static SUNXI_CCU_M_WITH_MUX_GATE(r_timer0_clk, "r-timer0",
		r_timer0_parents, 0x0100,
		1, 3,	/* M */
		4, 3,	/* mux */
		BIT(0),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const r_timer1_parents[] = { "dcxo24M", "rtc-32k", "rc16m", "peripll-div", "refpll-24m" };

static SUNXI_CCU_M_WITH_MUX_GATE(r_timer1_clk, "r-timer1",
		r_timer1_parents, 0x0104,
		1, 3,	/* M */
		4, 3,	/* mux */
		BIT(0),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const r_timer2_parents[] = { "dcxo24M", "rtc-32k", "rc16m", "peripll-div", "refpll-24m" };

static SUNXI_CCU_M_WITH_MUX_GATE(r_timer2_clk, "r-timer2",
		r_timer2_parents, 0x0108,
		1, 3,	/* M */
		4, 3,	/* mux */
		BIT(0),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const r_timer3_parents[] = { "dcxo24M", "rtc-32k", "rc16m", "peripll-div", "refpll-24m" };

static SUNXI_CCU_M_WITH_MUX_GATE(r_timer3_clk, "r-timer3",
		r_timer3_parents, 0x010C,
		1, 3,	/* M */
		4, 3,	/* mux */
		BIT(0),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(r_timer_clk, "r-timer",
		"dcxo24M",
		0x011C, BIT(0), 0);

static SUNXI_CCU_GATE(r_twd_clk, "r-twd",
		"dcxo24M",
		0x012C, BIT(0), 0);

static const char * const r_pwm_parents[] = { "dcxo24M", "rtc-32k", "rc16m", "refpll-24m" };

static SUNXI_CCU_MUX_WITH_GATE(r_pwm_clk, "r-pwm",
		r_pwm_parents, 0x0130,
		24, 2,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(r_pwm_bus_clk, "r-pwm-bus",
		"dcxo24M",
		0x013C, BIT(0), 0);

static const char * const r_spi_parents[] = { "dcxo24M", "peripll-div", "peri0-300m", "peri1-300m", "refpll-24m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(r_spi_clk, "r-spi",
		r_spi_parents, 0x0150,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(r_spi_bus_clk, "r-spi-bus",
		"dcxo24M",
		0x015C, BIT(0), 0);

static SUNXI_CCU_GATE(r_mbox_clk, "r-mbox",
		"dcxo24M",
		0x017C, BIT(0), 0);

static SUNXI_CCU_GATE(r_uart1_clk, "r-uart1",
		"dcxo24M",
		0x018C, BIT(1), 0);

static SUNXI_CCU_GATE(r_uart0_clk, "r-uart0",
		"dcxo24M",
		0x018C, BIT(0), 0);

static SUNXI_CCU_GATE(r_twi1_clk, "r-twi1",
		"dcxo24M",
		0x019C, BIT(1), 0);

static SUNXI_CCU_GATE(r_twi0_clk, "r-twi0",
		"dcxo24M",
		0x019C, BIT(0), 0);

static SUNXI_CCU_GATE(r_ppu_clk, "r-ppu",
		"dcxo24M",
		0x01AC, BIT(0), 0);

static SUNXI_CCU_GATE(r_tzma_clk, "r-tzma",
		"dcxo24M",
		0x01B0, BIT(0), 0);

static SUNXI_CCU_GATE(r_cpus_bist_clk, "r-cpus-bist",
		"dcxo24M",
		0x01BC, BIT(0), 0);

static const char * const r_irrx_parents[] = { "osc32k", "dcxo24M", "refpll-24m" };

static SUNXI_CCU_M_WITH_MUX_GATE(r_irrx_clk, "r-irrx",
		r_irrx_parents, 0x01C0,
		0, 5,	/* M */
		24, 2,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(r_irrx_bus_clk, "r-irrx-bus",
		"dcxo24M",
		0x01CC, BIT(0), 0);

static SUNXI_CCU_GATE(rtc_clk, "rtc",
		"dcxo24M",
		0x020C, BIT(0), 0);

static SUNXI_CCU_GATE(r_cpucfg_clk, "r-cpucfg",
		"dcxo24M",
		0x022C, BIT(0), 0);

static SUNXI_CCU_GATE(vdd_usb2cpus_clk, "vdd-usb2cpus",
		"dcxo24M",
		0x0250, BIT(8), 0);

static SUNXI_CCU_GATE(vdd_sys2cpus_clk, "vdd-sys2cpus",
		"dcxo24M",
		0x0250, BIT(2), 0);

static SUNXI_CCU_GATE(vdd_ddr_clk, "vdd-ddr",
		"dcxo24M",
		0x0250, BIT(0), 0);

static SUNXI_CCU_GATE(tt_auto_clk, "tt-auto",
		"dcxo24M",
		0x0338, BIT(9), 0);

static SUNXI_CCU_GATE(cpu_icache_auto_clk, "cpu-icache-auto",
		"dcxo24M",
		0x0338, BIT(8), 0);

static SUNXI_CCU_GATE(ahbs_auto_clk__clk, "ahbs-auto-clk",
		"dcxo24M",
		0x033C, BIT(24), 0);
/* ccu_des_end */

/* rst_def_start */
static struct ccu_reset_map sun55iw6_r_ccu_resets[] = {
	[RST_BUS_R_TIME]		= { 0x011c, BIT(16) },
	[RST_BUS_R_PWM]			= { 0x013c, BIT(16) },
	[RST_BUS_R_SPI]			= { 0x015c, BIT(16) },
	[RST_BUS_R_MBOX]		= { 0x017c, BIT(16) },
	[RST_BUS_R_UART1]		= { 0x018c, BIT(17) },
	[RST_BUS_R_UART0]		= { 0x018c, BIT(16) },
	[RST_BUS_R_TWI1]		= { 0x019c, BIT(17) },
	[RST_BUS_R_TWI0]		= { 0x019c, BIT(16) },
	[RST_BUS_R_IRRX]		= { 0x01cc, BIT(16) },
	[RST_BUS_RTC]			= { 0x020c, BIT(16) },
	[RST_BUS_R_CPUCFG]		= { 0x022c, BIT(16) },
	[RST_BUS_MODULE]		= { 0x0260, BIT(0) },
};
/* rst_def_end */
/* ccu_def_start */
static struct clk_hw_onecell_data sun55iw6_r_hw_clks = {
	.hws    = {
		[CLK_R_TIMER0]			= &r_timer0_clk.common.hw,
		[CLK_R_TIMER1]			= &r_timer1_clk.common.hw,
		[CLK_R_TIMER2]			= &r_timer2_clk.common.hw,
		[CLK_R_TIMER3]			= &r_timer3_clk.common.hw,
		[CLK_R_TIMER]			= &r_timer_clk.common.hw,
		[CLK_R_TWD]			= &r_twd_clk.common.hw,
		[CLK_R_PWM]			= &r_pwm_clk.common.hw,
		[CLK_R_BUS_PWM]			= &r_pwm_bus_clk.common.hw,
		[CLK_R_SPI]			= &r_spi_clk.common.hw,
		[CLK_R_BUS_SPI]			= &r_spi_bus_clk.common.hw,
		[CLK_R_MBOX]			= &r_mbox_clk.common.hw,
		[CLK_R_UART1]			= &r_uart1_clk.common.hw,
		[CLK_R_UART0]			= &r_uart0_clk.common.hw,
		[CLK_R_TWI1]			= &r_twi1_clk.common.hw,
		[CLK_R_TWI0]			= &r_twi0_clk.common.hw,
		[CLK_R_PPU]			= &r_ppu_clk.common.hw,
		[CLK_R_TZMA]			= &r_tzma_clk.common.hw,
		[CLK_R_CPUS_BIST]		= &r_cpus_bist_clk.common.hw,
		[CLK_R_IRRX]			= &r_irrx_clk.common.hw,
		[CLK_R_BUS_IRRX]		= &r_irrx_bus_clk.common.hw,
		[CLK_RTC]			= &rtc_clk.common.hw,
		[CLK_R_CPUCFG]			= &r_cpucfg_clk.common.hw,
		[CLK_VDD_USB2CPUS]		= &vdd_usb2cpus_clk.common.hw,
		[CLK_VDD_SYS2CPUS]		= &vdd_sys2cpus_clk.common.hw,
		[CLK_VDD_DDR]			= &vdd_ddr_clk.common.hw,
		[CLK_TT_AUTO]			= &tt_auto_clk.common.hw,
		[CLK_CPU_ICACHE_AUTO]		= &cpu_icache_auto_clk.common.hw,
		[CLK_AHBS_AUTO_CLK]		= &ahbs_auto_clk__clk.common.hw,
	},
	.num = CLK_R_NUMBER,
};
/* ccu_def_end */

static struct ccu_common *sun55iw6_r_ccu_clks[] = {
	&r_timer0_clk.common,
	&r_timer1_clk.common,
	&r_timer2_clk.common,
	&r_timer3_clk.common,
	&r_timer_clk.common,
	&r_twd_clk.common,
	&r_pwm_clk.common,
	&r_pwm_bus_clk.common,
	&r_spi_clk.common,
	&r_spi_bus_clk.common,
	&r_mbox_clk.common,
	&r_uart1_clk.common,
	&r_uart0_clk.common,
	&r_twi1_clk.common,
	&r_twi0_clk.common,
	&r_ppu_clk.common,
	&r_tzma_clk.common,
	&r_cpus_bist_clk.common,
	&r_irrx_clk.common,
	&r_irrx_bus_clk.common,
	&rtc_clk.common,
	&r_cpucfg_clk.common,
	&vdd_usb2cpus_clk.common,
	&vdd_sys2cpus_clk.common,
	&vdd_ddr_clk.common,
	&tt_auto_clk.common,
	&cpu_icache_auto_clk.common,
	&ahbs_auto_clk__clk.common,
};

static const struct sunxi_ccu_desc sun55iw6_r_ccu_desc = {
	.ccu_clks	= sun55iw6_r_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun55iw6_r_ccu_clks),

	.hw_clks	= &sun55iw6_r_hw_clks,

	.resets		= sun55iw6_r_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun55iw6_r_ccu_resets),
};

static int sun55iw6_r_ccu_probe(struct platform_device *pdev)
{
	void __iomem *reg;
	int ret;

	reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	ret = sunxi_ccu_probe(pdev->dev.of_node, reg, &sun55iw6_r_ccu_desc);
	if (ret)
		return ret;

	sunxi_ccu_sleep_init(reg, sun55iw6_r_ccu_clks,
			ARRAY_SIZE(sun55iw6_r_ccu_clks),
			NULL, 0);

	return 0;
}

static const struct of_device_id sun55iw6_r_ccu_ids[] = {
	{ .compatible = "allwinner,sun55iw6-r-ccu" },
	{ }
};

static struct platform_driver sun55iw6_r_ccu_driver = {
	.probe	= sun55iw6_r_ccu_probe,
	.driver	= {
		.name	= "sun55iw6-r-ccu",
		.of_match_table	= sun55iw6_r_ccu_ids,
	},
};

static int __init sunxi_ccu_sun55iw6_r_init(void)
{
	int ret;

	ret = platform_driver_register(&sun55iw6_r_ccu_driver);
	if (ret)
		pr_err("register ccu sun55iw6-r failed\n");

	return ret;
}
core_initcall(sunxi_ccu_sun55iw6_r_init);

static void __exit sunxi_ccu_sun55iw6_r_exit(void)
{
	return platform_driver_unregister(&sun55iw6_r_ccu_driver);
}
module_exit(sunxi_ccu_sun55iw6_r_exit);

MODULE_DESCRIPTION("Allwinner sun55iw6-r clk driver");
MODULE_AUTHOR("panzhijian<panzhijian@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.1.0");
