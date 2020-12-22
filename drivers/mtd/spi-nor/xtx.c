// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2005, Intec Automation Inc.
 * Copyright (C) 2014, Freescale Semiconductor, Inc.
 */

#include <linux/mtd/spi-nor.h>

#include "core.h"

static const struct flash_info xtx_parts[] = {
 /* XTX (Shenzhen Xin Tian Xia Tech) */
 { "xt25f32b", INFO(0x0b4016, 0, 64 * 1024, 64, SECT_4K) },
 { "xt25f128b", INFO(0x0b4018, 0, 64 * 1024, 256, SECT_4K) },
};

const struct spi_nor_manufacturer spi_nor_xtx = {
 .name = "xtx",
 .parts = xtx_parts,
 .nparts = ARRAY_SIZE(xtx_parts),
};
