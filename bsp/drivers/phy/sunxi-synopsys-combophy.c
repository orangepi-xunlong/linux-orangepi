// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner PIPE USB3.0 PCIE Combo Phy driver
 *
 * Copyright (C) 2024 Allwinner Electronics Co., Ltd.
 */

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/phy/phy.h>
#include <linux/reset.h>
#include <dt-bindings/phy/phy.h>
#include <sunxi-common.h>

struct sunxi_synopsys_combophy {
	const char *name;
	struct phy *phy;

	struct clk *comb0_ref_clk;
	struct clk *comb0_cfg_clk;
	struct clk *u3_only_clk;

	struct sunxi_synopsys_phy *sunxi_cphy;
};

struct sunxi_synopsys_phy {
	struct device *dev;
	void __iomem *top_subsys_reg;
	void __iomem *comb0_top_reg;
	void __iomem *comb0_core_reg;
	void __iomem *sys_ccu_reg;

	struct clk *bus_clk;
	struct clk *axi_clk;
	struct clk *ahb_bus_clk;
	struct clk *axi_bus_clk;
	struct reset_control *reset;

	struct sunxi_synopsys_combophy *combo_usb3;
	struct sunxi_synopsys_combophy *combo_pcie;

	__u32 mode;
	__u32 vernum;	    /* SUBSYS TOP Version number */
	bool usb_supported; /* USB can use combo0 or support only U3/U2 mode */
};

enum phy_type_e {
	COMB0_PHY_USB3 = 0,
	COMB0_PHY_PCIE,
};

/* The mode record which combo phy xlate owner */
#define COMBO_PHY_TYPE(mode, type)			(mode & ((1 << type)))

/* HSI Sub-System Registers */
/* HSI Global Control Register */
#define HSI_GBLC				0x0000
#define   PHY_USE_SEL				BIT(0)	/* 0: PCIe0; 1: USB2(USB3.1 GEN1) */

/* PCIE Bus Gating Reset Register */
#define PCIE_BGR				0x0004
#define   PCIE0_SLV_ACLK_EN			BIT(18)
#define   PCIE0_MST_ACLK_EN			BIT(17)
#define   PCIE0_HCLK_EN				BIT(16)
#define   PCIE0_PERSTN				BIT(1)

/* USB Bus Gating Reset Register */
#define USB_BGR					0x0008
#define   USB2_U3_UTMI_CLK_SEL			BIT(21)
#define   USB2_U2_PIPE_CLK_SEL			BIT(20)
#define   USB2_ACLK_EN				BIT(17)
#define   USB2_HCLK_EN				BIT(16)

/* USB2_U2 PHY Control Register */
#define USB2_U2_PHY_MODE			0x000C
#define   USB2_U2PHY_MODE			BIT(0)

/* HSI COMB0 PHY Version Register */
#define HSI_COMB0_PHY_VER			0x7FE0
#define   GEN_VER				GENMASK(31, 24)
#define   SUB_VER				GENMASK(23, 16)
#define   PRJ_VER				GENMASK(15, 8)
#define   IP_VENDOR_VER				GENMASK(7, 0)

/* HSI COMB0 PHY Control Register */
#define HSI_COMB0_PHY_CTL			0x0000
#define   PHY_REF_CLK_SEL			BIT(17)
#define   PHY_REF_CLK_ANA_GATE			BIT(16)
#define   REF_SSP_EN				BIT(8)
#define   PHY_PCIE_PIPE_PORT_SEL		BIT(6)
#define   HSI_COMB_PHY_MODE_SEL_MASK		GENMASK(5, 4)
#define   HSI_COMB_PHY_MODE_SEL(n)		(((n) & 0x3) << 4)
#define   RX0_LOS_LFPS_EN			BIT(3)
#define   PHY_PIPE_RST_SEL			BIT(2)
#define   PHY_PIPE_RST_APP			BIT(1)
#define   PHY_RSTN				BIT(0)

/* HSI COMB0 PHY PARAMETER0 Control Register */
#define HSI_COMB0_PHY_PARA0_CTL			0x0004
#define   PHY_TX_VBOOST_LVL_MASK		GENMASK(31, 29)
#define   PHY_TX_VBOOST_LVL(n)			(((n) & 0x7) << 29)
#define   PHY_PCS_RX_LOS_MASK_VAL		GENMASK(28, 19)
#define   PHY_PCS_RX_LOS_VAL(n)			(((n) & 0x3FF) << 19)
#define   PHY_PCS_DEEMPH_3P5DB_MASK		GENMASK(18, 13)
#define   PHY_PCS_DEEMPH_3P5DB(n)		(((n) & 0x3F) << 13)
#define   PHY_PCS_TX_DEEMPH_6DB_MASK		GENMASK(12, 7)
#define   PHY_PCS_TX_DEEMPH_6DB(n)		(((n) & 0x3F) << 7)
#define   PHY_PCS_TX_SWING_FULL_MASK		GENMASK(6, 0)
#define   PHY_PCS_TX_SWING_FULL(n)		(((n) & 0x7F) << 0)

/* HSI COMB0 PHY PARAMETER1 Control Register */
#define HSI_COMB0_PHY_PARA1_CTL			0x0008
#define   PHY_MPLL_MULTIPLIER_MASK		GENMASK(23, 17)
#define   PHY_MPLL_MULTIPLIER(n)		(((n) & 0x7F) << 17)
#define   PHY_LOS_BIAS_MASK			GENMASK(16, 14)
#define   PHY_LOS_BIAS(n)			(((n) & 0x7) << 14)
#define   PHY_LOS_LEVEL_MASK			GENMASK(13, 9)
#define   PHY_LOS_LEVEL(n)			(((n) & 0x1F) << 9)
#define   VREG_BYPASS				BIT(8)
#define   PHY_RX0_EQ_MASK			GENMASK(7, 5)
#define   PHY_RX0_EQ(n)				(((n) & 0x7) << 5)
#define   PHY_TX0_TERM_OFFSET_MASK		GENMASK(4, 0)
#define   PHY_TX0_TERM_OFFSET(n)		(((n) & 0x1F) << 0)

