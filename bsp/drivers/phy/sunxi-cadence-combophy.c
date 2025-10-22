// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner USB3.0/PCIE/DisplayPort Combo Phy driver
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
#include <linux/phy/phy-dp.h>
#include <linux/reset.h>
#include <dt-bindings/phy/phy.h>
#include <linux/usb/typec_mux.h>
#include <linux/usb/typec_dp.h>
#include <linux/extcon.h>
#include <linux/extcon-provider.h>
#include <linux/regulator/consumer.h>

static struct phy *sunxi_cadence_phy_xlate(struct device *dev,
					  struct of_phandle_args *args);

#define phy_set_mask(width, shift)   ((width?((-1U) >> (32-width)):0)  << (shift))
#define phy_clear_mask(width, shift)   (~(phy_set_mask(width, shift)))

#define phy_readl(addr) readl(addr)
#define phy_readw(addr) readw(addr)
#define phy_writel(addr, val) writel(val, addr)
#define phy_writew(addr, val) writew(val, addr)

/* The mode record which combo phy xlate owner */
#define COMBO_PHY_TYPE(mode, type)	(mode & ((1 << type)))

#define MAX_LANE_CNT 4
#define MAX_LANE_NO  3
#define MAX_STATE_D_F_LANE_CNT 2
#define MAX_SWING 4
#define MAX_PRE 4

/**
 * NOTE:
 * External PHY mode:
 * bit[1:0] - orientation,
 * bit[5:2] - mode
 */
#define COMBO0_TYPEC_ORIENTATION(n)	(n & GENMASK(1, 0))
#define COMBO0_TYPEC_MODE(n)		((n & GENMASK(5, 2)) >> 0x2)
#define COMBO0_HPD_STATE(n)		((n & BIT(6)) >> 0x6)

static const char * const typec_orientation_name[] = {
	[TYPEC_ORIENTATION_NONE]		= "UNKNOW",
	[TYPEC_ORIENTATION_NORMAL]		= "NORMAL",
	[TYPEC_ORIENTATION_REVERSE]		= "REVERSE",
};

static const char * const typec_state_name[] = {
	[TYPEC_STATE_SAFE]			= "STATE_SAFE",
	[TYPEC_STATE_USB]			= "STATE_USB",
	[TYPEC_DP_STATE_C]			= "STATE_DP_C",
	[TYPEC_DP_STATE_D]			= "STATE_DP_D",
	[TYPEC_DP_STATE_E]			= "STATE_DP_E",
	[TYPEC_DP_STATE_F]			= "STATE_DP_F",
};

static const unsigned int sunxi_cadence_cable[] = {
	EXTCON_DISP_DP,
	EXTCON_USB,
	EXTCON_NONE,
};

static void phy_set_lbits(void *addr, u32 shift, u32 width, u32 val)
{
	u32 reg_val;

	reg_val = phy_readl(addr);
	reg_val &= phy_clear_mask(width, shift);
	reg_val |= (val << shift);
	phy_writel(addr, reg_val);
}

struct sunxi_cadence_combophy {
	const char *name;
	void __iomem *top_reg;
	void __iomem *phy_reg;
	struct clk *clk;
	struct clk *bus_clk;
	struct reset_control *reset;
	enum typec_orientation orientation;
	struct sunxi_cadence_phy *sunxi_cphy;
	struct mutex phy_lock;

	unsigned long state;
	bool usb_dp_state;
	__u8 hpd_state;
	__u8 ssc_en;
	__u32 mode;
	__u8 configuration;
	__u32 vernum; /* COMB0/1 PHY Version number */
	__u32 lane_remap[MAX_LANE_CNT];
	__u32 typec_remap[MAX_LANE_CNT];
	__u32 lane_invert[MAX_LANE_CNT];
	unsigned int link_rate;
};

struct sunxi_cadence_phy {
	void __iomem *top_subsys_reg;
	void __iomem *top_combo_reg;
	struct device *dev;
	struct clk *serdes_clk;
	struct clk *dcxo_serdes0_clk;
	struct clk *dcxo_serdes1_clk;
	struct reset_control *serdes_reset;
	struct regulator *avdd_h_regulator;
	struct regulator *avdd_c_regulator;
	struct regulator *avdd_d_regulator;
	struct regulator *avdd_aux_regulator;
	struct extcon_dev *extcon;

	struct sunxi_cadence_combophy *combo0;
	struct sunxi_cadence_combophy *combo1;
	struct sunxi_cadence_combophy *aux_hpd;

	__u32 vernum; /* SUBSYS TOP Version number */
	bool usb_supported; /* USB can use combo0/1 or support only U3/U2 mode */
	bool udp_supported; /* Both USB and DP can use combo0 meanwhile */
	bool phy_switcher_quirk;

	struct regulator *serdes1v8_supply;
};

enum phy_type_e {
	COMBO_PHY0 = 0,
	COMBO_PHY1,
	AUX_HPD,
};

enum phy_config_e {
	PHY_CFG_SSC = 0x1,
};

static int combo0_configure_usb_dp(struct sunxi_cadence_combophy *combo0);

/* Sub-System PCIE Bus Gating Reset Register */
#define SUBSYS_PCIE_BGR				0x0004
#define SUBSYS_PCIE_GATING			(BIT(16) | BIT(17) | BIT(18))
#define SUBSYS_DISABLE_COMBO1_AUTOGATING	BIT(29)
#define SUBSYS_COMB1_PIPE			0xc44
#define SUBSYS_COMB1_PIPE_PCIE			0x1

/* Sub-System USB3.1 Bus Gating Reset Register */
#define SUBSYS_USB3P1_BGR			0x0008
#define   USB3P1_ONLY_UTMI_CLK_SEL		BIT(21)
#define   USB3P1_ONLY_PIPE_CLK_SEL		BIT(20)
#define   USB3P1_ACLK_EN			BIT(17)
#define   USB3P1_HCLK_EN			BIT(16)
#define   USB3P1_USB2P0_PHY_RSTN		BIT(4)

/* Sub-System DBG CTRL Register */
#define SUBSYS_DBG_CTL				0x00f0
#define   COMBO1_PHY_REFCLK_AUTO_GATE_DIS	BIT(29)
#define   COMBO0_PHY_REFCLK_AUTO_GATE_DIS	BIT(28)

/* COMB0 PHY PINS Link Control Register0 */
#define COMB0_PHY_PINS_LINK_CTRL0		0x0000
#define   PHY0_TYPEC_CONN_DIR			BIT(12) /* 0: normal orientation; 1: flipped orientation */
#define   PHY0_DP_LINK_RESET_N_SOFT		BIT(8)
#define   PHY0_PIPE_LINK_RESET_N_USB3P1_MASK	BIT(5)
#define   PHY0_PIPE_LINK_RESET_N_SOFT		BIT(4)
#define   PHY0_RESET_N				BIT(0)

/* COMB0 PHY PINS Link Control Register1 */
#define COMB0_PHY_PINS_LINK_CTRL1		0x0004
#define   PHY0_PMA_CMN_PLL1_REFCLK_SEL		BIT(24)
#define   PHY0_PMA_CMN_PLL0_REFCLK_SEL		BIT(20)
#define   PHY0_PMA_CMN_REFCLK_TERM_EN		BIT(16)
#define   PHY0_PMA_CMN_REFCLK_DIG_DIV_MASK	GENMASK(13, 12)
#define   PHY0_PMA_CMN_REFCLK_DIG_DIV(n)	(((n) & 0x3) << 12)
#define   PHY0_PMA_CMN_REFCLK_INT_MODE_MASK	GENMASK(9, 8)
#define   PHY0_PMA_CMN_REFCLK_INT_MODE(n)	(((n) & 0x3) << 8)
#define   PHY0_PMA_CMN_REFCLK_MODE_MASK		GENMASK(5, 4)
#define   PHY0_PMA_CMN_REFCLK_MODE(n)		(((n) & 0x3) << 4)
#define   PHY0_PMA_CMN_REFCLK_DIG_SEL		BIT(0)

/* COMB0 PHY PINS Link Control Register2 */
#define COMB0_PHY_PINS_LINK_CTRL2		0x0008
#define   PHY0_XCVR_BSCAN_MODE_EN_SEL		BIT(21)
#define   PHY0_XCVR_BSCAN_MODE_EN_SOFT		BIT(20)
#define   PHY0_BSCAN_EXT_SELECT			BIT(16)
#define   PHY0_DBG_RX_RD_LANE_SEL_MASK		GENMASK(6, 4)
#define   PHY0_DBG_RX_RD_LANE_SEL(n)		(((n) & 0x7) << 4)
#define   PHY0_IDDQ_EN_SEL			BIT(1)
#define   PHY0_IDDQ_EN_SOFT			BIT(0)

/* COMB0 PHY PINS Link Control Register3 */
#define COMB0_PHY_PINS_LINK_CTRL3		0x000C
#define   PHY0_PMA_TX_ELEC_IDLE_LN_3		BIT(15)
#define   PHY0_PMA_TX_ELEC_IDLE_LN_2		BIT(14)
#define   PHY0_PMA_TX_ELEC_IDLE_LN_1		BIT(13)
#define   PHY0_PMA_TX_ELEC_IDLE_LN_0		BIT(12)
#define   PHY0_PMA_XCVR_POWER_STATE_REQ_LN_MASK		GENMASK(9, 4)
#define   PHY0_PMA_XCVR_POWER_STATE_REQ_LN(n)	(((n) & 0x3f) << 4)
#define   PHY0_PMA_XCVR_PLLCLK_EN_LN		BIT(0)

/* COMB0 PHY PINS Link Status Register0 */
#define COMB0_PHY_PINS_LINK_STUS0		0x0900
#define   PHY0_LINK_POWER_STATE_ACK_MASK	GENMASK(13, 8)
#define   PHY0_LINK_POWER_STATE_ACK(n)		(((n) & 0x3f) << 8)
#define   PHY0_LINK_CLOCK_EN_ACK		BIT(4)
#define   PHY0_PMA_CMN_READY			BIT(0)

/* COMB1 PHY PINS Link Control Register0 */
#define COMB1_PHY_PINS_LINK_CTRL0		0x0000
#define   PHY1_LINK_CFG_LN_MASK			GENMASK(6, 4)
#define   PHY1_LINK_CFG_LN(n)			(((n) & 0x7) << 4)
#define   PHY1_RESET_N				BIT(0)

/* COMB1 PHY PINS Link Control Register1 */
#define COMB1_PHY_PINS_LINK_CTRL1		0x0004
#define   PHY1_EN_REFCLK			BIT(28)
#define   PHY1_PMA_CMN_PLL1_REFCLK_SEL		BIT(24)
#define   PHY1_PMA_CMN_PLL0_REFCLK_SEL		BIT(20)
#define   PHY1_PMA_CMN_REFCLK_TERM_EN		BIT(16)
#define   PHY1_PMA_CMN_REFCLK_DIG_DIV_MASK	GENMASK(13, 12)
#define   PHY1_PMA_CMN_REFCLK_DIG_DIV(n)	(((n) & 0x3) << 12)
#define   PHY1_PMA_CMN_REFCLK_INT_MODE_MASK	GENMASK(9, 8)
#define   PHY1_PMA_CMN_REFCLK_INT_MODE(n)	(((n) & 0x3) << 8)
#define   PHY1_PMA_CMN_REFCLK_MODE_MASK		GENMASK(5, 4)
#define   PHY1_PMA_CMN_REFCLK_MODE(n)		(((n) & 0x3) << 4)
#define   PHY1_PMA_CMN_REFCLK_DIG_SEL		BIT(0)

/* COMB1 PHY PINS Link Control Register3 */
#define COMB1_PHY_PINS_LINK_CTRL3		0x000C
#define   PHY1_XCVR_BSCAN_MODE_EN		BIT(20)
#define   PHY1_BSCAN_EXT_SELECT			BIT(16)
#define   PHY1_DTB_MUX_SELECTE_MASK		GENMASK(12, 8)
#define   PHY1_DTB_MUX_SELECTE(n)		(((n) & 0x1f) << 8)
#define   PHY1_IDDQ_EN_SEL			BIT(1)
#define   PHY1_IDDQ_EN				BIT(0)

/* COMB1 PHY PINS Lane0 Control Register1 */
#define COMB1_PHY_PINS_LANE0_CTRL0		0x0100
#define   PHY1_L00_ENT_L1_X			BIT(16)
#define   PHY1_L00_PCIE_L1_SS_SEL		BIT(12)
#define   PHY1_PMA_FULLRT_DIV_LN_0_MASK		GENMASK(9, 8)
#define   PHY1_PMA_FULLRT_DIV_LN_0(n)		(((n) & 0x3) << 8)
#define   PHY1_L00_MODE_MASK			GENMASK(5, 4)
#define   PHY1_L00_MODE(n)			(((n) & 0x3) << 4)
#define   PHY1_L00_RESET_N_HW_DIS		BIT(1)
#define   PHY1_L00_RESET_N_SW			BIT(0)

/* COMB1 PHY PINS Link Status Register0 */
#define COMB1_PHY_PINS_LINK_STUS0		0x0900
#define   PHY1_PMA_CMN_CLOCK_STOP_ACK		BIT(8)
#define   PHY1_PMA_CMN_EXT_REFCLK_DETECTED_VALID	BIT(5)
#define   PHY1_PMA_CMN_EXT_REFCLK_DETECTED	BIT(4)
#define   PHY1_PMA_CMN_READY			BIT(0)

/* COMBO0 DP RAWS TXDATA CONVERTION CTRL Register */
#define DP_RAWS_TXDW_CONV_CTRL			0x0004
#define DP_PHY_CFG_DATA_EN_BIT			BIT(0)
#define DP_PHY_CFG_LN_EN_MASK			GENMASK(7, 4)
#define DP_PHY_CFG_LN_RSTN_MASK			GENMASK(11, 8)

/* USB3.1 PIPE CLK MAP CTRL Register */
#define USB3P1_PIPE_CLK_MAP_CTRL		0x0100
#define   USB3P1_PIPE_CLK_MAP_MASK		GENMASK(3, 0)
#define   USB3P1_PIPE_CLK_MAP(n)		(((n) & 0xf)) /* 0x0: PHY0 Lane0 or Lane3; 0x1: PHY1 Lane0 */

/* USB3.1 PIPE RXDATA MAP CTRL Register */
#define USB3P1_PIPE_RXD_MAP_CTRL		0x0104
#define   USB3P1_PIPE_RXD_MAP_MASK		GENMASK(3, 0)
#define   USB3P1_PIPE_RXD_MAP(n)		(((n) & 0xf)) /* 0x0: PHY0 Lane0 or Lane2; 0x1: PHY1 Lane0 */

/* COMB1 PHY PIPE TXDATA MAP CTRL Register */
#define COMB1_PHY_PIPE_TXDATA_MAP_CTRL		0x0c44
#define   COMB1_PHY_LN0_PIPE_TXD_MAP_MASK	GENMASK(3, 0)
#define   COMB1_PHY_LN0_PIPE_TXD_MAP(n)		(((n) & 0xf)) /* 0x0: USB3.1 PIPE; 0x1: PCIE0 Lane0 */

/* SUBSYS COMB0 PHY Version Register */
#define COMB0_PHY_VER				0x1FE0
#define   PHY0_GEN_VER				GENMASK(31, 24)
#define   PHY0_SUB_VER				GENMASK(23, 16)
#define   PHY0_PRJ_VER				GENMASK(15, 8)

/* SUBSYS COMB1 PHY Version Register */
#define COMB1_PHY_VER				0x1FE4
#define   PHY1_GEN_VER				GENMASK(31, 24)
#define   PHY1_SUB_VER				GENMASK(23, 16)
#define   PHY1_PRJ_VER				GENMASK(15, 8)

/* SUBSYS TOP Version Register */
#define SUBSYS_TOP_VER				0x1FFC
#define   TOP_GEN_VER				GENMASK(31, 24)
#define   TOP_SUB_VER				GENMASK(23, 16)
#define   TOP_PRJ_VER				GENMASK(15, 8)

/* Registers */
#define SUBSYS_REG_PCIEBGR(top_subsys_reg)		((top_subsys_reg) \
								+ SUBSYS_PCIE_BGR)
#define SUBSYS_REG_USB3BGR(top_subsys_reg)		((top_subsys_reg) \
								+ SUBSYS_USB3P1_BGR)
#define SUBSYS_REG_DBGCTRL(top_subsys_reg)		((top_subsys_reg) \
								+ SUBSYS_DBG_CTL)

#define COMB0_REG_PHYPINS_LINK_CTRL0(top_reg)			((top_reg) \
									+ COMB0_PHY_PINS_LINK_CTRL0)

#define COMB0_REG_PHYPINS_LINK_CTRL1(top_reg)			((top_reg) \
									+ COMB0_PHY_PINS_LINK_CTRL1)

#define COMB0_REG_PHYPINS_LINK_CTRL2(top_reg)			((top_reg) \
									+ COMB0_PHY_PINS_LINK_CTRL2)

#define COMB0_REG_PHYPINS_LINK_CTRL3(top_reg)			((top_reg) \
									+ COMB0_PHY_PINS_LINK_CTRL3)

#define COMB0_REG_PHYPINS_LINK_STUS0(top_reg)			((top_reg) \
									+ COMB0_PHY_PINS_LINK_STUS0)

#define COMB1_REG_PHYPINS_LINK_CTRL0(top_reg)			((top_reg) \
									+ COMB1_PHY_PINS_LINK_CTRL0)

#define COMB1_REG_PHYPINS_LINK_CTRL1(top_reg)			((top_reg) \
									+ COMB1_PHY_PINS_LINK_CTRL1)

#define COMB1_REG_PHYPINS_LINK_CTRL3(top_reg)			((top_reg) \
									+ COMB1_PHY_PINS_LINK_CTRL3)

#define COMB1_REG_PHYPINS_LANE0_CTRL0(top_reg)			((top_reg) \
									+ COMB1_PHY_PINS_LANE0_CTRL0)

#define COMB1_REG_PHYPINS_LINK_STUS0(top_reg)			((top_reg) \
									+ COMB1_PHY_PINS_LINK_STUS0)

#define DP_RAWS_TXDW_CONV_CRTL(top_combo_reg)			((top_combo_reg) \
									+ DP_RAWS_TXDW_CONV_CTRL)

#define USB3P1_REG_PIPE_CLK_MAP_CTRL(top_combo_reg)		((top_combo_reg) \
									+ USB3P1_PIPE_CLK_MAP_CTRL)

#define USB3P1_REG_PIPE_RXD_MAP_CTRL(top_combo_reg)		((top_combo_reg) \
									+ USB3P1_PIPE_RXD_MAP_CTRL)

#define COMB1_REG_PHYPIPE_TXDATA_MAP_CTRL(top_combo_reg)	((top_combo_reg) \
									+ COMB1_PHY_PIPE_TXDATA_MAP_CTRL)

#define COMB0_REG_PHY_VER(top_combo_reg)			((top_combo_reg) \
									+ COMB0_PHY_VER)

#define COMB1_REG_PHY_VER(top_combo_reg)			((top_combo_reg) \
									+ COMB1_PHY_VER)

#define SUBSYS_REG_TOP_VER(top_combo_reg)			((top_combo_reg) \
									+ SUBSYS_TOP_VER)

struct dp_signal_level {
	u16 lane_txdrv[MAX_LANE_CNT];
	u16 lane_mgnfs[MAX_LANE_CNT];
	u16 lane_cpost[MAX_LANE_CNT];
};

#define DP_TXDRV(r0, r1, r2, r3) \
	.lane_txdrv[0] = r0, \
	.lane_txdrv[1] = r1, \
	.lane_txdrv[2] = r2, \
	.lane_txdrv[3] = r3,

#define DP_MGNFS(r0, r1, r2, r3) \
	.lane_mgnfs[0] = r0, \
	.lane_mgnfs[1] = r1, \
	.lane_mgnfs[2] = r2, \
	.lane_mgnfs[3] = r3,

#define DP_CPOST(r0, r1, r2, r3) \
	.lane_cpost[0] = r0, \
	.lane_cpost[1] = r1, \
	.lane_cpost[2] = r2, \
	.lane_cpost[3] = r3,


