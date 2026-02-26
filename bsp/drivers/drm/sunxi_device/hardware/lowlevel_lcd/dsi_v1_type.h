/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DSI_V1_TYPE_H__
#define __DSI_V1_TYPE_H__

#include <linux/types.h>
#include "dsi_v1.h"

/*
 * Detail information of registers
 */
union dsi_ctl_reg_t {
	u32 dwval;
	struct {
		u32 dsi_en:1;
		u32 res0:31;
	} bits;
};

union dsi_gint0_reg_t {
	u32 dwval;
	struct {
		u32 dsi_irq_en:16;
		u32 dsi_irq_flag:16;
	} bits;
};

union dsi_gint1_reg_t {
	u32 dwval;
	struct {
		u32 video_line_int_num:13;
		u32 res0:19;
	} bits;
};

union dsi_basic_ctl_reg_t {
	u32 dwval;
	struct {
		u32 video_mode_burst:1;
		u32 hsa_hse_dis:1;
		u32 hbp_dis:1;
		u32 trail_fill:1;
		u32 trail_inv:4;
		u32 start_mode:1;
		u32 res0:7;
		u32 brdy_set:8;
		u32 brdy_l_sel:3;
		u32 res1:5;
	} bits;
};

union dsi_basic_ctl0_reg_t {
	u32 dwval;
	struct {
		u32 inst_st:1;
		u32 res0:3;
		u32 src_sel:2;
		u32 res1:4;
		u32 fifo_manual_reset:1;
		u32 res2:1;
		u32 fifo_gating:1;
		u32 res3:3;
		u32 ecc_en:1;
		u32 crc_en:1;
		u32 hs_eotp_en:1;
		u32 res4:13;
	} bits;
};

union dsi_basic_ctl1_reg_t {
	u32 dwval;
	struct {
		u32 dsi_mode:1;
		u32 video_frame_start:1;
		u32 video_precision_mode_align:1;
		u32 res0:1;
		u32 video_start_delay:8;
		u32 res1:4;
		u32 tri_delay:16;
	} bits;
};

union dsi_basic_size0_reg_t {
	u32 dwval;
	struct {
		u32 vsa:12;
		u32 res0:4;
		u32 vbp:12;
		u32 res1:4;
	} bits;
};

union dsi_basic_size1_reg_t {
	u32 dwval;
	struct {
		u32 vact:12;
		u32 res0:4;
		u32 vt:13;
		u32 res1:3;
	} bits;
};

union dsi_basic_inst0_reg_t {
	u32 dwval;
	struct {
		u32 lane_den:4;
		u32 lane_cen:1;
		u32 res0:11;
		u32 trans_start_condition:4;
		u32 trans_packet:4;
		u32 escape_enrty:4;
		u32 instru_mode:4;
	} bits;
};

union dsi_basic_inst1_reg_t {
	u32 dwval;
	struct {
		u32 inst0_sel:4;
		u32 inst1_sel:4;
		u32 inst2_sel:4;
		u32 inst3_sel:4;
		u32 inst4_sel:4;
		u32 inst5_sel:4;
		u32 inst6_sel:4;
		u32 inst7_sel:4;
	} bits;
};
union dsi_basic_inst11_reg_t {
	u32 dwval;
	struct {
		u32 inst0_sel:4;
		u32 inst1_sel:4;
		u32 inst2_sel:4;
		u32 inst3_sel:4;
		u32 inst4_sel:4;
		u32 inst5_sel:4;
		u32 inst6_sel:4;
		u32 inst7_sel:4;
	} bits;
};
union dsi_basic_inst2_reg_t {
	u32 dwval;
	struct {
		u32 loop_n0:12;
		u32 res0:4;
		u32 loop_n1:12;
		u32 res1:4;
	} bits;
};

