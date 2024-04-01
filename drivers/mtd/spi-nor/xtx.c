// SPDX-License-Identifier: GPL-2.0

#include <linux/mtd/spi-nor.h>

#include "core.h"

static const struct flash_info xtx_parts[] = {
 /* XTX (Shenzhen Xin Tian Xia Tech) */
 { "xt25f128b", INFO(0x0b4018, 0, 64 * 1024, 256) },
};

const struct spi_nor_manufacturer spi_nor_xtx = {
 .name = "xtx",
 .parts = xtx_parts,
 .nparts = ARRAY_SIZE(xtx_parts),
};
