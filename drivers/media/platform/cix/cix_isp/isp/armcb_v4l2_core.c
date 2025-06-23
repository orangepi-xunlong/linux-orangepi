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

#include "armcb_v4l2_core.h"
#include "armcb_camera_io_drv.h"
#include "armcb_isp.h"
#include "armcb_isp_driver.h"
#include "armcb_platform.h"
#include "armcb_register.h"
#include "armcb_v4l2_config.h"
#include "armcb_v4l2_stream.h"
#include "armcb_v4l_sd.h"
#include "armcb_vb2.h"
#include "asm-generic/errno-base.h"
#include "isp_hw_ops.h"
#include "linux/types.h"
#include "media/media-device.h"
#include "media/media-entity.h"
#include "system_dma.h"
#include "system_logger.h"
#include <linux/errno.h>
#include <linux/font.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/v4l2-dv-timings.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>
#include <media/v4l2-dv-timings.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-vmalloc.h>

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_ISP
#endif

#define ARMCB_MODULE_NAME "armcb_isp_v4l2"

/* if set disable the error injecting controls */
static bool no_error_inj;

static armcb_v4l2_dev_t *g_isp_v4l2_devs[ARMCB_MAX_DEVS] = { 0 };
static uint32_t outport_array[ARMCB_MAX_DEVS][V4L2_STREAM_TYPE_MAX];
static int g_adev_idx;
static armcb_v4l2_stream_t
	*g_outport_map[ARMCB_MAX_DEVS][ISP_OUTPUT_PORT_MAX] = { 0 };

/// define isp port token list
static char *const g_IspPortToken[] = {
	[ISP_OUTPUT_PORT_VIN] = (char *const) "VIN",
	[ISP_OUTPUT_PORT_3A] = (char *const) "3A",
	[ISP_OUTPUT_PORT_VOUT0] = (char *const) "VOUT0",
	[ISP_OUTPUT_PORT_VOUT1] = (char *const) "VOUT1",
	[ISP_OUTPUT_PORT_VOUT2] = (char *const) "VOUT2",
	[ISP_OUTPUT_PORT_VOUT3] = (char *const) "VOUT3",
	[ISP_OUTPUT_PORT_VOUT4] = (char *const) "VOUT4",
	[ISP_OUTPUT_PORT_VOUT5] = (char *const) "VOUT5",
	[ISP_OUTPUT_PORT_VOUT6] = (char *const) "VOUT6",
	[ISP_OUTPUT_PORT_VOUT7] = (char *const) "VOUT7",
	[ISP_OUTPUT_PORT_VOUT8] = (char *const) "VOUT8",
	[ISP_OUTPUT_PORT_VOUT9] = (char *const) "VOUT9",

	[ISP_OUTPUT_PORT_MAX] = NULL,
};

#define fh_to_private(__fh) container_of(__fh, struct armcb_isp_v4l2_fh, fh)

struct armcb_isp_v4l2_fh {
	struct v4l2_fh fh;
	uint32_t stream_id;
	uint32_t ctx_id;
	struct vb2_queue vb2_q;
};

armcb_v4l2_stream_t *armcb_v4l2_get_stream(uint32_t ctx_id, int stream_id)
{
	armcb_v4l2_stream_t *pstream = NULL;

	pstream = g_isp_v4l2_devs[ctx_id]->pstreams[stream_id];

	return pstream;
}

/* ----------------------------------------------------------------
 * stream finder utility function
 */
int armcb_v4l2_find_stream(armcb_v4l2_stream_t **ppstream, uint32_t ctx_id,
			   int stream_type)
{
	int stream_id = 0;
	*ppstream = NULL;

	if (stream_type >= V4L2_STREAM_TYPE_MAX || stream_type < 0 ||
		ctx_id >= ARMCB_MAX_DEVS) {
		LOG(LOG_ERR, "stream_id=%d, ctx_id=%d", stream_id, ctx_id);
		return -EINVAL;
	}

	if (g_isp_v4l2_devs[ctx_id] == NULL) {
		LOG(LOG_ERR, "ctx %d dev is NULL", ctx_id);
		return -EBUSY;
	}

	stream_id = g_isp_v4l2_devs[ctx_id]->stream_id_index[stream_type];
	if (stream_id < 0 || stream_id >= V4L2_STREAM_TYPE_MAX ||
		g_isp_v4l2_devs[ctx_id]->pstreams[stream_id] == NULL) {
		LOG(LOG_DEBUG, "stream_type:%d stream_id:%d", stream_type,
			stream_id);
		return -ENODEV;
	}

	*ppstream = g_isp_v4l2_devs[ctx_id]->pstreams[stream_id];
	LOG(LOG_DEBUG, "ctx_id=%d stream_id=%d stream=%p", ctx_id, stream_id,
		*ppstream);

	return 0;
}

