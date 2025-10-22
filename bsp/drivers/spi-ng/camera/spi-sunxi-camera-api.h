/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SUNXI SPI Controller Camera Register Definition
 *
 */

#ifndef _SUNXI_SPI_CAMERA_API_H_
#define _SUNXI_SPI_CAMERA_API_H_

#include <linux/ctype.h>
#include <linux/spi/spi.h>

enum sunxi_spi_camera_mode {
	SUNXI_SPI_CAMERA_VSYNC,
	SUNXI_SPI_CAMERA_FRAMEHEAD,
	SUNXI_SPI_CAMERA_IDLEWAIT,
};

extern int sunxi_spi_camera_set_mode(struct spi_device *spi, enum sunxi_spi_camera_mode mode);
extern int sunxi_spi_camera_set_vsync(struct spi_device *spi, u32 len);
extern int sunxi_spi_camera_set_framehead_flag(struct spi_device *spi, u8 *buf, int len);
extern int sunxi_spi_camera_get_framehead_flag(struct spi_device *spi, u8 *buf, int len);
extern int sunxi_spi_camera_set_idlewait_us(struct spi_device *spi, u32 us);

#endif /* _SUNXI_SPI_CAMERA_API_H_ */