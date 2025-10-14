// SPDX-License-Identifier: GPL-2.0

#ifndef _PHY_CIX_USBDP_H_
#define _PHY_CIX_USBDP_H_

#define CMN_SSM_BIAS_TMR             0x0022
#define CMN_PLLSM0_PLLPRE_TMR        0x002a
#define CMN_PLLSM0_PLLLOCK_TMR       0x002c
#define CMN_PLLSM1_PLLPRE_TMR        0x0032
#define CMN_PLLSM1_PLLLOCK_TMR       0x0034
#define CMN_BGCAL_INIT_TMR           0x0064
#define CMN_BGCAL_ITER_TMR           0x0065
#define CMN_IBCAL_INIT_TMR           0x0074
#define CMN_IBCAL_ITER_TMR           0x0075
#define CMN_PLL0_SS_CTRL1_M0         0x0098
#define CMN_PLL0_SS_CTRL2_M0         0x0099
#define CMN_PLL0_SS_CTRL3_M0         0x009a
#define CMN_PLL0_SS_CTRL4_M0         0x009b
#define CMN_PLL1_SS_CTRL1_M0         0x00d8
#define CMN_PLL1_SS_CTRL2_M0         0x00d9
#define CMN_PLL1_SS_CTRL3_M0         0x00da
#define CMN_PLL1_SS_CTRL4_M0         0x00db
#define CMN_TXPUCAL_INIT_TMR         0x0104
#define CMN_TXPUCAL_ITER_TMR         0x0105
#define CMN_TXPDCAL_INIT_TMR         0x010c
#define CMN_TXPDCAL_ITER_TMR         0x010d
#define CMN_RXCAL_INIT_TMR           0x0114
#define CMN_RXCAL_ITER_TMR           0x0115
#define CMN_SD_CAL_INIT_TMR          0x0124
#define CMN_SD_CAL_ITER_TMR          0x0125
#define CMN_SD_CAL_REFTIM_START      0x0126
#define CMN_SD_CAL_PLLCNT_START      0x0128
#define CMN_PDIAG_PLL1_ITRIM_M0      0x01c3
#define CMN_DIAG_GPANA_0             0x01f0

#define RX_SDCAL0_INIT_TMR_LANE0     0x8044
#define RX_SDCAL0_INIT_TMR_LANE1     0x8244
#define RX_SDCAL0_INIT_TMR_LANE2     0x8444
#define RX_SDCAL0_INIT_TMR_LANE3     0x8644

#define RX_SDCAL0_ITER_TMR_LANE0     0x8045
#define RX_SDCAL0_ITER_TMR_LANE1     0x8245
#define RX_SDCAL0_ITER_TMR_LANE2     0x8445
#define RX_SDCAL0_ITER_TMR_LANE3     0x8645

#define RX_SDCAL1_INIT_TMR_LANE0     0x804c
#define RX_SDCAL1_INIT_TMR_LANE1     0x824c
#define RX_SDCAL1_INIT_TMR_LANE2     0x844c
#define RX_SDCAL1_INIT_TMR_LANE3     0x864c

#define RX_SDCAL1_ITER_TMR_LANE0     0x804d
#define RX_SDCAL1_ITER_TMR_LANE1     0x824d
#define RX_SDCAL1_ITER_TMR_LANE2     0x844d
#define RX_SDCAL1_ITER_TMR_LANE3     0x864d

#define TX_RCVDET_ST_TMR_LANE0       0x4123
#define TX_RCVDET_ST_TMR_LANE1       0x4323
#define TX_RCVDET_ST_TMR_LANE2       0x4523
#define TX_RCVDET_ST_TMR_LANE3       0x4723

#define CMN_PDIAG_PLL0_CLK_SEL_M0    0x01a1
#define CMN_PDIAG_PLL1_CLK_SEL_M0    0x01c1

#define XCVR_DIAG_HSCLK_SEL_LANE0    0x40e6
#define XCVR_DIAG_HSCLK_SEL_LANE1    0x42e6
#define XCVR_DIAG_HSCLK_SEL_LANE2    0x44e6
#define XCVR_DIAG_HSCLK_SEL_LANE3    0x46e6

#define XCVR_DIAG_HSCLK_DIV_LANE0    0x40e7
#define XCVR_DIAG_HSCLK_DIV_LANE1    0x42e7
#define XCVR_DIAG_HSCLK_DIV_LANE2    0x44e7
#define XCVR_DIAG_HSCLK_DIV_LANE3    0x46e7

#define XCVR_DIAG_PLLDRC_CTRL_LANE0  0x40e5
#define XCVR_DIAG_PLLDRC_CTRL_LANE1  0x42e5
#define XCVR_DIAG_PLLDRC_CTRL_LANE2  0x44e5
#define XCVR_DIAG_PLLDRC_CTRL_LANE3  0x46e5