static struct dp_signal_level dp_lv_table_5g4[MAX_SWING][MAX_PRE] = {
	/* sw-0 */
	{
		{
			/* sw-0 pre-0 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0025, 0x0024, 0x0024, 0x0025)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-0 pre-1 */
			DP_TXDRV(0x00A3, 0x00A3, 0x00A3, 0x00A3)
			DP_MGNFS(0x001E, 0x001D, 0x001D, 0x001E)
			DP_CPOST(0x000A, 0x000A, 0x000A, 0x000A)
		},
		{
			/* sw-0 pre-2 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0011, 0x0010, 0x0010, 0x0011)
			DP_CPOST(0x001C, 0x001C, 0x001C, 0x001C)
		},
		{
			/* sw-0 pre-3 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0001, 0x0000, 0x0000, 0x0001)
			DP_CPOST(0x0027, 0x0027, 0x0027, 0x0027)
		},
	},
	/* sw-1 */
	{
		{
			/* sw-1 pre-0 */
			DP_TXDRV(0x0002, 0x0002, 0x0002, 0x0002)
			DP_MGNFS(0x001D, 0x001C, 0x001C, 0x001D)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-1 pre-1 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0012, 0x0011, 0x0011, 0x0012)
			DP_CPOST(0x000A, 0x000A, 0x000A, 0x000A)
		},
		{
			/* sw-1 pre-2 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0001, 0x0000, 0x0000, 0x0001)
			DP_CPOST(0x001E, 0x001E, 0x001E, 0x001E)
		},
		{
			/* sw-1 pre-3 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0001, 0x0000, 0x0000, 0x0001)
			DP_CPOST(0x001E, 0x001E, 0x001E, 0x001E)
		},
	},
	/* sw-2 */
	{
		{
			/* sw-2 pre-0 */
			DP_TXDRV(0x0002, 0x0002, 0x0002, 0x0002)
			DP_MGNFS(0x0010, 0x000F, 0x000F, 0x0010)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-2 pre-1 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x000F, 0x000F, 0x000F, 0x000F)
		},
		{
			/* sw-2 pre-2 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x000F, 0x000F, 0x000F, 0x000F)
		},
		{
			/* sw-2 pre-3 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x000F, 0x000F, 0x000F, 0x000F)
		},
	},
	/* sw-3 */
	{
		{
			/* sw-3 pre-0 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-3 pre-1 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-3 pre-2 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-3 pre-3 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
	},

};

static struct dp_signal_level dp_lv_table_2g7[MAX_SWING][MAX_PRE] = {
	/* sw-0 */
	{
		{
			/* sw-0 pre-0 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0028, 0x0026, 0x0026, 0x0028)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-0 pre-1 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0024, 0x0023, 0x0023, 0x0024)
			DP_CPOST(0x0008, 0x0008, 0x0008, 0x0008)
		},
		{
			/* sw-0 pre-2 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x001C, 0x001B, 0x001B, 0x001C)
			DP_CPOST(0x0016, 0x0016, 0x0016, 0x0016)
		},
		{
			/* sw-0 pre-3 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x000B, 0x000A, 0x000A, 0x000B)
			DP_CPOST(0x0023, 0x0023, 0x0023, 0x0023)
		},
	},
	/* sw-1 */
	{
		{
			/* sw-1 pre-0 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x001F, 0x001E, 0x001E, 0x001F)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-1 pre-1 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x001A, 0x0019, 0x0019, 0x001A)
			DP_CPOST(0x000A, 0x000A, 0x000A, 0x000A)
		},
		{
			/* sw-1 pre-2 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x000A, 0x0009, 0x0009, 0x000A)
			DP_CPOST(0x0019, 0x0019, 0x0019, 0x0019)
		},
		{
			/* sw-1 pre-3 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x000A, 0x0009, 0x0009, 0x000A)
			DP_CPOST(0x0019, 0x0019, 0x0019, 0x0019)
		},
	},
	/* sw-2 */
	{
		{
			/* sw-2 pre-0 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0010, 0x000E, 0x000E, 0x0010)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-2 pre-1 */
			DP_TXDRV(0x0082, 0x0082, 0x0082, 0x0082)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0011, 0x0011, 0x0011, 0x0011)
		},
		{
			/* sw-2 pre-2 */
			DP_TXDRV(0x0082, 0x0082, 0x0082, 0x0082)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0011, 0x0011, 0x0011, 0x0011)
		},
		{
			/* sw-2 pre-3 */
			DP_TXDRV(0x0082, 0x0082, 0x0082, 0x0082)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0011, 0x0011, 0x0011, 0x0011)
		},
	},
	/* sw-3 */
	{
		{
			/* sw-3 pre-0 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-3 pre-1 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-3 pre-2 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-3 pre-3 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
	},

};

static struct dp_signal_level dp_lv_table_1g62[MAX_SWING][MAX_PRE] = {
	/* sw-0 */
	{
		{
			/* sw-0 pre-0 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0027, 0x0027, 0x0027, 0x0027)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-0 pre-1 */
			DP_TXDRV(0x00A2, 0x00A2, 0x00A2, 0x00A2)
			DP_MGNFS(0x0023, 0x0023, 0x0023, 0x0023)
			DP_CPOST(0x000A, 0x000A, 0x000A, 0x000A)
		},
		{
			/* sw-0 pre-2 */
			DP_TXDRV(0x0092, 0x0092, 0x0092, 0x0092)
			DP_MGNFS(0x001A, 0x001A, 0x001A, 0x001A)
			DP_CPOST(0x0016, 0x0016, 0x0016, 0x0016)
		},
		{
			/* sw-0 pre-3 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x000E, 0x000E, 0x000E, 0x000E)
			DP_CPOST(0x0020, 0x0020, 0x0020, 0x0020)
		},
	},
	/* sw-1 */
	{
		{
			/* sw-1 pre-0 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0020, 0x0020, 0x0020, 0x0020)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-1 pre-1 */
			DP_TXDRV(0x00A2, 0x00A2, 0x00A2, 0x00A2)
			DP_MGNFS(0x0019, 0x0018, 0x0018, 0x0019)
			DP_CPOST(0x000C, 0x000C, 0x000C, 0x000C)
		},
		{
			/* sw-1 pre-2 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x000D, 0x000C, 0x000C, 0x000D)
			DP_CPOST(0x0017, 0x0017, 0x0017, 0x0017)
		},
		{
			/* sw-1 pre-3 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x000D, 0x000C, 0x000C, 0x000D)
			DP_CPOST(0x0017, 0x0017, 0x0017, 0x0017)
		},
	},
	/* sw-2 */
	{
		{
			/* sw-2 pre-0 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0010, 0x000F, 0x000F, 0x0010)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-2 pre-1 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0004, 0x0003, 0x0003, 0x0004)
			DP_CPOST(0x0010, 0x0010, 0x0010, 0x0010)
		},
		{
			/* sw-2 pre-2 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0004, 0x0003, 0x0003, 0x0004)
			DP_CPOST(0x0010, 0x0010, 0x0010, 0x0010)
		},
		{
			/* sw-2 pre-3 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0004, 0x0003, 0x0003, 0x0004)
			DP_CPOST(0x0010, 0x0010, 0x0010, 0x0010)
		},
	},
	/* sw-3 */
	{
		{
			/* sw-3 pre-0 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-3 pre-1 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-3 pre-2 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-3 pre-3 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
	},

};

static void sunxi_cadence_phy_reset_control_put(void *data)
{
	reset_control_put(data);
}

/* Serdes Sub-System Top Support. */
static u32 top_ver_get(struct sunxi_cadence_phy *sunxi_cphy)
{
	u32 reg;

	reg = readl(SUBSYS_REG_TOP_VER(sunxi_cphy->top_combo_reg));

	return reg;
}

static void combo_usb2_clk_set(struct sunxi_cadence_phy *sunxi_cphy, bool enable)
{
	u32 val;

	val = readl(SUBSYS_REG_USB3BGR(sunxi_cphy->top_subsys_reg));
	if (enable)
		val |= USB3P1_USB2P0_PHY_RSTN;
	else
		val &= ~USB3P1_USB2P0_PHY_RSTN;
	writel(val, SUBSYS_REG_USB3BGR(sunxi_cphy->top_subsys_reg));
}

static void combo_usb_clk_set(struct sunxi_cadence_phy *sunxi_cphy, bool enable)
{
	u32 val, tmp = 0;

	val = readl(SUBSYS_REG_USB3BGR(sunxi_cphy->top_subsys_reg));
	tmp = USB3P1_ACLK_EN | USB3P1_HCLK_EN;
	if (enable)
		val |= tmp;
	else
		val &= ~tmp;
	writel(val, SUBSYS_REG_USB3BGR(sunxi_cphy->top_subsys_reg));
}


/***************************************************************************************************
 * Combo PHY0 Support.                                                                             *
 * - PIPE interface mode  USB3.1 SS and SS+                                                        *
 * - Raw SerDes interface mode  DisplayPort Tx / embedded DisplayPort Tx                           *
 * - Supports up to 4Lane                                                                          *
 **************************************************************************************************/
void combo0_dp_phy_reset(struct sunxi_cadence_combophy *combo0, bool deassert)
{
	struct sunxi_cadence_phy *sunxi_cphy = combo0->sunxi_cphy;
	u32 phy0_pma_cmn_ready = 0;
	u32 reg_val;
	u32 retry_cnt = 0;

	if (deassert) {
		reg_val = readl(COMB0_REG_PHYPINS_LINK_CTRL2(combo0->top_reg));
		reg_val &= ~(PHY0_IDDQ_EN_SEL | PHY0_IDDQ_EN_SOFT);
		writel(reg_val, COMB0_REG_PHYPINS_LINK_CTRL2(combo0->top_reg));

		//PHY RESET
		phy_writel(combo0->top_reg + 0x00, 0x0);

		//PHY RESET
		phy_writel(combo0->top_reg + 0x00, 0x001);
		msleep(1);
		//DP LINK RESET
		phy_writel(combo0->top_reg + 0x00, 0x101);
		msleep(1);
		//PHY RESET
		phy_writel(combo0->top_reg + 0x00, 0x131);
		msleep(1);

		reg_val = readl(COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));
		if (combo0->orientation == TYPEC_ORIENTATION_REVERSE)
			reg_val |= PHY0_TYPEC_CONN_DIR;
		else
			reg_val &= ~PHY0_TYPEC_CONN_DIR;
		writel(reg_val, COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));

		msleep(1);

		while (phy0_pma_cmn_ready == 0) {
			if (++retry_cnt > 1000) {
				pr_warn("wrn: %s dp wait pll enable timeout line:%d\n", combo0->name, __LINE__);
				break;
			}
			reg_val = phy_readl(combo0->top_reg+0x900);
			reg_val = reg_val & (1 << 0);
			phy0_pma_cmn_ready = reg_val;

			udelay(5);
		}

		phy_set_lbits(sunxi_cphy->top_combo_reg + 0x4, 0, 1, 1);
		phy_set_lbits(sunxi_cphy->top_combo_reg + 0x4, 4, 4, 0xf);
		phy_set_lbits(sunxi_cphy->top_combo_reg + 0x4, 8, 4, 0xf);

		phy_set_lbits(combo0->top_reg + 0x0c, 0, 1, 1);

		phy0_pma_cmn_ready = 0;
		retry_cnt = 0;
		while (phy0_pma_cmn_ready == 0) {
			if (++retry_cnt > 1000) {
				pr_warn("wrn: %s dp wait pll enable timeout line:%d\n", combo0->name, __LINE__);
				break;
			}
			reg_val = phy_readl(combo0->top_reg+0x900);
			reg_val = (reg_val & (1 << 4)) >> 4;
			phy0_pma_cmn_ready = reg_val;

			udelay(5);
		}

		msleep(1);

		phy_set_lbits(combo0->top_reg + 0x0c, 4, 6, 1);

		phy0_pma_cmn_ready = 0;
		retry_cnt = 0;
		while (phy0_pma_cmn_ready == 0) {
			if (++retry_cnt > 100000) {
				pr_warn("wrn: %s dp wait pll enable timeout line:%d\n", combo0->name, __LINE__);
				break;
			}
			reg_val = phy_readl(combo0->top_reg+0x900);
			reg_val = (reg_val & (0x3f << 8)) >> 8;
			phy0_pma_cmn_ready = reg_val;

			udelay(5);
		}
	} else {
		//PHY RESET
		phy_writel(combo0->top_reg + 0x00, 0x0);
		phy_writel(sunxi_cphy->top_combo_reg + 0x4, 0x0);
		phy_writel(combo0->top_reg + 0x0c, 0x0);
		msleep(1);
	}
}

static int combo0_dp_set_rate(struct sunxi_cadence_combophy *combo0,
		       struct phy_configure_opts_dp *dp_config)
{
	u32 val;

	if (dp_config->set_rate == 1) {
		/* if in typec STATE_D/F, skip phy configure/reset which has been set */
		if (combo0->usb_dp_state) {
			switch (dp_config->link_rate) {
			case 2700:
				if (combo0->orientation == TYPEC_ORIENTATION_REVERSE) {
					phy_writew(combo0->phy_reg + (0x01c1 << 1), 0x0701);
					phy_writew(combo0->phy_reg + (0x40e7 << 1), 0x1);
					phy_writew(combo0->phy_reg + (0x42e7 << 1), 0x1);
				} else {
					phy_writew(combo0->phy_reg + (0x01c1 << 1), 0x0701);
					phy_writew(combo0->phy_reg + (0x44e7 << 1), 0x1);
					phy_writew(combo0->phy_reg + (0x46e7 << 1), 0x1);
				}
				break;
			case 5400:
				if (combo0->orientation == TYPEC_ORIENTATION_REVERSE) {
					phy_writew(combo0->phy_reg + (0x01c1 << 1), 0x0601);
					phy_writew(combo0->phy_reg + (0x40e7 << 1), 0x0);
					phy_writew(combo0->phy_reg + (0x42e7 << 1), 0x0);
				} else {
					phy_writew(combo0->phy_reg + (0x01c1 << 1), 0x0601);
					phy_writew(combo0->phy_reg + (0x44e7 << 1), 0x0);
					phy_writew(combo0->phy_reg + (0x46e7 << 1), 0x0);
				}
				break;
			case 1620:
			case 8100:
			default:
				pr_warn("[Cadence PHY]: Unsupport link_rate:%d state:%d\n",
					dp_config->link_rate, combo0->usb_dp_state);
				return -EINVAL;
			}
			combo0->link_rate = dp_config->link_rate;
		} else {
			combo0_dp_phy_reset(combo0, false);
			switch (dp_config->link_rate) {
			case 1620:
				// set ref-clk to 26M
				val = readl(COMB0_REG_PHYPINS_LINK_CTRL2(combo0->top_reg));
				val &= ~(PHY0_IDDQ_EN_SEL | PHY0_IDDQ_EN_SOFT);
				writel(val, COMB0_REG_PHYPINS_LINK_CTRL2(combo0->top_reg));

				phy_writel(combo0->top_reg + 0x4, 0x1100001);

				//CMN_SSM_BIAS_TMR
				phy_writew(combo0->phy_reg + (0x0022 << 1), 0x001a);
				//CMN_PLLSM0_PLLPRE_TMR
				phy_writew(combo0->phy_reg + (0x002A << 1), 0x0034);
				//CMN_PLLSM0_PLLLOCK_TMR
				phy_writew(combo0->phy_reg + (0x002C << 1), 0x00da);

				//CMN_PLLSM1_PLLPRE_TMR
				phy_writew(combo0->phy_reg + (0x0032 << 1), 0x0034);
				//CMN_PLLSM1_PLLLOCK_TMR
				phy_writew(combo0->phy_reg + (0x0034 << 1), 0x00da);

				//CMN_BGCAL_INIT_TMR
				phy_writew(combo0->phy_reg + (0x0064 << 1), 0x0082);
				//CMN_BGCAL_ITER_TMR
				phy_writew(combo0->phy_reg + (0x0065 << 1), 0x0082);
				//CMN_IBCAL_INIT_TMR
				phy_writew(combo0->phy_reg + (0x0074 << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x0104 << 1), 0x0020);
				phy_writew(combo0->phy_reg + (0x0105 << 1), 0x0007);
				phy_writew(combo0->phy_reg + (0x010c << 1), 0x0020);
				phy_writew(combo0->phy_reg + (0x010d << 1), 0x0007);
				phy_writew(combo0->phy_reg + (0x0114 << 1), 0x030c);
				phy_writew(combo0->phy_reg + (0x0115 << 1), 0x0007);
				phy_writew(combo0->phy_reg + (0x0124 << 1), 0x0007);
				phy_writew(combo0->phy_reg + (0x0125 << 1), 0x0003);
				phy_writew(combo0->phy_reg + (0x0126 << 1), 0x000f);
				phy_writew(combo0->phy_reg + (0x0128 << 1), 0x0132);

				phy_writew(combo0->phy_reg + (0x8044 << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x8244 << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x8444 << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x8644 << 1), 0x001a);

				phy_writew(combo0->phy_reg + (0x8045 << 1), 0x0082);
				phy_writew(combo0->phy_reg + (0x8245 << 1), 0x0082);
				phy_writew(combo0->phy_reg + (0x8445 << 1), 0x0082);
				phy_writew(combo0->phy_reg + (0x8645 << 1), 0x0082);

				phy_writew(combo0->phy_reg + (0x804c << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x824c << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x844c << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x864c << 1), 0x001a);

				phy_writew(combo0->phy_reg + (0x804d << 1), 0x0082);
				phy_writew(combo0->phy_reg + (0x824d << 1), 0x0082);
				phy_writew(combo0->phy_reg + (0x844d << 1), 0x0082);
				phy_writew(combo0->phy_reg + (0x864d << 1), 0x0082);

				phy_writew(combo0->phy_reg + (0x4123 << 1), 0x0a28);
				phy_writew(combo0->phy_reg + (0x4323 << 1), 0x0a28);
				phy_writew(combo0->phy_reg + (0x4523 << 1), 0x0a28);
				phy_writew(combo0->phy_reg + (0x4723 << 1), 0x0a28);

				//VCO calibration settings.
				phy_writew(combo0->phy_reg + (0x00c4 << 1), 0x0104);
				phy_writew(combo0->phy_reg + (0x00c5 << 1), 0x0005);
				phy_writew(combo0->phy_reg + (0x00c6 << 1), 0x0337);
				phy_writew(combo0->phy_reg + (0x00c8 << 1), 0x0335);
				phy_writew(combo0->phy_reg + (0x00c2 << 1), 0x0003);

				phy_writew(combo0->phy_reg + (0x0084 << 1), 0x0104);
				phy_writew(combo0->phy_reg + (0x0085 << 1), 0x0005);
				phy_writew(combo0->phy_reg + (0x0086 << 1), 0x0337);
				phy_writew(combo0->phy_reg + (0x0088 << 1), 0x0335);
				phy_writew(combo0->phy_reg + (0x0082 << 1), 0x0003);

				 //PLL lock detect settings
				phy_writew(combo0->phy_reg + (0x00dc << 1), 0x00cf);
				phy_writew(combo0->phy_reg + (0x00de << 1), 0x00ce);
				phy_writew(combo0->phy_reg + (0x00dF << 1), 0x0005);

				phy_writew(combo0->phy_reg + (0x009c << 1), 0x00cf);
				phy_writew(combo0->phy_reg + (0x009e << 1), 0x00ce);
				phy_writew(combo0->phy_reg + (0x009F << 1), 0x0005);

				//DP_LANE_SET as follow
				phy_writew(combo0->phy_reg + (0xc010 << 1), 0x51d9);
				//phy DP lane enabled
				phy_writew(combo0->phy_reg + (0xc011 << 1), 0x0100);

				//PHY_PMA_ISO_PLL_CTRL0   cmn_pll1_clk_en
				phy_writew(combo0->phy_reg + (0xe003 << 1), 0x0003);
				phy_writew(combo0->phy_reg + (0xe005 << 1), 0x000B);
				//PHY_PMA_ISO_PLL_CTRL1
				phy_writew(combo0->phy_reg + (0xe006 << 1), 0x2224);

				//TX_PSC_A0
				phy_writew(combo0->phy_reg + (0x4100 << 1), 0x00FB);
				phy_writew(combo0->phy_reg + (0x4300 << 1), 0x00FB);
				phy_writew(combo0->phy_reg + (0x4500 << 1), 0x00FB);
				phy_writew(combo0->phy_reg + (0x4700 << 1), 0x00FB);

				//TX_PSC_A2
				phy_writew(combo0->phy_reg + (0x4102 << 1), 0x04AA);
				phy_writew(combo0->phy_reg + (0x4302 << 1), 0x04AA);
				phy_writew(combo0->phy_reg + (0x4502 << 1), 0x04AA);
				phy_writew(combo0->phy_reg + (0x4702 << 1), 0x04AA);

				//TX_PSC_A3
				phy_writew(combo0->phy_reg + (0x4102 << 1), 0x04AA);
				phy_writew(combo0->phy_reg + (0x4302 << 1), 0x04AA);
				phy_writew(combo0->phy_reg + (0x4502 << 1), 0x04AA);
				phy_writew(combo0->phy_reg + (0x4702 << 1), 0x04AA);

				//RX all set to 0
				//RX_PSC_A0
				phy_writew(combo0->phy_reg + (0x8000 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8200 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8400 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8600 << 1), 0x0);

				//RX_PSC_A0
				phy_writew(combo0->phy_reg + (0x8000 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8202 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8402 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8602 << 1), 0x0);

				//RX_PSC_A0
				phy_writew(combo0->phy_reg + (0x8002 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8203 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8403 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8603 << 1), 0x0);

				//RX_PSC_CAL
				phy_writew(combo0->phy_reg + (0x8006 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8206 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8406 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8606 << 1), 0x0);

				//RX_REE_GCSM1_CTRL
				phy_writew(combo0->phy_reg + (0x8108 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8308 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8508 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8708 << 1), 0x0);

				//RX_REE_GCSM2_CTRL
				phy_writew(combo0->phy_reg + (0x8110 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8310 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8510 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8710 << 1), 0x0);

				//RX_REE_PERGCSM_CTRL
				phy_writew(combo0->phy_reg + (0x8118 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8318 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8518 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8718 << 1), 0x0);

				//XCVR_DIAG_BIDI_CTRL  diefferent mode with TX/RX enable
				phy_writew(combo0->phy_reg + (0x40EA << 1), 0x000F);
				phy_writew(combo0->phy_reg + (0x42EA << 1), 0x000F);
				phy_writew(combo0->phy_reg + (0x44EA << 1), 0x000F);
				phy_writew(combo0->phy_reg + (0x46EA << 1), 0x000F);

				//TX_TXCC_CTRL
				phy_writew(combo0->phy_reg + (0x4040 << 1), 0x08A4);
				phy_writew(combo0->phy_reg + (0x4240 << 1), 0x08A4);
				phy_writew(combo0->phy_reg + (0x4440 << 1), 0x08A4);
				phy_writew(combo0->phy_reg + (0x4640 << 1), 0x08A4);

				//DRV_DIAG_TX_DRV
				phy_writew(combo0->phy_reg + (0x40C6 << 1), 0x0003);
				phy_writew(combo0->phy_reg + (0x42C6 << 1), 0x0003);
				phy_writew(combo0->phy_reg + (0x44C6 << 1), 0x0003);
				phy_writew(combo0->phy_reg + (0x46C6 << 1), 0x0003);

				phy_writew(combo0->phy_reg + (0x4050 << 1), 0x002A);
				phy_writew(combo0->phy_reg + (0x4250 << 1), 0x002A);
				phy_writew(combo0->phy_reg + (0x4450 << 1), 0x002A);
				phy_writew(combo0->phy_reg + (0x4650 << 1), 0x002A);

				phy_writew(combo0->phy_reg + (0x404C << 1), 0x0000);
				phy_writew(combo0->phy_reg + (0x424C << 1), 0x0000);
				phy_writew(combo0->phy_reg + (0x444C << 1), 0x0000);
				phy_writew(combo0->phy_reg + (0x464C << 1), 0x0000);

				//CMN-pull up-down res cal
				phy_writew(combo0->phy_reg + (0x0103 << 1), 0x007F);
				phy_writew(combo0->phy_reg + (0x010B << 1), 0x007F);

				phy_writew(combo0->phy_reg + (0x4140 << 1), 0x0801);
				phy_writew(combo0->phy_reg + (0x4340 << 1), 0x0801);
				phy_writew(combo0->phy_reg + (0x4540 << 1), 0x0801);
				phy_writew(combo0->phy_reg + (0x4740 << 1), 0x0801);

				//ssc
				if (combo0->ssc_en)
					phy_writew(combo0->phy_reg + (0x00D8 << 1), 0x0001);
				else
					phy_writew(combo0->phy_reg + (0x00D8 << 1), 0x0002);
				phy_writew(combo0->phy_reg + (0x00D9 << 1), 0x04C4);
				phy_writew(combo0->phy_reg + (0x00DA << 1), 0x006A);
				phy_writew(combo0->phy_reg + (0x00DB << 1), 0x0004);

				phy_writew(combo0->phy_reg + (0x0098 << 1), 0x0001);
				phy_writew(combo0->phy_reg + (0x00D9 << 1), 0x044A);
				phy_writew(combo0->phy_reg + (0x009A << 1), 0x006b);
				phy_writew(combo0->phy_reg + (0x009B << 1), 0x0004);

				// pll configure
				phy_writew(combo0->phy_reg + (0x00d4 << 1), 0x0004);
				phy_writew(combo0->phy_reg + (0x01C4 << 1), 0x0509);
				phy_writew(combo0->phy_reg + (0x01C5 << 1), 0x0F00);
				phy_writew(combo0->phy_reg + (0x01C6 << 1), 0x0F08);

				phy_writew(combo0->phy_reg + (0x00d0 << 1), 0x0175);
				phy_writew(combo0->phy_reg + (0x00d1 << 1), 0xD89E);
				phy_writew(combo0->phy_reg + (0x00d2 << 1), 0x0002);
				phy_writew(combo0->phy_reg + (0x00d3 << 1), 0x00FA);
				phy_writew(combo0->phy_reg + (0x01c0 << 1), 0x0002);

				//SET common  PLL1_CLOCK0
				phy_writew(combo0->phy_reg + (0x1c1 << 1), 0x0F01);
				phy_writew(combo0->phy_reg + (0x004B << 1), 0x0);

				//dp XCVR(LANE) pll1 seting
				phy_writew(combo0->phy_reg + (0x40e6 << 1), 0x0001);
				phy_writew(combo0->phy_reg + (0x42e6 << 1), 0x0001);
				phy_writew(combo0->phy_reg + (0x44e6 << 1), 0x0001);
				phy_writew(combo0->phy_reg + (0x46e6 << 1), 0x0001);

				phy_writew(combo0->phy_reg + (0x40e7 << 1), 0x0002);
				phy_writew(combo0->phy_reg + (0x42e7 << 1), 0x0002);
				phy_writew(combo0->phy_reg + (0x44e7 << 1), 0x0002);
				phy_writew(combo0->phy_reg + (0x46e7 << 1), 0x0002);
				phy_writew(combo0->phy_reg + (0x40e5 << 1), 0x0019);
				phy_writew(combo0->phy_reg + (0x42e5 << 1), 0x0019);
				phy_writew(combo0->phy_reg + (0x44e5 << 1), 0x0019);
				phy_writew(combo0->phy_reg + (0x46e5 << 1), 0x0019);
				phy_writew(combo0->phy_reg + (0xe006 << 1), 0x2224);

				//PLL0 configure
				phy_writew(combo0->phy_reg + (0x1A1 << 1), 0x8600);
				phy_writew(combo0->phy_reg + (0x0094 << 1), 0x0004);
				phy_writew(combo0->phy_reg + (0x01A4 << 1), 0x0509);
				phy_writew(combo0->phy_reg + (0x01A5 << 1), 0x0F00);
				phy_writew(combo0->phy_reg + (0x01A6 << 1), 0x0F08);
				phy_writew(combo0->phy_reg + (0x0090 << 1), 0x0180);
				phy_writew(combo0->phy_reg + (0x0091 << 1), 0x9DBA);
				phy_writew(combo0->phy_reg + (0x0092 << 1), 0x0002);
				phy_writew(combo0->phy_reg + (0x0093 << 1), 0x0102);
				phy_writew(combo0->phy_reg + (0x01A0 << 1), 0x0002);
				break;
			case 2160:
				break;
			case 2430:
				break;
			case 2700:
				val = readl(COMB0_REG_PHYPINS_LINK_CTRL2(combo0->top_reg));
				val &= ~(PHY0_IDDQ_EN_SEL | PHY0_IDDQ_EN_SOFT);
				writel(val, COMB0_REG_PHYPINS_LINK_CTRL2(combo0->top_reg));

				phy_writel(combo0->top_reg + 0x4, 0x1100001);

				//CMN_SSM_BIAS_TMR
				phy_writew(combo0->phy_reg + (0x0022 << 1), 0x001a);

				//CMN_PLLSM0_PLLPRE_TMR
				phy_writew(combo0->phy_reg + (0x002A << 1), 0x0034);
				//CMN_PLLSM0_PLLLOCK_TMR
				phy_writew(combo0->phy_reg + (0x002C << 1), 0x00da);

				//CMN_PLLSM1_PLLPRE_TMR
				phy_writew(combo0->phy_reg + (0x0032 << 1), 0x0034);
				//CMN_PLLSM1_PLLLOCK_TMR
				phy_writew(combo0->phy_reg + (0x0034 << 1), 0x00da);

				//CMN_BGCAL_INIT_TMR
				phy_writew(combo0->phy_reg + (0x0064 << 1), 0x0082);
				//CMN_BGCAL_ITER_TMR
				phy_writew(combo0->phy_reg + (0x0065 << 1), 0x0082);
				//CMN_IBCAL_INIT_TMR
				phy_writew(combo0->phy_reg + (0x0074 << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x0104 << 1), 0x0020);
				phy_writew(combo0->phy_reg + (0x0105 << 1), 0x0007);
				phy_writew(combo0->phy_reg + (0x010c << 1), 0x0020);
				phy_writew(combo0->phy_reg + (0x010d << 1), 0x0007);
				phy_writew(combo0->phy_reg + (0x0114 << 1), 0x030c);
				phy_writew(combo0->phy_reg + (0x0115 << 1), 0x0007);
				phy_writew(combo0->phy_reg + (0x0124 << 1), 0x0007);
				phy_writew(combo0->phy_reg + (0x0125 << 1), 0x0003);
				phy_writew(combo0->phy_reg + (0x0126 << 1), 0x000f);
				phy_writew(combo0->phy_reg + (0x0128 << 1), 0x0132);

				phy_writew(combo0->phy_reg + (0x8044 << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x8244 << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x8444 << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x8644 << 1), 0x001a);

				phy_writew(combo0->phy_reg + (0x8045 << 1), 0x0082);
				phy_writew(combo0->phy_reg + (0x8245 << 1), 0x0082);
				phy_writew(combo0->phy_reg + (0x8445 << 1), 0x0082);
				phy_writew(combo0->phy_reg + (0x8645 << 1), 0x0082);

				phy_writew(combo0->phy_reg + (0x804c << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x824c << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x844c << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x864c << 1), 0x001a);

				phy_writew(combo0->phy_reg + (0x804d << 1), 0x0082);
				phy_writew(combo0->phy_reg + (0x824d << 1), 0x0082);
				phy_writew(combo0->phy_reg + (0x844d << 1), 0x0082);
				phy_writew(combo0->phy_reg + (0x864d << 1), 0x0082);

				phy_writew(combo0->phy_reg + (0x4123 << 1), 0x0a28);
				phy_writew(combo0->phy_reg + (0x4323 << 1), 0x0a28);
				phy_writew(combo0->phy_reg + (0x4523 << 1), 0x0a28);
				phy_writew(combo0->phy_reg + (0x4723 << 1), 0x0a28);

				//VCO calibration settings.
				phy_writew(combo0->phy_reg + (0x00c4 << 1), 0x0104);
				phy_writew(combo0->phy_reg + (0x00c5 << 1), 0x0005);
				phy_writew(combo0->phy_reg + (0x00c6 << 1), 0x0337);
				phy_writew(combo0->phy_reg + (0x00c8 << 1), 0x0335);
				phy_writew(combo0->phy_reg + (0x00c2 << 1), 0x0003);
				phy_writew(combo0->phy_reg + (0x0084 << 1), 0x0104);
				phy_writew(combo0->phy_reg + (0x0085 << 1), 0x0005);
				phy_writew(combo0->phy_reg + (0x0086 << 1), 0x0337);
				phy_writew(combo0->phy_reg + (0x0088 << 1), 0x0335);
				phy_writew(combo0->phy_reg + (0x0082 << 1), 0x0003);

				//PLL lock detect settings
				phy_writew(combo0->phy_reg + (0x00dc << 1), 0x00cf);
				phy_writew(combo0->phy_reg + (0x00de << 1), 0x00ce);
				phy_writew(combo0->phy_reg + (0x00dF << 1), 0x0005);
				phy_writew(combo0->phy_reg + (0x009c << 1), 0x00cf);
				phy_writew(combo0->phy_reg + (0x009e << 1), 0x00ce);
				phy_writew(combo0->phy_reg + (0x009F << 1), 0x0005);

				//DP_LANE_SET
				phy_writew(combo0->phy_reg + (0xc010 << 1), 0x51d9);
				phy_writew(combo0->phy_reg + (0xc011 << 1), 0x0100);
				//PHY_PMA_ISO_PLL_CTRL0   cmn_pll1_clk_en
				phy_writew(combo0->phy_reg + (0xe003 << 1), 0x0003);
				phy_writew(combo0->phy_reg + (0xe005 << 1), 0x000B);
				//PHY_PMA_ISO_PLL_CTRL1
				phy_writew(combo0->phy_reg + (0xe006 << 1), 0x2224);
				//TX_PSC_A0
				phy_writew(combo0->phy_reg + (0x4100 << 1), 0x00FB);
				phy_writew(combo0->phy_reg + (0x4300 << 1), 0x00FB);
				phy_writew(combo0->phy_reg + (0x4500 << 1), 0x00FB);
				phy_writew(combo0->phy_reg + (0x4700 << 1), 0x00FB);

				//TX_PSC_A2
				phy_writew(combo0->phy_reg + (0x4102 << 1), 0x04AA);
				phy_writew(combo0->phy_reg + (0x4302 << 1), 0x04AA);
				phy_writew(combo0->phy_reg + (0x4502 << 1), 0x04AA);
				phy_writew(combo0->phy_reg + (0x4702 << 1), 0x04AA);

				//TX_PSC_A3
				phy_writew(combo0->phy_reg + (0x4102 << 1), 0x04AA);
				phy_writew(combo0->phy_reg + (0x4302 << 1), 0x04AA);
				phy_writew(combo0->phy_reg + (0x4502 << 1), 0x04AA);
				phy_writew(combo0->phy_reg + (0x4702 << 1), 0x04AA);

				//RX all set to 0
				phy_writew(combo0->phy_reg + (0x8000 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8200 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8400 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8600 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8000 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8202 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8402 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8602 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8002 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8203 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8403 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8603 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8006 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8206 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8406 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8606 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8108 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8308 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8508 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8708 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8110 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8310 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8510 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8710 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8118 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8318 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8518 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8718 << 1), 0x0);

				//XCVR_DIAG_BIDI_CTRL
				phy_writew(combo0->phy_reg + (0x40EA << 1), 0x000F);
				phy_writew(combo0->phy_reg + (0x42EA << 1), 0x000F);
				phy_writew(combo0->phy_reg + (0x44EA << 1), 0x000F);
				phy_writew(combo0->phy_reg + (0x46EA << 1), 0x000F);

				//TX_TXCC_CTRL
				phy_writew(combo0->phy_reg + (0x4040 << 1), 0x08A4);
				phy_writew(combo0->phy_reg + (0x4240 << 1), 0x08A4);
				phy_writew(combo0->phy_reg + (0x4440 << 1), 0x08A4);
				phy_writew(combo0->phy_reg + (0x4640 << 1), 0x08A4);

				//DRV_DIAG_TX_DRV
				phy_writew(combo0->phy_reg + (0x40C6 << 1), 0x0003);
				phy_writew(combo0->phy_reg + (0x42C6 << 1), 0x0003);
				phy_writew(combo0->phy_reg + (0x44C6 << 1), 0x0003);
				phy_writew(combo0->phy_reg + (0x46C6 << 1), 0x0003);

				phy_writew(combo0->phy_reg + (0x4050 << 1), 0x002A);
				phy_writew(combo0->phy_reg + (0x4250 << 1), 0x002A);
				phy_writew(combo0->phy_reg + (0x4450 << 1), 0x002A);
				phy_writew(combo0->phy_reg + (0x4650 << 1), 0x002A);

				phy_writew(combo0->phy_reg + (0x404C << 1), 0x0000);
				phy_writew(combo0->phy_reg + (0x424C << 1), 0x0000);
				phy_writew(combo0->phy_reg + (0x444C << 1), 0x0000);
				phy_writew(combo0->phy_reg + (0x464C << 1), 0x0000);

				//CMN-pull up-down res cal
				phy_writew(combo0->phy_reg + (0x0103 << 1), 0x007F);
				phy_writew(combo0->phy_reg + (0x010B << 1), 0x007F);

				//ssc
				if (combo0->ssc_en)
					phy_writew(combo0->phy_reg + (0x00D8 << 1), 0x0001);
				else
					phy_writew(combo0->phy_reg + (0x00D8 << 1), 0x0002);
				phy_writew(combo0->phy_reg + (0x00D9 << 1), 0x04C4);
				phy_writew(combo0->phy_reg + (0x00DA << 1), 0x006A);
				phy_writew(combo0->phy_reg + (0x00DB << 1), 0x0004);

				phy_writew(combo0->phy_reg + (0x0098 << 1), 0x0001);
				phy_writew(combo0->phy_reg + (0x00D9 << 1), 0x045f);
				phy_writew(combo0->phy_reg + (0x009A << 1), 0x006b);
				phy_writew(combo0->phy_reg + (0x009B << 1), 0x0004);

				// pll configure
				phy_writew(combo0->phy_reg + (0x00d4 << 1), 0x0004);
				phy_writew(combo0->phy_reg + (0x01C4 << 1), 0x0509);
				phy_writew(combo0->phy_reg + (0x01C5 << 1), 0x0F00);
				phy_writew(combo0->phy_reg + (0x01C6 << 1), 0x0F08);

				phy_writew(combo0->phy_reg + (0x00d0 << 1), 0x019f);
				phy_writew(combo0->phy_reg + (0x00d1 << 1), 0x6276);
				phy_writew(combo0->phy_reg + (0x00d2 << 1), 0x0002);
				phy_writew(combo0->phy_reg + (0x00d3 << 1), 0x0116);
				phy_writew(combo0->phy_reg + (0x01c0 << 1), 0x0002);

				phy_writew(combo0->phy_reg + (0x1c1 << 1), 0x0701);
				phy_writew(combo0->phy_reg + (0x004B << 1), 0x0);

				//dp XCVR(LANE) pll1 seting
				phy_writew(combo0->phy_reg + (0x40e6 << 1), 0x0001);
				phy_writew(combo0->phy_reg + (0x42e6 << 1), 0x0001);
				phy_writew(combo0->phy_reg + (0x44e6 << 1), 0x0001);
				phy_writew(combo0->phy_reg + (0x46e6 << 1), 0x0001);

				phy_writew(combo0->phy_reg + (0x40e7 << 1), 0x0001);
				phy_writew(combo0->phy_reg + (0x42e7 << 1), 0x0001);
				phy_writew(combo0->phy_reg + (0x44e7 << 1), 0x0001);
				phy_writew(combo0->phy_reg + (0x46e7 << 1), 0x0001);

				phy_writew(combo0->phy_reg + (0x40e5 << 1), 0x0019);
				phy_writew(combo0->phy_reg + (0x42e5 << 1), 0x0019);
				phy_writew(combo0->phy_reg + (0x44e5 << 1), 0x0019);
				phy_writew(combo0->phy_reg + (0x46e5 << 1), 0x0019);
				phy_writew(combo0->phy_reg + (0xe006 << 1), 0x2224);

				// pll0 configure
				phy_writew(combo0->phy_reg + (0x1A1 << 1), 0x8600);
				phy_writew(combo0->phy_reg + (0x0094 << 1), 0x0004);
				phy_writew(combo0->phy_reg + (0x01A4 << 1), 0x0509);
				phy_writew(combo0->phy_reg + (0x01A5 << 1), 0x0F00);
				phy_writew(combo0->phy_reg + (0x01A6 << 1), 0x0F08);
				phy_writew(combo0->phy_reg + (0x0090 << 1), 0x0180);
				phy_writew(combo0->phy_reg + (0x0091 << 1), 0x9DBA);
				phy_writew(combo0->phy_reg + (0x0092 << 1), 0x0002);
				phy_writew(combo0->phy_reg + (0x0093 << 1), 0x0102);
				phy_writew(combo0->phy_reg + (0x01A0 << 1), 0x0002);
				break;
			case 3240:
				break;
			case 4320:
				break;
			case 5400:
				val = readl(COMB0_REG_PHYPINS_LINK_CTRL2(combo0->top_reg));
				val &= ~(PHY0_IDDQ_EN_SEL | PHY0_IDDQ_EN_SOFT);
				writel(val, COMB0_REG_PHYPINS_LINK_CTRL2(combo0->top_reg));

				phy_writel(combo0->top_reg + 0x4, 0x1100001);

				//CMN_SSM_BIAS_TMR
				phy_writew(combo0->phy_reg + (0x0022 << 1), 0x001a);
				//CMN_PLLSM0_PLLPRE_TMR
				phy_writew(combo0->phy_reg + (0x002A << 1), 0x0034);
				//CMN_PLLSM0_PLLLOCK_TMR
				phy_writew(combo0->phy_reg + (0x002C << 1), 0x00da);
				//CMN_PLLSM1_PLLPRE_TMR
				phy_writew(combo0->phy_reg + (0x0032 << 1), 0x0034);
				//CMN_PLLSM1_PLLLOCK_TMR
				phy_writew(combo0->phy_reg + (0x0034 << 1), 0x00da);
				//CMN_BGCAL_INIT_TMR
				phy_writew(combo0->phy_reg + (0x0064 << 1), 0x0082);
				//CMN_BGCAL_ITER_TMR
				phy_writew(combo0->phy_reg + (0x0065 << 1), 0x0082);
				//CMN_IBCAL_INIT_TMR
				phy_writew(combo0->phy_reg + (0x0074 << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x0104 << 1), 0x0020);
				phy_writew(combo0->phy_reg + (0x0105 << 1), 0x0007);
				phy_writew(combo0->phy_reg + (0x010c << 1), 0x0020);
				phy_writew(combo0->phy_reg + (0x010d << 1), 0x0007);
				phy_writew(combo0->phy_reg + (0x0114 << 1), 0x030c);
				phy_writew(combo0->phy_reg + (0x0115 << 1), 0x0007);
				phy_writew(combo0->phy_reg + (0x0124 << 1), 0x0007);
				phy_writew(combo0->phy_reg + (0x0125 << 1), 0x0003);
				phy_writew(combo0->phy_reg + (0x0126 << 1), 0x000f);
				phy_writew(combo0->phy_reg + (0x0128 << 1), 0x0132);
				phy_writew(combo0->phy_reg + (0x8044 << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x8244 << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x8444 << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x8644 << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x8045 << 1), 0x0082);
				phy_writew(combo0->phy_reg + (0x8245 << 1), 0x0082);
				phy_writew(combo0->phy_reg + (0x8445 << 1), 0x0082);
				phy_writew(combo0->phy_reg + (0x8645 << 1), 0x0082);
				phy_writew(combo0->phy_reg + (0x804c << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x824c << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x844c << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x864c << 1), 0x001a);
				phy_writew(combo0->phy_reg + (0x804d << 1), 0x0082);
				phy_writew(combo0->phy_reg + (0x824d << 1), 0x0082);
				phy_writew(combo0->phy_reg + (0x844d << 1), 0x0082);
				phy_writew(combo0->phy_reg + (0x864d << 1), 0x0082);
				phy_writew(combo0->phy_reg + (0x4123 << 1), 0x0a28);
				phy_writew(combo0->phy_reg + (0x4323 << 1), 0x0a28);
				phy_writew(combo0->phy_reg + (0x4523 << 1), 0x0a28);
				phy_writew(combo0->phy_reg + (0x4723 << 1), 0x0a28);

				//VCO calibration settings.
				phy_writew(combo0->phy_reg + (0x00c4 << 1), 0x0104);
				phy_writew(combo0->phy_reg + (0x00c5 << 1), 0x0005);
				phy_writew(combo0->phy_reg + (0x00c6 << 1), 0x0337);
				phy_writew(combo0->phy_reg + (0x00c8 << 1), 0x0335);
				phy_writew(combo0->phy_reg + (0x00c2 << 1), 0x0003);
				phy_writew(combo0->phy_reg + (0x0084 << 1), 0x0104);
				phy_writew(combo0->phy_reg + (0x0085 << 1), 0x0005);
				phy_writew(combo0->phy_reg + (0x0086 << 1), 0x0337);
				phy_writew(combo0->phy_reg + (0x0088 << 1), 0x0335);
				phy_writew(combo0->phy_reg + (0x0082 << 1), 0x0003);

				//PLL lock detect settings
				phy_writew(combo0->phy_reg + (0x00dc << 1), 0x00cf);
				phy_writew(combo0->phy_reg + (0x00de << 1), 0x00ce);
				phy_writew(combo0->phy_reg + (0x00dF << 1), 0x0005);
				phy_writew(combo0->phy_reg + (0x009c << 1), 0x00cf);
				phy_writew(combo0->phy_reg + (0x009e << 1), 0x00ce);
				phy_writew(combo0->phy_reg + (0x009F << 1), 0x0005);

				//DP_LANE_SET
				//lane mapping PHY_PMA_LANE_MAP
				phy_writew(combo0->phy_reg + (0xc010 << 1), 0x51d9);
				//phy DP lane enabled
				phy_writew(combo0->phy_reg + (0xc011 << 1), 0x0100);
				//PHY_PMA_ISO_PLL_CTRL0   cmn_pll1_clk_en
				phy_writew(combo0->phy_reg + (0xe003 << 1), 0x0003);
				phy_writew(combo0->phy_reg + (0xe005 << 1), 0x000B);
				//PHY_PMA_ISO_PLL_CTRL1
				phy_writew(combo0->phy_reg + (0xe006 << 1), 0x2224);
				//TX_PSC_A0
				phy_writew(combo0->phy_reg + (0x4100 << 1), 0x00FB);
				phy_writew(combo0->phy_reg + (0x4300 << 1), 0x00FB);
				phy_writew(combo0->phy_reg + (0x4500 << 1), 0x00FB);
				phy_writew(combo0->phy_reg + (0x4700 << 1), 0x00FB);
				phy_writew(combo0->phy_reg + (0x4102 << 1), 0x04AA);
				phy_writew(combo0->phy_reg + (0x4302 << 1), 0x04AA);
				phy_writew(combo0->phy_reg + (0x4502 << 1), 0x04AA);
				phy_writew(combo0->phy_reg + (0x4702 << 1), 0x04AA);
				phy_writew(combo0->phy_reg + (0x4102 << 1), 0x04AA);
				phy_writew(combo0->phy_reg + (0x4302 << 1), 0x04AA);
				phy_writew(combo0->phy_reg + (0x4502 << 1), 0x04AA);
				phy_writew(combo0->phy_reg + (0x4702 << 1), 0x04AA);

				//RX all set to 0
				phy_writew(combo0->phy_reg + (0x8000 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8200 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8400 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8600 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8000 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8202 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8402 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8602 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8002 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8203 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8403 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8603 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8006 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8206 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8406 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8606 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8108 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8308 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8508 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8708 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8110 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8310 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8510 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8710 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8118 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8318 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8518 << 1), 0x0);
				phy_writew(combo0->phy_reg + (0x8718 << 1), 0x0);

				//XCVR_DIAG_BIDI_CTRL
				phy_writew(combo0->phy_reg + (0x40EA << 1), 0x000F);
				phy_writew(combo0->phy_reg + (0x42EA << 1), 0x000F);
				phy_writew(combo0->phy_reg + (0x44EA << 1), 0x000F);
				phy_writew(combo0->phy_reg + (0x46EA << 1), 0x000F);

				//TX_TXCC_CTRL
				phy_writew(combo0->phy_reg + (0x4040 << 1), 0x08A4);
				phy_writew(combo0->phy_reg + (0x4240 << 1), 0x08A4);
				phy_writew(combo0->phy_reg + (0x4440 << 1), 0x08A4);
				phy_writew(combo0->phy_reg + (0x4640 << 1), 0x08A4);

				//DRV_DIAG_TX_DRV
				phy_writew(combo0->phy_reg + (0x40C6 << 1), 0x0003);
				phy_writew(combo0->phy_reg + (0x42C6 << 1), 0x0003);
				phy_writew(combo0->phy_reg + (0x44C6 << 1), 0x0003);
				phy_writew(combo0->phy_reg + (0x46C6 << 1), 0x0003);
				phy_writew(combo0->phy_reg + (0x4050 << 1), 0x002A);
				phy_writew(combo0->phy_reg + (0x4250 << 1), 0x002A);
				phy_writew(combo0->phy_reg + (0x4450 << 1), 0x002A);
				phy_writew(combo0->phy_reg + (0x4650 << 1), 0x002A);
				phy_writew(combo0->phy_reg + (0x404C << 1), 0x0000);
				phy_writew(combo0->phy_reg + (0x424C << 1), 0x0000);
				phy_writew(combo0->phy_reg + (0x444C << 1), 0x0000);
				phy_writew(combo0->phy_reg + (0x464C << 1), 0x0000);

				//CMN-pull-up-down res cal
				phy_writew(combo0->phy_reg + (0x0103 << 1), 0x007F);
				phy_writew(combo0->phy_reg + (0x010B << 1), 0x007F);

				//ssc
				if (combo0->ssc_en)
					phy_writew(combo0->phy_reg + (0x00D8 << 1), 0x0001);
				else
					phy_writew(combo0->phy_reg + (0x00D8 << 1), 0x0002);
				phy_writew(combo0->phy_reg + (0x00D9 << 1), 0x04C4);
				phy_writew(combo0->phy_reg + (0x00DA << 1), 0x006A);
				phy_writew(combo0->phy_reg + (0x00DB << 1), 0x0004);

				phy_writew(combo0->phy_reg + (0x0098 << 1), 0x0001);
				phy_writew(combo0->phy_reg + (0x00D9 << 1), 0x045f);
				phy_writew(combo0->phy_reg + (0x009A << 1), 0x006b);
				phy_writew(combo0->phy_reg + (0x009B << 1), 0x0004);

				// pll configure
				phy_writew(combo0->phy_reg + (0x00d4 << 1), 0x0004);
				phy_writew(combo0->phy_reg + (0x01C4 << 1), 0x0509);
				phy_writew(combo0->phy_reg + (0x01C5 << 1), 0x0F00);
				phy_writew(combo0->phy_reg + (0x01C6 << 1), 0x0F08);

				phy_writew(combo0->phy_reg + (0x00d0 << 1), 0x019f);
				phy_writew(combo0->phy_reg + (0x00d1 << 1), 0x6276);
				phy_writew(combo0->phy_reg + (0x00d2 << 1), 0x0002);
				phy_writew(combo0->phy_reg + (0x00d3 << 1), 0x0116);
				phy_writew(combo0->phy_reg + (0x01c0 << 1), 0x0002);

				//SET common  PLL1_CLOCK0
				phy_writew(combo0->phy_reg + (0x1c1 << 1), 0x0601);
				phy_writew(combo0->phy_reg + (0x004B << 1), 0x0);

				//dp XCVR(LANE) pll1 seting
				phy_writew(combo0->phy_reg + (0x40e6 << 1), 0x0001);
				phy_writew(combo0->phy_reg + (0x42e6 << 1), 0x0001);
				phy_writew(combo0->phy_reg + (0x44e6 << 1), 0x0001);
				phy_writew(combo0->phy_reg + (0x46e6 << 1), 0x0001);

				phy_writew(combo0->phy_reg + (0x40e7 << 1), 0x0000);
				phy_writew(combo0->phy_reg + (0x42e7 << 1), 0x0000);
				phy_writew(combo0->phy_reg + (0x44e7 << 1), 0x0000);
				phy_writew(combo0->phy_reg + (0x46e7 << 1), 0x0000);

				phy_writew(combo0->phy_reg + (0x40e5 << 1), 0x0019);
				phy_writew(combo0->phy_reg + (0x42e5 << 1), 0x0019);
				phy_writew(combo0->phy_reg + (0x44e5 << 1), 0x0019);
				phy_writew(combo0->phy_reg + (0x46e5 << 1), 0x0019);
				phy_writew(combo0->phy_reg + (0xe006 << 1), 0x2224);

				phy_writew(combo0->phy_reg + (0x1A1 << 1), 0x8600);
				phy_writew(combo0->phy_reg + (0x0094 << 1), 0x0004);
				phy_writew(combo0->phy_reg + (0x01A4 << 1), 0x0509);
				phy_writew(combo0->phy_reg + (0x01A5 << 1), 0x0F00);
				phy_writew(combo0->phy_reg + (0x01A6 << 1), 0x0F08);
				phy_writew(combo0->phy_reg + (0x0090 << 1), 0x0180);
				phy_writew(combo0->phy_reg + (0x0091 << 1), 0x9DBA);
				phy_writew(combo0->phy_reg + (0x0092 << 1), 0x0002);
				phy_writew(combo0->phy_reg + (0x0093 << 1), 0x0102);
				phy_writew(combo0->phy_reg + (0x01A0 << 1), 0x0002);
				break;
			case 8100:
				break;
			default:
				pr_err("[sunxi cadence phy]: not support rate(%d Mb/s) for dp\n", dp_config->link_rate);
				return -1;
			}

			combo0_dp_phy_reset(combo0, true);
			combo0->link_rate = dp_config->link_rate;
		}

	}

	return 0;
}

static int combo0_dp_set_lanes(struct sunxi_cadence_combophy *combo0,
			       struct phy_configure_opts_dp *dp_config)
{

	if (dp_config->set_lanes == 1) {
		/* usb plugout would clear congiguration, we should re-config it */
		if (!COMBO_PHY_TYPE(combo0->mode, PHY_TYPE_USB3)) {
			combo0_dp_phy_reset(combo0, false);
			combo0_configure_usb_dp(combo0);
			combo0_dp_phy_reset(combo0, true);
		}

		/* usb_dp_state need enable all lanes */
		if (combo0->usb_dp_state) {
				phy_writew(combo0->phy_reg + (0x4011 << 1), 0x0100);
		} else {
			switch (dp_config->lanes) {
			case 1:
				// close lane 321
				phy_writew(combo0->phy_reg + (0x4011 << 1), 0x010E);
				break;
			case 2:
				// close lane 32
				phy_writew(combo0->phy_reg + (0x4011 << 1), 0x010C);
				break;
			case 4:
				phy_writew(combo0->phy_reg + (0x4011 << 1), 0x0100);
				break;
			default:
				pr_err("[sunxi cadence phy]: not support lanes(%d) for dp\n", dp_config->lanes);
				return -1;;
			}
		}
	}

	return 0;
}

static int combo0_dp_set_sw_pre(struct sunxi_cadence_combophy *combo0,
			       struct phy_configure_opts_dp *dp_config)
{
	int i = 0;
	u16 lane_txdrv = 0;
	u16 lane_mgnfs = 0;
	u16 lane_cpost = 0;
	unsigned int sw = 0;
	unsigned int pre = 0;
	u32 offset;

	if (dp_config->set_voltages == 1) {
		/* configure lane before sw-pre */
		for (i = 0; i < dp_config->lanes; i++) {
			if (combo0->usb_dp_state) {
				if (combo0->orientation == TYPEC_ORIENTATION_REVERSE)
					offset = combo0->typec_remap[MAX_LANE_NO - i] * 0x200;
				else
					offset = combo0->typec_remap[i] * 0x200;
				phy_writew(combo0->phy_reg + ((0x41E7 + offset) << 1), 0x0001);
			} else {
				offset = i * 0x200;
				phy_writew(combo0->phy_reg + ((0x41E7 + offset) << 1), 0x0001);
			}
		}

		udelay(10);

		for (i = 0; i < dp_config->lanes; i++) {
			if (combo0->usb_dp_state) {
				if (combo0->orientation == TYPEC_ORIENTATION_REVERSE)
					offset = combo0->typec_remap[MAX_LANE_NO - i] * 0x200;
				else
					offset = combo0->typec_remap[i] * 0x200;
				phy_writew(combo0->phy_reg + ((0x4040 + offset) << 1), 0x08A4);
			} else {
				offset = i * 0x200;
				phy_writew(combo0->phy_reg + ((0x4040 + offset) << 1), 0x08A4);
			}
		}
		// end of configure

		for (i = 0; i < dp_config->lanes; i++) {
			sw = dp_config->voltage[i];
			pre = dp_config->pre[i];

			switch (combo0->link_rate) {
			case 5400:
				lane_txdrv = dp_lv_table_5g4[sw][pre].lane_txdrv[i];
				lane_mgnfs = dp_lv_table_5g4[sw][pre].lane_mgnfs[i];
				lane_cpost = dp_lv_table_5g4[sw][pre].lane_cpost[i];
				break;
			case 2700:
				lane_txdrv = dp_lv_table_2g7[sw][pre].lane_txdrv[i];
				lane_mgnfs = dp_lv_table_2g7[sw][pre].lane_mgnfs[i];
				lane_cpost = dp_lv_table_2g7[sw][pre].lane_cpost[i];
				break;
			case 1620:
				if (combo0->usb_dp_state) {
					pr_err("[sunxi cadence phy]: not support rate(%d Mb/s) for dp signal level\n",
					       dp_config->link_rate);
					return -1;
				}
				lane_txdrv = dp_lv_table_1g62[sw][pre].lane_txdrv[i];
				lane_mgnfs = dp_lv_table_1g62[sw][pre].lane_mgnfs[i];
				lane_cpost = dp_lv_table_1g62[sw][pre].lane_cpost[i];
				break;
			default:
				pr_err("[sunxi cadence phy]: not support rate(%d Mb/s) for dp signal level\n",
				       dp_config->link_rate);
				return -1;
			}

			if (combo0->usb_dp_state) {
				/* FIXME:
				 * this usp_dp param only works if dp->typec connection use
				 * standard hardware design in 733 evb/P50AI */
				if (combo0->orientation == TYPEC_ORIENTATION_REVERSE) {
					offset = combo0->typec_remap[MAX_LANE_NO - i] * 0x400;
					phy_writew(combo0->phy_reg + (0x818C + offset), lane_txdrv);
					phy_writew(combo0->phy_reg + (0x80A0 + offset), lane_mgnfs);
					phy_writew(combo0->phy_reg + (0x8098 + offset), lane_cpost);
				} else {
					offset = combo0->typec_remap[i] * 0x400;
					phy_writew(combo0->phy_reg + (0x818C + offset), lane_txdrv);
					phy_writew(combo0->phy_reg + (0x80A0 + offset), lane_mgnfs);
					phy_writew(combo0->phy_reg + (0x8098 + offset), lane_cpost);
				}
			} else {
				phy_writew(combo0->phy_reg + (0x818C + (i * 0x400)), lane_txdrv);
				phy_writew(combo0->phy_reg + (0x80A0 + (i * 0x400)), lane_mgnfs);
				phy_writew(combo0->phy_reg + (0x8098 + (i * 0x400)), lane_cpost);
			}

		}

		/* configure after sw-pre */
		udelay(10);

		for (i = 0; i < dp_config->lanes; i++) {
			if (combo0->usb_dp_state) {
				if (combo0->orientation == TYPEC_ORIENTATION_REVERSE)
					offset = combo0->typec_remap[MAX_LANE_NO - i] * 0x200;
				else
					offset = combo0->typec_remap[i] * 0x200;
				phy_writew(combo0->phy_reg + ((0x41E7 + offset) << 1), 0x0000);
			} else {
				offset = i * 0x200;
				phy_writew(combo0->phy_reg + ((0x41E7 + offset) << 1), 0x0000);
			}
		}
	}

	return 0;
}

static int combo0_dp_set_ssc(struct sunxi_cadence_combophy *combo0,
			       struct phy_configure_opts_dp *dp_config)
{
	/* enable ssc if need */
	if (dp_config->ssc == 1)
		combo0->ssc_en = 1;

	return 0;
}

static void combo0_dp_phy_init(struct sunxi_cadence_combophy *combo0)
{
	struct sunxi_cadence_phy *sunxi_cphy = combo0->sunxi_cphy;
	int ret;

	if (sunxi_cphy->avdd_h_regulator)
		ret = regulator_enable(sunxi_cphy->avdd_h_regulator);

	if (sunxi_cphy->avdd_d_regulator)
		ret = regulator_enable(sunxi_cphy->avdd_d_regulator);

	if (sunxi_cphy->avdd_c_regulator)
		ret = regulator_enable(sunxi_cphy->avdd_c_regulator);

	if (sunxi_cphy->avdd_aux_regulator)
		ret = regulator_enable(sunxi_cphy->avdd_aux_regulator);

	if (sunxi_cphy->dcxo_serdes0_clk)
		clk_prepare_enable(sunxi_cphy->dcxo_serdes0_clk);

	if (sunxi_cphy->serdes_clk)
		clk_prepare_enable(sunxi_cphy->serdes_clk);

	if (sunxi_cphy->serdes_reset)
		reset_control_deassert(sunxi_cphy->serdes_reset);

	if (combo0->clk)
		clk_prepare_enable(combo0->clk);
}

static void combo0_dp_phy_exit(struct sunxi_cadence_combophy *combo0)
{
	struct sunxi_cadence_phy *sunxi_cphy = combo0->sunxi_cphy;

	if (combo0->clk)
		clk_disable_unprepare(combo0->clk);

	if (sunxi_cphy->serdes_reset)
		reset_control_assert(sunxi_cphy->serdes_reset);

	if (sunxi_cphy->serdes_clk)
		clk_disable_unprepare(sunxi_cphy->serdes_clk);

	if (sunxi_cphy->dcxo_serdes0_clk)
		clk_disable_unprepare(sunxi_cphy->dcxo_serdes0_clk);

	if (sunxi_cphy->avdd_h_regulator)
		regulator_disable(sunxi_cphy->avdd_h_regulator);

	if (sunxi_cphy->avdd_d_regulator)
		regulator_disable(sunxi_cphy->avdd_d_regulator);

	if (sunxi_cphy->avdd_c_regulator)
		regulator_disable(sunxi_cphy->avdd_c_regulator);

	if (sunxi_cphy->avdd_aux_regulator)
		regulator_disable(sunxi_cphy->avdd_aux_regulator);

}

static void combo0_dp_lane_remap(struct sunxi_cadence_combophy *combo0,
				 enum typec_orientation orientation)
{
	struct sunxi_cadence_phy *sunxi_cphy = combo0->sunxi_cphy;
	int val = 0;

	//set phy's lane remap
	if (orientation != TYPEC_ORIENTATION_REVERSE) {
		val |= combo0->lane_remap[0] << 0;
		val |= combo0->lane_remap[1] << 4;
		val |= combo0->lane_remap[2] << 8;
		val |= combo0->lane_remap[3] << 12;
	} else {
		val |= combo0->lane_remap[3] << 0;
		val |= combo0->lane_remap[2] << 4;
		val |= combo0->lane_remap[1] << 8;
		val |= combo0->lane_remap[0] << 12;
	}
	phy_writel(sunxi_cphy->top_combo_reg + 0xc24, val);
}

static void combo0_dp_lane_invert(struct sunxi_cadence_combophy *combo0)
{
	int val = 0, i = 0;

	for (i = 0; i < MAX_LANE_CNT; i++) {
		val = phy_readw(combo0->phy_reg + ((0xF000 + (i * 0x100)) << 1));
		val &= ~(1 << 8);
		val |= combo0->lane_invert[i] ? (1 << 8) : (0 << 8);
		phy_writew(combo0->phy_reg + ((0xF000 + (i * 0x100)) << 1), val);
	}
}

static void combo0_dp_phy_power_on(struct sunxi_cadence_combophy *combo0)
{
	// set lane remap
	combo0_dp_lane_remap(combo0, TYPEC_ORIENTATION_NONE);

	// set lane invert
	combo0_dp_lane_invert(combo0);
}

static void combo0_dp_phy_power_off(struct sunxi_cadence_combophy *combo0)
{
}

static int combo0_configure_usb_dp(struct sunxi_cadence_combophy *combo0)
{
	u32 val = 0;

	pr_debug("==================== orientation: %s USB configured: %s ====================",
		 typec_orientation_name[combo0->orientation], COMBO_PHY_TYPE(combo0->mode, PHY_TYPE_USB3) ? "yes" : "no");

	val = readl(COMB0_REG_PHYPINS_LINK_CTRL2(combo0->top_reg));
	val &= ~(PHY0_IDDQ_EN_SEL | PHY0_IDDQ_EN_SOFT);
	writel(val, COMB0_REG_PHYPINS_LINK_CTRL2(combo0->top_reg));

	phy_writew(combo0->phy_reg + (0xE005 << 1), 0x0003);
	phy_writew(combo0->phy_reg + (0xE006 << 1), 0x1124);
	if (combo0->orientation == TYPEC_ORIENTATION_REVERSE) {
		/* Restore DP Normal orientation register configuration */
		phy_writew(combo0->phy_reg + (0x4500 << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x4700 << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x4502 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x4702 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x4503 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x4703 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x8400 << 1), 0x091D);
		phy_writew(combo0->phy_reg + (0x8600 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8402 << 1), 0x0900);
		phy_writew(combo0->phy_reg + (0x8602 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8403 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8603 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8406 << 1), 0x010F);
		phy_writew(combo0->phy_reg + (0x8606 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x44EA << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x46EA << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x8508 << 1), 0x0009);
		phy_writew(combo0->phy_reg + (0x8708 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8510 << 1), 0x0009);
		phy_writew(combo0->phy_reg + (0x8710 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8518 << 1), 0x000D);
		phy_writew(combo0->phy_reg + (0x8718 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x4440 << 1), 0x2A84);
		phy_writew(combo0->phy_reg + (0x4640 << 1), 0x2A84);
		phy_writew(combo0->phy_reg + (0x44C6 << 1), 0x00A3);
		phy_writew(combo0->phy_reg + (0x46C6 << 1), 0x00A3);
		phy_writew(combo0->phy_reg + (0x4450 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x4650 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x444C << 1), 0x001C);
		phy_writew(combo0->phy_reg + (0x464C << 1), 0x001C);
	} else {
		/* Restore DP Reverse orientation register configuration */
		phy_writew(combo0->phy_reg + (0x4100 << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x4300 << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x4102 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x4302 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x4103 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x4303 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x8000 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8200 << 1), 0x091D);
		phy_writew(combo0->phy_reg + (0x8002 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8202 << 1), 0x0900);
		phy_writew(combo0->phy_reg + (0x8003 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8203 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8006 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8206 << 1), 0x010F);
		phy_writew(combo0->phy_reg + (0x40EA << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x42EA << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x8108 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8308 << 1), 0x0009);
		phy_writew(combo0->phy_reg + (0x8110 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8310 << 1), 0x0009);
		phy_writew(combo0->phy_reg + (0x8118 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8318 << 1), 0x000D);
		phy_writew(combo0->phy_reg + (0x4040 << 1), 0x2A84);
		phy_writew(combo0->phy_reg + (0x4240 << 1), 0x2A84);
		phy_writew(combo0->phy_reg + (0x40C6 << 1), 0x00A3);
		phy_writew(combo0->phy_reg + (0x42C6 << 1), 0x00A3);
		phy_writew(combo0->phy_reg + (0x4050 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x4250 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x404C << 1), 0x001C);
		phy_writew(combo0->phy_reg + (0x424C << 1), 0x001C);
	}

	phy_writel(combo0->top_reg + 0x4, 0x01100001);
	phy_writew(combo0->phy_reg + (0x0022 << 1), 0x001A);
	phy_writew(combo0->phy_reg + (0x002A << 1), 0x0034);
	phy_writew(combo0->phy_reg + (0x002C << 1), 0x00DA);
	phy_writew(combo0->phy_reg + (0x0032 << 1), 0x0034);
	phy_writew(combo0->phy_reg + (0x0034 << 1), 0x00DA);
	phy_writew(combo0->phy_reg + (0x0064 << 1), 0x0082);
	phy_writew(combo0->phy_reg + (0x0065 << 1), 0x0082);
	phy_writew(combo0->phy_reg + (0x0074 << 1), 0x001A);
	phy_writew(combo0->phy_reg + (0x0104 << 1), 0x0020);
	phy_writew(combo0->phy_reg + (0x0105 << 1), 0x0007);
	phy_writew(combo0->phy_reg + (0x010C << 1), 0x0020);
	phy_writew(combo0->phy_reg + (0x010D << 1), 0x0007);
	phy_writew(combo0->phy_reg + (0x0114 << 1), 0x030C);
	phy_writew(combo0->phy_reg + (0x0115 << 1), 0x0007);
	phy_writew(combo0->phy_reg + (0x0124 << 1), 0x0007);
	phy_writew(combo0->phy_reg + (0x0125 << 1), 0x0003);
	phy_writew(combo0->phy_reg + (0x0126 << 1), 0x000F);
	phy_writew(combo0->phy_reg + (0x0128 << 1), 0x0132);
	phy_writew(combo0->phy_reg + (0x8044 << 1), 0x001A);
	phy_writew(combo0->phy_reg + (0x8244 << 1), 0x001A);
	phy_writew(combo0->phy_reg + (0x8444 << 1), 0x001A);
	phy_writew(combo0->phy_reg + (0x8644 << 1), 0x001A);
	phy_writew(combo0->phy_reg + (0x8045 << 1), 0x0082);
	phy_writew(combo0->phy_reg + (0x8245 << 1), 0x0082);
	phy_writew(combo0->phy_reg + (0x8445 << 1), 0x0082);
	phy_writew(combo0->phy_reg + (0x8645 << 1), 0x0082);
	phy_writew(combo0->phy_reg + (0x804C << 1), 0x001A);
	phy_writew(combo0->phy_reg + (0x824C << 1), 0x001A);
	phy_writew(combo0->phy_reg + (0x844C << 1), 0x001A);
	phy_writew(combo0->phy_reg + (0x864C << 1), 0x001A);
	phy_writew(combo0->phy_reg + (0x804D << 1), 0x0082);
	phy_writew(combo0->phy_reg + (0x824D << 1), 0x0082);
	phy_writew(combo0->phy_reg + (0x844D << 1), 0x0082);
	phy_writew(combo0->phy_reg + (0x864D << 1), 0x0082);
	phy_writew(combo0->phy_reg + (0x4123 << 1), 0x0A28);
	phy_writew(combo0->phy_reg + (0x4323 << 1), 0x0A28);
	phy_writew(combo0->phy_reg + (0x4523 << 1), 0x0A28);
	phy_writew(combo0->phy_reg + (0x4723 << 1), 0x0A28);
	phy_writew(combo0->phy_reg + (0x01A1 << 1), 0x8600);
	phy_writew(combo0->phy_reg + (0x01C1 << 1), 0x0701);

	if (combo0->orientation == TYPEC_ORIENTATION_REVERSE) {
		/* Restore USB Normal orientation register configuration */
		phy_writew(combo0->phy_reg + (0x40E5 << 1), 0x0012);
		phy_writew(combo0->phy_reg + (0x42E5 << 1), 0x0012);
		phy_writew(combo0->phy_reg + (0x44E6 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x46E6 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x44E7 << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x46E7 << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x44E5 << 1), 0x0012);
		phy_writew(combo0->phy_reg + (0x46E5 << 1), 0x0012);

		phy_writew(combo0->phy_reg + (0x44E5 << 1), 0x0041);
		phy_writew(combo0->phy_reg + (0x46E5 << 1), 0x0041);
		phy_writew(combo0->phy_reg + (0x40E6 << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x42E6 << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x40E7 << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x42E7 << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x40E5 << 1), 0x0019);
		phy_writew(combo0->phy_reg + (0x42E5 << 1), 0x0019);
	} else {
		/* Restore USB Reverse orientation register configuration */
		phy_writew(combo0->phy_reg + (0x44E5 << 1), 0x0012);
		phy_writew(combo0->phy_reg + (0x46E5 << 1), 0x0012);
		phy_writew(combo0->phy_reg + (0x40E6 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x42E6 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x40E7 << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x42E7 << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x40E5 << 1), 0x0012);
		phy_writew(combo0->phy_reg + (0x42E5 << 1), 0x0012);

		phy_writew(combo0->phy_reg + (0x40E5 << 1), 0x0041);
		phy_writew(combo0->phy_reg + (0x42E5 << 1), 0x0041);
		phy_writew(combo0->phy_reg + (0x44E6 << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x46E6 << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x44E7 << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x46E7 << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x44E5 << 1), 0x0019);
		phy_writew(combo0->phy_reg + (0x46E5 << 1), 0x0019);
	}

	phy_writew(combo0->phy_reg + (0x0094 << 1), 0x0004);
	phy_writew(combo0->phy_reg + (0x00D4 << 1), 0x0004);
	phy_writew(combo0->phy_reg + (0x01A4 << 1), 0x0509);
	phy_writew(combo0->phy_reg + (0x01C4 << 1), 0x0509);
	phy_writew(combo0->phy_reg + (0x01A5 << 1), 0x0F00);
	phy_writew(combo0->phy_reg + (0x01C5 << 1), 0x0F00);
	phy_writew(combo0->phy_reg + (0x01A6 << 1), 0x0F08);
	phy_writew(combo0->phy_reg + (0x01C6 << 1), 0x0F08);
	phy_writew(combo0->phy_reg + (0x0090 << 1), 0x0180);
	phy_writew(combo0->phy_reg + (0x00D0 << 1), 0x019F);
	phy_writew(combo0->phy_reg + (0x0091 << 1), 0x9D8A);
	phy_writew(combo0->phy_reg + (0x00D1 << 1), 0x6276);
	phy_writew(combo0->phy_reg + (0x0092 << 1), 0x0002);
	phy_writew(combo0->phy_reg + (0x00D2 << 1), 0x0002);
	phy_writew(combo0->phy_reg + (0x0093 << 1), 0x0102);
	phy_writew(combo0->phy_reg + (0x00D3 << 1), 0x0116);
	phy_writew(combo0->phy_reg + (0x01A0 << 1), 0x0002);
	phy_writew(combo0->phy_reg + (0x01C0 << 1), 0x0002);
	phy_writew(combo0->phy_reg + (0x0098 << 1), 0x0001);
	if (combo0->ssc_en)
		phy_writew(combo0->phy_reg + (0x00D8 << 1), 0x0001);
	else
		phy_writew(combo0->phy_reg + (0x00D8 << 1), 0x0002);
	phy_writew(combo0->phy_reg + (0x0099 << 1), 0x045F);
	phy_writew(combo0->phy_reg + (0x00D9 << 1), 0x04C4);
	phy_writew(combo0->phy_reg + (0x009A << 1), 0x006B);
	phy_writew(combo0->phy_reg + (0x00DA << 1), 0x006A);
	phy_writew(combo0->phy_reg + (0x009B << 1), 0x0004);
	phy_writew(combo0->phy_reg + (0x00DB << 1), 0x0004);
	phy_writew(combo0->phy_reg + (0x0084 << 1), 0x0104);
	phy_writew(combo0->phy_reg + (0x00C4 << 1), 0x0104);
	phy_writew(combo0->phy_reg + (0x0085 << 1), 0x0005);
	phy_writew(combo0->phy_reg + (0x00C5 << 1), 0x0005);
	phy_writew(combo0->phy_reg + (0x0086 << 1), 0x0337);
	phy_writew(combo0->phy_reg + (0x00C6 << 1), 0x0337);
	phy_writew(combo0->phy_reg + (0x0088 << 1), 0x0335);
	phy_writew(combo0->phy_reg + (0x00C8 << 1), 0x0335);
	phy_writew(combo0->phy_reg + (0x0082 << 1), 0x0003);
	phy_writew(combo0->phy_reg + (0x00C2 << 1), 0x0003);
	phy_writew(combo0->phy_reg + (0x009C << 1), 0x00CF);
	phy_writew(combo0->phy_reg + (0x00DC << 1), 0x00CF);
	phy_writew(combo0->phy_reg + (0x009E << 1), 0x00CE);
	phy_writew(combo0->phy_reg + (0x00DE << 1), 0x00CE);
	phy_writew(combo0->phy_reg + (0x009F << 1), 0x0005);
	phy_writew(combo0->phy_reg + (0x00DF << 1), 0x0005);

	val = readl(COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));
	if (combo0->orientation == TYPEC_ORIENTATION_REVERSE)
		val |= PHY0_TYPEC_CONN_DIR;
	else
		val &= ~PHY0_TYPEC_CONN_DIR;
	writel(val, COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));

	phy_writew(combo0->phy_reg + (0xC010 << 1), 0x5100);
	phy_writew(combo0->phy_reg + (0xC011 << 1), 0x0100);
	phy_writew(combo0->phy_reg + (0xC018 << 1), 0x0A0A);
	phy_writew(combo0->phy_reg + (0xC01A << 1), 0x1000);
	phy_writew(combo0->phy_reg + (0xC01B << 1), 0x0010);
	phy_writew(combo0->phy_reg + (0x0041 << 1), 0x8200);
	phy_writew(combo0->phy_reg + (0x0047 << 1), 0x8200);

	if (combo0->orientation == TYPEC_ORIENTATION_REVERSE) {
		/* Restore USB Normal orientation register configuration */
		phy_writew(combo0->phy_reg + (0x4100 << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x4101 << 1), 0x04AF);
		phy_writew(combo0->phy_reg + (0x4102 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x4103 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x8200 << 1), 0x091D);
		phy_writew(combo0->phy_reg + (0x8201 << 1), 0x091D);
		phy_writew(combo0->phy_reg + (0x8202 << 1), 0x0900);
		phy_writew(combo0->phy_reg + (0x8203 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x4040 << 1), 0x2A84);
		phy_writew(combo0->phy_reg + (0x404D << 1), 0x0011);
		phy_writew(combo0->phy_reg + (0x8290 << 1), 0x000C);
		phy_writew(combo0->phy_reg + (0x8308 << 1), 0x0009);
		phy_writew(combo0->phy_reg + (0x8349 << 1), 0x0c02);
		phy_writew(combo0->phy_reg + (0x8377 << 1), 0x06F6);
		phy_writew(combo0->phy_reg + (0x8378 << 1), 0x4606);
		phy_writew(combo0->phy_reg + (0x42EB << 1), 0x0006);
		phy_writew(combo0->phy_reg + (0x8371 << 1), 0x0519);
		phy_writew(combo0->phy_reg + (0x8372 << 1), 0x0519);
		phy_writew(combo0->phy_reg + (0x83E8 << 1), 0x1002);
		phy_writew(combo0->phy_reg + (0x83E5 << 1), 0x0B98);
		phy_writew(combo0->phy_reg + (0x83E2 << 1), 0x0C01);
		phy_writew(combo0->phy_reg + (0x83E3 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x83F5 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x83F4 << 1), 0x0311);
		phy_writew(combo0->phy_reg + (0x83FF << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x81FF << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8280 << 1), 0x010a);
		phy_writew(combo0->phy_reg + (0x8282 << 1), 0x0003);
		phy_writew(combo0->phy_reg + (0x40EA << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x42EA << 1), 0x00FF);

		phy_writew(combo0->phy_reg + (0x4700 << 1), 0x02FF);
		phy_writew(combo0->phy_reg + (0x4701 << 1), 0x06AF);
		phy_writew(combo0->phy_reg + (0x4702 << 1), 0x06AE);
		phy_writew(combo0->phy_reg + (0x4703 << 1), 0x06AE);
		phy_writew(combo0->phy_reg + (0x8400 << 1), 0x0D1D);
		phy_writew(combo0->phy_reg + (0x8401 << 1), 0x0D1D);
		phy_writew(combo0->phy_reg + (0x8402 << 1), 0x0D00);
		phy_writew(combo0->phy_reg + (0x8403 << 1), 0x0500);
		phy_writew(combo0->phy_reg + (0x4640 << 1), 0x2A82);
		phy_writew(combo0->phy_reg + (0x464D << 1), 0x0014);
		phy_writew(combo0->phy_reg + (0x8490 << 1), 0x0013);
		phy_writew(combo0->phy_reg + (0x8508 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8549 << 1), 0x0C02);
		phy_writew(combo0->phy_reg + (0x8577 << 1), 0x0330);
		phy_writew(combo0->phy_reg + (0x8578 << 1), 0x0300);
		phy_writew(combo0->phy_reg + (0x44EB << 1), 0x0003);
		phy_writew(combo0->phy_reg + (0x8571 << 1), 0x0019);
		phy_writew(combo0->phy_reg + (0x8572 << 1), 0x0019);
		phy_writew(combo0->phy_reg + (0x85E8 << 1), 0x1004);
		phy_writew(combo0->phy_reg + (0x85E5 << 1), 0x00F9);
		phy_writew(combo0->phy_reg + (0x85E2 << 1), 0x0C01);
		phy_writew(combo0->phy_reg + (0x85E3 << 1), 0x0002);
		phy_writew(combo0->phy_reg + (0x85F5 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x85F4 << 1), 0x0031);
		phy_writew(combo0->phy_reg + (0x85FF << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x87FF << 1), 0x0002);
		phy_writew(combo0->phy_reg + (0x8480 << 1), 0x018C);
		phy_writew(combo0->phy_reg + (0x8482 << 1), 0x0003);
		phy_writew(combo0->phy_reg + (0x46EA << 1), 0x000F);
		phy_writew(combo0->phy_reg + (0x44EA << 1), 0x00F0);
	} else {
		/* Restore USB Reverse orientation register configuration */
		phy_writew(combo0->phy_reg + (0x4700 << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x4701 << 1), 0x04AF);
		phy_writew(combo0->phy_reg + (0x4702 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x4703 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x8400 << 1), 0x091D);
		phy_writew(combo0->phy_reg + (0x8401 << 1), 0x091D);
		phy_writew(combo0->phy_reg + (0x8402 << 1), 0x0900);
		phy_writew(combo0->phy_reg + (0x8403 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x4640 << 1), 0x2A84);
		phy_writew(combo0->phy_reg + (0x464D << 1), 0x0011);
		phy_writew(combo0->phy_reg + (0x8490 << 1), 0x000C);
		phy_writew(combo0->phy_reg + (0x8508 << 1), 0x0009);
		phy_writew(combo0->phy_reg + (0x8549 << 1), 0x0C02);
		phy_writew(combo0->phy_reg + (0x8577 << 1), 0x06F6);
		phy_writew(combo0->phy_reg + (0x8578 << 1), 0x4606);
		phy_writew(combo0->phy_reg + (0x44EB << 1), 0x0006);
		phy_writew(combo0->phy_reg + (0x8571 << 1), 0x0519);
		phy_writew(combo0->phy_reg + (0x8572 << 1), 0x0519);
		phy_writew(combo0->phy_reg + (0x85E8 << 1), 0x1002);
		phy_writew(combo0->phy_reg + (0x85E5 << 1), 0x0B98);
		phy_writew(combo0->phy_reg + (0x85E2 << 1), 0x0C01);
		phy_writew(combo0->phy_reg + (0x85E3 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x85F5 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x85F4 << 1), 0x0311);
		phy_writew(combo0->phy_reg + (0x85FF << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x87FF << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8480 << 1), 0x010A);
		phy_writew(combo0->phy_reg + (0x8482 << 1), 0x0003);
		phy_writew(combo0->phy_reg + (0x46EA << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x44EA << 1), 0x00FF);

		phy_writew(combo0->phy_reg + (0x4100 << 1), 0x02FF);
		phy_writew(combo0->phy_reg + (0x4101 << 1), 0x06AF);
		phy_writew(combo0->phy_reg + (0x4102 << 1), 0x06AE);
		phy_writew(combo0->phy_reg + (0x4103 << 1), 0x06AE);
		phy_writew(combo0->phy_reg + (0x8200 << 1), 0x0D1D);
		phy_writew(combo0->phy_reg + (0x8201 << 1), 0x0D1D);
		phy_writew(combo0->phy_reg + (0x8202 << 1), 0x0D00);
		phy_writew(combo0->phy_reg + (0x8203 << 1), 0x0500);
		phy_writew(combo0->phy_reg + (0x4040 << 1), 0x2A82);
		phy_writew(combo0->phy_reg + (0x404D << 1), 0x0014);
		phy_writew(combo0->phy_reg + (0x8290 << 1), 0x0013);
		phy_writew(combo0->phy_reg + (0x8308 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8349 << 1), 0x0C02);
		phy_writew(combo0->phy_reg + (0x8377 << 1), 0x0330);
		phy_writew(combo0->phy_reg + (0x8378 << 1), 0x0300);
		phy_writew(combo0->phy_reg + (0x42EB << 1), 0x0003);
		phy_writew(combo0->phy_reg + (0x8371 << 1), 0x0019);
		phy_writew(combo0->phy_reg + (0x8372 << 1), 0x0019);
		phy_writew(combo0->phy_reg + (0x83E8 << 1), 0x1004);
		phy_writew(combo0->phy_reg + (0x83E5 << 1), 0x00F9);
		phy_writew(combo0->phy_reg + (0x83E2 << 1), 0x0C01);
		phy_writew(combo0->phy_reg + (0x83E3 << 1), 0x0002);
		phy_writew(combo0->phy_reg + (0x83F5 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x83F4 << 1), 0x0031);
		phy_writew(combo0->phy_reg + (0x83FF << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x81FF << 1), 0x0002);
		phy_writew(combo0->phy_reg + (0x8280 << 1), 0x018C);
		phy_writew(combo0->phy_reg + (0x8282 << 1), 0x0003);
		phy_writew(combo0->phy_reg + (0x40EA << 1), 0x000F);
		phy_writew(combo0->phy_reg + (0x42EA << 1), 0x00F0);
	}

	phy_writew(combo0->phy_reg + (0xE005 << 1), 0x000B);
	phy_writew(combo0->phy_reg + (0xE006 << 1), 0x2224);
	if (combo0->orientation == TYPEC_ORIENTATION_REVERSE) {
		phy_writew(combo0->phy_reg + (0x4100 << 1), 0x00FB);
		phy_writew(combo0->phy_reg + (0x4300 << 1), 0x00FB);
		phy_writew(combo0->phy_reg + (0x4102 << 1), 0x04AA);
		phy_writew(combo0->phy_reg + (0x4302 << 1), 0x04AA);
		phy_writew(combo0->phy_reg + (0x4103 << 1), 0x04AA);
		phy_writew(combo0->phy_reg + (0x4303 << 1), 0x04AA);
		phy_writew(combo0->phy_reg + (0x8000 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8200 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8002 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8202 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8003 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8203 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8006 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8206 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x40EA << 1), 0x000F);
		phy_writew(combo0->phy_reg + (0x42EA << 1), 0x000F);
		phy_writew(combo0->phy_reg + (0x8108 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8308 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8110 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8310 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8118 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8318 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x4040 << 1), 0x08A4);
		phy_writew(combo0->phy_reg + (0x4240 << 1), 0x08A4);
		phy_writew(combo0->phy_reg + (0x40C6 << 1), 0x0003);
		phy_writew(combo0->phy_reg + (0x42C6 << 1), 0x0003);
		phy_writew(combo0->phy_reg + (0x4050 << 1), 0x002A);
		phy_writew(combo0->phy_reg + (0x4250 << 1), 0x002A);
		phy_writew(combo0->phy_reg + (0x404C << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x424C << 1), 0x0000);
	} else {
		phy_writew(combo0->phy_reg + (0x4500 << 1), 0x00FB);
		phy_writew(combo0->phy_reg + (0x4700 << 1), 0x00FB);
		phy_writew(combo0->phy_reg + (0x4502 << 1), 0x04AA);
		phy_writew(combo0->phy_reg + (0x4702 << 1), 0x04AA);
		phy_writew(combo0->phy_reg + (0x4503 << 1), 0x04AA);
		phy_writew(combo0->phy_reg + (0x4703 << 1), 0x04AA);
		phy_writew(combo0->phy_reg + (0x8400 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8600 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8402 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8602 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8403 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8603 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8406 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8606 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x44EA << 1), 0x000F);
		phy_writew(combo0->phy_reg + (0x46EA << 1), 0x000F);
		phy_writew(combo0->phy_reg + (0x8508 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8708 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8510 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8710 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8518 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8718 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x4440 << 1), 0x08A4);
		phy_writew(combo0->phy_reg + (0x4640 << 1), 0x08A4);
		phy_writew(combo0->phy_reg + (0x44C6 << 1), 0x0003);
		phy_writew(combo0->phy_reg + (0x46C6 << 1), 0x0003);
		phy_writew(combo0->phy_reg + (0x4450 << 1), 0x002A);
		phy_writew(combo0->phy_reg + (0x4650 << 1), 0x002A);
		phy_writew(combo0->phy_reg + (0x444C << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x464C << 1), 0x0000);
	}
	phy_writew(combo0->phy_reg + (0x0103 << 1), 0x007F);
	phy_writew(combo0->phy_reg + (0x010B << 1), 0x007F);

	return 0;
}

static int sunxi_cadence_combo0_dp_phy_init(struct phy *phy)
{
	struct sunxi_cadence_combophy *combo0 = phy_get_drvdata(phy);

	mutex_lock(&combo0->phy_lock);

	combo0_dp_phy_init(combo0);

	combo0->mode |= (1 << PHY_TYPE_DP);

	mutex_unlock(&combo0->phy_lock);

	return 0;
}

static int sunxi_cadence_combo0_dp_phy_exit(struct phy *phy)
{
	struct sunxi_cadence_combophy *combo0 = phy_get_drvdata(phy);

	mutex_lock(&combo0->phy_lock);

	combo0_dp_phy_exit(combo0);

	mutex_unlock(&combo0->phy_lock);

	return 0;
}

static int sunxi_cadence_combo0_dp_phy_power_on(struct phy *phy)
{
	struct sunxi_cadence_combophy *combo0 = phy_get_drvdata(phy);

	mutex_lock(&combo0->phy_lock);

	combo0_dp_phy_power_on(combo0);

	mutex_unlock(&combo0->phy_lock);

	return 0;
}

static int sunxi_cadence_combo0_dp_phy_power_off(struct phy *phy)
{
	struct sunxi_cadence_combophy *combo0 = phy_get_drvdata(phy);

	mutex_lock(&combo0->phy_lock);

	combo0_dp_phy_power_off(combo0);

	mutex_unlock(&combo0->phy_lock);

	return 0;
}

static int sunxi_cadence_combo0_dp_phy_configure(struct phy *phy, union phy_configure_opts *opts)
{
	struct sunxi_cadence_combophy *combo0 = phy_get_drvdata(phy);
	struct phy_configure_opts_dp *dp_config = &opts->dp;
	int ret = 0;

	mutex_lock(&combo0->phy_lock);

	combo0->mode |= (1 << PHY_TYPE_DP);

	ret = combo0_dp_set_ssc(combo0, dp_config);
	if (ret < 0)
		goto OUT;

	ret = combo0_dp_set_lanes(combo0, dp_config);
	if (ret < 0)
		goto OUT;

	ret = combo0_dp_set_rate(combo0, dp_config);
	if (ret < 0)
		goto OUT;

	ret = combo0_dp_set_sw_pre(combo0, dp_config);
	if (ret < 0)
		goto OUT;

OUT:
	mutex_unlock(&combo0->phy_lock);
	return ret;
}

static int sunxi_cadence_combo0_dp_phy_set_mode(struct phy *phy, enum phy_mode mode, int submode)
{
	struct sunxi_cadence_combophy *combo0 = phy_get_drvdata(phy);
	struct sunxi_cadence_phy *sunxi_cphy = combo0->sunxi_cphy;
	enum typec_orientation orientation = COMBO0_TYPEC_ORIENTATION(submode);
	unsigned long state = COMBO0_TYPEC_MODE(submode);
	__u8 hpd_state = COMBO0_HPD_STATE(submode);
	u32 val = 0;

	pr_debug("mode: %d, submode: %d, orientation: %s, state: %s", mode, submode,
	       typec_orientation_name[orientation], typec_state_name[state]);

	mutex_lock(&combo0->phy_lock);

	switch (mode) {
	case PHY_MODE_DP:
		if ((combo0->orientation != orientation) || (combo0->state != state)) {
			if ((state == TYPEC_DP_STATE_D) || (state == TYPEC_DP_STATE_F)) {
				combo0->usb_dp_state = true;
				combo0_dp_phy_reset(combo0, false);
				combo0_configure_usb_dp(combo0);
				combo0_dp_phy_reset(combo0, true);
			} else {
				combo0->usb_dp_state = false;
			}

			// set lane remap
			//combo0_dp_lane_remap(combo0, orientation);
			if (combo0->orientation != orientation) {
				val = readl(COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));
				if (orientation == TYPEC_ORIENTATION_REVERSE)
					val |= PHY0_TYPEC_CONN_DIR;
				else
					val &= ~PHY0_TYPEC_CONN_DIR;
				writel(val, COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));
			}
			/* something such as lane cnt limit are need in future */
			if (combo0->state != state) {
				//TODO
			}
		}

		if (combo0->hpd_state != hpd_state) {
			if (sunxi_cphy->extcon)
				extcon_set_state_sync(sunxi_cphy->extcon, EXTCON_DISP_DP, hpd_state);
			combo0->hpd_state = hpd_state;
		}
		combo0->orientation = orientation;
		combo0->state = state;

		break;
	default:
		dev_warn(sunxi_cphy->dev, "%s Configuration mismatch.\n", combo0->name);
		break;
	}

	mutex_unlock(&combo0->phy_lock);
	return 0;
}

/* use for typec STATE_D/F device, to limit its lane cnt max to 2 */
int sunxi_cadence_combo0_dp_phy_validate(struct phy *phy, enum phy_mode mode,
					 int submode, union phy_configure_opts *opts)
{
	struct sunxi_cadence_combophy *combo0 = phy_get_drvdata(phy);
	struct phy_configure_opts_dp *dp_config = &opts->dp;

	/* if not typec STATE_D/F application, ignore it */
	if (!combo0->usb_dp_state)
		return 0;

	switch (mode) {
	case PHY_MODE_DP:
		if (dp_config->set_lanes) {
			if (dp_config->lanes > MAX_STATE_D_F_LANE_CNT)
				return -EPROTONOSUPPORT;
			else
				return 0;
		}
		break;
	default:
		break;
	}

	return 0;
}

static const struct phy_ops combo0_dp_phy_ops = {
	.power_on	= sunxi_cadence_combo0_dp_phy_power_on,
	.power_off	= sunxi_cadence_combo0_dp_phy_power_off,
	.init		= sunxi_cadence_combo0_dp_phy_init,
	.exit		= sunxi_cadence_combo0_dp_phy_exit,
	.configure	= sunxi_cadence_combo0_dp_phy_configure,
	.set_mode	= sunxi_cadence_combo0_dp_phy_set_mode,
	.validate	= sunxi_cadence_combo0_dp_phy_validate,
	.owner		= THIS_MODULE,
};

static void combo0_usb_clk_set(struct sunxi_cadence_phy *sunxi_cphy, bool enable)
{
	struct sunxi_cadence_combophy *combo0 = sunxi_cphy->combo0;
	u32 val, tmp = 0;

	val = readl(COMB0_REG_PHYPINS_LINK_CTRL1(combo0->top_reg));
	tmp = PHY0_PMA_CMN_PLL1_REFCLK_SEL | PHY0_PMA_CMN_PLL0_REFCLK_SEL | PHY0_PMA_CMN_REFCLK_DIG_SEL;
	if (enable)
		val |= tmp;
	else
		val &= ~tmp;
	writel(val, COMB0_REG_PHYPINS_LINK_CTRL1(combo0->top_reg));

	val = readl(USB3P1_REG_PIPE_CLK_MAP_CTRL(sunxi_cphy->top_combo_reg));
	if (enable) {
		val &= ~USB3P1_PIPE_CLK_MAP_MASK;
		val |= USB3P1_PIPE_CLK_MAP(0);
	} else {
		val |= USB3P1_PIPE_CLK_MAP_MASK;
	}
	writel(val, USB3P1_REG_PIPE_CLK_MAP_CTRL(sunxi_cphy->top_combo_reg));

	val = readl(USB3P1_REG_PIPE_RXD_MAP_CTRL(sunxi_cphy->top_combo_reg));
	if (enable) {
		val &= ~USB3P1_PIPE_RXD_MAP_MASK;
		val |= USB3P1_PIPE_RXD_MAP(0);
	} else {
		val |= USB3P1_PIPE_RXD_MAP_MASK;
	}
	writel(val, USB3P1_REG_PIPE_RXD_MAP_CTRL(sunxi_cphy->top_combo_reg));
}

static void combo0_usb_param_config(struct sunxi_cadence_combophy *combo0)
{
	struct sunxi_cadence_phy *sunxi_cphy = combo0->sunxi_cphy;
	u32 val = 0;

	if (!COMBO_PHY_TYPE(combo0->mode, PHY_TYPE_USB3)) {
		pr_warn("%s USB3 Configuration mismatch.\n", combo0->name);
		return;
	}

	if (sunxi_cphy->udp_supported) {
		combo0_configure_usb_dp(combo0);
		return;
	}

	pr_debug("==================== orientation: %s DP configured: %s ====================",
		 typec_orientation_name[combo0->orientation], COMBO_PHY_TYPE(combo0->mode, PHY_TYPE_DP) ? "yes" : "no");

	if (COMBO_PHY_TYPE(combo0->mode, PHY_TYPE_DP)) {

		val = readl(COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));
		val &= ~PHY0_PIPE_LINK_RESET_N_USB3P1_MASK;
		writel(val, COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));

		phy_writew(combo0->phy_reg + (0xE005 << 1), 0x0003);
		phy_writew(combo0->phy_reg + (0xE006 << 1), 0x1124);

		/* Restore DP Reverse orientation register configuration */
		phy_writew(combo0->phy_reg + (0x4100 << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x4300 << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x4102 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x4302 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x4103 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x4303 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x8000 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8200 << 1), 0x091D);
		phy_writew(combo0->phy_reg + (0x8002 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8202 << 1), 0x0900);
		phy_writew(combo0->phy_reg + (0x8003 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8203 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8006 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8206 << 1), 0x010F);
		phy_writew(combo0->phy_reg + (0x40EA << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x42EA << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x8108 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8308 << 1), 0x0009);
		phy_writew(combo0->phy_reg + (0x8110 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8310 << 1), 0x0009);
		phy_writew(combo0->phy_reg + (0x8118 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8318 << 1), 0x000D);
		phy_writew(combo0->phy_reg + (0x4040 << 1), 0x2A84);
		phy_writew(combo0->phy_reg + (0x4240 << 1), 0x2A84);
		phy_writew(combo0->phy_reg + (0x40C6 << 1), 0x00A3);
		phy_writew(combo0->phy_reg + (0x42C6 << 1), 0x00A3);
		phy_writew(combo0->phy_reg + (0x4050 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x4250 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x404C << 1), 0x001C);
		phy_writew(combo0->phy_reg + (0x424C << 1), 0x001C);

		/* Restore DP Normal orientation register configuration */
		phy_writew(combo0->phy_reg + (0x4500 << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x4700 << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x4502 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x4702 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x4503 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x4703 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x8400 << 1), 0x091D);
		phy_writew(combo0->phy_reg + (0x8600 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8402 << 1), 0x0900);
		phy_writew(combo0->phy_reg + (0x8602 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8403 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8603 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8406 << 1), 0x010F);
		phy_writew(combo0->phy_reg + (0x8606 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x44EA << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x46EA << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x8508 << 1), 0x0009);
		phy_writew(combo0->phy_reg + (0x8708 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8510 << 1), 0x0009);
		phy_writew(combo0->phy_reg + (0x8710 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8518 << 1), 0x000D);
		phy_writew(combo0->phy_reg + (0x8718 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x4440 << 1), 0x2A84);
		phy_writew(combo0->phy_reg + (0x4640 << 1), 0x2A84);
		phy_writew(combo0->phy_reg + (0x44C6 << 1), 0x00A3);
		phy_writew(combo0->phy_reg + (0x46C6 << 1), 0x00A3);
		phy_writew(combo0->phy_reg + (0x4450 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x4650 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x444C << 1), 0x001C);
		phy_writew(combo0->phy_reg + (0x464C << 1), 0x001C);
	}

	phy_writew(combo0->phy_reg + (0x0022 << 1), 0x001A);
	phy_writew(combo0->phy_reg + (0x002A << 1), 0x0034);
	phy_writew(combo0->phy_reg + (0x002C << 1), 0x00DA);
	phy_writew(combo0->phy_reg + (0x0032 << 1), 0x0034);
	phy_writew(combo0->phy_reg + (0x0034 << 1), 0x00DA);
	phy_writew(combo0->phy_reg + (0x0064 << 1), 0x0082);
	phy_writew(combo0->phy_reg + (0x0065 << 1), 0x0082);
	phy_writew(combo0->phy_reg + (0x0074 << 1), 0x001A);
	phy_writew(combo0->phy_reg + (0x0104 << 1), 0x0020);
	phy_writew(combo0->phy_reg + (0x0105 << 1), 0x0007);
	phy_writew(combo0->phy_reg + (0x010C << 1), 0x0020);
	phy_writew(combo0->phy_reg + (0x010D << 1), 0x0007);
	phy_writew(combo0->phy_reg + (0x0114 << 1), 0x030C);
	phy_writew(combo0->phy_reg + (0x0115 << 1), 0x0007);
	phy_writew(combo0->phy_reg + (0x0124 << 1), 0x0007);
	phy_writew(combo0->phy_reg + (0x0125 << 1), 0x0003);
	phy_writew(combo0->phy_reg + (0x0126 << 1), 0x000F);
	phy_writew(combo0->phy_reg + (0x0128 << 1), 0x0132);
	phy_writew(combo0->phy_reg + (0x8044 << 1), 0x001A);
	phy_writew(combo0->phy_reg + (0x8244 << 1), 0x001A);
	phy_writew(combo0->phy_reg + (0x8444 << 1), 0x001A);
	phy_writew(combo0->phy_reg + (0x8644 << 1), 0x001A);
	phy_writew(combo0->phy_reg + (0x8045 << 1), 0x0082);
	phy_writew(combo0->phy_reg + (0x8245 << 1), 0x0082);
	phy_writew(combo0->phy_reg + (0x8445 << 1), 0x0082);
	phy_writew(combo0->phy_reg + (0x8645 << 1), 0x0082);
	phy_writew(combo0->phy_reg + (0x804C << 1), 0x001A);
	phy_writew(combo0->phy_reg + (0x824C << 1), 0x001A);
	phy_writew(combo0->phy_reg + (0x844C << 1), 0x001A);
	phy_writew(combo0->phy_reg + (0x864C << 1), 0x001A);
	phy_writew(combo0->phy_reg + (0x804D << 1), 0x0082);
	phy_writew(combo0->phy_reg + (0x824D << 1), 0x0082);
	phy_writew(combo0->phy_reg + (0x844D << 1), 0x0082);
	phy_writew(combo0->phy_reg + (0x864D << 1), 0x0082);
	phy_writew(combo0->phy_reg + (0x4123 << 1), 0x0A28);
	phy_writew(combo0->phy_reg + (0x4323 << 1), 0x0A28);
	phy_writew(combo0->phy_reg + (0x4523 << 1), 0x0A28);
	phy_writew(combo0->phy_reg + (0x4723 << 1), 0x0A28);
	phy_writew(combo0->phy_reg + (0x01A1 << 1), 0x8600);
	phy_writew(combo0->phy_reg + (0x01C1 << 1), 0x0601);

	if (combo0->orientation == TYPEC_ORIENTATION_REVERSE) {
		/* Restore USB Normal orientation register configuration */
		phy_writew(combo0->phy_reg + (0x40E5 << 1), 0x0012);
		phy_writew(combo0->phy_reg + (0x42E5 << 1), 0x0012);
		phy_writew(combo0->phy_reg + (0x44E6 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x46E6 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x44E7 << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x46E7 << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x44E5 << 1), 0x0012);
		phy_writew(combo0->phy_reg + (0x46E5 << 1), 0x0012);

		phy_writew(combo0->phy_reg + (0x44E5 << 1), 0x0041);
		phy_writew(combo0->phy_reg + (0x46E5 << 1), 0x0041);
		phy_writew(combo0->phy_reg + (0x40E6 << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x42E6 << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x40E7 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x42E7 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x40E5 << 1), 0x0019);
		phy_writew(combo0->phy_reg + (0x42E5 << 1), 0x0019);
	} else {
		/* Restore USB Reverse orientation register configuration */
		phy_writew(combo0->phy_reg + (0x44E5 << 1), 0x0012);
		phy_writew(combo0->phy_reg + (0x46E5 << 1), 0x0012);
		phy_writew(combo0->phy_reg + (0x40E6 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x42E6 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x40E7 << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x42E7 << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x40E5 << 1), 0x0012);
		phy_writew(combo0->phy_reg + (0x42E5 << 1), 0x0012);

		phy_writew(combo0->phy_reg + (0x40E5 << 1), 0x0041);
		phy_writew(combo0->phy_reg + (0x42E5 << 1), 0x0041);
		phy_writew(combo0->phy_reg + (0x44E6 << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x46E6 << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x44E7 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x46E7 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x44E5 << 1), 0x0019);
		phy_writew(combo0->phy_reg + (0x46E5 << 1), 0x0019);
	}

	phy_writew(combo0->phy_reg + (0x0094 << 1), 0x0004);
	phy_writew(combo0->phy_reg + (0x00D4 << 1), 0x0004);
	phy_writew(combo0->phy_reg + (0x01A4 << 1), 0x0509);
	phy_writew(combo0->phy_reg + (0x01C4 << 1), 0x0509);
	phy_writew(combo0->phy_reg + (0x01A5 << 1), 0x0F00);
	phy_writew(combo0->phy_reg + (0x01C5 << 1), 0x0F00);
	phy_writew(combo0->phy_reg + (0x01A6 << 1), 0x0F08);
	phy_writew(combo0->phy_reg + (0x01C6 << 1), 0x0F08);
	phy_writew(combo0->phy_reg + (0x0090 << 1), 0x0180);
	phy_writew(combo0->phy_reg + (0x00D0 << 1), 0x019F);
	phy_writew(combo0->phy_reg + (0x0091 << 1), 0x9D8A);
	phy_writew(combo0->phy_reg + (0x00D1 << 1), 0x6276);
	phy_writew(combo0->phy_reg + (0x0092 << 1), 0x0002);
	phy_writew(combo0->phy_reg + (0x00D2 << 1), 0x0002);
	phy_writew(combo0->phy_reg + (0x0093 << 1), 0x0102);
	phy_writew(combo0->phy_reg + (0x00D3 << 1), 0x0116);
	phy_writew(combo0->phy_reg + (0x01A0 << 1), 0x0002);
	phy_writew(combo0->phy_reg + (0x01C0 << 1), 0x0002);

	if (combo0->configuration & PHY_CFG_SSC) {
		phy_writew(combo0->phy_reg + (0x0098 << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x00D8 << 1), 0x0001);
	} else {
		phy_writew(combo0->phy_reg + (0x0098 << 1), 0x0002);
		phy_writew(combo0->phy_reg + (0x00D8 << 1), 0x0002);
	}

	phy_writew(combo0->phy_reg + (0x0099 << 1), 0x045F);
	phy_writew(combo0->phy_reg + (0x00D9 << 1), 0x04C4);
	phy_writew(combo0->phy_reg + (0x009A << 1), 0x006B);
	phy_writew(combo0->phy_reg + (0x00DA << 1), 0x006A);
	phy_writew(combo0->phy_reg + (0x009B << 1), 0x0004);
	phy_writew(combo0->phy_reg + (0x00DB << 1), 0x0004);
	phy_writew(combo0->phy_reg + (0x0084 << 1), 0x0104);
	phy_writew(combo0->phy_reg + (0x00C4 << 1), 0x0104);
	phy_writew(combo0->phy_reg + (0x0085 << 1), 0x0005);
	phy_writew(combo0->phy_reg + (0x00C5 << 1), 0x0005);
	phy_writew(combo0->phy_reg + (0x0086 << 1), 0x0337);
	phy_writew(combo0->phy_reg + (0x00C6 << 1), 0x0337);
	phy_writew(combo0->phy_reg + (0x0088 << 1), 0x0335);
	phy_writew(combo0->phy_reg + (0x00C8 << 1), 0x0335);
	phy_writew(combo0->phy_reg + (0x0082 << 1), 0x0003);
	phy_writew(combo0->phy_reg + (0x00C2 << 1), 0x0003);
	phy_writew(combo0->phy_reg + (0x009C << 1), 0x00CF);
	phy_writew(combo0->phy_reg + (0x00DC << 1), 0x00CF);
	phy_writew(combo0->phy_reg + (0x009E << 1), 0x00CE);
	phy_writew(combo0->phy_reg + (0x00DE << 1), 0x00CE);
	phy_writew(combo0->phy_reg + (0x009F << 1), 0x0005);
	phy_writew(combo0->phy_reg + (0x00DF << 1), 0x0005);

	val = readl(COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));
	if (combo0->orientation == TYPEC_ORIENTATION_REVERSE)
		val |= PHY0_TYPEC_CONN_DIR;
	else
		val &= ~PHY0_TYPEC_CONN_DIR;
	writel(val, COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));

	phy_writew(combo0->phy_reg + (0xC010 << 1), 0x5100);
	phy_writew(combo0->phy_reg + (0xC011 << 1), 0x0100);
	phy_writew(combo0->phy_reg + (0xC011 << 1), 0x010F);
	phy_writew(combo0->phy_reg + (0xC018 << 1), 0x0A0A);
	phy_writew(combo0->phy_reg + (0xC01A << 1), 0x1008);
	phy_writew(combo0->phy_reg + (0xC01B << 1), 0x0010);
	phy_writew(combo0->phy_reg + (0x0041 << 1), 0x8200);
	phy_writew(combo0->phy_reg + (0x0047 << 1), 0x8200);

	if (combo0->orientation == TYPEC_ORIENTATION_REVERSE) {
		/* Restore USB Normal orientation register configuration */
		phy_writew(combo0->phy_reg + (0x4100 << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x4101 << 1), 0x04AF);
		phy_writew(combo0->phy_reg + (0x4102 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x4103 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x8200 << 1), 0x091D);
		phy_writew(combo0->phy_reg + (0x8201 << 1), 0x091D);
		phy_writew(combo0->phy_reg + (0x8202 << 1), 0x0900);
		phy_writew(combo0->phy_reg + (0x8203 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x4040 << 1), 0x2A84);
		phy_writew(combo0->phy_reg + (0x404D << 1), 0x0011);
		phy_writew(combo0->phy_reg + (0x8290 << 1), 0x000C);
		phy_writew(combo0->phy_reg + (0x8308 << 1), 0x0009);
		phy_writew(combo0->phy_reg + (0x8349 << 1), 0x0c02);
		phy_writew(combo0->phy_reg + (0x8377 << 1), 0x06F6);
		phy_writew(combo0->phy_reg + (0x8378 << 1), 0x4606);
		phy_writew(combo0->phy_reg + (0x42EB << 1), 0x0006);
		phy_writew(combo0->phy_reg + (0x8371 << 1), 0x0519);
		phy_writew(combo0->phy_reg + (0x8372 << 1), 0x0519);
		phy_writew(combo0->phy_reg + (0x83E8 << 1), 0x1002);
		phy_writew(combo0->phy_reg + (0x83E5 << 1), 0x0B98);
		phy_writew(combo0->phy_reg + (0x83E2 << 1), 0x0C01);
		phy_writew(combo0->phy_reg + (0x83E3 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x83F5 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x83F4 << 1), 0x0311);
		phy_writew(combo0->phy_reg + (0x83FF << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x81FF << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8280 << 1), 0x010a);
		phy_writew(combo0->phy_reg + (0x8282 << 1), 0x0003);
		phy_writew(combo0->phy_reg + (0x40EA << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x42EA << 1), 0x00FF);

		phy_writew(combo0->phy_reg + (0x4700 << 1), 0x02FF);
		phy_writew(combo0->phy_reg + (0x4701 << 1), 0x06AF);
		phy_writew(combo0->phy_reg + (0x4702 << 1), 0x06AE);
		phy_writew(combo0->phy_reg + (0x4703 << 1), 0x06AE);
		phy_writew(combo0->phy_reg + (0x8400 << 1), 0x0D1D);
		phy_writew(combo0->phy_reg + (0x8401 << 1), 0x0D1D);
		phy_writew(combo0->phy_reg + (0x8402 << 1), 0x0D00);
		phy_writew(combo0->phy_reg + (0x8403 << 1), 0x0500);
		phy_writew(combo0->phy_reg + (0x4640 << 1), 0x2A82);
		phy_writew(combo0->phy_reg + (0x464D << 1), 0x0014);
		phy_writew(combo0->phy_reg + (0x8490 << 1), 0x0013);
		phy_writew(combo0->phy_reg + (0x8508 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8549 << 1), 0x0C02);
		phy_writew(combo0->phy_reg + (0x8577 << 1), 0x0330);
		phy_writew(combo0->phy_reg + (0x8578 << 1), 0x0300);
		phy_writew(combo0->phy_reg + (0x44EB << 1), 0x0003);
		phy_writew(combo0->phy_reg + (0x8571 << 1), 0x0019);
		phy_writew(combo0->phy_reg + (0x8572 << 1), 0x0019);
		phy_writew(combo0->phy_reg + (0x85E8 << 1), 0x1004);
		phy_writew(combo0->phy_reg + (0x85E5 << 1), 0x00F9);
		phy_writew(combo0->phy_reg + (0x85E2 << 1), 0x0C01);
		phy_writew(combo0->phy_reg + (0x85E3 << 1), 0x0002);
		phy_writew(combo0->phy_reg + (0x85F5 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x85F4 << 1), 0x0031);
		phy_writew(combo0->phy_reg + (0x85FF << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x87FF << 1), 0x0002);
		phy_writew(combo0->phy_reg + (0x8480 << 1), 0x018C);
		phy_writew(combo0->phy_reg + (0x8482 << 1), 0x0003);
		phy_writew(combo0->phy_reg + (0x46EA << 1), 0x000F);
		phy_writew(combo0->phy_reg + (0x44EA << 1), 0x00F0);
	} else {
		/* Restore USB Reverse orientation register configuration */
		phy_writew(combo0->phy_reg + (0x4700 << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x4701 << 1), 0x04AF);
		phy_writew(combo0->phy_reg + (0x4702 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x4703 << 1), 0x04AE);
		phy_writew(combo0->phy_reg + (0x8400 << 1), 0x091D);
		phy_writew(combo0->phy_reg + (0x8401 << 1), 0x091D);
		phy_writew(combo0->phy_reg + (0x8402 << 1), 0x0900);
		phy_writew(combo0->phy_reg + (0x8403 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x4640 << 1), 0x2A84);
		phy_writew(combo0->phy_reg + (0x464D << 1), 0x0011);
		phy_writew(combo0->phy_reg + (0x8490 << 1), 0x000C);
		phy_writew(combo0->phy_reg + (0x8508 << 1), 0x0009);
		phy_writew(combo0->phy_reg + (0x8549 << 1), 0x0C02);
		phy_writew(combo0->phy_reg + (0x8577 << 1), 0x06F6);
		phy_writew(combo0->phy_reg + (0x8578 << 1), 0x4606);
		phy_writew(combo0->phy_reg + (0x44EB << 1), 0x0006);
		phy_writew(combo0->phy_reg + (0x8571 << 1), 0x0519);
		phy_writew(combo0->phy_reg + (0x8572 << 1), 0x0519);
		phy_writew(combo0->phy_reg + (0x85E8 << 1), 0x1002);
		phy_writew(combo0->phy_reg + (0x85E5 << 1), 0x0B98);
		phy_writew(combo0->phy_reg + (0x85E2 << 1), 0x0C01);
		phy_writew(combo0->phy_reg + (0x85E3 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x85F5 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x85F4 << 1), 0x0311);
		phy_writew(combo0->phy_reg + (0x85FF << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x87FF << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8480 << 1), 0x010A);
		phy_writew(combo0->phy_reg + (0x8482 << 1), 0x0003);
		phy_writew(combo0->phy_reg + (0x46EA << 1), 0x00FF);
		phy_writew(combo0->phy_reg + (0x44EA << 1), 0x00FF);

		phy_writew(combo0->phy_reg + (0x4100 << 1), 0x02FF);
		phy_writew(combo0->phy_reg + (0x4101 << 1), 0x06AF);
		phy_writew(combo0->phy_reg + (0x4102 << 1), 0x06AE);
		phy_writew(combo0->phy_reg + (0x4103 << 1), 0x06AE);
		phy_writew(combo0->phy_reg + (0x8200 << 1), 0x0D1D);
		phy_writew(combo0->phy_reg + (0x8201 << 1), 0x0D1D);
		phy_writew(combo0->phy_reg + (0x8202 << 1), 0x0D00);
		phy_writew(combo0->phy_reg + (0x8203 << 1), 0x0500);
		phy_writew(combo0->phy_reg + (0x4040 << 1), 0x2A82);
		phy_writew(combo0->phy_reg + (0x404D << 1), 0x0014);
		phy_writew(combo0->phy_reg + (0x8290 << 1), 0x0013);
		phy_writew(combo0->phy_reg + (0x8308 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x8349 << 1), 0x0C02);
		phy_writew(combo0->phy_reg + (0x8377 << 1), 0x0330);
		phy_writew(combo0->phy_reg + (0x8378 << 1), 0x0300);
		phy_writew(combo0->phy_reg + (0x42EB << 1), 0x0003);
		phy_writew(combo0->phy_reg + (0x8371 << 1), 0x0019);
		phy_writew(combo0->phy_reg + (0x8372 << 1), 0x0019);
		phy_writew(combo0->phy_reg + (0x83E8 << 1), 0x1004);
		phy_writew(combo0->phy_reg + (0x83E5 << 1), 0x00F9);
		phy_writew(combo0->phy_reg + (0x83E2 << 1), 0x0C01);
		phy_writew(combo0->phy_reg + (0x83E3 << 1), 0x0002);
		phy_writew(combo0->phy_reg + (0x83F5 << 1), 0x0000);
		phy_writew(combo0->phy_reg + (0x83F4 << 1), 0x0031);
		phy_writew(combo0->phy_reg + (0x83FF << 1), 0x0001);
		phy_writew(combo0->phy_reg + (0x81FF << 1), 0x0002);
		phy_writew(combo0->phy_reg + (0x8280 << 1), 0x018C);
		phy_writew(combo0->phy_reg + (0x8282 << 1), 0x0003);
		phy_writew(combo0->phy_reg + (0x40EA << 1), 0x000F);
		phy_writew(combo0->phy_reg + (0x42EA << 1), 0x00F0);
	}

	phy_writew(combo0->phy_reg + (0xE003 << 1), 0x0001);
	phy_writew(combo0->phy_reg + (0x0103 << 1), 0x007F);
	phy_writew(combo0->phy_reg + (0x010B << 1), 0x007F);
}

static void combo0_usb_phy_reset(struct sunxi_cadence_combophy *combo0, bool enable)
{
	u32 val, cnt = 0;
	struct sunxi_cadence_phy *sunxi_cphy = combo0->sunxi_cphy;

	if (enable) {
		val = readl(COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));
		val &= ~PHY0_RESET_N;
		writel(val, COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));

		mdelay(1);

		val = readl(COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));
		val |= PHY0_RESET_N;
		writel(val, COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));
	}

	/* restore dp lane convertion configuration */
	val = readl(DP_RAWS_TXDW_CONV_CRTL(sunxi_cphy->top_combo_reg));
	val |= DP_PHY_CFG_DATA_EN_BIT;
	val |= DP_PHY_CFG_LN_EN_MASK;
	val |= DP_PHY_CFG_LN_RSTN_MASK;
	writel(val, DP_RAWS_TXDW_CONV_CRTL(sunxi_cphy->top_combo_reg));

	while (enable) {
		val = readl(COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));
		if (++cnt > 1000) {
			pr_warn("wrn: %s wait phy reset timeout\n", combo0->name);
			break;
		}
		if (val & PHY0_RESET_N)
			break;
		udelay(1);
	}

	mdelay(1);

	val = readl(COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));
	if (enable)
		val |= PHY0_PIPE_LINK_RESET_N_SOFT;
	else
		val &= ~PHY0_PIPE_LINK_RESET_N_SOFT;
	writel(val, COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));

	val = readl(COMB0_REG_PHYPINS_LINK_CTRL3(combo0->top_reg));
	if (enable)
		val |= PHY0_PMA_XCVR_PLLCLK_EN_LN;
	else
		val &= ~PHY0_PMA_XCVR_PLLCLK_EN_LN;
	writel(val, COMB0_REG_PHYPINS_LINK_CTRL3(combo0->top_reg));

	cnt = 0;
	while (enable) {
		val = readl(COMB0_REG_PHYPINS_LINK_CTRL3(combo0->top_reg));
		if (++cnt > 1000) {
			pr_warn("wrn: %s wait pll enable timeout\n", combo0->name);
			break;
		}
		if (val & PHY0_PMA_XCVR_PLLCLK_EN_LN)
			break;
		udelay(1);
	}

	val = readl(COMB0_REG_PHYPINS_LINK_CTRL3(combo0->top_reg));
	if (enable)
		val |= PHY0_PMA_XCVR_POWER_STATE_REQ_LN(1);
	else
		val &= ~PHY0_PMA_XCVR_POWER_STATE_REQ_LN_MASK;
	writel(val, COMB0_REG_PHYPINS_LINK_CTRL3(combo0->top_reg));

	val = readl(COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));
	if (enable)
		val |= PHY0_DP_LINK_RESET_N_SOFT;
	else
		val &= ~PHY0_DP_LINK_RESET_N_SOFT;
	writel(val, COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));

	val = readl(COMB0_REG_PHYPINS_LINK_STUS0(combo0->top_reg));
	if (!(val & PHY0_PMA_CMN_READY)) {
		mdelay(10);

		val = phy_readw(combo0->phy_reg + (0xC008 << 1));
		if (enable)
			val &= ~(0xF << 8);
		else
			val |= (0xF << 8);
		phy_writew(combo0->phy_reg + (0xC008 << 1), val);

		cnt = 0;
		while (enable) {
			val = readl(COMB0_REG_PHYPINS_LINK_STUS0(combo0->top_reg));
			if (++cnt > 1000) {
				pr_warn("wrn: %s wait pma ready timeout\n", combo0->name);
				break;
			}
			if (val & PHY0_PMA_CMN_READY)
				break;
			udelay(5);
		}
	}
}

