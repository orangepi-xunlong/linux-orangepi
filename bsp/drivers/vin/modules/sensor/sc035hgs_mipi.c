/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for sc031gs_mipi cameras.
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

MODULE_AUTHOR("zhj");
MODULE_DESCRIPTION("A low-level driver for sc035hgs sensors");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR 0x00310b

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 120

/*
 * The sc035hgs_mipi sits on i2c with ID 0x60
 */
#define I2C_ADDR 0x60
#define SENSOR_NAME "sc035hgs_mipi"

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

/* 640x480 RAW 120fps 24MHz */
static struct regval_list sensor_VGA_120fps_2lane_regs[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x36e9, 0x80},
	{0x36f9, 0x80},
	{0x3000, 0x00},
	{0x3001, 0x00},
	{0x300f, 0x0f},
	{0x3018, 0x33},
	{0x3019, 0xfc},
	{0x301c, 0x78},
	{0x301f, 0x8c},
	{0x3031, 0x0a},
	{0x3037, 0x20},
	{0x303f, 0x01},
	{0x320c, 0x04},
	{0x320d, 0x70},
	{0x320e, 0x02},
	{0x320f, 0x10},
	{0x3217, 0x00},
	{0x3218, 0x00},
	{0x3220, 0x10},
	{0x3223, 0x48},
	{0x3226, 0x74},
	{0x3227, 0x07},
	{0x323b, 0x00},
	{0x3250, 0xf0},
	{0x3251, 0x02},
	{0x3252, 0x02},
	{0x3253, 0x08},
	{0x3254, 0x02},
	{0x3255, 0x07},
	{0x3304, 0x48},
	{0x3305, 0x00},
	{0x3306, 0x98},
	{0x3309, 0x50},
	{0x330a, 0x01},
	{0x330b, 0x18},
	{0x330c, 0x18},
	{0x330f, 0x40},
	{0x3310, 0x10},
	{0x3314, 0x6b},
	{0x3315, 0x30},
	{0x3316, 0x68},
	{0x3317, 0x14},
	{0x3329, 0x5c},
	{0x332d, 0x5c},
	{0x332f, 0x60},
	{0x3335, 0x64},
	{0x3344, 0x64},
	{0x335b, 0x80},
	{0x335f, 0x80},
	{0x3366, 0x06},
	{0x3385, 0x31},
	{0x3387, 0x39},
	{0x3389, 0x01},
	{0x33b1, 0x03},
	{0x33b2, 0x06},
	{0x33bd, 0xe0},
	{0x33bf, 0x10},
	{0x3621, 0xa4},
	{0x3622, 0x05},
	{0x3624, 0x47},
	{0x3630, 0x4a},
	{0x3631, 0x58},
	{0x3633, 0x52},
	{0x3635, 0x03},
	{0x3636, 0x25},
	{0x3637, 0x8a},
	{0x3638, 0x0f},
	{0x3639, 0x08},
	{0x363a, 0x00},
	{0x363b, 0x48},
	{0x363c, 0x86},
	{0x363e, 0xf8},
	{0x3640, 0x00},
	{0x3641, 0x01},
	{0x36ea, 0x36},
	{0x36eb, 0x0e},
	{0x36ec, 0x1e},
	{0x36ed, 0x00},
	{0x36fa, 0x36},
	{0x36fb, 0x10},
	{0x36fc, 0x00},
	{0x36fd, 0x00},
	{0x3908, 0x91},
	{0x391b, 0x81},
	{0x3d08, 0x01},
//exp
	{0x3e01, 0x01},
	{0x3e02, 0x0a},
	{0x3e03, 0x2b},
//gain
	{0x3e08, 0x1c},
	{0x3e09, 0x1f},
	{0x3e06, 0x00},
	{0x3e07, 0xea},
	{0x3f04, 0x03},
	{0x3f05, 0x80},
	{0x4500, 0x59},
	{0x4501, 0xc4},
	{0x4603, 0x00},
	{0x4800, 0x64},
	{0x4809, 0x01},
	{0x4810, 0x00},
	{0x4811, 0x01},
	{0x4837, 0x38},
	{0x5011, 0x00},
	{0x5988, 0x02},
	{0x598e, 0x04},
	{0x598f, 0x30},
	{0x36e9, 0x03},
	{0x36f9, 0x03},
	{0x0100, 0x01},
//	delay in sensor_reg_init
	{0x4418, 0x0a},
	{0x363d, 0x10},
	{0x4419, 0x80},
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

