/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for sc2315 Raw cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
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

MODULE_AUTHOR("myf");
MODULE_DESCRIPTION("A low-level driver for SC2315_MIPI sensors");
MODULE_LICENSE("GPL");

#define MCLK              (27*1000*1000)
#define V4L2_IDENT_SENSOR 0x2311

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

#define HDR_RATIO 16

/*
 * The SC2315_MIPI i2c address
 */
#define I2C_ADDR 0x60

#define SENSOR_NUM 0x2
#define SENSOR_NAME "sc2315_mipi"
#define SENSOR_NAME_2 "sc2315_mipi_2"

/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};

static struct regval_list sensor_1080p12b30_regs[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},

	{0x36e9, 0xa3},/* bypass pll1	20180830 */
	{0x36f9, 0x85},/* bypass pll2	20180830 */

	/* close mipi */
	/* 0x3018,0x1f, */
	/* 0x3019,0xff, */
	/* 0x301c,0xb4, */

	{0x320c, 0x05},
	{0x320d, 0x28},

	/* 0x320c,0x0a, */
	/* 0x320d,0x50, */


	{0x4509, 0x10},
	{0x4500, 0x39},
	{0x3907, 0x00},
	{0x3908, 0x44},


	{0x3633, 0x87},
	{0x3306, 0x7e},
	{0x330b, 0x00},

	{0x3635, 0x4c},
	{0x330e, 0x7a},
	/* 0x3621,0xb0, */
	{0x3302, 0x1f}, /* 3302 need be odd why????  3366  3302   3621 */
	{0x3e01, 0x8c},
	{0x3e02, 0x80},
	{0x3e09, 0x1f},
	{0x3e08, 0x3f},
	{0x3e06, 0x03},

	{0x337f, 0x03}, /* new auto precharge  330e in 3372   [7:6] 11: close div_rst 00:open div_rst */
	{0x3368, 0x04},
	{0x3369, 0x00},
	{0x336a, 0x00},
	{0x336b, 0x00},
	{0x3367, 0x08},
	{0x330e, 0x30},
	{0x3320, 0x06}, /*  New ramp offset timing */
	/* 0x3321,0x06, */
	{0x3326, 0x00},
	{0x331e, 0x11},
	{0x331f, 0x21},
	{0x3303, 0x20},
	{0x3309, 0x30},


	{0x4501, 0xc4},
	{0x3e06, 0x00},
	{0x3e08, 0x03},
	{0x3e09, 0x10},

	{0x3366, 0x7c}, /*  div_rst gap */

	/* noise */
	{0x3622, 0x02},
	{0x3633, 0x63},

	/* fpn */
	{0x3038, 0x88},
	{0x3635, 0x45},
	{0x363b, 0x04},
	{0x363c, 0x06},
	{0x363d, 0x05},


	/* 1021 */
	{0x3633, 0x23},

	{0x3301, 0x10},
	{0x3306, 0x58},
	{0x3622, 0x06},/* blksun */
	{0x3631, 0x88},
	{0x3630, 0x38},
	{0x3633, 0x22},

	/* noise test */
	/* 0x3e08,0x3f, */
	/* 0x3e09,0x1f, */
	/* 0x3e06,0x0f, */
	/* 0x5000,0x00, */
	/* 0x5002,0x00, */
	/* 0x3907,0x08, */


	/* mipi */
	{0x3018, 0x33},/* [7:5] lane_num-1 */
	{0x3031, 0x0c},/* [3:0] bitmode */
	{0x3037, 0x40},/* [6:5] bitsel  40:12bit */
	{0x3001, 0xFE},/* [0] c_y */

	/* lane_dis of lane3~8 */
	/* 0x3018,0x12, */
	/* 0x3019,0xfc, */

	{0x4603, 0x00},/* [0] data_fifo mipi mode */
	{0x4837, 0x35},/* [7:0] pclk period * 2 */
	/* 0x4827,0x48,/* [7:0] hs_prepare_time[7:0] */

	{0x36e9, 0x83},
	/* 0x36ea,0x2a, */
	{0x36eb, 0x0f},
	{0x36ec, 0x1f},


	/* 0x3e08,0x03, */
	/* 0x3e09,0x10, */

	{0x303f, 0x01},

	/* 1024 */

	{0x330b, 0x20},

	/* 20170307 */
	{0x3640, 0x00},
	{0x3308, 0x10},
	{0x3637, 0x0a},
	{0x3e09, 0x20}, /* 3f for 2x fine gain */
	{0x363b, 0x08},


	/* 20170307 */
	{0x3637, 0x09}, /*  ADC range: 14.8k fullwell  blc target : 0.9k   output fullwell: 13.9k (5fps 27C  linear fullwell is 14.5K) */
	{0x3638, 0x14},
	{0x3636, 0x65},
	{0x3907, 0x01},
	{0x3908, 0x01},

	{0x3320, 0x01}, /* ramp */
	{0x331e, 0x15},
	{0x331f, 0x25},
	{0x3366, 0x80},
	{0x3634, 0x34},

	{0x57a4, 0xf0}, /* default c0, */

	{0x3635, 0x41}, /* fpn */

	{0x3e02, 0x30}, /* minimum exp 3? debug */
	{0x3333, 0x30}, /* col fpn  G >br  ? */
	{0x331b, 0x83},
	{0x3334, 0x40},

	/* 30fps */
	{0x320c, 0x04}, /*  hts 1100*2 */
	{0x320d, 0x4c},
	{0x3306, 0x6c},
	{0x3638, 0x17},
	{0x330a, 0x01},
	{0x330b, 0x14},

	{0x3302, 0x10},
	{0x3308, 0x08},
	{0x3303, 0x18},
	{0x3309, 0x18},
	{0x331e, 0x11},
	{0x331f, 0x11},

	/* sram write */
	{0x3f00, 0x0d}, /* [2]   hts/2-4 */
	{0x3f04, 0x02},
	{0x3f05, 0x22},

	{0x3622, 0xe6},
	{0x3633, 0x22},
	{0x3630, 0xc8},
	{0x3301, 0x10},

	/* reduce samp timing,add 330b/blksun margin */

	/* 0319 */
	{0x36e9, 0xa3},
	{0x36eb, 0x0b},
	{0x36ec, 0x0f},
	{0x3638, 0x27},

	{0x33aa, 0x00}, /* power save mode */
	{0x3624, 0x02},
	{0x3621, 0xac},


	/* aec  analog gain: 0x570 (15.75 x 2.7=41.85)   2xdgain :0xae0  4xdgain : 0x15c0  8xdgain : 0x2b80 16xdgain : 0x5700 3e0a 3e0b real ana gain  3e2c 3e2d real digital fine gain */

	/* 0x3e03,0x03, */
	/* 0x3e14,0xb0, /* [7]:1 ana fine gain double 20~3f */
	/* 0x3e1e,0xb5, /* [7]:1 open DCG function in 0x3e03=0x03 [6]:0 DCG >2   [2] 1: dig_fine_gain_en [1:0] max fine gain  01: 3f */
	/* 0x3e0e,0x66, /* [7:3] fine gain to compsensate 2.4x DCGgain  5 : 2.3125x  6:2.375x  [2]:1 DCG gain between sa1gain 2~4  [1]:1 dcg gain in 0x3e08[5] */


	{0x4509, 0x40},

	/* 321 */
	{0x391e, 0x00},
	{0x391f, 0xc0},
	{0x3635, 0x45},
	{0x336c, 0x40},

	/* 0324 */
	{0x3621, 0xae},
	{0x3623, 0x08},

	/* 0326 rownoise */
	{0x36fa, 0xad}, /* charge pump */
	{0x3634, 0x44},

	/* 0327 */
	{0x3621, 0xac}, /* fifo delay */
	{0x4500, 0x59},
	{0x3623, 0x18}, /* for more grp rdout setup margin */

	/* sram write */
	{0x3f08, 0x04},
	{0x3f00, 0x0d}, /* [2]   hts/2-4-{3f08} */
	{0x3f04, 0x02}, /* 0321 */
	{0x3f05, 0x1e}, /* 0321 */

	/* 0329 */
	{0x336c, 0x42}, /* recover read timing */

	/* 20180509 */
	{0x5000, 0x06},/* dpc enable */
	{0x5780, 0x7f},/* auto blc setting */
	{0x57a0, 0x00},	/* gain0 = 2x	0x0710 ~ 0x071f */
	{0x57a1, 0x71},
	{0x57a2, 0x01},	/* gain1 = 8x	0x1f10 ~ 0x1f1f */
	{0x57a3, 0xf1},
	{0x5781, 0x06},	/* white	1x */
	{0x5782, 0x04},	/* 2x */
	{0x5783, 0x02},	/* 8x */
	{0x5784, 0x01},	/* 128x */
	{0x5785, 0x16},	/* black	1x */
	{0x5786, 0x12},	/* 2x */
	{0x5787, 0x08},	/* 8x */
	{0x5788, 0x02},	/* 128x */

	/* 20180509 high temperature */
	{0x3933, 0x28},
	{0x3934, 0x0a},
	{0x3940, 0x1b},
	{0x3941, 0x40},
	{0x3942, 0x08},
	{0x3943, 0x0e},

	/* 20180604 */
	{0x3624, 0x47},
	{0x3621, 0xac},

	/* 0612 */
	{0x3637, 0x08},
	{0x3638, 0x25},
	{0x3635, 0x40},
	{0x363b, 0x08},
	{0x363c, 0x05},
	{0x363d, 0x05},

	/* 20180702 */
	{0x3802, 0x01},

	/* 0705 */
	{0x3324, 0x02}, /* falling edge: ramp_offset_en cover ramp_integ_en */
	{0x3325, 0x02},
	{0x333d, 0x08}, /* count_div_rst_width */
	{0x36fa, 0xa8},
	{0x3314, 0x04},

	{0x3802, 0x01},

	/* 20180716 */
	/* 0x3222,0x47,/* debug:The last few columns can't be read */
	{0x3213, 0x02},
	{0x3207, 0x49},
	{0x3205, 0x93},
	{0x3208, 0x07},/* 1936x1096 */
	{0x3209, 0x90},
	{0x320a, 0x04},
	{0x320b, 0x48},
	{0x3211, 0x00},
	/* 0x3e03,0x03,/* gain map: 0x03 mode */
	{0x3e14, 0xb0}, /* [7]:1 ana fine gain double 20~3f */
	{0x3e1e, 0x35}, /* [7]:1 open DCG function in 0x3e03=0x03 [6]:0 DCG >2   [2] 1: dig_fine_gain_en [1:0] max fine gain  01: 3f */
	{0x3e0e, 0x66}, /* [7:3] fine gain to compsensate 2.4x DCGgain  5 : 2.3125x  6:2.375x  [2]:1 DCG gain between sa1gain 2~4  [1]:1 dcg gain in 0x3e08[5] */


	/* 20180730 */
	{0x6000, 0x00},
	{0x6002, 0x00},
	{0x301c, 0x78},/* close dvp */

	/* 20180822 */
	{0x3037, 0x42},/* [3:0] pump div	range [10M,20M],sclk=74.25/2=37.125M,div=2-->sclk/2=18.5625M,duty cycle-->even number */
	{0x3038, 0x22},/* [7:4]ppump & [3:0]npump */
	{0x3632, 0x18},/* [5:4]idac driver */
	{0x5785, 0x40},/* black	point 1x */

	/* 20180824 */
	{0x4809, 0x01},/* mipi first frame, lp status */

	/* 20181120	modify analog fine gain */
	{0x3637, 0x10},
	/* 20181120 */
	{0x5000, 0x06},/* dpc enable */
	{0x5780, 0x7f},/* auto blc setting */
	{0x57a0, 0x00},	/* gain0 = 2x	0x0740~0x077f */
	{0x57a1, 0x74},
	{0x57a2, 0x01},	/* gain1 = 8x	0x1f40~0x1f7f */
	{0x57a3, 0xf4},
	{0x5781, 0x06},	/* white	1x */
	{0x5782, 0x04},	/* 2x */
	{0x5783, 0x02},	/* 8x */
	{0x5784, 0x01},	/* 128x */
	{0x5785, 0x16},	/* black	1x */
	{0x5786, 0x12},	/* 2x */
	{0x5787, 0x08},	/* 8x */
	{0x5788, 0x02},	/* 128x */

	{0x4501, 0xb4},/* reduce bit */
	{0x3637, 0x20},
	{0x4509, 0x20},/* blc quantification	/* 20181206 */
	{0x331f, 0x29},
	{0x3309, 0x30},
	{0x330a, 0x00},
	{0x330b, 0xc8}, /* [aa,e0] */
	{0x3306, 0x60}, /* [34,78] */
	{0x330e, 0x28},
	/* 0x3933,0x30,/* 20181206 */
	/* 0x3942,0x10, */

	/* 20181120 digital logic */
	/* 3301 auto logic read 0x3373 for auto value */
	{0x3364, 0x1d},/* [4] fine gain op 1~20--3f 0~10--1f [4] ana dithring en */
	{0x33b6, 0x07},/* gain0 when dcg off	gain<dcg */
	{0x33b7, 0x07},/* gain1 when dcg off */
	{0x33b8, 0x10},/* sel0 when dcg off */
	{0x33b9, 0x10},/* sel1 when dcg off */
	{0x33ba, 0x10},/* sel2 when dcg off */
	{0x33bb, 0x07},/* gain0 when dcg on		gain>=dcg */
	{0x33bc, 0x07},/* gain1 when dcg on */
	{0x33bd, 0x20},/* sel0 when dcg on */
	{0x33be, 0x20},/* sel1 when dcg on */
	{0x33bf, 0x20},/* sel2 when dcg on */
	/* 3622 auto logic read 0x3680 for auto value */
	{0x360f, 0x05},/* [0] 3622 auto en */
	{0x367a, 0x40},/* gain0 */
	{0x367b, 0x40},/* gain1 */
	{0x3671, 0xf6},/* sel0 */
	{0x3672, 0x16},/* sel1 */
	{0x3673, 0x16},/* sel2 */
	/* 3630 auto logic read 0x3681 for auto value */
	{0x366e, 0x04},/* [2] fine gain op 1~20--3f 0~10--1f */
	{0x3670, 0x4a},/* [1] 3630 auto en, [3] 3633 auto en, [6] 363a auto en */
	{0x367c, 0x40},/* gain0  3e08[5:2] 1000 */
	{0x367d, 0x58},/* gain1 3e08[5:2] 1011 */
	{0x3674, 0xc8},/* sel0 */
	{0x3675, 0x54},/* sel1 */
	{0x3676, 0x18},/* sel2 */
	/* 3633 auto logic read 0x3682 for auto value */
	{0x367e, 0x40},/* gain0  3e08[5:2] 1000 */
	{0x367f, 0x58},/* gain1  3e08[5:2] 1011 */
	{0x3677, 0x22},/* sel0 */
	{0x3678, 0x33},/* sel1 */
	{0x3679, 0x44},/* sel2 */
	/* 363a auto logic read 0x3685 for auto value */
	{0x36a0, 0x58},/* gain0  3e08[5:2] 1011 */
	{0x36a1, 0x78},/* gain1  3e08[5:2] 1111 */
	{0x3696, 0x83},/* sel0 */
	{0x3697, 0x87},/* sel1 */
	{0x3698, 0x9f},/* sel2 */

	/* 20181122 */
	{0x3637, 0x17},/* fullwell 8.6k */

	/* sysclk=74.25/2=37.125 */
	/* countclk=445.5	12:1 */
	/* 0x3303,0x18,	/* [hl,to][1,1a,2e] */
	/* 0x3309,0x30,	/* [hl,to][1,32,46] */
	/* 0x330b,0xc8,	/* [bs,to][1,b0,f8] */
	/* 0x3306,0x60,	/* [hl,bs][1,4a,76] */
	/* 0x3301,0x10,0x20,/* [hl,to][1,7,28] */
	/* 0x330e,0x30,	/* [lag][8] */
	/* 0x3367,0x08, */
	/* fullwell 8.6k */
	/* noise 0.98e */

	/* 20181123 window	1920x1080 centered */
	{0x3200, 0x00},
	{0x3201, 0x04},
	{0x3202, 0x00},
	{0x3203, 0x04},
	{0x3204, 0x07},
	{0x3205, 0x8b},
	{0x3206, 0x04},
	{0x3207, 0x43},
	{0x3208, 0x07},
	{0x3209, 0x80},
	{0x320a, 0x04},
	{0x320b, 0x38},
	{0x3211, 0x04},
	{0x3213, 0x04},

	/* 20181205	debug: flip aec bug */
	{0x3380, 0x1b},
	{0x3341, 0x07},/* 3318[3:0] + 2 */
	{0x3343, 0x03},/* 3318[3:0] -2 */
	{0x3e25, 0x03},/* blc dithering(analog fine gain) */
	{0x3e26, 0x40},

	/* 20190215 */
	{0x3905, 0xd8},  /* one channel blc */

	/* 20190725 */
	{0x391d, 0x01},/* blc dithering always on */
	{0x3902, 0xc5},/* mirror & flip --> trig blc */
	{0x3905, 0xd8},/* one channel blc */
	{0x3625, 0x0a},/* smear */

	/* init */
	{0x3e00, 0x00},/* max_exposure = vts*2-6; min_exposure = 3;	20180712 */
	{0x3e01, 0x8c},
	{0x3e02, 0x40},
	{0x3e03, 0x0b},/* gain map 0x0b mode	gain=1x */
	{0x3e06, 0x00},
	{0x3e07, 0x80},
	{0x3e08, 0x03},
	{0x3e09, 0x40},

	{0x36e9, 0x23},/* enable pll1	20180830 */
	{0x36f9, 0x05},/* enable pll2	20180830 */

	{0x0100, 0x01},
};