/* HSI COMB0 PHY REF Control Register */
#define HSI_COMB0_PHY_REF_CTL			0x0010
#define   SSC_REF_CLK_SEL_MASK			GENMASK(24, 16)
#define   SSC_REF_CLK_SEL(n)			(((n) & 0x1FF) << 16)
#define   SSC_RANGE_MASK			GENMASK(11, 9)
#define   SSC_RANGE(n)				(((n) & 0x7) << 9)
#define   SSC_EN				BIT(8)
#define   PHY_RTUNE_REQ_EN			BIT(2)
#define   PHY_REF_USB_PAD			BIT(1)
#define   PHY_REF_CLKDIV2			BIT(0)

/* HSI COMB0 PHY PCS Control Register */
#define HSI_COMB0_PHY_PCIE_PCS_CTL		0x0014
#define   PCS_TX_SWING_LOW_MASK			GENMASK(25, 19)
#define   PCS_TX_SWING_LOW(n)			(((n) & 0x7F) << 19)
#define   PCS_TX_DEEMPH_GEN2_6DB_MASK		GENMASK(18, 13)
#define   PCS_TX_DEEMPH_GEN2_6DB(n)		(((n) & 0x3F) << 13)
#define   PCS_TX_DEEMPH_GEN2_3P5DB_MASK		GENMASK(12, 7)
#define   PCS_TX_DEEMPH_GEN2_3P5DB(n)		(((n) & 0x3F) << 7)
#define   PCS_TX_DEEMPH_GEN1_MASK		GENMASK(6, 1)
#define   PCS_TX_DEEMPH_GEN1(n)			(((n) & 0x3F) << 1)
#define   PCS_COMMON_CLOCKS			BIT(0)

/* HSI COMB0 PHY PCS Control Register */
#define HSI_COMB0_PHY_PIPE_CTL			0x001C
#define   USB3P1_POWER_PRESENT			BIT(6)
#define   USB3P1_TX2RX_LOOPBK			BIT(5)
#define   USB3P1_PCLK_REQ			BIT(4)
#define   PCIE_MAC_PCLKREQ_APP_MASK		GENMASK(3, 2)
#define   PCIE_MAC_PCLKREQ_APP(n)		(((n) & 0x3) << 2)
#define   PCIE_MAC_PCLKREQ_SEL			BIT(1)
#define   PCIE_TX2RX_LOOPBK			BIT(0)

/* HSI COMB0 PHY CR Datain Register */
#define HSI_COMB0_PHY_CR_DATA_IN		0x0000
#define   HSI_COMB0_PHY_CR_DATA_IN_MASK		GENMASK(15, 0)

/* HSI COMB0 PHY CR WR Control Register */
#define HSI_COMB0_PHY_CR_WR_CTL			0x0004
#define   HSI_COMB0_PHY_CR_WRITE		BIT(3)
#define   HSI_COMB0_PHY_CR_READ			BIT(2)
#define   HSI_COMB0_PHY_CR_CAP_DATA		BIT(1)
#define   HSI_COMB0_PHY_CR_CAP_ADDR		BIT(0)

/* HSI COMB0 PHY CR RDATA Register */
#define HSI_COMB0_PHY_CR_RDATA			0x0008
#define   HSI_COMB0_PHY_CR_RDATA_MASK		GENMASK(15, 0)

/* Registers */
#define  COMBO_REG_HSI_GBLC(comb_base_addr)		((comb_base_addr) \
							+ HSI_GBLC)
#define  COMBO_REG_PCIE_BGR(comb_base_addr)		((comb_base_addr) \
							+ PCIE_BGR)
#define  COMBO_REG_USB_BGR(comb_base_addr)		((comb_base_addr) \
							+ USB_BGR)
#define  COMBO_REG_U2PHY_MODE(comb_base_addr)		((comb_base_addr) \
							+ USB2_U2_PHY_MODE)
#define  COMBO_REG_COMB0_PHY_VER(comb_base_addr)	((comb_base_addr) \
							+ HSI_COMB0_PHY_VER)

#define  COMB0_REG_PHY_CTL(top_base_addr)		((top_base_addr) \
							+ HSI_COMB0_PHY_CTL)
#define  COMB0_REG_PHY_PARA0_CTL(top_base_addr)		((top_base_addr) \
							+ HSI_COMB0_PHY_PARA0_CTL)
#define  COMB0_REG_PHY_PARA1_CTL(top_base_addr)		((top_base_addr) \
							+ HSI_COMB0_PHY_PARA1_CTL)
#define  COMB0_REG_PHY_REF_CTL(top_base_addr)		((top_base_addr) \
							+ HSI_COMB0_PHY_REF_CTL)
#define  COMB0_REG_PHY_PCIE_PCS_CTL(top_base_addr)	((top_base_addr) \
							+ HSI_COMB0_PHY_PCIE_PCS_CTL)
#define  COMB0_REG_PHY_PIPE_CTL(top_base_addr)		((top_base_addr) \
							+ HSI_COMB0_PHY_PIPE_CTL)

#define  COMB0_REG_PHY_CR_DATA_IN(comb_core_addr)	((comb_core_addr) \
							+ HSI_COMB0_PHY_CR_DATA_IN)
#define  COMB0_REG_PHY_CR_WR_CTL(comb_core_addr)	((comb_core_addr) \
							+ HSI_COMB0_PHY_CR_WR_CTL)
