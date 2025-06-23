// SPDX-License-Identifier: GPL-2.0
/*
 * phy driver for cdn_sd0804_t7g_typec
 */

#include <linux/acpi.h>
#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/usb/ch9.h>
#include <linux/usb/typec_dp.h>
#include <linux/usb/typec_mux.h>
#include <linux/io.h>

#include "phy-cix-usbdp.h"

enum {
	UDPHY_MODE_NONE		= 0,
	UDPHY_MODE_USB		= BIT(0),
	UDPHY_MODE_DP		= BIT(1),
	UDPHY_MODE_DP_USB	= BIT(1) | BIT(0),
};

#define PHY_LANE_MUX_USB			0
#define PHY_LANE_MUX_DP				1

struct cix_udphy;

struct cix_udphy_con_dir {
	u16 offset;
	u8 bit;
};

struct cix_udphy_cfg {
	const struct cix_udphy_con_dir* con_dir;
	int (*udphy_init)(struct cix_udphy *udphy);
	int (*udphy_exit)(struct cix_udphy *udphy);
};

/*
 * USB 3.0/3.1 Type C Alt Mode
 *   port 0: USB_TCAM; port 1, DP, 8.1 Gbps by default
 */
static const struct reg_sequence sky1_udphy_usbdp_conf_normal[] = {
	{CMN_SSM_BIAS_TMR,              0x0018},
	{CMN_PLLSM0_PLLPRE_TMR,         0x0030},
	{CMN_PLLSM0_PLLLOCK_TMR,        0x00f0},
	{CMN_PLLSM1_PLLPRE_TMR,         0x0030},
	{CMN_PLLSM1_PLLLOCK_TMR,        0x00f0},
	{CMN_BGCAL_INIT_TMR,            0x0078},
	{CMN_BGCAL_ITER_TMR,            0x0078},
	{CMN_IBCAL_INIT_TMR,            0x0018},
	{CMN_TXPUCAL_INIT_TMR,          0x001d},
	{CMN_TXPDCAL_INIT_TMR,          0x001d},
	{CMN_RXCAL_INIT_TMR,            0x02d0},
	{CMN_SD_CAL_PLLCNT_START,       0x0137},

	{RX_SDCAL0_INIT_TMR_LANE0,      0x0018},
	{RX_SDCAL0_INIT_TMR_LANE1,      0x0018},
	{RX_SDCAL0_INIT_TMR_LANE2,      0x0018},
	{RX_SDCAL0_INIT_TMR_LANE3,      0x0018},
	{RX_SDCAL0_ITER_TMR_LANE0,      0x0078},
	{RX_SDCAL0_ITER_TMR_LANE1,      0x0078},
	{RX_SDCAL0_ITER_TMR_LANE2,      0x0078},
	{RX_SDCAL0_ITER_TMR_LANE3,      0x0078},
	{RX_SDCAL1_INIT_TMR_LANE0,      0x0018},
	{RX_SDCAL1_INIT_TMR_LANE1,      0x0018},
	{RX_SDCAL1_INIT_TMR_LANE2,      0x0018},
	{RX_SDCAL1_INIT_TMR_LANE3,      0x0018},
	{RX_SDCAL1_ITER_TMR_LANE0,      0x0078},
	{RX_SDCAL1_ITER_TMR_LANE1,      0x0078},
	{RX_SDCAL1_ITER_TMR_LANE2,      0x0078},
	{RX_SDCAL1_ITER_TMR_LANE3,      0x0078},
	{TX_RCVDET_ST_TMR_LANE0,        0x0960},
	{TX_RCVDET_ST_TMR_LANE1,        0x0960},
	{TX_RCVDET_ST_TMR_LANE2,        0x0960},
	{TX_RCVDET_ST_TMR_LANE3,        0x0960},

	{CMN_PDIAG_PLL0_CLK_SEL_M0,     0x8600},
	{CMN_PDIAG_PLL1_CLK_SEL_M0,     0x0601},
	{XCVR_DIAG_PLLDRC_CTRL_LANE0,   0x0041},
	{XCVR_DIAG_PLLDRC_CTRL_LANE1,   0x0041},
	{XCVR_DIAG_HSCLK_SEL_LANE2,     0x0001},
	{XCVR_DIAG_HSCLK_SEL_LANE3,     0x0001},
	{XCVR_DIAG_HSCLK_DIV_LANE2,     0x0000},
	{XCVR_DIAG_HSCLK_DIV_LANE3,     0x0000},
	{XCVR_DIAG_PLLDRC_CTRL_LANE2,   0x0019},
	{XCVR_DIAG_PLLDRC_CTRL_LANE3,   0x0019},

	{CMN_PLL0_DSM_DIAG_M0,          0x0004},
	{CMN_PLL1_DSM_DIAG_M0,          0x8004},
	{CMN_PDIAG_PLL1_ITRIM_M0,       0x003f},
	{CMN_PDIAG_PLL0_CP_PADJ_M0,     0x0B17},
	{CMN_PDIAG_PLL1_CP_PADJ_M0,     0x0B17},
	{CMN_PDIAG_PLL0_CP_IADJ_M0,     0x0E01},
	{CMN_PDIAG_PLL1_CP_IADJ_M0,     0x0E01},
	{CMN_PDIAG_PLL0_FILT_PADJ_M0,   0x0D05},
	{CMN_PDIAG_PLL1_FILT_PADJ_M0,   0x0D05},

	{CMN_PLL0_INTDIV_M0,            0x01A0},
	{CMN_PLL1_INTDIV_M0,            0x01C2},
	{CMN_PLL0_FRACDIVL_M0,          0xAAAB},

	{CMN_PLL1_FRACDIVL_M0,          0x0000}, /* reset value. */

	{CMN_PLL0_FRACDIVH_M0,          0x0002},
	{CMN_PLL1_FRACDIVH_M0,          0x0002},
	{CMN_PLL0_HIGH_THR_M0,          0x0116},
	{CMN_PLL1_HIGH_THR_M0,          0x012C},
	{CMN_PDIAG_PLL0_CTRL_M0,        0x1002},
	{CMN_PDIAG_PLL1_CTRL_M0,        0x1002},
	{CMN_PLL0_SS_CTRL1_M0,          0x0001},
	{CMN_PLL1_SS_CTRL1_M0,          0x0001},
	{CMN_PLL0_SS_CTRL2_M0,          0x0416},
	{CMN_PLL1_SS_CTRL2_M0,          0x044F},
	{CMN_PLL0_SS_CTRL3_M0,          0x007C},
	{CMN_PLL1_SS_CTRL3_M0,          0x007F},
	{CMN_PLL0_SS_CTRL4_M0,          0x0003},
	{CMN_PLL1_SS_CTRL4_M0,          0x0003},
	{CMN_PLL0_VCOCAL_INIT_TMR,      0x00F0},
	{CMN_PLL1_VCOCAL_INIT_TMR,      0x00F0},
	{CMN_PLL0_VCOCAL_ITER_TMR,      0x0004},
	{CMN_PLL1_VCOCAL_ITER_TMR,      0x0004},
	{CMN_PLL0_VCOCAL_REFTIM_START,  0x02F8},
	{CMN_PLL1_VCOCAL_REFTIM_START,  0x02F8},
	{CMN_PLL0_VCOCAL_PLLCNT_START,  0x02F6},
	{CMN_PLL1_VCOCAL_PLLCNT_START,  0x02F6},
	{CMN_PLL0_VCOCAL_TCTRL,         0x0003},
	{CMN_PLL1_VCOCAL_TCTRL,         0x0003},
	{CMN_PLL0_LOCK_REFCNT_START,    0x00BF},
	{CMN_PLL1_LOCK_REFCNT_START,    0x00BF},
	{CMN_PLL0_LOCK_PLLCNT_START,    0x00BF},
	{CMN_PLL1_LOCK_PLLCNT_START,    0x00BF},
	{CMN_PLL0_LOCK_PLLCNT_THR,      0x0005},
	{CMN_PLL1_LOCK_PLLCNT_THR,      0x0005},

	/* usb_part */
	{PHY_PMA_LANE_MAP,              0x5100},
	{PHY_LANE_OFF_CTRL,             0x0100},
	{PHY_PIPE_USB3_GEN2_PRE_CFG0,   0x0A0A},
	{PHY_PIPE_USB3_GEN2_POST_CFG0,  0x1000},
	{PHY_PIPE_USB3_GEN2_POST_CFG1,  0x0010},
	{CMN_CDIAG_CDB_PWRI_OVRD,       0x8200},
	{CMN_CDIAG_XCVRC_PWRI_OVRD,     0x8200},
	{TX_PSC_A0_LANE0,               0x02FF},
	{TX_PSC_A1_LANE0,               0x06AF},
	{TX_PSC_A2_LANE0,               0x06AE},
	{TX_PSC_A3_LANE0,               0x06AE},
	{RX_PSC_A0_LANE1,               0x0D1D},
	{RX_PSC_A1_LANE1,               0x0D1D},
	{RX_PSC_A2_LANE1,               0x0D00},
	{RX_PSC_A3_LANE1,               0x0500},
	{TX_TXCC_CTRL_LANE0,            0x2A82},
	{TX_TXCC_CPOST_MULT_01_LANE0,   0x0014},
	{TX_TXCC_MGNFS_MULT_000_LANE0,  0x0007},
	{RX_SIGDET_HL_FILT_TMR_LANE1,   0x0013},
	{RX_REE_GCSM1_CTRL_LANE1,       0x0000},
	{RX_REE_ATTEN_THR_LANE1,        0x0C02},
	{RX_REE_SMGM_CTRL1_LANE1,       0x0330},
	{RX_REE_SMGM_CTRL2_LANE1,       0x0300},
	{XCVR_DIAG_PSC_OVRD_LANE1,      0x0003},
	{RX_DIAG_SIGDET_TUNE_LANE1,     0x1004},
	{RX_DIAG_NQST_CTRL_LANE1,       0x00F9},
	{RX_DIAG_DFE_AMP_TUNE_2_LANE1,  0x0C01},
	{RX_DIAG_DFE_AMP_TUNE_3_LANE1,  0x0002},
	{RX_DIAG_PI_CAP_LANE1,          0x0000},
	{RX_DIAG_PI_RATE_LANE1,         0x0031},
	{RX_DIAG_ACYA_LANE0,            0x0002},
	{RX_CDRLF_CNFG_LANE1,           0x018C},
	{RX_CDRLF_CNFG3_LANE1,          0x0003},
	/* usb adding to upspeed */
	{XCVR_DIAG_BIDI_CTRL_LANE0,     0x000f}, /* new add */
	{XCVR_DIAG_BIDI_CTRL_LANE1,     0x00F0}, /* new add */

	/* dp_part */
	{PHY_PMA_ISO_PLL_CTRL0,         0x000B},
	{PHY_PMA_ISO_PLL_CTRL1,         0x2224},

	{TX_PSC_A0_LANE2,              0x00FB},
	{TX_PSC_A0_LANE3,              0x00FB},
	{TX_PSC_A2_LANE2,              0x04AA},
	{TX_PSC_A2_LANE3,              0x04AA},
	{TX_PSC_A3_LANE2,              0x04AA},
	{TX_PSC_A3_LANE3,              0x04AA},
	{RX_PSC_A0_LANE2,              0x0000},
	{RX_PSC_A0_LANE3,              0x0000},
	{RX_PSC_A2_LANE2,              0x0000},
	{RX_PSC_A2_LANE3,              0x0000},
	{RX_PSC_A3_LANE2,              0x0000},
	{RX_PSC_A3_LANE3,              0x0000},
	{RX_PSC_CAL_LANE2,             0x0000},
	{RX_PSC_CAL_LANE3,             0x0000},
	{XCVR_DIAG_BIDI_CTRL_LANE2,    0x000F},
	{XCVR_DIAG_BIDI_CTRL_LANE3,    0x000F},
	{RX_REE_GCSM1_CTRL_LANE2,      0x0000},
	{RX_REE_GCSM1_CTRL_LANE3,      0x0000},
	{RX_REE_GCSM2_CTRL_LANE2,      0x0000},
	{RX_REE_GCSM2_CTRL_LANE3,      0x0000},
	{RX_REE_PERGCSM_CTRL_LANE2,    0x0000},
	{RX_REE_PERGCSM_CTRL_LANE3,    0x0000},

	{CMN_DIAG_GPANA_0,             0x0100}, /* new add */
};

