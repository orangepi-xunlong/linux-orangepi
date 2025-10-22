/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for C2399 Raw cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *    Yang Feng <yangfeng@allwinnertech.com>
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

MODULE_AUTHOR("xiongbiao");
MODULE_DESCRIPTION("A low-level driver for C2399 Raw sensors");
MODULE_LICENSE("GPL");

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR  0x020B

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The c2399 i2c address
 */

#define C2399_WRITE_ADDR (0x6c)
#define C2399_READ_ADDR  (0x6d)

#define SENSOR_NAME "c2399_mipi"

struct cfg_array {		/* coming later */
	struct regval_list *regs;
	int size;
};

/*
 * The default register settings
 *
 */

static struct regval_list sensor_default_regs[] = { };

static struct regval_list sensor_1080p_regs[] = {
	{0x0103, 0x01},
	{REG_DLY, 0x40},/* add by cista_yjp,2020-1112 */

	{0x3288, 0x50}, /* tm */
	{0x0401, 0x3b},
	{0x0403, 0x00},

	{0x3298, 0x42},
	{0x3290, 0x69},
	{0x3286, 0x77},
	{0x32aa, 0x0f},
	{0x3295, 0x02},
	{0x328b, 0x20},

	{0x3211, 0x16},
	{0x3216, 0x28},
	{0x3217, 0x28},
	{0x3224, 0x99},
	{0x3212, 0x80},

	{0x3d00, 0x33},
	{0x3187, 0x07},
	{0x3222, 0x82},
	{0x322c, 0x04},
	{0x3c01, 0x13},

	{0x3087, 0xb0}, /* aec off 0xb0,test by cista_yjp,2020-1111 */

	{0x3584, 0x02},
	{0x3108, 0xef},
	{0x3112, 0xe0},
	{0x3115, 0x00},
	{0x3118, 0x50},
	{0x3118, 0x50},
	{0x311b, 0x01},
	{0x3126, 0x02},

	{0x3584, 0x22},
	{0x3009, 0x05},
	{0x022d, 0x1f},
	{0x0101, 0x01},
	{0x0340, 0x04},
	{0x0341, 0x50},
	{0x0342, 0x07},
	{0x0343, 0xbc},

	/* AEC/AWB  }, */
	{0x3082, 0x00},
	{0x3083, 0x0a},
	{0x3084, 0x80}, /* light target */
	/* 0x3087,0x},d0 // 30/31: 50/60 AEC on f0: aec off */
	{0x3089, 0x10}, /* 0x1c again controlled by reg3293/32a9 */

	{0x3f10, 0x00}, /* avg ROI off */

	/* mipi     }, */
	{0x0303, 0x01},
	{0x0309, 0x20},
	{0x0307, 0x52},
	{0x3805, 0x07},
	{0x3806, 0x06},
	{0x3807, 0x06},
	{0x3808, 0x16},
	{0x3809, 0x85},
	{0x380b, 0xaa},
	{0x380e, 0x31},

