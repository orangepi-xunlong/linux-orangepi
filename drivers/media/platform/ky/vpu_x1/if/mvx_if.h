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

#ifndef _MVX_IF_H_
#define _MVX_IF_H_

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/kref.h>
#include <linux/list.h>
#include "mvx_mmu.h"

/****************************************************************************
 * Defines
 ****************************************************************************/

/**
 * The name of the device driver.
 */
#define MVX_IF_NAME     "amvx_if"

/****************************************************************************
 * Types
 ****************************************************************************/

struct device;
struct mvx_client_ops;
struct mvx_client_session;
struct mvx_if_ctx;
struct platform_device;

/**
 * enum mvx_direction - Direction from the point of view of the hardware block.
 */
enum mvx_direction {
	MVX_DIR_INPUT,
	MVX_DIR_OUTPUT,
	MVX_DIR_MAX
};

/**
 * enum mvx_tristate - Tristate boolean variable.
 */
enum mvx_tristate {
	MVX_TRI_UNSET = -1,
	MVX_TRI_TRUE  = 0,
	MVX_TRI_FALSE = 1
};

/**
 * enum mvx_format - List of compressed formats and frame formats.
 *
 * Enumeration of formats that are supported by all know hardware revisions.
 *
 * The enumeration should start at 0 and should not contain any gaps.
 */
enum mvx_format {
	/* Compressed formats. */
	MVX_FORMAT_BITSTREAM_FIRST,
	MVX_FORMAT_AVS = MVX_FORMAT_BITSTREAM_FIRST,
	MVX_FORMAT_AVS2,
	MVX_FORMAT_H263,
	MVX_FORMAT_H264,
	MVX_FORMAT_HEVC,
	MVX_FORMAT_JPEG,
	MVX_FORMAT_MPEG2,
	MVX_FORMAT_MPEG4,
	MVX_FORMAT_RV,
	MVX_FORMAT_VC1,
	MVX_FORMAT_VP8,
	MVX_FORMAT_VP9,
	MVX_FORMAT_BITSTREAM_LAST = MVX_FORMAT_VP9,

	/* Uncompressed formats. */
	MVX_FORMAT_FRAME_FIRST,
	MVX_FORMAT_YUV420_AFBC_8 = MVX_FORMAT_FRAME_FIRST,
	MVX_FORMAT_YUV420_AFBC_10,
	MVX_FORMAT_YUV422_AFBC_8,
	MVX_FORMAT_YUV422_AFBC_10,
	MVX_FORMAT_YUV420_I420,
	MVX_FORMAT_YUV420_NV12,
	MVX_FORMAT_YUV420_NV21,
	MVX_FORMAT_YUV420_P010,
	MVX_FORMAT_YUV420_Y0L2,
	MVX_FORMAT_YUV420_AQB1,
	MVX_FORMAT_YUV422_YUY2,
	MVX_FORMAT_YUV422_UYVY,
	MVX_FORMAT_YUV422_Y210,
	MVX_FORMAT_RGBA_8888,
	MVX_FORMAT_BGRA_8888,
	MVX_FORMAT_ARGB_8888,
	MVX_FORMAT_ABGR_8888,
	MVX_FORMAT_FRAME_LAST = MVX_FORMAT_ABGR_8888,

	MVX_FORMAT_MAX
};

/**
 * enum mvx_hw_id - Enumeration of known hardware revisions.
 */
enum mvx_hw_id {
	MVE_Unknown = 0x0,
	MVE_v500    = 0x500,
	MVE_v550    = 0x550,
	MVE_v61     = 0x61,
	MVE_v52_v76 = 0x5276
};

/**
 * struct mvx_hw_ver - Hardware version.
 */
struct mvx_hw_ver {
	enum mvx_hw_id id;
	uint32_t revision;
	uint32_t patch;
};

/**
 * enum mvx_nalu_format - NALU format.
 */
enum mvx_nalu_format {
	MVX_NALU_FORMAT_UNDEFINED,
	MVX_NALU_FORMAT_START_CODES,
	MVX_NALU_FORMAT_ONE_NALU_PER_BUFFER,
	MVX_NALU_FORMAT_ONE_BYTE_LENGTH_FIELD,
	MVX_NALU_FORMAT_TWO_BYTE_LENGTH_FIELD,
	MVX_NALU_FORMAT_FOUR_BYTE_LENGTH_FIELD
};

/**
 * enum mvx_profile - Profile for encoder.
 */
enum mvx_profile {
	MVX_PROFILE_NONE,

	MVX_PROFILE_H264_BASELINE,
	MVX_PROFILE_H264_MAIN,
	MVX_PROFILE_H264_HIGH,
	MVX_PROFILE_H264_HIGH_10,

	MVX_PROFILE_H265_MAIN,
	MVX_PROFILE_H265_MAIN_STILL,
	MVX_PROFILE_H265_MAIN_INTRA,