/*
 * TYPEC_ALT_MODE_DP
 *   DP only
 */
static const struct reg_sequence sky1_udphy_dp_conf[] = {
	{CMN_SSM_BIAS_TMR,              0x0018},
	{CMN_PLLSM0_PLLPRE_TMR,         0x0030},
	{CMN_PLLSM0_PLLLOCK_TMR,        0x00f0},
	{CMN_PLLSM1_PLLPRE_TMR,         0x0030},
	{CMN_PLLSM1_PLLLOCK_TMR,        0x00f0},
	{CMN_BGCAL_INIT_TMR,            0x0078},
	{CMN_BGCAL_ITER_TMR,            0x0078},
	{CMN_IBCAL_INIT_TMR,            0x0018},
	{CMN_TXPUCAL_INIT_TMR,          0x001d},
	{CMN_TXPDCAL_INIT_TMR,          0x001d},
	{CMN_RXCAL_INIT_TMR,            0x02d0},
	{CMN_SD_CAL_PLLCNT_START,       0x0137},
	{RX_SDCAL0_INIT_TMR_LANE0,      0x0018},
	{RX_SDCAL0_INIT_TMR_LANE1,      0x0018},
	{RX_SDCAL0_INIT_TMR_LANE2,      0x0018},
	{RX_SDCAL0_INIT_TMR_LANE3,      0x0018},
	{RX_SDCAL0_ITER_TMR_LANE0,      0x0078},
	{RX_SDCAL0_ITER_TMR_LANE1,      0x0078},
	{RX_SDCAL0_ITER_TMR_LANE2,      0x0078},
	{RX_SDCAL0_ITER_TMR_LANE3,      0x0078},
	{RX_SDCAL1_INIT_TMR_LANE0,      0x0018},
	{RX_SDCAL1_INIT_TMR_LANE1,      0x0018},
	{RX_SDCAL1_INIT_TMR_LANE2,      0x0018},
	{RX_SDCAL1_INIT_TMR_LANE3,      0x0018},
	{RX_SDCAL1_ITER_TMR_LANE0,      0x0078},
	{RX_SDCAL1_ITER_TMR_LANE1,      0x0078},
	{RX_SDCAL1_ITER_TMR_LANE2,      0x0078},
	{RX_SDCAL1_ITER_TMR_LANE3,      0x0078},
	{TX_RCVDET_ST_TMR_LANE0,        0x0960},
	{TX_RCVDET_ST_TMR_LANE1,        0x0960},
	{TX_RCVDET_ST_TMR_LANE2,        0x0960},
	{TX_RCVDET_ST_TMR_LANE3,        0x0960},
	{CMN_PDIAG_PLL0_CLK_SEL_M0,     0x8600},
	{CMN_PDIAG_PLL1_CLK_SEL_M0,     0x0701},
	{XCVR_DIAG_HSCLK_SEL_LANE0,     0x0001},
	{XCVR_DIAG_HSCLK_SEL_LANE1,     0x0001},
	{XCVR_DIAG_HSCLK_SEL_LANE2,     0x0001},
	{XCVR_DIAG_HSCLK_SEL_LANE3,     0x0001},
	{XCVR_DIAG_HSCLK_DIV_LANE0,     0x0001},
	{XCVR_DIAG_HSCLK_DIV_LANE1,     0x0001},
	{XCVR_DIAG_HSCLK_DIV_LANE2,     0x0001},
	{XCVR_DIAG_HSCLK_DIV_LANE3,     0x0001},
	{XCVR_DIAG_PLLDRC_CTRL_LANE0,   0x0019},
	{XCVR_DIAG_PLLDRC_CTRL_LANE1,   0x0019},
	{XCVR_DIAG_PLLDRC_CTRL_LANE2,   0x0019},
	{XCVR_DIAG_PLLDRC_CTRL_LANE3,   0x0019},
	{CMN_PLL0_DSM_DIAG_M0,          0x0004},
	{CMN_PLL1_DSM_DIAG_M0,          0x8004},
	{CMN_PDIAG_PLL1_ITRIM_M0,       0x003f},

	{CMN_PDIAG_PLL0_CP_PADJ_M0,     0x0b17},
	{CMN_PDIAG_PLL1_CP_PADJ_M0,     0x0b17},
	{CMN_PDIAG_PLL0_CP_IADJ_M0,     0x0e01},
	{CMN_PDIAG_PLL1_CP_IADJ_M0,     0x0e01},
	{CMN_PDIAG_PLL0_FILT_PADJ_M0,   0x0d05},
	{CMN_PDIAG_PLL1_FILT_PADJ_M0,   0x0d05},
	{CMN_PLL0_INTDIV_M0,            0x01a0},
	{CMN_PLL1_INTDIV_M0,            0x01c2},
	{CMN_PLL0_FRACDIVL_M0,          0xaaab},
	{CMN_PLL1_FRACDIVL_M0,          0x0000},
	{CMN_PLL0_FRACDIVH_M0,          0x0002},
	{CMN_PLL1_FRACDIVH_M0,          0x0002},
	{CMN_PLL0_HIGH_THR_M0,          0x0116},
	{CMN_PLL1_HIGH_THR_M0,          0x012c},
	{CMN_PDIAG_PLL0_CTRL_M0,        0x1002},
	{CMN_PDIAG_PLL1_CTRL_M0,        0x1002},

	{CMN_PLL0_SS_CTRL1_M0,          0x0001},
	{CMN_PLL1_SS_CTRL1_M0,          0x0001},
	{CMN_PLL0_SS_CTRL2_M0,          0x0416},
	{CMN_PLL1_SS_CTRL2_M0,          0x044F},
	{CMN_PLL0_SS_CTRL3_M0,          0x007C},
	{CMN_PLL1_SS_CTRL3_M0,          0x007F},
	{CMN_PLL0_SS_CTRL4_M0,          0x0003},
	{CMN_PLL1_SS_CTRL4_M0,          0x0003},

	{CMN_PLL0_VCOCAL_INIT_TMR,      0x00f0},
	{CMN_PLL1_VCOCAL_INIT_TMR,      0x00f0},
	{CMN_PLL0_VCOCAL_ITER_TMR,      0x0004},
	{CMN_PLL1_VCOCAL_ITER_TMR,      0x0004},
	{CMN_PLL0_VCOCAL_REFTIM_START,  0x02f8},
	{CMN_PLL1_VCOCAL_REFTIM_START,  0x02f8},
	{CMN_PLL0_VCOCAL_PLLCNT_START,  0x02f6},
	{CMN_PLL1_VCOCAL_PLLCNT_START,  0x02f6},
	{CMN_PLL0_VCOCAL_TCTRL,         0x0003},
	{CMN_PLL1_VCOCAL_TCTRL,         0x0003},
	{CMN_PLL0_LOCK_REFCNT_START,    0x00bf},
	{CMN_PLL1_LOCK_REFCNT_START,    0x00bf},
	{CMN_PLL0_LOCK_PLLCNT_START,    0x00bf},
	{CMN_PLL1_LOCK_PLLCNT_START,    0x00bf},
	{CMN_PLL0_LOCK_PLLCNT_THR,      0x0005},
	{CMN_PLL1_LOCK_PLLCNT_THR,      0x0005},
	{PHY_PMA_LANE_MAP,              0x51D9},
	{PHY_LANE_OFF_CTRL,             0x0100},
	{PHY_PIPE_USB3_GEN2_PRE_CFG0,   0x0a0a},
	{PHY_PIPE_USB3_GEN2_POST_CFG0,  0x1000},
	{PHY_PIPE_USB3_GEN2_POST_CFG1,  0x0010},
	{CMN_CDIAG_CDB_PWRI_OVRD,       0x8200},
	{CMN_CDIAG_XCVRC_PWRI_OVRD,     0x8200},

	{PHY_PMA_ISO_PLL_CTRL0,         0x000b},
	{PHY_PMA_ISO_PLL_CTRL1,         0x2224},
	{TX_PSC_A0_LANE0,               0x00fb},
	{TX_PSC_A0_LANE1,               0x00fb},
	{TX_PSC_A0_LANE2,               0x00fb},
	{TX_PSC_A0_LANE3,               0x00fb},
	{TX_PSC_A2_LANE0,               0x04aa},
	{TX_PSC_A2_LANE1,               0x04aa},
	{TX_PSC_A2_LANE2,               0x04aa},
	{TX_PSC_A2_LANE3,               0x04aa},
	{TX_PSC_A3_LANE0,               0x04aa},
	{TX_PSC_A3_LANE1,               0x04aa},
	{TX_PSC_A3_LANE2,               0x04aa},
	{TX_PSC_A3_LANE3,               0x04aa},
	{RX_PSC_A0_LANE0,               0x0000},
	{RX_PSC_A0_LANE1,               0x0000},
	{RX_PSC_A0_LANE2,               0x0000},
	{RX_PSC_A0_LANE3,               0x0000},
	{RX_PSC_A2_LANE0,               0x0000},
	{RX_PSC_A2_LANE1,               0x0000},
	{RX_PSC_A2_LANE2,               0x0000},
	{RX_PSC_A2_LANE3,               0x0000},
	{RX_PSC_A3_LANE0,               0x0000},
	{RX_PSC_A3_LANE1,               0x0000},
	{RX_PSC_A3_LANE2,               0x0000},
	{RX_PSC_A3_LANE3,               0x0000},
	{RX_PSC_CAL_LANE0,              0x0000},
	{RX_PSC_CAL_LANE1,              0x0000},
	{RX_PSC_CAL_LANE2,              0x0000},
	{RX_PSC_CAL_LANE3,              0x0000},
	{XCVR_DIAG_BIDI_CTRL_LANE0,     0x000f},
	{XCVR_DIAG_BIDI_CTRL_LANE1,     0x000f},
	{XCVR_DIAG_BIDI_CTRL_LANE2,     0x000f},
	{XCVR_DIAG_BIDI_CTRL_LANE3,     0x000f},
	{RX_REE_GCSM1_CTRL_LANE0,       0x0000},
	{RX_REE_GCSM1_CTRL_LANE1,       0x0000},
	{RX_REE_GCSM1_CTRL_LANE2,       0x0000},
	{RX_REE_GCSM1_CTRL_LANE3,       0x0000},
	{RX_REE_GCSM2_CTRL_LANE0,       0x0000},
	{RX_REE_GCSM2_CTRL_LANE1,       0x0000},
	{RX_REE_GCSM2_CTRL_LANE2,       0x0000},
	{RX_REE_GCSM2_CTRL_LANE3,       0x0000},
	{RX_REE_PERGCSM_CTRL_LANE0,     0x0000},
	{RX_REE_PERGCSM_CTRL_LANE1,     0x0000},
	{RX_REE_PERGCSM_CTRL_LANE2,     0x0000},
	{RX_REE_PERGCSM_CTRL_LANE3,     0x0000},
	{CMN_DIAG_GPANA_0,              0x0100},
};

