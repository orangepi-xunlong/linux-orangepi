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

#ifndef _MVX_V4L2_CONTROLS_H_
#define _MVX_V4L2_CONTROLS_H_

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>
#include <drm/drm_fourcc.h>

/****************************************************************************
 * Pixel formats
 ****************************************************************************/

#define V4L2_PIX_FMT_YUV420_AFBC_8   v4l2_fourcc('Y', '0', 'A', '8')
#define V4L2_PIX_FMT_YUV420_AFBC_10  v4l2_fourcc('Y', '0', 'A', 'A')
#define V4L2_PIX_FMT_YUV422_AFBC_8   v4l2_fourcc('Y', '2', 'A', '8')
#define V4L2_PIX_FMT_YUV422_AFBC_10  v4l2_fourcc('Y', '2', 'A', 'A')
#define V4L2_PIX_FMT_Y210            v4l2_fourcc('Y', '2', '1', '0')
//#define V4L2_PIX_FMT_P010            v4l2_fourcc('Y', '0', 'P', '1')
#define V4L2_PIX_FMT_Y0L2            v4l2_fourcc('Y', '0', 'Y', 'L')
#define V4L2_PIX_FMT_RV              v4l2_fourcc('R', 'V', '0', '0')

#ifndef V4L2_PIX_FMT_HEVC
#define V4L2_PIX_FMT_HEVC            v4l2_fourcc('H', 'E', 'V', 'C')
#endif

#ifndef V4L2_PIX_FMT_VP9
#define V4L2_PIX_FMT_VP9             v4l2_fourcc('V', 'P', '9', '0')
#endif

#define V4L2_PIX_FMT_AVS              v4l2_fourcc('A', 'V', 'S', '1')
#define V4L2_PIX_FMT_AVS2             v4l2_fourcc('A', 'V', 'S', '2')

/****************************************************************************
 * Buffers
 * @see v4l2_buffer
 ****************************************************************************/

/*
 * Extended buffer flags.
 */
/*
#define V4L2_BUF_FLAG_MVX_DECODE_ONLY           0x01000000
#define V4L2_BUF_FLAG_MVX_CODEC_CONFIG          0x02000000
#define V4L2_BUF_FLAG_MVX_AFBC_TILED_HEADERS    0x10000000
#define V4L2_BUF_FLAG_MVX_AFBC_TILED_BODY       0x20000000
#define V4L2_BUF_FLAG_MVX_AFBC_32X8_SUPERBLOCK  0x40000000
#define V4L2_BUF_FLAG_MVX_MASK                  0xff000000
#define V4L2_BUF_FLAG_END_OF_SUB_FRAME          0x04000000
#define V4L2_BUF_FLAG_MVX_BUFFER_FRAME_PRESENT  0x08000000
#define V4L2_BUF_FLAG_MVX_BUFFER_NEED_REALLOC   0x07000000


#define V4L2_BUF_FRAME_FLAG_ROTATION_90   0x81000000
#define V4L2_BUF_FRAME_FLAG_ROTATION_180  0x82000000
#define V4L2_BUF_FRAME_FLAG_ROTATION_270  0x83000000
#define V4L2_BUF_FRAME_FLAG_ROTATION_MASK 0x83000000
#define V4L2_BUF_FRAME_FLAG_MIRROR_HORI   0x90000000
#define V4L2_BUF_FRAME_FLAG_MIRROR_VERT   0xA0000000
#define V4L2_BUF_FRAME_FLAG_MIRROR_MASK   0xB0000000
#define V4L2_BUF_FRAME_FLAG_SCALING_2     0x84000000
#define V4L2_BUF_FRAME_FLAG_SCALING_4     0x88000000
#define V4L2_BUF_FRAME_FLAG_SCALING_MASK  0x8C000000

#define V4L2_BUF_FLAG_MVX_BUFFER_EPR      0xC0000000
#define V4L2_BUF_FLAG_MVX_BUFFER_ROI      0x70000000
*/
//redefine these flags
/*use encode/decode frame/bitstream to update these flags*/

#define V4L2_BUF_FLAG_MVX_MASK                  0xff000000