int armcb_v4l2_find_ctx_stream_by_outport(uint32_t outport, uint32_t *p_ctx_id,
					  uint32_t *p_stream_id)
{
	uint32_t ctx_id = 0;
	uint32_t stream_id = 0;

	if (outport < 0 || p_ctx_id == NULL || p_stream_id == NULL) {
		LOG(LOG_ERR, "invalid parameter");
		return -EINVAL;
	}

	for (ctx_id = 0; ctx_id < ARMCB_MAX_DEVS; ctx_id++) {
		for (stream_id = 0; stream_id < V4L2_STREAM_TYPE_MAX;
			 stream_id++) {
			if (outport & outport_array[ctx_id][stream_id])
				goto exit;
		}
	}

	if (ctx_id == ARMCB_MAX_DEVS) {
		*p_ctx_id = -1;
		*p_stream_id = -1;
		LOG(LOG_ERR,
			"failed to find a valid ctx_id and stream_id for outport:%d",
			outport);
		return -EINVAL;
	}

exit:
	*p_ctx_id = ctx_id;
	*p_stream_id = stream_id;

	LOG(LOG_DEBUG, "success find ctx_id:%d stream_id:%d for outport:%d",
		ctx_id, stream_id, outport);
	return 0;
}

int armcb_v4l2_find_stream_by_outport_ctx(uint32_t outport, uint32_t ctx_id,
					  uint32_t *p_stream_id)
{
	uint32_t stream_id = 0;

	if (outport < 0 || ctx_id < 0 || p_stream_id == NULL) {
		LOG(LOG_ERR, "invalid parameter");
		return -EINVAL;
	}

	for (stream_id = 0; stream_id < V4L2_STREAM_TYPE_MAX; stream_id++) {
		if (outport & outport_array[ctx_id][stream_id])
			goto exit;
	}

	if (stream_id == V4L2_STREAM_TYPE_MAX) {
		*p_stream_id = -1;
		LOG(LOG_DEBUG,
			"failed to find a valid stream_id for outport:%d and ctx_id:%d",
			outport, ctx_id);
		return -EINVAL;
	}

exit:
	*p_stream_id = stream_id;

	LOG(LOG_DEBUG, "success find stream_id:%d for outport:%d and ctx_id:%d",
		stream_id, outport, ctx_id);
	return 0;
}

void armcb_isp_put_frame(uint32_t ctx_id, int stream_id, isp_output_port_t port)
{
	int rc = 0;
	armcb_v4l2_stream_t *pstream = NULL;
	armcb_v4l2_buffer_t *pbuf = NULL;
	struct vb2_buffer *vb = NULL;

	static armcb_v4l2_buffer_t *splastbuf;
	armcb_v4l2_buffer_t *plastbuf = NULL;

	if (stream_id < 0 && port < ISP_OUTPUT_PORT_MAX) {
		pstream = g_outport_map[ctx_id][port];
	} else {
		/* find stream pointer */
		rc = armcb_v4l2_find_stream(&pstream, ctx_id, stream_id);
		if (rc < 0) {
			LOG(LOG_WARN,
				"can't find stream on ctx %d stream_id %d (errno = %d)",
				ctx_id, stream_id, rc);
			return;
		}
	}

	LOG(LOG_DEBUG,
		"ctx_id:%d Stream#%d fmt(%d*%d %d %d) outport(%d %s) streamType(%d) "
		"reserved_buf_addr(0x%x)",
		ctx_id, pstream->stream_id, pstream->cur_v4l2_fmt.fmt.pix_mp.width,
		pstream->cur_v4l2_fmt.fmt.pix_mp.height,
		pstream->cur_v4l2_fmt.fmt.pix_mp.pixelformat,
		pstream->cur_v4l2_fmt.type, pstream->outport, g_IspPortToken[port],
		pstream->stream_type, pstream->reserved_buf_addr);

	/* check if stream is on */
	if (!pstream || !pstream->stream_started) {
		LOG(LOG_DEBUG, "[Stream#%d] is not started yet on ctx %d",
			stream_id, ctx_id);
		return;
	}
	LOG(LOG_DEBUG, "ctx_id:%d [Stream#%d] %p", ctx_id, pstream->stream_id,
		pstream);

	plastbuf = list_last_entry(&(pstream->stream_buffer_list_busy),
				   armcb_v4l2_buffer_t, list);

	/* try to get an active buffer from vb2 queue  */
	if (!list_empty(&pstream->stream_buffer_list_busy)) {
		if (!list_is_singular(&pstream->stream_buffer_list_busy) ||
			(plastbuf == splastbuf)) {
			pbuf = list_entry(pstream->stream_buffer_list_busy.next,
					  armcb_v4l2_buffer_t, list);
			list_del(&pbuf->list);
		}
	}
	splastbuf = list_last_entry(&(pstream->stream_buffer_list_busy),
					armcb_v4l2_buffer_t, list);

	if (!pbuf) {
		/// @TODO: need to use reserved buffer to hw output.
		LOG(LOG_DEBUG, "[Stream#%d] type: %d no empty buffers",
			pstream->stream_id, V4L2_STREAM_TYPE_VIDEO);
		return;
	}
	vb = &pbuf->vvb.vb2_buf;

	vb->timestamp = ktime_get_ns();
	vb2_buffer_done(vb, VB2_BUF_STATE_DONE);
	LOG(LOG_DEBUG, "%s put frame success ctx_id:%d stream_id:%d",
		g_IspPortToken[port], ctx_id, stream_id);
}

