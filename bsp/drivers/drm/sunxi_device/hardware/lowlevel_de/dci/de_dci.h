/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * de_dci.h
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
#ifndef _DE_DCI_H
#define _DE_DCI_H

#include <linux/types.h>
#include "de_base.h"
#include "de_channel.h"
#include "de_dci_platform.h"

struct de_dci_handle {
	struct module_create_info cinfo;
	unsigned int block_num;
	struct de_reg_block **block;
	struct de_dci_private *private;
};

struct de_dci_handle *de_dci_create(struct module_create_info *info);
bool de_dci_is_enabled(struct de_dci_handle *hdl);
s32 de_dci_set_color_range(struct de_dci_handle *hdl, enum de_color_range cr);
void de_dci_update_regs(struct de_dci_handle *hdl);
s32 de_dci_enable(struct de_dci_handle *hdl, u32 en);

s32 de_dci_set_demo_mode(struct de_dci_handle *hdl, bool enable);
s32 de_dci_set_window(struct de_dci_handle *hdl,
		      u32 x, u32 y, u32 w, u32 h);
s32 de_dci_set_size(struct de_dci_handle *hdl, u32 width, u32 height);
s32 de_dci_dump_state(struct drm_printer *p, struct de_dci_handle *hdl);
s32 de_dci_update_local_param(struct de_dci_handle *hdl);
int de_dci_pq_proc(struct de_dci_handle *hdl, dci_module_param_t *para);

#endif /*End of file*/
