// SPDX-License-Identifier: GPL-2.0-only
/*
 * Allwinner trace stack
 *
 * Copyright (C) 2022 Allwinner.
 */

#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/nmi.h>
#include <trace/hooks/softlockup.h>
#include <linux/interrupt.h>
#include <linux/sched/debug.h>
#include <linux/sched.h>
#include <trace/hooks/sched.h>
#include <linux/hrtimer.h>
#include <linux/kernel_stat.h>
#include <trace/hooks/mm.h>
#include <linux/jiffies.h>
#include <linux/kprobes.h>
#include <linux/of.h>
#include <linux/of_address.h>

#define CPU_NUMS 8
#define STACK_SIZE 2048
struct hrtimer healthd_hrtimer;
static u64 sched_info_count;
static DEFINE_RAW_SPINLOCK(cpu_bt_lock);
static struct task_struct *aw_healthd;
static int check_tsk_status;
static void __iomem *gic_base;
unsigned long *entries[CPU_NUMS];
unsigned char *stackbuf[CPU_NUMS];

ktime_t sd_time[8];
static LIST_HEAD(tsk_list);
static LIST_HEAD(sched_info_list);
static LIST_HEAD(tsk_data_free_list);
static struct list_head *sched_info_pos;
static DEFINE_RAW_SPINLOCK(tsk_list_lock);
static DEFINE_RAW_SPINLOCK(td_free_list_lock);

u64 detect_period_ms = 1000; //ms
#define SCHED_INFO_COUNT_MAX 1000000
unsigned int sched_info_count_max = 100000;
unsigned int sched_running_thr = 10000;
unsigned int sched_hl_thr = 2000;
unsigned int sched_rt_thr = 2000;
unsigned int sched_ivcs_thr = 5000;

unsigned int aw_healthd_enable = 1;
unsigned int rvh_schedule_enable = 1;
unsigned int alloc_pages_ms = 4;

u64 healthd_hrtimer_count;

#define GIC_DISTRIBUTOR_ENABLE 0x100
#define GIC_DISTRIBUTOR_PENDING 0x200
#define GIC_DISTRIBUTOR_ACTIVE 0x300

#define is_rt_tsk(tsk) (tsk->prio < 100 ? true : false)

static u64 sd_prev_ktime[8];

struct tsk_data {
	int prev_pid, next_pid;
	unsigned long ncsw, nvcsw;
	s64 rtime_us;
	u64 sum_exec_runtime;
	struct task_struct *prev, *next;
	struct list_head list;
	u32 dlen;
	char *dbuf;
};

void free_tsk_data(struct tsk_data *tsk_data);
void move_to_sched_info(struct tsk_data *tsk_data);
static void system_irq_stat(int print_show);

struct tsk_data *get_tsk_data(u32 len)
{
	struct tsk_data *tsk_data = NULL;
	struct list_head *next;

	raw_spin_lock(&td_free_list_lock);
	if (len <= 256 && tsk_data_free_list.next != tsk_data_free_list.prev) {
		next = tsk_data_free_list.next;
		list_del(next);
		tsk_data = container_of(next, struct tsk_data, list);
	}
	raw_spin_unlock(&td_free_list_lock);
	if (!tsk_data) {
		if (len < 256)
			len = 256;
		tsk_data = kzalloc(sizeof(*tsk_data), GFP_ATOMIC);
		if (!tsk_data) {
			pr_err("kmalloc tsk_data failed\n");
			return NULL;
		}
		tsk_data->dbuf = kzalloc(len, GFP_ATOMIC);
		if (!tsk_data->dbuf) {
			pr_err("kmalloc tsk_data dbuf failed, len=%d\n", len);
			kfree(tsk_data);
		}
		tsk_data->dlen = len;
	}

	return tsk_data;
}

static void get_gic_base(void)
{
	struct device_node *gic_node;
	struct resource res;

	gic_node = of_find_node_by_name(NULL, "interrupt-controller");

	if (!gic_node) {
		pr_err("error!Can't find gic node");
		return;
	}

	if (of_address_to_resource(gic_node, 0, &res)) {
		pr_err("Error:failed to get gic resource");
		goto out;
	}

	gic_base = ioremap(res.start, resource_size(&res));
out:
	of_node_put(gic_node);
	return;

}

static void show_cpu_bt(void *dummy)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&cpu_bt_lock, flags);
	pr_info("CPU%d:\n", smp_processor_id());
	dump_stack();
	raw_spin_unlock_irqrestore(&cpu_bt_lock, flags);
}

