/*
 * Allwinner wakeup irq support.
 *
 *  Copyright (C) 2019 Allwinner Technology, Inc.
 *	fanqinghua <fanqinghua@allwinnertech.com>
 *
 * SUNXI WakeupGen is the interrupt controller extension used along
 * with ARM GIC to wake the CPU out from low power states on
 * external interrupts. It is responsible for generating wakeup
 * event from the incoming interrupts and enable bits. It is
 * implemented in PMU always ON power domain. During normal operation,
 * WakeupGen delivers external interrupts directly to the GIC.
 *
 * Copyright (C) 2017 Allwinner Technology, Inc.
 * Author: Fan Qinghua <fanqinghua@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/irqchip.h>
#include <linux/irqdomain.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/cpu.h>
#include <linux/notifier.h>
#include <linux/cpu_pm.h>
#include <soc/allwinner/sunxi_sip.h>

#define GIC_SUPPORT_IRQS 1024

struct sunxi_irq_domain {
	struct device		*dev;
	struct irq_domain	*irqd;
};

static inline int set_wakeup_source(u32 wakeup_irq)
{
	int result;

	result = invoke_scp_fn_smc(SET_WAKEUP_SRC,
								wakeup_irq, 0, 0);

	return result;
}

static inline int clear_wakeup_source(u32 wakeup_irq)
{
	int result;

	result = invoke_scp_fn_smc(CLEAR_WAKEUP_SRC,
								wakeup_irq, 0, 0);

	return result;
}

static int sunxi_irq_set_wake(struct irq_data *d, unsigned int on)
{
	if (on)
		set_wakeup_source(SET_ROOT_WAKEUP_SOURCE(d->hwirq));
	else
		clear_wakeup_source(SET_ROOT_WAKEUP_SOURCE(d->hwirq));
	/*
	 * Do *not* call into the parent, as the GIC doesn't have any
	 * wake-up facility...
	 */
	return 0;
}

static struct irq_chip wakeupgen_chip = {
	.name			= "wakeupgen",
	.irq_enable		= irq_chip_enable_parent,
	.irq_disable		= irq_chip_disable_parent,
	.irq_eoi		= irq_chip_eoi_parent,
	.irq_mask		= irq_chip_mask_parent,
	.irq_unmask		= irq_chip_unmask_parent,
	.irq_retrigger		= irq_chip_retrigger_hierarchy,
	.irq_set_wake		= sunxi_irq_set_wake,
	.irq_set_type           = irq_chip_set_type_parent,
	.irq_set_affinity	= irq_chip_set_affinity_parent,
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

#ifndef MODULE
static int __init wakeupgen_init(struct device_node *node,
			       struct device_node *parent)
{
	struct irq_domain *parent_domain, *domain;

	if (!parent) {
		pr_err("%s: no parent, giving up\n", node->full_name);
		return -ENODEV;
	}

	parent_domain = irq_find_host(parent);
	if (!parent_domain) {
		pr_err("%s: unable to obtain parent domain\n", node->full_name);
		return -ENXIO;
	}

	domain = irq_domain_add_hierarchy(parent_domain, 0, GIC_SUPPORT_IRQS,
					  node, &sunxi_domain_ops,
					  NULL);
	if (!domain) {
		pr_err("%s: failed to allocated domain\n", node->full_name);
		return -ENOMEM;
	}

	return 0;
}
IRQCHIP_DECLARE(sunxi_wakeupgen, "allwinner,sunxi-wakeupgen", wakeupgen_init);
#else
static int sunxi_irq_domain_probe(struct platform_device *pdev)
{
	struct irq_domain *parent_domain;
	struct sunxi_irq_domain *intr;
	struct device_node *parent_node;
	struct device *dev = &pdev->dev;

	parent_node = of_irq_find_parent(dev_of_node(dev));
	if (!parent_node) {
		dev_err(dev, "Failed to get IRQ parent node\n");
		return -ENODEV;
	}

	parent_domain = irq_find_host(parent_node);
	if (!parent_domain) {
		dev_err(dev, "Failed to find IRQ parent domain\n");
		return -ENODEV;
	}

	intr = devm_kzalloc(dev, sizeof(*intr), GFP_KERNEL);
	if (!intr)
		return -ENOMEM;

	intr->dev = dev;
	intr->irqd = irq_domain_add_hierarchy(parent_domain, 0,
											GIC_SUPPORT_IRQS,
											dev_of_node(dev),
											&sunxi_domain_ops,
											NULL);
	if (IS_ERR(intr->irqd)) {
		dev_err(dev, "Failed to allocate IRQ domain\n");
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, intr);

	return 0;
}

static int sunxi_irq_domain_remove(struct platform_device *pdev)
{
	struct sunxi_irq_domain *intr = platform_get_drvdata(pdev);

	irq_domain_remove(intr->irqd);
	return 0;
}

static const struct of_device_id sunxi_irq_domain_of_match[] = {
	{ .compatible = "allwinner,sunxi-wakeupgen", },
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_irq_domain_of_match);

static struct platform_driver sunxi_irq_domain_driver = {
	.probe = sunxi_irq_domain_probe,
	.remove	= sunxi_irq_domain_remove,
	.driver = {
		.name = "sunxi_wakeupgen",
		.of_match_table = sunxi_irq_domain_of_match,
	},
};
module_platform_driver(sunxi_irq_domain_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Allwinner wakeupgen driver");
MODULE_ALIAS("platform: sunxi_wakeupgen");
MODULE_AUTHOR("fanqinghua <fanqinghua@allwinnertech.com>");
MODULE_VERSION("1.0.0");
#endif
