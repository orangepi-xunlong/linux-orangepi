// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 Huawei Technologies Co., Ltd. All rights reserved.
 */

#define pr_fmt(fmt) "hungtask_base " fmt

#include <linux/nmi.h>
#include <linux/delay.h>
#include <linux/freezer.h>
#include <linux/utsname.h>
#include <trace/events/sched.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/sched/debug.h>
#include <linux/suspend.h>
#include <linux/spinlock.h>
#ifdef CONFIG_DFX_ZEROHUNG
#include <dfx/zrhung.h>
#endif
#include <dfx/hungtask_base.h>
#include "hungtask_user.h"

static struct rb_root list_tasks = RB_ROOT;
static DEFINE_SPINLOCK(list_tasks_lock);
static struct hlist_head whitelist[WHITELIST_LEN];
static struct whitelist_item whitetmplist[WHITELIST_LEN];
static bool whitelist_empty = true;
static int remove_cnt;
static struct task_item *remove_list[MAX_REMOVE_LIST_NUM + 1];
static unsigned long __read_mostly hungtask_timeout_secs =
	CONFIG_DEFAULT_HUNG_TASK_TIMEOUT;
static int did_panic;
static unsigned int hungtask_enable = HT_DISABLE;
static unsigned int whitelist_type = WHITE_LIST;
static int whitelist_dump_cnt = DEFAULT_WHITE_DUMP_CNT;
static int whitelist_panic_cnt = DEFAULT_WHITE_PANIC_CNT;
static int appspawn_pid;
static int dump_and_upload;
static int time_since_upload;
static int hung_task_must_panic;
static int report_zrhung_id;
static struct task_hung_upload upload;
static int do_refresh;
static char frozen_buf[FROZEN_BUF_LEN];
static int frozen_used;
static bool frozed_head;
static unsigned long cur_heartbeat;
static struct work_struct send_work;
static char report_buf_text[REPORT_MSGLENGTH];

bool hashlist_find(struct hlist_head *head, int count, pid_t tgid)
{
	struct hashlist_node *hnode = NULL;

	if (count <= 0)
		return false;
	if (hlist_empty(&head[tgid % count]))
		return false;
	hlist_for_each_entry(hnode, &head[tgid % count], list) {
		if (hnode->pid == tgid)
			return true;
	}
	return false;
}

void hashlist_clear(struct hlist_head *head, int count)
{
	int i = 0;
	struct hlist_node *n = NULL;
	struct hashlist_node *hnode = NULL;

	for (i = 0; i < count; i++) {
		hlist_for_each_entry_safe(hnode, n, &head[i], list) {
			hlist_del(&hnode->list);
			kfree(hnode);
			hnode = NULL;
		}
	}
	for (i = 0; i < count; i++)
		INIT_HLIST_HEAD(&head[i]);
}

bool hashlist_insert(struct hlist_head *head, int count, pid_t tgid)
{
	struct hashlist_node *hnode = NULL;

	if (hashlist_find(head, count, tgid))
		return false;
	hnode = kmalloc(sizeof(struct hashlist_node), GFP_ATOMIC);
	if (!hnode)
		return false;
	INIT_HLIST_NODE(&hnode->list);
	hnode->pid = tgid;
	hlist_add_head(&hnode->list, &head[tgid % count]);
	return true;
}

static bool rcu_lock_break(struct task_struct *g, struct task_struct *t)
{
	bool can_cont = false;

	get_task_struct(g);
	get_task_struct(t);
	rcu_read_unlock();
	cond_resched();
	rcu_read_lock();
	can_cont = pid_alive(g) && pid_alive(t);
	put_task_struct(t);
	put_task_struct(g);
	return can_cont;
}

static bool rcu_break(int *max_count, int *batch_count,
		      struct task_struct *g,
		      struct task_struct *t)
{
	if (!(*max_count)--)
		return true;
	if (!--(*batch_count)) {
		*batch_count = HUNG_TASK_BATCHING;
		if (!rcu_lock_break(g, t))
			return true;
	}
	return false;
}

static pid_t get_pid_by_name(const char *name)
{
	int max_count = PID_MAX_LIMIT;
	int batch_count = HUNG_TASK_BATCHING;
	struct task_struct *g = NULL;
	struct task_struct *t = NULL;
	int pid = 0;

	rcu_read_lock();
	do_each_thread(g, t) {
		if (rcu_break(&max_count, &batch_count, g, t))
			goto unlock;
		if (!strncmp(t->comm, name, TASK_COMM_LEN)) {
			pid = t->tgid;
			goto unlock;
		}
	} while_each_thread(g, t);

unlock:
	rcu_read_unlock();
	return pid;
}

