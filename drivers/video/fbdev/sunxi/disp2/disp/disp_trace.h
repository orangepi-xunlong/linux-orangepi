/* Copyright (c) 2014-2017, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#if !defined(_DISP_TRACE_H_) || defined(TRACE_HEADER_MULTI_READ)
#define _DISP_TRACE_H_

#include <linux/stringify.h>
#include <linux/types.h>
#include <linux/tracepoint.h>

#undef  TRACE_SYSTEM
#define TRACE_SYSTEM disp_trace

TRACE_EVENT(tracing_mark_write,
	TP_PROTO(int pid, const char *name, bool trace_begin),
	TP_ARGS(pid, name, trace_begin),
	TP_STRUCT__entry(
		__field(int, pid)
		__string(trace_name, name)
		__field(bool, trace_begin)
		),
	TP_fast_assign(
		__entry->pid = pid;
		__assign_str(trace_name, name);
		__entry->trace_begin = trace_begin;
		),

	TP_printk("%s|%d|%s", __entry->trace_begin ? "B" : "E",
		__entry->pid, __get_str(trace_name))
);

TRACE_EVENT(display_trace_counter,
	TP_PROTO(int pid, char *name, int value),
	TP_ARGS(pid, name, value),
	TP_STRUCT__entry(
		__field(int, pid)
		__string(counter_name, name)
		__field(int, value)
		),
	TP_fast_assign(
		__entry->pid = pid;
		__assign_str(counter_name, name);
		__entry->value = value;
		),
	TP_printk("C|%d|%s|%d",
		__entry->pid,
		__get_str(counter_name), __entry->value)
);

TRACE_EVENT(display_trace_counter2,
	TP_PROTO(int pid, char *name, int id, int value),
	TP_ARGS(pid, name, id, value),
	TP_STRUCT__entry(
		__field(int, pid)
		__string(counter_name, name)
		__field(int, id)
		__field(int, value)
		),
	TP_fast_assign(
		__entry->pid = pid;
		__assign_str(counter_name, name);
		__entry->id = id;
		__entry->value = value;
		),
	TP_printk("C|%d|%s-%d|%d",
		__entry->pid,
		__get_str(counter_name), __entry->id, __entry->value)
);

#define DISP_TRACE_END(name)   trace_tracing_mark_write(current->tgid, name, 0)
#define DISP_TRACE_BEGIN(name) trace_tracing_mark_write(current->tgid, name, 1)
#define DISP_TRACE_FUNC()      DISP_TRACE_BEGIN(__func__)

#define DISP_TRACE_INT(name, value) \
	trace_display_trace_counter(current->tgid, name, value)

/*
 * Using the api in the interrupt context ensures
 * that the systrace information is bound to the same process.
 */
#define __fake_pid__ (0x0624)
#define DISP_TRACE_INT_F(name, value) \
	trace_display_trace_counter(__fake_pid__, name, value)

#define DISP_TRACE_INT2(name, id, value) \
	trace_display_trace_counter2(__fake_pid__, name, id, value)

#endif /* _DISP_TRACE_H_ */

/*
 * This part must be outside protection
 */
#undef  TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#include <trace/define_trace.h>

