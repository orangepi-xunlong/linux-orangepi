/*
 * ky-pwrseq.c -- power on/off pwrseq part of SoC
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
#include <linux/of_gpio.h>
#include <linux/clk.h>
#include <linux/of_platform.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include "ky-pwrseq.h"


struct ky_pwrseq *ky_get_pwrseq_from_dev(struct device *dev)
{
	if (dev && dev->parent && dev->parent->of_node &&
	    of_device_is_compatible(dev->parent->of_node, "ky,rf-pwrseq"))
		return platform_get_drvdata(to_platform_device(dev->parent));
	return NULL;
}

static void ky_set_gpios_value(struct ky_pwrseq *pwrseq,
						int value)
{
	struct gpio_descs *pwr_gpios = pwrseq->pwr_gpios;

	if (!IS_ERR(pwr_gpios)) {
		unsigned long *values;
		int nvalues = pwr_gpios->ndescs;

		values = bitmap_alloc(nvalues, GFP_KERNEL);
		if (!values)
			return;

		if (value)
			bitmap_fill(values, nvalues);
		else
			bitmap_zero(values, nvalues);

		gpiod_set_array_value_cansleep(nvalues, pwr_gpios->desc,
					       pwr_gpios->info, values);

		bitmap_free(values);
	}
}

static int ky_regulator_set_voltage_if_supported(struct regulator *regulator,
						  int min_uV, int target_uV,
						  int max_uV)
{
	int current_uV;

	/*
	 * Check if supported first to avoid errors since we may try several
	 * signal levels during power up and don't want to show errors.
	 */
	if (!regulator_is_supported_voltage(regulator, min_uV, max_uV))
		return -EINVAL;

	/*
	 * The voltage is already set, no need to switch.
	 * Return 1 to indicate that no switch happened.
	 */
	current_uV = regulator_get_voltage(regulator);
	if (current_uV == target_uV)
		return 1;

	return regulator_set_voltage_triplet(regulator, min_uV, target_uV,
					     max_uV);
}

static void ky_regulator_on(struct ky_pwrseq *pwrseq,
						struct regulator *regulator, int volt, bool on_off)
{
	struct device *dev = pwrseq->dev;
	int ret, min_uV, max_uV;

	if(on_off){
		/*
		 * mostly, vdd voltage is 3.3V, io voltage is 1.8V or 3.3V.
		 * maybe need support 1.2V io signaling later.
		 */
		if(regulator == pwrseq->io_supply){
			min_uV = max(volt - 100000, 1700000);
			max_uV = min(volt + 150000, 3300000);
		}else{
			min_uV = max(volt - 300000, 2700000);
			max_uV = min(volt + 200000, 3600000);
		}

		ret = ky_regulator_set_voltage_if_supported(regulator,
					min_uV, volt, max_uV);
		if (ret < 0) {
			dev_err(dev, "set voltage failed!\n");
			return;
		}

		ret = regulator_enable(regulator);
		if (ret < 0) {
			dev_err(dev, "enable failed\n");
			return;
		}

		ret = regulator_get_voltage(regulator);
		if (ret < 0) {
			dev_err(dev, "get voltage failed\n");
			return;
		}

		dev_info(dev, "check voltage: %d\n", ret);
	}else{
		regulator_disable(regulator);
	}
}

static void ky_pre_power_on(struct ky_pwrseq *pwrseq,
						bool on_off)
{
	if(!IS_ERR(pwrseq->vdd_supply))
		ky_regulator_on(pwrseq, pwrseq->vdd_supply, pwrseq->vdd_voltage, on_off);

	if(!IS_ERR(pwrseq->io_supply))
		ky_regulator_on(pwrseq, pwrseq->io_supply, pwrseq->io_voltage, on_off);
}

static void ky_post_power_on(struct ky_pwrseq *pwrseq,
						bool on_off)
{
	if (!IS_ERR(pwrseq->ext_clk)) {
		if(on_off && !pwrseq->clk_enabled){
			clk_prepare_enable(pwrseq->ext_clk);
			pwrseq->clk_enabled = true;
		}

		if(!on_off && pwrseq->clk_enabled){
			clk_disable_unprepare(pwrseq->ext_clk);
			pwrseq->clk_enabled = false;
		}
	}
}

void ky_power_on(struct ky_pwrseq *pwrseq,
						bool on_off)
{
	mutex_lock(&pwrseq->pwrseq_mutex);
	if(on_off){
		if (!atomic_read(&pwrseq->pwrseq_count)){
			dev_info(pwrseq->dev, "turn power on\n");
			ky_pre_power_on(pwrseq, on_off);
			ky_set_gpios_value(pwrseq, on_off);
			if (pwrseq->power_on_delay_ms)
				msleep(pwrseq->power_on_delay_ms);
			ky_post_power_on(pwrseq, on_off);
		}
		atomic_inc(&pwrseq->pwrseq_count);
	}else{
		if (atomic_read(&pwrseq->pwrseq_count)){
			if (!atomic_dec_return(&pwrseq->pwrseq_count)){
				dev_info(pwrseq->dev, "turn power off\n");
				ky_post_power_on(pwrseq, on_off);
				ky_set_gpios_value(pwrseq, on_off);
				ky_pre_power_on(pwrseq, on_off);
			}
		}else{
			dev_err(pwrseq->dev, "already power off, please check\n");
		}
	}
	mutex_unlock(&pwrseq->pwrseq_mutex);
}

