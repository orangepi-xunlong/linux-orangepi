// SPDX-License-Identifier: GPL-2.0

/*
 * Copyright 2017-2018 Cadence
 *
 * Authors:
 *  Jan Kotas <jank@cadence.com>
 *  Boris Brezillon <boris.brezillon@free-electrons.com>
 */

#include <linux/acpi.h>
#include <linux/gpio/driver.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/screen_info.h>
#include <linux/spinlock.h>
#include <linux/acpi.h>
#include <linux/reset.h>
#include <linux/pm.h>
#include <linux/of_irq.h>

#define CDNS_GPIO_BYPASS_MODE		0x00
#define CDNS_GPIO_DIRECTION_MODE	0x04
#define CDNS_GPIO_OUTPUT_EN		0x08
#define CDNS_GPIO_OUTPUT_VALUE		0x0c
#define CDNS_GPIO_INPUT_VALUE		0x10
#define CDNS_GPIO_IRQ_MASK		0x14
#define CDNS_GPIO_IRQ_EN		0x18
#define CDNS_GPIO_IRQ_DIS		0x1c
#define CDNS_GPIO_IRQ_STATUS		0x20
#define CDNS_GPIO_IRQ_TYPE		0x24
#define CDNS_GPIO_IRQ_VALUE		0x28
#define CDNS_GPIO_IRQ_ANY_EDGE		0x2c

#define SKY1_SIP_GPIO_PDC               0xC2000009
#define SKY1_SIP_GPIO_PDC_SET_WAKE      0x02
#define GPIO_ENABLE_WAKE                1

struct cdns_gpio_reg_saved {
	u32 bypass_mode;
	u32 direction_mode;
	u32 output_en;
	u32 output_value;
	u32 irq_mask;
	u32 irq_en;
	u32 irq_dis;
	u32 irq_type;
	u32 irq_value;
	u32 irq_any_edge;
	u32 wake_en;
};

struct cdns_gpio_chip {
	struct gpio_chip gc;
	struct clk *pclk;
	int id;
	void __iomem *regs;
	u32 bypass_orig;
	struct reset_control *apb_reset;
	struct cdns_gpio_reg_saved gpio_saved_reg;
	u32 hw_irq;
};

static unsigned int cdns_gpio_base = 0;

static int cdns_gpio_request(struct gpio_chip *chip, unsigned int offset)
{
	struct cdns_gpio_chip *cgpio = gpiochip_get_data(chip);
	unsigned long flags;

	raw_spin_lock_irqsave(&chip->bgpio_lock, flags);

	iowrite32(ioread32(cgpio->regs + CDNS_GPIO_BYPASS_MODE) & ~BIT(offset),
		  cgpio->regs + CDNS_GPIO_BYPASS_MODE);

	raw_spin_unlock_irqrestore(&chip->bgpio_lock, flags);
	return 0;
}

static void cdns_gpio_free(struct gpio_chip *chip, unsigned int offset)
{
	struct cdns_gpio_chip *cgpio = gpiochip_get_data(chip);
	unsigned long flags;

	raw_spin_lock_irqsave(&chip->bgpio_lock, flags);

	iowrite32(ioread32(cgpio->regs + CDNS_GPIO_BYPASS_MODE) |
		  (BIT(offset) & cgpio->bypass_orig),
		  cgpio->regs + CDNS_GPIO_BYPASS_MODE);

	raw_spin_unlock_irqrestore(&chip->bgpio_lock, flags);
}

static void cdns_gpio_irq_mask(struct irq_data *d)
{
	struct gpio_chip *chip = irq_data_get_irq_chip_data(d);
	struct cdns_gpio_chip *cgpio = gpiochip_get_data(chip);

	iowrite32(BIT(d->hwirq), cgpio->regs + CDNS_GPIO_IRQ_DIS);
	gpiochip_disable_irq(chip, irqd_to_hwirq(d));
}

static void cdns_gpio_irq_unmask(struct irq_data *d)
{
	struct gpio_chip *chip = irq_data_get_irq_chip_data(d);
	struct cdns_gpio_chip *cgpio = gpiochip_get_data(chip);

	gpiochip_enable_irq(chip, irqd_to_hwirq(d));
	iowrite32(BIT(d->hwirq), cgpio->regs + CDNS_GPIO_IRQ_EN);
}

