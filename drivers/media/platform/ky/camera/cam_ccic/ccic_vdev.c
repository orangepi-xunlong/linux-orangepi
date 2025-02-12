// SPDX-License-Identifier: GPL-2.0
/*
 * vdev.c - video divece functions
 *
 * Copyright(C) 2019 SPM Micro Limited
 */
#include <media/v4l2-dev.h>
#include <media/media-entity.h>
#include <media/media-device.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf2-v4l2.h>
#include <media/v4l2-ioctl.h>
#include <linux/media-bus-format.h>
#include <linux/compat.h>
#include <media/x1/x1_media_bus_format.h>
#include <media/x1/x1_ccic_uapi.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-contig.h>
#include <linux/pm_runtime.h>
#include <linux/pm_qos.h>
#include "ccic_vdev.h"

#define pr_dbg(format, ...)

static struct {
	__u32 pixelformat;
	__u8 num_planes;
	__u32 pixel_width_align;
	__u32 pixel_height_align;
	__u32 plane_bytes_align[VIDEO_MAX_PLANES];
	struct {
		__u32 num;
		__u32 den;
	}plane_bpp[VIDEO_MAX_PLANES];
	struct {
		__u32 num;
		__u32 den;
	}height_subsampling[VIDEO_MAX_PLANES];
} spm_ccic_formats_table[] = {
	/* bayer raw8 */
	{
		.pixelformat = V4L2_PIX_FMT_SBGGR8,
		.num_planes = 1,
		.pixel_width_align = 1,
		.plane_bytes_align = {
			[0] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 8,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SGBRG8,
		.num_planes = 1,
		.pixel_width_align = 1,
		.plane_bytes_align = {
			[0] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 8,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SGRBG8,
		.num_planes = 1,
		.pixel_width_align = 1,
		.plane_bytes_align = {
			[0] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 8,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SRGGB8,
		.num_planes = 1,
		.pixel_width_align = 1,
		.plane_bytes_align = {
			[0] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 8,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	/* bayer raw10 */
	{
		.pixelformat = V4L2_PIX_FMT_SBGGR10P,
		.num_planes = 1,
		.pixel_width_align = 1,
		.plane_bytes_align = {
			[0] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 10,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SGBRG10P,
		.num_planes = 1,
		.pixel_width_align = 1,
		.plane_bytes_align = {
			[0] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 10,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SGRBG10P,
		.num_planes = 1,
		.pixel_width_align = 1,
		.plane_bytes_align = {
			[0] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 10,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SRGGB10P,
		.num_planes = 1,
		.pixel_width_align = 1,
		.plane_bytes_align = {
			[0] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 10,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	/* bayer raw12 */
	{
		.pixelformat = V4L2_PIX_FMT_SBGGR12P,
		.num_planes = 1,
		.pixel_width_align = 1,
		.plane_bytes_align = {
			[0] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 12,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SGBRG12P,
		.num_planes = 1,
		.pixel_width_align = 1,
		.plane_bytes_align = {
			[0] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 12,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SGRBG12P,
		.num_planes = 1,
		.pixel_width_align = 1,
		.plane_bytes_align = {
			[0] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 12,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SRGGB12P,
		.num_planes = 1,
		.pixel_width_align = 1,
		.plane_bytes_align = {
			[0] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 12,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	/* yuv */
	/* YUYV YUV422 */
	{
		.pixelformat = V4L2_PIX_FMT_YUYV,
		.num_planes = 1,
		.pixel_width_align = 2,
		.plane_bytes_align = {
			[0] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 16,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	/* YVYU YUV422 */
	{
		.pixelformat = V4L2_PIX_FMT_YVYU,
		.num_planes = 1,
		.pixel_width_align = 2,
		.plane_bytes_align = {
			[0] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 16,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
};

static int spm_cvdev_lookup_formats_table(struct v4l2_format *f, int *bit_depth)
{
	struct v4l2_pix_format_mplane *pix_fmt = &f->fmt.pix_mp;
	int loop = 0;

	for (loop = 0; loop < ARRAY_SIZE(spm_ccic_formats_table); loop++) {
		if (spm_ccic_formats_table[loop].pixelformat == pix_fmt->pixelformat) {
			*bit_depth = spm_ccic_formats_table[loop].plane_bpp[0].num;
			break;
		}
	}
	if (loop >= ARRAY_SIZE(spm_ccic_formats_table))
		return -1;

	return 0;
}

void spm_cvdev_fill_v4l2_format(struct v4l2_format *f)
{
	int loop = 0, plane = 0;
	unsigned int width = 0, height = 0, stride = 0;
	struct v4l2_plane_pix_format *plane_fmt = NULL;

	for (loop = 0; loop < ARRAY_SIZE(spm_ccic_formats_table); loop++) {
		if (f->fmt.pix_mp.pixelformat == spm_ccic_formats_table[loop].pixelformat) {
			width = CAM_ALIGN(f->fmt.pix_mp.width, spm_ccic_formats_table[loop].pixel_width_align);
			if (0 == spm_ccic_formats_table[loop].pixel_height_align)
				spm_ccic_formats_table[loop].pixel_height_align = 1;
			height = CAM_ALIGN(f->fmt.pix_mp.height, spm_ccic_formats_table[loop].pixel_height_align);
			pr_dbg("%s width=%u, width_align=%u",__func__ ,width, spm_ccic_formats_table[loop].pixel_width_align);
			f->fmt.pix_mp.num_planes = spm_ccic_formats_table[loop].num_planes;
			for (plane = 0; plane < f->fmt.pix_mp.num_planes; plane++) {
				plane_fmt = &f->fmt.pix_mp.plane_fmt[plane];
				stride = CAM_ALIGN((width * spm_ccic_formats_table[loop].plane_bpp[plane].num) / (spm_ccic_formats_table[loop].plane_bpp[plane].den * 8),
								spm_ccic_formats_table[loop].plane_bytes_align[plane]);
				plane_fmt->sizeimage =
					height * stride * spm_ccic_formats_table[loop].height_subsampling[plane].num / spm_ccic_formats_table[loop].height_subsampling[plane].den;
				plane_fmt->bytesperline = stride;
				pr_dbg("plane%d stride=%u", plane, stride);
			}
			break;
		}
	}
}

static int spm_cvdev_queue_setup(struct vb2_queue *q,
								unsigned int *num_buffers,
								unsigned int *num_planes,
								unsigned int sizes[],
								struct device *alloc_devs[])
{
	struct spm_ccic_vnode *ac_vnode = container_of(q, struct spm_ccic_vnode, buf_queue);
	int loop = 0;

	if (num_buffers && num_planes) {
		*num_planes = ac_vnode->cur_fmt.fmt.pix_mp.num_planes;
		pr_dbg("%s num_buffers=%d num_planes=%d ", __func__, *num_buffers, *num_planes);
		for (loop = 0; loop < *num_planes; loop++) {
			sizes[loop] = ac_vnode->cur_fmt.fmt.pix_mp.plane_fmt[loop].sizeimage;
			pr_dbg("plane%d size=%u ", loop, sizes[loop]);
		}
	}
	else {
		pr_err("%s NULL num_buffers or num_planes\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static void spm_cvdev_wait_prepare(struct vb2_queue *q)
{
	//going to wait sleep, release all locks that may block any vb2 buf/stream functions
	struct spm_ccic_vnode *ac_vnode = container_of(q, struct spm_ccic_vnode, buf_queue);
	mutex_unlock(&ac_vnode->mlock);
}

static void spm_cvdev_wait_finish(struct vb2_queue *q)
{
	//wakeup from wait sleep, reacquire all locks
	struct spm_ccic_vnode *ac_vnode = container_of(q, struct spm_ccic_vnode, buf_queue);
	mutex_lock(&ac_vnode->mlock);
}

static int spm_cvdev_buf_init(struct vb2_buffer *vb)
{
	struct spm_ccic_vbuffer *ac_vb = to_ccic_vbuffer(vb);

	INIT_LIST_HEAD(&ac_vb->list_entry);
	ac_vb->reset_flag = 0;
	return 0;
}

static int spm_cvdev_buf_prepare(struct vb2_buffer *vb)
{
	struct spm_ccic_vbuffer *ac_vb = to_ccic_vbuffer(vb);
	struct spm_ccic_vnode *ac_vnode = container_of(vb->vb2_queue, struct spm_ccic_vnode, buf_queue);

	ac_vb->flags = 0;
	memset(ac_vb->reserved, 0, AC_BUF_RESERVED_DATA_LEN);
	//ac_vb->vb2_v4l2_buf.flags &= ~V4L2_BUF_FLAG_IGNOR;
	ac_vb->vb2_v4l2_buf.flags = 0;
	ac_vb->ac_vnode = ac_vnode;
	return 0;
}

static void spm_cvdev_buf_finish(struct vb2_buffer *vb)
{
}

static void spm_cvdev_buf_cleanup(struct vb2_buffer *vb)
{

}

static int spm_cvdev_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct spm_ccic_vnode *ac_vnode = container_of(q, struct spm_ccic_vnode, buf_queue);
	struct ccic_ctrl *ccic_ctrl = ac_vnode->ccic_dev->ctrl;
	struct ccic_dma *ccic_dma = ac_vnode->ccic_dev->dma;
	struct device *dev = ac_vnode->ccic_dev->dev;
	struct spm_ccic_vbuffer *ac_vb = NULL;
	int ret = 0, csi2idi = 0;

	pr_dbg("%s(%s)", __func__, ac_vnode->name);
	ac_vnode->total_frm = 0;
	ac_vnode->sw_err_frm = 0;
	ac_vnode->hw_err_frm = 0;
	ac_vnode->ok_frm = 0;
	ac_vnode->frame_id = 0;

	if (ac_vnode->ccic_dev->index == 0) {
		csi2idi = CCIC_CSI2IDI0;
	} else {
		csi2idi = CCIC_CSI2IDI1;
	}
	ret = ccic_ctrl->ops->config_csi2idi_mux(ccic_ctrl, ac_vnode->csi2vc, csi2idi, 1);
	if (ret) {
		dev_err(dev, "%s config mux(enable) failed ret=%d\n", __func__, ret);
		return ret;
	}
	if (ac_vnode->ccic_mode == CCIC_MODE_NM) {
		ret = ccic_ctrl->ops->config_csi2_mbus(ccic_ctrl, CCIC_CSI2VC_NM, 0, 0, 0, 0, ac_vnode->lane_num);
	} else if (ac_vnode->ccic_mode == CCIC_MODE_VC) {
		ret = ccic_ctrl->ops->config_csi2_mbus(ccic_ctrl, CCIC_CSI2VC_VC, ac_vnode->main_vc, ac_vnode->sub_vc, 0, 0, ac_vnode->lane_num);
	} else {
		ret = ccic_ctrl->ops->config_csi2_mbus(ccic_ctrl, CCIC_CSI2VC_VCDT,
											ac_vnode->main_vc, ac_vnode->sub_vc,
											ac_vnode->main_dt, ac_vnode->sub_dt, ac_vnode->lane_num);
	}
	if (ret) {
		dev_err(dev, "%s config mbus(enable) lane=%d failed ret=%d\n", __func__,
				ac_vnode->lane_num, ret);
		return ret;
	}
	ccic_dma->ops->src_sel(ccic_dma, ac_vnode->src_sel, ac_vnode->main_ccic_id);
	ccic_dma->ops->ccic_enable(ccic_dma, 1);
	ccic_ctrl->ops->irq_mask(ccic_ctrl, 1);
	ret = spm_cvdev_dq_idle_vbuffer(ac_vnode, &ac_vb);
	if (ret) {
		dev_info(dev, "%s no initial buffer available\n", __func__);
	} else {
		spm_cvdev_q_busy_vbuffer(ac_vnode, ac_vb);
	}
	if (ac_vb) {
		ccic_update_dma_addr(ac_vnode, ac_vb, 0);
		ccic_dma->ops->shadow_ready(ccic_dma);
	}
	ac_vnode->is_streaming = 1;
	return 0;
}

static void spm_cvdev_stop_streaming(struct vb2_queue *q)
{
	struct spm_ccic_vnode *ac_vnode = container_of(q, struct spm_ccic_vnode, buf_queue);
	struct ccic_ctrl *ccic_ctrl = ac_vnode->ccic_dev->ctrl;
	struct ccic_dma *ccic_dma = ac_vnode->ccic_dev->dma;
	int csi2idi = 0;
	unsigned long flags = 0;
	//int ret = 0;

	pr_dbg("%s(%s) enter", __func__, ac_vnode->name);

	pr_notice("%s total_frm(%u) sw_err_frm(%u) hw_err_frm(%u) ok_frm(%u)\n",
			ac_vnode->name, ac_vnode->total_frm, ac_vnode->sw_err_frm, ac_vnode->hw_err_frm, ac_vnode->ok_frm);
	if (ac_vnode->ccic_dev->index == 0) {
		csi2idi = CCIC_CSI2IDI0;
	} else {
		csi2idi = CCIC_CSI2IDI1;
	}
	spin_lock_irqsave(&(ac_vnode->waitq_head.lock), flags);
	wait_event_interruptible_locked_irq(ac_vnode->waitq_head,
										!ac_vnode->in_irq && !ac_vnode->in_tasklet);
	ac_vnode->in_streamoff = 1;
	spin_unlock_irqrestore(&(ac_vnode->waitq_head.lock), flags);
	ccic_ctrl->ops->irq_mask(ccic_ctrl, 0);
	//if (ac_vnode->ccic_mode == CCIC_MODE_NM) {
		ccic_ctrl->ops->config_csi2_mbus(ccic_ctrl, CCIC_CSI2VC_NM, 0, 0, 0, 0, 0);
	//} else {
	//	ccic_ctrl->ops->config_csi2_mbus(ccic_ctrl, CCIC_CSI2VC_VC, ac_vnode->main_vc, ac_vnode->sub_vc, 0);
	//}
	ccic_ctrl->ops->config_csi2idi_mux(ccic_ctrl, ac_vnode->csi2vc, csi2idi, 0);
	ccic_dma->ops->ccic_enable(ccic_dma, 0);
	ac_vnode->is_streaming = 0;
	spin_lock_irqsave(&(ac_vnode->waitq_head.lock), flags);
	ac_vnode->in_streamoff = 0;
	spin_unlock_irqrestore(&(ac_vnode->waitq_head.lock), flags);
	pr_dbg("%s(%s) leave", __func__, ac_vnode->name);
}

static void spm_cvdev_buf_queue(struct vb2_buffer *vb)
{
	unsigned long flags = 0;
	struct spm_ccic_vbuffer *ac_vb = to_ccic_vbuffer(vb);
	struct vb2_queue *buf_queue = vb->vb2_queue;
	struct spm_ccic_vnode *ac_vnode = container_of(buf_queue, struct spm_ccic_vnode, buf_queue);
	//struct ccic_dma *ccic_dma = ac_vnode->ccic_dev->dma;
	//unsigned int v4l2_buf_flags = ac_vnode->v4l2_buf_flags[vb->index];

	spin_lock_irqsave(&ac_vnode->slock, flags);
	atomic_inc(&ac_vnode->queued_buf_cnt);
	list_add_tail(&ac_vb->list_entry, &ac_vnode->queued_list);
	//if (ac_vnode->is_streaming) {
	//	if (__spm_cvdev_busy_list_empty(ac_vnode)) {
	//		__spm_cvdev_dq_idle_vbuffer(ac_vnode, &ac_vb);
	//		if (ac_vb) {
	//			__spm_cvdev_q_busy_vbuffer(ac_vnode, ac_vb);
	//			ccic_update_dma_addr(ac_vnode, ac_vb, 0);
	//			ccic_dma->ops->shadow_ready(ccic_dma);
	//		}
	//	}
	//}
	spin_unlock_irqrestore(&ac_vnode->slock, flags);
}

static struct vb2_ops spm_ccic_vb2_ops = {
	.queue_setup = spm_cvdev_queue_setup,
	.wait_prepare = spm_cvdev_wait_prepare,
	.wait_finish = spm_cvdev_wait_finish,
	.buf_init = spm_cvdev_buf_init,
	.buf_prepare = spm_cvdev_buf_prepare,
	.buf_finish = spm_cvdev_buf_finish,
	.buf_cleanup = spm_cvdev_buf_cleanup,
	.start_streaming = spm_cvdev_start_streaming,
	.stop_streaming = spm_cvdev_stop_streaming,
	.buf_queue = spm_cvdev_buf_queue,
};

static void spm_cvdev_cancel_all_buffers(struct spm_ccic_vnode *ac_vnode)
{
	unsigned long flags = 0;
	struct spm_ccic_vbuffer *pos = NULL, *n = NULL;
	struct vb2_buffer *vb2_buf = NULL;

	spin_lock_irqsave(&ac_vnode->slock, flags);
	list_for_each_entry_safe(pos, n, &ac_vnode->queued_list, list_entry) {
		vb2_buf = &(pos->vb2_v4l2_buf.vb2_buf);
		vb2_buffer_done(vb2_buf, VB2_BUF_STATE_ERROR);
		list_del_init(&pos->list_entry);
		atomic_dec(&ac_vnode->queued_buf_cnt);
	}
	list_for_each_entry_safe(pos, n, &ac_vnode->busy_list, list_entry) {
		vb2_buf = &(pos->vb2_v4l2_buf.vb2_buf);
		vb2_buffer_done(vb2_buf, VB2_BUF_STATE_ERROR);
		list_del_init(&pos->list_entry);
		atomic_dec(&ac_vnode->busy_buf_cnt);
	}
	spin_unlock_irqrestore(&ac_vnode->slock, flags);
}

static int spm_cvdev_vidioc_reqbufs(struct file *file, void *fh, struct v4l2_requestbuffers *b)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_ccic_vnode *ac_vnode = container_of(vnode, struct spm_ccic_vnode, vnode);
	int ret = 0;

	mutex_lock(&ac_vnode->mlock);
	ret = vb2_reqbufs(&ac_vnode->buf_queue, b);
	mutex_unlock(&ac_vnode->mlock);
	return ret;
}

static int spm_cvdev_vidioc_querybuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_ccic_vnode *ac_vnode = container_of(vnode, struct spm_ccic_vnode, vnode);
	int ret = 0;

	mutex_lock(&ac_vnode->mlock);
	ret = vb2_querybuf(&ac_vnode->buf_queue, b);
	mutex_unlock(&ac_vnode->mlock);
	return ret;
}

static int spm_cvdev_vidioc_qbuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_ccic_vnode *ac_vnode = container_of(vnode, struct spm_ccic_vnode, vnode);
	int ret = 0;
	unsigned int i = 0;

	ac_vnode->v4l2_buf_flags[b->index] = b->flags;
	if (!b->m.planes) {
		return -EINVAL;
	}
	for (i = 0; i < b->length; i++) {
		ac_vnode->planes_offset[b->index][i] = b->m.planes[i].data_offset;
	}
	mutex_lock(&ac_vnode->mlock);
	ret = vb2_qbuf(&ac_vnode->buf_queue, vnode->v4l2_dev->mdev, b);
	mutex_unlock(&ac_vnode->mlock);
	return ret;
}

static int spm_cvdev_vidioc_expbuf(struct file *file, void *fh, struct v4l2_exportbuffer *e)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_ccic_vnode *ac_vnode = container_of(vnode, struct spm_ccic_vnode, vnode);
	int ret = 0;

	mutex_lock(&ac_vnode->mlock);
#ifndef MODULE
	ret = vb2_expbuf(&ac_vnode->buf_queue, e);
#endif
	mutex_unlock(&ac_vnode->mlock);
	return ret;
}

static int spm_cvdev_vidioc_dqbuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_ccic_vnode *ac_vnode = container_of(vnode, struct spm_ccic_vnode, vnode);
	int ret = 0;

	mutex_lock(&ac_vnode->mlock);
	ret = vb2_dqbuf(&ac_vnode->buf_queue, b, file->f_flags & O_NONBLOCK);
	mutex_unlock(&ac_vnode->mlock);
	return ret;
}

static int spm_cvdev_vidioc_create_bufs(struct file *file, void *fh, struct v4l2_create_buffers *b)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_ccic_vnode *ac_vnode = container_of(vnode, struct spm_ccic_vnode, vnode);
	int ret = 0;

	mutex_lock(&ac_vnode->mlock);
#ifndef MODULE
	ret = vb2_create_bufs(&ac_vnode->buf_queue, b);
#endif
	mutex_unlock(&ac_vnode->mlock);
	return ret;
}

static int spm_cvdev_vidioc_prepare_buf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_ccic_vnode *ac_vnode = container_of(vnode, struct spm_ccic_vnode, vnode);
	int ret = 0;

	mutex_lock(&ac_vnode->mlock);
	ret = vb2_prepare_buf(&ac_vnode->buf_queue, vnode->v4l2_dev->mdev, b);
	mutex_unlock(&ac_vnode->mlock);
	return ret;
}

static int spm_cvdev_vidioc_streamon(struct file *file, void *fn, enum v4l2_buf_type i)
{
	int ret = 0;
	struct video_device *vnode = video_devdata(file);
	struct spm_ccic_vnode *ac_vnode = container_of(vnode, struct spm_ccic_vnode, vnode);
	mutex_lock(&ac_vnode->mlock);
	ret = vb2_streamon(&ac_vnode->buf_queue, i);
	mutex_unlock(&ac_vnode->mlock);
	return ret;
}

static int spm_cvdev_vidioc_streamoff(struct file *file, void *fn, enum v4l2_buf_type i)
{
	int ret = 0;
	struct video_device *vnode = video_devdata(file);
	struct spm_ccic_vnode *ac_vnode = container_of(vnode, struct spm_ccic_vnode, vnode);
	unsigned long flags = 0;

	pr_dbg("%s(%s) enter", __func__, ac_vnode->name);
	pr_dbg("%s(%s) queued_buf_cnt=%d busy_buf_cnt=%d.", __func__, ac_vnode->name, atomic_read(&ac_vnode->queued_buf_cnt), atomic_read(&ac_vnode->busy_buf_cnt));
	spin_lock_irqsave(&ac_vnode->waitq_head.lock, flags);
	wait_event_interruptible_locked_irq(ac_vnode->waitq_head, !ac_vnode->in_tasklet && !ac_vnode->in_irq);
	ac_vnode->in_streamoff = 1;
	spin_unlock_irqrestore(&ac_vnode->waitq_head.lock, flags);
	pr_dbg("%s tasklet clean", ac_vnode->name);
	mutex_lock(&ac_vnode->mlock);
	pr_dbg("%s cancel all buffers", ac_vnode->name);
	spm_cvdev_cancel_all_buffers(ac_vnode);
	pr_dbg("%s streamoff", ac_vnode->name);
	ret = vb2_streamoff(&ac_vnode->buf_queue, i);
	mutex_unlock(&ac_vnode->mlock);
	spin_lock_irqsave(&ac_vnode->waitq_head.lock, flags);
	ac_vnode->in_streamoff = 0;
	spin_unlock_irqrestore(&ac_vnode->waitq_head.lock, flags);
	pr_dbg("%s(%s) leave", __func__, ac_vnode->name);
	return ret;
}

static int spm_cvdev_vidioc_querycap(struct file *file, void *fh, struct v4l2_capability *cap)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_ccic_vnode *ac_vnode = container_of(vnode, struct spm_ccic_vnode, vnode);

	strlcpy(cap->driver, ac_vnode->name, 16);
	cap->capabilities = V4L2_CAP_DEVICE_CAPS | V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_CAPTURE_MPLANE;
	cap->device_caps = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_CAPTURE_MPLANE;
	return 0;
}

/*
 * static int spm_cvdev_vidioc_enum_fmt_vid_cap_mplane(struct file *file, void *fh, struct v4l2_fmtdesc *f)
 * {
 *     return 0;
 * }
 */

static int spm_cvdev_vidioc_g_fmt_vid_cap_mplane(struct file *file, void *fh, struct v4l2_format *f)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_ccic_vnode *ac_vnode = container_of(vnode, struct spm_ccic_vnode, vnode);

	pr_dbg("get format fourcc code[0x%08x] (%dx%d)",
			ac_vnode->cur_fmt.fmt.pix_mp.pixelformat,
			ac_vnode->cur_fmt.fmt.pix_mp.width,
			ac_vnode->cur_fmt.fmt.pix_mp.height);
	*f = ac_vnode->cur_fmt;
	return 0;
}

static int __spm_cvdev_vidioc_s_fmt_vid_cap_mplane(struct file *file, void *fh, struct v4l2_format *f)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_ccic_vnode *ac_vnode = container_of(vnode, struct spm_ccic_vnode, vnode);
	struct vb2_queue *vb2_queue = &ac_vnode->buf_queue;
	struct ccic_dma *ccic_dma = ac_vnode->ccic_dev->dma;
	struct device *dev = ac_vnode->ccic_dev->dev;
	int ret = 0, bit_depth = 0;
	unsigned int fmt_code = 0, width = 0, height = 0;

	pr_dbg("set format fourcc code[0x%08x] (%dx%d)",
			f->fmt.pix_mp.pixelformat, f->fmt.pix_mp.width, f->fmt.pix_mp.height);
	width = f->fmt.pix_mp.width;
	height = f->fmt.pix_mp.height;
	if (vb2_is_streaming(vb2_queue)) {
		pr_err("%s set format not allowed while streaming on.\n", __func__);
		return -EBUSY;
	}
	ret = spm_cvdev_lookup_formats_table(f, &bit_depth);
	if (ret) {
		pr_err("%s failed to lookup formats table fourcc code[0x%08x]\n", __func__, f->fmt.pix_mp.pixelformat);
		return ret;
	}

	if (bit_depth == 8) {
		fmt_code = MEDIA_BUS_FMT_SBGGR8_1X8;
	} else if (bit_depth == 10) {
		fmt_code = MEDIA_BUS_FMT_SBGGR10_1X10;
	} else if (bit_depth == 12) {
		fmt_code = MEDIA_BUS_FMT_SBGGR12_1X12;
	} else if (bit_depth == 16) {
		fmt_code = MEDIA_BUS_FMT_UYVY8_2X8;
		width *= 2;
	} else {
		dev_err(dev, "unknown bit_depth=%d\n", bit_depth);
		return -EINVAL;
	}
	ret = ccic_dma->ops->set_fmt(ccic_dma, width, height, fmt_code);
	if (ret) {
		dev_err(dev, "%s set fmt(%ux%u code:0x%08x) failed\n", __func__,
				width, height, fmt_code);
		return ret;
	}
	spm_cvdev_fill_v4l2_format(f);
	ac_vnode->cur_fmt = *f;

	return 0;
}

static int spm_cvdev_vidioc_s_fmt_vid_cap_mplane(struct file *file, void *fh, struct v4l2_format *f)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_ccic_vnode *ac_vnode = container_of(vnode, struct spm_ccic_vnode, vnode);
	int ret = 0;

	mutex_lock(&ac_vnode->mlock);
	ret = __spm_cvdev_vidioc_s_fmt_vid_cap_mplane(file, fh, f);
	mutex_unlock(&ac_vnode->mlock);
	return ret;
}

static int spm_cvdev_vidioc_try_fmt_vid_cap_mplane(struct file *file, void *fh, struct v4l2_format *f)
{
	return 0;
}

static long spm_cvdev_vidioc_default(struct file *file,
									void *fh,
									bool valid_prio,
									unsigned int cmd,
									void *arg)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_ccic_vnode *ac_vnode = container_of(vnode, struct spm_ccic_vnode, vnode);
	struct device *dev = ac_vnode->ccic_dev->dev;
	struct v4l2_ccic_params *ccic_params = NULL;
	//int ret = 0;

	switch (cmd) {
	case VIDIOC_CCIC_S_PARAMS:
		ccic_params = (struct v4l2_ccic_params*)arg;
		if (ac_vnode->is_streaming) {
			dev_err(dev, "%s set params failed, device is busy now\n", ac_vnode->name);
			return -EBUSY;
		}
		ac_vnode->lane_num = ccic_params->lane_num;
		ac_vnode->ccic_mode = ccic_params->ccic_mode;
		ac_vnode->ch_mode = ccic_params->ch_mode;
		ac_vnode->main_vc = ccic_params->main_vc;
		ac_vnode->sub_vc = ccic_params->sub_vc;
		ac_vnode->main_dt = ccic_params->main_dt;
		ac_vnode->sub_dt = ccic_params->sub_dt;
		ac_vnode->main_ccic_id = ccic_params->main_ccic_id;
		if (ac_vnode->ch_mode == CCIC_CH_MODE_MAIN) {
			if (ac_vnode->ccic_mode == CCIC_MODE_NM) {
				ac_vnode->src_sel = CCIC_DMA_SEL_LOCAL_MAIN;
			} else {
				ac_vnode->src_sel = CCIC_DMA_SEL_LOCAL_VCDT;
			}
		} else {
			ac_vnode->src_sel = CCIC_DMA_SEL_REMOTE_VCDT;
		}
		dev_info(dev, "%s set mipi lane:%u ccic_mode=%d ch_mode=%d main_vc=%u sub_vc=%u main_dt=%u sub_dt=%u main_ccic_id=%u\n",
				ac_vnode->name, ccic_params->lane_num, ccic_params->ccic_mode,
				ccic_params->ch_mode, ccic_params->main_vc, ccic_params->sub_vc,
				ccic_params->main_dt, ccic_params->sub_dt, ccic_params->main_ccic_id);
		break;
	default:
		pr_err("unknown ioctl cmd(%d)\n", cmd);
		return -ENOIOCTLCMD;
	}

	return 0;
}

static struct v4l2_ioctl_ops spm_ccic_v4l2_ioctl_ops = {
	/* VIDIOC_QUERYCAP handler */
	.vidioc_querycap = spm_cvdev_vidioc_querycap,
	/* VIDIOC_ENUM_FMT handlers */
	/* .vidioc_enum_fmt_vid_cap = spm_cvdev_vidioc_enum_fmt_vid_cap_mplane, */
	/* VIDIOC_G_FMT handlers */
	.vidioc_g_fmt_vid_cap_mplane = spm_cvdev_vidioc_g_fmt_vid_cap_mplane,
	/* VIDIOC_S_FMT handlers */
	.vidioc_s_fmt_vid_cap_mplane = spm_cvdev_vidioc_s_fmt_vid_cap_mplane,
	/* VIDIOC_TRY_FMT handlers */
	.vidioc_try_fmt_vid_cap_mplane = spm_cvdev_vidioc_try_fmt_vid_cap_mplane,
	/* Buffer handlers */
	.vidioc_reqbufs = spm_cvdev_vidioc_reqbufs,
	.vidioc_querybuf = spm_cvdev_vidioc_querybuf,
	.vidioc_qbuf = spm_cvdev_vidioc_qbuf,
	.vidioc_expbuf = spm_cvdev_vidioc_expbuf,
	.vidioc_dqbuf = spm_cvdev_vidioc_dqbuf,
	.vidioc_create_bufs = spm_cvdev_vidioc_create_bufs,
	.vidioc_prepare_buf = spm_cvdev_vidioc_prepare_buf,
	.vidioc_streamon = spm_cvdev_vidioc_streamon,
	.vidioc_streamoff = spm_cvdev_vidioc_streamoff,
	.vidioc_default = spm_cvdev_vidioc_default,
};

static int spm_cvdev_open(struct file *file)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_ccic_vnode *ac_vnode = container_of(vnode, struct spm_ccic_vnode, vnode);
	struct ccic_ctrl *ccic_ctrl = ac_vnode->ccic_dev->ctrl;
	struct ccic_dma *ccic_dma = ac_vnode->ccic_dev->dma;
	struct device *dev = ac_vnode->ccic_dev->dev;
	int ret = 0;
	pr_dbg("%s in, open vnode(%s - %s).", __func__, ac_vnode->name, video_device_node_name(vnode));

	if (atomic_inc_return(&ac_vnode->ref_cnt) != 1) {
		pr_err("vnode(%s - %s) was already openned.\n", ac_vnode->name, video_device_node_name(vnode));
		atomic_dec(&ac_vnode->ref_cnt);
		return -EBUSY;
	}
	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		dev_err(dev, "%s rpm get failed ret=%d\n", ac_vnode->name, ret);
		return ret;
	}
	ccic_ctrl->ops->clk_enable(ccic_ctrl, 1);
	ccic_dma->ops->clk_enable(ccic_dma, 1);
	pr_dbg("%s exit, open vnode(%s - %s).", __func__, ac_vnode->name, video_device_node_name(vnode));
	return 0;
}

static void __spm_cvdev_close(struct spm_ccic_vnode *ac_vnode)
{
	unsigned long flags = 0;

	pr_dbg("%s(%s) enter", __func__, ac_vnode->name);
	pr_dbg("%s(%s) queued_buf_cnt=%d busy_buf_cnt=%d.", __func__, ac_vnode->name, atomic_read(&ac_vnode->queued_buf_cnt), atomic_read(&ac_vnode->busy_buf_cnt));
	spin_lock_irqsave(&ac_vnode->waitq_head.lock, flags);
	ac_vnode->in_streamoff = 1;
	wait_event_interruptible_locked_irq(ac_vnode->waitq_head, !ac_vnode->in_tasklet && !ac_vnode->in_irq);
	spin_unlock_irqrestore(&ac_vnode->waitq_head.lock, flags);
	pr_dbg("%s tasklet clean", ac_vnode->name);
	mutex_lock(&ac_vnode->mlock);
	pr_dbg("%s cancel all buffers", ac_vnode->name);
	spm_cvdev_cancel_all_buffers(ac_vnode);
	pr_dbg("%s queue release", ac_vnode->name);
	vb2_queue_release(&ac_vnode->buf_queue);
	ac_vnode->buf_queue.owner = NULL;
	ac_vnode->is_streaming = 0;
	mutex_unlock(&ac_vnode->mlock);
	spin_lock_irqsave(&ac_vnode->waitq_head.lock, flags);
	ac_vnode->in_streamoff = 0;
	spin_unlock_irqrestore(&ac_vnode->waitq_head.lock, flags);
	pr_dbg("%s(%s) leave", __func__, ac_vnode->name);
}

static int spm_cvdev_close(struct file *file)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_ccic_vnode *ac_vnode = container_of(vnode, struct spm_ccic_vnode, vnode);
	struct ccic_ctrl *ccic_ctrl = ac_vnode->ccic_dev->ctrl;
	struct ccic_dma *ccic_dma = ac_vnode->ccic_dev->dma;
	struct device *dev = ac_vnode->ccic_dev->dev;

	if (atomic_dec_and_test(&ac_vnode->ref_cnt)) {
		__spm_cvdev_close(ac_vnode);
		ccic_dma->ops->clk_enable(ccic_dma, 0);
		ccic_ctrl->ops->clk_enable(ccic_ctrl, 0);
		pm_runtime_put_sync(dev);
	}

	return v4l2_fh_release(file);
}

static __poll_t spm_cvdev_poll(struct file *file, struct poll_table_struct *wait)
{
	__poll_t ret;
	struct video_device *vnode = video_devdata(file);
	struct spm_ccic_vnode *ac_vnode = container_of(vnode, struct spm_ccic_vnode, vnode);

	ret = vb2_poll(&ac_vnode->buf_queue, file, wait);

	return ret;
}

#ifdef SPM_CONFIG_COMPAT

static int alloc_userspace(unsigned int size, u32 aux_space,
			   void __user **new_p64)
{
	*new_p64 = compat_alloc_user_space(size + aux_space);
	if (!*new_p64)
		return -ENOMEM;
	if (clear_user(*new_p64, size))
		return -EFAULT;
	return 0;
}

long spm_cvdev_compat_ioctl32(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *p32 = compat_ptr(arg);
	void __user *new_p64 = NULL;
	//void __user *aux_buf;
	//u32 aux_space;
	long err = 0;
	const size_t ioc_size = _IOC_SIZE(cmd);
	//size_t ioc_size64 = 0;

	//if (_IOC_TYPE(cmd) == 'V') {
	//	switch (_IOC_NR(cmd)) {
	//		//int r
	//		case _IOC_NR(VIDIOC_G_SLICE_MODE):
	//		case _IOC_NR(VIDIOC_CPU_Z1):
	//		//int w
	//		case _IOC_NR(VIDIOC_PUT_PIPELINE):
	//		case _IOC_NR(VIDIOC_RESET_PIPELINE):
	//		ioc_size64 = sizeof(int);
	//		break;
	//		//unsigned int
	//		case _IOC_NR(VIDIOC_G_PIPE_STATUS):
	//		ioc_size64 = sizeof(int);
	//		break;
	//		case _IOC_NR(VIDIOC_S_PORT_CFG):
	//		ioc_size64 = sizeof(struct v4l2_vi_port_cfg);
	//		break;
	//		case _IOC_NR(VIDIOC_DBG_REG_WRITE):
	//		case _IOC_NR(VIDIOC_DBG_REG_READ):
	//		ioc_size64 = sizeof(struct v4l2_vi_dbg_reg);
	//		break;
	//		case _IOC_NR(VIDIOC_CFG_INPUT_INTF):
	//		ioc_size64 = sizeof(struct v4l2_vi_input_interface);
	//		break;
	//		case _IOC_NR(VIDIOC_SET_SELECTION):
	//		ioc_size64 = sizeof(struct v4l2_vi_selection);
	//		break;
	//		case _IOC_NR(VIDIOC_QUERY_SLICE_READY):
	//		ioc_size64 = sizeof(struct v4l2_vi_slice_info);
	//		break;
	//		case _IOC_NR(VIDIOC_S_BANDWIDTH):
	//		ioc_size64 = sizeof(struct v4l2_vi_bandwidth_info);
	//		break;
	//		case _IOC_NR(VIDIOC_G_ENTITY_INFO):
	//		ioc_size64 = sizeof(struct v4l2_vi_entity_info);
	//		break;
	//	}
	//	pr_dbg("%s cmd_nr=%d ioc_size32=%u ioc_size64=%u",__func__,  _IOC_NR(cmd), ioc_size, ioc_size64);
	//}
	if (_IOC_DIR(cmd) != _IOC_NONE) {
		err = alloc_userspace(ioc_size, 0, &new_p64);
		if (err) {
			pr_err("%s alloc userspace failed err=%l cmd=%d ioc_size=%u\n", __func__, err, _IOC_NR(cmd), ioc_size);
			return err;
		}
		if ((_IOC_DIR(cmd) & _IOC_WRITE)) {
			err = copy_in_user(new_p64, p32, ioc_size);
			if (err) {
				pr_err("%s copy in user 1 failed err=%l cmd=%d ioc_size=%u\n", __func__, err, _IOC_NR(cmd), ioc_size);
				return err;
			}
		}
	}

	err = video_ioctl2(file, cmd, (unsigned long)new_p64);
	if (err) {
		return err;
	}

	if ((_IOC_DIR(cmd) & _IOC_READ)) {
		err = copy_in_user(p32, new_p64, ioc_size);
		if (err) {
			pr_err("%s copy in user 2 failed err=%l cmd=%d ioc_size=%u\n", __func__, err, _IOC_NR(cmd), ioc_size);
			return err;
		}
	}

	//switch (cmd) {
	//	//int r
	//	case VIDIOC_G_SLICE_MODE:
	//	case VIDIOC_CPU_Z1:
	//	//int w
	//	case VIDIOC_PUT_PIPELINE:
	//	case VIDIOC_RESET_PIPELINE:
	//	err = alloc_userspace(sizeof(int), 0, &new_p64);
	//	if (!err && assign_in_user((int __user *)new_p64,
	//				   (compat_int_t __user *)p32))
	//		err = -EFAULT;
	//	break;
	//	//unsigned int
	//	case VIDIOC_G_PIPE_STATUS:
	//	err = alloc_userspace(sizeof(unsigned int), 0, &new_p64);
	//	if (!err && assign_in_user((unsigned int __user *)new_p64,
	//				   (compat_uint_t __user *)p32))
	//		err = -EFAULT;
	//	break;
	//	case VIDIOC_S_PORT_CFG:
	//	err = alloc_userspace(sizeof(struct v4l2_vi_port_cfg), 0, &new_p64);
	//	if (!err) {
	//		err = -EFAULT;
	//		break;
	//	}
	//	break;
	//	case VIDIOC_DBG_REG_WRITE:
	//	case VIDIOC_DBG_REG_READ:
	//	break;
	//	case VIDIOC_CFG_INPUT_INTF:
	//	break;
	//	case VIDIOC_SET_SELECTION:
	//	break;
	//	case VIDIOC_QUERY_SLICE_READY:
	//	break;
	//	case VIDIOC_S_BANDWIDTH:
	//	break;
	//	case VIDIOC_G_ENTITY_INFO:
	//	break;

	//}
	//if (err)
	//	return err;
	return 0;
}
#endif

static struct v4l2_file_operations spm_ccic_file_operations = {
	.owner = THIS_MODULE,
	.poll = spm_cvdev_poll,
	.unlocked_ioctl = video_ioctl2,
	.open = spm_cvdev_open,
	.release = spm_cvdev_close,
#ifdef SPM_CONFIG_COMPAT
	.compat_ioctl32 = spm_cvdev_compat_ioctl32,
#endif
};

static void spm_cvdev_release(struct video_device *vdev)
{
	struct spm_ccic_vnode *ac_vnode = container_of(vdev, struct spm_ccic_vnode, vnode);

	pr_dbg("%s(%s %s) enter.", __func__, ac_vnode->name, video_device_node_name(&ac_vnode->vnode));
	mutex_destroy(&ac_vnode->mlock);
}
/*
static void spm_cvdev_block_release(struct spm_ccic_block *b)
{
	struct spm_ccic_vnode *ac_vnode = container_of(b, struct spm_ccic_vnode, ac_block);

	pr_dbg("%s(%s %s) enter.", __func__, ac_vnode->name, video_device_node_name(&ac_vnode->vnode));
	vb2_queue_release(&ac_vnode->buf_queue);
	video_unregister_device(&ac_vnode->vnode);
}
*/

void spm_cvdev_destroy_vnode(struct spm_ccic_vnode *ac_vnode)
{
	video_unregister_device(&ac_vnode->vnode);
}

struct spm_ccic_vnode* spm_cvdev_create_vnode(const char *name,
							unsigned int idx,
							struct v4l2_device *v4l2_dev,
							struct device *alloc_dev,
							struct ccic_dev *ccic_dev,
							void (*dma_tasklet_handler)(unsigned long),
							unsigned int min_buffers_needed)
{
	int ret = 0, i = 0;
	struct spm_ccic_vnode *ac_vnode = NULL;
	struct ccic_dma_context *dma_ctx = NULL;
	struct ccic_dma_work_struct *ccic_dma_work = NULL;

	if (NULL == name || NULL == v4l2_dev || NULL == alloc_dev || NULL == ccic_dev) {
		pr_err("%s invalid arguments.\n", __func__);
		return NULL;
	}
	ac_vnode = devm_kzalloc(alloc_dev, sizeof(*ac_vnode), GFP_KERNEL);
	if (NULL == ac_vnode) {
		pr_err("%s failed to alloc mem for spm_ccic_vnode(%s).\n", __func__, name);
		return NULL;
	}
	dma_ctx = &ac_vnode->dma_ctx;
	dma_ctx->ac_vnode = ac_vnode;
	INIT_LIST_HEAD(&dma_ctx->dma_work_idle_list);
	INIT_LIST_HEAD(&dma_ctx->dma_work_busy_list);
	spin_lock_init(&dma_ctx->slock);
	for (i = 0; i < CCIC_DMA_WORK_MAX_CNT; i++) {
		ccic_dma_work = devm_kzalloc(alloc_dev, sizeof(*ccic_dma_work), GFP_KERNEL);
		if (!ccic_dma_work) {
			dev_err(alloc_dev, "%s not enough mem\n", __func__);
			return NULL;
		}
		tasklet_init(&ccic_dma_work->dma_tasklet, dma_tasklet_handler, (unsigned long)ccic_dma_work);
		INIT_LIST_HEAD(&ccic_dma_work->idle_list_entry);
		INIT_LIST_HEAD(&ccic_dma_work->busy_list_entry);
		ccic_dma_work->ac_vnode = ac_vnode;
		list_add(&ccic_dma_work->idle_list_entry, &dma_ctx->dma_work_idle_list);
	}
	ac_vnode->csi2vc = CCIC_CSI2VC_MAIN;
	ac_vnode->src_sel = CCIC_DMA_SEL_LOCAL_MAIN;
	ac_vnode->lane_num = 1;
	ac_vnode->in_streamoff = 0;
	ac_vnode->in_irq = 0;
	ac_vnode->in_tasklet = 0;
	INIT_LIST_HEAD(&ac_vnode->queued_list);
	INIT_LIST_HEAD(&ac_vnode->busy_list);
	atomic_set(&ac_vnode->queued_buf_cnt, 0);
	atomic_set(&ac_vnode->busy_buf_cnt, 0);
	spin_lock_init(&ac_vnode->slock);
	mutex_init(&ac_vnode->mlock);
	init_waitqueue_head(&ac_vnode->waitq_head);
	ac_vnode->idx = idx;
	ac_vnode->buf_queue.timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC | V4L2_BUF_FLAG_TSTAMP_SRC_SOE;
	ac_vnode->buf_queue.buf_struct_size = sizeof(struct spm_ccic_vbuffer);
	ac_vnode->buf_queue.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	ac_vnode->buf_queue.io_modes = VB2_DMABUF;
	ac_vnode->buf_queue.ops = &spm_ccic_vb2_ops;
	ac_vnode->buf_queue.mem_ops = &vb2_dma_contig_memops;
	ac_vnode->buf_queue.min_buffers_needed = min_buffers_needed;
	ac_vnode->buf_queue.dev = alloc_dev;
	ret = vb2_queue_init(&ac_vnode->buf_queue);
	if (ret) {
		pr_err("%s vb2_queue_init failed for spm_ccic_vnode(%s).\n", __func__, name);
		goto queue_init_fail;
	}

	strlcpy(ac_vnode->vnode.name, name, 32);
	strlcpy(ac_vnode->name, name, 32);
	ac_vnode->ccic_dev = ccic_dev;
	ac_vnode->vnode.queue = &ac_vnode->buf_queue;
	ac_vnode->vnode.fops = &spm_ccic_file_operations;
	ac_vnode->vnode.ioctl_ops = &spm_ccic_v4l2_ioctl_ops;
	ac_vnode->vnode.release = spm_cvdev_release;
	ac_vnode->vnode.device_caps = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_CAPTURE_MPLANE;
	ac_vnode->vnode.v4l2_dev = v4l2_dev;
	ret = __video_register_device(&ac_vnode->vnode, VFL_TYPE_VIDEO, -1, 1, THIS_MODULE);
	if (ret) {
		pr_err("%s video dev register failed for spm_ccic_vnode(%s).\n", __func__, name);
		goto vdev_register_fail;
	}
	ccic_dev->vnode = ac_vnode;
	pr_dbg("create vnode(%s - %s) successfully.", name, video_device_node_name(&ac_vnode->vnode));
	return ac_vnode;
vdev_register_fail:
	vb2_queue_release(&ac_vnode->buf_queue);
queue_init_fail:
	devm_kfree(alloc_dev, ac_vnode);
	return NULL;
}

int __spm_cvdev_dq_idle_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer **ac_vb)
{
	*ac_vb = list_first_entry_or_null(&ac_vnode->queued_list, struct spm_ccic_vbuffer, list_entry);
	if (NULL == *ac_vb)
		return -1;
	list_del_init(&(*ac_vb)->list_entry);
	atomic_dec(&ac_vnode->queued_buf_cnt);
	return 0;
}

int __spm_cvdev_q_idle_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer *ac_vb)
{
	list_add_tail(&ac_vb->list_entry, &ac_vnode->queued_list);
	atomic_inc(&ac_vnode->queued_buf_cnt);
	return 0;
}

int spm_cvdev_dq_idle_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer **ac_vb)
{
	unsigned long flags = 0;
	int ret = 0;

	spin_lock_irqsave(&ac_vnode->slock, flags);
	ret = __spm_cvdev_dq_idle_vbuffer(ac_vnode, ac_vb);
	spin_unlock_irqrestore(&ac_vnode->slock, flags);
	return ret;
}

int spm_cvdev_q_idle_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer *ac_vb)
{
	unsigned long flags = 0;
	int ret = 0;

	spin_lock_irqsave(&ac_vnode->slock, flags);
	ret = __spm_cvdev_q_idle_vbuffer(ac_vnode, ac_vb);
	spin_unlock_irqrestore(&ac_vnode->slock, flags);

	return ret;
}

int spm_cvdev_pick_idle_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer **ac_vb)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&ac_vnode->slock, flags);
	*ac_vb = list_first_entry_or_null(&ac_vnode->queued_list, struct spm_ccic_vbuffer, list_entry);
	spin_unlock_irqrestore(&ac_vnode->slock, flags);
	if (NULL == *ac_vb) {
		return -1;
	}
	return 0;
}

