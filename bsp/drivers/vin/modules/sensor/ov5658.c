/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for ov5658 cameras.
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
MODULE_DESCRIPTION("A low-level driver for ov5658 sensors");
MODULE_LICENSE("GPL");

#define MCLK              (24 * 1000 * 1000)
#define V4L2_IDENT_SENSOR 0x5656

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The ov5658 sits on i2c with ID 0x6c
 */
#define I2C_ADDR 0x6c

#define SENSOR_NAME "ov5658"

struct cfg_array { /* coming later */
	struct regval_list *regs;
	int size;
};

/*
 * The default register settings
 *
 */
static struct regval_list sensor_default_regs[] = {
	{0x0103, 0x01},
	{0x3210, 0x43},
	{0x3001, 0x0e},
	{0x3002, 0xc0},
	{0x3011, 0x21},	/* 2 lanes */
	{0x3012, 0x0a},	/* 10-bits */
	{0x3013, 0x50},
	{0x3015, 0x09},
	{0x3018, 0xf0},
	{0x3021, 0x40},
	{0x3500, 0x00},	/* log exp */
	{0x3501, 0x9b},
	{0x3502, 0x00},
	{0x3503, 0x07},
	{0x3505, 0x00},	/* manual sensor gain */
	{0x3506, 0x70},	/* short exp */
	{0x3507, 0x02},
	{0x3508, 0x00},
	{0x3509, 0x10},
	{0x350a, 0x00},
	{0x350b, 0x80},

	/* clk */
	{0x3600, 0x69},	/* multiplier */
	{0x3602, 0x3c},	/* 24/3=8M, 8M x 105 = 840M, 840M/4/2=105M(pclk) */
	{0x3605, 0x14},
	{0x3606, 0x09},

	{0x3612, 0x04},
	{0x3613, 0x66},
	{0x3614, 0x39},
	{0x3615, 0x33},
	{0x3616, 0x46},
	{0x361a, 0x0a},
	{0x361c, 0x76},
	{0x3620, 0x40},
	{0x3640, 0x03},
	{0x3641, 0x60},
	{0x3642, 0x00},
	{0x3643, 0x90},
	{0x3660, 0x04},
	{0x3665, 0x00},
	{0x3666, 0x20},
	{0x3667, 0x00},
	{0x366a, 0x80},
	{0x3680, 0xe0},
	{0x3692, 0x80},
	{0x3700, 0x42},
	{0x3701, 0x14},
	{0x3702, 0xe8},
	{0x3703, 0x20},
	{0x3704, 0x5e},
	{0x3705, 0x02},
	{0x3708, 0xe3},
	{0x3709, 0xc3},
	{0x370a, 0x00},
	{0x370b, 0x20},
	{0x370c, 0x0c},
	{0x370d, 0x11},
	{0x370e, 0x00},
	{0x370f, 0x40},
	{0x3710, 0x00},
	{0x3715, 0x09},
	{0x371a, 0x04},
	{0x371b, 0x05},
	{0x371c, 0x01},
	{0x371e, 0xa1},
	{0x371f, 0x18},
	{0x3721, 0x00},
	{0x3726, 0x00},
	{0x372a, 0x01},
	{0x3730, 0x10},
	{0x3738, 0x22},
	{0x3739, 0xe5},
	{0x373a, 0x50},
	{0x373b, 0x02},
	{0x373c, 0x2c},
	{0x373f, 0x02},
	{0x3740, 0x42},
	{0x3741, 0x02},
	{0x3743, 0x01},
	{0x3744, 0x02},
	{0x3747, 0x00},
	{0x3754, 0xc0},
	{0x3755, 0x07},
	{0x3756, 0x1a},
	{0x3759, 0x0f},
	{0x375c, 0x04},
	{0x3767, 0x00},
	{0x376b, 0x44},
	{0x377b, 0x44},
	{0x377c, 0x30},
	{0x377e, 0x30},
	{0x377f, 0x08},
	{0x3781, 0x0c},
	{0x3785, 0x1e},
	{0x378f, 0xf5},
	{0x3791, 0xb0},
	{0x379c, 0x0c},
	{0x379d, 0x20},
	{0x379e, 0x00},
	{0x3796, 0x72},
	{0x379a, 0x07},
	{0x379b, 0xb0},
	{0x37c5, 0x00},
	{0x37c6, 0x00},
	{0x37c7, 0x00},
	{0x37c9, 0x00},
	{0x37ca, 0x00},
	{0x37cb, 0x00},
	{0x37cc, 0x00},
	{0x37cd, 0x00},
	{0x37ce, 0x01},
	{0x37cf, 0x00},
	{0x37d1, 0x00},
	{0x37de, 0x00},
	{0x37df, 0x00},