static unsigned int get_task_type(pid_t pid, pid_t tgid, struct task_struct *parent)
{
	unsigned int flag = TASK_TYPE_IGNORE;
	/* check tgid of it's parent as PPID */
	if (parent) {
		pid_t ppid = parent->tgid;

		if (ppid == PID_KTHREAD)
			flag |= TASK_TYPE_KERNEL;
		else if (ppid == appspawn_pid)
			flag |= TASK_TYPE_APP;
		else if (ppid == PID_INIT)
			flag |= TASK_TYPE_NATIVE;
	}
	if (!whitelist_empty && hashlist_find(whitelist, WHITELIST_LEN, tgid))
		flag |= TASK_TYPE_WHITE | TASK_TYPE_JANK;

	return flag;
}

static void refresh_appspawn_pids(void)
{
	int max_count = PID_MAX_LIMIT;
	int batch_count = HUNG_TASK_BATCHING;
	struct task_struct *g = NULL;
	struct task_struct *t = NULL;

	rcu_read_lock();
	do_each_thread(g, t) {
		if (rcu_break(&max_count, &batch_count, g, t))
			goto unlock;
		if (!strncmp(t->comm, "appspawn", TASK_COMM_LEN))
			appspawn_pid = t->tgid;
	} while_each_thread(g, t);
unlock:
	rcu_read_unlock();
}

static void refresh_task_type(pid_t pid, int task_type)
{
	struct task_item *item = NULL;
	struct rb_node *p = NULL;

	spin_lock(&list_tasks_lock);
	for (p = rb_first(&list_tasks); p; p = rb_next(p)) {
		item = rb_entry(p, struct task_item, node);
		if (item->tgid == pid)
			item->task_type = task_type;
	}
	spin_unlock(&list_tasks_lock);
}

static void refresh_whitelist_pids(void)
{
	int i;

	hashlist_clear(whitelist, WHITELIST_LEN);
	for (i = 0; i < WHITELIST_LEN; i++) {
		if (!strlen(whitetmplist[i].name))
			continue;
		whitetmplist[i].pid =
			get_pid_by_name(whitetmplist[i].name);
		if (!whitetmplist[i].pid)
			continue;
		refresh_task_type(whitetmplist[i].pid,
			TASK_TYPE_WHITE | TASK_TYPE_JANK);
		if (hashlist_insert(whitelist, WHITELIST_LEN,
			whitetmplist[i].pid))
			pr_info("whitelist[%d]-%s-%d\n", i,
				whitetmplist[i].name, whitetmplist[i].pid);
		else
			pr_info("can't find %s\n", whitetmplist[i].name);
	}
	refresh_appspawn_pids();
}

static struct task_item *find_task(pid_t pid, struct rb_root *root)
{
	struct rb_node **p = &root->rb_node;
	struct task_item *cur = NULL;
	struct rb_node *parent = NULL;

	while (*p) {
		parent = *p;
		cur = rb_entry(parent, struct task_item, node);
		if (!cur)
			return NULL;
		if (pid < cur->pid)
			p = &(*p)->rb_left;
		else if (pid > cur->pid)
			p = &(*p)->rb_right;
		else
			return cur;
	}
	return NULL;
}

static bool insert_task(struct task_item *item, struct rb_root *root)
{
	struct rb_node **p = &root->rb_node;
	struct rb_node *parent = NULL;
	struct task_item *cur = NULL;

	while (*p) {
		parent = *p;

		cur = rb_entry(parent, struct task_item, node);
		if (!cur)
			return false;
		if (item->pid < cur->pid) {
			p = &(*p)->rb_left;
		} else if (item->pid > cur->pid) {
			p = &(*p)->rb_right;
		} else {
			pr_info("insert pid=%d,tgid=%d,name=%s,type=%d fail\n",
				item->pid, item->tgid,
				item->name, item->task_type);
			return false;
		}
	}
	rb_link_node(&item->node, parent, p);
	rb_insert_color(&item->node, root);
	return true;
}

