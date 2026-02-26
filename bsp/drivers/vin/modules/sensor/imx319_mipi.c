/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for imx319 Raw cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Cao FuMing <caofuming@allwinnertech.com>
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

MODULE_AUTHOR("cfm");
MODULE_DESCRIPTION("A low-level driver for IMX319 sensors");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR 0x0319

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The IMX319 i2c address
 */
#define I2C_ADDR 		0x20

#define SENSOR_NUM     0x2
#define SENSOR_NAME    "imx319_mipi"
#define SENSOR_NAME_2  "imx319_mipi_2"

#define DOL_RHS1	64
#define DOL_RATIO	16
#define V_SIZE		(0x44E)
#define OUT_SIZE	(V_SIZE + DOL_RHS1)

/*
 * The default register settings
 */
static struct regval_list sensor_default_regs[] = {

};

/*
************* Sensor Information *****************
@Sensor	  			: imx319
@Date		  		: 2017-04-06
@Image size	  		: 3264x2448
@MCLK/PCLK	  		: 24MHz /288Mhz
@MIPI speed(Mbps)	: 720Mbps x 4Lane
@Frame Length	  	: 2492
@Line Length 	 	: 3800
@line Time       	: 13194
@Max Fps 	  		: 30.00fps
@Pixel order 	  	: Green 1st (=GB)
@X/Y-flip        	: X-flip
@BLC offset	   	 	: 64code
@Firmware Ver.   	: v1.0
**************************************************
*/
static struct regval_list sensor_3264_2448_30_regs[] = {
	{0x0100, 0x00},
	{0x0101, 0x00},
	{0x0136, 0x18},
	{0x0137, 0x00},
	{0x3c7e, 0x02},
	{0x3c7f, 0x00},

	{0x0112, 0x0a},
	{0x0113, 0x0a},
	{0x0114, 0x03},

	{0x0342, 0x0f},//linelength 3968
	{0x0343, 0x80},

