/*
 * Rafael R848 silicon tuner driver
 *
 * Copyright (C) 2015 Luis Alves <ljalvs@gmail.com>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef R848_PRIV_H
#define R848_PRIV_H

struct r848_priv {
	struct r848_config *cfg;
	struct i2c_adapter *i2c;
	u8 inited;
};




/* R848 */

typedef struct _R848_I2C_LEN_TYPE
{
	u8 RegAddr;
	u8 Data[50];
	u8 Len;
}I2C_LEN_TYPE;

typedef struct _R848_I2C_TYPE
{
	u8 RegAddr;
	u8 Data;
}I2C_TYPE;
//----------------------------------------------------------//
//                   Define                                 //
//----------------------------------------------------------//
#define FOR_TDA10024    0

#define VERSION   "R848_GUI_2.3A"

#define R848_REG_NUM         40
#define R848_TF_HIGH_NUM  8  
#define R848_TF_MID_NUM    8
#define R848_TF_LOW_NUM   8
#define R848_TF_LOWEST_NUM   8
#define R848_RING_POWER_FREQ_LOW  115000
#define R848_RING_POWER_FREQ_HIGH 450000
#define R848_IMR_IF              5300         
#define R848_IMR_TRIAL       9
#define R848_Xtal	                   16000
//----------------------------------------------------------//
//                   Internal Structure                            //
//----------------------------------------------------------//


typedef struct _R848_Freq_Info_Type
{
	u8		RF_POLY;
	u8		LNA_BAND;
	u8		LPF_CAP;
	u8		LPF_NOTCH;
    u8		XTAL_POW0;
	u8		CP_CUR;
	u8		IMR_MEM;
	u8		Q_CTRL;   
}R848_Freq_Info_Type;

typedef struct _R848_SysFreq_Info_Type
{
	u8		LNA_TOP;
	u8		LNA_VTH_L;
	u8		MIXER_TOP;
	u8		MIXER_VTH_L;
	u8       RF_TOP;
	u8       NRB_TOP;
	u8       NRB_BW;
	u8       BYP_LPF;
	u8       RF_FAST_DISCHARGE;
	u8       RF_SLOW_DISCHARGE;
	u8       RFPD_PLUSE_ENA;
	u8       LNA_FAST_DISCHARGE;
	u8       LNA_SLOW_DISCHARGE;
	u8       LNAPD_PLUSE_ENA;
	u8       AGC_CLK;

}R848_SysFreq_Info_Type;

typedef struct _R848_Cal_Info_Type
{
	u8		FILTER_6DB;
	u8		MIXER_AMP_GAIN;
	u8		MIXER_BUFFER_GAIN;
	u8		LNA_GAIN;
	u8		LNA_POWER;
	u8		RFBUF_OUT;
	u8		RFBUF_POWER;
	u8		TF_CAL;
}R848_Cal_Info_Type;

typedef struct _R848_SectType
{
	u8   Phase_Y;
	u8   Gain_X;
	u8   Iqcap;
	u8   Value;
}R848_SectType;

typedef struct _R848_TF_Result
{
	u8   TF_Set;
	u8   TF_Value;
}R848_TF_Result;

typedef enum _R848_TF_Band_Type
{
    TF_HIGH = 0,
	TF_MID,
	TF_LOW
}R848_TF_Band_Type;

typedef enum _R848_TF_Type
{
	R848_TF_NARROW = 0,             //270n/68n   (ISDB-T, DVB-T/T2)
	R848_TF_BEAD,                   //Bead/68n   (DTMB)
	R848_TF_NARROW_LIN,             //270n/68n   (N/A)
	R848_TF_NARROW_ATV_LIN,		//270n/68n   (ATV)
	R848_TF_BEAD_LIN,               //Bead/68n   (PAL_DK for China Hybrid TV)
	R848_TF_NARROW_ATSC,		//270n/68n   (ATSC, DVB-C, J83B)
	R848_TF_BEAD_LIN_ATSC,		//Bead/68n   (ATSC, DVB-C, J83B)
	R848_TF_82N_BEAD,		//Bead/82n   (DTMB)
	R848_TF_82N_270N,		//270n/82n   (OTHER Standard)
	R848_TF_SIZE
} R848_TF_Type;



