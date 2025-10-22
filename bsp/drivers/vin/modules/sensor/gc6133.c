/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for gc6133_mipi cameras.
 *
 * Copyright (c) 2022 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Liu Chensheng <liuchensheng@allwinnertech.com>
 *    Liang WeiJie <liangweijie@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/* This is an spi sensor. This driver can only successfully
* communicate with i2c and generate video node. The register
* configuration is also the configuration of spi camera register .*/
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
MODULE_DESCRIPTION("A low-level driver for gc6133_mipi sensors");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

#define MCLK              (24 * 1000 * 1000)
#define V4L2_IDENT_SENSOR 0xba

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 30

/*
 * The gc6133 i2c address
 */
#define I2C_ADDR 0x80

#define SENSOR_NAME "gc6133"

/*
 * The default register settings
 */
static struct regval_list sensor_default_regs[] = {
	{0xfe, 0xa0},
	{0xfe, 0xa0},
	{0xfe, 0xa0},
	{0xf6, 0x00},
	{0xfa, 0x11},
	{0xfc, 0x12},
	{0xfe, 0x00},
	{0x49, 0x70},
	{0x4a, 0x40},
	{0x4b, 0x5d},
	{0x03, 0x00},
	{0x04, 0xfa},
	{0x01, 0x41},
	{0x02, 0x12},
	{0x0f, 0x01},
	{0x0d, 0x30},
	{0x12, 0xc8},
	{0x14, 0x54},
	{0x15, 0x32},
	{0x16, 0x04},
	{0x17, 0x19},
	{0x1d, 0xb9},
	{0x1f, 0x15},
	{0x7a, 0x00},
	{0x7b, 0x14},
	{0x7d, 0x36},
	{0xfe, 0x10},
	{0x20, 0x7e},
	{0x22, 0xf8},//0xb8
	{0x24, 0x54},
	{0x26, 0x87},
	{0x39, 0x00},
	{0x3a, 0x80},
	{0x3b, 0x01},
	{0x3c, 0x40},
	{0x3e, 0xf0},
	{0x2a, 0x2f},
	{0x37, 0x46},
	{0x3f, 0x18},
	{0x50, 0x3c},
	{0x52, 0x4f},
	{0x53, 0x81},
	{0x54, 0x43},
	{0x56, 0x78},
	{0x57, 0xaa},//add
	{0x58, 0xff},//add
	{0x5b, 0x60},
	{0x5c, 0x80},
	{0xab, 0x28},
	{0xac, 0xb5},
	{0x60, 0x45},
	{0x62, 0x68},//add
	{0x63, 0x13},
	{0x64, 0x43},
	{0x65, 0x13},
	{0x66, 0x26},
	{0x67, 0x07},
	{0x68, 0xf5},
	{0x69, 0xea},
	{0x6a, 0x21},
	{0x6b, 0x21},
	{0x6c, 0xe4},
	{0x6d, 0xfb},
	{0x81, 0x30},
	{0x82, 0x30},
	{0x83, 0x4a},
	{0x85, 0x06},
	{0x8d, 0x78},
	{0x8e, 0x25},
	{0x90, 0x38},
	{0x92, 0x36},
	{0x9d, 0x32},
	{0x9e, 0x61},
	{0x9f, 0xf4},
	{0xa3, 0x28},
	{0xa4, 0x01},
	{0xb1, 0x1e},
	{0xb3, 0x20},
	{0xbd, 0x70},
	{0xbe, 0x58},
	{0xbf, 0xa0},
	{0x43, 0xa8},
	{0xb0, 0xf2},
	{0xb5, 0x40},
	{0xb8, 0x05},
	{0xba, 0x60},
	{0xfe, 0x02},
	{0x01, 0x01},
	{0x02, 0x02},
	{0x03, 0x20},
	{0x04, 0x20},
	{0x0a, 0x00},
	{0x13, 0x10},
	{0x24, 0x00},
	{0x28, 0x03},
	{0xfe, 0x00},
	{0x22, 0xf8},
	{0xf1, 0x03},
	{0xfe, 0x00},
	{0x1f, 0x33},
};

