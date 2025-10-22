/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for imx386 Raw cameras.
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

MODULE_AUTHOR("lwj");
MODULE_DESCRIPTION("A low-level driver for MN169 sensors");
MODULE_LICENSE("GPL");

#define MCLK              (12*1000*1000)
#define V4L2_IDENT_SENSOR 0x0169

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The imx386 i2c address
 */
#define I2C_ADDR 0x20

#define SENSOR_NUM 0x2
#define SENSOR_NAME "MN169_mipi"
#define SENSOR_NAME_2 "MN169_mipi_2"


#define  PIC_OFFSET_H  (16*12)  /* 16 */
#define  PIC_OFFSET_V  (16*12*3/4)
#define  VIDEO_OFFSET_H  (16*8)  /* 10 */
#define  VIDEO_OFFSET_V  (16*8*9/16)


/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {
	{0x0100, 0x00},
	{0x0103, 0x01},
	{0xFFFF, 0x01},
	{0x0136, 0x18},
	{0x0137, 0x00},

	{0x3A7D, 0x00},
	{0x3A7E, 0x02},
	{0x3A7F, 0x05},

	{0x3100, 0x00},
	{0x3101, 0x40},
	{0x3102, 0x00},
	{0x3103, 0x10},
	{0x3104, 0x01},
	{0x3105, 0xE8},
	{0x3106, 0x01},
	{0x3107, 0xF0},
	{0x3150, 0x04},
	{0x3151, 0x03},
	{0x3152, 0x02},
	{0x3153, 0x01},
	{0x5A86, 0x00},
	{0x5A87, 0x82},
	{0x5D1A, 0x00},
	{0x5D95, 0x02},
	{0x5E1B, 0x00},
	{0x5F5A, 0x00},
	{0x5F5B, 0x04},
	{0x682C, 0x31},
	{0x6831, 0x31},
	{0x6835, 0x0E},
	{0x6836, 0x31},
	{0x6838, 0x30},
	{0x683A, 0x06},
	{0x683B, 0x33},
	{0x683D, 0x30},
	{0x6842, 0x31},
	{0x6844, 0x31},
	{0x6847, 0x31},
	{0x6849, 0x31},
	{0x684D, 0x0E},
	{0x684E, 0x32},
	{0x6850, 0x31},
	{0x6852, 0x06},
	{0x6853, 0x33},
	{0x6855, 0x31},
	{0x685A, 0x32},
	{0x685C, 0x33},
	{0x685F, 0x31},
	{0x6861, 0x33},
	{0x6865, 0x0D},
	{0x6866, 0x33},
	{0x6868, 0x31},
	{0x686B, 0x34},
	{0x686D, 0x31},
	{0x6872, 0x32},
	{0x6877, 0x33},
	{0x7FF0, 0x01},
	{0x7FF4, 0x08},
	{0x7FF5, 0x3C},
	{0x7FFA, 0x01},
	{0x7FFD, 0x00},
	{0x831E, 0x00},
	{0x831F, 0x00},
	{0x9301, 0xBD},
	{0x9B94, 0x03},
	{0x9B95, 0x00},
	{0x9B96, 0x08},
	{0x9B97, 0x00},
	{0x9B98, 0x0A},
	{0x9B99, 0x00},
	{0x9BA7, 0x18},
	{0x9BA8, 0x18},
	{0x9D04, 0x08},
	{0x9D50, 0x8C},
	{0x9D51, 0x64},
	{0x9D52, 0x50},
	{0x9E31, 0x04},
	{0x9E32, 0x04},
	{0x9E33, 0x04},
	{0x9E34, 0x04},
	{0xA200, 0x00},
	{0xA201, 0x0A},
	{0xA202, 0x00},
	{0xA203, 0x0A},
	{0xA204, 0x00},
	{0xA205, 0x0A},
	{0xA206, 0x01},
	{0xA207, 0xC0},
	{0xA208, 0x00},
	{0xA209, 0xC0},
	{0xA20C, 0x00},
	{0xA20D, 0x0A},
	{0xA20E, 0x00},
	{0xA20F, 0x0A},
	{0xA210, 0x00},
	{0xA211, 0x0A},
	{0xA212, 0x01},
	{0xA213, 0xC0},
	{0xA214, 0x00},
	{0xA215, 0xC0},
	{0xA300, 0x00},
	{0xA301, 0x0A},
	{0xA302, 0x00},
	{0xA303, 0x0A},
	{0xA304, 0x00},
	{0xA305, 0x0A},
	{0xA306, 0x01},
	{0xA307, 0xC0},
	{0xA308, 0x00},
	{0xA309, 0xC0},
	{0xA30C, 0x00},
	{0xA30D, 0x0A},
	{0xA30E, 0x00},
	{0xA30F, 0x0A},
	{0xA310, 0x00},
	{0xA311, 0x0A},
	{0xA312, 0x01},
	{0xA313, 0xC0},
	{0xA314, 0x00},
	{0xA315, 0xC0},
	{0xBC19, 0x01},
	{0xBC1C, 0x0A},
	{0x300b, 0x01},
};

