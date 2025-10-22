/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for tp2815 cameras and TVI Coax protocol.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Hongyi <hongyi@allwinnertech.com>
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

MODULE_AUTHOR("HY");
MODULE_DESCRIPTION("A low-level driver for tp2815 mipi chip for TVI sensor");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.1");

/*define module timing*/
#define MCLK              (27*1000*1000)
#define V4L2_IDENT_SENSOR  0x40

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE	25

#define Test_flag 1   // add adaptive function
#define Print_reg 0	  // print reg
/*
 * The tp2815 i2c address
 */

#define I2C_ADDR  0x88

//TP28xx audio
//both record and playback are master, 20ch,I2S mode, backend can use 16ch mode to capture.
#define I2S  0
#define DSP  1
#define AUDIO_FORMAT   I2S

#define SAMPLE_8K    0
#define SAMPLE_16K   1
#define SAMPLE_RATE  SAMPLE_16K

#define DATA_16BIT  0
#define DATA_8BIT   1
#define DATA_BIT    DATA_16BIT

#define AUDIO_CHN   4

enum{
	CH_1 = 0,   //
	CH_2 = 1,   //
	CH_3 = 2,   //
	CH_4 = 3,   //
	CH_ALL = 4,   //
	MIPI_PAGE = 8,
	APAGE = 0x40,
};
enum{
	STD_TVI, //TVI
	STD_HDA, //AHD
};
enum{
	PAL,
	NTSC,
	HD25,  //720p25
	HD30,  //720p30
	FHD25, //1080p25
	FHD30, //1080p30
	FHD50, //1080p50
	FHD60, //1080p60
	QHD25, //2560x1440p25
	QHD30, //2560x1440p30
	UVGA25,  //1280x960p25, must use with MIPI_4CH4LANE_445M
	UVGA30,  //1280x960p30, must use with MIPI_4CH4LANE_445M
	HD30HDR, //special 720p30 with ISX019, must use with MIPI_4CH4LANE_396M
	HD50,    //720p50
	HD60,    //720p60
	A_UVGA30,  //HDA 1280x960p30, must use with MIPI_4CH4LANE_378M
	F_UVGA30,  //FH 1280x960p30, must use with MIPI_4CH4LANE_432M
	UVGA30_945, //TVI 1280x960p30, must use with MIPI_4CH4LANE_378M
	FHD275,   //1080p27.5
	FMT5M20,
	FMT5M12,
	FMT8M15,
	FMT8M12,
	FMT8M7,
	HD30864, //total 1600x900 86.4M
};
enum{
	MIPI_4CH4LANE_297M, //up to 4x720p25/30
	MIPI_4CH4LANE_594M, //up to 4x1080p25/30
	MIPI_4CH2LANE_594M, //up to 4x720pp25/30
	MIPI_4CH4LANE_445M, //only for 4x960p25/30
	MIPI_2CH4LANE_297M, //up to 2x1080p25/30
	MIPI_2CH4LANE_594M, //up to 2xQHDp25/30 or 2x1080p50/60
	MIPI_4CH4LANE_396M, //only for 4xHD30HDR
	MIPI_4CH4LANE_378M, //only for 4xA_UVGA30
	MIPI_4CH4LANE_432M, //only for 4xF_UVGA30
	MIPI_1CH2LANE_594M,
	MIPI_3CH4LANE_594M,
	MIPI_4CH4LANE_345M, //only for HD30864
	MIPI_2CH2LANE_297M, //up to 2x720p25/30
	MIPI_2CH2LANE_594M, //up to 2x1080p25/30
};

/*static struct delayed_work sensor_s_ae_ratio_work;*/
static bool restart;
#define SENSOR_NUM 0x2
#define SENSOR_NAME "tp2815_mipi"
#define SENSOR_NAME_2 "tp2815_mipi_2"

void tp2815_decoder_init(struct v4l2_subdev *sd, unsigned char ch, unsigned char fmt, unsigned char std);
int tp2815_hardware_init(struct v4l2_subdev *sd, unsigned char fmt);
void tp2815_mipi_out(struct v4l2_subdev *sd, unsigned char output);

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

static struct regval_list sensor_1080p_25fps_regs[] = {
};


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
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(1000, 1200);

		vin_set_pmu_channel(sd, CAMERAVDD, ON);
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);/*CSI_GPIO_HIGH*/
		usleep_range(20000, 22000);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(10000, 10200);  //delay 10ms
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF\n");
		cci_lock(sd);
		vin_set_mclk(sd, OFF);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_set_status(sd, RESET, CSI_GPIO_LOW);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
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
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);//HIGH
		usleep_range(30000, 32000);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(60000, 62000);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
#if !defined CONFIG_VIN_INIT_MELIS
	int i = 0;
	data_type rdval = 0;

	sensor_write(sd, 0x40, 0x00);
	sensor_read(sd, 0xff, &rdval);
	sensor_print("reg 0xff = 0x%x --N\n", rdval);
	sensor_read(sd, 0xfe, &rdval);
	sensor_print("reg 0xfe = 0x%x --N\n", rdval);

	while ((rdval != 0x28) && (i < 5)) {
		sensor_read(sd, 0x1E, &rdval);
		sensor_dbg("reg 0x%x = 0x%x\n", 0x1e, rdval);
		i++;
	}
	if (rdval != 0x28) {
		sensor_err("No tp2815 address 0x88 is connectted\n");
		//return -ENXIO;
	}
#endif

	return 0;
}