	{0x0340, 0x0a},//framelength 2688
	{0x0341, 0x80},

	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x00},
	{0x0347, 0x08},
	{0x0348, 0x0c},
	{0x0349, 0xcf},
	{0x034a, 0x09},
	{0x034b, 0x97},

	{0x0220, 0x00},
	{0x0221, 0x11},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x0900, 0x00},
	{0x0901, 0x11},
	{0x0902, 0x0a},
	{0x3140, 0x02},
	{0x3141, 0x00},
	{0x3f0d, 0x0a},
	{0x3f14, 0x01},
	{0x3f3c, 0x01},
	{0x3f4d, 0x01},
	{0x3f4c, 0x01},
	{0x4254, 0x7f},

	{0x0401, 0x00},
	{0x0404, 0x00},
	{0x0405, 0x10},
	{0x0408, 0x00},
	{0x0409, 0x08},
	{0x040a, 0x00},
	{0x040b, 0x00},
	{0x040c, 0x0c},
	{0x040d, 0xc0},
	{0x040e, 0x09},
	{0x040f, 0x90},
	{0x034c, 0x0c},
	{0x034d, 0xc0},
	{0x034e, 0x09},
	{0x034f, 0x90},
	{0x3261, 0x00},
	{0x3264, 0x00},
	{0x3265, 0x10},

	{0x0301, 0x06},
	{0x0303, 0x04},
	{0x0305, 0x04},
	{0x0306, 0x01},
	{0x0307, 0x40},
	{0x0309, 0x0a},
	{0x030b, 0x02},
	{0x030d, 0x03},
	{0x030e, 0x00},
	{0x030f, 0xc8},
	{0x0310, 0x01},
	{0x0820, 0x0c},
	{0x0821, 0x80},
	{0x0822, 0x00},
	{0x0823, 0x00},

	{0x3e20, 0x01},
	{0x3e37, 0x01},
	{0x3e3b, 0x01},

	{0x3603, 0x00},

	{0x38a3, 0x01},
	{0x38a8, 0x00},
	{0x38a9, 0x00},
	{0x38aa, 0x00},
	{0x38ab, 0x00},

	{0x7485, 0x08},
	{0x7487, 0x0c},
	{0x7488, 0x0c},
	{0x7489, 0xc7},
	{0x748a, 0x09},
	{0x748b, 0x8b},

	{0x3234, 0x00},
	{0x3fc1, 0x00},

	{0x3235, 0x00},
	{0x3802, 0x00},
	{0x3143, 0x04},
	{0x360a, 0x00},
	{0x0b00, 0x00},

	{0x3237, 0x00},
	{0x3900, 0x00},
	{0x3901, 0x00},
	{0x3902, 0x00},
	{0x3904, 0x00},
	{0x3905, 0x00},
	{0x3906, 0x00},
	{0x3907, 0x00},
	{0x3908, 0x00},
	{0x3909, 0x00},
	{0x3912, 0x00},
	{0x3930, 0x00},
	{0x3931, 0x00},
	{0x3933, 0x00},
	{0x3934, 0x00},
	{0x3935, 0x00},
	{0x3936, 0x00},
	{0x3937, 0x00},
	{0x3614, 0x00},
	{0x3616, 0x0d},
	{0x3617, 0x56},
	{0x0106, 0x00},
	{0x0b05, 0x01},
	{0x0b06, 0x01},
	{0x3230, 0x00},
	{0x3602, 0x01},
	{0x3607, 0x01},
	{0x3c00, 0x00},
	{0x3c01, 0x48},
	{0x3c02, 0xc8},
	{0x3c03, 0xaa},
	{0x3c04, 0x91},
	{0x3c05, 0x54},
	{0x3c06, 0x26},
	{0x3c07, 0x20},
	{0x3c08, 0x51},
	{0x3d80, 0x00},
	{0x3f50, 0x00},
	{0x3f56, 0x00},
	{0x3f57, 0x30},

	{0x3f78, 0x01},
	{0x3f79, 0x18},
	{0x3f7c, 0x00},
	{0x3f7d, 0x00},
	{0x3fba, 0x00},
	{0x3fbb, 0x00},
	{0xa081, 0x00},
	{0xe014, 0x00},

	{0x0202, 0x0a},
	{0x0203, 0x6e},
	{0x0224, 0x01},
	{0x0225, 0xf4},

	{0x0204, 0x03},
	{0x0205, 0xc0},
	{0xe216, 0x00},
	{0x0217, 0x00},
	{0x020e, 0x01},
	{0x020f, 0x01},
	{0x0210, 0x10},
	{0x0211, 0x00},
	{0x0212, 0x01},
	{0xe213, 0x00},
	{0x0214, 0x01},
	{0x0215, 0x00},
	{0x0218, 0x01},
	{0x0219, 0x00},

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

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	data_type explow, exphigh;
	struct sensor_info *info = to_state(sd);

	exphigh = (unsigned char)((exp_val >> 12) & 0xff);
	explow  = (unsigned char)((exp_val >> 4)  & 0xff);

	sensor_write(sd, 0x0203, explow);/* coarse integration time */
	sensor_write(sd, 0x0202, exphigh);

	sensor_dbg("sensor_s_exp info->exp %d\n", exp_val);
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
		gainana = 1024 - 16384/gain_val;
		gaindigi = 256;
	} else {
		gainana = 960;
		gaindigi = gain_val;
	}

	gainlow = (unsigned char)(gainana&0xff);
	gainhigh = (unsigned char)((gainana>>8)&0xff);

	sensor_write(sd, 0x0205, gainlow);
	sensor_write(sd, 0x0204, gainhigh);

	sensor_write(sd, 0x020f, (unsigned char)(gaindigi & 0xff));
	sensor_write(sd, 0x020e, (unsigned char)(gaindigi >> 8));

	sensor_dbg("sensor_set_gain = %d, Done!\n", gain_val);
	info->gain = gain_val;

	return 0;
}

static int imx319_sensor_vts;
static int frame_length = 2688;
static int expo_val_save = 1540 * 16;
static int gain_val_save = 480;
static int sensor_s_exp_gain(struct v4l2_subdev *sd,
				struct sensor_exp_gain *exp_gain)
{
	struct sensor_info *info = to_state(sd);
	int exp_val, gain_val, shutter;

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	expo_val_save = exp_gain->exp_val;
	gain_val_save = exp_gain->gain_val;

	shutter = exp_val>>4;
	if (shutter > imx319_sensor_vts - 18)
		frame_length = shutter;
	else
		frame_length = imx319_sensor_vts;

