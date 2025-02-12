// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * DOC: dwmac-ky.c - Ky DWMAC specific glue layer
 *
 * Copyright (c) 2023, ky Corporation.
 */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_net.h>
#include <linux/platform_device.h>
#include <linux/reset.h>

#include "stmmac_platform.h"

struct ky_dwmac_data {
	u32 addr_width;
};

struct ky_dwmac_priv_data {
	int id;
	void __iomem *sys_gmac_cfg;
	phy_interface_t interface;
	struct clk *gmac_pll_clk;
	unsigned long gmac_pll_clk_freq;
	struct reset_control *gmac_csr_rstc;
	struct reset_control *gmac_dma_rstc;

	const struct ky_dwmac_data *data;
};

static struct ky_dwmac_data x1_pro_dwmac_data = {
	.addr_width = 40,
};

static const struct of_device_id ky_dwmac_match[] = {
	{ .compatible = "ky,x1-pro-mac", .data = &x1_pro_dwmac_data},
	{ }
};
MODULE_DEVICE_TABLE(of, ky_dwmac_match);

/* set gmac speed */
static void ky_dwmac_set_speed(void __iomem *sys_gmac_cfg, int interface,
				unsigned int speed)
{
	volatile unsigned int reg;

	if (sys_gmac_cfg == NULL)
		return;

	if (interface == PHY_INTERFACE_MODE_MII
	    || interface == PHY_INTERFACE_MODE_RMII) {
		reg = readl(sys_gmac_cfg);
		reg |= BIT(0);
		writel(reg, sys_gmac_cfg);
		return;
	} else if (interface == PHY_INTERFACE_MODE_GMII
		|| interface == PHY_INTERFACE_MODE_RGMII
		|| interface == PHY_INTERFACE_MODE_RGMII_ID
		|| interface == PHY_INTERFACE_MODE_RGMII_RXID
		|| interface == PHY_INTERFACE_MODE_RGMII_TXID) {
        reg = readl(sys_gmac_cfg);
		reg &= ~BIT(0);
		writel(reg, sys_gmac_cfg);
	} else {
		pr_err("phy interface %d not supported\n", interface);
		return;
	}
}

static int ky_dwmac_init(struct platform_device *pdev, void *bsp_priv)
{
    struct ky_dwmac_priv_data *ky_plat_dat = bsp_priv;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct resource *res;
	void __iomem *ptr;
	struct clk *clktmp;
	int ret;

	ky_plat_dat->id = of_alias_get_id(np, "ethernet");
	if (ky_plat_dat->id < 0) {
		ky_plat_dat->id = 0;
	}
	dev_info(dev, "id: %d\n", ky_plat_dat->id);

	if (of_get_phy_mode(dev->of_node, &(ky_plat_dat->interface))) {
		dev_err(dev, "of_get_phy_mode error\n");
		return -1;
	}

	dev_info(dev, "phy interface: %d\n", ky_plat_dat->interface);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "sys_gmac_cfg");
	if ((res != NULL) && (resource_type(res) == IORESOURCE_MEM)) {
		ptr = devm_ioremap(dev, res->start, resource_size(res));
		if (!ptr) {
			dev_err(dev, "phy interface register not exist, skipped it\n");
		} else {
			ky_plat_dat->sys_gmac_cfg = ptr;
		}
	}

    /* get gmac pll clk */
	clktmp = devm_clk_get(dev, "gmac_pll_clk");
	if (IS_ERR(clktmp)) {
		dev_err(dev, "gmac_pll_clk not exist, skipped it\n");
	} else {
		ky_plat_dat->gmac_pll_clk = clktmp;

		ret = clk_prepare_enable(ky_plat_dat->gmac_pll_clk);
		if (ret) {
			dev_err(dev, "Failed to enable clk 'gmac_pll_clk'\n");
			return -1;
		}

		ky_plat_dat->gmac_pll_clk_freq =
				clk_get_rate(ky_plat_dat->gmac_pll_clk);
	}

	ky_plat_dat->gmac_csr_rstc = devm_reset_control_get_exclusive(&pdev->dev, "gmac_csr");
	ky_plat_dat->gmac_dma_rstc = devm_reset_control_get_exclusive(&pdev->dev, "gmac_dma");
	if (IS_ERR(ky_plat_dat->gmac_csr_rstc) || IS_ERR(ky_plat_dat->gmac_dma_rstc)) {
		dev_err(&pdev->dev, "failed to get reset.\n");
		return -1;
	}
	reset_control_deassert(ky_plat_dat->gmac_csr_rstc);
	reset_control_deassert(ky_plat_dat->gmac_dma_rstc);

	/* default speed is 1Gbps */
	ky_dwmac_set_speed(ky_plat_dat->sys_gmac_cfg,
				ky_plat_dat->interface, SPEED_1000);

	return 0;
}

static void ky_dwmac_fix_speed(void *bsp_priv, unsigned int speed)
{
	struct ky_dwmac_priv_data *ky_plat_dat = bsp_priv;

	ky_dwmac_set_speed(ky_plat_dat->sys_gmac_cfg,
				ky_plat_dat->interface, speed);
}

/**
 * dwmac1000_validate_mcast_bins - validates the number of Multicast filter bins
 * @mcast_bins: Multicast filtering bins
 * Description:
 * this function validates the number of Multicast filtering bins specified
 * by the configuration through the device tree. The Synopsys GMAC supports
 * 64 bins, 128 bins, or 256 bins. "bins" refer to the division of CRC
 * number space. 64 bins correspond to 6 bits of the CRC, 128 corresponds
 * to 7 bits, and 256 refers to 8 bits of the CRC. Any other setting is
 * invalid and will cause the filtering algorithm to use Multicast
 * promiscuous mode.
 */
