/*
 * A V4L2 driver for rn6854m cameras and AHD Coax protocol.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Chen Weihong <chenweihong@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

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


MODULE_AUTHOR("cwh");
MODULE_DESCRIPTION("A low-level driver for rn6854m mipi chip for cvbs sensor");
MODULE_LICENSE("GPL");

/*define module timing*/
#define MCLK              (27*1000*1000)
#define VREF_POL          V4L2_MBUS_VSYNC_ACTIVE_LOW
#define HREF_POL          V4L2_MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_RISING
#define V4L2_IDENT_SENSOR  0x05

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE	25

/*
 * The rn6854m i2c address
 */

#define I2C_ADDR  0x58 //0x5a,0x2c,0x2d

/*static struct delayed_work sensor_s_ae_ratio_work;*/
static bool restart;
#define SENSOR_NAME "rn6854m_mipi"

struct cfg_array {		/* coming later */
	struct regval_list *regs;
	int size;
};


/*
 * The default register settings
 *
 */
static struct regval_list sensor_default_regs[] = {

};

static struct regval_list sensor_720p_25fps_regs[] = {

	{0x81, 0x0F}, // turn on video decoder
	{0xDF, 0xF0}, // enable HD format
	{0x88, 0x00},
	{0xF6, 0x00},
	// ch0
	{0xFF, 0x00}, // switch to ch0
	{0x00, 0x20}, // internal use*
	{0x06, 0x08}, // internal use*
	{0x07, 0x63}, // HD format
	{0x2A, 0x01}, // filter control
	{0x3A, 0x20}, // Insert Channel ID in SAV/EAV code
	{0x3F, 0x10}, // channel ID
	{0x4C, 0x37}, // equalizer
	{0x4F, 0x03}, // sync control
	{0x50, 0x02}, // 720p resolution
	{0x56, 0x01}, // BT 72M mode
	{0x5F, 0x40}, // blank level
	{0x63, 0xF5}, // filter control
	{0x59, 0x00}, // extended register access
	{0x5A, 0x42}, // data for extended register
	{0x58, 0x01}, // enable extended register write
	{0x59, 0x33}, // extended register access
	{0x5A, 0x23}, // data for extended register
	{0x58, 0x01}, // enable extended register write
	{0x51, 0xE1}, // scale factor1
	{0x52, 0x88}, // scale factor2
	{0x53, 0x12}, // scale factor3
	{0x5B, 0x07}, // H-scaling control
	{0x5E, 0x0B}, // enable H-scaling control
	{0x6A, 0x82}, // H-scaling control
	{0x28, 0x92}, // cropping
	{0x03, 0x80}, // saturation
	{0x04, 0x80}, // hue
	{0x05, 0x04}, // sharpness
	{0x57, 0x23}, // black/white stretch
	{0x68, 0x32}, // coring
	{0x37, 0x33},
	{0x61, 0x6C},
	// ch1    ,
	{0xFF, 0x01}, // switch to ch1
	{0x00, 0x20}, // internal use*
	{0x06, 0x08}, // internal use*
	{0x07, 0x63}, // HD format
	{0x2A, 0x01}, // filter control
	{0x3A, 0x20}, // Insert Channel ID in SAV/EAV code
	{0x3F, 0x11}, // channel ID
	{0x4C, 0x37}, // equalizer
	{0x4F, 0x03}, // sync control
	{0x50, 0x02}, // 720p resolution
	{0x56, 0x01}, // BT 72M mode
	{0x5F, 0x40}, // blank level
	{0x63, 0xF5}, // filter control
	{0x59, 0x00}, // extended register access
	{0x5A, 0x42}, // data for extended register
	{0x58, 0x01}, // enable extended register write
	{0x59, 0x33}, // extended register access
	{0x5A, 0x23}, // data for extended register
	{0x58, 0x01}, // enable extended register write
	{0x51, 0xE1}, // scale factor1
	{0x52, 0x88}, // scale factor2
	{0x53, 0x12}, // scale factor3
	{0x5B, 0x07}, // H-scaling control
	{0x5E, 0x0B}, // enable H-scaling control
	{0x6A, 0x82}, // H-scaling control
	{0x28, 0x92}, // cropping
	{0x03, 0x80}, // saturation
	{0x04, 0x80}, // hue
	{0x05, 0x04}, // sharpness
	{0x57, 0x23}, // black/white stretch
	{0x68, 0x32}, // coring
	{0x37, 0x33},
	{0x61, 0x6C},

