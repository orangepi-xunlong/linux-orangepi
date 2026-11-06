// SPDX-License-Identifier: GPL-2.0
/*
 * pr2100 driver
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
#include <linux/comm_vi.h>
#include <linux/sns_v4l2_uapi.h>

#include "pr2100.h"

/* I2C per write of bits */
#define REG_VALUE_08BIT		1
#define REG_VALUE_16BIT		2
#define REG_VALUE_24BIT		3
#define PAGE_SELECT 0XFF
/* Chip ID */
#define PR2100_CHIP_ID_ADDR_L		0xfd
#define PR2100_CHIP_ID_ADDR_H		0xfc
#define PR2100_CHIP_ID			0x2100

static int pr2100_count;

static int force_bus[MAX_SENSOR_DEVICE] = {[0 ... (MAX_SENSOR_DEVICE - 1)] = 0xFF};
module_param_array(force_bus, int, &pr2100_count, 0644);

static int force_i2caddr[MAX_SENSOR_DEVICE] = {[0 ... (MAX_SENSOR_DEVICE - 1)] = 0xFF};
module_param_array(force_i2caddr, int, &pr2100_count, 0644);

static int force_slave[MAX_SENSOR_DEVICE] = {[0 ... (MAX_SENSOR_DEVICE - 1)] = 0xFF};
module_param_array(force_slave, int, &pr2100_count, 0644);

static int pr2100_probe_index;

static unsigned short pr2100_i2caddr_map[] = {0x5F, 0x5F, 0x5C, 0x5C, 0x5F, 0x5F, 0x5c, 0x5c};
static int pr2100_bus_map[MAX_SENSOR_DEVICE] = {1, -1, -1, -1, -1, -1, -1, -1};
static int pr2100_slave_map[MAX_SENSOR_DEVICE] = {0, 0, 1, 1, 0, 0, 1, 1};
static int pr2100_type_map[MAX_SENSOR_DEVICE] = {
	V4L2_PIXELPLUS_PR2100_2M_25FPS_8BIT,
	V4L2_PIXELPLUS_PR2100_2M_4CH_25FPS_8BIT,
	V4L2_PIXELPLUS_PR2100_2M_4CH_25FPS_8BIT,
	V4L2_PIXELPLUS_PR2100_2M_4CH_25FPS_8BIT,
	V4L2_PIXELPLUS_PR2100_2M_4CH_25FPS_8BIT,
	V4L2_PIXELPLUS_PR2100_2M_4CH_25FPS_8BIT,
	V4L2_PIXELPLUS_PR2100_2M_4CH_25FPS_8BIT,
	V4L2_PIXELPLUS_PR2100_2M_4CH_25FPS_8BIT,
	V4L2_PIXELPLUS_PR2100_2M_2CH_25FPS_8BIT,
	V4L2_PIXELPLUS_PR2100_2M_2CH_25FPS_8BIT,
	V4L2_PIXELPLUS_PR2100_2M_2CH_25FPS_8BIT,
	V4L2_PIXELPLUS_PR2100_2M_2CH_25FPS_8BIT,
};

struct pr2100_yuv_format {
	vi_isp_yuv_scene_e	yuv_scene_mode;
	vi_intf_mode_e	inf_mode;
	vi_work_mode_e	mux_mode;
	vi_yuv_data_seq_e data_seq;
};
struct pr2100_reg_list {
	u32 num_of_regs;
	const struct pr2100_reg *regs;
};

/* Mode : resolution and related config&values */
struct pr2100_mode {
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
	sns_sync_info_t pr2100_sync_info;
	struct pr2100_yuv_format yuv_format;
	struct pr2100_reg_list reg_list;
	struct pr2100_reg_list wdr_reg_list;
};

/* Mode configs */
static struct pr2100_mode supported_modes[] = {
	{
		.max_width = 1920,
		.max_height = 1080,
		.width = 1920,
		.height = 1080,
		.max_fps = {
			.numerator = 10000,
			.denominator = 250000,
		},
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(mode_1920x1080_regs),
			.regs = mode_1920x1080_regs,
		},
	}
};

