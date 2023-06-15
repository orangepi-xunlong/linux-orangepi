/*
 * A V4L2 driver for OV8858 Raw cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *    Yang Feng <yangfeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>
#include "camera.h"
#include "sensor_helper.h"

MODULE_AUTHOR("lwj");
MODULE_DESCRIPTION("A low-level driver for c2590_mipi Raw sensors");
MODULE_LICENSE("GPL");

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR  0x0202

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The c2590_mipi i2c address
 */
#define I2C_ADDR 0x6c
#define C2590_WRITE_ADDR (0x6c)
#define C2590_READ_ADDR  (0x6d)

#define SENSOR_NAME "c2590_mipi"

struct cfg_array {		/* coming later */
	struct regval_list *regs;
	int size;
};
/*
 * The default register settings
 *
 */

static struct regval_list sensor_default_regs[] = {
};
static struct regval_list sensor_1600_1200_regs[] = {
	{0x0103, 0x01},
	{REG_DLY, 0x05},
	{0x0100, 0x00},
	{0x3400, 0x00},
	{0x3404, 0x05},
	{0x3500, 0x10},
	{0xe000, 0x02},
	{0xe001, 0x02},
	{0xe002, 0x02},
	{0xe003, 0x02},
	{0xe004, 0x03},
	{0xe005, 0x68},
	{0xe006, 0x02},
	{0xe007, 0x05},
	{0xe008, 0x00},
	{0xe009, 0x03},
	{0xe00A, 0x40},
	{0xe00B, 0x02},
	{0xe00C, 0x03},
	{0xe00D, 0x41},
	{0xe00E, 0x68},
	{0x3500, 0x00},

	{0x0101, 0x03},

	{0x3180, 0xf0},
	{0x3181, 0x40},
	{0x3584, 0x00},

	{0x3280, 0x06},
	{0x3281, 0x05},
	{0x3282, 0x93},
	{0x3283, 0xd2},
	{0x3287, 0x46},
	{0x3288, 0x5f},
	{0x3289, 0x30},
	{0x328A, 0x21},
	{0x328B, 0x44},
	{0x328C, 0x78},
	{0x328D, 0x55},
	{0x328E, 0x00},
	{0x308a, 0x00},
	{0x308b, 0x00},
	{0x3209, 0x80},
	{0x320C, 0x00},
	{0x320E, 0x08},
	{0x3210, 0x11},
	{0x3211, 0x09},
	{0x3212, 0x1a},
	{0x3213, 0x15},
	{0x3214, 0x17},
	{0x3215, 0x09},
	{0x3216, 0x17},
	{0x3217, 0x06},
	{0x3218, 0x28},
	{0x3219, 0x12},
	{0x321A, 0x00},
	{0x321B, 0x04},
	{0x321C, 0x00},
	{0x3200, 0x03},
	{0x3201, 0x46},
	{0x3202, 0x00},
	{0x3203, 0x17},
	{0x3204, 0x01},
	{0x3205, 0x64},
	{0x3206, 0x00},
	{0x3207, 0xde},
	{0x3208, 0x83},

	{0x0303, 0x01},
	{0x0304, 0x00},
	{0x0305, 0x03},
	{0x0307, 0xa9},
	{0x0309, 0x30},
	{0x0342, 0x0b},
	{0x0343, 0x5c},
	{0x3087, 0x90},
	{0x3089, 0x18},
	{0x3c03, 0x01},

	{0x3d00, 0xad},
	{0x3d01, 0x15},
	{0x3d07, 0x40},
	{0x3d08, 0x40},

	{0x3805, 0x06},
	{0x3806, 0x06},
	{0x3807, 0x06},
	{0x3808, 0x14},
	{0x3809, 0xc4},
	{0x380a, 0x6c},
	{0x380b, 0x8c},
	{0x380c, 0x21},
	{0x3009, 0x05},
	{0x300b, 0x05},
	{0x0101, 0x03},
	{0x0100, 0x01},/* stream on*/
};

/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */

static struct regval_list sensor_fmt_raw[] = {
};

static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->exp;
	sensor_dbg("sensor_get_exposure = %d\n", info->exp);
	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	return 0;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->gain;
	sensor_dbg("sensor_get_gain = %d\n", info->gain);
	return 0;
}

unsigned int BASEGAIN = 128;

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	return 0;
}

