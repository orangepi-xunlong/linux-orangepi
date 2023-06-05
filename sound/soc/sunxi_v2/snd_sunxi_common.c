/*
 * sound\soc\sunxi\snd_sunxi_common.c
 * (C) Copyright 2021-2025
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

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

#include "snd_sunxi_log.h"
#include "snd_sunxi_common.h"

#define HLOG		"COMMON"

/* for regmap */
int snd_sunxi_mem_init(struct platform_device *pdev,
		       struct sunxi_mem_info *mem_info)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	ret = of_address_to_resource(np, 0, mem_info->res);
	if (ret) {
		SND_LOG_ERR(HLOG, "parse device node resource failed\n");
		ret = -EINVAL;
		goto err_of_addr_to_resource;
	}

	mem_info->memregion = devm_request_mem_region(&pdev->dev,
					mem_info->res->start,
					resource_size(mem_info->res),
					mem_info->dev_name);
	if (IS_ERR_OR_NULL(mem_info->memregion)) {
		SND_LOG_ERR(HLOG, "memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_request_region;
	}

	mem_info->membase = devm_ioremap(&pdev->dev,
					 mem_info->memregion->start,
					 resource_size(mem_info->memregion));
	if (IS_ERR_OR_NULL(mem_info->membase)) {
		SND_LOG_ERR(HLOG, "ioremap failed\n");
		ret = -EBUSY;
		goto err_devm_ioremap;
	}

	mem_info->regmap = devm_regmap_init_mmio(&pdev->dev,
						 mem_info->membase,
						 mem_info->regmap_config);
	if (IS_ERR_OR_NULL(mem_info->regmap)) {
		SND_LOG_ERR(HLOG, "regmap init failed\n");
		ret = -EINVAL;
		goto err_devm_regmap_init;
	}

	return 0;

err_devm_regmap_init:
	devm_iounmap(&pdev->dev, mem_info->membase);
err_devm_ioremap:
	devm_release_mem_region(&pdev->dev, mem_info->memregion->start,
				resource_size(mem_info->memregion));
err_devm_request_region:
err_of_addr_to_resource:
	return ret;
}

void snd_sunxi_mem_exit(struct platform_device *pdev,
			struct sunxi_mem_info *mem_info)
{
	SND_LOG_DEBUG(HLOG, "\n");

	devm_iounmap(&pdev->dev, mem_info->membase);
	devm_release_mem_region(&pdev->dev, mem_info->memregion->start,
				resource_size(mem_info->memregion));
}

/* for reg labels */
int snd_sunxi_save_reg(struct regmap *regmap, struct reg_label *reg_labels)
{
	int i = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	while (reg_labels[i].name != NULL) {
		regmap_read(regmap,
			    reg_labels[i].address, &(reg_labels[i].value));
		i++;
	}

	return i;
}

int snd_sunxi_echo_reg(struct regmap *regmap, struct reg_label *reg_labels)
{
	int i = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	while (reg_labels[i].name != NULL) {
		regmap_write(regmap,
			     reg_labels[i].address, reg_labels[i].value);
		i++;
	}

	return i;
}

/* for pa config */
struct pa_config *snd_sunxi_pa_pin_init(struct platform_device *pdev,
					u32 *pa_pin_max)
{
	int ret, i;
	u32 pin_max;
	u32 gpio_tmp;
	u32 temp_val;
	char str[20] = {0};
	struct pa_config *pa_cfg;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	*pa_pin_max = 0;
	ret = of_property_read_u32(np, "pa_pin_max", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN(HLOG, "pa_pin_max get failed, default 0\n");
		return NULL;
	} else {
		pin_max = temp_val;
	}

	pa_cfg = kzalloc(sizeof(struct pa_config) * pin_max, GFP_KERNEL);
	if (!pa_cfg) {
		SND_LOG_ERR(HLOG, "can't pa_config memory\n");
		return NULL;
	}

	for (i = 0; i < pin_max; i++) {
		sprintf(str, "pa_pin_%d", i);
		ret = of_get_named_gpio(np, str, 0);
		if (ret < 0) {
			SND_LOG_ERR(HLOG, "pa_pin_%u get failed\n", i);
			pa_cfg[i].used = 0;
			continue;
		}
		gpio_tmp = ret;
		if (!gpio_is_valid(gpio_tmp)) {
			SND_LOG_ERR(HLOG, "pa_pin_%u (%u) is invalid\n",
				    i, gpio_tmp);
			pa_cfg[i].used = 0;
			continue;
		}
		ret = devm_gpio_request(&pdev->dev, gpio_tmp, str);
		if (ret) {
			SND_LOG_ERR(HLOG, "pa_pin_%u (%u) request failed\n",
				    i, gpio_tmp);
			pa_cfg[i].used = 0;
			continue;
		}
		pa_cfg[i].used = 1;
		pa_cfg[i].pin = gpio_tmp;

		sprintf(str, "pa_pin_level_%d", i);
		ret = of_property_read_u32(np, str, &temp_val);
		if (ret < 0) {
			SND_LOG_WARN(HLOG, "%s get failed, default low\n", str);
			pa_cfg[i].level = 0;
		} else {
			if (temp_val > 0)
				pa_cfg[i].level = 1;
		}
		sprintf(str, "pa_pin_msleep_%d", i);
		ret = of_property_read_u32(np, str, &temp_val);
		if (ret < 0) {
			SND_LOG_WARN(HLOG, "%s get failed, default 0\n", str);
			pa_cfg[i].msleep = 0;
		} else {
			pa_cfg[i].msleep = temp_val;
		}
	}

	*pa_pin_max = pin_max;
	snd_sunxi_pa_pin_disable(pa_cfg, pin_max);

	return pa_cfg;
}

void snd_sunxi_pa_pin_exit(struct platform_device *pdev,
			   struct pa_config *pa_cfg, u32 pa_pin_max)
{
	int i;

	SND_LOG_DEBUG(HLOG, "\n");

	snd_sunxi_pa_pin_disable(pa_cfg, pa_pin_max);

	for (i = 0; i < pa_pin_max; i++) {
		if (!pa_cfg[i].used)
			continue;

		gpio_free(pa_cfg[i].pin);
	}

	if (pa_cfg)
		kfree(pa_cfg);
}

int snd_sunxi_pa_pin_enable(struct pa_config *pa_cfg, u32 pa_pin_max)
{
	int i;

	SND_LOG_DEBUG(HLOG, "\n");

	if (pa_pin_max < 1) {
		SND_LOG_DEBUG(HLOG, "no pa pin config\n");
		return 0;
	}

	for (i = 0; i < pa_pin_max; i++) {
		if (!pa_cfg[i].used)
			continue;

		gpio_direction_output(pa_cfg[i].pin, 1);
		gpio_set_value(pa_cfg[i].pin, pa_cfg[i].level);
	}

	return 0;
}

void snd_sunxi_pa_pin_disable(struct pa_config *pa_cfg, u32 pa_pin_max)
{
	int i;

	SND_LOG_DEBUG(HLOG, "\n");

	if (pa_pin_max < 1) {
		SND_LOG_DEBUG(HLOG, "no pa pin config\n");
		return;
	}

	for (i = 0; i < pa_pin_max; i++) {
		if (!pa_cfg[i].used)
			continue;

		gpio_direction_output(pa_cfg[i].pin, 1);
		gpio_set_value(pa_cfg[i].pin, !pa_cfg[i].level);
	}
}
