
#ifndef __EBSP_IEP_H__
#define __EBSP_IEP_H__

#include "../bsp_display.h"

typedef enum
{
	IEP_DRC_MODE_UI,
	IEP_DRC_MODE_VIDEO,
}__iep_drc_mode_t;

typedef struct
{
	bool                  b_interlace_out;
	bool                  b_trd_out;
	disp_3d_out_mode     trd_out_mode;
	disp_size         in_size; //single size
	disp_size         out_size;//single size
	disp_size         disp_size;//full size out
	disp_cs_mode        csc_mode;
}__disp_frame_info_t;

//DRC
extern __s32 drc_clk_open(__u32 sel,__u32 type);
extern __s32 drc_clk_close(__u32 sel,__u32 type);
extern __s32 IEP_Drc_Init(__u32 sel);
extern __s32 IEP_Drc_Exit(__u32 sel);
extern __s32 IEP_Drc_Enable(__u32 sel, __u32 en);
extern __s32 IEP_Drc_Operation_In_Vblanking(__u32 sel);
extern __s32 IEP_Drc_Tasklet(u32 sel);
extern __s32 IEP_Drc_Set_Reg_Base(__u32 sel, __u32 base);
extern __s32 IEP_Drc_Get_Reg_Base(__u32 sel);
extern __s32 IEP_Drc_Set_Winodw(__u32 sel, disp_window window);//full screen for default
extern __s32 IEP_Drc_Set_Mode(__u32 sel, __iep_drc_mode_t mode);
extern __s32 iep_drc_early_suspend(__u32 sel);//close clk
extern __s32 iep_drc_suspend(__u32 sel);//save register
extern __s32 iep_drc_resume (__u32 sel);//restore register
extern __s32 iep_drc_late_resume(__u32 sel);//open clk
extern __s32 IEP_Drc_Set_Imgsize(__u32 sel, __u32 width, __u32 height);
extern __s32 drc_enable(__u32 sel, __u32 en);
extern s32 IEP_Drc_Update_Backlight(u32 sel, u32 backlight);
extern s32 IEP_Drc_Get_Backlight_Dimming(u32 sel);

//drc wb
extern __s32 WB_EBIOS_Set_Reg_Base(__u32 sel, __u32 base);
extern __u32 WB_EBIOS_Get_Reg_Base(__u32 sel);
extern __s32 WB_EBIOS_Enable(__u32 sel);
extern __s32 WB_EBIOS_Reset(__u32 sel);
extern __s32 WB_EBIOS_Set_Reg_Rdy(__u32 sel);
extern __s32 WB_EBIOS_Set_Capture_Mode(__u32 sel);
extern __s32 WB_EBIOS_Writeback_Enable(__u32 sel);
extern __s32 WB_EBIOS_Writeback_Disable(__u32 sel);
extern __s32 WB_EBIOS_Set_Para(__u32 sel,  disp_size insize, disp_window cropwin, disp_window outwin, disp_fb_info outfb);
extern __s32 WB_EBIOS_Set_Addr(__u32 sel, __u32 addr[3]);
extern __u32 WB_EBIOS_Get_Status(__u32 sel);
extern __s32 WB_EBIOS_EnableINT(__u32 sel);
extern __s32 WB_EBIOS_DisableINT(__u32 sel);
extern __u32 WB_EBIOS_QueryINT(__u32 sel);
extern __u32 WB_EBIOS_ClearINT(__u32 sel);
extern __s32 WB_EBIOS_Init(__u32 sel);

//sat
extern __s32 SAT_Set_Reg_Base(__u32 sel, __u32 base);
extern __s32 SAT_Enable_Disable( __u32 sel,__u32 en);
extern __s32 SAT_Set_Display_Size( __u32 sel,__u32 width,__u32 height);
extern __s32 SAT_Set_Window(__u32 sel, disp_window *window);
extern __s32 SAT_Set_Sync_Para(__u32 sel);


#endif
