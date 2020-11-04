/*
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _DI300_H_
#define _DI300_H_

#include <linux/types.h>


enum di_irq_flag {
	DI_IRQ_FLAG_PROC_FINISH = 0x1 << 0,
	DI_IRQ_FLAG_MASK =
		DI_IRQ_FLAG_PROC_FINISH,
};

enum di_irq_state {
	DI_IRQ_STATE_PROC_FINISH  = 0x1 << 0,
	DI_IRQ_STATE_MASK =
		DI_IRQ_STATE_PROC_FINISH,
};

void di_dev_set_reg_base(void __iomem *reg_base);
void di_dev_exit(void);

s32 di_dev_apply_fixed_para(void *client);
s32 di_dev_apply_para(void *client);
s32 di_dev_get_proc_result(void *client);
s32 di_dev_save_spot(void *client);
s32 di_dev_restore_spot(void *client);
s32 di_dev_enable_irq(u32 irq_flag, u32 en);
u32 di_dev_query_state_with_clear(u32 irq_state);
void di_dev_start(u32 start);
void di_dev_reset(void);

u32 di_dev_get_cdata_size(void);
u32 di_dev_reset_cdata(void *dev_cdata);

/* works after di_dev_enable??? */
u32 di_dev_get_ip_version(void);

void di_dev_dump_reg_value(void);


#endif /* #ifndef _DI300_H_ */
