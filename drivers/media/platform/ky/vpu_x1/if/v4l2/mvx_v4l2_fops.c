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

#include <linux/fs.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-event.h>
#include "mvx_ext_if.h"
#include "mvx_v4l2_buffer.h"
#include "mvx_v4l2_ctrls.h"
#include "mvx_v4l2_fops.h"
#include "mvx_v4l2_session.h"
#include "mvx_v4l2_vidioc.h"
#include "mvx_log_group.h"

/****************************************************************************
 * Exported functions and variables
 ****************************************************************************/

int mvx_v4l2_open(struct file *file)
{
	struct mvx_ext_if *ctx = video_drvdata(file);
	struct mvx_v4l2_session *session;
	struct v4l2_format fmt = { 0 };
	int ret;

	session = devm_kzalloc(ctx->dev, sizeof(*session), GFP_KERNEL);
	if (session == NULL) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Failed to allocate V4L2 session.");
		return -ENOMEM;
	}

	MVX_SESSION_INFO(&session->session, "v4l2: Open device. id=%u.",
			 ctx->dev->id);

	ret = mvx_v4l2_session_construct(session, ctx);
	if (ret != 0)
		goto free_session;

	file->private_data = &session->fh;
	v4l2_fh_init(&session->fh, &ctx->vdev);
	v4l2_fh_add(&session->fh);

	/* Set default port formats. */
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420M;
	fmt.fmt.pix.width = 2;
	fmt.fmt.pix.height = 2;
	(void)mvx_v4l2_vidioc_s_fmt_vid_out(file, NULL, &fmt);

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	(void)mvx_v4l2_vidioc_s_fmt_vid_cap(file, NULL, &fmt);

	ret = mvx_v4l2_ctrls_init(&session->v4l2_ctrl);
	if (ret != 0) {
		MVX_SESSION_WARN(&session->session,
				 "Failed to register V4L2 controls handler. ret=%x",
				 ret);
		goto put_session;
	}

	session->fh.ctrl_handler = &session->v4l2_ctrl;

	return 0;

put_session:

	/*
	 * Session was completely constructed, so we have to destroy it
	 * gracefully using reference counting.
	 */
	mvx_session_put(&session->session);
	return ret;

free_session:
	devm_kfree(ctx->dev, session);

	return ret;
}

int mvx_v4l2_release(struct file *file)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	int i;
	int ret;

	MVX_SESSION_INFO(&vsession->session, "v4l2: Release.");

	mvx_v4l2_ctrls_done(vsession->fh.ctrl_handler);

	mutex_lock(&vsession->mutex);

	for (i = 0; i < MVX_DIR_MAX; i++)
		if (vsession->port[i].q_set != false) {
			vb2_queue_release(&vsession->port[i].vb2_queue);
			vsession->port[i].q_set = false;
		}

	ret = mvx_session_put(&vsession->session);
	if (ret == 0)
		mutex_unlock(&vsession->mutex);

	file->private_data = NULL;

	MVX_SESSION_INFO(&vsession->session, "v4l2: Release exit.");

	return 0;
}

unsigned int mvx_v4l2_poll(struct file *file,
			   struct poll_table_struct *wait)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	unsigned long events = poll_requested_events(wait);
	unsigned int revents = 0;

	mutex_lock(&vsession->mutex);

	if (vsession->session.error != 0) {
		revents = POLLERR;
		goto unlock_mutex;
	}

	/* POLLPRI events are handled by Vb2 */
	if (vsession->port[MVX_DIR_INPUT].q_set)
            revents |= vb2_poll(&vsession->port[MVX_DIR_INPUT].vb2_queue,
			    file, wait);
	if (vsession->port[MVX_DIR_OUTPUT].q_set)
            revents |= vb2_poll(&vsession->port[MVX_DIR_OUTPUT].vb2_queue,
			    file, wait);
#ifndef MODULE
	MVX_SESSION_VERBOSE(&vsession->session,
			    "v4l2: Poll. events=0x%lx, revents=0x%x, nevents=%d.",
			    events, revents, v4l2_event_pending(&vsession->fh));
#else
	MVX_SESSION_VERBOSE(&vsession->session,
			    "v4l2: Poll. events=0x%lx, revents=0x%x",
			    events, revents);
#endif
unlock_mutex:
	mutex_unlock(&vsession->mutex);

	return revents;
}

int mvx_v4l2_mmap(struct file *file,
		  struct vm_area_struct *vma)
{
	struct mvx_v4l2_session *session = file_to_session(file);
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	enum mvx_direction dir;
	struct mvx_v4l2_port *vport;
	struct vb2_queue *q;
	int ret;

	MVX_SESSION_INFO(&session->session,
			 "v4l2: Memory map. start=0x%08lx, end=0x%08lx, pgoff=0x%08lx, flags=0x%08lx.",
			 vma->vm_start, vma->vm_end,
			 vma->vm_pgoff, vma->vm_flags);

	if (offset >= DST_QUEUE_OFF_BASE) {
		dir = MVX_DIR_OUTPUT;
		vma->vm_pgoff -= (DST_QUEUE_OFF_BASE >> PAGE_SHIFT);
	} else {
		dir = MVX_DIR_INPUT;
	}

	vport = &session->port[dir];
	q = &vport->vb2_queue;

	ret = vb2_mmap(q, vma);
	if (ret != 0) {
		MVX_SESSION_WARN(&session->session,
				 "Failed to memory map buffer. q=%p, pgoff=0x%08lx, dir=%d, ret=%d",
				 q, vma->vm_pgoff, dir, ret);
		return ret;
	}

	return 0;
}