/////////////////////////////////
//ch: video channel
//fmt: PAL/NTSC/HD25/HD30...
//std: STD_TVI/STD_HDA
////////////////////////////////
void tp2815_decoder_init(struct v4l2_subdev *sd, unsigned char ch, unsigned char fmt, unsigned char std)
{
	data_type tmp;
	const unsigned char SYS_MODE[5] = {0x01, 0x02, 0x04, 0x08, 0x0f};

	sensor_print("tp2815_decoder_init\n");

	sensor_write(sd, 0x40, ch);
	sensor_write(sd, 0x45, 0x01);
	sensor_write(sd, 0x06, 0x12); //default value
	sensor_write(sd, 0x27, 0x2d); //default value

	if (PAL == fmt) {
		sensor_read(sd, 0xf5, &tmp);
		tmp |= SYS_MODE[ch];
		sensor_write(sd, 0xf5, tmp);
		sensor_write(sd, 0x06, 0x32);
		sensor_write(sd, 0x02, 0x47);
		sensor_write(sd, 0x07, 0x80);
		sensor_write(sd, 0x0b, 0x80);
		sensor_write(sd, 0x0c, 0x13);
		sensor_write(sd, 0x0d, 0x51);
		sensor_write(sd, 0x15, 0x03);
		sensor_write(sd, 0x16, 0xf0);
		sensor_write(sd, 0x17, 0xa0);
		sensor_write(sd, 0x18, 0x17);
		sensor_write(sd, 0x19, 0x20);
		sensor_write(sd, 0x1a, 0x15);
		sensor_write(sd, 0x1c, 0x06);
		sensor_write(sd, 0x1d, 0xc0);
		sensor_write(sd, 0x20, 0x48);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x37);
		sensor_write(sd, 0x23, 0x3f);
		sensor_write(sd, 0x2a, 0x34); // add pattern mode
		sensor_write(sd, 0x2b, 0x70);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x4b);
		sensor_write(sd, 0x2e, 0x56);
		sensor_write(sd, 0x30, 0x7a);
		sensor_write(sd, 0x31, 0x4a);
		sensor_write(sd, 0x32, 0x4d);
		sensor_write(sd, 0x33, 0xfb);
		sensor_write(sd, 0x35, 0x65);
		sensor_write(sd, 0x38, 0x00);
		sensor_write(sd, 0x39, 0x04);
	} else if (NTSC == fmt) {
		sensor_read(sd, 0xf5, &tmp);
		tmp |= SYS_MODE[ch];
		sensor_write(sd, 0xf5, tmp);
		sensor_write(sd, 0x02, 0x47);
		sensor_write(sd, 0x07, 0x80);
		sensor_write(sd, 0x0b, 0x80);
		sensor_write(sd, 0x0c, 0x13);
		sensor_write(sd, 0x0d, 0x50);
		sensor_write(sd, 0x15, 0x03);
		sensor_write(sd, 0x16, 0xd6);
		sensor_write(sd, 0x17, 0xa0);
		sensor_write(sd, 0x18, 0x12);
		sensor_write(sd, 0x19, 0xf0);
		sensor_write(sd, 0x1a, 0x05);
		sensor_write(sd, 0x1c, 0x06);
		sensor_write(sd, 0x1d, 0xb4);
		sensor_write(sd, 0x20, 0x40);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);
		sensor_write(sd, 0x2a, 0x34); // add pattern mode
		sensor_write(sd, 0x2b, 0x70);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x4b);
		sensor_write(sd, 0x2e, 0x57);

		sensor_write(sd, 0x30, 0x62);
		sensor_write(sd, 0x31, 0xbb);
		sensor_write(sd, 0x32, 0x96);
		sensor_write(sd, 0x33, 0xcb);
		sensor_write(sd, 0x35, 0x65);
		sensor_write(sd, 0x38, 0x00);
		sensor_write(sd, 0x39, 0x04);
	} else if (HD25 == fmt) {
		sensor_read(sd, 0xf5, &tmp);
		tmp |= SYS_MODE[ch];
		sensor_write(sd, 0xf5, tmp);
		sensor_write(sd, 0x02, 0x42);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x13);
		sensor_write(sd, 0x0d, 0x50);
		sensor_write(sd, 0x15, 0x13);
		sensor_write(sd, 0x16, 0x15);
		sensor_write(sd, 0x17, 0x00);
		sensor_write(sd, 0x18, 0x19);
		sensor_write(sd, 0x19, 0xd0);
		sensor_write(sd, 0x1a, 0x25);
		sensor_write(sd, 0x1c, 0x07);  //1280*720, 25fps
		sensor_write(sd, 0x1d, 0xbc);  //1280*720, 25fps
		sensor_write(sd, 0x20, 0x30);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);

		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x0a);
		sensor_write(sd, 0x2d, 0x30);
		sensor_write(sd, 0x2e, 0x70);
		sensor_write(sd, 0x30, 0x48);
		sensor_write(sd, 0x31, 0xbb);
		sensor_write(sd, 0x32, 0x2e);
		sensor_write(sd, 0x33, 0x90);
		sensor_write(sd, 0x35, 0x25);
		sensor_write(sd, 0x38, 0x00);
		sensor_write(sd, 0x39, 0x18);

		if (STD_HDA == std) {
			sensor_write(sd, 0x02, 0x46);

			sensor_write(sd, 0x0d, 0x71);

			sensor_write(sd, 0x18, 0x1b);

			sensor_write(sd, 0x20, 0x40);
			sensor_write(sd, 0x21, 0x46);

			sensor_write(sd, 0x25, 0xfe);
			sensor_write(sd, 0x26, 0x01);

			sensor_write(sd, 0x2a, 0x34); // add pattern mode
			sensor_write(sd, 0x2c, 0x3a);
			sensor_write(sd, 0x2d, 0x5a);
			sensor_write(sd, 0x2e, 0x40);

			sensor_write(sd, 0x30, 0x9e);
			sensor_write(sd, 0x31, 0x20);
			sensor_write(sd, 0x32, 0x10);
			sensor_write(sd, 0x33, 0x90);
		}
	} else if (HD30 == fmt) {
		sensor_read(sd, 0xf5, &tmp);
		tmp |= SYS_MODE[ch];
		sensor_write(sd, 0xf5, tmp);
		sensor_write(sd, 0x02, 0x42);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x13);
		sensor_write(sd, 0x0d, 0x50);
		sensor_write(sd, 0x15, 0x13);
		sensor_write(sd, 0x16, 0x15);
		sensor_write(sd, 0x17, 0x00);
		sensor_write(sd, 0x18, 0x19);
		sensor_write(sd, 0x19, 0xd0);
		sensor_write(sd, 0x1a, 0x25);
		sensor_write(sd, 0x1c, 0x06);  //1280*720, 30fps
		sensor_write(sd, 0x1d, 0x72);  //1280*720, 30fps
		sensor_write(sd, 0x20, 0x30);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);
		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x30);
		sensor_write(sd, 0x2e, 0x70);
		sensor_write(sd, 0x30, 0x48);
		sensor_write(sd, 0x31, 0xbb);
		sensor_write(sd, 0x32, 0x2e);
		sensor_write(sd, 0x33, 0x90);
		sensor_write(sd, 0x35, 0x25);
		sensor_write(sd, 0x38, 0x00);
		sensor_write(sd, 0x39, 0x18);

		if (STD_HDA == std) {
			sensor_write(sd, 0x02, 0x46);

			sensor_write(sd, 0x0d, 0x70);

			sensor_write(sd, 0x18, 0x1b);

			sensor_write(sd, 0x20, 0x40);
			sensor_write(sd, 0x21, 0x46);

			sensor_write(sd, 0x25, 0xfe);
			sensor_write(sd, 0x26, 0x01);

			sensor_write(sd, 0x2c, 0x3a);
			sensor_write(sd, 0x2d, 0x5a);
			sensor_write(sd, 0x2e, 0x40);

			sensor_write(sd, 0x30, 0x9d);
			sensor_write(sd, 0x31, 0xca);
			sensor_write(sd, 0x32, 0x01);
			sensor_write(sd, 0x33, 0xd0);
		}
	} else if (FHD30 == fmt) {
		sensor_read(sd, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(sd, 0xf5, tmp);
		sensor_write(sd, 0x02, 0x40);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x03);
		sensor_write(sd, 0x0d, 0x50);
		sensor_write(sd, 0x15, 0x03);
		sensor_write(sd, 0x16, 0xd2);
		sensor_write(sd, 0x17, 0x80);
		sensor_write(sd, 0x18, 0x29);
		sensor_write(sd, 0x19, 0x38);
		sensor_write(sd, 0x1a, 0x47);
		sensor_write(sd, 0x1c, 0x08);  //1920*1080, 30fps
		sensor_write(sd, 0x1d, 0x98);  //
		sensor_write(sd, 0x20, 0x30);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);
		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x30);
		sensor_write(sd, 0x2e, 0x70);
		sensor_write(sd, 0x30, 0x48);
		sensor_write(sd, 0x31, 0xbb);
		sensor_write(sd, 0x32, 0x2e);
		sensor_write(sd, 0x33, 0x90);
		sensor_write(sd, 0x35, 0x05);
		sensor_write(sd, 0x38, 0x00);
		sensor_write(sd, 0x39, 0x1C);

		if (STD_HDA == std) {
			sensor_write(sd, 0x02, 0x44);

			sensor_write(sd, 0x0d, 0x72);

			sensor_write(sd, 0x15, 0x01);
			sensor_write(sd, 0x16, 0xf0);
			sensor_write(sd, 0x18, 0x2a);

			sensor_write(sd, 0x20, 0x38);
			sensor_write(sd, 0x21, 0x46);

			sensor_write(sd, 0x25, 0xfe);
			sensor_write(sd, 0x26, 0x0d);

			sensor_write(sd, 0x2c, 0x3a);
			sensor_write(sd, 0x2d, 0x54);
			sensor_write(sd, 0x2e, 0x40);

			sensor_write(sd, 0x30, 0xa5);
			sensor_write(sd, 0x31, 0x95);
			sensor_write(sd, 0x32, 0xe0);
			sensor_write(sd, 0x33, 0x60);
		}
	} else if (FHD25 == fmt) {
		sensor_read(sd, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(sd, 0xf5, tmp);
		sensor_write(sd, 0x02, 0x40);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x03);
		sensor_write(sd, 0x0d, 0x50);
		sensor_write(sd, 0x15, 0x03);
		sensor_write(sd, 0x16, 0xd2);
		sensor_write(sd, 0x17, 0x80);
		sensor_write(sd, 0x18, 0x29);
		sensor_write(sd, 0x19, 0x38);
		sensor_write(sd, 0x1a, 0x47);
		sensor_write(sd, 0x1c, 0x0a);  //1920*1080, 25fps
		sensor_write(sd, 0x1d, 0x50);  //
		sensor_write(sd, 0x20, 0x30);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);
		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x30);
		sensor_write(sd, 0x2e, 0x70);
		sensor_write(sd, 0x30, 0x48);
		sensor_write(sd, 0x31, 0xbb);
		sensor_write(sd, 0x32, 0x2e);
		sensor_write(sd, 0x33, 0x90);
		sensor_write(sd, 0x35, 0x05);
		sensor_write(sd, 0x38, 0x00);
		sensor_write(sd, 0x39, 0x1C);

		if (STD_HDA == std) {
			sensor_write(sd, 0x02, 0x44);
			sensor_write(sd, 0x0d, 0x73);
			sensor_write(sd, 0x15, 0x01);
			sensor_write(sd, 0x16, 0xf0);
			sensor_write(sd, 0x18, 0x2a);

			sensor_write(sd, 0x20, 0x3c);
			sensor_write(sd, 0x21, 0x46);
			sensor_write(sd, 0x25, 0xfe);
			sensor_write(sd, 0x26, 0x0d);
			sensor_write(sd, 0x2a, 0x34); // add pattern mode
			sensor_write(sd, 0x2c, 0x3a);
			sensor_write(sd, 0x2d, 0x54);
			sensor_write(sd, 0x2e, 0x40);
			sensor_write(sd, 0x30, 0xa5);
			sensor_write(sd, 0x31, 0x86);
			sensor_write(sd, 0x32, 0xfb);
			sensor_write(sd, 0x33, 0x60);
		}
	} else if (FHD275 == fmt) {
		sensor_read(sd, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(sd, 0xf5, tmp);
		sensor_write(sd, 0x02, 0x40);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x03);
		sensor_write(sd, 0x0d, 0x50);

		sensor_write(sd, 0x15, 0x13);
		sensor_write(sd, 0x16, 0x88);
		sensor_write(sd, 0x17, 0x80);
		sensor_write(sd, 0x18, 0x29);
		sensor_write(sd, 0x19, 0x38);
		sensor_write(sd, 0x1a, 0x47);
		sensor_write(sd, 0x1c, 0x09);  //1920*1080, 30fps
		sensor_write(sd, 0x1d, 0x60);  //
		sensor_write(sd, 0x20, 0x30);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);

		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x30);
		sensor_write(sd, 0x2e, 0x70);

		sensor_write(sd, 0x30, 0x48);
		sensor_write(sd, 0x31, 0xbb);
		sensor_write(sd, 0x32, 0x2e);
		sensor_write(sd, 0x33, 0x90);
		sensor_write(sd, 0x35, 0x05);
		sensor_write(sd, 0x38, 0x00);
		sensor_write(sd, 0x39, 0x1C);
	} else if (FHD60 == fmt) {
		sensor_read(sd, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(sd, 0xf5, tmp);
		sensor_write(sd, 0x02, 0x40);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x03);
		sensor_write(sd, 0x0d, 0x50);

		sensor_write(sd, 0x15, 0x03);
		sensor_write(sd, 0x16, 0xf0);
		sensor_write(sd, 0x17, 0x80);
		sensor_write(sd, 0x18, 0x12);
		sensor_write(sd, 0x19, 0x38);
		sensor_write(sd, 0x1a, 0x47);
		sensor_write(sd, 0x1c, 0x08);  //
		sensor_write(sd, 0x1d, 0x96);  //
		sensor_write(sd, 0x20, 0x38);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);
		sensor_write(sd, 0x27, 0xad);
		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x40);
		sensor_write(sd, 0x2e, 0x70);
		sensor_write(sd, 0x30, 0x74);
		sensor_write(sd, 0x31, 0x9b);
		sensor_write(sd, 0x32, 0xa5);
		sensor_write(sd, 0x33, 0xe0);
		sensor_write(sd, 0x35, 0x05);
		sensor_write(sd, 0x38, 0x40);
		sensor_write(sd, 0x39, 0x68);
	} else if (FHD50 == fmt) {
		sensor_read(sd, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(sd, 0xf5, tmp);
		sensor_write(sd, 0x02, 0x40);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x03);
		sensor_write(sd, 0x0d, 0x50);
		sensor_write(sd, 0x15, 0x03);
		sensor_write(sd, 0x16, 0xe2);
		sensor_write(sd, 0x17, 0x80);
		sensor_write(sd, 0x18, 0x27);
		sensor_write(sd, 0x19, 0x38);
		sensor_write(sd, 0x1a, 0x47);
		sensor_write(sd, 0x1c, 0x0a);  //
		sensor_write(sd, 0x1d, 0x4e);  //
		sensor_write(sd, 0x20, 0x38);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);

		sensor_write(sd, 0x27, 0xad);
		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x40);
		sensor_write(sd, 0x2e, 0x70);
		sensor_write(sd, 0x30, 0x74);
		sensor_write(sd, 0x31, 0x9b);
		sensor_write(sd, 0x32, 0xa5);
		sensor_write(sd, 0x33, 0xe0);
		sensor_write(sd, 0x35, 0x05);
		sensor_write(sd, 0x38, 0x40);
		sensor_write(sd, 0x39, 0x68);
	} else if (QHD30 == fmt) {
		sensor_read(sd, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(sd, 0xf5, tmp);
		sensor_write(sd, 0x02, 0x50);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x03);
		sensor_write(sd, 0x0d, 0x50);

		sensor_write(sd, 0x15, 0x23);
		sensor_write(sd, 0x16, 0x1b);
		sensor_write(sd, 0x17, 0x00);
		sensor_write(sd, 0x18, 0x38);
		sensor_write(sd, 0x19, 0xa0);
		sensor_write(sd, 0x1a, 0x5a);
		sensor_write(sd, 0x1c, 0x0c);  //2560*1440, 30fps
		sensor_write(sd, 0x1d, 0xe2);  //

		sensor_write(sd, 0x20, 0x50);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);

		sensor_write(sd, 0x27, 0xad);
		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x58);
		sensor_write(sd, 0x2e, 0x70);
		sensor_write(sd, 0x30, 0x74);
		sensor_write(sd, 0x31, 0x58);
		sensor_write(sd, 0x32, 0x9f);
		sensor_write(sd, 0x33, 0x60);
		sensor_write(sd, 0x35, 0x15);
		sensor_write(sd, 0x36, 0xdc);
		sensor_write(sd, 0x38, 0x40);
		sensor_write(sd, 0x39, 0x48);

	} else if (QHD25 == fmt) {
		sensor_read(sd, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(sd, 0xf5, tmp);
		sensor_write(sd, 0x02, 0x50);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x03);
		sensor_write(sd, 0x0d, 0x50);

		sensor_write(sd, 0x15, 0x23);
		sensor_write(sd, 0x16, 0x1b);
		sensor_write(sd, 0x17, 0x00);
		sensor_write(sd, 0x18, 0x38);
		sensor_write(sd, 0x19, 0xa0);
		sensor_write(sd, 0x1a, 0x5a);
		sensor_write(sd, 0x1c, 0x0f);  //2560*1440, 25fps
		sensor_write(sd, 0x1d, 0x76);  //

		sensor_write(sd, 0x20, 0x50);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);

		sensor_write(sd, 0x27, 0xad);
		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x58);
		sensor_write(sd, 0x2e, 0x70);

		sensor_write(sd, 0x30, 0x74);
		sensor_write(sd, 0x31, 0x58);
		sensor_write(sd, 0x32, 0x9f);
		sensor_write(sd, 0x33, 0x60);

		sensor_write(sd, 0x35, 0x15);
		sensor_write(sd, 0x36, 0xdc);
		sensor_write(sd, 0x38, 0x40);
		sensor_write(sd, 0x39, 0x48);
	} else if (UVGA25 == fmt) { //960P25
		sensor_write(sd, 0xf5, 0xf0);
		sensor_write(sd, 0x02, 0x42);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x13);
		sensor_write(sd, 0x0d, 0x50);

		sensor_write(sd, 0x15, 0x13);
		sensor_write(sd, 0x16, 0x16);
		sensor_write(sd, 0x17, 0x00);
		sensor_write(sd, 0x18, 0xa0);
		sensor_write(sd, 0x19, 0xc0);
		sensor_write(sd, 0x1a, 0x35);
		sensor_write(sd, 0x1c, 0x07);  //
		sensor_write(sd, 0x1d, 0xbc);  //

		sensor_write(sd, 0x20, 0x30);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);

		sensor_write(sd, 0x26, 0x01);
		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x30);
		sensor_write(sd, 0x2e, 0x70);
		sensor_write(sd, 0x30, 0x48);
		sensor_write(sd, 0x31, 0xba);
		sensor_write(sd, 0x32, 0x2e);
		sensor_write(sd, 0x33, 0x90);
		sensor_write(sd, 0x35, 0x14);
		sensor_write(sd, 0x36, 0x65);
		sensor_write(sd, 0x38, 0x00);
		sensor_write(sd, 0x39, 0x18);
	} else if (UVGA30 == fmt) { //960P30
		sensor_write(sd, 0xf5, 0xf0);
		sensor_write(sd, 0x02, 0x42);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x13);
		sensor_write(sd, 0x0d, 0x50);

		sensor_write(sd, 0x15, 0x13);
		sensor_write(sd, 0x16, 0x16);
		sensor_write(sd, 0x17, 0x00);
		sensor_write(sd, 0x18, 0xa0);
		sensor_write(sd, 0x19, 0xc0);
		sensor_write(sd, 0x1a, 0x35);
		sensor_write(sd, 0x1c, 0x06);  //
		sensor_write(sd, 0x1d, 0x72);  //

		sensor_write(sd, 0x20, 0x30);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);

		sensor_write(sd, 0x26, 0x01);
		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x30);
		sensor_write(sd, 0x2e, 0x70);

		sensor_write(sd, 0x30, 0x43);
		sensor_write(sd, 0x31, 0x3b);
		sensor_write(sd, 0x32, 0x79);
		sensor_write(sd, 0x33, 0x90);

		sensor_write(sd, 0x35, 0x14);
		sensor_write(sd, 0x36, 0x65);
		sensor_write(sd, 0x38, 0x00);
		sensor_write(sd, 0x39, 0x18);
	} else if (HD30HDR == fmt) {
		sensor_write(sd, 0xf5, 0xf0);
		sensor_write(sd, 0x02, 0x42);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x13);
		sensor_write(sd, 0x0d, 0x50);

		sensor_write(sd, 0x15, 0x13);
		sensor_write(sd, 0x16, 0x15);
		sensor_write(sd, 0x17, 0x00);
		sensor_write(sd, 0x18, 0x90);
		sensor_write(sd, 0x19, 0xd0);
		sensor_write(sd, 0x1a, 0x25);
		sensor_write(sd, 0x1c, 0x06);
		sensor_write(sd, 0x1d, 0x72);

		sensor_write(sd, 0x20, 0x30);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);

		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x30);
		sensor_write(sd, 0x2e, 0x70);

		sensor_write(sd, 0x30, 0x48);
		sensor_write(sd, 0x31, 0xba);
		sensor_write(sd, 0x32, 0x2e);
		sensor_write(sd, 0x33, 0x90);

		sensor_write(sd, 0x35, 0x13);
		sensor_write(sd, 0x36, 0xe8);
		sensor_write(sd, 0x38, 0x00);
		sensor_write(sd, 0x39, 0x18);
	} else if (HD50 == fmt) {
		sensor_read(sd, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(sd, 0xf5, tmp);

		sensor_write(sd, 0x02, 0x42);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x13);
		sensor_write(sd, 0x0d, 0x50);

		sensor_write(sd, 0x15, 0x13);
		sensor_write(sd, 0x16, 0x15);
		sensor_write(sd, 0x17, 0x00);
		sensor_write(sd, 0x18, 0x19);
		sensor_write(sd, 0x19, 0xd0);
		sensor_write(sd, 0x1a, 0x25);
		sensor_write(sd, 0x1c, 0x07);  //1280*720,
		sensor_write(sd, 0x1d, 0xbc);  //1280*720, 50fps

		sensor_write(sd, 0x20, 0x30);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);

		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x30);
		sensor_write(sd, 0x2e, 0x70);

		sensor_write(sd, 0x30, 0x48);
		sensor_write(sd, 0x31, 0xbb);
		sensor_write(sd, 0x32, 0x2e);
		sensor_write(sd, 0x33, 0x90);

		sensor_write(sd, 0x35, 0x05);
		sensor_write(sd, 0x38, 0x00);
		sensor_write(sd, 0x39, 0x1c);

		if (STD_HDA == std) { //subcarrier=24M
			sensor_write(sd, 0x02, 0x46);
			sensor_write(sd, 0x05, 0x01);
			sensor_write(sd, 0x0d, 0x76);
			sensor_write(sd, 0x0e, 0x0a);
			sensor_write(sd, 0x14, 0x00);
			sensor_write(sd, 0x15, 0x13);
			sensor_write(sd, 0x16, 0x1a);
			sensor_write(sd, 0x18, 0x1b);

			sensor_write(sd, 0x20, 0x40);

			sensor_write(sd, 0x26, 0x01);

			sensor_write(sd, 0x2c, 0x3a);
			sensor_write(sd, 0x2d, 0x54);
			sensor_write(sd, 0x2e, 0x50);

			sensor_write(sd, 0x30, 0xa5);
			sensor_write(sd, 0x31, 0x9f);
			sensor_write(sd, 0x32, 0xce);
			sensor_write(sd, 0x33, 0x60);
		}
	} else if (HD60 == fmt) {
		sensor_read(sd, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(sd, 0xf5, tmp);

		sensor_write(sd, 0x02, 0x42);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x13);
		sensor_write(sd, 0x0d, 0x50);

		sensor_write(sd, 0x15, 0x13);
		sensor_write(sd, 0x16, 0x15);
		sensor_write(sd, 0x17, 0x00);
		sensor_write(sd, 0x18, 0x19);
		sensor_write(sd, 0x19, 0xd0);
		sensor_write(sd, 0x1a, 0x25);
		sensor_write(sd, 0x1c, 0x06);  //1280*720,
		sensor_write(sd, 0x1d, 0x72);  //1280*720, 60fps

		sensor_write(sd, 0x20, 0x30);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);

		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x30);
		sensor_write(sd, 0x2e, 0x70);

		sensor_write(sd, 0x30, 0x48);
		sensor_write(sd, 0x31, 0xbb);
		sensor_write(sd, 0x32, 0x2e);
		sensor_write(sd, 0x33, 0x90);

		sensor_write(sd, 0x35, 0x05);
		sensor_write(sd, 0x38, 0x00);
		sensor_write(sd, 0x39, 0x1c);

		if (STD_HDA == std) { //subcarrier=11M
			sensor_write(sd, 0x02, 0x46);
			sensor_write(sd, 0x05, 0xf9);
			sensor_write(sd, 0x0d, 0x76);
			sensor_write(sd, 0x0e, 0x03);
			sensor_write(sd, 0x14, 0x00);
			sensor_write(sd, 0x15, 0x13);
			sensor_write(sd, 0x16, 0x41);
			sensor_write(sd, 0x18, 0x1b);
			sensor_write(sd, 0x20, 0x50);
			sensor_write(sd, 0x21, 0x84);

			sensor_write(sd, 0x25, 0xff);
			sensor_write(sd, 0x26, 0x0d);

			sensor_write(sd, 0x2c, 0x3a);
			sensor_write(sd, 0x2d, 0x68);
			sensor_write(sd, 0x2e, 0x60);

			sensor_write(sd, 0x30, 0x4e);
			sensor_write(sd, 0x31, 0xf8);
			sensor_write(sd, 0x32, 0xdc);
			sensor_write(sd, 0x33, 0xf0);
		}
	} else if (A_UVGA30 == fmt) { //HDA 960P30
		sensor_write(sd, 0xf5, 0xf0);
		sensor_write(sd, 0x02, 0x40);
		sensor_write(sd, 0x05, 0x01);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x03);
		sensor_write(sd, 0x0d, 0x76);
		sensor_write(sd, 0x0e, 0x12);

		sensor_write(sd, 0x15, 0x03);
		sensor_write(sd, 0x16, 0x5f);
		sensor_write(sd, 0x17, 0x00);
		sensor_write(sd, 0x18, 0x9c);
		sensor_write(sd, 0x19, 0xc0);
		sensor_write(sd, 0x1a, 0x35);
		sensor_write(sd, 0x1c, 0x85);  //
		sensor_write(sd, 0x1d, 0x78);  //

		sensor_write(sd, 0x20, 0x14);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);

		sensor_write(sd, 0x26, 0x0d);
		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x1e);
		sensor_write(sd, 0x2e, 0x50);

		sensor_write(sd, 0x30, 0x29);
		sensor_write(sd, 0x31, 0x01);
		sensor_write(sd, 0x32, 0x76);
		sensor_write(sd, 0x33, 0x80);

		sensor_write(sd, 0x35, 0x14);
		sensor_write(sd, 0x36, 0x65);
		sensor_write(sd, 0x38, 0x00);
		sensor_write(sd, 0x39, 0x88);
	} else if (F_UVGA30 == fmt) { //FH 960P30
		sensor_write(sd, 0xf5, 0xf0);
		sensor_write(sd, 0x02, 0x4c);
		sensor_write(sd, 0x05, 0xfd);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x03);
		sensor_write(sd, 0x0d, 0x76);
		sensor_write(sd, 0x0e, 0x16);
		sensor_write(sd, 0x15, 0x13);
		sensor_write(sd, 0x16, 0x8f);
		sensor_write(sd, 0x17, 0x00);
		sensor_write(sd, 0x18, 0x23);
		sensor_write(sd, 0x19, 0xc0);
		sensor_write(sd, 0x1a, 0x35);
		sensor_write(sd, 0x1c, 0x07);  //
		sensor_write(sd, 0x1d, 0x08);  //

		sensor_write(sd, 0x20, 0x60);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);

		sensor_write(sd, 0x26, 0x05);
		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x70);
		sensor_write(sd, 0x2e, 0x50);

		sensor_write(sd, 0x30, 0x7f);
		sensor_write(sd, 0x31, 0x49);
		sensor_write(sd, 0x32, 0xf4);
		sensor_write(sd, 0x33, 0x90);
		sensor_write(sd, 0x35, 0x13);
		sensor_write(sd, 0x36, 0xe8);
		sensor_write(sd, 0x38, 0x00);
		sensor_write(sd, 0x39, 0x88);
	} else if (UVGA30_945 == fmt) { //1400x1125x30,94.5M
		sensor_write(sd, 0xf5, 0xf0);
		sensor_write(sd, 0x02, 0x40);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x03);
		sensor_write(sd, 0x0d, 0x50);
		sensor_write(sd, 0x14, 0x00);
		sensor_write(sd, 0x15, 0x03);
		sensor_write(sd, 0x16, 0x60);
		sensor_write(sd, 0x17, 0x00);
		sensor_write(sd, 0x18, 0xa0);
		sensor_write(sd, 0x19, 0xc0);
		sensor_write(sd, 0x1a, 0x35);
		sensor_write(sd, 0x1c, 0x05);  //1920*1080, 30fps
		sensor_write(sd, 0x1d, 0x78);  //
		sensor_write(sd, 0x20, 0x18);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);
		sensor_write(sd, 0x26, 0x06);
		sensor_write(sd, 0x27, 0x2d);
		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x1a);
		sensor_write(sd, 0x2d, 0x1a);
		sensor_write(sd, 0x2e, 0x70);
		sensor_write(sd, 0x30, 0x48);
		sensor_write(sd, 0x31, 0xba);
		sensor_write(sd, 0x32, 0x2e);
		sensor_write(sd, 0x33, 0x90);
		sensor_write(sd, 0x35, 0x14);
		sensor_write(sd, 0x36, 0x65);
		sensor_write(sd, 0x38, 0x00);
		sensor_write(sd, 0x39, 0x88);
	} else if (FMT5M20 == fmt) {
		sensor_read(sd, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(sd, 0xf5, tmp);

		sensor_write(sd, 0x02, 0x50);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x03);
		sensor_write(sd, 0x0d, 0x50);

		sensor_write(sd, 0x15, 0x23);
		sensor_write(sd, 0x16, 0x36);
		sensor_write(sd, 0x17, 0x20);
		sensor_write(sd, 0x18, 0x1a);
		sensor_write(sd, 0x19, 0x98);
		sensor_write(sd, 0x1a, 0x7a);
		sensor_write(sd, 0x1c, 0x0e);  //
		sensor_write(sd, 0x1d, 0xa4);  //
		sensor_write(sd, 0x20, 0x50);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);

		sensor_write(sd, 0x27, 0xad);
		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x54);
		sensor_write(sd, 0x2e, 0x70);

		sensor_write(sd, 0x30, 0x74);
		sensor_write(sd, 0x31, 0xa7);
		sensor_write(sd, 0x32, 0x18);
		sensor_write(sd, 0x33, 0x50);
		sensor_write(sd, 0x35, 0x17);
		sensor_write(sd, 0x36, 0xbc);
		sensor_write(sd, 0x38, 0x40);
		sensor_write(sd, 0x39, 0x48);
		if (STD_HDA == std) {
			sensor_write(sd, 0x0d, 0x70);
			sensor_write(sd, 0x0e, 0x0b);
			sensor_write(sd, 0x1c, 0x8e);
			sensor_write(sd, 0x20, 0x80);
			sensor_write(sd, 0x21, 0x86);
			sensor_write(sd, 0x2d, 0xa0);
			sensor_write(sd, 0x2e, 0x40);
			sensor_write(sd, 0x30, 0x48);
			sensor_write(sd, 0x31, 0x77);
			sensor_write(sd, 0x32, 0x0e);
			sensor_write(sd, 0x33, 0xa0);
			sensor_write(sd, 0x39, 0x68);
		}
	} else if (FMT5M12 == fmt) {
		sensor_read(sd, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(sd, 0xf5, tmp);
		sensor_write(sd, 0x02, 0x40);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x03);
		sensor_write(sd, 0x0d, 0x50);

		sensor_write(sd, 0x15, 0x13);
		sensor_write(sd, 0x16, 0x1f);
		sensor_write(sd, 0x17, 0x20);
		sensor_write(sd, 0x18, 0x34);
		sensor_write(sd, 0x19, 0x98);
		sensor_write(sd, 0x1a, 0x7a);
		sensor_write(sd, 0x1c, 0x0b);  //
		sensor_write(sd, 0x1d, 0x9a);  //
		sensor_write(sd, 0x20, 0x30);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);

		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x30);
		sensor_write(sd, 0x2e, 0x70);

		sensor_write(sd, 0x30, 0x48);
		sensor_write(sd, 0x31, 0xbb);
		sensor_write(sd, 0x32, 0x2e);
		sensor_write(sd, 0x33, 0x90);
		sensor_write(sd, 0x35, 0x17);
		sensor_write(sd, 0x36, 0xd0);
		sensor_write(sd, 0x38, 0x00);
		sensor_write(sd, 0x39, 0x1C);

		if (STD_HDA == std) {
			sensor_write(sd, 0x02, 0x44);
			sensor_write(sd, 0x05, 0x29);
			sensor_write(sd, 0x0d, 0x76);
			sensor_write(sd, 0x0e, 0x0f);
			sensor_write(sd, 0x15, 0x73);
			sensor_write(sd, 0x16, 0x5a);
			sensor_write(sd, 0x17, 0x20);
			sensor_write(sd, 0x18, 0x1a);
			sensor_write(sd, 0x19, 0x98);
			sensor_write(sd, 0x1a, 0x7a);
			sensor_write(sd, 0x1c, 0x0b);
			sensor_write(sd, 0x1d, 0xb8);
			sensor_write(sd, 0x20, 0x38);
			sensor_write(sd, 0x21, 0x46);
			sensor_write(sd, 0x25, 0xfe);
			sensor_write(sd, 0x26, 0x01);
			sensor_write(sd, 0x2c, 0x2a);
			sensor_write(sd, 0x2d, 0x62);
			sensor_write(sd, 0x2e, 0x40);
			sensor_write(sd, 0x30, 0xa5);
			sensor_write(sd, 0x31, 0xa1);
			sensor_write(sd, 0x32, 0xca);
			sensor_write(sd, 0x33, 0xc0);
			sensor_write(sd, 0x35, 0x17);
			sensor_write(sd, 0x36, 0xbc);
		}
	} else if (FMT8M15 == fmt) {
		sensor_read(sd, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(sd, 0xf5, tmp);
		sensor_write(sd, 0x02, 0x50);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x03);
		sensor_write(sd, 0x0d, 0x50);
		sensor_write(sd, 0x15, 0x13);
		sensor_write(sd, 0x16, 0xbd);
		sensor_write(sd, 0x17, 0x00);
		sensor_write(sd, 0x18, 0x50);
		sensor_write(sd, 0x19, 0x70);
		sensor_write(sd, 0x1a, 0x8f);
		sensor_write(sd, 0x1c, 0x11);  //3840*2160, 15fps
		sensor_write(sd, 0x1d, 0x2e);  //
		sensor_write(sd, 0x20, 0x60);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);
		sensor_write(sd, 0x27, 0xad);
		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x54);
		sensor_write(sd, 0x2e, 0x70);
		sensor_write(sd, 0x30, 0x74);
		sensor_write(sd, 0x31, 0x59);
		sensor_write(sd, 0x32, 0xbd);
		sensor_write(sd, 0x33, 0x60);
		sensor_write(sd, 0x35, 0x18);
		sensor_write(sd, 0x36, 0xca);
		sensor_write(sd, 0x38, 0x40);
		sensor_write(sd, 0x39, 0x68);

		if (STD_HDA == std) {
			sensor_write(sd, 0x14, 0x40);
			sensor_write(sd, 0x15, 0x13);
			sensor_write(sd, 0x16, 0x74);
			sensor_write(sd, 0x20, 0x80);
			sensor_write(sd, 0x21, 0x86);
			sensor_write(sd, 0x2d, 0x58);
			sensor_write(sd, 0x2e, 0x40);
			sensor_write(sd, 0x30, 0x48);
			sensor_write(sd, 0x31, 0x68);
			sensor_write(sd, 0x32, 0x43);
			sensor_write(sd, 0x33, 0x00);
		}
	} else if (FMT8M12 == fmt) {
		sensor_read(sd, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(sd, 0xf5, tmp);

		sensor_write(sd, 0x02, 0x50);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x03);
		sensor_write(sd, 0x0d, 0x50);

		sensor_write(sd, 0x15, 0x13);
		sensor_write(sd, 0x16, 0xbd);
		sensor_write(sd, 0x17, 0x00);
		sensor_write(sd, 0x18, 0x50);
		sensor_write(sd, 0x19, 0x70);
		sensor_write(sd, 0x1a, 0x8f);
		sensor_write(sd, 0x1c, 0x14);  //3840*2160, 12fps
		sensor_write(sd, 0x1d, 0x9e);  //

		sensor_write(sd, 0x20, 0x60);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);

		sensor_write(sd, 0x27, 0xad);

		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x02a);
		sensor_write(sd, 0x2d, 0x54);
		sensor_write(sd, 0x2e, 0x70);

		sensor_write(sd, 0x30, 0x74);
		sensor_write(sd, 0x31, 0x59);
		sensor_write(sd, 0x32, 0xbd);
		sensor_write(sd, 0x33, 0x60);

		sensor_write(sd, 0x35, 0x18);
		sensor_write(sd, 0x36, 0xca);
		sensor_write(sd, 0x38, 0x40);
		sensor_write(sd, 0x39, 0x68);
	} else if (FMT8M7 == fmt) {
		sensor_read(sd, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(sd, 0xf5, tmp);

		sensor_write(sd, 0x02, 0x40);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x03);
		sensor_write(sd, 0x0d, 0x50);
		sensor_write(sd, 0x14, 0x40);
		sensor_write(sd, 0x15, 0x03);
		sensor_write(sd, 0x16, 0xc0);
		sensor_write(sd, 0x17, 0x80);
		sensor_write(sd, 0x18, 0x50);
		sensor_write(sd, 0x19, 0x70);
		sensor_write(sd, 0x1a, 0x8f);
		sensor_write(sd, 0x1c, 0x0f);  //3840*2160, 7fps
		sensor_write(sd, 0x1d, 0xa0);  //
		sensor_write(sd, 0x20, 0x30);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);

		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x38);
		sensor_write(sd, 0x2e, 0x70);

		sensor_write(sd, 0x30, 0x24);
		sensor_write(sd, 0x31, 0x34);
		sensor_write(sd, 0x32, 0x21);
		sensor_write(sd, 0x33, 0x80);
		sensor_write(sd, 0x35, 0x19);
		sensor_write(sd, 0x36, 0xab);
		sensor_write(sd, 0x38, 0x00);
		sensor_write(sd, 0x39, 0x1C);
	} else if (HD30864 == fmt) {
		sensor_write(sd, 0xf5, 0xf0);
		sensor_write(sd, 0x02, 0x42);
		sensor_write(sd, 0x07, 0xc0);
		sensor_write(sd, 0x0b, 0xc0);
		sensor_write(sd, 0x0c, 0x13);
		sensor_write(sd, 0x0d, 0x50);

		sensor_write(sd, 0x15, 0x03);
		sensor_write(sd, 0x16, 0xec);
		sensor_write(sd, 0x17, 0x00);
		sensor_write(sd, 0x18, 0xae);
		sensor_write(sd, 0x19, 0xd0);
		sensor_write(sd, 0x1a, 0x25);
		sensor_write(sd, 0x1c, 0x06);
		sensor_write(sd, 0x1d, 0x40);

		sensor_write(sd, 0x20, 0x30);
		sensor_write(sd, 0x21, 0x84);
		sensor_write(sd, 0x22, 0x36);
		sensor_write(sd, 0x23, 0x3c);
		sensor_write(sd, 0x26, 0x05);
		sensor_write(sd, 0x27, 0x2d);

		sensor_write(sd, 0x2b, 0x60);
		sensor_write(sd, 0x2c, 0x2a);
		sensor_write(sd, 0x2d, 0x30);
		sensor_write(sd, 0x2e, 0x70);

		sensor_write(sd, 0x30, 0x48);
		sensor_write(sd, 0x31, 0xba);
		sensor_write(sd, 0x32, 0x2e);
		sensor_write(sd, 0x33, 0x90);

		sensor_write(sd, 0x35, 0x13);
		sensor_write(sd, 0x36, 0x84);
		sensor_write(sd, 0x38, 0x00);
		sensor_write(sd, 0x39, 0x18);
	}
}