void show_block_task(struct task_item *taskitem, struct task_struct *p)
{
	unsigned long last_arrival;
	unsigned long last_queued;

#ifdef CONFIG_SCHED_INFO
	last_arrival = p->sched_info.last_arrival;
	last_queued = p->sched_info.last_queued;
#else
	last_arrival = 0;
	last_queued = 0;
#endif /* CONFIG_SCHED_INFO */

	if (unlikely(READ_ONCE(p->__state) & TASK_FROZEN)) {
		if (taskitem)
			pr_err("name=%s,PID=%d,tgid=%d,tgname=%s,"
			       "FROZEN for %ds,type=%d,la%lu/lq%lu\n",
			       p->comm, p->pid, p->tgid,
			       p->group_leader->comm,
			       taskitem->d_state_time * HEARTBEAT_TIME,
			       taskitem->task_type,
			       last_arrival, last_queued);
		else
			pr_err("name=%s,PID=%d,tgid=%d,tgname=%s,"
			       "just FROZE,la%lu/lq%lu\n",
			       p->comm, p->pid, p->tgid,
			       p->group_leader->comm,
			       last_arrival, last_queued);
	} else {
		if (taskitem)
			pr_err("name=%s,PID=%d,tgid=%d,prio=%d,cpu=%d,tgname=%s,"
			       "type=%d,blocked for %ds,la%lu/lq%lu\n",
			       taskitem->name, taskitem->pid, p->tgid, p->prio,
			       task_cpu(p), p->group_leader->comm, taskitem->task_type,
			       taskitem->d_state_time * HEARTBEAT_TIME,
			       last_arrival, last_queued);
		else
			pr_err("name=%s,PID=%d,tgid=%d,prio=%d,cpu=%d,"
			       "tgname=%s,la%lu/lq%lu\n",
			       p->comm, p->pid, p->tgid, p->prio, task_cpu(p),
			       p->group_leader->comm,
			       last_arrival, last_queued);

		sched_show_task(p);
	}
}

void htbase_show_state_filter(unsigned long state_filter)
{
	struct task_struct *g = NULL;
	struct task_struct *p = NULL;
	struct task_item *taskitem = NULL;
	unsigned int tsk_state;

#if BITS_PER_LONG == 32
	pr_info("  task                PC stack   pid father\n");
#else
	pr_info("  task                        PC stack   pid father\n");
#endif
	rcu_read_lock();
	for_each_process_thread(g, p) {
		/*
		 * reset the NMI-timeout, listing all files on a slow
		 * console might take a lot of time:
		 */
		touch_nmi_watchdog();
		tsk_state = READ_ONCE(p->__state);
		if ((tsk_state == TASK_RUNNING) || (tsk_state & state_filter)) {
			spin_lock(&list_tasks_lock);
			taskitem = find_task(p->pid, &list_tasks);
			spin_unlock(&list_tasks_lock);
			show_block_task(taskitem, p);
		}
	}
	touch_all_softlockup_watchdogs();
	rcu_read_unlock();
	/* Show locks if hungtask happen */
	if ((state_filter == TASK_UNINTERRUPTIBLE) || !state_filter)
		debug_show_all_locks();
}

void hungtask_show_state_filter(unsigned long state_filter)
{
	pr_err("BinderChain_SysRq start\n");
	htbase_show_state_filter(state_filter);
	pr_err("BinderChain_SysRq end\n");
}

void do_dump_task(struct task_struct *task)
{
	sched_show_task(task);
	debug_show_held_locks(task);
}

void do_show_task(struct task_struct *task, unsigned int flag, int d_state_time)
{
	pr_err("%s, flag=%d\n", __func__, flag);
	rcu_read_lock();
	if (!pid_alive(task)) {
		rcu_read_unlock();
		return;
	}
	if (flag & (FLAG_DUMP_WHITE | FLAG_DUMP_APP)) {
		int cnt = 0;

		trace_sched_process_hang(task);
		cnt = d_state_time;
		pr_err("INFO: task %s:%d tgid:%d blocked for %ds in %s\n",
		       task->comm, task->pid, task->tgid,
		       (HEARTBEAT_TIME * cnt),
		       (flag & FLAG_DUMP_WHITE) ? "whitelist" : "applist");
		pr_err("      %s %s %.*s\n",
		       print_tainted(), init_utsname()->release,
		       (int)strcspn(init_utsname()->version, " "),
		       init_utsname()->version);
		do_dump_task(task);
		touch_nmi_watchdog();
		if (flag & FLAG_DUMP_WHITE && (!dump_and_upload)) {
			dump_and_upload++;
			upload.pid = task->pid;
			upload.tgid = task->tgid;
			upload.duration = d_state_time;
			memset(upload.name, 0, sizeof(upload.name));
			strncpy(upload.name, task->comm, sizeof(upload.name));
			upload.flag = flag;
			if (READ_ONCE(task->__state) & TASK_FROZEN)
				upload.flag = (upload.flag | FLAG_PF_FROZEN);
		}
	}
	rcu_read_unlock();
}

