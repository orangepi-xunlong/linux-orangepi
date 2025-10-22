/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for SC200AI Raw cameras.
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
MODULE_DESCRIPTION("A low-level driver for SC200AI sensors");
MODULE_LICENSE("GPL");

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR 0xcb1c

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

#define HDR_RATIO 8

/*
 * The SC200AI i2c address
 */
#define I2C_ADDR 0x60

#define SENSOR_NUM 0x2
#define SENSOR_NAME "sc200ai_mipi"
#define SENSOR_NAME_2 "sc200ai_mipi_2"

/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};



//24M 30fps normal
static struct regval_list sensor_1080p30_regs[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x36e9, 0x80},
	{0x36f9, 0x80},
	{0x301f, 0x0f},

	{0x320e, 0x04},  /* vts-high */
	{0x320f, 0x65},  /* vts-low */

	{0x3243, 0x01},
	{0x3248, 0x02},
	{0x3249, 0x09},
	{0x3253, 0x08},
	{0x3271, 0x0a},
	{0x3301, 0x20},
	{0x3304, 0x40},
	{0x3306, 0x32},
	{0x330b, 0x88},
	{0x330f, 0x02},
	{0x331e, 0x39},
	{0x3333, 0x10},
	{0x3621, 0xe8},
	{0x3622, 0x16},
	{0x3637, 0x1b},
	{0x363a, 0x1f},
	{0x363b, 0xc6},
	{0x363c, 0x0e},
	{0x3670, 0x0a},
	{0x3674, 0x82},
	{0x3675, 0x76},
	{0x3676, 0x78},
	{0x367c, 0x48},
	{0x367d, 0x58},
	{0x3690, 0x34},
	{0x3691, 0x33},
	{0x3692, 0x44},
	{0x369c, 0x40},
	{0x369d, 0x48},
	{0x36ea, 0x75},
	{0x36eb, 0x0d},
	{0x36ec, 0x1c},
	{0x36ed, 0x24},
	{0x36fa, 0x5f},
	{0x36fb, 0x00},
	{0x36fc, 0x10},
	{0x36fd, 0x07},
	{0x3901, 0x02},
	{0x3904, 0x04},
	{0x3908, 0x41},
	{0x391d, 0x14},
	{0x391f, 0x18},
	{0x3e01, 0x8c},
	{0x3e02, 0x20},
	{0x3e16, 0x00},
	{0x3e17, 0x80},
	{0x3f09, 0x48},
	{0x5787, 0x10},
	{0x5788, 0x06},
	{0x578a, 0x10},
	{0x578b, 0x06},
	{0x5790, 0x10},
	{0x5791, 0x10},
	{0x5792, 0x00},
	{0x5793, 0x10},
	{0x5794, 0x10},
	{0x5795, 0x00},
	{0x5799, 0x00},
	{0x57c7, 0x10},
	{0x57c8, 0x06},
	{0x57ca, 0x10},
	{0x57cb, 0x06},
	{0x57d1, 0x10},
	{0x57d4, 0x10},
	{0x57d9, 0x00},
	{0x59e0, 0x60},
	{0x59e1, 0x08},
	{0x59e2, 0x3f},
	{0x59e3, 0x18},
	{0x59e4, 0x18},
	{0x59e5, 0x3f},
	{0x59e6, 0x06},
	{0x59e7, 0x02},
	{0x59e8, 0x38},
	{0x59e9, 0x10},
	{0x59ea, 0x0c},
	{0x59eb, 0x10},
	{0x59ec, 0x04},
	{0x59ed, 0x02},
	{0x59ee, 0xa0},
	{0x59ef, 0x08},
	{0x59f4, 0x18},
	{0x59f5, 0x10},
	{0x59f6, 0x0c},
	{0x59f7, 0x10},
	{0x59f8, 0x06},
	{0x59f9, 0x02},
	{0x59fa, 0x18},
	{0x59fb, 0x10},
	{0x59fc, 0x0c},
	{0x59fd, 0x10},
	{0x59fe, 0x04},
	{0x59ff, 0x02},
	{0x36e9, 0x51},
	{0x36f9, 0x53},
	{0x0100, 0x01},
};

