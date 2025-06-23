/*
 * fdleak_main.c
 *
 * ioctl and leak watchpoint report implementaion for fdleak
 *
 * Copyright (c) 2018-2019 Huawei Technologies Co., Ltd.
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

#include <linux/soc/cix/dst_fdleak.h>

#include <linux/sched.h>
#include <linux/module.h>
#include <asm/ioctls.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>

#define fdleak_err(format, ...)  \
	pr_err("[fdleak]%s %d: " format, __func__, __LINE__, ##__VA_ARGS__)
#define fdleak_info(format, ...)  \
	pr_devel("[fdleak]%s %d: " format, __func__, __LINE__, ##__VA_ARGS__)

struct stack_pid_item {
	struct stack_item items[MAX_STACK_TRACE_COUNT];
	int hit_cnt[MAX_STACK_TRACE_COUNT];
	int diff_cnt;
};
struct fdleak_wp_item {
	enum fdleak_wp_id id;
	char *name;
	int probe_cnt;
	int pid_cnt;
	int pids[MAX_FDLEAK_PID_NUM];
	int is_32bit[MAX_FDLEAK_PID_NUM];
	struct stack_pid_item *list[MAX_FDLEAK_PID_NUM];
};
static struct fdleak_wp_item fdleak_table[] = {
	{ FDLEAK_WP_EVENTFD, "eventfd", MAX_EVENTFD_PROBE_CNT, 0, {0}, {0}, {0} },
	{ FDLEAK_WP_EVENTPOLL, "eventpoll", MAX_EVENTPOLL_PROBE_CNT, 0, {0}, {0}, {0} },
	{ FDLEAK_WP_DMABUF, "dmabuf", MAX_DMABUF_PROBE_CNT, 0, {0}, {0}, {0} },
	{ FDLEAK_WP_SYNCFENCE, "syncfence", MAX_SYNCFENCE_PROBE_CNT, 0, {0}, {0}, {0} },
	{ FDLEAK_WP_SOCKET, "socket", MAX_SOCKET_PROBE_CNT, 0, {0}, {0}, {0} },
	{ FDLEAK_WP_PIPE, "pipe", MAX_PIPE_PROBE_CNT, 0, {0}, {0}, {0} },
	{ FDLEAK_WP_ASHMEM, "ashmem", MAX_ASHMEM_PROBE_CNT, 0, {0}, {0}, {0} },
};

struct stack_frame_user64 {
	const void __user *fp;
	unsigned long long lr;
};
struct stack_frame_user32 {
	unsigned int fp;
	unsigned int sp;
	unsigned int lr;
	unsigned int pc;
};
static unsigned long long stack_entries[FDLEAK_MAX_STACK_TRACE_DEPTH];

#if 0
static unsigned int usertype;
#endif
static int tgid_hiview;

DEFINE_MUTEX(mutex);

static int fdleak_check_parameter(const void __user *argp, struct fdleak_op *op)
{
	if (!argp || !op) {
		fdleak_err("invalid ioctl parameter\n");
		return -1;
	}
	if (copy_from_user((void *)op, argp, sizeof(*op))) {
		fdleak_err("can not copy parameter data from user space\n");
		return -1;
	}
	if (op->magic != FDLEAK_MAGIC) {
		fdleak_err("invalid magic number\n");
		return -1;
	}
	if (op->pid <= 0) {
		fdleak_err("invalid pid\n");
		return -1;
	}

	return 0;
}

static int fdleak_find_pid(int pid, int wpid)
{
	int idx_pid;

	for (idx_pid = 0; idx_pid < fdleak_table[wpid].pid_cnt; idx_pid++) {
		if (fdleak_table[wpid].pids[idx_pid] == pid)
			break;
	}
	if (idx_pid == fdleak_table[wpid].pid_cnt)
		return -1;

	return idx_pid;
}

static int fdleak_insert_pid(int pid, int wpid)
{
	int idx_pid;
	int size;
	void *stack = NULL;

	if ((wpid < FDLEAK_WP_MIN) || (wpid >= FDLEAK_WP_NUM_MAX))
		return -1;
	if (fdleak_table[wpid].pid_cnt >= MAX_FDLEAK_PID_NUM) {
		fdleak_err("%s %s has reach to maximun pid count\n",
			   __func__, fdleak_table[wpid].name);
		return -1;
	}

	idx_pid = fdleak_find_pid(pid, wpid);
	if (idx_pid >= 0) {
		fdleak_err("%s %s is aleady enabled for pid=%d, do nothing\n",
			   __func__, fdleak_table[wpid].name, pid);
	} else {
		size = fdleak_table[wpid].probe_cnt * sizeof(struct stack_pid_item);
		stack = vmalloc(size);
		if (!stack) {
			fdleak_err("%s %s insert for pid=%d vmalloc failed\n",
				   __func__, fdleak_table[wpid].name, pid);
			return -1;
		}
		memset(stack, 0, size);
		fdleak_table[wpid].pids[fdleak_table[wpid].pid_cnt] = pid;
		fdleak_table[wpid].list[fdleak_table[wpid].pid_cnt] = stack;
		fdleak_table[wpid].pid_cnt++;
		fdleak_info
		    ("%s %s for pid=%d insert success\n",
		     __func__, fdleak_table[wpid].name, pid);
	}

	return 0;
}

static int fdleak_remove_pid(int pid, int wpid)
{
	int idx_pid;
	int copy_count;

	if ((wpid < FDLEAK_WP_MIN) || (wpid >= FDLEAK_WP_NUM_MAX))
		return -1;
	if (fdleak_table[wpid].pid_cnt <= 0) {
		fdleak_err("%s %s for pid %d has been disabled\n",
			   __func__, fdleak_table[wpid].name, pid);
		return -1;
	}

	idx_pid = fdleak_find_pid(pid, wpid);
	if (idx_pid < 0) {
		fdleak_err("%s %s is aleady removed for pid=%d, do nothing\n",
			   __func__, fdleak_table[wpid].name, pid);
		return -1;
	}

	if (fdleak_table[wpid].list[idx_pid]) {
		vfree(fdleak_table[wpid].list[idx_pid]);
		fdleak_table[wpid].list[idx_pid] = NULL;
	}
	if (idx_pid < fdleak_table[wpid].pid_cnt - 1) {
		copy_count = fdleak_table[wpid].pid_cnt - idx_pid - 1;
		memmove(&fdleak_table[wpid].pids[idx_pid],
			&fdleak_table[wpid].pids[idx_pid + 1],
			copy_count * sizeof(fdleak_table[wpid].pids[idx_pid]));
		memmove(&fdleak_table[wpid].is_32bit[idx_pid],
			&fdleak_table[wpid].is_32bit[idx_pid + 1],
			copy_count * sizeof(fdleak_table[wpid].is_32bit[idx_pid]));
		memmove(&fdleak_table[wpid].list[idx_pid],
			&fdleak_table[wpid].list[idx_pid + 1],
			copy_count * sizeof(fdleak_table[wpid].list[idx_pid]));
	}
	fdleak_table[wpid].pid_cnt--;
	fdleak_info("%s %s removed for pid=%d successfully\n",
		    __func__, fdleak_table[wpid].name, pid);
	return 0;
}

static void fdleak_cleanup(void)
{
	int idx_pid;
	int wpid;

	for (wpid = FDLEAK_WP_MIN; wpid < FDLEAK_WP_NUM_MAX; wpid++) {
		for (idx_pid = 0; idx_pid < fdleak_table[wpid].pid_cnt; idx_pid++) {
			if (fdleak_table[wpid].list[idx_pid]) {
				vfree(fdleak_table[wpid].list[idx_pid]);
				fdleak_table[wpid].list[idx_pid] = NULL;
			}
		}
		fdleak_table[wpid].pid_cnt = 0;
		memset(fdleak_table[wpid].pids, 0, sizeof(fdleak_table[wpid].pids));
		memset(fdleak_table[wpid].is_32bit, 0, sizeof(fdleak_table[wpid].is_32bit));
	}
	fdleak_err("hiview crash, cleanup kernel fdleak data\n");
}

static int fdleak_process_onoff(struct fdleak_op *op, bool enable)
{
	int wpid;

	if (!op)
		return -1;

	for (wpid = FDLEAK_WP_MIN; wpid < FDLEAK_WP_NUM_MAX; wpid++) {
		if (!(op->wp_mask & (1 << wpid)))
			continue;

		if (fdleak_table[wpid].probe_cnt <= 0) {
			fdleak_err("%s %s is not supported to be probed currently\n",
				   __func__, fdleak_table[wpid].name);
			continue;
		}
		mutex_lock(&mutex);
		if (enable) {
			/* check if pid of hiview changed or not */
			if (current->real_parent &&
			    (tgid_hiview != current->real_parent->tgid)) {
				fdleak_err("hiview tgid old=%d, current=%d",
					   tgid_hiview, current->real_parent->tgid);
				if (tgid_hiview)
					fdleak_cleanup();
				tgid_hiview = current->real_parent->tgid;
			}
			fdleak_insert_pid(op->pid, wpid);
		} else {
			fdleak_remove_pid(op->pid, wpid);
		}

		mutex_unlock(&mutex);
	}
	return 0;
}

