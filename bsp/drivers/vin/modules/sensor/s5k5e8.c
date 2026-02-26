/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for s5k5e8_mipi cameras.
 *
 * Copyright (c) 2018 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Cao FuMing <caofuming@allwinnertech.com>
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

MODULE_AUTHOR("cfm");
MODULE_DESCRIPTION("A low-level driver for s5k5e8 sensors");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR 0x5e80

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 15

/*
 * The s5k5e8_mipi sits on i2c with ID 0x20
 */
#define I2C_ADDR 0x30

#define SENSOR_NUM    0x2
#define SENSOR_NAME   "s5k5e8"
#define SENSOR_NAME_2 "s5k5e8_2"

struct cfg_array { /* coming later */
	struct regval_list *regs;
	int size;
};

/*
 * The default register settings
 *
 */

static struct regval_list sensor_default_regs[] = {

};

/* 2592*1944 */
static struct regval_list sensor_2592_1944_regs[] = {
	{0x0100, 0x00}, // stream off
	{0x3906, 0x7E}, // global setting
	{0x0101, 0x00}, //[1:0] mirror and vertical flip
	{0x3C01, 0x0F}, {0x3C14, 0x00}, {0x3235, 0x08}, {0x3063, 0x2E},
	{0x307A, 0x10}, {0x307B, 0x0E}, {0x3079, 0x20}, {0x3070, 0x05},
	{0x3067, 0x06}, {0x3071, 0x62}, {0x3203, 0x43}, {0x3205, 0x43},
	{0x320B, 0x42}, {0x323B, 0x02}, {0x3007, 0x00}, {0x3008, 0x14},
	{0x3020, 0x58}, {0x300D, 0x34}, {0x300E, 0x17}, {0x3021, 0x02},
	{0x3010, 0x59}, {0x3002, 0x01}, {0x3005, 0x01}, {0x3008, 0x04},
	{0x300F, 0x70}, {0x3010, 0x69}, {0x3017, 0x10}, {0x3019, 0x19},
	{0x300C, 0x62}, {0x3064, 0x10}, {0x3C08, 0x0E}, {0x3C09, 0x10},
	{0x3C31, 0x0D}, {0x3C32, 0xAC}, {0x3929, 0x07}, {0x3C02, 0x0F},

	{0x0103, 0x01}, {0x0100, 0x00}, // stream off
	{0x0101, 0x00}, //[1:0] mirror and vertical flip
	{0x3906, 0x7E},                 // global setting
	{0x3C01, 0x0F}, {0x3C14, 0x00}, {0x3235, 0x08}, {0x3063, 0x2E},
	{0x307A, 0x10}, {0x307B, 0x0E}, {0x3079, 0x20}, {0x3070, 0x05},
	{0x3067, 0x06}, {0x3071, 0x62}, {0x3203, 0x43}, {0x3205, 0x43},
	{0x320B, 0x42}, {0x323B, 0x02}, {0x3007, 0x00}, {0x3008, 0x14},
	{0x3020, 0x58}, {0x300D, 0x34}, {0x300E, 0x17}, {0x3021, 0x02},
	{0x3010, 0x59}, {0x3002, 0x01}, {0x3005, 0x01}, {0x3008, 0x04},
	{0x300F, 0x70}, {0x3010, 0x69}, {0x3017, 0x10}, {0x3019, 0x19},
	{0x300C, 0x62}, {0x3064, 0x10}, {0x3C08, 0x0E}, {0x3C09, 0x10},
	{0x3C31, 0x0D}, {0x3C32, 0xAC}, {0x3929, 0x07}, {0x3C02, 0x0F},

	{0x0100, 0x00}, // stream off
	{0x0136, 0x18}, // Capture_setting
	{0x0137, 0x00}, {0x0305, 0x06}, {0x0306, 0x18}, {0x0307, 0xA8},
	{0x0308, 0x34}, {0x0309, 0x42}, {0x3C1F, 0x00}, {0x3C17, 0x00},
	{0x3C0B, 0x04}, {0x3C1C, 0x47}, {0x3C1D, 0x15}, {0x3C14, 0x04},
	{0x3C16, 0x00}, {0x0820, 0x03}, {0x0821, 0x44}, {0x0114, 0x01},
	{0x0344, 0x00}, {0x0345, 0x08}, {0x0346, 0x00}, {0x0347, 0x08},
	{0x0348, 0x0A}, {0x0349, 0x27}, {0x034A, 0x07}, {0x034B, 0x9F},
	{0x034C, 0x0A}, {0x034D, 0x20}, {0x034E, 0x07}, {0x034F, 0x98}, // x, y
	{0x0900, 0x00}, {0x0901, 0x00}, {0x0381, 0x01}, {0x0383, 0x01},
	{0x0385, 0x01}, {0x0387, 0x01}, {0x0340, 0x07}, {0x0341, 0xB0},
	{0x0342, 0x0B}, {0x0343, 0x28}, {0x0200, 0x00}, {0x0201, 0x00},
	{0x0100, 0x01},	{0x0202, 0x07}, {0x0203, 0x9e}, {0x0204, 0x03},
	{0x0205, 0x00}, {0x0100, 0x00},
	{0x3303, 0x02}, {0x3400, 0x01}, {0x323b, 0x02}, {0x3301, 0x00},
	{0x3321, 0x04}, {0x3306, 0x00}, {0x3307, 0x08}, {0x3308, 0x0A},
	{0x3309, 0x27}, {0x330A, 0x01}, {0x330B, 0x01}, {0x330E, 0x00},
	{0x330F, 0x08}, {0x3310, 0x07}, {0x3311, 0x9F}, {0x3312, 0x01},
	{0x3313, 0x01},
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
	data_type explow, exphigh;
	struct sensor_info *info = to_state(sd);

