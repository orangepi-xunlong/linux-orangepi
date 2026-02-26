/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for ov5675_mipi cameras.
 *
 * Copyright (c) 2023 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Liu Chensheng <liuchensheng@allwinnertech.com>
 *    Liang WeiJie <liangweijie@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "../../utility/vin_log.h"
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

MODULE_AUTHOR("lcs");
MODULE_DESCRIPTION("A low-level driver for ov5675 sensors");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR 0x5675

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The ov5675_mipi sits on i2c with ID 0x6c or 0x20
 */
#define I2C_ADDR 0x6c //0x20

#define SENSOR_NUM 0x2
#define SENSOR_NAME "ov5675_mipi"
#define SENSOR_NAME_2 "ov5675_mipi_b"

/*
 * The default register settings
 *
 */
static struct regval_list sensor_default_regs[] = {

};

static struct regval_list sensor_2592_1944_30_regs[] = {
	{0x0100, 0x00}, {0x0103, 0x01}, {0x0300, 0x05}, {0x0302, 0x97},
	{0x0303, 0x00}, {0x030d, 0x1e}, {0x3002, 0x21}, {0x3107, 0x01},
	// {0x3501, 0x20},
	{0x3503, 0x0c},
	//{0x3508, 0x03}, {0x3509, 0x00},
	{0x3600, 0x66}, {0x3602, 0x30}, {0x3610, 0xa5}, {0x3612, 0x93},
	{0x3620, 0x80}, {0x3642, 0x0e}, {0x3661, 0x00}, {0x3662, 0x10},
	{0x3664, 0xf3}, {0x3665, 0x9e}, {0x3667, 0xa5}, {0x366e, 0x55},
	{0x366f, 0x55}, {0x3670, 0x11}, {0x3671, 0x11}, {0x3672, 0x11},
	{0x3673, 0x11}, {0x3714, 0x24}, {0x371a, 0x3e}, {0x3733, 0x10},
	{0x3734, 0x00}, {0x373d, 0x24}, {0x3764, 0x20}, {0x3765, 0x20},
	{0x3766, 0x12}, {0x37a1, 0x14}, {0x37a8, 0x1c}, {0x37ab, 0x0f},
	{0x37c2, 0x04}, {0x37cb, 0x00}, {0x37cc, 0x00}, {0x37cd, 0x00},
	{0x37ce, 0x00}, {0x37d8, 0x02}, {0x37d9, 0x08}, {0x37dc, 0x04},
	{0x3800, 0x00}, {0x3801, 0x00}, {0x3802, 0x00}, {0x3803, 0x04},
	{0x3804, 0x0a}, {0x3805, 0x3f}, {0x3806, 0x07}, {0x3807, 0xb3},
	{0x3808, 0x0a}, {0x3809, 0x20}, {0x380a, 0x07}, {0x380b, 0x98},
	{0x380c, 0x02}, {0x380d, 0xee}, {0x380e, 0x07}, {0x380f, 0xd0},
	{0x3811, 0x10}, {0x3813, 0x0c}, {0x3814, 0x01}, {0x3815, 0x01},
	{0x3816, 0x01}, {0x3817, 0x01}, {0x381e, 0x02}, {0x3820, 0x88},
	{0x3821, 0x01}, {0x3832, 0x04}, {0x3c80, 0x08}, {0x3c82, 0x00},
	{0x3c83, 0xb1}, {0x3c8c, 0x10}, {0x3c8d, 0x00}, {0x3c90, 0x00},
	{0x3c91, 0x00}, {0x3c92, 0x00}, {0x3c93, 0x00}, {0x3c94, 0x00},
	{0x3c95, 0x00}, {0x3c96, 0x00}, {0x3c97, 0x00}, {0x4001, 0xe0},
	{0x4008, 0x02}, {0x4009, 0x0d}, {0x400f, 0x80}, {0x4013, 0x02},
	{0x4040, 0x00}, {0x4041, 0x07}, {0x404c, 0x50}, {0x404e, 0x20},
	{0x4500, 0x06}, {0x4503, 0x00}, {0x450a, 0x04}, {0x4809, 0x04},
	{0x480c, 0x12}, {0x4819, 0x70}, {0x4825, 0x32}, {0x4826, 0x32},
	{0x482a, 0x06}, {0x4833, 0x08}, {0x4837, 0x0d}, {0x5000, 0x77},
	{0x5b00, 0x01}, {0x5b01, 0x10}, {0x5b02, 0x01}, {0x5b03, 0xdb},
	{0x5b05, 0x6c}, {0x5e10, 0xfc},
	{0x3500, 0x00}, {0x3501, 0x3F}, {0x3502, 0xF0},
	{0x3503, 0x78},
	{0x3508, 0x04}, {0x3509, 0x00},
	{0x3832, 0x48}, {0x3c90, 0x00}, {0x5780, 0x3e}, {0x5781, 0x0f},
	{0x5782, 0x44}, {0x5783, 0x02}, {0x5784, 0x01}, {0x5785, 0x01},
	{0x5786, 0x00}, {0x5787, 0x04}, {0x5788, 0x02}, {0x5789, 0x0f},
	{0x578a, 0xfd}, {0x578b, 0xf5}, {0x578c, 0xf5}, {0x578d, 0x03},
	{0x578e, 0x08}, {0x578f, 0x0c}, {0x5790, 0x08}, {0x5791, 0x06},
	{0x5792, 0x00}, {0x5793, 0x52}, {0x5794, 0xa3}, {0x4003, 0x40},
	{0x3d8c, 0x71}, {0x3d8d, 0xE7}, {0x37cb, 0x09}, {0x37cc, 0x15},
	{0x37cd, 0x1f}, {0x37ce, 0x1f},
	{0x030e, 0x05}, {0x3016, 0x32}, {0x3106, 0x15},
	// {0x3501, 0x3e},
	{0x3662, 0x10}, {0x3714, 0x24}, {0x371a, 0x3e},
	{0x37c2, 0x04}, {0x37d9, 0x08}, {0x3803, 0x04}, {0x3807, 0xb3},
	{0x3808, 0x0a}, {0x3809, 0x20}, {0x380a, 0x07}, {0x380b, 0x98},
	{0x380c, 0x02}, {0x380d, 0xee}, {0x380e, 0x07}, {0x380f, 0xd0},
	{0x3811, 0x10}, {0x3813, 0x0c}, {0x3814, 0x01}, {0x3816, 0x01},
#if VIN_FALSE
	{0x3820, 0x88}, {0x373d, 0x24},
#else
	{0x3820, 0x80}, {0x373d, 0x26},
#endif
	{0x3821, 0x01}, {0x4008, 0x02}, {0x4009, 0x0d}, {0x4041, 0x07},


