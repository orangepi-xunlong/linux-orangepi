// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * sunxi RTC ccu driver
 *
 * Copyright (c) 2023,<rengaomin@allwinnertech.com>
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

#include "ccu-sun60iw2-rtc.h"

#define  SUNXI_RTC_CCU_VERSION	"0.5.5"
/*
 * iosc clk:
 */
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
				      "dcxo", 0x60,
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
static const char * const rtc_32k_fanout_clk_parents[] = { "rtc32k", "osc32k", "dcxo24M-div32k"};
static SUNXI_CCU_MUX_WITH_GATE(rtc_32k_fanout_clk, "rtc-32k-fanout",
			       rtc_32k_fanout_clk_parents, 0x60, 1,
			       2, BIT(0), 0);

/* multi-oscillator scheme */
static const char * const dcxo_parents[] = { "dcxo24M", "dcxo19_2M", "dcxo26M", "dcxo24M" };
static SUNXI_CCU_MUX(dcxo_clk, "dcxo",
		 dcxo_parents, 0x160,
		14, 2,	/* mux */
		CLK_SET_RATE_NO_REPARENT);

static struct ccu_gate dcxo_wakeup_clk = {
	.enable	= BIT(31),
	.common	= {
		.reg		= 0x0160,
		.key_reg	= 0x015c,
		.key_value	= DCXO_WAKEUP_KEY_FIELD,
		.features	= CCU_FEATURE_KEY_FIELD_MOD | CCU_FEATURE_GATE_IS_REVERSE,
		.hw.init	= CLK_HW_INIT("dcxo-wakeup",
					"dcxo",
					&ccu_gate_ops,
					0),
		},
};

static SUNXI_CCU_GATE(dcxo_serdes1_clk, "dcxo-serdes1", "r-ahb", 0x16c, BIT(5), 0);

static SUNXI_CCU_GATE(dcxo_serdes0_clk, "dcxo-serdes0", "r-ahb", 0x16c, BIT(4), 0);
/* TODO: should add the div func */
static SUNXI_CCU_GATE(rtc_spi_clk, "rtc-spi", "r-ahb", 0x310, BIT(31), 0);

static struct ccu_common *sun60iw2_rtc_ccu_clks[] = {
	&iosc_clk.common,
	&ext32k_gate_clk.common,
	&osc32k_clk.common,
	&dcxo24M_div32k_clk.common,
	&rtc32k_clk.common,
	&rtc_32k_fanout_clk.common,
	&dcxo_wakeup_clk.common,
	&dcxo_serdes1_clk.common,
	&dcxo_serdes0_clk.common,
	&rtc_spi_clk.common,
	&dcxo_clk.common,
};

static struct clk_hw_onecell_data sun60iw2_rtc_ccu_hw_clks = {
	.hws	= {
		[CLK_IOSC]			= &iosc_clk.common.hw,
		[CLK_EXT32K_GATE]		= &ext32k_gate_clk.common.hw,
		[CLK_IOSC_DIV32K]		= &iosc_div32k_clk.hw,
		[CLK_OSC32K]			= &osc32k_clk.common.hw,
		[CLK_DCXO24M_DIV32K]		= &dcxo24M_div32k_clk.common.hw,
		[CLK_RTC32K]			= &rtc32k_clk.common.hw,
		[CLK_RTC_1K]			= &rtc_1k_clk.hw,
		[CLK_RTC_32K_FANOUT]		= &rtc_32k_fanout_clk.common.hw,
		[CLK_RTC_DCXO_WAKEUP]		= &dcxo_wakeup_clk.common.hw,
		[CLK_RTC_DCXO_SERDES1]		= &dcxo_serdes1_clk.common.hw,
		[CLK_RTC_DCXO_SERDES0]		= &dcxo_serdes0_clk.common.hw,
		[CLK_RTC_SPI]			= &rtc_spi_clk.common.hw,
		[CLK_DCXO]			= &dcxo_clk.common.hw,
	},
	.num	= CLK_NUMBER,
};

static const struct sunxi_ccu_desc sun60iw2_rtc_ccu_desc = {
	.ccu_clks	= sun60iw2_rtc_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun60iw2_rtc_ccu_clks),

	.hw_clks	= &sun60iw2_rtc_ccu_hw_clks,
};

static void clock_source_init(char __iomem *base)
{
	u32 val;
	u32 extern32k_enabled_mask = 0x10;
	u32 ext_losc_sta_mask = 0x10;
	__maybe_unused int read_times = 0;
	__maybe_unused int normal_times = 0;

	/* (1) enable DCXO */
	/* by default, DCXO_EN = 1. We don't have to do this... */
	set_reg(base + XO_CTRL_REG, 0x1, 1, 1);

	/* (2) enable auto switch function */
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

	/* (3) If Losc needs to be enabled, read sta_reg until it stabilizes */
	val = readl(base + LOSC_CTRL_REG);
	if (!(val & extern32k_enabled_mask)) {
		/* enable external 32K crystal */
		set_reg_key(base + LOSC_CTRL_REG,
				KEY_FIELD_MAGIC_NUM_RTC >> 16, 16, 16,
				0x1, 1, 4);

		while (read_times != 100 && normal_times != 4) {
			if ((readl(base + LOSC_AUTO_SWT_STA_REG) & ext_losc_sta_mask) == 0)
				normal_times += 1;
			else
				normal_times = 0;

			udelay(3);

			read_times += 1;
		}

		if (normal_times < 4)
			WARN(1, "losc not stable, 32K clk will use 16M as parent!\n");
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

static int sun60iw2_rtc_ccu_probe(struct platform_device *pdev)
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

	sunxi_info(NULL, "sunxi rtc-ccu version: %s\n", SUNXI_RTC_CCU_VERSION);

	return sunxi_ccu_probe(pdev->dev.of_node, reg, &sun60iw2_rtc_ccu_desc);
}

static const struct of_device_id sun60iw2_rtc_ccu_ids[] = {
	{ .compatible = "allwinner,sun60iw2-rtc-ccu" },
	{ }
};

static struct platform_driver sun60iw2_rtc_ccu_driver = {
	.probe	= sun60iw2_rtc_ccu_probe,
	.driver	= {
		.name	= "sun60iw2-ccu-rtc",
		.of_match_table	= sun60iw2_rtc_ccu_ids,
	},
};

static int __init sun60iw2_rtc_ccu_init(void)
{
	int err;

	err = platform_driver_register(&sun60iw2_rtc_ccu_driver);
	if (err)
		pr_err("register ccu sun60iw2 rtc failed\n");

	return err;
}

core_initcall(sun60iw2_rtc_ccu_init);

static void __exit sun60iw2_rtc_ccu_exit(void)
{
	platform_driver_unregister(&sun60iw2_rtc_ccu_driver);
}
module_exit(sun60iw2_rtc_ccu_exit);
MODULE_DESCRIPTION("sunxi RTC CCU driver");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SUNXI_RTC_CCU_VERSION);
