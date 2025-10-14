// SPDX-License-Identifier: GPL-2.0-only
/* vendor_hook.c
 *
 * Copyright 2022 Google LLC
 */
#include <linux/sched/cputime.h>
#include "sched.h"
#include "pelt.h"
#include "smp.h"

#define CREATE_TRACE_POINTS
#include <trace/hooks/vendor_hooks.h>
#include <linux/tracepoint.h>
#include <trace/hooks/sched.h>

EXPORT_TRACEPOINT_SYMBOL_GPL(android_vh_scheduler_tick);
EXPORT_TRACEPOINT_SYMBOL_GPL(android_rvh_enqueue_task_fair);
EXPORT_TRACEPOINT_SYMBOL_GPL(android_rvh_dequeue_task_fair);