// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2020 huafenghuang@allwinnertech.com
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

#include "ccu_common.h"
#include "ccu_reset.h"

#include "ccu_nm.h"

#define SUN50IW10_DISPLL_TUN1_REG	0x00c
/* Fix me */
void __iomem *switch_reg;
void __iomem *ccu_cpu_base;

static struct ccu_nm pll_displl_clk = {
	.enable		= BIT(22) | BIT(23) | BIT(24) | BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_OFFSET_MIN_MAX(8, 8, 0, 12, 0), /* form shc */
	.m		= _SUNXI_CCU_DIV_MAX(0, 4, 1),
	.min_rate	= 288000000,
	.common		= {
		.reg		= 0x0,	/* restore in low level driver */
		.ssc_reg	= 0x18,
		.clear		= BIT(25),
		.features	= CCU_FEATURE_CLEAR_MOD,
		.hw.init	= CLK_HW_INIT("displl-cpu", "dcxo24M",
					&ccu_nm_ops,
					CLK_GET_RATE_NOCACHE | CLK_IS_CRITICAL \
					| CLK_SET_RATE_UNGATE),
	},
};

struct ccu_common displl_ssc_common = {
	.reg = 0x18,
};

static struct ccu_common *sunxi_displl_clks[] = {
	&pll_displl_clk.common,
	&displl_ssc_common,
};

static struct clk_hw_onecell_data sunxi_displl_hw_clks = {
	.hws	= {
		[0]		= &pll_displl_clk.common.hw,
	},
	.num = 1,
};

static const struct sunxi_ccu_desc displl_desc = {
	.ccu_clks	= sunxi_displl_clks,
	.num_ccu_clks	= ARRAY_SIZE(sunxi_displl_clks),
	.hw_clks	= &sunxi_displl_hw_clks,
	.resets		= NULL,
	.num_resets	= 0,
};

static int displl_set_ssc(struct ccu_pll_nb *pll, bool on)
{
	u32 reg;
	unsigned long flags;

	if (IS_ERR_OR_NULL(pll))
		return 0;

	spin_lock_irqsave(pll->common->lock, flags);

	if (on) {
		reg = readl(pll->common->base + pll->common->ssc_reg);
		reg |= pll->enable;
		writel(reg, pll->common->base + pll->common->ssc_reg);

		/* Set Bit25 for cfg pll n and m */
		reg = readl(pll->common->base + pll->common->reg);
		reg |= pll->common->clear;
		writel(reg, pll->common->base + pll->common->reg);
	} else {
		reg = readl(pll->common->base + pll->common->ssc_reg);
		reg &= ~(pll->enable);
		writel(reg, pll->common->base + pll->common->ssc_reg);

		/* Set Bit25 for cfg pll n and m */
		reg = readl(pll->common->base + pll->common->reg);
		reg |= pll->common->clear;
		writel(reg, pll->common->base + pll->common->reg);
	}

	WARN_ON(readl_relaxed_poll_timeout_atomic(pll->common->base \
				+ pll->common->reg, reg,
			!(reg & pll->common->clear), 100, 10000));

	spin_unlock_irqrestore(pll->common->lock, flags);


	return 0;
}

static int displl_notifier_cb(struct notifier_block *nb,
				unsigned long event, void *data)
{
	struct ccu_pll_nb *pll = to_ccu_pll_nb(nb);
	int ret = 0;

	if (event == PRE_RATE_CHANGE) {
		/* Enable ssc function */
		ret = displl_set_ssc(pll, true);
	} else if (event == POST_RATE_CHANGE) {
		/* Disable ssc function */
		ret = displl_set_ssc(pll, false);
	}

	return notifier_from_errno(ret);
}

static struct ccu_pll_nb displl_nb = {
	.common = &pll_displl_clk.common,
	.enable = BIT(31),
	.clk_nb = {
		.notifier_call = displl_notifier_cb,
	},
};

static inline unsigned int calc_pll_ssc(unsigned int step, unsigned int scale,
		unsigned int ssc)
{
	return (unsigned int)(((1 << 17) * ssc) - (scale * (1 << step))) / scale;
}

#ifdef CONFIG_PM_SLEEP

struct sunxi_displl_reg_cache {
	struct list_head node;
	struct ccu_reg_dump *rdump;
	unsigned int rd_num;
	void __iomem *reg_base;
	struct ccu_nm *nm;
};

LIST_HEAD(displl_reg_cache_list);

static int displl_suspend(void)
{
	struct sunxi_displl_reg_cache *reg_cache;
	int num_regs = 0;
	struct ccu_reg_dump *rd = NULL;

	list_for_each_entry(reg_cache, &displl_reg_cache_list, node) {
		rd = reg_cache->rdump;
		for (num_regs = reg_cache->rd_num; num_regs > 0;
				--num_regs, ++rd)
			rd->value = readl(reg_cache->reg_base + rd->offset);
	}
	return 0;
}

