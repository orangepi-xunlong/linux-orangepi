// SPDX-License-Identifier: GPL-2.0
/*
 * phy driver for cdn_usb2_p_sd10000
 */
#include <linux/acpi.h>
#include <linux/reset.h>
#include <linux/kernel.h>
#include <linux/of_platform.h>

static const struct acpi_device_id cix_usb2phy_acpi_match[] = {
	{ "CIXH2032" },
	{ },
};
MODULE_DEVICE_TABLE(acpi, cix_usb2phy_acpi_match);

static const struct of_device_id cix_usb2phy_dt_match[] = {
	{
		.compatible = "cix,sky1-usb2-phy",
		.data = NULL
	},
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, rockchip_udphy_dt_match);

static int cix_usb2phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct reset_control *preset;

	preset = devm_reset_control_get(dev, "preset");

	if (IS_ERR(preset)) {
		dev_err(dev, "%s: failed to get preset\n",
			dev->of_node->full_name);
	} else {
		reset_control_deassert(preset);
	}

	return 0;
}

static struct platform_driver cix_usb2phy_driver = {
	.probe		= cix_usb2phy_probe,
	.driver		= {
		.name	= "cix,sky1-usb2-phy",
		.of_match_table = cix_usb2phy_dt_match,
		.acpi_match_table = cix_usb2phy_acpi_match,
		.pm = NULL,
	},
};

module_platform_driver(cix_usb2phy_driver);

MODULE_AUTHOR("Chao Zeng <chao.zeng@cixteck.com>");
MODULE_DESCRIPTION("Cix Usb2 PHY driver");
MODULE_LICENSE("GPL v2");
