// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright(c) 2024-2025 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Allwinner sunxi minidump debug
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

//#define DEBUG
#define pr_fmt(fmt)  "sunxi_minidump: " fmt
#define SUNXI_MINIDUMP_VERSION	"1.0.0"

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/proc_fs.h>
#include <linux/reboot.h>
#include <linux/rtc.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/sysrq.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/regmap.h>
#include <linux/kdebug.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/crypto.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <asm/kexec.h>
#if IS_ENABLED(CONFIG_AW_KERNEL_AOSP)
#include <linux/android_debug_symbols.h>
#endif
#include <sunxi-log.h>
#include "sunxi-minidump.h"

struct minidump_info md_info_g = {
#ifdef CONFIG_ARM64
	.regs_info			= {
		.arch			= ARM64,
		.num			= 33, /* x0~x30 and SP(x31), PC x30 = lr */
		.per_reg_memory_size	= 256,
		.valid_reg_num		= 0,
	},
#endif
#ifdef CONFIG_ARM
	.regs_info			= {
		.arch			= ARM,
		.num			= 16,
		.per_reg_memory_size	= 256,
		.valid_reg_num		= 0,
	},
#endif
};

static int sunxi_md_read(struct seq_file *s, void *v)
{
	/* @TODO */
	return 0;
}

static int sunxi_md_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_md_read, NULL);
}

static int sunxi_md_add_phdr(void *v_addr, phys_addr_t p_addr, u32 size)
{
	struct elfhdr *ehdr;
	struct elf_phdr *phdr;

	if (!v_addr) {
		sunxi_err(NULL, "v_addr is null, can't add phdr\n");
		return -EINVAL;
	}

	ehdr = (struct elfhdr *)(md_info_g.elf_header);
	if (ehdr->e_phnum > md_info_g.phdr_max_num) {
		sunxi_err(NULL, "exceed max phdr_num[%d]\n", md_info_g.phdr_max_num);
		return -EINVAL;
	}

	phdr = (struct elf_phdr *)(ehdr + 1);
	phdr = phdr + ehdr->e_phnum;

	phdr->p_type = PT_LOAD;
	phdr->p_flags = PF_R | PF_W | PF_X;
	phdr->p_paddr = (elf_addr_t)p_addr;
	phdr->p_offset = md_info_g.poffset_pos;

	phdr->p_vaddr = (elf_addr_t)v_addr;
	phdr->p_filesz = size;
	phdr->p_align = 0;

	sunxi_debug(NULL,
		"Add phdr:phdr=%p vaddr=0x%llx, paddr=0x%llx, sz=0x%llx e_phnum=%d p_offset=0x%llx\n",
		phdr, (unsigned long long)phdr->p_vaddr,
		(unsigned long long)phdr->p_paddr,
		(unsigned long long)phdr->p_filesz, ehdr->e_phnum,
		(unsigned long long)phdr->p_offset);
	print_hex_dump_bytes("phdr data", DUMP_PREFIX_ADDRESS, (void *)phdr->p_vaddr, 32);

	ehdr->e_phnum++;
	md_info_g.elf_size += sizeof(*phdr);
	md_info_g.poffset_pos += phdr->p_filesz;

	return 0;
}

static int sunxi_md_prepare_ehdr(void)
{
	struct elfhdr *ehdr;
	struct elf_phdr *phdr;

	md_info_g.elf_header = kzalloc(sizeof(*ehdr) + sizeof(*phdr) * (REGS_NUM_MAX + SECTION_NUM_MAX), GFP_KERNEL);
	if (!md_info_g.elf_header)
		return -ENOMEM;

	ehdr = (struct elfhdr *)(md_info_g.elf_header);

	memcpy(ehdr->e_ident, ELFMAG, SELFMAG);
	ehdr->e_ident[EI_CLASS] = ELFCLASS64;
	ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
	ehdr->e_ident[EI_VERSION] = EV_CURRENT;
	ehdr->e_ident[EI_OSABI] = ELF_OSABI;
	memset(ehdr->e_ident + EI_PAD, 0, EI_NIDENT - EI_PAD);
	ehdr->e_type = ET_CORE;
	ehdr->e_machine = ELF_ARCH;
	ehdr->e_version = EV_CURRENT;
	ehdr->e_phoff = sizeof(*ehdr);
	ehdr->e_ehsize = sizeof(*ehdr);
	ehdr->e_phentsize = sizeof(struct elf_phdr);
	ehdr->e_phnum = 0;

	md_info_g.elf_size = sizeof(*ehdr);
	md_info_g.poffset_pos = sizeof(*ehdr) + sizeof(*phdr) * (REGS_NUM_MAX + SECTION_NUM_MAX);
	md_info_g.phdr_max_num = REGS_NUM_MAX + SECTION_NUM_MAX;

	return 0;
}

