/*
*************************************************************************************
*                         			      Linux
*					           AHCI SATA platform driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: ahci_sunxi.c
*
* Author 		: danielwang
*
* Description 	: SATA Host Controller Driver for A20 Platform
*
* Notes         :
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*    danielwang        2013-3-28            1.0          create this file
*
*************************************************************************************
*/

#include <linux/kernel.h>
#include <linux/gfp.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/libata.h>
#include <linux/ahci_platform.h>
#include "ahci.h"

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>

#define DRV_NAME "sunxi-ahci"

#define AHCI_BISTAFR	0x00a0
#define AHCI_BISTCR	0x00a4
#define AHCI_BISTFCTR	0x00a8
#define AHCI_BISTSR	0x00ac
#define AHCI_BISTDECR	0x00b0
#define AHCI_DIAGNR0	0x00b4
#define AHCI_DIAGNR1	0x00b8
#define AHCI_OOBR	0x00bc
#define AHCI_PHYCS0R	0x00c0
#define AHCI_PHYCS1R	0x00c4
#define AHCI_PHYCS2R	0x00c8
#define AHCI_TIMER1MS	0x00e0
#define AHCI_GPARAM1R	0x00e8
#define AHCI_GPARAM2R	0x00ec
#define AHCI_PPARAMR	0x00f0
#define AHCI_TESTR	0x00f4
#define AHCI_VERSIONR	0x00f8
#define AHCI_IDR	0x00fc
#define AHCI_RWCR	0x00fc
#define AHCI_P0DMACR	0x0170
#define AHCI_P0PHYCR	0x0178
#define AHCI_P0PHYSR	0x017c

#define AHCI_SUNXI_DBUG 0

struct ahci_sunxi_platform_data {
	struct clk *pclk;  /* pll clock */
	struct clk *mclk;  /* module clock */
	struct regulator *regu0; /*vdd_sata_25*/
	struct regulator *regu1; /*vdd_sata_12*/
	char regu_id0[16];
	char regu_id1[16];
};

static struct ata_port_info ahci_sunxi_port_info = {
	.flags = AHCI_FLAG_COMMON,
	/*.link_flags = ,*/
	.pio_mask = ATA_PIO4,
	/*.mwdma_mask = ,*/
	.udma_mask = ATA_UDMA6,
	.port_ops = &ahci_ops,
	.private_data = (void *)(AHCI_HFLAG_32BIT_ONLY | AHCI_HFLAG_NO_MSI
		| AHCI_HFLAG_NO_PMP | AHCI_HFLAG_YES_NCQ),
};

static struct scsi_host_template ahci_platform_sht = {
	AHCI_SHT(DRV_NAME),
};

static char* ahci_sunxi_gpio_name = "sata_power_en";

static void sunxi_clrbits(void __iomem *reg, u32 clr_val)
{
	u32 reg_val;

	reg_val = readl(reg);
	reg_val &= ~(clr_val);
	writel(reg_val, reg);
}

static void sunxi_setbits(void __iomem *reg, u32 set_val)
{
	u32 reg_val;

	reg_val = readl(reg);
	reg_val |= set_val;
	writel(reg_val, reg);
}

static void sunxi_clrsetbits(void __iomem *reg, u32 clr_val, u32 set_val)
{
	u32 reg_val;

	reg_val = readl(reg);
	reg_val &= ~(clr_val);
	reg_val |= set_val;
	writel(reg_val, reg);
}

static u32 sunxi_getbits(void __iomem *reg, u8 mask, u8 shift)
{
	return (readl(reg) >> shift) & mask;
}

static void ahci_sunxi_dump_reg(struct device *dev, void __iomem *base)
{
#if AHCI_SUNXI_DBUG
	int i;
	dev_info(dev, "base: 0x%px\n", base);
	for (i = 0; i < 0x200; i += 0x10) {
		dev_info(dev, "0x%3x = 0x%08x, 0x%3x = 0x%08x, 0x%3x = 0x%08x, 0x%3x = 0x%08x\n",
			i, readl(base + i),
			i + 4, readl(base + i + 4),
			i + 8, readl(base + i + 8),
			i + 12, readl(base + i + 12));
	}
#endif
}

