/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for os05a20 Raw cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *           Liang WeiJie <liangweijie@allwinnertech.com>
 *           FuMing Cao <caofuming@allwinnertech.com>
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
MODULE_DESCRIPTION("A low-level driver for os05a20 sensors");
MODULE_LICENSE("GPL");

#define MCLK (24 * 1000 * 1000)
#define V4L2_IDENT_SENSOR 0x530541

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The os05a20 i2c address
 */
#define I2C_ADDR 0x6C

#define SENSOR_NUM 0x2
#define SENSOR_NAME "os05a20_mipi"
#define SENSOR_NAME_2 "os05a20_mipi_2"

#define HDR_RATIO 16

/*
 * The default register settings
 */
static struct regval_list sensor_default_regs[] = {

};

static struct regval_list sensor_2688_1944_Linear10_15fps_regs[] = {
	{0x0100, 0x00},
	{0x0103, 0x01},
	{0x0303, 0x01},
	{0x0305, 0x44},
	{0x0306, 0x00},
	{0x0307, 0x00},
	{0x0308, 0x03},
	{0x0309, 0x04},
	{0x030c, 0x01},
	{0x0322, 0x01},
	{0x032a, 0x00},
	{0x031e, 0x09},
	{0x0325, 0x48},
	{0x0328, 0x07},
	{0x300d, 0x11},
	{0x300e, 0x11},
	{0x300f, 0x11},
	{0x3010, 0x01},
	{0x3012, 0x41},
	{0x3016, 0xf0},
	{0x3018, 0xf0},
	{0x3028, 0xf0},
	{0x301e, 0x98},
	{0x3010, 0x04},
	{0x3011, 0x06},
	{0x3031, 0xa9},
	{0x3103, 0x48},
	{0x3104, 0x01},
	{0x3106, 0x10},
	{0x3400, 0x04},
	{0x3025, 0x03},
	{0x3425, 0x01},
	{0x3428, 0x01},
	{0x3406, 0x08},
	{0x3408, 0x03},
	{0x3501, 0x09},
	{0x3502, 0xa0},
	{0x3505, 0x83},
	{0x3508, 0x00},
	{0x3509, 0x80},
	{0x350a, 0x04},
	{0x350b, 0x00},
	{0x350c, 0x00},
	{0x350d, 0x80},
	{0x350e, 0x04},
	{0x350f, 0x00},
	{0x3600, 0x00},
	{0x3626, 0xff},
	{0x3605, 0x50},
	{0x3609, 0xb5},
	{0x3610, 0x69},
	{0x360c, 0x01},
	{0x3628, 0xa4},
	{0x3629, 0x6a},
	{0x362d, 0x10},
	{0x3660, 0x43},
	{0x3661, 0x06},
	{0x3662, 0x00},
	{0x3663, 0x28},
	{0x3664, 0x0d},
	{0x366a, 0x38},
	{0x366b, 0xa0},
	{0x366d, 0x00},
	{0x366e, 0x00},
	{0x3680, 0x00},
	{0x36c0, 0x00},
	{0x3621, 0x81},
	{0x3634, 0x31},
	{0x3620, 0x00},
	{0x3622, 0x00},
	{0x362a, 0xd0},
	{0x362e, 0x8c},
	{0x362f, 0x98},
	{0x3630, 0xb0},
	{0x3631, 0xd7},
	{0x3701, 0x0f},
	{0x3737, 0x02},
	{0x3740, 0x18},
	{0x3741, 0x04},
	{0x373c, 0x0f},
	{0x373b, 0x02},
	{0x3705, 0x00},
	{0x3706, 0x50},
	{0x370a, 0x00},
	{0x370b, 0xe4},
	{0x3709, 0x4a},
	{0x3714, 0x21},
	{0x371c, 0x00},
	{0x371d, 0x08},
	{0x375e, 0x0e},
	{0x3760, 0x13},
	{0x3776, 0x10},
	{0x3781, 0x02},
	{0x3782, 0x04},
	{0x3783, 0x02},
	{0x3784, 0x08},
	{0x3785, 0x08},
	{0x3788, 0x01},
	{0x3789, 0x01},
	{0x3797, 0x84},
	{0x3798, 0x01},
	{0x3799, 0x00},
	{0x3761, 0x02},
	{0x3762, 0x0d},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x0c},
	{0x3804, 0x0e},
	{0x3805, 0xff},
	{0x3806, 0x08},
	{0x3807, 0x6f},
	{0x3808, 0x0a},
	{0x3809, 0x80},
	{0x380a, 0x07},
	{0x380b, 0x98},
	{0x380c, 0x05},
	{0x380d, 0xA0},
	{0x380e, 0x09},
	{0x380f, 0xc0},
	{0x3813, 0x04},
	{0x3814, 0x01},
	{0x3815, 0x01},
	{0x3816, 0x01},
	{0x3817, 0x01},
	{0x381c, 0x00},
	{0x3820, 0x00},
	{0x3821, 0x04},
	{0x3823, 0x18},
	{0x3826, 0x00},
	{0x3827, 0x01},
	{0x3833, 0x00},
	{0x3832, 0x02},
	{0x383c, 0x48},
	{0x383d, 0xff},
	{0x3843, 0x20},
	{0x382d, 0x08},
	{0x3d85, 0x0b},
	{0x3d84, 0x40},
	{0x3d8c, 0x63},
	{0x3d8d, 0x00},
	{0x4000, 0x78},
	{0x4001, 0x2b},
	{0x4004, 0x00},
	{0x4005, 0x40},
	{0x4028, 0x2f},
	{0x400a, 0x01},
	{0x4010, 0x12},
	{0x4008, 0x02},
	{0x4009, 0x0d},
	{0x401a, 0x58},
	{0x4050, 0x00},
	{0x4051, 0x01},
	{0x4052, 0x00},
	{0x4053, 0x80},
	{0x4054, 0x00},
	{0x4055, 0x80},
	{0x4056, 0x00},
	{0x4057, 0x80},
	{0x4058, 0x00},
	{0x4059, 0x80},
	{0x430b, 0xff},
	{0x430c, 0xff},
	{0x430d, 0x00},
	{0x430e, 0x00},
	{0x4501, 0x18},
	{0x4502, 0x00},
	{0x4643, 0x00},
	{0x4640, 0x01},
	{0x4641, 0x04},
	{0x480e, 0x00},
	{0x4813, 0x00},
	{0x4815, 0x2b},
	{0x486e, 0x36},
	{0x486f, 0x84},
	{0x4860, 0x00},
	{0x4861, 0xa0},
	{0x484b, 0x05},
	{0x4850, 0x00},
	{0x4851, 0xaa},
	{0x4852, 0xff},
	{0x4853, 0x8a},
	{0x4854, 0x08},
	{0x4855, 0x30},
	{0x4800, 0x60},
	{0x4837, 0x1d},
	{0x484a, 0x3f},
	{0x5000, 0xc9},
	{0x5001, 0x43},
	{0x5002, 0x00},
	{0x5211, 0x03},
	{0x5291, 0x03},
	{0x520d, 0x0f},
	{0x520e, 0xfd},
	{0x520f, 0xa5},
	{0x5210, 0xa5},
	{0x528d, 0x0f},
	{0x528e, 0xfd},
	{0x528f, 0xa5},
	{0x5290, 0xa5},
	{0x5004, 0x40},
	{0x5005, 0x00},
	{0x5180, 0x00},
	{0x5181, 0x10},
	{0x5182, 0x0f},
	{0x5183, 0xff},
	{0x580b, 0x03},
	{0x4d00, 0x03},
	{0x4d01, 0xe9},
	{0x4d02, 0xba},
	{0x4d03, 0x66},
	{0x4d04, 0x46},
	{0x4d05, 0xa5},
	{0x3603, 0x3c},
	{0x3703, 0x26},
	{0x3709, 0x49},
	{0x3708, 0x2d},
	{0x3719, 0x1c},
	{0x371a, 0x06},
	{0x4000, 0x79},
	{0x4837, 0x1d},
	{0x0100, 0x01},
	{0x0100, 0x01},
	{0x0100, 0x01},
	{0x0100, 0x01},
};

