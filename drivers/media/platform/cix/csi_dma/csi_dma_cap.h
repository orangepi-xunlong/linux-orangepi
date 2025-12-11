/* SPDX-License-Identifier: GPL-2.0 */
/*
 *Copyright 2024 Cix Technology Group Co., Ltd.
 */
#include "csi_common.h"

int csi_dma_register_stream(struct csi_dma_dev *csi_dma);
void csi_dma_unregister_stream(struct csi_dma_dev *csi_dma);
int csi_dma_config_rcsu(struct csi_dma_cap_dev *dma_cap);
int csi_dma_register_buffer_done(struct csi_dma_dev *csi_dma);
int csi_dma_cap_enable_irq(struct csi_dma_dev *csi_dma, int enable);
int csi_dma_cap_store(struct csi_dma_dev *csi_dma_info);
int csi_dma_cap_restore(struct csi_dma_dev *csi_dma_info);
int csi_dma_cap_stream_start(struct csi_dma_dev *csi_dma_info, struct csi_dma_frame *frame);
