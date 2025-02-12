// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef _KY_DMMU_H_
#define _KY_DMMU_H_

#include <linux/types.h>

#include <drm/drm_file.h>
#include "dpu/dpu_saturn.h"

#define DPU_QOS_URGENT	4
#define DPU_QOS_NORMAL	3
#define DPU_QOS_LOW	2
/*
 * In worst case, the tlb alignment requires src_x * 16 * pixel_bytes.
 * The maximum src_x currently support is 4096. So we need to fill
 * extra 60 (4096*15*4) entries into mmu page table.
 */
#define HW_ALIGN_TTB_NUM	60

#define RD_OUTS_NUM             16
#define RDMA_TIMELIMIT  0xFFFF

#define BASE_VA         0x10000000ULL
#define VA_STEP_PER_TBU 0x40000000ULL

#define TBU_BASE_VA(tbu_id) ((uint64_t)BASE_VA + (uint64_t)VA_STEP_PER_TBU * tbu_id)

#define CONFIG_TBU_REGS(priv, hwdev, reg_id, tbu_id) \
{\
	struct ky_hw_device *hwdev_p = (struct ky_hw_device *)hwdev; \
	if (hwdev_p) { \
		dpu_write_reg(hwdev_p, MMU_REG, MMU_BASE_ADDR, v.TBU[tbu_id].TBU_Base_Addr##reg_id##_Low, \
				tbu.ttb_pa[reg_id] & 0xFFFFFFFF); \
		dpu_write_reg(hwdev_p, MMU_REG, MMU_BASE_ADDR, v.TBU[tbu_id].TBU_Base_Addr##reg_id##_High, \
				(tbu.ttb_pa[reg_id] >> 32) & 0x3); \
		dpu_write_reg(hwdev_p, MMU_REG, MMU_BASE_ADDR, v.TBU[tbu_id].TBU_VA##reg_id, \
				(tbu.tbu_va[reg_id] >> 12) & 0x3FFFFF); \
		dpu_write_reg(hwdev_p, MMU_REG, MMU_BASE_ADDR, \
				v.TBU[tbu_id].TBU_SIZE##reg_id, tbu.ttb_size[reg_id]); \
	} else { \
		write_to_cmdlist(priv, MMU_REG, MMU_BASE_ADDR, \
				TBU[tbu_id].TBU_Base_Addr##reg_id##_Low, \
				tbu.ttb_pa[reg_id] & 0xFFFFFFFF); \
		write_to_cmdlist(priv, MMU_REG, MMU_BASE_ADDR, \
				TBU[tbu_id].TBU_Base_Addr##reg_id## _High, \
				(tbu.ttb_pa[reg_id] >> 32) & 0x3); \
		write_to_cmdlist(priv, MMU_REG, MMU_BASE_ADDR, TBU[tbu_id].TBU_VA##reg_id, \
				(tbu.tbu_va[reg_id] >> 12) & 0x3FFFFF); \
		write_to_cmdlist(priv, MMU_REG, MMU_BASE_ADDR, \
				TBU[tbu_id].TBU_SIZE##reg_id, tbu.ttb_size[reg_id]); \
	} \
}

#define CONFIG_RDMA_ADDR_REG(priv, reg_id, rdma_id, addr) \
{ \
	write_to_cmdlist(priv, RDMA_PATH_X_REG, RDMA_BASE_ADDR(rdma_id), \
			 LEFT_BASE_ADDR##reg_id##_LOW, addr & 0xFFFFFFFF); \
	write_to_cmdlist(priv, RDMA_PATH_X_REG, RDMA_BASE_ADDR(rdma_id), \
			 LEFT_BASE_ADDR##reg_id##_HIGH, (addr >> 32) & 0x3); \
}

#define CONFIG_RDMA_ADDR_REG_32(priv, reg_id, rdma_id, addr) \
{ \
	write_to_cmdlist(priv, RDMA_PATH_X_REG, RDMA_BASE_ADDR(rdma_id), \
			 LEFT_BASE_ADDR##reg_id##_LOW, addr & 0xFFFFFFFF); \
	write_to_cmdlist(priv, RDMA_PATH_X_REG, RDMA_BASE_ADDR(rdma_id), \
			 LEFT_BASE_ADDR##reg_id##_HIGH, 0); \
}

#define CONFIG_WB_ADDR_REG(hwdev, reg_id, addr) \
{ \
	struct ky_hw_device *hwdev_p = (struct ky_hw_device *)hwdev; \
	dpu_write_reg(hwdev_p, WB_TOP_REG, WB0_TOP_BASE_ADDR, \
			wb_wdma_base_addr##reg_id##_low, addr & 0xFFFFFFFF); \
	dpu_write_reg(hwdev_p, WB_TOP_REG, WB0_TOP_BASE_ADDR, \
			wb_wdma_base_addr##reg_id##_high, (addr >> 32) & 0x3); \
}

struct tbu_instance {
	uint64_t ttb_pa[3];
	uint64_t tbu_va[3];
	uint32_t ttb_size[3];
};

int ky_dmmu_map(struct drm_plane *plane, struct dpu_mmu_tbl *mmu_tbl, u8 tbu_id, bool wb);
void ky_dmmu_unmap(struct drm_plane *plane);

#endif /* _KY_DMMU_H_ */
