// SPDX-License-Identifier: GPL-2.0
/*
 * imx327 driver
 *
 * Copyright (C) 2024 Sophon Co., Ltd.
 *
 */
#include <linux/clk.h>
#include <linux/acpi.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/pm_runtime.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fwnode.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/gpio/consumer.h>

#include <linux/comm_cif.h>
#include <linux/sns_v4l2_uapi.h>

#include "imx327.h"

/* I2C per write of bits */
#define REG_VALUE_08BIT		1
#define REG_VALUE_16BIT		2
#define REG_VALUE_24BIT		3

/* Chip ID */
#define IMX327_CHIP_ID_ADDR		0x31DC
#define IMX327_CHIP_ID			0X6
#define IMX327_CHIP_ID_MASK		0x6

/*Sensor type for isp middleware*/
#define IMX327_SNS_TYPE_SDR V4L2_SONY_IMX327_2L_MIPI_2M_30FPS_12BIT
#define IMX327_SNS_TYPE_WDR V4L2_SONY_IMX327_2L_MIPI_2M_30FPS_12BIT_WDR2TO1

static const enum mipi_wdr_mode_e imx327_wdr_mode = MIPI_WDR_MODE_DOL;

static int imx327_count;
static int force_bus[MAX_SENSOR_DEVICE] = {[0 ... (MAX_SENSOR_DEVICE - 1)] = -1};
module_param_array(force_bus, int, &imx327_count, 0644);

static int imx327_probe_index;
static const unsigned short imx327_i2c_list[] = {0x1a};
static const int imx327_bus_map[MAX_SENSOR_DEVICE] = {1, -1, -1, -1, -1, -1};

struct imx327_reg_list {
	u32 num_of_regs;
	const struct imx327_reg *regs;
};

/* Mode : resolution and related config&values */
struct imx327_mode {
	u32 max_width;
	u32 max_height;
	u32 width;
	u32 height;
	u32 hts_def;
	u32 vts_def;
	u32 exp_def;
	u32 mipi_wdr_mode;
	struct v4l2_fract max_fps;
	struct v4l2_fract wdr_max_fps;
	sns_sync_info_t imx327_sync_info;
	struct imx327_reg_list reg_list;
	struct imx327_reg_list wdr_reg_list;
};

/* Mode configs */
static struct imx327_mode supported_modes[] = {
	{
		.max_width = 1948,
		.max_height = 1097,
		.width = 1920,
		.height = 1080,
		.exp_def = 0x2000,
		.hts_def = 0x1130,
		.vts_def = 1125,
		.mipi_wdr_mode = MIPI_WDR_MODE_NONE,
		.max_fps = {
			.numerator = 10000,
			.denominator = 300000,
		},
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(mode_1920x1080_2l_regs),
			.regs = mode_1920x1080_2l_regs,
		},
		.wdr_reg_list = {
			.num_of_regs = ARRAY_SIZE(mode_1920x1080_2l_wdr_regs),
			.regs = mode_1920x1080_2l_wdr_regs,
		},
	},
};

struct imx327 {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct i2c_client *client;
	struct v4l2_ctrl_handler ctrl_handler;
	/* V4L2 Controls */
	struct v4l2_ctrl *vblank;
	struct v4l2_ctrl *hblank;
	struct v4l2_ctrl *exposure;
	/* Current mode */
	struct imx327_mode *cur_mode;
	/* Mutex for serialized access */
	struct mutex mutex;
	/* Streaming on/off */
	bool streaming;
	/*dtsi config*/
	struct clk       *xvclk;
	struct gpio_desc *power_gpio;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *pwdn_gpio;
	struct pinctrl   *pinctrl;

	unsigned int lane_num;
	unsigned int module_index;
};

#define to_imx327(_sd)	container_of(_sd, struct imx327, sd)