u32 R848_LNA_HIGH_MID[R848_TF_SIZE] = { 644000, 644000, 644000, 644000, 644000, 500000, 500000, 500000, 500000}; 
u32 R848_LNA_MID_LOW[R848_TF_SIZE] = { 388000, 388000, 348000, 348000, 348000, 300000, 300000, 300000, 300000};
u32 R848_LNA_LOW_LOWEST[R848_TF_SIZE] = {164000, 164000, 148000, 124000, 124000, 156000, 156000, 108000, 108000};


u32 R848_TF_Freq_High[R848_TF_SIZE][R848_TF_HIGH_NUM] = 
{  	 { 784000, 784000, 776000, 744000, 712000, 680000, 648000, 647000},
	 { 784000, 784000, 776000, 744000, 712000, 680000, 648000, 647000},
	 { 784000, 784000, 776000, 744000, 712000, 680000, 648000, 647000},
	 { 784000, 784000, 776000, 744000, 712000, 680000, 648000, 647000},
	 { 784000, 784000, 776000, 744000, 712000, 680000, 648000, 647000},
     { 784000, 784000, 776000, 680000, 608000, 584000, 536000, 504000},
	 { 784000, 784000, 776000, 680000, 608000, 584000, 536000, 504000},
	 { 784000, 776000, 712000, 616000, 584000, 560000, 520000, 504000},
	 { 784000, 776000, 712000, 616000, 584000, 560000, 520000, 504000}
};


u32 R848_TF_Freq_Mid[R848_TF_SIZE][R848_TF_MID_NUM] = 
{	  {608000, 584000, 560000, 536000, 488000, 440000, 416000, 392000},
	  {608000, 584000, 560000, 536000, 488000, 440000, 416000, 392000},
	  {608000, 560000, 536000, 488000, 440000, 392000, 376000, 352000},
	  {608000, 560000, 536000, 488000, 440000, 392000, 376000, 352000},
	  {608000, 560000, 536000, 488000, 440000, 392000, 376000, 352000},
      {488000, 464000, 440000, 416000, 392000, 352000, 320000, 304000},
	  {488000, 464000, 440000, 416000, 392000, 352000, 320000, 304000},
	  {480000, 464000, 440000, 416000, 392000, 352000, 320000, 304000},
	  {480000, 464000, 440000, 416000, 392000, 352000, 320000, 304000},
};
u32 R848_TF_Freq_Low[R848_TF_SIZE][R848_TF_LOW_NUM] = 
{    {320000, 304000, 272000, 240000, 232000, 216000, 192000, 168000},  //164~388
      {320000, 304000, 272000, 240000, 232000, 216000, 192000, 168000},  //164~388
	  {256000, 240000, 232000, 224000, 216000, 192000, 168000, 160000},  //148~348
	  {256000, 240000, 232000, 192000, 160000, 152000, 144000, 128000},  //124~348
	  {264000, 240000, 192000, 184000, 176000, 152000, 144000, 128000},  //124~348
      {280000, 248000, 232000, 216000, 192000, 176000, 168000, 160000},  //156~300
      {280000, 248000, 232000, 216000, 192000, 176000, 168000, 160000},   //156~300
	  {296000, 280000, 256000, 216000, 184000, 168000, 136000, 112000},   //
	  {296000, 280000, 256000, 216000, 184000, 168000, 136000, 112000}   //
};
u32 R848_TF_Freq_Lowest[R848_TF_SIZE][R848_TF_LOWEST_NUM] = 
{    {160000, 120000, 104000, 88000, 80000, 72000, 56000, 48000},
      {160000, 120000, 104000, 88000, 80000, 72000, 56000, 48000},
	  {144000, 120000, 104000, 88000, 80000, 72000, 56000, 48000},
	  {120000, 96000,   88000,   80000, 72000, 64000, 56000, 48000},
	  {104000, 96000,   88000,   80000, 72000, 64000, 56000, 48000},
	  {136000, 120000, 104000, 88000, 72000, 64000, 56000, 48000},
	  {128000, 120000, 104000, 96000, 80000, 72000, 64000, 56000},
	  {104000, 96000, 88000, 80000, 72000, 64000, 56000, 48000},
	  {104000, 96000, 88000, 80000, 72000, 64000, 56000, 48000}
};