#define  COMB0_REG_PHY_CR_RDATA(comb_core_addr)		((comb_core_addr) \
							+ HSI_COMB0_PHY_CR_RDATA)

/* SYNOPSYS Sub-System Top Support. */
static void combo_pcie_clk_set(struct sunxi_synopsys_phy *sunxi_cphy, bool enable)
{
	__u32 val, tmp = 0;

	val = readl(COMBO_REG_PCIE_BGR(sunxi_cphy->top_subsys_reg));
	tmp = PCIE0_SLV_ACLK_EN | PCIE0_MST_ACLK_EN | PCIE0_HCLK_EN | PCIE0_PERSTN;
	if (enable)
		val |= tmp;
	else
		val &= ~tmp;
	writel(val, COMBO_REG_PCIE_BGR(sunxi_cphy->top_subsys_reg));
}

static void combo_usb3_clk_set(struct sunxi_synopsys_phy *sunxi_cphy, bool enable)
{
	__u32 val, tmp = 0;

	val = readl(COMBO_REG_USB_BGR(sunxi_cphy->top_subsys_reg));
	tmp = USB2_ACLK_EN | USB2_HCLK_EN | USB2_U3_UTMI_CLK_SEL;
	//tmp = USB2_ACLK_EN | USB2_HCLK_EN;
	if (enable)
		val |= tmp;
	else
		val &= ~tmp;
	writel(val, COMBO_REG_USB_BGR(sunxi_cphy->top_subsys_reg));
}

static void combo_phy_mode_set(struct sunxi_synopsys_phy *sunxi_cphy, bool enable)
{
	__u32 val = 0;

	val = readl(COMBO_REG_HSI_GBLC(sunxi_cphy->top_subsys_reg));
	if (enable)
		val |= PHY_USE_SEL;
	else
		val &= ~PHY_USE_SEL;
	writel(val, COMBO_REG_HSI_GBLC(sunxi_cphy->top_subsys_reg));

	val = readl(COMBO_REG_U2PHY_MODE(sunxi_cphy->top_subsys_reg));
	//if (enable)
	//	val |= USB2_U2PHY_MODE;
	//else
	//	val &= ~USB2_U2PHY_MODE;
	writel(val, COMBO_REG_U2PHY_MODE(sunxi_cphy->top_subsys_reg));
}

static __u32 combo_phy_ver_get(struct sunxi_synopsys_phy *sunxi_cphy)
{
	__u32 reg;

	reg = readl(COMBO_REG_COMB0_PHY_VER(sunxi_cphy->top_subsys_reg));

	return reg;
}

static int combo0_usb_check(void __iomem *phy_reg, __u32 bit_mask)
{
	__u32 data = 0;
	__u32 count = 0;

	while (count < 1000) {
		data = readl(phy_reg);
		if ((data & bit_mask) == 0)
			return 0;
		count++;
	}
	sunxi_err(NULL, "time out !!\n");
	return -EINVAL;
}

static int combo0_usb_cr_write(struct sunxi_synopsys_phy *sunxi_cphy, __u16 phy_reg, __u16 phy_data)
{
	__u32 data;

	/* wait write ready */
	combo0_usb_check(COMB0_REG_PHY_CR_WR_CTL(sunxi_cphy->comb0_core_reg), HSI_COMB0_PHY_CR_CAP_ADDR);
	combo0_usb_check(COMB0_REG_PHY_CR_WR_CTL(sunxi_cphy->comb0_core_reg), HSI_COMB0_PHY_CR_CAP_DATA);
	combo0_usb_check(COMB0_REG_PHY_CR_WR_CTL(sunxi_cphy->comb0_core_reg), HSI_COMB0_PHY_CR_WRITE);

	/* write phy addr */
	data = readl(COMB0_REG_PHY_CR_DATA_IN(sunxi_cphy->comb0_core_reg));
	data &= ~(HSI_COMB0_PHY_CR_DATA_IN_MASK);
	data |= (__u32)phy_reg;
	writel(data, COMB0_REG_PHY_CR_DATA_IN(sunxi_cphy->comb0_core_reg));

	/* phy_cr_cap_addr set */
	data = readl(COMB0_REG_PHY_CR_WR_CTL(sunxi_cphy->comb0_core_reg));
	data |= (HSI_COMB0_PHY_CR_CAP_ADDR);
	writel(data, COMB0_REG_PHY_CR_WR_CTL(sunxi_cphy->comb0_core_reg));
	combo0_usb_check(COMB0_REG_PHY_CR_WR_CTL(sunxi_cphy->comb0_core_reg), HSI_COMB0_PHY_CR_CAP_ADDR);

	/* write phy data */
	data = readl(COMB0_REG_PHY_CR_DATA_IN(sunxi_cphy->comb0_core_reg));
	data &= ~(HSI_COMB0_PHY_CR_DATA_IN_MASK);
	data |= (__u32)phy_data;
	writel(data, COMB0_REG_PHY_CR_DATA_IN(sunxi_cphy->comb0_core_reg));

	/* phy_cr_cap_data set */
	data = readl(COMB0_REG_PHY_CR_WR_CTL(sunxi_cphy->comb0_core_reg));
	data |= (HSI_COMB0_PHY_CR_CAP_DATA);
	writel(data, COMB0_REG_PHY_CR_WR_CTL(sunxi_cphy->comb0_core_reg));
	combo0_usb_check(COMB0_REG_PHY_CR_WR_CTL(sunxi_cphy->comb0_core_reg), HSI_COMB0_PHY_CR_CAP_DATA);

	/* phy_cr_write set */
	data = readl(COMB0_REG_PHY_CR_WR_CTL(sunxi_cphy->comb0_core_reg));
	data |= (HSI_COMB0_PHY_CR_WRITE);
	writel(data, COMB0_REG_PHY_CR_WR_CTL(sunxi_cphy->comb0_core_reg));
	combo0_usb_check(COMB0_REG_PHY_CR_WR_CTL(sunxi_cphy->comb0_core_reg), HSI_COMB0_PHY_CR_WRITE);

	return 0;
}

