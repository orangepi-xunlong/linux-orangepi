/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * A V4L2 driver for TP9930 YUV cameras.
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

//	Mode setting
/*
 *	mode_set_720p_2ch
 *	mode_set_720p_4ch
 *	mode_set_1080p_2ch
 *	mode_set_1080p_4ch
 */

#define mode_set_720p_4ch

MODULE_AUTHOR("zw");
MODULE_DESCRIPTION("A low-level driver for TP9930 sensors");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

#define MCLK				(27 * 1000 * 1000)
#define CLK_POL				V4L2_MBUS_PCLK_SAMPLE_FALLING
#define DOUBLE_CLK_POL		(V4L2_MBUS_PCLK_SAMPLE_FALLING | V4L2_MBUS_PCLK_SAMPLE_RISING)
#define V4L2_IDENT_SENSOR	0x3228

#define DBG_INFO(format, args...) (printk("[TP9930 INFO] LINE:%04d-->%s:"format, __LINE__, __func__, ##args))
#define DBG_ERR(format, args...)  (printk("[TP9930 ERR] LINE:%04d-->%s:"format, __LINE__, __func__, ##args))


/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 25

/*
 * The TP9920 sits on i2c with ID 0x88 or 0x8a
 * SAD-low:0x88 SAD-high:0x8a
 */
#define I2C_ADDR 0x88
#define SENSOR_NAME "tp9930"
__maybe_unused static struct regval_list reg_1080p25_2ch[] = {
	{0x40, 0x00},
	{0x4d, 0x00},
	{0x4e, 0x00},
	{0x44, 0x47},
	{0xf4, 0x80},
	{0xffff, 0xa0},
	{0x44, 0x07},
	{0x3b, 0x20},
	{0x3d, 0xe0},
	{0x3d, 0x60},
	{0x3b, 0x25},
	{0x40, 0x40},
	{0x7a, 0x20},
	{0x3c, 0x20},
	{0x3c, 0x00},
	{0x7a, 0x25},
	{0x40, 0x00},
	{0x44, 0x57},
	{0x43, 0x12},
	{0x45, 0x09},
	{0xf4, 0x80},
	{0x44, 0x17},
//video setting
	{0x40, 0x04},
	{0x02, 0x44},/* set 16bit 1120 output */
	{0x07, 0x80},
	{0x0b, 0xc0},
	{0x0c, 0x03},
	{0x0d, 0x73},
	{0x10, 0x00},
	{0x11, 0x40},
	{0x12, 0x40},
	{0x13, 0x00},
	{0x14, 0x00},
	{0x15, 0x01},
	{0x16, 0xf0},
	{0x17, 0x80},
	{0x18, 0x29},
	{0x19, 0x38},
	{0x1a, 0x47},
	{0x1c, 0x0a},
	{0x1d, 0x50},
	{0x20, 0x3c},
	{0x21, 0x46},
	{0x22, 0x36},
	{0x23, 0x3c},
	{0x24, 0x04},
	{0x25, 0xfe},
	{0x26, 0x0d},
	{0x27, 0x2d},
	{0x28, 0x00},
	{0x29, 0x48},
	{0x2a, 0x30},
	{0x2b, 0x60},
	{0x2c, 0x3a},
	{0x2d, 0x54},
	{0x2e, 0x40},
	{0x30, 0xa5},
	{0x31, 0x86},
	{0x32, 0xfb},
	{0x33, 0x60},
	{0x35, 0x05},
	{0x36, 0xca},
	{0x38, 0x00},
	{0x39, 0x1c},
	{0x3a, 0x32},
	{0x3b, 0x26},
//output format
	{0x4f, 0x03},
	{0x50, 0x00},
	{0x52, 0x00},
	{0xf1, 0x04},
	{0xf2, 0x77},
	{0xf3, 0x77},
	{0xf4, 0x0c},/* close the 3-4 channel power down */
	{0xf5, 0x00},
	{0xf6, 0x01},
	{0xf8, 0x45},
	{0xfa, 0x88},
	{0xfb, 0x88},
//channel ID
	{0x40, 0x00},
	{0x34, 0x10},
	{0x40, 0x01},
	{0x34, 0x11},
	{0x40, 0x02},
	{0x34, 0x10},
	{0x3d, 0xff}, /* close the 3 channel */
	{0x40, 0x03},
	{0x34, 0x11},
	{0x3d, 0xff}, /* close the 4 channel */
/* output enable */
	{0x4d, 0x07},
	{0x4e, 0x55} /* 0x05 */
};