static const struct reg_sequence sky1_udphy_dp_8p1_conf[] = {
	{CMN_PDIAG_PLL1_ITRIM_M0,       0x007f},

	{CMN_PLL1_SS_CTRL2_M0,          0x0430},
	{CMN_PLL1_SS_CTRL3_M0,          0x0062},
	{CMN_PLL1_SS_CTRL3_M0,          0x0004},
};

static const struct reg_sequence sky1_udphy_dp_partial_conf[] = {
	{CMN_PDIAG_PLL1_ITRIM_M0,       0x003f},

	{CMN_PLL1_SS_CTRL2_M0,          0x044F},
	{CMN_PLL1_SS_CTRL3_M0,          0x007F},
	{CMN_PLL1_SS_CTRL4_M0,          0x0003},
};

static const struct reg_sequence sky1_udphy_usbdp_conf_flip[] = {
	/* common_part */
	{CMN_SSM_BIAS_TMR,             0x0018},
	{CMN_PLLSM0_PLLPRE_TMR,        0x0030},
	{CMN_PLLSM0_PLLLOCK_TMR,       0x00F0},
	{CMN_PLLSM1_PLLPRE_TMR,        0x0030},
	{CMN_PLLSM1_PLLLOCK_TMR,       0x00F0},
	{CMN_BGCAL_INIT_TMR,           0x0078},
	{CMN_BGCAL_ITER_TMR,           0x0078},
	{CMN_IBCAL_INIT_TMR,           0x0018},
	{CMN_TXPUCAL_INIT_TMR,         0x001D},
	{CMN_TXPDCAL_INIT_TMR,         0x001D},
	{CMN_RXCAL_INIT_TMR,           0x02D0},
	{CMN_SD_CAL_PLLCNT_START,      0x0137},
	{RX_SDCAL0_INIT_TMR_LANE0,     0x0018},
	{RX_SDCAL0_INIT_TMR_LANE1,     0x0018},
	{RX_SDCAL0_INIT_TMR_LANE2,     0x0018},
	{RX_SDCAL0_INIT_TMR_LANE3,     0x0018},
	{RX_SDCAL0_ITER_TMR_LANE0,     0x0078},
	{RX_SDCAL0_ITER_TMR_LANE1,     0x0078},
	{RX_SDCAL0_ITER_TMR_LANE2,     0x0078},
	{RX_SDCAL0_ITER_TMR_LANE3,     0x0078},
	{RX_SDCAL1_INIT_TMR_LANE0,     0x0018},
	{RX_SDCAL1_INIT_TMR_LANE1,     0x0018},
	{RX_SDCAL1_INIT_TMR_LANE2,     0x0018},
	{RX_SDCAL1_INIT_TMR_LANE3,     0x0018},
	{RX_SDCAL1_ITER_TMR_LANE0,     0x0078},
	{RX_SDCAL1_ITER_TMR_LANE1,     0x0078},
	{RX_SDCAL1_ITER_TMR_LANE2,     0x0078},
	{RX_SDCAL1_ITER_TMR_LANE3,     0x0078},
	{TX_RCVDET_ST_TMR_LANE0,       0x0960},
	{TX_RCVDET_ST_TMR_LANE1,       0x0960},
	{TX_RCVDET_ST_TMR_LANE2,       0x0960},
	{TX_RCVDET_ST_TMR_LANE3,       0x0960},
	{CMN_PDIAG_PLL0_CLK_SEL_M0,    0x8600},
	{CMN_PDIAG_PLL1_CLK_SEL_M0,    0x0601},
	{XCVR_DIAG_PLLDRC_CTRL_LANE2,  0x0041},
	{XCVR_DIAG_PLLDRC_CTRL_LANE3,  0x0041},
	{XCVR_DIAG_HSCLK_SEL_LANE0,    0x0001},
	{XCVR_DIAG_HSCLK_SEL_LANE1,    0x0001},
	{XCVR_DIAG_HSCLK_DIV_LANE0,    0x0000},
	{XCVR_DIAG_HSCLK_DIV_LANE1,    0x0000},
	{XCVR_DIAG_PLLDRC_CTRL_LANE0,  0x0019},
	{XCVR_DIAG_PLLDRC_CTRL_LANE1,  0x0019},
	{CMN_PLL0_DSM_DIAG_M0,         0x0004},
	{CMN_PLL1_DSM_DIAG_M0,         0x8004},
	{CMN_PDIAG_PLL1_ITRIM_M0,      0x003f},
	{CMN_PDIAG_PLL0_CP_PADJ_M0,    0x0B17},
	{CMN_PDIAG_PLL1_CP_PADJ_M0,    0x0B17},
	{CMN_PDIAG_PLL0_CP_IADJ_M0,    0x0E01},
	{CMN_PDIAG_PLL1_CP_IADJ_M0,    0x0E01},
	{CMN_PDIAG_PLL0_FILT_PADJ_M0,  0x0D05},
	{CMN_PDIAG_PLL1_FILT_PADJ_M0,  0x0D05},

	{CMN_PLL0_INTDIV_M0,           0x01A0},
	{CMN_PLL1_INTDIV_M0,           0x01C2},
	{CMN_PLL0_FRACDIVL_M0,         0xAAAB},

	{CMN_PLL1_FRACDIVL_M0,         0x0000}, /* reset value */

	{CMN_PLL0_FRACDIVH_M0,         0x0002},
	{CMN_PLL1_FRACDIVH_M0,         0x0002},
	{CMN_PLL0_HIGH_THR_M0,         0x0116},
	{CMN_PLL1_HIGH_THR_M0,         0x012C},
	{CMN_PDIAG_PLL0_CTRL_M0,       0x1002},
	{CMN_PDIAG_PLL1_CTRL_M0,       0x1002},
	{CMN_PLL0_SS_CTRL1_M0,         0x0001},
	{CMN_PLL1_SS_CTRL1_M0,         0x0001},
	{CMN_PLL0_SS_CTRL2_M0,         0x0416},
	{CMN_PLL1_SS_CTRL2_M0,         0x044F},
	{CMN_PLL0_SS_CTRL3_M0,         0x007C},
	{CMN_PLL1_SS_CTRL3_M0,         0x007F},
	{CMN_PLL0_SS_CTRL4_M0,         0x0003},
	{CMN_PLL1_SS_CTRL4_M0,         0x0003},
	{CMN_PLL0_VCOCAL_INIT_TMR,     0x00F0},
	{CMN_PLL1_VCOCAL_INIT_TMR,     0x00F0},
	{CMN_PLL0_VCOCAL_ITER_TMR,     0x0004},
	{CMN_PLL1_VCOCAL_ITER_TMR,     0x0004},
	{CMN_PLL0_VCOCAL_REFTIM_START, 0x02F8},
	{CMN_PLL1_VCOCAL_REFTIM_START, 0x02F8},
	{CMN_PLL0_VCOCAL_PLLCNT_START, 0x02F6},
	{CMN_PLL1_VCOCAL_PLLCNT_START, 0x02F6},
	{CMN_PLL0_VCOCAL_TCTRL,        0x0003},
	{CMN_PLL1_VCOCAL_TCTRL,        0x0003},
	{CMN_PLL0_LOCK_REFCNT_START,   0x00BF},
	{CMN_PLL1_LOCK_REFCNT_START,   0x00BF},
	{CMN_PLL0_LOCK_PLLCNT_START,   0x00BF},
	{CMN_PLL1_LOCK_PLLCNT_START,   0x00BF},
	{CMN_PLL0_LOCK_PLLCNT_THR,     0x0005},
	{CMN_PLL1_LOCK_PLLCNT_THR,     0x0005},
	/* usb_part */
	{PHY_PMA_LANE_MAP,             0x5100},
	{PHY_LANE_OFF_CTRL,            0x0100},
	{PHY_PIPE_USB3_GEN2_PRE_CFG0,  0x0A0A},
	{PHY_PIPE_USB3_GEN2_POST_CFG0, 0x1000},
	{PHY_PIPE_USB3_GEN2_POST_CFG1, 0x0010},
	{CMN_CDIAG_CDB_PWRI_OVRD,      0x8200},
	{CMN_CDIAG_XCVRC_PWRI_OVRD,    0x8200},
	{TX_PSC_A0_LANE3,              0x02FF},
	{TX_PSC_A1_LANE3,              0x06AF},
	{TX_PSC_A2_LANE3,              0x06AE},
	{TX_PSC_A3_LANE3,              0x06AE},
	{RX_PSC_A0_LANE2,              0x0D1D},
	{RX_PSC_A1_LANE2,              0x0D1D},
	{RX_PSC_A2_LANE2,              0x0D00},
	{RX_PSC_A3_LANE2,              0x0500},
	{TX_TXCC_CTRL_LANE3,           0x2A82},
	{TX_TXCC_CPOST_MULT_01_LANE3,  0x0014},
	{TX_TXCC_MGNFS_MULT_000_LANE3, 0x0007},
	{RX_SIGDET_HL_FILT_TMR_LANE2,  0x0013},
	{RX_REE_GCSM1_CTRL_LANE2,      0x0000},
	{RX_REE_ATTEN_THR_LANE2,       0x0C02},
	{RX_REE_SMGM_CTRL1_LANE2,      0x0330},
	{RX_REE_SMGM_CTRL2_LANE2,      0x0300},
	{XCVR_DIAG_PSC_OVRD_LANE2,     0x0003},
	{RX_DIAG_SIGDET_TUNE_LANE2,    0x1004},
	{RX_DIAG_NQST_CTRL_LANE2,      0x00F9},
	{RX_DIAG_DFE_AMP_TUNE_2_LANE2, 0x0C01},
	{RX_DIAG_DFE_AMP_TUNE_3_LANE2, 0x0002},
	{RX_DIAG_PI_CAP_LANE2,         0x0000},
	{RX_DIAG_PI_RATE_LANE2,        0x0031},
	{RX_DIAG_ACYA_LANE3,           0x0002},
	{RX_CDRLF_CNFG_LANE2,          0x018C},
	{RX_CDRLF_CNFG3_LANE2,         0x0003},
	/* usb adding to upspeed */
	{XCVR_DIAG_BIDI_CTRL_LANE2,    0x00f0},  /* New add */
	{XCVR_DIAG_BIDI_CTRL_LANE3,    0x000f},  /* New add */

	/* dp_part */
	{PHY_PMA_ISO_PLL_CTRL0,        0x000B},
	{PHY_PMA_ISO_PLL_CTRL1,        0x2224},

	/* lane0, lane1 */
	{TX_PSC_A0_LANE0,              0x00FB},
	{TX_PSC_A0_LANE1,              0x00FB},
	{TX_PSC_A2_LANE0,              0x04AA},
	{TX_PSC_A2_LANE1,              0x04AA},
	{TX_PSC_A3_LANE0,              0x04AA},
	{TX_PSC_A3_LANE1,              0x04AA},
	{RX_PSC_A0_LANE0,              0x0000},
	{RX_PSC_A0_LANE1,              0x0000},
	{RX_PSC_A2_LANE0,              0x0000},
	{RX_PSC_A2_LANE1,              0x0000},
	{RX_PSC_A3_LANE0,              0x0000},
	{RX_PSC_A3_LANE1,              0x0000},
	{RX_PSC_CAL_LANE0,             0x0000},
	{RX_PSC_CAL_LANE1,             0x0000},
	{XCVR_DIAG_BIDI_CTRL_LANE0,    0x000F},
	{XCVR_DIAG_BIDI_CTRL_LANE1,    0x000F},
	{RX_REE_GCSM1_CTRL_LANE0,      0x0000},
	{RX_REE_GCSM1_CTRL_LANE1,      0x0000},
	{RX_REE_GCSM2_CTRL_LANE0,      0x0000},
	{RX_REE_GCSM2_CTRL_LANE1,      0x0000},
	{RX_REE_PERGCSM_CTRL_LANE0,    0x0000},
	{RX_REE_PERGCSM_CTRL_LANE1,    0x0000},

	{CMN_DIAG_GPANA_0,             0x100}, /* New add */
};

