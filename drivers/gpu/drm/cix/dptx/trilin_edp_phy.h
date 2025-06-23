// SPDX-License-Identifier: GPL-2.0
//------------------------------------------------------------------------------
//    Trilinear Technologies DisplayPort DRM Driver
//    Copyright (C) 2023~2024 Trilinear Technologies
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, version 2.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//    General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program. If not, see <http://www.gnu.org/licenses/>.
//------------------------------------------------------------------------------

#ifndef _TRILIN_CADENCE_PHY_H_
#define _TRILIN_CADENCE_PHY_H_



//------------------------------------------------------------------------------
//     register map
//------------------------------------------------------------------------------
#define TRILIN_EDP_CDB_ADDR                    0x0000
#define TRILIN_EDP_CDB_WDATA                   0x0004
#define TRILIN_EDP_CDB_SEL_WR                  0x0008    // cdb_select @ bit 1, cdb_write @ bit 0
#define TRILIN_EDP_CDB_READY                   0x000c    // read-only address..
#define TRILIN_EDP_CDB_RDATA                   0x0010    // read-only address
#define TRILIN_EDP_CDB_RSTN                    0x0014    // cdb_rstn at bit 0
#define TRILIN_EDP_CDB_ENABLE                  0x0018    // write a 1 to enable configured transaction (resets CDB_READY flag)
#define TRILIN_EDP_REFCLK_IS_100MHz            0x0030
#define TRILIN_EDP_PHY_CONFIG_RSTN             0x0040
#define TRILIN_EDP_PHY_CMN_READY               0x0044    // read-only address
#define TRILIN_EDP_PHY_LANE_EN_BITS            0x0048
#define TRILIN_EDP_PHY_LANE_RSTN_BITS          0x004c
#define TRILIN_EDP_PHY_DATA_ENABLE             0x0050
#define TRILIN_EDP_AUXHPD_ENABLE_BITS          0x0054
#define TRILIN_EDP_TX_ELEC_IDLE_BITS           0x0058
#define TRILIN_EDP_PHY_LANE_PWR_STATE_REQ      0x0060
#define TRILIN_EDP_PHY_LANE_PWR_STATE_ACK      0x0064    // read-only address
#define TRILIN_EDP_PHY_LANE_PLLCLK_ENABLE      0x0068
#define TRILIN_EDP_PHY_LANE_PLLCLK_EN_ACK      0x006c    // read-only address
#define TRILIN_EDP_PHY_POWERDOWN               0x0070
#define TRILIN_EDP_PHY_TX_LINK_RATE            0x0078
#define TRILIN_EDP_PMA_EMPHASIS_LN0            0x00a0
#define TRILIN_EDP_PMA_EMPHASIS_LN1            0x00a4
#define TRILIN_EDP_PMA_EMPHASIS_LN2            0x00a8
#define TRILIN_EDP_PMA_EMPHASIS_LN3            0x00ac
#define TRILIN_EDP_PMA_VMARGIN_LN0             0x00b0
#define TRILIN_EDP_PMA_VMARGIN_LN1             0x00b4
#define TRILIN_EDP_PMA_VMARGIN_LN2             0x00b8
#define TRILIN_EDP_PMA_VMARGIN_LN3             0x00bc
#define TRILIN_EDP_PMA_LOW_POWER_SWING         0x00c0
#define TRILIN_EDP_PHY_TEST_RDWR_ADDRESS       0x00f4
#define TRILIN_EDP_PHY_MAX_DP_LANE_COUNT       0x00f4

//------------------------------------------------------------------------------
//    Cadence PHY config & diagnostics bus
//------------------------------------------------------------------------------
// PHY and PCS COnfiguration and Control
#define kPHY_PIPE_CMN_CTRL1                0xc000
#define kPHY_PIPE_CMN_CTRL2                0xc001
#define kPHY_PIPE_COM_LOCK_CFG1            0xc002
#define kPHY_PIPE_COM_LOCK_CFG2            0xc003
#define kPHY_PIPE_EIE_LOCK_CFG             0xc004
#define kPHY_PIPE_LANE_DSBL                0xc005
#define kPHY_PIPE_RCV_DEF_INH              0xc006
#define kPHY_PIPE_RX_ELEC_IDLE_DLY         0xc007
#define kPHY_PLL_CFG                       0xc00e
#define kPHY_STATE_CHG_TIMEOUT             0xc00a
#define kPHY_AUTO_CFG_SPDUP                0xc00f
#define kPHY_REFCLK_DET_THRES_LOW          0xc010
#define kPHY_REFCLK_DET_THRES_HIGH         0xc011
#define kPHY_REFCLK_DET_INTERVAL           0xc012
#define kPHY_REFCLK_DET_OP_DELAY           0xc013
#define kPHY_REFCLK_DET_ISO_CTRL           0xc014
#define kPHY_INTERRUPT_STS                 0xd012
#define kPHY_PIPE_USB3_GEN2_PRE_CFG0       0xc01c
#define kPHY_PIPE_USB3_GEN2_PRE_CFG1       0xc01d
#define kPHY_PIPE_USB3_GEN2_POST_CFG0      0xc01e
#define kPHY_PIPE_USB3_GEN2_POST_CFG1      0xc01f
#define kPHY_FULLRT_DIV_CFG                0xd013

// PMA Configuration and control
#define kPHY_PMA_CMN_CTRL1                 0xe000
#define kPHY_PMA_CMN_CTRL2                 0xe001
#define kPHY_PMA_SSM_STATE                 0xe002
#define kPHY_PMA_PLL_RAW_CTRL              0xe003
#define kPHY_PMA_PLL0_SM_STATE             0xe00b
#define kPHY_PMA_PLL1_SM_STATE             0xe00c
#define kPHY_PMA_XCVR_CTRL                 0xf000
#define kPHY_PMA_XCVR_LPBK                 0xf001
#define kPHY_PMA_PI_POS                    0xf002
#define kPHY_PMA_PSM_STATE_LO              0xf010
#define kPHY_PMA_PSM_STATE_HI              0xf011