static int armcb_v4l2_querycap(struct file *file, void *priv,
				   struct v4l2_capability *cap)
{
	armcb_v4l2_dev_t *dev = video_drvdata(file);

	strcpy(cap->driver, "arm-china-isp");
	strcpy(cap->card, "linlon isp");
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s",
		 dev->v4l2_dev.name);

	/* V4L2_CAP_VIDEO_CAPTURE_MPLANE */
	cap->device_caps = V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_STREAMING |
			   V4L2_CAP_READWRITE;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

	// cap->capabilities = dev->vid_cap_caps | V4L2_CAP_DEVICE_CAPS;
	LOG(LOG_DEBUG, "capabilities(0x%x)", cap->capabilities);

	return 0;
}

static int armcb_v4l2_log_status(struct file *file, void *fh)
{
	return v4l2_ctrl_log_status(file, fh);
}

static bool armcb_is_in_use(struct video_device *vdev)
{
	unsigned long flags;
	bool res;

	spin_lock_irqsave(&vdev->fh_lock, flags);
	res = !list_empty(&vdev->fh_list);
	spin_unlock_irqrestore(&vdev->fh_lock, flags);
	return res;
}

static bool armcb_is_last_user(armcb_v4l2_dev_t *dev)
{
	unsigned int uses = armcb_is_in_use(&dev->vid_cap_dev);

	return uses == 1;
}

static int armcb_v4l2_fh_release(struct file *file)
{
	struct armcb_isp_v4l2_fh *sp = fh_to_private(file->private_data);
	armcb_v4l2_dev_t *dev = video_drvdata(file);

	LOG(LOG_DEBUG, "isp_v4l2 close: ctx_id: %d, called for sid:%d.",
		dev->ctx_id, sp->stream_id);
	if (sp) {
		v4l2_fh_del(&sp->fh);
		v4l2_fh_exit(&sp->fh);
	}
	kfree(sp);

	return 0;
}

static int armcb_v4l2_fop_release(struct file *file)
{
	armcb_v4l2_dev_t *dev = video_drvdata(file);
	struct armcb_isp_v4l2_fh *sp = fh_to_private(file->private_data);
	armcb_v4l2_stream_t *pstream = dev->pstreams[sp->stream_id];
	int outport_idx = -1;
	int open_counter;

	struct video_device *vdev = video_devdata(file);
	struct v4l2_event_subscription sub;
	int ret = 0;

	dev->stream_mask &= ~(1 << sp->stream_id);
	open_counter = atomic_sub_return(1, &dev->opened);

	/* deinit stream */
	if (pstream) {
		outport_idx = armcb_outport_bits_to_idx(pstream->outport);
		if (outport_idx >= 0 && outport_idx < ISP_OUTPUT_PORT_MAX)
			g_outport_map[sp->ctx_id][outport_idx] = NULL;
		if (pstream->stream_type < V4L2_STREAM_TYPE_MAX)
			dev->stream_id_index[pstream->stream_type] = -1;
		armcb_v4l2_stream_deinit(pstream);
		dev->pstreams[sp->stream_id] = NULL;
	}

	ret = armcb_isp_hw_apply_list(CMD_TYPE_STREAMOFF);
	if (ret < 0)
		LOG(LOG_ERR, "armcb_isp_hw_apply_list failed ret(%d)", ret);

	ret = armcb_isp_hw_apply_list(CMD_TYPE_POWERDOWN);
	if (ret < 0)
		LOG(LOG_ERR, "armcb_isp_hw_apply_list failed ret(%d)", ret);

	atomic_set(&dev->stream_on_cnt, 0);
	/// unsubscribe event when close file
	memset(&sub, 0, sizeof(sub));
	sub.type = V4L2_EVENT_ALL;
	ret = v4l2_event_unsubscribe(file->private_data, &sub);
	LOG(LOG_DEBUG, "v4l2_event_unsubscribe, ret = %d", ret);

	if (!no_error_inj && v4l2_fh_is_singular_file(file) &&
		!video_is_registered(vdev) && armcb_is_last_user(dev)) {
		/*
		 * I am the last user of this driver, and a disconnect
		 * was forced (since this video_device is unregistered),
		 * so re-register all video_device's again.
		 */
		v4l2_info(&dev->v4l2_dev, "reconnect");
		set_bit(V4L2_FL_REGISTERED, &dev->vid_cap_dev.flags);
	}

	/* release vb2 queue */
	if (sp->vb2_q.lock)
		mutex_lock(sp->vb2_q.lock);

	isp_vb2_queue_release(&sp->vb2_q);

	if (sp->vb2_q.lock)
		mutex_unlock(sp->vb2_q.lock);

	if (vdev->queue)
		return vb2_fop_release(file);

	/* release file handle */
	armcb_v4l2_fh_release(file);

	LOG(LOG_DEBUG, "release v4l2 fp success");
	return 0;
}

