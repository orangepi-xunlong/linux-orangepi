/*
 * A V4L2 driver for ov8858_4lane Raw cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *	Yang Feng <yangfeng@allwinnertech.com>
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

MODULE_AUTHOR("lwj");
MODULE_DESCRIPTION("A low-level driver for OV8858 sensors");
MODULE_LICENSE("GPL");

#define MCLK			  (24*1000*1000)
#define V4L2_IDENT_SENSOR 0x8858
int ov8858_sensor_vts;

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The ov8858 sits on i2c with ID 0x6c
 */
#define I2C_ADDR 0x6c
#define SENSOR_NAME "ov8858_r2a_4lane"

struct cfg_array {		/* coming later */
	struct regval_list *regs;
	int size;
};



/*
 * The default register settings
 *
 */

static struct regval_list sensor_default_regs[] = {
	//4lane initial
	{0x0103, 0x01},   // ; software reset
	{0x0100, 0x00},   // ; software standby
	{0x0302, 0x1e},   // ; pll1_multi
	{0x0303, 0x00},   // ; pll1_divm
	{0x0304, 0x03},   // ; pll1_div_mipi
	{0x030e, 0x00},   // ; pll2_rdiv
	{0x030f, 0x04},   // ; pll2_divsp
	{0x0312, 0x01},   // ; pll2_pre_div0, pll2_r_divdac
	{0x031e, 0x0c},   // ; pll1_no_lat
	{0x3600, 0x00},   //
	{0x3601, 0x00},   //
	{0x3602, 0x00},   //
	{0x3603, 0x00},   //
	{0x3604, 0x22},   //
	{0x3605, 0x20},   //
	{0x3606, 0x00},   //
	{0x3607, 0x20},   //
	{0x3608, 0x11},   //
	{0x3609, 0x28},   //
	{0x360a, 0x00},   //
	{0x360b, 0x05},   //
	{0x360c, 0xd4},   //
	{0x360d, 0x40},   //
	{0x360e, 0x0c},   //
	{0x360f, 0x20},   //
	{0x3610, 0x07},   //
	{0x3611, 0x20},   //
	{0x3612, 0x88},   //
	{0x3613, 0x80},   //
	{0x3614, 0x58},   //
	{0x3615, 0x00},   //
	{0x3616, 0x4a},   //
	{0x3617, 0x90},   //
	{0x3618, 0x5a},   //
	{0x3619, 0x70},   //
	{0x361a, 0x99},   //
	{0x361b, 0x0a},   //
	{0x361c, 0x07},   //
	{0x361d, 0x00},   //
	{0x361e, 0x00},   //
	{0x361f, 0x00},   //
	{0x3638, 0xff},   //
	{0x3633, 0x0f},   //
	{0x3634, 0x0f},   //
	{0x3635, 0x0f},   //
	{0x3636, 0x12},   //
	{0x3645, 0x13},   //
	{0x3646, 0x83},   //
	{0x364a, 0x07},   //
	{0x3015, 0x01},   // ;
	{0x3018, 0x72},   // ; MIPI 4 lane
	{0x3020, 0x93},   // ; Clock switch output normal, pclk_div =/1
	{0x3022, 0x01},   // ; pd_mipi enable when rst_sync
	{0x3031, 0x0a},   // ; MIPI 10-bit mode
	{0x3034, 0x00},   //
	{0x3106, 0x01},   // ; sclk_div, sclk_pre_div
	{0x3305, 0xf1},   //
	{0x3308, 0x00},   //
	{0x3309, 0x28},   //
	{0x330a, 0x00},   //
	{0x330b, 0x20},   //
	{0x330c, 0x00},   //
	{0x330d, 0x00},   //
	{0x330e, 0x00},   //
	{0x330f, 0x40},   //
	{0x3307, 0x04},   //
	{0x3500, 0x00},   // ; exposure H
	{0x3501, 0x4d},   // ; exposure M
	{0x3502, 0x40},   // ; exposure L
	{0x3503, 0x70},   // ; gain delay 1 frame, exposure delay 1 frame, sensor gain
	{0x3505, 0x80},   // ; gain option
	{0x3508, 0x04},   // ; gain H
	{0x3509, 0x00},   // ; gain L
	{0x350c, 0x00},   // ; short gain H
	{0x350d, 0x80},   // ; short gain L
	{0x3510, 0x00},   // ; short exposure H
	{0x3511, 0x02},   // ; short exposure M
	{0x3512, 0x00},   // ; short exposure L
	{0x3700, 0x30},   //
	{0x3701, 0x18},   //
	{0x3702, 0x50},   //
	{0x3703, 0x32},   //
	{0x3704, 0x28},   //
	{0x3705, 0x00},   //
	{0x3706, 0x82},   //
	{0x3707, 0x08},   //
	{0x3708, 0x48},   //
	{0x3709, 0x66},   //
	{0x370a, 0x01},   //
	{0x370b, 0x82},   //
	{0x370c, 0x07},   //
	{0x3718, 0x14},   //
	{0x3719, 0x31},   //
	{0x3712, 0x44},   //
	{0x3714, 0x24},   //
	{0x371e, 0x31},   //
	{0x371f, 0x7f},   //
	{0x3720, 0x0a},   //
	{0x3721, 0x0a},   //
	{0x3724, 0x0c},   //
	{0x3725, 0x02},   //
	{0x3726, 0x0c},   //
	{0x3728, 0x0a},   //
	{0x3729, 0x03},   //
	{0x372a, 0x06},   //
	{0x372b, 0xa6},   //
	{0x372c, 0xa6},   //
	{0x372d, 0xa6},   //
	{0x372e, 0x0c},   //
	{0x372f, 0x20},   //
	{0x3730, 0x02},   //
	{0x3731, 0x0c},   //
	{0x3732, 0x28},   //
	{0x3733, 0x10},   //
	{0x3734, 0x40},   //
	{0x3736, 0x30},   //
	{0x373a, 0x0a},   //
	{0x373b, 0x0b},   //
	{0x373c, 0x14},   //
	{0x373e, 0x06},   //
	{0x3750, 0x0a},   //
	{0x3751, 0x0e},   //
	{0x3755, 0x10},   //
	{0x3758, 0x00},   //
	{0x3759, 0x4c},   //
	{0x375a, 0x0c},   //
	{0x375b, 0x26},   //
	{0x375c, 0x20},   //
	{0x375d, 0x04},   //
	{0x375e, 0x00},   //
	{0x375f, 0x28},   //
	{0x3768, 0x22},   //
	{0x3769, 0x44},   //
	{0x376a, 0x44},   //
	{0x3761, 0x00},   //
	{0x3762, 0x00},   //
	{0x3763, 0x00},   //
	{0x3766, 0xff},   //
	{0x376b, 0x00},   //
	{0x3772, 0x46},   //
	{0x3773, 0x04},   //
	{0x3774, 0x2c},   //
	{0x3775, 0x13},   //
	{0x3776, 0x08},   //
	{0x3777, 0x00},   //
	{0x3778, 0x17},   //
	{0x37a0, 0x88},   //
	{0x37a1, 0x7a},   //
	{0x37a2, 0x7a},   //
	{0x37a3, 0x00},   //
	{0x37a4, 0x00},   //
	{0x37a5, 0x00},   //
	{0x37a6, 0x00},   //
	{0x37a7, 0x88},   //
	{0x37a8, 0x98},   //
	{0x37a9, 0x98},   //
	{0x3760, 0x00},   //
	{0x376f, 0x01},   //
	{0x37aa, 0x88},   //
	{0x37ab, 0x5c},   //
	{0x37ac, 0x5c},   //
	{0x37ad, 0x55},   //
	{0x37ae, 0x19},   //
	{0x37af, 0x19},   //
	{0x37b0, 0x00},   //
	{0x37b1, 0x00},   //
	{0x37b2, 0x00},   //
	{0x37b3, 0x84},   //
	{0x37b4, 0x84},   //
	{0x37b5, 0x60},   //
	{0x37b6, 0x00},   //
	{0x37b7, 0x00},   //
	{0x37b8, 0x00},   //
	{0x37b9, 0xff},   //
	{0x3800, 0x00},   // ; x start H
	{0x3801, 0x0c},   // ; x start L
	{0x3802, 0x00},   // ; y start H
	{0x3803, 0x0c},   // ; y start L
	{0x3804, 0x0c},   // ; x end H
	{0x3805, 0xd3},   // ; x end L
	{0x3806, 0x09},   // ; y end H
	{0x3807, 0xa3},   // ; y end L
	{0x3808, 0x06},   // ; x output size H
	{0x3809, 0x60},   // ; x output size L
	{0x380a, 0x04},   // ; y output size H
	{0x380b, 0xc8},   // ; y output size L
	{0x380c, 0x07},   // ; HTS H
	{0x380d, 0x88},   // ; HTS L
	{0x380e, 0x04},   // ; VTS H
	{0x380f, 0xdc},   // ; VTS L
	{0x3810, 0x00},   // ; ISP x win H
	{0x3811, 0x04},   // ; ISP x win L
	{0x3813, 0x02},   // ; ISP y win L
	{0x3814, 0x03},   // ; x odd inc
	{0x3815, 0x01},   // ; x even inc
	{0x3820, 0x00},   // ; vflip off
	{0x3821, 0x67},   // ; mirror on, bin on
	{0x382a, 0x03},   // ; y odd inc
	{0x382b, 0x01},   // ; y even inc
	{0x3830, 0x08},   //
	{0x3836, 0x02},   //
	{0x3837, 0x18},   //
	{0x3841, 0xff},   // ; window auto size enable
	{0x3846, 0x48},   //
	{0x3d85, 0x16},   // ; OTP power up load data enable, OTP powerr up load setting enable
	{0x3d8c, 0x73},   // ; OTP setting start High
	{0x3d8d, 0xde},   // ; OTP setting start Low
	{0x3f08, 0x10},   //
	{0x3f0a, 0x00},   //
	{0x4000, 0xf1},   // ; out_range_trig, format_chg_trig, gain_trig, exp_chg_trig, median filter enable
	{0x4001, 0x10},   // ; total 128 black column
	{0x4005, 0x10},   // ; BLC target L
	{0x4002, 0x27},   // ; value used to limit BLC offset
	{0x4009, 0x81},   // ; final BLC offset limitation enable
	{0x400b, 0x0c},   // ; DCBLC on, DCBLC manual mode on
	{0x401b, 0x00},   // ; zero line R coefficient
	{0x401d, 0x00},   // ; zoro line T coefficient
	{0x4020, 0x00},   // ; Anchor left start H
	{0x4021, 0x04},   // ; Anchor left start L
	{0x4022, 0x06},   // ; Anchor left end H
	{0x4023, 0x00},   // ; Anchor left end L
	{0x4024, 0x0f},   // ; Anchor right start H
	{0x4025, 0x2a},   // ; Anchor right start L
	{0x4026, 0x0f},   // ; Anchor right end H
	{0x4027, 0x2b},   // ; Anchor right end L
	{0x4028, 0x00},   // ; top zero line start
	{0x4029, 0x02},   // ; top zero line number
	{0x402a, 0x04},   // ; top black line start
	{0x402b, 0x04},   // ; top black line number
	{0x402c, 0x00},   // ; bottom zero line start
	{0x402d, 0x02},   // ; bottom zoro line number
	{0x402e, 0x04},   // ; bottom black line start
	{0x402f, 0x04},   // ; bottom black line number
	{0x401f, 0x00},   // ; interpolation x disable, interpolation y disable, Anchor one disable
	{0x4034, 0x3f},   //
	{0x403d, 0x04},   // ; md_precison_en
	{0x4300, 0xff},   // ; clip max H
	{0x4301, 0x00},   // ; clip min H
	{0x4302, 0x0f},   // ; clip min L, clip max L
	{0x4316, 0x00},   //
	{0x4500, 0x58},   //
	{0x4503, 0x18},   //
	{0x4600, 0x00},   //
	{0x4601, 0xcb},   //
	{0x481f, 0x32},   // ; clk prepare min
	{0x4837, 0x16},   // ; global timing
	{0x4850, 0x10},   // ; lane 1 = 1, lane 0 = 0
	{0x4851, 0x32},   // ; lane 3 = 3, lane 2 = 2
	{0x4b00, 0x2a},   //
	{0x4b0d, 0x00},   //
	{0x4d00, 0x04},   // ; temperature sensor
	{0x4d01, 0x18},   // ;
	{0x4d02, 0xc3},   // ;
	{0x4d03, 0xff},   // ;
	{0x4d04, 0xff},   // ;
	{0x4d05, 0xff},   // ; temperature sensor
	{0x5000, 0xfe},   // ; lenc on, slave AWB gain enable, slave AWB statistics enable, master AWB gain enable, master AWB statistics enable, BPC on, WPC on
	{0x5001, 0x01},   // ; BLC on
	{0x5002, 0x08},   // ; H scale off, WBMATCH select slave sensor's gain, WBMATCH off, OTP_DPC off,
	{0x5003, 0x20},   // ; DPC_DBC buffer control enable, WB
	{0x5046, 0x12},   //
	{0x5780, 0x3e},   // ; DPC
	{0x5781, 0x0f},   // ;
	{0x5782, 0x44},   // ;
	{0x5783, 0x02},   // ;
	{0x5784, 0x01},   // ;
	{0x5785, 0x00},   // ;
	{0x5786, 0x00},   // ;
	{0x5787, 0x04},   // ;
	{0x5788, 0x02},   // ;
	{0x5789, 0x0f},   // ;
	{0x578a, 0xfd},   // ;
	{0x578b, 0xf5},   // ;
	{0x578c, 0xf5},   // ;
	{0x578d, 0x03},   // ;
	{0x578e, 0x08},   // ;
	{0x578f, 0x0c},   // ;
	{0x5790, 0x08},   // ;
	{0x5791, 0x04},   // ;
	{0x5792, 0x00},   // ;
	{0x5793, 0x52},   // ;
	{0x5794, 0xa3},   // ; DPC
	{0x5871, 0x0d},   // ; Lenc
	{0x5870, 0x18},   // ;
	{0x586e, 0x10},   // ;
	{0x586f, 0x08},   // ;
	{0x58f7, 0x01},   // ;
	{0x58f8, 0x3d},   // ; Lenc
	{0x5901, 0x00},   // ; H skip off, V skip off
	{0x5b00, 0x02},   // ; OTP DPC start address
	{0x5b01, 0x10},   // ; OTP DPC start address
	{0x5b02, 0x03},   // ; OTP DPC end address
	{0x5b03, 0xcf},   // ; OTP DPC end address
	{0x5b05, 0x6c},   // ; recover method = 2b11, use fixed pattern to recover cluster, use 0x3ff to recover cluster
	{0x5e00, 0x00},   // ; test pattern off
	{0x5e01, 0x41},   // ; window cut enable
	{0x382d, 0x7f},   //
	{0x4825, 0x3a},   // ; lpx_p_min
	{0x4826, 0x40},   // ; hs_prepare_min
	{0x4808, 0x25},   // ; wake up delay in 1/1024 s
	{0x3763, 0x18},   //
	{0x3768, 0xcc},   //
	{0x470b, 0x28},   //
	{0x4202, 0x00},   //
	{0x400d, 0x10},   // ; BLC offset trigger L
	{0x4040, 0x04},   // ; BLC gain th2
	{0x403e, 0x04},   // ; BLC gain th1
	{0x4041, 0xc6},   // ; BLC
	{0x3007, 0x80},   //
	{0x400a, 0x01},   //

//////////////////////////////
	{0x5780, 0xfc},
	{0x5784, 0x0c},
	{0x5787, 0x40},
	{0x5788, 0x08},
	{0x578a, 0x02},
	{0x578b, 0x01},
	{0x578c, 0x01},
	{0x578e, 0x02},
	{0x578f, 0x01},
	{0x5790, 0x01},
/////////////////////////////