static int sunxi_md_write_data(const char *section_name, unsigned long addr, unsigned long end)
{
#if !IS_ENABLED(CONFIG_AW_KERNEL_AOSP)
	size_t ret;
	struct file *filep;
	loff_t pos = 0;
	unsigned long size = end - addr;
	char file_name[50] = {0};

	sprintf(file_name, "/tmp/%s", section_name);
	sunxi_info(NULL, "Writing to %s\n", file_name);

	filep = filp_open(file_name, O_RDWR | O_APPEND | O_CREAT, 0644);
	if (IS_ERR(filep)) {
		sunxi_err(NULL, "Open file %s error\n", file_name);
		return PTR_ERR(filep);
	}

	ret = kernel_write(filep, (void *)addr, size, &pos);
	if (pos < 0) {
		sunxi_err(NULL, "Write data to %s failed\n", file_name);
		goto out;
	}

	/* @TODO */
out:
	fput(filep);
	return ret;
#endif
	return 0;
}

static void sunxi_md_prepare_ptregs(struct pt_regs *recorded_regs)
{
	int i;
	unsigned long addr;

	for (i = 0; i < md_info_g.regs_info.num; i++) {
		if (REG_SP_INDEX == i)
			addr = recorded_regs->sp - md_info_g.regs_info.per_reg_memory_size / 2;
		else if (REG_PC_INDEX == i)
			addr = recorded_regs->pc - md_info_g.regs_info.per_reg_memory_size / 2;
		else
			addr = recorded_regs->regs[i] - md_info_g.regs_info.per_reg_memory_size / 2;

		if (addr > PAGE_OFFSET && addr < PAGE_END) {
			sunxi_md_add_phdr((void *)addr, virt_to_phys((void *)addr),
						md_info_g.regs_info.per_reg_memory_size);
			md_info_g.regs_info.valid_reg_num++;
			sunxi_debug(NULL, "prepare ptregs: R[%d] vaddr: 0x%lx, paddr: 0x%llx\n",
				    i, addr, virt_to_phys((void *)addr));
		}
	}
	sunxi_debug(NULL, "Prepare valid regs num: %d\n", md_info_g.regs_info.valid_reg_num);
}

static int sunxi_md_prepare_sections(void)
{
#if IS_ENABLED(CONFIG_ANDROID_DEBUG_SYMBOLS)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
	size_t sec_size;
	int cpu;
	void *start, *end;
	int ret;
	/* Depends on CONFIG_ANDROID_DEBUG_SYMBOLS */
	start = android_debug_symbol(ADS_TEXT);
	end = android_debug_symbol(ADS_SEND);
	if (!start || !end) {
		sunxi_err(NULL, "Can't get code segment addr\n");
		return -ENXIO;
	}
	sec_size = roundup(end - start, 4);
	ret = sunxi_md_add_phdr(start, virt_to_phys(start), sec_size);
	if (ret) {
		sunxi_err(NULL, "Can't add code segment to phdr\n");
		return ret;
	}

	start = android_debug_symbol(ADS_PER_CPU_START);
	end = android_debug_symbol(ADS_PER_CPU_END);
	sec_size = end - start;
	for_each_possible_cpu(cpu) {
		void *percpu_start = per_cpu_ptr((void __percpu *)start, cpu);
		sunxi_md_add_phdr(percpu_start, per_cpu_ptr_to_phys(percpu_start), sec_size);
	}
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0) */
#endif /* IS_ENABLED(CONFIG_ANDROID_DEBUG_SYMBOLS) */
	return 0;
}

static int sunxi_md_dump_all(void)
{
	sunxi_md_write_data("kernel_elf_header", (unsigned long)md_info_g.elf_header,
			    (unsigned long)(md_info_g.elf_header + md_info_g.elf_size));

	return 0;
}

#define MAX_MINIDUMP_WRITE	5
static ssize_t sunxi_md_write(struct file *file, const char __user *buf,
				size_t count, loff_t *data)
{
	char minidump_buf[MAX_MINIDUMP_WRITE + 1] = { 0 };

	if (count > MAX_MINIDUMP_WRITE) {
		sunxi_err(NULL, "Exceed max len\n");
		return -EINVAL;
	}

	if (copy_from_user(minidump_buf, buf, count)) {
		sunxi_err(NULL, "Copy_from_user failed!\n");
		return -EINVAL;
	}

	minidump_buf[count] = '\0';

	if (!strncmp(minidump_buf, "dump", 4)) {
		sunxi_md_dump_all();
	}

	return count;
}

void sunxi_md_exec(struct pt_regs *recorded_regs)
{
	sunxi_md_prepare_ptregs(recorded_regs);
}

unsigned long sunxi_md_get_elf_pa(void)
{
	return virt_to_phys(md_info_g.elf_header);
}

static const struct proc_ops md_proc_fops = {
	.proc_open = sunxi_md_open,
	.proc_read = seq_read,
	.proc_write = sunxi_md_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

int sunxi_md_init(void)
{
	int ret;
	struct proc_dir_entry *sunxi_md_proc;

	sunxi_md_proc = proc_create("sunxi_minidump", S_IWUSR | S_IRUSR, NULL, &md_proc_fops);
	if (!sunxi_md_proc)
		return -ENOMEM;

	ret = sunxi_md_prepare_ehdr();
	if (ret) {
		sunxi_err(NULL, "Prepare ehdr failed!\n");
		return ret;
	}

	ret = sunxi_md_prepare_sections();
	if (ret) {
		sunxi_err(NULL, "Prepare sections failed\n");
		return ret;
	}

	return 0;
}
