// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025
 Cix Technology Group Co., Ltd.*/
/**
 * SoC: CIX SKY1 platform
 */
#include <linux/printk.h>
#include <linux/err.h>
#include <linux/types.h>
#include "se_crash.h"

struct ddr_reg {
	uint32_t offset;
	uint32_t type;
	uint32_t val;
};

struct ddr_channel_info {
	uint32_t num;
	uint32_t channel;
	struct ddr_reg regs[];
};

struct ddr_einfo {
	uint32_t channel;
	struct ddr_channel_info info[];
};

static void se_ddr_show_channel(struct file *fp, struct ddr_channel_info *info)
{
	bool err;

	rdr_cleartext_print(fp, &err, "channel[%d]: num=%d, %px\n",
			    info->channel, info->num, info);
	for (u32 i = 0; i < info->num; i++) {
		rdr_cleartext_print(
			fp, &err,
			"\toffset[%d]: 0x%x, type[%d]: 0x%x, val[%d]: 0x%x\n",
			i, info->regs[i].offset, i, info->regs[i].type, i,
			info->regs[i].val);
	}
}

int se_ddr_cleartext(const char *log_dir_path, u64 log_addr, u32 log_len)
{
	struct ddr_einfo *einfo = (void *)log_addr;
	struct ddr_channel_info *info = einfo->info;
	uint64_t start = (uint64_t)info;
	struct file *fp;

	if (IS_ERR_OR_NULL(einfo))
		return -1;

	fp = bbox_cleartext_get_filep(log_dir_path, "ddr.txt");
	if (IS_ERR_OR_NULL(fp))
		return -1;

	for (u32 i = 0; i < einfo->channel; i++) {
		se_ddr_show_channel(fp, info);
		start += (sizeof(struct ddr_channel_info) +
			  info->num * sizeof(struct ddr_reg));
		info = (void *)start;
	}
	bbox_cleartext_end_filep(fp);

	return 0;
}