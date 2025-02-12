/*
 * ky-rfkill.c -- power on/off rfkill part of SoC
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
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/of.h>
#include "ky-pwrseq.h"

struct rfkill_pwrseq {
	struct device *dev;
	struct ky_pwrseq *parent;

	bool unblock_at_boot;
	bool power_state;
	u32 power_on_delay_ms;

	bool power_use_pulse;
	u32 power_pulse_delay_ms;
	u32 power_off_pulse_delay_ms;

	bool reset_power_off;
	bool reset_use_pulse;
	u32 reset_pulse_delay_ms;

	struct gpio_desc *reset_n;
	struct gpio_desc *power_n;
	struct clk *ext_clk;
	struct rfkill *rfkill;
};

static void ky_rfkill_set_pin(struct gpio_desc *gpio, int value,
				    bool use_pulse, u32 pulse_delay)
{
	if (!gpio)
		return;
	if (use_pulse) {
		value = 1;
		gpiod_set_value_cansleep(gpio, value);
		msleep(pulse_delay);
		/* Pulse power key should restore to original level after pulse */
		value = !value;
	}
	gpiod_set_value_cansleep(gpio, value);
}

static int ky_rfkill_on(struct rfkill_pwrseq *pwrseq, bool on_off)
{
	if (!pwrseq || IS_ERR(pwrseq->reset_n))
		return 0;

	if (on_off) {
		if (pwrseq->parent)
			ky_power_on(pwrseq->parent, 1);
		ky_rfkill_set_pin(pwrseq->power_n, 1,
					pwrseq->power_use_pulse,
					pwrseq->power_pulse_delay_ms);
		if (pwrseq->power_on_delay_ms)
			msleep(pwrseq->power_on_delay_ms);
		ky_rfkill_set_pin(pwrseq->reset_n, 1,
					pwrseq->reset_use_pulse,
					pwrseq->reset_pulse_delay_ms);
	} else {
		if (pwrseq->reset_power_off)
			ky_rfkill_set_pin(pwrseq->reset_n, 0,
						pwrseq->reset_use_pulse,
						pwrseq->reset_pulse_delay_ms);
		ky_rfkill_set_pin(pwrseq->power_n, 0,
					pwrseq->power_use_pulse,
					pwrseq->power_off_pulse_delay_ms);
		if (pwrseq->parent)
			ky_power_on(pwrseq->parent, 0);
	}

	pwrseq->power_state = on_off;
	return 0;
}

static int ky_rfkill_set_block(void *data, bool blocked)
{
	struct rfkill_pwrseq *pwrseq = data;
	int ret;

	if (blocked != pwrseq->power_state) {
		dev_warn(pwrseq->dev, "block state already is %d\n", blocked);
		return 0;
	}

	dev_info(pwrseq->dev, "set block: %d\n", blocked);
	ret = ky_rfkill_on(pwrseq, !blocked);
	if (ret) {
		dev_err(pwrseq->dev, "set block failed\n");
		return ret;
	}

	return 0;
}

static const struct rfkill_ops ky_rfkill_ops = {
	.set_block = ky_rfkill_set_block,
};

static const char *const ky_rfkill_types[] = {
	[RFKILL_TYPE_ALL] = "ky,rfkill-pwrseq",
	[RFKILL_TYPE_WLAN] = "ky,wlan-pwrseq",
	[RFKILL_TYPE_BLUETOOTH] = "ky,bt-pwrseq",
	[RFKILL_TYPE_UWB] = "ky,uwb-pwrseq",
	[RFKILL_TYPE_WIMAX] = "ky,wimax-pwrseq",
	[RFKILL_TYPE_WWAN] = "ky,wwan-pwrseq",
	[RFKILL_TYPE_GPS] = "ky,gps-pwrseq",
	[RFKILL_TYPE_FM] = "ky,fm-pwrseq",
	[RFKILL_TYPE_NFC] = "ky,nfc-pwrseq"
};

static enum rfkill_type
ky_rfkill_determine_type(struct rfkill_pwrseq *pwrseq)
{
	struct device *dev = pwrseq->dev;
	const char *compatible;
	int ret;
	ret = of_property_read_string(dev->of_node, "compatible", &compatible);
	if (ret) {
		dev_warn(dev, "failed to get compatible\n");
		compatible = "ky,rfkill-pwrseq";
	}
	ret = match_string(ky_rfkill_types,
			   ARRAY_SIZE(ky_rfkill_types), compatible);
	return (ret < 0) ? RFKILL_TYPE_ALL : ret;
}

