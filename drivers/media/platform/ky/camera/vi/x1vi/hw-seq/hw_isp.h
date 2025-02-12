// SPDX-License-Identifier: GPL-2.0
/*
 * hw_isp.h - isp top hw sequence
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#ifndef _HW_ISP_H_
#define _HW_ISP_H_
#include "../../cam_block.h"
#define KY_PIPE_OFFSET		(0x00008000)
#define KY_ISP_TOP0_OFFSET	(0x00001700)
#define KY_ISP_TOP1_OFFSET	(KY_ISP_TOP0_OFFSET + KY_PIPE_OFFSET)
#define ERR2_PIPE0_OVERRUN		(1 << 28)
#define ERR2_PIPE1_OVERRUN		(1 << 29)

enum {
	RGGB = 0,
	GRBG,
	GBRG,
	BGGR,
	CFA_IGNR,
};

int hw_isp_top_set_idi_online_input_fmt(struct spm_camera_block *sc_block,
					unsigned int width,
					unsigned int height, int cfa_pattern);
int hw_isp_top_set_idi_offline_input_fmt(struct spm_camera_block *sc_block,
					 unsigned int rdma_ch,
					 unsigned int width,
					 unsigned int height,
					 int cfa_pattern, unsigned int bit_depth);
void hw_isp_top_set_rawdump_fmt(struct spm_camera_block *sc_block,
				unsigned int idi_wdma_ch,
				unsigned int width,
				unsigned int height, unsigned int bit_depth);

//void hw_isp_top_set_reg_14(struct spm_camera_block *sc_block, unsigned int width, unsigned int idi_insert_dummy_line);
void hw_isp_top_set_idi_dummyline(struct spm_camera_block *sc_block,
				  unsigned int idi_insert_dummy_line);
void hw_isp_top_set_idi_linebuf_depth(struct spm_camera_block *sc_block,
				      unsigned int width, unsigned int percentage);
void hw_isp_top_set_idi_linebuf(struct spm_camera_block *sc_block,
				unsigned int fifo_depth, unsigned int line_depth,
				unsigned int pix_depth);

enum {
	INVALID_CH = -1,
	SENSOR0_CH0 = 0,
	SENSOR0_CH1,
	SENSOR0_CH2,
	SENSOR0_CH3,
	SENSOR1_CH0,
	SENSOR1_CH1,
	SENSOR1_CH2,
	SENSOR1_CH3,
	OFFLINE_CH0,
	OFFLINE_CH1,
};

void hw_isp_top_set_idi_input_source(struct spm_camera_block *sc_block, int source);
void hw_isp_top_set_rawdump_source(struct spm_camera_block *sc_block,
				   unsigned int rawdump_idx, int source);
void hw_isp_top_enable_rawdump(struct spm_camera_block *sc_block, int enable,
			       int rawdump_only);
//void hw_isp_top_set_vsync2href_dly_cnt(struct spm_camera_block *sc_block, unsigned int p0_dly_cnt, unsigned int p1_dly_cnt);
void hw_isp_top_set_vsync2href_dly_cnt(struct spm_camera_block *sc_block, int pipe_id,
				       unsigned int dly_cnt);
void hw_isp_top_set_gap_value(struct spm_camera_block *sc_block, unsigned int p0_gap,
			      unsigned int p1_gap, unsigned int p01_gap);
void hw_isp_top_set_idi_rd_burst_len(struct spm_camera_block *sc_block,
				     unsigned int rdma_ch, unsigned int speed_cnt,
				     unsigned int bstlen_sel);
void hw_isp_top_enable_hw_gap(struct spm_camera_block *sc_block, int pipe_id,
			      unsigned int enable);

enum {
	HDR_NONE = 0,
	HDR_OFFLINE,
	HDR_ONLINE,
	HDR_MIX,
};

void hw_isp_top_enable_hdr(struct spm_camera_block *sc_block, int hdr_mode);
void hw_isp_top_enable_rd_outstanding(struct spm_camera_block *sc_block, unsigned int enable);
void hw_isp_top_set_mix_hdr_line(struct spm_camera_block *sc_block, unsigned int mix_hdr_line);
void hw_isp_top_set_ddr_wr_line(struct spm_camera_block *sc_block, unsigned int ddr_wr_line_cnt);
void hw_isp_top_enable_vsync_pass_through(struct spm_camera_block *sc_block, int pipe_id, unsigned int enable);
void hw_isp_pwr(unsigned int on);
unsigned int hw_isp_top_get_irq_status(struct spm_camera_block *sc_block);
void hw_isp_top_clr_irq_status(struct spm_camera_block *sc_block, unsigned int clr);
unsigned int hw_isp_top_get_irq_raw_status(struct spm_camera_block *sc_block);
void hw_isp_top_set_cfg_rdy(struct spm_camera_block *sc_block, unsigned int ready);
void hw_isp_top_set_rdp_cfg_rdy(struct spm_camera_block *sc_block,
				unsigned int rawdump_id, unsigned int ready);
void hw_isp_top_shadow_latch(struct spm_camera_block *sc_block);

#define ISP_IRQ_PIPE_SOF		(1 << 0)
#define ISP_IRQ_PDC_SOF			(1 << 1)
#define ISP_IRQ_PDF_SOF			(1 << 2)
#define ISP_IRQ_BPC_SOF			(1 << 3)
#define ISP_IRQ_LSC_SOF			(1 << 4)
#define ISP_IRQ_DNS_SOF			(1 << 5)
#define ISP_IRQ_BINNING_SOF		(1 << 6)
#define ISP_IRQ_DEMOSAIC_SOF		(1 << 7)
#define ISP_IRQ_HDR_SOF			(1 << 8)
#define ISP_IRQ_LTM_SOF			(1 << 9)
#define ISP_IRQ_MCU_TRIGGER		(1 << 10)
#define ISP_IRQ_STATS_ERR		(1 << 11)
#define ISP_IRQ_SDE_SOF			(1 << 12)
#define ISP_IRQ_SDE_EOF			(1 << 13)
#define ISP_IRQ_G_RST_DONE		(1 << 14)
#define ISP_IRQ_IDI_SHADOW_DONE		(1 << 15)
#define ISP_IRQ_PIPE_EOF		(1 << 16)
#define ISP_IRQ_PDC_EOF			(1 << 17)
#define ISP_IRQ_PDF_EOF			(1 << 18)
#define ISP_IRQ_BPC_EOF			(1 << 19)
#define ISP_IRQ_LSC_EOF			(1 << 20)
#define ISP_IRQ_DNS_EOF			(1 << 21)
#define ISP_IRQ_BINNING_EOF		(1 << 22)
#define ISP_IRQ_DEMOSAIC_EOF		(1 << 23)
#define ISP_IRQ_HDR_EOF			(1 << 24)
#define ISP_IRQ_LTM_EOF			(1 << 25)
#define ISP_IRQ_AEM_EOF			(1 << 26)
#define ISP_IRQ_WBM_EOF			(1 << 27)
#define ISP_IRQ_LSCM_EOF		(1 << 28)
#define ISP_IRQ_AFC_EOF			(1 << 29)
#define ISP_IRQ_FLICKER_EOF		(1 << 30)
#define ISP_IRQ_ERR			(1 << 31)
#define ISP_IRQ_ALL			(0xffffffff)

void hw_isp_top_set_irq_enable(struct spm_camera_block *sc_block, unsigned int enable,
			       unsigned int disable);
void hw_isp_top_set_err0_irq_enable(struct spm_camera_block *sc_block,
				    unsigned int enable, unsigned int disable);
void hw_isp_top_set_err2_irq_enable(struct spm_camera_block *sc_block,
				    unsigned int enable, unsigned int disable);
#define POSTERR_IRQ_RDP0_SDW_OPEN_DONE	(1 << 18)
#define POSTERR_IRQ_RDP1_SDW_OPEN_DONE	(1 << 18)
#define POSTERR_IRQ_RDP0_SDW_CLOSE_DONE	(1 << 28)
#define POSTERR_IRQ_RDP1_SDW_CLOSE_DONE	(1 << 29)
#define POSTERR_IRQ_PIP0_SDW_OPEN_DONE	(1 << 26)
#define POSTERR_IRQ_PIP1_SDW_OPEN_DONE	(1 << 27)
#define POSTERR_IRQ_PIP0_SDW_CLOSE_DONE	(1 << 30)
#define POSTERR_IRQ_PIP1_SDW_CLOSE_DONE	(1 << 31)

void hw_isp_top_set_posterr_irq_enable(struct spm_camera_block *sc_block,
				       unsigned int enable, unsigned int disable);
unsigned int hw_isp_top_get_err0_irq_status(struct spm_camera_block *sc_block);
void hw_isp_top_clr_err0_irq_status(struct spm_camera_block *sc_block,
				    unsigned int clr);
unsigned int hw_isp_top_get_err2_irq_status(struct spm_camera_block *sc_block);
void hw_isp_top_clr_err2_irq_status(struct spm_camera_block *sc_block,
				    unsigned int clr);
unsigned int hw_isp_top_get_err1_irq_status(struct spm_camera_block *sc_block);
void hw_isp_top_clr_err1_irq_status(struct spm_camera_block *sc_block,
				    unsigned int clr);
unsigned int hw_isp_top_get_posterr_irq_status(struct spm_camera_block *sc_block);
void hw_isp_top_clr_posterr_irq_status(struct spm_camera_block *sc_block,
				       unsigned int clr);
void hw_isp_top_config_tpg(struct spm_camera_block *sc_block, unsigned int pipe_id,
			   unsigned int rolling, unsigned int dummy_line,
			   unsigned int hblank, unsigned int vblank);
void hw_isp_top_enable_tpg(struct spm_camera_block *sc_block, unsigned int enable);
void hw_isp_top_enable_debug_clk(struct spm_camera_block *sc_block,
				 unsigned int enable);
unsigned int hw_isp_top_get_idi_fifo_depth(struct spm_camera_block *sc_block);
void hw_isp_top_global_reset(struct spm_camera_block *sc_block);
void hw_isp_top_pipe0_debug_dump(struct spm_camera_block *sc_block);
void hw_isp_top_pipe1_debug_dump(struct spm_camera_block *sc_block);
void hw_isp_top_set_speed_ctrl(struct spm_camera_block *sc_block, unsigned int speed_ctrl);
#endif
