/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for sc2231 Raw cameras.
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

MODULE_AUTHOR("myf");
MODULE_DESCRIPTION("A low-level driver for SC2231 sensors");
MODULE_LICENSE("GPL");

#define MCLK              (27 * 1000 * 1000)
#define V4L2_IDENT_SENSOR 0x2232

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 20

/*
 * The SC2231 i2c address
 */
#define I2C_ADDR 0x60

#define SENSOR_NUM 0x2
#define SENSOR_NAME "sc2231_mipi"
#define SENSOR_NAME_2 "sc2231_mipi_2"

/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};

static struct regval_list sensor_1080p20_regs[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x3039, 0x52},
	{0x303b, 0x14},
	{0x303d, 0x10},
	{0x3221, 0x00},
	{0x391e, 0x00},
	{0x5780, 0x7f},
	{0x5781, 0x04},
	{0x5782, 0x04},
	{0x5783, 0x02},
	{0x5784, 0x01},
	{0x57a0, 0x00},
	{0x57a2, 0x01},
	{0x57a4, 0x60},
	{0x3002, 0x80},
	{0x3000, 0x00},
	{0x3001, 0x00},
	{0x303f, 0x01},
	{0x3018, 0x33},
	{0x301a, 0xf0},
	{0x301c, 0x78},
	{0x3037, 0x20},
	{0x4603, 0x00},
	{0x3635, 0x1c},
	{0x363b, 0x16},
	{0x3306, 0x50},
	{0x330a, 0x00},
	{0x330b, 0xd0},
	{0x3f00, 0x0f},
	{0x3f04, 0x04},
	{0x3f05, 0x28},
	{0x3631, 0x88},
	{0x3638, 0x24},
	{0x330d, 0x36},
	{0x33b3, 0x32},
	{0x33b4, 0x32},
	{0x3338, 0x37},
	{0x33ae, 0x32},
	{0x3339, 0x37},
	{0x333a, 0x33},
	{0x3314, 0x14},
	{0x330f, 0x01},
	{0x3310, 0x23},
	{0x330e, 0x18},
	{0x3367, 0x08},
	{0x335d, 0x20},
	{0x3637, 0x2c},
	{0x3303, 0x38},
	{0x331e, 0x31},
	{0x3309, 0x70},
	{0x331f, 0x69},
	{0x3038, 0x6e},
	{0x3614, 0x80},
	{0x3364, 0x1d},
	{0x33b6, 0x0f},
	{0x33b7, 0x3e},
	{0x33b8, 0x08},
	{0x33b9, 0x80},
	{0x33ba, 0xc0},
	{0x360f, 0x05},
	{0x367a, 0x38},
	{0x367b, 0x38},
	{0x3671, 0xf6},
	{0x3672, 0x16},
	{0x3673, 0x16},
	{0x366e, 0x04},
	{0x3670, 0x48},
	{0x369c, 0x38},
	{0x369d, 0x38},
	{0x3690, 0x24},
	{0x3691, 0x44},
	{0x3692, 0x44},
	{0x36a2, 0x00},
	{0x36a3, 0x3f},
	{0x3699, 0x80},
	{0x369a, 0x80},
	{0x369b, 0x9f},
	{0x4837, 0x35},
	{0x3641, 0x01},
	{0x3902, 0xc5},
	{0x33aa, 0x00},
	{0x3e26, 0x20},
	{0x57a1, 0x72},
	{0x57a3, 0xf2},
	{0x363c, 0x06},
	{0x4800, 0x64},
	{0x5988, 0x02},
	{0x598e, 0x08},
	{0x598f, 0x50},
	{0x3940, 0x64},
	{0x3933, 0x0a},
	{0x3934, 0x0c},
	{0x3942, 0x02},
	{0x3943, 0x15},
	{0x3e00, 0x00},
	{0x3e01, 0x8c},
	{0x3e02, 0x20},
	{0x3e03, 0x0b},
	{0x3e06, 0x00},
	{0x3e07, 0x80},
	{0x3e08, 0x03},
	{0x3e09, 0x20},
	{0x3633, 0x24},
	{0x3622, 0xf6},
	{0x3301, 0x08},
	{0x363a, 0x80},
	{0x3630, 0x40},
	{0x3632, 0x88},
	{0x3200, 0x00},
	{0x3201, 0x00},
	{0x3202, 0x00},
	{0x3203, 0x00},
	{0x3204, 0x07},
	{0x3205, 0x8f},
	{0x3206, 0x04},
	{0x3207, 0x47},
	{0x3208, 0x07},
	{0x3209, 0x88},
	{0x320a, 0x04},
	{0x320b, 0x40},
	{0x3210, 0x00},
	{0x3211, 0x04},
	{0x3212, 0x00},
	{0x3213, 0x04},
	{0x320e, 0x06},
	{0x320f, 0x97},
	{0x3e01, 0xd2},
	{0x3e02, 0x60},
	{0x0100, 0x01},
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

