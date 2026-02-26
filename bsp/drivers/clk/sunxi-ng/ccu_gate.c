// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2016 Maxime Ripard
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/version.h>

#include "ccu_gate.h"
#include "ccu-sunxi-trace.h"

void ccu_pll_output_helper_disable(struct ccu_common *common, u32 output)
{
	unsigned long flags;
	u32 reg;

	spin_lock_irqsave(common->lock, flags);

	reg = readl(common->base + common->reg);
	writel(reg & ~output, common->base + common->reg);

	spin_unlock_irqrestore(common->lock, flags);
}
void ccu_pll_output_helper_enable(struct ccu_common *common, u32 output)
{
	unsigned long flags;
	u32 reg;

	spin_lock_irqsave(common->lock, flags);

	reg = readl(common->base + common->reg);
	writel(reg | output, common->base + common->reg);

	spin_unlock_irqrestore(common->lock, flags);
}

void ccu_pll_gate_helper_disable(struct ccu_common *common, u32 gate, u32 output, u32 lock_en, u32 ldo_en)
{
	unsigned long flags;
	u32 reg;

	if (!gate)
		return;

	spin_lock_irqsave(common->lock, flags);

	/* Disable PLL_OUTPUT_GATE */
	reg = readl(common->base + common->reg);
	writel(reg & ~output, common->base + common->reg);

	/* Disable PLL_LOCK_ENABLE */
	reg = readl(common->base + common->reg);
	writel(reg & ~lock_en, common->base + common->reg);

	spin_unlock_irqrestore(common->lock, flags);

	/* Disable PLL_EN */
	ccu_gate_helper_disable(common, gate);

	spin_lock_irqsave(common->lock, flags);
	/* Disable PLL_LDO*/
	reg = readl(common->base + common->reg);
	writel(reg & ~ldo_en, common->base + common->reg);

	spin_unlock_irqrestore(common->lock, flags);
}

void ccu_gate_helper_disable(struct ccu_common *common, u32 gate)
{
	unsigned long flags;
	u32 reg;
	u32 assoc_reg;
	u32 key_reg;

	if (!gate)
		return;

	spin_lock_irqsave(common->lock, flags);

	reg = readl(common->base + common->reg);

	/* data reading result of the keyfield bits are always 0 */
	if (common->features & CCU_FEATURE_KEY_FIELD_MOD) {
		if (common->reg == common->key_reg) {
			reg = reg | common->key_value;
		} else {
			key_reg = readl(common->base + common->key_reg);
			key_reg = key_reg | common->key_value;
			writel(key_reg, common->base + common->key_reg);
		}
	}

	if (common->features & CCU_FEATURE_GATE_IS_REVERSE)
		writel(reg | gate, common->base + common->reg);
	else
		writel(reg & ~gate, common->base + common->reg);

	if (common->features & CCU_FEATURE_CLEAR_MOD)
		ccu_helper_wait_for_clear(common, common->clear);

	if (common->features & CCU_FEATURE_GATE_DOUBLE_REG) {
		assoc_reg = readl(common->base + common->assoc_reg);
		if (common->features & CCU_FEATURE_KEY_FIELD_MOD)
			assoc_reg = assoc_reg | common->assoc_key_value;
		writel(assoc_reg & ~(common->assoc_val), common->base + common->assoc_reg);
	}

	spin_unlock_irqrestore(common->lock, flags);
}

