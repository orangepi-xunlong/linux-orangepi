/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * mars11_ccic_uapi.h - Driver uapi for KY X1 CCIC
 *
 * Copyright (C) 2024 KY Micro Limited
 */

#ifndef _UAPI_LINUX_X1_CCIC_H_
#define _UAPI_LINUX_X1_CCIC_H_
//#include <linux/videodev2.h>

enum {
	CCIC_MODE_NM = 0,
	CCIC_MODE_VC,
	CCIC_MODE_VCDT,
};

enum {
	CCIC_CH_MODE_MAIN = 0,
	CCIC_CH_MODE_SUB,
};

struct v4l2_ccic_params {
	unsigned int lane_num;
	int ccic_mode;
	int ch_mode;
	unsigned int main_ccic_id;
	unsigned int main_vc;
	unsigned int sub_vc;
	unsigned int main_dt;
	unsigned int sub_dt;
};


#define BASE_VIDIOC_CCIC		(BASE_VIDIOC_PRIVATE + 20)
#define VIDIOC_CCIC_S_PARAMS	_IOWR('V', BASE_VIDIOC_CCIC + 1, struct v4l2_ccic_params)
#endif
