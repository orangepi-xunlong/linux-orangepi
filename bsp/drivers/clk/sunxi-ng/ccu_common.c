// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
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
#include <linux/clkdev.h>
#include <linux/delay.h>
#include <sunxi-autogen.h>
#include "ccu_common.h"
#include "ccu_gate.h"
#include "ccu_reset.h"
#include "ccu_nm.h"

#define SUNXI_CCU_COMMON_VERSION	"1.2.4"

static DEFINE_SPINLOCK(ccu_lock);

#if (defined CONFIG_AW_FPGA_BOARD)
void ccu_helper_wait_for_lock(struct ccu_common *common, u32 lock)
{
}
#else
void ccu_helper_wait_for_lock(struct ccu_common *common, u32 lock)
{
	void __iomem *addr;
	__maybe_unused u32 reg;
	__maybe_unused int read_times = 0;
	__maybe_unused int lock_times = 0;

	if (!lock)
		return;

	if (common->features & CCU_FEATURE_LOCK_REG)
		addr = common->base + common->lock_reg;
	else
		addr = common->base + common->reg;

#if IS_ENABLED(CONFIG_AW_STANDARD_CCU)
	/*
	 * Judge the lock status every 3us, if lock, lock_times +1, when lock_times is 3,
	 * indicates that the current pll is basically stable. if not lock, print err msg.
	*/
	while (read_times != 100 && lock_times != 3) {
		if (readl(addr) & lock)
			lock_times += 1;
		else
			lock_times = 0;

		udelay(3);

		read_times += 1;
	}

	if (lock_times == 3)
		udelay(20);
	else
		WARN(1, " %s fail to wait lock, please check if pll rate in vco range!\n", clk_hw_get_name(&common->hw));
#else
	WARN_ON(readl_relaxed_poll_timeout(addr, reg, reg & lock, 100, 70000));
#endif
}
#endif

#if (defined CONFIG_AW_FPGA_BOARD)
void ccu_helper_wait_for_clear(struct ccu_common *common, u32 clear)
{
}
#else
void ccu_helper_wait_for_clear(struct ccu_common *common, u32 clear)
{
	void __iomem *addr;
	u32 reg;

	if (!clear)
		return;

	addr = common->base + common->reg;
	reg = readl(addr);
	writel(reg | clear, addr);

	WARN_ON(readl_relaxed_poll_timeout_atomic(addr, reg, !(reg & clear), 100, 10000));
}
#endif
EXPORT_SYMBOL_GPL(ccu_helper_wait_for_clear);

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
	if (IS_ERR_OR_NULL(pll_nb->clk_nb.notifier_call))
		pll_nb->clk_nb.notifier_call = ccu_pll_notifier_cb;

	return clk_notifier_register(pll_nb->common->hw.clk,
				     &pll_nb->clk_nb);
}
EXPORT_SYMBOL_GPL(ccu_pll_notifier_register);

#ifdef CONFIG_PM_SLEEP

static LIST_HEAD(ccu_reg_cache_list);
static DEFINE_MUTEX(ccu_reg_cache_lock);

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

static void ccu_restore_with_value(void __iomem *base,
			const struct ccu_reg_dump *rd,
			unsigned int num_regs)
{
	unsigned int reg, restore_reg;

	for (; num_regs > 0; --num_regs, ++rd) {
		reg = readl(base + rd->offset);
		restore_reg = rd->value | reg;
		writel(restore_reg, base + rd->offset);
	}
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

	list_for_each_entry(reg_cache, &ccu_reg_cache_list, node)
		ccu_save(reg_cache->reg_base, reg_cache->rdump,
			 reg_cache->rd_num);
	return 0;
}

static void ccu_resume(void)
{
	struct sunxi_clock_reg_cache *reg_cache;

	list_for_each_entry(reg_cache, &ccu_reg_cache_list, node) {
		ccu_restore(reg_cache->reg_base, reg_cache->rdump,
				reg_cache->rd_num);
		ccu_restore_with_value(reg_cache->reg_base, reg_cache->rsuspend,
			    reg_cache->rsuspend_num);
	}
}

static struct syscore_ops sunxi_clk_syscore_ops = {
	.suspend = ccu_suspend,
	.resume = ccu_resume,
};
#endif

void sunxi_ccu_sleep_init(void __iomem *reg_base,
			  struct ccu_common **rdump,
			  unsigned long nr_rdump,
			  const struct ccu_reg_dump *rsuspend,
			  unsigned long nr_rsuspend)
{
#ifdef CONFIG_PM_SLEEP
	struct sunxi_clock_reg_cache *reg_cache;

	reg_cache = kzalloc(sizeof(*reg_cache),
			GFP_KERNEL);
	if (!reg_cache)
		panic("could not allocate register reg_cache.\n");
	reg_cache->rdump = ccu_alloc_reg_dump(rdump, nr_rdump);

	if (!reg_cache->rdump)
		panic("could not allocate register dump storage.\n");

	mutex_lock(&ccu_reg_cache_lock);
	if (list_empty(&ccu_reg_cache_list))
		register_syscore_ops(&sunxi_clk_syscore_ops);

	reg_cache->reg_base = reg_base;
	reg_cache->rd_num = nr_rdump;
	reg_cache->rsuspend = rsuspend;
	reg_cache->rsuspend_num = nr_rsuspend;
	list_add_tail(&reg_cache->node, &ccu_reg_cache_list);
	mutex_unlock(&ccu_reg_cache_lock);
#endif
}
EXPORT_SYMBOL_GPL(sunxi_ccu_sleep_init);

