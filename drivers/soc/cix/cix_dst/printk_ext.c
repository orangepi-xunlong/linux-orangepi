#include <linux/printk.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>
#include <linux/ktime.h>
#include <linux/sched/task_stack.h>
#include <linux/soc/cix/printk_ext.h>

extern raw_spinlock_t *g_logbuf_level_lock_ex;

int printk_level = LOGLEVEL_DEBUG;
int sysctl_printk_level = LOGLEVEL_DEBUG;

#define TRUE 1
#define SECONDS_OF_MINUTE 60
#define BEGIN_YEAR 1900

void plat_log_store_add_time(
	char *logbuf, u32 logsize, u16 *retlen)
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
			BEGIN_YEAR + tm.tm_year,
			tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
			tm.tm_min, tm.tm_sec);
		if (ret < 0) {
			pr_err("%s: snprintf_s failed\n", __func__);
			return;
		}
		*retlen += ret;
	}

	ret = snprintf(logbuf + *retlen, logsize - *retlen - 1,
		"[pid:%d,cpu%u,%s]", current->pid,
		smp_processor_id(), in_irq() ? "in irq" : current->comm);

	if (ret < 0) {
		pr_err("%s: snprintf_s failed\n", __func__);
		return;
	}
	*retlen += ret;
}

int get_printk_level(void)
{
	return printk_level;
}

int get_sysctl_printk_level(int level)
{
	return sysctl_printk_level;
}

/*
 * if loglevel > level, the log will not be saved to memory in no log load
 * log load and factory mode load will not be affected
 */
void printk_level_setup(int level)
{
	pr_alert("%s: %d\n", __func__, level);

	if (level >= LOGLEVEL_EMERG && level <= LOGLEVEL_DEBUG)
		printk_level = level;
}

void sysctl_printk_level_setup(void)
{
	printk_level_setup(sysctl_printk_level);
}
