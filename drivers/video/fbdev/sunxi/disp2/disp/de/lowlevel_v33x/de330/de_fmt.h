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
#include "de_top.h"

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

s32 de_fmt_set_para(u32 disp);

s32 de_fmt_init(u32 disp, u8 __iomem *de_reg_base);
s32 de_fmt_exit(u32 disp);
s32 de_fmt_get_reg_blocks(u32 disp,
	struct de_reg_block **blks, u32 *blk_num);

#endif /* #ifndef _DE_FMT_H_ */
