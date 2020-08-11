
/*
 * vin_video.c for video api
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 * Yang Feng <yangfeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/version.h>
#include <linux/videodev2.h>
#include <linux/string.h>
#include <linux/freezer.h>
#include <linux/sort.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/moduleparam.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-common.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf2-dma-contig.h>

#include <linux/regulator/consumer.h>
#ifdef CONFIG_DEVFREQ_DRAM_FREQ_WITH_SOFT_NOTIFY
#include <linux/sunxi_dramfreq.h>
#endif

#include "../utility/config.h"
#include "../utility/sensor_info.h"
#include "../modules/sensor/sensor_helper.h"
#include "../utility/vin_io.h"
#include "../vin-csi/sunxi_csi.h"
#include "../vin-isp/sunxi_isp.h"
#include "../vin-vipp/sunxi_scaler.h"
#include "../vin-mipi/sunxi_mipi.h"
#include "../vin.h"

#define VIN_MAJOR_VERSION 1
#define VIN_MINOR_VERSION 0
#define VIN_RELEASE       0

#define VIN_VERSION \
		KERNEL_VERSION(VIN_MAJOR_VERSION, VIN_MINOR_VERSION, VIN_RELEASE)

void __vin_s_stream_handle(struct work_struct *work)
{
	int ret = 0;
	struct vin_vid_cap *cap =
			container_of(work, struct vin_vid_cap, s_stream_task);
	ret = vin_pipeline_call(cap->vinc, set_stream, &cap->pipe, cap->vinc->stream_idx);
	if (ret < 0) {
		vin_err("%s error!\n", __func__);
		return;
	}
	/*set saved exp and gain for reopen*/
	if (cap->vinc->exp_gain.exp_val && cap->vinc->exp_gain.gain_val) {
		v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core, ioctl,
			VIDIOC_VIN_SENSOR_EXP_GAIN, &cap->vinc->exp_gain);
	}

	vin_log(VIN_LOG_VIDEO, "%s done, id = %d!\n", __func__, cap->vinc->id);
}

void __vin_pipeline_reset_handle(struct work_struct *work)
{
	int ret = 0;
	struct vin_vid_cap *cap =
			container_of(work, struct vin_vid_cap, pipeline_reset_task);
	struct v4l2_subdev *sd	= cap->pipe.sd[VIN_IND_SENSOR];

	if (cap->capture_mode != V4L2_MODE_IMAGE) {
		mutex_lock(&cap->lock);
		cap->first_flag = 0;
		cap->vinc->vin_status.frame_cnt = 0;
		cap->vinc->vin_status.err_cnt = 0;
		cap->vinc->vin_status.lost_cnt = 0;

		vin_pipeline_call(cap->vinc, set_stream, &cap->pipe, 0);

		v4l2_subdev_call(sd, core, s_power, 0);
		usleep_range(100, 120);
		v4l2_subdev_call(sd, core, s_power, 1);

		ret = vin_pipeline_call(cap->vinc, set_stream, &cap->pipe, cap->vinc->stream_idx);
		if (ret < 0) {
			vin_err("%s error!\n", __func__);
			return;
		}
		vin_warn("vin pipiline reset after interrupt timeout!\n");

		mod_timer(&cap->vinc->timer_for_reset, jiffies + 3*HZ);
		mutex_unlock(&cap->lock);
	}
}

/* The color format (colplanes, memplanes) must be already configured. */
int vin_set_addr(struct vin_core *vinc, struct vb2_buffer *vb,
		      struct vin_frame *frame, struct vin_addr *paddr)
{
	u32 pix_size, depth, y_stride, u_stride, v_stride;

	if (vb == NULL || frame == NULL)
		return -EINVAL;

	pix_size = ALIGN(frame->o_width, VIN_ALIGN_WIDTH) * frame->o_height;
	depth = frame->fmt.depth[0] + frame->fmt.depth[1] + frame->fmt.depth[2];

	paddr->y = vb2_dma_contig_plane_dma_addr(vb, 0);

	if (frame->fmt.memplanes == 1) {
		switch (frame->fmt.colplanes) {
		case 1:
			paddr->cb = 0;
			paddr->cr = 0;
			break;
		case 2:
			/* decompose Y into Y/Cb */

			if (frame->fmt.fourcc == V4L2_PIX_FMT_FBC) {
				paddr->cb = (u32)(paddr->y + CEIL_EXP(frame->o_width, 7) * CEIL_EXP(frame->o_height, 5) * 96);
				paddr->cr = 0;

			} else {
				paddr->cb = (u32)(paddr->y + pix_size);
				paddr->cr = 0;
			}
			break;
		case 3:
			paddr->cb = (u32)(paddr->y + pix_size);
			/* 420 */
			if (frame->fmt.depth[0] == 12)
				paddr->cr = (u32)(paddr->cb + (pix_size >> 2));
			else /* 422 */
				paddr->cr = (u32)(paddr->cb + (pix_size >> 1));
			break;
		default:
			return -EINVAL;
		}
	} else if (!frame->fmt.mdataplanes) {
		if (frame->fmt.memplanes >= 2)
			paddr->cb = vb2_dma_contig_plane_dma_addr(vb, 1);

		if (frame->fmt.memplanes == 3)
			paddr->cr = vb2_dma_contig_plane_dma_addr(vb, 2);
	}

	if ((vinc->vflip == 1) && (frame->fmt.fourcc == V4L2_PIX_FMT_FBC)) {
		paddr->y += CEIL_EXP(frame->o_width, 7) * (CEIL_EXP(frame->o_height, 5) - 1) *  96;
		paddr->cb += CEIL_EXP(frame->o_width, 4) * (CEIL_EXP(frame->o_height, 2) - 1) * 96;
		paddr->cr = 0;
	} else if (vinc->vflip == 1) {
		switch (frame->fmt.colplanes) {
		case 1:
			paddr->y += (pix_size - frame->o_width) * frame->fmt.depth[0] / 8;
			paddr->cb = 0;
			paddr->cr = 0;
			break;
		case 2:
			paddr->y += pix_size - frame->o_width;
			/* 420 */
			if (depth == 12)
				paddr->cb += pix_size / 2 - frame->o_width;
			else /* 422 */
				paddr->cb += pix_size - frame->o_width;
			paddr->cr = 0;
			break;
		case 3:
			paddr->y += pix_size - frame->o_width;
			if (depth == 12) {
				paddr->cb += pix_size / 4 - frame->o_width / 2;
				paddr->cr += pix_size / 4 - frame->o_width / 2;
			} else {
				paddr->cb += pix_size / 2 - frame->o_width / 2;
				paddr->cr += pix_size / 2 - frame->o_width / 2;
			}
			break;
		default:
			return -EINVAL;
		}
	}

	if ((vinc->large_image == 2) && (vinc->vin_status.frame_cnt % 2)) {
		if (frame->fmt.colplanes == 3) {
			if (depth == 12) {
				/* 420 */
				y_stride = frame->o_width / 2;
				u_stride = frame->o_width / 2 / 2;
				v_stride = frame->o_width / 2 / 2;
			} else {
				/* 422 */
				y_stride = frame->o_width / 2;
				u_stride = frame->o_width / 2;
				v_stride = frame->o_width / 2;
			}
		} else {
			y_stride = frame->o_width / 2;
			u_stride = frame->o_width / 2;
			v_stride = frame->o_width / 2;
		}

		csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_0_A, paddr->y + y_stride);
		csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_1_A, paddr->cb + u_stride);
		csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_2_A, paddr->cr + v_stride);
	} else {
		if (vinc->vid_cap.frame.fmt.fourcc == V4L2_PIX_FMT_YVU420) {
			csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_0_A, paddr->y);
			csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_2_A, paddr->cb);
			csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_1_A, paddr->cr);
		} else {
			csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_0_A, paddr->y);
			csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_1_A, paddr->cb);
			csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_2_A, paddr->cr);
		}
	}
	return 0;
}

/*
 * Videobuf operations
 */
static int queue_setup(struct vb2_queue *vq,
		       unsigned int *nbuffers, unsigned int *nplanes,
		       unsigned int sizes[], struct device *alloc_devs[])
{
	struct vin_vid_cap *cap = vb2_get_drv_priv(vq);
	unsigned int size;
	int buf_max_flag = 0;
	int i;

	cap->frame.bytesperline[0] = cap->frame.o_width * cap->frame.fmt.depth[0] / 8;
	cap->frame.bytesperline[1] = cap->frame.o_width * cap->frame.fmt.depth[1] / 8;
	cap->frame.bytesperline[2] = cap->frame.o_width * cap->frame.fmt.depth[2] / 8;

	size = cap->frame.o_width * cap->frame.o_height;
	if (cap->frame.fmt.fourcc == V4L2_PIX_FMT_FBC) {
		cap->frame.payload[0] = (CEIL_EXP(cap->frame.o_width, 7) * CEIL_EXP(cap->frame.o_height, 5) +
			CEIL_EXP(cap->frame.o_width, 4) * CEIL_EXP(cap->frame.o_height, 2)) * 96;
	} else {
		cap->frame.payload[0] = size * cap->frame.fmt.depth[0] / 8;
	}
	cap->frame.payload[1] = size * cap->frame.fmt.depth[1] / 8;
	cap->frame.payload[2] = size * cap->frame.fmt.depth[2] / 8;
	cap->buf_byte_size =
		PAGE_ALIGN(cap->frame.payload[0]) +
		PAGE_ALIGN(cap->frame.payload[1]) +
		PAGE_ALIGN(cap->frame.payload[2]);

	size = cap->buf_byte_size;

	if (size == 0)
		return -EINVAL;

	if (*nbuffers == 0)
		*nbuffers = 8;

	while (size * *nbuffers > MAX_FRAME_MEM) {
		(*nbuffers)--;
		buf_max_flag = 1;
		if (*nbuffers == 0)
			vin_err("Buffer size > max frame memory! count = %d\n",
			     *nbuffers);
	}

	if (buf_max_flag == 0) {
		if (cap->capture_mode == V4L2_MODE_IMAGE) {
			if (*nbuffers != 1) {
				*nbuffers = 1;
				vin_err("buffer count != 1 in capture mode\n");
			}
		} else {
			if (*nbuffers < 3) {
				*nbuffers = 3;
				vin_err("buffer count is invalid, set to 3\n");
			}
		}
	}

	*nplanes = cap->frame.fmt.memplanes;
	for (i = 0; i < *nplanes; i++) {
		sizes[i] = cap->frame.payload[i];
		alloc_devs[i] = cap->dev;
	}
	vin_log(VIN_LOG_VIDEO, "%s, buf count = %d, nplanes = %d, size = %d\n",
		__func__, *nbuffers, *nplanes, size);
	cap->vinc->vin_status.buf_cnt = *nbuffers;
	cap->vinc->vin_status.buf_size = size;
	return 0;
}

static int buffer_prepare(struct vb2_buffer *vb)
{
	struct vin_vid_cap *cap = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vvb = container_of(vb, struct vb2_v4l2_buffer, vb2_buf);
	struct vin_buffer *buf = container_of(vvb, struct vin_buffer, vb);
	int i;

	if (cap->frame.o_width < MIN_WIDTH || cap->frame.o_width > MAX_WIDTH ||
	    cap->frame.o_height < MIN_HEIGHT || cap->frame.o_height > MAX_HEIGHT) {
		return -EINVAL;
	}
	/*size = dev->buf_byte_size;*/

	for (i = 0; i < cap->frame.fmt.memplanes; i++) {
		if (vb2_plane_size(vb, i) < cap->frame.payload[i]) {
			vin_err("%s data will not fit into plane (%lu < %lu)\n",
				__func__, vb2_plane_size(vb, i),
				cap->frame.payload[i]);
			return -EINVAL;
		}
		vb2_set_plane_payload(&buf->vb.vb2_buf, i, cap->frame.payload[i]);
		vb->planes[i].m.offset = vb2_dma_contig_plane_dma_addr(vb, i);
	}

	return 0;
}

static void buffer_queue(struct vb2_buffer *vb)
{
	struct vin_vid_cap *cap = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vvb = container_of(vb, struct vb2_v4l2_buffer, vb2_buf);
	struct vin_buffer *buf = container_of(vvb, struct vin_buffer, vb);
	unsigned long flags = 0;

	spin_lock_irqsave(&cap->slock, flags);
	list_add_tail(&buf->list, &cap->vidq_active);
	spin_unlock_irqrestore(&cap->slock, flags);
}

static int start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct vin_vid_cap *cap = vb2_get_drv_priv(vq);

	set_bit(VIN_STREAM, &cap->state);
	return 0;
}

/* abort streaming and wait for last buffer */
static void stop_streaming(struct vb2_queue *vq)
{
	struct vin_vid_cap *cap = vb2_get_drv_priv(vq);
	unsigned long flags = 0;

	clear_bit(VIN_STREAM, &cap->state);

	spin_lock_irqsave(&cap->slock, flags);
	/* Release all active buffers */
	while (!list_empty(&cap->vidq_active)) {
		struct vin_buffer *buf;

		buf =
		    list_entry(cap->vidq_active.next, struct vin_buffer, list);
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
		vin_log(VIN_LOG_VIDEO, "buf %d stop\n", buf->vb.vb2_buf.index);
	}
	spin_unlock_irqrestore(&cap->slock, flags);
}

static const struct vb2_ops vin_video_qops = {
	.queue_setup = queue_setup,
	.buf_prepare = buffer_prepare,
	.buf_queue = buffer_queue,
	.start_streaming = start_streaming,
	.stop_streaming = stop_streaming,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
};

/*
 * IOCTL vidioc handling
 */
static int vidioc_querycap(struct file *file, void *priv,
			   struct v4l2_capability *cap)
{
	strcpy(cap->driver, "sunxi-vin");
	strcpy(cap->card, "sunxi-vin");

	cap->version = VIN_VERSION;
	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_STREAMING |
			V4L2_CAP_READWRITE | V4L2_CAP_DEVICE_CAPS;

