/*
 * mntn_dump.h
 *
 * dump the register of mntn.
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
#ifndef __MNTN_DUMP_H
#define __MNTN_DUMP_H
#include <linux/types.h>
#include <mntn_public_interface.h>

#ifdef CONFIG_PLAT_MNTNDUMP
extern int register_mntn_dump(int mod_id, unsigned int size, void **vaddr);
#else
static inline int register_mntn_dump(int mod_id, unsigned int size, void **vaddr) { return -1; }
#endif

#ifdef CONFIG_PLAT_KERNELDUMP
void kd_save_state_shutdown(void);
int kd_add_reserved_mem(void *vaddr, phys_addr_t paddr, u64 size);
#else
static inline void kd_save_state_shutdown(void) {}
static inline int kd_add_reserved_mem(void *vaddr, phys_addr_t paddr, u64 size) {return 0;}
#endif

#endif