	{ 0x0100, 0x01 }	//
};


static struct regval_list sensor_quxga_regs[] = {
	//@@3264_2448_4lane_30fps_720Mbps/lane
	{0x0100, 0x00},
	{0x3501, 0x9a},  //; exposure M
	{0x3502, 0x20},  //; exposure L
	{0x3508, 0x02},  //; gain H
	{0x3808, 0x0c},  //; x output size H
	{0x3809, 0xc0},  //; x output size L
	{0x380a, 0x09},  //; y output size H
	{0x380b, 0x90},  //; y output size L
	{0x380c, 0x07},  //; HTS H
	{0x380d, 0x94},  //; HTS L
	{0x380e, 0x09},  //; VTS H
	{0x380f, 0xaa},  //; VTS L
	{0x3814, 0x01},  //; x odd inc
	{0x3821, 0x46},  //; mirror on, bin off
	{0x382a, 0x01},  //; y odd inc
	{0x3830, 0x06},
	{0x3836, 0x01},
	{0x3f0a, 0x00},
	{0x4001, 0x00},   //; total 256 black column
	{0x4022, 0x0c},   //; Anchor left end H
	{0x4023, 0x60},   //; Anchor left end L
	{0x4025, 0x36},   //; Anchor right start L
	{0x4027, 0x37},   //; Anchor right end L
	{0x402b, 0x08},   //; top black line number
	{0x402f, 0x08},   //; interpolation x disable, interpolation y disable, Anchor one disable
	{0x4500, 0x58},
	{0x4600, 0x01},
	{0x4601, 0x97},
	{0x382d, 0xff},
	{0x0100, 0x01}
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

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
				 struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val, frame_length, shutter;
	unsigned char explow = 0, expmid = 0, exphigh = 0;
	unsigned char gainlow = 0, gainhigh = 0;
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;