	cap->device_caps |= V4L2_CAP_EXT_PIX_FORMAT;

	return 0;
}

static int vidioc_enum_fmt_vid_cap_mplane(struct file *file, void *priv,
					  struct v4l2_fmtdesc *f)
{
	struct vin_fmt *fmt;

	fmt = vin_find_format(NULL, NULL, VIN_FMT_ALL, f->index, false);
	if (!fmt)
		return -EINVAL;

	strlcpy(f->description, fmt->name, sizeof(f->description));
	f->pixelformat = fmt->fourcc;
	return 0;
}

static int vidioc_enum_framesizes(struct file *file, void *fh,
				  struct v4l2_frmsizeenum *fsize)
{
	struct vin_core *vinc = video_drvdata(file);
	struct v4l2_subdev_frame_size_enum fse;
	int ret;

	if (vinc == NULL)
		return -EINVAL;
	fse.index = fsize->index;

	ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], pad,
				enum_frame_size, NULL, &fse);
	if (ret < 0)
		return -1;
	fsize->stepwise.max_width = fse.max_width;
	fsize->stepwise.max_height = fse.max_height;
	fsize->stepwise.min_width = fse.min_width;
	fsize->stepwise.min_height = fse.min_height;
	return 0;
}

static int vidioc_g_fmt_vid_cap_mplane(struct file *file, void *priv,
				       struct v4l2_format *f)
{
	struct vin_vid_cap *cap =
	    container_of(video_devdata(file), struct vin_vid_cap, vdev);

	vin_get_fmt_mplane(&cap->frame, f);
	return 0;
}

static int vin_pipeline_try_format(struct vin_core *vinc,
				    struct v4l2_mbus_framefmt *tfmt,
				    struct vin_fmt **fmt_id,
				    bool set)
{
	struct v4l2_subdev *sd = vinc->vid_cap.pipe.sd[VIN_IND_SENSOR];
	struct v4l2_subdev_format sfmt;
	struct media_entity *me;
	struct vin_fmt *ffmt;
	unsigned int mask;
	struct media_entity_graph graph;
	int ret, i = 0, sd_ind;

	if (WARN_ON(!sd || !tfmt || !fmt_id))
		return -EINVAL;

	memset(&sfmt, 0, sizeof(sfmt));
	sfmt.format = *tfmt;
	sfmt.which = set ? V4L2_SUBDEV_FORMAT_ACTIVE : V4L2_SUBDEV_FORMAT_TRY;

	mask = (*fmt_id)->flags;
	if ((mask & VIN_FMT_YUV) && (vinc->support_raw == 0))
		mask = VIN_FMT_YUV;

	/* when diffrent video output have same sensor,
	 * this pipeline try fmt will lead to false result.
	 * so it should be updated at later.
	 */
	while (1) {

		ffmt = vin_find_format(NULL, sfmt.format.code != 0 ? &sfmt.format.code : NULL,
					mask, i++, true);
		if (ffmt == NULL) {
			/*
			 * Notify user-space if common pixel code for
			 * host and sensor does not exist.
			 */
			vin_err("vin is not support this pixelformat\n");
			return -EINVAL;
		}

		sfmt.format.code = tfmt->code = ffmt->mbus_code;
		me = &vinc->vid_cap.subdev.entity;
		if (media_entity_graph_walk_init(&graph, me->graph_obj.mdev) != 0)
			return -EINVAL;

		media_entity_graph_walk_start(&graph, me);
		while ((me = media_entity_graph_walk_next(&graph)) &&
			me != &vinc->vid_cap.subdev.entity) {

			sd = media_entity_to_v4l2_subdev(me);
			switch (sd->grp_id) {
			case VIN_GRP_ID_SENSOR:
				sd_ind = VIN_IND_SENSOR;
				break;
			case VIN_GRP_ID_MIPI:
				sd_ind = VIN_IND_MIPI;
				break;
			case VIN_GRP_ID_CSI:
				sd_ind = VIN_IND_CSI;
				break;
			case VIN_GRP_ID_ISP:
				sd_ind = VIN_IND_ISP;
				break;
			case VIN_GRP_ID_SCALER:
				sd_ind = VIN_IND_SCALER;
				break;
			case VIN_GRP_ID_CAPTURE:
				sd_ind = VIN_IND_CAPTURE;
				break;
			default:
				sd_ind = VIN_IND_SENSOR;
				break;
			}

			if (sd != vinc->vid_cap.pipe.sd[sd_ind])
				continue;
			vin_log(VIN_LOG_FMT, "found %s in this pipeline\n", me->name);

			if (me->num_pads == 1 &&
				(me->pads[0].flags & MEDIA_PAD_FL_SINK)) {
				vin_log(VIN_LOG_FMT, "skip %s.\n", me->name);
				continue;
			}

			sfmt.pad = 0;
			ret = v4l2_subdev_call(sd, pad, set_fmt, NULL, &sfmt);
			if (ret)
				return ret;

			/*set isp input win size for isp server call sensor_req_cfg*/
			if (sd->grp_id == VIN_GRP_ID_ISP)
				sensor_isp_input(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], &sfmt.format);

			/*change output resolution of scaler*/
			if (sd->grp_id == VIN_GRP_ID_SCALER) {
				sfmt.format.width = tfmt->width;
				sfmt.format.height = tfmt->height;
			}

			if (me->pads[0].flags & MEDIA_PAD_FL_SINK) {
				sfmt.pad = me->num_pads - 1;
				ret = v4l2_subdev_call(sd, pad, set_fmt, NULL, &sfmt);
				if (ret)
					return ret;
			}
		}

		if (sfmt.format.code != tfmt->code)
			continue;

		if (ffmt->mbus_code)
			sfmt.format.code = ffmt->mbus_code;

		break;
	}

	if (ffmt)
		*fmt_id = ffmt;
	*tfmt = sfmt.format;

	return 0;
}

static int vin_pipeline_set_mbus_config(struct vin_core *vinc)
{
	struct vin_pipeline *pipe = &vinc->vid_cap.pipe;
	struct v4l2_subdev *sd = pipe->sd[VIN_IND_SENSOR];
	struct v4l2_mbus_config mcfg;
	struct media_entity *me;
	struct media_entity_graph graph;
	struct csi_dev *csi = NULL;
	int ret;

	ret = v4l2_subdev_call(sd, video, g_mbus_config, &mcfg);
	if (ret < 0) {
		vin_err("%s g_mbus_config error!\n", sd->name);
		goto out;
	}
	/* s_mbus_config on all mipi and csi */
	me = &vinc->vid_cap.subdev.entity;
	if (media_entity_graph_walk_init(&graph, me->graph_obj.mdev) != 0)
			return -EINVAL;
	media_entity_graph_walk_start(&graph, me);
	while ((me = media_entity_graph_walk_next(&graph)) &&
		me != &vinc->vid_cap.subdev.entity) {
		sd = media_entity_to_v4l2_subdev(me);
		if ((sd == pipe->sd[VIN_IND_MIPI]) ||
		    (sd == pipe->sd[VIN_IND_CSI])) {
			ret = v4l2_subdev_call(sd, video, s_mbus_config, &mcfg);
			if (ret < 0) {
				vin_err("%s s_mbus_config error!\n", me->name);
				goto out;
			}
		}
	}

	csi = v4l2_get_subdevdata(pipe->sd[VIN_IND_CSI]);
	vinc->total_rx_ch = csi->bus_info.ch_total_num;
	vinc->vid_cap.frame.fmt.mbus_type = mcfg.type;
	if (vinc->ptn_cfg.ptn_en) {
		csi->bus_info.bus_if = V4L2_MBUS_PARALLEL;
		switch (vinc->ptn_cfg.ptn_dw) {
		case 0:
			csi->csi_fmt->data_width = 8;
			break;
		case 1:
			csi->csi_fmt->data_width = 10;
			break;
		case 2:
			csi->csi_fmt->data_width = 12;
			break;
		default:
			csi->csi_fmt->data_width = 12;
			break;
		}
	}

	return 0;
out:
	return ret;

}

static int vidioc_try_fmt_vid_cap_mplane(struct file *file, void *priv,
					 struct v4l2_format *f)
{
	struct vin_core *vinc = video_drvdata(file);
	struct v4l2_mbus_framefmt mf;
	struct vin_fmt *ffmt = NULL;

	ffmt = vin_find_format(&f->fmt.pix_mp.pixelformat, NULL,
		VIN_FMT_ALL, -1, false);
	if (ffmt == NULL) {
		vin_err("vin is not support this pixelformat\n");
		return -EINVAL;
	}

	mf.width = f->fmt.pix_mp.width;
	mf.height = f->fmt.pix_mp.height;
	mf.code = ffmt->mbus_code;
	vin_pipeline_try_format(vinc, &mf, &ffmt, true);

	f->fmt.pix_mp.width = mf.width;
	f->fmt.pix_mp.height = mf.height;
	f->fmt.pix_mp.colorspace = V4L2_COLORSPACE_JPEG;
	return 0;
}


static int __vin_set_fmt(struct vin_core *vinc, struct v4l2_format *f)
{
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct sensor_win_size win_cfg;
	struct v4l2_mbus_framefmt mf;
	struct vin_fmt *ffmt = NULL;
	struct mbus_framefmt_res *res = (void *)mf.reserved;
	int ret = 0;

	if (vin_streaming(cap)) {
		vin_err("%s device busy\n", __func__);
		return -EBUSY;
	}

	ffmt = vin_find_format(&f->fmt.pix_mp.pixelformat, NULL,
					VIN_FMT_ALL, -1, false);
	if (ffmt == NULL) {
		vin_err("vin does not support this pixelformat 0x%x\n",
				f->fmt.pix_mp.pixelformat);
		return -EINVAL;
	}
	cap->frame.fmt = *ffmt;
	mf.width = f->fmt.pix_mp.width;
	mf.height = f->fmt.pix_mp.height;
	mf.field = f->fmt.pix_mp.field;
	mf.code = ffmt->mbus_code;
	res->res_pix_fmt = f->fmt.pix_mp.pixelformat;
	ret = vin_pipeline_try_format(vinc, &mf, &ffmt, true);
	if (ret < 0) {
		vin_err("vin_pipeline_try_format failed\n");
		return -EINVAL;
	}
	cap->frame.fmt.mbus_code = mf.code;
	cap->frame.fmt.field = mf.field;

	vin_log(VIN_LOG_FMT, "vin_pipeline_try_format %d*%d %x %d\n",
		mf.width, mf.height, mf.code, mf.field);

	vin_pipeline_set_mbus_config(vinc);

	/*get current win configs*/
	memset(&win_cfg, 0, sizeof(struct sensor_win_size));
	ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core, ioctl,
			     GET_CURRENT_WIN_CFG, &win_cfg);
	if (ret == 0) {
		struct v4l2_subdev_pad_config cfg;
		struct v4l2_subdev_selection sel;

		vinc->vin_status.width = win_cfg.width;
		vinc->vin_status.height = win_cfg.height;
		vinc->vin_status.h_off = win_cfg.hoffset;
		vinc->vin_status.v_off = win_cfg.voffset;
		/*parser crop*/
		cfg.try_crop.width = win_cfg.width;
		cfg.try_crop.height = win_cfg.height;
		cfg.try_crop.left = win_cfg.hoffset;
		cfg.try_crop.top = win_cfg.voffset;

		ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_CSI], pad,
					set_selection, &cfg, NULL);
		if (ret < 0) {
			vin_err("csi parser set_selection error!\n");
			goto out;
		}

		/*vipp crop*/
		if ((win_cfg.vipp_hoff != 0) || (win_cfg.vipp_voff != 0)) {
			sel.target = V4L2_SEL_TGT_CROP;
			sel.pad = SCALER_PAD_SINK;
			sel.which = V4L2_SUBDEV_FORMAT_ACTIVE;
			sel.r.width = win_cfg.width_input - 2 * win_cfg.vipp_hoff;
			sel.r.height = win_cfg.height_input - 2 * win_cfg.vipp_voff;
			sel.r.left = win_cfg.vipp_hoff;
			sel.r.top = win_cfg.vipp_voff;
			ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SCALER],
					pad, set_selection, NULL, &sel);
			if (ret < 0) {
				vin_err("vipp set_selection error!\n");
				goto out;
			}
		}
	} else {
		ret = 0;
		vinc->vin_status.width = mf.width;
		vinc->vin_status.height = mf.height;
		vinc->vin_status.h_off = 0;
		vinc->vin_status.v_off = 0;
		vin_warn("get sensor win_cfg failed!\n");
	}

	if (vinc->vid_cap.frame.fmt.mbus_type == V4L2_MBUS_SUBLVDS ||
	    vinc->vid_cap.frame.fmt.mbus_type == V4L2_MBUS_HISPI) {
		struct combo_sync_code sync;
		struct combo_lane_map map;
		struct combo_wdr_cfg wdr;

		memset(&sync, 0, sizeof(sync));
		ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core,
				ioctl, GET_COMBO_SYNC_CODE, &sync);
		if (ret < 0) {
			vin_err("get combo sync code error!\n");
			goto out;
		}
		sunxi_combo_set_sync_code(cap->pipe.sd[VIN_IND_MIPI], &sync);

		memset(&map, 0, sizeof(map));
		ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core,
				ioctl, GET_COMBO_LANE_MAP, &map);
		if (ret < 0) {
			vin_err("get combo lane map error!\n");
			goto out;
		}
		sunxi_combo_set_lane_map(cap->pipe.sd[VIN_IND_MIPI], &map);

		if (res->res_combo_mode & 0xf) {
			memset(&wdr, 0, sizeof(wdr));
			ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core,
					ioctl, GET_COMBO_WDR_CFG, &wdr);
			if (ret < 0) {
				vin_err("get combo wdr cfg error!\n");
				goto out;
			}
			sunxi_combo_wdr_config(cap->pipe.sd[VIN_IND_MIPI], &wdr);
		}
	}
	cap->isp_wdr_mode = res->res_wdr_mode;

	if (cap->capture_mode == V4L2_MODE_IMAGE) {
		sunxi_flash_check_to_start(cap->pipe.sd[VIN_IND_FLASH],
					   SW_CTRL_FLASH_ON);
	} else {
		sunxi_flash_stop(vinc->vid_cap.pipe.sd[VIN_IND_FLASH]);
	}

	if ((mf.width < f->fmt.pix_mp.width) || (mf.height < f->fmt.pix_mp.height)) {
		f->fmt.pix_mp.width = mf.width;
		f->fmt.pix_mp.height = mf.height;
	}
	/*for csi dma size set*/
	cap->frame.offs_h = (mf.width - f->fmt.pix_mp.width) / 2;
	cap->frame.offs_v = (mf.height - f->fmt.pix_mp.height) / 2;
	cap->frame.o_width = f->fmt.pix_mp.width;
	cap->frame.o_height = f->fmt.pix_mp.height;