static __u16 combo0_usb_cr_read(struct sunxi_synopsys_phy *sunxi_cphy, __u16 phy_reg)
{
	__u32 data;
	__u16 phy_data;

	//read ready
	combo0_usb_check(COMB0_REG_PHY_CR_WR_CTL(sunxi_cphy->comb0_core_reg), HSI_COMB0_PHY_CR_CAP_ADDR);
	combo0_usb_check(COMB0_REG_PHY_CR_WR_CTL(sunxi_cphy->comb0_core_reg), HSI_COMB0_PHY_CR_CAP_DATA);
	combo0_usb_check(COMB0_REG_PHY_CR_WR_CTL(sunxi_cphy->comb0_core_reg), HSI_COMB0_PHY_CR_READ);

	/* write phy addr */
	data = readl(COMB0_REG_PHY_CR_DATA_IN(sunxi_cphy->comb0_core_reg));
	data &= ~HSI_COMB0_PHY_CR_DATA_IN_MASK;
	data |= (__u32)phy_reg;
	writel(data, COMB0_REG_PHY_CR_DATA_IN(sunxi_cphy->comb0_core_reg));

	/* phy_cr_cap_addr set */
	data = readl(COMB0_REG_PHY_CR_WR_CTL(sunxi_cphy->comb0_core_reg));
	data |= HSI_COMB0_PHY_CR_CAP_ADDR; //bit0
	writel(data, COMB0_REG_PHY_CR_WR_CTL(sunxi_cphy->comb0_core_reg));
	combo0_usb_check(COMB0_REG_PHY_CR_WR_CTL(sunxi_cphy->comb0_core_reg), HSI_COMB0_PHY_CR_CAP_ADDR);

	/* phy_cr_read set */
	data = readl(COMB0_REG_PHY_CR_WR_CTL(sunxi_cphy->comb0_core_reg));
	data |= HSI_COMB0_PHY_CR_READ; //bit2
	writel(data, COMB0_REG_PHY_CR_WR_CTL(sunxi_cphy->comb0_core_reg));
	combo0_usb_check(COMB0_REG_PHY_CR_WR_CTL(sunxi_cphy->comb0_core_reg), HSI_COMB0_PHY_CR_READ);

	/* read data, phy inner data is 16bit*/
	phy_data = readl(COMB0_REG_PHY_CR_RDATA(sunxi_cphy->comb0_core_reg));

	return phy_data;
}

static void combo0_usb_set_calibrate(struct sunxi_synopsys_phy *sunxi_cphy)
{
	__u16 phy_data;

	phy_data = combo0_usb_cr_read(sunxi_cphy, 0x30);
	phy_data &= ~(GENMASK(7, 4));
	phy_data |= BIT(7);
	combo0_usb_cr_write(sunxi_cphy, 0x30, phy_data);

	dev_info(sunxi_cphy->dev, " PHY calibrate set %x:%x\n",
			0x30, combo0_usb_cr_read(sunxi_cphy, 0x30));
}

static void combo0_usb_param_config(struct sunxi_synopsys_phy *sunxi_cphy, bool enable)
{
	__u32 val = 0;

	val = readl(COMB0_REG_PHY_PIPE_CTL(sunxi_cphy->comb0_top_reg));
	if (enable)
		val |= USB3P1_POWER_PRESENT;
	else
		val &= ~USB3P1_POWER_PRESENT;
	writel(val, COMB0_REG_PHY_PIPE_CTL(sunxi_cphy->comb0_top_reg));

	val = readl(COMB0_REG_PHY_REF_CTL(sunxi_cphy->comb0_top_reg));
	if (enable)
		val |= SSC_EN;
	else
		val &= ~SSC_EN;
	writel(val, COMB0_REG_PHY_REF_CTL(sunxi_cphy->comb0_top_reg));

	val = readl(COMB0_REG_PHY_PARA1_CTL(sunxi_cphy->comb0_top_reg));
	if (enable)
		val |= PHY_MPLL_MULTIPLIER(0x19);
	else
		val &= ~PHY_MPLL_MULTIPLIER_MASK;
	writel(val, COMB0_REG_PHY_PARA1_CTL(sunxi_cphy->comb0_top_reg));

	val = readl(COMB0_REG_PHY_PARA0_CTL(sunxi_cphy->comb0_top_reg));
	if (enable)
		val |= (PHY_PCS_TX_DEEMPH_6DB(0x21) | PHY_PCS_DEEMPH_3P5DB(0x16));
	else
		val &= ~(PHY_PCS_TX_DEEMPH_6DB_MASK | PHY_PCS_DEEMPH_3P5DB_MASK);
	writel(val, COMB0_REG_PHY_PARA0_CTL(sunxi_cphy->comb0_top_reg));

	udelay(1);

	val = readl(COMB0_REG_PHY_PARA0_CTL(sunxi_cphy->comb0_top_reg));
	if (enable)
		val |= (PHY_PCS_TX_SWING_FULL(0x78) | PHY_TX_VBOOST_LVL(0x4));
	else
		val &= ~(PHY_PCS_TX_SWING_FULL_MASK | PHY_TX_VBOOST_LVL_MASK);
	writel(val, COMB0_REG_PHY_PARA0_CTL(sunxi_cphy->comb0_top_reg));

	val = readl(COMB0_REG_PHY_PARA1_CTL(sunxi_cphy->comb0_top_reg));
	val &= ~PHY_RX0_EQ_MASK;
	if (enable)
		val |= PHY_RX0_EQ(0x2);
	else
		val |= PHY_RX0_EQ(0x3);
	writel(val, COMB0_REG_PHY_PARA1_CTL(sunxi_cphy->comb0_top_reg));

	udelay(1);

	val = readl(COMB0_REG_PHY_CTL(sunxi_cphy->comb0_top_reg));
	if (enable)
		val |= (PHY_REF_CLK_SEL | HSI_COMB_PHY_MODE_SEL(0x3) | RX0_LOS_LFPS_EN);
	else
		val &= ~(PHY_REF_CLK_SEL | HSI_COMB_PHY_MODE_SEL(0x3) | RX0_LOS_LFPS_EN);
	writel(val, COMB0_REG_PHY_CTL(sunxi_cphy->comb0_top_reg));

	udelay(1);

	val = readl(COMB0_REG_PHY_CTL(sunxi_cphy->comb0_top_reg));
	if (enable)
		val |= REF_SSP_EN;
	else
		val &= ~REF_SSP_EN;
	writel(val, COMB0_REG_PHY_CTL(sunxi_cphy->comb0_top_reg));

	udelay(10);

	val = readl(COMB0_REG_PHY_CTL(sunxi_cphy->comb0_top_reg));
	if (enable)
		val &= ~PHY_RSTN;
	else
		val |= PHY_RSTN;
	writel(val, COMB0_REG_PHY_CTL(sunxi_cphy->comb0_top_reg));

	udelay(1);

	val = readl(COMB0_REG_PHY_CTL(sunxi_cphy->comb0_top_reg));
	if (enable)
		val |= PHY_PIPE_RST_APP;
	else
		val &= ~PHY_PIPE_RST_APP;
	writel(val, COMB0_REG_PHY_CTL(sunxi_cphy->comb0_top_reg));
}