int __spm_cvdev_pick_idle_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer **ac_vb)
{
	*ac_vb = list_first_entry_or_null(&ac_vnode->queued_list, struct spm_ccic_vbuffer, list_entry);
	if (NULL == *ac_vb) {
		return -1;
	}
	return 0;
}

int __spm_cvdev_dq_busy_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer **ac_vb)
{
	*ac_vb = list_first_entry_or_null(&ac_vnode->busy_list, struct spm_ccic_vbuffer, list_entry);
	if (NULL == *ac_vb)
		return -1;
	list_del_init(&(*ac_vb)->list_entry);
	atomic_dec(&ac_vnode->busy_buf_cnt);
	return 0;
}

int __spm_cvdev_q_busy_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer *ac_vb)
{
	list_add_tail(&ac_vb->list_entry, &ac_vnode->busy_list);
	atomic_inc(&ac_vnode->busy_buf_cnt);
	return 0;
}

int spm_cvdev_dq_busy_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer **ac_vb)
{
	unsigned long flags = 0;
	int ret = 0;

	spin_lock_irqsave(&ac_vnode->slock, flags);
	ret = __spm_cvdev_dq_busy_vbuffer(ac_vnode, ac_vb);
	spin_unlock_irqrestore(&ac_vnode->slock, flags);
	return ret;
}

