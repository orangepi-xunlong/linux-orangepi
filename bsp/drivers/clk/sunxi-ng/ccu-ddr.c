/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner DDR Clock driver.
 *
 * Copyright (C) 2020 Allwinner Technology, Inc.
 *	fanqinghua <fanqinghua@allwinnertech.com>
 *
 * Implementation of ddr clock source driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <sunxi-log.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/regmap.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <sunxi-sip.h>

#define DRIVER_NAME	"DDR-Clock-Driver"

struct sunxi_ddrclk_plat_data {
	unsigned int dram_clk_reg_offset;
	unsigned int div_shift;
	unsigned int div_width;
	/* the freq factor about ddrc and memory die, 1:2 or 1:4*/
	unsigned int factor;
};

/*
 * PLL_DDR / (DIV + 1) = DDR_CLK;
 * DDR_PHY_CLK = DDR_CLK * 2;
 * DDR_PHY_IO_CLK = DDR_CLK * 4;
 * So (DIV + 1) = PLL_DDR / (DDR_PHY_CLK / 2);
 * So (DIV + 1) = (PLL_DDR / DDR_PHY_CLK) * 2;
 * The freq in opp table is DDR_PHY_CLK.
 */

struct sunxi_ddrclk {
	struct device *dev;
	void __iomem	*ccmu_base;
	unsigned int	dram_clk;
	unsigned int	dram_div;
	struct clk_hw	hw;
	struct mutex  ddrfreq_lock;
	const struct sunxi_ddrclk_plat_data *plat_data;
	spinlock_t      lock;
};

#define to_sunxi_ddrclk_hw(_hw) container_of(_hw, struct sunxi_ddrclk, hw)

static int dbg_enable;
module_param_named(dbg_level, dbg_enable, int, 0644);

#define DBG(args...) \
	do { \
		if (dbg_enable) { \
			sunxi_info(NULL, args); \
		} \
	} while (0)

static inline int set_ddrfreq(struct sunxi_ddrclk *ddrclk, unsigned int freq_id)
{
	int result;

	result = invoke_scp_fn_smc(ARM_SVC_SUNXI_DDRFREQ, freq_id, 0, 0);

	return result;
}

static unsigned long sunxi_ddr_clk_recalc_rate(struct clk_hw *hw,
					       unsigned long parent_rate)
{
	struct sunxi_ddrclk *ddrclk = to_sunxi_ddrclk_hw(hw);
	const struct sunxi_ddrclk_plat_data *plat_data = ddrclk->plat_data;
	unsigned int reg_val, div;
	unsigned long rate = parent_rate << plat_data->factor;

	reg_val = readl_relaxed(ddrclk->ccmu_base + plat_data->dram_clk_reg_offset);

	div = (reg_val >> plat_data->div_shift) & GENMASK(plat_data->div_width - 1, 0);
	rate /= (div + 1);

	return rate;
}

static long sunxi_ddr_clk_round_rate(struct clk_hw *hw,
				     unsigned long target_rate,
				     unsigned long *prate)
{
	struct sunxi_ddrclk *ddrclk = to_sunxi_ddrclk_hw(hw);
	const struct sunxi_ddrclk_plat_data *plat_data = ddrclk->plat_data;
	unsigned int dram_div = ddrclk->dram_div;
	unsigned long rate = *prate << plat_data->factor;

	if ((dram_div & 0x1f) == 0x3) {
		if (target_rate <= rate / (((dram_div >> 24) & 0x1f) + 1))
			return rate / (((dram_div >> 24) & 0x1f) + 1);
		else if (target_rate <= rate / (((dram_div >> 16) & 0x1f) + 1))
			return rate / (((dram_div >> 16) & 0x1f) + 1);
		else if (target_rate <= rate / (((dram_div >> 8) & 0x1f) + 1))
			return rate / (((dram_div >> 8) & 0x1f) + 1);
		else
			return rate / ((dram_div & 0x1f) + 1);
	} else {
		if (target_rate <= rate / (((dram_div >> 24) & 0x1f) + 1))
			return rate / (((dram_div >> 24) & 0x1f) + 1);
		else if (target_rate <= rate / (((dram_div >> 16) & 0x1f) + 1))
			return rate / (((dram_div >> 16) & 0x1f) + 1);
		else if (target_rate <= rate / (((dram_div >> 8) & 0x1f) + 1))
			return rate / (((dram_div >> 8) & 0x1f) + 1);
		else if (target_rate <= rate / ((dram_div & 0x1f) + 1))
			return rate / ((dram_div & 0x1f) + 1);
		else if (target_rate <= rate / 7)
			return rate / 7;
		else if (target_rate <= rate / 6)
			return rate / 6;
		else if (target_rate <= rate / 5)
			return rate / 5;
		else
			return rate / 4;
	}
}

