/*
 * ky-bt.c -- power on/off bt part of SoC
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
#include <linux/clk.h>
#include <linux/rfkill.h>
#include <linux/property.h>
#include <linux/of_device.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include "ky-pwrseq.h"

struct bt_pwrseq {
	struct device		*dev;
	struct ky_pwrseq *parent;
	bool power_state;
	u32 power_on_delay_ms;

	struct gpio_desc *reset_n;
	struct clk *ext_clk;
	struct rfkill *rfkill;
};

static int ky_bt_on(struct bt_pwrseq *pwrseq, bool on_off)
{
	if (!pwrseq || IS_ERR(pwrseq->reset_n))
		return 0;

	if (on_off){
		if(pwrseq->parent)
			ky_power_on(pwrseq->parent, 1);
		gpiod_set_value_cansleep(pwrseq->reset_n, 1);
		if (pwrseq->power_on_delay_ms)
			msleep(pwrseq->power_on_delay_ms);
	}else{
		gpiod_set_value_cansleep(pwrseq->reset_n, 0);
		if(pwrseq->parent)
			ky_power_on(pwrseq->parent, 0);
	}

	pwrseq->power_state = on_off;
	return 0;
}

static int ky_bt_set_block(void *data, bool blocked)
{
	struct bt_pwrseq *pwrseq = data;
	int ret;

	if (blocked != pwrseq->power_state) {
		dev_warn(pwrseq->dev, "block state already is %d\n", blocked);
		return 0;
	}

	dev_info(pwrseq->dev, "set block: %d\n", blocked);
	ret = ky_bt_on(pwrseq, !blocked);
	if (ret) {
		dev_err(pwrseq->dev, "set block failed\n");
		return ret;
	}

	return 0;
}

static const struct rfkill_ops ky_bt_rfkill_ops = {
	.set_block = ky_bt_set_block,
};

static int ky_bt_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct bt_pwrseq *pwrseq;
	int ret;

	pwrseq = devm_kzalloc(dev, sizeof(*pwrseq), GFP_KERNEL);
	if (!pwrseq)
		return -ENOMEM;

	pwrseq->dev = dev;
	pwrseq->parent = ky_get_pwrseq_from_dev(pwrseq->dev);
	platform_set_drvdata(pdev, pwrseq);

	pwrseq->reset_n = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(pwrseq->reset_n) &&
		PTR_ERR(pwrseq->reset_n) != -ENOENT &&
		PTR_ERR(pwrseq->reset_n) != -ENOSYS) {
		return PTR_ERR(pwrseq->reset_n);
	}

	pwrseq->ext_clk = devm_clk_get(dev, "clock");
	if (IS_ERR_OR_NULL(pwrseq->ext_clk)){
		dev_dbg(dev, "failed get ext clock\n");
	} else {
		ret = clk_prepare_enable(pwrseq->ext_clk);
		if (ret < 0)
			dev_warn(dev, "can't enable clk\n");
	}

	if(device_property_read_u32(dev, "power-on-delay-ms",
				 &pwrseq->power_on_delay_ms))
		pwrseq->power_on_delay_ms = 10;

	pwrseq->rfkill = rfkill_alloc("ky-bt", dev, RFKILL_TYPE_BLUETOOTH,
				    &ky_bt_rfkill_ops, pwrseq);
	if (!pwrseq->rfkill) {
		dev_err(dev, "failed alloc bt rfkill\n");
		ret = -ENOMEM;
		goto alloc_err;
	}

	rfkill_set_states(pwrseq->rfkill, true, false);

	ret = rfkill_register(pwrseq->rfkill);
	if (ret) {
		dev_err(dev, "failed register bt rfkill\n");
		goto register_err;
	}

	return 0;

register_err:
	if (pwrseq->rfkill)
		rfkill_destroy(pwrseq->rfkill);
alloc_err:
	if (!IS_ERR_OR_NULL(pwrseq->ext_clk))
		clk_disable_unprepare(pwrseq->ext_clk);
	return ret;
}

static int ky_bt_remove(struct platform_device *pdev)
{
	struct bt_pwrseq *pwrseq = platform_get_drvdata(pdev);

	if (pwrseq->rfkill) {
		rfkill_unregister(pwrseq->rfkill);
		rfkill_destroy(pwrseq->rfkill);
	}

	if (!IS_ERR_OR_NULL(pwrseq->ext_clk))
		clk_disable_unprepare(pwrseq->ext_clk);

	return 0;
}

static const struct of_device_id ky_bt_ids[] = {
	{ .compatible = "ky,bt-pwrseq" },
	{ /* Sentinel */ }
};

#ifdef CONFIG_PM_SLEEP
static int ky_bt_suspend(struct device *dev)
{
	return 0;
}

static int ky_bt_resume(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops ky_bt_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ky_bt_suspend, ky_bt_resume)
};

#define DEV_PM_OPS	(&ky_bt_dev_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif /* CONFIG_PM_SLEEP */

static struct platform_driver ky_bt_driver = {
	.probe		= ky_bt_probe,
	.remove	= ky_bt_remove,
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= "ky-bt",
		.of_match_table	= ky_bt_ids,
	},
};

module_platform_driver(ky_bt_driver);

MODULE_DESCRIPTION("ky bt pwrseq driver");
MODULE_LICENSE("GPL");
