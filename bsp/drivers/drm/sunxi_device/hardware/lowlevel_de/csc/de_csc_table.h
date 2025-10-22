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

#ifndef _DE_CSC_TABLE_H_
#define _DE_CSC_TABLE_H_

#include <linux/types.h>
#include "de_base.h"
#include "de_csc.h"

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

extern int y2r8bit[192];
extern int y2r10bit[288];
extern int ir2y8bit[128];
extern int ir2y10bit[192];
extern int y2y8bit[64];
extern int ir2r8bit[32];
extern int ir2r10bit[32];
extern int y2y10bit[192];
extern int bypass_csc8bit[12];
extern int bypass_csc10bit[12];
extern unsigned int sin_cos8bit[128];
extern int sin_cos10bit[128];

s32 de_csc_coeff_calc(const struct de_csc_info *in_info,
	const struct de_csc_info *out_info, u32 **csc_coeff);

#endif /* #ifndef _DE_CSC_TABLE_H_ */
