/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for Raw cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Chen weihong <chenweihong@allwinnertech.com>
 *
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
MODULE_DESCRIPTION("A low-level driver for GC4663 sensors");
MODULE_LICENSE("GPL");

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR  0x4653

//define the registers
#define EXP_HIGH		0xff
#define EXP_MID			0x03
#define EXP_LOW			0x04
#define GAIN_HIGH		0xff
#define GAIN_LOW		0x24
/*
 * Our nominal (default) frame rate.
 */
#define ID_REG_HIGH		0x03f0
#define ID_REG_LOW		0x03f1
#define ID_VAL_HIGH		((V4L2_IDENT_SENSOR) >> 8)
#define ID_VAL_LOW		((V4L2_IDENT_SENSOR) & 0xff)
#define SENSOR_FRAME_RATE 30

/*
 * The GC4663 i2c address
 */
#define I2C_ADDR 0x52

#define SENSOR_NUM 0x2
#define SENSOR_NAME "gc4663_mipi"
#define SENSOR_NAME_2 "gc4663_mipi_2"

#define HDR_RATIO 16

/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};


static struct regval_list sensor_2560x1440p30_regs[] = {
//version 6.8
//mclk 24Mhz
//mipi_data_rate 648Mbps
//framelength 1500
//linelength 4800
//pclk 216Mhz
//rowtime 22.2222us
//pattern grbg
	{0x03fe, 0xf0},
	{0x03fe, 0x00},
	{0x0317, 0x00},
	{0x0320, 0x77},
	{0x0324, 0xc8},
	{0x0325, 0x06},
	{0x0326, 0x6c},
	{0x0327, 0x03},
	{0x0334, 0x40},
	{0x0336, 0x6c},
	{0x0337, 0x82},
	{0x0315, 0x25},
	{0x031c, 0xc6},
	{0x0287, 0x18},
	{0x0084, 0x00},
	{0x0087, 0x50},
	{0x029d, 0x08},
	{0x0290, 0x00},
	{0x0340, 0x05},
	{0x0341, 0xdc},
	{0x0345, 0x06},
	{0x034b, 0xb0},
	{0x0352, 0x08},
	{0x0354, 0x08},
	{0x02d1, 0xe0},
	{0x0223, 0xf2},
	{0x0238, 0xa4},
	{0x02ce, 0x7f},
	{0x0232, 0xc4},
	{0x02d3, 0x05},
	{0x0243, 0x06},
	{0x02ee, 0x30},
	{0x026f, 0x70},
	{0x0257, 0x09},
	{0x0211, 0x02},
	{0x0219, 0x09},
	{0x023f, 0x2d},
	{0x0518, 0x00},
	{0x0519, 0x01},
	{0x0515, 0x08},
	{0x02d9, 0x3f},
	{0x02da, 0x02},
	{0x02db, 0xe8},
	{0x02e6, 0x20},
	{0x021b, 0x10},
	{0x0252, 0x22},
	{0x024e, 0x22},
	{0x02c4, 0x01},
	{0x021d, 0x17},
	{0x024a, 0x01},
	{0x02ca, 0x02},
	{0x0262, 0x10},
	{0x029a, 0x20},
	{0x021c, 0x0e},
	{0x0298, 0x03},
	{0x029c, 0x00},
	{0x027e, 0x14},
	{0x02c2, 0x10},
	{0x0540, 0x20},
	{0x0546, 0x01},
	{0x0548, 0x01},
	{0x0544, 0x01},
	{0x0242, 0x1b},
	{0x02c0, 0x1b},
	{0x02c3, 0x20},
	{0x02e4, 0x10},
	{0x022e, 0x00},
	{0x027b, 0x3f},
	{0x0269, 0x0f},
	{0x02d2, 0x40},
	{0x027c, 0x08},
	{0x023a, 0x2e},
	{0x0245, 0xce},
	{0x0530, 0x20},
	{0x0531, 0x02},
	{0x0228, 0x50},
	{0x02ab, 0x00},
	{0x0250, 0x00},
	{0x0221, 0x50},
	{0x02ac, 0x00},
	{0x02a5, 0x02},
	{0x0260, 0x0b},
	{0x0216, 0x04},
	{0x0299, 0x1C},
	{0x02bb, 0x0d},
	{0x02a3, 0x02},
	{0x02a4, 0x02},
	{0x021e, 0x02},
	{0x024f, 0x08},
	{0x028c, 0x08},
	{0x0532, 0x3f},
	{0x0533, 0x02},
	{0x0277, 0xc0},
	{0x0276, 0xc0},
	{0x0239, 0xc0},
	{0x0202, 0x05},
	{0x0203, 0xd0},
	{0x0205, 0xc0},
	{0x02b0, 0x68},
	{0x0002, 0xa9},
	{0x0004, 0x01},
	{0x021a, 0x98},
	{0x0266, 0xa0},
	{0x0020, 0x01},
	{0x0021, 0x03},
	{0x0022, 0x00},
	{0x0023, 0x04},
	{0x0342, 0x06},
	{0x0343, 0x40},
	{0x03fe, 0x10},
	{0x03fe, 0x00},
	{0x0106, 0x78},
	{0x0108, 0x0c},
	{0x0114, 0x01},
	{0x0115, 0x12},
	{0x0180, 0x46},
	{0x0181, 0x30},
	{0x0182, 0x05},
	{0x0185, 0x01},
	{0x03fe, 0x10},
	{0x03fe, 0x00},
	{0x0100, 0x09},
	{0x000f, 0x00},
	/* otp */
	{0x0080, 0x02},
	{0x0097, 0x0a},
	{0x0098, 0x10},
	{0x0099, 0x05},
	{0x009a, 0xb0},
	{0x0317, 0x08},
	{0x0a67, 0x80},
	{0x0a70, 0x03},
	{0x0a82, 0x00},
	{0x0a83, 0x10},
	{0x0a80, 0x2b},
	{0x05be, 0x00},
	{0x05a9, 0x01},
	{0x0313, 0x80},
	{0x05be, 0x01},
	{0x0317, 0x00},
	{0x0a67, 0x00},
	/* colorbar */
/*	{0x008c, 0x11}, */
};

