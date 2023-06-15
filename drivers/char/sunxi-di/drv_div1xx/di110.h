/*
 * Copyright (c) 2020-2031 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef _DI_110_H_
#define _DI_110_H_

enum di_pixel_format {
	YUV420_PLANNER = 0,
	YUV420_NV12,
	YUV420_NV21,
	YUV422_PLANNER,
	YUV422_NV16,
	YUV422_NV61,
};

enum di_format_type {
	PLANNER_FORMAT = 0,
	UVCOMBINE_FORMAT = 1,
};
#endif
