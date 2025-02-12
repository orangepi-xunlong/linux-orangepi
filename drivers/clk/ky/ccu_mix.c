// SPDX-License-Identifier: GPL-2.0-only
/*
 * Ky clock type mix(div/mux/gate/factor)
 *
 * Copyright (c) 2023, ky Corporation.
 *
 */
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/clk.h>
#include "ccu_mix.h"

#define TIMEOUT_LIMIT (20000) /* max timeout 10000us */
static int twsi8_reg_val = 0x04;
const char * tswi8_clk_name = "twsi8_clk";
static void ccu_mix_disable(struct clk_hw *hw)
{
	struct ccu_mix *mix = hw_to_ccu_mix(hw);
	struct ccu_common * common = &mix->common;
	struct ccu_gate_config *gate = mix->gate;
	unsigned long flags = 0;
	unsigned long rate;
	u32 tmp;

	if (!gate)
		return;

	if (!strcmp(common->name, tswi8_clk_name)){
		twsi8_reg_val &= ~gate->gate_mask;;
		twsi8_reg_val |= gate->val_disable;
		tmp = twsi8_reg_val;
		if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
			|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
			writel(tmp, common->base + common->reg_sel);
		else
			writel(tmp, common->base + common->reg_ctrl);
		return;
	}

	if (common->lock)
		spin_lock_irqsave(common->lock, flags);

	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		tmp = readl(common->base + common->reg_sel);
	else
		tmp = readl(common->base + common->reg_ctrl);

	tmp &= ~gate->gate_mask;
	tmp |= gate->val_disable;

	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		writel(tmp, common->base + common->reg_sel);
	else
		writel(tmp, common->base + common->reg_ctrl);

	if (common->lock)
		spin_unlock_irqrestore(common->lock, flags);

	if (gate->flags & KY_CLK_GATE_NEED_DELAY) {
		rate = clk_hw_get_rate(&common->hw);

		if (rate == 0)
			pr_err("clock rate of %s is 0.\n", clk_hw_get_name(&common->hw));
		else
			/* Need delay 2M cycles. */
			udelay(DIV_ROUND_UP(2000000, rate));
	}

	return;
}

static int ccu_mix_enable(struct clk_hw *hw)
{
	struct ccu_mix *mix = hw_to_ccu_mix(hw);
	struct ccu_common * common = &mix->common;
	struct ccu_gate_config *gate = mix->gate;
	unsigned long flags = 0;
	unsigned long rate;
	u32 tmp;
	u32 val = 0;
	int timeout_power = 1;

    if (!gate)
		return 0;

	if (!strcmp(common->name, tswi8_clk_name)){
		twsi8_reg_val &= ~gate->gate_mask;;
		twsi8_reg_val |= gate->val_enable;
		tmp = twsi8_reg_val;
		if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
			|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
			writel(tmp, common->base + common->reg_sel);
		else
			writel(tmp, common->base + common->reg_ctrl);
		return 0;
	}

	if (common->lock)
		spin_lock_irqsave(common->lock, flags);

	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		tmp = readl(common->base + common->reg_sel);
	else
		tmp = readl(common->base + common->reg_ctrl);

	tmp &= ~gate->gate_mask;
	tmp |= gate->val_enable;

	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		writel(tmp, common->base + common->reg_sel);
	else
		writel(tmp, common->base + common->reg_ctrl);

	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		val = readl(common->base + common->reg_sel);
	else
		val = readl(common->base + common->reg_ctrl);

	if (common->lock)
		spin_unlock_irqrestore(common->lock, flags);

	while ((val & gate->gate_mask) != gate->val_enable && (timeout_power < TIMEOUT_LIMIT)) {
		udelay(timeout_power);
		if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
			val = readl(common->base + common->reg_sel);
		else
			val = readl(common->base + common->reg_ctrl);
		timeout_power *= 10;
	}

	if (timeout_power > 1) {
		if (val == tmp)
			pr_err("write clk_gate %s timeout occur, read pass after %d us delay\n",
			clk_hw_get_name(&common->hw), timeout_power);
		else
			pr_err("write clk_gate  %s timeout after %d us!\n", clk_hw_get_name(&common->hw), timeout_power);
	}

	if (gate->flags & KY_CLK_GATE_NEED_DELAY) {
		rate = clk_hw_get_rate(&common->hw);

		if (rate == 0)
			pr_err("clock rate of %s is 0.\n", clk_hw_get_name(&common->hw));
		else
			/* Need delay 2M cycles. */
			udelay(DIV_ROUND_UP(2000000, rate));
	}

	return 0;
}

