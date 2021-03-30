
/*
 ******************************************************************************
 *
 * sunxi_isp.c
 *
 * Hawkview ISP - sunxi_isp.c module
 *
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Yang Feng   	2014/12/11	ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ctrls.h>
#include "../platform/platform_cfg.h"
#include "bsp_isp.h"
#include "sunxi_isp.h"
#include "../vin-video/vin_core.h"
#include "../utility/vin_io.h"

#define ISP_MODULE_NAME "vin_isp"

#if defined CONFIG_ARCH_SUN50I
#define ISP_HEIGHT_16B_ALIGN 0
#else
#define ISP_HEIGHT_16B_ALIGN 1
#endif

extern unsigned int isp_reparse_flag;

static LIST_HEAD(isp_drv_list);

#define MIN_IN_WIDTH			32
#define MIN_IN_HEIGHT			32
#define MAX_IN_WIDTH			4095
#define MAX_IN_HEIGHT			4095

#define MIN_OUT_WIDTH			16
#define MIN_OUT_HEIGHT			2
#define MAX_OUT_WIDTH			4095
#define MAX_OUT_HEIGHT			4095

static const struct isp_pix_fmt sunxi_isp_formats[] = {
	{
		.name = "RAW8 (GRBG)",
		.fourcc = V4L2_PIX_FMT_SGRBG8,
		.depth = {8},
		.color = 0,
		.memplanes = 1,
		.mbus_code = V4L2_MBUS_FMT_SGRBG8_1X8,
	}, {
		.name = "RAW10 (GRBG)",
		.fourcc = V4L2_PIX_FMT_SGRBG10,
		.depth = {10},
		.color = 0,
		.memplanes = 1,
		.mbus_code = V4L2_MBUS_FMT_SGRBG10_1X10,
	}, {
		.name = "RAW12 (GRBG)",
		.fourcc = V4L2_PIX_FMT_SGRBG12,
		.depth = {12},
		.color = 0,
		.memplanes = 1,
		.mbus_code = V4L2_MBUS_FMT_SGRBG12_1X12,
	},
};
void isp_isr_bh_handle(struct work_struct *work);
static int sunxi_isp_subdev_s_power(struct v4l2_subdev *sd, int enable)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	if (!isp->use_isp)
		return 0;

	return 0;
}

static int sunxi_isp_subdev_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	if (!isp->use_isp)
		return 0;

	if (isp->use_cnt > 1)
		return 0;

	if (enable) {
		bsp_isp_init(isp->id, &isp->isp_init_para);
		bsp_isp_enable(isp->id);
		bsp_isp_set_para_ready(isp->id);
		bsp_isp_clr_irq_status(isp->id, ISP_IRQ_EN_ALL);
		bsp_isp_irq_enable(isp->id, FINISH_INT_EN | SRC0_FIFO_INT_EN);
		if (isp->capture_mode == V4L2_MODE_IMAGE) {
			bsp_isp_image_capture_start(isp->id);
		} else {
			bsp_isp_video_capture_start(isp->id);
		}
	} else {
		if (isp->capture_mode == V4L2_MODE_IMAGE) {
			bsp_isp_image_capture_stop(isp->id);
		} else {
			bsp_isp_video_capture_stop(isp->id);
		}
		bsp_isp_irq_disable(isp->id, ISP_IRQ_EN_ALL);
		bsp_isp_clr_irq_status(isp->id, ISP_IRQ_EN_ALL);
		bsp_isp_disable(isp->id);
		bsp_isp_exit(isp->id);
	}

	return 0;
}

static struct v4l2_rect *__isp_get_crop(struct isp_dev *isp,
					struct v4l2_subdev_fh *fh,
					enum v4l2_subdev_format_whence which)
{
	if (which == V4L2_SUBDEV_FORMAT_TRY)
		return fh ? v4l2_subdev_get_try_crop(fh, ISP_PAD_SINK) : NULL;
	else
		return &isp->crop.request;
}

static void __isp_try_crop(const struct v4l2_mbus_framefmt *sink,
			 const struct v4l2_mbus_framefmt *source,
			 struct v4l2_rect *crop)
{
	unsigned int min_width = source->width;
	unsigned int min_height = source->height;
	unsigned int max_width = sink->width;
	unsigned int max_height = sink->height;

	crop->width = clamp_t(u32, crop->width, min_width, max_width);
	crop->height = clamp_t(u32, crop->height, min_height, max_height);

	/* Crop can not go beyond of the input rectangle */
	crop->left = clamp_t(u32, crop->left, 0, sink->width - MIN_IN_WIDTH);
	crop->width =
	    clamp_t(u32, crop->width, MIN_IN_WIDTH, sink->width - crop->left);
	crop->top = clamp_t(u32, crop->top, 0, sink->height - MIN_IN_HEIGHT);
	crop->height =
	    clamp_t(u32, crop->height, MIN_IN_HEIGHT, sink->height - crop->top);
}

