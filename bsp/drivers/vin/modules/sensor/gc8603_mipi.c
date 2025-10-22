/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * A V4L2 driver for GC5024 Raw cameras.
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
MODULE_AUTHOR("sochip");
MODULE_DESCRIPTION("A low-level driver for GC8603 sensors");
MODULE_LICENSE("GPL");
#define MCLK              (24*1000*1000)
#define VREF_POL          V4L2_MBUS_VSYNC_ACTIVE_LOW
#define HREF_POL          V4L2_MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_RISING
#define V4L2_IDENT_SENSOR 0x8003

#define I2C_ADDR 0x6e
#define SENSOR_NAME "gc8603_mipi"

#define ID_REG_HIGH		0xf0
#define ID_REG_LOW		0xf1
#define ID_VAL_HIGH		((V4L2_IDENT_SENSOR) >> 8)
#define ID_VAL_LOW		((V4L2_IDENT_SENSOR) & 0xff)

/* define the voltage level of control signal */
#define CSI_STBY_ON	1
#define CSI_STBY_OFF	0
#define CSI_RST_ON	0
#define CSI_RST_OFF	1
#define CSI_PWR_ON	1
#define CSI_PWR_OFF	0
#define CSI_AF_PWR_ON   1
#define CSI_AF_PWR_OFF  0
#define REG_TERM 0xfffe
#define VAL_TERM 0xfe

#define DGAIN_R  0x0100
#define DGAIN_G  0x0100
#define DGAIN_B  0x0100

#define ANALOG_GAIN_1	64  /* 1.00x */
#define ANALOG_GAIN_2	88  /* 1.38x */
#define ANALOG_GAIN_3	124  /* 1.94x */
#define ANALOG_GAIN_4	168  /* 2.63x */
#define ANALOG_GAIN_5	225  /* 3.52x */
#define ANALOG_GAIN_6	311  /* 4.86x */
#define ANALOG_GAIN_7	435  /* 6.80x */