/* Read registers up to 4 at a time */
static int imx327_read_reg(struct imx327 *imx327, u16 reg, u32 len,
			    u32 *val)
{
	struct i2c_client *client = v4l2_get_subdevdata(&imx327->sd);
	struct i2c_msg msgs[2];
	u8 *data_be_p;
	int ret;
	__be32 data_be = 0;
	__be16 reg_addr_be = cpu_to_be16(reg);

	if (len > 4)
		return -EINVAL;

	data_be_p = (u8 *)&data_be;
	/* Write register address */
	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 2;
	msgs[0].buf = (u8 *)&reg_addr_be;

	/* Read data from register */
	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = &data_be_p[4 - len];

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret != ARRAY_SIZE(msgs))
		return -EIO;

	*val = be32_to_cpu(data_be);

	return 0;
}

/* Write registers up to 4 at a time */
static int imx327_write_reg(struct imx327 *imx327, u16 reg, u32 len,
			     u32 __val)
{
	struct i2c_client *client = v4l2_get_subdevdata(&imx327->sd);
	int buf_i, val_i;
	u8 buf[6], *val_p;
	__be32 val;

	if (len > 4)
		return -EINVAL;

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	val = cpu_to_be32(__val);
	val_p = (u8 *)&val;
	buf_i = 2;
	val_i = 4 - len;

	while (val_i < 4)
		buf[buf_i++] = val_p[val_i++];

	if (i2c_master_send(client, buf, len + 2) != len + 2)
		return -EIO;

	return 0;
}

/* Write a list of registers */
static int imx327_write_regs(struct imx327 *imx327,
			      const struct imx327_reg *regs, u32 len)
{
	struct i2c_client *client = v4l2_get_subdevdata(&imx327->sd);
	int ret;
	u32 i;

	for (i = 0; i < len; i++) {
		ret = imx327_write_reg(imx327, regs[i].address, 1,
					regs[i].val);
		if (ret) {
			dev_err_ratelimited(&client->dev, "Failed to write reg 0x%4.4x. error=%d\n",
					    regs[i].address, ret);

			return ret;
		}
	}

	return 0;
}

/* Open sub-device */
static int imx327_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct imx327 *imx327 = to_imx327(sd);
	struct v4l2_mbus_framefmt *try_fmt =
			v4l2_subdev_get_try_format(sd, fh->pad, 0);

	mutex_lock(&imx327->mutex);

	/* Initialize try_fmt */
	try_fmt->width = imx327->cur_mode->width;
	try_fmt->height = imx327->cur_mode->height;
	try_fmt->code = MEDIA_BUS_FMT_SGRBG10_1X10;
	try_fmt->field = V4L2_FIELD_NONE;

	/* No crop or compose */
	mutex_unlock(&imx327->mutex);

	return 0;
}

static int imx327_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct imx327 *imx327 = container_of(ctrl->handler,
					       struct imx327, ctrl_handler);
	struct i2c_client *client = v4l2_get_subdevdata(&imx327->sd);

	pm_runtime_put(&client->dev);

	return 0;
}

static const struct v4l2_ctrl_ops imx327_ctrl_ops = {
	.s_ctrl = imx327_set_ctrl,
};

static int g_mbus_config(struct v4l2_subdev *sd, unsigned int pad_id,
			 struct v4l2_mbus_config *config)
{
	config->type = V4L2_MBUS_CSI2_DPHY;
	return 0;
}

static int enum_mbus_code(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_mbus_code_enum *code)
{
	/* Only one bayer order(GRBG) is supported */
	if (code->index > 0)
		return -EINVAL;

	code->code = MEDIA_BUS_FMT_SGRBG10_1X10;

	return 0;
}

static int enum_frame_interval(struct v4l2_subdev *sd,
			      struct v4l2_subdev_pad_config *cfg,
			      struct v4l2_subdev_frame_interval_enum *fie)
{
	struct imx327 *imx327 = to_imx327(sd);

	fie->width  = imx327->cur_mode->width;
	fie->height = imx327->cur_mode->height;