/* 24M 30fps wdr */
static struct regval_list sensor_1080p30_hdr_regs[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x36e9, 0x80},
	{0x36f9, 0x80},
	{0x301f, 0x1e},
	{0x320e, 0x08},  /* vts=2250 */
	{0x320f, 0xca},
	{0x3220, 0x53},
	{0x3243, 0x01},
	{0x3248, 0x02},
	{0x3249, 0x09},
	{0x3250, 0x3f},
	{0x3253, 0x08},
	{0x3271, 0x0a},
	{0x3301, 0x06},
	{0x3302, 0x0c},
	{0x3303, 0x08},
	{0x3304, 0x60},
	{0x3306, 0x30},
	{0x3308, 0x10},
	{0x3309, 0x70},
	{0x330b, 0x80},
	{0x330d, 0x16},
	{0x330e, 0x1c},
	{0x330f, 0x02},
	{0x3310, 0x02},
	{0x331c, 0x04},
	{0x331e, 0x51},
	{0x331f, 0x61},
	{0x3320, 0x07},
	{0x3333, 0x10},
	{0x334c, 0x08},
	{0x3356, 0x09},
	{0x3364, 0x17},
	{0x3390, 0x08},
	{0x3391, 0x18},
	{0x3392, 0x38},
	{0x3393, 0x06},
	{0x3394, 0x06},
	{0x3395, 0x06},
	{0x3396, 0x08},
	{0x3397, 0x18},
	{0x3398, 0x38},
	{0x3399, 0x06},
	{0x339a, 0x0a},
	{0x339b, 0x10},
	{0x339c, 0x20},
	{0x33ac, 0x08},
	{0x33ae, 0x10},
	{0x33af, 0x19},
	{0x3621, 0xe8},
	{0x3622, 0x16},
	{0x3630, 0xa0},
	{0x3637, 0x36},
	{0x363a, 0x1f},
	{0x363b, 0xc6},
	{0x363c, 0x0e},
	{0x3670, 0x0a},
	{0x3674, 0x82},
	{0x3675, 0x76},
	{0x3676, 0x78},
	{0x367c, 0x48},
	{0x367d, 0x58},
	{0x3690, 0x34},
	{0x3691, 0x33},
	{0x3692, 0x44},
	{0x369c, 0x40},
	{0x369d, 0x48},
	{0x36ea, 0x75},
	{0x36eb, 0x0c},
	{0x36ec, 0x0c},
	{0x36ed, 0x24},
	{0x36fa, 0xea},
	{0x36fb, 0x00},
	{0x36fc, 0x10},
	{0x36fd, 0x04},
	{0x3901, 0x02},
	{0x3904, 0x04},
	{0x3908, 0x41},
	{0x391f, 0x10},
	{0x3e00, 0x01},
	{0x3e01, 0x06},
	{0x3e02, 0x00},
	{0x3e04, 0x10},
	{0x3e05, 0x60},
	{0x3e06, 0x00},
	{0x3e07, 0x80},
	{0x3e08, 0x03},
	{0x3e09, 0x40},
	{0x3e10, 0x00},
	{0x3e11, 0x80},
	{0x3e12, 0x03},
	{0x3e13, 0x40},
	{0x3e16, 0x00},
	{0x3e17, 0x80},
	{0x3e23, 0x00},
	{0x3e24, 0x88},
	{0x3f09, 0x48},
	{0x4816, 0xb1},
	{0x4819, 0x09},
	{0x481b, 0x05},
	{0x481d, 0x14},
	{0x481f, 0x04},
	{0x4821, 0x0a},
	{0x4823, 0x05},
	{0x4825, 0x04},
	{0x4827, 0x05},
	{0x4829, 0x08},
	{0x5787, 0x10},
	{0x5788, 0x06},
	{0x578a, 0x10},
	{0x578b, 0x06},
	{0x5790, 0x10},
	{0x5791, 0x10},
	{0x5792, 0x00},
	{0x5793, 0x10},
	{0x5794, 0x10},
	{0x5795, 0x00},
	{0x5799, 0x00},
	{0x57c7, 0x10},
	{0x57c8, 0x06},
	{0x57ca, 0x10},
	{0x57cb, 0x06},
	{0x57d1, 0x10},
	{0x57d4, 0x10},
	{0x57d9, 0x00},
	{0x59e0, 0x60},
	{0x59e1, 0x08},
	{0x59e2, 0x3f},
	{0x59e3, 0x18},
	{0x59e4, 0x18},
	{0x59e5, 0x3f},
	{0x59e6, 0x06},
	{0x59e7, 0x02},
	{0x59e8, 0x38},
	{0x59e9, 0x10},
	{0x59ea, 0x0c},
	{0x59eb, 0x10},
	{0x59ec, 0x04},
	{0x59ed, 0x02},
	{0x59ee, 0xa0},
	{0x59ef, 0x08},
	{0x59f4, 0x18},
	{0x59f5, 0x10},
	{0x59f6, 0x0c},
	{0x59f7, 0x10},
	{0x59f8, 0x06},
	{0x59f9, 0x02},
	{0x59fa, 0x18},
	{0x59fb, 0x10},
	{0x59fc, 0x0c},
	{0x59fd, 0x10},
	{0x59fe, 0x04},
	{0x59ff, 0x02},
	{0x36e9, 0x51},
	{0x36f9, 0x53},
	{0x0100, 0x01},

};

