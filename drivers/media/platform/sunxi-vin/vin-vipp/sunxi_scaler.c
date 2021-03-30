
/*
 *****************************************************************************
 *
 * sunxi_scaler.c
 *
 * Hawkview ISP - sunxi_scaler.c module
 *
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Zhao Wei   	2014/12/11	ISP Tuning Tools Support
 *
 ******************************************************************************
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

#define SCALER_MODULE_NAME "vin_scaler"

static LIST_HEAD(scaler_drv_list);

#define MIN_IN_WIDTH			32
#define MIN_IN_HEIGHT			32
#define MAX_IN_WIDTH			4095
#define MAX_IN_HEIGHT			4095

#define MIN_OUT_WIDTH			16
#define MIN_OUT_HEIGHT			2
#define MAX_OUT_WIDTH			4095
#define MAX_OUT_HEIGHT			4095

static struct v4l2_mbus_framefmt *__scaler_get_format(struct scaler_dev *scaler,
				struct v4l2_subdev_fh *fh,
				unsigned int pad,
				enum v4l2_subdev_format_whence which)
{
	if (which == V4L2_SUBDEV_FORMAT_TRY)
		return fh ? v4l2_subdev_get_try_format(fh, pad) : NULL;
	else
		return &scaler->formats[pad];
}

static struct v4l2_rect *__scaler_get_crop(struct scaler_dev *scaler,
					   struct v4l2_subdev_fh *fh,
					   enum v4l2_subdev_format_whence which)
{
	if (which == V4L2_SUBDEV_FORMAT_TRY)
		return fh ? v4l2_subdev_get_try_crop(fh,\
						     SCALER_PAD_SINK) : NULL;
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
	crop->width = \
	    clamp_t(u32, crop->width, MIN_IN_WIDTH, sink->width - crop->left);
	crop->top = clamp_t(u32, crop->top, 0, sink->height - MIN_IN_HEIGHT);
	crop->height = \
	    clamp_t(u32, crop->height, MIN_IN_HEIGHT, sink->height - crop->top);
}

static void __scaler_calc_ratios(struct scaler_dev *scaler,
			       struct v4l2_rect *input,
			       struct v4l2_mbus_framefmt *output,
			       struct scaler_ratio *ratio)
{
	unsigned int width;
	unsigned int height;

	output->width = clamp_t(u32, output->width, MIN_IN_WIDTH, input->width);
	output->height = \
	    clamp_t(u32, output->height, MIN_IN_HEIGHT, input->height);

	ratio->horz = 256 * input->width / output->width;
	ratio->vert = 256 * input->height / output->height;
	ratio->horz = clamp_t(u32, ratio->horz, 256, 2048);
	ratio->vert = clamp_t(u32, ratio->vert, 256, 2048);

	width = output->width * ratio->horz / 256;
	height = output->height * ratio->vert / 256;

	vin_log(VIN_LOG_SCALER, "ratio: horz = %d, vert = %d\n",
		  ratio->horz, ratio->vert);

	/* Center the new crop rectangle. */
	input->left += (input->width - width) / 2;
	input->top += (input->height - height) / 2;
	input->width = width;
	input->height = height;
	vin_log(VIN_LOG_SCALER, "crop: left = %d, top = %d, w = %d, h = %d\n",
		input->left, input->top, input->width, input->height);
}

/* scaler pixel formats */
static const unsigned int sunxi_scaler_formats[] = {
	V4L2_MBUS_FMT_UYVY8_2X8,
	V4L2_MBUS_FMT_YUYV8_2X8,
	V4L2_MBUS_FMT_UYVY8_1X16,
	V4L2_MBUS_FMT_YUYV8_1X16,
};

static void __scaler_try_format(struct scaler_dev *scaler,
			      struct v4l2_subdev_fh *fh, unsigned int pad,
			      struct v4l2_mbus_framefmt *fmt,
			      enum v4l2_subdev_format_whence which)
{
	struct v4l2_mbus_framefmt *format;
	struct scaler_ratio ratio;
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
		    __scaler_get_format(scaler, fh, SCALER_PAD_SINK, which);
		fmt->code = format->code;