static struct regval_list sensor_2688_1944_HDRVC10_15fps_regs[] = {
	{0x0100, 0x00},
	{0x0103, 0x01},
	{0x0303, 0x01},
	{0x0305, 0x27},
	{0x0306, 0x00},
	{0x0307, 0x00},
	{0x0308, 0x03},
	{0x0309, 0x04},
	{0x032a, 0x00},
	{0x031e, 0x09},
	{0x0325, 0x48},
	{0x0328, 0x07},
	{0x300d, 0x11},
	{0x300e, 0x11},
	{0x300f, 0x11},
	{0x3010, 0x01},
	{0x3012, 0x41},
	{0x3016, 0xf0},
	{0x3018, 0xf0},
	{0x3028, 0xf0},
	{0x301e, 0x98},
	{0x3010, 0x04},
	{0x3011, 0x06},
	{0x3031, 0xa9},
	{0x3103, 0x48},
	{0x3104, 0x01},
	{0x3106, 0x10},
	{0x3501, 0x08},
	{0x3502, 0x6f},
	{0x3505, 0x83},
	{0x3508, 0x00},
	{0x3509, 0x80},
	{0x350a, 0x04},
	{0x350b, 0x00},
	{0x350c, 0x00},
	{0x350d, 0x80},
	{0x350e, 0x04},
	{0x350f, 0x00},
	{0x3600, 0x00},
	{0x3626, 0xff},
	{0x3605, 0x50},
	{0x3609, 0xb5},
	{0x3610, 0x69},
	{0x360c, 0x01},
	{0x3628, 0xa4},
	{0x3629, 0x6a},
	{0x362d, 0x10},
	{0x3660, 0x42},
	{0x3661, 0x07},
	{0x3662, 0x00},
	{0x3663, 0x28},
	{0x3664, 0x0d},
	{0x366a, 0x38},
	{0x366b, 0xa0},
	{0x366d, 0x00},
	{0x366e, 0x00},
	{0x3680, 0x00},
	{0x36c0, 0x00},
	{0x3621, 0x81},
	{0x3634, 0x31},
	{0x3620, 0x00},
	{0x3622, 0x00},
	{0x362a, 0xd0},
	{0x362e, 0x8c},
	{0x362f, 0x98},
	{0x3630, 0xb0},
	{0x3631, 0xd7},
	{0x3701, 0x0f},
	{0x3737, 0x02},
	{0x3740, 0x18},
	{0x3741, 0x04},
	{0x373c, 0x0f},
	{0x373b, 0x02},
	{0x3705, 0x00},
	{0x3706, 0x50},
	{0x370a, 0x00},
	{0x370b, 0xe4},
	{0x3709, 0x4a},
	{0x3714, 0x21},
	{0x371c, 0x00},
	{0x371d, 0x08},
	{0x375e, 0x0e},
	{0x3760, 0x13},
	{0x3776, 0x10},
	{0x3781, 0x02},
	{0x3782, 0x04},
	{0x3783, 0x02},
	{0x3784, 0x08},
	{0x3785, 0x08},
	{0x3788, 0x01},
	{0x3789, 0x01},
	{0x3797, 0x84},
	{0x3798, 0x01},
	{0x3799, 0x00},
	{0x3761, 0x02},
	{0x3762, 0x0d},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x0c},
	{0x3804, 0x0e},
	{0x3805, 0xff},
	{0x3806, 0x08},
	{0x3807, 0x6f},
	{0x3808, 0x0a},
	{0x3809, 0x80},
	{0x380a, 0x07},
	{0x380b, 0x98},
	{0x380c, 0x04},
	{0x380d, 0xd0},
	{0x380e, 0x08},
	{0x380f, 0x8f},
	{0x3813, 0x04},
	{0x3814, 0x01},
	{0x3815, 0x01},
	{0x3816, 0x01},
	{0x3817, 0x01},
	{0x381c, 0x08},
	{0x3820, 0x00},
	{0x3821, 0x24},
	{0x3823, 0x08},
	{0x3826, 0x00},
	{0x3827, 0x01},
	{0x3833, 0x01},
	{0x3832, 0x02},
	{0x383c, 0x48},
	{0x383d, 0xff},
	{0x3843, 0x20},
	{0x382d, 0x08},
	{0x3d85, 0x0b},
	{0x3d84, 0x40},
	{0x3d8c, 0x63},
	{0x3d8d, 0x00},
	{0x4000, 0x78},
	{0x4001, 0x2b},
	{0x4004, 0x00},
	{0x4005, 0x40},
	{0x4028, 0x2f},
	{0x400a, 0x01},
	{0x4010, 0x12},
	{0x4008, 0x02},
	{0x4009, 0x0d},
	{0x401a, 0x58},
	{0x4050, 0x00},
	{0x4051, 0x01},
	{0x4052, 0x00},
	{0x4053, 0x80},
	{0x4054, 0x00},
	{0x4055, 0x80},
	{0x4056, 0x00},
	{0x4057, 0x80},
	{0x4058, 0x00},
	{0x4059, 0x80},
	{0x430b, 0xff},
	{0x430c, 0xff},
	{0x430d, 0x00},
	{0x430e, 0x00},
	{0x4501, 0x18},
	{0x4502, 0x00},
	{0x4643, 0x00},
	{0x4640, 0x01},
	{0x4641, 0x04},
	{0x480e, 0x04},
	{0x4813, 0x98},
	{0x4815, 0x2b},
	{0x486e, 0x36},
	{0x486f, 0x84},
	{0x4860, 0x00},
	{0x4861, 0xa0},
	{0x484b, 0x05},
	{0x4850, 0x00},
	{0x4851, 0xaa},
	{0x4852, 0xff},
	{0x4853, 0x8a},
	{0x4854, 0x08},
	{0x4855, 0x30},
	{0x4800, 0x60},
	{0x4837, 0x19},
	{0x484a, 0x3f},
	{0x5000, 0xc9},
	{0x5001, 0x43},
	{0x5002, 0x00},
	{0x5211, 0x03},
	{0x5291, 0x03},
	{0x520d, 0x0f},
	{0x520e, 0xfd},
	{0x520f, 0xa5},
	{0x5210, 0xa5},
	{0x528d, 0x0f},
	{0x528e, 0xfd},
	{0x528f, 0xa5},
	{0x5290, 0xa5},
	{0x5004, 0x40},
	{0x5005, 0x00},
	{0x5180, 0x00},
	{0x5181, 0x10},
	{0x5182, 0x0f},
	{0x5183, 0xff},
	{0x580b, 0x03},
	{0x4d00, 0x03},
	{0x4d01, 0xe9},
	{0x4d02, 0xba},
	{0x4d03, 0x66},
	{0x4d04, 0x46},
	{0x4d05, 0xa5},
	{0x3603, 0x3c},
	{0x3703, 0x26},
	{0x3709, 0x49},
	{0x3708, 0x2d},
	{0x3719, 0x1c},
	{0x371a, 0x06},
	{0x4000, 0x79},
	{0x380c, 0x04},
	{0x380d, 0xd0},
	{0x380e, 0x0b},
	{0x380f, 0x6a},
	{0x3501, 0x0a},
	{0x3502, 0x65},
	{0x3511, 0x00},
	{0x3512, 0x20},
	{0x0100, 0x01},
	{0x0100, 0x01},
	{0x0100, 0x01},
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

