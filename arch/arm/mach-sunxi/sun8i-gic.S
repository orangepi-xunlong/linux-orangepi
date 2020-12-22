/*
 * linux/arch/arm/mach-sunxi/sun8i-gic.S
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Author: zhuzhenhua <zhuzhenhua@allwinnertech.com>
 *
 * allwinner sun8i gic write dword operations.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/linkage.h>

ENTRY(sun8i_write_dword)
    strd    r0, r1, [r2,#0]
    dsb
    isb
    bx lr
ENDPROC(sun8i_write_dword)