static int glb_exp_val = 17626*2;
static int glb_gain_val = 240;
static int c2590_sensor_vts;
static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val, frame_length, shutter;
	unsigned char explow = 0, expmid = 0;
	unsigned char gainlow = 0;
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	if (gain_val < 1 * 16)
		gain_val = 16;
	if (gain_val > 16 * 16 - 1)
		gain_val = 16 * 16 - 1;

	if (exp_val > 17626*2)
		exp_val = 17626*2;

	glb_exp_val = exp_val;
	glb_gain_val = gain_val;

	gainlow = (unsigned char)((gain_val-16) & 0xff);

	shutter = exp_val >> 4;

	expmid = (unsigned char)(((shutter) >> 8) & 0xff);
	explow = (unsigned char)((shutter) & 0xff);

	if (shutter > c2590_sensor_vts - 4)
		frame_length = shutter + 4;
	else
		frame_length = c2590_sensor_vts;


	sensor_write(sd, 0x0341, (frame_length & 0xff));
	sensor_write(sd, 0x0340, (frame_length >> 8));

	sensor_write(sd, 0x0205, gainlow);


	sensor_write(sd, 0x0203, explow);
	sensor_write(sd, 0x0202, expmid);

	info->exp = exp_val;
	info->gain = gain_val;
	return 0;

}

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	int ret = 0;
	return ret;
}

/*
 * Stuff that knows about the sensor.
 */

static int sensor_power(struct v4l2_subdev *sd, int on)
{
	int ret;

	ret = 0;
	switch (on) {
	case STBY_ON:
		sensor_dbg("STBY_ON!\n");
		ret = sensor_s_sw_stby(sd, CSI_GPIO_HIGH);
		if (ret < 0)
			sensor_err("soft stby falied!\n");
		usleep_range(10000, 12000);
		cci_lock(sd);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		cci_unlock(sd);
		vin_set_mclk(sd, OFF);
		break;
	case STBY_OFF:
		sensor_dbg("STBY_OFF!\n");
		cci_lock(sd);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		ret = sensor_s_sw_stby(sd, CSI_GPIO_LOW);
		if (ret < 0)
			sensor_err("soft stby off falied!\n");
		cci_unlock(sd);
		break;
	case PWR_ON:
		sensor_print("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, POWER_EN, 1);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(7000, 8000);
		vin_set_pmu_channel(sd, IOVDD, ON);
		usleep_range(7000, 8000);
		vin_set_pmu_channel(sd, AVDD, ON);
		usleep_range(7000, 8000);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(7000, 8000);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_print("PWR_OFF!\n");
		cci_lock(sd);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vin_set_mclk(sd, OFF);
		usleep_range(7000, 8000);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_LOW);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		cci_unlock(sd);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	switch (val) {
	case 0:
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	unsigned int SENSOR_ID = 0;
	data_type val;
	int cnt = 0;

	sensor_read(sd, 0x0000, &val);
	SENSOR_ID |= (val << 8);
	sensor_read(sd, 0x0001, &val);
	SENSOR_ID |= (val);
	sensor_print("V4L2_IDENT_SENSOR = 0x%x\n", SENSOR_ID);

	while ((SENSOR_ID != V4L2_IDENT_SENSOR) && (cnt < 3)) {
		sensor_read(sd, 0x0000, &val);
		SENSOR_ID |= (val << 8);
		sensor_read(sd, 0x0001, &val);
		SENSOR_ID |= (val);
		sensor_print("retry = %d, V4L2_IDENT_SENSOR = %x\n", cnt, SENSOR_ID);
		cnt++;
	}

	if (SENSOR_ID != V4L2_IDENT_SENSOR)
		return -ENODEV;

	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_init\n");

	/*Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = 1600;
	info->height = 1200;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30;	/* 30fps */

	info->preview_first_flag = 1;

	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);

	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		if (info->current_wins != NULL) {
			memcpy(arg,
			       info->current_wins,
			       sizeof(struct sensor_win_size));
			ret = 0;
		} else {
			sensor_err("empty wins!\n");
			ret = -1;
		}
		break;
	case SET_FPS:
		break;
	case VIDIOC_VIN_SENSOR_EXP_GAIN:
		ret = sensor_s_exp_gain(sd, (struct sensor_exp_gain *)arg);
		break;
	case VIDIOC_VIN_SENSOR_CFG_REQ:
		sensor_cfg_req(sd, (struct sensor_config *)arg);
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

/*
 * Store information about the video data format.
 */
static struct sensor_format_struct sensor_formats[] = {
	{
		.desc = "Raw RGB Bayer",
		.mbus_code = MEDIA_BUS_FMT_SGBRG10_1X10,
		.regs = sensor_fmt_raw,
		.regs_size = ARRAY_SIZE(sensor_fmt_raw),
		.bpp = 1
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */
static struct sensor_win_size sensor_win_sizes[] = {
	/* 1600*1200 */
	{
	.width = 1600,
	.height = 1200,
	.hoffset = 0,
	.voffset = 0,
	.hts = 1126,
	.vts = 1243,
	.pclk = 42 * 1000 * 1000,
	.mipi_bps = 572 * 1000 * 1000,
	.fps_fixed = 30,
	.bin_factor = 1,
	.intg_min = 4 << 4,
	.intg_max = (1224 - 4) << 4,
	.gain_min = 1 << 4,
	.gain_max = (16*16) << 4,
	.regs = sensor_1600_1200_regs,
	.regs_size = ARRAY_SIZE(sensor_1600_1200_regs),
	.set_size = NULL,
	 },

	/* 720P */
/*	{
	.width = 1280,
	.height = 720,
	.hoffset = 0,
	.voffset = 0,
	.hts = 1446,
	.vts = 760,
	.pclk = 66 * 1000 * 1000,
	.mipi_bps = 60 * 1000 * 1000,
	.fps_fixed = 60,
	.bin_factor = 1,
	.intg_min = 4 << 4,
	.intg_max = (720 - 12) << 4,
	.gain_min = 1 << 4,
	.gain_max = 1440 << 4,
	.regs = sensor_720P_60fps_regs,
	.regs_size = ARRAY_SIZE(sensor_720P_60fps_regs),
	.set_size = NULL,

	 },*/

};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_reg_init(struct sensor_info *info)
{

	int ret = 0;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;
	struct sensor_exp_gain exp_gain;

	ret = sensor_write_array(sd, sensor_default_regs,
			       ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);

	if (wsize->set_size)
		wsize->set_size(sd);

	info->width = wsize->width;
	info->height = wsize->height;
	info->exp = 0;
	info->gain = 0;
	c2590_sensor_vts = wsize->vts;

	exp_gain.exp_val = glb_exp_val;
	exp_gain.gain_val = glb_gain_val;

	sensor_s_exp_gain(sd, &exp_gain);

	sensor_print("s_fmt set width = %d, height = %d\n", wsize->width,
		      wsize->height);

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);

	sensor_print("%s on = %d, %d*%d %x\n", __func__, enable,
		  info->current_wins->width,
		  info->current_wins->height, info->fmt->mbus_code);

	if (!enable)
		return 0;
	return sensor_reg_init(info);
}

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_CSI2_DPHY;
	cfg->flags = 0 | V4L2_MBUS_CSI2_1_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
	return 0;
}

static int sensor_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *info =
			container_of(ctrl->handler, struct sensor_info, handler);
	struct v4l2_subdev *sd = &info->sd;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->val);
	case V4L2_CID_EXPOSURE:
		return sensor_g_exp(sd, &ctrl->val);
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *info =
			container_of(ctrl->handler, struct sensor_info, handler);
	struct v4l2_subdev *sd = &info->sd;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->val);
	case V4L2_CID_EXPOSURE:
		return sensor_s_exp(sd, ctrl->val);
	}
	return -EINVAL;
}

