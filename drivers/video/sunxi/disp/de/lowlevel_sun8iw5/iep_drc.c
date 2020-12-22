#include "ebios_de.h"
#include "iep_drc_tab.h"
#include "iep_drc.h"

static __drc_t giep[2]; //DRC module parameters
static __drc_pwrsv_t gpwrsv[2]; //Power Saving algorithm parameters
static u32 *pttab[2];   //POINTER of LGC tab
static u32 printf_cnt=0; //for test

//power save core
#define SCENE_CHNG_THR   45 //
#define SCENE_CHANGE_DETECT_DISABLE 1 //enable detetion cause filcker in actual ic test 111230, so disable it.

//#define DRC_DEFAULT_ENABLE  //Enable drc default
//#define DRC_DEMO_HALF_SCREEN //when defined DRC_DEFAULT_ENABLE, run DRC in DEMO mode
static u32 PWRSAVE_PROC_THRES = 85; //when bsp_disp_lcd_get_bright() exceed PWRSAVE_PROC_THRES, STOP PWRSAVE.

#define ____SEPARATOR_DRC_MAIN_TASK____

s32 drc_init(u32 sel)
{
	/* DRC module */
	DRC_EBIOS_Set_Mode(sel, 2);
	DRC_EBIOS_Set_Display_Size(sel, giep[sel].scn_width, giep[sel].scn_height);
	DRC_EBIOS_Drc_Set_Spa_Coeff(sel, spatial_coeff);
	DRC_EBIOS_Drc_Set_Int_Coeff(sel, intensity_coeff);
	DRC_EBIOS_Drc_Adjust_Enable(sel, 0);  //default: no adjust
	DRC_EBIOS_Drc_Set_Lgc_Autoload_Disable(sel, 0); //default: autoload enable
	DRC_EBIOS_Lh_Set_Mode(sel, 0);  //default: histogram normal mode
	DRC_EBIOS_Lh_Set_Thres(sel, hist_thres_pwrsv);

	memset(gpwrsv[sel].min_adj_index_hist, 255, sizeof(u8)*IEP_LH_PWRSV_NUM);

	giep[sel].drc_win_en = 1;
	/*  giep[sel].drc_win.x = 0;
	giep[sel].drc_win.y = 0;
	giep[sel].drc_win.width = scn_width;
	giep[sel].drc_win.height = scn_height;*/ //will clear when drc enable actually, but apps dont know when, so delete it.
	giep[sel].waitframe = 1;  //set 1 to make sure first frame wont get a random lgc table
	giep[sel].runframe = 0;

	return 0;
}

#define ____SEPARATOR_IEP_DRC_CORE____