static const struct cix_udphy_con_dir sky1_udphy_con_dir[] = {
	{0x420, 0},
	{0x420, 1},
	{0x420, 2},
	{0x420, 3}
};

struct cix_udphy {
	struct device *dev;
	void __iomem *base;
	struct regmap *phy_regmap;
	struct regmap *usbphy_syscon;
	struct reset_control *preset;
	struct reset_control *reset;
	//struct clk *ref_clk;
	struct clk *apb_clk;
	struct typec_switch_dev *sw;
	struct typec_mux_dev *mux;
	struct mutex mutex; /* mutex to protect access to individual PHYs */
	bool flip;
	bool dp_4k120;
	u8 cur_mode;
	u8 next_mode;
	int id;
	const struct cix_udphy_cfg *cfg;
	struct phy* dp_phy;
	const char *phy_status;
	bool phy_reset;
	int phy_init_skip_count;
};

//----------------------------------------------------------------------------
//	link rate change
//----------------------------------------------------------------------------
typedef struct {
	u32 hsclk_div;
	u32 clk_sel;
	u32 pll_intdiv;
	u32 pll_fracdivl;
	u32 pll_high_thr;
} phy_link_cfg_t;

static const phy_link_cfg_t link_cfg[] = {
	{ 0x0002, 0x0f01, 0x0195, 0x0000, 0x010e },  /* 1.62gbps */
	{ 0x0001, 0x0701, 0x01c2, 0x0000, 0x012c },  /* 2.70gbps */
	{ 0x0000, 0x0301, 0x01c2, 0x0000, 0x012c },  /* 5.40gbps */
	{ 0x0000, 0x0200, 0x0151, 0x8000, 0x00e2 },  /* 8.10gbps */
};

typedef struct {
	u32 tx_txcc_ctrl;
	u32 drv_diag_tx_drv;
	u32 tx_txcc_mgnfc_mult_000;
	u32 tx_txcc_cpost_mult_00;
} phy_voltage_conf_t;

