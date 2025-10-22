// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2023, huhaoxin <huhaoxin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#define SUNXI_MODNAME		"sound-clk-test"
#include "snd_sunxi_log.h"
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/regmap.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>

#include "snd_sunxi_test_clk.h"

#define DRV_NAME "sunxi-test-clk"

static struct regmap_config sunxi_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_I2S_MAX_REG,
	.cache_type = REGCACHE_NONE,
};

static int snd_sunxi_mem_init(struct platform_device *pdev, struct sunxi_test_clk_mem *mem)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	ret = of_address_to_resource(np, 0, &mem->res);
	if (ret) {
		SND_LOG_ERR("parse device node resource failed\n");
		ret = -EINVAL;
		goto err_of_addr_to_resource;
	}

	mem->memregion = devm_request_mem_region(&pdev->dev, mem->res.start,
						 resource_size(&mem->res), DRV_NAME);
	if (IS_ERR_OR_NULL(mem->memregion)) {
		SND_LOG_ERR("memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_request_region;
	}

	mem->membase = devm_ioremap(&pdev->dev, mem->memregion->start,
				    resource_size(mem->memregion));
	if (IS_ERR_OR_NULL(mem->membase)) {
		SND_LOG_ERR("ioremap failed\n");
		ret = -EBUSY;
		goto err_devm_ioremap;
	}

	mem->regmap = devm_regmap_init_mmio(&pdev->dev, mem->membase, &sunxi_regmap_config);
	if (IS_ERR_OR_NULL(mem->regmap)) {
		SND_LOG_ERR("regmap init failed\n");
		ret = -EINVAL;
		goto err_devm_regmap_init;
	}

	return 0;

err_devm_regmap_init:
	devm_iounmap(&pdev->dev, mem->membase);
err_devm_ioremap:
	devm_release_mem_region(&pdev->dev, mem->memregion->start, resource_size(mem->memregion));
err_devm_request_region:
err_of_addr_to_resource:
	return ret;
}

static void snd_sunxi_mem_exit(struct platform_device *pdev, struct sunxi_test_clk_mem *mem)
{
	SND_LOG_DEBUG("\n");

	devm_iounmap(&pdev->dev, mem->membase);
	devm_release_mem_region(&pdev->dev, mem->memregion->start, resource_size(mem->memregion));
}

static int snd_sunxi_clk_enable_sun55iw3(struct sunxi_test_clk_clk *clk)
{
	int ret = 0;

	SND_LOG_DEBUG("\n");

	if (reset_control_deassert(clk->clk_rst)) {
		SND_LOG_ERR("clk_rst deassert failed\n");
		goto err_deassert_rst;
	}

	if (clk_prepare_enable(clk->clk_peri0_2x)) {
		SND_LOG_ERR("clk_peri0_2x enable failed\n");
		goto err_enable_clk_peri0_2x;
	}
	if (clk_prepare_enable(clk->clk_mcu_src)) {
		SND_LOG_ERR("clk_mcu_src enable failed\n");
		goto err_enable_clk_mcu_src;
	}

	if (clk_prepare_enable(clk->clk_bus)) {
		SND_LOG_ERR("clk_bus enable failed\n");
		goto err_enable_clk_bus;
	}

	if (clk_prepare_enable(clk->clk_pll_audio0_4x)) {
		SND_LOG_ERR("clk_pll_audio0_4x enable failed\n");
		goto err_enable_clk_pll_audio0_4x;
	}
	if (clk_prepare_enable(clk->clk_pll_audio1_div2)) {
		SND_LOG_ERR("clk_pll_audio1_div2 enable failed\n");
		goto err_enable_clk_pll_audio1_div2;
	}
	if (clk_prepare_enable(clk->clk_pll_audio1_div5)) {
		SND_LOG_ERR("clk_pll_audio1_div5 enable failed\n");
		goto err_enable_clk_pll_audio1_div5;
	}

	if (clk_prepare_enable(clk->clk_i2s)) {
		SND_LOG_ERR("clk_i2s enable failed\n");
		goto err_enable_clk_i2s;
	}

	return 0;

err_enable_clk_i2s:
	clk_disable_unprepare(clk->clk_pll_audio1_div5);
err_enable_clk_pll_audio1_div5:
	clk_disable_unprepare(clk->clk_pll_audio1_div2);
err_enable_clk_pll_audio1_div2:
	clk_disable_unprepare(clk->clk_pll_audio0_4x);
err_enable_clk_pll_audio0_4x:
	clk_disable_unprepare(clk->clk_bus);
err_enable_clk_bus:
	clk_disable_unprepare(clk->clk_mcu_src);
err_enable_clk_mcu_src:
	clk_disable_unprepare(clk->clk_peri0_2x);
err_enable_clk_peri0_2x:
	reset_control_assert(clk->clk_rst);
err_deassert_rst:
	return ret;
}

