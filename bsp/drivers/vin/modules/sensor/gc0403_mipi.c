/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for gc0403_mipi cameras.
 *
 * Copyright (c) 2019 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zheng ZeQun <zequnzheng@allwinnertech.com>
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

MODULE_AUTHOR("lwj");
MODULE_DESCRIPTION("A low-level driver for gc0403 sensors");
MODULE_LICENSE("GPL");

#define MCLK              (24 * 1000 * 1000)
#define V4L2_IDENT_SENSOR 0x0403

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The gc0403_mipi sits on i2c with ID 0x78
 */
#define I2C_ADDR 0x78

#define SENSOR_NAME "gc0403_mipi"

/* Set Gain */
#define ANALOG_GAIN_1 64 /* 1.00x */
#define ANALOG_GAIN_2 90 /* 1.42x */
#define ANALOG_GAIN_3 160 /* 2.5x */
#define ANALOG_GAIN_4 226 /* 3.54x */
#define ANALOG_GAIN_5 314 /* 4.9x */
#define ANALOG_GAIN_6 442 /* 6.91x */
#define ANALOG_GAIN_7 620 /* 9.7x */
#define ANALOG_GAIN_8 872 /* 13.63x */
#define ANALOG_GAIN_9 1244 /* 19.45x */
#define ANALOG_GAIN_10 1730 /* 27.04x */
#define ANALOG_GAIN_11 2489 /* 38.89x */

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

/* 768x576 Raw10 30fps 24MHz */
static struct regval_list sensor_768_576_30fps_regs[] = {
	/****************************************************
	 *********************   SYS   **********************
	 ***************************************************/
	{0xfe, 0x80},
	{REG_DLY, 0x10},
	{0xf2, 0x00}, /* sync_pad_io_ebi */
	{0xf6, 0x00}, /* up down */
	{0xfc, 0xc6},
	{0xf7, 0x19}, /* pll enable */
	{0xf8, 0x01}, /* Pll mode 2 */
	{0xf9, 0x3e}, /* [0] pll enable */
	{0xfe, 0x03},
	{0x06, 0x80},
	{0x06, 0x00},
	{0xfe, 0x00},
	{0xf9, 0x2e},
	{0xfe, 0x00},
	{0xfa, 0x00}, /* div */
	{0xfe, 0x00},
	/****************************************************
	 ***************   ANALOG & CISCTL   ****************
	 ***************************************************/
	{0x03, 0x02},
	{0x04, 0x96},

	{0x05, 0x00},
	{0x06, 0xbb},
	{0x07, 0x00},
	{0x08, 0x46},