static void combo0_usb_pwr_set(struct sunxi_cadence_phy *sunxi_cphy, bool enable)
{
	struct sunxi_cadence_combophy *combo0 = sunxi_cphy->combo0;
	u32 val;

	dev_info(sunxi_cphy->dev, "%s set power %s\n", combo0->name, enable ? "OFF" : "ON");
	val = readl(COMB0_REG_PHYPINS_LINK_CTRL2(combo0->top_reg));
	if (enable)
		val |= PHY0_IDDQ_EN_SEL;
	else
		val &= ~PHY0_IDDQ_EN_SEL;
	writel(val, COMB0_REG_PHYPINS_LINK_CTRL2(combo0->top_reg));

	/* IDDQ mode enable */
	val = readl(COMB0_REG_PHYPINS_LINK_CTRL2(combo0->top_reg));
	if (enable)
		val |= PHY0_IDDQ_EN_SOFT;
	else
		val &= ~PHY0_IDDQ_EN_SOFT;
	writel(val, COMB0_REG_PHYPINS_LINK_CTRL2(combo0->top_reg));
}

static int combo0_usb_phy_init(struct sunxi_cadence_combophy *combo0)
{
	struct sunxi_cadence_phy *sunxi_cphy = combo0->sunxi_cphy;
	int ret;

	combo0->configuration = PHY_CFG_SSC;

	if (sunxi_cphy->avdd_h_regulator)
		ret = regulator_enable(sunxi_cphy->avdd_h_regulator);

	if (sunxi_cphy->avdd_d_regulator)
		ret = regulator_enable(sunxi_cphy->avdd_d_regulator);

	if (sunxi_cphy->avdd_c_regulator)
		ret = regulator_enable(sunxi_cphy->avdd_c_regulator);

	combo_usb2_clk_set(sunxi_cphy, true);

	if (sunxi_cphy->serdes_clk) {
		ret = clk_set_rate(sunxi_cphy->serdes_clk, 100000000);
		if (ret) {
			dev_err(sunxi_cphy->dev, "set serdes_clk rate 100MHz err, return %d\n", ret);
			return ret;
		}

		ret = clk_prepare_enable(sunxi_cphy->serdes_clk);
		if (ret) {
			dev_err(sunxi_cphy->dev, "enable serdes_clk err, return %d\n", ret);
			return ret;
		}
	}

	if (sunxi_cphy->dcxo_serdes0_clk) {
		ret = clk_prepare_enable(sunxi_cphy->dcxo_serdes0_clk);
		if (ret) {
			dev_err(sunxi_cphy->dev, "enable dcxo_serdes0_clk err, return %d\n", ret);
			return ret;
		}
	}

	if (combo0->clk) {
		ret = clk_prepare_enable(combo0->clk);
		if (ret) {
			dev_err(sunxi_cphy->dev, "enable combo0 clk err, return %d\n", ret);
			return ret;
		}
	}

	combo0_usb_pwr_set(sunxi_cphy, false);
	combo0_usb_clk_set(sunxi_cphy, true);
	combo0_usb_param_config(combo0);
	combo0_usb_phy_reset(combo0, true);

	return 0;
}