	exphigh = (unsigned char)((exp_val >> 12) & 0xff);
	explow  = (unsigned char)((exp_val >> 4)  & 0xff);

	sensor_write(sd, 0x0203, explow);/* coarse integration time */
	sensor_write(sd, 0x0202, exphigh);

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
	data_type gainlow = 0;
	data_type gainhigh = 0;

	gain_val = gain_val * 2;
	gainlow = (unsigned char)(gain_val & 0xff);
	gainhigh = (unsigned char)((gain_val >> 8) & 0xff);

	sensor_write(sd, 0x0205, gainlow);
	sensor_write(sd, 0x0204, gainhigh);

	sensor_dbg("sensor_s_gain info->gain %d\n", gain_val);
	info->gain = gain_val / 2;
	return 0;
}

static int s5k5e8_sensor_vts;
static int frame_length = 1968;
static int sensor_s_exp_gain(struct v4l2_subdev *sd,
				struct sensor_exp_gain *exp_gain)
{
	int exp_val = 0, gain_val = 0, shutter = 0;
	//struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	shutter = exp_val >> 4;
	if (shutter > s5k5e8_sensor_vts - 5)
		frame_length = shutter + 5;
	else
		frame_length = s5k5e8_sensor_vts;

	sensor_write(sd, 0x0104, 0x01);
	sensor_write(sd, 0x0341, frame_length & 0xff);
	sensor_write(sd, 0x0340, frame_length >> 8);
	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);
	sensor_write(sd, 0x0104, 0x00);

	sensor_dbg("sensor_set_gain exp = %d, %d Done!\n", gain_val, exp_val);

	// info->exp = exp_val;
	// info->gain = gain_val;
	return 0;
}

static int sensor_s_fps(struct v4l2_subdev *sd,
			struct sensor_fps *fps)
{
	struct sensor_info *info = to_state(sd);
	struct sensor_win_size *wsize = info->current_wins;

	s5k5e8_sensor_vts = wsize->pclk/fps->fps/wsize->hts;
	sensor_write(sd, 0x0104, 0x01);
	sensor_write(sd, 0x0341, s5k5e8_sensor_vts & 0xff);
	sensor_write(sd, 0x0340, s5k5e8_sensor_vts >> 8);
	sensor_write(sd, 0x0104, 0x00);

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
		ret = sensor_s_sw_stby(sd, 1);
		if (ret < 0)
			sensor_err("soft stby falied!\n");
		usleep_range(10000, 12000);

		cci_lock(sd);
		/* standby on io */
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		cci_unlock(sd);
		/* inactive mclk after stadby in */
		vin_set_mclk(sd, OFF);
		break;
	case STBY_OFF:
		cci_lock(sd);

		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);

		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);

		cci_unlock(sd);
		ret = sensor_s_sw_stby(sd, 0);
		if (ret < 0)
			sensor_err("soft stby off falied!\n");
		usleep_range(10000, 12000);

		break;
	case PWR_ON:
		sensor_dbg("PWR_ON!\n");

		cci_lock(sd);
		vin_set_pmu_channel(sd, CAMERAVDD, ON);

		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);

		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);

		vin_set_mclk_freq(sd, MCLK);
		usleep_range(1000, 1200);
		vin_set_mclk(sd, ON);
		usleep_range(100, 120);

		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, AFVDD, ON);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(1000, 1200);

		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(1000, 1200);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");
		cci_lock(sd);

		vin_set_mclk(sd, OFF);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);

		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		vin_set_pmu_channel(sd, AFVDD, OFF);
		vin_set_pmu_channel(sd, CAMERAVDD, OFF);

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
#if !defined CONFIG_VIN_INIT_MELIS
	data_type rdval = 0xff;
	int ret;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	__maybe_unused struct sensor_info *info;

	ret = sensor_read(sd, 0x0000, &rdval);
	sensor_dbg("20x0000 0x%x\n", rdval);
	if (rdval != (V4L2_IDENT_SENSOR >> 8) && (ret < 0 || rdval != 0x0)) {
#if IS_ENABLED(CONFIG_SAME_I2C)
		info = to_state(sd);
		sensor_err(" read 0x0000 return 0x%x, sensor i2c_addr(info->sensor_i2c_addr) = 0x%x\n", rdval, info->sensor_i2c_addr);
		info->sensor_i2c_addr = 0x20 >> 1;
#else
		sensor_err(" read 0x0000 return 0x%x, sensor i2c_addr = 0x%x\n", rdval, client->addr);
		client->addr = 0x20 >> 1;
#endif
		sd = i2c_get_clientdata(client);
		ret = sensor_read(sd, 0x0000, &rdval);
		sensor_dbg("20x0000 0x%x\n", rdval);
	}

	if (rdval != (V4L2_IDENT_SENSOR >> 8) && (ret < 0 || rdval != 0x0)) {
		sensor_err(" read 0x0000 return 0x%x\n", rdval);
		return -ENODEV;
	}

	ret = sensor_read(sd, 0x0001, &rdval);
	sensor_dbg("0x0001 0x%x\n", rdval);
	if (rdval != (V4L2_IDENT_SENSOR & 0xff) && (ret < 0 || rdval != 0x0)) {
		sensor_err(" read 0x0001 return 0x%x\n", rdval);
		return -ENODEV;
	}