union dsi_basic_inst3_reg_t {
	u32 dwval;
	struct {
		u32 inst0_jump:4;
		u32 inst1_jump:4;
		u32 inst2_jump:4;
		u32 inst3_jump:4;
		u32 inst4_jump:4;
		u32 inst5_jump:4;
		u32 inst6_jump:4;
		u32 inst7_jump:4;
	} bits;
};
union dsi_basic_inst13_reg_t {
	u32 dwval;
	struct {
		u32 inst0_jump:4;
		u32 inst1_jump:4;
		u32 inst2_jump:4;
		u32 inst3_jump:4;
		u32 inst4_jump:4;
		u32 inst5_jump:4;
		u32 inst6_jump:4;
		u32 inst7_jump:4;
	} bits;
};
union dsi_basic_inst4_reg_t {
	u32 dwval;
	struct {
		u32 jump_cfg_num:16;
		u32 jump_cfg_point:4;
		u32 jump_cfg_to:4;
		u32 res0:4;
		u32 jump_cfg_en:1;
		u32 res1:3;
	} bits;
};

union dsi_basic_tran0_reg_t {
	u32 dwval;
	struct {
		u32 trans_start_set:13;
		u32 res0:19;
	} bits;
};

union dsi_basic_tran1_reg_t {
	u32 dwval;
	struct {
		u32 trans_size:16;
		u32 res0:12;
		u32 trans_end_condition:1;
		u32 res1:3;
	} bits;
};

union dsi_basic_tran2_reg_t {
	u32 dwval;
	struct {
		u32 trans_cycle_set:16;
		u32 res0:16;
	} bits;
};

union dsi_basic_tran3_reg_t {
	u32 dwval;
	struct {
		u32 trans_blank_set:16;
		u32 res0:16;
	} bits;
};

union dsi_basic_tran4_reg_t {
	u32 dwval;
	struct {
		u32 hs_zero_reduce_set:16;
		u32 res0:16;
	} bits;
};

union dsi_basic_tran5_reg_t {
	u32 dwval;
	struct {
		u32 drq_set:10;
		u32 res0:18;
		u32 drq_mode:1;
		u32 res1:3;
	} bits;
};

union dsi_pixel_ctl0_reg_t {
	u32 dwval;
	struct {
		u32 pixel_format:4;
		u32 pixel_endian:1;
		u32 res0:11;
		u32 pd_plug_dis:1;
		u32 res1:15;
	} bits;
};

union dsi_pixel_ctl1_reg_t {
	u32 dwval;
	struct {
		u32 res0;
	} bits;
};

union dsi_pixel_ph_reg_t {
	u32 dwval;
	struct {
		u32 dt:6;
		u32 vc:2;
		u32 wc:16;
		u32 ecc:8;
	} bits;
};

union dsi_pixel_pd_reg_t {
	u32 dwval;
	struct {
		u32 pd_tran0:8;
		u32 res0:8;
		u32 pd_trann:8;
		u32 res1:8;
	} bits;
};

union dsi_pixel_pf0_reg_t {
	u32 dwval;
	struct {
		u32 crc_force:16;
		u32 res0:16;
	} bits;
};

union dsi_pixel_pf1_reg_t {
	u32 dwval;
	struct {
		u32 crc_init_line0:16;
		u32 crc_init_linen:16;
	} bits;
};

union dsi_short_pkg_reg_t {
	u32 dwval;
	struct {
		u32 dt:6;
		u32 vc:2;
		u32 d0:8;
		u32 d1:8;
		u32 ecc:8;
	} bits;
};

union dsi_blk_pkg0_reg_t {
	u32 dwval;
	struct {
		u32 dt:6;
		u32 vc:2;
		u32 wc:16;
		u32 ecc:8;
	} bits;
};

union dsi_blk_pkg1_reg_t {
	u32 dwval;
	struct {
		u32 pd:8;
		u32 res0:8;
		u32 pf:16;
	} bits;
};

union dsi_burst_line_reg_t {
	u32 dwval;
	struct {
		u32 line_num:16;
		u32 line_syncpoint:16;
	} bits;
};

union dsi_burst_drq_reg_t {
	u32 dwval;
	struct {
		u32 drq_edge0:16;
		u32 drq_edge1:16;
	} bits;
};

union dsi_debug_reg_t {
	u32 dwval;
	struct {
		u32 res1:31;
		u32 dsi_fifo_under_flow:1;
	} bits;
};

