// SPDX-License-Identifier: GPL-2.0-only
// Copyright 2025 Cix Technology Group Co., Ltd.

#include <asm/debug-monitors.h>
#include <linux/sched/debug.h>
#include <asm/hw_breakpoint.h>
#include <linux/hw_breakpoint.h>
#include <linux/kallsyms.h>
#include <linux/version.h>
#include <asm/stacktrace.h>
#include <asm/system_misc.h>
#include "../dst_print.h"

#define HW_GET_BP_ATTR(bp) (&bp->attr.bp_attr)
#define HW_BP_LIST_MAX 20

#define for_each_hw_bp(i) for (i = 0; i < g_hw_manage.max_bp_num; i++)

#define for_each_hw_wp(i) for (i = 0; i < g_hw_manage.max_wp_num; i++)

struct hw_bp_manage_info {
	struct perf_event *__percpu *info; /*percpu bp info*/
	hw_bp_attr attr; /*bp attr*/
	int mask; /*bp register cpu mask*/
	char symbol_name[KSYM_SYMBOL_LEN]; /*symbol name of addr*/
	struct list_head rules; /*list of trigger rules*/
};

struct hw_bp_manage {
	struct hw_bp_manage_info wp[ARM_MAX_WRP]; /*wp*/
	struct hw_bp_manage_info bp[ARM_MAX_BRP]; /*bp*/
	int max_wp_num; /*max num of wp*/
	int max_bp_num; /*max num of bp*/
	int cpu_mask; /*cpu mask, num of cpu*/
	int cpu_num; /**/
	struct mutex lock; /*mutex lock*/
} __aligned(512);

typedef struct hw_bp_trace {
	struct pt_regs regs;
	hw_bp_callback_data report;
	hw_bp_callback handler;
	struct list_head list;
} hw_bp_trace;

typedef struct hw_bp_work {
	struct list_head head;
	int list_num;
	spinlock_t lock;
	struct task_struct *thread;
	struct semaphore sem;
} hw_bp_work;
struct mutex g_handle_lock;

static DEFINE_PER_CPU(hw_bp_work, g_hw_work);
static DEFINE_PER_CPU(int, hw_stepping_bp);

static struct hw_bp_manage g_hw_manage;
const char bp_type_str[4][30] = { "HW_BREAKPOINT_R", "HW_BREAKPOINT_W",
				  "HW_BREAKPOINT_RW", "HW_BREAKPOINT_X" };

extern void toggle_bp_registers(int reg, enum dbg_active_el el, int enable);
extern void hw_del_all_contion(u64 addr);
bool hw_check_contion(struct list_head *head, const struct pt_regs *regs,
		      const hw_bp_value *value, u64 access);
extern void hw_show_all_contion(u64 addr);

void hw_manage_lock(void)
{
	mutex_lock(&g_hw_manage.lock);
}

void hw_manage_unlock(void)
{
	mutex_unlock(&g_hw_manage.lock);
}

int hw_bp_thread_handler(void *data)
{
	hw_bp_work *work = data;
	hw_bp_trace *tail;

	while (!kthread_should_stop()) {
		if (down_interruptible(&work->sem))
			continue;
		spin_lock(&work->lock);
		tail = list_first_entry_or_null(&work->head, hw_bp_trace, list);
		if (IS_ERR_OR_NULL(tail)) {
			spin_unlock(&work->lock);
			continue;
		}
		list_del(&tail->list);
		work->list_num--;
		spin_unlock(&work->lock);

		mutex_lock(&g_handle_lock);
		tail->handler(&tail->report, &tail->regs);
		mutex_unlock(&g_handle_lock);
		kfree(tail);
	}
	return 0;
}

#if KERNEL_VERSION(6, 0, 0) <= LINUX_VERSION_CODE
static bool hw_backtrace_entry(void *arg, unsigned long where)
{
	hw_bp_trace *trace = arg;
	u32 *index = &trace->report.k_stack_size;

	if (*index >= HW_BP_TRACE_DEPTH)
		return false;

	trace->report.k_stack[(*index)++] = where;

	return true;
}
#endif