static int sc2231_sensor_vts;
/* static int sc2231_sensor_svr; */
static unsigned char shutter_delay = 1;
static unsigned char shutter_delay_cnt;
static unsigned char fps_change_flag;
static unsigned char reg_hold;

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	data_type explow, exphigh;
	struct sensor_info *info = to_state(sd);

	exphigh = (unsigned char) (0xff & (exp_val>>7));
	explow = (unsigned char) (0xf0 & (exp_val<<1));
	sensor_write(sd, 0x3e02, explow);
	sensor_write(sd, 0x3e01, exphigh);
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

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	struct sensor_info *info = to_state(sd);
	data_type gainlow = 0;
	data_type gainhigh = 0;
	data_type gaindiglow = 0x80;
	data_type gaindighigh = 0x00;
	int gainana = gain_val;

	if (gainana < 0x20) {
		gainhigh = 0x03;
		gainlow = gainana << 1;
		sensor_write(sd, 0x3630, 0x40);
		sensor_write(sd, 0x3632, 0x88);
	} else if (gainana < 2 * 0x20) {
		gainhigh = 0x07;
		gainlow = gainana;
		sensor_write(sd, 0x3630, 0x40);
		sensor_write(sd, 0x3632, 0x88);
	} else if (gainana < 4 * 0x20) {
		gainhigh = 0x0f;
		gainlow = gainana >> 1;
		sensor_write(sd, 0x3630, 0x40);
		sensor_write(sd, 0x3632, 0x88);
	} else if (gainana < 8 * 0x20) {
		gainhigh = 0x1f;
		gainlow = gainana >> 2;
		sensor_write(sd, 0x3630, 0x40);
		sensor_write(sd, 0x3632, 0x88);
	} else {
		gainhigh = 0x1f;
		gainlow = 0x3f;
		if (gainana < 16 * 0x20) {
			gaindiglow = gainana >> 1;
			gaindighigh = 0x00;
			sensor_write(sd, 0x3630, 0x40);
			sensor_write(sd, 0x3632, 0x88);
		} else if (gainana < 32 * 0x20) {
			gaindiglow = gainana >> 2;
			gaindighigh = 0x01;
			sensor_write(sd, 0x3630, 0x40);
			sensor_write(sd, 0x3632, 0x88);
		} else if (gainana < 64 * 0x20) {
			gaindiglow = gainana >> 3;
			gaindighigh = 0x03;
			sensor_write(sd, 0x3630, 0x40);
			sensor_write(sd, 0x3632, 0x88);
		} else if (gainana < 128 * 0x20) {
			gaindiglow = gainana >> 4;
			gaindighigh = 0x07;
			sensor_write(sd, 0x3630, 0x40);
			sensor_write(sd, 0x3632, 0x88);
		} else if (gainana < 256 * 0x20) {
			gaindiglow = gainana >> 5;
			gaindighigh = 0x0f;
			sensor_write(sd, 0x3630, 0x40);
			sensor_write(sd, 0x3632, 0x88);
		} else {
			gaindiglow = 0xfc;
			gaindighigh = 0x0f;
			sensor_write(sd, 0x3630, 0x4f);
			sensor_write(sd, 0x3632, 0xc8);
		}
	}

	sensor_write(sd, 0x3e09, (unsigned char)gainlow);
	sensor_write(sd, 0x3e08, (unsigned char)gainhigh);
	sensor_write(sd, 0x3e07, (unsigned char)gaindiglow);
	sensor_write(sd, 0x3e06, (unsigned char)gaindighigh);
	sensor_dbg("sensor_set_anagain = %d, 0x%x, 0x%x Done!\n",
			gain_val, gainhigh << 2, gainlow);
	sensor_dbg("digital_gain = 0x%x, 0x%x Done!\n", gaindighigh, gaindiglow);
	info->gain = gain_val;

	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	struct sensor_info *info = to_state(sd);
	int exp_val, gain_val;

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	if (gain_val < 1 * 16)
		gain_val = 16;
	if (exp_val > 0xfffff)
		exp_val = 0xfffff;
	if (fps_change_flag) {
		if (shutter_delay_cnt == shutter_delay) {
			/* sensor_write(sd, 0x320f, sc2231_sensor_vts / (sc2231_sensor_svr + 1) & 0xFF);
			sensor_write(sd, 0x320e, sc2231_sensor_vts / (sc2231_sensor_svr + 1) >> 8 & 0xFF);
			sensor_write(sd, 0x302d, 0);
			shutter_delay_cnt = 0; */
			fps_change_flag = 0;
		} else
			shutter_delay_cnt++;
	}

	reg_hold = 0xff;
	sensor_write(sd, 0x3812, 0x00);
	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);
	sensor_write(sd, 0x3812, 0x30);
	if (reg_hold != 0xff)
		sensor_write(sd, 0x3221, reg_hold);
	reg_hold = 0;

	sensor_dbg("sensor_set_gain exp = %d, %d Done!\n", gain_val, exp_val);
	info->exp = exp_val;
	info->gain = gain_val;
	return 0;
}

