// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/iopoll.h>
#include <linux/syscore_ops.h>
#include <linux/slab.h>
#include <dt-bindings/clock/sun60iw2-cpupll-ccu.h>

#include "ccu_common.h"
#include "ccu_reset.h"
#include "ccu_nm.h"
#include "ccu_nkmp.h"

#define SUNXI_CPUPLL_CCU_VERSION		"0.0.5"

#define SUN60IW2_PLL_CPU_BACK_REG	0x0000
#define SUN60IW2_PLL_CPU_L_REG		0x1000
#define SUN60IW2_PLL_CPU_B_REG		0x2000
#define SUN60IW2_PLL_CPU_DSU_REG	0x3000

#define SUN60IW2_PLL_CPU_BACK_PAT0_REG	0x0008
#define SUN60IW2_PLL_CPU_L_PAT0_REG	0x1004
#define SUN60IW2_PLL_CPU_B_PAT0_REG	0x2004
#define SUN60IW2_PLL_CPU_DSU_PAT0_REG	0x3004

#define SUN60IW2_PLL_CPU_BACK_PAT1_REG	0x000c
#define SUN60IW2_PLL_CPU_L_PAT1_REG	0x1008
#define SUN60IW2_PLL_CPU_B_PAT1_REG	0x2008
#define SUN60IW2_PLL_CPU_DSU_PAT1_REG	0x3008

#define SUN60IW2_PLL_CPU_L_LFM_REG	0x1018
#define SUN60IW2_PLL_CPU_B_LFM_REG	0x2018
#define SUN60IW2_PLL_CPU_DSU_LFM_REG	0x3018

#define SUN60IW2_CPU_L_REG	0x101c
#define SUN60IW2_CPU_B_REG	0x201c
#define SUN60IW2_CPU_DSU_REG	0x301c

static struct ccu_nm pll_cpu_back_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_OFFSET_MIN_MAX(8, 8, 0, 53, 105),
	.m		= _SUNXI_CCU_DIV(0, 4),
	.min_rate	= 159000000,
	.max_rate	= 2520000000,
	.common		= {
		.reg		= 0x0000,
		.hw.init	= CLK_HW_INIT("pll-cpu-back", "dcxo",
				&ccu_nm_ops,
				CLK_GET_RATE_NOCACHE | CLK_IS_CRITICAL \
				| CLK_SET_RATE_UNGATE),
	},
};

static struct ccu_nkmp pll_cpu_l_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_OFFSET_MIN_MAX(8, 8, 0, 28, 94),
	.p		= _SUNXI_CCU_DIV(16, 2), /* P in cpu_clk reg */
	.p_reg		= 0x101c,
	.max_rate	= 2256000000,
	.common		= {
		.reg		= 0x1000,
		.clear		= BIT(26),
		.features	= CCU_FEATURE_CLEAR_MOD | CCU_FEATURE_CLAC_CACHED | CCU_FEATURE_TYPE_NKMP,
		.hw.init	= CLK_HW_INIT("pll-cpu-l", "dcxo",
				&ccu_nkmp_ops,
				CLK_GET_RATE_NOCACHE | CLK_IS_CRITICAL \
				| CLK_SET_RATE_UNGATE),
	},
};

static struct ccu_nkmp pll_cpu_b_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_OFFSET_MIN_MAX(8, 8, 0, 28, 94),
	.p		= _SUNXI_CCU_DIV(16, 2), /* P in cpu_clk reg */
	.p_reg		= 0x201c,
	.max_rate	= 2256000000,
	.common		= {
		.reg		= 0x2000,
		.clear		= BIT(26),
		.features	= CCU_FEATURE_CLEAR_MOD | CCU_FEATURE_CLAC_CACHED | CCU_FEATURE_TYPE_NKMP,
		.hw.init	= CLK_HW_INIT("pll-cpu-b", "dcxo",
				&ccu_nkmp_ops,
				CLK_GET_RATE_NOCACHE | CLK_IS_CRITICAL \
				| CLK_SET_RATE_UNGATE),
	},
};

static struct ccu_nkmp pll_cpu_dsu_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_OFFSET_MIN_MAX(8, 8, 0, 28, 94),
	.p		= _SUNXI_CCU_DIV(16, 2), /* P in cpu_clk reg */
	.p_reg		= 0x301c,
	.max_rate	= 2256000000,
	.common		= {
		.reg		= 0x3000,
		.clear		= BIT(26),
		.features	= CCU_FEATURE_CLEAR_MOD | CCU_FEATURE_CLAC_CACHED | CCU_FEATURE_TYPE_NKMP,
		.hw.init	= CLK_HW_INIT("pll-cpu-dsu", "dcxo",
				&ccu_nkmp_ops,
				CLK_GET_RATE_NOCACHE | CLK_IS_CRITICAL \
				| CLK_SET_RATE_UNGATE),
	},
};


