
/*
 ******************************************************************************
 *
 * sun8iw12p1_isp_reg.h
 *
 * Hawkview ISP - sun8iw12p1_isp_reg.h module
 *
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http:
 *
 * Version		  Author         Date		    Description
 *
 *   2.0		  Yang Feng   	2014/06/30	      Second Version
 *
 ******************************************************************************
 */

#ifndef __REG20__ISP__H__
#define __REG20__ISP__H__

#ifdef __cplusplus
extern "C" {

#endif

/*FOR SUN8IW12P1 ISP*/

#define SUN8IW12P1_ISP_FE_CFG_REG_OFF                  0x000
#define SUN8IW12P1_ISP_FE_CTRL_REG_OFF                 0x004
#define SUN8IW12P1_ISP_FE_INT_EN_REG_OFF               0x008
#define SUN8IW12P1_ISP_FE_INT_STA_REG_OFF              0x00c
#define SUN8IW12P1_ISP_DBG_OUTPUT_REG_OFF              0x018
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
		unsigned int res2:15;
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
		unsigned int res1:18;
		unsigned int vcap_read_start:1;
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
		unsigned int res1:9;
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
		unsigned int res0:15;
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
		unsigned int res0:15;
	} bits;
} SUN8IW12P1_ISP_SRAM_RW_OFFSET_REG_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int sram_data;
	} bits;
} SUN8IW12P1_ISP_SRAM_RW_DATA_REG_t;


#ifdef __cplusplus
}
#endif

#endif