static int ahci_sunxi_clk_init(struct device *dev)
{
	int ret;
	struct ahci_sunxi_platform_data *pdata = dev_get_platdata(dev);

	/*Enable mclk and pclk for AHCI*/
	pdata->mclk = of_clk_get(dev->of_node, 1);
	if (IS_ERR_OR_NULL(pdata->mclk)) {
		dev_err(dev, "Error to get module clk for AHCI\n");
		return -1;
	}

	pdata->pclk = of_clk_get(dev->of_node, 0);
	if (IS_ERR_OR_NULL(pdata->pclk)) {
		dev_err(dev, "Error to get pll clk for AHCI\n");
		return -1;
	}

	ret = clk_set_parent(pdata->mclk, pdata->pclk);
	if (ret != 0) {
		dev_err(dev, "clk_set_parent() failed! return %d\n", ret);
		return -1;
	}

	clk_prepare_enable(pdata->mclk);
	dev_info(dev, "frequncy of module clk for AHCI: %lu\n",
			clk_get_rate(pdata->mclk));
	dev_info(dev, "frequncy of pll clk for AHCI: %lu\n",
			clk_get_rate(pdata->pclk));

	return 0;

}

static int ahci_sunxi_clk_exit(struct device *dev)
{
	struct ahci_sunxi_platform_data *pdata = dev_get_platdata(dev);
	if (IS_ERR_OR_NULL(pdata->mclk)) {
		dev_err(dev, "AHCI mclk handle is invalid\n");
		return -1;
	}

	clk_disable_unprepare(pdata->mclk);
	clk_put(pdata->mclk);
	clk_put(pdata->pclk);
	pdata->mclk = NULL;
	pdata->pclk = NULL;
	return 0;
}

static int ahci_sunxi_regulator_request(struct device *dev)
{
	struct ahci_sunxi_platform_data *pdata = dev_get_platdata(dev);
	const char *regu_id0 = NULL;
	const char *regu_id1 = NULL;
	int ret = 0;

	if (IS_ERR_OR_NULL(pdata->regu0)) {
		ret = of_property_read_string(dev->of_node,
				"sata_regulator0", &regu_id0);
		dev_info(dev, "sata_regulator0 \"%s\"!\n", regu_id0);
		if (ret) {
			dev_err(dev, "Failed to get \"%s\"!\n", regu_id0);
			ret = -1;
			goto err0;
		}

		pdata->regu0 = regulator_get(NULL, regu_id0);
		if (IS_ERR_OR_NULL(pdata->regu0)) {
			dev_err(dev, "Failed to get \"%s\"!\n", regu_id0);
			ret = -1;
			goto err0;
		}
		strncpy(pdata->regu_id0, regu_id0, sizeof(pdata->regu_id0));
	}

	if (IS_ERR_OR_NULL(pdata->regu1)) {
		ret = of_property_read_string(dev->of_node,
				"sata_regulator1", &regu_id1);
		dev_info(dev, "sata_regulator1 \"%s\"!\n", regu_id1);
		if (ret) {
			dev_err(dev, "Failed to get \"%s\"!\n", regu_id1);
			ret = -1;
			goto err1;
		}

		pdata->regu1 = regulator_get(NULL, regu_id1);
		if (IS_ERR_OR_NULL(pdata->regu1)) {
			dev_err(dev, "Failed to get \"%s\"!\n", regu_id1);
			ret = -1;
			goto err1;
		}
		strncpy(pdata->regu_id1, regu_id1, sizeof(pdata->regu_id1));
	}
err0:
	return ret;

err1:
	regulator_put(pdata->regu0);
	goto err0;
}

static void ahci_sunxi_regulator_release(struct device *dev)
{
	struct ahci_sunxi_platform_data *pdata = dev_get_platdata(dev);
	if (!IS_ERR_OR_NULL(pdata->regu0)) {
		regulator_put(pdata->regu0);
		pdata->regu0 = NULL;
	}

	if (!IS_ERR_OR_NULL(pdata->regu1)) {
		regulator_put(pdata->regu1);
		pdata->regu1 = NULL;
	}
}

static int ahci_sunxi_set_voltage(struct device *dev)
{
	struct ahci_sunxi_platform_data *pdata = dev_get_platdata(dev);
	int voltage;
	int ret = 0;

	if (IS_ERR_OR_NULL(pdata->regu0) || IS_ERR_OR_NULL(pdata->regu0))
		return 0;

	voltage = regulator_get_voltage(pdata->regu0);
	if (regulator_set_voltage(pdata->regu0, 2500000, 2500000) != 0) {
		dev_info(dev,
			"failed to set voltage for regulator \"%s\" !\n",
			pdata->regu_id0);
		ret = -1;
	} else
		dev_info(dev,
			"regulator \"%s\" voltage set: %d to 2500000!\n",
			pdata->regu_id0, voltage);

	voltage = regulator_get_voltage(pdata->regu1);
	if (regulator_set_voltage(pdata->regu1, 1200000, 1200000) != 0) {
		dev_info(dev,
			"failed to set voltage for regulator \"%s\" !\n",
			pdata->regu_id1);
		ret = -1;
	} else
		dev_info(dev,
			"regulator \"%s\" voltage set: %d to 1200000!\n",
			pdata->regu_id1, voltage);

	return ret;
}