/*
*********************************************************************************************************
*                                           PWRSAVE_CORE
*
* Description : PoWeRSAVE alg core
*
* Arguments   : sel   <screen index>
*
* Returns         :     0
*
*Note        :    power save mode alg.  Dynamic adjust backlight and lgc gain through screen content and user backlight setting
*                               Add SCENE CHANGE DETECT.
*       Add HANG-UP DETECT: When use PWRSAVE_CORE in LOW referential backlight condiction, backlight will flicker. So STOP use PWRSAVE_CORE.
*
*Date :       11/12/21
**********************************************************************************************************/
static __inline s32 PWRSAVE_CORE(u32 sel)
{
	u32 i;
	u32 hist_region_num = 8;
	u32 histcnt[IEP_LH_INTERVAL_NUM];
	u32 hist[IEP_LH_INTERVAL_NUM], p95;
	u32 size=0;
	u32 min_adj_index;
	u32 lgcaddr;
	u32 drc_filter_tmp=0;
	u32 backlight, thres_low, thres_high;

	backlight = IEP_Drc_Get_Cur_Backlight(sel);

	if(backlight < PWRSAVE_PROC_THRES) {
		/* if current backlight lt PWRSAVE_PROC_THRES, close smart backlight function */
		memset(gpwrsv[sel].min_adj_index_hist, 255, sizeof(u8)*IEP_LH_PWRSV_NUM);
		lgcaddr = (u32)pttab[sel] + ((128-1)<<9);
		lgcaddr = __pa(lgcaddr);
		DRC_EBIOS_Drc_Set_Lgc_Addr(sel, lgcaddr); //set "gain=1" tab to lgc

		IEP_Drc_Set_Backlight_Dimming(sel, 256);
	}	else {
		/* if current backlight ge PWRSAVE_PROC_THRES, open smart backlight function */
		p95=0;

		hist_region_num = (hist_region_num>8)? 8 : IEP_LH_INTERVAL_NUM;

		//read histogram result
		DRC_EBIOS_Lh_Get_Cnt_Rec(sel, histcnt);

		size = giep[sel].scn_width * giep[sel].scn_height / 100;

		/* sometime, hist[hist_region_num - 1] may less than 95 due to integer calc */
		p95 = hist_thres_pwrsv[7];
		//calculate some var
		hist[0] = (histcnt[0])/size;
		if (hist[0] >= 95){
			p95 = hist_thres_pwrsv[0];
		}
		else{
			for (i = 1; i < hist_region_num; i++) {
				hist[i] = (histcnt[i])/size + hist[i-1];
					if (hist[i] >= 95) {
						p95 = hist_thres_pwrsv[i];
						break;
					}
			}
		}

		min_adj_index = p95;

		//__inf("min_adj_index: %d\n", min_adj_index);

#if SCENE_CHANGE_DETECT_DISABLE
		for(i = 0; i <IEP_LH_PWRSV_NUM - 1; i++) {
			gpwrsv[sel].min_adj_index_hist[i] = gpwrsv[sel].min_adj_index_hist[i+1];
		}
		gpwrsv[sel].min_adj_index_hist[IEP_LH_PWRSV_NUM-1] = min_adj_index;

		for (i = 0; i <IEP_LH_PWRSV_NUM; i++) {
//			drc_filter_total += drc_filter[i];
			drc_filter_tmp += drc_filter[i]*gpwrsv[sel].min_adj_index_hist[i];
		}
		min_adj_index = drc_filter_tmp>>10;
#else
		if (abs((s32)min_adj_index - gpwrsv[sel].min_adj_index_hist[IEP_LH_PWRSV_NUM - 1]) > SCENE_CHNG_THR) {
			memset(gpwrsv[sel].min_adj_index_hist, min_adj_index, sizeof(u8)*IEP_LH_PWRSV_NUM);
		}	else {
			//store new gain index, shift history data
			for(i = 0; i <IEP_LH_PWRSV_NUM - 1; i++)
			{
			gpwrsv[sel].min_adj_index_hist[i] = gpwrsv[sel].min_adj_index_hist[i+1];
			}
			gpwrsv[sel].min_adj_index_hist[IEP_LH_PWRSV_NUM-1] = min_adj_index;

			for (i = 0; i <IEP_LH_PWRSV_NUM; i++)
			{
			drc_filter_total += drc_filter[i];
			drc_filter_tmp += drc_filter[i]*gpwrsv[sel].min_adj_index_hist[i];
			}
			min_adj_index = drc_filter_tmp/drc_filter_total;
		}
#endif
		thres_low = PWRSAVE_PROC_THRES;
		thres_high = PWRSAVE_PROC_THRES + ((255 - PWRSAVE_PROC_THRES)/5);
		if(backlight < thres_high) {
			min_adj_index = min_adj_index + (255 - min_adj_index)*(thres_high - backlight)
			    / (thres_high - thres_low);
		}
		min_adj_index = (min_adj_index >= 255)?255:((min_adj_index<hist_thres_pwrsv[0])?hist_thres_pwrsv[0]:min_adj_index);

		IEP_Drc_Set_Backlight_Dimming(sel, min_adj_index+1);

		//lgcaddr = (u32)pwrsv_lgc_tab[min_adj_index-128];
		lgcaddr = (u32)pttab[sel] + ((min_adj_index - 128)<<9);

		if(printf_cnt == 600)	{
			__inf("save backlight power: %d percent\n", (256 - (u32)min_adj_index)*100 / 256);
			printf_cnt = 0;
		}	else {
			printf_cnt++;
		}
		/* virtual to physcal addr */
		lgcaddr = __pa(lgcaddr);
		DRC_EBIOS_Drc_Set_Lgc_Addr(sel, lgcaddr);
	}

	return 0;
}