	{0x3823, 0x00},
	{0x3824, 0x00},
	{0x3825, 0x00},
	{0x3826, 0x00},
	{0x3827, 0x00},
	{0x3829, 0x0b},
	{0x382a, 0x04},
	{0x382c, 0x34},
	{0x382d, 0x00},
	{0x3a04, 0x06},
	{0x3a05, 0x14},
	{0x3a06, 0x00},
	{0x3a07, 0xfe},
	{0x3b00, 0x00},
	{0x3b02, 0x00},
	{0x3b03, 0x00},
	{0x3b04, 0x00},
	{0x3b05, 0x00},
	{0x4000, 0x18},
	{0x4001, 0x04},
	{0x4002, 0x45},
	{0x4004, 0x04},
	{0x4005, 0x18},
	{0x4006, 0x20},
	{0x4007, 0x98},
	{0x4008, 0x24},
	{0x4009, 0x12},	/* blc level, 0~255 */
	{0x400c, 0x00},
	{0x400d, 0x00},
	{0x404e, 0x37},
	{0x404f, 0x8f},
	{0x4058, 0x00},
	{0x4100, 0x50},
	{0x4101, 0x34},
	{0x4102, 0x34},
	{0x4104, 0xde},
	{0x4300, 0xff},
	{0x4307, 0x30},
	{0x4311, 0x04},	/* vsync width, 4311~4316 */
	{0x4315, 0x01},
	{0x4501, 0x08},
	{0x4502, 0x08},
	{0x4800, 0x24},	/* lp */
	{0x4816, 0x52},
	{0x481f, 0x30},
	{0x4826, 0x28},
	{0x4837, 0x0d},
	{0x4a00, 0xaa},
	{0x4a02, 0x00},
	{0x4a03, 0x01},
	{0x4a05, 0x40},
	{0x4a0a, 0x88},
	{0x5000, 0x06},
	{0x5001, 0x01},
	{0x5002, 0x00},
	{0x5003, 0x20},
	{0x5013, 0x00},
	{0x501f, 0x00},
	{0x5043, 0x48},
	{0x5780, 0x1c},
	{0x5786, 0x20},
	{0x5788, 0x18},
	{0x578a, 0x04},
	{0x578b, 0x02},
	{0x578c, 0x02},
	{0x578e, 0x06},
	{0x578f, 0x02},
	{0x5790, 0x02},
	{0x5791, 0xff},
	{0x5e00, 0x00},
	{0x5e10, 0x0c},

	{0x0100, 0x01},

	{REG_DLY, 0x08},

	{0x5800, 0x22},
	{0x5801, 0x11},
	{0x5802, 0x0d},
	{0x5803, 0x0d},
	{0x5804, 0x12},
	{0x5805, 0x26},
	{0x5806, 0x09},
	{0x5807, 0x07},
	{0x5808, 0x05},
	{0x5809, 0x05},
	{0x580a, 0x07},
	{0x580b, 0x0a},
	{0x580c, 0x07},
	{0x580d, 0x02},
	{0x580e, 0x00},
	{0x580f, 0x00},
	{0x5810, 0x03},
	{0x5811, 0x07},
	{0x5812, 0x06},
	{0x5813, 0x02},
	{0x5814, 0x00},
	{0x5815, 0x00},
	{0x5816, 0x03},
	{0x5817, 0x07},
	{0x5818, 0x09},
	{0x5819, 0x05},
	{0x581a, 0x04},
	{0x581b, 0x04},
	{0x581c, 0x07},
	{0x581d, 0x09},
	{0x581e, 0x1d},
	{0x581f, 0x0e},
	{0x5820, 0x0b},
	{0x5821, 0x0b},
	{0x5822, 0x0f},
	{0x5823, 0x1e},
	{0x5824, 0x59},
	{0x5825, 0x46},
	{0x5826, 0x37},
	{0x5827, 0x36},
	{0x5828, 0x45},
	{0x5829, 0x39},
	{0x582a, 0x46},
	{0x582b, 0x44},
	{0x582c, 0x45},
	{0x582d, 0x28},
	{0x582e, 0x38},
	{0x582f, 0x52},
	{0x5830, 0x60},
	{0x5831, 0x51},
	{0x5832, 0x26},
	{0x5833, 0x38},
	{0x5834, 0x43},
	{0x5835, 0x42},
	{0x5836, 0x34},
	{0x5837, 0x18},
	{0x5838, 0x05},
	{0x5839, 0x27},
	{0x583a, 0x27},
	{0x583b, 0x27},
	{0x583c, 0x0a},
	{0x583d, 0xbf},
	{0x5842, 0x01},
	{0x5843, 0x2b},
	{0x5844, 0x01},
	{0x5845, 0x92},
	{0x5846, 0x01},
	{0x5847, 0x8f},
	{0x5848, 0x01},
	{0x5849, 0x0c},
};

/*
 * Xclk 24Mhz
 * Pclk 84Mhz
 * linelength 2816(0xb00)
 * framelength 1984(0x7c0)
 * grabwindow_width 2592
 * grabwindow_height 1944
 * max_framerate 15fps
 * mipi_datarate per lane 420Mbps
 */