static struct regval_list sensor_320_240_30_raw[] = {
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
#if VIN_FALSE
static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->exp;
	sensor_dbg("sensor_get_exposure = %d\n", info->exp);

	return 0;
}

static int gc6133_sensor_vts;
static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	struct sensor_info *info = to_state(sd);
	data_type explow, exphigh;

	exp_val = exp_val >> 4;

	if (exp_val > (gc6133_sensor_vts - 16))
		exp_val = gc6133_sensor_vts - 16;
	if (exp_val < 16)
		exp_val = 16;

	exphigh = (unsigned char)((exp_val >> 8) & 0x7F);
	explow = (unsigned char)(exp_val & 0xFF);

	sensor_write(sd, 0xfe, 0x00);
	sensor_write(sd, 0x03, exphigh);
	sensor_write(sd, 0x04, explow);

	sensor_print("%s info->exp %d\n", __func__, exp_val);
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

#define SENSOR_BASE_GAIN       0x40
#define SENSOR_MAX_GAIN        (16 * SENSOR_BASE_GAIN)
#define MAX_AG_INDEX           9
#define AGC_REG_NUM            14
static int gain_level[MAX_AG_INDEX] = {
	0x0040, /* 1.000 */
	0x0058, /* 1.375 */
	0x007d, /* 1.950 */
	0x00ad, /* 2.700 */
	0x00f3, /* 3.800 */
	0x0159, /* 5.400 */
	0x01ea, /* 7.660 */
	0x02ac, /* 10.688 */
	0x03c2, /* 15.030 */
};
static unsigned char agc_register[MAX_AG_INDEX][AGC_REG_NUM] = {
	/* fullsize */
	{ 0x00, 0x55, 0x83, 0x01, 0x06, 0x18, 0x20, 0x16, 0x17, 0x50, 0x6c, 0x9b, 0xd8, 0x00 },
	{ 0x00, 0x55, 0x83, 0x01, 0x06, 0x18, 0x20, 0x16, 0x17, 0x50, 0x6c, 0x9b, 0xd8, 0x00 },
	{ 0x00, 0x4e, 0x84, 0x01, 0x0c, 0x2e, 0x2d, 0x15, 0x19, 0x47, 0x70, 0x9f, 0xd8, 0x00 },
	{ 0x00, 0x51, 0x80, 0x01, 0x07, 0x28, 0x32, 0x22, 0x20, 0x49, 0x70, 0x91, 0xd9, 0x00 },
	{ 0x00, 0x4d, 0x83, 0x01, 0x0f, 0x3b, 0x3b, 0x1c, 0x1f, 0x47, 0x6f, 0x9b, 0xd3, 0x00 },
	{ 0x00, 0x50, 0x83, 0x01, 0x08, 0x35, 0x46, 0x1e, 0x22, 0x4c, 0x70, 0x9a, 0xd2, 0x00 },
	{ 0x00, 0x52, 0x80, 0x01, 0x0c, 0x35, 0x3a, 0x2b, 0x2d, 0x4c, 0x67, 0x8d, 0xc0, 0x00 },
	{ 0x00, 0x52, 0x80, 0x01, 0x0c, 0x35, 0x3a, 0x2b, 0x2d, 0x4c, 0x67, 0x8d, 0xc0, 0x00 },
	{ 0x00, 0x52, 0x80, 0x01, 0x0c, 0x35, 0x3a, 0x2b, 0x2d, 0x4c, 0x67, 0x8d, 0xc0, 0x00 }
};
static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	struct sensor_info *info = to_state(sd);
	int gain_index = 0, temp_gain = 0;

	gain_val = gain_val * 4;

	if (gain_val < SENSOR_BASE_GAIN)
		gain_val = SENSOR_BASE_GAIN;
	if (gain_val > SENSOR_MAX_GAIN)
		gain_val = SENSOR_MAX_GAIN;

	for (gain_index = MAX_AG_INDEX - 1; gain_index >= 0; gain_index--) {
		if (gain_val >= gain_level[gain_index]) {
			/* analog gain */
			sensor_write(sd, 0xfe, 0x00);
			sensor_write(sd, 0xb6, gain_index);

			/* digital gain */
			temp_gain = 256 * gain_val / gain_level[gain_index];
			/* temp_gain = temp_gain * Dgain_ratio / 256; */
			sensor_write(sd, 0xb1, temp_gain >> 8);
			sensor_write(sd, 0xb2, temp_gain & 0xff);

			sensor_write(sd, 0xfe, agc_register[gain_index][0]);
			sensor_write(sd, 0x20, agc_register[gain_index][1]);
			sensor_write(sd, 0x33, agc_register[gain_index][2]);
			sensor_write(sd, 0xfe, agc_register[gain_index][3]);
			sensor_write(sd, 0xdf, agc_register[gain_index][4]);
			sensor_write(sd, 0xe7, agc_register[gain_index][5]);
			sensor_write(sd, 0xe8, agc_register[gain_index][6]);
			sensor_write(sd, 0xe9, agc_register[gain_index][7]);
			sensor_write(sd, 0xea, agc_register[gain_index][8]);
			sensor_write(sd, 0xeb, agc_register[gain_index][9]);
			sensor_write(sd, 0xec, agc_register[gain_index][10]);
			sensor_write(sd, 0xed, agc_register[gain_index][11]);
			sensor_write(sd, 0xee, agc_register[gain_index][12]);
			sensor_write(sd, 0xfe, agc_register[gain_index][13]);
		}
	}

	sensor_print("%s info->gain %d\n", __func__, gain_val);
	info->gain = gain_val;

	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	sensor_s_exp(sd, exp_gain->exp_val);
	sensor_s_gain(sd, exp_gain->gain_val);

	return 0;
}

