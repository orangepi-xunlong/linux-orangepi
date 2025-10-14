// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 */

#include <uapi/linux/mount.h>
#include <linux/statfs.h>
#include <linux/namei.h>
#include <linux/delay.h>
#include <linux/syscalls.h>
#include "rdr_print.h"
#include "rdr_inner.h"

static int get_fs_stat(const char *path, struct kstatfs *statfs)
{
	struct path p;
	int ret = kern_path(path, 0, &p);

	if (ret)
		return ret;

	ret = vfs_statfs(&p, statfs);
	if (ret)
		DST_ERR("failed to getattr: %s %d\n", path, ret);

	path_put(&p);

	return ret;
}

int rdr_wait_partition(const char *path, int timeouts, int mode)
{
	struct kstat m_stat;
	struct kstatfs m_statfs;
	int timeo;
	int ret;
	bool need_write = false;

	BB_PR_START();
	if (path == NULL) {
		BB_ERR("invalid  parameter path\n");
		BB_PR_END();
		return -1;
	}

	for (;;) {
		if (rdr_get_suspend_state()) {
			BB_PN("wait for suspend\n");
			msleep(WAIT_TIME);
		} else if (rdr_get_reboot_state()) {
			BB_PN("wait for reboot\n");
			msleep(WAIT_TIME);
		} else {
			break;
		}
	}

	timeo = timeouts;

	while (1) {
		ret = rdr_vfs_stat(path, &m_stat);
		if (ret)
			goto delay;

		if (mode & (S_IWUSR | S_IWGRP | S_IWOTH))
			need_write = true;

		if (need_write && get_fs_stat(path, &m_statfs)) {
			BB_PN("get_fs_stat error\n");
			goto delay;
		}

		if (need_write && (m_statfs.f_flags & MS_RDONLY))
			goto delay;

		if ((m_stat.mode & mode) == mode)
			break;

delay:
		set_current_state(TASK_INTERRUPTIBLE);
		(void)schedule_timeout(HZ / 10); /* wait for 1/10 second */
		BB_DBG("path=%s\n", path);
		if (timeouts-- < 0) {
			BB_ERR("wait partiton[%s] fail. use [%d]'s . skip!\n",
			       path, timeo);
			if (!ret)
				DST_ERR("%s mode = %x\n", path, m_stat.mode);
			BB_PR_END();
			return -1;
		}
	}

	BB_PR_END();
	return 0;
}

void rdr_sys_sync(void)
{
	if (!in_atomic() && !irqs_disabled() && !in_irq())
		/* Ensure all previous file system related operations can be completed */
		ksys_sync();
}
