// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM sunxi_uart

#if !defined(__SUNXI_UART_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define __SUNXI_UART_TRACE_H
#include <linux/tracepoint.h>
#include <linux/serial_core.h>
#include "sunxi-uart-ng.h"
#include <linux/tty.h>

DECLARE_EVENT_CLASS(uart_count,
	TP_PROTO(struct sunxi_uart_port *uart_port, int count),
	TP_ARGS(uart_port, count),
	TP_STRUCT__entry(
		__field(unsigned char, id)
		__field(int, count)
	),
	TP_fast_assign(
		__entry->id = uart_port->id;
		__entry->count = count;
	),

	TP_printk("flip buff uart%d count:%d", __entry->id, __entry->count)
);

DEFINE_EVENT(uart_count, uart_count_cpu,
	     TP_PROTO(struct sunxi_uart_port *uart_port, int count),
	     TP_ARGS(uart_port, count)
);
DEFINE_EVENT(uart_count, uart1_count_timer,
	     TP_PROTO(struct sunxi_uart_port *uart_port, int count),
	     TP_ARGS(uart_port, count)
);

DECLARE_EVENT_CLASS(uart_data,
	TP_PROTO(struct sunxi_uart_port *uart_port, int data, int lsr),
	TP_ARGS(uart_port, data, lsr),
	TP_STRUCT__entry(
		__field(unsigned char, id)
		__field(int, data)
		__field(int, lsr)
	),
	TP_fast_assign(
		__entry->id = uart_port->id;
		__entry->data = data;
		__entry->lsr = lsr;
	),

	TP_printk("uart%d data:0x%x, lsr:0x%x", __entry->id, __entry->data, __entry->lsr)
);
DEFINE_EVENT(uart_data, uart_data_rx,
	     TP_PROTO(struct sunxi_uart_port *uart_port, int data, int lsr),
	     TP_ARGS(uart_port, data, lsr)
);

DEFINE_EVENT(uart_data, uart_data_tx,
	     TP_PROTO(struct sunxi_uart_port *uart_port, int data, int lsr),
	     TP_ARGS(uart_port, data, lsr)
);

TRACE_EVENT(uart_irq_status,
	TP_PROTO(struct sunxi_uart_port *uart_port, int iir, int lsr, int tx_cnt, int rx_cnt),
	TP_ARGS(uart_port, iir, lsr, tx_cnt, rx_cnt),
	TP_STRUCT__entry(
		__field(unsigned char, id)
		__field(int, iir)
		__field(int, lsr)
		__field(int, tx_cnt)
		__field(int, rx_cnt)
	),
	TP_fast_assign(
		__entry->id = uart_port->id;
		__entry->iir = iir;
		__entry->lsr = lsr;
		__entry->tx_cnt = tx_cnt;
		__entry->rx_cnt = rx_cnt;
	),

	TP_printk("uart%d iir:0x%x, lsr:0x%x, tx_cnt:0x%x, rx_cnt:0x%x", __entry->id, __entry->iir, __entry->lsr, __entry->tx_cnt, __entry->rx_cnt)
);
#endif

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE sunxi-uart-trace
#include <trace/define_trace.h>
