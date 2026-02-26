// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * sunxi RTC ccu driver
 *
 * Copyright (c) 2021,<liujuan1@allwinnertech.com>
 */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/delay.h>

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

#include "ccu-sun55iw3-rtc.h"

/*
 * iosc clk:
 */
static SUNXI_CCU_GATE(dcxo24M_out_clk, "dcxo24M-out", "dcxo24M", 0x160, BIT(31), 0);
static SUNXI_CCU_GATE(iosc_clk, "iosc", "rc-16m", 0x160, BIT(0), 0);

static SUNXI_CCU_GATE_WITH_KEY(ext32k_gate_clk, "ext32k-gate",
			       "ext-32k", 0x0,
			       KEY_FIELD_MAGIC_NUM_RTC,
			       BIT(4), 0);

static CLK_FIXED_FACTOR(iosc_div32k_clk, "iosc-div32k", "iosc", 500, 1, 0);

/*
 * osc32k clk(losc)
 */
static const char * const osc32k_parents[] = { "iosc-div32k", "ext32k-gate" };
static SUNXI_CCU_MUX_WITH_GATE_KEY(osc32k_clk, "osc32k", osc32k_parents,
				   0x0, 0, 1,
				   KEY_FIELD_MAGIC_NUM_RTC, 0, 0);

static SUNXI_CCU_GATE_WITH_FIXED_RATE(dcxo24M_div32k_clk, "dcxo24M-div32k",
				      "dcxo24M", 0x60,
				      32768, BIT(16));
/*
 * rtc-1k clock
 */
static const char * const rtc32k_clk_parents[] = { "osc32k", "dcxo24M-div32k"};
static SUNXI_CCU_MUX_WITH_GATE_KEY(rtc32k_clk, "rtc32k", rtc32k_clk_parents,
				   0x0, 1, 1,
				   KEY_FIELD_MAGIC_NUM_RTC, 0, 0);
static CLK_FIXED_FACTOR(rtc_1k_clk, "rtc-1k", "rtc32k", 32, 1, 0);

/* rtc-32k-fanout: only for debug */
static const char * const rtc_32k_fanout_clk_parents[] = { "osc32k", "ext32k-gate",
							   "dcxo24M-div32k"};
static SUNXI_CCU_MUX_WITH_GATE(osc32k_out_clk, "osc32k-out",
			       rtc_32k_fanout_clk_parents, 0x60, 1,
			       2, BIT(0), 0);

/* TODO: should add the div func */
static SUNXI_CCU_GATE(rtc_spi_clk, "rtc-spi", "r-ahb", 0x310, BIT(31), 0);

static struct ccu_common *sun55iw3_rtc_ccu_clks[] = {
	&dcxo24M_out_clk.common,
	&iosc_clk.common,
	&ext32k_gate_clk.common,
	&osc32k_clk.common,
	&dcxo24M_div32k_clk.common,
	&rtc32k_clk.common,
	&osc32k_out_clk.common,
	&rtc_spi_clk.common,
};

static struct clk_hw_onecell_data sun55iw3_rtc_ccu_hw_clks = {
	.hws	= {
		[CLK_DCXO24M_OUT]		= &dcxo24M_out_clk.common.hw,
		[CLK_IOSC]			= &iosc_clk.common.hw,
		[CLK_EXT32K_GATE]		= &ext32k_gate_clk.common.hw,
		[CLK_IOSC_DIV32K]		= &iosc_div32k_clk.hw,
		[CLK_OSC32K]			= &osc32k_clk.common.hw,
		[CLK_DCXO24M_DIV32K]		= &dcxo24M_div32k_clk.common.hw,
		[CLK_RTC32K]			= &rtc32k_clk.common.hw,
		[CLK_RTC_1K]			= &rtc_1k_clk.hw,
		[CLK_OSC32K_OUT]		= &osc32k_out_clk.common.hw,
		[CLK_RTC_SPI]			= &rtc_spi_clk.common.hw,
	},
	.num	= CLK_NUMBER,
};

static const struct sunxi_ccu_desc sun55iw3_rtc_ccu_desc = {
	.ccu_clks	= sun55iw3_rtc_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun55iw3_rtc_ccu_clks),

	.hw_clks	= &sun55iw3_rtc_ccu_hw_clks,
};

static void clock_source_init(char __iomem *base)
{
	u32 val;
	u32 extern32k_enabled_mask = 0x10;

	/* (1) enable DCXO */
	/* by default, DCXO_EN = 1. We don't have to do this... */
	set_reg(base + XO_CTRL_REG, 0x1, 1, 1);

	/* (2) enable calibrated RC-16M, and switch to it */
	set_reg(base + CALI_CTRL_REG, 0x0, 1, 31);
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
	set_reg_key(base + LOSC_CTRL_REG,
		    KEY_FIELD_MAGIC_NUM_RTC >> 16, 16, 16,
		    0x1, 2, 14);

	/* To reduce boot time, this handled only for the external 32K enable bit not enabled */
	val = readl(base + LOSC_CTRL_REG);
	if (!(val & extern32k_enabled_mask)) {
		/* enable external 32K crystal */
		set_reg_key(base + LOSC_CTRL_REG,
			    KEY_FIELD_MAGIC_NUM_RTC >> 16, 16, 16,
			    0x1, 1, 4);
		/* sleep to wait enable 32k crystal ready */
		msleep(50);
		/* clear auto-switch pending */
		set_reg(base + LOSC_AUTO_SWT_STA_REG, 0x1, 1, 1);
		/* sleep to wait 32K crystal to normal */
		msleep(200);
	}
	/* (4) set the parent of osc32k-sys to ext-osc32k */
	set_reg_key(base + LOSC_CTRL_REG,
		    KEY_FIELD_MAGIC_NUM_RTC >> 16, 16, 16,
		    0x1, 1, 0);

	/* (5) set the parent of osc32k-out to osc32k-sys */
	/* by default, LOSC_OUT_SRC_SEL = 0x0. We don't have to do this... */
	set_reg(base + LOSC_OUT_GATING_REG,
		0x0, 2, 1);
}

static int sun55iw3_rtc_ccu_probe(struct platform_device *pdev)
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

	return sunxi_ccu_probe(pdev->dev.of_node, reg, &sun55iw3_rtc_ccu_desc);
}

static const struct of_device_id sun55iw3_rtc_ccu_ids[] = {
	{ .compatible = "allwinner,sun55iw3-rtc-ccu" },
	{ }
};

static struct platform_driver sun55iw3_rtc_ccu_driver = {
	.probe	= sun55iw3_rtc_ccu_probe,
	.driver	= {
		.name	= "sun55iw3-rtc-ccu",
		.of_match_table	= sun55iw3_rtc_ccu_ids,
	},
};

static int __init sun55iw3_rtc_ccu_init(void)
{
	int err;

	err = platform_driver_register(&sun55iw3_rtc_ccu_driver);
	if (err)
		pr_err("Fail to register sunxi_rtc_ccu as platform device\n");

	return err;
}
core_initcall(sun55iw3_rtc_ccu_init);

static void __exit sun55iw3_rtc_ccu_exit(void)
{
	platform_driver_unregister(&sun55iw3_rtc_ccu_driver);
}
module_exit(sun55iw3_rtc_ccu_exit);

MODULE_DESCRIPTION("sunxi RTC CCU driver");
MODULE_AUTHOR("Juan Liu<liujuan1@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.4");
