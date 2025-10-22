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

#ifndef _DE_FBD_ATW_H_
#define _DE_FBD_ATW_H_

#include "de_base.h"
#include "de_channel.h"

struct de_afbd_handle {
	struct module_create_info cinfo;
	bool rotate_support;
	unsigned int rotate_limit_height;
	unsigned int format_modifiers_num;
	const uint64_t *format_modifiers;
	unsigned int block_num;
	struct de_reg_block **block;
	struct de_fbd_atw_private *private;
};

struct de_afbd_cfg {
	struct de_rect_s ovl_win;
	u8 lay_premul;
};

int de_afbd_apply_lay(struct de_afbd_handle *handle, struct display_channel_state *state, struct de_afbd_cfg *cfg, bool *is_enable);
struct de_afbd_handle *de_afbd_create(struct module_create_info *info);
bool de_afbd_should_enable(struct de_afbd_handle *handle, struct display_channel_state *state);
bool de_afbc_format_mod_supported(struct de_afbd_handle *hdl, u32 format, u64 modifier);
void de_dump_afbd_state(struct drm_printer *p, struct de_afbd_handle *handle, const struct display_channel_state *state);

#endif /* #ifndef _DE_FBD_ATW_H_ */
