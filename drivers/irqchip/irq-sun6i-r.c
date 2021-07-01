// SPDX-License-Identifier: GPL-2.0-only
//
// Allwinner A31 and newer SoCs R_INTC driver
//

#include <linux/atomic.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/syscore_ops.h>

#include <dt-bindings/interrupt-controller/arm-gic.h>

#define NMI_HWIRQ		0
#define NMI_HWIRQ_BIT		BIT(NMI_HWIRQ)

#define SUN6I_R_INTC_NR_IRQS	16

#define SUN6I_R_INTC_NMI_CTRL	0x0c
#define SUN6I_R_INTC_PENDING	0x10
#define SUN6I_R_INTC_ENABLE	0x40

static void __iomem *base;
static irq_hw_number_t parent_offset;
static u32 parent_type;
#ifdef CONFIG_PM_SLEEP
static atomic_t wake_mask;
#endif

static struct irq_chip sun6i_r_intc_edge;
static struct irq_chip sun6i_r_intc_level;

static void sun6i_r_intc_nmi_ack(void)
{
	/*
	 * The NMI IRQ channel has a latch, separate from its trigger.
	 * This latch must be cleared to clear the output to the GIC.
	 */
	writel_relaxed(NMI_HWIRQ_BIT, base + SUN6I_R_INTC_PENDING);
}

static void sun6i_r_intc_irq_ack(struct irq_data *data)
{
	if (data->hwirq == NMI_HWIRQ)
		sun6i_r_intc_nmi_ack();
}

static void sun6i_r_intc_irq_unmask(struct irq_data *data)
{
	if (data->hwirq == NMI_HWIRQ)
		sun6i_r_intc_nmi_ack();

	irq_chip_unmask_parent(data);
}

static int sun6i_r_intc_irq_set_type(struct irq_data *data, unsigned int type)
{
	/*
	 * Only the NMI IRQ is routed through this interrupt controller on its
	 * way to the GIC. Other IRQs are routed to the GIC in parallel and
	 * must have a trigger type appropriate for the GIC.
	 *
	 * The "External NMI" input to the GIC actually comes from bit 0 of
	 * this device's PENDING register. So the IRQ type of the NMI, as seen
	 * by the GIC, does not depend on the IRQ type of the NMI pin itself.
	 */
	if (data->hwirq == NMI_HWIRQ) {
		u32 nmi_src_type;

		switch (type) {
		case IRQ_TYPE_LEVEL_LOW:
			nmi_src_type = 0;
			break;
		case IRQ_TYPE_EDGE_FALLING:
			nmi_src_type = 1;
			break;
		case IRQ_TYPE_LEVEL_HIGH:
			nmi_src_type = 2;
			break;
		case IRQ_TYPE_EDGE_RISING:
			nmi_src_type = 3;
			break;
		default:
			pr_err("%pOF: invalid trigger type %d for IRQ %d\n",
			       irq_domain_get_of_node(data->domain), type,
			       data->irq);
			return -EBADR;
		}

		if (type & IRQ_TYPE_EDGE_BOTH) {
			irq_set_chip_handler_name_locked(data,
							 &sun6i_r_intc_edge,
							 handle_fasteoi_ack_irq,
							 NULL);
		} else {
			irq_set_chip_handler_name_locked(data,
							 &sun6i_r_intc_level,
							 handle_fasteoi_irq,
							 NULL);
		}

		writel_relaxed(nmi_src_type, base + SUN6I_R_INTC_NMI_CTRL);

		/* Send the R_INTC -> GIC trigger type to the GIC driver. */
		type = parent_type;
	}

	return irq_chip_set_type_parent(data, type);
}

#ifdef CONFIG_PM_SLEEP
static int sun6i_r_intc_irq_set_wake(struct irq_data *data, unsigned int on)
{
	if (on)
		atomic_or(BIT(data->hwirq), &wake_mask);
	else
		atomic_andnot(BIT(data->hwirq), &wake_mask);

	return 0;
}
#else
#define sun6i_r_intc_irq_set_wake NULL
#endif

static struct irq_chip sun6i_r_intc_edge = {
	.name			= "sun6i-r-intc",
	.irq_ack		= sun6i_r_intc_irq_ack,
	.irq_mask		= irq_chip_mask_parent,
	.irq_unmask		= irq_chip_unmask_parent,
	.irq_eoi		= irq_chip_eoi_parent,
	.irq_set_affinity	= irq_chip_set_affinity_parent,
	.irq_set_type		= sun6i_r_intc_irq_set_type,
	.irq_get_irqchip_state	= irq_chip_get_parent_state,
	.irq_set_irqchip_state	= irq_chip_set_parent_state,
	.irq_set_wake		= sun6i_r_intc_irq_set_wake,
	.irq_set_vcpu_affinity	= irq_chip_set_vcpu_affinity_parent,
	.flags			= IRQCHIP_SET_TYPE_MASKED,
};

