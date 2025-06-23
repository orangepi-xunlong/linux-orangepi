// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 Huawei Technologies Co., Ltd. All rights reserved.
 */

#define pr_fmt(fmt) "hungtask_user " fmt

#include <linux/cred.h>
#include <linux/sched/debug.h>
#include <linux/profile.h>
#include <linux/sched.h>
#include <linux/syscalls.h>

#include <dfx/hungtask_base.h>

#define CMD_MIN_LEN 3
#define CMD_MAX_LEN 20
#define USERLIST_NUM 10
#define MAX_USER_TIMEOUT 120
#define MAX_SHOW_LEN 512

struct user_item {
	pid_t pid;
	int cur_cnt;
	int panic_cnt;
};

static struct user_item userlist[USERLIST_NUM];
static int userlist_count;
static DEFINE_SPINLOCK(userlist_lock);
static bool is_registered;
static bool need_panic;
static bool need_dump;
static int block_time;
static int block_pid;

static void htuser_show_task(int pid)
{
	struct task_struct *p = NULL;
	unsigned int tsk_state;

	p = pid_task(find_vpid(pid), PIDTYPE_PID);
	if (p == NULL) {
		pr_err("can not find pid %d\n", pid);
		return;
	}

	tsk_state = READ_ONCE(p->__state);

	if (tsk_state & TASK_FROZEN) {
		pr_info("process %d is frozen\n", pid);
		return;
	}
	if (tsk_state == TASK_UNINTERRUPTIBLE) {
		pr_err("UserList_KernelStack start\n");
		sched_show_task(p);
		pr_err("UserList_KernelStack end\n");
	}
}

static void htuser_list_insert(int pid, int count)
{
	spin_lock(&userlist_lock);
	if (userlist_count >= USERLIST_NUM) {
		pr_err("list is full\n");
		spin_unlock(&userlist_lock);
		return;
	}
	userlist[userlist_count].pid = pid;
	userlist[userlist_count].cur_cnt = 0;
	userlist[userlist_count].panic_cnt = count;
	userlist_count++;
	spin_unlock(&userlist_lock);
}

static int htuser_list_remove(int pid)
{
	int i;

	spin_lock(&userlist_lock);
	for (i = 0; i < userlist_count; i++) {
		if (userlist[i].pid == pid) {
			if (i == userlist_count - 1) {
				memset(&userlist[i], 0, sizeof(userlist[i]));
			} else {
				int len = sizeof(userlist[0]) * (userlist_count - i - 1);
				memmove(&userlist[i], &userlist[i + 1], len);
			}
			userlist_count--;
			spin_unlock(&userlist_lock);
			return 0;
		}
	}
	spin_unlock(&userlist_lock);
	return -ENOENT;
}

static void htuser_list_update(void)
{
	int i;

	need_panic = false;
	need_dump = false;
	spin_lock(&userlist_lock);
	for (i = 0; i < userlist_count; i++) {
		userlist[i].cur_cnt++;
		if ((userlist[i].cur_cnt >= userlist[i].panic_cnt) ||
		    (userlist[i].cur_cnt == userlist[i].panic_cnt / 2)) {
			htuser_show_task(userlist[i].pid);
			pr_err("process %d not scheduled for %ds\n",
				userlist[i].pid,
				userlist[i].cur_cnt * HEARTBEAT_TIME);
		}
		if (userlist[i].cur_cnt == userlist[i].panic_cnt) {
			need_dump = true;
			need_panic = true;
			block_time = userlist[i].cur_cnt * HEARTBEAT_TIME;
			block_pid = userlist[i].pid;
		}
	}
	spin_unlock(&userlist_lock);
}

static void htuser_list_kick(int pid)
{
	int i;

	spin_lock(&userlist_lock);
	for (i = 0; i < userlist_count; i++) {
		if (userlist[i].pid == pid) {
			userlist[i].cur_cnt = 0;
			spin_unlock(&userlist_lock);
			return;
		}
	}
	spin_unlock(&userlist_lock);
}