void tp2815_mipi_out(struct v4l2_subdev *sd, unsigned char output)
{
	//mipi setting
	sensor_write(sd, 0x40, MIPI_PAGE); //MIPI page
	sensor_write(sd, 0x01, 0xf0);
    sensor_write(sd, 0x02, 0x01);
	sensor_write(sd, 0x08, 0x0f);
    sensor_print("tp2815_mipi_out\n");
	if (MIPI_4CH4LANE_594M == output || MIPI_2CH4LANE_594M == output) {
		sensor_write(sd, 0x20, 0x44);
		if (MIPI_2CH4LANE_594M == output)
			sensor_write(sd, 0x20, 0x24);
		sensor_write(sd, 0x34, 0xe4); //
		sensor_write(sd, 0x15, 0x0C);
		sensor_write(sd, 0x25, 0x08);
		sensor_write(sd, 0x26, 0x06);
		sensor_write(sd, 0x27, 0x11);
		sensor_write(sd, 0x29, 0x0a);
		sensor_write(sd, 0x33, 0x07);
		sensor_write(sd, 0x33, 0x00);
		sensor_write(sd, 0x14, 0x33);
		sensor_write(sd, 0x14, 0xb3);
		sensor_write(sd, 0x14, 0x33);
	}
	if (MIPI_3CH4LANE_594M == output) {
		sensor_write(sd, 0x20, 0x34);
		sensor_write(sd, 0x18, 0x8d); //VC0+VC1+VC
		sensor_write(sd, 0x34, 0x8d); //VIN1+VIN2+VIN3
		sensor_write(sd, 0x15, 0x0C);
		sensor_write(sd, 0x25, 0x08);
		sensor_write(sd, 0x26, 0x06);
		sensor_write(sd, 0x27, 0x11);
		sensor_write(sd, 0x29, 0x0a);
		sensor_write(sd, 0x33, 0x07);
		sensor_write(sd, 0x33, 0x00);
		sensor_write(sd, 0x14, 0x33);
		sensor_write(sd, 0x14, 0xb3);
		sensor_write(sd, 0x14, 0x33);
	} else if (MIPI_4CH4LANE_297M == output || MIPI_2CH4LANE_297M == output) {
		sensor_write(sd, 0x20, 0x44);
		sensor_write(sd, 0x20, 0x44);
		if (MIPI_2CH4LANE_297M == output) {
			sensor_write(sd, 0x20, 0x24);
			sensor_write(sd, 0x18, 0xd8); //VC0+VC1
			sensor_write(sd, 0x34, 0xd8); //VIN0+VIN1
		} else {
			sensor_write(sd, 0x18, 0xe4);
			sensor_write(sd, 0x34, 0xe4);
		}
		sensor_write(sd, 0x14, 0x44);
		sensor_write(sd, 0x15, 0x0d);
		sensor_write(sd, 0x25, 0x04);
		sensor_write(sd, 0x26, 0x03);
		sensor_write(sd, 0x27, 0x09);
		sensor_write(sd, 0x29, 0x02);
		sensor_write(sd, 0x33, 0x07);
		sensor_write(sd, 0x33, 0x00);
		sensor_write(sd, 0x14, 0xc4);
		sensor_write(sd, 0x14, 0x44);
	} else if (MIPI_4CH2LANE_594M == output) {
		sensor_write(sd, 0x20, 0x42);
		sensor_write(sd, 0x34, 0xe4); //output vin1&vin2
		sensor_write(sd, 0x15, 0x0c);
		sensor_write(sd, 0x25, 0x08);
		sensor_write(sd, 0x26, 0x06);
		sensor_write(sd, 0x27, 0x11);
		sensor_write(sd, 0x29, 0x0a);
		sensor_write(sd, 0x33, 0x07);
		sensor_write(sd, 0x33, 0x00);
		sensor_write(sd, 0x14, 0x43);
		sensor_write(sd, 0x14, 0xc3);
		sensor_write(sd, 0x14, 0x43);
	} else if (MIPI_4CH4LANE_445M == output) {//only for 4x960p25/30
		sensor_write(sd, 0x20, 0x44);
		sensor_write(sd, 0x34, 0xe4); //
		sensor_write(sd, 0x12, 0x5f);
		sensor_write(sd, 0x13, 0x07);
		sensor_write(sd, 0x15, 0x0C);
		sensor_write(sd, 0x25, 0x06);
		sensor_write(sd, 0x26, 0x05);
		sensor_write(sd, 0x27, 0x0d);
		sensor_write(sd, 0x29, 0x0a);
		sensor_write(sd, 0x33, 0x07);
		sensor_write(sd, 0x33, 0x00);
		sensor_write(sd, 0x14, 0x33);
		sensor_write(sd, 0x14, 0xb3);
		sensor_write(sd, 0x14, 0x33);
	} else if (MIPI_4CH4LANE_396M == output) {//only for Sony ISX019
		sensor_write(sd, 0x20, 0x44);
		sensor_write(sd, 0x34, 0xe4); //
		sensor_write(sd, 0x12, 0x6a);
		sensor_write(sd, 0x13, 0x27);
		sensor_write(sd, 0x15, 0x0C);
		sensor_write(sd, 0x25, 0x06);
		sensor_write(sd, 0x26, 0x04);
		sensor_write(sd, 0x27, 0x0c);
		sensor_write(sd, 0x29, 0x0a);
		sensor_write(sd, 0x33, 0x07);
		sensor_write(sd, 0x33, 0x00);
		sensor_write(sd, 0x14, 0x33);
		sensor_write(sd, 0x14, 0xb3);
		sensor_write(sd, 0x14, 0x33);
	} else if (MIPI_4CH4LANE_378M == output) {//only for 4xA_UVGA30
		sensor_write(sd, 0x20, 0x44);
		sensor_write(sd, 0x34, 0xe4); //
		sensor_write(sd, 0x12, 0x5a);
		sensor_write(sd, 0x13, 0x07);
		sensor_write(sd, 0x15, 0x0C);
		sensor_write(sd, 0x25, 0x06);
		sensor_write(sd, 0x26, 0x04);
		sensor_write(sd, 0x27, 0x0c);
		sensor_write(sd, 0x29, 0x0a);
		sensor_write(sd, 0x33, 0x07);
		sensor_write(sd, 0x33, 0x00);
		sensor_write(sd, 0x14, 0x33);
		sensor_write(sd, 0x14, 0xb3);
		sensor_write(sd, 0x14, 0x33);
	} else if (MIPI_4CH4LANE_432M == output) {//only for 4xF_UVGA30
		sensor_write(sd, 0x20, 0x44);
		sensor_write(sd, 0x34, 0xe4); //
		sensor_write(sd, 0x12, 0x5e);
		sensor_write(sd, 0x13, 0x07);
		sensor_write(sd, 0x15, 0x0C);
		sensor_write(sd, 0x25, 0x06);
		sensor_write(sd, 0x26, 0x05);
		sensor_write(sd, 0x27, 0x0d);
		sensor_write(sd, 0x29, 0x0a);
		sensor_write(sd, 0x33, 0x07);
		sensor_write(sd, 0x33, 0x00);
		sensor_write(sd, 0x14, 0x33);
		sensor_write(sd, 0x14, 0xb3);
		sensor_write(sd, 0x14, 0x33);
	} else if (MIPI_1CH2LANE_594M == output) {
		sensor_write(sd, 0x20, 0x12);
		sensor_write(sd, 0x34, 0x10); //output vin1&vin2
		sensor_write(sd, 0x15, 0x0c);
		sensor_write(sd, 0x25, 0x08);
		sensor_write(sd, 0x26, 0x06);
		sensor_write(sd, 0x27, 0x11);
		sensor_write(sd, 0x29, 0x0a);
		sensor_write(sd, 0x33, 0x07);
		sensor_write(sd, 0x33, 0x00);
		sensor_write(sd, 0x14, 0x43);
		sensor_write(sd, 0x14, 0xc3);
		sensor_write(sd, 0x14, 0x43);
	} else if (MIPI_2CH2LANE_297M == output) {
		sensor_write(sd, 0x20, 0x22);
		sensor_write(sd, 0x18, 0xd8); // VC0 + VC1
		sensor_write(sd, 0x34, 0xd8); // output vin1 & vin2
		sensor_write(sd, 0x14, 0x54);
		sensor_write(sd, 0x15, 0x0d);
		sensor_write(sd, 0x25, 0x04);
		sensor_write(sd, 0x26, 0x03);
		sensor_write(sd, 0x27, 0x09);
		sensor_write(sd, 0x29, 0x02);
		sensor_write(sd, 0x33, 0x07);
		sensor_write(sd, 0x33, 0x00);
		sensor_write(sd, 0x14, 0xd4);
		sensor_write(sd, 0x14, 0x54);
	} else if (MIPI_2CH2LANE_594M == output) {
		sensor_write(sd, 0x20, 0x22);
		sensor_write(sd, 0x18, 0xd8); // VC0 + VC1
		sensor_write(sd, 0x34, 0xd8); // output vin1 & vin2
		sensor_write(sd, 0x15, 0x0c);
		sensor_write(sd, 0x25, 0x08);
		sensor_write(sd, 0x26, 0x06);
		sensor_write(sd, 0x27, 0x11);
		sensor_write(sd, 0x29, 0x0a);
		sensor_write(sd, 0x33, 0x07);
		sensor_write(sd, 0x33, 0x00);
		sensor_write(sd, 0x14, 0x43);
		sensor_write(sd, 0x14, 0xc3);
		sensor_write(sd, 0x14, 0x43);
	}

	if (MIPI_4CH4LANE_345M == output) {
		sensor_write(sd, 0x20, 0x44);
		sensor_write(sd, 0x34, 0xe4);
		sensor_write(sd, 0x12, 0x7e);
		sensor_write(sd, 0x13, 0x67);
		sensor_write(sd, 0x15, 0x0C);
		sensor_write(sd, 0x25, 0x06);
		sensor_write(sd, 0x26, 0x04);
		sensor_write(sd, 0x27, 0x11);
		sensor_write(sd, 0x29, 0x0e);
		sensor_write(sd, 0x33, 0x07);
		sensor_write(sd, 0x33, 0x00);
		sensor_write(sd, 0x14, 0x33);
		sensor_write(sd, 0x14, 0xb3);
		sensor_write(sd, 0x14, 0x33);
	}
	/* Enable MIPI CSI2 output */
	sensor_write(sd, 0x23, 0x02);
	sensor_write(sd, 0x23, 0x00);
	sensor_write(sd, 0x40, 0x04); //Decoder page
}

