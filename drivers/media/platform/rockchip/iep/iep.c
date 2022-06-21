// SPDX-License-Identifier: GPL-2.0-only
/*
 * Rockchip Image Enhancement Processor (IEP) driver
 *
 * Copyright (C) 2020 Alex Bee <knaerzche@gmail.com>
 *
 * Based on Allwinner sun8i deinterlacer with scaler driver
 *  Copyright (C) 2019 Jernej Skrabec <jernej.skrabec@siol.net>
 *
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-contig.h>
#include <linux/videodev2.h>

#include "iep-regs.h"
#include "iep.h"

static struct iep_fmt formats[] = {
	{
		.fourcc = V4L2_PIX_FMT_NV12,
		.color_swap = IEP_YUV_SWP_SP_UV,
		.hw_format = IEP_COLOR_FMT_YUV420,
		.depth = 12,
		.uv_factor = 4,
	},
	{
		.fourcc = V4L2_PIX_FMT_NV21,
		.color_swap = IEP_YUV_SWP_SP_VU,
		.hw_format = IEP_COLOR_FMT_YUV420,
		.depth = 12,
		.uv_factor = 4,
	},
	{
		.fourcc = V4L2_PIX_FMT_NV16,
		.color_swap = IEP_YUV_SWP_SP_UV,
		.hw_format = IEP_COLOR_FMT_YUV422,
		.depth = 16,
		.uv_factor = 2,
	},
	{
		.fourcc = V4L2_PIX_FMT_NV61,
		.color_swap = IEP_YUV_SWP_SP_VU,
		.hw_format = IEP_COLOR_FMT_YUV422,
		.depth = 16,
		.uv_factor = 2,
	},
	{
		.fourcc = V4L2_PIX_FMT_YUV420,
		.color_swap = IEP_YUV_SWP_P,
		.hw_format = IEP_COLOR_FMT_YUV420,
		.depth = 12,
		.uv_factor = 4,
	},
	{
		.fourcc = V4L2_PIX_FMT_YUV422P,
		.color_swap = IEP_YUV_SWP_P,
		.hw_format = IEP_COLOR_FMT_YUV422,
		.depth = 16,
		.uv_factor = 2,
	},
};

static struct iep_fmt *iep_fmt_find(struct v4l2_pix_format *pix_fmt)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(formats); i++) {
		if (formats[i].fourcc == pix_fmt->pixelformat)
			return &formats[i];
	}

	return NULL;
}

static bool iep_check_pix_format(u32 pixelformat)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(formats); i++)
		if (formats[i].fourcc == pixelformat)
			return true;

	return false;
}

static struct vb2_v4l2_buffer *iep_m2m_next_dst_buf(struct iep_ctx *ctx)
{
	struct vb2_v4l2_buffer *dst_buf = v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx);

	/* application has set a dst sequence: take it as start point */
	if (ctx->dst_sequence == 0 && dst_buf->sequence > 0)
		ctx->dst_sequence = dst_buf->sequence;

	dst_buf->sequence = ctx->dst_sequence++;

	return dst_buf;
}

static void iep_m2m_dst_bufs_done(struct iep_ctx *ctx, enum vb2_buffer_state state)
{
	if (ctx->dst0_buf) {
		v4l2_m2m_buf_done(ctx->dst0_buf, state);
		ctx->dst_buffs_done++;
		ctx->dst0_buf = NULL;
	}

	if (ctx->dst1_buf) {
		v4l2_m2m_buf_done(ctx->dst1_buf, state);
		ctx->dst_buffs_done++;
		ctx->dst1_buf = NULL;
	}
}

static void iep_setup_formats(struct iep_ctx *ctx)
{
	/* setup src dimensions */
	iep_write(ctx->iep, IEP_SRC_IMG_SIZE,
		  IEP_IMG_SIZE(ctx->src_fmt.pix.width, ctx->src_fmt.pix.height));

	/* setup dst dimensions */
	iep_write(ctx->iep, IEP_DST_IMG_SIZE,
		  IEP_IMG_SIZE(ctx->dst_fmt.pix.width, ctx->dst_fmt.pix.height));

	/* setup virtual width */
	iep_write(ctx->iep, IEP_VIR_IMG_WIDTH,
		  IEP_VIR_WIDTH(ctx->src_fmt.pix.width, ctx->dst_fmt.pix.width));

	/* setup src format */
	iep_shadow_mod(ctx->iep, IEP_CONFIG1, IEP_RAW_CONFIG1,
		       IEP_SRC_FMT_MASK | IEP_SRC_FMT_SWP_MASK(ctx->src_fmt.hw_fmt->hw_format),
		       IEP_SRC_FMT(ctx->src_fmt.hw_fmt->hw_format,
				   ctx->src_fmt.hw_fmt->color_swap));
	/* setup dst format */
	iep_shadow_mod(ctx->iep, IEP_CONFIG1, IEP_RAW_CONFIG1,
		       IEP_DST_FMT_MASK | IEP_DST_FMT_SWP_MASK(ctx->dst_fmt.hw_fmt->hw_format),
		       IEP_DST_FMT(ctx->dst_fmt.hw_fmt->hw_format,
				   ctx->dst_fmt.hw_fmt->color_swap));

	ctx->fmt_changed = false;
}

