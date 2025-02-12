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

#include "ccu_pll.h"

#define PLL_MIN_FREQ 600000000
#define PLL_MAX_FREQ 3400000000
#define PLL_DELAYTIME 590 //(590*5)us

#define pll_readl(reg)		readl(reg)
#define pll_readl_pll_swcr1(p)	pll_readl(p.base + p.reg_ctrl)
#define pll_readl_pll_swcr2(p)	pll_readl(p.base + p.reg_sel)
#define pll_readl_pll_swcr3(p)	pll_readl(p.base + p.reg_xtc)

#define pll_writel(val, reg)		writel(val, reg)
#define pll_writel_pll_swcr1(val, p)	pll_writel(val, p.base + p.reg_ctrl)
#define pll_writel_pll_swcr2(val, p)	pll_writel(val, p.base + p.reg_sel)
#define pll_writel_pll_swcr3(val, p)	pll_writel(val, p.base + p.reg_xtc)

/* unified pllx_swcr1 for pll1~3 */
union pllx_swcr1 {
	struct {
		unsigned int reg5:8;
		unsigned int reg6:8;
		unsigned int reg7:8;
		unsigned int reg8:8;
	} b;
	unsigned int v;
};

/* unified pllx_swcr2 for pll1~3 */
union pllx_swcr2 {
	struct {
		unsigned int div1_en:1;
		unsigned int div2_en:1;
		unsigned int div3_en:1;
		unsigned int div4_en:1;
		unsigned int div5_en:1;
		unsigned int div6_en:1;
		unsigned int div7_en:1;
		unsigned int div8_en:1;
		unsigned int reserved1:4;
		unsigned int atest_en:1;
		unsigned int cktest_en:1;
		unsigned int dtest_en:1;
		unsigned int rdo:2;
		unsigned int mon_cfg:4;
		unsigned int reserved2:11;
	} b;
	unsigned int v;
};

/* unified pllx_swcr3 for pll1~3 */
union pllx_swcr3{
	struct {
		unsigned int div_frc:24;
		unsigned int div_int:7;
		unsigned int pll_en:1;
	} b;

	unsigned int v;
};

static int ccu_pll_is_enabled(struct clk_hw *hw)
{
	struct ccu_pll *p = hw_to_ccu_pll(hw);
	union pllx_swcr3 swcr3;
	unsigned int enabled;

	swcr3.v = pll_readl_pll_swcr3(p->common);
	enabled = swcr3.b.pll_en;

	return enabled;
}

/* frequency unit Mhz, return pll vco freq */
static unsigned long __get_vco_freq(struct clk_hw *hw)
{
	unsigned int reg5, reg6, reg7, reg8, size, i;
	unsigned int div_int, div_frc;
	struct ccu_pll_rate_tbl *freq_pll_regs_table;
	struct ccu_pll *p = hw_to_ccu_pll(hw);
	union pllx_swcr1 swcr1;
	union pllx_swcr3 swcr3;

	swcr1.v = pll_readl_pll_swcr1(p->common);
	swcr3.v = pll_readl_pll_swcr3(p->common);

    reg5 = swcr1.b.reg5;
    reg6 = swcr1.b.reg6;
    reg7 = swcr1.b.reg7;
    reg8 = swcr1.b.reg8;

    div_int = swcr3.b.div_int;
    div_frc = swcr3.b.div_frc;

    freq_pll_regs_table = p->pll.rate_tbl;
    size = p->pll.tbl_size;

    for (i = 0; i < size; i++) {
              if ((freq_pll_regs_table[i].reg5 == reg5)
              && (freq_pll_regs_table[i].reg6 == reg6)
              && (freq_pll_regs_table[i].reg7 == reg7)
              && (freq_pll_regs_table[i].reg8 == reg8)
	  && (freq_pll_regs_table[i].div_int == div_int)
	  && (freq_pll_regs_table[i].div_frac == div_frc))
                    return freq_pll_regs_table[i].rate;

    }

    pr_err("Unknown rate for clock %s\n", __clk_get_name(hw->clk));

    return 0;
}

static int ccu_pll_enable(struct clk_hw *hw)
{
	unsigned int delaytime = PLL_DELAYTIME;
	unsigned long flags;
	struct ccu_pll *p = hw_to_ccu_pll(hw);
	union pllx_swcr3 swcr3;

	if (ccu_pll_is_enabled(hw))
		return 0;

	spin_lock_irqsave(p->common.lock, flags);
	swcr3.v = pll_readl_pll_swcr3(p->common);
	swcr3.b.pll_en = 1;
	pll_writel_pll_swcr3(swcr3.v, p->common);
	spin_unlock_irqrestore(p->common.lock, flags);

	/* check lock status */
	udelay(50);

	while ((!(readl(p->pll.lock_base + p->pll.reg_lock) & p->pll.lock_enable_bit))
	       && delaytime) {
		udelay(5);
		delaytime--;
	}
	if (unlikely(!delaytime)) {
		pr_err("%s enabling didn't get stable within 3000us!!!\n", __clk_get_name(hw->clk));
		//panic("pllx_r/w timeout!\n");
	}

	return 0;
}

