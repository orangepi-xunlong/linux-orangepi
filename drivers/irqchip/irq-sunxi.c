/*
 * SUNXI WakeupGen Source file
 *
 * SUNXI WakeupGen is the interrupt controller extension used along
 * with ARM GIC to wake the CPU out from low power states on
 * external interrupts. It is responsible for generating wakeup
 * event from the incoming interrupts and enable bits. It is
 * implemented in PMU always ON power domain. During normal operation,
 * WakeupGen delivers external interrupts directly to the GIC.
 *
 * Copyright (C) 2017 Allwinner Technology, Inc.
 *	fanqinghua <fanqinghua@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/irqdomain.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/cpu.h>
#include <linux/notifier.h>
#include <linux/cpu_pm.h>
#include <asm/cacheflush.h>

#define GIC_SUPPORT_IRQS 1024

static void __iomem *wakeupgen_base;
unsigned int wakeupgen_size;
static DEFINE_SPINLOCK(irq_wake_lock);

static int sunxi_irq_set_wake(struct irq_data *d, unsigned int on)
{
	unsigned int idx = d->hwirq / 32;
	u32 mask, tmp;

	spin_lock(&irq_wake_lock);
	mask = 1 << d->hwirq % 32;
	tmp = readl_relaxed(wakeupgen_base + idx * 4);
	tmp = on ? tmp | mask : tmp & ~mask;
	writel_relaxed(tmp, wakeupgen_base + idx * 4);
	__dma_flush_area(wakeupgen_base, wakeupgen_size);
	spin_unlock(&irq_wake_lock);
	/*
	 * Do *not* call into the parent, as the GIC doesn't have any
	 * wake-up facility...
	 */
	return 0;
}

static struct irq_chip wakeupgen_chip = {
	.name			= "wakeupgen",
	.irq_eoi		= irq_chip_eoi_parent,
	.irq_mask		= irq_chip_mask_parent,
	.irq_unmask		= irq_chip_unmask_parent,
	.irq_retrigger		= irq_chip_retrigger_hierarchy,
	.irq_set_wake		= sunxi_irq_set_wake,
	.irq_set_type           = irq_chip_set_type_parent,
#ifdef CONFIG_SMP
	.irq_set_affinity	= irq_chip_set_affinity_parent,
#endif
};

static int sunxi_domain_translate(struct irq_domain *d,
				    struct irq_fwspec *fwspec,
				    unsigned long *hwirq,
				    unsigned int *type)
{
	if (is_of_node(fwspec->fwnode)) {
		if (fwspec->param_count != 3)
			return -EINVAL;

		/* No PPI should point to this domain */
		if (fwspec->param[0] != 0)
			return -EINVAL;

		*hwirq = fwspec->param[1];
		*type = fwspec->param[2];
		return 0;
	}

	return -EINVAL;
}

static int sunxi_domain_alloc(struct irq_domain *domain,
				  unsigned int irq,
				  unsigned int nr_irqs, void *data)
{
	struct irq_fwspec *fwspec = data;
	struct irq_fwspec parent_fwspec;
	irq_hw_number_t hwirq;
	int i;

	if (fwspec->param_count != 3)
		return -EINVAL;	/* Not GIC compliant */
	if (fwspec->param[0] != 0)
		return -EINVAL;	/* No PPI should point to this domain */

	hwirq = fwspec->param[1];
	if (hwirq >= GIC_SUPPORT_IRQS)
		return -EINVAL;	/* Can't deal with this */

	for (i = 0; i < nr_irqs; i++)
		irq_domain_set_hwirq_and_chip(domain, irq + i, hwirq + i,
					      &wakeupgen_chip, NULL);

	parent_fwspec = *fwspec;
	parent_fwspec.fwnode = domain->parent->fwnode;
	return irq_domain_alloc_irqs_parent(domain, irq, nr_irqs,
					    &parent_fwspec);
}

static const struct irq_domain_ops sunxi_domain_ops = {
	.translate	= sunxi_domain_translate,
	.alloc		= sunxi_domain_alloc,
	.free		= irq_domain_free_irqs_common,
};

static int __init wakeupgen_init(struct device_node *node,
			       struct device_node *parent)
{
	struct irq_domain *parent_domain, *domain;
	unsigned int value[2] = { 0, 0 };
	int i, ret;

	if (!parent) {
		pr_err("%s: no parent, giving up\n", node->full_name);
		return -ENODEV;
	}

	parent_domain = irq_find_host(parent);
	if (!parent_domain) {
		pr_err("%s: unable to obtain parent domain\n", node->full_name);
		return -ENXIO;
	}

	ret = of_property_read_u32_array(node, "space1", value,
				ARRAY_SIZE(value));
	if (ret) {
		pr_err("get sunxi_irq_space1 err.\n");
		return -EINVAL;
	}

	pr_err("%s, 0x%x, 0x%x\n", __func__,
			value[0], value[1]);

	wakeupgen_base = phys_to_virt(value[0]);
	wakeupgen_size = value[1];

	domain = irq_domain_add_hierarchy(parent_domain, 0, GIC_SUPPORT_IRQS,
					  node, &sunxi_domain_ops,
					  NULL);
	if (!domain) {
		iounmap(wakeupgen_base);
		return -ENOMEM;
	}

	/* Initially mask all interrupts */
	for (i = 0; i < SZ_2K / sizeof(int); i++)
		writel_relaxed(0, wakeupgen_base + i * sizeof(int));
	__dma_flush_area(wakeupgen_base, wakeupgen_size);

	return 0;
}
IRQCHIP_DECLARE(sunxi_wakeupgen, "allwinner,sunxi-wakeupgen", wakeupgen_init);