static int sunxi_ddr_clk_set_rate(struct clk_hw *hw, unsigned long drate,
				  unsigned long prate)
{
	struct sunxi_ddrclk *ddrclk = to_sunxi_ddrclk_hw(hw);
	const struct sunxi_ddrclk_plat_data *plat_data = ddrclk->plat_data;
	unsigned int dram_div = ddrclk->dram_div;
	unsigned long rate = prate << plat_data->factor;
	unsigned int freq_id;

	if ((dram_div & 0x1f) == 0x3) {
		if (drate <= rate / (((dram_div >> 24) & 0x1f) + 1))
			freq_id = 3;
		else if (drate <= rate / (((dram_div >> 16) & 0x1f) + 1))
			freq_id = 2;
		else if (drate <= rate / (((dram_div >> 8) & 0x1f) + 1))
			freq_id = 1;
		else
			freq_id = 0;
	} else {
		if (drate <= rate / (((dram_div >> 24) & 0x1f) + 1))
			freq_id = 7;
		else if (drate <= rate / (((dram_div >> 16) & 0x1f) + 1))
			freq_id = 6;
		else if (drate <= rate / (((dram_div >> 8) & 0x1f) + 1))
			freq_id = 5;
		else if (drate <= rate / ((dram_div & 0x1f) + 1))
			freq_id = 4;
		else if (drate <= rate / 7)
			freq_id = 3;
		else if (drate <= rate / 6)
			freq_id = 2;
		else if (drate <= rate / 5)
			freq_id = 1;
		else
			freq_id = 0;
	}
	DBG("drate:%ldM\n", drate / 1000000);
	mutex_lock(&ddrclk->ddrfreq_lock);
	set_ddrfreq(ddrclk, freq_id);
	ddrclk->dram_clk = drate;
	mutex_unlock(&ddrclk->ddrfreq_lock);

	return 0;
}

const struct clk_ops sunxi_ddrclk_ops = {
	.recalc_rate = sunxi_ddr_clk_recalc_rate,
	.round_rate = sunxi_ddr_clk_round_rate,
	.set_rate = sunxi_ddr_clk_set_rate,
};

static const struct sunxi_ddrclk_plat_data ddrclk_sun55iw3_data = {
	.dram_clk_reg_offset = 0x800,
	.div_shift = 0,
	.div_width = 5,
	.factor = 1,
};

static const struct sunxi_ddrclk_plat_data ddrclk_sun55iw6_data = {
	.dram_clk_reg_offset = 0xc00,
	.div_shift = 0,
	.div_width = 5,
	.factor = 1,
};

static const struct sunxi_ddrclk_plat_data ddrclk_sun60iw2_data = {
	.dram_clk_reg_offset = 0xc00,
	.div_shift = 0,
	.div_width = 5,
	.factor = 2,
};

static const struct of_device_id clk_ddr_of_match[] = {
	{ .compatible = "allwinner,clock_ddr", .data = &ddrclk_sun55iw3_data},
	{ .compatible = "allwinner,sun55iw6_clock_ddr", .data = &ddrclk_sun55iw6_data},
	{ .compatible = "allwinner,sun60iw2_clock_ddr", .data = &ddrclk_sun60iw2_data},
	{ },
};
MODULE_DEVICE_TABLE(of, clk_ddr_of_match);