static int ccu_mix_is_enabled(struct clk_hw *hw)
{
	struct ccu_mix *mix = hw_to_ccu_mix(hw);
	struct ccu_common * common = &mix->common;
	struct ccu_gate_config *gate = mix->gate;
	unsigned long flags = 0;
	u32 tmp;

	if (!gate)
		return 1;

	if (!strcmp(common->name, tswi8_clk_name)){
		return (twsi8_reg_val & gate->gate_mask) == gate->val_enable;
	}

	if (common->lock)
		spin_lock_irqsave(common->lock, flags);

	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		tmp = readl(common->base + common->reg_sel);
	else
		tmp = readl(common->base + common->reg_ctrl);

	if (common->lock)
		spin_unlock_irqrestore(common->lock, flags);

	return (tmp & gate->gate_mask) == gate->val_enable;
}

static unsigned long ccu_mix_recalc_rate(struct clk_hw *hw,
					unsigned long parent_rate)
{
	struct ccu_mix *mix = hw_to_ccu_mix(hw);
	struct ccu_common * common = &mix->common;
	struct ccu_div_config *div = mix->div;
	unsigned long val;
	u32 reg;

	if (!div){
		if (mix->factor)
			return parent_rate * mix->factor->mul / mix->factor->div;
		else
		    return parent_rate;
	}
    if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		reg = readl(common->base + common->reg_sel);
	else
		reg = readl(common->base + common->reg_ctrl);

	val = reg >> div->shift;
	val &= (1 << div->width) - 1;

	val = divider_recalc_rate(hw, parent_rate, val, div->table,
				  div->flags, div->width);

	return val;
}


static int ccu_mix_trigger_fc(struct clk_hw *hw)
{
	struct ccu_mix *mix = hw_to_ccu_mix(hw);
	struct ccu_common * common = &mix->common;
	unsigned long val = 0;

	int ret = 0, timeout = 50;

	if (common->reg_type == CLK_DIV_TYPE_1REG_FC_V2
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4
		|| common->reg_type == CLK_DIV_TYPE_1REG_FC_DIV_V5
		|| common->reg_type == CLK_DIV_TYPE_1REG_FC_MUX_V6) {

		timeout = 50;
		val = readl(common->base + common->reg_ctrl);
		val |= common->fc;
		writel(val, common->base + common->reg_ctrl);

		do {
			val = readl(common->base + common->reg_ctrl);
			timeout--;
			if (!(val & (common->fc)))
				break;
		} while (timeout);

		if (timeout == 0) {
			timeout = 5000;
			do {
				val = readl(common->base + common->reg_ctrl);
				timeout--;
				if (!(val & (common->fc)))
					break;
			} while (timeout);
			if (timeout != 0) {
				ret = 0;

			} else {
				ret = -1;
			}
		}
	}

	return ret;

}

static long ccu_mix_round_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long *prate)
{
	return rate;
}

unsigned long ccu_mix_calc_best_rate(struct clk_hw *hw, unsigned long rate, u32 *mux_val, u32 *div_val)
{
	struct ccu_mix *mix = hw_to_ccu_mix(hw);
	struct ccu_common * common = &mix->common;
	struct ccu_div_config *div = mix->div? mix->div: NULL;
	struct clk_hw *parent;
	unsigned long parent_rate = 0, best_rate = 0;
	u32 i, j, div_max;

	for (i = 0; i < common->num_parents; i++) {

		parent = clk_hw_get_parent_by_index(hw, i);
		if (!parent)
			continue;
		parent_rate = clk_get_rate(clk_hw_get_clk(parent, common->name));

		if(div)
			div_max = 1 << div->width;
		else
			div_max = 1;

		for(j = 1; j <= div_max; j++){
			if(abs(parent_rate/j - rate) < abs(best_rate - rate)){
				best_rate = DIV_ROUND_UP_ULL(parent_rate, j);
				*mux_val = i;
				*div_val = j - 1;
			}
		}
	}

	return best_rate;
}

static int ccu_mix_determine_rate(struct clk_hw *hw, struct clk_rate_request *req)
{
	return 0;
}

