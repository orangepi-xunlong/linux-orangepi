/*
 * Cvitek SoCs saradc driver
 *
 * Copyright (c) 2023 Cvitek Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/uaccess.h>
#include <linux/dma-buf.h>
#include <linux/dma-direction.h>
#include <linux/interrupt.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/of.h>

#include "./cvi_saradc.h"
#include "pinctrl-cv186x.h"


enum ADCChannel {
	/* Top domain ADC ch1, ch2, ch3 */
	ADC1 = 1,
	ADC2,
	ADC3,
	/* no die domain ADC ch1, ch2 */
	PWR_ADC1,/* ADC4 <== PWR_GPIO0 */
	PWR_ADC2,/* ADC5 <== PWR_GPIO1 */
};

#define REG_ADC1 (G12_PINMUX_REG_REG_BASE + G12_PINMUX_REG_REG_ADC1)
#define REG_ADC2 (G12_PINMUX_REG_REG_BASE + G12_PINMUX_REG_REG_ADC2)
#define REG_ADC3 (G12_PINMUX_REG_REG_BASE + G12_PINMUX_REG_REG_ADC3)
#define REG_ADC4 (G7_PINMUX_REG_REG_BASE + G7_PINMUX_REG_REG_PWR_GPIO0)
#define REG_ADC5 (G7_PINMUX_REG_REG_BASE + G7_PINMUX_REG_REG_PWR_GPIO1)
#define ADC_IE  (1 << 14)
#define PD_EN   (1 << 0)

static void clrbits(uintptr_t addr, uint32_t clr) {
	void __iomem *vaddr;
	vaddr = ioremap(addr, 0x4);
	writel(readl(vaddr) & ~clr, vaddr);
	iounmap(vaddr);
}

static void io_config(uint32_t channel)
{
	switch (channel) {
	case ADC1:
		PINMUX_CONFIG(ADC1, ADC1, G12);
		clrbits(REG_ADC1, ADC_IE | PD_EN);
		break;
	case ADC2:
		PINMUX_CONFIG(ADC2, ADC2, G12);
		clrbits(REG_ADC2, ADC_IE | PD_EN);
		break;
	case ADC3:
		PINMUX_CONFIG(ADC3, ADC3, G12);
		clrbits(REG_ADC3, ADC_IE | PD_EN);
		break;
	case PWR_ADC1:
		PINMUX_CONFIG(PWR_GPIO0, PWR_SAR0, G7);
		clrbits(REG_ADC4, ADC_IE | PD_EN);
		break;
	case PWR_ADC2:
		PINMUX_CONFIG(PWR_GPIO1, PWR_SAR1, G7);
		clrbits(REG_ADC5, ADC_IE | PD_EN);
		break;
	default:
		pr_err("io_config: invalid channel index\n");
		break;
	}
}

#define	SARADC_CHAN_VOLTAGE(lval, idx, addr)			  \
	{													  \
		lval.type =	IIO_VOLTAGE;						  \
		lval.channel = idx;								  \
		lval.indexed = 1;								  \
		lval.address = addr;							  \
		lval.info_mask_separate	= BIT(IIO_CHAN_INFO_RAW); \
		lval.scan_index	= idx;							  \
		lval.scan_type.sign	= 'u';						  \
		lval.scan_type.realbits	= 12;					  \
		lval.scan_type.storagebits = 16;				  \
		lval.scan_type.endianness =	IIO_LE;				  \
	}

static int platform_saradc_clk_init(struct cvi_saradc_device *ndev)
{
	// enable clock
	if (ndev->clk_saradc) {
		pr_debug("cvi_saradc enable	clock\n");
		clk_prepare_enable(ndev->clk_saradc);
	}

	return 0;
}

static void	platform_saradc_clk_deinit(struct cvi_saradc_device	*ndev)
{
	// disable clock
	if (ndev->clk_saradc) {
		pr_debug("cvi_saradc disable clock\n");
		clk_disable_unprepare(ndev->clk_saradc);
	}
}

static irqreturn_t cvi_saradc_irq(int irq, void	*data)
{
	struct cvi_saradc_device *ndev = data;
	unsigned long flags	= 0;

	spin_lock_irqsave(&ndev->close_lock, flags);

	// clear irq
	writel(0x1,	ndev->saradc_vaddr + SARADC_INTR_CLR);

	spin_unlock_irqrestore(&ndev->close_lock, flags);

	return IRQ_HANDLED;
}

static void	set_saradc_addr(struct cvi_saradc_device *ndev,	int	index)
{
	if (index >	ADC3)
		ndev->saradc_vaddr = ndev->rtcsys_saradc_base_addr;
	else
		ndev->saradc_vaddr = ndev->top_saradc_base_addr;
	return;
}

static int get_saradc_idx(int chan)
{
	if (chan > ADC3)
		return chan	- ADC3;
	return chan;
}