static int os05a20_sensor_vts;
static int os05a20_sensor_hts;

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	data_type explow, exphigh;
	data_type rdval;
	int exp_short = 0;
	int exp_val_tmp;
	unsigned int exp_short_h, exp_short_l;
	struct sensor_info *info = to_state(sd);

	exp_val_tmp = exp_val;
	if (exp_val > ((os05a20_sensor_vts - 8) << 4))
		exp_val = (os05a20_sensor_vts - 8) << 4;
	if (exp_val < (16 * 4))
		exp_val = 16 * 4;

	exp_val = (exp_val + 8) >> 4;

	exphigh = (unsigned char)((0xff00 & exp_val) >> 8);
	explow  = (unsigned char)((0x00ff & exp_val));

	sensor_write(sd, 0x3501, exphigh);
	sensor_write(sd, 0x3502, explow);

	if (info->isp_wdr_mode == ISP_DOL_WDR_MODE) {
		sensor_dbg("Sensor in WDR mode, HDR_RATIO = %d\n", HDR_RATIO);
		exp_short = exp_val / HDR_RATIO;

		exp_short_h = (unsigned char)((0xff00 & exp_short) >> 8);
		exp_short_l = (unsigned char)((0x00ff & exp_short));

		sensor_write(sd, 0x3511, exp_short_h);
		sensor_write(sd, 0x3512, exp_short_l);
	}
	info->exp = exp_val_tmp;
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
	data_type rdval;
	int digi_gain;
	int ana_gain;
	int gain_val_tmp;

	gain_val_tmp = gain_val;
	gain_val = gain_val << 3;

	if (gain_val <= (16 * 1 * 8))
		gain_val = 16 * 1 * 8;

	if (gain_val > (16 * 15 * 8)) {
		ana_gain = 16 * 15 * 8;
		digi_gain = gain_val / ana_gain;
		gain_val = ana_gain;

		sensor_write(sd, 0x350a, digi_gain << 2);
		sensor_write(sd, 0x350b, 0x00);

		if (info->isp_wdr_mode == ISP_DOL_WDR_MODE) {
			sensor_write(sd, 0x350e, digi_gain << 2);
			sensor_write(sd, 0x350f, 0x00);
		}
	}

	gainhigh = (unsigned char)((gain_val>>8)&0xff);
	gainlow = (unsigned char)(gain_val&0xff);

	sensor_write(sd, 0x3508, gainhigh);
	sensor_write(sd, 0x3509, gainlow);

	if (info->isp_wdr_mode == ISP_DOL_WDR_MODE) {
		sensor_write(sd, 0x350c, gainhigh);
		sensor_write(sd, 0x350d, gainlow);
	}

	info->gain = gain_val_tmp;
	return 0;
}