static const char * const cpu_l_parents[] = { "dcxo", "dcxo", "dcxo", "pll-cpu-l", "pll-peri0-2x", "pll-cpu-back" };

static SUNXI_CCU_MUX(cpu_l_clk, "cpu_l", cpu_l_parents,
		     0x101c, 24, 3, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);

static const char * const cpu_b_parents[] = { "dcxo", "dcxo", "dcxo", "pll-cpu-b", "pll-peri0-2x", "pll-cpu-back" };

static SUNXI_CCU_MUX(cpu_b_clk, "cpu_b", cpu_b_parents,
		     0x201c, 24, 3, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);

static const char * const cpu_dsu_parents[] = { "dcxo", "dcxo", "dcxo", "pll-cpu-dsu", "pll-peri0-2x", "pll-cpu-back" };

static SUNXI_CCU_MUX(cpu_dsu_clk, "cpu_dsu", cpu_dsu_parents,
		     0x301c, 24, 3, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);

static struct ccu_common *sunxi_pll_cpu_clks[] = {
	&pll_cpu_back_clk.common,
	&pll_cpu_l_clk.common,
	&pll_cpu_b_clk.common,
	&pll_cpu_dsu_clk.common,
	&cpu_l_clk.common,
	&cpu_b_clk.common,
	&cpu_dsu_clk.common,
};

static struct clk_hw_onecell_data sunxi_cpupll_hw_clks = {
	.hws	= {
		[CLK_PLL_CPU_BACK]	= &pll_cpu_back_clk.common.hw,
		[CLK_PLL_CPU_L]		= &pll_cpu_l_clk.common.hw,
		[CLK_PLL_CPU_B]		= &pll_cpu_b_clk.common.hw,
		[CLK_PLL_CPU_DSU]	= &pll_cpu_dsu_clk.common.hw,
		[CLK_CPU_L]		= &cpu_l_clk.common.hw,
		[CLK_CPU_B]		= &cpu_b_clk.common.hw,
		[CLK_CPU_DSU]		= &cpu_dsu_clk.common.hw,
	},
	.num = CLK_CPUPLL_MAX_NO,
};

static const struct sunxi_ccu_desc cpupll_desc = {
	.ccu_clks	= sunxi_pll_cpu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sunxi_pll_cpu_clks),
	.hw_clks	= &sunxi_cpupll_hw_clks,
	.resets		= NULL,
	.num_resets	= 0,
};

static const u32 sun60iw2_pll_cpu_regs[] = {
	SUN60IW2_PLL_CPU_BACK_REG,
	SUN60IW2_PLL_CPU_L_REG,
	SUN60IW2_PLL_CPU_B_REG,
	SUN60IW2_PLL_CPU_DSU_REG,
};

static const u32 sun60iw2_pll_cpu_pat0_regs[] = {
	SUN60IW2_PLL_CPU_BACK_PAT0_REG,
	SUN60IW2_PLL_CPU_L_PAT0_REG,
	SUN60IW2_PLL_CPU_B_PAT0_REG,
	SUN60IW2_PLL_CPU_DSU_PAT0_REG,
};

static const u32 sun60iw2_pll_cpu_pat1_regs[] = {
	SUN60IW2_PLL_CPU_BACK_PAT1_REG,
	SUN60IW2_PLL_CPU_L_PAT1_REG,
	SUN60IW2_PLL_CPU_B_PAT1_REG,
	SUN60IW2_PLL_CPU_DSU_PAT1_REG,
};

