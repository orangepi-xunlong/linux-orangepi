// SPDX-License-Identifier: GPL-2.0-only
/*
 * Ky clock type ddr
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
#include "ccu_ddr.h"

#define PMU_AP_IMR			(0x098)
#define AP_DCLK_FC_DONE_INT_MSK		BIT(15)
#define DCLK_FC_DONE_INT_MSK		BIT(4)

#define PMU_AP_ISR			(0x0a0)
#define AP_DCLK_FC_DONE_INT_STS		BIT(15)
#define DCLK_FC_DONE_INT_STS		BIT(4)
#define AP_FC_STS			BIT(1)

#define DFC_AP				(0x180)
#define DFC_FREQ_LV			0x1
#define DFC_REQ				BIT(0)

#define DFC_STATUS			(0x188)
#define DFC_CAUSE_SHIFT			0x7
#define DFC_STS				BIT(0)

/* enable/disable ddr frequency change done interrupt */
static void ccu_ddr_enable_dfc_int(struct ccu_common * common, bool enable)
{
	u32 val;
	unsigned long flags;

	spin_lock_irqsave(common->lock, flags);
	val = readl(common->base + PMU_AP_IMR);
	if (enable) {
		val |= AP_DCLK_FC_DONE_INT_MSK;
	} else {
		val &= ~AP_DCLK_FC_DONE_INT_MSK;
	}
	writel(val, common->base + PMU_AP_IMR);
	spin_unlock_irqrestore(common->lock, flags);
}

/* clear ddr frequency change done interrupt status*/
static void ccu_ddr_clear_dfc_int_status(struct ccu_common * common)
{
	u32 val;
	unsigned long flags;

	spin_lock_irqsave(common->lock, flags);
	val = readl(common->base + PMU_AP_ISR);
	val &= ~(AP_DCLK_FC_DONE_INT_STS | AP_FC_STS);
	writel(val, common->base + PMU_AP_ISR);
	spin_unlock_irqrestore(common->lock, flags);
}

static int ccu_ddr_wait_freq_change_done(struct ccu_common * common)
{
	int timeout = 100;
	u32 val;

	while (--timeout) {
		udelay(10);
		val = readl(common->base + PMU_AP_ISR);
		if (val & AP_DCLK_FC_DONE_INT_STS)
			break;
	}
	if (!timeout) {
		pr_err("%s: timeout! can not wait dfc done interrupt\n", __func__);
		return -EBUSY;
	}
	return 0;
}

static int ccu_ddr_freq_chg(struct ccu_common * common, struct ccu_mux_config *mux, u8 level)
{
	u32 reg;
	u32 timeout;
	unsigned long flags;

	if (level > MAX_FREQ_LV) {
		pr_err("%s: invalid %d freq level\n", __func__, level);
		return -EINVAL;
	}

	/* check if dfc in progress */
	timeout = 1000;
	while (--timeout) {
		if (!(readl(common->base + DFC_STATUS) & DFC_STS))
				break;
		udelay(10);
	}

	if (!timeout) {
		pr_err("%s: another dfc is in pregress. status:0x%x\n", __func__, readl(common->base + DFC_STATUS));
		return -EBUSY;
	}

	spin_lock_irqsave(common->lock, flags);
	reg = readl(common->base + common->reg_sel);
	reg &= ~GENMASK(mux->width + mux->shift - 1, mux->shift);
	writel(reg | (level << mux->shift) | common->fc, common->base + common->reg_sel);
	spin_unlock_irqrestore(common->lock, flags);

	timeout = 1000;
	while (--timeout) {
		udelay(10);
		if (!(readl(common->base + DFC_STATUS) & DFC_STS))
			break;
	}

	if (!timeout) {
		pr_err("dfc error! status:0x%x\n", readl(common->base + DFC_STATUS));
		return -EBUSY;
	}

	return 0;
}

static unsigned long ccu_ddr_recalc_rate(struct clk_hw *hw,
				unsigned long parent_rate)
{
	return parent_rate;
}