		crop = *__scaler_get_crop(scaler, fh, which);
		__scaler_calc_ratios(scaler, &crop, fmt, &ratio);
		break;
	}

	fmt->field = V4L2_FIELD_NONE;
}

static int sunxi_scaler_enum_mbus_code(struct v4l2_subdev *sd,
				       struct v4l2_subdev_fh *fh,
				       struct v4l2_subdev_mbus_code_enum *code)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format;

	if (code->pad == SCALER_PAD_SINK) {
		if (code->index >= ARRAY_SIZE(sunxi_scaler_formats))
			return -EINVAL;

		code->code = sunxi_scaler_formats[code->index];
	} else {
		if (code->index != 0)
			return -EINVAL;

		format = __scaler_get_format(scaler, fh, SCALER_PAD_SINK,
					V4L2_SUBDEV_FORMAT_ACTIVE);
		code->code = format->code;
	}
	return 0;
}

static int sunxi_scaler_enum_frame_size(struct v4l2_subdev *sd,
					struct v4l2_subdev_fh *fh,
					struct v4l2_subdev_frame_size_enum *fse)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt format;

	if (fse->code != V4L2_MBUS_FMT_YUYV8_1X16
	    && fse->code != V4L2_MBUS_FMT_UYVY8_1X16)
		return -EINVAL;

	if (fse->index != 0)
		return -EINVAL;

	format.code = fse->code;
	format.width = 1;
	format.height = 1;
	__scaler_try_format(scaler, fh, fse->pad, &format,
			V4L2_SUBDEV_FORMAT_ACTIVE);
	fse->min_width = format.width;
	fse->min_height = format.height;

	if (format.code != fse->code)
		return -EINVAL;

	format.code = fse->code;
	format.width = -1;
	format.height = -1;
	__scaler_try_format(scaler, fh, fse->pad, &format,
			V4L2_SUBDEV_FORMAT_ACTIVE);
	fse->max_width = format.width;
	fse->max_height = format.height;

	return 0;
}

static int sunxi_scaler_subdev_get_fmt(struct v4l2_subdev *sd,
				       struct v4l2_subdev_fh *fh,
				       struct v4l2_subdev_format *fmt)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format;

	format = __scaler_get_format(scaler, fh, fmt->pad, fmt->which);
	if (format == NULL)
		return -EINVAL;

	fmt->format = *format;
	return 0;
}

static int sunxi_scaler_subdev_set_fmt(struct v4l2_subdev *sd,
				       struct v4l2_subdev_fh *fh,
				       struct v4l2_subdev_format *fmt)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format;
	struct v4l2_rect *crop;

	format = __scaler_get_format(scaler, fh, fmt->pad, fmt->which);
	if (format == NULL)
		return -EINVAL;
	vin_log(VIN_LOG_FMT, "%s %d*%d %x %d\n", __func__, fmt->format.width,
		  fmt->format.height, fmt->format.code, fmt->format.field);
	__scaler_try_format(scaler, fh, fmt->pad, &fmt->format, fmt->which);
	*format = fmt->format;

	if (fmt->pad == SCALER_PAD_SINK) {
		/* reset crop rectangle */
		crop = __scaler_get_crop(scaler, fh, fmt->which);
		crop->left = 0;
		crop->top = 0;
		crop->width = fmt->format.width;
		crop->height = fmt->format.height;

		/* Propagate the format from sink to source */
		format = __scaler_get_format(scaler, fh, SCALER_PAD_SOURCE,\
						fmt->which);
		*format = fmt->format;
		__scaler_try_format(scaler, fh, SCALER_PAD_SOURCE, format,\
				  fmt->which);
	}

	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE) {
		scaler->crop.active = scaler->crop.request;
		__scaler_calc_ratios(scaler, &scaler->crop.active, format,\
				   &scaler->ratio);
	}

	return 0;

}