static int cdns_gpio_irq_set_type(struct irq_data *d, unsigned int type)
{
	struct gpio_chip *chip = irq_data_get_irq_chip_data(d);
	struct cdns_gpio_chip *cgpio = gpiochip_get_data(chip);
	unsigned long flags;
	u32 int_value;
	u32 int_type;
	u32 int_oany;
	u32 mask = BIT(d->hwirq);
	int ret = 0;

	raw_spin_lock_irqsave(&chip->bgpio_lock, flags);

	int_value = ioread32(cgpio->regs + CDNS_GPIO_IRQ_VALUE) & ~mask;
	int_type = ioread32(cgpio->regs + CDNS_GPIO_IRQ_TYPE) & ~mask;
	int_oany = ioread32(cgpio->regs + CDNS_GPIO_IRQ_ANY_EDGE) & ~mask;

	/*
	 * The GPIO controller doesn't have an ACK register.
	 * All interrupt statuses are cleared on a status register read.
	 * Don't support edge interrupts for now.
	 */

	if (type == IRQ_TYPE_EDGE_RISING) {
		int_value |= mask;
	} else if (type == IRQ_TYPE_EDGE_FALLING) {
		int_value = int_value;
	} else if (type == IRQ_TYPE_EDGE_BOTH) {
		int_oany |= mask;
	} else if (type == IRQ_TYPE_LEVEL_HIGH) {
		int_type |= mask;
		int_value |= mask;
	} else  if (type == IRQ_TYPE_LEVEL_LOW) {
		int_type |= mask;
	} else {
		ret = -EINVAL;
		goto err_irq_type;
	}

	iowrite32(int_value, cgpio->regs + CDNS_GPIO_IRQ_VALUE);
	iowrite32(int_type, cgpio->regs + CDNS_GPIO_IRQ_TYPE);
	iowrite32(int_oany, cgpio->regs + CDNS_GPIO_IRQ_ANY_EDGE);
err_irq_type:
	raw_spin_unlock_irqrestore(&chip->bgpio_lock, flags);
	return ret;
}

static int cdns_gpio_irq_set_wake(struct irq_data *d, unsigned int enable)
{
	struct gpio_chip *chip = irq_data_get_irq_chip_data(d);
	struct cdns_gpio_chip *cgpio = gpiochip_get_data(chip);

	irq_hw_number_t bit = irqd_to_hwirq(d);

	if (enable)
		cgpio->gpio_saved_reg.wake_en |= BIT(bit);
	else
		cgpio->gpio_saved_reg.wake_en &= ~BIT(bit);
	return 0;
}

static void cdns_gpio_irq_handler(struct irq_desc *desc)
{
	struct gpio_chip *chip = irq_desc_get_handler_data(desc);
	struct cdns_gpio_chip *cgpio = gpiochip_get_data(chip);
	struct irq_chip *irqchip = irq_desc_get_chip(desc);
	unsigned long status;
	int hwirq;

	chained_irq_enter(irqchip, desc);

	status = ioread32(cgpio->regs + CDNS_GPIO_IRQ_STATUS) &
		~ioread32(cgpio->regs + CDNS_GPIO_IRQ_MASK);

	for_each_set_bit(hwirq, &status, chip->ngpio)
		generic_handle_domain_irq(chip->irq.domain, hwirq);

	chained_irq_exit(irqchip, desc);
}

static const struct irq_chip cdns_gpio_irqchip = {
	.name		= "cdns-gpio",
	.irq_mask	= cdns_gpio_irq_mask,
	.irq_unmask	= cdns_gpio_irq_unmask,
	.irq_set_type	= cdns_gpio_irq_set_type,
	.irq_set_wake   = cdns_gpio_irq_set_wake,
	.flags		= IRQCHIP_IMMUTABLE,
	GPIOCHIP_IRQ_RESOURCE_HELPERS,
};