static int ahci_sunxi_regulator_enable(struct device *dev)
{
	struct ahci_sunxi_platform_data *pdata = dev_get_platdata(dev);
	int ret = 0;

	if (IS_ERR_OR_NULL(pdata->regu0) || IS_ERR_OR_NULL(pdata->regu0))
		return 0;

	if (regulator_enable(pdata->regu0) != 0) {
		dev_err(dev,
			"Failed to enable regulator \"%s\"!\n",
			pdata->regu_id0);
		ret = -1;
		goto err0;
	}

	if (regulator_enable(pdata->regu1) != 0) {
		dev_err(dev,
			"Failed to enable regulator \"%s\"!\n",
			pdata->regu_id1);
		ret = -1;
		goto err1;
	}
err0:
	return ret;

err1:
	regulator_disable(pdata->regu0);
	goto err0;
}

static int ahci_sunxi_regulator_disable(struct device *dev)
{
	struct ahci_sunxi_platform_data *pdata = dev_get_platdata(dev);
	int ret = 0;

	if (!IS_ERR_OR_NULL(pdata->regu0)) {
		if (regulator_disable(pdata->regu0) != 0) {
			dev_err(dev,
				"Failed to disable regulator \"%s\"!\n",
				pdata->regu_id0);
			ret = -1;
		}
	}

	if (!IS_ERR_OR_NULL(pdata->regu1)) {
		if (regulator_disable(pdata->regu1) != 0) {
			dev_err(dev,
				"Failed to disable regulator \"%s\"!\n",
				pdata->regu_id1);
			ret = -1;
		}
	}

	return ret;
}

static int ahci_sunxi_regulator_init(struct device *dev)
{
	if (ahci_sunxi_regulator_request(dev) != 0)
		return -1;

	ahci_sunxi_set_voltage(dev);

	if (ahci_sunxi_regulator_enable(dev) != 0)
		goto err1;

	return 0;

err1:
	ahci_sunxi_regulator_release(dev);
	return -1;
}

static int ahci_sunxi_regulator_exit(struct device *dev)
{
	ahci_sunxi_regulator_disable(dev);
	ahci_sunxi_regulator_release(dev);

	return 0;
}

static int ahci_sunxi_phy_init(struct device *dev, void __iomem *reg_base)
{
	u32 reg_val;
	int timeout;
	int rc = 0;

	/* This magic is from the original code */
	writel(0, reg_base + AHCI_RWCR);
	msleep(5);

	sunxi_setbits(reg_base + AHCI_PHYCS1R, BIT(19));
	sunxi_clrsetbits(reg_base + AHCI_PHYCS0R,
			 (0x7 << 24),
			 (0x5 << 24) | BIT(23) | BIT(18));

	sunxi_clrsetbits(reg_base + AHCI_PHYCS1R,
			 (0x3 << 16) | (0x1f << 8) | (0x3 << 6),
			 (0x2 << 16) | (0x6 << 8) | (0x2 << 6));

	sunxi_setbits(reg_base + AHCI_PHYCS1R, BIT(28) | BIT(15));
	sunxi_clrbits(reg_base + AHCI_PHYCS1R, BIT(19));

	sunxi_clrsetbits(reg_base + AHCI_PHYCS0R,
			 (0x7 << 20), (0x3 << 20));

	sunxi_clrsetbits(reg_base + AHCI_PHYCS2R,
			 (0x1f << 5), (0x19 << 5));

	msleep(5);

	sunxi_setbits(reg_base + AHCI_PHYCS0R, (0x1 << 19));

	timeout = 250; /* Power up takes aprox 50 us */
	do {
		reg_val = sunxi_getbits(reg_base + AHCI_PHYCS0R, 0x7, 28);
		if (reg_val == 0x02)
			break;

		if (--timeout == 0) {
			dev_err(dev, "PHY power up failed.\n");
			ahci_sunxi_dump_reg(dev, reg_base);
			rc = -EIO;
			/*goto out;*/
			break;
		}
		udelay(1);
	} while (1);

	sunxi_setbits(reg_base + AHCI_PHYCS2R, (0x1 << 24));

	timeout = 100; /* Calibration takes aprox 10 us */
	do {
		reg_val = sunxi_getbits(reg_base + AHCI_PHYCS2R, 0x1, 24);
		if (reg_val == 0x00)
			break;

		if (--timeout == 0) {
			dev_err(dev, "PHY calibration failed.\n");
			ahci_sunxi_dump_reg(dev, reg_base);
			rc = -EIO;
			break;
		}
		udelay(1);
	} while (1);

	msleep(15);

	writel(0x7, reg_base + AHCI_RWCR);
	return rc;
}

