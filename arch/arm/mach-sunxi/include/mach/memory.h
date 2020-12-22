/*
 * arch/arm/mach-sunxi/include/mach/memory.h
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liugang <liugang@allwinnertech.com>
 *
 * sunxi memory header file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SUNXI_MEMORY_H
#define __SUNXI_MEMORY_H

/*
 * NOTE: CMA reserved area: (CONFIG_CMA_RESERVE_BASE ~ CONFIG_CMA_RESERVE_BASE + CONFIG_CMA_SIZE_MBYTES),
 *   which is reserved in the function of dma_contiguous_reserve, drivers/base/dma-contiguous.c.
 *   Please DO NOT conflict with it when you reserved your own areas.
 *
 *   We need to restrict CMA area in the front 256M, because VE only support these address.
 */

#if defined CONFIG_ARCH_SUN8IW1P1
#include "sun8i/memory-sun8iw1p1.h"
#elif defined CONFIG_ARCH_SUN8IW3P1
#include "sun8i/memory-sun8iw3p1.h"
#elif defined CONFIG_ARCH_SUN8IW5P1
#include "sun8i/memory-sun8iw5p1.h"
#elif defined CONFIG_ARCH_SUN8IW6P1
#include "sun8i/memory-sun8iw6p1.h"
#elif defined CONFIG_ARCH_SUN8IW7P1
#include "sun8i/memory-sun8iw7p1.h"
#elif defined CONFIG_ARCH_SUN8IW8P1
#include "sun8i/memory-sun8iw8p1.h"
#elif defined CONFIG_ARCH_SUN8IW9P1
#include "sun8i/memory-sun8iw9p1.h"
#elif defined CONFIG_ARCH_SUN9IW1P1
#include "sun9i/memory-sun9iw1p1.h"
#else
#error "please select a platform\n"
#endif

#define __sram	__section(.sram.text)
#define __sramdata __section(.sram.data)

/* For assembly routines */
#define __SRAM		.section	".sram.text","ax"
#define __SRAMDATA	.section	".sram.text","aw"

#ifndef SRAM_DDRFREQ_OFFSET
#define SRAM_DDRFREQ_OFFSET 0xF0000000
#endif

#define SUNXI_DDRFREQ_SRAM_SECTION(OFFSET, align) 		\
	. = ALIGN(align);					\
	__sram_start = .;					\
	.sram.text OFFSET : AT(__sram_start) {			\
		__sram_text_start = .;				\
		. = ALIGN(align);				\
		*(.sram.text)					\
		__sram_text_end = .;				\
	}							\
	.sram.data OFFSET + SIZEOF(.sram.text) :		\
		AT(__sram_start + SIZEOF(.sram.text)) {		\
		__sram_data_start = .;				\
		. = ALIGN(align);				\
		*(.sram.data)					\
		__sram_data_end = .;				\
	}							\
	. = __sram_start + SIZEOF(.sram.text) +			\
			SIZEOF(.sram.data);			\
	__sram_end = .;

#define SUNXI_SRAM_SECTION	\
	SUNXI_DDRFREQ_SRAM_SECTION(SRAM_DDRFREQ_OFFSET, 4)

#endif /* __SUNXI_MEMORY_H */
