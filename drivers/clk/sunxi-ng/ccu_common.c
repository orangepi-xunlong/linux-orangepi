// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2016 Maxime Ripard
 *
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/iopoll.h>
#include <linux/slab.h>
#include <linux/syscore_ops.h>
#include <linux/module.h>

#include "ccu_common.h"
#include "ccu_gate.h"
#include "ccu_reset.h"

static DEFINE_SPINLOCK(ccu_lock);

void ccu_helper_wait_for_lock(struct ccu_common *common, u32 lock)
{
	void __iomem *addr;
	u32 reg;

	if (!lock)
		return;

	if (common->features & CCU_FEATURE_LOCK_REG)
		addr = common->base + common->lock_reg;
	else
		addr = common->base + common->reg;

	WARN_ON(readl_relaxed_poll_timeout(addr, reg, reg & lock, 100, 70000));
}

/*
 * This clock notifier is called when the frequency of a PLL clock is
 * changed. In common PLL designs, changes to the dividers take effect
 * almost immediately, while changes to the multipliers (implemented
 * as dividers in the feedback loop) take a few cycles to work into
 * the feedback loop for the PLL to stablize.
 *
 * Sometimes when the PLL clock rate is changed, the decrease in the
 * divider is too much for the decrease in the multiplier to catch up.
 * The PLL clock rate will spike, and in some cases, might lock up
 * completely.
 *
 * This notifier callback will gate and then ungate the clock,
 * effectively resetting it, so it proceeds to work. Care must be
 * taken to reparent consumers to other temporary clocks during the
 * rate change, and that this notifier callback must be the first
 * to be registered.
 */
static int ccu_pll_notifier_cb(struct notifier_block *nb,
			       unsigned long event, void *data)
{
	struct ccu_pll_nb *pll = to_ccu_pll_nb(nb);
	int ret = 0;

	if (event != POST_RATE_CHANGE)
		goto out;

	ccu_gate_helper_disable(pll->common, pll->enable);

	ret = ccu_gate_helper_enable(pll->common, pll->enable);
	if (ret)
		goto out;

	ccu_helper_wait_for_lock(pll->common, pll->lock);

out:
	return notifier_from_errno(ret);
}

int ccu_pll_notifier_register(struct ccu_pll_nb *pll_nb)
{
	pll_nb->clk_nb.notifier_call = ccu_pll_notifier_cb;

	return clk_notifier_register(pll_nb->common->hw.clk,
				     &pll_nb->clk_nb);
}
EXPORT_SYMBOL_GPL(ccu_pll_notifier_register);

#ifdef CONFIG_PM_SLEEP

static LIST_HEAD(ccu_reg_cache_list);

struct sunxi_clock_reg_cache {
	struct list_head node;
	void __iomem *reg_base;
	struct ccu_reg_dump *rdump;
	unsigned int rd_num;
	const struct ccu_reg_dump *rsuspend;
	unsigned int rsuspend_num;
};

static void ccu_save(void __iomem *base, struct ccu_reg_dump *rd,
		    unsigned int num_regs)
{
	for (; num_regs > 0; --num_regs, ++rd)
		rd->value = readl(base + rd->offset);
}

static void ccu_restore(void __iomem *base,
			const struct ccu_reg_dump *rd,
			unsigned int num_regs)
{
	for (; num_regs > 0; --num_regs, ++rd)
		writel(rd->value, base + rd->offset);
}

static struct ccu_reg_dump *ccu_alloc_reg_dump(struct ccu_common **rdump,
					       unsigned long nr_rdump)
{
	struct ccu_reg_dump *rd;
	unsigned int i;

	rd = kcalloc(nr_rdump, sizeof(*rd), GFP_KERNEL);
	if (!rd)
		return NULL;

	for (i = 0; i < nr_rdump; ++i) {
		struct ccu_common *ccu_clks = rdump[i];

		rd[i].offset = ccu_clks->reg;
	}

	return rd;
}

static int ccu_suspend(void)
{
	struct sunxi_clock_reg_cache *reg_cache;

	list_for_each_entry(reg_cache, &ccu_reg_cache_list, node) {
		ccu_save(reg_cache->reg_base, reg_cache->rdump,
			 reg_cache->rd_num);
		ccu_restore(reg_cache->reg_base, reg_cache->rsuspend,
			    reg_cache->rsuspend_num);
	}
	return 0;
}

static void ccu_resume(void)
{
	struct sunxi_clock_reg_cache *reg_cache;

	list_for_each_entry(reg_cache, &ccu_reg_cache_list, node)
		ccu_restore(reg_cache->reg_base, reg_cache->rdump,
				reg_cache->rd_num);
}

