/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2007-2021 Allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SUNXI_AFBD_HWSPINLOCK_H_
#define _SUNXI_AFBD_HWSPINLOCK_H_

#include <linux/hwspinlock.h>

#ifdef CONFIG_SUNXI_AFBD_WORKAROUND

#define AFBD_BUG_WORKAROUND

int __afbd_hwspinlock_lock(int mode, unsigned long *flags);
void __afbd_hwspinlock_unlock(int mode, unsigned long *flags);

/*
 * Synchronization of access to the afbd register is achieved using hwspinlock.
 *
 * Returns 0 when the @hwlock was successfully taken,
 * and an appropriate error code otherwise
 * (most notably an -ETIMEDOUT if the @hwlock is still
 * busy after @timeout msecs).
 */
static inline int afbd_read_lock(unsigned long *flags)
{
	return __afbd_hwspinlock_lock(HWLOCK_IRQSTATE, flags);
}

/*
 * unlock the specific hwspinlock acquired by afbd_read_lock().
 */
static inline void afbd_read_unlock(unsigned long *flags)
{
	__afbd_hwspinlock_unlock(HWLOCK_IRQSTATE, flags);
}

#else

static inline int __afbd_hwspinlock_lock(int mode, unsigned long *flags)
{
	return 0;
}

static inline void __afbd_hwspinlock_unlock(int mode, unsigned long *flags)
{
}

static inline int afbd_read_lock(unsigned long *flags)
{
	return 0;
}

static inline void afbd_read_unlock(unsigned long *flags)
{
}

#endif

#endif
