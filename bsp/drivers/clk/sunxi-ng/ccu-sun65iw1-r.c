// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 rengaomin@allwinnertech.com
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

#include "ccu-sun65iw1-r.h"

/* ccu_des_start */

static const char * const r_timer0_parents[] = { "dcxo", "rtc-32k", "rc16m", "peripll-div" };

static SUNXI_CCU_M_WITH_MUX_GATE(r_timer0_clk, "r-timer0",
		r_timer0_parents, 0x0100,
		1, 3,	/* M */
		4, 3,	/* mux */
		BIT(0),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const r_timer1_parents[] = { "dcxo", "rtc-32k", "rc16m", "peripll-div" };

static SUNXI_CCU_M_WITH_MUX_GATE(r_timer1_clk, "r-timer1",
		r_timer1_parents, 0x0104,
		1, 3,	/* M */
		4, 3,	/* mux */
		BIT(0),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const r_timer2_parents[] = { "dcxo", "rtc-32k", "rc16m", "peripll-div" };

static SUNXI_CCU_M_WITH_MUX_GATE(r_timer2_clk, "r-timer2",
		r_timer2_parents, 0x0108,
		1, 3,	/* M */
		4, 3,	/* mux */
		BIT(0),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const r_timer3_parents[] = { "dcxo", "rtc-32k", "rc16m", "peripll-div" };

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

static const char * const r_pwm_parents[] = { "dcxo", "rtc-32k", "rc16m" };

static SUNXI_CCU_MUX_WITH_GATE(r_pwm_clk, "r-pwm",
		r_pwm_parents, 0x0130,
		24, 2,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(r_bus_pwm_clk, "r-bus-pwm",
		"dcxo24M",
		0x013C, BIT(0), 0);

static const char * const r_spi_parents[] = { "dcxo", "peripll-div", "peri0-300m", "peri1-300m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(r_spi_clk, "r-spi",
		r_spi_parents, 0x0150,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(r_bus_spi_clk, "r-bus-spi",
		"dcxo24M",
		0x015C, BIT(0), 0);

static SUNXI_CCU_GATE(r_uart1_clk, "r-uart1",
		"dcxo24M",
		0x018C, BIT(1), 0);

static SUNXI_CCU_GATE(r_uart0_clk, "r-uart0",
		"dcxo24M",
		0x018C, BIT(0), 0);

static SUNXI_CCU_GATE(r_twi2_clk, "r-twi2",
		"dcxo24M",
		0x019C, BIT(2), 0);

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

static const char * const r_irrx_parents[] = { "rtc-32k", "dcxo", "11" };

static SUNXI_CCU_M_WITH_MUX_GATE(r_irrx_clk, "r-irrx",
		r_irrx_parents, 0x01C0,
		0, 5,	/* M */
		24, 2,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(r_bus_irrx_clk, "r-bus-irrx",
		"dcxo24M",
		0x01CC, BIT(0), 0);

static SUNXI_CCU_GATE(rtc_clk, "rtc",
		"dcxo24M",
		0x020C, BIT(0), 0);

static const char * const e902_24m_parents[] = { "hosc", "clk32k", "clk16m-rc" };

static SUNXI_CCU_MUX_WITH_GATE(e902_24m_clk, "e902-24m",
		e902_24m_parents, 0x0210,
		24, 2,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(e902_cfg_clk, "e902-cfg",
		"dcxo24M",
		0x021C, BIT(1), 0);

static SUNXI_CCU_GATE(e902_core_clk, "e902-core",
		"dcxo24M",
		0x021C, BIT(0), 0);

static SUNXI_CCU_GATE(r_cpucfg_clk, "r-cpucfg",
		"dcxo24M",
		0x022C, BIT(0), 0);
/* ccu_des_end */

/* rst_def_start */
static struct ccu_reset_map sun65iw1_r_ccu_resets[] = {
	[RST_BUS_R_TIME]		= { 0x011c, BIT(16) },
	[RST_BUS_R_PWM]			= { 0x013c, BIT(16) },
	[RST_BUS_R_SPI]			= { 0x015c, BIT(16) },
	[RST_BUS_R_UART1]		= { 0x018c, BIT(17) },
	[RST_BUS_R_UART0]		= { 0x018c, BIT(16) },
	[RST_BUS_R_TWI2]		= { 0x019c, BIT(18) },
	[RST_BUS_R_TWI1]		= { 0x019c, BIT(17) },
	[RST_BUS_R_TWI0]		= { 0x019c, BIT(16) },
	[RST_BUS_R_IRRX]		= { 0x01cc, BIT(16) },
	[RST_BUS_RTC]			= { 0x020c, BIT(16) },
	[RST_BUS_E902_CFG]		= { 0x021c, BIT(16) },
	[RST_BUS_R_CPUCFG]		= { 0x022c, BIT(16) },
	[RST_BUS_MODULE]		= { 0x0260, BIT(0) },
};
/* rst_def_end */

/* ccu_def_start */
static struct clk_hw_onecell_data sun65iw1_r_hw_clks = {
	.hws	= {
		[CLK_R_TIMER0]			= &r_timer0_clk.common.hw,
		[CLK_R_TIMER1]			= &r_timer1_clk.common.hw,
		[CLK_R_TIMER2]			= &r_timer2_clk.common.hw,
		[CLK_R_TIMER3]			= &r_timer3_clk.common.hw,
		[CLK_R_TIMER]			= &r_timer_clk.common.hw,
		[CLK_R_TWD]			= &r_twd_clk.common.hw,
		[CLK_R_PWM]			= &r_pwm_clk.common.hw,
		[CLK_BUS_R_PWM]			= &r_bus_pwm_clk.common.hw,
		[CLK_R_SPI]			= &r_spi_clk.common.hw,
		[CLK_BUS_R_SPI]			= &r_bus_spi_clk.common.hw,
		[CLK_R_UART1]			= &r_uart1_clk.common.hw,
		[CLK_R_UART0]			= &r_uart0_clk.common.hw,
		[CLK_R_TWI2]			= &r_twi2_clk.common.hw,
		[CLK_R_TWI1]			= &r_twi1_clk.common.hw,
		[CLK_R_TWI0]			= &r_twi0_clk.common.hw,
		[CLK_R_PPU]			= &r_ppu_clk.common.hw,
		[CLK_R_TZMA]			= &r_tzma_clk.common.hw,
		[CLK_R_IRRX]			= &r_irrx_clk.common.hw,
		[CLK_BUS_R_IRRX]		= &r_bus_irrx_clk.common.hw,
		[CLK_RTC]			= &rtc_clk.common.hw,
		[CLK_E902_24M]			= &e902_24m_clk.common.hw,
		[CLK_E902_CFG]			= &e902_cfg_clk.common.hw,
		[CLK_E902_CORE]			= &e902_core_clk.common.hw,
		[CLK_R_CPUCFG]			= &r_cpucfg_clk.common.hw,
	},
	.num	= CLK_R_NUMBER,
};

/* ccu_def_end */

static struct ccu_common *sun65iw1_r_ccu_clks[] = {
	&r_timer0_clk.common,
	&r_timer1_clk.common,
	&r_timer2_clk.common,
	&r_timer3_clk.common,
	&r_timer_clk.common,
	&r_twd_clk.common,
	&r_pwm_clk.common,
	&r_bus_pwm_clk.common,
	&r_spi_clk.common,
	&r_bus_spi_clk.common,
	&r_uart1_clk.common,
	&r_uart0_clk.common,
	&r_twi2_clk.common,
	&r_twi1_clk.common,
	&r_twi0_clk.common,
	&r_ppu_clk.common,
	&r_tzma_clk.common,
	&r_irrx_clk.common,
	&r_bus_irrx_clk.common,
	&rtc_clk.common,
	&e902_24m_clk.common,
	&e902_cfg_clk.common,
	&e902_core_clk.common,
	&r_cpucfg_clk.common,
};

static const struct sunxi_ccu_desc sun65iw1_r_ccu_desc = {
	.ccu_clks	= sun65iw1_r_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun65iw1_r_ccu_clks),

	.hw_clks	= &sun65iw1_r_hw_clks,

	.resets		= sun65iw1_r_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun65iw1_r_ccu_resets),
};

static int sun65iw1_r_ccu_probe(struct platform_device *pdev)
{
	void __iomem *reg;
	int ret;

	reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	ret = sunxi_ccu_probe(pdev->dev.of_node, reg, &sun65iw1_r_ccu_desc);
	if (ret)
		return ret;

	sunxi_ccu_sleep_init(reg, sun65iw1_r_ccu_clks,
			ARRAY_SIZE(sun65iw1_r_ccu_clks),
			NULL, 0);

	return 0;
}

static const struct of_device_id sun65iw1_r_ccu_ids[] = {
	{ .compatible = "allwinner,sun65iw1-r-ccu" },
	{ }
};

static struct platform_driver sun65iw1_r_ccu_driver = {
	.probe	= sun65iw1_r_ccu_probe,
	.driver	= {
		.name	= "sun65iw1-r-ccu",
		.of_match_table	= sun65iw1_r_ccu_ids,
	},
};

static int __init sunxi_ccu_sun65iw1_r_init(void)
{
	int ret;

	ret = platform_driver_register(&sun65iw1_r_ccu_driver);
	if (ret)
		pr_err("register ccu sun65iw1-r failed\n");

	return ret;
}
core_initcall(sunxi_ccu_sun65iw1_r_init);

static void __exit sunxi_ccu_sun65iw1_r_exit(void)
{
	return platform_driver_unregister(&sun65iw1_r_ccu_driver);
}
module_exit(sunxi_ccu_sun65iw1_r_exit);

MODULE_DESCRIPTION("Allwinner sun65iw1-r clk driver");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.0.1");
