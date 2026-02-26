/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for Raw cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
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

MODULE_AUTHOR("xjg");
MODULE_DESCRIPTION("A low-level driver for f37h sensors");
MODULE_LICENSE("GPL");

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR  0x0F37

/*
 * Our nominal (default) frame rate.
 */
#define ID_REG_HIGH		0x0A
#define ID_REG_LOW		0x0B
#define ID_VAL_HIGH		((V4L2_IDENT_SENSOR) >> 8)
#define ID_VAL_LOW		((V4L2_IDENT_SENSOR) & 0xff)
#define SENSOR_FRAME_RATE 30

/*
 * The f37h i2c address
 */
#define I2C_ADDR 0x80
#define SENSOR_NUM 0x2
#define SENSOR_NAME "f37h_mipi"
#define SENSOR_NAME_2 "f37h_mipi_2"
/*
 * The default register settings
 */
static struct regval_list sensor_default_regs[] = {

};

/*
 * window_size=1920*1080 mipi@2lane
 * mclk=24mhz, mipi_clk=324Mbps
 * pixel_line_total=3840, line_frame_total=1125
 * frame_rate=30fps
 */
static struct regval_list sensor_1080p30_regs[] = {
	{0x12, 0x50},
	{0x48, 0x8A},
	{0x48, 0x0A},
	{0x0E, 0x11},
	{0x0F, 0x14},
	{0x10, 0x36},
	{0x11, 0x80},
	{0x0D, 0xF0},
	{0x5F, 0x41},
	{0x60, 0x20},
	{0x58, 0x12},
	{0x57, 0x60},
	{0x9D, 0x00},
	{0x20, 0x80},
	{0x21, 0x07},
	{0x22, 0x65},
	{0x23, 0x04},
	{0x24, 0xC0},
	{0x25, 0x38},
	{0x26, 0x43},
	{0x27, 0x19},
	{0x28, 0x14},
	{0x29, 0x07},
	{0x2A, 0x0A},
	{0x2B, 0x17},
	{0x2C, 0x00},
	{0x2D, 0x00},
	{0x2E, 0x14},
	{0x2F, 0x64},/* bsun */
	{0x41, 0xC8},
	{0x42, 0x3B},
	{0x47, 0x42},
	{0x76, 0x60},
	{0x77, 0x09},
	{0x1D, 0x00},
	{0x1E, 0x04},
	{0x6C, 0x40},
	{0x6E, 0x2C},
	{0x70, 0xDC},
	{0x71, 0xD3},
	{0x72, 0xD4},
	{0x73, 0x58},
	{0x74, 0x02},
	{0x78, 0x96},
	{0x89, 0x01},
	{0x6B, 0x20},
	{0x86, 0x40},
	{0x31, 0x0C},
	{0x32, 0x38},
	{0x33, 0x6C},
	{0x34, 0x88},
	{0x35, 0x88},
	{0x3A, 0xAF},
	{0x3B, 0x00},
	{0x3C, 0x57},
	{0x3D, 0x78},
	{0x3E, 0xFF},
	{0x3F, 0xF8},
	{0x40, 0xFF},
	{0x56, 0xB2},
	{0x59, 0xE8},
	{0x5A, 0x04},
	{0x85, 0x70},
	{0x8A, 0x04},
	{0x91, 0x13},
	{0x9B, 0x03},
	{0x9C, 0xE1},
	{0xA9, 0x78},
	{0x5B, 0xB0},
	{0x5C, 0x71},
	{0x5D, 0xF6},
	{0x5E, 0x14},
	{0x62, 0x01},
	{0x63, 0x0F},
	{0x64, 0xC0},
	{0x65, 0x02},
	{0x67, 0x65},
	{0x66, 0x04},
	{0x68, 0x00},
	{0x69, 0x7C},
	{0x6A, 0x12},
	{0x7A, 0x80},
	{0x82, 0x23},/* bsun */
	{0x8F, 0x91},
	{0xAE, 0x30},
	{0x13, 0x81},
	{0x96, 0x04},
	{0x4A, 0x05},
	{0x7E, 0xCD},
	{0x50, 0x02},
	{0x49, 0x10},
	{0xAF, 0x12},
	{0x80, 0x01},
	{0x7B, 0x4A},
	{0x7C, 0x08},
	{0x7F, 0x57},
	{0x90, 0x00},
	{0x8C, 0xFF},
	{0x8D, 0xC7},
	{0x8E, 0x00},
	{0x8B, 0x01},
	{0x0C, 0x40},/* bsun */
	{0x81, 0x74},
	{0x19, 0x20},
	{0x46, 0x00},
	{0x12, 0x10},
	{0x48, 0x8a},
	{0x48, 0x0a},
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
 * if not support the follow function , retrun -EINVAL
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
	struct sensor_info *info = to_state(sd);
	int tmp_exp_val = exp_val / 16;