	MVX_PROFILE_VC1_SIMPLE,
	MVX_PROFILE_VC1_MAIN,
	MVX_PROFILE_VC1_ADVANCED,

	MVX_PROFILE_VP8_MAIN
};

/**
 * enum mvx_level - Level for encoder.
 */
enum mvx_level {
	MVX_LEVEL_NONE,

	MVX_LEVEL_H264_1,
	MVX_LEVEL_H264_1b,
	MVX_LEVEL_H264_11,
	MVX_LEVEL_H264_12,
	MVX_LEVEL_H264_13,
	MVX_LEVEL_H264_2,
	MVX_LEVEL_H264_21,
	MVX_LEVEL_H264_22,
	MVX_LEVEL_H264_3,
	MVX_LEVEL_H264_31,
	MVX_LEVEL_H264_32,
	MVX_LEVEL_H264_4,
	MVX_LEVEL_H264_41,
	MVX_LEVEL_H264_42,
	MVX_LEVEL_H264_5,
	MVX_LEVEL_H264_51,
	MVX_LEVEL_H264_52,
	MVX_LEVEL_H264_6,
	MVX_LEVEL_H264_61,
	MVX_LEVEL_H264_62,

	MVX_LEVEL_H265_MAIN_1,
	MVX_LEVEL_H265_HIGH_1,
	MVX_LEVEL_H265_MAIN_2,
	MVX_LEVEL_H265_HIGH_2,
	MVX_LEVEL_H265_MAIN_21,
	MVX_LEVEL_H265_HIGH_21,
	MVX_LEVEL_H265_MAIN_3,
	MVX_LEVEL_H265_HIGH_3,
	MVX_LEVEL_H265_MAIN_31,
	MVX_LEVEL_H265_HIGH_31,
	MVX_LEVEL_H265_MAIN_4,
	MVX_LEVEL_H265_HIGH_4,
	MVX_LEVEL_H265_MAIN_41,
	MVX_LEVEL_H265_HIGH_41,
	MVX_LEVEL_H265_MAIN_5,
	MVX_LEVEL_H265_HIGH_5,
	MVX_LEVEL_H265_MAIN_51,
	MVX_LEVEL_H265_HIGH_51,
	MVX_LEVEL_H265_MAIN_52,
	MVX_LEVEL_H265_HIGH_52,
	MVX_LEVEL_H265_MAIN_6,
	MVX_LEVEL_H265_HIGH_6,
	MVX_LEVEL_H265_MAIN_61,
	MVX_LEVEL_H265_HIGH_61,
	MVX_LEVEL_H265_MAIN_62,
	MVX_LEVEL_H265_HIGH_62
};

/**
 * enum mvx_gop_type - GOP type for encoder.
 */
enum mvx_gop_type {
	MVX_GOP_TYPE_NONE,
	MVX_GOP_TYPE_BIDIRECTIONAL,
	MVX_GOP_TYPE_LOW_DELAY,
	MVX_GOP_TYPE_PYRAMID
};

/**
 * enum mvx_entropy_mode - Entropy mode for encoder.
 */
enum mvx_entropy_mode {
	MVX_ENTROPY_MODE_NONE,
	MVX_ENTROPY_MODE_CAVLC,
	MVX_ENTROPY_MODE_CABAC
};

/**
 * enum mvx_multi_slice_mode - Multi slice mode.
 */
enum mvx_multi_slice_mode {
	MVX_MULTI_SLICE_MODE_SINGLE,
	MVX_MULTI_SLICE_MODE_MAX_MB
};

/**
 * enum mvx_vp9_prob_update - Probability update method.
 */
enum mvx_vp9_prob_update {
	MVX_VP9_PROB_UPDATE_DISABLED,
	MVX_VP9_PROB_UPDATE_IMPLICIT,
	MVX_VP9_PROB_UPDATE_EXPLICIT
};

/**
 * enum mvx_rgb_to_yuv_mode - RGB to YUV conversion mode.
 */
enum mvx_rgb_to_yuv_mode {
	MVX_RGB_TO_YUV_MODE_BT601_STUDIO,
	MVX_RGB_TO_YUV_MODE_BT601_FULL,
	MVX_RGB_TO_YUV_MODE_BT709_STUDIO,
	MVX_RGB_TO_YUV_MODE_BT709_FULL
};

/**
 * struct mvx_if_session - Structure holding members needed to map a session to
 *                         a hardare device.
 * @kref:	Reference counter for the session object.
 * @release:	Function pointer that shall be passed to kref_put. If the
 *              reference count reaches 0 this function will be called to
 *              destruct and deallocate the object.
 * @ncores:	Number of cores this session has been mapped to.
 * @l0_pte:	Level 0 page table entry. This value is written to the hardware
 *              MMU CTRL register to point out the location of the L1 page table
 *              and to set access permissions and bus attributes.
 * @securevideo:Secure video enabled.
 */