	// ch2
	{0xFF, 0x02}, // switch to ch2
	{0x00, 0x20}, // internal use*
	{0x06, 0x08}, // internal use*
	{0x07, 0x63}, // HD format
	{0x2A, 0x01}, // filter control
	{0x3A, 0x20}, // Insert Channel ID in SAV/EAV code
	{0x3F, 0x12}, // channel ID
	{0x4C, 0x37}, // equalizer
	{0x4F, 0x03}, // sync control
	{0x50, 0x02}, // 720p resolution
	{0x56, 0x01}, // BT 72M mode
	{0x5F, 0x40}, // blank level
	{0x63, 0xF5}, // filter control
	{0x59, 0x00}, // extended register access
	{0x5A, 0x42}, // data for extended register
	{0x58, 0x01}, // enable extended register write
	{0x59, 0x33}, // extended register access
	{0x5A, 0x23}, // data for extended register
	{0x58, 0x01}, // enable extended register write
	{0x51, 0xE1}, // scale factor1
	{0x52, 0x88}, // scale factor2
	{0x53, 0x12}, // scale factor3
	{0x5B, 0x07}, // H-scaling control
	{0x5E, 0x0B}, // enable H-scaling control
	{0x6A, 0x82}, // H-scaling control
	{0x28, 0x92}, // cropping
	{0x03, 0x80}, // saturation
	{0x04, 0x80}, // hue
	{0x05, 0x04}, // sharpness
	{0x57, 0x23}, // black/white stretch
	{0x68, 0x32}, // coring
	{0x37, 0x33},
	{0x61, 0x6C},

	// ch3    ,
	{0xFF, 0x03}, // switch to ch1
	{0x00, 0x20}, // internal use*
	{0x06, 0x08}, // internal use*
	{0x07, 0x63}, // HD format
	{0x2A, 0x01}, // filter control
	{0x3A, 0x20}, // Insert Channel ID in SAV/EAV code
	{0x3F, 0x13}, // channel ID
	{0x4C, 0x37}, // equalizer
	{0x4F, 0x03}, // sync control
	{0x50, 0x02}, // 720p resolution
	{0x56, 0x01}, // BT 72M mode
	{0x5F, 0x40}, // blank level
	{0x63, 0xF5}, // filter control
	{0x59, 0x00}, // extended register access
	{0x5A, 0x42}, // data for extended register
	{0x58, 0x01}, // enable extended register write
	{0x59, 0x33}, // extended register access
	{0x5A, 0x23}, // data for extended register
	{0x58, 0x01}, // enable extended register write
	{0x51, 0xE1}, // scale factor1
	{0x52, 0x88}, // scale factor2
	{0x53, 0x12}, // scale factor3
	{0x5B, 0x07}, // H-scaling control
	{0x5E, 0x0B}, // enable H-scaling control
	{0x6A, 0x82}, // H-scaling control
	{0x28, 0x92}, // cropping
	{0x03, 0x80}, // saturation
	{0x04, 0x80}, // hue
	{0x05, 0x04}, // sharpness
	{0x57, 0x23}, // black/white stretch
	{0x68, 0x32}, // coring
	{0x37, 0x33},
	{0x61, 0x6C},
	// mipi lik1
	{0xFF, 0x09}, // switch to mipi tx1
	{0x00, 0x03}, // enable bias
	{0xFF, 0x08}, // switch to mipi csi1
	{0x04, 0x03}, // csi1 and tx1 reset
	{0x6C, 0x1F}, // disable ch output; turn on ch0
	{0x06, 0x7C}, // 4 lanes
	{0x21, 0x01}, // enable hs clock
	{0x78, 0x80}, // Y/C counts for ch0
	{0x79, 0x02}, // Y/C counts for ch0
	{0x7A, 0x80}, // Y/C counts for ch1
	{0x7B, 0x02}, // Y/C counts for ch1
	{0x7C, 0x80}, // Y/C counts for ch2
	{0x7D, 0x02}, // Y/C counts for ch2
	{0x7E, 0x80}, // Y/C counts for ch3
	{0x7F, 0x02}, // Y/C counts for ch3
	{0x6C, 0x0F}, // enable ch output
	{0x04, 0x00}, // csi1 and tx1 reset finish
	//{0x07, 0x05}, // Enable non-continuous clock
	 {0x20, 0xAA}, // invert clock phase
	// mipi lik3
	{0xFF, 0x0A}, // switch to mipi csi3
	{0x6C, 0x10}, // disable ch output; turn off ch0~3
	{0xd3, 0x50}
};

