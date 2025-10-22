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

#ifndef _DE_OVL_H_
#define _DE_OVL_H_

#include "de_base.h"
#include "de_channel.h"

enum ovl_premul {
	DE_OVL_PREMUL_NON_TO_NON = 0,
	DE_OVL_PREMUL_NON_TO_EXC = 1,
	DE_OVL_PREMUL_HAS_PREMUL = 2,
};

struct de_ovl_handle {
	struct module_create_info cinfo;
	const char *name;
	u32 channel_reg_base;
	const uint32_t *formats;
	unsigned int format_count;
	unsigned int layer_cnt;
	bool is_video;
	unsigned int type_hw_id;
	unsigned int block_num;
	struct de_reg_block **block;
	struct de_ovl_private *private;
};

struct de_ovl_cfg {
	unsigned int layer_en_cnt;
	u8 layer_en[MAX_LAYER_NUM_PER_CHN];
	u8 lay_premul[MAX_LAYER_NUM_PER_CHN];
	struct de_rect_s lay_win[MAX_LAYER_NUM_PER_CHN];
	struct de_rect_s ovl_win;
	struct de_rect_s ovl_out_win;
};

int de_ovl_apply_lay(struct de_ovl_handle *handle, struct display_channel_state *state, const struct de_ovl_cfg *cfg);
struct de_ovl_handle *de_ovl_create(struct module_create_info *info);
void de_dump_ovl_state(struct drm_printer *p, struct de_ovl_handle *handle, const struct display_channel_state *state);

#endif
