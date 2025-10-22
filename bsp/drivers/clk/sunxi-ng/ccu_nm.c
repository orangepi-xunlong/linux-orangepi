// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2016 Maxime Ripard
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/version.h>

#include "ccu_frac.h"
#include "ccu_gate.h"
#include "ccu_nm.h"
#include "ccu_sdm.h"

struct _ccu_nm {
	unsigned long	n, min_n, max_n;
	unsigned long	m, min_m, max_m;
};

static u64 ccu_nm_calc_rate(unsigned long parent,
				      unsigned long n, unsigned long m)
{
	u64 rate = parent;

	rate *= n;
	do_div(rate, m);

	return rate;
}

static void ccu_nm_find_best(unsigned long parent, u64 rate,
			     const struct clk_div_table *table, struct _ccu_nm *nm)
{
	u64 best_rate = 0;
	unsigned long best_n = 0, best_m = 0;
	unsigned long _n, _m;

	for (_n = nm->min_n; _n <= nm->max_n; _n++) {
		for (_m = nm->min_m; _m <= nm->max_m; _m++) {
			u64 tmp_rate, div = 0;

			/* Look for div in the table first, no div in the table skip this loop */
			if (table) {
				div = ccu_get_table_div(table, (_m - 1));
				if (!div) {
					sunxi_info(NULL, "val %lu has no corresponding div in table\n", (_m - 1));
					continue;
				}
			} else {
				div = _m;
			}

			tmp_rate = ccu_nm_calc_rate(parent,
							 _n, div);

			if (tmp_rate > rate)
				continue;

			if ((rate - tmp_rate) < (rate - best_rate)) {
				best_rate = tmp_rate;
				best_n = _n;
				best_m = div;
			}
		}
	}

	nm->n = best_n;
	nm->m = best_m;
}