static int armcb_v4l2_fh_open(struct file *file)
{
	armcb_v4l2_dev_t *dev = video_drvdata(file);
	struct armcb_isp_v4l2_fh *sp = NULL;
	int i = 0;
	int stream_opened = 0;

	sp = kzalloc(sizeof(struct armcb_isp_v4l2_fh), GFP_KERNEL);
	if (!sp)
		return -ENOMEM;

	stream_opened = atomic_read(&dev->opened);
	if (stream_opened >= V4L2_STREAM_TYPE_MAX) {
		LOG(LOG_ERR,
			"too many open streams, stream_opened: %d, max: %d.",
			stream_opened, V4L2_STREAM_TYPE_MAX);
		kfree(sp);
		return -EBUSY;
	}

	file->private_data = &sp->fh;

	for (i = 0; i < V4L2_STREAM_TYPE_MAX; i++) {
		if ((dev->stream_mask & (1 << i)) == 0) {
			dev->stream_mask |= (1 << i);
			sp->stream_id = i;
			sp->ctx_id = dev->ctx_id;
			break;
		}
	}

	v4l2_fh_init(&sp->fh, &dev->vid_cap_dev);
	v4l2_fh_add(&sp->fh);

	LOG(LOG_DEBUG, "open v4l2 fp success");
	return 0;
}

static int armcb_v4l2_fop_open(struct file *filp)
{
	int rc = 0;
	struct armcb_isp_v4l2_fh *sp = NULL;
	armcb_v4l2_stream_t *pstream = NULL;
	armcb_v4l2_dev_t *dev = video_drvdata(filp);

	rc = armcb_v4l2_fh_open(filp);
	if (rc < 0) {
		LOG(LOG_ERR, "Error, file handle open fail (rc=%d)", rc);
		goto fh_open_fail;
	}

	sp = fh_to_private(filp->private_data);
	LOG(LOG_DEBUG, "isp_v4l2 open: ctx_id: %d, called for sid:%d.",
		dev->ctx_id, sp->stream_id);
	/* init stream */
	armcb_v4l2_stream_init(&dev->pstreams[sp->stream_id], sp->stream_id,
				   dev->ctx_id);
	pstream = dev->pstreams[sp->stream_id];
	if (pstream == NULL) {
		LOG(LOG_ERR, "stream alloc failed\n");
		return -ENOMEM;
	}

	/* init vb2 queue */
	rc = isp_vb2_queue_init(&sp->vb2_q, &dev->mutex,
				dev->pstreams[sp->stream_id],
				dev->v4l2_dev.dev);
	if (rc < 0) {
		LOG(LOG_ERR, "Error, vb2 queue init fail (rc=%d)", rc);
		goto vb2_q_fail;
	}

	atomic_add(1, &dev->opened);
	LOG(LOG_DEBUG, "open v4l2 fp success");

	return rc;

vb2_q_fail:
	armcb_v4l2_stream_deinit(dev->pstreams[sp->stream_id]);

	// too_many_stream:
	armcb_v4l2_fh_release(filp);

	/* update open counter */

fh_open_fail:
	return rc;
}

static ssize_t armcb_v4l2_fop_write(struct file *filep, const char __user *buf,
					size_t count, loff_t *ppos)
{
	struct armcb_isp_v4l2_fh *sp = fh_to_private(filep->private_data);
	int rc = 0;

	if (sp->vb2_q.lock && mutex_lock_interruptible(sp->vb2_q.lock))
		return -ERESTARTSYS;

	rc = vb2_write(&sp->vb2_q, buf, count, ppos,
			   filep->f_flags & O_NONBLOCK);

	if (sp->vb2_q.lock)
		mutex_unlock(sp->vb2_q.lock);
	return rc;
}

static ssize_t armcb_v4l2_fop_read(struct file *filep, char __user *buf,
				   size_t count, loff_t *ppos)
{
	struct armcb_isp_v4l2_fh *sp = fh_to_private(filep->private_data);
	int rc = 0;

	if (sp->vb2_q.lock && mutex_lock_interruptible(sp->vb2_q.lock))
		return -ERESTARTSYS;

	rc = vb2_read(&sp->vb2_q, buf, count, ppos,
			  filep->f_flags & O_NONBLOCK);

	if (sp->vb2_q.lock)
		mutex_unlock(sp->vb2_q.lock);
	return rc;
}

static unsigned int armcb_v4l2_fop_poll(struct file *filep,
					struct poll_table_struct *wait)
{
	struct armcb_isp_v4l2_fh *sp = fh_to_private(filep->private_data);
	int rc = 0;

	if (sp->vb2_q.lock && mutex_lock_interruptible(sp->vb2_q.lock))
		return POLLERR;

	rc = vb2_poll(&sp->vb2_q, filep, wait);

	if (sp->vb2_q.lock)
		mutex_unlock(sp->vb2_q.lock);

	return rc;
}

