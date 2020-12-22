#ifndef __IEP_DRC_H__
#define __IEP_DRC_H__
#include "ebios_de.h"
#include "iep_drc_ebios.h"

#define CLK_DRC0_AHB_ON     0x00000001
#define CLK_DRC0_MOD_ON   0x00000002
#define CLK_DRC0_DRAM_ON    0x00000004
#define CLK_DRC1_AHB_ON     0x00010000
#define CLK_DRC1_MOD_ON   0x00020000
#define CLK_DRC1_DRAM_ON    0x00040000

#define CLK_DRC0_AHB_OFF  (~(CLK_DRC0_AHB_ON      ))
#define CLK_DRC0_MOD_OFF  (~(CLK_DRC0_MOD_ON      ))
#define CLK_DRC0_DRAM_OFF   (~(CLK_DRC0_DRAM_ON     ))
#define CLK_DRC1_AHB_OFF  (~(CLK_DRC1_AHB_ON      ))
#define CLK_DRC1_MOD_OFF  (~(CLK_DRC1_MOD_ON      ))
#define CLK_DRC1_DRAM_OFF   (~(CLK_DRC1_DRAM_ON     ))

#define DE_FLICKER_USED         0x01000000
#define DE_FLICKER_USED_MASK      (~(DE_FLICKER_USED))
#define DE_FLICKER_REQUIRED       0x02000000
#define DE_FLICKER_REQUIRED_MASK    (~(DE_FLICKER_REQUIRED))
#define DRC_USED            0x04000000
#define DRC_USED_MASK           (~(DRC_USED))
#define DRC_REQUIRED          0x08000000
#define DRC_REQUIRED_MASK         (~(DRC_REQUIRED))
#define DE_FLICKER_NEED_CLOSED      0x10000000
#define DE_FLICKER_NEED_CLOSED_MASK   (~(DE_FLICKER_NEED_CLOSED))
#define DRC_NEED_CLOSED         0x20000000
#define DRC_NEED_CLOSED_MASK      (~(DRC_NEED_CLOSED))

//for power saving mode alg0
#define IEP_LH_PWRSV_NUM 24
#define IEP_LGC_TAB_SIZE 92160  //(256(GAMMA/LEVEL)*180(LEVEL)*2(BYTE))
typedef struct
{
  u32       mod;

	//drc
	//u32           drc_en;
	u32       drc_win_en;
	disp_window   drc_win;
	u32           drc_win_dirty;
	u32           adjust_en;
	u32           lgc_autoload_dis;
	u32           waitframe;
	u32           runframe;
	u32           scn_width;
	u32           scn_height;
	u32           video_mode_en;
	u32           backlight;
	u32           backlight_dimming;

	//lh
	u32           lgc_base_add;
	//u8            lh_thres_val[IEP_LH_THRES_NUM];
	u8            lh_thres_val[7];

	//de-flicker
	//u32           deflicker_en;
	u32           deflicker_win_en;
	disp_window   deflicker_win;
}__drc_t;

typedef struct
{
	u8      min_adj_index_hist[IEP_LH_PWRSV_NUM];
	u32           user_bl;
}__drc_pwrsv_t;


s32 drc_clk_init(u32 sel);
s32 drc_clk_exit(u32 sel);
s32 drc_clk_open(u32 sel, u32 type);
s32 drc_clk_close(u32 sel, u32 type);
s32 drc_enable(u32 sel, u32 en);
s32 drc_init(u32 sel);
s32 drc_proc(u32 sel);
s32 drc_close_proc(u32 sel);

s32 IEP_Drc_Init(u32 sel);
s32 IEP_Drc_Exit(u32 sel);
s32 IEP_Drc_Enable(u32 sel, u32 en);
s32 IEP_Drc_Operation_In_Vblanking(u32 sel);
s32 IEP_Drc_Tasklet(u32 sel);
s32 IEP_Drc_Set_Reg_Base(u32 sel, u32 base);
s32 IEP_Drc_Get_Reg_Base(u32 sel);
s32 IEP_Drc_Set_Winodw(u32 sel, disp_window window);//full screen for default
s32 IEP_Drc_Set_Mode(u32 sel, __iep_drc_mode_t mode);
s32 IEP_Drc_Early_Suspend(u32 sel);
s32 iep_drc_suspend(u32 sel);//save register
s32 iep_drc_resume (u32 sel);//restore register
s32 IEP_Drc_Late_Resume(u32 sel);
s32 IEP_Drc_Set_Imgsize(u32 sel, u32 width, u32 height);
s32 IEP_Drc_Get_Cur_Backlight(u32 sel);
s32 IEP_Drc_Update_Backlight(u32 sel, u32 backlight);
s32 IEP_Drc_Get_Backlight_Dimming(u32 sel);
s32 IEP_Drc_Set_Backlight_Dimming(u32 sel, u32 backlight_dimming);

extern u16 pwrsv_lgc_tab[1408][256];
extern u8 spatial_coeff[9];
extern u8 intensity_coeff[256];
extern u8 hist_thres_drc[8];
extern u8 hist_thres_pwrsv[8];
extern u8 drc_filter[IEP_LH_PWRSV_NUM];


#endif