static int combo0_usb_phy_exit(struct sunxi_cadence_combophy *combo0)
{
	struct sunxi_cadence_phy *sunxi_cphy = combo0->sunxi_cphy;

	combo0_usb_clk_set(sunxi_cphy, false);
	combo0_usb_phy_reset(combo0, false);
	combo0_usb_pwr_set(sunxi_cphy, true);

	if (combo0->clk)
		clk_disable_unprepare(combo0->clk);

	if (sunxi_cphy->dcxo_serdes0_clk)
		clk_disable_unprepare(sunxi_cphy->dcxo_serdes0_clk);

	if (sunxi_cphy->serdes_clk)
		clk_disable_unprepare(sunxi_cphy->serdes_clk);

	combo_usb2_clk_set(sunxi_cphy, false);

	if (sunxi_cphy->avdd_h_regulator)
		regulator_disable(sunxi_cphy->avdd_h_regulator);

	if (sunxi_cphy->avdd_d_regulator)
		regulator_disable(sunxi_cphy->avdd_d_regulator);

	if (sunxi_cphy->avdd_c_regulator)
		regulator_disable(sunxi_cphy->avdd_c_regulator);

	return 0;
}

static void combo0_usb_phy_power_on(struct sunxi_cadence_combophy *combo0)
{
}

