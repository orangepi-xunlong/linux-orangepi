/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef __DE_WB_H__
#define __DE_WB_H__
#include "../include.h"

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

s32 WB_EBIOS_Set_Reg_Base(u32 sel, u32 base);
u32 WB_EBIOS_Get_Reg_Base(u32 sel);
s32 WB_EBIOS_Init(struct disp_bsp_init_para *para);
s32 WB_EBIOS_Writeback_Enable(u32 sel, bool en);
s32 WB_EBIOS_Set_Para(u32 sel, struct disp_capture_config *cfg);
s32 WB_EBIOS_Apply(u32 sel, struct disp_capture_config *cfg);
s32 WB_EBIOS_Update_Regs(u32 sel);
u32 WB_EBIOS_Get_Status(u32 sel);
s32 WB_EBIOS_EnableINT(u32 sel);
s32 WB_EBIOS_DisableINT(u32 sel);
u32 WB_EBIOS_QueryINT(u32 sel);
u32 WB_EBIOS_ClearINT(u32 sel);

#endif