static int tp2815_audio_config_rmpos(struct v4l2_subdev *sd, unsigned chip, unsigned format, unsigned chn_num)
{
	int i = 0;

	//clear first
	for (i = 0; i < 20; i++) {
		sensor_write(sd, i, 0);
	}

	switch (chn_num) {
	case 2:
		if (format) {
			sensor_write(sd, 0x0, 1);
			sensor_write(sd, 0x1, 2);
		} else {
			sensor_write(sd, 0x0, 1);
			sensor_write(sd, 0x8, 2);
		}

		break;
	case 4:
		if (format) {
			sensor_write(sd, 0x0, 1);
			sensor_write(sd, 0x1, 2);
			sensor_write(sd, 0x2, 3);
			sensor_write(sd, 0x3, 4);/**/
		} else {
			sensor_write(sd, 0x0, 1);
			sensor_write(sd, 0x1, 2);
			sensor_write(sd, 0x8, 3);
			sensor_write(sd, 0x9, 4);/**/
		}
		break;
	case 8:
		if (0 == chip % 4) {
			if (format) {
				sensor_write(sd, 0x0, 1);
				sensor_write(sd, 0x1, 2);
				sensor_write(sd, 0x2, 3);
				sensor_write(sd, 0x3, 4);/**/
				sensor_write(sd, 0x4, 5);
				sensor_write(sd, 0x5, 6);
				sensor_write(sd, 0x6, 7);
				sensor_write(sd, 0x7, 8);/**/
			} else {
				sensor_write(sd, 0x0, 1);
				sensor_write(sd, 0x1, 2);
				sensor_write(sd, 0x2, 3);
				sensor_write(sd, 0x3, 4);/**/
				sensor_write(sd, 0x8, 5);
				sensor_write(sd, 0x9, 6);
				sensor_write(sd, 0xa, 7);
				sensor_write(sd, 0xb, 8);/**/
			}
		} else if (1 == chip % 4) {
			if (format) {
				sensor_write(sd, 0x0, 0);
				sensor_write(sd, 0x1, 0);
				sensor_write(sd, 0x2, 0);
				sensor_write(sd, 0x3, 0);
				sensor_write(sd, 0x4, 1);
				sensor_write(sd, 0x5, 2);
				sensor_write(sd, 0x6, 3);
				sensor_write(sd, 0x7, 4);/**/
			} else {
				sensor_write(sd, 0x0, 0);
				sensor_write(sd, 0x1, 0);
				sensor_write(sd, 0x2, 1);
				sensor_write(sd, 0x3, 2);
				sensor_write(sd, 0x8, 0);
				sensor_write(sd, 0x9, 0);
				sensor_write(sd, 0xa, 3);
				sensor_write(sd, 0xb, 4);/**/
			}
		}
		break;

	case 16:
		if (0 == chip % 4) {
			for (i = 0; i < 16; i++) {
				sensor_write(sd, i, i + 1);
			}
		} else if (1 == chip % 4) {
			for (i = 4; i < 16; i++) {
				sensor_write(sd, i, i + 1 -4);
			}
		} else if (2 == chip % 4) {
			for (i = 8; i < 16; i++) {
				sensor_write(sd, i, i + 1 - 8);
			}
		} else {
			for (i = 12; i < 16; i++) {
				sensor_write(sd, i, i + 1 - 12);
			}
		}
		break;

	case 20:
		for (i = 0; i < 20; i++) {
			sensor_write(sd, i, i + 1);
		}
		break;

	default:
		for (i = 0; i < 20; i++) {
			sensor_write(sd, i, i + 1);
		}
		break;
	}

	mdelay(10);
	return 0;
}