static int cdns_gpio_probe(struct platform_device *pdev)
{
	struct cdns_gpio_chip *cgpio;
	struct irq_data *irq_data = NULL;
	int ret, irq;
	u32 dir_prev;
	u32 num_gpios = 32;
	u32 gmask;

	cgpio = devm_kzalloc(&pdev->dev, sizeof(*cgpio), GFP_KERNEL);
	if (!cgpio)
		return -ENOMEM;

	cgpio->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(cgpio->regs))
		return PTR_ERR(cgpio->regs);

	ret = device_property_read_u32(&pdev->dev, "ngpios", &num_gpios);
	if (ret < 0 || num_gpios > 32) {
		dev_err(&pdev->dev, "ngpios must be less or equal 32\n");
		return -EINVAL;
	}

	cgpio->id = of_alias_get_id(pdev->dev.of_node, "gpio");
	if (cgpio->id < 0)
		if (device_property_read_u32(&pdev->dev, "id", &cgpio->id))
			return cgpio->id;

	ret = device_property_read_u32(&pdev->dev, "gpio-io-mask", &gmask);
	if (ret < 0)
		gmask = 0;
	/*
	 * Set all pins as inputs by default, otherwise:
	 * gpiochip_lock_as_irq:
	 * tried to flag a GPIO set as output for IRQ
	 * Generic GPIO driver stores the direction value internally,
	 * so it needs to be changed before bgpio_init() is called.
	 */
	dir_prev = ioread32(cgpio->regs + CDNS_GPIO_DIRECTION_MODE);
	iowrite32(GENMASK(num_gpios - 1, 0) & ~gmask,
		  cgpio->regs + CDNS_GPIO_DIRECTION_MODE);

	ret = bgpio_init(&cgpio->gc, &pdev->dev, 4,
			 cgpio->regs + CDNS_GPIO_INPUT_VALUE,
			 cgpio->regs + CDNS_GPIO_OUTPUT_VALUE,
			 NULL,
			 NULL,
			 cgpio->regs + CDNS_GPIO_DIRECTION_MODE,
			 BGPIOF_READ_OUTPUT_REG_SET);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register generic gpio, %d\n",
			ret);
		goto err_revert_dir;
	}

	cgpio->gc.label = dev_name(&pdev->dev);
	cgpio->gc.ngpio = num_gpios;
	cgpio->gc.parent = &pdev->dev;
	cgpio->gc.base = cdns_gpio_base;
	cdns_gpio_base += num_gpios;
	cgpio->gc.owner = THIS_MODULE;
	cgpio->gc.request = cdns_gpio_request;
	cgpio->gc.free = cdns_gpio_free;

	cgpio->pclk = devm_clk_get_optional(&pdev->dev, NULL);
	if (IS_ERR(cgpio->pclk)) {
		ret = PTR_ERR(cgpio->pclk);
		dev_err(&pdev->dev,
			"Failed to retrieve peripheral clock, %d\n", ret);
		cgpio->pclk = NULL;
	}

	if (cgpio->pclk)
	{
		ret = clk_prepare_enable(cgpio->pclk);
		if (ret) {
			dev_err(&pdev->dev,
				"Failed to enable the peripheral clock, %d\n", ret);
			goto err_revert_dir;
		}
	}

	cgpio->apb_reset = devm_reset_control_get_optional_shared(&pdev->dev, "apb_reset");
	if (IS_ERR(cgpio->apb_reset)) {
		dev_info(&pdev->dev, "[%s:%d]get reset error\n", __func__, __LINE__);
		cgpio->apb_reset = NULL;
	}

	/* disalbe all irq to prevent unpredictable events */
	iowrite32(BIT(num_gpios) - 1, cgpio->regs + CDNS_GPIO_IRQ_DIS);

	/*
	 * Optional irq_chip support
	 */
	irq = platform_get_irq(pdev, 0);
	if (irq >= 0) {
		struct gpio_irq_chip *girq;

		girq = &cgpio->gc.irq;
		gpio_irq_chip_set_chip(girq, &cdns_gpio_irqchip);
		girq->parent_handler = cdns_gpio_irq_handler;
		girq->num_parents = 1;
		girq->parents = devm_kcalloc(&pdev->dev, 1,
					     sizeof(*girq->parents),
					     GFP_KERNEL);
		if (!girq->parents) {
			ret = -ENOMEM;
			goto err_disable_clk;
		}
		girq->parents[0] = irq;
		girq->default_type = IRQ_TYPE_NONE;
		girq->handler = handle_level_irq;
	}

	irq_data = irq_get_irq_data(irq);
	cgpio->hw_irq = irq_data->hwirq;

	ret = devm_gpiochip_add_data(&pdev->dev, &cgpio->gc, cgpio);
	if (ret < 0) {
		dev_err(&pdev->dev, "Could not register gpiochip, %d\n", ret);
		goto err_disable_clk;
	}

	cgpio->bypass_orig = ioread32(cgpio->regs + CDNS_GPIO_BYPASS_MODE);

	/*
	 * Enable gpio outputs, ignored for input direction
	 */
	iowrite32(GENMASK(num_gpios - 1, 0),
		  cgpio->regs + CDNS_GPIO_OUTPUT_EN);
	iowrite32(0, cgpio->regs + CDNS_GPIO_BYPASS_MODE);

	platform_set_drvdata(pdev, cgpio);
	return 0;

