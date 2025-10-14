// SPDX-License-Identifier: GPL-2.0-only
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

#include "rdr_exception_trace.h"

struct exception_core_s {
	u32 offset;
	u32 size;
};

typedef struct {
	struct platform_device *plat_device;
	uint32_t area_num;
	uint32_t area_size[RDR_CORE_MAX_INDEX];
	struct exception_core_s exception_core[RDR_CORE_MAX_INDEX];
	struct rdr_register_module_result current_info;
	u8 *trace_addr;
	spinlock_t lock;
} rdr_exceptiontrace_pdev;

static rdr_exceptiontrace_pdev g_et_pdev = { 0 };
static rdr_execption_trace_ops g_exception_ops[RDR_CORE_MAX_INDEX] = {
	{ rdr_exception_trace_ap_init, NULL,
	  rdr_exception_trace_ap_cleartext_print },
};

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
		BB_ERR("invalid  parameter num or size\n");
		return -1;
	}

	*num = g_et_pdev.area_num;

	if (unlikely(*num != sizelen)) {
		BB_ERR("invaild core num in dts/acpi!\n");
		return -1;
	}

	memcpy(size, g_et_pdev.area_size, (*num) * sizeof(*size));

	return 0;
}

static int rdr_exception_trace_cleartext_print(const char *dir_path,
					       u64 log_addr, u32 log_len)
{
	pfn_cleartext_ops ops_fn = NULL;
	u32 i, offset;

	if (IS_ERR_OR_NULL(dir_path) ||
	    IS_ERR_OR_NULL((void *)(uintptr_t)log_addr)) {
		BB_ERR("error:dir_path 0x%pK log_addr 0x%pK\n", dir_path,
		       (void *)(uintptr_t)log_addr);
		return -1;
	}

	offset = 0;
	for (i = 0; i < RDR_CORE_MAX_INDEX; i++) {
		ops_fn = g_exception_ops[i].cleartext;

		if (unlikely(offset + g_et_pdev.area_size[i] > log_len)) {
			BB_ERR("offset %u overflow! core %u size %u log_len %u\n",
			       offset, i, g_et_pdev.area_size[i], log_len);
			return -1;
		}

		if (unlikely(log_addr + offset < log_addr)) {
			BB_ERR("log_addr: %llx offset: %x\n", log_addr, offset);
			return -1;
		}

		if (unlikely(ops_fn && ops_fn(dir_path, log_addr + offset,
					      g_et_pdev.area_size[i]))) {
			BB_ERR("pfn_cleartext_ops %pS fail! core %u size %u\n",
			       ops_fn, i, g_et_pdev.area_size[i]);
			return -1;
		}

		offset += g_et_pdev.area_size[i];
	}

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
	u32 min_size = sizeof(struct rdr_ringbuffer) +
		       sizeof(struct rdr_exception_trace_s);

	if (IS_ERR_OR_NULL(addr))
		return -1;

	if (unlikely(size < min_size))
		return -1;

	return rdr_rbuf_init((struct rdr_ringbuffer *)(addr),
				      size,
				      sizeof(struct rdr_exception_trace_s),
				      NULL);
}

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

	BB_PR_START();
	memset(&g_et_pdev, 0, sizeof(g_et_pdev));
	if (unlikely(rdr_get_areainfo(RDR_EXCEPTION_TRACE_INDEX,
				      &g_et_pdev.current_info))) {
		BB_ERR("rdr_get_areainfo fail!\n");
		goto error;
	}

	g_et_pdev.trace_addr = rdr_bbox_map(g_et_pdev.current_info.log_addr,
					    g_et_pdev.current_info.log_len);
	if (unlikely(!g_et_pdev.trace_addr)) {
		BB_ERR("rdr_bbox_map fail! addr=0x%llx, len=0x%x\n",
		       g_et_pdev.current_info.log_addr,
		       g_et_pdev.current_info.log_len);
		goto error;
	}

	ret = device_property_read_u32(dev, "area_num", &g_et_pdev.area_num);
	if (unlikely(ret)) {
		BB_ERR("cannot find area_num in dts/acpi!\n");
		goto error;
	}
	BB_DBG("get area_num %u in dts!\n", g_et_pdev.area_num);

	ret = device_property_read_u32_array(
		dev, "area_sizes", &g_et_pdev.area_size[0],
		(unsigned long)(g_et_pdev.area_num));
	if (unlikely(ret)) {
		BB_ERR("cannot find area_sizes in dts/acpi!\n");
		goto error;
	}

	offset = 0;
	for (i = 0; i < RDR_CORE_MAX_INDEX; i++) {
		g_et_pdev.exception_core[i].offset = offset;
		g_et_pdev.exception_core[i].size = g_et_pdev.area_size[i];

		BB_PN("core %u offset %u size %u addr 0x%llx\n", i, offset,
		      g_et_pdev.area_size[i],
		      (u64)(g_et_pdev.trace_addr + offset));
		offset += g_et_pdev.area_size[i];

		if (unlikely(offset > g_et_pdev.current_info.log_len)) {
			BB_ERR("offset %u overflow! core %u size %u log_len %u\n",
			       offset, i, g_et_pdev.area_size[i],
			       g_et_pdev.current_info.log_len);
			goto error;
		}

		ops_fn = g_exception_ops[i].init;
		if (unlikely(ops_fn &&
			     ops_fn((u8 *)(uintptr_t)g_et_pdev.current_info
						    .log_addr +
					    g_et_pdev.exception_core[i].offset,
				    g_et_pdev.trace_addr +
					    g_et_pdev.exception_core[i].offset,
				    g_et_pdev.area_size[i]))) {
			BB_ERR("exception init fail: core %u size %u ops_fn 0x%pK\n",
			       i, g_et_pdev.area_size[i], ops_fn);
			goto error;
		}
	}

	ret = rdr_register_cleartext_ops(RDR_EXCEPTION_TRACE,
					 rdr_exception_trace_cleartext_print);
	if (unlikely(ret < 0)) {
		BB_ERR("register rdr_exception_trace_cleartext_print fail, ret = [%d]\n",
		       ret);
		goto error;
	}

	BB_PR_END();
	g_et_pdev.plat_device = pdev;
	return 0;

error:
	BB_PR_END();
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
