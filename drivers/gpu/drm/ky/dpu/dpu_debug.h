// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef _DPU_DEBUG_H_
#define _DPU_DEBUG_H_

#include <linux/types.h>
#include "saturn_regs/reg_map.h"
#include "./../ky_dpu.h"

typedef enum {
	E_DPU_TOP_REG = 0,
	E_DPU_CTRL_REG,
	E_DPU_CRG_REG,
	E_DPU_CMDLIST_REG,
	E_DPU_INT_REG,

	E_DMA_TOP_CTRL_REG,
	E_RDMA_LAYER0_REG,
	E_RDMA_LAYER1_REG,
	E_RDMA_LAYER2_REG,
	E_RDMA_LAYER3_REG,
	E_RDMA_LAYER4_REG,
	E_RDMA_LAYER5_REG,
	E_RDMA_LAYER6_REG,
	E_RDMA_LAYER7_REG,
	E_RDMA_LAYER8_REG,
	E_RDMA_LAYER9_REG,
	E_RDMA_LAYER10_REG,
	E_RDMA_LAYER11_REG,

	E_MMU_TBU0_REG,
	E_MMU_TBU1_REG,
	E_MMU_TBU2_REG,
	E_MMU_TBU3_REG,
	E_MMU_TBU4_REG,
	E_MMU_TBU5_REG,
	E_MMU_TBU6_REG,
	E_MMU_TBU7_REG,
	E_MMU_TBU8_REG,
	E_MMU_TOP_REG,

	E_LP0_REG,
	E_LP1_REG,
	E_LP2_REG,
	E_LP3_REG,
	E_LP4_REG,
	E_LP5_REG,
	E_LP6_REG,
	E_LP7_REG,
	E_LP8_REG,
	E_LP9_REG,
	E_LP10_REG,
	E_LP11_REG,

	E_LM0_REG,
	E_LM1_REG,
	E_LM2_REG,
	E_LM3_REG,
	E_LM4_REG,
	E_LM5_REG,
	E_LM6_REG,
	E_LM7_REG,
	E_LM8_REG,
	E_LM9_REG,
	E_LM10_REG,
	E_LM11_REG,

	E_COMPOSER0_REG,
	E_COMPOSER1_REG,
	E_COMPOSER2_REG,
	E_COMPOSER3_REG,

	E_SCALER0_REG,
	E_SCALER1_REG,

	E_OUTCTRL0_REG,
	E_PP0_REG,
	E_OUTCTRL1_REG,
	E_PP1_REG,
	E_OUTCTRL2_REG,
	E_PP2_REG,
	E_OUTCTRL3_REG,
	E_PP3_REG,

	E_WB_TOP_0_REG,
	E_WB_TOP_1_REG,

	E_DPU_DUMP_ALL
}dpu_reg_enum;

typedef struct dpu_reg_dump {
	dpu_reg_enum	index;
	u8*		module_name;
	uint32_t	module_offset;
	uint32_t	dump_reg_num;
}dpu_reg_dump_t;

void dump_dpu_regs(struct ky_dpu *dpu, dpu_reg_enum reg_enum, u8 trace_dump);
void dpu_dump_reg(struct ky_dpu *dpu);
void dpu_dump_fps(struct ky_dpu *dpu);
int dpu_buffer_dump(struct drm_plane *plane);
void dpu_underrun_wq_stop_trace(struct work_struct *work);

#endif