static int sensor_s_wb(struct v4l2_subdev *sd, int rgain, int bgain)
{
	if (rgain > 751)
		rgain = 751;
	if (bgain > 751)
		bgain = 751;

	sensor_write(sd, 0x0B90, rgain >> 8);
	sensor_write(sd, 0x0B91, rgain & 0xFF);
	sensor_write(sd, 0x0B92, bgain >> 8);
	sensor_write(sd, 0x0B93, bgain & 0xFF);

	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
				struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val, shutter, frame_length;
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);

	info->exp = exp_val;
	info->gain = gain_val;
	return 0;
}


/* long exp mode eg:
 *
 *   coarse_int_time = 60290; frame_length= 65535;
 *   times =8; VTPXCLK = 480Mhz; imx278_hts = 4976
 *
 *	EXP time = 60290 * 8 * 4976 / (120[Mhz] * 4 ) = 5 s
 */
static int fps_change_flag;

static int sensor_s_fps(struct v4l2_subdev *sd,
			struct sensor_fps *fps)
{
	unsigned int coarse_int_time, frame_length = 0;
	struct sensor_info *info = to_state(sd);
	struct sensor_win_size *wsize = info->current_wins;
	unsigned int times_reg, times, imx278_hts = 0, FRM_LINES = 0;