///////////////////////////////////////////////

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

static int sc200ai_sensor_vts;
static int sc200ai_sensor_svr;
static int shutter_delay = 1;
static int shutter_delay_cnt;
static int fps_change_flag;

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	data_type explow, expmid, exphigh;
	struct sensor_info *info = to_state(sd);

	if (info->isp_wdr_mode == ISP_DOL_WDR_MODE) {
		if (exp_val < 16 * HDR_RATIO)
			exp_val = 16 * HDR_RATIO;


		expmid = (unsigned char) (0xff & (exp_val>>7));
		explow = (unsigned char) (0xf0 & (exp_val<<1));

		sensor_write(sd, 0x3e02, explow);  /* long exposure */
		sensor_write(sd, 0x3e01, expmid);

		sensor_dbg("sensor_set_long_exp = %d line Done!\n", exp_val);

		exp_val /= HDR_RATIO;
		expmid = (unsigned char) (0xff & (exp_val>>7));
		explow = (unsigned char) (0xf0 & (exp_val<<1));

		sensor_write(sd, 0x3e05, explow);  /* short exposure */
		sensor_write(sd, 0x3e04, expmid);

		sensor_dbg("sensor_set_short_exp = %d line Done!\n", exp_val);

	} else {
		if (exp_val < 16)
			exp_val = 16;
		exphigh = (unsigned char) (0xf & (exp_val>>15));
		expmid = (unsigned char) (0xff & (exp_val>>7));
		explow = (unsigned char) (0xf0 & (exp_val<<1));

		sensor_write(sd, 0x3e02, explow);
		sensor_write(sd, 0x3e01, expmid);
		sensor_write(sd, 0x3e00, exphigh);
		sensor_dbg("sensor_set_exp = %d line Done!\n", exp_val);
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

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	struct sensor_info *info = to_state(sd);
	data_type gainlow = 0;
	data_type gainhigh = 0;
	data_type gaindiglow = 0x80;
	data_type gaindighigh = 0x00;

	int gainana = gain_val << 2;

	if (gainana < 2 * 64) {  /* 2*64 */
		gainhigh = 0x03;
		gainlow = gainana;
		gaindighigh = 0x00;
		gaindiglow = 0x80;
	} else if (gainana <= 217) {/* 3.4 * 64 =217.6 */
		gainhigh = 0x07;
		gainlow = gainana >> 1;
		gaindighigh = 0x00;
		gaindiglow = 0x80;
	} else if (gainana <= 435) { /* 6.8 * 64=435.2 */
		gainhigh = 0x23;
		gainlow = gainana * 10 / (34 * 1);
		gaindighigh = 0x00;
		gaindiglow = 0x80;
	} else if (gainana <= 870) { /* 13.6 * 64=870.4 */
		gainhigh = 0x27;
		gainlow = gainana * 10 / (34 * 2);
		gaindighigh = 0x00;
		gaindiglow = 0x80;
	} else if (gainana <= 1740) { /* 27.2 * 64 =1740.8 */
		gainhigh = 0x2F;
		gainlow = gainana * 10 / (34 * 4);
		gaindighigh = 0x00;
		gaindiglow = 0x80;
	} else if (gainana < 3455) { /* 53.975 * 64 =3454.4 */
		gainhigh = 0x3F;
		gainlow = gainana * 10 / (34 * 8);
		gaindighigh = 0x00;
		gaindiglow = 0x80;
	} else if (gainana < 2 * 3455) {/* 53.975 * 64 digital_gain:*2 */
		gainhigh = 0x3F;
		gainlow = 0x7F;
		gaindighigh = 0x00;
		gaindiglow = 128 * gainana * 10 / (34 * 8 * 1) / 127;

	} else {
		gainhigh = 0x3F;
		gainlow = 0x7F;
		gaindighigh = 0x00;
		gaindiglow = 0xFE;
	}


/*
	/* digital_gain: *4 - *32

	else if (gainana < 4 * 3455) {/* 53.975*64  digital_gain: *4
		 gainhigh = 0x3F;
		 gainlow = 0x7F;
		 gaindighigh = 0x01;
		 gaindiglow = 128 * gainana * 10 / (34 * 8 * 2) / 127;

	} else if (gainana < 8 * 3455) {/* 53.975*64 *8
		 gainhigh = 0x3F;
		 gainlow = 0x7F;
		 gaindighigh = 0x03;
		 gaindiglow = 128 * gainana * 10 / (34 * 8 * 4) / 127;
	} else if (gainana < 16 * 3455) {/* 53.975*64 digital_gain: *16
		 gainhigh = 0x3F;
		 gainlow = 0x7F;
		 gaindighigh = 0x07;
		 gaindiglow = 128 * gainana * 10 / (34 * 8 * 8) / 127;
	} else if (gainana <= 31.75 * 3455) {/* 53.975*64 digital_gain: *32
		 gainhigh = 0x3F;
		 gainlow = 0x7F;
		 gaindighigh = 0x0f;
		 gaindiglow = 128 * gainana * 10 / (34 * 8 * 16) / 127;
	} else {
		 gainhigh = 0x3F;
		 gainlow = 0x7F;
		 gaindighigh = 0x0f;
		 gaindiglow = 0xfe;
	}

*/

	if (info->isp_wdr_mode == ISP_DOL_WDR_MODE) {
		sensor_write(sd, 0x3e13, (unsigned char)gainlow);
		sensor_write(sd, 0x3e12, (unsigned char)gainhigh);
		sensor_write(sd, 0x3e11, (unsigned char)gaindiglow);
		sensor_write(sd, 0x3e10, (unsigned char)gaindighigh);
	}

	sensor_write(sd, 0x3e09, (unsigned char)gainlow);
	sensor_write(sd, 0x3e08, (unsigned char)gainhigh);
	sensor_write(sd, 0x3e07, (unsigned char)gaindiglow);
	sensor_write(sd, 0x3e06, (unsigned char)gaindighigh);

	sensor_dbg("sensor_set_anagain = %d, 0x%x, 0x%x Done!\n", gain_val, gainhigh, gainlow);
	sensor_dbg("digital_gain = 0x%x, 0x%x Done!\n", gaindighigh, gaindiglow);
	info->gain = gain_val;

	return 0;
}

