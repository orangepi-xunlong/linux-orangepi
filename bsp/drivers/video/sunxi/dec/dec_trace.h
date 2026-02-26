/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2007-2021 Allwinnertech Co., Ltd.
 *
 * Author: Yajiaz <yajianz@allwinnertech.com>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#undef TRACE_SYSTEM
#define TRACE_INCLUDE_PATH .
#define TRACE_SYSTEM dec_trace

#if !defined(_TRACE_DEC_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_DEC_H

#include <linux/tracepoint.h>

TRACE_EVENT(dec_tracing,
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

#define DEC_TRACE_END(name)   trace_dec_tracing(current->tgid, name, 0)
#define DEC_TRACE_BEGIN(name) trace_dec_tracing(current->tgid, name, 1)
#define DEC_TRACE_FUNC()	  DEC_TRACE_BEGIN(__func__)

TRACE_EVENT(mux_tracing,
	TP_PROTO(const char *name, unsigned int y, unsigned int c, unsigned int field, unsigned int mux, unsigned int vsyncnt),
	TP_ARGS(name, y, c, field, mux, vsyncnt),
	TP_STRUCT__entry(
		__string(trace_name, name)
		__field(unsigned int, y)
		__field(unsigned int, c)
		__field(unsigned int, field)
		__field(unsigned int, mux)
		__field(unsigned int, vsyncnt)
		),
	TP_fast_assign(
		__assign_str(trace_name, name);
		__entry->y = y;
		__entry->c = c;
		__entry->field	 = field;
		__entry->mux	 = mux;
		__entry->vsyncnt = vsyncnt;
		),

	TP_printk("%s: %08x %08x field %d mux %d vsyn %08x",
		__get_str(trace_name), __entry->y, __entry->c, __entry->field, __entry->mux, __entry->vsyncnt)
);

TRACE_EVENT(frame_tracing,
	TP_PROTO(int pid, const char *name, unsigned int fnumber),
	TP_ARGS(pid, name, fnumber),
	TP_STRUCT__entry(
		__field(int, pid)
		__string(trace_name, name)
		__field(unsigned int, fnumber)
		),
	TP_fast_assign(
		__entry->pid = pid;
		__assign_str(trace_name, name);
		__entry->fnumber = fnumber;
		),

	TP_printk("%d-%s: %08x",
		__entry->pid, __get_str(trace_name), __entry->fnumber)
);

#define DEC_TRACE_FRAME(name, fnumber)	trace_frame_tracing(current->tgid, name, fnumber)

TRACE_EVENT(hwreg_tracing,
	TP_PROTO(int pid, const char *name, int id, unsigned int fnumber),
	TP_ARGS(pid, name, id, fnumber),
	TP_STRUCT__entry(
		__field(int, pid)
		__string(trace_name, name)
		__field(int, id)
		__field(unsigned int, fnumber)
		),
	TP_fast_assign(
		__entry->pid = pid;
		__assign_str(trace_name, name);
		__entry->id = id;
		__entry->fnumber = fnumber;
		),

	TP_printk("|%d|%s: [%d] %08x",
		__entry->pid, __get_str(trace_name), __entry->id, __entry->fnumber)
);

#define DEC_TRACE_HWREG(name, id, fnumber)	trace_hwreg_tracing(current->tgid, name, id, fnumber)

TRACE_EVENT(vsync_tracing,
	TP_PROTO(int pid, const char *name, int64_t timestamp),
	TP_ARGS(pid, name, timestamp),
	TP_STRUCT__entry(
		__field(int, pid)
		__string(trace_name, name)
		__field(int64_t, timestamp)
		),
	TP_fast_assign(
		__entry->pid = pid;
		__assign_str(trace_name, name);
		__entry->timestamp = timestamp;
		),

	TP_printk("%d-%s: vsync %lld",
		__entry->pid, __get_str(trace_name), __entry->timestamp)
);

#define DEC_TRACE_VSYNC(name, timestamp)  trace_vsync_tracing(current->tgid, name, timestamp)

TRACE_EVENT(vinfo_tracing,
	TP_PROTO(int pid, const char *name, int id, unsigned int buf, unsigned int vinfo_addr),
	TP_ARGS(pid, name, id, buf, vinfo_addr),
	TP_STRUCT__entry(
		__field(int, pid)
		__string(trace_name, name)
		__field(int, id)
		__field(unsigned int, buf)
		__field(unsigned int, vinfo_addr)
		),
	TP_fast_assign(
		__entry->pid = pid;
		__assign_str(trace_name, name);
		__entry->id = id;
		__entry->buf = buf;
		__entry->vinfo_addr = vinfo_addr;
		),

	TP_printk("|%d|%s: [%d] %08x %08x",
		__entry->pid, __get_str(trace_name), __entry->id, __entry->buf, __entry->vinfo_addr)
);

#define DEC_TRACE_VINFO(name, id, buf, addr)	trace_vinfo_tracing(current->tgid, name, id, buf, addr)


TRACE_EVENT(address_tracing,
	TP_PROTO(int pid, const char *name, unsigned int address),
	TP_ARGS(pid, name, address),
	TP_STRUCT__entry(
		__field(int, pid)
		__string(trace_name, name)
		__field(unsigned int, address)
		),
	TP_fast_assign(
		__entry->pid = pid;
		__assign_str(trace_name, name);
		__entry->address = address;
		),

	TP_printk("|%d|%s: 0x%08x",
		__entry->pid, __get_str(trace_name), __entry->address)
);

#define DEC_TRACE_ADDRESS(name, address) trace_address_tracing(current->tgid, name, address)

TRACE_EVENT(dec_trace_counter,
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

/*
 * Using the api in the interrupt context ensures
 * that the systrace information is bound to the same process.
 */
#define __fake_pid__ (0x0624)
#define DEC_TRACE_INT_F(name, value) \
	trace_dec_trace_counter(__fake_pid__, name, value)

#endif

/* This part must be outside protection */
#include <trace/define_trace.h>