static int sensor_s_fps(struct v4l2_subdev *sd,
			struct sensor_fps *fps)
{
	return 0;
}

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	int ret = 0;

	return ret;
}
#endif
/*
 * Stuff that knows about the sensor.
 */
static int sensor_power(struct v4l2_subdev *sd, int on)
{

	switch (on) {
#if VIN_FALSE
	case STBY_ON:
		sensor_dbg("STBY_ON!\n");
		cci_lock(sd);
		ret = sensor_s_sw_stby(sd, STBY_ON);
		if (ret < 0)
			sensor_err("soft stby falied!\n");
		usleep_range(1000, 1200);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		sensor_dbg("STBY_OFF!\n");
		cci_lock(sd);
		usleep_range(1000, 1200);
		ret = sensor_s_sw_stby(sd, STBY_OFF);
		if (ret < 0)
			sensor_err("soft stby off falied!\n");
		cci_unlock(sd);
		break;
#endif
	case PWR_ON:
		sensor_dbg("PWR_ON!\n");
		cci_lock(sd);

		//vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		//vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(1000, 1200);
//		udelay(10);
		// vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		// vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		// usleep_range(1000, 1200);

		// vin_set_pmu_channel(sd, CAMERAVDD, ON);

		// vin_set_pmu_channel(sd, IOVDD, ON);
		// usleep_range(1000, 1200);

		// vin_set_pmu_channel(sd, DVDD, ON);
		// usleep_range(1000, 1200);

		vin_set_pmu_channel(sd, AVDD, ON);
//		udelay(35);
//		udelay(10);
		usleep_range(3500, 3700);
		usleep_range(1000, 1200);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
//		udelay(10);
		usleep_range(1000, 1200);
		//vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
//		udelay(10);
		usleep_range(1000, 1200);
		//vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
//		udelay(10);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		//vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(10, 12);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		vin_set_mclk(sd, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		usleep_range(10, 12);

		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		// vin_set_pmu_channel(sd, CAMERAVDD, OFF);

		vin_gpio_set_status(sd, RESET, 0);
		vin_gpio_set_status(sd, PWDN, 0);
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
		usleep_range(100, 120);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(100, 120);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	data_type rdval = 0;
	//while (1) {
	sensor_read(sd, 0xf0, &rdval);
	if (rdval != 0xba) {
		sensor_err("read 0xf0 return value 0x%x\n", rdval);
		//return -ENODEV;
	}
	//}
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
	info->width = 320;
	info->height = 240;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->exp = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30;

	info->preview_first_flag = 1;

	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);

	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		if (info->current_wins) {
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
/*	case VIDIOC_VIN_SENSOR_EXP_GAIN:
		ret = sensor_s_exp_gain(sd, (struct sensor_exp_gain *)arg);
		break;
	case VIDIOC_VIN_SENSOR_SET_FPS:
		ret = sensor_s_fps(sd, (struct sensor_fps *)arg);
		break; */
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
	{
		.width      = 320,
		.height     = 240,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 3798,
		.vts        = 2464,
		.pclk       = 234 * 1000 * 1000,
		.mipi_bps   = 1171 * 1000 * 1000,
		.fps_fixed  = 25,
		.bin_factor = 1,
		.intg_min   = 16,
		.intg_max   = (2464 - 4) << 4,
		.gain_min   = 16,
		.gain_max   = (128 << 4),
		.regs       = sensor_320_240_30_raw,
		.regs_size  = ARRAY_SIZE(sensor_320_240_30_raw),
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
/*
static int sensor_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *info = container_of(ctrl->handler,
						struct sensor_info, handler);
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
	struct sensor_info *info = container_of(ctrl->handler,
						struct sensor_info, handler);
	struct v4l2_subdev *sd = &info->sd;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->val);
	case V4L2_CID_EXPOSURE:
		return sensor_s_exp(sd, ctrl->val);
	}

	return -EINVAL;
}
*/
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
	/* gc6133_sensor_vts = wsize->vts; */

	sensor_write(sd, 0xfe, 0x00);
	sensor_write(sd, 0x3f, 0x91);
	sensor_write(sd, 0xfe, 0x00);

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);

	sensor_dbg("%s on = %d, %d*%d fps: %d code: %x\n", __func__, enable,
		   info->current_wins->width, info->current_wins->height,
		   info->current_wins->fps_fixed, info->fmt->mbus_code);

	if (!enable) {
		sensor_write(sd, 0xfe, 0x00);
		sensor_write(sd, 0x3f, 0x00);
		sensor_write(sd, 0xfe, 0x00);

		return 0;
	}

	return sensor_reg_init(info);
}

/* ----------------------------------------------------------------------- */
/*
static const struct v4l2_ctrl_ops sensor_ctrl_ops = {
	.g_volatile_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
};
*/
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
static struct cci_driver cci_drv = {
	.name = SENSOR_NAME,
	.addr_width = CCI_BITS_8,
	.data_width = CCI_BITS_8,
};
/*
static int sensor_init_controls(struct v4l2_subdev *sd,
				const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	struct v4l2_ctrl *ctrl;
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 2);

	v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 16,
			  256 * 16, 1, 1 * 16);
	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 0,
				 65536 * 16, 1, 0);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	if (handler->error) {
		ret = handler->error;
		v4l2_ctrl_handler_free(handler);
	}

	sd->ctrl_handler = handler;

	return ret;
}
*/
static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd = NULL;
	struct sensor_info *info = NULL;

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	sd = &info->sd;

	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);

	/* sensor_init_controls(sd, &sensor_ctrl_ops); */

	mutex_init(&info->lock);

	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->af_first_flag = 1;
	info->exp = 0;
	info->gain = 0;

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

VIN_INIT_DRIVERS(init_sensor);
module_exit(exit_sensor);
