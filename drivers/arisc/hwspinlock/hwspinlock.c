/*
 *  arch/arm/mach-sunxi/arisc/hwspinlock/hwspinlock.c
 *
 * Copyright (c) 2012 Allwinner.
 * 2012-05-01 Written by sunny (sunny@allwinnertech.com).
 * 2012-10-01 Written by superm (superm@allwinnertech.com).
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
#include <linux/spinlock.h>
#include "hwspinlock_i.h"

/**
 * initialize hwspinlock.
 * @para:  none.
 *
 * returns:  0 if initialize hwspinlock succeeded, others if failed.
 */
int arisc_hwspinlock_init(void)
{
	return 0;
}

/**
 * exit hwspinlock.
 * @para:none.
 *
 * returns:  0 if exit hwspinlock succeeded, others if failed.
 */
int arisc_hwspinlock_exit(void)
{
	return 0;
}

/**
 * lock an hwspinlock with timeout limit,
 * and hwspinlock will be unlocked in arisc_hwspin_unlock().
 * @hwid: an hwspinlock id which we want to lock.
 *
 * returns:  0 if lock hwspinlock succeeded, other if failed.
 */
int arisc_hwspin_lock_timeout(int hwid, unsigned int timeout, \
                              spinlock_t *lock, unsigned long *flags)
{
	unsigned long expire;

	expire = msecs_to_jiffies(timeout) + jiffies;

	if (hwid >= ARISC_HW_SPINLOCK_NUM) {
		ARISC_ERR("invalid hwspinlock id [%d] for trylock\n", hwid);
		return -EINVAL;
	}

	/* is lock already taken by another context on the local cpu ? */
	while (!(spin_trylock_irqsave(lock, *flags))) {
		if (time_is_before_eq_jiffies(expire)) {
			ARISC_ERR("try to take spinlock fail\n");
			return -EBUSY;
		}
	}

	/* try to take spinlock */
	while (readl(IO_ADDRESS(AW_SPINLOCK_LOCK_REG(hwid))) == AW_SPINLOCK_TAKEN) {
		/*
		 * The lock is already taken, let's check if the user wants
		 * us to try again
		 */
		if (time_is_before_eq_jiffies(expire)) {
			ARISC_ERR("try to take hwspinlock timeout\n");
			spin_unlock_irqrestore(lock, *flags);
			return -ETIMEDOUT;
		}
	}

	return 0;
}
EXPORT_SYMBOL(arisc_hwspin_lock_timeout);

/**
 * unlock a specific hwspinlock.
 * hwid:  an hwspinlock id which we want to unlock.
 *
 * returns:  0 if unlock hwspinlock succeeded, other if failed.
 */
int arisc_hwspin_unlock(int hwid, spinlock_t *lock, unsigned long *flags)
{
	if (hwid >= ARISC_HW_SPINLOCK_NUM) {
		ARISC_ERR("invalid hwspinlock id [%d] for unlock\n", hwid);
		return -EINVAL;
	}

	/* untaken the spinlock */
	writel(0x0, IO_ADDRESS(AW_SPINLOCK_LOCK_REG(hwid)));

	spin_unlock_irqrestore(lock, *flags);

	return 0;
}
EXPORT_SYMBOL(arisc_hwspin_unlock);

int arisc_hwspinlock_standby_suspend(void)
{
	return 0;
}

int arisc_hwspinlock_standby_resume(void)
{
	return 0;
}