	if (fps->fps == 0)
		fps->fps = 30;

	/****************************/
	/* Early enter long exp mode; in case of quit before */
	if (fps->fps < 0) {
		times = 2;
		times_reg = 0x1;
		imx278_hts = wsize->hts;

		/* test: when fps = 10  && delay 150ms, will not quit before */
		coarse_int_time = wsize->pclk/imx278_hts/times/10;
		FRM_LINES = coarse_int_time;
		coarse_int_time -= 10;

		sensor_write(sd, 0x0100, 0x00);
		sensor_write(sd, 0x0104, 0x01);

		sensor_write(sd, 0x3002, times_reg);

		sensor_write(sd, 0x0202, (coarse_int_time >> 8));
		sensor_write(sd, 0x0203, (coarse_int_time & 0xff));

		sensor_write(sd, 0x0340, (FRM_LINES >> 8));
		sensor_write(sd, 0x0341, (FRM_LINES & 0xff));

		sensor_write(sd, 0x0342, (imx278_hts >> 8));
		sensor_write(sd, 0x0343, (imx278_hts & 0xff));

		sensor_write(sd, 0x0100, 0x01);
		sensor_write(sd, 0x0104, 0x00);

		usleep_range(150000, 200000);
	}

	/****************************/

	if (fps->fps < 0) {
		fps->fps = -fps->fps;

		if (fps->fps >= 1 && fps->fps <= 5) {
			times = 8;
			times_reg = 0x3;
			imx278_hts = wsize->hts;
		} else if (fps->fps <= 10) {
			times = 16;
			times_reg = 0x4;
			imx278_hts = wsize->hts;
		} else if (fps->fps <= 20) {
			times = 16;
			times_reg = 0x4;
			imx278_hts = wsize->hts * 2;
		} else {
			times = 16;
			times_reg = 0x4;
			imx278_hts = wsize->hts * 4;
		}

		coarse_int_time = wsize->pclk/imx278_hts/times * fps->fps;
		FRM_LINES = coarse_int_time;
		/* 0 <=  coarse_int_time  <= FRM_LINES - 10 */
		coarse_int_time -= 20;
		fps_change_flag = 1;
	} else {
		coarse_int_time = wsize->pclk/wsize->hts/fps->fps;
		if (coarse_int_time	> os05a20_sensor_vts - 4)
			frame_length = coarse_int_time + 4;
		else
			frame_length = os05a20_sensor_vts;
		fps_change_flag = 0;
	}

	/* sensor reg standby */
	sensor_write(sd, 0x0100, 0x00);
	/* grouped hold function */
	sensor_write(sd, 0x0104, 0x01);