__maybe_unused static struct regval_list reg_1080p25_4ch[] = {
	{0x40, 0x00},
	{0x4d, 0x00},
	{0x4e, 0x00},
	{0x44, 0x07},
	{0xf4, 0xa0},
	{0x40, 0x04},
	{0x3b, 0x20},
	{0x3d, 0xe0},
	{0x3d, 0x60},
	{0x3b, 0x25},
	{0x40, 0x40},
	{0x7a, 0x20},
	{0x3c, 0x20},
	{0x3c, 0x00},
	{0x7a, 0x25},
	{0x40, 0x00},
	{0x44, 0x07},
	{0x43, 0x12},
	{0x45, 0x09},
	{0xf4, 0xa0},
//video setting
	{0x40, 0x04},
	{0x02, 0x44},
	{0x05, 0x00},
	{0x06, 0x32},
	{0x07, 0x80},
	{0x08, 0x00},
	{0x09, 0x24},
	{0x0a, 0x48},
	{0x0b, 0xc0},
	{0x0c, 0x03},
	{0x0d, 0x73},
	{0x10, 0x00},
	{0x11, 0x40},
	{0x12, 0x40},
	{0x13, 0x00},
	{0x14, 0x00},
	{0x15, 0x01},
	{0x16, 0xf0},
	{0x17, 0x80},
	{0x18, 0x29},
	{0x19, 0x38},
	{0x1a, 0x47},
	{0x1c, 0x0a},
	{0x1d, 0x50},
	{0x20, 0x3c},
	{0x21, 0x46},
	{0x22, 0x36},
	{0x23, 0x3c},
	{0x24, 0x04},
	{0x25, 0xfe},
	{0x26, 0x0d},
	{0x27, 0x2d},
	{0x28, 0x00},
	{0x29, 0x48},
	{0x2a, 0x30},
/* {0x2a, 0x3c}, */
	{0x2b, 0x60},
	{0x2c, 0x3a},
	{0x2d, 0x54},
	{0x2e, 0x40},
	{0x30, 0xa5},
	{0x31, 0x86},
	{0x32, 0xfb},
	{0x33, 0x60},
	{0x35, 0x05},
	{0x36, 0xca},
	{0x38, 0x00},
	{0x39, 0x1c},
	{0x3a, 0x32},
	{0x3b, 0x26},
//channel ID
	{0x40, 0x00},
	{0x34, 0x10},
	{0x40, 0x01},
	{0x34, 0x11},
	{0x40, 0x02},
	{0x34, 0x12},
	{0x40, 0x03},
	{0x34, 0x13},
//output format
	{0x4f, 0x03},
	{0x50, 0xb2},
	{0x52, 0xf6},
	{0xf1, 0x04},
	{0xf2, 0x11},
	{0xf3, 0x11},
	{0xf5, 0x00},
	{0xf6, 0x10},
	{0xf8, 0x54},
	{0xfa, 0x88},
	{0xfb, 0x88},
//output enable
	{0x4d, 0x07},
	{0x4e, 0x55},

};
__maybe_unused static struct regval_list reg_720p25_2ch[] = {
	{0x40, 0x04},
	{0x3b, 0x20},
	{0x3d, 0xe0},
	{0x3d, 0x60},
	{0x3b, 0x25},
	{0x40, 0x40},
	{0x7a, 0x20},
	{0x3c, 0x20},
	{0x3c, 0x00},
	{0x7a, 0x25},
	{0x40, 0x00},
	{0x44, 0x17},
	{0x43, 0x12},
	{0x45, 0x09},
//video setting
	{0x40, 0x04},
	{0x02, 0xce},
	{0x05, 0x00},
	{0x06, 0x32},
	{0x07, 0xc0},
	{0x08, 0x00},
	{0x09, 0x24},
	{0x0a, 0x48},
	{0x0b, 0xc0},
	{0x0c, 0x13},
	{0x0d, 0x71},
	{0x0e, 0x00},
	{0x0f, 0x00},
	{0x10, 0x00},
	{0x11, 0x40},
	{0x12, 0x40},
	{0x13, 0x00},
	{0x14, 0x00},
	{0x15, 0x13},
	{0x16, 0x16},
	{0x17, 0x00},
	{0x18, 0x19},
	{0x19, 0xd0},
	{0x1a, 0x25},
	{0x1b, 0x00},
	{0x1c, 0x07},
	{0x1d, 0xbc},
	{0x1e, 0x60},
	{0x1f, 0x06},
	{0x20, 0x40},
	{0x21, 0x46},
	{0x22, 0x36},
	{0x23, 0x3c},
	{0x24, 0x04},
	{0x25, 0xfe},
	{0x26, 0x01},
	{0x27, 0x2d},
	{0x28, 0x00},
	{0x29, 0x48},
	{0x2a, 0x30},
	{0x2b, 0x60},
	{0x2c, 0x3a},
	{0x2d, 0x5a},
	{0x2e, 0x40},
	{0x2f, 0x06},
	{0x30, 0x9e},
	{0x31, 0x20},
	{0x32, 0x01},
	{0x33, 0x90},
	{0x35, 0x25},
	{0x36, 0xca},
	{0x37, 0x00},
	{0x38, 0x00},
	{0x39, 0x18},
	{0x3a, 0x32},
	{0x3b, 0x26},
	{0x3c, 0x00},
	{0x3d, 0x60},
	{0x3e, 0x00},
	{0x3f, 0x00},
//channel ID
	{0x40, 0x00},
	{0x34, 0x10},
	{0x40, 0x01},
	{0x34, 0x11},
	{0x40, 0x02},
	{0x34, 0x10},
	{0x40, 0x03},
	{0x34, 0x11},
//output format
	{0x4f, 0x03},
	{0x50, 0x00},
	{0x52, 0x00},
	{0xf1, 0x04},
	{0xf2, 0x77},
	{0xf3, 0x77},
	{0xf4, 0x00},
	{0xf5, 0x0f},
	{0xf6, 0x10},
	{0xf8, 0x23},
	{0xfa, 0x88},
	{0xfb, 0x88},
//output enable
	{0x4d, 0x07},
	{0x4e, 0x05},

};