static int ddr_clock_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *dram_np;
	struct sunxi_ddrclk *ddrclk;
	struct clk_init_data init;
	struct clk *clk, *parent_clk;
	const char *parent_name;
	int ret = 0;

	if (!np) {
		sunxi_err(&pdev->dev, "failed to match ddr clock\n");
		return -ENODEV;
	}

	ddrclk = devm_kzalloc(&pdev->dev, sizeof(*ddrclk), GFP_KERNEL);
	if (!ddrclk)
		return -ENOMEM;

	ddrclk->dev = &pdev->dev;
	platform_set_drvdata(pdev, ddrclk);

	mutex_init(&ddrclk->ddrfreq_lock);
	dram_np = of_find_node_by_path("/dram");
	if (!dram_np) {
		sunxi_err(&pdev->dev, "failed to find dram node\n");
		return -ENODEV;
	}

	ret = of_property_read_u32(dram_np, "dram_para[00]", &ddrclk->dram_clk);
	if (ret) {
		ret = of_property_read_u32(dram_np, "dram_para00", &ddrclk->dram_clk);
		if (ret) {
			sunxi_err(&pdev->dev, "failed to find dram_clk\n");
			return -ENODEV;
		}
	}

	sunxi_err(NULL, "dram_clk:%d\n", ddrclk->dram_clk);

	ret = of_property_read_u32(dram_np, "dram_para[24]", &ddrclk->dram_div);
	if (ret) {
		ret = of_property_read_u32(dram_np, "dram_para24", &ddrclk->dram_div);
		if (ret) {
			sunxi_err(&pdev->dev, "failed to find dram_div\n");
			return -ENODEV;
		}
	}

	sunxi_err(NULL, "dram_div:0x%x\n", ddrclk->dram_div);

	ddrclk->ccmu_base = of_iomap(np, 0);
	if (!ddrclk->ccmu_base) {
		sunxi_err(&pdev->dev, "map ccmu failed\n");
		return -ENODEV;
	}

	parent_clk = devm_clk_get(&pdev->dev, "pll_ddr");
	if (IS_ERR(parent_clk)) {
		sunxi_err(&pdev->dev, "clk_get pll_ddr failed\n");
		ret = -ENODEV;
		goto out;
	}

	parent_name = __clk_get_name(parent_clk);
	if (!parent_name) {
		sunxi_err(&pdev->dev, "get clk name failed\n");
		ret = -ENODEV;
		goto out;
	}

	ddrclk->hw.init = &init;
	init.name = "sdram";
	init.ops = &sunxi_ddrclk_ops;
	init.parent_names = &parent_name;
	init.num_parents = 1;
	init.flags |= CLK_SET_RATE_NO_REPARENT | CLK_GET_RATE_NOCACHE;

	ddrclk->plat_data = of_device_get_match_data(dev);
	clk = devm_clk_register(&pdev->dev, &ddrclk->hw);
	if (IS_ERR(clk)) {
		sunxi_err(&pdev->dev, "clk_register failed\n");
		ret = -ENODEV;
		goto out;
	}

	of_clk_add_provider(np, of_clk_src_simple_get, clk);
	return 0;

out:
	iounmap((char __iomem *)ddrclk->ccmu_base);
	return ret;
}

static int ddr_clock_remove(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;

	of_clk_del_provider(np);
	return 0;
}

static struct platform_driver ddr_clock_driver = {
	.probe   = ddr_clock_probe,
	.remove  = ddr_clock_remove,
	.driver  = {
		.name  = "sunxi-ddrclock",
		.of_match_table = clk_ddr_of_match,
	},
};

module_platform_driver(ddr_clock_driver);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Allwinner DDR Clock driver");
MODULE_ALIAS("platform:" DRIVER_NAME);
MODULE_AUTHOR("fanqinghua <fanqinghua@allwinnertech.com>");
MODULE_VERSION("1.0.1");