#define XCVR_DIAG_BIDI_CTRL_LANE0    0x40ea
#define XCVR_DIAG_BIDI_CTRL_LANE1    0x42ea
#define XCVR_DIAG_BIDI_CTRL_LANE2    0x44ea
#define XCVR_DIAG_BIDI_CTRL_LANE3    0x46ea

#define CMN_PLL0_DSM_DIAG_M0         0x0094
#define CMN_PLL1_DSM_DIAG_M0         0x00d4
#define CMN_PDIAG_PLL0_CP_PADJ_M0    0x01a4
#define CMN_PDIAG_PLL1_CP_PADJ_M0    0x01c4

#define CMN_PDIAG_PLL0_CP_IADJ_M0    0x01a5
#define CMN_PDIAG_PLL1_CP_IADJ_M0    0x01c5
#define CMN_PDIAG_PLL0_FILT_PADJ_M0  0x01a6
#define CMN_PDIAG_PLL1_FILT_PADJ_M0  0x01c6

#define CMN_PLL0_INTDIV_M0           0x0090
#define CMN_PLL1_INTDIV_M0           0x00d0
#define CMN_PLL0_FRACDIVL_M0         0x0091
#define CMN_PLL1_FRACDIVL_M0         0x00d1

#define CMN_PLL0_FRACDIVH_M0         0x0092
#define CMN_PLL1_FRACDIVH_M0         0x00d2
#define CMN_PLL0_HIGH_THR_M0         0x0093
#define CMN_PLL1_HIGH_THR_M0         0x00d3

#define CMN_PDIAG_PLL0_CTRL_M0       0x01a0
#define CMN_PDIAG_PLL1_CTRL_M0       0x01c0

#define CMN_PLL0_VCOCAL_TCTRL        0x0082
#define CMN_PLL1_VCOCAL_TCTRL        0x00c2

#define CMN_PLL0_VCOCAL_INIT_TMR     0x0084
#define CMN_PLL1_VCOCAL_INIT_TMR     0x00c4

#define CMN_PLL0_VCOCAL_ITER_TMR     0x0085
#define CMN_PLL1_VCOCAL_ITER_TMR     0x00c5

#define CMN_PLL0_VCOCAL_REFTIM_START 0x0086
#define CMN_PLL1_VCOCAL_REFTIM_START 0x00c6

#define CMN_PLL0_VCOCAL_PLLCNT_START 0x0088
#define CMN_PLL1_VCOCAL_PLLCNT_START 0x00c8

#define CMN_PLL0_LOCK_REFCNT_START   0x009c
#define CMN_PLL1_LOCK_REFCNT_START   0x00dc

#define CMN_PLL0_LOCK_PLLCNT_START   0x009e
#define CMN_PLL1_LOCK_PLLCNT_START   0x00de

#define CMN_PLL0_LOCK_PLLCNT_THR     0x009f
#define CMN_PLL1_LOCK_PLLCNT_THR     0x00df

#define PHY_PMA_LANE_MAP             0xc010
#define PHY_LANE_OFF_CTRL            0xc011

#define PHY_PIPE_USB3_GEN2_PRE_CFG0  0xc018
#define PHY_PIPE_USB3_GEN2_POST_CFG0 0xc01a
#define PHY_PIPE_USB3_GEN2_POST_CFG1 0xc01b

#define CMN_CDIAG_CDB_PWRI_OVRD      0x0041
#define CMN_CDIAG_XCVRC_PWRI_OVRD    0x0047

#define TX_PSC_A0_LANE0              0x4100
#define TX_PSC_A0_LANE1              0x4300
#define TX_PSC_A0_LANE2              0x4500
#define TX_PSC_A0_LANE3              0x4700

#define TX_PSC_A1_LANE0              0x4101
#define TX_PSC_A1_LANE1              0x4301
#define TX_PSC_A1_LANE2              0x4501
#define TX_PSC_A1_LANE3              0x4701

#define TX_PSC_A2_LANE0              0x4102
#define TX_PSC_A2_LANE1              0x4302
#define TX_PSC_A2_LANE2              0x4502
#define TX_PSC_A2_LANE3              0x4702

#define TX_PSC_A3_LANE0              0x4103
#define TX_PSC_A3_LANE1              0x4303
#define TX_PSC_A3_LANE2              0x4503
#define TX_PSC_A3_LANE3              0x4703

#define RX_PSC_A0_LANE0              0x8000
#define RX_PSC_A0_LANE1              0x8200
#define RX_PSC_A0_LANE2              0x8400
#define RX_PSC_A0_LANE3              0x8600

