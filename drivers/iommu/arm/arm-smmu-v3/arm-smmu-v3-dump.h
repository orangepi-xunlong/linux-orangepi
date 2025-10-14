// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 * Author: Zichar Zhang <zichar.zhang@cixtech.com>
 */

#ifndef __ARM_SMMU_V3_DUMP_H__
#define __ARM_SMMU_V3_DUMP_H__

#include "arm-smmu-v3.h"

#define SMMU_DUMP_VALID  0x01

#define MASTER_NAME_SIZE  32

#define SMMU_STE_SIZE (STRTAB_STE_DWORDS * 4)
#define SMMU_CD_SIZE (CTXDESC_CD_DWORDS * 4)

#define LAST_MASTER_HEAD(head) \
	(struct smmu_dump_master_head *)((char *)head + head->last_master)

#define MASTER_STE_SIZE(mhead) (mhead->ste_num * SMMU_STE_SIZE)
#define MASTER_STE(mhead) \
		((void *)mhead + sizeof(struct smmu_dump_master_head))
#define MASTER_CD(mhead) (MASTER_STE(mhead) + MASTER_STE_SIZE(mhead))
#define MASTER_IO_MAP_S1(mhead) (MASTER_CD(mhead) + SMMU_CD_SIZE)
#define MASTER_IO_MAP_S1_END(mhead) \
		(MASTER_IO_MAP_S1(mhead) + mhead->io_map_s1_size)

struct smmu_dump_head {
	u32 status; /* type, valid */
	u32 smmu_ver;
	u32 reserve[1];
	u32 total_size;
	u32 dump_max;
	u32 master_num;
	u32 last_master;
	u32 crc;
} __attribute__((packed));

struct smmu_dump_master_head {
	u8  name[MASTER_NAME_SIZE];
	u32 type;
	u32 reserved[3];
	u32 ste_num;
	u32 io_map_s1_size;
	u32 io_map_s2_size;
	u32 crc;
} __attribute__((packed));

struct smmu_dump_io_map {
	u64 iova;
	u64 pa;
	u32 len;
	u32 prot;
	u32 reserved[2];
} __attribute__((packed));

struct smmu_master_dump_info {
	const char *smmu_name;
	const char *smmu_master_name;
	void *buf;
	size_t size;
};

int smmu_master_dump(struct smmu_master_dump_info *info);

#endif /* __ARM_SMMU_V3_DUMP_H__ */
