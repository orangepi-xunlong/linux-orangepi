// SPDX-License-Identifier: GPL-2.0-only
// Copyright 2025 Cix Technology Group Co., Ltd.

#include <linux/sched/clock.h>
#include <linux/soc/cix/rdr_platform_ap_ringbuffer.h>
#include "rdr_exception_trace.h"

typedef struct {
	void *addr;
	u32 len;
	spinlock_t lock;
} ap_exception_trace;

static ap_exception_trace g_ap_et = {
	.addr = NULL,
	.len = 0,
};

/*
 * The clear text printing for the reserved debug memory of ap exceptiontrace
 *
 * func args:
 * @dir_path: the file directory of saved clear text
 * @log_addr: the start address of reserved memory for specified core
 * @log_len: the length of reserved memory for specified core
 *
 * return value
 * 0 success
 * -1 failed
 *
 */
int rdr_exception_trace_ap_cleartext_print(const char *dir_path, u64 log_addr,
					   u32 log_len)
{
	struct rdr_ringbuffer *q = NULL;
	struct rdr_exception_trace_s *trace = NULL;
	struct file *fp = NULL;
	bool error = false;
	u32 start, end, i;

	if (!dir_path) {
		BB_ERR("parameter is NULL\n");
		return -1;
	}
	q = (struct rdr_ringbuffer *)(uintptr_t)log_addr;
	if (unlikely(rdr_rbuf_is_invalid(sizeof(*trace), log_len, q))) {
		BB_ERR("fail:check_ringbuffer_invalid\n");
		return -1;
	}

	/* ring buffer is empty, return directly */
	if (rdr_rbuf_is_empty(q)) {
		BB_PN("ring buffer is empty\n");
		return 0;
	}

	/* get the file descriptor from the specified directory path */
	fp = bbox_cleartext_get_filep(dir_path, "exception_trace_ap.txt");
	if (IS_ERR_OR_NULL(fp)) {
		BB_ERR("error:fp 0x%pK\n", fp);
		return -1;
	}

	rdr_cleartext_print(
		fp, &error,
		"slice          reset_core_mask   from_core      exception_type           exception_subtype\n");

	rdr_rbuf_get_start_end(q, &start, &end);
	for (i = start; i <= end; i++) {
		trace = (struct rdr_exception_trace_s *)&q
				->data[(i % q->max_num) * q->field_count];
		rdr_cleartext_print(
			fp, &error, "%-15llu0x%-16llx%-15s%-25s%-25s\n",
			trace->e_32k_time, trace->e_reset_core_mask,
			rdr_get_core_name_by_core(trace->e_from_core),
			rdr_get_exception_type_name(trace->e_exce_type),
			rdr_get_exception_subtype_name(trace->e_exce_type,
					     trace->e_exce_subtype));
	}

	/* the cleaning of specified file descriptor */
	bbox_cleartext_end_filep(fp);

	if (unlikely(error == true))
		return -1;

	return 0;
}

/*
 * to initialize the reserved memory of core AP exception trace
 *
 * func args:
 * @phy_addr: the physical start address of the reserved memory for core AP exception trace
 * @virt_addr: the virtual start address of the reserved memory for core AP exception trace
 * @log_len: the length of the reserved memory for core AP exception trace
 *
 * return value
 * 0 success
 * otherwise failure
 *
 */
int rdr_exception_trace_ap_init(u8 *phy_addr, u8 *virt_addr, u32 log_len)
{
	memset(virt_addr, 0, log_len);

	if (unlikely(exception_trace_buffer_init(virt_addr, log_len)))
		return -1;

	g_ap_et.addr = virt_addr;
	g_ap_et.len = log_len;
	spin_lock_init(&g_ap_et.lock);

	return 0;
}

/*
 * when the exception break out, it's necessary to record it
 *
 * func args:
 * @e_reset_core_mask: notify which core need to be reset, when include
 *  the ap core to be reset that will reboot the whole system
 * @e_from_core: exception triggered from which core
 * @e_exce_type: exception type
 * @e_exce_subtype: exception subtype
 *
 * return value
 * 0 success
 * otherwise failure
 *
 */
int rdr_exception_trace_record_ap(u64 e_reset_core_mask, u64 e_from_core,
				  u32 e_exce_type, u32 e_exce_subtype)
{
	struct rdr_exception_trace_s trace;
	unsigned long lock_flag;

	BB_PR_START();

	if (!rdr_init_done() || IS_ERR_OR_NULL(g_ap_et.addr)) {
		BB_ERR("rdr init faild!\n");
		BB_PR_END();
		return -1;
	}

	trace.e_32k_time = sched_clock();
	trace.e_reset_core_mask = e_reset_core_mask;
	trace.e_from_core = e_from_core;
	trace.e_exce_type = e_exce_type;
	trace.e_exce_subtype = e_exce_subtype;

	spin_lock_irqsave(&g_ap_et.lock, lock_flag);

	rdr_rbuf_write((struct rdr_ringbuffer *)(g_ap_et.addr),
				(u8 *)&trace);

	spin_unlock_irqrestore(&g_ap_et.lock, lock_flag);

	BB_PR_END();

	return 0;
}
