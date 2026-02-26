// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2022 Allwinner.
 *
 * SUNXI SPI Controller Debug Function Definition
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef _SUNXI_SPI_DEBUG_H_
#define _SUNXI_SPI_DEBUG_H_

#include <linux/kernel.h>
#include <linux/device.h>

/* Debug Dump */
#define SUNXI_SPI_DUMP_ROWSIZE	(16)
#define SUNXI_SPI_DUMP_BASE_SIZE	(0x50)
#define SUNXI_SPI_DUMP_DBI_SIZE		(0x30)
#define SUNXI_SPI_DUMP_CAMERA_SIZE	(0x20)
#define SUNXI_SPI_DUMP_BSR_SIZE		(0x4)

enum {
	SUNXI_SPI_DEBUG_DUMP_REG	= BIT(0),
	SUNXI_SPI_DEBUG_DUMP_DATA	= BIT(1),
};

__maybe_unused static void sunxi_spi_dump_reg(struct device *dev, void __iomem *vaddr, u32 paddr, u32 len)
{
	int i;
	char str[32] = { 0 };

	for (i = 0; i < len; i += SUNXI_SPI_DUMP_ROWSIZE) {
		scnprintf(str, sizeof(str), "0x%08x: ", paddr + i);
		print_hex_dump(KERN_INFO, str, DUMP_PREFIX_NONE, SUNXI_SPI_DUMP_ROWSIZE, 4, vaddr + i, SUNXI_SPI_DUMP_ROWSIZE, false);
	}
}

__maybe_unused static void sunxi_spi_dump_data(struct device *dev, const u8 *buf, u32 len, const char *prefix_str)
{
	if (buf && len > 0) {
		if (len > 256)
			len = 256;
		print_hex_dump(KERN_INFO, prefix_str, DUMP_PREFIX_OFFSET, SUNXI_SPI_DUMP_ROWSIZE, 1, buf, len, true);
	}
}

#endif /* _SUNXI_SPI_DEBUG_H_ */