static void combo0_usb_phy_power_off(struct sunxi_cadence_combophy *combo0)
{
}


static int sunxi_cadence_combo0_usb_phy_init(struct phy *phy)
{
	struct sunxi_cadence_combophy *combo0 = phy_get_drvdata(phy);

	mutex_lock(&combo0->phy_lock);

	combo0->mode |= (1 << PHY_TYPE_USB3);

	combo0_usb_phy_init(combo0);

	mutex_unlock(&combo0->phy_lock);

	return 0;
}

static int sunxi_cadence_combo0_usb_phy_exit(struct phy *phy)
{
	struct sunxi_cadence_combophy *combo0 = phy_get_drvdata(phy);

	mutex_lock(&combo0->phy_lock);

	combo0_usb_phy_exit(combo0);

	combo0->mode &= ~(1 << PHY_TYPE_USB3);

	mutex_unlock(&combo0->phy_lock);

	return 0;
}

static int sunxi_cadence_combo0_usb_phy_power_on(struct phy *phy)
{
	struct sunxi_cadence_combophy *combo0 = phy_get_drvdata(phy);

	mutex_lock(&combo0->phy_lock);

	combo0_usb_phy_power_on(combo0);

	mutex_unlock(&combo0->phy_lock);

	return 0;
}

static int sunxi_cadence_combo0_usb_phy_power_off(struct phy *phy)
{
	struct sunxi_cadence_combophy *combo0 = phy_get_drvdata(phy);

	mutex_lock(&combo0->phy_lock);

	combo0_usb_phy_power_off(combo0);

	mutex_unlock(&combo0->phy_lock);

	return 0;
}

