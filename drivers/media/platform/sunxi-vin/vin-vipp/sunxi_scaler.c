/*
 * linux-4.9/drivers/media/platform/sunxi-vin/vin-vipp/sunxi_scaler.c
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include "../platform/platform_cfg.h"
#include "sunxi_scaler.h"
#include "../vin-video/vin_core.h"
#include "vipp_reg.h"

#define SCALER_MODULE_NAME "vin_scaler"

#define ALIGN_4B(x)	(((x) + (3)) & ~(3))
#define ALIGN_2B(x)	(((x) + (1)) & ~(1))

static LIST_HEAD(scaler_drv_list);

#define MIN_IN_WIDTH			192
#define MIN_IN_HEIGHT			128
#define MAX_IN_WIDTH			4224
#define MAX_IN_HEIGHT			4224

#define MIN_OUT_WIDTH			16
#define MIN_OUT_HEIGHT			10
#define MAX_OUT_WIDTH			4224
#define MAX_OUT_HEIGHT			4224

static struct v4l2_mbus_framefmt *__scaler_get_format(struct scaler_dev *scaler,
				struct v4l2_subdev_pad_config *cfg,
				unsigned int pad,
				enum v4l2_subdev_format_whence which)
{
	if (which == V4L2_SUBDEV_FORMAT_TRY)
		return cfg ? &cfg->try_fmt : NULL;
	else
		return &scaler->formats[pad];
}

static struct v4l2_rect *__scaler_get_crop(struct scaler_dev *scaler,
					   struct v4l2_subdev_pad_config *cfg,
					   enum v4l2_subdev_format_whence which)
{
	if (which == V4L2_SUBDEV_FORMAT_TRY)
		return cfg ? &cfg->try_crop : NULL;
	else
		return &scaler->crop.request;
}

static void __scaler_try_crop(const struct v4l2_mbus_framefmt *sink,
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

static int __scaler_w_shift(int x_ratio, int y_ratio)
{
	int m, n;
	int sum_weight = 0;
	int weight_shift = -8;
	int xr = (x_ratio >> 8) + 1;
	int yr = (y_ratio >> 8) + 1;

	for (m = 0; m <= xr; m++) {
		for (n = 0; n <= yr; n++) {
			sum_weight += (y_ratio - abs((n << 8) - (yr << 7)))
				* (x_ratio - abs((m << 8) - (xr << 7)));
		}
	}
	sum_weight >>= 8;
	while (sum_weight != 0) {
		weight_shift++;
		sum_weight >>= 1;
	}
	return weight_shift;
}

static void __scaler_calc_ratios(struct scaler_dev *scaler,
			       struct v4l2_rect *input,
			       struct v4l2_mbus_framefmt *output,
			       struct scaler_para *para)
{
	unsigned int width;
	unsigned int height;
	unsigned int r_min;

	output->width = clamp_t(u32, output->width, MIN_IN_WIDTH, input->width);
	output->height =
	    clamp_t(u32, output->height, MIN_IN_HEIGHT, input->height);

	para->xratio = 256 * input->width / output->width;
	para->yratio = 256 * input->height / output->height;
	para->xratio = clamp_t(u32, para->xratio, 256, 2048);
	para->yratio = clamp_t(u32, para->yratio, 256, 2048);

	r_min = min(para->xratio, para->yratio);
#ifdef CROP_AFTER_SCALER
	width = ALIGN_4B(256 * input->width / r_min);
	height = ALIGN_2B(256 * input->height / r_min);
	para->xratio = 256 * input->width / width;
	para->yratio = 256 * input->height / height;
	para->xratio = clamp_t(u32, para->xratio, 256, 2048);
	para->yratio = clamp_t(u32, para->yratio, 256, 2048);
	para->width = width;
	para->height = height;
	vin_log(VIN_LOG_SCALER, "para: xr = %d, yr = %d, w = %d, h = %d\n",
		  para->xratio, para->yratio, para->width, para->height);

	output->width = width;
	output->height = height;
#else
	width = ALIGN_4B(output->width * r_min / 256);
	height = ALIGN_2B(output->height * r_min / 256);
	para->xratio = 256 * width / output->width;
	para->yratio = 256 * height / output->height;
	para->xratio = clamp_t(u32, para->xratio, 256, 2048);
	para->yratio = clamp_t(u32, para->yratio, 256, 2048);
	para->width = output->width;
	para->height = output->height;
	vin_log(VIN_LOG_SCALER, "para: xr = %d, yr = %d, w = %d, h = %d\n",
		  para->xratio, para->yratio, para->width, para->height);
	/* Center the new crop rectangle.
	 * crop is before scaler
	 */
	input->left += (input->width - width) / 2;
	input->top += (input->height - height) / 2;
	input->left = ALIGN_4B(input->left);
	input->top = ALIGN_2B(input->top);
	input->width = width;
	input->height = height;