static struct regval_list sensor_1940p30_regs[] = {

	{0x0136, 0x18},
	{0x0137, 0x00},
	{0x3C7D, 0x28},
	{0x3C7E, 0x01},
	{0x3C7F, 0x08},
	{0x3F02, 0x02},
	{0x3F22, 0x01},
	{0x3F7F, 0x01},
	{0x4421, 0x04},
	{0x4430, 0x05},
	{0x4431, 0xDC},
	{0x5222, 0x02},
	{0x56B7, 0x74},
	{0x6204, 0xC6},
	{0x620E, 0x27},
	{0x6210, 0x69},
	{0x6211, 0xD6},
	{0x6213, 0x01},
	{0x6215, 0x5A},
	{0x6216, 0x75},
	{0x6218, 0x5A},
	{0x6219, 0x75},
	{0x6220, 0x06},
	{0x6222, 0x0C},
	{0x6225, 0x19},
	{0x6228, 0x32},
	{0x6229, 0x70},
	{0x622B, 0x64},
	{0x622E, 0xB0},
	{0x6231, 0x71},
	{0x6234, 0x06},
	{0x6236, 0x46},
	{0x6237, 0x46},
	{0x6239, 0x0C},
	{0x623C, 0x19},
	{0x623F, 0x32},
	{0x6240, 0x71},
	{0x6242, 0x64},
	{0x6243, 0x44},
	{0x6245, 0xB0},
	{0x6246, 0xA8},
	{0x6248, 0x71},
	{0x624B, 0x06},
	{0x624D, 0x46},
	{0x625C, 0xC9},
	{0x625F, 0x92},
	{0x6262, 0x26},
	{0x6264, 0x46},
	{0x6265, 0x46},
	{0x6267, 0x0C},
	{0x626A, 0x19},
	{0x626D, 0x32},
	{0x626E, 0x72},
	{0x6270, 0x64},
	{0x6271, 0x68},
	{0x6273, 0xC8},
	{0x6276, 0x91},
	{0x6279, 0x27},
	{0x627B, 0x46},
	{0x627C, 0x55},
	{0x627F, 0x95},
	{0x6282, 0x84},
	{0x6283, 0x40},
	{0x6284, 0x00},
	{0x6285, 0x00},
	{0x6286, 0x08},
	{0x6287, 0xC0},
	{0x6288, 0x00},
	{0x6289, 0x00},
	{0x628A, 0x1B},
	{0x628B, 0x80},
	{0x628C, 0x20},
	{0x628E, 0x35},
	{0x628F, 0x00},
	{0x6290, 0x50},
	{0x6291, 0x00},
	{0x6292, 0x14},
	{0x6293, 0x00},
	{0x6294, 0x00},
	{0x6296, 0x54},
	{0x6297, 0x00},
	{0x6298, 0x00},
	{0x6299, 0x01},
	{0x629A, 0x10},
	{0x629B, 0x01},
	{0x629C, 0x00},
	{0x629D, 0x03},
	{0x629E, 0x50},
	{0x629F, 0x05},
	{0x62A0, 0x00},
	{0x62B1, 0x00},
	{0x62B2, 0x00},
	{0x62B3, 0x00},
	{0x62B5, 0x00},
	{0x62B6, 0x00},
	{0x62B7, 0x00},
	{0x62B8, 0x00},
	{0x62B9, 0x00},
	{0x62BA, 0x00},
	{0x62BB, 0x00},
	{0x62BC, 0x00},
	{0x62BD, 0x00},
	{0x62BE, 0x00},
	{0x62BF, 0x00},
	{0x62D0, 0x0C},
	{0x62D1, 0x00},
	{0x62D2, 0x00},
	{0x62D4, 0x40},
	{0x62D5, 0x00},
	{0x62D6, 0x00},
	{0x62D7, 0x00},
	{0x62D8, 0xD8},
	{0x62D9, 0x00},
	{0x62DA, 0x00},
	{0x62DB, 0x02},
	{0x62DC, 0xB0},
	{0x62DD, 0x03},
	{0x62DE, 0x00},
	{0x62EF, 0x14},
	{0x62F0, 0x00},
	{0x62F1, 0x00},
	{0x62F3, 0x58},
	{0x62F4, 0x00},
	{0x62F5, 0x00},
	{0x62F6, 0x01},
	{0x62F7, 0x20},
	{0x62F8, 0x00},
	{0x62F9, 0x00},
	{0x62FA, 0x03},
	{0x62FB, 0x80},
	{0x62FC, 0x00},
	{0x62FD, 0x00},
	{0x62FE, 0x04},
	{0x62FF, 0x60},
	{0x6300, 0x04},
	{0x6301, 0x00},
	{0x6302, 0x09},
	{0x6303, 0x00},
	{0x6304, 0x0C},
	{0x6305, 0x00},
	{0x6306, 0x1B},
	{0x6307, 0x80},
	{0x6308, 0x30},
	{0x630A, 0x38},
	{0x630B, 0x00},
	{0x630C, 0x60},
	{0x630E, 0x14},
	{0x630F, 0x00},
	{0x6310, 0x00},
	{0x6312, 0x58},
	{0x6313, 0x00},
	{0x6314, 0x00},
	{0x6315, 0x01},
	{0x6316, 0x18},
	{0x6317, 0x01},
	{0x6318, 0x80},
	{0x6319, 0x03},
	{0x631A, 0x60},
	{0x631B, 0x06},
	{0x631C, 0x00},
	{0x632D, 0x0E},
	{0x632E, 0x00},
	{0x632F, 0x00},
	{0x6331, 0x44},
	{0x6332, 0x00},
	{0x6333, 0x00},
	{0x6334, 0x00},
	{0x6335, 0xE8},
	{0x6336, 0x00},
	{0x6337, 0x00},
	{0x6338, 0x02},
	{0x6339, 0xF0},
	{0x633A, 0x00},
	{0x633B, 0x00},
	{0x634C, 0x0C},
	{0x634D, 0x00},
	{0x634E, 0x00},
	{0x6350, 0x40},
	{0x6351, 0x00},
	{0x6352, 0x00},
	{0x6353, 0x00},
	{0x6354, 0xD8},
	{0x6355, 0x00},
	{0x6356, 0x00},
	{0x6357, 0x02},
	{0x6358, 0xB0},
	{0x6359, 0x04},
	{0x635A, 0x00},
	{0x636B, 0x00},
	{0x636C, 0x00},
	{0x636D, 0x00},
	{0x636F, 0x00},
	{0x6370, 0x00},
	{0x6371, 0x00},
	{0x6372, 0x00},
	{0x6373, 0x00},
	{0x6374, 0x00},
	{0x6375, 0x00},
	{0x6376, 0x00},
	{0x6377, 0x00},
	{0x6378, 0x00},
	{0x6379, 0x00},
	{0x637A, 0x13},
	{0x637B, 0xD4},
	{0x6388, 0x22},
	{0x6389, 0x82},
	{0x638A, 0xC8},
	{0x639D, 0x20},
	{0x7BA0, 0x01},
	{0x7BA9, 0x00},
	{0x7BAA, 0x01},
	{0x7BAD, 0x00},
	{0x9002, 0x00},
	{0x9003, 0x00},
	{0x9004, 0x0D},
	{0x9006, 0x01},
	{0x9200, 0x93},
	{0x9201, 0x85},
	{0x9202, 0x93},
	{0x9203, 0x87},
	{0x9204, 0x93},
	{0x9205, 0x8D},
	{0x9206, 0x93},
	{0x9207, 0x8F},
	{0x9208, 0x62},
	{0x9209, 0x2C},
	{0x920A, 0x62},
	{0x920B, 0x2F},
	{0x920C, 0x6A},
	{0x920D, 0x23},
	{0x920E, 0x71},
	{0x920F, 0x08},
	{0x9210, 0x71},
	{0x9211, 0x09},
	{0x9212, 0x71},
	{0x9213, 0x0B},
	{0x9214, 0x6A},
	{0x9215, 0x0F},
	{0x9216, 0x71},
	{0x9217, 0x07},
	{0x9218, 0x71},
	{0x9219, 0x03},
	{0x935D, 0x01},
	{0x9389, 0x05},
	{0x938B, 0x05},
	{0x9391, 0x05},
	{0x9393, 0x05},
	{0x9395, 0x65},
	{0x9397, 0x5A},
	{0x9399, 0x05},
	{0x939B, 0x05},
	{0x939D, 0x05},
	{0x939F, 0x05},
	{0x93A1, 0x05},
	{0x93A3, 0x05},
	{0xB3F1, 0x80},
	{0xB3F2, 0x0E},
	{0xBC40, 0x03},
	{0xBC82, 0x07},
	{0xBC83, 0xB0},
	{0xBC84, 0x0D},
	{0xBC85, 0x08},
	{0xE0A6, 0x0A},
	{0x0112, 0x0A},
	{0x0113, 0x0A},
	{0x0114, 0x03},
	{0x0342, 0x0E},
	{0x0343, 0x50},
	{0x0340, 0x0E},
	{0x0341, 0xEC},
	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x00},
	{0x0347, 0x00},
	{0x0348, 0x14},
	{0x0349, 0x3F},
	{0x034A, 0x0F},
	{0x034B, 0x27},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x0900, 0x01},
	{0x0901, 0x22},
	{0x0902, 0x08},
	{0x3F4D, 0x81},
	{0x3F4C, 0x81},
	{0x4254, 0x7F},
	{0x0401, 0x00},
	{0x0404, 0x00},
	{0x0405, 0x10},
	{0x0408, 0x00},
	{0x0409, 0x00},
	{0x040A, 0x00},
	{0x040B, 0x00},
	{0x040C, 0x0A},
	{0x040D, 0x20},
	{0x040E, 0x07},
	{0x040F, 0x94},
	{0x034C, 0x0A},
	{0x034D, 0x20},
	{0x034E, 0x07},
	{0x034F, 0x94},
	{0x0301, 0x05},
	{0x0303, 0x04},
	{0x0305, 0x02},
	{0x0306, 0x00},
	{0x0307, 0xAF},
	{0x030B, 0x02},
	{0x030D, 0x04},
	{0x030E, 0x01},
	{0x030F, 0x0A},
	{0x0310, 0x01},
	{0x0820, 0x0C},
	{0x0821, 0x78},
	{0x0822, 0x00},
	{0x0823, 0x00},
	{0xBC41, 0x01},
	{0x0106, 0x00},
	{0x0B00, 0x00},
	{0x0B05, 0x01},
	{0x0B06, 0x01},
	{0x3230, 0x00},
	{0x3602, 0x01},
	{0x3C00, 0x74},
	{0x3C01, 0x5F},
	{0x3C02, 0x73},
	{0x3C03, 0x64},
	{0x3C04, 0x54},
	{0x3C05, 0xA8},
	{0x3C06, 0xBC},
	{0x3C07, 0x00},
	{0x3C08, 0x00},
	{0x3C09, 0x01},
	{0x3C0A, 0x14},
	{0x3C0B, 0x01},
	{0x3C0C, 0x00},
	{0x3F14, 0x00},
	{0x3F17, 0x00},
	{0x3F3C, 0x00},
	{0x3F78, 0x03},
	{0x3F79, 0x84},
	{0x3F7A, 0x02},
	{0x3F7B, 0xFC},
	{0x562B, 0x0A},
	{0x562D, 0x0C},
	{0x5617, 0x0A},
	{0x9104, 0x04},
	{0x0202, 0x0E},
	{0x0203, 0xD8},
	{0x0204, 0x00},
	{0x0205, 0x00},
	{0x020E, 0x01},
	{0x020F, 0x00},
	{0x3614, 0x00},
	{0x3616, 0x0D},
	{0x3617, 0x56},
	{0xB612, 0x2C},
	{0xB613, 0x2C},
	{0xB614, 0x1C},
	{0xB615, 0x1C},
	{0xB616, 0x06},
	{0xB617, 0x06},
	{0xB618, 0x20},
	{0xB619, 0x20},
	{0xB61A, 0x0C},
	{0xB61B, 0x0C},
	{0xB61C, 0x06},
	{0xB61D, 0x06},
	{0xB666, 0x39},
	{0xB667, 0x39},
	{0xB668, 0x39},
	{0xB669, 0x39},
	{0xB66A, 0x13},
	{0xB66B, 0x13},
	{0xB66C, 0x20},
	{0xB66D, 0x20},
	{0xB66E, 0x20},
	{0xB66F, 0x20},
	{0xB670, 0x10},
	{0xB671, 0x10},
	{0x3900, 0x00},
	{0x3901, 0x00},
	{0x3237, 0x00},
	{0x30AC, 0x00},
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

