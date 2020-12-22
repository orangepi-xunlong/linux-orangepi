#include "iep_deu.h"
#include "iep_deu_ebios.h"

extern __u8 deu_str_tab[512];
extern __u8 deu_lp_tab_s[5][5][5];
extern __u8 deu_lp_tab_l[5][5][5];

static __deu_t gdeu[2];	//DRC module parameters
static u32 *g_strtab_addr;

static __u8 *plptab;

#define ____SEPARATOR_DEU_ALG____
s32 DEU_ALG(u32 sel)
{
	static disp_frame_info frameinfo;
	u32 lpmode, dctimode;
	u32 scalefact, filtertype = 0;
	u32 deuwidth, deuheight;
	{
		static u32 count = 0;
		count++;
		if(count == 300) {
			count = 0;
			if(gdeu[sel].frameinfo.in_size.width == 0) {
				__wrn("DEU_ALG, input width is ZERO\n");
			}
			//pr_warn("<<deu-I>>\n");
			//pr_warn("frameinfo.disp_size: <%dx%d>\n", gdeu[sel].frameinfo.disp_size.width,  gdeu[sel].frameinfo.disp_size.height);
			//pr_warn("[%d,%d,%d,%d]\n", gdeu[sel].lumashplvl, gdeu[sel].chromashplvl, gdeu[sel].wlelvl, gdeu[sel].blelvl);
		}
	}

	if((frameinfo.disp_size.width != gdeu[sel].frameinfo.disp_size.width)
	    || (frameinfo.disp_size.height != gdeu[sel].frameinfo.disp_size.height)) {
		//pr_warn("frameinfo.disp_size: <%dx%d>\n", gdeu[sel].frameinfo.disp_size.width,  gdeu[sel].frameinfo.disp_size.height);
	}
	memcpy(&frameinfo, &gdeu[sel].frameinfo, sizeof(disp_frame_info));

	if(frameinfo.b_trd_out == 1 && frameinfo.trd_out_mode == DISP_3D_OUT_MODE_LIRGB) {
		deuwidth = frameinfo.disp_size.width*2;
		deuheight = frameinfo.disp_size.height/2;
	}
	/*
	else if(frameinfo.b_interlace_out)
	{
	deuwidth = frameinfo.disp_size.width;
	deuheight = frameinfo.disp_size.height/2;
	}
	*/
	else {
		deuwidth = frameinfo.disp_size.width;
		deuheight = frameinfo.disp_size.height;
	}

	//1D/2D/DISABLE of LP and ENABLE/DISABLE DCTI
	if((frameinfo.trd_out_mode == DISP_3D_OUT_MODE_CI_1  || frameinfo.trd_out_mode == 	DISP_3D_OUT_MODE_CI_2  ||
	    frameinfo.trd_out_mode == DISP_3D_OUT_MODE_CI_3  || frameinfo.trd_out_mode ==  DISP_3D_OUT_MODE_CI_4) &&
	    frameinfo.b_trd_out == 1)	{
		lpmode = 0;	//disable
		dctimode = 0;	//disable
	}	else if((( frameinfo.trd_out_mode == DISP_3D_OUT_MODE_FA ) &&
	    frameinfo.b_trd_out == 1)|| frameinfo.b_interlace_out == 1 || deuwidth > 2048) {
		lpmode = 1;	//1d lp
		dctimode = 1;	//1d dcti
	}	else {
		lpmode = 2;	//2d lp
		dctimode = 1;	//1d dcti
	}

	if(frameinfo.in_size.width == 0) {
		frameinfo.in_size.width = 2;
		//__wrn("DEU_ALG, input width is ZERO\n");
	}
	scalefact = (frameinfo.out_size.width<<2)/frameinfo.in_size.width;  //scale factor X4

	if (scalefact<5) {
		filtertype = 0;
	}	else if(scalefact>=5 && scalefact<7) {
		filtertype = 1;
	}	else if(scalefact>=7 && scalefact<9) {
		filtertype = 2;
	}	else if(scalefact>=9 && scalefact<11) {
		filtertype = 3;
	}	else if(scalefact>=11) {
		filtertype = 4;
	}
	/* set reg */
	DEU_EBIOS_Set_Display_Size(sel, deuwidth, deuheight);
	DEU_EBIOS_LP_Enable(sel, ((lpmode==0)||(gdeu[sel].lumashplvl==0))?0:1);
	DEU_EBIOS_LP_Set_Mode(sel, lpmode-1);
	DEU_EBIOS_DCTI_Enable(sel, ((dctimode==0)||(gdeu[sel].chromashplvl==0))?0:1);

	DEU_EBIOS_LP_Set_Para(sel, gdeu[sel].lumashplvl, filtertype, plptab);
	DEU_EBIOS_DCTI_Set_Para(sel, gdeu[sel].chromashplvl);

	return 0;
}