static const phy_voltage_conf_t volt_cfg[] = {
	{ 0x08A4, 0x0003, 0x0028, 0x0000 }, // swing level == 0
	{ 0x08A4, 0x0003, 0x001f, 0x0014 },
	{ 0x08A4, 0x0003, 0x0012, 0x0020 },
	{ 0x08A4, 0x0003, 0x0000, 0x0027 },
	{ 0x08A4, 0x0003, 0x001f, 0x0000 }, // swing level == 1
	{ 0x08A4, 0x0003, 0x0013, 0x0012 },
	{ 0x08A4, 0x0003, 0x0000, 0x001f },
	{ 0x08A4, 0x0003, 0x0000, 0x001f },
	{ 0x08A4, 0x0003, 0x0013, 0x0000 }, // swing level == 2
	{ 0x08A4, 0x0003, 0x0000, 0x0013 },
	{ 0x08A4, 0x0003, 0x0000, 0x0013 },
	{ 0x08A4, 0x0003, 0x0000, 0x0013 },
	{ 0x08A4, 0x0003, 0x0000, 0x0000 }, // swing level == 3
	{ 0x08A4, 0x0003, 0x0000, 0x0000 },
	{ 0x08A4, 0x0003, 0x0000, 0x0000 },
	{ 0x08A4, 0x0003, 0x0000, 0x0000 },
};

static int udphy_dplane_get(struct cix_udphy *udphy)
{
	int dp_lanes;

	switch (udphy->next_mode) {
	case UDPHY_MODE_DP:
		dp_lanes = 4;
		break;
	case UDPHY_MODE_DP_USB:
		dp_lanes = 2;
		break;
	case UDPHY_MODE_USB:
		fallthrough;
	default:
		dp_lanes = 0;
		break;
	}

	return dp_lanes;
}

static int udphy_status_check(struct cix_udphy *udphy)
{
	//TODO check the phy status
	return 0;
}

static int sky1_udphy_exit(struct cix_udphy *udphy)
{
	dev_dbg(udphy->dev, "sky1_udphy_exit\n");
	reset_control_assert(udphy->reset);
	reset_control_assert(udphy->preset);
	clk_disable_unprepare(udphy->apb_clk);
	return 0;
}

static int sky1_udphy_init(struct cix_udphy *udphy)
{
	int ret;
	u32 value;

	regmap_update_bits(udphy->usbphy_syscon, udphy->cfg->con_dir[udphy->id].offset,
		GENMASK(udphy->cfg->con_dir[udphy->id].bit,udphy->cfg->con_dir[udphy->id].bit),
		(udphy->flip << udphy->cfg->con_dir[udphy->id].bit));

	udelay(10);

	regmap_read(udphy->usbphy_syscon, udphy->cfg->con_dir[udphy->id].offset, &value);
	dev_info(udphy->dev, "sky1_udphy_init: typec dir= 0x%x\n", value);

	//usb rcsu reset is default deassert
	reset_control_assert(udphy->reset);
	reset_control_assert(udphy->preset);

	ret = clk_prepare_enable(udphy->apb_clk);
	if (ret) {
		dev_err(udphy->dev, "Failed to prepare_enable udphy apb clock\n");
		return ret;
	}

	reset_control_deassert(udphy->preset);

	if (udphy->next_mode == UDPHY_MODE_DP) {
		dev_info(udphy->dev, "sky1_udphy_init: sky1_udphy_dp_conf\n");
		ret = regmap_multi_reg_write(udphy->phy_regmap, sky1_udphy_dp_conf,
						ARRAY_SIZE(sky1_udphy_dp_conf));

		if (ret) {
			dev_err(udphy->dev, "Failed to write the reg sequence\n");
			goto assert_apb;
		}
	} else {
		if (!udphy->flip) {
			ret = regmap_multi_reg_write(udphy->phy_regmap, sky1_udphy_usbdp_conf_normal,
							ARRAY_SIZE(sky1_udphy_usbdp_conf_normal));
			if (ret) {
				dev_err(udphy->dev, "Failed to write the reg sequence\n");
				goto assert_apb;
			}
		} else {
			ret = regmap_multi_reg_write(udphy->phy_regmap, sky1_udphy_usbdp_conf_flip,
							ARRAY_SIZE(sky1_udphy_usbdp_conf_flip));
			if (ret) {
				dev_err(udphy->dev, "Failed to write the reg sequence\n");
				goto assert_apb;
			}
		}
	}

	reset_control_deassert(udphy->reset);

	ret = udphy_status_check(udphy);

	if (ret) {
		dev_err(udphy->dev, "phy status not ready\n");
		goto assert_phy;
	}

	return 0;

assert_phy:
	reset_control_assert(udphy->reset);

assert_apb:
	reset_control_assert(udphy->preset);
	clk_disable_unprepare(udphy->apb_clk);
	return 0;
}

static const struct cix_udphy_cfg sky1_udphy_cfg = {
	.con_dir = sky1_udphy_con_dir,
	.udphy_init = sky1_udphy_init,
	.udphy_exit = sky1_udphy_exit
};

static int udphy_regmap_write(void *context, unsigned int reg, unsigned int val)
{
#if (!IS_ENABLED(CONFIG_ARCH_CIX_EMU_FPGA))
	struct cix_udphy *udphy = context;
	u32 offset = reg << 2;

	writel(val, udphy->base + offset);
#endif

	return 0;
}

static int udphy_regmap_read(void *context, unsigned int reg, unsigned int *val)
{
#if (!IS_ENABLED(CONFIG_ARCH_CIX_EMU_FPGA))
	struct cix_udphy *udphy = context;
	u32 offset = reg << 2;

	*val = readl(udphy->base + offset);
#endif

	return 0;
}


static const struct regmap_config cix_udphy_regmap_cfg = {
	.reg_bits = 32,
	.reg_stride = 1,//register width, if =2, then only reg 0 2 4 ...2^n can access
	.val_bits = 16,
	.fast_io = true,
	.reg_write = udphy_regmap_write,
	.reg_read = udphy_regmap_read,
};

static int udphy_orien_sw_set(struct typec_switch_dev *sw,
			      enum typec_orientation orien)
{
	struct cix_udphy *udphy = typec_switch_get_drvdata(sw);

	mutex_lock(&udphy->mutex);

	if (orien == TYPEC_ORIENTATION_NONE) {
		goto unlock_ret;
	}

	dev_info(udphy->dev, "typec orientation switch: %d\n", orien);
	udphy->flip = (orien == TYPEC_ORIENTATION_REVERSE) ? true : false;

unlock_ret:
	mutex_unlock(&udphy->mutex);
	return 0;
}

static int udphy_setup_orien_switch(struct cix_udphy *udphy)
{
	struct typec_switch_desc sw_desc = { };

	sw_desc.drvdata = udphy;
	sw_desc.fwnode = dev_fwnode(udphy->dev);
	sw_desc.set = udphy_orien_sw_set;

	udphy->sw = typec_switch_register(udphy->dev, &sw_desc);
	if (IS_ERR(udphy->sw)) {
		dev_err(udphy->dev, "Error register typec orientation switch: %ld\n",
			PTR_ERR(udphy->sw));
		return PTR_ERR(udphy->sw);
	}

	return 0;
}

static void usbdp_set_dpphy_mode(struct cix_udphy *udphy)
{
	if (NULL != udphy->dp_phy) {
		if(udphy->next_mode == UDPHY_MODE_DP || udphy->next_mode == UDPHY_MODE_DP_USB) {
			udphy->dp_phy->attrs.mode = PHY_MODE_DP;
			dev_info(udphy->dev, "set dp phy mode: %d\n", udphy->dp_phy->attrs.mode);
		} else {
			udphy->dp_phy->attrs.mode = PHY_MODE_INVALID;
		}
	}
}

static int usbdp_typec_mux_set(struct typec_mux_dev *mux,
			       struct typec_mux_state *state)
{
	struct cix_udphy *udphy = typec_mux_get_drvdata(mux);


	mutex_lock(&udphy->mutex);

	dev_info(udphy->dev, "usbdp_typec_mux_set mode: %ld\n",
		state->mode);

	switch(state->mode) {
	case TYPEC_DP_STATE_C:
		fallthrough;
	case TYPEC_DP_STATE_E:
		udphy->next_mode = UDPHY_MODE_DP;
		break;
	case TYPEC_DP_STATE_D:
		udphy->next_mode = UDPHY_MODE_DP_USB;
		break;
	case TYPEC_STATE_SAFE:
		udphy->next_mode = UDPHY_MODE_USB;
		break;
	default:
		//usb only handle as usb+dp mode
		udphy->next_mode = UDPHY_MODE_USB;
		break;
	}

	usbdp_set_dpphy_mode(udphy);

	//TODO check whether the HPD state need software
	mutex_unlock(&udphy->mutex);

	return 0;
}
static void udphy_orien_switch_unregister(void *data)
{
	struct cix_udphy *udphy = data;

	typec_switch_unregister(udphy->sw);
}

static int udphy_setup_typec_mux(struct cix_udphy *udphy)
{
	struct typec_mux_desc mux_desc = {};

	mux_desc.drvdata = udphy;
	mux_desc.fwnode = dev_fwnode(udphy->dev);
	mux_desc.set = usbdp_typec_mux_set;

	udphy->mux = typec_mux_register(udphy->dev, &mux_desc);
	if (IS_ERR(udphy->mux)) {
		dev_err(udphy->dev, "Error register typec mux: %ld\n",
			PTR_ERR(udphy->mux));
		return PTR_ERR(udphy->mux);
	}

	return 0;
}

