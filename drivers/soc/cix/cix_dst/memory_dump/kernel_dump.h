/* SPDX-License-Identifier: GPL-2.0-only */
// Copyright 2025 Cix Technology Group Co., Ltd.

#ifndef __MEMORY_DUMP_H
#define __MEMORY_DUMP_H
#include <linux/mm_types.h>
#include <linux/soc/cix/mntn_dump.h>

#define KERNELDUMP_CB_MAGIC 0xDEADBEEFDEADBEEF
#define KERNELDUMP_CB_MAX_SEC (128UL)

struct kernel_dump_cb {
	u64 magic; /*mem dump block magic, default value is 0xdeadbeefdeadbeef*/
	u32 checksum;
	u32 reserved;
	char buf[];
};

#endif