static int combo0_usb_phy_init(struct phy *phy)
{
	struct sunxi_synopsys_combophy *combo0 = phy_get_drvdata(phy);
	struct sunxi_synopsys_phy *sunxi_cphy = combo0->sunxi_cphy;
	int ret;

	sunxi_cphy->mode |= (1 << PHY_TYPE_USB3);

	if (combo0->comb0_cfg_clk) {
		ret = clk_set_rate(combo0->comb0_cfg_clk, 24000000);
		if (ret) {
			dev_err(sunxi_cphy->dev, "set comb0_cfg_clk rate 24MHz err, return %d\n", ret);
			return ret;
		}

		ret = clk_prepare_enable(combo0->comb0_cfg_clk);
		if (ret) {
			dev_err(sunxi_cphy->dev, "enable comb0_cfg_clk err, return %d\n", ret);
			return ret;
		}
	}

	if (combo0->comb0_ref_clk) {
		ret = clk_set_rate(combo0->comb0_ref_clk, 100000000);
		if (ret) {
			dev_err(sunxi_cphy->dev, "set comb0_ref_clk rate 100MHz err, return %d\n", ret);
			return ret;
		}

		ret = clk_prepare_enable(combo0->comb0_ref_clk);
		if (ret) {
			dev_err(sunxi_cphy->dev, "enable comb0_ref_clk err, return %d\n", ret);
			return ret;
		}
	}

	if (combo0->u3_only_clk) {
		ret = clk_prepare_enable(combo0->u3_only_clk);
		if (ret) {
			dev_err(sunxi_cphy->dev, "enable u3_only_clk err, return %d\n", ret);
			return ret;
		}
	}

	combo0_usb_param_config(sunxi_cphy, true);
	combo0_usb_set_calibrate(sunxi_cphy);

	return 0;
}

static int combo0_usb_phy_exit(struct phy *phy)
{
	struct sunxi_synopsys_combophy *combo0 = phy_get_drvdata(phy);
	struct sunxi_synopsys_phy *sunxi_cphy = combo0->sunxi_cphy;

	combo0_usb_param_config(sunxi_cphy, false);

	if (combo0->u3_only_clk)
		clk_disable_unprepare(combo0->u3_only_clk);

	if (combo0->comb0_ref_clk)
		clk_disable_unprepare(combo0->comb0_ref_clk);

	if (combo0->comb0_cfg_clk)
		clk_disable_unprepare(combo0->comb0_cfg_clk);

	return 0;
}

static const struct phy_ops combo0_usb3_phy_ops = {
	.init = combo0_usb_phy_init,
	.exit = combo0_usb_phy_exit,
	.owner = THIS_MODULE,
};

