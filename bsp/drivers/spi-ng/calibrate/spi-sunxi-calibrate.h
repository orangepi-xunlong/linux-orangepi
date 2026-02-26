/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SUNXI SPI Controller Calibrate Definition
 *
 */

#ifndef _SUNXI_SPI_CALIBRATE_H_
#define _SUNXI_SPI_CALIBRATE_H_

#include "../spi-sunxi.h"

#define SUNXI_SPI_BUS_FLASH		(SUNXI_SPI_BUS_NOR | SUNXI_SPI_BUS_NAND)

/* SPI Sample Delay Register */
#define SUNXI_SPI_SAMP_DL_REG	(0x28)
	#define SUNXI_SPI_SAMP_DL_CAL_START	BIT(15)			/* Sample Delay Calibration Start */
	#define SUNXI_SPI_SAMP_DL_CAL_DONE	BIT(14)			/* Sample Delay Calibration Done */
	#define SUNXI_SPI_SAMP_DL			GENMASK(13, 8)	/* Sample Delay */
	#define SUNXI_SPI_SAMP_DL_SW_EN		BIT(7)			/* Sample Delay Software Enable */
	#define SUNXI_SPI_SAMP_DL_SW		GENMASK(5, 0)	/* Sample Delay Software */

#define SUNXI_SPI_SAMP_HIGH_FREQ	(60000000)	/* sample mode threshold frequency */
#define SUNXI_SPI_SAMP_LOW_FREQ		(24000000)	/* sample mode threshold frequency */

enum sunxi_spi_sample_type {
	SUNXI_SPI_SAMP_TYPE_OLD = 0,
	SUNXI_SPI_SAMP_TYPE_NEW = 1,
};

void sunxi_spi_delay_chain_init(struct sunxi_spi *sspi);
void sunxi_spi_set_delay_chain(struct sunxi_spi *sspi, u32 clk);
void sunxi_spi_resource_get_calibrate(struct sunxi_spi *sspi);
#ifndef CONFIG_AW_BSP_LOWMEM
void sunxi_spi_create_calibrate_sysfs(struct sunxi_spi *sspi);
void sunxi_spi_remove_calibrate_sysfs(struct sunxi_spi *sspi);
#endif

#endif /* _SUNXI_SPI_CALIBRATE_H_ */