static int armcb_v4l2_fop_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct armcb_isp_v4l2_fh *sp = fh_to_private(file->private_data);
	int rc = 0;

	rc = vb2_mmap(&sp->vb2_q, vma);

	return rc;
}

static const struct v4l2_file_operations armcb_fops = {
	.owner = THIS_MODULE,
	.open = armcb_v4l2_fop_open,
	.release = armcb_v4l2_fop_release,
	.read = armcb_v4l2_fop_read,
	.write = armcb_v4l2_fop_write,
	.poll = armcb_v4l2_fop_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap = armcb_v4l2_fop_mmap,
};

/* Per-stream control operations */
static inline bool armcb_v4l2_is_q_busy(struct vb2_queue *queue,
					struct file *file)
{
	if (queue->owner && queue->owner != file->private_data)
		LOG(LOG_ERR, "vb2_queue %p is busy!", queue);

	return queue->owner && queue->owner != file->private_data;
}

static int armcb_v4l2_streamon(struct file *file, void *priv,
				   enum v4l2_buf_type buf_type)
{
	int rc = -1;
	armcb_v4l2_dev_t *dev = video_drvdata(file);
	struct armcb_isp_v4l2_fh *sp = fh_to_private(priv);
	armcb_v4l2_stream_t *pstream = dev->pstreams[sp->stream_id];

	if (armcb_v4l2_is_q_busy(&sp->vb2_q, file))
		return -EBUSY;

	rc = vb2_streamon(&sp->vb2_q, buf_type);
	if (rc != 0) {
		LOG(LOG_ERR, "fail to vb2_streamon. (rc=%d)", rc);
		return rc;
	}

	/*config first frame output address*/
	rc = armcb_v4l2_config_update_stream_vin_addr(pstream);
	if (rc != 0) {
		LOG(LOG_ERR,
			"fail to update stream vin addr. (stream_id = %d, rc=%d)",
			sp->stream_id, rc);
	}

	rc = armcb_v4l2_config_update_stream_hw_addr(pstream);
	if (rc != 0) {
		LOG(LOG_ERR,
			"fail to update stream output addr. (stream_id = %d, rc=%d)",
			sp->stream_id, rc);
	}

	/* Start hardware */
	if (atomic_read(&dev->stream_on_cnt) == 0) {
		rc = armcb_isp_hw_apply_list(CMD_TYPE_STREAMON);
		if (rc < 0) {
			LOG(LOG_ERR, "armcb_isp_hw_apply_list failed ret(%d)",
				rc);
		}
	}

	atomic_add(1, &dev->stream_on_cnt);
	LOG(LOG_INFO, "ctx_id:%d, stream_id:%d",
		dev->ctx_id, sp->stream_id);

	rc = armcb_v4l2_stream_on(pstream);
	if (rc != 0) {
		LOG(LOG_ERR, "fail to isp_stream_on. (stream_id = %d, rc=%d)",
			sp->stream_id, rc);
		armcb_v4l2_stream_off(pstream);
		return rc;
	}

	return rc;
}

static int armcb_v4l2_streamoff(struct file *file, void *priv,
				enum v4l2_buf_type buf_type)
{
	int rc = -1;
	armcb_v4l2_dev_t *dev = video_drvdata(file);
	struct armcb_isp_v4l2_fh *sp = fh_to_private(priv);
	armcb_v4l2_stream_t *pstream = dev->pstreams[sp->stream_id];

	if (atomic_read(&dev->stream_on_cnt) == 1) {
		rc = armcb_isp_hw_apply_list(CMD_TYPE_STREAMOFF);
		if (rc < 0) {
			LOG(LOG_ERR, "armcb_isp_hw_apply_list failed ret(%d)",
				rc);
		}
	}
	LOG(LOG_INFO, "ctx_id:%d, stream_id:%d",
		dev->ctx_id, sp->stream_id);
	/* Stop hardware */
	armcb_v4l2_stream_off(pstream);

	/* vb streamoff */
	rc = vb2_streamoff(&sp->vb2_q, buf_type);

	atomic_sub_return(1, &dev->stream_on_cnt);

	return rc;
}

int armcb_v4l2_g_fmt_vid_cap_mplane(struct file *file, void *priv,
					struct v4l2_format *f)
{
	/*empty function*/
	/*we get fmt from userspace*/
	return 0;
}

int armcb_v4l2_s_fmt_vid_cap_mplane(struct file *file, void *priv,
					struct v4l2_format *f)
{
	armcb_v4l2_dev_t *dev = video_drvdata(file);
	struct armcb_isp_v4l2_fh *sp = fh_to_private(file->private_data);
	armcb_v4l2_stream_t *pstream = dev->pstreams[sp->stream_id];
	struct vb2_queue *q = &sp->vb2_q;
	int outport_idx = -1;
	int rc = 0;

	LOG(LOG_INFO, "ctx_id:%d stream_id:%d", sp->ctx_id, sp->stream_id);

	if (vb2_is_busy(q))
		return -EBUSY;