err_disable_clk:
	clk_disable_unprepare(cgpio->pclk);

err_revert_dir:
	iowrite32(dir_prev, cgpio->regs + CDNS_GPIO_DIRECTION_MODE);

	return ret;
}

static int cdns_gpio_remove(struct platform_device *pdev)
{
	struct cdns_gpio_chip *cgpio = platform_get_drvdata(pdev);

	iowrite32(cgpio->bypass_orig, cgpio->regs + CDNS_GPIO_BYPASS_MODE);
	clk_disable_unprepare(cgpio->pclk);

	return 0;
}

static void cdns_gpio_shutdown(struct platform_device *pdev)
{
        struct cdns_gpio_chip *cgpio = platform_get_drvdata(pdev);
        struct arm_smccc_res res;
        u32 gpio_wake_en;

        if (cgpio->pclk) {
		/* Mask out interrupts */
		iowrite32(~cgpio->gpio_saved_reg.wake_en, cgpio->regs + CDNS_GPIO_IRQ_DIS);
		gpio_wake_en = cgpio->gpio_saved_reg.wake_en;
		arm_smccc_smc(SKY1_SIP_GPIO_PDC, SKY1_SIP_GPIO_PDC_SET_WAKE,
			cgpio->hw_irq - 32, GPIO_ENABLE_WAKE, gpio_wake_en, 0, 0, 0, &res);

                clk_disable_unprepare(cgpio->pclk);
        }
}

static void cdns_gpio_save_regs(struct cdns_gpio_chip *cgpio)
{
	cgpio->gpio_saved_reg.bypass_mode = readl(cgpio->regs + CDNS_GPIO_BYPASS_MODE);
	cgpio->gpio_saved_reg.direction_mode = readl(cgpio->regs + CDNS_GPIO_DIRECTION_MODE);
	cgpio->gpio_saved_reg.output_en = readl(cgpio->regs + CDNS_GPIO_OUTPUT_EN);
	cgpio->gpio_saved_reg.output_value = readl(cgpio->regs + CDNS_GPIO_OUTPUT_VALUE);
	cgpio->gpio_saved_reg.irq_en = readl(cgpio->regs + CDNS_GPIO_IRQ_MASK);
	cgpio->gpio_saved_reg.irq_type = readl(cgpio->regs + CDNS_GPIO_IRQ_TYPE);
	cgpio->gpio_saved_reg.irq_value = readl(cgpio->regs + CDNS_GPIO_IRQ_VALUE);
	cgpio->gpio_saved_reg.irq_any_edge = readl(cgpio->regs + CDNS_GPIO_IRQ_ANY_EDGE);
}

