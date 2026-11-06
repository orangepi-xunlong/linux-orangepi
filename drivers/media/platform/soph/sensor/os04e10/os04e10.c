// SPDX-License-Identifier: GPL-2.0
/*
 * os04e10 driver
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

#include "os04e10.h"

/* I2C per write of bits */
#define REG_VALUE_08BIT		1
#define REG_VALUE_16BIT		2
#define REG_VALUE_24BIT		3

/* Chip ID */
#define OS04E10_CHIP_ID_ADDR_H	0x300A
#define OS04E10_CHIP_ID_ADDR_M	0x300B
#define OS04E10_CHIP_ID_ADDR_L	0x300C
#define OS04E10_CHIP_ID		0x530445

/*Sensor type for isp middleware*/
#define OS04E10_SNS_TYPE_SDR V4L2_OV_OS04E10_MIPI_4M_30FPS_2L_10BIT
#define OS04E10_SNS_TYPE_WDR V4L2_OV_OS04E10_MIPI_4M_30FPS_2L_10BIT_WDR2TO1

static const enum mipi_wdr_mode_e os04e10_wdr_mode = MIPI_WDR_MODE_VC;

static int os04e10_count;
static int force_bus[MAX_SENSOR_DEVICE] = {[0 ... (MAX_SENSOR_DEVICE - 1)] = -1};
module_param_array(force_bus, int, &os04e10_count, 0644);

static int os04e10_probe_index;
static const unsigned short os04e10_i2c_list[] = {0x36};
static int os04e10_bus_map[MAX_SENSOR_DEVICE] = {1, 0, -1, -1, -1, -1};

struct os04e10_reg_list {
	u32 num_of_regs;
	const struct os04e10_reg *regs;
};

/* Mode : resolution and related config&values */
struct os04e10_mode {
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
	sns_sync_info_t os04e10_sync_info;
	struct os04e10_reg_list reg_list;
	struct os04e10_reg_list wdr_reg_list;
};

/* Mode configs */
static struct os04e10_mode supported_modes[] = {
	{
		.max_width = 2048,
		.max_height = 2048,
		.width = 2048,
		.height = 2048,
		.exp_def = 0x2000,
		.hts_def = 0x67E,
		.vts_def = 0x89B,
		.max_fps = {
			.numerator = 10000,
			.denominator = 310000,
		},
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(mode_2048x2048_regs),
			.regs = mode_2048x2048_regs,
		},
	},
	{
		.max_width = 2048,
		.max_height = 2048,
		.width = 2048,
		.height = 2048,
		.exp_def = 0x2000,
		.hts_def = 0x67E,
		.vts_def = 0x89B,
		.max_fps = {
			.numerator = 10000,
			.denominator = 310000,
		},
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(mode_2048x2048_slave_regs),
			.regs = mode_2048x2048_slave_regs,
		},
	},
};

