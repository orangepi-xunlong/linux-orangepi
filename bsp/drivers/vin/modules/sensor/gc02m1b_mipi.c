/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for GC02M1 Raw cameras.
 *
 * Copyright (c) 2018 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zheng ZeQun <zequnzheng@allwinnertech.com>
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

MODULE_AUTHOR("lwj");
MODULE_DESCRIPTION("A low-level driver for GC02M1 sensors");
MODULE_LICENSE("GPL");

#define MCLK              (24 * 1000 * 1000)
#define V4L2_IDENT_SENSOR 0x02E0

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The GC02M1 i2c address
 */
#define I2C_ADDR 0x6e

#define SENSOR_NAME "gc02m1b_mipi"

#define GC02M1B_MIRROR_NORMAL    1
#define GC02M1B_MIRROR_H         0
#define GC02M1B_MIRROR_V         0
#define GC02M1B_MIRROR_HV        0

#if GC02M1B_MIRROR_NORMAL
#define GC02M1B_MIRROR	        0x80
#elif GC02M1B_MIRROR_H
#define GC02M1B_MIRROR	        0x81
#elif GC02M1B_MIRROR_V
#define GC02M1B_MIRROR	        0x82
#elif GC02M1B_MIRROR_HV
#define GC02M1B_MIRROR	        0x83
#else
#define GC02M1B_MIRROR	        0x80
#endif

#define EEPROM_WRITE_ID     0x17

#define AW_OTP_OFFSET     0x1000

#define AW_OTP_AWB_Start 0x0009
#define AW_OTP_MSC_Start 0x0011
#define AW_OTP_DPC_Start 0x0613
#define AW_OTP_CHECK_Start 0x061d

/* #define OTP_SIZE 1895 */
/* static int sensor_otp_enable;  0: init, -1: disable, 1: enable */
/* static unsigned char sensor_otp_info[OTP_SIZE]; */