static void iep_dein_init(struct rockchip_iep *iep)
{
	unsigned int i;

	/* values taken from BSP driver */
	iep_shadow_mod(iep, IEP_CONFIG0, IEP_RAW_CONFIG0,
		       (IEP_DEIN_EDGE_INTPOL_SMTH_EN |
		       IEP_DEIN_EDGE_INTPOL_RADIUS_MASK |
		       IEP_DEIN_HIGH_FREQ_EN |
		       IEP_DEIN_HIGH_FREQ_MASK),
		       (IEP_DEIN_EDGE_INTPOL_SMTH_EN |
		       IEP_DEIN_EDGE_INTPOL_RADIUS(3) |
		       IEP_DEIN_HIGH_FREQ_EN |
		       IEP_DEIN_HIGH_FREQ(64)));

	for (i = 0; i < ARRAY_SIZE(default_dein_motion_tbl); i++)
		iep_write(iep, default_dein_motion_tbl[i][0],
			  default_dein_motion_tbl[i][1]);
}

static void iep_init(struct rockchip_iep *iep)
{
	iep_write(iep, IEP_CONFIG0,
		  IEP_DEIN_MODE(IEP_DEIN_MODE_BYPASS) // |
		  //IEP_YUV_ENHNC_EN
	);

	/* TODO: B/S/C/H works
	 *  only in 1-frame-out modes
	iep_write(iep, IEP_ENH_YUV_CNFG_0,
		  YUV_BRIGHTNESS(0) |
		  YUV_CONTRAST(128) |
		  YUV_SATURATION(128));

	iep_write(iep, IEP_ENH_YUV_CNFG_1,
		  YUV_COS_HUE(255) |
		  YUV_SIN_HUE(255));

	iep_write(iep, IEP_ENH_YUV_CNFG_2,
		  YUV_VIDEO_MODE(VIDEO_MODE_NORMAL_VIDEO));

	*/

	/* reset frame counter */
	iep_write(iep, IEP_FRM_CNT, 0);
}

static void iep_device_run(void *priv)
{
	struct iep_ctx *ctx = priv;
	struct rockchip_iep *iep = ctx->iep;
	struct vb2_v4l2_buffer *src, *dst;
	unsigned int dein_mode;
	dma_addr_t addr;

	if (ctx->fmt_changed)
		iep_setup_formats(ctx);

	if (ctx->prev_src_buf)
		dein_mode = IEP_DEIN_MODE_I4O2;
	else
		dein_mode = ctx->field_bff ? IEP_DEIN_MODE_I2O1B : IEP_DEIN_MODE_I2O1T;

	iep_shadow_mod(iep, IEP_CONFIG0, IEP_RAW_CONFIG0,
		       IEP_DEIN_MODE_MASK, IEP_DEIN_MODE(dein_mode));

	/* sync RAW_xxx registers with actual used */
	iep_write(iep, IEP_CONFIG_DONE, 1);

	/* setup src buff(s)/addresses */
	src = v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx);
	addr = vb2_dma_contig_plane_dma_addr(&src->vb2_buf, 0);

	iep_write(iep, IEP_DEIN_IN_IMG0_Y(ctx->field_bff), addr);

	iep_write(iep, IEP_DEIN_IN_IMG0_CBCR(ctx->field_bff),
		  addr + ctx->src_fmt.y_stride);

	iep_write(iep, IEP_DEIN_IN_IMG0_CR(ctx->field_bff),
		  addr + ctx->src_fmt.uv_stride);

	if (IEP_DEIN_IN_MODE_FIELDS(dein_mode) == IEP_DEIN_IN_FIELDS_4)
		addr = vb2_dma_contig_plane_dma_addr(&ctx->prev_src_buf->vb2_buf, 0);

	iep_write(iep, IEP_DEIN_IN_IMG1_Y(ctx->field_bff), addr);

	iep_write(iep, IEP_DEIN_IN_IMG1_CBCR(ctx->field_bff),
		  addr + ctx->src_fmt.y_stride);

	iep_write(iep, IEP_DEIN_IN_IMG1_CR(ctx->field_bff),
		  addr + ctx->src_fmt.uv_stride);

	/* setup dst buff(s)/addresses */
	dst = iep_m2m_next_dst_buf(ctx);
	addr = vb2_dma_contig_plane_dma_addr(&dst->vb2_buf, 0);

	if (IEP_DEIN_OUT_MODE_FRAMES(dein_mode) == IEP_DEIN_OUT_FRAMES_2) {
		v4l2_m2m_buf_copy_metadata(ctx->prev_src_buf, dst, true);

		iep_write(iep, IEP_DEIN_OUT_IMG0_Y(ctx->field_bff), addr);

		iep_write(iep, IEP_DEIN_OUT_IMG0_CBCR(ctx->field_bff),
			  addr + ctx->dst_fmt.y_stride);

		iep_write(iep, IEP_DEIN_OUT_IMG0_CR(ctx->field_bff),
			  addr + ctx->dst_fmt.uv_stride);

		ctx->dst0_buf = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);

		dst = iep_m2m_next_dst_buf(ctx);
		addr = vb2_dma_contig_plane_dma_addr(&dst->vb2_buf, 0);
	}

	v4l2_m2m_buf_copy_metadata(src, dst, true);

	iep_write(iep, IEP_DEIN_OUT_IMG1_Y(ctx->field_bff), addr);

	iep_write(iep, IEP_DEIN_OUT_IMG1_CBCR(ctx->field_bff),
		  addr + ctx->dst_fmt.y_stride);

	iep_write(iep, IEP_DEIN_OUT_IMG1_CR(ctx->field_bff),
		  addr + ctx->dst_fmt.uv_stride);

	ctx->dst1_buf = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);

	iep_mod(ctx->iep, IEP_INT, IEP_INT_FRAME_DONE_EN,
		IEP_INT_FRAME_DONE_EN);

	/* start HW */
	iep_write(iep, IEP_FRM_START, 1);
}

