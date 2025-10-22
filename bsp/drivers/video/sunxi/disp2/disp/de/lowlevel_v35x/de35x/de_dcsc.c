/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*****************************************************************************
 *All Winner Tech, All Right Reserved. 2019-2022 Copyright (c)
 *
 *File name   :       de_dcsc.c
 *
 *Description :       display engine drc basic function definition
 *
 *History     :       2022/09/19 v0.1  Initial version
 *****************************************************************************/

#include "de_rtmx.h"
#include "de_dcsc_type.h"
#include "de_dcsc.h"
#include "de_enhance.h"

#define CSC_ENHANCE_MODE_NUM 3
/* must equal to ENHANCE_MODE_NUM */

int y2r8bit[192] = {

	/* bt601 */
	0x000012A0, 0x0, 0x00000000, 0x0, 0x00001989, 0x0, 0xFFF21168, 0xFFFFFFFF, 0x000012A0, 0x0,
	0xFFFFF9BE, 0xFFFFFFFF, 0xFFFFF2FE, 0xFFFFFFFF, 0x000877CF, 0x0,
	0x000012A0, 0x0, 0x0000204A, 0x0, 0x00000000, 0x0, 0xFFEEB127, 0xFFFFFFFF, 0x00000000, 0x0,
	0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

	/* bt709 */
	0x000012A0, 0x0, 0x00000000, 0x0, 0x00001CB0, 0x0, 0xFFF07DF4, 0xFFFFFFFF, 0x000012A0, 0x0,
	0xfffffC98, 0xFFFFFFFF, 0xfffff775, 0xFFFFFFFF, 0x0004CFDF, 0x0,
	0x000012A0, 0x0, 0x000021D7, 0x0, 0x00000000, 0x0, 0xFFEDEA7F, 0xFFFFFFFF, 0x00000000, 0x0,
	0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

	/* ycc */
	0x00001000, 0x0, 0x00000000, 0x0, 0x0000166F, 0x0, 0xFFF4C84B, 0xFFFFFFFF, 0x00001000, 0x0,
	0xFFFFFA78, 0xFFFFFFFF, 0xFFFFF491, 0xFFFFFFFF, 0x00087B16, 0x0,
	0x00001000, 0x0, 0x00001C56, 0x0, 0x00000000, 0x0, 0xFFF1D4FE, 0xFFFFFFFF, 0x00000000, 0x0,
	0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

	/* ehance */
	0x00001000, 0x0, 0x00000000, 0x0, 0x00001933, 0x0, 0xFFF36666, 0xFFFFFFFF, 0x00001000, 0x0,
	0xFFFFFD02, 0xFFFFFFFF, 0xFFFFF883, 0xFFFFFFFF, 0x00053D71, 0x0,
	0x00001000, 0x0, 0x00001DB2, 0x0, 0x00000000, 0x0, 0xFFF126E9, 0xFFFFFFFF, 0x00000000, 0x0,
	0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

	/* bt601 studio */
	0x00001000, 0x0, 0x00000000, 0x0, 0x000015F0, 0x0, 0xFFF50831, 0xFFFFFFFF, 0x00001000, 0x0,
	0xFFFFFAA0, 0xFFFFFFFF, 0xFFFFF4FA, 0xFFFFFFFF, 0x00083333, 0x0,
	0x00001000, 0x0, 0x00001BB6, 0x0, 0x00000000, 0x0, 0xFFF224DD, 0xFFFFFFFF, 0x00000000, 0x0,
	0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

	/* bt709 studio */
	0x00001000, 0x0, 0x00000000, 0x0, 0x000018A4, 0x0, 0xFFF3AE14, 0xFFFFFFFF, 0x00001000, 0x0,
	0xFFFFFD12, 0xFFFFFFFF, 0xFFFFF8A8, 0xFFFFFFFF, 0x000522D1, 0x0,
	0x00001000, 0x0, 0x00001D0E, 0x0, 0x00000000, 0x0, 0xFFF178D5, 0xFFFFFFFF, 0x00000000, 0x0,
	0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

};

int y2r10bit[288] = {

    /* bt601 */
    0x000012A1, 0x0, 0x00000000, 0x0, 0x00001989, 0x0, 0xFFC845c0, 0xFFFFFFFF,
    0x000012A1, 0x0, 0xFFFFF9BB, 0xFFFFFFFF, 0xFFFFF2FE, 0xFFFFFFFF, 0x0021e5c0,
    0x0, 0x000012A1, 0x0, 0x00002046, 0x0, 0x00000000, 0x0, 0xFFBACBC0,
    0xFFFFFFFF, 0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000,
    0x0,

    /* bt709 */
    0x000012A1, 0x0, 0x00000000, 0x0, 0x00001CAF, 0x0, 0xFFC1F9C0, 0xFFFFFFFF,
    0x000012A1, 0x0, 0xfffffC97, 0xFFFFFFFF, 0xfffff77A, 0xFFFFFFFF, 0x001335C0,
    0x0, 0x000012A1, 0x0, 0x000021CC, 0x0, 0x00000000, 0x0, 0xFFB7bfc0,
    0xFFFFFFFF, 0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000,
    0x0,

    /* ycc  bt601 10bit full range yuv*/
    0x00001000, 0x0, 0x00000000, 0x0, 0x0000166F, 0x0, 0xFFD32200, 0xFFFFFFFF,
    0x00001000, 0x0, 0xFFFFFA7E, 0xFFFFFFFF, 0xFFFFF493, 0xFFFFFFFF, 0x0021DE00,
    0x0, 0x00001000, 0x0, 0x00001C5A, 0x0, 0x00000000, 0x0, 0xFFC74C00,
    0xFFFFFFFF, 0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000,
    0x0,

    /* ehance bt709 10bit full range yuv*/
    0x00001000, 0x0, 0x00000000, 0x0, 0x00001932, 0x0, 0xFFCD9C00, 0xFFFFFFFF,
    0x00001000, 0x0, 0xFFFFFD01, 0xFFFFFFFF, 0xFFFFF883, 0xFFFFFFFF, 0x0014F800,
    0x0, 0x00001000, 0x0, 0x00001DB1, 0x0, 0x00000000, 0x0, 0xFFC49E00,
    0xFFFFFFFF, 0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000,
    0x0,

    /* bt601 studio */
    0x00001000, 0x0, 0x00000000, 0x0, 0x000015F0, 0x0, 0xFFD42000, 0xFFFFFFFF,
    0x00001000, 0x0, 0xFFFFFAA0, 0xFFFFFFFF, 0xFFFFF4D5, 0xFFFFFFFF, 0x00211600,
    0x0, 0x00001000, 0x0, 0x00001BB6, 0x0, 0x00000000, 0x0, 0xFFC89400,
    0xFFFFFFFF, 0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000,
    0x0,

    /* bt709 studio */
    0x00001000, 0x0, 0x00000000, 0x0, 0x000018A4, 0x0, 0xFFCEB800, 0xFFFFFFFF,
    0x00001000, 0x0, 0xFFFFFD12, 0xFFFFFFFF, 0xFFFFF8A8, 0xFFFFFFFF, 0x00148C00,
    0x0, 0x00001000, 0x0, 0x00001D0E, 0x0, 0x00000000, 0x0, 0xFFC5E400,
    0xFFFFFFFF, 0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000,
    0x0,

    /* bt2020 */
    0x000012A1, 0x0, 0x00000000, 0x0, 0x00001ADC, 0x0, 0xFFC59FC0, 0xFFFFFFFF,
    0x000012A1, 0x0, 0xfffffd00, 0xFFFFFFFF, 0xfffff598, 0xFFFFFFFF, 0x001627C0,
    0x0, 0x000012A1, 0x0, 0x00002245, 0x0, 0x00000000, 0x0, 0xFFB6CDC0,
    0xFFFFFFFF, 0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000,
    0x0,

    /*bt2020 10bit full range yuv*/
    0x00001000, 0x0, 0x00000000, 0x0, 0x00001798, 0x0, 0xFFD0D000, 0xFFFFFFFF,
    0x00001000, 0x0, 0xFFFFFD5E, 0xFFFFFFFF, 0xFFFFF6DC, 0xFFFFFFFF, 0x00178C00,
    0x0, 0x00001000, 0x0, 0x00001E1A, 0x0, 0x00000000, 0x0, 0xFFC3CC00,
    0xFFFFFFFF, 0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000,
    0x0,

    /* bt2020 studio */
    0x00001000, 0x0, 0x00000000, 0x0, 0x00001711, 0x0, 0xFFD1DE00, 0xFFFFFFFF,
    0x00001000, 0x0, 0xFFFFFD6D, 0xFFFFFFFF, 0xFFFFF710, 0xFFFFFFFF, 0x00170600,
    0x0, 0x00001000, 0x0, 0x00001D6F, 0x0, 0x00000000, 0x0, 0xFFC52200,
    0xFFFFFFFF, 0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000,
    0x0,
};