static void show_all_cpu_bt(struct pt_regs *regs)
{
	int cpu;

	if (!trigger_all_cpu_backtrace()) {
		cpu = get_cpu();
		pr_info("CPU%d:\n", cpu);
		put_cpu();
		if (regs)
			show_regs(regs);
		else
			dump_stack();

		smp_call_function(show_cpu_bt, NULL, 0);
	}
}

void cpu_stat(void)
{
	int cpu;
	u64 *cpustat;
	u64 user, nice, system, idle, iowait, irq, softirq;
	struct kernel_cpustat *kcpustat;
	u64 irqs[NR_CPUS];
	u32 len = 0;
	char *tbuf = NULL;
	struct tsk_data *tsk_data = NULL;

	tbuf = kzalloc(128 * nr_cpu_ids, GFP_ATOMIC);
	if (!tbuf) {
		pr_err("kmalloc tsk_data failed\n");
		return;
	}
	for (cpu = 0; cpu < nr_cpu_ids; cpu++) {
		kcpustat = &kcpustat_cpu(cpu);
		cpustat = kcpustat->cpustat;
		user		= cpustat[CPUTIME_USER];
		nice		= cpustat[CPUTIME_NICE];
		system		= cpustat[CPUTIME_SYSTEM];
		idle		= cpustat[CPUTIME_IOWAIT];
		iowait		= cpustat[CPUTIME_IDLE];
		irq		= cpustat[CPUTIME_IRQ];
		softirq		= cpustat[CPUTIME_SOFTIRQ];
		irqs[cpu]	= kstat_cpu_irqs_sum(cpu);

		len += sprintf(tbuf + len, "CPU%d: %ld  %ld  %ld  %ld  %ld  %ld  %ld\n",
			cpu,
			nsec_to_clock_t(user),
			nsec_to_clock_t(nice),
			nsec_to_clock_t(system),
			nsec_to_clock_t(idle),
			nsec_to_clock_t(iowait),
			nsec_to_clock_t(irq),
			nsec_to_clock_t(softirq)
			);
	}
	tsk_data = get_tsk_data(len + 1);
	if (!tsk_data) {
		pr_err("cpu stat get tsk_data failed\n");
		return;
	}

	INIT_LIST_HEAD(&tsk_data->list);
	memcpy(tsk_data->dbuf, tbuf, len);
	move_to_sched_info(tsk_data);
	kfree(tbuf);
}

void dump_all_task(void)
{
	struct task_struct *p, *g;

	for_each_process_thread(g, p) {
		sched_show_task(p);
	}
}

static void android_vh_watchdog_timer_softlockup(void *p, int duration, struct pt_regs *regs, bool is_panic)
{
	system_irq_stat(1);
	show_all_cpu_bt(regs);
	system_irq_stat(1);

	return;
}

static void android_vh_alloc_pages_slowpath(void *p, gfp_t gfp_mask, unsigned int order,
					    unsigned long delta)
{
	unsigned long alloc_end = jiffies;
	int num_entries = 0, len = 0;
	int i;
	struct tsk_data *tsk_data = NULL;
	unsigned long flags;
	int cpu = get_cpu();
	put_cpu();

	if (jiffies_to_msecs(alloc_end - delta) > alloc_pages_ms) {
		len += sprintf(stackbuf[cpu] + len,
			       "process:%s(%d) on cpu%d, prio:%d\n",
			       current->comm, current->prio, smp_processor_id(), current->prio);
		len += sprintf(stackbuf[cpu] + len,
			       ", alloc page order:%d, gfp_mask:%#x consume %dms\n",
			       order, gfp_mask, jiffies_to_msecs(alloc_end - delta));
		num_entries = stack_trace_save(entries[cpu], STACK_SIZE, 0);
		for (i = 0; i < num_entries; i++)
			len += sprintf(stackbuf[cpu] + len, " %pS\n", (void *)*(entries[cpu] + i));
		tsk_data = get_tsk_data(len + 1);
		if (!tsk_data) {
			pr_err("alloc_pages slowpath get tsk_data failed\n");
			return;
		}
		INIT_LIST_HEAD(&tsk_data->list);
		memcpy(tsk_data->dbuf, stackbuf[cpu], len);
		raw_spin_lock_irqsave(&tsk_list_lock, flags);
		move_to_sched_info(tsk_data);
		raw_spin_unlock_irqrestore(&tsk_list_lock, flags);
	}
}

