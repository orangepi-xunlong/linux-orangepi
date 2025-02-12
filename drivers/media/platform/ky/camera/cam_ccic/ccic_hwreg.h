/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ccic_hwreg.h - hw register for ccic
 *
 * Copyright (C) 2023 KY Micro Limited
 */

#ifndef __CCIC_HWREG_H__
#define __CCIC_HWREG_H__

#ifndef BIT
#define BIT(nr)			(1 << (nr))
#endif
#define REG_Y0BAR	0x00
#define REG_U0BAR	0x0c
#define REG_V0BAR	0x18

#define	REG_IMGPITCH	0x24	/* Image pitch register */
#define	IMGP_YP_SHFT	0	/* Y pitch params */
#define	IMGP_YP_MASK	0x00003fff	/* Y pitch field */
#define	IMGP_UVP_SHFT	16	/* UV pitch (planar) */
#define	IMGP_UVP_MASK	0x3fff0000

#define	REG_IRQSTATRAW	0x28	/* RAW IRQ Status */
#define	REG_IRQMASK		0x2c	/* IRQ mask - same bits as IRQSTAT */
#define	REG_IRQSTAT		0x30	/* IRQ status / clear */
#define	IRQ_DMA_EOF					(BIT(0))	/* DMA EOF IRQ */
#define	IRQ_DMA_SOF					(BIT(1))	/* DMA SOF IRQ */
#define	IRQ_CSI_EOF					(BIT(2))	/* CSI EOF IRQ */
#define	IRQ_CSI_SOF					(BIT(3))	/* CSI SOF IRQ */
#define	IRQ_DMA_NOT_DONE			(BIT(4))	/* DMA not done at frame start IRQ */
#define	IRQ_SHADOW_NOT_RDY			(BIT(5))	/* Shadow bit not ready at frame start IRQ */
#define	IRQ_DMA_OVERFLOW			(BIT(6))	/* FIFO full IRQ */
#define	IRQ_DMA_PRO_LINE			(BIT(7))	/* CCIC Programmable Line IRQ */
#define	IRQ_IDI_PRO_LINE			(BIT(8))	/* IDI Programmable Line IRQ */
#define	IRQ_CSI2IDI_FLUSH			(BIT(9))	/* CSI2IDI DATA FLUSH IRQ */
#define	IRQ_CSI2IDI_HBLK2HSYNC		(BIT(10))	/* HBLK_TO_HSYNC IRQ, CSI2IDI module detects Hblank to Hsync gap too small */
#define	IRQ_DMA_WR_ERR				(BIT(11))	/* AXI Write Error IRQ */
#define	IRQ_DPHY_RX_CLKULPS_ACTIVE	(BIT(12))	/* DPHY Rx CLKULPS Active IRQ */
#define	IRQ_DPHY_RX_CLKULPS			(BIT(13))	/* DPHY Rx CLKULPS IRQ */
#define	IRQ_DPHY_LN_ULPS_ACTIVE		(BIT(14))	/* DPHY Lane ULPS Active IRQ */
#define	IRQ_DPHY_LN_ERR_CTL			(BIT(15))	/* DPHY Lane Error Control IRQ */
#define	IRQ_DPHY_LN_TX_SYNC_ERR		(BIT(16))	/* DPHY Lane Start of Transmission Sync Error IRQ */
#define	IRQ_DPHY_LN_TX_ERR			(BIT(17))	/* DPHY Lane Start of Transmission Error IRQ */
#define	IRQ_DPHY_LN_RX_ERR			(BIT(18))	/* DPHY receiver Line Error IRQ */
#define	IRQ_DPCM_REPACK_ERR			(BIT(19))	/* DPCM/Repack IRQ */

#define	IRQ_CSI2PACKET_ERR			(BIT(23))
#define	IRQ_CSI2CRC_ERR				(BIT(24))
#define	IRQ_CSI2ECC2BIT_ERR			(BIT(25))
#define	IRQ_CSI2PATIRY_ERR			(BIT(26))
#define	IRQ_CSI2ECCCORRECTABLE_ERR	(BIT(27))
#define	IRQ_CSI2LANEFIFOOVERRUN_ERR	(BIT(28))
#define	IRQ_CSI2PARSE_ERR			(BIT(29))
#define	IRQ_CSI2GENSHORTPACKVALID	(BIT(30))
#define	IRQ_CSI2GENSHORTPACK_ERR	(BIT(31))
// #define              FRAMEIRQS       (IRQ_CSI_SOF | IRQ_CSI_EOF  | IRQ_DMA_SOF | IRQ_DMA_EOF | IRQ_DMA_OVERFLOW | IRQ_DMA_NOT_DONE | IRQ_SHADOW_NOT_RDY)
#define	FRAMEIRQS		(IRQ_DMA_SOF | IRQ_DMA_EOF | IRQ_DMA_OVERFLOW | IRQ_DMA_NOT_DONE | IRQ_SHADOW_NOT_RDY)
#define	CSI2PHYERRS		(0xFF0B0000)
#define	ALLIRQS			(FRAMEIRQS | CSI2PHYERRS | IRQ_CSI2IDI_HBLK2HSYNC)