// PHY isolation
#define kPHY_ISO_CMN_CTRL                  0xc008
#define kPHY_PIPE_ISO_TX_CTRL              0xd000
#define kPHY_PIPE_ISO_TX_LPC_LO            0xd001
#define kPHY_PIPE_ISO_TX_LPC_HI            0xd002
#define kPHY_PIPE_ISO_TX_DMPH_LO           0xd003
#define kPHY_PIPE_ISO_TX_DMPH_HI           0xd004
#define kPHY_PIPE_ISO_TX_FSLF              0xd005
#define kPHY_PIPE_ISO_TX_DATA_LO           0xd006
#define kPHY_PIPE_ISO_TX_DATA_HI           0xd007
#define kPHY_PIPE_ISO_RX_CTRL              0xd008
#define kPHY_PIPE_ISO_RX_EQ_EVAL           0xd009
#define kPHY_PIPE_ISO_LINK_CFG             0xd00a
#define kPHY_PIPE_ISO_LINK_CTRL            0xd00b
#define kPHY_PIPE_ISO_USB_BER_CNT          0xd00c
#define kPHY_PIPE_ISO_RX_DATA_LO           0xd00e
#define kPHY_PIPE_ISO_RX_DATA_HI           0xd00f
#define kPHY_ETH_ISO_MAC_CLK_CFG           0xd010
#define kPHY_ETH_ISO_MAC_CLK_DIV           0xd011

// PMA isolation
#define kPHY_PMA_ISO_CMN_CTRL              0xe004
#define kPHY_PMA_ISO_PLL_CTRL0             0xe005
#define kPHY_PMA_ISO_PLL_CTRL1             0xe006
#define kPHY_PMA_ISOLATION_CTRL            0xe00f
#define kPHY_PMA_ISO_XCVR_CTRL             0xf003
#define kPHY_PMA_ISO_TX_LPC_LO             0xf004
#define kPHY_PMA_ISO_TX_LPC_HI             0xf005
#define kPHY_PMA_ISO_TX_DMPH_LO            0xf006
#define kPHY_PMA_ISO_TX_DMPH_HI            0xf007
#define kPHY_PMA_ISO_TX_FSLF               0xf008
#define kPHY_PMA_ISO_MGN                   0xf009
#define kPHY_PMA_ISO_LINK_MODE             0xf00a
#define kPHY_PMA_ISO_PWRST_CTRL            0xf00b
#define kPHY_PMA_ISO_RX_EQ_CTRL            0xf00d
#define kPHY_PMA_ISO_DATA_LO               0xf00e
#define kPHY_PMA_ISO_DATA_HI               0xf00f

// PMA Configuration and Diagnostic Bus
#define kCMN_PID_TYPE                      0x0000
#define kCMN_PID_NUM                       0x0003
#define kCMN_PID_REV                       0x0004
#define kCMN_PID_MFG                       0x0008
#define kCMN_PID_NODE                      0x0009
#define kCMN_PID_FLV0                      0x000a
#define kCMN_PID_FLV1                      0x000b
#define kCMN_PID_IOV                       0x000c
#define kCMN_PID_LANES                     0x000d
#define kCMN_PID_METAL0                    0x0010
#define kCMN_PID_METAL1                    0x0011
#define kCMN_PID_METAL2                    0x0012
#define kCMN_PID_METAL3                    0x0013
#define kCMN_PID_METALD                    0x0014

// startup state Machine Registers
#define kCMN_SSM_SM_CTRL                   0x0020
#define kCMN_SSM_BANDGAP_TMR               0x0021
#define kCMN_SSM_BIAS_TMR                  0x0022
#define kCMN_SSM_USER_DEF_CTRL             0x0027

// PLL 0 Control State Machine
#define kCMN_PLLSM0_SM_CTRL                0x0028
#define kCMN_PLLSM0_PLLEN_TMR              0x0029
#define kCMN_PLLSM0_PLLPRE_TMR             0x002a
#define kCMN_PLLSM0_PLLVREF_TMR            0x002b
#define kCMN_PLLSM0_PLLLOCK_TMR            0x002c
#define kCMN_PLLSM0_PLLCLKDIS_TMR          0x002d
#define kCMN_PLLSM0_USER_DEF_CTRL          0x002f

// PLL 1 Control State Machine
#define kCMN_PLLSM1_SM_CTRL                0x0030
#define kCMN_PLLSM1_PLLEN_TMR              0x0031
#define kCMN_PLLSM1_PLLPRE_TMR             0x0032
#define kCMN_PLLSM1_PLLVREF_TMR            0x0033
#define kCMN_PLLSM1_PLLLOCK_TMR            0x0034
#define kCMN_PLLSM1_PLLCLKDIS_TMR          0x0035
#define kCMN_PLLSM1_USER_DEF_CTRL          0x0037

