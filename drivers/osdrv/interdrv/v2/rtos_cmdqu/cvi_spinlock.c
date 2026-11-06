// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_spinlock.c
 * Description:
 */

#ifdef FREERTOS_BSP
#include "cvi_spinlock.h"
#include "arch_time.h"
#include "core_rv64.h"
#include "csi_rv64_gcc.h"
#include "csr.h"
#include "delay.h"
#include "mmio.h"
#include "stdint.h"
#include "top_reg.h"
#include "types.h"
#else
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/of_reserved_mem.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include "cvi_spinlock.h"
#endif

#ifndef FREERTOS_BSP
static unsigned long reg_base;
#else
static unsigned long reg_base = SPINLOCK_REG_BASE;
#endif

spinlock_t reg_write_lock;
static unsigned char lockCount;
static void *__iomem c906l_pc_reg;

void cvi_spinlock_init(void)
{
	spin_lock_init(&reg_write_lock);
	lockCount = 0;
	c906l_pc_reg = ioremap(0x1901070, 4);
	if (c906l_pc_reg == NULL) {
		pr_err("c906l_pc_reg ioremap failed!\n");
	}
	pr_info("[%s] success\n", __func__);
}

void cvi_spinlock_uninit(void)
{
	iounmap(c906l_pc_reg);
}

void spinlock_base(unsigned long mb_base)
{
	reg_base = mb_base;
}

static inline int hw_spin_trylock(hw_raw_spinlock_t *lock)
{
#ifndef RISCV_QEMU
	writew(lock->locks, (void *)(reg_base + sizeof(int) * lock->hw_field));
	if (readw((void *)(reg_base + sizeof(int) * lock->hw_field)) == lock->locks)
		return MAILBOX_LOCK_SUCCESS;
	return MAILBOX_LOCK_FAILED;
#else
	return MAILBOX_LOCK_SUCCESS;
#endif
}

int hw_spin_lock(hw_raw_spinlock_t *lock)
{
	u64 i;
	u64 loops = 1000000;

	if (lock->hw_field >= SPIN_LINUX_RTOS) {
		unsigned long flags;
		spin_lock_irqsave(&reg_write_lock, flags);
		lock->locks = lockCount | 0x100;
		lockCount++;
		spin_unlock_irqrestore(&reg_write_lock, flags);
	}
	else {
		//....
	}
	for (i = 0; i < loops; i++) {
		if (hw_spin_trylock(lock) == MAILBOX_LOCK_SUCCESS)
			return MAILBOX_LOCK_SUCCESS;
		udelay(1);
	}

#ifdef FREERTOS_BSP
	uart_puts("__spin_lock_debug fail\n");
#else
	pr_err("__spin_lock_debug fail! loops = %lld\n", loops);
#endif
	return MAILBOX_LOCK_FAILED;
}

int _hw_raw_spin_lock_irqsave(hw_raw_spinlock_t *lock)
{
	int flag = MAILBOX_LOCK_SUCCESS;

#ifdef FREERTOS_BSP
	// save and disable irq
	flag = (__get_MSTATUS() & 8);
	__disable_irq();
#endif

	// lock
	if (hw_spin_lock(lock) == MAILBOX_LOCK_FAILED) {
	#ifdef FREERTOS_BSP
		// if spinlock fail , restore irq
		if (flag) {
			__enable_irq();
		}
	#endif
		pr_err("spin lock fail! C906L pc = 0x%x,reg_val=0x%x, lock->locks=0x%x\n",
			ioread32(c906l_pc_reg), readw((void *)(reg_base + sizeof(int) * lock->hw_field)), lock->locks);
		return MAILBOX_LOCK_FAILED;
	}
	return flag;
}

void _hw_raw_spin_unlock_irqrestore(hw_raw_spinlock_t *lock, int flag)
{
#ifndef RISCV_QEMU
	// unlock
	if (readw((void *)(reg_base + sizeof(int) * lock->hw_field)) == lock->locks) {
		writew(lock->locks, (void *)(reg_base + sizeof(int) * lock->hw_field));

#ifdef FREERTOS_BSP
		// restore irq
		if (flag) {
			__enable_irq();
		}
#endif
	} else {
#ifdef FREERTOS_BSP
		uart_puts("spin unlock fail\n");
#else
		pr_err("spin unlock fail! C906L pc=0x%x,reg_val=0x%x, lock->locks=0x%x\n",
			ioread32(c906l_pc_reg), readw((void *)(reg_base + sizeof(int) * lock->hw_field)), lock->locks);
#endif
	}
#else
	if (flag) {
		__enable_irq();
	}
#endif
}
