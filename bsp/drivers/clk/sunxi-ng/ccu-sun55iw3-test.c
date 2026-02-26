// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2022 lvda@allwinnertech.com
 */

#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include "clk-debugfs.h"
#include "ccu_common.h"
#include "ccu_reset.h"

#include "ccu_div.h"
#include "ccu_gate.h"
#include "ccu_mp.h"
#include "ccu_nm.h"

#define SUN55IW3_TEST_DEBUG_REG0		0x0
#define SUN55IW3_PLL_BACKDOOR_OUTPUT_ENABLE_REG	0x8

#define CLK_PLL_FANOUT_TEST			0
#define CLK_NUMBER				(CLK_PLL_FANOUT_TEST + 1)
static const char * const pll_fanout_test_parents[] = { "pll-video0-4x", "pll-video1-4x",
					     "pll-video2-4x", "pll-video3-4x", "pll-audio1-div5", "", "", "", "pll-gpu",
					     "pll-ve", "pll-audio0-4x", "pll-npu-2x",
					     "pll-peri0-800m", "pll-peri1-800m", "pll-ddr",
					     "dcxo24M", "", "", "", "", "pll-cpu0", "pll-cpu1", "pll-cpu2", "", "pll-cpu3"};

static SUNXI_CCU_MUX_WITH_GATE(pll_fanout_test, "pll-fanout-test",
			       pll_fanout_test_parents, 0x0,
			       24, 5,
			       BIT(31) | BIT(15),
			       0);

static struct ccu_common *sun55iw3_test_ccu_clks[] = {
	&pll_fanout_test.common,
};

static struct clk_hw_onecell_data sun55iw3_test_hw_clks = {
	.hws	= {
		[CLK_PLL_FANOUT_TEST] = &pll_fanout_test.common.hw,
	},
	.num	= CLK_NUMBER,
};

static const struct sunxi_ccu_desc sun55iw3_test_ccu_desc = {
	.ccu_clks	= sun55iw3_test_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun55iw3_test_ccu_clks),

	.hw_clks	= &sun55iw3_test_hw_clks,
};

static int sun55iw3_test_ccu_probe(struct platform_device *pdev)
{
	void __iomem *reg;
	int ret;

	reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	/*
	 * Set PLL_TEST_P_DIV = 8
	 */
	set_reg(reg + SUN55IW3_TEST_DEBUG_REG0, 0x3, 2, 20);
	/*
	 * Set DBG_CLK_DIV_FACTOR = 256
	 */
	set_reg(reg + SUN55IW3_TEST_DEBUG_REG0, 0x3, 2, 12);
	/*
	 * Enable all the pll backdoor
	 */
	set_reg(reg + SUN55IW3_PLL_BACKDOOR_OUTPUT_ENABLE_REG, 0xffff, 16, 0);

	ret = sunxi_ccu_probe(pdev->dev.of_node, reg, &sun55iw3_test_ccu_desc);

	clktest_register_rst(&pdev->dev);

	return ret;
}

static const struct of_device_id sun55iw3_test_ccu_ids[] = {
	{ .compatible = "allwinner,sun55iw3-test-ccu" },
	{ }
};

static struct platform_driver sun55iw3_test_ccu_driver = {
	.probe	= sun55iw3_test_ccu_probe,
	.driver	= {
		.name	= "sun55iw3-test-ccu",
		.of_match_table	= sun55iw3_test_ccu_ids,
	},
};

static int __init sunxi_test_ccu_sun55iw3_init(void)
{
	int ret;

	ret = platform_driver_register(&sun55iw3_test_ccu_driver);
	if (ret)
		pr_err("register ccu sun55iw3-test failed\n");

	return ret;
}
core_initcall(sunxi_test_ccu_sun55iw3_init);

static void __exit sunxi_test_ccu_sun55iw3_exit(void)
{
	return platform_driver_unregister(&sun55iw3_test_ccu_driver);
}
module_exit(sunxi_test_ccu_sun55iw3_exit);

MODULE_VERSION("1.0.3");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
