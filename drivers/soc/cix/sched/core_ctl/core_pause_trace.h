/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 MediaTek Inc.
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM core_ctl

#if !defined(_CORE_CTL_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _CORE_CTL_TRACE_H

#include <linux/types.h>
#include <linux/tracepoint.h>

#if IS_ENABLED(CONFIG_CIX_CORE_PAUSE)
TRACE_EVENT(
	sched_pause_cpus,
	TP_PROTO(struct cpumask *req_cpus, struct cpumask *last_cpus,
		 u64 start_time, unsigned char pause, int err,
		 struct cpumask *pause_cpus),

	TP_ARGS(req_cpus, last_cpus, start_time, pause, err, pause_cpus),

	TP_STRUCT__entry(__field(unsigned int, req_cpus) __field(
		unsigned int, last_cpus) __field(unsigned int, time)
				 __field(unsigned char, pause) __field(int, err)
					 __field(unsigned int, pause_cpus)
						 __field(unsigned int,
							 online_cpus)
							 __field(unsigned int,
								 active_cpus)),

	TP_fast_assign(
		__entry->req_cpus = cpumask_bits(req_cpus)[0];
		__entry->last_cpus = cpumask_bits(last_cpus)[0];
		__entry->time = div64_u64(sched_clock() - start_time, 1000);
		__entry->pause = pause; __entry->err = err;
		__entry->pause_cpus = cpumask_bits(pause_cpus)[0];
		__entry->online_cpus = cpumask_bits(cpu_online_mask)[0];
		__entry->active_cpus = cpumask_bits(cpu_active_mask)[0];),

	TP_printk(
		"req=0x%x cpus=0x%x time=%u us paused=%d, err=%d, pause=0x%x, online=0x%x, active=0x%x",
		__entry->req_cpus, __entry->last_cpus, __entry->time,
		__entry->pause, __entry->err, __entry->pause_cpus,
		__entry->online_cpus, __entry->active_cpus));

TRACE_EVENT(
	sched_set_cpus_allowed,
	TP_PROTO(struct task_struct *p, unsigned int *dest_cpu,
		 struct cpumask *new_mask, struct cpumask *valid_mask,
		 struct cpumask *pause_cpus),

	TP_ARGS(p, dest_cpu, new_mask, valid_mask, pause_cpus),

	TP_STRUCT__entry(
		__field(pid_t, pid) __field(unsigned int, dest_cpu)
			__field(bool, kthread) __field(unsigned int, new_mask)
				__field(unsigned int, valid_mask)
					__field(unsigned int, pause_cpus)),

	TP_fast_assign(__entry->pid = p->pid; __entry->dest_cpu = *dest_cpu;
		       __entry->kthread = p->flags & PF_KTHREAD;
		       __entry->new_mask = cpumask_bits(new_mask)[0];
		       __entry->valid_mask = cpumask_bits(valid_mask)[0];
		       __entry->pause_cpus = cpumask_bits(pause_cpus)[0];),

	TP_printk(
		"p=%d, dest_cpu=%d, k=%d, new_mask=0x%x, valid=0x%x, pause=0x%x",
		__entry->pid, __entry->dest_cpu, __entry->kthread,
		__entry->new_mask, __entry->valid_mask, __entry->pause_cpus));

TRACE_EVENT(
	sched_find_lowest_rq,
	TP_PROTO(struct task_struct *tsk, int policy, int target_cpu,
		 struct cpumask *avail_lowest_mask, struct cpumask *lowest_mask,
		 const struct cpumask *active_mask),

	TP_ARGS(tsk, policy, target_cpu, avail_lowest_mask, lowest_mask,
		active_mask),

	TP_STRUCT__entry(__field(pid_t, pid) __field(int, policy) __field(
		int, target_cpu) __field(unsigned int, avail_lowest_mask)
				 __field(unsigned int, lowest_mask)
					 __field(unsigned int, active_mask)),

	TP_fast_assign(
		__entry->pid = tsk->pid; __entry->policy = policy;
		__entry->target_cpu = target_cpu;
		__entry->avail_lowest_mask = cpumask_bits(avail_lowest_mask)[0];
		__entry->lowest_mask = cpumask_bits(lowest_mask)[0];
		__entry->active_mask = cpumask_bits(active_mask)[0];),

	TP_printk(
		"pid=%4d policy=0x%08x target=%d avail_lowest_mask=0x%x lowest_mask=0x%x, active_mask:0x%x",
		__entry->pid, __entry->policy, __entry->target_cpu,
		__entry->avail_lowest_mask, __entry->lowest_mask,
		__entry->active_mask));
#endif

#endif /* _CORE_PAUSE_TRACE_H */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH ../../drivers/soc/cix/sched/core_ctl
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE core_pause_trace
/* This part must be outside protection */
#include <trace/define_trace.h>