static struct regval_list sensor_1080p10b30_HDR_regs[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},

	{0x36e9, 0xa6},/* bypass pll1	20180830 */
	{0x36f9, 0x85},/* bypass pll2	20180830 */

	/* close mipi */
	/* 0x3018,0x1f, */
	/* 0x3019,0xff, */
	/* 0x301c,0xb4, */

	{0x320c, 0x05},
	{0x320d, 0x28},

	/* 0x320c,0x0a, */
	/* 0x320d,0x50, */


	{0x4509, 0x10},
	{0x4500, 0x39},
	{0x3907, 0x00},
	{0x3908, 0x44},
	{0x3633, 0x87},
	{0x3306, 0x7e},
	{0x330b, 0x00},
	{0x3635, 0x4c},
	{0x330e, 0x7a},
	/* 0x3621,0xb0, */
	{0x3302, 0x1f}, /* 3302 need be odd   3366  3302   3621 */

	{0x3e01, 0x8c},
	{0x3e02, 0x80},
	{0x3e09, 0x1f},
	{0x3e08, 0x3f},
	{0x3e06, 0x03},

	{0x337f, 0x03}, /* new auto precharge  330e in 3372   [7:6] 11: close div_rst 00:open div_rst */
	{0x3368, 0x04},
	{0x3369, 0x00},
	{0x336a, 0x00},
	{0x336b, 0x00},
	{0x3367, 0x08},
	{0x330e, 0x30},

	{0x3320, 0x06}, /*  New ramp offset timing */
	/* 0x3321,0x06, */
	{0x3326, 0x00},
	{0x331e, 0x11},
	{0x331f, 0x21},
	{0x3303, 0x20},
	{0x3309, 0x30},


	{0x4501, 0xc4},
	{0x3e06, 0x00},
	{0x3e08, 0x03},
	{0x3e09, 0x10},
	{0x3366, 0x7c}, /*  div_rst gap */

	/* noise */
	{0x3622, 0x02},
	{0x3633, 0x63},

	/* fpn */
	{0x3038, 0x88},
	{0x3635, 0x45},
	{0x363b, 0x04},
	{0x363c, 0x06},
	{0x363d, 0x05},


	/* 1021 */
	{0x3633, 0x23},
	{0x3301, 0x10},
	{0x3306, 0x58},
	{0x3622, 0x06},/* blksun */
	{0x3631, 0x88},
	{0x3630, 0x38},
	{0x3633, 0x22},

	/* noise test */
	/* 0x3e08,0x3f, */
	/* 0x3e09,0x1f, */
	/* 0x3e06,0x0f, */
	/* 0x5000,0x00, */
	/* 0x5002,0x00, */
	/* 0x3907,0x08, */


	/* mipi */
	{0x3018, 0x33},/* [7:5] lane_num-1 */
	{0x3031, 0x0a},/* [3:0] bitmode */
	{0x3037, 0x20},/* [6:5] bitsel  40:12bit  20:10bit */
	{0x3001, 0xFE},/* [0] c_y */

	/* lane_dis of lane3~8 */
	/* 0x3018,0x12, */
	/* 0x3019,0xfc, */

	{0x4603, 0x00},/* [0] data_fifo mipi mode */
	{0x4837, 0x1a},/* [7:0] pclk period * 2  0321 */
	/* 0x4827,0x48,/* [7:0] hs_prepare_time[7:0] */

	{0x36e9, 0x83},
	/* 0x36ea,0x2a, */
	{0x36eb, 0x0f},
	{0x36ec, 0x1f},


	/* 0x3e08,0x03, */
	/* 0x3e09,0x10, */

	{0x303f, 0x01},

	/* 1024 */

	{0x330b, 0x20},

	/* 20170307 */
	{0x3640, 0x00},
	{0x3308, 0x10},
	{0x3637, 0x0a},
	{0x3e09, 0x20}, /* 3f for 2x fine gain */
	{0x363b, 0x08},


	/* 20170307 */
	{0x3637, 0x09}, /*  ADC range: 14.8k fullwell  blc target : 0.9k   output fullwell: 13.9k (5fps 27C  linear fullwell is 14.5K) */
	{0x3638, 0x14},
	{0x3636, 0x65},
	{0x3907, 0x01},
	{0x3908, 0x01},
	{0x3320, 0x01}, /* ramp */
	{0x331e, 0x15},
	{0x331f, 0x25},
	{0x3366, 0x80},
	{0x3634, 0x34},
	{0x57a4, 0xf0}, /* default c0, */
	{0x3635, 0x41}, /* fpn */
	{0x3e02, 0x30}, /* minimum exp 3? debug */
	{0x3333, 0x30}, /* col fpn  G >br  ? */
	{0x331b, 0x83},
	{0x3334, 0x40},

	/* 30fps */
	{0x320c, 0x04},
	{0x320d, 0x4c},
	{0x3306, 0x6c},
	{0x3638, 0x17},
	{0x330a, 0x01},
	{0x330b, 0x14},

	{0x3302, 0x10},
	{0x3308, 0x08},
	{0x3303, 0x18},
	{0x3309, 0x18},
	{0x331e, 0x11},
	{0x331f, 0x11},

	/* sram write */
	{0x3f00, 0x0d}, /* [2]   hts/2-4 */
	{0x3f04, 0x02}, /* 0321 */
	{0x3f05, 0x22}, /* 0321 */

	{0x3622, 0xe6},
	{0x3633, 0x22},
	{0x3630, 0xc8},
	{0x3301, 0x10},

	/* reduce samp timing,add 330b/blksun margin */

	/* 135M */
	{0x36e9, 0xa6},  /* 742.5Mbps x2 lane 148.5M sys clk   371.25M cntclk */
	{0x36ea, 0x35},
	{0x36eb, 0x0a},
	{0x36ec, 0x0e},
	{0x36fa, 0xb2},

	{0x320c, 0x04}, /*  hts 1100*2 */
	{0x320d, 0x4c},

	{0x3205, 0x89},
	{0x3f08, 0x04},

	{0x4501, 0xa4},

	{0x3303, 0x28},
	{0x3309, 0x48},
	{0x331e, 0x19},
	{0x331f, 0x39},
	{0x3306, 0x40},
	{0x330a, 0x00},
	{0x330b, 0xb0},
	{0x3308, 0x10},
	{0x3366, 0xc0},
	{0x3637, 0x24},
	{0x3638, 0x27},

	{0x3e01, 0x54},
	{0x3e02, 0x60},

	{0x33aa, 0x00}, /* power save mode */

	{0x3624, 0x02},
	{0x3621, 0xac},

	/* aec  analog gain: 0x570 (15.75 x 2.7=41.85)   2xdgain :0xae0  4xdgain : 0x15c0  8xdgain : 0x2b80 16xdgain : 0x5700 3e0a 3e0b real ana gain  3e2c 3e2d real digital fine gain */

	/* 0x3e03,0x03, */
	/* 0x3e14,0xb0, /* [7]:1 ana fine gain double 20~3f */
	/* 0x3e1e,0xb5, /* [7]:1 open DCG function in 0x3e03=0x03 [6]:0 DCG >2   [2] 1: dig_fine_gain_en [1:0] max fine gain  01: 3f */
	/* 0x3e0e,0x66, /* [7:3] fine gain to compsensate 2.4x DCGgain  5 : 2.3125x  6:2.375x  [2]:1 DCG gain between sa1gain 2~4  [1]:1 dcg gain in 0x3e08[5] */


	/* 0321 */
	{0x391e, 0x00},
	{0x391f, 0xc0},
	{0x3635, 0x45},
	{0x336c, 0x40},

	/* 0324 */
	{0x36fa, 0xaa},
	{0x330b, 0xa8},
	{0x3621, 0xae},
	{0x3623, 0x08},

	/* 0326 */
	{0x36fa, 0xad}, /* charge pump */
	{0x3634, 0x44},

	/* 0327 */
	{0x4500, 0x59}, /* dig delay */
	{0x3621, 0xac}, /* not retime error */
	{0x3623, 0x18}, /* for more grp rdout setup margin */

	/* sram write */
	{0x3f00, 0x0d}, /* [2]   hts/2-4-{3f08} */
	{0x3f04, 0x02}, /* 0321 */
	{0x3f05, 0x1e}, /* 0321 */

	/* 0329 */
	{0x336c, 0x42},  /* recover read timing */

	/* 20180509 */
	{0x5000, 0x06},/* dpc enable */
	{0x5780, 0x7f},/* auto blc setting */
	{0x57a0, 0x00},	/* gain0 = 2x	0x0710 ~ 0x071f */
	{0x57a1, 0x71},
	{0x57a2, 0x01},	/* gain1 = 8x	0x1f10 ~ 0x1f1f */
	{0x57a3, 0xf1},
	{0x5781, 0x06},	/* white	1x */
	{0x5782, 0x04},	/* 2x */
	{0x5783, 0x02},	/* 8x */
	{0x5784, 0x01},	/* 128x */
	{0x5785, 0x16},	/* black	1x */
	{0x5786, 0x12},	/* 2x */
	{0x5787, 0x08},	/* 8x */
	{0x5788, 0x02},	/* 128x */

	/* 20180509 high temperature */
	{0x3933, 0x28},
	{0x3934, 0x0a},
	{0x3940, 0x1b},
	{0x3941, 0x40},
	{0x3942, 0x08},
	{0x3943, 0x0e},

	/* 20180604 */
	{0x3624, 0x47},
	{0x3621, 0xac},

	/* 20180605 */
	{0x3222, 0x29},
	{0x3901, 0x02},


	/* 0612 */
	{0x3637, 0x20},
	{0x3638, 0x25},
	{0x3635, 0x40},
	{0x363b, 0x08},
	{0x363c, 0x05},
	{0x363d, 0x05},

	/* 20180702 */
	{0x3802, 0x01},

	/* 0705 */
	{0x3324, 0x02}, /* falling edge: ramp_offset_en cover ramp_integ_en */
	{0x3325, 0x02},
	{0x333d, 0x08}, /* count_div_rst_width */
	{0x36fa, 0xa8},
	{0x3314, 0x04},
	{0x3306, 0x48},

	/* 20180711 digital logic */
	/* 3301 auto logic read 0x3373 for auto value */
	{0x3364, 0x1d},/* [4] fine gain op 1~20--3f 0~10--1f [4] ana dithring en */
	{0x33b6, 0x07},/* gain0 when dcg off */
	{0x33b7, 0x07},/* gain1 when dcg off */
	{0x33b8, 0x10},/* sel0 when dcg off */
	{0x33b9, 0x30},/* sel1 when dcg off */
	{0x33ba, 0x30},/* sel2 when dcg off */
	{0x33bb, 0x07},/* gain0 when dcg on */
	{0x33bc, 0x07},/* gain1 when dcg on */
	{0x33bd, 0x30},/* sel0 when dcg on */
	{0x33be, 0x30},/* sel1 when dcg on */
	{0x33bf, 0x30},/* sel2 when dcg on */
	/* 3622 auto logic read 0x3680 for auto value */
	{0x360f, 0x05},/* [0] 3622 auto en */
	{0x367a, 0x08},/* gain0 */
	{0x367b, 0x08},/* gain1 */
	{0x3671, 0xf6},/* sel0 */
	{0x3672, 0x16},/* sel0 */
	{0x3673, 0x16},/* sel0 */
	/* 3630 auto logic read 0x3681 for auto value */
	{0x366e, 0x04},/* [2] fine gain op 1~20--3f 0~10--1f */
	{0x3670, 0x0a},/* [1] 3630 auto en [3] 3633 auto en */
	{0x367c, 0x08},/* gain0 */
	{0x367d, 0x08},/* gain1 */
	{0x3674, 0xc8},/* sel0 */
	{0x3675, 0x08},/* sel0 */
	{0x3676, 0x08},/* sel0 */
	/* 3633 auto logic read 0x3682 for auto value */
	{0x367e, 0x08},/* gain0 */
	{0x367f, 0x08},/* gain1 */
	{0x3677, 0x22},/* sel0 */
	{0x3678, 0x55},/* sel0 */
	{0x3679, 0x55},/* sel0 */
	{0x3802, 0x01},

	/* 20180716 */
	/* 0x3222,0x47,/* debug:The last few columns can't be read */
	{0x3213, 0x02},
	{0x3207, 0x49},
	{0x3205, 0x93},
	{0x3208, 0x07},/* 1936x1096 */
	{0x3209, 0x90},
	{0x320a, 0x04},
	{0x320b, 0x48},
	{0x3211, 0x00},
	/* 0x3e03,0x03, /* gain map: 0x03 mode */
	{0x3e14, 0xb0}, /* [7]:1 ana fine gain double 20~3f */
	{0x3e1e, 0x35}, /* [7]:1 open DCG function in 0x3e03=0x03 [6]:0 DCG >2   [2] 1: dig_fine_gain_en [1:0] max fine gain  01: 3f */
	{0x3e0e, 0x66}, /* [7:3] fine gain to compsensate 2.4x DCGgain  5 : 2.3125x  6:2.375x  [2]:1 DCG gain between sa1gain 2~4  [1]:1 dcg gain in 0x3e08[5] */


	/* 20180716 digital logic */
	/* 3301 auto logic read 0x3373 for auto value */
	{0x3364, 0x1d},/* [4] fine gain op 1~20--3f 0~10--1f [4] ana dithring en */
	{0x33b6, 0x07},/* gain0 when dcg off	gain<dcg */
	{0x33b7, 0x07},/* gain1 when dcg off */
	{0x33b8, 0x10},/* sel0 when dcg off */
	{0x33b9, 0x10},/* sel1 when dcg off */
	{0x33ba, 0x10},/* sel2 when dcg off */
	{0x33bb, 0x07},/* gain0 when dcg on		gain>=dcg */
	{0x33bc, 0x07},/* gain1 when dcg on */
	{0x33bd, 0x18},/* sel0 when dcg on */
	{0x33be, 0x18},/* sel1 when dcg on */
	{0x33bf, 0x18},/* sel2 when dcg on */
	/* 3622 auto logic read 0x3680 for auto value */
	{0x360f, 0x05},/* [0] 3622 auto en */
	{0x367a, 0x40},/* gain0 */
	{0x367b, 0x40},/* gain1 */
	{0x3671, 0xf6},/* sel0 */
	{0x3672, 0x16},/* sel1 */
	{0x3673, 0x16},/* sel2 */
	/* 3630 auto logic read 0x3681 for auto value */
	{0x366e, 0x04},/* [2] fine gain op 1~20--3f 0~10--1f */
	{0x3670, 0x4a},/* [1] 3630 auto en, [3] 3633 auto en, [6] 363a auto en */
	{0x367c, 0x40},/* gain0  3e08[5:2] 1000 */
	{0x367d, 0x58},/* gain1 3e08[5:2] 1011 */
	{0x3674, 0xc8},/* sel0 */
	{0x3675, 0x54},/* sel1 */
	{0x3676, 0x18},/* sel2 */
	/* 3633 auto logic read 0x3682 for auto value */
	{0x367e, 0x40},/* gain0  3e08[5:2] 1000 */
	{0x367f, 0x58},/* gain1  3e08[5:2] 1011 */
	{0x3677, 0x22},/* sel0 */
	{0x3678, 0x53},/* sel1 */
	{0x3679, 0x55},/* sel2 */
	/* 363a auto logic read 0x3685 for auto value */
	{0x36a0, 0x58},/* gain0  3e08[5:2] 1011 */
	{0x36a1, 0x78},/* gain1  3e08[5:2] 1111 */
	{0x3696, 0x9f},/* sel0 */
	{0x3697, 0x9f},/* sel1 */
	{0x3698, 0x9f},/* sel2 */

	/* 20180730 */
	{0x6000, 0x00},
	{0x6002, 0x00},
	{0x301c, 0x78},/* close dvp */

	/* 20180822 */
	{0x3037, 0x24},/* [3:0] npump2 div */
	{0x3038, 0x44},/* ppump & npump div */
	{0x3632, 0x18},/* idac driver */
	{0x5785, 0x40},/* black	point 1x */

	/* 20180824 */
	{0x4809, 0x01},/* mipi first frame, lp status */
	{0x330e, 0x20},

	/* 20180825 */
	/* 3635 auto logic read 0x3684 for auto value */
	{0x3670, 0x6a},/* [5] auto en */
	{0x369e, 0x01},/* gain0 < 0x0324,	0x369e[7]=0x3e06[0],0x369e[6:3]=0x3e08[5:2],0x369e[2:0]=0x3e09[4:2] */
	{0x369f, 0x01},/* gain1 < 0x0324 */
	{0x3693, 0x20},/* bypass txvdd */
	{0x3694, 0x40},/* enable txvdd */
	{0x3695, 0x40},/* enable txvdd */

	/* 20181120	modify analog fine gain */
	{0x3637, 0x10},
	{0x3636, 0x62},
	{0x3625, 0x01},

	/* 20181120 */
	/* 3635 auto logic read 0x3684 for auto value */
	{0x3670, 0x6a},/* [5] auto en */
	{0x369e, 0x40},/* gain0 < 0x2340,	0x369e[7]=0x3e06[0],0x369e[6:3]=0x3e08[5:2],0x369e[2:0]=0x3e09[4:2] */
	{0x369f, 0x40},/* gain1 < 0x2340 */
	{0x3693, 0x20},/* bypass txvdd */
	{0x3694, 0x40},/* enable txvdd */
	{0x3695, 0x40},/* enable txvdd */

	/* 20181120 */
	{0x5000, 0x06},/* dpc enable */
	{0x5780, 0x7f},/* auto blc setting */
	{0x57a0, 0x00},	/* gain0 = 2x	0x0740 ~ 0x077f */
	{0x57a1, 0x74},
	{0x57a2, 0x01},	/* gain1 = 8x	0x1f40 ~ 0x1f7f */
	{0x57a3, 0xf4},
	{0x5781, 0x06},	/* white	1x */
	{0x5782, 0x04},	/* 2x */
	{0x5783, 0x02},	/* 8x */
	{0x5784, 0x01},	/* 128x */
	{0x5785, 0x16},	/* black	1x */
	{0x5786, 0x12},	/* 2x */
	{0x5787, 0x08},	/* 8x */
	{0x5788, 0x02},	/* 128x */

	/* 20181122 */
	{0x3637, 0x0c},	/* fullwell 9k */
	{0x3638, 0x24},	/* ramp offset */

	/* sysclk=148.5/2=74.25 */
	/* countclk=445.5	6:1 */
	{0x331e, 0x21},	/* [ne,][5,1d] */
	{0x3303, 0x30},	/* [ne-hl,to][5,2c,4c] */
	/* 0x3309,0x48,	/* [hl,to][5,32,64] */
	{0x330b, 0xb4},	/* [bs,to][5,a4,d4]	[1,a4,d4] */
	{0x3306, 0x54},	/* [hl,bs][5,48,5c]	[1,48,5c] */
	/* 0x3301,0x10,0x18,/* [hl,to][5,10,1f--2c][1,f,1f--2c] */
	{0x330e, 0x30},	/* [lag][20] */
	/* 0x3367,0x08, */
	/* fullwell 9k */
	/* noise 1.03e */

	/* 20181123 window	1920x1080 centered */
	{0x3200, 0x00},
	{0x3201, 0x04},
	{0x3202, 0x00},
	{0x3203, 0x04},
	{0x3204, 0x07},
	{0x3205, 0x8b},
	{0x3206, 0x04},
	{0x3207, 0x43},
	{0x3208, 0x07},
	{0x3209, 0x80},
	{0x320a, 0x04},
	{0x320b, 0x38},
	{0x3211, 0x04},
	{0x3213, 0x04},

	/* 20181204 vc mode */
	{0x4816, 0x51},/* bit[4] */
	{0x3220, 0x51},/* bit[6] */
	{0x4602, 0x0f},/* bit[3:0] */
	{0x33c0, 0x05},/* bit[2] */
	{0x6000, 0x06},
	{0x6002, 0x06},
	{0x320e, 0x08},/* double vts  2250 */
	{0x320f, 0xca},

	/* 20181205	debug: flip aec bug */
	{0x3380, 0x1b},
	{0x3341, 0x07},/* 3318[3:0] + 2 */
	{0x3343, 0x03},/* 3318[3:0] -2 */
	{0x3e25, 0x03},/* blc dithering(analog fine gain) */
	{0x3e26, 0x40},
	{0x391d, 0x24},

	/* 20181225 */
	{0x3202, 0x00},/* x_start must be 0x00 */
	{0x3203, 0x00},
	{0x3206, 0x04},/* 1088	activeBoard=4 */
	{0x3207, 0x3f},

	/* 20190725 */
	{0x391d, 0x21},/* blc dithering always on */
	{0x3902, 0xc5},/* mirror & flip --> trig blc */
	{0x3905, 0xd8},/* one channel blc */
	{0x3625, 0x09},/* smear */
	/* 0x331e,0x21, */
	/* 0x331f,0x31, */
	/* 0x3303,0x30,/* 28,38 */
	/* 0x3309,0x40,/* 38,48 */
	/* 0x3306,0x54,/* [46,64] */
	/* 0x330b,0xb4,/* [a4,d5] */

	/* init */
	{0x3e00, 0x00},/* max long exposure = 0x107a */
	{0x3e01, 0x07},
	{0x3e02, 0xa0},
	{0x3e04, 0x10},/* max short exposure = 0x108 */
	{0x3e05, 0x80},
	{0x3e23, 0x00},/* max long exp : max short exp <= 16:1 */
	{0x3e24, 0x88},
	{0x3e03, 0x0b},/* gain map 0x0b mode	gain=1x */
	{0x3e06, 0x00},
	{0x3e07, 0x80},
	{0x3e08, 0x03},
	{0x3e09, 0x40},
	/* 0x3e03,0x03,/* gain map 0x03 mode	gain=1x */
	/* 0x3e1e,0xb5, */
	/* 0x3e06,0x00, */
	/* 0x3e07,0x80, */
	/* 0x3e08,0x00, */
	/* 0x3e09,0x40, */
	{0x3622, 0xf6}, /* 20180612 */
	{0x3633, 0x22},
	{0x3630, 0xc8},
	{0x3301, 0x10}, /* [6,18] */
	{0x363a, 0x83},
	{0x3635, 0x20},

	{0x36e9, 0x26},/* enable pll1	20180830 */
	{0x36f9, 0x05},/* enable pll2	20180830 */

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