static void combo0_pcie_param_config(struct sunxi_synopsys_phy *sunxi_cphy, bool enable)
{
	u32 val;

	combo_phy_mode_set(sunxi_cphy, false);
	combo_pcie_clk_set(sunxi_cphy, true);

	val = readl(COMB0_REG_PHY_CTL(sunxi_cphy->comb0_top_reg));
	val |= PHY_PIPE_RST_APP;
	val |= PHY_PCIE_PIPE_PORT_SEL;
	val &= ~HSI_COMB_PHY_MODE_SEL_MASK;
	val |= HSI_COMB_PHY_MODE_SEL(0x0);
	val &= ~PHY_PIPE_RST_SEL;
	if (enable)
		val &= ~PHY_RSTN;
	else
		val |= PHY_RSTN;
	writel_verify(val, COMB0_REG_PHY_CTL(sunxi_cphy->comb0_top_reg));

	val = readl(COMB0_REG_PHY_REF_CTL(sunxi_cphy->comb0_top_reg));
	val |= PHY_REF_USB_PAD;
	val &= ~SSC_REF_CLK_SEL_MASK;
	val &= ~SSC_RANGE_MASK;
	val &= ~PHY_RTUNE_REQ_EN;
	if (enable)
		val &= ~SSC_EN;
	else
		val |= SSC_EN;
	writel_verify(val, COMB0_REG_PHY_REF_CTL(sunxi_cphy->comb0_top_reg));

	val = readl(COMB0_REG_PHY_PARA1_CTL(sunxi_cphy->comb0_top_reg));
	val &= ~PHY_MPLL_MULTIPLIER_MASK;
	val |= PHY_MPLL_MULTIPLIER(0x19);
	val &= ~PHY_LOS_BIAS_MASK;
	val |= PHY_LOS_BIAS(0x2);
	val &= ~PHY_LOS_LEVEL_MASK;
	val |= PHY_LOS_LEVEL(0x9);
	val &= ~PHY_RX0_EQ_MASK;
	val |= PHY_RX0_EQ(0x3);
	writel_verify(val, COMB0_REG_PHY_PARA1_CTL(sunxi_cphy->comb0_top_reg));

	val = readl(COMB0_REG_PHY_PARA0_CTL(sunxi_cphy->comb0_top_reg));
	val &= ~PHY_TX_VBOOST_LVL_MASK;
	val |= PHY_TX_VBOOST_LVL(0x4);
	val &= ~PHY_PCS_TX_SWING_FULL_MASK;
	val |= PHY_PCS_TX_SWING_FULL(0x78);
	writel_verify(val, COMB0_REG_PHY_PARA0_CTL(sunxi_cphy->comb0_top_reg));

	val = readl(COMB0_REG_PHY_PCIE_PCS_CTL(sunxi_cphy->comb0_top_reg));
	val &= ~PCS_TX_SWING_LOW_MASK;
	val |= PCS_TX_SWING_LOW(0x78);
	val &= ~PCS_TX_DEEMPH_GEN1_MASK;
	val |= PCS_TX_DEEMPH_GEN1(0x18);
	writel_verify(val, COMB0_REG_PHY_PCIE_PCS_CTL(sunxi_cphy->comb0_top_reg));

	udelay(250);
}

static int combo0_pcie_phy_init(struct phy *phy)
{
	struct sunxi_synopsys_combophy *combo0 = phy_get_drvdata(phy);
	struct sunxi_synopsys_phy *sunxi_cphy = combo0->sunxi_cphy;
	int ret;

	if (combo0->comb0_cfg_clk) {
		ret = clk_set_rate(combo0->comb0_cfg_clk, 200000000); /* 200M */
		if (ret) {
			dev_err(sunxi_cphy->dev, "pcie:set comb0_cfg_clk rate 24MHz err, return %d\n", ret);
			return ret;
		}

		ret = clk_prepare_enable(combo0->comb0_cfg_clk);
		if (ret) {
			dev_err(sunxi_cphy->dev, "pcie:enable comb0_cfg_clk err, return %d\n", ret);
			return ret;
		}
	}

	if (combo0->comb0_ref_clk) {
		ret = clk_set_rate(combo0->comb0_ref_clk, 100000000); /* 100M */
		if (ret) {
			dev_err(sunxi_cphy->dev, "pcie:set comb0_ref_clk rate 100MHz err, return %d\n", ret);
			return ret;
		}

		ret = clk_prepare_enable(combo0->comb0_ref_clk);
		if (ret) {
			dev_err(sunxi_cphy->dev, "pcie:enable comb0_ref_clk err, return %d\n", ret);
			return ret;
		}
	}

	combo0_pcie_param_config(sunxi_cphy, true);
	sunxi_info(sunxi_cphy->dev, "pcie phy init done\n");

	return 0;
}

static int combo0_pcie_phy_exit(struct phy *phy)
{
	struct sunxi_synopsys_combophy *combo0 = phy_get_drvdata(phy);
	struct sunxi_synopsys_phy *sunxi_cphy = combo0->sunxi_cphy;

	combo0_pcie_param_config(sunxi_cphy, false);

	if (combo0->comb0_cfg_clk)
		clk_disable_unprepare(combo0->comb0_cfg_clk);

	if (combo0->comb0_ref_clk)
		clk_disable_unprepare(combo0->comb0_ref_clk);

	sunxi_info(sunxi_cphy->dev, "pcie phy exit done\n");

	return 0;
}

static const struct phy_ops combo0_pcie_phy_ops = {
	.init = combo0_pcie_phy_init,
	.exit = combo0_pcie_phy_exit,
	.owner = THIS_MODULE,
};

static struct phy *sunxi_synopsys_phy_xlate(struct device *dev,
					    struct of_phandle_args *args)
{
	struct phy *phy = NULL;

	phy = of_phy_simple_xlate(dev, args);
	if (IS_ERR_OR_NULL(phy)) {
		dev_err(dev, "%s failed to xlate phy\n", __func__);
		return phy;
	}

	/* TODO: if need */

	return phy;
}

static int sunxi_synopsys_phy_create(struct device *dev, struct device_node *np,
				   struct sunxi_synopsys_combophy *combophy, enum phy_type_e type)
{
	struct sunxi_synopsys_phy *sunxi_cphy = dev_get_drvdata(dev);
	struct phy *phy = NULL;
	const struct phy_ops *ops = NULL;
	int ret;

	switch (type) {
	case COMB0_PHY_USB3:
		combophy->name = kstrdup_const("comb0-usb3", GFP_KERNEL);
		ops = &combo0_usb3_phy_ops;
		break;
	case COMB0_PHY_PCIE:
		combophy->name = kstrdup_const("comb0-pcie", GFP_KERNEL);
		ops = &combo0_pcie_phy_ops;
		break;
	default:
		pr_err("not support phy type (%d)\n", type);
		return -EINVAL;
	}

	combophy->comb0_ref_clk = devm_get_clk_from_child(dev, np, "comb0_ref_clk");
	if (IS_ERR(combophy->comb0_ref_clk)) {
		combophy->comb0_ref_clk = NULL;
		pr_debug("Maybe there is no reference clk for phy (%s)\n", combophy->name);
	}