void htuser_post_process_userlist(void)
{
	htuser_list_update();
	if (need_dump) {
		pr_err("print all cpu stack and D state stack\n");
		hungtask_show_state_filter(TASK_UNINTERRUPTIBLE);
	}
	if (need_panic)
		panic("UserList Process %d blocked for %ds causing panic", block_pid, block_time);
}

static int htuser_process_notifier(struct notifier_block *self,
	unsigned long cmd, void *v)
{
	struct task_struct *task = v;

	if (task == NULL)
		return NOTIFY_OK;

	if ((task->tgid == task->pid) && (!htuser_list_remove(task->tgid)))
		pr_err("remove success due to process %d die\n", task->tgid);

	return NOTIFY_OK;
}

static struct notifier_block htuser_process_notify = {
	.notifier_call = htuser_process_notifier,
};

ssize_t htuser_list_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	int i;
	char tmp[MAX_SHOW_LEN] = {0};
	int len = 0;

	len += snprintf(tmp + len, MAX_SHOW_LEN - len,
		"   Pid   Current(sec)  Expired(sec)\n");

	spin_lock(&userlist_lock);
	for (i = 0; i < userlist_count; i++) {
		len += snprintf(tmp + len, MAX_SHOW_LEN - len,
			"%5d    %5d      %5d", userlist[i].pid,
			userlist[i].cur_cnt * HEARTBEAT_TIME,
			userlist[i].panic_cnt * HEARTBEAT_TIME);
		if (len >= MAX_SHOW_LEN) {
			len = MAX_SHOW_LEN - 1;
			break;
		}
	}
	spin_unlock(&userlist_lock);
	pr_info("%s\n", tmp);
	strncpy(buf, tmp, len);

	return len;
}

static int htuser_list_store_on(char *tmp, size_t len, int pid)
{
	unsigned long sec = 0;

	if (kstrtoul(tmp + 3, 10, &sec)) {
		pr_err("invalid timeout value\n");
		return -EINVAL;
	}
	if ((sec > MAX_USER_TIMEOUT) || !sec) {
		pr_err("invalid timeout value, should be in 0-%d\n", MAX_USER_TIMEOUT);
		return -EINVAL;
	}
	if (sec % HEARTBEAT_TIME) {
		pr_err("invalid timeout value, should be devided by %d\n", HEARTBEAT_TIME);
		return -EINVAL;
	}
	pr_info("process %d set to enable, timeout=%ld\n", pid, sec);
	htuser_list_insert(pid, sec / HEARTBEAT_TIME);
	if (!is_registered) {
		profile_event_register(PROFILE_TASK_EXIT,
			&htuser_process_notify);
		is_registered = true;
	}

	return 0;
}

ssize_t htuser_list_store(struct kobject *kobj,
			  struct kobj_attribute *attr,
			  const char *buf, size_t count)
{
	char tmp[CMD_MAX_LEN]; /* on/off/kick */
	size_t len;
	char *p = NULL;
	int pid = current->tgid;
	int uid = current->cred->euid.val;

	if (uid >= 10000)
		pr_err("non-system process %d(uid=%d) can not be added to hungtask userlist\n",
			pid, uid);
	if ((count < CMD_MIN_LEN) || (count > CMD_MAX_LEN)) {
		pr_err("string too long or too short\n");
		return -EINVAL;
	}
	if (!buf)
		return -EINVAL;

	memset(tmp, 0, sizeof(tmp));
	p = memchr(buf, '\n', count);
	len = p ? (size_t)(p - buf) : count;
	strncpy(tmp, buf, len);

	if (strncmp(tmp, "on", CMD_MIN_LEN) == 0) {
		if (htuser_list_store_on(tmp, len, pid))
			return -EINVAL;
	} else if (unlikely(strncmp(tmp, "off", CMD_MIN_LEN) == 0)) {
		pr_info("process %d set to disable\n", pid);
		if (!htuser_list_remove(pid))
			pr_err("remove success duet to process %d call off\n", pid);
	} else if (likely(strncmp(tmp, "kick", CMD_MIN_LEN) == 0)) {
		pr_info("process %d is kicked\n", pid);
		htuser_list_kick(pid);
	} else {
		pr_err("only accept on off or kick\n");
	}
	return (ssize_t)count;
}