static const struct isp_pix_fmt *__isp_find_format(const u32 *
							 pixelformat,
							 const u32 *mbus_code,
							 int index)
{
	const struct isp_pix_fmt *fmt, *def_fmt = NULL;
	unsigned int i;
	int id = 0;

	if (index >= (int)ARRAY_SIZE(sunxi_isp_formats))
		return NULL;

	for (i = 0; i < ARRAY_SIZE(sunxi_isp_formats); ++i) {
		fmt = &sunxi_isp_formats[i];
		if (pixelformat && fmt->fourcc == *pixelformat)
			return fmt;
		if (mbus_code && fmt->mbus_code == *mbus_code)
			return fmt;
		if (index == id)
			def_fmt = fmt;
		id++;
	}
	return def_fmt;
}

static struct v4l2_mbus_framefmt *__isp_get_format(struct isp_dev *isp,
					struct v4l2_subdev_fh *fh, u32 pad,
					enum v4l2_subdev_format_whence which)
{
	if (which == V4L2_SUBDEV_FORMAT_TRY)
		return fh ? v4l2_subdev_get_try_format(fh, pad) : NULL;
	return &isp->format[pad];
}

static void __isp_try_format(struct isp_dev *isp, struct v4l2_subdev_fh *fh,
			   unsigned int pad, struct v4l2_mbus_framefmt *fmt,
			   enum v4l2_subdev_format_whence which)
{
	struct v4l2_mbus_framefmt *format;
	struct v4l2_rect crop;

	switch (pad) {
	case ISP_PAD_SINK:
		fmt->width =
		    clamp_t(u32, fmt->width, MIN_IN_WIDTH, MAX_IN_WIDTH);
		fmt->height =
		    clamp_t(u32, fmt->height, MIN_IN_HEIGHT, MAX_IN_HEIGHT);
		break;
	case ISP_PAD_SOURCE:
		format = __isp_get_format(isp, fh, ISP_PAD_SINK, which);
		fmt->code = format->code;

		crop = *__isp_get_crop(isp, fh, which);
		break;
	}
	fmt->colorspace = V4L2_COLORSPACE_JPEG;
	fmt->field = V4L2_FIELD_NONE;
}

static int sunxi_isp_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	isp->capture_mode = cp->capturemode;

	return 0;
}

static int sunxi_isp_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->capturemode = isp->capture_mode;

	return 0;
}

static int sunxi_isp_enum_mbus_code(struct v4l2_subdev *sd,
				    struct v4l2_subdev_fh *fh,
				    struct v4l2_subdev_mbus_code_enum *code)
{
	const struct isp_pix_fmt *fmt;

	fmt = __isp_find_format(NULL, NULL, code->index);
	if (!fmt)
		return -EINVAL;
	code->code = fmt->mbus_code;
	return 0;
}

static int sunxi_isp_enum_frame_size(struct v4l2_subdev *sd,
				     struct v4l2_subdev_fh *fh,
				     struct v4l2_subdev_frame_size_enum *fse)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt format;

	if (fse->code != V4L2_MBUS_FMT_SGRBG10_1X10
	    && fse->code != V4L2_MBUS_FMT_SGRBG12_1X12)
		return -EINVAL;

	if (fse->index != 0)
		return -EINVAL;

	format.code = fse->code;
	format.width = 1;
	format.height = 1;
	__isp_try_format(isp, fh, fse->pad, &format, V4L2_SUBDEV_FORMAT_ACTIVE);
	fse->min_width = format.width;
	fse->min_height = format.height;

	if (format.code != fse->code)
		return -EINVAL;

	format.code = fse->code;
	format.width = -1;
	format.height = -1;
	__isp_try_format(isp, fh, fse->pad, &format, V4L2_SUBDEV_FORMAT_ACTIVE);
	fse->max_width = format.width;
	fse->max_height = format.height;

	return 0;
}

static int sunxi_isp_subdev_get_fmt(struct v4l2_subdev *sd,
				    struct v4l2_subdev_fh *fh,
				    struct v4l2_subdev_format *fmt)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf;

	mf = __isp_get_format(isp, fh, fmt->pad, fmt->which);
	if (!mf)
		return -EINVAL;

	mutex_lock(&isp->subdev_lock);
	fmt->format = *mf;
	mutex_unlock(&isp->subdev_lock);
	return 0;
}

static int sunxi_isp_subdev_set_fmt(struct v4l2_subdev *sd,
				    struct v4l2_subdev_fh *fh,
				    struct v4l2_subdev_format *fmt)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format;
	struct v4l2_rect *crop;

	format = __isp_get_format(isp, fh, fmt->pad, fmt->which);
	if (format == NULL)
		return -EINVAL;
	vin_log(VIN_LOG_FMT, "%s %d*%d %x %d\n", __func__, fmt->format.width,
		  fmt->format.height, fmt->format.code, fmt->format.field);
	__isp_try_format(isp, fh, fmt->pad, &fmt->format, fmt->which);
	*format = fmt->format;

	if (fmt->pad == ISP_PAD_SINK) {
		/* reset crop rectangle */
		crop = __isp_get_crop(isp, fh, fmt->which);
		crop->left = 0;
		crop->top = 0;
		crop->width = fmt->format.width;
		crop->height = fmt->format.height;

		/* Propagate the format from sink to source */
		format =
		    __isp_get_format(isp, fh, ISP_PAD_SOURCE, fmt->which);
		*format = fmt->format;
		__isp_try_format(isp, fh, ISP_PAD_SOURCE, format, fmt->which);
	}

	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE) {
		isp->crop.active = isp->crop.request;
	}

	return 0;

}

