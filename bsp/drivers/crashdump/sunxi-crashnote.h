/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

#ifndef _SUNXI_CRASHNOTE_H_
#define _SUNXI_CRASHNOTE_H_

#include <linux/linkage.h>
#include <linux/elfcore.h>
#include <linux/elf.h>

#define	NUMA_NO_NODE	(-1)

#define CRASH_CORE_NOTE_NAME	   "CORE"
#define CRASH_CORE_NOTE_HEAD_BYTES ALIGN(sizeof(struct elf_note), 4)
#define CRASH_CORE_NOTE_NAME_BYTES ALIGN(sizeof(CRASH_CORE_NOTE_NAME), 4)
#define CRASH_CORE_NOTE_DESC_BYTES ALIGN(sizeof(struct elf_prstatus), 4)

/*
 * The per-cpu notes area is a list of notes terminated by a "NULL"
 * note header.  For kdump, the code in vmcore.c runs in the context
 * of the second kernel to combine them into one note.
 */
#define CRASH_CORE_NOTE_BYTES	   ((CRASH_CORE_NOTE_HEAD_BYTES * 2) +	\
				     CRASH_CORE_NOTE_NAME_BYTES +	\
				     CRASH_CORE_NOTE_DESC_BYTES)

void sunxi_crashdump_exec(struct pt_regs *recorded_regs);
void sunxi_dump_status(struct pt_regs *regs);
void crash_info_init(void);
void prepare_kdump_header(void);

#if IS_ENABLED(CONFIG_KEXEC) && IS_ENABLED(CONFIG_CRASH_DUMP) && IS_BUILTIN(CONFIG_AW_CRASHDUMP)

void sunxi_crash_save_cpu(struct pt_regs *regs, int cpu);
void crash_setup_elfheader(void);

#else
void sunxi_crash_save_cpu(struct pt_regs *regs, int cpu);
#endif /*IS_ENABLED(CONFIG_KEXEC) && IS_ENABLED(CONFIG_CRASH_DUMP) && IS_BUILTIN(CONFIG_AW_CRASHDUMP)*/

#endif