static int sc2315_sensor_vts;
static int sc2315_sensor_svr;
static int shutter_delay = 1;
static int shutter_delay_cnt;
static int fps_change_flag;
static unsigned int short_exp_save;

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	data_type explow, exphigh;
	struct sensor_info *info = to_state(sd);
	if (info->isp_wdr_mode == ISP_DOL_WDR_MODE) {
		if (exp_val < 0x18 * HDR_RATIO)
			exp_val = 0x18 * HDR_RATIO;
		exphigh = (unsigned char) (0xff & (exp_val >> 7));
		explow = (unsigned char) (0xf0 & (exp_val << 1));

		sensor_write(sd, 0x3e02, explow);
		sensor_write(sd, 0x3e01, exphigh);

		sensor_dbg("sensor_set_long_exp = %d line Done!\n", exp_val);

		/* exp_val /= HDR_RATIO; */

		exphigh = (unsigned char) (0xff & (short_exp_save >> 7));
		explow = (unsigned char) (0xf0 & (short_exp_save << 1));

		sensor_write(sd, 0x3e05, explow);
		sensor_write(sd, 0x3e04, exphigh);
		short_exp_save = exp_val / HDR_RATIO;
		sensor_dbg("sensor_set_short_exp = %d line Done!\n", exp_val);

	} else {
		if (exp_val < 0x18)
			exp_val = 0x18;
		exphigh = (unsigned char) (0xff & (exp_val >> 7));
		explow = (unsigned char) (0xf0 & (exp_val << 1));

		sensor_write(sd, 0x3e02, explow);
		sensor_write(sd, 0x3e01, exphigh);
		sensor_dbg("sensor_set_exp = %d line Done!\n", exp_val);
	}
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
	data_type gaindiglow = 0x80;
	data_type gaindighigh = 0x00;

	int gainana = gain_val << 2 ;
	if (gainana < 0x80) {
		gainhigh = 0x03;
		gainlow = gainana;
	} else if (gainana < 2 * 0x57) {
		gainhigh = 0x07;
		gainlow = gainana >> 1;
	} else if (gainana < 2 * 0x80) {
		gainhigh = 0x23;
		gainlow = ((gainana >> 1) - 0x56) * (0x5f - 0x40) / (0x7f - 0x56) + 0x40;
	} else if (gainana < 4 * (0x80 - 0x5f + 0x40)) {
		gainhigh = 0x23;
		gainlow = ((gainana >> 2) - 0x40) + 0x5f;
	} else if (gainana < 4 * 0x80) {
		gainhigh = 0x27;
		gainlow = ((gainana >> 2) - 0x60) * (0x5f - 0x40) / (0x7f - 0x60) + 0x40;
	} else if (gainana < 8 * (0x80 - 0x5f + 0x40)) {
		gainhigh = 0x27;
		gainlow = ((gainana >> 3) - 0x40) + 0x5f;
	} else if (gainana < 8 * 0x80) {
		gainhigh = 0x2f;
		gainlow = ((gainana >> 3) - 0x60) * (0x5f - 0x40) / (0x7f - 0x60) + 0x40;
	} else if (gainana < 16 * (0x80 - 0x5f + 0x40)) {
		gainhigh = 0x2f;
		gainlow = ((gainana >> 4) - 0x40) + 0x5f;
	} else if (gainana < 16 * 0x80) {
		gainhigh = 0x3f;
		gainlow = ((gainana >> 4) - 0x60) * (0x5f - 0x40) / (0x7f - 0x60) + 0x40;
	} else {
		gainhigh = 0x3f;
		gainlow = 0x5f;
		if (gainana < 32 * 0x80) {
			gaindiglow = gainana >> 4;
			gaindighigh = 0x00;
		} else {
			gaindiglow = 0xfc;
			gaindighigh = 0x00;
		}
		/*
		if (gainana < 16 * 0x80) {
			gaindiglow = gainana >> 3;
			gaindighigh = 0x00;
		} else if (gainana < 32 * 0x80) {
			gaindiglow = gainana >> 4;
			gaindighigh = 0x01;
		} else if (gainana < 64 * 0x80) {
			gaindiglow = gainana >> 5;
			gaindighigh = 0x03;
		} else if (gainana < 128 * 0x80) {
			gaindiglow = gainana >> 6;
			gaindighigh = 0x07;
		} else if (gainana < 256 * 0x80) {
			gaindiglow = gainana >> 7;
			gaindighigh = 0x0f;
		} else {
			gaindiglow = 0xfc;
			gaindighigh = 0x0f;
		}
		*/
	}
	if (info->isp_wdr_mode == ISP_DOL_WDR_MODE) {
		sensor_write(sd, 0x3e13, (unsigned char)gainlow);
		sensor_write(sd, 0x3e12, (unsigned char)gainhigh);
		sensor_write(sd, 0x3e11, (unsigned char)gaindiglow);
/* 		sensor_write(sd, 0x3e10, (unsigned char)gaindighigh); */
	}
	sensor_write(sd, 0x3e09, (unsigned char)gainlow);
	sensor_write(sd, 0x3e08, (unsigned char)gainhigh);
	sensor_write(sd, 0x3e07, (unsigned char)gaindiglow);
