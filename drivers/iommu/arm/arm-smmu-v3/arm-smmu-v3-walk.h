// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 * Author: Zichar Zhang <zichar.zhang@cixtech.com>
 */
#include "arm-smmu-v3.h"
#include <linux/io-pgtable.h>
#include "../../io-pgtable-arm.h"

#ifndef __ARM_SMMU_V3_WALK_H__
#define __ARM_SMMU_V3_WALK_H__

struct smmu_master_handle {
	const char *smmu_name;
	const char *smmu_master_name;
	void *data;

	int (*init) (struct arm_smmu_master *, void *);
	int (*hdl_ste) (__le64 *, u32, void *);
	int (*hdl_cd) (__le64 *, void *);
	int (*hdl_io_map_s1) (u64 iova, phys_addr_t pa, u32 len, u32 prot,
				void *);
	int (*hdl_io_map_s2) (u64 iova, phys_addr_t pa, u32 len, u32 prot,
				void *);
	void (*finish)(void *, int);
};

int smmu_master_walk_register(struct arm_smmu_master *master);
void smmu_master_walk_unregister(struct arm_smmu_master *master);
int smmu_master_walk(struct smmu_master_handle *mhdl);

#endif /* __ARM_SMMU_V3_WALK_H__ */
