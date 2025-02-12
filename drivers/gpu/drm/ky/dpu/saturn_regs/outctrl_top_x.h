// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef OUTCTRL_TOP_X_REG_H
#define OUTCTRL_TOP_X_REG_H

typedef union
{
	struct
	{
	//REGISTER Out_ctrl_reg_0
	UINT32 m_n_inwdith               : 13;
	UINT32                           : 3 ;
	UINT32 m_n_inheight              : 14;
	UINT32                           : 2 ;


	//REGISTER Out_ctrl_reg_1
	UINT32 scale_wdith               : 13;
	UINT32                           : 3 ;
	UINT32 scale_height              : 14;
	UINT32                           : 2 ;


	//REGISTER Out_ctrl_reg_2
	UINT32                           : 32;


	//REGISTER Out_ctrl_reg_3
	UINT32                           : 32;


	//REGISTER Out_ctrl_reg_4
	UINT32 acad_en                   : 1 ;
	UINT32 dsc_dfc_switch_en         : 1 ;
	UINT32                           : 30;


	//REGISTER Out_ctrl_reg_5
	UINT32 scale_en                  : 1 ;
	UINT32 dither_en                 : 1 ;
	UINT32 dither_mode               : 1 ;
	UINT32 dither_auto_temp          : 1 ;
	UINT32 dither_out_dpth0          : 4 ;
	UINT32 dither_out_dpth1          : 4 ;
	UINT32 dither_out_dpth2          : 4 ;
	UINT32 dither_temp_value         : 8 ;
	UINT32 dither_rotate_mode        : 2 ;
	UINT32 dither_pattern_bit        : 3 ;
	UINT32                           : 3 ;


	//REGISTER Out_ctrl_reg_6
	UINT32 sbs_en                    : 1 ;
	UINT32 split_en                  : 1 ;
	UINT32 rgb2yuv_en                : 1 ;
	UINT32 narrow_yuv_en             : 1 ;
	UINT32 cmd_screen                : 1 ;
	UINT32 frame_timing_en           : 1 ;
	UINT32 cmd_wait_en               : 1 ;
	UINT32 cmd_wait_te               : 1 ;
	UINT32 reg06_bit8_dmy            : 1 ;
	UINT32 tm_ctrl_reload_option     : 1 ;
	UINT32                           : 22;


	//REGISTER Out_ctrl_reg_7
	UINT32 hblank                    : 12;
	UINT32                           : 4 ;
	UINT32 split_overlap             : 10;
	UINT32                           : 6 ;


	//REGISTER Out_ctrl_reg_8
	UINT32 rgb2yuv_matrix00          : 14;
	UINT32                           : 2 ;
	UINT32 rgb2yuv_matrix01          : 14;
	UINT32                           : 2 ;


	//REGISTER Out_ctrl_reg_9
	UINT32 rgb2yuv_matrix02          : 14;
	UINT32                           : 2 ;
	UINT32 rgb2yuv_matrix03          : 14;
	UINT32                           : 2 ;


	//REGISTER Out_ctrl_reg_10
	UINT32 rgb2yuv_matrix10          : 14;
	UINT32                           : 2 ;
	UINT32 rgb2yuv_matrix11          : 14;
	UINT32                           : 2 ;


	//REGISTER Out_ctrl_reg_11
	UINT32 rgb2yuv_matrix12          : 14;
	UINT32                           : 2 ;
	UINT32 rgb2yuv_matrix13          : 14;
	UINT32                           : 2 ;


	//REGISTER Out_ctrl_reg_12
	UINT32 rgb2yuv_matrix20          : 14;
	UINT32                           : 2 ;
	UINT32 rgb2yuv_matrix21          : 14;
	UINT32                           : 2 ;


	//REGISTER Out_ctrl_reg_13
	UINT32 rgb2yuv_matrix22          : 14;
	UINT32                           : 2 ;
	UINT32 rgb2yuv_matrix23          : 14;
	UINT32                           : 2 ;


	//REGISTER Out_ctrl_reg_14
	UINT32 dither_bayer_map00        : 8 ;
	UINT32 dither_bayer_map01        : 8 ;
	UINT32 dither_bayer_map02        : 8 ;
	UINT32 dither_bayer_map03        : 8 ;


	//REGISTER Out_ctrl_reg_15
	UINT32 dither_bayer_map04        : 8 ;
	UINT32 dither_bayer_map05        : 8 ;
	UINT32 dither_bayer_map06        : 8 ;
	UINT32 dither_bayer_map07        : 8 ;


	//REGISTER Out_ctrl_reg_16
	UINT32 dither_bayer_map10        : 8 ;
	UINT32 dither_bayer_map11        : 8 ;
	UINT32 dither_bayer_map12        : 8 ;
	UINT32 dither_bayer_map13        : 8 ;


	//REGISTER Out_ctrl_reg_17
	UINT32 dither_bayer_map14        : 8 ;
	UINT32 dither_bayer_map15        : 8 ;
	UINT32 dither_bayer_map16        : 8 ;
	UINT32 dither_bayer_map17        : 8 ;


	//REGISTER Out_ctrl_reg_18
	UINT32 dither_bayer_map20        : 8 ;
	UINT32 dither_bayer_map21        : 8 ;
	UINT32 dither_bayer_map22        : 8 ;
	UINT32 dither_bayer_map23        : 8 ;


	//REGISTER Out_ctrl_reg_19
	UINT32 dither_bayer_map24        : 8 ;
	UINT32 dither_bayer_map25        : 8 ;
	UINT32 dither_bayer_map26        : 8 ;
	UINT32 dither_bayer_map27        : 8 ;


	//REGISTER Out_ctrl_reg_20
	UINT32 dither_bayer_map30        : 8 ;
	UINT32 dither_bayer_map31        : 8 ;
	UINT32 dither_bayer_map32        : 8 ;
	UINT32 dither_bayer_map33        : 8 ;


	//REGISTER Out_ctrl_reg_21
	UINT32 dither_bayer_map34        : 8 ;
	UINT32 dither_bayer_map35        : 8 ;
	UINT32 dither_bayer_map36        : 8 ;
	UINT32 dither_bayer_map37        : 8 ;


	//REGISTER Out_ctrl_reg_22
	UINT32 dither_bayer_map40        : 8 ;
	UINT32 dither_bayer_map41        : 8 ;
	UINT32 dither_bayer_map42        : 8 ;
	UINT32 dither_bayer_map43        : 8 ;


	//REGISTER Out_ctrl_reg_23
	UINT32 dither_bayer_map44        : 8 ;
	UINT32 dither_bayer_map45        : 8 ;
	UINT32 dither_bayer_map46        : 8 ;
	UINT32 dither_bayer_map47        : 8 ;


	//REGISTER Out_ctrl_reg_24
	UINT32 dither_bayer_map50        : 8 ;
	UINT32 dither_bayer_map51        : 8 ;
	UINT32 dither_bayer_map52        : 8 ;
	UINT32 dither_bayer_map53        : 8 ;


	//REGISTER Out_ctrl_reg_25
	UINT32 dither_bayer_map54        : 8 ;
	UINT32 dither_bayer_map55        : 8 ;
	UINT32 dither_bayer_map56        : 8 ;
	UINT32 dither_bayer_map57        : 8 ;


	//REGISTER Out_ctrl_reg_26
	UINT32 dither_bayer_map60        : 8 ;
	UINT32 dither_bayer_map61        : 8 ;
	UINT32 dither_bayer_map62        : 8 ;
	UINT32 dither_bayer_map63        : 8 ;


	//REGISTER Out_ctrl_reg_27
	UINT32 dither_bayer_map64        : 8 ;
	UINT32 dither_bayer_map65        : 8 ;
	UINT32 dither_bayer_map66        : 8 ;
	UINT32 dither_bayer_map67        : 8 ;


	//REGISTER Out_ctrl_reg_28
	UINT32 dither_bayer_map70        : 8 ;
	UINT32 dither_bayer_map71        : 8 ;
	UINT32 dither_bayer_map72        : 8 ;
	UINT32 dither_bayer_map73        : 8 ;


	//REGISTER Out_ctrl_reg_29
	UINT32 dither_bayer_map74        : 8 ;
	UINT32 dither_bayer_map75        : 8 ;
	UINT32 dither_bayer_map76        : 8 ;
	UINT32 dither_bayer_map77        : 8 ;


	//REGISTER Out_ctrl_reg_30
	UINT32 dfc_low_thre              : 12;
	UINT32                           : 4 ;
	UINT32 dfc_high_thre             : 12;
	UINT32                           : 4 ;


	//REGISTER Out_ctrl_reg_31
	UINT32 split_cfg_manual_en       : 1 ;
	UINT32                           : 7 ;
	UINT32 disp_ready_man_en         : 1 ;
	UINT32 disp_ready_1_non_active   : 1 ;
	UINT32                           : 22;


	//REGISTER Out_ctrl_reg_32
	UINT32 hfp                       : 12;
	UINT32                           : 4 ;
	UINT32 hbp                       : 12;
	UINT32                           : 4 ;


	//REGISTER Out_ctrl_reg_33
	UINT32 vfp                       : 12;
	UINT32                           : 4 ;
	UINT32 vbp                       : 12;
	UINT32                           : 4 ;


	//REGISTER Out_ctrl_reg_34
	UINT32 hsync_width               : 10;
	UINT32                           : 2 ;
	UINT32 hsp                       : 1 ;
	UINT32                           : 3 ;
	UINT32 vsync_width               : 10;
	UINT32                           : 2 ;
	UINT32 vsp                       : 1 ;
	UINT32                           : 3 ;


	//REGISTER Out_ctrl_reg_35
	UINT32 h_active                  : 14;
	UINT32                           : 2 ;
	UINT32 v_active                  : 14;
	UINT32                           : 2 ;


	//REGISTER Out_ctrl_reg_36
	UINT32 user                      : 4 ;
	UINT32                           : 28;


	//REGISTER Out_ctrl_reg_37
	UINT32 fm_cmd_timeout_num        : 24;
	UINT32 fm_cmd_timeout_eq_eof     : 1 ;
	UINT32                           : 7 ;


	//REGISTER Out_ctrl_reg_38
	UINT32 back_ground_r             : 12;
	UINT32                           : 4 ;
	UINT32 back_ground_g             : 12;
	UINT32                           : 4 ;


	//REGISTER Out_ctrl_reg_39
	UINT32 back_ground_b             : 12;
	UINT32                           : 20;


	//REGISTER Out_ctrl_reg_40
	UINT32 drift_timeout             : 12;
	UINT32                           : 20;


	//REGISTER Out_ctrl_reg_41
	UINT32 eof_ln_dly                : 10;
	UINT32                           : 22;


	//REGISTER Out_ctrl_reg_42
	UINT32 sof_pre_ln_num            : 10;
	UINT32                           : 22;


	//REGISTER Out_ctrl_reg_43
	UINT32 cfg_ln_num_intp           : 14;
	UINT32                           : 18;


	//REGISTER Out_ctrl_reg_44
	UINT32 sof0_irq_raw              : 1 ;
	UINT32 eof0_irq_raw              : 1 ;
	UINT32 cfg_eof0_irq_raw          : 1 ;
	UINT32 cfg_eol0_irq_raw          : 1 ;
	UINT32 underflow0_irq_raw        : 1 ;
	UINT32 sof1_irq_raw              : 1 ;
	UINT32 eof1_irq_raw              : 1 ;
	UINT32 cfg_eof1_irq_raw          : 1 ;
	UINT32 cfg_eol1_irq_raw          : 1 ;
	UINT32 underflow1_irq_raw        : 1 ;
	UINT32                           : 22;


	//REGISTER Out_ctrl_reg_45
	UINT32 line_irq_raw              : 1 ;
	UINT32 drift_timeout_irq_raw     : 1 ;
	UINT32 acad_irq_raw              : 1 ;
	UINT32 dsc0_overflow_irq_raw     : 1 ;
	UINT32 dsc1_overflow_irq_raw     : 1 ;
	UINT32                           : 27;


	//REGISTER Out_ctrl_reg_46
	UINT32 sof0_irq_mask             : 1 ;
	UINT32 eof0_irq_mask             : 1 ;
	UINT32 cfg_eof0_irq_mask         : 1 ;
	UINT32 cfg_eol0_irq_mask         : 1 ;
	UINT32 underflow0_irq_mask       : 1 ;
	UINT32 sof1_irq_mask             : 1 ;
	UINT32 eof1_irq_mask             : 1 ;
	UINT32 cfg_eof1_irq_mask         : 1 ;
	UINT32 cfg_eol1_irq_mask         : 1 ;
	UINT32 underflow1_irq_mask       : 1 ;
	UINT32                           : 22;


	//REGISTER Out_ctrl_reg_47
	UINT32 line_irq_mask             : 1 ;
	UINT32 drift_timeout_irq_mask    : 1 ;
	UINT32 acad_irq_mask             : 1 ;
	UINT32 dsc0_overflow_mask        : 1 ;
	UINT32 dsc1_overflow_mask        : 1 ;
	UINT32                           : 27;


	//REGISTER Out_ctrl_reg_48
	UINT32 sof0_irq_status           : 1 ;
	UINT32 eof0_irq_status           : 1 ;
	UINT32 cfg_eof0_irq_status       : 1 ;
	UINT32 cfg_eol0_irq_status       : 1 ;
	UINT32 underflow0_irq_status     : 1 ;
	UINT32 sof1_irq_status           : 1 ;
	UINT32 eof1_irq_status           : 1 ;
	UINT32 cfg_eof1_irq_status       : 1 ;
	UINT32 cfg_eol1_irq_status       : 1 ;
	UINT32 underflow1_irq_status     : 1 ;
	UINT32                           : 22;


	//REGISTER Out_ctrl_reg_49
	UINT32 line0_irq_status          : 1 ;
	UINT32 drift_timeout_irq_status  : 1 ;
	UINT32 acad_irq_status           : 1 ;
	UINT32 dsc0_overflow_irq_sta     : 1 ;
	UINT32 dsc1_overflow_irq_sta     : 1 ;
	UINT32                           : 27;


	//REGISTER Out_ctrl_reg_50
	UINT32 postproc_ln_cnt           : 14;
	UINT32                           : 2 ;
	UINT32 postproc_pix_cnt          : 13;
	UINT32                           : 3 ;


	//REGISTER Out_ctrl_reg_51
	UINT32 dither_ln_cnt             : 14;
	UINT32                           : 2 ;
	UINT32 dither_pix_cnt            : 13;
	UINT32                           : 3 ;


	//REGISTER Out_ctrl_reg_52
	UINT32 dat_convert_ln_cnt        : 14;
	UINT32                           : 2 ;
	UINT32 dat_convert_pix_cnt       : 13;
	UINT32                           : 3 ;


	//REGISTER Out_ctrl_reg_53
	UINT32 merge_ln_cnt              : 14;
	UINT32                           : 2 ;
	UINT32 merge_pix_cnt             : 14;
	UINT32                           : 2 ;


	//REGISTER Out_ctrl_reg_54
	UINT32 split_ln_cnt              : 14;
	UINT32                           : 2 ;
	UINT32 split_pix_cnt             : 14;
	UINT32                           : 2 ;


	};

	INT32 value32[55];

}OUTCTRL_TOP_X_REG;

#endif