static void snd_sunxi_clk_disable_sun55iw3(struct sunxi_test_clk_clk *clk)
{
	SND_LOG_DEBUG("\n");

	clk_disable_unprepare(clk->clk_i2s);
	clk_disable_unprepare(clk->clk_pll_audio1_div5);
	clk_disable_unprepare(clk->clk_pll_audio1_div2);
	clk_disable_unprepare(clk->clk_pll_audio0_4x);
	clk_disable_unprepare(clk->clk_bus);
	clk_disable_unprepare(clk->clk_mcu_src);
	clk_disable_unprepare(clk->clk_peri0_2x);
	reset_control_assert(clk->clk_rst);
}

static int snd_sunxi_clk_init_sun55iw3(struct platform_device *pdev, struct sunxi_test_clk_clk *clk)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	/* Get rst clk */
	clk->clk_rst = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR_OR_NULL(clk->clk_rst)) {
		SND_LOG_ERR("clk rst get failed\n");
		ret =  PTR_ERR(clk->clk_rst);
		goto err_get_clk_rst;
	}

	/* Get dependent clock */
	clk->clk_peri0_2x = of_clk_get_by_name(np, "clk_pll_peri0_2x");
	if (IS_ERR_OR_NULL(clk->clk_peri0_2x)) {
		SND_LOG_ERR("clk_peri0_2x get failed\n");
		ret = PTR_ERR(clk->clk_peri0_2x);
		goto err_get_clk_peri0_2x;
	}
	clk->clk_mcu_src = of_clk_get_by_name(np, "clk_mcu_src");
	if (IS_ERR_OR_NULL(clk->clk_mcu_src)) {
		SND_LOG_ERR("clk_mcu_src get failed\n");
		ret = PTR_ERR(clk->clk_mcu_src);
		goto err_get_clk_mcu_src;
	}

	/* Get bus clk */
	clk->clk_bus = of_clk_get_by_name(np, "clk_bus_i2s");
	if (IS_ERR_OR_NULL(clk->clk_bus)) {
		SND_LOG_ERR("clk bus get failed\n");
		ret = PTR_ERR(clk->clk_bus);
		goto err_get_clk_bus;
	}

	/* Get parent clk */
	clk->clk_pll_audio0_4x = of_clk_get_by_name(np, "clk_pll_audio0_4x");
	if (IS_ERR_OR_NULL(clk->clk_pll_audio0_4x)) {
		SND_LOG_ERR("clk_pll_audio0_4x get failed\n");
		ret = PTR_ERR(clk->clk_pll_audio0_4x);
		goto err_get_clk_pll_audio0_4x;
	}
	clk->clk_pll_audio1_div2 = of_clk_get_by_name(np, "clk_pll_audio1_div2");
	if (IS_ERR_OR_NULL(clk->clk_pll_audio1_div2)) {
		SND_LOG_ERR("clk_pll_audio1_div2 get failed\n");
		ret = PTR_ERR(clk->clk_pll_audio1_div2);
		goto err_get_clk_pll_audio1_div2;
	}
	clk->clk_pll_audio1_div5 = of_clk_get_by_name(np, "clk_pll_audio1_div5");
	if (IS_ERR_OR_NULL(clk->clk_pll_audio1_div5)) {
		SND_LOG_ERR("clk_pll_audio1_div5 get failed\n");
		ret = PTR_ERR(clk->clk_pll_audio1_div5);
		goto err_get_clk_pll_audio1_div5;
	}

	/* Get i2s clk */
	clk->clk_i2s = of_clk_get_by_name(np, "clk_i2s");
	if (IS_ERR_OR_NULL(clk->clk_i2s)) {
		SND_LOG_ERR("clk i2s get failed\n");
		ret = PTR_ERR(clk->clk_i2s);
		goto err_get_clk_i2s;
	}

	ret = snd_sunxi_clk_enable_sun55iw3(clk);
	if (ret) {
		SND_LOG_ERR("clk enable failed\n");
		ret = -EINVAL;
		goto err_clk_enable;
	}

	return 0;

err_clk_enable:
	clk_put(clk->clk_i2s);
err_get_clk_i2s:
	clk_put(clk->clk_pll_audio1_div5);
err_get_clk_pll_audio1_div5:
	clk_put(clk->clk_pll_audio1_div2);
err_get_clk_pll_audio1_div2:
	clk_put(clk->clk_pll_audio0_4x);
err_get_clk_pll_audio0_4x:
	clk_put(clk->clk_bus);
err_get_clk_bus:
	clk_put(clk->clk_mcu_src);
err_get_clk_mcu_src:
	clk_put(clk->clk_peri0_2x);