#define ____SEPARATOR_DRC_PROC____
s32 drc_proc(u32 sel)
{
	u32 top, bot, left, right;
	u32 lgcaddr;
	u32 csc_mode, drc_mode;

	csc_mode = 1;
	drc_mode = 1;

	if(giep[sel].runframe < giep[sel].waitframe) {
		/* first  frame, wont get the valid histogram, so open a "zero" window */
		top = 0;
		bot = 0;
		left = 0;
		right = 0;

		DRC_EBIOS_Drc_Set_Mode(sel, drc_mode);
		DRC_EBIOS_Set_Win_Para(sel, top, bot, left, right);
		DRC_EBIOS_Win_Enable(sel, 1);   //enable here
		DRC_EBIOS_Set_Csc_Coeff(sel, csc_mode);   //12-04-01 debug flicker in LCD opening
		//bsp_disp_set_output_csc(sel, gdisp.screen[sel].output_type, 1); //TBD

		lgcaddr = (u32)pttab[sel] + ((128-1)<<9);
		lgcaddr = __pa(lgcaddr);
		DRC_EBIOS_Drc_Set_Lgc_Addr(sel, lgcaddr); //set "gain=1" tab to lgc
		DRC_EBIOS_Enable(sel);  //enable here
		//DE_INF("waitting for runframe %d up to%d!\n", giep.runframe, giep.waitframe);
		giep[sel].runframe++;
	}	else {
		//DRC_EBIOS_Set_Csc_Coeff(sel, csc_mode); //flicker when mode change??
		if(giep[sel].drc_win_en && giep[sel].drc_win_dirty) {
			/* convert rectangle to register */
			top = giep[sel].drc_win.y;
			bot = giep[sel].drc_win.y + giep[sel].drc_win.height - 1;
			left = giep[sel].drc_win.x;
			right = giep[sel].drc_win.x + giep[sel].drc_win.width - 1;

			DRC_EBIOS_Set_Win_Para(sel, top, bot, left, right);
			giep[sel].drc_win_dirty = 0;
		}
		//BACKLIGHT Control ALG
		//PWRSAVE_CORE(sel);
	}
	DRC_EBIOS_Drc_Cfg_Rdy(sel);
	return 0;
}

s32 drc_close_proc(u32 sel)
{
	/* Disable autoload table */
	DRC_EBIOS_Drc_Set_Lgc_Autoload_Disable(sel, 1); //autoload table disable

	/* DRC module */
	DRC_EBIOS_Disable(sel);
	/* another module */
	//bsp_disp_set_output_csc(sel, gdisp.screen[sel].output_type, 0); //TBD

	IEP_Drc_Set_Backlight_Dimming(sel, 256);

	return 0;
}

s32 IEP_Drc_Tasklet(u32 sel)
{
	PWRSAVE_CORE(sel);
	return 0;
}

