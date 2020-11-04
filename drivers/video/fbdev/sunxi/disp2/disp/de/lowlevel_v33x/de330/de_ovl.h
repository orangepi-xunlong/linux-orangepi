/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_OVL_H_
#define _DE_OVL_H_

#include <linux/types.h>
#include "de_rtmx.h"

enum {
	DE_OVL_PREMUL_NON_TO_NON = 0,
	DE_OVL_PREMUL_NON_TO_EXC = 1,
	DE_OVL_PREMUL_HAS_PREMUL = 2,
};

s32 de_ovl_init(u32 disp, u8 __iomem *de_reg_base);
s32 de_ovl_exit(u32 disp);
s32 de_ovl_get_reg_blocks(u32 disp,
	struct de_reg_block **blks, u32 *blk_num);

s32 de_ovl_apply_lay(u32 disp, u32 chn,
	struct disp_layer_config_data **const pdata, u32 layer_num);
s32 de_ovl_disable_lay(u32 disp, u32 chn, u32 layer_id);
s32 de_ovl_disable(u32 disp, u32 chn);
void de_ovl_flush_layer_address(u32 disp, u32 chn, u32 layer_id);

#endif /* #ifndef _DE_OVL_H_ */