static int MN169_sensor_vts;
static int MN169_sensor_hts;

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	data_type explow, exphigh;
	struct sensor_info *info = to_state(sd);

	exp_val = (exp_val+8)>>4;

	exphigh = (unsigned char) ((0xff00&exp_val)>>8);
	explow	= (unsigned char) ((0x00ff&exp_val));

	sensor_write(sd, 0x0203, explow);
	sensor_write(sd, 0x0202, exphigh);

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
	long gaindigi = 0;
	int gainana = 0;

	if (gain_val < 16) {
		gainana = 0;
		gaindigi = 256;
	} else if (gain_val <= 256) {
		gainana = 512 - 8192/gain_val;
		gaindigi = 256;
	} else {
		gainana = 480;
		gaindigi = gain_val;
	}

	gainlow = (unsigned char)(gainana&0xff);
	gainhigh = (unsigned char)((gainana>>8)&0xff);

	sensor_write(sd, 0x0205, gainlow);
	sensor_write(sd, 0x0204, gainhigh);

	sensor_write(sd, 0x020F, (unsigned char)(gaindigi&0xff));
	sensor_write(sd, 0x020E, (unsigned char)(gaindigi>>8));

	info->gain = gain_val;

	return 0;
}


static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	int shutter, frame_length;
	static int exp_val, gain_val;
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	shutter = exp_val>>4;
	if (shutter  > MN169_sensor_vts - 4)
		frame_length = shutter + 4;
	else
		frame_length = MN169_sensor_vts;

	sensor_write(sd, 0x0104, 0x01);
	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);
	sensor_write(sd, 0x0104, 0x00);

	info->exp = exp_val;
	info->gain = gain_val;
	return 0;
}

