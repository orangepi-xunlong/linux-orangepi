/*
 * arch/arm/mach-sunxi/include/mach/sun9i/memory-sun9iw1p1.h
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liugang <liugang@allwinnertech.com>
 *
 * sun9i memory header file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MEMORY_SUN9I_W1P1_H
#define __MEMORY_SUN9I_W1P1_H

#define PLAT_PHYS_OFFSET         UL(0x20000000)
#ifdef CONFIG_EVB_PLATFORM
#define PLAT_MEM_SIZE            SZ_1G
#else
#define PLAT_MEM_SIZE            SZ_512M
#endif

#define SYS_CONFIG_MEMBASE       (PLAT_PHYS_OFFSET + SZ_32M + SZ_16M)           /* 0x23000000 */
#define SYS_CONFIG_MEMSIZE       (SZ_128K)                                      /* 0x00020000 */

#define SUPER_STANDBY_MEM_BASE   ((PLAT_PHYS_OFFSET + SZ_32M + SZ_16M + SZ_1M ) + (128 * SZ_1M)) //(SYS_CONFIG_MEMBASE + SYS_CONFIG_MEMSIZE)   /* 0x23010000 */
#define SUPER_STANDBY_MEM_SIZE   (SZ_2K)

#ifdef CONFIG_ANDROID_RAM_CONSOLE
#define RC_MEM_BASE              (SUPER_STANDBY_MEM_BASE + SUPER_STANDBY_MEM_SIZE)
#define RC_MEM_SIZE              (SZ_64K)
#endif

#define SRAM_DDRFREQ_OFFSET	0xf0010000

#endif /* __MEMORY_SUN9I_W1P1_H */