char *print_task_flag(struct task_struct *p, char *buf)
{
	int len = 0;
	int state = p->__state & (TASK_REPORT_MAX -1);

	if (state & TASK_RUNNING)
		len += sprintf(buf + len, "R");
	if (state & TASK_INTERRUPTIBLE)
		len += sprintf(buf + len, "S");
	if (state & TASK_UNINTERRUPTIBLE)
		len += sprintf(buf + len, "D");
	if (state & __TASK_STOPPED)
		len += sprintf(buf + len, "T");
	if (state & EXIT_DEAD)
		len += sprintf(buf + len, "X");
	if (state & EXIT_ZOMBIE)
		len += sprintf(buf + len, "Z");
	if (state & TASK_PARKED)
		len += sprintf(buf + len, "P");
	if (state & TASK_DEAD)
		len += sprintf(buf + len, "I");
	sprintf(buf + len, " 0x%x", state);
	return buf;
}

static void  android_rvh_schedule(void *p, struct task_struct *prev, struct task_struct *next,
				  struct rq *rq)
{
	s64 rtime_us = 0;
	int len = 0;
	char buf[16];
	char tbuf[256];
	struct sched_entity *se = &prev->se;
	struct tsk_data *tsk_data = NULL;
	unsigned long ncsw;
	unsigned long flags;
	ktime_t now;
	u32 cpu;
	u64 timestamp_s, timestamp_us;

	if (!rvh_schedule_enable)
		return;

	cpu = get_cpu();
	put_cpu();
	now = ktime_get();
	rtime_us = ktime_to_us(ktime_sub(now, sd_prev_ktime[cpu]));
	sd_prev_ktime[cpu] = now;
	if (check_tsk_status)
		return;

	if (prev != next && !is_idle_task(prev)) {
		//record message
		if ((prev->__state & TASK_INTERRUPTIBLE) && rtime_us < sched_rt_thr)
			return;
		if ((is_rt_tsk(prev) || is_rt_tsk(next)) && rtime_us < sched_rt_thr)
			return;
		if ((prev->__state & TASK_INTERRUPTIBLE) && rtime_us > sched_rt_thr)
			return;

		ncsw = prev->nvcsw + prev->nivcsw;
		timestamp_s = ktime_to_us(now) / 1000000;
		timestamp_us = ktime_to_us(now) % 1000000;
		len = snprintf(tbuf, 256,
			       "[%u.%u][c%d]prev:%s(%d) exec=%lluus sum_exec=%llums ncsw=%lu prio=%d %s => next:%s(%d) prio=%d\n",
			       timestamp_s,
			       timestamp_us,
			       cpu,
			       prev->comm,
			       prev->pid,
			       rtime_us,
			       se->sum_exec_runtime / 1000000,
			       ncsw,
			       prev->prio,
			       print_task_flag(prev, buf),
			       next->comm,
			       next->pid,
			       next->prio);

		tsk_data = get_tsk_data(len + 1);
		if (!tsk_data) {
			pr_err("rvh_schedule get tsk_data fail\n");
			return;
		}
		memcpy(tsk_data->dbuf, tbuf, len);
		INIT_LIST_HEAD(&tsk_data->list);
		raw_spin_lock_irqsave(&tsk_list_lock, flags);
		list_add_tail(&tsk_data->list, &tsk_list);
		raw_spin_unlock_irqrestore(&tsk_list_lock, flags);
		tsk_data->prev_pid = prev->pid;
		tsk_data->next_pid = next->pid;
		tsk_data->prev = prev;
		tsk_data->next = next;
		tsk_data->rtime_us = rtime_us;
		tsk_data->nvcsw = prev->nvcsw;
		tsk_data->ncsw = prev->nvcsw + prev->nivcsw;
		tsk_data->sum_exec_runtime = se->sum_exec_runtime;
	}
}

