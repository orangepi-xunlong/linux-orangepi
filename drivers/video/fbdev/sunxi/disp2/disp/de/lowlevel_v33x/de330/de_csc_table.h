/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_CSC_TABLE_H_
#define _DE_CSC_TABLE_H_

#include <linux/types.h>
#include "de_rtmx.h"

extern u32 r2r[2][16];
extern u32 r2y[14][16];
extern u32 y2yl2l[21][16];
extern u32 y2yf2l[21][16];
extern u32 y2yf2f[21][16];
extern u32 y2yl2f[21][16];
extern u32 y2rl2l[7][16];
extern u32 y2rl2f[7][16];
extern u32 y2rf2l[7][16];
extern u32 y2rf2f[7][16];

struct de_csc_info {
	enum de_format_space px_fmt_space;
	enum de_color_space color_space;
	enum de_color_range color_range;
	enum de_eotf eotf;
};

s32 de_csc_coeff_calc(struct de_csc_info *in_info,
	struct de_csc_info *out_info, u32 **csc_coeff);

#endif /* #ifndef _DE_CSC_TABLE_H_ */