	if (imx327->cur_mode->mipi_wdr_mode == MIPI_WDR_MODE_NONE) {
		fie->interval.numerator   = imx327->cur_mode->max_fps.numerator;
		fie->interval.denominator = imx327->cur_mode->max_fps.denominator;
	} else {
		fie->interval.numerator   = imx327->cur_mode->wdr_max_fps.numerator;
		fie->interval.denominator = imx327->cur_mode->wdr_max_fps.denominator;
	}
	return 0;
}

static int enum_frame_size(struct v4l2_subdev *sd,
			   struct v4l2_subdev_pad_config *cfg,
			   struct v4l2_subdev_frame_size_enum *fse)
{
	struct imx327 *imx327 = to_imx327(sd);

	fse->min_width = imx327->cur_mode->width;
	fse->max_width = imx327->cur_mode->max_width;
	fse->min_height = imx327->cur_mode->height;
	fse->max_height = imx327->cur_mode->max_height;

	return 0;
}

static void update_pad_format(const struct imx327_mode *mode, struct v4l2_subdev_format *fmt)
{
	fmt->format.width = mode->width;
	fmt->format.height = mode->height;
	fmt->format.code = MEDIA_BUS_FMT_SGRBG10_1X10;
	fmt->format.field = V4L2_FIELD_NONE;
}

static int get_pad_format(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct imx327 *imx327 = to_imx327(sd);
	struct v4l2_mbus_framefmt *framefmt;
	int ret = 0;

	mutex_lock(&imx327->mutex);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		framefmt = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
		fmt->format = *framefmt;
		ret = -ENOTTY;
	} else {
		update_pad_format(imx327->cur_mode, fmt);
	}
	mutex_unlock(&imx327->mutex);

	return ret;
}

static int set_pad_format(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct imx327 *imx327 = to_imx327(sd);
	struct imx327_mode *mode;
	struct v4l2_mbus_framefmt *framefmt;

	mutex_lock(&imx327->mutex);

	/* Only one raw bayer(GRBG) order is supported */
	if (fmt->format.code != MEDIA_BUS_FMT_SGRBG10_1X10)
		fmt->format.code = MEDIA_BUS_FMT_SGRBG10_1X10;

	mode = v4l2_find_nearest_size(supported_modes,
				      ARRAY_SIZE(supported_modes),
				      width, height,
				      fmt->format.width, fmt->format.height);
	update_pad_format(mode, fmt);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		framefmt = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
		*framefmt = fmt->format;
	} else {
		imx327->cur_mode = mode;
	}

	mutex_unlock(&imx327->mutex);

	return 0;
}

static void imx327_standby(struct imx327 *imx327)
{
	imx327_write_reg(imx327, 0x3000, REG_VALUE_08BIT, 0x01);
	imx327_write_reg(imx327, 0x3002, REG_VALUE_08BIT, 0x01);
}

static void imx327_restart(struct imx327 *imx327)
{
	imx327_write_reg(imx327, 0x3000, REG_VALUE_08BIT, 0x00);
	imx327_write_reg(imx327, 0x3002, REG_VALUE_08BIT, 0x00);
	imx327_write_reg(imx327, 0x304b, REG_VALUE_08BIT, 0x0a);
}

/* Start streaming */
static int start_streaming(struct imx327 *imx327)
{
	struct i2c_client *client = v4l2_get_subdevdata(&imx327->sd);
	const struct imx327_reg_list *reg_list;
	const sns_sync_info_t *sync_info;
	int ret;

	if (imx327->cur_mode->mipi_wdr_mode == MIPI_WDR_MODE_NONE) {//linear
		reg_list = &imx327->cur_mode->reg_list;
	} else {//wdr
		reg_list = &imx327->cur_mode->wdr_reg_list;
	}

	ret = imx327_write_regs(imx327, reg_list->regs, reg_list->num_of_regs);
	if (ret) {
		dev_err(&client->dev, "%s failed to set mode\n", __func__);
		return ret;
	}

	sync_info = &imx327->cur_mode->imx327_sync_info;

	if (sync_info->num_of_regs > 0) {
		ret = imx327_write_regs(imx327, (struct imx327_reg *)sync_info->regs,
					sync_info->num_of_regs);
		if (ret) {
			dev_err(&client->dev, "%s failed to set default\n", __func__);
			return ret;
		}
	}

	usleep_range(100 * 1000, 200 * 1000);
	/* Apply customized values from user */
	ret =  __v4l2_ctrl_handler_setup(imx327->sd.ctrl_handler);
	if (ret)
		return ret;

	dev_info(&client->dev, "wdr_mode(%d) reg setting done\n", imx327->cur_mode->mipi_wdr_mode);

	return ret;
}

