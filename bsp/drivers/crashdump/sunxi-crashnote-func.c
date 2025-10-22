// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright(c) 2019-2020 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Allwinner sunxi crash dump debug
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/libfdt.h>
#include <linux/memblock.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/ioport.h>
#include <linux/cpumask.h>
#include <linux/minmax.h>

#include <linux/crash_core.h>
#include <linux/utsname.h>
#include <linux/elfcore.h>
#include <linux/elf.h>
#include <linux/kexec.h>
#include <linux/version.h>
#include <asm/system_misc.h>
#include <asm/ptrace.h>
#include <linux/dma-mapping.h>
#include "sunxi-crashnote.h"

spinlock_t cpu_dump_lock;

#if IS_ENABLED(CONFIG_ARM)
static const char *processor_modes[] __maybe_unused = {
  "USER_26", "FIQ_26", "IRQ_26", "SVC_26", "UK4_26", "UK5_26", "UK6_26", "UK7_26",
  "UK8_26", "UK9_26", "UK10_26", "UK11_26", "UK12_26", "UK13_26", "UK14_26", "UK15_26",
  "USER_32", "FIQ_32", "IRQ_32", "SVC_32", "UK4_32", "UK5_32", "MON_32", "ABT_32",
  "UK8_32", "UK9_32", "HYP_32", "UND_32", "UK12_32", "UK13_32", "UK14_32", "SYS_32"
};

static const char *isa_modes[] __maybe_unused = {
  "ARM", "Thumb", "Jazelle", "ThumbEE"
};
#endif

#if IS_ENABLED(CONFIG_KEXEC) && IS_ENABLED(CONFIG_CRASH_DUMP) && IS_BUILTIN(CONFIG_AW_CRASHDUMP)
static char *sunxi_crashdump_info;
#else
#include <asm/checksum.h>
#include "./borrowed_code.c"

#define SUNXI_CRASH_DATA_BUFFER_SIZE (2 * 4096)
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
/*
 * older version enclose struct crash_mem with CONFIG_KEXEC
 * delare it ourself
 */
struct crash_mem {
	unsigned int max_nr_ranges;
	unsigned int nr_ranges;
	struct range ranges[];
};
#endif
struct crash_mem *crash_mem_info;
struct crash_header_t {
	char magic[32];
	uint32_t len;
	uint32_t checksum;
	uint8_t data[];
} *crash_header;
static void prepare_crash_mem_info(void)
{
	unsigned int nr_ranges;
	struct crash_mem *cmem;
	/* TDB: we should get nr_ranges somethere else instead of hardcode */
	nr_ranges = 1;
	if (crash_mem_info)
		kfree(crash_mem_info);
	crash_mem_info = kmalloc(struct_size(crash_mem_info, ranges, nr_ranges),
				 GFP_KERNEL | __GFP_ZERO);
	if (!crash_mem_info)
		return;
	cmem = crash_mem_info;
	cmem->max_nr_ranges = nr_ranges;
	cmem->nr_ranges = 1;
	/* TDB: we should get start somethere else instead of hardcode */
	cmem->ranges[0].start = 0x40000000;
	cmem->ranges[0].end = memblock_end_of_DRAM();

	crash_header =
		kmalloc(SUNXI_CRASH_DATA_BUFFER_SIZE, GFP_KERNEL | __GFP_ZERO);
	if (!crash_header)
		goto err_cmem;

	memcpy(crash_header->magic, "sunxi_crash_header",
	       sizeof("sunxi_crash_header"));

	return;
err_cmem:
	kfree(crash_mem_info);
	crash_mem_info = NULL;
	return;
}
void crash_buffer_init(void)
{
	prepare_crash_mem_info();
	crash_notes_memory_init();
	crash_save_vmcoreinfo_init();
}
static uint32_t __checksum(uint32_t *buf, size_t len_in_word)
{
	uint32_t checksum = 0;
	while (len_in_word--)
		checksum += *buf++;
	return checksum;
}

/*
 * prepare kdump elf header here
 *
 * accroding to vmcore.c, final kdump file should be like this:
 *  ----
 *  1. elf header <- ehdr
 *  -----
 *  2. section header for crashnote(s) <- phdr
 *  3. section header for memory regions <- phdr
 *  -----
 *  4. section data of crash notes
 *  5. section data of memory
 *
 *  crash dump read <5> from ddr, so we need to prepare <1> to <4>
 *  here
 *  note: kexec prepare dedicated notes for eache cpu an vmcoreinfo
 *  but vmcore will merge them into one
 */

