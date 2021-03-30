
/*
 ******************************************************************************
 *
 * vin_video.c
 *
 * Hawkview ISP - vin_video.c module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Zhao Wei   	2015/11/30	ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#include <linux/version.h>
#include <linux/videodev2.h>
#include <linux/string.h>
#include <linux/freezer.h>

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

extern unsigned int vin_dump;
extern unsigned int frame_cnt;

void __vin_start_opened(struct vin_vid_cap *cap)
{
	mutex_lock(&cap->opened_lock);
	set_bit(0, &cap->opened);
	mutex_unlock(&cap->opened_lock);
}

void __vin_stop_opened(struct vin_vid_cap *cap)
{
	mutex_lock(&cap->opened_lock);
	clear_bit(0, &cap->opened);
	mutex_unlock(&cap->opened_lock);
}
int vin_is_generating(struct vin_vid_cap *cap)
{
	int ret;
	unsigned long flags = 0;
	spin_lock_irqsave(&cap->slock, flags);
	ret = test_bit(0, &cap->generating);
	spin_unlock_irqrestore(&cap->slock, flags);
	return ret;
}
int vin_is_opened(struct vin_vid_cap *cap)
{
	int ret;
	mutex_lock(&cap->opened_lock);
	ret = test_bit(0, &cap->opened);
	mutex_unlock(&cap->opened_lock);
	return ret;
}

void vin_start_generating(struct vin_vid_cap *cap)
{
	unsigned long flags = 0;
	spin_lock_irqsave(&cap->slock, flags);
	set_bit(0, &cap->generating);
	spin_unlock_irqrestore(&cap->slock, flags);
}

void vin_stop_generating(struct vin_vid_cap *cap)
{
	unsigned long flags = 0;
	spin_lock_irqsave(&cap->slock, flags);
	cap->first_flag = 0;
	clear_bit(0, &cap->generating);
	spin_unlock_irqrestore(&cap->slock, flags);
}

/* The color format (colplanes, memplanes) must be already configured. */
int vin_set_addr(struct vin_core *vinc, struct vb2_buffer *vb,
		      struct vin_frame *frame, struct vin_addr *paddr)
{
	int ret = 0, valid_idx = 0;
	u32 pix_size;

	if (vb == NULL || frame == NULL)
		return -EINVAL;

	pix_size = ALIGN(frame->width, VIN_ALIGN_WIDTH)
		* ALIGN(frame->height, VIN_ALIGN_HEIGHT);

	vin_log(VIN_LOG_ISP, "memplanes= %d, colplanes= %d, pix_size= %d",
		frame->fmt->memplanes, frame->fmt->colplanes, pix_size);

	paddr->y = vb2_dma_contig_plane_dma_addr(vb, 0);

	if (frame->fmt->memplanes == 1) {
		switch (frame->fmt->colplanes) {
		case 1:
			paddr->cb = 0;
			paddr->cr = 0;
			break;
		case 2:
			/* decompose Y into Y/Cb */
			paddr->cb = (u32)(paddr->y + pix_size);
			paddr->cr = 0;
			break;
		case 3:
			paddr->cb = (u32)(paddr->y + pix_size);
			/* 420 */
			if (12 == frame->fmt->depth[0])
				paddr->cr = (u32)(paddr->cb + (pix_size >> 2));
			else /* 422 */
				paddr->cr = (u32)(paddr->cb + (pix_size >> 1));

			break;
		default:
			return -EINVAL;
		}
	} else if (!frame->fmt->mdataplanes) {
		if (frame->fmt->memplanes >= 2)
			paddr->cb = vb2_dma_contig_plane_dma_addr(vb, 1);

		if (frame->fmt->memplanes == 3)
			paddr->cr = vb2_dma_contig_plane_dma_addr(vb, 2);
	}

	vin_log(VIN_LOG_ISP, "PHYS_ADDR: y = 0x%X  cb = 0x%X cr = 0x%X ret= %d",
			paddr->y, paddr->cb, paddr->cr, ret);

	valid_idx = vinc->modu_cfg.sensors.valid_idx;
	if (vinc->modu_cfg.sensors.inst[valid_idx].is_isp_used) {

	} else {
		bsp_csi_set_addr(vinc->csi_sel, paddr->y, paddr->y, paddr->cb,
				 paddr->cr);
	}
	return 0;
}

/*
 * Videobuf operations
 */
static int queue_setup(struct vb2_queue *vq, const struct v4l2_format *fmt,
		       unsigned int *nbuffers, unsigned int *nplanes,
		       unsigned int sizes[], void *alloc_ctxs[])
{
	struct vin_vid_cap *cap = vb2_get_drv_priv(vq);
	unsigned int size;
	int buf_max_flag = 0;
	int i;
	vin_log(VIN_LOG_VIDEO, "queue_setup\n");

	size = cap->buf_byte_size;

	if (size == 0)
		return -EINVAL;

	if (0 == *nbuffers)
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

	*nplanes = cap->frame.fmt->memplanes;
	for (i = 0; i < *nplanes; i++) {
		sizes[i] = cap->frame.payload[i];
		alloc_ctxs[i] = cap->alloc_ctx;
	}
	vin_print("%s, buf count = %d, nplanes = %d, size = %d\n",
		__func__, *nbuffers, *nplanes, size);

	return 0;
}

static int buffer_prepare(struct vb2_buffer *vb)
{
	struct vin_vid_cap *cap = vb2_get_drv_priv(vb->vb2_queue);
	struct vin_buffer *buf = container_of(vb, struct vin_buffer, vb);
	/*unsigned long size;*/
	int i;
	vin_log(VIN_LOG_VIDEO, "buffer_prepare\n");

	if (cap->width < MIN_WIDTH || cap->width > MAX_WIDTH ||
	    cap->height < MIN_HEIGHT || cap->height > MAX_HEIGHT) {
		return -EINVAL;
	}
	/*size = dev->buf_byte_size;*/

	for (i = 0; i < cap->frame.fmt->memplanes; i++) {
		if (vb2_plane_size(vb, i) < cap->frame.payload[i]) {
			vin_err("%s data will not fit into plane (%lu < %lu)\n",
				__func__, vb2_plane_size(vb, i),
				cap->frame.payload[i]);
			return -EINVAL;
		}
		vb2_set_plane_payload(&buf->vb, i, cap->frame.payload[i]);
		vb->v4l2_planes[i].m.mem_offset = vb2_dma_contig_plane_dma_addr(vb, i);
	}

	return 0;
}

