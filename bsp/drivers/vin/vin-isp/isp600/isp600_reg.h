/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

/* SPDX-License-Identifier: GPL-2.0 */
 /*
  * isp600_reg.h
  *
  * Copyright (c) 2007-2021 Allwinnertech Co., Ltd.
  *
  * Authors:  Zheng ZeQun <zequnzheng@allwinnertech.com>
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

#ifndef _ISP600_REG_H_
#define _ISP600_REG_H_

#define ISP_TOP_REG_OFFSET			0x0000
#define ISP_AHB_REG_OFFSET			0x0020
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
#define ISP_AHB_AMONG_OFFSET		0x0020
#else
#define ISP_AHB_AMONG_OFFSET		0x0030
#endif
#define ISP_LOAD_REG_OFFSET			0x0100
#define ISP_SAVE_REG_OFFSET			0x1000
#define ISP_SAVE_LOAD_REG_OFFSET	0x1200

/*top reg*/
#define ISP_TOP_CFG0_REG			0x000
#define ISP_VER_CFG_REG				0x010
#define ISP_MAX_WIDTH_REG			0x014
#define ISP_MODULE_FET_REG			0x018

/*ahb reg*/
#define ISP_AHB_CFG0_REG			0x000
#define ISP_LOAD_ADDR_REG			0x004
#define ISP_INT_BYPASS_REG			0x008
#define ISP_INT_STATUS_REG			0x00c
#define ISP_INTER_STATUS0_REG		0x010
#define ISP_INTER_STATUS1_REG		0x014
#define ISP_INTER_STATUS2_REG		0x018
#define ISP_SAVE_ADDR_REG			0x01c
#if !defined CONFIG_ARCH_SUN55IW3
#define ISP_SAVE_LOAD_ADDR_REG		0x020
#endif

/*load reg*/
#define ISP_GLOBAL_CFG0_REG			0x000
#define ISP_GLOBAL_CFG1_REG			0x004
#define ISP_LBC_TIME_CYCLE_REG		0x008
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
#define ISP_SAVE_LOAD_ADDR_REG		0x014
#endif
#define ISP_INPUT_SIZE_REG			0x020
#define ISP_VALID_SIZE_REG			0x024
#define ISP_VALID_START_REG			0x028
#define ISP_MODULE_BYPASS0_REG		0x030
#define ISP_MODULE_BYPASS1_REG		0x034
#define ISP_MODULE_MODE0_REG		0x038
#define ISP_MODULE_MODE1_REG		0x03c
#define ISP_D3D_BAYER_ADDR_REG		0x040
#define ISP_D3D_K0_ADDR_REG			0x044
#define ISP_D3D_K1_ADDR_REG			0x048
#define ISP_D3D_STATUS_ADDR_REG		0x04c
#define ISP_VIN_CFG0_REG			0x070
#define ISP_CH0_EXPAND_CFG0_REG		0x098
#define ISP_CH1_EXPAND_CFG0_REG		0x0b8
#define ISP_CH2_EXPAND_CFG0_REG		0x0d8
#define ISP_D3D_CFG0_REG			0x220
#define ISP_D3D_LBC_CFG0_REG		0x280
#define ISP_D3D_LBC_CFG1_REG		0x284
#define ISP_D3D_LBC_CFG2_REG		0x288
#define ISP_D3D_LBC_CFG3_REG		0x28c
#define ISP_D3D_LBC_CFG4_REG		0x290
#define ISP_D3D_LBC_CFG5_REG		0x294

/*save load reg*/
#define ISP_RANDOM_SEED_REG			0x030