static struct regval_list sensor_2560x1440p20_wdr_regs[] = {
/* version 4.2
mclk 24Mhz
mipiclk 1309.5Mhz
framelength 2400
exp1 <= 108
exp2+exp1<frame_length
if exp1 * 16 = exp2
exp1_max = 94, exp2_max = 1504
rowtime 10.41666667us
frame_rate 20fps
pattern grbg */
	{0x03fe, 0xf0},
	{0x03fe, 0x00},
	{0x0317, 0x00},
	{0x0320, 0x77},
	{0x0324, 0xc4},
	{0x0326, 0x42}, /* 41 */
	{0x0327, 0x03},
	{0x0321, 0x10},
	{0x0314, 0x50},
	{0x0334, 0x40},
	{0x0335, 0xd1},
	{0x0336, 0x70}, /* 61 */
	{0x0337, 0x82},
	{0x0315, 0x33},
	{0x031c, 0xce},
	{0x0287, 0x18},
	{0x0084, 0x00},
	{0x0087, 0x50},
	{0x029d, 0x08},
	{0x0290, 0x00},
	{0x0340, 0x09}, /* fl */
	{0x0341, 0x60}, /* fl 2400 */
	{0x0345, 0x06},
	{0x034b, 0xb0},
	{0x0352, 0x08},
	{0x0354, 0x08},
	{0x02d1, 0xc0},  /* add vref 3.3v */
	{0x023c, 0x04},
	{0x0238, 0xb4},
	{0x0223, 0xfb},
	{0x0232, 0xc4},
	{0x0279, 0x53},
	{0x02d3, 0x01},
	{0x0243, 0x06},
	{0x02ce, 0xbf},
	{0x02ee, 0x30},
	{0x026f, 0x70},
	{0x0257, 0x09},
	{0x0211, 0x02},
	{0x0219, 0x09},
	{0x023f, 0x2d},
	{0x0518, 0x00},
	{0x0519, 0x01},
	{0x0515, 0x18},/* decin offset */
	{0x02d9, 0x3f},
	{0x02da, 0x02},
	{0x02db, 0xe8},
	{0x02e6, 0x20},
	{0x021b, 0x10},
	{0x0252, 0x22},
	{0x024e, 0x22},
	{0x02c4, 0x01},
	{0x021d, 0x17},
	{0x024a, 0x01},
	{0x02ca, 0x02},
	{0x0262, 0x10},
	{0x029a, 0x20},
	{0x021c, 0x0e},
	{0x0298, 0x03},
	{0x029c, 0x00},
	{0x027e, 0x14},
	{0x02c2, 0x10},
	{0x0540, 0x20},
	{0x0546, 0x01},
	{0x0548, 0x01},
	{0x0544, 0x01},
	{0x0242, 0x36},
	{0x02c0, 0x36},
	{0x02c3, 0x4d},
	{0x02e4, 0x10},
	{0x022e, 0x00},
	{0x027b, 0x3f},
	{0x0269, 0x0f},
	{0x02d2, 0x40},
	{0x027c, 0x08},
	{0x023a, 0x2e},
	{0x0245, 0xce},
	{0x0530, 0x3f},
	{0x0531, 0x02},
	{0x0228, 0x50},/* eqc1 */
	{0x02ab, 0x00},
	{0x0250, 0x00},
	{0x0221, 0x50},/* eqc2 */
	{0x02ac, 0x00},
	{0x02a5, 0x02},
	{0x0260, 0x0b},
	{0x0216, 0x04},
	{0x0299, 0x1C},
	/* tony_add */
	{0x021a, 0x98},
	{0x0266, 0xd0},
	{0x0020, 0x01},
	{0x0021, 0x03},
	{0x0022, 0xc0},
	{0x0023, 0x04},
	{0x02bb, 0x0d},
	{0x02a3, 0x02},
	{0x02a4, 0x02},
	{0x021e, 0x02},
	{0x024f, 0x08},
	{0x028c, 0x08},
	{0x0532, 0x3f},
	{0x0533, 0x02},
	{0x0277, 0x70}, /* tx_width */
	{0x0276, 0xc0},
	{0x0239, 0xc0},
	{0x0200, 0x00},
	{0x0201, 0x5f},
	{0x0202, 0x05},
	{0x0203, 0xf0},
	{0x0205, 0xc0},
	{0x02b0, 0x68},
	{0x000f, 0x00},
	{0x0006, 0xe0},
	{0x0002, 0xa9},
	{0x0004, 0x01},
	{0x0060, 0x40},

