/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_FBD_ATW_H_
#define _DE_FBD_ATW_H_

#include <linux/types.h>
#include "de_rtmx.h"

s32 de_fbd_atw_apply_lay(u32 disp, u32 chn,
	struct disp_layer_config_data **const pdata, u32 layer_num);
s32 de_fbd_atw_disable(u32 disp, u32 chn);

s32 de_fbd_atw_init(u32 disp, u8 __iomem *de_reg_base);
s32 de_fbd_atw_exit(u32 disp);
s32 de_fbd_atw_get_reg_blocks(u32 disp,
	struct de_reg_block **blks, u32 *blk_num);

#endif /* #ifndef _DE_OVL_H_ */
