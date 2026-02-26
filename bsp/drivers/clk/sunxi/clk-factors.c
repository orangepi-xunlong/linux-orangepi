/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2013 Allwinnertech, kevin.z.m <kevin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Adjustable factor-based clock implementation
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/delay.h>

#include "clk-sunxi.h"
#include "clk-factors.h"

static int sunxi_clk_disable_plllock(struct sunxi_clk_factors *factor)
{
	volatile u32 reg;

	switch (factor->lock_mode) {
	case PLL_LOCK_NEW_MODE:
	case PLL_LOCK_OLD_MODE:
		/* make sure pll new mode is disable */
		reg = factor_readl(factor, factor->pll_lock_ctrl_reg);
		reg = SET_BITS(factor->lock_en_bit, 1, reg, 0);
		factor_writel(factor, reg, factor->pll_lock_ctrl_reg);

		reg = factor_readl(factor, factor->pll_lock_ctrl_reg);
		reg = SET_BITS(28, 1, reg, 0);
		factor_writel(factor, reg, factor->pll_lock_ctrl_reg);
		break;
	case PLL_LOCK_NONE_MODE:
		break;
	default:
		WARN(1, "invaild pll lock mode:%u\n", factor->lock_mode);
		return -1;
	}

	return 0;
}

static int sunxi_clk_is_lock(struct sunxi_clk_factors *factor)
{
	volatile u32 reg;
	u32 loop = 5000;

	if (factor->lock_mode >= PLL_LOCK_MODE_MAX) {
		WARN(1, "invaild pll lock mode:%u\n", factor->lock_mode);
		return -1;
	}

	if (factor->lock_mode == PLL_LOCK_NEW_MODE) {
		/*
		 * bit28 is only read, remove it.
		 * reg = factor_readl(factor, factor->pll_lock_ctrl_reg);
		 * reg = SET_BITS(28, 1, reg, 1);
		 * factor_writel(factor, reg, factor->pll_lock_ctrl_reg);
		 */

		/* enable pll new mode */
		reg = factor_readl(factor, factor->pll_lock_ctrl_reg);
		reg = SET_BITS(factor->lock_en_bit, 1, reg, 1);
		factor_writel(factor, reg, factor->pll_lock_ctrl_reg);
	}

	while (--loop) {
		reg = factor_readl(factor, factor->lock_reg);
		if (GET_BITS(factor->lock_bit, 1, reg)) {
			udelay(20);
			break;
		}

		udelay(1);
	}

	if (factor->lock_mode == PLL_LOCK_NEW_MODE) {
		/* disable pll new mode */
		reg = factor_readl(factor, factor->pll_lock_ctrl_reg);
		reg = SET_BITS(factor->lock_en_bit, 1, reg, 0);
		factor_writel(factor, reg, factor->pll_lock_ctrl_reg);

		/*
		 * bit28 is only read, remove it.
		 * reg = factor_readl(factor, factor->pll_lock_ctrl_reg);
		 * reg = SET_BITS(28, 1, reg, 0);
		 * factor_writel(factor, reg, factor->pll_lock_ctrl_reg);
		 */
	}

	if (!loop) {
#if (defined CONFIG_FPGA_V4_PLATFORM) || (defined CONFIG_FPGA_V7_PLATFORM)
		pr_err("clk %s wait lock timeout\n",
		       clk_hw_get_name(&factor->hw));
		return 0;
#else
		return -1;
#endif
	}

	return 0;
}