	combophy->comb0_cfg_clk = devm_get_clk_from_child(dev, np, "comb0_cfg_clk");
	if (IS_ERR(combophy->comb0_cfg_clk)) {
		combophy->comb0_cfg_clk = NULL;
		pr_debug("Maybe there is no configure clk for phy (%s)\n", combophy->name);
	}

	combophy->u3_only_clk = devm_get_clk_from_child(dev, np, "u3_only_clk");
	if (IS_ERR(combophy->u3_only_clk)) {
		combophy->u3_only_clk = NULL;
		pr_debug("Maybe there is no u3 only utmi clk for phy (%s)\n", combophy->name);
	}

	phy = devm_phy_create(dev, np, ops);
	if (IS_ERR(phy)) {
		ret = PTR_ERR(phy);
		dev_err(dev, "failed to create phy for %s, ret:%d\n", combophy->name, ret);
		return ret;
	}
	phy_set_drvdata(phy, combophy);

	combophy->phy = phy;
	combophy->sunxi_cphy = sunxi_cphy;

	return 0;
}

static int sunxi_synopsys_phy_subsys_init(struct sunxi_synopsys_phy *sunxi_cphy)
{
	int ret;

	if (sunxi_cphy->bus_clk) {
		ret = clk_prepare_enable(sunxi_cphy->bus_clk);
		if (ret) {
			dev_err(sunxi_cphy->dev, "enable bus_clk err, return %d\n", ret);
			return ret;
		}
	}

	if (sunxi_cphy->reset) {
		ret = reset_control_deassert(sunxi_cphy->reset);
		if (ret) {
			dev_err(sunxi_cphy->dev, "reset serdes err, return %d\n", ret);
			return ret;
		}
	}

	if (sunxi_cphy->axi_clk) {
		ret = clk_prepare_enable(sunxi_cphy->axi_clk);
		if (ret) {
			dev_err(sunxi_cphy->dev, "enable axi_clk err, return %d\n", ret);
			return ret;
		}
	}

	if (sunxi_cphy->ahb_bus_clk) {
		ret = clk_prepare_enable(sunxi_cphy->ahb_bus_clk);
		if (ret) {
			dev_err(sunxi_cphy->dev, "enable ahb_bus_clk err, return %d\n", ret);
			return ret;
		}
	}

	if (sunxi_cphy->axi_bus_clk) {
		ret = clk_prepare_enable(sunxi_cphy->axi_bus_clk);
		if (ret) {
			dev_err(sunxi_cphy->dev, "enable axi_bus_clk err, return %d\n", ret);
			return ret;
		}
	}

	if (sunxi_cphy->usb_supported) {
		combo_phy_mode_set(sunxi_cphy, true);
		combo_usb3_clk_set(sunxi_cphy, true);
	}

	sunxi_cphy->vernum = combo_phy_ver_get(sunxi_cphy);

	return 0;
}

static void sunxi_synopsys_phy_subsys_exit(struct sunxi_synopsys_phy *sunxi_cphy)
{
	if (sunxi_cphy->usb_supported) {
		combo_usb3_clk_set(sunxi_cphy, false);
		combo_phy_mode_set(sunxi_cphy, false);
	}

	if (sunxi_cphy->axi_bus_clk)
		clk_disable_unprepare(sunxi_cphy->axi_bus_clk);

	if (sunxi_cphy->ahb_bus_clk)
		clk_disable_unprepare(sunxi_cphy->ahb_bus_clk);

	if (sunxi_cphy->axi_clk)
		clk_disable_unprepare(sunxi_cphy->axi_clk);

	if (sunxi_cphy->reset)
		reset_control_assert(sunxi_cphy->reset);

	if (sunxi_cphy->bus_clk)
		clk_disable_unprepare(sunxi_cphy->bus_clk);
}