__maybe_unused static void tp2815_audio_dataSet(struct v4l2_subdev *sd, unsigned char chip)
{
	data_type tmp;

	sensor_read(sd, 0x40, &tmp);
	sensor_write(sd, 0x40, 0x40);

	tp2815_audio_config_rmpos(sd, chip, AUDIO_FORMAT, AUDIO_CHN);

	sensor_write(sd, 0x17, 0x00|(DATA_BIT<<2));
	sensor_write(sd, 0x1B, 0x01|(DATA_BIT<<6));

#if (AUDIO_CHN == 20)
	sensor_write(sd, 0x18, 0xd0|(SAMPLE_RATE));
#else
	sensor_write(sd, 0x18, 0xc0|(SAMPLE_RATE));
#endif

#if (AUDIO_CHN >= 8)
	sensor_write(sd, 0x19, 0x1F);
#else
	sensor_write(sd, 0x19, 0x0F);
#endif
	sensor_write(sd, 0x1A, 0x15);
	sensor_write(sd, 0x37, 0x20);
	sensor_write(sd, 0x38, 0x38);
	sensor_write(sd, 0x3E, 0x00);
	sensor_write(sd, 0x7a, 0x25);
	sensor_write(sd, 0x3d, 0x01);//audio reset
	sensor_write(sd, 0x40, tmp);
}

