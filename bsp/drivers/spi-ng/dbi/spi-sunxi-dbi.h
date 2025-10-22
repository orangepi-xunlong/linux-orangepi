/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SUNXI SPI Controller DBI Register Definition
 *
 */

#ifndef _SUNXI_SPI_DBI_H_
#define _SUNXI_SPI_DBI_H_

#include "../spi-sunxi.h"

/* DBI Control Register 0 */
#define SUNXI_DBI_CTL0_REG		(0x100)
	#define SUNXI_DBI_CTL0_CMDT				BIT(31)	/* Command Type */
	#define SUNXI_DBI_CTL0_WCDC				GENMASK(30, 20)	/* Write Command Dummy Cycles */
	#define SUNXI_DBI_CTL0_DAT_SEQ			BIT(19)	/* Output Data Sequence */
	#define SUNXI_DBI_CTL0_RGB_SEQ			GENMASK(18, 16)	/* Output RGB Sequence */
	#define SUNXI_DBI_CTL0_TRAN_MOD			BIT(15)	/* Transmit Mode */
	#define SUNXI_DBI_CTL0_DAT_FMT			GENMASK(14, 12)	/* Output Data Format */
	#define SUNXI_DBI_CTL0_INTF				GENMASK(10, 8)	/* DBI Interface */
	#define SUNXI_DBI_CTL0_RGB_FMT			GENMASK(7, 4)	/* RGB Source Format */
	#define SUNXI_DBI_CTL0_DUM_VAL			BIT(3)	/* Dummy Cycle Value */
	#define SUNXI_DBI_CTL0_RGB_BO			BIT(2)	/* RGB Bit Order */
	#define SUNXI_DBI_CTL0_ELEMENT_A_POS	BIT(1)	/* Element A position, only For RGB32 Data Format */
	#define SUNXI_DBI_CTL0_VI_SRC_TYPE		BIT(0)	/* RGB Source Type */
	/* non-Register */
	#define SUNXI_DBI_RGB_SRC_TYPE_RGB32	(0)
	#define SUNXI_DBI_RGB_SRC_TYPE_RGB16	(1)

/* DBI Control Register 1 */
#define SUNXI_DBI_CTL1_REG		(0x104)
	#define SUNXI_DBI_CTL1_SOFT_TRG		BIT(31)
	#define SUNXI_DBI_CTL1_EN_MOD_SEL	GENMASK(30, 29)	/* DBI Enable Mode Select */
	#define SUNXI_DBI_CTL1_RGB666_FMT	GENMASK(27, 26)	/* 2 Data Lane RGB666 Format */
	#define SUNXI_DBI_CTL1_RXCLK_INV	BIT(25)	/* DBI RX clock inverse */
	#define SUNXI_DBI_CTL1_CLKO_MOD		BIT(24)	/* DBI output clock mode */
	#define SUNXI_DBI_CTL1_CLKO_INV		BIT(23)	/* DBI clock output inverse */
	#define SUNXI_DBI_CTL1_DCX_DATA		BIT(22)	/* DCX Data Value */
	#define SUNXI_DBI_CTL1_RGB16_SEL	BIT(21)	/* RGB 16 Data Source Select */
	#define SUNXI_DBI_CTL1_RDAT_LSB		BIT(20)	/* Bit Order of Read Data */
	#define SUNXI_DBI_CTL1_RCDC			GENMASK(15, 8)	/* Read Command Dummy Cycles */
	#define SUNXI_DBI_CTL1_RDBN			GENMASK(7, 0)	/* Read Data Number of Bytes */

/* DBI Control Register 2 */
#define SUNXI_DBI_CTL2_REG		(0x108)
	#define SUNXI_DBI_CTL2_FIFO_DRQ_EN		BIT(15)	/* DBI FIFO DMA Request Enable */
	#define SUNXI_DBI_CTL2_TX_TRIG_LEVEL	GENMASK(14, 8)	/* DBI FIFO Empty Request Trigger Level */
	#define SUNXI_DBI_CTL2_SDI_OUT_SEL		BIT(6)	/* DBI SDI PIN Output select */
	#define SUNXI_DBI_CTL2_DCX_SEL			BIT(5)	/* DBI SDX PIN Function select */
	#define SUNXI_DBI_CTL2_SDI_SEL			GENMASK(4, 3)	/* DBI SDI PIN Function select */
	#define SUNXI_DBI_CTL2_TE_DBC_SEL		BIT(2)	/* TE debounce function select */
	#define SUNXI_DBI_CTL2_TE_TRIG_SEL		BIT(1)	/* TE edge trigger select */
	#define SUNXI_DBI_CTL2_TE_EN			BIT(0)	/* TE enable */

