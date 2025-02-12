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

#ifndef _MVX_V4L2_VIDIOC_H_
#define _MVX_V4L2_VIDIOC_H_

/****************************************************************************
 * Exported functions
 *
 * Callbacks for struct v4l2_ioctl_ops.
 *
 * Prototypes declared bellow implement certain v4l2 ioctls and used to
 * initialize members of v4l2_ioctl_ops structure.
 ****************************************************************************/

int mvx_v4l2_vidioc_querycap(struct file *file,
			     void *fh,
			     struct v4l2_capability *cap);

int mvx_v4l2_vidioc_enum_fmt_vid_cap(struct file *file,
				     void *fh,
				     struct v4l2_fmtdesc *f);

int mvx_v4l2_vidioc_enum_fmt_vid_out(struct file *file,
				     void *fh,
				     struct v4l2_fmtdesc *f);

int mvx_v4l2_vidioc_enum_framesizes(struct file *file,
				    void *fh,
				    struct v4l2_frmsizeenum *fsize);

int mvx_v4l2_vidioc_g_fmt_vid_cap(struct file *file,
				  void *fh,
				  struct v4l2_format *f);

int mvx_v4l2_vidioc_g_fmt_vid_out(struct file *file,
				  void *fh,
				  struct v4l2_format *f);

int mvx_v4l2_vidioc_s_fmt_vid_cap(struct file *file,
				  void *fh,
				  struct v4l2_format *f);

int mvx_v4l2_vidioc_s_fmt_vid_out(struct file *file,
				  void *fh,
				  struct v4l2_format *f);

int mvx_v4l2_vidioc_try_fmt_vid_cap(struct file *file,
				    void *fh,
				    struct v4l2_format *f);

int mvx_v4l2_vidioc_try_fmt_vid_out(struct file *file,
				    void *fh,
				    struct v4l2_format *f);

int mvx_v4l2_vidioc_g_crop(struct file *file,
			   void *fh,
			   struct v4l2_crop *a);
int mvx_v4l2_vidioc_g_selection(struct file *file, void *fh,
				  struct v4l2_selection *s);
int mvx_v4l2_vidioc_streamon(struct file *file,
			     void *priv,
			     enum v4l2_buf_type type);

int mvx_v4l2_vidioc_streamoff(struct file *file,
			      void *priv,
			      enum v4l2_buf_type type);

int mvx_v4l2_vidioc_encoder_cmd(struct file *file,
				void *priv,
				struct v4l2_encoder_cmd *cmd);

int mvx_v4l2_vidioc_try_encoder_cmd(struct file *file,
				    void *priv,
				    struct v4l2_encoder_cmd *cmd);

int mvx_v4l2_vidioc_decoder_cmd(struct file *file,
				void *priv,
				struct v4l2_decoder_cmd *cmd);

int mvx_v4l2_vidioc_try_decoder_cmd(struct file *file,
				    void *priv,
				    struct v4l2_decoder_cmd *cmd);

int mvx_v4l2_vidioc_reqbufs(struct file *file,
			    void *fh,
			    struct v4l2_requestbuffers *b);

int mvx_v4l2_vidioc_create_bufs(struct file *file,
				void *fh,
				struct v4l2_create_buffers *b);

int mvx_v4l2_vidioc_querybuf(struct file *file,
			     void *fh,
			     struct v4l2_buffer *b);

int mvx_v4l2_vidioc_qbuf(struct file *file,
			 void *fh,
			 struct v4l2_buffer *b);

int mvx_v4l2_vidioc_dqbuf(struct file *file,
			  void *fh,
			  struct v4l2_buffer *b);

int mvx_v4l2_vidioc_subscribe_event(struct v4l2_fh *fh,
				    const struct v4l2_event_subscription *sub);

long mvx_v4l2_vidioc_default(struct file *file,
			     void *fh,
			     bool valid_prio,
			     unsigned int cmd,
			     void *arg);
int mvx_v4l2_vidioc_s_selection(struct file *file, void *fh,
				  struct v4l2_selection *s);
int mvx_v4l2_vidioc_g_parm(struct file *file, void *fh,
			     struct v4l2_streamparm *a);
int mvx_v4l2_vidioc_s_parm(struct file *file, void *fh,
			     struct v4l2_streamparm *a);
#endif /* _MVX_V4L2_VIDIOC_H_ */
