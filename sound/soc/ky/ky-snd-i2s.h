// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 KY
 *
 */

#ifndef _SPI_KY_H
#define _SPI_KY_H

/* KY AQUILA SPI Registers */
#define TOP_CTRL		0x00	/* SSP Top Control Register */
#define FIFO_CTRL		0x04	/* SSP FIFO Control Register */
#define INT_EN			0x08    /* SSP Interrupt Enable Register */
#define TO			0x0C    /* SSP Time Out Register */
#define DATAR			0x10    /* SSP Data Register */
#define STATUS			0x14    /* SSP Stauts Register */
#define PSP_CTRL		0x18    /* SSP Programmable Serial Protocal Control Register */
#define NET_WORK_CTRL		0x1C    /* SSP NET Work Control Register */
#define NET_WORK_STATUS		0x20    /* SSP Net Work Status Register */
#define RWOT_CTRL		0x24    /* SSP RWOT Control Register */
#define RWOT_CCM		0x28    /* SSP RWOT Counter Cycles Match Register */
#define RWOT_CVWRn		0x2C    /* SSP RWOT Counter Value Write for Read Request Register */

/* 0x00 TOP_CTRL */
#define TOP_TTELP		(1 << 18)
#define TOP_TTE			(1 << 17)
#define TOP_SCFR		(1 << 16)
#define TOP_IFS			(1 << 15)
#define TOP_HOLD_FRAME_LOW	(1 << 14)
#define TOP_TRAIL_PXA	(0 << 13)
#define TOP_TRAIL_DMA   (1 << 13)
#define TOP_LBM			(1 << 12)
#define TOP_SPH1		(1 << 11)
#define TOP_SPH0		(1 << 11)
#define TOP_SPO1		(1 << 10)
#define TOP_SPO0		(0 << 10)
#define DW_8BYTE        (0x7<<5)   //SSP_TOP_CTRL[9:5]
#define DW_16BYTE       (0xf<<5)   //SSP_TOP_CTRL[9:5] 
#define DW_18BYTE       (0x11<<5)  //SSP_TOP_CTRL[9:5]
#define DW_32BYTE       (0x1f<<5)  //SSP_TOP_CTRL[9:5]
//#define TOP_DSS(x)		((x - 1) << 5)
//#define TOP_DSS_MASK		(0x1F << 5)
#define TOP_SFRMDIR_S		(1 << 4)
#define TOP_SFRMDIR_M		(0 << 4)
#define TOP_SCLKDIR_S		(1 << 3)
#define TOP_SCLKDIR_M		(0 << 3)
#define TOP_FRF_MASK		(0x3 << 1)
#define TOP_FRF_Motorola	(0x0 << 1)	/* Motorola's Serial Peripheral Interface (SPI) */
#define TOP_FRF_TI		(0x1 << 1)	/* Texas Instruments' Synchronous Serial Protocol (SSP) */
#define TOP_FRF_National	(0x2 << 1)	/* National Microwire */
#define TOP_FRF_PSP		(0x3 << 1)	/* Programmable Serial Protocol(PSP) */
#define TOP_SSE			(1 << 0)

/* 0x04 FIFO_CTRL */
#define FIFO_STRF			(1 << 19)
#define FIFO_EFWR			(1 << 18)
#define FIFO_RXFIFO_AUTO_FULL_CTRL	(1 << 17)
#define FIFO_FPCKE			(1 << 16)
#define FIFO_UNPACKING      (0 << 16)
#define FIFO_TXFIFO_WR_ENDIAN_MASK	(0x3 << 14)
#define FIFO_RXFIFO_RD_ENDIAN_MASK	(0x3 << 12)
#define FIFO_WR_ENDIAN_16BITS		(1 << 14)	/* Swap first 16 bits and last 16 bits */
#define FIFO_WR_ENDIAN_8BITS		(2 << 14)	/* Swap all 4 bytes */
#define FIFO_RD_ENDIAN_16BITS		(1 << 12)	/* Swap first 16 bits and last 16 bits */
#define FIFO_RD_ENDIAN_8BITS		(2 << 12)	/* Swap all 4 bytes */
#define FIFO_RSRE			(1 << 11)
#define FIFO_TSRE			(1 << 10)
#define FIFO_RX_THRES_15                (0xf<<5)   //SSP_FIFO_CTRL[9:5]
#define FIFO_RX_THRES_7                 (0x7<<5)   //SSP_FIFO_CTRL[9:5]
#define FIFO_TX_THRES_15                (0xf<<0)   //SSP_FIFO_CTRL[4:0]
#define FIFO_TX_THRES_7                 (0x7<<0)   //SSP_FIFO_CTRL[4:0]

/* 0x08 INT_EN */
#define INT_EN_EBCEI		(1 << 6)
#define INT_EN_TIM		(1 << 5)
#define INT_EN_RIM		(1 << 4)
#define INT_EN_TIE		(1 << 3)
#define INT_EN_RIE		(1 << 2)
#define INT_EN_TINTE		(1 << 1)
#define INT_EN_PINTE		(1 << 0)

/* 0x0C TO */
#define TIMEOUT(x)	((x) << 0)

/* 0x10 DATAR */
#define DATA(x)		((x) << 0)

/* 0x14 STATUS */
#define STATUS_OSS		(1 << 23)
#define STATUS_TX_OSS		(1 << 22)
#define STATUS_BCE		(1 << 21)
#define STATUS_ROR		(1 << 20)
#define STATUS_RNE		(1 << 14)
#define STATUS_RFS		(1 << 13)
#define STATUS_TUR		(1 << 12)
#define STATUS_TNF		(1 << 6)
#define STATUS_TFS		(1 << 5)
#define STATUS_EOC		(1 << 4)
#define STATUS_TINT		(1 << 3)
#define STATUS_PINT		(1 << 2)
#define STATUS_CSS		(1 << 1)
#define STATUS_BSY		(1 << 0)

/* 0x18 PSP_CTRL */
#define PSP_EDMYSTOP(x)		((x) << 27)
#define PSP_EMYSTOP(x)		((x) << 25)
#define PSP_EDMYSTRT(x)		((x) << 23)
#define PSP_DMYSTRT(x)		((x) << 21)
#define PSP_STRTDLY(x)		((x) << 18)
#define PSP_SFRMWDTH(x)		((x) << 12)
#define PSP_SFRMDLY(x)		((x) << 5)
#define PSP_SFRMP		(1 << 4)
#define PSP_FSRT		(1 << 3)
#define PSP_ETDS		(1 << 2)
#define PSP_SCMODE(x)		((x) << 0)

/* 0x1C NET_WORK_CTRL */
#define RTSA(x)			((x) << 12)
#define RTSA_MASK		(0xFF << 12)
#define TTSA(x)			((x) << 4)
#define TTSA_MASK		(0xFF << 4)
#define NET_FRDC(x)		((x) << 1)
#define NET_WORK_MODE		(1 << 0)

/* 0x20 NET_WORK_STATUS */
#define NET_SATUS_NMBSY		(1 << 3)
#define NET_STATUS_TSS(x)	((x) << 0)

/* 0x24 RWOT_CTRL */
#define RWOT_MASK_RWOT_LAST_SAMPLE	(1 << 4)
#define RWOT_CLR_RWOT_CYCLE		(1 << 3)
#define RWOT_SET_RWOT_CYCLE		(1 << 2)
#define RWOT_CYCLE_RWOT_EN		(1 << 1)
#define RWOT_RWOT			(1 << 0)
#endif /* _SPI_KY_H */
