// SPDX-License-Identifier: GPL-2.0+
#ifndef MPAM_POLICY_H
#define MPAM_POLICY_H

#include <linux/sched.h>

extern void mpam_sync_task(struct task_struct *task);
extern void mpam_hook_fork(struct task_struct *task);

#endif