#define ____SEPARATOR_DEU_BSP____
//enable: 0 disable
//          1 enable
s32 IEP_Deu_Enable(u32 sel, u32 enable)
{
	u32 strtab_addr;

	__inf("IEP_Deu_Enable, sel=%d, enable=%d\n", sel, enable);
	strtab_addr =(u32)g_strtab_addr;
	if(enable == 1) {
		DEU_EBIOS_Enable(sel, 1);

		__inf("vir_addr:0x%08x, phy_addr:0x%08x\n", strtab_addr, (u32)(__pa(strtab_addr)));

		/* virtual to physcal addr */
		strtab_addr = __pa(strtab_addr);

		DEU_EBIOS_LP_Set_STR_Addr(sel, strtab_addr);
		DEU_EBIOS_LP_STR_Enable(sel, true);
		DEU_EBIOS_LP_STR_Cfg_Rdy(sel);
		DEU_EBIOS_Set_Csc_Coeff(sel, gdeu[sel].frameinfo.csc_mode);
		DEU_EBIOS_Csc_Enable(sel, 1);
	}	else {
		DEU_EBIOS_Csc_Enable(sel, 0);
		DEU_EBIOS_Enable(sel, 0);

		IEP_Deu_Operation_In_Vblanking(sel);
	}

	return 0;
}

s32 IEP_Deu_Set_Luma_Sharpness_Level(u32 sel, u32 level)
{
	gdeu[sel].lumashplvl = level;

	return 0;
}

s32 IEP_Deu_Set_Chroma_Sharpness_Level(u32 sel, u32 level)
{
	gdeu[sel].chromashplvl = level;

	return 0;
}

s32 IEP_Deu_Set_White_Level_Extension(u32 sel, u32 level)
{
	DEU_EBIOS_WLE_Set_Para(sel,level);
	DEU_EBIOS_WLE_Enable(sel, (level==0)?0:1);

	gdeu[sel].wlelvl = level;

	return 0;
}

s32 IEP_Deu_Set_Black_Level_Extension(u32 sel, u32 level)
{
	DEU_EBIOS_BLE_Set_Para(sel,level);
	DEU_EBIOS_BLE_Enable(sel, (level==0)?0:1);

	gdeu[sel].blelvl = level;

	return 0;
}

s32 IEP_Deu_Set_Ready(u32 sel)
{
	DEU_EBIOS_Cfg_Rdy(sel);

	return 0;
}

s32 IEP_Deu_Set_Reg_base(u32 sel, u32 base)
{
	DEU_EBIOS_Set_Reg_Base(sel, base);

	return 0;
}

s32 IEP_Deu_Set_Winodw(u32 sel, disp_window *window)
{
	u32 top, bot, left, right;

	__inf("IEP_Deu_Set_Winodw, sel=%d, [%d,%d,%d,%d]\n", sel, window->x, window->y, window->width, window->height);

	//convert rectangle to register
	top = window->y;
	bot = window->y + window->height - 1;
	left = window->x;
	right = window->x + window->width - 1;

	DEU_EBIOS_Set_Win_Para(sel, top, bot, left, right);

	return 0;
}

s32 IEP_Deu_Output_Select(u32 sel, u32 be_ch)
{
	DEU_EBIOS_Set_Output_Chnl(sel, be_ch);

	return 0;
}

s32 IEP_Deu_Init(u32 sel)
{
	int ret;
	int value = 1;
	char primary_key[20];

	g_strtab_addr = (u32 *)kmalloc(512, GFP_KERNEL | __GFP_ZERO);
	memcpy(g_strtab_addr, deu_str_tab, 512);

	sprintf(primary_key, "lcd%d_para", sel);
	ret = OSAL_Script_FetchParser_Data(primary_key, "deu_mode", &value, 1);
	if(ret < 0) {
		__inf("deu_mode%d not exist.\n", sel);
		plptab = &deu_lp_tab_s[0][0][0];
	}	else {
		__inf("deu_mode%d = %d.\n", sel, value);
		if(value > 1 || value < 0) {
			__wrn("deu_mode%d invalid.\n",sel);
			plptab = &deu_lp_tab_s[0][0][0];
		}	else {
			plptab = (value == 1)? (&deu_lp_tab_l[0][0][0]):(&deu_lp_tab_s[0][0][0]);
		}
	}

	return 0;
}

s32 IEP_Deu_Exit(u32 sel)
{
	kfree((void*)g_strtab_addr);

	return 0;
}

s32 IEP_Deu_Operation_In_Vblanking(u32 sel)
{
	//function about setting level through frameinfo
	DEU_ALG(sel);
	DEU_EBIOS_Cfg_Rdy(sel);

	return 0;
}

s32 IEP_Deu_Early_Suspend(u32 sel);//close clk

s32 iep_deu_suspend(u32 sel)//save register
{
	return 0;
}

s32 iep_deu_resume (u32 sel)//restore register
{
	return 0;
}


s32 IEP_Deu_Late_Resume(u32 sel);//open clk

s32 IEP_Deu_Set_frameinfo(u32 sel, disp_frame_info frameinfo)
{
	if((frameinfo.disp_size.width != gdeu[sel].frameinfo.disp_size.width)
	    || (frameinfo.disp_size.height != gdeu[sel].frameinfo.disp_size.height)) {
	//pr_warn("IEP_Deu_Set_frameinfo,   frameinfo.disp_size: <%dx%d>\n", gdeu[sel].frameinfo.disp_size.width,  gdeu[sel].frameinfo.disp_size.height);
	}

	memcpy(&gdeu[sel].frameinfo, &frameinfo, sizeof(disp_frame_info));

	return 0;
}