static int fdleak_get_stack(void __user *arg)
{
	int i;
	int idx_pid;
	int probe_id;
	struct fdleak_stackinfo *info = NULL;

	if (!arg) {
		fdleak_err("invalid ioctl parameter for FDLEAK_GET_STACK\n");
		return -1;
	}

	info = vmalloc(sizeof(*info));
	if (!info)
		return -1;

	memset(info, 0, sizeof(*info));
	if (copy_from_user((void *)info, arg, sizeof(*info))) {
		fdleak_err("can't copy data from user space for FDLEAK_GET_STACK\n");
		goto err;
	}
	if (info->magic != FDLEAK_MAGIC) {
		fdleak_err("invalid magic number for FDLEAK_GET_STACK\n");
		goto err;
	}
	if (info->pid <= 0) {
		fdleak_err("invalid pid\n");
		goto err;
	}
	if ((info->wpid < FDLEAK_WP_MIN) || (info->wpid >= FDLEAK_WP_NUM_MAX)) {
		fdleak_err("invalid leak watchpoint id\n");
		goto err;
	}
	if (fdleak_table[info->wpid].probe_cnt <= 0) {
		fdleak_err("no probes for this leak watchpoint %s\n",
			   fdleak_table[info->wpid].name);
		goto err;
	}

	mutex_lock(&mutex);
	idx_pid = fdleak_find_pid(info->pid, info->wpid);
	if (idx_pid < 0) {
		fdleak_err("%s %s for pid=%d is not enable\n",
			   __func__, fdleak_table[info->wpid].name, info->pid);
		mutex_unlock(&mutex);
		goto err;
	}

	info->is_32bit = fdleak_table[info->wpid].is_32bit[idx_pid];
	info->probe_cnt = fdleak_table[info->wpid].probe_cnt;
	strncpy(info->wp_name, fdleak_table[info->wpid].name, sizeof(info->wp_name) - 1);
	info->wp_name[sizeof(info->wp_name) - 1] = '\0';

	for (probe_id = 0; probe_id < info->probe_cnt; probe_id++) {
		info->diff_cnt[probe_id] = fdleak_table[info->wpid].list[idx_pid][probe_id].diff_cnt;
		memcpy(info->hit_cnt[probe_id],
		       fdleak_table[info->wpid].list[idx_pid][probe_id].hit_cnt,
		       sizeof(info->hit_cnt[probe_id]));
		for (i = 0; i < info->diff_cnt[probe_id]; i++) {
			memcpy(&info->list[probe_id][i],
			       &fdleak_table[info->wpid].list[idx_pid][probe_id].items[i],
			       sizeof(info->list[probe_id][i]));
		}
	}
	mutex_unlock(&mutex);

	if (copy_to_user(arg, info, sizeof(*info))) {
		fdleak_err("can not copy data to user space for FDLEAK_GET_STACK\n");
		goto err;
	}

	vfree(info);
	return 0;
err:
	vfree(info);
	return -1;
}