#define RX_PSC_A1_LANE0              0x8001
#define RX_PSC_A1_LANE1              0x8201
#define RX_PSC_A1_LANE2              0x8401
#define RX_PSC_A1_LANE3              0x8601

#define RX_PSC_A2_LANE0              0x8002
#define RX_PSC_A2_LANE1              0x8202
#define RX_PSC_A2_LANE2              0x8402
#define RX_PSC_A2_LANE3              0x8602

#define RX_PSC_A3_LANE0              0x8003
#define RX_PSC_A3_LANE1              0x8203
#define RX_PSC_A3_LANE2              0x8403
#define RX_PSC_A3_LANE3              0x8603

#define RX_PSC_CAL_LANE0             0x8006
#define RX_PSC_CAL_LANE1             0x8206
#define RX_PSC_CAL_LANE2             0x8406
#define RX_PSC_CAL_LANE3             0x8606

#define TX_TXCC_CTRL_LANE0           0x4040
#define TX_TXCC_CTRL_LANE1           0x4240
#define TX_TXCC_CTRL_LANE2           0x4440
#define TX_TXCC_CTRL_LANE3           0x4640

#define TX_TXCC_CPOST_MULT_00_LANE0  0x404c
#define TX_TXCC_CPOST_MULT_00_LANE1  0x424c
#define TX_TXCC_CPOST_MULT_00_LANE2  0x444c
#define TX_TXCC_CPOST_MULT_00_LANE3  0x464c

#define TX_TXCC_CPOST_MULT_01_LANE0  0x404d
#define TX_TXCC_CPOST_MULT_01_LANE1  0x424d
#define TX_TXCC_CPOST_MULT_01_LANE2  0x444d
#define TX_TXCC_CPOST_MULT_01_LANE3  0x464d

#define TX_TXCC_MGNFS_MULT_000_LANE0 0x4050
#define TX_TXCC_MGNFS_MULT_000_LANE1 0x4250
#define TX_TXCC_MGNFS_MULT_000_LANE2 0x4450
#define TX_TXCC_MGNFS_MULT_000_LANE3 0x4650

#define TX_DIAG_ACYA_LANE0           0x41E7
#define TX_DIAG_ACYA_LANE1           0x43E7
#define TX_DIAG_ACYA_LANE2           0x45E7
#define TX_DIAG_ACYA_LANE3           0x47E7

#define DRV_DIAG_TX_DRV_LANE0        0x40c6
#define DRV_DIAG_TX_DRV_LANE1        0x42c6
#define DRV_DIAG_TX_DRV_LANE2        0x44c6
#define DRV_DIAG_TX_DRV_LANE3        0x46c6

#define RX_SIGDET_HL_FILT_TMR_LANE0  0x8090
#define RX_SIGDET_HL_FILT_TMR_LANE1  0x8290
#define RX_SIGDET_HL_FILT_TMR_LANE2  0x8490

#define RX_REE_GCSM1_CTRL_LANE0      0x8108
#define RX_REE_GCSM1_CTRL_LANE1      0x8308
#define RX_REE_GCSM1_CTRL_LANE2      0x8508
#define RX_REE_GCSM1_CTRL_LANE3      0x8708

#define RX_REE_GCSM2_CTRL_LANE0      0x8110
#define RX_REE_GCSM2_CTRL_LANE1      0x8310
#define RX_REE_GCSM2_CTRL_LANE2      0x8510
#define RX_REE_GCSM2_CTRL_LANE3      0x8710

#define RX_REE_PERGCSM_CTRL_LANE0    0x8118
#define RX_REE_PERGCSM_CTRL_LANE1    0x8318
#define RX_REE_PERGCSM_CTRL_LANE2    0x8518
#define RX_REE_PERGCSM_CTRL_LANE3    0x8718

#define RX_REE_ATTEN_THR_LANE0       0x8149
#define RX_REE_ATTEN_THR_LANE1       0x8349
#define RX_REE_ATTEN_THR_LANE2       0x8549
#define RX_REE_ATTEN_THR_LANE3       0x8749

#define RX_REE_SMGM_CTRL1_LANE0      0x8177
#define RX_REE_SMGM_CTRL1_LANE1      0x8377
#define RX_REE_SMGM_CTRL1_LANE2      0x8577

#define RX_REE_SMGM_CTRL2_LANE0      0x8178
#define RX_REE_SMGM_CTRL2_LANE1      0x8378
#define RX_REE_SMGM_CTRL2_LANE2      0x8578

#define XCVR_DIAG_PSC_OVRD_LANE0     0x40eb
#define XCVR_DIAG_PSC_OVRD_LANE1     0x42eb
#define XCVR_DIAG_PSC_OVRD_LANE2     0x44eb

#define RX_DIAG_SIGDET_TUNE_LANE0    0x81e8
#define RX_DIAG_SIGDET_TUNE_LANE1    0x83e8
#define RX_DIAG_SIGDET_TUNE_LANE2    0x85e8