	sensor_write(sd, 0x0104, 0x01);
	sensor_write(sd, 0x0341, frame_length & 0xff);
	sensor_write(sd, 0x0340, frame_length >> 8);
	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);
	sensor_write(sd, 0x0104, 0x00);

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

	imx319_sensor_vts = wsize->pclk/fps->fps/wsize->hts;
	sensor_write(sd, 0x0104, 0x01);
	sensor_write(sd, 0x0341, imx319_sensor_vts & 0xff);
	sensor_write(sd, 0x0340, imx319_sensor_vts >> 8);
	sensor_write(sd, 0x0104, 0x00);

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
		ret = sensor_write(sd, 0x0100, rdval | 0x01);
	else
		ret = sensor_write(sd, 0x0100, rdval & 0xfe);
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
		vin_set_pmu_channel(sd, CAMERAVDD, ON);
		usleep_range(3000, 3200);
		vin_set_pmu_channel(sd, IOVDD, ON);
		usleep_range(2000, 2200);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(100, 120);
		vin_set_mclk_freq(sd, MCLK);
		usleep_range(3000, 3200);
		vin_set_mclk(sd, ON);
		usleep_range(1000, 1200);
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
		vin_set_pmu_channel(sd, CAMERAVDD, OFF);
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
#if !defined CONFIG_VIN_INIT_MELIS
	data_type rdval = 0;
	int ret;
	ret = sensor_read(sd, 0x0016, &rdval);
	sensor_dbg("0x0016 0x%x\n", rdval);
	if (rdval != (V4L2_IDENT_SENSOR >> 8) && (ret < 0 || rdval != 0x0)) {
		sensor_err(" read 0x0016 return 0x%2x\n", rdval);
		return -ENODEV;
	}

	ret = sensor_read(sd, 0x0017, &rdval);
	sensor_dbg("0x0017 0x%x\n", rdval);
	if (rdval != (V4L2_IDENT_SENSOR & 0xff) && (ret < 0 || rdval != 0x0)) {
		sensor_err(" read 0x0016 return 0x%2x\n", rdval);
		return -ENODEV;
	}
#endif
	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	__maybe_unused struct sensor_exp_gain exp_gain;

	sensor_dbg("sensor_init\n");

	/*Make sure it is a target sensor */
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
	case VIDIOC_VIN_SENSOR_SET_FPS:
		ret = sensor_s_fps(sd, (struct sensor_fps *)arg);
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
		.mbus_code = MEDIA_BUS_FMT_SRGGB10_1X10,
	    //.mbus_code = MEDIA_BUS_FMT_SRGGB12_1X12,
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
	 .hts = 3968,
	 .vts = 2688,
	 .pclk = 320 * 1000 * 1000,
	 .mipi_bps = 720 * 1000 * 1000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .intg_min = 4 << 4,
	 .intg_max = (2492 - 18) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 256 << 4,
	 .regs = sensor_3264_2448_30_regs,
	 .regs_size = ARRAY_SIZE(sensor_3264_2448_30_regs),
	 .set_size = NULL,
	 .top_clk = 310*1000*1000,
	 .isp_clk = 297*1000*1000,
	 },
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
	int ret;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;
	__maybe_unused struct sensor_exp_gain exp_gain;

	ret = sensor_write_array(sd, sensor_default_regs,
				 ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	sensor_dbg("sensor_reg_init\n");

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

#if IS_ENABLED(CONFIG_VIN_INIT_MELIS)
	if (info->preview_first_flag) {
		info->preview_first_flag = 0;
	} else {
		if (wsize->regs)
			sensor_write_array(sd, wsize->regs, wsize->regs_size);
		if (wsize->wdr_mode == ISP_DOL_WDR_MODE) {
			exp_gain.exp_val = 30720;
			exp_gain.gain_val = 16;
		} else {
			exp_gain.exp_val = 15408;
			exp_gain.gain_val = 32;
		}
		sensor_s_exp_gain(sd, &exp_gain);
	}
#else
	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);

#endif

	if (wsize->set_size)
		wsize->set_size(sd);

	info->width = wsize->width;
	info->height = wsize->height;
	imx319_sensor_vts = wsize->vts;

	sensor_write(sd, 0x0341, frame_length & 0xff);
	sensor_write(sd, 0x0340, frame_length >> 8);
	sensor_s_exp(sd, expo_val_save);
	sensor_s_gain(sd, gain_val_save);

	sensor_write(sd, 0x3000, 0); /*sensor mipi stream on*/

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

static int imx319_g_pixelaspect(struct v4l2_subdev *sd, struct v4l2_fract *aspect)
{

	aspect->numerator = 4;
	aspect->denominator = 3;

	return 0;
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
	.g_pixelaspect = imx319_g_pixelaspect,

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
#if IS_ENABLED(CONFIG_SAME_I2C)
		info->sensor_i2c_addr = I2C_ADDR >> 1;
#endif
	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET2 | MIPI_NORMAL_MODE;
//	info->time_hs = 0x10;
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->preview_first_flag = 1;
	info->af_first_flag = 1;
	info->exp = 0;
	info->gain = 0;
	info->stable_frame_cnt = 0;
	info->sdram_dfs_flag = SENSOR_ALREADY_DEBUG;

	return 0;
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
static void sensor_remove(struct i2c_client *client)
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
}
#else
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
#endif
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
