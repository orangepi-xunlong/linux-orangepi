/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __CSIC__DMA140__REG__H__
#define __CSIC__DMA140__REG__H__

#include <linux/types.h>
#include <media/sunxi_camera_v2.h>

#define DMA_VIRTUAL_NUM	4

#if IS_ENABLED(CONFIG_ARCH_SUN60IW1)
extern int dma_virtual_find_ch[24];
extern int dma_virtual_find_logic[24];
#else
extern int dma_virtual_find_ch[18];
extern int dma_virtual_find_logic[18];
#endif

/*
 * dma top interrupt select
 */
enum dma_top_int_sel {
	VIDEO_INPUT_TO_INT = 0X1,
	CLR_FS_FRM_CNT_INT = 0X2,
	FS_PUL_INT = 0X4,
	DMA_TOP_INT_ALL = 0X7,
};

/* register value */

enum ve_online_ch {
	CH0 = 0,
	CH1,
	CH2,
	CH3,
};

enum frm_rate_cnt_spl {
	frame_done = 0,
	vsync,
};
/*
 * YUV 10bit store format
 */
enum store_fmt {
	IN_LOW_10BIT = 0,
	IN_HIGH_10BIT,
};

/*
 * output data format
 */
enum output_fmt {
	/* only when input is raw */
	FIELD_RAW_8 = 0,
	FIELD_RAW_10 = 1,
	FIELD_RAW_12 = 2,
	FIELD_RAW_14 = 3,
	FIELD_RAW_16 = 4,
	FIELD_RAW_20 = 5,
	FIELD_RAW_24 = 6,
	FIELD_RGB565 = 0x8,
	FIELD_RGB888 = 0x9,
	FIELD_PRGB888 = 0x10,

	/*
	 *The following formats are not
	 *supported in version 140.000
	 *set these as field fmt
	 */
	FRAME_RAW_8 = 0,
	FRAME_RAW_10 = 1,
	FRAME_RAW_12 = 2,
	FRAME_RAW_14 = 3,
	FRAME_RAW_16 = 4,

	/* only when input is yuv422/yuv420 */
	FIELD_PLANAR_YUV422 = 0, /* yuv422 */
	LBC_MODE_OUTPUT = 0,     /* yuv420 */
	FIELD_PLANAR_YUV420 = 1,
	FRAME_PLANAR_YUV420 = 2,
	FRAME_PLANAR_YUV422 = 3,
	FIELD_UV_CB_YUV422 = 4,
	FIELD_UV_CB_YUV420 = 5,
	FRAME_UV_CB_YUV420 = 6,
	FRAME_UV_CB_YUV422 = 7,
	FIELD_VU_CB_YUV422 = 8,
	FIELD_VU_CB_YUV420 = 9,
	FRAME_VU_CB_YUV420 = 10,
	FRAME_VU_CB_YUV422 = 11,
	FIELD_CB_YUV400 = 13,
	FRAME_CB_YUV400 = 15,
};

/*
 * input field selection, only when input is ccir656
 */
enum field_sel {
	FIELD_1,	/* odd field */
	FIELD_2,	/* even field */
	FIELD_EITHER,	/* either field */
};

/*
 * fps down sample
 */
enum fps_ds {
	FPS_NO_DS,	/* no down sample */
	FPS_2_DS,	/* only receives the first frame every 2 frames */
	FPS_3_DS,	/* only receives the first frame every 3 frames */
	FPS_4_DS,	/* only receives the first frame every 4 frames */
	FPS_5_DS,	/* only receives the first frame every 5 frames */
	FPS_6_DS,	/* only receives the first frame every 6 frames */
	FPS_7_DS,	/* only receives the first frame every 7 frames */
	FPS_8_DS,	/* only receives the first frame every 8 frames */
	FPS_9_DS,	/* only receives the first frame every 9 frames */
	FPS_10_DS,	/* only receives the first frame every 10 frames */
	FPS_11_DS,	/* only receives the first frame every 11 frames */
	FPS_12_DS,	/* only receives the first frame every 12 frames */
	FPS_13_DS,	/* only receives the first frame every 13 frames */
	FPS_14_DS,	/* only receives the first frame every 14 frames */
	FPS_15_DS,	/* only receives the first frame every 15 frames */
	FPS_16_DS,	/* only receives the first frame every 16 frames */
};

/*
 * Minimum size of SDRAM block write
 */
enum min_sdr_wr_size {
	SDR_256B,	/* if hflip is enable, always select 256 bytes */
	SDR_512B,
	SDR_1K,
	SDR_2K,
};

/*
 * dma fifo buffer select
 */
enum fifo_buf_sel {
	FIFO0_BUF_A = 0,
	FIFO1_BUF_A,
	FIFO2_BUF_A,
};

/*
 * csi buffer
 */

enum csi_buf_sel {
	CSI_BUF_0_A = 0,    /* FIFO for Y address A */
	CSI_BUF_0_B,        /* FIFO for Y address B */
	CSI_BUF_1_A,        /* FIFO for Cb address A */
	CSI_BUF_1_B,        /* FIFO for Cb address B */
	CSI_BUF_2_A,        /* FIFO for Cr address A */
	CSI_BUF_2_B,        /* FIFO for Cr address B */
};

