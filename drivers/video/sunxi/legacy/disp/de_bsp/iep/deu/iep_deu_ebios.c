//*****************************************************************************
//  All Winner Micro, All Right Reserved. 2006-2012 Copyright (c)
//
//  File name   :        iep_deu_ebios.c
//
//  Description :  Image Enhancement Processor Detail Enhancement Unit registers and interface functions define
//                 for A1x
//  History     :
//                2011/05/13      vito       v0.1    Initial version in A10 in de_fe.c
//				  2012/06/12      vito       v0.5    Initial version for iep-deu in A1x
//******************************************************************************

#include "iep_deu_ebios.h"

static __iep_deu_dev_t *deu_dev[2];

static __u32 deu_csc_tab[48] =
{
	//Y/G   Y/G      Y/G      Y/G      U/R      U/R     U/R        U/R     V/B      V/B       V/B       V/B
	//bt601
	0x04a8,0x1e70,0x1cbf,0x0878,0x04a8,0x0000,0x0662,0x3211,0x04a8,0x0812,0x0000,0x2eb1,//yuv2rgb
	//bt709
	0x04a7,0x1f25,0x1ddd,0x04cf,0x04a7,0x0000,0x072c,0x307d,0x04a7,0x0875,0x0000,0x2dea,//yuv2rgb
	//DISP_YCC
	0x0400,0x1e9e,0x1d24,0x087b,0x0400,0x0000,0x059b,0x34c8,0x0400,0x0715,0x0000,0x31d4,//yuv2rgb
	//xvYCC
	0x04a7,0x1f25,0x1ddd,0x04cf,0x04a7,0x0000,0x072c,0x307d,0x04a7,0x0875,0x0000,0x2dea//yuv2rgb
};

__u8 deu_str_tab[512] =
{
	//strspatab[256]=
	1,4,8,12,16,20,24,28,31,35,39,43,46,50,53,57,60,64,67,71,74,
	78,81,84,87,91,94,97,100,103,106,109,112,115,118,121,124,127,130,133,135,138,
	141,143,146,149,151,154,156,159,161,164,166,168,171,173,175,178,180,182,184,
	186,188,190,192,194,196,198,200,202,204,205,207,209,211,212,214,215,217,219,
	220,222,223,224,226,227,228,230,231,232,233,235,236,237,238,239,240,241,242,
	243,244,244,245,246,247,248,248,249,250,250,251,251,252,252,253,253,253,254,
	254,254,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,254,
	254,254,253,253,253,252,252,251,251,250,250,249,248,248,247,246,245,244,244,
	243,242,241,240,239,238,237,236,235,233,232,231,230,228,227,226,224,223,222,
	220,219,217,215,214,212,211,209,207,205,204,202,200,198,196,194,192,190,188,
	186,184,182,180,178,175,173,171,168,166,164,161,159,156,154,151,149,146,143,141,
	138,135,133,130,127,124,121,118,115,112,109,106,103,100,97,94,91,87,84,81,78,74,
	71,67,64,60,57,53,50,46,43,39,35,31,28,24,20,16,12,8,4,1,	//tab in y(x) = 1-(2*(x-0.5))^2

	//strinttab[256]=
	1,64,76,84,91,96,100,104,108,111,114,117,119,122,124,126,
	128,130,132,134,135,137,139,140,142,143,145,146,147,149,150,151,152,153,155,156,
	157,158,159,160,161,162,163,164,165,166,167,168,168,169,170,171,172,173,173,174,
	175,176,177,177,178,179,180,180,181,182,182,183,184,184,185,186,186,187,188,188,
	189,190,190,191,191,192,193,193,194,194,195,195,196,196,197,198,198,199,199,200,
	200,201,201,202,202,203,203,204,204,205,205,206,206,207,207,208,208,209,209,209,
	210,210,211,211,212,212,213,213,213,214,214,215,215,216,216,216,217,217,218,218,
	218,219,219,220,220,220,221,221,222,222,222,223,223,223,224,224,225,225,225,226,
	226,226,227,227,227,228,228,229,229,229,230,230,230,231,231,231,232,232,232,233,
	233,233,234,234,234,235,235,235,236,236,236,236,237,237,237,238,238,238,239,239,
	239,240,240,240,240,241,241,241,242,242,242,243,243,243,243,244,244,244,245,245,
	245,245,246,246,246,247,247,247,247,248,248,248,248,249,249,249,250,250,250,250,
	251,251,251,251,252,252,252,252,253,253,253,253,254,254,254,254,255,255,255,255	//tab in y(x) = x^(1/4)

};

