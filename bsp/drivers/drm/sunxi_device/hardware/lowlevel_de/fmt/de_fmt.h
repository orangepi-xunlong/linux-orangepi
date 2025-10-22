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

#ifndef _DE_FMT_H_
#define _DE_FMT_H_

#include <linux/types.h>
#include "de_base.h"

struct de_fmt_config {
	unsigned int enable; /* return mod en info */

	/* parameter */
	unsigned int width; /* input size */
	unsigned int height; /* input height */

	/* output bitdepth compensation :
	* 0-8bit mode (when output device is cvbs/YPbPr/hdmi8bit etc)
	* 1-10bit mode (when output device is hdmi10bit etc)
	*/
	unsigned int bitdepth;

	/* output colorspace : 0-YUV444(RGB); 1-YUV422; 2-YUV420 */
	unsigned int colorspace;
	/* output pixel format :
	*     colorspace = 0 :
	*         0-YCbCr(RGB); 1-CbYCr(GRB);
	*     colorspace = 1 :
	*         0-CbY/CrY; 1-YCbCr/YCbCr;
	*     colorspace = 2 :
	*         0-CbYY/CrYY; 1-YYCb/YYCr;
	*/
	unsigned int pixelfmt;

	/* horizontal low-pass-filter coefficients selection for 2
	* chroma channels :
	* 0-subsample phase = 0, and 6-tap LPF (Recommended!)
	* 1-subsample phase = 1, and 6-tap LPF
	* 2-subsample phase = 0.5, and 6-tap LPF
	* 3-subsample phase = 1.5, and 6-tap LPF
	* 4-subsample phase = 0, and no LPF
	* 5-subsample phase = 1, and no LPF
	* 6-subsample phase = 0.5, and 2-tap average
	* 7-subsample phase = 1.5, and 2-tap average
	*/
	unsigned int hcoef_sel_c0;
	unsigned int hcoef_sel_c1;
	/* vertical low-pass-filter coefficients selection for 2
	* chroma channels :
	* 0-subsample phase = 0.5, and 2-tap average (Recommended!)
	* 1-subsample phase = 0, and no LPF
	* 2-subsample phase = 1, and no LPF
	*/
	unsigned int vcoef_sel_c0;
	unsigned int vcoef_sel_c1;
	unsigned int swap_enable; /* swap Cb and Cr channel input data  */

};

struct de_fmt_handle {
	struct module_create_info cinfo;
	u32 disp_reg_base;
	unsigned int block_num;
	struct de_reg_block **block;
	struct de_fmt_private *private;
};

struct de_fmt_info {
	enum de_format_space px_fmt_space;
	enum de_yuv_sampling yuv_sampling;
	enum de_data_bits bits;
	u32 width;
	u32 height;
};

s32 de_fmt_apply(struct de_fmt_handle *hdl, const struct de_fmt_info *out_info);
struct de_fmt_handle *de_fmt_create(struct module_create_info *info);
void de_fmt_dump_state(struct drm_printer *p, struct de_fmt_handle *hdl);

#endif /* #ifndef _DE_FMT_H_ */