// common control model control and diagnostic
#define kCMN_CDIAG_PWRI_TMR                0x0040
#define kCMN_CDIAG_CDB_PWRI_OVRD           0x0041
#define kCMN_CDIAG_CDB_PWRI_STAT           0x0042
#define kCMN_CDIAG_PLLC_PWRI_OVRD          0x0043
#define kCMN_CDIAG_PLLC_PWRI_STAT          0x0044
#define kCMN_CDIAG_CCAL_PWRI_OVRD          0x0045
#define kCMN_CDIAG_CCAL_PWRI_STAT          0x0046
#define kCMN_CDIAG_XCVRC_PWRI_OVRD         0x0047
#define kCMN_CDIAG_XCVRC_PWRI_STAT         0x0048
#define kCMN_CDIAG_DIAG_PWRI_OVRD          0x0049
#define kCMN_CDIAG_DIAG_PWRI_STAT          0x004a
#define kCMN_CDIAG_PRATECLK_CTRL           0x004b
#define kCMN_CDIAG_REFCLK_OVRD             0x004c
#define kCMN_CDIAG_REFCLK_TEST             0x004d
#define kCMN_CDIAG_PSMCLK_CTRL             0x004e
#define kCMN_CDIAG_SDOSC_CTRL              0x004f
#define kCMN_CDIAG_REFCLK_DRV0_CTRL        0x0050
#define kCMN_CDIAG_CDB_DIAG                0x005c
#define kCMN_CDIAG_RST_DIAG                0x005d
#define kCMN_CDIAG_DCYA                    0x005f

// Bandgap calibration controller
#define kCMN_BGCAL_CTRL                    0x0060
#define kCMN_BGCAL_OVRD                    0x0061
#define kCMN_BGCAL_START                   0x0062
#define kCMN_BGCAL_TUNE                    0x0063
#define kCMN_BGCAL_INIT_TMR                0x0064
#define kCMN_BGCAL_ITER_TMR                0x0065

// external bias current calibration controller
#define kCMN_IBCAL_CTRL                    0x0070
#define kCMN_IBCAL_OVRD                    0x0071
#define kCMN_IBCAL_START                   0x0072
#define kCMN_IBCAL_TUNE                    0x0073
#define kCMN_IBCAL_INIT_TMR                0x0074
#define kCMN_IBCAL_ITER_TMR                0x0075

// PLL 0 controller
#define kCMN_PLL0_VCOCAL_CTRL              0x0080
#define kCMN_PLL0_VCOCAL_START             0x0081
#define kCMN_PLL0_VCOCAL_TCTRL             0x0082
#define kCMN_PLL0_VCOCAL_OVRD              0x0083
#define kCMN_PLL0_VCOCAL_INIT_TMR          0x0084
#define kCMN_PLL0_VCOCAL_ITER_TMR          0x0085
#define kCMN_PLL0_VCOCAL_REFTIM_START      0x0086
#define kCMN_PLL0_VCOCAL_PLLCNT_START      0x0088
#define kCMN_PLL0_LOCK_REFCNT_START        0x009c
#define kCMN_PLL0_LOCK_REFCNT_IDLE         0x009d
#define kCMN_PLL0_LOCK_PLLCNT_START        0x009e
#define kCMN_PLL0_LOCK_PLLCNT_THR          0x009f
#define kCMN_PLL0_INTDIV_M0                0x0090
#define kCMN_PLL0_FRACDIVL_M0              0x0091
#define kCMN_PLL0_FRACDIVH_M0              0x0092
#define kCMN_PLL0_HIGH_THR_M0              0x0093
#define kCMN_PLL0_DSM_DIAG_M0              0x0094
#define kCMN_PLL0_FBH_OVRD_M0              0x0095
#define kCMN_PLL0_FBL_OVRD_M0              0x0096
#define kCMN_PLL0_SS_CTRL1_M0              0x0098
#define kCMN_PLL0_SS_CTRL2_M0              0x0099
#define kCMN_PLL0_SS_CTRL3_M0              0x009a
#define kCMN_PLL0_SS_CTRL4_M0              0x009b
#define kCMN_PLL0_INTDIV_M1                0x00a0
#define kCMN_PLL0_FRACDIVL_M1              0x00a1
#define kCMN_PLL0_FRACDIVH_M1              0x00a2
#define kCMN_PLL0_HIGH_THR_M1              0x00a3
#define kCMN_PLL0_DSM_DIAG_M1              0x00a4
#define kCMN_PLL0_FBH_OVRD_M1              0x00a5
#define kCMN_PLL0_FBL_OVRD_M1              0x00a6
#define kCMN_PLL0_SS_CTRL1_M1              0x00a8
#define kCMN_PLL0_SS_CTRL2_M1              0x00a9
#define kCMN_PLL0_SS_CTRL3_M1              0x00aa
#define kCMN_PLL0_SS_CTRL4_M1              0x00ab

// PLL 1 controller
#define kCMN_PLL1_VCOCAL_CTRL              0x00c0
#define kCMN_PLL1_VCOCAL_START             0x00c1
#define kCMN_PLL1_VCOCAL_TCTRL             0x00c2
#define kCMN_PLL1_VCOCAL_OVRD              0x00c3
#define kCMN_PLL1_VCOCAL_INIT_TMR          0x00c4
#define kCMN_PLL1_VCOCAL_ITER_TMR          0x00c5
#define kCMN_PLL1_VCOCAL_REFTIM_START      0x00c6
#define kCMN_PLL1_VCOCAL_PLLCNT_START      0x00c8
#define kCMN_PLL1_LOCK_REFCNT_START        0x00dc
#define kCMN_PLL1_LOCK_REFCNT_IDLE         0x00dd
#define kCMN_PLL1_LOCK_PLLCNT_START        0x00de
#define kCMN_PLL1_LOCK_PLLCNT_THR          0x00df
#define kCMN_PLL1_INTDIV_M0                0x00d0
#define kCMN_PLL1_FRACDIVL_M0              0x00d1
#define kCMN_PLL1_FRACDIVH_M0              0x00d2
#define kCMN_PLL1_HIGH_THR_M0              0x00d3
#define kCMN_PLL1_DSM_DIAG_M0              0x00d4
#define kCMN_PLL1_FBH_OVRD_M0              0x00d5
#define kCMN_PLL1_FBL_OVRD_M0              0x00d6
#define kCMN_PLL1_SS_CTRL1_M0              0x00d8
#define kCMN_PLL1_SS_CTRL2_M0              0x00d9
#define kCMN_PLL1_SS_CTRL3_M0              0x00da
#define kCMN_PLL1_SS_CTRL4_M0              0x00db

