/*
*
* Copyright 2024 Cix Technology Group Co., Ltd.
*
*/

#ifndef __CSI_BRIDGE_HW_H__
#define __CSI_BRIDGE_HW_H__

#include "csi_common.h"

/* CSI bridge Registers Define */
#define CSI_BRIDGE_CTRL				0x0
#define DMA_BRIDGE_CTRL				0x04
#define BRIDGE_STATUS				0x08
#define CNT_EN_CTRL					0x0C
#define PIXETL_CNT_TOTAL			0x10
#define LINE_CNT_TOTAL				0x14
#define FRAME_CNT					0x18
#define PKT_INFO0					0x1C
#define PKT_INFO1					0x20
#define PKT_INFO2					0x24
#define FRAME_CFG					0x28

#define OT_INFO_CFG					0x100
#define OUT_CTRL					0x104
#define AXI_CTRL					0x108
#define AXI_STAUS					0x10C
#define AXI_USER0					0x110
#define AXI_USER1					0x114
#define AXI_ADDR_START0_LOW			0x118
#define AXI_ADDR_START0_HIGH		0x11C
#define AXI_ADDR_START1_LOW			0x120
#define AXI_ADDR_START1_HIGH		0x124
#define CURRENT_ADDR0_LO			0x128
#define CURRENT_ADDR0_HI			0x12C
#define CURRENT_ADDR1_LO			0x130
#define CURRENT_ADDR1_HI			0x134
#define LINE_STRIDE					0x138
#define VSYNC_WIDTH					0x13C
#define NSAID						0x140
#define AW_PROT						0x144

#define INTERRUTP_EN				0x200
#define INTERRUPT_STATUS			0x204
#define INTERRUPT_CLR				0x208
#define PWR_CTRL					0x20C
#define TIMEOUT_CTRL				0x210
#define LINE_INT_CTRL				0x214

#define DBG_REG0					0x300
#define DBG_REG1					0x304
#define DBG_REG2					0x308

/*CSI_BRIDGE_CTRL*/
#define  ASYNC_BRIDGE_EN_OFFSET		0
#define  ASYNC_BRIDGE_EN_MASK		0x1
#define  DMA_BRIDGE_EN_OFFSET		1
#define  DMA_BRIDGE_EN_MASK			0x2

/*DMA_BRIDGE_CTRL*/
#define FIFO_START_OFFSET			0
#define FIFO_START_MASK				0x1
#define FIFO_STOP_OFFSET			1
#define FIFO_STOP_MASK				0x2
#define DMA_START_OFFSET			2
#define DMA_START_MASK				0x4
#define DMA_STOP_OFFSET				3
#define DMA_STOP_MASK				0x8

/*BRIDGE_STATUS*/
#define ASYNC_FIFO_STATUS_OFFSET	0
#define ASYNC_FIFO_STATUS_MASK		0x3
#define DMA_ENGINE_STATUS_OFFSET	2
#define DMA_ENGINE_STATUS_MASK		0xC

/*CNT_EN_CTRL*/
#define PIXEL_TOTAL_CNT_EN_OFFSET	0
#define PIXEL_TOTAL_CNT_EN_MASK		0x1
#define LINE_TOTAL_CNT_EN_OFFSET	1
#define LINE_TOTAL_CNT_EN_MASK		0x2
#define FRAME_CNT_EN_OFFSET			2
#define FRAME_CNT_EN_MASK			0x4
#define PIXEL_CNT_PER_LINE_EN_OFFSET	3
#define PIXEL_CNT_PER_LINE_EN_MASK		0x8
#define LINE_CNT_PER_FRAME_EN_OFFSET	4
#define LINE_CNT_PER_FRAME_EN_MASK		0x10

/*PIXEL_CNT_TOTAL*/
#define TOTAL_PIXEL_CNT_OFFSET		0
#define TOTAL_PIXEL_CNT_MASK		0xFFFFFFFF

/*LINE_CNT_TOTAL*/
#define TOTAL_LINE_CNT_OFFSET		0
#define TOTAL_LINE_CNT_MASK			0xFFFFFFFF

/*FRAME_CNT*/
#define FRAME_CNT_OFFSET			0
#define FRAME_CNT_MASK				0xFFFFFFFF

/*PKT_INFO0*/
#define PIXEL_CNT_PER_LINE_OFFSET	0
#define PIXEL_CNT_PER_LINE_MASK		0xFFFF
#define LINE_CNT_PER_FRAME_OFFSET	16
#define LINE_CNT_PER_FRAME_MASK		0xFFFF0000