static int iep_job_ready(void *priv)
{
	struct iep_ctx *ctx = priv;

	return v4l2_m2m_num_dst_bufs_ready(ctx->fh.m2m_ctx) >= 2 &&
	       v4l2_m2m_num_src_bufs_ready(ctx->fh.m2m_ctx) >= 1;
}

static void iep_job_abort(void *priv)
{
	struct iep_ctx *ctx = priv;

	/* Will cancel the transaction in the next interrupt handler */
	ctx->job_abort = true;
}

static const struct v4l2_m2m_ops iep_m2m_ops = {
	.device_run	= iep_device_run,
	.job_ready	= iep_job_ready,
	.job_abort	= iep_job_abort,
};

static int iep_queue_setup(struct vb2_queue *vq, unsigned int *nbuffers,
			   unsigned int *nplanes, unsigned int sizes[],
				   struct device *alloc_devs[])
{
	struct iep_ctx *ctx = vb2_get_drv_priv(vq);
	struct v4l2_pix_format *pix_fmt;

	if (V4L2_TYPE_IS_OUTPUT(vq->type))
		pix_fmt = &ctx->src_fmt.pix;
	else
		pix_fmt = &ctx->dst_fmt.pix;

	if (*nplanes) {
		if (sizes[0] < pix_fmt->sizeimage)
			return -EINVAL;
	} else {
		sizes[0] = pix_fmt->sizeimage;
		*nplanes = 1;
	}

	return 0;
}

static int iep_buf_prepare(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct iep_ctx *ctx = vb2_get_drv_priv(vq);
	struct v4l2_pix_format *pix_fmt;

	if (V4L2_TYPE_IS_OUTPUT(vq->type))
		pix_fmt = &ctx->src_fmt.pix;
	else
		pix_fmt = &ctx->dst_fmt.pix;

	if (vb2_plane_size(vb, 0) < pix_fmt->sizeimage)
		return -EINVAL;

	/* set bytesused for capture buffers */
	if (!V4L2_TYPE_IS_OUTPUT(vq->type))
		vb2_set_plane_payload(vb, 0, pix_fmt->sizeimage);

	return 0;
}

static void iep_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct iep_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);

	v4l2_m2m_buf_queue(ctx->fh.m2m_ctx, vbuf);
}

static void iep_queue_cleanup(struct vb2_queue *vq, u32 state)
{
	struct iep_ctx *ctx = vb2_get_drv_priv(vq);
	struct vb2_v4l2_buffer *vbuf;

	do {
		if (V4L2_TYPE_IS_OUTPUT(vq->type))
			vbuf = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);
		else
			vbuf = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);

		if (vbuf)
			v4l2_m2m_buf_done(vbuf, state);
	} while (vbuf);

	if (V4L2_TYPE_IS_OUTPUT(vq->type) && ctx->prev_src_buf)
		v4l2_m2m_buf_done(ctx->prev_src_buf, state);
	else
		iep_m2m_dst_bufs_done(ctx, state);
}

