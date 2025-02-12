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

#ifndef _MVX_V4L2_BUFFER_H_
#define _MVX_V4L2_BUFFER_H_

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/types.h>
#include <media/videobuf2-v4l2.h>
#include "mvx_buffer.h"
#include "mvx_if.h"
#include "mvx_v4l2_session.h"

/****************************************************************************
 * Types
 ****************************************************************************/

#define vb2_v4l2_to_mvx_v4l2_buffer(v4l2) \
	container_of(v4l2, struct mvx_v4l2_buffer, vb2_v4l2_buffer)

#define vb2_to_mvx_v4l2_buffer(vb2) \
	vb2_v4l2_to_mvx_v4l2_buffer(to_vb2_v4l2_buffer(vb2))

#define to_vb2_buf(vbuf) (&((vbuf)->vb2_v4l2_buffer.vb2_buf))

/**
 * struct mvx_v4l2_buffer - MVX V4L2 buffer.
 * @vb2_v4l2_buffer:	VB2 V4L2 buffer.
 * @buf:	MVX buffer.
 * @dentry:	Debug file system entry.
 */
struct mvx_v4l2_buffer {
	struct vb2_v4l2_buffer vb2_v4l2_buffer;
	struct mvx_buffer buf;
	struct dentry *dentry;
};

/****************************************************************************
 * Exported functions
 ****************************************************************************/

/**
 * mvx_v4l2_buffer_construct() - Construct MVX V4L2 buffer object.
 * @vbuf:	Pointer to MVX V4L2 buffer.
 * @vsession:	Pointer to V4L2 session.
 * @dir:	Direction of the buffer.
 * @nplanes:	Number of planes.
 * @sgt:	Array of pointers to scatter-gatter lists. Each SG list
 *              contains memory pages for a corresponding plane.
 *
 * Return: 0 on success, else error code.
 */
int mvx_v4l2_buffer_construct(struct mvx_v4l2_buffer *vbuf,
			      struct mvx_v4l2_session *vsession,
			      enum mvx_direction dir,
			      unsigned int nplanes,
			      struct sg_table **sgt);

/**
 * mvx_v4l2_buffer_destruct() - Destruct v4l2 buffer object.
 * @vbuf:	Pointer to MVX V4L2 buffer.
 */
void mvx_v4l2_buffer_destruct(struct mvx_v4l2_buffer *vbuf);

/**
 * mvx_buffer_to_v4l2_buffer() - Cast mvx_buffer to mvx_v4l2_buffer.
 * @buf:	Pointer MVX buffer.
 *
 * This function casts a pointer to struct mvx_buffer to a pointer to
 * a corresponding struct mvx_v4l2_buffer.
 *
 * Return: Pointer to corresponding mvx_v4l2_buffer object.
 */
struct mvx_v4l2_buffer *mvx_buffer_to_v4l2_buffer(struct mvx_buffer *buf);

/**
 * mvx_v4l2_buffer_set_status() - Set status for a buffer.
 * @vbuf:	Pointer to MVX V4L2 buffer.
 * @status:	Status to set.
 *
 * Status is a combination of the following flags:
 * V4L2_BUF_FLAG_QUEUED,
 * V4L2_BUF_FLAG_DONE,
 * V4L2_BUF_FLAG_PREPARED,
 * V4L2_BUF_FLAG_ERROR
 */
void mvx_v4l2_buffer_set_status(struct mvx_v4l2_buffer *vbuf,
				uint32_t status);

/**
 * mvx_v4l2_buffer_get_status() - Get the buffer status.
 * @vbuf:	Pointer to MVX V4L2 buffer.
 *
 * Return: Buffer status.
 */
uint32_t mvx_v4l2_buffer_get_status(struct mvx_v4l2_buffer *vbuf);

/**
 * mvx_v4l2_buffer_set() - Copy Vb2 buffer to VBUF.
 * @vbuf:	Destination MVX V4L2 buffer.
 * @b:		Source Vb2 buffer.
 *
 * Copies and validates paramters from 'b' to 'vbuf'.
 *
 * Return: 0 on success, else error code.
 */
int mvx_v4l2_buffer_set(struct mvx_v4l2_buffer *vbuf,
			struct vb2_buffer *b);

/**
 * mvx_v4l2_buffer_get() - Copy VBUF to V4L2 buffer.
 * @vbuf:	Source MVX V4L2 buffer.
 * @b:		Destination V4L2 buffer.
 *
 * Copies parameters from 'vbuf' to 'b'.
 */
void mvx_v4l2_buffer_get(struct mvx_v4l2_buffer *vbuf,
			 struct v4l2_buffer *b);

/**
 * mvx_v4l2_buffer_update() - Update the V4L2 buffer.
 * @vbuf:	Pointer to MVX V4L2 buffer.
 *
 * This function copies parameters from the MVX buffer to the V4L2 buffer.
 * It also sets the time stamp and validates that the buffer length is correct.
 * If an error is detectd the buffer length is cleared and the error flag
 * is set.
 *
 * This function should be called after the MVX buffer has changed, for example
 * after it has been returned by the firmware or flushed.
 *
 * Return: VB2_BUF_STATE_DONE on success, else VB2_BUF_STATE_ERROR.
 */
enum vb2_buffer_state mvx_v4l2_buffer_update(struct mvx_v4l2_buffer *vbuf);

#endif /* _MVX_V4L2_BUFFER_H_ */