int ir2y8bit[128] = {

	/* bt601 */
	0x0000041D, 0x0, 0x00000810, 0x0, 0x00000191, 0x0, 0x00010000, 0x0, 0xFFFFFDA2, 0xFFFFFFFF,
	0xFFFFFB58, 0xFFFFFFFF, 0x00000706, 0x0, 0x00080000, 0x0,
	0x00000706, 0x0, 0xFFFFFA1D, 0xFFFFFFFF, 0xFFFFFEDD, 0xFFFFFFFF, 0x00080000, 0x0, 0x00000000,
	0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

	/* bt709 */
	0x000002EE, 0x0, 0x000009D3, 0x0, 0x000000FE, 0x0, 0x00010000, 0x0, 0xfffffe62, 0xFFFFFFFF,
	0xfffffA98, 0xFFFFFFFF, 0x00000706, 0x0, 0x00080000, 0x0,
	0x00000706, 0x0, 0xfffff99E, 0xFFFFFFFF, 0xffffff5C, 0xFFFFFFFF, 0x00080000, 0x0, 0x00000000,
	0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

	/* ycc */
	0x000004C8, 0x0, 0x00000963, 0x0, 0x000001D5, 0x0, 0x00000000, 0x0, 0xFFFFFD4D, 0xFFFFFFFF,
	0xFFFFFAB3, 0xFFFFFFFF, 0x00000800, 0x0, 0x00080000, 0x0,
	0x00000800, 0x0, 0xFFFFF94F, 0xFFFFFFFF, 0xFFFFFEB2, 0xFFFFFFFF, 0x00080000, 0x0, 0x00000000,
	0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

	/* ehance */
	0x00000368, 0x0, 0x00000B71, 0x0, 0x00000127, 0x0, 0x00000000, 0x0, 0xFFFFFE29, 0xFFFFFFFF,
	0xFFFFF9D7, 0xFFFFFFFF, 0x00000800, 0x0, 0x00080000, 0x0,
	0x00000800, 0x0, 0xFFFFF8BC, 0xFFFFFFFF, 0xFFFFFF44, 0xFFFFFFFF, 0x00080000, 0x0, 0x00000000,
	0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,
};

int ir2y10bit[192] = {

    /* bt601 */
    0x0000041D, 0x0, 0x00000810, 0x0, 0x00000191, 0x0, 0x00040000, 0x0,
    0xFFFFFDA1, 0xFFFFFFFF, 0xFFFFFB58, 0xFFFFFFFF, 0x00000707, 0x0, 0x00200000,
    0x0, 0x00000707, 0x0, 0xFFFFFA1D, 0xFFFFFFFF, 0xFFFFFEDD, 0xFFFFFFFF,
    0x00200000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0,
    0x00001000, 0x0,

    /* bt709 */
    0x000002EC, 0x0, 0x000009D4, 0x0, 0x000000FE, 0x0, 0x00040000, 0x0,
    0xfffffe64, 0xFFFFFFFF, 0xfffffA95, 0xFFFFFFFF, 0x00000707, 0x0, 0x00200000,
    0x0, 0x00000707, 0x0, 0xfffff99E, 0xFFFFFFFF, 0xffffff5B, 0xFFFFFFFF,
    0x00200000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0,
    0x00001000, 0x0,

    /* ycc bt601 10bit full range yuv*/
    0x000004C9, 0x0, 0x00000964, 0x0, 0x000001D3, 0x0, 0x00000000, 0x0,
    0xFFFFFD4D, 0xFFFFFFFF, 0xFFFFFAB3, 0xFFFFFFFF, 0x00000800, 0x0, 0x00200000,
    0x0, 0x00000800, 0x0, 0xFFFFF94D, 0xFFFFFFFF, 0xFFFFFEB3, 0xFFFFFFFF,
    0x00200000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0,
    0x00001000, 0x0,

    /* ehance bt709 10bit full range yuv */
    0x00000367, 0x0, 0x00000B71, 0x0, 0x00000128, 0x0, 0x00000000, 0x0,
    0xFFFFFE2B, 0xFFFFFFFF, 0xFFFFF9D5, 0xFFFFFFFF, 0x00000800, 0x0, 0x00200000,
    0x0, 0x00000800, 0x0, 0xFFFFF8BC, 0xFFFFFFFF, 0xFFFFFF44, 0xFFFFFFFF,
    0x00200000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0,
    0x00001000, 0x0,

    /* bt2020 tv */
    0x0000039C, 0x0, 0x00000951, 0x0, 0x000000D0, 0x0, 0x00040000, 0x0,
    0xfffffe0A, 0xFFFFFFFF, 0xfffffAEF, 0xFFFFFFFF, 0x00000707, 0x0, 0x00200000,
    0x0, 0x00000707, 0x0, 0xfffff98A, 0xFFFFFFFF, 0xffffff6F, 0xFFFFFFFF,
    0x00200000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0,
    0x00001000, 0x0,

    /* bt2020 10bit full range yuv */
    0x00000434, 0x0, 0x00000AD9, 0x0, 0x000000F3, 0x0, 0x00000000, 0x0,
    0xFFFFFDC4, 0xFFFFFFFF, 0xFFFFFA3C, 0xFFFFFFFF, 0x00000800, 0x0, 0x00200000,
    0x0, 0x00000800, 0x0, 0xFFFFF8A5, 0xFFFFFFFF, 0xFFFFFF5B, 0xFFFFFFFF,
    0x00200000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0,
    0x00001000, 0x0,

};

int y2y8bit[64] = {

	/* bt601 to bt709 */
	0x00001000, 0x0, 0xFFFFFE27, 0xFFFFFFFF, 0xFFFFFCAC, 0xFFFFFFFF, 0x00029681, 0x0, 0x00000000,
	0x0, 0x0000104C, 0x0, 0x000001D5, 0x0, 0xFFFEEF17, 0xFFFFFFFF,
	0x00000000, 0x0, 0x00000133, 0x0, 0x00001068, 0x0, 0xFFFF326E, 0xFFFFFFFF, 0x00000000, 0x0,
	0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

	/* bt709 to bt601 */
	0x00001000, 0x0, 0x00000197, 0x0, 0x00000311, 0x0, 0xFFFDAC02, 0xFFFFFFFF, 0x00000000, 0x0,
	0x00000FD6, 0x0, 0xFFFFFE3B, 0xFFFFFFFF, 0x0000F765, 0x0,
	0x00000000, 0x0, 0xFFFFFED7, 0xFFFFFFFF, 0x00000FBC, 0x0, 0x0000B663, 0x0, 0x00000000, 0x0,
	0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,
};

