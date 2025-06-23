#include <asm/debug-monitors.h>
#include <linux/sched/debug.h>
#include <asm/hw_breakpoint.h>
#include <linux/hw_breakpoint.h>
#include <linux/kallsyms.h>
#include <linux/version.h>
#include <asm/stacktrace.h>
#include <asm/system_misc.h>

#define HW_GET_BP_ATTR(bp) (&bp->attr.bp_attr)
#define HW_BP_LIST_MAX 20

#define for_each_hw_bp(i)                            \
	for (i = 0; i < g_hw_manage.max_bp_num; i++) \
		if (g_hw_manage.bp[i].mask & g_hw_manage.cpu_mask)
#define for_each_hw_wp(i)                            \
	for (i = 0; i < g_hw_manage.max_wp_num; i++) \
		if (g_hw_manage.wp[i].mask & g_hw_manage.cpu_mask)

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
		if (down_interruptible(&work->sem)) {
			continue;
		}
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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
static bool hw_backtrace_entry(void *arg, unsigned long where)
{
	hw_bp_trace *trace = arg;
	u32 *index = &trace->report.k_stack_size;

	if (*index >= HW_BP_TRACE_DEPTH) {
		return false;
	}
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
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0)
	int i = 0;
	struct stackframe frame;
#endif

	if (user_mode(regs) || attr->access_type == HW_BREAKPOINT_EMPTY) {
		return;
	}

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

	if (!hw_check_contion(attr->rule, regs, &attr->value, access_type)) {
		return;
	}

	work = this_cpu_ptr(&g_hw_work);
	if (!spin_trylock_irqsave(&work->lock, flags)) {
		return;
	}
	trace = kzalloc(sizeof(hw_bp_trace), GFP_KERNEL);
	if (IS_ERR_OR_NULL(trace)) {
		goto error;
	}

	trace->handler = attr->handler;
	trace->report.addr = info->trigger;
	trace->report.type = access_type;
	trace->report.times = attr->times;
	trace->report.value = attr->value;
	memcpy(trace->report.comm, task->comm, TASK_COMM_LEN);
	trace->report.pid = task->pid;
	trace->report.cpu = smp_processor_id();
	trace->regs = *regs;
	if (!try_get_task_stack(task)) {
		goto out;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
	arch_stack_walk(hw_backtrace_entry, trace, task, regs);
#else
	start_backtrace(&frame, regs->regs[29], regs->pc);
	do {
		trace->report.k_stack[i++] = frame.pc;
		if (i >= HW_BP_TRACE_DEPTH) {
			break;
		}
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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
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
		bp = per_cpu(*g_hw_manage.bp[i].info, smp_processor_id());
		hw_bp_step_handler(bp, regs);
	}

	for_each_hw_wp(i) {
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

	if (user_mode(regs)) {
		return;
	}

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

	if (kernel_active_single_step()) {
		*hw_step = ARM_KERNEL_STEP_SUSPEND;
	} else {
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

	pr_info("pc : %pS\n", (void *)regs->pc);
	pr_info("lr : %pS\n", (void *)ptrauth_strip_insn_pac(lr));
	pr_info("sp : %016llx\n", sp);

	if (system_uses_irq_prio_masking())
		pr_info("pmr_save: %08llx\n", regs->pmr_save);

	i = top_reg;
	while (i >= 0) {
		pr_info("x%-2d: %016llx", i, regs->regs[i]);
		while (i-- % 3)
			pr_cont(" x%-2d: %016llx", i, regs->regs[i]);
		pr_cont("\n");
	}
}

static void hw_bp_handler_default(const hw_bp_callback_data *info,
				  const struct pt_regs *regs)
{
	int i = 0;

	pr_info("bp is triger = 0x%llx, type = %s\n", info->addr,
		bp_type_str[info->type - 1]);
	if (info->type & HW_BREAKPOINT_W) {
		if (!info->value.old_flag) {
			pr_info("old: 0x%llx, new: 0x%llx\n", info->value.old,
				info->value.new);
		}
	}
	pr_info("times: read=%llu, write=%llu, exec=%llu\n", info->times.read,
		info->times.write, info->times.exec);
	pr_info("CPU: %d PID: %d Comm: %.20s \n", info->cpu, info->pid,
		info->comm);
	hw_show_regs((struct pt_regs *)regs);
	pr_info("stack trace:\n");
	for (i = 0; i < HW_BP_TRACE_DEPTH; i++) {
		if (info->k_stack[i] == 0) {
			break;
		}
		pr_info("\t %pS\n", (void *)info->k_stack[i]);
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
	if ((1 << mask) < len) {
		mask = mask + 1;
	}
	for (i = 0; i < mask; i++) {
		alignment_mask |= (1 << i);
	}

	/*Confirm that the end address is within the actual monitoring range*/
	while (1) {
		if ((addr | alignment_mask) >= addr_tmp) {
			break;
		}
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

	pr_info("--------------------------------------------------\n");
	/*index of bp*/
	switch (bp_info->attr.type) {
	case HW_BREAKPOINT_R:
	case HW_BREAKPOINT_W:
	case HW_BREAKPOINT_RW:
	case HW_BREAKPOINT_X: {
		pr_info("breakpoint[%d]:\n", index);
		break;
	}
	default: {
		pr_info("breakpoint[%d] type is error!\n", index);
		return;
	}
	}

	/*bp type*/
	pr_info("\ttype: \t%s\n", bp_type_str[bp_info->attr.type - 1]);
	/*symbol name of addr*/
	pr_info("\tname: \t%s\n", bp_info->symbol_name);
	/*the range of detect*/
	pr_info("\tmonit: \t0x%llx--->0x%llx\n", bp_info->attr.addr,
		bp_info->attr.addr + bp_info->attr.len - 1);
	/*detect len*/
	pr_info("\tlen: \t%llu\n", bp_info->attr.len);
	/*addr mask*/
	pr_info("\tmask: \t0x%x\n", bp_info->attr.mask);
	/*the fact of detect range*/
	pr_info("\trange: \t0x%llx--->0x%llx\n", bp_info->attr.start_addr,
		bp_info->attr.end_addr);
	pr_info("\tsize: \t%llu\n",
		bp_info->attr.end_addr - bp_info->attr.start_addr);
	pr_info("\ttimes:\n");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
	cpus_read_lock();
#else
	get_online_cpus();
#endif
	for_each_online_cpu(cpu) {
		if (bp_info->mask & 1 << cpu) {
			bp_percpu = per_cpu(*bp_info->info, cpu);
			attr = HW_GET_BP_ATTR(bp_percpu);
			pr_info("\t\tcpu[%d]: \tread: %llu, write: %llu, exec: %llu\n",
				cpu, attr->times.read, attr->times.write,
				attr->times.exec);
		}
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
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
		bp_info = &g_hw_manage.bp[i];
		hw_bp_show_one(bp_info, i);
	}

	for_each_hw_wp(i) {
		bp_info = &g_hw_manage.wp[i];
		hw_bp_show_one(bp_info, i + g_hw_manage.max_bp_num);
	}
	hw_manage_unlock();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
	cpus_read_lock();
#else
	get_online_cpus();
#endif
	for_each_possible_cpu(cpu) {
		hw_bp_work *work = &per_cpu(g_hw_work, cpu);
		pr_info("cpu[%d] list_num = %d\n", cpu, work->list_num);
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
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

	if ((0 == addr) || (addr < TASK_SIZE)) {
		pr_info("hw_bp_install_from_addr para is error\n");
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
			for (i = 0; i < mask; i++) {
				alignment_mask |= (1 << i);
			}
			start_addr = addr & ~(alignment_mask);
			end_addr = addr | alignment_mask;
		} else {
			/*len<=8, use LBN*/
			alignment_mask = 0x7;
			offset = addr & alignment_mask;
			real_len = len << offset;
			if (real_len > 8) {
				real_len = 8;
			}
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
		if (real_len > 8) {
			real_len = 8;
		}
		start_addr = addr & ~(alignment_mask);
		end_addr = start_addr + real_len;
		break;
	}
	default: {
		/*bp type error*/
		pr_info("breakpoint type error\n");
		return -1;
	}
	}

	hw_manage_lock();
	for (i = 0; i < max_num; i++) {
		if ((bp_info[i].mask & g_hw_manage.cpu_mask) != 0) {
			/*This bp has been set*/
			if (bp_info[i].attr.addr == addr) {
				pr_info("[install] The addr [%llx] is already set at index %d\n",
					addr, i);
				hw_manage_unlock();
				return -1;
			}
		}
	}

	for (i = 0; i < max_num; i++) {
		if ((bp_info[i].mask & g_hw_manage.cpu_mask) != 0) {
			continue;
		}
		bp_info[i].attr.len = len;
		bp_info[i].attr.real_len = real_len;
		bp_info[i].attr.mask = mask;
		bp_info[i].attr.type = type;
		bp_info[i].attr.addr = addr;
		bp_info[i].attr.start_addr = start_addr;
		bp_info[i].attr.end_addr = end_addr;
		bp_info[i].attr.handler = handler;
		if (bp_info[i].attr.handler == NULL) {
			bp_info[i].attr.handler = hw_bp_handler_default;
		}
		break;
	}

	if (i == max_num) {
		pr_info("[install] breakpoint is full type = %x\n", type);
		hw_manage_unlock();
		return -1;
	}

	// pr_info("gHwManage.wp[%d].info = %lx\n", i, gHwManage.wp[i].info);
	// pr_info("info = %lx,attr=%lx,state=%lx\n", bpInfo[i].info, &bpInfo[i].attr, &state);
	bp_info[i].attr.rule = &bp_info[i].rules;
	ret = hw_bp_register(&bp_info[i].info, &bp_info[i].attr, &state);
	if (ret) {
		goto clear;
	}
	/*Several CPUs are registered with the breakpoint*/
	bp_info[i].mask = state;
	memset(bp_info[i].symbol_name, 0, sizeof(bp_info[i].symbol_name));
	sprint_symbol(bp_info[i].symbol_name, addr);
	hw_manage_unlock();
	hw_bp_show_one(&bp_info[i], i);
	return 0;
clear:
	pr_info("hw_bp_install_from_addr [%llx] error\n", addr);
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

	if ((NULL == name) || (HW_BREAKPOINT_INVALID == type)) {
		pr_info("HW_breakpointInstallFromSymbol para is error\n");
		return -1;
	}

	addr = kallsyms_lookup_name(name);
	if (0 == addr) {
		/*the symbol is invalid*/
		pr_info("Can not find the symbol, name: %s\n", name);
		return -1;
	}

	ret = hw_bp_install_from_addr(addr, len, type, handler);
	if (ret) {
		pr_info("HW_breakpointInstallFromSymbol error [%s]\n", name);
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
		if (g_hw_manage.bp[i].attr.addr == addr) {
			bp_info = &g_hw_manage.bp[i];
			pr_info("[uninstall] find addr: bp[%d]\n", i);
			break;
		}
	}

	/*find wp*/
	for_each_hw_wp(i) {
		if (bp_info) {
			break;
		}
		if (g_hw_manage.wp[i].attr.addr == addr) {
			bp_info = &g_hw_manage.wp[i];
			pr_info("[uninstall] find addr: wp[%d]\n", i);
			break;
		}
	}

	if (NULL == bp_info) {
		pr_info("HW_breakpointUnInstallFromAddr fail,can not find addr:0x%llx\n",
			addr);
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

	if (NULL == name) {
		pr_info("HW_breakpointUnInstallFromSymbol para is error\n");
		return;
	}

	addr = kallsyms_lookup_name(name);
	if (0 == addr) {
		/*the symbol is invalid*/
		pr_info("[uninstall] Can not find the symbol, name: %s\n",
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
			if (node->attr) {
				kfree(node->attr);
			}
			kfree(node);
		}
		if (info->attr) {
			kfree(info->attr);
		}
		kfree(info);
	}
}
EXPORT_SYMBOL_GPL(hw_free_bp_infos);

static void hw_fill_report_data(struct hw_bp_manage_info *bp_info,
				hw_bp_info_list *node)
{
	struct perf_event *bp = NULL;
	int cpu = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
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
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
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
		bp_info = &g_hw_manage.bp[i];
		/*bp is set*/
		if (head == NULL) {
			head = kzalloc(sizeof(hw_bp_info_list), GFP_KERNEL);
			if (head == NULL) {
				goto err;
			}
			INIT_LIST_HEAD(&head->list);
			head->attr = kzalloc(sizeof(hw_bp_report) *
						     g_hw_manage.cpu_num,
					     GFP_KERNEL);
			if (head->attr == NULL) {
				goto err;
			}
			head->cpu_mask = bp_info->mask;
			head->cpu_num = g_hw_manage.cpu_num;
			hw_fill_report_data(bp_info, head);
		}
		node = kzalloc(sizeof(hw_bp_info_list), GFP_KERNEL);
		if (node == NULL) {
			goto err;
		}
		INIT_LIST_HEAD(&node->list);
		list_add_tail(&node->list, &head->list);
		node->attr = kzalloc(sizeof(hw_bp_report) * g_hw_manage.cpu_num,
				     GFP_KERNEL);
		if (node->attr == NULL) {
			goto err;
		}
		node->cpu_mask = bp_info->mask;
		node->cpu_num = g_hw_manage.cpu_num;
		hw_fill_report_data(bp_info, node);
	}

	for_each_hw_wp(i) {
		bp_info = &g_hw_manage.wp[i];
		/*bp is set*/
		if (head == NULL) {
			head = kzalloc(sizeof(hw_bp_info_list), GFP_KERNEL);
			if (head == NULL) {
				goto err;
			}
			INIT_LIST_HEAD(&head->list);
			head->attr = kzalloc(sizeof(hw_bp_report) *
						     g_hw_manage.cpu_num,
					     GFP_KERNEL);
			if (head->attr == NULL) {
				goto err;
			}
			head->cpu_mask = bp_info->mask;
			head->cpu_num = g_hw_manage.cpu_num;
			hw_fill_report_data(bp_info, head);
		}
		node = kzalloc(sizeof(hw_bp_info_list), GFP_KERNEL);
		if (node == NULL) {
			goto err;
		}
		INIT_LIST_HEAD(&node->list);
		list_add_tail(&node->list, &head->list);
		node->attr = kzalloc(sizeof(hw_bp_report) * g_hw_manage.cpu_num,
				     GFP_KERNEL);
		if (node->attr == NULL) {
			goto err;
		}
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
		if (g_hw_manage.bp[i].attr.addr == addr) {
			bp_info = &g_hw_manage.bp[i];
			break;
		}
	}

	/*find wp*/
	for_each_hw_wp(i) {
		if (bp_info) {
			break;
		}
		if (g_hw_manage.wp[i].attr.addr == addr) {
			bp_info = &g_hw_manage.wp[i];
			break;
		}
	}
	if (lock) {
		mutex_unlock(&g_handle_lock);
	}

	if (bp_info) {
		return &bp_info->rules;
	}

	return NULL;
}

/*release bp*/
void hw_bp_manage_deinit(void)
{
	int i = 0;

	// hw_bp_uninstall_all();

	for (i = 0; i < g_hw_manage.max_wp_num; i++) {
		free_percpu(g_hw_manage.wp[i].info);
	}

	for (i = 0; i < g_hw_manage.max_bp_num; i++) {
		free_percpu(g_hw_manage.bp[i].info);
	}
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
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
	cpus_read_lock();
#else
	get_online_cpus();
#endif
	for_each_online_cpu(cpu) {
		g_hw_manage.cpu_mask |= 1 << cpu;
		g_hw_manage.cpu_num++;
		work = &per_cpu(g_hw_work, cpu);
		INIT_LIST_HEAD(&work->head);
		spin_lock_init(&work->lock);
		work->thread = kthread_create(hw_bp_thread_handler, work,
					      "hw_bp_thread");
		if (IS_ERR_OR_NULL(work->thread)) {
			continue;
		}
		kthread_bind(work->thread, cpu);
		wake_up_process(work->thread);
		sema_init(&work->sem, 0);
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
	cpus_read_unlock();
#else
	put_online_cpus();
#endif
	pr_info("CPU MASK =  %x\n", g_hw_manage.cpu_mask);

	/*mange mem of bp*/
	for (i = 0; i < g_hw_manage.max_wp_num; i++) {
		bp = alloc_percpu(typeof(*bp));
		if (!bp) {
			pr_info("wp alloc_percpu fail\n");
			goto free;
		}
		g_hw_manage.wp[i].info = bp;
		INIT_LIST_HEAD(&g_hw_manage.wp[i].rules);
		bp = NULL;
	}
	for (i = 0; i < g_hw_manage.max_bp_num; i++) {
		bp = alloc_percpu(typeof(*bp));
		if (!bp) {
			pr_info("wp alloc_percpu fail\n");
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

	pr_info("%s ok\n", __FUNCTION__);
	return 0;
}

arch_initcall(hw_bp_init);

MODULE_AUTHOR("Vimoon Zheng <Vimoon.Zheng@cixtech.com>");
MODULE_DESCRIPTION("hardware breakpoint for SKY1 and later");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform: sky1-bp");