/*
 *set && get sensor flip
 */

static int sensor_get_fmt_mbus_core(struct v4l2_subdev *sd, int *code)
{
	struct sensor_info *info = to_state(sd);
	data_type get_value;

	sensor_read(sd, 0x0101, &get_value);
	switch (get_value) {
	case 0x00:
		*code = MEDIA_BUS_FMT_SRGGB10_1X10;
		break;
	case 0x01:
		*code = MEDIA_BUS_FMT_SRGGB10_1X10;
		break;
	case 0x02:
		*code = MEDIA_BUS_FMT_SGBRG10_1X10;
		break;
	case 0x03:
		*code = MEDIA_BUS_FMT_SBGGR10_1X10;
		break;
	default:
		*code = info->fmt->mbus_code;
	}
	return 0;
}

static int sensor_s_hflip(struct v4l2_subdev *sd, int enable)
{
	data_type get_value;
	data_type set_value;

	sensor_print("into set sensor hfilp the value:%d \n", enable);
	if (!(enable == 0 || enable == 1))
		return -1;

	sensor_read(sd, 0x0101, &get_value);
	if (enable)
		set_value = get_value | 0x02;
	else
		set_value = get_value & 0xfd;

	sensor_write(sd, 0x0101, set_value);
	return 0;
}

