/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/* sunxi_tcon_v35x.c
 *
 * Copyright (C) 2023 Allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/pm_runtime.h>
#include <linux/dma-mapping.h>
#include <drm/drm_print.h>
#include <linux/component.h>

#include "sunxi_drm_drv.h"
#include "sunxi_tcon_top.h"

struct tcon_top_data {
	unsigned int id;
};

struct tcon_top {
	uintptr_t reg_base;
	struct clk *clk_dpss;
	struct clk *clk_ahb;
	struct clk *clk_ahb_gate;
	struct reset_control *rst_bus_dpss;
	struct reset_control *rst_bus_reg;
	const struct tcon_top_data *top_data;
	bool sw_enable;
};

static int sunxi_tcon_top_bind(struct device *dev, struct device *master,
			       void *data)
{
	struct tcon_top *top;
	struct resource *res;
	struct drm_device *drm = (struct drm_device *)data;
	struct platform_device *pdev = to_platform_device(dev);

	DRM_INFO("[TCON_TOP]%s start\n", __FUNCTION__);
	top = devm_kzalloc(dev, sizeof(*top), GFP_KERNEL);
	if (!top)
		return -ENOMEM;

	top->top_data = of_device_get_match_data(dev);
	if (!top->top_data) {
		DRM_ERROR("sunxi_tcon_top fail to get match data\n");
		return -ENODEV;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	top->reg_base = (uintptr_t)devm_ioremap_resource(dev, res);
	if (!top->reg_base) {
		DRM_ERROR("unable to map tcon top registers\n");
		return -EINVAL;
	}

	top->clk_dpss = devm_clk_get_optional(dev, "clk_bus_dpss_top");

	if (IS_ERR(top->clk_dpss)) {
		DRM_ERROR("fail to get clk dpss_top\n");
		return -EINVAL;
	}

	top->clk_ahb = devm_clk_get_optional(dev, "clk_ahb");

	if (IS_ERR(top->clk_ahb)) {
		DRM_ERROR("fail to get clk ahb %ld\n", PTR_ERR(top->clk_ahb));
		return -EINVAL;
	}

	top->clk_ahb_gate = devm_clk_get_optional(dev, "clk_ahb_gate");

	if (IS_ERR(top->clk_ahb_gate)) {
		DRM_ERROR("fail to get clk ahb gate\n");
		return -EINVAL;
	}

	top->rst_bus_dpss =
		devm_reset_control_get_optional_shared(dev, "rst_bus_dpss_top");
	if (IS_ERR(top->rst_bus_dpss)) {
		DRM_ERROR("fail to get reset rst_bus_dpss_top\n");
		return -EINVAL;
	}

	top->rst_bus_reg =
		devm_reset_control_get_optional_shared(dev, "rst_bus_reg");

	if (IS_ERR(top->rst_bus_reg)) {
		DRM_ERROR("fail to get reset rst_bus_reg\n");
		return -EINVAL;
	}

	top->sw_enable = sunxi_drm_check_tcon_top_boot_enabled(drm, top->top_data->id);
	/* sw_enable means the device is enabled in uboot, so set pm runtime to active */
	if (top->sw_enable) {
		pm_runtime_set_active(dev);
		pm_runtime_enable(dev);
	} else {
		pm_runtime_enable(dev);
	}

	tcon_top_set_reg_base(top->top_data->id, top->reg_base);
	dev_set_drvdata(dev, top);
	return 0;
}

static void sunxi_tcon_top_unbind(struct device *dev, struct device *master,
				  void *data)
{
	pm_runtime_disable(dev);
}

static const struct component_ops sunxi_tcon_top_component_ops = {
	.bind = sunxi_tcon_top_bind,
	.unbind = sunxi_tcon_top_unbind,
};

static int sunxi_tcon_top_probe(struct platform_device *pdev)
{
	DRM_INFO("[TCON_TOP]%s start\n", __FUNCTION__);
	return component_add(&pdev->dev, &sunxi_tcon_top_component_ops);
}

static int sunxi_tcon_top_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &sunxi_tcon_top_component_ops);
	return 0;
}

/* Note: sunxi-lcd is represented of sunxi tcon,
 * using lcd is order to be same with sunxi display driver
 */

static const struct tcon_top_data top0_data = {
	.id = 0,
};

static const struct tcon_top_data top1_data = {
	.id = 1,
};

static const struct of_device_id sunxi_tcon_top_match[] = {

	{ .compatible = "allwinner,tcon-top0", .data = &top0_data },
	{ .compatible = "allwinner,tcon-top1", .data = &top1_data },
	{},
};

struct platform_driver sunxi_tcon_top_platform_driver = {
	.probe = sunxi_tcon_top_probe,
	.remove = sunxi_tcon_top_remove,
	.driver = {
		   .name = "tcon-top",
		   .owner = THIS_MODULE,
		   .of_match_table = sunxi_tcon_top_match,
	},
};

static bool sunxi_tcon_top_node_is_tcon_top(struct device_node *node)
{
	return !!of_match_node(sunxi_tcon_top_match, node);
}

int sunxi_tcon_top_clk_enable(struct device *tcon_top)
{
	int ret;
	struct tcon_top *topif = dev_get_drvdata(tcon_top);

	if (!sunxi_tcon_top_node_is_tcon_top(tcon_top->of_node)) {
		DRM_ERROR("Device is not TCON TOP!\n");
		return -EINVAL;
	}

	ret = reset_control_deassert(topif->rst_bus_reg);
	if (ret) {
		DRM_ERROR("reset_control_deassert for rst_bus_reg failed!\n");
		return ret;
	}

	ret = reset_control_deassert(topif->rst_bus_dpss);
	if (ret) {
		DRM_ERROR("reset_control_deassert for rst_bus_dpss failed!\n");
		return ret;
	}

	ret = clk_prepare_enable(topif->clk_ahb_gate);
	if (ret != 0) {
		DRM_ERROR("fail enable topif's ahb gate clock!\n");
		return ret;
	}

	ret = clk_prepare_enable(topif->clk_ahb);
	if (ret != 0) {
		DRM_ERROR("fail enable topif's ahb clock!\n");
		return ret;
	}

	ret = clk_prepare_enable(topif->clk_dpss);
	if (ret != 0) {
		DRM_ERROR("fail enable topif's clock!\n");
		return ret;
	}

	pm_runtime_get_sync(tcon_top);

	return 0;
}

int sunxi_tcon_top_clk_disable(struct device *tcon_top)
{
//TODO FXIME

	int ret;
	struct tcon_top *topif = dev_get_drvdata(tcon_top);

	if (!sunxi_tcon_top_node_is_tcon_top(tcon_top->of_node)) {
		DRM_ERROR("Device is not TCON TOP!\n");
		return -EINVAL;
	}

	pm_runtime_put_sync(tcon_top);

	clk_disable_unprepare(topif->clk_dpss);
	clk_disable_unprepare(topif->clk_ahb);
	clk_disable_unprepare(topif->clk_ahb_gate);
	ret = reset_control_assert(topif->rst_bus_dpss);
	if (ret) {
		DRM_ERROR(
			"reset_control_deassert for rst_bus_if_top failed!\n");
		return ret;
	}
	ret = reset_control_assert(topif->rst_bus_reg);
	if (ret) {
		DRM_ERROR(
			"reset_control_deassert for rst_bus_reg failed!\n");
		return ret;
	}

	return 0;
}