static int sensor_s_fps(struct v4l2_subdev *sd,
			struct sensor_fps *fps)
{
	struct sensor_info *info = to_state(sd);
	struct sensor_win_size *wsize = info->current_wins;

	sc2231_sensor_vts = wsize->pclk/fps->fps/wsize->hts;
	fps_change_flag = 1;
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
		ret = sensor_write(sd, 0x0100, rdval&0xfe);
	else
		ret = sensor_write(sd, 0x0100, rdval|0x01);
	return ret;
}
/*
 *set && get sensor flip
 */
static int sensor_get_fmt_mbus_core(struct v4l2_subdev *sd, int *code)
{
	struct sensor_info *info = to_state(sd);
	data_type get_value;

	sensor_read(sd, 0x3221, &get_value);
	switch (get_value) {
	case 0x00:
		*code = MEDIA_BUS_FMT_SBGGR10_1X10;
		break;
	case 0x01:
		*code = MEDIA_BUS_FMT_SBGGR10_1X10;
		break;
	case 0x10:
		*code = MEDIA_BUS_FMT_SBGGR10_1X10;
		break;
	case 0x11:
		*code = MEDIA_BUS_FMT_SBGGR10_1X10;
		break;
	default:
		*code = info->fmt->mbus_code;
	}
	return 0;
}

data_type sensor_flip_status;

static int sensor_s_hflip(struct v4l2_subdev *sd, int enable)
{
	data_type get_value;
	data_type set_value;

	printk("into set sensor hfilp the value:%d \n", enable);
	if (!(enable == 0 || enable == 1))
		return -1;

	sensor_read(sd, 0x3221, &get_value);
	if (enable)
		set_value = get_value | 0x06;
	else
		set_value = get_value & 0xf9;

	if (reg_hold != 0)
		reg_hold = set_value;
	else
		sensor_write(sd, 0x3221, set_value);

	return 0;
}

static int sensor_s_vflip(struct v4l2_subdev *sd, int enable)
{
	data_type get_value;
	data_type set_value;

	printk("into set sensor hfilp the value:%d \n", enable);
	if (!(enable == 0 || enable == 1))
		return -1;

	sensor_read(sd, 0x3221, &get_value);
	if (enable)
		set_value = get_value | 0xe0;
	else
		set_value = get_value & 0x1f;

	if (reg_hold != 0)
		reg_hold = set_value;
	else
		sensor_write(sd, 0x3221, set_value);

	return 0;
}

