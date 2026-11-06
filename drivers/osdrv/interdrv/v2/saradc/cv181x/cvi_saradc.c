/*
 * Cvitek SoCs saradc driver
 *
 * Copyright (c) 2018 Cvitek Ltd.
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

#include "cvi_saradc.h"
#include "cvi_saradc_ioctl.h"

#define CVI_SARADC_CDEV_NAME "cvi-saradc"
#define CVI_SARADC_CLASS_NAME "cvi-saradc"

struct class *saradc_class;
static dev_t saradc_cdev_id;

static char flag = 'n';

#define PINMUX_BASE 0x03001000
#define PINMUX_MASK(PIN_NAME) fmux_gpio_funcsel_##PIN_NAME##_MASK
#define PINMUX_OFFSET(PIN_NAME) fmux_gpio_funcsel_##PIN_NAME##_OFFSET
#define PINMUX_VALUE(PIN_NAME, FUNC_NAME) PIN_NAME##__##FUNC_NAME
#define PINMUX_CONFIG(PIN_NAME, FUNC_NAME) \
		mmio_clrsetbits_32(PINMUX_BASE + fmux_gpio_funcsel_##PIN_NAME, \
		PINMUX_MASK(PIN_NAME) << PINMUX_OFFSET(PIN_NAME), \
		PINMUX_VALUE(PIN_NAME, FUNC_NAME))

#define ADC1__XGPIOB_3 3 // adc1
#define ADC2__XGPIOB_2 3 // adc2
#define ADC3__XGPIOB_1 3 // adc3
#define PWR_GPIO2__PWR_GPIO_2 0 // adc4
#define PWR_GPIO1__PWR_GPIO_1 0 // adc5

#define IOBLK_G1_REG_ADC1		0x03001810
#define IOBLK_G1_REG_ADC2		0x0300180C
#define IOBLK_G1_REG_ADC3		0x03001808
#define IOBLK_GRTC_REG_PWR_GPIO1	0x05027030
#define IOBLK_GRTC_REG_PWR_GPIO2	0x05027034

#define  fmux_gpio_funcsel_ADC3   0xf0
#define  fmux_gpio_funcsel_ADC3_OFFSET 0
#define  fmux_gpio_funcsel_ADC3_MASK   0x7
#define  fmux_gpio_funcsel_ADC2   0xf4
#define  fmux_gpio_funcsel_ADC2_OFFSET 0
#define  fmux_gpio_funcsel_ADC2_MASK   0x7
#define  fmux_gpio_funcsel_ADC1   0xf8
#define  fmux_gpio_funcsel_ADC1_OFFSET 0
#define  fmux_gpio_funcsel_ADC1_MASK   0x7
#define  fmux_gpio_funcsel_PWR_GPIO1   0xa8
#define  fmux_gpio_funcsel_PWR_GPIO1_OFFSET 0
#define  fmux_gpio_funcsel_PWR_GPIO1_MASK   0x7
#define  fmux_gpio_funcsel_PWR_GPIO2   0xac
#define  fmux_gpio_funcsel_PWR_GPIO2_OFFSET 0
#define  fmux_gpio_funcsel_PWR_GPIO2_MASK   0x7

enum ADCChannel {
	/* Top domain ADC ch1, ch2, ch3 */
	ADC1 = 1,
	ADC2,
	ADC3,
	/* no die domain ADC ch1, ch2, ch3 */
	PWR_ADC1,/* ADC4 <== PWR_GPIO2 */
	PWR_ADC2,/* ADC5 <== PWR_GPIO1 */
	PWR_ADC3,/* ADC6 <== PWR_VBAT_DET */
};

static inline void mmio_clrsetbits_32(uintptr_t addr,
				      uint32_t clear,
				      uint32_t set)
{
	void *vaddr = ioremap(addr, 0x4);

	iowrite32((ioread32(vaddr) & ~clear) | set, vaddr);
	iounmap(vaddr);
}