static long ccu_ddr_round_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long *prate)
{
	return rate;
}

unsigned long ccu_ddr_calc_best_rate(struct clk_hw *hw, unsigned long rate,
				u32 *mux_val)
{
	struct ccu_ddr *ddr = hw_to_ccu_ddr(hw);
	struct ccu_common * common = &ddr->common;
	struct clk_hw *parent;
	unsigned long parent_rate = 0, best_rate = 0;
	u32 i;

	for (i = 0; i < common->num_parents; i++) {
		parent = clk_hw_get_parent_by_index(hw, i);
		if (!parent)
			continue;
		parent_rate = clk_get_rate(clk_hw_get_clk(parent, common->name));
		if (abs(parent_rate - rate) < abs(best_rate - rate)) {
			best_rate = parent_rate;
			*mux_val = i;
		}
	}
	return best_rate;
}

static int ccu_ddr_set_rate(struct clk_hw *hw, unsigned long rate,
			   unsigned long parent_rate)
{
	struct ccu_ddr *ddr = hw_to_ccu_ddr(hw);
	struct ccu_common * common = &ddr->common;
	struct ccu_mux_config *mux = ddr->mux? ddr->mux: NULL;
	unsigned long best_rate = 0;
	u32 cur_mux, mux_val = 0;
	u32 reg = 0;

	if (!mux) {
		return 0;
	}

	best_rate = ccu_ddr_calc_best_rate(hw, rate, &mux_val);

	reg = readl(common->base + common->reg_sel);
	if (mux) {
		cur_mux = reg >> mux->shift;
		cur_mux &= (1 << mux->width) - 1;
		if (cur_mux != mux_val)
			clk_hw_set_parent(hw, clk_hw_get_parent_by_index(hw, mux_val));
	}
	return 0;
}

static u8 ccu_ddr_get_parent(struct clk_hw *hw)
{
	struct ccu_ddr *ddr = hw_to_ccu_ddr(hw);
	struct ccu_common *common = &ddr->common;
	struct ccu_mux_config *mux = ddr->mux;
	u32 reg;
	u8 parent;

	if (!mux)
		return 0;

	reg = readl(common->base + common->reg_sel);

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

static int ccu_ddr_set_parent(struct clk_hw *hw, u8 index)
{
	struct ccu_ddr *ddr = hw_to_ccu_ddr(hw);
	struct ccu_common * common = &ddr->common;
	struct ccu_mux_config *mux = ddr->mux;
	int ret = 0;

	if (!mux)
		return 0;

	if (mux->table)
		index = mux->table[index];

	/* request change begin */
	ccu_ddr_enable_dfc_int(common, true);

	/* change parent*/
	ret = ccu_ddr_freq_chg(common, mux, index);
	if (ret < 0) {
		pr_err("%s: ddr_freq_chg fail. ret = %d\n", __func__, ret);
		return ret;
	}

	/* wait for frequency change done */
	ret = ccu_ddr_wait_freq_change_done(common);
	if (ret < 0) {
		pr_err("%s: wait_freq_change_done timeout. ret = %d\n", __func__, ret);
		return ret;
	}
	ccu_ddr_clear_dfc_int_status(common);
	ccu_ddr_enable_dfc_int(common, false);

	return 0;
}

static int ccu_ddr_determine_rate(struct clk_hw *hw, struct clk_rate_request *req)
{
	unsigned long best_rate = req->rate;
	u32 mux_val = 0;

	best_rate = ccu_ddr_calc_best_rate(hw, req->rate, &mux_val);
	req->rate = best_rate;
	return 0;
}

const struct clk_ops ccu_ddr_ops = {
	.get_parent	= ccu_ddr_get_parent,
	.set_parent	= ccu_ddr_set_parent,
	.determine_rate	= ccu_ddr_determine_rate,
	.round_rate	= ccu_ddr_round_rate,
	.recalc_rate	= ccu_ddr_recalc_rate,
	.set_rate	= ccu_ddr_set_rate,
};