static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	*value = info->exp;
	sensor_dbg("sensor_get_exposure = %d\n", info->exp);
	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{

	unsigned char explow, exphigh;
	struct sensor_info *info = to_state(sd);

	if (exp_val > 0xffffff)
		exp_val = 0xfffff0;
	if (exp_val < 16)
		exp_val = 16;

	exp_val = (exp_val+8)>>4;/* rounding to 1 */

	exphigh = (unsigned char) ((0xff00&exp_val)>>8);
	explow	= (unsigned char) ((0x00ff&exp_val));

	sensor_write(sd, 0x04, explow);/* coarse integration time */
	sensor_write(sd, 0x03, exphigh);

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

static int sensor_s_gain(struct v4l2_subdev *sd, unsigned int gain_val)
{
	unsigned char tmp;
	struct sensor_info *info = to_state(sd);
	/*	printk("gc2235 sensor gain value is %d\n",gain_val); */
	gain_val = gain_val * 4;

	sensor_write(sd, 0xfe, 0x00);
	sensor_write(sd, 0xb1, 0x01);
	sensor_write(sd, 0xb2, 0x00);

	if (gain_val < 0x40)
		gain_val = 0x40;
	else if ((gain_val >= ANALOG_GAIN_1) && (gain_val < ANALOG_GAIN_2)) {
		/* analog gain */
		sensor_write(sd, 0xb6, 0x00);
		tmp = gain_val;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2)&0xfc);
		/* printk("GC5004 analogic gain 1x , */
		/* GC5004 add pregain = %d\n",tmp); */
	} else if ((gain_val >= ANALOG_GAIN_2) && (gain_val < ANALOG_GAIN_3)) {
		/* analog gain */
		sensor_write(sd, 0xb6,	0x01);//
		tmp = 64*gain_val/ANALOG_GAIN_2;
		sensor_write(sd, 0xb1, tmp>>6);
		sensor_write(sd, 0xb2, (tmp<<2)&0xfc);
		/* printk("GC5004 analogic gain 1.45x ,
		GC5004 add pregain = %d\n",tmp); */
	} else if ((gain_val >= ANALOG_GAIN_3) && (gain_val < ANALOG_GAIN_4)) {
		/* analog gain */
		sensor_write(sd, 0xb6,	0x02);//
		tmp = 64*gain_val/ANALOG_GAIN_3;
		sensor_write(sd, 0xb1, tmp>>6);
		sensor_write(sd, 0xb2, (tmp<<2)&0xfc);
		/* printk("GC5004 analogic gain 2.02x ,
		GC5004 add pregain = %d\n",tmp); */
	} else if ((gain_val >= ANALOG_GAIN_4) && (gain_val < ANALOG_GAIN_5)) {
		/* analog gain */
		sensor_write(sd, 0xb6,	0x03);//
		tmp = 64*gain_val/ANALOG_GAIN_4;
		sensor_write(sd, 0xb1, tmp>>6);
		sensor_write(sd, 0xb2, (tmp<<2)&0xfc);
		/* printk("GC5004 analogic gain 2.86x ,
		GC5004 add pregain = %d\n",tmp); */
	}
	/* else if((ANALOG_GAIN_5<= gain_val)&&
	(gain_val)&&(gain_val < ANALOG_GAIN_6)) */
	else if ((gain_val >= ANALOG_GAIN_5) && (gain_val < ANALOG_GAIN_6)) {
		/* analog gain */
		sensor_write(sd, 0xb6,	0x04);//
		tmp = 64*gain_val/ANALOG_GAIN_5;
		sensor_write(sd, 0xb1, tmp>>6);
		sensor_write(sd, 0xb2, (tmp<<2)&0xfc);
		/* printk("GC5004 analogic gain 3.95x ,
		GC5004 add pregain = %d\n",tmp); */
	}
		/* else if((ANALOG_GAIN_5<= gain_val)&&
		(gain_val)&&(gain_val < ANALOG_GAIN_6)) */
	else if ((gain_val >= ANALOG_GAIN_6) && (gain_val < ANALOG_GAIN_7)) {
		/* analog gain */
		sensor_write(sd, 0xb6,	0x05);//
		tmp = 64*gain_val/ANALOG_GAIN_6;
		sensor_write(sd, 0xb1, tmp>>6);
		sensor_write(sd, 0xb2, (tmp<<2)&0xfc);
		/* printk("GC5004 analogic gain 3.95x ,
		GC5004 add pregain = %d\n",tmp); */
	}

		/* else if((ANALOG_GAIN_5<= gain_val)&&
		(gain_val)&&(gain_val < ANALOG_GAIN_6)) */
	else if (gain_val >= ANALOG_GAIN_7) {
		/* analog gain */
		sensor_write(sd, 0xb6,	0x06);//
		tmp = 64*gain_val/ANALOG_GAIN_7;
		sensor_write(sd, 0xb1, tmp>>6);
		sensor_write(sd, 0xb2, (tmp<<2)&0xfc);
		/* printk("GC5004 analogic gain 3.95x ,
		GC5004 add pregain = %d\n",tmp); */
	}

	/* else if((ANALOG_GAIN_5 <= gain_val) && */
	/* (gain_val) && (gain_val < ANALOG_GAIN_6)) */

	/* printk("gc2235 sensor read out gain is %d\n",gain); */
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

	if (gain_val < 1*16)
		gain_val = 16;
	if (gain_val > 64*16-1)
		gain_val = 64*16-1;

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
		sensor_print("CSI_SUBDEV_STBY_ON!\n");
		usleep_range(10000, 12000);
		cci_lock(sd);
		vin_gpio_write(sd, PWDN, CSI_STBY_ON);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		sensor_print("CSI_SUBDEV_STBY_OFF!\n");
		cci_lock(sd);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_STBY_OFF);
		usleep_range(10000, 12000);
		cci_unlock(sd);
		usleep_range(10000, 12000);
		break;
	case PWR_ON:
		sensor_print("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_write(sd, PWDN, CSI_STBY_ON);
		vin_gpio_write(sd, RESET, CSI_RST_ON);
		usleep_range(1000, 1200);
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_set_pmu_channel(sd, AFVDD, ON);
		usleep_range(10000, 12000);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(20000, 21000);
		vin_gpio_write(sd, PWDN, CSI_STBY_OFF);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, RESET, CSI_RST_OFF);
		usleep_range(30000, 31000);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_print("PWR_OFF!\n");
		cci_lock(sd);

		vin_gpio_write(sd, PWDN, CSI_STBY_ON);
		usleep_range(1000, 2000);
		vin_gpio_write(sd, RESET, CSI_RST_ON);
		vin_set_mclk(sd, OFF);
		vin_set_pmu_channel(sd, AFVDD, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, RESET, CSI_RST_ON);
		vin_gpio_write(sd, PWDN, CSI_STBY_OFF);
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
		vin_gpio_write(sd, RESET, CSI_RST_OFF);
		usleep_range(100, 120);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_RST_ON);
		usleep_range(100, 120);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	data_type val;

	sensor_read(sd, ID_REG_HIGH, &val);
	if (val != ID_VAL_HIGH)
		return -ENODEV;
	sensor_print("read Sensor_ID [0xF0] = 0x%x\n", val);
	sensor_read(sd, ID_REG_LOW, &val);
	if (val != ID_VAL_LOW)
		return -ENODEV;
	sensor_print("read Sensor_ID [0xF1] = 0x%x\n", val);
	sensor_print("chip found is an target chip ***********\n");
	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	sensor_print("sensor_init\n");

	/* Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}
	info->focus_status = 0;
	info->low_speed = 0;
	info->width = QUXGA_WIDTH; /* 3264 */
	info->height = QUXGA_HEIGHT; /* 2448 */
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;

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
	case VIDIOC_VIN_SENSOR_CFG_REQ:
		sensor_cfg_req(sd, (struct sensor_config *)arg);
		break;
	case VIDIOC_VIN_ACT_INIT:
		ret = actuator_init(sd, (struct actuator_para *)arg);
		break;
	case VIDIOC_VIN_ACT_SET_CODE:
		ret = actuator_set_code(sd, (struct actuator_ctrl *)arg);
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