__maybe_unused static struct regval_list reg_720p25_4ch[] = {
	{0x40, 0x04},
	{0x3b, 0x20},
	{0x3d, 0xe0},
	{0x3d, 0x60},
	{0x3b, 0x25},
	{0x40, 0x40},
	{0x7a, 0x20},
	{0x3c, 0x20},
	{0x3c, 0x00},
	{0x7a, 0x25},
	{0x40, 0x00},
	{0x44, 0x17},
	{0x43, 0x12},
	{0x45, 0x09},
	{0xffff, 0x64},
//video setting
	{0x40, 0x04},
	{0x02, 0x4e},
	{0x05, 0x00},
	{0x06, 0x32},
	{0x07, 0xc0},
	{0x08, 0x00},
	{0x09, 0x24},
	{0x0a, 0x48},
	{0x0b, 0xc0},
	{0x0c, 0x13},
	{0x0d, 0x71},
	{0x0e, 0x00},
	{0x0f, 0x00},
	{0x10, 0x00},
	{0x11, 0x40},
	{0x12, 0x40},
	{0x13, 0x00},
	{0x14, 0x00},
	{0x15, 0x13},
	{0x16, 0x16},
	{0x17, 0x00},
	{0x18, 0x19},
	{0x19, 0xd0},
	{0x1a, 0x25},
	{0x1b, 0x00},
	{0x1c, 0x07},
	{0x1d, 0xbc},
	{0x1e, 0x60},
	{0x1f, 0x06},
	{0x20, 0x40},
	{0x21, 0x46},
	{0x22, 0x36},
	{0x23, 0x3c},
	{0x24, 0x04},
	{0x25, 0xfe},
	{0x26, 0x01},
	{0x27, 0x2d},
	{0x28, 0x00},
	{0x29, 0x48},
	{0x2a, 0x30},
	{0x2b, 0x60},
	{0x2c, 0x3a},
	{0x2d, 0x5A},
	{0x2e, 0x40},
	{0x2f, 0x06},
	{0x30, 0x9e},
	{0x31, 0x20},
	{0x32, 0x01},
	{0x33, 0x90},
	{0x35, 0x25},
	{0x36, 0xca},
	{0x37, 0x00},
	{0x38, 0x00},
	{0x39, 0x18},
	{0x3a, 0x32},
	{0x3b, 0x26},
	{0x3c, 0x00},
	{0x3d, 0x60},
	{0x3e, 0x00},
	{0x3f, 0x00},
//channel ID
	{0x40, 0x00},
	{0x34, 0x10},
	{0x40, 0x01},
	{0x34, 0x11},
	{0x40, 0x02},
	{0x34, 0x12},
	{0x40, 0x03},
	{0x34, 0x13},
//output format
	{0x4f, 0x03},
	{0x50, 0xA3},
	{0x52, 0xE7},
	{0xf1, 0x04},
	{0xf2, 0x33},
	{0xf3, 0x33},
	{0xf4, 0x00},
	{0xf5, 0x0f},
	{0xf6, 0x10},
	{0xf8, 0x54},
	{0xfa, 0x88},
	{0xfb, 0x88},
//output enable
	{0x4d, 0x07},
	{0x4e, 0x05},
};

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	if (on_off)
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
	else
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
	return 0;
}

