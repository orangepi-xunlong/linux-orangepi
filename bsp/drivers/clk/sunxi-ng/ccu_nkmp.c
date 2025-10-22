// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2016 Maxime Ripard
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/version.h>
#include <linux/slab.h>

#include "ccu_gate.h"
#include "ccu_nkmp.h"
#include "ccu-sunxi-trace.h"

struct _ccu_nkmp {
	unsigned long	n, min_n, max_n;
	unsigned long	k, min_k, max_k;
	unsigned long	m, min_m, max_m;
	unsigned long	p, min_p, max_p;
};

static u64 ccu_nkmp_calc_rate(unsigned long parent,
					unsigned long n, unsigned long k,
					unsigned long m, unsigned long p)
{
	u64 rate = parent;

	rate *= n * k;
	do_div(rate, m * p);

	return rate;
}

static void ccu_nkmp_find_best(struct ccu_common *common, unsigned long parent, u64 rate,
			       struct _ccu_nkmp *nkmp)
{
	u64 best_rate = 0;
	unsigned long best_n = 0, best_k = 0, best_m = 0, best_p = 0;
	unsigned long _n, _k, _m, _p;
	struct _ccu_nkmp_cache *nkmp_cache;

	if (common->features & CCU_FEATURE_CLAC_CACHED) {
		list_for_each_entry(nkmp_cache, &common->calc_cache, node) {
			if (nkmp_cache->rate != rate)
				continue;

			nkmp->n = nkmp_cache->n;
			nkmp->k = nkmp_cache->k;
			nkmp->m = nkmp_cache->m;
			nkmp->p = nkmp_cache->p;
			sunxi_debug(NULL, "nkmp_cache-clk:%s-n:%d-k:%d-m:%d-p:%d\n",
				     clk_hw_get_name(&common->hw), nkmp_cache->n, nkmp_cache->k,
					nkmp_cache->m, nkmp_cache->p);
			return;
		}
	}
	for (_k = nkmp->min_k; _k <= nkmp->max_k; _k++) {
		for (_n = nkmp->min_n; _n <= nkmp->max_n; _n++) {
			for (_m = nkmp->min_m; _m <= nkmp->max_m; _m++) {
				for (_p = nkmp->min_p; _p <= nkmp->max_p; _p <<= 1) {
					u64 tmp_rate;

					tmp_rate = ccu_nkmp_calc_rate(parent,
								      _n, _k,
								      _m, _p);

					if (tmp_rate > rate)
						continue;

					if ((rate - tmp_rate) < (rate - best_rate)) {
						best_rate = tmp_rate;
						best_n = _n;
						best_k = _k;
						best_m = _m;
						best_p = _p;
					}
				}
			}
		}
	}

	nkmp->n = best_n;
	nkmp->k = best_k;
	nkmp->m = best_m;
	nkmp->p = best_p;

	if (common->features & CCU_FEATURE_CLAC_CACHED) {
		nkmp_cache = kzalloc(sizeof(*nkmp_cache), GFP_KERNEL);
		if (!nkmp_cache)
			sunxi_info(NULL, "could not allocate nkmp cache!\n");

		nkmp_cache->n = best_n;
		nkmp_cache->k = best_k;
		nkmp_cache->m = best_m;
		nkmp_cache->p = best_p;
		nkmp_cache->rate = rate;

		list_add(&nkmp_cache->node, &common->calc_cache);
	}
}