static struct pr2100_yuv_format yuv_format[] = {
	{
		.yuv_scene_mode = VI_ISP_YUV_SCENE_BYPASS,
		.inf_mode = VI_MODE_MIPI_YUV422,
		.mux_mode = VI_WORK_MODE_1MULTIPLEX,
		.data_seq = VI_DATA_SEQ_YUYV,
	},
	{
		.yuv_scene_mode = VI_ISP_YUV_SCENE_BYPASS,
		.inf_mode = VI_MODE_MIPI_YUV422,
		.mux_mode = VI_WORK_MODE_2MULTIPLEX,
		.data_seq = VI_DATA_SEQ_YUYV,
	},
	{
		.yuv_scene_mode = VI_ISP_YUV_SCENE_BYPASS,
		.inf_mode = VI_MODE_MIPI_YUV422,
		.mux_mode = VI_WORK_MODE_4MULTIPLEX,
		.data_seq = VI_DATA_SEQ_YUYV,
	},
};

struct pr2100 {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct i2c_client *client;
	struct v4l2_ctrl_handler ctrl_handler;
	/* V4L2 Controls */
	struct v4l2_ctrl *vblank;
	struct v4l2_ctrl *hblank;
	struct v4l2_ctrl *exposure;
	/* Current mode */
	struct pr2100_mode *cur_mode;
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

#define to_pr2100(_sd)	container_of(_sd, struct pr2100, sd)

/* Read registers up to 4 at a time */
static int pr2100_read_reg(struct pr2100 *pr2100, u8 reg, u32 len, u32 *val)
{
	struct i2c_client *client = v4l2_get_subdevdata(&pr2100->sd);
	struct i2c_msg msgs[2];
	u8 *data_be_p;
	int ret;
	__be32 data_be = 0;
	u8 reg_addr_be = reg;

	if (len > 5)
		return -EINVAL;

	data_be_p = (u8 *)&data_be;
	/* Write register address */
	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = (u8 *)&reg_addr_be;

	/* Read data from register */
	msgs[1].addr = client->addr;
	msgs[1].flags =  I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = &data_be_p[4 - len];

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret != ARRAY_SIZE(msgs))
		return -EIO;

	*val = be32_to_cpu(data_be);

	return 0;
}

/* Write registers up to 4 at a time */
static int pr2100_write_reg(struct pr2100 *pr2100, u16 reg, u32 len, u32 __val)
{
	struct i2c_client *client = v4l2_get_subdevdata(&pr2100->sd);
	int buf_i, val_i;
	u8 buf[6], *val_p;
	__be32 val;

	if (len > 4)
		return -EINVAL;

	buf[0] = reg;

	val = cpu_to_be32(__val);
	val_p = (u8 *)&val;
	buf_i = 1;
	val_i = 4 - len;

	while (val_i < 4)
		buf[buf_i++] = val_p[val_i++];

	if (i2c_master_send(client, buf, len + 1) != len + 1)
		return -EIO;

	return 0;
}

