// SPDX-License-Identifier: GPL-2.0
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

#include "ccu-sun50iw9-r.h"

static const char * const cpus_r_apb2_parents[] = { "dcxo24M", "osc32k",
						     "iosc", "pll-periph0" };
static const struct ccu_mux_var_prediv cpus_r_apb2_predivs[] = {
	{ .index = 3, .shift = 0, .width = 5 },
};

static struct ccu_div r_cpus_clk = {
	.div		= _SUNXI_CCU_DIV_FLAGS(8, 2, CLK_DIVIDER_POWER_OF_TWO),

	.mux		= {
		.shift	= 24,
		.width	= 2,

		.var_predivs	= cpus_r_apb2_predivs,
		.n_var_predivs	= ARRAY_SIZE(cpus_r_apb2_predivs),
	},

	.common		= {
		.reg		= 0x000,
		.features	= CCU_FEATURE_VARIABLE_PREDIV,
		.hw.init	= CLK_HW_INIT_PARENTS("cpus",
						      cpus_r_apb2_parents,
						      &ccu_div_ops,
						      0),
	},
};

static CLK_FIXED_FACTOR_HW(r_ahb_clk, "r-ahb", &r_cpus_clk.common.hw, 1, 1, 0);

static struct ccu_div r_apb1_clk = {
	.div		= _SUNXI_CCU_DIV(0, 2),

	.common		= {
		.reg		= 0x00c,
		.hw.init	= CLK_HW_INIT("r-apb1",
					      "r-ahb",
					      &ccu_div_ops,
					      0),
	},
};

static struct ccu_div r_apb2_clk = {
	.div		= _SUNXI_CCU_DIV_FLAGS(8, 2, CLK_DIVIDER_POWER_OF_TWO),

	.mux		= {
		.shift	= 24,
		.width	= 2,

		.var_predivs	= cpus_r_apb2_predivs,
		.n_var_predivs	= ARRAY_SIZE(cpus_r_apb2_predivs),
	},

	.common		= {
		.reg		= 0x010,
		.features	= CCU_FEATURE_VARIABLE_PREDIV,
		.hw.init	= CLK_HW_INIT_PARENTS("r-apb2",
						      cpus_r_apb2_parents,
						      &ccu_div_ops,
						      0),
	},
};

static SUNXI_CCU_GATE(r_apb1_twd_clk, "r-apb1-twd", "r-apb1",
		      0x12c, BIT(0), 0);

static SUNXI_CCU_GATE(r_apb2_i2c_clk, "r-apb2-i2c", "r-apb2",
		      0x19c, BIT(0), 0);

/* APB2? */

static const char * const r_apb1_ir_rx_parents[] = { "osc32k", "dcxo24M" };
static SUNXI_CCU_MP_WITH_MUX_GATE(r_apb1_ir_rx_clk, "r-apb1-ir-rx",
				  r_apb1_ir_rx_parents, 0x1c0,
				  0, 5,		/* M */
				  8, 2,		/* P */
				  24, 1,	/* mux */
				  BIT(31),	/* gate */
				  0);

static SUNXI_CCU_GATE(r_apb1_bus_ir_rx_clk, "r-apb1-bus-ir-rx", "r-apb1",
		      0x1cc, BIT(0), 0);

static SUNXI_CCU_GATE(r_ahb_bus_rtc_clk, "r-ahb-rtc", "r-ahb",
		      0x20c, BIT(0), 0);

static struct ccu_common *sun50iw9_r_ccu_clks[] = {
	&r_cpus_clk.common,
	&r_apb1_clk.common,
	&r_apb2_clk.common,
	&r_apb1_twd_clk.common,
	&r_apb2_i2c_clk.common,
	&r_apb1_ir_rx_clk.common,
	&r_apb1_bus_ir_rx_clk.common,
	&r_ahb_bus_rtc_clk.common,
};

static struct clk_hw_onecell_data sun50iw9_r_hw_clks = {
	.hws	= {
		[CLK_R_CPUS]		= &r_cpus_clk.common.hw,
		[CLK_R_AHB]		= &r_ahb_clk.hw,
		[CLK_R_APB1]		= &r_apb1_clk.common.hw,
		[CLK_R_APB2]		= &r_apb2_clk.common.hw,
		[CLK_R_APB1_TWD]	= &r_apb1_twd_clk.common.hw,
		[CLK_R_APB2_I2C]	= &r_apb2_i2c_clk.common.hw,
		[CLK_R_APB1_IR]		= &r_apb1_ir_rx_clk.common.hw,
		[CLK_R_APB1_BUS_IR]	= &r_apb1_bus_ir_rx_clk.common.hw,
		[CLK_R_AHB_BUS_RTC]	= &r_ahb_bus_rtc_clk.common.hw,
	},
	.num	= CLK_NUMBER,
};

static struct ccu_reset_map sun50iw9_r_ccu_resets[] = {
	[RST_R_APB1_TWD]	=  { 0x12c, BIT(16) },
	[RST_R_APB2_I2C]	=  { 0x19c, BIT(16) },
	[RST_R_APB1_BUS_IR]	=  { 0x1cc, BIT(16) },
};

static const struct sunxi_ccu_desc sun50iw9_r_ccu_desc = {
	.ccu_clks	= sun50iw9_r_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun50iw9_r_ccu_clks),

	.hw_clks	= &sun50iw9_r_hw_clks,

	.resets		= sun50iw9_r_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun50iw9_r_ccu_resets),
};

static int sun50iw9_r_ccu_probe(struct platform_device *pdev)
{
	void __iomem *reg;
	int ret;

	reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	ret = sunxi_ccu_probe(pdev->dev.of_node, reg, &sun50iw9_r_ccu_desc);
	if (ret)
		return ret;

	sunxi_ccu_sleep_init(reg, sun50iw9_r_ccu_clks,
			     ARRAY_SIZE(sun50iw9_r_ccu_clks),
			     NULL, 0);

	printk("Sunxi ccu sun50iw9-r init OK\n");

	return 0;
}

static const struct of_device_id sun50iw9_r_ccu_ids[] = {
	{ .compatible = "allwinner,sun50iw9-r-ccu" },
	{ }
};

static struct platform_driver sun50iw9_r_ccu_driver = {
	.probe	= sun50iw9_r_ccu_probe,
	.driver	= {
		.name	= "sun50iw9-r-ccu",
		.of_match_table	= sun50iw9_r_ccu_ids,
	},
};

static int __init sunxi_ccu_sun50iw9_r_init(void)
{
	return platform_driver_register(&sun50iw9_r_ccu_driver);
}
core_initcall(sunxi_ccu_sun50iw9_r_init);

MODULE_DESCRIPTION("Allwinner sun50iw9-r clk driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.5");