static void	cvi_saradc_cyc_setting(struct cvi_saradc_device	*ndev)
{
	uint32_t value;

	value =	readl(ndev->saradc_vaddr + SARADC_CYC_SET);
	value &= ~(0xf << 12);
	value |= (0xf << 12); // set saradc	clock cycle=840ns
	writel(value, ndev->saradc_vaddr + SARADC_CYC_SET);
}

static int saradc_read_raw(struct iio_dev *indio_dev,
						   struct iio_chan_spec	const *chan, int *val, int *val2, long info)
{
	struct cvi_saradc_device *ndev = iio_priv(indio_dev);
	uint32_t value;
	uint32_t adc_value;
	unsigned long flags	= 0;

	// platform_saradc_clk_init(ndev);
	if (!ndev->enable[chan->channel]) {
		pr_notice("Please echo 1 to	channel	to enable it\n");
		return -1;
	}

	spin_lock_irqsave(&ndev->close_lock, flags);

	set_saradc_addr(ndev, chan->channel);

	pr_debug("saradc addr:0x%p\n", ndev->saradc_vaddr);

	// Disable saradc interrupt
	writel(0x0,	ndev->saradc_vaddr + SARADC_INTR_EN);

	pr_debug("adc_channel_index: %d\n",	chan->channel);

	// Trigger measurement
	value =	readl(ndev->saradc_vaddr + SARADC_CTRL);
	value |= 1;
	writel(value, ndev->saradc_vaddr + SARADC_CTRL);
	pr_debug("cv_saradc_show: SARADC_CTRL =	%#X\n",	value);

	// Check busy status
	while (readl(ndev->saradc_vaddr	+ SARADC_STATUS) & 0x1)
		;

	adc_value =	readl(ndev->saradc_vaddr + chan->address) &	0xFFF;

	pr_debug("cvi_saradc channel%d value = %#X\n", chan->channel, adc_value);

	spin_unlock_irqrestore(&ndev->close_lock, flags);

	// platform_saradc_clk_deinit(ndev);

	*val = adc_value;
	return IIO_VAL_INT;
}

static int saradc_write_raw(struct iio_dev *indio_dev,
							struct iio_chan_spec const *chan, int val, int val2, long info)
{
	struct cvi_saradc_device *ndev = iio_priv(indio_dev);
	uint32_t value;
	unsigned long flags	= 0;
	int	index;
	uint32_t sel = 0;

	// set pinmux and pd register
	io_config(chan->channel);

	spin_lock_irqsave(&ndev->close_lock, flags);
	ndev->enable[chan->channel]	= val;

	// Set saradc_vaddr
	set_saradc_addr(ndev, chan->channel);
	index =	get_saradc_idx(chan->channel);

	sel	= readl(ndev->saradc_vaddr + SARADC_CTRL);
	// enable channel
	if (val) {
		// Set saradc cycle
		cvi_saradc_cyc_setting(ndev);
		writel(sel | (1	<< (SARADC_SEL_SHIFT + index)),	ndev->saradc_vaddr + SARADC_CTRL);
	} else {
		writel(sel & ~(1 <<	(SARADC_SEL_SHIFT +	index)), ndev->saradc_vaddr	+ SARADC_CTRL);
	}

	value =	readl(ndev->saradc_vaddr + SARADC_CTRL);
	spin_unlock_irqrestore(&ndev->close_lock, flags);

	pr_debug("cv_saradc_store: SARADC_CTRL = %#X\n", value);
	pr_debug("cvi_saradc %s	channel%d\n", val ?	"enable" : "disable", chan->channel);

	return 0;
}

static const struct	iio_info saradc_info = {
	.read_raw =	saradc_read_raw,
	.write_raw = saradc_write_raw,
};