static void ccu_nm_disable(struct clk_hw *hw)
{
	struct ccu_nm *nm = hw_to_ccu_nm(hw);

#if IS_ENABLED(CONFIG_AW_STANDARD_CCU)
	return ccu_pll_gate_helper_disable(&nm->common, nm->enable, nm->output, nm->lock_enable, nm->ldo_en);
#else
	return ccu_gate_helper_disable(&nm->common, nm->enable);
#endif
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ccu_nm_init(struct clk_hw *hw)
{
	struct ccu_common *common = hw_to_ccu_common(hw);

	if (!(common->features & CCU_FEATURE_INIT_GATE))
		return 0;

	if ((clk_hw_get_flags(hw) & CLK_IS_CRITICAL) || (clk_hw_get_flags(hw) & CLK_IGNORE_UNUSED))
		return 0;

	ccu_nm_disable(hw);

	return 0;
}
#else
static void ccu_nm_init(struct clk_hw *hw)
{
}
#endif

static int ccu_nm_enable(struct clk_hw *hw)
{
	struct ccu_nm *nm = hw_to_ccu_nm(hw);

#if IS_ENABLED(CONFIG_AW_STANDARD_CCU)
	return ccu_pll_gate_helper_enable(&nm->common, nm->enable, nm->output, nm->lock, nm->lock_enable, nm->ldo_en);
#else
	return ccu_gate_helper_enable(&nm->common, nm->enable);
#endif
}

static int ccu_nm_is_enabled(struct clk_hw *hw)
{
	struct ccu_nm *nm = hw_to_ccu_nm(hw);

	return ccu_gate_helper_is_enabled(&nm->common, nm->enable);
}

int ccu_nm_is_sdm_enabled(struct clk_hw *hw)
{
	struct ccu_nm *nm;
	struct ccu_common sdm_common;

	nm = hw_to_ccu_nm(hw);
	sdm_common = nm->common;

	return readl(sdm_common.base + sdm_common.reg) & BIT(24);
}

static unsigned long ccu_nm_recalc_rate(struct clk_hw *hw,
					unsigned long parent_rate)
{
	struct ccu_nm *nm = hw_to_ccu_nm(hw);
	u64 rate;
	unsigned long n, m;
	u32 reg;
	u32 sdmval;

	if (ccu_frac_helper_is_enabled(&nm->common, &nm->frac)) {
		rate = ccu_frac_helper_read_rate(&nm->common, &nm->frac);

		if (nm->common.features & CCU_FEATURE_FIXED_POSTDIV)
			do_div(rate, nm->fixed_post_div);

		return rate;
	}

	reg = readl(nm->common.base + nm->common.reg);

	n = reg >> nm->n.shift;
	n &= (1 << nm->n.width) - 1;
	n += nm->n.offset;
	if (!n)
		n++;

	m = reg >> nm->m.shift;
	m &= (1 << nm->m.width) - 1;
	if (nm->m.table)
		m = ccu_get_table_div(nm->m.table, m);
	else
		m += nm->m.offset;
	if (!m)
		m++;

	if (ccu_sdm_helper_is_enabled(&nm->common, &nm->sdm))
		rate = ccu_sdm_helper_read_rate(&nm->common, &nm->sdm, m, n);
	else
		rate = ccu_nm_calc_rate(parent_rate, n, m);

	if (nm->common.features & CCU_FEATURE_FIXED_PREDIV)
		rate *= nm->fixed_pre_div;

	if (nm->common.features & CCU_FEATURE_FIXED_POSTDIV)
		do_div(rate, nm->fixed_post_div);

	if (nm->common.sdm_info) {
		sdmval = ccu_get_sdmval(rate, &nm->common, m, n);
		ccu_common_set_sdm_value(&nm->common, &nm->sdm, sdmval);
	}
	return rate;
}

static long ccu_nm_round_rate(struct clk_hw *hw, unsigned long _rate,
			      unsigned long *parent_rate)
{
	struct ccu_nm *nm = hw_to_ccu_nm(hw);
	struct _ccu_nm _nm;
	u64 rate = _rate;

	if (nm->common.features & CCU_FEATURE_FIXED_POSTDIV)
		rate *= nm->fixed_post_div;

	if (nm->common.features & CCU_FEATURE_FIXED_PREDIV)
		do_div(rate, nm->fixed_pre_div);

	if (rate < nm->min_rate) {
		rate = nm->min_rate;
		if (nm->common.features & CCU_FEATURE_FIXED_POSTDIV)
			do_div(rate, nm->fixed_post_div);
		if (nm->common.features & CCU_FEATURE_FIXED_PREDIV)
			rate *= nm->fixed_pre_div;
		return rate;
	}

	if (nm->max_rate && rate > nm->max_rate) {
		rate = nm->max_rate;
		if (nm->common.features & CCU_FEATURE_FIXED_POSTDIV)
			do_div(rate, nm->fixed_post_div);
		if (nm->common.features & CCU_FEATURE_FIXED_PREDIV)
			rate *= nm->fixed_pre_div;
		return rate;
	}

	if (ccu_frac_helper_has_rate(&nm->common, &nm->frac, rate)) {
		if (nm->common.features & CCU_FEATURE_FIXED_POSTDIV)
			do_div(rate, nm->fixed_post_div);
		if (nm->common.features & CCU_FEATURE_FIXED_PREDIV)
			rate *= nm->fixed_pre_div;
		return rate;
	}

	if (ccu_sdm_helper_has_rate(&nm->common, &nm->sdm, rate)) {
		if (nm->common.features & CCU_FEATURE_FIXED_POSTDIV)
			do_div(rate, nm->fixed_post_div);
		if (nm->common.features & CCU_FEATURE_FIXED_PREDIV)
			rate *= nm->fixed_pre_div;
		return rate;
	}

	_nm.min_n = nm->n.min ?: 1;
	_nm.max_n = nm->n.max ?: 1 << nm->n.width;
	_nm.min_m = 1;
	_nm.max_m = nm->m.max ?: 1 << nm->m.width;

	ccu_nm_find_best(*parent_rate, rate, nm->m.table, &_nm);
	rate = ccu_nm_calc_rate(*parent_rate, _nm.n, _nm.m);

	if (nm->common.features & CCU_FEATURE_FIXED_POSTDIV)
		do_div(rate, nm->fixed_post_div);

	if (nm->common.features & CCU_FEATURE_FIXED_PREDIV)
		rate *= nm->fixed_pre_div;

	return rate;
}

static int ccu_nm_set_rate(struct clk_hw *hw, unsigned long _rate,
			   unsigned long parent_rate)
{
	struct ccu_nm *nm = hw_to_ccu_nm(hw);
	struct _ccu_nm _nm;
	unsigned long flags;
	u32 reg;
	u64 rate = _rate;

	/* Adjust target rate according to post-dividers */
	if (nm->common.features & CCU_FEATURE_FIXED_POSTDIV)
		rate = rate * nm->fixed_post_div;

#if IS_ENABLED(CONFIG_AW_STANDARD_CCU)
	ccu_pll_output_helper_disable(&nm->common, nm->output);
#endif

	if (ccu_frac_helper_has_rate(&nm->common, &nm->frac, rate)) {
		spin_lock_irqsave(nm->common.lock, flags);

		/* most SoCs require M to be 0 if fractional mode is used */
		reg = readl(nm->common.base + nm->common.reg);
		reg &= ~GENMASK(nm->m.width + nm->m.shift - 1, nm->m.shift);
		writel(reg, nm->common.base + nm->common.reg);

		spin_unlock_irqrestore(nm->common.lock, flags);

		ccu_frac_helper_enable(&nm->common, &nm->frac);

		return ccu_frac_helper_set_rate(&nm->common, &nm->frac,
						rate, nm->lock);
	} else {
		ccu_frac_helper_disable(&nm->common, &nm->frac);
	}

	_nm.min_n = nm->n.min ?: 1;
	_nm.max_n = nm->n.max ?: 1 << nm->n.width;
	_nm.min_m = 1;
	_nm.max_m = nm->m.max ?: 1 << nm->m.width;

	if (ccu_sdm_helper_has_rate(&nm->common, &nm->sdm, rate)) {
		/* Sigma delta modulation requires specific N and M factors */
		ccu_sdm_helper_get_factors(&nm->common, &nm->sdm, rate,
					   &_nm.m, &_nm.n);
	} else {
		ccu_sdm_helper_disable(&nm->common, &nm->sdm);
		ccu_nm_find_best(parent_rate, rate, nm->m.table, &_nm);
	}

	spin_lock_irqsave(nm->common.lock, flags);

	reg = readl(nm->common.base + nm->common.reg);

	if (nm->n.width)
		reg &= ~GENMASK(nm->n.width + nm->n.shift - 1, nm->n.shift);
	if (nm->m.width)
		reg &= ~GENMASK(nm->m.width + nm->m.shift - 1, nm->m.shift);

	reg |= (_nm.n - nm->n.offset) << nm->n.shift;
	if (nm->m.table)
		reg |= (ccu_get_table_val(nm->m.table, _nm.m)) << nm->m.shift;
	else
		reg |= (_nm.m - nm->m.offset) << nm->m.shift;

	writel(reg, nm->common.base + nm->common.reg);

	spin_unlock_irqrestore(nm->common.lock, flags);

	if (ccu_sdm_helper_has_rate(&nm->common, &nm->sdm, rate))
		ccu_sdm_helper_enable(&nm->common, &nm->sdm, rate);

	spin_lock_irqsave(nm->common.lock, flags);

	if (nm->common.features & CCU_FEATURE_CLEAR_MOD)
		ccu_helper_wait_for_clear(&nm->common, nm->common.clear);

#if IS_ENABLED(CONFIG_AW_STANDARD_CCU)
	/* Enable lock enable */
	reg = readl(nm->common.base + nm->common.reg);
	reg &= ~(nm->lock_enable);
	writel(reg, nm->common.base + nm->common.reg);

	reg = readl(nm->common.base + nm->common.reg);
	reg |= (nm->lock_enable);
	writel(reg, nm->common.base + nm->common.reg);
#endif

	spin_unlock_irqrestore(nm->common.lock, flags);

	ccu_helper_wait_for_lock(&nm->common, nm->lock);

#if IS_ENABLED(CONFIG_AW_STANDARD_CCU)
	ccu_pll_output_helper_enable(&nm->common, nm->output);
#endif
	return 0;
}

const struct clk_ops ccu_nm_ops = {
	.disable	= ccu_nm_disable,
	.enable		= ccu_nm_enable,
	.is_enabled	= ccu_nm_is_enabled,

	.recalc_rate	= ccu_nm_recalc_rate,
	.round_rate	= ccu_nm_round_rate,
	.set_rate	= ccu_nm_set_rate,
	.init		= ccu_nm_init,
};
EXPORT_SYMBOL_GPL(ccu_nm_ops);