	sensor_write(sd, 0x02, (tmp_exp_val >> 8) & 0xFF);
	sensor_write(sd, 0x01, (tmp_exp_val & 0xFF));

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

#define GAIN_TABLE_MAX		64
unsigned int gainLevelTable[GAIN_TABLE_MAX] = {
	 64,  68,  72,  76,  80,  84,  88,  92,
	 96, 100, 104, 108, 112, 116, 120, 124,
	128, 136, 144, 152, 160, 168, 176, 184,
	192, 200, 208, 216, 224, 232, 240, 248,
	256, 272, 288, 304, 320, 336, 352, 368,
	384, 400, 416, 432, 448, 464, 480, 496,
	512, 544, 576, 608, 640, 672, 704, 736,
	768, 800, 832, 864, 896, 928, 960, 992
};

static int setSensorGain(struct v4l2_subdev *sd, int gain)
{
	int i;
	/* printk("gain_val:%d\n", gain*4); */
	for (i = 0; i < (GAIN_TABLE_MAX - 1); i++) {
		if ((gainLevelTable[i] <= gain) && (gain < gainLevelTable[i+1]))
			break;
	}
	sensor_write(sd, 0x00, i);
	/* Patch for F37H B-SUN , >2x Gain disable it */
	if (i <= 0x10) {
		sensor_write(sd, 0x2f, 0x64);
		sensor_write(sd, 0x0c, 0x40);
		sensor_write(sd, 0x82, 0x23);
	} else {
		sensor_write(sd, 0x2f, 0x44);
		sensor_write(sd, 0x0c, 0x20);
		sensor_write(sd, 0x82, 0x22);
	}

	return 0;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	struct sensor_info *info = to_state(sd);

	if (gain_val == info->gain)
		return 0;
	/* printk("gain_val:%d\n", gain_val); */
	setSensorGain(sd, gain_val * 4);
	info->gain = gain_val;

	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
				 struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val;
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	if (gain_val < (1 * 16))
		gain_val = 16;

	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);

	info->exp = exp_val;
	info->gain = gain_val;
	return 0;
}

/*
 * Stuff that knows about the sensor.
 */
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	switch (on) {
	case STBY_ON:
		sensor_dbg("STBY_ON!\n");
		cci_lock(sd);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		cci_unlock(sd);
		break;

	case STBY_OFF:
		sensor_dbg("STBY_OFF!\n");
		cci_lock(sd);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		cci_unlock(sd);
		usleep_range(10000, 12000);
		break;

	case PWR_ON:
		sensor_dbg("PWR_ON!\n");
		cci_lock(sd);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_set_status(sd, POWER_EN, 1);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_set_mclk(sd, ON);
		vin_set_mclk_freq(sd, MCLK);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		cci_unlock(sd);
		break;

	case PWR_OFF:
		sensor_dbg("PWR_OFF!do nothing\n");
		cci_lock(sd);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		vin_set_mclk(sd, OFF);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_LOW);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		usleep_range(10000, 12000);
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

	sensor_dbg("%s: val=%d\n", __func__);
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
	data_type rdval;
	int eRet;
	int times_out = 3;

	do {
		eRet = sensor_read(sd, ID_REG_HIGH, &rdval);
		sensor_dbg("eRet:%d, ID_VAL_HIGH:0x%x, times_out:%d\n",
			eRet, rdval, times_out);
		usleep_range(200000, 220000);
		times_out--;
	} while (eRet < 0  &&  times_out > 0);

	sensor_read(sd, ID_REG_HIGH, &rdval);
	sensor_dbg("ID_VAL_HIGH = %2x, Done!\n", rdval);
	if (rdval != ID_VAL_HIGH)
		return -ENODEV;

	sensor_read(sd, ID_REG_LOW, &rdval);
	sensor_dbg("ID_VAL_LOW = %2x, Done!\n", rdval);
	if (rdval != ID_VAL_LOW)
		return -ENODEV;

	sensor_dbg("Done!\n");
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
	info->low_speed    = 0;
	info->width        = 1920;
	info->height       = 1080;
	info->hflip        = 0;
	info->vflip        = 0;
	info->gain         = 0;
	info->exp          = 0;

	info->tpf.numerator      = 1;
	info->tpf.denominator    = 30;	/* 30fps */
