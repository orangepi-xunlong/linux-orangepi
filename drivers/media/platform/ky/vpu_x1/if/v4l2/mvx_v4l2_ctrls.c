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

#include "mvx_bitops.h"
#include "mvx_v4l2_ctrls.h"
#include "mvx_v4l2_session.h"
#include "mvx-v4l2-controls.h"

/****************************************************************************
 * Static functions and variables
 ****************************************************************************/

/*
 * V4L2_CID_MVE_VIDEO_NALU_FORMAT control defines.
 */
static const char *const nalu_format_str[] = {
	"Start codes",
	"One nalu per buffer",
	"One byte length field",
	"Two byte length field",
	"Four byte length field"
};

static const enum mvx_nalu_format mvx_nalu_format_list[] = {
	MVX_NALU_FORMAT_START_CODES,
	MVX_NALU_FORMAT_ONE_NALU_PER_BUFFER,
	MVX_NALU_FORMAT_ONE_BYTE_LENGTH_FIELD,
	MVX_NALU_FORMAT_TWO_BYTE_LENGTH_FIELD,
	MVX_NALU_FORMAT_FOUR_BYTE_LENGTH_FIELD
};

/*
 * V4L2_CID_MVE_VIDEO_H265_PROFILE control defines.
 */
static const char *const h265_profile_str[] = {
	"Main",
	"Main still",
	"Main intra"
};

static const int mvx_h265_profile_list[] = {
	MVX_PROFILE_H265_MAIN,
	MVX_PROFILE_H265_MAIN_STILL,
	MVX_PROFILE_H265_MAIN_INTRA
};

/*
 * V4L2_CID_MVE_VIDEO_VC1_PROFILE control defines.
 */
static const char *const vc1_profile_str[] = {
	"Simple",
	"Main",
	"Advanced"
};

static const int mvx_vc1_profile_list[] = {
	MVX_PROFILE_VC1_SIMPLE,
	MVX_PROFILE_VC1_MAIN,
	MVX_PROFILE_VC1_ADVANCED
};

/*
 * V4L2_CID_MPEG_VIDEO_H264_PROFILE control defines.
 */
static const uint8_t h264_profile_list[] = {
	V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE,
	V4L2_MPEG_VIDEO_H264_PROFILE_MAIN,
	V4L2_MPEG_VIDEO_H264_PROFILE_HIGH,
	V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_10
};
static const uint8_t hevc_profile_list[] = {
  V4L2_MPEG_VIDEO_HEVC_PROFILE_MAIN,
  //V4L2_MPEG_VIDEO_HEVC_PROFILE_MAIN_STILL_PICTURE,
  //V4L2_MPEG_VIDEO_HEVC_PROFILE_MAIN_10
};

static const enum mvx_profile mvx_h264_profile_list[] = {
	MVX_PROFILE_H264_BASELINE,
	MVX_PROFILE_H264_MAIN,
	MVX_PROFILE_H264_HIGH,
	MVX_PROFILE_H264_HIGH_10
};

/*
 * V4L2_CID_MPEG_VIDEO_H264_LEVEL control defines.
 */
static uint8_t h264_level_list[] = {
	V4L2_MPEG_VIDEO_H264_LEVEL_1_0,
	V4L2_MPEG_VIDEO_H264_LEVEL_1B,
	V4L2_MPEG_VIDEO_H264_LEVEL_1_1,
	V4L2_MPEG_VIDEO_H264_LEVEL_1_2,
	V4L2_MPEG_VIDEO_H264_LEVEL_1_3,
	V4L2_MPEG_VIDEO_H264_LEVEL_2_0,
	V4L2_MPEG_VIDEO_H264_LEVEL_2_1,
	V4L2_MPEG_VIDEO_H264_LEVEL_2_2,
	V4L2_MPEG_VIDEO_H264_LEVEL_3_0,
	V4L2_MPEG_VIDEO_H264_LEVEL_3_1,
	V4L2_MPEG_VIDEO_H264_LEVEL_3_2,
	V4L2_MPEG_VIDEO_H264_LEVEL_4_0,
	V4L2_MPEG_VIDEO_H264_LEVEL_4_1,
	V4L2_MPEG_VIDEO_H264_LEVEL_4_2,
	V4L2_MPEG_VIDEO_H264_LEVEL_5_0,
	V4L2_MPEG_VIDEO_H264_LEVEL_5_1
};
static uint8_t hevc_level_list[] = {
 V4L2_MPEG_VIDEO_HEVC_LEVEL_1,
  V4L2_MPEG_VIDEO_HEVC_LEVEL_2,
  V4L2_MPEG_VIDEO_HEVC_LEVEL_2_1,
  V4L2_MPEG_VIDEO_HEVC_LEVEL_3,
  V4L2_MPEG_VIDEO_HEVC_LEVEL_3_1,
  V4L2_MPEG_VIDEO_HEVC_LEVEL_4,
  V4L2_MPEG_VIDEO_HEVC_LEVEL_4_1,
  V4L2_MPEG_VIDEO_HEVC_LEVEL_5,
  V4L2_MPEG_VIDEO_HEVC_LEVEL_5_1,
  V4L2_MPEG_VIDEO_HEVC_LEVEL_5_2,
  V4L2_MPEG_VIDEO_HEVC_LEVEL_6,
  V4L2_MPEG_VIDEO_HEVC_LEVEL_6_1,
  V4L2_MPEG_VIDEO_HEVC_LEVEL_6_2,
};

static const int mvx_h264_level_list[] = {
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
	MVX_LEVEL_H264_51
};

/*
 * V4L2_CID_MVE_VIDEO_H265_LEVEL control defines.
 */