//large LCD panel (10 inch above)
__u8 deu_lp_tab_l[5][5][5] =
{
	//beta, tau, alpha, lmt, corthr
	{{0	,0	,0	,0	,0 },{3	,3	,0	,5	,5 },{6	,6	,0	,5	,5 },{7	,7	,2	,5	,5 },{8	,8	,4	,5	,5 }},
	{{0	,0	,0	,0	,0 },{4	,4	,0	,5	,5 },{8	,8	,0	,5	,5 },{8	,8	,4	,8	,5 },{8	,8	,8	,10	,5 }},
	{{0	,0	,0	,0	,0 },{4	,4	,4	,5	,10},{8	,8	,8	,10	,10},{12,12	,12	,12	,10},{16,16	,16	,15	,10}},
	{{0	,0	,0	,0	,0 },{5	,5	,5	,0	,10},{10,10	,10	,0	,10},{12,12	,12	,0	,10},{16,16	,16	,0	,10}},
	{{0	,0	,0	,0	,0 },{6	,6	,6	,0	,10},{12,12	,12	,0	,10},{16,16	,16	,0	,10},{20,20	,20	,0	,10}}
};

//small LCD panel (10 inch below)
__u8 deu_lp_tab_s[5][5][5] =
{
	//beta, tau, alpha, lmt, corthr
	{{0	,0	,0	,0	,0 },{0	,0	,0	,0	,0 },{3	,3	,0	,5	,5 },{4	,4	,0	,5	,5 },{6	,6	,0	,5	,5 }},
	{{0	,0	,0	,0	,0 },{4	,4	,0	,5	,5 },{8	,8	,0	,5	,5 },{12,12	,0	,5	,5 },{16,16	,0	,5	,5 }},
	{{0	,0	,0	,0	,0 },{4	,4	,4	,5	,10},{8	,8	,8	,10	,10},{12,12	,12 ,10 ,10},{16,16 ,16 ,10	,10}},
	{{0	,0	,0	,0	,0 },{5	,5	,5	,0	,10},{10,10	,10	,0	,10},{15,15	,15	,0	,10},{20,20	,20	,0	,10}},
	{{0	,0	,0	,0	,0 },{6	,6	,6	,0	,10},{12,12	,12	,0	,10},{18,18 ,18	,0	,10},{24,24	,24	,0	,10}}
};


#define ____SEPARATOR_GLOBAL____


__s32 DEU_EBIOS_Set_Reg_Base(__u32 sel, __u32 base)
{
	deu_dev[sel] = (__iep_deu_dev_t *)base;
	return 0;
}

__u32 DEU_EBIOS_Get_Reg_Base(__u32 sel)
{
	__u32 ret = 0;

	ret = (__u32)deu_dev[sel];
	return ret;
}

__s32 DEU_EBIOS_Enable(__u32 sel, __u32 en)
{
	deu_dev[sel]->en.bits.en = en;
	return 0;
}

__s32 DEU_EBIOS_Csc_Enable(__u32 sel, __u32 en)
{
	deu_dev[sel]->en.bits.csc_en = en;
	return 0;
}

__s32 DEU_EBIOS_Set_Csc_Coeff(__u32 sel, __u32 mode)
{
	__u32 *ptcsctab;

	mode = (mode>3)?3:mode;

	ptcsctab = &(deu_csc_tab[mode*12]);

	deu_dev[sel]->cscgcoff[0].bits.csc_g_coff = *ptcsctab;
	deu_dev[sel]->cscgcoff[1].bits.csc_g_coff = *(ptcsctab+1);
	deu_dev[sel]->cscgcoff[2].bits.csc_g_coff = *(ptcsctab+2);
	deu_dev[sel]->cscgcon.bits.csc_g_con =	    *(ptcsctab+3);
	deu_dev[sel]->cscrcoff[0].bits.csc_r_coff = *(ptcsctab+4);
	deu_dev[sel]->cscrcoff[1].bits.csc_r_coff = *(ptcsctab+5);
	deu_dev[sel]->cscrcoff[2].bits.csc_r_coff = *(ptcsctab+6);
	deu_dev[sel]->cscrcon.bits.csc_r_con =	    *(ptcsctab+7);
	deu_dev[sel]->cscbcoff[0].bits.csc_b_coff = *(ptcsctab+8);
	deu_dev[sel]->cscbcoff[1].bits.csc_b_coff = *(ptcsctab+9);
	deu_dev[sel]->cscbcoff[2].bits.csc_b_coff = *(ptcsctab+10);
	deu_dev[sel]->cscbcon.bits.csc_b_con =	    *(ptcsctab+11);

	return 0;

}

