/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * rdr_ap_hook.h
 *
 * AP side track hook header file.
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef __RDR_PLATFORM_AP_HOOK_H__
#define __RDR_PLATFORM_AP_HOOK_H__

#include <linux/thread_info.h>
#include <linux/soc/cix/rdr_types.h>
#include <linux/soc/cix/rdr_pub.h>
#include <mntn_public_interface.h>
#include <linux/version.h>
#include <linux/tracepoint.h>
#include <linux/sched.h>

#undef DECLARE_AP_HOOK
#define DECLARE_AP_HOOK(name, proto, args, id)                    \
	extern void __##name##_hook(PARAMS(proto));               \
	static inline void name##_hook(PARAMS(proto))             \
	{                                                         \
		if (!static_branch_unlikely(&g_ap_hook_keys[id])) \
			return;                                   \
		__##name##_hook(PARAMS(args));                    \
	}

#define AP_HOOK_LIST                                                           \
	MNTN_DEF_ARGS(                                                         \
		DECLARE_AP_HOOK(irq_trace,                                     \
				TP_PROTO(unsigned int dir,                     \
					 unsigned int old_vec,                 \
					 unsigned int new_vec),                \
				TP_ARGS(dir, old_vec, new_vec), HK_IRQ);       \
		DECLARE_AP_HOOK(task_switch,                                   \
				TP_PROTO(const void *pre_task,                 \
					 void *next_task),                     \
				TP_ARGS(pre_task, next_task), HK_TASK);        \
		DECLARE_AP_HOOK(cpuidle_stat, TP_PROTO(u32 dir), TP_ARGS(dir), \
				HK_CPUIDLE);                                   \
		DECLARE_AP_HOOK(cpu_on_off, TP_PROTO(u32 cpu, u32 on),         \
				TP_ARGS(cpu, on), HK_CPU_ONOFF);               \
		DECLARE_AP_HOOK(syscalls, TP_PROTO(u32 syscall_num, u32 dir),  \
				TP_ARGS(syscall_num, dir), HK_SYSCALL);        \
		DECLARE_AP_HOOK(hung_task, TP_PROTO(void *tsk, u32 timeout),   \
				TP_ARGS(tsk, timeout), HK_HUNGTASK);           \
		DECLARE_AP_HOOK(tasklet, TP_PROTO(u64 address, u32 dir),       \
				TP_ARGS(address, dir), HK_TASKLET);            \
		DECLARE_AP_HOOK(worker, TP_PROTO(u64 address, u32 dir),        \
				TP_ARGS(address, dir), HK_WORKER);)

#define MEM_ALLOC 1
#define MEM_FREE 0

enum hook_type {
	HK_IRQ = 0,
	HK_TASK,
	HK_CPUIDLE,
	HK_WORKER,
	HK_MEM_ALLOCATOR,
	HK_ION_ALLOCATOR,
	HK_TIME,
	HK_PERCPU_TAG, /* The track of percpu is above HK_PERCPU_TAG */
	HK_CPU_ONOFF = HK_PERCPU_TAG,
	HK_SYSCALL,
	HK_HUNGTASK,
	HK_TASKLET,
	HK_MAX
};

#ifdef CONFIG_PLAT_BBOX
DECLARE_STATIC_KEY_FALSE(g_ap_hook_keys[HK_MAX]);
AP_HOOK_LIST
#ifdef CONFIG_CIX_MEM_TRACE
void page_trace_hook(gfp_t gfp_flag, u8 action, u64 caller, struct page *page,
		     u32 order);
void kmalloc_trace_hook(u8 action, u64 caller, u64 va_addr, u64 phy_addr,
			u32 size);
void vmalloc_trace_hook(u8 action, u64 caller, u64 va_addr, struct page *page,
			u64 size);
void smmu_trace_hook(u8 action, u64 va_addr, u64 phy_addr, u32 size);
#else
static inline void page_trace_hook(gfp_t gfp_flag, u8 action, u64 caller,
				   struct page *page, u32 order)
{
}
static inline void kmalloc_trace_hook(u8 action, u64 caller, u64 va_addr,
				      u64 phy_addr, u32 size)
{
}
static inline void vmalloc_trace_hook(u8 action, u64 caller, u64 va_addr,
				      struct page *page, u64 size)
{
}
static inline void smmu_trace_hook(u8 action, u64 va_addr, u64 phy_addr,
				   u32 size)
{
}
#endif
#else
static inline void irq_trace_hook(unsigned int dir, unsigned int old_vec,
				  unsigned int new_vec)
{
}
static inline void task_switch_hook(const void *pre_task, void *next_task)
{
}
static inline void cpuidle_stat_hook(u32 dir)
{
}
static inline void cpu_on_off_hook(u32 cpu, u32 on)
{
}
static inline void syscalls_hook(u32 syscall_num, u32 dir)
{
}
static inline void hung_task_hook(void *tsk, u32 timeout)
{
}
static inline u32 get_current_last_irq(unsigned int cpu)
{
}
static inline void tasklet_hook(u64 address, u32 dir)
{
}
static inline void worker_hook(u64 address, u32 dir)
{
}
static inline void page_trace_hook(gfp_t gfp_flag, u8 action, u64 caller,
				   struct page *page, u32 order)
{
}
static inline void kmalloc_trace_hook(u8 action, u64 caller, u64 va_addr,
				      u64 phy_addr, u32 size)
{
}
static inline void vmalloc_trace_hook(u8 action, u64 caller, u64 va_addr,
				      struct page *page, u64 size)
{
}
static inline void smmu_trace_hook(u8 action, u64 va_addr, u64 phy_addr,
				   u32 size)
{
}
#endif

#ifdef CONFIG_CIX_TIME_HOOK
void time_hook(u64 address, u32 dir);
#else
static inline void time_hook(u64 address, u32 dir)
{
}
#endif

#endif
