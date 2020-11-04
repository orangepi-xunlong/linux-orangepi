/*
* Allwinner SoCs display driver.
*
* Copyright (C) 2017 Allwinner.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#ifndef _DE_FMT_TYPE_H_
#define _DE_FMT_TYPE_H_

#include <linux/types.h>

union fmt_ctl_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 res0:31;
	} bits;
};

union fmt_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

/* swap chroma */
union fmt_swap_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 res0:31;
	} bits;
};

/*
* out_bit_mode:
* 0: 8-bit;
* 1: 10-bit
*/
union fmt_bitdepth_reg {
	u32 dwval;
	struct {
		u32 out_bit_mode:1;
		u32 res0:31;
	} bits;
};

/*
* fmt_space:
* 0: yuv444
* 1: yuv422
* 2: yuv420
*/
union fmt_format_type_reg {
	u32 dwval;
	struct {
		u32 fmt_space:2;
		u32 res0:14;
		u32 px_fmt_type:2;
		u32 res1:14;
	} bits;
};

union fmt_coeff_reg {
	u32 dwval;
	struct {
		u32 c0_hcoeff:3;
		u32 res0:5;
		u32 c0_vcoeff:2;
		u32 res1:6;
		u32 c1_hcoeff:3;
		u32 res2:5;
		u32 c1_vcoeff:2;
		u32 res3:6;
	} bits;
};

union fmt_func_version_reg {
	u32 dwval;
	struct {
		u32 support_444to420:1;
		u32 res0:1;
		u32 max_width:2;
		u32 res1:28;
	} bits;
};

union fmt_ip_version_reg {
	u32 dwval;
	struct {
		u32 ip_ver:32;
	} bits;
};

union fmt_limit_setting_reg {
	u32 dwval;
	struct {
		u32 low:10;
		u32 res0:6;
		u32 high:10;
		u32 res1:6;
	} bits;
};

struct fmt_reg {
	union fmt_ctl_reg ctl;
	union fmt_size_reg size;
	union fmt_swap_reg swap;
	union fmt_bitdepth_reg bitdepth;
	union fmt_format_type_reg fmt_type;
	union fmt_coeff_reg coeff;
	union fmt_func_version_reg func_ver;
	union fmt_ip_version_reg ip_ver;
	union fmt_limit_setting_reg limit_y;
	union fmt_limit_setting_reg limit_c0;
	union fmt_limit_setting_reg limit_c1;
};

#endif /* #ifndef _DE_FMT_TYPE_H_ */