static int cvi_saradc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cvi_saradc_device *ndev;
	struct resource	*res;
	struct iio_dev *indio_dev;
	int	ret, i;

	pr_debug("cvi_saradc_probe start\n");

	indio_dev =	devm_iio_device_alloc(dev, sizeof(*ndev));
	if (!indio_dev)
		return -ENOMEM;

	ndev = iio_priv(indio_dev);
	ndev->dev =	dev;
	ndev->private_data = pdev;

	ndev->saradc_vaddr = NULL;
	memset(ndev->enable, 0,	SARADC_CHAN_NUM);

	platform_set_drvdata(pdev, indio_dev);

	res	= platform_get_resource_byname(pdev, IORESOURCE_MEM, "top_domain_saradc");
	if (res	== NULL) {
		dev_err(dev, "failed to	retrieve saradc	io\n");
		return -ENXIO;
	}

	ndev->top_saradc_base_addr = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ndev->top_saradc_base_addr))
		return PTR_ERR(ndev->top_saradc_base_addr);

	res	= platform_get_resource_byname(pdev, IORESOURCE_MEM, "rtc_domain_saradc");
	if (res	== NULL) {
		dev_err(dev, "failed to	retrieve saradc	io\n");
		return -ENXIO;
	}

	ndev->rtcsys_saradc_base_addr =	devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ndev->rtcsys_saradc_base_addr))
		return PTR_ERR(ndev->rtcsys_saradc_base_addr);

	indio_dev->name	= "cvi_saradc";
	indio_dev->info	= &saradc_info;
	indio_dev->dev.parent =	dev;
	indio_dev->modes = INDIO_DIRECT_MODE;
	for	(i = 1;	i <= SARADC_CHAN_NUM; ++i) {
		if (i >	ADC3) {
			SARADC_CHAN_VOLTAGE(ndev->iio_channels[i - 1], i, SARADC_CH1_RESULT	+ (i - 1 - 3) *	4);
		} else {
			SARADC_CHAN_VOLTAGE(ndev->iio_channels[i - 1], i, SARADC_CH1_RESULT	+ (i - 1) *	4);
		}
	}
	indio_dev->channels	= ndev->iio_channels;
	indio_dev->num_channels	= SARADC_CHAN_NUM;

	ndev->saradc_irq = platform_get_irq(pdev, 0);
	if (ndev->saradc_irq < 0) {
		dev_err(dev, "failed to	retrieve saradc	irq");
		return -ENXIO;
	}

	ret	= devm_request_irq(&pdev->dev, ndev->saradc_irq, cvi_saradc_irq, 0,
						   "cvi-saradc", ndev);
	if (ret)
		return -ENXIO;

	ndev->clk_saradc = devm_clk_get(&pdev->dev,	"clk_saradc");
	if (IS_ERR(ndev->clk_saradc)) {
		dev_err(dev, "failed to	retrieve clk_saradc\n");
		ndev->clk_saradc = NULL;
	}

	platform_saradc_clk_init(ndev);

	ndev->rst_saradc = devm_reset_control_get(&pdev->dev, "res_saradc");
	if (IS_ERR(ndev->rst_saradc)) {
		dev_err(dev, "failed to	retrieve res_saradc\n");
		ndev->rst_saradc = NULL;
	}

	spin_lock_init(&ndev->close_lock);

	ret	= iio_device_register(indio_dev);
	if (ret) {
		dev_err(dev, "failed to	register iio device: %d\n",	ret);
		return ret;
	}

	// platform_set_drvdata(pdev, ndev);
	pr_debug("cvi_saradc_probe end\n");
	return 0;
}

static int cvi_saradc_remove(struct	platform_device	*pdev)
{
	struct iio_dev *indio_dev =	platform_get_drvdata(pdev);
	struct cvi_saradc_device *ndev = iio_priv(indio_dev);
	iio_device_unregister(indio_dev);
	platform_saradc_clk_deinit(ndev);

	pr_debug("cvi_saradc_remove\n");
	return 0;
}

static const struct	of_device_id cvi_saradc_match[]	= {
	{.compatible = "cvitek,saradc"},
	{},
};
MODULE_DEVICE_TABLE(of,	cvi_saradc_match);

#ifdef CONFIG_PM_SLEEP
static int saradc_cv_suspend(struct	device *dev)
{
	struct cvi_saradc_device *ndev = iio_priv(dev_get_drvdata(dev));
	memcpy_fromio(ndev->saradc_saved_top_regs, ndev->top_saradc_base_addr, SARADC_REGS_SIZE);
	memcpy_fromio(ndev->saradc_saved_rtc_regs, ndev->rtcsys_saradc_base_addr, SARADC_REGS_SIZE);
	platform_saradc_clk_deinit(ndev);

	return 0;
}

static int saradc_cv_resume(struct device *dev)
{
	struct cvi_saradc_device *ndev = iio_priv(dev_get_drvdata(dev));

	platform_saradc_clk_init(ndev);

	memcpy_toio(ndev->top_saradc_base_addr,	ndev->saradc_saved_top_regs, SARADC_REGS_SIZE);
	memcpy_toio(ndev->rtcsys_saradc_base_addr, ndev->saradc_saved_rtc_regs, SARADC_REGS_SIZE);


	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(saradc_cv_pm_ops, saradc_cv_suspend,
						 saradc_cv_resume);

static struct platform_driver cvi_saradc_driver	= {
	.probe = cvi_saradc_probe,
	.remove	= cvi_saradc_remove,
	.driver	= {
		.owner = THIS_MODULE,
		.name =	"cvi-saradc",
		.pm	= &saradc_cv_pm_ops,
		.of_match_table	= cvi_saradc_match,
	},
};
module_platform_driver(cvi_saradc_driver);

MODULE_AUTHOR("zixun.li <zixun.li@sophgo.com>");
MODULE_DESCRIPTION("Cvitek SoC saradc driver");
MODULE_LICENSE("GPL");
