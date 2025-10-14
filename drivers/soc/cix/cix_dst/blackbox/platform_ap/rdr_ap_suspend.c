// SPDX-License-Identifier: GPL-2.0-only
/*
 * Stack tracing support
 *
 * Copyright (C) 2025 CIX Ltd.
 */
#include <linux/suspend.h>
#include <linux/pm_wakeup.h>
#include "include/rdr_ap_adapter.h"
#include "../rdr_print.h"

#define SUSPEND_MAGIC 0x1F2E3D4C
#define MAX_WAKEUP_NAME_LEN 64
#define MAX_WAKEUP_RECORD_NUM 20

struct wakeup_info {
	char name[MAX_WAKEUP_NAME_LEN];
	ktime_t total_time;
	ktime_t max_time;
	ktime_t last_time;
	ktime_t start_prevent_time;
	ktime_t prevent_sleep_time;
	unsigned long event_count;
	unsigned long active_count;
	unsigned long relax_count;
	unsigned long expire_count;
	unsigned long wakeup_count;
	bool active : 1;
	bool autosleep_enabled : 1;
};

struct wakeup {
	int num;
	struct wakeup_info info[MAX_WAKEUP_RECORD_NUM];
};

struct suspend_info {
	u32 magic;
	struct suspend_stats stats;
	struct wakeup wake_info;
};

static struct suspend_info *g_suspend_info;

int suspend_dump_init(struct platform_device *pdev,
		      struct rdr_safemem_pool *pool)
{
	u32 size = sizeof(struct suspend_info);
	struct bbox_mem m_info;

	if (rdr_safemem_alloc(pool, MEMID_SUSPEND_INFO, size, &m_info))
		return -1;

	memset(m_info.vaddr, 0, m_info.size);
	g_suspend_info = m_info.vaddr;
	BB_DBG("suspend addr: %px, size: 0x%llx", m_info.vaddr, m_info.size);
	return 0;
}

static char *suspend_step_name(enum suspend_stat_step step)
{
	switch (step) {
	case SUSPEND_FREEZE:
		return "freeze";
	case SUSPEND_PREPARE:
		return "prepare";
	case SUSPEND_SUSPEND:
		return "suspend";
	case SUSPEND_SUSPEND_NOIRQ:
		return "suspend_noirq";
	case SUSPEND_RESUME_NOIRQ:
		return "resume_noirq";
	case SUSPEND_RESUME:
		return "resume";
	default:
		return "";
	}
}

void ap_suspend_dump(u32 modid, u32 etype)
{
	struct wakeup_source *ws;
	struct wakeup *wake_info;
	unsigned long flags;
	int len = 0;

	if (modid != MODID_AP_SUSPEND_DEVICE_FAIL)
		return;

	if (!g_suspend_info)
		return;

	/*save suspend state*/
	memcpy(&g_suspend_info->stats, &suspend_stats,
	       sizeof(struct suspend_stats));

	/*save wake up info*/
	wake_info = &g_suspend_info->wake_info;
	for_each_wakeup_source(ws) {
		spin_lock_irqsave(&ws->lock, flags);
		len = MIN(MAX_WAKEUP_NAME_LEN - 1, strlen(ws->name));
		memcpy(wake_info->info[wake_info->num].name, ws->name, len);
		wake_info->info[wake_info->num].name[len] = '\0';
		wake_info->info[wake_info->num].total_time = ws->total_time;
		wake_info->info[wake_info->num].max_time = ws->max_time;
		wake_info->info[wake_info->num].last_time = ws->last_time;
		wake_info->info[wake_info->num].start_prevent_time =
			ws->start_prevent_time;
		wake_info->info[wake_info->num].prevent_sleep_time =
			ws->prevent_sleep_time;
		wake_info->info[wake_info->num].event_count = ws->event_count;
		wake_info->info[wake_info->num].active_count = ws->active_count;
		wake_info->info[wake_info->num].relax_count = ws->relax_count;
		wake_info->info[wake_info->num].expire_count = ws->expire_count;
		wake_info->info[wake_info->num].wakeup_count = ws->wakeup_count;
		wake_info->info[wake_info->num].active = ws->active;
		wake_info->info[wake_info->num].autosleep_enabled =
			ws->autosleep_enabled;
		spin_unlock_irqrestore(&ws->lock, flags);
		wake_info->num++;
	}

	g_suspend_info->magic = SUSPEND_MAGIC;
}