/* Write a list of registers */
static int pr2100_write_regs(struct pr2100 *pr2100, const struct pr2100_reg *regs, u32 len)
{
	struct i2c_client *client = v4l2_get_subdevdata(&pr2100->sd);
	int ret;
	u32 i;

	for (i = 0; i < len; i++) {
		ret = pr2100_write_reg(pr2100, regs[i].address, 1,
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
static int pr2100_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct pr2100 *pr2100 = to_pr2100(sd);
	struct v4l2_mbus_framefmt *try_fmt =
			v4l2_subdev_get_try_format(sd, fh->pad, 0);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int module_id = pr2100->module_index;

	mutex_lock(&pr2100->mutex);

	/* Initialize try_fmt */
	try_fmt->width = pr2100->cur_mode->width;
	try_fmt->height = pr2100->cur_mode->height;
	try_fmt->code = MEDIA_BUS_FMT_VUY8_1X24;
	try_fmt->field = V4L2_FIELD_NONE;


	switch (pr2100_type_map[module_id]) {
		case V4L2_PIXELPLUS_PR2100_2M_25FPS_8BIT:
			memcpy(&pr2100->cur_mode->yuv_format, &yuv_format[0],
				sizeof(struct pr2100_yuv_format));
			break;
		case V4L2_PIXELPLUS_PR2100_2M_2CH_25FPS_8BIT:
		case V4L2_PIXELPLUS_PR2100_2M_2CH_2L_25FPS_8BIT:
			memcpy(&pr2100->cur_mode->yuv_format, &yuv_format[1],
				sizeof(struct pr2100_yuv_format));
			break;
		case V4L2_PIXELPLUS_PR2100_2M_4CH_25FPS_8BIT:
			memcpy(&pr2100->cur_mode->yuv_format, &yuv_format[2],
				sizeof(struct pr2100_yuv_format));
			break;
		default:
			dev_err(&client->dev, "unknown sensor type:%d\n", pr2100_type_map[module_id]);
			break;
	}

	/* No crop or compose */
	mutex_unlock(&pr2100->mutex);

	return 0;
}

static int pr2100_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct pr2100 *pr2100 = container_of(ctrl->handler,
					       struct pr2100, ctrl_handler);
	struct i2c_client *client = v4l2_get_subdevdata(&pr2100->sd);

	pm_runtime_put(&client->dev);

	return 0;
}

static const struct v4l2_ctrl_ops pr2100_ctrl_ops = {
	.s_ctrl = pr2100_set_ctrl,
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

	code->code = MEDIA_BUS_FMT_VUY8_1X24;

	return 0;
}

static int enum_frame_interval(struct v4l2_subdev *sd,
			      struct v4l2_subdev_pad_config *cfg,
			      struct v4l2_subdev_frame_interval_enum *fie)
{
	struct pr2100 *pr2100 = to_pr2100(sd);

	fie->width  = pr2100->cur_mode->width;
	fie->height = pr2100->cur_mode->height;

	fie->interval.numerator   = pr2100->cur_mode->max_fps.numerator;
	fie->interval.denominator = pr2100->cur_mode->max_fps.denominator;

	return 0;
}

static int enum_frame_size(struct v4l2_subdev *sd,
			   struct v4l2_subdev_pad_config *cfg,
			   struct v4l2_subdev_frame_size_enum *fse)
{
	struct pr2100 *pr2100 = to_pr2100(sd);

	fse->min_width = pr2100->cur_mode->width;
	fse->max_width = pr2100->cur_mode->max_width;
	fse->min_height = pr2100->cur_mode->height;
	fse->max_height = pr2100->cur_mode->max_height;

	return 0;
}

static void update_pad_format(const struct pr2100_mode *mode, struct v4l2_subdev_format *fmt)
{
	fmt->format.width = mode->width;
	fmt->format.height = mode->height;
	fmt->format.code = MEDIA_BUS_FMT_VUY8_1X24;
	fmt->format.field = V4L2_FIELD_NONE;

	//set yuv format
	fmt->format.reserved[YUV_SCENCE_MODE] = mode->yuv_format.yuv_scene_mode;
	fmt->format.reserved[YUV_INF_MODE] = mode->yuv_format.inf_mode;
	fmt->format.reserved[YUV_MUX_MODE] = mode->yuv_format.mux_mode;
	fmt->format.reserved[YUV_DATA_SEQ] = mode->yuv_format.data_seq;
}

static int get_pad_format(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct pr2100 *pr2100 = to_pr2100(sd);
	struct v4l2_mbus_framefmt *framefmt;
	int ret = 0;

	mutex_lock(&pr2100->mutex);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		framefmt = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
		fmt->format = *framefmt;
		ret = -ENOTTY;
	} else {
		update_pad_format(pr2100->cur_mode, fmt);
	}
	mutex_unlock(&pr2100->mutex);

	return ret;
}

static int set_pad_format(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct pr2100 *pr2100 = to_pr2100(sd);
	struct pr2100_mode *mode;
	struct v4l2_mbus_framefmt *framefmt;

	mutex_lock(&pr2100->mutex);

	/* Only one raw bayer(GRBG) order is supported */
	if (fmt->format.code != MEDIA_BUS_FMT_VUY8_1X24)
		fmt->format.code = MEDIA_BUS_FMT_VUY8_1X24;

	mode = v4l2_find_nearest_size(supported_modes,
				      ARRAY_SIZE(supported_modes),
				      width, height,
				      fmt->format.width, fmt->format.height);
	update_pad_format(mode, fmt);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		framefmt = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
		*framefmt = fmt->format;
	} else {
		pr2100->cur_mode = mode;
	}

	mutex_unlock(&pr2100->mutex);

	return 0;
}