	rc = armcb_v4l2_stream_set_format(pstream, f);
	if (rc < 0) {
		LOG(LOG_ERR, "set format failed.");
		return rc;
	}

	/* update stream pointer index */
	dev->stream_id_index[pstream->stream_type] = pstream->stream_id;
	outport_array[sp->ctx_id][sp->stream_id] = pstream->outport;

	outport_idx = armcb_outport_bits_to_idx(pstream->outport);
	if (outport_idx < 0 || outport_idx >= ISP_OUTPUT_PORT_MAX) {
		rc = -EINVAL;
		LOG(LOG_ERR, "invalid outport idx:%d, bits:%#x\n", outport_idx,
			pstream->outport);
		return rc;
	}

	if (g_outport_map[sp->ctx_id][outport_idx]) {
		rc = -EINVAL;
		LOG(LOG_ERR, "busy outport idx:%d, bits:%#x\n", outport_idx,
			pstream->outport);
		return rc;
	}

	g_outport_map[sp->ctx_id][outport_idx] = pstream;

	LOG(LOG_INFO,
		"ctx_id:%d stream_id:%d stream_type:%d, outport_idx:%d, outport:%d",
		pstream->ctx_id, pstream->stream_id, pstream->stream_type,
		outport_idx, pstream->outport);

	return 0;
}

int armcb_v4l2_try_fmt_vid_cap_mplane(struct file *file, void *priv,
					  struct v4l2_format *f)
{
	/*empty function*/
	/*we get fmt from userspace*/
	return 0;
}

/* vb2 customization for multi-stream support */
static int armcb_v4l2_reqbufs(struct file *file, void *priv,
				  struct v4l2_requestbuffers *p)
{
	struct armcb_isp_v4l2_fh *sp = fh_to_private(file->private_data);
	int rc = 0;

	LOG(LOG_INFO, "(stream_id = %d, ownermatch=%d)", sp->stream_id,
		armcb_v4l2_is_q_busy(&sp->vb2_q, file));
	if (armcb_v4l2_is_q_busy(&sp->vb2_q, file))
		return -EBUSY;

	rc = vb2_reqbufs(&sp->vb2_q, p);
	if (rc == 0)
		sp->vb2_q.owner = p->count ? file->private_data : NULL;

	LOG(LOG_INFO, "sid:%d reqbuf p->type:%d p->memory %d p->count %d rc %d",
		sp->stream_id, p->type, p->memory, p->count, rc);
	return rc;
}

static int armcb_v4l2_expbuf(struct file *file, void *priv,
				 struct v4l2_exportbuffer *p)
{
	struct armcb_isp_v4l2_fh *sp = fh_to_private(file->private_data);
	int rc = 0;

	if (armcb_v4l2_is_q_busy(&sp->vb2_q, file))
		return -EBUSY;

	rc = vb2_expbuf(&sp->vb2_q, p);
	LOG(LOG_DEBUG, "expbuf sid:%d type:%d index:%d plane:%d rc: %d",
		sp->stream_id, p->type, p->index, p->plane, rc);

	return rc;
}

static int armcb_v4l2_querybuf(struct file *file, void *priv,
				   struct v4l2_buffer *p)
{
	struct armcb_isp_v4l2_fh *sp = fh_to_private(file->private_data);
	int rc = 0;

	rc = vb2_querybuf(&sp->vb2_q, p);
	LOG(LOG_DEBUG, "sid:%d querybuf p->type:%d p->index:%d , rc %d",
		sp->stream_id, p->type, p->index, rc);
	return rc;
}

static int armcb_v4l2_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	int rc = 0;
	armcb_v4l2_stream_t *pstream = NULL;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	armcb_v4l2_dev_t *dev = video_drvdata(file);
#endif
	struct armcb_isp_v4l2_fh *sp = fh_to_private(file->private_data);

	/* find stream pointer */
	pstream = armcb_v4l2_get_stream(sp->ctx_id, sp->stream_id);
	if (pstream) {
		if (pstream->stream_started == 0) {
			if (p->reserved) {
				LOG(LOG_WARN,
					"set reserved buffer %p userptr:%p", p,
					p->m.planes->m.userptr);
				pstream->reserved_buf_addr =
					(u32)p->m.planes->m.userptr;
				return 0;
			}
		}
	}

	LOG(LOG_DEBUG, "ctx_id:%d stream_id = %d, ownermatch=%d", sp->ctx_id,
		sp->stream_id, armcb_v4l2_is_q_busy(&sp->vb2_q, file));
	if (armcb_v4l2_is_q_busy(&sp->vb2_q, file))
		return -EBUSY;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	rc = vb2_qbuf(&sp->vb2_q, dev->v4l2_dev.mdev, p);
#else
	rc = vb2_qbuf(&sp->vb2_q, p);
#endif
	LOG(LOG_DEBUG,
		"ctx_id:%d stream_id:%d qbuf p->type:%d p->index:%d, rc %d",
		sp->ctx_id, sp->stream_id, p->type, p->index, rc);
	return rc;
}