static int iep_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct iep_ctx *ctx = vb2_get_drv_priv(vq);
	struct device *dev = ctx->iep->dev;
	int ret;

	if (V4L2_TYPE_IS_OUTPUT(vq->type)) {
		ret = pm_runtime_get_sync(dev);
		if (ret < 0) {
			dev_err(dev, "Failed to enable module\n");
			goto err_runtime_get;
		}

		ctx->field_order_bff =
			ctx->src_fmt.pix.field == V4L2_FIELD_INTERLACED_BT;
		ctx->field_bff = ctx->field_order_bff;

		ctx->src_sequence = 0;
		ctx->dst_sequence = 0;

		ctx->prev_src_buf = NULL;

		ctx->dst0_buf = NULL;
		ctx->dst1_buf = NULL;
		ctx->dst_buffs_done = 0;

		ctx->job_abort = false;

		iep_init(ctx->iep);
		//if (ctx->src_fmt.pix.field != ctx->dst_fmt.pix.field)
		iep_dein_init(ctx->iep);
	}

	return 0;

err_runtime_get:
	iep_queue_cleanup(vq, VB2_BUF_STATE_QUEUED);

	return ret;
}

static void iep_stop_streaming(struct vb2_queue *vq)
{
	struct iep_ctx *ctx = vb2_get_drv_priv(vq);

	if (V4L2_TYPE_IS_OUTPUT(vq->type)) {
		pm_runtime_mark_last_busy(ctx->iep->dev);
		pm_runtime_put_autosuspend(ctx->iep->dev);
	}

	iep_queue_cleanup(vq, VB2_BUF_STATE_ERROR);
}

static const struct vb2_ops iep_qops = {
	.queue_setup		= iep_queue_setup,
	.buf_prepare		= iep_buf_prepare,
	.buf_queue		= iep_buf_queue,
	.start_streaming	= iep_start_streaming,
	.stop_streaming		= iep_stop_streaming,
	.wait_prepare		= vb2_ops_wait_prepare,
	.wait_finish		= vb2_ops_wait_finish,
};

static int iep_queue_init(void *priv, struct vb2_queue *src_vq,
			  struct vb2_queue *dst_vq)
{
	struct iep_ctx *ctx = priv;
	int ret;

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	src_vq->dma_attrs = DMA_ATTR_ALLOC_SINGLE_PAGES |
			    DMA_ATTR_NO_KERNEL_MAPPING;
	src_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	src_vq->drv_priv = ctx;
	src_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	src_vq->min_buffers_needed = 1;
	src_vq->ops = &iep_qops;
	src_vq->mem_ops = &vb2_dma_contig_memops;
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock = &ctx->iep->mutex;
	src_vq->dev = ctx->iep->v4l2_dev.dev;

	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	dst_vq->dma_attrs = DMA_ATTR_ALLOC_SINGLE_PAGES |
			    DMA_ATTR_NO_KERNEL_MAPPING;
	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	dst_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	dst_vq->drv_priv = ctx;
	dst_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	dst_vq->min_buffers_needed = 2;
	dst_vq->ops = &iep_qops;
	dst_vq->mem_ops = &vb2_dma_contig_memops;
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &ctx->iep->mutex;
	dst_vq->dev = ctx->iep->v4l2_dev.dev;

	ret = vb2_queue_init(dst_vq);
	if (ret)
		return ret;

	return 0;
}

static void iep_prepare_format(struct v4l2_pix_format *pix_fmt)
{
	unsigned int height = pix_fmt->height;
	unsigned int width = pix_fmt->width;
	unsigned int sizeimage, bytesperline;

	struct iep_fmt *hw_fmt = iep_fmt_find(pix_fmt);

	if (!hw_fmt) {
		hw_fmt = &formats[0];
		pix_fmt->pixelformat = hw_fmt->fourcc;
	}

	width = ALIGN(clamp(width, IEP_MIN_WIDTH,
			    IEP_MAX_WIDTH), 16);
	height = ALIGN(clamp(height, IEP_MIN_HEIGHT,
			     IEP_MAX_HEIGHT), 16);

	bytesperline = FMT_IS_YUV(hw_fmt->hw_format)
		       ? width : (width * hw_fmt->depth) >> 3;

	sizeimage = height * (width * hw_fmt->depth) >> 3;

	pix_fmt->width = width;
	pix_fmt->height = height;
	pix_fmt->bytesperline = bytesperline;
	pix_fmt->sizeimage = sizeimage;
}

