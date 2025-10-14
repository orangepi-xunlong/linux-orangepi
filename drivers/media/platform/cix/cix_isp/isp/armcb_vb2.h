/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _ARMCB_VB2_H_
#define _ARMCB_VB2_H_

struct armcb_vb2_private_data {
	void *vaddr;
	unsigned long size;
	/* Offset of the plane inside the buffer */
	struct device *alloc_ctx;
};

struct vb2_ops *armcb_vb2_get_q_ops(void);
struct vb2_mem_ops *armcb_vb2_get_q_mem_ops(void);

int armcb_s_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f);
int armcb_g_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f);
int armcb_try_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f);
int armcb_s_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f);

int isp_vb2_queue_init(struct vb2_queue *q, struct mutex *mlock,
		       armcb_v4l2_stream_t *pstream, struct device *dev);
void isp_vb2_queue_release(struct vb2_queue *q);
#endif
