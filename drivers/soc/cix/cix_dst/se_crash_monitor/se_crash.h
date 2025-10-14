// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025
 Cix Technology Group Co., Ltd.*/
/**
 * SoC: CIX SKY1 platform
 */
#ifndef __SE_CRASH_H__
#define __SE_CRASH_H__

#include <linux/soc/cix/rdr_pub.h>
#include <linux/soc/cix/rdr_platform.h>
#include "../dst_print.h"

#define MAX_CLK_PWR_NAME_LEN (16)
#define SE_MEM_NAME_MAX_LEN 16
#define SE_MEM_DDR "DDR"
#define SE_MEM_CLK "CLK"
#define SE_MEM_POWER "POWER"
#define SE_MEM_AMU "AMU"
#define SE_MEM_SE_WDT "SE_WDT"
#define SE_MEM_PM_WDT "PM_WDT"

enum RDR_SEPM_MODID {
	// for CSU SE
	MODID_CSUSE_MODID_START = PLAT_BB_MOD_CSUSE_START,
	MODID_DEF(CSUSE_EXCEPTION, MODID_CSUSE_MODID_START),
	MODID_DEF(CSUPM_EXCEPTION, MODID_CSUSE_EXCEPTION_END + 1),
	MODID_CSUSE_MODID_END = PLAT_BB_MOD_CSUSE_END,
};

struct se_mem_info {
	char name[SE_MEM_NAME_MAX_LEN];
	u32 offset;
	u32 size;
};

struct se_cleartext {
	char name[SE_MEM_NAME_MAX_LEN];
	pfn_cleartext_ops ops;
};

struct einfo_offset {
	uint offset;
	uint size;
};

int se_clk_cleartext(const char *log_dir_path, u64 log_addr, u32 log_len);
int se_power_cleartext(const char *log_dir_path, u64 log_addr, u32 log_len);
int se_ddr_cleartext(const char *log_dir_path, u64 log_addr, u32 log_len);
int se_wdt_cleartext(const char *log_dir_path, u64 log_addr, u32 log_len);
int pm_wdt_cleartext(const char *log_dir_path, u64 log_addr, u32 log_len);
int se_amu_cleartext(const char *log_dir_path, u64 log_addr, u32 log_len);

#endif
