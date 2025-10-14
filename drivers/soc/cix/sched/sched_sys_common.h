/* SPDX-License-Identifier: GPL-2.0 */
/*
 *  * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef SCHED_SYS_COMMON_H
#define SCHED_SYS_COMMON_H
#include <linux/module.h>

extern int init_sched_common_sysfs(void);
extern void cleanup_sched_common_sysfs(void);

extern struct kobj_attribute sched_core_pause_info_attr;
extern int sched_pause_cpu(int cpu);
extern int sched_resume_cpu(int cpu);
extern int resume_cpus(struct cpumask *cpus);
#endif