static struct regval_list sensor_1080p_25fps_regs[] = {

	{0x81, 0x03}, // turn on video decoder0
	{0xDF, 0xFC}, // enable ch0 as HD format
	{0xF0, 0xC0}, // 144MHz output
	{0x88, 0x40}, // disable SCLK0B out
	{0xF6, 0x40}, // disable SCLK3A out

	// ch0
	{0xFF, 0x00}, // switch to ch0 (default; optional)
	{0x00, 0x20}, // internal use*
	{0x06, 0x08}, // internal use*
	{0x07, 0x63}, // HD format
	{0x2A, 0x01}, // filter control
	{0x3A, 0x20}, // Insert Channel ID in SAV/EAV code
	{0x3F, 0x10}, // channel ID
	{0x4C, 0x37}, // equalizer
	{0x4F, 0x03}, // sync control
	{0x50, 0x03}, // 1080p resolution
	{0x56, 0x02}, // BT 144M mode
	{0x5F, 0x44}, // blank level
	{0x63, 0xF8}, // filter control
	{0x59, 0x00}, // extended register access
	{0x5A, 0x48}, // data for extended register
	{0x58, 0x01}, // enable extended register write
	{0x59, 0x33}, // extended register access
	{0x5A, 0x23}, // data for extended register
	{0x58, 0x01}, // enable extended register write
	{0x51, 0xF4}, // scale factor1
	{0x52, 0x29}, // scale factor2
	{0x53, 0x15}, // scale factor3
	{0x5B, 0x01}, // H-scaling control
	{0x5E, 0x0F}, // enable H-scaling control
	{0x6A, 0x87}, // H-scaling control
	{0x28, 0x92}, // cropping
	{0x03, 0x80}, // saturation
	{0x04, 0x80}, // hue
	{0x05, 0x04}, // sharpness
	{0x57, 0x23}, // black/white stretch
	{0x68, 0x00}, // coring
	{0x37, 0x33},
	{0x61, 0x6C},

	// ch1
	{0xFF, 0x01}, // switch to ch1 (default; optional)
	{0x00, 0x20}, // internal use*
	{0x06, 0x08}, // internal use*
	{0x07, 0x63}, // HD format
	{0x2A, 0x01}, // filter control
	{0x3A, 0x20}, // Insert Channel ID in SAV/EAV code
	{0x3F, 0x11}, // channel ID
	{0x4C, 0x37}, // equalizer
	{0x4F, 0x03}, // sync control
	{0x50, 0x03}, // 1080p resolution
	{0x56, 0x02}, // BT 144M mode
	{0x5F, 0x44}, // blank level
	{0x63, 0xF8}, // filter control
	{0x59, 0x00}, // extended register access
	{0x5A, 0x48}, // data for extended register
	{0x58, 0x01}, // enable extended register write
	{0x59, 0x33}, // extended register access
	{0x5A, 0x23}, // data for extended register
	{0x58, 0x01}, // enable extended register write
	{0x51, 0xF4}, // scale factor1
	{0x52, 0x29}, // scale factor2
	{0x53, 0x15}, // scale factor3
	{0x5B, 0x01}, // H-scaling control
	{0x5E, 0x0F}, // enable H-scaling control
	{0x6A, 0x87}, // H-scaling control
	{0x28, 0x92}, // cropping
	{0x03, 0x80}, // saturation
	{0x04, 0x80}, // hue
	{0x05, 0x04}, // sharpness
	{0x57, 0x23}, // black/white stretch
	{0x68, 0x00}, // coring
	{0x37, 0x33},
	{0x61, 0x6C},