static void buffer_queue(struct vb2_buffer *vb)
{
	struct vin_vid_cap *cap = vb2_get_drv_priv(vb->vb2_queue);
	struct vin_buffer *buf = container_of(vb, struct vin_buffer, vb);
	unsigned long flags = 0;

	vin_log(VIN_LOG_VIDEO, "buffer_queue\n");
	spin_lock_irqsave(&cap->slock, flags);
	list_add_tail(&buf->list, &cap->vidq_active);
	spin_unlock_irqrestore(&cap->slock, flags);
}

static int start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct vin_vid_cap *cap = vb2_get_drv_priv(vq);
	vin_log(VIN_LOG_VIDEO, "%s\n", __func__);
	vin_start_generating(cap);
	return 0;
}

/* abort streaming and wait for last buffer */
static int stop_streaming(struct vb2_queue *vq)
{
	struct vin_vid_cap *cap = vb2_get_drv_priv(vq);
	unsigned long flags = 0;

	vin_log(VIN_LOG_VIDEO, "%s\n", __func__);

	vin_stop_generating(cap);

	spin_lock_irqsave(&cap->slock, flags);
	/* Release all active buffers */
	while (!list_empty(&cap->vidq_active)) {
		struct vin_buffer *buf;
		buf =
		    list_entry(cap->vidq_active.next, struct vin_buffer, list);
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_ERROR);
		vin_log(VIN_LOG_VIDEO, "buf %d stop\n", buf->vb.v4l2_buf.index);
	}
	spin_unlock_irqrestore(&cap->slock, flags);
	return 0;
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
	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING |
	    V4L2_CAP_READWRITE;
	return 0;
}

static int vidioc_enum_fmt_vid_cap_mplane(struct file *file, void *priv,
					  struct v4l2_fmtdesc *f)
{
	struct vin_fmt *fmt;

	vin_log(VIN_LOG_VIDEO, "%s\n", __func__);
	fmt = vin_get_format(f->index);
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

	vin_log(VIN_LOG_VIDEO, "%s\n", __func__);

	if (vinc == NULL) {
		return -EINVAL;
	}

	return v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], video,
				enum_framesizes, fsize);
}

static int vidioc_g_fmt_vid_cap_mplane(struct file *file, void *priv,
				       struct v4l2_format *f)
{
	struct vin_vid_cap *cap =
	    container_of(video_devdata(file), struct vin_vid_cap, vdev);

	vin_get_fmt_mplane(&cap->frame, f);
	return 0;
}

static struct media_entity *vin_pipeline_get_head(struct media_entity *me)
{
	struct media_pad *pad = &me->pads[0];

	while (!(pad->flags & MEDIA_PAD_FL_SOURCE)) {
		pad = media_entity_remote_source(pad);
		if (!pad)
			break;
		me = pad->entity;
		pad = &me->pads[0];
	}

	return me;
}

#if 0
static struct media_entity *vin_pipeline_get_terminus(struct media_entity *me)
{
	struct media_pad *pad = &me->pads[me->num_pads - 1];

	while (!(pad->flags & MEDIA_PAD_FL_SINK)) {
		pad = media_entity_remote_source(pad);
		if (!pad)
			break;
		me = pad->entity;
		pad = &me->pads[me->num_pads - 1];
		printk("vin_pipeline_get_terminus entity is %s\n", me->name);
	}

	return me;
}
#endif

static int vin_pipeline_try_format(struct vin_core *vinc,
				    struct v4l2_mbus_framefmt *tfmt,
				    struct vin_fmt **fmt_id,
				    bool set)
{
	struct v4l2_subdev *sd = vinc->vid_cap.pipe.sd[VIN_IND_SENSOR];
	struct v4l2_subdev_format sfmt;
	struct v4l2_mbus_framefmt *mf = &sfmt.format;
	struct media_entity *me;
	struct vin_fmt *ffmt;
	/*struct media_pad *pad;*/
	unsigned int mask = (*fmt_id)->flags;
	struct media_entity_graph graph;
	u32 fcc;
	int ret, i = 0;

	if (WARN_ON(!sd || !tfmt))
		return -EINVAL;

	memset(&sfmt, 0, sizeof(sfmt));
	sfmt.format = *tfmt;
	sfmt.which = set ? V4L2_SUBDEV_FORMAT_ACTIVE : V4L2_SUBDEV_FORMAT_TRY;

	if (((*fmt_id)->flags & VIN_FMT_YUV) &&	(vinc->support_raw == 0)) {
		mask = VIN_FMT_YUV;
	}

	while (1) {

		ffmt = vin_find_format(NULL, mf->code != 0 ? &mf->code : NULL,
					mask, i++, true);
		if (ffmt == NULL) {
			/*
			 * Notify user-space if common pixel code for
			 * host and sensor does not exist.
			 */
			vin_err("ffmt entity is null\n");

			return -EINVAL;
		}
#if 0
		mf->code = tfmt->code = ffmt->mbus_code;

		me = vin_pipeline_get_head(&sd->entity);

		/* set format on all pipeline subdevs */
		while (me != &vinc->vid_cap.subdev.entity) {
			struct media_pad *pad;

			sd = media_entity_to_v4l2_subdev(me);
			vin_print("media entity is %s\n", me->name);

			sfmt.pad = 0;
			ret = v4l2_subdev_call(sd, pad, set_fmt, NULL, &sfmt);
			if (ret)
				return ret;

			if (me->pads[0].flags & MEDIA_PAD_FL_SINK) {
				sfmt.pad = me->num_pads - 1;
				mf->code = tfmt->code;
				ret = v4l2_subdev_call(sd, pad, set_fmt, NULL,
									&sfmt);
				if (ret)
					return ret;
			}

			pad = media_entity_remote_source(&me->pads[sfmt.pad]);
			if (!pad)
				return -EINVAL;
			me = pad->entity;
		}
#else
		mf->code = tfmt->code = ffmt->mbus_code;

		me = &vinc->vid_cap.subdev.entity;
		media_entity_graph_walk_start(&graph, me);
		while ((me = media_entity_graph_walk_next(&graph)) &&
			me != &vinc->vid_cap.subdev.entity){

			sd = media_entity_to_v4l2_subdev(me);
			vin_log(VIN_LOG_VIDEO, "found %s in this pipeline\n",
								me->name);
			if (me->num_pads == 1 &&
				(me->pads[0].flags & MEDIA_PAD_FL_SINK)) {
				vin_log(VIN_LOG_VIDEO, "skip %s.\n", me->name);
				continue;
			}

			sfmt.pad = 0;
			ret = v4l2_subdev_call(sd, pad, set_fmt, NULL, &sfmt);
			if (ret)
				return ret;

			if (me->pads[0].flags & MEDIA_PAD_FL_SINK) {
				sfmt.pad = me->num_pads - 1;
				mf->code = tfmt->code;
				ret = v4l2_subdev_call(sd, pad, set_fmt, NULL,
									&sfmt);
				if (ret)
					return ret;
			}
		}
#endif
		if (mf->code != tfmt->code)
			continue;

		fcc = ffmt->fourcc;
		tfmt->width  = mf->width;
		tfmt->height = mf->height;

		if (ffmt && ffmt->mbus_code)
			mf->code = ffmt->mbus_code;

		if (mf->width != tfmt->width || mf->height != tfmt->height)
			continue;
		tfmt->code = mf->code;

		break;
	}

	if (fmt_id && ffmt)
		*fmt_id = ffmt;
	*tfmt = *mf;

#if 0
	me = &vinc->vid_cap.subdev.entity;
	media_entity_graph_walk_start(&graph, me);
	while ((me = media_entity_graph_walk_next(&graph)) &&
		me != &vinc->vid_cap.subdev.entity) {
		printk("media_entity_graph_walk_next entity is %s\n", me->name);
	}
#endif
	return 0;
}