static int print_wakeup_info(struct file *fp, struct wakeup_info *info)
{
	ktime_t total_time;
	ktime_t max_time;
	unsigned long active_count;
	ktime_t active_time;
	ktime_t prevent_sleep_time;
	bool err = 0;

	total_time = info->total_time;
	max_time = info->max_time;
	prevent_sleep_time = info->prevent_sleep_time;
	active_count = info->active_count;
	if (info->active) {
		ktime_t now = ktime_get();

		active_time = ktime_sub(now, info->last_time);
		total_time = ktime_add(total_time, active_time);
		if (active_time > max_time)
			max_time = active_time;

		if (info->autosleep_enabled)
			prevent_sleep_time = ktime_add(
				prevent_sleep_time,
				ktime_sub(now, info->start_prevent_time));
	} else {
		active_time = 0;
	}

	rdr_cleartext_print(
		fp, &err,
		"%-12s\t%lu\t\t%lu\t\t%lu\t\t%lu\t\t%lld\t\t%lld\t\t%lld\t\t%lld\t\t%lld\n",
		info->name, active_count, info->event_count, info->wakeup_count,
		info->expire_count, ktime_to_ms(active_time),
		ktime_to_ms(total_time), ktime_to_ms(max_time),
		ktime_to_ms(info->last_time), ktime_to_ms(prevent_sleep_time));

	return 0;
}

static void wakeup_cleartext(struct file *fp, struct wakeup *wake_info)
{
	bool err = 0;
	struct wakeup_info *info;

	rdr_cleartext_print(fp, &err,
			    "name\t\tactive_count\tevent_count\twakeup_count\t"
			    "expire_count\tactive_since\ttotal_time\tmax_time\t"
			    "last_change\tprevent_suspend_time\n");
	for (int i = 0; i < wake_info->num; i++) {
		info = &wake_info->info[i];
		print_wakeup_info(fp, info);
	}
}

static void suspend_stat_cleartext(struct file *fp, struct suspend_stats *stats)
{
	int i, index, last_dev, last_errno, last_step;
	bool err = 0;

	last_dev = stats->last_failed_dev + REC_FAILED_NUM - 1;
	last_dev %= REC_FAILED_NUM;
	last_errno = stats->last_failed_errno + REC_FAILED_NUM - 1;
	last_errno %= REC_FAILED_NUM;
	last_step = stats->last_failed_step + REC_FAILED_NUM - 1;
	last_step %= REC_FAILED_NUM;
	rdr_cleartext_print(fp, &err,
			    "%s: %d\n%s: %d\n%s: %d\n%s: %d\n%s: %d\n"
			    "%s: %d\n%s: %d\n%s: %d\n%s: %d\n%s: %d\n",
			    "success", stats->success, "fail", stats->fail,
			    "failed_freeze", stats->failed_freeze,
			    "failed_prepare", stats->failed_prepare,
			    "failed_suspend", stats->failed_suspend,
			    "failed_suspend_late", stats->failed_suspend_late,
			    "failed_suspend_noirq", stats->failed_suspend_noirq,
			    "failed_resume", stats->failed_resume,
			    "failed_resume_early", stats->failed_resume_early,
			    "failed_resume_noirq", stats->failed_resume_noirq);
	rdr_cleartext_print(fp, &err, "failures:\n  last_failed_dev:\t%-s\n",
			    stats->failed_devs[last_dev]);
	for (i = 1; i < REC_FAILED_NUM; i++) {
		index = last_dev + REC_FAILED_NUM - i;
		index %= REC_FAILED_NUM;
		rdr_cleartext_print(fp, &err, "\t\t\t%-s\n",
				    stats->failed_devs[index]);
	}
	rdr_cleartext_print(fp, &err, "  last_failed_errno:\t%-d\n",
			    stats->errno[last_errno]);
	for (i = 1; i < REC_FAILED_NUM; i++) {
		index = last_errno + REC_FAILED_NUM - i;
		index %= REC_FAILED_NUM;
		rdr_cleartext_print(fp, &err, "\t\t\t%-d\n",
				    stats->errno[index]);
	}
	rdr_cleartext_print(fp, &err, "  last_failed_step:\t%-s\n",
			    suspend_step_name(stats->failed_steps[last_step]));
	for (i = 1; i < REC_FAILED_NUM; i++) {
		index = last_step + REC_FAILED_NUM - i;
		index %= REC_FAILED_NUM;
		rdr_cleartext_print(
			fp, &err, "\t\t\t%-s\n",
			suspend_step_name(stats->failed_steps[index]));
	}
}

int ap_suspend_cleartext(const char *dir_path, u64 log_addr, u32 log_len)
{
	struct ap_eh_root *head = (struct ap_eh_root *)log_addr;
	struct suspend_info *info = NULL;
	struct file *fp;
	bool err = 0;
	bbox_mem mem;

	if (IS_ERR_OR_NULL(head))
		return -1;
	if (rdr_safemem_get(&head->pool, MEMID_SUSPEND_INFO, &mem))
		return -1;
	info = get_addr_from_root(head, mem.vaddr);
	if (IS_ERR_OR_NULL(info))
		return -1;
	if (info->magic != SUSPEND_MAGIC)
		return 0;
	info->magic = 0;

	fp = bbox_cleartext_get_filep(dir_path, "suspend_info");
	if (IS_ERR_OR_NULL(fp))
		return -1;

	suspend_stat_cleartext(fp, &info->stats);
	rdr_cleartext_print(fp, &err, "\n\n");
	wakeup_cleartext(fp, &info->wake_info);

	bbox_cleartext_end_filep(fp);
	memset(g_suspend_info, 0, sizeof(*g_suspend_info));
	return 0;
}
