/*
 * de_snr_type.h
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
#ifndef _DE_SNR_TYPE_H
#define _DE_SNR_TYPE_H

union snr_ctrl_reg {
	unsigned int dwval;
	struct {
		unsigned int en:1;
		unsigned int res0:30;
		unsigned int demo_en:1;
	} bits;
};

union snr_size_reg {
	unsigned int dwval;
	struct {
		unsigned int width:13;
		unsigned int res0:3;
		unsigned int height:13;
		unsigned int res1:3;
	} bits;
};

union snr_strengths_reg {
	unsigned int dwval;
	struct {
		unsigned int strength_v:5;
		unsigned int res0:3;
		unsigned int strength_u:5;
		unsigned int res1:3;
		unsigned int strength_y:5;
		unsigned int res2:11;
	} bits;
};

union snr_line_det_reg {
	unsigned int dwval;
	struct {
		unsigned int th_hor_line:8;
		unsigned int res0:8;
		unsigned int th_ver_line:8;
		unsigned int res1:8;
	} bits;
};

union snr_demo_win_hor_reg {
	unsigned int dwval;
	struct {
		unsigned int demo_horz_start:13;
		unsigned int res0:3;
		unsigned int demo_horz_end:13;
		unsigned int res1:3;
	} bits;
};

union snr_demo_win_ver_reg {
	unsigned int dwval;
	struct {
		unsigned int demo_vert_start:13;
		unsigned int res0:3;
		unsigned int demo_vert_end:13;
		unsigned int res1:3;
	} bits;
};

struct snr_reg {
	union snr_ctrl_reg snr_ctrl;
	union snr_size_reg snr_size;
	union snr_strengths_reg snr_strength;
	union snr_line_det_reg snr_line_det;
	union snr_demo_win_hor_reg demo_win_hor;
	union snr_demo_win_ver_reg demo_win_ver;
};

#endif /*End of file*/
