/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
* Allwinner DWMAC driver header.
*
* Copyright(c) 2022-2027 Allwinnertech Co., Ltd.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#ifndef _DWMAC_SUNXI_H_
#define _DWMAC_SUNXI_H_

#include <linux/version.h>
#include <linux/bitfield.h>

/* DWCMAC5 ECC Debug Register
 * These macro do not defined in mainline code dwmac5.h
 */
#define MTL_DBG_CTL				0x00000c08
#define EIEC				BIT(18)
#define EIAEE				BIT(17)
#define EIEE				BIT(16)
#define FIFOSEL				GENMASK(13, 12)
#define FIFOWREN			BIT(11)
#define FIFORDEN			BIT(10)
#define RSTSEL				BIT(9)
#define RSTALL				BIT(8)
#define DBGMOD				BIT(1)
#define FDBGEN				BIT(0)
#define MTL_DBG_STS				0x00000c0c
#define FIFOBUSY			BIT(0)
#define MTL_FIFO_DEBUG_DATA		0x00000c10
#define MTL_ECC_ERR_STS_RCTL	0x00000cd0
#define CUES				BIT(5)
#define CCES				BIT(4)
#define EMS					GENMASK(3, 1)
#define EESRE				BIT(0)
#define MTL_ECC_ERR_ADDR_STATUS	0x00000cd4
#define EUEAS				GENMASK(31, 16)
#define ECEAS				GENMASK(15, 0)
#define MTL_ECC_ERR_CNTR_STATUS	0x00000cd8
#define EUECS				GENMASK(19, 16)
#define ECECS				GENMASK(7, 0)
#define MTL_DPP_ECC_EIC			0x00000ce4
#define EIM					BIT(16)
#define BLEI				GENMASK(7, 0)

/* GMAC-200 Register */
#define SUNXI_DWMAC200_SYSCON_REG	(0x00)
	#define SUNXI_DWMAC200_SYSCON_BPS_EFUSE		GENMASK(31, 28)
	#define SUNXI_DWMAC200_SYSCON_XMII_SEL		BIT(27)
	#define SUNXI_DWMAC200_SYSCON_EPHY_MODE		GENMASK(26, 25)
	#define SUNXI_DWMAC200_SYSCON_PHY_ADDR		GENMASK(24, 20)
	#define SUNXI_DWMAC200_SYSCON_BIST_CLK_EN	BIT(19)
	#define SUNXI_DWMAC200_SYSCON_CLK_SEL		BIT(18)
	#define SUNXI_DWMAC200_SYSCON_LED_POL		BIT(17)
	#define SUNXI_DWMAC200_SYSCON_SHUTDOWN		BIT(16)
	#define SUNXI_DWMAC200_SYSCON_PHY_SEL		BIT(15)
	#define SUNXI_DWMAC200_SYSCON_ENDIAN_MODE	BIT(14)
	#define SUNXI_DWMAC200_SYSCON_RMII_EN		BIT(13)
	#define SUNXI_DWMAC200_SYSCON_ETXDC			GENMASK(12, 10)
	#define SUNXI_DWMAC200_SYSCON_ERXDC			GENMASK(9, 5)
	#define SUNXI_DWMAC200_SYSCON_ERXIE			BIT(4)
	#define SUNXI_DWMAC200_SYSCON_ETXIE			BIT(3)
	#define SUNXI_DWMAC200_SYSCON_EPIT			BIT(2)
	#define SUNXI_DWMAC200_SYSCON_ETCS			GENMASK(1, 0)

/* GMAC-210 Register */
#define SUNXI_DWMAC210_CFG_REG	(0x00)
	#define SUNXI_DWMAC210_CFG_ETXDC_H		GENMASK(17, 16)
	#define SUNXI_DWMAC210_CFG_PHY_SEL		BIT(15)
	#define SUNXI_DWMAC210_CFG_ENDIAN_MODE	BIT(14)
	#define SUNXI_DWMAC210_CFG_RMII_EN		BIT(13)
	#define SUNXI_DWMAC210_CFG_ETXDC_L		GENMASK(12, 10)
	#define SUNXI_DWMAC210_CFG_ERXDC		GENMASK(9, 5)
	#define SUNXI_DWMAC210_CFG_ERXIE		BIT(4)
	#define SUNXI_DWMAC210_CFG_ETXIE		BIT(3)
	#define SUNXI_DWMAC210_CFG_EPIT			BIT(2)
	#define SUNXI_DWMAC210_CFG_ETCS			GENMASK(1, 0)