static int sunxi_cadence_combo0_usb_phy_set_mode(struct phy *phy, enum phy_mode mode, int submode)
{
	struct sunxi_cadence_combophy *combo0 = phy_get_drvdata(phy);
	struct sunxi_cadence_phy *sunxi_cphy = combo0->sunxi_cphy;
	enum typec_orientation orientation = COMBO0_TYPEC_ORIENTATION(submode);
	unsigned long state = COMBO0_TYPEC_MODE(submode);

	combo0->usb_dp_state = false;
	pr_debug("mode: %d, submode: %d, orientation: %s, state: %s", mode, submode,
	       typec_orientation_name[orientation], typec_state_name[state]);

	mutex_lock(&combo0->phy_lock);
	switch (mode) {
	case PHY_MODE_USB_OTG:
		if (sunxi_cphy->phy_switcher_quirk)
			break;
		if ((combo0->orientation != orientation) || (combo0->state != state)) {
			combo0->orientation = orientation;
			combo0->state = state;

			combo0_usb_param_config(combo0);
		}
		combo0->orientation = orientation;
		combo0->state = state;
		break;
	case PHY_MODE_USB_HOST:
	case PHY_MODE_USB_DEVICE:
		break;
	case PHY_MODE_USB_HOST_SS:
		/**
		 * Condition 1: Power On or Reboot
		 * Condition 2: xhci_plat_probe Invalid
		 */
		if (combo0->orientation == TYPEC_ORIENTATION_NONE) {
			combo0->orientation = orientation;
			combo0_usb_param_config(combo0);
		} else if (orientation != TYPEC_ORIENTATION_NONE) {
			combo0->orientation = orientation;
		}
		break;
	default:
		dev_warn(sunxi_cphy->dev, "%s Configuration mismatch.\n", combo0->name);
		break;
	}

	mutex_unlock(&combo0->phy_lock);

	return 0;
}

static const struct phy_ops combo0_usb_phy_ops = {
	.power_on	= sunxi_cadence_combo0_usb_phy_power_on,
	.power_off	= sunxi_cadence_combo0_usb_phy_power_off,
	.init		= sunxi_cadence_combo0_usb_phy_init,
	.exit		= sunxi_cadence_combo0_usb_phy_exit,
	.set_mode	= sunxi_cadence_combo0_usb_phy_set_mode,
	.owner		= THIS_MODULE,
};


/***************************************************************************************************
 * Combo PHY1 Support.                                                                             *
 * - PIPE interface mode Compatible with PIPE Revision 4.4.1                                       *
 * - PCIe Gen 1, 2 and 3, Maximum Speed up to 8Gbps                                                *
 * - USB3.1 SS and SS+, Maximum Speed up to 10Gbps                                                 *
 * - Supports up to 1Lane                                                                          *
 **************************************************************************************************/