static int vin_pipeline_set_mbus_config(struct vin_core *vinc)
{
	struct vin_pipeline *pipe = &vinc->vid_cap.pipe;
	struct v4l2_subdev *sd = pipe->sd[VIN_IND_SENSOR];
	struct v4l2_mbus_config mcfg;
	struct media_entity *me;
	struct media_pad *pad;
	int ret;

	me = vin_pipeline_get_head(&sd->entity);
	ret = v4l2_subdev_call(sd, video, g_mbus_config, &mcfg);
	if (ret < 0) {
		vin_err("%s g_mbus_config error!\n", me->name);
		goto out;
	}
	/* s_mbus_config on all mipi and csi */
	while (me != &vinc->vid_cap.subdev.entity) {
		sd = media_entity_to_v4l2_subdev(me);
		vin_log(VIN_LOG_VIDEO, "media entity is %s\n", me->name);
		if ((sd == pipe->sd[VIN_IND_MIPI]) ||
		    (sd == pipe->sd[VIN_IND_CSI])) {
			ret = v4l2_subdev_call(sd, video, s_mbus_config, &mcfg);
			if (ret < 0) {
				vin_err("%s s_mbus_config error!\n", me->name);
				goto out;
			}
		}

		pad = media_entity_remote_source(&me->pads[me->num_pads - 1]);
		if (!pad)
			return -EINVAL;
		me = pad->entity;
	}

	vinc->mbus_type = mcfg.type;
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

	vin_log(VIN_LOG_VIDEO, "%s\n", __func__);
	ffmt = vin_find_format(&f->fmt.pix_mp.pixelformat, NULL,
		VIN_FMT_ALL, -1, false);

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
	struct sensor_instance *inst = get_valid_sensor(vinc);
	struct sensor_win_size win_cfg;
	struct v4l2_subdev_format mipi_fmt;

	struct v4l2_mbus_framefmt mf;
	struct vin_fmt *ffmt = NULL;

	int ret;

	vin_log(VIN_LOG_VIDEO, "%s\n", __func__);

	if (vin_is_generating(cap)) {
		vin_err("%s device busy\n", __func__);
		return -EBUSY;
	}

	ffmt = vin_find_format(&f->fmt.pix_mp.pixelformat, NULL,
		VIN_FMT_ALL, -1, false);
	if (NULL == ffmt) {
		vin_err("vin does not support this pixelformat 0x%x\n",
				f->fmt.pix_mp.pixelformat);
		return -EINVAL;
	}
	mf.width = f->fmt.pix_mp.width;
	mf.height = f->fmt.pix_mp.height;
	mf.code = ffmt->mbus_code;
	ret = vin_pipeline_try_format(vinc, &mf, &ffmt, true);
	if (ret < 0) {
		vin_err("vin_pipeline_try_format failed\n");
		return -EINVAL;
	}

	vin_print("vin_pipeline_try_format %d*%d %x %d\n",
		mf.width, mf.height,
		mf.code, mf.field);

	vin_pipeline_set_mbus_config(vinc);

	/*get current win configs*/
	memset(&win_cfg, 0, sizeof(struct sensor_win_size));
	ret =
	    v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core, ioctl,
			     GET_CURRENT_WIN_CFG, &win_cfg);

	if (vinc->mbus_type == V4L2_MBUS_CSI2) {
		mipi_fmt.format.reserved[0] = win_cfg.mipi_bps;
		mipi_fmt.format.code = mf.code;
		mipi_fmt.format.field = f->fmt.pix_mp.field;
		mipi_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
		mipi_fmt.pad = MIPI_PAD_SINK;
		ret =
		    v4l2_subdev_call(cap->pipe.sd[VIN_IND_MIPI], pad,
				     set_fmt, NULL, &mipi_fmt);
		if (ret < 0) {
			vin_err("v4l2 sub device mipi set_fmt error!\n");
			goto out;
		}
	}

	if (cap->capture_mode == V4L2_MODE_IMAGE) {
		sunxi_flash_check_to_start(cap->pipe.sd[VIN_IND_FLASH],
					   SW_CTRL_FLASH_ON);
	} else {
		sunxi_flash_stop(vinc->vid_cap.pipe.sd[VIN_IND_FLASH]);
	}

	cap->frame.fmt = vin_find_format(&f->fmt.pix_mp.pixelformat, NULL,
					VIN_FMT_ALL, -1, false);
	cap->frame.o_width = mf.width;
	cap->frame.o_height = mf.height;
	/******** output format should set in video, but now in csi ********/
	sunxi_csi_set_output_fmt(cap->pipe.sd[VIN_IND_CSI],
				f->fmt.pix_mp.pixelformat);

	if (inst->is_isp_used) {

	} else {
		if (cap->frame.fmt->memplanes == 1) {
			cap->frame.payload[0] = mf.width * mf.height * 3 / 2;
			cap->buf_byte_size = PAGE_ALIGN(cap->frame.payload[0]);
		} else if (cap->frame.fmt->memplanes == 2) {
			cap->frame.payload[0] = mf.width * mf.height;
			cap->frame.payload[1] = mf.width * mf.height / 2;
			cap->buf_byte_size =
				PAGE_ALIGN(cap->frame.payload[0]) +
				PAGE_ALIGN(cap->frame.payload[1]);
		} else if (cap->frame.fmt->memplanes == 3) {
			cap->frame.payload[0] = mf.width * mf.height;
			cap->frame.payload[1] = mf.width * mf.height / 4;
			cap->frame.payload[2] = mf.width * mf.height / 4;
			cap->buf_byte_size =
				PAGE_ALIGN(cap->frame.payload[0]) +
				PAGE_ALIGN(cap->frame.payload[1]) +
				PAGE_ALIGN(cap->frame.payload[2]);
		}
	}
	cap->width = mf.width;
	cap->height = mf.height;

	ret = 0;
