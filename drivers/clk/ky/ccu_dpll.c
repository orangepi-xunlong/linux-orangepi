// SPDX-License-Identifier: GPL-2.0-only
/*
 * Ky clock type pll
 *
 * Copyright (c) 2023, ky Corporation.
 *
 */

#include <linux/io.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/delay.h>

#include "ccu_dpll.h"

#define DPLL_MIN_FREQ 1700000000
#define DPLL_MAX_FREQ 3400000000

#define pll_readl(reg)			readl(reg)
#define pll_readl_pll_swcr1(p)		pll_readl(p.base + p.reg_ctrl)
#define pll_readl_pll_swcr2(p)		pll_readl(p.base + p.reg_sel)

#define pll_writel(val, reg)		writel(val, reg)
#define pll_writel_pll_swcr1(val, p)	pll_writel(val, p.base + p.reg_ctrl)
#define pll_writel_pll_swcr2(val, p)	pll_writel(val, p.base + p.reg_sel)

/* unified dpllx_swcr1 for dpll1~2 */
union dpllx_swcr1 {
	struct {
		unsigned int reg0:8;
		unsigned int reg1:8;
		unsigned int reg2:8;
		unsigned int reg3:8;
	} b;
	unsigned int v;
};

/* unified dpllx_swcr2 for dpll1~2 */
union dpllx_swcr2 {
	struct {
		unsigned int reg4:8;
		unsigned int reg5:8;
		unsigned int reg6:8;
		unsigned int reg7:8;
	} b;
	unsigned int v;
};

/* frequency unit Mhz, return pll vco freq */
static unsigned long __get_vco_freq(struct clk_hw *hw)
{
	unsigned int reg0, reg1, reg2, reg3, reg4, reg5, reg6, reg7, size, i;
	struct ccu_dpll_rate_tbl *freq_pll_regs_table;
	struct ccu_dpll *p = hw_to_ccu_dpll(hw);
	union dpllx_swcr1 swcr1;
	union dpllx_swcr2 swcr2;

	swcr1.v = pll_readl_pll_swcr1(p->common);
	swcr2.v = pll_readl_pll_swcr2(p->common);

	reg0 = swcr1.b.reg0;
	reg1 = swcr1.b.reg1;
	reg2 = swcr1.b.reg2;
	reg3 = swcr1.b.reg3;
	reg4 = swcr2.b.reg4;
	reg5 = swcr2.b.reg5;
	reg6 = swcr2.b.reg6;
	reg7 = swcr2.b.reg7;

	freq_pll_regs_table = p->dpll.rate_tbl;
	size = p->dpll.tbl_size;

	for (i = 0; i < size; i++) {
		if ((freq_pll_regs_table[i].reg0 == reg0)
			&& (freq_pll_regs_table[i].reg1 == reg1)
			&& (freq_pll_regs_table[i].reg2 == reg2)
			&& (freq_pll_regs_table[i].reg3 == reg3)
			&& (freq_pll_regs_table[i].reg4 == reg4)
			&& (freq_pll_regs_table[i].reg5 == reg5)
			&& (freq_pll_regs_table[i].reg6 == reg6)
			&& (freq_pll_regs_table[i].reg7 == reg7))
				return freq_pll_regs_table[i].rate;
	}
	pr_err("Unknown rate for clock %s\n", __clk_get_name(hw->clk));
	return 0;
}

static unsigned long ccu_dpll_recalc_rate(struct clk_hw *hw,
				unsigned long parent_rate)
{
	return __get_vco_freq(hw);
}

static long ccu_dpll_round_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long *prate)
{
	struct ccu_dpll *p = hw_to_ccu_dpll(hw);
	unsigned long max_rate = 0;
	unsigned int i;
	struct ccu_dpll_config *params = &p->dpll;

	if (rate > DPLL_MAX_FREQ || rate < DPLL_MIN_FREQ) {
		pr_err("%lu rate out of range!\n", rate);
		return -EINVAL;
	}

	if (params->rate_tbl) {
		for (i = 0; i < params->tbl_size; i++) {
			if (params->rate_tbl[i].rate <= rate) {
				if (max_rate < params->rate_tbl[i].rate)
					max_rate = params->rate_tbl[i].rate;
			}
		}
	} else {
		pr_err("don't find freq table for pll\n");
	}
	return max_rate;
}

const struct clk_ops ccu_dpll_ops = {
	.recalc_rate = ccu_dpll_recalc_rate,
	.round_rate = ccu_dpll_round_rate,
};

