// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023, ky Corporation.
 */

#include <linux/mtd/spi-nor.h>

#include "core.h"

static const struct flash_info fmsh_nor_parts[] = {
	{ "FM25Q64AI3", INFO(0xa14017, 0, 4 * 1024,  2048)
		NO_SFDP_FLAGS(SECT_4K | SPI_NOR_QUAD_READ |
			      SPI_NOR_DUAL_READ) },
};

const struct spi_nor_manufacturer spi_nor_fmsh = {
	.name = "fmsh",
	.parts = fmsh_nor_parts,
	.nparts = ARRAY_SIZE(fmsh_nor_parts),
};