static void cdns_gpio_restore_regs(struct cdns_gpio_chip *cgpio)
{
	writel(cgpio->gpio_saved_reg.bypass_mode, cgpio->regs + CDNS_GPIO_BYPASS_MODE);
	writel(cgpio->gpio_saved_reg.direction_mode, cgpio->regs + CDNS_GPIO_DIRECTION_MODE);
	writel(cgpio->gpio_saved_reg.output_en, cgpio->regs + CDNS_GPIO_OUTPUT_EN);
	writel(cgpio->gpio_saved_reg.output_value ,cgpio->regs + CDNS_GPIO_OUTPUT_VALUE);
	writel(cgpio->gpio_saved_reg.irq_type, cgpio->regs + CDNS_GPIO_IRQ_TYPE);
	writel(cgpio->gpio_saved_reg.irq_value, cgpio->regs + CDNS_GPIO_IRQ_VALUE);
	writel(cgpio->gpio_saved_reg.irq_any_edge, cgpio->regs + CDNS_GPIO_IRQ_ANY_EDGE);
	writel(~cgpio->gpio_saved_reg.irq_en, cgpio->regs + CDNS_GPIO_IRQ_EN);
}

/**
 * cdns_gpio_suspend - Suspend method for GPIO driver
 * @dev:	Address of gpio device structure
 *
 * This function disable gpio device and changes
 * the driver state to "suspend"
 *
 * Return: 0 on success and error value on error
 *
 */

static int __maybe_unused cdns_gpio_suspend(struct device *dev)
{
	struct cdns_gpio_chip *cgpio = dev_get_drvdata(dev);
	struct arm_smccc_res res;
	u32 gpio_wake_en;

	if (cgpio->pclk) {
		cdns_gpio_save_regs(cgpio);
		iowrite32(~cgpio->gpio_saved_reg.wake_en, cgpio->regs + CDNS_GPIO_IRQ_DIS);

		gpio_wake_en = cgpio->gpio_saved_reg.wake_en;
		arm_smccc_smc(SKY1_SIP_GPIO_PDC, SKY1_SIP_GPIO_PDC_SET_WAKE,
			cgpio->hw_irq - 32, GPIO_ENABLE_WAKE, gpio_wake_en, 0, 0, 0, &res);

		clk_disable_unprepare(cgpio->pclk);
	}

	return 0;
}

/**
 * cdns_gpio_resume - Resume method for GPIO driver
 * @dev:	Address of the GPIO device structure
 *
 * This function changes the driver state to "ready"
 *
 * Retrun: 0 on success and error value on error
 */

static int __maybe_unused cdns_gpio_resume(struct device *dev)
{

	int ret;
	struct cdns_gpio_chip *cgpio = dev_get_drvdata(dev);
	if (cgpio->pclk) {
		ret = clk_prepare_enable(cgpio->pclk);
		if (ret) {
			dev_err(dev,
				"Failed to enable the peripheral clock, %d\n", ret);
			return ret;
		}

		/* reset cgpio */
		if (cgpio->apb_reset)
			reset_control_reset(cgpio->apb_reset);

		cdns_gpio_restore_regs(cgpio);
	}
	return 0;
}


static const struct dev_pm_ops cdns_gpio_dev_ops = {
	LATE_SYSTEM_SLEEP_PM_OPS(cdns_gpio_suspend, cdns_gpio_resume)
};

static const struct of_device_id cdns_of_ids[] = {
	{ .compatible = "cdns,gpio-r1p02" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, cdns_of_ids);

static const struct acpi_device_id cdns_acpi_ids[] = {
	{"CIXH1002", 0},
	{"CIXH1003", 0},
	{"", 0},
};
MODULE_DEVICE_TABLE(acpi, cdns_acpi_ids);

static struct platform_driver cdns_gpio_driver = {
	.driver = {
		.name = "cdns-gpio",
		.of_match_table = cdns_of_ids,
		.acpi_match_table = ACPI_PTR(cdns_acpi_ids),
		.pm = &cdns_gpio_dev_ops,
	},
	.probe = cdns_gpio_probe,
	.remove = cdns_gpio_remove,
	.shutdown = cdns_gpio_shutdown,
};
module_platform_driver(cdns_gpio_driver);

MODULE_AUTHOR("Jan Kotas <jank@cadence.com>");
MODULE_DESCRIPTION("Cadence GPIO driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:cdns-gpio");