out:
	return ret;
}


static int vidioc_s_fmt_vid_cap_mplane(struct file *file, void *priv,
				struct v4l2_format *f)
{
	struct vin_core *vinc = video_drvdata(file);

	return __vin_set_fmt(vinc, f);
}

int vidioc_g_crop(struct file *file, void *fh,
				struct v4l2_crop *a)
{
	struct vin_core *vinc = video_drvdata(file);

	a->c.left = vinc->vid_cap.frame.offs_h;
	a->c.top = vinc->vid_cap.frame.offs_v;
	a->c.width = vinc->vid_cap.frame.o_width;
	a->c.height = vinc->vid_cap.frame.o_height;

	return 0;
}

int vidioc_s_crop(struct file *file, void *fh,
				const struct v4l2_crop *a)
{
	struct vin_core *vinc = video_drvdata(file);

	vinc->vid_cap.frame.offs_h = a->c.left;
	vinc->vid_cap.frame.offs_v = a->c.top;
	vinc->vid_cap.frame.o_width = a->c.width;
	vinc->vid_cap.frame.o_height = a->c.height;

	return 0;
}

int vidioc_g_selection(struct file *file, void *fh,
				struct v4l2_selection *s)
{
	struct vin_core *vinc = video_drvdata(file);
	struct v4l2_subdev_selection sel;
	int ret = 0;

	sel.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	sel.pad = SCALER_PAD_SINK;
	sel.target = s->target;
	sel.flags = s->flags;
	sel.r = s->r;
	ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SCALER], pad,
				set_selection, NULL, &sel);
	if (ret < 0)
		vin_err("v4l2 sub device scaler set_selection error!\n");
	return ret;
}
int vidioc_s_selection(struct file *file, void *fh,
				struct v4l2_selection *s)
{
	struct vin_core *vinc = video_drvdata(file);
	struct v4l2_subdev_selection sel;
	int ret = 0;

	sel.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	sel.pad = SCALER_PAD_SINK;
	sel.target = s->target;
	sel.flags = s->flags;
	ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SCALER], pad,
				get_selection, NULL, &sel);
	if (ret < 0)
		vin_err("v4l2 sub device scaler get_selection error!\n");
	else
		s->r = sel.r;
	return ret;

}

static int vidioc_enum_fmt_vid_overlay(struct file *file, void *__fh,
					    struct v4l2_fmtdesc *f)
{
	struct vin_fmt *fmt;

	fmt = vin_find_format(NULL, NULL, VIN_FMT_OSD, f->index, false);
	if (!fmt)
		return -EINVAL;

	strlcpy(f->description, fmt->name, sizeof(f->description));
	f->pixelformat = fmt->fourcc;
	return 0;
}

static int vidioc_g_fmt_vid_overlay(struct file *file, void *__fh,
					struct v4l2_format *f)
{
	struct vin_core *vinc = video_drvdata(file);

	f->fmt.win.w.left = 0;
	f->fmt.win.w.top = 0;
	f->fmt.win.w.width = vinc->vid_cap.frame.o_width;
	f->fmt.win.w.height = vinc->vid_cap.frame.o_height;
	f->fmt.win.clipcount = vinc->vid_cap.osd.overlay_cnt;
	f->fmt.win.chromakey = vinc->vid_cap.osd.chromakey;

	return 0;
}
static void __osd_win_check(struct v4l2_window *win)
{
	if (win->w.width > MAX_WIDTH)
		win->w.width = MAX_WIDTH;
	if (win->w.width < MIN_WIDTH)
		win->w.width = MIN_WIDTH;
	if (win->w.height > MAX_HEIGHT)
		win->w.height = MAX_HEIGHT;
	if (win->w.height < MIN_HEIGHT)
		win->w.height = MIN_HEIGHT;

	if (win->bitmap) {
		if (win->clipcount > MAX_OVERLAY_NUM)
			win->clipcount = MAX_OVERLAY_NUM;
	} else {
		if (win->clipcount > MAX_COVER_NUM)
			win->clipcount = MAX_COVER_NUM;
	}
}

static int vidioc_try_fmt_vid_overlay(struct file *file, void *__fh,
					struct v4l2_format *f)
{
	__osd_win_check(&f->fmt.win);

	return 0;
}

void __osd_bitmap2dram(struct vin_osd *osd, void *databuf)
{
	int i, j, k, m = 0, n = 0;
	int kend = 0, idx = 0, ww = 0, ysn = 0;
	int y_num = 0, *y_temp = NULL;
	int *hor_num = NULL, *hor_index = NULL;
	int *x_temp = NULL, *xbuf = NULL, *x_idx = NULL;
	int addr_offset = 0, pix_size = osd->fmt->depth[0]/8;
	int cnt = osd->overlay_cnt;
	void *dram_addr = osd->ov_mask[osd->ov_set_cnt % 2].vir_addr;

	y_temp = (int *)kzalloc(2 * cnt * sizeof(int), GFP_KERNEL);
	for (i = 0; i < cnt; i++) {
		y_temp[i] = osd->ov_win[i].top;
		y_temp[i + cnt] = osd->ov_win[i].top + osd->ov_win[i].height - 1;
	}
	sort(y_temp, 2 * cnt, sizeof(int), vin_cmp, vin_swap);
	y_num = vin_unique(y_temp, 2 * cnt);
	hor_num = (int *)kzalloc(y_num * sizeof(int), GFP_KERNEL); /*0~y_num-1*/
	hor_index = (int *)kzalloc(y_num * cnt * sizeof(int), GFP_KERNEL); /*(0~y_num-1) * (0~N+1)*/

	for (j = 0; j < y_num; j++) {
		ysn = 0;
		for (i = 0; i < cnt; i++) {
			if (osd->ov_win[i].top <= y_temp[j] &&
			   (osd->ov_win[i].top + osd->ov_win[i].height) > y_temp[j]) {
				hor_num[j]++;
				hor_index[j * cnt + ysn] = i;
				ysn = ysn + 1;
			}
		}
	}

	for (j = 0; j < y_num; j++) {
		x_temp = (int *)kzalloc(hor_num[j] * sizeof(int), GFP_KERNEL);
		xbuf = (int *)kzalloc(hor_num[j] * sizeof(int), GFP_KERNEL);
		x_idx = (int *)kzalloc(hor_num[j] * sizeof(int), GFP_KERNEL);
		for (k = 0; k < hor_num[j]; k++)
			x_temp[k] = osd->ov_win[hor_index[j * cnt + k]].left;
		memcpy(xbuf, x_temp, hor_num[j] * sizeof(int));
		sort(x_temp, hor_num[j], sizeof(int), vin_cmp, vin_swap);

		for (k = 0; k < hor_num[j]; k++)	{
			for (m = 0; m < hor_num[j]; m++) {
				if (x_temp[k] == xbuf[m]) {
					x_idx[k] = m;
					break;
				}
			}
		}

		if (j == y_num - 1)
			kend = y_temp[j];
		else
			kend = y_temp[j + 1] - 1;
		for (k = y_temp[j]; k <= kend; k++) {
			for (i = 0; i < hor_num[j]; i++)	{
				idx = hor_index[j * cnt + x_idx[i]];
				addr_offset = 0;
				for (n = 0; n < idx; n++)
					addr_offset +=	(osd->ov_win[n].width * osd->ov_win[n].height) * pix_size;
				ww = osd->ov_win[idx].width;
				if (k < (osd->ov_win[idx].top + osd->ov_win[idx].height)) {
					memcpy(dram_addr, databuf + addr_offset
						+ ww * (k - osd->ov_win[idx].top) * pix_size,
						ww * pix_size);
					dram_addr += ww * pix_size;
				}
			}
		}
		kfree(x_temp);
		kfree(xbuf);
		kfree(x_idx);
		x_temp = NULL;
		xbuf = NULL;
		x_idx = NULL;
	}
	kfree(hor_num);
	kfree(hor_index);
	kfree(y_temp);
	y_temp = NULL;
	hor_index = NULL;
	hor_num = NULL;
}

static void __osd_rgb_to_yuv(u8 r, u8 g, u8 b, u8 *y, u8 *u, u8 *v)
{
	int jc0	= 0x00000132;
	int jc1 = 0x00000259;
	int jc2 = 0x00000075;
	int jc3 = 0xffffff53;
	int jc4 = 0xfffffead;
	int jc5 = 0x00000200;
	int jc6 = 0x00000200;
	int jc7 = 0xfffffe53;
	int jc8 = 0xffffffad;
	int jc9 = 0x00000000;
	int jc10 = 0x00000080;
	int jc11 = 0x00000080;
	int y_tmp, u_tmp, v_tmp;

	y_tmp = (((jc0 * r >> 6) + (jc1 * g >> 6) + (jc2 * b >> 6)) >> 4) + jc9;
	*y = CLIP(y_tmp, 0, 255);

	u_tmp = (((jc3 * r >> 6) + (jc4 * g >> 6) + (jc5 * b >> 6)) >> 4) + jc10;
	*u = CLIP(u_tmp, 0, 255);

	v_tmp = (((jc6 * r >> 6) + (jc7 * g >> 6) + (jc8 * b >> 6)) >> 4) + jc11;
	*v = CLIP(v_tmp, 0, 255);
}

static void __osd_bmp_to_yuv(struct vin_osd *osd, void *databuf)
{

	u8 alpha, r, g, b, y, u, v;
	int i, j, y_sum, bmp;

	for (i = 0; i < osd->overlay_cnt; i++) {
		int bmp_size = osd->ov_win[i].height * osd->ov_win[i].width;
		int valid_pix = 1;

		y_sum = 0;
		for (j = 0; j < bmp_size; j++) {
			switch (osd->overlay_fmt) {
			case 0:
				bmp = *(short *)databuf;
				alpha = (int)((bmp >> 15) & 0x01) * 100;
				r = (bmp >> 10) & 0x1f;
				r = (r << 3) + (r >> 2);
				g = (bmp >> 5) & 0x1f;
				g = (g << 3) + (g >> 2);
				b = bmp & 0x1f;
				b = (b << 3) + (b >> 2);
				databuf += 2;
				break;
			case 1:
				bmp = *(short *)databuf;
				alpha = (int)((bmp >> 12) & 0x0f) * 100 / 15;
				r = (bmp >> 8) & 0x0f;
				r = (r << 4) + r;
				g = (bmp >> 4) & 0x0f;
				g = (g << 4) + g;
				b = bmp & 0x0f;
				b = (b << 4) + b;
				databuf += 2;
				break;
			case 2:
				bmp = *(int *)databuf;
				alpha = (int)((bmp >> 24) & 0xff) * 100 / 255;
				r = (bmp >> 16) & 0xff;
				g = (bmp >> 8) & 0xff;
				b = bmp & 0xff;
				databuf += 4;
				break;
			default:
				bmp = *(int *)databuf;
				alpha = (int)((bmp >> 24) & 0xff) * 100 / 255;
				r = (bmp >> 16) & 0xff;
				g = (bmp >> 8) & 0xff;
				b = bmp & 0xff;
				databuf += 4;
				break;
			}
			if (alpha >= 80) {
				__osd_rgb_to_yuv(r, g, b, &y, &u, &v);
				y_sum += y;
				valid_pix++;
			}
		}
		osd->y_bmp_avp[i] = y_sum / valid_pix;
	}
}

