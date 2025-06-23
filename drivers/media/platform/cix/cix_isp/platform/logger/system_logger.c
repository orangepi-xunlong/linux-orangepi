// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "system_logger.h"
#include <linux/kernel.h>
#include <linux/rtc.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/version.h>

uint8_t _acbcamera_output_level = LOG_INFO;
uint32_t _acbcamera_output_mask = 0xffffffff;

// debug log names for level
const char *const log_level_name[SYSTEM_LOG_LEVEL_MAX] = { "DEBUG", "INFO",
							   "NOTIC", "WARN",
							   "ERR",   "CRIT" };
// debug log names for modules
const char *const log_module_name[SYSTEM_LOG_MODULE_MAX] = { "GENERIC",
								 "COMMON", "SENSOR",
								 "ISP" };

#define SYSTEM_VPRINTF vprintk
#define SYSTEM_PRINTF printk
#define SYSTEM_PRINTF_RATELIMITED printk_ratelimited
#define SYSTEM_SNPRINTF snprintf
#define SYSTEM_VSNPRINTF vsnprintf

static char _acbcamera_logger_buf[ACBCAMERA_BUFFER_SIZE] = { 0 };

static const char *filename_short(const char *filename)
{
	const char *short_name = filename;

	while (*filename != 0) {
		if (*filename == '/' || *filename == '\\')
			short_name = filename + 1;
		filename++;
	}
	return short_name;
}

const char *sys_time_log_cb(void)
{
	static char time_buf[32];
	struct rtc_time tm;

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	struct timespec64 tv;

	ktime_get_real_ts64(&tv);
	rtc_time64_to_tm(tv.tv_sec, &tm);

	snprintf(time_buf, 32, "%4d_%02d_%02d %02d:%02d:%02d.%06ld",
		 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
		 tm.tm_min, tm.tm_sec, tv.tv_nsec / 1000);
#else
	struct timeval tv = { 0 };

	do_gettimeofday(&tv);
	rtc_time_to_tm(tv.tv_sec, &tm);
	snprintf(time_buf, 32, "%4d_%02d_%02d %02d:%02d:%02d.%06ld",
		 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
		 tm.tm_min, tm.tm_sec, tv.tv_usec);
#endif
	return (const char *)time_buf;
}

void _acbcamera_log_write_ext(const char *const func, const char *const file,
				  const unsigned int line, const uint32_t log_level,
				  const uint32_t log_module,
				  const uint32_t ratelimited, const char *const fmt,
				  va_list vaa)
{
	uint32_t size = 0;
	const char *timestamp = NULL;

	timestamp = sys_time_log_cb();
	size = SYSTEM_SNPRINTF(
		_acbcamera_logger_buf, ACBCAMERA_BUFFER_SIZE,
		"%s %5u %5u <%-8s> [%-5s] %s %s (%u): ", timestamp,
		current->pid, current->tgid, log_module_name[log_module],
		log_level_name[log_level], filename_short(file), func, line);
	SYSTEM_VSNPRINTF(_acbcamera_logger_buf + size,
			 ACBCAMERA_BUFFER_SIZE - size, fmt, vaa);

	if (log_level > LOG_DEBUG) {
		if (ratelimited)
			SYSTEM_PRINTF_RATELIMITED("%s", _acbcamera_logger_buf);
		else
			SYSTEM_PRINTF("%s\n", _acbcamera_logger_buf);
	}
}

void _acbcamera_log_write(const char *const func, const char *const file,
			  const unsigned int line, const uint32_t log_level,
			  const uint32_t log_module, const uint32_t ratelimited,
			  const char *const fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	_acbcamera_log_write_ext(func, file, line, log_level, log_module,
				 ratelimited, fmt, va);
	va_end(va);
}