//for decode frame flag
#define V4L2_BUF_FRAME_FLAG_ROTATION_90         0x01000000  /* Frame is rotated 90 degrees */
#define V4L2_BUF_FRAME_FLAG_ROTATION_180        0x02000000  /* Frame is rotated 180 degrees */
#define V4L2_BUF_FRAME_FLAG_ROTATION_270        0x03000000  /* Frame is rotated 270 degrees */
#define V4L2_BUF_FRAME_FLAG_ROTATION_MASK       0x03000000
#define V4L2_BUF_FRAME_FLAG_SCALING_2           0x04000000  /* Frame is scaled by half */
#define V4L2_BUF_FRAME_FLAG_SCALING_4           0x08000000  /* Frame is scaled by quarter */
#define V4L2_BUF_FRAME_FLAG_SCALING_MASK        0x0C000000
#define V4L2_BUF_FLAG_MVX_BUFFER_FRAME_PRESENT  0x10000000
#define V4L2_BUF_FLAG_MVX_BUFFER_NEED_REALLOC   0x20000000

//for decode bitstream flag
#define V4L2_BUF_FLAG_MVX_CODEC_CONFIG          0xC1000000
#define V4L2_BUF_FLAG_END_OF_SUB_FRAME          0xC2000000
#define V4L2_BUF_FLAG_MVX_DECODE_ONLY           0xC4000000

//for encode frame flag
#define V4L2_BUF_FRAME_FLAG_MIRROR_HORI         0x81000000
#define V4L2_BUF_FRAME_FLAG_MIRROR_VERT         0x82000000
#define V4L2_BUF_FRAME_FLAG_MIRROR_MASK         0x83000000
#define V4L2_BUF_FLAG_MVX_BUFFER_ROI            0x84000000  /* this buffer has a roi region */
#define V4L2_BUF_FLAG_MVX_BUFFER_EPR            0x88000000  /* EPR buffer flag */

//afbc flag
#define V4L2_BUF_FLAG_MVX_AFBC_TILED_HEADERS    0x01000000
#define V4L2_BUF_FLAG_MVX_AFBC_TILED_BODY       0x02000000
#define V4L2_BUF_FLAG_MVX_AFBC_32X8_SUPERBLOCK  0x04000000
#define V4L2_BUF_FLAG_MVX_AFBC_BLOCK_SPLIT      0x08000000

#define V4L2_BUF_FLAG_MVX_DISABLE_CACHE_MAINTENANCE 0x50000000 /*disable cache maintenance for buffer.*/

/****************************************************************************
 * HDR color description.
 ****************************************************************************/

#define V4L2_EVENT_MVX_COLOR_DESC       V4L2_EVENT_PRIVATE_START
#define V4L2_MVX_MAX_FRAME_REGIONS 16

enum v4l2_mvx_range {
	V4L2_MVX_RANGE_UNSPECIFIED,
	V4L2_MVX_RANGE_FULL,
	V4L2_MVX_RANGE_LIMITED
};

enum v4l2_mvx_primaries {
	V4L2_MVX_PRIMARIES_UNSPECIFIED,
	V4L2_MVX_PRIMARIES_BT709,         /* Rec.ITU-R BT.709 */
	V4L2_MVX_PRIMARIES_BT470M,        /* Rec.ITU-R BT.470 System M */
	V4L2_MVX_PRIMARIES_BT601_625,     /* Rec.ITU-R BT.601 625 */
	V4L2_MVX_PRIMARIES_BT601_525,     /* Rec.ITU-R BT.601 525 */
	V4L2_MVX_PRIMARIES_GENERIC_FILM,  /* Generic Film */
	V4L2_MVX_PRIMARIES_BT2020         /* Rec.ITU-R BT.2020 */
};