/* DBI Timer Register */
#define SUNXI_DBI_TIMER_REG		(0x10C)
	#define SUNXI_DBI_TIMER_TM_EN	BIT(31)			/* DBI Timer enable */
	#define SUNXI_DBI_TIMER_TM_VAL	GENMASK(30, 0)	/* DBI Timer value */

/* DBI Video Size Register */
#define SUNXI_DBI_VIDEO_SIZE_REG	(0x110)
	#define SUNXI_DBI_VIDEO_SIZE_V		GENMASK(26, 16)	/* V Frame int */
	#define SUNXI_DBI_VIDEO_SIZE_H		GENMASK(10, 0)	/* H Line int */

/* DBI Interrupter Register */
#define SUNXI_DBI_INT_REG		(0x120)
	#define SUNXI_DBI_FIFO_EMPTY_INT		BIT(14)	/* DBI FIFO Empty Interrupt Status */
	#define SUNXI_DBI_FIFO_FULL_INT			BIT(13)	/* DBI FIFO Full Interrupt Status */
	#define SUNXI_DBI_TIMER_INT				BIT(12)	/* Timer Interrupt Status */
	#define SUNXI_DBI_RD_DONE_INT			BIT(11)	/* Read Done Interrupt Status */
	#define SUNXI_DBI_TE_INT				BIT(10)	/* TE Interrupt Status */
	#define SUNXI_DBI_FRAM_DONE_INT			BIT(9)	/* Frame Done Interrupt Status */
	#define SUNXI_DBI_LINE_DONE_INT			BIT(8)	/* Line Done Interrupt Status */
	#define SUNXI_DBI_FIFO_EMPTY_INT_EN		BIT(6)	/* DBI FIFO Empty Interrupt Enable */
	#define SUNXI_DBI_FIFO_FULL_INT_EN		BIT(5)	/* DBI FIFO Full Interrupt Enable */
	#define SUNXI_DBI_TIMER_INT_EN			BIT(4)	/* Timer Interrupt Enable */
	#define SUNXI_DBI_RD_DONE_INT_EN		BIT(3)	/* Read Done Interrupt Enable */
	#define SUNXI_DBI_TE_INT_EN				BIT(2)	/* TE Interrupt Enable */
	#define SUNXI_DBI_FRAM_DONE_INT_EN		BIT(1)	/* Frame Done Interrupt Enable */
	#define SUNXI_DBI_LINE_DONE_INT_EN		BIT(0)	/* Line Done Interrupt Enable */
	#define SUNXI_DBI_INT_STA_MASK			GENMASK(14, 8)
	#define SUNXI_DBI_INT_EN_MASK			GENMASK(6, 0)

/* DBI debug Register 0 */
#define SUNXI_DBI_DEBUG_REG0	(0x124)

/* DBI debug Register 1 */
#define SUNXI_DBI_DEBUG_REG1	(0x128)

#if IS_ENABLED(CONFIG_AW_SPI_NG_DBI)
void sunxi_dbi_disable_dma(struct sunxi_spi *sspi);
void sunxi_dbi_enable_dma(struct sunxi_spi *sspi);
u32 sunxi_dbi_qry_irq_pending(struct sunxi_spi *sspi);
void sunxi_dbi_clr_irq_pending(struct sunxi_spi *sspi, u32 bitmap);
u32 sunxi_dbi_qry_irq_enable(struct sunxi_spi *sspi);
void sunxi_dbi_disable_irq(struct sunxi_spi *sspi, u32 bitmap);
void sunxi_dbi_enable_irq(struct sunxi_spi *sspi, u32 bitmap);
void sunxi_dbi_config(struct sunxi_spi *sspi);
int sunxi_dbi_mode_check_dbi(struct sunxi_spi *sspi, struct spi_transfer *t);
void sunxi_dbi_handler(struct sunxi_spi *sspi);
#else
static inline void sunxi_dbi_disable_dma(struct sunxi_spi *sspi) {};
static inline void sunxi_dbi_enable_dma(struct sunxi_spi *sspi) {};
static inline u32 sunxi_dbi_qry_irq_pending(struct sunxi_spi *sspi) { return -EINVAL; };
static inline void sunxi_dbi_clr_irq_pending(struct sunxi_spi *sspi, u32 bitmap) {};
static inline u32 sunxi_dbi_qry_irq_enable(struct sunxi_spi *sspi) { return -EINVAL; };
static inline void sunxi_dbi_disable_irq(struct sunxi_spi *sspi, u32 bitmap) {};
static inline void sunxi_dbi_enable_irq(struct sunxi_spi *sspi, u32 bitmap) {};
static inline void sunxi_dbi_config(struct sunxi_spi *sspi) {};
static inline int sunxi_dbi_mode_check_dbi(struct sunxi_spi *sspi, struct spi_transfer *t) { return -EINVAL; };
static inline void sunxi_dbi_handler(struct sunxi_spi *sspi) {};
#endif

#endif /* _SUNXI_SPI_DBI_H_ */