/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2024 - 2027 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Driver for sunxi SD/MMC host controllers
* (C) Copyright 2024-2026 lixiang <lixiang@allwinnertech>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */



#ifndef _TC_DWC_H
#define _TC_DWC_H

struct tc_rate_cfg_params {
	u16 mplla_multiplier;
	u16 mplla_bandwidth;
	u16 rx0_vco_ld_val;
	u16 rx0_ref_ld_val;
	u16 rx1_vco_ld_val;
	u16 rx1_ref_ld_val;
};

/* C10 Registers */
#define TC_MPLL_COARSE_TUNE_VAL			0x0027

/* C10 Interface */
#define TC_CBC_REG_ADDR_LSB			0x8116
#define TC_CBC_REG_ADDR_MSB			0x8117
#define TC_CBC_REG_WR_LSB			0x8118
#define TC_CBC_REG_WR_MSB			0x8119
#define TC_CBC_REG_RD_LSB			0x811a
#define TC_CBC_REG_RD_MSB			0x811b
#define TC_CBC_REG_RD_WR_SEL			0x811c

/* Misc Helpers */
#define	TC_LSB(x)				(((x) >> 0) & 0xff)
#define	TC_MSB(x)				(((x) >> 8) & 0xff)

/*
 * Test Chip Attributes
 */

#define CBRATSEL			0x8114

/* PCS - SFR attributes */
#define SFR_ZCALL_OUT			0x810b
#define SFR_TX_WC_BYPASS		0x8180
#define SFR_RX_WC_BYPASS		0x8181
#define SFR_REF_CLK_SEL                 0x8184
#define SFR_ZCALL			0x8185
#define SFR_CFG_CLK                     0x8189
#define SFR_IDELAY_RX0_DATA		0x818c
#define SFR_IDELAY_TX0_SYMBOL		0x818e
#define SFR_IDELAY_RX1_DATA			0x818d
#define SFR_IDELAY_TX1_SYMBOL		0x818f
#define SFR_ODELAY_TX0_DATA_0		0x819a
#define SFR_ODELAY_TX1_DATA_0		0x819c
#define SFR_IDELAYCTRL_RST		0x819e
#define SFR_ODELAY_TX0_VAL_IN		0x819f

#define SFR_IDELAY_TX0_CLK		0x818a
#define SFR_IDELAY_TX1_CLK		0x818b
#define SFR_IDELAY_RX0_SYMBOL	0x8196
#define SFR_IDELAY_RX1_SYMBOL	0x8198

/* PCS-SFR JTAG interface attributes */
#define SFR_JTAG_RESET		0x8190
#define SFR_JTAG_ADDR                   0x8191
#define SFR_JTAG_DATA_WR1               0x8192
#define SFR_JTAG_DATA_WR2               0x8193

#define SFR_JTAG_CMD                    0x8194
#define SFR_JTAG_CMD_ADDR               0x0
#define SFR_JTAG_CMD_DATA               0x1
#define SFR_JTAG_CMD_READ               0x3

#define SFR_JTAG_START			0x8195
#define SFR_JTAG_START_PULSE_LOW	0x0
#define SFR_JTAG_START_PULSE_HIGH	0x02
#define SFR_JTAG_START_TRST_N		0x04

/* Attributes configurable through SFR JTAG */
#define JTAG_REF_USE_PAD		0x2002
#define JTAG_MPLLA_MULTIPLIER		0x2003
#define JTAG_MPLLA_BANDWIDTH		0x2004
#define JTAG_INIT_SGEAR			0x200a
#define JTAG_INIT_REFSEL		0x400a
#define JTAG_RX0_CDR_VCO_LOW_FREQ	0x3004
#define JTAG_RX0_REF_LD_VAL		0x3005
#define JTAG_RX0_VCO_LD_VAL		0x3006
#define JTAG_RX0_EQ_DFE_TAP		0x3007
#define JTAG_RX1_CDR_VCO_LOW_FREQ	0x3104
#define JTAG_RX1_REF_LD_VAL		0x3105
#define JTAG_RX1_VCO_LD_VAL		0x3106
#define JTAG_RX1_EQ_DFE_TAP		0x3107

/* C10 interface attributues */
/* C10 Interface */
#define TC_CBC_REG_ADDR_LSB			0x8116
#define TC_CBC_REG_ADDR_MSB			0x8117
#define TC_CBC_REG_WR_LSB			0x8118
#define TC_CBC_REG_WR_MSB			0x8119
#define TC_CBC_REG_RD_LSB			0x811a
#define TC_CBC_REG_RD_MSB			0x811b
#define CBC_REG_RD_WR_SEL			0x811c
#define CBC_REG_READ			0x00
#define CBC_REG_WRITE			0x01