// Transmitter pull up resiter calibration controller
#define kCMN_TXPUCAL_CTRL                  0x0100
#define kCMN_TXPUCAL_OVRD                  0x0101
#define kCMN_TXPUCAL_START                 0x0102
#define kCMN_TXPUCAL_TUNE                  0x0103
#define kCMN_TXPUCAL_INIT_TMR              0x0104
#define kCMN_TXPUCAL_ITER_TMR              0x0105
#define kCMN_TXPDCAL_CTRL                  0x0108
#define kCMN_TXPDCAL_OVRD                  0x0109
#define kCMN_TXPDCAL_START                 0x010a
#define kCMN_TXPDCAL_TUNE                  0x010b
#define kCMN_TXPDCAL_INIT_TMR              0x010c
#define kCMN_TXPDCAL_ITER_TMR              0x010d

// Receiver resistor calibration controller
#define kCMN_RXPCAL_CTRL                   0x0110
#define kCMN_RXCAL_OVRD                    0x0111
#define kCMN_RXCAL_START                   0x0112
#define kCMN_RXCAL_TUNE                    0x0113
#define kCMN_RXCAL_INIT_TMR                0x0114
#define kCMN_RXCAL_ITER_TMR                0x0115

// Signal Dectect Clock Calibration
#define kCMN_SD_CAL_CTRL                   0x0120
#define kCMN_SD_CAL_START                  0x0121
#define kCMN_SD_CAL_TCTRL                  0x0122
#define kCMN_SD_CAL_OVRD                   0x0123
#define kCMN_SD_CAL_INIT_TMR               0x0124
#define kCMN_SD_CAL_ITER_TMR               0x0125
#define kCMN_SD_CAL_REFTIM_START           0x0126
#define kCMN_SD_CAL_PLLCNT_START           0x0128

// common clock measurement module
#define kCMN_CMSMT_CLK_MSMT_CTRL           0x0180
#define kCMN_CMSMT_TEST_CLK_SEL            0x0181
#define kCMN_CMSMT_REF_CLK_TMR_VALUE       0x0182
#define kCMN_CMSMT_TEST_CLK_CNT_VALUE      0x0183

// common PLL 0 control and diagnostic
#define kCMN_PDIAG_PLL0_CTRL_M0            0x01a0
#define kCMN_PDIAG_PLL0_CLK_SEL_M0         0x01a1
#define kCMN_PDIAG_PLL0_OVRD_M0            0x01a2
#define kCMN_PDIAG_PLL0_ITRIM_M0           0x01a3
#define kCMN_PDIAG_PLL0_CP_PADJ_M0         0x01a4
#define kCMN_PDIAG_PLL0_CP_IADJ_M0         0x01a5
#define kCMN_PDIAG_PLL0_FILT_PADJ_M0       0x01a6
#define kCMN_PDIAG_PLL0_CP_TUNE_M0         0x01a7
#define kCMN_PDIAG_PLL0_CTRL_M1            0x01b0
#define kCMN_PDIAG_PLL0_CLK_SEL_M1         0x01b1
#define kCMN_PDIAG_PLL0_OVRD_M1            0x01b2
#define kCMN_PDIAG_PLL0_ITRM_M1            0x01b3
#define kCMN_PDIAG_PLL0_CP_PADJ_M1         0x01b4
#define kCMN_PDIAG_PLL0_CP_IADJ_M1         0x01b5
#define kCMN_PDIAG_PLL0_FILT_PADJ_M1       0x01b6
#define kCMN_PDIAG_PLL0_CP_TUNE_M1         0x01b7

// common PLL 1 control
#define kCMN_PDIAG_PLL1_CTRL_M0            0x01c0
#define kCMN_PDIAG_PLL1_CLK_SEL_M0         0x01c1
#define kCMN_PDIAG_PLL1_OVRD_M0            0x01c2
#define kCMN_PDIAG_PLL1_ITRIM_M0           0x01c3
#define kCMN_PDIAG_PLL1_CP_PADJ_M0         0x01c4
#define kCMN_PDIAG_PLL1_CP_IADJ_M0         0x01c5
#define kCMN_PDIAG_PLL1_FILT_PADJ_M0       0x01c6
#define kCMN_PDIAG_PLL1_CP_TUNE_M0         0x01c7

// common functions control and diagnostic
#define kCMN_DIAG_BANDGAP_OVRD             0x01e0
#define kCMN_DIAG_BIAS_OVRD1               0x01e1
#define kCMN_DIAG_BIAS_OVRD2               0x01e2
#define kCMN_DIAG_VREG_CTRL                0x01e3
#define kCMN_DIAG_PM_CTRL                  0x01e4
#define kCMN_DIAG_SH_BANDGAP               0x01e5
#define kCMN_DIAG_SH_RESISTOR              0x01e6
#define kCMN_DIAG_SH_SDCLK                 0x01e7
#define kCMN_DIAG_ATB_CTRL1                0x01e8
#define kCMN_DIAG_ATB_CTRL2                0x01e9
#define kCMN_DIAG_ATB_ADC_CTRL0            0x01ea
#define kCMN_DIAG_ATB_ADC_CTRL1            0x01eb
#define kCMN_DIAG_HSRRSM_CTRL              0x01ec
#define kCMN_DIAG_RST_DIAG                 0x01ed
#define kCMN_DIAG_DCYA                     0x01ee
#define kCMN_DIAG_ACYA                     0x01ef
#define kCMN_DIAG_GPANA_0                  0x01f0
#define kCMN_DIAG_GPANA_1                  0x01f1
#define kCMN_DIAG_GPANA_2                  0x01f2
#define kCMN_DIAG_GPANA_ST                 0x01f3