static int iep_open(struct file *file)
{
	struct rockchip_iep *iep = video_drvdata(file);
	struct iep_ctx *ctx = NULL;

	int ret;

	if (mutex_lock_interruptible(&iep->mutex))
		return -ERESTARTSYS;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		mutex_unlock(&iep->mutex);
		return -ENOMEM;
	}

	/* default output format */
	ctx->src_fmt.pix.pixelformat = formats[0].fourcc;
	ctx->src_fmt.pix.field = V4L2_FIELD_INTERLACED;
	ctx->src_fmt.pix.width = IEP_DEFAULT_WIDTH;
	ctx->src_fmt.pix.height = IEP_DEFAULT_HEIGHT;
	iep_prepare_format(&ctx->src_fmt.pix);
	ctx->src_fmt.hw_fmt = &formats[0];
	ctx->dst_fmt.y_stride = IEP_Y_STRIDE(ctx->src_fmt.pix.width, ctx->src_fmt.pix.height);
	ctx->dst_fmt.uv_stride = IEP_UV_STRIDE(ctx->src_fmt.pix.width, ctx->src_fmt.pix.height,
					       ctx->src_fmt.hw_fmt->uv_factor);

	/* default capture format */
	ctx->dst_fmt.pix.pixelformat = formats[0].fourcc;
	ctx->dst_fmt.pix.field = V4L2_FIELD_NONE;
	ctx->dst_fmt.pix.width = IEP_DEFAULT_WIDTH;
	ctx->dst_fmt.pix.height = IEP_DEFAULT_HEIGHT;
	iep_prepare_format(&ctx->dst_fmt.pix);
	ctx->dst_fmt.hw_fmt = &formats[0];
	ctx->dst_fmt.y_stride = IEP_Y_STRIDE(ctx->dst_fmt.pix.width, ctx->dst_fmt.pix.height);
	ctx->dst_fmt.uv_stride = IEP_UV_STRIDE(ctx->dst_fmt.pix.width, ctx->dst_fmt.pix.height,
					       ctx->dst_fmt.hw_fmt->uv_factor);
	/* ensure fmts are written to HW */
	ctx->fmt_changed = true;

	v4l2_fh_init(&ctx->fh, video_devdata(file));
	file->private_data = &ctx->fh;
	ctx->iep = iep;

	ctx->fh.m2m_ctx = v4l2_m2m_ctx_init(iep->m2m_dev, ctx,
					    &iep_queue_init);

	if (IS_ERR(ctx->fh.m2m_ctx)) {
		ret = PTR_ERR(ctx->fh.m2m_ctx);
		goto err_free;
	}

	v4l2_fh_add(&ctx->fh);

	mutex_unlock(&iep->mutex);

	return 0;

err_free:
	kfree(ctx);
	mutex_unlock(&iep->mutex);

	return ret;
}

static int iep_release(struct file *file)
{
	struct rockchip_iep *iep = video_drvdata(file);
	struct iep_ctx *ctx = container_of(file->private_data,
						   struct iep_ctx, fh);

	mutex_lock(&iep->mutex);

	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	v4l2_m2m_ctx_release(ctx->fh.m2m_ctx);
	kfree(ctx);

	mutex_unlock(&iep->mutex);
	return 0;
}

static const struct v4l2_file_operations iep_fops = {
	.owner		= THIS_MODULE,
	.open		= iep_open,
	.release	= iep_release,
	.poll		= v4l2_m2m_fop_poll,
	.unlocked_ioctl	= video_ioctl2,
	.mmap		= v4l2_m2m_fop_mmap,
};

static int iep_querycap(struct file *file, void *priv,
			struct v4l2_capability *cap)
{
	strscpy(cap->driver, IEP_NAME, sizeof(cap->driver));
	strscpy(cap->card, IEP_NAME, sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info),
		 "platform:%s", IEP_NAME);

	return 0;
}

static int iep_enum_fmt(struct file *file, void *priv,
			struct v4l2_fmtdesc *f)
{
	struct iep_fmt *fmt;

	if (f->index < ARRAY_SIZE(formats)) {
		fmt = &formats[f->index];
		f->pixelformat = fmt->fourcc;

		return 0;
	}

	return -EINVAL;
}

static int iep_enum_framesizes(struct file *file, void *priv,
			       struct v4l2_frmsizeenum *fsize)
{
	if (fsize->index != 0)
		return -EINVAL;

	if (!iep_check_pix_format(fsize->pixel_format))
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;

	fsize->stepwise.min_width = IEP_MIN_WIDTH;
	fsize->stepwise.max_width = IEP_MAX_WIDTH;
	fsize->stepwise.step_width = 16;

	fsize->stepwise.min_height = IEP_MIN_HEIGHT;
	fsize->stepwise.max_height = IEP_MAX_HEIGHT;
	fsize->stepwise.step_height = 16;

	return 0;
}

static inline struct iep_ctx *iep_file2ctx(struct file *file)
{
	return container_of(file->private_data, struct iep_ctx, fh);
}

static int iep_g_fmt_vid_cap(struct file *file, void *priv,
			     struct v4l2_format *f)
{
	struct iep_ctx *ctx = iep_file2ctx(file);

	f->fmt.pix = ctx->dst_fmt.pix;

	return 0;
}

