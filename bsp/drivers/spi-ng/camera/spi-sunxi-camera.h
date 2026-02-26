/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SUNXI SPI Controller Camera Register Definition
 *
 */

#ifndef _SUNXI_SPI_CAMERA_H_
#define _SUNXI_SPI_CAMERA_H_

#include "../spi-sunxi.h"

#define SUNXI_SPI_FRAMEHEAD_MAX	(8)

/* SPI Slave Vsync Cycle Number Register */
#define SUNXI_SPI_SVCN_REG		(0x160)
	#define SUNXI_SPI_SVCN_SFT		GENMASK(31, 24)	/* Slave Frequency Relation */
	#define SUNXI_SPI_SVCN_SVCN		GENMASK(23, 0)	/* Slave Vsync Cycle Number */

/* SPI Slave Frame Head Number Register */
#define SUNXI_SPI_SFHN_REG		(0x164)
	#define SUNXI_SPI_SFHN			GENMASK(2, 0)	/* Slave Frame Head Number */

/* SPI Slave Frame Head Low Register */
#define SUNXI_SPI_SFHL_REG		(0x168)

/* SPI Slave Frame Head High Register */
#define SUNXI_SPI_SFHH_REG		(0x16C)

/* SPI Slave Frame Head Low Receive Register */
#define SUNXI_SPI_SFHLR_REG		(0x170)

/* SPI Slave Frame Head High Receive Register */
#define SUNXI_SPI_SFHHR_REG		(0x174)

/* SPI Slave Idle Wait Value Enable Register */
#define SUNXI_SPI_SIWVE_REG		(0x178)

/* SPI Slave Idle Wait Value Reset Register */
#define SUNXI_SPI_SIWVR_REG		(0x17C)
	#define SUNXI_SPI_SIWVR			GENMASK(15, 0)	/* Slave Idle Wait Value Reset */

#if IS_ENABLED(CONFIG_AW_SPI_NG_CAMERA)
void sunxi_spi_camera_enable_idlewait(struct sunxi_spi *sspi);
void sunxi_spi_camera_disable_idlewait(struct sunxi_spi *sspi);
void sunxi_spi_camera_enable_framehead(struct sunxi_spi *sspi);
void sunxi_spi_camera_disable_framehead(struct sunxi_spi *sspi);
void sunxi_spi_camera_enable_vsync(struct sunxi_spi *sspi);
void sunxi_spi_camera_disable_vsync(struct sunxi_spi *sspi);
int sunxi_spi_camera_get_frame_head(struct sunxi_spi *sspi, u8 *buf, int len);
#else
static inline void sunxi_spi_camera_enable_idlewait(struct sunxi_spi *sspi) {};
static inline void sunxi_spi_camera_disable_idlewait(struct sunxi_spi *sspi) {};
static inline void sunxi_spi_camera_enable_framehead(struct sunxi_spi *sspi) {};
static inline void sunxi_spi_camera_disable_framehead(struct sunxi_spi *sspi) {};
static inline void sunxi_spi_camera_enable_vsync(struct sunxi_spi *sspi) {};
static inline void sunxi_spi_camera_disable_vsync(struct sunxi_spi *sspi) {};
static inline int sunxi_spi_camera_get_frame_head(struct sunxi_spi *sspi, u8 *buf, int len) { return -EINVAL; };
#endif

#endif /* _SUNXI_SPI_CAMERA_H_ */