enum v4l2_mvx_transfer {
	V4L2_MVX_TRANSFER_UNSPECIFIED,
	V4L2_MVX_TRANSFER_LINEAR,         /* Linear transfer characteristics */
	V4L2_MVX_TRANSFER_SRGB,           /* sRGB */
	V4L2_MVX_TRANSFER_SMPTE170M,      /* SMPTE 170M */
	V4L2_MVX_TRANSFER_GAMMA22,        /* Assumed display gamma 2.2 */
	V4L2_MVX_TRANSFER_GAMMA28,        /* Assumed display gamma 2.8 */
	V4L2_MVX_TRANSFER_ST2084,         /* SMPTE ST 2084 */
	V4L2_MVX_TRANSFER_HLG,            /* ARIB STD-B67 hybrid-log-gamma */
	V4L2_MVX_TRANSFER_SMPTE240M,      /* SMPTE 240M */
	V4L2_MVX_TRANSFER_XVYCC,          /* IEC 61966-2-4 */
	V4L2_MVX_TRANSFER_BT1361,         /* Rec.ITU-R BT.1361 extended gamut */
	V4L2_MVX_TRANSFER_ST428           /* SMPTE ST 428-1 */
};

enum v4l2_mvx_matrix {
	V4L2_MVX_MATRIX_UNSPECIFIED,
	V4L2_MVX_MATRIX_BT709,            /* Rec.ITU-R BT.709 */
	V4L2_MVX_MATRIX_BT470M,           /* KR=0.30, KB=0.11 */
	V4L2_MVX_MATRIX_BT601,            /* Rec.ITU-R BT.601 625 */
	V4L2_MVX_MATRIX_SMPTE240M,        /* SMPTE 240M or equivalent */
	V4L2_MVX_MATRIX_BT2020,           /* Rec.ITU-R BT.2020 non-const lum */
	V4L2_MVX_MATRIX_BT2020Constant    /* Rec.ITU-R BT.2020 constant lum */
};

enum v4l2_nalu_format {
    V4L2_OPT_NALU_FORMAT_START_CODES,
    V4L2_OPT_NALU_FORMAT_ONE_NALU_PER_BUFFER,
    V4L2_OPT_NALU_FORMAT_ONE_BYTE_LENGTH_FIELD,
    V4L2_OPT_NALU_FORMAT_TWO_BYTE_LENGTH_FIELD,
    V4L2_OPT_NALU_FORMAT_FOUR_BYTE_LENGTH_FIELD,
    V4L2_OPT_NALU_FORMAT_ONE_FRAME_PER_BUFFER
};

struct v4l2_mvx_primary {
	uint16_t x;
	uint16_t y;
};

/**
 * struct v4l2_mvx_color_desc - HDR color description.
 * @flags:			Flags which fields that are valid.
 * @range:			enum v4l2_mvx_range.
 * @primaries:			enum v4l2_mvx_primaries.
 * @transfer:			enum v4l2_mvx_transfer.
 * @matrix:			enum v4l2_mvx_matrix.
 * @display.r:			Red point.
 * @display.g:			Green point.
 * @display.b:			Blue point.
 * @display.w:			White point.
 * @display.luminance_min:	Minimum display luminance.
 * @display.luminance_max:	Maximum display luminance.
 * @content.luminance_max:	Maximum content luminance.
 * @content.luminance_average:	Average content luminance.
 *
 * Color- and white point primaries are given in increments of 0.00002
 * and in the range of 0 to 50'000.
 *
 * Luminance is given in increments of 0.0001 candelas per m3.
 */
struct v4l2_mvx_color_desc {
	uint32_t flags;
        #define V4L2_BUFFER_PARAM_COLOUR_FLAG_MASTERING_DISPLAY_DATA_VALID  (1)
        #define V4L2_BUFFER_PARAM_COLOUR_FLAG_CONTENT_LIGHT_DATA_VALID      (2)
	uint8_t range;
	uint8_t primaries;
	uint8_t transfer;
	uint8_t matrix;
	struct {
		struct v4l2_mvx_primary r;
		struct v4l2_mvx_primary g;
		struct v4l2_mvx_primary b;
		struct v4l2_mvx_primary w;
		uint16_t luminance_min;
		uint16_t luminance_max;
	} display;
	struct {
		uint16_t luminance_max;
		uint16_t luminance_average;
	} content;

    uint8_t video_format;
    uint8_t aspect_ratio_idc;
    uint16_t sar_width;
    uint16_t sar_height;
    uint32_t num_units_in_tick;
    uint32_t time_scale;
} __attribute__ ((packed));

