#ifndef __DISP_IEP_H_
#define __DISP_IEP_H_

#include "disp_display_i.h"
#include "disp_display.h"


#define DRC_USED 						0x04000000
#define DRC_USED_MASK 					(~(DRC_USED))
#define CMU_SCREEN_EN                   0x10000000
#define CMU_SCREEN_EN_MASK              (~(CMU_SCREEN_EN))
#define CMU_LAYER_EN                    0x20000000
#define CMU_LAYER_EN_MASK               (~(CMU_LAYER_EN))


__s32 disp_drc_start_video_mode(__u32 screen_id);
__s32 disp_drc_start_ui_mode(__u32 screen_id);
__s32 disp_deu_set_frame_info(__u32 screen_id, __u32 hid);
__s32 disp_cmu_layer_clear(__u32 screen_id);
__s32 disp_deu_clear(__u32 screen_id, __u32 hid);
__s32 disp_deu_output_select(__u32 screen_id, __u32 hid, __u32 ch);
__s32 bsp_disp_drc_get_input_csc(__u32 screen_id);
__s32 disp_deu_get_support(__u32 screen_id);
__s32 disp_drc_get_support(__u32 screen_id);

#endif