static int sunxi_synopsys_phy_parse_dt(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_synopsys_phy *sunxi_cphy = dev_get_drvdata(dev);
	struct device_node *child;
	int ret;

	/* usb supported */
	sunxi_cphy->usb_supported = !device_property_read_bool(dev, "usb-disable");
	/* parse top register, which determide general configuration such as mode */
	sunxi_cphy->top_subsys_reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(sunxi_cphy->top_subsys_reg))
		return PTR_ERR(sunxi_cphy->top_subsys_reg);

	sunxi_cphy->comb0_top_reg = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(sunxi_cphy->comb0_top_reg))
		return PTR_ERR(sunxi_cphy->comb0_top_reg);

	sunxi_cphy->comb0_core_reg = devm_platform_ioremap_resource(pdev, 2);
	if (IS_ERR(sunxi_cphy->comb0_core_reg))
		return PTR_ERR(sunxi_cphy->comb0_core_reg);

	sunxi_cphy->sys_ccu_reg = devm_ioremap(dev, 0x03008000, 0x4000); /* platform_get_resource(pdev, IORESOURCE_MEM, 3); */
	if (IS_ERR(sunxi_cphy->sys_ccu_reg))
		return PTR_ERR(sunxi_cphy->sys_ccu_reg);

	sunxi_cphy->bus_clk = devm_clk_get(dev, "bus_clk");
	if (IS_ERR(sunxi_cphy->bus_clk)) {
		return dev_err_probe(dev, PTR_ERR(sunxi_cphy->bus_clk),
				     "failed to get bus clock for synopsys phy\n");
	}

	sunxi_cphy->axi_clk = devm_clk_get(dev, "axi_clk");
	if (IS_ERR(sunxi_cphy->axi_clk)) {
		return dev_err_probe(dev, PTR_ERR(sunxi_cphy->axi_clk),
				     "failed to get axi clock for synopsys phy\n");
	}

	sunxi_cphy->ahb_bus_clk = devm_clk_get(dev, "ahb_bus_clk");
	if (IS_ERR(sunxi_cphy->ahb_bus_clk)) {
		return dev_err_probe(dev, PTR_ERR(sunxi_cphy->ahb_bus_clk),
				     "failed to get ahb bus clock for synopsys phy\n");
	}

	sunxi_cphy->axi_bus_clk = devm_clk_get(dev, "axi_bus_clk");
	if (IS_ERR(sunxi_cphy->axi_bus_clk)) {
		return dev_err_probe(dev, PTR_ERR(sunxi_cphy->axi_bus_clk),
				     "failed to get axi bus clock for synopsys phy\n");
	}

	sunxi_cphy->reset = devm_reset_control_get_shared(dev, "reset");
	if (IS_ERR(sunxi_cphy->reset)) {
		return dev_err_probe(dev, PTR_ERR(sunxi_cphy->reset),
				     "failed to get reset for synopsys phy\n");
	}

	for_each_available_child_of_node(dev->of_node, child) {
		if (of_node_name_eq(child, "combo-usb3")) {
			sunxi_cphy->combo_usb3 = devm_kzalloc(dev, sizeof(*sunxi_cphy->combo_usb3), GFP_KERNEL);
			if (!sunxi_cphy->combo_usb3)
				return -ENOMEM;

			/* create usb3 combophy */
			ret = sunxi_synopsys_phy_create(dev, child, sunxi_cphy->combo_usb3, COMB0_PHY_USB3);
			if (ret) {
				dev_err(dev, "failed to create synopsys combophy0, ret:%d\n", ret);
				goto err_node_put;
			}
		} else if (of_node_name_eq(child, "combo-pcie")) {
			sunxi_cphy->combo_pcie = devm_kzalloc(dev, sizeof(*sunxi_cphy->combo_pcie), GFP_KERNEL);
			if (!sunxi_cphy->combo_pcie)
				return -ENOMEM;

			/* create pcie combophy */
			ret = sunxi_synopsys_phy_create(dev, child, sunxi_cphy->combo_pcie, COMB0_PHY_PCIE);
			if (ret) {
				dev_err(dev, "failed to create synopsys combophy1, ret:%d\n", ret);
				goto err_node_put;
			}
		}
	}

	return 0;

err_node_put:
	of_node_put(child);

	return ret;
}

static int sunxi_synopsys_phy_probe(struct platform_device *pdev)
{
	struct sunxi_synopsys_phy *sunxi_cphy;
	struct device *dev = &pdev->dev;
	struct phy_provider *phy_provider;
	int ret;

	sunxi_cphy = devm_kzalloc(dev, sizeof(*sunxi_cphy), GFP_KERNEL);
	if (!sunxi_cphy)
		return -ENOMEM;

	sunxi_cphy->dev = dev;
	sunxi_cphy->mode = PHY_NONE;
	dev_set_drvdata(dev, sunxi_cphy);

	ret = sunxi_synopsys_phy_parse_dt(pdev);
	if (ret)
		return -EINVAL;

	phy_provider = devm_of_phy_provider_register(dev, sunxi_synopsys_phy_xlate);
	if (IS_ERR(phy_provider))
		return PTR_ERR_OR_ZERO(phy_provider);

	ret = sunxi_synopsys_phy_subsys_init(sunxi_cphy);
	if (ret)
		return -EINVAL;
	dev_info(dev, "Synopsys Combophy Version v%lu.%lu.%lu.%lu\n",
		 FIELD_GET(GEN_VER, sunxi_cphy->vernum), FIELD_GET(SUB_VER, sunxi_cphy->vernum),
		 FIELD_GET(PRJ_VER, sunxi_cphy->vernum), FIELD_GET(IP_VENDOR_VER, sunxi_cphy->vernum));

	return 0;
}

static int sunxi_synopsys_phy_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_synopsys_phy *sunxi_cphy = dev_get_drvdata(dev);

	sunxi_synopsys_phy_subsys_exit(sunxi_cphy);

	return 0;
}

static int __maybe_unused sunxi_synopsys_phy_suspend(struct device *dev)
{
	struct sunxi_synopsys_phy *sunxi_cphy = dev_get_drvdata(dev);

	sunxi_synopsys_phy_subsys_exit(sunxi_cphy);

	return 0;
}

static int __maybe_unused sunxi_synopsys_phy_resume(struct device *dev)
{
	struct sunxi_synopsys_phy *sunxi_cphy = dev_get_drvdata(dev);
	int ret;

	ret = sunxi_synopsys_phy_subsys_init(sunxi_cphy);
	if (ret) {
		dev_err(dev, "failed to resume sub system\n");
		return ret;
	}

	return 0;
}

static struct dev_pm_ops sunxi_synopsys_phy_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sunxi_synopsys_phy_suspend, sunxi_synopsys_phy_resume)
};

static const struct of_device_id sunxi_synopsys_phy_of_match_table[] = {
	{ .compatible = "allwinner,synopsys-combophy", },
	{ /* Sentinel */ }
};

static struct platform_driver sunxi_synopsys_phy_driver = {
	.probe		= sunxi_synopsys_phy_probe,
	.remove		= sunxi_synopsys_phy_remove,
	.driver = {
		.name	= "synopsys-combophy",
		.pm	= &sunxi_synopsys_phy_pm_ops,
		.of_match_table = sunxi_synopsys_phy_of_match_table,
	},
};

module_platform_driver(sunxi_synopsys_phy_driver);

MODULE_AUTHOR("kanghoupeng@allwinnertech.com");
MODULE_DESCRIPTION("Allwinner SYNOPSYS COMBOPHY driver");
MODULE_VERSION("0.1.1");
MODULE_LICENSE("GPL v2");