long fdleak_ioctl(struct file *file, unsigned int cmd, uintptr_t arg)
{
	long ret = FDLEAK_CMD_INVALID;
	struct fdleak_op op = {0};

	if ((cmd < FDLEAK_ENABLE_WATCH) || (cmd >= FDLEAK_CMD_MAX)) {
		fdleak_err("invalid ioctl parameter\n");
		return ret;
	}

	switch (cmd) {
	case FDLEAK_ENABLE_WATCH:
		memset(&op, 0, sizeof(op));
		if (fdleak_check_parameter((void *)arg, &op))
			return -EINVAL;
		ret = fdleak_process_onoff(&op, true);
		break;

	case FDLEAK_DISABLE_WATCH:
		memset(&op, 0, sizeof(op));
		if (fdleak_check_parameter((void *)arg, &op))
			return -EINVAL;
		ret = fdleak_process_onoff(&op, false);
		break;

	case FDLEAK_GET_STACK:
		ret = fdleak_get_stack((void *)arg);
		break;

	default:
		break;
	}

	return ret;
}
EXPORT_SYMBOL(fdleak_ioctl);

/*
 * the following session are used for save user stack
 * when specific syscall happen
 */
static void save_stack_trace32(struct task_struct *task,
			       struct stack_trace *trace)
{
	trace->entries[trace->nr_entries++] = ULONG_MAX;
}

