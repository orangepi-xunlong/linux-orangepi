// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2022 liujuan1@allwinnertech.com
 */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/iopoll.h>
#include <linux/syscore_ops.h>
#include <linux/slab.h>
#include <dt-bindings/clock/sun55iw3-displl-ccu.h>

#include "ccu_common.h"
#include "ccu_reset.h"

#include "ccu_nm.h"
#include "ccu_nkmp.h"

#define SUN55IW3_PLL_CPU0_REG	0x0000
#define SUN55IW3_PLL_CPU1_REG	0x0004
#define SUN55IW3_PLL_CPU2_REG   0x0008
#define SUN55IW3_PLL_CPU3_REG   0x000c

#define SUN55IW3_PLL_CPU1_SSC_REG   0x0054
#define SUN55IW3_PLL_CPU2_SSC_REG   0x0058
#define SUN55IW3_PLL_CPU3_SSC_REG   0x005c

static struct ccu_mult pll_cpu0_clk = {
	.enable		= BIT(27) | BIT(29) | BIT(30) | BIT(31),
	.lock		= BIT(28),
	.mult		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.common		= {
		.reg		= 0x0000,
		.hw.init	= CLK_HW_INIT("pll-cpu0", "dcxo24M",
				&ccu_mult_ops,
				CLK_SET_RATE_UNGATE),
	},
};

static struct ccu_nkmp pll_cpu1_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_OFFSET_MIN_MAX(8, 8, 0, 20, 108), /* form shc */
	.p		= _SUNXI_CCU_DIV(16, 2), /* P in CPUA reg */
	.p_reg		= 0x0060,
	.common		= {
		.reg		= 0x0004,
		.ssc_reg	= 0x54,
		.clear		= BIT(26),
		.features	= CCU_FEATURE_CLEAR_MOD | CCU_FEATURE_CLAC_CACHED | CCU_FEATURE_TYPE_NKMP,
		.hw.init	= CLK_HW_INIT("pll-cpu1", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_GET_RATE_NOCACHE | CLK_IS_CRITICAL \
				| CLK_SET_RATE_UNGATE),
	},
};

static struct ccu_nkmp pll_cpu2_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_OFFSET_MIN_MAX(8, 8, 0, 12, 0),
	.m		= _SUNXI_CCU_DIV(16, 4), /* output divider */
	.p		= _SUNXI_CCU_DIV(0, 1), /* input divider */
	.common		= {
		.reg		= 0x0008,
		.ssc_reg	= 0x58,
		.clear		= BIT(26),
		.features	= CCU_FEATURE_CLEAR_MOD | CCU_FEATURE_CLAC_CACHED | CCU_FEATURE_TYPE_NKMP,
		.hw.init	= CLK_HW_INIT("pll-cpu2", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_GET_RATE_NOCACHE | CLK_IS_CRITICAL \
				| CLK_SET_RATE_UNGATE),
	},
};

static struct ccu_nkmp pll_cpu3_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_OFFSET_MIN_MAX(8, 8, 0, 20, 108),
	.p		= _SUNXI_CCU_DIV(16, 2), /* P in CPUB reg */
	.p_reg		= 0x0064,
	.common		= {
		.reg		= 0x000c,
		.ssc_reg	= 0x5c,
		.clear		= BIT(26),
		.features	= CCU_FEATURE_CLEAR_MOD | CCU_FEATURE_CLAC_CACHED | CCU_FEATURE_TYPE_NKMP,
		.hw.init	= CLK_HW_INIT("pll-cpu3", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_GET_RATE_NOCACHE | CLK_IS_CRITICAL \
				| CLK_SET_RATE_UNGATE),
	},
};

#define SUN55IW3_CPUA_REG	0x0060
#define SUN55IW3_CPUB_REG	0x0064
#define SUN55IW3_DSU_REG	0x006c

static const char * const cpua_parents[] = { "dcxo24M", "osc32k", "iosc", "pll-cpu1", "pll-peri0-600m", "pll-cpu0" };

