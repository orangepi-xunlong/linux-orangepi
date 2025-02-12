// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include "ky_dphy.h"
#include "sysfs/sysfs_display.h"

LIST_HEAD(dphy_core_head);

int ky_dphy_resume(struct ky_dphy *dphy)
{
	if (dphy->core && dphy->core->init)
		dphy->core->init(&dphy->ctx);

	DRM_DEBUG("dphy resume OK\n");
	return 0;
}

int ky_dphy_suspend(struct ky_dphy *dphy)
{
	if (dphy->core && dphy->core->uninit)
		dphy->core->uninit(&dphy->ctx);

	DRM_DEBUG("dphy suspend OK\n");
	return 0;
}

int ky_dphy_reset(struct ky_dphy *dphy)
{
	if (dphy->core && dphy->core->reset)
		dphy->core->reset(&dphy->ctx);

	DRM_DEBUG("dphy reset OK\n");
	return 0;
}

int ky_dphy_get_status(struct ky_dphy *dphy)
{
	if (dphy->core && dphy->core->get_status)
		dphy->core->get_status(&dphy->ctx);

	DRM_DEBUG("dphy get status OK\n");
	return 0;
}

static int ky_dphy_device_create(struct ky_dphy *dphy,
				   struct device *parent)
{
	int ret;

	dphy->dev.class = display_class;
	dphy->dev.parent = parent;
	dphy->dev.of_node = parent->of_node;
	dev_set_name(&dphy->dev, "dphy%d", dphy->ctx.id);
	dev_set_drvdata(&dphy->dev, dphy);

	ret = device_register(&dphy->dev);
	if (ret)
		DRM_ERROR("dphy device register failed\n");

	return ret;
}

static int ky_dphy_context_init(struct ky_dphy *dphy,
				  struct device_node *np)
{
	struct resource r;
	u32 tmp;

	if (dphy->core && dphy->core->parse_dt)
		dphy->core->parse_dt(&dphy->ctx, np);

	if (!of_address_to_resource(np, 0, &r)) {
		dphy->ctx.base_addr = (void __iomem *)ioremap(r.start, resource_size(&r));
		if (NULL == dphy->ctx.base_addr) {
			DRM_ERROR("dphy ctrlbase ioremap failed\n");
			return -EFAULT;
		}
	} else {
		DRM_ERROR("parse dphy ctrl reg base failed\n");
		return -EINVAL;
	}

	if (!of_property_read_u32(np, "dev-id", &tmp))
		dphy->ctx.id = tmp;

	return 0;
}

static int ky_dphy_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct ky_dphy *dphy;
	const char *str;
	int ret;

	DRM_DEBUG("%s()\n", __func__);

	dphy = devm_kzalloc(&pdev->dev, sizeof(*dphy), GFP_KERNEL);
	if (!dphy)
		return -ENOMEM;

	if (!of_property_read_string(np, "ip", &str))
		dphy->core = dphy_core_ops_attach(str);
	else
		DRM_WARN("dphy pll ops parse failed\n");

	ret = ky_dphy_context_init(dphy, pdev->dev.of_node);
	if (ret)
		return ret;

	ky_dphy_device_create(dphy, &pdev->dev);
	ky_dphy_sysfs_init(&dphy->dev);
	platform_set_drvdata(pdev, dphy);

	DRM_DEBUG("dphy driver probe success\n");

	return 0;
}


static const struct of_device_id dt_ids[] = {
	{ .compatible = "ky,dsi0-phy", },
	{ .compatible = "ky,dsi1-phy", },
	{ .compatible = "ky,dsi2-phy", },
	{},
};

struct platform_driver ky_dphy_driver = {
	.probe = ky_dphy_probe,
	.driver = {
		.name = "ky-dphy-drv",
		.of_match_table = of_match_ptr(dt_ids),
	}
};

//module_platform_driver(ky_dphy_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Ky MIPI DSI PHY driver");

