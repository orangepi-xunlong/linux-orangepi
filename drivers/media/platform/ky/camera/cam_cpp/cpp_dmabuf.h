/* SPDX-License-Identifier: GPL-2.0 */
/*
 * lizhirong <zhirong.li@ky.com>
 *
 * Copyright (C) 2023 KY Micro Limited
 */

#ifndef __CPP_DMABUF_H__
#define __CPP_DMABUF_H__

#include <linux/types.h>
#include <linux/device.h>
#include <media/x1/x1_cpp_uapi.h>

struct cpp_device;

enum cpp_mac_dma_port {
	MAC_DMA_PORT_R0,
	MAC_DMA_PORT_R1,
	MAC_DMA_PORT_W0,
	MAX_DMA_PORT,
};

enum mac_dma_channel_type {
	MAC_DMA_CHNL_DWT_Y_L0,
	MAC_DMA_CHNL_DWT_C_L0,
	MAC_DMA_CHNL_DWT_Y_L1,
	MAC_DMA_CHNL_DWT_C_L1,
	MAC_DMA_CHNL_DWT_Y_L2,
	MAC_DMA_CHNL_DWT_C_L2,
	MAC_DMA_CHNL_DWT_Y_L3,
	MAC_DMA_CHNL_DWT_C_L3,
	MAC_DMA_CHNL_DWT_Y_L4,
	MAC_DMA_CHNL_DWT_C_L4,
	MAC_DMA_CHNL_KGAIN_L0,
	MAC_DMA_CHNL_KGAIN_L1,
	MAC_DMA_CHNL_KGAIN_L2,
	MAC_DMA_CHNL_KGAIN_L3,
	MAC_DMA_CHNL_KGAIN_L4,
	MAC_DMA_CHNL_FBC_HEADER,
	MAC_DMA_CHNL_FBC_PAYLOAD,
	MAX_DMA_CHNLS,
};

struct cpp_dma_chnl_info {
	int32_t fd;
	unsigned int dbuf_mapped:1;
	unsigned int synced:1;
	unsigned int prepared:1;
	unsigned int need_cache_sync:1;
	unsigned int tid;
	uint8_t chnl_id;
	uint32_t offset;
	uint32_t length;
	int mmu_attached;
	dma_addr_t phy_addr;	/* cpp dmac addr */
	uint32_t *tt_base;	/* translation table cpu base */
	uint32_t tt_size;	/* translation table size */
};

struct cpp_dma_port_info {
	uint8_t port_id;
	bool fbc_enabled;
	struct cpp_dma_chnl_info dma_chnls[MAX_DMA_CHNLS];
	// dma_addr_t trans_tab_dma_addr;
	// void *trans_tab_cpu_addr;
	// size_t total_trans_tab_sz;
};

int cpp_dma_alloc_iommu_channels(struct cpp_device *cpp_dev,
				 struct cpp_dma_port_info *dma_info);

int cpp_dma_free_iommu_channels(struct cpp_device *cpp_dev,
				struct cpp_dma_port_info *dma_info);

int cpp_dma_fill_iommu_channels(struct cpp_device *cpp_dev,
				struct cpp_dma_port_info *dma_info);

struct cpp_dma_port_info *cpp_dmabuf_prepare(struct cpp_device *cpp_dev,
					     struct cpp_buffer_info *buf_info,
					     uint8_t blk_id);
void cpp_dmabuf_cleanup(struct cpp_device *cpp_dev, struct cpp_dma_port_info *dma_info);

void cpp_dmabuf_debug_dump(struct cpp_dma_port_info *dma_info);
#endif