static struct regval_list sensor_2592x1944_regs[] = {
	{0x0100, 0x00},
	{0x3800, 0x00}, /* xstart = 0 */
	{0x3801, 0x00}, /* xstart */
	{0x3802, 0x00}, /* ystart = 0 */
	{0x3803, 0x00}, /* ystart */
	{0x3804, 0x0a}, /* xend = 2623 */
	{0x3805, 0x3f}, /* xend */
	{0x3806, 0x07}, /* yend = 1955 */
	{0x3807, 0xa3}, /* yend */

	{0x3808, 0x07}, /* x output size = 1920 */
	{0x3809, 0x80}, /* x output size */
	{0x380a, 0x06}, /* y output size = 1600 */
	{0x380b, 0x40}, /* y output size */

	{0x380c, 0x0c}, /* hts = 3224 */
	{0x380d, 0x98}, /* hts */
	{0x380e, 0x07}, /* vts = 1984 */
	{0x380f, 0xc0}, /* vts */
	{0x3810, 0x01}, /* isp x win = 436 */
	{0x3811, 0x50}, /* isp x win */
	{0x3812, 0x00}, /* isp y win = 180 */
	{0x3813, 0xb4}, /* isp y win */
	{0x3814, 0x11}, /* x inc */
	{0x3815, 0x11}, /* y inc */
	{0x3820, 0x10}, /* flip off, v bin off */
	{0x3821, 0x1e}, /* mirror on, v bin off */
	{0x4009, 0x12},
	{0x4826, 0x38}, /* gain = 4x */
	{0x4837, 0x0a}, /* MIPI global timing */

	{0x3500, 0x00},
	{0x3501, 0x6f},
	{0x3502, 0xb0},
	{0x350b, 0xF6},

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

static unsigned int last_exp;
static int ov5658_sensor_vts;
static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	data_type explow, expmid, exphigh;
	struct sensor_info *info = to_state(sd);

	if (exp_val > ((ov5658_sensor_vts - 16) << 4))
		exp_val = (ov5658_sensor_vts - 16) << 4;
	if (exp_val < 16)
		exp_val = 16;

	if (last_exp == exp_val)
		return 0;

	exphigh = (unsigned char)((exp_val >> 16) & 0x0F);
	expmid = (unsigned char)((exp_val >> 8) & 0xFF);
	explow = (unsigned char)(exp_val & 0xFF);

	sensor_write(sd, 0x3500, exphigh);
	sensor_write(sd, 0x3501, expmid);
	sensor_write(sd, 0x3502, explow);

	sensor_dbg("sensor_s_exp info->exp 0x%x\n", exp_val);
	info->exp = exp_val;

	last_exp = info->exp;

	return 0;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->gain;
	sensor_dbg("sensor_get_gain = %d\n", info->gain);

	return 0;
}

static unsigned int last_gain;
static int sensor_s_gain(struct v4l2_subdev *sd, unsigned int gain_val)
{
	data_type gainhigh, gainlow;
	struct sensor_info *info = to_state(sd);

	if (gain_val < 1 * 16)
		gain_val = 16;
	if (gain_val > 16 * 16 - 1)
		gain_val = 16 * 16 - 1;

	if (last_gain == gain_val)
		return 0;

	gainhigh = (unsigned char)((gain_val >> 8) & 0xFF);
	gainlow = (unsigned char)(gain_val & 0xFF);

	/* sensor_write(sd, 0x350a, gainhigh); */
	sensor_write(sd, 0x350b, gainlow);

	sensor_print("sensor_s_gain info->gain %d\n", gain_val);
	info->gain = gain_val;

	last_gain = info->gain;

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
		cci_lock(sd);

		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);

		vin_set_mclk(sd, ON);
		vin_set_mclk_freq(sd, MCLK);

		vin_set_pmu_channel(sd, IOVDD, ON);
		usleep_range(1000, 1100);	/* T0 */
		vin_set_pmu_channel(sd, AVDD, ON);

		usleep_range(4000, 5100);	/* T2 */

		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(8000, 10000);

		cci_unlock(sd);
		break;
	case PWR_OFF:
		cci_lock(sd);

		vin_set_mclk(sd, OFF);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);

		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);

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

	sensor_read(sd, 0x300A, &rdval);
	if (rdval != (V4L2_IDENT_SENSOR >> 8))
		return -ENODEV;

	sensor_read(sd, 0x300B, &rdval);
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
	info->width = 1920;
	info->height = 1600;
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
		.width      = 1920,
		.height     = 1600,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 3224,
		.vts        = 1984,
		.pclk       = 192 * 1000 * 1000,
		.mipi_bps   = 840 * 1000 * 1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (1984) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 16 << 4,
		.regs       = sensor_2592x1944_regs,
		.regs_size  = ARRAY_SIZE(sensor_2592x1944_regs),
		.set_size   = NULL,
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
	ov5658_sensor_vts = wsize->vts;
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

	last_exp = 0;
	last_gain = 0;

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