#ifndef CONFIG_ARCH_SUN8IW12P1
static int sunxi_clk_fators_enable(struct clk_hw *hw)
{
	struct sunxi_clk_factors *factor = to_clk_factor(hw);
	struct sunxi_clk_factors_config *config = factor->config;
	unsigned long reg;
	unsigned long flags = 0;

	/* check if the pll enabled already */
	reg = factor_readl(factor, factor->reg);
	if (GET_BITS(config->enshift, 1, reg)) {
		if (factor->lock)
			spin_lock_irqsave(factor->lock, flags);
		sunxi_clk_disable_plllock(factor);
		/* get factor register value */
		reg = factor_readl(factor, factor->reg);
		goto enable_sdm;
	}

	if (factor->lock)
		spin_lock_irqsave(factor->lock, flags);

	sunxi_clk_disable_plllock(factor);

	/* get factor register value */
	reg = factor_readl(factor, factor->reg);

	/* enable the register */
	reg = SET_BITS(config->enshift, 1, reg, 1);

	/* update for pll_ddr register */
	if (config->updshift)
		reg = SET_BITS(config->updshift, 1, reg, 1);

	if (config->out_enshift)
		reg = SET_BITS(config->out_enshift, 1, reg, 1);

	if (config->mux_inshift)
		reg = SET_BITS(config->mux_inshift, 1, reg, 1);

enable_sdm:
	if (config->sdmwidth) {
		factor_writel(factor, config->sdmval,
			 (void __iomem *)config->sdmpat);
		reg = SET_BITS(config->sdmshift, config->sdmwidth, reg, 1);
	}

	factor_writel(factor, reg, factor->reg);

	if (sunxi_clk_is_lock(factor)) {
		if (factor->lock)
			spin_unlock_irqrestore(factor->lock, flags);
		WARN(1, "clk %s wait lock timeout\n", clk_hw_get_name(&factor->hw));
		return -1;
	}

	if (factor->lock)
		spin_unlock_irqrestore(factor->lock, flags);

	return 0;
}
#else
#define PLL_ENABLE_INIT_FACTOR_N	80
#define PLL_LOCK_TIMEOUT_CNT		5

static int sunxi_clk_fators_enable(struct clk_hw *hw)
{
	struct sunxi_clk_factors *factor = to_clk_factor(hw);
	struct sunxi_clk_factors_config *config = factor->config;
	unsigned long reg;
	unsigned long flags = 0;
	u16 factor_n;
	u8 i;

	/* check if the pll enabled already */
	reg = factor_readl(factor, factor->reg);
	if (GET_BITS(config->enshift, 1, reg)) {
		if (factor->lock)
			spin_lock_irqsave(factor->lock, flags);
		sunxi_clk_disable_plllock(factor);
		goto enable_sdm;
	}

	if (factor->lock)
		spin_lock_irqsave(factor->lock, flags);

	sunxi_clk_disable_plllock(factor);

	/* get factor register value */
	reg = factor_readl(factor, factor->reg);
	/* store the factor n */
	factor_n = (reg >> config->nshift) & 0xFF;

	/* enable the register */
	reg = SET_BITS(config->enshift, 1, reg, 1);


	/* config a larger value for factor_n */
	if (config->nwidth)
		reg = SET_BITS(config->nshift, config->nwidth, reg,
					 PLL_ENABLE_INIT_FACTOR_N);

	/* update for pll_ddr register */
	if (config->updshift)
		reg = SET_BITS(config->updshift, 1, reg, 1);

enable_sdm:
	if (config->sdmwidth) {
		factor_writel(factor, config->sdmval,
			(void __iomem *)config->sdmpat);
		reg = SET_BITS(config->sdmshift, config->sdmwidth, reg, 1);
	}

	factor_writel(factor, reg, factor->reg);

	for (i = 0; i < PLL_LOCK_TIMEOUT_CNT; i++) {
		if (sunxi_clk_is_lock(factor) == 0)
			break;

		reg = SET_BITS(config->enshift, 1, reg, 0);
		factor_writel(factor, reg, factor->reg);
		udelay(1);
		reg = SET_BITS(config->enshift, 1, reg, 1);
		factor_writel(factor, reg, factor->reg);
	}
	if (i == PLL_LOCK_TIMEOUT_CNT) {
		if (factor->lock)
			spin_unlock_irqrestore(factor->lock, flags);
		WARN(1, "clk %s wait lock timeout\n", clk_hw_get_name(&factor->hw));
		return -1;
	}

	if (config->nwidth)
		reg = SET_BITS(config->nshift, config->nwidth, reg, factor_n);

	factor_writel(factor, reg, factor->reg);

	if (sunxi_clk_is_lock(factor)) {
		if (factor->lock)
			spin_unlock_irqrestore(factor->lock, flags);
		WARN(1, "clk %s wait lock timeout\n", clk_hw_get_name(&factor->hw));
		return -1;
	}

	if (factor->lock)
		spin_unlock_irqrestore(factor->lock, flags);

	return 0;
}
#endif