	if (gain_val < 1 * 16)
		gain_val = 16;
	if (gain_val > 64 * 16 - 1)
		gain_val = 64 * 16 - 1;

	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	gain_val *= 8;
	gainlow = (unsigned char)(gain_val & 0xff);
	gainhigh = (unsigned char)((gain_val >> 8) & 0x7);
	exphigh = (unsigned char)((0x0f0000 & exp_val) >> 16);
	expmid = (unsigned char)((0x00ff00 & exp_val) >> 8);
	explow = (unsigned char)((0x0000ff & exp_val));

	shutter = exp_val / 16;
	if (shutter > ov8858_sensor_vts - 4)
		frame_length = shutter + 4;
	else
		frame_length = ov8858_sensor_vts;

	sensor_write(sd, 0x3208, 0x00);

	sensor_write(sd, 0x380f, (frame_length & 0xff));
	sensor_write(sd, 0x380e, (frame_length >> 8));

	sensor_write(sd, 0x3509, gainlow);
	sensor_write(sd, 0x3508, gainhigh);

	sensor_write(sd, 0x3502, explow);
	sensor_write(sd, 0x3501, expmid);
	sensor_write(sd, 0x3500, exphigh);
	sensor_write(sd, 0x3208, 0x10);
	sensor_write(sd, 0x3208, 0xa0);
	info->exp = exp_val;
	info->gain = gain_val;
	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	unsigned char explow, expmid, exphigh;
	struct sensor_info *info = to_state(sd);