static int iep_g_fmt_vid_out(struct file *file, void *priv,
			     struct v4l2_format *f)
{
	struct iep_ctx *ctx = iep_file2ctx(file);

	f->fmt.pix = ctx->src_fmt.pix;

	return 0;
}

static int iep_try_fmt_vid_cap(struct file *file, void *priv,
			       struct v4l2_format *f)
{
	f->fmt.pix.field = V4L2_FIELD_NONE;
	iep_prepare_format(&f->fmt.pix);

	return 0;
}

static int iep_try_fmt_vid_out(struct file *file, void *priv,
			       struct v4l2_format *f)
{
	if (f->fmt.pix.field != V4L2_FIELD_INTERLACED_TB &&
	    f->fmt.pix.field != V4L2_FIELD_INTERLACED_BT &&
	    f->fmt.pix.field != V4L2_FIELD_INTERLACED)
		f->fmt.pix.field = V4L2_FIELD_INTERLACED;

	iep_prepare_format(&f->fmt.pix);

	return 0;
}

static int iep_s_fmt_vid_out(struct file *file, void *priv,
			     struct v4l2_format *f)
{
	struct iep_ctx *ctx = iep_file2ctx(file);
	struct vb2_queue *vq;

	int ret;

	ret = iep_try_fmt_vid_out(file, priv, f);
	if (ret)
		return ret;

	vq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, f->type);
	if (vb2_is_busy(vq))
		return -EBUSY;

	ctx->src_fmt.pix = f->fmt.pix;
	ctx->src_fmt.hw_fmt = iep_fmt_find(&f->fmt.pix);
	ctx->src_fmt.y_stride = IEP_Y_STRIDE(f->fmt.pix.width, f->fmt.pix.height);
	ctx->src_fmt.uv_stride = IEP_UV_STRIDE(f->fmt.pix.width, f->fmt.pix.height,
					       ctx->src_fmt.hw_fmt->uv_factor);

	/* Propagate colorspace information to capture. */
	ctx->dst_fmt.pix.colorspace = f->fmt.pix.colorspace;
	ctx->dst_fmt.pix.xfer_func = f->fmt.pix.xfer_func;
	ctx->dst_fmt.pix.ycbcr_enc = f->fmt.pix.ycbcr_enc;
	ctx->dst_fmt.pix.quantization = f->fmt.pix.quantization;

	/* scaling is not supported */
	ctx->dst_fmt.pix.width = f->fmt.pix.width;
	ctx->dst_fmt.pix.height = f->fmt.pix.height;
	ctx->dst_fmt.y_stride = IEP_Y_STRIDE(f->fmt.pix.width, f->fmt.pix.height);
	ctx->dst_fmt.uv_stride = IEP_UV_STRIDE(f->fmt.pix.width, f->fmt.pix.height,
					       ctx->dst_fmt.hw_fmt->uv_factor);

	ctx->fmt_changed = true;

	return 0;
}

static int iep_s_fmt_vid_cap(struct file *file, void *priv,
			     struct v4l2_format *f)
{
	struct iep_ctx *ctx = iep_file2ctx(file);
	struct vb2_queue *vq;
	int ret;

	ret = iep_try_fmt_vid_cap(file, priv, f);
	if (ret)
		return ret;

	vq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, f->type);
	if (vb2_is_busy(vq))
		return -EBUSY;

	/* scaling is not supported */
	f->fmt.pix.width = ctx->src_fmt.pix.width;
	f->fmt.pix.height = ctx->src_fmt.pix.height;

	ctx->dst_fmt.pix = f->fmt.pix;
	ctx->dst_fmt.hw_fmt = iep_fmt_find(&f->fmt.pix);

	ctx->dst_fmt.y_stride = IEP_Y_STRIDE(f->fmt.pix.width, f->fmt.pix.height);
	ctx->dst_fmt.uv_stride = IEP_UV_STRIDE(f->fmt.pix.width, f->fmt.pix.height,
					       ctx->dst_fmt.hw_fmt->uv_factor);

	ctx->fmt_changed = true;

	return 0;
}

static const struct v4l2_ioctl_ops iep_ioctl_ops = {
	.vidioc_querycap		= iep_querycap,

	.vidioc_enum_framesizes		= iep_enum_framesizes,

	.vidioc_enum_fmt_vid_cap	= iep_enum_fmt,
	.vidioc_g_fmt_vid_cap		= iep_g_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap		= iep_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap		= iep_s_fmt_vid_cap,

	.vidioc_enum_fmt_vid_out	= iep_enum_fmt,
	.vidioc_g_fmt_vid_out		= iep_g_fmt_vid_out,
	.vidioc_try_fmt_vid_out		= iep_try_fmt_vid_out,
	.vidioc_s_fmt_vid_out		= iep_s_fmt_vid_out,

	.vidioc_reqbufs			= v4l2_m2m_ioctl_reqbufs,
	.vidioc_querybuf		= v4l2_m2m_ioctl_querybuf,
	.vidioc_qbuf			= v4l2_m2m_ioctl_qbuf,
	.vidioc_dqbuf			= v4l2_m2m_ioctl_dqbuf,
	.vidioc_prepare_buf		= v4l2_m2m_ioctl_prepare_buf,
	.vidioc_create_bufs		= v4l2_m2m_ioctl_create_bufs,
	.vidioc_expbuf			= v4l2_m2m_ioctl_expbuf,

	.vidioc_streamon		= v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff		= v4l2_m2m_ioctl_streamoff,
};

