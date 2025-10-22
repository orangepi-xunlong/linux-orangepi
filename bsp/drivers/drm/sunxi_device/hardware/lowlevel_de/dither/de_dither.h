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

#ifndef _DE_DITHER_H_
#define _DE_DITHER_H_

#include "de_base.h"

enum dither_mode {
	QUANTIZATION = 0,
	FLOYD_STEINBERG = 1,
	ORDER = 3,/*565/666 not supported*/
	SIERRA_LITE = 4,
	BURKE = 5,
	RANDOM = 6,
};

enum dither_out_fmt {
	FMT888 = 0,
	FMT444 = 1,
	FMT565 = 2,
	FMT666 = 3,
};

struct dither_config {
	bool enable;
	unsigned int w, h;
	bool enable_3d_fifo;
	enum dither_mode mode;
	enum dither_out_fmt out_fmt;
};

struct de_dither_handle {
	struct module_create_info cinfo;
	const enum dither_out_fmt *support_fmts;
	unsigned fmt_cnt;
	const enum dither_mode *support_modes;
	unsigned mode_cnt;
	bool support_3d_fifo;
	unsigned int block_num;
	struct de_reg_block **block;
	struct de_dither_private *private;
};

struct de_dither_handle *de_dither_create(struct module_create_info *info);
int de_dither_config(struct de_dither_handle *hdl, struct dither_config *cfg);
void de_dither_dump_state(struct drm_printer *p, struct de_dither_handle *hdl);

#endif
