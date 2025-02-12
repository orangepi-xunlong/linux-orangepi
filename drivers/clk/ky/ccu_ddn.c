// SPDX-License-Identifier: GPL-2.0-only
/*
 * Ky clock type ddn
 *
 * Copyright (c) 2023, ky Corporation.
 *
 */

#include <linux/clk-provider.h>
#include <linux/io.h>

#include "ccu_ddn.h"
/*
 * It is M/N clock
 *
 * Fout from synthesizer can be given from two equations:
 * numerator/denominator = Fin / (Fout * factor)
 */

static void ccu_ddn_disable(struct clk_hw *hw)
{
	struct ccu_ddn *ddn = hw_to_ccu_ddn(hw);
	struct ccu_common * common = &ddn->common;
	unsigned long flags;
	u32 reg;

	if (!ddn->gate)
		return;

	spin_lock_irqsave(common->lock, flags);

	reg = readl(common->base + common->reg_sel);

	writel(reg & ~ddn->gate, common->base + common->reg_sel);

	spin_unlock_irqrestore(common->lock, flags);
}

static int ccu_ddn_enable(struct clk_hw *hw)
{
	struct ccu_ddn *ddn = hw_to_ccu_ddn(hw);
	struct ccu_common * common = &ddn->common;
	unsigned long flags;
	u32 reg;

	if (!ddn->gate)
		return 0;

	spin_lock_irqsave(common->lock, flags);

	reg = readl(common->base + common->reg_sel);

	writel(reg | ddn->gate, common->base + common->reg_sel);

	spin_unlock_irqrestore(common->lock, flags);

	return 0;
}

static int ccu_ddn_is_enabled(struct clk_hw *hw)
{
	struct ccu_ddn *ddn = hw_to_ccu_ddn(hw);
	struct ccu_common * common = &ddn->common;

	if (!ddn->gate)
		return 1;

	return readl(common->base + common->reg_sel) & ddn->gate;
}

static long clk_ddn_round_rate(struct clk_hw *hw, unsigned long drate,
		unsigned long *prate)
{
	struct ccu_ddn *ddn = hw_to_ccu_ddn(hw);
	struct ccu_ddn_config *params = &ddn->ddn;
	unsigned long rate = 0, prev_rate;
	unsigned long result;
	int i;

	for (i = 0; i < params->tbl_size; i++) {
		prev_rate = rate;
		rate = (((*prate / 10000) * params->tbl[i].den) /
			(params->tbl[i].num * params->info->factor)) * 10000;
		if (rate > drate)
			break;
	}
	if ((i == 0) || (i == params->tbl_size)) {
		result = rate;
	} else {
		if ((drate - prev_rate) > (rate - drate))
			result = rate;
		else
			result = prev_rate;
	}
	return result;
}

static unsigned long clk_ddn_recalc_rate(struct clk_hw *hw,
		unsigned long parent_rate)
{
	struct ccu_ddn *ddn = hw_to_ccu_ddn(hw);
	struct ccu_ddn_config *params = &ddn->ddn;
	unsigned int val, num, den;
	unsigned long rate;

	val = readl(ddn->common.base + ddn->common.reg_ctrl);

	/* calculate numerator */
	num = (val >> params->info->num_shift) & params->info->num_mask;

	/* calculate denominator */
	den = (val >> params->info->den_shift) & params->info->den_mask;

	if (!den)
		return 0;
	rate = (((parent_rate / 10000)  * den) /
			(num * params->info->factor)) * 10000;
	return rate;
}

/* Configures new clock rate*/
static int clk_ddn_set_rate(struct clk_hw *hw, unsigned long drate,
				unsigned long prate)
{
	struct ccu_ddn *ddn = hw_to_ccu_ddn(hw);
	struct ccu_ddn_config *params = &ddn->ddn;
	int i;
	unsigned long val;
	unsigned long prev_rate, rate = 0;
	unsigned long flags = 0;

	for (i = 0; i < params->tbl_size; i++) {
		prev_rate = rate;
		rate = (((prate / 10000) * params->tbl[i].den) /
			(params->tbl[i].num * params->info->factor)) * 10000;
		if (rate > drate)
			break;
	}

	if (i > 0)
		i--;

	if (ddn->common.lock)
		spin_lock_irqsave(ddn->common.lock, flags);

	val = readl(ddn->common.base + ddn->common.reg_ctrl);

	val &= ~(params->info->num_mask << params->info->num_shift);
	val |= (params->tbl[i].num & params->info->num_mask) << params->info->num_shift;

	val &= ~(params->info->den_mask << params->info->den_shift);
	val |= (params->tbl[i].den & params->info->den_mask) << params->info->den_shift;

	writel(val, ddn->common.base + ddn->common.reg_ctrl);

	if (ddn->common.lock)
		spin_unlock_irqrestore(ddn->common.lock, flags);

	return 0;
}

const struct clk_ops ccu_ddn_ops = {
    .disable	 = ccu_ddn_disable,
	.enable		 = ccu_ddn_enable,
	.is_enabled	 = ccu_ddn_is_enabled,
	.recalc_rate = clk_ddn_recalc_rate,
	.round_rate = clk_ddn_round_rate,
	.set_rate = clk_ddn_set_rate,
};