/*
 * Stuff that knows about the sensor.
 */
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	int ret = 0;

	switch (on) {
	case STBY_ON:
		sensor_dbg("STBY_ON!\n");
		cci_lock(sd);
		ret = sensor_s_sw_stby(sd, STBY_ON);
		if (ret < 0)
			sensor_err("soft stby falied!\n");
		usleep_range(10000, 12000);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		sensor_dbg("STBY_OFF!\n");
		cci_lock(sd);
		usleep_range(10000, 12000);
		ret = sensor_s_sw_stby(sd, STBY_OFF);
		if (ret < 0)
			sensor_err("soft stby off falied!\n");
		cci_unlock(sd);
		break;
	case PWR_ON:
		sensor_dbg("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_set_status(sd, POWER_EN, 1);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_set_mclk_freq(sd, MCLK);
		usleep_range(30000, 32000);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_set_mclk(sd, OFF);
		vin_set_pmu_channel(sd, AFVDD, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_LOW);
		vin_gpio_set_status(sd, RESET, 0);
		vin_gpio_set_status(sd, PWDN, 0);
		vin_gpio_set_status(sd, POWER_EN, 0);
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
		usleep_range(1000, 1200);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	unsigned int SENSOR_ID = 0;
	data_type rdval;
	int cnt = 0;

	sensor_read(sd, 0x3107, &rdval);
	SENSOR_ID |= (rdval << 8);
	sensor_read(sd, 0x3108, &rdval);
	SENSOR_ID |= (rdval);
	sensor_print("V4L2_IDENT_SENSOR = 0x%x\n", SENSOR_ID);

	while ((SENSOR_ID != V4L2_IDENT_SENSOR) && (cnt < 5)) {
		sensor_read(sd, 0x3107, &rdval);
		SENSOR_ID |= (rdval << 8);
		sensor_read(sd, 0x3108, &rdval);
		SENSOR_ID |= (rdval);
		sensor_print("retry = %d, V4L2_IDENT_SENSOR = %x\n",
			cnt, SENSOR_ID);
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

	/* Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = 1920;
	info->height = 1080;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->exp = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30;	/* 30fps */

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
	case VIDIOC_VIN_SENSOR_SET_FPS:
		ret = sensor_s_fps(sd, (struct sensor_fps *)arg);
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
	 .width = 1928,
	 .height = 1088,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 2200,
	 .vts = 1688,
	 .pclk = 74250000,
	 .mipi_bps = 371 * 1000 * 1000,
	 .fps_fixed = 20,
	 .bin_factor = 1,
	 .intg_min = 3 << 4,
	 .intg_max = (1688 - 4) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 1440 << 4,
	 .regs = sensor_1080p20_regs,
	 .regs_size = ARRAY_SIZE(sensor_1080p20_regs),
	 .set_size = NULL,
	 .vipp_w     = 1920,
	 .vipp_h     = 1080,
	 .vipp_hoff  = 4,
	 .vipp_voff  = 4,
	 },

};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	struct sensor_info *info = to_state(sd);

	cfg->type = V4L2_MBUS_CSI2_DPHY;
	if (info->isp_wdr_mode == ISP_DOL_WDR_MODE)
		cfg->flags = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0 | V4L2_MBUS_CSI2_CHANNEL_1;
	else
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
	/* data_type rdval_l, rdval_h; */
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
	sc2231_sensor_vts = wsize->vts;
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
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_8,
	}, {
		.name = SENSOR_NAME_2,
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_8,
	}
};

static int sensor_init_controls(struct v4l2_subdev *sd, const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	struct v4l2_ctrl *ctrl;
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 4);

	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 1600,
			      256 * 1600, 1, 1 * 1600);

	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 1,
			      65536 * 16, 1, 1);
	v4l2_ctrl_new_std(handler, ops, V4L2_CID_HFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(handler, ops, V4L2_CID_VFLIP, 0, 1, 1, 0);

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
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET2 | MIPI_NORMAL_MODE;
	info->time_hs = 0x23;
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