static void system_irq_stat(int print_show)
{
	int i = 0, j = 0;
	int count = 0;
	struct irq_desc *desc;
	struct tsk_data *tsk_data = NULL;
	unsigned long len = 0;
	char *tbuf;

	tsk_data = get_tsk_data(PAGE_SIZE);
	INIT_LIST_HEAD(&tsk_data->list);
	if (!tsk_data) {
		pr_err("%s get tsk_data failed\n", __func__);
		return;
	}
	len += sprintf(tsk_data->dbuf + len, "%30s %10s %10s\n", "irq_name", "irq_id", "irq_count");
	if (print_show)
		pr_info("%s", tsk_data->dbuf);
	for (i = 10; i < 1000; i++, count = 0) {
		desc = irq_to_desc(i);
		if (desc && desc->kstat_irqs && desc->action && desc->action->name) {
			for (j = 0; j < nr_cpu_ids; j++)
				count += *per_cpu_ptr(desc->kstat_irqs, j);
			tbuf = tsk_data->dbuf + len;
			if (count > 0 && len < PAGE_SIZE - 128) {
				len += sprintf(tsk_data->dbuf + len, "%30s %10d %10d\n",
					       desc->action->name,
					       desc->irq_data.domain ? desc->irq_data.hwirq : -1,
					       count);
				if (print_show)
					pr_info("%s", tbuf);
			}
		}
	}
	tbuf = tsk_data->dbuf + len;
	if (gic_base && len < (PAGE_SIZE - 512)) {
		len += sprintf(tsk_data->dbuf + len,
			       "GIC: irq enable regs: 0x%x  0x%x  0x%x  0x%x 0x%x 0x%x\n",
			       readl(gic_base + GIC_DISTRIBUTOR_ENABLE + 0),
			       readl(gic_base + GIC_DISTRIBUTOR_ENABLE + 4),
			       readl(gic_base + GIC_DISTRIBUTOR_ENABLE + 8),
			       readl(gic_base + GIC_DISTRIBUTOR_ENABLE + 12),
			       readl(gic_base + GIC_DISTRIBUTOR_ENABLE + 16),
			       readl(gic_base + GIC_DISTRIBUTOR_ENABLE + 20));
		len += sprintf(tsk_data->dbuf + len,
			       "GIC: irq pending regs: 0x%x  0x%x  0x%x  0x%x 0x%x 0x%x\n",
			       readl(gic_base + GIC_DISTRIBUTOR_PENDING + 0),
			       readl(gic_base + GIC_DISTRIBUTOR_PENDING + 4),
			       readl(gic_base + GIC_DISTRIBUTOR_PENDING + 8),
			       readl(gic_base + GIC_DISTRIBUTOR_PENDING + 12),
			       readl(gic_base + GIC_DISTRIBUTOR_PENDING + 16),
			       readl(gic_base + GIC_DISTRIBUTOR_PENDING + 20));
		len += sprintf(tsk_data->dbuf + len,
			       "GIC: irq active regs: 0x%x  0x%x  0x%x  0x%x 0x%x 0x%x\n",
			       readl(gic_base + GIC_DISTRIBUTOR_ACTIVE + 0),
			       readl(gic_base + GIC_DISTRIBUTOR_ACTIVE + 4),
			       readl(gic_base + GIC_DISTRIBUTOR_ACTIVE + 8),
			       readl(gic_base + GIC_DISTRIBUTOR_ACTIVE + 12),
			       readl(gic_base + GIC_DISTRIBUTOR_ACTIVE + 16),
			       readl(gic_base + GIC_DISTRIBUTOR_ACTIVE + 20));
	}
	move_to_sched_info(tsk_data);
	if (print_show)
		pr_info("len:%d\n%s", len, tbuf);
	return;
}

void cpu_irq_healthiness(void)
{
	system_irq_stat(0);
}

void cpu_sched_healthiness(void)
{
	cpu_stat();
	wake_up_process(aw_healthd);
}

static enum hrtimer_restart healthd_hrtimer_handle(struct hrtimer *hrtimer)
{
	healthd_hrtimer_count++;
	if (healthd_hrtimer_count % 10 == 0)
		cpu_irq_healthiness();
	cpu_sched_healthiness();

	hrtimer_forward_now(hrtimer, ms_to_ktime(detect_period_ms));

	return HRTIMER_RESTART;
}

noinline void free_tsk_data(struct tsk_data *tsk_data)
{
	list_del(&tsk_data->list);
	//raw_spin_lock(&td_free_list_lock);
	if (tsk_data->dlen <= 256) {
		memset(tsk_data->dbuf, 0, tsk_data->dlen);
		list_add_tail(&tsk_data->list, &tsk_data_free_list);
	} else {
		kfree(tsk_data->dbuf);
		kfree(tsk_data);
	}
	//raw_spin_unlock(&td_free_list_lock);
}

