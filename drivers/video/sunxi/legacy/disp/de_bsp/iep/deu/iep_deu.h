#ifndef __IEP_DEU_H
#define __IEP_DEU_H
#include "../iep.h"

#define CLK_DEU0_AHB_ON     0x00000001
#define CLK_DEU0_MOD_ON 	0x00000002
#define CLK_DEU0_DRAM_ON    0x00000004
#define CLK_DEU1_AHB_ON     0x00010000
#define CLK_DEU1_MOD_ON 	0x00020000
#define CLK_DEU1_DRAM_ON    0x00040000

#define CLK_DEU0_AHB_OFF	(~(CLK_DEU0_AHB_ON	    ))
#define CLK_DEU0_MOD_OFF 	(~(CLK_DEU0_MOD_ON	    ))
#define CLK_DEU0_DRAM_OFF 	(~(CLK_DEU0_DRAM_ON	    ))
#define CLK_DEU1_AHB_OFF	(~(CLK_DEU1_AHB_ON	    ))
#define CLK_DEU1_MOD_OFF 	(~(CLK_DEU1_MOD_ON	    ))
#define CLK_DEU1_DRAM_OFF 	(~(CLK_DEU1_DRAM_ON	    ))

#define DEU_USED 						0x00000001
#define DEU_USED_MASK 					(~(DEU_USED))
#define DEU_NEED_CLOSED 				0x00000002
#define DEU_NEED_CLOSED_MASK			(~(DEU_NEED_CLOSED))


typedef struct
{
	__disp_frame_info_t frameinfo;
	__u32       width;
	__u32       height;
	__u32 		lumashplvl;
	__u32 		chromashplvl;
	__u32 		wlelvl;
	__u32 		blelvl;
}__deu_t;



__s32 deu_clk_init(__u32 sel);
__s32 deu_clk_exit(__u32 sel);
__s32 deu_clk_open(__u32 sel, __u32 type);
__s32 deu_clk_close(__u32 sel, __u32 type);

__s32 DEU_ALG(__u32 sel);

__s32 IEP_Deu_Enable(__u32 sel, __u32 enable);
__s32 IEP_Deu_Set_Luma_Sharpness_Level(__u32 sel, __u32 level);
__s32 IEP_Deu_Set_Chroma_Sharpness_Level(__u32 sel, __u32 level);
__s32 IEP_Deu_Set_White_Level_Extension(__u32 sel, __u32 level);
__s32 IEP_Deu_Set_Black_Level_Extension(__u32 sel, __u32 level);
__s32 IEP_Deu_Set_Ready(__u32 sel);
__s32 IEP_Deu_Set_Reg_base(__u32 sel, __u32 base);
__s32 IEP_Deu_Set_Winodw(__u32 sel, __disp_rect_t *window);
__s32 IEP_Deu_Output_Select(__u32 sel, __u32 be_ch);
__s32 IEP_Deu_Init(__u32 sel);
__s32 IEP_Deu_Exit(__u32 sel);
__s32 IEP_Deu_Operation_In_Vblanking(__u32 sel);
__s32 iep_deu_early_suspend(__u32 sel);//close clk
__s32 iep_deu_suspend(__u32 sel);//save register
__s32 iep_deu_resume (__u32 sel);//restore register
__s32 iep_deu_late_resume(__u32 sel);//open clk
__s32 IEP_Deu_Set_frameinfo(__u32 sel, __disp_frame_info_t frameinfo);

#endif