/* Stop streaming */
static int stop_streaming(struct imx327 *imx327)
{
	imx327_standby(imx327);
	return 0;
}

static int set_stream(struct v4l2_subdev *sd, int enable)
{
	struct imx327 *imx327 = to_imx327(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	mutex_lock(&imx327->mutex);
	if (imx327->streaming == enable) {
		mutex_unlock(&imx327->mutex);
		return 0;
	}

	if (enable) {
		ret = pm_runtime_get_sync(&client->dev);
		if (ret < 0) {
			pm_runtime_put_noidle(&client->dev);
			goto err_unlock;
		}

		/*
		 * Apply default & customized values
		 * and then start streaming.
		 */
		ret = start_streaming(imx327);
		if (ret)
			goto err_rpm_put;
	} else {
		stop_streaming(imx327);
		pm_runtime_put(&client->dev);
	}

	imx327->streaming = enable;
	mutex_unlock(&imx327->mutex);

	dev_info(&client->dev, "set stream(%d) success\n", enable);

	return ret;

err_rpm_put:
	pm_runtime_put(&client->dev);
err_unlock:
	mutex_unlock(&imx327->mutex);

	return ret;
}

static int __maybe_unused suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx327 *imx327 = to_imx327(sd);

	if (imx327->streaming)
		stop_streaming(imx327);

	return 0;
}

static int __maybe_unused resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx327 *imx327 = to_imx327(sd);
	int ret;

	if (imx327->streaming) {
		ret = start_streaming(imx327);
		if (ret)
			goto error;
	}

	return 0;

error:
	stop_streaming(imx327);
	imx327->streaming = false;
	return ret;
}

/* Verify chip ID */
static int imx327_identify_module(struct imx327 *imx327)
{
	struct i2c_client *client = v4l2_get_subdevdata(&imx327->sd);
	int ret;
	u32 val = 0;

	ret = imx327_read_reg(imx327, IMX327_CHIP_ID_ADDR,
			       REG_VALUE_08BIT, &val);
	dev_info(&client->dev, "read id:0x%x, ret:%d", val, ret);
	if (ret)
		return ret;

	if ((val & IMX327_CHIP_ID_MASK) != IMX327_CHIP_ID) {
		dev_err(&client->dev, "chip id(%x) mismatch, read(%x)\n",
			IMX327_CHIP_ID, val);
		return -EIO;
	}

	return 0;
}

static void imx327_mirror_flip(struct imx327 *imx327, int orient)
{
	int val = 0;
	int ori_addr = 0x3007;

	pr_info("set mirror_flip:%d", orient);

	imx327_read_reg(imx327, ori_addr, REG_VALUE_08BIT, &val);
	val = val & ~0x3;

	switch (orient) {
	case 0:
		break;
	case 1:
		val |= 0x2;
		break;
	case 2:
		val |= 0x1;
		break;
	case 3:
		val |= 0x3;
		break;
	default:
		return;
	}

	imx327_standby(imx327);
	imx327_write_reg(imx327, ori_addr, REG_VALUE_08BIT, val);
	imx327_restart(imx327);
}