static int sensor_s_vflip(struct v4l2_subdev *sd, int enable)
{
	data_type get_value;
	data_type set_value;

	sensor_print("into set sensor hfilp the value:%d \n", enable);
	if (!(enable == 0 || enable == 1))
		return -1;

	sensor_read(sd, 0x0101, &get_value);
	if (enable)
		set_value = get_value | 0x02;
	else
		set_value = get_value & 0xfd;

	sensor_write(sd, 0x0101, set_value);
	return 0;

}

/* long exp mode eg:
 *
 *   coarse_int_time = 60290; frame_length= 65535;
 *   times =8; VTPXCLK = 480Mhz; imx386_hts = 4976
 *
 *	EXP time = 60290 * 8 * 4976 / (120[Mhz] * 4 ) = 5 s
 */
static int fps_change_flag;

static int sensor_s_fps(struct v4l2_subdev *sd,
			struct sensor_fps *fps)
{
	unsigned int coarse_int_time, frame_length;
	struct sensor_info *info = to_state(sd);
	struct sensor_win_size *wsize = info->current_wins;
	unsigned int times_reg, times, MN169_hts, FRM_LINES;

	sensor_print("------start set fps = %d--------!!!!\n", fps->fps);
	if (fps->fps == 0)
		fps->fps = 30;

