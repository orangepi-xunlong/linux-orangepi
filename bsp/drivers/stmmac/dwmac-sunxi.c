/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
* Allwinner DWMAC driver.
*
* Copyright(c) 2022-2027 Allwinnertech Co., Ltd.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/
#define SUNXI_MODNAME "stmmac"
#include <sunxi-log.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/mdio-mux.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_mdio.h>
#include <linux/of_net.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/regmap.h>
#include <linux/stmmac.h>
#include <sunxi-sid.h>

#include "stmmac.h"
#include "stmmac_platform.h"

#include "dwmac-sunxi.h"

#define DWMAC_MODULE_VERSION		"0.3.2"

#define MAC_ADDR_LEN			18
#define SUNXI_DWMAC_MAC_ADDRESS		"00:00:00:00:00:00"
#define MAC_IRQ_NAME			8

static char mac_str[MAC_ADDR_LEN] = SUNXI_DWMAC_MAC_ADDRESS;
module_param_string(mac_str, mac_str, MAC_ADDR_LEN, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mac_str, "MAC Address String.(xx:xx:xx:xx:xx:xx)");

#ifdef MODULE
extern int get_custom_mac_address(int fmt, char *name, char *addr);
#endif

static int sunxi_dwmac200_set_syscon(struct sunxi_dwmac *chip)
{
	u32 reg_val = 0;

	/* Clear interface mode bits */
	reg_val &= ~(SUNXI_DWMAC200_SYSCON_ETCS | SUNXI_DWMAC200_SYSCON_EPIT);
	if (chip->variant->interface & PHY_INTERFACE_MODE_RMII)
		reg_val &= ~SUNXI_DWMAC200_SYSCON_RMII_EN;

	switch (chip->interface) {
	case PHY_INTERFACE_MODE_MII:
		/* default */
		break;
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
		reg_val |= SUNXI_DWMAC200_SYSCON_EPIT;
		reg_val |= FIELD_PREP(SUNXI_DWMAC200_SYSCON_ETCS,
					chip->rgmii_clk_ext ? SUNXI_DWMAC_ETCS_EXT_GMII : SUNXI_DWMAC_ETCS_INT_GMII);
		if (chip->rgmii_clk_ext)
			sunxi_info(chip->dev, "RGMII use external transmit clock\n");
		else
			sunxi_info(chip->dev, "RGMII use internal transmit clock\n");
		break;
	case PHY_INTERFACE_MODE_RMII:
		reg_val |= SUNXI_DWMAC200_SYSCON_RMII_EN;
		reg_val &= ~SUNXI_DWMAC200_SYSCON_ETCS;
		break;
	default:
		sunxi_err(chip->dev, "Unsupported interface mode: %s", phy_modes(chip->interface));
		return -EINVAL;
	}

	writel(reg_val, chip->syscfg_base + SUNXI_DWMAC200_SYSCON_REG);
	return 0;
}

static int sunxi_dwmac200_set_delaychain(struct sunxi_dwmac *chip, enum sunxi_dwmac_delaychain_dir dir, u32 delay)
{
	u32 reg_val = readl(chip->syscfg_base + SUNXI_DWMAC200_SYSCON_REG);
	int ret = -EINVAL;

	switch (dir) {
	case SUNXI_DWMAC_DELAYCHAIN_TX:
		if (delay <= chip->variant->tx_delay_max) {
			reg_val &= ~SUNXI_DWMAC200_SYSCON_ETXDC;
			reg_val |= FIELD_PREP(SUNXI_DWMAC200_SYSCON_ETXDC, delay);
			ret = 0;
		}
		break;
	case SUNXI_DWMAC_DELAYCHAIN_RX:
		if (delay <= chip->variant->rx_delay_max) {
			reg_val &= ~SUNXI_DWMAC200_SYSCON_ERXDC;
			reg_val |= FIELD_PREP(SUNXI_DWMAC200_SYSCON_ERXDC, delay);
			ret = 0;
		}
		break;
	}

	if (!ret)
		writel(reg_val, chip->syscfg_base + SUNXI_DWMAC200_SYSCON_REG);

	return ret;
}