#define SUNXI_DWMAC210_PTP_TIMESTAMP_L_REG	(0x40)
#define SUNXI_DWMAC210_PTP_TIMESTAMP_H_REG	(0x48)
#define SUNXI_DWMAC210_STAT_INT_REG		(0x4C)
	#define SUNXI_DWMAC210_STAT_PWR_DOWN_ACK	BIT(4)
	#define SUNXI_DWMAC210_STAT_SBD_TX_CLK_GATE	BIT(3)
	#define SUNXI_DWMAC210_STAT_LPI_INT			BIT(1)
	#define SUNXI_DWMAC210_STAT_PMT_INT			BIT(0)
#define SUNXI_DWMAC210_CLK_GATE_CFG_REG	(0x80)
	#define SUNXI_DWMAC210_CLK_GATE_CFG_RX		BIT(7)
	#define SUNXI_DWMAC210_CLK_GATE_CFG_PTP_REF	BIT(6)
	#define SUNXI_DWMAC210_CLK_GATE_CFG_CSR		BIT(5)
	#define SUNXI_DWMAC210_CLK_GATE_CFG_TX		BIT(4)
	#define SUNXI_DWMAC210_CLK_GATE_CFG_APP		BIT(3)

/* GMAC-110 Register */
#define SUNXI_DWMAC110_CFG_REG	SUNXI_DWMAC210_CFG_REG
	/* SUNXI_DWMAC110_CFG_REG is same with SUNXI_DWMAC210_CFG_REG */
#define SUNXI_DWMAC110_CLK_GATE_CFG_REG	(0x04)
	#define SUNXI_DWMAC110_CLK_GATE_CFG_RX	BIT(3)
	#define SUNXI_DWMAC110_CLK_GATE_CFG_TX	BIT(2)
	#define SUNXI_DWMAC110_CLK_GATE_CFG_APP	BIT(1)
	#define SUNXI_DWMAC110_CLK_GATE_CFG_CSR	BIT(0)
#define SUNXI_DWMAC110_VERSION_REG		(0xfc)
	#define SUNXI_DWMAC110_VERSION_IP_TAG	GENMASK(31, 16)
	#define SUNXI_DWMAC110_VERSION_IP_VRM	GENMASK(15, 0)

#define SUNXI_DWMAC_ETCS_MII		0x0
#define SUNXI_DWMAC_ETCS_EXT_GMII	0x1
#define SUNXI_DWMAC_ETCS_INT_GMII	0x2

/* MAC flags defined */
#define SUNXI_DWMAC_SPH_DISABLE		BIT(0)
#define SUNXI_DWMAC_NSI_CLK_GATE	BIT(1)
#define SUNXI_DWMAC_MULTI_MSI		BIT(2)
#define SUNXI_DWMAC_MEM_ECC			BIT(3)
#define SUNXI_DWMAC_HSI_CLK_GATE	BIT(4)

struct sunxi_dwmac;

enum sunxi_dwmac_delaychain_dir {
	SUNXI_DWMAC_DELAYCHAIN_TX,
	SUNXI_DWMAC_DELAYCHAIN_RX,
};

enum sunxi_dwmac_ecc_fifo_type {
	SUNXI_DWMAC_ECC_FIFO_TX,
	SUNXI_DWMAC_ECC_FIFO_RX,
};

struct sunxi_dwmac_variant {
	u32 flags;
	u32 interface;
	u32 rx_delay_max;
	u32 tx_delay_max;
	int (*set_syscon)(struct sunxi_dwmac *chip);
	int (*set_delaychain)(struct sunxi_dwmac *chip, enum sunxi_dwmac_delaychain_dir dir, u32 delay);
	u32 (*get_delaychain)(struct sunxi_dwmac *chip, enum sunxi_dwmac_delaychain_dir dir);
	int (*get_version)(struct sunxi_dwmac *chip, u16 *ip_tag, u16 *ip_vrm);
};

struct sunxi_dwmac_mii_reg {
	u32 addr;
	u16 reg;
	u16 value;
};

struct sunxi_dwmac {
	const struct sunxi_dwmac_variant *variant;
	struct sunxi_dwmac_mii_reg mii_reg;
	struct clk *phy_clk;
	struct clk *nsi_clk;
	struct clk *hsi_ahb;
	struct clk *hsi_axi;
	struct reset_control *ahb_rst;
	struct reset_control *hsi_rst;
	struct device *dev;
	void __iomem *syscfg_base;
	struct regulator *dwmac3v3_supply;
	struct regulator *phy3v3_supply;

	u32 tx_delay; /* adjust transmit clock delay */
	u32 rx_delay; /* adjust receive clock delay */

	bool rgmii_clk_ext;
	bool soc_phy_clk_en;
	int interface;
	unsigned int uevent_suppress; /* suspend error workaround: control kobject_uevent_env */

	struct stmmac_resources *res;
};

#include "dwmac-sunxi-sysfs.h"

#endif /* _DWMAC_SUNXI_H_ */