#define LSC_INFO_SIZE (16 * 16 * 3)
#define AWB_INFO_SIZE 8
/*
static unsigned short sensor_otp_lscinfo[LSC_INFO_SIZE + AWB_INFO_SIZE];

static unsigned int convert_lsc_tbl[LSC_INFO_SIZE] = {
	 90794,  93286,  94355,  95203,  95488,  95698,  95863,	 95621,	 95607,
			 95389, 94966, 94276, 93462, 93519, 93640, 93029,
	 92161,  94458,  95063,  95418,  95426,  95633,  95839,	 96284,	 96155,
			 95693, 94974, 94375, 93595, 93379, 93698, 93491,
	 94002,  95405,  95594,  95816,  95769,  96476,  96655,	 96984,	 96844,
			 96518, 95741, 94401, 94027, 93768, 93805, 94388,
	 94338,  95584,  95729,  95857,  96380,  96507,  97464,	 97489,	 97712,
			 97476, 96223, 95129, 93891, 94084, 93747, 94457,
	 95094,  95634,  95921,  95918,  96541,  97302,  98164,	 98798,	 98745,
			 98314, 97196, 95692, 94539, 93872, 93463, 93996,
	 95563,  95880,  95834,  96228,  96831,  97998,  99208,	 99224,	 99362,
			 99370, 97937, 96184, 94885, 93624, 93535, 94172,
	 95974,  95969,  96028,  96748,  97520,  98831,  99804,	 99832,	 99887,
			 99802, 99004, 97083, 95296, 93967, 93918, 94130,
	 96463,  96361,  96072,  96771,  97685,  99395,  99907,	 99942,	 99980,
			 99907, 99519, 97526, 95603, 94223, 94039, 94566,
	 96828,  96234,  95815,  96585,  97646,  99087,  99499,	 99670,	 99863,
			100019, 99326, 97509, 95621, 94260, 93730, 94437,
	 97190,  96111,  95808,  96212,  97418,  98897,  99619,	 99753,	 99905,
			 99890, 99325, 97105, 95366, 94302, 94205, 94696,
	 97125,  96820,  96013,  96301,  97275,  98591,  99843,	100108,	 99945,
			 99842, 98711, 96760, 95154, 94165, 94175, 95302,
	 97271,  96717,  96384,  96343,  97008,  98162,  99211,	 99795,	 99965,
			 99347, 98120, 96482, 95186, 94313, 94767, 95468,
	 97485,  97427,  97182,  96816,  97124,  97936,  98792,	 99240,	 98955,
			 98459, 97318, 95958, 94926, 94740, 95224, 96241,
	 97947,  98224,  98035,  97456,  97468,  97844,  98288,	 98620,	 98418,
			 97921, 96715, 95502, 94975, 94995, 95659, 96495,
	 98374,  99411,  98538,  98035,  98052,  98065,  98639,	 98562,	 98391,
			 97545, 96900, 95787, 95599, 95529, 96383, 97012,
	 98493,  99883,  99472,  98913,  98779,  98466,  98990,	 98780,	 98466,
			 97806, 96803, 96379, 96211, 95944, 96839, 97217,
	 95167,  97054,  97999,  98597,  98838,  99028,  98602,	 98802,	 98325,
			 98098, 97316, 96384, 95865, 95616, 95769, 96492,
	 96472,  97878,  98848,  99158,  99696,  99128,  99016,	 99123,	 98709,
			 98244, 97968, 96879, 96163, 95803, 95718, 96587,
	 97643,  99200,  99580, 100014,  99912,  99649,  99573,	 99700,	 99463,
			 98649, 98380, 97498, 96649, 96318, 96092, 96614,
	 98782,  99785, 100331, 100243,  99935,  99879,  99968,	 99778,	 99748,
			 99373, 98957, 98033, 96954, 96248, 96266, 96122,
	 99405, 100087, 100110,  99913, 100071, 100211, 100012,	100112,	 99835,
			 99685, 99432, 98343, 97445, 96488, 96000, 95936,
	 99546, 100065, 100119,  99867,  99872, 100151, 100014,	100316,	100141,
			100017, 99481, 98465, 97450, 96530, 95938, 95814,
	 99987, 100214,  99993,  99774,  99808, 100078, 100230,	100335,	100187,
			 99964, 99551, 98496, 97518, 96665, 95723, 95756,
	100255, 100218, 100068,  99588,  99610,  99979, 100220,	100000,	 99942,
			 99777, 99674, 98659, 97490, 96525, 95839, 95839,
	100288, 100216,  99812,  99352,  99362,  99774,  99775,	 99922,	 99786,
			 99869, 99393, 98342, 97232, 96397, 95798, 95804,
	100462, 100053,  99416,  98911,  99081,  99394,  99543,	 99581,	 99542,
			 99412, 98913, 97923, 96962, 95853, 95494, 95804,
	100158,  99730,  99272,  98881,  98822,  99131,  99353,	 99361,	 99340,
			 99067, 98422, 97503, 96595, 95641, 95247, 95889,
	 99927,  99607,  99153,  98817,  98734,  98827,  98804,	 98905,	 98730,
			 98566, 98034, 96975, 96176, 95471, 95253, 96117,
	 99818,  99435,  99139,  98711,  98426,  98368,  98362,	 98464,	 98190,
			 97776, 97183, 96305, 95729, 95357, 95361, 96179,
	 99953,  99800,  99022,  98405,  97944,  97789,  97565,	 97728,	 97382,
			 96863, 96261, 95692, 95146, 95057, 95442, 96732,
	100892, 100152,  99033,  98339,  97535,  97378,  96947,	 96971,	 96648,
			 96383, 95842, 94989, 95029, 95082, 95749, 97167,
	100691,  99646,  99030,  97684,  97051,  96639,  96174,	 95940,	 95697,
			 95297, 94761, 94417, 94306, 94606, 95613, 97906,
	 94166,  94745,  95182,  94660,  94720,  94283,  94250,	 94067,	 93543,
			 93296, 92217, 91759, 91132, 91139, 92114, 93992,
	 94768,  95622,  95524,  95702,  95455,  95194,  94967,	 95092,	 94705,
			 93831, 93179, 92004, 91381, 91093, 91411, 93008,
	 96201,  96840,  96891,  96465,  96619,  96438,  96354,	 96124,	 95432,
			 94781, 94249, 93074, 92209, 91734, 91804, 93166,
	 96803,  97489,  97542,  97337,  97475,  97294,  97336,	 97077,	 96807,
			 95985, 95257, 94187, 93138, 91777, 91756, 92828,
	 97483,  98216,  98141,  98057,  98277,  98205,  98235,	 98282,	 98116,
			 97135, 96348, 94887, 93534, 92596, 92342, 92720,
	 97704,  98934,  98913,  98666,  98738,  98734,  99576,	 99468,	 98964,
			 98488, 97433, 95873, 94303, 92998, 92272, 92670,
	 98573,  98942,  99404,  99153,  99257,  99997, 100052,	100148,	 99588,
			 99125, 98057, 96393, 95046, 93737, 92591, 93350,
	 99403,  99413,  99670,  99410,  99725, 100167, 100459,	100382,	 99845,
			 99512, 98706, 96952, 95533, 94053, 92519, 93052,
	 99741,  99833,  99887,  99638,  99715, 100306, 100183,	100310,	 99980,
			 99715, 98657, 96971, 95323, 93733, 92891, 93371,
	100039,  99868,  99795,  99235,  99326, 100167, 100532,	100230,	100347,
			 99718, 98676, 96966, 95176, 93902, 93000, 93556,
	100379,  99879,  99669,  99482,  99040,  99849, 100303,	100221,	100168,
			 99418, 98413, 96469, 94942, 93692, 93008, 94060,
	100458, 100021,  99420,  99113,  98867,  98925,  99553,	 99999,	 99464,
			 98824, 97671, 95973, 94784, 93887, 93748, 94398,
	100775, 100053,  99237,  98843,  98241,  98231,  98587,	 98595,	 98274,
			 97705, 96547, 95292, 94529, 93701, 93543, 95055,
	100338,  99786,  99016,  98066,  97803,  97130,  97318,	 97093,	 97143,
			 96269, 95703, 94486, 93920, 93354, 93945, 95302,
	100687, 100183,  98987,  97628,  96974,  96581,  96328,	 95876,	 95425,
			 95016, 94588, 93792, 93795, 93560, 94329, 95973,
	100902,  99620,  98430,  96779,  96097,  95514,  94750,	 94272,	 94094,
			 93973, 93625, 92738, 93083, 93403, 94215, 97442,
};
*/
/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {
};