/*PKT_INFO1*/
#define PKT_INFO1_WC_OFFSET			0
#define PKT_INFO1_WC_MASK			0xFFFF
#define PKT_INFO1_VC_OFFSET			16
#define PKT_INFO1_VC_MASK			0xF0000
#define PKT_INFO1_DT_OFFSET			20
#define PKT_INFO1_DT_MASK			0x3F00000

/*PKT_INFO2*/
#define PKT_INFO2_CRC_OFFSET		0
#define PKT_INFO2_CRC_MASK			0xFFFFFFFF

/*FRAME_CFG*/
#define FRAME_NUM_OFFSET			0
#define FRAME_NUM_MASK				0xFFFF
#define LINE_NUM_OFFSET				16
#define LINE_NUM_MASK				0xFFFF0000

/*OT_INFO_CFG*/
#define OT_CFG_Y_OFFSET				0
#define OT_CFG_Y_MASK				0x7F
#define OT_CFG_U_OFFSET				7
#define OT_CFG_U_MASK				0x3F80
#define PENDING_REQ_NUM_Y_OFFSET	14
#define PENDING_REQ_NUM_Y_MASK		0x1FC000
#define PENDING_REQ_NUM_U_OFFSET	21
#define PENDING_REQ_NUM_U_MASK		0xFE00000

/*OUT_CTRL*/
#define MEM_LAYOUT_FMT_OFFSET		0
#define MEM_LAYOUT_FMT_MASK			0xF
#define STRIDE_OFFSET				4
#define STRIDE_MASK					0x30
#define COMP_SWAP_OFFSET			6
#define COMP_SWAP_MASK				0x40
#define LEGACY_YUV_OFFSET			7
#define LEGACY_YUV_MASK				0x80
#define VC_SYNC_FIFO_OFFSET			8
#define VC_SYNC_FIFO_MASK			0xF00
#define DT_SYNC_FIFO_OFFSET			12
#define DT_SYNC_FIFO_MASK			0x3F000
#define VC_ASYNC_FIFO_OFFSET		18
#define VC_ASYNC_FIFO_MASK			0x3C0000
#define DT_ASYNC_FIFO_OFFSET		22
#define DT_ASYNC_FIFO_MASK			0xFC00000
#define VC_EN_OFFSET				28
#define VC_EN_MASK					0x10000000
#define DT_EN_OFFSET				29
#define DT_EN_MASK					0x20000000

/*AXI_CTRL*/
#define QOS_OFFSET					0
#define QOS_MASK					0xF
#define AXCACHE_OFFSET				4
#define AXCACHE_MASK				0xF0
#define BURST_TYPE_OFFSET			8
#define BURST_TYPE_MASK				0x300
#define BURST_LEN_OFFSET			10
#define BURST_LEN_MASK				0x3C00

/*AXI_STATUS*/
#define ERROR_STATUS_OFFSET			0
#define ERROR_STATUS_MASK			0x3

/*AXI_USER0*/
#define STREAM_ID_OFFSET			0
#define STREAM_ID_MASK				0xFFFF
#define MASTER_ID_OFFSET			16
#define MASTER_ID_MASK				0xFF0000

/*AXI_USER1*/
#define SUB_STREAM_ID_OFFSET		0
#define SUB_STREAM_ID_MASK			0xFFFFF
#define SUB_STREAM_ID_VALID_OFFSET	20
#define SUB_STREAM_ID_VALID_MASK	0x100000

/*AXI_START_ADDR0_LO*/
#define ADDRESS0_LO_OFFSET			0
#define ADDRESS0_LO_MASK			0xFFFFFFFF

/*AXI_START_ADDR0_HI*/
#define ADDRESS0_HI_OFFSET			0
#define ADDRESS0_HI_MASK			0xFFFFFFFF

/*AXI_START_ADDR1_LO*/
#define ADDRESS1_LO_OFFSET			0
#define ADDRESS1_LO_MASK			0xFFFFFFFF

/*AXI_START_ADDR1_HI*/
#define ADDRESS1_HI_OFFSET			0
#define ADDRESS1_HI_MASK			0xFFFFFFFF

/*CURRENT_ADDR0_LO*/
#define CUR_ADDR0_LO_OFFSET			0
#define CUR_ADDR0_LO_MASK			0xFFFFFFFF

/*CURRENT_ADDR0_HI*/
#define CUR_ADDR0_HI_OFFSET			0
#define CUR_ADDR0_HI_MASK			0xFFFFFFFF

/*CURRENT_ADDR1_LO*/
#define CUR_ADDR1_LO_OFFSET			0
#define CUR_ADDR1_LO_MASK			0xFFFFFFFF

/*CURRENT_ADDR1_HI*/
#define CUR_ADDR1_HI_OFFSET			0
#define CUR_ADDR1_HI_MASK			0xFFFFFFFF

