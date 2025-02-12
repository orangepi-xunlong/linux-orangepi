// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2023, ky Corporation.

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/phy.h>
#include "stmmac.h"
#include "stmmac_platform.h"

struct ky_ethqos {
	struct device *dev;
	void __iomem *sys_gmac_cfg;

    unsigned int speed;
	struct reset_control *gmac_csr_rstc;
	struct reset_control *gmac_dma_rstc;
	struct clk *clk_master;
	struct clk *clk_slave;
	struct clk *clk_tx;
	struct clk *clk_rx;

	struct gpio_desc *reset;
};

static int dwc_eth_dwmac_config_dt(struct platform_device *pdev,
				   struct plat_stmmacenet_data *plat_dat)
{
	struct device *dev = &pdev->dev;
	u32 burst_map = 0;
	u32 bit_index = 0;
	u32 a_index = 0;

	if (!plat_dat->axi) {
		plat_dat->axi = kzalloc(sizeof(struct stmmac_axi), GFP_KERNEL);

		if (!plat_dat->axi)
			return -ENOMEM;
	}

	plat_dat->axi->axi_lpi_en = device_property_read_bool(dev,
							      "snps,en-lpi");
	if (device_property_read_u32(dev, "snps,write-requests",
				     &plat_dat->axi->axi_wr_osr_lmt)) {
		/**
		 * Since the register has a reset value of 1, if property
		 * is missing, default to 1.
		 */
		plat_dat->axi->axi_wr_osr_lmt = 1;
	} else {
		/**
		 * If property exists, to keep the behavior from dwc_eth_qos,
		 * subtract one after parsing.
		 */
		plat_dat->axi->axi_wr_osr_lmt--;
	}

	if (device_property_read_u32(dev, "snps,read-requests",
				     &plat_dat->axi->axi_rd_osr_lmt)) {
		/**
		 * Since the register has a reset value of 1, if property
		 * is missing, default to 1.
		 */
		plat_dat->axi->axi_rd_osr_lmt = 1;
	} else {
		/**
		 * If property exists, to keep the behavior from dwc_eth_qos,
		 * subtract one after parsing.
		 */
		plat_dat->axi->axi_rd_osr_lmt--;
	}
	device_property_read_u32(dev, "snps,burst-map", &burst_map);

	/* converts burst-map bitmask to burst array */
	for (bit_index = 0; bit_index < 7; bit_index++) {
		if (burst_map & (1 << bit_index)) {
			switch (bit_index) {
			case 0:
			plat_dat->axi->axi_blen[a_index] = 4; break;
			case 1:
			plat_dat->axi->axi_blen[a_index] = 8; break;
			case 2:
			plat_dat->axi->axi_blen[a_index] = 16; break;
			case 3:
			plat_dat->axi->axi_blen[a_index] = 32; break;
			case 4:
			plat_dat->axi->axi_blen[a_index] = 64; break;
			case 5:
			plat_dat->axi->axi_blen[a_index] = 128; break;
			case 6:
			plat_dat->axi->axi_blen[a_index] = 256; break;
			default:
			break;
			}
			a_index++;
		}
	}

	/* dwc-qos needs GMAC4, AAL, TSO and PMT */
	plat_dat->has_gmac4 = 1;
	plat_dat->dma_cfg->aal = 1;
	plat_dat->tso_en = 1;
	plat_dat->pmt = 1;

	return 0;
}

struct dwc_eth_dwmac_data {
	int (*probe)(struct platform_device *pdev,
		     struct plat_stmmacenet_data *data,
		     struct stmmac_resources *res);
	int (*remove)(struct platform_device *pdev);
};

static int dwc_qos_probe(struct platform_device *pdev,
			 struct plat_stmmacenet_data *plat_dat,
			 struct stmmac_resources *stmmac_res)
{
    int err;

	plat_dat->stmmac_clk = devm_clk_get(&pdev->dev, "apb_pclk");
	if (IS_ERR(plat_dat->stmmac_clk)) {
		dev_err(&pdev->dev, "apb_pclk clock not found.\n");
		return PTR_ERR(plat_dat->stmmac_clk);
	}

	err = clk_prepare_enable(plat_dat->stmmac_clk);
	if (err < 0) {
		dev_err(&pdev->dev, "failed to enable apb_pclk clock: %d\n",
			err);
		return err;
	}

	plat_dat->pclk = devm_clk_get(&pdev->dev, "phy_ref_clk");
	if (IS_ERR(plat_dat->pclk)) {
		dev_err(&pdev->dev, "phy_ref_clk clock not found.\n");
		err = PTR_ERR(plat_dat->pclk);
		goto disable;
	}

	err = clk_prepare_enable(plat_dat->pclk);
	if (err < 0) {
		dev_err(&pdev->dev, "failed to enable phy_ref clock: %d\n",
			err);
		goto disable;
	}

	return 0;

disable:
	clk_disable_unprepare(plat_dat->stmmac_clk);
	return err;
}

static int dwc_qos_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct stmmac_priv *priv = netdev_priv(ndev);

	clk_disable_unprepare(priv->plat->pclk);
	clk_disable_unprepare(priv->plat->stmmac_clk);

	return 0;
}

static const struct dwc_eth_dwmac_data dwc_qos_data = {
	.probe = dwc_qos_probe,
	.remove = dwc_qos_remove,
};

