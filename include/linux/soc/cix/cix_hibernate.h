// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Cix Technology Group Co., Ltd.*/
/**
 * SoC: CIX SKY1 platform
 */
#ifndef __CIX_HIBERNATE_H
#define __CIX_HIBERNATE_H

#include <linux/list.h>

struct hibernate_rmem_ops {
	struct list_head node;
	char *name;
	u64 paddr;
	u64 size;
	int (*suspend)(u64 vaddr, u64 size);
	void (*resume)(u64 vaddr, u64 size);
	void *priv;
};

extern void register_reserve_mem_ops(struct hibernate_rmem_ops *ops);
extern void unregister_reserve_mem_ops(struct hibernate_rmem_ops *ops);
#ifdef CONFIG_PM_SLEEP
extern int reserve_mem_suspend(void);
extern void reserve_mem_resume(void);
#endif

#endif