// power State machine
#define kXCVR_PSM_CTRL                     0x4000
#define kXCVR_PSM_RCTRL                    0x4001
#define kXCVR_PSM_CALIN_TMR                0x4002
#define kXCVR_PSM_A0IN_TMR                 0x4003
#define kXCVR_PSM_A0BYP_TMR                0x4004
#define kXCVR_PSM_A1IN_TMR                 0x4005
#define kXCVR_PSM_A2IN_TMR                 0x4006
#define kXCVR_PSM_A3IN_TMR                 0x4007
#define kXCVR_PSM_A4IN_TMR                 0x4008
#define kXCVR_PSM_A5IN_TMR                 0x4009
#define kXCVR_PSM_CALOUT_TMR               0x400a
#define kXCVR_PSM_A0OUT_TMR                0x400b
#define kXCVR_PSM_A1OUT_TMR                0x400c
#define kXCVR_PSM_A2OUT_TMR                0x400d
#define kXCVR_PSM_A3OUT_TMR                0x400e
#define kXCVR_PSM_A4OUT_TMR                0x400f
#define kXCVR_PSM_A5OUT_TMR                0x4010
#define kXCVR_PSM_RDY_TMR                  0x4011
#define kXCVR_PSM_DIAG                     0x4012
#define kXCVR_PSM_ST_0                     0x4013
#define kXCVR_PSM_ST_1                     0x4014
#define kXCVR_PSM_USER_DEF_CTRL            0x401f

// TX coefficient calculator
#define kTX_TXCC_CTRL                      0x4040
#define kTX_TXCC_PRE_OVRD                  0x4041
#define kTX_TXCC_MAIN_OVRD                 0x4042
#define kTX_TXCC_POST_OVRD                 0x4043
#define kTX_TXCC_PRE_CVAL                  0x4044
#define kTX_TXCC_MAIN_CVAL                 0x4045
#define kTX_TXCC_POST_CVAL                 0x4046
#define kTX_TXCC_LF_MULT                   0x4047
#define kTX_TXCC_CPRE_MULT_00              0x4048
#define kTX_TXCC_CPRE_MULT_01              0x4049
#define kTX_TXCC_CPRE_MULT_10              0x404a
#define kTX_TXCC_CPRE_MULT_11              0x404b
#define kTX_TXCC_CPOST_MULT_00             0x404c
#define kTX_TXCC_CPOST_MULT_01             0x404d
#define kTX_TXCC_CPOST_MULT_10             0x404e
#define kTX_TXCC_CPOST_MULT_11             0x404f
#define kTX_TXCC_MGNFS_MULT_000            0x4050
#define kTX_TXCC_MGNFS_MULT_001            0x4051
#define kTX_TXCC_MGNFS_MULT_010            0x4052
#define kTX_TXCC_MGNFS_MULT_011            0x4053
#define kTX_TXCC_MGNFS_MULT_100            0x4054
#define kTX_TXCC_MGNFS_MULT_101            0x4055
#define kTX_TXCC_MGNFS_MULT_110            0x4056
#define kTX_TXCC_MGNFS_MULT_111            0x4057
#define kTX_TXCC_MGNHS_MULT_000            0x4058
#define kTX_TXCC_MGNHS_MULT_001            0x4059
#define kTX_TXCC_MGNHS_MULT_010            0x405a
#define kTX_TXCC_MGNHS_MULT_011            0x405b
#define kTX_TXCC_MGNHS_MULT_100            0x405c
#define kTX_TXCC_MGNHS_MULT_101            0x405d
#define kTX_TXCC_MGNHS_MULT_110            0x405e
#define kTX_TXCC_MGNHS_MULT_111            0x405f
#define kTX_TXCC_P0PRE_COEF_MULT           0x4060
#define kTX_TXCC_P1PRE_COEF_MULT           0x4061
#define kTX_TXCC_P2PRE_COEF_MULT           0x4062
#define kTX_TXCC_P3PRE_COEF_MULT           0x4063
#define kTX_TXCC_P4PRE_COEF_MULT           0x4064
#define kTX_TXCC_P5PRE_COEF_MULT           0x4065
#define kTX_TXCC_P6PRE_COEF_MULT           0x4066
#define kTX_TXCC_P7PRE_COEF_MULT           0x4067
#define kTX_TXCC_P8PRE_COEF_MULT           0x4068
#define kTX_TXCC_P9PRE_COEF_MULT           0x4069
#define kTX_TXCC_P0POST_COEF_MULT          0x4070
#define kTX_TXCC_P1POST_COEF_MULT          0x4071
#define kTX_TXCC_P2POST_COEF_MULT          0x4072
#define kTX_TXCC_P3POST_COEF_MULT          0x4073
#define kTX_TXCC_P4POST_COEF_MULT          0x4074
#define kTX_TXCC_P5POST_COEF_MULT          0x4075
#define kTX_TXCC_P6POST_COEF_MULT          0x4076
#define kTX_TXCC_P7POST_COEF_MULT          0x4077
#define kTX_TXCC_P8POST_COEF_MULT          0x4078
#define kTX_TXCC_P9POST_COEF_MULT          0x4079