static int armcb_v4l2_dqbuf(struct file *file, void *priv,
				struct v4l2_buffer *p)
{
	struct armcb_isp_v4l2_fh *sp = fh_to_private(file->private_data);
	int rc = 0;

	LOG(LOG_DEBUG, "ctx_id:%d stream_id = %d, ownermatch=%d", sp->ctx_id,
		sp->stream_id, armcb_v4l2_is_q_busy(&sp->vb2_q, file));
	if (armcb_v4l2_is_q_busy(&sp->vb2_q, file))
		return -EBUSY;

	rc = vb2_dqbuf(&sp->vb2_q, p, file->f_flags & O_NONBLOCK);
	LOG_RATELIMITED(
		LOG_DEBUG,
		"ctx_id:%d stream_id:%d dqbuf p->type:%d p->index:%d, rc %d",
		sp->ctx_id, sp->stream_id, p->type, p->index, rc);
	return rc;
}

static const struct v4l2_ioctl_ops armcb_ioctl_ops = {
	.vidioc_querycap = armcb_v4l2_querycap,

	.vidioc_g_fmt_vid_cap_mplane = armcb_v4l2_g_fmt_vid_cap_mplane,
	.vidioc_s_fmt_vid_cap_mplane = armcb_v4l2_s_fmt_vid_cap_mplane,
	.vidioc_try_fmt_vid_cap_mplane = armcb_v4l2_try_fmt_vid_cap_mplane,

	.vidioc_reqbufs = armcb_v4l2_reqbufs,
	.vidioc_expbuf = armcb_v4l2_expbuf,
	.vidioc_querybuf = armcb_v4l2_querybuf,

	.vidioc_qbuf = armcb_v4l2_qbuf,
	.vidioc_dqbuf = armcb_v4l2_dqbuf,

	.vidioc_streamon = armcb_v4l2_streamon,
	.vidioc_streamoff = armcb_v4l2_streamoff,

	.vidioc_log_status = armcb_v4l2_log_status,
	.vidioc_subscribe_event = NULL,
	.vidioc_unsubscribe_event = NULL,
};

/*-----------------------------------------------------------------
 *	Initialization and module stuff
 *  ---------------------------------------------------------------
 */
armcb_v4l2_dev_t *armcb_v4l2_core_get_dev(uint32_t ctx_id)
{
	LOG(LOG_DEBUG, "get ctx_id:%d pdev:%p", ctx_id,
		g_isp_v4l2_devs[ctx_id]);
	return g_isp_v4l2_devs[ctx_id];
}

uint32_t armcb_v4l2_core_find_1st_opened_dev(void)
{
	uint32_t i = 0;
	armcb_v4l2_dev_t *pdev = NULL;

	for (i = 0; i < ARMCB_MAX_DEVS; i++) {
		pdev = armcb_v4l2_core_get_dev(i);
		if (pdev && atomic_read(&pdev->opened) > 0)
			break;
	}

	if (i == ARMCB_MAX_DEVS)
		LOG(LOG_WARN, "No v4l2 device opened");
	return i;
}

static void armcb_v4l2_dev_release(struct v4l2_device *v4l2_dev)
{
	armcb_v4l2_dev_t *dev =
		container_of(v4l2_dev, armcb_v4l2_dev_t, v4l2_dev);

	v4l2_device_unregister(&dev->v4l2_dev);
	kfree(dev);
}

static armcb_v4l2_dev_t *
armcb_v4l2_create_instance(struct platform_device *pdev, int ctx_id,
			   struct device *devnode)
{
	armcb_v4l2_dev_t *dev = NULL;
	struct video_device *vfd = NULL;
	// struct vb2_queue *q = NULL;
	int ret = 0;
	int i = 0;

	LOG(LOG_INFO, " ctx_id(%d) +", ctx_id);
	/* allocate main vivid state structure */
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		LOG(LOG_ERR, "failed to alloc memory for armcb dev.");
		return NULL;
	}

	dev->ctx_id = ctx_id;
	if (pdev)
		dev->pvdev = pdev;
	else if (devnode)
		dev->pvdev = (struct platform_device *)devnode;
	else {
		LOG(LOG_ERR, "invalid device for drivers.");
		kfree(dev);
		return NULL;
	}

#ifdef CONFIG_MEDIA_CONTROLLER
	dev->v4l2_dev.mdev = &dev->mdev;

	/* Initialize media device */
	(void)strscpy(dev->mdev.model, ARMCB_MODULE_NAME,
			  sizeof(dev->mdev.model));
	snprintf(dev->mdev.bus_info, sizeof(dev->mdev.bus_info),
		 "platform:%s-%03d", ARMCB_MODULE_NAME, ctx_id);
	dev->mdev.dev = devnode;
	media_device_init(&dev->mdev);