	if (exp_val > 0xfffff)
		exp_val = 0xfffff;




	exphigh = (unsigned char)((0x0f0000 & exp_val) >> 16);
	expmid = (unsigned char)((0x00ff00 & exp_val) >> 8);
	explow = (unsigned char)((0x0000ff & exp_val));

	sensor_write(sd, 0x3208, 0x00);
	sensor_write(sd, 0x3502, explow);
	sensor_write(sd, 0x3501, expmid);
	sensor_write(sd, 0x3500, exphigh);

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
	unsigned char gainlow = 0;
	unsigned char gainhigh = 0;

	if (gain_val < 1 * 16)
		gain_val = 16;
	if (gain_val > 64 * 16 - 1)
		gain_val = 64 * 16 - 1;
	sensor_dbg("sensor_set_gain = %d\n", gain_val);



	gain_val *= 8;

	if (gain_val < 2 * 16 * 8) {
		gainhigh = 0;
		gainlow = gain_val;
	} else if (2 * 16 * 8 <= gain_val && gain_val < 4 * 16 * 8) {
		gainhigh = 1;
		gainlow = gain_val / 2 - 8;
	} else if (4 * 16 * 8 <= gain_val && gain_val < 8 * 16 * 8) {
		gainhigh = 3;
		gainlow = gain_val / 4 - 12;
	} else {
		gainhigh = 7;
		gainlow = gain_val / 8 - 8;
	}

