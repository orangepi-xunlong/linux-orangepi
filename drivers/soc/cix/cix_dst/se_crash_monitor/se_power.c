// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025
 Cix Technology Group Co., Ltd.*/
/**
 * SoC: CIX SKY1 platform
 */
#include "se_crash.h"

#define MAX_PWR_CONFIG_NAME_LEN (8)

struct pwr_info {
	char name[MAX_CLK_PWR_NAME_LEN];
	char sw_config[MAX_PWR_CONFIG_NAME_LEN];
	char hw_config[MAX_PWR_CONFIG_NAME_LEN];
	uint32_t data;
};

struct pwr_einfo {
	uint32_t count;
	struct pwr_info pwrs[];
};

int se_power_cleartext(const char *log_dir_path, u64 log_addr, u32 log_len)
{
	struct pwr_einfo *einfo = (void *)log_addr;
	struct file *fp;
	bool err;

	if (IS_ERR_OR_NULL(einfo))
		return -1;

	fp = bbox_cleartext_get_filep(log_dir_path, "power.txt");
	if (IS_ERR_OR_NULL(fp))
		return -1;

	for (u32 i = 0; i < einfo->count; i++) {
		rdr_cleartext_print(
			fp, &err, "pwr: %s, sw: %s, hw: %s, reg: 0x%x\n",
			einfo->pwrs[i].name, einfo->pwrs[i].sw_config,
			einfo->pwrs[i].hw_config, einfo->pwrs[i].data);
	}
	bbox_cleartext_end_filep(fp);

	return 0;
}