u8 R848_TF_Result_High[R848_TF_SIZE][R848_TF_HIGH_NUM] = 
{    {0x00, 0x00, 0x01, 0x03, 0x04, 0x05, 0x07, 0x07},
      {0x00, 0x00, 0x01, 0x03, 0x04, 0x05, 0x07, 0x07},
	  {0x00, 0x00, 0x01, 0x03, 0x04, 0x05, 0x07, 0x07},
	  {0x00, 0x00, 0x01, 0x03, 0x04, 0x05, 0x07, 0x07},
	  {0x00, 0x00, 0x01, 0x03, 0x04, 0x05, 0x07, 0x07},
      {0x00, 0x00, 0x01, 0x05, 0x0A, 0x0C, 0x13, 0x19},
	  {0x00, 0x00, 0x01, 0x05, 0x0A, 0x0C, 0x13, 0x19},
	  {0x00, 0x03, 0x07, 0x0C, 0x0E, 0x0F, 0x1A, 0x1A},
	  {0x00, 0x03, 0x07, 0x0C, 0x0E, 0x0F, 0x1A, 0x1A}
};

u8 R848_TF_Result_Mid[R848_TF_SIZE][R848_TF_MID_NUM] = 
{    {0x00, 0x01, 0x03, 0x03, 0x06, 0x0B, 0x0E, 0x11},
      {0x00, 0x01, 0x03, 0x03, 0x06, 0x0B, 0x0E, 0x11},
	  {0x00, 0x03, 0x03, 0x06, 0x0B, 0x11, 0x12, 0x19},  
	  {0x00, 0x03, 0x03, 0x06, 0x0B, 0x11, 0x12, 0x19},  
	  {0x00, 0x03, 0x03, 0x06, 0x0B, 0x11, 0x12, 0x19},
	  {0x06, 0x08, 0x0B, 0x0E, 0x13, 0x17, 0x1E, 0x1F},
      {0x06, 0x08, 0x0B, 0x0E, 0x13, 0x17, 0x1E, 0x1F},
	  {0x09, 0x0D, 0x10, 0x12, 0x16, 0x1B, 0x1E, 0x1F},
	  {0x09, 0x0D, 0x10, 0x12, 0x16, 0x1B, 0x1E, 0x1F}
};
u8 R848_TF_Result_Low[R848_TF_SIZE][R848_TF_LOW_NUM] = 
{    {0x00, 0x02, 0x04, 0x07, 0x0A, 0x0B, 0x0F, 0x16},
      {0x00, 0x02, 0x04, 0x07, 0x0A, 0x0B, 0x0F, 0x16},
	  {0x05, 0x07, 0x0A, 0x0B, 0x0B, 0x0F, 0x16, 0x1A},
	  {0x05, 0x07, 0x0A, 0x0F, 0x1A, 0x1A, 0x23, 0x2A},
	  {0x05, 0x08, 0x10, 0x13, 0x1A, 0x1A, 0x23, 0x2A},
	  {0x05, 0x08, 0x0C, 0x0E, 0x10, 0x14, 0x18, 0x1A},
	  {0x05, 0x08, 0x0C, 0x0E, 0x10, 0x14, 0x18, 0x1A},
	  {0x00, 0x01, 0x03, 0x07, 0x0D, 0x11, 0x1E, 0x2F},
	  {0x00, 0x01, 0x03, 0x07, 0x0D, 0x11, 0x1E, 0x2F}
};
u8 R848_TF_Result_Lowest[R848_TF_SIZE][R848_TF_LOWEST_NUM] = 
{    {0x00, 0x06, 0x0C, 0x15, 0x1C, 0x1F, 0x3C, 0x3F},
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x08},
	  {0x02, 0x06, 0x0C, 0x15, 0x1C, 0x1F, 0x3C, 0x3F},
	  {0x06, 0x11, 0x15, 0x1C, 0x1F, 0x2F, 0x3C, 0x3F},
      {0x04, 0x08, 0x08, 0x08, 0x10, 0x12, 0x13, 0x13},
	  {0x06, 0x09, 0x0E, 0x18, 0x25, 0x2F, 0x3C, 0x3F},
	  {0x00, 0x04, 0x04, 0x08, 0x08, 0x10, 0x12, 0x13},
	  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x08},
	  {0x0E, 0x14, 0x18, 0x1E, 0x25, 0x2F, 0x3C, 0x3F}
};