#define	REG_IMGSIZE		0x34	/* Image size */
#define	IMGSZ_V_MASK	0x1fff0000
#define	IMGSZ_V_SHIFT	16
#define	IMGSZ_H_MASK	0x00003fff
#define	IMGSZ_H_SHIFT	0

#define	REG_IMGOFFSET	0x38	/* IMage offset */

#define REG_CTRL0		0x3c	/* Control 0 */
#define	C0_ENABLE		0x00000001	/* Makes the whole thing go */
/* Mask for all the format bits */
#define	C0_DF_MASK		0x08dffffc
/* RGB ordering */
#define	C0_RGB4_RGBX	0x00000000
#define	C0_RGB4_XRGB	0x00000004
#define	C0_RGB4_BGRX	0x00000008
#define	C0_RGB4_XBGR	0x0000000c
#define	C0_RGB5_RGGB	0x00000000
#define	C0_RGB5_GRBG	0x00000004
#define	C0_RGB5_GBRG	0x00000008
#define	C0_RGB5_BGGR	0x0000000c
/* YUV4222 to 420 Semi-Planar Enable */
#define	C0_YUV420SP		0x00000010
/* Spec has two fields for DIN and DOUT, but they must match, so
   combine them here. */
#define	C0_DF_YUV		0x00000000	/* Data is YUV */
#define	C0_DF_RGB		0x000000a0	/* Data is RGB */
#define	C0_DF_BAYER		0x00000140	/* Data is Bayer */
/* 8-8-8 must be missing from the below - ask */
#define	C0_RGBF_565		0x00000000
#define	C0_RGBF_444		0x00000800
#define	C0_RGB_BGR		0x00001000	/* Blue comes first */
#define	C0_YUV_PLANAR	0x00000000	/* YUV 422 planar format */
#define	C0_YUV_PACKED	0x00008000	/* YUV 422 packed format */
#define	C0_YUV_420PL	0x0000a000	/* YUV 420 planar format */
/* Think that 420 packed must be 111 - ask */
#define	C0_YUVE_YUYV	0x00000000	/* Y1CbY0Cr */
#define	C0_YUVE_YVYU	0x00010000	/* Y1CrY0Cb */
#define	C0_YUVE_VYUY	0x00020000	/* CrY1CbY0 */
#define	C0_YUVE_UYVY	0x00030000	/* CbY1CrY0 */
#define	C0_YUVE_XYUV	0x00000000	/* 420: .YUV */
#define	C0_YUVE_XYVU	0x00010000	/* 420: .YVU */
#define	C0_YUVE_XUVY	0x00020000	/* 420: .UVY */
#define	C0_YUVE_XVUY	0x00030000	/* 420: .VUY */
/* Bayer bits 18,19 if needed */
#define	C0_HPOL_LOW		0x01000000	/* HSYNC polarity active low */
#define	C0_VPOL_LOW		0x02000000	/* VSYNC polarity active low */
#define	C0_VCLK_LOW		0x04000000	/* VCLK on falling edge */
#define	C0_420SP_UVSWAP	0x08000000	/* YUV420SP U/V Swap */
#define	C0_SIFM_MASK	0xc0000000	/* SIF mode bits */
#define	C0_SIF_HVSYNC	0x00000000	/* Use H/VSYNC */
#define	C0_SOF_NOSYNC	0x40000000	/* Use inband active signaling */
#define	C0_EOF_VSYNC	0x00400000	/* Generate EOF by VSYNC */
#define	C0_VEDGE_CTRL	0x00800000	/* Detecting falling edge of VSYNC */
/* bit 21: fifo overrun auto-recovery */
#define	C0_EOFFLUSH		0x00200000
/* bit 27 YUV420SP_UV_SWAP */
#define	C0_YUV420SP_UV_SWAP	0x08000000