static struct regval_list sensor_1600x1200p30_regs[] = {
	/* system */
	{0xfc, 0x01},
	{0xf4, 0x41},
	{0xf5, 0xe3},
	{0xf6, 0x44},
	{0xf8, 0x38},
	{0xf9, 0x82},
	{0xfa, 0x00},
	{0xfd, 0x80},
	{0xfc, 0x81},
	{0xfe, 0x03},
	{0x01, 0x0b},
	{0xf7, 0x01},
	{0xfc, 0x80},
	{0xfc, 0x80},
	{0xfc, 0x80},
	{0xfc, 0x8e},

	/* CISCTL */
	{0xfe, 0x00},
	{0x87, 0x09},
	{0xee, 0x72},
	{0xfe, 0x01},
	{0x8c, 0x90},
	{0xfe, 0x00},
	{0x90, 0x00},
	{0x03, 0x04},
	{0x04, 0x7d},
	{0x41, 0x04},
	{0x42, 0xf4},
	{0x05, 0x04},
	{0x06, 0x48},
	{0x07, 0x00},
	{0x08, 0x18},
	{0x9d, 0x18},
	{0x09, 0x00},
	{0x0a, 0x02},
	{0x0d, 0x04},
	{0x0e, 0xbc},
    {0x17, GC02M1B_MIRROR},
	{0x19, 0x04},
	{0x24, 0x00},
	{0x56, 0x20},
	{0x5b, 0x00},
	{0x5e, 0x01},

	/* analog Register width */
	{0x21, 0x3c},
	{0x44, 0x20},
	{0xcc, 0x01},

	/* analog mode */
	{0x1a, 0x04},
	{0x1f, 0x11},
	{0x27, 0x30},
	{0x2b, 0x00},
	{0x33, 0x00},
	{0x53, 0x90},
	{0xe6, 0x50},

	/* analog voltage */
	{0x39, 0x07},
	{0x43, 0x04},
	{0x46, 0x4a},
	{0x7c, 0xa0},
	{0xd0, 0xbe},
	{0xd1, 0x60},
	{0xd2, 0x40},
	{0xd3, 0xf3},
	{0xde, 0x1d},

