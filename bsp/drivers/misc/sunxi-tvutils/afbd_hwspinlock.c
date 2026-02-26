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

#include <linux/atomic.h>
#include <linux/debugfs.h>
#include <linux/hwspinlock.h>
#include <linux/module.h>

#define AFBD_BUG_WORKAROUND     1
#define AFBD_READ_HWSPINLOCK_ID 4
#define AFBD_READ_LOCK_TIMEOUT  3 /* 1 msecs */

static struct hwspinlock *hwlock;

struct hwspinlock_debug_info {
	struct dentry *dbgfs;
	atomic_t success_count;
	atomic_t failed_count;
};

static struct hwspinlock_debug_info dbginfo;

static int hwspinlock_debugfs_show(struct seq_file *s, void *unused)
{
	seq_puts(s, "afbd hwspinlock debug info:\n");
	seq_printf(s, " success locked count: %d\n", atomic_read(&dbginfo.success_count));
	seq_printf(s, "  failed locked count: %d\n", atomic_read(&dbginfo.failed_count));
	return 0;
}

DEFINE_SHOW_ATTRIBUTE(hwspinlock_debugfs);

static int hwspinlock_debugfs_init(void)
{
	atomic_set(&dbginfo.success_count, 0);
	atomic_set(&dbginfo.failed_count,  0);

	dbginfo.dbgfs = debugfs_create_dir("afbd-hwspinlock", NULL);
	debugfs_create_file_unsafe("info", 0444, dbginfo.dbgfs, NULL,
			&hwspinlock_debugfs_fops);
	return 0;
}

static int __init afbd_hwspinlock_init(void)
{
	hwlock = hwspin_lock_request_specific(AFBD_READ_HWSPINLOCK_ID);
	if (!hwlock) {
		pr_err("hwspinlock request failed: id=%d\n", AFBD_READ_HWSPINLOCK_ID);
		return -1;
	}

	hwspinlock_debugfs_init();

	pr_err("afbd workaround hwspinlock init successfully!\n");
	return 0;
}

/*
 * Returns 0 when the @hwlock was successfully taken,
 * and an appropriate error code otherwise
 * (most notably an -ETIMEDOUT if the @hwlock is still
 * busy after @timeout msecs).
 */
int __afbd_hwspinlock_lock(int mode, unsigned long *flags)
{
	int ret = 0;

	ret = __hwspin_lock_timeout(hwlock, AFBD_READ_LOCK_TIMEOUT, mode, flags);
	if (ret == 0)
		atomic_inc(&dbginfo.success_count);
	else
		atomic_inc(&dbginfo.failed_count);

	return ret;
}
EXPORT_SYMBOL_GPL(__afbd_hwspinlock_lock);

void __afbd_hwspinlock_unlock(int mode, unsigned long *flags)
{
	__hwspin_unlock(hwlock, mode, flags);
}
EXPORT_SYMBOL_GPL(__afbd_hwspinlock_unlock);

int afbd_read_lock(unsigned long *flags)
{
	return __afbd_hwspinlock_lock(HWLOCK_IRQSTATE, flags);
}
EXPORT_SYMBOL_GPL(afbd_read_lock);

void afbd_read_unlock(unsigned long *flags)
{
	__afbd_hwspinlock_unlock(HWLOCK_IRQSTATE, flags);
}
EXPORT_SYMBOL_GPL(afbd_read_unlock);

int afbd_read_lock_in_atomic(void)
{
	return __afbd_hwspinlock_lock(HWLOCK_IN_ATOMIC, NULL);
}
EXPORT_SYMBOL_GPL(afbd_read_lock_in_atomic);

void afbd_read_unlock_in_atomic(void)
{
	__afbd_hwspinlock_unlock(HWLOCK_IN_ATOMIC, NULL);
}
EXPORT_SYMBOL_GPL(afbd_read_unlock_in_atomic);

module_init(afbd_hwspinlock_init);

MODULE_AUTHOR("Allwinnertech Co., Ltd");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
