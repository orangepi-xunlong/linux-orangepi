/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_CCSC_TYPE_H_
#define _DE_CCSC_TYPE_H_

#include <linux/types.h>

union ccsc_ctl_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 res0:31;
	} bits;
};

union ccsc_const_reg {
	u32 dwval;
	struct {
		u32 d:10;
		u32 res0:22;
	} bits;
};

union ccsc_coeff_reg {
	u32 dwval;
	struct {
		u32 c:20;
		u32 res0:12;
	} bits;
};

union ccsc_alpha_reg {
	u32 dwval;
	struct {
		u32 alpha:8;
		u32 en:1;
		u32 res0:23;
	} bits;
};

struct ccsc_reg {
	union ccsc_ctl_reg ctl;

	union ccsc_const_reg d0;
	union ccsc_const_reg d1;
	union ccsc_const_reg d2;

	union ccsc_coeff_reg c0[4];
	union ccsc_coeff_reg c1[4];
	union ccsc_coeff_reg c2[4];

	union ccsc_alpha_reg alpha;
};

#endif /* #ifndef _DE_CCSC_TYPE_H_ */