static void fill_merged_crash_notes(Elf_Phdr *phdr, Elf_Word *note_in_head,
				    size_t note_date_size)
{
	unsigned int cpu;
	unsigned int size_to_write;
	phdr->p_filesz = 0;
	phdr->p_type = PT_NOTE;

	for_each_online_cpu(cpu) {
		struct elf_note *note =
			(struct elf_note *)per_cpu_ptr(sunxi_crash_notes, cpu);
		size_to_write = sizeof(*note) +
				ALIGN(note->n_namesz, sizeof(Elf_Word)) +
				ALIGN(note->n_descsz, sizeof(Elf_Word));
		if (size_to_write > note_date_size)
			goto buf_too_small;
		note_in_head = append_elf_note(
			note_in_head, (void *)(note + 1), note->n_type,
			(void *)(note + 1) +
				ALIGN(note->n_namesz, sizeof(Elf_Word)),
			note->n_descsz);
		phdr->p_filesz += size_to_write;
		note_date_size -= size_to_write;
	}

	size_to_write = sizeof(struct elf_note) +
			ALIGN(strlen(VMCOREINFO_NOTE_NAME), sizeof(Elf_Word)) +
			ALIGN(vmcoreinfo_size, sizeof(Elf_Word));
	if (size_to_write > note_date_size)
		goto buf_too_small;
	note_in_head = append_elf_note(note_in_head, VMCOREINFO_NOTE_NAME, 0,
				       vmcoreinfo_data, vmcoreinfo_size);
	phdr->p_filesz += size_to_write;
	note_date_size -= size_to_write;

	size_to_write = sizeof(struct elf_note);
	if (size_to_write > note_date_size)
		goto buf_too_small;
	final_note(note_in_head);
	phdr->p_filesz += size_to_write;
	note_date_size -= size_to_write;
	return;

buf_too_small:
	return;
}
int crash_prepare_elf64_headers(struct crash_mem *mem, int need_kernel_map,
				void **addr, unsigned long *sz)
{
	Elf64_Ehdr *ehdr;
	struct elf_phdr *phdr;
	unsigned char *buf;
	unsigned int i;
	unsigned long mstart, mend;
	unsigned elf_len;
	struct elf_phdr *note_phdr;

	buf = crash_header->data;

	ehdr = (Elf64_Ehdr *)buf;
	phdr = (struct elf_phdr *)(ehdr + 1);
	memcpy(ehdr->e_ident, ELFMAG, SELFMAG);
	ehdr->e_ident[EI_CLASS] = ELFCLASS64;
	ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
	ehdr->e_ident[EI_VERSION] = EV_CURRENT;
	ehdr->e_ident[EI_OSABI] = ELF_OSABI;
	memset(ehdr->e_ident + EI_PAD, 0, EI_NIDENT - EI_PAD);
	ehdr->e_type = ET_CORE;
	ehdr->e_machine = ELF_ARCH;
	ehdr->e_version = EV_CURRENT;
	ehdr->e_phoff = sizeof(Elf64_Ehdr);
	ehdr->e_ehsize = sizeof(Elf64_Ehdr);
	ehdr->e_phentsize = sizeof(struct elf_phdr);

	/* note date comes after section header, fill it later */
	note_phdr = phdr;
	(ehdr->e_phnum)++;
	phdr++;

	/* presume all crash header data is used to keep things simple */
	compiletime_assert((offsetof(struct crash_header_t, data) % 4) == 0,
			   "data should be word ALIGN");
	compiletime_assert(
		((SUNXI_CRASH_DATA_BUFFER_SIZE - sizeof(*crash_header)) % 4) ==
			0,
		"len should be word ALIGN");
	elf_len = crash_header->len =
		SUNXI_CRASH_DATA_BUFFER_SIZE - sizeof(*crash_header);
	/* Go through all the ranges in mem->ranges[] and prepare phdr */
	for (i = 0; i < mem->nr_ranges; i++) {
		mstart = mem->ranges[i].start;
		mend = mem->ranges[i].end;

		phdr->p_type = PT_LOAD;
		phdr->p_flags = PF_R | PF_W | PF_X;
		phdr->p_paddr = mstart;
		phdr->p_offset = elf_len;

		phdr->p_vaddr = (unsigned long)__va(mstart);
		phdr->p_filesz = phdr->p_memsz = mend - mstart;
		elf_len += phdr->p_filesz;
		phdr->p_align = 0;
		ehdr->e_phnum++;
		pr_debug(
			"Crash PT_LOAD ELF header. phdr=%p vaddr=0x%llx, paddr=0x%llx, sz=0x%llx e_phnum=%d p_offset=0x%llx\n",
			phdr, (unsigned long long)phdr->p_vaddr,
			(unsigned long long)phdr->p_paddr,
			(unsigned long long)phdr->p_filesz, ehdr->e_phnum,
			(unsigned long long)phdr->p_offset);
		phdr++;
	}

	note_phdr->p_offset = note_phdr->p_paddr = (void *)phdr - (void *)ehdr;
	fill_merged_crash_notes(
		note_phdr, (void *)crash_header->data + note_phdr->p_offset,
		SUNXI_CRASH_DATA_BUFFER_SIZE - note_phdr->p_offset -
			sizeof(*crash_header));
	crash_header->checksum = __checksum((uint32_t *)crash_header->data,
					    crash_header->len / 4);
	pr_err("sunxi crash header len: 0x%08x, checksum: 0x%08x",
	       crash_header->len, crash_header->checksum);

	return 0;
}
void prepare_kdump_header(void)
{
#if IS_ENABLED(CONFIG_ARM64)
	crash_prepare_elf64_headers(crash_mem_info, 0, NULL, NULL);
#else
	pr_err("sunxi_crash_header not supported yet\n");
#endif
}
#endif