#endif
	vin_log(VIN_LOG_SCALER, "crop: left = %d, top = %d, w = %d, h = %d\n",
		input->left, input->top, input->width, input->height);
}

static void __scaler_try_format(struct scaler_dev *scaler,
			      struct v4l2_subdev_pad_config *cfg, unsigned int pad,
			      struct v4l2_mbus_framefmt *fmt,
			      enum v4l2_subdev_format_whence which)
{
	struct v4l2_mbus_framefmt *format;
	struct scaler_para ratio;
	struct v4l2_rect crop;

	switch (pad) {
	case SCALER_PAD_SINK:
		fmt->width =
		    clamp_t(u32, fmt->width, MIN_IN_WIDTH, MAX_IN_WIDTH);
		fmt->height =
		    clamp_t(u32, fmt->height, MIN_IN_HEIGHT, MAX_IN_HEIGHT);
		break;
	case SCALER_PAD_SOURCE:
		format =
		    __scaler_get_format(scaler, cfg, SCALER_PAD_SINK, which);
		fmt->code = format->code;

		crop = *__scaler_get_crop(scaler, cfg, which);
		__scaler_calc_ratios(scaler, &crop, fmt, &ratio);
		break;
	}
}

static int sunxi_scaler_enum_mbus_code(struct v4l2_subdev *sd,
				       struct v4l2_subdev_pad_config *cfg,
				       struct v4l2_subdev_mbus_code_enum *code)
{
	return 0;
}

static int sunxi_scaler_enum_frame_size(struct v4l2_subdev *sd,
					struct v4l2_subdev_pad_config *cfg,
					struct v4l2_subdev_frame_size_enum *fse)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt format;

	if (fse->index != 0)
		return -EINVAL;

	format.code = fse->code;
	format.width = 1;
	format.height = 1;
	__scaler_try_format(scaler, cfg, fse->pad, &format,
			V4L2_SUBDEV_FORMAT_ACTIVE);
	fse->min_width = format.width;
	fse->min_height = format.height;

	if (format.code != fse->code)
		return -EINVAL;

	format.code = fse->code;
	format.width = -1;
	format.height = -1;
	__scaler_try_format(scaler, cfg, fse->pad, &format,
			V4L2_SUBDEV_FORMAT_ACTIVE);
	fse->max_width = format.width;
	fse->max_height = format.height;

	return 0;
}

static int sunxi_scaler_subdev_get_fmt(struct v4l2_subdev *sd,
				       struct v4l2_subdev_pad_config *cfg,
				       struct v4l2_subdev_format *fmt)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format;

	format = __scaler_get_format(scaler, cfg, fmt->pad, fmt->which);
	if (format == NULL)
		return -EINVAL;

	fmt->format = *format;
	return 0;
}

static int sunxi_scaler_subdev_set_fmt(struct v4l2_subdev *sd,
				       struct v4l2_subdev_pad_config *cfg,
				       struct v4l2_subdev_format *fmt)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format;
	struct v4l2_rect *crop;

	format = __scaler_get_format(scaler, cfg, fmt->pad, fmt->which);
	if (format == NULL)
		return -EINVAL;
	vin_log(VIN_LOG_FMT, "%s %d*%d %x %d\n", __func__, fmt->format.width,
		  fmt->format.height, fmt->format.code, fmt->format.field);
	__scaler_try_format(scaler, cfg, fmt->pad, &fmt->format, fmt->which);
	*format = fmt->format;

	if (fmt->pad == SCALER_PAD_SINK) {
		/* reset crop rectangle */
		crop = __scaler_get_crop(scaler, cfg, fmt->which);
		crop->left = 0;
		crop->top = 0;
		crop->width = fmt->format.width;
		crop->height = fmt->format.height;

		/* Propagate the format from sink to source */
		format = __scaler_get_format(scaler, cfg, SCALER_PAD_SOURCE,
						fmt->which);
		*format = fmt->format;
		__scaler_try_format(scaler, cfg, SCALER_PAD_SOURCE, format,
				  fmt->which);
	}

	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE) {
		scaler->crop.active = scaler->crop.request;
		__scaler_calc_ratios(scaler, &scaler->crop.active, format,
				   &scaler->para);
	}

	/*return the format for other subdev*/
	fmt->format = *format;

	return 0;
}

