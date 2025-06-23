/*
 * rdr_exception_trace.c
 *
 * blackbox. (kernel run data recorder.)
 *
 * Copyright (c) 2012-2019 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/thread_info.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/kmsg_dump.h>
#include <linux/io.h>
#include <linux/kallsyms.h>
#include <linux/blkdev.h>
#include <linux/soc/cix/util.h>
#include <linux/soc/cix/rdr_pub.h>
#include <linux/soc/cix/rdr_platform_ap_ringbuffer.h>
#include <linux/soc/cix/mntn_dump.h>
#include <mntn_subtype_exception.h>
#include <linux/soc/cix/rdr_platform_ap_hook.h>
#include <linux/soc/cix/rdr_platform.h>
#include "platform_ap/rdr_platform_ap_adapter.h"
#include "rdr_inner.h"
#include "rdr_print.h"
#include "rdr_field.h"


#define TICK_PER_SECOND        32768
#define MIN_EXCEPTION_TIME_GAP (5 * 60 * TICK_PER_SECOND) /* 5minute, 32768 32k time tick per second */

struct exception_core_s {
	u32 offset;
	u32 size;
};

typedef struct {
	struct platform_device *plat_device;
	uint32_t area_num;
	uint32_t area_size[EXCEPTION_TRACE_ENUM];
	struct exception_core_s exception_core[EXCEPTION_TRACE_ENUM];
	struct rdr_register_module_result current_info;
	u8 *trace_addr;
	spinlock_t lock;
} rdr_exceptiontrace_pdev;

static rdr_exceptiontrace_pdev g_et_pdev = { 0 };

/*
 * in the case of reboot reason error reported, we must correct it to the real
 * reboot reason.
 * The way is to traverse each recorded exception trace, select the most early
 * exception.
 * Now only support the ap watchdog reset case, afterword extend to the other
 * popular reset exception case.
 *
 * func args:
 * @etime: the exception break out time
 * @addr: the start address of reserved memory to record the exception trace
 * @len: the length of reserved memory to record the exception trace
 * @exception: to store the corrected real exception
 *
 * return value
 * 0 success
 * otherwise failure
 *
 */
int rdr_exception_analysis_ap(u64 etime, u8 *addr, u32 len,
				struct rdr_exception_info_s *exception)
{
	struct hisiap_ringbuffer_s *q = NULL;
	rdr_exception_trace_t *trace = NULL;
	rdr_exception_trace_t *min_trace = NULL;
	u64 min_etime = etime;
	u32 start, end, i;

	q = (struct hisiap_ringbuffer_s *)addr;
	if (unlikely((q == NULL) ||
		is_ringbuffer_invalid(sizeof(rdr_exception_trace_t), len, q) ||
		(exception == NULL))) {
		BB_PRINT_ERR("%s() fail:check_ringbuffer_invalid, exception 0x%pK\n",
			__func__, exception);
		return -1;
	}

	/* ring buffer is empty, return directly */
	if (unlikely(is_ringbuffer_empty(q))) {
		BB_PRINT_ERR("%s():ring buffer is empty\n", __func__);
		return -1;
	}

	get_ringbuffer_start_end(q, &start, &end);

	min_trace = NULL;
	for (i = start; i <= end; i++) {
		trace = (rdr_exception_trace_t *)&q->data[(i % q->max_num) * q->field_count];

		if ((trace->e_exce_type == AP_S_AWDT) ||
			((rdr_get_reboot_type() == trace->e_exce_type) &&
			(rdr_get_exec_subtype_value() == trace->e_exce_subtype)))
			continue;

		if ((trace->e_32k_time < min_etime) &&
			(trace->e_32k_time + MIN_EXCEPTION_TIME_GAP >= etime)) {
			/* shall be exception which trigger the whose system reset */
			if (trace->e_reset_core_mask & RDR_AP) {
				min_trace = trace;
				min_etime = trace->e_32k_time;
			}
		}
	}

	if (unlikely(min_trace == NULL)) {
		BB_PRINT_PN("%s(): seach minimum exception trace fail\n", __func__);
		return -1;
	}

