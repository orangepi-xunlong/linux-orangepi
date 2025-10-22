/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */


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

#endif /* #ifndef _DE_DITHER_TYPE_H_ */
