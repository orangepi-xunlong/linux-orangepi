/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2022 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_CRC_H_
#define _DE_CRC_H_

#include <linux/types.h>
#include "de_top.h"

enum de_crc_status {
	DE_CRC_REGION0 = 0,
	DE_CRC_REGION1 = 4,
	DE_CRC_REGION2 = 8,
	DE_CRC_REGION3 = 12,
	DE_CRC_REGION4 = 16,
	DE_CRC_REGION5 = 20,
	DE_CRC_REGION6 = 24,
	DE_CRC_REGION7 = 28,
};

s32 de_crc_init(u32 disp, u8 __iomem *reg_base);
s32 de_crc_exit(u32 disp);
s32 de_crc_get_reg_blocks(u32 disp, struct de_reg_block **blks, u32 *blk_num);
s32 de_crc_set_size(u32 disp, u32 width, u32 height);
enum de_crc_status de_crc_get_status(u32 disp);
s32 de_crc_enable(u32 disp, u32 region, u32 enable);
s32 de_crc_set_polarity(u32 disp, u32 region, u32 polarity);
s32 de_crc_set_win(u32 disp, u32 region, u32 x_start, u32 x_end, u32 y_start, u32 y_end);
s32 de_crc_set_size(u32 disp, u32 width, u32 height);
s32 de_crc_set_compare_step(u32 disp, u32 region, u32 step);
s32 de_crc_set_run_frames(u32 disp, u32 frames);

#endif