static int sc035hgs_sensor_vts;
static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	struct sensor_info *info = to_state(sd);
	data_type explow, exphigh;
	data_type get_value;
	int ret = -1;
//	static int frame_cnt = 0;
	if (exp_val > ((sc035hgs_sensor_vts - 6) << 4))
		exp_val = (sc035hgs_sensor_vts - 6) << 4;
	if (exp_val < 16)
		exp_val = 16;

	exphigh = (unsigned char)(exp_val >> 8);
	explow = (unsigned char)(exp_val & 0xFF);
	ret = sensor_write(sd, 0x3e01, exphigh);
	if (ret < 0) {
		sensor_err(" sensor write error\n");
		return ret;
	}
	sensor_write(sd, 0x3e02, explow);
	sensor_read(sd, 0x3e01, &get_value);
	sensor_read(sd, 0x3e02, &get_value);
	sensor_dbg("sensor_s_exp info->exp %d\n", exp_val);
	info->exp = exp_val;

//	if (++frame_cnt > 30) {
//		frame_cnt = 0;
//		sensor_read(sd, 0x3e01, &exphigh);
//		sensor_read(sd, 0x3e02, &explow);
//		sensor_print("=====exp=========> 0x3e01 = 0x%x, 0x3e02  = 0x%x\n", exphigh, explow);
//	}

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
	data_type gain_ana = gain_val;
	data_type gain_dig_low = 0x80;
	data_type gain_dig_high = 0x00;
	data_type gain_high = 0;
	data_type gain_low = 0;
//	static int frame_cnt = 0;

	if (gain_val < 1 * 16)
		gain_val = 16;

//	gain_ana = 80;
	if (gain_ana < 0x20) {
		gain_high = 0x0;
		gain_low = gain_ana;
		sensor_write(sd, 0x3314, 0x6b);
		sensor_write(sd, 0x3317, 0x14);
		sensor_write(sd, 0x3631, 0x58);
		sensor_write(sd, 0x3630, 0x4a);
	} else if (gain_ana < 2 * 0x20) {
		gain_high = 0x01;
		gain_low = gain_ana >> 1;
		sensor_write(sd, 0x3314, 0x4f);
		sensor_write(sd, 0x3317, 0x10);
		sensor_write(sd, 0x3631, 0x48);
		sensor_write(sd, 0x3630, 0x4c);
	} else if (gain_ana < 4 * 0x20) {
		gain_high = 0x03;
		gain_low = gain_ana >> 2;
	} else if (gain_ana < 8 * 0x20) {
		gain_high = 0x07;
		gain_low = gain_ana >> 3;
		sensor_write(sd, 0x3314, 0x74);
		sensor_write(sd, 0x3317, 0x15);
		sensor_write(sd, 0x3631, 0x48);
		sensor_write(sd, 0x3630, 0x4c);
	} else {
		sensor_write(sd, 0x3314, 0x74);
		sensor_write(sd, 0x3317, 0x15);
		sensor_write(sd, 0x3631, 0x48);
		sensor_write(sd, 0x3630, 0x4c);
		gain_high = 0x07;
		gain_low = 0x1f;
		if (gain_ana < 16 * 0x20) {
			gain_dig_high = 0x00;
			gain_dig_low = gain_ana >> 1;
		} else if (gain_ana < 32 * 0x20) {
			gain_dig_high = 0x01;
			gain_dig_low = gain_ana >> 2;
		} else if (gain_ana < 64 * 0x20) {
			gain_dig_high = 0x03;
			gain_dig_low = gain_ana >> 3;
		} else {
			gain_dig_high = 0x03;
			gain_dig_low = 0xf8;
		}
	}

	sensor_write(sd, 0x3e08, (unsigned char)(gain_high << 2));
	sensor_write(sd, 0x3e09, (unsigned char)gain_low);
	sensor_write(sd, 0x3e06, (unsigned char)gain_dig_high);
	sensor_write(sd, 0x3e07, (unsigned char)gain_dig_low);
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

static int sc035hgs_flip_status;
static int sensor_get_fmt_mbus_core(struct v4l2_subdev *sd, int *code)
{
	struct sensor_info *info = to_state(sd);
	data_type get_value;

	sensor_read(sd, 0x3221, &get_value);
	sensor_dbg("===> regs_data = 0x%x, SC035HGS's format is MONO, will not to modify format\n", get_value);
	*code = info->fmt->mbus_code;

	return 0;
}

