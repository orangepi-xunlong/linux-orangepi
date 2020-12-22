/*
 * arch/arm/mach-sunxi/include/mach/sram.h
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: Sugar <shuge@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SRAM_H__
#define __SRAM_H__

#include <linux/types.h>

extern char __sram_start, __sram_end;
extern char __sram_text_start, __sram_text_end;
extern char __sram_data_start, __sram_data_end;

typedef enum {
	SRAM_A1,
	SRAM_A2,
	SRAM_B,
	SRAM_MAX,
} sram_t;

struct own_info {
	unsigned long addr;
	size_t size;
	struct list_head node;
	const char *name;
};

struct sunxi_sram {
	phys_addr_t phys_addr;		/* physical starting address of memory chunk */
	unsigned long virt_addr;	/* virtual starting address of memory chunk */
	size_t size;			/* the size of sarm */
	spinlock_t lock;
	struct list_head own_list;
	struct gen_pool *pool;
};

void *sram_alloc(size_t len, dma_addr_t *dma, sram_t type);
void sram_free(void *addr);
int sram_reserve(const char *name, unsigned long phy_addr, size_t size);


#endif /* __SRAM_H__ */