static void sunxi_clk_fators_disable(struct clk_hw *hw)
{
	struct sunxi_clk_factors *factor = to_clk_factor(hw);
	struct sunxi_clk_factors_config *config = factor->config;
	unsigned long reg;
	unsigned long flags = 0;

	if (factor->flags & CLK_NO_DISABLE)
		return;

	/* check if the pll disabled already */
	reg = factor_readl(factor, factor->reg);
	if (!GET_BITS(config->enshift, 1, reg))
		return;

	/* When the pll is not in use, just set it to the minimum frequency */
	if (factor->flags & CLK_IGNORE_DISABLE) {
		/*
		clk_set_rate(hw->clk, 0);
		*/
		return;
	}

	if (factor->lock)
		spin_lock_irqsave(factor->lock, flags);

	reg = factor_readl(factor, factor->reg);
	if (config->sdmwidth)
		reg = SET_BITS(config->sdmshift, config->sdmwidth, reg, 0);

	/* update for pll_ddr register */
	if (config->updshift)
		reg = SET_BITS(config->updshift, 1, reg, 1);

	/* disable pll */
	reg = SET_BITS(config->enshift, 1, reg, 0);
	factor_writel(factor, reg, factor->reg);

	/* disable pll lock if needed */
	sunxi_clk_disable_plllock(factor);

	if (factor->lock)
		spin_unlock_irqrestore(factor->lock, flags);
}

static int sunxi_clk_fators_is_enabled(struct clk_hw *hw)
{
	unsigned long val;
	struct sunxi_clk_factors *factor = to_clk_factor(hw);
	struct sunxi_clk_factors_config *config = factor->config;
	unsigned long reg;
	unsigned long flags = 0;

	if (factor->flags & CLK_NO_DISABLE)
		return __clk_get_enable_count(hw->clk);

	if (factor->lock)
		spin_lock_irqsave(factor->lock, flags);

	reg = factor_readl(factor, factor->reg);
	val = GET_BITS(config->enshift, 1, reg);

	if (factor->lock)
		spin_unlock_irqrestore(factor->lock, flags);

	return val ? 1 : 0;
}

static u32 sunxi_clk_get_factor_sdmval(unsigned long rate, struct sunxi_clk_factors *factor, struct clk_factors_value *value)
{
	u32 sdm_val, sdm_freq, step_value, pre_div;
	u64 x2, wave_step;
	struct sunxi_clk_factors_config *config = factor->config;

	sdm_freq = 315 + factor->sdm_freq * 5;

	/* @TODO:we can't confirm the position of pre_div now */
	if (config->mwidth)
		pre_div = value->factorm;
	else
		pre_div = value->factord1;

	x2 = factor->sdm_factor * (value->factorn + 1);
	if (x2 >= 1000) {
		pr_err("clk: invalid sdm_factor: %d\n", factor->sdm_factor);
		return -1;
	}
	/*
	 * 1. pre_div=0
	 *	SDM_CLK_SEL->24M
	 *	fix coefficient=2^17*2/24 = 10922.5
	 * 2. pre_div=1
	 *	SDM_CLK_SEL->12M
	 *	fix coefficient=2^17*2/12 = 21845
	 **/
	if (pre_div)
		wave_step = 218450 * x2 * sdm_freq;
	else
		wave_step = 109225 * x2 * sdm_freq;

	do_div(wave_step, 100000000);
	step_value = (u32)wave_step;
	pr_debug("clk: wave_step:0x%x %s:%d\n", step_value, __func__, __LINE__);

	sdm_val = (wave_step << 20);
	/* enanle SDM */
	sdm_val = SET_BITS(31, 1, sdm_val, 1);
	/* choose freq_mode */
	sdm_val = SET_BITS(29, 2, sdm_val, factor->freq_mode << 1);
	/* choose sdm_freq */
	sdm_val = SET_BITS(17, 2, sdm_val, factor->sdm_freq);
	/* choose sdm_clk */
	sdm_val = SET_BITS(19, 1, sdm_val, pre_div);
	pr_debug("clk: sdm_val:0x%x %s:%d\n", sdm_val, __func__, __LINE__);
	return sdm_val;
}

