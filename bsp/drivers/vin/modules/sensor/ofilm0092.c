/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for ofilm0092 tof cameras.
 *
 * Copyright (c) 2020 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
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

MODULE_AUTHOR("lwj");
MODULE_DESCRIPTION("A low-level driver for ofilm0092 tof sensors");
MODULE_LICENSE("GPL");

#define MCLK              (24 * 1000 * 1000)
#define V4L2_IDENT_SENSOR 0x0B12

/*
 * Our nominal (default) frame rate.
 */
/* i2c clk 100k */
#define SENSOR_FRAME_RATE 15

/*
 * The ofilm0092 i2c address
 */
#define I2C_ADDR 0x7A

#define SENSOR_NAME "ofilm0092"

/*
 * The default register settings
 */
static struct regval_list sensor_default_regs[] = {
	{0x8423, 0x00A2},	 /* FRAMEBLANK */
	/* S00_EXPOTIME 0.128 ms @ 60.24 MHz (Grey-Scale) */
	{0x9000, 0x1E1E},
	{0x9001, 0x0000},	 /* S00_FRAMETIME */
	{0x9002, 0x6992},	 /* S01_EXPOTIME 1.060 ms @ 80.32 MHz */
	{0x9003, 0x0000},	 /* S01_FRAMETIME */
	{0x9004, 0x6992},	 /* S02_EXPOTIME 1.060 ms @ 80.32 MHz */
	{0x9005, 0x0000},	 /* S02_FRAMETIME */
	{0x9006, 0x6992},	 /* S03_EXPOTIME 1.060 ms @ 80.32 MHz */
	{0x9007, 0x0000},	 /* S03_FRAMETIME */
	{0x9008, 0x6992},	 /* S04_EXPOTIME 1.060 ms @ 80.32 MHz */
	{0x9009, 0x0000},	 /* S04_FRAMETIME */
	{0x900A, 0x5F2D},	 /* S05_EXPOTIME 1.060 ms @ 60.24 MHz */
	{0x900B, 0x0000},	 /* S05_FRAMETIME */
	{0x900C, 0x5F2D},	 /* S06_EXPOTIME 1.060 ms @ 60.24 MHz */
	{0x900D, 0x0000},	 /* S06_FRAMETIME */
	{0x900E, 0x5F2D},	 /* S07_EXPOTIME 1.060 ms @ 60.24 MHz */
	{0x900F, 0x0000},	 /* S07_FRAMETIME */
	{0x9010, 0x5F2D},	 /* S08_EXPOTIME 1.060 ms @ 60.24 MHz */
	{0x9011, 0x0000},	 /* S08_FRAMETIME */
	/* S00_EXPOTIMEMAX 0.128 ms @ 60.24 MHz (Grey-Scale) */
	{0x9080, 0x1E1E},
	{0x9081, 0x0000},	 /* S00_FRAMETIMEMIN */
	{0x9082, 0x10A0},	 /* S00_ILLUCFG Gray-Scale @ 60.24 MHz (LUT1) */
	{0x9083, 0x00A0},	 /* S00_SENSORCFG (Greyscale) */
	{0x9084, 0x8000},	 /* S00_ROCFG (Reconfig allowed) */
	{0x9085, 0x6992},	 /* S01_EXPOTIMEMAX 1.060 ms @ 80.32 MHz */
	{0x9086, 0x0000},	 /* S01_FRAMETIMEMIN */
	{0x9087, 0x0000},	 /* S01_ILLUCFG 0 @ 80.32 MHz (LUT0) */
	{0x9088, 0x0000},	 /* S01_SENSORCFG */
	{0x9089, 0x0000},	 /* S01_ROCFG (Reconfig forbidden) */
	{0x908A, 0x6992},	 /* S02_EXPOTIMEMAX 1.060 ms @ 80.32 MHz */
	{0x908B, 0x0000},	 /* S02_FRAMETIMEMIN */
	{0x908C, 0x0020},	 /* S02_ILLUCFG 90 @ 80.32 MHz (LUT0) */
	{0x908D, 0x0000},	 /* S02_SENSORCFG */
	{0x908E, 0x0000},	 /* S02_ROCFG (Reconfig forbidden) */
	{0x908F, 0x6992},	 /* S03_EXPOTIMEMAX 1.060 ms @ 80.32 MHz */
	{0x9090, 0x0000},	 /* S03_FRAMETIMEMIN */
	{0x9091, 0x0040},	 /* S03_ILLUCFG 180 :wq@ 80.32 MHz (LUT0) */
	{0x9092, 0x0000},	 /* S03_SENSORCFG */
	{0x9093, 0x0000},	 /* S03_ROCFG (Reconfig forbidden) */
	{0x9094, 0x6992},	 /* S04_EXPOTIMEMAX 1.060 ms @ 80.32 MHz */
	{0x9095, 0x0000},	 /* S04_FRAMETIMEMIN */
	{0x9096, 0x0060},	 /* S04_ILLUCFG 270 @ 80.32 MHz (LUT0) */
	{0x9097, 0x0000},	 /* S04_SENSORCFG */
	{0x9098, 0x0000},	 /* S04_ROCFG (Reconfig forbidden) */
	{0x9099, 0x5F2D},	 /* S05_EXPOTIMEMAX 1.060 ms @ 60.24 MHz */
	{0x909A, 0x0000},	 /* S05_FRAMETIMEMIN */
	{0x909B, 0x1000},	 /* S05_ILLUCFG 0 @ 60.24 MHz (LUT1) */
	{0x909C, 0x0000},	 /* S05_SENSORCFG */
	{0x909D, 0x0000},	 /* S05_ROCFG (Reconfig forbidden) */
	{0x909E, 0x5F2D},	 /* S06_EXPOTIMEMAX 1.060 ms @ 60.24 MHz */
	{0x909F, 0x0000},	 /* S06_FRAMETIMEMIN */
	{0x90A0, 0x1020},	 /* S06_ILLUCFG 90 @ 60.24 MHz (LUT1) */
	{0x90A1, 0x0000},	 /* S06_SENSORCFG */
	{0x90A2, 0x0000},	 /* S06_ROCFG (Reconfig forbidden) */
	{0x90A3, 0x5F2D},	 /* S07_EXPOTIMEMAX 1.060 ms @ 60.24 MHz */
	{0x90A4, 0x0000},	 /* S07_FRAMETIMEMIN */
	{0x90A5, 0x1040},	 /* S07_ILLUCFG 180 @ 60.24 MHz (LUT1) */
	{0x90A6, 0x0000},	 /* S07_SENSORCFG */
	{0x90A7, 0x0000},	 /* S07_ROCFG (Reconfig forbidden) */
	{0x90A8, 0x5F2D},	 /* S08_EXPOTIMEMAX 1.060 ms @ 60.24 MHz */
	{0x90A9, 0x0000},	 /* S08_FRAMETIMEMIN */
	{0x90AA, 0x1060},	 /* S08_ILLUCFG 270 @ 60.24 MHz (LUT1) */
	{0x90AB, 0x0000},	 /* S08_SENSORCFG */
	/* S08_ROCFG (Reconfig & Usecase Switch allowed) */
	{0x90AC, 0xC000},
	{0x91C0, 0x0592},	 /* CSICFG */
	{0x91C1, 0xDF00},	 /* ROICOL */
	{0x91C2, 0xAB00},	 /* ROIROW */
	{0x91C3, 0x0508},	 /* ROS */
	{0x91C4, 0x0008},	 /* EXPCFG0 */
	{0x91C5, 0x0020},	 /* EXPCFG1 */
	{0x91C6, 0x8008},	 /* PSOUT (SE enabled) */
	{0x91CF, 0x0009},	 /* MB_CFG0 (9 sequences to execute) */
	{0x91D3, 0x0C35},	 /* MB0_FRAMETIME (15 fps) */
	{0x91DB, 0x0008},	 /* MBSEQ0_CFG0 (MB0 Repetitions = 1) */
	/* PLL_MODLUT0_CFG0 (for f_illu = 80.32 MHz [f_mod = 321.28 MHz]) */
	{0x91EA, 0x1EA1},
	/* PLL_MODLUT0_CFG1 (for f_illu = 80.32 MHz [f_mod = 321.28 MHz]) */
	{0x91EB, 0xBF26},
	/* PLL_MODLUT0_CFG2 (for f_illu = 80.32 MHz [f_mod = 321.28 MHz]) */
	{0x91EC, 0x0008},
	/* PLL_MODLUT0_CFG3 (for f_illu = 80.32 MHz [f_mod = 321.28 MHz]) */
	{0x91ED, 0x0D01},
	/* PLL_MODLUT0_CFG4 (for f_illu = 80.32 MHz [f_mod = 321.28 MHz]) */
	{0x91EE, 0x5555},
	/* PLL_MODLUT0_CFG5 (for f_illu = 80.32 MHz [f_mod = 321.28 MHz]) */
	{0x91EF, 0x02F5},
	/* PLL_MODLUT0_CFG6 (for f_illu = 80.32 MHz [f_mod = 321.28 MHz]) */
	{0x91F0, 0x0009},
	/* PLL_MODLUT0_CFG7 (for f_illu = 80.32 MHz [f_mod = 321.28 MHz]) */
	{0x91F1, 0x0031},
	/* PLL_MODLUT1_CFG0 (for f_illu = 60.24 MHz [f_mod = 240.96 MHz]) */
	{0x91F2, 0x16A1},
	/* PLL_MODLUT1_CFG1 (for f_illu = 60.24 MHz [f_mod = 240.96 MHz]) */
	{0x91F3, 0x1EB8},
	/* PLL_MODLUT1_CFG2 (for f_illu = 60.24 MHz [f_mod = 240.96 MHz]) */
	{0x91F4, 0x0005},
	/* PLL_MODLUT1_CFG3 (for f_illu = 60.24 MHz [f_mod = 240.96 MHz]) */
	{0x91F5, 0x0C01},
	/* PLL_MODLUT1_CFG4 (for f_illu = 60.24 MHz [f_mod = 240.96 MHz]) */
	{0x91F6, 0xFCBF},
	/* PLL_MODLUT1_CFG5 (for f_illu = 60.24 MHz [f_mod = 240.96 MHz]) */
	{0x91F7, 0x04A7},
	/* PLL_MODLUT1_CFG6 (for f_illu = 60.24 MHz [f_mod = 240.96 MHz]) */
	{0x91F8, 0x000D},
	/* PLL_MODLUT1_CFG7 (for f_illu = 60.24 MHz [f_mod = 240.96 MHz]) */
	{0x91F9, 0x0022},
	{0x9220, 0x0003},	 /* SENSOR_LENGTH_CODE0 (length = 4) */
	{0x9221, 0x000C},	 /* SENSOR_CODE0_0 (code = 1100) */
	{0x9244, 0x0003},	 /* ILLU_LENGTH_CODE0 (length = 4) */
	{0x9245, 0x0008},	 /* ILLU_CODE0_0 (code = 1000) */
	/* DIGCLKDIV_S0_PLL0 (for f_illu = 80.32 MHz [f_mod = 321.28 MHz]) */
	{0x9268, 0x0001},
	/* DIGCLKDIV_S0_PLL1 (for f_illu = 60.24 MHz [f_mod = 240.96 MHz]) */
	{0x9269, 0x0001},
	/* DLLREGDELAY_S0_PLL0 (for f_illu = 80.32 MHz [f_mod = 321.28 MHz]) */
	{0x9278, 0x0B01},
	/* DLLREGDELAY_S0_PLL1 (for f_illu = 60.24 MHz [f_mod = 240.96 MHz]) */
	{0x9279, 0x0001},
	{0x9288, 0x0242},	 /* Filter Stage initializaiton Value 611 */
	{0x9289, 0x0246},	 /* Filter Stage initializaiton Value 610 */
	{0x928A, 0x0246},	 /* Filter Stage initializaiton Value 69 */
	{0x928B, 0x0242},	 /* Filter Stage initializaiton Value 68 */
	{0x928C, 0x0246},	 /* Filter Stage initializaiton Value 67 */
	{0x928D, 0x0246},	 /* Filter Stage initializaiton Value 66 */
	{0x928E, 0x0242},	 /* Filter Stage initializaiton Value 65 */
	{0x928F, 0x0246},	 /* Filter Stage initializaiton Value 64 */
	{0x9290, 0x0246},	 /* Filter Stage initializaiton Value 63 */
	{0x9291, 0x0242},	 /* Filter Stage initializaiton Value 62 */
	{0x9292, 0x0246},	 /* Filter Stage initializaiton Value 61 */
	{0x9293, 0x0246},	 /* Filter Stage initializaiton Value 60 */
	{0x9294, 0x011C},	 /* Filter Stage initializaiton Value 57 */
	{0x9295, 0x012A},	 /* Filter Stage initializaiton Value 56 */
	{0x9296, 0x011C},	 /* Filter Stage initializaiton Value 55 */
	{0x9297, 0x012A},	 /* Filter Stage initializaiton Value 54 */
	{0x9298, 0x011C},	 /* Filter Stage initializaiton Value 53 */
	{0x9299, 0x012A},	 /* Filter Stage initializaiton Value 52 */
	{0x929A, 0x011C},	 /* Filter Stage initializaiton Value 51 */
	{0x929B, 0x012A},	 /* Filter Stage initializaiton Value 50 */
	{0x929C, 0x008E},	 /* Filter Stage initializaiton Value 47 */
	{0x929D, 0x00AA},	 /* Filter Stage initializaiton Value 46 */
	{0x929E, 0x0080},	 /* Filter Stage initializaiton Value 45 */
	{0x929F, 0x0080},	 /* Filter Stage initializaiton Value 44 */
	{0x92A0, 0x00B8},	 /* Filter Stage initializaiton Value 43 */
	{0x92A1, 0x0080},	 /* Filter Stage initializaiton Value 42 */
	{0x92A2, 0x0080},	 /* Filter Stage initializaiton Value 41 */
	{0x92A3, 0x0080},	 /* Filter Stage initializaiton Value 40 */
	{0x92A4, 0x0040},	 /* Filter Stage initializaiton Value 37 */
	{0x92A5, 0x0040},	 /* Filter Stage initializaiton Value 36 */
	{0x92A6, 0x0040},	 /* Filter Stage initializaiton Value 35 */
	{0x92A7, 0x0040},	 /* Filter Stage initializaiton Value 34 */
	{0x92A8, 0x0040},	 /* Filter Stage initializaiton Value 33 */
	{0x92A9, 0x0040},	 /* Filter Stage initializaiton Value 32 */
	{0x92AA, 0x0040},	 /* Filter Stage initializaiton Value 31 */
	{0x92AB, 0x0040},	 /* Filter Stage initializaiton Value 30 */
	{0x92AC, 0x0040},	 /* Filter Stage initializaiton Value 27 */
	{0x92AD, 0x0040},	 /* Filter Stage initializaiton Value 26 */
	{0x92AE, 0x0040},	 /* Filter Stage initializaiton Value 25 */
	{0x92AF, 0x0040},	 /* Filter Stage initializaiton Value 24 */
	{0x92B0, 0x0040},	 /* Filter Stage initializaiton Value 23 */
	{0x92B1, 0x0040},	 /* Filter Stage initializaiton Value 22 */
	{0x92B2, 0x0040},	 /* Filter Stage initializaiton Value 21 */
	{0x92B3, 0x0040},	 /* Filter Stage initializaiton Value 20 */
	{0x92B4, 0x0040},	 /* Filter Stage initializaiton Value 17 */
	{0x92B5, 0x0040},	 /* Filter Stage initializaiton Value 16 */
	{0x92B6, 0x0040},	 /* Filter Stage initializaiton Value 15 */
	{0x92B7, 0x0040},	 /* Filter Stage initializaiton Value 14 */
	{0x92B8, 0x0040},	 /* Filter Stage initializaiton Value 13 */
	{0x92B9, 0x0040},	 /* Filter Stage initializaiton Value 12 */
	{0x92BA, 0x0040},	 /* Filter Stage initializaiton Value 11 */
	{0x92BB, 0x0040},	 /* Filter Stage initializaiton Value 10 */
	{0x92BC, 0x0040},	 /* Filter Stage initializaiton Value 07 */
	{0x92BD, 0x0040},	 /* Filter Stage initializaiton Value 06 */
	{0x92BE, 0x0040},	 /* Filter Stage initializaiton Value 05 */
	{0x92BF, 0x0040},	 /* Filter Stage initializaiton Value 04 */
	{0x92C0, 0x0040},	 /* Filter Stage initializaiton Value 03 */
	{0x92C1, 0x0040},	 /* Filter Stage initializaiton Value 02 */
	{0x92C2, 0x0040},	 /* Filter Stage initializaiton Value 01 */
	{0x92C3, 0x0040},	 /* Filter Stage initializaiton Value 00 */
	{0x92C4, 0x5633},	 /* TH0 */
	{0x92C5, 0x9175},	 /* TH1 */
	{0x92C6, 0xC5AB},	 /* TH2 */
	{0x92C7, 0xF5DD},	 /* TH3 */
	{0x92C8, 0x4633},	 /* TH4 */
	{0x92C9, 0x6857},	 /* TH5 */
	{0x92CA, 0x8677},	 /* TH6 */
	{0x92CB, 0x0095},	 /* TH7 */
	{0x92CC, 0x2B1F},	 /* TH8 */
	{0x92CD, 0x3F35},	 /* TH9 */
	{0x92CE, 0x5249},	 /* TH10 */
	{0x92CF, 0x005B},	 /* TH11 */
	{0x92D0, 0x1A13},	 /* TH12 */
	{0x92D1, 0x1F1E},	 /* TH13 */
	{0x92D2, 0x2120},	 /* TH14 */
	{0x92D3, 0x0022},	 /* TH15 */
	{0x92D4, 0x322A},	 /* TH16 */
	{0x92D5, 0x534B},	 /* TH17 */
	{0x92D6, 0x735B},	 /* TH18 */
	{0x92D7, 0x007B},	 /* TH19 */
	{0x92D8, 0x4E35},	 /* TH20 */
	{0x92D9, 0x7C64},	 /* TH21 */
	{0x92DA, 0xA891},	 /* TH22 */
	{0x92DB, 0x00BD},	 /* TH23 */
	{0x92DC, 0x100B},	 /* TH24 */
	{0x92DD, 0x1915},	 /* TH25 */
	{0x92DE, 0x221E},	 /* TH26 */
	{0x92DF, 0x2C27},	 /* TH27 */
	{0x92E0, 0x3530},	 /* TH28 */
	{0x92E1, 0x003A},	 /* TH29 */
	{0x92E2, 0x0500},	 /* CMPMUX */
	{0x92E3, 0x08D8},	 /* HH */
	{0x92E4, 0x0872},	 /* HL */
	{0x92E5, 0x084A},	 /* LH */
	{0x92E6, 0x0835},	 /* LL */
	{0x92E7, 0x0001},	 /* PlausiCheck ; Idle Mode enable */
	{0x92E8, 0x00D0},	 /* Idle Counter */
	{0x92E9, 0x0300},	 /* Module ID 0 */
	{0x92EA, 0x0000},	 /* Module ID 1 */
	{0x92EB, 0x0000},	 /* Module ID 2 */
	{0x92EC, 0x0000},	 /* Module ID 3 */
	{0x92ED, 0x0000},	 /* Module ID 4 */
	{0x92EE, 0x0000},	 /* Module ID 5 */
	{0x92EF, 0x0000},	 /* Module ID 6 */
	{0x92F0, 0x002E},	 /* Module ID 7 */
	{0x92F1, 0x960E},	 /* CRC */
	{0x9401, 0x0002},	 /* Mode */
	{0xA001, 0x0007},	 /* DMUX1 */
	{0xA008, 0x1513},	 /* PADGPIOCFG1 */
	{0xA00C, 0x0135},	 /* PADMODCFG */
	{0xA039, 0x1AA1},	 /* PLL_SYSLUT_CFG0 (for fref = 24.00 MHz) */
	{0xA03A, 0xAAAB},	 /* PLL_SYSLUT_CFG1 (for fref = 24.00 MHz) */
	{0xA03B, 0x000A},	 /* PLL_SYSLUT_CFG2 (for fref = 24.00 MHz) */
	{0xA03C, 0x0000},	 /* PLL_SYSLUT_CFG3 (for fref = 24.00 MHz) */
	{0xA03D, 0x03C0},	 /* PLL_SYSLUT_CFG4 (for fref = 24.00 MHz) */
	{0xA03E, 0x0000},	 /* PLL_SYSLUT_CFG5 (for fref = 24.00 MHz) */
	{0xA03F, 0x0017},	 /* PLL_SYSLUT_CFG6 (for fref = 24.00 MHz) */
	{0x9400, 0x0001},	 /* stream on */
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
	struct sensor_info *info = to_state(sd);

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

