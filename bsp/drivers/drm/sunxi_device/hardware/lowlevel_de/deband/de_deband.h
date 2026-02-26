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

#ifndef _DE_DEBAND_H_
#define _DE_DEBAND_H_

#include <linux/types.h>
#include <video/sunxi_drm.h>
#include "de_base.h"
#include "de_deband_platform.h"

struct de_deband_handle {
	struct module_create_info cinfo;
	unsigned int block_num;
	struct de_reg_block **block;
	struct de_deband_private *private;
};

s32 de_deband_set_outinfo(struct de_deband_handle *hdl, enum de_color_space cs,
			  enum de_data_bits bits, enum de_format_space fmt);
bool de_deband_is_enabled(struct de_deband_handle *hdl);
struct de_deband_handle *de_deband_create(struct module_create_info *info);
s32 de_deband_set_size(struct de_deband_handle *hdl, u32 width, u32 height);
s32 de_deband_set_demo_mode(struct de_deband_handle *hdl, bool enable);
s32 de_deband_set_window(struct de_deband_handle *hdl, u32 x, u32 y, u32 w, u32 h);
s32 de_deband_enable(struct de_deband_handle *hdl, u32 en);
s32 de_deband_dump_state(struct drm_printer *p, struct de_deband_handle *hdl);
int de_deband_pq_proc(struct de_deband_handle *hdl, deband_module_param_t *para);

#endif /* #ifndef _DE_CDC_H_ */