static int sensor_power(struct v4l2_subdev *sd, int on)
{
	switch (on) {
	case STBY_ON:
		sensor_dbg("CSI_SUBDEV_STBY_ON!\n");
		sensor_s_sw_stby(sd, ON);
		break;
	case STBY_OFF:
		sensor_dbg("CSI_SUBDEV_STBY_OFF!\n");
		sensor_s_sw_stby(sd, OFF);
		break;
	case PWR_ON:
		sensor_dbg("CSI_SUBDEV_PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, RESET, CSI_GPIO_HIGH);
		vin_gpio_set_status(sd, PWDN, CSI_GPIO_HIGH);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
/*		vin_set_pmu_channel(sd, RESERVEVDD, ON); */ /* not support yet */
		vin_set_pmu_channel(sd, CAMERAVDD, ON);
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(1000, 1200);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("CSI_SUBDEV_PWR_OFF!\n");
		cci_lock(sd);
		vin_set_mclk(sd, OFF);
		vin_gpio_set_status(sd, RESET, CSI_GPIO_HIGH);
/*		vin_set_pmu_channel(sd, RESERVEVDD, OFF);  *//* not support yet */
		vin_set_pmu_channel(sd, CAMERAVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_set_status(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		cci_unlock(sd);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
	usleep_range(5000, 6000);
	vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
	usleep_range(5000, 6000);
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	data_type rdval, rdval1, rdval2;
	int cnt = 0;

	rdval = 0;
	rdval1 = 0;
	rdval2 = 0;
	DBG_INFO("\n");
	sensor_read(sd, 0xfe, &rdval1);
	sensor_read(sd, 0xff, &rdval2);
	rdval = ((rdval2 << 8) & 0xff00) | rdval1;
	pr_err("V4L2_IDENT_SENSOR = 0x%x\n", rdval);

	while ((rdval != V4L2_IDENT_SENSOR) && (cnt < 5)) {
		sensor_read(sd, 0xfe, &rdval1);
		sensor_read(sd, 0xff, &rdval2);
		rdval = ((rdval2 << 8) & 0xff00) | rdval1;
		DBG_INFO("retry = %d, V4L2_IDENT_SENSOR = %x\n", cnt,
			     rdval);
		cnt++;
	}

	if (rdval != V4L2_IDENT_SENSOR)
		return -ENODEV;
	DBG_INFO("tp9930 detect ok !!!");
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
	info->width = HD720_WIDTH;
	info->height = HD720_HEIGHT;
	info->hflip = 0;
	info->vflip = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 25;	/* 25fps */

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
	.desc = "BT656 4CH",
#if 1 /* BT1120 */
	.mbus_code = MEDIA_BUS_FMT_YUYV8_1X16,
#else /* BT656 */
	.mbus_code = MEDIA_BUS_FMT_YUYV8_2X8,
#endif
	.regs = NULL,
	.regs_size = 0,
	.bpp = 2,
	},
};

#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */

static struct sensor_win_size sensor_win_sizes[] = {
	{
		.width = HD1080_WIDTH,
		.height = HD1080_HEIGHT,
		.hoffset = 0,
		.voffset = 0,
		.fps_fixed = 25,
		.regs = reg_1080p25_4ch,
		.regs_size = ARRAY_SIZE(reg_1080p25_4ch),
		.pclk_dly = 0x06, // should not larger than 0x1f
		.set_size = NULL,
	},
	{
		.width = HD720_WIDTH,
		.height = HD720_HEIGHT,
		.hoffset = 0,
		.voffset = 0,
		.fps_fixed = 25,
		.regs = reg_720p25_4ch,
		.regs_size = ARRAY_SIZE(reg_720p25_4ch),
		.pclk_dly = 0x06, // should not larger than 0x1f
		.set_size = NULL,
	},
/*
	{
		.width = HD1080_WIDTH,
		.height = HD1080_HEIGHT,
		.hoffset = 0,
		.voffset = 0,
		.fps_fixed = 25,
		.regs = reg_1080p25_2ch,
		.regs_size = ARRAY_SIZE(reg_1080p25_2ch),
		.set_size = NULL,
	},
	{
		.width = HD720_WIDTH,
		.height = HD720_HEIGHT,
		.hoffset = 0,
		.voffset = 0,
		.fps_fixed = 25,
		.regs = reg_720p25_2ch,
		.regs_size = ARRAY_SIZE(reg_720p25_2ch),
		.set_size = NULL,
	},
*/
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				struct v4l2_mbus_config *cfg)
{
	struct sensor_info *info = to_state(sd);
	cfg->type = V4L2_MBUS_BT656;

	if (info->current_wins->width_input == 1920 && info->current_wins->height_input == 1080) {
		cfg->flags = DOUBLE_CLK_POL | CSI_CH_0 | CSI_CH_1 | CSI_CH_2 | CSI_CH_3;
	} else {
		cfg->flags = CLK_POL | CSI_CH_0 | CSI_CH_1 | CSI_CH_2 | CSI_CH_3;
	}
	return 0;
}

static int sensor_g_ctrl(struct v4l2_ctrl *ctrl)
{
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_ctrl *ctrl)
{
	return -EINVAL;
}

static int sensor_reg_init(struct sensor_info *info)
{
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	sensor_print("sensor_reg_init sensor_fmt->regs_size = %d\n", sensor_fmt->regs_size);

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);
	sensor_print("sensor_reg_init end %dx%d  wsize->regs_size=%d\n", wsize->width, wsize->height, wsize->regs_size);

	if (wsize->set_size)
		wsize->set_size(sd);

	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;
	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);

	DBG_INFO("%s on = %d, %d*%d %x\n", __func__, enable,
		  info->current_wins->width,
		  info->current_wins->height, info->fmt->mbus_code);

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
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_stream = sensor_s_stream,
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

	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;

	sd = cci_dev_remove_helper(client, &cci_drv);

	kfree(to_state(sd));
	return 0;
}

static void sensor_shutdown(struct i2c_client *client)
{
	struct v4l2_subdev *sd;

	sd = cci_dev_remove_helper(client, &cci_drv);

	kfree(to_state(sd));
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
	.shutdown = sensor_shutdown,
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