static const u32 sun60iw2_pll_lfm_regs[] = {
	SUN60IW2_PLL_CPU_L_LFM_REG,
	SUN60IW2_PLL_CPU_B_LFM_REG,
	SUN60IW2_PLL_CPU_DSU_LFM_REG,
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

static void sunxi_vco_range_self_adaption(struct device *dev, int dcxo)
{
	if (dcxo == 19200000) {
		pll_cpu_back_clk.n.min = 66;  /* 1.26G/19.2M */
		pll_cpu_back_clk.n.min = 131;  /* 2.52G/19.2M */
		pll_cpu_l_clk.n.min = 35;  /* 672M/19.2M */
		pll_cpu_l_clk.n.max = 117;  /* 2.26G/19.2M */
		pll_cpu_b_clk.n.min = 35;  /* 672M/19.2M */
		pll_cpu_b_clk.n.max = 117;  /* 2.26G/19.2M */
		pll_cpu_dsu_clk.n.min = 35;  /* 672M/19.2M */
		pll_cpu_dsu_clk.n.max = 117;  /* 2.26G/19.2M */
		return;
	} else if (dcxo == 26000000) {
		pll_cpu_back_clk.n.min = 49;  /* 1.26G/26M */
		pll_cpu_back_clk.n.min = 96;  /* 2.52G/26M */
		pll_cpu_l_clk.n.min = 26;  /* 676M/26M */
		pll_cpu_l_clk.n.max = 84;  /* 2.26G/26M */
		pll_cpu_b_clk.n.min = 26;  /* 676M/26M */
		pll_cpu_b_clk.n.max = 86;  /* 2.26G/26M */
		pll_cpu_dsu_clk.n.min = 26;  /* 676M/26M */
		pll_cpu_dsu_clk.n.max = 86;  /* 2.26G/26M */
		return;
	} else if (dcxo == 24000000) {
		return;
	} else {
		sunxi_err(dev, "hosc rate:%d is error! plead check!", dcxo);
		return;
	}
}

static int sun60iw2_cpupll_probe(struct platform_device *pdev)
{
	void __iomem *reg;
	u32 val;
	int i;
	struct clk *dcxo;
	int dcxo_rate;

	reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	dcxo = devm_clk_get(&pdev->dev, "dcxo");
	if (!dcxo) {
		sunxi_err(&pdev->dev, "fail to get hosc clk!\n");
		return -EINVAL;
	}
	dcxo_rate = clk_get_rate(dcxo);
	sunxi_vco_range_self_adaption(&pdev->dev, dcxo_rate);

	/* TODO: assume boot use the cpupll */
	for (i = 0; i < ARRAY_SIZE(sun60iw2_pll_cpu_regs); i++) {
		/*
		 * set pat0_ctrl_reg and pat1_ctrl_reg
		 */
		val = readl(reg + sun60iw2_pll_cpu_pat0_regs[i]);
		val |= BIT(29) | BIT(30);  /* ues Triangular(3bit) */
		writel(val, reg + sun60iw2_pll_cpu_pat0_regs[i]);

		val = readl(reg + sun60iw2_pll_cpu_pat1_regs[i]);
		val |= BIT(31);  /* enable lfm_en */
		writel(val, reg + sun60iw2_pll_cpu_pat1_regs[i]);

		/*
		 * set pll_lfm_reg
		 */
		if (i) {  /* pll_cpu_back have no lfm reg */
			val = readl(reg + sun60iw2_pll_lfm_regs[i - 1]);
			val &= ~BIT(0);
			val |= BIT(31) | BIT(8);  /* 1.125MHz/us */
			writel(val, reg + sun60iw2_pll_lfm_regs[i - 1]);
		}

		cpupll_helper_wait_for_clear(reg + sun60iw2_pll_cpu_regs[i], BIT(26));

		/*
		 * 1. Enable pll_en pll_ldo_en lock_en pll_output
		 * 2. wait for update and lock
		 */
		val = readl(reg + sun60iw2_pll_cpu_regs[i]);
		val |= BIT(27) | BIT(29) | BIT(30) | BIT(31);
		writel(val, reg + sun60iw2_pll_cpu_regs[i]);
		cpupll_helper_wait_for_clear(reg + sun60iw2_pll_cpu_regs[i], BIT(26));
		ccupll_helper_wait_for_lock(reg + sun60iw2_pll_cpu_regs[i], BIT(28));
	}

	/* set default pll to cpu */
	set_reg(reg + SUN60IW2_CPU_L_REG, 0x3, 3, 24);
	set_reg(reg + SUN60IW2_CPU_B_REG, 0x3, 3, 24);
	set_reg(reg + SUN60IW2_CPU_DSU_REG, 0x3, 3, 24);

	sunxi_ccu_probe(pdev->dev.of_node, reg, &cpupll_desc);

	sunxi_info(NULL, "sunxi pll_cpu driver version: %s\n", SUNXI_CPUPLL_CCU_VERSION);

	return 0;
}

static const struct of_device_id sun60iw2_cpupll_ids[] = {
	{ .compatible = "allwinner,sun60iw2-cpupll" },
	{ }
};

static struct platform_driver sun60iw2_cpupll_driver = {
	.probe	= sun60iw2_cpupll_probe,
	.driver	= {
		.name	= "sun60iw2-cpupll",
		.of_match_table	= sun60iw2_cpupll_ids,
	},
};

static int __init sunxi_ccu_cpupll_init(void)
{
	int ret;

	ret = platform_driver_register(&sun60iw2_cpupll_driver);
	if (ret)
		sunxi_err(NULL, "register ccu sun60iw2 cpupll failed\n");

	return ret;
}
core_initcall(sunxi_ccu_cpupll_init);

static void __exit sunxi_ccu_cpupll_exit(void)
{
	return platform_driver_unregister(&sun60iw2_cpupll_driver);
}
module_exit(sunxi_ccu_cpupll_exit);

MODULE_DESCRIPTION("Allwinner sun60iw2 cpupll clk driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SUNXI_CPUPLL_CCU_VERSION);
MODULE_AUTHOR("emma<liujuan1@allwinnertech.com>");
