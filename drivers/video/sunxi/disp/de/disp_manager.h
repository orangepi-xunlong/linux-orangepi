
#ifndef __DISP_MANAGER_H__
#define __DISP_MANAGER_H__

#include "disp_private.h"

s32 disp_init_mgr(__disp_bsp_init_para * para);
s32 disp_mgr_shadow_protect(struct disp_manager *mgr, bool protect);

#endif