	/****************************/
	/* Early enter long exp mode; in case of quit before */
	if (fps->fps < 0) {
		times = 2;
		times_reg = 0x1;
		MN169_hts = wsize->hts;

		/* test : when fps = 10  && delay 150ms, will not quit before */
		coarse_int_time = wsize->pclk/MN169_hts/times/10;
		FRM_LINES = coarse_int_time;
		coarse_int_time -= 10;

		sensor_write(sd, 0x0100, 0x00);
		sensor_write(sd, 0x0104, 0x01);

		sensor_write(sd, 0x3004, times_reg);

		sensor_write(sd, 0x0202, (coarse_int_time >> 8));
		sensor_write(sd, 0x0203, (coarse_int_time & 0xff));

		sensor_write(sd, 0x0340, (FRM_LINES >> 8));
		sensor_write(sd, 0x0341, (FRM_LINES & 0xff));

		sensor_write(sd, 0x0342, (MN169_hts >> 8));
		sensor_write(sd, 0x0343, (MN169_hts & 0xff));

		sensor_write(sd, 0x0100, 0x01);
		sensor_write(sd, 0x0104, 0x00);

		usleep_range(150000, 200000);
	}

	/****************************/

	if (fps->fps < 0) {
	sensor_print("------enter long exp--------!!!!\n");
		fps->fps = -fps->fps;

		if (fps->fps >= 1 && fps->fps <= 5) {
			times = 8;
			times_reg = 0x3;
			MN169_hts = wsize->hts;
		} else if (fps->fps <= 10) {
			times = 16;
			times_reg = 0x4;
			MN169_hts = wsize->hts;
		} else if (fps->fps <= 20) {
			times = 16;
			times_reg = 0x4;
			MN169_hts = wsize->hts * 2;
		} else {
			times = 16;
			times_reg = 0x4;
			MN169_hts = wsize->hts * 4;
		}

		coarse_int_time = wsize->pclk/MN169_hts/times * fps->fps;
		FRM_LINES = coarse_int_time;
		/* 0 <=  coarse_int_time  <= FRM_LINES - 10 */
		coarse_int_time -= 20;
		fps_change_flag = 1;
	} else {
		sensor_print("------enter normal exp--------!!!!\n");
		coarse_int_time = wsize->pclk/wsize->hts/fps->fps;
		if (coarse_int_time	> MN169_sensor_vts - 4)
			frame_length = coarse_int_time + 4;
		else
			frame_length = MN169_sensor_vts;
		fps_change_flag = 0;
	}