noinline void move_to_sched_info(struct tsk_data *tsk_data)
{
	if (!tsk_data->dbuf)
		return;
	list_del(&tsk_data->list);

	list_add_tail(&tsk_data->list, &sched_info_list);
	sched_info_count++;
	if (sched_info_count > sched_info_count_max) {
		tsk_data = container_of(sched_info_list.next, struct tsk_data, list);
		free_tsk_data(tsk_data);
		sched_info_count--;
	}
}

void check_tsk_data(void)
{
	struct tsk_data *tsk_data;
	struct task_struct *tsk;
	struct list_head *curr, *next;
	u64 count = 0, tsk_died = 0, free_data = 0;
	u64 exec_time_delta;
	u64 delta_us;
	ktime_t prev_t;
	unsigned long flags;

	if (list_empty(&tsk_list)) {
		pr_err("tsk list empty!\n");
		return;
	}
	check_tsk_status = 1;

	/* Attention the irq-off time!!! */
	raw_spin_lock_irqsave(&tsk_list_lock, flags);
	prev_t = ktime_get();
	for (curr = tsk_list.next; curr != &tsk_list; curr = next) {
		count++;
		next = curr->next;
		tsk_data = container_of(curr, struct tsk_data, list);
		tsk = find_task_by_vpid(tsk_data->prev_pid);
		if (!tsk) {
			free_tsk_data(tsk_data);
			tsk_died++;
			continue;
		}
		if (tsk_data->ncsw == tsk->nvcsw + tsk->nivcsw
		    || tsk_data->rtime_us > sched_running_thr
		    || (tsk->nvcsw == tsk_data->nvcsw && tsk_data->rtime_us < sched_rt_thr)
		    || (is_rt_tsk(tsk) && tsk_data->rtime_us > sched_rt_thr)) {
			move_to_sched_info(tsk_data);
			continue;
		}
		exec_time_delta = (tsk->se.sum_exec_runtime - tsk_data->sum_exec_runtime)/1000000;
		if ((exec_time_delta * 3 > detect_period_ms) && tsk_data->rtime_us > sched_hl_thr) {
			move_to_sched_info(tsk_data);
		} else {
			free_data++;
			free_tsk_data(tsk_data);
		}
	}
	delta_us = ktime_to_us(ktime_sub(ktime_get(), prev_t));
	pr_debug("total count:%u,print:%d, tsk died:%u, free data:%u, func time:%dus\n",
	       count, count -(tsk_died + free_data),  tsk_died, free_data, delta_us);
	raw_spin_unlock_irqrestore(&tsk_list_lock, flags);
	check_tsk_status = 0;
}

int healthd_thread_work(void *data)
{
	while (!kthread_should_stop()) {
		set_current_state(TASK_RUNNING);
		check_tsk_data();
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule();
	}

	return 0;
}

/*
static ktime_t __percpu *pg_alloc_t;
static int page_alloc_handler_pre(struct kprobe *p, struct pt_regs *regs)
{
	unsigned int order;
	int cpu;
	ktime_t *t;

	order = regs->regs[1];
	if (order > 2) {
		cpu = get_cpu();
		t = per_cpu_ptr(pg_alloc_t, cpu);
		put_cpu();
		*t = ktime_get();
	}
	return 0;
}

static void page_alloc_handler_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
	s64 delta_ms;
	ktime_t now;
	int cpu;
	ktime_t *t;

	cpu = get_cpu();
	t = per_cpu_ptr(pg_alloc_t, cpu);
	put_cpu();
	if (*t) {
		now = ktime_get();
		delta_ms = ktime_to_ms(ktime_sub(now, *t));
		if (delta_ms > alloc_pages_ms) {
			pr_err("kprobe alloc_pages, delta_ms=%dms\n", delta_ms);
			dump_stack();
		}
		*t = 0;
	}
}

void register_alloc_pages_kprobe(void)
{
	struct kprobe *kp;
	int ret;
	register memory alloc kprobe
	pg_alloc_t = __alloc_percpu(nr_cpu_ids * sizeof(ktime_t), sizeof(ktime_t));
	for (i = 0; i < nr_cpu_ids; i++)
		*(per_cpu_ptr(pg_alloc_t, i)) = 0;

	if (pg_alloc_t)
		register_alloc_pages_kprobe();
	else
		pr_err("alloc pg_alloc_t percpu failed\n");

	kp = kmalloc(sizeof(*kp), GFP_KERNEL);
	kp->pre_handler = page_alloc_handler_pre;
	kp->post_handler = page_alloc_handler_post;
	kp->symbol_name = "__alloc_pages";
	ret = register_kprobe(kp);
	if (ret < 0)
		pr_err("register kprobe:%s failed, ret:%d\n", kp->symbol_name, ret);
}
*/