	/* analog current */
	{0xcd, 0x05},
	{0xce, 0x6f},

	/* CISCTL RESET */
	{0xfc, 0x88},
	{0xfe, 0x10},
	{0xfe, 0x00},
	{0xfc, 0x8e},
	{0xfe, 0x00},
	{0xfe, 0x00},
	{0xfe, 0x00},
	{0xfe, 0x00},
	{0xfc, 0x88},
	{0xfe, 0x10},
	{0xfe, 0x00},
	{0xfc, 0x8e},
	{0xfe, 0x04},
	{0xe0, 0x01},
	{0xfe, 0x00},

	/* ISP */
	{0xfe, 0x01},
	{0x53, 0x54},
	{0x87, 0x53},
	{0x89, 0x03},

	/* Gain */
	{0xfe, 0x00},
	{0xb0, 0x74},
	{0xb1, 0x04},
	{0xb2, 0x00},
	{0xb6, 0x00},
	{0xfe, 0x04},
	{0xd8, 0x00},
	{0xc0, 0x40},
	{0xc0, 0x00},
	{0xc0, 0x00},
	{0xc0, 0x00},
	{0xc0, 0x60},
	{0xc0, 0x00},
	{0xc0, 0xc0},
	{0xc0, 0x2a},
	{0xc0, 0x80},
	{0xc0, 0x00},
	{0xc0, 0x00},
	{0xc0, 0x40},
	{0xc0, 0xa0},
	{0xc0, 0x00},
	{0xc0, 0x90},
	{0xc0, 0x19},
	{0xc0, 0xc0},
	{0xc0, 0x00},
	{0xc0, 0xD0},
	{0xc0, 0x2F},
	{0xc0, 0xe0},
	{0xc0, 0x00},
	{0xc0, 0x90},
	{0xc0, 0x39},
	{0xc0, 0x00},
	{0xc0, 0x01},
	{0xc0, 0x20},
	{0xc0, 0x04},
	{0xc0, 0x20},
	{0xc0, 0x01},
	{0xc0, 0xe0},
	{0xc0, 0x0f},
	{0xc0, 0x40},
	{0xc0, 0x01},
	{0xc0, 0xe0},
	{0xc0, 0x1a},
	{0xc0, 0x60},
	{0xc0, 0x01},
	{0xc0, 0x20},
	{0xc0, 0x25},
	{0xc0, 0x80},
	{0xc0, 0x01},
	{0xc0, 0xa0},
	{0xc0, 0x2c},
	{0xc0, 0xa0},
	{0xc0, 0x01},
	{0xc0, 0xe0},
	{0xc0, 0x32},
	{0xc0, 0xc0},
	{0xc0, 0x01},
	{0xc0, 0x20},
	{0xc0, 0x38},
	{0xc0, 0xe0},
	{0xc0, 0x01},
	{0xc0, 0x60},
	{0xc0, 0x3c},
	{0xc0, 0x00},
	{0xc0, 0x02},
	{0xc0, 0xa0},
	{0xc0, 0x40},
	{0xc0, 0x80},
	{0xc0, 0x02},
	{0xc0, 0x18},
	{0xc0, 0x5c},
	{0xfe, 0x00},
	{0x9f, 0x10},

	/* BLK */
	{0xfe, 0x00},
	{0x26, 0x20},
	{0xfe, 0x01},
	{0x40, 0x22},
	{0x46, 0x7f},
	{0x49, 0x0f},
	{0x4a, 0xf0},
	{0xfe, 0x04},
	{0x14, 0x80},
	{0x15, 0x80},
	{0x16, 0x80},
	{0x17, 0x80},

	/* ant _blooming */
	{0xfe, 0x01},
	{0x41, 0x20},
	{0x4c, 0x00},
	{0x4d, 0x0c},
	{0x44, 0x08},
	{0x48, 0x03},

	/* Window 1600X1200 */
	{0xfe, 0x01},
	{0x90, 0x01},
	{0x91, 0x00},
	{0x92, 0x06},
	{0x93, 0x00},
	{0x94, 0x06},
	{0x95, 0x04},
	{0x96, 0xb0},
	{0x97, 0x06},
	{0x98, 0x40},