void crash_info_init(void)
{
#if IS_ENABLED(CONFIG_KEXEC) && IS_ENABLED(CONFIG_CRASH_DUMP) && IS_BUILTIN(CONFIG_AW_CRASHDUMP)
	if (sunxi_crashdump_info)
		kfree(sunxi_crashdump_info);
	sunxi_crashdump_info = kzalloc(CRASH_CORE_NOTE_BYTES * 2, GFP_KERNEL);
#else
	crash_buffer_init();
#endif
	spin_lock_init(&cpu_dump_lock);
}


void sunxi_crash_save_cpu(struct pt_regs *regs, int cpu)
{
	struct elf_prstatus prstatus;
	u32 *buf;

	if ((cpu < 0) || (cpu >= nr_cpu_ids))
		return;

#if IS_ENABLED(CONFIG_KEXEC) && IS_ENABLED(CONFIG_CRASH_DUMP) && IS_BUILTIN(CONFIG_AW_CRASHDUMP)
	buf = (u32 *)per_cpu_ptr(crash_notes, cpu);
#else
	buf = (u32 *)per_cpu_ptr(sunxi_crash_notes, cpu);
#endif
	if (!buf)
		return;
	memset(&prstatus, 0, sizeof(prstatus));
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	prstatus.pr_pid = current->pid;
#else
	prstatus.common.pr_pid = current->pid;
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	elf_core_copy_kernel_regs(&prstatus.pr_reg, regs);
#else
	elf_core_copy_regs(&prstatus.pr_reg, regs);
#endif
	buf = append_elf_note(buf, CRASH_CORE_NOTE_NAME, NT_PRSTATUS,
			      &prstatus, sizeof(prstatus));
	final_note(buf);
}

#if IS_ENABLED(CONFIG_KEXEC) && IS_ENABLED(CONFIG_CRASH_DUMP) && IS_BUILTIN(CONFIG_AW_CRASHDUMP)
void crash_setup_elfheader(void)
{
	struct elf_phdr *nhdr = NULL;
	sunxi_crashdump_info += sizeof(struct elfhdr);
	nhdr = (struct elf_phdr *)sunxi_crashdump_info;
	memset(nhdr, 0, sizeof(struct elf_phdr));
	nhdr->p_memsz = CRASH_CORE_NOTE_BYTES;
}

#endif /*IS_ENABLED(CONFIG_KEXEC) && IS_ENABLED(CONFIG_CRASH_DUMP) && IS_BUILTIN(CONFIG_AW_CRASHDUMP)*/