static int ky_pwrseq_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct ky_pwrseq *pwrseq;
	int ret;

	pwrseq = devm_kzalloc(dev, sizeof(*pwrseq), GFP_KERNEL);
	if (!pwrseq)
		return -ENOMEM;

	pwrseq->dev = dev;
	platform_set_drvdata(pdev, pwrseq);

	pwrseq->vdd_supply = devm_regulator_get_optional(dev, "vdd");
	if (IS_ERR(pwrseq->vdd_supply)) {
		if (PTR_ERR(pwrseq->vdd_supply) == -EPROBE_DEFER)
			return -EPROBE_DEFER;
		dev_dbg(dev, "No vdd regulator found\n");
	}else{
		if (device_property_read_u32(dev, "vdd_voltage", &pwrseq->vdd_voltage)) {
			pwrseq->vdd_voltage = 3300000;
			dev_dbg(dev, "failed get vdd voltage,use default value (%u)\n", pwrseq->vdd_voltage);
		}
	}

	pwrseq->io_supply = devm_regulator_get_optional(dev, "io");
	if (IS_ERR(pwrseq->io_supply)) {
		if (PTR_ERR(pwrseq->io_supply) == -EPROBE_DEFER)
			return -EPROBE_DEFER;
		dev_dbg(dev, "No io regulator found\n");
	}else{
		if (device_property_read_u32(dev, "io_voltage", &pwrseq->io_voltage)) {
			pwrseq->io_voltage = 1800000;
			dev_dbg(dev, "failed get io voltage,use default value (%u)\n", pwrseq->io_voltage);
		}
	}

	pwrseq->pwr_gpios = devm_gpiod_get_array(dev, "pwr",
							GPIOD_OUT_LOW);
	if (IS_ERR(pwrseq->pwr_gpios) &&
		PTR_ERR(pwrseq->pwr_gpios) != -ENOENT &&
		PTR_ERR(pwrseq->pwr_gpios) != -ENOSYS) {
		return PTR_ERR(pwrseq->pwr_gpios);
	}

	pwrseq->ext_clk = devm_clk_get(dev, "clock");
	if (IS_ERR(pwrseq->ext_clk) && PTR_ERR(pwrseq->ext_clk) != -ENOENT){
		dev_dbg(dev, "failed get ext clock\n");
		return PTR_ERR(pwrseq->ext_clk);
	}

	if(device_property_read_u32(dev, "power-on-delay-ms",
				 &pwrseq->power_on_delay_ms))
		pwrseq->power_on_delay_ms = 100;

	if(device_property_read_bool(dev, "power-always-on"))
		pwrseq->always_on = true;

	if (node) {
		ret = of_platform_populate(node, NULL, NULL, dev);
		if (ret) {
			dev_err(dev, "failed to add sub pwrseq\n");
			return ret;
		}
	} else {
		dev_err(dev, "no device node, failed to add sub pwrseq\n");
		ret = -ENODEV;
		return ret;
	}

	mutex_init(&pwrseq->pwrseq_mutex);
	atomic_set(&pwrseq->pwrseq_count, 0);

	if(pwrseq->always_on)
		ky_power_on(pwrseq, 1);

	return 0;
}

static int ky_pwrseq_remove(struct platform_device *pdev)
{
	struct ky_pwrseq *pwrseq = platform_get_drvdata(pdev);

	if(pwrseq->always_on)
		ky_power_on(pwrseq, 0);

	mutex_destroy(&pwrseq->pwrseq_mutex);
	of_platform_depopulate(&pdev->dev);

	return 0;
}

static const struct of_device_id ky_pwrseq_ids[] = {
	{ .compatible = "ky,rf-pwrseq" },
	{ /* Sentinel */ }
};

#ifdef CONFIG_PM_SLEEP
static int ky_pwrseq_suspend(struct device *dev)
{
	return 0;
}

static int ky_pwrseq_resume(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops ky_pwrseq_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ky_pwrseq_suspend, ky_pwrseq_resume)
};

#define DEV_PM_OPS	(&ky_pwrseq_dev_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif /* CONFIG_PM_SLEEP */

static struct platform_driver ky_pwrseq_driver = {
	.probe		= ky_pwrseq_probe,
	.remove	= ky_pwrseq_remove,
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= "ky-rf-pwrseq",
		.of_match_table	= ky_pwrseq_ids,
	},
};

module_platform_driver(ky_pwrseq_driver);

MODULE_DESCRIPTION("ky rf pwrseq driver");
MODULE_LICENSE("GPL");