static void ccu_gate_disable(struct clk_hw *hw)
{
	struct ccu_gate *cg = hw_to_ccu_gate(hw);

	return ccu_gate_helper_disable(&cg->common, cg->enable);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ccu_gate_init(struct clk_hw *hw)
{
	struct ccu_common *common = hw_to_ccu_common(hw);

	if (!(common->features & CCU_FEATURE_INIT_GATE))
		return 0;

	if ((clk_hw_get_flags(hw) & CLK_IS_CRITICAL) || (clk_hw_get_flags(hw) & CLK_IGNORE_UNUSED))
		return 0;

	ccu_gate_disable(hw);

	return 0;
}
#else
static void ccu_gate_init(struct clk_hw *hw)
{
}
#endif

void ccu_common_helper_enable(struct ccu_common *common)
{
	u32 assoc_reg;

	if (common->features & CCU_FEATURE_CLEAR_MOD)
		ccu_helper_wait_for_clear(common, common->clear);

	if (common->features & CCU_FEATURE_GATE_DOUBLE_REG) {
		assoc_reg = readl(common->base + common->assoc_reg);
		if (common->features & CCU_FEATURE_KEY_FIELD_MOD)
			assoc_reg = assoc_reg | common->assoc_key_value;
		writel(assoc_reg | common->assoc_val, common->base + common->assoc_reg);
	}
}

int ccu_pll_gate_helper_enable(struct ccu_common *common, u32 gate, u32 output, u32 lock, u32 lock_enable, u32 ldo_en)
{
	unsigned long flags;
	u32 reg;

	if (!gate)
		return 0;

	spin_lock_irqsave(common->lock, flags);

	reg = readl(common->base + common->reg);

	sunxi_debug(NULL, "%s is 0x%x gate: 0x%x output: 0x%x lock: 0x%x\n", clk_hw_get_name(&common->hw), reg, gate, output, lock);

	if (output && ldo_en && (reg & gate) && (reg & output) && (reg & ldo_en)) {
		spin_unlock_irqrestore(common->lock, flags);
		return 0;
	}

	if (output) {
		reg = readl(common->base + common->reg);
		writel(reg & ~output, common->base + common->reg);
	}

	/* Enable ldo */
	if (ldo_en) {
		reg = readl(common->base + common->reg);
		writel(reg | ldo_en, common->base + common->reg);
	}

	/* Enable enable */
	reg = readl(common->base + common->reg);
	writel(reg | gate, common->base + common->reg);

	/* Enable lock enable */
	if (lock_enable) {
		reg = readl(common->base + common->reg);
		writel(reg | lock_enable, common->base + common->reg);
	}

	ccu_common_helper_enable(common);
	ccu_helper_wait_for_lock(common, lock);

	if (output) {
		reg = readl(common->base + common->reg);
		writel(reg | output, common->base + common->reg);
	}

	spin_unlock_irqrestore(common->lock, flags);
	trace_clk_ng_enable(&common->hw);

	sunxi_debug(NULL, "%s is 0x%x \n", clk_hw_get_name(&common->hw), readl(common->base + common->reg));

	return 0;
}

int ccu_gate_helper_enable(struct ccu_common *common, u32 gate)
{
	unsigned long flags;
	u32 reg;
	u32 key_reg;

	if (!gate)
		return 0;

	spin_lock_irqsave(common->lock, flags);

	reg = readl(common->base + common->reg);

	/* data reading result of the keyfield bits are always 0 */
	if (common->features & CCU_FEATURE_KEY_FIELD_MOD) {
		if (common->reg == common->key_reg) {
			reg = reg | common->key_value;
		} else {
			key_reg = readl(common->base + common->key_reg);
			key_reg = key_reg | common->key_value;
			writel(key_reg, common->base + common->key_reg);
		}
	}

	if (common->features & CCU_FEATURE_GATE_IS_REVERSE)
		writel(reg & ~gate, common->base + common->reg);
	else
		writel(reg | gate, common->base + common->reg);

	ccu_common_helper_enable(common);

	spin_unlock_irqrestore(common->lock, flags);
	trace_clk_ng_enable(&common->hw);

	return 0;
}

static int ccu_gate_enable(struct clk_hw *hw)
{
	struct ccu_gate *cg = hw_to_ccu_gate(hw);

	return ccu_gate_helper_enable(&cg->common, cg->enable);
}

int ccu_gate_helper_is_enabled(struct ccu_common *common, u32 gate)
{
	unsigned long val;

	if (!gate)
		return 1;

	val = readl(common->base + common->reg) & gate;

	if (common->features & CCU_FEATURE_GATE_IS_REVERSE)
		val = !val;

	return val;
}

static int ccu_gate_is_enabled(struct clk_hw *hw)
{
	struct ccu_gate *cg = hw_to_ccu_gate(hw);

	return ccu_gate_helper_is_enabled(&cg->common, cg->enable);
}

static unsigned long ccu_gate_recalc_rate(struct clk_hw *hw,
					  unsigned long parent_rate)
{
	struct ccu_gate *cg = hw_to_ccu_gate(hw);
	unsigned long rate = parent_rate;

	if (cg->common.features & CCU_FEATURE_FIXED_RATE_GATE)
		return cg->fixed_rate;

	if (cg->common.features & CCU_FEATURE_ALL_PREDIV)
		rate /= cg->common.prediv;

	return rate;
}

static long ccu_gate_round_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long *prate)
{
	struct ccu_gate *cg = hw_to_ccu_gate(hw);
	int div = 1;

	if (cg->common.features & CCU_FEATURE_FIXED_RATE_GATE)
		return cg->fixed_rate;

	if (cg->common.features & CCU_FEATURE_ALL_PREDIV)
		div = cg->common.prediv;

	if (clk_hw_get_flags(hw) & CLK_SET_RATE_PARENT) {
		unsigned long best_parent = rate;

		if (cg->common.features & CCU_FEATURE_ALL_PREDIV)
			best_parent *= div;
		*prate = clk_hw_round_rate(clk_hw_get_parent(hw), best_parent);
	}

	return *prate / div;
}

static int ccu_gate_set_rate(struct clk_hw *hw, unsigned long rate,
			     unsigned long parent_rate)
{
	/*
	 * We must report success but we can do so unconditionally because
	 * clk_factor_round_rate returns values that ensure this call is a
	 * nop.
	 */

	return 0;
}

const struct clk_ops ccu_gate_ops = {
	.disable	= ccu_gate_disable,
	.enable		= ccu_gate_enable,
	.is_enabled	= ccu_gate_is_enabled,
	.round_rate	= ccu_gate_round_rate,
	.set_rate	= ccu_gate_set_rate,
	.recalc_rate	= ccu_gate_recalc_rate,
	.init		= ccu_gate_init,
};
EXPORT_SYMBOL_GPL(ccu_gate_ops);