static void save_stack_trace64(struct task_struct *task,
			       struct stack_trace *trace)
{
	const struct pt_regs *regs = task_pt_regs(task);
	const void __user *fp = NULL;
	unsigned long addr;
	struct stack_frame_user64 frame;

	fp = (const void __user *)regs->regs[29];
	if (trace->nr_entries < trace->max_entries)
		trace->entries[trace->nr_entries++] = regs->pc;

	while (trace->nr_entries < trace->max_entries) {
		memset(&frame, 0, sizeof(frame));
		addr = (unsigned long)fp;
		if (!access_process_vm(task, addr, (void *)&frame, sizeof(frame), 0))
			break;
		if ((unsigned long)fp < regs->sp)
			break;
		if (frame.lr)
			trace->entries[trace->nr_entries++] = frame.lr;
		if (fp == frame.fp)
			break;
		fp = frame.fp;
	}

	if (trace->nr_entries < trace->max_entries)
		trace->entries[trace->nr_entries++] = ULONG_MAX;
}

static bool fdleak_compare_stack(unsigned long long *stack1,
				 unsigned long long *stack2)
{
	int i;

	/* find the last equivalent value in the stack */
	for (i = 0; (i < FDLEAK_MAX_STACK_TRACE_DEPTH) && (stack1[i] == stack2[i]); i++)
		;
	return i == FDLEAK_MAX_STACK_TRACE_DEPTH;
}

static int fdleak_insert_stack(enum fdleak_wp_id wpid, int pid_index, int probe_id,
			       struct stack_trace *stack)
{
	int i;

	if (fdleak_table[wpid].list[pid_index][probe_id].diff_cnt >= MAX_STACK_TRACE_COUNT)
		return -1;

	for (i = 0; i < fdleak_table[wpid].list[pid_index][probe_id].diff_cnt; i++) {
		if (fdleak_compare_stack(fdleak_table[wpid].list[pid_index][probe_id].items[i].stack,
					 (unsigned long long *)stack->entries)) {
			fdleak_table[wpid].list[pid_index][probe_id].hit_cnt[i]++;
			break;
		}
	}

	if (i == fdleak_table[wpid].list[pid_index][probe_id].diff_cnt) {
		fdleak_table[wpid].list[pid_index][probe_id].hit_cnt[i] = 1;
		memcpy(fdleak_table[wpid].list[pid_index][probe_id].items[i].stack,
		       stack->entries, sizeof(stack_entries));
		fdleak_table[wpid].list[pid_index][probe_id].diff_cnt++;
	}

	return 0;
}

int fdleak_report(enum fdleak_wp_id wpid, int probe_id)
{
	int ret = -1;
	int idx_pid;
	struct stack_trace trace;

#if 0
	if (usertype != BETA_USER) {
		if (usertype)
			return -1;
		usertype = get_logusertype_flag();
		if (usertype != BETA_USER)
			return -1;
	}
#endif

	mutex_lock(&mutex);
	idx_pid = fdleak_find_pid(current->tgid, wpid);

	if (idx_pid < 0) {
		mutex_unlock(&mutex);
		return -1;
	}
	if (probe_id >= fdleak_table[wpid].probe_cnt) {
		mutex_unlock(&mutex);
		return -1;
	}

	fdleak_table[wpid].is_32bit[idx_pid] = is_compat_task();
	if (fdleak_table[wpid].list[idx_pid] && current->mm) {
		memset(stack_entries, 0, sizeof(stack_entries));
		trace.nr_entries = 0;
		trace.max_entries = FDLEAK_MAX_STACK_TRACE_DEPTH;
		trace.entries = (unsigned long *)stack_entries;
		trace.skip = 0;

		if (fdleak_table[wpid].is_32bit[idx_pid])
			save_stack_trace32(current, &trace);
		else
			save_stack_trace64(current, &trace);

		ret = fdleak_insert_stack(wpid, idx_pid, probe_id, &trace);
	}
	mutex_unlock(&mutex);

	return ret;
}
EXPORT_SYMBOL(fdleak_report);

static int __init fdleak_init(void)
{
#if 0
	usertype = get_logusertype_flag();
#endif
	return 0;
}

module_init(fdleak_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ioctl and leak watchpoint report implementaion for fdleak");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