struct os04e10 {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct i2c_client *client;
	struct v4l2_ctrl_handler ctrl_handler;
	/* V4L2 Controls */
	struct v4l2_ctrl *vblank;
	struct v4l2_ctrl *hblank;
	struct v4l2_ctrl *exposure;
	/* Current mode */
	struct os04e10_mode *cur_mode;
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

#define to_os04e10(_sd)	container_of(_sd, struct os04e10, sd)

/* Read registers up to 4 at a time */
static int os04e10_read_reg(struct os04e10 *os04e10, u16 reg, u32 len,
			    u32 *val)
{
	struct i2c_client *client = v4l2_get_subdevdata(&os04e10->sd);
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
static int os04e10_write_reg(struct os04e10 *os04e10, u16 reg, u32 len,
			     u32 __val)
{
	struct i2c_client *client = v4l2_get_subdevdata(&os04e10->sd);
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
static int os04e10_write_regs(struct os04e10 *os04e10,
			      const struct os04e10_reg *regs, u32 len)
{
	struct i2c_client *client = v4l2_get_subdevdata(&os04e10->sd);
	int ret;
	u32 i;

	for (i = 0; i < len; i++) {
		ret = os04e10_write_reg(os04e10, regs[i].address, 1,
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
static int os04e10_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct os04e10 *os04e10 = to_os04e10(sd);
	struct v4l2_mbus_framefmt *try_fmt =
			v4l2_subdev_get_try_format(sd, fh->pad, 0);

	mutex_lock(&os04e10->mutex);

	/* Initialize try_fmt */
	try_fmt->width = os04e10->cur_mode->width;
	try_fmt->height = os04e10->cur_mode->height;
	try_fmt->code = MEDIA_BUS_FMT_SGRBG10_1X10;
	try_fmt->field = V4L2_FIELD_NONE;

	/* No crop or compose */
	mutex_unlock(&os04e10->mutex);

	return 0;
}

static int os04e10_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct os04e10 *os04e10 = container_of(ctrl->handler,
					       struct os04e10, ctrl_handler);
	struct i2c_client *client = v4l2_get_subdevdata(&os04e10->sd);

	pm_runtime_put(&client->dev);

	return 0;
}

static const struct v4l2_ctrl_ops os04e10_ctrl_ops = {
	.s_ctrl = os04e10_set_ctrl,
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
	struct os04e10 *os04e10 = to_os04e10(sd);

	fie->width  = os04e10->cur_mode->width;
	fie->height = os04e10->cur_mode->height;

	if (os04e10->cur_mode->mipi_wdr_mode == MIPI_WDR_MODE_NONE) {
		fie->interval.numerator   = os04e10->cur_mode->max_fps.numerator;
		fie->interval.denominator = os04e10->cur_mode->max_fps.denominator;
	} else {
		fie->interval.numerator   = os04e10->cur_mode->wdr_max_fps.numerator;
		fie->interval.denominator = os04e10->cur_mode->wdr_max_fps.denominator;
	}

	return 0;
}

static int enum_frame_size(struct v4l2_subdev *sd,
			   struct v4l2_subdev_pad_config *cfg,
			   struct v4l2_subdev_frame_size_enum *fse)
{
	struct os04e10 *os04e10 = to_os04e10(sd);

	fse->min_width = os04e10->cur_mode->width;
	fse->max_width = os04e10->cur_mode->max_width;
	fse->min_height = os04e10->cur_mode->height;
	fse->max_height = os04e10->cur_mode->max_height;

	return 0;
}

static void update_pad_format(const struct os04e10_mode *mode, struct v4l2_subdev_format *fmt)
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
	struct os04e10 *os04e10 = to_os04e10(sd);
	struct v4l2_mbus_framefmt *framefmt;
	int ret = 0;

	mutex_lock(&os04e10->mutex);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		framefmt = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
		fmt->format = *framefmt;
		ret = -ENOTTY;
	} else {
		update_pad_format(os04e10->cur_mode, fmt);
	}
	mutex_unlock(&os04e10->mutex);

	return ret;
}

static int set_pad_format(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct os04e10 *os04e10 = to_os04e10(sd);
	struct os04e10_mode *mode;
	struct v4l2_mbus_framefmt *framefmt;

	mutex_lock(&os04e10->mutex);

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
		os04e10->cur_mode = mode;
	}

	mutex_unlock(&os04e10->mutex);

	return 0;
}

static void os04e10_standby(struct os04e10 *os04e10)
{
	os04e10_write_reg(os04e10, 0x0100, REG_VALUE_08BIT, 0x00);
}

static void os04e10_restart(struct os04e10 *os04e10)
{
	os04e10_write_reg(os04e10, 0x0100, REG_VALUE_08BIT, 0x01);
}

/* Start streaming */
static int start_streaming(struct os04e10 *os04e10)
{
	struct i2c_client *client = v4l2_get_subdevdata(&os04e10->sd);
	const struct os04e10_reg_list *reg_list;
	const sns_sync_info_t *sync_info;
	int ret;

	if (os04e10->cur_mode->mipi_wdr_mode == MIPI_WDR_MODE_NONE) {//linear
		reg_list = &os04e10->cur_mode->reg_list;
	} else {//wdr
		reg_list = &os04e10->cur_mode->wdr_reg_list;
	}

	ret = os04e10_write_regs(os04e10, reg_list->regs, reg_list->num_of_regs);
	if (ret) {
		dev_err(&client->dev, "%s failed to set mode\n", __func__);
		return ret;
	}

	sync_info = &os04e10->cur_mode->os04e10_sync_info;

	if (sync_info->num_of_regs > 3) {
		ret = os04e10_write_regs(os04e10, (struct os04e10_reg *)sync_info->regs + 1,
					 sync_info->num_of_regs - 3);
		if (ret) {
			dev_err(&client->dev, "%s failed to set default\n", __func__);
			return ret;
		}
	}

	usleep_range(100 * 1000, 200 * 1000);
	/* Apply customized values from user */
	ret =  __v4l2_ctrl_handler_setup(os04e10->sd.ctrl_handler);
	if (ret)
		return ret;

	dev_info(&client->dev, "wdr_mode(%d) reg setting done\n", os04e10->cur_mode->mipi_wdr_mode);

	return ret;
}

/* Stop streaming */
static int stop_streaming(struct os04e10 *os04e10)
{
	os04e10_standby(os04e10);
	return 0;
}

static int set_stream(struct v4l2_subdev *sd, int enable)
{
	struct os04e10 *os04e10 = to_os04e10(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	mutex_lock(&os04e10->mutex);
	if (os04e10->streaming == enable) {
		mutex_unlock(&os04e10->mutex);
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
		ret = start_streaming(os04e10);
		if (ret)
			goto err_rpm_put;
	} else {
		stop_streaming(os04e10);
		pm_runtime_put(&client->dev);
	}

	os04e10->streaming = enable;
	mutex_unlock(&os04e10->mutex);

	dev_info(&client->dev, "set stream(%d) success\n", enable);

	return ret;

err_rpm_put:
	pm_runtime_put(&client->dev);
err_unlock:
	mutex_unlock(&os04e10->mutex);

	return ret;
}

static int __maybe_unused suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct os04e10 *os04e10 = to_os04e10(sd);

	if (os04e10->streaming)
		stop_streaming(os04e10);

	return 0;
}

static int __maybe_unused resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct os04e10 *os04e10 = to_os04e10(sd);
	int ret;

