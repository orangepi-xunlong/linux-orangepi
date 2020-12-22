#ifndef __IEP_DEU_H
#define __IEP_DEU_H
#include "ebios_de.h"
#include "iep_drc_ebios.h"

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
	disp_frame_info frameinfo;
	u32     width;
	u32     height;
	u32 		lumashplvl;
	u32 		chromashplvl;
	u32 		wlelvl;
	u32 		blelvl;
}__deu_t;

s32 DEU_ALG(u32 sel);

s32 IEP_Deu_Enable(u32 sel, u32 enable);
s32 IEP_Deu_Set_Luma_Sharpness_Level(u32 sel, u32 level);
s32 IEP_Deu_Set_Chroma_Sharpness_Level(u32 sel, u32 level);
s32 IEP_Deu_Set_White_Level_Extension(u32 sel, u32 level);
s32 IEP_Deu_Set_Black_Level_Extension(u32 sel, u32 level);
s32 IEP_Deu_Set_Ready(u32 sel);
s32 IEP_Deu_Set_Reg_base(u32 sel, u32 base);
s32 IEP_Deu_Set_Winodw(u32 sel, disp_window *window);
s32 IEP_Deu_Output_Select(u32 sel, u32 be_ch);
s32 IEP_Deu_Init(u32 sel);
s32 IEP_Deu_Exit(u32 sel);
s32 IEP_Deu_Operation_In_Vblanking(u32 sel);
s32 iep_deu_suspend(u32 sel);//save register
s32 iep_deu_resume (u32 sel);//restore register

s32 IEP_Deu_Set_frameinfo(u32 sel, disp_frame_info frameinfo);

#endif