static int imx327_update_link_menu(struct imx327 *imx327)
{
	struct i2c_client *client = v4l2_get_subdevdata(&imx327->sd);
	struct v4l2_ctrl_handler *ctrl_hdlr;
	struct v4l2_ctrl *ctrl;
	int wdr_index = SNS_CFG_TYPE_WDR_MODE;
	int id = imx327->module_index;
	int ret;
	int i;

	imx327_link_cif_menu[id][wdr_index] = imx327->cur_mode->mipi_wdr_mode;

	dev_info(&client->dev, "update mipi_mode:%lld", imx327_link_cif_menu[id][wdr_index]);

	ctrl_hdlr = imx327->sd.ctrl_handler;
	v4l2_ctrl_handler_free(ctrl_hdlr);

	ret = v4l2_ctrl_handler_init(ctrl_hdlr, 10);
	if (ret) {
		dev_err(&client->dev, "%s ctrl handler init failed (%d)\n",
			__func__, ret);
		return ret;
	}

	for (i = 0; i < SNS_CFG_TYPE_MAX; i++) {
		ctrl = v4l2_ctrl_new_int_menu(ctrl_hdlr, &imx327_ctrl_ops, V4L2_CID_LINK_FREQ,
					      imx327_link_cif_menu[id][i], 0,
					       (const s64 *)imx327_link_cif_menu[id]);

		if (ctrl)
			ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;
	}

	if (ctrl_hdlr->error) {
		ret = ctrl_hdlr->error;
		dev_err(&client->dev, "%s new int menu failed (%d)\n",
			__func__, ret);
		goto error;
	}

	return 0;

error:
	v4l2_ctrl_handler_free(ctrl_hdlr);

	return ret;
}

