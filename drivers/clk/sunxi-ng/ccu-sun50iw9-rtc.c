// SPDX-License-Identifier: GPL-2.0-only
/*
 * sunxi RTC ccu driver
 *
 * Copyright (c) 2020, DaLv <lvda@allwinnertech.com>
 */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/module.h>

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
#include "ccu_phase.h"

#include "ccu-sun50iw9-rtc.h"

/*
 * clock source:
 *  iosc---16M_RC: used by the cpu system directly.
 *  dcxo---24M Crystal: used by the cpu system directly.
 *  osc32k---to make it easier, force it to be a fixed clock source:open auto-switch,
 *  use the 32768 EXT Crystal as default
 *
 * the 3 clock source can be treated as the fixed rate clock.
 * They should be descripted in the dts, being parsed by
 * clk-fixed-rate.c
 */

static CLK_FIXED_FACTOR_FW_NAME(rtc_1k_clk, "rtc-1k", "losc", 32, 1, 0);

/* pll-periph0-2x-32k, real source is pll-periph0-2x */
static CLK_FIXED_FACTOR_FW_NAME(pll_periph0_2x_32k_clk, "pll-periph0-2x-32k", "losc", 1, 1, 0);

static CLK_FIXED_FACTOR_FW_NAME(dcxo_32k_clk, "dcxo-32k", "dcxo24M", 750, 1, 0);

static SUNXI_CCU_GATE(dcxo_32k_out_clk, "dcxo-32k-out", "dcxo-32k", 0x60, BIT(16), 0);

static const char * const osc32k_out_parents[] = { "osc32k", "pll-periph0-2x-32k", "dcxo-32k-out"};

static SUNXI_CCU_MUX_WITH_GATE(osc32k_out_clk, "osc32k-out", osc32k_out_parents,
			       0x60, 1, 2, BIT(0), 0);

static struct ccu_common *sun50iw9_rtc_ccu_clks[] = {
	&osc32k_out_clk.common,
	&dcxo_32k_out_clk.common,
};

static struct clk_hw_onecell_data sun50iw9_rtc_ccu_hw_clks = {
	.hws	= {
		[CLK_RTC_1K]			= &rtc_1k_clk.hw,
		[CLK_PLL_PERIPHO_2X_32K]	= &pll_periph0_2x_32k_clk.hw,
		[CLK_DCXO_32K]			= &dcxo_32k_clk.hw,
		[CLK_DCXO_32K_OUT]		= &dcxo_32k_out_clk.common.hw,
		[CLK_OSC32K_OUT]		= &osc32k_out_clk.common.hw,
	},
	.num	= CLK_NUMBER,
};

static const struct sunxi_ccu_desc sun50iw9_rtc_ccu_desc = {
	.ccu_clks	= sun50iw9_rtc_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun50iw9_rtc_ccu_clks),

	.hw_clks	= &sun50iw9_rtc_ccu_hw_clks,
};

static void clock_source_init(char __iomem *base)
{
	/* (1) enable DCXO */
	/* by default, DCXO_EN = 1. We don't have to do this... */
	set_reg(base + XO_CTRL_REG, 0x1, 1, 0);

	/* (2) enable calibrated RC-16M, and switch to it */
	/* set WAKEUP_DCXO_EN is 0 */
	set_reg(base + CALI_CTRL_REG, 0x0, 1, 31);
	/*
	 * open Calibrated RC and select it
	 * BIT(0)   0: Normal RC; 1: Calibrated RC
	 * BIT(1)   0: disable; 1: enable
	 */
	set_reg(base + INTOSC_CLK_AUTO_CALI_REG, 0x3, 2, 0);

	/* (3) enable auto switch function */
	/*
	 * In some cases, we boot with auto switch function disabled, and try to
	 * enable the auto switch function by rebooting.
	 * But the rtc default value does not change unless vcc-rtc is loss.
	 * So we should not rely on the default value of reg.
	 * BIT(14): LOSC auto switch 32k clk source sel enable. 1: enable
	 * BIT(15): LOSC auto switch function disable. 1: disable
	 */
	//set_reg(base + LOSC_CTRL_REG, 0x16aa3, 20, 12);
	set_reg_key(base + LOSC_CTRL_REG,
		    KEY_FIELD_MAGIC, 16, 16,
		    0x1, 2, 14);

	/* (4) set the parent of osc32k-sys to ext-osc32k */
	set_reg_key(base + LOSC_CTRL_REG,
		    0x16aa, 16, 16,
		    0x1, 1, 0);
	/*
	 * the 32K fanout has been set in the clk tree
	 * this part can be deleted
	 */
	/* (5) set the parent of osc32k-out to osc32k-sys*/
	/* by default, LOSC_OUT_SRC_SEL = 0x0. We don't have to do this... */
	set_reg(base + LOSC_OUT_GATING_REG,
		0x0, 2, 1);
}

static int sun50iw9_rtc_ccu_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct device *dev = &pdev->dev;
	void __iomem *reg;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "Fail to get IORESOURCE_MEM\n");
		return -EINVAL;
	}

	/*
	 * Don't use devm_ioremap_resource() here! Or else the RTC driver will
	 * not able to get the same resource later in rtc-sunxi.c.
	 */
	reg = devm_ioremap(dev, res->start, resource_size(res));
	if (IS_ERR(reg)) {
		dev_err(dev, "Fail to map IO resource\n");
		return PTR_ERR(reg);
	}

	clock_source_init(reg);

	return sunxi_ccu_probe(pdev->dev.of_node, reg, &sun50iw9_rtc_ccu_desc);
}

static const struct of_device_id sun50iw9_rtc_ccu_ids[] = {
	{ .compatible = "allwinner,sun50iw9-rtc-ccu" },
	{ }
};

static struct platform_driver sun50iw9_rtc_ccu_driver = {
	.probe	= sun50iw9_rtc_ccu_probe,
	.driver	= {
		.name	= "sun50iw9-rtc-ccu",
		.of_match_table	= sun50iw9_rtc_ccu_ids,
	},
};
builtin_platform_driver(sun50iw9_rtc_ccu_driver);

MODULE_DESCRIPTION("sunxi RTC CCU driver");
MODULE_AUTHOR("Da Lv <lvda@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.1.0");