	{0x3600, 0xc8},
	{0x3602, 0x01},
	{0x3605, 0x22},
	{0x3607, 0x22},
	{0x3583, 0x10},
	{0x3584, 0x02},
	{0xa000, 0x0b},
	{0xa001, 0x50},
	{0xa002, 0x80},
	{0xa003, 0x00},
	{0xa004, 0x00},
	{0xa005, 0x80},
	{0xa006, 0x80},
	{0xa007, 0x80},
	{0xa008, 0x4e},
	{0xa009, 0x01},
	{0xa00a, 0x03},
	{0xa00b, 0x1c},
	{0xa00c, 0x83},
	{0xa00d, 0xa5},
	{0xa00e, 0x8c},
	{0xa00f, 0x81},
	{0xa010, 0x0a},
	{0xa011, 0x90},
	{0xa012, 0x0d},
	{0xa013, 0x81},
	{0xa014, 0x0c},
	{0xa015, 0x50},
	{0xa016, 0x70},
	{0xa017, 0x50},
	{0xa018, 0xa9},
	{0xa019, 0xe1},
	{0xa01a, 0x05},
	{0xa01b, 0x81},
	{0xa01c, 0xb0},
	{0xa01d, 0xe0},
	{0xa01e, 0x86},
	{0xa01f, 0x01},
	{0xa020, 0x08},
	{0xa021, 0x10},
	{0xa022, 0x8f},
	{0xa023, 0x81},
	{0xa024, 0x88},
	{0xa025, 0x90},
	{0xa026, 0x8f},
	{0xa027, 0x01},
	{0xa028, 0xd0},
	{0xa029, 0xe1},
	{0xa02a, 0x0f},
	{0xa02b, 0x84},
	{0xa02c, 0x03},
	{0xa02d, 0x38},
	{0xa02e, 0x1a},
	{0xa02f, 0x50},
	{0xa030, 0x8e},
	{0xa031, 0x03},
	{0xa032, 0xaf},
	{0xa033, 0xd0},
	{0xa034, 0xb0},
	{0xa035, 0x61},
	{0xa036, 0x0f},
	{0xa037, 0x84},
	{0xa038, 0x83},
	{0xa039, 0x38},
	{0xa03a, 0xa0},
	{0xa03b, 0x50},
	{0xa03c, 0x90},
	{0xa03d, 0xe0},
	{0xa03e, 0x2e},
	{0xa03f, 0xd0},
	{0xa040, 0x90},
	{0xa041, 0x61},
	{0xa042, 0x8f},
	{0xa043, 0x84},
	{0xa044, 0x03},
	{0xa045, 0x38},
	{0xa046, 0xa6},
	{0xa047, 0xd0},
	{0xa048, 0x20},
	{0xa049, 0x60},
	{0xa04a, 0x2e},
	{0xa04b, 0x50},
	{0xa04c, 0x70},
	{0xa04d, 0xe0},
	{0xa04e, 0x0f},
	{0xa04f, 0x04},
	{0xa050, 0x03},
	{0xa051, 0x38},
	{0xa052, 0xac},
	{0xa053, 0xd0},
	{0xa054, 0x30},
	{0xa055, 0x60},
	{0xa056, 0xae},
	{0xa057, 0x50},
	{0xa058, 0x8f},
	{0xa059, 0x10},
	{0xa05a, 0xf0},
	{0xa05b, 0x79},
	{0xa05c, 0x8e},
	{0xa05d, 0x01},
	{0xa05e, 0x27},
	{0xa05f, 0xe0},
	{0xa060, 0x05},
	{0xa061, 0x01},
	{0xa062, 0x31},
	{0xa063, 0x60},
	{0xa064, 0x86},
	{0xa065, 0x01},
	{0xa066, 0x0e},
	{0xa067, 0x90},
	{0xa068, 0x08},
	{0xa069, 0x01},
	{0xa06a, 0xe0},
	{0xa06b, 0xe1},
	{0xa06c, 0x8f},
	{0xa06d, 0x04},
	{0xa06e, 0x8a},
	{0xa06f, 0xe0},
	{0xa070, 0x03},
	{0xa071, 0x38},
	{0xa072, 0xc6},
	{0xa073, 0xd0},
	{0xa074, 0x85},
	{0xa075, 0x81},
	{0xa076, 0xb1},
	{0xa077, 0xe0},
	{0xa078, 0x86},
	{0xa079, 0x01},
	{0xa07a, 0xa0},
	{0xa07b, 0xe1},
	{0xa07c, 0x08},
	{0xa07d, 0x81},
	{0xa07e, 0x0b},
	{0xa07f, 0x60},
	{0xa080, 0xe5},
	{0xa081, 0xc0},
	{0xa082, 0x8c},
	{0xa083, 0x60},
	{0xa084, 0x65},
	{0xa085, 0xc0},
	{0xa086, 0x8d},
	{0xa087, 0xe0},
	{0xa088, 0xe5},
	{0xa089, 0x40},
	{0xa08a, 0x59},
	{0xa08b, 0x50},
	{0xa08c, 0x05},
	{0xa08d, 0x81},
	{0xa08e, 0x31},
	{0xa08f, 0xe0},
	{0xa090, 0x86},
	{0xa091, 0x01},
	{0xa092, 0x08},
	{0xa093, 0x83},
	{0xa094, 0x0b},
	{0xa095, 0xe0},
	{0xa096, 0x05},
	{0xa097, 0x81},
	{0xa098, 0xb1},
	{0xa099, 0x60},
	{0xa09a, 0x86},
	{0xa09b, 0x01},
	{0xa09c, 0x88},
	{0xa09d, 0x83},
	{0xa09e, 0x0c},
	{0xa09f, 0xe0},
	{0xa0a0, 0x05},
	{0xa0a1, 0x01},
	{0xa0a2, 0xb1},
	{0xa0a3, 0xe0},
	{0xa0a4, 0x06},
	{0xa0a5, 0x81},
	{0xa0a6, 0x88},
	{0xa0a7, 0x03},
	{0xa0a8, 0x8d},
	{0xa0a9, 0x60},
	{0xa0aa, 0x05},
	{0xa0ab, 0x01},
	{0xa0ac, 0x31},
	{0xa0ad, 0xe0},
	{0xa0ae, 0x86},
	{0xa0af, 0x81},
	{0xa0b0, 0x08},
	{0xa0b1, 0x03},
	{0xa0b2, 0x90},
	{0xa0b3, 0xe0},
	{0xa0b4, 0x85},
	{0xa0b5, 0x81},
	{0xa0b6, 0x36},
	{0xa0b7, 0x60},
	{0xa0b8, 0x06},
	{0xa0b9, 0x81},
	{0xa0ba, 0x88},
	{0xa0bb, 0x03},
	{0xa0bc, 0x0d},
	{0xa0bd, 0x10},
	{0xa0be, 0x8a},
	{0xa0bf, 0x01},
	{0xa0c0, 0x0c},
	{0xa0c1, 0x9c},
	{0xa0c2, 0x03},
	{0xa0c3, 0x81},
	{0xa0c4, 0xce},
	{0xa0c5, 0x9d},
	{0xa0c6, 0x4e},
	{0xa0c7, 0x1c},
	{0xa0c8, 0x09},
	{0xa0c9, 0x80},
	{0xa0ca, 0x05},
	{0xa0cb, 0x81},
	{0xa0cc, 0x31},
	{0xa0cd, 0xe0},
	{0xa0ce, 0x06},
	{0xa0cf, 0x01},
	{0xa0d0, 0x20},
	{0xa0d1, 0x61},
	{0xa0d2, 0x88},
	{0xa0d3, 0x81},
	{0xa0d4, 0x08},
	{0xa0d5, 0x00},
	{0xa0d6, 0x83},
	{0xa0d7, 0x2d},
	{0xa0d8, 0x01},
	{0xa0d9, 0xae},
	{0xa0da, 0x8b},
	{0xa0db, 0x2c},
	{0xa0dc, 0x0b},
	{0xa0dd, 0x2f},
	{0xa0de, 0xef},
	{0xa0df, 0x50},
	{0xa0e0, 0x83},
	{0xa0e1, 0x83},
	{0xa0e2, 0xeb},
	{0xa0e3, 0x50},
	{0x3583, 0x00},
	{0x3584, 0x22},