err_get_clk_peri0_2x:
err_get_clk_rst:
	return ret;
}

static void snd_sunxi_clk_exit_sun55iw3(struct sunxi_test_clk_clk *clk)
{
	SND_LOG_DEBUG("\n");

	snd_sunxi_clk_disable_sun55iw3(clk);
	clk_put(clk->clk_i2s);
	clk_put(clk->clk_pll_audio1_div5);
	clk_put(clk->clk_pll_audio1_div2);
	clk_put(clk->clk_pll_audio0_4x);
	clk_put(clk->clk_bus);
	clk_put(clk->clk_mcu_src);
	clk_put(clk->clk_peri0_2x);
}

static int snd_sunxi_clk_rate_sun55iw3(struct sunxi_test_clk_clk *clk, unsigned int freq)
{
	SND_LOG_DEBUG("\n");

	/* Set dependent clk */
	if (clk_set_parent(clk->clk_mcu_src, clk->clk_peri0_2x)) {
		SND_LOG_ERR("set clk_mcu_src parent clk failed\n");
		return -EINVAL;
	}
	if (clk_set_rate(clk->clk_mcu_src, 600000000)) {
		SND_LOG_ERR("set clk_mcu_src rate failed, rate\n");
		return -EINVAL;
	}

	if (freq % 24576000 == 0) {
		/* If you want to use clk_pll_audio0_4x, must set it 1083801600Hz */
		if (clk_set_parent(clk->clk_i2s, clk->clk_pll_audio0_4x)) {
			SND_LOG_ERR("set i2s parent clk failed\n");
			return -EINVAL;
		}
		if (clk_set_rate(clk->clk_pll_audio0_4x, 1083801600)) {
			SND_LOG_ERR("set clk_pll_audio0_4x rate failed\n");
			return -EINVAL;
		}
	} else {
		if (clk_set_parent(clk->clk_i2s, clk->clk_pll_audio1_div5)) {
			SND_LOG_ERR("set i2s parent clk failed\n");
			return -EINVAL;
		}
	}

	if (clk_set_rate(clk->clk_i2s, freq)) {
		SND_LOG_ERR("freq : %u module clk unsupport\n", freq);
		return -EINVAL;
	}

	return 0;
}

static int snd_sunxi_pin_init(struct platform_device *pdev, struct sunxi_test_clk_pinctl *pin)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	if (of_property_read_bool(np, "pinctrl-used")) {
		pin->pinctrl_used = 1;
	} else {
		pin->pinctrl_used = 0;
		SND_LOG_DEBUG("unused pinctrl\n");
		return 0;
	}

	pin->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR_OR_NULL(pin->pinctrl)) {
		SND_LOG_ERR("pinctrl get failed\n");
		ret = -EINVAL;
		return ret;
	}
	pin->pinstate = pinctrl_lookup_state(pin->pinctrl, PINCTRL_STATE_DEFAULT);
	if (IS_ERR_OR_NULL(pin->pinstate)) {
		SND_LOG_ERR("pinctrl default state get fail\n");
		ret = -EINVAL;
		goto err_loopup_pinstate;
	}
	pin->pinstate_sleep = pinctrl_lookup_state(pin->pinctrl, PINCTRL_STATE_SLEEP);
	if (IS_ERR_OR_NULL(pin->pinstate_sleep)) {
		SND_LOG_ERR("pinctrl sleep state get failed\n");
		ret = -EINVAL;
		goto err_loopup_pin_sleep;
	}
	ret = pinctrl_select_state(pin->pinctrl, pin->pinstate);
	if (ret < 0) {
		SND_LOG_ERR("test set pinctrl default state fail\n");
		ret = -EBUSY;
		goto err_pinctrl_select_default;
	}

	return 0;

err_pinctrl_select_default:
err_loopup_pin_sleep:
err_loopup_pinstate:
	devm_pinctrl_put(pin->pinctrl);
	return ret;
}

static void snd_sunxi_pin_exit(struct sunxi_test_clk_pinctl *pin)
{
	SND_LOG_DEBUG("\n");

	if (pin->pinctrl_used)
		devm_pinctrl_put(pin->pinctrl);
}

static int snd_sunxi_reg_set(struct sunxi_test_clk *sunxi_test)
{
	struct regmap *regmap = sunxi_test->mem.regmap;

	SND_LOG_DEBUG("\n");

	regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << GLOBAL_EN, 1 << GLOBAL_EN);
	regmap_update_bits(regmap, SUNXI_I2S_CLKDIV, 1 << MCLKOUT_EN, 1 << MCLKOUT_EN);
	regmap_update_bits(regmap, SUNXI_I2S_CLKDIV, 1 << MCLK_DIV, 1 << MCLK_DIV);

	return 0;
}