	if (os04e10->streaming) {
		ret = start_streaming(os04e10);
		if (ret)
			goto error;
	}

	return 0;

error:
	stop_streaming(os04e10);
	os04e10->streaming = false;
	return ret;
}

/* Verify chip ID */
static int os04e10_identify_module(struct os04e10 *os04e10)
{
	struct i2c_client *client = v4l2_get_subdevdata(&os04e10->sd);
	int ret;
	int val1, val2, val3;
	int read_data = 0;

	ret = os04e10_read_reg(os04e10, OS04E10_CHIP_ID_ADDR_H,
			       REG_VALUE_08BIT, &val1);
	dev_info(&client->dev, "read id:0x%x, ret:%d", val1, ret);
	if (ret)
		return ret;

	ret = os04e10_read_reg(os04e10, OS04E10_CHIP_ID_ADDR_M,
			       REG_VALUE_08BIT, &val2);
	ret = os04e10_read_reg(os04e10, OS04E10_CHIP_ID_ADDR_L,
			       REG_VALUE_08BIT, &val3);

	read_data = ((val1 & 0xFF) << 16) | ((val2 & 0xFF) << 8) | (val3 & 0xFF);

	if (read_data != OS04E10_CHIP_ID) {
		dev_err(&client->dev, "chip id(%x) mismatch, read(%x)\n",
			OS04E10_CHIP_ID, read_data);
		return -EIO;
	}

	return 0;
}

static void os04e10_mirror_flip(struct os04e10 *os04e10, int orient)
{
	int val = 0;
	int ori_addr = 0x3820;

	pr_info("set mirror_flip:%d", orient);

	os04e10_read_reg(os04e10, ori_addr, REG_VALUE_08BIT, &val);

	val &= ~(0x3 << 1);

	switch (orient) {
	case 0:
		break;
	case 1:
		val |= 0x1 << 1;
		break;
	case 2:
		val |= 0x1 << 2;
		break;
	case 3:
		val |= 0x1 << 1;
		val |= 0x1 << 2;
		break;
	default:
		return;
	}

	os04e10_standby(os04e10);
	os04e10_write_reg(os04e10, ori_addr, REG_VALUE_08BIT, val);
	os04e10_restart(os04e10);
}