int ir2r8bit[32] = {

	/* 0-255 to 16-235 */
	0x00000DC0, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00010000, 0x0, 0x00000000, 0x0,
	0x00000DC0, 0x0, 0x00000000, 0x0, 0x00010000, 0x0,
	0x00000000, 0x0, 0x00000000, 0x0, 0x00000DC0, 0x0, 0x00010000, 0x0, 0x00000000, 0x0,
	0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,
};

int ir2r10bit[32] = {

    /* 0-255 to 16-235 */
    0x00000DC0, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00010000, 0x0,
    0x00000000, 0x0, 0x00000DC0, 0x0, 0x00000000, 0x0, 0x00010000, 0x0,
    0x00000000, 0x0, 0x00000000, 0x0, 0x00000DC0, 0x0, 0x00010000, 0x0,
    0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,
};

int y2y10bit[192] = {

    /* bt601 to bt709 */
    0x00001000, 0x0, 0xFFFFFE27, 0xFFFFFFFF, 0xFFFFFCAC, 0xFFFFFFFF, 0x000a5a00,
    0x0, 0x00000000, 0x0, 0x0000104C, 0x0, 0x000001D5, 0x0, 0xFFFBBE00,
    0xFFFFFFFF, 0x00000000, 0x0, 0x00000133, 0x0, 0x00001068, 0x0, 0xFFFCCA00,
    0xFFFFFFFF, 0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000,
    0x0,

    /* bt709 to bt601 */
    0x00001000, 0x0, 0x00000197, 0x0, 0x00000311, 0x0, 0xFFF6B000, 0xFFFFFFFF,
    0x00000000, 0x0, 0x00000FD6, 0x0, 0xFFFFFE3B, 0xFFFFFFFF, 0x0003DE00, 0x0,
    0x00000000, 0x0, 0xFFFFFED7, 0xFFFFFFFF, 0x00000FBC, 0x0, 0x0002DA00, 0x0,
    0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

    /* bt601 to bt2020 */
    0x00001000, 0x0, 0xFFFFFE22, 0xFFFFFFFF, 0xFFFFFCE8, 0xFFFFFFFF, 0xFFF61400,
    0xFFFFFFFF, 0x00000000, 0x0, 0x00000E0B, 0x0, 0x00000141, 0x0, 0x00016800,
    0x0, 0x00000000, 0x0, 0x000000F6, 0x0, 0x000009B9, 0x0, 0x000AA200, 0x0,
    0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

    /* bt2020 to bt601 */
    0x00001000, 0x0, 0x000001CC, 0x0, 0x000004DC, 0x0, 0xFFF2B000, 0xFFFFFFFF,
    0x00000000, 0x0, 0x00001264, 0x0, 0xFFFFFDA1, 0xFFFFFFFF, 0xFFFFF600,
    0xFFFFFFFF, 0x00000000, 0x0, 0xFFFFFE2E, 0xFFFFFFFF, 0x000001A91, 0x0,
    0xFFEE8200, 0xFFFFFFFF, 0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0,
    0x00001000, 0x0,

    /* bt709 to bt2020 */
    0x00001000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0,
    0x00000000, 0x0, 0x00000DFE, 0x0, 0xFFFFFFDE, 0xFFFFFFFF, 0x00044800, 0x0,
    0x00000000, 0x0, 0x00000034, 0x0, 0x0000090C, 0x0, 0x000D8000, 0x0,
    0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

    /* bt2020 to bt709 */
    0x00001000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0,
    0x00000000, 0x0, 0x0000124A, 0x0, 0x00000044, 0x0, 0xFFFAE400, 0xFFFFFFFF,
    0x00000000, 0x0, 0xFFFFFF97, 0xFFFFFFFF, 0x00001C4B, 0x0, 0xFFE83C00,
    0xFFFFFFFF, 0x00000000, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000,
    0x0,
};
int bypass_csc8bit[12] = {

	0x00000400, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000400, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000400, 0x00000000,
};

int bypass_csc10bit[12] = {

    0x00000400, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000400,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000400, 0x00000000,
};

unsigned int sin_cos8bit[128] = {
	/* sin table */
	0xffffffbd, 0xffffffbf, 0xffffffc1, 0xffffffc2, 0xffffffc4, 0xffffffc6, 0xffffffc8, 0xffffffca,
	0xffffffcc, 0xffffffce, 0xffffffd1, 0xffffffd3, 0xffffffd5, 0xffffffd7, 0xffffffd9, 0xffffffdb,
	0xffffffdd, 0xffffffdf, 0xffffffe2, 0xffffffe4, 0xffffffe6, 0xffffffe8, 0xffffffea, 0xffffffec,
	0xffffffef, 0xfffffff1, 0xfffffff3, 0xfffffff5, 0xfffffff8, 0xfffffffa, 0xfffffffc, 0xfffffffe,
	0x00000000, 0x00000002, 0x00000004, 0x00000006, 0x00000008, 0x0000000b, 0x0000000d, 0x0000000f,
	0x00000011, 0x00000014, 0x00000016, 0x00000018, 0x0000001a, 0x0000001c, 0x0000001e, 0x00000021,
	0x00000023, 0x00000025, 0x00000027, 0x00000029, 0x0000002b, 0x0000002d, 0x0000002f, 0x00000032,
	0x00000034, 0x00000036, 0x00000038, 0x0000003a, 0x0000003c, 0x0000003e, 0x0000003f, 0x00000041,
	/* cos table */
	0x0000006c, 0x0000006d, 0x0000006e, 0x0000006f, 0x00000071, 0x00000072, 0x00000073, 0x00000074,
	0x00000074, 0x00000075, 0x00000076, 0x00000077, 0x00000078, 0x00000079, 0x00000079, 0x0000007a,
	0x0000007b, 0x0000007b, 0x0000007c, 0x0000007c, 0x0000007d, 0x0000007d, 0x0000007e, 0x0000007e,
	0x0000007e, 0x0000007f, 0x0000007f, 0x0000007f, 0x0000007f, 0x0000007f, 0x0000007f, 0x0000007f,
	0x00000080, 0x0000007f, 0x0000007f, 0x0000007f, 0x0000007f, 0x0000007f, 0x0000007f, 0x0000007f,
	0x0000007e, 0x0000007e, 0x0000007e, 0x0000007d, 0x0000007d, 0x0000007c, 0x0000007c, 0x0000007b,
	0x0000007b, 0x0000007a, 0x00000079, 0x00000079, 0x00000078, 0x00000077, 0x00000076, 0x00000075,
	0x00000074, 0x00000074, 0x00000073, 0x00000072, 0x00000071, 0x0000006f, 0x0000006e, 0x0000006d
};

