#ifndef _SUNXI_NMI_H
#define _SUNXI_NMI_H

#define NMI_MODULE_NAME "nmi"

#define NMI_IRQ_LOW_LEVEL       (0x0)
#define NMI_IRQ_NE_EDGE         (0x1)
#define NMI_IRQ_HIGH_LEVEL      (0x2)
#define NMI_IRQ_PO_EDGE         (0x3)

struct nmi_struct {
	void __iomem *base_addr;
	u32 nmi_irq_ctrl;
	u32 nmi_irq_en;
	u32 nmi_irq_status;
};

struct nmi_struct *nmi_data;

static inline void __clear_nmi_status(void)
{
	if (nmi_data->base_addr)
		writel(0x1, nmi_data->base_addr + nmi_data->nmi_irq_status);
	else
		pr_err("%s: para invalid\n", __func__);
}

static inline void __enable_nmi_irq(void)
{
	if (nmi_data->base_addr)
		writel(0x1, nmi_data->base_addr + nmi_data->nmi_irq_en);
	else
		pr_err("%s: para invalid\n", __func__);
}

static inline void __disable_nmi_irq(void)
{
	if (nmi_data->base_addr)
		writel(0x0, nmi_data->base_addr + nmi_data->nmi_irq_en);
	else
		pr_err("%s: para invalid\n", __func__);
}

static inline void __set_nmi_trigger(unsigned int value)
{
	if (nmi_data->base_addr)
		writel(value, nmi_data->base_addr + nmi_data->nmi_irq_ctrl);
	else
		pr_err("%s: para invalid\n", __func__);
}

#endif /* _SUNXI_NMI_H */
