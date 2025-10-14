// SPDX-License-Identifier: GPL-2.0-only
/*
 * rdr_hisi_ap_hook.c
 *
 * AP side track hook function code
 *
 * Copyright (c) 2001-2019 Huawei Technologies Co., Ltd.
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

#include <linux/cpu.h>
#include <linux/delay.h>
#include <linux/cpu_pm.h>
#include <linux/sort.h>
#include <linux/sched/clock.h>
#include <linux/soc/cix/rdr_platform_ap_ringbuffer.h>
#include <linux/soc/cix/rdr_platform_ap_hook.h>
#include "include/rdr_ap_adapter.h"
#include "../rdr_print.h"
#include "../rdr_field.h"

#define TRACE_INIT_NAME(name) name##_trace_init
#define TRACE_GET_CLOCK(name) name##_get_clock
#define TRACE_PARSER(name) name##_parser
#define TRACE_PROP_INIT(name) PROPERTY_INIT(ap_trace_##name##_size)

static struct hook_buffer_info g_hook_buffer[HK_MAX];
static struct property_table g_trace_prop[HK_MAX] = {
	[HK_IRQ] = TRACE_PROP_INIT(irq),
	[HK_TASK] = TRACE_PROP_INIT(task),
	[HK_CPUIDLE] = TRACE_PROP_INIT(cpu_idle),
	[HK_WORKER] = TRACE_PROP_INIT(worker),
#ifdef CONFIG_DST_MEM_TRACE
	[HK_MEM_ALLOCATOR] = TRACE_PROP_INIT(mem_alloc),
	[HK_ION_ALLOCATOR] = TRACE_PROP_INIT(ion_alloc),
#else
	[HK_MEM_ALLOCATOR] = { NULL, 0 },
	[HK_ION_ALLOCATOR] = { NULL, 0 },
#endif
	[HK_TIME] = TRACE_PROP_INIT(time),
	[HK_CPU_ONOFF] = TRACE_PROP_INIT(cpu_on_off),
	[HK_SYSCALL] = TRACE_PROP_INIT(syscall),
	[HK_HUNGTASK] = TRACE_PROP_INIT(hung_task),
	[HK_TASKLET] = TRACE_PROP_INIT(tasklet),
};
static const char *g_trace_pattern[HK_MAX] = {
	"irq_switch::ktime,slice,vec,dir", /* IRQ */
	"task_switch::ktime,stack,pid,comm", /* TASK */
	"cpuidle::ktime,dir", /* CPUIDLE */
	"worker::ktime,action,dir,cpu,resv,pid", /* WORKER */
	"mem_alloc::ktime,pid/vec,comm/irq_name,caller,operation,vaddr,paddr,size", /* MEM ALLOCATOR */
	"ion_alloc::ktime,pid/vec,comm/irq_name,operation,vaddr,paddr,size", /* ION ALLOCATOR */
	"time::ktime,action,dir", /* TIME */
	"cpu_onoff::ktime,cpu,on", /* CPU_ONOFF */
	"syscall::ktime,syscall,cpu,dir", /* SYSCALL */
	"hung_task::ktime,timeout,pid,comm", /* HUNG_TASK */
	"tasklet::ktime,action,cpu,dir", /* TASKLET */
};

/* default opened hook id for each case */
static enum hook_type g_hisi_defopen_hook_id[] = { HK_CPUIDLE, HK_CPU_ONOFF,
						   HK_TASK };
DEFINE_STATIC_KEY_ARRAY_FALSE(g_ap_hook_keys, HK_MAX);
static u32 g_hisi_last_irqs[HOOK_MAX_NUMBERS] = { 0 };
struct task_struct *g_last_task_ptr[HOOK_MAX_NUMBERS] = { NULL };

struct task_struct **get_last_task_ptr(void)
{
	return (struct task_struct **)g_last_task_ptr;
}