static void io_config(uint32_t channel)
{
	void *vaddr = NULL;

	switch (channel) {
	case ADC1:
		PINMUX_CONFIG(ADC1, XGPIOB_3);
		vaddr = ioremap(IOBLK_G1_REG_ADC1, 0x4);
		iowrite32(0, vaddr);
		iounmap(vaddr);
		break;
	case ADC2:
		PINMUX_CONFIG(ADC2, XGPIOB_2);
		vaddr = ioremap(IOBLK_G1_REG_ADC2, 0x4);
		iowrite32(0, vaddr);
		iounmap(vaddr);
		break;
	case ADC3:
		PINMUX_CONFIG(ADC3, XGPIOB_1);
		vaddr = ioremap(IOBLK_G1_REG_ADC3, 0x4);
		iowrite32(0, vaddr);
		iounmap(vaddr);
		break;
	case PWR_ADC1:
		PINMUX_CONFIG(PWR_GPIO2, PWR_GPIO_2);
		vaddr = ioremap(IOBLK_GRTC_REG_PWR_GPIO2, 0x4);
		iowrite32(0, vaddr);
		iounmap(vaddr);
		break;
	case PWR_ADC2:
		PINMUX_CONFIG(PWR_GPIO1, PWR_GPIO_1);
		vaddr = ioremap(IOBLK_GRTC_REG_PWR_GPIO1, 0x4);
		iowrite32(0, vaddr);
		iounmap(vaddr);
		break;
	case PWR_ADC3:
		break;
	default:
		pr_err("io_config: invalid channel index\n");
		break;
	}
}

static void set_saradc_vaddr(struct cvi_saradc_device *ndev)
{
	if (ndev->channel_index > ADC3) {
		ndev->saradc_vaddr = ndev->rtcsys_saradc_base_addr;
		ndev->channel_index = ndev->channel_index - 3;
	} else {
		ndev->saradc_vaddr = ndev->top_saradc_base_addr;
	}
}

static int platform_saradc_clk_init(struct cvi_saradc_device *ndev)
{
	//enable clock
	if (ndev->clk_saradc) {
		pr_debug("cvi_saradc enable clock\n");
		clk_prepare_enable(ndev->clk_saradc);
	}

	return 0;
}

static void platform_saradc_clk_deinit(struct cvi_saradc_device *ndev)
{
	//disable clock
	if (ndev->clk_saradc) {
		pr_debug("cvi_saradc disable clock\n");
		clk_disable_unprepare(ndev->clk_saradc);
	}
}

static irqreturn_t cvi_saradc_irq(int irq, void *data)
{
	struct cvi_saradc_device *ndev = data;
	unsigned long flags = 0;

	spin_lock_irqsave(&ndev->close_lock, flags);

	flag = 'y';
	//clear irq
	writel(0x1, ndev->saradc_vaddr + SARADC_INTR_CLR);

	spin_unlock_irqrestore(&ndev->close_lock, flags);

	return IRQ_HANDLED;
}

static void cvi_saradc_cyc_setting(struct cvi_saradc_device *ndev)
{
	uint32_t value;

	value = readl(ndev->saradc_vaddr + SARADC_CYC_SET);
	value &= ~(0xf << 12);
	value |= (0xf << 12);//set saradc clock cycle=840ns
	writel(value, ndev->saradc_vaddr + SARADC_CYC_SET);
}