static void ccu_pll_disable(struct clk_hw *hw)
{
	unsigned long flags;
	struct ccu_pll *p = hw_to_ccu_pll(hw);
	union pllx_swcr3 swcr3;

	spin_lock_irqsave(p->common.lock, flags);
	swcr3.v = pll_readl_pll_swcr3(p->common);
	swcr3.b.pll_en = 0;
	pll_writel_pll_swcr3(swcr3.v, p->common);
	spin_unlock_irqrestore(p->common.lock, flags);
}

/*
 * pll rate change requires sequence:
 * clock off -> change rate setting -> clock on
 * This function doesn't really change rate, but cache the config
 */
static int ccu_pll_set_rate(struct clk_hw *hw, unsigned long rate,
			       unsigned long parent_rate)
{
	unsigned int i, reg5 = 0, reg6 = 0, reg7 = 0, reg8 = 0;
	unsigned int div_int, div_frc;
	unsigned long flags;
	unsigned long new_rate = rate, old_rate;
	struct ccu_pll *p = hw_to_ccu_pll(hw);
	struct ccu_pll_config *params = &p->pll;
	union pllx_swcr1 swcr1;
	union pllx_swcr3 swcr3;
	bool found = false;
	bool pll_enabled = false;

	if (ccu_pll_is_enabled(hw)) {
		pll_enabled = true;
		ccu_pll_disable(hw);
	}

	old_rate = __get_vco_freq(hw);
	/* setp 1: calculate fbd frcd kvco and band */
	if (params->rate_tbl) {
		for (i = 0; i < params->tbl_size; i++) {
			if (rate == params->rate_tbl[i].rate) {
				found = true;

				reg5 = params->rate_tbl[i].reg5;
				reg6 = params->rate_tbl[i].reg6;
				reg7 = params->rate_tbl[i].reg7;
				reg8 = params->rate_tbl[i].reg8;
				div_int = params->rate_tbl[i].div_int;
				div_frc = params->rate_tbl[i].div_frac;
				break;
			}
		}

		BUG_ON(!found);
	} else {
		pr_err("don't find freq table for pll\n");
		if (pll_enabled)
			ccu_pll_enable(hw);
		return -EINVAL;
	}

	spin_lock_irqsave(p->common.lock, flags);
	/* setp 2: set pll kvco/band and fbd/frcd setting */
	swcr1.v = pll_readl_pll_swcr1(p->common);
	swcr1.b.reg5 = reg5;
	swcr1.b.reg6 = reg6;
	swcr1.b.reg7 = reg7;
	swcr1.b.reg8 = reg8;
	pll_writel_pll_swcr1(swcr1.v, p->common);

	swcr3.v = pll_readl_pll_swcr3(p->common);
	swcr3.b.div_int = div_int;
	swcr3.b.div_frc = div_frc;
	pll_writel_pll_swcr3(swcr3.v, p->common);

	spin_unlock_irqrestore(p->common.lock, flags);

	if (pll_enabled)
		ccu_pll_enable(hw);

	pr_debug("%s %s rate %lu->%lu!\n", __func__,
		 __clk_get_name(hw->clk), old_rate, new_rate);
	return 0;
}

static unsigned long ccu_pll_recalc_rate(struct clk_hw *hw,
					 unsigned long parent_rate)
{
	return __get_vco_freq(hw);
}

static long ccu_pll_round_rate(struct clk_hw *hw, unsigned long rate,
			       unsigned long *prate)
{
	struct ccu_pll *p = hw_to_ccu_pll(hw);
	unsigned long max_rate = 0;
	unsigned int i;
	struct ccu_pll_config *params = &p->pll;

	if (rate > PLL_MAX_FREQ || rate < PLL_MIN_FREQ) {
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

const struct clk_ops ccu_pll_ops = {
	.enable = ccu_pll_enable,
	.disable = ccu_pll_disable,
	.set_rate = ccu_pll_set_rate,
	.recalc_rate = ccu_pll_recalc_rate,
	.round_rate = ccu_pll_round_rate,
	.is_enabled = ccu_pll_is_enabled,
};

