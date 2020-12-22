//*****************************************************************************
//  All Winner Micro, All Right Reserved. 2006-2012 Copyright (c)
//
//  File name   :        iep_deu_ebios.h
//
//  Description :  Image Enhancement Processor Detail Enhancement Unit registers and interface functions define
//                 for A1x
//  History     :
//                2011/05/13      vito       v0.1    Initial version in A10 in de_fe.h
//				  2012/06/12      vito       v0.5    Initial version for iep-deu in A1x
//******************************************************************************
#ifndef __IEP_DEU_EBIOS_H__
#define __IEP_DEU_EBIOS_H__

#include "../iep.h"

typedef union   
{
	__u32 dwval;
	struct
	{
		__u32	en				:1;	//bit0
		__u32	r0				:3;	//bit1~bit3
		__u32	reg_rdy			:1;	//bit4
		__u32   coeff_rdy		:1;	//bit5
		__u32   r1				:2; //bit6~bit7
		__u32	csc_en			:1;	//bit8
		__u32	r2				:3; //bit9~bit11
		__u32	out_port_sel	:1;	//bit12
		__u32	r3				:19;//bit13~bit31
		
	}bits;
}__imgehc_deu_en_reg_t;	//0x00

typedef union   
{
	__u32 dwval;
	struct
	{
		__u32	width			:12;//bit0~bit11
		__u32	r0				:4;	//bit12~15
		__u32	height			:12;//bit16~bit27
		__u32   r1				:4; //bit28~bit31
	}bits;
}__imgehc_deu_psize_reg_t;//0x04

typedef union   
{
	__u32 dwval;
	struct
	{
		__u32	win_left		:12;//bit0~bit11
		__u32	r0				:4;	//bit12~15
		__u32	win_top			:12;//bit16~bit27
		__u32   r1				:4; //bit28~bit31
	}bits;
}__imgehc_deu_pwp0_reg_t;	//0x08

typedef union   
{
	__u32 dwval;
	struct
	{
		__u32	win_right		:12;//bit0~bit11
		__u32	r0				:4;	//bit12~15
		__u32	win_bot			:12;//bit16~bit27
		__u32   r1				:4; //bit28~bit31
	}bits;
}__imgehc_deu_pwp1_reg_t;	//0x0c

typedef union   
{
	__u32 dwval;
	struct
	{
		__u32	dcti_en			:1;	//bit0
		__u32	r0				:5;	//bit1~bit5
		__u32	dcti_hill_en	:1;	//bit6
		__u32	dcti_suphill_en	:1;	//bit7
		__u32	dcti_filter1_sel:2;	//bit8~bit9
		__u32	dcti_filter2_sel:2;	//bit10~bit11
		__u32	dcti_path_limit :4;	//bit12~bit15
		__u32   dcti_gain		:6; //bit16~bit21
		__u32   r1				:2;	//bit22~bit23
		__u32   uv_diff_sign_mode_sel:2;//bit24~bit25
		__u32   uv_same_sign_mode_sel:2;//bit26~bit27
		__u32   uv_diff_sign_maxmin_mode_sel:1;//bit28
		__u32   uv_same_sign_maxmin_mode_sel:1;//bit29
		__u32   r2              :1;	//bit30
		__u32	uv_separate_en	:1;	//bit31
	}bits;
}__imgehc_deu_dcti_reg_t;	//0x10

typedef union   
{
	__u32 dwval;
	struct
	{
		__u32	lp_en			:1;	//bit0
		__u32	r0				:7;	//bit1~bit7
		__u32	tau				:5;	//bit8~bit12
		__u32   r1				:3; //bit13~bit15
		__u32	alpha			:5;	//bit16~bit20
		__u32   r2              :3;	//bit21~bit23
		__u32   beta            :5; //bit24~bit28
		__u32	r3				:3; //bit29~bit31
	}bits;
}__imgehc_deu_lp0_reg_t;	//0x14

typedef union   
{
	__u32 dwval;
	struct
	{
		__u32	lp_mode			:1;	//bit0
		__u32   str				:1;	//bit1
		__u32	r0				:6;	//bit2~bit7
		__u32	corthr			:8;	//bit8~bit15
		__u32	neggain			:2;	//bit16~bit17
		__u32   r1              :2;	//bit18~bit19
		__u32   delta           :2; //bit20~bit21
		__u32   r2              :2;	//bit22~bit23
		__u32	limit_thr		:8; //bit24~bit31
	}bits;
}__imgehc_deu_lp1_reg_t;	//0x18

typedef union   
{
	__u32 dwval;
	struct
	{
		__u32	straddr			:32;//bit0~bit31	
	}bits;
}__imgehc_deu_lp_straddr_reg_t;	//0x1c

typedef union   
{
	__u32 dwval;
	struct
	{
		__u32	wle_en			:1;	//bit0
		__u32	r0				:7;	//bit1~bit7
		__u32	wle_thr			:8;	//bit8~bit15
		__u32	wle_gain		:8;	//bit16~bit23
		__u32   r1              :8;	//bit24~bit31
	}bits;
}__imgehc_deu_wle_reg_t;	//0x28

typedef union   
{
	__u32 dwval;
	struct
	{
		__u32	ble_en			:1;	//bit0
		__u32	r0				:7;	//bit1~bit7
		__u32	ble_thr			:8;	//bit8~bit15
		__u32	ble_gain		:8;	//bit16~bit23
		__u32   r1              :8;	//bit24~bit31
	}bits;
}__imgehc_deu_ble_reg_t;	//0x2c

