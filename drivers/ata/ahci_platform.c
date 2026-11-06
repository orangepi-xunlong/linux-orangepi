// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * AHCI SATA platform driver
 *
 * Copyright 2004-2005  Red Hat, Inc.
 *   Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2010  MontaVista Software, LLC.
 *   Anton Vorontsov <avorontsov@ru.mvista.com>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pm.h>
#include <linux/device.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/libata.h>
#include <linux/ahci_platform.h>
#include <linux/acpi.h>
#include <linux/pci_ids.h>
#include "ahci.h"

#define DRV_NAME "ahci"

static const struct ata_port_info ahci_port_info = {
	.flags		= AHCI_FLAG_COMMON,
	.pio_mask	= ATA_PIO4,
	.udma_mask	= ATA_UDMA6,
	.port_ops	= &ahci_platform_ops,
};

static const struct ata_port_info ahci_port_info_nolpm = {
	.flags		= AHCI_FLAG_COMMON | ATA_FLAG_NO_LPM,
	.pio_mask	= ATA_PIO4,
	.udma_mask	= ATA_UDMA6,
	.port_ops	= &ahci_platform_ops,
};

static struct scsi_host_template ahci_platform_sht = {
	AHCI_SHT(DRV_NAME),
};

#define SSPERI_TOP_REG_BASE     0x20BE0000
#define SSPERI_AXI_BASE         0x20000000

int sophgo_phy_init(void)
{
	uint32_t reg;
	int timeout = 100;
	void __iomem *axi_base = NULL;
	void __iomem *top_base = NULL;

	axi_base = ioremap(SSPERI_AXI_BASE, 0x44);
	if (axi_base == NULL) {
		pr_info("ioremap failed!\n");
		return -1;
	}
	reg = readl(axi_base + 0x40);
	reg &= ~(0x1 << 2);
	writel(reg, axi_base + 0x40);
	iounmap(axi_base);


	top_base = ioremap(SSPERI_TOP_REG_BASE, 0x4f0);
	if (top_base == NULL) {
		pr_info("ioremap failed!\n");
		return -1;
	}

	/* set pipe for sata and pcie both enable */
	reg = readl(top_base + 0x44);
	/* pipe mode set for sata */
	reg &= ~(0x3);
	reg |= (0x2);
	writel(reg, top_base + 0x44);

	/* set phy protocol for sata operation */
	reg = readl(top_base + 0x278);
	reg &= ~(0x3);
	reg |= 0x2;
	writel(reg, top_base + 0x278);

	/* phy lane0 rx_term_acdc control bit */
	reg = readl(top_base + 0x48);
	reg &= ~(0x3);
	reg |= 0x2;
	writel(reg, top_base + 0x48);

	/* phy vph for 1.8v */
	reg = readl(top_base + 0x34);
	reg &= ~(0x7);
	reg |= 0x7;
	writel(reg, top_base + 0x34);

	/* phy0 sram_ld_done 0
	 * phy0 sram_bypass 0
	 * phy0 refclk for use_pad，
	 * phy0 ref_repeat_clk
	 */
	reg = readl(top_base + 0x4b0);
	reg &= ~(0xf << 24);
	reg &= ~(0x1 << 23);
	reg |= (0x2 << 24);
	reg |= (0x1 << 23);
	writel(reg, top_base + 0x4b0);

	/* set phy0 pwr_en/stable */
	reg = readl(top_base + 0x4b8);
	reg |= (0xf << 15);
	writel(reg, top_base + 0x4b8);

	reg = readl(top_base + 0x4a4);
	reg |= (0x1 << 10);
	writel(reg, top_base + 0x4a4);

	udelay(10);
	/* release phy_reset */
	reg = readl(top_base + 0x34);
	reg |= 0x1 << 28;
	writel(reg, top_base + 0x34);

	while (!(readl(top_base + 0x4b8) & 0x20)) {
		if (timeout-- > 0) {
			mdelay(1);
		} else {
			pr_info("phy sram init timeout");
			return -1;
			//break;
		}
	}
	reg = readl(top_base + 0x4b0);
	reg |= 0x1 << 27;
	writel(reg, top_base + 0x4b0);

	iounmap(top_base);
	return 0;
}

static int ahci_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ahci_host_priv *hpriv;
	const struct ata_port_info *port;
	int rc;

	hpriv = ahci_platform_get_resources(pdev,
					    AHCI_PLATFORM_GET_RESETS);
	if (IS_ERR(hpriv))
		return PTR_ERR(hpriv);

	rc = ahci_platform_enable_resources(hpriv);
	if (rc)
		return rc;

	of_property_read_u32(dev->of_node,
			     "ports-implemented", &hpriv->force_port_map);

	if (of_device_is_compatible(dev->of_node, "hisilicon,hisi-ahci"))
		hpriv->flags |= AHCI_HFLAG_NO_FBS | AHCI_HFLAG_NO_NCQ;

	port = acpi_device_get_match_data(dev);
	if (!port)
		port = &ahci_port_info;

	rc = sophgo_phy_init();
	if (rc)
		goto disable_resources;

	rc = ahci_platform_init_host(pdev, hpriv, port,
				     &ahci_platform_sht);
	if (rc)
		goto disable_resources;

	return 0;
disable_resources:
	ahci_platform_disable_resources(hpriv);
	return rc;
}

static SIMPLE_DEV_PM_OPS(ahci_pm_ops, ahci_platform_suspend,
			 ahci_platform_resume);

static const struct of_device_id ahci_of_match[] = {
	{ .compatible = "generic-ahci", },
	/* Keep the following compatibles for device tree compatibility */
	{ .compatible = "snps,spear-ahci", },
	{ .compatible = "ibm,476gtr-ahci", },
	{ .compatible = "snps,dwc-ahci", },
	{ .compatible = "hisilicon,hisi-ahci", },
	{ .compatible = "cavium,octeon-7130-ahci", },
	{},
};
MODULE_DEVICE_TABLE(of, ahci_of_match);

static const struct acpi_device_id ahci_acpi_match[] = {
	{ "APMC0D33", (unsigned long)&ahci_port_info_nolpm },
	{ ACPI_DEVICE_CLASS(PCI_CLASS_STORAGE_SATA_AHCI, 0xffffff) },
	{},
};
MODULE_DEVICE_TABLE(acpi, ahci_acpi_match);

static struct platform_driver ahci_driver = {
	.probe = ahci_probe,
	.remove = ata_platform_remove_one,
	.shutdown = ahci_platform_shutdown,
	.driver = {
		.name = DRV_NAME,
		.of_match_table = ahci_of_match,
		.acpi_match_table = ahci_acpi_match,
		.pm = &ahci_pm_ops,
	},
};
module_platform_driver(ahci_driver);

MODULE_DESCRIPTION("AHCI SATA platform driver");
MODULE_AUTHOR("Anton Vorontsov <avorontsov@ru.mvista.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ahci");
