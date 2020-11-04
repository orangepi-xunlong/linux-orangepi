/*
* Allwinner SoCs display driver.
*
* Copyright (C) 2017 Allwinner.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#ifndef _DE_WB_H_
#define _DE_WB_H_

#include "de_rtmx.h"

enum wb_irq_flag {
	WB_IRQ_FLAG_INTR = 1 << 0,
	WB_IRQ_FLAG_MASK = WB_IRQ_FLAG_INTR,
};

enum wb_irq_state {
	WB_IRQ_STATE_PROC_END = 1 << 0,
	WB_IRQ_STATE_FINISH   = 1 << 4,
	WB_IRQ_STATE_OVERFLOW = 1 << 5,
	WB_IRQ_STATE_TIMEOUT  = 1 << 6,
	WB_IRQ_STATE_MASK =
		WB_IRQ_STATE_PROC_END
		| WB_IRQ_STATE_FINISH
		| WB_IRQ_STATE_OVERFLOW
		| WB_IRQ_STATE_TIMEOUT,
};

s32 de_wb_start(u32 wb, u32 wb_mux);
s32 de_wb_stop(u32 wb);
s32 de_wb_writeback_enable(u32 wb, bool en);
s32 de_wb_apply(u32 wb, struct disp_capture_config *cfg);
s32 de_wb_update_regs_ahb(u32 wb);
u32 de_wb_get_status(u32 wb);
s32 de_wb_set_rcq_update(u32 wb, u32 en);
s32 de_wb_set_all_rcq_head_dirty(u32 wb, u32 dirty);

s32 de_wb_init(struct disp_bsp_init_para *para);
s32 de_wb_exit(void);
s32 de_wb_get_reg_blocks(u32 wb,
	struct de_reg_block **blks, u32 *blk_num);
s32 de_wb_enable_irq(u32 wb, u32 en);
s32 de_wb_irq_query_and_clear(u32 wb, enum wb_irq_state id);

void de_wb_rest(u32 wb, u32 reset);

#endif /* #ifndef _DE_WB_H_ */