	/* mipi */
	{0xfe, 0x03},
	{0x01, 0x23},
	{0x03, 0xce},
	{0x04, 0x48},
	{0x15, 0x00},
	{0x21, 0x10},
	{0x22, 0x05},
	{0x23, 0x20},
	{0x25, 0x20},
	{0x26, 0x08},
	{0x29, 0x06},
	{0x2a, 0x0a},
	{0x2b, 0x08},

	/* out */
	{0xfe, 0x01},
	{0x8c, 0x10},
	{0xfe, 0x00},
	{0x3e, 0x00},

	{0xfe, 0x00},
	{0x3e, 0x90},
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

static int gc02m1b_sensor_vts;
static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	unsigned char explow, exphigh;
	struct sensor_info *info = to_state(sd);

	sensor_write(sd, 0xfe, 0x00);

	exphigh  = (unsigned char)((exp_val >> 12) & 0x3f);
	explow  = (unsigned char)((exp_val >> 4) & 0xff);

	sensor_write(sd, 0x03, exphigh);
	sensor_write(sd, 0x04, explow);

	sensor_dbg("sensor_set_exp = %d, Done!\n", exp_val);

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

#define GC02M1_SENSOR_GAIN_MAX_VALID_INDEX  16
#define GC02M1_SENSOR_GAIN_BASE             0x400
#define GC02M1_SENSOR_GAIN_MAX              (12 * GC02M1_SENSOR_GAIN_BASE)
#define GC02M1_SENSOR_DGAIN_BASE            0x400
static int GC02M1_AGC_Param[GC02M1_SENSOR_GAIN_MAX_VALID_INDEX][2] = {
	{  1024,  0 },
	{  1536,  1 },
	{  2035,  2 },
	{  2519,  3 },
	{  3165,  4 },
	{  3626,  5 },
	{  4147,  6 },
	{  4593,  7 },
	{  5095,  8 },
	{  5697,  9 },
	{  6270, 10 },
	{  6714, 11 },
	{  7210, 12 },
	{  7686, 13 },
	{  8214, 14 },
	{ 10337, 15 },
};

static int sensor_s_gain(struct v4l2_subdev *sd, unsigned int gain_val)
{
	struct sensor_info *info = to_state(sd);
	unsigned int All_gain, temp_gain;
	unsigned int gain_index;

	All_gain = gain_val << 4;

	if (All_gain > GC02M1_SENSOR_GAIN_MAX)
		All_gain = GC02M1_SENSOR_GAIN_MAX;

	for (gain_index = GC02M1_SENSOR_GAIN_MAX_VALID_INDEX - 1;
	     gain_index > 0; gain_index--)
		if (All_gain >= GC02M1_AGC_Param[gain_index][0])
			break;

	sensor_write(sd, 0xfe, 0x00);
	sensor_write(sd, 0xb6, GC02M1_AGC_Param[gain_index][1]);
	temp_gain = All_gain *
		    GC02M1_SENSOR_DGAIN_BASE / GC02M1_AGC_Param[gain_index][0];
	sensor_write(sd, 0xb1, (temp_gain >> 8) & 0x1f);
	sensor_write(sd, 0xb2, temp_gain & 0xff);

	sensor_dbg("gc02m1b_mipi[gain_index][1] = %d, temp_gain = 0x%x, All_gain = %d\n",
		   GC02M1_AGC_Param[gain_index][1], temp_gain, All_gain);

	info->gain = gain_val;

	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val, frame_length, shutter;
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	shutter = exp_val >> 4;
	if (shutter  > gc02m1b_sensor_vts - 16)
		frame_length = shutter + 16;
	else
		frame_length = gc02m1b_sensor_vts;

	sensor_write(sd, 0xfe, 0x00);
	sensor_write(sd, 0x42, (frame_length & 0xff));
	sensor_write(sd, 0x41, (frame_length >> 8));
	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);

	sensor_dbg("sensor_set_gain exp = %d, gain = %d Done!\n",
		   exp_val, gain_val);

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