	/* sensor reg standby */
	sensor_write(sd, 0x0100, 0x00);
	/* grouped hold function */
	sensor_write(sd, 0x0104, 0x01);

	if (fps_change_flag == 1) {
		/* open long exp mode */
		sensor_write(sd, 0x3004, times_reg);

		sensor_write(sd, 0x0202, (coarse_int_time >> 8));
		sensor_write(sd, 0x0203, (coarse_int_time & 0xff));

		sensor_write(sd, 0x0340, (FRM_LINES >> 8));
		sensor_write(sd, 0x0341, (FRM_LINES & 0xff));

		sensor_write(sd, 0x0342, (MN169_hts >> 8));
		sensor_write(sd, 0x0343, (MN169_hts & 0xff));
	} else {
		/* close long exp mode */
		sensor_write(sd, 0x3004, 0x00);

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
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, IOVDD, ON);
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
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, AFVDD, OFF);
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
	sensor_read(sd, 0x0016, &rdval);
	sensor_dbg("0x0016 0x%x\n", rdval);
	sensor_print("#########________________##########");
	sensor_read(sd, 0x0017, &rdval);
	sensor_dbg("0x0017 0x%x\n", rdval);

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
	info->width = 4000;
	info->height = 3000;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->exp = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 120;	/* 30fps */

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
	.width         = 2592,
	.height        = 1940,
	.hoffset       = 0,
	.voffset       = 0,
	.hts           = 3664,
	.vts           = 3820,
	.pclk          = 419894400,
	.mipi_bps      = 740*1000*1000,
	.fps_fixed     = 120,
	.bin_factor    = 1,
	.intg_min      = 16,
	.intg_max      = (3820-4)<<4,
	.gain_min      = 16,
	.gain_max      = (128<<4),
	.regs          = sensor_1940p30_regs,
	.regs_size     = ARRAY_SIZE(sensor_1940p30_regs),
	.set_size      = NULL,
	.top_clk       = 310*1000*1000,
	.isp_clk       = 283*1000*1000,
	 },

};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_CSI2_DPHY;
	cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;

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
	MN169_sensor_vts = wsize->vts;
	MN169_sensor_hts = wsize->hts;

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

	v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 16,
			      2816 * 16, 1, 1 * 16);
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