int sin_cos10bit[128] = {
    /* sin table */
    0xffffffbd, 0xffffffbf, 0xffffffc1, 0xffffffc2, 0xffffffc4, 0xffffffc6,
    0xffffffc8, 0xffffffca, 0xffffffcc, 0xffffffce, 0xffffffd1, 0xffffffd3,
    0xffffffd5, 0xffffffd7, 0xffffffd9, 0xffffffdb, 0xffffffdd, 0xffffffdf,
    0xffffffe2, 0xffffffe4, 0xffffffe6, 0xffffffe8, 0xffffffea, 0xffffffec,
    0xffffffef, 0xfffffff1, 0xfffffff3, 0xfffffff5, 0xfffffff8, 0xfffffffa,
    0xfffffffc, 0xfffffffe, 0x00000000, 0x00000002, 0x00000004, 0x00000006,
    0x00000008, 0x0000000b, 0x0000000d, 0x0000000f, 0x00000011, 0x00000014,
    0x00000016, 0x00000018, 0x0000001a, 0x0000001c, 0x0000001e, 0x00000021,
    0x00000023, 0x00000025, 0x00000027, 0x00000029, 0x0000002b, 0x0000002d,
    0x0000002f, 0x00000032, 0x00000034, 0x00000036, 0x00000038, 0x0000003a,
    0x0000003c, 0x0000003e, 0x0000003f, 0x00000041,
    /* cos table */
    0x0000006c, 0x0000006d, 0x0000006e, 0x0000006f, 0x00000071, 0x00000072,
    0x00000073, 0x00000074, 0x00000074, 0x00000075, 0x00000076, 0x00000077,
    0x00000078, 0x00000079, 0x00000079, 0x0000007a, 0x0000007b, 0x0000007b,
    0x0000007c, 0x0000007c, 0x0000007d, 0x0000007d, 0x0000007e, 0x0000007e,
    0x0000007e, 0x0000007f, 0x0000007f, 0x0000007f, 0x0000007f, 0x0000007f,
    0x0000007f, 0x0000007f, 0x00000080, 0x0000007f, 0x0000007f, 0x0000007f,
    0x0000007f, 0x0000007f, 0x0000007f, 0x0000007f, 0x0000007e, 0x0000007e,
    0x0000007e, 0x0000007d, 0x0000007d, 0x0000007c, 0x0000007c, 0x0000007b,
    0x0000007b, 0x0000007a, 0x00000079, 0x00000079, 0x00000078, 0x00000077,
    0x00000076, 0x00000075, 0x00000074, 0x00000074, 0x00000073, 0x00000072,
    0x00000071, 0x0000006f, 0x0000006e, 0x0000006d
};

void pq_get_enhance(struct disp_csc_config *conig);
static struct disp_csc_config g_dcsc_config[DE_NUM];

static bool use_user_matrix;
/* device csc and smbl in the same module or not */

int user_matrix8bit[32] = {
	/* identity for default */
	0x00001000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00001000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00001000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00001000, 0x00000000,
};

int user_matrix10bit[32] = {
    /* identity for default */
    0x00001000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00001000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00001000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00001000, 0x00000000,
};

struct __scal_matrix4x4 {
	__s64 x00;
	__s64 x01;
	__s64 x02;
	__s64 x03;
	__s64 x10;
	__s64 x11;
	__s64 x12;
	__s64 x13;
	__s64 x20;
	__s64 x21;
	__s64 x22;
	__s64 x23;
	__s64 x30;
	__s64 x31;
	__s64 x32;
	__s64 x33;
};

inline int in_tright_shift(int datain, unsigned int shiftbit)
{
	int dataout;
	int tmp;

	tmp = (shiftbit >= 1) ? (1 << (shiftbit - 1)) : 0;
	if (datain >= 0)
		dataout = (datain + tmp) >> shiftbit;
	else
		dataout = -((-datain + tmp) >> shiftbit);

	return dataout;
}

inline __s64 IntRightShift64(__s64 datain, unsigned int shiftbit)
{
	__s64 dataout;
	__s64 tmp;

	tmp = (shiftbit >= 1) ? (1 << (shiftbit - 1)) : 0;
	if (datain >= 0)
		dataout = (datain + tmp) >> shiftbit;
	else
		dataout = -((-datain + tmp) >> shiftbit);

	return dataout;
}

static s32 IDE_SCAL_MATRIC_MUL(struct __scal_matrix4x4 *in1,
	struct __scal_matrix4x4 *in2, struct __scal_matrix4x4 *result)
{

	result->x00 =
	    IntRightShift64(in1->x00 * in2->x00 + in1->x01 * in2->x10 +
			    in1->x02 * in2->x20 + in1->x03 * in2->x30, 10);
	result->x01 =
	    IntRightShift64(in1->x00 * in2->x01 + in1->x01 * in2->x11 +
			    in1->x02 * in2->x21 + in1->x03 * in2->x31, 10);
	result->x02 =
	    IntRightShift64(in1->x00 * in2->x02 + in1->x01 * in2->x12 +
			    in1->x02 * in2->x22 + in1->x03 * in2->x32, 10);
	result->x03 =
	    IntRightShift64(in1->x00 * in2->x03 + in1->x01 * in2->x13 +
			    in1->x02 * in2->x23 + in1->x03 * in2->x33, 10);
	result->x10 =
	    IntRightShift64(in1->x10 * in2->x00 + in1->x11 * in2->x10 +
			    in1->x12 * in2->x20 + in1->x13 * in2->x30, 10);
	result->x11 =
	    IntRightShift64(in1->x10 * in2->x01 + in1->x11 * in2->x11 +
			    in1->x12 * in2->x21 + in1->x13 * in2->x31, 10);
	result->x12 =
	    IntRightShift64(in1->x10 * in2->x02 + in1->x11 * in2->x12 +
			    in1->x12 * in2->x22 + in1->x13 * in2->x32, 10);
	result->x13 =
	    IntRightShift64(in1->x10 * in2->x03 + in1->x11 * in2->x13 +
			    in1->x12 * in2->x23 + in1->x13 * in2->x33, 10);
	result->x20 =
	    IntRightShift64(in1->x20 * in2->x00 + in1->x21 * in2->x10 +
			    in1->x22 * in2->x20 + in1->x23 * in2->x30, 10);
	result->x21 =
	    IntRightShift64(in1->x20 * in2->x01 + in1->x21 * in2->x11 +
			    in1->x22 * in2->x21 + in1->x23 * in2->x31, 10);
	result->x22 =
	    IntRightShift64(in1->x20 * in2->x02 + in1->x21 * in2->x12 +
			    in1->x22 * in2->x22 + in1->x23 * in2->x32, 10);
	result->x23 =
	    IntRightShift64(in1->x20 * in2->x03 + in1->x21 * in2->x13 +
			    in1->x22 * in2->x23 + in1->x23 * in2->x33, 10);
	result->x30 =
	    IntRightShift64(in1->x30 * in2->x00 + in1->x31 * in2->x10 +
			    in1->x32 * in2->x20 + in1->x33 * in2->x30, 10);
	result->x31 =
	    IntRightShift64(in1->x30 * in2->x01 + in1->x31 * in2->x11 +
			    in1->x32 * in2->x21 + in1->x33 * in2->x31, 10);
	result->x32 =
	    IntRightShift64(in1->x30 * in2->x02 + in1->x31 * in2->x12 +
			    in1->x32 * in2->x22 + in1->x33 * in2->x32, 10);
	result->x33 =
	    IntRightShift64(in1->x30 * in2->x03 + in1->x31 * in2->x13 +
			    in1->x32 * in2->x23 + in1->x33 * in2->x33, 10);

	return 0;
}

/*enum {
	DCSC_REG_BLK_CTL = 0,
	DCSC_REG_BLK_COEFF,
	DCSC_REG_BLK_NUM,
};

struct de_dcsc_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[DCSC_REG_BLK_NUM];

	void (*set_blk_dirty)(struct de_dcsc_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_dcsc_private dcsc_priv[DE_NUM];

static inline struct __csc2_reg_t *get_dcsc_reg(struct de_dcsc_private *priv)
{
	return (struct __csc2_reg_t *)(priv->reg_blks[0].vir_addr);
}

static void dcsc_set_block_dirty(
	struct de_dcsc_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void dcsc_set_rcq_head_dirty(
	struct de_dcsc_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WARN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

static int de_dcsc_set_reg_base(unsigned int sel, void *base)
{
	DE_INFO("sel=%d, base=0x%p\n", sel, base);
	return 0;
}*/

int _csc_enhance_setting[CSC_ENHANCE_MODE_NUM][4] = {
	{50, 50, 50, 50},
	/* normal */
	{50, 50, 50, 50},
	/* vivid */
	{50, 40, 50, 50},
	/* soft */
};