/*
 * dma interrupt select
 */
enum dma_int_sel {
	DMA_INT_CAPTURE_DONE = 0X1,
	DMA_INT_FRAME_DONE = 0X2,
	DMA_INT_BUF_0_OVERFLOW = 0X4,
	DMA_INT_LINE_CNT = 0X20,
	DMA_INT_HBLANK_OVERFLOW = 0X40,
	DMA_INT_VSYNC_TRIG = 0X80,
	DMA_INT_FBC_OVHD_WRDDR_FULL = 0X100,
	DMA_INT_FBC_DATA_WRDDR_FULL = 0X200,
	DMA_INT_BUF_ADDR_FIFO = 0x2000,
	DMA_INT_STORED_FRM_CNT = 0x4000,
	DMA_INT_FRM_LOST = 0x8000,
	DMA_INT_LBC_HB = 0x10000,
	DMA_INT_ADDR_NO_READY = 0x20000,
	DMA_INT_ADDR_OVERFLOW = 0x40000,
	DMA_INT_PIXEL_MISS = 0x80000,
	DMA_INT_LINE_MISS = 0x200000,
	DMA_INT_ALL = 0XFFFFFFFF,
};

/*
* total frame counter mode
*1. FRMAE_DONE: red frame_count is interrupted at bk frame done
*2. FRAME_START: red frame_count is interrupted at bk vsync flag
*3. FRAME_END: red frame_count is interrupted at vipp frame done
*/
enum frmae_cnt_mode {
	FRMAE_DONE = 0,
	FRAME_START,
	FRAME_END,
};

/* register data struct */

struct csic_dma_top_cfg {
	enum frm_rate_cnt_spl spl;
	enum min_sdr_wr_size block;
	enum ve_online_ch ve_ol_ch;
};

struct csic_dma_flip {
	unsigned int hflip_en;
	unsigned int vflip_en;
};

struct csic_dma_cfg {
	unsigned int mask_num;
	enum output_fmt fmt;
	struct csic_dma_flip flip;
	enum field_sel field;
	enum fps_ds ds;
	enum min_sdr_wr_size block;
};

struct dma_top_int_status {
	bool fsync_sig_rec;
	bool clr_fs_cnt;
	bool video_inp_to;
	unsigned int mask;
};

struct dma_version {
	unsigned int big_ver;
	unsigned int small_ver;
};

struct dma_output_size {
	unsigned int hor_len;
	unsigned int hor_start;
	unsigned int ver_len;
	unsigned int ver_start;
};

struct dma_buf_len {
	unsigned int buf_len_y;
	unsigned int buf_len_c;
};

struct dma_flip_size {
	unsigned int ver_len;
	unsigned int hor_len;
};

struct dma_capture_status {
	bool field_sta;
	bool vcap_sta;
	bool scap_sta;
};

struct dma_int_status {
	bool capture_done;
	bool frame_done;
	bool buf_0_overflow;
	bool buf_1_overflow;
	bool buf_2_overflow;
	bool line_cnt_flag;
	bool hblank_overflow;
	bool vsync_trig;
	bool fbc_ovhd_wrddr_full;
	bool fbc_data_wrddr_full;
	bool buf_addr_fifo;
	bool stored_frm_cnt;
	bool frm_lost;
	bool lbc_hb;
	bool addr_no_ready;
	bool addr_overflow;
	bool pixel_miss;
	bool line_miss;
	unsigned int mask;
};

struct dma_fifo_threshold {
	unsigned int fifo_nearly_full_th;
	unsigned int fifo_thrs;
};

struct dma_pclk_statistic {
	unsigned int pclk_cnt_line_max;
	unsigned int pclk_cnt_line_min;
};

struct dma_lbc_cmp {
	unsigned char is_lossy;
	unsigned char bit_depth;
	unsigned char glb_enable;
	unsigned char dts_enable;
	unsigned char ots_enable;
	unsigned char msq_enable;
	unsigned int cmp_ratio_even;
	unsigned int cmp_ratio_odd;
	unsigned int mb_mi_bits[2];
	unsigned char rc_adv[4];
	unsigned char lmtqp_en;
	unsigned char lmtqp_min;
	unsigned char updata_adv_en;
	unsigned char updata_adv_ratio;
	unsigned int line_tar_bits[2];
};

struct dma_bufa_threshold {
	unsigned char bufa_fifo_threshold;
	unsigned char stored_frm_threshold;
	unsigned char bufa_fifo_total;
};

/* open module */
int csic_dma_get_frame_cnt(unsigned int sel);

