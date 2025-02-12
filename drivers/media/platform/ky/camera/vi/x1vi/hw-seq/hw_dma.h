// SPDX-License-Identifier: GPL-2.0
/*
 * hw_dma.h - isp front end dma hw sequence
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#ifndef _HW_DMA_H_
#define _HW_DMA_H_
#include "../../cam_block.h"
#define KY_ISP_DMA_OFFSET	(0x00011000)

struct wdma_fifo_ctrl {
	unsigned int offset;
	unsigned int depth;
	unsigned int weight;
	unsigned int div_mode;
};

void hw_dma_set_wdma_pitch(struct spm_camera_block *sc_block,
			   unsigned int wdma_ch,
			   unsigned int num_plane,
			   unsigned int p0_pitch, unsigned int p1_pitch);
void hw_dma_set_rdma_pitch(struct spm_camera_block *sc_block,
			   unsigned int rdma_ch, unsigned int pitch);
void hw_dma_update_rdma_address(struct spm_camera_block *sc_block,
				unsigned int rdma_ch, uint64_t buf_addr);
void hw_dma_update_wdma_address(struct spm_camera_block *sc_block,
				unsigned int wdma_ch,
				uint64_t p0_addr, uint64_t p1_addr);
void hw_dma_set_wdma_ready(struct spm_camera_block *sc_block,
			   unsigned int wdma_ch, unsigned int ready);
/*
void hw_dma_set_rdma_weight(unsigned int base_addr,
							unsigned int rdma_ch,
							unsigned int weight);
*/
void hw_dma_rdma_trigger(struct spm_camera_block *sc_block, unsigned int rdma_ch);
//void hw_dma_set_wdma_burst_length(struct spm_camera_block *sc_block, unsigned char burst_len);
void hw_dma_reset(struct spm_camera_block *sc_block);

enum {
	RAWDUMP0 = 0,
	RAWDUMP1,
	FORMATTER0,
	FORMATTER1,
	FORMATTER2,
	WBMP0 = FORMATTER2,
	EISP0 = FORMATTER2,
	FORMATTER3,
	WBMP1 = FORMATTER3,
	EISP1 = FORMATTER3,
	FORMATTER4,
	AEMP0 = FORMATTER4,
	FORMATTER5,
	AEMP1 = FORMATTER5,
	DWT0_LAYER1,
	AFCP0 = DWT0_LAYER1,
	DWT0_LAYER2,
	AFCP1 = DWT0_LAYER2,
	DWT0_LAYER3,
	DWT0_LAYER4,
	DWT1_LAYER1,
	DWT1_LAYER2,
	DWT1_LAYER3,
	DWT1_LAYER4,
};
void hw_dma_set_wdma_source(struct spm_camera_block *sc_block,
			    unsigned int wdma_ch,
			    int source,
			    unsigned int wr_offset,
			    unsigned int wr_fifo_depth,
			    unsigned int wr_weight, unsigned int div_mode);

unsigned int hw_dma_get_irq_status1(struct spm_camera_block *sc_block);
void hw_dma_clr_irq_status1(struct spm_camera_block *sc_block, unsigned int clr);
unsigned int hw_dma_get_irq_status2(struct spm_camera_block *sc_block);
void hw_dma_clr_irq_status2(struct spm_camera_block *sc_block, unsigned int clr);
unsigned int hw_dma_get_irq_raw_status1(struct spm_camera_block *sc_block);
unsigned int hw_dma_get_irq_raw_status2(struct spm_camera_block *sc_block);
enum {
	DMA_IRQ_SRC_ALL = -1,
	DMA_IRQ_SRC_WDMA_CH0,
	DMA_IRQ_SRC_WDMA_CH1,
	DMA_IRQ_SRC_WDMA_CH2,
	DMA_IRQ_SRC_WDMA_CH3,
	DMA_IRQ_SRC_WDMA_CH4,
	DMA_IRQ_SRC_WDMA_CH5,
	DMA_IRQ_SRC_WDMA_CH6,
	DMA_IRQ_SRC_WDMA_CH7,
	DMA_IRQ_SRC_WDMA_CH8,
	DMA_IRQ_SRC_WDMA_CH9,
	DMA_IRQ_SRC_WDMA_CH10,
	DMA_IRQ_SRC_WDMA_CH11,
	DMA_IRQ_SRC_WDMA_CH12,
	DMA_IRQ_SRC_WDMA_CH13,
	DMA_IRQ_SRC_WDMA_CH14_P0,
	DMA_IRQ_SRC_WDMA_CH14_P1,
	DMA_IRQ_SRC_WDMA_CH15,
	DMA_IRQ_SRC_RDMA_CH0,
	DMA_IRQ_SRC_RDMA_CH1,
	DMA_IRQ_SRC_RDMA_CH2,
};

#define DMA_IRQ_START		(1 << 0)
#define DMA_IRQ_DONE		(1 << 1)
#define DMA_IRQ_ERR		(1 << 2)
#define DMA_IRQ_ALL		(DMA_IRQ_START | DMA_IRQ_DONE | DMA_IRQ_ERR)

void hw_dma_set_irq_enable(struct spm_camera_block *sc_block, int irq_src,
			   unsigned int enable, unsigned int disable);
unsigned int hw_dma_irq_analyze(int irq_src, unsigned int status1,
				unsigned int status2);

#define DMA_IRQ_FBC_ENC0	(1 << 28)
#define DMA_IRQ_FBC_ENC1	(1 << 29)
#define DMA_IRQ_OVERRUN		(1 << 30)
#define DMA_IRQ_OVERLAP		(1 << 31)
void hw_dma_set_fbc_irq_enable(struct spm_camera_block *sc_block, unsigned int enable,
			       unsigned int disable);
void hw_dma_enable_rawdump(struct spm_camera_block *sc_block, int rawdump_id,
			   unsigned int enable);
void hw_dma_set_wdma_weight(struct spm_camera_block *sc_block, unsigned int wdma_ch,
			    unsigned int wr_weight);
void hw_dma_enable_afbc(struct spm_camera_block *sc_block, int afbc_id,
			unsigned int enable);
void hw_dma_trigger_pipe_tbu_load(struct spm_camera_block *sc_block, int pipe_id);
void hw_dma_trigger_rdp_tbu_load(struct spm_camera_block *sc_block, int rawdump_id);
void __hw_dma_set_irq_enable(int irq_src,
			     unsigned int enable,
			     unsigned int disable,
			     unsigned int *value1, unsigned int *value2);
void hw_dma_dump_regs(struct spm_camera_block *sc_block);
#endif