void de_dcsc_pq_get_enhance(u32 sel, int *pq_enh)
{
	struct disp_csc_config *config = &g_dcsc_config[sel];
	unsigned int enhance_mode = config->enhance_mode;

	pq_enh[0] = _csc_enhance_setting[enhance_mode][0];
	pq_enh[1] = _csc_enhance_setting[enhance_mode][1];
	pq_enh[2] = _csc_enhance_setting[enhance_mode][2];
	pq_enh[3] = _csc_enhance_setting[enhance_mode][3];
}

void de_dcsc_pq_set_enhance(u32 sel, int *pq_enh)
{
	struct disp_csc_config *config = &g_dcsc_config[sel];
	unsigned int enhance_mode = config->enhance_mode;

	_csc_enhance_setting[enhance_mode][0] = pq_enh[0];
	_csc_enhance_setting[enhance_mode][1] = pq_enh[1];
	_csc_enhance_setting[enhance_mode][2] = pq_enh[2];
	_csc_enhance_setting[enhance_mode][3] = pq_enh[3];
}

int de_dcsc_set_colormatrix(unsigned int sel, long long *matrix3x4, bool is_identity)
{
	int i;
	int *user_matrix = de_rtmx_is_input_10bit(sel) ? user_matrix10bit : user_matrix8bit;

	if (is_identity) {
		/* when input identity matrix, skip the process of this matrix*/
		use_user_matrix = false;
		return 0;
	}

	use_user_matrix = true;
	/* user_matrix is 64-bit and combined with two int, the last 8 int is fixed*/
	for (i = 0; i < 12; i++) {
		/*   matrix3x4 = real value*2^48
		 *   user_matrix = real value*2^12
		 *   so here >> 36
		 */
		user_matrix[2 * i] = matrix3x4[i] >> 36;
		/* constants  multiply by databits  */
		if (i == 3 || i == 7 || i == 11)
			user_matrix[2 * i] *= 1024;
		user_matrix[2 * i + 1] = matrix3x4[i] >= 0 ? 0 : -1;
	}
	return 0;
}

void pq_set_matrix(struct __scal_matrix4x4 *conig, int choice, int out,
		   int write)
{
	if (choice > 3)
		return;
	if (de_rtmx_is_input_10bit(0)) {
		if (out) {
			if (write) {
				memcpy((y2r10bit + 0x20 * choice), conig,
				       sizeof(struct __scal_matrix4x4));
			} else {
				memcpy(conig, (y2r10bit + 0x20 * choice),
				       sizeof(struct __scal_matrix4x4));
			}
		} else {
			if (write) {
				memcpy((ir2y10bit + 0x20 * choice), conig,
				       sizeof(struct __scal_matrix4x4));
			} else {
				memcpy(conig, (ir2y10bit + 0x20 * choice),
				       sizeof(struct __scal_matrix4x4));
			}
		}
	} else {
		if (out) {
			if (write) {
				memcpy((y2r8bit + 0x20 * choice), conig,
				       sizeof(struct __scal_matrix4x4));
			} else {
				memcpy(conig, (y2r8bit + 0x20 * choice),
				       sizeof(struct __scal_matrix4x4));
			}
		} else {
			if (write) {
				memcpy((ir2y8bit + 0x20 * choice), conig,
				       sizeof(struct __scal_matrix4x4));
			} else {
				memcpy(conig, (ir2y8bit + 0x20 * choice),
				       sizeof(struct __scal_matrix4x4));
			}
		}
	}
}

/* normal case:
 *display a SD video:
 *infmt = DE_YUV, incscmod = BT_601, outfmt = DE_RGB
 *outcscmod = BT_601, out_color_range = DISP_COLOR_RANGE_0_255
 *display a HD video:
 *infmt = DE_YUV, incscmod = BT_709, outfmt = DE_RGB
 *outcscmod = BT_601, out_color_range = DISP_COLOR_RANGE_0_255
 *display a JPEG picture:
 *infmt = DE_YUV, incscmod = BT_YCC, outfmt = DE_RGB
 *outcscmod = BT_601, out_color_range = DISP_COLOR_RANGE_0_255
 *display a UI (RGB format)     with ENHANCE enable
 *infmt = DE_YUV, incscmod = BT_ENHANCE, outfmt = DE_RGB
 *outcscmod = BT_601, out_color_range = DISP_COLOR_RANGE_0_255
 *output to TV with HDMI in RGB mode:
 *infmt = DE_RGB, incscmod = BT_601, outfmt = DE_RGB
 *outcscmod = BT_601, out_color_range = DISP_COLOR_RANGE_16_235
 *output to PC with HDMI in RGB mode:
 *infmt = DE_RGB, incscmod = BT_601, outfmt = DE_RGB
 *outcscmod = BT_601, out_color_range = DISP_COLOR_RANGE_0_255
 *output to TV with HDMI in YCbCr mode, 480i/576i/480p/576p:
 *infmt = DE_RGB, incscmod = BT_601, outfmt = DE_YUV, outcscmod = BT_601
 *out_color_range = DISP_COLOR_RANGE_0_255
 *output to TV with HDMI in YCbCr mode, 720p/1080p/2160p:
 *infmt = DE_RGB, incscmod = BT_601, outfmt = DE_YUV
 *outcscmod = BT_709, out_color_range = DISP_COLOR_RANGE_0_255
 *output to TV with CVBS:
 *infmt = DE_RGB, incscmod = BT_601, outfmt = DE_YUV
 *outcscmod = BT_601, out_color_range = DISP_COLOR_RANGE_0_255
 *bypass:
 *outfmt = infmt, outcscmod = incscmod
 *out_color_range = DISP_COLOR_RANGE_0_255
 *brightness=contrast=saturation=hue=50
 */
