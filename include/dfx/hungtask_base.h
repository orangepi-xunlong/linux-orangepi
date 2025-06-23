/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2022 Huawei Technologies Co., Ltd. All rights reserved.
 */

#ifndef DFX_HUNGTASK_BASE_H
#define DFX_HUNGTASK_BASE_H

#include <linux/list.h>
#include <linux/sched.h>
#include <linux/types.h>

#define ENABLE_SHOW_LEN 8
#define WHITELIST_STORE_LEN 400
#define WHITELIST_LEN 61
#define WHITE_LIST 1
#define BLACK_LIST 2
#define HT_ENABLE 1
#define HT_DISABLE 0
#define HEARTBEAT_TIME 3
#define MAX_LOOP_NUM (CONFIG_DEFAULT_HUNG_TASK_TIMEOUT / HEARTBEAT_TIME)
#define ONE_MINUTE (60 / HEARTBEAT_TIME)
#define ONE_AND_HALF_MINUTE (90 / HEARTBEAT_TIME)
#define TWO_MINUTES (120 / HEARTBEAT_TIME)
#define THREE_MINUTES (180 / HEARTBEAT_TIME)
#define TWENTY_SECONDS (21 / HEARTBEAT_TIME)
#define THIRTY_SECONDS (30 / HEARTBEAT_TIME)
#define HUNG_ONE_HOUR (3600 / HEARTBEAT_TIME)
#define HUNG_TEN_MINUTES (600 / HEARTBEAT_TIME)
#define HUNGTASK_REPORT_TIMECOST TWENTY_SECONDS
#define HT_DUMP_IN_PANIC_LOOSE 5
#define HT_DUMP_IN_PANIC_STRICT 2
#define REFRESH_INTERVAL THREE_MINUTES
#define FLAG_DUMP_WHITE (1 << 0)
#define FLAG_DUMP_APP (1 << 1)
#define FLAG_DUMP_NOSCHEDULE (1 << 2)
#define FLAG_DUMP_JANK (1 << 3)
#define FLAG_PANIC (1 << 4)
#define FLAG_PF_FROZEN (1 << 6)
#define TASK_TYPE_IGNORE 0
#define TASK_TYPE_WHITE (1 << 0)
#define TASK_TYPE_APP (1 << 1)
#define TASK_TYPE_JANK (1 << 2)
#define TASK_TYPE_KERNEL (1 << 3)
#define TASK_TYPE_NATIVE (1 << 4)
#define TASK_TYPE_FROZEN (1 << 6)
#define PID_INIT 1
#define PID_KTHREAD 2
#define DEFAULT_WHITE_DUMP_CNT MAX_LOOP_NUM
#define DEFAULT_WHITE_PANIC_CNT MAX_LOOP_NUM
#define HUNG_TASK_UPLOAD_ONCE 1
#define FROZEN_BUF_LEN 1024
#define MAX_REMOVE_LIST_NUM 200
#define HUNGTASK_DOMAIN "KERNEL_VENDOR"
#define HUNGTASK_NAME "HUNGTASK"
#define INIT_FREEZE_NAME "INIT_FREEZE"
#define HUNG_TASK_BATCHING 1024
#define TIME_REFRESH_PIDS 20
#define PID_ERROR (-1)
#define HUNGTASK_EVENT_WHITELIST 1
#define REPORT_MSGLENGTH 200

struct task_item {
	struct rb_node node;
	pid_t pid;
	pid_t tgid;
	char name[TASK_COMM_LEN + 1];
	unsigned long switch_count;
	unsigned int task_type;
	int dump_wa;
	int panic_wa;
	int dump_jank;
	int d_state_time;
	bool isdone_wa;
};

struct hashlist_node {
	pid_t pid;
	struct hlist_node list;
};

struct whitelist_item {
	pid_t pid;
	char name[TASK_COMM_LEN + 1];
};

struct task_hung_upload {
	char name[TASK_COMM_LEN + 1];
	pid_t pid;
	pid_t tgid;
	unsigned int flag;
	int duration;
};

extern unsigned long sysctl_hung_task_timeout_secs;
extern unsigned int sysctl_hung_task_panic;

void do_dump_task(struct task_struct *task);
int dump_task_wa(struct task_item *item, int dump_cnt,
		 struct task_struct *task, unsigned int flag);
void do_show_task(struct task_struct *task, unsigned int flag, int d_state_time);
void hungtask_show_state_filter(unsigned long state_filter);
int htbase_create_sysfs(void);
void htbase_set_panic(int new_did_panic);
void htbase_set_timeout_secs(unsigned long new_hungtask_timeout_secs);
void htbase_check_tasks(unsigned long timeout);
bool hashlist_find(struct hlist_head *head, int count, pid_t tgid);
void hashlist_clear(struct hlist_head *head, int count);
bool hashlist_insert(struct hlist_head *head, int count, pid_t tgid);

#endif /* DFX_HUNGTASK_BASE_H */
