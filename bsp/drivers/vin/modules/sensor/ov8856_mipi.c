/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for ov8856_4lane Raw cameras.
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

MODULE_AUTHOR("lwj");
MODULE_DESCRIPTION("A low-level driver for OV8856 sensors");
MODULE_LICENSE("GPL");

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR 0x8856
int ov8856_sensor_vts;
static int frame_length = 2482;

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The ov8856 sits on i2c with ID 0x6c
 */
#define I2C_ADDR 0x20

#define SENSOR_NUM	0x2
#define SENSOR_NAME "ov8856_mipi"
#define SENSOR_NAME_2 "ov8856_mipi_2"

struct cfg_array {		/* coming later */
	struct regval_list *regs;
	int size;
};

/*
 * The default register settings
 *
 */
static struct regval_list sensor_default_regs[] = {
	{0x0103, 0x01},
	{0x0302, 0x3c},
	{0x0303, 0x01},
	{0x3000, 0x00},
	{0x300e, 0x00},
	{0x3010, 0x00},
	{0x3015, 0x84},
	{0x3018, 0x72},		//  MIPI SC CTRL, bit[7:5]=0b011, 4 lane
	{0x3033, 0x24},
	{0x3500, 0x00},
	{0x3501, 0x9a},
	{0x3502, 0x20},
	{0x3503, 0x08},
	{0x3505, 0x83},
	{0x3508, 0x01},
	{0x3509, 0x80},
	{0x350c, 0x00},
	{0x350d, 0x80},
	{0x350e, 0x04},
	{0x350f, 0x00},
	{0x3510, 0x00},
	{0x3511, 0x02},
	{0x3512, 0x00},
	{0x3600, 0x72},
	{0x3601, 0x40},
	{0x3602, 0x30},
	{0x3610, 0xc5},
	{0x3611, 0x58},
	{0x3612, 0x5c},
	{0x3613, 0x5a},
	{0x3614, 0x60},
	{0x3628, 0xff},
	{0x3629, 0xff},
	{0x362a, 0xff},
	{0x3633, 0x10},
	{0x3634, 0x10},
	{0x3635, 0x10},
	{0x3636, 0x10},
	{0x3663, 0x08},
	{0x3669, 0x34},
	{0x366e, 0x10},
	{0x3733, 0x10},
	{0x3764, 0x00},
	{0x3765, 0x00},
	{0x3769, 0x42},
	{0x376a, 0x2a},
	{0x376b, 0x20},
	{0x3780, 0x00},
	{0x3781, 0x24},
	{0x3782, 0x00},
	{0x3783, 0x23},
	{0x3798, 0x2f},
	{0x37a1, 0x60},
	{0x37a8, 0x6a},
	{0x37ab, 0x3f},
	{0x37c2, 0x04},
	{0x37c3, 0xf1},
	{0x37c9, 0x80},
	{0x37cb, 0x03},
	{0x37cc, 0x0a},
	{0x37cd, 0x16},
	{0x37ce, 0x1f},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x0c},
	{0x3804, 0x0c},
	{0x3805, 0xdf},
	{0x3806, 0x09},
	{0x3807, 0xa3},
	{0x3808, 0x0c},
	{0x3809, 0xc0},
	{0x380a, 0x09},
	{0x380b, 0x90},
	{0x380c, 0x07},
	{0x380d, 0x8c},
	{0x380e, 0x09},
	{0x380f, 0xb2},
	{0x3810, 0x00},
	{0x3811, 0x10},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x01},
	{0x3815, 0x01},
	{0x3816, 0x00},
	{0x3817, 0x00},
	{0x3818, 0x00},
	{0x3819, 0x00},
	{0x3820, 0x80},
	{0x3821, 0x46},
	{0x382a, 0x01},
	{0x382b, 0x01},
	{0x3830, 0x06},
	{0x3836, 0x02},
	{0x3862, 0x04},
	{0x3863, 0x08},
	{0x3cc0, 0x33},
	{0x3d85, 0x17},
	{0x3d8c, 0x73},
	{0x3d8d, 0xde},
	{0x4001, 0xe0},
	{0x4003, 0x40},
	{0x4008, 0x00},
	{0x4009, 0x0b},
	{0x400f, 0x80},
	{0x4010, 0xf0},
	{0x4011, 0xff},
	{0x4012, 0x02},
	{0x4013, 0x01},
	{0x4014, 0x01},
	{0x4015, 0x01},
	{0x4042, 0x00},
	{0x4043, 0x80},
	{0x4044, 0x00},
	{0x4045, 0x80},
	{0x4046, 0x00},
	{0x4047, 0x80},
	{0x4048, 0x00},
	{0x4049, 0x80},
	{0x4041, 0x03},
	{0x404c, 0x20},
	{0x404d, 0x00},
	{0x404e, 0x80},
	{0x4203, 0x80},
	{0x4307, 0x30},
	{0x4317, 0x00},
	{0x4503, 0x08},
	{0x4601, 0x80},
	{0x4816, 0x53},
	{0x481f, 0x27},
	{0x4837, 0x16},
	{0x5000, 0x77},
	{0x5001, 0x0a},
	{0x5004, 0x04},
	{0x502e, 0x03},
	{0x5030, 0x41},
	{0x5795, 0x12},//; PD bypass enable
	{0x5796, 0x20},
	{0x5797, 0x20},
	{0x5798, 0xd5},
	{0x5799, 0xd5},
	{0x579a, 0x00},
	{0x579b, 0x50},
	{0x579c, 0x00},
	{0x579d, 0x2c},
	{0x579e, 0x0c},
	{0x579f, 0x40},
	{0x57a0, 0x09},
	{0x57a1, 0x40},
	{0x5780, 0x14},
	{0x5781, 0x0f},
	{0x5782, 0x44},
	{0x5783, 0x02},
	{0x5784, 0x01},
	{0x5785, 0x01},
	{0x5786, 0x00},
	{0x5787, 0x04},
	{0x5788, 0x02},
	{0x5789, 0x0f},
	{0x578a, 0xfd},
	{0x578b, 0xf5},
	{0x578c, 0xf5},
	{0x578d, 0x03},
	{0x578e, 0x08},
	{0x578f, 0x0c},
	{0x5790, 0x08},
	{0x5791, 0x04},
	{0x5792, 0x00},
	{0x5793, 0x52},
	{0x5794, 0xa3},
	{0x5a08, 0x02},
	{0x5b00, 0x02},
	{0x5b01, 0x10},
	{0x5b02, 0x03},
	{0x5b03, 0xcf},
	{0x5b05, 0x6c},
	{0x5e00, 0x00},
	//{0x0100, 0x01},
};