	{0x0101, 0x01}, /* bayer=BGGR */
	{0x3009, 0x05},
	{0x300b, 0x10},

	{0x0202, 0x00},/* test del by cista_yjp,2020-1111 */
	{0x0203, 0x04},
	{0x0205, 0x00},

	{0x0100, 0x01}, /* stream on */
};

/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */

static struct regval_list sensor_fmt_raw[] = {
};

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
	sensor_dbg("@@@CST:sensor_s_exp = %d\n", exp_val);

	 exp_val = (exp_val)>>4;/* rounding to 1 */
	if (exp_val > 0xffffff)
		exp_val = 0xfffff0;
	if (exp_val < 2)
		exp_val = 2;

	sensor_write(sd, 0x0202, (exp_val>>8) & 0x00ff);
	sensor_write(sd, 0x0203, exp_val & 0x00ff);

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

unsigned char Tab_C2399RegToGain[233] = {
	 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,
	 27,  28,  29,  30,  31,  32,  32,  34,  34,  36,  36,
	 38,  38,  40,  40,  42,  42,  44,  44,  46,  46,  48,
	 48,  50,  50,  52,  52,  54,  54,  56,  56,  58,  58,
	 60,  60,  62,  62,  64,  64,  64,  64,  68,  68,  68,
	 68,  72,  72,  72,  72,  76,  76,  76,  76,  80,  80,
	 80,  80,  84,  84,  84,  84,  88,  88,  88,  88,  92,
	 92,  92,  92,  96,  96,  96,  96,
	100, 100, 100, 100, 104, 104, 104, 104,
	108, 108, 108, 108, 112, 112, 112, 112, 116, 116, 116,
	116, 120, 120, 120, 120, 124, 124, 124,	124, 128, 128,
	128, 128, 128, 128, 128, 128, 136, 136,	136, 136, 136,
	136, 136, 136, 144, 144, 144, 144, 144,	144, 144, 144,
	152, 152, 152, 152, 152, 152, 152, 152,	160, 160, 160,
	160, 160, 160, 160, 160, 168, 168, 168,	168, 168, 168,
	168, 168, 176, 176, 176, 176, 176, 176,	176, 176, 184,
	184, 184, 184, 184, 184, 184, 184, 192,	192, 192, 192,
	192, 192, 192, 192, 200, 200, 200, 200,	200, 200, 200,
	200, 208, 208, 208, 208, 208, 208, 208,	208, 216, 216,
	216, 216, 216, 216, 216, 216, 224, 224,	224, 224, 224,
	224, 224, 224, 232, 232, 232, 232, 232,	232, 232, 232,
	240, 240, 240, 240, 240, 240, 240, 240,	248,
};

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	struct sensor_info *info = to_state(sd);
	unsigned char A_value = 0;
	unsigned int D_value = 0x100;
	unsigned int i;

	if (gain_val < 16)
		gain_val = 16;

	sensor_dbg("@@@CST:sensor_s_gain = %d\n", gain_val);
	if (gain_val >= Tab_C2399RegToGain[232]) {
		A_value	 = 232;
		D_value = (gain_val << 5)/31; /* d_value = (gain_val << 4)/15.5; <<4 mean 4bit to 8bit */
		if (D_value > 0xfff) {
			D_value = 0xfff;
		}
	} else {
		for (i = 1; i < 233; i++) {
			if (gain_val < Tab_C2399RegToGain[i]) {
				A_value = i-1;
				D_value = (gain_val<<8) / Tab_C2399RegToGain[A_value];
				break;
			}
		}
	}

	sensor_dbg("@@@CST:sensor_s_Again_0205 = %d\n", A_value);
	sensor_write(sd, 0x0205, A_value);     /* A-GIAN */
	sensor_dbg("@@@CST:sensor_s_Dgain_0216_0217 = %d\n", D_value);
	sensor_write(sd, 0x0216, (D_value>>8) & 0xff);/* D-GIAN */
	sensor_write(sd, 0x0217, D_value & 0xff);     /* D-GIAN */
	info->gain = gain_val;

	return 0;

}

