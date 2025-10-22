/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2023 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_DLC_H
#define _DE_DLC_H

#include <linux/types.h>
#include "de_base.h"
#include "de_channel.h"
#include "de_dlc_platform.h"

struct de_dlc_handle {
	struct module_create_info cinfo;
	unsigned int block_num;
	struct de_reg_block **block;
	struct de_dlc_private *private;
};

struct de_dlc_handle *de_dlc_create(struct module_create_info *info);
s32 de_dlc_enable(struct de_dlc_handle *hdl, u32 en);
bool de_dlc_is_enabled(struct de_dlc_handle *hdl);
s32 de_dlc_set_size(struct de_dlc_handle *hdl, u32 width, u32 height);
s32 de_dlc_set_color_range(struct de_dlc_handle *hdl, enum de_color_range color_range);
s32 de_dlc_update_local_param(struct de_dlc_handle *hdl, u32 *gamma_table);
int de_dlc_pq_proc(struct de_dlc_handle *hdl, dlc_module_param_t *para);
s32 de_dlc_dump_state(struct drm_printer *p, struct de_dlc_handle *hdl);
void de_dlc_update_regs(struct de_dlc_handle *hdl);

#endif /*End of file*/