static u32 sunxi_dwmac200_get_delaychain(struct sunxi_dwmac *chip, enum sunxi_dwmac_delaychain_dir dir)
{
	u32 delay = 0;
	u32 reg_val = readl(chip->syscfg_base + SUNXI_DWMAC200_SYSCON_REG);

	switch (dir) {
	case SUNXI_DWMAC_DELAYCHAIN_TX:
		delay = FIELD_GET(SUNXI_DWMAC200_SYSCON_ETXDC, reg_val);
		break;
	case SUNXI_DWMAC_DELAYCHAIN_RX:
		delay = FIELD_GET(SUNXI_DWMAC200_SYSCON_ERXDC, reg_val);
		break;
	default:
		sunxi_err(chip->dev, "Unknow delaychain dir %d\n", dir);
	}

	return delay;
}

static int sunxi_dwmac210_set_delaychain(struct sunxi_dwmac *chip, enum sunxi_dwmac_delaychain_dir dir, u32 delay)
{
	u32 reg_val = readl(chip->syscfg_base + SUNXI_DWMAC210_CFG_REG);
	int ret = -EINVAL;

	switch (dir) {
	case SUNXI_DWMAC_DELAYCHAIN_TX:
		if (delay <= chip->variant->tx_delay_max) {
			reg_val &= ~(SUNXI_DWMAC210_CFG_ETXDC_H | SUNXI_DWMAC210_CFG_ETXDC_L);
			reg_val |= FIELD_PREP(SUNXI_DWMAC210_CFG_ETXDC_H, delay >> 3);
			reg_val |= FIELD_PREP(SUNXI_DWMAC210_CFG_ETXDC_L, delay);
			ret = 0;
		}
		break;
	case SUNXI_DWMAC_DELAYCHAIN_RX:
		if (delay <= chip->variant->rx_delay_max) {
			reg_val &= ~SUNXI_DWMAC210_CFG_ERXDC;
			reg_val |= FIELD_PREP(SUNXI_DWMAC210_CFG_ERXDC, delay);
			ret = 0;
		}
		break;
	}

	if (!ret)
		writel(reg_val, chip->syscfg_base + SUNXI_DWMAC210_CFG_REG);

	return ret;
}

static u32 sunxi_dwmac210_get_delaychain(struct sunxi_dwmac *chip, enum sunxi_dwmac_delaychain_dir dir)
{
	u32 delay = 0;
	u32 tx_l, tx_h;
	u32 reg_val = readl(chip->syscfg_base + SUNXI_DWMAC210_CFG_REG);

	switch (dir) {
	case SUNXI_DWMAC_DELAYCHAIN_TX:
		tx_h = FIELD_GET(SUNXI_DWMAC210_CFG_ETXDC_H, reg_val);
		tx_l = FIELD_GET(SUNXI_DWMAC210_CFG_ETXDC_L, reg_val);
		delay = (tx_h << 3 | tx_l);
		break;
	case SUNXI_DWMAC_DELAYCHAIN_RX:
		delay = FIELD_GET(SUNXI_DWMAC210_CFG_ERXDC, reg_val);
		break;
	}

	return delay;
}

static int sunxi_dwmac110_get_version(struct sunxi_dwmac *chip, u16 *ip_tag, u16 *ip_vrm)
{
	u32 reg_val;

	if (!ip_tag || !ip_vrm)
		return -EINVAL;

	reg_val = readl(chip->syscfg_base + SUNXI_DWMAC110_VERSION_REG);
	*ip_tag = FIELD_GET(SUNXI_DWMAC110_VERSION_IP_TAG, reg_val);
	*ip_vrm = FIELD_GET(SUNXI_DWMAC110_VERSION_IP_VRM, reg_val);
	return 0;
}

static int sunxi_dwmac_power_on(struct sunxi_dwmac *chip)
{
	int ret;

	/* set dwmac pin bank voltage to 3.3v */
	if (!IS_ERR(chip->dwmac3v3_supply)) {
		ret = regulator_set_voltage(chip->dwmac3v3_supply, 3300000, 3300000);
		if (ret) {
			sunxi_err(chip->dev, "Set dwmac3v3-supply voltage 3300000 failed %d\n", ret);
			goto err_dwmac3v3;
		}

		ret = regulator_enable(chip->dwmac3v3_supply);
		if (ret) {
			sunxi_err(chip->dev, "Enable dwmac3v3-supply failed %d\n", ret);
			goto err_dwmac3v3;
		}
	}

	/* set phy voltage to 3.3v */
	if (!IS_ERR(chip->phy3v3_supply)) {
		ret = regulator_set_voltage(chip->phy3v3_supply, 3300000, 3300000);
		if (ret) {
			sunxi_err(chip->dev, "Set phy3v3-supply voltage 3300000 failed %d\n", ret);
			goto err_phy3v3;
		}

		ret = regulator_enable(chip->phy3v3_supply);
		if (ret) {
			sunxi_err(chip->dev, "Enable phy3v3-supply failed\n");
			goto err_phy3v3;
		}
	}

	return 0;

err_phy3v3:
	regulator_disable(chip->dwmac3v3_supply);
err_dwmac3v3:
	return ret;
}

