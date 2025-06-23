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
#include <mntn_public_interface.h>

#ifdef CONFIG_PLAT_MNTNDUMP
extern int register_mntn_dump(int mod_id, unsigned int size, void **vaddr);
#else
static inline int register_mntn_dump(int mod_id, unsigned int size, void **vaddr) { return -1; }
#endif

#endif
