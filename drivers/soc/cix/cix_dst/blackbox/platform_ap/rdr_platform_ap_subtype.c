/*
 * rdr_hisi_ap_subtype.c
 *
 * AP reset exception subtype function implementation
 *
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd.
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
#include <linux/io.h>
#include <linux/soc/cix/rdr_pub.h>
#include <linux/soc/cix/util.h>
#include <mntn_subtype_exception.h>
#include "../rdr_inner.h"
#include "../rdr_print.h"
#include "rdr_platform_ap_adapter.h"

#define RDR_CATEGORY_TYPE "UNDEF"

static char g_subtype_name[RDR_REBOOT_REASON_LEN] = "undef";
static u32 g_subtype;

/* subtype exception map */
#undef __MMC_EXCEPTION_SUBTYPE_MAP
#define __MMC_EXCEPTION_SUBTYPE_MAP(x, y, z) { x, #y, #z, z },

#undef __AP_DDRC_SEC_SUBTYPE_MAP
#define __AP_DDRC_SEC_SUBTYPE_MAP(x, y, z) { x, #y, #z, z },

#undef __AP_PANIC_SUBTYPE_MAP
#define __AP_PANIC_SUBTYPE_MAP(x, y, z) { x, #y, #z, z },

#undef __AP_BL31PANIC_SUBTYPE_MAP
#define __AP_BL31PANIC_SUBTYPE_MAP(x, y, z) { x, #y, #z, z },

#undef __AP_VENDOR_PANIC_SUBTYPE_MAP
#define __AP_VENDOR_PANIC_SUBTYPE_MAP(x, y, z) { x, #y, #z, z },

#undef __APWDT_HWEXC_SUBTYPE_MAP
#define __APWDT_HWEXC_SUBTYPE_MAP(x, y, z) { x, #y":hw", #z, z },

#undef __APWDT_EXC_SUBTYPE_MAP
#define __APWDT_EXC_SUBTYPE_MAP(x, y, z) { x, #y, #z, z },

#undef __LPM3_EXC_SUBTYPE_MAP
#define __LPM3_EXC_SUBTYPE_MAP(x, y, z) { x, #y, #z, z },

#undef __MEM_REPAIR_EXC_SUBTYPE_MAP
#define __MEM_REPAIR_EXC_SUBTYPE_MAP(x, y, z) { x, #y, #z, z },

#undef __SCHARGER_EXC_SUBTYPE_MAP
#define __SCHARGER_EXC_SUBTYPE_MAP(x, y, z) { x, #y, #z, z },

#undef __PMU_EXC_SUBTYPE_MAP
#define __PMU_EXC_SUBTYPE_MAP(x, y, z) { x, #y, #z, z },

#undef __NPU_EXC_SUBTYPE_MAP
#define __NPU_EXC_SUBTYPE_MAP(x, y, z) { x, #y, #z, z },

#undef __CONN_EXC_SUBTYPE_MAP
#define __CONN_EXC_SUBTYPE_MAP(x, y, z) { x, #y, #z, z },

#undef __HISEE_EXC_SUBTYPE_MAP
#define __HISEE_EXC_SUBTYPE_MAP(x, y, z) { x, #y, #z, z },

#undef __IVP_EXC_SUBTYPE_MAP
#define __IVP_EXC_SUBTYPE_MAP(x, y, z) { x, #y, #z, z },

#undef __DSS_EXC_SUBTYPE_MAP
#define __DSS_EXC_SUBTYPE_MAP(x, y, z) { x, #y, #z, z },

struct exp_subtype exp_subtype_map[] = {
	#include <mntn_subtype_exception_map.h>
};

#undef __MMC_EXCEPTION_SUBTYPE_MAP
#undef __AP_DDRC_SEC_SUBTYPE_MAP
#undef __AP_PANIC_SUBTYPE_MAP
#undef __AP_BL31PANIC_SUBTYPE_MAP
#undef __AP_VENDOR_PANIC_SUBTYPE_MAP
#undef __APWDT_HWEXC_SUBTYPE_MAP
#undef __APWDT_EXC_SUBTYPE_MAP
#undef __LPM3_EXC_SUBTYPE_MAP
#undef __MEM_REPAIR_EXC_SUBTYPE_MAP
#undef __SCHARGER_EXC_SUBTYPE_MAP
#undef __PMU_EXC_SUBTYPE_MAP
#undef __NPU_EXC_SUBTYPE_MAP
#undef __CONN_EXC_SUBTYPE_MAP
#undef __HISEE_EXC_SUBTYPE_MAP
#undef __IVP_EXC_SUBTYPE_MAP
#undef __DSS_EXC_SUBTYPE_MAP

static u32 get_exception_subtype_map_size(void)
{
	return ARRAY_SIZE(exp_subtype_map);
}

static u32 get_category_value(u32 e_exce_type)
{
	u32 i;
	u32 category = 0;
	const struct cmdword *reboot_reason_map = get_reboot_reason_map();

	if (!reboot_reason_map) {
		pr_err("[%s:%d]: reboot_reason_map is NULL\n", __func__, __LINE__);
		return 0;
	}

	for (i = 0; i < get_reboot_reason_map_size(); i++) {
		if (reboot_reason_map[i].num == e_exce_type) {
			category = reboot_reason_map[i].category_num;
			break;
		}
	}
	return category;
}

/*
 * Description:  get exception subtype name
 * Date:         2017/08/16
 * Modification: Created function
 */
char *rdr_get_subtype_name(u32 e_exce_type, u32 subtype)
{
	u32 i;

	if (get_category_value(e_exce_type) == SUBTYPE)
		for (i = 0; (unsigned int)i < get_exception_subtype_map_size(); i++) {
			if (exp_subtype_map[i].exception == e_exce_type &&
				exp_subtype_map[i].subtype_num == subtype)
				return (char *)exp_subtype_map[i].subtype_name;
		}

	return NULL;
}

/*
 * Description:  get category
 * Date:         2017/08/16
 * Modification: Created function
 */
char *rdr_get_category_name(u32 e_exce_type, u32 subtype)
{
	int i, category;
	const struct cmdword *reboot_reason_map = get_reboot_reason_map();

	if (!reboot_reason_map) {
		pr_err("[%s:%d]: reboot_reason_map is NULL\n", __func__, __LINE__);
		return NULL;
	}

	category = get_category_value(e_exce_type);
	if (category == SUBTYPE) {
		for (i = 0; (unsigned int)i < get_exception_subtype_map_size(); i++) {
			if (exp_subtype_map[i].exception == e_exce_type &&
				exp_subtype_map[i].subtype_num == subtype) {
				return (char *)exp_subtype_map[i].category_name;
			}
		}
	} else {
		for (i = 0; (unsigned int)i < get_reboot_reason_map_size(); i++) {
			if (reboot_reason_map[i].num == e_exce_type)
				return (char *)reboot_reason_map[i].category_name;
		}
	}
	return RDR_CATEGORY_TYPE;
}

char *rdr_get_exec_subtype(void)
{
	return g_subtype_name;
}

u32 rdr_get_exec_subtype_value(void)
{
	return g_subtype;
}