// Transceiver Control and Diagnostics
#define kXCVR_DIAG_PWRI_TMR                0x40e0
#define kXCVR_DIAG_XCAL_PWRI_OVRD          0x40e1
#define kXCVR_DIAG_XCAL_PWRI_STAT          0x40e2
#define kXCVR_DIAG_XDP_PWRI_OVRD           0x40e3
#define kXCVR_DIAG_XDP_PWRI_STAT           0x40e4
#define kXCVR_DIAG_PLLDRC_CTRL             0x40e5
#define kXCVR_DIAG_HSCLK_SEL               0x40e6
#define kXCVR_DIAG_HSCLK_DIV               0x40e7
#define kXCVR_DIAG_TXCLK_CTRL              0x40e8
#define kXCVR_DIAG_RXCLK_CTRL              0x40e9
#define kXCVR_DIAG_BIDI_CTRL               0x40ea
#define kXCVR_DIAG_PSC_OVRD                0x40eb
#define kXCVR_DIAG_RST_DIAG                0x40ec
#define kXCVR_DIAG_CLK_CTRL                0x40ed
#define kXCVR_DIAG_DCYA                    0x40ef
#define kXCVR_DIAG_GPANA_0                 0x40f0
#define kXCVR_DIAG_GPANA_1                 0x40f1
#define kXCVR_DIAG_GPANA_2                 0x40f2
#define kXCVR_DIAG_GPANA_ST                0x40f3

// Driver controller control and diagnostic
#define kDRV_DIAG_LANE_FCM_EN_TO           0x40c0
#define kDRV_DIAG_LANE_FCM_EN_SWAIT_TMR    0x40c1
#define kDRV_DIAG_LANE_FCM_EN_MGN_TMR      0x40c2
#define kDRV_DIAG_LANE_FCM_EN_TUNE         0x40c3
#define kDRV_DIAG_LFPS_CTRL                0x40c4
#define kDRV_DIAG_RCVDET_TUNE              0x40c5
#define kDRV_DIAG_TX_DRV                   0x40c6

// Transmitter power stat controller
#define kTX_PSC_A0                         0x4100
#define kTX_PSC_A1                         0x4101
#define kTX_PSC_A2                         0x4102
#define kTX_PSC_A3                         0x4103
#define kTX_PSC_A4                         0x4104
#define kTX_PSC_A5                         0x4105
#define kTX_PSC_CAL                        0x4106
#define kTX_PSC_RDY                        0x4107

// Receiver Detect
#define kRX_RCVDET_CTRL                    0x4120
#define kRX_RCVDET_OVRD                    0x4121
#define kRX_EN_TMR                         0x4122
#define kRX_ST_TMR                         0x4123

// Transmitter BIST controller
#define kTX_BIST_CTRL                      0x4140
#define kTX_BIST_UDDWR                     0x4141
#define kTX_BIST_SEED0                     0x4142
#define kTX_BIST_SEED1                     0x4143

// Transmitter controller and diagnostics
#define kTX_DIAG_SFIFO_CTRL                0x41e0
#define kTX_DIAG_SFIFO_TMR                 0x41e1
#define kTX_DIAG_ELEC_IDLE                 0x41e2
#define kTX_DIAG_RST_DIAG                  0x41e5
#define kTX_DIAG_DCYA                      0x41e6
#define kTX_DIAG_ACYA                      0x41e7

// Receiver Power state controller
#define kRX_PSC_A0                         0x8000
#define kRX_PSC_A1                         0x8001
#define kRX_PSC_A2                         0x8002
#define kRX_PSC_A3                         0x8003
#define kRX_PSC_A4                         0x8004
#define kRX_PSC_A5                         0x8005
#define kRX_PSC_CAL                        0x8006
#define kRX_PSC_RDY                        0x8007

// Signal dectect calibration 0
#define kRX_SDCAL0_CTRL                    0x8040
#define kRX_SDCAL0_OVRD                    0x8041
#define kRX_SDCAL0_START                   0x8042
#define kRX_SDCAL0_TUNE                    0x8043
#define kRX_SDCAL0_INIT_TMR                0x8044
#define kRX_SDCAL0_ITER_TMR                0x8045

// Signal dectect calibration 0
#define kRX_SDCAL1_CTRL                    0x8048
#define kRX_SDCAL1_OVRD                    0x8049
#define kRX_SDCAL1_START                   0x804a
#define kRX_SDCAL1_TUNE                    0x804b
#define kRX_SDCAL1_INIT_TMR                0x804c
#define kRX_SDCAL1_ITER_TMR                0x804d

// sampler error DAC control
#define kRX_SAMP_DAC_CTRL                  0x8058

// Receiver sampler latch calibration
#define kRX_SLC_CTRL                       0x8060
#define kRX_SLC_IPP_STAT                   0x8061
#define kRX_SLC_IPP_OVRD                   0x8062
#define kRX_SLC_IPM_STAT                   0x8063
#define kRX_SLC_IPM_OVRD                   0x8064
#define kRX_SLC_QPP_STAT                   0x8065
#define kRX_SLC_QPP_OVRD                   0x8066
#define kRX_SLC_QPM_STAT                   0x8067
#define kRX_SLC_QPM_OVRD                   0x8068
#define kRX_SLC_EPP_STAT                   0x8069
#define kRX_SLC_EPP_OVRD                   0x806a
#define kRX_SLC_EPM_STAT                   0x806b
#define kRX_SLC_EPM_OVRD                   0x806c
#define kRX_SLC_INIT_TMR                   0x806d
#define kRX_SLC_RUN_TMR                    0x806e
#define kRX_SLC_DIAG_CTRL                  0x806f
#define kRX_SLC_DIS                        0x8070

// Receiver Equalizer Engine (REE)