	{0x0c, 0x04},
	{0x0d, 0x02},
	{0x0e, 0x48},
	{0x0f, 0x03},
	{0x10, 0x08},
	{0x11, 0x23}, /* 44FPN */
	{0x12, 0x10},
	{0x13, 0x11},
	{0x14, 0x01},
	{0x15, 0x00},
	{0x16, 0xc0},
	{0x17, 0x14},
	{0x18, 0x02},
	{0x19, 0x38},
	{0x1a, 0x11},
	{0x1b, 0x06},
	{0x1c, 0x04},
	{0x1d, 0x00},
	{0x1e, 0xfc},
	{0x1f, 0x09},
	{0x20, 0xb5},
	{0x21, 0x3f},
	{0x22, 0xe6},
	{0x23, 0x32},
	{0x24, 0x2f},
	{0x27, 0x00},
	{0x28, 0x00},
	{0x2a, 0x00},
	{0x2b, 0x00},
	{0x2c, 0x00},
	{0x2d, 0x01},
	{0x2e, 0xf0},
	{0x2f, 0x01},
	{0x25, 0xc0},
	{0x3d, 0xd0},
	{0x3e, 0x45},
	{0x3f, 0x1f},
	{0xc2, 0x17},
	{0x30, 0x22},
	{0x31, 0x23},
	{0x32, 0x02},
	{0x33, 0x03},
	{0x34, 0x04},
	{0x35, 0x05},
	{0x36, 0x06},
	{0x37, 0x07},
	{0x38, 0x0f},
	{0x39, 0x17},
	{0x3a, 0x1f},
	/****************************************************
	 *********************   ISP   **********************
	 ***************************************************/
	{0xfe, 0x00},
	{0x8a, 0x00},
	{0x8c, 0x07},
	{0x8e, 0x02}, /* luma value not normal */
	{0x90, 0x01},
	{0x94, 0x02},
	{0x95, 0x02},
	{0x96, 0x40},
	{0x97, 0x03},
	{0x98, 0x00},
	/****************************************************
	 *********************   BLK   **********************
	 ***************************************************/
	{0xfe, 0x00},
	{0x18, 0x02},
	{0x40, 0x22},
	{0x41, 0x01},
	{0x5e, 0x00},
	{0x66, 0x20},
	/****************************************************
	 *********************   MIPI   *********************
	 ***************************************************/
	{0xfe, 0x03},
	{0x01, 0x83},
	{0x02, 0x11},
	{0x03, 0x96},
	{0x04, 0x01},
	{0x05, 0x00},
	{0x06, 0xa4},
	{0x10, 0x90},
	{0x11, 0x2b},
	{0x12, 0xc0},
	{0x13, 0x03},
	{0x15, 0x00},
	{0x21, 0x10},
	{0x22, 0x03},
	{0x23, 0x20},
	{0x24, 0x02},
	{0x25, 0x10},
	{0x26, 0x05},
	{0x21, 0x10},
	{0x29, 0x03},
	{0x2a, 0x0a},
	{0x2b, 0x04},
	{0xfe, 0x00},
	{0xb0, 0x50},
	{0xb6, 0x00},
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

static int gc0403_sensor_vts;
static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	data_type explow, exphigh;
	struct sensor_info *info = to_state(sd);

	if (exp_val > ((gc0403_sensor_vts - 4) << 4))
		exp_val = (gc0403_sensor_vts - 4) << 4;
	if (exp_val < 16)
		exp_val = 16;

	exp_val = exp_val >> 4;

	exphigh = (unsigned char)((exp_val >> 8) & 0x1F);
	explow = (unsigned char)(exp_val & 0xFF);

	sensor_write(sd, 0xfe, 0x00);
	sensor_write(sd, 0x03, exphigh);
	sensor_write(sd, 0x04, explow);

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
	unsigned int tmp;

	if (gain_val < 1 * 16)
		gain_val = 16;
	if (gain_val > 39 * 16 - 1)
		gain_val = 39 * 16 - 1;

	gain_val = gain_val * 4;

	sensor_write(sd, 0xfe, 0x00);
	sensor_write(sd, 0xb1, 0x01);
	sensor_write(sd, 0xb2, 0x00);