static void hw_bp_step_handler(struct perf_event *bp, struct pt_regs *regs)
{
	hw_bp_attr *attr = HW_GET_BP_ATTR(bp);
	struct arch_hw_breakpoint *info = counter_arch_bp(bp);
	hw_bp_trace *trace = NULL, *first = NULL;
	hw_bp_work *work = NULL;
	unsigned long flags;
	struct task_struct *task = current;
	u64 access_type = 0;
#if KERNEL_VERSION(6, 0, 0) > LINUX_VERSION_CODE
	int i = 0;
	struct stackframe frame;
#endif

	if (user_mode(regs) || attr->access_type == HW_BREAKPOINT_EMPTY)
		return;

	access_type = attr->access_type;
	attr->access_type = HW_BREAKPOINT_EMPTY;
	if (access_type & HW_BREAKPOINT_RW) {
		/*wp*/
		if (info->trigger < attr->addr ||
		    info->trigger >= attr->addr + attr->len) {
			/*Not within the detection range.*/
			return;
		}
	}

	switch (access_type) {
	case HW_BREAKPOINT_R: {
		/*read*/
		attr->times.read++;
		break;
	}
	case HW_BREAKPOINT_W: {
		/*write*/
		attr->times.write++;
		attr->value.new_flag = copy_from_kernel_nofault(
			&attr->value.new, (void *)info->trigger, 8);
		break;
	}
	case HW_BREAKPOINT_X: {
		/*exec*/
		attr->times.exec++;
		break;
	}
	default: {
		break;
	}
	}

	if (!hw_check_contion(attr->rule, regs, &attr->value, access_type))
		return;

	work = this_cpu_ptr(&g_hw_work);
	if (!spin_trylock_irqsave(&work->lock, flags))
		return;

	trace = kzalloc(sizeof(hw_bp_trace), GFP_KERNEL);
	if (IS_ERR_OR_NULL(trace))
		goto error;

	trace->handler = attr->handler;
	trace->report.addr = info->trigger;
	trace->report.type = access_type;
	trace->report.times = attr->times;
	trace->report.value = attr->value;
	memcpy(trace->report.comm, task->comm, TASK_COMM_LEN);
	trace->report.pid = task->pid;
	trace->report.cpu = smp_processor_id();
	trace->regs = *regs;
	if (!try_get_task_stack(task))
		goto out;

#if KERNEL_VERSION(6, 0, 0) <= LINUX_VERSION_CODE
	arch_stack_walk(hw_backtrace_entry, trace, task, regs);
#else
	start_backtrace(&frame, regs->regs[29], regs->pc);
	do {
		trace->report.k_stack[i++] = frame.pc;
		if (i >= HW_BP_TRACE_DEPTH)
			break;
	} while (!unwind_frame(task, &frame));
#endif
	put_task_stack(task);

out:
	if (work->list_num == HW_BP_LIST_MAX) {
		/*list is full*/
		first = list_first_entry(&work->head, hw_bp_trace, list);
		list_del(&first->list);
		work->list_num--;
		kfree(first);
	}
	list_add_tail(&trace->list, &work->head);
	work->list_num++;
	spin_unlock_irqrestore(&work->lock, flags);

	up(&work->sem);
	return;
error:
	spin_unlock_irqrestore(&work->lock, flags);
}

#if KERNEL_VERSION(6, 0, 0) <= LINUX_VERSION_CODE
static int hw_step_brk_fn(struct pt_regs *regs, unsigned long esr)
#else
static int hw_step_brk_fn(struct pt_regs *regs, unsigned int esr)
#endif
{
	int *hw_step = NULL;
	int handled_exception = DBG_HOOK_ERROR, i = 0;
	struct perf_event *bp;

	/*step states*/
	hw_step = this_cpu_ptr(&hw_stepping_bp);
	if (user_mode(regs) || !(*hw_step))
		return handled_exception;

	for_each_hw_bp(i) {
		if (!(g_hw_manage.bp[i].mask & g_hw_manage.cpu_mask))
			continue;
		bp = per_cpu(*g_hw_manage.bp[i].info, smp_processor_id());
		hw_bp_step_handler(bp, regs);
	}

	for_each_hw_wp(i) {
		if (!(g_hw_manage.wp[i].mask & g_hw_manage.cpu_mask))
			continue;
		bp = per_cpu(*g_hw_manage.wp[i].info, smp_processor_id());
		hw_bp_step_handler(bp, regs);
	}

	toggle_bp_registers(AARCH64_DBG_REG_BCR, DBG_ACTIVE_EL1, 1);
	toggle_bp_registers(AARCH64_DBG_REG_WCR, DBG_ACTIVE_EL1, 1);

	if (*hw_step != ARM_KERNEL_STEP_SUSPEND) {
		kernel_disable_single_step();
		handled_exception = DBG_HOOK_HANDLED;
	}

	*hw_step = ARM_KERNEL_STEP_NONE;

	return handled_exception;
}

