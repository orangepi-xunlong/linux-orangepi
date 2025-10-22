/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SUNXI SPI Controller BIT Register Definition
 *
 */

#ifndef _SUNXI_SPI_BIT_H_
#define _SUNXI_SPI_BIT_H_

#include <linux/ctype.h>
#include <linux/spi/spi.h>
#include "../spi-sunxi.h"

#define SUNXI_SPI_BIT_MAX_LEN	(32)

/* SPI Bit-Aligned Transfer Configure Register */
#define SUNXI_SPI_BATC_REG	(0x40)
	#define SUNXI_SPI_BATC_TCE			BIT(31)			/* Transfer Control Enable */
	#define SUNXI_SPI_BATC_MSMS			BIT(30)			/* Master Sample Standard */
	#define SUNXI_SPI_BATC_TBC			BIT(25)			/* Transfer Bits Completed */
	#define SUNXI_SPI_BATC_TBC_INT_EN	BIT(24)			/* Transfer Bits Completed Interrupt Enable */
	#define SUNXI_SPI_BATC_RX_FRM_LEN	GENMASK(21, 16)	/* Configure the length of serial data frame(burst) of RX */
	#define SUNXI_SPI_BATC_TX_FRM_LEN	GENMASK(13, 8)	/* Configure the length of serial data frame(burst) of TX */
	#define SUNXI_SPI_BATC_SS_LEVEL		BIT(7)			/* SS Signal Level Output */
	#define SUNXI_SPI_BATC_SS_OWNER		BIT(6)			/* SS Output Owner Select */
	#define SUNXI_SPI_BATC_SPOL			BIT(5)			/* SPI Chip Select Signal Polarity Control */
	#define SUNXI_SPI_BATC_SS_SEL		GENMASK(3, 2)	/* SPI Chip Select */
	#define SUNXI_SPI_BATC_WMS			GENMASK(1, 0)	/* Work Mode Select */

/* SPI TX Bit Register */
#define SUNXI_SPI_TB_REG	(0x48)

/* SPI RX Bit Register */
#define SUNXI_SPI_RB_REG	(0x4C)

#define SUNXI_SPI_BIT_3WIRE_MODE	(2)
#define SUNXI_SPI_BIT_STD_MODE		(3)

#if IS_ENABLED(CONFIG_AW_SPI_NG_BIT)
u32 sunxi_spi_bit_qry_irq_pending(struct sunxi_spi *sspi);
void sunxi_spi_bit_clr_irq_pending(struct sunxi_spi *sspi);
void sunxi_spi_bit_enable_irq(struct sunxi_spi *sspi);
void sunxi_spi_bit_disable_irq(struct sunxi_spi *sspi);
void sunxi_spi_bit_ss_owner(struct sunxi_spi *sspi, u32 owner);
void sunxi_spi_bit_set_cs(struct spi_device *spi, bool status);
void sunxi_spi_bit_sample_delay(struct sunxi_spi *sspi, u32 clk);
void sunxi_spi_bit_config_tc(struct sunxi_spi *sspi, u32 config);
int sunxi_spi_mode_check_bit(struct sunxi_spi *sspi, struct spi_transfer *t);
void sunxi_spi_bit_handler(struct sunxi_spi *sspi);
int sunxi_spi_xfer_bit(struct spi_device *spi, struct spi_transfer *t);
#else
static inline u32 sunxi_spi_bit_qry_irq_pending(struct sunxi_spi *sspi) { return -EINVAL; };
static inline void sunxi_spi_bit_clr_irq_pending(struct sunxi_spi *sspi) {};
static inline void sunxi_spi_bit_enable_irq(struct sunxi_spi *sspi) {};
static inline void sunxi_spi_bit_disable_irq(struct sunxi_spi *sspi) {};
static inline void sunxi_spi_bit_ss_owner(struct sunxi_spi *sspi, u32 owner) {};
static inline void sunxi_spi_bit_set_cs(struct spi_device *spi, bool status) {};
static inline void sunxi_spi_bit_sample_delay(struct sunxi_spi *sspi, u32 clk) {};
static inline void sunxi_spi_bit_config_tc(struct sunxi_spi *sspi, u32 config) {};
static inline int sunxi_spi_mode_check_bit(struct sunxi_spi *sspi, struct spi_transfer *t) { return -EINVAL; };
static inline void sunxi_spi_bit_handler(struct sunxi_spi *sspi) {};
static inline int sunxi_spi_xfer_bit(struct spi_device *spi, struct spi_transfer *t) { return -EINVAL; };
#endif

#endif /* _SUNXI_SPI_BIT_H_ */