#define ____SEPARATOR_DRC_BSP____
s32 IEP_Drc_Init(u32 sel)
{

#ifdef DRC_DEFAULT_ENABLE
	disp_window regn;
#endif
  u32 lcdgamma;
	int  value = 1;
	char primary_key[20];
	int  ret;

	memset(&giep[sel], 0, sizeof(__drc_t));
	memset(&gpwrsv[sel], 0, sizeof(__drc_pwrsv_t));

	pttab[sel] = OSAL_PhyAlloc(IEP_LGC_TAB_SIZE);
	if(pttab[sel] == NULL) {
		__wrn("iep_drc_init, OSAL_PhyAlloc fail\n");
	}

	sprintf(primary_key, "lcd%d_para", sel);

	ret = OSAL_Script_FetchParser_Data(primary_key, "lcdgamma4iep", &value, 1);
	if(ret < 0)	{
		lcdgamma = 6; //default gamma = 2.2;
	} else {
		//DE_INF("lcdgamma4iep for lcd%d = %d.\n", sel, value);
		if(value > 30 || value < 10) {
			//DE_WRN("lcdgamma4iep for lcd%d too small or too large. default value 22 will be set. please set it between 10 and 30 to make it valid.\n",sel);
			lcdgamma = 6;//default gamma = 2.2;
		}	else {
			lcdgamma = (value - 10)/2;
		}
	}

	memcpy(pttab[sel], pwrsv_lgc_tab[128*lcdgamma], IEP_LGC_TAB_SIZE);

	ret = OSAL_Script_FetchParser_Data(primary_key, "smartbl_low_limit", &value, 1);
	if(ret < 0) {
		//DE_INF("smartbl_low_limit for lcd%d not exist.\n", sel);
	}	else {
		//DE_INF("smartbl_low_limit for lcd%d = %d.\n", sel, value);
		if(value > 255 || value < 20)	{
			//DE_INF("smartbl_low_limit for lcd%d too small or too large. default value 85 will be set. please set it between 20 and 255 to make it valid.\n",sel);
		}	else {
			PWRSAVE_PROC_THRES = value;
		}
	}
	IEP_Drc_Update_Backlight(sel, 197);//default backlight
	IEP_Drc_Set_Backlight_Dimming(sel, 256);

	DRC_EBIOS_Drc_Cfg_Rdy(sel);

	return 0;
}

s32 IEP_Drc_Exit(u32 sel)
{
	OSAL_PhyFree(pttab[sel],IEP_LGC_TAB_SIZE);

	return 0;
}

/******************************************************************************/
/*************************IEP_Drc_Enable********************************/
/**开启/关闭自动背光控制功能**************************/
/*****************************************************************************/
//en : 0-close when vbi
//en : 1-open when vbi
//en : 2-close immediately, use when close screen
s32 IEP_Drc_Enable(u32 sel, u32 en)
{
	switch(en) {
	case 0:
		drc_close_proc(sel);
		break;

	case 1:
		drc_init(sel);
		break;
	}
	return 0;

	return 0;
}

/******************************************************************************/
/**************************IEP_Drc_Set_Winodw*************************/
/**设置DRC模式的窗口******************************************/
/******************************************************************************/
s32 IEP_Drc_Set_Winodw(u32 sel, disp_window window)//full screen for default
{
	giep[sel].drc_win_dirty = 1;
	memcpy(&giep[sel].drc_win, &window, sizeof(disp_window));

	return 0;
}

s32 IEP_Drc_Operation_In_Vblanking(u32 sel)
{
	drc_proc(sel);

	return 0;
}

s32 IEP_Drc_Set_Reg_Base(u32 sel, u32 base)
{
	DRC_EBIOS_Set_Reg_Base(sel, base);
	return 0;
}

s32 IEP_Drc_Set_Mode(u32 sel, __iep_drc_mode_t mode)
{
	giep[sel].video_mode_en = mode;

	return 0;
}

s32 IEP_Drc_Get_Cur_Backlight(u32 sel)
{
	return giep[sel].backlight;
}

s32 IEP_Drc_Update_Backlight(u32 sel, u32 backlight)
{
	giep[sel].backlight = backlight;
	return 0;
}

s32 IEP_Drc_Get_Backlight_Dimming(u32 sel)
{
	return giep[sel].backlight_dimming;
}

s32 IEP_Drc_Set_Backlight_Dimming(u32 sel, u32 backlight_dimming)
{
	giep[sel].backlight_dimming = backlight_dimming;

	return 0;
}

s32 IEP_Drc_Early_Suspend(u32 sel)
{
	return 0;
}

s32 iep_drc_suspend(u32 sel)
{
	return 0;
}

s32 iep_drc_resume (u32 sel)
{
	return 0;
}

s32 IEP_Drc_Late_Resume(u32 sel)
{
	return 0;
}

s32 IEP_Drc_Set_Imgsize(u32 sel, u32 width, u32 height)
{
	giep[sel].scn_width = width;
	giep[sel].scn_height = height;

	DRC_EBIOS_Set_Display_Size(sel, giep[sel].scn_width, giep[sel].scn_height);

	return 0;
}