static struct step_hook ghw_step_hook = { .fn = hw_step_brk_fn };

void hw_bp_perf_handler(struct perf_event *bp, struct perf_sample_data *data,
			struct pt_regs *regs)
{
	int *hw_step = NULL;
	hw_bp_attr *attr = HW_GET_BP_ATTR(bp);
	struct arch_hw_breakpoint *info = counter_arch_bp(bp);

	if (user_mode(regs))
		return;

	if (attr->access_type & HW_BREAKPOINT_RW) {
		toggle_bp_registers(AARCH64_DBG_REG_WCR, DBG_ACTIVE_EL1, 0);
		if (attr->access_type & HW_BREAKPOINT_W) {
			attr->value.old_flag = copy_from_kernel_nofault(
				&attr->value.old, (void *)info->trigger, 8);
		}
	} else {
		toggle_bp_registers(AARCH64_DBG_REG_BCR, DBG_ACTIVE_EL1, 0);
	}

	hw_step = this_cpu_ptr(&hw_stepping_bp);

	if (*hw_step != ARM_KERNEL_STEP_NONE)
		return;

	if (kernel_active_single_step())
		*hw_step = ARM_KERNEL_STEP_SUSPEND;
	else {
		*hw_step = ARM_KERNEL_STEP_ACTIVE;
		kernel_enable_single_step(regs);
	}
}

static void hw_show_regs(struct pt_regs *regs)
{
	int i, top_reg;
	u64 lr, sp;

	lr = regs->regs[30];
	sp = regs->sp;
	top_reg = 29;

	DST_PN("pc : %pS\n", (void *)regs->pc);
	DST_PN("lr : %pS\n", (void *)ptrauth_strip_kernel_insn_pac(lr));
	DST_PN("sp : %016llx\n", sp);

	if (system_uses_irq_prio_masking())
		DST_PN("pmr_save: %08llx\n", regs->pmr_save);

	i = top_reg;
	while (i >= 0) {
		DST_PN("x%-2d: %016llx", i, regs->regs[i]);
		while (i-- % 3)
			pr_cont(" x%-2d: %016llx", i, regs->regs[i]);
		pr_cont("\n");
	}
}

static void hw_bp_handler_default(const hw_bp_callback_data *info,
				  const struct pt_regs *regs)
{
	int i = 0;

	DST_PN("bp is triger = 0x%llx, type = %s\n", info->addr,
		bp_type_str[info->type - 1]);
	if (info->type & HW_BREAKPOINT_W) {
		if (!info->value.old_flag) {
			DST_PN("old: 0x%llx, new: 0x%llx\n", info->value.old,
				info->value.new);
		}
	}
	DST_PN("times: read=%llu, write=%llu, exec=%llu\n", info->times.read,
		info->times.write, info->times.exec);
	DST_PN("CPU: %d PID: %d Comm: %.20s\n", info->cpu, info->pid,
		info->comm);
	hw_show_regs((struct pt_regs *)regs);
	DST_PN("stack trace:\n");
	for (i = 0; i < HW_BP_TRACE_DEPTH; i++) {
		if (info->k_stack[i] == 0)
			break;
		DST_PN("\t %pS\n", (void *)info->k_stack[i]);
	}
}

static int hw_get_addr_mask(u64 addr, int len)
{
	/*end of the detect addr*/
	u64 addr_tmp = addr + len;
	u64 alignment_mask = 0;
	int mask, i = 0;

	/*log2(len)*/
	mask = (int)__ilog2_u64(len);
	if ((1 << mask) < len)
		mask = mask + 1;

	for (i = 0; i < mask; i++)
		alignment_mask |= (1 << i);

	/*Confirm that the end address is within the actual monitoring range*/
	while (1) {
		if ((addr | alignment_mask) >= addr_tmp)
			break;

		mask = mask + 1;
		alignment_mask |= (1 << i);
		i++;
	}

	if (mask > 31) {
		/*arm64 the mask is 0b11111*/
		mask = 31;
	}
	return mask;
}