static void do_panic(void)
{
	if (sysctl_hung_task_panic) {
		trigger_all_cpu_backtrace();
		panic("hungtask: blocked tasks");
	}
}

static void create_taskitem(struct task_item *taskitem,
			    struct task_struct *task)
{
	taskitem->pid = task->pid;
	taskitem->tgid = task->tgid;
	memset(taskitem->name, 0, sizeof(taskitem->name));
	strncpy(taskitem->name, task->comm, sizeof(taskitem->name));
	taskitem->switch_count = task->nvcsw + task->nivcsw;
	taskitem->dump_wa = 0; /* whitelist or applist task dump times */
	taskitem->panic_wa = 0; /* whitelist or applist task panic times */
	taskitem->d_state_time = -1;
	taskitem->isdone_wa = true; /* if task in white or app dealed */
}

static bool refresh_task(struct task_item *taskitem, struct task_struct *task)
{
	bool is_called = false;

	if (taskitem->switch_count != (task->nvcsw + task->nivcsw)) {
		taskitem->switch_count = task->nvcsw + task->nivcsw;
		is_called = true;
		return is_called;
	}
	if (taskitem->task_type & TASK_TYPE_WHITE) {
		taskitem->isdone_wa = false;
		taskitem->dump_wa++;
		taskitem->panic_wa++;
	}
	taskitem->d_state_time++;
	if (READ_ONCE(task->__state) & TASK_FROZEN)
		taskitem->task_type |= TASK_TYPE_FROZEN;
	return is_called;
}

static void remove_list_tasks(struct task_item *item)
{
	rb_erase(&item->node, &list_tasks);
	kfree(item);
}

static void shrink_process_item(struct task_item *item, bool *is_finish)
{
	if (remove_cnt >= MAX_REMOVE_LIST_NUM) {
		int i;

		remove_list[remove_cnt++] = item;
		for (i = 0; i < remove_cnt; i++)
			remove_list_tasks(remove_list[i]);
		remove_cnt = 0;
		*is_finish = false;
	} else {
		remove_list[remove_cnt++] = item;
	}
}

static void shrink_list_tasks(void)
{
	int i;
	bool is_finish = false;
	struct rb_node *n = NULL;
	struct task_item *item = NULL;

	spin_lock(&list_tasks_lock);
	while (!is_finish) {
		is_finish = true;
		for (n = rb_first(&list_tasks); n != NULL; n = rb_next(n)) {
			item = rb_entry(n, struct task_item, node);
			if (!item)
				continue;
			if (item->isdone_wa) {
				shrink_process_item(item, &is_finish);
				if (!is_finish)
					break;
			}
		}
	}
	for (i = 0; i < remove_cnt; i++)
		remove_list_tasks(remove_list[i]);
	remove_cnt = 0;
	spin_unlock(&list_tasks_lock);
}

static void check_parameters(void)
{
	if ((whitelist_dump_cnt < 0) ||
		(whitelist_dump_cnt > DEFAULT_WHITE_DUMP_CNT))
		whitelist_dump_cnt = DEFAULT_WHITE_DUMP_CNT;
	if ((whitelist_panic_cnt <= 0) ||
		(whitelist_panic_cnt > DEFAULT_WHITE_PANIC_CNT))
		whitelist_panic_cnt = DEFAULT_WHITE_PANIC_CNT;
}

static void send_work_handler(struct work_struct *data)
{
#ifdef CONFIG_DFX_ZEROHUNG
	zrhung_send_event(HUNGTASK_DOMAIN, HUNGTASK_NAME,
		report_buf_text);
#endif
}

static void htbase_report_zrhung_event(const char *report_buf_tag)
{
	htbase_show_state_filter(TASK_UNINTERRUPTIBLE);
	pr_err("%s end\n", report_buf_tag);
	schedule_work(&send_work);
	report_zrhung_id++;
}

