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

#ifndef _DE_GAMMA_H_
#define _DE_GAMMA_H_

#include "de_base.h"
#include "de_csc.h"

enum de_gamma_type {
	CHANNEL_DLC_GAMMA,
	DEVICE_GAMMA,
};

struct gamma_extra_create_info {
	enum de_gamma_type type;
};

struct de_gamma_handle {
	struct module_create_info cinfo;
    struct gamma_extra_create_info ex_cinfo;
	unsigned int gamma_lut_len;
	unsigned int cm_bit_width;
	bool support_ctc;
	bool support_cm;
	bool support_demo_skin;
	int hue_default_value;
	unsigned int block_num;
	struct de_reg_block **block;
	struct de_gamma_private *private;
};

struct de_gamma_cfg {
	bool enable;
	u32 *gamma_tbl;
	bool skin_protect_enable;
	unsigned int skin_brighten;
	unsigned int skin_darken;
};

struct de_ctc_cfg {
	bool enable;
	unsigned int red_gain, green_gain, blue_gain;
	unsigned int red_offset, green_offset, blue_offset;
};

int de_gamma_set_size(struct de_gamma_handle *hdl, u32 width, u32 height);
struct de_gamma_handle *de_gamma_create(struct module_create_info *info);
int de_gamma_ctc_config(struct de_gamma_handle *hdl, struct de_ctc_cfg *cfg);
int de_gamma_apply_csc(struct de_gamma_handle *hdl, const struct de_csc_info *in_info,
		    const struct de_csc_info *out_info, const struct bcsh_info *bcsh,  const struct ctm_info *ctm);
s32 de_gamma_set_demo_mode(struct de_gamma_handle *hdl, bool enable);
s32 de_gamma_set_window(struct de_gamma_handle *hdl, u32 x, u32 y, u32 w, u32 h);

int de_gamma_config(struct de_gamma_handle *hdl, struct de_gamma_cfg *cfg);
void de_gamma_dump_state(struct drm_printer *p, struct de_gamma_handle *hdl);

#endif