int  tp2815_hardware_init(struct v4l2_subdev *sd, unsigned char fmt)
{
	int ret = -1;
	sensor_print("tp2815_hardware_init.\n");

	/* Disable MIPI CSI2 output */
	ret = sensor_write(sd, 0x40, MIPI_PAGE);
	if (ret != 0) {
		sensor_err("Can't access tp2854.\n");
		return -ENODEV;
	}
	sensor_write(sd, 0x23, 0x02);

	tp2815_decoder_init(sd, CH_1, fmt, STD_HDA);
	tp2815_decoder_init(sd, CH_2, fmt, STD_HDA);

	if (!strcmp(sd->name, SENSOR_NAME_2)) {
		tp2815_mipi_out(sd, MIPI_2CH2LANE_594M);
	} else {
		tp2815_decoder_init(sd, CH_3, fmt, STD_HDA);
		tp2815_decoder_init(sd, CH_4, fmt, STD_HDA);
		tp2815_mipi_out(sd, MIPI_4CH4LANE_594M);
	}

	sensor_write(sd, 0x40, 0x0);
#if Print_reg
	int k;
	data_type tmp;
	for (k = 0; k < 255; ++k) {
		sensor_read(sd, k, &tmp);
		sensor_print("rx_page 0x%x val=0x%x\n", k, tmp);
	}
#endif
	sensor_write(sd, 0x40, MIPI_PAGE);

	sensor_write(sd, 0x23, 0x00);

#if Print_reg
	for (k = 0; k < 255; ++k) {

		sensor_read(sd, k, &tmp);
		sensor_print("mipi_page 0x%x val=0x%x\n", k, tmp);
	}
#endif
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
	info->width = 1920;
	info->height = 1080;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->tpf.numerator = 1;
	info->tpf.denominator = 25;

	return 0;
}
#if Test_flag

