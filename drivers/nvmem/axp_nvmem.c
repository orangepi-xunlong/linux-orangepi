// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2020 frank@allwinnertech.com
 */

#include <linux/device.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/nvmem-provider.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/regmap.h>

#include <linux/mfd/axp2101.h>

struct axp_nvmem {
	struct regmap		*regmap;
};

static int axp_nvmem_read(void *context, unsigned int offset,
			  void *val, size_t bytes)
{
	struct axp_nvmem *axp = context;
	u8 *buf = val;
	unsigned int reg_val;

	/* map to 0x1XX */
	regmap_write(axp->regmap, 0xff, 1);

	while (bytes--) {
		regmap_read(axp->regmap, offset++, &reg_val);
		*buf++ = reg_val;
	}

	/* ummap to 0x1XX */
	regmap_write(axp->regmap, 0xff, 0);

	return 0;
}

static int axp_nvmem_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct nvmem_config *nvmem_cfg;
	struct nvmem_device *nvmem;
	struct axp_nvmem *axp;
	struct axp20x_dev *axp20x_dev = dev_get_drvdata(pdev->dev.parent);

	axp = devm_kzalloc(dev, sizeof(*axp), GFP_KERNEL);
	if (!axp)
		return -ENOMEM;

	axp->regmap = axp20x_dev->regmap;
	if (IS_ERR(axp->regmap))
		return PTR_ERR(axp->regmap);

	nvmem_cfg = devm_kzalloc(dev, sizeof(*nvmem_cfg), GFP_KERNEL);
	if (!nvmem_cfg)
		return -ENOMEM;

	nvmem_cfg->dev = dev;
	nvmem_cfg->name = "axp-nvmem";
	nvmem_cfg->read_only = true;
	nvmem_cfg->size = 0xff;
	nvmem_cfg->word_size = 1;
	nvmem_cfg->stride = 1;
	nvmem_cfg->priv = axp;
	nvmem_cfg->reg_read = axp_nvmem_read;

	nvmem = nvmem_register(nvmem_cfg);
	if (IS_ERR(nvmem))
		return PTR_ERR(nvmem);

	platform_set_drvdata(pdev, nvmem);

	return 0;
}

static const struct of_device_id axp_nvmem_match[] = {
	{ .compatible = "x-powers,axp806-nvmem", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, axp_nvmem_match);

static struct platform_driver axp_nvmem_driver = {
	.probe = axp_nvmem_probe,
	.driver = {
		.name = "x-powers-nvmem",
		.of_match_table = axp_nvmem_match,
	},
};
module_platform_driver(axp_nvmem_driver);

MODULE_LICENSE("GPL");