static int vidioc_s_fmt_vid_overlay(struct file *file, void *__fh,
					struct v4l2_format *f)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_osd *osd = &vinc->vid_cap.osd;
	struct v4l2_clip *clip = NULL;
	void *bitmap = NULL;
	unsigned int bitmap_size = 0, pix_size = 0;
	int ret = 0, i = 0;

	__osd_win_check(&f->fmt.win);

	osd->chromakey = f->fmt.win.chromakey;

	if (f->fmt.win.bitmap) {
		if (f->fmt.win.clipcount <= 0) {
			osd->overlay_en = 0;
			goto osd_reset;
		} else {
			osd->overlay_en = 1;
			osd->overlay_cnt = f->fmt.win.clipcount;
		}

		clip = vmalloc(sizeof(struct v4l2_clip) * osd->overlay_cnt * 2);
		if (clip == NULL) {
			vin_err("%s - Alloc of clip mask failed\n", __func__);
			return -ENOMEM;
		}
		if (copy_from_user(clip, f->fmt.win.clips,
			sizeof(struct v4l2_clip) * osd->overlay_cnt * 2)) {
			vfree(clip);
			return -EFAULT;
		}

		/*save global alpha in the win top for diff overlay*/
		for (i = 0; i < osd->overlay_cnt; i++) {
			osd->ov_win[i] = clip[i].c;
			bitmap_size += clip[i].c.width * clip[i].c.height;
			if (f->fmt.win.global_alpha == 255)
				osd->global_alpha[i] = clamp_val(clip[i + osd->overlay_cnt].c.top, 0, 16);
			else
				osd->global_alpha[i] = clamp_val(f->fmt.win.global_alpha, 0, 16);
			osd->inverse_close[i] = clip[i + osd->overlay_cnt].c.left;
		}
		vfree(clip);

		osd->fmt = vin_find_format(&f->fmt.win.chromakey, NULL,
				VIN_FMT_OSD, -1, false);
		if (osd->fmt == NULL) {
			vin_err("osd is not support this chromakey\n");
			return -EINVAL;
		}
		pix_size = osd->fmt->depth[0]/8;

		bitmap = vmalloc(bitmap_size * pix_size);
		if (bitmap == NULL) {
			vin_err("%s - Alloc of bitmap buf failed\n", __func__);
			return -ENOMEM;
		}
		if (copy_from_user(bitmap, f->fmt.win.bitmap,
				bitmap_size * pix_size)) {
			vfree(bitmap);
			return -EFAULT;
		}

		osd->ov_set_cnt++;

		if (osd->ov_mask[osd->ov_set_cnt % 2].size != bitmap_size * pix_size) {
			if (osd->ov_mask[osd->ov_set_cnt % 2].phy_addr) {
				os_mem_free(&vinc->pdev->dev, &osd->ov_mask[osd->ov_set_cnt % 2]);
				osd->ov_mask[osd->ov_set_cnt % 2].phy_addr = NULL;
			}
			osd->ov_mask[osd->ov_set_cnt % 2].size = bitmap_size * pix_size;
			ret = os_mem_alloc(&vinc->pdev->dev, &osd->ov_mask[osd->ov_set_cnt % 2]);
			if (ret < 0) {
				vin_err("osd bitmap load addr requset failed!\n");
				vfree(bitmap);
				return -ENOMEM;
			}
		}
		memset(osd->ov_mask[osd->ov_set_cnt % 2].vir_addr, 0, bitmap_size * pix_size);
		__osd_bitmap2dram(osd, bitmap);

		switch (osd->chromakey) {
		case V4L2_PIX_FMT_RGB555:
			osd->overlay_fmt = ARGB1555;
			break;
		case V4L2_PIX_FMT_RGB444:
			osd->overlay_fmt = ARGB4444;
			break;
		case V4L2_PIX_FMT_RGB32:
			osd->overlay_fmt = ARGB8888;
			break;
		default:
			osd->overlay_fmt = ARGB8888;
			break;
		}
		__osd_bmp_to_yuv(osd, bitmap);
		vfree(bitmap);
	} else {
		if (f->fmt.win.clipcount <= 0) {
			osd->cover_en = 0;
			goto osd_reset;
		} else {
			osd->cover_en = 1;
			osd->cover_cnt = f->fmt.win.clipcount;
		}

		clip = vmalloc(sizeof(struct v4l2_clip) * osd->cover_cnt * 2);
		if (clip == NULL) {
			vin_err("%s - Alloc of clip mask failed\n", __func__);
			return -ENOMEM;
		}
		if (copy_from_user(clip, f->fmt.win.clips,
			sizeof(struct v4l2_clip) * osd->cover_cnt * 2)) {
			vfree(clip);
			return -EFAULT;
		}

		/*save rgb in the win top for diff cover*/
		for (i = 0; i < osd->cover_cnt; i++) {
			osd->cv_win[i] = clip[i].c;
			osd->rgb_cover[i] = clip[i + osd->cover_cnt].c.top;
		}
		vfree(clip);

		for (i = 0; i < osd->cover_cnt; i++) {
			u8 r, g, b;

			r = (osd->rgb_cover[i] >> 16) & 0xff;
			g = (osd->rgb_cover[i] >> 8) & 0xff;
			b = osd->rgb_cover[i] & 0xff;
			__osd_rgb_to_yuv(r, g, b, &osd->yuv_cover[0][i],
				&osd->yuv_cover[1][i], &osd->yuv_cover[2][i]);
		}
	}
osd_reset:
	osd->is_set = 0;

	return ret;
}

static int __osd_reg_setup(int id, struct vin_osd *osd)
{
	struct vipp_osd_config *osd_cfg = NULL;
	struct vipp_osd_para_config *para = NULL;
	struct vipp_rgb2yuv_factor rgb2yuv_def = {
		.jc0 = 0x00000132,
		.jc1 = 0x00000259,
		.jc2 = 0x00000075,
		.jc3 = 0xffffff53,
		.jc4 = 0xfffffead,
		.jc5 = 0x00000200,
		.jc6 = 0x00000200,
		.jc7 = 0xfffffe53,
		.jc8 = 0xffffffad,
		.jc9 = 0x00000000,
		.jc10 = 0x00000080,
		.jc11 = 0x00000080,
	};
	int i;

	osd_cfg = kzalloc(sizeof(struct vipp_osd_config), GFP_KERNEL);
	if (osd_cfg == NULL) {
		vin_err("%s - Alloc of osd_cfg failed\n", __func__);
		return -ENOMEM;
	}

	para = kzalloc(sizeof(struct vipp_osd_para_config), GFP_KERNEL);
	if (para == NULL) {
		vin_err("%s - Alloc of osd_para failed\n", __func__);
		kfree(osd_cfg);
		return -ENOMEM;
	}

	if (osd->overlay_en == 1) {
		osd_cfg->osd_argb_mode = osd->overlay_fmt;
		osd_cfg->osd_ov_num = osd->overlay_cnt - 1;
		osd_cfg->osd_ov_en = 1;
		osd_cfg->osd_stat_en = 1;
		for (i = 0; i < osd->overlay_cnt; i++) {
			para->overlay_cfg[i].h_start = osd->ov_win[i].left;
			para->overlay_cfg[i].h_end = osd->ov_win[i].left
				+ osd->ov_win[i].width - 1;
			para->overlay_cfg[i].v_start = osd->ov_win[i].top;
			para->overlay_cfg[i].v_end = osd->ov_win[i].top
				+ osd->ov_win[i].height - 1;
			para->overlay_cfg[i].alpha = osd->global_alpha[i];
		}
		vipp_set_osd_bm_load_addr(id, (unsigned long)osd->ov_mask[osd->ov_set_cnt % 2].dma_addr);
	}

	if (osd->cover_en == 1) {
		osd_cfg->osd_cv_num = osd->cover_cnt - 1;
		osd_cfg->osd_cv_en = 1;
		for (i = 0; i < osd->cover_cnt; i++) {
			para->cover_cfg[i].h_start = osd->cv_win[i].left;
			para->cover_cfg[i].h_end = osd->cv_win[i].left
				+ osd->cv_win[i].width - 1;
			para->cover_cfg[i].v_start = osd->cv_win[i].top;
			para->cover_cfg[i].v_end = osd->cv_win[i].top
				+ osd->cv_win[i].height - 1;
			para->cover_data[i].y = osd->yuv_cover[0][i];
			para->cover_data[i].u = osd->yuv_cover[1][i];
			para->cover_data[i].v = osd->yuv_cover[2][i];
		}
	}

	vipp_osd_cfg(id, osd_cfg);
	vipp_osd_rgb2yuv(id, &rgb2yuv_def);
	vipp_osd_para_cfg(id, para, osd_cfg);
	kfree(osd_cfg);
	kfree(para);
	return 0;
}

static int vidioc_overlay(struct file *file, void *__fh, unsigned int on)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_osd *osd = &vinc->vid_cap.osd;
	int i;
	int ret = 0;

	if (!on) {
		for (i = 0; i < 2; i++) {
			if (osd->ov_mask[i].phy_addr) {
				os_mem_free(&vinc->pdev->dev, &osd->ov_mask[i]);
				osd->ov_mask[i].phy_addr = NULL;
				osd->ov_mask[i].size = 0;
			}
		}
		osd->ov_set_cnt = 0;
		osd->overlay_en = 0;
		osd->cover_en = 0;
		vipp_chroma_ds_en(vinc->vipp_sel, 0);
		vipp_osd_en(vinc->vipp_sel, 0);
		switch (vinc->vid_cap.frame.fmt.fourcc) {
		case V4L2_PIX_FMT_YUV420:
		case V4L2_PIX_FMT_YUV420M:
		case V4L2_PIX_FMT_YVU420:
		case V4L2_PIX_FMT_YVU420M:
		case V4L2_PIX_FMT_NV21:
		case V4L2_PIX_FMT_NV21M:
		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV12M:
		case V4L2_PIX_FMT_FBC:
			sunxi_osd_change_sc_fmt(vinc->vipp_sel, YUV420, on);
			break;
		default:
			break;
		}
		goto para_ready;
	} else {
		if (osd->is_set)
			return ret;
		ret = __osd_reg_setup(vinc->vipp_sel, osd);
		/*when osd enable scaler down outfmt must be YUV422*/
		sunxi_osd_change_sc_fmt(vinc->vipp_sel, YUV422, on);
		vipp_osd_en(vinc->vipp_sel, 1);
		switch (vinc->vid_cap.frame.fmt.fourcc) {
		case V4L2_PIX_FMT_YUV420:
		case V4L2_PIX_FMT_YUV420M:
		case V4L2_PIX_FMT_YVU420:
		case V4L2_PIX_FMT_YVU420M:
		case V4L2_PIX_FMT_NV21:
		case V4L2_PIX_FMT_NV21M:
		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV12M:
		case V4L2_PIX_FMT_FBC:
			vipp_chroma_ds_en(vinc->vipp_sel, 1);
			break;
		default:
			break;
		}
	}

para_ready:
	osd->is_set = 1;
	return ret;
}

void vin_pipeline_reset(unsigned long data)
{
	struct vin_core *vinc = (struct vin_core *)data;

	schedule_work(&vinc->vid_cap.pipeline_reset_task);
}

int vin_timer_init(struct vin_core *vinc)
{
	INIT_WORK(&vinc->vid_cap.pipeline_reset_task, __vin_pipeline_reset_handle);
	init_timer(&vinc->timer_for_reset);
	vinc->timer_for_reset.data = (unsigned long)vinc;
	vinc->timer_for_reset.expires = jiffies + 3*HZ;
	vinc->timer_for_reset.function = vin_pipeline_reset;
	add_timer(&vinc->timer_for_reset);
	return 0;
}

static int vidioc_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct vin_buffer *buf;
	int ret = 0;

	if (i != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		ret = -EINVAL;
		goto streamon_error;
	}

	if (vin_streaming(cap)) {
		vin_err("stream has been already on\n");
		ret = -1;
		goto streamon_error;
	}
#if USE_BIG_PIPELINE
	/*
	 * when media_entity_pipeline_start is called, the diffrent video
	 * output which have same input maybe in the same pipeline. this
	 * situation is not good for our design. furthermore in our design
	 * the same subdev would in diffrent pipeline, which is not allow
	 * in media_entity_pipeline_start, so we give up it.
	 */
	 /*the big pipeline would use it*/
	ret = media_entity_pipeline_start(&cap->vdev.entity, &vinc->vid_cap.pipe.pipe);
	if (ret < 0)
		goto streamon_error;
#endif
	ret = vb2_ioctl_streamon(file, priv, i);
	if (ret)
		goto streamon_error;

	vinc->vid_cap.first_flag = 0;
	vinc->vin_status.frame_cnt = 0;
	vinc->vin_status.err_cnt = 0;
	vinc->vin_status.lost_cnt = 0;

	if (vinc->large_image == 1) {
		vinc->ptn_cfg.ptn_w = cap->frame.o_width;
		vinc->ptn_cfg.ptn_h = cap->frame.o_height;
		vinc->ptn_cfg.ptn_mode = 12;
		vinc->ptn_cfg.ptn_buf.size = cap->buf_byte_size;
		os_mem_alloc(&vinc->pdev->dev, &vinc->ptn_cfg.ptn_buf);
		if (vinc->ptn_cfg.ptn_buf.vir_addr == NULL) {
			vin_err("ptn buffer 0x%x alloc failed!\n", cap->buf_byte_size);
			return -ENOMEM;
		}
		csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_0_A, (unsigned long)vinc->ptn_cfg.ptn_buf.dma_addr);
	} else {
		buf = list_entry(cap->vidq_active.next, struct vin_buffer, list);
		vin_set_addr(vinc, &buf->vb.vb2_buf, &vinc->vid_cap.frame, &vinc->vid_cap.frame.paddr);
	}

	schedule_work(&vinc->vid_cap.s_stream_task);
	vin_timer_init(vinc);

streamon_error:

	return ret;
}

static int vidioc_streamoff(struct file *file, void *priv, enum v4l2_buf_type i)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	int ret = 0;

	if (!vin_streaming(cap)) {
		vin_err("video%d has been already streaming off\n", vinc->id);
		ret = 0;
		goto streamoff_error;
	}

	vin_pipeline_call(vinc, set_stream, &cap->pipe, 0);
	del_timer(&vinc->timer_for_reset);
	flush_work(&vinc->vid_cap.pipeline_reset_task);

	if (i != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		ret = -EINVAL;
		goto streamoff_error;
	}

	ret = vb2_ioctl_streamoff(file, priv, i);
	if (ret != 0) {
		vin_err("video%d stream off error!\n", vinc->id);
		goto streamoff_error;
	}
#if USE_BIG_PIPELINE
	media_entity_pipeline_stop(&cap->vdev.entity);
#endif
streamoff_error:

	return ret;
}

static int __vin_sensor_setup_link(struct modules_config *module,
					int i, int en)
{
	struct v4l2_subdev *sensor = module->modules.sensor[i].sd;
	struct media_entity *entity = NULL;
	struct media_link *link = NULL;
	int ret;

	if (sensor == NULL)
		return -1;

	entity = &sensor->entity;
	list_for_each_entry(link, &entity->links, list) {
		if (link->source->entity == entity)
			break;
	}
	if (link == NULL)
		return -1;

	if (sensor->entity.use_count >= 1)
		return 0;

	vin_log(VIN_LOG_VIDEO, "setup link: [%s] %c> [%s]\n",
		sensor->name, en ? '=' : '-', link->sink->entity->name);
	if (en)
		ret = media_entity_setup_link(link, MEDIA_LNK_FL_ENABLED);
	else
		ret = media_entity_setup_link(link, 0);

	if (ret) {
		vin_warn("%s setup link %s fail!\n", sensor->name,
				link->sink->entity->name);
		return -1;
	}
	return 0;
}