static void ethqos_fix_mac_speed(void *priv, unsigned int speed)
{
	struct ky_ethqos *ethqos = priv;
    volatile unsigned int reg;

	ethqos->speed = speed;

    switch(speed) {
	case SPEED_1000:
        reg = readl(ethqos->sys_gmac_cfg);
		reg &= ~BIT(0);
		writel(reg, ethqos->sys_gmac_cfg);
        break;

    case SPEED_100:
		reg = readl(ethqos->sys_gmac_cfg);
		reg |= BIT(0);
		writel(reg, ethqos->sys_gmac_cfg);
		break;

    default:
    }
}

static void ethqos_dump(void *priv)
{
	struct ky_ethqos *ethqos = priv;

	dev_dbg(ethqos->dev, "sys_gmac_cfg register dump\n");
	dev_dbg(ethqos->dev, "sys_gmac_cfg: %x\n",
		readl(ethqos->sys_gmac_cfg));
}

static int ky_ethqos_probe(struct platform_device *pdev)
{
	struct plat_stmmacenet_data *plat_dat;
	struct stmmac_resources stmmac_res;
	const struct dwc_eth_dwmac_data *data;
	struct ky_ethqos *ethqos;
	int ret;

    data = device_get_match_data(&pdev->dev);

    memset(&stmmac_res, 0, sizeof(struct stmmac_resources));
	ret = stmmac_get_platform_resources(pdev, &stmmac_res);
	if (ret)
		return ret;

	plat_dat = stmmac_probe_config_dt(pdev, stmmac_res.mac);
	if (IS_ERR(plat_dat)) {
		dev_err(&pdev->dev, "dt configuration failed\n");
		return PTR_ERR(plat_dat);
	}

	ethqos = devm_kzalloc(&pdev->dev, sizeof(*ethqos), GFP_KERNEL);
	if (!ethqos) {
		ret = -ENOMEM;
		goto err_mem;
	}
    ethqos->dev = &pdev->dev;

    ethqos->sys_gmac_cfg = devm_platform_ioremap_resource_byname(pdev, "sys_gmac_cfg");
    if (IS_ERR(ethqos->sys_gmac_cfg)) {
		ret = PTR_ERR(ethqos->sys_gmac_cfg);
		goto err_mem;
	}

    plat_dat->bsp_priv = ethqos;
	plat_dat->fix_mac_speed = ethqos_fix_mac_speed;
    plat_dat->dump_debug_regs = ethqos_dump;
    ret = data->probe(pdev, plat_dat, &stmmac_res);
	if (ret < 0) {
		dev_err_probe(&pdev->dev, ret, "failed to probe subdriver\n");
		goto err_mem;
	}

    ret = dwc_eth_dwmac_config_dt(pdev, plat_dat);
	if (ret)
		goto err_mem;

    ethqos->gmac_csr_rstc = devm_reset_control_get_exclusive(&pdev->dev, "gmac_csr");
    ethqos->gmac_dma_rstc = devm_reset_control_get_exclusive(&pdev->dev, "gmac_dma");
    if (IS_ERR(ethqos->gmac_csr_rstc) \
		|| IS_ERR(ethqos->gmac_dma_rstc)) {
		dev_err(&pdev->dev, "failed to get reset.\n");
		goto err_mem;
	}
    reset_control_deassert(ethqos->gmac_csr_rstc);
	reset_control_deassert(ethqos->gmac_dma_rstc);

	ret = stmmac_dvr_probe(&pdev->dev, plat_dat, &stmmac_res);
	if (ret)
		goto err_clk;

	return ret;

err_clk:
    reset_control_assert(ethqos->gmac_dma_rstc);
	reset_control_assert(ethqos->gmac_csr_rstc);
	data->remove(pdev);

err_mem:
	stmmac_remove_config_dt(pdev, plat_dat);

	return ret;
}

static int ky_ethqos_remove(struct platform_device *pdev)
{
	struct ky_ethqos *ethqos = get_stmmac_bsp_priv(&pdev->dev);
    struct net_device *ndev = platform_get_drvdata(pdev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	const struct dwc_eth_dwmac_data *data;
	int err;

	data = device_get_match_data(&pdev->dev);

	err = stmmac_dvr_remove(&pdev->dev);
	if (err < 0)
		dev_err(&pdev->dev, "failed to remove platform: %d\n", err);

	if (ethqos) {
        reset_control_assert(ethqos->gmac_dma_rstc);
		reset_control_assert(ethqos->gmac_csr_rstc);
	}

	err = data->remove(pdev);
	if (err < 0)
		dev_err(&pdev->dev, "failed to remove subdriver: %d\n", err);

	stmmac_remove_config_dt(pdev, priv->plat);

	return err;
}

static const struct of_device_id ky_ethqos_match[] = {
	{ .compatible = "ky,x1-pro-ethqos", .data = &dwc_qos_data},
    {}
};
MODULE_DEVICE_TABLE(of, ky_ethqos_match);

static struct platform_driver ky_ethqos_driver = {
	.probe  = ky_ethqos_probe,
	.remove = ky_ethqos_remove,
	.driver = {
		.name           = "ky-ethqos",
		.pm		= &stmmac_pltfr_pm_ops,
		.of_match_table = of_match_ptr(ky_ethqos_match),
	},
};
module_platform_driver(ky_ethqos_driver);

MODULE_DESCRIPTION("Ky Quality-of-Service driver");
MODULE_LICENSE("GPL v2");