typedef union {
	unsigned int dwval;
	struct {
		unsigned int isp_enable:1;
		unsigned int isp_mode:1;
		unsigned int isp_top_cap_en:1;
		unsigned int isp_ver_rd_en:1;
		unsigned int sram_clear:1;
		unsigned int res0:3;
		unsigned int int_mode:1;
		unsigned int md_clk_back_door:1;
		unsigned int d2d_clk_back_door:1;
		unsigned int auto_gating_mode:1;
		unsigned int auto_gating_back_door:1;
		unsigned int res1:19;
	} bits;
} ISP_TOP_CFG0_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int small_ver:12;
		unsigned int big_ver:12;
		unsigned int res0:8;
	} bits;
} ISP_VER_CFG_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int max_width:16;
		unsigned int max_height:16;
	} bits;
} ISP_MAX_SIZE_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int d3d_exist:1;
		unsigned int af_flt1_ext:1;
		unsigned int rgb_bit_mode:1;
		unsigned int res0:1;
		unsigned int wdr_feature:2;
		unsigned int res1:2;
		unsigned int d2d_feature:2;
		unsigned int res2:2;
		unsigned int pltm_feature:3;
		unsigned int res3:17;
	} bits;
} ISP_MODULE_FET_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int cap_en:1;
		unsigned int para_ready:1;
		unsigned int res0:6;
		unsigned int rsc_update:1;
		unsigned int gamma_update:1;
		unsigned int drc_update:1;
		unsigned int res1:2;
		unsigned int d3d_update:1;
		unsigned int pltm_update:1;
		unsigned int cem_update:1;
		unsigned int msc_update:1;
		unsigned int fe0_msc_update:1;
		unsigned int nr_msc_update:1;
		unsigned int fe1_msc_update:1;
		unsigned int fe2_msc_update:1;
		unsigned int res2:10;
		unsigned int dma_error_mode:1;
	} bits;
}  ISP_AHB_CFG0_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int finish_int_en:1;
		unsigned int start_int_en:1;
		unsigned int para_save_int_en:1;
		unsigned int para_load_int_en:1;
		unsigned int n_line_start_int_en:1;
		unsigned int res0:4;
		unsigned int hb_short_int_en:1;
		unsigned int cfg_error_int_en:1;
		unsigned int frame_lost_int_en:1;
		unsigned int res1:4;
		unsigned int inter_fifo_full_int_en:1;
		unsigned int wdma_fifo_full_int_en:1;
		unsigned int wdma_over_end_full_int_en:1;
		unsigned int rdma_fifo_full_int_en:1;
		unsigned int lbc_error_int_en:1;
		unsigned int ddr_rw_error_ini_en:1;
		unsigned int ahb_mbus_w_int_en:1;
		unsigned int res2:9;
	} bits;
} ISP_INT_BYPASS_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int finish_int_pd:1;
		unsigned int start_int_pd:1;
		unsigned int para_save_int_pd:1;
		unsigned int para_load_int_pd:1;
		unsigned int n_line_start_int_pd:1;
		unsigned int res0:4;
		unsigned int hb_short_int_pd:1;
		unsigned int cfg_error_int_pd:1;
		unsigned int frame_lost_int_pd:1;
		unsigned int res1:4;
		unsigned int inter_fifo_full_int_pd:1;
		unsigned int wdma_fifo_full_int_pd:1;
		unsigned int wdma_over_end_full_int_pd:1;
		unsigned int rdma_fifo_full_int_pd:1;
		unsigned int lbc_error_int_pd:1;
		unsigned int res2:1;
		unsigned int ahb_mbus_w_int_pd:1;
		unsigned int res3:9;
	} bits;
} ISP_INT_STATUS_REG_t;

typedef union {
	unsigned int dwval;
} ISP_INT_STATUS0_REG_t;

typedef union {
	unsigned int dwval;
} ISP_INT_STATUS1_REG_t;

