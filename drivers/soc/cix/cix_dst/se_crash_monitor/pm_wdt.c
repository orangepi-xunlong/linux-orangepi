// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025
 Cix Technology Group Co., Ltd.*/
/**
 * SoC: CIX SKY1 platform
 */
#include "se_crash.h"

struct pm_reg {
	uint32_t addr;
	uint32_t val;
};

struct pm_einfo {
	uint num;
	struct pm_reg reg[];
};

int pm_wdt_cleartext(const char *log_dir_path, u64 log_addr, u32 log_len)
{
	struct pm_einfo *einfo = (void *)log_addr;
	struct pm_reg *reg;
	struct file *fp;
	bool err;

	if (IS_ERR_OR_NULL(einfo))
		return -1;

	fp = bbox_cleartext_get_filep(log_dir_path, "pm_wdt.txt");
	if (IS_ERR_OR_NULL(fp))
		return -1;

	reg = &einfo->reg[0];
	rdr_cleartext_print(fp, &err, "   addr   ,    val    \n");
	for (int i = 0; i < einfo->num; i++) {
		rdr_cleartext_print(fp, &err, "0x%08x: 0x%08x\n", reg[i].addr,
				    reg[i].val);
	}
	bbox_cleartext_end_filep(fp);

	return 0;
}