static void sunxi_dwmac_power_off(struct sunxi_dwmac *chip)
{
	if (!IS_ERR(chip->phy3v3_supply))
		regulator_disable(chip->phy3v3_supply);
	if (!IS_ERR(chip->dwmac3v3_supply))
		regulator_disable(chip->dwmac3v3_supply);
}

static int sunxi_dwmac_clk_init(struct sunxi_dwmac *chip)
{
	int ret;

	if (chip->variant->flags & SUNXI_DWMAC_HSI_CLK_GATE)
		reset_control_deassert(chip->hsi_rst);
	reset_control_deassert(chip->ahb_rst);

	if (chip->variant->flags & SUNXI_DWMAC_HSI_CLK_GATE) {
		ret = clk_prepare_enable(chip->hsi_ahb);
		if (ret) {
			sunxi_err(chip->dev, "enable hsi_ahb failed\n");
			goto err_ahb;
		}
		ret = clk_prepare_enable(chip->hsi_axi);
		if (ret) {
			sunxi_err(chip->dev, "enable hsi_axi failed\n");
			goto err_axi;
		}
	}

	if (chip->variant->flags & SUNXI_DWMAC_NSI_CLK_GATE) {
		ret = clk_prepare_enable(chip->nsi_clk);
		if (ret) {
			sunxi_err(chip->dev, "enable nsi clk failed\n");
			goto err_nsi;
		}
	}

	if (chip->soc_phy_clk_en) {
		ret = clk_prepare_enable(chip->phy_clk);
		if (ret) {
			sunxi_err(chip->dev, "Enable phy clk failed\n");
			goto err_phy;
		}
	}

	return 0;

err_phy:
	if (chip->variant->flags & SUNXI_DWMAC_NSI_CLK_GATE)
		clk_disable_unprepare(chip->nsi_clk);
err_nsi:
	if (chip->variant->flags & SUNXI_DWMAC_HSI_CLK_GATE) {
		clk_disable_unprepare(chip->hsi_axi);
err_axi:
		clk_disable_unprepare(chip->hsi_ahb);
	}
err_ahb:
	reset_control_assert(chip->ahb_rst);
	if (chip->variant->flags & SUNXI_DWMAC_HSI_CLK_GATE)
		reset_control_assert(chip->hsi_rst);
	return ret;
}

static void sunxi_dwmac_clk_exit(struct sunxi_dwmac *chip)
{
	if (chip->soc_phy_clk_en)
		clk_disable_unprepare(chip->phy_clk);
	if (chip->variant->flags & SUNXI_DWMAC_NSI_CLK_GATE)
		clk_disable_unprepare(chip->nsi_clk);
	if (chip->variant->flags & SUNXI_DWMAC_HSI_CLK_GATE) {
		clk_disable_unprepare(chip->hsi_axi);
		clk_disable_unprepare(chip->hsi_ahb);
	}
	reset_control_assert(chip->ahb_rst);
	if (chip->variant->flags & SUNXI_DWMAC_HSI_CLK_GATE)
		reset_control_assert(chip->hsi_rst);
}

static int sunxi_dwmac_hw_init(struct sunxi_dwmac *chip)
{
	int ret;

	ret = chip->variant->set_syscon(chip);
	if (ret < 0) {
		sunxi_err(chip->dev, "Set syscon failed\n");
		goto err;
	}

	ret = chip->variant->set_delaychain(chip, SUNXI_DWMAC_DELAYCHAIN_TX, chip->tx_delay);
	if (ret < 0) {
		sunxi_err(chip->dev, "Invalid TX clock delay: %d\n", chip->tx_delay);
		goto err;
	}

	ret = chip->variant->set_delaychain(chip, SUNXI_DWMAC_DELAYCHAIN_RX, chip->rx_delay);
	if (ret < 0) {
		sunxi_err(chip->dev, "Invalid RX clock delay: %d\n", chip->rx_delay);
		goto err;
	}

err:
	return ret;
}

static void sunxi_dwmac_hw_exit(struct sunxi_dwmac *chip)
{
	writel(0, chip->syscfg_base);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0))