#define	REG_CTRL1		0x40	/* Control 1 */
#define	C1_SENCLKGATE	0x00000001	/* Sensor Clock Gate */
#define	C1_RESVZ		0x0001fffe
#define	C1_DMAB_LENSEL	0x00020000	/* set 1, coupled CCICx */
#define	C1_444ALPHA		0x00f00000	/* Alpha field in RGB444 */
#define	C1_ALPHA_SHFT	20
#define	C1_AWCACHE		0x00100000	/* set 1. coupled CCICx */
#define	C1_DMAB64		0x00000000	/* 64-byte DMA burst */
#define	C1_DMAB128		0x02000000	/* 128-byte DMA burst */
#define	C1_DMAB256		0x04000000	/* 256-byte DMA burst */
#define	C1_DMAB_MASK	0x06000000
#define	C1_SHADOW_RDY	0x08000000	/* set it 1 when BAR is set */
#define	C1_PWRDWN		0x10000000	/* Power down */
#define	C1_DMAPOSTED	0x40000000	/* DMA Posted Select */

#define	REG_CTRL2		0x44	/* Control 2 */
/* recommend set 1 to disable legacy calc DMA line num */
#define	C2_LGCY_LNNUM	0x80000000
/* recommend set 1 to disaable legacy CSI2 hblank */
#define	C2_LGCY_HBLANK	0x40000000

#define	REG_CTRL3		0x48	/* Control 2 */

#define	REG_LNNUM		0x60	/* Lines num DMA filled */

#define	CLK_DIV_MASK	0x0000ffff	/* Upper bits RW "reserved" */
#define	REG_FRAME_CNT	0x23C