	if (fps_change_flag == 1) {
		/* open long exp mode */
		sensor_write(sd, 0x3002, times_reg);

		sensor_write(sd, 0x0202, (coarse_int_time >> 8));
		sensor_write(sd, 0x0203, (coarse_int_time & 0xff));

		sensor_write(sd, 0x0340, (FRM_LINES >> 8));
		sensor_write(sd, 0x0341, (FRM_LINES & 0xff));

		sensor_write(sd, 0x0342, (imx278_hts >> 8));
		sensor_write(sd, 0x0343, (imx278_hts & 0xff));
	} else {
		/* close long exp mode */
		sensor_write(sd, 0x3002, 0x00);

		sensor_write(sd, 0x0340, (frame_length >> 8));
		sensor_write(sd, 0x0341, (frame_length & 0xff));

		sensor_write(sd, 0x0202, (coarse_int_time >> 8));
		sensor_write(sd, 0x0203, (coarse_int_time & 0xff));

		sensor_write(sd, 0x0342, (wsize->hts >> 8));
		sensor_write(sd, 0x0343, (wsize->hts & 0xff));
	}
	/* must release */
	sensor_write(sd, 0x0104, 0x00);
	sensor_write(sd, 0x0100, 0x01);

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
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(1000, 1200);
		vin_set_mclk(sd, ON);
		usleep_range(1000, 1200);
		vin_set_mclk_freq(sd, MCLK);
		usleep_range(3000, 3200);
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
	data_type rdval = 0;

	sensor_read(sd, 0x300A, &rdval);
	if (rdval != 0x53)
		return -ENODEV;
	sensor_print("sensor_detect (0x300A, 0x%02x)\n", rdval);

	sensor_read(sd, 0x300B, &rdval);
	if (rdval != 0x05)
		return -ENODEV;
	sensor_print("sensor_detect (0x300B, 0x%02x)\n", rdval);

	sensor_read(sd, 0x300C, &rdval);
	if (rdval != 0x41)
		return -ENODEV;
	sensor_print("sensor_detect (0x300C, 0x%02x)\n", rdval);
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
	info->width = 4208;
	info->height = 3120;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->exp = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30; /* 30fps */

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
		.width      = 2688,
		.height     = 1944,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 1440,
		.vts        = 2496,
		.pclk       = 29 * 1000 * 1000,
		.mipi_bps   = 800 * 1000 * 1000,
		.fps_fixed  = 15,
		.bin_factor = 1,
		.intg_min   = 4 << 4,
		.intg_max   = 2488 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 225 << 4,
		.regs       = sensor_2688_1944_Linear10_15fps_regs,
		.regs_size  = ARRAY_SIZE(sensor_2688_1944_Linear10_15fps_regs),
		.set_size   = NULL,
	},

	{
		.width      = 2688,
		.height     = 1944,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 1232,
		.vts        = 2922,
		.pclk       = 25 * 1000 * 1000,
		.mipi_bps   = 800 * 1000 * 1000,
		.fps_fixed  = 15,
		.bin_factor = 1,
		.if_mode    = MIPI_VC_WDR_MODE,
		.wdr_mode   = ISP_DOL_WDR_MODE,
		.intg_min   = 4 << 4,
		.intg_max   = 2914 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 225 << 4,
		.regs       = sensor_2688_1944_HDRVC10_15fps_regs,
		.regs_size  = ARRAY_SIZE(sensor_2688_1944_HDRVC10_15fps_regs),
		.set_size   = NULL,
	},

};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	struct sensor_info *info = to_state(sd);
	cfg->type = V4L2_MBUS_CSI2_DPHY;

	if (info->isp_wdr_mode == ISP_DOL_WDR_MODE)
		cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0 | V4L2_MBUS_CSI2_CHANNEL_1;
	else
		cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;

	return 0;
}

static int sensor_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *info = container_of(ctrl->handler, struct sensor_info, handler);
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
	struct sensor_info *info = container_of(ctrl->handler, struct sensor_info, handler);
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
	struct sensor_exp_gain exp_gain;

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
	os05a20_sensor_vts = wsize->vts;
	os05a20_sensor_hts = wsize->hts;

	exp_gain.exp_val = 16 * 1024;  /* 1024 lines */
	exp_gain.gain_val = 24;  /* 1.5x */
	sensor_s_exp_gain(sd, &exp_gain);

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
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET2 | MIPI_NORMAL_MODE;
	info->time_hs = 0x30;
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