static SUNXI_CCU_MUX(cpua_clk, "cpua", cpua_parents,
		     0x0060, 24, 3, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);

static const char * const cpub_parents[] = { "dcxo24M", "osc32k", "iosc", "pll-cpu3", "pll-peri0-600m", "pll-cpu0" };

static SUNXI_CCU_MUX(cpub_clk, "cpub", cpub_parents,
		     0x0064, 24, 3, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);

static struct ccu_common *sunxi_pll_cpu_clks[] = {
	&pll_cpu0_clk.common,
	&pll_cpu1_clk.common,
	&pll_cpu2_clk.common,
	&pll_cpu3_clk.common,
	&cpua_clk.common,
	&cpub_clk.common,
};

static struct clk_hw_onecell_data sunxi_cpupll_hw_clks = {
	.hws	= {
		[CLK_PLL_CPU0]		= &pll_cpu0_clk.common.hw,
		[CLK_PLL_CPU1]		= &pll_cpu1_clk.common.hw,
		[CLK_PLL_CPU2]		= &pll_cpu2_clk.common.hw,
		[CLK_PLL_CPU3]		= &pll_cpu3_clk.common.hw,
		[CLK_PLL_CPUA]		= &cpua_clk.common.hw,
		[CLK_PLL_CPUB]		= &cpub_clk.common.hw,
	},
	.num = CLK_DISPLL_MAX_NO,
};

static const struct sunxi_ccu_desc cpupll_desc = {
	.ccu_clks	= sunxi_pll_cpu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sunxi_pll_cpu_clks),
	.hw_clks	= &sunxi_cpupll_hw_clks,
	.resets		= NULL,
	.num_resets	= 0,
};

static inline unsigned int calc_pll_ssc(unsigned int step, unsigned int scale,
		unsigned int ssc)
{
	return (unsigned int)(((1 << 17) * ssc) - (scale * (1 << step))) / scale;
}

static const u32 sun55iw3_pll_cpu_regs[] = {
	SUN55IW3_PLL_CPU1_REG,
	SUN55IW3_PLL_CPU2_REG,
	SUN55IW3_PLL_CPU3_REG,
};

static const u32 sun55iw3_pll_cpu_ssc_regs[] = {
	SUN55IW3_PLL_CPU1_SSC_REG,
	SUN55IW3_PLL_CPU2_SSC_REG,
	SUN55IW3_PLL_CPU3_SSC_REG,
};

static void ccupll_helper_wait_for_lock(void __iomem *addr, u32 lock)
{
	u32 reg;

	WARN_ON(readl_relaxed_poll_timeout(addr, reg, reg & lock, 100, 70000));
}

static void cpupll_helper_wait_for_clear(void __iomem *addr, u32 clear)
{
	u32 reg;

	reg = readl(addr);
	writel(reg | clear, addr);

	WARN_ON(readl_relaxed_poll_timeout_atomic(addr, reg, !(reg & clear), 100, 10000));
}

static int cpupll_notifier_cb(struct notifier_block *nb,
				unsigned long event, void *data)
{
	struct ccu_pll_nb *pll = to_ccu_pll_nb(nb);
	int ret = 0;

	if (event == PRE_RATE_CHANGE) {
		/* Enable ssc function */
		set_reg(pll->common->base + pll->common->ssc_reg, 1, 1, pll->enable);
	} else if (event == POST_RATE_CHANGE) {
		/* Disable ssc function */
		set_reg(pll->common->base + pll->common->ssc_reg, 0, 1, pll->enable);
	}

	ccu_helper_wait_for_clear(pll->common, pll->common->clear);

	return notifier_from_errno(ret);
}

static struct ccu_pll_nb cpupll1_nb = {
	.common = &pll_cpu1_clk.common,
	.enable = BIT(31), /* switch ssc mode */
	.clk_nb = {
		.notifier_call = cpupll_notifier_cb,
	},
};