static int ccu_mix_set_rate(struct clk_hw *hw, unsigned long rate,
			   unsigned long parent_rate)
{
	struct ccu_mix *mix = hw_to_ccu_mix(hw);
	struct ccu_common * common = &mix->common;
	struct ccu_div_config *div = mix->div? mix->div: NULL;
	struct ccu_mux_config *mux = mix->mux? mix->mux: NULL;
	unsigned long best_rate = 0;
	unsigned long flags;
	u32 cur_mux, cur_div, mux_val = 0, div_val = 0;
	u32 reg = 0;
	int ret = 0;

	if(!div && !mux){
		return 0;
	}

	best_rate = ccu_mix_calc_best_rate(hw, rate, &mux_val, &div_val);
	if (!strcmp(common->name, tswi8_clk_name)){
		if(mux){
		cur_mux = twsi8_reg_val >> mux->shift;
		cur_mux &= (1 << mux->width) - 1;
		if(cur_mux != mux_val)
			clk_hw_set_parent(hw, clk_hw_get_parent_by_index(hw, mux_val));
		}
		return 0;
	}
	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		reg = readl(common->base + common->reg_sel);
	else
		reg = readl(common->base + common->reg_ctrl);

	if(mux){
		cur_mux = reg >> mux->shift;
		cur_mux &= (1 << mux->width) - 1;
		if(cur_mux != mux_val)
			clk_hw_set_parent(hw, clk_hw_get_parent_by_index(hw, mux_val));
	}
	if(div){
		cur_div = reg >> div->shift;
		cur_div &= (1 << div->width) - 1;
		if(cur_div == div_val)
			return 0;
	}else{
		return 0;
	}

	spin_lock_irqsave(common->lock, flags);
    if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		reg = readl(common->base + common->reg_sel);
	else
		reg = readl(common->base + common->reg_ctrl);

	reg &= ~GENMASK(div->width + div->shift - 1, div->shift);

    if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		writel(reg | (div_val << div->shift),
	       common->base + common->reg_sel);
	else
		writel(reg | (div_val << div->shift),
	       common->base + common->reg_ctrl);

	if (common->reg_type == CLK_DIV_TYPE_1REG_FC_V2
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4
		|| common->reg_type == CLK_DIV_TYPE_1REG_FC_DIV_V5) {

		ret = ccu_mix_trigger_fc(hw);
	}
	spin_unlock_irqrestore(common->lock, flags);

	if(ret)
		pr_err("%s of %s timeout\n", __func__, clk_hw_get_name(&common->hw));
	return 0;
}

static u8 ccu_mix_get_parent(struct clk_hw *hw)
{
	struct ccu_mix *mix = hw_to_ccu_mix(hw);
	struct ccu_common * common = &mix->common;
	struct ccu_mux_config *mux = mix->mux;
	u32 reg;
	u8 parent;

	if(!mux)
		return 0;

	if (!strcmp(common->name, tswi8_clk_name)){
		parent = twsi8_reg_val >> mux->shift;
		parent &= (1 << mux->width) - 1;
		return parent;
	}

	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		reg = readl(common->base + common->reg_sel);
	else
		reg = readl(common->base + common->reg_ctrl);

	parent = reg >> mux->shift;
	parent &= (1 << mux->width) - 1;

	if (mux->table) {
		int num_parents = clk_hw_get_num_parents(&common->hw);
		int i;

		for (i = 0; i < num_parents; i++)
			if (mux->table[i] == parent)
				return i;
	}
	return parent;
}

static int ccu_mix_set_parent(struct clk_hw *hw, u8 index)
{
	struct ccu_mix *mix = hw_to_ccu_mix(hw);
	struct ccu_common * common = &mix->common;
	struct ccu_mux_config *mux = mix->mux;
	unsigned long flags;
	u32 reg = 0;
	int ret = 0;

	if(!mux)
		return 0;

	if (mux->table)
		index = mux->table[index];

	if (!strcmp(common->name, tswi8_clk_name)){
		twsi8_reg_val &= ~GENMASK(mux->width + mux->shift - 1, mux->shift);
		twsi8_reg_val |= (index << mux->shift);
		reg = twsi8_reg_val;
		if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
			|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
			writel(reg, common->base + common->reg_sel);
		else
			writel(reg, common->base + common->reg_ctrl);
		return 0;
	}

	spin_lock_irqsave(common->lock, flags);

	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		reg = readl(common->base + common->reg_sel);
	else
		reg = readl(common->base + common->reg_ctrl);

	reg &= ~GENMASK(mux->width + mux->shift - 1, mux->shift);

	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		writel(reg | (index << mux->shift), common->base + common->reg_sel);
	else
		writel(reg | (index << mux->shift), common->base + common->reg_ctrl);

	if (common->reg_type == CLK_DIV_TYPE_1REG_FC_V2
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4
		|| common->reg_type == CLK_DIV_TYPE_1REG_FC_MUX_V6) {

		ret = ccu_mix_trigger_fc(hw);
	}
	spin_unlock_irqrestore(common->lock, flags);

	if(ret)
		pr_err("%s of %s timeout\n", __func__, clk_hw_get_name(&common->hw));

	return 0;
}

const struct clk_ops ccu_mix_ops = {
	.disable	 = ccu_mix_disable,
	.enable		 = ccu_mix_enable,
	.is_enabled	 = ccu_mix_is_enabled,
	.get_parent	 = ccu_mix_get_parent,
	.set_parent	 = ccu_mix_set_parent,
	.determine_rate = ccu_mix_determine_rate,
	.round_rate  = ccu_mix_round_rate,
	.recalc_rate = ccu_mix_recalc_rate,
	.set_rate	 = ccu_mix_set_rate,
};

