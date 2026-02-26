/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

#ifndef __DT_SUNXI_SPI_H
#define __DT_SUNXI_SPI_H

#define SUNXI_SPI_BUS_MASTER	(1 << 0)
#define SUNXI_SPI_BUS_SLAVE		(1 << 1)
#define SUNXI_SPI_BUS_DBI		(1 << 2)
#define SUNXI_SPI_BUS_BIT		(1 << 3)
#define SUNXI_SPI_BUS_NOR		(1 << 4)
#define SUNXI_SPI_BUS_NAND		(1 << 5)
#define SUNXI_SPI_BUS_CAMERA	(1 << 6)

#define SUNXI_SPI_CS_AUTO	0
#define SUNXI_SPI_CS_SOFT	1

#endif /* __DT_SUNXI_SPI_H */