int sunxi_isp_init(struct v4l2_subdev *sd, u32 val)
{
	int ret = 0;
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	if (isp->use_cnt > 1)
		return 0;
	vin_print("%s, val = %d.\n", __func__, val);
	if (val) {
		bsp_isp_set_dma_load_addr(isp->id, (unsigned long)isp->isp_load.dma_addr);
		bsp_isp_set_dma_saved_addr(isp->id, (unsigned long)isp->isp_save.dma_addr);
		ret = isp_resource_request(sd);
		if (ret) {
			vin_err("isp_resource_request error at %s\n", __func__);
			return ret;
		}
		INIT_WORK(&isp->isp_isr_bh_task, isp_isr_bh_handle);

		/*alternate isp setting*/
		update_isp_setting(sd);
	} else {
		flush_work(&isp->isp_isr_bh_task);
		isp_resource_release(sd);
	}
	return 0;
}
static int sunxi_isp_subdev_cropcap(struct v4l2_subdev *sd,
				    struct v4l2_cropcap *a)
{
	return 0;
}

static long sunxi_isp_subdev_ioctl(struct v4l2_subdev *sd, unsigned int cmd,
				   void *arg)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	int ret = 0;

	if (isp->use_cnt > 1)
		return 0;

	switch (cmd) {
	case VIDIOC_SUNXI_ISP_MAIN_CH_CFG:
		break;
	default:
		return -ENOIOCTLCMD;
	}

	return ret;
}

static int sunxi_isp_subdev_get_selection(struct v4l2_subdev *sd,
					  struct v4l2_subdev_fh *fh,
					  struct v4l2_subdev_selection *sel)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format_source, *format_sink;

	if (sel->pad != ISP_PAD_SINK)
		return -EINVAL;

	format_sink = __isp_get_format(isp, fh, ISP_PAD_SINK, sel->which);
	format_source =
	    __isp_get_format(isp, fh, ISP_PAD_SOURCE, sel->which);

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP_BOUNDS:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = INT_MAX;
		sel->r.height = INT_MAX;

		__isp_try_crop(format_sink, format_source, &sel->r);
		break;

	case V4L2_SEL_TGT_CROP:
		sel->r = *__isp_get_crop(isp, fh, sel->which);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_isp_subdev_set_selection(struct v4l2_subdev *sd,
					  struct v4l2_subdev_fh *fh,
					  struct v4l2_subdev_selection *sel)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format_sink, *format_source;

	if (sel->target != V4L2_SEL_TGT_CROP || sel->pad != ISP_PAD_SINK)
		return -EINVAL;

	format_sink = __isp_get_format(isp, fh, ISP_PAD_SINK, sel->which);
	format_source =
	    __isp_get_format(isp, fh, ISP_PAD_SOURCE, sel->which);

	vin_print("%s: L = %d, T = %d, W = %d, H = %d\n", __func__,
		  sel->r.left, sel->r.top, sel->r.width, sel->r.height);

	vin_print("%s: input = %dx%d, output = %dx%d\n", __func__,
		  format_sink->width, format_sink->height,
		  format_source->width, format_source->height);

	__isp_try_crop(format_sink, format_source, &sel->r);
	*__isp_get_crop(isp, fh, sel->which) = sel->r;

	if (sel->which == V4L2_SUBDEV_FORMAT_TRY)
		return 0;

	isp->crop.active = sel->r;

	return 0;
}

static int sunxi_isp_subdev_get_crop(struct v4l2_subdev *sd,
			struct v4l2_subdev_fh *fh,
			struct v4l2_subdev_crop *crop)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	if (crop->pad != ISP_PAD_SINK)
		return -EINVAL;

	crop->rect = *__isp_get_crop(isp, fh, crop->which);
	return 0;
}

static int sunxi_isp_subdev_set_crop(struct v4l2_subdev *sd,
			struct v4l2_subdev_fh *fh,
			struct v4l2_subdev_crop *crop)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format_sink, *format_source;

	if (crop->pad != ISP_PAD_SINK)
		return -EINVAL;

	format_sink = __isp_get_format(isp, fh, ISP_PAD_SINK, crop->which);
	format_source =
	    __isp_get_format(isp, fh, ISP_PAD_SOURCE, crop->which);

	vin_print("%s: L = %d, T = %d, W = %d, H = %d\n", __func__,
		  crop->rect.left, crop->rect.top,
		  crop->rect.width, crop->rect.height);

	vin_print("%s: input = %dx%d, output = %dx%d\n", __func__,
		  format_sink->width, format_sink->height,
		  format_source->width, format_source->height);

	__isp_try_crop(format_sink, format_source, &crop->rect);
	*__isp_get_crop(isp, fh, crop->which) = crop->rect;

	if (crop->which == V4L2_SUBDEV_FORMAT_TRY)
		return 0;

	isp->crop.active = crop->rect;

	return 0;
}