out:
	return ret;

}


static int vidioc_s_fmt_vid_cap_mplane(struct file *file, void *priv,
				       struct v4l2_format *f)
{
	struct vin_core *vinc = video_drvdata(file);
	/*vin_try_fmt_vid_cap(file, priv, f);*/
	return __vin_set_fmt(vinc, f);
}

static int vidioc_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct vin_buffer *buf;
	struct vin_pipeline *pipe = &vinc->vid_cap.pipe;
	int ret = 0;

	mutex_lock(&cap->stream_lock);
	vin_print("%s\n", __func__);
	if (i != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		ret = -EINVAL;
		goto streamon_unlock;
	}

	if (vin_is_generating(cap)) {
		vin_err("stream has been already on\n");
		ret = -1;
		goto streamon_unlock;
	}

	ret = media_entity_pipeline_start(&cap->vdev.entity, &pipe->pipe);
	if (ret < 0)
		goto streamon_unlock;

	ret = vb2_ioctl_streamon(file, priv, i);
	if (ret)
		goto streamon_unlock;

	buf = list_entry(cap->vidq_active.next, struct vin_buffer, list);

	vin_set_addr(vinc, &buf->vb, &vinc->vid_cap.frame,
			&vinc->vid_cap.frame.paddr);
	vin_print("call set_stream, %s!\n", __func__);

	ret = vin_pipeline_call(vinc, set_stream, &cap->pipe, 1);
	if (ret < 0) {
		vb2_ioctl_streamoff(file, priv, i);
		vin_err("%s error!\n", __func__);
		goto streamon_unlock;
	}

streamon_unlock:
	mutex_unlock(&cap->stream_lock);

	return ret;
}

static int vidioc_streamoff(struct file *file, void *priv, enum v4l2_buf_type i)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	int ret = 0;

	mutex_lock(&cap->stream_lock);
	vin_print("%s\n", __func__);
	if (!vin_is_generating(cap)) {
		vin_err("stream has been already off\n");
		ret = 0;
		goto streamoff_unlock;
	}

	vin_pipeline_call(vinc, set_stream, &cap->pipe, 0);

	if (i != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		ret = -EINVAL;
		goto streamoff_unlock;
	}

	ret = vb2_ioctl_streamoff(file, priv, i);
	if (ret != 0) {
		vin_err("%s error!\n", __func__);
		goto streamoff_unlock;
	}

	media_entity_pipeline_stop(&cap->vdev.entity);

streamoff_unlock:
	mutex_unlock(&cap->stream_lock);

	return ret;
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
	*i = 0;
	return 0;
}

static int vidioc_s_input(struct file *file, void *priv, unsigned int i)
{
	vin_log(VIN_LOG_VIDEO, "input_num = %d\n", i);
	return i == 0 ? i : -EINVAL;
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
	int ret = 0, valid_idx = vinc->modu_cfg.sensors.valid_idx;

	struct sensor_instance *inst = &vinc->modu_cfg.sensors.inst[valid_idx];

	if (parms->parm.capture.capturemode != V4L2_MODE_VIDEO &&
	    parms->parm.capture.capturemode != V4L2_MODE_IMAGE &&
	    parms->parm.capture.capturemode != V4L2_MODE_PREVIEW) {
		parms->parm.capture.capturemode = V4L2_MODE_PREVIEW;
	}

	cap->capture_mode = parms->parm.capture.capturemode;

	ret =
	    v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], video, s_parm,
				parms);
	if (ret < 0)
		vin_warn("v4l2 sub device s_parm error!\n");

	ret =
	    v4l2_subdev_call(cap->pipe.sd[VIN_IND_CSI], video, s_parm,
				parms);
	if (ret < 0)
		vin_warn("v4l2 sub device s_parm error!\n");

	if (inst->is_isp_used) {
		ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_ISP], video, s_parm,
					parms);
		if (ret < 0)
			vin_warn("v4l2 sub device s_parm error!\n");
	}

	return ret;
}

int isp_ae_stat_req(struct file *file, struct v4l2_fh *fh,
		    struct isp_stat_buf *ae_buf)
{
	struct vin_vid_cap *cap =
	    container_of(video_devdata(file), struct vin_vid_cap, vdev);
	struct isp_dev *isp = v4l2_get_subdevdata(cap->pipe.sd[VIN_IND_ISP]);
	struct isp_gen_settings *isp_gen = isp->isp_gen;

	int ret = 0;
	ae_buf->buf_size = ISP_STAT_AE_MEM_SIZE;
	ret = copy_to_user(ae_buf->buf, isp_gen->stat.ae_buf, ae_buf->buf_size);
	return ret;
}

int isp_gamma_req(struct file *file, struct v4l2_fh *fh,
		  struct isp_stat_buf *gamma_buf)
{
	/*struct vin_vid_cap *cap =
	    container_of(video_devdata(file), struct vin_vid_cap, vdev);
	struct isp_dev *isp = v4l2_get_subdevdata(cap->pipe.sd[VIN_IND_ISP]);
	struct isp_gen_settings *isp_gen = isp->isp_gen_set_pt;*/

	int ret = 0;
	gamma_buf->buf_size = ISP_GAMMA_MEM_SIZE;
	/*ret = copy_to_user(gamma_buf->buf,
			   isp_gen->isp_ini_cfg.isp_tunning_settings.gamma_tbl_post,
			   gamma_buf->buf_size);
	*/
	return ret;
}

int isp_hist_stat_req(struct file *file, struct v4l2_fh *fh,
		      struct isp_stat_buf *hist_buf)
{
	struct vin_vid_cap *cap =
	    container_of(video_devdata(file), struct vin_vid_cap, vdev);
	struct isp_dev *isp = v4l2_get_subdevdata(cap->pipe.sd[VIN_IND_ISP]);
	struct isp_gen_settings *isp_gen = isp->isp_gen;

	int ret = 0;
	hist_buf->buf_size = ISP_STAT_HIST_MEM_SIZE;
	ret = copy_to_user(hist_buf->buf, isp_gen->stat.hist_buf,
			   hist_buf->buf_size);
	return ret;
}

int isp_af_stat_req(struct file *file, struct v4l2_fh *fh,
		    struct isp_stat_buf *af_buf)
{
	struct vin_vid_cap *cap =
	    container_of(video_devdata(file), struct vin_vid_cap, vdev);
	struct isp_dev *isp = v4l2_get_subdevdata(cap->pipe.sd[VIN_IND_ISP]);
	struct isp_gen_settings *isp_gen = isp->isp_gen;

	int ret = 0;
	af_buf->buf_size = ISP_STAT_AF_MEM_SIZE;

	ret = copy_to_user(af_buf->buf, isp_gen->stat.af_buf, af_buf->buf_size);
	return 0;
}