static int c2399_sensor_vts;
static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val, shutter, frame_length;
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	sensor_dbg("@@@CST:sensor_s_exp_gain_input:exp_val = %d,gain_val = %d\n", exp_val, gain_val);

	if (gain_val < 1 * 16)
		gain_val = 16;
	if (gain_val > 64 * 16 - 1)
		gain_val = 64 * 16 - 1;

	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	shutter = exp_val / 16;
	sensor_dbg("@@@CST:sensor_s_exp_gain_shutter = %d\n", shutter);
	if (shutter > c2399_sensor_vts - 4)
		frame_length = shutter + 4;
	else
		frame_length = c2399_sensor_vts;

	sensor_dbg("@@@CST:sensor_s_exp_gain_ frame_length= %d\n", frame_length);
	sensor_write(sd, 0x0341, (frame_length & 0xff));
	sensor_write(sd, 0x0340, (frame_length >> 8));
	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);

	info->exp = exp_val;
	info->gain = gain_val;
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
		sensor_dbg("STBY_ON!\n");
		ret = sensor_s_sw_stby(sd, CSI_GPIO_HIGH);
		if (ret < 0)
			sensor_err("soft stby falied!\n");
		usleep_range(10000, 12000);
		cci_lock(sd);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		cci_unlock(sd);
		vin_set_mclk(sd, OFF);
		break;
	case STBY_OFF:
		sensor_dbg("STBY_OFF!\n");
		cci_lock(sd);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		ret = sensor_s_sw_stby(sd, CSI_GPIO_LOW);
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
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		vin_set_pmu_channel(sd, IOVDD, ON);
		usleep_range(1000, 1200);
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, AFVDD, ON);
		usleep_range(7000, 8000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(20000, 22000);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vin_set_mclk(sd, OFF);
		vin_set_pmu_channel(sd, AFVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_LOW);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
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

	sensor_read(sd, 0x0000, &rdval);

	if (rdval != (V4L2_IDENT_SENSOR >> 8)) {
		sensor_err("sensor error,read id is %d.\n", rdval);
		return -ENODEV;
	}

	sensor_read(sd, 0x0001, &rdval);
	if (rdval != (V4L2_IDENT_SENSOR & 0x00ff)) {
		sensor_err("sensor error,read id is %d.\n", rdval);
		return -ENODEV;
	} else {
		sensor_print("find c2399 raw data camera sensor now.\n");
		return 0;
	}
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
	info->width = HD1080_WIDTH;
	info->height = HD1080_HEIGHT;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30;	/* 30fps */

	info->preview_first_flag = 1;

	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);

	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		if (info->current_wins != NULL) {
			memcpy(arg,
			       info->current_wins,
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
	/* 1080P */
	{
	 .width = HD1080_WIDTH,
	 .height = HD1080_HEIGHT,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 1980,
	 .vts = 1104,
	 .pclk = 65.6 * 1000 * 1000,
	 .mipi_bps = 656 * 1000 * 1000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .intg_min = 1<<4,
	 .intg_max = 1100 << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 64 << 4,
	 .regs = sensor_1080p_regs,
	 .regs_size = ARRAY_SIZE(sensor_1080p_regs),
	 .set_size = NULL,
	 },
	/* 720P */
	{
	 .width = HD720_WIDTH,
	 .height = HD720_HEIGHT,
	 .hoffset = 320,
	 .voffset = 180,
	 .hts = 1980,
	 .vts = 1104,
	 .pclk = 65.6 * 1000 * 1000,
	 .mipi_bps = 656 * 1000 * 1000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .intg_min = 1,
	 .intg_max = 1100 << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 64 << 4,
	 .width_input = 1920,
	 .height_input = 1080,
	 .regs = sensor_1080p_regs,
	 .regs_size = ARRAY_SIZE(sensor_1080p_regs),
	 .set_size = NULL,
	 },
	/* VGA */
	{
	 .width = VGA_WIDTH,
	 .height = VGA_HEIGHT,
	 .hoffset = 240,
	 .voffset = 0,
	 .hts = 1980,
	 .vts = 1104,
	 .pclk = 65.6 * 1000 * 1000,
	 .mipi_bps = 656 * 1000 * 1000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .intg_min = 1,
	 .intg_max = 1100 << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 16 << 4,
	 .width_input = 1920,
	 .height_input = 1080,
	 .regs = sensor_1080p_regs,
	 .regs_size = ARRAY_SIZE(sensor_1080p_regs),
	 .set_size = NULL,
	 },
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_reg_init(struct sensor_info *info)
{

	int ret = 0;
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
	info->exp = 0;
	info->gain = 0;
	c2399_sensor_vts = wsize->vts;
	sensor_print("s_fmt set width = %d, height = %d\n", wsize->width,
		      wsize->height);

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);

	sensor_print("%s on = %d, %d*%d %x\n", __func__, enable,
		  info->current_wins->width,
		  info->current_wins->height, info->fmt->mbus_code);

	if (!enable) {
		sensor_write(sd, 0x0100, 0x01);	/* stream off,2020-1112-cista yjp */
		return 0;
	}
	return sensor_reg_init(info);/* stream on,2020-1112-cista yjp */
}

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

static int sensor_init_controls(struct v4l2_subdev *sd, const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	struct v4l2_ctrl *ctrl;
	int i;
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 2 + ARRAY_SIZE(sensor_custom_ctrls));

	v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 1600,
			      256 * 1600, 1, 1 * 1600);
	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 0,
			      65536 * 16, 1, 0);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
	for (i = 0; i < ARRAY_SIZE(sensor_custom_ctrls); i++)
		v4l2_ctrl_new_custom(handler, &sensor_custom_ctrls[i], NULL);

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
	if (info == NULL)
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
	info->af_first_flag = 1;

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
