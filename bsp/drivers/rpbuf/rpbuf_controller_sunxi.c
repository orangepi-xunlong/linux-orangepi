// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * (C) Copyright 2020-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Junyan Lin <junyanlin@allwinnertech.com>
 *
 * Allwinner RPBuf driver.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/dma-mapping.h>
#include <linux/iommu.h>

#include "rpbuf_internal.h"

#define SUNXI_RPBUF_CONTROLLER_VERSION "1.1.0"

struct sunxi_rpbuf_controller_priv {
	int ctrl_id;
	struct iommu_domain *domain;
};

static int sunxi_rpbuf_controller_alloc_payload_memory(struct rpbuf_controller *controller,
						       struct rpbuf_buffer *buffer,
						       void *priv)
{
	struct device *dev = controller->dev;
	struct sunxi_rpbuf_controller_priv *chip = priv;
	void *va;
	dma_addr_t pa;
	int ret;

	va = dma_alloc_coherent(dev, buffer->len, &pa, GFP_KERNEL);
	if (IS_ERR_OR_NULL(va)) {
		dev_err(dev, "dma_alloc_coherent for len %d failed\n", buffer->len);
		ret = -ENOMEM;
		goto err_out;
	}
	dev_dbg(dev, "allocate payload memory: va 0x%pK, pa %pad, len %d\n",
		va, &pa, buffer->len);

	if (chip->domain) {
		// TODO: has iommu
	} else {
		buffer->va = va;
		buffer->pa = (phys_addr_t)pa;
		buffer->da = (u64)pa;
	}

	return 0;

err_out:
	return ret;
}

static void sunxi_rpbuf_controller_free_payload_memory(struct rpbuf_controller *controller,
						       struct rpbuf_buffer *buffer,
						       void *priv)
{
	struct device *dev = controller->dev;
	struct sunxi_rpbuf_controller_priv *chip = priv;
	void *va = NULL;
	dma_addr_t pa = 0;

	if (chip->domain) {
		// TODO: has iommu
	} else {
		va = (void *)buffer->va;
		pa = (dma_addr_t)buffer->pa;
	}

	dev_dbg(dev, "free payload memory: va %pK, pa %pad, len %d\n",
		va, &pa, buffer->len);
	dma_free_coherent(dev, buffer->len, va, pa);
}

static struct rpbuf_controller_ops sunxi_rpbuf_controller_ops = {
	.alloc_payload_memory = sunxi_rpbuf_controller_alloc_payload_memory,
	.free_payload_memory = sunxi_rpbuf_controller_free_payload_memory,
};

