/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * dec_reg.h
 *
 * Copyright (c) 2007-2020 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
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
#ifndef _DEC_REG_H
#define _DEC_REG_H

#ifdef __cplusplus
extern "C" {
#endif

#define FPGA_DEBUG_ENABLE 0
#define AFBD_OFFLINE_MODE 0

union afbd_ctrl {
	unsigned int dwval;
	struct {
		unsigned int reset_cycle_num:16;
		unsigned int decode_ctl:1;
		unsigned int read_reg_sel:1;
		unsigned int res:6;
		unsigned int clk_bk_door:1;
		unsigned int res1:6;
		unsigned int en:1;
	} bits;
};

union afbd_dec_ctrl {
	unsigned int dwval;
	struct {
		unsigned int en:1;
		unsigned int res0:3;
		unsigned int int_to_display:1;
		unsigned int res1:27;
	} bits;
};

union afbd_dec_fild_mode {
	unsigned int dwval;
	struct {
		unsigned int field_mode:1;
		unsigned int res0:3;
		unsigned int filed_rpt_mode_out:1;
		unsigned int res1:3;
		unsigned int blue_en_out:1;
		unsigned int res2:23;
	} bits;
};

union afbd_dec_ybuf_sel {
	unsigned int dwval;
	struct {
		unsigned int addr_cur_y_out_mux:3;
		unsigned int res0:1;
		unsigned int addr_nxt_y_out_mux:3;
		unsigned int res1:1;
		unsigned int addr_pre_y_out_mux:3;
		unsigned int res2:1;
		unsigned int addr_2pre_y_out_mux:3;
		unsigned int res3:1;
		unsigned int addr_4pre_y_out_mux:3;
		unsigned int res4:1;
		unsigned int addr_6pre_y_out_mux:3;
		unsigned int res5:1;
		unsigned int uncom_addr_ctrl:1;
		unsigned int res6:7;
	} bits;
};

union afbd_dec_cbuf_sel {
	unsigned int dwval;
	struct {
		unsigned int addr_cur_c_out_mux:3;
		unsigned int res0:9;
		unsigned int addr_2pre_c_out_mux:3;
		unsigned int res1:1;
		unsigned int addr_4pre_c_out_mux:3;
		unsigned int res3:1;
		unsigned int addr_6pre_c_out_mux:3;
		unsigned int res4:9;
	} bits;
};

union afbd_dec_high_addr {
	unsigned int dwval;
	struct {
		unsigned int addr0:4;
		unsigned int addr1:4;
		unsigned int addr2:4;
		unsigned int addr3:4;
		unsigned int addr4:4;
		unsigned int addr5:4;
		unsigned int addr6:4;
		unsigned int addr7:4;
	} bits;
};

union afbd_dec_status {
	unsigned int dwval;
	struct {
		unsigned int vs_flag:1;
		unsigned int res0:31;
	} bits;
};

union afbd_wb_en {
	unsigned int dwval;
	struct {
		unsigned int vs_flag:1;
		unsigned int res0:31;
	} bits;
};

union afbd_dec_int_ctrl {
	unsigned int dwval;
	struct {
		unsigned int vs_int_en:1;
		unsigned int res0:31;
	} bits;
};

union afbd_clk_ctrl {
	unsigned int dwval;
	struct {
		unsigned int afbd_ahb_clk_gate:1;
		unsigned int res0:3;
		unsigned int afbd_mbus_clk_gate:1;
		unsigned int res1:3;
		unsigned int afbd_func_clk_gate:1;
		unsigned int res2:3;
		unsigned int afbd_pixel_clk_gate:1;
		unsigned int res3:3;
		unsigned int afbd_pixel_clk_sel:1;
		unsigned int res4:15;
	} bits;
};

union afbd_rst_ctrl {
	unsigned int dwval;
	struct {
		unsigned int afbd_rst:1;
		unsigned int res0:31;
	} bits;
};

union svp_clk_gate {
	unsigned int dwval;
	struct {
		unsigned int svp_ahb_clk_gate:1;
		unsigned int res0:3;
		unsigned int svp_axi_clk_gate:1;
		unsigned int res1:3;
		unsigned int svp_pixel_clk_gate:1;
		unsigned int res2:3;
		unsigned int svp_dtl_clk_gate:1;
		unsigned int res3:19;
	} bits;
};

union svp_rst_ctrl {
	unsigned int dwval;
	struct {
		unsigned int svp_ahb_rst:1;
		unsigned int res0:3;
		unsigned int svp_axi_rst:1;
		unsigned int res1:3;
		unsigned int svp_deint_rst:1;
		unsigned int res2:3;
		unsigned int svp_meter_rst:1;
		unsigned int res3:3;
		unsigned int svp_nr_rst:1;
		unsigned int res4:15;
	} bits;
};

union tvdisp_out_sel {
	unsigned int dwval;
	struct {
		unsigned int svp_ahb_rst:3;
		unsigned int res0:29;
	} bits;
};

union panel_ext_clk_gate {
	unsigned int dwval;
	struct {
		unsigned int panel_axb_gate:1;
		unsigned int res0:3;
		unsigned int panel_axi_gate:1;
		unsigned int res1:3;
		unsigned int panel_pixel_gate:1;
		unsigned int res2:3;
		unsigned int panel_dtl_gate:1;
		unsigned int res3:19;
	} bits;
};

union panel_int_clk_gate {
	unsigned int dwval;
	struct {
		unsigned int clk_src_lvds_clk5:1;
		unsigned int res0:3;
		unsigned int clk_src_lvds_clk6:2;
		unsigned int res1:2;
		unsigned int clk_src_lvds_clk7:2;
		unsigned int res2:2;
		unsigned int clk_src_lvds_clk8:2;
		unsigned int res3:2;
		unsigned int clk_src_lvds_clk9:2;
		unsigned int res4:2;
		unsigned int pllin_gate_en:1;
		unsigned int lvds_tx_gate_en:1;
		unsigned int lvds_tx1_gate_en:1;
		unsigned int lvds_tx2_gate_en:1;
		unsigned int res5:8;
	} bits;
};

union panel_reset_ctrl{
	unsigned int dwval;
	struct {
		unsigned int dtl_lvds_rst:1;
		unsigned int res0:3;
		unsigned int dtl_lvds_tx_rst:1;
		unsigned int res1:3;
		unsigned int lvds_buf_rst:1;
		unsigned int res2:3;
		unsigned int lvds_phy_rst:1;
		unsigned int res3:3;
		unsigned int lvds_tx1_rst:1;
		unsigned int res4:3;
		unsigned int lvds_wm_rst:1;
		unsigned int res5:3;
		unsigned int lvds_ahb_rst:1;
		unsigned int res6:3;
		unsigned int lvds_axi_rst:1;
		unsigned int res7:3;
	} bits;
};

struct tvdisp_top_reg {
	/*0x00 ~ 0x0c*/
	union afbd_clk_ctrl afbd_clk;
	union afbd_rst_ctrl afbd_rst;
	union svp_clk_gate svp_clk;
	union svp_rst_ctrl svp_rst;
	/*0x10 ~ 0x1c*/
	union tvdisp_out_sel out_sel;
	union panel_ext_clk_gate panel_ext_gate;
	union panel_int_clk_gate panel_int_gate;
	union panel_reset_ctrl panel_rst;

};

union afbd_channel_attr {
	unsigned int dwval;
	struct {
		unsigned int y_en:1;
		unsigned int c_en:1;
		unsigned int flip:2;
		unsigned int source:1;
		unsigned int res0:3;
		unsigned int format:8;
		unsigned int sb_layout:3;
		unsigned int res1:5;
		unsigned int components:3;
		unsigned int res2:4;
		unsigned int is_compressed:1;
	} bits;
};

union afbd_channel_crop {
	unsigned int dwval;
	struct {
		unsigned int width :16;
		unsigned int height:16;
	} bits;
};

union afbd_source_crop {
	unsigned int dwval;
	struct {
		unsigned int left_crop :5;
		unsigned int res0 :11;
		unsigned int top_crop:12;
		unsigned int res1:4;
	} bits;
};

union afbd_int_ctrl {
	unsigned int dwval;
	struct {
		unsigned int chl0_finish_int_en:1;
		unsigned int res0:3;
		unsigned int chl0_error_en:1;
		unsigned int res1:27;
	} bits;
};

union afbd_status_reg {
	unsigned int dwval;
	struct {
		unsigned int ch0_finish:1;
		unsigned int res0:3;
		unsigned int ch0_overflow_flag:1;
		unsigned int ch0_time_out:1;
		unsigned int ch0_size_error:1;
		unsigned int ch0_bctree_err:1;
		unsigned int ch0_mintree_err:1;
		unsigned int res1:23;
	} bits;
};

struct afbd_top_reg {
	union afbd_ctrl ctrl;	// 0x00
	unsigned int res0[3];	// 0x04 0x08 0x0c

	/* 0x10 ~ 0x1c */
	union afbd_channel_attr attr;
	unsigned int reg_ready;
	unsigned int res1[2];

	/* 0x20 ~ 0x2c */
	union afbd_channel_crop size;
	union afbd_channel_crop block_size;
	union afbd_source_crop source_crop;
	union afbd_channel_crop out_crop_pos;

	/* 0x30 ~ 0x3c */
	union afbd_channel_crop out_crop;
	unsigned int outst;
	union afbd_status_reg status;
	union afbd_int_ctrl int_ctrl;

	/* 0x40 ~ 0x4c */
	unsigned int y_stride;
	unsigned int c_stride;
	union afbd_channel_crop y_crop_size;
	union afbd_channel_crop c_crop_size;
};

struct afbd_dec_reg {
	/*0x60 ~ 0x6c*/
	union afbd_dec_ctrl ctrl;
	union afbd_dec_fild_mode fild_mode;
	union afbd_dec_ybuf_sel ybuf_sel;
	union afbd_dec_cbuf_sel cbuf_sel;

	/*0x70 ~ 0x7c*/
	unsigned int afbd_dec_reg_rdy;
	unsigned int y_addr0;
	unsigned int y_addr1;
	unsigned int y_addr2;

	/*0x80 ~ 0x8c*/
	unsigned int y_addr3;
	unsigned int y_addr4;
	unsigned int y_addr5;
	unsigned int y_addr6;

	/*0x90 ~ 0x9c*/
	unsigned int y_addr7;
	union afbd_dec_high_addr y_high_addr;
	unsigned int c_addr0;
	unsigned int c_addr1;

	/*0xa0 ~ 0xac*/
	unsigned int c_addr2;
	unsigned int c_addr3;
	unsigned int c_addr4;
	unsigned int c_addr5;

	/*0xb0 ~ 0xbc*/
	unsigned int c_addr6;
	unsigned int c_addr7;
	union afbd_dec_high_addr c_high_addr;
	unsigned int video_info0_addr;

	/*0xc0 ~ 0xcc*/
	unsigned int video_info1_addr;
	unsigned int video_info2_addr;
	unsigned int video_info3_addr;
	unsigned int video_info4_addr;

	/*0xd0 ~ 0xdc*/
	unsigned int video_info5_addr;
	unsigned int video_info6_addr;
	unsigned int video_info7_addr;
	union afbd_dec_high_addr info_high_addr;

	/*0xe0 ~ 0xec*/
	unsigned int dec_vsync_delay;
	unsigned int vtotal;
	unsigned int vs_width;
	unsigned int clk_cnt;

	/*0xf0 ~ 0xfc*/
	unsigned int frame_cnt;
	union afbd_dec_status vs_flag;
	union afbd_dec_int_ctrl int_ctrl;
};

union wb_ch_en_reg {
	unsigned int dwval;
	struct {
		unsigned int ch0_wb_en:1;
		unsigned int res0:3;
		unsigned int ch1_wb_en:1;
		unsigned int res1:3;
		unsigned int ch2_wb_en:1;
		unsigned int res2:3;
		unsigned int ch3_wb_en:1;
		unsigned int res3:3;
		unsigned int ch4_wb_en:1;
		unsigned int res4:3;
		unsigned int ch5_wb_en:1;
		unsigned int res5:3;
		unsigned int ch6_wb_en:1;
		unsigned int res6:3;
		unsigned int ch7_wb_en:1;
		unsigned int res7:3;
	} bits;
};

union wb_ctrl_reg {
	unsigned int dwval;
	struct {
		unsigned int wb_start:1;
		unsigned int res0:3;
		unsigned int wb_mode:1;
		unsigned int res1:27;
	} bits;
};

struct afbd_wb_reg {
	union wb_ch_en_reg wb_ch_en;
	union wb_ctrl_reg wb_ctrl;
	unsigned int wb_ch012_oust;
	unsigned int wb_ch345_oust;

	unsigned int wb_ch67_oust;
	unsigned int wb_ch0to4_fifo_ctrl;
	unsigned int wb_ch5to7_fifo_ctrl;
	unsigned int wb_ch0_y_addr;

	unsigned int wb_ch0_c_addr;
	unsigned int wb_ch1_addr;
	unsigned int wb_ch2_addr;
	unsigned int wb_ch3_addr;

	unsigned int wb_ch4_addr;
	unsigned int wb_ch5_y_addr;
	unsigned int wb_ch5_c_addr;
	unsigned int wb_ch6_y_addr;

	unsigned int wb_ch6_c_addr;
	unsigned int wb_ch7_y_addr;
	unsigned int wb_ch7_c_addr;
	unsigned int wb_ch0to4_high_addr;

	unsigned int wb_ch567_high_addr;
	//unsigned int ch0_y_clk_cycle;

	//unsigned int ch0_c_clk_cycle_fs2fvd;
	//unsigned int ch1_clk_cycle_fs2fvd;
	//unsigned int ch2_clk_cycle_fs2fvd;
	//unsigned int ch3_clk_cycle_fs2fvd;

	//unsigned int ch4_clk_cycle_fs2fvd;
	//unsigned int ch0_block_pos;
	//unsigned int ch1_block_pos;
	//unsigned int ch2_block_pos;
	//unsigned int ch3_block_pos;
	//unsigned int ch4_block_pos;
};

#define DEC_REG_OFFSET 0x60
#define WB_REG_OFFSET 0x300

struct afbd_reg_t {
	struct tvdisp_top_reg *p_tvdisp_top;
	struct afbd_top_reg *p_top_reg;
	struct afbd_dec_reg *p_dec_reg;
	struct afbd_wb_reg *p_wb_reg;
	_Bool reg_dirty;
};

// Decoder display buffer select id, requires by svp !!!
#define ADDR_OUT_MUX_ID (2)

void dec_reg_top_enable(struct afbd_reg_t *p_reg, unsigned int en);
void dec_reg_enable(struct afbd_reg_t *p_reg, unsigned int en);
void dec_reg_int_to_display(struct afbd_reg_t *p_reg);
void dec_reg_set_address(struct afbd_reg_t *p_reg,
			  unsigned long long *frame_addr,
			  unsigned long long info_addr, unsigned int id);
void dec_reg_set_videoinfo(struct afbd_reg_t *p_reg,
		unsigned long long info_addr, unsigned int id);
void dec_reg_blue_en(struct afbd_reg_t *p_reg, unsigned int en);
void dec_reg_set_filed_mode(struct afbd_reg_t *p_reg, unsigned int filed_mode);
void dec_reg_set_filed_repeat(struct afbd_reg_t *p_reg, unsigned int enable);
unsigned int dec_irq_query(struct afbd_reg_t *p_reg);
void dec_reg_set_dirty(struct afbd_reg_t *p_reg, _Bool dirty);
_Bool dec_reg_is_dirty(struct afbd_reg_t *p_reg);
void dec_reg_mux_select(struct afbd_reg_t *p_reg, unsigned int mux);
unsigned int dec_reg_frame_cnt(struct afbd_reg_t *p_reg);
unsigned int dec_reg_get_y_address(struct afbd_reg_t *p_reg, unsigned int id);
unsigned int dec_reg_get_c_address(struct afbd_reg_t *p_reg, unsigned int id);

struct dec_buffer_attr {
	unsigned int is_compressed;
	unsigned int format;
	unsigned int width;
	unsigned int height;
	unsigned int sb_layout;
};

unsigned int dec_reg_video_channel_attr_config(struct afbd_reg_t *p_reg,
		struct dec_buffer_attr *inattr);

void dec_reg_bypass_config(struct afbd_reg_t *p_reg, int enable);

uint32_t dec_workaround_read(struct afbd_reg_t *p_reg, uint32_t offset);
uint32_t dec_workaround_read_atomic(struct afbd_reg_t *p_reg, uint32_t offset);

#ifdef __cplusplus
}
#endif

#endif /*End of file*/