ssize_t cvi_saradc_read(struct file *filp, char *buff, size_t count, loff_t *offp)
{
	struct cvi_saradc_device *ndev = filp->private_data;
	uint32_t value;
	uint32_t adc_value;
	unsigned long flags = 0;

	if (!ndev->saradc_vaddr) {
		pr_err("Please write channel before read value\n");
		return -1;
	}

	spin_lock_irqsave(&ndev->close_lock, flags);

	value = readl(ndev->saradc_vaddr + SARADC_CYC_SET);
	pr_debug("cvi_saradc_read: %#X\n", value);
	pr_debug("channel_index: %d\n", ndev->channel_index);

	// Trigger measurement
	value = readl(ndev->saradc_vaddr + SARADC_CTRL);
	value |= 1;
	writel(value, ndev->saradc_vaddr + SARADC_CTRL);

	pr_debug("cvi_saradc_read: SARADC_CTRL = %#X\n", value);

	// Check busy status
	while (readl(ndev->saradc_vaddr + SARADC_STATUS) & 0x1)
		;

	adc_value = readl(ndev->saradc_vaddr + SARADC_CH1_RESULT + (ndev->channel_index - 1) * 4) & 0xFFF;
	pr_debug("cvi_saradc channel%d value = %#X\n", ndev->channel_index, adc_value);

	spin_unlock_irqrestore(&ndev->close_lock, flags);

	return 0;
}

ssize_t cvi_saradc_write(struct file *filp, const char *buff, size_t count, loff_t *offp)
{
	struct cvi_saradc_device *ndev = filp->private_data;
	uint32_t value;
	unsigned long flags = 0;

	if (copy_from_user(&flag, buff, 1))
		return -EFAULT;

	ndev->channel_index = flag - 0x30;

	// set pinmux and pd register
	io_config(ndev->channel_index);

	spin_lock_irqsave(&ndev->close_lock, flags);

	if (ndev->channel_index < ADC1 ||
			ndev->channel_index > PWR_ADC3) {
		pr_err("Error adc channel %d, valid input is 1, 2, 3, 4, 5 or 6\n", ndev->channel_index);
		ndev->channel_index = 0;
	} else {
		// Set saradc_vaddr
		set_saradc_vaddr(ndev);

		// Set saradc cycle
		cvi_saradc_cyc_setting(ndev);

		// Disable saradc interrupt
		writel(0x0, ndev->saradc_vaddr + SARADC_INTR_EN);

		// Set channel
		writel(ndev->channel_index << (SARADC_SEL_SHIFT + 1), ndev->saradc_vaddr + SARADC_CTRL);
		value = readl(ndev->saradc_vaddr + SARADC_CTRL);
		pr_debug("cvi_saradc_write: SARADC_CTRL = %#X\n", value);
	}

	pr_debug("cvi_saradc_write: %d\n", ndev->channel_index);

	spin_unlock_irqrestore(&ndev->close_lock, flags);

	return count;
}

static int cvi_saradc_open(struct inode *inode, struct file *filp)
{
	struct cvi_saradc_device *ndev =
		container_of(inode->i_cdev, struct cvi_saradc_device, cdev);
	unsigned long flags = 0;

	platform_saradc_clk_init(ndev);

	spin_lock_irqsave(&ndev->close_lock, flags);

	ndev->use_count++;

	spin_unlock_irqrestore(&ndev->close_lock, flags);

	filp->private_data = ndev;

	pr_debug("cvi_saradc_open\n");

	return 0;
}

static int cvi_saradc_close(struct inode *inode, struct file *filp)
{
	unsigned long flags = 0;
	struct cvi_saradc_device *ndev =
		container_of(inode->i_cdev, struct cvi_saradc_device, cdev);

	platform_saradc_clk_deinit(ndev);

	spin_lock_irqsave(&ndev->close_lock, flags);

	ndev->use_count--;

	spin_unlock_irqrestore(&ndev->close_lock, flags);

	filp->private_data = NULL;

	pr_debug("cvi_saradc_close\n");
	return 0;
}

static const struct file_operations saradc_fops = {
	.owner = THIS_MODULE,
	.open = cvi_saradc_open,
	.release = cvi_saradc_close,
	.read = cvi_saradc_read,
	.write = cvi_saradc_write,
};