static int ah_register_vendor_hook(void)
{
	int ret, i;

	ret = register_trace_android_vh_watchdog_timer_softlockup(android_vh_watchdog_timer_softlockup, NULL);
	if (ret) {
		pr_err("%s: register watchdog_timer_sofklockup vendor hook failed\n", __func__);
		goto out;
	}

	ret = register_trace_android_vh_alloc_pages_slowpath(android_vh_alloc_pages_slowpath, NULL);
	if (ret) {
		pr_err("%s: register alloc_pages_slowpath vendor hook failed\n");
		goto out;
	} else {
		for (i = 0; i < CPU_NUMS; i++) {
			entries[i] = kzalloc(STACK_SIZE, GFP_KERNEL);
			stackbuf[i] = kzalloc(STACK_SIZE, GFP_KERNEL);
			if (!entries[i] || !stackbuf[i])
				return -ENOMEM;
		}
	}

	ret = register_trace_android_rvh_schedule(android_rvh_schedule, NULL);
	if (ret)
		pr_err("%s: register schedule vendor hook failed\n");

out:
	return ret;
}

static void ah_unregister_vendor_hook(void)
{
	int ret;

	ret = unregister_trace_android_vh_watchdog_timer_softlockup(android_vh_watchdog_timer_softlockup, NULL);
	if (ret)
		pr_err("%s: register watchdog_timer_sofklockup vendor hook failed\n", __func__);

	ret = unregister_trace_android_vh_alloc_pages_slowpath(android_vh_alloc_pages_slowpath, NULL);
	if (ret)
		pr_err("%s: register alloc_pages_slowpath vendor hook failed\n");

	/*
	 * Can't unregister rvh
	ret = unregister_trace_android_rvh_schedule(android_rvh_schedule, NULL);
	if (ret)
		pr_err("%s: register schedule vendor hook failed\n");
	*/
}

static void *start_sched_info(struct seq_file *seq, loff_t *pos)
{
	if (list_empty(&sched_info_list))
		sched_info_pos = NULL;
	if (*pos == 0)
		sched_info_pos = sched_info_list.next;
	if (*pos < sched_info_count_max && *pos < sched_info_count)
		return (void *)((long)*pos + 1);
	return NULL;
}

static void stop_sched_info(struct seq_file *seq, void *v)
{
}

static void *next_sched_info(struct seq_file *seq, void *v, loff_t *pos)
{
	++*pos;
	if (!sched_info_pos)
		return NULL;
	sched_info_pos = sched_info_pos->next;
	if (sched_info_pos == sched_info_list.prev) {
		sched_info_pos = NULL;
		return NULL;
	}
	if (*pos < sched_info_count_max && *pos < sched_info_count)
		return (void *)((long)*pos);
	else
		sched_info_pos = NULL;
	return NULL;
}

static int show_sched_info(struct seq_file *seq, void *v)
{
	struct tsk_data *tsk_data;

	if (!sched_info_pos)
		return 0;
	tsk_data = container_of(sched_info_pos, struct tsk_data, list);
	seq_puts(seq, tsk_data->dbuf);

	return 0;
}

static const struct seq_operations sched_info_seqops = {
	.start = start_sched_info,
	.next = next_sched_info,
	.stop = stop_sched_info,
	.show = show_sched_info,
};

static int open_sched_info(struct inode *inode, struct file *file)
{
	return seq_open(file, &sched_info_seqops);
}