static void htbase_report_zrhung(unsigned int event)
{
	bool report_load = false;
	char report_buf_tag[REPORT_MSGLENGTH] = {0};
	char report_name[TASK_COMM_LEN + 1] = {0};
	int report_pid = 0;
	int report_hungtime = 0;
	int report_tasktype = 0;

	if (!event)
		return;
	if (event & HUNGTASK_EVENT_WHITELIST) {
		snprintf(report_buf_tag, sizeof(report_buf_tag),
			 "hungtask_whitelist_%d", report_zrhung_id);
		strncpy(report_name, upload.name, TASK_COMM_LEN);
		report_pid = upload.pid;
		report_tasktype = TASK_TYPE_WHITE;
		report_hungtime = whitelist_dump_cnt * HEARTBEAT_TIME;
		report_load = true;
	} else {
		pr_err("No such event report to zerohung!");
	}
	pr_err("%s start\n", report_buf_tag);
	if (event & HUNGTASK_EVENT_WHITELIST)
		pr_err("report HUNGTASK_EVENT_WHITELIST to zrhung\n");
	if (upload.flag & FLAG_PF_FROZEN)
		snprintf(report_buf_text, sizeof(report_buf_text),
			 "Task %s(%s) pid %d type %d blocked %ds.",
			 report_name, "FROZEN", report_pid, report_tasktype, report_hungtime);
	else
		snprintf(report_buf_text, sizeof(report_buf_text),
			 "Task %s pid %d type %d blocked %ds.",
			 report_name, report_pid, report_tasktype, report_hungtime);
	if (report_load)
		htbase_report_zrhung_event(report_buf_tag);
}

static int print_frozen_list_item(int pid)
{
	int tmp;

	if (!frozed_head) {
		tmp = snprintf(frozen_buf, FROZEN_BUF_LEN, "%s", "FROZEN Pid:");
		if (tmp < 0)
			return -1;
		frozen_used += min(tmp, FROZEN_BUF_LEN - 1);
		frozed_head = true;
	}
	tmp = snprintf(frozen_buf + frozen_used, FROZEN_BUF_LEN - frozen_used, "%d,",
		pid);
	if (tmp < 0)
		return -1;
	frozen_used += min(tmp, FROZEN_BUF_LEN - frozen_used - 1);
	return frozen_used;
}

int dump_task_wa(struct task_item *item, int dump_cnt,
	struct task_struct *task, unsigned int flag)
{
	int ret = 0;

	if ((item->d_state_time > TWO_MINUTES) &&
		(item->d_state_time % TWO_MINUTES != 0))
		return ret;
	if ((item->d_state_time > HUNG_TEN_MINUTES) &&
		(item->d_state_time % HUNG_TEN_MINUTES != 0))
		return ret;
	if ((item->d_state_time > HUNG_ONE_HOUR) &&
		(item->d_state_time % HUNG_ONE_HOUR != 0))
		return ret;
	if (dump_cnt && (item->dump_wa > dump_cnt)) {
		item->dump_wa = 1;
		if (!dump_and_upload && (READ_ONCE(task->__state) & TASK_FROZEN)) {
			int tmp = print_frozen_list_item(item->pid);
			if (tmp < 0)
				return ret;
			if (tmp >= FROZEN_BUF_LEN - 1) {
				pr_err("%s", frozen_buf);
				memset(frozen_buf, 0, sizeof(frozen_buf));
				frozen_used = 0;
				frozed_head = false;
				print_frozen_list_item(item->pid);
			}
		} else if (!dump_and_upload) {
			pr_err("Ready to dump a task %s\n", item->name);
			do_show_task(task, flag, item->d_state_time);
			ret++;
		}
	}
	return ret;
}

static void update_panic_task(struct task_item *item)
{
	if (upload.pid != 0)
		return;

	upload.pid = item->pid;
	upload.tgid = item->tgid;
	memset(upload.name, 0, sizeof(upload.name));
	strncpy(upload.name, item->name, sizeof(upload.name));
}