static void udphy_typec_mux_unregister(void *data)
{
	struct cix_udphy *udphy = data;

	typec_mux_unregister(udphy->mux);
}

static int cix_usbdp_phy_power_on(struct cix_udphy *udphy)
{
	int ret;

	const struct cix_udphy_cfg *phy_cfgs = udphy->cfg;

	if (udphy->cur_mode == udphy->next_mode) {
		return 0;
	}

	if (((udphy->cur_mode == UDPHY_MODE_USB) && (udphy->next_mode == UDPHY_MODE_DP_USB)) ||
		((udphy->cur_mode == UDPHY_MODE_DP) && (udphy->next_mode == UDPHY_MODE_DP_USB))) {
		dev_info(udphy->dev, "no need reconfig\n");
		return 0;
	}

	if (phy_cfgs->udphy_init) {
		ret = phy_cfgs->udphy_init(udphy);

		if(ret) {
			dev_err(udphy->dev, "failed to init udphy\n");
			return ret;
		}
	}

	return 0;
}

static int cix_usbdp_phy_power_off(struct cix_udphy *udphy)
{
	int ret;
	const struct cix_udphy_cfg *phy_cfgs = udphy->cfg;

	if (phy_cfgs->udphy_exit) {
		ret = phy_cfgs->udphy_exit(udphy);

		if(ret) {
			dev_err(udphy->dev, "failed to exit udphy\n");
			return ret;
		}
	}

	udphy->cur_mode = UDPHY_MODE_NONE;
	return 0;
}

static int udphy_dp_phy_power_on(struct phy *phy)
{
	struct cix_udphy *udphy = phy_get_drvdata(phy);
	int ret = 0;

	mutex_lock(&udphy->mutex);
	if (!(udphy->next_mode & UDPHY_MODE_DP)) {
		goto unlock;
	}

	/* It is only for GOP, dp+usb3.0 hub should skip the initialization from dp driver*/
	if (udphy->phy_init_skip_count) {
		dev_info(udphy->dev, "dp skip phy init ,mode:%d\n", udphy->next_mode);
		clk_prepare_enable(udphy->apb_clk); //add for clk disable
		udphy->phy_init_skip_count--;
		goto set_cur;
	}
	ret = cix_usbdp_phy_power_on(udphy);
	if (!ret) {
		phy_set_bus_width(phy, udphy_dplane_get(udphy));
	}

set_cur:
	if (udphy->next_mode == UDPHY_MODE_DP)
		udphy->cur_mode = UDPHY_MODE_DP;
	else
		udphy->cur_mode |= UDPHY_MODE_DP;

unlock:
	mutex_unlock(&udphy->mutex);
	return ret;
}

static int udphy_dp_phy_power_off(struct phy *phy)
{
	struct cix_udphy *udphy = phy_get_drvdata(phy);
	int ret = 0;

	mutex_lock(&udphy->mutex);

	if (!(udphy->cur_mode & UDPHY_MODE_DP))
		goto unlock;
	udphy->cur_mode &= ~UDPHY_MODE_DP;

	if (udphy->cur_mode == UDPHY_MODE_NONE) {
		ret = cix_usbdp_phy_power_off(udphy);
	}

	phy_set_bus_width(phy, 0);

unlock:
	mutex_unlock(&udphy->mutex);
	return ret;
}


static int udphy_dp_phy_verify_config(struct cix_udphy *udphy,
					struct phy_configure_opts_dp *dp)
{
	int i;

	switch (dp->lanes) {
	case 1:
	case 2:
	case 4:
		break;
	default:
		return -EINVAL;
	}

	/*
	 * If changing voltages is required, check swing and pre-emphasis
	 * levels, per-lane.
	 */
	if (dp->set_voltages) {
		for (i = 0; i < dp->lanes; i++) {
			if (dp->voltage[i] > 3 || dp->pre[i] > 3)
				return -EINVAL;

			/*
			 * Sum of voltage swing and pre-emphasis levels cannot
			 * exceed 3.
			 */
			if (dp->voltage[i] + dp->pre[i] > 3)
				return -EINVAL;
		}
	}

	return 0;
}

static int udphy_dp_phy_set_rate(struct cix_udphy *udphy,
				 struct phy_configure_opts_dp *dp)
{
	int ret;
	const phy_link_cfg_t *conf = NULL;

	dev_dbg(udphy->dev, "%s: lane %d, rate %x\n", __func__, dp->lanes, dp->link_rate);

	regmap_update_bits(udphy->phy_regmap, PHY_PMA_PLL_CTRL, BIT(1), 0x0);
	udelay(10);

	if (dp->link_rate == 0x1e) {
		ret = regmap_multi_reg_write(udphy->phy_regmap, sky1_udphy_dp_8p1_conf,
						ARRAY_SIZE(sky1_udphy_dp_8p1_conf));
	} else {
		ret = regmap_multi_reg_write(udphy->phy_regmap, sky1_udphy_dp_partial_conf,
						ARRAY_SIZE(sky1_udphy_dp_partial_conf));
	}

	switch (dp->link_rate) {
	case 0x6:  conf = &link_cfg[0]; break;
	case 0xa:  conf = &link_cfg[1]; break;
	case 0x14: conf = &link_cfg[2]; break;
	case 0x1e: conf = &link_cfg[3]; break;
	default:
		return -EINVAL;
	}

	switch (dp->lanes) {
	case 1:
		if (!udphy->flip) {
			regmap_write(udphy->phy_regmap,
					XCVR_DIAG_HSCLK_DIV_LANE2,
					conf->hsclk_div);
		} else {
			regmap_write(udphy->phy_regmap,
					XCVR_DIAG_HSCLK_DIV_LANE1,
					conf->hsclk_div);
		}
		break;
	case 2:
		if (!udphy->flip) {
			regmap_write(udphy->phy_regmap,
					XCVR_DIAG_HSCLK_DIV_LANE2,
					conf->hsclk_div);
			regmap_write(udphy->phy_regmap,
					XCVR_DIAG_HSCLK_DIV_LANE3,
					conf->hsclk_div);
		} else {
			regmap_write(udphy->phy_regmap,
					XCVR_DIAG_HSCLK_DIV_LANE0,
					conf->hsclk_div);
			regmap_write(udphy->phy_regmap,
					XCVR_DIAG_HSCLK_DIV_LANE1,
					conf->hsclk_div);
		}
		break;
	case 4:
		regmap_write(udphy->phy_regmap,
				XCVR_DIAG_HSCLK_DIV_LANE0,
				conf->hsclk_div);
		regmap_write(udphy->phy_regmap,
				XCVR_DIAG_HSCLK_DIV_LANE1,
				conf->hsclk_div);
		regmap_write(udphy->phy_regmap,
				XCVR_DIAG_HSCLK_DIV_LANE2,
				conf->hsclk_div);
		regmap_write(udphy->phy_regmap,
				XCVR_DIAG_HSCLK_DIV_LANE3,
				conf->hsclk_div);
		break;
	default:
		break;
	}

	regmap_write(udphy->phy_regmap, CMN_PLL1_INTDIV_M0, conf->pll_intdiv);
	regmap_write(udphy->phy_regmap, CMN_PLL1_FRACDIVL_M0, conf->pll_fracdivl);
	regmap_write(udphy->phy_regmap, CMN_PLL1_HIGH_THR_M0, conf->pll_high_thr);
	regmap_write(udphy->phy_regmap, CMN_PDIAG_PLL1_CLK_SEL_M0, conf->clk_sel);

	regmap_update_bits(udphy->phy_regmap, PHY_PMA_PLL_CTRL, BIT(1), BIT(1));
	udelay(10);

	return 0;
}

static int udphy_dp_phy_set_voltages(struct cix_udphy *udphy,
				      struct phy_configure_opts_dp *dp)
{
	u32 index = 0;
	const phy_voltage_conf_t *cfg = volt_cfg;

	dev_dbg(udphy->dev, "udphy_dp_phy_set_voltages: lane %d, vs %d, pe %d\n",
			dp->lanes, dp->voltage[0], dp->pre[0]);

