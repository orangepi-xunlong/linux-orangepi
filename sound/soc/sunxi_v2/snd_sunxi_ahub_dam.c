/*
 * sound\soc\sunxi\snd_sunxi_ahub_dam.c
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
#include <sound/soc.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/regmap.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <sound/soc.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_ahub_dam.h"

#define HLOG		"AHUB_DAM"
#define DRV_NAME	"sunxi-snd-plat-ahub_dam"

static struct resource g_res;
struct sunxi_ahub_mem_info g_mem_info = {
	.res = &g_res,
};
static struct sunxi_ahub_clk_info g_clk_info;
static struct regmap_config g_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_AHUB_MAX_REG,
	.cache_type = REGCACHE_NONE,
};

static struct snd_soc_dai_driver sunxi_ahub_dam_dai = {
	.name           = "ahub_dam",
};

static int sunxi_ahub_dam_probe(struct snd_soc_component *component)
{
	SND_LOG_DEBUG(HLOG, "\n");

	return 0;
}

static int sunxi_ahub_dam_suspend(struct snd_soc_component *component)
{
	struct sunxi_ahub_clk_info *clk_info = &g_clk_info;

	SND_LOG_DEBUG(HLOG, "\n");

	clk_disable_unprepare(clk_info->clk_module);
	clk_disable_unprepare(clk_info->clk_pll);
	//clk_disable_unprepare(clk_info->clk_pllx4);
	clk_disable_unprepare(clk_info->clk_bus);
	reset_control_assert(clk_info->clk_rst);

	return 0;
}

static int sunxi_ahub_dam_resume(struct snd_soc_component *component)
{
	struct sunxi_ahub_clk_info *clk_info = &g_clk_info;

	SND_LOG_DEBUG(HLOG, "\n");

	if (reset_control_deassert(clk_info->clk_rst)) {
		SND_LOG_ERR(HLOG, "clk rst deassert failed\n");
		return -EINVAL;
	}
	if (clk_prepare_enable(clk_info->clk_bus)) {
		SND_LOG_ERR(HLOG, "clk bus enable failed\n");
		return -EBUSY;
	}
	if (clk_prepare_enable(clk_info->clk_pll)) {
		SND_LOG_ERR(HLOG, "clk_pll enable failed\n");
		return -EBUSY;
	}
	//if (clk_prepare_enable(clk_info->clk_pllx4)) {
	//	SND_LOG_ERR(HLOG, "clk_pllx4 enable failed\n");
	//	return -EBUSY;
	//}
	if (clk_prepare_enable(clk_info->clk_module)) {
		SND_LOG_ERR(HLOG, "clk_module enable failed\n");
		return -EBUSY;
	}

	return 0;
}

struct str_conv {
	char *str;
	unsigned int reg;
};
static struct str_conv ahub_mux_name[] = {
	{"APBIF0 Src Select",	SUNXI_AHUB_APBIF_RXFIFO_CONT(0)},
	{"APBIF1 Src Select",	SUNXI_AHUB_APBIF_RXFIFO_CONT(1)},
	{"APBIF2 Src Select",	SUNXI_AHUB_APBIF_RXFIFO_CONT(2)},
	{"I2S0 Src Select",	SUNXI_AHUB_I2S_RXCONT(0)},
	{"I2S1 Src Select",	SUNXI_AHUB_I2S_RXCONT(1)},
	{"I2S2 Src Select",	SUNXI_AHUB_I2S_RXCONT(2)},
	{"I2S3 Src Select",	SUNXI_AHUB_I2S_RXCONT(3)},
	{"DAM0C0 Src Select",	SUNXI_AHUB_DAM_RX0_SRC(0)},
	{"DAM0C1 Src Select",	SUNXI_AHUB_DAM_RX1_SRC(0)},
	{"DAM0C2 Src Select",	SUNXI_AHUB_DAM_RX2_SRC(0)},
	{"DAM1C0 Src Select",	SUNXI_AHUB_DAM_RX0_SRC(1)},
	{"DAM1C1 Src Select",	SUNXI_AHUB_DAM_RX1_SRC(1)},
	{"DAM1C2 Src Select",	SUNXI_AHUB_DAM_RX2_SRC(1)},
};
static const char *ahub_mux_text[] = {
	"NONE",
	"APBIF_TXDIF0",
	"APBIF_TXDIF1",
	"APBIF_TXDIF2",
	"I2S0_TXDIF",
	"I2S1_TXDIF",
	"I2S2_TXDIF",
	"I2S3_TXDIF",
	"DAM0_TXDIF",
	"DAM1_TXDIF",
};
static const unsigned int ahub_mux_values[] = {
	0,
	1 << I2S_RX_APBIF_TXDIF0,
	1 << I2S_RX_APBIF_TXDIF1,
	1 << I2S_RX_APBIF_TXDIF2,
	1 << I2S_RX_I2S0_TXDIF,
	1 << I2S_RX_I2S1_TXDIF,
	1 << I2S_RX_I2S2_TXDIF,
	1 << I2S_RX_I2S3_TXDIF,
	1 << I2S_RX_DAM0_TXDIF,
	1 << I2S_RX_DAM1_TXDIF,
};
static SOC_ENUM_SINGLE_EXT_DECL(ahub_mux, ahub_mux_text);

static int sunxi_ahub_mux_get(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	int i;
	unsigned int reg_val;
	unsigned int src_reg;
	struct regmap *regmap = g_mem_info.regmap;

	for (i = 0; i < ARRAY_SIZE(ahub_mux_name); i++) {
		if (!strncmp(ahub_mux_name[i].str, kcontrol->id.name,
			     strlen(ahub_mux_name[i].str))) {
			src_reg = ahub_mux_name[i].reg;
			regmap_read(regmap, src_reg, &reg_val);
			reg_val &= 0xffffc000;
			break;
		}
	}

	for (i = 1; i < ARRAY_SIZE(ahub_mux_values); i++) {
		if (reg_val & ahub_mux_values[i]) {
			ucontrol->value.integer.value[0] = i;
			return 0;
		}
	}
	ucontrol->value.integer.value[0] = 0;

	return 0;
}

static int sunxi_ahub_mux_set(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	int i;
	unsigned int src_reg, src_regbit;
	struct regmap *regmap = g_mem_info.regmap;

	if (ucontrol->value.integer.value[0] > ARRAY_SIZE(ahub_mux_name))
		return -EINVAL;

	src_regbit = ahub_mux_values[ucontrol->value.integer.value[0]];
	for (i = 0; i < ARRAY_SIZE(ahub_mux_name); i++) {
		if (!strncmp(ahub_mux_name[i].str, kcontrol->id.name,
			     strlen(ahub_mux_name[i].str))) {
			src_reg = ahub_mux_name[i].reg;
			regmap_update_bits(regmap, src_reg, 0xffffc000, src_regbit);
			break;
		}
	}

	return 0;
}

static const struct snd_kcontrol_new sunxi_ahub_dam_controls[] = {
	SOC_ENUM_EXT("APBIF0 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("APBIF1 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("APBIF2 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("I2S0 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("I2S1 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("I2S2 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("I2S3 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("DAM0C0 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("DAM0C1 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("DAM0C2 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("DAM1C0 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("DAM1C1 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("DAM1C2 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
};

static struct snd_soc_component_driver sunxi_ahub_dam_dev = {
	.name		= DRV_NAME,
	.probe		= sunxi_ahub_dam_probe,
	.suspend	= sunxi_ahub_dam_suspend,
	.resume		= sunxi_ahub_dam_resume,
	.controls	= sunxi_ahub_dam_controls,
	.num_controls	= ARRAY_SIZE(sunxi_ahub_dam_controls),
};

/*******************************************************************************
 * for kernel source
 ******************************************************************************/
