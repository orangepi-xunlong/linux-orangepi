/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2025 - 2028 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2025, zhouxijing <zhouxijing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/of_address.h>
#include <sound/soc.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_syscfg.h"
#include "snd_sunxi_common.h"

#define DRV_NAME	"sunxi-snd-syscfg"

#define	EDP_I2S_SEL_REG		0x023c

/* EDP_I2S_SEL_REG register */
#define	EDP_I2S_SEL		0
#define SYSCFG_REG_MAX  EDP_I2S_SEL

static struct audio_reg_label sunxi_reg_labels[] = {
	REG_LABEL(EDP_I2S_SEL_REG),
};
static struct audio_reg_group sunxi_reg_group = REG_GROUP(sunxi_reg_labels);

static struct regmap_config g_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SYSCFG_REG_MAX,
	.cache_type = REGCACHE_NONE,
};

static const char *edp_src_text[] = {"I2S3", "I2S4"};
static SOC_ENUM_SINGLE_DECL(edp_src_enum, EDP_I2S_SEL_REG, EDP_I2S_SEL, edp_src_text);

static const struct snd_kcontrol_new sunxi_syscfg_edp_src_controls[] = {
	SOC_ENUM("EDP SRC SEL", edp_src_enum),
};

static int sunxi_add_edp_src_controls(struct snd_soc_component *component)
{
	int ret;

	SND_LOG_DEBUG("\n");

	if (!component) {
		SND_LOG_ERR("component invalid\n");
		return -1;
	}

	ret = snd_soc_add_component_controls(component, sunxi_syscfg_edp_src_controls,
					     ARRAY_SIZE(sunxi_syscfg_edp_src_controls));
	if (ret) {
		SND_LOG_ERR("add rx_raw kcontrols failed\n");
		return -1;
	}

	return 0;
}

int snd_sunxi_mem_init(struct platform_device *pdev, struct sunxi_syscfg_mem *mem)
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

	mem->regmap = devm_regmap_init_mmio(&pdev->dev, mem->membase, &g_regmap_config);
	if (IS_ERR_OR_NULL(mem->regmap)) {
		SND_LOG_ERR("regmap init failed\n");
		ret = -EINVAL;
		goto err_devm_regmap_init;
	}

	return 0;

err_devm_regmap_init:
	devm_iounmap(&pdev->dev, mem->membase);
err_devm_ioremap:
	devm_release_mem_region(&pdev->dev, mem->memregion->start,
				resource_size(mem->memregion));
err_devm_request_region:
err_of_addr_to_resource:
	return ret;
}

void snd_sunxi_mem_exit(struct platform_device *pdev, struct sunxi_syscfg_mem *mem)
{
	SND_LOG_DEBUG("\n");

	devm_iounmap(&pdev->dev, mem->membase);
	devm_release_mem_region(&pdev->dev, mem->memregion->start,
				resource_size(mem->memregion));
}

int sunxi_syscfg_probe(struct snd_soc_component *component)
{
	int ret;

	ret = sunxi_add_edp_src_controls(component);
	if (ret) {
		SND_LOG_ERR("add rx src kcontrols failed\n");
		return -EINVAL;
	}

	return 0;
}

void sunxi_syscfg_remove(struct snd_soc_component *component)
{
    (void)component;
}

int sunxi_syscfg_suspend(struct snd_soc_component *component)
{
	struct sunxi_syscfg_mem *mem = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = mem->regmap;

	SND_LOG_DEBUG("\n");

	snd_sunxi_save_reg(regmap, &sunxi_reg_group);

	return 0;
}

int sunxi_syscfg_resume(struct snd_soc_component *component)
{
	struct sunxi_syscfg_mem *mem = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = mem->regmap;

	SND_LOG_DEBUG("\n");

	snd_sunxi_echo_reg(regmap, &sunxi_reg_group);

	return 0;
}