static int DPC_frame_cnt;
static int DPC_Flag = 1;
static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	struct sensor_info *info = to_state(sd);
	int exp_val, gain_val;
	unsigned short R = 0;
	data_type R_high, R_low;

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;
	/* sensor_print(" %s :  exp_val = %ld ; gain_val = %ld\n", __FUNCTION__, exp_gain->exp_val, exp_gain->gain_val); */
	if (gain_val < 1 * 16)
		gain_val = 16;
	if (exp_val > 0xfffff)
		exp_val = 0xfffff;
	if (fps_change_flag) {
		if (shutter_delay_cnt == shutter_delay) {
			shutter_delay_cnt = 0;
			fps_change_flag = 0;
		} else
			shutter_delay_cnt++;
	}


	/////////////////////////////////////////////////////
	/////high-temp DPC logic/////
	if (++DPC_frame_cnt >= 3) {
		DPC_frame_cnt = 0;
		sensor_read(sd, 0x3974, &R_high);
		sensor_read(sd, 0x3975, &R_low);
		R = (R_high << 8) | R_low;

		if (R > 0x2040 || ((gain_val / 16) >= 53)) {
			if (R > 0x2040) {
				if (DPC_Flag != 2) {
					sensor_write(sd, 0x3812, 0x00);
					sensor_write(sd, 0x5787, 0x00);
					sensor_write(sd, 0x5788, 0x00);
					sensor_write(sd, 0x5790, 0x00);
					sensor_write(sd, 0x5791, 0x00);
					sensor_write(sd, 0x5799, 0x07);
					sensor_write(sd, 0x3812, 0x30);
					DPC_Flag = 2;
				}
			} else {
				if (DPC_Flag != 3) {
					sensor_write(sd, 0x3812, 0x00);
					sensor_write(sd, 0x5787, 0x10);
					sensor_write(sd, 0x5788, 0x06);
					sensor_write(sd, 0x5790, 0x10);
					sensor_write(sd, 0x5791, 0x10);
					sensor_write(sd, 0x5799, 0x07);
					sensor_write(sd, 0x3812, 0x30);
					DPC_Flag = 3;
				}
			}
		} else if (R < 0x2030 && ((gain_val / 16) < 53) && 1 != DPC_Flag) {
			sensor_write(sd, 0x3812, 0x00);
			sensor_write(sd, 0x5787, 0x10);
			sensor_write(sd, 0x5788, 0x06);
			sensor_write(sd, 0x5790, 0x10);
			sensor_write(sd, 0x5791, 0x10);
			sensor_write(sd, 0x5799, 0x00);
			sensor_write(sd, 0x3812, 0x30);
			DPC_Flag = 1;
		}
	}



	/* sensor_write(sd, 0x3812, 0x00); */
	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);
	/* sensor_write(sd, 0x3812, 0x30); */

	sensor_dbg("sensor_set_gain exp = %d, %d Done!\n", gain_val, exp_val);

	info->exp = exp_val;
	info->gain = gain_val;
	return 0;

}