static void combo1_usb_clk_set(struct sunxi_cadence_phy *sunxi_cphy, bool enable)
{
	struct sunxi_cadence_combophy *combo1 = sunxi_cphy->combo1;
	u32 val, tmp = 0;

	val = readl(COMB1_REG_PHYPINS_LINK_CTRL1(combo1->top_reg));
	tmp = PHY1_PMA_CMN_PLL1_REFCLK_SEL | PHY1_PMA_CMN_PLL0_REFCLK_SEL |
	      PHY1_PMA_CMN_REFCLK_INT_MODE(3) | PHY1_PMA_CMN_REFCLK_MODE(2) | PHY1_PMA_CMN_REFCLK_DIG_SEL;
	if (enable)
		val |= tmp;
	else
		val &= ~tmp;
	writel(val, COMB1_REG_PHYPINS_LINK_CTRL1(combo1->top_reg));

	val = readl(COMB1_REG_PHYPINS_LANE0_CTRL0(combo1->top_reg));
	if (enable) {
		val |= PHY1_L00_MODE(1);
	} else {
		val &= ~PHY1_L00_MODE_MASK;
	}
	writel(val, COMB1_REG_PHYPINS_LANE0_CTRL0(combo1->top_reg));

	val = readl(COMB1_REG_PHYPINS_LINK_CTRL0(combo1->top_reg));
	if (enable) {
		val |= PHY1_LINK_CFG_LN(1);
	} else {
		val &= ~PHY1_LINK_CFG_LN_MASK;
	}
	writel(val, COMB1_REG_PHYPINS_LINK_CTRL0(combo1->top_reg));

	val = readl(SUBSYS_REG_DBGCTRL(sunxi_cphy->top_subsys_reg));
	if (enable) {
		val |= COMBO1_PHY_REFCLK_AUTO_GATE_DIS;
	} else {
		val &= ~COMBO1_PHY_REFCLK_AUTO_GATE_DIS;
	}
	writel(val, SUBSYS_REG_DBGCTRL(sunxi_cphy->top_subsys_reg));
}

static void combo1_usb_param_config(struct sunxi_cadence_combophy *combo1)
{
	u32 val = 0;
	phy_writew(combo1->phy_reg + (0x0022 << 1), 0x001A);
	phy_writew(combo1->phy_reg + (0x002A << 1), 0x0034);
	phy_writew(combo1->phy_reg + (0x002C << 1), 0x00DA);
	phy_writew(combo1->phy_reg + (0x0032 << 1), 0x0034);
	phy_writew(combo1->phy_reg + (0x0034 << 1), 0x00DA);
	phy_writew(combo1->phy_reg + (0x0064 << 1), 0x0082);
	phy_writew(combo1->phy_reg + (0x0065 << 1), 0x0082);
	phy_writew(combo1->phy_reg + (0x0074 << 1), 0x001A);
	phy_writew(combo1->phy_reg + (0x0104 << 1), 0x0020);
	phy_writew(combo1->phy_reg + (0x0105 << 1), 0x0007);
	phy_writew(combo1->phy_reg + (0x010C << 1), 0x0020);
	phy_writew(combo1->phy_reg + (0x010D << 1), 0x0007);
	phy_writew(combo1->phy_reg + (0x0114 << 1), 0x030C);
	phy_writew(combo1->phy_reg + (0x0115 << 1), 0x0007);
	phy_writew(combo1->phy_reg + (0x0124 << 1), 0x0007);
	phy_writew(combo1->phy_reg + (0x0125 << 1), 0x0003);
	phy_writew(combo1->phy_reg + (0x0126 << 1), 0x000F);
	phy_writew(combo1->phy_reg + (0x0128 << 1), 0x0132);
	phy_writew(combo1->phy_reg + (0x40C0 << 1), 0x0208);
	phy_writew(combo1->phy_reg + (0x40C2 << 1), 0x009C);
	phy_writew(combo1->phy_reg + (0x8044 << 1), 0x001A);
	phy_writew(combo1->phy_reg + (0x8045 << 1), 0x0082);
	phy_writew(combo1->phy_reg + (0x804C << 1), 0x001A);
	phy_writew(combo1->phy_reg + (0x804D << 1), 0x0082);
	phy_writew(combo1->phy_reg + (0x4123 << 1), 0x0A28);

	val = readl(COMB1_REG_PHYPINS_LANE0_CTRL0(combo1->top_reg));
	val &= ~PHY1_L00_MODE_MASK;
	writel(val, COMB1_REG_PHYPINS_LANE0_CTRL0(combo1->top_reg));

	val = readl(COMB1_REG_PHYPINS_LANE0_CTRL0(combo1->top_reg));
	val |= PHY1_L00_MODE(1);
	writel(val, COMB1_REG_PHYPINS_LANE0_CTRL0(combo1->top_reg));

	phy_writew(combo1->phy_reg + (0xC00E << 1), 0x0000);
	phy_writew(combo1->phy_reg + (0x01A1 << 1), 0x8600);
	phy_writew(combo1->phy_reg + (0x40E5 << 1), 0x0041);
	phy_writew(combo1->phy_reg + (0x0094 << 1), 0x0004);
	phy_writew(combo1->phy_reg + (0x00D4 << 1), 0x0004);
	phy_writew(combo1->phy_reg + (0x01A4 << 1), 0x0509);
	phy_writew(combo1->phy_reg + (0x01C4 << 1), 0x0509);
	phy_writew(combo1->phy_reg + (0x01A5 << 1), 0x0F00);
	phy_writew(combo1->phy_reg + (0x01C5 << 1), 0x0F00);
	phy_writew(combo1->phy_reg + (0x01A6 << 1), 0x0F08);
	phy_writew(combo1->phy_reg + (0x01C6 << 1), 0x0F08);
	phy_writew(combo1->phy_reg + (0x0090 << 1), 0x0180);
	phy_writew(combo1->phy_reg + (0x00D0 << 1), 0x0133);
	phy_writew(combo1->phy_reg + (0x0091 << 1), 0x9D8A);
	phy_writew(combo1->phy_reg + (0x00D1 << 1), 0xB13B);
	phy_writew(combo1->phy_reg + (0x0092 << 1), 0x0002);
	phy_writew(combo1->phy_reg + (0x00D2 << 1), 0x0002);
	phy_writew(combo1->phy_reg + (0x0093 << 1), 0x0102);
	phy_writew(combo1->phy_reg + (0x00D3 << 1), 0x00CE);
	phy_writew(combo1->phy_reg + (0x01A0 << 1), 0x0002);
	phy_writew(combo1->phy_reg + (0x01C0 << 1), 0x0002);
	phy_writew(combo1->phy_reg + (0x0098 << 1), 0x0001);
	phy_writew(combo1->phy_reg + (0x00D8 << 1), 0x0001);
	phy_writew(combo1->phy_reg + (0x0099 << 1), 0x045F);
	phy_writew(combo1->phy_reg + (0x00D9 << 1), 0x0399);
	phy_writew(combo1->phy_reg + (0x009A << 1), 0x006B);
	phy_writew(combo1->phy_reg + (0x00DA << 1), 0x0068);
	phy_writew(combo1->phy_reg + (0x009B << 1), 0x0004);
	phy_writew(combo1->phy_reg + (0x00DB << 1), 0x0004);
	phy_writew(combo1->phy_reg + (0x0084 << 1), 0x0104);
	phy_writew(combo1->phy_reg + (0x00C4 << 1), 0x0104);
	phy_writew(combo1->phy_reg + (0x0085 << 1), 0x0005);
	phy_writew(combo1->phy_reg + (0x00C5 << 1), 0x0005);
	phy_writew(combo1->phy_reg + (0x0086 << 1), 0x0337);
	phy_writew(combo1->phy_reg + (0x00C6 << 1), 0x0337);
	phy_writew(combo1->phy_reg + (0x0088 << 1), 0x0335);
	phy_writew(combo1->phy_reg + (0x00C8 << 1), 0x0335);
	phy_writew(combo1->phy_reg + (0x0082 << 1), 0x0003);
	phy_writew(combo1->phy_reg + (0x00C2 << 1), 0x0003);
	phy_writew(combo1->phy_reg + (0x009C << 1), 0x00CF);
	phy_writew(combo1->phy_reg + (0x00DC << 1), 0x00CF);
	phy_writew(combo1->phy_reg + (0x009E << 1), 0x00CE);
	phy_writew(combo1->phy_reg + (0x00DE << 1), 0x00CE);
	phy_writew(combo1->phy_reg + (0x009F << 1), 0x0005);
	phy_writew(combo1->phy_reg + (0x00DF << 1), 0x0005);
	phy_writew(combo1->phy_reg + (0xC020 << 1), 0x0A0A);
	phy_writew(combo1->phy_reg + (0xC022 << 1), 0x1008);
	phy_writew(combo1->phy_reg + (0xC023 << 1), 0x0010);
	phy_writew(combo1->phy_reg + (0x0041 << 1), 0x8200);
	phy_writew(combo1->phy_reg + (0x0047 << 1), 0x8200);
	phy_writew(combo1->phy_reg + (0x4100 << 1), 0x02FF);
	phy_writew(combo1->phy_reg + (0x4101 << 1), 0x06AF);
	phy_writew(combo1->phy_reg + (0x4102 << 1), 0x06AE);
	phy_writew(combo1->phy_reg + (0x4103 << 1), 0x06AE);
	phy_writew(combo1->phy_reg + (0x8000 << 1), 0x0D1D);
	phy_writew(combo1->phy_reg + (0x8001 << 1), 0x0D1D);
	phy_writew(combo1->phy_reg + (0x8002 << 1), 0x0D00);
	phy_writew(combo1->phy_reg + (0x8003 << 1), 0x0500);
	phy_writew(combo1->phy_reg + (0x4040 << 1), 0x2A82);
	phy_writew(combo1->phy_reg + (0x404D << 1), 0x0014);
	phy_writew(combo1->phy_reg + (0x8090 << 1), 0x0013);
	phy_writew(combo1->phy_reg + (0x8108 << 1), 0x0000);
	phy_writew(combo1->phy_reg + (0x8149 << 1), 0x0C02);
	phy_writew(combo1->phy_reg + (0x8177 << 1), 0x0330);
	phy_writew(combo1->phy_reg + (0x8178 << 1), 0x0300);
	phy_writew(combo1->phy_reg + (0x40EB << 1), 0x0003);
	phy_writew(combo1->phy_reg + (0x8171 << 1), 0x0019);
	phy_writew(combo1->phy_reg + (0x8172 << 1), 0x0019);
	phy_writew(combo1->phy_reg + (0x81E8 << 1), 0x1004);
	phy_writew(combo1->phy_reg + (0x81E5 << 1), 0x00F9);
	phy_writew(combo1->phy_reg + (0x81E2 << 1), 0x0C01);
	phy_writew(combo1->phy_reg + (0x81E3 << 1), 0x0002);
	phy_writew(combo1->phy_reg + (0x81F5 << 1), 0x0000);
	phy_writew(combo1->phy_reg + (0x81F4 << 1), 0x0031);
	phy_writew(combo1->phy_reg + (0x81FF << 1), 0x0001);
	phy_writew(combo1->phy_reg + (0x8080 << 1), 0x018C);
	phy_writew(combo1->phy_reg + (0x8082 << 1), 0x0003);
}

static void combo1_usb_phy_reset(struct sunxi_cadence_combophy *combo1, bool enable)
{
	struct sunxi_cadence_phy *sunxi_cphy = combo1->sunxi_cphy;
	u32 val, cnt = 0;

	val = readl(USB3P1_REG_PIPE_CLK_MAP_CTRL(sunxi_cphy->top_combo_reg));
	if (enable) {
		val &= ~USB3P1_PIPE_CLK_MAP_MASK;
		val |= USB3P1_PIPE_CLK_MAP(1);
	} else {
		val |= USB3P1_PIPE_CLK_MAP_MASK;
	}
	writel(val, USB3P1_REG_PIPE_CLK_MAP_CTRL(sunxi_cphy->top_combo_reg));

	val = readl(USB3P1_REG_PIPE_RXD_MAP_CTRL(sunxi_cphy->top_combo_reg));
	if (enable) {
		val &= ~USB3P1_PIPE_RXD_MAP_MASK;
		val |= USB3P1_PIPE_RXD_MAP(1);
	} else {
		val |= USB3P1_PIPE_RXD_MAP_MASK;
	}
	writel(val, USB3P1_REG_PIPE_RXD_MAP_CTRL(sunxi_cphy->top_combo_reg));

	val = readl(COMB1_REG_PHYPIPE_TXDATA_MAP_CTRL(sunxi_cphy->top_combo_reg));
	if (enable) {
		val &= ~COMB1_PHY_LN0_PIPE_TXD_MAP_MASK;
		val |= COMB1_PHY_LN0_PIPE_TXD_MAP(0);
	} else {
		val |= COMB1_PHY_LN0_PIPE_TXD_MAP_MASK;
	}
	writel(val, COMB1_REG_PHYPIPE_TXDATA_MAP_CTRL(sunxi_cphy->top_combo_reg));

	val = readl(COMB1_REG_PHYPINS_LINK_CTRL0(combo1->top_reg));
	if (enable)
		val |= PHY1_RESET_N;
	else
		val &= ~PHY1_RESET_N;
	writel(val, COMB1_REG_PHYPINS_LINK_CTRL0(combo1->top_reg));

	while (enable) {
		val = readl(COMB1_REG_PHYPINS_LINK_CTRL0(combo1->top_reg));
		if (++cnt > 1000) {
			pr_warn("wrn: %s wait phy reset timeout\n", combo1->name);
			break;
		}
		if (val & PHY1_RESET_N)
			break;
		udelay(1);
	}

	mdelay(1);

	val = readl(COMB1_REG_PHYPINS_LANE0_CTRL0(combo1->top_reg));
	if (enable)
		val |= PHY1_L00_RESET_N_SW;
	else
		val &= ~PHY1_L00_RESET_N_SW;
	writel(val, COMB1_REG_PHYPINS_LANE0_CTRL0(combo1->top_reg));

	cnt = 0;
	while (enable) {
		val = readl(COMB1_REG_PHYPINS_LINK_STUS0(combo1->top_reg));
		if (++cnt > 1000) {
			pr_warn("wrn: %s wait pma ready timeout\n", combo1->name);
			break;
		}
		if (val & PHY1_PMA_CMN_READY)
			break;
		udelay(5);
	}
}

static void combo1_usb_pwr_set(struct sunxi_cadence_phy *sunxi_cphy, bool enable)
{
	struct sunxi_cadence_combophy *combo1 = sunxi_cphy->combo1;
	u32 val;

	dev_info(sunxi_cphy->dev, "%s set power %s\n", combo1->name, enable ? "OFF" : "ON");
	val = readl(COMB1_REG_PHYPINS_LINK_CTRL3(combo1->top_reg));
	if (enable)
		val |= PHY1_IDDQ_EN_SEL;
	else
		val &= ~PHY1_IDDQ_EN_SEL;
	writel(val, COMB1_REG_PHYPINS_LINK_CTRL3(combo1->top_reg));

	/* IDDQ mode enable */
	val = readl(COMB1_REG_PHYPINS_LINK_CTRL3(combo1->top_reg));
	if (enable)
		val |= PHY1_IDDQ_EN;
	else
		val &= ~PHY1_IDDQ_EN;
	writel(val, COMB1_REG_PHYPINS_LINK_CTRL3(combo1->top_reg));
}

static int sunxi_cadence_phy_combo1_usb3_init(struct sunxi_cadence_phy *sunxi_cphy)
{
	struct sunxi_cadence_combophy *combo1 = sunxi_cphy->combo1;
	int ret;

	combo_usb2_clk_set(sunxi_cphy, true);

	if (sunxi_cphy->serdes_clk) {
		ret = clk_set_rate(sunxi_cphy->serdes_clk, 100000000);
		if (ret) {
			dev_err(sunxi_cphy->dev, "set serdes_clk rate 100MHz err, return %d\n", ret);
			return ret;
		}

		ret = clk_prepare_enable(sunxi_cphy->serdes_clk);
		if (ret) {
			dev_err(sunxi_cphy->dev, "enable serdes_clk err, return %d\n", ret);
			return ret;
		}
	}

	if (sunxi_cphy->dcxo_serdes1_clk) {
		ret = clk_prepare_enable(sunxi_cphy->dcxo_serdes1_clk);
		if (ret) {
			dev_err(sunxi_cphy->dev, "enable dcxo_serdes1_clk err, return %d\n", ret);
			return ret;
		}
	}

	if (combo1->clk) {
		ret = clk_prepare_enable(combo1->clk);
		if (ret) {
			dev_err(sunxi_cphy->dev, "enable combo1 clk err, return %d\n", ret);
			return ret;
		}
	}

	combo1_usb_pwr_set(sunxi_cphy, false);
	combo1_usb_clk_set(sunxi_cphy, true);
	combo1_usb_param_config(combo1);
	combo1_usb_phy_reset(combo1, true);

	return 0;
}

static void sunxi_cadence_phy_combo1_usb3_exit(struct sunxi_cadence_phy *sunxi_cphy)
{
	struct sunxi_cadence_combophy *combo1 = sunxi_cphy->combo1;

	combo1_usb_clk_set(sunxi_cphy, false);
	combo1_usb_phy_reset(combo1, false);
	combo1_usb_pwr_set(sunxi_cphy, true);

	if (combo1->clk)
		clk_disable_unprepare(combo1->clk);

	if (sunxi_cphy->dcxo_serdes1_clk)
		clk_disable_unprepare(sunxi_cphy->dcxo_serdes1_clk);

	if (sunxi_cphy->serdes_clk)
		clk_disable_unprepare(sunxi_cphy->serdes_clk);

	combo_usb2_clk_set(sunxi_cphy, false);
}

static int sunxi_cadence_combo1_usb_phy_init(struct phy *phy)
{
	struct sunxi_cadence_combophy *combo1 = phy_get_drvdata(phy);
	struct sunxi_cadence_phy *sunxi_cphy = combo1->sunxi_cphy;
	int ret = 0;

	mutex_lock(&combo1->phy_lock);

	ret = sunxi_cadence_phy_combo1_usb3_init(sunxi_cphy);

	combo1->mode |= (1 << PHY_TYPE_USB3);

	mutex_unlock(&combo1->phy_lock);

	return ret;
}

static int sunxi_cadence_combo1_usb_phy_exit(struct phy *phy)
{
	struct sunxi_cadence_combophy *combo1 = phy_get_drvdata(phy);
	struct sunxi_cadence_phy *sunxi_cphy = combo1->sunxi_cphy;

	mutex_lock(&combo1->phy_lock);

	sunxi_cadence_phy_combo1_usb3_exit(sunxi_cphy);

	mutex_unlock(&combo1->phy_lock);

	return 0;
}

static const struct phy_ops combo1_usb_phy_ops = {
	.init		= sunxi_cadence_combo1_usb_phy_init,
	.exit		= sunxi_cadence_combo1_usb_phy_exit,
	.owner		= THIS_MODULE,
};

static void sunxi_cadence_phy_pcie_phy_init(struct sunxi_cadence_phy *sunxi_cphy)
{
	struct sunxi_cadence_combophy *combo1 = sunxi_cphy->combo1;
	u32 val;

	writel(0x01100001, combo1->top_reg + 0x4);

	writew(0x1a, combo1->phy_reg + 0x44);

	writew(0x34, combo1->phy_reg + 0x54);
	writew(0xda, combo1->phy_reg + 0x58);

	writew(0x34, combo1->phy_reg + 0x64);
	writew(0xda, combo1->phy_reg + 0x68);

	writew(0x82, combo1->phy_reg + 0xc8);
	writew(0x82, combo1->phy_reg + 0xca);

	writew(0x1a, combo1->phy_reg + 0xe8);

	writew(0x20, combo1->phy_reg + 0x208);
	writew(0x7, combo1->phy_reg + 0x20a);
	writew(0x20, combo1->phy_reg + 0x218);
	writew(0x7, combo1->phy_reg + 0x21a);
	writew(0x30c, combo1->phy_reg + 0x228);
	writew(0x7, combo1->phy_reg + 0x22a);

	writew(0x7, combo1->phy_reg + 0x248);
	writew(0x3, combo1->phy_reg + 0x24a);
	writew(0xf, combo1->phy_reg + 0x24c);
	writew(0x132, combo1->phy_reg + 0x250);

	writew(0x208, combo1->phy_reg + 0x8180);
	writew(0x9c, combo1->phy_reg + 0x8184);

	writew(0x1a, combo1->phy_reg + 0x10088);
	writew(0x82, combo1->phy_reg + 0x1008a);
	writew(0x1a, combo1->phy_reg + 0x10098);
	writew(0x82, combo1->phy_reg + 0x1009a);

	writew(0xa28, combo1->phy_reg + 0x8246);

	val = readl(combo1->top_reg + 0x100);
	val &= ~0x30;
	writel(val, combo1->top_reg + 0x100);

	writew(0x4, combo1->phy_reg + 0x128);
	writew(0x4, combo1->phy_reg + 0x148);
	writew(0x4, combo1->phy_reg + 0x1a8);

	writew(0x509, combo1->phy_reg + 0x348);
	writew(0x509, combo1->phy_reg + 0x368);
	writew(0x509, combo1->phy_reg + 0x388);
	writew(0xf00, combo1->phy_reg + 0x34a);
	writew(0xf00, combo1->phy_reg + 0x36a);
	writew(0xf00, combo1->phy_reg + 0x38a);
	writew(0xf08, combo1->phy_reg + 0x34c);
	writew(0xf08, combo1->phy_reg + 0x36c);
	writew(0xf08, combo1->phy_reg + 0x38c);

	writew(0x180, combo1->phy_reg + 0x120);
	writew(0x133, combo1->phy_reg + 0x140);
	writew(0x133, combo1->phy_reg + 0x1a0);
	writew(0x9d8a, combo1->phy_reg + 0x122);
	writew(0xb13b, combo1->phy_reg + 0x142);
	writew(0xb13b, combo1->phy_reg + 0x1a2);
	writew(0x2, combo1->phy_reg + 0x124);
	writew(0x2, combo1->phy_reg + 0x144);
	writew(0x2, combo1->phy_reg + 0x1a4);
	writew(0x102, combo1->phy_reg + 0x126);
	writew(0xce, combo1->phy_reg + 0x146);
	writew(0xce, combo1->phy_reg + 0x1a6);

	writew(0x22, combo1->phy_reg + 0x340);
	writew(0x22, combo1->phy_reg + 0x360);
	writew(0x22, combo1->phy_reg + 0x380);

	writew(0x1, combo1->phy_reg + 0x130);
	writew(0x1, combo1->phy_reg + 0x150);
	writew(0x1, combo1->phy_reg + 0x1b0);
	writew(0x45f, combo1->phy_reg + 0x132);
	writew(0x2f0, combo1->phy_reg + 0x152);
	writew(0x399, combo1->phy_reg + 0x1b2);
	writew(0x6b, combo1->phy_reg + 0x134);
	writew(0x68, combo1->phy_reg + 0x154);
	writew(0x68, combo1->phy_reg + 0x1b4);
	writew(0x4, combo1->phy_reg + 0x136);
	writew(0x4, combo1->phy_reg + 0x156);
	writew(0x4, combo1->phy_reg + 0x1b6);

	writew(0x104, combo1->phy_reg + 0x108);
	writew(0x104, combo1->phy_reg + 0x188);
	writew(0x5, combo1->phy_reg + 0x10a);
	writew(0x5, combo1->phy_reg + 0x18a);
	writew(0x337, combo1->phy_reg + 0x10c);
	writew(0x337, combo1->phy_reg + 0x18c);
	writew(0x3dbe, combo1->phy_reg + 0x110);
	writew(0x3dbe, combo1->phy_reg + 0x190);
	writew(0x3, combo1->phy_reg + 0x104);
	writew(0x3, combo1->phy_reg + 0x184);

	writew(0x14, combo1->phy_reg + 0x138);
	writew(0x14, combo1->phy_reg + 0x1b8);
	writew(0x192, combo1->phy_reg + 0x13c);
	writew(0x192, combo1->phy_reg + 0x1bc);
	writew(0x6, combo1->phy_reg + 0x13e);
	writew(0x6, combo1->phy_reg + 0x1be);

	writew(0x0, combo1->phy_reg + 0x103c0);
	writew(0x19, combo1->phy_reg + 0x102e2);
	writew(0x19, combo1->phy_reg + 0x102e4);

	writew(0x1, combo1->phy_reg + 0x103fe);

	val = readl(combo1->phy_reg + 0x98);
	val &= ~0x3;
	val |= 0x2;
	writel(val, combo1->phy_reg + 0x98);

	val = readl(combo1->phy_reg + 0xa8);
	val &= ~0x3;
	val |= 0x2;
	writel(val, combo1->phy_reg + 0xa8);

	val = readl(combo1->phy_reg + 0xd8);
	val &= ~0x3;
	val |= 0x2;
	writel(val, combo1->phy_reg + 0xd8);

	val = readl(combo1->top_reg);
	writel(val | 0x1, combo1->top_reg);
	val = readl(combo1->top_reg + 0x100);
	writel(val | 0x1, combo1->top_reg + 0x100);

	val = readl(combo1->top_reg + 0x900);
	while (true) {
		udelay(100);
		val = readl(combo1->top_reg + 0x900);
		if (val & BIT(0))
			break;
	}

	val = readw(combo1->phy_reg + 0xa0);
	val |= 0x220;
	val &= ~0x2;
	val = 0x270;
	writew(val, combo1->phy_reg + 0xa0);

	val = readw(combo1->phy_reg + 0x98);
	val |= BIT(4);
	writew(val, combo1->phy_reg + 0x98);

	val = readw(combo1->phy_reg + 0x18000);
	val |= BIT(0);
	writew(val, combo1->phy_reg + 0x18000);

	val = readl(combo1->top_reg + 0x4);
	val |= BIT(28);
	writel(val, combo1->top_reg + 0x4);

}

