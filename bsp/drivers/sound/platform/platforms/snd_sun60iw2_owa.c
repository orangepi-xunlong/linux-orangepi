/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2024, lijingpsw <lijingpsw@allwinnertech.com>
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
	/* parent clk */
	struct clk *clk_pll_audio0_4x;
	struct clk *clk_pll_audio1_div2;
	struct clk *clk_pll_audio1_div5;

	/* module clk */
	struct clk *clk_owa_tx;
	struct clk *clk_owa_rx;

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

	/* get parent clk */
	clk->clk_pll_audio0_4x = of_clk_get_by_name(np, "clk_pll_audio0_4x");
	if (IS_ERR_OR_NULL(clk->clk_pll_audio0_4x)) {
		SND_LOG_ERR_STD(E_OWA_SWDEP_CLK_INIT, "clk_pll_audio0_4x get failed\n");
		ret = PTR_ERR(clk->clk_pll_audio0_4x);
		goto err_get_clk_pll_audio0_4x;
	}
	clk->clk_pll_audio1_div5 = of_clk_get_by_name(np, "clk_pll_audio1_div5");
	if (IS_ERR_OR_NULL(clk->clk_pll_audio1_div5)) {
		SND_LOG_ERR_STD(E_OWA_SWDEP_CLK_INIT, "clk_pll_audio1_div5 get failed\n");
		ret = PTR_ERR(clk->clk_pll_audio1_div5);
		goto err_get_clk_pll_audio1_div5;
	}

	/* get owa clk */
	clk->clk_owa_tx = of_clk_get_by_name(np, "clk_owa_tx");
	if (IS_ERR_OR_NULL(clk->clk_owa_tx)) {
		SND_LOG_ERR("clk owa tx get failed\n");
		ret = PTR_ERR(clk->clk_owa_tx);
		goto err_get_clk_owa_tx;
	}
	clk->clk_owa_rx = of_clk_get_by_name(np, "clk_owa_rx");
	if (IS_ERR_OR_NULL(clk->clk_owa_rx)) {
		SND_LOG_ERR("clk owa rx get failed\n");
		ret = PTR_ERR(clk->clk_owa_rx);
		goto err_get_clk_owa_rx;
	}

	return clk;

err_get_clk_owa_rx:
	clk_put(clk->clk_owa_rx);
err_get_clk_owa_tx:
	clk_put(clk->clk_owa_tx);
err_get_clk_pll_audio1_div5:
	clk_put(clk->clk_pll_audio1_div5);
err_get_clk_pll_audio0_4x:
	clk_put(clk->clk_pll_audio0_4x);
err_get_clk_bus:
	clk_put(clk->clk_bus);
err_get_clk_rst:
	kfree(clk);
	return NULL;
}

void snd_owa_clk_exit(void *clk_orig)
{
	struct sunxi_owa_clk *clk = (struct sunxi_owa_clk *)clk_orig;

	SND_LOG_DEBUG("\n");

	clk_put(clk->clk_owa_rx);
	clk_put(clk->clk_owa_tx);
	clk_put(clk->clk_pll_audio0_4x);
	clk_put(clk->clk_pll_audio1_div5);
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

	if (clk_prepare_enable(clk->clk_pll_audio0_4x)) {
		SND_LOG_ERR_STD(E_OWA_SWDEP_CLK_EN, "clk_pll_audio0_4x enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_pll_audio0_4x;
	}
	if (clk_prepare_enable(clk->clk_pll_audio1_div5)) {
		SND_LOG_ERR_STD(E_OWA_SWDEP_CLK_EN, "clk_pll_audio1_div5 enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_pll_audio1_div5;
	}

	if (clk_prepare_enable(clk->clk_owa_tx)) {
		SND_LOG_ERR("clk_owa_tx enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_owa_tx;
	}

	if (clk_prepare_enable(clk->clk_owa_rx)) {
		SND_LOG_ERR("clk_owa_rx enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_owa_rx;
	}

	return 0;

err_enable_clk_owa_rx:
	clk_disable_unprepare(clk->clk_owa_rx);
err_enable_clk_owa_tx:
	clk_disable_unprepare(clk->clk_owa_tx);
err_enable_clk_pll_audio0_4x:
	clk_disable_unprepare(clk->clk_pll_audio0_4x);
err_enable_clk_pll_audio1_div5:
	clk_disable_unprepare(clk->clk_pll_audio1_div5);

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

	clk_disable_unprepare(clk->clk_owa_rx);
	clk_disable_unprepare(clk->clk_owa_tx);
	clk_disable_unprepare(clk->clk_pll_audio0_4x);
	clk_disable_unprepare(clk->clk_pll_audio1_div5);
}

int snd_owa_clk_rate(void *clk_orig, unsigned int freq_in, unsigned int freq_out)
{
	struct sunxi_owa_clk *clk = (struct sunxi_owa_clk *)clk_orig;

	SND_LOG_DEBUG("\n");

	if (freq_in % 24576000 == 0) {
		if (clk_set_parent(clk->clk_owa_tx, clk->clk_pll_audio1_div5)) {
			SND_LOG_ERR_STD(E_OWA_SWDEP_CLK_SET, "set owa tx parent clk failed\n");
			return -EINVAL;
		}
	} else {
		if (clk_set_parent(clk->clk_owa_tx, clk->clk_pll_audio0_4x)) {
			SND_LOG_ERR_STD(E_OWA_SWDEP_CLK_SET, "set owa tx parent clk failed\n");
			return -EINVAL;
		}
	}

	if (clk_set_rate(clk->clk_owa_tx, freq_out)) {
		SND_LOG_ERR_STD(E_OWA_SWDEP_CLK_SET, "clk owa tx set rate failed\n");
		return -EINVAL;
	}

	return 0;
}