static int sensor_s_fps(struct v4l2_subdev *sd,
			struct sensor_fps *fps)
{
	data_type rdval1, rdval2, rdval3;
	struct sensor_info *info = to_state(sd);
	struct sensor_win_size *wsize = info->current_wins;

	sc200ai_sensor_vts = wsize->pclk/fps->fps/wsize->hts;
	fps_change_flag = 1;

/*	sensor_read(sd, 0x30f8, &rdval1);
	sensor_read(sd, 0x30f9, &rdval2);
	sensor_read(sd, 0x30fa, &rdval3); */

	sensor_dbg("sc200ai_sensor_svr: %d, vts: %d.\n", sc200ai_sensor_svr, (rdval1 | (rdval2<<8) | (rdval3<<16)));
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
	data_type rdval = 0;

	sensor_read(sd, 0x3107, &rdval);
//	if (rdval != (V4L2_IDENT_SENSOR>>8))
//		return -ENODEV;
	sensor_print("0x3107 = 0x%x\n", rdval);
	sensor_read(sd, 0x3108, &rdval);
//	if (rdval != (V4L2_IDENT_SENSOR&0xff))
//		return -ENODEV;
	sensor_print("0x3108 = 0x%x\n", rdval);
	sensor_read(sd, 0x3e03, &rdval);
	sensor_print("0x3e03 = 0x%x\n", rdval);

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
	 .width = 1920,
	 .height = 1080,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 2200,
	 .vts = 1125,
	 .pclk = 74250000,
	 .mipi_bps = 371250000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .intg_min = 1 << 4,
	 .intg_max = (2*1125 - 8) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 128 << 4,
	 .regs = sensor_1080p30_regs,
	 .regs_size = ARRAY_SIZE(sensor_1080p30_regs),
	 .set_size = NULL,
	},

	{
	 .width = 1920,
	 .height = 1080,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 2200,
	 .vts = 2250,
	 .pclk = 148500000,
	 .mipi_bps = 742500000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .if_mode = MIPI_VC_WDR_MODE,
	 .wdr_mode = ISP_DOL_WDR_MODE,
	 .intg_min = 4 << 4,
	 .intg_max = (2*2250 - 2*136 - 10) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 128 << 4,
	 .regs = sensor_1080p30_hdr_regs,
	 .regs_size = ARRAY_SIZE(sensor_1080p30_hdr_regs),
	 .set_size = NULL,
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
	}
	return -EINVAL;
}

static int sensor_reg_init(struct sensor_info *info)
{
	int ret;
	data_type rdval_l, rdval_h;
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
	sc200ai_sensor_vts = wsize->vts;
//	sensor_read(sd, 0x300E, &rdval_l);
//	sensor_read(sd, 0x300F, &rdval_h);
//	sc200ai_sensor_svr = (rdval_h << 8) | rdval_l;

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

	v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 1600,
			      256 * 1600, 1, 1 * 1600);
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
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET1 | MIPI_NORMAL_MODE;
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->af_first_flag = 1;
	/* info->time_hs = 0x23; */
	info->time_hs = 0x09;
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