union dsi_cmd_ctl_reg_t {
	u32 dwval;
	struct {
		u32 tx_size:8;
		u32 tx_status:1;
		u32 tx_flag:1;
		u32 res0:6;
		u32 rx_size:5;
		u32 res1:3;
		u32 rx_status:1;
		u32 rx_flag:1;
		u32 rx_overflow:1;
		u32 res2:5;
	} bits;
};

union dsi_cmd_data_reg_t {
	u32 dwval;
	struct {
		u32 byte0:8;
		u32 byte1:8;
		u32 byte2:8;
		u32 byte3:8;
	} bits;
};

union dsi_debug0_reg_t {
	u32 dwval;
	struct {
		u32 video_curr_line:13;
		u32 res0:19;
	} bits;
};

union dsi_debug1_reg_t {
	u32 dwval;
	struct {
		u32 video_curr_lp2hs:16;
		u32 res0:16;
	} bits;
};

union dsi_debug2_reg_t {
	u32 dwval;
	struct {
		u32 trans_low_flag:1;
		u32 trans_fast_flag:1;
		u32 res0:2;
		u32 curr_loop_num:16;
		u32 curr_instru_num:3;
		u32 res1:1;
		u32 instru_unknown_flag:8;
	} bits;
};

union dsi_debug3_reg_t {
	u32 dwval;
	struct {
		u32 res0:16;
		u32 curr_fifo_num:16;
	} bits;
};

union dsi_debug4_reg_t {
	u32 dwval;
	struct {
		u32 test_data:24;
		u32 res0:4;
		u32 dsi_fifo_bist_en:1;
		u32 res1:3;
	} bits;
};

union dsi_reservd_reg_t {
	u32 dwval;
	struct {
		u32 res0;
	} bits;
};

