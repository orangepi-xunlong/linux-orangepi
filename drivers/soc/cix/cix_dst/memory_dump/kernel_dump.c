/*
 * kernel_dump.c
 *
 * balong memory/register proc-fs dump implementation
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <linux/memblock.h>
#include "kernel_dump.h"
#include <linux/soc/cix/mntn_dump.h>
#include <linux/soc/cix/util.h>
#include "../dst_print.h"
#include <mntn_dump_interface.h>
#include <linux/io.h>
#include <asm/cacheflush.h>
#include <linux/soc/cix/cix_hibernate.h>

#define UEFI_RESEVED_MEM_DESC_BASE CONFIG_KERNELDUMP_RESERVED_DESC
#define UEFI_RESEVED_MEM_DESC_SIZE (0x1000)
#define UEFI_RESEVED_MEM_HEADER 0x55aa55aa

/* the max size of mod->core_size is 4M */
#define MODULE_MAX_CORE_SIZE (4 * 1024 * 1024)

extern int pcpu_base_size;

struct kernel_dump_cb *g_kdump_cb;

struct uefi_mem_desc {
	u64 reserved_mem_start;
	u64 reserved_mem_end;
};

struct uefi_reserved_desc {
	u32 reserved_header;
	u32 reserved_count;
	struct uefi_mem_desc reserved_desc[0];
};

static struct table_extra {
	u64 extra_mem_phy_base;
	u64 extra_mem_virt_base;
	u64 extra_mem_size;
} g_tbl_extra_mem[MAX_EXTRA_MEM] = { { 0, 0, 0 } };

static unsigned int extra_index;
static DEFINE_RAW_SPINLOCK(g_kdump_lock);

void plat_set_cpu_regs(int coreid, struct pt_regs* src)
{
	struct pt_regs* dst;

	if (!g_kdump_cb || coreid >= MNTN_MAX_CPU_CORES) {
		DST_PRINT_ERR("%s():%d:input arg [%d], 0x%px\n", __func__, __LINE__, coreid, src);
		return;
	}

	dst = (struct pt_regs*)(g_kdump_cb + 1);
	dst += coreid;
	memcpy(dst, src, sizeof(struct pt_regs));
}
EXPORT_SYMBOL_GPL(plat_set_cpu_regs);

static int add_mem2table(u64 va, u64 pa, u64 size, bool need_crc)
{
	unsigned int i;
	bool crc_check = false;

	if ((pa == 0) || (va == 0) || (size == 0) || (extra_index >= MAX_EXTRA_MEM)) {
		return -1;
	}

	raw_spin_lock(&g_kdump_lock);
	/* Kernel dump is not inited */
	if (!g_kdump_cb) {
			g_tbl_extra_mem[extra_index].extra_mem_phy_base = pa;
			g_tbl_extra_mem[extra_index].extra_mem_virt_base = va;
			g_tbl_extra_mem[extra_index].extra_mem_size = size;
			extra_index++;
	} else {
		i = extra_index;
		if (i < MAX_EXTRA_MEM) {
			g_kdump_cb->extra_mem_phy_base[i] = pa;
			g_kdump_cb->extra_mem_virt_base[i] = va;
			g_kdump_cb->extra_mem_size[i] = size;
			extra_index += 1;

			crc_check = true;
		} else {
			pr_err("%s: extra memory(nums:%d) is out of range. \r\n", __func__, extra_index);
			goto err;
		}
	}

	if ((true == need_crc) && (true == crc_check)) {
		g_kdump_cb->crc = 0;
		g_kdump_cb->crc = checksum32((u32 *)g_kdump_cb, sizeof(struct kernel_dump_cb));
	}

	raw_spin_unlock(&g_kdump_lock);
	return 0;
err:
	raw_spin_unlock(&g_kdump_lock);
	return -1;
}

int add_extra_table(u64 pa, u64 size)
{
	return add_mem2table((uintptr_t)phys_to_virt(pa), pa, size, true);
}

static void kernel_dump_printcb(struct memblock_type *print_mb_cb,
				struct kernel_dump_cb *cb)
{
	(void)print_mb_cb;
	(void)cb;
}

#ifdef CONFIG_HIBERNATION
static int kernel_dump_suspend(u64 paddr, u64 size)
{
	u64 addr = (u64)g_kdump_cb;

	if (!addr) {
		return 0;
	}
	dcache_clean_poc(addr, addr + size);
	return 0;
}

static void kernel_dump_resume(u64 paddr, u64 size)
{
	u64 addr = (u64)g_kdump_cb;

	if (!addr) {
		return;
	}
	dcache_inval_poc(addr, addr + size);
}

/*
 * To preserve the kernel dump , the relevant memory segments
 * should be mapped again around the hibernation.
 */
static struct hibernate_rmem_ops kernel_dump_reserve_ops = {
	.name = "kernel_dump",
	.resume = kernel_dump_resume,
	.suspend = kernel_dump_suspend,
};
#endif