int snd_soc_sunxi_ahub_mem_get(struct sunxi_ahub_mem_info *mem_info)
{
	SND_LOG_DEBUG(HLOG, "\n");

	if (IS_ERR_OR_NULL(g_mem_info.regmap)) {
		SND_LOG_ERR(HLOG, "regmap is invalid\n");
		return -EINVAL;
	}
	if (IS_ERR_OR_NULL(g_mem_info.res)) {
		SND_LOG_ERR(HLOG, "res is invalid\n");
		return -EINVAL;
	}

	mem_info->regmap = g_mem_info.regmap;
	mem_info->res = g_mem_info.res;

	return 0;
}
EXPORT_SYMBOL_GPL(snd_soc_sunxi_ahub_mem_get);

int snd_soc_sunxi_ahub_clk_get(struct sunxi_ahub_clk_info *clk_info)
{
	SND_LOG_DEBUG(HLOG, "\n");

	if (IS_ERR_OR_NULL(g_clk_info.clk_pll)) {
		SND_LOG_ERR(HLOG, "clk_pll is invalid\n");
		return -EINVAL;
	}
	//if (IS_ERR_OR_NULL(g_clk_info.clk_pllx4)) {
	//	SND_LOG_ERR(HLOG, "clk_pllx4 is invalid\n");
	//	return -EINVAL;
	//}
	if (IS_ERR_OR_NULL(g_clk_info.clk_module)) {
		SND_LOG_ERR(HLOG, "clk_module is invalid\n");
		return -EINVAL;
	}

	clk_info->clk_pll = g_clk_info.clk_pll;
	//clk_info->clk_pllx4 = g_clk_info.clk_pllx4;
	clk_info->clk_module = g_clk_info.clk_module;

	return 0;
}
EXPORT_SYMBOL_GPL(snd_soc_sunxi_ahub_clk_get);

