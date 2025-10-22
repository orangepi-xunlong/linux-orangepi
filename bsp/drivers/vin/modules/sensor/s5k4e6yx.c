/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for s5k4e6yx Raw cameras.
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
MODULE_DESCRIPTION("A low-level driver for s5k4e6yx sensors");
MODULE_LICENSE("GPL");

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR 0x4e60

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The s5k3h5xa i2c address
 */
#define I2C_ADDR 0x6A                  /* s5k4e6_lib.h--231h */

#define SENSOR_NUM 0x2                 //
#define SENSOR_NAME   "s5k4e6yx"       //
#define SENSOR_NAME_2 "s5k4e6yx_2"     //



/* default_regs: 2608 x 1960 30fps */
static struct regval_list sensor_default_regs[] = {



};


/* default_regs: 2608 x 1960 30fps */
static struct regval_list sensor_2608_1960_30fps_30fps_regs[] = {

	{0xFCFC, 0x4000},
	{0x6010, 0x0001},

	{0xffff, 0x0100},  /* REG_DLY */

	{0x535B, 0x00},
	{0x5402, 0x15},
	{0x5401, 0x1D},
	{0x6102, 0xC000},
	{0x614C, 0x25AA},
	{0x614E, 0x25B8},
	{0x618C, 0x08D4},
	{0x618E, 0x08D6},
	{0x6028, 0x2000},
	{0x602A, 0x11A8},
	{0x6F12, 0x3AF9},
	{0x6F12, 0x1410},
	{0x6F12, 0x39F9},
	{0x6F12, 0x1410},
	{0x6028, 0x2000},
	{0x602A, 0x0668},
	{0x6F12, 0x4010},
	{0x602A, 0x12BC},
	{0x6F12, 0x1020},
	{0x602A, 0x12C2},
	{0x6F12, 0x1020},
	{0x6F12, 0x1020},
	{0x602A, 0x12CA},
	{0x6F12, 0x1020},
	{0x6F12, 0x1010},
	{0x602A, 0x12FC},
	{0x6F12, 0x1020},
	{0x602A, 0x1302},
	{0x6F12, 0x1020},
	{0x6F12, 0x1020},
	{0x602A, 0x130A},
	{0x6F12, 0x1020},
	{0x6F12, 0x1010},
	{0x602A, 0x14B8},
	{0x6F12, 0x0001},
	{0x602A, 0x14B9},
	{0x6F12, 0x0001},
	{0x602A, 0x14C0},
	{0x6F12, 0x0000},
	{0x6F12, 0xFFDA},
	{0x6F12, 0xFFDA},
	{0x6F12, 0x0000},
	{0x6F12, 0x0000},
	{0x6F12, 0xFFDA},
	{0x6F12, 0xFFDA},
	{0x6F12, 0x0000},
	{0x602A, 0x1488},
	{0x6F12, 0xFF80},
	{0x6F12, 0xFF80},
	{0x6F12, 0xFF80},
	{0x6F12, 0xFF80},
	{0x602A, 0x1496},
	{0x6F12, 0xFF80},
	{0x6F12, 0xFF80},
	{0x6F12, 0xFF80},
	{0x6F12, 0xFF80},
	{0x602A, 0x14A4},
	{0x6F12, 0xFFC0},
	{0x6F12, 0xFFC0},
	{0x6F12, 0xFFC0},
	{0x6F12, 0xFFC0},
	{0x602A, 0x147A},
	{0x6F12, 0x0000},
	{0x6F12, 0x0002},
	{0x6F12, 0xFFFC},
	{0x602A, 0x0512},
	{0x6F12, 0x0111},
	{0x602A, 0x14AC},
	{0x6F12, 0x0000},
	{0x602A, 0x0524},
	{0x6F12, 0x0000},
	{0x3285, 0x00},
	{0x327A, 0x0001},
	{0x3283, 0x0A},
	{0x3297, 0x18},
	{0x32E1, 0x00},
	{0x3286, 0x9000},
	{0x3298, 0x40},
	{0x32AA, 0x00},
	{0x327C, 0x0400},
	{0x328A, 0x0800},
	{0x3284, 0x37},
	{0x32A1, 0x20},
	{0x32A2, 0x10},
	{0x32A4, 0x0C},
	{0x3204, 0x000C},
	{0x3206, 0x000B},
	{0x3208, 0x0009},
	{0x3210, 0x0007},
	{0x3212, 0x0007},
	{0x0200, 0x0408},
	{0x3219, 0x1C},
	{0x321A, 0x32},
	{0x321B, 0x24},
	{0x321C, 0x07},
	{0x321E, 0x08},
	{0x3220, 0x13},
	{0x3226, 0x52},
	{0x3227, 0x5C},
	{0x3228, 0x03},
	{0x0305, 0x06},  /* pre_pll_clk_div */
	{0x0307, 0x64},  /* PLL multiplier Value */
	{0x5363, 0x00},
	{0x5364, 0x42},
	{0x5365, 0xA0},
	{0x534E, 0x49},
	{0x534F, 0x10},
	{0x0340, 0x07F8},  /* frame length lines =2040=0x07f8 */
	{0x0342, 0x0B68},  /* line length pck    =2920=0x0b68 */
	{0x021E, 0x03FC},   /* coarse_integration_time_long 0x03fc=1020 */
	{0x0344, 0x0000},  /* x_addr_start =0 */
	{0x0346, 0x0000},  /* y_addr_start =0 */
	{0x0348, 0x0A2F},  /* x_addr_end  =0x0a2f=2607 */
	{0x034A, 0x07A7},  /* y_addr_end  =0x07a7=1959 */
	{0x034C, 0x0A30},  /* x_output_size= 2608 =0x0a30 */
	{0x034E, 0x07A8},  /* y_output_szie= 1960 =0x07a8 */
	{0x3500, 0x00},
	{0x3089, 0x00},
	{0x0216, 0x01},
	{0x5333, 0xE0},
	{0x5080, 0x01},
	{0x0100, 0x01},

};