static int sunxi_scaler_subdev_get_selection(struct v4l2_subdev *sd,
					     struct v4l2_subdev_pad_config *cfg,
					     struct v4l2_subdev_selection *sel)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format_source;
	struct v4l2_mbus_framefmt *format_sink;
	struct scaler_para para;

	vin_log(VIN_LOG_SCALER, "%s\n", __func__);

	if (sel->pad != SCALER_PAD_SINK)
		return -EINVAL;

	format_sink =
	    __scaler_get_format(scaler, cfg, SCALER_PAD_SINK, sel->which);
	format_source =
	    __scaler_get_format(scaler, cfg, SCALER_PAD_SOURCE, sel->which);

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP_BOUNDS:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = INT_MAX;
		sel->r.height = INT_MAX;
		__scaler_try_crop(format_sink, format_source, &sel->r);
		__scaler_calc_ratios(scaler, &sel->r, format_source, &para);
		break;
	case V4L2_SEL_TGT_CROP:
		sel->r = *__scaler_get_crop(scaler, cfg, sel->which);
		__scaler_calc_ratios(scaler, &sel->r, format_source, &para);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_scaler_subdev_set_selection(struct v4l2_subdev *sd,
					     struct v4l2_subdev_pad_config *cfg,
					     struct v4l2_subdev_selection *sel)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format_sink, *format_source;
	struct scaler_para para;

	if (sel->target != V4L2_SEL_TGT_CROP || sel->pad != SCALER_PAD_SINK)
		return -EINVAL;

	format_sink =
	    __scaler_get_format(scaler, cfg, SCALER_PAD_SINK, sel->which);
	format_source =
	    __scaler_get_format(scaler, cfg, SCALER_PAD_SOURCE, sel->which);

	vin_log(VIN_LOG_FMT, "%s: L = %d, T = %d, W = %d, H = %d\n", __func__,
		  sel->r.left, sel->r.top, sel->r.width, sel->r.height);

	vin_log(VIN_LOG_FMT, "%s: input = %dx%d, output = %dx%d\n", __func__,
		  format_sink->width, format_sink->height,
		  format_source->width, format_source->height);

	__scaler_try_crop(format_sink, format_source, &sel->r);
	*__scaler_get_crop(scaler, cfg, sel->which) = sel->r;
	__scaler_calc_ratios(scaler, &sel->r, format_source, &para);

	if (sel->which == V4L2_SUBDEV_FORMAT_TRY)
		return 0;

	scaler->para = para;
	scaler->crop.active = sel->r;
	vin_log(VIN_LOG_SCALER, "active crop: l = %d, t = %d, w = %d, h = %d\n",
		scaler->crop.active.left, scaler->crop.active.top,
		scaler->crop.active.width, scaler->crop.active.height);
	return 0;
}

int sunxi_scaler_subdev_init(struct v4l2_subdev *sd, u32 val)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);

	vin_log(VIN_LOG_SCALER, "%s, val = %d.\n", __func__, val);
	if (val) {
		memset(scaler->vipp_reg.vir_addr, 0, VIPP_REG_SIZE);
		memset(scaler->osd_para.vir_addr, 0, OSD_PARA_SIZE);
		memset(scaler->osd_stat.vir_addr, 0, OSD_STAT_SIZE);
		vipp_set_reg_load_addr(scaler->id, (unsigned long)scaler->vipp_reg.dma_addr);
		vipp_set_osd_para_load_addr(scaler->id, (unsigned long)scaler->osd_para.dma_addr);
		vipp_set_osd_stat_load_addr(scaler->id, (unsigned long)scaler->osd_stat.dma_addr);
		vipp_set_osd_cv_update(scaler->id, NOT_UPDATED);
		vipp_set_osd_ov_update(scaler->id, NOT_UPDATED);
		vipp_set_para_ready(scaler->id, NOT_READY);
	}
	return 0;
}

