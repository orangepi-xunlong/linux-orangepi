/* SPDX-License-Identifier: GPL-2.0-only */
// Copyright 2025 Cix Technology Group Co., Ltd.

#ifndef __BB_EXCEPTION_TRACE_H__
#define __BB_EXCEPTION_TRACE_H__

#include "../rdr_inner.h"
#include "../rdr_print.h"
#include "../rdr_field.h"
#include <linux/soc/cix/rdr_platform_ap_ringbuffer.h>

#define TICK_PER_SECOND 32768
/* 5minute, 32768 32k time tick per second */
#define MIN_EXCEPTION_TIME_GAP (5 * 60 * TICK_PER_SECOND)

typedef int (*pfn_exception_init_ops)(u8 *phy_addr, u8 *virt_addr, u32 log_len);
typedef int (*pfn_exception_analysis_ops)(
	u64 etime, u8 *addr, u32 len, struct rdr_exception_info_s *exception);

typedef struct {
	pfn_exception_init_ops init; /* init exception trace */
	pfn_exception_analysis_ops analysis; /* analysis exception trace */
	pfn_cleartext_ops cleartext; /* print exception trace */
} rdr_execption_trace_ops;

int exception_trace_buffer_init(u8 *addr, unsigned int size);

/*exception trace for ap*/
int rdr_exception_trace_ap_cleartext_print(const char *dir_path, u64 log_addr,
					   u32 log_len);
int rdr_exception_trace_ap_init(u8 *phy_addr, u8 *virt_addr, u32 log_len);

#endif
