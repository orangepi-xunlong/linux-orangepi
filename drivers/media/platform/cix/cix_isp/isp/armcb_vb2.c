// SPDX-License-Identifier: GPL-2.0
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
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/v4l2-dv-timings.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>
#include <media/v4l2-common.h>
#include <media/v4l2-dv-timings.h>
#include <media/v4l2-event.h>
#include <media/v4l2-rect.h>

#include "armcb_isp_driver.h"
#include "armcb_v4l2_core.h"
#include "armcb_vb2.h"
#include "system_logger.h"

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_ISP
#endif

static int armcb_vb2_queue_setup(struct vb2_queue *vq, unsigned int *nbuffers,
				 unsigned int *nplanes, unsigned int sizes[],
				 struct device *alloc_devs[])
{
	int i = 0;
	int rc = -EINVAL;
	static unsigned long cnt;

	armcb_v4l2_stream_t *pstream = vb2_get_drv_priv(vq);
	struct v4l2_format vfmt;

	LOG(LOG_INFO, "Enter id:%d, cnt: %lu.", pstream->stream_id, cnt++);
	LOG(LOG_INFO, "vq: %p, *nplanes: %u.", vq, *nplanes);

	// get current format
	if (armcb_v4l2_stream_get_format(pstream, &vfmt) < 0) {
		LOG(LOG_ERR, "fail to get format from stream");
		return -EBUSY;
	}

	/* we just need to use one plane to store our image */
	if (vfmt.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		if (WARN_ON(vfmt.fmt.pix_mp.num_planes > VIDEO_MAX_PLANES))
			goto done;

		*nplanes = vfmt.fmt.pix_mp.num_planes;
		for (i = 0; i < vfmt.fmt.pix_mp.num_planes; i++)
			sizes[i] = vfmt.fmt.pix_mp.plane_fmt[i].sizeimage;
	} else {
		LOG(LOG_ERR, "Unsupported buf type :%d", vfmt.type);
		goto done;
	}

	rc = 0;

	LOG(LOG_INFO, "deivce inst:%d vb2 queue setup, plane size %u %u %u",
		pstream->ctx_id, sizes[0], sizes[1], sizes[2]);
done:
	return rc;
}

static void armcb_vb2_buf_finish(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);

	vbuf->flags |= V4L2_BUF_FLAG_TIMECODE;
}

static void armcb_vb2_buf_queue(struct vb2_buffer *vb)
{
	armcb_v4l2_stream_t *pstream = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vvb = to_vb2_v4l2_buffer(vb);
	armcb_v4l2_buffer_t *buf = container_of(vvb, armcb_v4l2_buffer_t, vvb);
	static unsigned long cnt;

	LOG(LOG_DEBUG, "Enter id:%d, cnt: %lu.", pstream->stream_id, cnt++);

	spin_lock(&pstream->slock);
	list_add_tail(&buf->list, &pstream->stream_buffer_list);
	spin_unlock(&pstream->slock);
}

static void *armcb_vb2_cma_get_userptr(struct vb2_buffer *vb,
					   struct device *alloc_ctx,
					   unsigned long vaddr, unsigned long size)
{
	struct armcb_vb2_private_data *priv;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return ERR_PTR(-ENOMEM);
	LOG(LOG_DEBUG, "get userptr 0x%x size %d", vaddr, size);
	priv->vaddr = (void *)vaddr;
	priv->size = size;
	priv->alloc_ctx = alloc_ctx;
	return priv;
}

static void armcb_vb2_cma_put_userptr(void *buf_priv)
{
	kfree(buf_priv);
}

static void *armcb_vb2_cma_vaddr(struct vb2_buffer *vb, void *buf_priv)
{
	struct armcb_vb2_private_data *buf = buf_priv;

	if (!buf->vaddr) {
		LOG(LOG_ERR,
			"Address of an unallocated plane requested or cannot map user pointer");
		return NULL;
	}

	LOG(LOG_DEBUG, "addr=%p", buf->vaddr);
	return buf->vaddr;
}

static struct vb2_mem_ops armcb_vb2_get_q_mem_op = {
	.get_userptr = armcb_vb2_cma_get_userptr,
	.put_userptr = armcb_vb2_cma_put_userptr,
	.vaddr = armcb_vb2_cma_vaddr,
};

struct vb2_mem_ops *armcb_vb2_get_q_mem_ops(void)
{
	return &armcb_vb2_get_q_mem_op;
}

static struct vb2_ops armcb_vid_cap_qops = {
	.queue_setup = armcb_vb2_queue_setup,
	.buf_queue = armcb_vb2_buf_queue,
	.buf_finish = armcb_vb2_buf_finish,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
};

struct vb2_ops *armcb_vb2_get_q_ops(void)
{
	return &armcb_vid_cap_qops;
}

int isp_vb2_queue_init(struct vb2_queue *q, struct mutex *mlock,
			   armcb_v4l2_stream_t *pstream, struct device *dev)
{
	memset(q, 0, sizeof(struct vb2_queue));

	/* start creating the vb2 queues */

	// all stream multiplanar
	q->type = pstream->cur_v4l2_fmt.type;

	LOG(LOG_DEBUG, "vb2 init for stream:%d type: %u.", pstream->stream_id,
		q->type);

	q->io_modes = VB2_USERPTR | VB2_READ;
	q->drv_priv = pstream;
	q->buf_struct_size = sizeof(armcb_v4l2_buffer_t);
	q->ops = armcb_vb2_get_q_ops();
	q->mem_ops = armcb_vb2_get_q_mem_ops();
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->min_buffers_needed = 1;
	q->lock = mlock;
	q->dev = dev;

	return vb2_queue_init(q);
}

void isp_vb2_queue_release(struct vb2_queue *q)
{
	vb2_queue_release(q);
}