/* LINE_STRIDE */
#define LINE_STRIDE_OFFSET			0
#define LINE_STRIDER_MASK			0xFFFFFFFF

/* VSYNC_WIDTH */
#define VSYNC_WIDTH_OFFSET			0
#define VSYNC_WIDTH_MASK			0x1

/* NSAID */
#define NSAID_OFFSET				0
#define NSAID_MASK					0xF

/* AW_PROT */
#define AW_PROT_OFFSET				0
#define AW_PROT_MASK				0xF

/* INTERRUPT_ENABLE/INTERRUPT_STATUS/INTERRUPT_CLR */
#define FRAME_START_INT_EN_OFFSET		0
#define FRAME_START_INT_EN_MASK			0x1
#define FRAME_END_INT_EN_OFFSET			1
#define FRAME_END_INT_EN_MASK			0x2
#define LINE_CNT_INT_EN_OFFSET			2
#define LINE_CNT_INT_EN_MASK			0x4
#define LINE_MODE_INT_EN_OFFSET			3
#define LINE_MODE_INT_EN_MASK			0x8
#define PIXEL_ERR_INT_EN_OFFSET			4
#define PIXEL_ERR_INT_EN_MAKS			0x10
#define ASYNC_FIFO_OVF_INT_EN_OFFSET	5
#define ASYNC_FIFO_OVF_INT_EN_MASK		0x20
#define ASYNC_FIFO_UNDF_INT_EN_OFFSET	6
#define ASYNC_FIFO_UNDF_INT_EN_MASK		0x40
#define DMA_OVF_INT_EN_OFFSET			7
#define DMA_OVF_INT_EN_MASK				0x80
#define DMA_UNDF_INT_EN_OFFSET			8
#define DMA_UNDF_INT_EN_MASK			0x100
#define FRAME_STOP_INT_EN_OFFSET		9
#define FRAME_STOP_INT_EN_MASK			0x200
#define DMA_ERR_RST_INT_EN_OFFSET		10
#define DMA_ERR_RST_INT_EN_MASK			0x400
#define UNSUPPORT_DT_INT_EN_OFFSET		11
#define UNSUPPORT_DT_INT_EN_MASK		0x800
#define UNSUPPORT_STRIDE_INT_EN_OFFSET	12
#define UNSUPPORT_STRIDE_INT_EN_MASK	0x1000
#define LINE_MISMATCH_INT_EN_OFFSET		13
#define LINE_MISMATCH_INT_EN_MASK		0x2000
#define PIXEL_MISMATCH_INT_EN_OFFSET	14
#define PIXEL_MISMATCH_INT_EN_MASK		0x4000
#define TIMEOUT_INT_EN_OFFSET			15
#define TIMEOUT_INT_EN_MASK				0x8000

/*PWR_CTRL*/
#define SRAM_SD_OFFSET					0
#define SRAM_SD_MASK					0x1
#define SRAM_PM_STA_OFFSET				1
#define SRAM_PM_STA_MASK				0x2

/*TIMEOUT_CTRL*/
#define TIME_OUT_OFFSET					0
#define TIME_OUT_MASK					0xFFFFFFFF

/*LINE_INT_CTRL*/
#define LINE_CNT_INT_CTRL_OFFSET		0
#define LINE_CNT_INT_CTRL_MASK			0xFFFF
#define LINE_MODE_INT_CTRL_OFFSET		16
#define LINE_MODE_INT_CTRL_MASK			0xFFFF0000

/* DBG_REG0 */
#define ERROR_ADDR_LO_OFFSET			0
#define ERROR_ADDR_LO_MASK				0xFFFFFFFF

/* DBG_REG1 */
#define ERROR_ADDR_HI_OFFSET			0
#define ERROR_ADDR_HI_MASK				0xFFFFFFFF

/* DBG_REG2 */
// TODO

/* ASYNC Status */
#define ASYNC_STATUS_IDLE				0
#define ASYNC_STATUS_RUNNING			1
#define ASYNC_STATUS_STARTED			2
#define ASYNC_STATUS_STOPPED			3

/* DMA Status */
#define DMA_STATUS_IDLE					0
#define DMA_STATUS_RUNNING				1
#define DMA_STATUS_STARTED				2
#define DMA_STATUS_STOPPED				3

#define ADDR_HI_OFFSET					32
#define CHECK_MAX_CNT					10

/* AXI Config */
#define AXI_CTRL_QOS					0
#define AXI_CTRL_AXCACHE				0
#define AXI_CTRL_BURST_TYPE				1
#define AXI_CTRL_BURST_LEN				7

/* AXI Security */
#define AW_PROT_VAL						0
#define NSAID_VAL						0