static int vidioc_enum_input(struct file *file, void *priv,
			     struct v4l2_input *inp)
{
	if (inp->index != 0)
		return -EINVAL;

	inp->type = V4L2_INPUT_TYPE_CAMERA;
	inp->std = V4L2_STD_UNKNOWN;
	strcpy(inp->name, "sunxi-vin");

	return 0;
}

static int vidioc_g_input(struct file *file, void *priv, unsigned int *i)
{
	struct vin_core *vinc = video_drvdata(file);

	if (vinc->sensor_sel == vinc->rear_sensor)
		*i = 0;
	else
		*i = 1;

	return 0;
}

static int __vin_actuator_set_power(struct v4l2_subdev *sd, int on)
{
	int *use_count;
	int ret;

	if (sd == NULL)
		return -ENXIO;

	use_count = &sd->entity.use_count;
	if (on && (*use_count)++ > 0)
		return 0;
	else if (!on && (*use_count == 0 || --(*use_count) > 0))
		return 0;
	ret = v4l2_subdev_call(sd, core, ioctl, ACT_SOFT_PWDN, 0);

	return ret != -ENOIOCTLCMD ? ret : 0;
}

static int vidioc_s_input(struct file *file, void *priv, unsigned int i)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_md *vind = dev_get_drvdata(vinc->v4l2_dev->dev);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct modules_config *module = NULL;
	struct sensor_instance *inst = NULL;
	struct sensor_info *info = NULL;
	struct mipi_dev *mipi = NULL;
	int valid_idx = -1;
	int ret;

	i = i > 1 ? 0 : i;

	if (i == 0)
		vinc->sensor_sel = vinc->rear_sensor;
	else
		vinc->sensor_sel = vinc->front_sensor;

	module = &vind->modules[vinc->sensor_sel];
	valid_idx = module->sensors.valid_idx;

	if (valid_idx == NO_VALID_SENSOR) {
		vin_err("there is no valid sensor\n");
		return -EINVAL;
	}

	if (__vin_sensor_setup_link(module, valid_idx, 1) < 0) {
		vin_err("sensor setup link failed\n");
		return -EINVAL;
	}
	inst = &module->sensors.inst[valid_idx];

	sunxi_isp_sensor_type(cap->pipe.sd[VIN_IND_ISP], inst->is_isp_used);
	vinc->support_raw = inst->is_isp_used;

	ret = vin_pipeline_call(vinc, open, &cap->pipe, &cap->vdev.entity, true);
	if (ret < 0) {
		vin_err("vin pipeline open failed (%d)!\n", ret);
		return ret;
	}

	if (module->modules.act[valid_idx].sd != NULL) {
		cap->pipe.sd[VIN_IND_ACTUATOR] = module->modules.act[valid_idx].sd;
		ret = __vin_actuator_set_power(cap->pipe.sd[VIN_IND_ACTUATOR], 1);
		if (ret < 0) {
			vin_err("actutor power off failed (%d)!\n", ret);
			return ret;
		}
	}

	ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_ISP], core, init, 1);
	if (ret < 0) {
		vin_err("ISP init error at %s\n", __func__);
		return ret;
	}

	ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SCALER], core, init, 1);
	if (ret < 0) {
		vin_err("SCALER init error at %s\n", __func__);
		return ret;
	}

	/*save exp and gain for reopen, sensor init may reset gain to 0, so save before init!*/
	info = container_of(cap->pipe.sd[VIN_IND_SENSOR], struct sensor_info, sd);
	if (info) {
		vinc->exp_gain.exp_val = info->exp;
		vinc->exp_gain.gain_val = info->gain;
		vinc->stream_idx = info->stream_seq + 1;
	}

	if (cap->pipe.sd[VIN_IND_MIPI] != NULL) {
		mipi = container_of(cap->pipe.sd[VIN_IND_MIPI], struct mipi_dev, subdev);
		if (mipi)
			mipi->sensor_flags = vinc->sensor_sel;
	}

	if (!vinc->ptn_cfg.ptn_en) {
		ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core, init, 1);
		if (ret) {
			vin_err("sensor initial error when selecting target device!\n");
			return ret;
		}
	}

	/*setup the current ctrl value*/
	v4l2_ctrl_handler_setup(&vinc->vid_cap.ctrl_handler);

	vinc->hflip = inst->hflip;
	vinc->vflip = inst->vflip;

	return ret;
}

static const char *const sensor_info_type[] = {
	"YUV",
	"RAW",
	NULL,
};

static int vidioc_g_parm(struct file *file, void *priv,
			 struct v4l2_streamparm *parms)
{
	struct vin_core *vinc = video_drvdata(file);
	int ret;

	ret =
	    v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], video,
			     g_parm, parms);
	if (ret < 0)
		vin_warn("v4l2 sub device g_parm fail!\n");

	return ret;
}

static int vidioc_s_parm(struct file *file, void *priv,
			 struct v4l2_streamparm *parms)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct sensor_instance *inst = get_valid_sensor(vinc);
	int ret = 0;

	if (parms->parm.capture.capturemode != V4L2_MODE_VIDEO &&
	    parms->parm.capture.capturemode != V4L2_MODE_IMAGE &&
	    parms->parm.capture.capturemode != V4L2_MODE_PREVIEW) {
		parms->parm.capture.capturemode = V4L2_MODE_PREVIEW;
	}

	cap->capture_mode = parms->parm.capture.capturemode;
	vinc->large_image = parms->parm.capture.reserved[2];

	ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], video, s_parm,
				parms);
	if (ret < 0)
		vin_warn("v4l2 subdev sensor s_parm error!\n");

	ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_CSI], video, s_parm,
				parms);
	if (ret < 0)
		vin_warn("v4l2 subdev csi s_parm error!\n");

	if (inst->is_isp_used) {
		ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_ISP], video, s_parm,
					parms);
		if (ret < 0)
			vin_warn("v4l2 subdev isp s_parm error!\n");
	}

	return ret;
}

static int isp_exif_req(struct file *file, struct v4l2_fh *fh,
			struct isp_exif_attribute *exif_attr)
{
	struct vin_core *vinc = video_drvdata(file);
	struct sensor_instance *inst = get_valid_sensor(vinc);
	struct v4l2_subdev *sd = vinc->vid_cap.pipe.sd[VIN_IND_SENSOR];
	struct sensor_exif_attribute exif;

	if (inst->is_bayer_raw)
		return 0;
	if (v4l2_subdev_call(sd, core, ioctl, GET_SENSOR_EXIF, &exif)) {
		exif_attr->fnumber = 240;
		exif_attr->focal_length = 180;
		exif_attr->brightness = 128;
		exif_attr->flash_fire = 0;
		exif_attr->iso_speed = 200;
		exif_attr->exposure_time.numerator = 1;
		exif_attr->exposure_time.denominator = 20;
		exif_attr->shutter_speed.numerator = 1;
		exif_attr->shutter_speed.denominator = 24;
	} else {
		exif_attr->fnumber = exif.fnumber;
		exif_attr->focal_length = exif.focal_length;
		exif_attr->brightness = exif.brightness;
		exif_attr->flash_fire = exif.flash_fire;
		exif_attr->iso_speed = exif.iso_speed;
		exif_attr->exposure_time.numerator = exif.exposure_time_num;
		exif_attr->exposure_time.denominator = exif.exposure_time_den;
		exif_attr->shutter_speed.numerator = exif.exposure_time_num;
		exif_attr->shutter_speed.denominator = exif.exposure_time_den;
	}
	return 0;
}

static int __vin_sensor_line2time(struct v4l2_subdev *sd, u32 exp_line)
{
	struct sensor_info *info = to_state(sd);
	u32 overflow = 0xffffffff / 1000000, pclk = 0;
	int exp_time = 0;

	if ((exp_line / 16) > overflow) {
		exp_line = exp_line / 16;
		pclk = info->current_wins->pclk / 1000000;
	} else if ((exp_line / 16) > (overflow / 10)) {
		exp_line = exp_line * 10 / 16;
		pclk = info->current_wins->pclk / 100000;
	} else if ((exp_line / 16) > (overflow / 100)) {
		exp_line = exp_line * 100 / 16;
		pclk = info->current_wins->pclk / 10000;
	} else if ((exp_line / 16) > (overflow / 1000)) {
		exp_line = exp_line * 1000 / 16;
		pclk = info->current_wins->pclk / 1000;
	} else {
		exp_line = exp_line * 10000 / 16;
		pclk = info->current_wins->pclk / 100;
	}

	if (pclk)
		exp_time = exp_line * info->current_wins->hts / pclk;

	return exp_time;
}

static int __vin_sensor_set_af_win(struct vin_vid_cap *cap)
{
	struct vin_pipeline *pipe = &cap->pipe;
	struct v4l2_win_setting af_win;
	int ret = 0;

	af_win.coor.x1 = cap->af_win[0]->val;
	af_win.coor.y1 = cap->af_win[1]->val;
	af_win.coor.x2 = cap->af_win[2]->val;
	af_win.coor.y2 = cap->af_win[3]->val;

	ret = v4l2_subdev_call(pipe->sd[VIN_IND_SENSOR],
				core, ioctl, SET_AUTO_FOCUS_WIN, &af_win);
	return ret;
}

static int __vin_sensor_set_ae_win(struct vin_vid_cap *cap)
{
	struct vin_pipeline *pipe = &cap->pipe;
	struct v4l2_win_setting ae_win;
	int ret = 0;

	ae_win.coor.x1 = cap->ae_win[0]->val;
	ae_win.coor.y1 = cap->ae_win[1]->val;
	ae_win.coor.x2 = cap->ae_win[2]->val;
	ae_win.coor.y2 = cap->ae_win[3]->val;
	ret = v4l2_subdev_call(pipe->sd[VIN_IND_SENSOR],
				core, ioctl, SET_AUTO_EXPOSURE_WIN, &ae_win);
	return ret;
}
int vidioc_hdr_ctrl(struct file *file, struct v4l2_fh *fh,
		    struct isp_hdr_ctrl *hdr)
{
	struct vin_core *vinc = video_drvdata(file);
	struct v4l2_event ev;
	struct vin_isp_hdr_event_data *ed = (void *)ev.u.data;
	struct sensor_instance *inst = get_valid_sensor(vinc);

	memset(&ev, 0, sizeof(ev));
	ev.type = V4L2_EVENT_VIN_CLASS;
	if (inst->is_bayer_raw) {
		if (hdr->flag == HDR_CTRL_SET) {
			ed->cmd = HDR_CTRL_SET;
			ed->hdr = *hdr;
		} else {
			ed->cmd = HDR_CTRL_GET;
		}
		v4l2_event_queue(video_devdata(file), &ev);
		return 0;
	}
	return -EINVAL;
}

int vidioc_sync_ctrl(struct file *file, struct v4l2_fh *fh,
			struct csi_sync_ctrl *sync)
{
	struct vin_core *vinc = video_drvdata(file);

	if (!sync->type) {
		csic_prs_sync_en_cfg(vinc->csi_sel, sync);
		csic_prs_sync_cfg(vinc->csi_sel, sync);
		csic_prs_sync_wait_N(vinc->csi_sel, sync);
		csic_prs_sync_wait_M(vinc->csi_sel, sync);
		csic_frame_cnt_enable(vinc->vipp_sel);
		csic_dma_frm_cnt(vinc->vipp_sel, sync);
		csic_prs_sync_en(vinc->csi_sel, sync);
	} else {
		csic_prs_xs_en(vinc->csi_sel, sync);
		csic_prs_xs_period_len_register(vinc->csi_sel, sync);
	}
	return 0;
}

static int vidioc_set_top_clk(struct file *file, struct v4l2_fh *fh,
			struct vin_top_clk *clk)
{
	struct vin_core *vinc = video_drvdata(file);

	vinc->vin_clk = clk->clk_rate;

	return 0;
}

static int vidioc_set_fps_ds(struct file *file, struct v4l2_fh *fh,
			struct vin_fps_ds *fps_down_sample)
{
	struct vin_core *vinc = video_drvdata(file);

	vinc->fps_ds = fps_down_sample->fps_ds;

	return 0;
}

static int vidioc_set_isp_debug(struct file *file, struct v4l2_fh *fh,
			struct isp_debug_mode *isp_debug)
{
	struct vin_core *vinc = video_drvdata(file);

	vinc->isp_dbg = *isp_debug;
	sunxi_isp_debug(vinc->vid_cap.pipe.sd[VIN_IND_ISP], isp_debug);

	return 0;
}

static int vidioc_vin_ptn_config(struct file *file, struct v4l2_fh *fh,
			struct vin_pattern_config *ptn)
{
	struct vin_core *vinc = video_drvdata(file);
	struct csi_dev *csi = v4l2_get_subdevdata(vinc->vid_cap.pipe.sd[VIN_IND_CSI]);
	int ret = 0;

	if (!csi)
		return -ENODEV;