u8 R848_iniArray_hybrid[R848_REG_NUM] = {
						0x00, 0x00, 0x40, 0x44, 0x17, 0x00, 0x06, 0xF0, 0x00, 0x41,
					//  0x08  0x09  0x0A  0x0B  0x0C  0x0D  0x0E  0x0F  0x10  0x11
						0x7B, 0x0B, 0x70, 0x06, 0x6E, 0x20, 0x70, 0x87, 0x96, 0x00,
					//  0x12  0x13  0x14  0x15  0x16  0x17  0x18  0x19  0x1A  0x1B  
						0x10, 0x00, 0x80, 0xA5, 0xB7, 0x00, 0x40, 0xCB, 0x95, 0xF0,
					//  0x1C  0x1D  0x1E  0x1F  0x20  0x21  0x22  0x23  0x24  0x25
						0x24, 0x00, 0xFD, 0x8B, 0x17, 0x13, 0x01, 0x07, 0x01, 0x3F};
					//  0x26  0x27  0x28  0x29  0x2A  0x2B  0x2C  0x2D  0x2E  0x2F



u8 R848_iniArray_dvbs[R848_REG_NUM] = {
						0x80, 0x05, 0x40, 0x40, 0x1F, 0x1F, 0x07, 0xFF, 0x00, 0x40,
					//  0x08  0x09  0x0A  0x0B  0x0C  0x0D  0x0E  0x0F  0x10  0x11
						0xF0, 0x0F, 0x4D, 0x06, 0x6F, 0x20, 0x28, 0x83, 0x96, 0x00,  //0x16[1] pulse_flag HPF : Bypass ;  0x19[1:0] Deglich SW Cur : highest
					//  0x12  0x13  0x14  0x15  0x16  0x17  0x18  0x19  0x1A  0x1B   
						0x1C, 0x99, 0xC1, 0x83, 0xB7, 0x00, 0x4F, 0xCB, 0x95, 0xFD,
					//  0x1C  0x1D  0x1E  0x1F  0x20  0x21  0x22  0x23  0x24  0x25
						0xA4, 0x01, 0x24, 0x0B, 0x4F, 0x05, 0x01, 0x47, 0x3F, 0x3F};
					//  0x26  0x27  0x28  0x29  0x2A  0x2B  0x2C  0x2D  0x2E  0x2F

R848_SectType R848_IMR_Data[5] = {
									{0, 0, 0, 0},
									{0, 0, 0, 0},
									{0, 0, 0, 0},
									{0, 0, 0, 0},
									{0, 0, 0, 0}
									};//Please keep this array data for standby mode.






typedef enum _R848_UL_TF_Type
{
	R848_UL_USING_BEAD = 0,            
    R848_UL_USING_270NH,                      
}R848_UL_TF_Type;



typedef enum _R848_Cal_Type
{
	R848_IMR_CAL = 0,
	R848_IMR_LNA_CAL,
	R848_TF_CAL,
	R848_TF_LNA_CAL,
	R848_LPF_CAL,
	R848_LPF_LNA_CAL
}R848_Cal_Type;