static int sunxi_dwmac_ecc_init(struct sunxi_dwmac *chip)
{
	struct net_device *ndev = dev_get_drvdata(chip->dev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct plat_stmmacenet_data *plat_dat = priv->plat;

	plat_dat->safety_feat_cfg = devm_kzalloc(chip->dev, sizeof(*plat_dat->safety_feat_cfg), GFP_KERNEL);
	if (!plat_dat->safety_feat_cfg)
		return -ENOMEM;

	plat_dat->safety_feat_cfg->tsoee	= 0; /* TSO memory ECC Disabled */
	plat_dat->safety_feat_cfg->mrxpee	= 0; /* MTL Rx Parser ECC Disabled */
	plat_dat->safety_feat_cfg->mestee	= 0; /* MTL EST ECC Disabled */
	plat_dat->safety_feat_cfg->mrxee	= 1; /* MTL Rx FIFO ECC Enable */
	plat_dat->safety_feat_cfg->mtxee	= 1; /* MTL Tx FIFO ECC Enable */
	plat_dat->safety_feat_cfg->epsi		= 0; /* Not Enable Parity on Slave Interface port */
	plat_dat->safety_feat_cfg->edpp		= 1; /* Enable Data path Parity Protection */
	plat_dat->safety_feat_cfg->prtyen	= 1; /* Enable FSM parity feature */
	plat_dat->safety_feat_cfg->tmouten	= 1; /* Enable FSM timeout feature */

	return 0;
}
#endif

static int sunxi_dwmac_init(struct platform_device *pdev, void *priv)
{
	struct sunxi_dwmac *chip = priv;
	int ret;

	ret = sunxi_dwmac_power_on(chip);
	if (ret) {
		sunxi_err(&pdev->dev, "Power on dwmac failed\n");
		return ret;
	}

	ret = sunxi_dwmac_clk_init(chip);
	if (ret) {
		sunxi_err(&pdev->dev, "Clk init dwmac failed\n");
		goto err_clk;
	}

	ret = sunxi_dwmac_hw_init(chip);
	if (ret)
		sunxi_warn(&pdev->dev, "Hw init dwmac failed\n");

	return 0;

err_clk:
	sunxi_dwmac_power_off(chip);
	return ret;
}

static void sunxi_dwmac_exit(struct platform_device *pdev, void *priv)
{
	struct sunxi_dwmac *chip = priv;

	sunxi_dwmac_hw_exit(chip);
	sunxi_dwmac_clk_exit(chip);
	sunxi_dwmac_power_off(chip);
}

static void sunxi_dwmac_parse_delay_maps(struct sunxi_dwmac *chip)
{
	struct platform_device *pdev = to_platform_device(chip->dev);
	struct device_node *np = pdev->dev.of_node;
	int ret, maps_cnt, i;
	const u8 array_size = 3;
	u32 *maps;
	u16 soc_ver;

	maps_cnt = of_property_count_elems_of_size(np, "delay-maps", sizeof(u32));
	if (maps_cnt <= 0) {
		sunxi_info(&pdev->dev, "Not found delay-maps in dts\n");
		return;
	}

	maps = devm_kcalloc(&pdev->dev, maps_cnt, sizeof(u32), GFP_KERNEL);
	if (!maps)
		return;

	ret = of_property_read_u32_array(np, "delay-maps", maps, maps_cnt);
	if (ret) {
		sunxi_err(&pdev->dev, "Failed to parse delay-maps\n");
		goto err_parse_maps;
	}

	soc_ver = (u16)sunxi_get_soc_ver();
	for (i = 0; i < (maps_cnt / array_size); i++) {
		if (soc_ver == maps[i * array_size]) {
			chip->rx_delay = maps[i * array_size + 1];
			chip->tx_delay = maps[i * array_size + 2];
			sunxi_info(&pdev->dev, "Overwrite delay-maps parameters, rx-delay:%d, tx-delay:%d\n",
					chip->rx_delay, chip->tx_delay);
		}
	}

err_parse_maps:
	devm_kfree(&pdev->dev, maps);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0))
static void sunxi_dwmac_request_mtl_irq(struct platform_device *pdev, struct sunxi_dwmac *chip,
		struct plat_stmmacenet_data *plat_dat)
{
	u32 queues;
	char int_name[MAC_IRQ_NAME];