static int ahci_sunxi_is_enabled(struct device *dev)
{
	int ret = 0;
	int is_enabled = 0;

	const char  *used_status;

	ret = of_property_read_string(dev->of_node, "status", &used_status);
	if (ret) {
		dev_err(dev, "get ahci_sunxi_is_enabled failed, %d\n", -ret);
		is_enabled = 0;
	}else if (!strcmp(used_status, "okay")) {
		is_enabled = 1;
	}else {
		dev_err(dev, "AHCI is not enabled\n");
		is_enabled = 0;
	}

	return is_enabled;
}

static int ahci_sunxi_gpio_set(struct device *dev, int value)
{
	int rc = 0;
	int ret;
	u32 pio_hdle;

	ret = of_get_named_gpio(dev->of_node, ahci_sunxi_gpio_name, 0);
	if (!gpio_is_valid(ret)) {
		dev_err(dev, "SATA power enable do not exist!!\n");
		rc = 0;
		goto err0;
	}else{
		pio_hdle = ret;
	}

	ret = gpio_request(pio_hdle, "sata");
	if (ret) {
		dev_err(dev, "SATA request power enable failed\n");
		return -EINVAL;
		goto err0;
	}

	gpio_direction_output(pio_hdle, value);
	gpio_free(pio_hdle);

err0:
	return rc;
}

static int ahci_sunxi_start(struct device *dev, void __iomem *addr)
{
	int rc = 0;

	if (!ahci_sunxi_is_enabled(dev)) {
		dev_err(dev, "sunxi AHCI is not enabled\n");
		return -ENODEV;
	}

	rc = ahci_sunxi_regulator_init(dev);
	if (rc)
		goto err0;

	rc = ahci_sunxi_clk_init(dev);
	if (rc)
		goto err1;

	rc = ahci_sunxi_phy_init(dev, addr);
	if (rc)
		goto err2;

	ahci_sunxi_gpio_set(dev, 1);

err0:
	return rc;

err2:
	ahci_sunxi_clk_exit(dev);
err1:
	ahci_sunxi_regulator_exit(dev);
	goto err0;
}

static void ahci_sunxi_stop(struct device *dev)
{
	ahci_sunxi_clk_exit(dev);
	ahci_sunxi_regulator_exit(dev);
	ahci_sunxi_gpio_set(dev, 0);
}