static int isp_exif_req(struct file *file, struct v4l2_fh *fh,
			struct isp_exif_attribute *exif_attr)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct isp_dev *isp = v4l2_get_subdevdata(cap->pipe.sd[VIN_IND_ISP]);
	struct isp_gen_settings *isp_gen = isp->isp_gen;
	struct sensor_instance *inst = get_valid_sensor(vinc);
	struct sensor_exif_attribute exif;

	if (isp_gen && inst->is_bayer_raw) {

	} else {
		if (v4l2_subdev_call
		    (vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], core, ioctl,
		     GET_SENSOR_EXIF, &exif) != 0) {
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
			exif_attr->exposure_time.numerator =
			    exif.exposure_time_num;
			exif_attr->exposure_time.denominator =
			    exif.exposure_time_den;
			exif_attr->shutter_speed.numerator =
			    exif.exposure_time_num;
			exif_attr->shutter_speed.denominator =
			    exif.exposure_time_den;
		}
	}
	return 0;
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

	memset(&ev, 0, sizeof(ev));
	ev.type = V4L2_EVENT_VIN_CLASS;
	if (vinc->modu_cfg.sensors.inst[0].is_bayer_raw) {
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

static long vin_param_handler(struct file *file, void *priv,
			      bool valid_prio, unsigned int cmd, void *param)
{
	int ret = 0;
	struct v4l2_fh *fh = (struct v4l2_fh *)priv;
	struct isp_stat_buf *stat = (struct isp_stat_buf *)param;

	switch (cmd) {
	case VIDIOC_ISP_AE_STAT_REQ:
		ret = isp_ae_stat_req(file, fh, stat);
		break;
	case VIDIOC_ISP_AF_STAT_REQ:
		ret = isp_af_stat_req(file, fh, stat);
		break;
	case VIDIOC_ISP_HIST_STAT_REQ:
		ret = isp_hist_stat_req(file, fh, stat);
		break;
	case VIDIOC_ISP_EXIF_REQ:
		ret =
		    isp_exif_req(file, fh, (struct isp_exif_attribute *)param);
		break;
	case VIDIOC_ISP_GAMMA_REQ:
		ret = isp_gamma_req(file, fh, stat);
		break;
	case VIDIOC_HDR_CTRL:
		ret = vidioc_hdr_ctrl(file, fh, (struct isp_hdr_ctrl *)param);
		break;
	default:
		ret = -ENOTTY;
	}
	return ret;
}

static int vin_subscribe_event(struct v4l2_fh *fh,
		const struct v4l2_event_subscription *sub)
{
	vin_print("%s id = %d\n", __func__, sub->id);

	if (sub->type == V4L2_EVENT_CTRL)
		return v4l2_ctrl_subscribe_event(fh, sub);
	else
		return v4l2_event_subscribe(fh, sub, 1, NULL);
}

static int vin_unsubscribe_event(struct v4l2_fh *fh,
	const struct v4l2_event_subscription *sub)
{
	vin_print("%s id = %d\n", __func__, sub->id);
	return v4l2_event_unsubscribe(fh, sub);
}

static int vin_video_set_default_format(struct vin_core *vinc)
{

	struct v4l2_format fmt = {
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
		.fmt.pix_mp = {
			.width		= 800,
			.height		= 600,
			.pixelformat	= V4L2_PIX_FMT_YUV420M,
			.field		= V4L2_FIELD_NONE,
			.colorspace	= V4L2_COLORSPACE_JPEG,
		},
	};
	return __vin_set_fmt(vinc, &fmt);
}
static int __vin_sensor_setup_link(struct vin_core *vinc, int i, int en)
{
	struct v4l2_subdev *sensor = NULL;
	struct media_link *link = NULL;
	int ret;
	sensor = vinc->modu_cfg.modules.sensor[i].sd;
	if (sensor == NULL)
		return -1;

	link = &sensor->entity.links[0];
	if (link == NULL)
		return -1;

	vin_print("setup link: [%s] %c> [%s]\n",
		sensor->name, en ? '=' : '-',
		link->sink->entity->name);
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

static int vin_open(struct file *file)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	int valid_idx = vinc->modu_cfg.sensors.valid_idx;
	struct sensor_instance *inst = NULL;
	struct v4l2_control ctrl;
	struct sensor_item sensor_info;
	unsigned long core_clk;
	int ret;

	vin_print("%s\n", __func__);
	if (vin_is_opened(cap)) {
		vin_err("device open busy\n");
		ret = -EBUSY;
		goto open_end;
	}
#ifdef CONFIG_DEVFREQ_DRAM_FREQ_WITH_SOFT_NOTIFY
	dramfreq_master_access(MASTER_CSI, true);
#endif
	if (-1 == valid_idx) {
		vin_err("there is no valid sensor\n");
		ret = -EINVAL;
		goto open_end;
	}

	if (__vin_sensor_setup_link(vinc, valid_idx, 1) < 0) {
		vin_err("sensor setup link failed\n");
		ret = -EINVAL;
		goto open_end;
	}
	inst = &vinc->modu_cfg.sensors.inst[valid_idx];
	pm_runtime_get_sync(&vinc->pdev->dev);	/*call pm_runtime resume*/
	csi_cci_init_helper(vinc->modu_cfg.bus_sel);

	cap->first_flag = 0;
	__vin_start_opened(cap);
	/* create event queue */
	v4l2_fh_open(file);

	/*set vin core clk rate for each sensor!*/
	if (get_sensor_info(inst->cam_name, &sensor_info) == 0)
		core_clk = sensor_info.core_clk_for_sensor;
	else
		core_clk = CSI_CORE_CLK_RATE;

	ret = vin_pipeline_call(vinc, open, &cap->pipe, &cap->vdev.entity, true);
	if (ret < 0) {
		vin_err("vin pipeline open failed (%d)!\n", ret);
		goto open_end;
	}

	v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_CSI], core, ioctl,
			 VIDIOC_SUNXI_CSI_SET_CORE_CLK, &core_clk);

	if (inst->is_isp_used) {
		/*must be after ahb and core clock enable*/
		ret =
		    v4l2_subdev_call(cap->pipe.sd[VIN_IND_ISP], core, init, 1);
		if (ret < 0) {
			vin_err("ISP init error at %s\n", __func__);
			return ret;
		}
	}

	ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core, init, 1);
	if (ret) {
		vin_err("sensor initial error when selecting target device!\n");
		goto open_end;
	}

	if (inst->is_isp_used) {

	} else {
		/* Set the initial flip */
		ctrl.id = V4L2_CID_VFLIP;
		ctrl.value = inst->vflip;
		ret =
		    v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core,
				     s_ctrl, &ctrl);
		if (ret)
			vin_err("sensor sensor_s_ctrl V4L2_CID_VFLIP error!\n");
		ctrl.id = V4L2_CID_HFLIP;
		ctrl.value = inst->hflip;
		ret =
		    v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core,
				     s_ctrl, &ctrl);
		if (ret)
			vin_err("sensor sensor_s_ctrl V4L2_CID_HFLIP error!\n");
	}

	vin_video_set_default_format(vinc);

