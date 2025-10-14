// SPDX-License-Identifier: GPL-2.0
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */

#ifndef _LINLON_AEU_IO_H_
#define _LINLON_AEU_IO_H_

#include <linux/io.h>
#include "linlon_aeu_log.h"

static inline u32 linlon_aeu_read(void __iomem *reg, u32 offset)
{
    u32 val = readl(reg + offset);
    linlon_aeu_log(true, offset, val);
    return val;
}

static inline void linlon_aeu_write(void __iomem *reg, u32 offset, u32 val)
{
    writel(val, reg + offset);
    linlon_aeu_log(false, offset, val);
}

#endif