	if (ptn->ptn_en) {
		vinc->ptn_cfg.ptn_en = 1;
		vinc->ptn_cfg.ptn_w = ptn->ptn_w;
		vinc->ptn_cfg.ptn_h = ptn->ptn_h;
		vinc->ptn_cfg.ptn_mode = 12;
		vinc->ptn_cfg.ptn_buf.size = ptn->ptn_size;

		switch (ptn->ptn_fmt) {
		case V4L2_PIX_FMT_SBGGR8:
		case V4L2_PIX_FMT_SGBRG8:
		case V4L2_PIX_FMT_SGRBG8:
		case V4L2_PIX_FMT_SRGGB8:
			vinc->ptn_cfg.ptn_dw = 0;
			csi->csi_fmt->data_width = 8;
			break;
		case V4L2_PIX_FMT_SBGGR10:
		case V4L2_PIX_FMT_SGBRG10:
		case V4L2_PIX_FMT_SGRBG10:
		case V4L2_PIX_FMT_SRGGB10:
			vinc->ptn_cfg.ptn_dw = 1;
			csi->csi_fmt->data_width = 10;
			break;
		case V4L2_PIX_FMT_SBGGR12:
		case V4L2_PIX_FMT_SGBRG12:
		case V4L2_PIX_FMT_SGRBG12:
		case V4L2_PIX_FMT_SRGGB12:
			vinc->ptn_cfg.ptn_dw = 2;
			csi->csi_fmt->data_width = 12;
			break;
		default:
			vinc->ptn_cfg.ptn_dw = 2;
			csi->csi_fmt->data_width = 12;
			break;
		}
		csi->bus_info.bus_if = V4L2_MBUS_PARALLEL;

		if (ptn->ptn_addr) {
			if (vinc->ptn_cfg.ptn_buf.vir_addr == NULL)
				os_mem_alloc(&vinc->pdev->dev, &vinc->ptn_cfg.ptn_buf);
			if (vinc->ptn_cfg.ptn_buf.vir_addr == NULL) {
				vin_err("ptn buffer 0x%x alloc failed!\n", ptn->ptn_size);
				return -ENOMEM;
			}

			ret = copy_from_user(vinc->ptn_cfg.ptn_buf.vir_addr, ptn->ptn_addr, ptn->ptn_size);
			if (ret < 0) {
				vin_err("copy ptn buffer from usr error!\n");
				return ret;
			}
		}
	} else {
		vinc->ptn_cfg.ptn_en = 0;
		os_mem_free(&vinc->pdev->dev, &vinc->ptn_cfg.ptn_buf);
	}

	return 0;
}

static long vin_param_handler(struct file *file, void *priv,
			      bool valid_prio, unsigned int cmd, void *param)
{
	int ret = 0;
	struct v4l2_fh *fh = (struct v4l2_fh *)priv;

	switch (cmd) {
	case VIDIOC_ISP_EXIF_REQ:
		ret = isp_exif_req(file, fh, param);
		break;
	case VIDIOC_HDR_CTRL:
		ret = vidioc_hdr_ctrl(file, fh, param);
		break;
	case VIDIOC_SYNC_CTRL:
		ret = vidioc_sync_ctrl(file, fh, param);
		break;
	case VIDIOC_SET_TOP_CLK:
		ret = vidioc_set_top_clk(file, fh, param);
		break;
	case VIDIOC_SET_FPS_DS:
		ret = vidioc_set_fps_ds(file, fh, param);
		break;
	case VIDIOC_ISP_DEBUG:
		ret = vidioc_set_isp_debug(file, fh, param);
		break;
	case VIDIOC_VIN_PTN_CFG:
		ret = vidioc_vin_ptn_config(file, fh, param);
		break;
	default:
		ret = -ENOTTY;
	}
	return ret;
}

static int vin_subscribe_event(struct v4l2_fh *fh,
		const struct v4l2_event_subscription *sub)
{
	if (sub->type == V4L2_EVENT_CTRL)
		return v4l2_ctrl_subscribe_event(fh, sub);
	else
		return v4l2_event_subscribe(fh, sub, 1, NULL);
}

static int vin_unsubscribe_event(struct v4l2_fh *fh,
	const struct v4l2_event_subscription *sub)
{
	return v4l2_event_unsubscribe(fh, sub);
}

static int vin_open(struct file *file)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_vid_cap *cap = &vinc->vid_cap;

	if (vin_busy(cap)) {
		vin_err("device open busy\n");
		return -EBUSY;
	}
	set_bit(VIN_BUSY, &cap->state);
	v4l2_fh_open(file);/* create event queue */

#ifdef CONFIG_DEVFREQ_DRAM_FREQ_WITH_SOFT_NOTIFY
	dramfreq_master_access(MASTER_CSI, true);
#endif

	vin_log(VIN_LOG_VIDEO, "video%d open\n", vinc->id);
	return 0;
}

static int vin_close(struct file *file)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_md *vind = dev_get_drvdata(vinc->v4l2_dev->dev);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct modules_config *module = &vind->modules[vinc->sensor_sel];
	int valid_idx = module->sensors.valid_idx;
	int ret;

	if (!vin_busy(cap)) {
		vin_warn("video%d device have been closed!\n", vinc->id);
		return 0;
	}
	if (vin_streaming(cap)) {
		clear_bit(VIN_STREAM, &cap->state);
		vin_pipeline_call(vinc, set_stream, &cap->pipe, 0);
		del_timer(&vinc->timer_for_reset);
		flush_work(&vinc->vid_cap.pipeline_reset_task);
		vb2_ioctl_streamoff(file, NULL, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
	}

	if (cap->pipe.sd[VIN_IND_ACTUATOR] != NULL) {
		ret = __vin_actuator_set_power(cap->pipe.sd[VIN_IND_ACTUATOR], 0);
		if (ret < 0) {
			vin_err("actutor power off failed (%d)!\n", ret);
			return ret;
		}
	}

	ret = vin_pipeline_call(vinc, close, &cap->pipe);
	if (ret)
		vin_err("vin pipeline close failed!\n");

	v4l2_subdev_call(cap->pipe.sd[VIN_IND_ISP], core, init, 0);

	if ((vinc->large_image == 2) && vinc->ptn_cfg.ptn_en) {
		os_mem_free(&vinc->pdev->dev, &vinc->ptn_cfg.ptn_buf);
		vinc->ptn_cfg.ptn_en = 0;
	}

	/*software*/
	del_timer(&vinc->timer_for_reset);
	vb2_fop_release(file); /*vb2_queue_release(&cap->vb_vidq);*/
	clear_bit(VIN_BUSY, &cap->state);
	__vin_sensor_setup_link(module, valid_idx, 0);

#ifdef CONFIG_DEVFREQ_DRAM_FREQ_WITH_SOFT_NOTIFY
	dramfreq_master_access(MASTER_CSI, false);
#endif
	vin_log(VIN_LOG_VIDEO, "video%d close\n", vinc->id);
	return 0;
}

static int vin_try_ctrl(struct v4l2_ctrl *ctrl)
{
	/*
	 * to cheat control framework, because of  when ctrl->cur.val == ctrl->val
	 * s_ctrl would not be called
	 */
	if ((ctrl->minimum == 0) && (ctrl->maximum == 1)) {
		if (ctrl->val)
			ctrl->cur.val = 0;
		else
			ctrl->cur.val = 1;
	} else {
		if (ctrl->val == ctrl->maximum)
			ctrl->cur.val = ctrl->val - 1;
		else
			ctrl->cur.val = ctrl->val + 1;
	}

	/*
	 * to cheat control framework, because of  when ctrl->flags is
	 * V4L2_CTRL_FLAG_VOLATILE, s_ctrl would not be called
	 */
	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_EXPOSURE_ABSOLUTE:
	case V4L2_CID_GAIN:
		if (ctrl->val != ctrl->cur.val)
			ctrl->flags &= ~V4L2_CTRL_FLAG_VOLATILE;
		break;
	default:
		break;
	}
	return 0;
}
static int vin_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	struct vin_vid_cap *cap = container_of(ctrl->handler, struct vin_vid_cap, ctrl_handler);
	struct sensor_instance *inst = get_valid_sensor(cap->vinc);
	struct v4l2_subdev *sensor = cap->pipe.sd[VIN_IND_SENSOR];
	struct v4l2_subdev *flash = cap->pipe.sd[VIN_IND_FLASH];
	struct v4l2_control c;
	int ret = 0;

	c.id = ctrl->id;
	if (inst->is_isp_used && inst->is_bayer_raw) {
		switch (ctrl->id) {
		case V4L2_CID_EXPOSURE:
			v4l2_g_ctrl(sensor->ctrl_handler, &c);
			ctrl->val = c.value;
			break;
		case V4L2_CID_EXPOSURE_ABSOLUTE:
			c.id = V4L2_CID_EXPOSURE;
			v4l2_g_ctrl(sensor->ctrl_handler, &c);
			ctrl->val = __vin_sensor_line2time(sensor, c.value);
			break;
		case V4L2_CID_GAIN:
			v4l2_g_ctrl(sensor->ctrl_handler, &c);
			ctrl->val = c.value;
			break;
		case V4L2_CID_HOR_VISUAL_ANGLE:
		case V4L2_CID_VER_VISUAL_ANGLE:
		case V4L2_CID_FOCUS_LENGTH:
		case V4L2_CID_3A_LOCK:
		case V4L2_CID_AUTO_FOCUS_STATUS: /*Read-Only*/
			break;
		case V4L2_CID_SENSOR_TYPE:
			ctrl->val = inst->is_bayer_raw;
			break;
		default:
			return -EINVAL;
		}
		return ret;
	} else {
		switch (ctrl->id) {
		case V4L2_CID_SENSOR_TYPE:
			c.value = inst->is_bayer_raw;
			break;
		case V4L2_CID_FLASH_LED_MODE:
			ret = v4l2_g_ctrl(flash->ctrl_handler, &c);
			break;
		case V4L2_CID_AUTO_FOCUS_STATUS:
			ret = v4l2_g_ctrl(sensor->ctrl_handler, &c);
			if (c.value != V4L2_AUTO_FOCUS_STATUS_BUSY)
				sunxi_flash_stop(flash);
			break;
		default:
			ret = v4l2_g_ctrl(sensor->ctrl_handler, &c);
			break;
		}
		ctrl->val = c.value;
		if (ret < 0)
			vin_warn("v4l2 sub device g_ctrl fail!\n");
	}
	return ret;
}

static int vin_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct vin_vid_cap *cap = container_of(ctrl->handler, struct vin_vid_cap, ctrl_handler);
	struct sensor_instance *inst = get_valid_sensor(cap->vinc);
	struct v4l2_subdev *sensor = cap->pipe.sd[VIN_IND_SENSOR];
	struct v4l2_subdev *flash = cap->pipe.sd[VIN_IND_FLASH];
	struct v4l2_subdev *act = cap->pipe.sd[VIN_IND_ACTUATOR];
	struct v4l2_subdev *isp = cap->pipe.sd[VIN_IND_ISP];
	struct csic_dma_flip flip;
	struct actuator_ctrl_word_t vcm_ctrl;
	struct v4l2_control c;
	int ret = 0;

	c.id = ctrl->id;
	c.value = ctrl->val;

	switch (ctrl->id) {
	case V4L2_CID_VFLIP:
		if (cap->first_flag)
			cap->vinc->vflip_delay = 2;
		cap->vinc->vflip = c.value;
		return ret;
	case V4L2_CID_HFLIP:
		cap->vinc->hflip = c.value;
		flip.hflip_en = cap->vinc->hflip;
		flip.vflip_en = cap->vinc->vflip;
		csic_dma_flip_en(cap->vinc->vipp_sel, &flip);
		return ret;
	default:
		break;
	}

	/*
	 * make sure g_ctrl will get the value that hardware is using
	 * so that ctrl->flags should be V4L2_CTRL_FLAG_VOLATILE, after s_ctrl
	 */
	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_EXPOSURE_ABSOLUTE:
	case V4L2_CID_GAIN:
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
		break;
	default:
		break;
	}

	if (inst->is_isp_used && inst->is_bayer_raw) {
		switch (ctrl->id) {
		case V4L2_CID_BRIGHTNESS:
		case V4L2_CID_CONTRAST:
		case V4L2_CID_SATURATION:
		case V4L2_CID_HUE:
		case V4L2_CID_AUTO_WHITE_BALANCE:
		case V4L2_CID_EXPOSURE:
		case V4L2_CID_AUTOGAIN:
		case V4L2_CID_GAIN:
		case V4L2_CID_POWER_LINE_FREQUENCY:
		case V4L2_CID_HUE_AUTO:
		case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
		case V4L2_CID_SHARPNESS:
		case V4L2_CID_CHROMA_AGC:
		case V4L2_CID_COLORFX:
		case V4L2_CID_AUTOBRIGHTNESS:
		case V4L2_CID_BAND_STOP_FILTER:
		case V4L2_CID_ILLUMINATORS_1:
		case V4L2_CID_ILLUMINATORS_2:
		case V4L2_CID_EXPOSURE_AUTO:
		case V4L2_CID_EXPOSURE_ABSOLUTE:
		case V4L2_CID_EXPOSURE_AUTO_PRIORITY:
		case V4L2_CID_FOCUS_ABSOLUTE:
		case V4L2_CID_FOCUS_RELATIVE:
		case V4L2_CID_FOCUS_AUTO:
		case V4L2_CID_AUTO_EXPOSURE_BIAS:
		case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		case V4L2_CID_WIDE_DYNAMIC_RANGE:
		case V4L2_CID_IMAGE_STABILIZATION:
		case V4L2_CID_ISO_SENSITIVITY:
		case V4L2_CID_ISO_SENSITIVITY_AUTO:
		case V4L2_CID_EXPOSURE_METERING:
		case V4L2_CID_SCENE_MODE:
		case V4L2_CID_3A_LOCK:
		case V4L2_CID_AUTO_FOCUS_START:
		case V4L2_CID_AUTO_FOCUS_STOP:
		case V4L2_CID_AUTO_FOCUS_RANGE:
		case V4L2_CID_FLASH_LED_MODE:
		case V4L2_CID_AUTO_FOCUS_INIT:
		case V4L2_CID_AUTO_FOCUS_RELEASE:
		case V4L2_CID_GSENSOR_ROTATION:
		case V4L2_CID_TAKE_PICTURE:
			ret = v4l2_s_ctrl(NULL, isp->ctrl_handler, &c);
			break;
		default:
			ret = -EINVAL;
			break;
		}
	} else {
		switch (ctrl->id) {
		case V4L2_CID_FOCUS_ABSOLUTE:
			vcm_ctrl.code = ctrl->val;
			vcm_ctrl.sr = 0x0;
			ret = v4l2_subdev_call(act, core, ioctl, ACT_SET_CODE, &vcm_ctrl);
			break;
		case V4L2_CID_FLASH_LED_MODE:
			ret = v4l2_s_ctrl(NULL, flash->ctrl_handler, &c);
			break;
		case V4L2_CID_AUTO_FOCUS_START:
			sunxi_flash_check_to_start(flash, SW_CTRL_TORCH_ON);
			ret = v4l2_s_ctrl(NULL, sensor->ctrl_handler, &c);
			break;
		case V4L2_CID_AUTO_FOCUS_STOP:
			sunxi_flash_stop(flash);
			ret = v4l2_s_ctrl(NULL, sensor->ctrl_handler, &c);
			break;
		case V4L2_CID_AE_WIN_X1:
			ret = __vin_sensor_set_ae_win(cap);
			break;
		case V4L2_CID_AF_WIN_X1:
			ret = __vin_sensor_set_af_win(cap);
			break;
		case V4L2_CID_AUTO_EXPOSURE_BIAS:
			c.value = ctrl->val;
			ret = v4l2_s_ctrl(NULL, sensor->ctrl_handler, &c);
			break;
		default:
			ret = v4l2_s_ctrl(NULL, sensor->ctrl_handler, &c);
			break;
		}
	}
	return ret;
}