open_end:
	if (ret)
		vin_print("%s fail!\n", __func__);
	else
		vin_print("%s ok!\n", __func__);
	return ret;
}

static int vin_close(struct file *file)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	int valid_idx = vinc->modu_cfg.sensors.valid_idx;
	struct sensor_instance *inst =
		&vinc->modu_cfg.sensors.inst[valid_idx];
	int ret;
	vin_print("%s\n", __func__);
	if (!vin_is_opened(cap)) {
		vin_warn("device have been closed!\n");
		return 0;
	}
	vin_stop_generating(cap);
	if (vinc->vid_cap.pipe.sd[VIN_IND_ACTUATOR] != NULL)
		v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_ACTUATOR], core,
				 ioctl, ACT_SOFT_PWDN, 0);
	ret = vin_pipeline_call(vinc, close, &cap->pipe);
	if (ret)
		vin_err("vin pipeline close failed!\n");

	v4l2_subdev_call(cap->pipe.sd[VIN_IND_CSI], video, s_stream, 0);

	if (inst->is_isp_used) {
		/*must be after ahb and core clock enable*/
		ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_ISP], core, init, 0);
		if (ret < 0) {
			vin_err("ISP deinit error at %s\n", __func__);
			return ret;
		}
	}
	/*software*/
	vb2_fop_release(file); /*vb2_queue_release(&cap->vb_vidq);*/
	__vin_stop_opened(cap);
	pm_runtime_put_sync(&vinc->pdev->dev); /*call pm_runtime suspend*/
	__vin_sensor_setup_link(vinc, valid_idx, 0);
#ifdef CONFIG_DEVFREQ_DRAM_FREQ_WITH_SOFT_NOTIFY
	dramfreq_master_access(MASTER_CSI, false);
#endif
	vin_print("vin_close end\n");
	return 0;
}

static int vin_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	int ret = 0;
	struct vin_vid_cap *cap =
	    container_of(ctrl->handler, struct vin_vid_cap, ctrl_handler);
	struct vin_core *vinc = cap->vinc;
	struct sensor_instance *inst = get_valid_sensor(vinc);
	struct v4l2_control c;
	c.id = ctrl->id;

	if (inst->is_isp_used && inst->is_bayer_raw) {
		switch (ctrl->id) {
		case V4L2_CID_EXPOSURE:
		case V4L2_CID_GAIN:
		case V4L2_CID_HOR_VISUAL_ANGLE:
		case V4L2_CID_VER_VISUAL_ANGLE:
		case V4L2_CID_FOCUS_LENGTH:
		case V4L2_CID_3A_LOCK:
		case V4L2_CID_AUTO_FOCUS_STATUS: /*Read-Only*/
		case V4L2_CID_SENSOR_TYPE:
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
			ret =
			    v4l2_subdev_call(cap->pipe.sd[VIN_IND_FLASH],
						core, g_ctrl, &c);
			break;
		case V4L2_CID_AUTO_FOCUS_STATUS:
			ret =
			    v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR],
						core, g_ctrl, &c);
			if (c.value != V4L2_AUTO_FOCUS_STATUS_BUSY)
				sunxi_flash_stop(cap->pipe.sd[VIN_IND_FLASH]);
			break;
		default:
			ret =
			    v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR],
						core, g_ctrl, &c);
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
	struct vin_vid_cap *cap =
	    container_of(ctrl->handler, struct vin_vid_cap, ctrl_handler);
	struct vin_core *vinc = cap->vinc;
	struct sensor_instance *inst = get_valid_sensor(vinc);
	int ret = 0;
	unsigned long flags = 0;
	struct actuator_ctrl_word_t vcm_ctrl;
	struct v4l2_control c;
	c.id = ctrl->id;
	c.value = ctrl->val;

	vin_log(VIN_LOG_VIDEO, "s_ctrl: %s, value: %d\n",
		ctrl->name, ctrl->val);

	spin_lock_irqsave(&cap->slock, flags);

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
			break;
		default:
			ret = -EINVAL;
		}
		if (ret < 0)
			vin_warn("v4l2 isp s_ctrl fail!\n");
	} else {
		switch (ctrl->id) {
		case V4L2_CID_FOCUS_ABSOLUTE:
			vcm_ctrl.code = ctrl->val;
			vcm_ctrl.sr = 0x0;
			ret =
			    v4l2_subdev_call(cap->pipe.sd[VIN_IND_ACTUATOR],
						core, ioctl,
						ACT_SET_CODE, &vcm_ctrl);
			break;
		case V4L2_CID_FLASH_LED_MODE:
			ret =
			    v4l2_subdev_call(cap->pipe.sd[VIN_IND_FLASH],
						core, s_ctrl, &c);
			break;
		case V4L2_CID_AUTO_FOCUS_START:
			sunxi_flash_check_to_start(cap->pipe.sd[VIN_IND_FLASH],
						SW_CTRL_TORCH_ON);
			ret =
			    v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR],
						core, s_ctrl, &c);
			break;
		case V4L2_CID_AUTO_FOCUS_STOP:
			sunxi_flash_stop(cap->pipe.sd[VIN_IND_FLASH]);
			ret =
			    v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR],
						core, s_ctrl, &c);
			break;
		case V4L2_CID_AE_WIN_X1:
			ret = __vin_sensor_set_ae_win(cap);
			break;
		case V4L2_CID_AF_WIN_X1:
			ret = __vin_sensor_set_af_win(cap);
			break;
		case V4L2_CID_AUTO_EXPOSURE_BIAS:
			c.value = ctrl->qmenu_int[ctrl->val];
			ret =
			    v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR],
						core, s_ctrl, &c);
			break;
		default:
			ret =
			    v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR],
						core, s_ctrl, &c);
			break;
		}
		if (ret < 0)
			vin_warn("v4l2 sensor s_ctrl fail!\n");
	}
	spin_unlock_irqrestore(&cap->slock, flags);
	return ret;
}