static unsigned long sunxi_clk_factors_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	unsigned long reg;
	unsigned long p_reg;
	struct clk_factors_value factor_val;
	struct sunxi_clk_factors *factor = to_clk_factor(hw);
	struct sunxi_clk_factors_config *config = factor->config;
	unsigned long flags = 0;
	u32 sdmval;

	if (!factor->calc_rate)
		return 0;
	if (factor->lock)
		spin_lock_irqsave(factor->lock, flags);

	reg = factor_readl(factor, factor->reg);
	if (factor->lock)
		spin_unlock_irqrestore(factor->lock, flags);

	if (config->nwidth)
		factor_val.factorn = GET_BITS(config->nshift, config->nwidth, reg);
	else
		factor_val.factorn = 0xffff;

	if (config->kwidth)
		factor_val.factork = GET_BITS(config->kshift, config->kwidth, reg);
	else
		factor_val.factork = 0xffff;

	if (config->mwidth)
		factor_val.factorm = GET_BITS(config->mshift, config->mwidth, reg);
	else
		factor_val.factorm = 0xffff;

	if (config->pwidth && factor->p_reg) {
		p_reg = factor_readl(factor, factor->p_reg);
		factor_val.factorp = GET_BITS(config->pshift, config->pwidth, p_reg);
	} else if (config->pwidth) {
		factor_val.factorp = GET_BITS(config->pshift, config->pwidth, reg);
	} else {
		factor_val.factorp = 0xffff;
	}

	if (config->d1width)
		factor_val.factord1 = GET_BITS(config->d1shift, config->d1width, reg);
	else
		factor_val.factord1 = 0xffff;

	if (config->d2width)
		factor_val.factord2 = GET_BITS(config->d2shift, config->d2width, reg);
	else
		factor_val.factord2 = 0xffff;

	if (config->frac) {
		factor_val.frac_mode = GET_BITS(config->modeshift, 1, reg);
		factor_val.frac_freq = GET_BITS(config->outshift, 1, reg);
	} else {
		factor_val.frac_mode = 0xffff;
		factor_val.frac_freq = 0xffff;
	}

	if (factor->sdm_enable == DTS_SDM_ON) {
		sdmval = sunxi_clk_get_factor_sdmval(parent_rate, factor, &factor_val);
		pr_info("clk: name:%s have sdm_enable, val:0x%x\n", clk_hw_get_name(&factor->hw), sdmval);
		factor_writel(factor, sdmval, (void __iomem *)config->sdmpat);
		reg = SET_BITS(config->sdmshift, config->sdmwidth, reg, 1);
	} else if (factor->sdm_enable == CODE_SDM) {
		if (config->sdmwidth) {
			factor_writel(factor, config->sdmval, (void __iomem *)config->sdmpat);
			reg = SET_BITS(config->sdmshift, config->sdmwidth, reg, 1);
		}
	}
	factor_writel(factor, reg, factor->reg);

	return factor->calc_rate(parent_rate, &factor_val);
}

static long sunxi_clk_factors_round_rate(struct clk_hw *hw, unsigned long rate, unsigned long *prate)
{
	struct clk_factors_value factor_val;
	struct sunxi_clk_factors *factor = to_clk_factor(hw);

	if (!factor->get_factors || !factor->calc_rate)
		return rate;

	factor->get_factors(rate, *prate, &factor_val);
	return factor->calc_rate(*prate, &factor_val);
}

static int sunxi_clk_factors_set_flat_facotrs(struct sunxi_clk_factors *factor,
				struct clk_factors_value *values)
{
	struct sunxi_clk_factors_config *config = factor->config;
	u32 reg, tmp_factor_p, tmp_factor_m;
	unsigned long flags = 0;