#ifdef CONFIG_COMPAT
static long vin_compat_ioctl32(struct file *file, unsigned int cmd,
			       unsigned long arg)
{
	void __user *up = compat_ptr(arg);
	long err = 0;

	err = video_ioctl2(file, cmd, (unsigned long)up);
	return err;
}
#endif
/* ------------------------------------------------------------------
 *File operations for the device
 *------------------------------------------------------------------*/

static const struct v4l2_ctrl_ops vin_ctrl_ops = {
	.g_volatile_ctrl = vin_g_volatile_ctrl,
	.s_ctrl = vin_s_ctrl,
	.try_ctrl = vin_try_ctrl,
};

static const struct v4l2_file_operations vin_fops = {
	.owner = THIS_MODULE,
	.open = vin_open,
	.release = vin_close,
	.read = vb2_fop_read,
	.poll = vb2_fop_poll,
	.unlocked_ioctl = video_ioctl2,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = vin_compat_ioctl32,
#endif
	.mmap = vb2_fop_mmap,
};

static const struct v4l2_ioctl_ops vin_ioctl_ops = {
	.vidioc_querycap = vidioc_querycap,
	.vidioc_enum_fmt_vid_cap_mplane = vidioc_enum_fmt_vid_cap_mplane,
	.vidioc_enum_framesizes = vidioc_enum_framesizes,
	.vidioc_g_fmt_vid_cap_mplane = vidioc_g_fmt_vid_cap_mplane,
	.vidioc_try_fmt_vid_cap_mplane = vidioc_try_fmt_vid_cap_mplane,
	.vidioc_s_fmt_vid_cap_mplane = vidioc_s_fmt_vid_cap_mplane,
	.vidioc_enum_fmt_vid_overlay = vidioc_enum_fmt_vid_overlay,
	.vidioc_g_fmt_vid_overlay = vidioc_g_fmt_vid_overlay,
	.vidioc_try_fmt_vid_overlay = vidioc_try_fmt_vid_overlay,
	.vidioc_s_fmt_vid_overlay = vidioc_s_fmt_vid_overlay,
	.vidioc_overlay = vidioc_overlay,
	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_qbuf = vb2_ioctl_qbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_expbuf = vb2_ioctl_expbuf,
	.vidioc_enum_input = vidioc_enum_input,
	.vidioc_g_input = vidioc_g_input,
	.vidioc_s_input = vidioc_s_input,
	.vidioc_streamon = vidioc_streamon,
	.vidioc_streamoff = vidioc_streamoff,
	.vidioc_g_parm = vidioc_g_parm,
	.vidioc_s_parm = vidioc_s_parm,
	.vidioc_g_crop = vidioc_g_crop,
	.vidioc_s_crop = vidioc_s_crop,
	.vidioc_g_selection = vidioc_g_selection,
	.vidioc_s_selection = vidioc_s_selection,
	.vidioc_default = vin_param_handler,
	.vidioc_subscribe_event = vin_subscribe_event,
	.vidioc_unsubscribe_event = vin_unsubscribe_event,
};

static const struct v4l2_ctrl_config ae_win_ctrls[] = {
	{
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_AE_WIN_X1,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_AE_WIN_Y1,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_AE_WIN_X2,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_AE_WIN_Y2,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}
};

static const struct v4l2_ctrl_config af_win_ctrls[] = {
	{
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_AF_WIN_X1,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_AF_WIN_Y1,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_AF_WIN_X2,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_AF_WIN_Y2,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}
};

static const struct v4l2_ctrl_config custom_ctrls[] = {
	{
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_HOR_VISUAL_ANGLE,
		.name = "Horizontal Visual Angle",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 360,
		.step = 1,
		.def = 60,
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_READ_ONLY,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_VER_VISUAL_ANGLE,
		.name = "Vertical Visual Angle",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 360,
		.step = 1,
		.def = 60,
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_READ_ONLY,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_FOCUS_LENGTH,
		.name = "Focus Length",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 1000,
		.step = 1,
		.def = 280,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_AUTO_FOCUS_INIT,
		.name = "AutoFocus Initial",
		.type = V4L2_CTRL_TYPE_BUTTON,
		.min = 0,
		.max = 0,
		.step = 0,
		.def = 0,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_AUTO_FOCUS_RELEASE,
		.name = "AutoFocus Release",
		.type = V4L2_CTRL_TYPE_BUTTON,
		.min = 0,
		.max = 0,
		.step = 0,
		.def = 0,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_GSENSOR_ROTATION,
		.name = "Gsensor Rotaion",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = -180,
		.max = 180,
		.step = 90,
		.def = 0,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_TAKE_PICTURE,
		.name = "Take Picture",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 16,
		.step = 1,
		.def = 0,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_SENSOR_TYPE,
		.name = "Sensor type",
		.type = V4L2_CTRL_TYPE_MENU,
		.min = 0,
		.max = 1,
		.def = 0,
		.menu_skip_mask = 0x0,
		.qmenu = sensor_info_type,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
};
static const s64 iso_qmenu[] = {
	100, 200, 400, 800, 1600, 3200, 6400,
};
static const s64 exp_bias_qmenu[] = {
	-4, -3, -2, -1, 0, 1, 2, 3, 4,
};

int vin_init_controls(struct v4l2_ctrl_handler *hdl, struct vin_vid_cap *cap)
{
	struct v4l2_ctrl *ctrl;
	unsigned int i, ret = 0;

	v4l2_ctrl_handler_init(hdl, 40 + ARRAY_SIZE(custom_ctrls)
		+ ARRAY_SIZE(ae_win_ctrls) + ARRAY_SIZE(af_win_ctrls));
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_BRIGHTNESS, -128, 128, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_CONTRAST, -128, 128, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_SATURATION, -256, 512, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_HUE, -180, 180, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_AUTO_WHITE_BALANCE, 0, 1, 1, 1);
	ctrl = v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_EXPOSURE, 1, 65536 * 16, 1, 1);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_AUTOGAIN, 0, 1, 1, 1);
	ctrl = v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_GAIN, 16, 6000 * 16, 1, 16);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_HFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_VFLIP, 0, 1, 1, 0);

	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops,
			       V4L2_CID_POWER_LINE_FREQUENCY,
			       V4L2_CID_POWER_LINE_FREQUENCY_AUTO, 0,
			       V4L2_CID_POWER_LINE_FREQUENCY_AUTO);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_HUE_AUTO, 0, 1, 1, 1);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops,
			  V4L2_CID_WHITE_BALANCE_TEMPERATURE, 2800, 10000, 1, 6500);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_SHARPNESS, -32, 32, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_CHROMA_AGC, 0, 1, 1, 1);
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops, V4L2_CID_COLORFX,
			       V4L2_COLORFX_SET_CBCR, 0, V4L2_COLORFX_NONE);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_AUTOBRIGHTNESS, 0, 1, 1, 1);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_BAND_STOP_FILTER, 0, 1, 1, 1);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_ILLUMINATORS_1, 0, 1, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_ILLUMINATORS_2, 0, 1, 1, 0);
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops, V4L2_CID_EXPOSURE_AUTO,
			       V4L2_EXPOSURE_APERTURE_PRIORITY, 0,
			       V4L2_EXPOSURE_AUTO);
	ctrl = v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_EXPOSURE_ABSOLUTE, 1, 30 * 1000000, 1, 1);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_EXPOSURE_AUTO_PRIORITY, 0, 1, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_FOCUS_ABSOLUTE, 0, 127, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_FOCUS_RELATIVE, -127, 127, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_FOCUS_AUTO, 0, 1, 1, 1);
	v4l2_ctrl_new_int_menu(hdl, &vin_ctrl_ops, V4L2_CID_AUTO_EXPOSURE_BIAS,
			       ARRAY_SIZE(exp_bias_qmenu) - 1,
			       ARRAY_SIZE(exp_bias_qmenu) / 2, exp_bias_qmenu);
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops,
			       V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE,
			       V4L2_WHITE_BALANCE_SHADE, 0,
			       V4L2_WHITE_BALANCE_AUTO);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_WIDE_DYNAMIC_RANGE, 0, 1, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_IMAGE_STABILIZATION, 0, 1, 1, 0);
	v4l2_ctrl_new_int_menu(hdl, &vin_ctrl_ops, V4L2_CID_ISO_SENSITIVITY,
			       ARRAY_SIZE(iso_qmenu) - 1,
			       ARRAY_SIZE(iso_qmenu) / 2 - 1, iso_qmenu);
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops,
			       V4L2_CID_ISO_SENSITIVITY_AUTO,
			       V4L2_ISO_SENSITIVITY_AUTO, 0,
			       V4L2_ISO_SENSITIVITY_AUTO);
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops,
			       V4L2_CID_EXPOSURE_METERING,
			       V4L2_EXPOSURE_METERING_MATRIX, 0,
			       V4L2_EXPOSURE_METERING_AVERAGE);
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops, V4L2_CID_SCENE_MODE,
			       V4L2_SCENE_MODE_TEXT, 0, V4L2_SCENE_MODE_NONE);
	ctrl = v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_3A_LOCK, 0, 7, 0, 0);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_AUTO_FOCUS_START, 0, 0, 0, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_AUTO_FOCUS_STOP, 0, 0, 0, 0);
	ctrl = v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_AUTO_FOCUS_STATUS, 0, 7, 0, 0);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops, V4L2_CID_AUTO_FOCUS_RANGE,
			       V4L2_AUTO_FOCUS_RANGE_INFINITY, 0,
			       V4L2_AUTO_FOCUS_RANGE_AUTO);
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops, V4L2_CID_FLASH_LED_MODE,
			       V4L2_FLASH_LED_MODE_RED_EYE, 0,
			       V4L2_FLASH_LED_MODE_NONE);

	for (i = 0; i < ARRAY_SIZE(custom_ctrls); i++)
		v4l2_ctrl_new_custom(hdl, &custom_ctrls[i], NULL);

	for (i = 0; i < ARRAY_SIZE(ae_win_ctrls); i++)
		cap->ae_win[i] = v4l2_ctrl_new_custom(hdl,
						&ae_win_ctrls[i], NULL);
	v4l2_ctrl_cluster(ARRAY_SIZE(ae_win_ctrls), &cap->ae_win[0]);

	for (i = 0; i < ARRAY_SIZE(af_win_ctrls); i++)
		cap->af_win[i] = v4l2_ctrl_new_custom(hdl,
						&af_win_ctrls[i], NULL);
	v4l2_ctrl_cluster(ARRAY_SIZE(af_win_ctrls), &cap->af_win[0]);

	if (hdl->error) {
		ret = hdl->error;
		v4l2_ctrl_handler_free(hdl);
	}
	return ret;
}

int vin_init_video(struct v4l2_device *v4l2_dev, struct vin_vid_cap *cap)
{
	int ret = 0;
	struct vb2_queue *q;
	static u64 vin_dma_mask = DMA_BIT_MASK(32);

	snprintf(cap->vdev.name, sizeof(cap->vdev.name),
		"vin_video%d", cap->vinc->id);
	cap->vdev.fops = &vin_fops;
	cap->vdev.ioctl_ops = &vin_ioctl_ops;
	cap->vdev.release = video_device_release_empty;
	cap->vdev.ctrl_handler = &cap->ctrl_handler;
	cap->vdev.v4l2_dev = v4l2_dev;
	cap->vdev.queue = &cap->vb_vidq;
	cap->vdev.lock = &cap->lock;
	cap->vdev.flags = V4L2_FL_USES_V4L2_FH;
	ret = video_register_device(&cap->vdev, VFL_TYPE_GRABBER, cap->vinc->id);
	if (ret < 0) {
		vin_err("Error video_register_device!!\n");
		return -1;
	}
	video_set_drvdata(&cap->vdev, cap->vinc);
	vin_log(VIN_LOG_VIDEO, "V4L2 device registered as %s\n",
		video_device_node_name(&cap->vdev));

	/* Initialize videobuf2 queue as per the buffer type */
	cap->vinc->pdev->dev.dma_mask = &vin_dma_mask;
	cap->vinc->pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	cap->dev = &cap->vinc->pdev->dev;
	vb2_dma_contig_set_max_seg_size(&cap->vinc->pdev->dev, 0);
	if (IS_ERR(cap->dev)) {
		vin_err("Failed to get the context\n");
		return -1;
	}
	/* initialize queue */
	q = &cap->vb_vidq;
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF | VB2_READ;
	q->drv_priv = cap;
	q->buf_struct_size = sizeof(struct vin_buffer);
	q->ops = &vin_video_qops;
	q->mem_ops = &vb2_dma_contig_memops;
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->lock = &cap->lock;

	ret = vb2_queue_init(q);
	if (ret) {
		vin_err("vb2_queue_init() failed\n");
		vb2_dma_contig_clear_max_seg_size(cap->dev);
		return ret;
	}

	cap->vd_pad.flags = MEDIA_PAD_FL_SINK;
	ret = media_entity_pads_init(&cap->vdev.entity, 1, &cap->vd_pad);
	if (ret)
		return ret;

	INIT_WORK(&cap->s_stream_task, __vin_s_stream_handle);

	cap->state = 0;
	cap->registered = 1;
	/* initial state */
	cap->capture_mode = V4L2_MODE_PREVIEW;
	/* init video dma queues */
	INIT_LIST_HEAD(&cap->vidq_active);
	mutex_init(&cap->lock);
	spin_lock_init(&cap->slock);

	return 0;
}