/* 	sensor_write(sd, 0x3e06, (unsigned char)gaindighigh); */
	sensor_dbg("sensor_set_anagain = %d, 0x%x, 0x%x Done!\n", gain_val, gainhigh, gainlow);
	sensor_dbg("digital_gain = 0x%x, 0x%x Done!\n", gaindighigh, gaindiglow);
	info->gain = gain_val;
	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	struct sensor_info *info = to_state(sd);
	int exp_val, gain_val;
	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	if (gain_val < 1 * 16)
		gain_val = 16;
	if (exp_val > 0xfffff)
		exp_val = 0xfffff;
	if (fps_change_flag) {
		if (shutter_delay_cnt == shutter_delay) {
			shutter_delay_cnt = 0;
			fps_change_flag = 0;
		} else
			shutter_delay_cnt++;
	}
	/* sensor_write(sd, 0x3812, 0x00); */
	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);
	/* sensor_write(sd, 0x3812, 0x30); */
	sensor_dbg("sensor_set_gain exp = %d, %d Done!\n", gain_val, exp_val);

	info->exp = exp_val;
	info->gain = gain_val;
	return 0;
}

static int sensor_s_fps(struct v4l2_subdev *sd,
			struct sensor_fps *fps)
{
	data_type rdval1, rdval2, rdval3;
	struct sensor_info *info = to_state(sd);
	struct sensor_win_size *wsize = info->current_wins;