static int sunxi_scaler_subdev_s_power(struct v4l2_subdev *sd, int enable)
{
	return 0;
}

static int sunxi_scaler_subdev_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf = &scaler->formats[SCALER_PAD_SOURCE];
	struct mbus_framefmt_res *res = (void *)scaler->formats[SCALER_PAD_SOURCE].reserved;
	struct vipp_scaler_config scaler_cfg;
	struct vipp_scaler_size scaler_size;
	struct vipp_crop crop;
	enum vipp_format out_fmt;

	switch (res->res_pix_fmt) {
	case V4L2_PIX_FMT_SBGGR8:
	case V4L2_PIX_FMT_SGBRG8:
	case V4L2_PIX_FMT_SGRBG8:
	case V4L2_PIX_FMT_SRGGB8:
	case V4L2_PIX_FMT_SBGGR10:
	case V4L2_PIX_FMT_SGBRG10:
	case V4L2_PIX_FMT_SGRBG10:
	case V4L2_PIX_FMT_SRGGB10:
	case V4L2_PIX_FMT_SBGGR12:
	case V4L2_PIX_FMT_SGBRG12:
	case V4L2_PIX_FMT_SGRBG12:
	case V4L2_PIX_FMT_SRGGB12:
		vin_log(VIN_LOG_FMT, "%s output fmt is raw, return directly\n", __func__);
		return 0;
	default:
		break;
	}
	if (mf->field == V4L2_FIELD_INTERLACED || mf->field == V4L2_FIELD_TOP ||
	    mf->field == V4L2_FIELD_BOTTOM) {
		vin_log(VIN_LOG_SCALER, "Scaler not support field mode, return directly!\n");
		return 0;
	}

	if (enable) {
		crop.hor = scaler->crop.active.left;
		crop.ver = scaler->crop.active.top;
		crop.width = scaler->crop.active.width;
		crop.height = scaler->crop.active.height;
		vipp_set_crop(scaler->id, &crop);
		scaler_size.sc_width = scaler->para.width;
		scaler_size.sc_height = scaler->para.height;
		vipp_scaler_output_size(scaler->id, &scaler_size);

		switch (res->res_pix_fmt) {
		case V4L2_PIX_FMT_YUV420:
		case V4L2_PIX_FMT_YUV420M:
		case V4L2_PIX_FMT_YVU420:
		case V4L2_PIX_FMT_YVU420M:
		case V4L2_PIX_FMT_NV21:
		case V4L2_PIX_FMT_NV21M:
		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV12M:
		case V4L2_PIX_FMT_FBC:
			out_fmt = YUV420;
			break;
		case V4L2_PIX_FMT_YUV422P:
		case V4L2_PIX_FMT_NV16:
		case V4L2_PIX_FMT_NV61:
		case V4L2_PIX_FMT_NV61M:
		case V4L2_PIX_FMT_NV16M:
			out_fmt = YUV422;
			break;
		default:
			out_fmt = YUV420;
			break;
		}
		if (scaler->is_osd_en)
			scaler_cfg.sc_out_fmt = YUV422;
		else
			scaler_cfg.sc_out_fmt = out_fmt;
		scaler_cfg.sc_x_ratio = scaler->para.xratio;
		scaler_cfg.sc_y_ratio = scaler->para.yratio;
		scaler_cfg.sc_w_shift = __scaler_w_shift(scaler->para.xratio, scaler->para.yratio);
		vipp_scaler_cfg(scaler->id, &scaler_cfg);
		vipp_output_fmt_cfg(scaler->id, out_fmt);
		vipp_scaler_en(scaler->id, 1);
		vipp_set_para_ready(scaler->id, HAS_READY);
		vipp_set_osd_ov_update(scaler->id, HAS_UPDATED);
		vipp_set_osd_cv_update(scaler->id, HAS_UPDATED);
		vipp_top_clk_en(scaler->id, enable);
		vipp_enable(scaler->id);
	} else {
		vipp_disable(scaler->id);
		vipp_top_clk_en(scaler->id, enable);
		vipp_scaler_en(scaler->id, 0);
		vipp_set_para_ready(scaler->id, NOT_READY);
		vipp_set_osd_ov_update(scaler->id, NOT_UPDATED);
		vipp_set_osd_cv_update(scaler->id, NOT_UPDATED);
	}
	vin_log(VIN_LOG_FMT, "vipp%d %s, %d*%d hoff: %d voff: %d xr: %d yr: %d\n",
		scaler->id, enable ? "stream on" : "stream off",
		scaler->para.width, scaler->para.height,
		scaler->crop.active.left, scaler->crop.active.top,
		scaler->para.xratio, scaler->para.yratio);

	return 0;
}