static void pr2100_standby(struct pr2100 *pr2100)
{
	pr2100_write_reg(pr2100, 0x0100, REG_VALUE_08BIT, 0x00);
}

//static void pr2100_restart(struct pr2100 *pr2100)
//{
//	pr2100_write_reg(pr2100, 0x0100, REG_VALUE_08BIT, 0x01);
//}

/* Start streaming */
static int start_streaming(struct pr2100 *pr2100)
{
	struct i2c_client *client = v4l2_get_subdevdata(&pr2100->sd);
	const sns_sync_info_t *sync_info;
	int module_id = pr2100->module_index;
	int ret = 0;
	u32 val;

	switch (pr2100_type_map[module_id]) {
	case V4L2_PIXELPLUS_PR2100_2M_25FPS_8BIT:
	{
		dev_info(&client->dev, "1 channal out regs start to write\n");
		ret = pr2100_write_regs(pr2100, mode_1920x1080_regs,
					ARRAY_SIZE(mode_1920x1080_regs));
		break;
	}
	case V4L2_PIXELPLUS_PR2100_2M_2CH_25FPS_8BIT:
	{
		dev_info(&client->dev, "2 channal out regs start to write\n");
		ret = pr2100_write_regs(pr2100, mode_1920x1080_2ch_regs,
					 ARRAY_SIZE(mode_1920x1080_2ch_regs));
		break;
	}
	case V4L2_PIXELPLUS_PR2100_2M_2CH_2L_25FPS_8BIT:
	{
		dev_info(&client->dev, "2 channal 2 lane out regs start to write\n");
		ret = pr2100_write_regs(pr2100, mode_1920x1080_2ch_2l_regs,
					 ARRAY_SIZE(mode_1920x1080_2ch_2l_regs));
		break;
	}
	case V4L2_PIXELPLUS_PR2100_2M_4CH_25FPS_8BIT:
	{
		int is_slave = 0;

		is_slave = force_slave[module_id] == 0xFF ?
				pr2100_slave_map[module_id] : force_slave[module_id];

		if (is_slave) {
			dev_info(&client->dev, "4 channal slave regs start to write\n");
			ret = pr2100_write_regs(pr2100, mode_1920x1080_4ch_slave_regs,
						ARRAY_SIZE(mode_1920x1080_4ch_slave_regs));
		} else {
			dev_info(&client->dev, "4 channal master regs start to write\n");
			ret = pr2100_write_regs(pr2100, mode_1920x1080_4ch_regs,
						ARRAY_SIZE(mode_1920x1080_4ch_regs));
		}

		break;
	}
	default:
		dev_err(&client->dev, "unknown sensor type:%d\n", pr2100_type_map[module_id]);
		break;
	}

	if (ret) {
		dev_err(&client->dev, "%s failed to set mode\n", __func__);
		return ret;
	}

	sync_info = &pr2100->cur_mode->pr2100_sync_info;
	if (sync_info->num_of_regs > 0) {
		ret = pr2100_write_regs(pr2100, (struct pr2100_reg *)sync_info->regs,
					sync_info->num_of_regs);
		if (ret) {
			dev_err(&client->dev, "%s failed to set default\n", __func__);
			return ret;
		}
	}

	usleep_range(100 * 1000, 200 * 1000);
	/* Apply customized values from user */
	ret =  __v4l2_ctrl_handler_setup(pr2100->sd.ctrl_handler);
	if (ret)
		return ret;

	dev_info(&client->dev, "wdr_mode(%d) reg setting done\n", pr2100->cur_mode->mipi_wdr_mode);

	return ret;
}

/* Stop streaming */
static int stop_streaming(struct pr2100 *pr2100)
{
	pr2100_standby(pr2100);
	return 0;
}

static int set_stream(struct v4l2_subdev *sd, int enable)
{
	struct pr2100 *pr2100 = to_pr2100(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	mutex_lock(&pr2100->mutex);
	if (pr2100->streaming == enable) {
		mutex_unlock(&pr2100->mutex);
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
		ret = start_streaming(pr2100);
		if (ret)
			goto err_rpm_put;
	} else {
		stop_streaming(pr2100);
		pm_runtime_put(&client->dev);
	}

	pr2100->streaming = enable;
	mutex_unlock(&pr2100->mutex);

	dev_info(&client->dev, "set stream(%d) success\n", enable);

	return ret;

err_rpm_put:
	pm_runtime_put(&client->dev);
err_unlock:
	mutex_unlock(&pr2100->mutex);

	return ret;
}

static int __maybe_unused suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct pr2100 *pr2100 = to_pr2100(sd);

	if (pr2100->streaming)
		stop_streaming(pr2100);

	return 0;
}