static int snd_soc_sunxi_ahub_mem_init(struct platform_device *pdev,
				       struct device_node *np,
				       struct sunxi_ahub_mem_info *mem_info)
{
	int ret = 0;

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
					DRV_NAME);
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
						 &g_regmap_config);
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
};

static int snd_soc_sunxi_ahub_clk_init(struct platform_device *pdev,
				       struct device_node *np,
				       struct sunxi_ahub_clk_info *clk_info)
{
	int ret = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	/* deassert rst clk */
	clk_info->clk_rst = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR_OR_NULL(clk_info->clk_rst)) {
		SND_LOG_ERR(HLOG, "clk rst get failed\n");
		ret = -EBUSY;
		goto err_rst_clk;
	}
	if (reset_control_deassert(clk_info->clk_rst)) {
		SND_LOG_ERR(HLOG, "deassert reset clk failed\n");
		ret = -EBUSY;
		goto err_rst_clk;
	}

	/* enable ahub bus clk */
	clk_info->clk_bus = of_clk_get_by_name(np, "clk_bus_audio_hub");
	if (IS_ERR_OR_NULL(clk_info->clk_bus)) {
		SND_LOG_ERR(HLOG, "clk bus get failed\n");
		ret = -EBUSY;
		goto err_bus_clk;
	}
	if (clk_prepare_enable(clk_info->clk_bus)) {
		SND_LOG_ERR(HLOG, "ahub clk bus enable failed\n");
		ret = -EBUSY;
		goto err_bus_clk;
	}

	/* get clk of ahub */
	clk_info->clk_module = of_clk_get_by_name(np, "clk_audio_hub");
	if (IS_ERR_OR_NULL(clk_info->clk_module)) {
		SND_LOG_ERR(HLOG, "clk module get failed\n");
		ret = -EBUSY;
		goto err_module_clk;
	}
	clk_info->clk_pll = of_clk_get_by_name(np, "clk_pll_audio");
	if (IS_ERR_OR_NULL(clk_info->clk_pll)) {
		SND_LOG_ERR(HLOG, "clk pll get failed\n");
		ret = -EBUSY;
		goto err_pll_clk;
	}
	//clk_info->clk_pllx4 = of_clk_get_by_name(np, "clk_pll_audio_4x");
	//if (IS_ERR_OR_NULL(clk_info->clk_pllx4)) {
	//	SND_LOG_ERR(HLOG, "clk pllx4 get failed\n");
	//	ret = -EBUSY;
	//	goto err_pllx4_clk;
	//}

	/* set ahub clk parent */
	//if (clk_set_parent(clk_info->clk_module, clk_info->clk_pllx4)) {
	//	SND_LOG_ERR(HLOG, "set parent of clk_module to pllx4 failed\n");
	//	ret = -EINVAL;
	//	goto err_set_parent_clk;
	//}

	/* enable clk of ahub */
	if (clk_prepare_enable(clk_info->clk_pll)) {
		SND_LOG_ERR(HLOG, "clk_pll enable failed\n");
		ret = -EBUSY;
		goto err_pll_clk_enable;
	}
	//if (clk_prepare_enable(clk_info->clk_pllx4)) {
	//	SND_LOG_ERR(HLOG, "clk_pllx4 enable failed\n");
	//	ret = -EBUSY;
	//	goto err_pllx4_clk_enable;
	//}
	if (clk_prepare_enable(clk_info->clk_module)) {
		SND_LOG_ERR(HLOG, "clk_module enable failed\n");
		ret = -EBUSY;
		goto err_module_clk_enable;
	}

	return 0;

