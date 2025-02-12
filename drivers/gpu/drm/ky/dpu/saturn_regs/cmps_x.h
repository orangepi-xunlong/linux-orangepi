// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef CMPS_X_REG_H
#define CMPS_X_REG_H

typedef union
{
	struct
	{
	//REGISTER dpu_cmps_reg_0
	UINT32 m_ncmps_en            : 1 ;
	UINT32                       : 7 ;
	UINT32 m_noutput_width       : 16;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_1
	UINT32 m_noutput_height      : 16;
	UINT32                       : 16;


	//REGISTER dpu_cmps_reg_2
	UINT32 m_nbg_color_R         : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_3
	UINT32 m_nbg_color_G         : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_4
	UINT32 m_nbg_color_B         : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_5
	UINT32 m_nbg_color_A         : 8 ;
	UINT32                       : 24;


	//REGISTER dpu_cmps_reg_6
	UINT32 m_nl0_solid_color_A   : 8 ;
	UINT32                       : 8 ;
	UINT32 m_nl1_solid_color_A   : 8 ;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_7
	UINT32 m_nl2_solid_color_A   : 8 ;
	UINT32                       : 8 ;
	UINT32 m_nl3_solid_color_A   : 8 ;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_8
	UINT32 m_nl4_solid_color_A   : 8 ;
	UINT32                       : 8 ;
	UINT32 m_nl5_solid_color_A   : 8 ;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_9
	UINT32 m_nl6_solid_color_A   : 8 ;
	UINT32                       : 8 ;
	UINT32 m_nl7_solid_color_A   : 8 ;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_10
	UINT32 m_nl8_solid_color_A   : 8 ;
	UINT32                       : 8 ;
	UINT32 m_nl9_solid_color_A   : 8 ;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_11
	UINT32 m_nl10_solid_color_A  : 8 ;
	UINT32                       : 8 ;
	UINT32 m_nl11_solid_color_A  : 8 ;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_12
	UINT32 m_nl12_solid_color_A  : 8 ;
	UINT32                       : 8 ;
	UINT32 m_nl13_solid_color_A  : 8 ;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_13
	UINT32 m_nl14_solid_color_A  : 8 ;
	UINT32                       : 8 ;
	UINT32 m_nl15_solid_color_A  : 8 ;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_14
	UINT32 m_nl0_en              : 1 ;
	UINT32 m_nl0_layer_id        : 4 ;
	UINT32 m_nl0_solid_en        : 1 ;
	UINT32                       : 26;


	//REGISTER dpu_cmps_reg_15
	UINT32 m_nl0_solid_color_R   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_16
	UINT32 m_nl0_solid_color_G   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_17
	UINT32 m_nl0_solid_color_B   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_18
	UINT32 m_nl0_color_key_en    : 1 ;
	UINT32                       : 7 ;
	UINT32 m_nl0_rect_ltopx      : 16;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_19
	UINT32 m_nl0_rect_ltopy      : 16;
	UINT32 m_nl0_rect_rbotx      : 16;


	//REGISTER dpu_cmps_reg_20
	UINT32 m_nl0_rect_rboty      : 16;
	UINT32 m_nl0_blend_mode      : 2 ;
	UINT32 m_nl0_alpha_sel       : 1 ;
	UINT32                       : 13;


	//REGISTER dpu_cmps_reg_21
	UINT32 m_nl0_alpha_factor    : 8 ;
	UINT32                       : 8 ;
	UINT32 m_nl0_layer_alpha     : 8 ;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_22
	UINT32 m_nl1_en              : 1 ;
	UINT32 m_nl1_layer_id        : 4 ;
	UINT32 m_nl1_solid_en        : 1 ;
	UINT32                       : 26;


	//REGISTER dpu_cmps_reg_23
	UINT32 m_nl1_solid_color_R   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_24
	UINT32 m_nl1_solid_color_G   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_25
	UINT32 m_nl1_solid_color_B   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_26
	UINT32 m_nl1_color_key_en    : 1 ;
	UINT32                       : 7 ;
	UINT32 m_nl1_rect_ltopx      : 16;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_27
	UINT32 m_nl1_rect_ltopy      : 16;
	UINT32 m_nl1_rect_rbotx      : 16;


	//REGISTER dpu_cmps_reg_28
	UINT32 m_nl1_rect_rboty      : 16;
	UINT32 m_nl1_blend_mode      : 2 ;
	UINT32 m_nl1_alpha_sel       : 1 ;
	UINT32                       : 13;


	//REGISTER dpu_cmps_reg_29
	UINT32 m_nl1_alpha_factor    : 8 ;
	UINT32                       : 8 ;
	UINT32 m_nl1_layer_alpha     : 8 ;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_30
	UINT32 m_nl2_en              : 1 ;
	UINT32 m_nl2_layer_id        : 4 ;
	UINT32 m_nl2_solid_en        : 1 ;
	UINT32                       : 26;


	//REGISTER dpu_cmps_reg_31
	UINT32 m_nl2_solid_color_R   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_32
	UINT32 m_nl2_solid_color_G   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_33
	UINT32 m_nl2_solid_color_B   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_34
	UINT32 m_nl2_color_key_en    : 1 ;
	UINT32                       : 7 ;
	UINT32 m_nl2_rect_ltopx      : 16;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_35
	UINT32 m_nl2_rect_ltopy      : 16;
	UINT32 m_nl2_rect_rbotx      : 16;


	//REGISTER dpu_cmps_reg_36
	UINT32 m_nl2_rect_rboty      : 16;
	UINT32 m_nl2_blend_mode      : 2 ;
	UINT32 m_nl2_alpha_sel       : 1 ;
	UINT32                       : 13;


	//REGISTER dpu_cmps_reg_37
	UINT32 m_nl2_alpha_factor    : 8 ;
	UINT32                       : 8 ;
	UINT32 m_nl2_layer_alpha     : 8 ;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_38
	UINT32 m_nl3_en              : 1 ;
	UINT32 m_nl3_layer_id        : 4 ;
	UINT32 m_nl3_solid_en        : 1 ;
	UINT32                       : 26;


	//REGISTER dpu_cmps_reg_39
	UINT32 m_nl3_solid_color_R   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_40
	UINT32 m_nl3_solid_color_G   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_41
	UINT32 m_nl3_solid_color_B   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_42
	UINT32 m_nl3_color_key_en    : 1 ;
	UINT32                       : 7 ;
	UINT32 m_nl3_rect_ltopx      : 16;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_43
	UINT32 m_nl3_rect_ltopy      : 16;
	UINT32 m_nl3_rect_rbotx      : 16;


	//REGISTER dpu_cmps_reg_44
	UINT32 m_nl3_rect_rboty      : 16;
	UINT32 m_nl3_blend_mode      : 2 ;
	UINT32 m_nl3_alpha_sel       : 1 ;
	UINT32                       : 13;


	//REGISTER dpu_cmps_reg_45
	UINT32 m_nl3_alpha_factor    : 8 ;
	UINT32                       : 8 ;
	UINT32 m_nl3_layer_alpha     : 8 ;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_46
	UINT32 m_nl4_en              : 1 ;
	UINT32 m_nl4_layer_id        : 4 ;
	UINT32 m_nl4_solid_en        : 1 ;
	UINT32                       : 26;


	//REGISTER dpu_cmps_reg_47
	UINT32 m_nl4_solid_color_R   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_48
	UINT32 m_nl4_solid_color_G   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_49
	UINT32 m_nl4_solid_color_B   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_50
	UINT32 m_nl4_color_key_en    : 1 ;
	UINT32                       : 7 ;
	UINT32 m_nl4_rect_ltopx      : 16;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_51
	UINT32 m_nl4_rect_ltopy      : 16;
	UINT32 m_nl4_rect_rbotx      : 16;


	//REGISTER dpu_cmps_reg_52
	UINT32 m_nl4_rect_rboty      : 16;
	UINT32 m_nl4_blend_mode      : 2 ;
	UINT32 m_nl4_alpha_sel       : 1 ;
	UINT32                       : 13;


	//REGISTER dpu_cmps_reg_53
	UINT32 m_nl4_alpha_factor    : 8 ;
	UINT32                       : 8 ;
	UINT32 m_nl4_layer_alpha     : 8 ;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_54
	UINT32 m_nl5_en              : 1 ;
	UINT32 m_nl5_layer_id        : 4 ;
	UINT32 m_nl5_solid_en        : 1 ;
	UINT32                       : 26;


	//REGISTER dpu_cmps_reg_55
	UINT32 m_nl5_solid_color_R   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_56
	UINT32 m_nl5_solid_color_G   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_57
	UINT32 m_nl5_solid_color_B   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_58
	UINT32 m_nl5_color_key_en    : 1 ;
	UINT32                       : 7 ;
	UINT32 m_nl5_rect_ltopx      : 16;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_59
	UINT32 m_nl5_rect_ltopy      : 16;
	UINT32 m_nl5_rect_rbotx      : 16;


	//REGISTER dpu_cmps_reg_60
	UINT32 m_nl5_rect_rboty      : 16;
	UINT32 m_nl5_blend_mode      : 2 ;
	UINT32 m_nl5_alpha_sel       : 1 ;
	UINT32                       : 13;


	//REGISTER dpu_cmps_reg_61
	UINT32 m_nl5_alpha_factor    : 8 ;
	UINT32                       : 8 ;
	UINT32 m_nl5_layer_alpha     : 8 ;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_62
	UINT32 m_nl6_en              : 1 ;
	UINT32 m_nl6_layer_id        : 4 ;
	UINT32 m_nl6_solid_en        : 1 ;
	UINT32                       : 26;


	//REGISTER dpu_cmps_reg_63
	UINT32 m_nl6_solid_color_R   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_64
	UINT32 m_nl6_solid_color_G   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_65
	UINT32 m_nl6_solid_color_B   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_66
	UINT32 m_nl6_color_key_en    : 1 ;
	UINT32                       : 7 ;
	UINT32 m_nl6_rect_ltopx      : 16;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_67
	UINT32 m_nl6_rect_ltopy      : 16;
	UINT32 m_nl6_rect_rbotx      : 16;


	//REGISTER dpu_cmps_reg_68
	UINT32 m_nl6_rect_rboty      : 16;
	UINT32 m_nl6_blend_mode      : 2 ;
	UINT32 m_nl6_alpha_sel       : 1 ;
	UINT32                       : 13;


	//REGISTER dpu_cmps_reg_69
	UINT32 m_nl6_alpha_factor    : 8 ;
	UINT32                       : 8 ;
	UINT32 m_nl6_layer_alpha     : 8 ;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_70
	UINT32 m_nl7_en              : 1 ;
	UINT32 m_nl7_layer_id        : 4 ;
	UINT32 m_nl7_solid_en        : 1 ;
	UINT32                       : 26;


	//REGISTER dpu_cmps_reg_71
	UINT32 m_nl7_solid_color_R   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_72
	UINT32 m_nl7_solid_color_G   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_73
	UINT32 m_nl7_solid_color_B   : 12;
	UINT32                       : 20;


	//REGISTER dpu_cmps_reg_74
	UINT32 m_nl7_color_key_en    : 1 ;
	UINT32                       : 7 ;
	UINT32 m_nl7_rect_ltopx      : 16;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_75
	UINT32 m_nl7_rect_ltopy      : 16;
	UINT32 m_nl7_rect_rbotx      : 16;


	//REGISTER dpu_cmps_reg_76
	UINT32 m_nl7_rect_rboty      : 16;
	UINT32 m_nl7_blend_mode      : 2 ;
	UINT32 m_nl7_alpha_sel       : 1 ;
	UINT32                       : 13;


	//REGISTER dpu_cmps_reg_77
	UINT32 m_nl7_alpha_factor    : 8 ;
	UINT32                       : 8 ;
	UINT32 m_nl7_layer_alpha     : 8 ;
	UINT32                       : 8 ;


	//REGISTER dpu_cmps_reg_78
	struct
	{
	UINT32                       : 32;
	}dpu_cmps_reg_78[64];

	//REGISTER dpu_cmps_reg_142
	UINT32 dbug_bus0             : 32;


	//REGISTER dpu_cmps_reg_143
	UINT32 dbg_bus1              : 32;


	//REGISTER dpu_cmps_reg_144
	UINT32 dbg_bus2              : 32;


	//REGISTER dpu_cmps_reg_145
	UINT32 dbg_bus3              : 32;


	};

	INT32 value32[146];

}CMPS_X_REG;

#endif