/*	info->preview_first_flag = 1; */
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
		break;
	case VIDIOC_VIN_SENSOR_EXP_GAIN:
		sensor_s_exp_gain(sd, (struct sensor_exp_gain *)arg);
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
		.desc      = "Raw RGB Bayer",
		/* .mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10, */
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.regs      = sensor_fmt_raw,
		.regs_size = ARRAY_SIZE(sensor_fmt_raw),
		.bpp       = 1
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */

static struct sensor_win_size sensor_win_sizes[] = {
#if F37H_DBG
	{
		.width      = 1920,
		.height     = 1080,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 3840,
		.vts        = 1125,
		.pclk       = 129600000,
		.mipi_bps   = 324 * 1000 * 1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 1100 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 15 << 4,
		.regs       = sensor_1080p30_regs,
		.regs_size  = ARRAY_SIZE(sensor_1080p30_regs),
		.set_size   = NULL,
	},

	{
		.width      = 1920,
		.height     = 1080,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 3840,
		.vts        = 1125,
		.pclk       = 129600000,
		.mipi_bps   = 324 * 1000 * 1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 1100 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 15 << 4,
		.regs       = sensor_1080p30_regs,
		.regs_size  = ARRAY_SIZE(sensor_1080p30_regs),
		.set_size   = NULL,
		.vipp_w     = 1920,
		.vipp_h     = 1080,
		.vipp_hoff  = 4,
		.vipp_voff  = 3,
	},
#else
	{
		.width      = 1920,
		.height     = 1080,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 3546,
		.vts        = 1250,
		.pclk       = 129600000,
		.mipi_bps   = 324 * 1000 * 1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 1100 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 15 << 4,
		.regs       = sensor_1080p30_regs,
		.regs_size  = ARRAY_SIZE(sensor_1080p30_regs),
		.set_size   = NULL,
	},

	{
		.width      = 1920,
		.height     = 1080,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 3456,
		.vts        = 1250,
		.pclk       = 129600000,
		.mipi_bps   = 324 * 1000 * 1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 1100 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 15 << 4,
		.regs       = sensor_1080p30_regs,
		.regs_size  = ARRAY_SIZE(sensor_1080p30_regs),
		.set_size   = NULL,
		.vipp_w     = 1920,
		.vipp_h     = 1080,
		.vipp_hoff  = 4,
		.vipp_voff  = 3,
	},
#endif
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	/* struct sensor_info *info = to_state(sd); */

	cfg->type  = V4L2_MBUS_CSI2_DPHY;
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

static int sensor_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	/* Fill in min, max, step and default value for these controls. */
	/* see include/linux/videodev2.h for details */
	switch (qc->id) {
	case V4L2_CID_GAIN:
		return v4l2_ctrl_query_fill(qc, 1 * 16, 128 * 16 - 1, 1, 16);
	case V4L2_CID_EXPOSURE:
		return v4l2_ctrl_query_fill(qc, 1, 65536 * 16, 1, 1);
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_queryctrl qc;
	int ret;
	struct sensor_info *info =
		container_of(ctrl->handler, struct sensor_info, handler);
	struct v4l2_subdev *sd = &info->sd;

	qc.id = ctrl->id;
	ret = sensor_queryctrl(sd, &qc);
	if (ret < 0)
		return ret;

	if (ctrl->val < qc.minimum || ctrl->val > qc.maximum)
		return -ERANGE;

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

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);

	if (wsize->set_size)
		wsize->set_size(sd);

	info->width = wsize->width;
	info->height = wsize->height;

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
	/* .queryctrl = sensor_queryctrl, */
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
static struct cci_driver cci_drv[] = {
	{
		.name = SENSOR_NAME,
		.addr_width = CCI_BITS_8,
		.data_width = CCI_BITS_8,
	}, {
		.name = SENSOR_NAME_2,
		.addr_width = CCI_BITS_8,
		.data_width = CCI_BITS_8,
	}
};

static int sensor_init_controls(struct v4l2_subdev *sd, const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	struct v4l2_ctrl *ctrl;
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 2);

	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 1600,
				  256 * 1600, 1, 1 * 1600);

	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 1,
				  65536 * 16, 1, 1);
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

	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	/* info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET3 | MIPI_NORMAL_MODE; */
	/* info->combo_mode = CMB_PHYA_OFFSET2 | MIPI_NORMAL_MODE; */
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->af_first_flag = 1;
	info->exp = 0;
	info->gain = 0;
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