static void sunxi_isp_stat_parse(struct isp_dev *isp)
{
	void *va = NULL;
	struct isp_gen_settings *isp_gen = isp->isp_gen;
	if (NULL == isp->h3a_stat.active_buf) {
		vin_log(VIN_LOG_ISP, "stat active buf is NULL, please enable\n");
		return;
	}
	va = isp->h3a_stat.active_buf->virt_addr;
	isp_gen->stat.hist_buf = (void *) (va);
	isp_gen->stat.ae_buf =  (void *) (va + ISP_STAT_AE_MEM_OFS);
	isp_gen->stat.awb_buf = (void *) (va + ISP_STAT_AWB_MEM_OFS);
	isp_gen->stat.af_buf = (void *) (va + ISP_STAT_AF_MEM_OFS);
	isp_gen->stat.afs_buf = (void *) (va + ISP_STAT_AFS_MEM_OFS);
	isp_gen->stat.awb_win_buf = (void *) (va + ISP_STAT_AWB_WIN_MEM_OFS);
}

void sunxi_isp_vsync_isr(struct v4l2_subdev *sd)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_event event;

	memset(&event, 0, sizeof(event));
	event.type = V4L2_EVENT_VSYNC;
	event.id = 0;
	v4l2_event_queue(isp->subdev.devnode, &event);
}

void sunxi_isp_frame_sync_isr(struct v4l2_subdev *sd)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct vin_pipeline *pipe = NULL;
	struct v4l2_event event;

	vin_isp_stat_isr_frame_sync(&isp->h3a_stat);

	memset(&event, 0, sizeof(event));
	event.type = V4L2_EVENT_FRAME_SYNC;
	event.id = 0;
	v4l2_event_queue(isp->subdev.devnode, &event);

	pipe = to_vin_pipeline(&sd->entity);
	atomic_inc(&pipe->frame_number);

	pipe = to_vin_pipeline(&isp->h3a_stat.sd.entity);
	vin_isp_stat_isr(&isp->h3a_stat);
	/* you should enable the isp stat buf first,
	** when you want to get the stat buf separate.
	** user can get stat buf from external ioctl.
	*/
	sunxi_isp_stat_parse(isp);
}

int sunxi_isp_subscribe_event(struct v4l2_subdev *sd,
				  struct v4l2_fh *fh,
				  struct v4l2_event_subscription *sub)
{
	vin_log(VIN_LOG_ISP, "%s id = %d\n", __func__, sub->id);
	if (sub->type == V4L2_EVENT_CTRL)
		return v4l2_ctrl_subdev_subscribe_event(sd, fh, sub);
	else
		return v4l2_event_subscribe(fh, sub, 1, NULL);
}

int sunxi_isp_unsubscribe_event(struct v4l2_subdev *sd,
				    struct v4l2_fh *fh,
				    struct v4l2_event_subscription *sub)
{
	vin_log(VIN_LOG_ISP, "%s id = %d\n", __func__, sub->id);
	return v4l2_event_unsubscribe(fh, sub);
}

static const struct v4l2_subdev_core_ops sunxi_isp_core_ops = {
	.s_power = sunxi_isp_subdev_s_power,
	.init = sunxi_isp_init,
	.ioctl = sunxi_isp_subdev_ioctl,
	.subscribe_event = sunxi_isp_subscribe_event,
	.unsubscribe_event = sunxi_isp_unsubscribe_event,
};

static const struct v4l2_subdev_video_ops sunxi_isp_subdev_video_ops = {
	.s_parm = sunxi_isp_s_parm,
	.g_parm = sunxi_isp_g_parm,
	.s_stream = sunxi_isp_subdev_s_stream,
	.cropcap = sunxi_isp_subdev_cropcap,
};

static const struct v4l2_subdev_pad_ops sunxi_isp_subdev_pad_ops = {
	.enum_mbus_code = sunxi_isp_enum_mbus_code,
	.enum_frame_size = sunxi_isp_enum_frame_size,
	.get_fmt = sunxi_isp_subdev_get_fmt,
	.set_fmt = sunxi_isp_subdev_set_fmt,
	.get_selection = sunxi_isp_subdev_get_selection,
	.set_selection = sunxi_isp_subdev_set_selection,
	.get_crop = sunxi_isp_subdev_get_crop,
	.set_crop = sunxi_isp_subdev_set_crop,
};

static struct v4l2_subdev_ops sunxi_isp_subdev_ops = {
	.core = &sunxi_isp_core_ops,
	.video = &sunxi_isp_subdev_video_ops,
	.pad = &sunxi_isp_subdev_pad_ops,
};

/*
static int sunxi_isp_subdev_registered(struct v4l2_subdev *sd)
{
	struct vin_core *vinc = v4l2_get_subdevdata(sd);
	int ret = 0;
	return ret;
}

static void sunxi_isp_subdev_unregistered(struct v4l2_subdev *sd)
{
	struct vin_core *vinc = v4l2_get_subdevdata(sd);
	return;
}

static const struct v4l2_subdev_internal_ops sunxi_isp_sd_internal_ops = {
	.registered = sunxi_isp_subdev_registered,
	.unregistered = sunxi_isp_subdev_unregistered,
};
*/

/* media operations */
static const struct media_entity_operations isp_media_ops = {
};


static int __sunxi_isp_ctrl(struct isp_dev *isp, struct v4l2_ctrl *ctrl)
{
	int ret = 0;

	if (ctrl->flags & V4L2_CTRL_FLAG_INACTIVE)
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_HFLIP:
		break;
	case V4L2_CID_VFLIP:
		break;
	case V4L2_CID_ROTATE:
		break;
	default:
		break;
	}
	return ret;
}