	sensor_write(sd, 0x3509, gainlow);
	sensor_write(sd, 0x3508, gainhigh);
	sensor_write(sd, 0x3208, 0x10);
	sensor_write(sd, 0x3208, 0xa0);


	info->gain = gain_val;

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
	if (rdval != 0x58)
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
		ret = sensor_s_exp_gain(sd, (struct sensor_exp_gain *)arg);
		break;
	case VIDIOC_VIN_SENSOR_CFG_REQ:
		sensor_cfg_req(sd, (struct sensor_config *)arg);
		break;
	case VIDIOC_VIN_ACT_INIT:
		sensor_print("%s VIDIOC_VIN_ACT_INIT\n", __func__);
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
	/* quxga: 3264*2448 */
	{
	 .width = QUXGA_WIDTH,
	 .height = QUXGA_HEIGHT,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 1940,
	 .vts = 2474,
	 .pclk = 144 * 1000 * 1000,
	 .mipi_bps = 720 * 1000 * 1000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .intg_min = 16,
	 .intg_max = (2474 - 4) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 15 << 4,
	 .regs = sensor_quxga_regs,
	 .regs_size = ARRAY_SIZE(sensor_quxga_regs),
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
	ov8858_sensor_vts = wsize->vts;

	sensor_print("s_fmt = %x, width = %d, height = %d\n",
			  sensor_fmt->mbus_code, wsize->width, wsize->height);
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

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_CSI2_DPHY;
	cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;

	return 0;
}

static int __attribute__((unused)) sensor_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	/* Fill in min, max, step and default value for these controls. */
	/* see include/linux/videodev2.h for details */