struct dsi_lcd_reg {
	/* 0x00 - 0x0c */
	union dsi_ctl_reg_t dsi_gctl;
	union dsi_gint0_reg_t dsi_gint0;
	union dsi_gint1_reg_t dsi_gint1;
	union dsi_basic_ctl_reg_t dsi_basic_ctl;
	/* 0x10 - 0x1c */
	union dsi_basic_ctl0_reg_t dsi_basic_ctl0;
	union dsi_basic_ctl1_reg_t dsi_basic_ctl1;
	union dsi_basic_size0_reg_t dsi_basic_size0;
	union dsi_basic_size1_reg_t dsi_basic_size1;
	/* 0x20 - 0x3c */
	union dsi_basic_inst0_reg_t dsi_inst_func[8];
	/* 0x40 - 0x5c */
	union dsi_basic_inst1_reg_t dsi_inst_loop_sel;
	union dsi_basic_inst2_reg_t dsi_inst_loop_num;
	union dsi_basic_inst3_reg_t dsi_inst_jump_sel;
	union dsi_basic_inst4_reg_t dsi_inst_jump_cfg[2];
	union dsi_basic_inst2_reg_t dsi_inst_loop_num2;
	union dsi_reservd_reg_t dsi_reg058[2];
	/* 0x60 - 0x6c */
	union dsi_basic_tran0_reg_t dsi_trans_start;
	union dsi_reservd_reg_t dsi_reg064[3];
	/* 0x70 - 0x7c */
	union dsi_reservd_reg_t dsi_reg070[2];
	union dsi_basic_tran4_reg_t dsi_trans_zero;
	union dsi_basic_tran5_reg_t dsi_tcon_drq;
	/* 0x80 - 0x8c */
	union dsi_pixel_ctl0_reg_t dsi_pixel_ctl0;
	union dsi_pixel_ctl1_reg_t dsi_pixel_ctl1;
	union dsi_reservd_reg_t dsi_reg088[2];
	/* 0x90 - 0x9c */
	union dsi_pixel_ph_reg_t dsi_pixel_ph;
	union dsi_pixel_pd_reg_t dsi_pixel_pd;
	union dsi_pixel_pf0_reg_t dsi_pixel_pf0;
	union dsi_pixel_pf1_reg_t dsi_pixel_pf1;
	/* 0xa0 - 0xac */
	union dsi_reservd_reg_t dsi_reg0a0[4];
	/* 0xb0 - 0xbc */
	union dsi_short_pkg_reg_t dsi_sync_hss;
	union dsi_short_pkg_reg_t dsi_sync_hse;
	union dsi_short_pkg_reg_t dsi_sync_vss;
	union dsi_short_pkg_reg_t dsi_sync_vse;
	/* 0xc0 - 0xcc */
	union dsi_blk_pkg0_reg_t dsi_blk_hsa0;
	union dsi_blk_pkg1_reg_t dsi_blk_hsa1;
	union dsi_blk_pkg0_reg_t dsi_blk_hbp0;
	union dsi_blk_pkg1_reg_t dsi_blk_hbp1;
	/* 0xd0 - 0xdc */
	union dsi_blk_pkg0_reg_t dsi_blk_hfp0;
	union dsi_blk_pkg1_reg_t dsi_blk_hfp1;
	union dsi_reservd_reg_t dsi_reg0d8[2];
	/* 0xe0 - 0xec */
	union dsi_blk_pkg0_reg_t dsi_blk_hblk0;
	union dsi_blk_pkg1_reg_t dsi_blk_hblk1;
	union dsi_blk_pkg0_reg_t dsi_blk_vblk0;
	union dsi_blk_pkg1_reg_t dsi_blk_vblk1;
	/* 0xf0 - 0x1fc */
	union dsi_burst_line_reg_t dsi_burst_line;
	union dsi_burst_drq_reg_t dsi_burst_drq;
	union dsi_reservd_reg_t dsi_reg0f0_0;
	union dsi_debug_reg_t dsi_fifo_debug;
	union dsi_reservd_reg_t dsi_reg0f0[8];
	union dsi_basic_inst0_reg_t dsi_inst_func1[7];
	union dsi_reservd_reg_t dsi_reg0f01;
	union dsi_basic_inst11_reg_t dsi_inst_loop_sel1;
	union dsi_reservd_reg_t dsi_reg0f02;
	union dsi_basic_inst13_reg_t dsi_inst_jump_sel1;
	union dsi_reservd_reg_t dsi_reg0f03[45];
	/* 0x200 - 0x23c */
	union dsi_cmd_ctl_reg_t dsi_cmd_ctl;
	union dsi_reservd_reg_t dsi_reg204[15];
	/* 0x240 - 0x2dc */
	union dsi_cmd_data_reg_t dsi_cmd_rx[8];
	union dsi_reservd_reg_t dsi_reg260[32];
	/* 0x2e0 - 0x2ec */
	union dsi_debug0_reg_t dsi_debug_video0;
	union dsi_debug1_reg_t dsi_debug_video1;
	union dsi_reservd_reg_t dsi_reg2e8[2];
	/* 0x2f0 - 0x2fc */
	union dsi_debug2_reg_t dsi_debug_inst;
	union dsi_debug3_reg_t dsi_debug_fifo;
	union dsi_debug4_reg_t dsi_debug_data;
	union dsi_reservd_reg_t dsi_reg2fc;
	/* 0x300 - 0x3fc */
	union dsi_cmd_data_reg_t dsi_cmd_tx[64];
};

union dsi_ph_t {
	struct {
		u32 byte012:24;
		u32 byte3:8;
	} bytes;
	struct {
		u32 bit00:1;
		u32 bit01:1;
		u32 bit02:1;
		u32 bit03:1;
		u32 bit04:1;
		u32 bit05:1;
		u32 bit06:1;
		u32 bit07:1;
		u32 bit08:1;
		u32 bit09:1;
		u32 bit10:1;
		u32 bit11:1;
		u32 bit12:1;
		u32 bit13:1;
		u32 bit14:1;
		u32 bit15:1;
		u32 bit16:1;
		u32 bit17:1;
		u32 bit18:1;
		u32 bit19:1;
		u32 bit20:1;
		u32 bit21:1;
		u32 bit22:1;
		u32 bit23:1;
		u32 bit24:1;
		u32 bit25:1;
		u32 bit26:1;
		u32 bit27:1;
		u32 bit28:1;
		u32 bit29:1;
		u32 bit30:1;
		u32 bit31:1;
	} bits;
};

#endif