	for (queues = 0; queues < plat_dat->tx_queues_to_use; queues++) {
		sprintf(int_name, "%s%d_%s", "tx", queues, "irq");
		chip->res->tx_irq[queues] = platform_get_irq_byname_optional(pdev, int_name);
		if (chip->res->tx_irq[queues] < 0)
			chip->res->tx_irq[queues] = 0;
	}

	for (queues = 0; queues < plat_dat->rx_queues_to_use; queues++) {
		sprintf(int_name, "%s%d_%s", "rx", queues, "irq");
		chip->res->rx_irq[queues] = platform_get_irq_byname_optional(pdev, int_name);
		if (chip->res->rx_irq[queues] < 0)
			chip->res->rx_irq[queues] = 0;
	}
}
#endif

static int sunxi_dwmac_resource_get(struct platform_device *pdev, struct sunxi_dwmac *chip,
		struct plat_stmmacenet_data *plat_dat)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct resource *res;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		sunxi_err(dev, "Get phy memory failed\n");
		return -ENODEV;
	}

	chip->syscfg_base = devm_ioremap_resource(dev, res);
	if (!chip->syscfg_base) {
		sunxi_err(dev, "Phy memory mapping failed\n");
		return -ENOMEM;
	}

	chip->rgmii_clk_ext	= of_property_read_bool(np, "aw,rgmii-clk-ext");
	chip->soc_phy_clk_en = of_property_read_bool(np, "aw,soc-phy-clk-en") ||
							of_property_read_bool(np, "aw,soc-phy25m");
	if (chip->soc_phy_clk_en) {
		chip->phy_clk = devm_clk_get(dev, "phy");
		if (IS_ERR(chip->phy_clk)) {
			chip->phy_clk = devm_clk_get(dev, "phy25m");
			if (IS_ERR(chip->phy_clk)) {
				sunxi_err(dev, "Get phy25m clk failed\n");
				return -EINVAL;
			}
		}
		sunxi_info(dev, "Phy use soc fanout\n");
	} else
		sunxi_info(dev, "Phy use ext osc\n");

	if (chip->variant->flags & SUNXI_DWMAC_HSI_CLK_GATE) {
		chip->hsi_ahb = devm_clk_get(dev, "hsi_ahb");
		if (IS_ERR(chip->hsi_ahb)) {
			sunxi_err(dev, "Get hsi_ahb clk failed\n");
			return -EINVAL;
		}
		chip->hsi_axi = devm_clk_get(dev, "hsi_axi");
		if (IS_ERR(chip->hsi_axi)) {
			sunxi_err(dev, "Get hsi_axi clk failed\n");
			return -EINVAL;
		}
	}

	if (chip->variant->flags & SUNXI_DWMAC_NSI_CLK_GATE) {
		chip->nsi_clk = devm_clk_get(dev, "nsi");
		if (IS_ERR(chip->nsi_clk)) {
			sunxi_err(dev, "Get nsi clk failed\n");
			return -EINVAL;
		}
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	if (chip->variant->flags & SUNXI_DWMAC_MEM_ECC) {
		sunxi_info(dev, "Support mem ecc\n");
		chip->res->sfty_ce_irq = platform_get_irq_byname_optional(pdev, "mac_eccirq");
		if (chip->res->sfty_ce_irq < 0) {
			sunxi_err(&pdev->dev, "Get ecc irq failed\n");
			return -EINVAL;
		}
	}
#endif

	if (chip->variant->flags & SUNXI_DWMAC_HSI_CLK_GATE) {
		chip->hsi_rst = devm_reset_control_get_shared(chip->dev, "hsi");
		if (IS_ERR(chip->hsi_rst)) {
			sunxi_err(dev, "Get hsi reset failed\n");
			return -EINVAL;
		}
	}

	chip->ahb_rst = devm_reset_control_get_optional_shared(chip->dev, "ahb");
	if (IS_ERR(chip->ahb_rst)) {
		sunxi_err(dev, "Get mac reset failed\n");
		return -EINVAL;
	}

	chip->dwmac3v3_supply = devm_regulator_get_optional(&pdev->dev, "dwmac3v3");
	if (IS_ERR(chip->dwmac3v3_supply))
		sunxi_warn(dev, "Not found dwmac3v3-supply\n");

	chip->phy3v3_supply = devm_regulator_get_optional(&pdev->dev, "phy3v3");
	if (IS_ERR(chip->phy3v3_supply))
		sunxi_warn(dev, "Not found phy3v3-supply\n");

	ret = of_property_read_u32(np, "tx-delay", &chip->tx_delay);
	if (ret) {
		sunxi_warn(dev, "Get gmac tx-delay failed, use default 0\n");
		chip->tx_delay = 0;
	}

	ret = of_property_read_u32(np, "rx-delay", &chip->rx_delay);
	if (ret) {
		sunxi_warn(dev, "Get gmac rx-delay failed, use default 0\n");
		chip->rx_delay = 0;
	}

	sunxi_dwmac_parse_delay_maps(chip);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0))
	if (chip->variant->flags & SUNXI_DWMAC_MULTI_MSI)
		sunxi_dwmac_request_mtl_irq(pdev, chip, plat_dat);
