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

#ifndef _DE_GAMMA_PLATFORM_H_
#define _DE_GAMMA_PLATFORM_H_

#include "de_base.h"
#include "de_dither.h"

struct de_dither_dsc {
	unsigned int id;
	const enum dither_out_fmt *support_fmts;
	unsigned fmt_cnt;
	const enum dither_mode *support_modes;
	unsigned mode_cnt;
	bool support_3d_fifo;
};

const struct de_dither_dsc *get_dither_dsc(struct module_create_info *info);

#endif