	if (factor->lock)
		spin_lock_irqsave(factor->lock, flags);

	sunxi_clk_disable_plllock(factor);

	/*get all factors from the regitsters*/
	reg = factor_readl(factor, factor->reg);
	tmp_factor_p = config->pwidth ? GET_BITS(config->pshift, config->pwidth, reg) : 0;
	tmp_factor_m = config->mwidth ? GET_BITS(config->mshift, config->mwidth, reg) : 0;

	/* 1).try to increase factor p first */
	if (config->pwidth && (tmp_factor_p < values->factorp)) {
		reg = factor_readl(factor, factor->reg);
		reg = SET_BITS(config->pshift, config->pwidth, reg, values->factorp);
		factor_writel(factor, reg, factor->reg);
		if (factor->flags & CLK_RATE_FLAT_DELAY)
			udelay(config->delay);
	}

	/* 2).try to increase factor m first */
	if (config->mwidth && (tmp_factor_m < values->factorm)) {
		reg = factor_readl(factor, factor->reg);
		reg = SET_BITS(config->mshift, config->mwidth, reg, values->factorm);
		factor_writel(factor, reg, factor->reg);
		if (factor->flags & CLK_RATE_FLAT_DELAY)
			udelay(config->delay);
	}

	/* 3. write factor n & k */
	reg = factor_readl(factor, factor->reg);
	if (config->nwidth)
		reg = SET_BITS(config->nshift, config->nwidth, reg, values->factorn);
	if (config->kwidth)
		reg = SET_BITS(config->kshift, config->kwidth, reg, values->factork);
	factor_writel(factor, reg, factor->reg);

	/* 4. do pair things for 2). decease factor m */
	if (config->mwidth && (tmp_factor_m > values->factorm)) {
		reg = factor_readl(factor, factor->reg);
		reg = SET_BITS(config->mshift, config->mwidth, reg, values->factorm);
		factor_writel(factor, reg, factor->reg);
		if (factor->flags & CLK_RATE_FLAT_DELAY)
			udelay(config->delay);
	}

	/* 5. wait for PLL state stable */
	if (sunxi_clk_is_lock(factor)) {
		if (factor->lock)
			spin_unlock_irqrestore(factor->lock, flags);
		WARN(1, "clk %s wait lock timeout\n", clk_hw_get_name(&factor->hw));
		return -1;
	}

	/*6.do pair things for 1).  decease factor p */
	if (config->pwidth && (tmp_factor_p > values->factorp)) {
		reg = factor_readl(factor, factor->reg);
		reg = SET_BITS(config->pshift, config->pwidth, reg, values->factorp);
		factor_writel(factor, reg, factor->reg);
		if (factor->flags & CLK_RATE_FLAT_DELAY)
			udelay(config->delay);
	}

	if (factor->lock)
		spin_unlock_irqrestore(factor->lock, flags);

	return 0;
}