static const struct video_device iep_video_device = {
	.name		= IEP_NAME,
	.vfl_dir	= VFL_DIR_M2M,
	.fops		= &iep_fops,
	.ioctl_ops	= &iep_ioctl_ops,
	.minor		= -1,
	.release	= video_device_release_empty,
	.device_caps	= V4L2_CAP_VIDEO_M2M | V4L2_CAP_STREAMING,
};

static int iep_parse_dt(struct rockchip_iep *iep)
{
	int ret = 0;

	iep->axi_clk = devm_clk_get(iep->dev, "axi");
	if (IS_ERR(iep->axi_clk)) {
		dev_err(iep->dev, "failed to get aclk clock\n");
		return PTR_ERR(iep->axi_clk);
	}

	iep->ahb_clk = devm_clk_get(iep->dev, "ahb");
	if (IS_ERR(iep->ahb_clk)) {
		dev_err(iep->dev, "failed to get hclk clock\n");
		return PTR_ERR(iep->ahb_clk);
	}

	ret = clk_set_rate(iep->axi_clk, 300000000);

	if (ret)
		dev_err(iep->dev, "failed to set axi clock rate to 300 MHz\n");

	return ret;
}

static irqreturn_t iep_isr(int irq, void *prv)
{
	struct rockchip_iep *iep = prv;
	struct iep_ctx *ctx;
	u32 val;
	enum vb2_buffer_state state = VB2_BUF_STATE_DONE;

	ctx = v4l2_m2m_get_curr_priv(iep->m2m_dev);
	if (!ctx) {
		v4l2_err(&iep->v4l2_dev,
			 "Instance released before the end of transaction\n");
		return IRQ_NONE;
	}

	/*
	 * The irq is shared with the iommu. If the runtime-pm state of the
	 * iep-device is disabled or the interrupt status doesn't match the
	 * expeceted mask the irq has been targeted to the iommu.
	 */

	if (!pm_runtime_active(iep->dev) ||
	    !(iep_read(iep, IEP_INT) & IEP_INT_MASK))
		return IRQ_NONE;

	/* disable interrupt - will be re-enabled at next iep_device_run */
	iep_mod(ctx->iep, IEP_INT,
		IEP_INT_FRAME_DONE_EN, 0);

	iep_mod(iep, IEP_INT, IEP_INT_FRAME_DONE_CLR,
		IEP_INT_FRAME_DONE_CLR);

	/* wait for all status regs to show "idle" */
	val = readl_poll_timeout(iep->regs + IEP_STATUS, val,
				  (val == 0), 100, IEP_TIMEOUT);

	if (val) {
		dev_err(iep->dev,
			"Failed to wait for job to finish: status: %u\n", val);
		state = VB2_BUF_STATE_ERROR;
		ctx->job_abort = true;
	}

	iep_m2m_dst_bufs_done(ctx, state);

	ctx->field_bff = (ctx->dst_buffs_done % 2 == 0)
		     ? ctx->field_order_bff : !ctx->field_order_bff;

	if (ctx->dst_buffs_done == 2 || ctx->job_abort) {
		if (ctx->prev_src_buf)
			v4l2_m2m_buf_done(ctx->prev_src_buf, state);

		/* current src buff will be next prev */
		ctx->prev_src_buf = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);

		v4l2_m2m_job_finish(ctx->iep->m2m_dev, ctx->fh.m2m_ctx);
		ctx->dst_buffs_done = 0;

	} else {
		iep_device_run(ctx);
	}

	return IRQ_HANDLED;
}

