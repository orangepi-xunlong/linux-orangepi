#ifndef _DISP_CURSOR_H_
#define _DISP_CURSOR_H_
#include "bsp_display.h"
#include "disp_private.h"

#define CURSOR_MAX_PALETTE_SIZE 1024
#define CURSOR_MAX_FB_SIZE (64*64*8/8)

s32 disp_cursor_shadow_protect(struct disp_cursor *cursor, bool protect);
s32 disp_init_cursor(__disp_bsp_init_para * para);

#endif