#endif

	return 0;
}

#ifndef MODULE
static void sunxi_dwmac_set_mac(u8 *dst, u8 *src)
{
	int i;
	char *p = src;

	for (i = 0; i < ETH_ALEN; i++, p++)
		dst[i] = simple_strtoul(p, &p, 16);
}
#endif

static int sunxi_dwmac_probe(struct platform_device *pdev)
{
	struct plat_stmmacenet_data *plat_dat;
	struct stmmac_resources stmmac_res;
	struct sunxi_dwmac *chip;
	struct device *dev = &pdev->dev;
	int ret;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 12, 0))
	char *mac_temp = NULL;
#endif

	ret = stmmac_get_platform_resources(pdev, &stmmac_res);
	if (ret)
		return ret;

	chip = devm_kzalloc(dev, sizeof(*chip), GFP_KERNEL);
	if (!chip) {
		sunxi_err(&pdev->dev, "Alloc sunxi dwmac err\n");
		return -ENOMEM;
	}

	chip->variant = of_device_get_match_data(&pdev->dev);
	if (!chip->variant) {
		sunxi_err(&pdev->dev, "Missing dwmac-sunxi variant\n");
		return -EINVAL;
	}

	chip->dev = dev;
	chip->res = &stmmac_res;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 12, 0))
	plat_dat = stmmac_probe_config_dt(pdev, stmmac_res.mac);
#else
	plat_dat = stmmac_probe_config_dt(pdev, &stmmac_res.mac);
	if (IS_ERR_OR_NULL(stmmac_res.mac)) {
		mac_temp = devm_kzalloc(dev, ETH_ALEN, GFP_KERNEL);
		stmmac_res.mac = mac_temp;
	}
#endif
	if (IS_ERR(plat_dat))
		return PTR_ERR(plat_dat);

	ret = sunxi_dwmac_resource_get(pdev, chip, plat_dat);
	if (ret < 0)
		return -EINVAL;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 12, 0))
#ifdef MODULE
	get_custom_mac_address(1, "eth", stmmac_res.mac);
	//printk(KERN_ERR"=========csy mac %02x:%02x:%02x:%02x:%02x:%02x\n", stmmac_res.mac[0], stmmac_res.mac[1], stmmac_res.mac[2], stmmac_res.mac[3], stmmac_res.mac[4], stmmac_res.mac[5]);
#else
	sunxi_dwmac_set_mac(stmmac_res.mac, mac_str);
	//printk(KERN_ERR"=========csy mac %02x:%02x:%02x:%02x:%02x:%02x\n", mac_str[0], mac_str[1], mac_str[2], mac_str[3], mac_str[4], mac_str[5]);
#endif
#else
	if (mac_temp) {
#ifdef MODULE
		get_custom_mac_address(1, "eth", mac_temp);
#else
		sunxi_dwmac_set_mac(mac_temp, mac_str);
#endif
	}
#endif

	plat_dat->bsp_priv = chip;
	plat_dat->init = sunxi_dwmac_init;
	plat_dat->exit = sunxi_dwmac_exit;
	/* must use 0~4G space */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0)) || \
	((LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 21)) && (LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)))
	plat_dat->host_dma_width = 32;
#else
	plat_dat->addr64 = 32;