static int sensor_s_hflip(struct v4l2_subdev *sd, int enable)
{
	data_type get_value;
	data_type set_value;

	if (!(enable == 0 || enable == 1))
		return -1;

	sensor_read(sd, 0x3221, &get_value);
	sensor_dbg("===> ready to hflip, regs_data = 0x%x\n", get_value);
	if (enable) {
		set_value = get_value | 0x06;
		sc035hgs_flip_status = get_value | 0x06;
	} else {
		set_value = get_value & 0xF9;
		sc035hgs_flip_status = get_value & 0xF9;
	}
	sensor_write(sd, 0x3221, set_value);

	return 0;
}

static int sensor_s_vflip(struct v4l2_subdev *sd, int enable)
{
	data_type get_value;
	data_type set_value;

	if (!(enable == 0 || enable == 1))
		return -1;

	sensor_read(sd, 0x3221, &get_value);
	sensor_dbg("===> ready to vflip, regs_data = 0x%x\n", get_value);
	if (enable) {
		set_value = get_value | 0x60;
		sc035hgs_flip_status = get_value | 0x60;
	} else {
		set_value = get_value & 0x9F;
		sc035hgs_flip_status = get_value & 0x9F;
	}
	sensor_write(sd, 0x3221, set_value);

	return 0;
}

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	int ret;
	data_type rdval;

	sensor_read(sd, 0x0100, &rdval);
	sensor_dbg("==> standby res_data = 0x%x\n", rdval);

	if (on_off == STBY_ON) {
		ret = sensor_write(sd, 0x0100, rdval & 0xfe);
		sensor_read(sd, 0x0100, &rdval);
		sensor_dbg("==> set standby on, standby res_data = 0x%x\n", rdval);
	} else {
		ret = sensor_write(sd, 0x0100, rdval | 0x01);
		sensor_read(sd, 0x0100, &rdval);
		sensor_dbg("==> set standby off, standby res_data = 0x%x\n", rdval);
	}

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
		usleep_range(1000, 1200);
		cci_lock(sd);
		/* inactive mclk after stadby in */
		vin_set_mclk(sd, OFF);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		cci_lock(sd);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(1000, 1200);
		cci_unlock(sd);
		ret = sensor_s_sw_stby(sd, STBY_OFF);
		if (ret < 0)
			sensor_err("soft stby off falied!\n");
		usleep_range(1000, 1200);
		break;
	case PWR_ON:
		sensor_print("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		vin_set_pmu_channel(sd, CAMERAVDD, ON);
		vin_set_pmu_channel(sd, IOVDD, ON);
		usleep_range(1000, 1200);
		vin_set_pmu_channel(sd, DVDD, ON);
		usleep_range(1000, 1200);
		vin_set_pmu_channel(sd, AVDD, ON);
		usleep_range(1000, 1200);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_print("PWR_OFF!\n");
		cci_lock(sd);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_set_mclk(sd, OFF);
		vin_set_pmu_channel(sd, CAMERAVDD, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		vin_gpio_set_status(sd, RESET, 0);
		cci_unlock(sd);
		break;
	case REG_ON:
		sensor_print("REG_ON!\n");
		cci_lock(sd);
		/* fastboot need to enable regulator when register */
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
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
#if !IS_ENABLED(CONFIG_VIN_INIT_MELIS)
	unsigned int SENSOR_ID = 0;
	data_type rdval;
	int cnt = 0;
	sensor_read(sd, 0x3107, &rdval);
	SENSOR_ID |= (rdval << 16);
	sensor_read(sd, 0x3108, &rdval);
	SENSOR_ID |= (rdval << 8);
	sensor_read(sd, 0x3109, &rdval);
	SENSOR_ID |= (rdval);
	sensor_dbg("V4L2_IDENT_SENSOR = 0x%x, 0x3109=0x%x\n", SENSOR_ID, rdval);

	while ((SENSOR_ID != V4L2_IDENT_SENSOR) && (cnt < 5)) {
		sensor_read(sd, 0x3107, &rdval);
		SENSOR_ID |= (rdval << 16);
		sensor_read(sd, 0x3108, &rdval);
		SENSOR_ID |= (rdval << 8);
		sensor_read(sd, 0x3109, &rdval);
		SENSOR_ID |= (rdval);
		sensor_dbg("retry = %d, V4L2_IDENT_SENSOR = %x\n",
			cnt, SENSOR_ID);
		cnt++;
		}
	if (SENSOR_ID != V4L2_IDENT_SENSOR)
		return -ENODEV;
#endif
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
	info->tpf.numerator = 1;
	info->tpf.denominator = SENSOR_FRAME_RATE;

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
	case VIDIOC_VIN_GET_SENSOR_CODE:
		sensor_get_fmt_mbus_core(sd, (int *)arg);
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
		.width      = VGA_WIDTH,
		.height     = VGA_HEIGHT,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 1136,
		.vts        = 528,
		.pclk       = 72 * 1000 * 1000,
		.mipi_bps   = 360 * 1000 * 1000,
		.fps_fixed  = 120,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (528 - 6) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 1440 << 4,
		.regs       = sensor_VGA_120fps_2lane_regs,
		.regs_size  = ARRAY_SIZE(sensor_VGA_120fps_2lane_regs),
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
	case V4L2_CID_HFLIP:
		return sensor_s_hflip(sd, ctrl->val);
	case V4L2_CID_VFLIP:
		return sensor_s_vflip(sd, ctrl->val);
	}
	return -EINVAL;
}