/*
 * Store information about the video data format.
 */
static struct regval_list sensor_fmt_raw[] = {

};

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

static struct regval_list sensor_default_regs[] = {

};

static struct regval_list sensor_8M30_regs[] = {
//MCLK=24Mhz,MIPI_clcok=720Mhz
//Actual_Window_size=3264*2448
//VD=2496,row_time=13.3us;
/***************SYS***********/
	{0xfe, 0x80},
	{0xfe, 0x80},
	{0xfe, 0x80},
	/* System Register */
	{0xf7, 0x95},
	{0xf8, 0x0e},
	{0xf9, 0x0a},
	{0xfa, 0x09},
	{0xfc, 0xea},
	{0xfe, 0x03},
	{0x01, 0x07},
	{0x18, 0x07},
	{0xfc, 0xee},
	{0xfe, 0x00},
	{0x88, 0x03},
	{0xe6, 0xc8},
	/**********analog***********/
	{0xfe, 0x00},
	{0x03, 0x08},	/* Exposure_t1 0x08ca = 2250 */
	{0x04, 0xca},
	{0x05, 0x01},	/* H Blanking 0x01c2 = 450 */
	{0x06, 0xc2},
	{0x09, 0x00},	/* Row start 0x001c= 28 */
	{0x0a, 0x1c},
	{0x0b, 0x00},	/* Col start 0x0010 = 16 */
	{0x0c, 0x10},
	{0x0d, 0x09},	/* Window height 0x09a0 = 2464 */
	{0x0e, 0xa0},
	{0x0f, 0x0c},	/* Window width 0x0cd0 = 3280 */
	{0x10, 0xd0},
	{0x11, 0x31},
	{0x17, 0xd5},
	{0x18, 0x02},
	{0x19, 0x0c},
	{0x1a, 0x1b},
	{0x21, 0x0b},
	{0x23, 0x80},
	{0x24, 0xc1},
	{0x26, 0x64},
	{0x28, 0x3f},
	{0x29, 0xc4},
	{0x2d, 0x44},
	{0x30, 0xf9},
	{0xcd, 0xca},
	{0xce, 0x89},
	{0xcf, 0x10},
	{0xd0, 0xc2},
	{0xd1, 0xdc},
	{0xd2, 0xcb},
	{0xd3, 0x33},
	{0xd8, 0x1b},
	{0xdc, 0xb3},
	{0xe1, 0x1b},
	{0xe2, 0x30},
	{0xe3, 0x01},
	{0xe4, 0x78},
	{0xe8, 0x02},
	{0xe9, 0x01},
	{0xea, 0x03},
	{0xeb, 0x03},
	{0xec, 0x02},
	{0xed, 0x01},
	{0xee, 0x03},
	{0xef, 0x03},
	/**********ISP***********/
	{0x80, 0x50},
	{0x89, 0x03},
	{0x8a, 0xb3},
	{0x8d, 0x03},
	/****window 3264x2448****/
	{0x90, 0x01},
	{0x92, 0x02},
	{0x94, 0x03},
	{0x95, 0x09},	/* Out window height 0x0990 = 2448 */
	{0x96, 0x90},
	{0x97, 0x0c},	/* Out window width 0x0cc0 = 3264 */
	{0x98, 0xc0},
	/**********gain***********/
	{0x99, 0x68},
	{0x9a, 0x70},
	{0x9b, 0x78},
	{0x9c, 0x79},
	{0x9d, 0x7a},
	{0x9e, 0x7b},
	{0x9f, 0x7c},
	{0xb0, 0x48},
	{0xb1, 0x01},
	{0xb2, 0x00},
	{0xb6, 0x00},
	/**********blk***********/
	{0x40, 0x22},
	{0x3e, 0x00},
	{0x3f, 0x0f},
	{0x4e, 0xf0},
	{0x4f, 0x00},
	{0x50, 0x00},
	{0x51, 0x00},
	{0x52, 0x00},
	{0x53, 0x00},
	{0x54, 0x00},
	{0x55, 0x00},
	{0x56, 0x00},
	{0x57, 0x00},
	{0x60, 0x00},
	{0x61, 0x80},
	/********** dark offset ***********/
	{0x35, 0x30},
	{0x36, 0x00},
	/********** darksun ***********/
	{0x37, 0x00},	/* 0xf6 0x00 */
	{0x39, 0x0f},
	{0x3b, 0xd0},
	{0x3d, 0x08},
	/********** degrid ***********/
	{0xfe, 0x01},
	{0xc0, 0x00},
	{0xa0, 0x60},
	/********** MIPI ***********/
	{0xfe, 0x03},
	{0x02, 0x34},
	{0x03, 0x13},
	{0x04, 0xf0},
	{0x06, 0x80},
	{0x10, 0xd0},
	{0x11, 0x2b},
	{0x12, 0xf0},
	/* LWC_set,0x0ff0 = 3264x5/4 RAW10, 0x0ff0 = 4080,4080*4/5 = 3264 */
	{0x13, 0x0f},
	{0x15, 0x02},
	{0x16, 0x09},
	{0x19, 0x30},
	{0x1a, 0x03},
	{0xfe, 0x00},
};

