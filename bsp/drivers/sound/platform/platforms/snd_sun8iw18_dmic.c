/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2022, lijingpsw <lijingpsw@allwinnertech.com>
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
#include "snd_sunxi_dmic.h"

struct sunxi_dmic_clk {
	struct clk *clk_pll_audio;
	struct clk *clk_dmic;
	struct clk *clk_bus;
	struct reset_control *clk_rst;
};

sunxi_dmic_clk_t *snd_dmic_clk_init(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_dmic_clk *clk = NULL;

	SND_LOG_DEBUG("\n");

	clk = kzalloc(sizeof(*clk), GFP_KERNEL);
	if (!clk) {
		SND_LOG_ERR("can't allocate sunxi_dmic_clk memory\n");
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
	clk->clk_bus = of_clk_get_by_name(np, "clk_bus_dmic");
	if (IS_ERR_OR_NULL(clk->clk_bus)) {
		SND_LOG_ERR("clk bus get failed\n");
		ret = PTR_ERR(clk->clk_bus);
		goto err_get_clk_bus;
	}

	/* get dmic clk */
	clk->clk_dmic = of_clk_get_by_name(np, "clk_dmic");
	if (IS_ERR_OR_NULL(clk->clk_dmic)) {
		SND_LOG_ERR("clk dmic get failed\n");
		ret = PTR_ERR(clk->clk_dmic);
		goto err_get_clk_dmic;
	}

	/* get parent clk */
	clk->clk_pll_audio = of_clk_get_by_name(np, "clk_pll_audio_4x");
	if (IS_ERR_OR_NULL(clk->clk_pll_audio)) {
		SND_LOG_ERR("clk_pll_audio get failed\n");
		ret = PTR_ERR(clk->clk_pll_audio);
		goto err_get_clk_pll_audio;
	}

	/* set clk dmic parent of clk_parent */
	if (clk_set_parent(clk->clk_dmic, clk->clk_pll_audio)) {
		SND_LOG_ERR("set parent clk dmic failed\n");
		ret = -EINVAL;
		goto err_set_parent;
	}

	return clk;

err_set_parent:
	clk_put(clk->clk_dmic);
err_get_clk_dmic:
	clk_put(clk->clk_pll_audio);
err_get_clk_pll_audio:
	clk_put(clk->clk_bus);
err_get_clk_bus:
err_get_clk_rst:
	kfree(clk);
	return NULL;
}

void snd_dmic_clk_exit(void *clk_orig)
{
	struct sunxi_dmic_clk *clk = (struct sunxi_dmic_clk *)clk_orig;

	SND_LOG_DEBUG("\n");

	clk_put(clk->clk_dmic);
	clk_put(clk->clk_pll_audio);
	clk_put(clk->clk_bus);

	kfree(clk);
}

int snd_dmic_clk_bus_enable(void *clk_orig)
{
	int ret = 0;
	struct sunxi_dmic_clk *clk = (struct sunxi_dmic_clk *)clk_orig;

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

int snd_dmic_clk_enable(void *clk_orig)
{
	int ret = 0;
	struct sunxi_dmic_clk *clk = (struct sunxi_dmic_clk *)clk_orig;

	SND_LOG_DEBUG("\n");

	if (clk_prepare_enable(clk->clk_pll_audio)) {
		SND_LOG_ERR("clk_parent enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_parent;
	}

	if (clk_prepare_enable(clk->clk_dmic)) {
		SND_LOG_ERR("clk_dmic enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_dmic;
	}

	return 0;

err_enable_clk_dmic:
	clk_disable_unprepare(clk->clk_pll_audio);
err_enable_clk_parent:
	return ret;
}

void snd_dmic_clk_bus_disable(void *clk_orig)
{
	struct sunxi_dmic_clk *clk = (struct sunxi_dmic_clk *)clk_orig;

	SND_LOG_DEBUG("\n");

	clk_disable_unprepare(clk->clk_bus);
	reset_control_assert(clk->clk_rst);
}

void snd_dmic_clk_disable(void *clk_orig)
{
	struct sunxi_dmic_clk *clk = (struct sunxi_dmic_clk *)clk_orig;

	SND_LOG_DEBUG("\n");

	clk_disable_unprepare(clk->clk_dmic);
	clk_disable_unprepare(clk->clk_pll_audio);
}

int snd_dmic_clk_rate(void *clk_orig, unsigned int freq_in, unsigned int freq_out)
{
	struct sunxi_dmic_clk *clk = (struct sunxi_dmic_clk *)clk_orig;

	SND_LOG_DEBUG("\n");

	if (clk_set_rate(clk->clk_dmic, freq_out)) {
		SND_LOG_ERR("freq : %u module clk unsupport\n", freq_out);
		return -EINVAL;
	}

	return 0;
}