int kernel_dump_init(void)
{
	unsigned int i, j;
	phys_addr_t mem_ret;
	struct kernel_dump_cb *cb = NULL;
	struct memblock_type *print_mb_cb = NULL;

	if (register_mntn_dump(MNTN_DUMP_KERNEL_DUMP, sizeof(struct kernel_dump_cb),
		(void **)&cb)) {
		DST_PRINT_ERR("%s: fail to get reserve memory\r\n", __func__);
		goto err;
	}

	memset((void *)cb, 0, sizeof(struct kernel_dump_cb));

	cb->magic = KERNELDUMP_CB_MAGIC;
	cb->size = sizeof(struct kernel_dump_cb);
	cb->page_shift = PAGE_SHIFT;
	cb->struct_page_size = sizeof(struct page);
#ifdef CONFIG_RANDOMIZE_BASE
	cb->phys_offset = memstart_addr;
	cb->kernel_offset = kimage_vaddr;
#else
	cb->phys_offset = PHYS_OFFSET;
	cb->kernel_offset = KIMAGE_VADDR;/*lint !e648*/
#endif
	cb->page_offset = PAGE_OFFSET;/*lint !e648*/
	cb->extra_mem_phy_base[0] = virt_to_phys(_text);
	cb->extra_mem_virt_base[0] = (u64)(uintptr_t)_text;
	cb->extra_mem_size[0] = ALIGN((u64)(uintptr_t)_end - (u64)(uintptr_t)_text, PAGE_SIZE);
	cb->extra_mem_phy_base[1] = virt_to_phys(pcpu_base_addr); /* per cpu info*/
	cb->extra_mem_virt_base[1] = (u64)(uintptr_t)pcpu_base_addr; /* per cpu info*/
	cb->extra_mem_size[1] = (u64)ALIGN(pcpu_base_size, PAGE_SIZE)*CONFIG_NR_CPUS;
	for (i = 2, j = 0; i < MAX_EXTRA_MEM && j < extra_index; i++, j++) {
		cb->extra_mem_phy_base[i] = g_tbl_extra_mem[j].extra_mem_phy_base;
		cb->extra_mem_virt_base[i] = g_tbl_extra_mem[j].extra_mem_virt_base;
		cb->extra_mem_size[i] = g_tbl_extra_mem[j].extra_mem_size;
	}
	extra_index = i;
#ifdef CONFIG_SPARSEMEM_VMEMMAP
	cb->page = vmemmap;
	cb->pfn_offset = 0;
	cb->pmd_size = PMD_SIZE;
	cb->section_size = 1UL << SECTION_SIZE_BITS;
#else
#error "Configurations other than CONFIG_PLATMEM and CONFIG_SPARSEMEM_VMEMMAP are not supported"
#endif
#ifdef CONFIG_64BIT
	/*Subtract the base address that TTBR1 maps*/
	cb->kern_map_offset = (UL(0xffffffffffffffff) << VA_BITS);/*lint !e648*/
#else
	cb->kern_map_offset = 0;
#endif
	cb->flag = 0xABCDABCDABCDABCD;
	cb->ttrb1_el1 = read_sysreg(ttbr1_el1);
	cb->tcr_el1 = read_sysreg(tcr_el1);
	cb->maid_el1 = read_sysreg(mair_el1);
	cb->amair_el1 = read_sysreg(amair_el1);
	cb->sctlr_el1 = read_sysreg(sctlr_el1);
	cb->mb_cb = (struct memblock_type *)(uintptr_t)virt_to_phys(&memblock.memory);
	print_mb_cb = &memblock.memory;
	cb->mbr_size = sizeof(struct memblock_region);
	mem_ret = memblock_start_of_DRAM();
	(void) mem_ret;
	cb->linear_kaslr_offset = kaslr_offset();
	cb->resize_flag = SKP_DUMP_RESIZE_FAIL; /*init fail status*/
	cb->skp_flag    = SKP_DUMP_SKP_FAIL; /*init fail status*/

	kernel_dump_printcb(print_mb_cb, cb);
	g_kdump_cb = cb;

	cb->crc = 0;
	cb->crc = checksum32((u32 *)cb, sizeof(struct kernel_dump_cb));

#ifdef CONFIG_HIBERNATION
	kernel_dump_reserve_ops.paddr =
		(vmalloc_to_pfn(g_kdump_cb) << PAGE_SHIFT) +
		((u64)g_kdump_cb & ((1 << PAGE_SHIFT) - 1));
	kernel_dump_reserve_ops.size = sizeof(*g_kdump_cb);
	register_reserve_mem_ops(&kernel_dump_reserve_ops);
#endif

	return 0;
err:
	return -1;
}
early_initcall(kernel_dump_init);

static int kernel_dump_flag = 0;
void __init kernel_dump_mem_reserve(void)
{
	u32 i;
	struct uefi_reserved_desc *pdesc;

	if (kernel_dump_flag) {
		return;
	}
	kernel_dump_flag = 1;

	pdesc = early_memremap(UEFI_RESEVED_MEM_DESC_BASE, UEFI_RESEVED_MEM_DESC_SIZE);
	if (!pdesc) {
		pr_info("%s map kernel dump description failed\n", __func__);
		return;
	}

	if (pdesc->reserved_header != UEFI_RESEVED_MEM_HEADER) {
		early_memunmap(pdesc, UEFI_RESEVED_MEM_DESC_SIZE);
		pr_info("%s uefi reserved description doesn't match\n", __func__);
		return;
	}

	pr_info("Kernel Dump Enabled. Will reserved:\n");
	for (i = 0; i < pdesc->reserved_count; i++) {
		if (!memblock_mark_nomap(pdesc->reserved_desc[i].reserved_mem_start,
			pdesc->reserved_desc[i].reserved_mem_end
				- pdesc->reserved_desc[i].reserved_mem_start + 1)) {
					pr_info("\tmem [0x%016llx-0x%016llx]\n",
						pdesc->reserved_desc[i].reserved_mem_start,
						pdesc->reserved_desc[i].reserved_mem_end);
				}
	}

	early_memunmap(pdesc, UEFI_RESEVED_MEM_DESC_SIZE);
}
