// $Module: ive_filterop $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Mon, 13 Dec 2021 01:21:08 PM $
//

#ifndef __REG_IVE_FILTEROP_STRUCT_H__
#define __REG_IVE_FILTEROP_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*soft reset for pipe engine;*/
		uint32_t reg_softrst:1;
		/*;*/
		uint32_t reg_softrst_wdma_y:1;
		/*;*/
		uint32_t reg_softrst_wdma_c:1;
		/*;*/
		uint32_t reg_softrst_rdma_gradfg:1;
		/*;*/
		uint32_t reg_softrst_op1:1;
		/*;*/
		uint32_t reg_softrst_filter3ch:1;
		/*;*/
		uint32_t reg_softrst_st:1;
		/*;*/
		uint32_t reg_softrst_odma:1;
	};
	uint32_t val;
} ive_filterop_reg_1_c;
typedef union {
	struct {
		/*set 1 as gradfg active to enable rdma  ;*/
		uint32_t reg_gradfg_bggrad_rdma_en:1;
		/*;*/
		uint32_t reg_gradfg_bggrad_uv_swap:1;
	};
	uint32_t val;
} ive_filterop_reg_h04_c;
typedef union {
	struct {
		/*reg reg sel;*/
		uint32_t reg_shdw_sel:1;
	};
	uint32_t val;
} ive_filterop_reg_2_c;
typedef union {
	struct {
		/*dmy;*/
		uint32_t reg_ctrl_dmy1:32;
	};
	uint32_t val;
} ive_filterop_reg_3_c;
typedef union {
	struct {
		/*
		 MOD_BYP       = 0 ;
		 MOD_FILTER3CH = 1 ;
		 MOD_DILA      = 2 ;
		 MOD_ERO       = 3 ;
		 MOD_CANNY     = 4 ;
		 MOD_STBOX     = 5 ;
		 MOD_GRADFG    = 6 ;
		 MOD_MAG       = 7 ;
		 MOD_NORMG     = 8 ;
		 MOD_SOBEL     = 9 ;
		 MOD_STCANDI   = 10 ;
		 MOD_MAP       = 11 ;;*/
		uint32_t reg_filterop_mode:4;
	};
	uint32_t val;
} ive_filterop_reg_h10_c;
typedef union {
	struct {
		/*Only use in MOD_DILA,MOD_ERO :
		can setting:
		 OP1_FILTER  = 1 ;
		 OP1_DILA    = 2 ;
		 OP1_ERO     = 3 ;
		 OP1_ORDERF  = 4 ;
		 OP1_BERN    = 5 ;
		 OP1_LBP     = 6 ;;*/
		uint32_t reg_filterop_op1_cmd:4;
		/*over write op1 cmd:
		 OP1_BYP     = 0 ;
		 OP1_FILTER  = 1 ;
		 OP1_DILA    = 2 ;
		 OP1_ERO     = 3 ;
		 OP1_ORDERF  = 4 ;
		 OP1_BERN    = 5 ;
		 OP1_LBP     = 6 ;
		 OP1_NORMG   = 7 ;
		 OP1_MAG     = 8 ;
		 OP1_SOBEL   = 9 ;
		 OP1_STCANDI = 10 ;
		 OP1_MAP     = 11 ;;*/
		uint32_t reg_filterop_sw_ovw_op:1;
		uint32_t rsv_5_7:3;
		/*only use in mod_filter3ch => 1 : filter out  0: center;*/
		uint32_t reg_filterop_3ch_en:1;
		/*y wdma dma enable;*/
		uint32_t reg_op_y_wdma_en:1;
		/*c wdma dma enable;*/
		uint32_t reg_op_c_wdma_en:1;
		/*only wlsb with Output data width;*/
		uint32_t reg_op_y_wdma_w1b_en:1;
		/*only wlsb with Output data width;*/
		uint32_t reg_op_c_wdma_w1b_en:1;
	};
	uint32_t val;
} ive_filterop_reg_h14_c;
typedef union {
	struct {
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix00     ;*/
		uint32_t reg_filterop_h_coef00:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix01;*/
		uint32_t reg_filterop_h_coef01:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix02;*/
		uint32_t reg_filterop_h_coef02:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix03;*/
		uint32_t reg_filterop_h_coef03:8;
	};
	uint32_t val;
} ive_filterop_reg_4_c;
typedef union {
	struct {
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix04;*/
		uint32_t reg_filterop_h_coef04:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix10;*/
		uint32_t reg_filterop_h_coef10:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix11;*/
		uint32_t reg_filterop_h_coef11:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix12;*/
		uint32_t reg_filterop_h_coef12:8;
	};
	uint32_t val;
} ive_filterop_reg_5_c;
typedef union {
	struct {
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix13;*/
		uint32_t reg_filterop_h_coef13:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix14;*/
		uint32_t reg_filterop_h_coef14:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix20;*/
		uint32_t reg_filterop_h_coef20:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix21;*/
		uint32_t reg_filterop_h_coef21:8;
	};
	uint32_t val;
} ive_filterop_reg_6_c;
typedef union {
	struct {
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix22;*/
		uint32_t reg_filterop_h_coef22:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix23;*/
		uint32_t reg_filterop_h_coef23:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix24;*/
		uint32_t reg_filterop_h_coef24:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix30;*/
		uint32_t reg_filterop_h_coef30:8;
	};
	uint32_t val;
} ive_filterop_reg_7_c;
typedef union {
	struct {
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix31;*/
		uint32_t reg_filterop_h_coef31:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix32;*/
		uint32_t reg_filterop_h_coef32:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix33;*/
		uint32_t reg_filterop_h_coef33:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix34;*/
		uint32_t reg_filterop_h_coef34:8;
	};
	uint32_t val;
} ive_filterop_reg_8_c;
typedef union {
	struct {
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix40;*/
		uint32_t reg_filterop_h_coef40:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix41;*/
		uint32_t reg_filterop_h_coef41:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix42;*/
		uint32_t reg_filterop_h_coef42:8;
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix43;*/
		uint32_t reg_filterop_h_coef43:8;
	};
	uint32_t val;
} ive_filterop_reg_9_c;
typedef union {
	struct {
		/*[filter/sobel/magandang] use 2's   show s7 (1signed + 7bit ) -> for pix44;*/
		uint32_t reg_filterop_h_coef44:8;
		/*value range [0-13];*/
		uint32_t reg_filterop_h_norm:4;
	};
	uint32_t val;
} ive_filterop_reg_10_c;
typedef union {
	struct {
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef00:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef01:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef02:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef03:8;
	};
	uint32_t val;
} ive_filterop_reg_11_c;
typedef union {
	struct {
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef04:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef10:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef11:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef12:8;
	};
	uint32_t val;
} ive_filterop_reg_12_c;
typedef union {
	struct {
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef13:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef14:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef20:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef21:8;
	};
	uint32_t val;
} ive_filterop_reg_13_c;
typedef union {
	struct {
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef22:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef23:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef24:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef30:8;
	};
	uint32_t val;
} ive_filterop_reg_14_c;
typedef union {
	struct {
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef31:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef32:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef33:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef34:8;
	};
	uint32_t val;
} ive_filterop_reg_15_c;
typedef union {
	struct {
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef40:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef41:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef42:8;
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef43:8;
	};
	uint32_t val;
} ive_filterop_reg_16_c;
typedef union {
	struct {
		/*[option] program v coeff : default will use filterop_h coeff transport;*/
		uint32_t reg_filterop_v_coef44:8;
		/*value range [0-13];*/
		uint32_t reg_filterop_v_norm:4;
	};
	uint32_t val;
} ive_filterop_reg_17_c;
typedef union {
	struct {
		/*default 1 : v filter coeff will use h coeff transport;*/
		uint32_t reg_filterop_mode_trans:1;
		uint32_t rsv_1_3:3;
		/*en_mode:
		0 : median filter
		1 : max filter
		2 : min filter;*/
		uint32_t reg_filterop_order_enmode:3;
		uint32_t rsv_7_15:9;
		/*use in MagAndAng ;*/
		uint32_t reg_filterop_mag_thr:16;
	};
	uint32_t val;
} ive_filterop_reg_18_c;
typedef union {
	struct {
		/*1 : use win 5x5 to do bersen
		0: use win 3x3 to do bernsen;*/
		uint32_t reg_filterop_bernsen_win5x5_en:1;
		uint32_t rsv_1_3:3;
		/*0 : normal mode => thrs = (max + min)/2
		1: threshold mode=> (thrs = (max+min)/2 + reg_filterop_bernsen_thr)/2
		2: paper omde => use local max : max-min and reg_filterop_u8ContrastThreshold;*/
		uint32_t reg_filterop_bernsen_mode:2;
		uint32_t rsv_6_7:2;
		/*;*/
		uint32_t reg_filterop_bernsen_thr:8;
		/*only used in bernsen_mode=2;*/
		uint32_t reg_filterop_u8ContrastThreshold:8;
	};
	uint32_t val;
} ive_filterop_reg_19_c;
typedef union {
	struct {
		/*unsigned threshold;*/
		uint32_t reg_filterop_lbp_u8bit_thr:8;
		/*signed threshold, use 2's   show s7 (1signed + 7bit );*/
		uint32_t reg_filterop_lbp_s8bit_thr:8;
		/*0: diff compare to signed threshold,
		1: abs compare to unsigned threshold;*/
		uint32_t reg_filterop_lbp_enmode:1;
	};
	uint32_t val;
} ive_filterop_reg_20_c;
typedef union {
	struct {
		/*U8 format : only set 255 or 0 ->  for pix00;*/
		uint32_t reg_filterop_op2_erodila_coef00:8;
		/*U8 format : only set 255 or 0 ->  for pix01;*/
		uint32_t reg_filterop_op2_erodila_coef01:8;
		/*U8 format : only set 255 or 0 ->  for pix02;*/
		uint32_t reg_filterop_op2_erodila_coef02:8;
		/*U8 format : only set 255 or 0 ->  for pix03;*/
		uint32_t reg_filterop_op2_erodila_coef03:8;
	};
	uint32_t val;
} ive_filterop_reg_21_c;
typedef union {
	struct {
		/*U8 format : only set 255 or 0 ->  for pix04;*/
		uint32_t reg_filterop_op2_erodila_coef04:8;
		/*U8 format : only set 255 or 0 ->  for pix10;*/
		uint32_t reg_filterop_op2_erodila_coef10:8;
		/*U8 format : only set 255 or 0 ->  for pix11;*/
		uint32_t reg_filterop_op2_erodila_coef11:8;
		/*U8 format : only set 255 or 0 ->  for pix12;*/
		uint32_t reg_filterop_op2_erodila_coef12:8;
	};
	uint32_t val;
} ive_filterop_reg_22_c;
typedef union {
	struct {
		/*U8 format : only set 255 or 0 ->  for pix13;*/
		uint32_t reg_filterop_op2_erodila_coef13:8;
		/*U8 format : only set 255 or 0 ->  for pix14;*/
		uint32_t reg_filterop_op2_erodila_coef14:8;
		/*U8 format : only set 255 or 0 ->  for pix20;*/
		uint32_t reg_filterop_op2_erodila_coef20:8;
		/*U8 format : only set 255 or 0 ->  for pix21;*/
		uint32_t reg_filterop_op2_erodila_coef21:8;
	};
	uint32_t val;
} ive_filterop_reg_23_c;
typedef union {
	struct {
		/*U8 format : only set 255 or 0 ->  for pix22;*/
		uint32_t reg_filterop_op2_erodila_coef22:8;
		/*U8 format : only set 255 or 0 ->  for pix23;*/
		uint32_t reg_filterop_op2_erodila_coef23:8;
		/*U8 format : only set 255 or 0 ->  for pix24;*/
		uint32_t reg_filterop_op2_erodila_coef24:8;
		/*U8 format : only set 255 or 0 ->  for pix30;*/
		uint32_t reg_filterop_op2_erodila_coef30:8;
	};
	uint32_t val;
} ive_filterop_reg_24_c;
typedef union {
	struct {
		/*U8 format : only set 255 or 0 ->  for pix31;*/
		uint32_t reg_filterop_op2_erodila_coef31:8;
		/*U8 format : only set 255 or 0 ->  for pix32;*/
		uint32_t reg_filterop_op2_erodila_coef32:8;
		/*U8 format : only set 255 or 0 ->  for pix33;*/
		uint32_t reg_filterop_op2_erodila_coef33:8;
		/*U8 format : only set 255 or 0 ->  for pix34;*/
		uint32_t reg_filterop_op2_erodila_coef34:8;
	};
	uint32_t val;
} ive_filterop_reg_25_c;
typedef union {
	struct {
		/*U8 format : only set 255 or 0 ->  for pix40;*/
		uint32_t reg_filterop_op2_erodila_coef40:8;
		/*U8 format : only set 255 or 0 ->  for pix41;*/
		uint32_t reg_filterop_op2_erodila_coef41:8;
		/*U8 format : only set 255 or 0 ->  for pix42;*/
		uint32_t reg_filterop_op2_erodila_coef42:8;
		/*U8 format : only set 255 or 0 ->  for pix43;*/
		uint32_t reg_filterop_op2_erodila_coef43:8;
	};
	uint32_t val;
} ive_filterop_reg_26_c;
typedef union {
	struct {
		/*U8 format : only set 255 or 0 ->  for pix44;*/
		uint32_t reg_filterop_op2_erodila_coef44:8;
	};
	uint32_t val;
} ive_filterop_reg_27_c;
typedef union {
	struct {
		/*set 0 : bypass center from erodilat module;*/
		uint32_t reg_filterop_op2_erodila_en:1;
	};
	uint32_t val;
} ive_filterop_reg_28_c;
typedef union {
	struct {
		/*dbg_en;*/
		uint32_t reg_csc_dbg_en:1;
		uint32_t rsv_1_3:3;
		/*dbg_hw_coeff_sel = [0:11];*/
		uint32_t reg_csc_dbg_coeff_sel:4;
		/*dbg_hw_coeff;*/
		uint32_t reg_csc_dbg_coeff:19;
	};
	uint32_t val;
} ive_filterop_reg_csc_dbg_coeff_c;
typedef union {
	struct {
		/*dbg_prob_pix_x;*/
		uint32_t reg_csc_dbg_prob_x:12;
		uint32_t rsv_12_15:4;
		/*dbg_prob_pix_y;*/
		uint32_t reg_csc_dbg_prob_y:12;
	};
	uint32_t val;
} ive_filterop_reg_csc_dbg_prob_pix_c;
typedef union {
	struct {
		/*dbg_src_pix;*/
		uint32_t reg_csc_dbg_src_data:24;
	};
	uint32_t val;
} ive_filterop_reg_csc_dbg_data_src_c;
typedef union {
	struct {
		/*dbg_dst_pix;*/
		uint32_t reg_csc_dbg_dst_data:24;
	};
	uint32_t val;
} ive_filterop_reg_csc_dbg_data_dat_c;
typedef union {
	struct {
		/*GradFg module enable
		1'b0: disable
		1'b1: enable;*/
		uint32_t reg_filterop_op2_gradfg_en:1;
		/*GradFg software reset
		1'b0: normal
		1'b1: reset;*/
		uint32_t reg_filterop_op2_gradfg_softrst:1;
		/*GradFg calculation mode
		1'b0: use current gradient
		1'b1: find min gradient;*/
		uint32_t reg_filterop_op2_gradfg_enmode:1;
		/*GradFg black pixels enable flag
		1'b0: no
		1'b1: yes;*/
		uint32_t reg_filterop_op2_gradfg_edwdark:1;
		uint32_t rsv_4_15:12;
		/*GradFg edge width adjustment factor
		range: 500 to 2000;*/
		uint32_t reg_filterop_op2_gradfg_edwfactor:16;
	};
	uint32_t val;
} ive_filterop_reg_33_c;
typedef union {
	struct {
		/*GradFg gradient vector correlation coefficient threshold
		range: 50 to 100;*/
		uint32_t reg_filterop_op2_gradfg_crlcoefthr:8;
		/*GradFg gradient amplitude threshold
		range: 0 to 20;*/
		uint32_t reg_filterop_op2_gradfg_magcrlthr:8;
		/*GradFg gradient magnitude difference threshold
		range: 2 to 8;*/
		uint32_t reg_filterop_op2_gradfg_minmagdiff:8;
		/*GradFg gradient amplitude noise threshold
		range: 1 to 8;*/
		uint32_t reg_filterop_op2_gradfg_noiseval:8;
	};
	uint32_t val;
} ive_filterop_reg_34_c;
typedef union {
	struct {
		/*0 : U8
		1 : S16
		2 : U16;*/
		uint32_t reg_filterop_map_enmode:2;
		uint32_t rsv_2_3:2;
		/*0 : h , v  -> wdma_y wdma_c will be active
		1 :h only -> wdma_y
		2: v only -> wdma_c
		3. h , v pack => {v ,h} -> wdma_y ;*/
		uint32_t reg_filterop_norm_out_ctrl:2;
		uint32_t rsv_6_7:2;
		/*0: Mag only ->  wdma_c
		1 : Mag + Ang -> wdma_y and wdma_c;*/
		uint32_t reg_filterop_magang_out_ctrl:1;
	};
	uint32_t val;
} ive_filterop_reg_110_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_dma_blen:1;
		/*;*/
		uint32_t reg_dma_en:1;
		uint32_t rsv_2_7:6;
		/*op;*/
		uint32_t reg_fmt_sel:4;
		uint32_t rsv_12_15:4;
		/*;*/
		uint32_t reg_sc_odma_hflip:1;
		/*;*/
		uint32_t reg_sc_odma_vflip:1;
		uint32_t rsv_18_19:2;
		/*;*/
		uint32_t reg_sc_422_avg:1;
		/*;*/
		uint32_t reg_sc_420_avg:1;
		/*0 : UV run away from zero
		1 : UV run to zero;*/
		uint32_t reg_c_round_mode:1;
		/*csc q mode outout BF16 enable
		when this bit set to 1
		1. reg_fmt_sel must set to 0x6
		2. reg_sc_csc_q_mode must set to 1
		and notice aht sc_hsv function would be auto disable ;*/
		uint32_t reg_bf16_en:1;
	};
	uint32_t val;
} ive_filterop_odma_reg_00_c;
typedef union {
	struct {
		/*output Y (R) base address;*/
		uint32_t reg_dma_y_base_low_part:32;
	};
	uint32_t val;
} ive_filterop_odma_reg_01_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_dma_y_base_high_part:8;
	};
	uint32_t val;
} ive_filterop_odma_reg_02_c;
typedef union {
	struct {
		/*putput U (G) base address;*/
		uint32_t reg_dma_u_base_low_part:32;
	};
	uint32_t val;
} ive_filterop_odma_reg_03_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_dma_u_base_high_part:8;
	};
	uint32_t val;
} ive_filterop_odma_reg_04_c;
typedef union {
	struct {
		/*output V (B) base address;*/
		uint32_t reg_dma_v_base_low_part:32;
	};
	uint32_t val;
} ive_filterop_odma_reg_05_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_dma_v_base_high_part:8;
	};
	uint32_t val;
} ive_filterop_odma_reg_06_c;
typedef union {
	struct {
		/*line pitch for Y/R/Packet data;*/
		uint32_t reg_dma_y_pitch:24;
	};
	uint32_t val;
} ive_filterop_odma_reg_07_c;
typedef union {
	struct {
		/*line pitch for C;*/
		uint32_t reg_dma_c_pitch:24;
	};
	uint32_t val;
} ive_filterop_odma_reg_08_c;
typedef union {
	struct {
		/*crop write start point ( in pixel base);*/
		uint32_t reg_dma_x_str:12;
	};
	uint32_t val;
} ive_filterop_odma_reg_09_c;
typedef union {
	struct {
		/*crop write start line (line line base);*/
		uint32_t reg_dma_y_str:12;
	};
	uint32_t val;
} ive_filterop_odma_reg_10_c;
typedef union {
	struct {
		/*output width, fill sub 1;*/
		uint32_t reg_dma_wd:12;
	};
	uint32_t val;
} ive_filterop_odma_reg_11_c;
typedef union {
	struct {
		/*output height, fill sub 1;*/
		uint32_t reg_dma_ht:12;
	};
	uint32_t val;
} ive_filterop_odma_reg_12_c;
typedef union {
	struct {
		/*dma debug information;*/
		uint32_t reg_dma_debug:32;
	};
	uint32_t val;
} ive_filterop_odma_reg_13_c;
typedef union {
	struct {
		/*line count hit target in axi_cmd_gen;*/
		uint32_t reg_dma_int_line_target:12;
		uint32_t rsv_12_15:4;
		/*line count hit target select:
		2'd0: from Y,
		2'd1: from U,
		2'd2: from V;*/
		uint32_t reg_dma_int_line_target_sel:2;
	};
	uint32_t val;
} ive_filterop_odma_reg_14_c;
typedef union {
	struct {
		/*cycle line count hit target in axi_cmd_gen;*/
		uint32_t reg_dma_int_cycle_line_target:11;
		uint32_t rsv_11_15:5;
		/*cycle line count hit target select:
		2'd0: from Y,
		2'd1: from U,
		2'd2: from V;*/
		uint32_t reg_dma_int_cycle_line_target_sel:2;
	};
	uint32_t val;
} ive_filterop_odma_reg_15_c;
typedef union {
	struct {
		/*latch odma y line count;*/
		uint32_t reg_dma_latch_line_cnt:1;
		uint32_t rsv_1_7:7;
		/*latched odma y line count;*/
		uint32_t reg_dma_latched_line_cnt:12;
	};
	uint32_t val;
} ive_filterop_odma_reg_16_c;
typedef union {
	struct {
		/*debug;*/
		uint32_t axi_active:1;
		/*debug;*/
		uint32_t axi_y_active:1;
		/*debug;*/
		uint32_t axi_u_active:1;
		/*debug;*/
		uint32_t axi_v_active:1;
		/*debug;*/
		uint32_t y_buf_full:1;
		/*debug;*/
		uint32_t y_buf_empty:1;
		/*debug;*/
		uint32_t u_buf_full:1;
		/*debug;*/
		uint32_t u_buf_empty:1;
		/*debug;*/
		uint32_t v_buf_full:1;
		/*debug;*/
		uint32_t v_buf_empty:1;
		/*debug;*/
		uint32_t line_target_hit:1;
		/*debug;*/
		uint32_t cycle_line_target_hit:1;
		/*debug;*/
		uint32_t axi_cmd_cs:4;
		/*debug;*/
		uint32_t y_line_cnt:12;
	};
	uint32_t val;
} ive_filterop_odma_reg_17_c;
typedef union {
	struct {
		/*canny low threshold;*/
		uint32_t reg_canny_lowthr:16;
		/*canny high threshold;*/
		uint32_t reg_canny_hithr:16;
	};
	uint32_t val;
} ive_filterop_reg_canny_0_c;
typedef union {
	struct {
		/*enable cannyhysedge module or not
		0:disable
		1:enable;*/
		uint32_t reg_canny_en:1;
		/*0:disable
		1:enable;*/
		uint32_t reg_canny_strong_point_cnt_en:1;
		/*should also set reg_canny_strong_point_cnt_en and reg_canny_strong_point_cnt to enable this bit
		0: the strong edge point will be tagged as weak-edge point when total strong edge point > reg_canny_strong_point_cnt
		1: the strong edge point will be tagged as non-edge point when total strong edge point > reg_canny_strong_point_cnt;*/
		uint32_t reg_canny_non_or_weak:1;
		uint32_t rsv_3_15:13;
		/*limit the number of strong edge points in each frame;*/
		uint32_t reg_canny_strong_point_cnt:16;
	};
	uint32_t val;
} ive_filterop_reg_canny_1_c;
typedef union {
	struct {
		/*the ending symbol for strong edge xy position in DRAM data ;*/
		uint32_t reg_canny_eof:32;
	};
	uint32_t val;
} ive_filterop_reg_canny_2_c;
typedef union {
	struct {
		/*DRAM address for storing strong edge xy position;*/
		uint32_t reg_canny_basel:32;
	};
	uint32_t val;
} ive_filterop_reg_canny_3_c;
typedef union {
	struct {
		/*DRAM address for storing strong edge xy position;*/
		uint32_t reg_canny_baseh:8;
	};
	uint32_t val;
} ive_filterop_reg_canny_4_c;
typedef union {
	struct {
		/*bypass  ive_st_candi_corner module
		0:disable
		1:enable;*/
		uint32_t reg_st_candi_corner_bypass:1;
		/*switch ive_st_candi_corner output when enable reg_st_candi_corner_bypass
		0:disable
		1:enable;*/
		uint32_t reg_st_candi_corner_switch_src:1;
	};
	uint32_t val;
} ive_filterop_reg_st_candi_0_c;
typedef union {
	struct {
		/*maximum eigen value;*/
		uint32_t reg_st_eigval_max_eigval:16;
		/*tile number;*/
		uint32_t reg_st_eigval_tile_num:4;
	};
	uint32_t val;
} ive_filterop_reg_st_eigval_0_c;
typedef union {
	struct {
		/*sw clear max eigen value;*/
		uint32_t reg_sw_clr_max_eigval:1;
	};
	uint32_t val;
} ive_filterop_reg_st_eigval_1_c;
typedef union {
	struct {
		/*unsigned 12 bit, update table value of inv_v_tab(rgb2hsv) or gamma_tab(rgb2lab) when reg_csc_tab_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_tab_sw_0:12;
		uint32_t rsv_12_15:4;
		/*unsigned 15 bit, update table value of inv_h_tab(rgb2hsv) or xyz_tab(rgb2lab) when reg_csc_tab_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_tab_sw_1:15;
	};
	uint32_t val;
} ive_filterop_reg_h190_c;
typedef union {
	struct {
		/*update rgb2hsv/rgb2lab table value by software
		0:use const, 1:update table by reg_csc_tab_sw_0 and reg_csc_tab_sw_1;*/
		uint32_t reg_filterop_op2_csc_tab_sw_update:1;
		uint32_t rsv_1_15:15;
		/*update yuv2rgb coeff value by software
		0: use const, 1:update coeff by reg_csc_coeff_sw;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_update:1;
	};
	uint32_t val;
} ive_filterop_reg_h194_c;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_00:19;
	};
	uint32_t val;
} ive_filterop_reg_csc_coeff_0_c;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_01:19;
	};
	uint32_t val;
} ive_filterop_reg_csc_coeff_1_c;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_02:19;
	};
	uint32_t val;
} ive_filterop_reg_csc_coeff_2_c;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_03:19;
	};
	uint32_t val;
} ive_filterop_reg_csc_coeff_3_c;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_04:19;
	};
	uint32_t val;
} ive_filterop_reg_csc_coeff_4_c;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_05:19;
	};
	uint32_t val;
} ive_filterop_reg_csc_coeff_5_c;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_06:19;
	};
	uint32_t val;
} ive_filterop_reg_csc_coeff_6_c;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_07:19;
	};
	uint32_t val;
} ive_filterop_reg_csc_coeff_7_c;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_08:19;
	};
	uint32_t val;
} ive_filterop_reg_csc_coeff_8_c;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_09:19;
	};
	uint32_t val;
} ive_filterop_reg_csc_coeff_9_c;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_10:19;
	};
	uint32_t val;
} ive_filterop_reg_csc_coeff_a_c;
typedef union {
	struct {
		/*unsigned 19 bit, update coeff of yuv2rgb/rgb2yuv when reg_csc_coeff_sw_update == 1;*/
		uint32_t reg_filterop_op2_csc_coeff_sw_11:19;
	};
	uint32_t val;
} ive_filterop_reg_csc_coeff_b_c;
typedef union {
	struct {
		/*en_mode:
		0,1,2,3: yuv2rgb
		4,5: yuv2rgb2hsv
		6,7: yuv2rgb2lab
		8,9,10,11: rgb2yuv;*/
		uint32_t reg_filterop_op2_csc_enmode:4;
		/*op2 csc enable;*/
		uint32_t reg_filterop_op2_csc_enable:1;
	};
	uint32_t val;
} ive_filterop_reg_h1c8_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_crop_y_start_x:16;
		/*;*/
		uint32_t reg_crop_y_end_x:16;
	};
	uint32_t val;
} ive_filterop_reg_cropy_s_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_crop_y_start_y:16;
		/*;*/
		uint32_t reg_crop_y_end_y:16;
	};
	uint32_t val;
} ive_filterop_reg_cropy_e_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_crop_y_enable:1;
	};
	uint32_t val;
} ive_filterop_reg_cropy_ctl_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_crop_c_start_x:16;
		/*;*/
		uint32_t reg_crop_c_end_x:16;
	};
	uint32_t val;
} ive_filterop_reg_cropc_s_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_crop_c_start_y:16;
		/*;*/
		uint32_t reg_crop_c_end_y:16;
	};
	uint32_t val;
} ive_filterop_reg_cropc_e_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_crop_c_enable:1;
	};
	uint32_t val;
} ive_filterop_reg_cropc_ctl_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_crop_odma_start_x:16;
		/*;*/
		uint32_t reg_crop_odma_end_x:16;
	};
	uint32_t val;
} ive_filterop_reg_crop_odma_s_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_crop_odma_start_y:16;
		/*;*/
		uint32_t reg_crop_odma_end_y:16;
	};
	uint32_t val;
} ive_filterop_reg_crop_odma_e_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_crop_odma_enable:1;
	};
	uint32_t val;
} ive_filterop_reg_crop_odma_ctl_c;
typedef struct {
	volatile ive_filterop_reg_1_c reg_1;
	volatile ive_filterop_reg_h04_c reg_h04;
	volatile ive_filterop_reg_2_c reg_2;
	volatile ive_filterop_reg_3_c reg_3;
	volatile ive_filterop_reg_h10_c reg_h10;
	volatile ive_filterop_reg_h14_c reg_h14;
	volatile uint32_t _reg_4_0; // 0x18
	volatile uint32_t _reg_4_1; // 0x1C
	volatile ive_filterop_reg_4_c reg_4;
	volatile ive_filterop_reg_5_c reg_5;
	volatile ive_filterop_reg_6_c reg_6;
	volatile ive_filterop_reg_7_c reg_7;
	volatile ive_filterop_reg_8_c reg_8;
	volatile ive_filterop_reg_9_c reg_9;
	volatile ive_filterop_reg_10_c reg_10;
	volatile uint32_t _reg_11_0; // 0x3C
	volatile ive_filterop_reg_11_c reg_11;
	volatile ive_filterop_reg_12_c reg_12;
	volatile ive_filterop_reg_13_c reg_13;
	volatile ive_filterop_reg_14_c reg_14;
	volatile ive_filterop_reg_15_c reg_15;
	volatile ive_filterop_reg_16_c reg_16;
	volatile ive_filterop_reg_17_c reg_17;
	volatile uint32_t _reg_18_0; // 0x5C
	volatile ive_filterop_reg_18_c reg_18;
	volatile ive_filterop_reg_19_c reg_19;
	volatile ive_filterop_reg_20_c reg_20;
	volatile uint32_t _reg_21_0; // 0x6C
	volatile ive_filterop_reg_21_c reg_21;
	volatile ive_filterop_reg_22_c reg_22;
	volatile ive_filterop_reg_23_c reg_23;
	volatile ive_filterop_reg_24_c reg_24;
	volatile ive_filterop_reg_25_c reg_25;
	volatile ive_filterop_reg_26_c reg_26;
	volatile ive_filterop_reg_27_c reg_27;
	volatile ive_filterop_reg_28_c reg_28;
	volatile ive_filterop_reg_csc_dbg_coeff_c REG_CSC_DBG_COEFF;
	volatile ive_filterop_reg_csc_dbg_prob_pix_c REG_CSC_DBG_PROB_PIX;
	volatile ive_filterop_reg_csc_dbg_data_src_c REG_CSC_DBG_DATA_SRC;
	volatile ive_filterop_reg_csc_dbg_data_dat_c REG_CSC_DBG_DATA_DAT;
	volatile uint32_t _reg_33_0; // 0xA0
	volatile uint32_t _reg_33_1; // 0xA4
	volatile uint32_t _reg_33_2; // 0xA8
	volatile uint32_t _reg_33_3; // 0xAC
	volatile uint32_t _reg_33_4; // 0xB0
	volatile uint32_t _reg_33_5; // 0xB4
	volatile uint32_t _reg_33_6; // 0xB8
	volatile uint32_t _reg_33_7; // 0xBC
	volatile uint32_t _reg_33_8; // 0xC0
	volatile uint32_t _reg_33_9; // 0xC4
	volatile uint32_t _reg_33_10; // 0xC8
	volatile uint32_t _reg_33_11; // 0xCC
	volatile uint32_t _reg_33_12; // 0xD0
	volatile uint32_t _reg_33_13; // 0xD4
	volatile uint32_t _reg_33_14; // 0xD8
	volatile uint32_t _reg_33_15; // 0xDC
	volatile uint32_t _reg_33_16; // 0xE0
	volatile uint32_t _reg_33_17; // 0xE4
	volatile uint32_t _reg_33_18; // 0xE8
	volatile uint32_t _reg_33_19; // 0xEC
	volatile uint32_t _reg_33_20; // 0xF0
	volatile uint32_t _reg_33_21; // 0xF4
	volatile uint32_t _reg_33_22; // 0xF8
	volatile uint32_t _reg_33_23; // 0xFC
	volatile ive_filterop_reg_33_c reg_33;
	volatile ive_filterop_reg_34_c reg_34;
	volatile uint32_t _reg_110_0; // 0x108
	volatile uint32_t _reg_110_1; // 0x10C
	volatile ive_filterop_reg_110_c reg_110;
	volatile uint32_t _odma_reg_00_0; // 0x114
	volatile uint32_t _odma_reg_00_1; // 0x118
	volatile uint32_t _odma_reg_00_2; // 0x11C
	volatile ive_filterop_odma_reg_00_c odma_reg_00;
	volatile ive_filterop_odma_reg_01_c odma_reg_01;
	volatile ive_filterop_odma_reg_02_c odma_reg_02;
	volatile ive_filterop_odma_reg_03_c odma_reg_03;
	volatile ive_filterop_odma_reg_04_c odma_reg_04;
	volatile ive_filterop_odma_reg_05_c odma_reg_05;
	volatile ive_filterop_odma_reg_06_c odma_reg_06;
	volatile ive_filterop_odma_reg_07_c odma_reg_07;
	volatile ive_filterop_odma_reg_08_c odma_reg_08;
	volatile ive_filterop_odma_reg_09_c odma_reg_09;
	volatile ive_filterop_odma_reg_10_c odma_reg_10;
	volatile ive_filterop_odma_reg_11_c odma_reg_11;
	volatile ive_filterop_odma_reg_12_c odma_reg_12;
	volatile ive_filterop_odma_reg_13_c odma_reg_13;
	volatile ive_filterop_odma_reg_14_c odma_reg_14;
	volatile ive_filterop_odma_reg_15_c odma_reg_15;
	volatile ive_filterop_odma_reg_16_c odma_reg_16;
	volatile ive_filterop_odma_reg_17_c odma_reg_17;
	volatile uint32_t _reg_canny_0_0; // 0x168
	volatile uint32_t _reg_canny_0_1; // 0x16C
	volatile ive_filterop_reg_canny_0_c reg_canny_0;
	volatile ive_filterop_reg_canny_1_c reg_canny_1;
	volatile ive_filterop_reg_canny_2_c reg_canny_2;
	volatile ive_filterop_reg_canny_3_c reg_canny_3;
	volatile ive_filterop_reg_canny_4_c reg_canny_4;
	volatile ive_filterop_reg_st_candi_0_c reg_st_candi_0;
	volatile ive_filterop_reg_st_eigval_0_c reg_st_eigval_0;
	volatile ive_filterop_reg_st_eigval_1_c reg_st_eigval_1;
	volatile ive_filterop_reg_h190_c reg_h190;
	volatile ive_filterop_reg_h194_c reg_h194;
	volatile ive_filterop_reg_csc_coeff_0_c reg_csc_coeff_0;
	volatile ive_filterop_reg_csc_coeff_1_c reg_csc_coeff_1;
	volatile ive_filterop_reg_csc_coeff_2_c reg_csc_coeff_2;
	volatile ive_filterop_reg_csc_coeff_3_c reg_csc_coeff_3;
	volatile ive_filterop_reg_csc_coeff_4_c reg_csc_coeff_4;
	volatile ive_filterop_reg_csc_coeff_5_c reg_csc_coeff_5;
	volatile ive_filterop_reg_csc_coeff_6_c reg_csc_coeff_6;
	volatile ive_filterop_reg_csc_coeff_7_c reg_csc_coeff_7;
	volatile ive_filterop_reg_csc_coeff_8_c reg_csc_coeff_8;
	volatile ive_filterop_reg_csc_coeff_9_c reg_csc_coeff_9;
	volatile ive_filterop_reg_csc_coeff_a_c reg_csc_coeff_a;
	volatile ive_filterop_reg_csc_coeff_b_c reg_csc_coeff_b;
	volatile ive_filterop_reg_h1c8_c reg_h1c8;
	volatile uint32_t _reg_cropy_s_0; // 0x1CC
	volatile ive_filterop_reg_cropy_s_c reg_cropy_s;
	volatile ive_filterop_reg_cropy_e_c reg_cropy_e;
	volatile ive_filterop_reg_cropy_ctl_c reg_cropy_ctl;
	volatile uint32_t _reg_cropc_s_0; // 0x1DC
	volatile ive_filterop_reg_cropc_s_c reg_cropc_s;
	volatile ive_filterop_reg_cropc_e_c reg_cropc_e;
	volatile ive_filterop_reg_cropc_ctl_c reg_cropc_ctl;
	volatile uint32_t _reg_crop_odma_s_0; // 0x1EC
	volatile ive_filterop_reg_crop_odma_s_c reg_crop_odma_s;
	volatile ive_filterop_reg_crop_odma_e_c reg_crop_odma_e;
	volatile ive_filterop_reg_crop_odma_ctl_c reg_crop_odma_ctl;
} ive_filterop_c;
#ifdef __cplusplus

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void ive_filterop_dump_ini(FILE* fp, ive_filterop_c* p) {
	fprintf(fp, "reg_softrst = 0x%x\n",p->reg_1.reg_softrst);
	fprintf(fp, "reg_softrst_wdma_y = 0x%x\n",p->reg_1.reg_softrst_wdma_y);
	fprintf(fp, "reg_softrst_wdma_c = 0x%x\n",p->reg_1.reg_softrst_wdma_c);
	fprintf(fp, "reg_softrst_rdma_gradfg = 0x%x\n",p->reg_1.reg_softrst_rdma_gradfg);
	fprintf(fp, "reg_softrst_op1 = 0x%x\n",p->reg_1.reg_softrst_op1);
	fprintf(fp, "reg_softrst_filter3ch = 0x%x\n",p->reg_1.reg_softrst_filter3ch);
	fprintf(fp, "reg_softrst_st = 0x%x\n",p->reg_1.reg_softrst_st);
	fprintf(fp, "reg_softrst_odma = 0x%x\n",p->reg_1.reg_softrst_odma);
	fprintf(fp, "reg_gradfg_bggrad_rdma_en = 0x%x\n",p->reg_h04.reg_gradfg_bggrad_rdma_en);
	fprintf(fp, "reg_gradfg_bggrad_uv_swap = 0x%x\n",p->reg_h04.reg_gradfg_bggrad_uv_swap);
	fprintf(fp, "reg_shdw_sel = 0x%x\n",p->reg_2.reg_shdw_sel);
	fprintf(fp, "reg_ctrl_dmy1 = 0x%x\n",p->reg_3.reg_ctrl_dmy1);
	fprintf(fp, "reg_filterop_mode = 0x%x\n",p->reg_h10.reg_filterop_mode);
	fprintf(fp, "reg_filterop_op1_cmd = 0x%x\n",p->reg_h14.reg_filterop_op1_cmd);
	fprintf(fp, "reg_filterop_sw_ovw_op = 0x%x\n",p->reg_h14.reg_filterop_sw_ovw_op);
	fprintf(fp, "reg_filterop_3ch_en = 0x%x\n",p->reg_h14.reg_filterop_3ch_en);
	fprintf(fp, "reg_op_y_wdma_en = 0x%x\n",p->reg_h14.reg_op_y_wdma_en);
	fprintf(fp, "reg_op_c_wdma_en = 0x%x\n",p->reg_h14.reg_op_c_wdma_en);
	fprintf(fp, "reg_op_y_wdma_w1b_en = 0x%x\n",p->reg_h14.reg_op_y_wdma_w1b_en);
	fprintf(fp, "reg_op_c_wdma_w1b_en = 0x%x\n",p->reg_h14.reg_op_c_wdma_w1b_en);
	fprintf(fp, "reg_filterop_h_coef00 = 0x%x\n",p->reg_4.reg_filterop_h_coef00);
	fprintf(fp, "reg_filterop_h_coef01 = 0x%x\n",p->reg_4.reg_filterop_h_coef01);
	fprintf(fp, "reg_filterop_h_coef02 = 0x%x\n",p->reg_4.reg_filterop_h_coef02);
	fprintf(fp, "reg_filterop_h_coef03 = 0x%x\n",p->reg_4.reg_filterop_h_coef03);
	fprintf(fp, "reg_filterop_h_coef04 = 0x%x\n",p->reg_5.reg_filterop_h_coef04);
	fprintf(fp, "reg_filterop_h_coef10 = 0x%x\n",p->reg_5.reg_filterop_h_coef10);
	fprintf(fp, "reg_filterop_h_coef11 = 0x%x\n",p->reg_5.reg_filterop_h_coef11);
	fprintf(fp, "reg_filterop_h_coef12 = 0x%x\n",p->reg_5.reg_filterop_h_coef12);
	fprintf(fp, "reg_filterop_h_coef13 = 0x%x\n",p->reg_6.reg_filterop_h_coef13);
	fprintf(fp, "reg_filterop_h_coef14 = 0x%x\n",p->reg_6.reg_filterop_h_coef14);
	fprintf(fp, "reg_filterop_h_coef20 = 0x%x\n",p->reg_6.reg_filterop_h_coef20);
	fprintf(fp, "reg_filterop_h_coef21 = 0x%x\n",p->reg_6.reg_filterop_h_coef21);
	fprintf(fp, "reg_filterop_h_coef22 = 0x%x\n",p->reg_7.reg_filterop_h_coef22);
	fprintf(fp, "reg_filterop_h_coef23 = 0x%x\n",p->reg_7.reg_filterop_h_coef23);
	fprintf(fp, "reg_filterop_h_coef24 = 0x%x\n",p->reg_7.reg_filterop_h_coef24);
	fprintf(fp, "reg_filterop_h_coef30 = 0x%x\n",p->reg_7.reg_filterop_h_coef30);
	fprintf(fp, "reg_filterop_h_coef31 = 0x%x\n",p->reg_8.reg_filterop_h_coef31);
	fprintf(fp, "reg_filterop_h_coef32 = 0x%x\n",p->reg_8.reg_filterop_h_coef32);
	fprintf(fp, "reg_filterop_h_coef33 = 0x%x\n",p->reg_8.reg_filterop_h_coef33);
	fprintf(fp, "reg_filterop_h_coef34 = 0x%x\n",p->reg_8.reg_filterop_h_coef34);
	fprintf(fp, "reg_filterop_h_coef40 = 0x%x\n",p->reg_9.reg_filterop_h_coef40);
	fprintf(fp, "reg_filterop_h_coef41 = 0x%x\n",p->reg_9.reg_filterop_h_coef41);
	fprintf(fp, "reg_filterop_h_coef42 = 0x%x\n",p->reg_9.reg_filterop_h_coef42);
	fprintf(fp, "reg_filterop_h_coef43 = 0x%x\n",p->reg_9.reg_filterop_h_coef43);
	fprintf(fp, "reg_filterop_h_coef44 = 0x%x\n",p->reg_10.reg_filterop_h_coef44);
	fprintf(fp, "reg_filterop_h_norm = 0x%x\n",p->reg_10.reg_filterop_h_norm);
	fprintf(fp, "reg_filterop_v_coef00 = 0x%x\n",p->reg_11.reg_filterop_v_coef00);
	fprintf(fp, "reg_filterop_v_coef01 = 0x%x\n",p->reg_11.reg_filterop_v_coef01);
	fprintf(fp, "reg_filterop_v_coef02 = 0x%x\n",p->reg_11.reg_filterop_v_coef02);
	fprintf(fp, "reg_filterop_v_coef03 = 0x%x\n",p->reg_11.reg_filterop_v_coef03);
	fprintf(fp, "reg_filterop_v_coef04 = 0x%x\n",p->reg_12.reg_filterop_v_coef04);
	fprintf(fp, "reg_filterop_v_coef10 = 0x%x\n",p->reg_12.reg_filterop_v_coef10);
	fprintf(fp, "reg_filterop_v_coef11 = 0x%x\n",p->reg_12.reg_filterop_v_coef11);
	fprintf(fp, "reg_filterop_v_coef12 = 0x%x\n",p->reg_12.reg_filterop_v_coef12);
	fprintf(fp, "reg_filterop_v_coef13 = 0x%x\n",p->reg_13.reg_filterop_v_coef13);
	fprintf(fp, "reg_filterop_v_coef14 = 0x%x\n",p->reg_13.reg_filterop_v_coef14);
	fprintf(fp, "reg_filterop_v_coef20 = 0x%x\n",p->reg_13.reg_filterop_v_coef20);
	fprintf(fp, "reg_filterop_v_coef21 = 0x%x\n",p->reg_13.reg_filterop_v_coef21);
	fprintf(fp, "reg_filterop_v_coef22 = 0x%x\n",p->reg_14.reg_filterop_v_coef22);
	fprintf(fp, "reg_filterop_v_coef23 = 0x%x\n",p->reg_14.reg_filterop_v_coef23);
	fprintf(fp, "reg_filterop_v_coef24 = 0x%x\n",p->reg_14.reg_filterop_v_coef24);
	fprintf(fp, "reg_filterop_v_coef30 = 0x%x\n",p->reg_14.reg_filterop_v_coef30);
	fprintf(fp, "reg_filterop_v_coef31 = 0x%x\n",p->reg_15.reg_filterop_v_coef31);
	fprintf(fp, "reg_filterop_v_coef32 = 0x%x\n",p->reg_15.reg_filterop_v_coef32);
	fprintf(fp, "reg_filterop_v_coef33 = 0x%x\n",p->reg_15.reg_filterop_v_coef33);
	fprintf(fp, "reg_filterop_v_coef34 = 0x%x\n",p->reg_15.reg_filterop_v_coef34);
	fprintf(fp, "reg_filterop_v_coef40 = 0x%x\n",p->reg_16.reg_filterop_v_coef40);
	fprintf(fp, "reg_filterop_v_coef41 = 0x%x\n",p->reg_16.reg_filterop_v_coef41);
	fprintf(fp, "reg_filterop_v_coef42 = 0x%x\n",p->reg_16.reg_filterop_v_coef42);
	fprintf(fp, "reg_filterop_v_coef43 = 0x%x\n",p->reg_16.reg_filterop_v_coef43);
	fprintf(fp, "reg_filterop_v_coef44 = 0x%x\n",p->reg_17.reg_filterop_v_coef44);
	fprintf(fp, "reg_filterop_v_norm = 0x%x\n",p->reg_17.reg_filterop_v_norm);
	fprintf(fp, "reg_filterop_mode_trans = 0x%x\n",p->reg_18.reg_filterop_mode_trans);
	fprintf(fp, "reg_filterop_order_enmode = 0x%x\n",p->reg_18.reg_filterop_order_enmode);
	fprintf(fp, "reg_filterop_mag_thr = 0x%x\n",p->reg_18.reg_filterop_mag_thr);
	fprintf(fp, "reg_filterop_bernsen_win5x5_en = 0x%x\n",p->reg_19.reg_filterop_bernsen_win5x5_en);
	fprintf(fp, "reg_filterop_bernsen_mode = 0x%x\n",p->reg_19.reg_filterop_bernsen_mode);
	fprintf(fp, "reg_filterop_bernsen_thr = 0x%x\n",p->reg_19.reg_filterop_bernsen_thr);
	fprintf(fp, "reg_filterop_u8ContrastThreshold = 0x%x\n",p->reg_19.reg_filterop_u8ContrastThreshold);
	fprintf(fp, "reg_filterop_lbp_u8bit_thr = 0x%x\n",p->reg_20.reg_filterop_lbp_u8bit_thr);
	fprintf(fp, "reg_filterop_lbp_s8bit_thr = 0x%x\n",p->reg_20.reg_filterop_lbp_s8bit_thr);
	fprintf(fp, "reg_filterop_lbp_enmode = 0x%x\n",p->reg_20.reg_filterop_lbp_enmode);
	fprintf(fp, "reg_filterop_op2_erodila_coef00 = 0x%x\n",p->reg_21.reg_filterop_op2_erodila_coef00);
	fprintf(fp, "reg_filterop_op2_erodila_coef01 = 0x%x\n",p->reg_21.reg_filterop_op2_erodila_coef01);
	fprintf(fp, "reg_filterop_op2_erodila_coef02 = 0x%x\n",p->reg_21.reg_filterop_op2_erodila_coef02);
	fprintf(fp, "reg_filterop_op2_erodila_coef03 = 0x%x\n",p->reg_21.reg_filterop_op2_erodila_coef03);
	fprintf(fp, "reg_filterop_op2_erodila_coef04 = 0x%x\n",p->reg_22.reg_filterop_op2_erodila_coef04);
	fprintf(fp, "reg_filterop_op2_erodila_coef10 = 0x%x\n",p->reg_22.reg_filterop_op2_erodila_coef10);
	fprintf(fp, "reg_filterop_op2_erodila_coef11 = 0x%x\n",p->reg_22.reg_filterop_op2_erodila_coef11);
	fprintf(fp, "reg_filterop_op2_erodila_coef12 = 0x%x\n",p->reg_22.reg_filterop_op2_erodila_coef12);
	fprintf(fp, "reg_filterop_op2_erodila_coef13 = 0x%x\n",p->reg_23.reg_filterop_op2_erodila_coef13);
	fprintf(fp, "reg_filterop_op2_erodila_coef14 = 0x%x\n",p->reg_23.reg_filterop_op2_erodila_coef14);
	fprintf(fp, "reg_filterop_op2_erodila_coef20 = 0x%x\n",p->reg_23.reg_filterop_op2_erodila_coef20);
	fprintf(fp, "reg_filterop_op2_erodila_coef21 = 0x%x\n",p->reg_23.reg_filterop_op2_erodila_coef21);
	fprintf(fp, "reg_filterop_op2_erodila_coef22 = 0x%x\n",p->reg_24.reg_filterop_op2_erodila_coef22);
	fprintf(fp, "reg_filterop_op2_erodila_coef23 = 0x%x\n",p->reg_24.reg_filterop_op2_erodila_coef23);
	fprintf(fp, "reg_filterop_op2_erodila_coef24 = 0x%x\n",p->reg_24.reg_filterop_op2_erodila_coef24);
	fprintf(fp, "reg_filterop_op2_erodila_coef30 = 0x%x\n",p->reg_24.reg_filterop_op2_erodila_coef30);
	fprintf(fp, "reg_filterop_op2_erodila_coef31 = 0x%x\n",p->reg_25.reg_filterop_op2_erodila_coef31);
	fprintf(fp, "reg_filterop_op2_erodila_coef32 = 0x%x\n",p->reg_25.reg_filterop_op2_erodila_coef32);
	fprintf(fp, "reg_filterop_op2_erodila_coef33 = 0x%x\n",p->reg_25.reg_filterop_op2_erodila_coef33);
	fprintf(fp, "reg_filterop_op2_erodila_coef34 = 0x%x\n",p->reg_25.reg_filterop_op2_erodila_coef34);
	fprintf(fp, "reg_filterop_op2_erodila_coef40 = 0x%x\n",p->reg_26.reg_filterop_op2_erodila_coef40);
	fprintf(fp, "reg_filterop_op2_erodila_coef41 = 0x%x\n",p->reg_26.reg_filterop_op2_erodila_coef41);
	fprintf(fp, "reg_filterop_op2_erodila_coef42 = 0x%x\n",p->reg_26.reg_filterop_op2_erodila_coef42);
	fprintf(fp, "reg_filterop_op2_erodila_coef43 = 0x%x\n",p->reg_26.reg_filterop_op2_erodila_coef43);
	fprintf(fp, "reg_filterop_op2_erodila_coef44 = 0x%x\n",p->reg_27.reg_filterop_op2_erodila_coef44);
	fprintf(fp, "reg_filterop_op2_erodila_en = 0x%x\n",p->reg_28.reg_filterop_op2_erodila_en);
	fprintf(fp, "reg_csc_dbg_en = 0x%x\n",p->REG_CSC_DBG_COEFF.reg_csc_dbg_en);
	fprintf(fp, "reg_csc_dbg_coeff_sel = 0x%x\n",p->REG_CSC_DBG_COEFF.reg_csc_dbg_coeff_sel);
	fprintf(fp, "reg_csc_dbg_coeff = 0x%x\n",p->REG_CSC_DBG_COEFF.reg_csc_dbg_coeff);
	fprintf(fp, "reg_csc_dbg_prob_x = 0x%x\n",p->REG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_x);
	fprintf(fp, "reg_csc_dbg_prob_y = 0x%x\n",p->REG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_y);
	fprintf(fp, "reg_csc_dbg_src_data = 0x%x\n",p->REG_CSC_DBG_DATA_SRC.reg_csc_dbg_src_data);
	fprintf(fp, "reg_csc_dbg_dst_data = 0x%x\n",p->REG_CSC_DBG_DATA_DAT.reg_csc_dbg_dst_data);
	fprintf(fp, "reg_filterop_op2_gradfg_en = 0x%x\n",p->reg_33.reg_filterop_op2_gradfg_en);
	fprintf(fp, "reg_filterop_op2_gradfg_softrst = 0x%x\n",p->reg_33.reg_filterop_op2_gradfg_softrst);
	fprintf(fp, "reg_filterop_op2_gradfg_enmode = 0x%x\n",p->reg_33.reg_filterop_op2_gradfg_enmode);
	fprintf(fp, "reg_filterop_op2_gradfg_edwdark = 0x%x\n",p->reg_33.reg_filterop_op2_gradfg_edwdark);
	fprintf(fp, "reg_filterop_op2_gradfg_edwfactor = 0x%x\n",p->reg_33.reg_filterop_op2_gradfg_edwfactor);
	fprintf(fp, "reg_filterop_op2_gradfg_crlcoefthr = 0x%x\n",p->reg_34.reg_filterop_op2_gradfg_crlcoefthr);
	fprintf(fp, "reg_filterop_op2_gradfg_magcrlthr = 0x%x\n",p->reg_34.reg_filterop_op2_gradfg_magcrlthr);
	fprintf(fp, "reg_filterop_op2_gradfg_minmagdiff = 0x%x\n",p->reg_34.reg_filterop_op2_gradfg_minmagdiff);
	fprintf(fp, "reg_filterop_op2_gradfg_noiseval = 0x%x\n",p->reg_34.reg_filterop_op2_gradfg_noiseval);
	fprintf(fp, "reg_filterop_map_enmode = 0x%x\n",p->reg_110.reg_filterop_map_enmode);
	fprintf(fp, "reg_filterop_norm_out_ctrl = 0x%x\n",p->reg_110.reg_filterop_norm_out_ctrl);
	fprintf(fp, "reg_filterop_magang_out_ctrl = 0x%x\n",p->reg_110.reg_filterop_magang_out_ctrl);
	fprintf(fp, "reg_dma_blen = 0x%x\n",p->odma_reg_00.reg_dma_blen);
	fprintf(fp, "reg_dma_en = 0x%x\n",p->odma_reg_00.reg_dma_en);
	fprintf(fp, "reg_fmt_sel = 0x%x\n",p->odma_reg_00.reg_fmt_sel);
	fprintf(fp, "reg_sc_odma_hflip = 0x%x\n",p->odma_reg_00.reg_sc_odma_hflip);
	fprintf(fp, "reg_sc_odma_vflip = 0x%x\n",p->odma_reg_00.reg_sc_odma_vflip);
	fprintf(fp, "reg_sc_422_avg = 0x%x\n",p->odma_reg_00.reg_sc_422_avg);
	fprintf(fp, "reg_sc_420_avg = 0x%x\n",p->odma_reg_00.reg_sc_420_avg);
	fprintf(fp, "reg_c_round_mode = 0x%x\n",p->odma_reg_00.reg_c_round_mode);
	fprintf(fp, "reg_bf16_en = 0x%x\n",p->odma_reg_00.reg_bf16_en);
	fprintf(fp, "reg_dma_y_base_low_part = 0x%x\n",p->odma_reg_01.reg_dma_y_base_low_part);
	fprintf(fp, "reg_dma_y_base_high_part = 0x%x\n",p->odma_reg_02.reg_dma_y_base_high_part);
	fprintf(fp, "reg_dma_u_base_low_part = 0x%x\n",p->odma_reg_03.reg_dma_u_base_low_part);
	fprintf(fp, "reg_dma_u_base_high_part = 0x%x\n",p->odma_reg_04.reg_dma_u_base_high_part);
	fprintf(fp, "reg_dma_v_base_low_part = 0x%x\n",p->odma_reg_05.reg_dma_v_base_low_part);
	fprintf(fp, "reg_dma_v_base_high_part = 0x%x\n",p->odma_reg_06.reg_dma_v_base_high_part);
	fprintf(fp, "reg_dma_y_pitch = 0x%x\n",p->odma_reg_07.reg_dma_y_pitch);
	fprintf(fp, "reg_dma_c_pitch = 0x%x\n",p->odma_reg_08.reg_dma_c_pitch);
	fprintf(fp, "reg_dma_x_str = 0x%x\n",p->odma_reg_09.reg_dma_x_str);
	fprintf(fp, "reg_dma_y_str = 0x%x\n",p->odma_reg_10.reg_dma_y_str);
	fprintf(fp, "reg_dma_wd = 0x%x\n",p->odma_reg_11.reg_dma_wd);
	fprintf(fp, "reg_dma_ht = 0x%x\n",p->odma_reg_12.reg_dma_ht);
	fprintf(fp, "reg_dma_debug = 0x%x\n",p->odma_reg_13.reg_dma_debug);
	fprintf(fp, "reg_dma_int_line_target = 0x%x\n",p->odma_reg_14.reg_dma_int_line_target);
	fprintf(fp, "reg_dma_int_line_target_sel = 0x%x\n",p->odma_reg_14.reg_dma_int_line_target_sel);
	fprintf(fp, "reg_dma_int_cycle_line_target = 0x%x\n",p->odma_reg_15.reg_dma_int_cycle_line_target);
	fprintf(fp, "reg_dma_int_cycle_line_target_sel = 0x%x\n",p->odma_reg_15.reg_dma_int_cycle_line_target_sel);
	fprintf(fp, "reg_dma_latch_line_cnt = 0x%x\n",p->odma_reg_16.reg_dma_latch_line_cnt);
	fprintf(fp, "reg_dma_latched_line_cnt = 0x%x\n",p->odma_reg_16.reg_dma_latched_line_cnt);
	fprintf(fp, "axi_active = 0x%x\n",p->odma_reg_17.axi_active);
	fprintf(fp, "axi_y_active = 0x%x\n",p->odma_reg_17.axi_y_active);
	fprintf(fp, "axi_u_active = 0x%x\n",p->odma_reg_17.axi_u_active);
	fprintf(fp, "axi_v_active = 0x%x\n",p->odma_reg_17.axi_v_active);
	fprintf(fp, "y_buf_full = 0x%x\n",p->odma_reg_17.y_buf_full);
	fprintf(fp, "y_buf_empty = 0x%x\n",p->odma_reg_17.y_buf_empty);
	fprintf(fp, "u_buf_full = 0x%x\n",p->odma_reg_17.u_buf_full);
	fprintf(fp, "u_buf_empty = 0x%x\n",p->odma_reg_17.u_buf_empty);
	fprintf(fp, "v_buf_full = 0x%x\n",p->odma_reg_17.v_buf_full);
	fprintf(fp, "v_buf_empty = 0x%x\n",p->odma_reg_17.v_buf_empty);
	fprintf(fp, "line_target_hit = 0x%x\n",p->odma_reg_17.line_target_hit);
	fprintf(fp, "cycle_line_target_hit = 0x%x\n",p->odma_reg_17.cycle_line_target_hit);
	fprintf(fp, "axi_cmd_cs = 0x%x\n",p->odma_reg_17.axi_cmd_cs);
	fprintf(fp, "y_line_cnt = 0x%x\n",p->odma_reg_17.y_line_cnt);
	fprintf(fp, "reg_canny_lowthr = 0x%x\n",p->reg_canny_0.reg_canny_lowthr);
	fprintf(fp, "reg_canny_hithr = 0x%x\n",p->reg_canny_0.reg_canny_hithr);
	fprintf(fp, "reg_canny_en = 0x%x\n",p->reg_canny_1.reg_canny_en);
	fprintf(fp, "reg_canny_strong_point_cnt_en = 0x%x\n",p->reg_canny_1.reg_canny_strong_point_cnt_en);
	fprintf(fp, "reg_canny_non_or_weak = 0x%x\n",p->reg_canny_1.reg_canny_non_or_weak);
	fprintf(fp, "reg_canny_strong_point_cnt = 0x%x\n",p->reg_canny_1.reg_canny_strong_point_cnt);
	fprintf(fp, "reg_canny_eof = 0x%x\n",p->reg_canny_2.reg_canny_eof);
	fprintf(fp, "reg_canny_basel = 0x%x\n",p->reg_canny_3.reg_canny_basel);
	fprintf(fp, "reg_canny_baseh = 0x%x\n",p->reg_canny_4.reg_canny_baseh);
	fprintf(fp, "reg_st_candi_corner_bypass = 0x%x\n",p->reg_st_candi_0.reg_st_candi_corner_bypass);
	fprintf(fp, "reg_st_candi_corner_switch_src = 0x%x\n",p->reg_st_candi_0.reg_st_candi_corner_switch_src);
	fprintf(fp, "reg_st_eigval_max_eigval = 0x%x\n",p->reg_st_eigval_0.reg_st_eigval_max_eigval);
	fprintf(fp, "reg_st_eigval_tile_num = 0x%x\n",p->reg_st_eigval_0.reg_st_eigval_tile_num);
	fprintf(fp, "reg_sw_clr_max_eigval = 0x%x\n",p->reg_st_eigval_1.reg_sw_clr_max_eigval);
	fprintf(fp, "reg_filterop_op2_csc_tab_sw_0 = 0x%x\n",p->reg_h190.reg_filterop_op2_csc_tab_sw_0);
	fprintf(fp, "reg_filterop_op2_csc_tab_sw_1 = 0x%x\n",p->reg_h190.reg_filterop_op2_csc_tab_sw_1);
	fprintf(fp, "reg_filterop_op2_csc_tab_sw_update = 0x%x\n",p->reg_h194.reg_filterop_op2_csc_tab_sw_update);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_update = 0x%x\n",p->reg_h194.reg_filterop_op2_csc_coeff_sw_update);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_00 = 0x%x\n",p->reg_csc_coeff_0.reg_filterop_op2_csc_coeff_sw_00);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_01 = 0x%x\n",p->reg_csc_coeff_1.reg_filterop_op2_csc_coeff_sw_01);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_02 = 0x%x\n",p->reg_csc_coeff_2.reg_filterop_op2_csc_coeff_sw_02);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_03 = 0x%x\n",p->reg_csc_coeff_3.reg_filterop_op2_csc_coeff_sw_03);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_04 = 0x%x\n",p->reg_csc_coeff_4.reg_filterop_op2_csc_coeff_sw_04);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_05 = 0x%x\n",p->reg_csc_coeff_5.reg_filterop_op2_csc_coeff_sw_05);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_06 = 0x%x\n",p->reg_csc_coeff_6.reg_filterop_op2_csc_coeff_sw_06);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_07 = 0x%x\n",p->reg_csc_coeff_7.reg_filterop_op2_csc_coeff_sw_07);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_08 = 0x%x\n",p->reg_csc_coeff_8.reg_filterop_op2_csc_coeff_sw_08);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_09 = 0x%x\n",p->reg_csc_coeff_9.reg_filterop_op2_csc_coeff_sw_09);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_10 = 0x%x\n",p->reg_csc_coeff_a.reg_filterop_op2_csc_coeff_sw_10);
	fprintf(fp, "reg_filterop_op2_csc_coeff_sw_11 = 0x%x\n",p->reg_csc_coeff_b.reg_filterop_op2_csc_coeff_sw_11);
	fprintf(fp, "reg_filterop_op2_csc_enmode = 0x%x\n",p->reg_h1c8.reg_filterop_op2_csc_enmode);
	fprintf(fp, "reg_filterop_op2_csc_enable = 0x%x\n",p->reg_h1c8.reg_filterop_op2_csc_enable);
	fprintf(fp, "reg_crop_y_start_x = 0x%x\n",p->reg_cropy_s.reg_crop_y_start_x);
	fprintf(fp, "reg_crop_y_end_x = 0x%x\n",p->reg_cropy_s.reg_crop_y_end_x);
	fprintf(fp, "reg_crop_y_start_y = 0x%x\n",p->reg_cropy_e.reg_crop_y_start_y);
	fprintf(fp, "reg_crop_y_end_y = 0x%x\n",p->reg_cropy_e.reg_crop_y_end_y);
	fprintf(fp, "reg_crop_y_enable = 0x%x\n",p->reg_cropy_ctl.reg_crop_y_enable);
	fprintf(fp, "reg_crop_c_start_x = 0x%x\n",p->reg_cropc_s.reg_crop_c_start_x);
	fprintf(fp, "reg_crop_c_end_x = 0x%x\n",p->reg_cropc_s.reg_crop_c_end_x);
	fprintf(fp, "reg_crop_c_start_y = 0x%x\n",p->reg_cropc_e.reg_crop_c_start_y);
	fprintf(fp, "reg_crop_c_end_y = 0x%x\n",p->reg_cropc_e.reg_crop_c_end_y);
	fprintf(fp, "reg_crop_c_enable = 0x%x\n",p->reg_cropc_ctl.reg_crop_c_enable);
	fprintf(fp, "reg_crop_odma_start_x = 0x%x\n",p->reg_crop_odma_s.reg_crop_odma_start_x);
	fprintf(fp, "reg_crop_odma_end_x = 0x%x\n",p->reg_crop_odma_s.reg_crop_odma_end_x);
	fprintf(fp, "reg_crop_odma_start_y = 0x%x\n",p->reg_crop_odma_e.reg_crop_odma_start_y);
	fprintf(fp, "reg_crop_odma_end_y = 0x%x\n",p->reg_crop_odma_e.reg_crop_odma_end_y);
	fprintf(fp, "reg_crop_odma_enable = 0x%x\n",p->reg_crop_odma_ctl.reg_crop_odma_enable);

}
static void ive_filterop_print(ive_filterop_c* p) {
    fprintf(stderr, "ive_filterop\n");
	fprintf(stderr, "\tREG_1.reg_softrst = 0x%x\n", p->reg_1.reg_softrst);
	fprintf(stderr, "\tREG_1.reg_softrst_wdma_y = 0x%x\n", p->reg_1.reg_softrst_wdma_y);
	fprintf(stderr, "\tREG_1.reg_softrst_wdma_c = 0x%x\n", p->reg_1.reg_softrst_wdma_c);
	fprintf(stderr, "\tREG_1.reg_softrst_rdma_gradfg = 0x%x\n", p->reg_1.reg_softrst_rdma_gradfg);
	fprintf(stderr, "\tREG_1.reg_softrst_op1 = 0x%x\n", p->reg_1.reg_softrst_op1);
	fprintf(stderr, "\tREG_1.reg_softrst_filter3ch = 0x%x\n", p->reg_1.reg_softrst_filter3ch);
	fprintf(stderr, "\tREG_1.reg_softrst_st = 0x%x\n", p->reg_1.reg_softrst_st);
	fprintf(stderr, "\tREG_1.reg_softrst_odma = 0x%x\n", p->reg_1.reg_softrst_odma);
	fprintf(stderr, "\tREG_H04.reg_gradfg_bggrad_rdma_en = 0x%x\n", p->reg_h04.reg_gradfg_bggrad_rdma_en);
	fprintf(stderr, "\tREG_H04.reg_gradfg_bggrad_uv_swap = 0x%x\n", p->reg_h04.reg_gradfg_bggrad_uv_swap);
	fprintf(stderr, "\tREG_2.reg_shdw_sel = 0x%x\n", p->reg_2.reg_shdw_sel);
	fprintf(stderr, "\tREG_3.reg_ctrl_dmy1 = 0x%x\n", p->reg_3.reg_ctrl_dmy1);
	fprintf(stderr, "\treg_h10.reg_filterop_mode = 0x%x\n", p->reg_h10.reg_filterop_mode);
	fprintf(stderr, "\treg_h14.reg_filterop_op1_cmd = 0x%x\n", p->reg_h14.reg_filterop_op1_cmd);
	fprintf(stderr, "\treg_h14.reg_filterop_sw_ovw_op = 0x%x\n", p->reg_h14.reg_filterop_sw_ovw_op);
	fprintf(stderr, "\treg_h14.reg_filterop_3ch_en = 0x%x\n", p->reg_h14.reg_filterop_3ch_en);
	fprintf(stderr, "\treg_h14.reg_op_y_wdma_en = 0x%x\n", p->reg_h14.reg_op_y_wdma_en);
	fprintf(stderr, "\treg_h14.reg_op_c_wdma_en = 0x%x\n", p->reg_h14.reg_op_c_wdma_en);
	fprintf(stderr, "\treg_h14.reg_op_y_wdma_w1b_en = 0x%x\n", p->reg_h14.reg_op_y_wdma_w1b_en);
	fprintf(stderr, "\treg_h14.reg_op_c_wdma_w1b_en = 0x%x\n", p->reg_h14.reg_op_c_wdma_w1b_en);
	fprintf(stderr, "\tREG_4.reg_filterop_h_coef00 = 0x%x\n", p->reg_4.reg_filterop_h_coef00);
	fprintf(stderr, "\tREG_4.reg_filterop_h_coef01 = 0x%x\n", p->reg_4.reg_filterop_h_coef01);
	fprintf(stderr, "\tREG_4.reg_filterop_h_coef02 = 0x%x\n", p->reg_4.reg_filterop_h_coef02);
	fprintf(stderr, "\tREG_4.reg_filterop_h_coef03 = 0x%x\n", p->reg_4.reg_filterop_h_coef03);
	fprintf(stderr, "\tREG_5.reg_filterop_h_coef04 = 0x%x\n", p->reg_5.reg_filterop_h_coef04);
	fprintf(stderr, "\tREG_5.reg_filterop_h_coef10 = 0x%x\n", p->reg_5.reg_filterop_h_coef10);
	fprintf(stderr, "\tREG_5.reg_filterop_h_coef11 = 0x%x\n", p->reg_5.reg_filterop_h_coef11);
	fprintf(stderr, "\tREG_5.reg_filterop_h_coef12 = 0x%x\n", p->reg_5.reg_filterop_h_coef12);
	fprintf(stderr, "\tREG_6.reg_filterop_h_coef13 = 0x%x\n", p->reg_6.reg_filterop_h_coef13);
	fprintf(stderr, "\tREG_6.reg_filterop_h_coef14 = 0x%x\n", p->reg_6.reg_filterop_h_coef14);
	fprintf(stderr, "\tREG_6.reg_filterop_h_coef20 = 0x%x\n", p->reg_6.reg_filterop_h_coef20);
	fprintf(stderr, "\tREG_6.reg_filterop_h_coef21 = 0x%x\n", p->reg_6.reg_filterop_h_coef21);
	fprintf(stderr, "\tREG_7.reg_filterop_h_coef22 = 0x%x\n", p->reg_7.reg_filterop_h_coef22);
	fprintf(stderr, "\tREG_7.reg_filterop_h_coef23 = 0x%x\n", p->reg_7.reg_filterop_h_coef23);
	fprintf(stderr, "\tREG_7.reg_filterop_h_coef24 = 0x%x\n", p->reg_7.reg_filterop_h_coef24);
	fprintf(stderr, "\tREG_7.reg_filterop_h_coef30 = 0x%x\n", p->reg_7.reg_filterop_h_coef30);
	fprintf(stderr, "\tREG_8.reg_filterop_h_coef31 = 0x%x\n", p->reg_8.reg_filterop_h_coef31);
	fprintf(stderr, "\tREG_8.reg_filterop_h_coef32 = 0x%x\n", p->reg_8.reg_filterop_h_coef32);
	fprintf(stderr, "\tREG_8.reg_filterop_h_coef33 = 0x%x\n", p->reg_8.reg_filterop_h_coef33);
	fprintf(stderr, "\tREG_8.reg_filterop_h_coef34 = 0x%x\n", p->reg_8.reg_filterop_h_coef34);
	fprintf(stderr, "\tREG_9.reg_filterop_h_coef40 = 0x%x\n", p->reg_9.reg_filterop_h_coef40);
	fprintf(stderr, "\tREG_9.reg_filterop_h_coef41 = 0x%x\n", p->reg_9.reg_filterop_h_coef41);
	fprintf(stderr, "\tREG_9.reg_filterop_h_coef42 = 0x%x\n", p->reg_9.reg_filterop_h_coef42);
	fprintf(stderr, "\tREG_9.reg_filterop_h_coef43 = 0x%x\n", p->reg_9.reg_filterop_h_coef43);
	fprintf(stderr, "\tREG_10.reg_filterop_h_coef44 = 0x%x\n", p->reg_10.reg_filterop_h_coef44);
	fprintf(stderr, "\tREG_10.reg_filterop_h_norm = 0x%x\n", p->reg_10.reg_filterop_h_norm);
	fprintf(stderr, "\tREG_11.reg_filterop_v_coef00 = 0x%x\n", p->reg_11.reg_filterop_v_coef00);
	fprintf(stderr, "\tREG_11.reg_filterop_v_coef01 = 0x%x\n", p->reg_11.reg_filterop_v_coef01);
	fprintf(stderr, "\tREG_11.reg_filterop_v_coef02 = 0x%x\n", p->reg_11.reg_filterop_v_coef02);
	fprintf(stderr, "\tREG_11.reg_filterop_v_coef03 = 0x%x\n", p->reg_11.reg_filterop_v_coef03);
	fprintf(stderr, "\tREG_12.reg_filterop_v_coef04 = 0x%x\n", p->reg_12.reg_filterop_v_coef04);
	fprintf(stderr, "\tREG_12.reg_filterop_v_coef10 = 0x%x\n", p->reg_12.reg_filterop_v_coef10);
	fprintf(stderr, "\tREG_12.reg_filterop_v_coef11 = 0x%x\n", p->reg_12.reg_filterop_v_coef11);
	fprintf(stderr, "\tREG_12.reg_filterop_v_coef12 = 0x%x\n", p->reg_12.reg_filterop_v_coef12);
	fprintf(stderr, "\tREG_13.reg_filterop_v_coef13 = 0x%x\n", p->reg_13.reg_filterop_v_coef13);
	fprintf(stderr, "\tREG_13.reg_filterop_v_coef14 = 0x%x\n", p->reg_13.reg_filterop_v_coef14);
	fprintf(stderr, "\tREG_13.reg_filterop_v_coef20 = 0x%x\n", p->reg_13.reg_filterop_v_coef20);
	fprintf(stderr, "\tREG_13.reg_filterop_v_coef21 = 0x%x\n", p->reg_13.reg_filterop_v_coef21);
	fprintf(stderr, "\tREG_14.reg_filterop_v_coef22 = 0x%x\n", p->reg_14.reg_filterop_v_coef22);
	fprintf(stderr, "\tREG_14.reg_filterop_v_coef23 = 0x%x\n", p->reg_14.reg_filterop_v_coef23);
	fprintf(stderr, "\tREG_14.reg_filterop_v_coef24 = 0x%x\n", p->reg_14.reg_filterop_v_coef24);
	fprintf(stderr, "\tREG_14.reg_filterop_v_coef30 = 0x%x\n", p->reg_14.reg_filterop_v_coef30);
	fprintf(stderr, "\tREG_15.reg_filterop_v_coef31 = 0x%x\n", p->reg_15.reg_filterop_v_coef31);
	fprintf(stderr, "\tREG_15.reg_filterop_v_coef32 = 0x%x\n", p->reg_15.reg_filterop_v_coef32);
	fprintf(stderr, "\tREG_15.reg_filterop_v_coef33 = 0x%x\n", p->reg_15.reg_filterop_v_coef33);
	fprintf(stderr, "\tREG_15.reg_filterop_v_coef34 = 0x%x\n", p->reg_15.reg_filterop_v_coef34);
	fprintf(stderr, "\tREG_16.reg_filterop_v_coef40 = 0x%x\n", p->reg_16.reg_filterop_v_coef40);
	fprintf(stderr, "\tREG_16.reg_filterop_v_coef41 = 0x%x\n", p->reg_16.reg_filterop_v_coef41);
	fprintf(stderr, "\tREG_16.reg_filterop_v_coef42 = 0x%x\n", p->reg_16.reg_filterop_v_coef42);
	fprintf(stderr, "\tREG_16.reg_filterop_v_coef43 = 0x%x\n", p->reg_16.reg_filterop_v_coef43);
	fprintf(stderr, "\tREG_17.reg_filterop_v_coef44 = 0x%x\n", p->reg_17.reg_filterop_v_coef44);
	fprintf(stderr, "\tREG_17.reg_filterop_v_norm = 0x%x\n", p->reg_17.reg_filterop_v_norm);
	fprintf(stderr, "\tREG_18.reg_filterop_mode_trans = 0x%x\n", p->reg_18.reg_filterop_mode_trans);
	fprintf(stderr, "\tREG_18.reg_filterop_order_enmode = 0x%x\n", p->reg_18.reg_filterop_order_enmode);
	fprintf(stderr, "\tREG_18.reg_filterop_mag_thr = 0x%x\n", p->reg_18.reg_filterop_mag_thr);
	fprintf(stderr, "\tREG_19.reg_filterop_bernsen_win5x5_en = 0x%x\n", p->reg_19.reg_filterop_bernsen_win5x5_en);
	fprintf(stderr, "\tREG_19.reg_filterop_bernsen_mode = 0x%x\n", p->reg_19.reg_filterop_bernsen_mode);
	fprintf(stderr, "\tREG_19.reg_filterop_bernsen_thr = 0x%x\n", p->reg_19.reg_filterop_bernsen_thr);
	fprintf(stderr, "\tREG_19.reg_filterop_u8ContrastThreshold = 0x%x\n", p->reg_19.reg_filterop_u8ContrastThreshold);
	fprintf(stderr, "\tREG_20.reg_filterop_lbp_u8bit_thr = 0x%x\n", p->reg_20.reg_filterop_lbp_u8bit_thr);
	fprintf(stderr, "\tREG_20.reg_filterop_lbp_s8bit_thr = 0x%x\n", p->reg_20.reg_filterop_lbp_s8bit_thr);
	fprintf(stderr, "\tREG_20.reg_filterop_lbp_enmode = 0x%x\n", p->reg_20.reg_filterop_lbp_enmode);
	fprintf(stderr, "\tREG_21.reg_filterop_op2_erodila_coef00 = 0x%x\n", p->reg_21.reg_filterop_op2_erodila_coef00);
	fprintf(stderr, "\tREG_21.reg_filterop_op2_erodila_coef01 = 0x%x\n", p->reg_21.reg_filterop_op2_erodila_coef01);
	fprintf(stderr, "\tREG_21.reg_filterop_op2_erodila_coef02 = 0x%x\n", p->reg_21.reg_filterop_op2_erodila_coef02);
	fprintf(stderr, "\tREG_21.reg_filterop_op2_erodila_coef03 = 0x%x\n", p->reg_21.reg_filterop_op2_erodila_coef03);
	fprintf(stderr, "\tREG_22.reg_filterop_op2_erodila_coef04 = 0x%x\n", p->reg_22.reg_filterop_op2_erodila_coef04);
	fprintf(stderr, "\tREG_22.reg_filterop_op2_erodila_coef10 = 0x%x\n", p->reg_22.reg_filterop_op2_erodila_coef10);
	fprintf(stderr, "\tREG_22.reg_filterop_op2_erodila_coef11 = 0x%x\n", p->reg_22.reg_filterop_op2_erodila_coef11);
	fprintf(stderr, "\tREG_22.reg_filterop_op2_erodila_coef12 = 0x%x\n", p->reg_22.reg_filterop_op2_erodila_coef12);
	fprintf(stderr, "\tREG_23.reg_filterop_op2_erodila_coef13 = 0x%x\n", p->reg_23.reg_filterop_op2_erodila_coef13);
	fprintf(stderr, "\tREG_23.reg_filterop_op2_erodila_coef14 = 0x%x\n", p->reg_23.reg_filterop_op2_erodila_coef14);
	fprintf(stderr, "\tREG_23.reg_filterop_op2_erodila_coef20 = 0x%x\n", p->reg_23.reg_filterop_op2_erodila_coef20);
	fprintf(stderr, "\tREG_23.reg_filterop_op2_erodila_coef21 = 0x%x\n", p->reg_23.reg_filterop_op2_erodila_coef21);
	fprintf(stderr, "\tREG_24.reg_filterop_op2_erodila_coef22 = 0x%x\n", p->reg_24.reg_filterop_op2_erodila_coef22);
	fprintf(stderr, "\tREG_24.reg_filterop_op2_erodila_coef23 = 0x%x\n", p->reg_24.reg_filterop_op2_erodila_coef23);
	fprintf(stderr, "\tREG_24.reg_filterop_op2_erodila_coef24 = 0x%x\n", p->reg_24.reg_filterop_op2_erodila_coef24);
	fprintf(stderr, "\tREG_24.reg_filterop_op2_erodila_coef30 = 0x%x\n", p->reg_24.reg_filterop_op2_erodila_coef30);
	fprintf(stderr, "\tREG_25.reg_filterop_op2_erodila_coef31 = 0x%x\n", p->reg_25.reg_filterop_op2_erodila_coef31);
	fprintf(stderr, "\tREG_25.reg_filterop_op2_erodila_coef32 = 0x%x\n", p->reg_25.reg_filterop_op2_erodila_coef32);
	fprintf(stderr, "\tREG_25.reg_filterop_op2_erodila_coef33 = 0x%x\n", p->reg_25.reg_filterop_op2_erodila_coef33);
	fprintf(stderr, "\tREG_25.reg_filterop_op2_erodila_coef34 = 0x%x\n", p->reg_25.reg_filterop_op2_erodila_coef34);
	fprintf(stderr, "\tREG_26.reg_filterop_op2_erodila_coef40 = 0x%x\n", p->reg_26.reg_filterop_op2_erodila_coef40);
	fprintf(stderr, "\tREG_26.reg_filterop_op2_erodila_coef41 = 0x%x\n", p->reg_26.reg_filterop_op2_erodila_coef41);
	fprintf(stderr, "\tREG_26.reg_filterop_op2_erodila_coef42 = 0x%x\n", p->reg_26.reg_filterop_op2_erodila_coef42);
	fprintf(stderr, "\tREG_26.reg_filterop_op2_erodila_coef43 = 0x%x\n", p->reg_26.reg_filterop_op2_erodila_coef43);
	fprintf(stderr, "\tREG_27.reg_filterop_op2_erodila_coef44 = 0x%x\n", p->reg_27.reg_filterop_op2_erodila_coef44);
	fprintf(stderr, "\tREG_28.reg_filterop_op2_erodila_en = 0x%x\n", p->reg_28.reg_filterop_op2_erodila_en);
	fprintf(stderr, "\tREG_CSC_DBG_COEFF.reg_csc_dbg_en = 0x%x\n", p->REG_CSC_DBG_COEFF.reg_csc_dbg_en);
	fprintf(stderr, "\tREG_CSC_DBG_COEFF.reg_csc_dbg_coeff_sel = 0x%x\n", p->REG_CSC_DBG_COEFF.reg_csc_dbg_coeff_sel);
	fprintf(stderr, "\tREG_CSC_DBG_COEFF.reg_csc_dbg_coeff = 0x%x\n", p->REG_CSC_DBG_COEFF.reg_csc_dbg_coeff);
	fprintf(stderr, "\tREG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_x = 0x%x\n", p->REG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_x);
	fprintf(stderr, "\tREG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_y = 0x%x\n", p->REG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_y);
	fprintf(stderr, "\tREG_CSC_DBG_DATA_SRC.reg_csc_dbg_src_data = 0x%x\n", p->REG_CSC_DBG_DATA_SRC.reg_csc_dbg_src_data);
	fprintf(stderr, "\tREG_CSC_DBG_DATA_DAT.reg_csc_dbg_dst_data = 0x%x\n", p->REG_CSC_DBG_DATA_DAT.reg_csc_dbg_dst_data);
	fprintf(stderr, "\treg_33.reg_filterop_op2_gradfg_en = 0x%x\n", p->reg_33.reg_filterop_op2_gradfg_en);
	fprintf(stderr, "\treg_33.reg_filterop_op2_gradfg_softrst = 0x%x\n", p->reg_33.reg_filterop_op2_gradfg_softrst);
	fprintf(stderr, "\treg_33.reg_filterop_op2_gradfg_enmode = 0x%x\n", p->reg_33.reg_filterop_op2_gradfg_enmode);
	fprintf(stderr, "\treg_33.reg_filterop_op2_gradfg_edwdark = 0x%x\n", p->reg_33.reg_filterop_op2_gradfg_edwdark);
	fprintf(stderr, "\treg_33.reg_filterop_op2_gradfg_edwfactor = 0x%x\n", p->reg_33.reg_filterop_op2_gradfg_edwfactor);
	fprintf(stderr, "\treg_34.reg_filterop_op2_gradfg_crlcoefthr = 0x%x\n", p->reg_34.reg_filterop_op2_gradfg_crlcoefthr);
	fprintf(stderr, "\treg_34.reg_filterop_op2_gradfg_magcrlthr = 0x%x\n", p->reg_34.reg_filterop_op2_gradfg_magcrlthr);
	fprintf(stderr, "\treg_34.reg_filterop_op2_gradfg_minmagdiff = 0x%x\n", p->reg_34.reg_filterop_op2_gradfg_minmagdiff);
	fprintf(stderr, "\treg_34.reg_filterop_op2_gradfg_noiseval = 0x%x\n", p->reg_34.reg_filterop_op2_gradfg_noiseval);
	fprintf(stderr, "\treg_110.reg_filterop_map_enmode = 0x%x\n", p->reg_110.reg_filterop_map_enmode);
	fprintf(stderr, "\treg_110.reg_filterop_norm_out_ctrl = 0x%x\n", p->reg_110.reg_filterop_norm_out_ctrl);
	fprintf(stderr, "\treg_110.reg_filterop_magang_out_ctrl = 0x%x\n", p->reg_110.reg_filterop_magang_out_ctrl);
	fprintf(stderr, "\tODMA_REG_00.reg_dma_blen = 0x%x\n", p->odma_reg_00.reg_dma_blen);
	fprintf(stderr, "\tODMA_REG_00.reg_dma_en = 0x%x\n", p->odma_reg_00.reg_dma_en);
	fprintf(stderr, "\tODMA_REG_00.reg_fmt_sel = 0x%x\n", p->odma_reg_00.reg_fmt_sel);
	fprintf(stderr, "\tODMA_REG_00.reg_sc_odma_hflip = 0x%x\n", p->odma_reg_00.reg_sc_odma_hflip);
	fprintf(stderr, "\tODMA_REG_00.reg_sc_odma_vflip = 0x%x\n", p->odma_reg_00.reg_sc_odma_vflip);
	fprintf(stderr, "\tODMA_REG_00.reg_sc_422_avg = 0x%x\n", p->odma_reg_00.reg_sc_422_avg);
	fprintf(stderr, "\tODMA_REG_00.reg_sc_420_avg = 0x%x\n", p->odma_reg_00.reg_sc_420_avg);
	fprintf(stderr, "\tODMA_REG_00.reg_c_round_mode = 0x%x\n", p->odma_reg_00.reg_c_round_mode);
	fprintf(stderr, "\tODMA_REG_00.reg_bf16_en = 0x%x\n", p->odma_reg_00.reg_bf16_en);
	fprintf(stderr, "\tODMA_REG_01.reg_dma_y_base_low_part = 0x%x\n", p->odma_reg_01.reg_dma_y_base_low_part);
	fprintf(stderr, "\tODMA_REG_02.reg_dma_y_base_high_part = 0x%x\n", p->odma_reg_02.reg_dma_y_base_high_part);
	fprintf(stderr, "\tODMA_REG_03.reg_dma_u_base_low_part = 0x%x\n", p->odma_reg_03.reg_dma_u_base_low_part);
	fprintf(stderr, "\tODMA_REG_04.reg_dma_u_base_high_part = 0x%x\n", p->odma_reg_04.reg_dma_u_base_high_part);
	fprintf(stderr, "\tODMA_REG_05.reg_dma_v_base_low_part = 0x%x\n", p->odma_reg_05.reg_dma_v_base_low_part);
	fprintf(stderr, "\tODMA_REG_06.reg_dma_v_base_high_part = 0x%x\n", p->odma_reg_06.reg_dma_v_base_high_part);
	fprintf(stderr, "\tODMA_REG_07.reg_dma_y_pitch = 0x%x\n", p->odma_reg_07.reg_dma_y_pitch);
	fprintf(stderr, "\tODMA_REG_08.reg_dma_c_pitch = 0x%x\n", p->odma_reg_08.reg_dma_c_pitch);
	fprintf(stderr, "\tODMA_REG_09.reg_dma_x_str = 0x%x\n", p->odma_reg_09.reg_dma_x_str);
	fprintf(stderr, "\tODMA_REG_10.reg_dma_y_str = 0x%x\n", p->odma_reg_10.reg_dma_y_str);
	fprintf(stderr, "\tODMA_REG_11.reg_dma_wd = 0x%x\n", p->odma_reg_11.reg_dma_wd);
	fprintf(stderr, "\tODMA_REG_12.reg_dma_ht = 0x%x\n", p->odma_reg_12.reg_dma_ht);
	fprintf(stderr, "\tODMA_REG_13.reg_dma_debug = 0x%x\n", p->odma_reg_13.reg_dma_debug);
	fprintf(stderr, "\tODMA_REG_14.reg_dma_int_line_target = 0x%x\n", p->odma_reg_14.reg_dma_int_line_target);
	fprintf(stderr, "\tODMA_REG_14.reg_dma_int_line_target_sel = 0x%x\n", p->odma_reg_14.reg_dma_int_line_target_sel);
	fprintf(stderr, "\tODMA_REG_15.reg_dma_int_cycle_line_target = 0x%x\n", p->odma_reg_15.reg_dma_int_cycle_line_target);
	fprintf(stderr, "\tODMA_REG_15.reg_dma_int_cycle_line_target_sel = 0x%x\n", p->odma_reg_15.reg_dma_int_cycle_line_target_sel);
	fprintf(stderr, "\tODMA_REG_16.reg_dma_latch_line_cnt = 0x%x\n", p->odma_reg_16.reg_dma_latch_line_cnt);
	fprintf(stderr, "\tODMA_REG_16.reg_dma_latched_line_cnt = 0x%x\n", p->odma_reg_16.reg_dma_latched_line_cnt);
	fprintf(stderr, "\tODMA_REG_17.axi_active = 0x%x\n", p->odma_reg_17.axi_active);
	fprintf(stderr, "\tODMA_REG_17.axi_y_active = 0x%x\n", p->odma_reg_17.axi_y_active);
	fprintf(stderr, "\tODMA_REG_17.axi_u_active = 0x%x\n", p->odma_reg_17.axi_u_active);
	fprintf(stderr, "\tODMA_REG_17.axi_v_active = 0x%x\n", p->odma_reg_17.axi_v_active);
	fprintf(stderr, "\tODMA_REG_17.y_buf_full = 0x%x\n", p->odma_reg_17.y_buf_full);
	fprintf(stderr, "\tODMA_REG_17.y_buf_empty = 0x%x\n", p->odma_reg_17.y_buf_empty);
	fprintf(stderr, "\tODMA_REG_17.u_buf_full = 0x%x\n", p->odma_reg_17.u_buf_full);
	fprintf(stderr, "\tODMA_REG_17.u_buf_empty = 0x%x\n", p->odma_reg_17.u_buf_empty);
	fprintf(stderr, "\tODMA_REG_17.v_buf_full = 0x%x\n", p->odma_reg_17.v_buf_full);
	fprintf(stderr, "\tODMA_REG_17.v_buf_empty = 0x%x\n", p->odma_reg_17.v_buf_empty);
	fprintf(stderr, "\tODMA_REG_17.line_target_hit = 0x%x\n", p->odma_reg_17.line_target_hit);
	fprintf(stderr, "\tODMA_REG_17.cycle_line_target_hit = 0x%x\n", p->odma_reg_17.cycle_line_target_hit);
	fprintf(stderr, "\tODMA_REG_17.axi_cmd_cs = 0x%x\n", p->odma_reg_17.axi_cmd_cs);
	fprintf(stderr, "\tODMA_REG_17.y_line_cnt = 0x%x\n", p->odma_reg_17.y_line_cnt);
	fprintf(stderr, "\tREG_CANNY_0.reg_canny_lowthr = 0x%x\n", p->reg_canny_0.reg_canny_lowthr);
	fprintf(stderr, "\tREG_CANNY_0.reg_canny_hithr = 0x%x\n", p->reg_canny_0.reg_canny_hithr);
	fprintf(stderr, "\tREG_CANNY_1.reg_canny_en = 0x%x\n", p->reg_canny_1.reg_canny_en);
	fprintf(stderr, "\tREG_CANNY_1.reg_canny_strong_point_cnt_en = 0x%x\n", p->reg_canny_1.reg_canny_strong_point_cnt_en);
	fprintf(stderr, "\tREG_CANNY_1.reg_canny_non_or_weak = 0x%x\n", p->reg_canny_1.reg_canny_non_or_weak);
	fprintf(stderr, "\tREG_CANNY_1.reg_canny_strong_point_cnt = 0x%x\n", p->reg_canny_1.reg_canny_strong_point_cnt);
	fprintf(stderr, "\tREG_CANNY_2.reg_canny_eof = 0x%x\n", p->reg_canny_2.reg_canny_eof);
	fprintf(stderr, "\tREG_CANNY_3.reg_canny_basel = 0x%x\n", p->reg_canny_3.reg_canny_basel);
	fprintf(stderr, "\tREG_CANNY_4.reg_canny_baseh = 0x%x\n", p->reg_canny_4.reg_canny_baseh);
	fprintf(stderr, "\tREG_ST_CANDI_0.reg_st_candi_corner_bypass = 0x%x\n", p->reg_st_candi_0.reg_st_candi_corner_bypass);
	fprintf(stderr, "\tREG_ST_CANDI_0.reg_st_candi_corner_switch_src = 0x%x\n", p->reg_st_candi_0.reg_st_candi_corner_switch_src);
	fprintf(stderr, "\tREG_ST_EIGVAL_0.reg_st_eigval_max_eigval = 0x%x\n", p->reg_st_eigval_0.reg_st_eigval_max_eigval);
	fprintf(stderr, "\tREG_ST_EIGVAL_0.reg_st_eigval_tile_num = 0x%x\n", p->reg_st_eigval_0.reg_st_eigval_tile_num);
	fprintf(stderr, "\tREG_ST_EIGVAL_1.reg_sw_clr_max_eigval = 0x%x\n", p->reg_st_eigval_1.reg_sw_clr_max_eigval);
	fprintf(stderr, "\treg_h190.reg_filterop_op2_csc_tab_sw_0 = 0x%x\n", p->reg_h190.reg_filterop_op2_csc_tab_sw_0);
	fprintf(stderr, "\treg_h190.reg_filterop_op2_csc_tab_sw_1 = 0x%x\n", p->reg_h190.reg_filterop_op2_csc_tab_sw_1);
	fprintf(stderr, "\treg_h194.reg_filterop_op2_csc_tab_sw_update = 0x%x\n", p->reg_h194.reg_filterop_op2_csc_tab_sw_update);
	fprintf(stderr, "\treg_h194.reg_filterop_op2_csc_coeff_sw_update = 0x%x\n", p->reg_h194.reg_filterop_op2_csc_coeff_sw_update);
	fprintf(stderr, "\tREG_CSC_COEFF_0.reg_filterop_op2_csc_coeff_sw_00 = 0x%x\n", p->reg_csc_coeff_0.reg_filterop_op2_csc_coeff_sw_00);
	fprintf(stderr, "\tREG_CSC_COEFF_1.reg_filterop_op2_csc_coeff_sw_01 = 0x%x\n", p->reg_csc_coeff_1.reg_filterop_op2_csc_coeff_sw_01);
	fprintf(stderr, "\tREG_CSC_COEFF_2.reg_filterop_op2_csc_coeff_sw_02 = 0x%x\n", p->reg_csc_coeff_2.reg_filterop_op2_csc_coeff_sw_02);
	fprintf(stderr, "\tREG_CSC_COEFF_3.reg_filterop_op2_csc_coeff_sw_03 = 0x%x\n", p->reg_csc_coeff_3.reg_filterop_op2_csc_coeff_sw_03);
	fprintf(stderr, "\tREG_CSC_COEFF_4.reg_filterop_op2_csc_coeff_sw_04 = 0x%x\n", p->reg_csc_coeff_4.reg_filterop_op2_csc_coeff_sw_04);
	fprintf(stderr, "\tREG_CSC_COEFF_5.reg_filterop_op2_csc_coeff_sw_05 = 0x%x\n", p->reg_csc_coeff_5.reg_filterop_op2_csc_coeff_sw_05);
	fprintf(stderr, "\tREG_CSC_COEFF_6.reg_filterop_op2_csc_coeff_sw_06 = 0x%x\n", p->reg_csc_coeff_6.reg_filterop_op2_csc_coeff_sw_06);
	fprintf(stderr, "\tREG_CSC_COEFF_7.reg_filterop_op2_csc_coeff_sw_07 = 0x%x\n", p->reg_csc_coeff_7.reg_filterop_op2_csc_coeff_sw_07);
	fprintf(stderr, "\tREG_CSC_COEFF_8.reg_filterop_op2_csc_coeff_sw_08 = 0x%x\n", p->reg_csc_coeff_8.reg_filterop_op2_csc_coeff_sw_08);
	fprintf(stderr, "\tREG_CSC_COEFF_9.reg_filterop_op2_csc_coeff_sw_09 = 0x%x\n", p->reg_csc_coeff_9.reg_filterop_op2_csc_coeff_sw_09);
	fprintf(stderr, "\tREG_CSC_COEFF_A.reg_filterop_op2_csc_coeff_sw_10 = 0x%x\n", p->reg_csc_coeff_a.reg_filterop_op2_csc_coeff_sw_10);
	fprintf(stderr, "\tREG_CSC_COEFF_B.reg_filterop_op2_csc_coeff_sw_11 = 0x%x\n", p->reg_csc_coeff_b.reg_filterop_op2_csc_coeff_sw_11);
	fprintf(stderr, "\treg_h1c8.reg_filterop_op2_csc_enmode = 0x%x\n", p->reg_h1c8.reg_filterop_op2_csc_enmode);
	fprintf(stderr, "\treg_h1c8.reg_filterop_op2_csc_enable = 0x%x\n", p->reg_h1c8.reg_filterop_op2_csc_enable);
	fprintf(stderr, "\tREG_cropy_s.reg_crop_y_start_x = 0x%x\n", p->reg_cropy_s.reg_crop_y_start_x);
	fprintf(stderr, "\tREG_cropy_s.reg_crop_y_end_x = 0x%x\n", p->reg_cropy_s.reg_crop_y_end_x);
	fprintf(stderr, "\tREG_cropy_e.reg_crop_y_start_y = 0x%x\n", p->reg_cropy_e.reg_crop_y_start_y);
	fprintf(stderr, "\tREG_cropy_e.reg_crop_y_end_y = 0x%x\n", p->reg_cropy_e.reg_crop_y_end_y);
	fprintf(stderr, "\tREG_cropy_ctl.reg_crop_y_enable = 0x%x\n", p->reg_cropy_ctl.reg_crop_y_enable);
	fprintf(stderr, "\tREG_cropc_s.reg_crop_c_start_x = 0x%x\n", p->reg_cropc_s.reg_crop_c_start_x);
	fprintf(stderr, "\tREG_cropc_s.reg_crop_c_end_x = 0x%x\n", p->reg_cropc_s.reg_crop_c_end_x);
	fprintf(stderr, "\tREG_cropc_e.reg_crop_c_start_y = 0x%x\n", p->reg_cropc_e.reg_crop_c_start_y);
	fprintf(stderr, "\tREG_cropc_e.reg_crop_c_end_y = 0x%x\n", p->reg_cropc_e.reg_crop_c_end_y);
	fprintf(stderr, "\tREG_cropc_ctl.reg_crop_c_enable = 0x%x\n", p->reg_cropc_ctl.reg_crop_c_enable);
	fprintf(stderr, "\tREG_crop_odma_s.reg_crop_odma_start_x = 0x%x\n", p->reg_crop_odma_s.reg_crop_odma_start_x);
	fprintf(stderr, "\tREG_crop_odma_s.reg_crop_odma_end_x = 0x%x\n", p->reg_crop_odma_s.reg_crop_odma_end_x);
	fprintf(stderr, "\tREG_crop_odma_e.reg_crop_odma_start_y = 0x%x\n", p->reg_crop_odma_e.reg_crop_odma_start_y);
	fprintf(stderr, "\tREG_crop_odma_e.reg_crop_odma_end_y = 0x%x\n", p->reg_crop_odma_e.reg_crop_odma_end_y);
	fprintf(stderr, "\tREG_crop_odma_ctl.reg_crop_odma_enable = 0x%x\n", p->reg_crop_odma_ctl.reg_crop_odma_enable);

}
#pragma GCC diagnostic pop
#define DEFINE_IVE_FILTEROP_C(X) \
	 ive_filterop_c X = \
{\
	{	/* reg_1.reg_softrst = */0x0,\
	/*.reg_1.reg_softrst_wdma_y = */0x0,\
	/*.reg_1.reg_softrst_wdma_c = */0x0,\
	/*.reg_1.reg_softrst_rdma_gradfg = */0x0,\
	/*.reg_1.reg_softrst_op1 = */0x0,\
	/*.reg_1.reg_softrst_filter3ch = */0x0,\
	/*.reg_1.reg_softrst_st = */0x0,\
	/*.reg_1.reg_softrst_odma = */0x0,\
	},\
	{	/*.reg_h04.reg_gradfg_bggrad_rdma_en = */0x0,\
	/*.reg_h04.reg_gradfg_bggrad_uv_swap = */0x0,\
	},\
	{	/*.reg_2.reg_shdw_sel = */0x1,\
	},\
	{	/*.reg_3.reg_ctrl_dmy1 = */0x0,\
	},\
	{	/*.reg_h10.reg_filterop_mode = */0x2,\
	},\
	{	/*.reg_h14.reg_filterop_op1_cmd = */0x3,\
	/*.reg_h14.reg_filterop_sw_ovw_op = */0x0,\
	/*uint32_t rsv_5_7:3;=*/0,\
	/*.reg_h14.reg_filterop_3ch_en = */0x1,\
	/*.reg_h14.reg_op_y_wdma_en = */0x1,\
	/*.reg_h14.reg_op_c_wdma_en = */0x0,\
	/*.reg_h14.reg_op_y_wdma_w1b_en = */0x0,\
	/*.reg_h14.reg_op_c_wdma_w1b_en = */0x0,\
	},\
	{	/*.reg_4.reg_filterop_h_coef00 = */0x0,\
	/*.reg_4.reg_filterop_h_coef01 = */0x0,\
	/*.reg_4.reg_filterop_h_coef02 = */0x0,\
	/*.reg_4.reg_filterop_h_coef03 = */0x0,\
	},\
	{	/*.reg_5.reg_filterop_h_coef04 = */0x0,\
	/*.reg_5.reg_filterop_h_coef10 = */0x0,\
	/*.reg_5.reg_filterop_h_coef11 = */0x0,\
	/*.reg_5.reg_filterop_h_coef12 = */0x0,\
	},\
	{	/*.reg_6.reg_filterop_h_coef13 = */0x0,\
	/*.reg_6.reg_filterop_h_coef14 = */0x0,\
	/*.reg_6.reg_filterop_h_coef20 = */0x0,\
	/*.reg_6.reg_filterop_h_coef21 = */0x0,\
	},\
	{	/*.reg_7.reg_filterop_h_coef22 = */0x0,\
	/*.reg_7.reg_filterop_h_coef23 = */0x0,\
	/*.reg_7.reg_filterop_h_coef24 = */0x0,\
	/*.reg_7.reg_filterop_h_coef30 = */0x0,\
	},\
	{	/*.reg_8.reg_filterop_h_coef31 = */0x0,\
	/*.reg_8.reg_filterop_h_coef32 = */0x0,\
	/*.reg_8.reg_filterop_h_coef33 = */0x0,\
	/*.reg_8.reg_filterop_h_coef34 = */0x0,\
	},\
	{	/*.reg_9.reg_filterop_h_coef40 = */0x0,\
	/*.reg_9.reg_filterop_h_coef41 = */0x0,\
	/*.reg_9.reg_filterop_h_coef42 = */0x0,\
	/*.reg_9.reg_filterop_h_coef43 = */0x0,\
	},\
	{	/*.reg_10.reg_filterop_h_coef44 = */0x0,\
	/*.reg_10.reg_filterop_h_norm = */0x0,\
	},\
	{	/*.reg_11.reg_filterop_v_coef00 = */0x0,\
	/*.reg_11.reg_filterop_v_coef01 = */0x0,\
	/*.reg_11.reg_filterop_v_coef02 = */0x0,\
	/*.reg_11.reg_filterop_v_coef03 = */0x0,\
	},\
	{	/*.reg_12.reg_filterop_v_coef04 = */0x0,\
	/*.reg_12.reg_filterop_v_coef10 = */0x0,\
	/*.reg_12.reg_filterop_v_coef11 = */0x0,\
	/*.reg_12.reg_filterop_v_coef12 = */0x0,\
	},\
	{	/*.reg_13.reg_filterop_v_coef13 = */0x0,\
	/*.reg_13.reg_filterop_v_coef14 = */0x0,\
	/*.reg_13.reg_filterop_v_coef20 = */0x0,\
	/*.reg_13.reg_filterop_v_coef21 = */0x0,\
	},\
	{	/*.reg_14.reg_filterop_v_coef22 = */0x0,\
	/*.reg_14.reg_filterop_v_coef23 = */0x0,\
	/*.reg_14.reg_filterop_v_coef24 = */0x0,\
	/*.reg_14.reg_filterop_v_coef30 = */0x0,\
	},\
	{	/*.reg_15.reg_filterop_v_coef31 = */0x0,\
	/*.reg_15.reg_filterop_v_coef32 = */0x0,\
	/*.reg_15.reg_filterop_v_coef33 = */0x0,\
	/*.reg_15.reg_filterop_v_coef34 = */0x0,\
	},\
	{	/*.reg_16.reg_filterop_v_coef40 = */0x0,\
	/*.reg_16.reg_filterop_v_coef41 = */0x0,\
	/*.reg_16.reg_filterop_v_coef42 = */0x0,\
	/*.reg_16.reg_filterop_v_coef43 = */0x0,\
	},\
	{	/*.reg_17.reg_filterop_v_coef44 = */0x0,\
	/*.reg_17.reg_filterop_v_norm = */0x0,\
	},\
	{	/*.reg_18.reg_filterop_mode_trans = */0x1,\
	/*uint32_t rsv_1_3:3;=*/0,\
	/*.reg_18.reg_filterop_order_enmode = */0x1,\
	/*uint32_t rsv_7_15:9;=*/0,\
	/*.reg_18.reg_filterop_mag_thr = */0x0,\
	},\
	{	/*.reg_19.reg_filterop_bernsen_win5x5_en = */0x1,\
	/*uint32_t rsv_1_3:3;=*/0,\
	/*.reg_19.reg_filterop_bernsen_mode = */0x0,\
	/*uint32_t rsv_6_7:2;=*/0,\
	/*.reg_19.reg_filterop_bernsen_thr = */0x0,\
	/*.reg_19.reg_filterop_u8ContrastThreshold = */0x0,\
	},\
	{	/*.reg_20.reg_filterop_lbp_u8bit_thr = */0x0,\
	/*.reg_20.reg_filterop_lbp_s8bit_thr = */0x0,\
	/*.reg_20.reg_filterop_lbp_enmode = */0x0,\
	},\
	{	/*.reg_21.reg_filterop_op2_erodila_coef00 = */0x0,\
	/*.reg_21.reg_filterop_op2_erodila_coef01 = */0x0,\
	/*.reg_21.reg_filterop_op2_erodila_coef02 = */0x0,\
	/*.reg_21.reg_filterop_op2_erodila_coef03 = */0x0,\
	},\
	{	/*.reg_22.reg_filterop_op2_erodila_coef04 = */0x0,\
	/*.reg_22.reg_filterop_op2_erodila_coef10 = */0x0,\
	/*.reg_22.reg_filterop_op2_erodila_coef11 = */0x0,\
	/*.reg_22.reg_filterop_op2_erodila_coef12 = */0x0,\
	},\
	{	/*.reg_23.reg_filterop_op2_erodila_coef13 = */0x0,\
	/*.reg_23.reg_filterop_op2_erodila_coef14 = */0x0,\
	/*.reg_23.reg_filterop_op2_erodila_coef20 = */0x0,\
	/*.reg_23.reg_filterop_op2_erodila_coef21 = */0x0,\
	},\
	{	/*.reg_24.reg_filterop_op2_erodila_coef22 = */0x0,\
	/*.reg_24.reg_filterop_op2_erodila_coef23 = */0x0,\
	/*.reg_24.reg_filterop_op2_erodila_coef24 = */0x0,\
	/*.reg_24.reg_filterop_op2_erodila_coef30 = */0x0,\
	},\
	{	/*.reg_25.reg_filterop_op2_erodila_coef31 = */0x0,\
	/*.reg_25.reg_filterop_op2_erodila_coef32 = */0x0,\
	/*.reg_25.reg_filterop_op2_erodila_coef33 = */0x0,\
	/*.reg_25.reg_filterop_op2_erodila_coef34 = */0x0,\
	},\
	{	/*.reg_26.reg_filterop_op2_erodila_coef40 = */0x0,\
	/*.reg_26.reg_filterop_op2_erodila_coef41 = */0x0,\
	/*.reg_26.reg_filterop_op2_erodila_coef42 = */0x0,\
	/*.reg_26.reg_filterop_op2_erodila_coef43 = */0x0,\
	},\
	{	/*.reg_27.reg_filterop_op2_erodila_coef44 = */0x0,\
	},\
	{	/*.reg_28.reg_filterop_op2_erodila_en = */0x0,\
	},\
	{	/*.REG_CSC_DBG_COEFF.reg_csc_dbg_en = */0x0,\
	/*uint32_t rsv_1_3:3;=*/0,\
	/*.REG_CSC_DBG_COEFF.reg_csc_dbg_coeff_sel = */0x0,\
	/*.REG_CSC_DBG_COEFF.reg_csc_dbg_coeff = */0x0,\
	},\
	{	/*.REG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_x = */0x0,\
	/*uint32_t rsv_12_15:4;=*/0,\
	/*.REG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_y = */0x0,\
	},\
	{	/*.REG_CSC_DBG_DATA_SRC.reg_csc_dbg_src_data = */0x0,\
	},\
	{	/*.REG_CSC_DBG_DATA_DAT.reg_csc_dbg_dst_data = */0x0,\
	},\
	{	/*.reg_33.reg_filterop_op2_gradfg_en = */0x0,\
	/*.reg_33.reg_filterop_op2_gradfg_softrst = */0x0,\
	/*.reg_33.reg_filterop_op2_gradfg_enmode = */0x1,\
	/*.reg_33.reg_filterop_op2_gradfg_edwdark = */0x1,\
	/*uint32_t rsv_4_15:12;=*/0,\
	/*.reg_33.reg_filterop_op2_gradfg_edwfactor = */0x3e8,\
	},\
	{	/*.reg_34.reg_filterop_op2_gradfg_crlcoefthr = */0x50,\
	/*.reg_34.reg_filterop_op2_gradfg_magcrlthr = */0x4,\
	/*.reg_34.reg_filterop_op2_gradfg_minmagdiff = */0x2,\
	/*.reg_34.reg_filterop_op2_gradfg_noiseval = */0x1,\
	},\
	{	/*.reg_110.reg_filterop_map_enmode = */0x0,\
	/*uint32_t rsv_2_3:2;=*/0,\
	/*.reg_110.reg_filterop_norm_out_ctrl = */0x0,\
	/*uint32_t rsv_6_7:2;=*/0,\
	/*.reg_110.reg_filterop_magang_out_ctrl = */0x0,\
	},\
	{	/*.odma_reg_00.reg_dma_blen = */0x0,\
	/*.odma_reg_00.reg_dma_en = */0x0,\
	/*uint32_t rsv_2_7:6;=*/0,\
	/*.odma_reg_00.reg_fmt_sel = */0x0,\
	/*uint32_t rsv_12_15:4;=*/0,\
	/*.odma_reg_00.reg_sc_odma_hflip = */0x0,\
	/*.odma_reg_00.reg_sc_odma_vflip = */0x0,\
	/*uint32_t rsv_18_19:2;=*/0,\
	/*.odma_reg_00.reg_sc_422_avg = */0x0,\
	/*.odma_reg_00.reg_sc_420_avg = */0x0,\
	/*.odma_reg_00.reg_c_round_mode = */0x0,\
	/*.odma_reg_00.reg_bf16_en = */0x0,\
	},\
	{	/*.odma_reg_01.reg_dma_y_base_low_part = */0x0,\
	},\
	{	/*.odma_reg_02.reg_dma_y_base_high_part = */0x0,\
	},\
	{	/*.odma_reg_03.reg_dma_u_base_low_part = */0x0,\
	},\
	{	/*.odma_reg_04.reg_dma_u_base_high_part = */0x0,\
	},\
	{	/*.odma_reg_05.reg_dma_v_base_low_part = */0x0,\
	},\
	{	/*.odma_reg_06.reg_dma_v_base_high_part = */0x0,\
	},\
	{	/*.odma_reg_07.reg_dma_y_pitch = */0x0,\
	},\
	{	/*.odma_reg_08.reg_dma_c_pitch = */0x0,\
	},\
	{	/*.odma_reg_09.reg_dma_x_str = */0x0,\
	},\
	{	/*.odma_reg_10.reg_dma_y_str = */0x0,\
	},\
	{	/*.odma_reg_11.reg_dma_wd = */0x0,\
	},\
	{	/*.odma_reg_12.reg_dma_ht = */0x0,\
	},\
	{	/*.odma_reg_13.reg_dma_debug = */0x0,\
	},\
	{	/*.odma_reg_14.reg_dma_int_line_target = */0x0,\
	/*uint32_t rsv_12_15:4;=*/0,\
	/*.odma_reg_14.reg_dma_int_line_target_sel = */0x0,\
	},\
	{	/*.odma_reg_15.reg_dma_int_cycle_line_target = */0x0,\
	/*uint32_t rsv_11_15:5;=*/0,\
	/*.odma_reg_15.reg_dma_int_cycle_line_target_sel = */0x0,\
	},\
	{	/*.odma_reg_16.reg_dma_latch_line_cnt = */0x0,\
	/*uint32_t rsv_1_7:7;=*/0,\
	/*.odma_reg_16.reg_dma_latched_line_cnt = */0x0,\
	},\
	{	/*.odma_reg_17.axi_active = */0x0,\
	/*.odma_reg_17.axi_y_active = */0x0,\
	/*.odma_reg_17.axi_u_active = */0x0,\
	/*.odma_reg_17.axi_v_active = */0x0,\
	/*.odma_reg_17.y_buf_full = */0x0,\
	/*.odma_reg_17.y_buf_empty = */0x0,\
	/*.odma_reg_17.u_buf_full = */0x0,\
	/*.odma_reg_17.u_buf_empty = */0x0,\
	/*.odma_reg_17.v_buf_full = */0x0,\
	/*.odma_reg_17.v_buf_empty = */0x0,\
	/*.odma_reg_17.line_target_hit = */0x0,\
	/*.odma_reg_17.cycle_line_target_hit = */0x0,\
	/*.odma_reg_17.axi_cmd_cs = */0x0,\
	/*.odma_reg_17.y_line_cnt = */0x0,\
	},\
	{	/*.reg_canny_0.reg_canny_lowthr = */0x0,\
	/*.reg_canny_0.reg_canny_hithr = */0x0,\
	},\
	{	/*.reg_canny_1.reg_canny_en = */0x0,\
	/*.reg_canny_1.reg_canny_strong_point_cnt_en = */0x0,\
	/*.reg_canny_1.reg_canny_non_or_weak = */0x0,\
	/*uint32_t rsv_3_15:13;=*/0,\
	/*.reg_canny_1.reg_canny_strong_point_cnt = */0x0,\
	},\
	{	/*.reg_canny_2.reg_canny_eof = */0xFFFFFFFF,\
	},\
	{	/*.reg_canny_3.reg_canny_basel = */0x0,\
	},\
	{	/*.reg_canny_4.reg_canny_baseh = */0x0,\
	},\
	{	/*.reg_st_candi_0.reg_st_candi_corner_bypass = */0x0,\
	/*.reg_st_candi_0.reg_st_candi_corner_switch_src = */0x0,\
	},\
	{	/*.reg_st_eigval_0.reg_st_eigval_max_eigval = */0x0,\
	/*.reg_st_eigval_0.reg_st_eigval_tile_num = */0x0,\
	},\
	{	/*.reg_st_eigval_1.reg_sw_clr_max_eigval = */0x0,\
	},\
	{	/*.reg_h190.reg_filterop_op2_csc_tab_sw_0 = */0x0,\
	/*uint32_t rsv_12_15:4;=*/0,\
	/*.reg_h190.reg_filterop_op2_csc_tab_sw_1 = */0x0,\
	},\
	{	/*.reg_h194.reg_filterop_op2_csc_tab_sw_update = */0x0,\
	/*uint32_t rsv_1_15:15;=*/0,\
	/*.reg_h194.reg_filterop_op2_csc_coeff_sw_update = */0x0,\
	},\
	{	/*.reg_csc_coeff_0.reg_filterop_op2_csc_coeff_sw_00 = */0x0,\
	},\
	{	/*.reg_csc_coeff_1.reg_filterop_op2_csc_coeff_sw_01 = */0x0,\
	},\
	{	/*.reg_csc_coeff_2.reg_filterop_op2_csc_coeff_sw_02 = */0x0,\
	},\
	{	/*.reg_csc_coeff_3.reg_filterop_op2_csc_coeff_sw_03 = */0x0,\
	},\
	{	/*.reg_csc_coeff_4.reg_filterop_op2_csc_coeff_sw_04 = */0x0,\
	},\
	{	/*.reg_csc_coeff_5.reg_filterop_op2_csc_coeff_sw_05 = */0x0,\
	},\
	{	/*.reg_csc_coeff_6.reg_filterop_op2_csc_coeff_sw_06 = */0x0,\
	},\
	{	/*.reg_csc_coeff_7.reg_filterop_op2_csc_coeff_sw_07 = */0x0,\
	},\
	{	/*.reg_csc_coeff_8.reg_filterop_op2_csc_coeff_sw_08 = */0x0,\
	},\
	{	/*.reg_csc_coeff_9.reg_filterop_op2_csc_coeff_sw_09 = */0x0,\
	},\
	{	/*.reg_csc_coeff_a.reg_filterop_op2_csc_coeff_sw_10 = */0x0,\
	},\
	{	/*.reg_csc_coeff_b.reg_filterop_op2_csc_coeff_sw_11 = */0x0,\
	},\
	{	/*.reg_h1c8.reg_filterop_op2_csc_enmode = */0x0,\
	/*.reg_h1c8.reg_filterop_op2_csc_enable = */0x0,\
	},\
	{	/*.reg_cropy_s.reg_crop_y_start_x = */0x0,\
	/*.reg_cropy_s.reg_crop_y_end_x = */0x0,\
	},\
	{	/*.reg_cropy_e.reg_crop_y_start_y = */0x0,\
	/*.reg_cropy_e.reg_crop_y_end_y = */0x0,\
	},\
	{	/*.reg_cropy_ctl.reg_crop_y_enable = */0x0,\
	},\
	{	/*.reg_cropc_s.reg_crop_c_start_x = */0x0,\
	/*.reg_cropc_s.reg_crop_c_end_x = */0x0,\
	},\
	{	/*.reg_cropc_e.reg_crop_c_start_y = */0x0,\
	/*.reg_cropc_e.reg_crop_c_end_y = */0x0,\
	},\
	{	/*.reg_cropc_ctl.reg_crop_c_enable = */0x0,\
	},\
	{	/*.reg_crop_odma_s.reg_crop_odma_start_x = */0x0,\
	/*.reg_crop_odma_s.reg_crop_odma_end_x = */0x0,\
	},\
	{	/*.reg_crop_odma_e.reg_crop_odma_start_y = */0x0,\
	/*.reg_crop_odma_e.reg_crop_odma_end_y = */0x0,\
	},\
	{	/*.reg_crop_odma_ctl.reg_crop_odma_enable = */0x0,\
	}\
};
#else /* !ifdef __cplusplus */
#define _DEFINE_IVE_FILTEROP_C \
{\
	.reg_1.reg_softrst = 0x0,\
	.reg_1.reg_softrst_wdma_y = 0x0,\
	.reg_1.reg_softrst_wdma_c = 0x0,\
	.reg_1.reg_softrst_rdma_gradfg = 0x0,\
	.reg_1.reg_softrst_op1 = 0x0,\
	.reg_1.reg_softrst_filter3ch = 0x0,\
	.reg_1.reg_softrst_st = 0x0,\
	.reg_1.reg_softrst_odma = 0x0,\
	.reg_h04.reg_gradfg_bggrad_rdma_en = 0x0,\
	.reg_h04.reg_gradfg_bggrad_uv_swap = 0x0,\
	.reg_2.reg_shdw_sel = 0x1,\
	.reg_3.reg_ctrl_dmy1 = 0x0,\
	.reg_h10.reg_filterop_mode = 0x2,\
	.reg_h14.reg_filterop_op1_cmd = 0x3,\
	.reg_h14.reg_filterop_sw_ovw_op = 0x0,\
	.reg_h14.reg_filterop_3ch_en = 0x1,\
	.reg_h14.reg_op_y_wdma_en = 0x1,\
	.reg_h14.reg_op_c_wdma_en = 0x0,\
	.reg_h14.reg_op_y_wdma_w1b_en = 0x0,\
	.reg_h14.reg_op_c_wdma_w1b_en = 0x0,\
	.reg_4.reg_filterop_h_coef00 = 0x0,\
	.reg_4.reg_filterop_h_coef01 = 0x0,\
	.reg_4.reg_filterop_h_coef02 = 0x0,\
	.reg_4.reg_filterop_h_coef03 = 0x0,\
	.reg_5.reg_filterop_h_coef04 = 0x0,\
	.reg_5.reg_filterop_h_coef10 = 0x0,\
	.reg_5.reg_filterop_h_coef11 = 0x0,\
	.reg_5.reg_filterop_h_coef12 = 0x0,\
	.reg_6.reg_filterop_h_coef13 = 0x0,\
	.reg_6.reg_filterop_h_coef14 = 0x0,\
	.reg_6.reg_filterop_h_coef20 = 0x0,\
	.reg_6.reg_filterop_h_coef21 = 0x0,\
	.reg_7.reg_filterop_h_coef22 = 0x0,\
	.reg_7.reg_filterop_h_coef23 = 0x0,\
	.reg_7.reg_filterop_h_coef24 = 0x0,\
	.reg_7.reg_filterop_h_coef30 = 0x0,\
	.reg_8.reg_filterop_h_coef31 = 0x0,\
	.reg_8.reg_filterop_h_coef32 = 0x0,\
	.reg_8.reg_filterop_h_coef33 = 0x0,\
	.reg_8.reg_filterop_h_coef34 = 0x0,\
	.reg_9.reg_filterop_h_coef40 = 0x0,\
	.reg_9.reg_filterop_h_coef41 = 0x0,\
	.reg_9.reg_filterop_h_coef42 = 0x0,\
	.reg_9.reg_filterop_h_coef43 = 0x0,\
	.reg_10.reg_filterop_h_coef44 = 0x0,\
	.reg_10.reg_filterop_h_norm = 0x0,\
	.reg_11.reg_filterop_v_coef00 = 0x0,\
	.reg_11.reg_filterop_v_coef01 = 0x0,\
	.reg_11.reg_filterop_v_coef02 = 0x0,\
	.reg_11.reg_filterop_v_coef03 = 0x0,\
	.reg_12.reg_filterop_v_coef04 = 0x0,\
	.reg_12.reg_filterop_v_coef10 = 0x0,\
	.reg_12.reg_filterop_v_coef11 = 0x0,\
	.reg_12.reg_filterop_v_coef12 = 0x0,\
	.reg_13.reg_filterop_v_coef13 = 0x0,\
	.reg_13.reg_filterop_v_coef14 = 0x0,\
	.reg_13.reg_filterop_v_coef20 = 0x0,\
	.reg_13.reg_filterop_v_coef21 = 0x0,\
	.reg_14.reg_filterop_v_coef22 = 0x0,\
	.reg_14.reg_filterop_v_coef23 = 0x0,\
	.reg_14.reg_filterop_v_coef24 = 0x0,\
	.reg_14.reg_filterop_v_coef30 = 0x0,\
	.reg_15.reg_filterop_v_coef31 = 0x0,\
	.reg_15.reg_filterop_v_coef32 = 0x0,\
	.reg_15.reg_filterop_v_coef33 = 0x0,\
	.reg_15.reg_filterop_v_coef34 = 0x0,\
	.reg_16.reg_filterop_v_coef40 = 0x0,\
	.reg_16.reg_filterop_v_coef41 = 0x0,\
	.reg_16.reg_filterop_v_coef42 = 0x0,\
	.reg_16.reg_filterop_v_coef43 = 0x0,\
	.reg_17.reg_filterop_v_coef44 = 0x0,\
	.reg_17.reg_filterop_v_norm = 0x0,\
	.reg_18.reg_filterop_mode_trans = 0x1,\
	.reg_18.reg_filterop_order_enmode = 0x1,\
	.reg_18.reg_filterop_mag_thr = 0x0,\
	.reg_19.reg_filterop_bernsen_win5x5_en = 0x1,\
	.reg_19.reg_filterop_bernsen_mode = 0x0,\
	.reg_19.reg_filterop_bernsen_thr = 0x0,\
	.reg_19.reg_filterop_u8ContrastThreshold = 0x0,\
	.reg_20.reg_filterop_lbp_u8bit_thr = 0x0,\
	.reg_20.reg_filterop_lbp_s8bit_thr = 0x0,\
	.reg_20.reg_filterop_lbp_enmode = 0x0,\
	.reg_21.reg_filterop_op2_erodila_coef00 = 0x0,\
	.reg_21.reg_filterop_op2_erodila_coef01 = 0x0,\
	.reg_21.reg_filterop_op2_erodila_coef02 = 0x0,\
	.reg_21.reg_filterop_op2_erodila_coef03 = 0x0,\
	.reg_22.reg_filterop_op2_erodila_coef04 = 0x0,\
	.reg_22.reg_filterop_op2_erodila_coef10 = 0x0,\
	.reg_22.reg_filterop_op2_erodila_coef11 = 0x0,\
	.reg_22.reg_filterop_op2_erodila_coef12 = 0x0,\
	.reg_23.reg_filterop_op2_erodila_coef13 = 0x0,\
	.reg_23.reg_filterop_op2_erodila_coef14 = 0x0,\
	.reg_23.reg_filterop_op2_erodila_coef20 = 0x0,\
	.reg_23.reg_filterop_op2_erodila_coef21 = 0x0,\
	.reg_24.reg_filterop_op2_erodila_coef22 = 0x0,\
	.reg_24.reg_filterop_op2_erodila_coef23 = 0x0,\
	.reg_24.reg_filterop_op2_erodila_coef24 = 0x0,\
	.reg_24.reg_filterop_op2_erodila_coef30 = 0x0,\
	.reg_25.reg_filterop_op2_erodila_coef31 = 0x0,\
	.reg_25.reg_filterop_op2_erodila_coef32 = 0x0,\
	.reg_25.reg_filterop_op2_erodila_coef33 = 0x0,\
	.reg_25.reg_filterop_op2_erodila_coef34 = 0x0,\
	.reg_26.reg_filterop_op2_erodila_coef40 = 0x0,\
	.reg_26.reg_filterop_op2_erodila_coef41 = 0x0,\
	.reg_26.reg_filterop_op2_erodila_coef42 = 0x0,\
	.reg_26.reg_filterop_op2_erodila_coef43 = 0x0,\
	.reg_27.reg_filterop_op2_erodila_coef44 = 0x0,\
	.reg_28.reg_filterop_op2_erodila_en = 0x0,\
	.REG_CSC_DBG_COEFF.reg_csc_dbg_en = 0x0,\
	.REG_CSC_DBG_COEFF.reg_csc_dbg_coeff_sel = 0x0,\
	.REG_CSC_DBG_COEFF.reg_csc_dbg_coeff = 0x0,\
	.REG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_x = 0x0,\
	.REG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_y = 0x0,\
	.REG_CSC_DBG_DATA_SRC.reg_csc_dbg_src_data = 0x0,\
	.REG_CSC_DBG_DATA_DAT.reg_csc_dbg_dst_data = 0x0,\
	.reg_33.reg_filterop_op2_gradfg_en = 0x0,\
	.reg_33.reg_filterop_op2_gradfg_softrst = 0x0,\
	.reg_33.reg_filterop_op2_gradfg_enmode = 0x1,\
	.reg_33.reg_filterop_op2_gradfg_edwdark = 0x1,\
	.reg_33.reg_filterop_op2_gradfg_edwfactor = 0x3e8,\
	.reg_34.reg_filterop_op2_gradfg_crlcoefthr = 0x50,\
	.reg_34.reg_filterop_op2_gradfg_magcrlthr = 0x4,\
	.reg_34.reg_filterop_op2_gradfg_minmagdiff = 0x2,\
	.reg_34.reg_filterop_op2_gradfg_noiseval = 0x1,\
	.reg_110.reg_filterop_map_enmode = 0x0,\
	.reg_110.reg_filterop_norm_out_ctrl = 0x0,\
	.reg_110.reg_filterop_magang_out_ctrl = 0x0,\
	.odma_reg_00.reg_dma_blen = 0x0,\
	.odma_reg_00.reg_dma_en = 0x0,\
	.odma_reg_00.reg_fmt_sel = 0x0,\
	.odma_reg_00.reg_sc_odma_hflip = 0x0,\
	.odma_reg_00.reg_sc_odma_vflip = 0x0,\
	.odma_reg_00.reg_sc_422_avg = 0x0,\
	.odma_reg_00.reg_sc_420_avg = 0x0,\
	.odma_reg_00.reg_c_round_mode = 0x0,\
	.odma_reg_00.reg_bf16_en = 0x0,\
	.odma_reg_01.reg_dma_y_base_low_part = 0x0,\
	.odma_reg_02.reg_dma_y_base_high_part = 0x0,\
	.odma_reg_03.reg_dma_u_base_low_part = 0x0,\
	.odma_reg_04.reg_dma_u_base_high_part = 0x0,\
	.odma_reg_05.reg_dma_v_base_low_part = 0x0,\
	.odma_reg_06.reg_dma_v_base_high_part = 0x0,\
	.odma_reg_07.reg_dma_y_pitch = 0x0,\
	.odma_reg_08.reg_dma_c_pitch = 0x0,\
	.odma_reg_09.reg_dma_x_str = 0x0,\
	.odma_reg_10.reg_dma_y_str = 0x0,\
	.odma_reg_11.reg_dma_wd = 0x0,\
	.odma_reg_12.reg_dma_ht = 0x0,\
	.odma_reg_13.reg_dma_debug = 0x0,\
	.odma_reg_14.reg_dma_int_line_target = 0x0,\
	.odma_reg_14.reg_dma_int_line_target_sel = 0x0,\
	.odma_reg_15.reg_dma_int_cycle_line_target = 0x0,\
	.odma_reg_15.reg_dma_int_cycle_line_target_sel = 0x0,\
	.odma_reg_16.reg_dma_latch_line_cnt = 0x0,\
	.odma_reg_16.reg_dma_latched_line_cnt = 0x0,\
	.odma_reg_17.axi_active = 0x0,\
	.odma_reg_17.axi_y_active = 0x0,\
	.odma_reg_17.axi_u_active = 0x0,\
	.odma_reg_17.axi_v_active = 0x0,\
	.odma_reg_17.y_buf_full = 0x0,\
	.odma_reg_17.y_buf_empty = 0x0,\
	.odma_reg_17.u_buf_full = 0x0,\
	.odma_reg_17.u_buf_empty = 0x0,\
	.odma_reg_17.v_buf_full = 0x0,\
	.odma_reg_17.v_buf_empty = 0x0,\
	.odma_reg_17.line_target_hit = 0x0,\
	.odma_reg_17.cycle_line_target_hit = 0x0,\
	.odma_reg_17.axi_cmd_cs = 0x0,\
	.odma_reg_17.y_line_cnt = 0x0,\
	.reg_canny_0.reg_canny_lowthr = 0x0,\
	.reg_canny_0.reg_canny_hithr = 0x0,\
	.reg_canny_1.reg_canny_en = 0x0,\
	.reg_canny_1.reg_canny_strong_point_cnt_en = 0x0,\
	.reg_canny_1.reg_canny_non_or_weak = 0x0,\
	.reg_canny_1.reg_canny_strong_point_cnt = 0x0,\
	.reg_canny_2.reg_canny_eof = 0xFFFFFFFF,\
	.reg_canny_3.reg_canny_basel = 0x0,\
	.reg_canny_4.reg_canny_baseh = 0x0,\
	.reg_st_candi_0.reg_st_candi_corner_bypass = 0x0,\
	.reg_st_candi_0.reg_st_candi_corner_switch_src = 0x0,\
	.reg_st_eigval_0.reg_st_eigval_max_eigval = 0x0,\
	.reg_st_eigval_0.reg_st_eigval_tile_num = 0x0,\
	.reg_st_eigval_1.reg_sw_clr_max_eigval = 0x0,\
	.reg_h190.reg_filterop_op2_csc_tab_sw_0 = 0x0,\
	.reg_h190.reg_filterop_op2_csc_tab_sw_1 = 0x0,\
	.reg_h194.reg_filterop_op2_csc_tab_sw_update = 0x0,\
	.reg_h194.reg_filterop_op2_csc_coeff_sw_update = 0x0,\
	.reg_csc_coeff_0.reg_filterop_op2_csc_coeff_sw_00 = 0x0,\
	.reg_csc_coeff_1.reg_filterop_op2_csc_coeff_sw_01 = 0x0,\
	.reg_csc_coeff_2.reg_filterop_op2_csc_coeff_sw_02 = 0x0,\
	.reg_csc_coeff_3.reg_filterop_op2_csc_coeff_sw_03 = 0x0,\
	.reg_csc_coeff_4.reg_filterop_op2_csc_coeff_sw_04 = 0x0,\
	.reg_csc_coeff_5.reg_filterop_op2_csc_coeff_sw_05 = 0x0,\
	.reg_csc_coeff_6.reg_filterop_op2_csc_coeff_sw_06 = 0x0,\
	.reg_csc_coeff_7.reg_filterop_op2_csc_coeff_sw_07 = 0x0,\
	.reg_csc_coeff_8.reg_filterop_op2_csc_coeff_sw_08 = 0x0,\
	.reg_csc_coeff_9.reg_filterop_op2_csc_coeff_sw_09 = 0x0,\
	.reg_csc_coeff_a.reg_filterop_op2_csc_coeff_sw_10 = 0x0,\
	.reg_csc_coeff_b.reg_filterop_op2_csc_coeff_sw_11 = 0x0,\
	.reg_h1c8.reg_filterop_op2_csc_enmode = 0x0,\
	.reg_h1c8.reg_filterop_op2_csc_enable = 0x0,\
	.reg_cropy_s.reg_crop_y_start_x = 0x0,\
	.reg_cropy_s.reg_crop_y_end_x = 0x0,\
	.reg_cropy_e.reg_crop_y_start_y = 0x0,\
	.reg_cropy_e.reg_crop_y_end_y = 0x0,\
	.reg_cropy_ctl.reg_crop_y_enable = 0x0,\
	.reg_cropc_s.reg_crop_c_start_x = 0x0,\
	.reg_cropc_s.reg_crop_c_end_x = 0x0,\
	.reg_cropc_e.reg_crop_c_start_y = 0x0,\
	.reg_cropc_e.reg_crop_c_end_y = 0x0,\
	.reg_cropc_ctl.reg_crop_c_enable = 0x0,\
	.reg_crop_odma_s.reg_crop_odma_start_x = 0x0,\
	.reg_crop_odma_s.reg_crop_odma_end_x = 0x0,\
	.reg_crop_odma_e.reg_crop_odma_start_y = 0x0,\
	.reg_crop_odma_e.reg_crop_odma_end_y = 0x0,\
	.reg_crop_odma_ctl.reg_crop_odma_enable = 0x0,\
}
#endif /* ifdef __cplusplus */
#endif //__REG_IVE_FILTEROP_STRUCT_H__