int spm_cvdev_pick_busy_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer **ac_vb)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&ac_vnode->slock, flags);
	*ac_vb = list_first_entry_or_null(&ac_vnode->busy_list, struct spm_ccic_vbuffer, list_entry);
	spin_unlock_irqrestore(&ac_vnode->slock, flags);
	if (NULL == *ac_vb)
		return -1;

	return 0;
}

int __spm_cvdev_pick_busy_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer **ac_vb)
{
	*ac_vb = list_first_entry_or_null(&ac_vnode->busy_list, struct spm_ccic_vbuffer, list_entry);
	if (NULL == *ac_vb)
		return -1;

	return 0;
}

int spm_cvdev_q_busy_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer *ac_vb)
{
	unsigned long flags = 0;
	int ret = 0;

	spin_lock_irqsave(&ac_vnode->slock, flags);
	ret = __spm_cvdev_q_busy_vbuffer(ac_vnode, ac_vb);
	spin_unlock_irqrestore(&ac_vnode->slock, flags);
	return ret;
}

int spm_cvdev_export_ccic_vbuffer(struct spm_ccic_vbuffer *ac_vb, int with_error)
{
	struct vb2_buffer *vb = &ac_vb->vb2_v4l2_buf.vb2_buf;
	if (with_error)
		vb2_buffer_done(vb, VB2_BUF_STATE_ERROR);
	else
		vb2_buffer_done(vb, VB2_BUF_STATE_DONE);
	return 0;
}

int __spm_cvdev_busy_list_empty(struct spm_ccic_vnode *ac_vnode)
{
	return list_empty(&ac_vnode->busy_list);
}

int spm_cvdev_busy_list_empty(struct spm_ccic_vnode *ac_vnode)
{
	unsigned long flags = 0;
	int ret = 0;

	spin_lock_irqsave(&ac_vnode->slock, flags);
	ret = __spm_cvdev_busy_list_empty(ac_vnode);
	spin_unlock_irqrestore(&ac_vnode->slock, flags);
	return ret;
}

int __spm_cvdev_idle_list_empty(struct spm_ccic_vnode *ac_vnode)
{
	return list_empty(&ac_vnode->queued_list);
}

int spm_cvdev_idle_list_empty(struct spm_ccic_vnode *ac_vnode)
{
	unsigned long flags = 0;
	int ret = 0;

	spin_lock_irqsave(&ac_vnode->slock, flags);
	ret = __spm_cvdev_idle_list_empty(ac_vnode);
	spin_unlock_irqrestore(&ac_vnode->slock, flags);
	return ret;
}