	exception->e_reset_core_mask = min_trace->e_reset_core_mask;
	exception->e_from_core       = min_trace->e_from_core;
	exception->e_exce_type       = min_trace->e_exce_type;
	exception->e_exce_subtype    = min_trace->e_exce_subtype;

	return 0;
}

#if 0
static pfn_exception_analysis_ops g_exception_analysis_fn[EXCEPTION_TRACE_ENUM] = {
	rdr_exception_analysis_ap,
};
#endif

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
int rdr_exception_trace_record(u64 e_reset_core_mask, u64 e_from_core,
				u32 e_exce_type, u32 e_exce_subtype)
{
	rdr_exception_trace_t trace;
	unsigned long         lock_flag;

	BB_PRINT_START();

	if (!rdr_init_done() || IS_ERR_OR_NULL(g_et_pdev.plat_device)) {
		BB_PRINT_ERR("rdr init faild!\n");
		BB_PRINT_END();
		return -1;
	}

	if (g_arch_timer_func_ptr)
		trace.e_32k_time = (*g_arch_timer_func_ptr) ();
	else
		trace.e_32k_time = jiffies_64;

	trace.e_reset_core_mask = e_reset_core_mask;
	trace.e_from_core = e_from_core;
	trace.e_exce_type = e_exce_type;
	trace.e_exce_subtype = e_exce_subtype;

	spin_lock_irqsave(&g_et_pdev.lock, lock_flag);

	hisiap_ringbuffer_write((struct hisiap_ringbuffer_s *)
				(g_et_pdev.trace_addr + g_et_pdev.exception_core[EXCEPTION_TRACE_AP].offset),
				(u8 *)&trace);

	spin_unlock_irqrestore(&g_et_pdev.lock, lock_flag);

	BB_PRINT_END();

	return 0;
}

/*
 * Get the info about the reserved debug memroy area from
 * the dtsi file.
 *
 * func args:
 * @num: the number of reserved debug memory area
 * @size: the size of each reserved debug memory area
 *
 * return value
 * 0 success
 * -1 failed
 *
 */
int get_every_core_exception_info(u32 *num, u32 *size, u32 sizelen)
{
	if ((num == NULL) || (size == NULL)) {
		BB_PRINT_ERR("invalid  parameter num or size\n");
		return -1;
	}

	*num = g_et_pdev.area_num;

	if (unlikely(*num != sizelen)) {
		BB_PRINT_ERR("[%s], invaild core num in dts!\n", __func__);
		return -1;
	}

	memcpy(size, g_et_pdev.area_size, (*num) * sizeof(*size));

	return 0;
}

/*
 * to initialize the ring buffer head of reserved memory for core AP exception trace
 *
 * func args:
 * @addr: the virtual start address of the reserved memory for core AP exception trace
 * @size: the length of the reserved memory for core AP exception trace
 *
 * return value
 * 0 success
 * otherwise failure
 *
 */