/* C10 configurable Registers */

#define MPLL_PWR_CTL_CAL_CTRL			0x20
#define MPLL_SKIPCAL_COARSE_TUNE		0x28
#define MPLL_COARSE_TUNE_VAL			0x27
#define LANE_DIG_ANA_RX0_DAC_CTRL_OVRD		0x10ae
#define LANE_DIG_ANA_RX0_DAC_CTRL		0x10ad
#define LANE_DIG_ANA_RX0_DAC_CTRL_SEL		0x10af
#define LANE_DIG_ANA_RX0_ANA_CAL_DAC_CTRL_EN	0x10b6
/*aw1903 phy use 401c*/
#define RAWAONLANEN_DIG_FAST_FLAGS0		0x401c
/*change 0x401c to 305c according to akrostar-tech phy databook in aw1885*/
//#define RAWAONLANEN_DIG_FAST_FLAGS0		0x305c
#define LANE_DIG_ANA_RX1_DAC_CTRL_OVRD		0x11ae
#define LANE_DIG_ANA_RX1_DAC_CTRL		0x11ad
#define LANE_DIG_ANA_RX1_DAC_CTRL_SEL		0x11af
#define LANE_DIG_ANA_RX1_ANA_CAL_DAC_CTRL_EN	0x11b6

/*LNAE 0 AFE OFST**/
#define RAWAONLANEN_DIG_AFE_ATT_IDAC_OFST0	0x4000
#define RAWAONLANEN_DIG_AFE_CTLE_IDAC_OFST0 	0x4001

/*lane 0,1 **/
#define RAWAONLANEN_DIG_RX_ADPT_DFE_TAP3_0 	0x401e
#define RAWAONLANEN_DIG_RX_ADPT_DFE_TAP3_1 	0x411e


/*LNAE 1 AFE OFST**/
#define RAWAONLANEN_DIG_AFE_ATT_IDAC_OFST1	0x4100
#define RAWAONLANEN_DIG_AFE_CTLE_IDAC_OFST1 	0x4101


#define RAWAONLANEN_DIG_FAST_FLAGS1		0x411c
/*synopsys use these address*/
#define RAWAONLANEN_DIG_MPLLA_COARSE_TUNE	0x7014
/*change 0x7014 to 3054 according to akrostar-tech phy databook*/
//#define RAWAONLANEN_DIG_MPLLA_COARSE_TUNE	0x3054
#define RAWMEM_DIG_RAM_CMN0_B0_R0 		(0xc000)

/* RMMI Registers */
#define RMMI_RX_HS_ADAPT_INITIAL_CAPABILITY 	0x9f
#define RMMI_RX_HS_ADAPT_REFRESH_CAPABILITY 	0x9e
#define RMMI_RX_HS_UNTERMINATED_CAPABILITY 	0x85
#define RMMI_RX_LS_TERMINATED_CAPABILITY 	0x86
#define RMMI_CBCRCTRL			0x811f
#define RMMI_RXSQCONTROL		0x8009
#define RMMI_TXLINERSTCFGMSB		0x8009
#define RMMI_TXLINERSTCFGLSB		0x800a
#define RMMI_RXLINERSTCFGMSB		0x8011
#define RMMI_RXLINERSTCFGLSB		0x8012
#define RMMI_RXRHOLDCTRLOPT		0x8013
#define RMMI_CBUPLH8			0x8130
#define RMMI_CBREFCLKCTRL2		0x8132
#define RMMI_CBUPLH8			0x8130
#define EXT_COARSE_TUNE_RATEB		0x814e
#define EXT_COARSE_TUNE_RATEA		0x814d
#define RMMI_CBENBLCPBATTRWR		0x8113
#define RMMI_RESET_OVR_REGISTER		0x20024
#define RMMI_REFCLKEN_OVR_REGISTER	0x20028

/* Prototyping Provision Module */
#define PROTO_L4_IRQ_CTRL		0x30010
#define PROTO_L4_IRQ_STAT		0x30014

#define PROTO_L4_IRQ_CTRL_CLR	BIT(4)
#define GET_L4_IRQ_CNTR_THOLD(x)	(((x) >> 8) & 0xffff)

/* helpers */
#define REG_16_LSB(x)				((x) & 0xff)
#define REG_16_MSB(x)				(((x) >> 8) & 0x00ff)

#endif /* End of Header */