static int iep_probe(struct platform_device *pdev)
{
	struct rockchip_iep *iep;
	struct video_device *vfd;
	struct resource *res;
	int ret = 0;
	int irq;

	if (!pdev->dev.of_node)
		return -ENODEV;

	iep = devm_kzalloc(&pdev->dev, sizeof(*iep), GFP_KERNEL);
	if (!iep)
		return -ENOMEM;

	platform_set_drvdata(pdev, iep);
	iep->dev = &pdev->dev;
	iep->vfd = iep_video_device;

	ret = iep_parse_dt(iep);
	if (ret)
		dev_err(&pdev->dev, "Unable to parse OF data\n");

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	iep->regs = devm_ioremap_resource(iep->dev, res);
	if (IS_ERR(iep->regs)) {
		ret = PTR_ERR(iep->regs);
		goto err_put_clk;
	}

	ret = dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(&pdev->dev, "Could not set DMA coherent mask.\n");
		goto err_put_clk;
	}

	vb2_dma_contig_set_max_seg_size(&pdev->dev, DMA_BIT_MASK(32));

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		ret = irq;
		goto err_put_clk;
	}

	/* IRQ is shared with IOMMU */
	ret = devm_request_irq(iep->dev, irq, iep_isr, IRQF_SHARED,
			       dev_name(iep->dev), iep);
	if (ret < 0) {
		dev_err(iep->dev, "failed to request irq\n");
		goto err_put_clk;
	}

	mutex_init(&iep->mutex);

	ret = v4l2_device_register(&pdev->dev, &iep->v4l2_dev);
	if (ret) {
		dev_err(iep->dev, "Failed to register V4L2 device\n");

		return ret;
	}

	vfd = &iep->vfd;
	vfd->lock = &iep->mutex;
	vfd->v4l2_dev = &iep->v4l2_dev;

	snprintf(vfd->name, sizeof(vfd->name), "%s",
		 iep_video_device.name);

	video_set_drvdata(vfd, iep);

	ret = video_register_device(vfd, VFL_TYPE_VIDEO, 0);
	if (ret) {
		v4l2_err(&iep->v4l2_dev, "Failed to register video device\n");

		goto err_v4l2;
	}

	v4l2_info(&iep->v4l2_dev,
		  "Device %s registered as /dev/video%d\n", vfd->name, vfd->num);

	iep->m2m_dev = v4l2_m2m_init(&iep_m2m_ops);
	if (IS_ERR(iep->m2m_dev)) {
		v4l2_err(&iep->v4l2_dev,
			 "Failed to initialize V4L2 M2M device\n");
		ret = PTR_ERR(iep->m2m_dev);

		goto err_video;
	}

	pm_runtime_set_autosuspend_delay(iep->dev, 100);
	pm_runtime_use_autosuspend(iep->dev);
	pm_runtime_enable(iep->dev);

	return ret;

err_video:
	video_unregister_device(&iep->vfd);
err_v4l2:
	v4l2_device_unregister(&iep->v4l2_dev);
err_put_clk:
	pm_runtime_dont_use_autosuspend(iep->dev);
	pm_runtime_disable(iep->dev);

return ret;
}

static int iep_remove(struct platform_device *pdev)
{
	struct rockchip_iep *iep = platform_get_drvdata(pdev);

	pm_runtime_dont_use_autosuspend(iep->dev);
	pm_runtime_disable(iep->dev);

	v4l2_m2m_release(iep->m2m_dev);
	video_unregister_device(&iep->vfd);
	v4l2_device_unregister(&iep->v4l2_dev);

	return 0;
}

static int __maybe_unused iep_runtime_suspend(struct device *dev)
{
	struct rockchip_iep *iep = dev_get_drvdata(dev);

	clk_disable_unprepare(iep->ahb_clk);
	clk_disable_unprepare(iep->axi_clk);

	return 0;
}

static int __maybe_unused iep_runtime_resume(struct device *dev)
{
	struct rockchip_iep *iep;
	int ret = 0;

	iep = dev_get_drvdata(dev);

	ret = clk_prepare_enable(iep->axi_clk);
	if (ret) {
		dev_err(iep->dev, "Cannot enable axi clock: %d\n", ret);
		return ret;
	}

	ret = clk_prepare_enable(iep->ahb_clk);
	if (ret) {
		dev_err(iep->dev, "Cannot enable ahb clock: %d\n", ret);
		goto err_disable_axi_clk;
	}

	return ret;

err_disable_axi_clk:
	clk_disable_unprepare(iep->axi_clk);
	return ret;
}

static const struct dev_pm_ops iep_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
	SET_RUNTIME_PM_OPS(iep_runtime_suspend,
			   iep_runtime_resume, NULL)
};

static const struct of_device_id rockchip_iep_match[] = {
	{
		.compatible = "rockchip,rk3228-iep",
	},
	{},
};

MODULE_DEVICE_TABLE(of, rockchip_iep_match);

static struct platform_driver iep_pdrv = {
	.probe = iep_probe,
	.remove = iep_remove,
	.driver = {
		.name = IEP_NAME,
		.pm = &iep_pm_ops,
		.of_match_table = rockchip_iep_match,
	},
};

module_platform_driver(iep_pdrv);

MODULE_AUTHOR("Alex Bee <knaerzche@gmail.com>");
MODULE_DESCRIPTION("Rockchip Image Enhancement Processor");
MODULE_LICENSE("GPL v2");
