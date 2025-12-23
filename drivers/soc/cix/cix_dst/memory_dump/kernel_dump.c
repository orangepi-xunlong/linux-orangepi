// SPDX-License-Identifier: GPL-2.0-only
/*
 * kernel_dump.c
 *
 * balong memory/register proc-fs dump implementation
 * Copyright 2024 Cix Technology Group Co., Ltd.
 * Copyright 2025 Cix Technology Group Co., Ltd.
 *
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

#include <linux/reboot.h>
#include <linux/kexec.h>
#include <linux/crash_core.h>
#include <linux/crash_dump.h>
#include <linux/memblock.h>
#include <linux/cacheflush.h>
#include <linux/soc/cix/cix_hibernate.h>
#include <linux/soc/cix/util.h>
#include <linux/sched/debug.h>
#include <linux/panic_notifier.h>
#include <mntn_dump_interface.h>
#include "kernel_dump.h"
#include "../dst_print.h"

#define UEFI_RESEVED_MEM_DESC_BASE CONFIG_KERNELDUMP_RESERVED_DESC
#define UEFI_RESEVED_MEM_DESC_SIZE (0x1000)
#define UEFI_RESEVED_MEM_HEADER 0x55aa55aa

/* the max size of mod->core_size is 4M */
#define MODULE_MAX_CORE_SIZE (4 * 1024 * 1024)

extern int pcpu_base_size;

static struct kernel_dump_cb *g_kdump_cb;
static u64 g_kdump_reserve_size = { 0 };
static Elf64_Phdr reserved_mem_phdr[KERNELDUMP_CB_MAX_SEC];
static int cur_reserved_mem;

struct uefi_mem_desc {
	u64 reserved_mem_start;
	u64 reserved_mem_end;
};

struct uefi_reserved_desc {
	u32 reserved_header;
	u32 reserved_count;
	struct uefi_mem_desc reserved_desc[0];
};

#ifdef CONFIG_HIBERNATION
static int kernel_dump_suspend(u64 paddr, u64 size)
{
	u64 addr = (u64)g_kdump_cb;

	if (!addr)
		return 0;

	dcache_clean_poc(addr, addr + size);
	return 0;
}

