//*****************************************************************************
//  All Winner Micro, All Right Reserved. 2006-2011 Copyright (c)
//
//  File name   :        de_scal_bsp.h
//
//  Description :  display engine scaler registers and interface functions define
//                 for aw1623
//  History     :
//                2011/05/03      zchmin       v0.1    Initial version
//******************************************************************************
#ifndef __DE_FE_H__
#define __DE_FE_H__

#include "de_be.h"

//macro define
#define SCALINITPASELMT (0xfffff)
#define SCALLINEMAX (2048)

// 
// Detail information of registers
//

typedef union
{
	u32 dwval;
	struct
	{
		u32 en                 :  1 ;    // Default: 0x0; 
		u32 res0               : 31 ;    // Default: ; 
	} bits;
} DEFE_EN_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 reg_rdy_en         :  1 ;    // Default: 0x0; 
		u32 coef_rdy_en        :  1 ;    // Default: 0x0; 
		u32 wb_en              :  1 ;    // Default: 0x0; 
		u32 res0               :  5 ;    // Default: ; 
		u32 out_port_sel       :  2 ;    // Default: 0x0; 
		u32 res1               :  1 ;    // Default: ; 
		u32 out_ctrl           :  1 ;    // Default: 0x0; 
		u32 in_ctrl            :  3 ;    // Default: 0x0; 
		u32 res2               :  1 ;    // Default: ; 
		u32 frm_start          :  1 ;    // Default: 0x0; 
		u32 res3               :  6 ;    // Default: ; 
		u32 coef_access_ctrl   :  1 ;    //fir coef ram access control, for  single buffer
		u32 res4               :  8 ;    // Default: ; 
	} bits;
} DEFE_FRM_CTRL_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 res0               :  1 ;    // Default: ; 
		u32 csc_bypass_en      :  1 ;    // Default: 0x0; 
		u32 res1               : 28 ;    // Default: ; 
		u32 sram_map_sel       :  2 ;    // Default: 0x0; 
	} bits;
} DEFE_BYPASS_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 res0               :  8 ;    // Default: ; 
		u32 linebuf_agth       :  1 ;    // Default: 0x0; 
		u32 res1               : 23 ;    // Default: ; 
	} bits;
} DEFE_AGTH_SEL_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 trig_line          : 13 ;    // Default: 0x0; 
		u32 res0               :  2 ;    // Default: ; 
		u32 field_sel          :  1 ;    // Default: 0x0; 
		u32 current_line       : 12 ;    // Default: 0x0; 
		u32 res1               :  4 ;    // Default: ; 
	} bits;
} DEFE_LINT_CTRL_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 prefrm_addr             ;    // Default: 0x0; 
	} bits;
} DEFE_3D_PRELUMA_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 buf_addr                ;    // Default: 0x0; 
	} bits;
} DEFE_BUF_ADDR0_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 buf_addr                ;    // Default: 0x0; 
	} bits;
} DEFE_BUF_ADDR1_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 buf_addr                ;    // Default: 0x0; 
	} bits;
} DEFE_BUF_ADDR2_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 field_cnt          :  8 ;    // Default: 0x0; 
		u32 valid_field_cnt    :  3 ;    // Default: 0x0; 
		u32 res0               :  1 ;    // Default: ; 
		u32 field_loop_mod     :  1 ;    // Default: 0x0; 
		u32 res1               : 11 ;    // Default: ; 
		u32 fir_offset         :  6 ;    // Default: 0x20; 
		u32 res2               :  2 ;    // Default: ; 
	} bits;
} DEFE_FIELD_CTRL_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 x_offset0          :  5 ;    // Default: 0x0; 
		u32 res0               :  3 ;    // Default: ; 
		u32 y_offset0          :  5 ;    // Default: 0x0; 
		u32 res1               :  3 ;    // Default: ; 
		u32 x_offset1          :  5 ;    // Default: 0x0; 
		u32 res2               : 11 ;    // Default: ; 
	} bits;
} DEFE_TB_OFF0_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 x_offset0          :  5 ;    // Default: 0x0; 
		u32 res0               :  3 ;    // Default: ; 
		u32 y_offset0          :  5 ;    // Default: 0x0; 
		u32 res1               :  3 ;    // Default: ; 
		u32 x_offset1          :  5 ;    // Default: 0x0; 
		u32 res2               : 11 ;    // Default: ; 
	} bits;
} DEFE_TB_OFF1_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 x_offset0          :  5 ;    // Default: 0x0; 
		u32 res0               :  3 ;    // Default: ; 
		u32 y_offset0          :  5 ;    // Default: 0x0; 
		u32 res1               :  3 ;    // Default: ; 
		u32 x_offset1          :  5 ;    // Default: 0x0; 
		u32 res2               : 11 ;    // Default: ; 
	} bits;
} DEFE_TB_OFF2_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 prefrm_addr             ;    // Default: 0x0; 
	} bits;
} DEFE_3D_PRECHROMA_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 line_stride             ;    // Default: 0x0; 
	} bits;
} DEFE_LINESTRD0_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 line_stride             ;    // Default: 0x0; 
	} bits;
} DEFE_LINESTRD1_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 line_stride             ;    // Default: 0x0; 
	} bits;
} DEFE_LINESTRD2_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 data_ps            :  2 ;    // Default: 0x0; 
		u32 res0               :  2 ;    // Default: ; 
		u32 data_fmt           :  3 ;    // Default: 0x0; 
		u32 res1               :  1 ;    // Default: ; 
		u32 data_mod           :  3 ;    // Default: 0x0; 
		u32 res2               :  1 ;    // Default: ; 
		u32 scan_mod           :  1 ;    // Default: 0x0; 
		u32 res3               :  1 ;    // Default: ; 
		u32 a_coef_sel         :  1 ;    // Default: 0x0; 
		u32 res4               :  1 ;    // Default: ; 
		u32 byte_seq           :  1 ;    // Default: 0x0; 
		u32 res5               : 15 ;    // Default: ; 
	} bits;
} DEFE_INPUT_FMT_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 wb_addr                 ;    // Default: 0x0; 
	} bits;
} DEFE_WB_ADDR0_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 wb_addr                 ;    // Default: 0x0; 
	} bits;
} DEFE_WB_ADDR1_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 wb_addr                 ;    // Default: 0x0; 
	} bits;
} DEFE_WB_ADDR2_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 data_fmt           :  3 ;    // Default: 0x0; 
		u32 res0               :  1 ;    // Default: ; 
		u32 scan_mod           :  1 ;    // Default: 0x0; 
		u32 res1               :  2 ;    // Default: ; 
		u32 alpha_en           :  1 ;    // Default: 0x0; 
		u32 byte_seq           :  1 ;    // Default: 0x0; 
		u32 res2               : 23 ;    // Default: ; 
	} bits;
} DEFE_OUTPUT_FMT_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 res0               :  7 ;    // Default: ; 
		u32 wb_en              :  1 ;    // Default: 0x0; 
		u32 res1               :  1 ;    // Default: ; 
		u32 line_en            :  1 ;    // Default: 0x0; 
		u32 reg_load_en        :  1 ;    // Default: 0x0; 
		u32 res2               : 21 ;    // Default: ; 
	} bits;
} DEFE_INT_EN_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 res0               :  7 ;    // Default: ; 
		u32 wb_status          :  1 ;    // Default: 0x0; 
		u32 res1               :  1 ;    // Default: ; 
		u32 line_status        :  1 ;    // Default: 0x0; 
		u32 reg_load_status    :  1 ;    // Default: 0x0; 
		u32 res2               : 21 ;    // Default: ; 
	} bits;
} DEFE_INT_STATUS_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 frm_busy           :  1 ;    // Default: 0x0; 
		u32 wb_status          :  1 ;    // Default: 0x0; 
		u32 cfg_pending        :  1 ;    // Default: 0x0; 
		u32 res0               :  1 ;    // Default: ; 
		u32 dram_status        :  1 ;    // Default: 0x0; 
		u32 lcd_field          :  1 ;    // Default: 0x0; 
		u32 res1               :  5 ;    // Default: ; 
		u32 coef_access_status :  1 ;    //fir coef access status, for single buffer
		u32 wb_err_status      :  1 ;    // Default: 0x0; 
		u32 res2               :  1 ;    // Default: ; 
		u32 wb_err_losedata    :  1 ;    // Default: 0x0; 
		u32 wb_err_sync        :  1 ;    // Default: 0x0; 
		u32 line_on_sync       : 13 ;    // Default: 0x0; 
		u32 res3               :  3 ;    // Default: ; 
	} bits;
} DEFE_STATUS_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 coef               : 13 ;    // Default: 0x0; 
		u32 res0               : 19 ;    // Default: ; 
	} bits;
} DEFE_CSC_COEF00_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 coef               : 13 ;    // Default: 0x0; 
		u32 res0               : 19 ;    // Default: ; 
	} bits;
} DEFE_CSC_COEF01_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 coef               : 13 ;    // Default: 0x0; 
		u32 res0               : 19 ;    // Default: ; 
	} bits;
} DEFE_CSC_COEF02_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 cont               : 14 ;    // Default: 0x0; 
		u32 res0               : 18 ;    // Default: ; 
	} bits;
} DEFE_CSC_COEF03_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 coef               : 13 ;    // Default: 0x0; 
		u32 res0               : 19 ;    // Default: ; 
	} bits;
} DEFE_CSC_COEF10_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 coef               : 13 ;    // Default: 0x0; 
		u32 res0               : 19 ;    // Default: ; 
	} bits;
} DEFE_CSC_COEF11_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 coef               : 13 ;    // Default: 0x0; 
		u32 res0               : 19 ;    // Default: ; 
	} bits;
} DEFE_CSC_COEF12_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 cont               : 14 ;    // Default: 0x0; 
		u32 res0               : 18 ;    // Default: ; 
	} bits;
} DEFE_CSC_COEF13_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 coef               : 13 ;    // Default: 0x0; 
		u32 res0               : 19 ;    // Default: ; 
	} bits;
} DEFE_CSC_COEF20_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 coef               : 13 ;    // Default: 0x0; 
		u32 res0               : 19 ;    // Default: ; 
	} bits;
} DEFE_CSC_COEF21_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 coef               : 13 ;    // Default: 0x0; 
		u32 res0               : 19 ;    // Default: ; 
	} bits;
} DEFE_CSC_COEF22_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 cont               : 14 ;    // Default: 0x0; 
		u32 res0               : 18 ;    // Default: ; 
	} bits;
} DEFE_CSC_COEF23_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 en                 :  1 ;    // Default: 0x0; 
		u32 res0               :  7 ;    // Default: ; 
		u32 flag_out_en        :  1 ;    // Default: 0x0; 
		u32 res1               :  7 ;    // Default: ; 
		u32 mod                :  2 ;    // Default: 0x0; 
		u32 res2               :  6 ;    // Default: ; 
		u32 diagintp_en        :  1 ;    // Default: 0x0; 
		u32 tempdiff_en        :  1 ;    // Default: 0x0; 
		u32 res3               :  6 ;    // Default: ; 
	} bits;
} DEFE_DI_CTRL_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 th0                :  7 ;    // Default: 0x28; 
		u32 res0               :  1 ;    // Default: ; 
		u32 th1                :  7 ;    // Default: 0x5; 
		u32 res1               :  9 ;    // Default: ; 
		u32 th3                :  8 ;    // Default: 0x8; 
	} bits;
} DEFE_DI_DIAGINTP_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 sad_central_th     :  7 ;    // Default: 0xA; 
		u32 res0               :  1 ;    // Default: ; 
		u32 ambiguity_th       :  7 ;    // Default: 0xA; 
		u32 res1               :  1 ;    // Default: ; 
		u32 direct_dither_th   : 11 ;    // Default: 0xF; 
		u32 res2               :  5 ;    // Default: ; 
	} bits;
} DEFE_DI_TEMPDIFF_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 minlumath          :  8 ;    // Default: 0x9; 
		u32 maxlumath          :  8 ;    // Default: 0x10; 
		u32 avglumashifter     :  8 ;    // Default: 0x6; 
		u32 pixel_static_th    :  2 ;    // Default: 0x3; 
		u32 res0               :  6 ;    // Default: ; 
	} bits;
} DEFE_DI_LUMA_TH_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 th2                :  8 ;    // Default: 0xA; 
		u32 res0               :  8 ;    // Default: ; 
		u32 th3                :  8 ;    // Default: 0x14; 
		u32 res1               :  8 ;    // Default: ; 
	} bits;
} DEFE_DI_SPATCOMP_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 chroma_diff_th     :  8 ;    // Default: 0x5; 
		u32 res0               :  8 ;    // Default: ; 
		u32 luma               :  6 ;    // Default: 0x1F; 
		u32 res1               :  2 ;    // Default: ; 
		u32 chroma             :  6 ;    // Default: 0x1F; 
		u32 res2               :  2 ;    // Default: ; 
	} bits;
} DEFE_DI_CHROMADIFF_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 prefrm_addr             ;    // Default: 0x0; 
	} bits;
} DEFE_DI_PRELUMA_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 prefrm_addr             ;    // Default: 0x0; 
	} bits;
} DEFE_DI_PRECHROMA_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 tile_flag_addr0         ;    // Default: 0x0; 
	} bits;
} DEFE_DI_TILEFLAG0_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 tile_flag_addr1         ;    // Default: 0x0; 
	} bits;
} DEFE_DI_TILEFLAG1_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 tile_flag_linestrd      ;    // Default: 0x200; 
	} bits;
} DEFE_DI_FLAGLINESTRD_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 field_cnt          :  8 ;    // Default: 0x0;
		u32 valid_field_cnt    :  3 ;    // Default: 0x0;
		u32 res0               :  1 ;
		u32 field_loop_mod     :  1 ;    // Default: 0x0;
		u32 res1               :  19;    
	} bits;
} DEFE_DI_FLAG_SEQ_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 en                 :  1 ;    // Default: 0x0; 
		u32 res0               : 31 ;    // Default: ; 
	} bits;
} DEFE_WB_LINESTRD_EN_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 line_strd               ;    // Default: 0x0; 
	} bits;
} DEFE_WB_LINESTRD0_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 line_strd               ;    // Default: 0x0; 
	} bits;
} DEFE_WB_LINESTRD1_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 line_strd               ;    // Default: 0x0; 
	} bits;
} DEFE_WB_LINESTRD2_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 mod_sel            :  2 ;    // Default: 0x0; 
		u32 res0               :  6 ;    // Default: ; 
		u32 ci_out_en          :  1 ;    // Default: 0x0; 
		u32 res1               :  1 ;    // Default: ; 
		u32 ss_out_en          :  1 ;    // Default: 0x0; 
		u32 li_in_en           :  1 ;    // Default: 0x0; 
		u32 tb_out_scan_mod    :  1 ;    // Default: 0x0; 
		u32 res2               :  3 ;    // Default: ; 
		u32 ci_out_mod         :  3 ;    // Default: 0x0; 
		u32 res3               :  5 ;    // Default: ; 
		u32 tb_out_mod_field   :  2 ;    // Default: 0x0; 
		u32 res4               :  6 ;    // Default: ; 
	} bits;
} DEFE_3D_CTRL_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 right_ch0_addr          ;    // Default: 0x0; 
	} bits;
} DEFE_3D_BUF_ADDR0_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 right_ch1_addr          ;    // Default: 0x0; 
	} bits;
} DEFE_3D_BUF_ADDR1_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 right_ch2_addr          ;    // Default: 0x0; 
	} bits;
} DEFE_3D_BUF_ADDR2_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 x_offset0          :  5 ;    // Default: 0x0; 
		u32 res0               :  3 ;    // Default: ; 
		u32 y_offset0          :  5 ;    // Default: 0x0; 
		u32 res1               :  3 ;    // Default: ; 
		u32 x_offset1          :  5 ;    // Default: 0x0; 
		u32 res2               : 11 ;    // Default: ; 
	} bits;
} DEFE_3D_TB_OFF0_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 x_offset0          :  5 ;    // Default: 0x0; 
		u32 res0               :  3 ;    // Default: ; 
		u32 y_offset0          :  5 ;    // Default: 0x0; 
		u32 res1               :  3 ;    // Default: ; 
		u32 x_offset1          :  5 ;    // Default: 0x0; 
		u32 res2               : 11 ;    // Default: ; 
	} bits;
} DEFE_3D_TB_OFF1_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 x_offset0          :  5 ;    // Default: 0x0; 
		u32 res0               :  3 ;    // Default: ; 
		u32 y_offset0          :  5 ;    // Default: 0x0; 
		u32 res1               :  3 ;    // Default: ; 
		u32 x_offset1          :  5 ;    // Default: 0x0; 
		u32 res2               : 11 ;    // Default: ; 
	} bits;
} DEFE_3D_TB_OFF2_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 wb_stride          :  5 ;    // Default: 0x0; 
	} bits;
} DEFE_3D_WB_STRD_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 in_width           : 13 ;    // Default: 0x0; 
		u32 res0               :  3 ;    // Default: ; 
		u32 in_height          : 13 ;    // Default: 0x0; 
		u32 res1               :  3 ;    // Default: ; 
	} bits;
} DEFE_CH0_INSIZE_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 out_width          : 13 ;    // Default: 0x0; 
		u32 res0               :  3 ;    // Default: ; 
		u32 out_height         : 13 ;    // Default: 0x0; 
		u32 res1               :  3 ;    // Default: ; 
	} bits;
} DEFE_CH0_OUTSIZE_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 factor_frac        : 16 ;    // Default: 0x0; 
		u32 factor_int         :  8 ;    // Default: 0x0; 
		u32 res0               :  8 ;    // Default: ; 
	} bits;
} DEFE_CH0_HORZFACT_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 factor_frac        : 16 ;    // Default: 0x0; 
		u32 factor_int         :  8 ;    // Default: 0x0; 
		u32 res0               :  8 ;    // Default: ; 
	} bits;
} DEFE_CH0_VERTFACT_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 phase              : 20 ;    // Default: 0x0; 
		u32 res0               : 12 ;    // Default: ; 
	} bits;
} DEFE_CH0_HORZPHASE_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 phase              : 20 ;    // Default: 0x0; 
		u32 res0               : 12 ;    // Default: ; 
	} bits;
} DEFE_CH0_VERTPHASE0_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 phase              : 20 ;    // Default: 0x0; 
		u32 res0               : 12 ;    // Default: ; 
	} bits;
} DEFE_CH0_VERTPHASE1_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 tap0               :  7 ;    // Default: 0x7D; 
		u32 res0               :  1 ;    // Default: ; 
		u32 tap1               :  7 ;    // Default: 0x1; 
		u32 res1               :  1 ;    // Default: ; 
		u32 tap2               :  7 ;    // Default: 0x1; 
		u32 res2               :  1 ;    // Default: ; 
		u32 tap3               :  7 ;    // Default: 0x1; 
		u32 res3               :  1 ;    // Default: ; 
	} bits;
} DEFE_CH0_HORZTAP0_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 tap4               :  7 ;    // Default: 0x1; 
		u32 res0               :  1 ;    // Default: ; 
		u32 tap5               :  7 ;    // Default: 0x1; 
		u32 res1               :  1 ;    // Default: ; 
		u32 tap6               :  7 ;    // Default: 0x1; 
		u32 res2               :  1 ;    // Default: ; 
		u32 tap7               :  7 ;    // Default: 0x1; 
		u32 res3               :  1 ;    // Default: ; 
	} bits;
} DEFE_CH0_HORZTAP1_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 tap0               :  7 ;    // Default: 0x7F; 
		u32 res0               :  1 ;    // Default: ; 
		u32 tap1               :  7 ;    // Default: 0x1; 
		u32 res1               :  1 ;    // Default: ; 
		u32 tap2               :  7 ;    // Default: 0x1; 
		u32 res2               :  1 ;    // Default: ; 
		u32 tap3               :  7 ;    // Default: 0x1; 
		u32 res3               :  1 ;    // Default: ; 
	} bits;
} DEFE_CH0_VERTTAP_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 in_width           : 13 ;    // Default: 0x0; 
		u32 res0               :  3 ;    // Default: ; 
		u32 in_height          : 13 ;    // Default: 0x0; 
		u32 res1               :  3 ;    // Default: ; 
	} bits;
} DEFE_CH1_INSIZE_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 out_width          : 13 ;    // Default: 0x0; 
		u32 res0               :  3 ;    // Default: ; 
		u32 out_height         : 13 ;    // Default: 0x0; 
		u32 res1               :  3 ;    // Default: ; 
	} bits;
} DEFE_CH1_OUTSIZE_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 factor_frac        : 16 ;    // Default: 0x0; 
		u32 factor_int         :  8 ;    // Default: 0x0; 
		u32 res0               :  8 ;    // Default: ; 
	} bits;
} DEFE_CH1_HORZFACT_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 factor_frac        : 16 ;    // Default: 0x0; 
		u32 factor_int         :  8 ;    // Default: 0x0; 
		u32 res0               :  8 ;    // Default: ; 
	} bits;
} DEFE_CH1_VERTFACT_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 phase              : 20 ;    // Default: 0x0; 
		u32 res0               : 12 ;    // Default: ; 
	} bits;
} DEFE_CH1_HORZPHASE_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 phase              : 20 ;    // Default: 0x0; 
		u32 res0               : 12 ;    // Default: ; 
	} bits;
} DEFE_CH1_VERTPHASE0_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 phase              : 20 ;    // Default: 0x0; 
		u32 res0               : 12 ;    // Default: ; 
	} bits;
} DEFE_CH1_VERTPHASE1_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 tap0               :  7 ;    // Default: 0x7D; 
		u32 res0               :  1 ;    // Default: ; 
		u32 tap1               :  7 ;    // Default: 0x1; 
		u32 res1               :  1 ;    // Default: ; 
		u32 tap2               :  7 ;    // Default: 0x1; 
		u32 res2               :  1 ;    // Default: ; 
		u32 tap3               :  7 ;    // Default: 0x1; 
		u32 res3               :  1 ;    // Default: ; 
	} bits;
} DEFE_CH1_HORZTAP0_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 tap4               :  7 ;    // Default: 0x1; 
		u32 res0               :  1 ;    // Default: ; 
		u32 tap5               :  7 ;    // Default: 0x1; 
		u32 res1               :  1 ;    // Default: ; 
		u32 tap6               :  7 ;    // Default: 0x1; 
		u32 res2               :  1 ;    // Default: ; 
		u32 tap7               :  7 ;    // Default: 0x1; 
		u32 res3               :  1 ;    // Default: ; 
	} bits;
} DEFE_CH1_HORZTAP1_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 tap0               :  7 ;    // Default: 0x7F; 
		u32 res0               :  1 ;    // Default: ; 
		u32 tap1               :  7 ;    // Default: 0x1; 
		u32 res1               :  1 ;    // Default: ; 
		u32 tap2               :  7 ;    // Default: 0x1; 
		u32 res2               :  1 ;    // Default: ; 
		u32 tap3               :  7 ;    // Default: 0x1; 
		u32 res3               :  1 ;    // Default: ; 
	} bits;
} DEFE_CH1_VERTTAP_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 tap0                         :  8 ;      
		u32 tap1                         :  8 ;
		u32 tap2                         :  8 ; 
		u32 tap3                         :  8 ;
	} bits;
} DEFE_CH0_HORZCOEF0_REGN_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 tap4                         :  8 ;   
		u32 tap5                         :  8 ;    
		u32 tap6                         :  8 ;     
		u32 tap7                         :  8 ;    
	} bits;
} DEFE_CH0_HORZCOEF1_REGN_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 tap0                         :  8 ; 
		u32 tap1                         :  8 ;
		u32 tap2                         :  8 ; 
		u32 tap3                         :  8 ;
	} bits;
} DEFE_CH0_VERTCOEF_REGN_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 res0               :  32 ;    // 
	} bits;
} DEFE_RESEVED_REG_t;


