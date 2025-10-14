/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM sched
#define TRACE_INCLUDE_PATH trace/hooks
#if !defined(_TRACE_HOOK_SCHED_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_HOOK_SCHED_H

#include <trace/hooks/vendor_hooks.h>

/*
 * Following tracepoints are not exported in tracefs and provide a
 * mechanism for vendor modules to hook and extend functionality
 */
struct task_struct;

DECLARE_RESTRICTED_HOOK(android_rvh_enqueue_task_fair,
	TP_PROTO(struct rq *rq, struct task_struct *p, int flags),
	TP_ARGS(rq, p, flags), 1);

DECLARE_RESTRICTED_HOOK(android_rvh_dequeue_task_fair,
	TP_PROTO(struct rq *rq, struct task_struct *p, int flags),
	TP_ARGS(rq, p, flags), 1);

struct rq;
DECLARE_HOOK(android_vh_scheduler_tick,
	TP_PROTO(struct rq *rq),
	TP_ARGS(rq));

DECLARE_RESTRICTED_HOOK(android_rvh_is_cpu_allowed,
	TP_PROTO(struct task_struct *p, int cpu, bool *allowed),
	TP_ARGS(p, cpu, allowed), 1);

DECLARE_RESTRICTED_HOOK(android_rvh_set_cpus_allowed_by_task,
	TP_PROTO(const struct cpumask *cpu_valid_mask, const struct cpumask *new_mask,
		 struct task_struct *p, unsigned int *dest_cpu),
	TP_ARGS(cpu_valid_mask, new_mask, p, dest_cpu), 1);

DECLARE_RESTRICTED_HOOK(android_rvh_rto_next_cpu,
	TP_PROTO(int rto_cpu, struct cpumask *rto_mask, int *cpu),
	TP_ARGS(rto_cpu, rto_mask, cpu), 1);

DECLARE_RESTRICTED_HOOK(android_rvh_get_nohz_timer_target,
	TP_PROTO(int *cpu, bool *done),
	TP_ARGS(cpu, done), 1);

DECLARE_RESTRICTED_HOOK(android_rvh_can_migrate_task,
	TP_PROTO(struct task_struct *p, int dst_cpu, int *can_migrate),
	TP_ARGS(p, dst_cpu, can_migrate), 1);

DECLARE_RESTRICTED_HOOK(android_rvh_find_busiest_queue,
	TP_PROTO(int dst_cpu, struct sched_group *group,
		 struct cpumask *env_cpus, struct rq **busiest,
		 int *done),
	TP_ARGS(dst_cpu, group, env_cpus, busiest, done), 1);

DECLARE_RESTRICTED_HOOK(android_rvh_find_new_ilb,
	TP_PROTO(struct cpumask *nohz_idle_cpus_mask, int *ilb),
	TP_ARGS(nohz_idle_cpus_mask, ilb), 1);

/* macro versions of hooks are no longer required */

#endif /* _TRACE_HOOK_SCHED_H */
/* This part must be outside protection */
#include <trace/define_trace.h>