static int sunxi_rpbuf_controller_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct sunxi_rpbuf_controller_priv *chip;
	struct rpbuf_controller *controller;
	struct device_node *rproc_np;
	struct device *rproc;
	struct device_node *tmp_np;
	u32 ctrl_id;
	int ret;

	chip = devm_kzalloc(dev, sizeof(struct sunxi_rpbuf_controller_priv), GFP_KERNEL);
	if (!chip) {
		dev_err(dev, "kzalloc for sunxi_rpbuf_controller_priv failed\n");
		ret = -ENOMEM;
		goto err_out;
	}

	rproc_np = of_parse_phandle(np, "remoteproc", 0);
	if (!rproc_np) {
		dev_err(dev, "no \"remoteproc\" node specified\n");
		ret = -EINVAL;
		goto err_free_controller_priv;
	}

	/* We assume that the remoteproc is always a platform device */
	rproc = bus_find_device_by_of_node(&platform_bus_type, rproc_np);
	if (!rproc) {
		dev_err(dev, "failed to get remoteproc device\n");
		ret = -EINVAL;
		goto err_free_controller_priv;
	}

	if (of_property_read_bool(rproc_np, "iommus")) {
		// TODO:
		dev_dbg(dev, "has iommu\n");

		chip->domain = iommu_domain_alloc(rproc->bus);
		if (!chip->domain) {
			dev_err(dev, "iommu_domain_alloc failed\n");
			ret = -ENODEV;
			goto err_free_controller_priv;
		}
	}

	ret = of_property_read_u32(np, "ctrl_id", &ctrl_id);
	if (ret < 0) {
		dev_err(dev, "no \"ctrl_id\" node specified\n");
		goto err_free_iommu_domain;
	}

	tmp_np = of_parse_phandle(np, "memory-region", 0);
	if (tmp_np) {
		ret = of_reserved_mem_device_init(dev);
		if (ret < 0) {
			dev_err(dev, "failed to get reserved memory (ret: %d)\n", ret);
			goto err_free_iommu_domain;
		}
	}

	controller = rpbuf_create_controller(dev, &sunxi_rpbuf_controller_ops, chip);
	if (!controller) {
		dev_err(dev, "rpbuf_create_controller failed\n");
		ret = -ENOMEM;
		goto err_free_iommu_domain;
	}

	/*
	 * Linux can only be MASTER until we ensure the translation from
	 * buffer->pa to buffer->va is correct.
	 * We use the remoteproc device as rpbuf link token.
	 */
	ret = rpbuf_register_controller(controller, (void *)rproc, RPBUF_ROLE_MASTER);
	if (ret < 0) {
		dev_err(dev, "rpbuf_register_controller failed\n");
		goto err_destroy_controller;
	}

	ret = rpbuf_register_ctrl_dev(dev, ctrl_id, controller);
	if (ret < 0) {
		dev_err(dev, "rpbuf_register_ctrl_dev failed\n");
		goto err_unregister_controller;
	}

	chip->ctrl_id = ctrl_id;

	/*
	 * We must set rpbuf_controller as the device drvdata, to ensure that it
	 * can be found by rpbuf_get_controller_by_of_node().
	 */
	dev_set_drvdata(dev, controller);

	return 0;
err_unregister_controller:
	rpbuf_unregister_controller(controller);
err_destroy_controller:
	rpbuf_destroy_controller(controller);
err_free_iommu_domain:
	if (chip->domain)
		iommu_domain_free(chip->domain);
err_free_controller_priv:
	devm_kfree(dev, chip);
err_out:
	return ret;
}

static int sunxi_rpbuf_controller_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rpbuf_controller *controller = dev_get_drvdata(dev);
	struct sunxi_rpbuf_controller_priv *chip = controller->priv;
	int ctrl_id = chip->ctrl_id;
	int ret = 0;

	if (IS_ERR_OR_NULL(controller)) {
		dev_err(dev, "invalid rpbuf_controller ptr\n");
		ret = -EFAULT;
		goto err_out;
	}

	dev_set_drvdata(dev, NULL);

	rpbuf_unregister_ctrl_dev(dev, ctrl_id);
	rpbuf_unregister_controller(controller);
	rpbuf_destroy_controller(controller);

	if (chip->domain)
		iommu_domain_free(chip->domain);

	devm_kfree(dev, chip);

	return 0;
err_out:
	return ret;
}

static const struct of_device_id sunxi_rpbuf_controller_ids[] = {
	{ .compatible = "allwinner,rpbuf-controller" },
	{}
};

static struct platform_driver sunxi_rpbuf_controller_driver = {
	.probe	= sunxi_rpbuf_controller_probe,
	.remove	= sunxi_rpbuf_controller_remove,
	.driver	= {
		.owner = THIS_MODULE,
		.name = "sunxi-rpbuf-controller",
		.of_match_table = sunxi_rpbuf_controller_ids,
	},
};

#ifdef CONFIG_AW_RPROC_FAST_BOOT
static int __init sunxi_rpbuf_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_rpbuf_controller_driver);

	return ret;
}

static void __exit sunxi_rpbuf_exit(void)
{
	platform_driver_unregister(&sunxi_rpbuf_controller_driver);
}

subsys_initcall(sunxi_rpbuf_init);
module_exit(sunxi_rpbuf_exit);
#else
module_platform_driver(sunxi_rpbuf_controller_driver);
#endif

MODULE_DESCRIPTION("Allwinner RPBuf controller driver");
MODULE_AUTHOR("Junyan Lin <junyanlin@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SUNXI_RPBUF_CONTROLLER_VERSION);
