/*
 * linux-4.9/drivers/media/platform/sunxi-vin/vin-isp/sun8iw12p1/sun8iw12p1_isp_reg.h
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
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

#ifndef __ISP500__H__
#define __ISP500__H__

/*FOR SUN8IW12P1 ISP*/

#define SUN8IW12P1_ISP_FE_CFG_REG_OFF                  0x000
#define SUN8IW12P1_ISP_FE_CTRL_REG_OFF                 0x004
#define SUN8IW12P1_ISP_FE_INT_EN_REG_OFF               0x008
#define SUN8IW12P1_ISP_FE_INT_STA_REG_OFF              0x00c
#define SUN8IW12P1_ISP_DBG_OUTPUT_REG_OFF              0x010
#define SUN8IW12P1_ISP_LINE_INT_NUM_REG_OFF            0x018
#define SUN8IW12P1_ISP_ROT_OF_CFG_REG_OFF              0x01c

#define SUN8IW12P1_ISP_REG_LOAD_ADDR_REG_OFF           0x020
#define SUN8IW12P1_ISP_REG_SAVED_ADDR_REG_OFF          0x024
#define SUN8IW12P1_ISP_LUT_LENS_GAMMA_ADDR_REG_OFF     0x028
#define SUN8IW12P1_ISP_DRC_ADDR_REG_OFF                0x02c
#define SUN8IW12P1_ISP_STATISTICS_ADDR_REG_OFF         0x030
#define SUN8IW12P1_ISP_VER_CFG_REG_OFF                 0x034
#define SUN8IW12P1_ISP_SRAM_RW_OFFSET_REG_OFF          0x038
#define SUN8IW12P1_ISP_SRAM_RW_DATA_REG_OFF            0x03c

#define SUN8IW12P1_ISP_EN_REG_OFF                      0x040
#define SUN8IW12P1_ISP_MODE_REG_OFF		       0x044
#define SUN8IW12P1_ISP_OB_SIZE_REG_OFF                 0x078
#define SUN8IW12P1_ISP_OB_VALID_REG_OFF                0x07c
#define SUN8IW12P1_ISP_OB_VALID_START_REG_OFF          0x080