/* read the value of the register */
static int sensor_read_byte(struct v4l2_subdev *sd, unsigned short reg,
	unsigned char *value)
{
	int ret = 0, cnt = 0;

	if (!sd || !sd->entity.use_count) {
		sensor_print("%s error! sensor is not used!\n", __func__);
		return -1;
	}

	ret = cci_read_a16_d8(sd, reg, value);
	while ((ret != 0) && (cnt < 2)) {
		ret = cci_read_a16_d8(sd, reg, value);
		cnt++;
	}
	if (cnt > 0)
		pr_info("%s sensor read retry = %d\n", sd->name, cnt);

	return ret;
}

static int sensor_write_byte(struct v4l2_subdev *sd, unsigned short reg,
	unsigned char value)
{
	int ret = 0, cnt = 0;

	if (!sd || !sd->entity.use_count) {
		sensor_print("%s error! sensor is not used!\n", __func__);
		return -1;
	}

	ret = cci_write_a16_d8(sd, reg, value);
	while ((ret != 0) && (cnt < 2)) {
		ret = cci_write_a16_d8(sd, reg, value);
		cnt++;
	}
	if (cnt > 0)
		pr_info("%s sensor write retry = %d\n", sd->name, cnt);

	return ret;
}


static int s5k4e6yx_write_array(struct v4l2_subdev *sd,
				struct regval_list *regs, int array_size)
{
	int i = 0, ret = 0;

	if (!regs)
		return -EINVAL;