static int __maybe_unused resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct pr2100 *pr2100 = to_pr2100(sd);
	int ret;

	if (pr2100->streaming) {
		ret = start_streaming(pr2100);
		if (ret)
			goto error;
	}

	return 0;

error:
	stop_streaming(pr2100);
	pr2100->streaming = false;
	return ret;
}

/* Verify chip ID */
static int pr2100_identify_module(struct pr2100 *pr2100)
{
	int ret;
	int val1, val2;
	int read_data = 0;
	struct i2c_client *client = v4l2_get_subdevdata(&pr2100->sd);


	pr2100_write_reg(pr2100, PAGE_SELECT, 1, 0x00);  //select page0

	ret = pr2100_read_reg(pr2100, PR2100_CHIP_ID_ADDR_L,
			       REG_VALUE_08BIT, &val1);
	ret = pr2100_read_reg(pr2100, PR2100_CHIP_ID_ADDR_H,
			       REG_VALUE_08BIT, &val2);
	read_data = (val1 & 0xFF) | ((val2 & 0xFF) << 8);

	if (ret)
		return ret;

	if (read_data != PR2100_CHIP_ID) {
		dev_err(&client->dev, "chip id(%x) mismatch, read(%x)\n",
			PR2100_CHIP_ID, read_data);
		return -EIO;
	}

	return 0;
}