	if (gain_val < 0x40) {
		gain_val = 0x40;
	} else if ((gain_val >= ANALOG_GAIN_1) && (gain_val < ANALOG_GAIN_2)) {
		sensor_write(sd, 0xb6, 0x00);
		tmp = gain_val;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if ((gain_val >= ANALOG_GAIN_2) && (gain_val < ANALOG_GAIN_3)) {
		sensor_write(sd, 0xb6, 0x01);
		tmp = 64 * gain_val / ANALOG_GAIN_2;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if ((gain_val >= ANALOG_GAIN_3) && (gain_val < ANALOG_GAIN_4)) {
		sensor_write(sd, 0xb6, 0x02);
		tmp = 64 * gain_val / ANALOG_GAIN_3;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if ((gain_val >= ANALOG_GAIN_4) && (gain_val < ANALOG_GAIN_5)) {
		sensor_write(sd, 0xb6, 0x03);
		tmp = 64 * gain_val / ANALOG_GAIN_4;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if ((gain_val >= ANALOG_GAIN_5) && (gain_val < ANALOG_GAIN_6)) {
		sensor_write(sd, 0xb6, 0x04);
		tmp = 64 * gain_val / ANALOG_GAIN_5;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if ((gain_val >= ANALOG_GAIN_6) && (gain_val < ANALOG_GAIN_7)) {
		sensor_write(sd, 0xb6, 0x05);
		tmp = 64 * gain_val / ANALOG_GAIN_6;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if ((gain_val >= ANALOG_GAIN_7) && (gain_val < ANALOG_GAIN_8)) {
		sensor_write(sd, 0xb6, 0x06);
		tmp = 64 * gain_val / ANALOG_GAIN_7;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if ((gain_val >= ANALOG_GAIN_8) && (gain_val < ANALOG_GAIN_9)) {
		sensor_write(sd, 0xb6, 0x07);
		tmp = 64 * gain_val / ANALOG_GAIN_8;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if ((gain_val >= ANALOG_GAIN_9) && (gain_val < ANALOG_GAIN_10)) {
		sensor_write(sd, 0xb6, 0x08);
		tmp = 64 * gain_val / ANALOG_GAIN_9;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if ((gain_val >= ANALOG_GAIN_10) &&
			(gain_val < ANALOG_GAIN_11)) {
		sensor_write(sd, 0xb6, 0x09);
		tmp = 64 * gain_val / ANALOG_GAIN_10;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if (gain_val >= ANALOG_GAIN_11) {
		sensor_write(sd, 0xb6, 0x0a);
		tmp = 64 * gain_val / ANALOG_GAIN_11;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	}

	sensor_dbg("sensor_s_gain info->gain %d\n", gain_val);
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

		sensor_print("PWR_ON!\n");
		cci_lock(sd);

		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_set_status(sd, PWDN, 1);

		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(30000, 31000);

		vin_set_pmu_channel(sd, IOVDD, ON);
		usleep_range(30000, 31000);

		vin_set_pmu_channel(sd, AVDD, ON);
		usleep_range(30000, 31000);

		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(30000, 31000);

		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);

		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(30000, 31000);
		cci_unlock(sd);
		break;
	case PWR_OFF:

		sensor_print("PWR_OFF!\n");
		cci_lock(sd);

		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);

		vin_set_mclk(sd, OFF);

		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);

		vin_gpio_set_status(sd, PWDN, 0);
		vin_gpio_set_status(sd, RESET, 0);

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

	sensor_read(sd, 0xf0, &rdval);
	if (rdval != (V4L2_IDENT_SENSOR >> 8))
		return -ENODEV;

	sensor_read(sd, 0xf1, &rdval);
	if (rdval != (V4L2_IDENT_SENSOR & 0xff))
		return -ENODEV;

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
	info->width = VGA_WIDTH;
	info->height = VGA_HEIGHT;
	info->hflip = 0;
	info->vflip = 0;
	info->exp = 0;
	info->gain = 0;

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
		.mbus_code = MEDIA_BUS_FMT_SRGGB10_1X10,
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
		.width      = 768,
		.height     = 576,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 1182,
		.vts        = 654,
		.pclk       = 24 * 1000 * 1000,
		.mipi_bps   = 192 * 1000 * 1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (654) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 256 << 4,
		.regs       = sensor_768_576_30fps_regs,
		.regs_size  = ARRAY_SIZE(sensor_768_576_30fps_regs),
		.set_size   = NULL,
	},
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

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
	gc0403_sensor_vts = wsize->vts;
	info->exp = 0;
	info->gain = 0;

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
	.g_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.enum_frame_interval = sensor_enum_frame_interval,
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
	.addr_width = CCI_BITS_8,
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

static int sensor_init_controls(struct v4l2_subdev *sd,
				const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	struct v4l2_ctrl *ctrl;
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 2);

	v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 16, 39 * 16, 1, 16);
	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE,
				 3 * 16, 65536 * 16, 1, 3 * 16);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

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

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	sd = &info->sd;

	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);
	sensor_init_controls(sd, &sensor_ctrl_ops);

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