__s32 DEU_EBIOS_Set_Display_Size(__u32 sel, __u32 width, __u32 height)
{
	deu_dev[sel]->psize.bits.width = width - 1;
	deu_dev[sel]->psize.bits.height = height - 1;

	return 0;
}

__s32 DEU_EBIOS_Set_Win_Para(__u32 sel, __u32 top, __u32 bot, __u32 left, __u32 right)
{

	deu_dev[sel]->pwp0.bits.win_left = left;
	deu_dev[sel]->pwp0.bits.win_top = top;
	deu_dev[sel]->pwp1.bits.win_right = right;
	deu_dev[sel]->pwp1.bits.win_bot = bot;
	return 0;

}

__s32 DEU_EBIOS_Cfg_Rdy(__u32 sel)
{
	deu_dev[sel]->en.bits.reg_rdy = 1;
	return 0;
}

__s32 DEU_EBIOS_Set_Output_Chnl(__u32 sel, __u32 be_sel)
{
	deu_dev[sel]->en.bits.out_port_sel = be_sel;
	return 0;
}

#define ____SEPARATOR_LP____
__s32 DEU_EBIOS_LP_STR_Cfg_Rdy(__u32 sel)
{
	deu_dev[sel]->en.bits.coeff_rdy = 1;
	return 0;
}

__s32 DEU_EBIOS_LP_Set_Mode(__u32 sel, __u32 en_2d)
{
	deu_dev[sel]->lp1.bits.lp_mode = en_2d;
	return 0;
}

__s32 DEU_EBIOS_LP_STR_Enable(__u32 sel, __u32 en)
{
	deu_dev[sel]->lp1.bits.str = en;
	return 0;
}

__s32 DEU_EBIOS_LP_Set_STR_Addr(__u32 sel, __u32 address)
{
	deu_dev[sel]->straddr.bits.straddr = address-0x40000000;
	return 0;
}

__s32 DEU_EBIOS_LP_Set_Para(__u32 sel, __u32 level, __u32 filtertype, __u8 *pttab)
{
	deu_dev[sel]->lp1.bits.neggain = 3;
	deu_dev[sel]->lp1.bits.delta = 3;
	deu_dev[sel]->lp0.bits.beta = *(pttab + filtertype * 25 + level *5 + 0);
	deu_dev[sel]->lp0.bits.tau = *(pttab + filtertype * 25 + level *5 + 1);
	deu_dev[sel]->lp0.bits.alpha = *(pttab + filtertype * 25 + level *5 + 2);
	deu_dev[sel]->lp1.bits.limit_thr = *(pttab + filtertype * 25 + level *5 + 3);
	deu_dev[sel]->lp1.bits.corthr = *(pttab + filtertype * 25 + level *5 + 4);

	return 0;
}

__s32 DEU_EBIOS_LP_Enable(__u32 sel, __u32 en)
{
	deu_dev[sel]->lp0.bits.lp_en = en;
	return 0;
}

