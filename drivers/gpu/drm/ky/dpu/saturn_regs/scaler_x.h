// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef SCALER_X_REG_H
#define SCALER_X_REG_H

typedef union
{
	struct
	{
	//REGISTER disp_scl_reg_0
	UINT32 m_nscl_hor_enable           : 1 ;
	UINT32 m_nscl_ver_enable           : 1 ;
	UINT32                             : 6 ;
	UINT32 m_nscl_input_width          : 16;
	UINT32                             : 8 ;


	//REGISTER disp_scl_reg_1
	UINT32 m_nscl_input_height         : 16;
	UINT32 m_nscl_output_width         : 16;


	//REGISTER disp_scl_reg_2
	UINT32 m_nscl_output_height        : 16;
	UINT32                             : 16;


	//REGISTER disp_scl_reg_3
	UINT32 m_nscl_hor_init_phase_l32b  : 32;


	//REGISTER disp_scl_reg_4
	UINT32 m_nscl_hor_init_phase_h1b   : 1 ;
	UINT32                             : 31;


	//REGISTER disp_scl_reg_5
	UINT32 m_nscl_ver_init_phase_l32b  : 32;


	//REGISTER disp_scl_reg_6
	UINT32 m_nscl_ver_init_phase_h1b   : 1 ;
	UINT32                             : 31;


	//REGISTER disp_scl_reg_7
	UINT32 m_nscl_hor_delta_phase      : 20;
	UINT32                             : 12;


	//REGISTER disp_scl_reg_8
	UINT32 m_nscl_ver_delta_phase      : 20;
	UINT32                             : 12;


	//REGISTER disp_scl_reg_9
	UINT32 m_nscl_hor_coef0            : 16;
	UINT32 m_nscl_hor_coef1            : 16;


	//REGISTER disp_scl_reg_10
	UINT32 m_nscl_hor_coef2            : 16;
	UINT32 m_nscl_hor_coef3            : 16;


	//REGISTER disp_scl_reg_11
	UINT32 m_nscl_hor_coef4            : 16;
	UINT32 m_nscl_hor_coef5            : 16;


	//REGISTER disp_scl_reg_12
	UINT32 m_nscl_hor_coef6            : 16;
	UINT32 m_nscl_hor_coef7            : 16;


	//REGISTER disp_scl_reg_13
	UINT32 m_nscl_hor_coef8            : 16;
	UINT32 m_nscl_hor_coef9            : 16;


	//REGISTER disp_scl_reg_14
	UINT32 m_nscl_hor_coef10           : 16;
	UINT32 m_nscl_hor_coef11           : 16;


	//REGISTER disp_scl_reg_15
	UINT32 m_nscl_hor_coef12           : 16;
	UINT32 m_nscl_hor_coef13           : 16;


	//REGISTER disp_scl_reg_16
	UINT32 m_nscl_hor_coef14           : 16;
	UINT32 m_nscl_hor_coef15           : 16;


	//REGISTER disp_scl_reg_17
	UINT32 m_nscl_hor_coef16           : 16;
	UINT32 m_nscl_hor_coef17           : 16;


	//REGISTER disp_scl_reg_18
	UINT32 m_nscl_hor_coef18           : 16;
	UINT32 m_nscl_hor_coef19           : 16;


	//REGISTER disp_scl_reg_19
	UINT32 m_nscl_hor_coef20           : 16;
	UINT32 m_nscl_hor_coef21           : 16;


	//REGISTER disp_scl_reg_20
	UINT32 m_nscl_hor_coef22           : 16;
	UINT32 m_nscl_hor_coef23           : 16;


	//REGISTER disp_scl_reg_21
	UINT32 m_nscl_hor_coef24           : 16;
	UINT32 m_nscl_hor_coef25           : 16;


	//REGISTER disp_scl_reg_22
	UINT32 m_nscl_hor_coef26           : 16;
	UINT32 m_nscl_hor_coef27           : 16;


	//REGISTER disp_scl_reg_23
	UINT32 m_nscl_hor_coef28           : 16;
	UINT32 m_nscl_hor_coef29           : 16;


	//REGISTER disp_scl_reg_24
	UINT32 m_nscl_hor_coef30           : 16;
	UINT32 m_nscl_hor_coef31           : 16;


	//REGISTER disp_scl_reg_25
	UINT32 m_nscl_hor_coef32           : 16;
	UINT32 m_nscl_hor_coef33           : 16;


	//REGISTER disp_scl_reg_26
	UINT32 m_nscl_hor_coef34           : 16;
	UINT32 m_nscl_hor_coef35           : 16;


	//REGISTER disp_scl_reg_27
	UINT32 m_nscl_hor_coef36           : 16;
	UINT32 m_nscl_hor_coef37           : 16;


	//REGISTER disp_scl_reg_28
	UINT32 m_nscl_hor_coef38           : 16;
	UINT32 m_nscl_hor_coef39           : 16;


	//REGISTER disp_scl_reg_29
	UINT32 m_nscl_hor_coef40           : 16;
	UINT32 m_nscl_hor_coef41           : 16;


	//REGISTER disp_scl_reg_30
	UINT32 m_nscl_hor_coef42           : 16;
	UINT32 m_nscl_hor_coef43           : 16;


	//REGISTER disp_scl_reg_31
	UINT32 m_nscl_hor_coef44           : 16;
	UINT32 m_nscl_hor_coef45           : 16;


	//REGISTER disp_scl_reg_32
	UINT32 m_nscl_hor_coef46           : 16;
	UINT32 m_nscl_hor_coef47           : 16;


	} b;

	struct
	{
	UINT32 disp_scl_reg_0;
	UINT32 disp_scl_reg_1;
	UINT32 disp_scl_reg_2;
	UINT32 disp_scl_reg_3;
	UINT32 disp_scl_reg_4;
	UINT32 disp_scl_reg_5;
	UINT32 disp_scl_reg_6;
	UINT32 disp_scl_reg_7;
	UINT32 disp_scl_reg_8;
	UINT32 disp_scl_reg_9;
	UINT32 disp_scl_reg_10;
	UINT32 disp_scl_reg_11;
	UINT32 disp_scl_reg_12;
	UINT32 disp_scl_reg_13;
	UINT32 disp_scl_reg_14;
	UINT32 disp_scl_reg_15;
	UINT32 disp_scl_reg_16;
	UINT32 disp_scl_reg_17;
	UINT32 disp_scl_reg_18;
	UINT32 disp_scl_reg_19;
	UINT32 disp_scl_reg_20;
	UINT32 disp_scl_reg_21;
	UINT32 disp_scl_reg_22;
	UINT32 disp_scl_reg_23;
	UINT32 disp_scl_reg_24;
	UINT32 disp_scl_reg_25;
	UINT32 disp_scl_reg_26;
	UINT32 disp_scl_reg_27;
	UINT32 disp_scl_reg_28;
	UINT32 disp_scl_reg_29;
	UINT32 disp_scl_reg_30;
	UINT32 disp_scl_reg_31;
	UINT32 disp_scl_reg_32;
	} v;

}SCALER_X_REG;

#endif

