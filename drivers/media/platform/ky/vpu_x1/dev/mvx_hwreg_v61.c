/*
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from Arm Technology (China) Co., Ltd.
 *
 *            (C) COPYRIGHT 2021-2021 Arm Technology (China) Co., Ltd.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from Arm Technology (China) Co., Ltd.
 * 
 * SPDX-License-Identifier: GPL-2.0-only
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 */

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/io.h>
#include "mvx_bitops.h"
#include "mvx_hwreg_v61.h"

/****************************************************************************
 * Exported functions
 ****************************************************************************/

void mvx_hwreg_get_formats_v61(enum mvx_direction direction,
			       uint64_t *formats)
{
	if (direction == MVX_DIR_INPUT) {
		mvx_set_bit(MVX_FORMAT_H263, formats);
		mvx_set_bit(MVX_FORMAT_H264, formats);
		mvx_set_bit(MVX_FORMAT_HEVC, formats);
		mvx_set_bit(MVX_FORMAT_JPEG, formats);
		mvx_set_bit(MVX_FORMAT_MPEG2, formats);
		mvx_set_bit(MVX_FORMAT_MPEG4, formats);
		mvx_set_bit(MVX_FORMAT_RV, formats);
		mvx_set_bit(MVX_FORMAT_VC1, formats);
		mvx_set_bit(MVX_FORMAT_VP8, formats);
		mvx_set_bit(MVX_FORMAT_VP9, formats);
		mvx_set_bit(MVX_FORMAT_YUV420_AFBC_8, formats);
		mvx_set_bit(MVX_FORMAT_YUV420_AFBC_10, formats);
		mvx_set_bit(MVX_FORMAT_YUV422_AFBC_8, formats);
		mvx_set_bit(MVX_FORMAT_YUV422_AFBC_10, formats);
		mvx_set_bit(MVX_FORMAT_YUV420_I420, formats);
		mvx_set_bit(MVX_FORMAT_YUV420_NV12, formats);
		mvx_set_bit(MVX_FORMAT_YUV420_NV21, formats);
		mvx_set_bit(MVX_FORMAT_YUV420_P010, formats);
		mvx_set_bit(MVX_FORMAT_YUV420_Y0L2, formats);
		mvx_set_bit(MVX_FORMAT_YUV420_AQB1, formats);
		mvx_set_bit(MVX_FORMAT_YUV422_YUY2, formats);
		mvx_set_bit(MVX_FORMAT_YUV422_UYVY, formats);
		mvx_set_bit(MVX_FORMAT_YUV422_Y210, formats);
		mvx_set_bit(MVX_FORMAT_RGBA_8888, formats);
		mvx_set_bit(MVX_FORMAT_BGRA_8888, formats);
		mvx_set_bit(MVX_FORMAT_ARGB_8888, formats);
		mvx_set_bit(MVX_FORMAT_ABGR_8888, formats);
	} else {
		mvx_set_bit(MVX_FORMAT_H264, formats);
		mvx_set_bit(MVX_FORMAT_HEVC, formats);
		mvx_set_bit(MVX_FORMAT_JPEG, formats);
		mvx_set_bit(MVX_FORMAT_VP8, formats);
		mvx_set_bit(MVX_FORMAT_VP9, formats);
		mvx_set_bit(MVX_FORMAT_YUV420_AFBC_8, formats);
		mvx_set_bit(MVX_FORMAT_YUV420_AFBC_10, formats);
		mvx_set_bit(MVX_FORMAT_YUV422_AFBC_8, formats);
		mvx_set_bit(MVX_FORMAT_YUV422_AFBC_10, formats);
		mvx_set_bit(MVX_FORMAT_YUV420_I420, formats);
		mvx_set_bit(MVX_FORMAT_YUV420_NV12, formats);
		mvx_set_bit(MVX_FORMAT_YUV420_NV21, formats);
		mvx_set_bit(MVX_FORMAT_YUV420_P010, formats);
		mvx_set_bit(MVX_FORMAT_YUV420_Y0L2, formats);
		mvx_set_bit(MVX_FORMAT_YUV420_AQB1, formats);
		mvx_set_bit(MVX_FORMAT_YUV422_YUY2, formats);
		mvx_set_bit(MVX_FORMAT_YUV422_UYVY, formats);
		mvx_set_bit(MVX_FORMAT_YUV422_Y210, formats);
	}
}