#endif
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
	info->width = HD720_WIDTH;
	info->height = HD720_HEIGHT;
	info->hflip = 0;
	info->vflip = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = SENSOR_FRAME_RATE; /* 15fps */

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
			       sizeof(struct sensor_win_size));
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
		.mbus_code = MEDIA_BUS_FMT_SGRBG10_1X10,
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
		.hts        = 2856,
		.vts        = 1968,
		.pclk       = 170 * 1000 * 1000,
		.mipi_bps   = 836 * 1000 * 1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 3 << 4,
		.intg_max   = (1968-5) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 16 << 4,
		.regs       = sensor_2592_1944_regs,
		.regs_size  = ARRAY_SIZE(sensor_2592_1944_regs),
		.set_size   = NULL,
	},
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				struct v4l2_mbus_config *cfg)
{
/*	struct sensor_info *info = to_state(sd); */

	cfg->type = V4L2_MBUS_CSI2_DPHY;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	cfg->bus.mipi_csi2.num_data_lanes = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
#else
	cfg->flags = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
#endif
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
	__maybe_unused struct sensor_exp_gain exp_gain;

	ret = sensor_write_array(sd, sensor_default_regs,
				 ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	sensor_dbg("sensor_reg_init\n");

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);
#if IS_ENABLED(CONFIG_VIN_INIT_MELIS)
	if (info->preview_first_flag) {
		info->preview_first_flag = 0;
	} else {
		if (wsize->regs)
			sensor_write_array(sd, wsize->regs, wsize->regs_size);
		if (wsize->wdr_mode == ISP_DOL_WDR_MODE) {
			exp_gain.exp_val = 30720;
			exp_gain.gain_val = 16;
		} else {
			exp_gain.exp_val = 15408;
			exp_gain.gain_val = 32;
		}
		sensor_s_exp_gain(sd, &exp_gain);
	}
#else
	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);
#endif
	if (wsize->set_size)
		wsize->set_size(sd);

	info->width = wsize->width;
	info->height = wsize->height;
	s5k5e8_sensor_vts = wsize->vts;

	sensor_write(sd, 0x0104, 0x01);
	sensor_write(sd, 0x0341, frame_length & 0xff);
	sensor_write(sd, 0x0340, frame_length >> 8);
	sensor_s_exp(sd, info->exp);
	sensor_s_gain(sd, info->gain);
	sensor_write(sd, 0x0104, 0x00);
	sensor_write(sd, 0x0100, 0x01);

	sensor_dbg("s_fmt set width = %d, height = %d\n", wsize->width,
	       wsize->height);

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);

	sensor_dbg("%s on = %d, %d*%d fps: %d code: %x\n", __func__, enable,
	       info->current_wins->width, info->current_wins->height,
	       info->current_wins->fps_fixed, info->fmt->mbus_code);

	if (!enable)
		return 0;

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

static struct cci_driver cci_drv[] = {
	{
		.name = SENSOR_NAME,
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_8,
	}, {
		.name = SENSOR_NAME_2,
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_8,
	}
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

	v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 32, 16 * 32, 1, 32);
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
	int i = 0;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
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
	info->exp = 1950 * 16;
	info->gain = 768;
	info->stable_frame_cnt = 0;
	info->sdram_dfs_flag = SENSOR_ALREADY_DEBUG;
	info->preview_first_flag = 1;

	return 0;
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
static void sensor_remove(struct i2c_client *client)
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
}
#else
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
#endif
static const struct i2c_device_id sensor_id_1[] = {
	{SENSOR_NAME, 0},
	{}
};

static const struct i2c_device_id sensor_id_2[] = {
	{SENSOR_NAME_2, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sensor_id_1);
MODULE_DEVICE_TABLE(i2c, sensor_id_2);

static struct i2c_driver sensor_driver[] = {
	{
		.driver = {
			   .owner = THIS_MODULE,
			   .name = SENSOR_NAME,
			   },
		.probe = sensor_probe,
		.remove = sensor_remove,
		.id_table = sensor_id_1,
	},
	{
		.driver = {
			   .owner = THIS_MODULE,
			   .name = SENSOR_NAME_2,
			   },
		.probe = sensor_probe,
		.remove = sensor_remove,
		.id_table = sensor_id_2,
	},
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