static int sunxi_cadence_phy_combo1_pcie_init(struct sunxi_cadence_phy *sunxi_cphy)
{
	int ret;
	u32 val;

	ret = clk_set_rate(sunxi_cphy->serdes_clk, 100000000);
	if (ret)
		return ret;

	ret = clk_prepare_enable(sunxi_cphy->serdes_clk);
	if (ret)
		return ret;

	ret = clk_prepare_enable(sunxi_cphy->dcxo_serdes1_clk);
	if (ret)
		return ret;

	if (sunxi_cphy->serdes_reset)
		reset_control_deassert(sunxi_cphy->serdes_reset);

	/* enable pcie */
	val = readl(sunxi_cphy->top_subsys_reg + SUBSYS_PCIE_BGR);
	writel(val | SUBSYS_PCIE_GATING, sunxi_cphy->top_subsys_reg + SUBSYS_PCIE_BGR);

	/* disable combo1 auto gating */
	val = readl(sunxi_cphy->top_subsys_reg + SUBSYS_DBG_CTL);
	writel(val | SUBSYS_DISABLE_COMBO1_AUTOGATING, sunxi_cphy->top_subsys_reg + SUBSYS_DBG_CTL);

	/* switch pipe to pcie */
	writel(SUBSYS_COMB1_PIPE_PCIE, sunxi_cphy->top_combo_reg + SUBSYS_COMB1_PIPE);

	sunxi_cadence_phy_pcie_phy_init(sunxi_cphy);

	return 0;
}

static void sunxi_cadence_phy_combo1_pcie_exit(struct sunxi_cadence_phy *sunxi_cphy)
{
	if (sunxi_cphy->serdes_reset)
		reset_control_assert(sunxi_cphy->serdes_reset);

	clk_disable_unprepare(sunxi_cphy->serdes_clk);
	clk_disable_unprepare(sunxi_cphy->dcxo_serdes1_clk);
}


static int sunxi_cadence_combo1_pcie_phy_init(struct phy *phy)
{
	struct sunxi_cadence_combophy *combo1 = phy_get_drvdata(phy);
	struct sunxi_cadence_phy *sunxi_cphy = combo1->sunxi_cphy;
	int ret = 0;

	mutex_lock(&combo1->phy_lock);

	ret = sunxi_cadence_phy_combo1_pcie_init(sunxi_cphy);

	combo1->mode |= (1 << PHY_TYPE_PCIE);

	mutex_unlock(&combo1->phy_lock);

	return ret;
}

static int sunxi_cadence_combo1_pcie_phy_exit(struct phy *phy)
{
	struct sunxi_cadence_combophy *combo1 = phy_get_drvdata(phy);
	struct sunxi_cadence_phy *sunxi_cphy = combo1->sunxi_cphy;

	mutex_lock(&combo1->phy_lock);

	sunxi_cadence_phy_combo1_pcie_exit(sunxi_cphy);

	mutex_unlock(&combo1->phy_lock);

	return 0;
}

static const struct phy_ops combo1_pcie_phy_ops = {
	.init		= sunxi_cadence_combo1_pcie_phy_init,
	.exit		= sunxi_cadence_combo1_pcie_phy_exit,
	.owner		= THIS_MODULE,
};

static int sunxi_cadence_aux_hpd_power_on(struct phy *phy)
{
	struct sunxi_cadence_combophy *aux_phy = phy_get_drvdata(phy);

	mutex_lock(&aux_phy->phy_lock);

	msleep(10);
	/* aux phy init*/
	phy_set_lbits(aux_phy->top_reg + 0x00, 24, 4, 0x3);
	phy_set_lbits(aux_phy->top_reg + 0x00, 20, 2, 0x3);
	phy_set_lbits(aux_phy->top_reg + 0x00, 16, 2, 0x3);
	phy_set_lbits(aux_phy->top_reg + 0x00, 12, 2, 0x1);
	phy_set_lbits(aux_phy->top_reg + 0x00, 8, 1, 0x1);

	phy_set_lbits(aux_phy->top_reg + 0x04, 28, 1, 0x0);
	phy_set_lbits(aux_phy->top_reg + 0x04, 24, 1, 0x0);
	phy_set_lbits(aux_phy->top_reg + 0x04, 16, 1, 0x1);
	phy_set_lbits(aux_phy->top_reg + 0x04, 8, 5, 0xe);
	phy_set_lbits(aux_phy->top_reg + 0x04, 0, 5, 0xd);

	phy_writel(aux_phy->top_reg + 0x08, 0);

	phy_set_lbits(aux_phy->top_reg + 0x00, 4, 1, 0x1);
	phy_set_lbits(aux_phy->top_reg + 0x00, 5, 1, 0x1);
	phy_set_lbits(aux_phy->top_reg + 0x00, 0, 1, 0x1);

	mutex_unlock(&aux_phy->phy_lock);

	return 0;
}

static int sunxi_cadence_aux_hpd_power_off(struct phy *phy)
{
	return 0;
}

static int sunxi_cadence_aux_hpd_init(struct phy *phy)
{
	struct sunxi_cadence_combophy *aux_phy = phy_get_drvdata(phy);
	struct sunxi_cadence_phy *sunxi_cphy = aux_phy->sunxi_cphy;

	mutex_lock(&aux_phy->phy_lock);

	msleep(10);
	if (sunxi_cphy->serdes_clk)
		clk_prepare_enable(sunxi_cphy->serdes_clk);

	if (sunxi_cphy->serdes_reset)
		reset_control_deassert(sunxi_cphy->serdes_reset);

	aux_phy->mode |= (1 << PHY_TYPE_DP);

	mutex_unlock(&aux_phy->phy_lock);

	return 0;
}

static int sunxi_cadence_aux_hpd_exit(struct phy *phy)
{
	struct sunxi_cadence_combophy *aux_phy = phy_get_drvdata(phy);
	struct sunxi_cadence_phy *sunxi_cphy = aux_phy->sunxi_cphy;

	mutex_lock(&aux_phy->phy_lock);

	if (sunxi_cphy->serdes_reset)
		reset_control_assert(sunxi_cphy->serdes_reset);

	if (sunxi_cphy->serdes_clk)
		clk_disable_unprepare(sunxi_cphy->serdes_clk);

	mutex_unlock(&aux_phy->phy_lock);

	return 0;
}

static const struct phy_ops aux_hpd_phy_ops = {
	.power_on	= sunxi_cadence_aux_hpd_power_on,
	.power_off	= sunxi_cadence_aux_hpd_power_off,
	.init		= sunxi_cadence_aux_hpd_init,
	.exit		= sunxi_cadence_aux_hpd_exit,
	.owner		= THIS_MODULE,
};

const struct phy_ops *sunxi_cadence_node_to_ops(struct device_node *np)
{
	const struct phy_ops *ops = NULL;

	if (of_node_name_eq(np, "combo0-dp-phy")) {
		ops = &combo0_dp_phy_ops;
	} else if (of_node_name_eq(np, "combo0-usb-phy")) {
		ops = &combo0_usb_phy_ops;
	} else if (of_node_name_eq(np, "combo1-usb-phy")) {
		ops = &combo1_usb_phy_ops;
	} else if (of_node_name_eq(np, "combo1-pcie-phy")) {
		ops = &combo1_pcie_phy_ops;
	} else if (of_node_name_eq(np, "aux-hpd-phy")) {
		ops = &aux_hpd_phy_ops;
	}

	return ops;
}

int sunxi_cadence_phy_create(struct device *dev, struct device_node *np,
			     struct sunxi_cadence_combophy *combophy, enum phy_type_e type)
{
	struct sunxi_cadence_phy *sunxi_cphy = dev_get_drvdata(dev);
	struct phy *phy = NULL;
	const struct phy_ops *ops = NULL;
	int ret;
	u32 prop_val[MAX_LANE_CNT], i;
	struct phy_provider *phy_provider = NULL;
	struct device_node *child;

	switch (type) {
	case COMBO_PHY0:
		combophy->name = kstrdup_const("combophy0", GFP_KERNEL);
		break;
	case COMBO_PHY1:
		combophy->name = kstrdup_const("combophy1", GFP_KERNEL);
		break;
	case AUX_HPD:
		combophy->name = kstrdup_const("aux_hpd", GFP_KERNEL);
		break;
	default:
		pr_err("not support phy type (%d)\n", type);
		return -EINVAL;
	}

	combophy->clk = devm_get_clk_from_child(dev, np, "phy-clk");
	if (IS_ERR(combophy->clk)) {
		combophy->clk = NULL;
		pr_debug("Maybe there is no clk for phy (%s)\n", combophy->name);
	}

	combophy->bus_clk = devm_get_clk_from_child(dev, np, "phy-bus-clk");
	if (IS_ERR(combophy->bus_clk)) {
		combophy->bus_clk = NULL;
		pr_debug("Maybe there is no bus clk for phy (%s)\n", combophy->name);
	}

	combophy->reset = of_reset_control_get(np, "phy_reset");
	if (IS_ERR(combophy->reset)) {
		combophy->reset = NULL;
		pr_debug("Maybe there is no reset for phy (%s)\n", combophy->name);
	}

	if (combophy->reset) {
		ret = devm_add_action_or_reset(dev, sunxi_cadence_phy_reset_control_put,
					       combophy->reset);
		if (ret)
			return ret;
	}

	combophy->top_reg = of_iomap(np, 0);
	if (!combophy->top_reg)
		return -ENOMEM;

	combophy->phy_reg = of_iomap(np, 1);
	if (!combophy->phy_reg) {
		combophy->phy_reg = NULL;
		pr_debug("Maybe there is no phy reg for %s\n", combophy->name);
	}

	ret = of_property_read_u32_array(np, "lane_invert", prop_val, MAX_LANE_CNT);
	if (ret == 0) {
		for (i = 0; i < MAX_LANE_CNT; i++)
			combophy->lane_invert[i] = prop_val[i];
	} else {
		for (i = 0; i < MAX_LANE_CNT; i++)
			combophy->lane_invert[i] = 0;
	}

	ret = of_property_read_u32_array(np, "lane_remap", prop_val, MAX_LANE_CNT);
	if (ret == 0) {
		for (i = 0; i < MAX_LANE_CNT; i++)
			combophy->lane_remap[i] = prop_val[i];
	} else {
		for (i = 0; i < MAX_LANE_CNT; i++)
			combophy->lane_remap[i] = i;
	}

	ret = of_property_read_u32_array(np, "typec_remap", prop_val, MAX_LANE_CNT);
	if (ret == 0) {
		for (i = 0; i < MAX_LANE_CNT; i++)
			combophy->typec_remap[i] = prop_val[i];
	} else {
		for (i = 0; i < MAX_LANE_CNT; i++)
			combophy->typec_remap[i] = i;
	}

	//ret = of_property_read_u32(np, "ssc_en", &val);
	//if (ret == 0) {
	//	combophy->ssc_en = val ? 1 : 0;
	//} else {
	//	combophy->ssc_en = 0;
	//}

	for_each_available_child_of_node(np, child) {
		ops = sunxi_cadence_node_to_ops(child);
		if (ops) {
			phy = devm_phy_create(dev, child, ops);
			if (IS_ERR(phy)) {
				ret = PTR_ERR(phy);
				dev_err(dev, "failed to create phy for %s, ret:%d\n", combophy->name, ret);
				return ret;
			}

			phy_set_drvdata(phy, combophy);
		} else {
			dev_err(dev, "failed to create phy for %s beacuse of ops missing\n", combophy->name);
		}
	}

	mutex_init(&combophy->phy_lock);
	combophy->sunxi_cphy = sunxi_cphy;
	combophy->mode = PHY_NONE;
	combophy->usb_dp_state = false;
	combophy->ssc_en = 0;

	pr_debug("phy[%s]: lane_invert[%d %d %d %d] lane_remap[%d %d %d %d] typec_remap[%d %d %d %d]\n",
		 combophy->name, combophy->lane_invert[0], combophy->lane_invert[1],
		 combophy->lane_invert[2], combophy->lane_invert[3], combophy->lane_remap[0],
		 combophy->lane_remap[1], combophy->lane_remap[2], combophy->lane_remap[3],
		 combophy->typec_remap[0], combophy->typec_remap[1], combophy->typec_remap[2],
		 combophy->typec_remap[3]);

	phy_provider = __devm_of_phy_provider_register(dev, np, THIS_MODULE, sunxi_cadence_phy_xlate);
	if (IS_ERR(phy_provider))
		return PTR_ERR_OR_ZERO(phy_provider);

	return 0;
}

/*******************************************************************
 * Note:
 * The features support by phy are listed as follow. From the table,
 * we can know that PCIE/DP/DP_AUX has their only one specific conmophy
 * the use, and USB3.1 can select between combo0 and combo1.
 *
 * So, we assume: DP only use combo0, PCIE only use combo1, USB may
 * choose combo0 firstly.
 *
 * Some complex situation need to be solved later:
 * How to compliance with all three module exist, USB/PCIE/DP ?
 *  _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
 * |             | USB3.1 | PCIE | DisplayPort | DP_AUX  |
 * |_ _ _ _ _ _ _|_ _ _ _ |_ _ _ |_ _ _ _ _ _ _|_ _ _ _ _|
 * | Combophy0   |   O    |  X   |      O      |    X    |
 * |_ _ _ _ _ _ _|_ _ _ _ |_ _ _ |_ _ _ _ _ _ _|_ _ _ _ _|
 * | Combophy1   |   O    |  O   |      X      |    X    |
 * |_ _ _ _ _ _ _|_ _ _ _ |_ _ _ |_ _ _ _ _ _ _|_ _ _ _ _|
 * | AUX_HPD_PHY |   X    |  X   |      X      |    O    |
 * |_ _ _ _ _ _  |_ _ _ _ |_ _ _ |_ _ _ _ _ _ _|_ _ _ _ _|
 *
 *******************************************************************/
static struct phy *sunxi_cadence_phy_xlate(struct device *dev,
					  struct of_phandle_args *args)
{
	struct phy *phy = NULL;

	phy = of_phy_simple_xlate(dev, args);
	if (IS_ERR(phy)) {
		pr_err("%s fail\n", __func__);
		return phy;
	}

	/* TODO: if need */

	return phy;
}

static int sunxi_cadence_phy_serdes_init(struct sunxi_cadence_phy *sunxi_cphy)
{
	int ret;

	if (!IS_ERR(sunxi_cphy->serdes1v8_supply)) {
		ret = regulator_set_voltage(sunxi_cphy->serdes1v8_supply, 1800000, 1800000);
		if (ret) {
			dev_err(sunxi_cphy->dev, "set serdes 1v8-supply failed\n");
		}

		ret = regulator_enable(sunxi_cphy->serdes1v8_supply);
		if (ret) {
			dev_err(sunxi_cphy->dev, "enable serdes 1v8-supply failed\n");
		}
	}

	if (sunxi_cphy->serdes_reset) {
		ret = reset_control_deassert(sunxi_cphy->serdes_reset);
		if (ret) {
			dev_err(sunxi_cphy->dev, "reset serdes err, return %d\n", ret);
			return ret;
		}
	}

	if (sunxi_cphy->usb_supported)
		combo_usb_clk_set(sunxi_cphy, true);

	sunxi_cphy->vernum = top_ver_get(sunxi_cphy);

	return 0;
}

static void sunxi_cadence_phy_serdes_exit(struct sunxi_cadence_phy *sunxi_cphy)
{
	if (sunxi_cphy->usb_supported)
		combo_usb_clk_set(sunxi_cphy, false);

	if (sunxi_cphy->serdes_reset)
		reset_control_assert(sunxi_cphy->serdes_reset);

	if (!IS_ERR(sunxi_cphy->serdes1v8_supply))
		regulator_disable(sunxi_cphy->serdes1v8_supply);
}

static int sunxi_cadence_phy_parse_dt(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_cadence_phy *sunxi_cphy = dev_get_drvdata(dev);
	struct device_node *child;
	int ret;

	/* usb supported */
	sunxi_cphy->usb_supported = !device_property_read_bool(dev, "usb-disable");
	sunxi_cphy->udp_supported = !device_property_read_bool(dev, "udp-disable");
	sunxi_cphy->phy_switcher_quirk = device_property_read_bool(dev, "aw,phy-switcher-quirk");
	/* parse top register, which determide general configuration such as mode */
	sunxi_cphy->top_subsys_reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(sunxi_cphy->top_subsys_reg))
		return PTR_ERR(sunxi_cphy->top_subsys_reg);

	sunxi_cphy->top_combo_reg = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(sunxi_cphy->top_combo_reg))
		return PTR_ERR(sunxi_cphy->top_combo_reg);

	sunxi_cphy->serdes_clk = devm_clk_get(dev, "serdes-clk");
	if (IS_ERR(sunxi_cphy->serdes_clk)) {
		return dev_err_probe(dev, PTR_ERR(sunxi_cphy->serdes_clk),
			"failed to get serdes clock for sunxi cadence phy\n");
	}

	sunxi_cphy->dcxo_serdes0_clk = devm_clk_get(dev, "dcxo-serdes0-clk");
	if (IS_ERR(sunxi_cphy->dcxo_serdes0_clk)) {
		return dev_err_probe(dev, PTR_ERR(sunxi_cphy->dcxo_serdes0_clk),
			"failed to get dcxo serdes0 clock for sunxi cadence phy\n");
	}

	sunxi_cphy->dcxo_serdes1_clk = devm_clk_get(dev, "dcxo-serdes1-clk");
	if (IS_ERR(sunxi_cphy->dcxo_serdes1_clk)) {
		return dev_err_probe(dev, PTR_ERR(sunxi_cphy->dcxo_serdes1_clk),
			"failed to get dcxo serdes1 clock for sunxi cadence phy\n");
	}

	sunxi_cphy->serdes_reset = devm_reset_control_get_shared(dev, "serdes-reset");
	if (IS_ERR(sunxi_cphy->serdes_reset)) {
		return dev_err_probe(dev, PTR_ERR(sunxi_cphy->serdes_reset),
			"failed to get serdes reset for sunxi cadence phy\n");
	}

	/* serdes 1.8V AVDD-H-COMB0 supply */
	sunxi_cphy->serdes1v8_supply = devm_regulator_get_optional(dev, "serdes1v8");
	if (IS_ERR(sunxi_cphy->serdes1v8_supply))
		dev_err(dev, "get serdes 1v8-supply fail\n");

/*
	sunxi_cphy->avdd_h_regulator = devm_regulator_get(dev, "avdd-h");
	if (IS_ERR(sunxi_cphy->avdd_h_regulator))
		pr_warn("AVDD-H supply is not found, maybe it's not need!");

	sunxi_cphy->avdd_c_regulator = devm_regulator_get(dev, "avdd-c");
	if (IS_ERR(sunxi_cphy->avdd_c_regulator))
		pr_warn("AVDD-C supply is not found, maybe it's not need!");

	sunxi_cphy->avdd_d_regulator = devm_regulator_get(dev, "avdd-d");
	if (IS_ERR(sunxi_cphy->avdd_d_regulator))
		pr_warn("AVDD-D supply is not found, maybe it's not need!");

	sunxi_cphy->avdd_aux_regulator = devm_regulator_get(dev, "avdd-aux");
	if (IS_ERR(sunxi_cphy->avdd_aux_regulator))
		pr_warn("AVDD-AUX supply is not found, maybe it's not need!");
*/

	for_each_available_child_of_node(dev->of_node, child) {
		if (of_node_name_eq(child, "combo-phy0")) {
			sunxi_cphy->combo0 = devm_kzalloc(dev, sizeof(*sunxi_cphy->combo0), GFP_KERNEL);
			if (!sunxi_cphy->combo0)
				return -ENOMEM;

			/* create combophy0 */
			ret = sunxi_cadence_phy_create(dev, child, sunxi_cphy->combo0, COMBO_PHY0);
			if (ret) {
				dev_err(dev, "failed to create cadence combophy0, ret:%d\n", ret);
				goto err_node_put;
			}
			sunxi_cphy->combo0->orientation = TYPEC_ORIENTATION_NONE;

		} else if (of_node_name_eq(child, "combo-phy1")) {
			sunxi_cphy->combo1 = devm_kzalloc(dev, sizeof(*sunxi_cphy->combo1), GFP_KERNEL);
			if (!sunxi_cphy->combo1)
				return -ENOMEM;

			/* create combophy1 */
			ret = sunxi_cadence_phy_create(dev, child, sunxi_cphy->combo1, COMBO_PHY1);
			if (ret) {
				dev_err(dev, "failed to create cadence combophy1, ret:%d\n", ret);
				goto err_node_put;
			}

		} else if (of_node_name_eq(child, "aux-hpd")) {
			sunxi_cphy->aux_hpd = devm_kzalloc(dev, sizeof(*sunxi_cphy->aux_hpd), GFP_KERNEL);
			if (!sunxi_cphy->aux_hpd)
				return -ENOMEM;

			/* create aux phy */
			ret = sunxi_cadence_phy_create(dev, child, sunxi_cphy->aux_hpd, AUX_HPD);
			if (ret) {
				dev_err(dev, "failed to create cadence aux-hpd phy, ret:%d\n", ret);
				goto err_node_put;
			}
		}
	}

	sunxi_cphy->extcon = devm_extcon_dev_allocate(dev, sunxi_cadence_cable);
	if (IS_ERR_OR_NULL(sunxi_cphy->extcon)) {
		dev_err(dev, "devm_extcon_dev_allocate fail\n");
		return PTR_ERR(sunxi_cphy->extcon);
	}
	devm_extcon_dev_register(dev, sunxi_cphy->extcon);


	return 0;

err_node_put:
	of_node_put(child);

	return ret;
}

static int sunxi_cadence_phy_probe(struct platform_device *pdev)
{
	struct sunxi_cadence_phy *sunxi_cphy;
	struct device *dev = &pdev->dev;
	int ret;

	sunxi_cphy = devm_kzalloc(dev, sizeof(*sunxi_cphy), GFP_KERNEL);
	if (!sunxi_cphy)
		return -ENOMEM;

	sunxi_cphy->dev = dev;
	dev_set_drvdata(dev, sunxi_cphy);

	ret = sunxi_cadence_phy_parse_dt(pdev);
	if (ret)
		return -EINVAL;

	ret = sunxi_cadence_phy_serdes_init(sunxi_cphy);
	if (ret)
		return -EINVAL;
	dev_info(dev, "Sub System Version v%lu.%lu.%lu\n", FIELD_GET(TOP_GEN_VER, sunxi_cphy->vernum),
		 FIELD_GET(TOP_SUB_VER, sunxi_cphy->vernum), FIELD_GET(TOP_PRJ_VER, sunxi_cphy->vernum));

	return 0;
}

static int sunxi_cadence_phy_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_cadence_phy *sunxi_cphy = dev_get_drvdata(dev);

	sunxi_cadence_phy_serdes_exit(sunxi_cphy);

	return 0;
}

static int __maybe_unused sunxi_cadence_phy_suspend(struct device *dev)
{
	struct sunxi_cadence_phy *sunxi_cphy = dev_get_drvdata(dev);

	sunxi_cadence_phy_serdes_exit(sunxi_cphy);

	return 0;
}

static int __maybe_unused sunxi_cadence_phy_resume(struct device *dev)
{
	struct sunxi_cadence_phy *sunxi_cphy = dev_get_drvdata(dev);
	struct sunxi_cadence_combophy *combo0 = sunxi_cphy->combo0;
	int ret;
	u32 reg_val;

	ret = sunxi_cadence_phy_serdes_init(sunxi_cphy);
	if (ret) {
		dev_err(dev, "failed to resume sub system\n");
		return ret;
	}

	reg_val = readl(COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));
	if (combo0->orientation == TYPEC_ORIENTATION_REVERSE)
		reg_val |= PHY0_TYPEC_CONN_DIR;
	else
		reg_val &= ~PHY0_TYPEC_CONN_DIR;
	writel(reg_val, COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));


	return 0;
}

static struct dev_pm_ops sunxi_cadence_phy_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sunxi_cadence_phy_suspend, sunxi_cadence_phy_resume)
};

static const struct of_device_id sunxi_cadence_phy_of_match_table[] = {
	{
		.compatible = "allwinner,cadence-combophy",
	},
};

static struct platform_driver sunxi_cadence_phy_driver = {
	.probe		= sunxi_cadence_phy_probe,
	.remove		= sunxi_cadence_phy_remove,
	.driver = {
		.name	= "sunxi-cadence-combophy",
		.pm	= &sunxi_cadence_phy_pm_ops,
		.of_match_table = sunxi_cadence_phy_of_match_table,
	},
};
module_platform_driver(sunxi_cadence_phy_driver);

MODULE_AUTHOR("huangyongxing@allwinnertech.com");
MODULE_DESCRIPTION("Allwinner CADENCE COMBOPHY driver");
MODULE_VERSION("0.1.7");
MODULE_LICENSE("GPL v2");