	{0x0218, 0x12},/* hdr */
	{0x0342, 0x05},/* hb */
	{0x0343, 0x5f},/* hb */
	{0x03fe, 0x10},
	{0x03fe, 0x00},
	{0x0106, 0x78},
	{0x0107, 0x89},
	{0x0108, 0x0c},
	{0x0114, 0x01},
	{0x0115, 0x12},
	{0x0180, 0x4f},
	{0x0181, 0x30},
	{0x0182, 0x05},
	{0x0185, 0x01},
	{0x03fe, 0x10},
	{0x03fe, 0x00},
	{0x0100, 0x09},
	/* colorbar */
/*	{0x008c, 0x11}, */
};

static struct regval_list sensor_2560x1440p15_wdr_regs[] = {
/*
* version 4.2
* mclk 24Mhz
/* mipiclk 1344Mhz
framelength 3200
exp1 <= 3200-1440-16-20
exp2+exp1 <= 3200
rowtime 10.41666667us
frame rate=15fps
pattern grbg */
	{0x03fe, 0xf0},
	{0x03fe, 0x00},
	{0x0317, 0x00},
	{0x0320, 0x77},
	{0x0324, 0xc4},
	{0x0326, 0x42}, /* 41 */
	{0x0327, 0x03},
	{0x0321, 0x10},
	{0x0314, 0x50},
	{0x0334, 0x40},
	{0x0335, 0xd1},
	{0x0336, 0x70}, /* 61 */
	{0x0337, 0x82},
	{0x0315, 0x33},
	{0x031c, 0xce},
	{0x0287, 0x18},
	{0x0084, 0x00},
	{0x0087, 0x50},
	{0x029d, 0x08},
	{0x0290, 0x00},
	{0x0340, 0x0c}, /* fl */
	{0x0341, 0x80}, /* fl 1600 */
	{0x0345, 0x06},
	{0x034b, 0xb0},
	{0x0352, 0x08},
	{0x0354, 0x08},
	{0x02d1, 0xc0},/* add vref 3.3v */
	{0x023c, 0x04},
	{0x0238, 0xb4},
	{0x0223, 0xfb},
	{0x0232, 0xc4},
	{0x0279, 0x53},
	{0x02d3, 0x01},
	{0x0243, 0x06},
	{0x02ce, 0xbf},
	{0x02ee, 0x30},
	{0x026f, 0x70},
	{0x0257, 0x09},
	{0x0211, 0x02},
	{0x0219, 0x09},
	{0x023f, 0x2d},
	{0x0518, 0x00},
	{0x0519, 0x01},
	{0x0515, 0x18},/* decin offset */
	{0x02d9, 0x3f},
	{0x02da, 0x02},
	{0x02db, 0xe8},
	{0x02e6, 0x20},
	{0x021b, 0x10},
	{0x0252, 0x22},
	{0x024e, 0x22},
	{0x02c4, 0x01},
	{0x021d, 0x17},
	{0x024a, 0x01},
	{0x02ca, 0x02},
	{0x0262, 0x10},
	{0x029a, 0x20},
	{0x021c, 0x0e},
	{0x0298, 0x03},
	{0x029c, 0x00},
	{0x027e, 0x14},
	{0x02c2, 0x10},
	{0x0540, 0x20},
	{0x0546, 0x01},
	{0x0548, 0x01},
	{0x0544, 0x01},
	{0x0242, 0x36},
	{0x02c0, 0x36},
	{0x02c3, 0x4d},
	{0x02e4, 0x10},
	{0x022e, 0x00},
	{0x027b, 0x3f},
	{0x0269, 0x0f},
	{0x02d2, 0x40},
	{0x027c, 0x08},
	{0x023a, 0x2e},
	{0x0245, 0xce},
	{0x0530, 0x3f},
	{0x0531, 0x02},
	{0x0228, 0x50},/* eqc1 */
	{0x02ab, 0x00},
	{0x0250, 0x00},
	{0x0221, 0x50},/* eqc2 */
	{0x02ac, 0x00},
	{0x02a5, 0x02},
	{0x0260, 0x0b},
	{0x0216, 0x04},
	{0x0299, 0x1C},
	/* tony_add */
	{0x021a, 0x98},
	{0x0266, 0xd0},
	{0x0020, 0x01},
	{0x0021, 0x05},
	{0x0022, 0xc0},
	{0x0023, 0x08},
	/* tony_add */
	{0x02bb, 0x0d},
	{0x02a3, 0x02},
	{0x02a4, 0x02},
	{0x021e, 0x02},
	{0x024f, 0x08},
	{0x028c, 0x08},
	{0x0532, 0x3f},
	{0x0533, 0x02},
	{0x0277, 0x70}, /* tx_width */
	{0x0276, 0xc0},
	{0x0239, 0xc0},
	{0x0200, 0x00},
	{0x0201, 0x5f},
	{0x0202, 0x05},
	{0x0203, 0xf0},
	{0x0205, 0xc0},
	{0x02b0, 0x68},
	{0x000f, 0x00},
	{0x0006, 0xe0},
	{0x0002, 0xa9},
	{0x0004, 0x01},
	{0x0060, 0x40},