static void deal_task(struct task_item *item, struct task_struct *task, bool is_called)
{
	int any_dumped_num = 0;

	if (is_called) {
		item->dump_wa = 1;
		item->panic_wa = 1;
		item->d_state_time = 0;
		return;
	}
	if (item->task_type & TASK_TYPE_WHITE)
		any_dumped_num = dump_task_wa(item, whitelist_dump_cnt, task,
					      FLAG_DUMP_WHITE);
	if (!is_called && (item->task_type & TASK_TYPE_WHITE)) {
		if (whitelist_panic_cnt && item->panic_wa > whitelist_panic_cnt) {
			pr_err("Task %s is causing panic\n", item->name);
			update_panic_task(item);
			item->panic_wa = 0;
			hung_task_must_panic++;
		} else {
			item->isdone_wa = false;
		}
	}
	if (item->isdone_wa)
		remove_list_tasks(item);
}

static bool check_conditions(struct task_struct *task, unsigned int task_type)
{
	bool no_check = true;

	if (READ_ONCE(task->__state) & TASK_FROZEN)
		return no_check;
	if (task_type & TASK_TYPE_WHITE &&
		(whitelist_dump_cnt || whitelist_panic_cnt))
		no_check = false;
	return no_check;
}

static void htbase_check_one_task(struct task_struct *t)
{
	unsigned int task_type = TASK_TYPE_IGNORE;
	unsigned long switch_count = t->nvcsw + t->nivcsw;
	struct task_item *taskitem = NULL;
	bool is_called = false;

	if (unlikely(!switch_count)) {
		pr_info("skip one's switch_count is zero\n");
		return;
	}

	taskitem = find_task(t->pid, &list_tasks);
	if (taskitem) {
		if (check_conditions(t, taskitem->task_type))
			return;
		is_called = refresh_task(taskitem, t);
	} else {
		task_type = get_task_type(t->pid, t->tgid, t->real_parent);
		if (check_conditions(t, task_type))
			return;
		taskitem = kmalloc(sizeof(*taskitem), GFP_ATOMIC);
		if (!taskitem) {
			pr_err("kmalloc failed");
			return;
		}
		memset(taskitem, 0, sizeof(*taskitem));
		taskitem->task_type = task_type;
		create_taskitem(taskitem, t);
		is_called = refresh_task(taskitem, t);
		insert_task(taskitem, &list_tasks);
	}
	deal_task(taskitem, t, is_called);
}

static void htbase_pre_process(void)
{
	htbase_set_timeout_secs(sysctl_hung_task_timeout_secs);
	cur_heartbeat++;
	if ((cur_heartbeat % REFRESH_INTERVAL) == 0)
		do_refresh = 1;
	else
		do_refresh = 0;
	if (do_refresh || (cur_heartbeat < TIME_REFRESH_PIDS)) {
		refresh_whitelist_pids();
		check_parameters();
	}
}

static void htbase_post_process(void)
{
	struct rb_node *n = NULL;
	unsigned int hungevent = 0;

	if (frozen_used) {
		pr_err("%s", frozen_buf);
		memset(frozen_buf, 0, sizeof(frozen_buf));
		frozen_used = 0;
		frozed_head = false;
	}
	if (dump_and_upload == HUNG_TASK_UPLOAD_ONCE) {
		hungevent |= HUNGTASK_EVENT_WHITELIST;
		dump_and_upload++;
	}
	if (dump_and_upload > 0) {
		time_since_upload++;
		if (time_since_upload > (whitelist_panic_cnt - whitelist_dump_cnt)) {
			dump_and_upload = 0;
			time_since_upload = 0;
		}
	}
	if (hung_task_must_panic) {
		htbase_show_state_filter(TASK_UNINTERRUPTIBLE);
		hung_task_must_panic = 0;
		pr_err("Task %s:%d blocked for %ds is causing panic\n",
		       upload.name, upload.pid,
		       whitelist_panic_cnt * HEARTBEAT_TIME);
		do_panic();
	}
	htuser_post_process_userlist();
	shrink_list_tasks();
	for (n = rb_first(&list_tasks); n != NULL; n = rb_next(n)) {
		struct task_item *item = rb_entry(n, struct task_item, node);
		item->isdone_wa = true;
	}

	if (hungevent)
		htbase_report_zrhung(hungevent);
}

void htbase_check_tasks(unsigned long timeout)
{
	int max_count = PID_MAX_LIMIT;
	int batch_count = HUNG_TASK_BATCHING;
	struct task_struct *g = NULL;
	struct task_struct *t = NULL;
	unsigned int tsk_state;

	if (!hungtask_enable)
		return;
	if (test_taint(TAINT_DIE) || did_panic) {
		pr_err("already in doing panic\n");
		return;
	}

	htbase_pre_process();
	rcu_read_lock();
	for_each_process_thread(g, t) {
		if (!max_count--)
			goto unlock;
		if (!--batch_count) {
			batch_count = HUNG_TASK_BATCHING;
			if (!rcu_lock_break(g, t))
				goto unlock;
		}
		tsk_state = READ_ONCE(t->__state);
		if ((tsk_state == TASK_UNINTERRUPTIBLE) ||
		    (tsk_state == TASK_KILLABLE)) {
			htbase_check_one_task(t);
		}
	}
unlock:
	rcu_read_unlock();
	htbase_post_process();
}