static const char *const h265_level_str[] = {
	"Main 1",
	"High 1",

	"Main 2",
	"High 2",
	"Main 2.1",
	"High 2.1",

	"Main 3",
	"High 3",
	"Main 3.1",
	"High 3.1",

	"Main 4",
	"High 4",
	"Main 4.1",
	"High 4.1",

	"Main 5",
	"High 5",
	"Main 5.1",
	"High 5.1",
	"Main 5.2",
	"High 5.2",

	"Main 6",
	"High 6",
	"Main 6.1",
	"High 6.1",
	"Main 6.2",
	"High 6.2"
};

static const int mvx_h265_level_list[] = {
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

/*
 * V4L2_CID_MVE_VIDEO_GOP_TYPE control defines.
 */
static const char *const gop_type_str[] = {
	"None",
	"Bidirectional",
	"Low delay",
	"Pyramid"
};

static const enum mvx_gop_type mvx_gop_type_list[] = {
	MVX_GOP_TYPE_NONE,
	MVX_GOP_TYPE_BIDIRECTIONAL,
	MVX_GOP_TYPE_LOW_DELAY,
	MVX_GOP_TYPE_PYRAMID
};

/*
 * V4L2_CID_MPEG_VIDEO_H264_ENTROPY_MODE control defines.
 */
#define V4L2_MPEG_VIDEO_H264_ENTROPY_MODE_NONE          2

static const uint8_t h264_entropy_mode_list[] = {
	V4L2_MPEG_VIDEO_H264_ENTROPY_MODE_CAVLC,
	V4L2_MPEG_VIDEO_H264_ENTROPY_MODE_CABAC,
	V4L2_MPEG_VIDEO_H264_ENTROPY_MODE_NONE
};

static const enum mvx_entropy_mode mvx_h264_entropy_mode_list[] = {
	MVX_ENTROPY_MODE_CAVLC,
	MVX_ENTROPY_MODE_CABAC,
	MVX_ENTROPY_MODE_NONE
};

static const char *const h264_entropy_mode_str[] = {
	"CAVLC",
	"CABAC",
	"None"
};

/*
 * V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MODE controls list.
 */
static uint8_t multi_slice_mode_list[] = {
	V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_SINGLE,

	/* Misspelling in the header file */
	V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_MB
};

static const enum mvx_multi_slice_mode mvx_multi_slice_mode_list[] = {
	MVX_MULTI_SLICE_MODE_SINGLE,
	MVX_MULTI_SLICE_MODE_MAX_MB
};

/*
 * V4L2_CID_MVE_VIDEO_VP9_PROB_UPDATE control defines.
 */
static const char *const vp9_prob_update_str[] = {
	"Disabled",
	"Implicit",
	"Explicit"
};

static const enum mvx_vp9_prob_update mvx_vp9_prob_update_list[] = {
	MVX_VP9_PROB_UPDATE_DISABLED,
	MVX_VP9_PROB_UPDATE_IMPLICIT,
	MVX_VP9_PROB_UPDATE_EXPLICIT
};

/*
 * V4L2_CID_MVE_VIDEO_RGB_TO_YUV_MODE control defines.
 */
static const char *const rgb_to_yuv_mode_str[] = {
	"BT601 studio",
	"BT601 full",
	"BT709 studio",
	"BT709 full"
};

static const enum mvx_rgb_to_yuv_mode mvx_rgb_to_yuv_mode_list[] = {
	MVX_RGB_TO_YUV_MODE_BT601_STUDIO,
	MVX_RGB_TO_YUV_MODE_BT601_FULL,
	MVX_RGB_TO_YUV_MODE_BT709_STUDIO,
	MVX_RGB_TO_YUV_MODE_BT709_FULL
};

/**
 * find_idx() - Find index of a value in an array.
 * @list:	Pointer to an array.
 * @size:	Size of an array.
 * @val:	Value to look for.
 *
 * Return: Index of the first occurrence of 'val' in 'list',
 *         or -EINVAL when not found.
 */
static int find_idx(const uint8_t *list,
		    size_t size,
		    uint8_t val)
{
	while (size--)
		if (list[size] == val)
			return size;

	return -EINVAL;
}

/**
 * set_ctrl() - Callback used by V4L2 framework to set a control.
 * @ctrl:	V4L2 control.
 *
 * Return: 0 on success, error code otherwise.
 */
static int set_ctrl(struct v4l2_ctrl *ctrl)
{
	int ret = 0;
	struct mvx_v4l2_session *vsession =
		container_of(ctrl->handler, struct mvx_v4l2_session,
			     v4l2_ctrl);
	struct mvx_session *session = &vsession->session;
	enum mvx_nalu_format nalu_fmt;
	enum mvx_profile mvx_profile;
	enum mvx_level mvx_level;
	enum mvx_gop_type gop_type;
	enum mvx_entropy_mode entropy_mode;
	enum mvx_multi_slice_mode multi_slice_mode;
	enum mvx_vp9_prob_update vp9_prob_update;
	enum mvx_rgb_to_yuv_mode rgb_to_yuv_mode;
	int32_t i32_val;
	int64_t i64_val;
	bool bool_val;
	enum mvx_tristate tri_val;
        struct mvx_buffer_param_rate_control rate_control_param;
	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	switch (ctrl->id) {
	case V4L2_CID_MVE_VIDEO_SECURE_VIDEO:
		bool_val = *ctrl->p_new.p_s32 != 0;
		ret = mvx_session_set_securevideo(session, bool_val);
		break;
	case V4L2_CID_MVE_VIDEO_FRAME_RATE:
		i64_val = (ctrl->type == V4L2_CTRL_TYPE_INTEGER)?*ctrl->p_new.p_s32:*ctrl->p_new.p_s64;
		ret = mvx_session_set_frame_rate(session, i64_val);
		break;
	case V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE:
		bool_val = *ctrl->p_new.p_s32 != 0;
		ret = mvx_session_set_rate_control(session, bool_val);
		break;
	case V4L2_CID_MPEG_VIDEO_BITRATE:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_bitrate(session, i32_val);
		break;
       case V4L2_CID_MPEG_VIDEO_BITRATE_MODE:
       {
              i32_val = *ctrl->p_new.p_s32;
              rate_control_param.rate_control_mode = session->rc_type;
              rate_control_param.target_bitrate = session->target_bitrate;
              rate_control_param.maximum_bitrate = session->maximum_bitrate;
              if (i32_val==V4L2_MPEG_VIDEO_BITRATE_MODE_VBR)
              {
                  rate_control_param.rate_control_mode = MVX_OPT_RATE_CONTROL_MODE_VARIABLE;
              }
              else if (i32_val==V4L2_MPEG_VIDEO_BITRATE_MODE_CBR)
              {
                  rate_control_param.rate_control_mode = MVX_OPT_RATE_CONTROL_MODE_CONSTANT;
              }
              else if (i32_val == V4L2_MPEG_VIDEO_BITRATE_MODE_CQ)
              {
                  rate_control_param.rate_control_mode = MVX_OPT_RATE_CONTROL_MODE_OFF;
              }
              ret = mvx_session_set_bitrate_control(session, &rate_control_param);
              break;
	}
	case V4L2_CID_MPEG_VIDEO_BITRATE_PEAK:
	{
             rate_control_param.rate_control_mode = session->rc_type;
             rate_control_param.target_bitrate = session->target_bitrate;
             rate_control_param.maximum_bitrate = *ctrl->p_new.p_s32;
             ret = mvx_session_set_bitrate_control(session, &rate_control_param);
             break;
       }
    case V4L2_CID_MVE_VIDEO_CROP_LEFT:
        i32_val = *ctrl->p_new.p_s32;
        ret = mvx_session_set_crop_left(session, i32_val);
        break;
    case V4L2_CID_MVE_VIDEO_CROP_RIGHT:
        i32_val = *ctrl->p_new.p_s32;
        ret = mvx_session_set_crop_right(session, i32_val);
        break;
    case V4L2_CID_MVE_VIDEO_CROP_TOP:
        i32_val = *ctrl->p_new.p_s32;
        ret = mvx_session_set_crop_top(session, i32_val);
        break;
    case V4L2_CID_MVE_VIDEO_CROP_BOTTOM:
        i32_val = *ctrl->p_new.p_s32;
        ret = mvx_session_set_crop_bottom(session, i32_val);
        break;
    case V4L2_CID_MVE_VIDEO_HRD_BUFFER_SIZE:
        i32_val = *ctrl->p_new.p_s32;
        ret = mvx_session_set_hrd_buffer_size(session, i32_val);
        break;
	case V4L2_CID_MVE_VIDEO_NALU_FORMAT:
		i32_val = *ctrl->p_new.p_s32;
		nalu_fmt = mvx_nalu_format_list[i32_val];
		ret = mvx_session_set_nalu_format(session, nalu_fmt);
		break;
	case V4L2_CID_MVE_VIDEO_STREAM_ESCAPING:
		tri_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_stream_escaping(session, tri_val);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_PROFILE:
		i32_val = *ctrl->p_new.p_s32;
		ret = find_idx(h264_profile_list,
			       ARRAY_SIZE(h264_profile_list), i32_val);
		if (ret == -EINVAL)
			goto unlock_mutex;

		mvx_profile = mvx_h264_profile_list[ret];
		ret = mvx_session_set_profile(session,
					      MVX_FORMAT_H264,
					      mvx_profile);
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_PROFILE:
        i32_val = *ctrl->p_new.p_s32;
		ret = find_idx(hevc_profile_list,
			       ARRAY_SIZE(hevc_profile_list), i32_val);
		if (ret == -EINVAL)
			goto unlock_mutex;

		mvx_profile = mvx_h265_profile_list[ret];
		ret = mvx_session_set_profile(session,
					      MVX_FORMAT_HEVC,
					      mvx_profile);
		break;
	case V4L2_CID_MVE_VIDEO_H265_PROFILE:
		i32_val = *ctrl->p_new.p_s32;
		mvx_profile = mvx_h265_profile_list[i32_val];
		ret = mvx_session_set_profile(session,
					      MVX_FORMAT_HEVC,
					      mvx_profile);
		break;
	case V4L2_CID_MVE_VIDEO_VC1_PROFILE:
		i32_val = *ctrl->p_new.p_s32;
		mvx_profile = mvx_vc1_profile_list[i32_val];
		ret = mvx_session_set_profile(session,
					      MVX_FORMAT_VC1,
					      mvx_profile);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LEVEL:
		i32_val = *ctrl->p_new.p_s32;
		ret = find_idx(h264_level_list,
			       ARRAY_SIZE(h264_level_list), i32_val);
		if (ret == -EINVAL)
			goto unlock_mutex;

		mvx_level = mvx_h264_level_list[ret];
		ret = mvx_session_set_level(session,
					    MVX_FORMAT_H264,
					    mvx_level);
		break;
    case V4L2_CID_MPEG_VIDEO_HEVC_LEVEL:
		i32_val = *ctrl->p_new.p_s32;
		ret = find_idx(hevc_level_list,
			       ARRAY_SIZE(hevc_level_list), i32_val);
		if (ret == -EINVAL)
			goto unlock_mutex;

		mvx_level = mvx_h265_level_list[ret*2];
		ret = mvx_session_set_level(session,
					    MVX_FORMAT_HEVC,
					    mvx_level);
		break;
	case V4L2_CID_MVE_VIDEO_H265_LEVEL:
		i32_val = *ctrl->p_new.p_s32;
		mvx_level = mvx_h265_level_list[i32_val];
		ret = mvx_session_set_level(session,
					    MVX_FORMAT_HEVC,
					    mvx_level);
		break;
	case V4L2_CID_MVE_VIDEO_IGNORE_STREAM_HEADERS:
		tri_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_ignore_stream_headers(session, tri_val);
		break;
	case V4L2_CID_MVE_VIDEO_FRAME_REORDERING:
		tri_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_frame_reordering(session, tri_val);
		break;
	case V4L2_CID_MVE_VIDEO_INTBUF_SIZE:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_intbuf_size(session, i32_val);
		break;
	case V4L2_CID_MVE_VIDEO_P_FRAMES:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_p_frames(session, i32_val);
		break;
	case V4L2_CID_MPEG_VIDEO_B_FRAMES:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_b_frames(session, i32_val);
		break;
	case V4L2_CID_MVE_VIDEO_GOP_TYPE:
		i32_val = *ctrl->p_new.p_s32;
		gop_type = mvx_gop_type_list[i32_val];
		ret = mvx_session_set_gop_type(session, gop_type);
		break;
	case V4L2_CID_MPEG_VIDEO_CYCLIC_INTRA_REFRESH_MB:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_cyclic_intra_refresh_mb(session,
							      i32_val);
		break;
	case V4L2_CID_MVE_VIDEO_CONSTR_IPRED:
		tri_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_constr_ipred(session, tri_val);
		break;
	case V4L2_CID_MVE_VIDEO_ENTROPY_SYNC:
		tri_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_entropy_sync(session, tri_val);
		break;
	case V4L2_CID_MVE_VIDEO_TEMPORAL_MVP:
		tri_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_temporal_mvp(session, tri_val);
		break;
	case V4L2_CID_MVE_VIDEO_TILE_ROWS:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_tile_rows(session, i32_val);
		break;
	case V4L2_CID_MVE_VIDEO_TILE_COLS:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_tile_cols(session, i32_val);
		break;
	case V4L2_CID_MVE_VIDEO_MIN_LUMA_CB_SIZE:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_min_luma_cb_size(session, i32_val);
		break;
	case V4L2_CID_MVE_VIDEO_MB_MASK:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_mb_mask(session, i32_val);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_ENTROPY_MODE:
		i32_val = *ctrl->p_new.p_s32;
		ret = find_idx(h264_entropy_mode_list,
			       ARRAY_SIZE(h264_entropy_mode_list), i32_val);
		if (ret == -EINVAL)
			goto unlock_mutex;

		entropy_mode = mvx_h264_entropy_mode_list[ret];
		ret = mvx_session_set_entropy_mode(session, entropy_mode);
		break;
	case V4L2_CID_MPEG_VIDEO_FORCE_KEY_FRAME:
	    mvx_session_set_force_idr(session);
	    break;
	case V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MODE:
		i32_val = *ctrl->p_new.p_s32;
		ret = find_idx(multi_slice_mode_list,
			       ARRAY_SIZE(multi_slice_mode_list), i32_val);
		if (ret == -EINVAL)
			goto unlock_mutex;

		multi_slice_mode = mvx_multi_slice_mode_list[ret];
		ret = mvx_session_set_multi_slice_mode(session,
						       multi_slice_mode);
		break;
	case V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_multi_slice_max_mb(session, i32_val);
		break;
	case V4L2_CID_MVE_VIDEO_VP9_PROB_UPDATE:
		i32_val = *ctrl->p_new.p_s32;
		vp9_prob_update = mvx_vp9_prob_update_list[i32_val];
		ret = mvx_session_set_vp9_prob_update(session,
						      vp9_prob_update);
		break;
	case V4L2_CID_MPEG_VIDEO_MV_H_SEARCH_RANGE:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_mv_h_search_range(session, i32_val);
		break;
	case V4L2_CID_MPEG_VIDEO_MV_V_SEARCH_RANGE:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_mv_v_search_range(session, i32_val);
		break;
	case V4L2_CID_MVE_VIDEO_BITDEPTH_CHROMA:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_bitdepth_chroma(session, i32_val);
		break;
	case V4L2_CID_MVE_VIDEO_BITDEPTH_LUMA:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_bitdepth_luma(session, i32_val);
		break;
	case V4L2_CID_MVE_VIDEO_FORCE_CHROMA_FORMAT:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_force_chroma_format(session, i32_val);
		break;
	case V4L2_CID_MVE_VIDEO_RGB_TO_YUV_MODE:
		i32_val = *ctrl->p_new.p_s32;
		rgb_to_yuv_mode = mvx_rgb_to_yuv_mode_list[i32_val];
		ret = mvx_session_set_rgb_to_yuv_mode(session,
						      rgb_to_yuv_mode);
		break;
	case V4L2_CID_MVE_VIDEO_BANDWIDTH_LIMIT:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_band_limit(session, i32_val);
		break;
	case V4L2_CID_MVE_VIDEO_CABAC_INIT_IDC:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_cabac_init_idc(session, i32_val);
		break;
	case V4L2_CID_MPEG_VIDEO_H263_I_FRAME_QP:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_i_frame_qp(session, MVX_FORMAT_H263,
						 i32_val);
		break;
	case V4L2_CID_MPEG_VIDEO_H263_P_FRAME_QP:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_p_frame_qp(session, MVX_FORMAT_H263,
						 i32_val);
		break;
	case V4L2_CID_MPEG_VIDEO_H263_B_FRAME_QP:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_b_frame_qp(session, MVX_FORMAT_H263,
						 i32_val);
		break;
	case V4L2_CID_MPEG_VIDEO_H263_MIN_QP:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_min_qp(session, MVX_FORMAT_H263,
					     i32_val);
		break;
	case V4L2_CID_MPEG_VIDEO_H263_MAX_QP:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_max_qp(session, MVX_FORMAT_H263,
					     i32_val);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_i_frame_qp(session, MVX_FORMAT_H264,
						 i32_val);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_p_frame_qp(session, MVX_FORMAT_H264,
						 i32_val);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_B_FRAME_QP:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_b_frame_qp(session, MVX_FORMAT_H264,
						 i32_val);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MIN_QP:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_min_qp(session, MVX_FORMAT_H264,
					     i32_val);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MAX_QP:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_max_qp(session, MVX_FORMAT_H264,
					     i32_val);
		break;
    case V4L2_CID_MPEG_VIDEO_HEVC_I_FRAME_QP:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_i_frame_qp(session, MVX_FORMAT_HEVC,
						 i32_val);
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_P_FRAME_QP:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_p_frame_qp(session, MVX_FORMAT_HEVC,
						 i32_val);
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_B_FRAME_QP:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_b_frame_qp(session, MVX_FORMAT_HEVC,
						 i32_val);
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_min_qp(session, MVX_FORMAT_HEVC,
					     i32_val);
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_max_qp(session, MVX_FORMAT_HEVC,
					     i32_val);
		break;
	case V4L2_CID_MPEG_VIDEO_VPX_I_FRAME_QP:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_i_frame_qp(session, MVX_FORMAT_VP9,
						 i32_val);
		break;
	case V4L2_CID_MPEG_VIDEO_VPX_P_FRAME_QP:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_p_frame_qp(session, MVX_FORMAT_VP9,
						 i32_val);
		break;
	case V4L2_CID_MVE_VIDEO_VPX_B_FRAME_QP:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_b_frame_qp(session, MVX_FORMAT_VP9,
						 i32_val);
		break;
	case V4L2_CID_MPEG_VIDEO_VPX_MIN_QP:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_min_qp(session, MVX_FORMAT_VP9,
					     i32_val);
		break;
	case V4L2_CID_MPEG_VIDEO_VPX_MAX_QP:
		i32_val = *ctrl->p_new.p_s32;
		ret = mvx_session_set_max_qp(session, MVX_FORMAT_VP9,
					     i32_val);
		break;
	case V4L2_CID_JPEG_RESTART_INTERVAL:
		i32_val = *ctrl->p_new.p_s32;
		if (i32_val != -1)
			ret = mvx_session_set_resync_interval(session, i32_val);

		break;
	case V4L2_CID_JPEG_COMPRESSION_QUALITY:
		i32_val = *ctrl->p_new.p_s32;
		if (i32_val != 0)
			ret = mvx_session_set_jpeg_quality(session, i32_val);

		break;
	case V4L2_CID_MVE_VIDEO_WATCHDOG_TIMEOUT:
		i32_val = *ctrl->p_new.p_s32;
		if (i32_val != 0)
			ret = mvx_session_set_watchdog_timeout(session, i32_val);

		break;
	case V4L2_CID_MVE_VIDEO_PROFILING:
	    i32_val = *ctrl->p_new.p_s32;
	    if (i32_val != 0)
		    ret = mvx_session_set_profiling(session, i32_val);

	    break;
	}

unlock_mutex:
	mutex_unlock(&vsession->mutex);

	return ret;
}

/**
 * get_volatile_ctrl() - Get control value.
 * @ctrl:	V4L2 control.
 *
 * Return: 0 on success, else error code.
 */
static int get_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mvx_v4l2_session *vsession =
		container_of(ctrl->handler, struct mvx_v4l2_session,
			     v4l2_ctrl);

	switch (ctrl->id) {
	case V4L2_CID_MIN_BUFFERS_FOR_OUTPUT:
		ctrl->val = vsession->session.port[MVX_DIR_INPUT].buffer_min;
		break;
	case V4L2_CID_MIN_BUFFERS_FOR_CAPTURE:
		ctrl->val = vsession->session.port[MVX_DIR_OUTPUT].buffer_min;
		break;
	default:
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Unsupported get control. id=%u.",
			      ctrl->id);
		return -EINVAL;
	}

	return 0;
}