#define ctrl_to_sunxi_isp(ctrl) \
	container_of(ctrl->handler, struct isp_dev, ctrls.handler)

static int sunxi_isp_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct isp_dev *isp = ctrl_to_sunxi_isp(ctrl);
	unsigned long flags;
	int ret;

	if (isp->use_cnt > 1)
		return 0;
	vin_log(VIN_LOG_ISP, "id = %d, val = %d, cur.val = %d\n",
		  ctrl->id, ctrl->val, ctrl->cur.val);
	spin_lock_irqsave(&isp->slock, flags);
	ret = __sunxi_isp_ctrl(isp, ctrl);
	spin_unlock_irqrestore(&isp->slock, flags);

	return ret;
}

static const struct v4l2_ctrl_ops sunxi_isp_ctrl_ops = {
	.s_ctrl = sunxi_isp_s_ctrl,
};

static const struct v4l2_ctrl_config ae_win_ctrls[] = {
	{
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AE_WIN_X1,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AE_WIN_Y1,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AE_WIN_X2,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
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
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AF_WIN_X1,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AF_WIN_Y1,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AF_WIN_X2,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
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

static const struct v4l2_ctrl_config wb_gain_ctrls[] = {
	{
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_R_GAIN,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 1024,
		.step = 1,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_GR_GAIN,
		.name = "GR GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 1024,
		.step = 1,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_GB_GAIN,
		.name = "GB GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 1024,
		.step = 1,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_B_GAIN,
		.name = "B GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 1024,
		.step = 1,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}
};

int __isp_init_subdev(struct isp_dev *isp)
{
	const struct v4l2_ctrl_ops *ops = &sunxi_isp_ctrl_ops;
	struct v4l2_ctrl_handler *handler = &isp->ctrls.handler;
	struct v4l2_subdev *sd = &isp->subdev;
	struct sunxi_isp_ctrls *ctrls = &isp->ctrls;
	int i, ret;
	mutex_init(&isp->subdev_lock);

	v4l2_subdev_init(sd, &sunxi_isp_subdev_ops);
	sd->grp_id = VIN_GRP_ID_ISP;
	sd->flags |= V4L2_SUBDEV_FL_HAS_EVENTS | V4L2_SUBDEV_FL_HAS_DEVNODE;
	snprintf(sd->name, sizeof(sd->name), "sunxi_isp.%u", isp->id);
	v4l2_set_subdevdata(sd, isp);

	v4l2_ctrl_handler_init(handler, 3 + ARRAY_SIZE(ae_win_ctrls)
		+ ARRAY_SIZE(af_win_ctrls) + ARRAY_SIZE(wb_gain_ctrls));

	ctrls->rotate =
	    v4l2_ctrl_new_std(handler, ops, V4L2_CID_ROTATE, 0, 270, 90, 0);
	ctrls->hflip =
	    v4l2_ctrl_new_std(handler, ops, V4L2_CID_HFLIP, 0, 1, 1, 0);
	ctrls->vflip =
	    v4l2_ctrl_new_std(handler, ops, V4L2_CID_VFLIP, 0, 1, 1, 0);

	for (i = 0; i < ARRAY_SIZE(wb_gain_ctrls); i++)
		ctrls->wb_gain[i] = v4l2_ctrl_new_custom(handler,
						&wb_gain_ctrls[i], NULL);
	v4l2_ctrl_cluster(ARRAY_SIZE(wb_gain_ctrls), &ctrls->wb_gain[0]);

	for (i = 0; i < ARRAY_SIZE(ae_win_ctrls); i++)
		ctrls->ae_win[i] = v4l2_ctrl_new_custom(handler,
						&ae_win_ctrls[i], NULL);
	v4l2_ctrl_cluster(ARRAY_SIZE(ae_win_ctrls), &ctrls->ae_win[0]);

	for (i = 0; i < ARRAY_SIZE(af_win_ctrls); i++)
		ctrls->af_win[i] = v4l2_ctrl_new_custom(handler,
						&af_win_ctrls[i], NULL);
	v4l2_ctrl_cluster(ARRAY_SIZE(af_win_ctrls), &ctrls->af_win[0]);

	if (handler->error) {
		return handler->error;
	}

	/*sd->entity->ops = &isp_media_ops;*/
	isp->isp_pads[ISP_PAD_SINK].flags = MEDIA_PAD_FL_SINK;
	isp->isp_pads[ISP_PAD_SOURCE_ST].flags = MEDIA_PAD_FL_SOURCE;
	isp->isp_pads[ISP_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.type = MEDIA_ENT_T_V4L2_SUBDEV;

	ret = media_entity_init(&sd->entity, ISP_PAD_NUM, isp->isp_pads, 0);
	if (ret < 0)
		return ret;

	sd->ctrl_handler = handler;
	/*sd->internal_ops = &sunxi_isp_sd_internal_ops;*/
	return 0;
}

void update_isp_setting(struct v4l2_subdev *sd)
{
	struct vin_core *vinc = sd_to_vin_core(sd);
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct sensor_instance *inst = get_valid_sensor(vinc);
	printk("isp->use_cnt = %d\n", isp->use_cnt);
	if (isp->use_cnt > 1)
		return;

	isp->isp_gen->module_cfg.isp_platform_id = vinc->platform_id;
	if (inst->is_bayer_raw) {
		isp->isp_gen->module_cfg.lut_src0_table =
		    isp->isp_tbl.isp_def_lut_tbl_vaddr;
		isp->isp_gen->module_cfg.gamma_table =
		    isp->isp_tbl.isp_gamma_tbl_vaddr;
		isp->isp_gen->module_cfg.lens_table =
		    isp->isp_tbl.isp_lsc_tbl_vaddr;
		isp->isp_gen->module_cfg.linear_table =
		    isp->isp_tbl.isp_linear_tbl_vaddr;
		isp->isp_gen->module_cfg.disc_table =
		    isp->isp_tbl.isp_disc_tbl_vaddr;
		if (inst->is_isp_used)
			bsp_isp_update_lut_lens_gamma_table(isp->id, &isp->isp_tbl);
	}
	isp->isp_gen->module_cfg.drc_table =
	    isp->isp_tbl.isp_drc_tbl_vaddr;
	if (inst->is_isp_used)
		bsp_isp_update_drc_table(isp->id, &isp->isp_tbl);
}

/*static int isp_resource_request(struct isp_dev *isp)*/
int isp_resource_request(struct v4l2_subdev *sd)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	void *pa_base, *va_base, *dma_base;
	int ret;

	if (isp->use_cnt > 1)
		return 0;

	/*requeset for isp table and statistic buffer*/
	if (isp->use_isp && isp->is_raw) {
		isp->isp_lut_tbl.size =
		    ISP_LINEAR_LUT_LENS_GAMMA_MEM_SIZE;
		ret = os_mem_alloc(&isp->isp_lut_tbl);
		if (!ret) {
			pa_base = isp->isp_lut_tbl.phy_addr;
			va_base = isp->isp_lut_tbl.vir_addr;
			dma_base = isp->isp_lut_tbl.dma_addr;
			isp->isp_tbl.isp_def_lut_tbl_paddr =
			    (void *)(pa_base + ISP_LUT_MEM_OFS);
			isp->isp_tbl.isp_def_lut_tbl_dma_addr =
			    (void *)(dma_base + ISP_LUT_MEM_OFS);
			isp->isp_tbl.isp_def_lut_tbl_vaddr =
			    (void *)(va_base + ISP_LUT_MEM_OFS);
			isp->isp_tbl.isp_lsc_tbl_paddr =
			    (void *)(pa_base + ISP_LENS_MEM_OFS);
			isp->isp_tbl.isp_lsc_tbl_dma_addr =
			    (void *)(dma_base + ISP_LENS_MEM_OFS);
			isp->isp_tbl.isp_lsc_tbl_vaddr =
			    (void *)(va_base + ISP_LENS_MEM_OFS);
			isp->isp_tbl.isp_gamma_tbl_paddr =
			    (void *)(pa_base + ISP_GAMMA_MEM_OFS);
			isp->isp_tbl.isp_gamma_tbl_dma_addr =
			    (void *)(dma_base + ISP_GAMMA_MEM_OFS);
			isp->isp_tbl.isp_gamma_tbl_vaddr =
			    (void *)(va_base + ISP_GAMMA_MEM_OFS);

			isp->isp_tbl.isp_linear_tbl_paddr =
			    (void *)(pa_base + ISP_LINEAR_MEM_OFS);
			isp->isp_tbl.isp_linear_tbl_dma_addr =
			    (void *)(dma_base + ISP_LINEAR_MEM_OFS);
			isp->isp_tbl.isp_linear_tbl_vaddr =
			    (void *)(va_base + ISP_LINEAR_MEM_OFS);
			vin_log(VIN_LOG_ISP, "isp_def_lut_tbl_vaddr = %p\n",
				isp->isp_tbl.isp_def_lut_tbl_vaddr);
			vin_log(VIN_LOG_ISP, "isp_lsc_tbl_vaddr = %p\n",
				isp->isp_tbl.isp_lsc_tbl_vaddr);
			vin_log(VIN_LOG_ISP, "isp_gamma_tbl_vaddr = %p\n",
				isp->isp_tbl.isp_gamma_tbl_vaddr);
		} else {
			vin_err("isp lookup table request failed!\n");
			return -ENOMEM;
		}

		if (isp->use_isp && isp->is_raw) {
			isp->isp_drc_tbl.size = ISP_DRC_DISC_MEM_SIZE;
			ret = os_mem_alloc(&isp->isp_drc_tbl);
			if (!ret) {
				pa_base = isp->isp_drc_tbl.phy_addr;
				va_base = isp->isp_drc_tbl.vir_addr;
				dma_base = isp->isp_drc_tbl.dma_addr;

				isp->isp_tbl.isp_drc_tbl_paddr =
				    (void *)(pa_base + ISP_DRC_MEM_OFS);
				isp->isp_tbl.isp_drc_tbl_dma_addr =
				    (void *)(dma_base + ISP_DRC_MEM_OFS);
				isp->isp_tbl.isp_drc_tbl_vaddr =
				    (void *)(va_base + ISP_DRC_MEM_OFS);

				isp->isp_tbl.isp_disc_tbl_paddr =
				    (void *)(pa_base + ISP_DISC_MEM_OFS);
				isp->isp_tbl.isp_disc_tbl_dma_addr =
				    (void *)(dma_base + ISP_DISC_MEM_OFS);
				isp->isp_tbl.isp_disc_tbl_vaddr =
				    (void *)(va_base + ISP_DISC_MEM_OFS);

				vin_log(VIN_LOG_ISP, "isp_drc_tbl_vaddr = %p\n",
					isp->isp_tbl.isp_drc_tbl_vaddr);
			} else {
				vin_err("isp drc table request pa failed!\n");
				return -ENOMEM;
			}
		}
	}

	return 0;
}

/*static void isp_resource_release(struct isp_dev *isp)*/
void isp_resource_release(struct v4l2_subdev *sd)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	if (isp->use_cnt == 0)
		return;

	/* release isp table and statistic buffer */
	if (isp->use_isp && isp->is_raw) {
		os_mem_free(&isp->isp_lut_tbl);
		os_mem_free(&isp->isp_drc_tbl);
	}
}

static int isp_resource_alloc(struct isp_dev *isp)
{
	int ret = 0;
	isp->isp_load.size = ISP_LOAD_REG_SIZE;
	isp->isp_save.size = ISP_SAVED_REG_SIZE;

	os_mem_alloc(&isp->isp_load);
	os_mem_alloc(&isp->isp_save);

	if (isp->isp_load.phy_addr != NULL) {
		if (!isp->isp_load.vir_addr) {
			vin_err("isp load regs va requset failed!\n");
			return -ENOMEM;
		}
	} else {
		vin_err("isp load regs pa requset failed!\n");
		return -ENOMEM;
	}

	if (isp->isp_save.phy_addr != NULL) {
		if (!isp->isp_save.vir_addr) {
			vin_err("isp save regs va requset failed!\n");
			return -ENOMEM;
		}
	} else {
		vin_err("isp save regs pa requset failed!\n");
		return -ENOMEM;
	}
	return ret;

}
static void isp_resource_free(struct isp_dev *isp)
{
	os_mem_free(&isp->isp_load);
	os_mem_free(&isp->isp_save);
}

#define ISP_REGS(n) (void __iomem *)(ISP_REGS_BASE + n)

void isp_isr_bh_handle(struct work_struct *work)
{
	struct isp_dev *isp =
	    container_of(work, struct isp_dev, isp_isr_bh_task);

	FUNCTION_LOG;

	if (isp->is_raw) {
		mutex_lock(&isp->subdev_lock);
		if (1 == isp_reparse_flag) {
			vin_print("ISP reparse ini file!\n");
			isp_reparse_flag = 0;
		} else if (2 == isp_reparse_flag) {
			vin_reg_set(ISP_REGS(0x10), (1 << 20));
		} else if (3 == isp_reparse_flag) {
			vin_reg_clr_set(ISP_REGS(0x10), (0xF << 16), (1 << 16));
			vin_reg_set(ISP_REGS(0x10), (1 << 20));
		} else if (4 == isp_reparse_flag) {
			vin_reg_clr(ISP_REGS(0x10), (1 << 20));
			vin_reg_clr(ISP_REGS(0x10), (0xF << 16));
		}
		mutex_unlock(&isp->subdev_lock);
	}

	FUNCTION_LOG;
}

void __isp_isr(struct v4l2_subdev *sd)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	unsigned long flags;

	FUNCTION_LOG;
	vin_log(VIN_LOG_ISP, "isp interrupt!!!\n");

	FUNCTION_LOG;
	spin_lock_irqsave(&isp->slock, flags);
	FUNCTION_LOG;

	if (isp->use_isp) {
		if (bsp_isp_get_irq_status(isp->id, SRC0_FIFO_INT_EN)) {
			vin_err("isp source0 fifo overflow\n");
			bsp_isp_clr_irq_status(isp->id, SRC0_FIFO_INT_EN);
			goto unlock;
		}
	}
	if (isp->capture_mode == V4L2_MODE_IMAGE) {
		if (isp->use_isp)
			bsp_isp_irq_disable(isp->id, FINISH_INT_EN);
		goto unlock;
	} else {
		if (isp->use_isp)
			bsp_isp_irq_disable(isp->id, FINISH_INT_EN);
	}
unlock:
	spin_unlock_irqrestore(&isp->slock, flags);

	if (isp->use_isp && bsp_isp_get_irq_status(isp->id, FINISH_INT_EN)) {
		/* if(bsp_isp_get_para_ready()) */
		{
			vin_log(VIN_LOG_ISP, "call tasklet schedule! \n");
			bsp_isp_clr_para_ready(isp->id);
			schedule_work(&isp->isp_isr_bh_task);
			bsp_isp_set_para_ready(isp->id);
		}
	}
	if (isp->use_isp) {
		bsp_isp_clr_irq_status(isp->id, FINISH_INT_EN);
		bsp_isp_irq_enable(isp->id, FINISH_INT_EN);
	}
	return;
}
static int isp_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct isp_dev *isp = NULL;
	int ret = 0, i;

	if (np == NULL) {
		vin_err("ISP failed to get of node\n");
		return -ENODEV;
	}

	isp = kzalloc(sizeof(struct isp_dev), GFP_KERNEL);
	if (!isp) {
		ret = -ENOMEM;
		goto ekzalloc;
	}

	isp->isp_gen = kzalloc(sizeof(struct isp_gen_settings), GFP_KERNEL);

	if (!isp->isp_gen) {
		vin_err("request isp_gen_settings mem failed!\n");
		return -ENOMEM;
	}

	pdev->id = of_alias_get_id(np, "isp");
	if (pdev->id < 0) {
		vin_err("ISP failed to get alias id\n");
		ret = -EINVAL;
		goto freedev;
	}

	isp->id = pdev->id;
	isp->pdev = pdev;