/*
static struct regval_list mode_1640x1232_regs[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x0302, 0x4b},
	{0x0303, 0x03},
	{0x030b, 0x02},
	{0x030d, 0x4b},
	{0x031e, 0x0c},

	{0x3000, 0x20},
	{0x3003, 0x08},
	{0x300e, 0x20},
	{0x3010, 0x00},
	{0x3015, 0x84},
	{0x3018, 0x72},
	{0x3021, 0x23},
	{0x3033, 0x24},
	{0x3500, 0x00},
	{0x3501, 0x4c},
	{0x3502, 0xe0},
	{0x3503, 0x08},
	{0x3505, 0x83},
	{0x3508, 0x01},
	{0x3509, 0x80},
	{0x350c, 0x00},
	{0x350d, 0x80},
	{0x350e, 0x04},
	{0x350f, 0x00},
	{0x3510, 0x00},
	{0x3511, 0x02},
	{0x3512, 0x00},
	{0x3600, 0x72},
	{0x3601, 0x40},
	{0x3602, 0x30},
	{0x3610, 0xc5},
	{0x3611, 0x58},
	{0x3612, 0x5c},
	{0x3613, 0xca},
	{0x3614, 0x20},
	{0x3628, 0xff},
	{0x3629, 0xff},
	{0x362a, 0xff},
	{0x3633, 0x10},
	{0x3634, 0x10},
	{0x3635, 0x10},
	{0x3636, 0x10},
	{0x3663, 0x08},
	{0x3669, 0x34},
	{0x366e, 0x08},
	{0x3706, 0x86},
	{0x370b, 0x7e},
	{0x3714, 0x27},
	{0x3730, 0x12},
	{0x3733, 0x10},
	{0x3764, 0x00},
	{0x3765, 0x00},
	{0x3769, 0x62},
	{0x376a, 0x2a},
	{0x376b, 0x30},
	{0x3780, 0x00},
	{0x3781, 0x24},
	{0x3782, 0x00},
	{0x3783, 0x23},
	{0x3798, 0x2f},
	{0x37a1, 0x60},
	{0x37a8, 0x6a},
	{0x37ab, 0x3f},
	{0x37c2, 0x14},
	{0x37c3, 0xf1},
	{0x37c9, 0x80},
	{0x37cb, 0x16},
	{0x37cc, 0x16},
	{0x37cd, 0x16},
	{0x37ce, 0x16},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x06},
	{0x3804, 0x0c},
	{0x3805, 0xdf},
	{0x3806, 0x09},
	{0x3807, 0xa7},
	{0x3808, 0x06},
	{0x3809, 0x68},
	{0x380a, 0x04},
	{0x380b, 0xd0},
	{0x380c, 0x0e},
	{0x380d, 0xec},
	{0x380e, 0x04},
	{0x380f, 0xe8},
	{0x3810, 0x00},
	{0x3811, 0x00},
	{0x3812, 0x00},
	{0x3813, 0x01},
	{0x3814, 0x03},
	{0x3815, 0x01},
	{0x3816, 0x00},
	{0x3817, 0x00},
	{0x3818, 0x00},
	{0x3819, 0x10},
	{0x3820, 0x90},
	{0x3821, 0x67},
	{0x382a, 0x03},
	{0x382b, 0x01},
	{0x3830, 0x06},
	{0x3836, 0x02},
	{0x3862, 0x04},
	{0x3863, 0x08},
	{0x3cc0, 0x33},
	{0x3d85, 0x17},
	{0x3d8c, 0x73},
	{0x3d8d, 0xde},
	{0x4001, 0xe0},
	{0x4003, 0x40},
	{0x4008, 0x00},
	{0x4009, 0x05},
	{0x400a, 0x00},
	{0x400b, 0x84},
	{0x400f, 0x80},
	{0x4010, 0xf0},
	{0x4011, 0xff},
	{0x4012, 0x02},
	{0x4013, 0x01},
	{0x4014, 0x01},
	{0x4015, 0x01},
	{0x4042, 0x00},
	{0x4043, 0x80},
	{0x4044, 0x00},
	{0x4045, 0x80},
	{0x4046, 0x00},
	{0x4047, 0x80},
	{0x4048, 0x00},
	{0x4049, 0x80},
	{0x4041, 0x03},
	{0x404c, 0x20},
	{0x404d, 0x00},
	{0x404e, 0x20},
	{0x4203, 0x80},
	{0x4307, 0x30},
	{0x4317, 0x00},
	{0x4503, 0x08},
	{0x4601, 0x80},
	{0x4800, 0x44},
	{0x4816, 0x53},
	{0x481b, 0x58},
	{0x481f, 0x27},
	{0x4837, 0x16},
	{0x483c, 0x0f},
	{0x484b, 0x05},
	{0x5000, 0x57},
	{0x5001, 0x0a},
	{0x5004, 0x04},
	{0x502e, 0x03},
	{0x5030, 0x41},
	{0x5780, 0x14},
	{0x5781, 0x0f},
	{0x5782, 0x44},
	{0x5783, 0x02},
	{0x5784, 0x01},
	{0x5785, 0x01},
	{0x5786, 0x00},
	{0x5787, 0x04},
	{0x5788, 0x02},
	{0x5789, 0x0f},
	{0x578a, 0xfd},
	{0x578b, 0xf5},
	{0x578c, 0xf5},
	{0x578d, 0x03},
	{0x578e, 0x08},
	{0x578f, 0x0c},
	{0x5790, 0x08},
	{0x5791, 0x04},
	{0x5792, 0x00},
	{0x5793, 0x52},
	{0x5794, 0xa3},
	{0x5795, 0x00},
	{0x5796, 0x10},
	{0x5797, 0x10},
	{0x5798, 0x73},
	{0x5799, 0x73},
	{0x579a, 0x00},
	{0x579b, 0x28},
	{0x579c, 0x00},
	{0x579d, 0x16},
	{0x579e, 0x06},
	{0x579f, 0x20},
	{0x57a0, 0x04},
	{0x57a1, 0xa0},
	{0x59f8, 0x3d},
	{0x5a08, 0x02},
	{0x5b00, 0x02},
	{0x5b01, 0x10},
	{0x5b02, 0x03},
	{0x5b03, 0xcf},
	{0x5b05, 0x6c},
	{0x5e00, 0x00},
	{0x0100, 0x01},
};
*/


