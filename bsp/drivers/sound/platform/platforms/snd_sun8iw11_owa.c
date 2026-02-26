/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2022, huhaoxin <huhaoxin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/device.h>
#include <linux/regmap.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_owa.h"

struct sunxi_owa_clk {
	struct clk *clk_pll;
	struct clk *clk_owa;

	struct clk *clk_bus;
	struct reset_control *clk_rst;
};

sunxi_owa_clk_t *snd_owa_clk_init(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_owa_clk *clk = NULL;

	SND_LOG_DEBUG("\n");

	clk = kzalloc(sizeof(*clk), GFP_KERNEL);
	if (!clk) {
		SND_LOG_ERR("can't allocate sunxi_owa_clk memory\n");
		return NULL;
	}

	/* get rst clk */
	clk->clk_rst = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR_OR_NULL(clk->clk_rst)) {
		SND_LOG_ERR("clk rst get failed\n");
		ret =  PTR_ERR(clk->clk_rst);
		goto err_get_clk_rst;
	}

	/* get bus clk */
	clk->clk_bus = of_clk_get_by_name(np, "clk_bus_owa");
	if (IS_ERR_OR_NULL(clk->clk_bus)) {
		SND_LOG_ERR("clk bus get failed\n");
		ret = PTR_ERR(clk->clk_bus);
		goto err_get_clk_bus;
	}

	clk->clk_pll = of_clk_get_by_name(np, "clk_pll_audio");
	if (IS_ERR_OR_NULL(clk->clk_pll)) {
		SND_LOG_ERR("clk pll get failed\n");
		ret = PTR_ERR(clk->clk_pll);
		goto err_get_clk_pll;
	}

	/* get owa clk */
	clk->clk_owa = of_clk_get_by_name(np, "clk_owa");
	if (IS_ERR_OR_NULL(clk->clk_owa)) {
		SND_LOG_ERR("clk owa get failed\n");
		ret = PTR_ERR(clk->clk_owa);
		goto err_get_clk_owa;
	}

	/* set clk owa parent of pllaudio */
	if (clk_set_parent(clk->clk_owa, clk->clk_pll)) {
		SND_LOG_ERR("set parent clk owa failed\n");
		ret = -EINVAL;
		goto err_set_parent;
	}

	return clk;

err_set_parent:
	clk_put(clk->clk_owa);
err_get_clk_owa:
	clk_put(clk->clk_pll);
err_get_clk_pll:
	clk_put(clk->clk_bus);
err_get_clk_bus:
err_get_clk_rst:
	kfree(clk);
	return NULL;
}

void snd_owa_clk_exit(void *clk_orig)
{
	struct sunxi_owa_clk *clk = (struct sunxi_owa_clk *)clk_orig;

	SND_LOG_DEBUG("\n");

	clk_put(clk->clk_owa);
	clk_put(clk->clk_pll);
	clk_put(clk->clk_bus);

	kfree(clk);
}

int snd_owa_clk_bus_enable(void *clk_orig)
{
	int ret = 0;
	struct sunxi_owa_clk *clk = (struct sunxi_owa_clk *)clk_orig;

	SND_LOG_DEBUG("\n");

	if (reset_control_deassert(clk->clk_rst)) {
		SND_LOG_ERR("clk_rst deassert failed\n");
		ret = -EINVAL;
		goto err_deassert_rst;
	}

	if (clk_prepare_enable(clk->clk_bus)) {
		SND_LOG_ERR("clk_bus enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_bus;
	}

	return 0;

err_enable_clk_bus:
	reset_control_assert(clk->clk_rst);
err_deassert_rst:
	return ret;
}

int snd_owa_clk_enable(void *clk_orig)
{
	int ret = 0;
	struct sunxi_owa_clk *clk = (struct sunxi_owa_clk *)clk_orig;

	SND_LOG_DEBUG("\n");

	if (clk_prepare_enable(clk->clk_pll)) {
		SND_LOG_ERR("clk pll enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_pll;
	}

	if (clk_prepare_enable(clk->clk_owa)) {
		SND_LOG_ERR("clk_owa enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_owa;
	}

	return 0;

err_enable_clk_owa:
	clk_disable_unprepare(clk->clk_pll);
err_enable_clk_pll:
	return ret;
}

void snd_owa_clk_bus_disable(void *clk_orig)
{
	struct sunxi_owa_clk *clk = (struct sunxi_owa_clk *)clk_orig;

	SND_LOG_DEBUG("\n");

	clk_disable_unprepare(clk->clk_bus);
	reset_control_assert(clk->clk_rst);
}

void snd_owa_clk_disable(void *clk_orig)
{
	struct sunxi_owa_clk *clk = (struct sunxi_owa_clk *)clk_orig;

	SND_LOG_DEBUG("\n");

	clk_disable_unprepare(clk->clk_owa);
	clk_disable_unprepare(clk->clk_pll);
}

int snd_owa_clk_rate(void *clk_orig, unsigned int freq_in, unsigned int freq_out)
{
	struct sunxi_owa_clk *clk = (struct sunxi_owa_clk *)clk_orig;

	SND_LOG_DEBUG("\n");

	if (clk_set_rate(clk->clk_pll, freq_out)) {
		SND_LOG_ERR("set clk_i2s rate failed, rate: %u\n", freq_out);
		return -EINVAL;
	}

	return 0;
}