static int sunxi_scaler_subdev_get_selection(struct v4l2_subdev *sd,
					     struct v4l2_subdev_fh *fh,
					     struct v4l2_subdev_selection *sel)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format_source;
	struct v4l2_mbus_framefmt *format_sink;
	struct scaler_ratio ratio;

	if (sel->pad != SCALER_PAD_SINK)
		return -EINVAL;

	format_sink = \
	    __scaler_get_format(scaler, fh, SCALER_PAD_SINK, sel->which);
	format_source = \
	    __scaler_get_format(scaler, fh, SCALER_PAD_SOURCE, sel->which);

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP_BOUNDS:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = INT_MAX;
		sel->r.height = INT_MAX;

		__scaler_try_crop(format_sink, format_source, &sel->r);
		__scaler_calc_ratios(scaler, &sel->r, format_source, &ratio);
		break;

	case V4L2_SEL_TGT_CROP:
		sel->r = *__scaler_get_crop(scaler, fh, sel->which);
		__scaler_calc_ratios(scaler, &sel->r, format_source, &ratio);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_scaler_subdev_set_selection(struct v4l2_subdev *sd,
					     struct v4l2_subdev_fh *fh,
					     struct v4l2_subdev_selection *sel)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format_sink, *format_source;
	struct scaler_ratio ratio;

	if (sel->target != V4L2_SEL_TGT_CROP || sel->pad != SCALER_PAD_SINK)
		return -EINVAL;

	format_sink = \
	    __scaler_get_format(scaler, fh, SCALER_PAD_SINK, sel->which);
	format_source = \
	    __scaler_get_format(scaler, fh, SCALER_PAD_SOURCE, sel->which);

	vin_print("%s: L = %d, T = %d, W = %d, H = %d\n", __func__,
		  sel->r.left, sel->r.top, sel->r.width, sel->r.height);

	vin_print("%s: input = %dx%d, output = %dx%d\n", __func__,
		  format_sink->width, format_sink->height,
		  format_source->width, format_source->height);

	__scaler_try_crop(format_sink, format_source, &sel->r);
	*__scaler_get_crop(scaler, fh, sel->which) = sel->r;
	__scaler_calc_ratios(scaler, &sel->r, format_source, &ratio);

	if (sel->which == V4L2_SUBDEV_FORMAT_TRY)
		return 0;

	scaler->ratio = ratio;
	/*crop is after scale*/
	scaler->crop.active.left = sel->r.left * 256 / ratio.horz;
	scaler->crop.active.top = sel->r.top * 256 / ratio.vert;
	scaler->crop.active.width = format_source->width;
	scaler->crop.active.height = format_source->height;
	vin_log(VIN_LOG_SCALER, "active crop: l = %d, t = %d, w = %d, h = %d\n",
		scaler->crop.active.left, scaler->crop.active.top,
		scaler->crop.active.width, scaler->crop.active.height);
	return 0;
}

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
	.pad = &sunxi_scaler_subdev_pad_ops,
};

int sunxi_scaler_init_subdev(struct scaler_dev *scaler)
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
	sd->entity.type = MEDIA_ENT_T_V4L2_SUBDEV;

	ret = media_entity_init(&sd->entity, SCALER_PAD_NUM,
			scaler->scaler_pads, 0);
	if (ret < 0)
		return ret;
	return 0;
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
	pdev->id = of_alias_get_id(np, "scaler");
	if (pdev->id < 0) {
		vin_err("Scaler failed to get alias id\n");
		ret = -EINVAL;
		goto freedev;
	}

	scaler->id = pdev->id;
	scaler->pdev = pdev;
	list_add_tail(&scaler->scaler_list, &scaler_drv_list);
	sunxi_scaler_init_subdev(scaler);

	spin_lock_init(&scaler->slock);
	init_waitqueue_head(&scaler->wait);
	platform_set_drvdata(pdev, scaler);

	vin_print("scaler%d probe end\n", scaler->id);
	return 0;
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
	platform_set_drvdata(pdev, NULL);
	v4l2_device_unregister_subdev(sd);
	v4l2_set_subdevdata(sd, NULL);
	list_del(&scaler->scaler_list);
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
      }
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

int sunxi_scaler_platform_register(void)
{
	return platform_driver_register(&scaler_platform_driver);
}

void sunxi_scaler_platform_unregister(void)
{
	platform_driver_unregister(&scaler_platform_driver);
	vin_print("scaler_exit end\n");
}
