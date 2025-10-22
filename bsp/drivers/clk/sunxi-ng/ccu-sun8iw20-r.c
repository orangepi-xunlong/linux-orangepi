// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2020 huangzhenwei@allwinnertech.com
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

#include "ccu-sun8iw20-r.h"

static const char * const ahbs_apbs0_parents[] = { "dcxo24M", "osc32k",
						   "iosc", "pll-periph0-div3" };
static SUNXI_CCU_MP_WITH_MUX(r_ahb_clk, "r-ahb",
			     ahbs_apbs0_parents, 0x000,
			     0, 5,
			     8, 2,
			     24, 3,
			     0);

static SUNXI_CCU_MP_WITH_MUX(r_apb0_clk, "r-apb0",
			     ahbs_apbs0_parents, 0x00c,
			     0, 5,
			     8, 2,
			     24, 3,
			     0);

static SUNXI_CCU_GATE(r_apb0_timer_clk, "r-apb0-timer", "r-apb0",
		      0x11c, BIT(0), 0);

static SUNXI_CCU_GATE(r_apb0_twd_clk, "r-apb0-twd", "r-apb0",
		      0x12c, BIT(0), 0);

static SUNXI_CCU_GATE(r_ppu_clk, "r-ppu", "r-apb0",
		      0x1ac, BIT(0), 0);

static const char * const r_apb0_ir_rx_parents[] = { "osc32k", "dcxo24M" };
static SUNXI_CCU_MP_WITH_MUX_GATE(r_apb0_ir_rx_clk, "r-apb0-ir-rx",
				  r_apb0_ir_rx_parents, 0x1c0,
				  0, 5,		/* M */
				  8, 2,		/* P */
				  24, 2,	/* mux */
				  BIT(31),	/* gate */
				  0);

static SUNXI_CCU_GATE(r_apb0_bus_ir_rx_clk, "r-apb0-bus-ir-rx", "r-apb0",
		      0x1cc, BIT(0), 0);

static SUNXI_CCU_GATE(r_ahb_bus_rtc_clk, "r-ahb-rtc", "r-ahb",
		      0x20c, BIT(0), 0);

static SUNXI_CCU_GATE(r_apb0_cpucfg_clk, "r-apb0-cpucfg", "r-apb0",
		      0x22c, BIT(0), 0);

static struct ccu_common *sun8iw20_r_ccu_clks[] = {
	&r_ahb_clk.common,
	&r_apb0_clk.common,
	&r_apb0_timer_clk.common,
	&r_apb0_twd_clk.common,
	&r_ppu_clk.common,
	&r_apb0_ir_rx_clk.common,
	&r_apb0_bus_ir_rx_clk.common,
	&r_ahb_bus_rtc_clk.common,
	&r_apb0_cpucfg_clk.common,
};

static struct clk_hw_onecell_data sun8iw20_r_hw_clks = {
	.hws	= {
		[CLK_R_AHB]		= &r_ahb_clk.common.hw,
		[CLK_R_APB0]		= &r_apb0_clk.common.hw,
		[CLK_R_APB0_TIMER]	= &r_apb0_timer_clk.common.hw,
		[CLK_R_APB0_TWD]	= &r_apb0_twd_clk.common.hw,
		[CLK_R_PPU]		= &r_ppu_clk.common.hw,
		[CLK_R_APB0_IRRX]	= &r_apb0_ir_rx_clk.common.hw,
		[CLK_R_APB0_BUS_IRRX]	= &r_apb0_bus_ir_rx_clk.common.hw,
		[CLK_R_AHB_BUS_RTC]	= &r_ahb_bus_rtc_clk.common.hw,
		[CLK_R_APB0_CPUCFG]	= &r_apb0_cpucfg_clk.common.hw,
	},
	.num	= CLK_NUMBER,
};

static struct ccu_reset_map sun8iw20_r_ccu_resets[] = {
	[RST_R_APB0_TIMER]	=  { 0x11c, BIT(16) },
	[RST_R_APB0_TWD]	=  { 0x12c, BIT(16) },
	[RST_R_PPU]		=  { 0x1ac, BIT(16) },
	[RST_R_APB0_BUS_IRRX]	=  { 0x1cc, BIT(16) },
	[RST_R_AHB_BUS_RTC]	=  { 0x20c, BIT(16) },
	[RST_R_APB0_CPUCFG]	=  { 0x22c, BIT(16) },
};

static const struct sunxi_ccu_desc sun8iw20_r_ccu_desc = {
	.ccu_clks	= sun8iw20_r_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun8iw20_r_ccu_clks),

	.hw_clks	= &sun8iw20_r_hw_clks,

	.resets		= sun8iw20_r_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun8iw20_r_ccu_resets),
};

static int sun8iw20_r_ccu_probe(struct platform_device *pdev)
{
	void __iomem *reg;
	int ret;

	reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	ret = sunxi_ccu_probe(pdev->dev.of_node, reg, &sun8iw20_r_ccu_desc);
	if (ret)
		return ret;

	sunxi_ccu_sleep_init(reg, sun8iw20_r_ccu_clks,
			     ARRAY_SIZE(sun8iw20_r_ccu_clks),
			     NULL, 0);

	return 0;
}

static const struct of_device_id sun8iw20_r_ccu_ids[] = {
	{ .compatible = "allwinner,sun8iw20-r-ccu" },
	{ .compatible = "allwinner,sun20iw1-r-ccu" },
	{ }
};

static struct platform_driver sun8iw20_r_ccu_driver = {
	.probe	= sun8iw20_r_ccu_probe,
	.driver	= {
		.name	= "sun8iw20-r-ccu",
		.of_match_table	= sun8iw20_r_ccu_ids,
	},
};

static int __init sunxi_r_ccu_sun8iw20_init(void)
{
	int ret;

	ret = platform_driver_register(&sun8iw20_r_ccu_driver);
	if (ret)
		pr_err("register ccu sun8iw20 failed\n");

	return ret;
}
core_initcall(sunxi_r_ccu_sun8iw20_init);

static void __exit sunxi_r_ccu_sun8iw20_exit(void)
{
	return platform_driver_unregister(&sun8iw20_r_ccu_driver);
}
module_exit(sunxi_r_ccu_sun8iw20_exit);

MODULE_VERSION("0.5.1");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