static int sensor_reg_init(struct sensor_info *info)
{
	int ret;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;
	data_type flip_status = 0;
	__maybe_unused struct sensor_exp_gain exp_gain;

	ret = sensor_write_array(sd, sensor_default_regs,
		 ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	ret = sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);
	if (ret < 0) {
		sensor_err("write sensor_fmt_regs error\n");
		return ret;
	}

#if IS_ENABLED(CONFIG_VIN_INIT_MELIS)
	if (info->preview_first_flag) {
		info->preview_first_flag = 0;
	} else {
		if (wsize->regs)
			sensor_write_array(sd, wsize->regs, wsize->regs_size);
		if (info->exp && info->gain) {
			exp_gain.exp_val = info->exp;
			exp_gain.gain_val = info->gain;
		} else {
			if (wsize->wdr_mode == ISP_DOL_WDR_MODE) {
				exp_gain.exp_val = 30720;
				exp_gain.gain_val = 16;
			} else {
				exp_gain.exp_val = 15408;
				exp_gain.gain_val = 32;
			}
		}
		sensor_s_exp_gain(sd, &exp_gain);
	}
#else
	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);
#endif

	/* delay for sensor stability */
	usleep_range(10000, 12000);
	ret = sensor_write(sd, 0x4418, 0x0a);
	if (ret < 0) {
		sensor_err("write reg error\n");
		return ret;
	}
	sensor_write(sd, 0x363d, 0x10);
	sensor_write(sd, 0x4419, 0x80);

	if (wsize->set_size)
		wsize->set_size(sd);
	info->width = wsize->width;
	info->height = wsize->height;
	sc035hgs_sensor_vts = wsize->vts;

	/* check flip_status and recover it */
	ret = sensor_read(sd, 0x3221, &flip_status);
	if (ret < 0) {
		sensor_err("read reg error\n");
		return ret;
	}
	if (flip_status == 0x0) {
		sensor_write(sd, 0x3221, sc035hgs_flip_status);
		sensor_print("===> flip_status = 0x%x, recover h_v_flip_status to 0x%x\n", flip_status, sc035hgs_flip_status);
	} else {
		sensor_print("===> flip_status = 0x%x, not to recover\n", flip_status);
	}
	sensor_read(sd, 0x3221, &flip_status);

	sensor_print("s_fmt set width = %d, height = %d, flip_status: 0x3221 = 0x%x\n", wsize->width,
		     wsize->height, flip_status);

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);

	sensor_print("%s on = %d, %d*%d fps: %d code: %x\n", __func__, enable,
		     info->current_wins->width, info->current_wins->height,
		     info->current_wins->fps_fixed, info->fmt->mbus_code);

	if (!enable) {
//		sensor_s_sw_stby(sd, STBY_ON);
		return 0;
	}

	return sensor_reg_init(info);
}

/* ----------------------------------------------------------------------- */

static const struct v4l2_ctrl_ops sensor_ctrl_ops = {
	.g_volatile_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
	.try_ctrl = sensor_try_ctrl,
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

static int sensor_init_controls(struct v4l2_subdev *sd,
				const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	struct v4l2_ctrl *ctrl;
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 2);

	v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 16, 16 * 16, 1, 16);
	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE,
				 3 * 16, 65536 * 16, 1, 3 * 16);
	v4l2_ctrl_new_std(handler, ops, V4L2_CID_HFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(handler, ops, V4L2_CID_VFLIP, 0, 1, 1, 0);

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
	struct v4l2_subdev *sd = NULL;
	struct sensor_info *info = NULL;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
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
	info->preview_first_flag = 1;
	info->first_power_flag = 1;

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
