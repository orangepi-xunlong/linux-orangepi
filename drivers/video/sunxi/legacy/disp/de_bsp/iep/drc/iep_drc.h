#ifndef __IEP_DRC_H__
#define __IEP_DRC_H__
#include "../iep.h"
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
  __u32       mod;

	//drc
	//__u32           drc_en;
	__u32       drc_win_en;
	__disp_rect_t   drc_win;
	__u32           adjust_en;
	__u32           lgc_autoload_dis;
	__u32           waitframe;
	__u32           runframe;
	__u32           scn_width;
	__u32           scn_height;
	__u32           video_mode_en;

	//lh
	__u32           lgc_base_add;
	//__u8            lh_thres_val[IEP_LH_THRES_NUM];
	__u8            lh_thres_val[7];

	//de-flicker
	//__u32           deflicker_en;
	__u32           deflicker_win_en;
	__disp_rect_t   deflicker_win;
}__drc_t;

typedef struct
{
	__u8      min_adj_index_hist[IEP_LH_PWRSV_NUM];
	__u32           user_bl;
}__drc_pwrsv_t;


__s32 drc_clk_init(__u32 sel);
__s32 drc_clk_exit(__u32 sel);
__s32 drc_clk_open(__u32 sel, __u32 type);
__s32 drc_clk_close(__u32 sel, __u32 type);
__s32 drc_enable(__u32 sel, __u32 en);
__s32 drc_init(__u32 sel);
__s32 drc_proc(__u32 sel);
__s32 drc_close_proc(__u32 sel);

__s32 IEP_Drc_Init(__u32 sel);
__s32 IEP_Drc_Exit(__u32 sel);
__s32 IEP_Drc_Enable(__u32 sel, __u32 en);
__s32 IEP_Drc_Operation_In_Vblanking(__u32 sel);
__s32 IEP_Drc_Set_Reg_Base(__u32 sel, __u32 base);
__s32 IEP_Drc_Get_Reg_Base(__u32 sel);
__s32 IEP_Drc_Set_Winodw(__u32 sel, __disp_rect_t window);//full screen for default
__s32 IEP_Drc_Set_Mode(__u32 sel, __iep_drc_mode_t mode);
__s32 IEP_Drc_Early_Suspend(__u32 sel);//close clk
__s32 iep_drc_suspend(__u32 sel);//save register
__s32 iep_drc_resume (__u32 sel);//restore register
__s32 IEP_Drc_Late_Resume(__u32 sel);//open clk
__s32 IEP_Drc_Set_Imgsize(__u32 sel, __u32 width, __u32 height);

extern __u16 pwrsv_lgc_tab[1408][256];
extern __u8 spatial_coeff[9];
extern __u8 intensity_coeff[256];
extern __u8 hist_thres_drc[8];
extern __u8 hist_thres_pwrsv[8];
extern __u8 drc_filter[IEP_LH_PWRSV_NUM];

#endif

