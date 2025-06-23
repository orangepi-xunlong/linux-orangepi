/*
 * rdr_hisi_ap_hook.h
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
#include <linux/version.h>

#if (KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE)
#include <linux/hisi/hisi_ion.h>
#endif

#if (KERNEL_VERSION(4, 9, 76) <= LINUX_VERSION_CODE)
#include <linux/sched.h>
#endif

#define MEM_ALLOC 1
#define MEM_FREE 0

#define PLAT_AP_HOOK_CPU_NUMBERS 12
#define PERCPU_TOTAL_RATIO 64

/* Need to print more 2 workers before begin worker, and 1 worker includes 2 numbers */
#define AHEAD_BEGIN_WORKER 4

typedef u64 (*arch_timer_func_ptr)(void);

extern arch_timer_func_ptr g_arch_timer_func_ptr;

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

struct hook_irq_info {
	u64 clock;
	u64 jiff;
	u32 irq;
	u8 dir;
};

struct task_info {
	u64 clock;
	u64 stack;
	u32 pid;
	char comm[TASK_COMM_LEN];
};

struct cpuidle_info {
	u64 clock;
	u8 dir;
};

struct cpu_onoff_info {
	u64 clock;
	u8 cpu;
	u8 on;
};

struct syscall_info_s {
	u64 clock;
	u32 syscall;
	u8 cpu;
	u8 dir;
};

struct hung_task_info {
	u64 clock;
	u32 timeout;
	u32 pid;
	char comm[TASK_COMM_LEN];
};

struct tasklet_info {
	u64 clock;
	u64 action;
	u8 cpu;
	u8 dir;
};

struct worker_info {
	u64 clock;
	u64 action;
	u8 dir;
	u8 cpu;
	u16 resv;
	u32 pid;
};

struct mem_allocator_info {
	u64 clock;
	u32 pid;
	char comm[TASK_COMM_LEN];
	u64 caller;
	u8 operation;
	u64 va_addr;
	u64 phy_addr;
	u64 size;
};

struct ion_allocator_info {
	u64 clock;
	u32 pid;
	char comm[TASK_COMM_LEN];
	u8 operation;
	u64 va_addr;
	u64 phy_addr;
	u32 size;
};

struct time_info {
	u64 clock;
	u64 action;
	u8 dir;
};

struct percpu_buffer_info {
	unsigned char *buffer_addr;
	unsigned char *percpu_addr[PLAT_AP_HOOK_CPU_NUMBERS];
	unsigned int percpu_length[PLAT_AP_HOOK_CPU_NUMBERS];
	unsigned int buffer_size;
};

int register_arch_timer_func_ptr(arch_timer_func_ptr func);
int irq_buffer_init(struct percpu_buffer_info *buffer_info, unsigned char *addr, unsigned int size);
int task_buffer_init(struct percpu_buffer_info *buffer_info, unsigned char *addr, unsigned int size);
int cpuidle_buffer_init(struct percpu_buffer_info *buffer_info, unsigned char *addr, unsigned int size);
int cpu_onoff_buffer_init(unsigned char **addr, unsigned int size);
int syscall_buffer_init(unsigned char **addr, unsigned int size);
int hung_task_buffer_init(unsigned char **addr, unsigned int size);
int worker_buffer_init(struct percpu_buffer_info *buffer_info, unsigned char *addr, unsigned int size);
int tasklet_buffer_init(unsigned char **addr, unsigned int size);
int diaginfo_buffer_init(unsigned char **addr, unsigned int size);
int mem_alloc_buffer_init(struct percpu_buffer_info *buffer_info, unsigned char *addr, unsigned int size);
int ion_alloc_buffer_init(struct percpu_buffer_info *buffer_info, unsigned char *addr, unsigned int size);
int percpu_buffer_init(struct percpu_buffer_info *buffer_info, u32 ratio[][PLAT_AP_HOOK_CPU_NUMBERS],
				u32 cpu_num, u32 fieldcnt, const char *keys, u32 gap);
int time_buffer_init(struct percpu_buffer_info *buffer_info, unsigned char *addr,
				unsigned int size);

void worker_recent_search(u8 cpu, u64 basetime, u64 delta_time);

#ifdef CONFIG_PLAT_BBOX
void irq_trace_hook(unsigned int dir, unsigned int old_vec, unsigned int new_vec);
void task_switch_hook(const void *pre_task, void *next_task);
void cpuidle_stat_hook(u32 dir);
void cpu_on_off_hook(u32 cpu, u32 on);
void syscall_hook(u32 syscall_num, u32 dir);
void hung_task_hook(void *tsk, u32 timeout);
void tasklet_hook(u64 address, u32 dir);
void worker_hook(u64 address, u32 dir);
#ifdef CONFIG_HISI_MEM_TRACE
void page_trace_hook(gfp_t gfp_flag, u8 action, u64 caller, struct page *page, u32 order);
void kmalloc_trace_hook(u8 action, u64 caller, u64 va_addr, u64 phy_addr, u32 size);
void vmalloc_trace_hook(u8 action, u64 caller, u64 va_addr, struct page *page, u64 size);
#if (KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE)
void ion_trace_hook(u8 action, struct ion_client *client, struct ion_handle *handle);
#endif
void smmu_trace_hook(u8 action, u64 va_addr, u64 phy_addr, u32 size);
#else
static inline void page_trace_hook(gfp_t gfp_flag, u8 action, u64 caller, struct page *page, u32 order) {}
static inline void kmalloc_trace_hook(u8 action, u64 caller, u64 va_addr, u64 phy_addr, u32 size) {}
static inline void vmalloc_trace_hook(u8 action, u64 caller, u64 va_addr, struct page *page, u64 size) {}
#if (KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE)
static inline void ion_trace_hook(u8 action, struct ion_client *client, struct ion_handle *handle) {}
#endif
static inline void smmu_trace_hook(u8 action, u64 va_addr, u64 phy_addr, u32 size) {}
#endif
#else
static inline void irq_trace_hook(unsigned int dir, unsigned int old_vec, unsigned int new_vec) {}
static inline void task_switch_hook(const void *pre_task, void *next_task) {}
static inline void cpuidle_stat_hook(u32 dir) {}
static inline void cpu_on_off_hook(u32 cpu, u32 on) {}
static inline void syscall_hook(u32 syscall_num, u32 dir) {}
static inline void hung_task_hook(void *tsk, u32 timeout) {}
static inline u32 get_current_last_irq(unsigned int cpu) {}
static inline void tasklet_hook(u64 address, u32 dir) {}
static inline void worker_hook(u64 address, u32 dir) {}
static inline void page_trace_hook(gfp_t gfp_flag, u8 action, u64 caller, struct page *page, u32 order) {}
static inline void kmalloc_trace_hook(u8 action, u64 caller, u64 va_addr, u64 phy_addr, u32 size) {}
static inline void vmalloc_trace_hook(u8 action, u64 caller, u64 va_addr, struct page *page, u64 size) {}
#if (KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE)
static inline void ion_trace_hook(u8 action, struct ion_client *client, struct ion_handle *handle) {}
#endif
static inline void smmu_trace_hook(u8 action, u64 va_addr, u64 phy_addr, u32 size) {}
#endif

#ifdef CONFIG_HISI_TIME_HOOK
void time_hook(u64 address, u32 dir);
#else
static inline void time_hook(u64 address, u32 dir) {}
#endif

void hisi_ap_defopen_hook_install(void);
int hisi_ap_hook_install(enum hook_type hk);
int hisi_ap_hook_uninstall(enum hook_type hk);

#endif
