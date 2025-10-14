// SPDX-License-Identifier: GPL-2.0-only
// Copyright 2025 Cix Technology Group Co., Ltd.

#include <linux/printk.h>
#include <linux/sched/task_stack.h>
#include <linux/soc/cix/printk_ext.h>
#include "dst_print.h"

#define TRUE 1
#define SECONDS_OF_MINUTE 60
#define BEGIN_YEAR 1900

void plat_log_store_add_time(char *logbuf, u32 logsize, u16 *retlen)
{
	static unsigned long prev_jffy;
	static unsigned int prejf_init_flag;
	int ret = 0;
	struct tm tm;
	time64_t now = ktime_get_real_seconds();

	if (!prejf_init_flag) {
		prejf_init_flag = true;
		prev_jffy = jiffies;
	}

	time64_to_tm(now, 0, &tm);

	if (time_after(jiffies, prev_jffy + 1 * HZ)) {
		prev_jffy = jiffies;
		ret = snprintf(logbuf, logsize - 1,
			       "[%lu:%.2d:%.2d %.2d:%.2d:%.2d]",
			       BEGIN_YEAR + tm.tm_year, tm.tm_mon + 1,
			       tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		if (ret < 0) {
			DST_ERR("snprintf_s failed\n");
			return;
		}
		*retlen += ret;
	}

	ret = snprintf(logbuf + *retlen, logsize - *retlen - 1,
		       "[pid:%d,cpu%u,%s]", current->pid, smp_processor_id(),
		       in_irq() ? "in irq" : current->comm);

	if (ret < 0) {
		DST_ERR("snprintf_s failed\n");
		return;
	}
	*retlen += ret;
}