static void kernel_dump_resume(u64 paddr, u64 size)
{
	u64 addr = (u64)g_kdump_cb;

	if (!addr)
		return;

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

static int kd_prepare_elf_headers(void **addr, unsigned long *sz)
{
	struct crash_mem *cmem;
	unsigned int nr_ranges;
	int ret;
	u64 i;
	phys_addr_t start, end;

	nr_ranges = 2;
	for_each_mem_range(i, &start, &end)
		nr_ranges++;

	cmem = kmalloc(struct_size(cmem, ranges, nr_ranges), GFP_KERNEL);
	if (!cmem)
		return -ENOMEM;

	cmem->max_nr_ranges = nr_ranges;
	cmem->nr_ranges = 0;
	for_each_mem_range(i, &start, &end) {
		cmem->ranges[cmem->nr_ranges].start = start;
		cmem->ranges[cmem->nr_ranges].end = end - 1;
		cmem->nr_ranges++;
	}

	ret = crash_prepare_elf64_headers(cmem, true, addr, sz);
	kfree(cmem);
	return ret;
}

static void kd_flush_cache(void)
{
	Elf64_Ehdr *ehdr;
	Elf64_Phdr *phdr;
	int cpu;

	if (!g_kdump_cb)
		return;

	dcache_clean_poc((u64)g_kdump_cb,
			 (u64)g_kdump_cb + g_kdump_reserve_size);
	ehdr = (Elf64_Ehdr *)g_kdump_cb->buf;
	phdr = (Elf64_Phdr *)(ehdr + 1);

	/* prepare note vaddr for each possible CPU */
	for_each_possible_cpu(cpu) {
		if (phdr->p_vaddr)
			continue;
		phdr->p_vaddr = (u64)per_cpu_ptr(crash_notes, cpu);
		phdr++;
	}
	/* prepare note vaddr for vmcoreinfo */
	if (!phdr->p_vaddr)
		phdr->p_vaddr = (u64)vmcoreinfo_note;

	phdr = (Elf64_Phdr *)(ehdr + 1);
	/* flush cache for elf64_phdr */
	for (int i = 0; i < ehdr->e_phnum; i++) {
		if (phdr[i].p_type != PT_NOTE)
			continue;
		DST_PN("flush cache for pt[%d] type %d, 0x%llx~0x%llx\n", i,
		       phdr[i].p_type, phdr[i].p_vaddr,
		       phdr[i].p_vaddr + phdr[i].p_memsz);
		dcache_clean_poc(phdr[i].p_vaddr,
				 phdr[i].p_vaddr + phdr[i].p_memsz);
	}
}

void kd_save_state_shutdown(void)
{
	struct pt_regs regs;

	local_irq_disable();
	crash_setup_regs(&regs, NULL);
	crash_save_vmcoreinfo();

	/* for crashing cpu */
	crash_save_cpu(&regs, smp_processor_id());
	kd_flush_cache();
	__flush_dcache_all();

	/* HIMNTN_PANIC_INTO_LOOP will disbale ap reset */
	if (check_himntn(HIMNTN_PANIC_INTO_LOOP) == 1) {
		do {
		} while (1);
	}
	machine_restart(NULL);
}
EXPORT_SYMBOL(kd_save_state_shutdown);

static int kd_add_reserved_mem_cached(void *vaddr, phys_addr_t paddr, u64 size)
{
	Elf64_Phdr *phdr;

	if (cur_reserved_mem >= KERNELDUMP_CB_MAX_SEC)
		return -1;
	if (!size)
		return -1;
	if (size && size % PAGE_SIZE)
		return -1;

	phdr = reserved_mem_phdr;
	phdr[cur_reserved_mem].p_type = PT_LOAD;
	phdr[cur_reserved_mem].p_flags = PF_R | PF_W | PF_X;
	phdr[cur_reserved_mem].p_offset = paddr;

	phdr[cur_reserved_mem].p_paddr = paddr;
	phdr[cur_reserved_mem].p_vaddr = (u64)vaddr;
	phdr[cur_reserved_mem].p_filesz = phdr[cur_reserved_mem].p_memsz = size;
	phdr[cur_reserved_mem].p_align = 0;
	cur_reserved_mem++;
	return 0;
}

int kd_add_reserved_mem(void *vaddr, phys_addr_t paddr, u64 size)
{
	Elf64_Ehdr *ehdr;
	Elf64_Phdr *phdr;

	if (!g_kdump_cb)
		return kd_add_reserved_mem_cached(vaddr, paddr, size);

	if (!size)
		return -1;

	if (size && size % PAGE_SIZE)
		return -1;

	ehdr = (Elf64_Ehdr *)g_kdump_cb->buf;
	if (ehdr->e_phnum >= KERNELDUMP_CB_MAX_SEC) {
		DST_ERR("too many sections\n");
		return -1;
	}

	phdr = (Elf64_Phdr *)(ehdr + 1);
	phdr[ehdr->e_phnum].p_type = PT_LOAD;
	phdr[ehdr->e_phnum].p_flags = PF_R | PF_W | PF_X;
	phdr[ehdr->e_phnum].p_offset = paddr;

	phdr[ehdr->e_phnum].p_paddr = paddr;
	phdr[ehdr->e_phnum].p_vaddr = (u64)vaddr;
	phdr[ehdr->e_phnum].p_filesz = phdr[ehdr->e_phnum].p_memsz = size;
	phdr[ehdr->e_phnum].p_align = 0;
	ehdr->e_phnum++;
	g_kdump_cb->checksum =
		checksum32((u32 *)g_kdump_cb->buf,
			   sizeof(*ehdr) + (ehdr->e_phnum * sizeof(*phdr)));
	kd_flush_cache();

	return 0;
}
EXPORT_SYMBOL(kd_add_reserved_mem);

static int kd_add_cached_reserved_mem(void)
{
	for (int i = 0; i < cur_reserved_mem; i++)
		if (kd_add_reserved_mem((void *)reserved_mem_phdr[i].p_vaddr,
					reserved_mem_phdr[i].p_paddr,
					reserved_mem_phdr[i].p_memsz))
			return -1;
	cur_reserved_mem = 0;
	return 0;
}

int kernel_dump_init(void)
{
	struct kernel_dump_cb *cb = NULL;
	unsigned long sz;
	void *buf;
	int ret;

	g_kdump_reserve_size = (KERNELDUMP_CB_MAX_SEC * sizeof(Elf64_Phdr)) +
			       sizeof(Elf64_Ehdr) + sizeof(*cb);
	if (register_mntn_dump(MNTN_DUMP_KERNEL_DUMP, g_kdump_reserve_size,
			       (void **)&cb)) {
		DST_ERR("fail to get reserve memory\n");
		return -1;
	}

	/*clear kernel dump info*/
	cb->checksum = 0;
	cb->magic = 0;
	ret = kd_prepare_elf_headers(&buf, &sz);
	if (ret) {
		DST_ERR("fail to prepare elf headers\n");
		return -1;
	}
	if (sz + sizeof(*cb) > g_kdump_reserve_size) {
		DST_ERR("reserve memory is not enough %lu\n", sz);
		return -1;
	}
	cb->magic = KERNELDUMP_CB_MAGIC;
	memcpy(cb->buf, buf, sz);
	cb->checksum = checksum32((u32 *)cb->buf, sz);
	vfree(buf);
	g_kdump_cb = cb;
	kd_add_cached_reserved_mem();
	kd_flush_cache();

#ifdef CONFIG_HIBERNATION
	kernel_dump_reserve_ops.paddr =
		(vmalloc_to_pfn(g_kdump_cb) << PAGE_SHIFT) +
		((u64)g_kdump_cb & ((1 << PAGE_SHIFT) - 1));
	kernel_dump_reserve_ops.size = sizeof(*g_kdump_cb);
	register_reserve_mem_ops(&kernel_dump_reserve_ops);
#endif

	/*PANIC_PRINT_ALL_CPU_BT*/
	panic_print |= 0x00000040;
	/*save other cpu crash_notes*/
	crash_kexec_post_notifiers = true;
	return 0;
}
subsys_initcall_sync(kernel_dump_init);

static int kernel_dump_flag;
void __init kernel_dump_mem_reserve(void)
{
	u32 i;
	struct uefi_reserved_desc *pdesc;

	if (kernel_dump_flag)
		return;

	kernel_dump_flag = 1;

	pdesc = early_memremap(UEFI_RESEVED_MEM_DESC_BASE,
			       UEFI_RESEVED_MEM_DESC_SIZE);
	if (!pdesc) {
		DST_PN("map kernel dump description failed\n");
		return;
	}

	if (pdesc->reserved_header != UEFI_RESEVED_MEM_HEADER) {
		early_memunmap(pdesc, UEFI_RESEVED_MEM_DESC_SIZE);
		DST_PN("uefi reserved description doesn't match\n");
		return;
	}

	DST_PN("Kernel Dump Enabled. Will reserved:\n");
	for (i = 0; i < pdesc->reserved_count; i++) {
		if (!memblock_mark_nomap(
			    pdesc->reserved_desc[i].reserved_mem_start,
			    pdesc->reserved_desc[i].reserved_mem_end -
				    pdesc->reserved_desc[i].reserved_mem_start +
				    1)) {
			DST_PN("\tmem [0x%016llx-0x%016llx]\n",
			       pdesc->reserved_desc[i].reserved_mem_start,
			       pdesc->reserved_desc[i].reserved_mem_end);
		}
	}

	early_memunmap(pdesc, UEFI_RESEVED_MEM_DESC_SIZE);
}