static struct ccu_pll_nb cpupll2_nb = {
	.common = &pll_cpu2_clk.common,
	.enable = BIT(31),
	.clk_nb = {
		.notifier_call = cpupll_notifier_cb,
	},
};

static struct ccu_pll_nb cpupll3_nb = {
	.common = &pll_cpu3_clk.common,
	.enable = BIT(31),
	.clk_nb = {
		.notifier_call = cpupll_notifier_cb,
	},
};

static int sun55iw3_cpupll_probe(struct platform_device *pdev)
{
	void __iomem *reg;
	u32 val;
	int i;
	unsigned int step = 0, scale = 0, ssc = 0;
	struct device_node *np = pdev->dev.of_node;

	reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	if (of_property_read_u32(np, "pll_step", &step))
		step = 0x9;

	if (of_property_read_u32(np, "pll_ssc_scale", &scale))
		scale = 0xa;

	if (of_property_read_u32(np, "pll_ssc", &ssc))
		ssc = 0x1;
	/* TODO: assume boot use the cpupll */
	for (i = 0; i < ARRAY_SIZE(sun55iw3_pll_cpu_ssc_regs); i++) {
		/*
		 * 1. Config n,m1,m0,p: default:480M
		 * 2. Enable pll_en pll_ldo_en lock_en pll_output
		 * 3. wait for update and lock
		 */
		val = readl(reg + sun55iw3_pll_cpu_regs[i]);
		val |= BIT(27) | BIT(29) | BIT(30) | BIT(31);
		set_reg(reg + sun55iw3_pll_cpu_regs[i], val, 32, 0);

		cpupll_helper_wait_for_clear(reg + sun55iw3_pll_cpu_regs[i], BIT(26));
		ccupll_helper_wait_for_lock(reg + sun55iw3_pll_cpu_regs[i], BIT(28));

		/*
		 * set ssc/step in ssc reg
		 */
		val = 0;
		ssc = calc_pll_ssc(step, scale, ssc);
		val = (ssc << 12 | step << 0);
		set_reg(reg + sun55iw3_pll_cpu_ssc_regs[i], val, 29, 0);

		cpupll_helper_wait_for_clear(reg + sun55iw3_pll_cpu_regs[i], BIT(26));
	}

	/* set default pll to cpu */
	set_reg(reg + SUN55IW3_CPUA_REG, 0x3, 3, 24);
	set_reg(reg + SUN55IW3_CPUB_REG, 0x3, 3, 24);

	/* Keep off cpu0 during startup */
	set_reg(reg + SUN55IW3_PLL_CPU0_REG, 0x0, 1, 30);

	sunxi_ccu_probe(pdev->dev.of_node, reg, &cpupll_desc);

	ccu_pll_notifier_register(&cpupll1_nb);
	ccu_pll_notifier_register(&cpupll2_nb);
	ccu_pll_notifier_register(&cpupll3_nb);

	dev_info(&pdev->dev, "Sunxi pll_cpu clk init OK\n");

	return 0;
}

static const struct of_device_id sun55iw3_cpupll_ids[] = {
	{ .compatible = "allwinner,sun55iw3-cpupll" },
	{ }
};

static struct platform_driver sun55iw3_cpupll_driver = {
	.probe	= sun55iw3_cpupll_probe,
	.driver	= {
		.name	= "sun55iw3-cpupll",
		.of_match_table	= sun55iw3_cpupll_ids,
	},
};

static int __init sunxi_ccu_cpupll_init(void)
{
	int ret;

	ret = platform_driver_register(&sun55iw3_cpupll_driver);
	if (ret)
		pr_err("register ccu sun55iw3 cpupll failed\n");

	return ret;
}
core_initcall(sunxi_ccu_cpupll_init);

static void __exit sunxi_ccu_cpupll_exit(void)
{
	return platform_driver_unregister(&sun55iw3_cpupll_driver);
}
module_exit(sunxi_ccu_cpupll_exit);

MODULE_DESCRIPTION("Allwinner sun55iw3 cpupll clk driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.0.8");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
