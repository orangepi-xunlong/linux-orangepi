/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_TFBD_H_
#define _DE_TFBD_H_

#include "de_base.h"
#include "de_channel.h"

struct de_tfbd_handle {
	struct module_create_info cinfo;
	unsigned int format_modifiers_num;
	const uint64_t *format_modifiers;
	unsigned int block_num;
	struct de_reg_block **block;
	struct de_tfbd_private *private;
};

struct de_tfbd_cfg {
	struct de_rect_s ovl_win;
};

int de_tfbd_apply_lay(struct de_tfbd_handle *handle, struct display_channel_state *state, struct de_tfbd_cfg *cfg, bool *is_enable);
struct de_tfbd_handle *de_tfbd_create(struct module_create_info *info);
bool de_tfbd_should_enable(struct de_tfbd_handle *handle, struct display_channel_state *state);
bool de_tfbd_format_mod_supported(struct de_tfbd_handle *hdl, u32 format, u64 modifier);
void de_dump_tfbd_state(struct drm_printer *p, struct de_tfbd_handle *handle, const struct display_channel_state *state);

#endif /* #ifndef _DE_TFBD_H_ */