typedef union {
	unsigned int dwval;
} ISP_INT_STATUS2_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int input_fmt:3;
		unsigned int res0:1;
		unsigned int byr_act_bit:5;
		unsigned int res1:3;
		unsigned int byr_max_bit:3;
		unsigned int res2:1;
		unsigned int rgb_cfg_bit:4;
		unsigned int res3:10;
		unsigned int save_reg_dbg_en:1;
		unsigned int dbg_out_en:1;
	} bits;
} ISP_GLOBAL_CFG0_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int speed_mode:3;
		unsigned int res0:1;
		unsigned int last_blank_cycle:3;
		unsigned int res1:1;
		unsigned int dbg_out_sel:5;
		unsigned int res2:1;
		unsigned int dbg_out_mode:2;
		unsigned int line_int_num:13;
		unsigned int dbg_out_shft_bit:3;
	} bits;
} ISP_GLOBAL_CFG1_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lbc_tx_t1_cycle:16;
		unsigned int lbc_tx_t2_cycle:16;
	} bits;
} ISP_LBC_TIME_CYCLE_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int input_width:14;
		unsigned int res0:2;
		unsigned int input_height:14;
		unsigned int res1:2;
	} bits;
} ISP_INPUT_SIZE_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int valid_width:14;
		unsigned int res0:2;
		unsigned int valid_height:14;
		unsigned int res1:2;
	} bits;
} ISP_VALID_SIZE_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int valid_hor_start:13;
		unsigned int res0:3;
		unsigned int valid_ver_start:13;
		unsigned int res1:3;
	} bits;
} ISP_VALID_START_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int wdr_en:1;
		unsigned int wdr_split_en:1;
		unsigned int res0:6;
		unsigned int ch0_dg_en:1;
		unsigned int ch1_dg_en:1;
		unsigned int ch2_dg_en:1;
		unsigned int res1:1;
		unsigned int ch0_msc_en:1;
		unsigned int ch1_msc_en:1;
		unsigned int ch2_msc_en:1;
		unsigned int res2:17;
	} bits;
} ISP_MODULE_BYPASS0_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int wdr_stitch_en:1;
		unsigned int dpc_en:1;
		unsigned int ctc_en:1;
		unsigned int gca_en:1;
		unsigned int d2d_en:1;
		unsigned int d3d_en:1;
		unsigned int blc_en:1;
		unsigned int wb_en:1;
		unsigned int dg_en:1;
		unsigned int rsc_en:1;
		unsigned int msc_en:1;
		unsigned int pltm_en:1;
		unsigned int lca_en:1;
		unsigned int sharp_en:1;
		unsigned int ccm_en:1;
		unsigned int cnr_en:1;
		unsigned int drc_en:1;//rgb_drc_en
		unsigned int gamma_en:1;//bgc_en
		unsigned int cem_en:1;
		unsigned int res0:5;
		unsigned int ae_en:1;
		unsigned int af_en:1;
		unsigned int awb_en:1;
		unsigned int afs_en:1;
		unsigned int hist0_en:1;
		unsigned int hist1_en:1;
		unsigned int res1:2;
	} bits;
} ISP_MODULE_BYPASS1_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int res0:20;
		unsigned int d3d_ref_frm_mode:1;
		unsigned int res1:11;
	} bits;
} ISP_MODULE_MODE0_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int input_cfg:4;
		unsigned int res0:4;
		unsigned int output_cfg:4;
		unsigned int res1:4;
		unsigned int output_chn0_data:3;
		unsigned int res2:1;
		unsigned int output_chn1_data:3;
		unsigned int res3:1;
		unsigned int output_chn2_data:3;
		unsigned int res4:5;
	} bits;
} ISP_VIN_CFG0_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int blc_en:1;
		unsigned int inv_blc_en:1;
		unsigned int res0:2;
		unsigned int input_bit:5;
		unsigned int res1:7;
		unsigned int output_bit:5;
		unsigned int res2:11;
	} bits;
} ISP_CH0_EXPAND_CFG0_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int blc_en:1;
		unsigned int inv_blc_en:1;
		unsigned int res0:2;
		unsigned int input_bit:5;
		unsigned int res1:7;
		unsigned int output_bit:5;
		unsigned int res2:11;
	} bits;
} ISP_CH1_EXPAND_CFG0_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int blc_en:1;
		unsigned int inv_blc_en:1;
		unsigned int res0:2;
		unsigned int input_bit:5;
		unsigned int res1:7;
		unsigned int output_bit:5;
		unsigned int res2:11;
	} bits;
} ISP_CH2_EXPAND_CFG0_REG_t;


typedef union {
	unsigned int dwval;
	struct {
		unsigned int sharp_rnd_seed:12;
		unsigned int res0:4;
		unsigned int d3d_rnd_seed:8;
		unsigned int res1:4;
		unsigned int input_fmt:3;
		unsigned int d3d_k_state:1;
	} bits;
} ISP_RANDOM_SEED_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int d3d_rec_en:1;
		unsigned int res0:31;
	} bits;
} ISP_D3D_CFG0_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int res0:24;
		unsigned int sram_bmode_ctrl:1;
		unsigned int res1:7;
	} bits;
} ISP_SRAM_CTRL_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int mb_min_bit:9;
		unsigned int res0:3;
		unsigned int gbgr_ratio:10;
		unsigned int res1:2;
		unsigned int mb_num:8;
	} bits;
} ISP_D3D_LBC_CFG0_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int mb_min_bits_std3:4;
		unsigned int mb_min_bits_std2:4;
		unsigned int mb_min_bits_std1:4;
		unsigned int mb_min_bits_std0:4;
		unsigned int start_qp_std3:4;
		unsigned int start_qp_std2:4;
		unsigned int start_qp_std1:4;
		unsigned int start_qp_std0:4;
	} bits;
} ISP_D3D_LBC_CFG1_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int start_qp:4;
		unsigned int start_qp_en:1;
		unsigned int res0:3;
		unsigned int start_qp_thr2:8;
		unsigned int start_qp_thr1:8;
		unsigned int start_qp_thr0:8;
	} bits;
} ISP_D3D_LBC_CFG2_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int is_lossy:1;
		unsigned int log_en:1;
		unsigned int max_sad_ratio:2;
		unsigned int res0:4;
		unsigned int mb_sad_thr2:8;
		unsigned int mb_sad_thr1:8;
		unsigned int mb_sad_thr0:8;
	} bits;
} ISP_D3D_LBC_CFG3_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int line_tar_bit:16;
		unsigned int line_max_bit:16;
	} bits;
} ISP_D3D_LBC_CFG4_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int line_stride:16;
		unsigned int res0:16;
	} bits;
} ISP_D3D_LBC_CFG5_REG_t;

#endif /*_1_REG_H_*/