#define kRX_REE_PTXEQSM_CTRL               0x8100
#define kRX_REE_PTXEQSM_EQENM_EVAL         0x8101
#define kRX_REE_PTXEQSM_EQENM_PEVAL        0x8102
#define kRX_REE_PTXEQSM_PEVAL_TMR          0x8103
#define kRX_REE_PTXEQSM_TIMEOUT_TMR        0x8104
#define kRX_REE_PTXEQSM_MAX_EVAL_CNT       0x8105
#define kRX_REE_GCSM1_CTRL                 0x8108
#define kRX_REE_GCSM1_EQENM_PH1            0x8109
#define kRX_REE_GCSM1_EQENM_PH2            0x810a
#define kRX_REE_GCSM1_START_TMR            0x810b
#define kRX_REE_GCSM1_RUN_PH1_TMR          0x810c
#define kRX_REE_GCSM1_RUN_PH2_TMR          0x810d
#define kRX_REE_GCSM2_CTRL                 0x8110
#define kRX_REE_GCSM2_EQENM_PH1            0x8111
#define kRX_REE_GCSM2_EQENM_PH2            0x8112
#define kRX_REE_GCSM2_START_TMR            0x8113
#define kRX_REE_GCSM2_RUN_PH1_TMR          0x8114
#define kRX_REE_GCSM2_RUN_PH2_TMR          0x8115
#define kRX_REE_PERGCSM_CTRL               0x8118
#define kRX_REE_PERGCSM_EQENM_PH1          0x8119
#define kRX_REE_PERGCSM_EQENM_PH2          0x811a
#define kRX_REE_PERGCSM_START_TMR          0x811b
#define kRX_REE_PERGCSM_RUN_PH1_TMR        0x811c
#define kRX_REE_PERGCSM_RUN_PH2_TMR        0x811d
#define kRX_REE_U3GCSM_CTRL                0x8120
#define kRX_REE_U3GCSM_EQENM_PH1           0x8121
#define kRX_REE_U3GCSM_EQENM_PH2           0x8122
#define kRX_REE_U3GCSM_START_TMR           0x8123
#define kRX_REE_U3GCSM_RUN_PH1_TMR         0x8124
#define kRX_REE_U3GCSM_RUN_PH2_TMR         0x8125
#define kRX_REE_ANAENSM_DEL_TMR            0x8128
#define kRX_REE_TXPOST_CTRL                0x8130
#define kRX_REE_TXPOST_CODE_CTRL           0x8131
#define kRX_REE_TXPOST_UTHR                0x8132
#define kRX_REE_TXPOST_LTHR                0x8133
#define kRX_REE_TXPOST_IOVRD               0x8134
#define kRX_REE_TXPOST_COVRD0              0x8135
#define kRX_REE_TXPOST_COVRD1              0x8136
#define kRX_REE_TXPOST_DIAG                0x8137
#define kRX_REE_TXPRE_CTRL                 0x8138
#define kRX_REE_TXPRE_OVRD                 0x8139
#define kRX_REE_TXPRE_DIAG                 0x813a
#define kRX_REE_PEAK_CTRL                  0x8140
#define kRX_REE_PEAK_CODE_CTRL             0x8141
#define kRX_REE_PEAK_UTHR                  0x8142
#define kRX_REE_PEAK_LTHR                  0x8143
#define kRX_REE_PEAK_IOVRD                 0x8144
#define kRX_REE_PEAK_COVRD0                0x8145
#define kRX_REE_PEAK_COVRD1                0x8146
#define kRX_REE_PEAK_DIAG                  0x8147
#define kRX_REE_ATTEN_CTRL                 0x8148
#define kRX_REE_ATTEN_THR                  0x8149
#define kRX_REE_ATTEN_CNT                  0x814a
#define kRX_REE_ATTEN_OVRD                 0x814b
#define kRX_REE_ATTEN_DIAG                 0x814c
#define kRX_REE_TAP1_CTRL                  0x8150
#define kRX_REE_TAP1_OVRD                  0x8151
#define kRX_REE_TAP1_DIAG                  0x8152
#define kRX_REE_TAP2_CTRL                  0x8154
#define kRX_REE_TAP2_OVRD                  0x8155
#define kRX_REE_TAP2_DIAG                  0x8156
#define kRX_REE_TAP3_CTRL                  0x8158
#define kRX_REE_TAP3_OVRD                  0x8159
#define kRX_REE_TAP3_DIAG                  0x815a
#define kRX_REE_LFEQ_CTRL                  0x815c
#define kRX_REE_LFEQ_OVRD                  0x815d
#define kRX_REE_LFEQ_DIAG                  0x815e
#define kRX_REE_VGA_GAIN_CTRL              0x8160
#define kRX_REE_VGA_GAIN_OVRD              0x8161
#define kRX_REE_VGA_GAIN_DIAG              0x8162
#define kRX_REE_VGA_GAIN_TGT_DIAG          0x8163
#define kRX_REE_OFF_COR_CTRL               0x8164
#define kRX_REE_OFF_COR_OVRD               0x8165
#define kRX_REE_OFF_COR_DIAG               0x8166
#define kRX_REE_SC_COR_WCNT                0x8168
#define kRX_REE_SC_COR_TCNT                0x8169
#define kRX_REE_ADDR_CFG                   0x8170
#define kRX_REE_TAP1_CLIP                  0x8171
#define kRX_REE_TAP2_CLIP                  0x8172
#define kRX_REE_CTRL_DATA_MASK             0x8173
#define kRX_REE_FIFO_DIAG                  0x8174
#define kRX_REE_DIAG_CTRL                  0x8175
#define kRX_REE_TXEQEVAL_CTRL              0x8176
#define kRX_REE_SMGM_CTRL1                 0x8177
#define kRX_REE_SMGM_CTRL2                 0x8178
#define kRX_REE_TXEQEVAL_PRE               0x8179
#define kRX_REE_TXEQEVAL_POS               0x817a

// CDRLF configuration
#define kRX_CDRLF_CNFG                     0x8080
#define kRX_CDRLF_CNFG2                    0x8081
#define kRX_CDRLF_CNFG3                    0x8082
#define kRX_MGN_DIAG                       0x8083
#define kRX_FPL_TMR0                       0x8084
#define kRX_FPL_TMR1                       0x8085

