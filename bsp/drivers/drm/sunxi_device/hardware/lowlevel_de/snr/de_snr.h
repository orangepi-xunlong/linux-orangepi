/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * de_snr.h
 *
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _DE_SNR_H
#define _DE_SNR_H

#include <linux/types.h>
#include "de_base.h"
#include "de_channel.h"
#include "de_snr_platform.h"

struct de_snr_handle {
	struct module_create_info cinfo;
	unsigned int block_num;
	struct de_reg_block **block;
	struct de_snr_private *private;
};

s32 de_snr_set_demo_mode(struct de_snr_handle *hdl, bool enable);
s32 de_snr_set_window(struct de_snr_handle *hdl,
		      u32 x, u32 y, u32 w, u32 h);
bool de_snr_is_enabled(struct de_snr_handle *hdl);
s32 de_snr_set_para(struct de_snr_handle *hdl, struct display_channel_state *state, struct de_snr_para *snr_para);
s32 de_snr_set_size(struct de_snr_handle *hdl, u32 width, u32 height);
struct de_snr_handle *de_snr_create(struct module_create_info *info);
void de_snr_update_regs(struct de_snr_handle *hdl);
s32 de_snr_enable(struct de_snr_handle *hdl, u32 snr_enable);
s32 de_snr_dump_state(struct drm_printer *p, struct de_snr_handle *hdl);

#endif /*End of file*/