static long imx327_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct imx327 *imx327 = to_imx327(sd);
	long ret = 0;

	switch (cmd) {
	case SNS_V4L2_GET_TYPE:
	{
		int type = 0;

		if (imx327->cur_mode->mipi_wdr_mode == MIPI_WDR_MODE_NONE) {//linear
			type = IMX327_SNS_TYPE_SDR;
		} else {//wdr
			type = IMX327_SNS_TYPE_WDR;
		}
		memcpy(arg, &type, sizeof(int));
		break;
	}

	case SNS_V4L2_SET_MIRROR_FLIP:
	{
		int orient = 0;

		memcpy(&orient, arg, sizeof(int));
		imx327_mirror_flip(imx327, orient);
		break;
	}

	case SNS_V4L2_GET_I2C_INFO:
	{
		struct i2c_client *client = v4l2_get_subdevdata(&imx327->sd);
		sns_i2c_info_t i2c_info;

		i2c_info.i2c_addr =  client->addr;
		i2c_info.i2c_idx  =  client->adapter->i2c_idx;
		memcpy(arg, &i2c_info, sizeof(sns_i2c_info_t));
		break;
	}

	case SNS_V4L2_SET_HDR_ON:
	{
		int hdr_on = 0;

		memcpy(&hdr_on, arg, sizeof(int));

		if (hdr_on)
			imx327->cur_mode->mipi_wdr_mode = imx327_wdr_mode;
		else
			imx327->cur_mode->mipi_wdr_mode = MIPI_WDR_MODE_NONE;

		imx327_update_link_menu(imx327);
		break;
	}

	case SNS_V4L2_SET_SNS_SYNC_INFO:
	{
		memcpy(&imx327->cur_mode->imx327_sync_info, arg, sizeof(sns_sync_info_t));
		break;
	}

	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long imx327_compat_ioctl32(struct v4l2_subdev *sd,
				   unsigned int cmd, unsigned long arg)
{
	long ret;

	switch (cmd) {
	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}
#endif

static const struct v4l2_subdev_core_ops imx327_core_ops = {
	.ioctl = imx327_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = imx327_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops imx327_video_ops = {
	.s_stream = set_stream,
};

static const struct v4l2_subdev_pad_ops imx327_pad_ops = {
	.enum_mbus_code = enum_mbus_code,
	.get_fmt = get_pad_format,
	.set_fmt = set_pad_format,
	.enum_frame_size = enum_frame_size,
	.enum_frame_interval = enum_frame_interval,
	.get_mbus_config = g_mbus_config,
};

static const struct v4l2_subdev_ops imx327_subdev_ops = {
	.core	= &imx327_core_ops,
	.video  = &imx327_video_ops,
	.pad    = &imx327_pad_ops,
};

static const struct media_entity_operations imx327_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static const struct v4l2_subdev_internal_ops imx327_internal_ops = {
	.open = imx327_open,
};

/* Initialize control handlers */
static int imx327_init_controls(struct imx327 *imx327, int index_id)
{
	struct i2c_client *client = v4l2_get_subdevdata(&imx327->sd);
	struct v4l2_fwnode_device_properties props;
	struct v4l2_ctrl_handler *ctrl_hdlr;
	struct v4l2_ctrl *ctrl;
	int ret;
	int i;

	ctrl_hdlr = &imx327->ctrl_handler;
	ret = v4l2_ctrl_handler_init(ctrl_hdlr, 10);
	if (ret) {
		dev_err(&client->dev, "%s ctrl handler init failed (%d)\n",
			__func__, ret);
		return ret;
	}

	mutex_init(&imx327->mutex);
	ctrl_hdlr->lock = &imx327->mutex;
	for (i = 0; i < SNS_CFG_TYPE_MAX; i++) {
		if (i == SNS_CFG_TYPE_WDR_MODE)
			imx327->cur_mode->mipi_wdr_mode = imx327_link_cif_menu[index_id][i];

		ctrl = v4l2_ctrl_new_int_menu(ctrl_hdlr, &imx327_ctrl_ops, V4L2_CID_LINK_FREQ,
					      imx327_link_cif_menu[index_id][i], 0,
					       (const s64 *)imx327_link_cif_menu[index_id]);

		if (ctrl)
			ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;
	}

	if (ctrl_hdlr->error) {
		ret = ctrl_hdlr->error;
		dev_err(&client->dev, "%s new std menu failed (%d)\n",
			__func__, ret);
		goto error;
	}

	ret = v4l2_fwnode_device_parse(&client->dev, &props);
	if (ret)
		goto error;

	ret = v4l2_ctrl_new_fwnode_properties(ctrl_hdlr, &imx327_ctrl_ops,
					      &props);
	if (ret)
		goto error;

	imx327->sd.ctrl_handler = ctrl_hdlr;

	return 0;

error:
	v4l2_ctrl_handler_free(ctrl_hdlr);
	mutex_destroy(&imx327->mutex);

	return ret;
}

static void imx327_free_controls(struct imx327 *imx327)
{
	v4l2_ctrl_handler_free(imx327->sd.ctrl_handler);
	mutex_destroy(&imx327->mutex);
}

static int imx327_probe(struct i2c_client *client,
			 const struct i2c_device_id *devid)
{
	struct imx327 *imx327;
	struct v4l2_subdev *sd;
	struct device *dev = &client->dev;
	int index_id = imx327_probe_index;
	int addr_num = sizeof(imx327_i2c_list) / sizeof(unsigned short);
	int bus_id;
	int ret = -1;
	int i;

	dev_info(dev, "probe id[%d] start\n", imx327_probe_index);

	imx327_probe_index++;

	if (index_id >= MAX_SENSOR_DEVICE || index_id < 0) {
		dev_info(dev, "invalid devid(%d)\n", index_id);
		return ret;
	}

	imx327 = devm_kzalloc(&client->dev, sizeof(*imx327), GFP_KERNEL);
	if (!imx327)
		return -ENOMEM;

	sd = &imx327->sd;

	for (i = 0; i < addr_num; i++) {
		if (force_bus[index_id] < 0)
			bus_id = imx327_bus_map[index_id];
		else
			bus_id = force_bus[index_id];

		if (bus_id < 0 || bus_id > MAX_I2C_BUS_NUM)
			return ret;

		client->addr = imx327_i2c_list[i];
		client->adapter = i2c_get_adapter(bus_id);
		imx327->client = client;
		v4l2_i2c_subdev_init(sd, client, &imx327_subdev_ops);

		/* Check module identity */
		ret = imx327_identify_module(imx327);
		if (ret) {
			dev_info(dev, "id[%d] bus[%d] i2c_addr[%d][0x%x] no sensor found\n",
				 index_id, bus_id, i, client->addr);

			if (i == addr_num - 1)
				return ret;

			continue;
		} else {
			dev_info(dev, "id[%d] bus[%d] i2c_addr[0x%x] sensor found\n",
				 index_id, bus_id, client->addr);
			break;
		}
	}

	imx327->module_index = index_id;

	if (index_id >= ARRAY_SIZE(supported_modes)) {
		imx327->cur_mode = devm_kzalloc(&client->dev,
						sizeof(struct imx327_mode), GFP_KERNEL);
		memcpy(imx327->cur_mode, &supported_modes[0], sizeof(struct imx327_mode));
	} else {
		imx327->cur_mode = devm_kzalloc(&client->dev,
						sizeof(struct imx327_mode), GFP_KERNEL);
		memcpy(imx327->cur_mode, &supported_modes[index_id], sizeof(struct imx327_mode));
	}

	memset(&imx327->cur_mode->imx327_sync_info, 0, sizeof(sns_sync_info_t));

	mutex_init(&imx327->mutex);

	ret = imx327_init_controls(imx327, index_id);
	if (ret)
		return ret;

	/* Initialize subdev */
	sd->internal_ops = &imx327_internal_ops;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	sd->entity.ops = &imx327_subdev_entity_ops;
	sd->entity.function = MEDIA_ENT_F_CAM_SENSOR;

	/* Initialize source pad */
	imx327->pad.flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&sd->entity, 1, &imx327->pad);
	if (ret) {
		dev_err(&client->dev, "failed to init pads:%d\n", ret);
		goto error_handler_free;
	}

	snprintf(sd->name, sizeof(sd->name), "cam%d_%s %s",
		 imx327->module_index, "imx327", dev_name(sd->dev));

	ret = v4l2_async_register_subdev_sensor_common(sd);
	if (ret < 0) {
		dev_err(&client->dev, "failed to async subdev:%d\n", ret);
		goto error_media_entity;
	}

	/*
	 * Device is already turned on by i2c-core with ACPI domain PM.
	 * Enable runtime PM and turn off the device.
	 */
	pm_runtime_set_active(&client->dev);
	pm_runtime_enable(&client->dev);
	pm_runtime_idle(&client->dev);

	dev_info(dev, "sensor_%d probe success\n", index_id);

	return 0;

error_media_entity:
	media_entity_cleanup(&imx327->sd.entity);

error_handler_free:
	imx327_free_controls(imx327);
	dev_err(&client->dev, "%s failed:%d\n", __func__, ret);

	return ret;
}

static int imx327_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx327 *imx327 = to_imx327(sd);

	imx327_probe_index = 0;

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	imx327_free_controls(imx327);

	pm_runtime_disable(&client->dev);

	return 0;
}

static const struct of_device_id imx327_of_match[] = {
	{ .compatible = "cvitek,sensor0" },
	{ .compatible = "cvitek,sensor1" },
	{ .compatible = "cvitek,sensor2" },
	{ .compatible = "cvitek,sensor3" },
	{ .compatible = "cvitek,sensor4" },
	{ .compatible = "cvitek,sensor5" },
	{},
};
MODULE_DEVICE_TABLE(of, imx327_of_match);

static const struct dev_pm_ops imx327_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(suspend, resume)
};

static struct i2c_driver imx327_i2c_driver = {
	.driver = {
		.name = "imx327",
		.pm = &imx327_pm_ops,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(imx327_of_match),
	},
	.probe    = imx327_probe,
	.remove   = imx327_remove,
};

static int __init sensor_mod_init(void)
{
	pr_info("== imx327 mod add ==\n");

	return i2c_add_driver(&imx327_i2c_driver);
}

static void __exit sensor_mod_exit(void)
{
	i2c_del_driver(&imx327_i2c_driver);
}

module_init(sensor_mod_init);
module_exit(sensor_mod_exit);

MODULE_DESCRIPTION("imx327 sensor driver");
MODULE_LICENSE("GPL v2");