#endif

	/* register v4l2_device */
	snprintf(dev->v4l2_dev.name, sizeof(dev->v4l2_dev.name), "%s-%02d",
		 ARMCB_MODULE_NAME, ctx_id);
	LOG(LOG_INFO, "dev->v4l2_dev.name[%s]", dev->v4l2_dev.name);
	ret = v4l2_device_register(devnode, &dev->v4l2_dev);
	if (ret) {
		kfree(dev);
		return NULL;
	}
	dev->v4l2_dev.release = armcb_v4l2_dev_release;
	dev->v4l2_dev.notify = armcb_v4l2_subdev_notify;

	/* set up the capabilities of the video capture device */
	dev->vid_cap_caps = V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_STREAMING |
				V4L2_CAP_READWRITE;
	/* initialize locks */
	spin_lock_init(&dev->slock);
	spin_lock_init(&dev->v4l2_event_slock);

	mutex_init(&dev->mutex);
	mutex_init(&dev->v4l2_event_mutex);
	mutex_init(&dev->ordered_sd_mutex);

	/* init subdev list */
	INIT_LIST_HEAD(&dev->ordered_sd_list);

	/* defualt video device*/
	dev->has_vid_cap = 1;

	/* initialize stream id table */
	for (i = 0; i < V4L2_STREAM_TYPE_MAX; i++)
		dev->stream_id_index[i] = -1;
	/* initialize open counter */
	atomic_set(&dev->stream_on_cnt, 0);
	atomic_set(&dev->opened, 0);

	/* finally start creating the device nodes */
	if (dev->has_vid_cap) {
		vfd = &dev->vid_cap_dev;
		snprintf(vfd->name, sizeof(vfd->name), "armcb-%02d-vid-cap",
			 ctx_id);
		vfd->fops = &armcb_fops;
		vfd->ioctl_ops = &armcb_ioctl_ops;
		vfd->device_caps = dev->vid_cap_caps;
		vfd->release = video_device_release_empty;
		vfd->v4l2_dev = &dev->v4l2_dev;
		vfd->queue =
			NULL; //&dev->vb_vid_cap_q; // queue will be customized in file handle
		vfd->tvnorms = 0;

		/*
		 * Provide a mutex to v4l2 core. It will be used to protect
		 * all fops and v4l2 ioctls.
		 */
		vfd->lock = &dev->mutex;
		video_set_drvdata(vfd, dev);
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
		ret = video_register_device(vfd, VFL_TYPE_VIDEO, -1);
#else
		ret = video_register_device(vfd, VFL_TYPE_GRABBER, -1);
#endif
		if (ret < 0)
			goto unreg_dev;
		LOG(LOG_INFO, "V4L2 capture device registered as %s",
			video_device_node_name(vfd));
		LOG(LOG_INFO,
			"[has_vid_cap] vfd->name[%s] v4l2_dev.name[%s] dev_name[%s]",
			vfd->name, dev->v4l2_dev.name, video_device_node_name(vfd));
	}

	/* Now that everything is fine, let's add it to device list */
	LOG(LOG_INFO, "create video device instance success");
	return dev;

unreg_dev:
	video_unregister_device(&dev->vid_cap_dev);
	v4l2_device_put(&dev->v4l2_dev);
	kfree(dev);
	LOG(LOG_ERR, "create video device instance failed. ret = %d", ret);
	return NULL;
}

armcb_v4l2_dev_t *armcb_register_instance(struct platform_device *pdev,
					  struct device *devnode,
					  unsigned int cam_id)
{
	armcb_v4l2_dev_t *adev = NULL;

	if (cam_id + 1 >= ARMCB_MAX_DEVS) {
		LOG(LOG_ERR, "too many instance, current is %d.", cam_id);
		return NULL;
	}
	if (g_isp_v4l2_devs[cam_id] != NULL) {
		LOG(LOG_ERR, "camera %d has probe.", cam_id);
		return NULL;
	}

	adev = armcb_v4l2_create_instance(pdev, cam_id, devnode);
	if (adev == NULL) {
		LOG(LOG_ERR, "too many instance, current is %d.", cam_id);
		return NULL;
	}

	LOG(LOG_INFO, "register v4l2 video instance %d %p", cam_id, adev);
	g_isp_v4l2_devs[cam_id] = adev;
	return adev;
}

void armcb_cam_instance_destroy(void)
{
	int i = 0;

	for (; i < ARMCB_MAX_DEVS; i++) {
		if (g_isp_v4l2_devs[i] == NULL)
			continue;
		v4l2_async_nf_unregister(&g_isp_v4l2_devs[i]->dts_notifier);
#if (KERNEL_VERSION(4, 17, 0) <= LINUX_VERSION_CODE)
		v4l2_async_nf_cleanup(&g_isp_v4l2_devs[i]->dts_notifier);
#endif
		video_unregister_device(&g_isp_v4l2_devs[i]->vid_cap_dev);
		v4l2_device_put(&g_isp_v4l2_devs[i]->v4l2_dev);
		media_device_unregister(&g_isp_v4l2_devs[i]->mdev);
		LOG(LOG_INFO, "release armcb instance %d (%p)", i,
			g_isp_v4l2_devs[i]);
		g_isp_v4l2_devs[i] = NULL;
	}
	g_adev_idx = 0;
}