/*show info of bp*/
static void hw_bp_show_one(struct hw_bp_manage_info *bp_info, int index)
{
	int cpu;
	struct perf_event *bp_percpu;
	hw_bp_attr *attr;

	DST_PN("--------------------------------------------------\n");
	/*index of bp*/
	switch (bp_info->attr.type) {
	case HW_BREAKPOINT_R:
	case HW_BREAKPOINT_W:
	case HW_BREAKPOINT_RW:
	case HW_BREAKPOINT_X: {
		DST_PN("breakpoint[%d]:\n", index);
		break;
	}
	default: {
		DST_PN("breakpoint[%d] type is error!\n", index);
		return;
	}
	}

	/*bp type*/
	DST_PN("\ttype: \t%s\n", bp_type_str[bp_info->attr.type - 1]);
	/*symbol name of addr*/
	DST_PN("\tname: \t%s\n", bp_info->symbol_name);
	/*the range of detect*/
	DST_PN("\tmonit: \t0x%llx--->0x%llx\n", bp_info->attr.addr,
		bp_info->attr.addr + bp_info->attr.len - 1);
	/*detect len*/
	DST_PN("\tlen: \t%llu\n", bp_info->attr.len);
	/*addr mask*/
	DST_PN("\tmask: \t0x%x\n", bp_info->attr.mask);
	/*the fact of detect range*/
	DST_PN("\trange: \t0x%llx--->0x%llx\n", bp_info->attr.start_addr,
		bp_info->attr.end_addr);
	DST_PN("\tsize: \t%llu\n",
		bp_info->attr.end_addr - bp_info->attr.start_addr);
	DST_PN("\ttimes:\n");
#if KERNEL_VERSION(6, 0, 0) <= LINUX_VERSION_CODE
	cpus_read_lock();
#else
	get_online_cpus();
#endif
	for_each_online_cpu(cpu) {
		if (bp_info->mask & 1 << cpu) {
			bp_percpu = per_cpu(*bp_info->info, cpu);
			attr = HW_GET_BP_ATTR(bp_percpu);
			DST_PN("\t\tcpu[%d]: \tread: %llu, write: %llu, exec: %llu\n",
				cpu, attr->times.read, attr->times.write,
				attr->times.exec);
		}
	}
#if KERNEL_VERSION(6, 0, 0) <= LINUX_VERSION_CODE
	cpus_read_unlock();
#else
	put_online_cpus();
#endif
	hw_show_all_contion(bp_info->attr.addr);
}

/*show all bp info*/
void hw_bp_show_all(void)
{
	struct hw_bp_manage_info *bp_info = NULL;
	int i = 0, cpu;

	hw_manage_lock();
	for_each_hw_bp(i) {
		if (!(g_hw_manage.bp[i].mask & g_hw_manage.cpu_mask))
			continue;
		bp_info = &g_hw_manage.bp[i];
		hw_bp_show_one(bp_info, i);
	}

	for_each_hw_wp(i) {
		if (!(g_hw_manage.wp[i].mask & g_hw_manage.cpu_mask))
			continue;
		bp_info = &g_hw_manage.wp[i];
		hw_bp_show_one(bp_info, i + g_hw_manage.max_bp_num);
	}
	hw_manage_unlock();
#if KERNEL_VERSION(6, 0, 0) <= LINUX_VERSION_CODE
	cpus_read_lock();
#else
	get_online_cpus();
#endif
	for_each_possible_cpu(cpu) {
		hw_bp_work *work = &per_cpu(g_hw_work, cpu);

		DST_PN("cpu[%d] list_num = %d\n", cpu, work->list_num);
	}
#if KERNEL_VERSION(6, 0, 0) <= LINUX_VERSION_CODE
	cpus_read_unlock();
#else
	put_online_cpus();
#endif
}

