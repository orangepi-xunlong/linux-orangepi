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

#ifndef _DE_BLENDER_H_
#define _DE_BLENDER_H_

#include <linux/types.h>
#include "de_base.h"

struct de_bld_handle {
	struct module_create_info cinfo;
	u32 disp_reg_base;
	unsigned int block_num;
	struct de_reg_block **block;
	struct de_bld_private *private;
};

int de_bld_output_set_attr(struct de_bld_handle *hdl, u32 width, u32 height, u32 fmt_space, u32 interlaced, bool apply);
int de_bld_pipe_reset(struct de_bld_handle *hdl, unsigned int pipe_id, int port_id);
int de_bld_get_chn_mux_port(struct de_bld_handle *hdl, unsigned int chn_mode, bool is_video, unsigned int type_id);
int de_bld_pipe_set_attr(struct de_bld_handle *hdl, unsigned int pipe_id, unsigned int port_id, const struct drm_rect *rect, bool is_premul);
struct de_bld_handle *de_blender_create(struct module_create_info *info,  unsigned int chn_mode);

void dump_bld_state(struct drm_printer *p, struct de_bld_handle *hdl);

#endif /* #ifndef _DE_BLENDER_H_ */
