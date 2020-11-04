/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_CDC_H_
#define _DE_CDC_H_

#include <linux/types.h>
#include "de_top.h"
#include "de_csc_table.h"

s32 de_cdc_set_para(void *cdc_hdl,
	struct de_csc_info *in_info, struct de_csc_info *out_info);
s32 de_cdc_disable(void *cdc_hdl);

void *de_cdc_create(u8 __iomem *reg_base, u32 rcq_used, u8 has_csc);
s32 de_cdc_destroy(void *cdc_hdl);
s32 de_cdc_get_reg_blks(void *cdc_hdl,
	struct de_reg_block **blks, u32 *blk_num);

#endif /* #ifndef _DE_VSU_H_ */
