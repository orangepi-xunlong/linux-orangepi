
#ifndef __DISP_CAPTURE_H__
#define __DISP_CAPTURE_H__

#include "disp_private.h"

s32 disp_init_capture(__disp_bsp_init_para *para);
extern s32 disp_al_capture_init(u32 screen_id);
extern s32 disp_al_capture_sync(u32 screen_id);

#endif
