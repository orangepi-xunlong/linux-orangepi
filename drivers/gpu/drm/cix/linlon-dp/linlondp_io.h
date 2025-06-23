/* SPDX-License-Identifier: GPL-2.0 */
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */
#ifndef _LINLONDP_IO_H_
#define _LINLONDP_IO_H_

#include <linux/io.h>

static inline u32
linlondp_read32(u32 __iomem *base, u32 offset)
{
    return readl((base + (offset >> 2)));
}

static inline void
linlondp_write32(u32 __iomem *base, u32 offset, u32 v)
{
    writel(v, (base + (offset >> 2)));
}

static inline void
linlondp_write64(u32 __iomem *base, u32 offset, u64 v)
{
    writel(lower_32_bits(v), (base + (offset >> 2)));
    writel(upper_32_bits(v), (base + (offset >> 2) + 1));
}

static inline void
linlondp_write32_mask(u32 __iomem *base, u32 offset, u32 m, u32 v)
{
    u32 tmp = linlondp_read32(base, offset);

    tmp &= (~m);
    linlondp_write32(base, offset, v | tmp);
}

static inline void
linlondp_write_group(u32 __iomem *base, u32 offset, int num, const u32 *values)
{
    int i;

    for (i = 0; i < num; i++)
        linlondp_write32(base, offset + i * 4, values[i]);
}

#endif /*_LINLONDP_IO_H_*/
