// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025
 Cix Technology Group Co., Ltd.*/
/**
 * SoC: CIX SKY1 platform
 */
#include "se_crash.h"

struct clk_info {
	char name[MAX_CLK_PWR_NAME_LEN];
	uint32_t freq;
	bool enable;
};

struct clk_einfo {
	uint32_t count;
	struct clk_info clks[];
};

int se_clk_cleartext(const char *log_dir_path, u64 log_addr, u32 log_len)
{
	struct clk_einfo *einfo = (void *)log_addr;
	bool err;
	struct file *fp;

	if (IS_ERR_OR_NULL(einfo))
		return -1;

	fp = bbox_cleartext_get_filep(log_dir_path, "clk.txt");
	if (IS_ERR_OR_NULL(fp))
		return -1;

	for (u32 i = 0; i < einfo->count; i++) {
		rdr_cleartext_print(fp, &err, "clk: %s, freq: %u, enable: %d\n",
				    einfo->clks[i].name, einfo->clks[i].freq,
				    einfo->clks[i].enable);
	}
	bbox_cleartext_end_filep(fp);

	return 0;
}