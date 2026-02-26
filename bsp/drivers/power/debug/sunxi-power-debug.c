// SPDX-License-Identifier: GPL-2.0+
/*
 * Debug driver for Type-C power delivery
 * Copyright(c) 2020-2023 Allwinner Technology Co.,Ltd.
 */

#include "sunxi-power-debug.h"

#ifdef CONFIG_DEBUG_FS

static bool __maybe_unused sunxi_power_log_full(struct sunxi_power_debug_data *dbg)
{
	return dbg->logbuffer_tail ==
		(dbg->logbuffer_head + 1) % LOG_BUFFER_ENTRIES;
}

__printf(2, 0)
static void _sunxi_power_log(struct sunxi_power_debug_data *dbg, const char *fmt,
			   va_list args, bool force)
{
	char tmpbuffer[LOG_BUFFER_ENTRY_SIZE];
	u64 ts_nsec = local_clock();
	unsigned long rem_nsec;
	bool buffer_full = sunxi_power_log_full(dbg);

	mutex_lock(&dbg->logbuffer_lock);

	if (dbg->dev) {
		snprintf(tmpbuffer, sizeof(tmpbuffer), "%s ", dev_name(dbg->dev));
	} else {
		tmpbuffer[0] = '\0';
	}

	if (buffer_full) {
		if (!force) {
			atomic_inc(&dbg->dropped_logs);
			mutex_unlock(&dbg->logbuffer_lock);
			return;
		}
		kfree(dbg->logbuffer[dbg->logbuffer_tail]);
		dbg->logbuffer_tail = (dbg->logbuffer_tail + 1) % LOG_BUFFER_ENTRIES;
	}

	if (!dbg->logbuffer[dbg->logbuffer_head]) {
		dbg->logbuffer[dbg->logbuffer_head] =
			kzalloc(LOG_BUFFER_ENTRY_SIZE, GFP_KERNEL);
		if (!dbg->logbuffer[dbg->logbuffer_head]) {
			mutex_unlock(&dbg->logbuffer_lock);
			return;
		}
	}

	vsnprintf(tmpbuffer, sizeof(tmpbuffer), fmt, args);

	rem_nsec = do_div(ts_nsec, 1000000000);
	scnprintf(dbg->logbuffer[dbg->logbuffer_head],
		  LOG_BUFFER_ENTRY_SIZE, "[%5lu.%06lu] %s",
		  (unsigned long)ts_nsec, rem_nsec / 1000,
		  tmpbuffer);
	dbg->logbuffer_head = (dbg->logbuffer_head + 1) % LOG_BUFFER_ENTRIES;

	mutex_unlock(&dbg->logbuffer_lock);
}

void sunxi_power_log_force(struct sunxi_power_debug_data *dbg, const char *fmt, ...)
{
	va_list args;

	if (IS_ERR_OR_NULL(dbg))
		return;

	va_start(args, fmt);
	_sunxi_power_log(dbg, fmt, args, true);
	va_end(args);
}
EXPORT_SYMBOL_GPL(sunxi_power_log_force);

void sunxi_power_log(struct sunxi_power_debug_data *dbg, int level, const char *fmt, ...)
{
	va_list args;

	if (IS_ERR_OR_NULL(dbg))
		return;

	if (level > dbg->log_level) {
		return;
	}

	va_start(args, fmt);
	_sunxi_power_log(dbg, fmt, args, dbg->force_log);
	va_end(args);
}
EXPORT_SYMBOL_GPL(sunxi_power_log);

static int sunxi_power_debug_show(struct seq_file *s, void *v)
{
	struct sunxi_power_debug_data *dbg = s->private;
	int tail;

	seq_printf(s, "Log Level: %d\n", dbg->log_level);
	seq_printf(s, "Force Log: %s\n", dbg->force_log ? "enabled" : "disabled");
	seq_printf(s, "Dropped Logs: %d\n", atomic_read(&dbg->dropped_logs));
	seq_printf(s, "Buffer Usage: %d/%d\n",
		  (dbg->logbuffer_head - dbg->logbuffer_tail + LOG_BUFFER_ENTRIES) % LOG_BUFFER_ENTRIES,
		  LOG_BUFFER_ENTRIES);

	mutex_lock(&dbg->logbuffer_lock);
	tail = dbg->logbuffer_tail;
	while (tail != dbg->logbuffer_head) {
		seq_printf(s, "%s\n", dbg->logbuffer[tail]);
		tail = (tail + 1) % LOG_BUFFER_ENTRIES;
	}
	if (!seq_has_overflowed(s))
		dbg->logbuffer_tail = tail;
	mutex_unlock(&dbg->logbuffer_lock);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(sunxi_power_debug);

struct sunxi_power_debug_data *sunxi_power_debugfs_init(struct device *dev)
{
	struct sunxi_power_debug_data *dbg;
	char name[NAME_MAX];
	static struct dentry *pmic_dir;
	static DEFINE_MUTEX(pmic_dir_lock);

	dbg = kzalloc(sizeof(*dbg), GFP_KERNEL);
	if (!dbg)
		return ERR_PTR(-ENOMEM);

	mutex_init(&dbg->logbuffer_lock);
	dbg->dev = dev;

	mutex_lock(&pmic_dir_lock);
	if (!pmic_dir) {
		pmic_dir = debugfs_create_dir("pmic", NULL);
		if (!pmic_dir) {
			mutex_unlock(&pmic_dir_lock);
			kfree(dbg);
			return ERR_PTR(-ENOMEM);
		}
	}
	mutex_unlock(&pmic_dir_lock);

	snprintf(name, NAME_MAX, "%s", dev_name(dev));
	dbg->debug_dir = debugfs_create_dir(name, pmic_dir);
	if (!dbg->debug_dir) {
		kfree(dbg);
		return ERR_PTR(-ENOMEM);
	}

	debugfs_create_file("log", S_IFREG | 0444, dbg->debug_dir, dbg,
		   &sunxi_power_debug_fops);

	debugfs_create_u32("log_level", 0644, dbg->debug_dir, &dbg->log_level);
	debugfs_create_bool("force_log", 0644, dbg->debug_dir, &dbg->force_log);
	debugfs_create_atomic_t("dropped_logs", 0444, dbg->debug_dir, &dbg->dropped_logs);

	dbg->log_level = true;
	dbg->log_level = LOG_LEVEL_INFO;

	return dbg;
}
EXPORT_SYMBOL_GPL(sunxi_power_debugfs_init);

void sunxi_power_debugfs_exit(struct sunxi_power_debug_data *dbg)
{
	int i;

	if (IS_ERR_OR_NULL(dbg))
		return;

	mutex_lock(&dbg->logbuffer_lock);
	for (i = 0; i < LOG_BUFFER_ENTRIES; i++) {
		kfree(dbg->logbuffer[i]);
		dbg->logbuffer[i] = NULL;
	}
	mutex_unlock(&dbg->logbuffer_lock);

	debugfs_remove_recursive(dbg->debug_dir);
	kfree(dbg);
}
EXPORT_SYMBOL_GPL(sunxi_power_debugfs_exit);

#endif

MODULE_DESCRIPTION("sunxi power debug driver");
MODULE_AUTHOR("xinouyang <xinouyang@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