#ifdef DEFINE_ISP_REGS
	isp->base = of_iomap(np, 0);
	if (!isp->base) {
		ret = -EIO;
		goto freedev;
	}
#else
	isp->base = kzalloc(0x300, GFP_KERNEL);
	if (!isp->base) {
		ret = -EIO;
		goto freedev;
	}
#endif
	list_add_tail(&isp->isp_list, &isp_drv_list);
	__isp_init_subdev(isp);

	spin_lock_init(&isp->slock);
	init_waitqueue_head(&isp->wait);

	if (isp_resource_alloc(isp) < 0) {
		ret = -ENOMEM;
		goto ehwinit;
	}

	ret = vin_isp_h3a_init(isp);
	if (ret < 0) {
		vin_err("VIN H3A initialization failed\n");
			goto free_res;
	}

	bsp_isp_init_platform(SUNXI_PLATFORM_ID);
	bsp_isp_set_base_addr(isp->id, (unsigned long)isp->base);
	bsp_isp_set_map_load_addr(isp->id, (unsigned long)isp->isp_load.vir_addr);
	bsp_isp_set_map_saved_addr(isp->id, (unsigned long)isp->isp_save.vir_addr);
	memset(isp->isp_load.vir_addr, 0, ISP_LOAD_REG_SIZE);
	memset(isp->isp_save.vir_addr, 0, ISP_SAVED_REG_SIZE);
	isp->isp_init_para.isp_src_ch_mode = ISP_SINGLE_CH;
	for (i = 0; i < MAX_ISP_SRC_CH_NUM; i++)
		isp->isp_init_para.isp_src_ch_en[i] = 0;
	isp->isp_init_para.isp_src_ch_en[0] = 1;
	platform_set_drvdata(pdev, isp);
	vin_print("isp%d probe end!\n", isp->id);
	return 0;