static ssize_t htbase_enable_show(struct kobject *kobj,
				  struct kobj_attribute *attr,
				  char *buf)
{
	if (hungtask_enable)
		return snprintf(buf, ENABLE_SHOW_LEN, "on\n");
	else
		return snprintf(buf, ENABLE_SHOW_LEN, "off\n");
}

static ssize_t htbase_enable_store(struct kobject *kobj,
				   struct kobj_attribute *attr,
				   const char *buf, size_t count)
{
	char tmp[6]; /* only storage "on" "off" "kick" and enter */
	size_t len;
	char *p = NULL;

	if (!buf)
		return -EINVAL;
	if ((count < 2) || (count > (sizeof(tmp) - 1))) {
		pr_err("string too long or too short\n");
		return -EINVAL;
	}

	p = memchr(buf, '\n', count);
	len = p ? (size_t)(p - buf) : count;
	memset(tmp, 0, sizeof(tmp));
	strncpy(tmp, buf, len);
	if (!strncmp(tmp, "on", strlen(tmp))) {
		hungtask_enable = HT_ENABLE;
		pr_info("set hungtask_enable to enable\n");
	} else if (!strncmp(tmp, "off", strlen(tmp))) {
		hungtask_enable = HT_DISABLE;
		pr_info("set hungtask_enable to disable\n");
	} else {
		pr_err("only accept on or off\n");
	}
	return (ssize_t) count;
}

static ssize_t htbase_monitorlist_show(struct kobject *kobj,
				       struct kobj_attribute *attr,
				       char *buf)
{
	int i;
	char *start = buf;
	char all_buf[WHITELIST_STORE_LEN - 20];	/* exclude extra header len 20*/
	unsigned long len = 0;

	memset(all_buf, 0, sizeof(all_buf));
	for (i = 0; i < WHITELIST_LEN; i++) {
		if (whitetmplist[i].pid > 0) {
			len += snprintf(all_buf + len, sizeof(all_buf) - len,
					"%s-%d,", whitetmplist[i].name, whitetmplist[i].pid);
			if (!(len < sizeof(all_buf))) {
				len = sizeof(all_buf) - 1;
				break;
			}
		}
	}
	if (len > 0)
		all_buf[len] = 0;
	if (whitelist_type == WHITE_LIST)
		buf += snprintf(buf, WHITELIST_STORE_LEN, "whitelist:[%s]\n", all_buf);
	else if (whitelist_type == BLACK_LIST)
		buf += snprintf(buf, WHITELIST_STORE_LEN, "blacklist:[%s]\n", all_buf);
	else
		buf += snprintf(buf, WHITELIST_STORE_LEN, "\n");
	return buf - start;
}

static void htbase_monitorlist_update(char **cur)
{
	int index = 0;
	char *token = NULL;

	hashlist_clear(whitelist, WHITELIST_LEN);
	memset(whitetmplist, 0, sizeof(whitetmplist));
	/* generate the new whitelist */
	for (; ; ) {
		token = strsep(cur, ",");
		if (token && strlen(token)) {
			strncpy(whitetmplist[index].name, token, TASK_COMM_LEN);
			if (strlen(whitetmplist[index].name) > 0)
				whitelist_empty = false;
			index++;
			if (index >= WHITELIST_LEN)
				break;
		}
		if (!(*cur))
			break;
	}
}

/*
 * monitorlist_store    -  Called when 'write/echo' method is
 * used on entry '/sys/kernel/hungtask/monitorlist'.
 */
