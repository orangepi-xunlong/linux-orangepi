/*
 * de_ksc_type/de_ksc_type.h
 *
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _DE_KSC_TYPE_H
#define _DE_KSC_TYPE_H

union ksc_ctrl_reg {
	unsigned int dwval;
	struct {
		unsigned int en:1;
		unsigned int res0:31;
	} bits;
};

union ksc_fw_reg {
	unsigned int dwval;
	struct {
		unsigned int first_line_width:12;
		unsigned int res0:20;
	} bits;
};

union ksc_cvsf_reg {
	unsigned int dwval;
	struct {
		unsigned int frac_ration:12;
		unsigned int int_ration:8;
		unsigned int res0:11;
		unsigned int sign_bit:1;
	} bits;
};

union ksc_size_reg {
	unsigned int dwval;
	struct {
		unsigned int width:13;
		unsigned int res0:3;
		unsigned int height:13;
		unsigned int res1:3;
	} bits;
};

union ksc_coef_reg {
	unsigned int dwval;
	struct {
		unsigned int tap0:8;
		unsigned int tap1:8;
		unsigned int tap2:8;
		unsigned int tap3:8;
	} bits;
};

union ksc_correct_color_reg {
	unsigned int dwval;
	struct {
		unsigned int blue_v:8;
		unsigned int green_u:8;
		unsigned int red_y:8;
		unsigned int res0:8;
	} bits;
};

struct ksc_reg {
	union ksc_ctrl_reg ksc_ctrl;
	union ksc_fw_reg ksc_fw;
	union ksc_cvsf_reg ksc_cvsf;
	union ksc_size_reg ksc_size;
	union ksc_coef_reg coef[8];
	union ksc_correct_color_reg ksc_color;
};

#endif /*End of file*/