/*
 * Callbacks required by V4L2 framework to implement controls support.
 */
static const struct v4l2_ctrl_ops ctrl_ops = {
	.g_volatile_ctrl = get_volatile_ctrl,
	.s_ctrl          = set_ctrl
};

/**
 * get_skip_mask() - Calculate V4L2 menu skip mask.
 * @list:	Array of menu items.
 * @cnt:	Number of menu items.
 *
 * Return: V4L2 menu skip mask.
 */
static uint64_t get_skip_mask(const uint8_t *list,
			      size_t cnt)
{
	uint64_t mask = 0;
	int i;

	for (i = 0; i < cnt; ++i)
		mvx_set_bit(list[i], &mask);

	return ~mask;
}

/**
 * mvx_v4l2_ctrl_new_custom_int() - Create custom V4L2 integer control.
 * @hnd:	V4L2 handler.
 * @id:		Id of a control.
 * @name:	Name of a control.
 * @min:	Minimum allowed value.
 * @max:	Maximum allowed value.
 * @def:	Default value.
 * @step:	Step.
 *
 * Return: Pointer to v4l2_ctrl structure in case of success,
 *         or NULL in case of failure.
 */
static struct v4l2_ctrl *mvx_v4l2_ctrl_new_custom_int(
	struct v4l2_ctrl_handler *hnd,
	int id,
	const char *name,
	int64_t min,
	int64_t max,
	int64_t def,
	int32_t step)
{
	struct v4l2_ctrl_config cfg;

	memset(&cfg, 0, sizeof(cfg));

	cfg.id = id;
	cfg.ops = &ctrl_ops;
	cfg.type = V4L2_CTRL_TYPE_INTEGER;
	cfg.name = name;
	cfg.min = min;
	cfg.max = max;
	cfg.def = def;
	cfg.step = step;

	return v4l2_ctrl_new_custom(hnd, &cfg, NULL);
}

