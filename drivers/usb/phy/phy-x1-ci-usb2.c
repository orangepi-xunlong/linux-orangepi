// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * UDC Phy support for Ky x1 SoCs
 *
 * Copyright (c) 2023 Ky Inc.
 */

#include <linux/resource.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/platform_data/x1_ci_usb.h>
#include <linux/of_address.h>
#include "phy-x1-ci-usb2.h"

static int mv_usb2_phy_init(struct usb_phy *phy)
{
	struct mv_usb2_phy *mv_phy = container_of(phy, struct mv_usb2_phy, phy);
	void __iomem *base = mv_phy->base;
	uint32_t loops, temp;

	clk_enable(mv_phy->clk);

	// make sure the usb controller is not under reset process before any configuration
	udelay(150);

	loops = USB2D_CTRL_RESET_TIME_MS * 1000;

	//wait for usb2 phy PLL ready
	do {
		temp = readl(base + USB2_PHY_REG01);
		if (temp & USB2_PHY_REG01_PLL_IS_READY)
			break;
		udelay(50);
	} while(--loops);

	if (loops == 0) {
		pr_err("Wait PHY_REG01[PLLREADY] timeout\n");
		return -ETIMEDOUT;
	}

	//release usb2 phy internal reset and enable clock gating
	writel(0x60ef, base + USB2_PHY_REG01);
	writel(0x1c, base + USB2_PHY_REG0D);

	temp = readl(base + USB2_ANALOG_REG14_13);
	temp &= ~(USB2_ANALOG_HSDAC_ISEL_MASK);
	temp |= USB2_ANALOG_HSDAC_ISEL_15_INC | USB2_ANALOG_HSDAC_IREG_EN;
	writel(temp, base + USB2_ANALOG_REG14_13);

	/* auto clear host disc*/
	temp = readl(base + USB2_PHY_REG04);
	temp |= USB2_PHY_REG04_AUTO_CLEAR_DIS;
	writel(temp, base + USB2_PHY_REG04);

	return 0;
}

static void mv_usb2_phy_shutdown(struct usb_phy *phy)
{
	struct mv_usb2_phy *mv_phy = container_of(phy, struct mv_usb2_phy, phy);

	clk_disable(mv_phy->clk);
}

static int mv_usb2_phy_suspend(struct usb_phy *phy, int suspend)
{
	struct mv_usb2_phy *mv_phy = container_of(phy, struct mv_usb2_phy, phy);

	if (suspend)
		clk_disable(mv_phy->clk);
	else
		clk_enable(mv_phy->clk);

	return 0;
}

static int mv_usb2_phy_connect_change(struct usb_phy *phy,
					  enum usb_device_speed speed)
{
	struct mv_usb2_phy *mv_phy = container_of(phy, struct mv_usb2_phy, phy);
	uint32_t reg;
	if (!mv_phy->handle_connect_change)
		return 0;
	reg = readl(mv_phy->base + USB2_PHY_REG40);
	reg |= USB2_PHY_REG40_CLR_DISC;
	writel(reg, mv_phy->base + USB2_PHY_REG40);
	return 0;
}

static int mv_usb2_phy_probe(struct platform_device *pdev)
{
	struct mv_usb2_phy *mv_phy;
	struct resource *r;

	dev_dbg(&pdev->dev, "x1-ci-usb-phy-probe: Enter...\n");
	mv_phy = devm_kzalloc(&pdev->dev, sizeof(*mv_phy), GFP_KERNEL);
	if (mv_phy == NULL) {
		dev_err(&pdev->dev, "failed to allocate memory\n");
		return -ENOMEM;
	}

	mv_phy->pdev = pdev;

	mv_phy->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(mv_phy->clk)) {
		dev_err(&pdev->dev, "failed to get clock.\n");
		return PTR_ERR(mv_phy->clk);
	}
	clk_prepare(mv_phy->clk);

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (r == NULL) {
		dev_err(&pdev->dev, "no phy I/O memory resource defined\n");
		return -ENODEV;
	}
	mv_phy->base = devm_ioremap_resource(&pdev->dev, r);
	if (mv_phy->base == NULL) {
		dev_err(&pdev->dev, "error map register base\n");
		return -EBUSY;
	}

	mv_phy->handle_connect_change = device_property_read_bool(&pdev->dev,
		"ky,handle_connect_change");

	mv_phy->phy.dev = &pdev->dev;
	mv_phy->phy.label = "mv-usb2";
	mv_phy->phy.type = USB_PHY_TYPE_USB2;
	mv_phy->phy.init = mv_usb2_phy_init;
	mv_phy->phy.shutdown = mv_usb2_phy_shutdown;
	mv_phy->phy.set_suspend = mv_usb2_phy_suspend;
	mv_phy->phy.notify_disconnect = mv_usb2_phy_connect_change;
	mv_phy->phy.notify_connect = mv_usb2_phy_connect_change;

	usb_add_phy_dev(&mv_phy->phy);

	platform_set_drvdata(pdev, mv_phy);

	return 0;
}

static int mv_usb2_phy_remove(struct platform_device *pdev)
{
	struct mv_usb2_phy *mv_phy = platform_get_drvdata(pdev);

	usb_remove_phy(&mv_phy->phy);

	clk_unprepare(mv_phy->clk);

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id mv_usbphy_dt_match[] = {
	{ .compatible = "ky,usb2-phy",},
	{},
};
MODULE_DEVICE_TABLE(of, mv_usbphy_dt_match);

static struct platform_driver mv_usb2_phy_driver = {
	.probe	= mv_usb2_phy_probe,
	.remove = mv_usb2_phy_remove,
	.driver = {
		.name   = "mv-usb2-phy",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(mv_usbphy_dt_match),
	},
};

module_platform_driver(mv_usb2_phy_driver);
MODULE_DESCRIPTION("Ky USB2 phy driver");
MODULE_LICENSE("GPL v2");
