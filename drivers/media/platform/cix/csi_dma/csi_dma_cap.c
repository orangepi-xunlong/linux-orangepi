/*
*
* Copyright 2024 Cix Technology Group Co., Ltd.
*/

#include "csi_bridge_hw.h"
#include "csi_dma_cap.h"
#include "csi_rcsu_hw.h"
#include "cdns-csi2rx.h"

#define CSIDMA_PLL_CLK	1200

static int csi_dma_cap_streamoff(struct file *file, void *priv,
				 enum v4l2_buf_type type);

#define CSI_DMA_FRAME_TRACE_COUNT	(30)

/* CSI-DMA source format same with output format */
struct csi_dma_fmt csi_dma_src_formats[] = {
	{
		.name		= "RGB888",
		.fourcc		= V4L2_PIX_FMT_RGB24,
		.depth		= { 24 },
		.memplanes	= 1,
		.mbus_code  = MEDIA_BUS_FMT_RGB888_1X24,
	}, {
		.name		= "RGB565",
		.fourcc		= V4L2_PIX_FMT_RGB565,
		.depth		= { 16 },
		.memplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_RGB565_1X16,
	}, {
		.name		= "RAW8",
		.fourcc		= V4L2_PIX_FMT_SBGGR8,
		.depth		= { 16 },
		.memplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_SBGGR8_1X8,
	}, {
		.name		= "RAW10",
		.fourcc		= V4L2_PIX_FMT_SBGGR10,
		.depth		= { 16 },
		.memplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_SBGGR10_1X10,
	}, {
		.name		= "RAW12",
		.fourcc		= V4L2_PIX_FMT_SBGGR12,
		.depth		= { 16 },
		.memplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_SBGGR12_1X12,
	}, {
		.name		= "RAW14",
		.fourcc		= V4L2_PIX_FMT_SBGGR14,
		.depth		= { 16 },
		.memplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_SBGGR14_1X14,
	}, {
		.name		= "RAW16",
		.fourcc		= V4L2_PIX_FMT_SBGGR16,
		.depth		= { 16 },
		.memplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_SBGGR16_1X16,
	}, {
		.name		= "NV12",
		.fourcc		= V4L2_PIX_FMT_NV12,
		.depth		= { 8, 8 },
		.memplanes	= 2,
		.mbus_code	= MEDIA_BUS_FMT_YVYU8_2X8,
	}, {
		.name		= "P010",
		.fourcc		= V4L2_PIX_FMT_P010,
		.depth		= { 10, 10 },
		.memplanes	= 2,
		.mbus_code	= MEDIA_BUS_FMT_YUYV10_2X10,
	}, {
		.name		= "YUYV",
		.fourcc		= V4L2_PIX_FMT_YUYV,
		.depth		= { 16 },
		.memplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_YUYV8_1X16,
	}, {
		.name		= "YUYV10",
		.fourcc		= V4L2_PIX_FMT_YUYV10,
		.depth		= { 10, 10 },
		.memplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_YUYV10_1X20,
	}
};

pid_t csi_dma_getpid(void)
{
	return current->pid;
}

struct csi_dma_fmt *csi_dma_get_format(unsigned int index)
{
	return &csi_dma_src_formats[index];
}

/*
 * lookup csi_dma color format by fourcc or media bus format
 */
struct csi_dma_fmt *csi_dma_find_format(const u32 *pixelformat,
					const u32 *mbus_code, int index)
{
	struct csi_dma_fmt *fmt, *def_fmt = NULL;
	int i;

	if (index >= ARRAY_SIZE(csi_dma_src_formats))
		return NULL;

	for (i = 0; i < ARRAY_SIZE(csi_dma_src_formats); i++) {
		fmt = &csi_dma_src_formats[i];

		if ((pixelformat && (fmt->fourcc == *pixelformat)) ||
				(mbus_code && (fmt->mbus_code == *mbus_code))) {
			return fmt;
		}

		if (index == i) {
			def_fmt = fmt;
		}
	}
	return def_fmt;
}

static int csi_dma_cal_clk_freq(struct csi_dma_cap_dev *dma_cap)
{
	/********************TODO**************************
	******Calibrate the frequency of csidam************
	***********************************************/
	if (dma_cap->csi_dma->sys_clk_freq > CSIDMA_PLL_CLK)
		dma_cap->csi_dma->sys_clk_freq = CSIDMA_PLL_CLK;

	return 0;
}


struct csi_dma_fmt *csi_dma_get_src_fmt(struct csi_dma_cap_dev *dma_cap,
					struct v4l2_subdev_format *sd_fmt)
{
	u32 index;
	struct device *dev = &dma_cap->pdev->dev;

	dev_info(dev, "%s enter sd_fmt->format.code = 0x%x\n", __func__, sd_fmt->format.code);

	switch (sd_fmt->format.code) {
		case MEDIA_BUS_FMT_RGB888_1X24:
			index = 0;
			break;
		case MEDIA_BUS_FMT_RGB565_1X16:
			index = 1;
			break;
		case MEDIA_BUS_FMT_SBGGR10_1X10:
			index = 3;
			break;
		case MEDIA_BUS_FMT_SBGGR16_1X16:
			index = 6;
			break;
		case MEDIA_BUS_FMT_YVYU8_2X8:
			index = 7;
			break;
		case MEDIA_BUS_FMT_YUYV10_2X10:
			index = 8;
			break;
		case MEDIA_BUS_FMT_YUYV8_1X16:
			index = 9;
			break;
		case MEDIA_BUS_FMT_YUYV10_1X20:
			index = 10;
			break;
		default:
			dev_err(dev, "Invalid media bus fmt %d\n",
					sd_fmt->format.code);
			index = 0;
			break;
	};

	dev_info(dev, "%s exit index = %d\n", __func__, index);
	return &csi_dma_src_formats[index];
}

static inline struct csi_dma_buffer *to_csi_dma_buffer(struct vb2_v4l2_buffer *v4l2_buf)
{
	return container_of(v4l2_buf, struct csi_dma_buffer, v4l2_buf);
}

/* Enable streaming on a pipeline */
static int csi_dma_pipeline_enable(struct csi_dma_cap_dev *dma_cap, bool enable)
{
	struct csi_dma_dev *csi_dma = dma_cap->csi_dma;
	return csi_dma->pipe.set_stream(&csi_dma->pipe,enable);
}