static struct sensor_win_size sensor_win_sizes[] = {
	{	/* full size 30fps capture */
		.width		= 3264,
		.height		= 2448,
		.hoffset    = 0,
		.voffset	= 0,
		.hts		= 1800,		/* 1800,	3600 */
		.vts		= 2496,		/* 2496,	2496 */
		.pclk		= 135*1000*1000,
		.mipi_bps   = 720*1000*1000,
		.fps_fixed  = 30,		/* 15fps 30fps */
		.bin_factor = 1,
		.intg_min   = 16,
		.intg_max   = (2496-4)<<4,
		.gain_min   = 16,
		.gain_max   = (32 << 4),
		/* sensor_8M15_regs */
		.regs	= sensor_8M30_regs,
		/* sensor_8M15_regs */
		.regs_size  = ARRAY_SIZE(sensor_8M30_regs),
		.set_size	= NULL,
	}
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

	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;
	struct sensor_exp_gain exp_gain;

	sensor_print("sensor_reg_init\n");

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);

	if (wsize->set_size)
		wsize->set_size(sd);

	info->width = wsize->width;
	info->height = wsize->height;
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
static struct cci_driver cci_drv = {
		.name = SENSOR_NAME,
		.addr_width = CCI_BITS_8,
		.data_width = CCI_BITS_8,
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
	int ret = 0;

	ret = cci_dev_init_helper(&sensor_driver);

	return ret;
}

static __exit void exit_sensor(void)
{
	cci_dev_exit_helper(&sensor_driver);
}

VIN_INIT_DRIVERS(init_sensor);
module_exit(exit_sensor);
