#ifndef _DISP_SMCL_H_
#define _DISP_SMCL_H_
#include "bsp_display.h"
#include "disp_private.h"

s32 disp_smcl_shadow_protect(struct disp_smcl *smcl, bool protect);
s32 disp_init_smcl(__disp_bsp_init_para * para);
s32 disp_smcl_set_size(struct disp_smcl* smcl, disp_size *size);

#endif