	while (i < array_size) {
		if (regs->addr == REG_DLY) {
			usleep_range(regs->data * 1000,
				regs->data * 1000 + 100);
		} else {
			data_type testvalue;     /* for test */

			if (regs->addr == 0x535B   || regs->addr == 0x5402
			|| regs->addr == 0x5401   || regs->addr == 0x3285
			|| regs->addr == 0x3283   || regs->addr == 0x3297
			|| regs->addr == 0x32E1   || regs->addr == 0x3298
			|| regs->addr == 0x32AA   || regs->addr == 0x3284
			|| regs->addr == 0x32A1   || regs->addr == 0x32A2
			|| regs->addr == 0x32A4   || regs->addr == 0x3219
			|| regs->addr == 0x321A   || regs->addr == 0x321B
			|| regs->addr == 0x321C   || regs->addr == 0x321E
			|| regs->addr == 0x3220   || regs->addr == 0x3226
			|| regs->addr == 0x3227   || regs->addr == 0x3228
			|| regs->addr == 0x0305   || regs->addr == 0x0307
			|| regs->addr == 0x5363   || regs->addr == 0x5364
			|| regs->addr == 0x5365   || regs->addr == 0x534E
			|| regs->addr == 0x534F   || regs->addr == 0x3500
			|| regs->addr == 0x3089   || regs->addr == 0x0216
			|| regs->addr == 0x5333   || regs->addr == 0x5080
			|| regs->addr == 0x0100
		/*
		*		regs->addr == 0x32BC   ||
		*		regs->addr == 0x32BD   ||
		*		regs->addr == 0x32BE   ||
		*		regs->addr == 0x012C   ||
		*		regs->addr == 0x012D   ||
		*		regs->addr == 0x012E   ||
		*		regs->addr == 0x012F   ||
		*		regs->addr == 0x6F12
		*/
				) {
				ret = sensor_write_byte(sd,
					regs->addr, regs->data);

				usleep_range(10000, 12000);
				sensor_read(sd, regs->addr, &testvalue);
				sensor_dbg("[0x%.2x] = 0x%.2x, read = 0x%.2x\n",
					regs->addr, regs->data, testvalue);
			} else {
				ret = sensor_write(sd, regs->addr, regs->data);

				usleep_range(10000, 12000);
				sensor_read(sd, regs->addr, &testvalue);
				sensor_dbg("[0x%.2x] = 0x%.2x, read = 0x%.2x\n", regs->addr, regs->data, testvalue);

			}
			if (ret < 0) {
				sensor_print("%s sensor write array error,array_size %d!\n", sd->name, array_size);
				return -1;
			}
		}
		i++;
		regs++;
	}
	return 0;
}


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

static int s5k4e6yx_sensor_vts;
static int s5k4e6yx_sensor_hts;

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	unsigned int exp_coarse;
	unsigned short exp_fine;
	struct sensor_info *info = to_state(sd);

	if (exp_val > 0xffffff)
		exp_val = 0xfffff0;
	if (exp_val < 16)
		exp_val = 16;

	if (info->exp == exp_val)
		return 0;

	exp_coarse = exp_val >> 4;

	exp_fine = (unsigned short) (((exp_val - exp_coarse * 16) *
					s5k4e6yx_sensor_hts) / 16);

	sensor_write(sd, 0x0200, exp_fine);

	sensor_write(sd, 0x0202, (unsigned short)exp_coarse);
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

	data_type ana_gain_min, ana_gain_max;

	ana_gain_min = 0x0020;
	ana_gain_max = 0x0200;

	int ana_gain = 0;

	if (info->gain == gain_val)
		return 0;

	if (gain_val <= ana_gain_min)
		ana_gain = ana_gain_min;
	else if (gain_val > ana_gain_min && gain_val <= (ana_gain_max))
		ana_gain = gain_val;
	else
		ana_gain = ana_gain_max;


	ana_gain *= 2;

	sensor_write(sd, 0x0204, (unsigned short)ana_gain);
	info->gain = gain_val;

	return 0;
}


