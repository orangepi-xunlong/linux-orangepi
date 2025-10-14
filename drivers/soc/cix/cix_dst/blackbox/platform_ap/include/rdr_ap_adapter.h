/* SPDX-License-Identifier: GPL-2.0-only */
// Copyright 2025 Cix Technology Group Co., Ltd.
/*
 * rdr_hisi_ap_adapter.h
 *
 * Based on the RDR framework, adapt to the AP side to implement resource
 *
 * Copyright (c) 2012-2019 Huawei Technologies Co., Ltd.
 * Copyright 2024 Cix Technology Group Co., Ltd.
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
#ifndef __RDR_AP_ADAPTER_H__
#define __RDR_AP_ADAPTER_H__

#include <linux/thread_info.h>
#include <linux/pstore.h>
#include <linux/soc/cix/rdr_platform.h>
#include <linux/soc/cix/rdr_platform_ap_hook.h>
#include <linux/platform_device.h>
#include "rdr_ap_hook.h"
#include "rdr_ap_moddump.h"
#include "rdr_ap_regdump.h"
#include "rdr_ap_logbuf.h"
#include "rdr_ap_stack.h"
#include "rdr_ap_suspend.h"

#define PRODUCT_VERSION_LEN 32
#define PRODUCT_DEVICE_LEN 32
#define AP_DUMP_MAGIC 0x19283746
#define BBOX_VERSION 0x1001B /* v1.0.11 */
#define AP_DUMP_END_MAGIC 0x1F2E3D4C
#define SIZE_1K 0x400
#define ROOT_CHECK_SIZE 128
#define ROOT_HEAD_SIZE 0x4000
#define PROPERTY_INIT(name) { #name, 0 }
#define STRUCT_PRINT(struct, name, format) \
	rdr_cleartext_print(fp, &error, #name "[" format "]\n", ap_root->name);
#define GET_ADDR_FROM_EHROOT(ehroot, addr) \
	(((void *)ehroot) + ((addr) - (ehroot->mem.vaddr)))
#ifndef MAX
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#endif
#ifndef MIN
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#endif

struct property_table {
	const char *prop_name;
	unsigned int size;
};

struct ap_eh_root {
	struct {
		unsigned int dump_magic;
		unsigned char version[PRODUCT_VERSION_LEN];
		struct bbox_mem mem;
		unsigned char device_id[PRODUCT_DEVICE_LEN];
		/* Indicates the BBox version */
		u64 bbox_version;
	} __aligned(ROOT_CHECK_SIZE);
	char ecc[RDR_BCH_GET_ECC_BYTES(ROOT_CHECK_SIZE)];
	/* Reentrant count,The initial value is 0,Each entry++ */
	unsigned int enter_times;
	u64 slice;
	struct rdr_safemem_pool pool;
} __aligned(ROOT_HEAD_SIZE);

int ap_prop_table_init(struct device *dev, struct property_table *table,
		       u32 table_size);
void *get_addr_from_root(struct ap_eh_root *ehroot, void *addr);
void ap_exception_callback(u32 argc, void *argv);
int rdr_exception_init(void);
#endif
