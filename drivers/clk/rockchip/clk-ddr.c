// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2016 Rockchip Electronics Co. Ltd.
 * Author: Lin Huang <hl@rock-chips.com>
 */

#include <linux/arm-smccc.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <soc/rockchip/rockchip_sip.h>
#include "clk.h"

struct rockchip_ddrclk {
	struct clk_hw	hw;
	void __iomem	*reg_base;
	int		mux_offset;
	int		mux_shift;
	int		mux_width;
	int		div_shift;
	int		div_width;
	int		ddr_flag;
	spinlock_t	*lock;
};

#define to_rockchip_ddrclk_hw(hw) container_of(hw, struct rockchip_ddrclk, hw)

static int rockchip_ddrclk_sip_set_rate(struct clk_hw *hw, unsigned long drate,
					unsigned long prate)
{
	struct rockchip_ddrclk *ddrclk = to_rockchip_ddrclk_hw(hw);
	unsigned long flags;
	struct arm_smccc_res res;

	spin_lock_irqsave(ddrclk->lock, flags);
	arm_smccc_smc(ROCKCHIP_SIP_DRAM_FREQ, drate, 0,
		      ROCKCHIP_SIP_CONFIG_DRAM_SET_RATE,
		      0, 0, 0, 0, &res);
	spin_unlock_irqrestore(ddrclk->lock, flags);

	return res.a0;
}

static unsigned long
rockchip_ddrclk_sip_recalc_rate(struct clk_hw *hw,
				unsigned long parent_rate)
{
	struct arm_smccc_res res;

	arm_smccc_smc(ROCKCHIP_SIP_DRAM_FREQ, 0, 0,
		      ROCKCHIP_SIP_CONFIG_DRAM_GET_RATE,
		      0, 0, 0, 0, &res);

	return res.a0;
}

static long rockchip_ddrclk_sip_round_rate(struct clk_hw *hw,
					   unsigned long rate,
					   unsigned long *prate)
{
	struct arm_smccc_res res;

	arm_smccc_smc(ROCKCHIP_SIP_DRAM_FREQ, rate, 0,
		      ROCKCHIP_SIP_CONFIG_DRAM_ROUND_RATE,
		      0, 0, 0, 0, &res);

	return res.a0;
}

static u8 rockchip_ddrclk_get_parent(struct clk_hw *hw)
{
	struct rockchip_ddrclk *ddrclk = to_rockchip_ddrclk_hw(hw);
	u32 val;

	val = readl(ddrclk->reg_base +
			ddrclk->mux_offset) >> ddrclk->mux_shift;
	val &= GENMASK(ddrclk->mux_width - 1, 0);

	return val;
}

static const struct clk_ops rockchip_ddrclk_sip_ops = {
	.recalc_rate = rockchip_ddrclk_sip_recalc_rate,
	.set_rate = rockchip_ddrclk_sip_set_rate,
	.round_rate = rockchip_ddrclk_sip_round_rate,
	.get_parent = rockchip_ddrclk_get_parent,
};

/* See v4.4/include/dt-bindings/display/rk_fb.h */
#define SCREEN_NULL			0
#define SCREEN_HDMI			6

static inline int rk_drm_get_lcdc_type(void)
{
	return SCREEN_NULL;
}

struct share_params {
	u32 hz;
	u32 lcdc_type;
	u32 vop;
	u32 vop_dclk_mode;
	u32 sr_idle_en;
	u32 addr_mcu_el3;
	/*
	 * 1: need to wait flag1
	 * 0: never wait flag1
	 */
	u32 wait_flag1;
	/*
	 * 1: need to wait flag1
	 * 0: never wait flag1
	 */
	u32 wait_flag0;
	u32 complt_hwirq;
	 /* if need, add parameter after */
};

struct rockchip_ddrclk_data {
	u32 inited_flag;
	void __iomem *share_memory;
};

static struct rockchip_ddrclk_data ddr_data;

static void rockchip_ddrclk_data_init(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(ROCKCHIP_SIP_SHARE_MEM,
		      1, SHARE_PAGE_TYPE_DDR, 0,
		      0, 0, 0, 0, &res);

	if (!res.a0) {
		ddr_data.share_memory = (void __iomem *)ioremap(res.a1, 1<<12);
		ddr_data.inited_flag = 1;
	}
}