static void displl_resume(void)
{
	struct sunxi_displl_reg_cache *reg_cache;
	int num_regs = 0;
	struct ccu_reg_dump *rd = NULL;
	u32 val = 0;
	unsigned int timeout = 0xff;
	struct ccu_nm *nm = NULL;
	struct ccu_common *com = NULL;

	list_for_each_entry(reg_cache, &displl_reg_cache_list, node) {
		rd = reg_cache->rdump;
		nm = reg_cache->nm;
		com = &nm->common;
		for (num_regs = reg_cache->rd_num; num_regs > 0;
				--num_regs, ++rd) {
			val = readl(reg_cache->reg_base + rd->offset);
			/* Check if the reg should be restored */
			if (val != rd->value) {
				writel(rd->value, reg_cache->reg_base + rd->offset);
				val = readl(com->base + com->reg);
				val |= com->clear;
				writel(val, com->base + com->reg);
				while ((readl(com->base + com->reg) &
						com->clear) && --timeout)
					udelay(10);
				if (timeout == 0) {
					pr_err("Wait for clear timeout....\n");
					WARN_ON(1);
				}
				if (!rd->offset) {
					/* Wait pll stable */
					timeout = 0xff;
					val = readl(com->base + com->reg);
					while ((!(readl(com->base + com->reg) &
						nm->lock)) && --timeout)
						udelay(10);
					if (timeout == 0) {
						pr_err("Wait for displl stable timeout....\n");
						WARN_ON(1);
					}
				}
			}
		}
	}

	/* Fix Me
	 * In order to work around an hardware problem(the cpu clk switching
	 * with glitch),must switch cpu clk to 24M.
	 */
	/* AXI to 24M */
	val = readl(ccu_cpu_base + 0x500);
	val &= ~(0x7 << 24);
	val |= (0x0 << 24);
	writel(val, ccu_cpu_base + 0x500);

	/* Switch to displl */
	val = readl(switch_reg);
	/* Check if the reg should be restored */
	if (!(val & BIT(2))) {
		val |= BIT(2);
		writel(val, switch_reg);
	}

	/* Fix Me
	 * In order to work around an hardware problem(the cpu clk switching
	 * with glitch),must switch cpu clk to 24M, now when displl ready,
	 * swich to it.
	 */
	/* AXI to PLL_CPUX */
	val = readl(ccu_cpu_base + 0x500);
	val &= ~(0x7 << 24);
	val |= (0x3 << 24);
	writel(val, ccu_cpu_base + 0x500);
}

static struct syscore_ops sunxi_displl_syscore_ops = {
	.suspend = displl_suspend,
	.resume = displl_resume,
};

void static displl_sleep_init(void __iomem *reg_base,
				struct ccu_common **rdump,
				unsigned long nr_rdump, struct ccu_nm *nm)
{
	struct sunxi_displl_reg_cache *reg_cache = NULL;
	struct ccu_reg_dump *rd;
	unsigned int i;

	reg_cache = kzalloc(sizeof(*reg_cache), GFP_KERNEL);
	if (!reg_cache)
		panic("Could not alloc memory for reg_cache\n");

	reg_cache->rdump = kcalloc(nr_rdump, sizeof(*rd), GFP_KERNEL);
	if (!reg_cache->rdump)
		panic("Could not alloc memory for reg_cache->rdump\n");

	for (i = 0; i < nr_rdump; ++i) {
		struct ccu_common *ccu_clks = rdump[i];
		reg_cache->rdump[i].offset = ccu_clks->reg;
	}

	if (list_empty(&displl_reg_cache_list))
		register_syscore_ops(&sunxi_displl_syscore_ops);

	reg_cache->reg_base = reg_base;
	reg_cache->rd_num = nr_rdump;
	reg_cache->nm = nm;
	list_add_tail(&reg_cache->node, &displl_reg_cache_list);
}
#endif