int percpu_buffer_init(struct hook_buffer_info *info, u32 fieldcnt,
		       enum hook_type hk, u32 ratio[][HOOK_MAX_NUMBERS],
		       bool reset_rbuf)
{
	u32 min_size;
	void *start_addr = NULL;
	int ratio_index = 0, ret;
	bbox_mem *pinfo = NULL;
	u32 pice_size = 0;

	if (IS_ERR_OR_NULL(info) || IS_ERR_OR_NULL(info->mem.vaddr)) {
		BB_ERR("hook_buffer_info is NULL\n");
		return -EINVAL;
	}

	min_size = info->cpu_num * (sizeof(struct rdr_ringbuffer) +
				    TOTAL_RATIO * (u64)(unsigned int)fieldcnt);
	if (info->mem.size < (u32)min_size) {
		memset(info->percpu, 0, sizeof(info->percpu));
		BB_PN("HOOK[%d] buffer size [0x%llx] is too small, require min size[0x%x]!\n",
		      hk, info->mem.size, min_size);
		return 0;
	}

	BB_DBG("buffer_addr [0x%px], buffer_size [0x%llx]\n", info->mem.vaddr,
	       info->mem.size);

	start_addr = info->mem.vaddr;
	ratio_index = info->cpu_num - 1;
	pinfo = info->percpu;
	pice_size = info->mem.size / TOTAL_RATIO;

	for (int i = 0; i < info->cpu_num; i++) {
		BB_DBG("ratio[%u][%u] = [%u]\n", ratio_index, i,
		       ratio[ratio_index][i]);
		pinfo[i].vaddr = start_addr;
		pinfo[i].size = pice_size * ratio[ratio_index][i];
		start_addr += pinfo[i].size;

		BB_DBG("cpu[%u]: ratio[%u][%u]=%u%%, addr [0x%px], size [0x%llx]\n",
		       i, ratio_index, i,
		       (ratio[ratio_index][i] * 100) / TOTAL_RATIO,
		       pinfo[i].vaddr, pinfo[i].size);
		if (!reset_rbuf)
			continue;
		ret = rdr_rbuf_init(pinfo[i].vaddr, pinfo[i].size, fieldcnt,
				    g_trace_pattern[hk]);
		if (ret) {
			BB_ERR("cpu [%u] ringbuffer init failed!\n", i);
			return ret;
		}
	}
	return 0;
}