	// mipi link1
	{0xFF, 0x09}, // switch to mipi tx1
	{0x00, 0x03}, // enable bias
	{0xFF, 0x08}, // switch to mipi csi1
	{0x04, 0x03}, // csi1 and tx1 reset
	{0x6C, 0x13}, // disable ch output; turn on ch0
	{0x06, 0x7C}, // 4 lanes
	{0x21, 0x01}, // enable hs clock
	{0x78, 0xC0}, // Y/C counts for ch0
	{0x79, 0x03}, // Y/C counts for ch0
	{0x7A, 0xC0}, // Y/C counts for ch1
	{0x7B, 0x03}, // Y/C counts for ch1
	{0x6C, 0x03}, // enable ch output
	{0x04, 0x00}, // csi1 and tx1 reset finish
	//0x07, 0x05, //enable non-continuous clock
	{0x20, 0xAA}, // invert hs clock

	// mipi link3
	{0xFF, 0x0A}, // switch to mipi csi3
	{0x6C, 0x10}, // disable ch output; turn off ch0~3
};

struct v4l2_subdev *rn6854m_sd;

static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	return 0;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	return 0;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val;
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

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
		cci_lock(sd);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vin_set_mclk(sd, OFF);
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
		ret = sensor_s_sw_stby(sd, CSI_GPIO_LOW);
		if (ret < 0)
			sensor_err("soft stby off falied!\n");
		usleep_range(20000, 22000);

		break;
	case PWR_ON:
		sensor_dbg("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(1000, 1200);
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(30000, 32000);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(50000, 52000);
		cci_unlock(sd);
		break;

	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");
		cci_lock(sd);
		vin_set_mclk(sd, OFF);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_gpio_set_status(sd, RESET, 0);
		vin_gpio_set_status(sd, PWDN, 0);
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
	switch (val) {
	case 0:
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(30000, 32000);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(60000, 62000);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	int i = 0;
	data_type rdval = 0;

	sensor_read(sd, 0xfe, &rdval);
	sensor_print("reg 0x%x = 0x%x\n", 0xfe, rdval);
	while ((rdval != V4L2_IDENT_SENSOR) && (i < 5)) {
		sensor_read(sd, 0xfe, &rdval);
		sensor_dbg("reg 0x%x = 0x%x\n", 0xfe, rdval);
		i++;
	}
	if (rdval != V4L2_IDENT_SENSOR) {
		sensor_dbg("reg 0x%x = 0x%x\n", 0xfe, rdval);
		return -EINVAL;
	}


	return 0;
}
static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_init\n");
	restart = 0;
	/*Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = 1280;
	info->height = 720;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->tpf.numerator = 1;
	info->tpf.denominator = 25;

	return 0;
}


static int sensor_initial(struct v4l2_subdev *sd)
{
	data_type rom_byte1, rom_byte2, rom_byte3, rom_byte4, rom_byte5, rom_byte6;
	sensor_dbg("sensor_initial\n");
	sensor_write(sd, 0xE1, 0x80);
		sensor_write(sd, 0xFA, 0x81);
		sensor_read (sd, 0xFB, &rom_byte1);
		sensor_read (sd, 0xFB, &rom_byte2);
		sensor_read (sd, 0xFB, &rom_byte3);
		sensor_read (sd, 0xFB, &rom_byte4);
		sensor_read (sd, 0xFB, &rom_byte5);
		sensor_read (sd, 0xFB, &rom_byte6);

		// config. decoder accroding to rom_byte5 and rom_byte6
		if ((rom_byte6 == 0x00) && (rom_byte5 == 0x00)) {
			sensor_write(sd, 0xEF, 0xAA);
			sensor_write(sd, 0xE7, 0xFF);
			sensor_write(sd, 0xFF, 0x09);
			sensor_write(sd, 0x03, 0x0C);
			sensor_write(sd, 0xFF, 0x0B);
			sensor_write(sd, 0x03, 0x0C);
		} else if (((rom_byte6 == 0x34) && (rom_byte5 == 0xA9)) ||
			 ((rom_byte6 == 0x2C) && (rom_byte5 == 0xA8))) {
			sensor_write(sd, 0xEF, 0xAA);
			sensor_write(sd, 0xE7, 0xFF);
			sensor_write(sd, 0xFC, 0x60);
			sensor_write(sd, 0xFF, 0x09);
			sensor_write(sd, 0x03, 0x18);
			sensor_write(sd, 0xFF, 0x0B);
			sensor_write(sd, 0x03, 0x18);
		} else {
			sensor_write(sd, 0xEF, 0xAA);
			sensor_write(sd, 0xFC, 0x60);
			sensor_write(sd, 0xFF, 0x09);
			sensor_write(sd, 0x03, 0x18);
			sensor_write(sd, 0xFF, 0x0B);
			sensor_write(sd, 0x03, 0x18);
		}

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

static struct sensor_format_struct sensor_formats[] = {
	{
		.desc		= "BT656 4CH",
		.mbus_code	= MEDIA_BUS_FMT_UYVY8_2X8,
		.regs 		= sensor_default_regs,
		.regs_size = ARRAY_SIZE(sensor_default_regs),
		.bpp		= 2,
	}
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */
static struct sensor_win_size sensor_win_sizes[] = {

	  {
	  .width = 1280,
	  .height = 720,
	  .hoffset = 0,
	  .voffset = 0,
	  .pclk = 567*1000*1000,
	  .mipi_bps = 567*1000*1000,
	  .fps_fixed = 25,
	  .regs = sensor_720p_25fps_regs,
	  .regs_size = ARRAY_SIZE(sensor_720p_25fps_regs),
	  .set_size = NULL,
	  },
	  {
	  .width = 1920,
	  .height = 1080,
	  .hoffset = 0,
	  .voffset = 0,
	  .pclk = 567*1000*1000,
	  .mipi_bps = 567*1000*1000,
	  .fps_fixed = 25,
	  .regs = sensor_1080p_25fps_regs,
	  .regs_size = ARRAY_SIZE(sensor_1080p_25fps_regs),
	  .set_size = NULL,
	  },

};
#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	struct sensor_info *info = to_state(sd);
	cfg->type = V4L2_MBUS_CSI2_DPHY;
	if (info->current_wins->width_input == 1920 && info->current_wins->height_input == 1080)
		cfg->flags = V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0 | V4L2_MBUS_CSI2_CHANNEL_1 ;
	else
		cfg->flags = V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0 | V4L2_MBUS_CSI2_CHANNEL_1 | V4L2_MBUS_CSI2_CHANNEL_2 | V4L2_MBUS_CSI2_CHANNEL_3;
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

	return 0;
}