	//{0x0100, 0x01},
};

/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */
static struct regval_list sensor_fmt_raw[] = {

};

/*
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function ,retrun -EINVAL
 */

static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->exp;
	sensor_dbg("sensor_get_exposure = %d\n", info->exp);

	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	data_type explow, expmid, exphigh;
	struct sensor_info *info = to_state(sd);

	exphigh = (unsigned char)((exp_val >> 17) & 0x0F);
	expmid = (unsigned char)((exp_val >> 9) & 0xFF);
	explow = (unsigned char)((exp_val >> 1)& 0xFF);

	sensor_write(sd, 0x3500, exphigh);
	sensor_write(sd, 0x3501, expmid);
	sensor_write(sd, 0x3502, explow);

	sensor_dbg("sensor_s_exp info->exp %d\n", exp_val);
	info->exp = exp_val;

	return 0;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->gain;
	sensor_dbg("sensor_get_gain = %d\n", info->gain);

	return 0;
}

static int sensor_s_gain(struct v4l2_subdev *sd, unsigned int gain_val)
{
	struct sensor_info *info = to_state(sd);
	data_type gainlow, gainhigh = 0;
	gain_val = gain_val << 3;
	gainlow = (unsigned char)(gain_val& 0xff);
	gainhigh = (unsigned char)((gain_val >> 8) & 0x0f);

	sensor_write(sd, 0x3508, gainhigh);
	sensor_write(sd, 0x3509, gainlow);

	sensor_dbg("sensor_s_gain info->gain %d\n", gain_val);
	info->gain = gain_val >> 3;

	return 0;
}

static int ov5675_sensor_vts;
static int frame_length = 2000;
static int sensor_s_exp_gain(struct v4l2_subdev *sd,
				struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val, shutter;

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	shutter = exp_val >> 4;
	if (shutter > ov5675_sensor_vts - 4)
		frame_length = shutter + 4;
	else
		frame_length = ov5675_sensor_vts;
	sensor_write(sd, 0x3208, 0x00);/* enter group write */
	sensor_write(sd, 0x380f, frame_length & 0xff);
	sensor_write(sd, 0x380e, frame_length >> 8);
	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);
	sensor_write(sd, 0x3208, 0x10);/* end group write */
	sensor_write(sd, 0x3208, 0xa0);/* init group write */

	return 0;
}