#undef DECLARE_AP_HOOK
#define DECLARE_AP_HOOK(name, proto, args, id)                              \
	static int TRACE_INIT_NAME(name)(struct hook_buffer_info *info,    \
					 bool reset_rbuf)                   \
	{                                                                   \
		unsigned int ratio[HOOK_MAX_NUMBERS][HOOK_MAX_NUMBERS] =    \
			id##_RATIO;                                         \
		return percpu_buffer_init(info, sizeof(struct name##_info), \
					  id, ratio, reset_rbuf);           \
	}

AP_HOOK_LIST

/*
 * Description: Interrupt track record
 * Input:       dir: 0 interrupt entry, 1 interrupt exit, new_vec: current interrupt
 */
void __irq_trace_hook(unsigned int dir, unsigned int old_vec,
		      unsigned int new_vec)
{
	/* Record time stamp, cpu_id, interrupt number, interrupt in and out direction */
	struct irq_trace_info info;
	u8 cpu;

	info.clock = local_clock();

	cpu = (u8)smp_processor_id();
	info.dir = (u8)dir;
	info.irq = (u32)new_vec;
	g_hisi_last_irqs[cpu] = new_vec;

	rdr_rbuf_write(g_hook_buffer[HK_IRQ].percpu[cpu].vaddr, (u8 *)&info);
}

/*
 * Description: Record kernel task traces
 * Input:       Pre_task: current task structure pointer
 *				next_task: next task structure pointer
 * Other:       added to kernel/sched/core.c
 */
void __task_switch_hook(const void *pre_task, void *next_task)
{
	/* Record the timestamp, cpu_id, next_task, task name, and the loop buffer corresponding to the cpu */
	struct task_struct *task = next_task;
	struct task_switch_info info;
	u8 cpu;

	if (!pre_task || !next_task) {
		BB_ERR("error:pre_task or next_task is NULL\n");
		return;
	}

	info.clock = local_clock();
	cpu = (u8)smp_processor_id();
	info.pid = (u32)task->pid;
	(void)strncpy(info.comm, task->comm, sizeof(task->comm) - 1);
	info.comm[TASK_COMM_LEN - 1] = '\0';
	info.stack = (uintptr_t)task->stack;

	g_last_task_ptr[cpu] = task;
	rdr_rbuf_write(g_hook_buffer[HK_TASK].percpu[cpu].vaddr, (u8 *)&info);
	stack_last_task_update(cpu, task);
}

/*
 * Description: Record the cpu into the idle power off, exit the idle power-on track
 * Input:       dir: 0 enters idle or 1 exits idle
 */
void __cpuidle_stat_hook(u32 dir)
{
	/* Record timestamp, cpu_id, enter idle or exit idle to the corresponding loop buffer */
	struct cpuidle_stat_info info;
	u8 cpu;

	info.clock = local_clock();
	cpu = (u8)smp_processor_id();
	info.dir = (u8)dir;

	rdr_rbuf_write(g_hook_buffer[HK_CPUIDLE].percpu[cpu].vaddr,
		       (u8 *)&info);
}

/*
 * Description: The CPU inserts and deletes the core record, which is consistent with the scenario of the SR process
 * Input:       cpu:cpu number, on:1 plus core, 0 minus core
 * Other:       added to drivers/cpufreq/cpufreq.c
 */
void __cpu_on_off_hook(u32 cpu, u32 on)
{
	/* Record the time stamp, cpu_id, cpu is on or off, to the corresponding loop buffer */
	struct cpu_on_off_info info;

	info.clock = local_clock();
	info.cpu = (u8)cpu;
	info.on = (u8)on;

	rdr_rbuf_write(g_hook_buffer[HK_CPU_ONOFF].percpu[0].vaddr,
		       (u8 *)&info);
}

/*
 * Description: Record system call trace
 * Input:       syscall_num: system call number, dir: call in and out direction, 0: enter, 1: exit
 * Other:       added to arch/arm64/kernel/entry.S
 */
void __syscalls_hook(u32 syscall_num, u32 dir)
{
	/* Record the time stamp, system call number, to the corresponding loop buffer */
	struct syscalls_info info;

	info.clock = local_clock();
	info.syscall = (u32)syscall_num;
	preempt_disable();
	info.cpu = (u8)smp_processor_id();
	preempt_enable_no_resched();
	info.dir = (u8)dir;

	rdr_rbuf_write(g_hook_buffer[HK_SYSCALL].percpu[0].vaddr, (u8 *)&info);
}

/*
 * Description: Record the task information of the hung task
 * Input:       tsk: task struct body pointer, timeout: timeout time
 */
void __hung_task_hook(void *tsk, u32 timeout)
{
	/* Record time stamp, task pid, timeout time, to the corresponding loop buffer */
	struct task_struct *task = tsk;
	struct hung_task_info info;

	if (!tsk) {
		BB_ERR("error:tsk is NULL\n");
		return;
	}

	info.clock = local_clock();
	info.timeout = (u32)timeout;
	info.pid = (u32)task->pid;
	(void)strncpy(info.comm, task->comm, sizeof(task->comm) - 1);
	info.comm[TASK_COMM_LEN - 1] = '\0';

	rdr_rbuf_write(g_hook_buffer[HK_HUNGTASK].percpu[0].vaddr, (u8 *)&info);
}

/*
 * Description: Record tasklet execution track
 * Input:       address:For the tasklet to execute the function address,
 *              dir:    call the inbound and outbound direction, 0: enter, 1: exit
 * Other:       added to arch/arm64/kernel/entry.S
 */
void __tasklet_hook(u64 address, u32 dir)
{
	/* Record the timestamp, function address, CPU number, to the corresponding loop buffer */
	struct tasklet_info info;

	info.clock = local_clock();
	info.action = (u64)address;
	info.cpu = (u8)smp_processor_id();
	info.dir = (u8)dir;

	rdr_rbuf_write(g_hook_buffer[HK_TASKLET].percpu[0].vaddr, (u8 *)&info);
}

/*
 * Description: Record worker execution track
 * Input:       address:for the worker to execute the function address,
 *              dir:    call the inbound and outbound direction, 0: enter, 1: exit
 * Other:       added to arch/arm64/kernel/entry.S
 */
asmlinkage void __worker_hook(u64 address, u32 dir)
{
	/* Record the timestamp, function address, CPU number, to the corresponding loop buffer */
	struct worker_info info;
	u8 cpu;

	info.clock = local_clock();
	info.action = (u64)address;
	info.pid = (u32)(current->pid);
	info.dir = (u8)dir;

	preempt_disable();
	cpu = (u8)smp_processor_id();
	rdr_rbuf_write(g_hook_buffer[HK_WORKER].percpu[cpu].vaddr, (u8 *)&info);
	preempt_enable();
}

#undef DECLARE_AP_HOOK
#define DECLARE_AP_HOOK(name, proto, args, id)                      \
	static u64 TRACE_GET_CLOCK(name)(struct name##_info *info) \
	{                                                           \
		return info->clock;                                 \
	}
AP_HOOK_LIST

#undef DECLARE_AP_HOOK
#define DECLARE_AP_HOOK(name, proto, args, id) \
	case id:                               \
		return TRACE_GET_CLOCK(name)(info);
u64 hook_get_clock(void *info, u32 hk_id)
{
	switch (hk_id) {
		AP_HOOK_LIST
	default:
		break;
	}
	return 0;
}

static bool hook_should_parser(u32 hk_id)
{
	if (hk_id == HK_ION_ALLOCATOR || hk_id == HK_MEM_ALLOCATOR)
		return false;
	return true;
}

#undef DECLARE_AP_HOOK
#define DECLARE_AP_HOOK(name, proto, args, id)                         \
	case id:                                                       \
		rdr_cleartext_print(fp, &error, HK_PARSER_TEXT(id));   \
		rdr_cleartext_print(                                   \
			fp, &error,                                    \
			id##_PARSER_TEXT(((struct name##_info *)data), \
					 kaslr));                      \
		rdr_cleartext_print(fp, &error, "\n");                 \
		break;

void hook_info_parser(struct file *fp, struct sort_info *info)
{
	bool error;
	void *data = info->data;
	u32 hk_id = info->hk_id;
	u64 kaslr = kaslr_offset();

	if (!hook_should_parser(hk_id))
		return;

	if (IS_ERR_OR_NULL(info->data)) {
		BB_PN("hk=%d, addr=%px\n", info->hk_id, info->data);
		return;
	}

	if (hk_id < HK_PERCPU_TAG)
		rdr_cleartext_print(fp, &error, "cpu[%d]	", info->cpu);

	switch (hk_id) {
		AP_HOOK_LIST
	default:
		break;
	}
}

static int hook_sort_handle(const void *va, const void *vb)
{
	const struct sort_info *a = va;
	const struct sort_info *b = vb;

	if (a->clock < b->clock)
		return -1;
	if (a->clock > b->clock)
		return 1;

	return 0;
}

static struct hook_buffer_info *
ap_hook_last_buf_alloc(struct rdr_safemem_pool *pool)
{
	int ret;
	struct hook_buffer_info *info = NULL;

	info = kzalloc(HK_MAX * sizeof(*info), GFP_KERNEL);
	if (IS_ERR_OR_NULL(info))
		return NULL;

	for (int i = 0; i < HK_MAX; i++) {
		if (rdr_safemem_get(pool, MEMID_HOOK + i, &info[i].mem))
			continue;
		if (info[i].mem.size == 0)
			continue;
		if (i < HK_PERCPU_TAG)
			info[i].cpu_num = HOOK_MAX_NUMBERS;
		else
			info[i].cpu_num = 1;
	}
#undef DECLARE_AP_HOOK
#define DECLARE_AP_HOOK(name, proto, args, id)                 \
	do {                                                   \
		ret = TRACE_INIT_NAME(name)(&info[id], false); \
		if (ret) {                                     \
			BB_ERR("hook[%d] init failed!\n", id); \
			kfree(info);                           \
			return NULL;                           \
		}                                              \
	} while (0);

	AP_HOOK_LIST;
	return info;
}

int ap_hook_cleartext(const char *dir_path, u64 log_addr, u32 log_len)
{
	u32 total_num = 0, index = 0, start, end;
	struct sort_info *hk_sort = NULL;
	struct ap_eh_root *head = (struct ap_eh_root *)log_addr;
	struct hook_buffer_info *hinfo = NULL;
	struct rdr_ringbuffer *rb = NULL;
	void *tmp;
	bool islast = rdr_log_save_is_last();
	u64 clock = 0;
	struct file *fp;

	if (IS_ERR_OR_NULL(head))
		return -EINVAL;
	if (islast)
		hinfo = ap_hook_last_buf_alloc(&head->pool);
	else
		hinfo = g_hook_buffer;

	if (IS_ERR_OR_NULL(hinfo))
		return -ENOMEM;
	/*Request sufficient memory.*/
	for (int i = 0; i < HK_MAX; i++) {
		if (hinfo[i].mem.size == 0)
			continue;
		for (int j = 0; j < hinfo[i].cpu_num; j++) {
			if (IS_ERR_OR_NULL(hinfo[i].percpu[j].vaddr))
				continue;
			rb = get_addr_from_root(head, hinfo[i].percpu[j].vaddr);
			if (IS_ERR_OR_NULL(rb))
				continue;
			BB_DBG("[%s]: cpu%d, size %d\n", rb->keys, j,
			       rb->max_num);
			total_num += rdr_rbuf_get_maxnum(rb);
		}
	}
	BB_DBG("total num %d\n", total_num);
	hk_sort = kcalloc(total_num, sizeof(*hk_sort), GFP_KERNEL);
	if (IS_ERR_OR_NULL(hk_sort)) {
		if (islast)
			kfree(hinfo);
		return -ENOMEM;
	}

	/*Collect all trace information.*/
	for (int i = 0; i < HK_MAX; i++) {
		if (hinfo[i].mem.size == 0)
			continue;
		for (int j = 0; j < hinfo[i].cpu_num; j++) {
			if (IS_ERR_OR_NULL(hinfo[i].percpu[j].vaddr))
				continue;
			rb = get_addr_from_root(head, hinfo[i].percpu[j].vaddr);
			if (IS_ERR_OR_NULL(rb))
				continue;
			if (rdr_rbuf_is_empty(rb)) {
				BB_DBG("[%s] cpu[%d] is empty\n", rb->keys, j);
				continue;
			}
			rdr_rbuf_get_start_end(rb, &start, &end);
			for (u32 k = start; k <= end; k++) {
				tmp = rdr_rbuf_get_data(rb, k);
				clock = hook_get_clock(tmp, i);
				if (!clock)
					continue;
				hk_sort[index].clock = clock;
				hk_sort[index].hk_id = i;
				hk_sort[index].data = tmp;
				hk_sort[index].cpu = j;
				index++;
			}
		}
	}

	/*sort trace info*/
	sort(hk_sort, index, sizeof(*hk_sort), hook_sort_handle, NULL);

	/*parser trace info*/
	fp = bbox_cleartext_get_filep(dir_path, "trace_hook.txt");
	if (IS_ERR_OR_NULL(fp)) {
		if (islast)
			kfree(hinfo);
		kfree(hk_sort);
		return -EIO;
	}

	for (int i = 0; i < index; i++) {
		hook_info_parser(fp, &hk_sort[i]);
	}

	bbox_cleartext_end_filep(fp);

	if (islast)
		kfree(hinfo);
	kfree(hk_sort);

	return 0;
}

/*
 * Description: default oepned hook install
 */
void ap_def_trace_hook_install(void)
{
	enum hook_type hk;
	u32 i, size;

	size = ARRAY_SIZE(g_hisi_defopen_hook_id);
	for (i = 0; i < size; i++) {
		hk = g_hisi_defopen_hook_id[i];
		if (g_hook_buffer[hk].mem.vaddr) {
			static_branch_enable(&g_ap_hook_keys[hk]);
			BB_DBG("hook [%d] is installed!\n", hk);
		}
	}
}

/*
 * Description: Install hooks
 * Input:       hk: hook type
 * Return:      0: The installation was successful, <0: The installation failed
 */
int ap_hook_install(enum hook_type hk)
{
	if (hk >= HK_MAX) {
		BB_ERR("hook type [%d] is invalid!\n", hk);
		return -EINVAL;
	}

	if (g_hook_buffer[hk].mem.vaddr) {
		static_branch_enable(&g_ap_hook_keys[hk]);
		BB_DBG("hook [%d] is installed!\n", hk);
	}

	return 0;
}

/*
 * Description: Uninstall the hook
 * Input:       hk: hook type
 * Return:      0: Uninstall succeeded, <0: Uninstall failed
 */
int ap_hook_uninstall(enum hook_type hk)
{
	if (hk >= HK_MAX) {
		BB_ERR("hook type [%d] is invalid!\n", hk);
		return -EINVAL;
	}

	static_branch_disable(&g_ap_hook_keys[hk]);
	BB_DBG("hook [%d] is uninstalled!\n", hk);

	return 0;
}

static int cpuidle_notifier(struct notifier_block *self, unsigned long cmd,
			    void *v)
{
	if (!self) {
		BB_ERR("self is NULL\n");
		return NOTIFY_BAD;
	}

	switch (cmd) {
	case CPU_PM_ENTER:
		cpuidle_stat_hook(0);
		break;
	case CPU_PM_EXIT:
		cpuidle_stat_hook(1);
		break;
	default:
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}

static struct notifier_block cpuidle_notifier_block = {
	.notifier_call = cpuidle_notifier,
};

static int cpu_online_hook(unsigned int cpu)
{
	cpu_on_off_hook(cpu, 1);
	return 0;
}

static int cpu_offline_hook(unsigned int cpu)
{
	cpu_on_off_hook(cpu, 0);
	return 0;
}

void hook_debug_info(void)
{
	struct bbox_mem *percpu = NULL;
	struct hook_buffer_info *info = g_hook_buffer;

	for (int i = 0; i < HK_MAX; i++) {
		BB_DBG("hook[%d] addr = %px, size = %lld,cpunum = %d\n", i,
		       info[i].mem.vaddr, info[i].mem.size, info[i].cpu_num);
		for (int j = 0; j < info[i].cpu_num; j++) {
			percpu = info[i].percpu;
			BB_DBG("cpu[%d] addr = %px, size = %lld\n", j,
			       percpu[j].vaddr, percpu[j].size);
		}
	}
}

int ap_trace_hook_install(void)
{
	int ret = 0;
	enum hook_type hk;

	for (hk = HK_IRQ; hk < HK_MAX; hk++) {
		ret = ap_hook_install(hk);
		if (ret) {
			BB_ERR("hook_type [%u] install failed!\n", hk);
			return ret;
		}
	}
	return ret;
}

void ap_trace_hook_uninstall(void)
{
	enum hook_type hk;

	for (hk = HK_IRQ; hk < HK_MAX; hk++)
		ap_hook_uninstall(hk);
}

static int hook_buffer_init(struct platform_device *pdev,
			    struct rdr_safemem_pool *pool)
{
	int ret;
	struct hook_buffer_info *info = g_hook_buffer;

	ret = ap_prop_table_init(&pdev->dev, g_trace_prop,
				 ARRAY_SIZE(g_trace_prop));
	if (ret) {
		BB_ERR("get hook prop failed!\n");
		return ret;
	}

	for (int i = 0; i < HK_MAX; i++) {
		if (g_trace_prop[i].size == 0)
			continue;
		if (rdr_safemem_alloc(pool, MEMID_HOOK + i,
				      g_trace_prop[i].size, &info[i].mem))
			return -1;

		if (i < HK_PERCPU_TAG)
			info[i].cpu_num = HOOK_MAX_NUMBERS;
		else
			info[i].cpu_num = 1;
		BB_DBG("hook[%d] size %lld, cpu_num %d, addr = %px\n", i,
		       info[i].mem.size, info[i].cpu_num, info[i].mem.vaddr);
	}

#undef DECLARE_AP_HOOK
#define DECLARE_AP_HOOK(name, proto, args, id)                 \
	do {                                                   \
		ret = TRACE_INIT_NAME(name)(&info[id], true);  \
		if (ret) {                                     \
			BB_ERR("hook[%d] init failed!\n", id); \
			return ret;                            \
		}                                              \
	} while (0);

	AP_HOOK_LIST;

	return ret;
}

int ap_hook_init(struct platform_device *pdev, struct rdr_safemem_pool *pool)
{
	int ret = 0;

	ret = hook_buffer_init(pdev, pool);
	if (ret) {
		BB_ERR("hook buffer init failed!\n");
		return ret;
	}

	cpu_pm_register_notifier(&cpuidle_notifier_block);
	/*need add*/
	ret = cpuhp_setup_state_nocalls(CPUHP_AP_ONLINE_DYN, "cpuonoff:online",
					cpu_online_hook, cpu_offline_hook);
	if (ret < 0)
		BB_ERR("cpu_on_off cpuhp_setup_state_nocalls failed\n");
	else
		ret = 0;

	/* Whether the track is generated is consistent with the kernel dump */
	if (check_himntn(HIMNTN_KERNEL_DUMP_ENABLE)) {
		BB_DBG("ap_trace_hook_install start\n");
		ret = ap_trace_hook_install();
		if (ret) {
			BB_ERR("ap_trace_hook_install failed!\n");
			return ret;
		}
	} else {
		BB_DBG("ap_def_trace_hook_install start\n");
		ap_def_trace_hook_install();
	}

	return ret;
}