static int os04e10_update_link_menu(struct os04e10 *os04e10)
{
	struct i2c_client *client = v4l2_get_subdevdata(&os04e10->sd);
	struct v4l2_ctrl_handler *ctrl_hdlr;
	struct v4l2_ctrl *ctrl;
	int wdr_index = SNS_CFG_TYPE_WDR_MODE;
	int id = os04e10->module_index;
	int ret;
	int i;

	os04e10_link_cif_menu[id][wdr_index] = os04e10->cur_mode->mipi_wdr_mode;

	dev_info(&client->dev, "update mipi_mode:%lld", os04e10_link_cif_menu[id][wdr_index]);

	ctrl_hdlr = os04e10->sd.ctrl_handler;
	v4l2_ctrl_handler_free(ctrl_hdlr);

	ret = v4l2_ctrl_handler_init(ctrl_hdlr, 10);
	if (ret) {
		dev_err(&client->dev, "%s ctrl handler init failed (%d)\n",
			__func__, ret);
		return ret;
	}

	for (i = 0; i < SNS_CFG_TYPE_MAX; i++) {
		ctrl = v4l2_ctrl_new_int_menu(ctrl_hdlr, &os04e10_ctrl_ops, V4L2_CID_LINK_FREQ,
					      os04e10_link_cif_menu[id][i], 0,
					       (const s64 *)os04e10_link_cif_menu[id]);

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

static long os04e10_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct os04e10 *os04e10 = to_os04e10(sd);
	long ret = 0;

	switch (cmd) {
	case SNS_V4L2_GET_TYPE:
	{
		int type = 0;

		if (os04e10->cur_mode->mipi_wdr_mode == MIPI_WDR_MODE_NONE) {//linear
			type = OS04E10_SNS_TYPE_SDR;
		} else {//wdr
			type = OS04E10_SNS_TYPE_WDR;
		}
		memcpy(arg, &type, sizeof(int));
		break;
	}

	case SNS_V4L2_SET_MIRROR_FLIP:
	{
		int orient = 0;

		memcpy(&orient, arg, sizeof(int));
		os04e10_mirror_flip(os04e10, orient);
		break;
	}

	case SNS_V4L2_GET_I2C_INFO:
	{
		struct i2c_client *client = v4l2_get_subdevdata(&os04e10->sd);
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
			os04e10->cur_mode->mipi_wdr_mode = os04e10_wdr_mode;
		else
			os04e10->cur_mode->mipi_wdr_mode = MIPI_WDR_MODE_NONE;

		os04e10_update_link_menu(os04e10);
		break;
	}

	case SNS_V4L2_SET_SNS_SYNC_INFO:
	{
		memcpy(&os04e10->cur_mode->os04e10_sync_info, arg, sizeof(sns_sync_info_t));
		break;
	}

	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long os04e10_compat_ioctl32(struct v4l2_subdev *sd,
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

static const struct v4l2_subdev_core_ops os04e10_core_ops = {
	.ioctl = os04e10_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = os04e10_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops os04e10_video_ops = {
	.s_stream = set_stream,
};

static const struct v4l2_subdev_pad_ops os04e10_pad_ops = {
	.enum_mbus_code = enum_mbus_code,
	.get_fmt = get_pad_format,
	.set_fmt = set_pad_format,
	.enum_frame_size = enum_frame_size,
	.enum_frame_interval = enum_frame_interval,
	.get_mbus_config = g_mbus_config,
};

static const struct v4l2_subdev_ops os04e10_subdev_ops = {
	.core	= &os04e10_core_ops,
	.video  = &os04e10_video_ops,
	.pad    = &os04e10_pad_ops,
};

static const struct media_entity_operations os04e10_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static const struct v4l2_subdev_internal_ops os04e10_internal_ops = {
	.open = os04e10_open,
};

/* Initialize control handlers */
static int os04e10_init_controls(struct os04e10 *os04e10, int index_id)
{
	struct i2c_client *client = v4l2_get_subdevdata(&os04e10->sd);
	struct v4l2_fwnode_device_properties props;
	struct v4l2_ctrl_handler *ctrl_hdlr;
	struct v4l2_ctrl *ctrl;
	int ret;
	int i;

	ctrl_hdlr = &os04e10->ctrl_handler;
	ret = v4l2_ctrl_handler_init(ctrl_hdlr, 10);
	if (ret) {
		dev_err(&client->dev, "%s ctrl handler init failed (%d)\n",
			__func__, ret);
		return ret;
	}

	mutex_init(&os04e10->mutex);
	ctrl_hdlr->lock = &os04e10->mutex;
	for (i = 0; i < SNS_CFG_TYPE_MAX; i++) {
		if (i == SNS_CFG_TYPE_WDR_MODE)
			os04e10->cur_mode->mipi_wdr_mode = os04e10_link_cif_menu[index_id][i];

		ctrl = v4l2_ctrl_new_int_menu(ctrl_hdlr, &os04e10_ctrl_ops, V4L2_CID_LINK_FREQ,
					      os04e10_link_cif_menu[index_id][i], 0,
					       (const s64 *)os04e10_link_cif_menu[index_id]);

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

	ret = v4l2_ctrl_new_fwnode_properties(ctrl_hdlr, &os04e10_ctrl_ops,
					      &props);
	if (ret)
		goto error;

	os04e10->sd.ctrl_handler = ctrl_hdlr;

	return 0;

error:
	v4l2_ctrl_handler_free(ctrl_hdlr);
	mutex_destroy(&os04e10->mutex);

	return ret;
}

static void os04e10_free_controls(struct os04e10 *os04e10)
{
	v4l2_ctrl_handler_free(os04e10->sd.ctrl_handler);
	mutex_destroy(&os04e10->mutex);
}

static int os04e10_probe(struct i2c_client *client,
			 const struct i2c_device_id *devid)
{
	struct os04e10 *os04e10;
	struct v4l2_subdev *sd;
	struct device *dev = &client->dev;
	int index_id = os04e10_probe_index;
	int addr_num = sizeof(os04e10_i2c_list) / sizeof(unsigned short);
	int bus_id;
	int ret = -1;
	int i;

	dev_info(dev, "probe id[%d] start\n", os04e10_probe_index);

	os04e10_probe_index++;

	if (index_id >= MAX_SENSOR_DEVICE || index_id < 0) {
		dev_info(dev, "invalid devid(%d)\n", index_id);
		return ret;
	}

	os04e10 = devm_kzalloc(&client->dev, sizeof(*os04e10), GFP_KERNEL);
	if (!os04e10)
		return -ENOMEM;

	sd = &os04e10->sd;

	for (i = 0; i < addr_num; i++) {
		if (force_bus[index_id] < 0)
			bus_id = os04e10_bus_map[index_id];
		else
			bus_id = force_bus[index_id];

		if (bus_id < 0 || bus_id > MAX_I2C_BUS_NUM)
			return ret;

		client->addr = os04e10_i2c_list[i];
		client->adapter = i2c_get_adapter(bus_id);
		os04e10->client = client;
		v4l2_i2c_subdev_init(sd, client, &os04e10_subdev_ops);

		/* Check module identity */
		ret = os04e10_identify_module(os04e10);
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

	os04e10->module_index = index_id;

	if (index_id >= ARRAY_SIZE(supported_modes)) {
		os04e10->cur_mode = devm_kzalloc(&client->dev,
						 sizeof(struct os04e10_mode), GFP_KERNEL);
		memcpy(os04e10->cur_mode, &supported_modes[0], sizeof(struct os04e10_mode));
	} else {
		os04e10->cur_mode = devm_kzalloc(&client->dev,
						 sizeof(struct os04e10_mode), GFP_KERNEL);
		memcpy(os04e10->cur_mode, &supported_modes[index_id], sizeof(struct os04e10_mode));
	}

	memset(&os04e10->cur_mode->os04e10_sync_info, 0, sizeof(sns_sync_info_t));

	mutex_init(&os04e10->mutex);

	ret = os04e10_init_controls(os04e10, index_id);
	if (ret)
		return ret;

	/* Initialize subdev */
	sd->internal_ops = &os04e10_internal_ops;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	sd->entity.ops = &os04e10_subdev_entity_ops;
	sd->entity.function = MEDIA_ENT_F_CAM_SENSOR;

	/* Initialize source pad */
	os04e10->pad.flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&sd->entity, 1, &os04e10->pad);
	if (ret) {
		dev_err(&client->dev, "failed to init pads:%d\n", ret);
		goto error_handler_free;
	}

	snprintf(sd->name, sizeof(sd->name), "cam%d_%s %s",
		 os04e10->module_index, "os04e10", dev_name(sd->dev));

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
	media_entity_cleanup(&os04e10->sd.entity);

error_handler_free:
	os04e10_free_controls(os04e10);
	dev_err(&client->dev, "%s failed:%d\n", __func__, ret);

	return ret;
}

static int os04e10_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct os04e10 *os04e10 = to_os04e10(sd);

	os04e10_probe_index = 0;

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	os04e10_free_controls(os04e10);

	pm_runtime_disable(&client->dev);

	return 0;
}

static const struct of_device_id os04e10_of_match[] = {
	{ .compatible = "cvitek,sensor0" },
	{ .compatible = "cvitek,sensor1" },
	{ .compatible = "cvitek,sensor2" },
	{ .compatible = "cvitek,sensor3" },
	{ .compatible = "cvitek,sensor4" },
	{ .compatible = "cvitek,sensor5" },
	{},
};
MODULE_DEVICE_TABLE(of, os04e10_of_match);

static const struct dev_pm_ops os04e10_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(suspend, resume)
};

static struct i2c_driver os04e10_i2c_driver = {
	.driver = {
		.name = "os04e10",
		.pm = &os04e10_pm_ops,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(os04e10_of_match),
	},
	.probe    = os04e10_probe,
	.remove   = os04e10_remove,
};

static int __init sensor_mod_init(void)
{
	pr_info("== os04e10 mod add ==\n");

	return i2c_add_driver(&os04e10_i2c_driver);
}

static void __exit sensor_mod_exit(void)
{
	i2c_del_driver(&os04e10_i2c_driver);
}

module_init(sensor_mod_init);
module_exit(sensor_mod_exit);

MODULE_DESCRIPTION("os04e10 sensor driver");
MODULE_LICENSE("GPL v2");
