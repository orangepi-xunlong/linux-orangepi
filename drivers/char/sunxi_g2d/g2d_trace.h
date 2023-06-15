/*
 * Allwinner SoCs g2d driver.
 *
 * Copyright (C) 2020 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#undef TRACE_SYSTEM
#define TRACE_INCLUDE_PATH ../../../drivers/char/sunxi_g2d
#define TRACE_SYSTEM g2d_trace

#if !defined(_TRACE_G2D_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_G2D_H

#include <linux/tracepoint.h>

TRACE_EVENT(g2d_tracing,
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

#define G2D_TRACE_END(name)   trace_g2d_tracing(current->tgid, name, 0)
#define G2D_TRACE_BEGIN(name) trace_g2d_tracing(current->tgid, name, 1)
#define G2D_TRACE_FUNC()      G2D_TRACE_BEGIN(__func__)

#endif

/* This part must be outside protection */
#include <trace/define_trace.h>