int de_csc_coeff_calc_inner8bit(unsigned int infmt, unsigned int incscmod,
		      unsigned int outfmt, unsigned int outcscmod,
		      unsigned int brightness, unsigned int contrast,
		      unsigned int saturation, unsigned int hue,
		      unsigned int out_color_range, int *csc_coeff,
		      bool use_user_matrix)
{
	struct __scal_matrix4x4 *enhancecoeff, *tmpcoeff;
	struct __scal_matrix4x4 *coeff[5], *in0coeff, *in1coeff;
	int oper, i;
	int i_bright, i_contrast, i_saturation, i_hue, sinv, cosv;

	oper = 0;

	enhancecoeff = kmalloc(sizeof(struct __scal_matrix4x4),
			GFP_KERNEL | __GFP_ZERO);
	tmpcoeff = kmalloc(sizeof(struct __scal_matrix4x4),
		GFP_KERNEL | __GFP_ZERO);
	in0coeff = kmalloc(sizeof(struct __scal_matrix4x4),
		GFP_KERNEL | __GFP_ZERO);

	if (!enhancecoeff || !tmpcoeff || !in0coeff) {
		DE_WARN("kmalloc fail\n");
		goto err;
	}

	/* BYPASS */
	if (infmt == outfmt && incscmod == outcscmod
	    && out_color_range == DE_COLOR_RANGE_0_255 && brightness == 50
	    && contrast == 50 && saturation == 50 && hue == 50 && !use_user_matrix) {
		memcpy(csc_coeff, bypass_csc8bit, 48);
		goto err;
	}

	/* dcsc's infmt=DE_RGB, use_user_matrix is used by dcsc, it's ok to do so*/
	if (use_user_matrix) {
		coeff[oper] = (struct __scal_matrix4x4 *)(user_matrix8bit);
		oper++;

	}

	/* NON-BYPASS */
	if (infmt == DE_FORMAT_SPACE_RGB) {
		/* convert to YCbCr */
		if (outfmt == DE_FORMAT_SPACE_RGB) {
			coeff[oper] = (struct __scal_matrix4x4 *) (ir2y8bit + 0x20);
			oper++;
		} else {
			if (outcscmod == DE_COLOR_SPACE_BT601) {
				coeff[oper] = (struct __scal_matrix4x4 *) (ir2y8bit);
				oper++;
			} else if (outcscmod == DE_COLOR_SPACE_BT709) {
				coeff[oper] = (struct __scal_matrix4x4 *)(ir2y8bit + 0x20);
				oper++;
			}
		}
	} else {
		if (incscmod != outcscmod && outfmt == DE_YUV) {
			if (incscmod == DE_COLOR_SPACE_BT601
				&& outcscmod == DE_COLOR_SPACE_BT709) {
				coeff[oper] = (struct __scal_matrix4x4 *) (y2y8bit);
				oper++;
			} else if (incscmod == DE_COLOR_SPACE_BT709
				   && outcscmod == DE_COLOR_SPACE_BT601) {
				coeff[oper] = (struct __scal_matrix4x4 *)(y2y8bit + 0x20);
				oper++;
			}
		}
	}

	if (brightness != 50 || contrast != 50
		|| saturation != 50 || hue != 50) {
		brightness = brightness > 100 ? 100 : brightness;
		contrast = contrast > 100 ? 100 : contrast;
		saturation = saturation > 100 ? 100 : saturation;
		hue = hue > 100 ? 100 : hue;

		i_bright = (int)(brightness * 64 / 100);
		i_saturation = (int)(saturation * 64 / 100);
		i_contrast = (int)(contrast * 64 / 100);
		i_hue = (int)(hue * 64 / 100);

		sinv = sin_cos8bit[i_hue & 0x3f];
		cosv = sin_cos8bit[64 + (i_hue & 0x3f)];

		/* calculate enhance matrix */
		enhancecoeff->x00 = i_contrast << 7;
		enhancecoeff->x01 = 0;
		enhancecoeff->x02 = 0;
		enhancecoeff->x03 =
		    (((i_bright - 32) * 5 + 16) << 12) - (i_contrast << 11);
		enhancecoeff->x10 = 0;
		enhancecoeff->x11 = (i_contrast * i_saturation * cosv) >> 5;
		enhancecoeff->x12 = (i_contrast * i_saturation * sinv) >> 5;
		enhancecoeff->x13 =
		    (1 << 19) - ((enhancecoeff->x11 + enhancecoeff->x12) << 7);
		enhancecoeff->x20 = 0;
		enhancecoeff->x21 = (-i_contrast * i_saturation * sinv) >> 5;
		enhancecoeff->x22 = (i_contrast * i_saturation * cosv) >> 5;
		enhancecoeff->x23 =
		    (1 << 19) - ((enhancecoeff->x22 + enhancecoeff->x21) << 7);
		enhancecoeff->x30 = 0;
		enhancecoeff->x31 = 0;
		enhancecoeff->x32 = 0;
		enhancecoeff->x33 = 4096;

		coeff[oper] = enhancecoeff;
		oper++;

	}

	if (outfmt == DE_FORMAT_SPACE_RGB) {
		if (infmt == DE_FORMAT_SPACE_RGB) {
			coeff[oper] = (struct __scal_matrix4x4 *) (y2r8bit + 0x20);
			oper++;

			if (out_color_range == DE_COLOR_RANGE_16_235) {
				coeff[oper] = (struct __scal_matrix4x4 *) (ir2r8bit);
				oper++;
			}
		} else {
			if (out_color_range == DE_COLOR_RANGE_16_235) {
				if (incscmod == DE_COLOR_SPACE_BT601) {
					coeff[oper] =
					    (struct __scal_matrix4x4 *)
						(y2r8bit + 0x80);
					oper++;
				} else if (incscmod == DE_COLOR_SPACE_BT709) {
					coeff[oper] =
					    (struct __scal_matrix4x4 *)
						(y2r8bit + 0xa0);
					oper++;
				}
			} else {
				if (incscmod == DE_COLOR_SPACE_BT601) {
					coeff[oper] =
					    (struct __scal_matrix4x4 *) (y2r8bit);
					oper++;
				} else if (incscmod == DE_COLOR_SPACE_BT709) {
					coeff[oper] =
					    (struct __scal_matrix4x4 *)
						(y2r8bit + 0x20);
					oper++;
				}
				/*else if (incscmod == DE_YCC) {
					coeff[oper] =
					    (struct __scal_matrix4x4 *)
						(y2r8bit + 0x40);
					oper++;
				} else if (incscmod == DE_ENHANCE) {
					coeff[oper] =
					    (struct __scal_matrix4x4 *)
						(y2r8bit + 0x60);
					oper++;
				}*/
			}
		}
	}
	/* matrix multiply */
	if (oper == 0) {
		memcpy(csc_coeff, bypass_csc8bit, sizeof(bypass_csc8bit));
	} else if (oper == 1) {
		for (i = 0; i < 12; i++)
			*(csc_coeff + i) =
			    IntRightShift64((int)(*((__s64 *) coeff[0] + i)),
					    oper << 1);
	} else {
		memcpy((void *)in0coeff, (void *)coeff[0],
		    sizeof(struct __scal_matrix4x4));
		for (i = 1; i < oper; i++) {
			in1coeff = coeff[i];
			IDE_SCAL_MATRIC_MUL(in1coeff, in0coeff, tmpcoeff);
			memcpy((void *)in0coeff, (void *)tmpcoeff,
				sizeof(struct __scal_matrix4x4));
		}

		for (i = 0; i < 12; i++)
			*(csc_coeff + i) =
			    IntRightShift64((int)(*((__s64 *) tmpcoeff + i)),
					    oper << 1);
	}

err:
	kfree(in0coeff);
	kfree(tmpcoeff);
	kfree(enhancecoeff);

	return 0;

}