#define ____SEPARATOR_DCTI____
__s32 DEU_EBIOS_DCTI_Set_Para(__u32 sel, __u32 level)
{
	deu_dev[sel]->dcti.bits.dcti_filter1_sel = 2;
	deu_dev[sel]->dcti.bits.dcti_filter2_sel = 2;
	deu_dev[sel]->dcti.bits.dcti_hill_en = 1;
	deu_dev[sel]->dcti.bits.dcti_suphill_en = 1;
	deu_dev[sel]->dcti.bits.uv_separate_en = 0;
	deu_dev[sel]->dcti.bits.uv_same_sign_mode_sel = 3;
	deu_dev[sel]->dcti.bits.uv_diff_sign_mode_sel = 3;

	switch(level) {
	case	0x0:
		deu_dev[sel]->dcti.bits.dcti_gain = 0;
		deu_dev[sel]->dcti.bits.dcti_path_limit = 0;
		deu_dev[sel]->dcti.bits.uv_same_sign_maxmin_mode_sel = 0;
		deu_dev[sel]->dcti.bits.uv_diff_sign_maxmin_mode_sel = 0;
		break;

	case	0x1:
		deu_dev[sel]->dcti.bits.dcti_gain = 12;
		deu_dev[sel]->dcti.bits.dcti_path_limit = 4;
		deu_dev[sel]->dcti.bits.uv_same_sign_maxmin_mode_sel = 0;
		deu_dev[sel]->dcti.bits.uv_diff_sign_maxmin_mode_sel = 0;
		break;

	case	0x2:
		deu_dev[sel]->dcti.bits.dcti_gain = 23;
		deu_dev[sel]->dcti.bits.dcti_path_limit = 4;
		deu_dev[sel]->dcti.bits.uv_same_sign_maxmin_mode_sel = 0;
		deu_dev[sel]->dcti.bits.uv_diff_sign_maxmin_mode_sel = 0;
		break;

	case	0x3:
		deu_dev[sel]->dcti.bits.dcti_gain = 23;
		deu_dev[sel]->dcti.bits.dcti_path_limit = 4;
		deu_dev[sel]->dcti.bits.uv_same_sign_maxmin_mode_sel = 1;
		deu_dev[sel]->dcti.bits.uv_diff_sign_maxmin_mode_sel = 0;
		break;

	case	0x4:
		deu_dev[sel]->dcti.bits.dcti_gain = 32;
		deu_dev[sel]->dcti.bits.dcti_path_limit = 5;
		deu_dev[sel]->dcti.bits.uv_same_sign_maxmin_mode_sel = 1;
		deu_dev[sel]->dcti.bits.uv_diff_sign_maxmin_mode_sel = 1;
		break;

	default:
		deu_dev[sel]->dcti.bits.dcti_gain = 0;
		deu_dev[sel]->dcti.bits.dcti_path_limit = 0;
		deu_dev[sel]->dcti.bits.uv_same_sign_maxmin_mode_sel = 0;
		deu_dev[sel]->dcti.bits.uv_diff_sign_maxmin_mode_sel = 0;
		break;
	}

	return 0;
}

__s32 DEU_EBIOS_DCTI_Enable(__u32 sel, __u32 en)
{
	deu_dev[sel]->dcti.bits.dcti_en = en;

	return 0;
}


#define ____SEPARATOR_WLE_BLE____
__s32 DEU_EBIOS_WLE_Set_Para(__u32 sel, __u32 level)
{
	deu_dev[sel]->wle.bits.wle_thr = 128;

	switch(level) {
	case	0x0:
		deu_dev[sel]->wle.bits.wle_gain = 64;
		break;

	case	0x1:
		deu_dev[sel]->wle.bits.wle_gain = 73;
		break;

	case	0x2:
		deu_dev[sel]->wle.bits.wle_gain = 79;
		break;

	case	0x3:
		deu_dev[sel]->wle.bits.wle_gain = 92;
		break;

	case	0x4:
		deu_dev[sel]->wle.bits.wle_gain = 127;
		break;

	default:
		deu_dev[sel]->wle.bits.wle_gain = 64;
		break;
	}

	return 0;
}

__s32 DEU_EBIOS_WLE_Enable(__u32 sel, __u32 en)
{
	deu_dev[sel]->wle.bits.wle_en = en;

	return 0;

}

__s32 DEU_EBIOS_BLE_Set_Para(__u32 sel, __u32 level)
{
	deu_dev[sel]->ble.bits.ble_thr = 127;

	switch(level) {
	case	0x0:
		deu_dev[sel]->ble.bits.ble_gain = 0;
		break;

	case	0x1:
		deu_dev[sel]->ble.bits.ble_gain = 9;
		break;

	case	0x2:
		deu_dev[sel]->ble.bits.ble_gain = 16;
		break;

	case	0x3:
		deu_dev[sel]->ble.bits.ble_gain = 32;
		break;

	case	0x4:
		deu_dev[sel]->ble.bits.ble_gain = 64;
		break;

	default:
		deu_dev[sel]->ble.bits.ble_gain = 0;
		break;
	}

	return 0;
}

__s32 DEU_EBIOS_BLE_Enable(__u32 sel, __u32 en)
{
	deu_dev[sel]->ble.bits.ble_en = en;
	return 0;
}