static const struct v4l2_subdev_core_ops sunxi_scaler_subdev_core_ops = {
	.s_power = sunxi_scaler_subdev_s_power,
	.init = sunxi_scaler_subdev_init,
};
static const struct v4l2_subdev_video_ops sunxi_scaler_subdev_video_ops = {
	.s_stream = sunxi_scaler_subdev_s_stream,
};
/* subdev pad operations */
static const struct v4l2_subdev_pad_ops sunxi_scaler_subdev_pad_ops = {
	.enum_mbus_code = sunxi_scaler_enum_mbus_code,
	.enum_frame_size = sunxi_scaler_enum_frame_size,
	.get_fmt = sunxi_scaler_subdev_get_fmt,
	.set_fmt = sunxi_scaler_subdev_set_fmt,
	.get_selection = sunxi_scaler_subdev_get_selection,
	.set_selection = sunxi_scaler_subdev_set_selection,
};

static struct v4l2_subdev_ops sunxi_scaler_subdev_ops = {
	.core = &sunxi_scaler_subdev_core_ops,
	.video = &sunxi_scaler_subdev_video_ops,
	.pad = &sunxi_scaler_subdev_pad_ops,
};

int __scaler_init_subdev(struct scaler_dev *scaler)
{
	int ret;
	struct v4l2_subdev *sd = &scaler->subdev;

	mutex_init(&scaler->subdev_lock);

	v4l2_subdev_init(sd, &sunxi_scaler_subdev_ops);
	sd->grp_id = VIN_GRP_ID_SCALER;
	sd->flags |= V4L2_SUBDEV_FL_HAS_EVENTS | V4L2_SUBDEV_FL_HAS_DEVNODE;
	snprintf(sd->name, sizeof(sd->name), "sunxi_scaler.%u", scaler->id);
	v4l2_set_subdevdata(sd, scaler);

	/*sd->entity->ops = &isp_media_ops;*/
	scaler->scaler_pads[SCALER_PAD_SINK].flags = MEDIA_PAD_FL_SINK;
	scaler->scaler_pads[SCALER_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.function = MEDIA_ENT_F_PROC_VIDEO_SCALER;

	ret = media_entity_pads_init(&sd->entity, SCALER_PAD_NUM,
			scaler->scaler_pads);
	if (ret < 0)
		return ret;
	return 0;
}

static int scaler_resource_alloc(struct scaler_dev *scaler)
{
	int ret = 0;

	scaler->vipp_reg.size = VIPP_REG_SIZE + OSD_PARA_SIZE + OSD_STAT_SIZE;

	ret = os_mem_alloc(&scaler->pdev->dev, &scaler->vipp_reg);
	if (ret < 0) {
		vin_err("scaler regs load addr requset failed!\n");
		return -ENOMEM;
	}

	scaler->osd_para.dma_addr = scaler->vipp_reg.dma_addr + VIPP_REG_SIZE;
	scaler->osd_para.vir_addr = scaler->vipp_reg.vir_addr + VIPP_REG_SIZE;
	scaler->osd_stat.dma_addr = scaler->osd_para.dma_addr + OSD_PARA_SIZE;
	scaler->osd_stat.vir_addr = scaler->osd_para.vir_addr + OSD_PARA_SIZE;

	return 0;
}

static void scaler_resource_free(struct scaler_dev *scaler)
{
	os_mem_free(&scaler->pdev->dev, &scaler->vipp_reg);
}

static int scaler_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct scaler_dev *scaler = NULL;
	int ret = 0;

	if (np == NULL) {
		vin_err("Scaler failed to get of node\n");
		return -ENODEV;
	}

	scaler = kzalloc(sizeof(struct scaler_dev), GFP_KERNEL);
	if (!scaler) {
		ret = -ENOMEM;
		vin_err("sunxi scaler kzalloc failed!\n");
		goto ekzalloc;
	}
	of_property_read_u32(np, "device_id", &pdev->id);
	if (pdev->id < 0) {
		vin_err("Scaler failed to get device id\n");
		ret = -EINVAL;
		goto freedev;
	}

	scaler->id = pdev->id;
	scaler->pdev = pdev;

	scaler->base = of_iomap(np, 0);
	if (!scaler->base) {
		scaler->is_empty = 1;
		scaler->base = kzalloc(0x400, GFP_KERNEL);
		if (!scaler->base) {
			ret = -EIO;
			goto freedev;
		}
	}

	list_add_tail(&scaler->scaler_list, &scaler_drv_list);
	__scaler_init_subdev(scaler);

	spin_lock_init(&scaler->slock);
	init_waitqueue_head(&scaler->wait);

	if (scaler_resource_alloc(scaler) < 0) {
		ret = -ENOMEM;
		goto unmap;
	}

	vipp_set_base_addr(scaler->id, (unsigned long)scaler->base);
	vipp_map_reg_load_addr(scaler->id, (unsigned long)scaler->vipp_reg.vir_addr);
	vipp_map_osd_para_load_addr(scaler->id, (unsigned long)scaler->osd_para.vir_addr);

	platform_set_drvdata(pdev, scaler);

	vin_log(VIN_LOG_SCALER, "scaler%d probe end\n", scaler->id);
	return 0;
unmap:
	if (!scaler->is_empty)
		iounmap(scaler->base);
	else
		kfree(scaler->base);
freedev:
	kfree(scaler);
ekzalloc:
	vin_err("scaler%d probe err!\n", scaler->id);
	return ret;
}