#define RX_DIAG_NQST_CTRL_LANE0      0x81e5
#define RX_DIAG_NQST_CTRL_LANE1      0x83e5
#define RX_DIAG_NQST_CTRL_LANE2      0x85e5

#define RX_DIAG_DFE_AMP_TUNE_2_LANE0 0x81e2
#define RX_DIAG_DFE_AMP_TUNE_2_LANE1 0x83e2
#define RX_DIAG_DFE_AMP_TUNE_2_LANE2 0x85e2

#define RX_DIAG_DFE_AMP_TUNE_3_LANE0 0x81e3
#define RX_DIAG_DFE_AMP_TUNE_3_LANE1 0x83e3
#define RX_DIAG_DFE_AMP_TUNE_3_LANE2 0x85e3

#define RX_DIAG_PI_CAP_LANE0         0x81f5
#define RX_DIAG_PI_CAP_LANE1         0x83f5
#define RX_DIAG_PI_CAP_LANE2         0x85f5

#define RX_DIAG_PI_RATE_LANE0        0x81f4
#define RX_DIAG_PI_RATE_LANE1        0x83f4
#define RX_DIAG_PI_RATE_LANE2        0x85f4

#define RX_DIAG_ACYA_LANE0           0x81ff
#define RX_DIAG_ACYA_LANE1           0x83ff
#define RX_DIAG_ACYA_LANE2           0x85ff
#define RX_DIAG_ACYA_LANE3           0x87ff

#define RX_CDRLF_CNFG_LANE0          0x8080
#define RX_CDRLF_CNFG_LANE1          0x8280
#define RX_CDRLF_CNFG_LANE2          0x8480
#define RX_CDRLF_CNFG_LANE3          0x8680

#define RX_CDRLF_CNFG3_LANE0         0x8082
#define RX_CDRLF_CNFG3_LANE1         0x8282
#define RX_CDRLF_CNFG3_LANE2         0x8482
#define RX_CDRLF_CNFG3_LANE3         0x8682

#define PHY_PMA_CMN_CTRL1            0xe000
#define PHY_PMA_CMN_CTRL2            0xe001
#define PHY_PMA_PLL_CTRL             0xe003
#define PHY_PMA_ISO_CMN_CTRL         0xe004

#define PHY_ISO_CMN_CTRL1            0xc008
#define PHY_ISO_CMN_CTRL2            0xc009
#define PHY_ISO_CMN_CTRL3            0xc00a
#define PHY_PMA_ISO_PLL_CTRL0        0xe005
#define PHY_PMA_ISO_PLL_CTRL1        0xe006
#define PHY_PMA_ISOLATION_CTRL       0xe00f
#define PHY_PMA_ISO_XCVR_CTRL        0xf003

#define LANE0_OFFSET                 0x0000
#define LANE1_OFFSET                 0x0200
#define LANE2_OFFSET                 0x0400
#define LANE3_OFFSET                 0x0600
#define LANE_OFFSET                  0x0200

// Bits
#define PHY_PMA_CMN_CTRL2_PLL0_READY      BIT(0)
#define PHY_PMA_CMN_CTRL2_PLL1_READY      BIT(1)
#define PHY_PMA_CMN_CTRL2_PLL0_DISABLED   BIT(2)
#define PHY_PMA_CMN_CTRL2_PLL1_DISABLED   BIT(3)
#define PHY_PMA_CMN_CTRL2_PLL0_CLK_EN_ACK BIT(4)
#define PHY_PMA_CMN_CTRL2_PLL1_CLK_EN_ACK BIT(5)
#define PHY_PMA_CMN_CTRL2_PLL0_LOCKED     BIT(6)
#define PHY_PMA_CMN_CTRL2_PLL1_LOCKED     BIT(7)

#define PHY_PLL_CFG_0803              0xC00E
// 0803 phy register
#define PHY_PIPE_USB3_GEN2_PRE_CFG0_0803	0xc01c
#define PHY_PIPE_USB3_GEN2_POST_CFG0_0803	0xc01e
#define PHY_PIPE_USB3_GEN2_POST_CFG1_0803	0xc01f

//gop status address
#define GOP_STATUS_ADDRESS 0x83E05000
#define GOP_STATUS_SIZE 0x04

struct gop_status{
	unsigned char phy_status[4];
};

/*
* 0: usb
* 1: 2 lane usb+ 2 lane dp
* 2: usb device
* 3: usb2.0 + 4 lane dp
*/
enum phy_role {
	USB_ROLE_NONE,
	USB_ROLE_HOST,
	USB_ROLE_DEVICE,
	USB_ROLE_HOST_20,
};

#endif