#endif
	/* Disable Split Header (SPH) feature for sunxi platfrom as default
	 * The same issue also detect on intel platfrom, see 41eebbf90dfbcc8ad16d4755fe2cdb8328f5d4a7.
	 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
	if (chip->variant->flags & SUNXI_DWMAC_SPH_DISABLE)
		plat_dat->flags |= STMMAC_FLAG_SPH_DISABLE;
	if (chip->variant->flags & SUNXI_DWMAC_MULTI_MSI)
		plat_dat->flags |= STMMAC_FLAG_MULTI_MSI_EN;
	chip->interface = plat_dat->mac_interface;
#else
	if (chip->variant->flags & SUNXI_DWMAC_SPH_DISABLE)
		plat_dat->sph_disable = true;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0))
	if (chip->variant->flags & SUNXI_DWMAC_MULTI_MSI)
		plat_dat->multi_msi_en = true;
#endif
	chip->interface = plat_dat->interface;
#endif
	plat_dat->clk_csr = 4; /* MDC = AHB(200M)/102 = 2M */
	if (!plat_dat->has_gmac4) {
		/* force fix dwmac ip version */
		plat_dat->has_gmac4 = 1;
		plat_dat->has_gmac = 0;
		plat_dat->pmt = 1;
	}

	ret = sunxi_dwmac_init(pdev, plat_dat->bsp_priv);
	if (ret)
		goto err_init;

	ret = stmmac_dvr_probe(&pdev->dev, plat_dat, &stmmac_res);
	if (ret)
		goto err_dvr_probe;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0))
	if (chip->variant->flags & SUNXI_DWMAC_MEM_ECC) {
		ret = sunxi_dwmac_ecc_init(chip);
		if (ret < 0) {
			sunxi_err(chip->dev, "Init ecc failed\n");
			goto err_cfg;
		}
	}
#endif

	sunxi_dwmac_sysfs_init(&pdev->dev);

	sunxi_info(&pdev->dev, "probe success (Version %s)\n", DWMAC_MODULE_VERSION);

	return 0;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0))
err_cfg:
	stmmac_dvr_remove(&pdev->dev);
#endif
err_dvr_probe:
	sunxi_dwmac_exit(pdev, chip);
err_init:
	stmmac_remove_config_dt(pdev, plat_dat);
	return ret;
}

static int sunxi_dwmac_remove(struct platform_device *pdev)
{
	sunxi_dwmac_sysfs_exit(&pdev->dev);
	stmmac_pltfr_remove(pdev);
	return 0;
}

static void sunxi_dwmac_shutdown(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct sunxi_dwmac *chip = priv->plat->bsp_priv;

	sunxi_dwmac_exit(pdev, chip);
}

static int __maybe_unused sunxi_dwmac_suspend(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_dwmac *chip = priv->plat->bsp_priv;
	int ret;

	/* suspend error workaround */
	if (ndev && ndev->phydev) {
		chip->uevent_suppress = dev_get_uevent_suppress(&ndev->phydev->mdio.dev);
		dev_set_uevent_suppress(&ndev->phydev->mdio.dev, true);
	}

	ret = stmmac_suspend(dev);
	sunxi_dwmac_exit(pdev, chip);
	stmmac_bus_clks_config(priv, false);

	sunxi_info(chip->dev, "suspend finish %d\n", ret);

	return ret;
}

static int __maybe_unused sunxi_dwmac_resume(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_dwmac *chip = priv->plat->bsp_priv;
	int ret;

	stmmac_bus_clks_config(priv, true);
	sunxi_dwmac_init(pdev, chip);
	if (ndev && ndev->phydev) {
		phy_device_reset(ndev->phydev, 1);
		phy_device_reset(ndev->phydev, 0);
	}
	ret = stmmac_resume(dev);

	if (ndev && ndev->phydev) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0))
		if (ndev->phydev->mac_managed_pm) {
			sunxi_info(chip->dev, "pm managed by mac, hw init phy\n");
			phy_init_hw(ndev->phydev);
		} else {
#endif
			/* State machine change phy state too early before mdio bus resume.
			 * WARN_ON would print in mdio_bus_phy_resume if state not equal to PHY_HALTED/PHY_READY/PHY_UP.
			 * Workaround is change the state back to PHY_UP and modify the state machine work so the judgment can be passed.
			 */
			rtnl_lock();
			mutex_lock(&ndev->phydev->lock);
			if (ndev->phydev->state == PHY_UP || ndev->phydev->state == PHY_NOLINK) {
				if (ndev->phydev->state == PHY_NOLINK)
					ndev->phydev->state = PHY_UP;
				phy_queue_state_machine(ndev->phydev, HZ);
			}
			mutex_unlock(&ndev->phydev->lock);
			rtnl_unlock();
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0))
		}
#endif
		/* suspend error workaround */
		dev_set_uevent_suppress(&ndev->phydev->mdio.dev, chip->uevent_suppress);
	}

	sunxi_info(chip->dev, "resume finish %d\n", ret);

	return ret;
}

