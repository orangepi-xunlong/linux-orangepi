/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2021 Copyright (c)
 *
 *  File name   :       de_deband_type.h
 *
 *  Description :       display engine 35x  basic function declaration
 *
 *  History     :       2021/10/29  v0.1  Initial version
 *
 ******************************************************************************/

#ifndef _DE_DITHER_TYPE_H_
#define _DE_DITHER_TYPE_H_

#include "linux/types.h"

/*offset:0x0000*/
union dither_ctl_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 dither_out_fmt:3;
		u32 dither_mode:4;
		u32 _3d_fifo_out:1;
		u32 res0:23;
	} bits;
};

/*offset:0x0004*/
union dither_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

/*offset:0x0008*/
union dither_rand_bits_reg {
	u32 dwval;
	struct {
		u32 rand_num_bits:4;
		u32 res0:28;
	} bits;
};

/*offset:0x000c+n*4*/
union dither_random_generator_reg {
	u32 dwval;
	struct {
		u32 rand_generator:32;
	} bits;
};

struct dither_reg {
    /*offset:0x0000*/
    union dither_ctl_reg ctl;
    /*offset:0x0004*/
    union dither_size_reg size;
    /*offset:0x0008*/
    union dither_rand_bits_reg rand_bits;
    /*offset:0x000c*/
    union dither_random_generator_reg random_generator[3];
};

enum dither_3d_fifo_mode {
    DISABLE = 0,
    LEFT_RIGHT_INTERLACE = 1,
};

enum dither_mode {
    QUANTIZATION = 0,
    FLOYD_STEINBERG = 1,
    ORDER = 3,/*565/666 not supported*/
    SIERRA_LITE = 4,
    BURKE = 5,
    RANDUM = 6,
};

enum dither_out_fmt {
    FMT888 = 0,
    FMT444 = 1,
    FMT565 = 2,
    FMT666 = 3,
};

struct dither_config {
    bool enable;
    enum dither_3d_fifo_mode _3d_fifo_mode;
    enum dither_mode mode;
    enum dither_out_fmt out_fmt;
};

#endif /* #ifndef _DE_DITHER_TYPE_H_ */