/* ----------------------------------------------------------------------- */

static const struct v4l2_ctrl_ops sensor_ctrl_ops = {
	.g_volatile_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
};

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = sensor_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_stream = sensor_s_stream,
	.g_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.pad = &sensor_pad_ops,
};


/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv = {
	.name = SENSOR_NAME,
	.addr_width = CCI_BITS_16,
	.data_width = CCI_BITS_8,
};

static const struct v4l2_ctrl_config sensor_custom_ctrls[] = {
	{
		.ops = &sensor_ctrl_ops,
		.id = V4L2_CID_FRAME_RATE,
		.name = "frame rate",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 15,
		.max = 120,
		.step = 1,
		.def = 120,
	},
};

static int sensor_init_controls(struct v4l2_subdev *sd, const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	struct v4l2_ctrl *ctrl;
	int i;
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 2 + ARRAY_SIZE(sensor_custom_ctrls));

	v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 1600,
			      256 * 1600, 1, 1 * 1600);
	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 0,
			      65536 * 16, 1, 0);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
	for (i = 0; i < ARRAY_SIZE(sensor_custom_ctrls); i++)
		v4l2_ctrl_new_custom(handler, &sensor_custom_ctrls[i], NULL);

	if (handler->error) {
		ret = handler->error;
		v4l2_ctrl_handler_free(handler);
	}

	sd->ctrl_handler = handler;

	return ret;
}

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);
	sensor_init_controls(sd, &sensor_ctrl_ops);
	mutex_init(&info->lock);

#ifdef CONFIG_SAME_I2C
	info->sensor_i2c_addr = I2C_ADDR >> 1;
#endif
	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	info->af_first_flag = 1;

	return 0;
}
static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;

	sd = cci_dev_remove_helper(client, &cci_drv);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{SENSOR_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sensor_id);

static struct i2c_driver sensor_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = SENSOR_NAME,
		   },
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};
static __init int init_sensor(void)
{
	return cci_dev_init_helper(&sensor_driver);
}

static __exit void exit_sensor(void)
{
	cci_dev_exit_helper(&sensor_driver);
}

module_init(init_sensor);
module_exit(exit_sensor);
