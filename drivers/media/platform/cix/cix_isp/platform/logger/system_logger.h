/* SPDX-License-Identifier: GPL-2.0 */
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

#ifndef __SYSTEM_LOGGER_H__
#define __SYSTEM_LOGGER_H__

#include <linux/stdarg.h>
#include <linux/types.h>

#define LOG_DEBUG 0
#define LOG_INFO 1
#define LOG_NOTICE 2
#define LOG_WARN 3
#define LOG_ERR 4
#define LOG_CRIT 5
#define LOG_NOTHING 6

enum {
	LOG_MODULE_GENERIC = 0,
	LOG_MODULE_COMMON,
	LOG_MODULE_SENSOR,
	LOG_MODULE_ISP,
};

#define SYSTEM_LOG_LEVEL_MAX LOG_NOTHING
#define SYSTEM_LOG_MODULE_MAX 14

#define ACBCAMERA_BUFFER_SIZE 2048

extern uint32_t _acbcamera_output_mask;
extern uint8_t _acbcamera_output_level;
#define _ACBCAMERA_LOG_OUTPUT_LEVEL _acbcamera_output_level
#define _ACBCAMERA_LOG_OUTPUT_MASK _acbcamera_output_mask

#define ACBCAMERA_LOG_ON(level, mask)		\
	((mask & _ACBCAMERA_LOG_OUTPUT_MASK) && \
	(level >= _ACBCAMERA_LOG_OUTPUT_LEVEL))

#define LOG_MODULE 0

void _acbcamera_log_write(const char *const func, const char *const file,
			  const unsigned int line, const uint32_t log_level,
			  const uint32_t log_module, const uint32_t ratelimited,
			  const char *const fmt, ...);

#define ACBCAMERA_LOG_WRITE(level, module, ratelimited, ...)       \
	do {                                                           \
		if (ACBCAMERA_LOG_ON(level, 1 << module))                  \
			_acbcamera_log_write(__func__, __FILE__, __LINE__, \
			level, module, ratelimited, __VA_ARGS__);              \
	} while (0)

#define LOG(level, ...) ACBCAMERA_LOG_WRITE(level, LOG_MODULE, 0, __VA_ARGS__)
#define LOG_RATELIMITED(level, ...) \
	ACBCAMERA_LOG_WRITE(level, LOG_MODULE, 1, __VA_ARGS__)
#endif