//device define
typedef struct __DE_SCAL_DEV
{
	DEFE_EN_REG_t              		modl_en            ;//0x00
	DEFE_FRM_CTRL_REG_t        		frm_ctrl           ;//0x04
	DEFE_BYPASS_REG_t          		bypass             ;//0x08
	DEFE_AGTH_SEL_REG_t        		agth_sel           ;//0x0c
	DEFE_LINT_CTRL_REG_t       		lint_ctrl          ;//0x10
	DEFE_RESEVED_REG_t              res0x14[2]         ;//0x14~0x1b
	DEFE_3D_PRELUMA_REG_t           trd_preluma        ;//0x1c	
	DEFE_BUF_ADDR0_REG_t       		buf_addr0          ;//0x20
	DEFE_BUF_ADDR1_REG_t       		buf_addr1          ;//0x24
	DEFE_BUF_ADDR2_REG_t       		buf_addr2          ;//0x28
	DEFE_FIELD_CTRL_REG_t      		field_ctrl         ;//0x2c
	DEFE_TB_OFF0_REG_t         		tb_off0            ;//0x30
	DEFE_TB_OFF1_REG_t         		tb_off1            ;//0x34
	DEFE_TB_OFF2_REG_t         		tb_off2            ;//0x38
	DEFE_3D_PRECHROMA_REG_t         trd_prechroma      ;//0x3c
	DEFE_LINESTRD0_REG_t       		linestrd0          ;//0x40
	DEFE_LINESTRD1_REG_t       		linestrd1          ;//0x44
	DEFE_LINESTRD2_REG_t       		linestrd2          ;//0x48
	DEFE_INPUT_FMT_REG_t       		input_fmt          ;//0x4c
	DEFE_WB_ADDR0_REG_t        		wb_addr0           ;//0x50
	DEFE_WB_ADDR1_REG_t        		wb_addr1           ;//0x54
	DEFE_WB_ADDR2_REG_t        		wb_addr2           ;//0x58
	DEFE_OUTPUT_FMT_REG_t      		output_fmt         ;//0x5c
	DEFE_INT_EN_REG_t          		int_en             ;//0x60
	DEFE_INT_STATUS_REG_t      		int_status         ;//0x64
	DEFE_STATUS_REG_t          		status             ;//0x68
	DEFE_RESEVED_REG_t              res0x6c            ;//0x6c
	DEFE_CSC_COEF03_REG_t           csc_coef[12]       ;//0x70~0x9f
	DEFE_DI_CTRL_REG_t         		di_ctrl            ;//0xa0
	DEFE_DI_DIAGINTP_REG_t     		di_diagintp        ;//0xa4
	DEFE_DI_TEMPDIFF_REG_t     		di_tempdiff        ;//0xa8
	DEFE_DI_LUMA_TH_REG_t     		di_lumath          ;//0xac
	DEFE_DI_SPATCOMP_REG_t     		di_spatcomp        ;//0xb0
	DEFE_DI_CHROMADIFF_REG_t   		di_chromadiff      ;//0xb4
	DEFE_DI_PRELUMA_REG_t      		di_preluma         ;//0xb8
	DEFE_DI_PRECHROMA_REG_t   		di_prechroma       ;//0xbc
	DEFE_DI_TILEFLAG0_REG_t 		di_tileflag0       ;//0xc0
	DEFE_DI_TILEFLAG1_REG_t         di_tileflag1       ;//0xc4
	DEFE_DI_FLAGLINESTRD_REG_t      di_flaglinestrd    ;//0xc8
	DEFE_DI_FLAG_SEQ_REG_t          di_flagseq         ;//0xcc
	DEFE_WB_LINESTRD_EN_REG_t  		wb_linestrd_en     ;//0xd0
	DEFE_WB_LINESTRD0_REG_t    		wb_linestrd0       ;//0xd4
	DEFE_WB_LINESTRD1_REG_t    		wb_linestrd1       ;//0xd8
	DEFE_WB_LINESTRD2_REG_t    		wb_linestrd2       ;//0xdc
	DEFE_3D_CTRL_REG_t         		trd_ctrl           ;//0xe0
	DEFE_3D_BUF_ADDR0_REG_t    		trd_buf_addr0      ;//0xe4 
	DEFE_3D_BUF_ADDR1_REG_t    		trd_buf_addr1      ;//0xe8
	DEFE_3D_BUF_ADDR2_REG_t    		trd_buf_addr2      ;//0xec 
	DEFE_3D_TB_OFF0_REG_t      		trd_tb_off0        ;//0xf0 
	DEFE_3D_TB_OFF1_REG_t      		trd_tb_off1        ;//0xf4 
	DEFE_3D_TB_OFF2_REG_t      		trd_tb_off2        ;//0xf8
	DEFE_3D_WB_STRD_REG_t           trd_wb_strd        ;//0xfc 
	DEFE_CH0_INSIZE_REG_t      		ch0_insize         ;//0x100
	DEFE_CH0_OUTSIZE_REG_t     		ch0_outsize        ;//0x104
	DEFE_CH0_HORZFACT_REG_t    		ch0_horzfact       ;//0x108
	DEFE_CH0_VERTFACT_REG_t    		ch0_vertfact       ;//0x10c
	DEFE_CH0_HORZPHASE_REG_t   		ch0_horzphase      ;//0x110
	DEFE_CH0_VERTPHASE0_REG_t  		ch0_vertphase0     ;//0x114
	DEFE_CH0_VERTPHASE1_REG_t  		ch0_vertphase1     ;//0x118
	DEFE_RESEVED_REG_t              res0x11c           ;//0x11c
	DEFE_CH0_HORZTAP0_REG_t    		ch0_horztap0       ;//0x120
	DEFE_CH0_HORZTAP1_REG_t    		ch0_horztap1       ;//0x124
	DEFE_CH0_VERTTAP_REG_t     		ch0_verttap        ;//0x128
	DEFE_RESEVED_REG_t              res0x12c[53]       ;//0x12c~0x1FF
	DEFE_CH1_INSIZE_REG_t      		ch1_insize         ;//0x200
	DEFE_CH1_OUTSIZE_REG_t     		ch1_outsize        ;//0x204
	DEFE_CH1_HORZFACT_REG_t    		ch1_horzfact       ;//0x208
	DEFE_CH1_VERTFACT_REG_t    		ch1_vertfact       ;//0x20c
	DEFE_CH1_HORZPHASE_REG_t   		ch1_horzphase      ;//0x210
	DEFE_CH1_VERTPHASE0_REG_t  		ch1_vertphase0     ;//0x214
	DEFE_CH1_VERTPHASE1_REG_t  		ch1_vertphase1     ;//0x218
	DEFE_RESEVED_REG_t              res0x21c           ;//0x21c
	DEFE_CH1_HORZTAP0_REG_t    		ch1_horztap0       ;//0x220
	DEFE_CH1_HORZTAP1_REG_t    		ch1_horztap1       ;//0x224
	DEFE_CH1_VERTTAP_REG_t     		ch1_verttap        ;//0x228
	DEFE_RESEVED_REG_t              res0x22c[117]      ;//0x22c~0x3FF
	DEFE_CH0_HORZCOEF0_REGN_t  		ch0_horzcoef0[32]  ;//0x400~0x47f	
	DEFE_CH0_HORZCOEF1_REGN_t  		ch0_horzcoef1[32]  ;//0x480~0x4ff	
	DEFE_CH0_VERTCOEF_REGN_t   		ch0_vertcoef[32]   ;//0x500~0x5ff	
	DEFE_RESEVED_REG_t              res0x580[32]       ;//0x580~0x57f
	DEFE_CH0_HORZCOEF0_REGN_t  		ch1_horzcoef0[32]  ;//0x600~0x67f	
	DEFE_CH0_HORZCOEF1_REGN_t  		ch1_horzcoef1[32]  ;//0x680~6ff	
	DEFE_CH0_VERTCOEF_REGN_t   		ch1_vertcoef[32]   ;//0x700~0x77f	
	DEFE_RESEVED_REG_t              res0x780[32]       ;//0x780~0x7ff
	DEFE_CH0_HORZCOEF0_REGN_t  		ch3_horzcoef0[32]  ;//0x800~0x87f	
	DEFE_CH0_HORZCOEF1_REGN_t  		ch3_horzcoef1[32]  ;//0x880~8ff	
	DEFE_CH0_VERTCOEF_REGN_t   		ch3_vertcoef[32]   ;//0x900~0x97f
	DEFE_RESEVED_REG_t              res0xA80[32]       ;//0xa80~0xaff
}__de_scal_dev_t;


typedef struct __SCAL_MATRIX4X4
{
	__s64 x00;
	__s64 x01;
	__s64 x02;
	__s64 x03;
	__s64 x10;
	__s64 x11;
	__s64 x12;
	__s64 x13;
	__s64 x20;
	__s64 x21;
	__s64 x22;
	__s64 x23;
	__s64 x30;
	__s64 x31;
	__s64 x32;
	__s64 x33;	
}__scal_matrix4x4;

extern __s32 iDE_SCAL_Matrix_Mul(__scal_matrix4x4 in1, __scal_matrix4x4 in2, __scal_matrix4x4 *result);
extern __s32 iDE_SCAL_Csc_Lmt(__s64 *value, __s32 min, __s32 max, __s32 shift, __s32 validbit);

#endif