	index = dp->voltage[0] * 4 + dp->pre[0];
	switch (dp->lanes) {
	case 1:
		if (!udphy->flip) { // lane 2
			regmap_update_bits(udphy->phy_regmap, TX_DIAG_ACYA_LANE2,
						BIT(0), 0x1);
			regmap_write(udphy->phy_regmap, TX_TXCC_CTRL_LANE2,
						cfg[index].tx_txcc_ctrl);
			regmap_write(udphy->phy_regmap, DRV_DIAG_TX_DRV_LANE2,
						cfg[index].drv_diag_tx_drv);
			regmap_write(udphy->phy_regmap, TX_TXCC_MGNFS_MULT_000_LANE2,
						cfg[index].tx_txcc_mgnfc_mult_000);
			regmap_write(udphy->phy_regmap, TX_TXCC_CPOST_MULT_00_LANE2,
						cfg[index].tx_txcc_cpost_mult_00);
			regmap_update_bits(udphy->phy_regmap, TX_DIAG_ACYA_LANE2,
						BIT(0), 0x0);
		} else { // lane 1
			regmap_update_bits(udphy->phy_regmap, TX_DIAG_ACYA_LANE1,
						BIT(0), 0x1);
			regmap_write(udphy->phy_regmap, TX_TXCC_CTRL_LANE1,
						cfg[index].tx_txcc_ctrl);
			regmap_write(udphy->phy_regmap, DRV_DIAG_TX_DRV_LANE1,
						cfg[index].drv_diag_tx_drv);
			regmap_write(udphy->phy_regmap, TX_TXCC_MGNFS_MULT_000_LANE1,
						cfg[index].tx_txcc_mgnfc_mult_000);
			regmap_write(udphy->phy_regmap, TX_TXCC_CPOST_MULT_00_LANE1,
						cfg[index].tx_txcc_cpost_mult_00);
			regmap_update_bits(udphy->phy_regmap, TX_DIAG_ACYA_LANE1,
						BIT(0), 0x0);
		}
		break;
	case 2:
		if (!udphy->flip) {
			regmap_update_bits(udphy->phy_regmap, TX_DIAG_ACYA_LANE2,
						BIT(0), 0x1);
			regmap_write(udphy->phy_regmap, TX_TXCC_CTRL_LANE2,
						cfg[index].tx_txcc_ctrl);
			regmap_write(udphy->phy_regmap, DRV_DIAG_TX_DRV_LANE2,
						cfg[index].drv_diag_tx_drv);
			regmap_write(udphy->phy_regmap, TX_TXCC_MGNFS_MULT_000_LANE2,
						cfg[index].tx_txcc_mgnfc_mult_000);
			regmap_write(udphy->phy_regmap, TX_TXCC_CPOST_MULT_00_LANE2,
						cfg[index].tx_txcc_cpost_mult_00);
			regmap_update_bits(udphy->phy_regmap, TX_DIAG_ACYA_LANE2,
						BIT(0), 0x0);
			regmap_update_bits(udphy->phy_regmap, TX_DIAG_ACYA_LANE3,
						BIT(0), 0x1);
			regmap_write(udphy->phy_regmap, TX_TXCC_CTRL_LANE3,
						cfg[index].tx_txcc_ctrl);
			regmap_write(udphy->phy_regmap, DRV_DIAG_TX_DRV_LANE3,
						cfg[index].drv_diag_tx_drv);
			regmap_write(udphy->phy_regmap, TX_TXCC_MGNFS_MULT_000_LANE3,
						cfg[index].tx_txcc_mgnfc_mult_000);
			regmap_write(udphy->phy_regmap, TX_TXCC_CPOST_MULT_00_LANE3,
						cfg[index].tx_txcc_cpost_mult_00);
			regmap_update_bits(udphy->phy_regmap, TX_DIAG_ACYA_LANE3,
						BIT(0), 0x0);
		} else {
			regmap_update_bits(udphy->phy_regmap, TX_DIAG_ACYA_LANE0,
						BIT(0), 0x1);
			regmap_write(udphy->phy_regmap, TX_TXCC_CTRL_LANE0,
						cfg[index].tx_txcc_ctrl);
			regmap_write(udphy->phy_regmap, DRV_DIAG_TX_DRV_LANE0,
						cfg[index].drv_diag_tx_drv);
			regmap_write(udphy->phy_regmap, TX_TXCC_MGNFS_MULT_000_LANE0,
						cfg[index].tx_txcc_mgnfc_mult_000);
			regmap_write(udphy->phy_regmap, TX_TXCC_CPOST_MULT_00_LANE0,
						cfg[index].tx_txcc_cpost_mult_00);
			regmap_update_bits(udphy->phy_regmap, TX_DIAG_ACYA_LANE0,
						BIT(0), 0x0);
			regmap_update_bits(udphy->phy_regmap, TX_DIAG_ACYA_LANE1,
						BIT(0), 0x1);
			regmap_write(udphy->phy_regmap, TX_TXCC_CTRL_LANE1,
						cfg[index].tx_txcc_ctrl);
			regmap_write(udphy->phy_regmap, DRV_DIAG_TX_DRV_LANE1,
						cfg[index].drv_diag_tx_drv);
			regmap_write(udphy->phy_regmap, TX_TXCC_MGNFS_MULT_000_LANE1,
						cfg[index].tx_txcc_mgnfc_mult_000);
			regmap_write(udphy->phy_regmap, TX_TXCC_CPOST_MULT_00_LANE1,
						cfg[index].tx_txcc_cpost_mult_00);
			regmap_update_bits(udphy->phy_regmap, TX_DIAG_ACYA_LANE1,
						BIT(0), 0x0);
		}
		break;
	case 4:
		regmap_update_bits(udphy->phy_regmap, TX_DIAG_ACYA_LANE0,
					BIT(0), 0x1);
		regmap_write(udphy->phy_regmap, TX_TXCC_CTRL_LANE0,
					cfg[index].tx_txcc_ctrl);
		regmap_write(udphy->phy_regmap, DRV_DIAG_TX_DRV_LANE0,
					cfg[index].drv_diag_tx_drv);
		regmap_write(udphy->phy_regmap, TX_TXCC_MGNFS_MULT_000_LANE0,
					cfg[index].tx_txcc_mgnfc_mult_000);
		regmap_write(udphy->phy_regmap, TX_TXCC_CPOST_MULT_00_LANE0,
					cfg[index].tx_txcc_cpost_mult_00);
		regmap_update_bits(udphy->phy_regmap, TX_DIAG_ACYA_LANE0,
					BIT(0), 0x0);

		regmap_update_bits(udphy->phy_regmap, TX_DIAG_ACYA_LANE1,
					BIT(0), 0x1);
		regmap_write(udphy->phy_regmap, TX_TXCC_CTRL_LANE1,
					cfg[index].tx_txcc_ctrl);
		regmap_write(udphy->phy_regmap, DRV_DIAG_TX_DRV_LANE1,
					cfg[index].drv_diag_tx_drv);
		regmap_write(udphy->phy_regmap, TX_TXCC_MGNFS_MULT_000_LANE1,
					cfg[index].tx_txcc_mgnfc_mult_000);
		regmap_write(udphy->phy_regmap, TX_TXCC_CPOST_MULT_00_LANE1,
					cfg[index].tx_txcc_cpost_mult_00);
		regmap_update_bits(udphy->phy_regmap, TX_DIAG_ACYA_LANE1,
					BIT(0), 0x0);

		regmap_update_bits(udphy->phy_regmap, TX_DIAG_ACYA_LANE2,
					BIT(0), 0x1);
		regmap_write(udphy->phy_regmap, TX_TXCC_CTRL_LANE2,
					cfg[index].tx_txcc_ctrl);
		regmap_write(udphy->phy_regmap, DRV_DIAG_TX_DRV_LANE2,
					cfg[index].drv_diag_tx_drv);
		regmap_write(udphy->phy_regmap, TX_TXCC_MGNFS_MULT_000_LANE2,
					cfg[index].tx_txcc_mgnfc_mult_000);
		regmap_write(udphy->phy_regmap, TX_TXCC_CPOST_MULT_00_LANE2,
					cfg[index].tx_txcc_cpost_mult_00);
		regmap_update_bits(udphy->phy_regmap, TX_DIAG_ACYA_LANE2,
					BIT(0), 0x0);

		regmap_update_bits(udphy->phy_regmap, TX_DIAG_ACYA_LANE3,
					BIT(0), 0x1);
		regmap_write(udphy->phy_regmap, TX_TXCC_CTRL_LANE3,
					cfg[index].tx_txcc_ctrl);
		regmap_write(udphy->phy_regmap, DRV_DIAG_TX_DRV_LANE3,
					cfg[index].drv_diag_tx_drv);
		regmap_write(udphy->phy_regmap, TX_TXCC_MGNFS_MULT_000_LANE3,
					cfg[index].tx_txcc_mgnfc_mult_000);
		regmap_write(udphy->phy_regmap, TX_TXCC_CPOST_MULT_00_LANE3,
					cfg[index].tx_txcc_cpost_mult_00);
		regmap_update_bits(udphy->phy_regmap, TX_DIAG_ACYA_LANE3,
					BIT(0), 0x0);
		break;
	default:
		break;
	}
	return 0;
}

static int udphy_dp_phy_configure (struct phy *phy,
				   union phy_configure_opts *opts)
{
	struct cix_udphy *udphy = phy_get_drvdata(phy);
	int ret;

	ret = udphy_dp_phy_verify_config(udphy, &opts->dp);
	if (ret)
		return ret;

	if (opts->dp.set_rate) {
		ret = udphy_dp_phy_set_rate(udphy, &opts->dp);
		if (ret) {
			dev_err(udphy->dev,
				"udphy_dp_phy_configure failed\n");
			return ret;
		}
	}

	if (opts->dp.set_voltages) {
		ret = udphy_dp_phy_set_voltages(udphy, &opts->dp);
		if (ret) {
			dev_err(udphy->dev,
				"dp_phy_set_voltages failed\n");
			return ret;
		}
	}

	dev_dbg(udphy->dev, "udphy_dp_phy_configure end\n");
	return 0;
}

static const struct phy_ops cix_dp_phy_ops = {
	.power_on	= udphy_dp_phy_power_on,
	.power_off	= udphy_dp_phy_power_off,
	.configure	= udphy_dp_phy_configure,
	.owner		= THIS_MODULE,
};