/* BK TOP EN */
int csic_dma_set_base_addr(unsigned int sel, unsigned long addr);
void csic_dma_top_enable(unsigned int sel);
void csic_dma_top_disable(unsigned int sel);
void csic_dma_clk_cnt_en(unsigned int sel, unsigned int en);
void csic_dma_clk_cnt_sample(unsigned int sel, unsigned int en);
void csic_frame_cnt_enable(unsigned int sel);
void csic_frame_cnt_disable(unsigned int sel);
void csic_buf_addr_fifo_en(unsigned int sel, unsigned int en);
void csic_vi_to_cnt_enable(unsigned int sel, unsigned int en);
void csic_dma_sdr_wr_size(unsigned int sel, unsigned int block);
void csic_ve_online_hs_enable(unsigned int sel);
void csic_ve_online_hs_disable(unsigned int sel);
void csic_ve_online_ch_sel(unsigned int sel, unsigned int ch);
void csic_dma_buf_length_software_enable(unsigned int sel, unsigned int en);
void csi_dam_flip_software_enable(unsigned int sel, unsigned int en);
void csic_dma_version_read_enable(unsigned int sel, unsigned int en);
/** top offset 0x4 **/
void csic_dma_mul_ch_enable(unsigned int sel, unsigned int en);
unsigned int csic_get_input_ch_id(unsigned int sel);
unsigned int csic_get_output_ch_id(unsigned int sel);
/** get ve online info **/
unsigned int csic_get_ve_frame_st_cnt(unsigned int sel);
unsigned int csic_get_ve_frame_done_cnt(unsigned int sel);
unsigned int csic_get_ve_line_st_cnt(unsigned int sel);
unsigned int csic_get_ve_line_done_cnt(unsigned int sel);
unsigned int csic_get_ve_cur_frame_addr(unsigned int sel);
unsigned int csic_get_ve_last_frame_addr(unsigned int sel);
/* top interrupt en and cfg  */
void csic_dma_top_interrupt_en(unsigned int sel, unsigned int mask);
void csic_dma_top_interrupt_disable(unsigned int sel, unsigned int mask);
void csic_get_dma_top_interrupt_status(unsigned int sel, struct dma_top_int_status *status);
void csic_get_dma_version(unsigned int sel, struct dma_version *version);
/* Multichannel */
void csic_dma_enable(unsigned int sel);
void csic_dma_disable(unsigned int sel);
void csic_lbc_enable(unsigned int sel);
void csic_lbc_disable(unsigned int sel);
void csic_fbc_enable(unsigned int sel);
void csic_fbc_disable(unsigned int sel);
void csic_dma_frame_drop_en(unsigned int sel);
void csic_dma_frame_drop_disable(unsigned int sel);
void csic_dma_output_fmt_cfg(unsigned int sel, enum output_fmt fmt);
void csic_dma_flip_en(unsigned int sel, struct csic_dma_flip *flip);
void csic_dma_cap_mask_num(unsigned int sel, unsigned int num);
void csic_dma_config(unsigned int sel, struct csic_dma_cfg *cfg);
void csic_dma_10bit_cut2_8bit_enable(unsigned int sel);
void csic_dma_10bit_cut2_8bit_disable(unsigned int sel);
void csic_frame_lost_cnt_en(unsigned int sel);
unsigned int csic_get_frame_lost_cnt(unsigned int sel);
void csic_set_threshold_for_bufa_mode(unsigned int sel, struct dma_bufa_threshold *threshold);
void csic_total_frm_cnt_enable(unsigned int sel);
void csic_total_frm_cnt_disable(unsigned int sel);
void csic_total_frm_cnt_clear(unsigned int sel);
void csic_total_frm_cnt_mode(unsigned int sel, enum frmae_cnt_mode mode);
void csic_total_frm_cnt(unsigned int sel, unsigned long *count);
void csic_dma_output_size_cfg(unsigned int sel, struct dma_output_size *size);
void csic_dma_buffer_address(unsigned int sel, enum csi_buf_sel buf, unsigned long addr);
void csic_dma_get_buffer_address(unsigned int sel, enum csi_buf_sel buf, unsigned int *addr);
void csic_dma_buffer_length(unsigned int sel, struct dma_buf_len *buf_len);
void csic_dma_flip_size(unsigned int sel, struct dma_flip_size *flip_size);
void csic_dma_cap_status(unsigned int sel, struct dma_capture_status *status);
void csic_dma_int_enable(unsigned int sel, enum dma_int_sel interrupt);
void csic_dma_int_disable(unsigned int sel, enum dma_int_sel interrupt);
void csic_dma_int_get_status(unsigned int sel, struct dma_int_status *status);
unsigned int csic_dma_get_pclk_cnt_line_min(unsigned int sel);
unsigned int csic_dma_get_pclk_cnt_line_max(unsigned int sel);
void csic_dma_int_get_status_and_chn(unsigned int sel,
			struct dma_int_status *status, unsigned int *id);
void csic_dma_line_cnt(unsigned int sel, int line);
void csic_dma_frm_cnt(unsigned int sel, struct csi_sync_ctrl *sync);
void csic_dma_int_clear_status(unsigned int sel, enum dma_int_sel interrupt);
void csic_dma_int_clear_all_status(unsigned int sel, enum dma_int_sel interrupt);
void csic_lbc_cmp_ratio(unsigned int sel, struct dma_lbc_cmp *lbc_cmp);

#endif /* __CSIC__DMA140__REG__H__ */
