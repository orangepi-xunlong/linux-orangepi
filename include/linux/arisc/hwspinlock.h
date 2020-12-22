/*
 *  include/linux/arisc/hwspinlock.h
 *
 * Copyright 2012 (c) Allwinner.
 * sunny (sunny@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef	__ASM_ARCH_HWSPINLOCK_H
#define	__ASM_ARCH_HWSPINLOCK_H

#include <linux/spinlock.h>

#define AW_MSG_HWSPINLOCK               (0)
#define AW_AUDIO_HWSPINLOCK             (1)
#define AW_RTC_REG_HWSPINLOCK           (2)
#define SUNXI_HWSPINLOCKID_CPUIDLE      (3)

/* spinlock max timeout, base on ms */
#define ARISC_SPINLOCK_TIMEOUT          (100)

//the taken ot not state of spinlock
#define	AW_SPINLOCK_NOTTAKEN      (0)
#define	AW_SPINLOCK_TAKEN         (1)

//hardware spinlock register list
#define	AW_SPINLOCK_SYS_STATUS_REG		(SUNXI_SPINLOCK_PBASE + 0x0000)
#define	AW_SPINLOCK_STATUS_REG			(SUNXI_SPINLOCK_PBASE + 0x0010)
#define	AW_SPINLOCK_IRQ_EN_REG			(SUNXI_SPINLOCK_PBASE + 0x0020)
#define AW_SPINLOCK_IRQ_PEND_REG		(SUNXI_SPINLOCK_PBASE + 0x0040)
#define AW_SPINLOCK_LOCK_REG(id)		(SUNXI_SPINLOCK_PBASE + 0x0100 + id * 4)

/**
 * lock an hwspinlock with timeout limit.
 * @hwid: an hwspinlock id which we want to lock.
 *
 * returns:  0 if lock hwspinlock succeeded, other if failed.
 */
int arisc_hwspin_lock_timeout(int hwid, unsigned int timeout, \
                              spinlock_t *lock, unsigned long *flags);

/**
 * unlock a specific hwspinlock.
 * hwid:  an hwspinlock id which we want to unlock.
 *
 * returns:  0 if unlock hwspinlock succeeded, other if failed.
 */
int arisc_hwspin_unlock(int hwid, spinlock_t *lock, unsigned long *flags);
#endif	//__ASM_ARCH_HWSPINLOCK_H
