// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023 CIX.
 */

#include <linux/module.h>
#include <linux/nvmem-provider.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/arm-smccc.h>


#define FUSE_SMC_RV(func_num) \
		((1U << 31U) | \
		 ((0U) << 30U) | \
		 (62 << 24U) | \
		 ((func_num) & 0xFFFFU))

#define FUSE_FUNCID_REQ_TO_SE	9
#define FUSE_REQUEST_TO_SE_DONE \
	FUSE_SMC_RV(FUSE_FUNCID_REQ_TO_SE)

struct sky1_nvmem_data {
	struct device *dev;
	struct nvmem_device *nvmem;
};

static int sky1_nvmem_read(void *context, unsigned int offset,
			     void *val, size_t bytes)
{
	struct sky1_nvmem_data *priv = context;
	struct arm_smccc_res smccc;
	unsigned char *buf = val;

	/* Send request to bl31 */
	for(int i = 0; i < bytes; i++) {
		arm_smccc_smc(FUSE_REQUEST_TO_SE_DONE, offset++, 1, 0, 0, 0, 0, 0, &smccc);
		if (0x0 == smccc.a0)
			return -1;
		buf[i] = smccc.a1;
		dev_dbg(priv->dev, "Read fuse data 0x%x", buf[i]);
	}

	return 0;
}

static struct nvmem_config config = {
	.name = "sky1-nvmem",
	.owner = THIS_MODULE,
	.word_size = 1,
	.size = 1,
	.read_only = true,
};

static const struct of_device_id sky1_nvmem_match[] = {
	{ .compatible = "cix,sky1-nvmem-fw", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, sky1_nvmem_match);

static int sky1_nvmem_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sky1_nvmem_data *priv;

	priv = devm_kzalloc(dev, sizeof(struct sky1_nvmem_data), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = dev;
	config.dev = dev;
	config.reg_read = sky1_nvmem_read;
	config.priv = priv;

	priv->nvmem = devm_nvmem_register(dev, &config);

	return PTR_ERR_OR_ZERO(priv->nvmem);
}

static struct platform_driver sky1_nvmem_driver = {
	.probe = sky1_nvmem_probe,
	.driver = {
		.name = "sky1-nvmem",
		.of_match_table = sky1_nvmem_match,
	},
};

module_platform_driver(sky1_nvmem_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jerry Zhu <jerry.zhu@cixtech.com>");
MODULE_DESCRIPTION("Cix efuse driver");