	switch (qc->id) {
	case V4L2_CID_GAIN:
		return v4l2_ctrl_query_fill(qc, 1 * 16, 64 * 16 - 1, 1, 1 * 16);
	case V4L2_CID_EXPOSURE:
		return v4l2_ctrl_query_fill(qc, 0, 65535 * 16, 1, 0);
	case V4L2_CID_FRAME_RATE:
		return v4l2_ctrl_query_fill(qc, 15, 120, 1, 120);
	}
	return -EINVAL;
}
static int __attribute__((unused))  sensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE:
		return sensor_g_exp(sd, &ctrl->value);
	}
	return -EINVAL;
}

static int __attribute__((unused)) sensor_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct v4l2_queryctrl qc;
	int ret;

	qc.id = ctrl->id;
	ret = sensor_queryctrl(sd, &qc);
	if (ret < 0) {
		return ret;
	}

	if (ctrl->value < qc.minimum || ctrl->value > qc.maximum) {
		sensor_err("max gain qurery is %d,min gain qurey is %d\n",
				qc.maximum, qc.minimum);
		return -ERANGE;
	}

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->value);
	case V4L2_CID_EXPOSURE:
		return sensor_s_exp(sd, ctrl->value);
	}
	return -EINVAL;
}

/* ----------------------------------------------------------------------- */

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	//.g_ctrl = sensor_g_ctrl,
	//.s_ctrl = sensor_s_ctrl,
	//.queryctrl = sensor_queryctrl,
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
	.addr_width = CCI_BITS_16,
	.data_width = CCI_BITS_8,
};

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
	mutex_init(&info->lock);
#ifdef CONFIG_SAME_I2C
	info->sensor_i2c_addr = I2C_ADDR >> 1;
#endif
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

module_init(init_sensor);
module_exit(exit_sensor);
