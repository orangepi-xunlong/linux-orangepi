// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef _SUNXI_MINIDUMP_H
#define _SUNXI_MINIDUMP_H

#include <asm/ptrace.h>

#define REG_SP_INDEX	31
#define REG_PC_INDEX	32

enum reg_arch_type {
	ARM,
	ARM64,
	RSICV
};

#define REGS_NUM_MAX 50
#define SECTION_NUM_MAX 20
struct regs_info {
	int arch;
	int num;
	int valid_reg_num;
	int per_reg_memory_size;
};

struct minidump_info {
	char kernel_magic[6];
	struct regs_info regs_info;
	void *elf_header;
	u32 elf_size;
	u32 poffset_pos;
	u32 phdr_max_num;
};

int sunxi_md_init(void);
void sunxi_md_exec(struct pt_regs *recorded_regs);
unsigned long sunxi_md_get_elf_pa(void);

#endif