free_res:
	isp_resource_free(isp);
ehwinit:
#ifdef DEFINE_ISP_REGS
	iounmap(isp->base);
#else
	kfree(isp->base);
#endif
	list_del(&isp->isp_list);
freedev:
	kfree(isp);
ekzalloc:
	vin_print("isp probe err!\n");
	return ret;
}

static int isp_remove(struct platform_device *pdev)
{
	struct isp_dev *isp = platform_get_drvdata(pdev);
	struct v4l2_subdev *sd = &isp->subdev;

	platform_set_drvdata(pdev, NULL);
	v4l2_ctrl_handler_free(sd->ctrl_handler);
	v4l2_set_subdevdata(sd, NULL);

	isp_resource_free(isp);
#ifdef DEFINE_ISP_REGS
	if (isp->base)
		iounmap(isp->base);
#else
	if (isp->base)
		kfree(isp->base);
#endif
	list_del(&isp->isp_list);
	kfree(isp->isp_gen);
	vin_isp_h3a_cleanup(isp);
	kfree(isp);
	return 0;
}

static const struct of_device_id sunxi_isp_match[] = {
	{.compatible = "allwinner,sunxi-isp",},
	{},
};

static struct platform_driver isp_platform_driver = {
	.probe = isp_probe,
	.remove = isp_remove,
	.driver = {
		   .name = ISP_MODULE_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = sunxi_isp_match,
		   }
};