struct v4l2_buffer_param_region
{
    uint16_t mbx_left;   /**< X coordinate of the left most macroblock */
    uint16_t mbx_right;  /**< X coordinate of the right most macroblock */
    uint16_t mby_top;    /**< Y coordinate of the top most macroblock */
    uint16_t mby_bottom; /**< Y coordinate of the bottom most macroblock */
    int16_t qp_delta;   /**< QP delta value. This region will be encoded
                         *   with qp = qp_default + qp_delta. */
};

struct v4l2_mvx_roi_regions
{
    unsigned int pic_index;
    unsigned char qp_present;
    unsigned char qp;
    unsigned char roi_present;
    unsigned char num_roi;
    struct v4l2_buffer_param_region roi[V4L2_MVX_MAX_FRAME_REGIONS];
};

struct v4l2_sei_user_data
{
    uint8_t flags;
        #define V4L2_BUFFER_PARAM_USER_DATA_UNREGISTERED_VALID  (1)
    uint8_t uuid[16];
    char user_data[256 - 35];
    uint8_t user_data_len;
};

struct v4l2_rate_control
{
    uint32_t rc_type;
        #define V4L2_OPT_RATE_CONTROL_MODE_OFF          (0)
        #define V4L2_OPT_RATE_CONTROL_MODE_STANDARD     (1)
        #define V4L2_OPT_RATE_CONTROL_MODE_VARIABLE     (2)
        #define V4L2_OPT_RATE_CONTROL_MODE_CONSTANT     (3)
        #define V4L2_OPT_RATE_CONTROL_MODE_C_VARIABLE   (4)
    uint32_t target_bitrate;
    uint32_t maximum_bitrate;
};

struct v4l2_mvx_dsl_frame
{
    uint32_t width;
    uint32_t height;
};

struct v4l2_mvx_dsl_ratio
{
    uint32_t hor;
    uint32_t ver;
};

struct v4l2_mvx_long_term_ref
{
    uint32_t mode;
    uint32_t period;
};

#define V4L2_MVX_COLOR_DESC_DISPLAY_VALID       0x1
#define V4L2_MVX_COLOR_DESC_CONTENT_VALID       0x2

/****************************************************************************
 * Custom IOCTL
 ****************************************************************************/

#define VIDIOC_G_MVX_COLORDESC  _IOWR('V', BASE_VIDIOC_PRIVATE,	\
				      struct v4l2_mvx_color_desc)
#define VIDIOC_S_MVX_ROI_REGIONS _IOWR('V', BASE_VIDIOC_PRIVATE + 1,	\
				      struct v4l2_mvx_roi_regions)
#define VIDIOC_S_MVX_QP_EPR _IOWR('V', BASE_VIDIOC_PRIVATE + 2,	\
				      int)
#define VIDIOC_S_MVX_COLORDESC  _IOWR('V', BASE_VIDIOC_PRIVATE + 3,	\
				      struct v4l2_mvx_color_desc)
#define VIDIOC_S_MVX_SEI_USERDATA _IOWR('V', BASE_VIDIOC_PRIVATE + 4,	\
				      struct v4l2_sei_user_data)
#define VIDIOC_S_MVX_RATE_CONTROL _IOWR('V', BASE_VIDIOC_PRIVATE + 5,	\
				      struct v4l2_rate_control)
#define VIDIOC_S_MVX_DSL_FRAME _IOWR('V', BASE_VIDIOC_PRIVATE + 6,	\
				      struct v4l2_mvx_dsl_frame)
#define VIDIOC_S_MVX_DSL_RATIO _IOWR('V', BASE_VIDIOC_PRIVATE + 7,	\
				      struct v4l2_mvx_dsl_ratio)
#define VIDIOC_S_MVX_LONG_TERM_REF _IOWR('V', BASE_VIDIOC_PRIVATE + 8,	\
				      struct v4l2_mvx_long_term_ref)
#define VIDIOC_S_MVX_DSL_MODE _IOWR('V', BASE_VIDIOC_PRIVATE + 9,	\
				      int)
