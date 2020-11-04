/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DISP_AL_DE_H_
#define _DISP_AL_DE_H_

#include "../include.h"
#include "de330/de_rtmx.h"
#include "de330/de_wb.h"

enum {
	DISP_AL_IRQ_FLAG_FRAME_END  = DE_IRQ_FLAG_FRAME_END,
	DISP_AL_IRQ_FLAG_RCQ_FINISH = DE_IRQ_FLAG_RCQ_FINISH,
	DISP_AL_IRQ_FLAG_RCQ_ACCEPT = DE_IRQ_FLAG_RCQ_ACCEPT,
	DISP_AL_IRQ_FLAG_MASK       = DE_IRQ_FLAG_MASK,
};

enum  {
	DISP_AL_IRQ_STATE_FRAME_END  = DE_IRQ_STATE_FRAME_END,
	DISP_AL_IRQ_STATE_RCQ_FINISH = DE_IRQ_STATE_RCQ_FINISH,
	DISP_AL_IRQ_STATE_RCQ_ACCEPT = DE_IRQ_STATE_RCQ_ACCEPT,
	DISP_AL_IRQ_STATE_MASK = DE_IRQ_STATE_MASK,
};

enum {
	DISP_AL_CAPTURE_IRQ_FLAG_FRAME_END = WB_IRQ_FLAG_INTR,
	DISP_AL_CAPTURE_IRQ_FLAG_RCQ_ACCEPT = DE_WB_IRQ_FLAG_RCQ_ACCEPT,
	DISP_AL_CAPTURE_IRQ_FLAG_RCQ_FINISH = DE_WB_IRQ_FLAG_RCQ_FINISH,
	DISP_AL_CAPTURE_IRQ_FLAG_MASK =
		DISP_AL_CAPTURE_IRQ_FLAG_FRAME_END
		| DISP_AL_CAPTURE_IRQ_FLAG_RCQ_ACCEPT
		| DISP_AL_CAPTURE_IRQ_FLAG_RCQ_FINISH,
};

enum {
	DISP_AL_CAPTURE_IRQ_STATE_FRAME_END = WB_IRQ_STATE_PROC_END,
	DISP_AL_CAPTURE_IRQ_STATE_RCQ_ACCEPT = DE_WB_IRQ_STATE_RCQ_ACCEPT,
	DISP_AL_CAPTURE_IRQ_STATE_RCQ_FINISH = DE_WB_IRQ_STATE_RCQ_FINISH,
	DISP_AL_CAPTURE_IRQ_STATE_MASK =
		DISP_AL_CAPTURE_IRQ_STATE_FRAME_END
		| DISP_AL_CAPTURE_IRQ_STATE_RCQ_ACCEPT
		| DISP_AL_CAPTURE_IRQ_STATE_RCQ_FINISH,
};

s32 disp_al_manager_init(u32 disp);
s32 disp_al_manager_exit(u32 disp);
s32 disp_al_manager_apply(u32 disp,
	struct disp_manager_data *data);
s32 disp_al_layer_apply(u32 disp,
	struct disp_layer_config_data *data, u32 layer_num);
s32 disp_init_al(struct disp_bsp_init_para *para);
s32 disp_al_manager_sync(u32 disp);
s32 disp_al_manager_update_regs(u32 disp);
s32 disp_al_manager_set_rcq_update(u32 disp, u32 en);
s32 disp_al_manager_set_all_rcq_head_dirty(u32 disp, u32 dirty);
s32 disp_al_manager_set_irq_enable(u32 disp, u32 irq_flag, u32 en);
u32 disp_al_manager_query_irq_state(u32 disp, u32 irq_state);

s32 disp_al_enhance_apply(u32 disp,
	struct disp_enhance_config *config);
s32 disp_al_enhance_update_regs(u32 disp);
s32 disp_al_enhance_sync(u32 disp);
s32 disp_al_enhance_tasklet(u32 disp);

s32 disp_al_smbl_apply(u32 disp, struct disp_smbl_info *info);
s32 disp_al_smbl_update_regs(u32 disp);
s32 disp_al_smbl_sync(u32 disp);
s32 disp_al_smbl_get_status(u32 disp);
s32 disp_al_smbl_tasklet(u32 disp);

s32 disp_al_capture_init(u32 disp);
s32 disp_al_capture_exit(u32 disp);
s32 disp_al_capture_sync(u32 disp);
s32 disp_al_capture_apply(u32 disp, struct disp_capture_config *cfg);
s32 disp_al_capture_get_status(u32 disp);
s32 disp_al_capture_set_rcq_update(u32 disp, u32 en);
s32 disp_al_capture_set_all_rcq_head_dirty(u32 disp, u32 dirty);
s32 disp_al_capture_set_irq_enable(u32 disp, u32 irq_flag, u32 en);
u32 disp_al_capture_query_irq_state(u32 disp, u32 irq_state);

s32 disp_al_get_fb_info(u32 disp, struct disp_layer_info *info);
s32 disp_al_get_display_size(u32 disp, u32 *width,
	u32 *height);
int disp_exit_al(void);
bool disp_al_get_direct_show_state(unsigned int disp);
s32 disp_al_capture_set_mode(u32 disp, enum de_rtwb_mode mode);

void disp_al_flush_layer_address(u32 disp, u32 chn, u32 layer_id);

#endif /* #ifndef _DISP_AL_DE_H_ */