#ifdef CONFIG_COMPAT
struct isp_stat_buf32 {
	compat_caddr_t buf;
	__u32 buf_size;
};
static int get_isp_stat_buf32(struct isp_stat_buf *kp,
			      struct isp_stat_buf32 __user *up)
{
	u32 tmp;
	if (!access_ok(VERIFY_READ, up, sizeof(struct isp_stat_buf32)) ||
	    get_user(kp->buf_size, &up->buf_size) || get_user(tmp, &up->buf))
		return -EFAULT;
	kp->buf = compat_ptr(tmp);
	return 0;
}
static int put_isp_stat_buf32(struct isp_stat_buf *kp,
			      struct isp_stat_buf32 __user *up)
{
	u32 tmp = (u32) ((unsigned long)kp->buf);
	if (!access_ok(VERIFY_WRITE, up, sizeof(struct isp_stat_buf32)) ||
	    put_user(kp->buf_size, &up->buf_size) || put_user(tmp, &up->buf))
		return -EFAULT;
	return 0;
}

#define VIDIOC_ISP_AE_STAT_REQ32 _IOWR('V', BASE_VIDIOC_PRIVATE + 1, struct isp_stat_buf32)
#define VIDIOC_ISP_HIST_STAT_REQ32 _IOWR('V', BASE_VIDIOC_PRIVATE + 2, struct isp_stat_buf32)
#define VIDIOC_ISP_AF_STAT_REQ32 _IOWR('V', BASE_VIDIOC_PRIVATE + 3, struct isp_stat_buf32)
#define VIDIOC_ISP_GAMMA_REQ32 _IOWR('V', BASE_VIDIOC_PRIVATE + 5, struct isp_stat_buf32)

static long vin_compat_ioctl32(struct file *file, unsigned int cmd,
			       unsigned long arg)
{
	union {
		struct isp_stat_buf isb;
	} karg;
	void __user *up = compat_ptr(arg);
	int compatible_arg = 1;
	long err = 0;
	switch (cmd) {
	case VIDIOC_ISP_AE_STAT_REQ32:
		cmd = VIDIOC_ISP_AE_STAT_REQ;
		break;
	case VIDIOC_ISP_HIST_STAT_REQ32:
		cmd = VIDIOC_ISP_HIST_STAT_REQ;
		break;
	case VIDIOC_ISP_AF_STAT_REQ32:
		cmd = VIDIOC_ISP_AF_STAT_REQ;
		break;
	case VIDIOC_ISP_GAMMA_REQ32:
		cmd = VIDIOC_ISP_GAMMA_REQ;
		break;
	}
	switch (cmd) {
	case VIDIOC_ISP_AE_STAT_REQ:
	case VIDIOC_ISP_HIST_STAT_REQ:
	case VIDIOC_ISP_AF_STAT_REQ:
	case VIDIOC_ISP_GAMMA_REQ:
		err = get_isp_stat_buf32(&karg.isb, up);
		compatible_arg = 0;
		break;
	}
	if (err)
		return err;
	if (compatible_arg)
		err = video_ioctl2(file, cmd, (unsigned long)up);
	else {
		mm_segment_t old_fs = get_fs();
		set_fs(KERNEL_DS);
		if (file->f_op->unlocked_ioctl)
			err =
			    file->f_op->unlocked_ioctl(file, cmd,
						       (unsigned long)&karg);
		else
			err = -ENOIOCTLCMD;
		set_fs(old_fs);
	}
	switch (cmd) {
	case VIDIOC_ISP_AE_STAT_REQ:
	case VIDIOC_ISP_HIST_STAT_REQ:
	case VIDIOC_ISP_AF_STAT_REQ:
	case VIDIOC_ISP_GAMMA_REQ:
		if (put_isp_stat_buf32(&karg.isb, up))
			err = -EFAULT;
		break;
	}
	return err;
}
#endif
/* ------------------------------------------------------------------
	File operations for the device
   ------------------------------------------------------------------*/

static const struct v4l2_ctrl_ops vin_ctrl_ops = {
	.g_volatile_ctrl = vin_g_volatile_ctrl,
	.s_ctrl = vin_s_ctrl,
};

static const struct v4l2_file_operations vin_fops = {
	.owner = THIS_MODULE,
	.open = vin_open,
	.release = vin_close,
	.read = vb2_fop_read,
	.poll = vb2_fop_poll,
	.ioctl = video_ioctl2,
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
	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_qbuf = vb2_ioctl_qbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_enum_input = vidioc_enum_input,
	.vidioc_g_input = vidioc_g_input,
	.vidioc_s_input = vidioc_s_input,
	.vidioc_streamon = vidioc_streamon,
	.vidioc_streamoff = vidioc_streamoff,
	.vidioc_g_parm = vidioc_g_parm,
	.vidioc_s_parm = vidioc_s_parm,
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
	50, 100, 200, 400, 800,
};
static const s64 exp_bias_qmenu[] = {
	-4, -3, -2, -1, 0, 1, 2, 3, 4,
};

int vin_init_controls(struct v4l2_ctrl_handler *hdl, struct vin_vid_cap *cap)
{
	struct v4l2_ctrl *ctrl;
	unsigned int i, ret = 0;

	v4l2_ctrl_handler_init(hdl, 37 + ARRAY_SIZE(custom_ctrls)
		+ ARRAY_SIZE(ae_win_ctrls) + ARRAY_SIZE(af_win_ctrls));
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_BRIGHTNESS, 0, 255, 1,
			  128);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_CONTRAST, 0, 128, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_SATURATION, -4, 4, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_HUE, -180, 180, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_AUTO_WHITE_BALANCE, 0, 1,
			  1, 1);
	ctrl =
	    v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_EXPOSURE, 0,
			      65536 * 16, 1, 0);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_AUTOGAIN, 0, 1, 1, 1);
	ctrl =
	    v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_GAIN, 1 * 16,
			      64 * 16 - 1, 1, 1 * 16);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops,
			       V4L2_CID_POWER_LINE_FREQUENCY,
			       V4L2_CID_POWER_LINE_FREQUENCY_AUTO, 0,
			       V4L2_CID_POWER_LINE_FREQUENCY_AUTO);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_HUE_AUTO, 0, 1, 1, 1);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops,
			  V4L2_CID_WHITE_BALANCE_TEMPERATURE, 2800, 10000, 1,
			  6500);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_SHARPNESS, -32, 32, 1,
			  0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_CHROMA_AGC, 0, 1, 1, 1);
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops, V4L2_CID_COLORFX,
			       V4L2_COLORFX_SET_CBCR, 0, V4L2_COLORFX_NONE);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_AUTOBRIGHTNESS, 0, 1, 1,
			  1);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_BAND_STOP_FILTER, 0, 1,
			  1, 1);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_ILLUMINATORS_1, 0, 1, 1,
			  0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_ILLUMINATORS_2, 0, 1, 1,
			  0);
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops, V4L2_CID_EXPOSURE_AUTO,
			       V4L2_EXPOSURE_APERTURE_PRIORITY, 0,
			       V4L2_EXPOSURE_AUTO);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_EXPOSURE_ABSOLUTE, 1,
			  1000000, 1, 1);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_EXPOSURE_AUTO_PRIORITY,
			  0, 1, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_FOCUS_ABSOLUTE, 0, 127,
			  1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_FOCUS_RELATIVE, -127,
			  127, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_FOCUS_AUTO, 0, 1, 1, 1);
	v4l2_ctrl_new_int_menu(hdl, &vin_ctrl_ops, V4L2_CID_AUTO_EXPOSURE_BIAS,
			       ARRAY_SIZE(exp_bias_qmenu) - 1,
			       ARRAY_SIZE(exp_bias_qmenu) / 2, exp_bias_qmenu);
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops,
			       V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE,
			       V4L2_WHITE_BALANCE_SHADE, 0,
			       V4L2_WHITE_BALANCE_AUTO);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_WIDE_DYNAMIC_RANGE, 0, 1,
			  1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_IMAGE_STABILIZATION, 0,
			  1, 1, 0);
	v4l2_ctrl_new_int_menu(hdl, &vin_ctrl_ops, V4L2_CID_ISO_SENSITIVITY,
			       ARRAY_SIZE(iso_qmenu) - 1,
			       ARRAY_SIZE(iso_qmenu) / 2 - 1, iso_qmenu);
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops,
			       V4L2_CID_ISO_SENSITIVITY_AUTO,
			       V4L2_ISO_SENSITIVITY_AUTO, 0,
			       V4L2_ISO_SENSITIVITY_AUTO);
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops, V4L2_CID_SCENE_MODE,
			       V4L2_SCENE_MODE_TEXT, 0, V4L2_SCENE_MODE_NONE);
	ctrl =
	    v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_3A_LOCK, 0, 7, 0, 0);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_AUTO_FOCUS_START, 0, 0,
			  0, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_AUTO_FOCUS_STOP, 0, 0, 0,
			  0);
	ctrl =
	    v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_AUTO_FOCUS_STATUS, 0,
			      7, 0, 0);
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

