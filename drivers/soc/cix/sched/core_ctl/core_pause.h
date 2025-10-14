/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _CORE_PAUSE_H
#define _CORE_PAUSE_H
#include <linux/ioctl.h>

extern struct cpumask __cpu_pause_mask;
#define cpu_pause_mask ((struct cpumask *)&__cpu_pause_mask)

#if IS_ENABLED(CONFIG_CIX_CORE_PAUSE)
#define cpu_paused(cpu) cpumask_test_cpu((cpu), cpu_pause_mask)

extern void sched_pause_init(void);
#else
#define cpu_paused(cpu) 0
#endif

#endif /* _CORE_PAUSE_H */