static struct syscore_ops sunxi_clk_syscore_ops = {
	.suspend = ccu_suspend,
	.resume = ccu_resume,
};

void sunxi_ccu_sleep_init(void __iomem *reg_base,
			  struct ccu_common **rdump,
			  unsigned long nr_rdump,
			  const struct ccu_reg_dump *rsuspend,
			  unsigned long nr_rsuspend)
{
	struct sunxi_clock_reg_cache *reg_cache;

	reg_cache = kzalloc(sizeof(struct sunxi_clock_reg_cache),
			GFP_KERNEL);
	if (!reg_cache)
		panic("could not allocate register reg_cache.\n");
	reg_cache->rdump = ccu_alloc_reg_dump(rdump, nr_rdump);

	if (!reg_cache->rdump)
		panic("could not allocate register dump storage.\n");

	if (list_empty(&ccu_reg_cache_list))
		register_syscore_ops(&sunxi_clk_syscore_ops);

	reg_cache->reg_base = reg_base;
	reg_cache->rd_num = nr_rdump;
	reg_cache->rsuspend = rsuspend;
	reg_cache->rsuspend_num = nr_rsuspend;
	list_add_tail(&reg_cache->node, &ccu_reg_cache_list);
}
#else
void sunxi_ccu_sleep_init(void __iomem *reg_base,
			  struct ccu_common **rdump,
			  unsigned long nr_rdump,
			  const struct ccu_reg_dump *rsuspend,
			  unsigned long nr_rsuspend)
{ }
#endif
EXPORT_SYMBOL_GPL(sunxi_ccu_sleep_init);

int sunxi_ccu_probe(struct device_node *node, void __iomem *reg,
		    const struct sunxi_ccu_desc *desc)
{
	struct ccu_reset *reset;
	int i, ret;

	for (i = 0; i < desc->num_ccu_clks; i++) {
		struct ccu_common *cclk = desc->ccu_clks[i];

		if (!cclk)
			continue;

		cclk->base = reg;
		cclk->lock = &ccu_lock;
	}

	for (i = 0; i < desc->hw_clks->num ; i++) {
		struct clk_hw *hw = desc->hw_clks->hws[i];
		const char *name;

		if (!hw)
			continue;

		name = hw->init->name;
		ret = of_clk_hw_register(node, hw);
		if (ret) {
			pr_err("Couldn't register clock %d - %s\n", i, name);
			goto err_clk_unreg;
		}
	}

	ret = of_clk_add_hw_provider(node, of_clk_hw_onecell_get,
				     desc->hw_clks);
	if (ret)
		goto err_clk_unreg;

	reset = kzalloc(sizeof(*reset), GFP_KERNEL);
	if (!reset) {
		ret = -ENOMEM;
		goto err_alloc_reset;
	}

	reset->rcdev.of_node = node;
	reset->rcdev.ops = &ccu_reset_ops;
	reset->rcdev.owner = THIS_MODULE;
	reset->rcdev.nr_resets = desc->num_resets;
	reset->base = reg;
	reset->lock = &ccu_lock;
	reset->reset_map = desc->resets;

	ret = reset_controller_register(&reset->rcdev);
	if (ret)
		goto err_of_clk_unreg;

	pr_info("%s: sunxi ccu init OK\n", node->name);

	return 0;

err_of_clk_unreg:
	kfree(reset);
err_alloc_reset:
	of_clk_del_provider(node);
err_clk_unreg:
	while (--i >= 0) {
		struct clk_hw *hw = desc->hw_clks->hws[i];

		if (!hw)
			continue;
		clk_hw_unregister(hw);
	}
	return ret;
}

void set_reg(char __iomem *addr, u32 val, u8 bw, u8 bs)
{
	u32 mask = (1UL << bw) - 1UL;
	u32 tmp = 0;

	tmp = readl(addr);
	tmp &= ~(mask << bs);

	writel(tmp | ((val & mask) << bs), addr);
}

void set_reg_key(char __iomem *addr,
		 u32 key, u8 kbw, u8 kbs,
		 u32 val, u8 bw, u8 bs)
{
	u32 mask = (1UL << bw) - 1UL;
	u32 kmask = (1UL << kbw) - 1UL;
	u32 tmp = 0;

	tmp = readl(addr);
	tmp &= ~(mask << bs);

	writel(tmp | ((val & mask) << bs) | ((key & kmask) << kbs), addr);
}
EXPORT_SYMBOL_GPL(sunxi_ccu_probe);

MODULE_LICENSE("GPL v2");