static ssize_t cv_saradc_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct cvi_saradc_device *ndev = dev->platform_data;
	uint32_t value;
	uint32_t adc_value;
	unsigned long flags = 0;

	if (!ndev->saradc_vaddr) {
		pr_err("Please echo channel before cat value\n");
		return -1;
	}

	platform_saradc_clk_init(ndev);

	spin_lock_irqsave(&ndev->close_lock, flags);

	// Disable saradc interrupt
	writel(0x0, ndev->saradc_vaddr + SARADC_INTR_EN);

	pr_debug("adc_channel_index: %d\n", ndev->channel_index);

	// Trigger measurement
	value = readl(ndev->saradc_vaddr + SARADC_CTRL);
	value |= 1;
	writel(value, ndev->saradc_vaddr + SARADC_CTRL);
	pr_debug("cv_saradc_show: SARADC_CTRL = %#X\n", value);

	// Check busy status
	while (readl(ndev->saradc_vaddr + SARADC_STATUS) & 0x1)
		;

	adc_value = readl(ndev->saradc_vaddr + SARADC_CH1_RESULT + (ndev->channel_index - 1) * 4) & 0xFFF;

	pr_debug("cvi_saradc channel%d value = %#X\n", ndev->channel_index, adc_value);

	spin_unlock_irqrestore(&ndev->close_lock, flags);

	platform_saradc_clk_deinit(ndev);

	return scnprintf(buf, PAGE_SIZE, "%04u\n", adc_value);
}

static ssize_t cv_saradc_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct cvi_saradc_device *ndev = dev->platform_data;
	uint32_t value;
	unsigned long flags = 0;

	ndev->channel_index = buf[0] - 0x30;

	platform_saradc_clk_init(ndev);

	// set pinmux and pd register
	io_config(ndev->channel_index);

	spin_lock_irqsave(&ndev->close_lock, flags);

	if (ndev->channel_index < ADC1 ||
			ndev->channel_index > PWR_ADC3) {
		pr_err("Error adc channel %d, valid input is 1, 2, 3, 4, 5 or 6\n", ndev->channel_index);
		ndev->channel_index = 0;
	} else {
		// Set saradc_vaddr
		set_saradc_vaddr(ndev);
		// Set saradc cycle
		cvi_saradc_cyc_setting(ndev);
		// Set channel
		writel(1 << (SARADC_SEL_SHIFT + ndev->channel_index), ndev->saradc_vaddr + SARADC_CTRL);
		value = readl(ndev->saradc_vaddr + SARADC_CTRL);
		pr_debug("cv_saradc_store: SARADC_CTRL = %#X\n", value);
	}

	pr_debug("cvi_saradc set channel%d\n", ndev->channel_index);

	spin_unlock_irqrestore(&ndev->close_lock, flags);

	platform_saradc_clk_deinit(ndev);

	return count;
}

DEVICE_ATTR_RW(cv_saradc);

static struct attribute *tee_dev_attrs[] = {
	&dev_attr_cv_saradc.attr,
	NULL
};

static const struct attribute_group tee_dev_group = {
	.attrs = tee_dev_attrs,
};

int cvi_saradc_register_cdev(struct cvi_saradc_device *ndev)
{
	int ret;
	int rc;

	saradc_class = class_create(THIS_MODULE, CVI_SARADC_CLASS_NAME);
	if (IS_ERR(saradc_class)) {
		pr_err("create class failed\n");
		return PTR_ERR(saradc_class);
	}

	ret = alloc_chrdev_region(&saradc_cdev_id, 0, 1, CVI_SARADC_CDEV_NAME);
	if (ret < 0) {
		pr_err("alloc chrdev failed\n");
		return ret;
	}

	cdev_init(&ndev->cdev, &saradc_fops);
	ndev->cdev.owner = THIS_MODULE;
	cdev_add(&ndev->cdev, saradc_cdev_id, 1);

	device_create(saradc_class, ndev->dev, saradc_cdev_id, NULL, "%s%d",
		      CVI_SARADC_CDEV_NAME, 0);

	rc = sysfs_create_group(&ndev->dev->kobj, &tee_dev_group);
	if (rc) {
		dev_err(ndev->dev,
			"failed to create sysfs attributes, err=%d\n", rc);
		return rc;
	}

	return 0;
}