static struct video_device vin_template[] = {
	[0] = {
	       .name = "vin_video0",
	       .fops = &vin_fops,
	       .ioctl_ops = &vin_ioctl_ops,
	       .release = video_device_release_empty,
	       },
	[1] = {
	       .name = "vin_video1",
	       .fops = &vin_fops,
	       .ioctl_ops = &vin_ioctl_ops,
	       .release = video_device_release_empty,
	       },
};

int vin_init_video(struct v4l2_device *v4l2_dev, struct vin_vid_cap *cap)
{
	int ret = 0;
	struct vb2_queue *q;
	static u64 vin_dma_mask = DMA_BIT_MASK(32);

	cap->vdev = vin_template[cap->vinc->id];
	cap->vdev.ctrl_handler = &cap->ctrl_handler;
	cap->vdev.v4l2_dev = v4l2_dev;
	cap->vdev.queue = &cap->vb_vidq;
	cap->vdev.lock = &cap->buf_lock;
	cap->vdev.flags = V4L2_FL_USES_V4L2_FH;
	ret = video_register_device(&cap->vdev, VFL_TYPE_GRABBER, cap->vinc->id);
	if (ret < 0) {
		vin_err("Error video_register_device!!\n");
		return -1;
	}
	video_set_drvdata(&cap->vdev, cap->vinc);
	vin_print("V4L2 device registered as %s\n",
		video_device_node_name(&cap->vdev));

	/* Initialize videobuf2 queue as per the buffer type */
	cap->vinc->pdev->dev.dma_mask = &vin_dma_mask;
	cap->vinc->pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	cap->alloc_ctx = vb2_dma_contig_init_ctx(&cap->vinc->pdev->dev);
	if (IS_ERR(cap->alloc_ctx)) {
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
	q->timestamp_type = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->lock = &cap->buf_lock;

	ret = vb2_queue_init(q);
	if (ret) {
		vin_err("vb2_queue_init() failed\n");
		vb2_dma_contig_cleanup_ctx(cap->alloc_ctx);
		return ret;
	}

	cap->vd_pad.flags = MEDIA_PAD_FL_SINK;
	ret = media_entity_init(&cap->vdev.entity, 1, &cap->vd_pad, 0);
	if (ret)
		return ret;

	cap->generating = 0;
	cap->opened = 0;
	cap->registered = 1;
	/* initial state */
	cap->capture_mode = V4L2_MODE_PREVIEW;
	/* init video dma queues */
	INIT_LIST_HEAD(&cap->vidq_active);
	mutex_init(&cap->stream_lock);
	mutex_init(&cap->opened_lock);
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
				     struct v4l2_subdev_fh *fh,
				     struct v4l2_subdev_mbus_code_enum *code)
{
	return 0;
}

static int vin_subdev_get_fmt(struct v4l2_subdev *sd,
			      struct v4l2_subdev_fh *fh,
			      struct v4l2_subdev_format *fmt)
{
	return 0;
}

static int vin_subdev_set_fmt(struct v4l2_subdev *sd,
			      struct v4l2_subdev_fh *fh,
			      struct v4l2_subdev_format *fmt)
{
	return 0;
}

static int vin_subdev_get_selection(struct v4l2_subdev *sd,
				    struct v4l2_subdev_fh *fh,
				    struct v4l2_subdev_selection *sel)
{
	return 0;
}

static int vin_subdev_set_selection(struct v4l2_subdev *sd,
				    struct v4l2_subdev_fh *fh,
				    struct v4l2_subdev_selection *sel)
{
	return 0;
}
static int vin_video_core_s_power(struct v4l2_subdev *sd, int on)
{
	struct vin_core *vinc = v4l2_get_subdevdata(sd);
	if (on)
		pm_runtime_get_sync(&vinc->pdev->dev);
	else
		pm_runtime_put_sync(&vinc->pdev->dev);
	return 0;
}

static struct v4l2_subdev_core_ops vin_subdev_core_ops = {
	.s_power = vin_video_core_s_power,
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
	.pad = &vin_subdev_pad_ops,
};

static int vin_capture_subdev_registered(struct v4l2_subdev *sd)
{
	struct vin_core *vinc = v4l2_get_subdevdata(sd);
	int ret;
	vin_print("vin video subdev registered\n");
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
		vin_print("unregistering %s\n",
			video_device_node_name(&vinc->vid_cap.vdev));
		video_unregister_device(&vinc->vid_cap.vdev);
		vb2_dma_contig_cleanup_ctx(vinc->vid_cap.alloc_ctx);
		media_entity_cleanup(&vinc->vid_cap.vdev.entity);
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
	ret = media_entity_init(&sd->entity, VIN_SD_PADS_NUM,
				vinc->vid_cap.sd_pads, 0);
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