static int hw_bp_register(struct perf_event **__percpu *cpu_events,
			  hw_bp_attr *bp_attr, int *state)
{
	int ret = 0;
	struct perf_event_attr attr;
	struct perf_event *__percpu *hbp;

	hw_breakpoint_init(&attr);
	attr.bp_addr = bp_attr->start_addr;
	attr.bp_len = bp_attr->real_len;
	attr.bp_type = bp_attr->type;
	attr.bp_attr = *bp_attr;
	hbp = register_wide_hw_breakpoint(&attr, hw_bp_perf_handler, NULL);
	if (IS_ERR((void __force *)hbp)) {
		ret = (int)PTR_ERR((void __force *)hbp);
		goto err;
	}

	*state = g_hw_manage.cpu_mask;
	*cpu_events = hbp;
err:
	return ret;
}

static void hw_bp_unregister(struct perf_event *__percpu *bp, int state)
{
	unregister_wide_hw_breakpoint(bp);
}

/*install bp from addr*/
int hw_bp_install_from_addr(u64 addr, int len, int type, hw_bp_callback handler)
{
	int state, i, max_num, ret, mask = 0;
	struct hw_bp_manage_info *bp_info;
	u64 start_addr, end_addr;
	u64 alignment_mask = 0, real_len = len, offset;

	if ((addr == 0) || (addr < TASK_SIZE)) {
		DST_PN("para is error\n");
		return -1;
	}

	switch (type) {
	case HW_BREAKPOINT_R:
	case HW_BREAKPOINT_W:
	case HW_BREAKPOINT_RW: {
		/*wp*/
		bp_info = g_hw_manage.wp;
		max_num = g_hw_manage.max_wp_num;
		if (len > 8) {
			/*len>8, use mask*/
			mask = hw_get_addr_mask(addr, len);
			real_len = 4;
		}
		if (mask != 0) {
			/*get mask startaddr&endaddr*/
			for (i = 0; i < mask; i++)
				alignment_mask |= (1 << i);

			start_addr = addr & ~(alignment_mask);
			end_addr = addr | alignment_mask;
		} else {
			/*len<=8, use LBN*/
			alignment_mask = 0x7;
			offset = addr & alignment_mask;
			real_len = len << offset;
			if (real_len > 8)
				real_len = 8;

			start_addr = addr & ~(alignment_mask);
			end_addr = start_addr + real_len;
		}
		break;
	}
	case HW_BREAKPOINT_X: {
		/*bp*/
		real_len = 4;
		bp_info = g_hw_manage.bp;
		max_num = g_hw_manage.max_bp_num;
		alignment_mask = 0x3;
		offset = addr & alignment_mask;
		real_len = len << offset;
		if (real_len > 8)
			real_len = 8;

		start_addr = addr & ~(alignment_mask);
		end_addr = start_addr + real_len;
		break;
	}
	default: {
		/*bp type error*/
		DST_PN("breakpoint type error\n");
		return -1;
	}
	}

	hw_manage_lock();
	for (i = 0; i < max_num; i++) {
		if ((bp_info[i].mask & g_hw_manage.cpu_mask) != 0) {
			/*This bp has been set*/
			if (bp_info[i].attr.addr == addr) {
				DST_PN("[install] The addr [%llx] is already set at index %d\n",
					addr, i);
				hw_manage_unlock();
				return -1;
			}
		}
	}

	for (i = 0; i < max_num; i++) {
		if ((bp_info[i].mask & g_hw_manage.cpu_mask) != 0)
			continue;

		bp_info[i].attr.len = len;
		bp_info[i].attr.real_len = real_len;
		bp_info[i].attr.mask = mask;
		bp_info[i].attr.type = type;
		bp_info[i].attr.addr = addr;
		bp_info[i].attr.start_addr = start_addr;
		bp_info[i].attr.end_addr = end_addr;
		bp_info[i].attr.handler = handler;
		if (bp_info[i].attr.handler == NULL)
			bp_info[i].attr.handler = hw_bp_handler_default;

		break;
	}

	if (i == max_num) {
		DST_PN("[install] breakpoint is full type = %x\n", type);
		hw_manage_unlock();
		return -1;
	}

	bp_info[i].attr.rule = &bp_info[i].rules;
	ret = hw_bp_register(&bp_info[i].info, &bp_info[i].attr, &state);
	if (ret)
		goto clear;

	/*Several CPUs are registered with the breakpoint*/
	bp_info[i].mask = state;
	memset(bp_info[i].symbol_name, 0, sizeof(bp_info[i].symbol_name));
	sprint_symbol(bp_info[i].symbol_name, addr);
	hw_manage_unlock();
	hw_bp_show_one(&bp_info[i], i);
	return 0;
clear:
	DST_PN("[%llx] error\n", addr);
	/*clear bp info*/
	memset(&bp_info[i].attr, 0, sizeof(bp_info[i].attr));
	memset(bp_info[i].symbol_name, 0, sizeof(bp_info[i].symbol_name));
	bp_info[i].mask = 0;
	hw_manage_unlock();
	return -1;
}
EXPORT_SYMBOL_GPL(hw_bp_install_from_addr);