struct mvx_if_session {
	struct kref kref;
	struct mutex *mutex;
	void (*release)(struct kref *kref);
	unsigned int ncores;
	mvx_mmu_pte l0_pte;
	bool securevideo;
};

/**
 * struct mvx_if_ops - Functions pointers the registered device may use to call
 *                     the if device.
 */
struct mvx_if_ops {
	/**
	 * irq() - Handle IRQ sent from firmware to driver.
	 */
	void (*irq)(struct mvx_if_session *session);
};

/**
 * struct mvx_client_ops - Functions pointers the if device may use to call
 *                         the registered device.
 */
struct mvx_client_ops {
	struct list_head list;

	/**
	 * get_hw_ver() - Get MVE hardware version
	 */
	void (*get_hw_ver)(struct mvx_client_ops *client,
			   struct mvx_hw_ver *version);

	/**
	 * get_formats() - Get list of supported formats.
	 *
	 * Return: 0 on success, else error code.
	 */
	void (*get_formats)(struct mvx_client_ops *client,
			    enum mvx_direction direction,
			    uint64_t *formats);

	/**
	 * get_ncores() - Get number of cores.
	 *
	 * Return: Number of cores on success, else error code.
	 */
	unsigned int (*get_ncores)(struct mvx_client_ops *client);

	/*
	 * SESSION.
	 */

	/**
	 * register_session() - Register if session with client.
	 *
	 * Return: Client session handle on success, else ERR_PTR.
	 */
	struct mvx_client_session
	*(*register_session)(struct mvx_client_ops *client,
			     struct mvx_if_session *session);

	/**
	 * unregister_session() - Unregister session.
	 *
	 * Return: 0 on success, else error code.
	 */
	void (*unregister_session)(struct mvx_client_session *session);

	/**
	 * switch_in() - Switch in session.
	 *
	 * After a session has been switched in it must wait for a 'switched
	 * out' event before it is allowed to requested to be switched in again.
	 * Switching in a already switched in session is regarded as an error.
	 *
	 * Return: 0 on success, else error code.
	 */
	int (*switch_in)(struct mvx_client_session *session);

	/**
	 * send_irq() - Send IRQ from driver to firmware.
	 *
	 * Return: 0 on success, else error code.
	 */
	int (*send_irq)(struct mvx_client_session *session);

	/**
	 * flush_mmu() - Flush MMU tables.
	 *
	 * Flushing MMU tables is only required if pages have been removed
	 * from the page tables.
	 *
	 * Return: 0 on success, else error code.
	 */
	int (*flush_mmu)(struct mvx_client_session *session);

	/**
	 * print_debug() - Print debug information.
	 *
	 * Return: 0 on success, else error code.
	 */
	void (*print_debug)(struct mvx_client_session *session);

	void (*wait_session_idle)(struct mvx_client_session *session);
};

/****************************************************************************
 * Static functions
 ****************************************************************************/

/**
 * mvx_is_bitstream(): Detect if format is of type bitstream.
 * @format:	Format.
 *
 * Return: True if format is bitstream, else false.
 */
static inline bool mvx_is_bitstream(enum mvx_format format)
{
	return (format >= MVX_FORMAT_BITSTREAM_FIRST) &&
	       (format <= MVX_FORMAT_BITSTREAM_LAST);
}

/**
 * mvx_is_frame(): Detect if format is of type frame.
 * @format:	Format.
 *
 * Return: True if format is frame, else false.
 */
static inline bool mvx_is_frame(enum mvx_format format)
{
	return (format >= MVX_FORMAT_FRAME_FIRST) &&
	       (format <= MVX_FORMAT_FRAME_LAST);
}

/**
 * mvx_is_rgb(): Detect if format is of type RGB.
 * @format:	Format.
 *
 * Return: True if format is RGB, else false.
 */
static inline bool mvx_is_rgb(enum mvx_format format)
{
	return (format >= MVX_FORMAT_RGBA_8888) &&
	       (format <= MVX_FORMAT_ABGR_8888);
}

/**
 * mvx_is_afbc(): Detect if format is of type AFBC.
 * @format:	Format.
 *
 * Return: True if format is AFBC, else false.
 */
static inline bool mvx_is_afbc(enum mvx_format format)
{
	return (format >= MVX_FORMAT_YUV420_AFBC_8) &&
	       (format <= MVX_FORMAT_YUV422_AFBC_10);
}

/****************************************************************************
 * Exported functions
 ****************************************************************************/

/**
 * mvx_if_create() - Create IF device.
 */
struct mvx_if_ops *mvx_if_create(struct device *dev,
				 struct mvx_client_ops *client_ops,
				 void *priv);

/**
 * mvx_if_destroy() - Destroy IF device.
 */
void mvx_if_destroy(struct mvx_if_ops *if_ops);

#endif /* _MVX_IF_H_ */