static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val, shutter, frame_length;
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	shutter = exp_val >> 4;

	if (shutter  > s5k4e6yx_sensor_vts - 4)
		frame_length = shutter + 4;
	else
		frame_length = s5k4e6yx_sensor_vts;

	sensor_write_byte(sd, 0x0104, 0x01);
	sensor_write(sd, 0x0340, frame_length);
	sensor_s_gain(sd, gain_val);
	sensor_s_exp(sd, exp_val);
	sensor_write_byte(sd, 0x0104, 0x00);

	info->exp = exp_val;
	info->gain = gain_val;
	return 0;
}

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	int ret;
	unsigned char rdval;

	ret = sensor_read_byte(sd, 0x0100, &rdval);
	if (ret != 0)
		return ret;

	if (on_off == STBY_ON)
		ret = sensor_write_byte(sd, 0x0100, rdval & 0xfe);
	else
		ret = sensor_write_byte(sd, 0x0100, rdval | 0x01);
	return ret;
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
			sensor_dbg("soft stby falied!\n");
		usleep_range(10000, 12000);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		sensor_dbg("STBY_OFF!\n");
		cci_lock(sd);
		usleep_range(10000, 12000);
		ret = sensor_s_sw_stby(sd, STBY_OFF);
		if (ret < 0)
			sensor_dbg("soft stby off falied!\n");
		cci_unlock(sd);
		break;
	case PWR_ON:
		sensor_dbg("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		/* RESET port set to output mode and pull-up */
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_set_status(sd, POWER_EN, 1);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);
		usleep_range(1000, 1200);
		vin_set_pmu_channel(sd, IOVDD, ON);
		usleep_range(1000, 1200);
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
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
	unsigned short rdval = 0;

	sensor_read(sd, 0x0000, &rdval);
	if (rdval != 0x4E60) {
	/* sensor ID */
		sensor_dbg("s5k4e6y read 0x0000: 0x%04x != 0x4E60\n", rdval);
		return -ENODEV;
	}
	sensor_dbg("s5k4e6y sensor detect success ID = 0x%04x\n", rdval);
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
		sensor_dbg("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = 2608;
	info->height = 1960;
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
			sensor_dbg("empty wins!\n");
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
		ret = 0;
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
	.width         = 2608,    ////
	.height        = 1960,    ////
	.hoffset       = 0,
	.voffset       = 0,
	.hts           = 2040,  /* 2040*2920*30=178 704 00 */
	.vts           = 2920,  ////
	.pclk          = 180*1000*1000,   ////
	.mipi_bps      = 1008*1000*1000,  ////
	.fps_fixed     = 30,
	.bin_factor    = 1,
	.intg_min      = 16,
	.intg_max      = (2920-4)<<4,    ////
	.gain_min      = 16,
	.gain_max      = (32<<4),
	.regs          = sensor_2608_1960_30fps_30fps_regs,
	.regs_size     = ARRAY_SIZE(sensor_2608_1960_30fps_30fps_regs),
	.set_size      = NULL,
	},

};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_CSI2_DPHY;

	cfg->flags = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0;

	return 0;
}



static int sensor_g_ctrl(struct v4l2_ctrl *ctrl)
{

	struct sensor_info *info =
			container_of(ctrl->handler,
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
	struct sensor_info *info =
			container_of(ctrl->handler,
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



static int sensor_reg_init(struct sensor_info *info)
{
	int ret;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	ret = s5k4e6yx_write_array(sd, sensor_default_regs,
				 ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_dbg("write sensor_default_regs error\n");
		return ret;
	}

	sensor_dbg("sensor_reg_init\n");

	s5k4e6yx_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs) {
		s5k4e6yx_write_array(sd, wsize->regs, wsize->regs_size);
		sensor_dbg("sensor_reg_init: wsize->regs\n");
	}
	if (wsize->set_size) {
		sensor_dbg("sensor_reg_init: wsize->set_size\n");
		wsize->set_size(sd);
	}
	info->width = wsize->width;
	info->height = wsize->height;
	s5k4e6yx_sensor_vts = wsize->vts;
	s5k4e6yx_sensor_hts = wsize->hts;
	sensor_dbg("sensor_reg_init: end\n");

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
		.data_width = CCI_BITS_16,
	}, {
		.name = SENSOR_NAME_2,
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_16,
	}
};


static int sensor_init_controls(struct v4l2_subdev *sd,
			const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	struct v4l2_ctrl *ctrl;
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 2);

	v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 1600,
			      256 * 1600, 1, 1 * 1600);
	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 0,
			      65536 * 16, 1, 0);
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
		cci_dev_probe_helper(sd, client,
			&sensor_ops, &cci_drv[sensor_dev_id++]);
	}

	sensor_init_controls(sd, &sensor_ctrl_ops);

	mutex_init(&info->lock);

	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->combo_mode = CMB_PHYA_OFFSET1 | MIPI_NORMAL_MODE;
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