static int cvi_saradc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cvi_saradc_device *ndev;
	struct resource *res;
	int ret;

	pr_debug("cvi_saradc_probe start\n");

	ndev = devm_kzalloc(&pdev->dev, sizeof(*ndev), GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;
	ndev->dev = dev;

	ndev->dev->platform_data = ndev; // test code
	ndev->saradc_vaddr = NULL;

	//res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "saradc");
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "top_domain_saradc");
	if (res == NULL) {
		dev_err(dev, "failed to retrieve saradc io\n");
		return -ENXIO;
	}

	ndev->top_saradc_base_addr = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ndev->top_saradc_base_addr))
		return PTR_ERR(ndev->top_saradc_base_addr);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "rtc_domain_saradc");
	if (res == NULL) {
		dev_err(dev, "failed to retrieve saradc io\n");
		return -ENXIO;
	}

	ndev->rtcsys_saradc_base_addr = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ndev->rtcsys_saradc_base_addr))
		return PTR_ERR(ndev->rtcsys_saradc_base_addr);

	ndev->saradc_irq = platform_get_irq(pdev, 0);
	if (ndev->saradc_irq < 0) {
		dev_err(dev, "failed to retrieve saradc irq");
		return -ENXIO;
	}

	ret = devm_request_irq(&pdev->dev, ndev->saradc_irq, cvi_saradc_irq, 0,
							"cvi-saradc", ndev);
	if (ret)
		return -ENXIO;

	ndev->clk_saradc = devm_clk_get(&pdev->dev, "clk_saradc");
	if (IS_ERR(ndev->clk_saradc)) {
		dev_err(dev, "failed to retrieve clk_saradc\n");
		ndev->clk_saradc = NULL;
	}

	ndev->rst_saradc = devm_reset_control_get(&pdev->dev, "res_saradc");
	if (IS_ERR(ndev->rst_saradc)) {
		dev_err(dev, "failed to retrieve res_saradc\n");
		ndev->rst_saradc = NULL;
	}

	spin_lock_init(&ndev->close_lock);
	ndev->use_count = 0;

	ret = cvi_saradc_register_cdev(ndev);
	if (ret < 0) {
		dev_err(dev, "regsiter chrdev error\n");
		return ret;
	}

	platform_set_drvdata(pdev, ndev);
	pr_debug("cvi_saradc_probe end\n");
	return 0;
}

static int cvi_saradc_remove(struct platform_device *pdev)
{
	struct cvi_saradc_device *ndev = platform_get_drvdata(pdev);

	device_destroy(saradc_class, saradc_cdev_id);

	cdev_del(&ndev->cdev);

	unregister_chrdev_region(saradc_cdev_id, 1);

	class_destroy(saradc_class);

	platform_set_drvdata(pdev, NULL);

	sysfs_remove_group(&ndev->dev->kobj, &tee_dev_group);

	pr_debug("cvi_saradc_remove\n");

	return 0;
}

static const struct of_device_id cvi_saradc_match[] = {
	{ .compatible = "cvitek,saradc" },
    {},
};
MODULE_DEVICE_TABLE(of, cvi_saradc_match);

static struct platform_driver cvi_saradc_driver = {
    .probe = cvi_saradc_probe,
    .remove = cvi_saradc_remove,
	.driver = {
            .owner = THIS_MODULE,
            .name = "cvi-saradc",
            .of_match_table = cvi_saradc_match,
        },
};
module_platform_driver(cvi_saradc_driver);

MODULE_AUTHOR("Mark Hsieh<mark.hsieh@wisecore.com.tw>");
MODULE_DESCRIPTION("Cvitek SoC saradc driver");
MODULE_LICENSE("GPL");