static int scaler_remove(struct platform_device *pdev)
{
	struct scaler_dev *scaler = platform_get_drvdata(pdev);
	struct v4l2_subdev *sd = &scaler->subdev;

	scaler_resource_free(scaler);
	platform_set_drvdata(pdev, NULL);
	v4l2_device_unregister_subdev(sd);
	v4l2_set_subdevdata(sd, NULL);
	list_del(&scaler->scaler_list);
	if (scaler->base) {
		if (!scaler->is_empty)
			iounmap(scaler->base);
		else
			kfree(scaler->base);
	}
	kfree(scaler);
	return 0;
}

static const struct of_device_id sunxi_scaler_match[] = {
	{.compatible = "allwinner,sunxi-scaler",},
	{},
};

static struct platform_driver scaler_platform_driver = {
	.probe    = scaler_probe,
	.remove   = scaler_remove,
	.driver = {
		.name   = SCALER_MODULE_NAME,
		.owner  = THIS_MODULE,
		.of_match_table = sunxi_scaler_match,
	},
};

struct v4l2_subdev *sunxi_scaler_get_subdev(int id)
{
	struct scaler_dev *scaler;

	list_for_each_entry(scaler, &scaler_drv_list, scaler_list) {
		if (scaler->id == id) {
			scaler->use_cnt++;
			return &scaler->subdev;
		}
	}
	return NULL;
}

int sunxi_vipp_get_osd_stat(int id, unsigned int *stat)
{
	struct v4l2_subdev *sd = sunxi_scaler_get_subdev(id);
	struct scaler_dev *vipp = v4l2_get_subdevdata(sd);
	unsigned long long *stat_buf = vipp->osd_stat.vir_addr;
	int i;

	if (stat_buf == NULL || stat == NULL)
		return -EINVAL;

	for (i = 0; i < MAX_OVERLAY_NUM; i++)
		stat[i] = stat_buf[i];

	return 0;
}

int sunxi_osd_change_sc_fmt(int id, enum vipp_format out_fmt, int en)
{
	struct v4l2_subdev *sd = sunxi_scaler_get_subdev(id);
	struct scaler_dev *vipp = v4l2_get_subdevdata(sd);

	vipp->is_osd_en = en;

	vipp_scaler_output_fmt(id, out_fmt);

	return 0;
}

int sunxi_scaler_platform_register(void)
{
	return platform_driver_register(&scaler_platform_driver);
}

void sunxi_scaler_platform_unregister(void)
{
	platform_driver_unregister(&scaler_platform_driver);
	vin_log(VIN_LOG_SCALER, "scaler_exit end\n");
}