// Eys surf
#define kRX_EYESURF_CTRL                   0x80a0
#define kRX_EYESURF_TMR_DELLOW             0x80a4
#define kRX_EYESURF_TMR_DELHIGH            0x80a5
#define kRX_EYESURF_TMR_TESTLOW            0x80a6
#define kRX_EYESURF_TMR_TESTHIGH           0x80a7
#define kRX_EYESURF_NS_COORD               0x80a8
#define kRX_EYESURF_EW_COORD               0x80a9
#define kRX_EYESURF_ERRCNT                 0x80aa

// Receiver BIST controller
#define kRX_BIST_CTRL                      0x80b0
#define kRX_BIST_SYNCCNT                   0x80b1
#define kRX_BIST_UDDWR                     0x80b2
#define kRX_BIST_ERRCNT                    0x80b3

// Receiver signal detect filter
#define kRX_SIGDET_HL_FILT_TMR             0x8090
#define kRX_SIGDET_HL_DLY_TMR              0x8091
#define kRX_SIGDET_HL_MIN_TMR              0x8092
#define kRX_SIGDET_HL_INIT_TMR             0x8093
#define kRX_SIGDET_LH_FILT_TMR             0x8094
#define kRX_SIGDET_LH_DLY_TMR              0x8095
#define kRX_SIGDET_LH_MIN_TMR              0x8096
#define kRX_SIGDET_LH_INIT_TMR             0x8097

// Receiver LFPS detect filter
#define kRX_LFPSDET_MD_CNT                 0x8098
#define kRX_LFPSDET_NS_CNT                 0x8099
#define kRX_LFPSDET_RD_CNT                 0x809a
#define kRX_LFPSDET_MP_CNT                 0x809b
#define kRX_LFPSDET_DIAG_CTRL              0x809c

// Transceiver clock measurement module
#define kXCVR_CMSMT_CLK_FREQ_MSMT_CTRL     0x81c0
#define kXCVR_CMSMT_TEST_CLK_SEL           0x81c1
#define kXCVR_CMSMT_REF_CLK_TMR_VALUE      0x81c2
#define kXCVR_CMSMT_REF_CLK_CNT_VALUE      0x81c3

// Receiver Control and Diagnostics
#define kRX_DIAG_DFE_CTRL                  0x81e0
#define kRX_DIAG_DFE_AMP_TUNE              0x81e1
#define kRX_DIAG_DFE_AMP_TUNE2             0x81e2
#define kRX_DIAG_DFE_AMP_TUNE3             0x81e3
#define kRX_DIAG_REE_DAC_CTRL              0x81e4
#define kRX_DIAG_NQST_CTRL                 0x81e5
#define kRX_DIAG_LFEQ_TUNE                 0x81e6
#define kRX_DIAG_SIGDET_TUNE               0x81e8
#define kRX_DIAG_SH_SIGDET                 0x81e9
#define kRX_DIAG_SD_TEST                   0x81ea
#define kRX_DIAG_SAMP_CTRL                 0x81ec
#define kRX_DIAG_SH_SLC_IPP                0x81ed
#define kRX_DIAG_SH_SLC_IPM                0x81ee
#define kRX_DIAG_SH_SLC_QPP                0x81ef
#define kRX_DIAG_SH_SLC_QPM                0x81f0
#define kRX_DIAG_SH_SLC_EPP                0x81f1
#define kRX_DIAG_SH_SLC_EPM                0x81f2
#define kRX_DIAG_SH_PI_RATE                0x81f4
#define kRX_DIAG_SH_PI_CAP                 0x81f5
#define kRX_DIAG_SH_PI_TUN                 0x81f6
#define kRX_DIAG_LPBK_CTRL                 0x81f8
#define kRX_DIAG_TST_DIAG                  0x81f9
#define kRX_DIAG_DCYA                      0x81fe
#define kRX_DIAG_ACYA                      0x81ff

//------------------------------------------------------------------------------
//    PMA_CMN_CTRL1 bit defines
//------------------------------------------------------------------------------
#define PMA_CMN_CTRL1_CMN_READY_BIT        (1ul<<0)

//------------------------------------------------------------------------------
//    PMA_CMN_CTRL2 bit defines
//------------------------------------------------------------------------------
#define PMA_CMN_CTRL2_PLL0_READY_BIT       (1ul<<0)
#define PMA_CMN_CTRL2_PLL1_READY_BIT       (1ul<<1)
#define PMA_CMN_CTRL2_PLL0_DISABLED_BIT    (1ul<<2)
#define PMA_CMN_CTRL2_PLL1_DISABLED_BIT    (1ul<<3)
#define PMA_CMN_CTRL2_PLL0_CLK_EN_ACK_BIT  (1ul<<4)
#define PMA_CMN_CTRL2_PLL1_CLK_EN_ACK_BIT  (1ul<<5)
#define PMA_CMN_CTRL2_PLL0_LOCKED_BIT      (1ul<<6)
#define PMA_CMN_CTRL2_PLL1_LOCKED_BIT      (1ul<<7)

//------------------------------------------------------------------------------
//    PMA_PLL_RAW_CTRL bit defines
//------------------------------------------------------------------------------
#define PMA_PLL_RAW_CTRL_PLL0_FORCE_RSTN    (1ul<<0)
#define PMA_PLL_RAW_CTRL_PLL1_FORCE_RSTN    (1ul<<1)

//------------------------------------------------------------------------------
//    Cadence PHY IP internal configuration addresses
//------------------------------------------------------------------------------
#define kLANE0_OFFSET                      0x0000
#define kLANE1_OFFSET                      0x0200
#define kLANE2_OFFSET                      0x0400
#define kLANE3_OFFSET                      0x0600

trilin_phy_error_t trilin_edp_phy_register(struct trilin_dp *dp);

#endif