	{0x0218, 0x12},/* hdr */
	{0x0342, 0x05},/* hb */
	{0x0343, 0x5f},/* hb */
	{0x03fe, 0x10},
	{0x03fe, 0x00},
	{0x0106, 0x78},
	{0x0107, 0x89},/* VC_enable,ID__01 */
	{0x0108, 0x0c},
	{0x0114, 0x01},
	{0x0115, 0x12},
	{0x0180, 0x4f},
	{0x0181, 0x30},
	{0x0182, 0x05},
	{0x0185, 0x01},
	{0x03fe, 0x10},
	{0x03fe, 0x00},
	{0x0100, 0x09},
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

static int sensor_s_shutter(struct v4l2_subdev *sd, unsigned int intt_long, unsigned int intt_short)
{
	unsigned int intt_long_h, intt_long_l, intt_short_h, intt_short_l;
	unsigned int short_exp_max = 900, long_exp_max = 0; /* 20fps - 900 */

	if (intt_long <= 1)
		intt_long = 1;
	if (intt_short < 1)
		intt_short = 1;

	if (intt_short >= short_exp_max)
		intt_short = short_exp_max;
	long_exp_max = 2400 - intt_short - 16;
	if (intt_long >= long_exp_max)
		intt_long = long_exp_max;

	intt_long_l = intt_long & 0xff;
	intt_long_h = (intt_long >> 8) & 0x3f;
	intt_short_l = intt_short & 0xff;
	intt_short_h = (intt_short >> 8) & 0x3f;

	sensor_write(sd, 0x0202, intt_long_h);
	sensor_write(sd, 0x0203, intt_long_l);
	sensor_dbg("sensor_set_long_exp = %d line Done!\n", intt_long);
	sensor_write(sd, 0x0200, intt_short_h);
	sensor_write(sd, 0x0201, intt_short_l);
	sensor_dbg("sensor_set_short_exp = %d line Done!\n", intt_short);

	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	struct sensor_info *info = to_state(sd);
	int tmp_exp_val = exp_val / 16;
	int exp_short = 0;

	if (info->isp_wdr_mode == ISP_DOL_WDR_MODE) {
		sensor_dbg("Sensor in WDR mode, HDR_RATIO = %d\n", HDR_RATIO);
		exp_short = tmp_exp_val / HDR_RATIO;
		sensor_s_shutter(sd, tmp_exp_val, exp_short);
	} else {
		sensor_dbg("exp_val:%d\n", exp_val);
		sensor_write(sd, 0x202, (tmp_exp_val >> 8) & 0xFF);
		sensor_write(sd, 0x203, (tmp_exp_val & 0xFF));
	}

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

unsigned char regValTable[26][7] = {
	/* 2b3  2b4   2b8   2b9	 515   519   2d9 */
	{0x00, 0x00, 0x01, 0x00, 0x30, 0x1e, 0x5C},
	{0x20, 0x00, 0x01, 0x0B, 0x30, 0x1e, 0x5C},
	{0x01, 0x00, 0x01, 0x19, 0x30, 0x1d, 0x5B},
	{0x21, 0x00, 0x01, 0x2A, 0x30, 0x1e, 0x5C},
	{0x02, 0x00, 0x02, 0x00, 0x30, 0x1e, 0x5C},
	{0x22, 0x00, 0x02, 0x17, 0x30, 0x1d, 0x5B},
	{0x03, 0x00, 0x02, 0x33, 0x20, 0x16, 0x54},
	{0x23, 0x00, 0x03, 0x14, 0x20, 0x17, 0x55},
	{0x04, 0x00, 0x04, 0x00, 0x20, 0x17, 0x55},
	{0x24, 0x00, 0x04, 0x2F, 0x20, 0x19, 0x57},
	{0x05, 0x00, 0x05, 0x26, 0x20, 0x19, 0x57},
	{0x25, 0x00, 0x06, 0x28, 0x20, 0x1b, 0x59},
	{0x0c, 0x00, 0x08, 0x00, 0x20, 0x1d, 0x5B},
	{0x2C, 0x00, 0x09, 0x1E, 0x20, 0x1f, 0x5D},
	{0x0D, 0x00, 0x0B, 0x0C, 0x20, 0x21, 0x5F},
	{0x2D, 0x00, 0x0D, 0x11, 0x20, 0x24, 0x62},
	{0x1C, 0x00, 0x10, 0x00, 0x20, 0x26, 0x64},
	{0x3C, 0x00, 0x12, 0x3D, 0x18, 0x2a, 0x68},
	{0x5C, 0x00, 0x16, 0x19, 0x18, 0x2c, 0x6A},
	{0x7C, 0x00, 0x1A, 0x22, 0x18, 0x2e, 0x6C},
	{0x9C, 0x00, 0x20, 0x00, 0x18, 0x32, 0x70},
	{0xBC, 0x00, 0x25, 0x3A, 0x18, 0x35, 0x73},
	{0xDC, 0x00, 0x2C, 0x33, 0x10, 0x36, 0x74},
	{0xFC, 0x00, 0x35, 0x05, 0x10, 0x38, 0x76},
	{0x1C, 0x01, 0x40, 0x00, 0x10, 0x3c, 0x7A},
	{0x3C, 0x01, 0x4B, 0x35, 0x10, 0x42, 0x80},
};

unsigned char regValTable_wdr[26][7] = {
	/* 2b3  2b4   2b8   2b9	 515   519   2d9 */
	{0x00, 0x00, 0x01, 0x00, 0x30, 0x28, 0x66},
	{0x20, 0x00, 0x01, 0x0B, 0x30, 0x2a, 0x68},
	{0x01, 0x00, 0x01, 0x19, 0x30, 0x27, 0x65},
	{0x21, 0x00, 0x01, 0x2A, 0x30, 0x29, 0x67},
	{0x02, 0x00, 0x02, 0x00, 0x30, 0x27, 0x65},
	{0x22, 0x00, 0x02, 0x17, 0x30, 0x29, 0x67},
	{0x03, 0x00, 0x02, 0x33, 0x30, 0x28, 0x66},
	{0x23, 0x00, 0x03, 0x14, 0x30, 0x2a, 0x68},
	{0x04, 0x00, 0x04, 0x00, 0x30, 0x2a, 0x68},
	{0x24, 0x00, 0x04, 0x2F, 0x30, 0x2b, 0x69},
	{0x05, 0x00, 0x05, 0x26, 0x30, 0x2c, 0x6A},
	{0x25, 0x00, 0x06, 0x28, 0x30, 0x2e, 0x6C},
	{0x06, 0x00, 0x08, 0x00, 0x30, 0x2f, 0x6D},
	{0x26, 0x00, 0x09, 0x1E, 0x30, 0x31, 0x6F},
	{0x46, 0x00, 0x0B, 0x0C, 0x30, 0x34, 0x72},
	{0x66, 0x00, 0x0D, 0x11, 0x30, 0x37, 0x75},
	{0x0e, 0x00, 0x10, 0x00, 0x30, 0x3a, 0x78},
	{0x2e, 0x00, 0x12, 0x3D, 0x30, 0x3e, 0x7C},
	{0x4e, 0x00, 0x16, 0x19, 0x30, 0x41, 0x7F},
	{0x6e, 0x00, 0x1A, 0x22, 0x30, 0x45, 0x83},
	{0x1e, 0x00, 0x20, 0x00, 0x30, 0x49, 0x87},
	{0x3e, 0x00, 0x25, 0x3A, 0x30, 0x4d, 0x8B},
	{0x5e, 0x00, 0x2C, 0x33, 0x30, 0x53, 0x91},
	{0x7e, 0x00, 0x35, 0x05, 0x30, 0x5a, 0x98},
	{0x9e, 0x00, 0x40, 0x00, 0x30, 0x60, 0x9E},
	{0xbe, 0x00, 0x4B, 0x35, 0x30, 0x67, 0xA5},
};

unsigned int analog_gain_table[26] = {
	  64,   75,   89,  106,  128,  151,  179,  212,
	 256,  303,  358,  424,  512,  606,  716,  849,
	1024, 1213, 1433, 1698, 2048, 2426, 2867, 3397,
	4096, 4853,
};

static int setSensorGain(struct v4l2_subdev *sd, unsigned int gain)
{
	struct sensor_info *info = to_state(sd);
	int i, total;
	unsigned int tol_dig_gain = 0;

	total = sizeof(analog_gain_table) / sizeof(unsigned int);
	for (i = 0; i < total - 1; i++) {
	  if ((analog_gain_table[i] <= gain) && (gain < analog_gain_table[i+1]))
	    break;
	}
	tol_dig_gain = gain*64/analog_gain_table[i];

	if (info->isp_wdr_mode == ISP_DOL_WDR_MODE) {
		sensor_write(sd, 0x02b3, regValTable_wdr[i][0]);
		sensor_write(sd, 0x02b4, regValTable_wdr[i][1]);
		sensor_write(sd, 0x02b8, regValTable_wdr[i][2]);
		sensor_write(sd, 0x02b9, regValTable_wdr[i][3]);
		sensor_write(sd, 0x0515, regValTable_wdr[i][4]);
		sensor_write(sd, 0x0519, regValTable_wdr[i][5]);
		sensor_write(sd, 0x02d9, regValTable_wdr[i][6]);
	} else {
		sensor_write(sd, 0x02b3, regValTable[i][0]);
		sensor_write(sd, 0x02b4, regValTable[i][1]);
		sensor_write(sd, 0x02b8, regValTable[i][2]);
		sensor_write(sd, 0x02b9, regValTable[i][3]);
		sensor_write(sd, 0x0515, regValTable[i][4]);
		sensor_write(sd, 0x0519, regValTable[i][5]);
		sensor_write(sd, 0x02d9, regValTable[i][6]);

		sensor_write(sd, 0x20e, (tol_dig_gain>>6));
		sensor_write(sd, 0x20f, ((tol_dig_gain&0x3f)<<2));
	}

	return 0;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	struct sensor_info *info = to_state(sd);

	if (gain_val == info->gain) {
		return 0;
	}

	sensor_dbg("gain_val:%d\n", gain_val);
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

	if (gain_val < (1 * 16)) {
		gain_val = 16;
	}

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
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_set_status(sd, POWER_EN, 1);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		/* /* sk
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);
		*/
		vin_set_pmu_channel(sd, IOVDD, ON);
		usleep_range(100, 120);
		vin_set_pmu_channel(sd, DVDD, ON);
		usleep_range(1000, 1200);
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_set_mclk(sd, ON);
		usleep_range(1000, 1200);
		vin_set_mclk_freq(sd, MCLK);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(30000, 31000);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!do nothing\n");
		cci_lock(sd);
		vin_set_mclk(sd, OFF);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_LOW);
		/* vin_set_pmu_channel(sd, CMBCSI, OFF); */
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
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
		sensor_dbg("eRet:%d, ID_VAL_HIGH:0x%x, times_out:%d\n", eRet, rdval, times_out);
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
	info->width        = 2560;
	info->height       = 1440;
	info->hflip        = 0;
	info->vflip        = 0;
	info->gain         = 0;
	info->exp          = 0;

	info->tpf.numerator      = 1;
	info->tpf.denominator    = 30;	/* 30fps */
	info->preview_first_flag = 1;
	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);

	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		sensor_print("%s: GET_CURRENT_WIN_CFG, info->current_wins=%p\n", __func__, info->current_wins);

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
		.mbus_code = MEDIA_BUS_FMT_SGRBG10_1X10, /* .mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10, */
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
	{
		.width      = 2560,
		.height     = 1440,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 4800,
		.vts        = 1500,
		.pclk       = 216 * 1000 * 1000,
		.mipi_bps   = 648 * 1000 * 1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 1500 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 110 << 4,
		.regs       = sensor_2560x1440p30_regs,
		.regs_size  = ARRAY_SIZE(sensor_2560x1440p30_regs),
		.set_size   = NULL,
		.top_clk    = 300*1000*1000,
		.isp_clk    = 297*1000*1000,
	},