/**
 * mvx_v4l2_ctrl_new_custom_tristate() - Create custom V4L2 tristate control.
 * @hnd:	V4L2 handler.
 * @id:		Id of a control.
 * @name:	Name of a control.
 * @def:	Default value.
 *
 * Return: Pointer to v4l2_ctrl structure in case of success,
 *         or NULL in case of failure.
 */
static struct v4l2_ctrl *mvx_v4l2_ctrl_new_custom_tristate(
	struct v4l2_ctrl_handler *hnd,
	int id,
	const char *name,
	enum mvx_tristate def)
{
	struct v4l2_ctrl_config cfg;

	memset(&cfg, 0, sizeof(cfg));

	cfg.id = id;
	cfg.ops = &ctrl_ops;
	cfg.type = V4L2_CTRL_TYPE_INTEGER;
	cfg.name = name;
	cfg.min = -1;
	cfg.max = 1;
	cfg.def = def;
	cfg.step = 1;

	return v4l2_ctrl_new_custom(hnd, &cfg, NULL);
}

/****************************************************************************
 * Exported functions
 ****************************************************************************/

int mvx_v4l2_ctrls_init(struct v4l2_ctrl_handler *hnd)
{
	int ret;
	struct v4l2_ctrl_config cfg;
	struct v4l2_ctrl *ctrl;

	ret = v4l2_ctrl_handler_init(hnd, 128);
	if (ret != 0)
		return ret;

	ctrl = mvx_v4l2_ctrl_new_custom_int(
		hnd, V4L2_CID_MVE_VIDEO_SECURE_VIDEO,
		"secure video", 0, 1, 0, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_int(
		hnd, V4L2_CID_MVE_VIDEO_FRAME_RATE,
		"frame rate", 0, 0x10000000, 30 << 16, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_int(
		hnd, V4L2_CID_MVE_VIDEO_CROP_LEFT,
		"bitrate control mode", 0, 10000000, 0, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_int(
		hnd, V4L2_CID_MVE_VIDEO_CROP_RIGHT,
		"bitrate control mode", 0, 10000000, 0, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_int(
		hnd, V4L2_CID_MVE_VIDEO_CROP_TOP,
		"bitrate control mode", 0, 10000000, 0, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_int(
		hnd, V4L2_CID_MVE_VIDEO_CROP_BOTTOM,
		"bitrate control mode", 0, 10000000, 0, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_int(
		hnd, V4L2_CID_MVE_VIDEO_HRD_BUFFER_SIZE,
		"HRD buffer size", 0, 1073741823, 0, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE,
		0, 1, 1, 0);
	if (ctrl == NULL)
		goto handler_free;

        ctrl = v4l2_ctrl_new_std_menu(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_BITRATE_MODE,
		V4L2_MPEG_VIDEO_BITRATE_MODE_CQ, ~7,  V4L2_MPEG_VIDEO_BITRATE_MODE_VBR);
	if (ctrl == NULL)
		goto handler_free;

        ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_BITRATE_PEAK,
		1000, 1000000000, 1, 64000);
	if (ctrl == NULL)
		goto handler_free;
	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_BITRATE,
		1000, 1000000000, 1, 64000);
	if (ctrl == NULL)
		goto handler_free;

	memset(&cfg, 0, sizeof(cfg));
	cfg.id = V4L2_CID_MVE_VIDEO_NALU_FORMAT;
	cfg.ops = &ctrl_ops;
	cfg.type = V4L2_CTRL_TYPE_MENU;
	cfg.name = "nalu format";
	cfg.max = ARRAY_SIZE(nalu_format_str) - 1;
	cfg.def = 0;
	cfg.qmenu = nalu_format_str;
	ctrl = v4l2_ctrl_new_custom(hnd, &cfg, NULL);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_tristate(
		hnd, V4L2_CID_MVE_VIDEO_STREAM_ESCAPING,
		"stream escaping", MVX_TRI_UNSET);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std_menu(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_H264_PROFILE,
		V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_10,
		get_skip_mask(h264_profile_list,
			      ARRAY_SIZE(h264_profile_list)),
		V4L2_MPEG_VIDEO_H264_PROFILE_MAIN);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std_menu(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_HEVC_PROFILE,
		V4L2_MPEG_VIDEO_HEVC_PROFILE_MAIN,
		get_skip_mask(hevc_profile_list,
			      ARRAY_SIZE(hevc_profile_list)),
		V4L2_MPEG_VIDEO_HEVC_PROFILE_MAIN);
	if (ctrl == NULL)
		goto handler_free;

	memset(&cfg, 0, sizeof(cfg));
	cfg.id = V4L2_CID_MVE_VIDEO_H265_PROFILE;
	cfg.ops = &ctrl_ops;
	cfg.type = V4L2_CTRL_TYPE_MENU;
	cfg.name = "h265 profile";
	cfg.max = ARRAY_SIZE(h265_profile_str) - 1;
	cfg.def = 0;
	cfg.qmenu = h265_profile_str;
	ctrl = v4l2_ctrl_new_custom(hnd, &cfg, NULL);
	if (ctrl == NULL)
		goto handler_free;

	memset(&cfg, 0, sizeof(cfg));
	cfg.id = V4L2_CID_MVE_VIDEO_VC1_PROFILE;
	cfg.ops = &ctrl_ops;
	cfg.type = V4L2_CTRL_TYPE_MENU;
	cfg.name = "vc1 profile";
	cfg.max = ARRAY_SIZE(vc1_profile_str) - 1;
	cfg.def = 0;
	cfg.qmenu = vc1_profile_str;
	ctrl = v4l2_ctrl_new_custom(hnd, &cfg, NULL);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std_menu(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_H264_LEVEL,
		V4L2_MPEG_VIDEO_H264_LEVEL_5_1,
		get_skip_mask(h264_level_list, ARRAY_SIZE(h264_level_list)),
		V4L2_MPEG_VIDEO_H264_LEVEL_1_0);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std_menu(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_HEVC_LEVEL,
		V4L2_MPEG_VIDEO_HEVC_LEVEL_6_2,
		get_skip_mask(hevc_level_list, ARRAY_SIZE(hevc_level_list)),
		V4L2_MPEG_VIDEO_HEVC_LEVEL_1);
	if (ctrl == NULL)
		goto handler_free;

	memset(&cfg, 0, sizeof(cfg));
	cfg.id = V4L2_CID_MVE_VIDEO_H265_LEVEL;
	cfg.ops = &ctrl_ops;
	cfg.type = V4L2_CTRL_TYPE_MENU;
	cfg.name = "h265 level";
	cfg.max = ARRAY_SIZE(h265_level_str) - 1;
	cfg.def = cfg.max;
	cfg.qmenu = h265_level_str;
	ctrl = v4l2_ctrl_new_custom(hnd, &cfg, NULL);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_tristate(
		hnd, V4L2_CID_MVE_VIDEO_IGNORE_STREAM_HEADERS,
		"ignore stream headers", MVX_TRI_UNSET);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_tristate(
		hnd, V4L2_CID_MVE_VIDEO_FRAME_REORDERING,
		"frame reordering", MVX_TRI_UNSET);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_int(
		hnd, V4L2_CID_MVE_VIDEO_INTBUF_SIZE,
		"internal buffer size", 0, INT_MAX, 0, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_int(
		hnd, V4L2_CID_MVE_VIDEO_P_FRAMES,
		"video P frames", 0, INT_MAX, 30, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_B_FRAMES,
		0, INT_MAX, 1, 0);
	if (ctrl == NULL)
		goto handler_free;

	memset(&cfg, 0, sizeof(cfg));
	cfg.id = V4L2_CID_MVE_VIDEO_GOP_TYPE;
	cfg.ops = &ctrl_ops;
	cfg.type = V4L2_CTRL_TYPE_MENU;
	cfg.name = "GOP type";
	cfg.max = ARRAY_SIZE(gop_type_str) - 1;
	cfg.def = 0;
	cfg.qmenu = gop_type_str;
	ctrl = v4l2_ctrl_new_custom(hnd, &cfg, NULL);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_CYCLIC_INTRA_REFRESH_MB,
		0, INT_MAX, 1, 0);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_tristate(
		hnd, V4L2_CID_MVE_VIDEO_CONSTR_IPRED,
		"constrained intra prediction", MVX_TRI_UNSET);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_tristate(
		hnd, V4L2_CID_MVE_VIDEO_ENTROPY_SYNC, "entropy sync",
		MVX_TRI_UNSET);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_tristate(
		hnd, V4L2_CID_MVE_VIDEO_TEMPORAL_MVP,
		"temporal mvp", MVX_TRI_UNSET);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_int(
		hnd, V4L2_CID_MVE_VIDEO_TILE_ROWS,
		"tile rows", 0, 65536, 0, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_int(
		hnd, V4L2_CID_MVE_VIDEO_TILE_COLS,
		"tile columns", 0, 65536, 0, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_int(
		hnd, V4L2_CID_MVE_VIDEO_MIN_LUMA_CB_SIZE,
		"min luma cb size", 8, 16, 8, 8);
	if (ctrl == NULL)
		goto handler_free;

	memset(&cfg, 0, sizeof(cfg));
	cfg.id = V4L2_CID_MVE_VIDEO_MB_MASK;
	cfg.ops = &ctrl_ops;
	cfg.type = V4L2_CTRL_TYPE_BITMASK;
	cfg.name = "macroblocks mask";
	cfg.def = 0x7fff;
	cfg.min = 0;
	cfg.max = 0x7fff;
	cfg.step = 0;
	ctrl = v4l2_ctrl_new_custom(hnd, &cfg, NULL);
	if (ctrl == NULL)
		goto handler_free;

	memset(&cfg, 0, sizeof(cfg));
	cfg.id = V4L2_CID_MPEG_VIDEO_H264_ENTROPY_MODE;
	cfg.ops = &ctrl_ops;
	cfg.type = V4L2_CTRL_TYPE_MENU;
	cfg.name = "H264 Entropy Mode";
	cfg.max = ARRAY_SIZE(h264_entropy_mode_str) - 1;
	cfg.def = V4L2_MPEG_VIDEO_H264_ENTROPY_MODE_NONE;
	cfg.qmenu = h264_entropy_mode_str;
	ctrl = v4l2_ctrl_new_custom(hnd, &cfg, NULL);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std_menu(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MODE,
		V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_MB,
		get_skip_mask(multi_slice_mode_list,
			      ARRAY_SIZE(multi_slice_mode_list)),
		V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_SINGLE);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB,
		0, INT_MAX, 1, 0);
	if (ctrl == NULL)
		goto handler_free;

	memset(&cfg, 0, sizeof(cfg));
	cfg.id = V4L2_CID_MVE_VIDEO_VP9_PROB_UPDATE;
	cfg.ops = &ctrl_ops;
	cfg.type = V4L2_CTRL_TYPE_MENU;
	cfg.name = "VP9 prob update";
	cfg.max = ARRAY_SIZE(vp9_prob_update_str) - 1;
	cfg.def = cfg.max;
	cfg.qmenu = vp9_prob_update_str;
	ctrl = v4l2_ctrl_new_custom(hnd, &cfg, NULL);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_MV_H_SEARCH_RANGE,
		0, INT_MAX, 1, 0);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_MV_V_SEARCH_RANGE,
		0, INT_MAX, 1, 0);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_int(
		hnd, V4L2_CID_MVE_VIDEO_BITDEPTH_CHROMA,
		"bitdepth chroma", 0, 0xff, 0, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_int(
		hnd, V4L2_CID_MVE_VIDEO_BITDEPTH_LUMA,
		"bitdepth luma", 0, 0xff, 0, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_int(
		hnd, V4L2_CID_MVE_VIDEO_FORCE_CHROMA_FORMAT,
		"force chroma format", -1, INT_MAX, 0, 1);
	if (ctrl == NULL)
		goto handler_free;

	memset(&cfg, 0, sizeof(cfg));
	cfg.id = V4L2_CID_MVE_VIDEO_RGB_TO_YUV_MODE;
	cfg.ops = &ctrl_ops;
	cfg.type = V4L2_CTRL_TYPE_MENU;
	cfg.name = "RGB to YUV conversion mode";
	cfg.max = ARRAY_SIZE(rgb_to_yuv_mode_str) - 1;
	cfg.def = cfg.max;
	cfg.qmenu = rgb_to_yuv_mode_str;
	ctrl = v4l2_ctrl_new_custom(hnd, &cfg, NULL);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_int(
		hnd, V4L2_CID_MVE_VIDEO_BANDWIDTH_LIMIT,
		"bandwidth limit", 0, INT_MAX, 0, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_int(
		hnd, V4L2_CID_MVE_VIDEO_CABAC_INIT_IDC,
		"CABAC init IDC", 0, 4, 0, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_int(
		hnd, V4L2_CID_MVE_VIDEO_WATCHDOG_TIMEOUT,
		"watchdog timeout", 5, 60, 30, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_int(
		hnd, V4L2_CID_MVE_VIDEO_PROFILING,
		"enable profiling", 0, 1, 0, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_H263_I_FRAME_QP,
		0, 31, 1, 0);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_H263_P_FRAME_QP,
		0, 31, 1, 0);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_H263_B_FRAME_QP,
		0, 31, 1, 0);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_H263_MIN_QP,
		1, 31, 1, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_H263_MAX_QP,
		1, 31, 1, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP,
		0, 51, 1, 0);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP,
		0, 51, 1, 0);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_H264_B_FRAME_QP,
		0, 51, 1, 0);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_H264_MIN_QP,
		1, 51, 1, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_H264_MAX_QP,
		1, 51, 1, 1);
	if (ctrl == NULL)
		goto handler_free;

    ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_HEVC_I_FRAME_QP,
		0, 51, 1, 0);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_HEVC_P_FRAME_QP,
		0, 51, 1, 0);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_HEVC_B_FRAME_QP,
		0, 51, 1, 0);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP,
		1, 51, 1, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP,
		1, 51, 1, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_VPX_I_FRAME_QP,
		0, 51, 1, 0);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_VPX_P_FRAME_QP,
		0, 51, 1, 0);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = mvx_v4l2_ctrl_new_custom_int(
		hnd, V4L2_CID_MVE_VIDEO_VPX_B_FRAME_QP,
		"VPx B frame QP value",
		0, 51, 0, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_VPX_MIN_QP,
		1, 51, 1, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_VPX_MAX_QP,
		1, 51, 1, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MIN_BUFFERS_FOR_OUTPUT,
		1, 32, 1, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY | V4L2_CTRL_FLAG_VOLATILE;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MIN_BUFFERS_FOR_CAPTURE,
		1, 32, 1, 1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY | V4L2_CTRL_FLAG_VOLATILE;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_JPEG_RESTART_INTERVAL,
		-1, 0xffff, 1, -1);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_JPEG_COMPRESSION_QUALITY,
		0, 100, 1, 0);
	if (ctrl == NULL)
		goto handler_free;

	ctrl = v4l2_ctrl_new_std(
		hnd, &ctrl_ops, V4L2_CID_MPEG_VIDEO_FORCE_KEY_FRAME,
		0, 1, 1, 0);
	if (ctrl == NULL)
		goto handler_free;

	ret = v4l2_ctrl_handler_setup(hnd);
	if (ret != 0)
		goto handler_free;

	return 0;

handler_free:
	v4l2_ctrl_handler_free(hnd);
	return -EINVAL;
}

void mvx_v4l2_ctrls_done(struct v4l2_ctrl_handler *hnd)
{
	v4l2_ctrl_handler_free(hnd);
}