typedef union {
	unsigned int dwval;
	struct {
		unsigned int isp_enable:1;
		unsigned int res0:7;
		unsigned int isp_ch0_en:1;
		unsigned int isp_ch1_en:1;
		unsigned int isp_ch2_en:1;
		unsigned int isp_ch3_en:1;
		unsigned int res1:4;
		unsigned int wdr_ch_seq:1;
		unsigned int wdr_exp_seq:1;
		unsigned int wdr_ver_read_en:1;
		unsigned int res2:13;
	} bits;
} SUN8IW12P1_ISP_FE_CFG_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int cap_en:1;
		unsigned int res0:1;
		unsigned int para_ready:1;
		unsigned int linear_update:1;
		unsigned int lens_update:1;
		unsigned int gamma_update:1;
		unsigned int drc_update:1;
		unsigned int disc_update:1;
		unsigned int satu_update:1;
		unsigned int wdr_update:1;
		unsigned int tdnf_update:1;
		unsigned int pltm_update:1;
		unsigned int cem_update:1;
		unsigned int contrast_update:1;
		unsigned int res1:18;
	} bits;
} SUN8IW12P1_ISP_FE_CTRL_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int finish_int_en:1;
		unsigned int start_int_en:1;
		unsigned int para_save_int_en:1;
		unsigned int para_load_int_en:1;
		unsigned int src0_fifo_int_en:1;
		unsigned int res0:2;
		unsigned int n_line_start_int_en:1;
		unsigned int frame_error_int_en:1;
		unsigned int res1:5;
		unsigned int frame_lost_int_en:1;
		unsigned int res2:17;
	} bits;
} SUN8IW12P1_ISP_FE_INT_EN_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int finish_pd:1;
		unsigned int start_pd:1;
		unsigned int para_saved_pd:1;
		unsigned int para_load_pd:1;
		unsigned int src0_fifo_of_pd:1;
		unsigned int res0:2;
		unsigned int n_line_start_pd:1;
		unsigned int cin_fifo_pd:1;
		unsigned int dpc_fifo_pd:1;
		unsigned int d2d_fifo_pd:1;
		unsigned int bis_fifo_pd:1;
		unsigned int cnr_fifo_pd:1;
		unsigned int frame_lost_pd:1;
		unsigned int res1:7;
		unsigned int d3d_w_finish_pd:1;
		unsigned int wdr_w_finish_pd:1;
		unsigned int d3d_hb_pd:1;
		unsigned int pltm_fifo_pd:1;
		unsigned int d3d_write_fifo_pd:1;
		unsigned int d3d_read_fifo_pd:1;
		unsigned int d3d_wt2cmp_fifo_pd:1;
		unsigned int wdr_write_fifo_pd:1;
		unsigned int wdr_wt2cmp_fifo_pd:1;
		unsigned int wdr_read_fifo_pd:1;
	} bits;
} SUN8IW12P1_ISP_FE_INT_STA_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int top_ctrl_st:8;
		unsigned int wdr_ctrl_st:3;
		unsigned int res0:4;
		unsigned int debug_sel:5;
		unsigned int debug_en:1;
		unsigned int res1:11;
	} bits;
} SUN8IW12P1_ISP_DBG_OUTPUT_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int line_int_num:14;
		unsigned int res0:13;
		unsigned int last_blank_cycle:3;
		unsigned int res1:2;
	} bits;
} SUN8IW12P1_ISP_LINE_INT_NUM_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int res0:26;
		unsigned int speed_mode:3;
		unsigned int res1:3;
	} bits;
} SUN8IW12P1_ISP_ROT_OF_CFG_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int reg_load_addr;
	} bits;
} SUN8IW12P1_ISP_REG_LOAD_ADDR_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int reg_saved_addr;
	} bits;
} SUN8IW12P1_ISP_REG_SAVED_ADDR_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lut_lens_gamma_addr;
	} bits;
} SUN8IW12P1_ISP_LUT_LENS_GAMMA_ADDR_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int rgb_yuv_drc_addr;
	} bits;
} SUN8IW12P1_ISP_DRC_ADDR_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int statistics_addr;
	} bits;
} SUN8IW12P1_ISP_STATISTICS_ADDR_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int minor_ver:12;
		unsigned int major_ver:12;
		unsigned int res0:8;
	} bits;
} SUN8IW12P1_ISP_VER_CFG_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int sram_addr:17;
		unsigned int res0:14;
		unsigned int sram_clear:1;
	} bits;
} SUN8IW12P1_ISP_SRAM_RW_OFFSET_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int sram_data;
	} bits;
} SUN8IW12P1_ISP_SRAM_RW_DATA_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int ae_en:1;
		unsigned int lc_en:1;
		unsigned int wdr_en:1;
		unsigned int otf_dpc_en:1;
		unsigned int bdnf_en:1;
		unsigned int tdnf_en:1;
		unsigned int awb_en:1;
		unsigned int wb_en:1;
		unsigned int lsc_en:1;
		unsigned int bgc_en:1;
		unsigned int sap_en:1;
		unsigned int af_en:1;
		unsigned int rgb2rgb_en:1;
		unsigned int rgb_drc_en:1;
		unsigned int pltm_en:1;
		unsigned int cem_en:1;
		unsigned int afs_en:1;
		unsigned int hist_en:1;
		unsigned int blc_en:1;
		unsigned int dg_en:1;
		unsigned int so_en:1;
		unsigned int ctc_en:1;
		unsigned int contrast_en:1;
		unsigned int cnr_en:1;
		unsigned int saturation_en:1;
		unsigned int res2:6;
		unsigned int src0_en:1;
	} bits;
} SUN8IW12P1_ISP_EN_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int input_fmt:3;
		unsigned int res0:5;
		unsigned int wdr_mode:1;
		unsigned int wdr_dol_mode:1;
		unsigned int res1:1;
		unsigned int wdr_cmp_mode:1;
		unsigned int res2:4;
		unsigned int otf_dpc_mode:2;
		unsigned int res3:1;
		unsigned int saturation_mode:1;
		unsigned int hist_mode:2;
		unsigned int hist_sel:1;
		unsigned int caf_mode:1;
		unsigned int ae_mode:2;
		unsigned int awb_mode:1;
		unsigned int dg_mode:1;
		unsigned int res4:4;
	} bits;
} SUN8IW12P1_ISP_MODE_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int ob_width:14;
		unsigned int res0:2;
		unsigned int ob_height:14;
		unsigned int res1:2;
	} bits;
} SUN8IW12P1_ISP_OB_SIZE_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int ob_valid_width:13;
		unsigned int res0:3;
		unsigned int ob_valid_height:13;
		unsigned int res1:3;
	} bits;
} SUN8IW12P1_ISP_OB_VALID_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int ob_hor_start:13;
		unsigned int res0:3;
		unsigned int ob_ver_start:13;
		unsigned int res1:3;
	} bits;
} SUN8IW12P1_ISP_OB_VALID_START_REG_t;

#endif