#if IS_ENABLED(CONFIG_ARM)
static void sunxi_show_regs32(struct pt_regs *regs)
{
	unsigned long flags;
	char buf[64];
#ifndef CONFIG_CPU_V7M
	unsigned int domain;
#ifdef CONFIG_CPU_SW_DOMAIN_PAN
	/*
	 * Get the domain register for the parent context. In user
	 * mode, we don't save the DACR, so lets use what it should
	 * be. For other modes, we place it after the pt_regs struct.
	 */
	if (user_mode(regs)) {
		domain = DACR_UACCESS_ENABLE;
	} else {
		domain = to_svc_pt_regs(regs)->dacr;
	}
#else
	domain = get_domain();
#endif
#endif

#if IS_BUILTIN(CONFIG_AW_CRASHDUMP)
	show_regs_print_info(KERN_DEFAULT);
#endif

	printk("PC is at %pS\n", (void *)instruction_pointer(regs));
	printk("LR is at %pS\n", (void *)regs->ARM_lr);
	printk("pc : [<%08lx>]    lr : [<%08lx>]    psr: %08lx\n",
	       regs->ARM_pc, regs->ARM_lr, regs->ARM_cpsr);
	printk("sp : %08lx  ip : %08lx  fp : %08lx\n",
	       regs->ARM_sp, regs->ARM_ip, regs->ARM_fp);
	printk("r10: %08lx  r9 : %08lx  r8 : %08lx\n",
		regs->ARM_r10, regs->ARM_r9,
		regs->ARM_r8);
	printk("r7 : %08lx  r6 : %08lx  r5 : %08lx  r4 : %08lx\n",
		regs->ARM_r7, regs->ARM_r6,
		regs->ARM_r5, regs->ARM_r4);
	printk("r3 : %08lx  r2 : %08lx  r1 : %08lx  r0 : %08lx\n",
		regs->ARM_r3, regs->ARM_r2,
		regs->ARM_r1, regs->ARM_r0);

	flags = regs->ARM_cpsr;
	buf[0] = flags & PSR_N_BIT ? 'N' : 'n';
	buf[1] = flags & PSR_Z_BIT ? 'Z' : 'z';
	buf[2] = flags & PSR_C_BIT ? 'C' : 'c';
	buf[3] = flags & PSR_V_BIT ? 'V' : 'v';
	buf[4] = '\0';

#ifndef CONFIG_CPU_V7M
	{
		const char *segment;

		if ((domain & domain_mask(DOMAIN_USER)) ==
		    domain_val(DOMAIN_USER, DOMAIN_NOACCESS))
			segment = "none";
		else
			segment = "user";

		printk("Flags: %s  IRQs o%s  FIQs o%s  Mode %s  ISA %s  Segment %s\n",
			buf, interrupts_enabled(regs) ? "n" : "ff",
			fast_interrupts_enabled(regs) ? "n" : "ff",
			processor_modes[processor_mode(regs)],
			isa_modes[isa_mode(regs)], segment);
	}
#else
	printk("xPSR: %08lx\n", regs->ARM_cpsr);
#endif

#ifdef CONFIG_CPU_CP15
	{
		unsigned int ctrl;

		buf[0] = '\0';
#ifdef CONFIG_CPU_CP15_MMU
		{
			unsigned int transbase;
			asm("mrc p15, 0, %0, c2, c0\n\t"
			    : "=r" (transbase));
			snprintf(buf, sizeof(buf), "  Table: %08x  DAC: %08x",
				transbase, domain);
		}
#endif
		asm("mrc p15, 0, %0, c1, c0\n" : "=r" (ctrl));

		printk("Control: %08x%s\n", ctrl, buf);
	}
#endif
}
#endif

#if IS_ENABLED(CONFIG_ARM64)
static void sunxi_show_regs64(struct pt_regs *regs)
{
	int i, top_reg;
	u64 lr, sp;

	printk("\ncpu %d regs status:\n", smp_processor_id());
	if (compat_user_mode(regs)) {
		lr = regs->compat_lr;
		sp = regs->compat_sp;
		top_reg = 12;
	} else {
		lr = regs->regs[30];
		sp = regs->sp;
		top_reg = 29;
	}

	if (!user_mode(regs)) {
		printk("pc : %pS\n", (void *)regs->pc);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
		printk("lr : %pS\n", (void *)ptrauth_strip_kernel_insn_pac(lr));
#else
		printk("lr : %pS\n", (void *)ptrauth_clear_pac(lr));
#endif
	} else {
		printk("pc : %016llx\n", regs->pc);
		printk("lr : %016llx\n", lr);
	}
	i = top_reg;

	while (i >= 0) {
		printk("x%-2d: %016llx", i, regs->regs[i]);

		while (i-- % 3)
			pr_cont(" x%-2d: %016llx", i, regs->regs[i]);

		pr_cont("\n");
	}
}
#endif

static void sunxi_show_regs(struct pt_regs *regs)
{
#if IS_ENABLED(CONFIG_ARM64)
	sunxi_show_regs64(regs);
#endif

#if IS_ENABLED(CONFIG_ARM)
	sunxi_show_regs32(regs);
#endif
}

void sunxi_dump_status(struct pt_regs *regs)
{
	unsigned long flags;
	spin_lock_irqsave(&cpu_dump_lock, flags);
	sunxi_show_regs(regs);
	dump_stack();
	spin_unlock_irqrestore(&cpu_dump_lock, flags);
}
