/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * rdr_field.h
 *
 * blackbox header file (blackbox: kernel run data recorder.)
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

#ifndef __BB_FIELD_H__
#define __BB_FIELD_H__

#include <linux/types.h>
#include <linux/soc/cix/rdr_pub.h>
#include "rdr_inner.h"

#define RDR_TIME_LEN 16

#define FILE_MAGIC 0xdead8d8d
#ifdef CONFIG_SMP
#define RDR_SMP_FLAG 1
#else
#define RDR_SMP_FLAG 0
#endif

#define RDR_VERSION ((RDR_SMP_FLAG << 16) | (0x205 << 0)) /* v2.04 2019.08.19 */
#define RDR_BASEINFO_SIZE (0x1000)
#define RDR_AUC_SIZE 3

struct rdr_base_info_s {
	u32 modid;
	u32 arg1;
	u32 arg2;
	u32 e_core;
	u32 e_type;
	u32 e_subtype;
	u32 start_flag;
	u32 savefile_flag;
	u32 reboot_flag;
	u32 reserve;
	u8 e_module[MODULE_NAME_LEN];
	u8 e_desc[STR_EXCEPTIONDESC_MAXLEN];

	u8 datetime[DATATIME_MAXLEN];
};

#define RDR_BUILD_DATE_TIME_LEN 16
struct rdr_top_head_s {
	u32 magic;
	u32 version;
	u32 area_number;
	u64 base_addr;
	u32 size;
	u8 build_time[RDR_BUILD_DATE_TIME_LEN];
	u8 product_name[RDR_PRODUCT_MAXLEN];
	u8 product_version[RDR_PRODUCT_MAXLEN];
};

struct rdr_cleartext_s {
	u8 savefile_flag;
	u8 auc_resv[RDR_AUC_SIZE];
};

#pragma pack(4)
struct rdr_struct_s {
	struct rdr_top_head_s top_head;
	char ecc[RDR_BCH_GET_ECC_BYTES(sizeof(struct rdr_top_head_s))];
	struct rdr_base_info_s base_info;
	struct rdr_cleartext_s cleartext_info;
	struct rdr_safemem_pool pool;
} __aligned(RDR_BASEINFO_SIZE);
#pragma pack()

struct rdr_struct_s *rdr_get_head(bool is_last);
void rdr_clear_last_head(void);
u32 rdr_total_mem_size(struct rdr_struct_s *data);
int rdr_field_init(struct rdr_area_data *data);
void rdr_save_args(u32 modid, u32 arg1, u32 arg2);
void rdr_show_base_info(bool is_last);
void rdr_fill_edata(struct rdr_exception_info_s *e, const char *date);
int rdr_get_areainfo(int core_index,
		     struct rdr_register_module_result *retinfo);
#endif /* End #define __BB_FIELD_H__ */
