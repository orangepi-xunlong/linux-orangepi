// SPDX-License-Identifier: GPL-2.0-only
/*
 * Ky eFuse Driver
 *
 * Copyright (c) 2024 Ky Co. Ltd.
 */

#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/nvmem-provider.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>


struct ky_efuse_bank {
	struct device *dev;
	void __iomem *base;
	struct clk *clk;
	struct reset_control *reset;
	struct nvmem_device *nvmem;
	struct nvmem_config *econfig;
	u8 *efuse_data;
	u32 size;
};


/*
 * read efuse data to buffer for x1 soc.
 */
static int ky_x1_efuse_read(struct ky_efuse_bank *efuse)
{
	int i, ret;
	u32 *buffer;

	ret = clk_prepare_enable(efuse->clk);
	if (ret < 0) {
		dev_err(efuse->dev, "failed to prepare/enable efuse clk\n");
		return ret;
	}
	ret = reset_control_deassert(efuse->reset);
	if (ret < 0) {
		dev_err(efuse->dev, "failed to deassert efuse\n");
		clk_disable_unprepare(efuse->clk);
		return ret;
	}

	/*
	 * efuse data has been load into register by uboot already,
	 * just get efuse data from register
	 */
	buffer = (u32 *)efuse->efuse_data;
	for (i = 0; i < efuse->size/sizeof(u32); i++) {
		buffer[i] = readl(efuse->base + i*4);
	}

	reset_control_assert(efuse->reset);
	clk_disable_unprepare(efuse->clk);

	return ret;
}


/*
 * call-back function, just read data from buffer
 */
static int ky_efuse_read(void *context, unsigned int offset,
				      void *val, size_t bytes)
{
	int i;
	u8 *buf = (u8 *)val;
	struct ky_efuse_bank *efuse = context;

	/* check if data request is out of bound */
	for(i=0; i<bytes; i++) {
		buf[i] = efuse->efuse_data[offset + i];
	}

	return 0;
}

static int ky_efuse_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *res;
	struct nvmem_device *nvmem;
	struct nvmem_config *econfig;
	struct ky_efuse_bank *efuse;
	struct device *dev = &pdev->dev;
	int (*efuse_read)(struct ky_efuse_bank *efuse);

	efuse_read = of_device_get_match_data(dev);
	if (!efuse_read) {
		return -EINVAL;
	}

	efuse = devm_kzalloc(dev, sizeof(struct ky_efuse_bank),
			     GFP_KERNEL);
	if (!efuse)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	efuse->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(efuse->base))
		return PTR_ERR(efuse->base);

	efuse->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(efuse->clk))
		return PTR_ERR(efuse->clk);

	efuse->reset = devm_reset_control_get_optional_shared(dev, NULL);
	if (IS_ERR(efuse->reset))
		return PTR_ERR(efuse->reset);

	/* try read efuse data to buffer */
	efuse->size = roundup(resource_size(res), sizeof(u32));
	efuse->efuse_data = devm_kzalloc(dev, efuse->size, GFP_KERNEL);
	if (!efuse->efuse_data)
		return -ENOMEM;

	ret = efuse_read(efuse);
	if (ret < 0)
		return -EBUSY;
	efuse->dev = dev;

	econfig = devm_kzalloc(dev, sizeof(*econfig), GFP_KERNEL);
        if (!econfig)
		return -ENOMEM;

	efuse->econfig = econfig;
	econfig->dev = dev;
	econfig->name = dev_name(dev),
	econfig->stride = 1;
	econfig->word_size = 1;
	econfig->read_only = true;
	econfig->reg_read = ky_efuse_read;
	econfig->size = resource_size(res);
	econfig->priv = efuse;
	econfig->add_legacy_fixed_of_cells = true;

	nvmem = devm_nvmem_register(dev, econfig);
	efuse->nvmem = nvmem;

	platform_set_drvdata(pdev, efuse);

	return PTR_ERR_OR_ZERO(nvmem);
}

static const struct of_device_id ky_efuse_match[] = {
	{
		.compatible = "ky,x1-efuse",
		.data = (void *)&ky_x1_efuse_read,
	},
	{ /* sentinel */},
};

static struct platform_driver ky_efuse_driver = {
	.probe = ky_efuse_probe,
	.driver = {
		.name = "ky-efuse",
		.of_match_table = ky_efuse_match,
	},
};
module_platform_driver(ky_efuse_driver);

MODULE_DESCRIPTION("Ky eFuse driver");
MODULE_LICENSE("GPL v2");