	{
		.width      = 2560,
		.height     = 1440,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 1375,
		.vts        = 2400,
		.pclk       = 132 * 1000 * 1000,
		.mipi_bps   = 648 * 1000 * 1000,
		.fps_fixed  = 20,
		.bin_factor = 1,
		.if_mode    = MIPI_VC_WDR_MODE,
		.wdr_mode   = ISP_DOL_WDR_MODE,
		.intg_min   = 1 << 4,
		.intg_max   = 1600 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 110 << 4,
		.regs       = sensor_2560x1440p20_wdr_regs,
		.regs_size  = ARRAY_SIZE(sensor_2560x1440p20_wdr_regs),
		.set_size   = NULL,
		.top_clk    = 300*1000*1000,
		.isp_clk    = 297*1000*1000,
	},

	{
		.width      = 2560,
		.height     = 1440,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 1375,
		.vts        = 2400,
		.pclk       = 132 * 1000 * 1000,
		.mipi_bps   = 648 * 1000 * 1000,
		.fps_fixed  = 15,
		.bin_factor = 1,
		.if_mode    = MIPI_VC_WDR_MODE,
		.wdr_mode   = ISP_DOL_WDR_MODE,
		.intg_min   = 1 << 4,
		.intg_max   = 1600 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 110 << 4,
		.regs       = sensor_2560x1440p15_wdr_regs,
		.regs_size  = ARRAY_SIZE(sensor_2560x1440p15_wdr_regs),
		.set_size   = NULL,
		.top_clk    = 300*1000*1000,
		.isp_clk    = 297*1000*1000,
	},

};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	struct sensor_info *info = to_state(sd);
	cfg->type  = V4L2_MBUS_CSI2_DPHY;

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

	sensor_dbg("ARRAY_SIZE(sensor_default_regs)=%d\n",
			(unsigned int)ARRAY_SIZE(sensor_default_regs));

	ret = sensor_write_array(sd, sensor_default_regs,
				 ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs) {
		sensor_write_array(sd, wsize->regs, wsize->regs_size);
	}

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
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET3 | MIPI_NORMAL_MODE;
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