/* AXI Outstanding */
#define OT_CFG_Y						1 << 6
#define OT_CFG_U						1 << 6
#define OT_CFG_PENDING_REQ_NUM_Y		0
#define OT_CFG_PENDING_REQ_NUM_U		0

#define PIXEL_TOTAL_CNT_EN				1
#define LINE_TOTAL_CNT_EN				1
#define FRAME_CNT_EN					1
#define PIXEL_CNT_PER_LINE_EN			1
#define LINE_CNT_PER_FRAME_EN			1

/* Data Type - YUV */
#define IMG_DT_YUV420_8BIT				0x18
#define IMG_DT_YUV420_10BIT				0X19
#define IMG_DT_LEGACY_YUV420_8BIT		0x1A
#define IMG_DT_YUV422_8BIT				0x1E
#define IMG_DT_YUV422_10BIT				0X1F

/* Data Type - RGB */
#define IMG_DT_RGB565					0X22
#define IMG_DT_RGB888					0x24

/* Data Type - RAW */
#define IMG_DT_RAW8						0x2A
#define IMG_DT_RAW10					0x2B
#define IMG_DT_RAW12					0x2C
#define IMG_DT_RAW14					0x2D
#define IMG_DT_RAW16					0x2E

/* Resolution */
#define FRAME_NUM						1920
#define LINE_NUM						1080

/* Memory Layout Format */
#define MEM_LAYOUT_FMT_RGB888			0
#define MEM_LAYOUT_FMT_RGB888X			1
#define MEM_LAYOUT_FMT_RGB565			2
#define MEM_LAYOUT_FMT_RAW16			2
#define MEM_LAYOUT_FMT_NV12			3
#define MEM_LAYOUT_FMT_P010			4
#define MEM_LAYOUT_FMT_YUYV			5
#define MEM_LAYOUT_FMT_YUYV10			6
#define MEM_LAYOUT_FMT_RAW			7

/* Stride */
#define STRIDE_16BYTE					0
#define STRIDE_32BYTE					1
#define STRIDE_64BYTE					2

#define COMP_SWAP						1
#define LEGACY_YUV						1

#define SRAM_SD							1

/* StreamOn & StreamOff */
#define ASYNC_BRIDGE_OFF				0
#define DMA_BRIDGE_OFF					0
#define ASYNC_BRIDGE_EN					1
#define DMA_BRIDGE_EN					1

#define START							1
#define STOP							1

void csi_dma_channel_enable(struct csi_dma_dev *csi_dma);
void csi_dma_channel_disable(struct csi_dma_dev *csi_dma);
void csi_dma_set_timeout(struct csi_dma_dev *csi_dma, u32 val);
u32 csi_dma_axi_burst_config(struct csi_dma_dev *csi_dma, u32 burst_len);
u32 csi_dma_axi_ot_config(struct csi_dma_dev *csi_dma, u32 ot_y, u32 ot_u);
u32 csi_dma_axi_pendreq_config(struct csi_dma_dev *csi_dma, u32 pend_y, u32 pend_u);
void csi_dma_cap_frame_write_done(struct csi_dma_dev *csi_dma);
void csi_dma_channel_set_outbuf(struct csi_dma_dev *csi_dma,
				struct csi_dma_buffer *buf);
void csi_dma_bridge_start(struct csi_dma_dev *csi_dma,
				struct csi_dma_frame *src_f);
void csi_dma_bridge_stop(struct csi_dma_dev *csi_dma);
void csi_dma_clean_irq_status(struct csi_dma_dev *csi_dma, u32 val);
void csi_dma_clean_registers(struct csi_dma_dev *csi_dma);
void csi_dma_enable_irq(struct csi_dma_dev *csi_dma);
void csi_dma_disable_irq(struct csi_dma_dev *csi_dma);
void csi_dma_pwr_ctrl(struct csi_dma_dev *csi_dma);
u32 csi_dma_get_pwr_status(struct csi_dma_dev *csi_dma);
#ifdef DEBUG
void dump_csi_bridge_regs(struct csi_dma_dev *csi_dma);
#endif
u32 csi_dma_get_irq_status(struct csi_dma_dev *csi_dma);
struct device *csi_dma_dev_get_parent(struct platform_device *pdev);
struct csi_dma_dev *csi_dma_get_hostdata(struct platform_device *pdev);

int csi_dma_clk_enable(struct csi_dma_dev *csi_dma);
void csi_dma_clk_disable(struct csi_dma_dev *csi_dma);
int csi_dma_resets_assert(struct csi_dma_dev *csi_dma);
int csi_dma_resets_deassert(struct csi_dma_dev *csi_dma);

void csi_dma_store(struct csi_dma_dev *csi_dma);
void csi_dma_restore(struct csi_dma_dev *csi_dma);

#endif /* __CSI_BRIDGE_HW_H__ */