int sunxi_ccu_probe(struct device_node *node, void __iomem *reg,
		    const struct sunxi_ccu_desc *desc)
{
	struct ccu_reset *reset;
	int i, ret;
	u32 flag = 0;
	static bool probed;

	/*
	* AW_BSP_VERSION is defined in sunxi-autogen.h, it saves the latest submission point,
	* please refer to ${sdk}/build/mkkernel.sh.
	* sunxi_ccu_probe will be called multiple times, only printing on the first call.
	*/
	if (!probed) {
		/* Do not use pr_*() or dev_*() here -- We want a clear message */
		printk(KERN_EMERG "AW BSP version: %s\n", AW_BSP_VERSION);
		probed = true;
	}

	if (!of_property_read_u32(node, "clk-init-gate", &flag))
		sunxi_info(NULL, "%s will stop clk in init process\n", node->name);

	for (i = 0; i < desc->num_ccu_clks; i++) {
		struct ccu_common *cclk = desc->ccu_clks[i];

		if (!cclk)
			continue;

		if (cclk->hw.init)
			cclk->sdm_info = sunxi_clk_get_sdm_info(cclk->hw.init->name);

		cclk->base = reg;
		cclk->lock = &ccu_lock;

		if (cclk->features & CCU_FEATURE_CLAC_CACHED)
			INIT_LIST_HEAD(&cclk->calc_cache);

		if (flag)
			cclk->features |= CCU_FEATURE_INIT_GATE;
	}

	for (i = 0; i < desc->hw_clks->num ; i++) {
		struct clk_hw *hw = desc->hw_clks->hws[i];
		const char *name;

		if (!hw)
			continue;

		name = hw->init->name;

		sunxi_debug(NULL, "%s: sunxi ccu register. \n", name);

		ret = of_clk_hw_register(node, hw);

/* add this CONFIG for clk SATA */
#if IS_ENABLED(CONFIG_AW_CCU_DEBUG)
		clk_hw_register_clkdev(hw, name, NULL);
#endif

		if (ret) {
			sunxi_err(NULL, "Couldn't register clock %d - %s\n", i, name);
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

	sunxi_info(NULL, "%s: sunxi ccu init OK\n", node->name);

	sunxi_info_once(NULL, "sunxi ccu common driver version: %s\n", SUNXI_CCU_COMMON_VERSION);
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
EXPORT_SYMBOL_GPL(sunxi_ccu_probe);

void set_reg(char __iomem *addr, u32 val, u8 bw, u8 bs)
{
	u32 mask = (1UL << bw) - 1UL;
	u32 tmp = 0;

	tmp = readl(addr);
	tmp &= ~(mask << bs);

	writel(tmp | ((val & mask) << bs), addr);
}
EXPORT_SYMBOL_GPL(set_reg);

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
EXPORT_SYMBOL_GPL(set_reg_key);

int ccu_is_sdm_enabled(struct clk *clk)
{
	struct clk_hw *cur_hw;
	struct clk_hw *parent_hw;
	cur_hw = __clk_get_hw(clk);

	while (cur_hw && strcmp((clk_hw_get_name(cur_hw)), "dcxo24M")) {
		parent_hw = cur_hw;
		cur_hw = clk_hw_get_parent(cur_hw);
	}

	if (!parent_hw) {
		sunxi_err(NULL, "Do not pass the argument 'dcxo' to the function\n");
		return -1;
	}
	sunxi_err(NULL, "please make sure the top clk %s is nm type! %s:%d\n", clk_hw_get_name(parent_hw), __func__, __LINE__);

	return ccu_nm_is_sdm_enabled(parent_hw);
}
EXPORT_SYMBOL_GPL(ccu_is_sdm_enabled);

unsigned int ccu_get_table_div(const struct clk_div_table *table,
							unsigned int val)
{
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->val == val)
			return clkt->div;
	return 0;
}
EXPORT_SYMBOL_GPL(ccu_get_table_div);

unsigned int ccu_get_table_val(const struct clk_div_table *table,
							unsigned int div)
{
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->div == div)
			return clkt->val;
	return 0;
}
EXPORT_SYMBOL_GPL(ccu_get_table_val);

MODULE_LICENSE("GPL v2");
MODULE_VERSION(SUNXI_CCU_COMMON_VERSION);
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