static int ky_rfkill_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rfkill_pwrseq *pwrseq;
	const char *device_name;
	enum rfkill_type rftype;
	int ret;

	pwrseq = devm_kzalloc(dev, sizeof(*pwrseq), GFP_KERNEL);
	if (!pwrseq)
		return -ENOMEM;

	pwrseq->dev = dev;
	pwrseq->parent = ky_get_pwrseq_from_dev(dev);
	platform_set_drvdata(pdev, pwrseq);

	pwrseq->reset_n = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(pwrseq->reset_n) && PTR_ERR(pwrseq->reset_n) != -ENOENT &&
	    PTR_ERR(pwrseq->reset_n) != -ENOSYS) {
		return PTR_ERR(pwrseq->reset_n);
	}

	pwrseq->power_n = devm_gpiod_get_optional(dev, "power", GPIOD_OUT_LOW);
	if (IS_ERR(pwrseq->power_n) && PTR_ERR(pwrseq->power_n) != -ENOENT &&
	    PTR_ERR(pwrseq->power_n) != -ENOSYS) {
		return PTR_ERR(pwrseq->power_n);
	}

	pwrseq->ext_clk = devm_clk_get(dev, "clock");
	if (IS_ERR_OR_NULL(pwrseq->ext_clk)) {
		dev_dbg(dev, "failed get ext clock\n");
	} else {
		ret = clk_prepare_enable(pwrseq->ext_clk);
		if (ret < 0)
			dev_warn(dev, "can't enable clk\n");
	}

	if (device_property_read_u32(dev, "power-on-delay-ms",
				     &pwrseq->power_on_delay_ms))
		pwrseq->power_on_delay_ms = 10;

	if (device_property_read_u32(dev, "power-pulse-delay-ms",
				     &pwrseq->power_pulse_delay_ms))
		pwrseq->power_pulse_delay_ms = 10;

	if (device_property_read_u32(dev, "power-off-pulse-delay-ms",
				     &pwrseq->power_off_pulse_delay_ms))
		pwrseq->power_off_pulse_delay_ms = pwrseq->power_pulse_delay_ms;

	/* Some rf modules reset cause power on, thus not reset on power-off sequence by default */
	pwrseq->reset_power_off =
		device_property_read_bool(dev, "reset-power-off");

	pwrseq->power_use_pulse =
		device_property_read_bool(dev, "power-use-pulse");
	if (pwrseq->power_use_pulse) {
		dev_info(dev, "power use pulse, delay: %dms/%dms\n",
			 pwrseq->power_pulse_delay_ms,
			 pwrseq->power_off_pulse_delay_ms);
	}

	if (device_property_read_u32(dev, "reset-pulse-delay-ms",
				     &pwrseq->reset_pulse_delay_ms))
		pwrseq->reset_pulse_delay_ms = 10;

	pwrseq->reset_use_pulse =
		device_property_read_bool(dev, "reset-use-pulse");
	if (pwrseq->reset_use_pulse) {
		dev_info(dev, "reset use pulse, delay: %dms\n",
			 pwrseq->reset_pulse_delay_ms);
	}

	rftype = ky_rfkill_determine_type(pwrseq);

	ret = device_property_read_string(dev, "rfkill-name", &device_name);
	if (ret) {
		dev_warn(dev, "no rfkill name specificed, use default one\n");
		device_name = ky_rfkill_types[rftype];
	}

	pwrseq->rfkill = rfkill_alloc(device_name, dev, rftype,
				      &ky_rfkill_ops, pwrseq);
	if (!pwrseq->rfkill) {
		dev_err(dev, "failed alloc rfkill device\n");
		ret = -ENOMEM;
		goto alloc_err;
	}

	rfkill_set_states(pwrseq->rfkill, true, false);

	ret = rfkill_register(pwrseq->rfkill);
	if (ret) {
		dev_err(dev, "failed register %s rfkill\n", device_name);
		goto register_err;
	}

	if (device_property_read_bool(dev, "rfkill-unblock-at-boot")) {
		ky_rfkill_on(pwrseq, true);
		rfkill_set_states(pwrseq->rfkill, false, false);
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

static int ky_rfkill_remove(struct platform_device *pdev)
{
	struct rfkill_pwrseq *pwrseq = platform_get_drvdata(pdev);

	if (pwrseq->rfkill) {
		rfkill_unregister(pwrseq->rfkill);
		rfkill_destroy(pwrseq->rfkill);
	}

	if (!IS_ERR_OR_NULL(pwrseq->ext_clk))
		clk_disable_unprepare(pwrseq->ext_clk);

	return 0;
}

static const struct of_device_id ky_rfkill_ids[] = {
	{ .compatible = "ky,rfkill-pwrseq" },
	/* { .compatible =  "ky,wlan-pwrseq" },
	 * { .compatible =  "ky,bt-pwrseq" }, */
	{ .compatible =  "ky,uwb-pwrseq" },
	{ .compatible = "ky,wwan-pwrseq" },
	{ .compatible = "ky,gps-pwrseq" },
	{ .compatible = "ky,fm-pwrseq" },
	{ .compatible = "ky,nfc-pwrseq" },
	{ /* Sentinel */ }
};

#ifdef CONFIG_PM_SLEEP
static int ky_rfkill_suspend(struct device *dev)
{
	return 0;
}

static int ky_rfkill_resume(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops ky_rfkill_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ky_rfkill_suspend, ky_rfkill_resume)
};

#define DEV_PM_OPS (&ky_rfkill_dev_pm_ops)
#else
#define DEV_PM_OPS NULL
#endif /* CONFIG_PM_SLEEP */

static struct platform_driver ky_rfkill_driver = {
	.probe		= ky_rfkill_probe,
	.remove	= ky_rfkill_remove,
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= "ky-rfkill",
		.of_match_table	= ky_rfkill_ids,
	},
};

module_platform_driver(ky_rfkill_driver);

MODULE_DESCRIPTION("ky rfkill pwrseq driver");
MODULE_LICENSE("GPL");