static int dwmac1000_validate_mcast_bins(int mcast_bins)
{
	int x = mcast_bins;

	switch (x) {
	case HASH_TABLE_SIZE:
	case 128:
	case 256:
		break;
	default:
		x = 0;
		pr_info("Hash table entries set to unexpected value %d",
			mcast_bins);
		break;
	}
	return x;
}

/**
 * dwmac1000_validate_ucast_entries - validate the Unicast address entries
 * @ucast_entries: number of Unicast address entries
 * Description:
 * This function validates the number of Unicast address entries supported
 * by a particular Synopsys 10/100/1000 controller. The Synopsys controller
 * supports 1..32, 64, or 128 Unicast filter entries for it's Unicast filter
 * logic. This function validates a valid, supported configuration is
 * selected, and defaults to 1 Unicast address if an unsupported
 * configuration is selected.
 */
static int dwmac1000_validate_ucast_entries(int ucast_entries)
{
	int x = ucast_entries;

	switch (x) {
	case 1 ... 32:
	case 64:
	case 128:
		break;
	default:
		x = 1;
		pr_info("Unicast table entries set to unexpected value %d\n",
			ucast_entries);
		break;
	}
	return x;
}

static int ky_dwmac_probe(struct platform_device *pdev)
{
	struct plat_stmmacenet_data *plat_dat;
	struct stmmac_resources stmmac_res;
	struct ky_dwmac_priv_data *ky_plat_dat;
	const struct ky_dwmac_data *data;
    struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	int ret;

    ky_plat_dat = devm_kzalloc(dev, sizeof(*ky_plat_dat), GFP_KERNEL);
	if (ky_plat_dat == NULL) {
		dev_err(&pdev->dev, "allocate memory failed\n");
		return -ENOMEM;
	}

    ret = stmmac_get_platform_resources(pdev, &stmmac_res);
	if (ret)
		return ret;

	if (pdev->dev.of_node) {
		plat_dat = stmmac_probe_config_dt(pdev, stmmac_res.mac);
		if (IS_ERR(plat_dat)) {
			dev_err(&pdev->dev, "dt configuration failed\n");
			return PTR_ERR(plat_dat);
		}
	} else {
		plat_dat = dev_get_platdata(&pdev->dev);
		if (!plat_dat) {
			dev_err(&pdev->dev, "no platform data provided\n");
			return  -EINVAL;
		}

		/* Set default value for multicast hash bins */
		plat_dat->multicast_filter_bins = HASH_TABLE_SIZE;

		/* Set default value for unicast filter entries */
		plat_dat->unicast_filter_entries = 1;
	}

    data = of_device_get_match_data(&pdev->dev);
	if (!data) {
		dev_err(&pdev->dev, "failed to get match data\n");
		ret = -EINVAL;
		goto err_match_data;
	}
    ky_plat_dat->data = data;

	/* Custom initialisation (if needed) */
	if (plat_dat->init) {
		ret = plat_dat->init(pdev, plat_dat->bsp_priv);
		if (ret)
			goto err_remove_config_dt;
	}

    /* populate bsp private data */
	plat_dat->bsp_priv = ky_plat_dat;
	plat_dat->fix_mac_speed = ky_dwmac_fix_speed;
	of_property_read_u32(np, "max-frame-size", &plat_dat->maxmtu);
	of_property_read_u32(np, "snps,multicast-filter-bins",
			     &plat_dat->multicast_filter_bins);
	of_property_read_u32(np, "snps,perfect-filter-entries",
			     &plat_dat->unicast_filter_entries);
	plat_dat->unicast_filter_entries = dwmac1000_validate_ucast_entries(
				       plat_dat->unicast_filter_entries);
	plat_dat->multicast_filter_bins = dwmac1000_validate_mcast_bins(
				      plat_dat->multicast_filter_bins);
	plat_dat->has_gmac = 1;
	plat_dat->pmt = 1;
	plat_dat->multi_msi_en = 0;
	plat_dat->addr64 = ky_plat_dat->data->addr_width;
	plat_dat->enh_desc = 1;

	ret = ky_dwmac_init(pdev, plat_dat->bsp_priv);
	if (ret)
		goto err_exit;

	ret = stmmac_dvr_probe(&pdev->dev, plat_dat, &stmmac_res);
	if (ret)
		goto err_exit;

err_exit:
	if (plat_dat->exit)
		plat_dat->exit(pdev, plat_dat->bsp_priv);
err_remove_config_dt:
err_match_data:
	if (pdev->dev.of_node)
		stmmac_remove_config_dt(pdev, plat_dat);

	return ret;
}

static int ky_dwmac_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct ky_dwmac_priv_data *ky_plat_dat = priv->plat->bsp_priv;

	stmmac_pltfr_remove(pdev);
	reset_control_assert(ky_plat_dat->gmac_dma_rstc);
	reset_control_assert(ky_plat_dat->gmac_csr_rstc);

	return 0;
}

static struct platform_driver ky_dwmac_driver = {
	.probe  = ky_dwmac_probe,
	.remove = ky_dwmac_remove,
	.driver = {
		.name           = "ky_dwmac_eth",
		.pm		= &stmmac_pltfr_pm_ops,
		.of_match_table = of_match_ptr(ky_dwmac_match),
	},
};
module_platform_driver(ky_dwmac_driver);

MODULE_DESCRIPTION("Ky DWMAC specific glue layer");
MODULE_LICENSE("GPL v2");