static ssize_t htbase_monitorlist_store(struct kobject *kobj,
					struct kobj_attribute *attr,
					const char *buf, size_t n)
{
	size_t len;
	char *p = NULL;
	char all_buf[WHITELIST_STORE_LEN];
	char *cur = all_buf;


	if ((n < 2) || (n > (sizeof(all_buf) - 1))) {
		pr_err("whitelist input string illegal\n");
		return -EINVAL;
	}
	if (!buf)
		return -EINVAL;
	/*
	 * input format:
	 * write /sys/kernel/hungtask/monitorlist "whitelist,
	 * system_server,surfaceflinger"
	 */
	p = memchr(buf, '\n', n);
	len = p ? (size_t)(p - buf) : n; /* exclude the '\n' */

	memset(all_buf, 0, sizeof(all_buf));
	len =  len > WHITELIST_STORE_LEN ? WHITELIST_STORE_LEN : len;
	strncpy(all_buf, buf, len);
	p = strsep(&cur, ",");
	if (!cur) {
		pr_err("string is not correct\n");
		return -EINVAL;
	}
	if (!strncmp(p, "whitelist", n)) {
		whitelist_type = WHITE_LIST;
	} else {
		if (!strncmp(p, "blacklist", n))
			pr_err("blacklist is not support\n");
		else
			pr_err("wrong list type is set\n");
		return -EINVAL;
	}
	if (!strlen(cur)) {
		pr_err("at least one process need to be set\n");
		return -EINVAL;
	}
	pr_err("whitelist is %s\n", cur);

	htbase_monitorlist_update(&cur);
	/* check again in case user input "whitelist,,,,,," */
	if (whitelist_empty) {
		pr_err("at least one process need to be set\n");
		return -EINVAL;
	}
	return (ssize_t) n;
}

/* used for sysctl at "/proc/sys/kernel/hung_task_timeout_secs" */
void htbase_set_timeout_secs(unsigned long new_hungtask_timeout_secs)
{
	if ((new_hungtask_timeout_secs > CONFIG_DEFAULT_HUNG_TASK_TIMEOUT) ||
		(new_hungtask_timeout_secs % HEARTBEAT_TIME))
		return;
	hungtask_timeout_secs = new_hungtask_timeout_secs;
	/*
	 * if user change panic timeout value, we sync it to dump value
	 * defaultly, user can set it diffrently
	 */
	whitelist_panic_cnt = (int)(hungtask_timeout_secs / HEARTBEAT_TIME);
	if (whitelist_panic_cnt > THIRTY_SECONDS)
		whitelist_dump_cnt = whitelist_panic_cnt / HT_DUMP_IN_PANIC_LOOSE;
	else
		whitelist_dump_cnt = whitelist_panic_cnt / HT_DUMP_IN_PANIC_STRICT;
}

void htbase_set_panic(int new_did_panic)
{
	did_panic = new_did_panic;
}

static struct kobj_attribute timeout_attribute = {
	.attr = {
		 .name = "enable",
		 .mode = 0640,
	},
	.show = htbase_enable_show,
	.store = htbase_enable_store,
};

static struct kobj_attribute monitorlist_attr = {
	.attr = {
		 .name = "monitorlist",
		 .mode = 0640,
	},
	.show = htbase_monitorlist_show,
	.store = htbase_monitorlist_store,
};

#ifdef CONFIG_DFX_HUNGTASK_USER
static struct kobj_attribute userlist_attr = {
	.attr = {
		 .name = "userlist",
		 .mode = 0640,
	},
	.show = htuser_list_show,
	.store = htuser_list_store,
};
#endif

static struct attribute *attrs[] = {
	&timeout_attribute.attr,
	&monitorlist_attr.attr,
#ifdef CONFIG_DFX_HUNGTASK_USER
	&userlist_attr.attr,
#endif
	NULL
};

static struct attribute_group hungtask_attr_group = {
	.attrs = attrs,
};

static struct kobject *hungtask_kobj;
int htbase_create_sysfs(void)
{
	int i;
	int ret;

	/* sleep 1000ms and wait /sys/kernel ready */
	while (!kernel_kobj)
		msleep(1000);

	/* Create kobject named "hungtask" located at /sys/kernel/huangtask */
	hungtask_kobj = kobject_create_and_add("hungtask", kernel_kobj);
	if (!hungtask_kobj)
		return -ENOMEM;
	ret = sysfs_create_group(hungtask_kobj, &hungtask_attr_group);
	if (ret)
		kobject_put(hungtask_kobj);

	for (i = 0; i < WHITELIST_LEN; i++)
		INIT_HLIST_HEAD(&whitelist[i]);
	memset(whitetmplist, 0, sizeof(whitetmplist));

	INIT_WORK(&send_work, send_work_handler);

	return ret;
}