/*
static struct regval_list sensor_quxga_regs[] = {
};
*/


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

	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	sensor_write(sd, 0x3502, (exp_val & 0xff));
	sensor_write(sd, 0x3501, (exp_val >> 8) & 0xff);
	sensor_write(sd, 0x3500, (exp_val >> 16) & 0xff);

	sensor_dbg("sensor_set_exp = %d\n", exp_val);

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

	if (gain_val < 1 * 16)
		gain_val = 16;
	if (gain_val > 15 * 16)
		gain_val = 15 * 16;

	gain_val = gain_val * 8;
	sensor_write(sd, 0x3509, (gain_val & 0xff));
	sensor_write(sd, 0x3508, ((gain_val >> 8) & 0x7));
	sensor_dbg("sensor_set_gain = %d\n", gain_val / 8);

	info->gain = gain_val / 8;
	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val, shutter;

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	if (gain_val < 1 * 16)
		gain_val = 16;
	if (gain_val > 64 * 16 - 1)
		gain_val = 64 * 16 - 1;

	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	shutter = exp_val / 16;
	if (shutter > ov8856_sensor_vts - 4)
		frame_length = shutter + 4;
	else
		frame_length = ov8856_sensor_vts;

	sensor_write(sd, 0x3208, 0x00);
	sensor_write(sd, 0x380f, (frame_length & 0xff));
	sensor_write(sd, 0x380e, (frame_length >> 8));
	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);
	sensor_write(sd, 0x3208, 0x10);
	sensor_write(sd, 0x3208, 0xa0);

	return 0;
}
static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	int ret;
	data_type rdval;

	ret = sensor_read(sd, 0x0100, &rdval);
	if (ret != 0)
		return ret;

	if (on_off == CSI_GPIO_LOW) {
		ret = sensor_write(sd, 0x0100, rdval & 0xfe);
	} else {
		ret = sensor_write(sd, 0x0100, rdval | 0x01);
	}
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
		ret = sensor_s_sw_stby(sd, CSI_GPIO_LOW);
		if (ret < 0)
			sensor_err("soft stby falied!\n");
		usleep_range(10000, 12000);
		cci_lock(sd);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		cci_unlock(sd);
		vin_set_mclk(sd, OFF);
		break;
	case STBY_OFF:
		sensor_dbg("STBY_OFF!\n");
		cci_lock(sd);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		ret = sensor_s_sw_stby(sd, CSI_GPIO_HIGH);
		if (ret < 0)
			sensor_err("soft stby off falied!\n");
		cci_unlock(sd);
		break;
	case PWR_ON:
		sensor_dbg("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_set_pmu_channel(sd, AFVDD, ON);
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		vin_set_pmu_channel(sd, CAMERAVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		usleep_range(5000, 6000);
		vin_set_pmu_channel(sd, DVDD, ON);

		usleep_range(11000, 13000);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");
		cci_lock(sd);
		vin_set_mclk(sd, OFF);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		vin_set_pmu_channel(sd, DVDD, OFF);
		usleep_range(5000, 6000);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, CAMERAVDD, OFF);
		usleep_range(5000, 6000);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		vin_set_pmu_channel(sd, AFVDD, OFF);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
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
	sensor_read(sd, 0x300a, &rdval);
	if (rdval != 0x00)
		return -ENODEV;
	sensor_read(sd, 0x300b, &rdval);
	if (rdval != 0x88)
		return -ENODEV;
	sensor_read(sd, 0x300c, &rdval);
	if (rdval != 0x5a)
		return -ENODEV;
	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_init\n");

	/*Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = QUXGA_WIDTH;
	info->height = QUXGA_HEIGHT;
	info->hflip = 0;
	info->vflip = 0;

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
	case VIDIOC_VIN_ACT_INIT:
		ret = actuator_init(sd, (struct actuator_para *)arg);
		break;
	case VIDIOC_VIN_ACT_SET_CODE:
		ret = actuator_set_code(sd, (struct actuator_ctrl *)arg);
		break;
	case VIDIOC_VIN_FLASH_EN:
		ret = flash_en(sd, (struct flash_para *)arg);
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
	 .width = 3264,
	 .height = 2448,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 1932,
	 .vts = 2482,
	 .pclk = 144 * 1000 * 1000,
	 .mipi_bps = 720 * 1000 * 1000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .intg_min = 16,
	 .intg_max = (2482 - 4) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 15 << 4,
	 .regs = sensor_default_regs,
	 .regs_size = ARRAY_SIZE(sensor_default_regs),
	 .set_size = NULL,
	 },

	// {
	// .width = 2592,
	// .height = 1944,
	// .hoffset = 336,
	// .voffset = 252,
	// .hts = 1932,
	// .vts = 2482,
	// .pclk = 144 * 1000 * 1000,
	// .mipi_bps = 720 * 1000 * 1000,
	// .fps_fixed = 30,
	// .bin_factor = 1,
	// .intg_min = 16,
	// .intg_max = (2482 - 4) << 4,
	// .gain_min = 1 << 4,
	// .gain_max = 15 << 4,
	// .regs = sensor_default_regs,
	// .regs_size = ARRAY_SIZE(sensor_default_regs),
	// .set_size = NULL,
	// },

	// {
	// .width = 1640,
	// .height = 1232,
	// .hoffset = 0,
	// .voffset = 0,
	// .hts = 3820,
	// .vts = 1256,
	// .pclk = 144 * 1000 * 1000,
	// .mipi_bps = 360 * 1000 * 1000,
	// .fps_fixed = 30,
	// .bin_factor = 1,
	// .intg_min = 16,
	// .intg_max = (1256 - 4) << 4,
	// .gain_min = 1 << 4,
	// .gain_max = 15 << 4,
	// .regs = mode_1640x1232_regs,
	// .regs_size = ARRAY_SIZE(mode_1640x1232_regs),
	// .set_size = NULL,
	// },

};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_CSI2_DPHY;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	cfg->bus.mipi_csi2.num_data_lanes = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
#else
	cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
#endif

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
	sensor_print("sensor_reg_init\n");

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);

	if (wsize->set_size)
		wsize->set_size(sd);

	info->width = wsize->width;
	info->height = wsize->height;
	ov8856_sensor_vts = wsize->vts;

	sensor_write(sd, 0x3208, 0x00);
	sensor_write(sd, 0x380f, (frame_length & 0xff));
	sensor_write(sd, 0x380e, (frame_length >> 8));
	sensor_write(sd, 0x3502, (info->exp & 0xff));
	sensor_write(sd, 0x3501, (info->exp >> 8) & 0xff);
	sensor_write(sd, 0x3500, (info->exp >> 16) & 0xff);
	sensor_write(sd, 0x3509, (info->gain & 0xff));
	sensor_write(sd, 0x3508, ((info->gain >> 8) & 0x7));
	sensor_write(sd, 0x3208, 0x10);
	sensor_write(sd, 0x3208, 0xa0);
	sensor_write(sd, 0x0100, 0x01);
	sensor_print("s_fmt = %x, width = %d, height = %d\n",
		      sensor_fmt->mbus_code, wsize->width, wsize->height);
	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);

	sensor_print("%s on = %d, %d*%d fps: %d code: %x\n", __func__, enable,
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
	//.s_parm = sensor_s_parm,
	//.g_parm = sensor_g_parm,
	.s_stream = sensor_s_stream,
	//.g_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
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

static int sensor_dev_id;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
static int sensor_probe(struct i2c_client *client)
#else
static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
#endif
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	int i;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
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
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->af_first_flag = 1;
	info->exp = 16 * 2466;
	info->gain = 16 * 3;

	return 0;
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
static int sensor_remove(struct i2c_client *client)
#else
static void sensor_remove(struct i2c_client *client)
#endif
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
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
	return 0;
#endif
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
