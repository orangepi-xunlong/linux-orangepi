// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025
 Cix Technology Group Co., Ltd.*/
/**
 * SoC: CIX SKY1 platform
 */
#include "se_crash.h"

#define AMU_DATA_32TO64(data, x) \
	((u64)data->x##_low | (u64)data->x##_high << 32)

struct amu_data {
	uint cpu;
	uint cpu_cycle_low;
	uint cpu_cycle_high;
	uint cnt_cycle_low;
	uint cnt_cycle_high;
	uint cnt_inst_low;
	uint cnt_inst_high;
	uint memory_stall_cycle_low;
	uint memory_stall_cycle_high;
};

struct amu_einfo_head {
	struct einfo_offset amu;
};

struct amu_einfo {
	uint cpunum;
	uint per_amu_size;
	struct amu_einfo_head head;
	struct amu_data amu[];
};

static void se_show_amu_data(struct file *fp, struct amu_data *data)
{
	bool err;

	rdr_cleartext_print(fp, &err, "cpu[%d]:\n", data->cpu);
	rdr_cleartext_print(fp, &err, "\tcpu cycle: 0x%016llx, ",
			    AMU_DATA_32TO64(data, cpu_cycle));
	rdr_cleartext_print(fp, &err, "\tcnt cycle: 0x%016llx, ",
			    AMU_DATA_32TO64(data, cnt_cycle));
	rdr_cleartext_print(fp, &err, "\tcnt inst: 0x%016llx, ",
			    AMU_DATA_32TO64(data, cnt_inst));
	rdr_cleartext_print(fp, &err, "\tmem stall cycle: 0x%016llx\n",
			    AMU_DATA_32TO64(data, memory_stall_cycle));
}

static void se_show_amu_einfo(struct file *fp, struct amu_einfo *einfo)
{
	void *amu = (void *)einfo + einfo->head.amu.offset;

	for (int i = 0; i < einfo->cpunum; i++) {
		se_show_amu_data(fp, amu + (size_t)(i * einfo->per_amu_size));
	}
}

int se_amu_cleartext(const char *log_dir_path, u64 log_addr, u32 log_len)
{
	struct amu_einfo *einfo = (void *)log_addr;
	void *tmp;
	bool err;
	struct file *fp;

	if (IS_ERR_OR_NULL(einfo))
		return -1;

	fp = bbox_cleartext_get_filep(log_dir_path, "amu.txt");
	if (IS_ERR_OR_NULL(fp))
		return -1;

	for (int i = 0; i < 2; i++) {
		rdr_cleartext_print(
			fp, &err, "----------------time[%d]----------------\n",
			i);
		tmp = (void *)einfo +
		      i * (einfo->head.amu.size + sizeof(struct amu_einfo));
		se_show_amu_einfo(fp, tmp);
	}
	bbox_cleartext_end_filep(fp);

	return 0;
}