typedef union
{
	__u32 dwval;
	struct
	{
		__u32	csc_g_coff		:13;//bit0~bit12
		__u32	r0				:19;//bit13~bit31
	}bits;
}__imgehc_deu_cscgcoff_reg_t;	//0x30~0x38

typedef union
{
	__u32 dwval;
	struct
	{
		__u32	csc_g_con		:14;//bit0~bit13
		__u32	r0				:18;//bit14~bit31
	}bits;
}__imgehc_deu_cscgcon_reg_t;	//0x3c

typedef union
{
	__u32 dwval;
	struct
	{
		__u32	csc_r_coff		:13;//bit0~bit12
		__u32	r0				:19;//bit13~bit31
	}bits;
}__imgehc_deu_cscrcoff_reg_t;	//0x40~0x48

typedef union
{
	__u32 dwval;
	struct
	{
		__u32	csc_r_con		:14;//bit0~bit13
		__u32	r0				:18;//bit14~bit31
	}bits;
}__imgehc_deu_cscrcon_reg_t;	//0x4c

typedef union
{
	__u32 dwval;
	struct
	{
		__u32	csc_b_coff		:13;//bit0~bit12
		__u32	r0				:19;//bit13~bit31
	}bits;
}__imgehc_deu_cscbcoff_reg_t;	//0x50~0x58

typedef union
{
	__u32 dwval;
	struct
	{
		__u32	csc_b_con		:14;//bit0~bit13
		__u32	r0				:18;//bit14~bit31
	}bits;
}__imgehc_deu_cscbcon_reg_t;	//0x5c

typedef struct 
{
	__imgehc_deu_en_reg_t			en;		//0x00
	__imgehc_deu_psize_reg_t		psize;	//0x04
	__imgehc_deu_pwp0_reg_t			pwp0;	//0x08
	__imgehc_deu_pwp1_reg_t			pwp1;	//0x0c
	__imgehc_deu_dcti_reg_t			dcti;	//0x10
	__imgehc_deu_lp0_reg_t			lp0;	//0x14
	__imgehc_deu_lp1_reg_t			lp1;	//0x18
	__imgehc_deu_lp_straddr_reg_t   straddr;//0x1c
	__u32							r0[2];	//0x20~0x24
	__imgehc_deu_wle_reg_t			wle;	//0x28
	__imgehc_deu_ble_reg_t			ble;	//0x2c		
	__imgehc_deu_cscgcoff_reg_t		cscgcoff[3];//0x30~0x38
	__imgehc_deu_cscgcon_reg_t		cscgcon;	//0x3c
	__imgehc_deu_cscrcoff_reg_t		cscrcoff[3];//0x40~0x48
	__imgehc_deu_cscrcon_reg_t		cscrcon;	//0x4c
	__imgehc_deu_cscbcoff_reg_t		cscbcoff[3];//0x50~0x58
	__imgehc_deu_cscbcon_reg_t		cscbcon;	//0x5c
}__iep_deu_dev_t;

#define ____SEPARATOR_GLOBAL____
__s32 DEU_EBIOS_Set_Reg_Base(__u32 sel, __u32 base);
__u32 DEU_EBIOS_Get_Reg_Base(__u32 sel);
__s32 DEU_EBIOS_Enable(__u32 sel, __u32 en);
__s32 DEU_EBIOS_Csc_Enable(__u32 sel, __u32 en);
__s32 DEU_EBIOS_Set_Csc_Coeff(__u32 sel, __u32 mode);
__s32 DEU_EBIOS_Set_Display_Size(__u32 sel, __u32 width, __u32 height);
__s32 DEU_EBIOS_Set_Win_Para(__u32 sel, __u32 top, __u32 bot, __u32 left, __u32 right);
__s32 DEU_EBIOS_Cfg_Rdy(__u32 sel);
__s32 DEU_EBIOS_Set_Output_Chnl(__u32 sel, __u32 be_sel);
#define ____SEPARATOR_LP____
__s32 DEU_EBIOS_LP_STR_Cfg_Rdy(__u32 sel);
__s32 DEU_EBIOS_LP_Set_Mode(__u32 sel, __u32 en_2d);
__s32 DEU_EBIOS_LP_STR_Enable(__u32 sel, __u32 en);
__s32 DEU_EBIOS_LP_Set_STR_Addr(__u32 sel, __u32 address);
__s32 DEU_EBIOS_LP_Set_Para(__u32 sel, __u32 level, __u32 filtertype, __u8 *pttab);
__s32 DEU_EBIOS_LP_Enable(__u32 sel, __u32 en);
#define ____SEPARATOR_DCTI____
__s32 DEU_EBIOS_DCTI_Set_Para(__u32 sel, __u32 level);
__s32 DEU_EBIOS_DCTI_Enable(__u32 sel, __u32 en);
#define ____SEPARATOR_WLE_BLE____
__s32 DEU_EBIOS_WLE_Set_Para(__u32 sel, __u32 level);
__s32 DEU_EBIOS_WLE_Enable(__u32 sel, __u32 en);
__s32 DEU_EBIOS_BLE_Set_Para(__u32 sel, __u32 level);
__s32 DEU_EBIOS_BLE_Enable(__u32 sel, __u32 en);

extern __u8 deu_str_tab[512];
extern __u8 deu_lp_tab_l[5][5][5];
extern __u8 deu_lp_tab_s[5][5][5];

#endif