#define TVD_CH_MAX 4

int tp2815_init_ch_hardware(struct v4l2_subdev *sd,
	struct tvin_init_info *info)
{
	int fmt;
	int ch;

	ch = info->ch_id;
	fmt = info->input_fmt[ch];

	if (ch >= TVD_CH_MAX)
		return -1;

	switch (fmt) {
	case CVBS_NTSC:
		tp2815_decoder_init(sd, ch, NTSC, STD_HDA);
		break;
	case CVBS_PAL:
		tp2815_decoder_init(sd, ch, PAL, STD_HDA);
		break;
	case AHD720P25:
		tp2815_decoder_init(sd, ch, HD25, STD_HDA);
		break;
	case AHD720P30:
		tp2815_decoder_init(sd, ch, HD30, STD_HDA);
		break;
	case AHD1080P25:
		tp2815_decoder_init(sd, ch, FHD25, STD_HDA);
		break;
	case AHD1080P30:
		tp2815_decoder_init(sd, ch, FHD30, STD_HDA);
		break;
	default:
		return -1;
	}

	sensor_print("Init ch successful!\r\n");
	return 0;
}

static int tp2815_get_output_fmt(struct v4l2_subdev *sd,
		struct sensor_output_fmt *fmt)
{
	struct sensor_info *info = to_state(sd);
	__u32 *sensor_fmt = info->tvin.tvin_info.input_fmt;
	__u32 *out_feld = &fmt->field;
	__u32 ch_id = fmt->ch_id;

	if (ch_id >= TVD_CH_MAX)
		return -EINVAL;

