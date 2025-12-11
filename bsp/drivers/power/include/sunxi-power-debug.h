/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

#ifndef _SUNXI_POWER_DEBUG_H_
#define _SUNXI_POWER_DEBUG_H_

/*------------------------------
 * AW Power Core
 *------------------------------*/
#include "sunxi-power-core.h"

/*------------------------------
 * Debug Filesystem Support
 *------------------------------*/
#include <linux/debugfs.h>
#include <linux/seq_file.h>

/*------------------------------
 * Time Management
 *------------------------------*/
#include <linux/time.h>
#include <linux/clocksource.h>
#include <linux/sched/clock.h>

/*------------------------------
 * Sunxi Logging System
 *------------------------------*/
#include <sunxi-log.h>

#define PMIC_ERR(format, args...)	sunxi_err(NULL, format, ##args)
#define PMIC_WARN(format, args...)	sunxi_warn(NULL, format, ##args)
#define PMIC_INFO(format, args...)	sunxi_info(NULL, format, ##args)
#define PMIC_DEBUG(format, args...)	sunxi_debug(NULL, format, ##args)

#define PMIC_DEV_ERR(dev, format, args...)	sunxi_err(dev, format, ##args)
#define PMIC_DEV_WARN(dev, format, args...)	sunxi_warn(dev, format, ##args)
#define PMIC_DEV_INFO(dev, format, args...)	sunxi_info(dev, format, ##args)
#define PMIC_DEV_DEBUG(dev, format, args...)	sunxi_debug(dev, format, ##args)

#define PMIC_ERR_STD(error_id, format, args...)	sunxi_err_std(NULL, error_id, format, ##args)
#define PMIC_DEV_ERR_STD(error_id, dev, format, args...)	sunxi_err_std(dev, error_id, format, ##args)

#if IS_ENABLED(CONFIG_DEBUG_FS) && IS_ENABLED(CONFIG_AW_POWER_DEBUG)

#define LOG_BUFFER_ENTRIES 1024
#define LOG_BUFFER_ENTRY_SIZE 256

#define LOG_LEVEL_ERROR   0
#define LOG_LEVEL_WARN    1
#define LOG_LEVEL_INFO    2
#define LOG_LEVEL_DEBUG   3

#define SUNXI_POWER_LOG_ERR(dbg, format, args...) \
	sunxi_power_log(dbg, LOG_LEVEL_ERROR, format, ##args)
#define SUNXI_POWER_LOG_WARN(dbg, format, args...) \
	sunxi_power_log(dbg, LOG_LEVEL_WARN, format, ##args)
#define SUNXI_POWER_LOG_INFO(dbg, format, args...) \
	sunxi_power_log(dbg, LOG_LEVEL_INFO, format, ##args)
#define SUNXI_POWER_LOG_DEBUG(dbg, format, args...) \
	sunxi_power_log(dbg, LOG_LEVEL_DEBUG, format, ##args)
#define SUNXI_POWER_LOG_FORCE(dbg, format, args...) \
	sunxi_power_log_force(dbg, format, ##args)

struct sunxi_power_debug_data {
	struct dentry *debug_dir;
	char *logbuffer[LOG_BUFFER_ENTRIES];
	int logbuffer_head;
	int logbuffer_tail;
	struct mutex logbuffer_lock;
	struct device *dev;
	bool force_log;
	int log_level;
	atomic_t dropped_logs;
};

struct sunxi_power_debug_data *sunxi_power_debugfs_init(struct device *dev);
void sunxi_power_debugfs_exit(struct sunxi_power_debug_data *dbg);
void sunxi_power_log(struct sunxi_power_debug_data *dbg, int level, const char *fmt, ...);
void sunxi_power_log_force(struct sunxi_power_debug_data *dbg, const char *fmt, ...);

#else

struct sunxi_power_debug_data {};

static inline struct sunxi_power_debug_data *sunxi_power_debugfs_init(struct device *dev)
{
	return NULL;
}

static inline void sunxi_power_debugfs_exit(struct sunxi_power_debug_data *dbg) { }

#define SUNXI_POWER_LOG_ERR(dbg, format, args...)
#define SUNXI_POWER_LOG_WARN(dbg, format, args...)
#define SUNXI_POWER_LOG_INFO(dbg, format, args...)
#define SUNXI_POWER_LOG_DEBUG(dbg, format, args...)
#define SUNXI_POWER_LOG_FORCE(dbg, format, args...)

#endif

#endif /* _SUNXI_POWER_DEBUG_H_ */