err_module_clk_enable:
//	clk_disable_unprepare(clk_info->clk_pllx4);
//err_pllx4_clk_enable:
	clk_disable_unprepare(clk_info->clk_pll);
err_pll_clk_enable:
//err_set_parent_clk:
//	clk_put(clk_info->clk_pllx4);
//err_pllx4_clk:
//	clk_put(clk_info->clk_pll);
err_pll_clk:
	clk_put(clk_info->clk_module);
err_module_clk:
	clk_disable_unprepare(clk_info->clk_bus);
	clk_put(clk_info->clk_bus);
err_bus_clk:
	reset_control_assert(clk_info->clk_rst);
err_rst_clk:
	return ret;
}

static int sunxi_ahub_dam_dev_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	ret = snd_soc_sunxi_ahub_mem_init(pdev, np, &g_mem_info);
	if (ret) {
		SND_LOG_ERR(HLOG, "remap init failed\n");
		ret = -EINVAL;
		goto err_snd_soc_sunxi_ahub_mem_init;
	}

	ret = snd_soc_sunxi_ahub_clk_init(pdev, np, &g_clk_info);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk init failed\n");
		ret = -EINVAL;
		goto err_snd_soc_sunxi_ahub_clk_init;
	}

	ret = snd_soc_register_component(&pdev->dev,
					 &sunxi_ahub_dam_dev,
					 &sunxi_ahub_dam_dai, 1);
	if (ret) {
		SND_LOG_ERR(HLOG, "component register failed\n");
		ret = -ENOMEM;
		goto err_snd_soc_register_component;
	}

	SND_LOG_DEBUG(HLOG, "register ahub_dam platform success\n");

	return 0;

err_snd_soc_register_component:
err_snd_soc_sunxi_ahub_clk_init:
err_snd_soc_sunxi_ahub_mem_init:
	of_node_put(np);
	return ret;
}

static int sunxi_ahub_dam_dev_remove(struct platform_device *pdev)
{
	struct sunxi_ahub_mem_info *mem_info = &g_mem_info;
	struct sunxi_ahub_clk_info *clk_info = &g_clk_info;

	SND_LOG_DEBUG(HLOG, "\n");

	snd_soc_unregister_component(&pdev->dev);

	devm_iounmap(&pdev->dev, mem_info->membase);
	devm_release_mem_region(&pdev->dev, mem_info->memregion->start,
				resource_size(mem_info->memregion));

	clk_disable_unprepare(clk_info->clk_module);
	clk_put(clk_info->clk_module);
	clk_disable_unprepare(clk_info->clk_pll);
	clk_put(clk_info->clk_pll);
	//clk_disable_unprepare(clk_info->clk_pllx4);
	//clk_put(clk_info->clk_pllx4);
	clk_disable_unprepare(clk_info->clk_bus);
	clk_put(clk_info->clk_bus);
	reset_control_assert(clk_info->clk_rst);

	SND_LOG_DEBUG(HLOG, "unregister ahub_dam platform success\n");

	return 0;
}

static const struct of_device_id sunxi_ahub_dam_of_match[] = {
	{ .compatible = "allwinner," DRV_NAME, },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_ahub_dam_of_match);

static struct platform_driver sunxi_ahub_dam_driver = {
	.driver	= {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= sunxi_ahub_dam_of_match,
	},
	.probe	= sunxi_ahub_dam_dev_probe,
	.remove	= sunxi_ahub_dam_dev_remove,
};

int __init sunxi_ahub_dam_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_ahub_dam_driver);
	if (ret != 0) {
		SND_LOG_ERR(HLOG, "platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_ahub_dam_dev_exit(void)
{
	platform_driver_unregister(&sunxi_ahub_dam_driver);
}

late_initcall(sunxi_ahub_dam_dev_init);
module_exit(sunxi_ahub_dam_dev_exit);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sunxi soundcard platform of ahub_dam");
