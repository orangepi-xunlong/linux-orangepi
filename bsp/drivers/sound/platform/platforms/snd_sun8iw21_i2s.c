/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2023, xudongpdc <xudongpdc@allwinnertech.com>
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
#include "snd_sunxi_i2s.h"


struct sunxi_i2s_clk {
	/* parent clk */
	struct clk *clk_pll_audio_1x;
	/* module clk */
	struct clk *clk_i2s;

	struct clk *clk_bus;
	struct reset_control *clk_rst;

};

sunxi_i2s_clk_t *snd_i2s_clk_init(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_i2s_clk *clk = NULL;

	SND_LOG_DEBUG("\n");

	clk = kzalloc(sizeof(*clk), GFP_KERNEL);
	if (!clk) {
		SND_LOG_ERR("can't allocate sunxi_i2s_clk memory\n");
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
	clk->clk_bus = of_clk_get_by_name(np, "clk_bus_i2s");
	if (IS_ERR_OR_NULL(clk->clk_bus)) {
		SND_LOG_ERR("clk bus get failed\n");
		ret = PTR_ERR(clk->clk_bus);
		goto err_get_clk_bus;
	}

	/* get parent clk */
	clk->clk_pll_audio_1x = of_clk_get_by_name(np, "clk_pll_audio_1x");
	if (IS_ERR_OR_NULL(clk->clk_pll_audio_1x)) {
		SND_LOG_ERR("clk_pll_audio_1x get failed\n");
		ret = PTR_ERR(clk->clk_pll_audio_1x);
		goto err_get_clk_pll_audio_1x;
	}

	/* get i2s clk */
	clk->clk_i2s = of_clk_get_by_name(np, "clk_i2s");
	if (IS_ERR_OR_NULL(clk->clk_i2s)) {
		SND_LOG_ERR("clk dmic get failed\n");
		ret = PTR_ERR(clk->clk_i2s);
		goto err_get_clk_i2s;
	}

	return clk;

err_get_clk_i2s:
	clk_put(clk->clk_pll_audio_1x);
err_get_clk_pll_audio_1x:
	clk_put(clk->clk_bus);
err_get_clk_bus:
err_get_clk_rst:
	kfree(clk);
	return NULL;
}


void snd_i2s_clk_exit(void *clk_orig)
{
	struct sunxi_i2s_clk *clk = (struct sunxi_i2s_clk *)clk_orig;

	SND_LOG_DEBUG("\n");

	clk_put(clk->clk_i2s);
	clk_put(clk->clk_pll_audio_1x);
	clk_put(clk->clk_bus);

	kfree(clk);
}


int snd_i2s_clk_bus_enable(void *clk_orig)
{
	int ret = 0;
	struct sunxi_i2s_clk *clk = (struct sunxi_i2s_clk *)clk_orig;

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

int snd_i2s_clk_enable(void *clk_orig)
{
	int ret = 0;
	struct sunxi_i2s_clk *clk = (struct sunxi_i2s_clk *)clk_orig;

	SND_LOG_DEBUG("\n");

	if (clk_prepare_enable(clk->clk_pll_audio_1x)) {
		SND_LOG_ERR("clk_pll_audio_1x enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_pll_audio_1x;
	}

	if (clk_prepare_enable(clk->clk_i2s)) {
		SND_LOG_ERR("clk_i2s enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_i2s;
	}

	return 0;

err_enable_clk_i2s:
	clk_disable_unprepare(clk->clk_pll_audio_1x);
err_enable_clk_pll_audio_1x:
	return ret;
}


void snd_i2s_clk_bus_disable(void *clk_orig)
{
	struct sunxi_i2s_clk *clk = (struct sunxi_i2s_clk *)clk_orig;

	SND_LOG_DEBUG("\n");

	clk_disable_unprepare(clk->clk_bus);
	reset_control_assert(clk->clk_rst);
}

void snd_i2s_clk_disable(void *clk_orig)
{
	struct sunxi_i2s_clk *clk = (struct sunxi_i2s_clk *)clk_orig;

	SND_LOG_DEBUG("\n");

	clk_disable_unprepare(clk->clk_i2s);
	clk_disable_unprepare(clk->clk_pll_audio_1x);
}


int snd_i2s_clk_rate(void *clk_orig, unsigned int freq_in, unsigned int freq_out)
{
	struct sunxi_i2s_clk *clk = (struct sunxi_i2s_clk *)clk_orig;
	SND_LOG_DEBUG("\n");

	if (clk_set_parent(clk->clk_i2s, clk->clk_pll_audio_1x)) {
		SND_LOG_ERR("set dmic parent clk failed\n");
		return -EINVAL;
	}

	if (clk_set_rate(clk->clk_i2s, freq_out)) {
		SND_LOG_ERR("set clk_i2s rate failed, rate: %u\n", freq_out);
		return -EINVAL;
	}

	return 0;
}