static int sunxi_clk_factors_set_rate(struct clk_hw *hw, unsigned long rate, unsigned long parent_rate)
{
	unsigned long reg;
	unsigned long p_reg;
	struct clk_factors_value factor_val;
	struct sunxi_clk_factors *factor = to_clk_factor(hw);
	struct sunxi_clk_factors_config *config = factor->config;
	unsigned long flags = 0;
	u32 sdmval;

	if (!factor->get_factors)
		return 0;

	/* factor_val is initialized with its original value,
	 * it's factors(such as:M,N,K,P,d1,d2...) are Random Value.
	 * if donot judge the return value of "factor->get_factors",
	 * it may change the original register value.
	 */
	if (factor->get_factors(rate, parent_rate, &factor_val) < 0) {
		/* cannot get right factors for clk,just break */
		WARN(1, "clk %s set rate failed! Because cannot get right factors for clk\n", clk_hw_get_name(hw));
		return 0;
	}

	if (factor->flags & CLK_RATE_FLAT_FACTORS)
		return sunxi_clk_factors_set_flat_facotrs(factor, &factor_val);

	if (factor->lock)
		spin_lock_irqsave(factor->lock, flags);

	sunxi_clk_disable_plllock(factor);

	reg = factor_readl(factor, factor->reg);
	if (factor->sdm_enable == DTS_SDM_ON) {
		sdmval = sunxi_clk_get_factor_sdmval(rate, factor, &factor_val);
		pr_info("clk: name:%s have sdm_enable, val:0x%x\n", clk_hw_get_name(&factor->hw), sdmval);
		factor_writel(factor, sdmval, (void __iomem *)config->sdmpat);
		reg = SET_BITS(config->sdmshift, config->sdmwidth, reg, 1);
	} else if (factor->sdm_enable == CODE_SDM) {
		if (config->sdmwidth) {
			factor_writel(factor, config->sdmval, (void __iomem *)config->sdmpat);
			reg = SET_BITS(config->sdmshift, config->sdmwidth, reg, 1);
		}
	}

	if (config->nwidth)
		reg = SET_BITS(config->nshift, config->nwidth, reg, factor_val.factorn);
	if (config->kwidth)
		reg = SET_BITS(config->kshift, config->kwidth, reg, factor_val.factork);
	if (config->mwidth)
		reg = SET_BITS(config->mshift, config->mwidth, reg, factor_val.factorm);
	if (config->pwidth && !factor->p_reg)
		reg = SET_BITS(config->pshift, config->pwidth, reg, factor_val.factorp);
	if (config->d1width)
		reg = SET_BITS(config->d1shift, config->d1width, reg, factor_val.factord1);
	if (config->d2width)
		reg = SET_BITS(config->d2shift, config->d2width, reg, factor_val.factord2);
	if (config->frac) {
		reg = SET_BITS(config->modeshift, 1, reg, factor_val.frac_mode);
		reg = SET_BITS(config->outshift, 1, reg, factor_val.frac_freq);
	}
	if (config->updshift)
		reg = SET_BITS(config->updshift, 1, reg, 1);
	factor_writel(factor, reg, factor->reg);

	/* cpu */
	if (factor->p_reg && config->pwidth) {
		p_reg = factor_readl(factor, factor->p_reg);
		p_reg = SET_BITS(config->pshift, config->pwidth, p_reg, factor_val.factorp);
		factor_writel(factor, p_reg, factor->p_reg);
	}

#ifndef CONFIG_SUNXI_CLK_DUMMY_DEBUG
	if (GET_BITS(config->enshift, 1, reg)) {
		if (sunxi_clk_is_lock(factor)) {
			if (factor->lock)
				spin_unlock_irqrestore(factor->lock, flags);
			WARN(1, "clk %s wait lock timeout\n", clk_hw_get_name(&factor->hw));
			return -1;
		}
	}
#endif

	if (factor->lock)
		spin_unlock_irqrestore(factor->lock, flags);

	return 0;
}

static const struct clk_ops clk_factors_ops = {
	.enable = sunxi_clk_fators_enable,
	.disable = sunxi_clk_fators_disable,
	.is_enabled = sunxi_clk_fators_is_enabled,

	.recalc_rate = sunxi_clk_factors_recalc_rate,
	.round_rate = sunxi_clk_factors_round_rate,
	.set_rate = sunxi_clk_factors_set_rate,
};

void sunxi_clk_get_factors_ops(struct clk_ops *ops)
{
	memcpy(ops, &clk_factors_ops, sizeof(clk_factors_ops));
}

/*
 * sunxi_clk_set_factor_lock_mode() - Set factor lock mode
 */
void sunxi_clk_set_factor_lock_mode(struct factor_init_data *factor,
		const char *lock_mode)
{
	if (!strcmp(lock_mode, "new"))
		factor->lock_mode = PLL_LOCK_NEW_MODE;
	else if (!strcmp(lock_mode, "old"))
		factor->lock_mode = PLL_LOCK_OLD_MODE;
	else
		factor->lock_mode = PLL_LOCK_NONE_MODE;
}

/*
 *  sunxi_clk_set_factor_sdm_info
 */