static SIMPLE_DEV_PM_OPS(sunxi_dwmac_pm_ops, sunxi_dwmac_suspend, sunxi_dwmac_resume);

static const struct sunxi_dwmac_variant dwmac200_variant = {
	.interface = PHY_INTERFACE_MODE_RMII | PHY_INTERFACE_MODE_RGMII,
	.flags = SUNXI_DWMAC_SPH_DISABLE,
	.rx_delay_max = 31,
	.tx_delay_max = 7,
	.set_syscon = sunxi_dwmac200_set_syscon,
	.set_delaychain = sunxi_dwmac200_set_delaychain,
	.get_delaychain = sunxi_dwmac200_get_delaychain,
};

static const struct sunxi_dwmac_variant dwmac210_variant = {
	.interface = PHY_INTERFACE_MODE_RMII | PHY_INTERFACE_MODE_RGMII,
	.flags = SUNXI_DWMAC_SPH_DISABLE | SUNXI_DWMAC_MULTI_MSI,
	.rx_delay_max = 31,
	.tx_delay_max = 31,
	.set_syscon = sunxi_dwmac200_set_syscon,
	.set_delaychain = sunxi_dwmac210_set_delaychain,
	.get_delaychain = sunxi_dwmac210_get_delaychain,
};

static const struct sunxi_dwmac_variant dwmac220_variant = {
	.interface = PHY_INTERFACE_MODE_RMII | PHY_INTERFACE_MODE_RGMII,
	.flags = SUNXI_DWMAC_SPH_DISABLE | SUNXI_DWMAC_NSI_CLK_GATE | SUNXI_DWMAC_MULTI_MSI | SUNXI_DWMAC_MEM_ECC,
	.rx_delay_max = 31,
	.tx_delay_max = 31,
	.set_syscon = sunxi_dwmac200_set_syscon,
	.set_delaychain = sunxi_dwmac210_set_delaychain,
	.get_delaychain = sunxi_dwmac210_get_delaychain,
};

static const struct sunxi_dwmac_variant dwmac110_variant = {
	.interface = PHY_INTERFACE_MODE_RMII | PHY_INTERFACE_MODE_RGMII,
	.flags = SUNXI_DWMAC_SPH_DISABLE | SUNXI_DWMAC_NSI_CLK_GATE | SUNXI_DWMAC_HSI_CLK_GATE | SUNXI_DWMAC_MULTI_MSI,
	.rx_delay_max = 31,
	.tx_delay_max = 31,
	.set_syscon = sunxi_dwmac200_set_syscon,
	.set_delaychain = sunxi_dwmac210_set_delaychain,
	.get_delaychain = sunxi_dwmac210_get_delaychain,
	.get_version = sunxi_dwmac110_get_version,
};

static const struct of_device_id sunxi_dwmac_match[] = {
	{ .compatible = "allwinner,sunxi-gmac-200", .data = &dwmac200_variant },
	{ .compatible = "allwinner,sunxi-gmac-210", .data = &dwmac210_variant },
	{ .compatible = "allwinner,sunxi-gmac-220", .data = &dwmac220_variant },
	{ .compatible = "allwinner,sunxi-gmac-110", .data = &dwmac110_variant },
	{ }
};
MODULE_DEVICE_TABLE(of, sunxi_dwmac_match);

static struct platform_driver sunxi_dwmac_driver = {
	.probe = sunxi_dwmac_probe,
	.remove = sunxi_dwmac_remove,
	.shutdown = sunxi_dwmac_shutdown,
	.driver = {
		.name			= "dwmac-sunxi",
		.pm		= &sunxi_dwmac_pm_ops,
		.of_match_table = sunxi_dwmac_match,
	},
};
module_platform_driver(sunxi_dwmac_driver);

#ifndef MODULE
static int __init sunxi_dwmac_set_mac_addr(char *str)
{
	char *p = str;

	if (str && strlen(str))
		memcpy(mac_str, p, MAC_ADDR_LEN);

	return 0;
}
__setup("mac_addr=", sunxi_dwmac_set_mac_addr);
#endif /* MODULE */

MODULE_DESCRIPTION("Allwinner DWMAC driver");
MODULE_AUTHOR("wujiayi <wujiayi@allwinnertech.com>");
MODULE_AUTHOR("xuminghui <xuminghui@allwinnertech.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION(DWMAC_MODULE_VERSION);
