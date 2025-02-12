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

#ifndef _MVX_V4L2_SESSION_H_
#define _MVX_V4L2_SESSION_H_

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/videodev2.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fh.h>
#include <media/videobuf2-v4l2.h>

#include "mvx_session.h"
#include "mvx-v4l2-controls.h"

/****************************************************************************
 * Types
 ****************************************************************************/

/**
 * Offset used to distinguish between input and output port.
 */
#define DST_QUEUE_OFF_BASE (1 << 30)

/**
 * struct mvx_v4l2_port - V4L2 port type.
 *
 * Most of this structure will become redundant when buffer management
 * is transferred to Vb2 framework.
 *
 * @vsession:		Pointer to corresponding session.
 * @port:		Pointer to corresponding mvx port.
 * @dir:		Direction of a port.
 * @type:		V4L2 port type.
 * @pix_mp:		V4L2 multi planar pixel format.
 * @crop:		V4L2 cropping information.
 * @dentry:		Debugfs directory entry for the port.
 * @q_set:		Indicates of Vb2 queue was setup.
 * @vb2_queue:		Vb2 queue.
 */
struct mvx_v4l2_port {
	struct mvx_v4l2_session *vsession;
	struct mvx_session_port *port;
	enum mvx_direction dir;
	enum v4l2_buf_type type;
	struct v4l2_pix_format_mplane pix_mp;
	struct v4l2_rect crop;
	struct dentry *dentry;
	bool q_set;
	struct vb2_queue vb2_queue;
};

/**
 * struct mvx_v4l2_session - V4L2 session type.
 * @ext:		Pointer to external interface object.
 * @fh:			V4L2 file handler.
 * @mutex:		Mutex protecting the session object.
 * @session:		Session object.
 * @port:		Array of v4l2 ports.
 * @dentry:		Debugfs directory entry representing a session.
 * @v4l2_ctrl:		v4l2 controls handler.
 */
struct mvx_v4l2_session {
	struct mvx_ext_if *ext;
	struct v4l2_fh fh;
	struct mutex mutex;
	struct mvx_session session;
	struct mvx_v4l2_port port[MVX_DIR_MAX];
	struct dentry *dentry;
	struct v4l2_ctrl_handler v4l2_ctrl;
};

/****************************************************************************
 * Exported functions
 ****************************************************************************/

/**
 * mvx_v4l2_session_construct() - Construct v4l2 session object.
 * @vsession:	Pointer to a session object.
 * @ctx:	Pointer to an external interface object.
 *
 * Return: 0 on success, else error code.
 */
int mvx_v4l2_session_construct(struct mvx_v4l2_session *vsession,
			       struct mvx_ext_if *ctx);

/**
 * v4l2_fh_to_session() - Cast v4l2 file handler to mvx_v4l2_session.
 * @fh:		v4l2 file handler.
 *
 * Return: Pointer to a corresponding mvx_v4l2_session object.
 */
struct mvx_v4l2_session *v4l2_fh_to_session(struct v4l2_fh *fh);

/**
 * file_to_session() - Cast file object to mvx_v4l2_session.
 * @file:	Pointer to a file object.
 *
 * Return: Pointer to a corresponding mvx_v4l2_session object.
 */
struct mvx_v4l2_session *file_to_session(struct file *file);

/**
 * mvx_v4l2_session_get_color_desc() - Get color description.
 * @vsession:	Pointer to v4l2 session.
 * @color:	Color description.
 *
 * Return: 0 on success, else error code.
 */
int mvx_v4l2_session_get_color_desc(struct mvx_v4l2_session *vsession,
				    struct v4l2_mvx_color_desc *color_desc);

/**
 * mvx_v4l2_session_set_color_desc() - Set color description.
 * @vsession:	Pointer to v4l2 session.
 * @color:	Color description.
 *
 * Return: 0 on success, else error code.
 */

int mvx_v4l2_session_set_color_desc(struct mvx_v4l2_session *vsession,
				    struct v4l2_mvx_color_desc *color_desc);

/**
 * mvx_v4l2_session_set_sei_userdata() - Set SEI userdata.
 * @vsession:	Pointer to v4l2 session.
 * @sei_userdata:	SEI userdata.
 *
 * Return: 0 on success, else error code.
 */

int mvx_v4l2_session_set_sei_userdata(struct mvx_v4l2_session *vsession,
				    struct v4l2_sei_user_data *sei_userdata);

/**
 * mvx_v4l2_session_set_roi_regions() - Set Roi Regions.
 * @vsession:	Pointer to v4l2 session.
 * @roi:	ROI regions.
 *
 * Return: 0 on success, else error code.
 */
int mvx_v4l2_session_set_roi_regions(struct mvx_v4l2_session *vsession,
				    struct v4l2_mvx_roi_regions *roi);

/**
 * mvx_v4l2_session_set_qp_epr() - Set qp.
 * @vsession:	Pointer to v4l2 session.
 * @qp:	qp value.
 *
 * Return: 0 on success, else error code.
 */

int mvx_v4l2_session_set_qp_epr(struct mvx_v4l2_session *vsession,
				    int *qp);

/**
 * mvx_v4l2_session_set_rate_control() - Set rate Control.
 * @vsession:	Pointer to v4l2 session.
 * @rc:	rate control type.
 *
 * Return: 0 on success, else error code.
 */

int mvx_v4l2_session_set_rate_control(struct mvx_v4l2_session *vsession,
				    struct v4l2_rate_control *rc);

/**
 * mvx_v4l2_session_set_dsl_frame() - Set DownScale dst frame.
 * @vsession:	Pointer to v4l2 session.
 * @dsl:	DownScale dst frame.
 *
 * Return: 0 on success, else error code.
 */

int mvx_v4l2_session_set_dsl_frame(struct mvx_v4l2_session *vsession,
				    struct v4l2_mvx_dsl_frame *dsl);

/**
 * mvx_v4l2_session_set_dsl_ratio() - Set DownScale ratio.
 * @vsession:	Pointer to v4l2 session.
 * @dsl:	DownScale ratio.
 *
 * Return: 0 on success, else error code.
 */

int mvx_v4l2_session_set_dsl_ratio(struct mvx_v4l2_session *vsession,
				    struct v4l2_mvx_dsl_ratio *dsl);

/**
 * mvx_v4l2_session_set_long_term_ref() - Set long term ref.
 * @vsession:	Pointer to v4l2 session.
 * @ltr:	long term ref.
 *
 * Return: 0 on success, else error code.
 */

int mvx_v4l2_session_set_long_term_ref(struct mvx_v4l2_session *vsession,
				    struct v4l2_mvx_long_term_ref *ltr);

/**
 * mvx_v4l2_session_set_dsl_mode() - Set DownScale mode.
 * @vsession:	Pointer to v4l2 session.
 * @mode:	DownScale mode, oly enable on high precision mode.
 *
 * Return: 0 on success, else error code.
 */

int mvx_v4l2_session_set_dsl_mode(struct mvx_v4l2_session *vsession,
				    int *mode);
#endif /* _MVX_V4L2_SESSION_H_ */