static int cix_u3phy_init(struct phy *phy)
{
	struct cix_udphy *udphy = phy_get_drvdata(phy);
	int ret = 0;

	mutex_lock(&udphy->mutex);
	if (!(udphy->next_mode & UDPHY_MODE_USB)) {
		goto unlock;
	}

	if (udphy->cur_mode & UDPHY_MODE_USB) {
		goto unlock;
	}

	/* It is only for GOP, dp+usb3.0 hub should skip the initialization from usb driver*/
	if (udphy->phy_init_skip_count) {
		dev_info(udphy->dev, "usb skip phy init ,mode:%d\n", udphy->next_mode);
		udphy->phy_init_skip_count--;
		clk_prepare_enable(udphy->apb_clk);
		goto set_cur;
	}
	ret = cix_usbdp_phy_power_on(udphy);

set_cur:
	udphy->cur_mode |= UDPHY_MODE_USB;

unlock:
	mutex_unlock(&udphy->mutex);
	return ret;
}

static int cix_u3phy_exit(struct phy *phy)
{
	struct cix_udphy *udphy = phy_get_drvdata(phy);
	int ret = 0;

	mutex_lock(&udphy->mutex);

	if (!(udphy->cur_mode & UDPHY_MODE_USB))
		goto unlock;

	udphy->cur_mode &= ~UDPHY_MODE_USB;

	if (udphy->cur_mode == UDPHY_MODE_NONE) {
		ret = cix_usbdp_phy_power_off(udphy);
	}

unlock:
	mutex_unlock(&udphy->mutex);
	return ret;
}

static int cix_u3phy_set_mode(struct phy *phy, enum phy_mode mode , int submode)
{
	int ret = 0;
	if (mode == PHY_MODE_INVALID) {
		ret = cix_u3phy_exit(phy);
	} else {
		ret = cix_u3phy_init(phy);
	}

	return ret;
}

static const struct phy_ops cix_u3phy_ops = {
	.set_mode       = cix_u3phy_set_mode,
	.exit		= cix_u3phy_exit,
	.owner		= THIS_MODULE,
};

/*
PHY lane to PMA lane Mapping for a Normal plug orientation

PMA LANE     C       D                 E
0          DP TX2   USB TX0          DP TX2
1          DP TX3   USB RX0          DP TX3
2          DP TX0   USB RX1/DP TX0   DP TX0
3          DP TX1   USB TX1/DP TX1   DP TX1

PHY lane to PMA lane Mapping for a Flipped plug orientation

PMA LANE     C       D                 E
0          DP TX1   USB TX1/DP TX1   DP TX1
1          DP TX0   USB RX1/DP TX0   DP TX0
2          DP TX3   USB RX0          DP TX3
3          DP TX2   USB TX0          DP TX2

*/
static int udphy_set_default_mode(struct cix_udphy *udphy)
{
	udphy->cur_mode = UDPHY_MODE_NONE;
	return 0;
}

static int cix_udphy_probe(struct platform_device *pdev)
{
	struct cix_udphy *udphy;
	struct device *dev = &pdev->dev;
	struct phy_provider *phy_provider;
	struct device_node *np = dev->of_node;
	struct device_node *child_np;
	struct fwnode_handle *child_fn;
	int id, ret = 0;
	struct gop_status __iomem *g_status;
	enum phy_role gop_value;

	udphy = devm_kzalloc(dev, sizeof(*udphy), GFP_KERNEL);
	if (!udphy)
		return -ENOMEM;

	dev_set_drvdata(dev, udphy);
	udphy->dev = dev;

	id = of_alias_get_id(dev->of_node, "usbdpphy");
	if (id < 0)
		if (device_property_read_u32(dev, "id", &id) < 0)
			id = 0;
	udphy->id = id;

	udphy->cfg = device_get_match_data(dev);
	if (!udphy->cfg) {
		dev_err(dev, "no OF data can be matched with %p node\n", np);
		return -EINVAL;
	}

	udphy->reset = devm_reset_control_get(dev, "reset");
	if (IS_ERR(udphy->reset)) {
		dev_err(dev, "%s: failed to get reset\n",
			dev->of_node->full_name);
	}

	udphy->preset = devm_reset_control_get(dev, "preset");
	if (IS_ERR(udphy->preset)) {
		dev_err(dev, "%s: failed to get preset\n",
			dev->of_node->full_name);
	}

	if (!device_property_read_u8(dev, "default_conf", &udphy->next_mode))
		dev_info(dev, "phy init default mode %02x\n", udphy->next_mode);

	/* It is only for GOP, dp+usb3.0 hub should not reset the phy and skip the
	 * initialization from usb and dp driver, dp only device should not reset the phy.
	 */
	udphy->phy_reset = true;
	udphy->phy_init_skip_count = 0;

	g_status = ioremap(GOP_STATUS_ADDRESS, GOP_STATUS_SIZE);
	if (g_status) {
		gop_value = readb(&g_status->phy_status[id]);
		switch(gop_value){
		case USB_ROLE_HOST:
			udphy->phy_reset = false;
			udphy->phy_init_skip_count = 2;
			break;
		case USB_ROLE_HOST_20:
			udphy->phy_reset = false;
			udphy->phy_init_skip_count = 0;
			break;
		case USB_ROLE_NONE:
		case USB_ROLE_DEVICE:
		default:
			udphy->phy_reset = true;
			udphy->phy_init_skip_count = 0;
			break;
		}
		iounmap(g_status);
	}

	dev_info(dev, "get phy-status:[%d]%d\n", id, gop_value);
	if (udphy->phy_reset) {
		dev_info(dev, "phy reset\n");
		reset_control_assert(udphy->reset);
		reset_control_assert(udphy->preset);
	}

	udphy->apb_clk = devm_clk_get(dev, "pclk");
	if (IS_ERR(udphy->apb_clk)) {
		dev_err(dev, "phy apb clock not found\n");
		return PTR_ERR(udphy->apb_clk);
	}

	udphy->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(udphy->base))
		return PTR_ERR(udphy->base);

	udphy->phy_regmap = devm_regmap_init(dev, NULL, udphy, &cix_udphy_regmap_cfg);
	if (IS_ERR(udphy->phy_regmap)) {
		dev_err(dev, "failed to remap phy register\n");
		return PTR_ERR(udphy->phy_regmap);
	}

	udphy->usbphy_syscon =
		device_syscon_regmap_lookup_by_property(&pdev->dev,
					"cix,usbphy_syscon");
	if (IS_ERR(udphy->usbphy_syscon )) {
		dev_err(dev, "failed get usbphy syscon\n");
		return PTR_ERR(udphy->usbphy_syscon);
	}

	mutex_init(&udphy->mutex);

	if (device_property_present(dev, "orientation-switch")) {
		ret = udphy_setup_orien_switch(udphy);
		if (ret)
			return ret;

		ret = devm_add_action_or_reset(dev, udphy_orien_switch_unregister, udphy);
		if (ret)
			return ret;
	}

	if (device_property_present(dev, "mode-switch")) {
		ret = udphy_setup_typec_mux(udphy);
		if (ret)
			return ret;

		ret = devm_add_action_or_reset(dev, udphy_typec_mux_unregister, udphy);
		if (ret)
			return ret;
	}

	udphy_set_default_mode(udphy);

	device_for_each_child_node(dev, child_fn) {
		struct phy *phy;

		child_np = to_of_node(child_fn);

		if (!strncmp(fwnode_get_name(child_fn), "dp-port", 7)
			|| !strncmp(fwnode_get_name(child_fn), "UDPP", 4)) {
			phy = devm_phy_create(dev, child_np, &cix_dp_phy_ops);
			udphy->dp_phy = phy;
		}
		else if (!strncmp(fwnode_get_name(child_fn), "usb-port", 8)
			|| !strncmp(fwnode_get_name(child_fn), "USB", 3))
			phy = devm_phy_create(dev, child_np, &cix_u3phy_ops);
		else
			continue;

		if (IS_ERR(phy)) {
			dev_err(dev, "failed to create phy: %s\n",
						fwnode_get_name(child_fn));
			goto put_child;
		}

		phy_set_drvdata(phy, udphy);

		phy_create_lookup(phy, fwnode_get_name(child_fn), dev_name(dev));
	}

	phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	if (IS_ERR(phy_provider)) {
		dev_err(dev, "failed to register phy provider\n");
		goto put_child;
	}
	if (!device_property_present(dev, "mode-switch"))
		usbdp_set_dpphy_mode(udphy);

	return 0;

put_child:
	of_node_put(child_np);
	return ret;
}
static const struct of_device_id cix_udphy_dt_match[] = {
	{
		.compatible = "cix,sky1-usbdp-phy",
		.data = &sky1_udphy_cfg
	},
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, cix_udphy_dt_match);

static const struct acpi_device_id cix_udphy_acpi_match[] = {
	{ "CIXH2033", (kernel_ulong_t)&sky1_udphy_cfg },
	{ },
};

MODULE_DEVICE_TABLE(acpi, cix_udphy_acpi_match);

static struct platform_driver cix_udphy_driver = {
	.probe		= cix_udphy_probe,
	.driver		= {
		.name	= "cix-usbdp-phy",
		.of_match_table = cix_udphy_dt_match,
		.acpi_match_table = cix_udphy_acpi_match,
		.pm = NULL,
	},
};

module_platform_driver(cix_udphy_driver);

MODULE_AUTHOR("Chao Zeng <chao.zeng@cixteck.com>");
MODULE_DESCRIPTION("Cix USBDP Combo PHY driver");
MODULE_LICENSE("GPL v2");