typedef enum _R848_BW_Type
{
	BW_6M = 0,
	BW_7M,
	BW_8M,
	BW_1_7M,
	BW_10M,
	BW_200K
}R848_BW_Type;


enum R848_XTAL_PWR_VALUE
{
	XTAL_SMALL_LOWEST = 0,
    XTAL_SMALL_LOW,
    XTAL_SMALL_HIGH,
    XTAL_SMALL_HIGHEST,
    XTAL_LARGE_HIGHEST,
	XTAL_CHECK_SIZE
};


typedef enum _R848_Xtal_Div_TYPE
{
	XTAL_DIV1 = 0,
	XTAL_DIV2
}R848_Xtal_Div_TYPE;


//----------------------------------------------------------//
//                   R848 Public Parameter                     //
//----------------------------------------------------------//
typedef enum _R848_ErrCode
{
	RT_Success = 0,
	RT_Fail    = 1
}R848_ErrCode;

typedef enum _R848_Standard_Type  //Don't remove standand list!!
{
	R848_DVB_T_6M = 0,
	R848_DVB_T_7M,
	R848_DVB_T_8M, 
    R848_DVB_T2_6M,			//IF=4.57M
	R848_DVB_T2_7M,			//IF=4.57M
	R848_DVB_T2_8M,			//IF=4.57M
	R848_DVB_T2_1_7M,
	R848_DVB_T2_10M,
	R848_DVB_C_8M,
	R848_DVB_C_6M, 
	R848_J83B,
	R848_ISDB_T,             //IF=4.063M
	R848_ISDB_T_4570,		 //IF=4.57M
	R848_DTMB_4570,			 //IF=4.57M
	R848_DTMB_6000,			 //IF=6.00M
	R848_DTMB_6M_BW_IF_5M,   //IF=5.0M, BW=6M
	R848_DTMB_6M_BW_IF_4500, //IF=4.5M, BW=6M
	R848_ATSC,
	R848_DVB_S,
	R848_DVB_T_6M_IF_5M,
	R848_DVB_T_7M_IF_5M,
	R848_DVB_T_8M_IF_5M,
	R848_DVB_T2_6M_IF_5M,
	R848_DVB_T2_7M_IF_5M,
	R848_DVB_T2_8M_IF_5M,
	R848_DVB_T2_1_7M_IF_5M,
	R848_DVB_C_8M_IF_5M,
	R848_DVB_C_6M_IF_5M, 
	R848_J83B_IF_5M,
	R848_ISDB_T_IF_5M,            
	R848_DTMB_IF_5M,     
	R848_ATSC_IF_5M,
	R848_FM,
	R848_STD_SIZE,
}R848_Standard_Type;

typedef enum _R848_GPO_Type
{
	HI_SIG = 0,
	LO_SIG = 1
}R848_GPO_Type;

typedef enum _R848_RF_Gain_TYPE
{
	RF_AUTO = 0,
	RF_MANUAL
}R848_RF_Gain_TYPE;

typedef enum _R848_DVBS_OutputSignal_Type
{
	DIFFERENTIALOUT = 0,
	SINGLEOUT     = 1
}R848_DVBS_OutputSignal_Type;

typedef enum _R848_DVBS_AGC_Type
{
	AGC_NEGATIVE = 0,
	AGC_POSITIVE = 1
}R848_DVBS_AGC_Type;



typedef struct _R848_Set_Info
{
	u32                       RF_KHz;
	u32						 DVBS_BW;
	R848_Standard_Type           R848_Standard;
	R848_DVBS_OutputSignal_Type  R848_DVBS_OutputSignal_Mode;
	R848_DVBS_AGC_Type           R848_DVBS_AGC_Mode; 
}R848_Set_Info;

typedef struct _R848_RF_Gain_Info
{
	u16   RF_gain_comb;
	u8   RF_gain1;
	u8   RF_gain2;
	u8   RF_gain3;
}R848_RF_Gain_Info;




















#endif /* R848_PRIV_H */
