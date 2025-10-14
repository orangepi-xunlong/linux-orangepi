/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __RDR_AP_HOOK_H__
#define __RDR_AP_HOOK_H__

#include <linux/sched.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/soc/cix/rdr_pub.h>
#include "rdr_ap_memid.h"

#define TOTAL_RATIO 64
#define HOOK_DEFAULT_RATIO                                 \
	{                                                  \
		{ 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },   \
		{ 32, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  \
		{ 32, 16, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, \
		{ 32, 16, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0 },  \
		{ 32, 16, 8, 4, 4, 0, 0, 0, 0, 0, 0, 0 },  \
		{ 32, 12, 8, 4, 4, 4, 0, 0, 0, 0, 0, 0 },  \
		{ 32, 12, 8, 4, 4, 2, 2, 0, 0, 0, 0, 0 },  \
		{ 32, 12, 8, 4, 4, 2, 1, 1, 0, 0, 0, 0 },  \
		{ 28, 12, 8, 4, 4, 2, 2, 2, 2, 0, 0, 0 },  \
		{ 28, 12, 8, 4, 4, 2, 2, 2, 1, 1, 0, 0 },  \
		{ 24, 12, 8, 4, 4, 4, 2, 2, 2, 1, 1, 0 },  \
		{ 24, 12, 6, 4, 4, 4, 2, 2, 2, 2, 1, 1 },  \
	}
#define HK_IRQ_RATIO HOOK_DEFAULT_RATIO
#define HK_TASK_RATIO HOOK_DEFAULT_RATIO
#define HK_CPUIDLE_RATIO HOOK_DEFAULT_RATIO
#define HK_CPU_ONOFF_RATIO HOOK_DEFAULT_RATIO
#define HK_SYSCALL_RATIO HOOK_DEFAULT_RATIO
#define HK_IRQ_RATIO HOOK_DEFAULT_RATIO
#define HK_IRQ_RATIO HOOK_DEFAULT_RATIO
#define HK_HUNGTASK_RATIO HOOK_DEFAULT_RATIO
#define HK_TASKLET_RATIO HOOK_DEFAULT_RATIO
#define HK_WORKER_RATIO HOOK_DEFAULT_RATIO

#define HK_VEC_PARSER(vec) ((vec) ? "exit" : "enter")
#define HK_PARSER_TEXT(hk_id) #hk_id "--> "

#define HK_IRQ_PARSER_TEXT(info, kaslr)                            \
	"clock: %llu, hwirq: %u, vec: %s", info->clock, info->irq, \
		HK_VEC_PARSER(info->dir)

#define HK_TASK_PARSER_TEXT(info, kaslr) \
	"clock: %llu, pid: %u, comm: %s", info->clock, info->pid, info->comm

#define HK_CPUIDLE_PARSER_TEXT(info, kaslr) \
	"clock: %llu, vec: %s", info->clock, HK_VEC_PARSER(info->dir)

#define HK_WORKER_PARSER_TEXT(info, kaslr)                            \
	"clock: %llu, pid: %u, action: 0x%llx, vec: %s", info->clock, \
		info->pid, (info->action - kaslr), HK_VEC_PARSER(info->dir)

#define HK_MEM_ALLOCATOR_PARSER_TEXT(info, kaslr)

#define HK_ION_ALLOCATOR_PARSER_TEXT(info, kaslr)

#define HK_TIME_PARSER_TEXT(info, kaslr)                 \
	"clock: %llu, action: %u, vec: %s", info->clock, \
		(info->action - kaslr), HK_VEC_PARSER(info->dir)

#define HK_CPU_ONOFF_PARSER_TEXT(info, kaslr)                      \
	"clock: %llu, cpu: %u, state: %s", info->clock, info->cpu, \
		info->on ? "on" : "off"

#define HK_SYSCALL_PARSER_TEXT(info, kaslr)                                   \
	"clock: %llu, cpu: %u, syscall: %u, vec: %s", info->clock, info->cpu, \
		info->syscall, HK_VEC_PARSER(info->dir)

#define HK_HUNGTASK_PARSER_TEXT(info, kaslr)                                   \
	"clock: %llu, pid: %u, timeout: %u, comm: %s", info->clock, info->pid, \
		info->timeout, info->comm

#define HK_TASKLET_PARSER_TEXT(info, kaslr)                           \
	"clock: %llu, cpu: %u, action: 0x%llx, vec: %s", info->clock, \
		info->cpu, (info->action - kaslr), HK_VEC_PARSER(info->dir)

struct hook_buffer_info {
	bbox_mem mem;
	int cpu_num;
	bbox_mem percpu[HOOK_MAX_NUMBERS];
};

struct irq_trace_info {
	u64 clock;
	u32 irq;
	u8 dir;
};

struct task_switch_info {
	u64 clock;
	u64 stack;
	u32 pid;
	char comm[TASK_COMM_LEN];
};

struct cpuidle_stat_info {
	u64 clock;
	u8 dir;
};

struct cpu_on_off_info {
	u64 clock;
	u8 cpu;
	u8 on;
};

struct syscalls_info {
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

struct sort_info {
	u64 clock;
	u32 hk_id;
	int cpu;
	void *data;
};

int ap_hook_init(struct platform_device *pdev, struct rdr_safemem_pool *pool);
void hook_debug_info(void);
void ap_def_trace_hook_install(void);
int ap_trace_hook_install(void);
void ap_trace_hook_uninstall(void);
int ap_hook_cleartext(const char *dir_path, u64 log_addr, u32 log_len);

#endif