static int csi_dma_update_buf_paddr(struct csi_dma_buffer *buf, int memplanes)
{
	struct frame_addr *paddr = &buf->paddr;
	struct vb2_buffer *vb2 = &buf->v4l2_buf.vb2_buf;

	paddr->cb = 0;

	switch (memplanes) {
		case 1:
			paddr->y = vb2_dma_contig_plane_dma_addr(vb2, 0);
			break;
		case 2:
			paddr->y = vb2_dma_contig_plane_dma_addr(vb2, 0);
			paddr->cb = vb2_dma_contig_plane_dma_addr(vb2, 1);
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

void csi_dma_cap_frame_write_done(struct csi_dma_dev *csi_dma)
{
	struct csi_dma_cap_dev *dma_cap = csi_dma->dma_cap;
	struct device *dev = &dma_cap->pdev->dev;
	struct csi_dma_buffer *buf;
	struct vb2_buffer *vb2;
	unsigned long flags;

	dma_cap->frame_count++;

	if ((dma_cap->frame_count % CSI_DMA_FRAME_TRACE_COUNT) == 0) {
		dev_info(dev, "[%d] frame received \n", dma_cap->frame_count);
	}

	spin_lock_irqsave(&dma_cap->slock, flags);

	if (list_empty(&dma_cap->out_active)) {
		spin_unlock_irqrestore(&dma_cap->slock, flags);
		dev_warn(dev, "trying to access empty active list\n");
		return;
	}

	buf = list_first_entry(&dma_cap->out_active, struct csi_dma_buffer, list);
	if (buf->discard) {
		list_move_tail(dma_cap->out_active.next, &dma_cap->out_discard);
	} else {
		vb2 = &buf->v4l2_buf.vb2_buf;
		list_del_init(&buf->list);
		buf->v4l2_buf.vb2_buf.timestamp = ktime_get_ns();
		vb2_buffer_done(&buf->v4l2_buf.vb2_buf, VB2_BUF_STATE_DONE);
	}

	if (list_empty(&dma_cap->out_pending)) {
		if (list_empty(&dma_cap->out_discard)) {
			spin_unlock_irqrestore(&dma_cap->slock, flags);
			dev_err(dev, "trying to access empty discard list\n");
			return;
		}

		buf = list_first_entry(&dma_cap->out_discard,
				       struct csi_dma_buffer, list);
		buf->v4l2_buf.sequence = dma_cap->frame_count;
		csi_dma_channel_set_outbuf(csi_dma, buf);
		list_move_tail(dma_cap->out_discard.next, &dma_cap->out_active);

		spin_unlock_irqrestore(&dma_cap->slock, flags);
		return;
	}

	buf = list_first_entry(&dma_cap->out_pending, struct csi_dma_buffer, list);
	buf->v4l2_buf.sequence = dma_cap->frame_count;
	csi_dma_channel_set_outbuf(csi_dma, buf);
	vb2 = &buf->v4l2_buf.vb2_buf;
	vb2->state = VB2_BUF_STATE_ACTIVE;
	list_move_tail(dma_cap->out_pending.next, &dma_cap->out_active);

	spin_unlock_irqrestore(&dma_cap->slock, flags);
}
EXPORT_SYMBOL_GPL(csi_dma_cap_frame_write_done);

static int cap_vb2_queue_setup(struct vb2_queue *q,
			       unsigned int *num_buffers,
			       unsigned int *num_planes,
			       unsigned int sizes[],
			       struct device *alloc_devs[])
{
	struct csi_dma_cap_dev *dma_cap = vb2_get_drv_priv(q);
	struct device *dev = &dma_cap->pdev->dev;
	struct csi_dma_frame *src_f = &dma_cap->src_f;
	struct csi_dma_fmt *fmt = src_f->fmt;
	unsigned long wh;
	int i;

	if (!fmt)
		return -EINVAL;

	if (*num_planes) {
		if (*num_planes != fmt->memplanes)
			return -EINVAL;

		for (i = 0; i < *num_planes; i++)
			if (sizes[i] < src_f->sizeimage[i])
				return -EINVAL;
	}

	for (i = 0; i < fmt->memplanes; i++)
		alloc_devs[i] = &dma_cap->pdev->dev;

	wh = src_f->width * src_f->height;
	*num_planes = fmt->memplanes;

	for (i = 0; i < fmt->memplanes; i++) {
		unsigned int size = (wh * fmt->depth[i]) / 8;

		if (((V4L2_PIX_FMT_NV12 == fmt->fourcc) ||
				(V4L2_PIX_FMT_P010 == fmt->fourcc)) &&
				(1 == i)) {
			size >>= 1;
		}
		sizes[i] = max_t(u32, size, src_f->sizeimage[i]);
	}

	dev_info(dev, "%s, buf_n=%d, size=%d\n",
		__func__, *num_buffers, sizes[0]);

	return 0;
}

static int cap_vb2_buffer_prepare(struct vb2_buffer *vb2)
{
	struct vb2_queue *q = vb2->vb2_queue;
	struct csi_dma_cap_dev *dma_cap = vb2_get_drv_priv(q);
	struct device *dev = &dma_cap->pdev->dev;
	struct csi_dma_frame *src_f = &dma_cap->src_f;
	int i;

	if (!dma_cap->src_f.fmt)
		return -EINVAL;

	for (i = 0; i < src_f->fmt->memplanes; i++) {
		unsigned long size = src_f->sizeimage[i];
		if (vb2_plane_size(vb2, i) < size) {
			dev_err(dev,"User buffer too small (%ld < %ld)\n",
				 vb2_plane_size(vb2, i), size);

			return -EINVAL;
		}

		vb2_set_plane_payload(vb2, i, size);
	}

	return 0;
}

static void cap_vb2_buffer_queue(struct vb2_buffer *vb2)
{
	struct vb2_v4l2_buffer *v4l2_buf = to_vb2_v4l2_buffer(vb2);
	struct csi_dma_buffer *buf = to_csi_dma_buffer(v4l2_buf);
	struct csi_dma_cap_dev *dma_cap = vb2_get_drv_priv(vb2->vb2_queue);
	unsigned long flags;

	spin_lock_irqsave(&dma_cap->slock, flags);

	csi_dma_update_buf_paddr(buf, dma_cap->src_f.fmt->mdataplanes);
	list_add_tail(&buf->list, &dma_cap->out_pending);

	spin_unlock_irqrestore(&dma_cap->slock, flags);
}

static int cap_vb2_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct csi_dma_cap_dev *dma_cap = vb2_get_drv_priv(q);
	struct device *dev = &dma_cap->pdev->dev;
	struct csi_dma_dev *csi_dma = dma_cap->csi_dma;
	struct csi_dma_buffer *buf;
	struct vb2_buffer *vb2;
	unsigned long flags;
	int i, j;

	dev_info(dev, "%s enter\n", __func__);

	if (count < 2)
		return -ENOBUFS;

	if (!csi_dma)
		return -EINVAL;

	/* Create a buffer for discard operation */
	for (i = 0; i < dma_cap->pix.num_planes; i++) {
		dma_cap->discard_size[i] = dma_cap->src_f.sizeimage[i];
		dma_cap->discard_buffer[i] =
			dma_alloc_coherent(&dma_cap->pdev->dev,
					   PAGE_ALIGN(dma_cap->discard_size[i]),
					   &dma_cap->discard_buffer_dma[i],
					   GFP_KERNEL);

		if (!dma_cap->discard_buffer[i]) {
			for (j = 0; j < i; j++) {
				dma_free_coherent(&dma_cap->pdev->dev,
						  PAGE_ALIGN(dma_cap->discard_size[j]),
						  dma_cap->discard_buffer[j],
						  dma_cap->discard_buffer_dma[j]);
				dev_err(dev,
					"alloc dma buffer(%d) fail\n", j);
			}
			return -ENOMEM;
		}

		dev_info(dev,
			"%s: num_plane=%d discard_size=%d discard_buffer=%p\n"
			, __func__, i,
			PAGE_ALIGN((int)dma_cap->discard_size[i]),
			dma_cap->discard_buffer[i]);
	}

	spin_lock_irqsave(&dma_cap->slock, flags);

	/* add two list member to out_discard list head */
	dma_cap->buf_discard[0].discard = true;
	list_add_tail(&dma_cap->buf_discard[0].list, &dma_cap->out_discard);

	dma_cap->buf_discard[1].discard = true;
	list_add_tail(&dma_cap->buf_discard[1].list, &dma_cap->out_discard);

	/* ISI channel output buffer 2 */
	buf = list_first_entry(&dma_cap->out_pending, struct csi_dma_buffer, list);
	buf->v4l2_buf.sequence = 1;
	vb2 = &buf->v4l2_buf.vb2_buf;
	vb2->state = VB2_BUF_STATE_ACTIVE;

	csi_dma_channel_set_outbuf(csi_dma, buf);

	list_move_tail(dma_cap->out_pending.next, &dma_cap->out_active);

	/* Clear frame count */
	dma_cap->frame_count = 1;

	spin_unlock_irqrestore(&dma_cap->slock, flags);

	dev_info(dev, "%s exit\n", __func__);

	return 0;
}

static void cap_vb2_stop_streaming(struct vb2_queue *q)
{
	struct csi_dma_cap_dev *dma_cap = vb2_get_drv_priv(q);
	struct device *dev = &dma_cap->pdev->dev;
	struct csi_dma_buffer *buf;
	unsigned long flags;
	int i;

	dev_info(dev, "%s\n", __func__);

	spin_lock_irqsave(&dma_cap->slock, flags);

	while (!list_empty(&dma_cap->out_active)) {
		buf = list_entry(dma_cap->out_active.next,
				 struct csi_dma_buffer, list);
		list_del_init(&buf->list);
		if (buf->discard)
			continue;

		vb2_buffer_done(&buf->v4l2_buf.vb2_buf, VB2_BUF_STATE_ERROR);
	}

	while (!list_empty(&dma_cap->out_pending)) {
		buf = list_entry(dma_cap->out_pending.next,
				 struct csi_dma_buffer, list);
		list_del_init(&buf->list);
		vb2_buffer_done(&buf->v4l2_buf.vb2_buf, VB2_BUF_STATE_ERROR);
	}

	while (!list_empty(&dma_cap->out_discard)) {
		buf = list_entry(dma_cap->out_discard.next,
				 struct csi_dma_buffer, list);
		list_del_init(&buf->list);
	}

	INIT_LIST_HEAD(&dma_cap->out_active);
	INIT_LIST_HEAD(&dma_cap->out_pending);
	INIT_LIST_HEAD(&dma_cap->out_discard);

	spin_unlock_irqrestore(&dma_cap->slock, flags);

	for (i = 0; i < dma_cap->pix.num_planes; i++)
		dma_free_coherent(&dma_cap->pdev->dev,
				PAGE_ALIGN(dma_cap->discard_size[i]),
				dma_cap->discard_buffer[i],
				dma_cap->discard_buffer_dma[i]);
}

static struct vb2_ops sky1_cap_vb2_qops = {
	.queue_setup		= cap_vb2_queue_setup,
	.buf_prepare		= cap_vb2_buffer_prepare,
	.buf_queue		= cap_vb2_buffer_queue,
	.wait_prepare		= vb2_ops_wait_prepare,
	.wait_finish		= vb2_ops_wait_finish,
	.start_streaming	= cap_vb2_start_streaming,
	.stop_streaming		= cap_vb2_stop_streaming,
};

static int dma_cap_fmt_init(struct csi_dma_cap_dev *dma_cap)
{
	struct csi_dma_frame *src_f = &dma_cap->src_f;
	struct v4l2_subdev_format src_fmt;
	struct v4l2_subdev *src_sd = dma_cap->source_subdev;
	struct device *dev = &dma_cap->pdev->dev;
	int i, ret;

	dev_info(dev, "%s enter dma_cap = %px\n", __func__, dma_cap);
	if(src_sd == NULL) {
		dev_info(dev, "remote subdev link is not setup\n");
		return -1;
	}

	memset(&src_fmt, 0, sizeof(src_fmt));
	src_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	ret = v4l2_subdev_call(src_sd, pad, get_fmt, NULL, &src_fmt);
	if (ret < 0 && ret != -ENOIOCTLCMD) {
		dev_err(dev, "get remote fmt fail!\n");
		return ret;
	}
	/********************TODO**************************
	****************CSIDMA clock = CSI * 2  ***********
	***************************************************/
	dma_cap->csi_dma->sys_clk_freq = src_fmt.format.reserved[0] * 2;
	dev_info(dev, "%s  dam clk%d  %d  \n", __func__, dma_cap->csi_dma->sys_clk_freq, src_fmt.format.code);
	csi_dma_cal_clk_freq(dma_cap);
	if (src_f->width == 0 || src_f->height == 0)
		set_frame_bounds(src_f, src_fmt.format.width, src_fmt.format.height);

	if (!src_f->fmt)
		src_f->fmt = &csi_dma_src_formats[0];

	for (i = 0; i < src_f->fmt->memplanes; i++) {
		if (src_f->bytesperline[i] == 0)
			src_f->bytesperline[i] = src_f->width * src_f->fmt->depth[i] >> 3;
		if (src_f->sizeimage[i] == 0)
			src_f->sizeimage[i] = src_f->bytesperline[i] * src_f->height;
	}

	dev_info(dev, "%s exit dma_cap = %px src_f = %px src_f->fmt = %px\n",
		__func__, dma_cap, src_f, src_f->fmt);

	return 0;
}

static int csi_dma_capture_open(struct file *file)
{
	struct csi_dma_cap_dev *dma_cap = video_drvdata(file);
	struct device *dev = &dma_cap->pdev->dev;
	int ret = -1;

	mutex_lock(&dma_cap->lock);

	if (dma_cap->is_streaming) {
		dev_err(dev, "ISI channel[%d] is busy\n", dma_cap->id);
		goto err;
	}

	ret = v4l2_fh_open(file);
	if (ret) {
		goto err;
	}
err:
	mutex_unlock(&dma_cap->lock);

	return ret;
}

static int csi_dma_capture_release(struct file *file)
{
	struct csi_dma_cap_dev *dma_cap = video_drvdata(file);
	struct device *dev = &dma_cap->pdev->dev;
	struct video_device *vdev = video_devdata(file);
	struct vb2_queue *q = vdev->queue;
	int ret = -1;

	pid_t pid = csi_dma_getpid();

	if ((dma_cap->is_streaming)&&(pid == dma_cap->streaming_pid)) {
		csi_dma_cap_streamoff(file, NULL, q->type);
	}

	mutex_lock(&dma_cap->lock);

	ret = _vb2_fop_release(file, NULL);
	if (ret) {
		dev_err(dev, "%s fail\n", __func__);
	}

	mutex_unlock(&dma_cap->lock);

	return (ret) ? ret : 0;
}

static const struct v4l2_file_operations csi_dma_capture_fops = {
	.owner		= THIS_MODULE,
	.open		= csi_dma_capture_open,
	.release	= csi_dma_capture_release,
	.poll		= vb2_fop_poll,
	.unlocked_ioctl	= video_ioctl2,
	.mmap		= vb2_fop_mmap,
};

/*
 * The video node ioctl operations
 */
static int csi_dma_cap_querycap(struct file *file, void *priv,
				struct v4l2_capability *cap)
{
	struct csi_dma_cap_dev *dma_cap = video_drvdata(file);

	strlcpy(cap->driver, CSI_DMA_CAPTURE_NAME, sizeof(cap->driver));
	strlcpy(cap->card, CSI_DMA_CAPTURE_NAME, sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s.%d",
		 dev_name(&dma_cap->pdev->dev), dma_cap->id);

	cap->device_caps = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_CAPTURE_MPLANE;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

	return 0;
}

static int csi_dma_cap_enum_fmt(struct file *file, void *priv,
				       struct v4l2_fmtdesc *f)
{
	struct csi_dma_fmt *fmt;

	if (f->index >= (int)ARRAY_SIZE(csi_dma_src_formats))
		return -EINVAL;

	fmt = &csi_dma_src_formats[f->index];

	strncpy(f->description, fmt->name, sizeof(f->description) - 1);

	f->pixelformat = fmt->fourcc;

	return 0;
}

static int csi_dma_cap_g_fmt(struct file *file, void *fh,
				    struct v4l2_format *f)
{
	struct csi_dma_cap_dev *dma_cap = video_drvdata(file);
	struct device *dev = &dma_cap->pdev->dev;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct csi_dma_frame *src_f = &dma_cap->src_f;

	dev_info(dev, "%s enter\n", __func__);

	pix->width  = src_f->width;
	pix->height = src_f->height;
	pix->sizeimage = src_f->sizeimage[0];
	pix->bytesperline = src_f->bytesperline[0];

	pix->field = V4L2_FIELD_NONE;
	pix->pixelformat = src_f->fmt->fourcc;
	pix->colorspace = V4L2_COLORSPACE_SRGB;
	pix->ycbcr_enc = V4L2_MAP_YCBCR_ENC_DEFAULT(pix->colorspace);
	pix->quantization = V4L2_QUANTIZATION_FULL_RANGE;

	dev_info(dev, "%s enter\n", __func__);

	return 0;
}

static int csi_dma_cap_g_fmt_mplane(struct file *file, void *fh,
				    struct v4l2_format *f)
{
	struct csi_dma_cap_dev *dma_cap = video_drvdata(file);
	struct device *dev = &dma_cap->pdev->dev;
	struct v4l2_pix_format_mplane *pix = &f->fmt.pix_mp;
	struct csi_dma_frame *src_f = &dma_cap->src_f;
	int i;

	dev_info(dev, "%s enter\n", __func__);

	pix->width = src_f->width;
	pix->height = src_f->height;
	pix->field = V4L2_FIELD_NONE;
	pix->pixelformat = src_f->fmt->fourcc;
	pix->colorspace = V4L2_COLORSPACE_SRGB;
	pix->ycbcr_enc = V4L2_MAP_YCBCR_ENC_DEFAULT(pix->colorspace);
	pix->quantization = V4L2_QUANTIZATION_FULL_RANGE;
	pix->num_planes = src_f->fmt->memplanes;

	for (i = 0; i < pix->num_planes; ++i) {
		pix->plane_fmt[i].bytesperline = src_f->bytesperline[i];
		pix->plane_fmt[i].sizeimage = src_f->sizeimage[i];
	}

	dev_info(dev, "%s exit\n", __func__);

	return 0;
}

static int csi_dma_cap_try_fmt(struct file *file, void *fh,
				      struct v4l2_format *f)
{
	struct csi_dma_cap_dev *dma_cap = video_drvdata(file);
	struct device *dev = &dma_cap->pdev->dev;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct csi_dma_fmt *fmt;
	int i;

	dev_info(dev, "%s enter\n", __func__);

	for (i = 0; i < ARRAY_SIZE(csi_dma_src_formats); i++) {
		fmt = &csi_dma_src_formats[i];
		if (fmt->fourcc == pix->pixelformat)
			break;
	}

	dev_info(dev, "%s size{%d %d}\n", __func__, pix->width, pix->height);

	if (pix->width > CSI_DMA_4K) {
		dev_info(dev, "%s Max Resolution is 4K\n",
				__func__);
		pix->width = CSI_DMA_4K;
	}

	pix->pixelformat = fmt->fourcc;
	pix->field = V4L2_FIELD_NONE;
	pix->colorspace = V4L2_COLORSPACE_SRGB;
	pix->ycbcr_enc = V4L2_MAP_YCBCR_ENC_DEFAULT(pix->colorspace);
	pix->quantization = V4L2_QUANTIZATION_FULL_RANGE;

	pix->bytesperline = (ALIGN(pix->width, CSI_DMA_ALIGN) * fmt->depth[0]) >> 3;

	if (0 == pix->sizeimage) {
		pix->sizeimage = pix->bytesperline * pix->height;
	}

	dev_info(dev, "%s exit\n", __func__);

	return 0;
}

static int csi_dma_cap_try_fmt_mplane(struct file *file, void *fh,
				      struct v4l2_format *f)
{
	struct csi_dma_cap_dev *dma_cap = video_drvdata(file);
	struct v4l2_pix_format_mplane *pix = &f->fmt.pix_mp;
	struct csi_dma_fmt *fmt;
	struct device *dev = &dma_cap->pdev->dev;
	int i;

	dev_info(dev, "%s enter\n", __func__);

	for (i = 0; i < ARRAY_SIZE(csi_dma_src_formats); i++) {
		fmt = &csi_dma_src_formats[i];
		if (fmt->fourcc == pix->pixelformat) {
			dev_err(dev, "%s i = %d\n",__func__, i);
			break;
		}
	}

	if (i >= ARRAY_SIZE(csi_dma_src_formats)) {
		dev_err(dev, "format(%.4s) is not support!\n",
			 (char *)&pix->pixelformat);
		return -EINVAL;
	}

	if (pix->width <= 0 || pix->height <= 0) {
		dev_err(dev,"%s, W/H=(%d, %d) is not valid\n",
				__func__, pix->width, pix->height);
		return -EINVAL;
	}

	if (pix->width > CSI_DMA_4K) {
		dev_err(dev,"%s Max resolution is 4K\n",
				__func__);
		pix->width = CSI_DMA_4K;
	}

	pix->num_planes = fmt->memplanes;
	pix->pixelformat = fmt->fourcc;
	pix->field = V4L2_FIELD_NONE;
	pix->colorspace = V4L2_COLORSPACE_SRGB;
	pix->ycbcr_enc = V4L2_MAP_YCBCR_ENC_DEFAULT(pix->colorspace);
	pix->quantization = V4L2_QUANTIZATION_FULL_RANGE;
	memset(pix->reserved, 0x00, sizeof(pix->reserved));

	for (i = 0; i < pix->num_planes; i++) {

		pix->plane_fmt[i].bytesperline =
				(ALIGN(pix->width, CSI_DMA_ALIGN) * fmt->depth[i]) >> 3;

		if (0 == pix->plane_fmt[i].sizeimage) {
			if ((i != 0) && ((V4L2_PIX_FMT_NV12 == pix->pixelformat) ||
					(V4L2_PIX_FMT_NV21 == pix->pixelformat))) {
				pix->plane_fmt[i].sizeimage =
						(pix->plane_fmt[i].bytesperline * pix->height) >> 1;
			} else {
				pix->plane_fmt[i].sizeimage =
						pix->plane_fmt[i].bytesperline * pix->height;
			}
		}
	}

	dev_info(dev, "%s exit\n", __func__);

	return 0;
}

/* Update input frame size and formate  */
static int csi_dma_source_fmt_init(struct csi_dma_cap_dev *dma_cap)
{
	struct csi_dma_frame *src_f = &dma_cap->src_f;
	struct v4l2_subdev_format src_fmt;
	struct v4l2_subdev *src_sd;
	struct device *dev = &dma_cap->pdev->dev;
	int ret;

	dev_info(dev, "%s enter\n", __func__);

	ret = dma_cap_fmt_init(dma_cap);
	if (ret) {
		return -1;
	}

	src_sd = dma_cap->source_subdev;
	if (!src_sd) {
		dev_err(dev, "remote link not setup\n");
		return -EINVAL;
	}

	src_fmt.pad = dma_cap->source_pad;
	src_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	src_fmt.format.code = src_f->fmt->mbus_code;
	src_fmt.format.width = src_f->width;
	src_fmt.format.height = src_f->height;

	ret = v4l2_subdev_call(src_sd, pad, set_fmt, NULL, &src_fmt);
	if (ret < 0 && ret != -ENOIOCTLCMD) {
		dev_err(dev, "set remote fmt fail!\n");
		return ret;
	}

	/* Pixel link master will transfer format to RGB32 or YUV32 */
	src_f->fmt = csi_dma_get_src_fmt(dma_cap, &src_fmt);
	set_frame_bounds(src_f, src_fmt.format.width, src_fmt.format.height);
	dev_info(dev, "%s exit\n", __func__);

	return 0;
}

static int csi_dma_cap_s_fmt(struct file *file, void *priv,
					struct v4l2_format *f)
{
	struct csi_dma_cap_dev *dma_cap = video_drvdata(file);
	struct device *dev = &dma_cap->pdev->dev;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct csi_dma_frame *src_f = &dma_cap->src_f;
	struct csi_dma_fmt *fmt;
	int i, ret;

	struct vb2_queue *q = &dma_cap->vb2_q;
	struct v4l2_pix_format_mplane *cap_pix = &dma_cap->pix;

	dev_info(dev, "%s enter\n", __func__);
	cap_pix->num_planes = 1;
	memset(q, 0, sizeof(*q));

	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	q->drv_priv = dma_cap;
	q->ops = &sky1_cap_vb2_qops;
	q->mem_ops = &vb2_dma_contig_memops;
	q->buf_struct_size = sizeof(struct csi_dma_buffer);
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;

	q->lock = &dma_cap->qlock;

	ret = vb2_queue_init(q);
	if (ret) {
		dev_info(dev, "create vb2 queue failed\n");
		return ret;
	}

	dev_info(dev, "%s, fmt=0x%X\n",
			__func__, pix->pixelformat);
	if (vb2_is_busy(&dma_cap->vb2_q)) {
		dev_info(dev, "vb2_is_busy");
		return -EBUSY;
	}

	/* Check out put format */
	for (i = 0; i < ARRAY_SIZE(csi_dma_src_formats); i++) {
		fmt = &csi_dma_src_formats[i];
		if (pix && fmt->fourcc == pix->pixelformat)
			break;
	}

	if (i >= ARRAY_SIZE(csi_dma_src_formats)) {
		dev_info(dev, "format(%.4s) is not support!\n",
				(char *)&pix->pixelformat);
		return -EINVAL;
	}

	/* update out put frame size and formate */
	if (pix->height <= 0 || pix->width <= 0) {
		dev_info(dev, "height %d width %d",pix->height, pix->width);
		return -EINVAL;
	}

	ret = csi_dma_cap_try_fmt(file, priv, f);
	if (ret) {
		dev_info(dev,"csi_dma_cap_try_fmt ret %d",ret);
		return ret;
	}

	src_f->fmt = fmt;
	src_f->height = pix->height;
	src_f->width = pix->width;

	src_f->bytesperline[0] = pix->bytesperline;
	src_f->sizeimage[0]    = pix->sizeimage;

	memcpy(&dma_cap->pix, pix, sizeof(*pix));
	set_frame_bounds(src_f, pix->width, pix->height);

	dev_info(dev, "%s exit\n", __func__);

	return 0;
}

static int csi_dma_cap_s_fmt_mplane(struct file *file, void *priv,
				    struct v4l2_format *f)
{
	struct csi_dma_cap_dev *dma_cap = video_drvdata(file);
	struct device *dev = &dma_cap->pdev->dev;
	struct v4l2_pix_format_mplane *pix = &f->fmt.pix_mp;
	struct csi_dma_frame *src_f = &dma_cap->src_f;
	struct csi_dma_fmt *fmt;
	int i, ret;

	struct vb2_queue *q = &dma_cap->vb2_q;
	struct v4l2_pix_format_mplane *cap_pix = &dma_cap->pix;

	dev_info(dev, "%s enter\n", __func__);
	cap_pix->num_planes = 2;
	memset(q, 0, sizeof(*q));

	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	q->drv_priv = dma_cap;
	q->ops = &sky1_cap_vb2_qops;
	q->mem_ops = &vb2_dma_contig_memops;
	q->buf_struct_size = sizeof(struct csi_dma_buffer);
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->lock = &dma_cap->qlock;

	ret = vb2_queue_init(q);
	if (ret) {
		dev_info(dev, "create vb2 queue failed\n");
		return ret;
	}

	/* Step1: Check format with output support format list.
	 * Step2: Update output frame information.
	 * Step3: Checkout the format whether is supported by remote subdev
	 * Step3.1: If Yes, call remote subdev set_fmt.
	 * Step3.2: If NO, call remote subdev get_fmt.
	 * Step4: Update input frame information.
	 * Step5: Update mxc isi channel configuration.
	 */

	dev_dbg(dev, "%s, fmt=0x%X\n", __func__, pix->pixelformat);
	if (vb2_is_busy(&dma_cap->vb2_q))
		return -EBUSY;

	/* Check out put format */
	for (i = 0; i < ARRAY_SIZE(csi_dma_src_formats); i++) {
		fmt = &csi_dma_src_formats[i];
		if (pix && fmt->fourcc == pix->pixelformat)
			break;
	}

	if (i >= ARRAY_SIZE(csi_dma_src_formats)) {
		dev_err(dev,
			"format(%.4s) is not support!\n", (char *)&pix->pixelformat);
		return -EINVAL;
	}

	/* update out put frame size and formate */
	if (pix->height <= 0 || pix->width <= 0)
		return -EINVAL;

	ret = csi_dma_cap_try_fmt_mplane(file, priv, f);
	if (ret)
		return ret;

	src_f->fmt = fmt;
	src_f->height = pix->height;
	src_f->width = pix->width;

	dev_info(dev, "%s size{%d %d}\n", __func__, src_f->width, src_f->height);
	for (i = 0; i < pix->num_planes; i++) {
		src_f->bytesperline[i] = pix->plane_fmt[i].bytesperline;
		src_f->sizeimage[i]    = pix->plane_fmt[i].sizeimage;
	}

	memcpy(&dma_cap->pix, pix, sizeof(*pix));
	set_frame_bounds(src_f, pix->width, pix->height);

	dev_info(dev, "%s exit src_f=%px, src_f->fmt=%px\n", __func__, src_f, src_f->fmt);

	return 0;
}

int csi_dma_config_rcsu(struct csi_dma_cap_dev *dma_cap)
{
	struct csi_dma_dev *csi_dma = dma_cap->csi_dma;
	struct csi2rx_priv *mipi_csi2_dev = v4l2_subdev_to_csi2rx(dma_cap->source_subdev);

	/*here we select the mipi-csi2 dev id equal to the csi_dma id */
	csi_dma->rcsu_dev->chan_mux_sel(csi_dma->rcsu_dev,csi_dma->id,mipi_csi2_dev->id);
	csi_dma->rcsu_dev->dphy_psm_config(csi_dma->rcsu_dev);

	return 0;
}

static int csi_dma_config_parm(struct csi_dma_cap_dev *dma_cap)
{
	struct device *dev = &dma_cap->pdev->dev;
	int ret;

	dev_info(dev, "%s enter\n", __func__);
	ret = csi_dma_source_fmt_init(dma_cap);
	if (ret < 0)
		return -EINVAL;

	csi_dma_config_rcsu(dma_cap);
	dev_info(dev, "%s exit\n", __func__);

	return 0;
}

static int csi_dma_cap_g_parm(struct file *file, void *fh,
			      struct v4l2_streamparm *a)
{
	struct csi_dma_cap_dev *dma_cap = video_drvdata(file);
	return v4l2_g_parm_cap(video_devdata(file), dma_cap->source_subdev, a);
}

static int csi_dma_cap_s_parm(struct file *file, void *fh,
			      struct v4l2_streamparm *a)
{
	struct csi_dma_cap_dev *dma_cap = video_drvdata(file);
	return v4l2_s_parm_cap(video_devdata(file), dma_cap->source_subdev, a);
}

void csi_dma_s_stream(struct csi_dma_cap_dev *dma_cap, int enable)
{
	struct csi_dma_dev *csi_dma = dma_cap->csi_dma;
	struct device *dev = &dma_cap->pdev->dev;

	dev_info(dev, "%s enter enable = %d dma_cap = %px csi_dma = %px\n",
		__func__, enable, dma_cap, csi_dma);

	if (enable) {
		csi_dma_bridge_start(csi_dma, &dma_cap->src_f);
		csi_dma->stream_on = 1;
	} else {
		csi_dma_bridge_stop(csi_dma);
		csi_dma->stream_on = 0;
		dev_info(dev, "%s csi_dma stop\n", __func__);
	}

	dev_info(dev, "%s exit\n", __func__);

	return;
}

static int csi_dma_cap_streamon(struct file *file, void *priv,
				enum v4l2_buf_type type)
{
	struct csi_dma_cap_dev *dma_cap = video_drvdata(file);
	struct device *dev = &dma_cap->pdev->dev;
	struct v4l2_subdev *src_sd;
	int ret;

	dev_info(dev, "%s enter\n", __func__);

	mutex_lock(&dma_cap->lock);

	if (dma_cap->is_streaming) {
		dev_err(dev, "ISI channel[%d] is streaming\n", dma_cap->id);
		mutex_unlock(&dma_cap->lock);
		return -EBUSY;
	}

	src_sd = dma_cap->source_subdev;
	v4l2_subdev_call(src_sd, core, s_power, 1);
	if (ret) {
		dev_err(dev, "Call subdev s_power fail!\n");
		mutex_unlock(&dma_cap->lock);
		return ret;
	}

	dev_info(dev,"csi_dma get sync \n");

	pm_runtime_get_sync(dev);

	ret = csi_dma_config_parm(dma_cap);
	if (ret < 0)
		goto power;

	ret = vb2_ioctl_streamon(file, priv, type);
	if (ret < 0) {
		dev_err(dev, "%s fail to streamon ret = %d\n", __func__, ret);
		goto power;
	}

	csi_dma_s_stream(dma_cap,1);

	ret = csi_dma_pipeline_enable(dma_cap, 1);
	if (ret < 0)
		goto power;

	dma_cap->is_streaming = 1;
	dma_cap->streaming_pid = csi_dma_getpid();

	mutex_unlock(&dma_cap->lock);

	dev_info(dev, "%s exit\n", __func__);

	return 0;
power:
	mutex_unlock(&dma_cap->lock);
	dev_info(dev, "%s exit power\n", __func__);
	v4l2_subdev_call(src_sd, core, s_power, 0);
	return ret;
}

static int csi_dma_cap_streamoff(struct file *file, void *priv,
				 enum v4l2_buf_type type)
{
	struct csi_dma_cap_dev *dma_cap = video_drvdata(file);
	struct device *dev = &dma_cap->pdev->dev;
	struct v4l2_subdev *src_sd = dma_cap->source_subdev;
	int ret;

	dev_info(dev, "%s enter\n", __func__);

	mutex_lock(&dma_cap->lock);

	if (dma_cap->is_streaming == 0) {
		dev_err(dev, "dam_cap [%d] has stopped\n", dma_cap->id);
		mutex_unlock(&dma_cap->lock);
		return -EBUSY;
	}

	ret = csi_dma_pipeline_enable(dma_cap, 0);
	if (ret) {
		dev_err(dev, "dam_cap [%d] pipeline stop err \n", dma_cap->id);
		goto power;
	}

	ret = vb2_ioctl_streamoff(file, priv, type);
	if(ret){
		dev_err(dev, "dam_cap [%d] vb2 stream off err \n", dma_cap->id);
		goto power;
	}

	csi_dma_s_stream(dma_cap,0);
power:
	dma_cap->is_streaming = 0;
	dma_cap->streaming_pid = 0;
	v4l2_subdev_call(src_sd, core, s_power, 0);
	dev_info(dev, "csi_dma pm put sync\n");

	pm_runtime_put_sync(dev);
	mutex_unlock(&dma_cap->lock);

	dev_info(dev, "%s exit\n", __func__);
	return ret;
}

static int csi_dma_cap_g_selection(struct file *file, void *fh,
				   struct v4l2_selection *s)
{
	struct csi_dma_cap_dev *dma_cap = video_drvdata(file);
	struct device *dev = &dma_cap->pdev->dev;
	struct csi_dma_frame *f = &dma_cap->src_f;

	dev_info(dev, "%s enter\n", __func__);

	if (s->type != V4L2_BUF_TYPE_VIDEO_OUTPUT &&
	    s->type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		dev_err(dev, "%s fail to check s->type = %d\n", __func__, s->type);
		return -EINVAL;
	}

	switch (s->target) {
	case V4L2_SEL_TGT_COMPOSE_DEFAULT:
	case V4L2_SEL_TGT_COMPOSE_BOUNDS:
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = f->width;
		s->r.height = f->height;
		dev_info(dev, "%s exit\n", __func__);
		return 0;
	}

	dev_err(dev, "%s exit error\n", __func__);
	return -EINVAL;
}

static int enclosed_rectangle(struct v4l2_rect *a, struct v4l2_rect *b)
{
	if (a->left < b->left || a->top < b->top)
		return 0;

	if (a->left + a->width > b->left + b->width)
		return 0;

	if (a->top + a->height > b->top + b->height)
		return 0;

	return 1;
}

static int csi_dma_cap_s_selection(struct file *file, void *fh,
				   struct v4l2_selection *s)
{
	struct csi_dma_cap_dev *dma_cap = video_drvdata(file);
	struct device *dev = &dma_cap->pdev->dev;
	struct csi_dma_frame *f;
	struct v4l2_rect rect = s->r;
	unsigned long flags;

	dev_info(dev, "%s enter\n", __func__);
	if (s->type != V4L2_BUF_TYPE_VIDEO_OUTPUT &&
	    s->type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		return -EINVAL;

	if (s->target == V4L2_SEL_TGT_COMPOSE)
		f = &dma_cap->src_f;
	else
		return -EINVAL;

	if (s->flags & V4L2_SEL_FLAG_LE &&
	    !enclosed_rectangle(&rect, &s->r))
		return -ERANGE;

	if (s->flags & V4L2_SEL_FLAG_GE &&
	    !enclosed_rectangle(&s->r, &rect))
		return -ERANGE;

	if ((s->flags & V4L2_SEL_FLAG_LE) &&
	    (s->flags & V4L2_SEL_FLAG_GE) &&
	    (rect.width != s->r.width || rect.height != s->r.height))
		return -ERANGE;

	s->r = rect;
	spin_lock_irqsave(&dma_cap->slock, flags);
	spin_unlock_irqrestore(&dma_cap->slock, flags);

	dev_info(dev, "%s exit\n", __func__);

	return 0;
}

static int csi_dma_cap_enum_framesizes(struct file *file, void *priv,
				       struct v4l2_frmsizeenum *fsize)
{
	struct csi_dma_cap_dev *dma_cap = video_drvdata(file);
	struct device *dev = &dma_cap->pdev->dev;
	struct device_node *parent;
	struct v4l2_subdev *sd = dma_cap->source_subdev;
	struct csi_dma_fmt *fmt;
	struct v4l2_subdev_frame_size_enum fse = {
		.index = fsize->index,
		.which = V4L2_SUBDEV_FORMAT_ACTIVE,
	};

	int ret;

	fmt = csi_dma_find_format(&fsize->pixel_format, NULL, 0);
	if (!fmt || fmt->fourcc != fsize->pixel_format)
		return -EINVAL;
	fse.code = fmt->mbus_code;

	ret = v4l2_subdev_call(sd, pad, enum_frame_size, NULL, &fse);
	if (ret)
		return ret;

	parent = of_get_parent(dma_cap->pdev->dev.of_node);
	if (of_device_is_compatible(parent, "cix,cix-bridge") &&
			(fse.max_width > CSI_DMA_4K)) {
		dev_err(dev,"format exceed 4K, CSI-DMA not support\n");
		return -EINVAL;
	}

	if (fse.min_width == fse.max_width &&
	    fse.min_height == fse.max_height) {
		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = fse.min_width;
		fsize->discrete.height = fse.min_height;
	} else {
		fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;
		fsize->stepwise.min_width = fse.min_width;
		fsize->stepwise.max_width = fse.max_width;
		fsize->stepwise.min_height = fse.min_height;
		fsize->stepwise.max_height = fse.max_height;
		fsize->stepwise.step_width = 1;
		fsize->stepwise.step_height = 1;
	}

	return 0;
}

static int csi_dma_cap_enum_frameintervals(struct file *file, void *fh,
					   struct v4l2_frmivalenum *interval)
{
	struct csi_dma_cap_dev *dma_cap = video_drvdata(file);
	struct device *dev = &dma_cap->pdev->dev;
	struct device_node *parent;
	struct v4l2_subdev *sd = dma_cap->source_subdev;
	struct csi_dma_fmt *fmt;
	struct v4l2_subdev_frame_interval_enum fie = {
		.index = interval->index,
		.width = interval->width,
		.height = interval->height,
		.which = V4L2_SUBDEV_FORMAT_ACTIVE,
	};
	int ret;

	dev_info(dev, "%s enter\n", __func__);

	fmt = csi_dma_find_format(&interval->pixel_format, NULL, 0);
	if (!fmt || fmt->fourcc != interval->pixel_format)
		return -EINVAL;

	fie.code = fmt->mbus_code;
	ret = v4l2_subdev_call(sd, pad, enum_frame_interval, NULL, &fie);
	if (ret)
		return ret;

	parent = of_get_parent(dma_cap->pdev->dev.of_node);
	if (of_device_is_compatible(parent, "cix,cix-bridge")
			&& (fie.width > CSI_DMA_4K))
		return -EINVAL;

	interval->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	interval->discrete = fie.interval;

	dev_info(dev, "%s exit\n", __func__);

	return 0;
}

static const struct v4l2_ioctl_ops csi_dma_capture_ioctl_ops = {
	.vidioc_querycap		= csi_dma_cap_querycap,
	.vidioc_enum_fmt_vid_cap	= csi_dma_cap_enum_fmt,
	.vidioc_try_fmt_vid_cap		= csi_dma_cap_try_fmt,
	.vidioc_try_fmt_vid_cap_mplane	= csi_dma_cap_try_fmt_mplane,
	.vidioc_s_fmt_vid_cap		= csi_dma_cap_s_fmt,
	.vidioc_g_fmt_vid_cap		= csi_dma_cap_g_fmt,
	.vidioc_s_fmt_vid_cap_mplane	= csi_dma_cap_s_fmt_mplane,
	.vidioc_g_fmt_vid_cap_mplane	= csi_dma_cap_g_fmt_mplane,

	.vidioc_reqbufs			= vb2_ioctl_reqbufs,
	.vidioc_querybuf		= vb2_ioctl_querybuf,
	.vidioc_qbuf			= vb2_ioctl_qbuf,
	.vidioc_dqbuf			= vb2_ioctl_dqbuf,
	.vidioc_expbuf			= vb2_ioctl_expbuf,
	.vidioc_prepare_buf		= vb2_ioctl_prepare_buf,
	.vidioc_create_bufs		= vb2_ioctl_create_bufs,

	.vidioc_g_parm			= csi_dma_cap_g_parm,
	.vidioc_s_parm			= csi_dma_cap_s_parm,

	.vidioc_streamon		= csi_dma_cap_streamon,
	.vidioc_streamoff		= csi_dma_cap_streamoff,

	.vidioc_g_selection		= csi_dma_cap_g_selection,
	.vidioc_s_selection		= csi_dma_cap_s_selection,

	.vidioc_enum_framesizes = csi_dma_cap_enum_framesizes,
	.vidioc_enum_frameintervals = csi_dma_cap_enum_frameintervals,

	.vidioc_subscribe_event   = v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

static int csi_dma_register_cap_device(struct csi_dma_cap_dev *dma_cap,
				       struct v4l2_device *v4l2_dev)
{
	struct video_device *vdev = &dma_cap->vdev;
	struct vb2_queue *q = &dma_cap->vb2_q;
	struct device *dev = &dma_cap->pdev->dev;
	int ret = -ENOMEM;

	dev_info(dev, "%s enter\n", __func__);

	memset(vdev, 0, sizeof(*vdev));
	snprintf(vdev->name, sizeof(vdev->name), "cix-bridge.%d.capture", dma_cap->id);

	vdev->fops	= &csi_dma_capture_fops;
	vdev->ioctl_ops	= &csi_dma_capture_ioctl_ops;
	vdev->v4l2_dev	= v4l2_dev;
	vdev->minor	= -1;
	vdev->release	= video_device_release_empty;
	vdev->queue	= q;

	vdev->lock	= &dma_cap->vlock;

	vdev->device_caps = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_CAPTURE_MPLANE;
	video_set_drvdata(vdev, dma_cap);

	INIT_LIST_HEAD(&dma_cap->out_pending);
	INIT_LIST_HEAD(&dma_cap->out_active);
	INIT_LIST_HEAD(&dma_cap->out_discard);

	dma_cap->cap_pad.flags = MEDIA_PAD_FL_SINK;
	ret = media_entity_pads_init(&vdev->entity, 1, &dma_cap->cap_pad);
	if (ret)
		goto err_free_ctx;

	ret = video_register_device(vdev, VFL_TYPE_VIDEO, dma_cap->id);
	if (ret)
		goto err_me_cleanup;

	v4l2_ctrl_handler_init(&dma_cap->ctrls.handler,1);
	vdev->ctrl_handler = &dma_cap->ctrls.handler;
	dev_info(dev, "Registered %s as /dev/%s\n",
				vdev->name, video_device_node_name(vdev));

	return 0;

err_me_cleanup:
	media_entity_cleanup(&vdev->entity);
err_free_ctx:
	return ret;
}

int csi_dma_register_stream(struct csi_dma_dev *csi_dma)
{
	struct csi_dma_cap_dev *dma_cap;

	dma_cap = devm_kzalloc(csi_dma->dev, sizeof(*dma_cap), GFP_KERNEL);
	if (!dma_cap) {
		dev_err(csi_dma->dev, "devm_kzalloc failed\n");
		return -ENOMEM;
	}

	csi_dma->dma_cap = dma_cap;
	dma_cap->csi_dma = csi_dma;
	dma_cap->id = csi_dma->id;
	dma_cap->pdev = csi_dma->pdev; 
	dma_cap->dev = csi_dma->dev;

	spin_lock_init(&dma_cap->slock);
	mutex_init(&dma_cap->lock);
	mutex_init(&dma_cap->qlock);
	mutex_init(&dma_cap->vlock);

	return csi_dma_register_cap_device(dma_cap,&csi_dma->v4l2_dev);
}

void csi_dma_unregister_stream(struct csi_dma_dev *csi_dma)
{
	struct csi_dma_cap_dev *dma_cap = csi_dma->dma_cap;
	struct device *dev = &dma_cap->pdev->dev;
	struct video_device *vdev= &dma_cap->vdev;

	mutex_lock(&dma_cap->lock);

	if (video_is_registered(vdev)) {
		video_unregister_device(vdev);
		media_entity_cleanup(&vdev->entity);
	}

	mutex_unlock(&dma_cap->lock);
	dev_info(dev, "%s exit\n", __func__);

	return ;
}

MODULE_AUTHOR("Cixtech, Inc.");
MODULE_DESCRIPTION("SKY csi_dma cap driver");
MODULE_LICENSE("GPL");
