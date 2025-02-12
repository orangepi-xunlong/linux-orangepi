/*
 * ky-wlan.c -- power on/off wlan part of SoC
 *
 * Copyright 2023, Ky Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
 
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/property.h>
#include <linux/of_device.h>
#include <linux/interrupt.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include "ky-pwrseq.h"

struct wlan_pwrseq {
	struct device		*dev;
	struct ky_pwrseq *parent;
	bool power_state;
	u32 power_on_delay_ms;

	struct gpio_desc *regon;
	struct gpio_desc *hostwake;
	int irq;

	struct mutex wlan_mutex;
};

static struct wlan_pwrseq *pdata = NULL;
static int ky_wlan_on(struct wlan_pwrseq *pwrseq, bool on_off);

void ky_wlan_set_power(bool on_off)
{
	struct wlan_pwrseq *pwrseq = pdata;
	int ret = 0;

	if (!pwrseq)
		return;

	mutex_lock(&pwrseq->wlan_mutex);
	if (on_off != pwrseq->power_state) {
		ret = ky_wlan_on(pwrseq, on_off);
		if (ret)
			dev_err(pwrseq->dev, "set power failed\n");
	}
	mutex_unlock(&pwrseq->wlan_mutex);
}
EXPORT_SYMBOL_GPL(ky_wlan_set_power);

int ky_wlan_get_oob_irq(void)
{
	struct wlan_pwrseq *pwrseq = pdata;

	if (!pwrseq)
		return 0;

	if (pwrseq->irq <= 0){
		dev_err(pwrseq->dev, "get oob irq failed\n");
		return 0;
	}
	return pwrseq->irq;
}
EXPORT_SYMBOL_GPL(ky_wlan_get_oob_irq);

int ky_wlan_get_oob_irq_flags(void)
{
	struct wlan_pwrseq *pwrseq = pdata;
	int oob_irq_flags;

	if (!pwrseq)
		return 0;

	oob_irq_flags = (IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND);

	return oob_irq_flags;
}
EXPORT_SYMBOL_GPL(ky_wlan_get_oob_irq_flags);

static int ky_wlan_on(struct wlan_pwrseq *pwrseq, bool on_off)
{
	if (!pwrseq || IS_ERR(pwrseq->regon))
		return 0;

	if (on_off){
		if(pwrseq->parent)
			ky_power_on(pwrseq->parent, 1);
		gpiod_set_value_cansleep(pwrseq->regon, 1);
		if (pwrseq->power_on_delay_ms)
			msleep(pwrseq->power_on_delay_ms);
	}else{
		gpiod_set_value_cansleep(pwrseq->regon, 0);
		if(pwrseq->parent)
			ky_power_on(pwrseq->parent, 0);
	}

	pwrseq->power_state = on_off;
	return 0;
}

static void ky_get_gpio_irq(struct wlan_pwrseq *pwrseq)
{
	pwrseq->hostwake = devm_gpiod_get(pwrseq->dev, "hostwake", GPIOD_IN);
	if (IS_ERR_OR_NULL(pwrseq->hostwake)) {
		dev_err(pwrseq->dev, "no interrupt gpio property\n");
		return;
	}

	pwrseq->irq = gpiod_to_irq(pwrseq->hostwake);
	if (pwrseq->irq < 0)
		dev_err(pwrseq->dev, "failed to get GPIO IRQ\n");
}

static int ky_wlan_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct wlan_pwrseq *pwrseq;

	pwrseq = devm_kzalloc(dev, sizeof(*pwrseq), GFP_KERNEL);
	if (!pwrseq)
		return -ENOMEM;

	pwrseq->dev = dev;
	pwrseq->parent = ky_get_pwrseq_from_dev(dev);
	platform_set_drvdata(pdev, pwrseq);

	pwrseq->regon = devm_gpiod_get(dev, "regon", GPIOD_OUT_LOW);
	if (IS_ERR(pwrseq->regon) &&
		PTR_ERR(pwrseq->regon) != -ENOENT &&
		PTR_ERR(pwrseq->regon) != -ENOSYS) {
		return PTR_ERR(pwrseq->regon);
	}

	pwrseq->irq = platform_get_irq(pdev, 0);
	if (pwrseq->irq < 0) {
		dev_info(pwrseq->dev, "get platform irq failed, try to get gpio irq\n");
		ky_get_gpio_irq(pwrseq);
	}

	if (pwrseq->irq < 0)
		dev_err(pwrseq->dev, "get hostwake irq failed, ignore wow\n");

	if(device_property_read_u32(dev, "power-on-delay-ms",
				 &pwrseq->power_on_delay_ms))
		pwrseq->power_on_delay_ms = 10;

	mutex_init(&pwrseq->wlan_mutex);
	pdata = pwrseq;

	return 0;
}

static int ky_wlan_remove(struct platform_device *pdev)
{
	struct wlan_pwrseq *pwrseq = platform_get_drvdata(pdev);

	mutex_destroy(&pwrseq->wlan_mutex);
	pdata = NULL;

	return 0;
}

static const struct of_device_id ky_wlan_ids[] = {
	{ .compatible = "ky,wlan-pwrseq" },
	{ /* Sentinel */ }
};

#ifdef CONFIG_PM_SLEEP
static int ky_wlan_suspend(struct device *dev)
{
	return 0;
}

static int ky_wlan_resume(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops ky_wlan_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ky_wlan_suspend, ky_wlan_resume)
};

#define DEV_PM_OPS	(&ky_wlan_dev_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif /* CONFIG_PM_SLEEP */

static struct platform_driver ky_wlan_driver = {
	.probe		= ky_wlan_probe,
	.remove	= ky_wlan_remove,
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= "ky-wlan",
		.of_match_table	= ky_wlan_ids,
	},
};

module_platform_driver(ky_wlan_driver);

MODULE_DESCRIPTION("ky wlan pwrseq driver");
MODULE_LICENSE("GPL");