static int sensor_s_fps(struct v4l2_subdev *sd,
			struct sensor_fps *fps)
{
	struct sensor_info *info = to_state(sd);
	struct sensor_win_size *wsize = info->current_wins;

	ov5675_sensor_vts = wsize->pclk/fps->fps/wsize->hts;
	sensor_write(sd, 0x3208, 0x00);/* enter group write */
	sensor_write(sd, 0x380f, ov5675_sensor_vts & 0xff);
	sensor_write(sd, 0x380e, ov5675_sensor_vts >> 8);
	sensor_write(sd, 0x3208, 0x10);/* end group write */
	sensor_write(sd, 0x3208, 0xa0);/* init group write */
	return 0;
}

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	int ret;
	data_type rdval;

	ret = sensor_read(sd, 0x0100, &rdval);

	if (ret != 0)
		return ret;

	if (on_off == STBY_ON)
		ret = sensor_write(sd, 0x0100, rdval & 0xfe);
	else
		ret = sensor_write(sd, 0x0100, rdval | 0x01);

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
		ret = sensor_s_sw_stby(sd, STBY_ON);
		if (ret < 0)
			sensor_err("soft stby falied!\n");
		usleep_range(10000, 12000);

		cci_lock(sd);
		/* inactive mclk after stadby in */
		vin_set_mclk(sd, OFF);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		cci_lock(sd);

		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);

		cci_unlock(sd);
		ret = sensor_s_sw_stby(sd, STBY_OFF);
		if (ret < 0)
			sensor_err("soft stby off falied!\n");
		usleep_range(10000, 12000);

		break;
	case PWR_ON:
		sensor_dbg("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_set_pmu_vol(sd, DVDD, VDD_1200MV);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		vin_set_pmu_channel(sd, CAMERAVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(100, 120);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(1000, 1200);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");
		cci_lock(sd);
		vin_set_mclk(sd, OFF);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		vin_set_pmu_channel(sd, CAMERAVDD, OFF);
		cci_unlock(sd);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	sensor_print("%s val %d\n", __func__, val);

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
	data_type rdval = 0;
	int ret;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	sensor_read(sd, 0x300B, &rdval);
	sensor_dbg("0x300B 0x%x\n", rdval);
	if (rdval != (V4L2_IDENT_SENSOR >> 8)) {
		sensor_err(" read 0x300B return 0x%x, sensor i2c_addr = 0x%x\n", rdval, client->addr);
		client->addr = 0x20 >> 1;
		sd = i2c_get_clientdata(client);
		ret = sensor_read(sd, 0x300B, &rdval);
		sensor_dbg("0x300B 0x%x\n", rdval);
	}

	if (rdval != (V4L2_IDENT_SENSOR >> 8)) {
		sensor_err(" read 0x300B return 0x%x\n", rdval);
		return -ENODEV;
	}

	sensor_read(sd, 0x300C, &rdval);
	sensor_dbg("0x300C 0x%x\n", rdval);
	if (rdval != (V4L2_IDENT_SENSOR & 0xff)) {
		sensor_err(" read 0x300C return 0x%x\n", rdval);
		return -ENODEV;
	}

	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_init\n");

	/* Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = 2592;
	info->height = 1944;
	info->hflip = 0;
	info->vflip = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = SENSOR_FRAME_RATE; /* 30 fps */
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
			memcpy(arg, info->current_wins,
			       sizeof(*info->current_wins));
			ret = 0;
		} else {
			sensor_err("empty wins!\n");
			ret = -1;
		}
		break;
	case SET_FPS:
		ret = 0;
		break;
	case VIDIOC_VIN_SENSOR_EXP_GAIN:
		ret = sensor_s_exp_gain(sd, (struct sensor_exp_gain *)arg);
		break;
	case VIDIOC_VIN_SENSOR_CFG_REQ:
		sensor_cfg_req(sd, (struct sensor_config *)arg);
		break;
	case VIDIOC_VIN_SENSOR_SET_FPS:
		ret = sensor_s_fps(sd, (struct sensor_fps *)arg);
		break;
	case VIDIOC_VIN_ACT_INIT:
		ret = actuator_init(sd, (struct actuator_para *)arg);
		break;
	case VIDIOC_VIN_ACT_SET_CODE:
		ret = actuator_set_code(sd, (struct actuator_ctrl *)arg);
		break;
	case VIDIOC_VIN_FLASH_EN:
		ret = flash_en(sd, (struct flash_para *)arg);
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
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
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

	{
		.width      = 2592,
		.height     = 1944,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 750,
		.vts        = 2000,
		.pclk       = 45 * 1000 * 1000,
		.mipi_bps   = 906 * 1000 * 1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 4 << 4,
		.intg_max   = (2000 - 4) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 16 << 4,
		.regs       = sensor_2592_1944_30_regs,
		.regs_size  = ARRAY_SIZE(sensor_2592_1944_30_regs),
		.set_size   = NULL,
	},

};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_CSI2_DPHY;
	cfg->flags = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0;

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

