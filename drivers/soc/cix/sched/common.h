/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#ifndef _SCHED_COMMON_H
#define _SCHED_COMMON_H

#define MAX_CLUSTER_NR 3
#define L_CLUSTER_ID 0
#define BL_CLUSTER_ID 1
#define AB_CLUSTER_ID 2
#define MAX_CPUS_PER_CLUSTER 4
#define MAX_NR_DOWN_THRESHOLD 4
#define MAX_BTASK_THRESH 100
#define MAX_CPU_TJ_DEGREE 100000
#define BIG_TASK_AVG_THRESHOLD 25

#define OVER_THRES_SIZE (MAX_CLUSTER_NR - 1)
#define MAX_UTIL_TRACKER_PERIODIC_MS 8

extern bool core_ctl_debug_enable;
#define core_ctl_debug(x...)               \
	do {                               \
		if (core_ctl_debug_enable) \
			trace_printk(x);   \
	} while (0)

#endif /* _SCHED_COMMON_H */
