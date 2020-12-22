/*
 * Allwinner A1X SoCs timer handling.
 *
 * Copyright (C) 2012 Maxime Ripard
 *
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * Based on code from
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Benn Huang <benn@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/clk.h>
#include <linux/clockchips.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqreturn.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sunxi_timer.h>
#include <linux/clk/sunxi.h>
#include <linux/delay.h>
#include <mach/platform.h>

#define TIMER_CTL_REG		0x00
#define TIMER_CTL_ENABLE		(1 << 0)
#define TIMER_IRQ_ST_REG	0x04
#define TIMER0_CTL_REG		0x10
#define TIMER0_CTL_ENABLE		(1 << 0)
#define TIMER0_CTL_AUTORELOAD		(1 << 1)
#define TIMER0_CTL_ONESHOT		(1 << 7)
#define TIMER0_INTVAL_REG	0x14
#define TIMER0_CNTVAL_REG	0x18

#ifdef CONFIG_EVB_PLATFORM
#define TIMER_SCAL		16
#else /* it is proved by test that clk-src=32000, prescale=1 on fpga */
#define TIMER_SCAL		1
#endif

static void __iomem *timer_base;
static spinlock_t timer0_spin_lock;

#ifdef CONFIG_GENERIC_CLOCKEVENTS_BROADCAST
extern void smp_timer_broadcast(const struct cpumask *mask);
#else
#define smp_timer_broadcast	NULL
#endif

static void sunxi_clkevt_mode(enum clock_event_mode mode,
			      struct clock_event_device *clk)
{
	u32 u = readl(timer_base + TIMER0_CTL_REG);

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		u &= ~(TIMER0_CTL_ONESHOT);
		writel(u | TIMER0_CTL_ENABLE, timer_base + TIMER0_CTL_REG);
		break;

	case CLOCK_EVT_MODE_ONESHOT:
		writel(u | TIMER0_CTL_ONESHOT, timer_base + TIMER0_CTL_REG);
		break;
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	default:
		writel(u & ~(TIMER0_CTL_ENABLE), timer_base + TIMER0_CTL_REG);
		break;
	}
}

static int sunxi_clkevt_next_event(unsigned long evt,
				   struct clock_event_device *unused)
{
	unsigned long flags;
	volatile u32 ctrl = 0;
	if(unused){
		spin_lock_irqsave(&timer0_spin_lock,flags);

		/*disable timer0*/
		ctrl = readl(timer_base + TIMER0_CTL_REG);
		ctrl &=(~(1<<0));
		writel(ctrl,(timer_base+TIMER0_CTL_REG));
		udelay(1);

		/* set timer intervalue */
		writel(evt,(timer_base+TIMER0_INTVAL_REG));
		udelay(1);

        /*reload the timer intervalue*/
		ctrl = readl(timer_base + TIMER0_CTL_REG);
		ctrl |= (1<<1);
		writel(ctrl,(timer_base + TIMER0_CTL_REG));
		udelay(1);

        /*enable timer0*/
		ctrl = readl(timer_base + TIMER0_CTL_REG);
		ctrl |= (1<<0);
		writel(ctrl,(timer_base + TIMER0_CTL_REG));
		spin_unlock_irqrestore(&timer0_spin_lock,flags);

	}else{
		pr_warn("[%s][line-%d]set next event error!\n",__func__,__LINE__);
		BUG();
	return -EINVAL;
}

	return 0;
}

static struct clock_event_device sunxi_clockevent = {
	.name = "sunxi_tick",
	.shift = 32,
	.rating = 300,
	.features = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.set_mode = sunxi_clkevt_mode,
	.set_next_event = sunxi_clkevt_next_event,
	.broadcast = smp_timer_broadcast,
};


static irqreturn_t sunxi_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = (struct clock_event_device *)dev_id;

	writel(0x1, timer_base + TIMER_IRQ_ST_REG);
	evt->event_handler(evt);

	return IRQ_HANDLED;
}

static struct irqaction sunxi_timer_irq = {
	.name = "sunxi_timer0",
	.flags = IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
	.handler = sunxi_timer_interrupt,
	.dev_id = &sunxi_clockevent,
};

#ifdef CONFIG_OF
static struct of_device_id sunxi_timer_dt_ids[] = {
	{ .compatible = "allwinner,sunxi-timer" },
	{ }
};
#endif

void __init sunxi_timer_init(void)
{
#ifdef CONFIG_OF
	struct device_node *node;
	struct clk *clk;
#endif
	unsigned long rate = 0;
	int ret, irq;
	u32 val;

#ifdef CONFIG_OF
	node = of_find_matching_node(NULL, sunxi_timer_dt_ids);
	if (!node)
		panic("No sunxi timer node");

	timer_base = of_iomap(node, 0);
	if (!timer_base)
		panic("Can't map registers");

	irq = irq_of_parse_and_map(node, 0);
	if (irq <= 0)
		panic("Can't parse IRQ");

	clk = of_clk_get(node, 0);
	if (IS_ERR(clk))
		panic("Can't get timer clock");

	rate = clk_get_rate(clk);
#else
	timer_base = (void __iomem *)SUNXI_TIMER_VBASE;
	irq = SUNXI_IRQ_TIMER0;
#if defined(CONFIG_ARCH_SUN8IW3P1) && defined(CONFIG_FPGA_V4_PLATFORM)
	rate = 32000; /* it is proved by test that clk-src=32000, prescale=1 on fpga */
#else
	rate = 24000000;
#endif
#endif

	writel(rate / (TIMER_SCAL * HZ),
	       timer_base + TIMER0_INTVAL_REG);

	/* set clock source to HOSC, 16 pre-division */
	val = readl(timer_base + TIMER0_CTL_REG);
	val &= ~(0x07 << 4);
	val &= ~(0x03 << 2);
	val |= (4 << 4) | (1 << 2);
	writel(val, timer_base + TIMER0_CTL_REG);

	/* set mode to auto reload */
	val = readl(timer_base + TIMER0_CTL_REG);
	writel(val | TIMER0_CTL_AUTORELOAD, timer_base + TIMER0_CTL_REG);

	ret = setup_irq(irq, &sunxi_timer_irq);
	if (ret)
		pr_warn("failed to setup irq %d\n", irq);

	/* Enable timer0 interrupt */
	val = readl(timer_base + TIMER_CTL_REG);
	writel(val | TIMER_CTL_ENABLE, timer_base + TIMER_CTL_REG);

	sunxi_clockevent.mult = div_sc(rate / TIMER_SCAL,
				NSEC_PER_SEC,
				sunxi_clockevent.shift);
	sunxi_clockevent.max_delta_ns = clockevent_delta2ns(0x7fffffff,
							    &sunxi_clockevent);
	sunxi_clockevent.min_delta_ns = clockevent_delta2ns(0x10,
							    &sunxi_clockevent);
	sunxi_clockevent.cpumask = cpumask_of(0);

	clockevents_register_device(&sunxi_clockevent);
}