static void ccu_nkmp_disable(struct clk_hw *hw)
{
	struct ccu_nkmp *nkmp = hw_to_ccu_nkmp(hw);

#if IS_ENABLED(CONFIG_AW_STANDARD_CCU)
	return ccu_pll_gate_helper_disable(&nkmp->common, nkmp->enable, nkmp->output, nkmp->lock_enable, nkmp->ldo_en);
#else
	return ccu_gate_helper_disable(&nkmp->common, nkmp->enable);
#endif
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ccu_nkmp_init(struct clk_hw *hw)
{
	struct ccu_common *common = hw_to_ccu_common(hw);

	if (!(common->features & CCU_FEATURE_INIT_GATE))
		return 0;

	if ((clk_hw_get_flags(hw) & CLK_IS_CRITICAL) || (clk_hw_get_flags(hw) & CLK_IGNORE_UNUSED))
		return 0;

	ccu_nkmp_disable(hw);

	return 0;
}
#else
static void ccu_nkmp_init(struct clk_hw *hw)
{
}
#endif

static int ccu_nkmp_enable(struct clk_hw *hw)
{
	struct ccu_nkmp *nkmp = hw_to_ccu_nkmp(hw);

#if IS_ENABLED(CONFIG_AW_STANDARD_CCU)
	return ccu_pll_gate_helper_enable(&nkmp->common, nkmp->enable, nkmp->output, nkmp->lock, nkmp->lock_enable, nkmp->ldo_en);
#else
	return ccu_gate_helper_enable(&nkmp->common, nkmp->enable);
#endif
}

static int ccu_nkmp_is_enabled(struct clk_hw *hw)
{
	struct ccu_nkmp *nkmp = hw_to_ccu_nkmp(hw);

	return ccu_gate_helper_is_enabled(&nkmp->common, nkmp->enable);
}

static unsigned long ccu_nkmp_recalc_rate(struct clk_hw *hw,
					unsigned long parent_rate)
{
	struct ccu_nkmp *nkmp = hw_to_ccu_nkmp(hw);
	unsigned long n, m, k, p, rate;
	u32 reg, sdmval;

	reg = readl(nkmp->common.base + nkmp->common.reg);

	n = reg >> nkmp->n.shift;
	n &= (1 << nkmp->n.width) - 1;
	n += nkmp->n.offset;
	if (!n)
		n++;

	k = reg >> nkmp->k.shift;
	k &= (1 << nkmp->k.width) - 1;
	k += nkmp->k.offset;
	if (!k)
		k++;

	m = reg >> nkmp->m.shift;
	m &= (1 << nkmp->m.width) - 1;
	m += nkmp->m.offset;
	if (!m)
		m++;

	if (nkmp->p_reg)
		reg = readl(nkmp->common.base + nkmp->p_reg);

	p = reg >> nkmp->p.shift;
	p &= (1 << nkmp->p.width) - 1;

	rate = ccu_nkmp_calc_rate(parent_rate, n, k, m, 1 << p);
	if (nkmp->common.features & CCU_FEATURE_FIXED_POSTDIV)
		rate /= nkmp->fixed_post_div;

	if (nkmp->common.sdm_info) {
		sdmval = ccu_get_sdmval(rate, &nkmp->common, m, n);
		ccu_common_set_sdm_value(&nkmp->common, &nkmp->sdm, sdmval);
	}
	return rate;
}

static long ccu_nkmp_round_rate(struct clk_hw *hw, unsigned long _rate,
			      unsigned long *parent_rate)
{
	struct ccu_nkmp *nkmp = hw_to_ccu_nkmp(hw);
	struct _ccu_nkmp _nkmp;
	u64 rate = _rate;

	if (nkmp->common.features & CCU_FEATURE_FIXED_POSTDIV)
		rate *= nkmp->fixed_post_div;

	if (nkmp->max_rate && rate > nkmp->max_rate) {
		rate = nkmp->max_rate;
		if (nkmp->common.features & CCU_FEATURE_FIXED_POSTDIV)
			do_div(rate, nkmp->fixed_post_div);
		return rate;
	}

	_nkmp.min_n = nkmp->n.min ?: 1;
	_nkmp.max_n = nkmp->n.max ?: 1 << nkmp->n.width;
	_nkmp.min_k = nkmp->k.min ?: 1;
	_nkmp.max_k = nkmp->k.max ?: 1 << nkmp->k.width;
	_nkmp.min_m = 1;
	_nkmp.max_m = nkmp->m.max ?: 1 << nkmp->m.width;
	_nkmp.min_p = 1;
	_nkmp.max_p = nkmp->p.max ?: 1 << ((1 << nkmp->p.width) - 1);

	ccu_nkmp_find_best(&nkmp->common, *parent_rate, rate, &_nkmp);

	rate = ccu_nkmp_calc_rate(*parent_rate, _nkmp.n, _nkmp.k,
				  _nkmp.m, _nkmp.p);
	if (nkmp->common.features & CCU_FEATURE_FIXED_POSTDIV)
		do_div(rate, nkmp->fixed_post_div);

	return rate;
}

static int ccu_nkmp_set_rate(struct clk_hw *hw, unsigned long _rate,
			   unsigned long parent_rate)
{
	struct ccu_nkmp *nkmp = hw_to_ccu_nkmp(hw);
	u32 n_mask = 0, k_mask = 0, m_mask = 0, p_mask = 0;
	struct _ccu_nkmp _nkmp;
	unsigned long flags;
	u64 rate = _rate;
	u32 reg;
	u32 back_p, new_p;

	if (nkmp->common.features & CCU_FEATURE_FIXED_POSTDIV)
		rate = rate * nkmp->fixed_post_div;

	_nkmp.min_n = nkmp->n.min ?: 1;
	_nkmp.max_n = nkmp->n.max ?: 1 << nkmp->n.width;
	_nkmp.min_k = nkmp->k.min ?: 1;
	_nkmp.max_k = nkmp->k.max ?: 1 << nkmp->k.width;
	_nkmp.min_m = 1;
	_nkmp.max_m = nkmp->m.max ?: 1 << nkmp->m.width;
	_nkmp.min_p = 1;
	_nkmp.max_p = nkmp->p.max ?: 1 << ((1 << nkmp->p.width) - 1);

	ccu_nkmp_find_best(&nkmp->common, parent_rate, rate, &_nkmp);

	/*
	 * If width is 0, GENMASK() macro may not generate expected mask (0)
	 * as it falls under undefined behaviour by C standard due to shifts
	 * which are equal or greater than width of left operand. This can
	 * be easily avoided by explicitly checking if width is 0.
	 */
	if (nkmp->n.width)
		n_mask = GENMASK(nkmp->n.width + nkmp->n.shift - 1,
				 nkmp->n.shift);
	if (nkmp->k.width)
		k_mask = GENMASK(nkmp->k.width + nkmp->k.shift - 1,
				 nkmp->k.shift);
	if (nkmp->m.width)
		m_mask = GENMASK(nkmp->m.width + nkmp->m.shift - 1,
				 nkmp->m.shift);
	if (nkmp->p.width)
		p_mask = GENMASK(nkmp->p.width + nkmp->p.shift - 1,
				 nkmp->p.shift);

	spin_lock_irqsave(nkmp->common.lock, flags);

	reg = readl(nkmp->common.base + nkmp->common.reg);
	reg &= ~(n_mask | k_mask | m_mask | p_mask);

	reg |= ((_nkmp.n - nkmp->n.offset) << nkmp->n.shift) & n_mask;
	reg |= ((_nkmp.k - nkmp->k.offset) << nkmp->k.shift) & k_mask;
	reg |= ((_nkmp.m - nkmp->m.offset) << nkmp->m.shift) & m_mask;

	if (!nkmp->p_reg) {
		reg |= (ilog2(_nkmp.p) << nkmp->p.shift) & p_mask;
		writel(reg, nkmp->common.base + nkmp->common.reg);
	} else {
		back_p = get_field(nkmp->common.base + nkmp->p_reg, BITS_WIDTH(nkmp->p.shift, nkmp->p.width));
		new_p = ilog2(_nkmp.p);
		if (new_p > back_p) {
			set_reg(nkmp->common.base + nkmp->p_reg, new_p,
				nkmp->p.width, nkmp->p.shift);
			writel(reg, nkmp->common.base + nkmp->common.reg);
		} else {
			writel(reg, nkmp->common.base + nkmp->common.reg);
		}
	}

	if (nkmp->common.features & CCU_FEATURE_CLEAR_MOD && nkmp->common.clear) {
		reg |= nkmp->common.clear;
		writel(reg, nkmp->common.base + nkmp->common.reg);
		WARN_ON(readl_relaxed_poll_timeout_atomic(nkmp->common.base + nkmp->common.reg, reg, !(reg & nkmp->common.clear), 100, 10000));
	}

	if (nkmp->p_reg) {
		if (new_p < back_p)
			set_reg(nkmp->common.base + nkmp->p_reg, new_p,
				nkmp->p.width, nkmp->p.shift);
	}
	spin_unlock_irqrestore(nkmp->common.lock, flags);

	trace_clk_nkmp_set_rate(hw, rate, parent_rate, reg);

	ccu_helper_wait_for_lock(&nkmp->common, nkmp->lock);

	return 0;
}

const struct clk_ops ccu_nkmp_ops = {
	.disable	= ccu_nkmp_disable,
	.enable		= ccu_nkmp_enable,
	.is_enabled	= ccu_nkmp_is_enabled,

	.recalc_rate	= ccu_nkmp_recalc_rate,
	.round_rate	= ccu_nkmp_round_rate,
	.set_rate	= ccu_nkmp_set_rate,
	.init		= ccu_nkmp_init,
};
EXPORT_SYMBOL_GPL(ccu_nkmp_ops);