static int sunxi_test_dev_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_test_clk *sunxi_test;
	struct sunxi_test_clk_mem *mem;
	struct sunxi_test_clk_clk *clk;
	struct sunxi_test_clk_pinctl *pin;
	struct sunxi_test_clk_quirks *quirks;

	/* This variable should be set to 24576000 or 22579200 by clk developers */
	unsigned int freq = 24576000;
	int ret;

	SND_LOG_DEBUG("\n");

	/* Sunxi test clk info */
	sunxi_test = devm_kzalloc(dev, sizeof(*sunxi_test), GFP_KERNEL);
	if (!sunxi_test) {
		SND_LOG_ERR("can't allocate sunxi_test memory\n");
		ret = -ENOMEM;
		goto err_snd_sunxi_kzalloc;
	}
	dev_set_drvdata(dev, sunxi_test);
	mem = &sunxi_test->mem;
	clk = &sunxi_test->clk;
	pin = &sunxi_test->pin;

	/* Get quirks */
	quirks = (struct sunxi_test_clk_quirks *)of_device_get_match_data(&pdev->dev);
	if (quirks == NULL) {
		SND_LOG_ERR("quirks get failed\n");
		goto err_get_quirks;
	}
	sunxi_test->quirks = quirks;

	/* Init resource */
	ret = snd_sunxi_mem_init(pdev, mem);
	if (ret) {
		SND_LOG_ERR("remap init failed\n");
		goto err_snd_sunxi_mem_init;
	}

	ret = quirks->snd_sunxi_clk_init(pdev, clk);
	if (ret) {
		SND_LOG_ERR("clk init failed\n");
		goto err_snd_sunxi_clk_init;
	}

	ret = snd_sunxi_pin_init(pdev, pin);
	if (ret) {
		SND_LOG_ERR("pinctrl init failed\n");
		goto err_snd_sunxi_pin_init;
	}

	/* Start testing */
	ret = quirks->snd_sunxi_clk_rate(clk, freq);
	if (ret) {
		SND_LOG_ERR("set clk rate failed\n");
		goto err_snd_sunxi_clk_rate;
	}
	snd_sunxi_reg_set(sunxi_test);

	return 0;

err_snd_sunxi_clk_rate:
err_snd_sunxi_pin_init:
	quirks->snd_sunxi_clk_exit(&sunxi_test->clk);
err_snd_sunxi_clk_init:
	snd_sunxi_mem_exit(pdev, &sunxi_test->mem);
err_snd_sunxi_mem_init:
	kfree(sunxi_test);
err_get_quirks:
err_snd_sunxi_kzalloc:
	return ret;
}

static int sunxi_test_dev_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_test_clk *sunxi_test = dev_get_drvdata(dev);
	struct sunxi_test_clk_mem *mem = &sunxi_test->mem;
	struct sunxi_test_clk_clk *clk = &sunxi_test->clk;
	struct sunxi_test_clk_pinctl *pin = &sunxi_test->pin;
	struct sunxi_test_clk_quirks *quirks = sunxi_test->quirks;

	SND_LOG_DEBUG("\n");

	snd_sunxi_pin_exit(pin);
	quirks->snd_sunxi_clk_exit(clk);
	snd_sunxi_mem_exit(pdev, mem);
	kfree(sunxi_test);

	return 0;
}

static const struct sunxi_test_clk_quirks sunxi_test_clk_quirks_sun55iw3 = {
	.snd_sunxi_clk_init	= snd_sunxi_clk_init_sun55iw3,
	.snd_sunxi_clk_exit	= snd_sunxi_clk_exit_sun55iw3,
	.snd_sunxi_clk_enable	= snd_sunxi_clk_enable_sun55iw3,
	.snd_sunxi_clk_disable	= snd_sunxi_clk_disable_sun55iw3,
	.snd_sunxi_clk_rate	= snd_sunxi_clk_rate_sun55iw3,
};

static const struct of_device_id sunxi_test_clk_of_match[] = {
	{
		.compatible = "allwinner,sun55iw3",
		.data = &sunxi_test_clk_quirks_sun55iw3,
	},
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_test_clk_of_match);

static struct platform_driver sunxi_test_clk_driver = {
	.driver = {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= sunxi_test_clk_of_match,
	},
	.probe = sunxi_test_dev_probe,
	.remove = sunxi_test_dev_remove,
};

int __init sunxi_test_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_test_clk_driver);
	if (ret != 0) {
		SND_LOG_ERR("platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_test_dev_exit(void)
{
	platform_driver_unregister(&sunxi_test_clk_driver);
}

late_initcall(sunxi_test_dev_init);
module_exit(sunxi_test_dev_exit);

MODULE_AUTHOR("huhaoxin@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sunxi soundcard platform of i2s");
MODULE_VERSION("1.0.1");