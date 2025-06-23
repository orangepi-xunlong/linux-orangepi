/* SPDX-License-Identifier: GPL-2.0 */
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */
#ifndef _DP_DEV_H_
#define _DP_DEV_H_

#include "linlondp_dev.h"
#include "linlondp_pipeline.h"
#include "dp_regs.h"

struct dp_pipeline {
    struct linlondp_pipeline base;

    /* dp private pipeline blocks */
    u32 __iomem    *lpu_addr;
    u32 __iomem    *cu_addr;
    u32 __iomem    *dou_addr;
    u32 __iomem    *dou_ft_coeff_addr; /* forward transform coeffs table */
};

struct dp_dev {
    struct linlondp_dev *mdev;

    int    num_blocks;
    int    num_pipelines;
    int    num_rich_layers;
    u32    max_line_size;
    u32    max_vsize;
    u32    supports_dual_link : 1;
    u32    integrates_tbu : 1;

    /* global register blocks */
    u32 __iomem    *gcu_addr;
    /* scaling coeffs table */
    u32 __iomem    *glb_scl_coeff_addr[DP_MAX_GLB_SCL_COEFF];
    u32 __iomem    *periph_addr;

    struct dp_pipeline *pipes[DP_MAX_PIPELINE];
    struct linlondp_coeffs_manager *it_mgr;
    struct linlondp_coeffs_manager *ft_mgr;
    struct linlondp_coeffs_manager *it_s_mgr;
};

#define to_dp_pipeline(x)    container_of(x, struct dp_pipeline, base)

extern const struct linlondp_pipeline_funcs dp_pipeline_funcs;

int dp_probe_block(struct dp_dev *dp,
            struct block_header *blk, u32 __iomem *reg);
void dp_read_block_header(u32 __iomem *reg, struct block_header *blk);

void dp_dump(struct linlondp_dev *mdev, struct seq_file *sf);

void dp_close_gop(struct linlondp_dev *mdev);

int dp_pipeline_config_axi(struct dp_pipeline *dp_pipe);

#endif /* !_DP_DEV_H_ */