void sunxi_isp_dump_regs(struct v4l2_subdev *sd)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	int i = 0;
	printk("vin dump ISP regs :\n");
	for (i = 0; i < 0x40; i = i + 4) {
		if (i % 0x10 == 0)
			printk("0x%08x:  ", i);
		printk("0x%08x, ", readl(isp->base + i));
		if (i % 0x10 == 0xc)
			printk("\n");
	}
	for (i = 0x40; i < 0x240; i = i + 4) {
		if (i % 0x10 == 0)
			printk("0x%08x:  ", i);
		printk("0x%08x, ", readl(isp->isp_load.vir_addr + i));
		if (i % 0x10 == 0xc)
			printk("\n");
	}
}

struct v4l2_subdev *sunxi_isp_get_subdev(int id)
{
	struct isp_dev *isp;
	list_for_each_entry(isp, &isp_drv_list, isp_list) {
		if (isp->id == id) {
			isp->use_cnt++;
			return &isp->subdev;
		}
	}
	return NULL;
}

struct v4l2_subdev *sunxi_stat_get_subdev(int id)
{
	struct isp_dev *isp;
	list_for_each_entry(isp, &isp_drv_list, isp_list) {
		if (isp->id == id) {
			return &isp->h3a_stat.sd;
		}
	}
	return NULL;
}

int sunxi_isp_platform_register(void)
{
	return platform_driver_register(&isp_platform_driver);
}

void sunxi_isp_platform_unregister(void)
{
	platform_driver_unregister(&isp_platform_driver);
	vin_print("isp_exit end\n");
}