	switch (sensor_fmt[ch_id]) {
	case CVBS_PAL:
	case CVBS_NTSC:
		/*Interlace ouput set out_feld as 1*/
		*out_feld = 1;
		break;
	case AHD720P25:
	case AHD720P30:
	case AHD1080P25:
	case AHD1080P30:
		/*Progressive ouput set out_feld as 0*/
		*out_feld = 0;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void tp2815_set_input_size(struct sensor_info *info,
		struct v4l2_subdev_format *fmt, int ch_id)
{
	struct tvin_init_info *tvin_info = &info->tvin.tvin_info;

	switch (tvin_info->input_fmt[ch_id]) {
	case AHD720P25:
	case AHD720P30:
		fmt->format.width = 1280;
		fmt->format.height = 720;
		break;
	case AHD1080P25:
	case AHD1080P30:
		fmt->format.width = 1920;
		fmt->format.height = 1080;
		break;
	case CVBS_PAL:
		fmt->format.width = 720;
		fmt->format.height = 576;
		break;
	case CVBS_NTSC:
		fmt->format.width = 720;
		fmt->format.height = 480;
		break;
	default:
		break;
	}
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
int tp2815_sensor_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_state *state,
			struct v4l2_subdev_format *fmt)
{
	struct sensor_info *info = to_state(sd);
	int ret = 0;

	if (fmt->format.width == 1440 || fmt->format.width == 720)  // NTSC/PAL
		info->sensor_field = V4L2_FIELD_INTERLACED;
	else
		info->sensor_field = V4L2_FIELD_NONE;

	if (!info->tvin.flag)
		return sensor_set_fmt(sd, state, fmt);

	if (sd->entity.stream_count == 0) {
		tp2815_set_input_size(info, fmt, fmt->reserved[0]);
		ret = sensor_set_fmt(sd, state, fmt);
		sensor_print("%s befor ch%d %d*%d \n", __func__,
			fmt->reserved[0], fmt->format.width, fmt->format.height);
	} else {
		ret = sensor_set_fmt(sd, state, fmt);
		tp2815_set_input_size(info, fmt, fmt->reserved[0]);
		sensor_print("%s after ch%d %d*%d \n", __func__,
			fmt->reserved[0], fmt->format.width, fmt->format.height);
	}

	return ret;
}
#else
int tp2815_sensor_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt)
{
	struct sensor_info *info = to_state(sd);
	int ret = 0;

	if (fmt->format.width == 1440 || fmt->format.width == 720)  // NTSC/PAL
		info->sensor_field = V4L2_FIELD_INTERLACED;
	else
		info->sensor_field = V4L2_FIELD_NONE;

	if (!info->tvin.flag)
		return sensor_set_fmt(sd, cfg, fmt);

	if (sd->entity.stream_count == 0) {
		tp2815_set_input_size(info, fmt, fmt->reserved[0]);
		ret = sensor_set_fmt(sd, cfg, fmt);
		sensor_print("%s befor ch%d %d*%d \n", __func__,
			fmt->reserved[0], fmt->format.width, fmt->format.height);
	} else {
		ret = sensor_set_fmt(sd, cfg, fmt);
		tp2815_set_input_size(info, fmt, fmt->reserved[0]);
		sensor_print("%s after ch%d %d*%d \n", __func__,
			fmt->reserved[0], fmt->format.width, fmt->format.height);
	}

	return ret;
}
#endif

static int sensor_tvin_init(struct v4l2_subdev *sd, struct tvin_init_info *tvin_info)
{
	struct sensor_info *info = to_state(sd);
	__u32 *sensor_fmt = info->tvin.tvin_info.input_fmt;
	__u32 ch_id = tvin_info->ch_id;

	sensor_print("set ch%d fmt as %d\n", ch_id, tvin_info->input_fmt[ch_id]);
	sensor_fmt[ch_id] = tvin_info->input_fmt[ch_id];
	info->tvin.tvin_info.ch_id = ch_id;

	if (sd->entity.stream_count != 0) {
		tp2815_init_ch_hardware(sd, &info->tvin.tvin_info);
		sensor_print("sensor_tvin_init tp2815_init_ch_hardware\n");
	}

	info->tvin.flag = true;

	return 0;
}
#endif
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
#if Test_flag
	case SENSOR_TVIN_INIT:
		ret = sensor_tvin_init(sd, (struct tvin_init_info *)arg);
		break;
	case GET_SENSOR_CH_OUTPUT_FMT:
		ret = tp2815_get_output_fmt(sd, (struct sensor_output_fmt *)arg);
		break;
#endif
	default:
		return -EINVAL;
	}
	return ret;
}

static struct sensor_format_struct sensor_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.mbus_code	= MEDIA_BUS_FMT_UYVY8_2X8,  //  MEDIA_BUS_FMT_UYVY8_2X8
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
	  .width = 720,
	  .height = 480,
	  .hoffset = 0,
	  .voffset = 0,
	  .pclk = 150*1000*1000,
	  .mipi_bps = 600*1000*1000,
	  .fps_fixed = 25,
	  .regs = sensor_default_regs,
	  .regs_size = ARRAY_SIZE(sensor_default_regs),
	  .set_size = NULL,
	},
	{
	  .width = 720,
	  .height = 576,
	  .hoffset = 0,
	  .voffset = 0,
	  .pclk = 150*1000*1000,
	  .mipi_bps = 600*1000*1000,
	  .fps_fixed = 25,
	  .regs = sensor_default_regs,
	  .regs_size = ARRAY_SIZE(sensor_default_regs),
	  .set_size = NULL,
	},
	{
	  .width = 1280,
	  .height = 720,
	  .hoffset = 0,
	  .voffset = 0,
	  .pclk = 150*1000*1000,
	  .mipi_bps = 600*1000*1000,
	  .fps_fixed = 25,
	  .regs = sensor_default_regs,
	  .regs_size = ARRAY_SIZE(sensor_default_regs),
	  .set_size = NULL,
	},
	{
	  .width = 1280,
	  .height = 720,
	  .hoffset = 0,
	  .voffset = 0,
	  .pclk = 150*1000*1000,
	  .mipi_bps = 600*1000*1000,
	  .fps_fixed = 30,
	  .regs = sensor_default_regs,
	  .regs_size = ARRAY_SIZE(sensor_default_regs),
	  .set_size = NULL,
	},
	{
	 .width = 1920,
	 .height = 1080,
	 .hoffset = 0,
	 .voffset = 0,
	 .pclk = 594 * 1000 * 1000,
	 .mipi_bps = 1188 * 1000 * 1000,
	 .fps_fixed = 25,
	 .regs = sensor_1080p_25fps_regs,
	 .regs_size = ARRAY_SIZE(sensor_1080p_25fps_regs),
	 .set_size = NULL,
	},
	{
	 .width = 1920,
	 .height = 1080,
	 .hoffset = 0,
	 .voffset = 0,
	 .pclk = 594 * 1000 * 1000,
	 .mipi_bps = 1188 * 1000 * 1000,
	 .fps_fixed = 30,
	 .regs = sensor_1080p_25fps_regs,
	 .regs_size = ARRAY_SIZE(sensor_1080p_25fps_regs),
	 .set_size = NULL,
	},
};
#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))
static int sensor_g_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_CSI2_DPHY;
	if (!strcmp(sd->name, SENSOR_NAME_2)) {
		cfg->flags = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0 | V4L2_MBUS_CSI2_CHANNEL_1;
	} else {
		cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | \
			V4L2_MBUS_CSI2_CHANNEL_0 | V4L2_MBUS_CSI2_CHANNEL_1 | \
			V4L2_MBUS_CSI2_CHANNEL_2 | V4L2_MBUS_CSI2_CHANNEL_3;
	}
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
	sensor_dbg("s_fmt set width = %d, height = %d, fps = %d\n", wsize->width,
		      wsize->height, wsize->fps_fixed);

	sensor_dbg("sensor_reg_init\n");
	if (info->width == 1920 && info->height == 1080) {
		if (wsize->fps_fixed == 25) {
			ret = tp2815_hardware_init(sd, FHD25);
				if (ret != 0)
					return -ENXIO;
		} else {
			ret = tp2815_hardware_init(sd, FHD30);
				if (ret != 0)
					return -ENXIO;
		}
	} else if (info->width == 1280 && info->height == 720) {
		if (wsize->fps_fixed == 25) {
			ret = tp2815_hardware_init(sd, HD25);
				if (ret != 0)
					return -ENXIO;
		} else {
			ret = tp2815_hardware_init(sd, HD30);
				if (ret != 0)
					return -ENXIO;
		}
	} else if (info->width == 720 && info->height == 576) {
		ret = tp2815_hardware_init(sd, PAL);
			if (ret != 0)
				return -ENXIO;
	} else {
		ret = tp2815_hardware_init(sd, NTSC);
			if (ret != 0)
				return -ENXIO;
	}

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
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl32 = sensor_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_stream = sensor_s_stream,
};

#if Test_flag
static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.enum_frame_interval = sensor_enum_frame_interval,
	.get_fmt = sensor_get_fmt,
	.set_fmt = tp2815_sensor_set_fmt,
	.get_mbus_config = sensor_g_mbus_config,
};
#else
static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.enum_frame_interval = sensor_enum_frame_interval,
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
	.get_mbus_config = sensor_g_mbus_config,
};
#endif

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.pad = &sensor_pad_ops,
};


/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv[] = {
	{
		.name = SENSOR_NAME,
		.addr_width = CCI_BITS_8,
		.data_width = CCI_BITS_8,
	}, {
		.name = SENSOR_NAME_2,
		.addr_width = CCI_BITS_8,
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
	restart = 0;

	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET2 | MIPI_NORMAL_MODE;
	info->time_hs = 0x20;
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
	}
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