static int sensor_reg_init(struct sensor_info *info)
{
	int ret;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	sensor_dbg("sensor_reg_init\n");
	sensor_initial(sd);
	ret = sensor_write_array(sd, sensor_default_regs,
				 ARRAY_SIZE(sensor_default_regs));
	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}
	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);

	if (wsize->set_size)
		wsize->set_size(sd);

	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;
	sensor_dbg("s_fmt set width = %d, height = %d\n", wsize->width,
		      wsize->height);
	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);
	sensor_print("%s on = %d, %d*%d %x\n", __func__, enable,
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
#ifdef CONFIG_COMPAT
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

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);
	sensor_init_controls(sd, &sensor_ctrl_ops);
	mutex_init(&info->lock);
	restart = 0;

#ifdef CONFIG_SAME_I2C
	info->sensor_i2c_addr = I2C_ADDR >> 1;
#endif
	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	/* info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET1 | MIPI_NORMAL_MODE; */
	info->combo_mode = CMB_PHYA_OFFSET3 | MIPI_NORMAL_MODE;
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->af_first_flag = 1;
	info->exp = 0;
	info->gain = 0;


	rn6854m_sd = sd;

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

#ifdef CONFIG_VIDEO_SUNXI_VIN_SPECIAL
subsys_initcall_sync(init_sensor);
#else
module_init(init_sensor);
#endif
module_exit(exit_sensor);