/* MIPI */
#define REG_CSI2_CTRL0	0x100
#define	CSI2_C0_ENABLE	0x01
#define	CSI2_C0_LANE_NUM(n)		(((n)-1) << 1)
#define	CSI2_C0_LANE_NUM_MASK	0x06
#define	CSI2_C0_EXT_TIM_ENA		(0x1 << 3)
#define	CSI2_C0_VLEN			(0x4 << 4)
#define	CSI2_C0_VLEN_MASK		(0xf << 4)
#define	CSI2_C0_VCDT_SEL		(0x1 << 13)
#define	REG_CSI2_VCCTRL	0x114
#define	CSI2_VCCTRL_MD_MASK		(0x3 << 0)
#define	CSI2_VCCTRL_MD_NORMAL	(0x0 << 0)
#define	CSI2_VCCTRL_MD_VC		(0x1 << 0)
#define	CSI2_VCCTRL_MD_DT		(0x2 << 0)
#define	CSI2_VCCTRL_VC0_MASK	(0x3 << 14)
#define	CSI2_VCCTRL_DT1_MASK	(0x3 << 16)
#define	CSI2_VCCTRL_VC1_MASK	(0x3 << 22)
#define	CSI2_VCCTRL_DT_ENABLE	(0x1 << 24)
#define REG_CSI2_DT_FLT			0x11c
#define	CSI2_DT_FLT0_MASK		(BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4)|BIT(5))
#define CSI2_DT_FLT0_SHIFT		(0)
#define	CSI2_DT_FLT0_EN			(BIT(6))
#define	CSI2_DT_FLT1_MASK		(BIT(8)|BIT(9)|BIT(10)|BIT(11)|BIT(12)|BIT(13))
#define CSI2_DT_FLT1_SHIFT		(8)
#define	CSI2_DT_FLT1_EN			(BIT(14))
#define	CSI2_DT_FLT2_MASK		(BIT(16)|BIT(17)|BIT(18)|BIT(19)|BIT(20)|BIT(21))
#define CSI2_DT_FLT2_SHIFT		(16)
#define	CSI2_DT_FLT2_EN			(BIT(22))
#define REG_CSI2_DPHY1	0x124
#define	CSI2_DHPY1_ANA_PU		(0x1 << 0)
#define	CSI2_DHPY1_BIF_EN		(0x1 << 1)
#define REG_CSI2_DPHY2	0x128
#define	CSI2_DPHY2_SEL_IREF(n)		((n & 0x03) << 30)
#define	CSI2_DPHY2_VTH_LPRX_H(n)	((n & 0x07) << 27)
#define	CSI2_DPHY2_VTH_LPRX_L(n)	((n & 0x07) << 24)
#define	CSI2_DPHY2_CK_ENA	0x00800000
#define	CSI2_DPHY2_CK_DELAY(n)		((n & 0x07) << 20)
#define	CSI2_DPHY2_LPRX_CTL	0x00080000
#define	CSI2_DPHY2_HSRX_TERM(n)		((n & 0x07) << 16)
#define	CSI2_DPHY2_CH2_ENA	0x00008000
#define	CSI2_DPHY2_CH2_DELAY(n)		((n & 0x07) << 12)
#define	CSI2_DPHY2_CH3_ENA	0x00000800
#define	CSI2_DPHY2_CH3_DELAY(n)		((n & 0x07) << 8)
#define	CSI2_DPHY2_CH0_ENA	0x00000080
#define	CSI2_DPHY2_CH0_DELAY(n)		((n & 0x07) << 4)
#define	CSI2_DPHY2_CH1_ENA	0x00000008
#define	CSI2_DPHY2_CH1_DELAY(n)		((n & 0x07) << 0)
#define REG_CSI2_DPHY3		0x12c
#define	CSI2_DPHY3_HS_SETTLE_SHIFT	8
#define REG_CSI2_DPHY4		0x130
#define	CSI2_DHPY4_BIF_EN	(0x1 << 23)
#define	CSI2_DPHY4_CHK_ENA	0x00000040
#define	CSI2_DPHY4_ERR_OUT_SEL		0x00000020
#define	CSI2_DPHY4_ERR_CLEAR		0x00000010
#define	CSI2_DPHY4_LANE_SEL(n)		((n & 0x03) << 2)
#define	CSI2_DPHY4_PATTERN_SEL(n)	((n & 0x03) << 0)
#define	REG_CSI2_DPHY5		0x134
#define	CSI2_DPHY5_LANE_RESC_ENA_SHIFT	4
#define	CSI2_DPHY5_LANE_ENA(n)		((1 << (n)) - 1)
#define	REG_CSI2_DPHY6		0x138
#define	CSI2_DPHY6_CK_SETTLE_SHIFT	8
#define	REG_CSI2_CTRL2		0x140
#define	CSI2_C2_MUX_SEL_MASK		0x06000000
#define	CSI2_C2_MUX_SEL_LOCAL_MAIN	0x00000000
#define	CSI2_C2_MUX_SEL_IPE2_VCDT	0x02000000
#define	CSI2_C2_MUX_SEL_IPE2_MAIN	0x04000000
#define	CSI2_C2_MUX_SEL_REMOTE_VCDT	0x06000000
#define	CSI2_C2_IDI_MUX_SEL_DPCM	0x01000000
#define	CSI2_C2_REPACK_RST			0x00200000
#define	CSI2_C2_REPACK_ENA			0x00010000
#define	CSI2_C2_DPCM_ENA			0x00000001
#define REG_CSI2_CTRL3		0x144
/* IDI */
#define REG_IDI_CTRL		0x310
#define	IDI_SEL_MASK		0x06
#define	IDI_SEL_DPCM_REPACK	0x00
#define	IDI_SEL_PARALLEL	0x02
#define	IDI_SEL_AHB			0x04
#define	IDI_RELEASE_RESET	0x01

#define REG_IDI_TRIG_LINE_NUM	0x330
#define REG_CSI_LANE_STATE_DBG	0x334

/* APMU */
#define REG_CLK_CCIC_RES	0x50
#define REG_CLK_CCIC2_RES	0x24

#define CF_SINGLE_BUF	0
#define CF_FRAME_SOF0	1
#define CF_FRAME_OVERFLOW	2

int ccic_csi2_config_dphy(struct ccic_dev *ccic_dev, int lanes, int enable);
int ccic_csi2_lanes_enable(struct ccic_dev *ccic_dev, int lanes);
//int ccic_csi2_vc_ctrl(struct ccic_dev *ccic_dev, int md, u8 vc0, u8 vc1);
int ccic_dma_src_sel(struct ccic_dev *ccic_dev, int sel, unsigned int main_ccic_id);
int ccic_dma_set_out_format(struct ccic_dev *ccic_dev, u32 pixfmt, u32 width, u32 height);
int ccic_dma_set_burst(struct ccic_dev *ccic_dev);
//void ccic_dma_enable(struct ccic_dev *ccic_dev, int en);
int ccic_csi2idi_src_sel(struct ccic_dev *ccic_dev, int sel);
void ccic_csi2idi_reset(struct ccic_dev *ccic_dev, int reset);
void ccic_hw_dump_regs(struct ccic_dev *ccic_dev);
#endif