static int ahci_sunxi_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ahci_sunxi_platform_data *pdata;
	struct ata_port_info pi = ahci_sunxi_port_info;
	const struct ata_port_info *ppi[] = { &pi, NULL };
	struct ahci_host_priv *hpriv;
	struct ata_host *host;
	struct resource *mem;
	int irq;
	int n_ports;
	int i;
	int rc;

	pdata = devm_kzalloc(dev,
			sizeof(struct ahci_sunxi_platform_data), GFP_KERNEL);
	if (pdata == NULL) {
		dev_err(dev, "can't alloc ahci_sunxi_platform_data!\n");
		return -ENOMEM;
	}
	pdev->dev.platform_data = pdata;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(dev, "no mmio space\n");
		return -EINVAL;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		dev_err(dev, "no irq\n");
		return -EINVAL;
	}

	hpriv = devm_kzalloc(dev, sizeof(*hpriv), GFP_KERNEL);
	if (!hpriv) {
		dev_err(dev, "can't alloc ahci_host_priv\n");
		return -ENOMEM;
	}

	hpriv->flags |= (unsigned long)pi.private_data;

	hpriv->mmio = devm_ioremap(dev, mem->start, resource_size(mem));
	if (!hpriv->mmio) {
		dev_err(dev, "can't map %pR\n", mem);
		return -ENOMEM;
	}

	/*
	 * Some platforms might need to prepare for mmio region access,
	 * which could be done in the following init call. So, the mmio
	 * region shouldn't be accessed before init (if provided) has
	 * returned successfully.
	 */

	rc = ahci_sunxi_start(dev, hpriv->mmio);
	if (rc)
		return rc;

	ahci_save_initial_config(dev, hpriv, 0, 0);

	/* prepare host */
	if (hpriv->cap & HOST_CAP_NCQ)
		pi.flags |= ATA_FLAG_NCQ;

	if (hpriv->cap & HOST_CAP_PMP)
		pi.flags |= ATA_FLAG_PMP;

	ahci_set_em_messages(hpriv, &pi);

	/* CAP.NP sometimes indicate the index of the last enabled
	 * port, at other times, that of the last possible port, so
	 * determining the maximum port number requires looking at
	 * both CAP.NP and port_map.
	 */
	n_ports = max(ahci_nr_ports(hpriv->cap), fls(hpriv->port_map));

	host = ata_host_alloc_pinfo(dev, ppi, n_ports);
	if (!host) {
		rc = -ENOMEM;
		goto err0;
	}

	host->private_data = hpriv;

	if (!(hpriv->cap & HOST_CAP_SSS) || ahci_ignore_sss)
		host->flags |= ATA_HOST_PARALLEL_SCAN;
	else
		printk(KERN_INFO "ahci: SSS flag set, parallel bus scan disabled\n");

	if (pi.flags & ATA_FLAG_EM)
		ahci_reset_em(host);

	for (i = 0; i < host->n_ports; i++) {
		struct ata_port *ap = host->ports[i];

		ata_port_desc(ap, "mmio %pR", mem);
		ata_port_desc(ap, "port 0x%x", 0x100 + ap->port_no * 0x80);

		/* set enclosure management message type */
		if (ap->flags & ATA_FLAG_EM)
			ap->em_message_type = hpriv->em_msg_type;

		/* disabled/not-implemented port */
		if (!(hpriv->port_map & (1 << i)))
			ap->ops = &ata_dummy_port_ops;
	}

	rc = ahci_reset_controller(host);
	if (rc)
		goto err0;

	ahci_init_controller(host);
	ahci_print_info(host, "platform");

	rc = ata_host_activate(host, irq, ahci_interrupt, IRQF_SHARED,
			       &ahci_platform_sht);
	if (rc)
		goto err0;

	return 0;
err0:
	ahci_sunxi_stop(dev);
	return rc;
}

#ifdef CONFIG_PM_SLEEP

static int ahci_sunxi_suspend(struct device *dev)
{
	struct ata_host *host = dev_get_drvdata(dev);
	struct ahci_host_priv *hpriv = host->private_data;
	printk("ahci_sunxi: ahci_sunxi_suspend\n"); //danielwang

	ahci_sunxi_dump_reg(dev, hpriv->mmio);
	ahci_sunxi_stop(dev);

	return 0;
}

extern int ahci_hardware_recover_for_controller_resume(struct ata_host *host);
static int ahci_sunxi_resume(struct device *dev)
{
	struct ata_host *host = dev_get_drvdata(dev);
	struct ahci_host_priv *hpriv = host->private_data;

	printk("ahci_sunxi: ahci_sunxi_resume\n"); //danielwang

	ahci_sunxi_start(dev, hpriv->mmio);
	ahci_sunxi_dump_reg(dev, hpriv->mmio);

	ahci_hardware_recover_for_controller_resume(host);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(ahci_sunxi_pmops,
			ahci_sunxi_suspend, ahci_sunxi_resume);

static const struct of_device_id ahci_sunxi_of_match[] = {
	{ .compatible = "allwinner,sun8i-sata", },
	{},
};
MODULE_DEVICE_TABLE(of, ahci_sunxi_of_match);

static struct platform_driver ahci_sunxi_driver = {
	.probe = ahci_sunxi_probe,
	.remove = ata_platform_remove_one,
	.driver = {
		.name = "sunxi-ahci",
		.owner = THIS_MODULE,
		.of_match_table = ahci_sunxi_of_match,
		.pm = &ahci_sunxi_pmops,
	},
};
module_platform_driver(ahci_sunxi_driver);

MODULE_DESCRIPTION("SW AHCI SATA platform driver");
MODULE_AUTHOR("Daniel Wang <danielwang@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-ahci");