static int vin_link_setup(struct media_entity *entity,
			  const struct media_pad *local,
			  const struct media_pad *remote, u32 flags)
{
	return 0;
}

static const struct media_entity_operations vin_sd_media_ops = {
	.link_setup = vin_link_setup,
};

static int vin_subdev_enum_mbus_code(struct v4l2_subdev *sd,
				     struct v4l2_subdev_pad_config *cfg,
				     struct v4l2_subdev_mbus_code_enum *code)
{
	return 0;
}

static int vin_subdev_get_fmt(struct v4l2_subdev *sd,
			      struct v4l2_subdev_pad_config *cfg,
			      struct v4l2_subdev_format *fmt)
{
	return 0;
}

static int vin_subdev_set_fmt(struct v4l2_subdev *sd,
			      struct v4l2_subdev_pad_config *cfg,
			      struct v4l2_subdev_format *fmt)
{
	return 0;
}

static int vin_subdev_get_selection(struct v4l2_subdev *sd,
				    struct v4l2_subdev_pad_config *cfg,
				    struct v4l2_subdev_selection *sel)
{
	return 0;
}

static int vin_subdev_set_selection(struct v4l2_subdev *sd,
				    struct v4l2_subdev_pad_config *cfg,
				    struct v4l2_subdev_selection *sel)
{
	return 0;
}
static int vin_video_core_s_power(struct v4l2_subdev *sd, int on)
{
	struct vin_core *vinc = v4l2_get_subdevdata(sd);

	if (on)
		pm_runtime_get_sync(&vinc->pdev->dev);/*call pm_runtime resume*/
	else
		pm_runtime_put_sync(&vinc->pdev->dev);/*call pm_runtime suspend*/
	return 0;
}

static int vin_subdev_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct vin_core *vinc = v4l2_get_subdevdata(sd);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct csic_dma_cfg cfg;
	struct csic_dma_flip flip;
	struct dma_output_size size;
	struct dma_buf_len buf_len;
	struct dma_flip_size flip_size;
	int flag = 0;
	int flip_mul = 2;

	vin_log(VIN_LOG_FMT, "csic_dma%d %s, %d*%d hoff: %d voff: %d\n",
		vinc->id, enable ? "stream on" : "stream off",
		cap->frame.o_width, cap->frame.o_height,
		cap->frame.offs_h, cap->frame.offs_v);

	if (enable) {
		memset(&cfg, 0, sizeof(cfg));
		memset(&size, 0, sizeof(size));
		memset(&buf_len, 0, sizeof(buf_len));

		switch (cap->frame.fmt.field) {
		case V4L2_FIELD_ANY:
		case V4L2_FIELD_NONE:
			cfg.field = FIELD_EITHER;
			break;
		case V4L2_FIELD_TOP:
			cfg.field = FIELD_1;
			flag = 1;
			break;
		case V4L2_FIELD_BOTTOM:
			cfg.field = FIELD_2;
			flag = 1;
			break;
		case V4L2_FIELD_INTERLACED:
			cfg.field = FIELD_EITHER;
			flag = 1;
			break;
		default:
			cfg.field = FIELD_EITHER;
			break;
		}

		switch (cap->frame.fmt.fourcc) {
		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV12M:
		case V4L2_PIX_FMT_FBC:
			cfg.fmt = flag ? FRAME_UV_CB_YUV420 : FIELD_UV_CB_YUV420;
			buf_len.buf_len_y = cap->frame.o_width;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		case V4L2_PIX_FMT_NV21:
		case V4L2_PIX_FMT_NV21M:
			cfg.fmt = flag ? FRAME_VU_CB_YUV420 : FIELD_VU_CB_YUV420;
			buf_len.buf_len_y = cap->frame.o_width;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		case V4L2_PIX_FMT_YVU420:
		case V4L2_PIX_FMT_YUV420:
		case V4L2_PIX_FMT_YUV420M:
			cfg.fmt = flag ? FRAME_PLANAR_YUV420 : FIELD_PLANAR_YUV420;
			buf_len.buf_len_y = cap->frame.o_width;
			buf_len.buf_len_c = buf_len.buf_len_y >> 1;
			break;
		case V4L2_PIX_FMT_YUV422P:
			cfg.fmt = flag ? FRAME_PLANAR_YUV422 : FIELD_PLANAR_YUV422;
			buf_len.buf_len_y = cap->frame.o_width;
			buf_len.buf_len_c = buf_len.buf_len_y >> 1;
			break;
		case V4L2_PIX_FMT_NV61:
		case V4L2_PIX_FMT_NV61M:
			cfg.fmt = flag ? FRAME_VU_CB_YUV422 : FIELD_VU_CB_YUV422;
			buf_len.buf_len_y = cap->frame.o_width;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		case V4L2_PIX_FMT_NV16:
		case V4L2_PIX_FMT_NV16M:
			cfg.fmt = flag ? FRAME_UV_CB_YUV422 : FIELD_UV_CB_YUV422;
			buf_len.buf_len_y = cap->frame.o_width;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		case V4L2_PIX_FMT_SBGGR8:
		case V4L2_PIX_FMT_SGBRG8:
		case V4L2_PIX_FMT_SGRBG8:
		case V4L2_PIX_FMT_SRGGB8:
			flip_mul = 1;
			cfg.fmt = flag ? FRAME_RAW_8 : FIELD_RAW_8;
			buf_len.buf_len_y = cap->frame.o_width;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		case V4L2_PIX_FMT_SBGGR10:
		case V4L2_PIX_FMT_SGBRG10:
		case V4L2_PIX_FMT_SGRBG10:
		case V4L2_PIX_FMT_SRGGB10:
			flip_mul = 1;
			cfg.fmt = flag ? FRAME_RAW_10 : FIELD_RAW_10;
			buf_len.buf_len_y = cap->frame.o_width * 2;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		case V4L2_PIX_FMT_SBGGR12:
		case V4L2_PIX_FMT_SGBRG12:
		case V4L2_PIX_FMT_SGRBG12:
		case V4L2_PIX_FMT_SRGGB12:
			flip_mul = 1;
			cfg.fmt = flag ? FRAME_RAW_12 : FIELD_RAW_12;
			buf_len.buf_len_y = cap->frame.o_width * 2;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		default:
			cfg.fmt = flag ? FRAME_UV_CB_YUV420 : FIELD_UV_CB_YUV420;
			buf_len.buf_len_y = cap->frame.o_width;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		}

		if (vinc->isp_dbg.debug_en) {
			buf_len.buf_len_y = 0;
			buf_len.buf_len_c = 0;
		}

		switch (vinc->fps_ds) {
		case 0:
			cfg.ds = FPS_NO_DS;
			break;
		case 1:
			cfg.ds = FPS_2_DS;
			break;
		case 2:
			cfg.ds = FPS_3_DS;
			break;
		case 3:
			cfg.ds = FPS_4_DS;
			break;
		default:
			cfg.ds = FPS_NO_DS;
			break;
		}

		csic_dma_config(vinc->vipp_sel, &cfg);
		size.hor_len = vinc->isp_dbg.debug_en ? 0 : cap->frame.o_width;
		size.ver_len = vinc->isp_dbg.debug_en ? 0 : cap->frame.o_height;
		size.hor_start = vinc->isp_dbg.debug_en ? 0 : cap->frame.offs_h;
		size.ver_start = vinc->isp_dbg.debug_en ? 0 : cap->frame.offs_v;
		flip_size.hor_len = vinc->isp_dbg.debug_en ? 0 : cap->frame.o_width * flip_mul;
		flip_size.ver_len = vinc->isp_dbg.debug_en ? 0 : cap->frame.o_height;
		flip.hflip_en = vinc->hflip;
		flip.vflip_en = vinc->vflip;

		if (vinc->large_image == 2) {
			size.hor_len /= 2;
			flip_size.hor_len /= 2;
		}

		csic_dma_output_size_cfg(vinc->vipp_sel, &size);
		csic_dma_buffer_length(vinc->vipp_sel, &buf_len);
		csic_dma_flip_size(vinc->vipp_sel, &flip_size);
		csic_dma_flip_en(vinc->vipp_sel, &flip);
		/* give up line_cut interrupt. process in vsync and frame_done isr.*/
		/*csic_dma_line_cnt(vinc->vipp_sel, cap->frame.o_height / 16 * 12);*/
		csic_frame_cnt_enable(vinc->vipp_sel);
		if (cap->frame.fmt.fourcc == V4L2_PIX_FMT_FBC)
			csic_fbc_enable(vinc->vipp_sel);
		else
			csic_dma_enable(vinc->vipp_sel);
		csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_ALL);
		if (vinc->isp_dbg.debug_en)
			csic_dma_int_enable(vinc->vipp_sel, DMA_INT_ALL & ~(DMA_INT_FBC_OVHD_WRDDR_FULL));
		else
			csic_dma_int_enable(vinc->vipp_sel, DMA_INT_ALL);
		csic_dma_int_disable(vinc->vipp_sel, DMA_INT_LINE_CNT);
	} else {
		csic_dma_int_disable(vinc->vipp_sel, DMA_INT_ALL);
		csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_ALL);
		if (cap->frame.fmt.fourcc == V4L2_PIX_FMT_FBC)
			csic_fbc_disable(vinc->vipp_sel);
		else
			csic_dma_disable(vinc->vipp_sel);
	}

	return 0;
}

static struct v4l2_subdev_core_ops vin_subdev_core_ops = {
	.s_power = vin_video_core_s_power,
};

static const struct v4l2_subdev_video_ops vin_subdev_video_ops = {
	.s_stream = vin_subdev_s_stream,
};

static struct v4l2_subdev_pad_ops vin_subdev_pad_ops = {
	.enum_mbus_code = vin_subdev_enum_mbus_code,
	.get_selection = vin_subdev_get_selection,
	.set_selection = vin_subdev_set_selection,
	.get_fmt = vin_subdev_get_fmt,
	.set_fmt = vin_subdev_set_fmt,
};

static struct v4l2_subdev_ops vin_subdev_ops = {
	.core = &vin_subdev_core_ops,
	.video = &vin_subdev_video_ops,
	.pad = &vin_subdev_pad_ops,
};

static int vin_capture_subdev_registered(struct v4l2_subdev *sd)
{
	struct vin_core *vinc = v4l2_get_subdevdata(sd);
	int ret;

	vinc->vid_cap.vinc = vinc;
	if (vin_init_controls(&vinc->vid_cap.ctrl_handler, &vinc->vid_cap)) {
		vin_err("Error v4l2 ctrls new!!\n");
		return -1;
	}

	vinc->pipeline_ops = v4l2_get_subdev_hostdata(sd);
	if (vin_init_video(sd->v4l2_dev, &vinc->vid_cap)) {
		vin_err("vin init video!!!!\n");
		vinc->pipeline_ops = NULL;
	}
	ret = sysfs_create_link(&vinc->vid_cap.vdev.dev.kobj,
		&vinc->pdev->dev.kobj, "vin_dbg");
	if (ret)
		vin_err("sysfs_create_link failed\n");

	return 0;
}

static void vin_capture_subdev_unregistered(struct v4l2_subdev *sd)
{
	struct vin_core *vinc = v4l2_get_subdevdata(sd);

	if (vinc == NULL)
		return;

	if (video_is_registered(&vinc->vid_cap.vdev)) {
		sysfs_remove_link(&vinc->vid_cap.vdev.dev.kobj, "vin_dbg");
		vin_log(VIN_LOG_VIDEO, "unregistering %s\n",
			video_device_node_name(&vinc->vid_cap.vdev));
		video_unregister_device(&vinc->vid_cap.vdev);
		vb2_dma_contig_clear_max_seg_size(vinc->vid_cap.dev);
		media_entity_cleanup(&vinc->vid_cap.vdev.entity);
		flush_work(&vinc->vid_cap.s_stream_task);
		mutex_destroy(&vinc->vid_cap.lock);
	}
	v4l2_ctrl_handler_free(&vinc->vid_cap.ctrl_handler);
	vinc->pipeline_ops = NULL;
}

static const struct v4l2_subdev_internal_ops vin_capture_sd_internal_ops = {
	.registered = vin_capture_subdev_registered,
	.unregistered = vin_capture_subdev_unregistered,
};

int vin_initialize_capture_subdev(struct vin_core *vinc)
{
	struct v4l2_subdev *sd = &vinc->vid_cap.subdev;
	int ret;

	v4l2_subdev_init(sd, &vin_subdev_ops);
	sd->grp_id = VIN_GRP_ID_CAPTURE;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	snprintf(sd->name, sizeof(sd->name), "vin_cap.%d", vinc->id);

	vinc->vid_cap.sd_pads[VIN_SD_PAD_SINK].flags = MEDIA_PAD_FL_SINK;
	vinc->vid_cap.sd_pads[VIN_SD_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.function = MEDIA_ENT_F_IO_V4L;
	ret = media_entity_pads_init(&sd->entity, VIN_SD_PADS_NUM,
				vinc->vid_cap.sd_pads);
	if (ret)
		return ret;

	sd->entity.ops = &vin_sd_media_ops;
	sd->internal_ops = &vin_capture_sd_internal_ops;
	v4l2_set_subdevdata(sd, vinc);
	return 0;
}

void vin_cleanup_capture_subdev(struct vin_core *vinc)
{
	struct v4l2_subdev *sd = &vinc->vid_cap.subdev;

	media_entity_cleanup(&sd->entity);
	v4l2_set_subdevdata(sd, NULL);
}

