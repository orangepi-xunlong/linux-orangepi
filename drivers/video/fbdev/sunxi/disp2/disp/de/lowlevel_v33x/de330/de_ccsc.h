/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_CCSC_H_
#define _DE_CCSC_H_

#include <linux/types.h>
#include "de_top.h"

s32 de_ccsc_enable(u32 disp, u32 chn, u8 en);
s32 de_ccsc_apply(u32 disp, u32 chn,
	struct de_csc_info *in_info, struct de_csc_info *out_info);
s32 de_ccsc_set_alpha(u32 disp, u32 chn, u8 en, u8 alpha);

s32 de_ccsc_init(u32 disp, u8 __iomem *de_reg_base);
s32 de_ccsc_exit(u32 disp);
s32 de_ccsc_get_reg_blocks(u32 disp,
	struct de_reg_block **blks, u32 *blk_num);

#endif /* #ifndef _DE_CCSC_H_ */