static int sensor_reg_init(struct sensor_info *info)
{
	int ret;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	ret = sensor_write_array(sd, sensor_default_regs,
				 ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	sensor_dbg("sensor_reg_init\n");

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);

	if (wsize->set_size)
		wsize->set_size(sd);

	info->width = wsize->width;
	info->height = wsize->height;
	ov5675_sensor_vts = wsize->vts;

	sensor_write(sd, 0x380f, frame_length & 0xff);
	sensor_write(sd, 0x380e, frame_length >> 8);
	sensor_s_exp(sd, info->exp);
	sensor_s_gain(sd, info->gain);
	sensor_write(sd, 0x0100, 0x01);

	sensor_print("s_fmt set width = %d, height = %d\n", wsize->width,
		     wsize->height);

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);

	sensor_print("%s on = %d, %d*%d fps: %d code: %x\n", __func__, enable,
		     info->current_wins->width, info->current_wins->height,
		     info->current_wins->fps_fixed, info->fmt->mbus_code);

	if (!enable) {
		/* stream off */
		sensor_write(sd, 0x0100, 0x00);
		return 0;
	}

	return sensor_reg_init(info);
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
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl32 = sensor_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_stream = sensor_s_stream,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.enum_frame_interval = sensor_enum_frame_interval,
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
	.get_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.pad = &sensor_pad_ops,
};

/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv[] = {
	{
		.name = SENSOR_NAME,
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_8,
	}, {
		.name = SENSOR_NAME_2,
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_8,
	},
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
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 2);

	v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 16, 256 * 16, 1, 16);
	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 3 * 16, 65536 * 16, 1, 3 * 16);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	if (handler->error) {
		ret = handler->error;
		v4l2_ctrl_handler_free(handler);
	}

	sd->ctrl_handler = handler;

	return ret;
}

static int sensor_dev_id;
static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	int i;

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;

	if (client) {
		for (i = 0; i < SENSOR_NUM; i++) {
			if (!strcmp(cci_drv[i].name, client->name))
				break;
		}
		cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv[i]);
	} else {
		cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv[sensor_dev_id++]);
	}
	sensor_init_controls(sd, &sensor_ctrl_ops);

	mutex_init(&info->lock);
#if IS_ENABLED(CONFIG_SAME_I2C)
	info->sensor_i2c_addr = I2C_ADDR >> 1;
#endif
	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->af_first_flag = 1;
	info->exp = 16368;
	info->gain = 128;

	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;
	int i;

	if (client) {
		for (i = 0; i < SENSOR_NUM; i++) {
			if (!strcmp(cci_drv[i].name, client->name))
				break;
		}
		sd = cci_dev_remove_helper(client, &cci_drv[i]);
	} else {
		sd = cci_dev_remove_helper(client, &cci_drv[sensor_dev_id++]);
	}

	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{SENSOR_NAME, 0},
	{}
};

static const struct i2c_device_id sensor_id_2[] = {
	{SENSOR_NAME_2, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sensor_id);
MODULE_DEVICE_TABLE(i2c, sensor_id_2);

static struct i2c_driver sensor_driver[] = {
	{
		.driver = {
			   .owner = THIS_MODULE,
			   .name = SENSOR_NAME,
			   },
		.probe = sensor_probe,
		.remove = sensor_remove,
		.id_table = sensor_id,
	}, {
		.driver = {
			   .owner = THIS_MODULE,
			   .name = SENSOR_NAME_2,
			   },
		.probe = sensor_probe,
		.remove = sensor_remove,
		.id_table = sensor_id_2,
	}
};
static __init int init_sensor(void)
{
	int i, ret = 0;

	sensor_dev_id = 0;

	for (i = 0; i < SENSOR_NUM; i++)
		ret = cci_dev_init_helper(&sensor_driver[i]);

	return ret;
}

static __exit void exit_sensor(void)
{
	int i;

	sensor_dev_id = 0;

	for (i = 0; i < SENSOR_NUM; i++)
		cci_dev_exit_helper(&sensor_driver[i]);
}

VIN_INIT_DRIVERS(init_sensor);
module_exit(exit_sensor);