static const struct file_operations sched_info_fops = {
	.open = open_sched_info,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int sched_define_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int sched_define_release(struct inode *inode, struct file *file)
{
	return 0;
}

#define SCHED_DEFINE_DEBUG_FILE(__name)				\
static ssize_t __name ## _read(struct file *file, char __user *buf,\
				 size_t count, loff_t *ppos)	\
{								\
	char tbuf[32];						\
	unsigned long len;					\
	len = sprintf(tbuf, "%d\n", __name);			\
	if (len) {						\
		if (*ppos >= len)				\
			return 0;				\
		if (count >= len)				\
			count = len;				\
		if (count > (len - *ppos))			\
			count = (len - *ppos);			\
		if (copy_to_user((void __user *)buf, tbuf, (unsigned long)len)) { \
			pr_warn("copy_to_user fail\n");		\
			return 0;				\
		}						\
		*ppos += count;					\
	} else							\
		count = 0;					\
	return count;						\
}								\
static ssize_t __name ## _write(struct file *file, const char __user *buf, \
				     size_t count, loff_t *ppos)\
{								\
	int err;						\
	unsigned int val;					\
	char tbuf[32];						\
	if (count >= sizeof(tbuf))				\
		return 0;					\
	if (copy_from_user(tbuf, buf, count)) {			\
		pr_warn("copy_from_user fail\n");		\
		return 0;					\
	}							\
	err = kstrtoint(tbuf, 10, &val);			\
	if (err || val > SCHED_INFO_COUNT_MAX)			\
		return 0;					\
	__name = val;						\
	return count;						\
}								\
static const struct file_operations __name ## _fops = {		\
	.write = __name ## _write,				\
	.read = __name ## _read,				\
	.open = sched_define_open,				\
	.release = sched_define_release,			\
}

SCHED_DEFINE_DEBUG_FILE(sched_info_count_max);
SCHED_DEFINE_DEBUG_FILE(sched_running_thr);
SCHED_DEFINE_DEBUG_FILE(sched_hl_thr);
SCHED_DEFINE_DEBUG_FILE(sched_rt_thr);
SCHED_DEFINE_DEBUG_FILE(sched_ivcs_thr);
SCHED_DEFINE_DEBUG_FILE(aw_healthd_enable);
SCHED_DEFINE_DEBUG_FILE(rvh_schedule_enable);
SCHED_DEFINE_DEBUG_FILE(alloc_pages_ms);
SCHED_DEFINE_DEBUG_FILE(detect_period_ms);

static int aw_healthd_init(void)
{
	struct dentry *aw_healthd_dir;
	struct hrtimer *hrtimer = &healthd_hrtimer;
	int ret = 0;

	hrtimer_init(hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer->function = healthd_hrtimer_handle;
	hrtimer_start(hrtimer, ms_to_ktime(detect_period_ms), HRTIMER_MODE_REL_PINNED_HARD);

	ret = ah_register_vendor_hook();
	if (ret)
		goto out;

	aw_healthd = kthread_create(healthd_thread_work, NULL, "aw_healthd");
	if (IS_ERR(aw_healthd)) {
		pr_err("Failed to create aw_healthd thread\n");
		return PTR_ERR(aw_healthd);
	}

	aw_healthd_dir = debugfs_create_dir("aw_healthd", NULL);
	debugfs_create_file("sched_info", 0440, aw_healthd_dir, NULL, &sched_info_fops);
	debugfs_create_file("sched_info_count_max", 0644, aw_healthd_dir, NULL,
			    &sched_info_count_max_fops);
	debugfs_create_file("sched_running_thr", 0644, aw_healthd_dir, NULL,
			    &sched_running_thr_fops);
	debugfs_create_file("sched_hl_thr", 0644, aw_healthd_dir, NULL,
			    &sched_hl_thr_fops);
	debugfs_create_file("sched_rt_thr", 0644, aw_healthd_dir, NULL,
			    &sched_rt_thr_fops);
	debugfs_create_file("sched_ivcs_thr", 0644, aw_healthd_dir, NULL,
			    &sched_ivcs_thr_fops);

	debugfs_create_file("aw_healthd_enable", 0644, aw_healthd_dir, NULL,
			    &aw_healthd_enable_fops);
	debugfs_create_file("rvh_schedule_enable", 0644, aw_healthd_dir, NULL,
			    &rvh_schedule_enable_fops);
	debugfs_create_file("alloc_pages_ms", 0644, aw_healthd_dir, NULL,
			    &alloc_pages_ms_fops);

	debugfs_create_file("detect_period_ms", 0644, aw_healthd_dir, NULL,
			    &detect_period_ms_fops);

	get_gic_base();

out:
	return ret;
}

static void aw_healthd_exit(void)
{
	if (aw_healthd)
		kthread_stop(aw_healthd);
	hrtimer_cancel(&healthd_hrtimer);
	ah_unregister_vendor_hook();
}

module_init(aw_healthd_init);
module_exit(aw_healthd_exit);

MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.1");
MODULE_AUTHOR("henryli<henryli@allwinnertech.com>");