int de_csc_coeff_calc_inner10bit(unsigned int infmt, unsigned int incscmod,
			    unsigned int outfmt, unsigned int outcscmod,
			    unsigned int brightness, unsigned int contrast,
			    unsigned int saturation, unsigned int hue,
			    unsigned int out_color_range, int *csc_coeff,
			    bool use_user_matrix)
{
	struct __scal_matrix4x4 *enhancecoeff, *tmpcoeff;
	struct __scal_matrix4x4 *coeff[5], *in0coeff, *in1coeff;
	int oper, i;
	int i_bright, i_contrast, i_saturation, i_hue, sinv, cosv;

	oper = 0;

	enhancecoeff = kmalloc(sizeof(struct __scal_matrix4x4), GFP_KERNEL | __GFP_ZERO);
	tmpcoeff = kmalloc(sizeof(struct __scal_matrix4x4), GFP_KERNEL | __GFP_ZERO);
	in0coeff = kmalloc(sizeof(struct __scal_matrix4x4), GFP_KERNEL | __GFP_ZERO);

	if (!enhancecoeff || !tmpcoeff || !in0coeff) {
		DE_WARN("kmalloc fail\n");
		goto err;
	}

	/* BYPASS */
	if (infmt == outfmt && incscmod == outcscmod &&
	    out_color_range == DE_COLOR_RANGE_0_255 && brightness == 50 &&
	    contrast == 50 && saturation == 50 && hue == 50 &&
	    !use_user_matrix) {
		memcpy(csc_coeff, bypass_csc10bit, 48);
		goto err;
	}

	/* dcsc's infmt=DE_RGB, use_user_matrix is used by dcsc, it's ok to do
	 * so*/
	if (use_user_matrix) {
		coeff[oper] = (struct __scal_matrix4x4 *)(user_matrix10bit);
		oper++;
	}

	/* NON-BYPASS */
	if (infmt == DE_FORMAT_SPACE_RGB) { // r2y
		/* convert to YCbCr */
		if (outfmt == DE_FORMAT_SPACE_RGB) {
			coeff[oper] = (struct __scal_matrix4x4 *)(ir2y10bit + 0x20);
			oper++;
		} else {
			if (outcscmod == DE_COLOR_SPACE_BT601) {
				coeff[oper] = (struct __scal_matrix4x4 *)(
						ir2y10bit); // r2ybt601 tv range
				oper++;
			} else if (outcscmod == DE_COLOR_SPACE_BT709) {
				coeff[oper] = (struct __scal_matrix4x4 *)(ir2y10bit
						+ 0x20); // r2ybt709 tv range
				oper++;
			} else if (outcscmod == DE_COLOR_SPACE_BT2020C
					   || outcscmod == DE_COLOR_SPACE_BT2020NC) {
				coeff[oper] = (struct __scal_matrix4x4 *)(ir2y10bit
						+ 0x80); // r2ybt2020 tv range
				oper++;
			}
		}
	} else { // y2y
		if (incscmod != outcscmod && outfmt == DE_YUV) {
			if (incscmod == DE_COLOR_SPACE_BT601 &&
			    outcscmod == DE_COLOR_SPACE_BT709) {
				coeff[oper] = (struct __scal_matrix4x4 *)(y2y10bit);
				oper++;
			} else if (incscmod == DE_COLOR_SPACE_BT709 &&
				   outcscmod == DE_COLOR_SPACE_BT601) {
				coeff[oper] = (struct __scal_matrix4x4 *)(y2y10bit + 0x20);
				oper++;
			} else if (incscmod == DE_COLOR_SPACE_BT601 &&
				   (outcscmod == DE_COLOR_SPACE_BT2020C
					|| outcscmod == DE_COLOR_SPACE_BT2020NC)) {
				coeff[oper] = (struct __scal_matrix4x4 *)(y2y10bit + 0x40);
				oper++;
			} else if ((incscmod == DE_COLOR_SPACE_BT2020NC
						|| incscmod == DE_COLOR_SPACE_BT2020C) &&
				   outcscmod == DE_COLOR_SPACE_BT601) {
				coeff[oper] = (struct __scal_matrix4x4 *)(y2y10bit + 0x60);
				oper++;
			} else if (incscmod == DE_COLOR_SPACE_BT709 &&
				   (outcscmod == DE_COLOR_SPACE_BT2020NC
					|| outcscmod == DE_COLOR_SPACE_BT2020C)) {
				coeff[oper] = (struct __scal_matrix4x4 *)(y2y10bit + 0x80);
				oper++;
			} else if ((incscmod == DE_COLOR_SPACE_BT2020NC
						|| incscmod == DE_COLOR_SPACE_BT2020C) &&
				   outcscmod == DE_COLOR_SPACE_BT709) {
				coeff[oper] = (struct __scal_matrix4x4 *)(y2y10bit + 0xa0);
				oper++;
			}
		}
	}

	// hsbcmat
	if (brightness != 50 || contrast != 50 || saturation != 50 ||
	    hue != 50) {
		brightness = brightness > 100 ? 100 : brightness;
		contrast = contrast > 100 ? 100 : contrast;
		saturation = saturation > 100 ? 100 : saturation;
		hue = hue > 100 ? 100 : hue;

		i_bright = (int)(brightness * 64 / 100);
		i_saturation = (int)(saturation * 64 / 100);
		i_contrast = (int)(contrast * 64 / 100);
		i_hue = (int)(hue * 64 / 100);

		sinv = sin_cos10bit[i_hue & 0x3f];
		cosv = sin_cos10bit[64 + (i_hue & 0x3f)];

		/* calculate enhance matrix */
		enhancecoeff->x00 = i_contrast << 7;
		enhancecoeff->x01 = 0;
		enhancecoeff->x02 = 0;
		enhancecoeff->x03 =
		    (((i_bright - 32) * 20 + 64) << 12) - (i_contrast << 13);
		enhancecoeff->x10 = 0;
		enhancecoeff->x11 = (i_contrast * i_saturation * cosv) >> 5;
		enhancecoeff->x12 = (i_contrast * i_saturation * sinv) >> 5;
		enhancecoeff->x13 =
		    (1 << 21) - ((enhancecoeff->x11 + enhancecoeff->x12) << 9);
		enhancecoeff->x20 = 0;
		enhancecoeff->x21 = (-i_contrast * i_saturation * sinv) >> 5;
		enhancecoeff->x22 = (i_contrast * i_saturation * cosv) >> 5;
		enhancecoeff->x23 =
		    (1 << 21) - ((enhancecoeff->x22 + enhancecoeff->x21) << 9);
		enhancecoeff->x30 = 0;
		enhancecoeff->x31 = 0;
		enhancecoeff->x32 = 0;
		enhancecoeff->x33 = 4096;

		coeff[oper] = enhancecoeff;
		oper++;
	}

	if (outfmt == DE_FORMAT_SPACE_RGB) {
		if (infmt == DE_FORMAT_SPACE_RGB) {
			coeff[oper] = (struct __scal_matrix4x4 *)(y2r10bit + 0x20);
			oper++;

			if (out_color_range == DE_COLOR_RANGE_16_235) {
				coeff[oper] = (struct __scal_matrix4x4 *)(ir2r10bit); // r2r
				oper++;
			}
		} else {
			if (out_color_range == DE_COLOR_RANGE_16_235) {
				if (incscmod == DE_COLOR_SPACE_BT601) {
					coeff[oper] = (struct __scal_matrix4x4 *)(y2r10bit
							+ 0x80); // y2rbt601 studio
					oper++;
				} else if (incscmod == DE_COLOR_SPACE_BT709) {
					coeff[oper] = (struct __scal_matrix4x4 *)(y2r10bit
							+ 0xa0); // y2rbt709 studio
					oper++;
				} else if (incscmod == DE_COLOR_SPACE_BT2020NC
						   || incscmod == DE_COLOR_SPACE_BT2020C) {
					coeff[oper] = (struct __scal_matrix4x4 *)(y2r10bit
							+ 0x100); // y2rbt2020 studio
					oper++;
				}
			} else {
				if (incscmod == DE_COLOR_SPACE_BT601) {
					coeff[oper] = (struct __scal_matrix4x4 *)(
							y2r10bit); // y2rbt601 TV rangne
					oper++;
				} else if (incscmod == DE_COLOR_SPACE_BT709) {
					coeff[oper] = (struct __scal_matrix4x4 *)(y2r10bit
							+ 0x20); // y2rbt709 TV range
					oper++;
				} else if (incscmod == DE_COLOR_SPACE_BT2020NC
						 || incscmod == DE_COLOR_SPACE_BT2020C) {
					coeff[oper] = (struct __scal_matrix4x4 *)(y2r10bit
							+ 0xc0); // y2rbt2020 TV range
					oper++;
				}

				/*else if (incscmod == DE_YCC) {
					coeff[oper] = (struct __scal_matrix4x4 *)
							(y2r10bit + 0x40);
					oper++;
				} else if (incscmod == DE_ENHANCE) {
					coeff[oper] = (struct __scal_matrix4x4 *)
							(y2r10bit + 0x60);
					oper++;
				}*/
			}
		}
	}
	/* matrix multiply */
	if (oper == 0) {
		memcpy(csc_coeff, bypass_csc10bit, sizeof(bypass_csc8bit));
	} else if (oper == 1) {
		for (i = 0; i < 12; i++)
			*(csc_coeff + i) = IntRightShift64(
			    (int)(*((__s64 *)coeff[0] + i)), oper << 1);
	} else {
		memcpy((void *)in0coeff, (void *)coeff[0],
		       sizeof(struct __scal_matrix4x4));
		for (i = 1; i < oper; i++) {
			in1coeff = coeff[i];
			IDE_SCAL_MATRIC_MUL(in1coeff, in0coeff, tmpcoeff);
			memcpy((void *)in0coeff, (void *)tmpcoeff,
			       sizeof(struct __scal_matrix4x4));
		}

		for (i = 0; i < 12; i++)
			*(csc_coeff + i) = IntRightShift64(
			    (int)(*((__s64 *)tmpcoeff + i)), oper << 1);
	}

err:
	kfree(in0coeff);
	kfree(tmpcoeff);
	kfree(enhancecoeff);

	return 0;
}