int exception_trace_buffer_init(u8 *addr, unsigned int size)
{
	u32 min_size = sizeof(struct hisiap_ringbuffer_s) + sizeof(rdr_exception_trace_t);

	if (unlikely(IS_ERR_OR_NULL(addr)))
		return -1;

	if (unlikely(size < min_size))
		return -1;

	return hisiap_ringbuffer_init((struct hisiap_ringbuffer_s *)(addr), size,
		sizeof(rdr_exception_trace_t), NULL);
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
static int rdr_exception_trace_ap_init(u8 *phy_addr, u8 *virt_addr, u32 log_len)
{
	memset(virt_addr, 0, log_len);

	if (unlikely(exception_trace_buffer_init(virt_addr, log_len)))
		return -1;

	return 0;
}

static pfn_exception_init_ops g_exception_init_fn[EXCEPTION_TRACE_ENUM] = {
	rdr_exception_trace_ap_init,
//	rdr_exception_trace_bl31_init,
};

static const struct of_device_id rdr_et_of_match[] = {
	{ .compatible = "rdr,exceptiontrace" },
	{}
};

static int rdr_exceptiontrace_probe(struct platform_device *pdev)
{
	pfn_exception_init_ops ops_fn = NULL;
	struct device *dev = &pdev->dev;
	u32 offset, i;
	int ret;

	BB_PRINT_START();
	memset(&g_et_pdev, 0, sizeof(g_et_pdev));
	if (unlikely(rdr_get_areainfo(RDR_AREA_EXCEPTION_TRACE,
				      &g_et_pdev.current_info))) {
		BB_PRINT_ERR("[%s], rdr_get_areainfo fail!\n", __func__);
		goto error;
	}

	g_et_pdev.trace_addr = rdr_bbox_map(g_et_pdev.current_info.log_addr,
					    g_et_pdev.current_info.log_len);
	if (unlikely(!g_et_pdev.trace_addr)) {
		BB_PRINT_ERR(
			"[%s], rdr_bbox_map fail! addr=0x%llx, len=0x%x \n",
			__func__, g_et_pdev.current_info.log_addr,
			g_et_pdev.current_info.log_len);
		goto error;
	}

	ret = device_property_read_u32(dev, "area_num", &g_et_pdev.area_num);
	if (unlikely(ret)) {
		BB_PRINT_ERR("[%s], cannot find area_num in dts!\n", __func__);
		goto error;
	}
	BB_PRINT_DBG("[%s], get area_num %u in dts!\n", __func__,
		     g_et_pdev.area_num);

	ret = device_property_read_u32_array(dev, "area_sizes",
					 &g_et_pdev.area_size[0],
					 (unsigned long)(g_et_pdev.area_num));
	if (unlikely(ret)) {
		BB_PRINT_ERR("[%s], cannot find area_sizes in dts!\n",
			     __func__);
		goto error;
	}

	offset = 0;
	for (i = 0; i < EXCEPTION_TRACE_ENUM; i++) {
		g_et_pdev.exception_core[i].offset = offset;
		g_et_pdev.exception_core[i].size = g_et_pdev.area_size[i];

		BB_PRINT_PN("[%s]core %u offset %u size %u addr 0x%llx\n",
			    __func__, i, offset, g_et_pdev.area_size[i],
			    (u64)(g_et_pdev.trace_addr + offset));
		offset += g_et_pdev.area_size[i];

		if (unlikely(offset > g_et_pdev.current_info.log_len)) {
			BB_PRINT_ERR(
				"[%s], offset %u overflow! core %u size %u log_len %u\n",
				__func__, offset, i, g_et_pdev.area_size[i],
				g_et_pdev.current_info.log_len);
			goto error;
		}

		ops_fn = g_exception_init_fn[i];
		if (unlikely(ops_fn &&
			     ops_fn((u8 *)(uintptr_t)g_et_pdev.current_info
						    .log_addr +
					    g_et_pdev.exception_core[i].offset,
				    g_et_pdev.trace_addr +
					    g_et_pdev.exception_core[i].offset,
				    g_et_pdev.area_size[i]))) {
			BB_PRINT_ERR(
				"[%s], exception init fail: core %u size %u ops_fn 0x%pK\n",
				__func__, i, g_et_pdev.area_size[i], ops_fn);
			goto error;
		}
	}

	BB_PRINT_END();
	spin_lock_init(&g_et_pdev.lock);
	g_et_pdev.plat_device = pdev;
	return 0;

error:
	BB_PRINT_END();
	return -1;
}

static int rdr_exceptiontrace_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rdr_et_driver = {
	.driver		= {
		.name			= "rdr exception trace driver",
		.of_match_table		= rdr_et_of_match,
	},
	.probe		= rdr_exceptiontrace_probe,
	.remove		= rdr_exceptiontrace_remove,
};

/*
 * Description : Initialization Function
 */
int __init rdr_exceptiontrace_init(void)
{
	platform_driver_register(&rdr_et_driver);

	return 0;
}

static void __exit rdr_exceptiontrace_exit(void)
{
	platform_driver_unregister(&rdr_et_driver);
}

module_init(rdr_exceptiontrace_init);
module_exit(rdr_exceptiontrace_exit);