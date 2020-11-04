/*
 * Copyright (C) 2016-2020 Allwinnertech
 * Wim Hwang <huangwei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/clk/sunxi.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include "clk-sunxi.h"
#include "clk-factors.h"
#include "clk-periph.h"
#include "clk-cpu.h"

#ifdef CONFIG_PM_SLEEP
/*list head use for standby*/
LIST_HEAD(clk_periph_reg_cache_list);
LIST_HEAD(clk_factor_reg_cache_list);
#endif

/*
 * of_sunxi_clocks_init() - Clocks initialize
 */
static void __init of_sunxi_clocks_init(struct device_node *node)
{
	/* do some soc special init here */
	sunxi_clocks_init(node);
}

static void __init of_sunxi_fixed_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	u32 rate;

	if (of_property_read_u32(node, "clock-frequency", &rate))
		return;

	if (of_property_read_string(node, "clock-output-names", &clk_name)) {
		pr_err("%s:get clock-output-names failed in %s node\n",
						__func__, node->full_name);
		return;
	}

	clk = clk_register_fixed_rate(NULL, clk_name, NULL, CLK_IS_ROOT, rate);
	if (!IS_ERR(clk)) {
		clk_register_clkdev(clk, clk_name, NULL);
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
	}
}

static void __init of_sunxi_fixed_factor_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	const char *parent_name;
	u32 div, mult;

	if (of_property_read_u32(node, "clock-div", &div)) {
		pr_err("%s Fixed factor clock <%s> must have a clock-div property\n",
			__func__, node->name);
		return;
	}

	if (of_property_read_u32(node, "clock-mult", &mult)) {
		pr_err("%s Fixed factor clock <%s> must have a clokc-mult property\n",
			__func__, node->name);
		return;
	}

	if (of_property_read_string(node, "clock-output-names", &clk_name)) {
		pr_err("%s:get clock-output-names failed in %s node\n",
						__func__, node->full_name);
		return;
	}
	parent_name = of_clk_get_parent_name(node, 0);

	clk = clk_register_fixed_factor(NULL, clk_name, parent_name, 0,
					mult, div);
	if (!IS_ERR(clk)) {
		clk_register_clkdev(clk, clk_name, NULL);
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
	}
}

/*
 * of_sunxi_pll_clk_setup() - Setup function for pll factors clk
 */
static void __init of_sunxi_pll_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	const char *lock_mode = NULL;
	struct factor_init_data *factor;

	if (of_property_read_string(node, "clock-output-names", &clk_name)) {
		pr_err("%s:get clock-output-names failed in %s node\n",
						__func__, node->full_name);
		return;
	}
	factor = sunxi_clk_get_factor_by_name(clk_name);
	if (!factor) {
		pr_err("clk %s not found in %s\n", clk_name, __func__);
		return;
	}

	if (!of_property_read_string(node, "lock-mode", &lock_mode))
		sunxi_clk_set_factor_lock_mode(factor, lock_mode);

	clk = sunxi_clk_register_factors(NULL, sunxi_clk_base,
					 &clk_lock, factor);
	if (!IS_ERR(clk)) {
		clk_register_clkdev(clk, clk_name, NULL);
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
	}
}

/*
 * of_sunxi_periph_clk_setup() - Setup function for periph clk
 */
static void __init of_sunxi_periph_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	struct periph_init_data *periph;

	if (of_property_read_string(node, "clock-output-names", &clk_name)) {
		pr_err("%s:get clock-output-names failed in %s node\n",
						__func__, node->full_name);
		return;
	}

	periph = sunxi_clk_get_periph_by_name(clk_name);
	if (!periph) {
		pr_err("clk %s not found in %s\n", clk_name, __func__);
		return;
	}

	clk = sunxi_clk_register_periph(periph, sunxi_clk_base);
	if (!IS_ERR(clk)) {
		clk_register_clkdev(clk, clk_name, NULL);
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
	}
}

/*
 * of_sunxi_cpu_clk_setup() - Setup function for cpu clk
 */
static void __init of_sunxi_cpu_clk_setup(struct device_node *node)
{
	sunxi_cpu_clocks_init(node);
}

/**
 * of_periph_cpus_clk_setup() - Setup function for periph cpus clk
 */
void of_sunxi_periph_cpus_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	struct periph_init_data *periph;

	if (of_property_read_string(node, "clock-output-names", &clk_name)) {
		pr_err("%s:get clock-output-names failed in %s node\n",
						__func__, node->full_name);
		return;
	}

	periph = sunxi_clk_get_periph_cpus_by_name(clk_name);
	if (!periph) {
		pr_err("clk %s not found in %s\n", clk_name, __func__);
		return;
	}

	/* register clk */
	if (!strcmp(clk_name, "losc_out") ||
				!strcmp(clk_name, "dcxo_out") ||
				!strcmp(clk_name, "hosc32k")) {
		clk = sunxi_clk_register_periph(periph, 0);
	} else
		clk = sunxi_clk_register_periph(periph,
					sunxi_clk_cpus_base);

	if (!IS_ERR(clk)) {
		clk_register_clkdev(clk, clk_name, NULL);
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
		return;
	}

	pr_err("clk %s not found in %s\n", clk_name, __func__);
}

CLK_OF_DECLARE(sunxi_clocks_init, "allwinner,clk-init", of_sunxi_clocks_init);
CLK_OF_DECLARE(sunxi_fixed_clk, "allwinner,fixed-clock", of_sunxi_fixed_clk_setup);
CLK_OF_DECLARE(sunxi_pll_clk, "allwinner,pll-clock", of_sunxi_pll_clk_setup);
CLK_OF_DECLARE(sunxi_fixed_factor_clk, "allwinner,fixed-factor-clock", of_sunxi_fixed_factor_clk_setup);
CLK_OF_DECLARE(sunxi_cpu_clk, "allwinner,cpu-clock", of_sunxi_cpu_clk_setup);
CLK_OF_DECLARE(sunxi_periph_clk, "allwinner,periph-clock", of_sunxi_periph_clk_setup);
CLK_OF_DECLARE(periph_cpus_clk, "allwinner,periph-cpus-clock", of_sunxi_periph_cpus_clk_setup);