void sunxi_clk_set_factor_sdm_info(struct factor_init_data *factor,
		struct clk_sdm_info sdm_info)
{
	factor->sdm_enable = sdm_info.sdm_enable;
	factor->sdm_factor = sdm_info.sdm_factor;
	factor->freq_mode  = sdm_info.freq_mode;
	factor->sdm_freq   = sdm_info.sdm_freq;
	return;
}

/**
 * clk_register_factors - register a factors clock with
 * the clock framework
 * @dev: device registering this clock
 * @name: name of this clock
 * @parent_name: name of clock's parent
 * @flags: framework-specific flags
 * @reg: register address to adjust factors
 * @config: shift and width of factors n, k, m, p, div1 and div2
 * @get_factors: function to calculate the factors for a given frequency
 * @lock: shared register lock for this clock
 */
struct clk *sunxi_clk_register_factors(struct device *dev, void __iomem *base,
		spinlock_t *lock, struct factor_init_data *init_data)
{
	struct sunxi_clk_factors *factors;
	struct clk *clk;
	struct clk_init_data init;
#ifdef CONFIG_PM_SLEEP
	struct sunxi_factor_clk_reg_cache *factor_clk_reg;

	factor_clk_reg = kzalloc(sizeof(struct sunxi_factor_clk_reg_cache), GFP_KERNEL);
	if (!factor_clk_reg) {
		pr_err("%s: could not allocate factors clk reg\n", __func__);
		return ERR_PTR(-ENOMEM);
	}
#endif


	/* allocate the factors */
	factors = kzalloc(sizeof(struct sunxi_clk_factors), GFP_KERNEL);
	if (!factors) {
		pr_err("%s: could not allocate factors clk\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

#ifdef __SUNXI_ALL_CLK_IGNORE_UNUSED__
	init_data->flags |= CLK_IGNORE_UNUSED;
#endif
	init.name = init_data->name;
	init.ops = init_data->priv_ops ? (init_data->priv_ops) : (&clk_factors_ops);
	factors->priv_regops = init_data->priv_regops ? (init_data->priv_regops) : NULL;
	init.flags = init_data->flags;
	init.parent_names = init_data->parent_names;
	init.num_parents = init_data->num_parents;

	/* struct clk_factors assignments */
	factors->reg = base + init_data->reg;
	if (init_data->config->cpu_reg)
		factors->p_reg = base + init_data->config->cpu_reg;
	else
		factors->p_reg = NULL;
	factors->lock_reg = base + init_data->lock_reg;
	factors->lock_bit = init_data->lock_bit;
	factors->pll_lock_ctrl_reg = base + init_data->pll_lock_ctrl_reg;
	factors->lock_en_bit = init_data->lock_en_bit;
	factors->lock_mode = init_data->lock_mode;
	factors->config = init_data->config;
	factors->config->sdmpat = factors->config->sdmpat ?  (unsigned long __force)(base + factors->config->sdmpat) : 0;
	factors->lock = lock;
	factors->hw.init = &init;
	factors->get_factors = init_data->get_factors;
	factors->calc_rate = init_data->calc_rate;
	factors->flags = init_data->flags;
	factors->sdm_enable = init_data->sdm_enable;
	factors->sdm_factor = init_data->sdm_factor;
	factors->freq_mode = init_data->freq_mode;
	factors->sdm_freq = init_data->sdm_freq;
#ifdef CONFIG_PM_SLEEP
	if (!strcmp(init.name, "pll_cpu") ||
		!strcmp(init.name, "pll_ddr0") ||
#ifdef CONFIG_ARCH_SUN50IW10
		!strcmp(init.name, "pll_com") ||
#endif
#ifdef CONFIG_ARCH_SUN50IW11
		!strcmp(init.name, "pll_periph0") ||
		!strcmp(init.name, "pll_audio0") ||
		!strcmp(init.name, "pll_audio1") ||
#endif
		!strcmp(init.name, "pll_ddr1")) {
		kfree(factor_clk_reg);
	} else {
		factor_clk_reg->config_reg = factors->reg;
		factor_clk_reg->sdmpat_reg = (void *)factors->config->sdmpat;
		list_add_tail(&factor_clk_reg->node, &clk_factor_reg_cache_list);
	}
#endif
	/* register the clock */
	clk = clk_register(dev, &factors->hw);
	factors->hw.init = NULL;

	if (IS_ERR(clk))
		kfree(factors);

	return clk;
}

int sunxi_clk_get_common_factors(struct sunxi_clk_factors_config *f_config, struct clk_factors_value *factor,
		struct sunxi_clk_factor_freq table[], unsigned long index, unsigned long tbl_size)
{
	if (index >= tbl_size/sizeof(struct sunxi_clk_factor_freq))
		return -1;

	factor->factorn = (table[index].factor>>f_config->nshift)&((1<<(f_config->nwidth))-1);
	factor->factork = (table[index].factor>>f_config->kshift)&((1<<(f_config->kwidth))-1);
	factor->factorm = (table[index].factor>>f_config->mshift)&((1<<(f_config->mwidth))-1);
	factor->factorp = (table[index].factor>>f_config->pshift)&((1<<(f_config->pwidth))-1);
	factor->factord1 = (table[index].factor>>f_config->d1shift)&((1<<(f_config->d1width))-1);
	factor->factord2 = (table[index].factor>>f_config->d2shift)&((1<<(f_config->d2width))-1);

	if (f_config->frac) {
		factor->frac_mode = (table[index].factor>>f_config->modeshift)&1;
		factor->frac_freq = (table[index].factor>>f_config->outshift)&1;
	}

	return 0;
}

static int sunxi_clk_freq_search(struct sunxi_clk_factor_freq tbl[],
				unsigned long freq, int low, int high)
{
	int mid;
	unsigned long checkfreq;

	if (low > high)
		return (high == -1) ? 0 : high;

	mid = (low + high)/2;
	checkfreq = tbl[mid].freq/1000000;

	if (checkfreq == freq)
		return mid;
	else if (checkfreq > freq)
		return sunxi_clk_freq_search(tbl, freq, low, mid - 1);
	else
		return sunxi_clk_freq_search(tbl, freq, mid + 1, high);
}

static int sunxi_clk_freq_find(struct sunxi_clk_factor_freq tbl[],
				unsigned long n, unsigned long freq)
{
	int delta1, delta2;
	int i = sunxi_clk_freq_search(tbl, freq, 0, n-1);

	if (i != n-1) {

		delta1 = (freq > tbl[i].freq / 1000000)
			? (freq - tbl[i].freq / 1000000)
			: (tbl[i].freq / 1000000 - freq);

		delta2 = (freq > tbl[i+1].freq / 1000000)
			? (freq - tbl[i+1].freq / 1000000)
			: (tbl[i+1].freq / 1000000 - freq);

		if (delta2 < delta1)
			i++;
	}

	return i;
}

int sunxi_clk_com_ftr_sr(struct sunxi_clk_factors_config *f_config,
				struct clk_factors_value *factor,
				struct sunxi_clk_factor_freq table[],
				unsigned long index, unsigned long tbl_count)
{
	int i = sunxi_clk_freq_find(table, tbl_count, index);

	if (i >= tbl_count)
		return -1;

	factor->factorn = (table[i].factor >> f_config->nshift)&((1<<(f_config->nwidth))-1);
	factor->factork = (table[i].factor>>f_config->kshift)&((1<<(f_config->kwidth))-1);
	factor->factorm = (table[i].factor>>f_config->mshift)&((1<<(f_config->mwidth))-1);
	factor->factorp = (table[i].factor>>f_config->pshift)&((1<<(f_config->pwidth))-1);
	factor->factord1 = (table[i].factor>>f_config->d1shift)&((1<<(f_config->d1width))-1);
	factor->factord2 = (table[i].factor>>f_config->d2shift)&((1<<(f_config->d2width))-1);

	if (f_config->frac) {
		factor->frac_mode = (table[i].factor>>f_config->modeshift)&1;
		factor->frac_freq = (table[i].factor>>f_config->outshift)&1;
	}

	return 0;
}
