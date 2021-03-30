/*
 * Hibernation support specific for ARM
 *
 * Copyright (C) 2010 Nokia Corporation
 * Copyright (C) 2010 Texas Instruments, Inc.
 * Copyright (C) 2006 Rafael J. Wysocki <rjw <at> sisk.pl>
 *
 * Contact: Hiroshi DOYU <Hiroshi.DOYU <at> nokia.com>
 *
 * License terms: GNU General Public License (GPL) version 2
 */
#include <linux/module.h>
#include <linux/suspend.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/major.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/power/aw_pm.h>
#include <linux/module.h>
#include <asm/io.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#include <asm/pgtable.h>
#include "pm_o.h"

/*
 * "Linux" PTE definitions.
 *
 * We keep two sets of PTEs - the hardware and the linux version.
 * This allows greater flexibility in the way we map the Linux bits
 * onto the hardware tables, and allows us to have YOUNG and DIRTY
 * bits.
 *
 * The PTE table pointer refers to the hardware entries; the "Linux"
 * entries are stored 1024 bytes below.
 */

#define L_PTE_WRITE		(1 << 7)
#define L_PTE_EXEC		(1 << 9)

struct saved_mmu_level_one {
	u32 vaddr;
	u32 entry_val;
};

static struct saved_mmu_level_one backup_tbl[1];

/*
 * Create the page directory entries for 0x0000, 0000 <-> 0x0000, 0000
 */
void create_mapping(void)
{
	u32 ttbcr = 0;
	u32 ttbcr_n = 0;
	u32 ttbr0 = 0;
	volatile void *tlb_item_addr = 0;
	/*make sure ttbcr.n = 0, then we can use ttbr0*/
	asm volatile ("mrc p15, 0, %0, c2, c0, 2" : "=r"(ttbcr));
	ttbcr_n = ttbcr & 0x7;
	if (0 != ttbcr_n) {
		panic("err: notice ttbcr.n not == 0, so u can not use ttbr0.\n");
	}

	asm volatile ("mrc p15, 0, %0, c2, c0, 0" : "=r"(ttbr0));
	ttbr0 &= (0xffffffff << (14 - ttbcr_n));

	/*
	 * what does 0xc4a mean for tlb?
	 * when ttbcr.n == 0, the cortex-A7 will use ttbr0 for translate mmu.
	 * ttbr0: hold the base address of translation table0;
	 *        1M bytes need 1 item which need 4bytes.
	 *        so for SRAM_FUNC_START=0xf0000000,
	 *		need use pa_to_va(ttbr0) + 0xf00 * 4 for first-level description.
	 *        then content is: 0xc4a | 0x000<<20, which mean:
	 *           1. the section base addr pa == 0x0000,0000
	 *           2. tex[2:0],c,b == 000, 1, 0
	 *              prrr= 0xFF0A81A8
	 *              nmrr = 0x40E040E0
	 *        according to trm:
	 *        for 0xc4a
	 *           prrr[5:4] -> 0b10 -> normal memory
	 *           nmrr[5:4] -> 0b10 : write through, non-write allocate
	 *           nmrr[21:20] -> 0b10; write through, non-write allocate
	 *           prrr[26] -> 0 nonsharebale
	 */

	tlb_item_addr = phys_to_virt((phys_addr_t)(ttbr0 + (SRAM_FUNC_START>>18)));
	save_mapping((unsigned long)tlb_item_addr);
	writel(0xc4a | 0x000<<20, tlb_item_addr);


	/**
	 * clean dcache, , invalidat icache & invalidate tlb,
	 *
	 * function:
	 *	to make sure the correct PA will be access.
	 *
	 * cache:
	 *	clean cache unit: is cache line size;
	 *	whether the end addr will be flush? exclusive, not including.
	 *
	 * tlb:
	 *	invalidate tlb unit: PAGE_SIZE;
	 *	Not including end addr.
	 *
	 * Note:
	 *	actually, because the PA will be used at resume period time,
	 *	mean, not use immediately,
	 *	and, the cache will be clean at the end.
	 *	so, the clean & invalidate is not necessary.
	 *	do this here, just in case testing. like: jump to resume code for testing.
	**/

	/*Note: 0xc000, 0000, is device area; not need to flush cache. */
	/*ref: ./arch/arm/kernel/head.S */
	dmac_flush_range((void *)(tlb_item_addr), (void *)(tlb_item_addr + (sizeof(u32))));
	local_flush_tlb_kernel_range((unsigned long)(tlb_item_addr), (unsigned long)(tlb_item_addr + (sizeof(u32))));
	mem_flush_tlb();
	return;
}

/**save the va: 0x0000, 0000 mapping.
*@vaddr: the va of mmu mapping to save;
*/
void save_mapping(unsigned long vaddr)
{

	backup_tbl[0].vaddr = vaddr;
	backup_tbl[0].entry_val = *((volatile __u32 *)(vaddr));

	return;
}

/**restore the va: 0x0000, 0000 mapping.
*@vaddr: the va of mmu mapping to restore.
*
*/
void restore_mapping()
{
	unsigned long vaddr = backup_tbl[0].vaddr;

	*((volatile __u32 *)(vaddr)) = backup_tbl[0].entry_val;
	/*clean dcache, invalidat icache */
	dmac_flush_range((void *)(vaddr), (void *)(vaddr + (sizeof(u32))));
	/*      flust tlb after change mmu mapping. */
	local_flush_tlb_kernel_range((unsigned long)(vaddr), (unsigned long)(vaddr + (sizeof(u32))));

	return;
}