	sc2315_sensor_vts = wsize->pclk/fps->fps/wsize->hts;
	fps_change_flag = 1;
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
		ret = sensor_write(sd, 0x0100, rdval&0xfe);
	else
		ret = sensor_write(sd, 0x0100, rdval|0x01);
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
		usleep_range(10000, 12000);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		sensor_dbg("STBY_OFF!\n");
		cci_lock(sd);
		usleep_range(10000, 12000);
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
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_set_mclk_freq(sd, MCLK);
		usleep_range(30000, 32000);
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
		vin_set_pmu_channel(sd, AFVDD, OFF);
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
		usleep_range(1000, 1200);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	data_type rdval = 0;

	sensor_read(sd, 0x3107, &rdval);
	sensor_print("0x3107 = 0x%x\n", rdval);
	if (rdval != (0xff & (V4L2_IDENT_SENSOR>>8)))
		return -ENODEV;
	sensor_read(sd, 0x3108, &rdval);
	sensor_print("0x3108 = 0x%x\n", rdval);
	if (rdval != (0xff & V4L2_IDENT_SENSOR))
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
	info->width = 1920;
	info->height = 1080;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->exp = 0;

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
		.mbus_code = MEDIA_BUS_FMT_SBGGR12_1X12,
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
	 .width = 1920,
	 .height = 1080,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 2200,
	 .vts = 1125,
	 .pclk = 74250000,
	 .mipi_bps = 445500000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .intg_min = 1 << 4,
	 .intg_max = (1125 - 6) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 500 << 4,
	 .regs = sensor_1080p12b30_regs,
	 .regs_size = ARRAY_SIZE(sensor_1080p12b30_regs),
	 .set_size = NULL,
	 .top_clk  = 336*1000*1000,
	 .isp_clk  = 324*1000*1000,
	},

	{
	 .width = 1920,
	 .height = 1080,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 2200,
	 .vts = 2250,
	 .pclk = 148500000,
	 .mipi_bps = 742500000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .if_mode = MIPI_VC_WDR_MODE,
	 .wdr_mode = ISP_DOL_WDR_MODE,
	 .intg_min = 1 << 4,
	 .intg_max = (2250 - 6) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 500 << 4,
	 .regs = sensor_1080p10b30_HDR_regs,
	 .regs_size = ARRAY_SIZE(sensor_1080p10b30_HDR_regs),
	 .set_size = NULL,
	 .top_clk  = 336*1000*1000,
	 .isp_clk  = 324*1000*1000,
	},
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	struct sensor_info *info = to_state(sd);

	cfg->type = V4L2_MBUS_CSI2_DPHY;
	if (info->isp_wdr_mode == ISP_DOL_WDR_MODE)
		cfg->flags = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0 | V4L2_MBUS_CSI2_CHANNEL_1;
	else
		cfg->flags = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
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
	data_type rdval;

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
	sc2315_sensor_vts = wsize->vts;

	sensor_dbg("s_fmt set width = %d, height = %d\n", wsize->width,
		     wsize->height);
	/*
	sensor_read(sd, 0x320c, &rdval);
	sensor_print("0x320c = 0x%x\n", rdval);
	sensor_read(sd, 0x320d, &rdval);
	sensor_print("0x320d = 0x%x\n", rdval);
	sensor_read(sd, 0x320e, &rdval);
	sensor_print("0x320e = 0x%x\n", rdval);
	sensor_read(sd, 0x320f, &rdval);
	sensor_print("0x320f = 0x%x\n", rdval);
	*/
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

	v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 1600,
			      256 * 1600, 1, 1 * 1600);
	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 1,
			      65536 * 16, 1, 1);
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

	info = kzalloc(sizeof(*info), GFP_KERNEL);
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
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET1 | MIPI_NORMAL_MODE;
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
