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

#ifndef _DE_WB_H_
#define _DE_WB_H_

#include <drm/drm_framebuffer.h>
#include "de_base.h"
#include "de_csc.h"

struct de_wb_handle {
	//TODO add hw feature info, format / size etc
	struct module_create_info cinfo;
	unsigned int block_num;
	struct de_reg_block **block;
	const uint32_t *formats;
	unsigned int format_count;
	struct de_wb_private *private;
};

struct wb_in_config {
	unsigned int width;
	unsigned int height;
	struct de_csc_info csc_info;
};

struct de_wb_handle *de_wb_create(struct module_create_info *info);
int de_wb_apply(struct de_wb_handle *handle, struct wb_in_config *in, struct drm_framebuffer *out_fb);

#endif /* #ifndef _DE_WB_H_ */