static struct irq_chip sun6i_r_intc_level = {
	.name			= "sun6i-r-intc",
	.irq_mask		= irq_chip_mask_parent,
	.irq_unmask		= sun6i_r_intc_irq_unmask,
	.irq_eoi		= irq_chip_eoi_parent,
	.irq_set_affinity	= irq_chip_set_affinity_parent,
	.irq_set_type		= sun6i_r_intc_irq_set_type,
	.irq_get_irqchip_state	= irq_chip_get_parent_state,
	.irq_set_irqchip_state	= irq_chip_set_parent_state,
	.irq_set_wake		= sun6i_r_intc_irq_set_wake,
	.irq_set_vcpu_affinity	= irq_chip_set_vcpu_affinity_parent,
	.flags			= IRQCHIP_SET_TYPE_MASKED,
};

static int sun6i_r_intc_domain_alloc(struct irq_domain *domain,
				     unsigned int virq,
				     unsigned int nr_irqs, void *arg)
{
	struct irq_fwspec *fwspec = arg;
	struct irq_fwspec gic_fwspec;
	irq_hw_number_t hwirq;
	unsigned int type;
	int i, ret;

	ret = irq_domain_translate_twocell(domain, fwspec, &hwirq, &type);
	if (ret)
		return ret;
	if (hwirq + nr_irqs > SUN6I_R_INTC_NR_IRQS)
		return -EINVAL;

	/* Construct a GIC-compatible fwspec from this fwspec. */
	gic_fwspec = (struct irq_fwspec) {
		.fwnode      = domain->parent->fwnode,
		.param_count = 3,
		.param       = { GIC_SPI, parent_offset + hwirq, type },
	};

	for (i = 0; i < nr_irqs; ++i)
		irq_domain_set_hwirq_and_chip(domain, virq + i, hwirq + i,
					      &sun6i_r_intc_level, NULL);

	return irq_domain_alloc_irqs_parent(domain, virq, nr_irqs, &gic_fwspec);
}

static const struct irq_domain_ops sun6i_r_intc_domain_ops = {
	.translate	= irq_domain_translate_twocell,
	.alloc		= sun6i_r_intc_domain_alloc,
	.free		= irq_domain_free_irqs_common,
};

#ifdef CONFIG_PM_SLEEP
static int sun6i_r_intc_suspend(void)
{
	/* All wake IRQs are enabled during system sleep. */
	writel_relaxed(atomic_read(&wake_mask), base + SUN6I_R_INTC_ENABLE);

	return 0;
}

static void sun6i_r_intc_resume(void)
{
	/* Only the NMI is relevant during normal operation. */
	writel_relaxed(NMI_HWIRQ_BIT, base + SUN6I_R_INTC_ENABLE);
}

static struct syscore_ops sun6i_r_intc_syscore_ops = {
	.suspend	= sun6i_r_intc_suspend,
	.resume		= sun6i_r_intc_resume,
};

static void sun6i_r_intc_syscore_init(void)
{
	register_syscore_ops(&sun6i_r_intc_syscore_ops);
}
#else
static inline void sun6i_r_intc_syscore_init(void) {}
#endif

static int __init sun6i_r_intc_init(struct device_node *node,
				    struct device_node *parent)
{
	struct irq_domain *domain, *parent_domain;
	struct of_phandle_args parent_irq;
	int ret;

	/* Extract the R_INTC -> GIC mapping from the OF node. */
	ret = of_irq_parse_one(node, 0, &parent_irq);
	if (ret)
		return ret;
	if (parent_irq.args_count != 3 || parent_irq.args[0] != GIC_SPI)
		return -EINVAL;
	parent_offset = parent_irq.args[1];
	parent_type   = parent_irq.args[2];

	parent_domain = irq_find_host(parent);
	if (!parent_domain) {
		pr_err("%pOF: Failed to obtain parent domain\n", node);
		return -ENXIO;
	}

	base = of_io_request_and_map(node, 0, NULL);
	if (IS_ERR(base)) {
		pr_err("%pOF: Failed to map MMIO region\n", node);
		return PTR_ERR(base);
	}

	domain = irq_domain_add_hierarchy(parent_domain, 0,
					  SUN6I_R_INTC_NR_IRQS, node,
					  &sun6i_r_intc_domain_ops, NULL);
	if (!domain) {
		pr_err("%pOF: Failed to allocate domain\n", node);
		iounmap(base);
		return -ENOMEM;
	}

	/* Clear and enable the NMI. */
	writel_relaxed(NMI_HWIRQ_BIT, base + SUN6I_R_INTC_PENDING);
	writel_relaxed(NMI_HWIRQ_BIT, base + SUN6I_R_INTC_ENABLE);

	sun6i_r_intc_syscore_init();

	return 0;
}
IRQCHIP_DECLARE(sun6i_r_intc, "allwinner,sun6i-a31-r-intc", sun6i_r_intc_init);