int de_dcsc_apply(unsigned int sel, struct disp_csc_config *config)
{
	int csc_coeff[12];
	unsigned int enhance_mode;
	/*struct de_dcsc_private *priv = &(dcsc_priv[sel]);
	struct __csc2_reg_t *reg = get_dcsc_reg(priv);*/
	struct de_rtmx_context *ctx = de_rtmx_get_context(sel);

    config->in_fmt = ctx->output.px_fmt_space;
    config->out_fmt = ctx->output.px_fmt_space;
    config->in_mode = ctx->output.color_space;
    config->out_mode = ctx->output.color_space;
    config->out_color_range = ctx->output.color_range;
    config->in_color_range = ctx->output.color_range;

	config->enhance_mode =
	    (config->enhance_mode > CSC_ENHANCE_MODE_NUM - 1)
	     ? g_dcsc_config[sel].enhance_mode : config->enhance_mode;
	enhance_mode = config->enhance_mode;
	config->brightness = _csc_enhance_setting[enhance_mode][0];
	config->contrast = _csc_enhance_setting[enhance_mode][1];
	config->saturation = _csc_enhance_setting[enhance_mode][2];
	config->hue = _csc_enhance_setting[enhance_mode][3];
	if (config->brightness < 50)
		config->brightness = config->brightness * 3 / 5 + 20;/*map from 0~100 to 20~100*/
	if (config->contrast < 50)
		config->contrast = config->contrast * 3 / 5 + 20;/*map from 0~100 to 20~100*/

	DE_INFO("sel=%d, in_fmt=%d, mode=%d, out_fmt=%d, mode=%d, range=%d\n",
	      sel, config->in_fmt, config->in_mode, config->out_fmt,
	      config->out_mode, config->out_color_range);
	DE_INFO("brightness=%d, contrast=%d, saturation=%d, hue=%d\n",
	      config->brightness, config->contrast, config->saturation,
	      config->hue);

	memcpy(&g_dcsc_config[sel], config, sizeof(struct disp_csc_config));
	if (de_rtmx_is_input_10bit(sel)) {
		de_csc_coeff_calc_inner10bit(
		    config->in_fmt, config->in_mode, config->out_fmt,
		    config->out_mode, config->brightness, config->contrast,
		    config->saturation, config->hue, config->out_color_range,
		    csc_coeff, use_user_matrix);
	} else {
		de_csc_coeff_calc_inner8bit(
		    config->in_fmt, config->in_mode, config->out_fmt,
		    config->out_mode, config->brightness, config->contrast,
		    config->saturation, config->hue, config->out_color_range,
		    csc_coeff, use_user_matrix);
	}

	/*reg->c00.dwval = *(csc_coeff);
	reg->c01.dwval = *(csc_coeff + 1);
	reg->c02.dwval = *(csc_coeff + 2);
	reg->c03.dwval = *(csc_coeff + 3) >> 6;
	reg->c10.dwval = *(csc_coeff + 4);
	reg->c11.dwval = *(csc_coeff + 5);
	reg->c12.dwval = *(csc_coeff + 6);
	reg->c13.dwval = *(csc_coeff + 7) >> 6;
	reg->c20.dwval = *(csc_coeff + 8);
	reg->c21.dwval = *(csc_coeff + 9);
	reg->c22.dwval = *(csc_coeff + 10);
	reg->c23.dwval = *(csc_coeff + 11) >> 6;
	reg->bypass.bits.enable = 1;

	priv->set_blk_dirty(priv, DCSC_REG_BLK_CTL, 1);
	priv->set_blk_dirty(priv, DCSC_REG_BLK_COEFF, 1);*/

	de_gamma_set_csc(sel, 1, csc_coeff, ctx->output.data_bits);

	return 0;
}

int de_dcsc_get_config(unsigned int sel, struct disp_csc_config *config)
{
	memcpy(config, &g_dcsc_config[sel], sizeof(struct disp_csc_config));

	return 0;
}

int de_dcsc_update_regs(unsigned int sel)
{
	/*updated by rtmx*/
	return 0;
}

s32 de_dcsc_get_reg_blocks(u32 disp, struct de_reg_block **blks,
			  u32 *blk_num)
{
	/*struct de_dcsc_private *priv = &(dcsc_priv[disp]);
	u32 i, num;

	if (blks == NULL) {
		*blk_num = priv->reg_blk_num;
		DE_WARN("get dcsc[%d] num=%d\n", disp, priv->reg_blk_num);
		return 0;
	}

	if (*blk_num >= priv->reg_blk_num) {
		num = priv->reg_blk_num;
	} else {
		num = *blk_num;
		DE_WARN("should not happen\n");
	}
	for (i = 0; i < num; ++i)
		blks[i] = priv->reg_blks + i;

	*blk_num = i;
	DE_WARN("get dcsc[%d] iinum=%d\n", disp, i);*/
	return 0;
}

int de_dcsc_init(uintptr_t reg_base)
{
/*	struct de_dcsc_private *priv;
	struct de_reg_mem_info *reg_mem_info;
	u32 rcq_used;
	struct de_reg_block *reg_blk;
	uintptr_t base;
	int screen_id, device_num;

	device_num = de_feat_get_num_screens();

	for (screen_id = 0; screen_id < device_num; screen_id++) {

		priv = &dcsc_priv[screen_id];
		reg_mem_info = &(priv->reg_mem_info);
		rcq_used = de_feat_is_using_rcq(screen_id);
		base = reg_base + DE_DISP_OFFSET(screen_id) + DISP_DRC_OFFSET;

		reg_mem_info->size = sizeof(struct __csc2_reg_t);

		reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
				reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
				rcq_used);

		if (reg_mem_info->vir_addr == NULL) {
			DE_WARN("alloc dcsc[%d] mm fail!size=0x%x\n",
				   screen_id, reg_mem_info->size);
			return -1;
		}

		priv->reg_blk_num = DCSC_REG_BLK_NUM;

		reg_blk = &(priv->reg_blks[DCSC_REG_BLK_CTL]);
		reg_blk->phy_addr = reg_mem_info->phy_addr;
		reg_blk->vir_addr = reg_mem_info->vir_addr;
		reg_blk->size = 0x4;
		reg_blk->reg_addr = (u8 __iomem *)base;

		reg_blk = &(priv->reg_blks[DCSC_REG_BLK_COEFF]);
		reg_blk->phy_addr = reg_mem_info->phy_addr + 0xc0;
		reg_blk->vir_addr = reg_mem_info->vir_addr + 0xc0;
		reg_blk->size = 0x30;
		reg_blk->reg_addr = (u8 __iomem *)(base + 0xc0);

		if (rcq_used)
			priv->set_blk_dirty = dcsc_set_rcq_head_dirty;
		else
			priv->set_blk_dirty = dcsc_set_block_dirty;
		DE_WARN("init dcsc[%d] size=0x%x\n", screen_id, reg_mem_info->size);
	}*/

	return 0;
}

int de_dcsc_exit(void)
{
	return 0;
}
