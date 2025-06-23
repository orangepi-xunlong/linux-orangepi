// SPDX-License-Identifier: GPL-2.0
/*
 * V4L2 Capture CSI Subdev for Cix sky SOC
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 */
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/memory.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/platform_device.h>
#include <linux/acpi.h>
#include "csi_rcsu_hw.h"

#define CIX_RCSU_HW_DRIVER_NAME "cix-csi-rcsu-hw"
#define CSI_RCSU_STRAP_PIN0		(0x300)

static inline u32 csi_rcsu_read(struct csi_rcsu_dev *csi_rcsu_dev, u32 reg)
{
	return readl(csi_rcsu_dev->base + reg);
}

static inline void csi_rcsu_write(struct csi_rcsu_dev *csi_rcsu_dev, u32 reg, u32 val)
{
	writel(val, csi_rcsu_dev->base + reg);
}

static void csi_mux_sel(struct csi_rcsu_dev *csi_rcsu_dev, u32 index, u32 mipi_src)
{
	u32 offset;
	u32 value;

	if (index == 0 || index == 2 )
		offset = 24;
	else {
		offset = 28;
	}

	mutex_lock(&csi_rcsu_dev->mutex);
	value = csi_rcsu_read(csi_rcsu_dev,CSI_RCSU_STRAP_PIN0) &(~(0x3 << offset));
	csi_rcsu_write(csi_rcsu_dev,CSI_RCSU_STRAP_PIN0, value | (mipi_src&0x03) << offset);
	mutex_unlock(&csi_rcsu_dev->mutex);
}

static void dphy_psm_config(struct csi_rcsu_dev *csi_rcsu_dev)
{
	u32 value;

	mutex_lock(&csi_rcsu_dev->mutex);
	value = csi_rcsu_read(csi_rcsu_dev, CSI_RCSU_STRAP_PIN0);

	/*phy psm clock config bit[7:0] = 100 = 0x64*/
	value &= ~PHY_PSM_CLOCK_FREQ_MASK;
	value |= ((0x64 << PHY_PSM_CLOCK_FREQ_OFFSET) & PHY_PSM_CLOCK_FREQ_MASK);

	/*bit8=0, bit9=1*/
	value &= 0xfffffcff;
	value |= 0x200;

	csi_rcsu_write(csi_rcsu_dev, CSI_RCSU_STRAP_PIN0, value);

	mutex_unlock(&csi_rcsu_dev->mutex);
}

static const struct of_device_id csi_rcsu_hw_of_match[] = {
	{
		.compatible = "cix,cix-csi-rcsu-hw",
	},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, csi_rcsu_hw_of_match);

static const struct acpi_device_id csi_rcsu_hw_acpi_match[] = {
	{ .id = "CIXH3027", .driver_data = 0 },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(acpi, csi_rcsu_hw_acpi_match);

static int csi_rcsu_hw_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct csi_rcsu_dev *rcsu;

	dev_info(dev, "rcsu hw probe enter\n");

	rcsu = devm_kzalloc(dev, sizeof(struct csi_rcsu_dev), GFP_KERNEL);
	if (!rcsu)
		return -ENOMEM;

	rcsu->dev = dev;
	rcsu->pdev = pdev;
	rcsu->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(rcsu->base))
		return PTR_ERR(rcsu->base);

	mutex_init(&rcsu->mutex);

	rcsu->chan_mux_sel = csi_mux_sel;
	rcsu->dphy_psm_config = dphy_psm_config;
	platform_set_drvdata(pdev, rcsu);

	dev_info(dev, "csi rcsu hw probe exit\n");

	return 0;
}

static int csi_rcsu_hw_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	dev_info(dev, "csi rcsu hw remove enter\n");
	return 0;
}

static struct platform_driver csi_rcsu_hw_driver = {
	.driver = {
		.name = CIX_RCSU_HW_DRIVER_NAME,
		.of_match_table = csi_rcsu_hw_of_match,
		.acpi_match_table = ACPI_PTR(csi_rcsu_hw_acpi_match),
	},
	.probe = csi_rcsu_hw_probe,
	.remove = csi_rcsu_hw_remove,
};

module_platform_driver(csi_rcsu_hw_driver);
MODULE_AUTHOR("Cix Semiconductor, Inc.");
MODULE_DESCRIPTION("Cix CSI RCSU HWdriver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" CIX_RCSU_HW_DRIVER_NAME);