static long pr2100_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct pr2100 *pr2100 = to_pr2100(sd);
	long ret = 0;

	switch (cmd) {
	case SNS_V4L2_GET_TYPE:
	{
		int type = 0;
		int index = pr2100->module_index;

		if (index < 0 || index >= MAX_SENSOR_DEVICE) {
			pr_info("invalid module_index:%d\n", index);
		}

		type = pr2100_type_map[index];
		memcpy(arg, &type, sizeof(int));

		break;
	}

	case SNS_V4L2_GET_I2C_INFO:
	{
		struct i2c_client *client = v4l2_get_subdevdata(&pr2100->sd);
		sns_i2c_info_t i2c_info;

		i2c_info.i2c_addr =  client->addr;
		i2c_info.i2c_idx  =  client->adapter->i2c_idx;
		memcpy(arg, &i2c_info, sizeof(sns_i2c_info_t));
		break;
	}

	case SNS_V4L2_SET_SNS_SYNC_INFO:
	{
		memcpy(&pr2100->cur_mode->pr2100_sync_info, arg, sizeof(sns_sync_info_t));
		break;
	}

	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long pr2100_compat_ioctl32(struct v4l2_subdev *sd,
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

static const struct v4l2_subdev_core_ops pr2100_core_ops = {
	.ioctl = pr2100_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = pr2100_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops pr2100_video_ops = {
	.s_stream = set_stream,
};

static const struct v4l2_subdev_pad_ops pr2100_pad_ops = {
	.enum_mbus_code = enum_mbus_code,
	.get_fmt = get_pad_format,
	.set_fmt = set_pad_format,
	.enum_frame_size = enum_frame_size,
	.enum_frame_interval = enum_frame_interval,
	.get_mbus_config = g_mbus_config,
};

static const struct v4l2_subdev_ops pr2100_subdev_ops = {
	.core	= &pr2100_core_ops,
	.video  = &pr2100_video_ops,
	.pad    = &pr2100_pad_ops,
};

static const struct media_entity_operations pr2100_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static const struct v4l2_subdev_internal_ops pr2100_internal_ops = {
	.open = pr2100_open,
};

/* Initialize control handlers */
static int pr2100_init_controls(struct pr2100 *pr2100, int index_id)
{
	struct i2c_client *client = v4l2_get_subdevdata(&pr2100->sd);
	struct v4l2_fwnode_device_properties props;
	struct v4l2_ctrl_handler *ctrl_hdlr;
	struct v4l2_ctrl *ctrl;
	int ret;
	int i;

	ctrl_hdlr = &pr2100->ctrl_handler;
	ret = v4l2_ctrl_handler_init(ctrl_hdlr, 10);
	if (ret) {
		dev_err(&client->dev, "%s ctrl handler init failed (%d)\n",
			__func__, ret);
		return ret;
	}

	mutex_init(&pr2100->mutex);
	ctrl_hdlr->lock = &pr2100->mutex;
	for (i = 0; i < SNS_CFG_TYPE_MAX; i++) {
		if (i == SNS_CFG_TYPE_WDR_MODE)
			pr2100->cur_mode->mipi_wdr_mode = pr2100_link_cif_menu[index_id][i];

		ctrl = v4l2_ctrl_new_int_menu(ctrl_hdlr, &pr2100_ctrl_ops, V4L2_CID_LINK_FREQ,
					      pr2100_link_cif_menu[index_id][i], 0,
					       (const s64 *)pr2100_link_cif_menu[index_id]);

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

	ret = v4l2_ctrl_new_fwnode_properties(ctrl_hdlr, &pr2100_ctrl_ops,
					      &props);
	if (ret)
		goto error;

	pr2100->sd.ctrl_handler = ctrl_hdlr;

	return 0;

error:
	v4l2_ctrl_handler_free(ctrl_hdlr);
	mutex_destroy(&pr2100->mutex);

	return ret;
}

static void pr2100_free_controls(struct pr2100 *pr2100)
{
	v4l2_ctrl_handler_free(pr2100->sd.ctrl_handler);
	mutex_destroy(&pr2100->mutex);
}

static void pr2100_confirm_sensor_type(int id)
{
	int i;

	if (id < 0 || id >= MAX_SENSOR_DEVICE || force_slave[id] == 0xFF)
		return;

	if (force_slave[id] == 1) {
		for (i = id - 2; i <= id; i++) {
			pr2100_type_map[i] = V4L2_PIXELPLUS_PR2100_2M_4CH_25FPS_8BIT;
			pr2100_link_cif_menu[i][SNS_CFG_TYPE_WDR_MODE] = MIPI_WDR_MODE_VC;
		}
	} else if (force_slave[id] == 0 && id > 0) {
		if (force_slave[id - 1] != 1)
			for (i = id - 1; i <= id; i++) {
				if (pr2100_link_cif_menu[i][SNS_CFG_TYPE_DATA_LANE3] == -1) //if 2 lane
					pr2100_type_map[i] = V4L2_PIXELPLUS_PR2100_2M_2CH_2L_25FPS_8BIT;
				else
					pr2100_type_map[i] = V4L2_PIXELPLUS_PR2100_2M_2CH_25FPS_8BIT;
			}
	} else {
		pr2100_type_map[id] = V4L2_PIXELPLUS_PR2100_2M_25FPS_8BIT;
		pr2100_link_cif_menu[id][SNS_CFG_TYPE_WDR_MODE] = MIPI_WDR_MODE_VC;
	}
}

static int pr2100_probe(struct i2c_client *client,
			 const struct i2c_device_id *devid)
{
	struct pr2100 *pr2100;
	struct v4l2_subdev *sd;
	struct device *dev = &client->dev;
	int index_id = pr2100_probe_index;
	int bus_id;
	int ret = -1;

	dev_info(dev, "probe id[%d] start\n", pr2100_probe_index);

	pr2100_probe_index++;

	if (index_id >= MAX_SENSOR_DEVICE || index_id < 0) {
		dev_info(dev, "invalid devid(%d)\n", index_id);
		return ret;
	}

	pr2100 = devm_kzalloc(&client->dev, sizeof(*pr2100), GFP_KERNEL);
	if (!pr2100)
		return -ENOMEM;

	sd = &pr2100->sd;

	if (force_bus[index_id] == 0xFF)
		bus_id = pr2100_bus_map[index_id];
	else
		bus_id = force_bus[index_id];

	if (bus_id < 0 || bus_id > MAX_I2C_BUS_NUM)
		return ret;

	if (force_i2caddr[index_id] == 0xFF) {
		if (index_id >= ARRAY_SIZE(pr2100_i2caddr_map))
			client->addr = pr2100_i2caddr_map[index_id - 1];
		else
			client->addr = pr2100_i2caddr_map[index_id];
	} else
		client->addr = force_i2caddr[index_id];

	client->adapter = i2c_get_adapter(bus_id);
	pr2100->client = client;
	v4l2_i2c_subdev_init(sd, client, &pr2100_subdev_ops);

	/* Check module identity */
	ret = pr2100_identify_module(pr2100);
	if (ret) {
		dev_info(dev, "id[%d] bus[%d] i2c_addr[0x%x] no sensor found\n",
			 index_id, bus_id, client->addr);

		return ret;
	} else
		dev_info(dev, "id[%d] bus[%d] i2c_addr[0x%x] sensor found\n",
			 index_id, bus_id, client->addr);

	pr2100->module_index = index_id;

	pr2100_confirm_sensor_type(index_id);

	if (index_id >= ARRAY_SIZE(supported_modes)) {
		pr2100->cur_mode = devm_kzalloc(&client->dev,
						 sizeof(struct pr2100_mode), GFP_KERNEL);
		memcpy(pr2100->cur_mode, &supported_modes[0], sizeof(struct pr2100_mode));
	} else {
		pr2100->cur_mode = devm_kzalloc(&client->dev,
						 sizeof(struct pr2100_mode), GFP_KERNEL);
		memcpy(pr2100->cur_mode, &supported_modes[index_id], sizeof(struct pr2100_mode));
	}

	memset(&pr2100->cur_mode->pr2100_sync_info, 0, sizeof(sns_sync_info_t));

	mutex_init(&pr2100->mutex);

	ret = pr2100_init_controls(pr2100, index_id);
	if (ret)
		return ret;

	/* Initialize subdev */
	sd->internal_ops = &pr2100_internal_ops;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	sd->entity.ops = &pr2100_subdev_entity_ops;
	sd->entity.function = MEDIA_ENT_F_CAM_SENSOR;

	/* Initialize source pad */
	pr2100->pad.flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&sd->entity, 1, &pr2100->pad);
	if (ret) {
		dev_err(&client->dev, "failed to init pads:%d\n", ret);
		goto error_handler_free;
	}

	snprintf(sd->name, sizeof(sd->name), "cam%d_%s %s",
		 pr2100->module_index, "pr2100", dev_name(sd->dev));

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
	media_entity_cleanup(&pr2100->sd.entity);

error_handler_free:
	pr2100_free_controls(pr2100);
	dev_err(&client->dev, "%s failed:%d\n", __func__, ret);

	return ret;
}

static int pr2100_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct pr2100 *pr2100 = to_pr2100(sd);

	pr2100_probe_index = 0;

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	pr2100_free_controls(pr2100);

	pm_runtime_disable(&client->dev);

	return 0;
}

static const struct of_device_id pr2100_of_match[] = {
	{ .compatible = "v4l2,sensor0" },
	{ .compatible = "v4l2,sensor1" },
	{ .compatible = "v4l2,sensor2" },
	{ .compatible = "v4l2,sensor3" },
	{ .compatible = "v4l2,sensor4" },
	{ .compatible = "v4l2,sensor5" },
	{ .compatible = "v4l2,sensor6" },
	{ .compatible = "v4l2,sensor7" },
	{ .compatible = "v4l2,sensor8" },
	{ .compatible = "v4l2,sensor9" },
	{ .compatible = "v4l2,sensor10" },
	{ .compatible = "v4l2,sensor11" },
	{},
};
MODULE_DEVICE_TABLE(of, pr2100_of_match);

static const struct dev_pm_ops pr2100_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(suspend, resume)
};

static struct i2c_driver pr2100_i2c_driver = {
	.driver = {
		.name = "pr2100",
		.pm = &pr2100_pm_ops,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(pr2100_of_match),
	},
	.probe    = pr2100_probe,
	.remove   = pr2100_remove,
};

static int __init sensor_mod_init(void)
{
	pr_info("== pr2100 mod add ==\n");

	return i2c_add_driver(&pr2100_i2c_driver);
}

static void __exit sensor_mod_exit(void)
{
	i2c_del_driver(&pr2100_i2c_driver);
}

module_init(sensor_mod_init);
module_exit(sensor_mod_exit);

MODULE_DESCRIPTION("pr2100 sensor driver");
MODULE_LICENSE("GPL v2");