/****************************************************************************
 * Custom controls
 ****************************************************************************/

/*
 * Video for Linux 2 custom controls.
 */
enum v4l2_cid_mve_video {
	V4L2_CID_MVE_VIDEO_FRAME_RATE = V4L2_CTRL_CLASS_CODEC + 0x2000,
	V4L2_CID_MVE_VIDEO_NALU_FORMAT,
	V4L2_CID_MVE_VIDEO_STREAM_ESCAPING,
	V4L2_CID_MVE_VIDEO_H265_PROFILE,
	V4L2_CID_MVE_VIDEO_VC1_PROFILE,
	V4L2_CID_MVE_VIDEO_H265_LEVEL,
	V4L2_CID_MVE_VIDEO_IGNORE_STREAM_HEADERS,
	V4L2_CID_MVE_VIDEO_FRAME_REORDERING,
	V4L2_CID_MVE_VIDEO_INTBUF_SIZE,
	V4L2_CID_MVE_VIDEO_P_FRAMES,
	V4L2_CID_MVE_VIDEO_GOP_TYPE,
	V4L2_CID_MVE_VIDEO_CONSTR_IPRED,
	V4L2_CID_MVE_VIDEO_ENTROPY_SYNC,
	V4L2_CID_MVE_VIDEO_TEMPORAL_MVP,
	V4L2_CID_MVE_VIDEO_TILE_ROWS,
	V4L2_CID_MVE_VIDEO_TILE_COLS,
	V4L2_CID_MVE_VIDEO_MIN_LUMA_CB_SIZE,
	V4L2_CID_MVE_VIDEO_MB_MASK,
	V4L2_CID_MVE_VIDEO_VP9_PROB_UPDATE,
	V4L2_CID_MVE_VIDEO_BITDEPTH_CHROMA,
	V4L2_CID_MVE_VIDEO_BITDEPTH_LUMA,
	V4L2_CID_MVE_VIDEO_FORCE_CHROMA_FORMAT,
	V4L2_CID_MVE_VIDEO_RGB_TO_YUV_MODE,
	V4L2_CID_MVE_VIDEO_BANDWIDTH_LIMIT,
	V4L2_CID_MVE_VIDEO_CABAC_INIT_IDC,
	V4L2_CID_MVE_VIDEO_VPX_B_FRAME_QP,
	V4L2_CID_MVE_VIDEO_SECURE_VIDEO,
	V4L2_CID_MVE_VIDEO_CROP_LEFT,
	V4L2_CID_MVE_VIDEO_CROP_RIGHT,
	V4L2_CID_MVE_VIDEO_CROP_TOP,
	V4L2_CID_MVE_VIDEO_CROP_BOTTOM,
	V4L2_CID_MVE_VIDEO_HRD_BUFFER_SIZE,
	V4L2_CID_MVE_VIDEO_WATCHDOG_TIMEOUT,
	V4L2_CID_MVE_VIDEO_PROFILING
};

/* block configuration uncompressed rows header. this configures the size of the
 * uncompressed body. */
struct v4l2_buffer_general_rows_uncomp_hdr
{
    uint8_t n_cols_minus1; /* number of quad cols in picture minus 1 */
    uint8_t n_rows_minus1; /* number of quad rows in picture minus 1 */
    uint8_t reserved[2];
};

struct v4l2_buffer_general_block_configs
{
    uint8_t blk_cfg_type;
        #define V4L2_BLOCK_CONFIGS_TYPE_NONE       (0x00)
        #define V4L2_BLOCK_CONFIGS_TYPE_ROW_UNCOMP (0xff)
    uint8_t reserved[3];
    union
    {
        struct v4l2_buffer_general_rows_uncomp_hdr rows_uncomp;
    } blk_cfgs;
};

/* input for encoder */
struct v4l2_buffer_param_qp
{
    /* QP (quantization parameter) for encode.
     *
     * When used to set fixed QP for encode, with rate control
     * disabled, then the valid ranges are:
     *   H264: 0-51
     *   HEVC: 0-51
     *   VP8:  0-63
     *   VP9:  0-63
     * Note: The QP must be set separately for I, P and B frames.
     *
     * But when this message is used with the regions-feature,
     * then the valid ranges are the internal bitstream ranges:
     *   H264: 0-51
     *   HEVC: 0-51
     *   VP8:  0-127
     *   VP9:  0-255
     */
    int32_t qp;
};

