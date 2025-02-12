/*
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from Arm Technology (China) Co., Ltd.
 *
 *            (C) COPYRIGHT 2021-2021 Arm Technology (China) Co., Ltd.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from Arm Technology (China) Co., Ltd.
 * 
 * SPDX-License-Identifier: GPL-2.0-only
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 */

#ifndef MVX_LOG_H
#define MVX_LOG_H

/******************************************************************************
 * Includes
 ******************************************************************************/

#include <linux/net.h>
#include <linux/semaphore.h>
#include <linux/types.h>
#include <linux/uio.h>
#include <linux/wait.h>

/******************************************************************************
 * Defines
 ******************************************************************************/

/**
 * Print a log message.
 *
 * @_lg:		Pointer to log group.
 * @_severity:		Severity.
 * @_fmt:		Format string.
 */
#define MVX_LOG_PRINT(_lg, _severity, _fmt, ...)			       \
	do {								       \
		if ((_severity) <= (_lg)->severity) {			       \
			__MVX_LOG_PRINT(_lg, _severity, _fmt, ## __VA_ARGS__); \
		}							       \
	} while (0)

/**
 * Print a log message for a session.
 *
 * @_lg:		Pointer to log group.
 * @_severity:		Severity.
 * @_session:		Pointer to session.
 * @_fmt:		Format string.
 */
#define MVX_LOG_PRINT_SESSION(_lg, _severity, _session, _fmt, ...)	      \
	do {								      \
		if ((_severity) <= (_lg)->severity) {			      \
			__MVX_LOG_PRINT(_lg, _severity, "%px " _fmt, _session, \
					## __VA_ARGS__);		      \
		}							      \
	} while (0)

/**
 * Print binary data.
 *
 * @_lg:		Pointer to log group.
 * @_severity:		Severity.
 * @_vec:		Scatter input vector data.
 * @_count:		_vec array size.
 */
#define MVX_LOG_DATA(_lg, _severity, _vec, _count)			  \
	do {								  \
		if ((_severity) <= (_lg)->severity) {			  \
			(_lg)->drain->data((_lg)->drain, _severity, _vec, \
					   _count);			  \
		}							  \
	} while (0)

/**
 * Check if severity level for log group is enabled.
 *
 * @_lg:		Pointer to log group.
 * @_severity:		Severity.
 */
#define MVX_LOG_ENABLED(_lg, _severity)	\
	((_severity) <= (_lg)->severity)

/**
 * Execute function if log group is enabled.
 *
 * @_lg:		Pointer to log group.
 * @_severity:		Severity.
 * @_exec:		The function to be executed.
 */
#define MVX_LOG_EXECUTE(_lg, _severity, _exec)	       \
	do {					       \
		if (MVX_LOG_ENABLED(_lg, _severity)) { \
			_exec;			       \
		}				       \
	} while (0)

#ifdef MVX_LOG_PRINT_FILE_ENABLE
#define __MVX_LOG_PRINT(_lg, _severity, _fmt, ...)		  \
	((_lg)->drain->print((_lg)->drain, _severity, (_lg)->tag, \
			     _fmt " (%s:%d)",			  \
			     __MVX_LOG_N_ARGS(__VA_ARGS__),	  \
			     ## __VA_ARGS__,			  \
			     mvx_log_strrchr(__FILE__), __LINE__))
#else
#define __MVX_LOG_PRINT(_lg, _severity, _fmt, ...)			\
	((_lg)->drain->print((_lg)->drain, _severity, (_lg)->tag, _fmt,	\
			     __MVX_LOG_N_ARGS(__VA_ARGS__),		\
			     ## __VA_ARGS__))
#endif /* MVX_LOG_PRINT_FILE_ENABLE */

#define __MVX_LOG_N_ARGS(...) \
	__MVX_LOG_COUNT(dummy, ## __VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define __MVX_LOG_COUNT(_0, _1, _2, _3, _4, _5, _6, _7, _8, N, ...) N

/******************************************************************************
 * Types
 ******************************************************************************/

/**
 * enum mvx_log_severity - Severity levels.
 */
enum mvx_log_severity {
	MVX_LOG_PANIC,
	MVX_LOG_ERROR,
	MVX_LOG_WARNING,
	MVX_LOG_INFO,
	MVX_LOG_DEBUG,
	MVX_LOG_VERBOSE,
	MVX_LOG_MAX
};

struct mvx_log_drain;

/**
 * mvx_print_fptr() - Function pointer to output text messages.
 *
 * @drain:		Pointer to drain.
 * @severity:		Severity level.
 * @tag:		Log group tag.
 * @fmt:		Format string.
 * @n_args:		Number of arguments to format string.
 */
typedef void (*mvx_print_fptr)(struct mvx_log_drain *drain,
			       enum mvx_log_severity severity,
			       const char *tag,
			       const char *fmt,
			       const unsigned int n_args,
			       ...);

/**
 * mvx_data_fptr() - Function pointer to output binary data.
 *
 * @drain:		Pointer to drain.
 * @severity:		Severity level.
 * @vec:		Pointer to the buffers that are copied.
 * @count:		The number of vec buffers.
 */
typedef void (*mvx_data_fptr)(struct mvx_log_drain *drain,
			      enum mvx_log_severity severity,
			      struct iovec *vec,
			      size_t count);

/**
 * struct mvx_log_drain - Structure with information about the drain. The drain
 *			  handles the formatting and redirection of the log
 *			  messages.
 * @print:		Print function pointer.
 * @data:		Data function pointer.
 * @dentry:		Debugfs dentry.
 */
struct mvx_log_drain {
	mvx_print_fptr print;
	mvx_data_fptr data;
	struct dentry *dentry;
};

/**
 * struct mvx_log_drain_ram - Structure describing a specialized RAM drain.
 * @base:		Base class.
 * @buf:		Pointer to output buffer.
 * @buffer_size:	Size of the buffer. Must be power of 2.
 * @read_pos:		Read position when a new file handle is opened. Is
 *			updated when the buffer is cleared.
 * @write_pos:		Current write position in RAM buffer.
 * @queue:		Wait queue for blocking IO.
 * @sem:		Semaphore to prevent concurrent writes.
 */
struct mvx_log_drain_ram {
	struct mvx_log_drain base;
	char *buf;
	const size_t buffer_size;
	size_t read_pos;
	size_t write_pos;
	wait_queue_head_t queue;
	struct semaphore sem;
};

/**
 * struct mvx_log_group - Structure describing log group. The log group filters
 *			  which log messages that shall be forwarded to the
 *			  drain.
 * @tag:		Name of log group.
 * @severity:		Severity level.
 * @drain:		Drain.
 * @dentry:		Debugfs dentry.
 */
struct mvx_log_group {
	const char *tag;
	enum mvx_log_severity severity;
	struct mvx_log_drain *drain;
	struct dentry *dentry;
};

/**
 * struct mvx_log - Log class that keeps track of registered groups and drains.
 */
struct mvx_log {
	struct dentry *mvx_dir;
	struct dentry *log_dir;
	struct dentry *drain_dir;
	struct dentry *group_dir;
};

/****************************************************************************
 * Log
 ****************************************************************************/

/**
 * mvx_log_construct() - Log constructor.
 * @log:		Pointer to log.
 * @entry_name:		The name of the directory
 *
 * Return: 0 on success, else error code.
 */
int mvx_log_construct(struct mvx_log *log,
		      const char *entry_name);

/**
 * mvx_log_destruct() - Log destructor.
 * @log:		Pointer to log.
 */
void mvx_log_destruct(struct mvx_log *log);

/****************************************************************************
 * Drain
 ****************************************************************************/

/**
 * mvx_log_drain_dmesg_construct() - Dmesg drain constructor.
 * @drain:		Pointer to drain.
 *
 * Return: 0 on success, else error code.
 */
int mvx_log_drain_dmesg_construct(struct mvx_log_drain *drain);

/**
 * mvx_log_drain_dmesg_destruct() - Dmesg drain destructor.
 * @drain:		Pointer to drain.
 */
void mvx_log_drain_dmesg_destruct(struct mvx_log_drain *drain);

/**
 * mvx_log_drain_add() - Add drain to log.
 * @log:		Pointer to log.
 * @name:		Name of drain.
 * @drain:		Pointer to drain.
 *
 * Return: 0 on success, else error code.
 */
int mvx_log_drain_add(struct mvx_log *log,
		      const char *name,
		      struct mvx_log_drain *drain);

/**
 * mvx_log_drain_ram_construct() - RAM drain constructor.
 * @drain:		Pointer to drain.
 * @print:		Print function pointer.
 * @data:		Data function pointer.
 * @buffer_size:	The size of the RAM drain buffer.
 *
 * Return: 0 on success, else error code.
 */
int mvx_log_drain_ram_construct(struct mvx_log_drain_ram *drain,
				size_t buffer_size);

/**
 * mvx_log_drain_ram_destruct() - RAM drain destructor.
 * @drain:		Pointer to drain.
 */
void mvx_log_drain_ram_destruct(struct mvx_log_drain_ram *drain);

/**
 * mvx_log_drain_ram_add() - Derived function to add RAM drain to log.
 * @log:		Pointer to log.
 * @name:		Name of drain.
 * @drain:		Pointer to drain.
 *
 * Return: 0 on success, else error code.
 */
int mvx_log_drain_ram_add(struct mvx_log *log,
			  const char *name,
			  struct mvx_log_drain_ram *drain);

#ifdef MVX_LOG_FTRACE_ENABLE

/**
 * mvx_log_drain_ftrace_construct() - Ftrace drain constructor.
 * @drain:		Pointer to drain.
 *
 * Return: 0 on success, else error code.
 */
int mvx_log_drain_ftrace_construct(struct mvx_log_drain *drain);

/**
 * mvx_log_drain_ftrace_destruct() - Ftrace drain destructor.
 * @drain:		Pointer to drain.
 */
void mvx_log_drain_ftrace_destruct(struct mvx_log_drain *drain);

#endif /* MVX_LOG_FTRACE_ENABLE */

/****************************************************************************
 * Group
 ****************************************************************************/

/**
 * mvx_log_group_construct() - Group constructor.
 * @group:		Pointer to group.
 * @tag:		Name of the group, to be used in log messages.
 * @severity:		Minimum severity to output log message.
 * @drain:		Pointer to drain.
 */
void mvx_log_group_construct(struct mvx_log_group *group,
			     const char *tag,
			     const enum mvx_log_severity severity,
			     struct mvx_log_drain *drain);

/**
 * mvx_log_group_add() - Add a group with given name to log.
 * @log:		Pointer to log.
 * @name:		Name of group.
 * @group:		Pointer to group.
 *
 * Return: 0 on success, else error code.
 */
int mvx_log_group_add(struct mvx_log *log,
		      const char *name,
		      struct mvx_log_group *group);

/**
 * mvx_log_group_destruct() - Group destructor.
 * @group:		Pointer to group.
 */
void mvx_log_group_destruct(struct mvx_log_group *group);

/**
 * mvx_log_strrchr() - Find last occurrence of '/' in string.
 * @s:		Pointer to string.
 *
 * Return: Pointer to '/'+1, or pointer to begin of string.
 */
const char *mvx_log_strrchr(const char *s);

#endif /* MVX_LOG_H */