/*install from symbol*/
int hw_bp_install_from_symbol(char *name, int len, int type,
			      hw_bp_callback handler)
{
	int ret = 0;
	u64 addr = 0;

	if ((name == NULL) || (type == HW_BREAKPOINT_INVALID)) {
		DST_PN("para is error\n");
		return -1;
	}

	addr = kallsyms_lookup_name(name);
	if (addr == 0) {
		/*the symbol is invalid*/
		DST_PN("Can not find the symbol, name: %s\n", name);
		return -1;
	}

	ret = hw_bp_install_from_addr(addr, len, type, handler);
	if (ret) {
		DST_PN("error [%s]\n", name);
		return -1;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(hw_bp_install_from_symbol);

void hw_bp_uninstall_from_addr(u64 addr)
{
	int i = 0;
	struct hw_bp_manage_info *bp_info = NULL;

	/*traverse bp arrays*/
	/*find bp*/
	hw_manage_lock();
	for_each_hw_bp(i) {
		if (!(g_hw_manage.bp[i].mask & g_hw_manage.cpu_mask))
			continue;
		if (g_hw_manage.bp[i].attr.addr == addr) {
			bp_info = &g_hw_manage.bp[i];
			DST_PN("[uninstall] find addr: bp[%d]\n", i);
			break;
		}
	}

	/*find wp*/
	for_each_hw_wp(i) {
		if (!(g_hw_manage.wp[i].mask & g_hw_manage.cpu_mask))
			continue;
		if (bp_info)
			break;

		if (g_hw_manage.wp[i].attr.addr == addr) {
			bp_info = &g_hw_manage.wp[i];
			DST_PN("[uninstall] find addr: wp[%d]\n", i);
			break;
		}
	}

	if (bp_info == NULL) {
		DST_PN("fail,can not find addr:0x%llx\n", addr);
		hw_manage_unlock();
		return;
	}
	hw_bp_unregister(bp_info->info, bp_info->mask);
	/*clear bp info*/
	memset(bp_info->symbol_name, 0, sizeof(bp_info->symbol_name));
	memset(&bp_info->attr, 0, sizeof(bp_info->attr));
	bp_info->mask = 0;
	hw_manage_unlock();
	hw_del_all_contion(addr);
}
EXPORT_SYMBOL_GPL(hw_bp_uninstall_from_addr);

void hw_bp_uninstall_from_symbol(char *name)
{
	u64 addr = 0;

	if (name == NULL) {
		DST_PN("HW_breakpointUnInstallFromSymbol para is error\n");
		return;
	}

	addr = kallsyms_lookup_name(name);
	if (addr == 0) {
		/*the symbol is invalid*/
		DST_PN("[uninstall] Can not find the symbol, name: %s\n",
			name);
		return;
	}
	hw_bp_uninstall_from_addr(addr);
}
EXPORT_SYMBOL_GPL(hw_bp_uninstall_from_symbol);

void hw_free_bp_infos(hw_bp_info_list *info)
{
	hw_bp_info_list *node = NULL, *next = NULL;

	if (info) {
		list_for_each_entry_safe(node, next, &info->list, list) {
			list_del(&node->list);
			if (node->attr)
				kfree(node->attr);

			kfree(node);
		}
		if (info->attr)
			kfree(info->attr);

		kfree(info);
	}
}
EXPORT_SYMBOL_GPL(hw_free_bp_infos);

static void hw_fill_report_data(struct hw_bp_manage_info *bp_info,
				hw_bp_info_list *node)
{
	struct perf_event *bp = NULL;
	int cpu = 0;

#if KERNEL_VERSION(6, 0, 0) <= LINUX_VERSION_CODE
	cpus_read_lock();
#else
	get_online_cpus();
#endif
	for_each_online_cpu(cpu) {
		if (bp_info->mask & 1 << cpu) {
			bp = per_cpu(*bp_info->info, cpu);
			/*value*/
			node->attr[cpu].type = bp->attr.bp_attr.type;
			node->attr[cpu].addr = bp->attr.bp_attr.addr;
			node->attr[cpu].len = bp->attr.bp_attr.len;
			node->attr[cpu].mask = bp->attr.bp_attr.mask;
			node->attr[cpu].times = bp->attr.bp_attr.times;
		}
	}
#if KERNEL_VERSION(6, 0, 0) <= LINUX_VERSION_CODE
	cpus_read_unlock();
#else
	put_online_cpus();
#endif
}

hw_bp_info_list *hw_get_bp_infos(void)
{
	hw_bp_info_list *head = NULL;
	hw_bp_info_list *node = NULL;
	struct hw_bp_manage_info *bp_info = NULL;
	int i = 0;

	hw_manage_lock();
	for_each_hw_bp(i) {
		if (!(g_hw_manage.bp[i].mask & g_hw_manage.cpu_mask))
			continue;
		bp_info = &g_hw_manage.bp[i];
		/*bp is set*/
		if (head == NULL) {
			head = kzalloc(sizeof(hw_bp_info_list), GFP_KERNEL);
			if (head == NULL)
				goto err;

			INIT_LIST_HEAD(&head->list);
			head->attr = kcalloc(g_hw_manage.cpu_num,
					     sizeof(hw_bp_report), GFP_KERNEL);
			if (head->attr == NULL)
				goto err;

			head->cpu_mask = bp_info->mask;
			head->cpu_num = g_hw_manage.cpu_num;
			hw_fill_report_data(bp_info, head);
		}
		node = kzalloc(sizeof(hw_bp_info_list), GFP_KERNEL);
		if (node == NULL)
			goto err;

		INIT_LIST_HEAD(&node->list);
		list_add_tail(&node->list, &head->list);
		head->attr = kcalloc(g_hw_manage.cpu_num, sizeof(hw_bp_report),
				     GFP_KERNEL);
		if (node->attr == NULL)
			goto err;

		node->cpu_mask = bp_info->mask;
		node->cpu_num = g_hw_manage.cpu_num;
		hw_fill_report_data(bp_info, node);
	}

	for_each_hw_wp(i) {
		if (!(g_hw_manage.wp[i].mask & g_hw_manage.cpu_mask))
			continue;
		bp_info = &g_hw_manage.wp[i];
		/*bp is set*/
		if (head == NULL) {
			head = kzalloc(sizeof(hw_bp_info_list), GFP_KERNEL);
			if (head == NULL)
				goto err;

			INIT_LIST_HEAD(&head->list);
			head->attr = kcalloc(g_hw_manage.cpu_num,
					     sizeof(hw_bp_report), GFP_KERNEL);
			if (head->attr == NULL)
				goto err;

			head->cpu_mask = bp_info->mask;
			head->cpu_num = g_hw_manage.cpu_num;
			hw_fill_report_data(bp_info, head);
		}
		node = kzalloc(sizeof(hw_bp_info_list), GFP_KERNEL);
		if (node == NULL)
			goto err;

		INIT_LIST_HEAD(&node->list);
		list_add_tail(&node->list, &head->list);
		head->attr = kcalloc(g_hw_manage.cpu_num, sizeof(hw_bp_report),
				     GFP_KERNEL);
		if (node->attr == NULL)
			goto err;

		node->cpu_mask = bp_info->mask;
		node->cpu_num = g_hw_manage.cpu_num;
		hw_fill_report_data(bp_info, node);
	}
	hw_manage_unlock();

	return head;

err:
	hw_manage_unlock();
	hw_free_bp_infos(head);
	return NULL;
}
EXPORT_SYMBOL_GPL(hw_get_bp_infos);

struct list_head *hw_get_rules(u64 addr)
{
	int i = 0;
	struct hw_bp_manage_info *bp_info = NULL;
	int lock = 0;

	/*traverse bp arrays*/
	/*find bp*/
	lock = mutex_trylock(&g_handle_lock);
	for_each_hw_bp(i) {
		if (!(g_hw_manage.bp[i].mask & g_hw_manage.cpu_mask))
			continue;
		if (g_hw_manage.bp[i].attr.addr == addr) {
			bp_info = &g_hw_manage.bp[i];
			break;
		}
	}

	/*find wp*/
	for_each_hw_wp(i) {
		if (!(g_hw_manage.wp[i].mask & g_hw_manage.cpu_mask))
			continue;
		if (bp_info)
			break;

		if (g_hw_manage.wp[i].attr.addr == addr) {
			bp_info = &g_hw_manage.wp[i];
			break;
		}
	}
	if (lock)
		mutex_unlock(&g_handle_lock);

	if (bp_info)
		return &bp_info->rules;

	return NULL;
}

/*release bp*/
void hw_bp_manage_deinit(void)
{
	int i = 0;

	for (i = 0; i < g_hw_manage.max_wp_num; i++)
		free_percpu(g_hw_manage.wp[i].info);

	for (i = 0; i < g_hw_manage.max_bp_num; i++)
		free_percpu(g_hw_manage.bp[i].info);

	mutex_destroy(&g_hw_manage.lock);
}

/*bp arch init*/
int hw_bp_manage_init(void)
{
	int cpu = -1, i = 0;
	struct perf_event *__percpu *bp = NULL;
	hw_bp_work *work = NULL;

	/*get bp&wp num*/
	g_hw_manage.max_bp_num = hw_breakpoint_slots(TYPE_INST);
	g_hw_manage.max_wp_num = hw_breakpoint_slots(TYPE_DATA);

	/*get CPU num*/
	g_hw_manage.cpu_num = 0;
#if KERNEL_VERSION(6, 0, 0) <= LINUX_VERSION_CODE
	cpus_read_lock();
#else
	get_online_cpus();
#endif
	for_each_online_cpu(cpu) {
		g_hw_manage.cpu_mask |= 1 << cpu;
		g_hw_manage.cpu_num++;
		work = &per_cpu(g_hw_work, cpu);
		sema_init(&work->sem, 0);
		INIT_LIST_HEAD(&work->head);
		spin_lock_init(&work->lock);
		sema_init(&work->sem, 0);
		work->thread = kthread_create(hw_bp_thread_handler, work,
					      "hw_bp_thread");
		if (IS_ERR_OR_NULL(work->thread))
			continue;

		kthread_bind(work->thread, cpu);
		wake_up_process(work->thread);
	}
#if KERNEL_VERSION(6, 0, 0) <= LINUX_VERSION_CODE
	cpus_read_unlock();
#else
	put_online_cpus();
#endif
	DST_PN("CPU MASK =  %x\n", g_hw_manage.cpu_mask);

	/*mange mem of bp*/
	for (i = 0; i < g_hw_manage.max_wp_num; i++) {
		bp = alloc_percpu(typeof(*bp));
		if (!bp) {
			DST_PN("wp alloc_percpu fail\n");
			goto free;
		}
		g_hw_manage.wp[i].info = bp;
		INIT_LIST_HEAD(&g_hw_manage.wp[i].rules);
		bp = NULL;
	}
	for (i = 0; i < g_hw_manage.max_bp_num; i++) {
		bp = alloc_percpu(typeof(*bp));
		if (!bp) {
			DST_PN("wp alloc_percpu fail\n");
			goto free;
		}
		g_hw_manage.bp[i].info = bp;
		INIT_LIST_HEAD(&g_hw_manage.bp[i].rules);
		bp = NULL;
	}

	mutex_init(&g_hw_manage.lock);
	mutex_init(&g_handle_lock);

	register_kernel_step_hook(&ghw_step_hook);

	return 0;

free:
	hw_bp_manage_deinit();
	return -1;
}

extern int hw_proc_init(void);
extern void hw_until_init(void);
/*hp init*/
static int __init hw_bp_init(void)
{
	hw_bp_manage_init();
	hw_proc_init();
	hw_until_init();
	return 0;
}

arch_initcall(hw_bp_init);

MODULE_AUTHOR("Vimoon Zheng <Vimoon.Zheng@cixtech.com>");
MODULE_DESCRIPTION("hardware breakpoint for SKY1 and later");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform: sky1-bp");