static int rockchip_ddrclk_sip_set_rate_v2(struct clk_hw *hw,
					   unsigned long drate,
					   unsigned long prate)
{
	struct share_params *p;
	struct arm_smccc_res res;

	if (!ddr_data.inited_flag)
		rockchip_ddrclk_data_init();

	p = (struct share_params *)ddr_data.share_memory;

	p->hz = drate;
	p->lcdc_type = rk_drm_get_lcdc_type();
	p->wait_flag1 = 1;
	p->wait_flag0 = 1;

	arm_smccc_smc(ROCKCHIP_SIP_DRAM_FREQ,
		      SHARE_PAGE_TYPE_DDR, 0,
		      ROCKCHIP_SIP_CONFIG_DRAM_SET_RATE,
		      0, 0, 0, 0, &res);

	if ((int)res.a1 == -6) {
		pr_err("%s: timeout, drate = %lumhz\n", __func__, drate/1000000);
		/* TODO: rockchip_dmcfreq_wait_complete(); */
	}

	return res.a0;
}

static unsigned long rockchip_ddrclk_sip_recalc_rate_v2
			(struct clk_hw *hw, unsigned long parent_rate)
{
	struct arm_smccc_res res;

	arm_smccc_smc(ROCKCHIP_SIP_DRAM_FREQ,
		      SHARE_PAGE_TYPE_DDR, 0,
		      ROCKCHIP_SIP_CONFIG_DRAM_GET_RATE,
		      0, 0, 0, 0, &res);
	if (!res.a0)
		return res.a1;
	else
		return 0;
}

static long rockchip_ddrclk_sip_round_rate_v2(struct clk_hw *hw,
					      unsigned long rate,
					      unsigned long *prate)
{
	struct share_params *p;
	struct arm_smccc_res res;

	if (!ddr_data.inited_flag)
		rockchip_ddrclk_data_init();

	p = (struct share_params *)ddr_data.share_memory;

	p->hz = rate;

	arm_smccc_smc(ROCKCHIP_SIP_DRAM_FREQ,
		      SHARE_PAGE_TYPE_DDR, 0,
		      ROCKCHIP_SIP_CONFIG_DRAM_ROUND_RATE,
		      0, 0, 0, 0, &res);
	if (!res.a0)
		return res.a1;
	else
		return 0;
}

static const struct clk_ops rockchip_ddrclk_sip_ops_v2 = {
	.recalc_rate = rockchip_ddrclk_sip_recalc_rate_v2,
	.set_rate = rockchip_ddrclk_sip_set_rate_v2,
	.round_rate = rockchip_ddrclk_sip_round_rate_v2,
	.get_parent = rockchip_ddrclk_get_parent,
};

struct clk *rockchip_clk_register_ddrclk(const char *name, int flags,
					 const char *const *parent_names,
					 u8 num_parents, int mux_offset,
					 int mux_shift, int mux_width,
					 int div_shift, int div_width,
					 int ddr_flag, void __iomem *reg_base,
					 spinlock_t *lock)
{
	struct rockchip_ddrclk *ddrclk;
	struct clk_init_data init;
	struct clk *clk;

	ddrclk = kzalloc(sizeof(*ddrclk), GFP_KERNEL);
	if (!ddrclk)
		return ERR_PTR(-ENOMEM);

	init.name = name;
	init.parent_names = parent_names;
	init.num_parents = num_parents;

	init.flags = flags;
	init.flags |= CLK_SET_RATE_NO_REPARENT;

	switch (ddr_flag) {
	case ROCKCHIP_DDRCLK_SIP:
		init.ops = &rockchip_ddrclk_sip_ops;
		break;
	case ROCKCHIP_DDRCLK_SIP_V2:
		init.ops = &rockchip_ddrclk_sip_ops_v2;
		break;
	default:
		pr_err("%s: unsupported ddrclk type %d\n", __func__, ddr_flag);
		kfree(ddrclk);
		return ERR_PTR(-EINVAL);
	}

	ddrclk->reg_base = reg_base;
	ddrclk->lock = lock;
	ddrclk->hw.init = &init;
	ddrclk->mux_offset = mux_offset;
	ddrclk->mux_shift = mux_shift;
	ddrclk->mux_width = mux_width;
	ddrclk->div_shift = div_shift;
	ddrclk->div_width = div_width;
	ddrclk->ddr_flag = ddr_flag;

	clk = clk_register(NULL, &ddrclk->hw);
	if (IS_ERR(clk))
		kfree(ddrclk);

	return clk;
}
EXPORT_SYMBOL_GPL(rockchip_clk_register_ddrclk);