static int sun50iw10_displl_probe(struct platform_device *pdev)
{
	void __iomem *reg;
	u32 val;
	int ret;
	unsigned int step = 0, scale = 0, ssc = 0;
	struct device_node *np = pdev->dev.of_node;

	reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	switch_reg = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(switch_reg))
		return PTR_ERR(switch_reg);

	ccu_cpu_base = ioremap(0x03001000, 0x1000);
	if (IS_ERR(ccu_cpu_base))
		return PTR_ERR(ccu_cpu_base);

	if (of_property_read_u32(np, "pll_step", &step))
		step = 0x0;

	if (of_property_read_u32(np, "pll_ssc_scale", &scale))
		scale = 0xa;

	if (of_property_read_u32(np, "pll_ssc", &ssc))
		ssc = 0x1;

	/*
	 * Displl cfg
	 * 1. close sdm
	 * 2. enable displl
	 * 3. update displl
	 */
	val = readl(reg + SUN50IW10_DISPLL_TUN1_REG);
	val &= ~BIT(31);
	writel(val, reg + SUN50IW10_DISPLL_TUN1_REG);

	val = readl(reg + pll_displl_clk.common.reg);
	val |= BIT(22) | BIT(23) | BIT(24) | BIT(31) | BIT(29) ;
	writel(val, reg + pll_displl_clk.common.reg);

	/*
	 * Update displl
	 */
	val = readl(reg + pll_displl_clk.common.reg);
	val |= pll_displl_clk.common.clear;
	writel(val, reg + pll_displl_clk.common.reg);
	WARN_ON(readl_relaxed_poll_timeout(reg, val,
			!(val & pll_displl_clk.common.clear), 100, 10000));

	/*
	 * Wait pll stable
	 */
	WARN_ON(readl_relaxed_poll_timeout(reg, val,
			(val & pll_displl_clk.lock), 100, 70000));

	/*
	 * SSC cfg
	 */
	ssc = calc_pll_ssc(step, scale, ssc);
	writel((ssc << 12 | step << 0 | BIT(31)),
			reg + pll_displl_clk.common.ssc_reg);

	/* Update displl */
	val = readl(reg + pll_displl_clk.common.reg);
	val |= pll_displl_clk.common.clear;
	writel(val, reg + pll_displl_clk.common.reg);
	WARN_ON(readl_relaxed_poll_timeout(reg, val,
			!(val & pll_displl_clk.common.clear), 100, 10000));

	/* Enable displl clk and the the lock bits
	 * Set displl to 1008M and enforece m1 = 0, p = 0
	 */
	val = readl(reg + pll_displl_clk.common.reg);
	val &= ~(BIT(31) | (0xf << 16) | (0xff << 8) | (0x3 << 4) | (0xf << 0));
	val |= BIT(22) | BIT(23) | BIT(24) | BIT(31) | BIT(29) ;

	val |= (42 << 8);
	writel(val, reg + pll_displl_clk.common.reg);

	/* Update displl */
	val = readl(reg + pll_displl_clk.common.reg);
	val |= pll_displl_clk.common.clear;
	writel(val, reg + pll_displl_clk.common.reg);
	WARN_ON(readl_relaxed_poll_timeout(reg, val,
			!(val & pll_displl_clk.common.clear), 100, 10000));

	/* Wait pll stable */
	WARN_ON(readl_relaxed_poll_timeout(reg, val,
			(val & pll_displl_clk.lock), 100, 70000));

	/* Fix Me
	 * In order to work around an hardware problem(the cpu clk switching
	 * with glitch),must switch cpu clk to 24M.
	 */
	/* AXI to 24M */
	val = readl(ccu_cpu_base + 0x500);
	val &= ~(0x7 << 24);
	val |= (0x0 << 24);
	writel(val, ccu_cpu_base + 0x500);

	/* Switch to displl */
	val = readl(switch_reg);
	val |= BIT(2);
	writel(val, switch_reg);

	/* Fix Me
	 * In order to work around an hardware problem(the cpu clk switching
	 * with glitch),must switch cpu clk to 24M, now when displl ready,
	 * swich to it.
	 */
	/* AXI to PLL_CPUX */
	val = readl(ccu_cpu_base + 0x500);
	val &= ~(0x7 << 24);
	val |= (0x3 << 24);
	writel(val, ccu_cpu_base + 0x500);
	ret = sunxi_ccu_probe(np, reg, &displl_desc);
	if (ret)
		return ret;

	ccu_pll_notifier_register(&displl_nb);

#ifdef CONFIG_PM_SLEEP
	displl_sleep_init(reg, sunxi_displl_clks, ARRAY_SIZE(sunxi_displl_clks),
			&pll_displl_clk);
#endif

	dev_info(&pdev->dev, "Sunxi displl clk init OK\n");

	return 0;
}

static const struct of_device_id sun50iw10_displl_ids[] = {
	{ .compatible = "allwinner,sun50iw10-displl" },
	{ }
};

static struct platform_driver sun50iw10_displl_driver = {
	.probe	= sun50iw10_displl_probe,
	.driver	= {
		.name	= "sun50iw10-displl",
		.of_match_table	= sun50iw10_displl_ids,
	},
};

static int __init sunxi_ccu_displl_init(void)
{
	int ret;

	ret = platform_driver_register(&sun50iw10_displl_driver);
	if (ret)
		pr_err("register ccu sun50iw10 displl failed\n");

	return ret;
}
core_initcall(sunxi_ccu_displl_init);

static void __exit sunxi_ccu_displl_exit(void)
{
	return platform_driver_unregister(&sun50iw10_displl_driver);
}
module_exit(sunxi_ccu_displl_exit);

MODULE_DESCRIPTION("Allwinner sun50iw10 clk driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.6");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