/* the block parameter record specifies the various properties of a quad */
struct v4l2_block_param_record
{
    uint16_t qp_delta;
        /* Bitset of four 4-bit QP delta values for a quad.
         * For H.264 and HEVC these are qp delta values in the range -8 to +7.
         * For Vp9 these are segment map values in the range 0 to 7.
         */
        #define V4L2_BLOCK_PARAM_RECORD_QP_DELTA_TOP_LEFT_16X16     (0)
        #define V4L2_BLOCK_PARAM_RECORD_QP_DELTA_TOP_LEFT_16X16_SZ  (4)
        #define V4L2_BLOCK_PARAM_RECORD_QP_DELTA_TOP_RIGHT_16X16    (4)
        #define V4L2_BLOCK_PARAM_RECORD_QP_DELTA_TOP_RIGHT_16X16_SZ (4)
        #define V4L2_BLOCK_PARAM_RECORD_QP_DELTA_BOT_LEFT_16X16     (8)
        #define V4L2_BLOCK_PARAM_RECORD_QP_DELTA_BOT_LEFT_16X16_SZ  (4)
        #define V4L2_BLOCK_PARAM_RECORD_QP_DELTA_BOT_RIGHT_16X16    (12)
        #define V4L2_BLOCK_PARAM_RECORD_QP_DELTA_BOT_RIGHT_16X16_SZ (4)

        #define V4L2_BLOCK_PARAM_RECORD_VP9_SEGID_TOP_LEFT_16X16     (0)
        #define V4L2_BLOCK_PARAM_RECORD_VP9_SEGID_TOP_LEFT_16X16_SZ  (3)
        #define V4L2_BLOCK_PARAM_RECORD_VP9_SEGID_TOP_RIGHT_16X16    (4)
        #define V4L2_BLOCK_PARAM_RECORD_VP9_SEGID_TOP_RIGHT_16X16_SZ (3)
        #define V4L2_BLOCK_PARAM_RECORD_VP9_SEGID_BOT_LEFT_16X16     (8)
        #define V4L2_BLOCK_PARAM_RECORD_VP9_SEGID_BOT_LEFT_16X16_SZ  (3)
        #define V4L2_BLOCK_PARAM_RECORD_VP9_SEGID_BOT_RIGHT_16X16    (12)
        #define V4L2_BLOCK_PARAM_RECORD_VP9_SEGID_BOT_RIGHT_16X16_SZ (3)

    uint8_t force;
        #define V4L2_BLOCK_PARAM_RECORD_FORCE_NONE  (0x00)
        #define V4L2_BLOCK_PARAM_RECORD_FORCE_QP    (0x01)
        #define V4L2_BLOCK_PARAM_RECORD_FORCE_32X32 (0x02)
        #define V4L2_BLOCK_PARAM_RECORD_FORCE_RB    (0x04)

    uint8_t reserved;
};

struct v4l2_buffer_general_rows_uncomp_body
{
    /* the size of this array is variable and not necessarily equal to 1.
     * therefore the sizeof operator should not be used
     */
    struct v4l2_block_param_record bpr[1];
};

struct v4l2_core_buffer_header_general
{
    //uint64_t user_data_tag;   // User supplied tracking identifier
    //uint64_t app_handle;    // Host buffer handle number
    uint16_t type;  // type of config, value is one of V4L2_BUFFER_GENERAL_TYPE_X
        #define V4L2_BUFFER_GENERAL_TYPE_BLOCK_CONFIGS (1) /* block_configs */
        #define V4L2_BUFFER_GENERAL_TYPE_ENCODER_STATS (4) /* encoder_stats */
    uint16_t config_size;  // size of the configuration
    uint32_t buffer_size;
    struct v4l2_buffer_general_block_configs config;
};

#endif /* _MVX_V4L2_CONTROLS_H_ */