	switch (on) {
	case STBY_ON:
		sensor_print("STBY_ON!\n");
		cci_lock(sd);
		ret = sensor_s_sw_stby(sd, STBY_ON);
		if (ret < 0)
			sensor_err("soft stby falied!\n");
		usleep_range(1000, 1200);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		sensor_print("STBY_OFF!\n");
		cci_lock(sd);
		usleep_range(1000, 1200);
		ret = sensor_s_sw_stby(sd, STBY_OFF);
		if (ret < 0)
			sensor_err("soft stby falied!\n");
		cci_unlock(sd);
		break;
	case PWR_ON:
		sensor_print("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);

		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(100, 120);

		vin_set_pmu_channel(sd, IOVDD, ON);
		usleep_range(100, 120);
		vin_set_pmu_channel(sd, DVDD, ON);
		usleep_range(100, 120);
		vin_set_pmu_channel(sd, AVDD, ON);
		usleep_range(200, 220);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(200, 220);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_print("PWR_OFF!\n");
		cci_lock(sd);
		usleep_range(100, 120);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_set_mclk(sd, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
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
	unsigned int SENSOR_ID = 0;
	data_type val;

	sensor_read(sd, 0xf0, &val);
	SENSOR_ID |= (val << 8);
	sensor_read(sd, 0xf1, &val);
	SENSOR_ID |= (val);
	sensor_print("V4L2_IDENT_SENSOR = 0x%x\n", SENSOR_ID);

	if (SENSOR_ID != V4L2_IDENT_SENSOR)
		return -ENODEV;

	return 0;
}
#if VIN_FALSE
static int sensor_read_block_otp_a16_d8(struct v4l2_subdev *sd,
					unsigned char dev_addr,
					unsigned short eeprom_addr,
					unsigned char *buf, int len)
{
	unsigned char reg[3];
	struct i2c_msg msg[2];
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!sd || !buf)
		return -1;

	reg[0] = (eeprom_addr & 0xff00) >> 8;
	reg[1] = eeprom_addr & 0xff;
	reg[2] = 0xee;

	msg[0].addr = dev_addr >> 1;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = reg;

	msg[1].addr = dev_addr >> 1;
	msg[1].flags = I2C_M_RD;
	msg[1].len = len;
	msg[1].buf = buf;
	sensor_write(sd, 0xfe, 0x02);
	return (i2c_transfer(client->adapter, msg, 2) == 2);
}

static int sensor_write_block_otp_a16_d8(struct v4l2_subdev *sd,
					 unsigned char dev_addr,
					 unsigned short eeprom_addr,
					 unsigned char *val, int len)
{
	int ret = 0, i;
	struct i2c_msg msg;
	unsigned char data[3];
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	sensor_write(sd, 0xfe, 0x02);
	for (i = 0; i < len; i++) {
		data[0] = ((eeprom_addr + i) & 0xff00) >> 8;
		data[1] = ((eeprom_addr + i) & 0x00ff);
		data[2] = val[eeprom_addr + i];

		msg.addr = dev_addr >> 1;
		msg.flags = 0;
		msg.len = 3;
		msg.buf = data;

		ret = i2c_transfer(client->adapter, &msg, 1);
		usleep_range(10000, 12000);
		if (ret < 0)
			break;
	}
	return ret;
}
#endif
/*
static int sensor_write_block_otp_continuous_a16_d8(struct v4l2_subdev *sd,
		unsigned char dev_addr,
		unsigned short eeprom_addr,
		unsigned char *buf, int size)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg;
	unsigned char data[2 + 32];
	unsigned char *p = buf;
	int ret, i, len;

	while (size > 0) {
		len = size > 32 ? 32 : size;
		data[0] = (eeprom_addr & 0xff00) >> 8;
		data[1] = (eeprom_addr & 0x00ff);

		for (i = 2; i < 2 + len; i++)
			data[i] = *p++;

		msg.addr = dev_addr >> 1;
		msg.flags = 0;
		msg.len = 2 + len;
		msg.buf = data;

		ret = i2c_transfer(client->adapter, &msg, 1);

		if (ret > 0) {
			ret = 0;
		} else if (ret < 0) {
			sensor_err("otp_write error!\n");
			break;
		}
		dev_addr += len;
		size -= len;
	}
	return ret;
}
*/
#if VIN_FALSE
static int check_otp_checksum(unsigned char *data, int len)
{
	unsigned short rdval1, rdval2, rdval3;
	int ret = 0;

	if (!data || len <= 0)
		return -1;

	rdval1 = data[0x0011];
	rdval1 = (rdval1 << 8) + data[0x0012];
	if ((rdval1 / 128) != data[0x061d])
		ret = -1;

	rdval2 = data[0x0211];
	rdval2 = (rdval2 << 8) + data[0x0212];
	if ((rdval2 / 128) != data[0x061e])
		ret = -1;

	rdval3 = data[0x0411];
	rdval3 = (rdval3 << 8) + data[0x0412];
	if ((rdval3 / 128) != data[0x061f])
		ret = -1;

	if ((data[0x061d] + data[0x061e] + data[0x061f]) != data[0x0620])
		ret = -1;

	if (ret < 0)
		sensor_err("sensor otp info checksum error\n");

	return ret;
}

static void conver_Otp2AwInfo(unsigned char *sensor_otp_src,
			      unsigned short *aw_info_dst)
{
	unsigned char *src;
	unsigned short *dst;
	unsigned int i;
	unsigned short val, high, low;

	/* Get MSC table */
	src = &sensor_otp_src[AW_OTP_MSC_Start];
	dst = &aw_info_dst[0];
	for (i = 0; i < LSC_INFO_SIZE; i++) {
		high = *src;
		src++;
		low = *src;
		src++;
		val = (high << 8) + low;
		*dst = val;
		dst++;
	}

	/* Get AWB Data */
	src = &sensor_otp_src[AW_OTP_AWB_Start];
	dst = &aw_info_dst[LSC_INFO_SIZE];
	for (i = 0; i < AWB_INFO_SIZE; i++) {
		*dst = *src;
		dst++;
		src++;
	}
}

static void conver_AwInfo2Otp(unsigned short *aw_info_src,
			      unsigned char *sensor_otp_dst)
{
	unsigned int i;
	unsigned short val_high, val_low;

	/* Set MSC table */
	for (i = 0; i < LSC_INFO_SIZE; i++) {
		val_high = (aw_info_src[i] >> 8) & 0xff;
		val_low  = aw_info_src[i] & 0xff;
		sensor_otp_dst[AW_OTP_MSC_Start + 2 * i] =
						(unsigned char)val_high;
		sensor_otp_dst[AW_OTP_MSC_Start + 2 * i + 1] =
						(unsigned char)val_low;
	}
	/* Set MSC table size ----- 16*16*3 */
	sensor_otp_dst[0x0611] = 0x03;
	sensor_otp_dst[0x0612] = 0x00;

	/* DATA CHECK */
	sensor_otp_dst[0x061d] = aw_info_src[0] / 128;
	sensor_otp_dst[0x061e] = aw_info_src[256] / 128;
	sensor_otp_dst[0x061f] = aw_info_src[512] / 128;
	sensor_otp_dst[0x0620] = sensor_otp_dst[0x061d] +
				 sensor_otp_dst[0x061e] +
				 sensor_otp_dst[0x061f];
}
#endif
static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret = 0;
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
	info->width = 1600;
	info->height = 1200;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30; /* 30fps */
#if VIN_FALSE
	if (sensor_otp_enable == 0) {
		memset(sensor_otp_info, 0, sizeof(sensor_otp_info));
		ret = sensor_read_block_otp_a16_d8(sd, EEPROM_WRITE_ID,
						   AW_OTP_OFFSET,
						   sensor_otp_info,
						   sizeof(sensor_otp_info));
		if (ret < 0 || sensor_otp_info[0x0006] != 0x10) {
			sensor_otp_enable = -1;
			sensor_print("sensor_otp_disable ret=%d [0x0006]=%x\n",
				     ret, sensor_otp_info[0x0006]);
			return 0;
		}

		sensor_otp_enable = 1;
		ret = check_otp_checksum(sensor_otp_info,
					 sizeof(sensor_otp_info));
		if (ret < 0)
			sensor_otp_enable = -1;
		else
			sensor_print("sensor ckeck OTP success\n");

		conver_Otp2AwInfo(&sensor_otp_info[0], &sensor_otp_lscinfo[0]);
/*
		sensor_print("sensor_otp_lscinfo\n");
		for (i = 0; i < LSC_INFO_SIZE; i++) {
			if (i % 16 == 0)
				sensor_print("\n");
			sensor_print("%d, ", sensor_otp_lscinfo[i]);
		}
		sensor_print("\n");

		sensor_print("sensor_otp_AWB\n");
		for (i = 0; i < AWB_INFO_SIZE; i++)
			sensor_print("%d, ",
					sensor_otp_lscinfo[LSC_INFO_SIZE + i]);

		sensor_print("\n");
*/
		/* convert_lsc_tbl */
		for (i = 0; i < LSC_INFO_SIZE; i++)
			sensor_otp_lscinfo[i] = convert_lsc_tbl[i] *
						sensor_otp_lscinfo[i] / 100000;
	}
#endif
	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);
	struct sensor_config *config;

	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		if (info->current_wins) {
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
		config = (struct sensor_config *)arg;
		sensor_cfg_req(sd, config);
		break;
	case VIDIOC_VIN_FLASH_EN:
		ret = flash_en(sd, (struct flash_para *)arg);
		break;
	/* case VIDIOC_VIN_GET_SENSOR_OTP_INFO: {
		if (sensor_otp_enable == 1) {
			memcpy(arg, sensor_otp_lscinfo, sizeof(sensor_otp_lscinfo));
		} else {
			ret = -EFAULT;
		}
		break;
	} */
#if VIN_FALSE
	case VIDIOC_VIN_SET_SENSOR_OTP_INFO: {
		void __user *user_pmsc_table = (void __user *)arg;

		sensor_print("--- VIDIOC_VIN_SET_SENSOR_OTP_INFO ---\n ");
		if (copy_from_user(&sensor_otp_lscinfo[0], user_pmsc_table,
				   sizeof(sensor_otp_lscinfo))) {
			sensor_err(" copy from user lsc table error\n");
			ret = -EFAULT;
		}
/*
		sensor_print("sensor_otp_lscinfo----R:\n");
		for (i = 0; i < LSC_INFO_SIZE; i++) {
			if (i % 16 == 0)
				sensor_print("\n");

			sensor_print("%d, ", sensor_otp_lscinfo[i]);
		}
		sensor_print("\n");
*/
		conver_AwInfo2Otp(&sensor_otp_lscinfo[0], &sensor_otp_info[0]);

		if (sensor_write_block_otp_a16_d8(sd, EEPROM_WRITE_ID,
						  AW_OTP_MSC_Start +
						  AW_OTP_OFFSET,
						  sensor_otp_info,
						  (LSC_INFO_SIZE * 2) + 2) <
						  0) {
			sensor_err("write MSC OTP error\n");
		}
		if (sensor_write_block_otp_a16_d8(sd, EEPROM_WRITE_ID,
						  AW_OTP_CHECK_Start +
						  AW_OTP_OFFSET,
						  sensor_otp_info, 4) < 0) {
			sensor_err("write DATA CHECK error\n");
		}
		break;
	}
#endif
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
		.width      = 1600,
		.height     = 1200,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2192,
		.vts        = 1276,
		.pclk       = 84 * 1000 * 1000,
		.mipi_bps   = 865 * 1000 * 1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 1023 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 768 << 4,
		.regs       = sensor_1600x1200p30_regs,
		.regs_size  = ARRAY_SIZE(sensor_1600x1200p30_regs),
		.set_size   = NULL,
	},
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
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

static int sensor_reg_init(struct sensor_info *info)
{
	int ret;
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

	sensor_print("sensor_reg_init\n");

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);

	if (wsize->set_size)
		wsize->set_size(sd);

	info->width = wsize->width;
	info->height = wsize->height;
	gc02m1b_sensor_vts = wsize->vts;

	exp_gain.exp_val = 16000;
	exp_gain.gain_val = 512;
	sensor_s_exp_gain(sd, &exp_gain);
/*
	if (sensor_otp_enable == -1) {
		sensor_write(sd, 0xfe, 0x01); */ /* color bar */
/*		sensor_write(sd, 0x8c, 0x11);
		sensor_write(sd, 0xfe, 0x00);
	}
*/
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
	if (ctrl)
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
	if (!info)
		return -ENOMEM;
	sd = &info->sd;

	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);
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
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->time_hs = 0x30;
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