	info->gain = gain_val;

	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	return 0;
}

static int sensor_s_fps(struct v4l2_subdev *sd,
			struct sensor_fps *fps)
{
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

		vin_gpio_set_status(sd, RESET, 1);

		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);

		vin_set_pmu_channel(sd, CAMERAVDD, ON);
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
		usleep_range(1000, 1200);

		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(3000, 3200);

		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(20000, 22000);

		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(20000, 22000);

		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");
		cci_lock(sd);

		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);

		vin_set_mclk(sd, OFF);

		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, CAMERAVDD, OFF);

		vin_gpio_set_status(sd, RESET, 0);

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
	data_type rdval = 0;

	sensor_read(sd, 0xA0A5, &rdval);
	if (rdval != V4L2_IDENT_SENSOR)
		return -ENODEV;

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
	info->width = 224;
	info->height = 1557;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->exp = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = SENSOR_FRAME_RATE;

	info->preview_first_flag = 1;

	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);

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
		.mbus_code = MEDIA_BUS_FMT_SGRBG12_1X12,
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
		.width      = 224,
		.height     = 1557,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2496,
		.vts        = 1308,
		.pclk       = 48 * 1000 * 1000,
		.fps_fixed  = SENSOR_FRAME_RATE,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 1900 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 10 << 4,
		.regs       = NULL,
		.regs_size  = 0,
		.set_size   = NULL,
	},
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_CSI2_DPHY;
	cfg->flags = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0;

	return 0;
}

static int sensor_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *info = container_of(ctrl->handler,
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
	struct sensor_info *info = container_of(ctrl->handler,
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
	int ret;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

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
static struct cci_driver cci_drv = {
	.name = SENSOR_NAME,
	.addr_width = CCI_BITS_16,
	.data_width = CCI_BITS_16,
};

static int sensor_init_controls(struct v4l2_subdev *sd,
				const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	struct v4l2_ctrl *ctrl;
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 2);

	v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 16,
			  256 * 16, 1, 1 * 16);
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
	struct v4l2_subdev *sd = NULL;
	struct sensor_info *info = NULL;

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
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
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET2 |
				MIPI_NORMAL_MODE;
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
	return cci_dev_init_helper(&sensor_driver);
}

static __exit void exit_sensor(void)
{
	cci_dev_exit_helper(&sensor_driver);
}

VIN_INIT_DRIVERS(init_sensor);
module_exit(exit_sensor);
