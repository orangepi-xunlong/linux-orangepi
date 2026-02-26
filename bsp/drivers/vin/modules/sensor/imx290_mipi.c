/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for imx290 Raw cameras.
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

MODULE_AUTHOR("lwj");
MODULE_DESCRIPTION("A low-level driver for imx290 sensors");
MODULE_LICENSE("GPL");

//#define MCLK              (37500000)	/*(24*1000*1000)*/
//#define MCLK              (36*1000*1000)	/*(24*1000*1000)*/
#define MCLK              (37125000)	/*(24*1000*1000)*/
#define V4L2_IDENT_SENSOR  0x0290

#define DOL_RATIO	64
#define DOL3_RATIO1	32
#define DOL3_RATIO2	32
#define VMAX 		1454
#define VMAX_30 	1136
#define VMAX_DOL3	1125
#define HMAX		2200
#define HMAX_DOL3   2200
#define DOL_ENABLE
#define DOL_3FRAME  0
#define DOL_RHS0    ((VMAX/DOL_RATIO+3)|1)	/* 73 */
#define DOL_RHS1	((VMAX_DOL3*4*DOL3_RATIO2/(DOL3_RATIO1*(DOL3_RATIO2+1)+1)/3)*3+1)	/* 133//93 //11//133   //88 //93 */
#define DOL_RHS2	((VMAX_DOL3*4*(DOL3_RATIO2+1)/(DOL3_RATIO1*(DOL3_RATIO2+1)+1)/3)*3+5)	/* 146   //119 */
#define DOL3_YOUT   (DOL_RHS2+(1080+9+8+10)*3)

#define Y_SIZE		2350
/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 30

/*
 * The imx290 i2c address
 */
#define I2C_ADDR 0x34

#define SENSOR_NUM 0x2
#define SENSOR_NAME "imx290_mipi"
#define SENSOR_NAME_2 "imx290_mipi_2"

/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};
static struct regval_list sensor_dol3_4lane12bit_1080P15_regs[] = {
	{0x3002, 0x00},
	{0x3005, 0x01}, /* adbit 0:10bit ;1:12bit */
	{0x3007, 0x00},	/* full */
	{0x3009, 0x01},/* 02:LCG; 12:HCG */
	{0x300A, 0xF0},/* add */
	{0x300C, 0x21},/* dol3 */
	{0x3011, 0x0A},/* "set to 0A" */
	{0x3018, (VMAX_DOL3&0xFF)}, /* vts lsb */
	{0x3019, ((VMAX_DOL3>>8)&0xFF)}, /* vts msb */
	{0x301a, ((VMAX_DOL3>>16)&0x3)}, /* vts hsb */
	{0x301c, (HMAX_DOL3&0xFF)}, /* hts lsb */
	{0x301d, ((HMAX_DOL3>>8)&0xFF)}, /* hts msb */
	{0x3020, 0x02}, /* SHS1 */
	{0x3021, 0x00}, /* SHS1 */
	{0x3022, 0x00}, /* SHS1 */
	{0x3024, 0x89}, /* SHS2 */
	{0x3025, 0x00}, /* SHS2 */
	{0x3026, 0x00}, /* SHS2 */
	{0x3028, 0x93}, /* SHS3 */
	{0x3029, 0x01}, /* SHS3 */
	{0x302A, 0x00}, /* SHS3 */
	{0x3045, 0x05},
	{0x3046, 0x01}, /* 0:10bit ; 1:12bit */
	{0x304B, 0x0A},	/* VSYNC  HSYNC */
	{0x305c, 0x18},
	{0x305d, 0x03},
	{0x305e, 0x20},
	{0x305f, 0x01},
	{0x309e, 0x4A},/* "set to 4A" */
	{0x309f, 0x4A},/* "set to 4A" */
	{0x3106, 0x3b},
	{0x311c, 0x0e},/* "set to 0E" */
	{0x3128, 0x04},/* "set to 04" */
	{0x3129, 0x00},
	{0x313b, 0x41},/* "set to 41" */
	{0x315e, 0x1a},
	{0x3164, 0x1a},
	{0x317c, 0x00},
	{0x31ec, 0x0E},
	{0x3405, 0x10},
	{0x3407, 0x03},
	{0x3414, 0x0A},
	{0x3415, 0x00},
	{0x3418, 0x55},/* 9C},  //Y_OUT_SIZE */
	{0x3419, 0x11},/* pic_v  //Y_OUT_SIZE */
	{0x3441, 0x0c},
	{0x3442, 0x0c},
	{0x3443, 0x03},
	{0x3444, 0x20},
	{0x3445, 0x25},
	{0x3446, 0x57},
	{0x3447, 0x00},
	{0x3448, 0x37},
	{0x3449, 0x00},
	{0x344a, 0x1F},
	{0x344b, 0x00},
	{0x344c, 0x1F},/* modify 00 to 0F */
	{0x344d, 0x00},
	{0x344e, 0x3F},/* increase HP to LP time;  if no image try 3F */
	{0x344f, 0x00},
	{0x3450, 0x77},
	{0x3451, 0x00},
	{0x3452, 0x1F},/* modify 00 to 0F */
	{0x3453, 0x00},
	{0x3454, 0x07},/* modify 00 to 0F */
	{0x3455, 0x00},
	{0x3472, 0xA0},
	{0x3473, 0x07},
	{0x347B, 0x23},
	{0x3480, 0x49},
	{0x3030, (DOL_RHS1&0xff)},  /* RHS1 */
	{0x3031, ((DOL_RHS1>>8)&0xff)},
	{0x3032, ((DOL_RHS1>>16)&0x0f)},
	{0x3034, (DOL_RHS2&0xff)},  /* RHS2 */
	{0x3035, ((DOL_RHS2>>8)&0xff)},
	{0x3036, ((DOL_RHS2>>16)&0x0f)},
	{0x3000, 0x00},//
};

static struct regval_list sensor_dol_4lane12bit_1080P30_regs[] = {
	{0x3003, 0x01},
	{REG_DLY, 0x10}, /* delay */
	{0x3000, 0x01},
	{0x3002, 0x00},
	{0x3005, 0x01}, /* adbit 0:10bit ;1:12bit */
	{0x3007, 0x03},	/* full */
	{0x3009, 0x01},/* 02:LCG; 12:HCG */
	{0x300A, 0xF0},/* add */
	{0x300C, 0x11},/* dol2 */
	{0x3011, 0x0A},/* "set to 0A" */
	{0x3018, (VMAX&0xFF)}, /* vts lsb */
	{0x3019, ((VMAX>>8)&0xFF)}, /* vts msb */
	{0x301a, ((VMAX>>16)&0x3)}, /* vts hsb */
	{0x301c, (HMAX&0xFF)}, /* hts lsb */
	{0x301d, ((HMAX>>8)&0xFF)}, /* hts msb */
	{0x3045, 0x05},
	{0x3046, 0x01}, /* 0:10bit ; 1:12bit */
	{0x304B, 0x0A},	/* VSYNC  HSYNC */
	{0x305c, 0x18},
	{0x305d, 0x03},
	{0x305e, 0x20},
	{0x305f, 0x01},
	{0x309e, 0x4A},/* "set to 4A" */
	{0x309f, 0x4A},/* "set to 4A" */
	{0x3106, 0x3b},
	{0x311c, 0x0e},/* "set to 0E" */
	{0x3128, 0x04},/* "set to 04" */
	{0x3129, 0x00},
	{0x313b, 0x41},/* "set to 41" */
	{0x315e, 0x1a},
	{0x3164, 0x1a},
	{0x317c, 0x00},
	{0x31ec, 0x0E},
	{0x3405, 0x10},
	{0x3407, 0x03},
	{0x3414, 0x0A},
	{0x3415, 0x00},
	{0x3418, 0x49},/* Y_OUT_SIZE */
	{0x3419, 0x04},
	{0x3441, 0x0c},
	{0x3442, 0x0c},
	{0x3443, 0x03},
	{0x3444, 0x20},
	{0x3445, 0x25},
	{0x3446, 0x57},
	{0x3447, 0x00},
	{0x3448, 0x37},
	{0x3449, 0x00},
	{0x344a, 0x1F},
	{0x344b, 0x00},
	{0x344c, 0x1F},/* modify 00 to 0F */
	{0x344d, 0x00},
	{0x344e, 0x3F},/* increase HP to LP time;  if no image try 3F */
	{0x344f, 0x00},
	{0x3450, 0x77},
	{0x3451, 0x00},
	{0x3452, 0x1F},/* modify 00 to 0F */
	{0x3453, 0x00},
	{0x3454, 0x07},/* modify 00 to 0F */
	{0x3455, 0x00},
	{0x3472, 0xA0},
	{0x3473, 0x07},
	{0x347B, 0x23},
	{0x3480, 0x49},


#ifdef DOL_ENABLE
	{0x300C, 0x11},/* DOL 2frame */
	{0x3106, 0x11},/* xvs subsampe */
	{0x3415, 0x00},/* null0_size */
	{0x3418, (Y_SIZE&0xff)},/* 9C}, */
	{0x3419, ((Y_SIZE>>8)&0xFF)},/* pic_v */

	{0x3020, 0x02},
	{0x3021, 0x00},
	{0x3022, 0x00},

	{0x3024, 0xc9},
	{0x3025, 0x07},
	{0x3026, 0x00},

	{0x3030, (DOL_RHS0&0xff)},
	{0x3031, ((DOL_RHS0>>8)&0xff)},
	{0x3032, ((DOL_RHS0>>16)&0x0f)},
#endif
	{0x3000, 0x00},
#if 1 /* debug */
	{0x300A, 0x00},
	{0x300B, 0x00},
	{0x300E, 0x00},
	{0x300F, 0x00},
	{0x308C, 0x21},
#endif


};


static struct regval_list sensor_4lane12bit_1080P30_regs[] = {
	{0x3002, 0x00},
	{0x3005, 0x01},/* adbit 0:10bit ;1:12bit */
	{0x3007, 0x03},
	{0x3009, 0x02},/* 02:LCG; 12:HCG */
	{0x300A, 0xF0},/* add            */
	{0x3011, 0x0A},/* "set to 0A"   */
	{0x3018, 0x65},/* vmax 1125      */
	{0x3019, 0x04},
	/* {0x301c, 0xA0}, */
	/* {0x301d, 0x14}, */
	{0x301c, 0x30},/* hmax 4400     */
	{0x301d, 0x11},
	{0x304B, 0x0A},/* VSYNC  HSYNC  */
	{0x3046, 0x01},/* 0:10bit ; 1:12bit */
	{0x305c, 0x18},
	{0x305d, 0x03},
	{0x305e, 0x20},
	{0x305f, 0x01},
	{0x309e, 0x4A},/* "set to 4A" */
	{0x309f, 0x4A},/* "set to 4A" */
	{0x311c, 0x0e},/* "set to 0E" */
	{0x3128, 0x04},/* "set to 04" */
	{0x3129, 0x00},/* 10bit :1d;12bit:00 */
	{0x313b, 0x41},/* "set to 41" */
	{0x315e, 0x1a},
	{0x3164, 0x1a},
	{0x317c, 0x00},/* 10bit :12;12bit:00 */
	{0x31ec, 0x0e},/* 10bit :37;12bit:0e */
	{0x3405, 0x20},
	{0x3407, 0x03},
	{0x3414, 0x0A},
	{0x3418, 0x49},
	{0x3419, 0x04},
	{0x3441, 0x0c},/* raw 10 0A0A ;RAW 12 0C0C */
	{0x3442, 0x0c},
	{0x3443, 0x03},
	{0x3444, 0x20},
	{0x3445, 0x25},
	{0x3446, 0x47},
	{0x3447, 0x00},
	{0x3448, 0x1F},
	{0x3449, 0x00},
	{0x344a, 0x17},
	{0x344b, 0x00},
	{0x344c, 0x0F},/* modify 00 to 0F */
	{0x344d, 0x00},
	{0x344e, 0x37},/* increase HP toLP time */
	{0x344f, 0x00},
	{0x3450, 0x47},
	{0x3451, 0x00},
	{0x3452, 0x0F},/* modify 00 to 0F */
	{0x3453, 0x00},
	{0x3454, 0x0F},/* modify 00 to 0F */
	{0x3455, 0x00},
	{0x3472, 0x9c},
	{0x3473, 0x07},
	{0x3480, 0x49},
	/* {0x308c, 0x31},//color bar */
	{0x3000, 0x00},
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

static int imx290_sensor_vts;
static int imx290_sensor_svr;
static int shutter_delay = 1;
static int shutter_delay_cnt;
static int fps_change_flag;

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	data_type explow, expmid, exphigh;
	int exptime,  exp_val_m, exp_val_s;
	struct sensor_info *info = to_state(sd);
	struct sensor_win_size *wsize = info->current_wins;

	if (info->tpf.denominator == 15) {
		sensor_dbg("WDR --- DOL 3 FRAME mode !!!\n");
		/* LEF  --  set SHS3 */
		exptime = (imx290_sensor_vts<<2) - (exp_val>>4) - 1;
		if (exptime < DOL_RHS2 + 3) {
				exptime = DOL_RHS2 + 3;
				exp_val = ((imx290_sensor_vts<<2) - exptime - 1) << 4;
			}
		printk("long exp_val %d, exptime %d, RHS1 %d, RHS2 %d, Y_OUT %d, \n", exp_val, exptime, DOL_RHS1, DOL_RHS2, DOL3_YOUT);
		exphigh	= (unsigned char) ((0x0030000&exptime)>>16);
		expmid	= (unsigned char) ((0x000ff00&exptime)>>8);
		explow	= (unsigned char) ((0x00000ff&exptime));
		sensor_write(sd, 0x3028, explow);
		sensor_write(sd, 0x3029, expmid);
		sensor_write(sd, 0x302a, exphigh);
		/* SEF1  --  set SHS1 */
		exp_val_m = exp_val/DOL3_RATIO1;
		if (exp_val_m < 16)
			exp_val_m = 16;
		exptime = DOL_RHS1 - (exp_val_m>>4) - 1;
		if (exptime < 3)
			exptime = 3;

		printk("short 1 exp_val: %d, exptime: %d\n", exp_val_m, exptime);

		exphigh	= (unsigned char) ((0x0030000&exptime)>>16);
		expmid	= (unsigned char) ((0x000ff00&exptime)>>8);
		explow	= (unsigned char) ((0x00000ff&exptime));

		sensor_write(sd, 0x3020, explow);
		sensor_write(sd, 0x3021, expmid);
		sensor_write(sd, 0x3022, exphigh);
		/* SEF2 -- set SHS2 */
		exp_val_s = exp_val_m/DOL3_RATIO2;
		if (exp_val_s < 16)
			exp_val_s = 16;
		exptime = DOL_RHS2 - (exp_val_s>>4) - 1;
		if (exptime < DOL_RHS1 + 3)
			exptime = DOL_RHS1 + 3;
		printk("short 2 exp_val: %d, exptime: %d\n", exp_val_s, exptime);
		exphigh = (unsigned char) ((0x0030000&exptime)>>16);
		expmid	= (unsigned char) ((0x000ff00&exptime)>>8);
		explow	= (unsigned char) ((0x00000ff&exptime));
		sensor_write(sd, 0x3024, explow);
		sensor_write(sd, 0x3025, expmid);
		sensor_write(sd, 0x3026, exphigh);
	} else {
		if (info->isp_wdr_mode == ISP_DOL_WDR_MODE) {
			printk("WDR  mode !!!_imx290\n");
			/* LEF */
			exptime = (imx290_sensor_vts<<1) - (exp_val>>4) - 1;
			if (exptime < DOL_RHS0 + 2) {
				exptime = DOL_RHS0 + 2;
				exp_val = ((imx290_sensor_vts<<1) - exptime - 1) << 4;
			}
			printk("long exp_val: %d, exptime: %d\n", exp_val, exptime);
			exphigh	= (unsigned char) ((0x0030000&exptime)>>16);
			expmid	= (unsigned char) ((0x000ff00&exptime)>>8);
			explow	= (unsigned char) ((0x00000ff&exptime));
			sensor_write(sd, 0x3024, explow);
			sensor_write(sd, 0x3025, expmid);
			sensor_write(sd, 0x3026, exphigh);

			/* SEF */
			exp_val_m = exp_val/DOL_RATIO;
			if (exp_val_m < 2 * 16)
				exp_val_m = 2 * 16;
			exptime = DOL_RHS0 - (exp_val_m>>4) - 1;
			if (exptime < 1)
				exptime = 1;

			printk("short exp_val: %d, exptime: %d\n", exp_val_m, exptime);

			exphigh	= (unsigned char) ((0x0030000&exptime)>>16);
			expmid	= (unsigned char) ((0x000ff00&exptime)>>8);
			explow	= (unsigned char) ((0x00000ff&exptime));

			sensor_write(sd, 0x3020, explow);
			sensor_write(sd, 0x3021, expmid);
			sensor_write(sd, 0x3022, exphigh);
		} else {
			sensor_dbg("normal  mode !!!\n");
			exptime = imx290_sensor_vts - (exp_val>>4) - 1;
			exphigh = (unsigned char)((0x0030000 & exptime) >> 16);
			expmid = (unsigned char)((0x000ff00 & exptime) >> 8);
			explow = (unsigned char)((0x00000ff & exptime));
			sensor_write(sd, 0x3020, explow);
			sensor_write(sd, 0x3021, expmid);
			sensor_write(sd, 0x3022, exphigh);
		}
	}
	sensor_dbg("sensor_set_exp = %d %d %d %d Done!\n", imx290_sensor_vts, exp_val,
			exptime, exp_val_m);
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

unsigned char gain2db[497] = {
	0,   2,   3,	 5,   6,   8,	9,  11,  12,  13,  14,	15,  16,  17,
	18,  19,  20,	21,  22,  23,  23,  24,  25,  26,  27,	27,  28,  29,
	29,  30,  31,	31,  32,  32,  33,  34,  34,  35,  35,	36,  36,  37,
	37,  38,  38,	39,  39,  40,  40,  41,  41,  41,  42,	42,  43,  43,
	44,  44,  44,	45,  45,  45,  46,  46,  47,  47,  47,	48,  48,  48,
	49,  49,  49,	50,  50,  50,  51,  51,  51,  52,  52,	52,  52,  53,
	53,  53,  54,	54,  54,  54,  55,  55,  55,  56,  56,	56,  56,  57,
	57,  57,  57,	58,  58,  58,  58,  59,  59,  59,  59,	60,  60,  60,
	60,  60,  61,	61,  61,  61,  62,  62,  62,  62,  62,	63,  63,  63,
	63,  63,  64,	64,  64,  64,  64,  65,  65,  65,  65,	65,  66,  66,
	66,  66,  66,	66,  67,  67,  67,  67,  67,  68,  68,	68,  68,  68,
	68,  69,  69,	69,  69,  69,  69,  70,  70,  70,  70,	70,  70,  71,
	71,  71,  71,	71,  71,  71,  72,  72,  72,  72,  72,	72,  73,  73,
	73,  73,  73,	73,  73,  74,  74,  74,  74,  74,  74,	74,  75,  75,
	75,  75,  75,	75,  75,  75,  76,  76,  76,  76,  76,	76,  76,  77,
	77,  77,  77,	77,  77,  77,  77,  78,  78,  78,  78,	78,  78,  78,
	78,  79,  79,	79,  79,  79,  79,  79,  79,  79,  80,	80,  80,  80,
	80,  80,  80,	80,  80,  81,  81,  81,  81,  81,  81,	81,  81,  81,
	82,  82,  82,	82,  82,  82,  82,  82,  82,  83,  83,	83,  83,  83,
	83,  83,  83,	83,  83,  84,  84,  84,  84,  84,  84,	84,  84,  84,
	84,  85,  85,	85,  85,  85,  85,  85,  85,  85,  85,	86,  86,  86,
	86,  86,  86,	86,  86,  86,  86,  86,  87,  87,  87,	87,  87,  87,
	87,  87,  87,	87,  87,  88,  88,  88,  88,  88,  88,	88,  88,  88,
	88,  88,  88,	89,  89,  89,  89,  89,  89,  89,  89,	89,  89,  89,
	89,  90,  90,	90,  90,  90,  90,  90,  90,  90,  90,	90,  90,  91,
	91,  91,  91,	91,  91,  91,  91,  91,  91,  91,  91,	91,  92,  92,
	92,  92,  92,	92,  92,  92,  92,  92,  92,  92,  92,	93,  93,  93,
	93,  93,  93,	93,  93,  93,  93,  93,  93,  93,  93,	94,  94,  94,
	94,  94,  94,	94,  94,  94,  94,  94,  94,  94,  94,	95,  95,  95,
	95,  95,  95,	95,  95,  95,  95,  95,  95,  95,  95,	95,  96,  96,
	96,  96,  96,	96,  96,  96,  96,  96,  96,  96,  96,	96,  96,  97,
	97,  97,  97,	97,  97,  97,  97,  97,  97,  97,  97,	97,  97,  97,
	97,  98,  98,	98,  98,  98,  98,  98,  98,  98,  98,	98,  98,  98,
	98,  98,  98,	99,  99,  99,  99,  99,  99,  99,  99,	99,  99,  99,
	99,  99,  99,	99,  99,  99, 100, 100, 100, 100, 100, 100, 100, 100,
	100, 100, 100, 100, 100, 100, 100,
};
static char gain_mode_buf = 0x02;
static unsigned int count;

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	struct sensor_info *info = to_state(sd);
	int ret;
	data_type rdval;
	char gain_mode = 0x02;

	ret = sensor_read(sd, 0x3009, &rdval);
	if (ret != 0)
		return ret;
	if (gain_val < 1 * 16)
		gain_val = 16;
	if (gain_val > 16 * 16) {
		gain_mode = rdval | 0x10;
		if (gain_val < 64 * 16) {
			sensor_write(sd, 0x3014, gain2db[(gain_val>>1) - 16]);
		} else if (gain_val < 2048 * 16) {
			sensor_write(sd, 0x3014, gain2db[(gain_val>>6) - 16] + 100);
		} else {
			sensor_write(sd, 0x3014, gain2db[(gain_val>>11) - 16] + 200);
		}
	} else {
		gain_mode = rdval & 0xef;
		sensor_write(sd, 0x3014, gain2db[gain_val - 16]);
	}
	if (0 != (count++))
		sensor_write(sd, 0x3009, gain_mode_buf);
	gain_mode_buf = gain_mode;
	sensor_dbg("sensor_set_gain = %d, Done!\n", gain_val);
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
	if (fps_change_flag) {
		if (shutter_delay_cnt == shutter_delay) {
			sensor_write(sd, 0x3018, imx290_sensor_vts / (imx290_sensor_svr + 1) & 0xFF);
			sensor_write(sd, 0x3019, imx290_sensor_vts / (imx290_sensor_svr + 1) >> 8 & 0xFF);
			sensor_write(sd, 0x301a, imx290_sensor_vts / (imx290_sensor_svr + 1) >> 16);
			sensor_write(sd, 0x3001, 0);
			shutter_delay_cnt = 0;
			fps_change_flag = 0;
		} else
			shutter_delay_cnt++;
	}

	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);

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
	imx290_sensor_vts = wsize->pclk/fps->fps/wsize->hts;
	fps_change_flag = 1;
	sensor_write(sd, 0x3001, 1);

	sensor_read(sd, 0x3018, &rdval1);
	sensor_read(sd, 0x3019, &rdval2);
	sensor_read(sd, 0x301a, &rdval3);

	sensor_dbg("imx290_sensor_svr: %d, vts: %d.\n", imx290_sensor_svr, (rdval1 | (rdval2<<8) | (rdval3<<16)));
	return 0;
}

static void sensor_g_combo_wdr_cfg(struct v4l2_subdev *sd,
				struct combo_wdr_cfg *wdr)
{
	wdr->line_code_mode = 1;

	wdr->line_cnt = 2;

	wdr->code_mask = 0x40ff;
	wdr->wdr_fid_mode_sel = 0;
	wdr->wdr_fid_map_en = 0x3;
	wdr->wdr_fid0_map_sel = 0x0;
	wdr->wdr_fid1_map_sel = 0x1;
	wdr->wdr_fid2_map_sel = 0;
	wdr->wdr_fid3_map_sel = 0;
}

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	int ret;
	data_type rdval;

	ret = sensor_read(sd, 0x3000, &rdval);
	if (ret != 0)
		return ret;

	if (on_off == STBY_ON)
		ret = sensor_write(sd, 0x3000, rdval & 0xfe);
	else
		ret = sensor_write(sd, 0x3000, rdval | 0x01);

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
		vin_set_pmu_channel(sd, IOVDD, ON);
		usleep_range(10000, 12200);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(1000, 1200);
		vin_set_mclk(sd, ON);
		usleep_range(1000, 1200);
		vin_set_mclk_freq(sd, MCLK);
		usleep_range(3000, 3200);
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
	sensor_read(sd, 0x31D8, &rdval);
	sensor_print("%s read value is 0x%x\n", __func__, rdval);
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
	info->width = HD1080_WIDTH;
	info->height = HD1080_HEIGHT;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;

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
	case GET_COMBO_WDR_CFG:
		/* sensor_g_combo_wdr_cfg(sd, (struct combo_wdr_cfg *)arg); */
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
		.mbus_code = MEDIA_BUS_FMT_SRGGB12_1X12,
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


/* 1080P dol */
	/* 1080P */
	{
	.width = 1920,
	.height = 1080,
	.hoffset = 0,
	.voffset = 0,
	.hts = 4400,
	.vts = 1125,
	.pclk = 148500000,
	.mipi_bps = 446 * 1000 * 1000,
	.fps_fixed = 30,
	.bin_factor = 1,
	.intg_min = 1 << 4,
	.intg_max = (1125 - 3) << 4,
	.gain_min = 1 << 4,
	.gain_max = 8192 << 4,
	.regs = sensor_4lane12bit_1080P30_regs,
	.regs_size = ARRAY_SIZE(sensor_4lane12bit_1080P30_regs),
	.set_size = NULL,
	.vipp_hoff = 0,
	.vipp_voff = 0,
	.top_clk = 300000000,
	.isp_clk = 300000000,
	},
	{
	.width = 1920,
	.height = Y_SIZE-2,
	.hoffset = 0,
	.voffset = 0,
	.hts = HMAX,
	.vts = VMAX,
	.pclk = 72 * 1000 * 1000,
	.mipi_bps = 891 * 1000 * 1000,
	.fps_fixed = 30,
	.bin_factor = 1,
	.intg_min = 1 << 4,
	.intg_max = (VMAX - 3) << 4,
	.gain_min = 1 << 4,
	.gain_max = 8192 << 4,
	/* .if_mode = MIPI_DOL_WDR_MODE, */
	.wdr_mode = ISP_DOL_WDR_MODE,
	.regs = sensor_dol_4lane12bit_1080P30_regs,
	.regs_size = ARRAY_SIZE(sensor_dol_4lane12bit_1080P30_regs),
	.set_size = NULL,
	.top_clk = 360*1000*1000,
	.isp_clk = 360*1000*1000,
	},

	 /* 1080P dol 3 frame */
	{
	.width = 1920,
	.height = DOL3_YOUT,
	.hoffset = 4,
	.voffset = 0,
	.hts = 2200,
	.vts = 1125,
	.pclk = 72 * 1000 * 1000,
	.mipi_bps = 446 * 1000 * 1000,
	.fps_fixed = 15,
	.bin_factor = 1,
	.intg_min = 1 << 4,
	.intg_max = (1125 - 3) << 4,
	.gain_min = 1 << 4,
	.gain_max = 8192 << 4,
	/* .if_mode = MIPI_DOL_WDR_MODE, */
	.wdr_mode = ISP_DOL_WDR_MODE,
	.regs = sensor_dol3_4lane12bit_1080P15_regs,
	.regs_size = ARRAY_SIZE(sensor_dol3_4lane12bit_1080P15_regs),
	.set_size = NULL,
	.vipp_hoff = 4,
	.vipp_voff = 12,
	.top_clk = 360*1000*1000,
	.isp_clk = 360*1000*1000,
	},

};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	struct sensor_info *info = to_state(sd);

	cfg->type = V4L2_MBUS_CSI2_DPHY;
	if (info->isp_wdr_mode == ISP_DOL_WDR_MODE)
		cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0 | V4L2_MBUS_CSI2_CHANNEL_1;
	else
		cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;

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
	imx290_sensor_vts = wsize->vts;
	info->tpf.denominator = wsize->fps_fixed;
	sensor_print("s_fmt set width = %d, height = %d, vts = %d, fps = %d\n", wsize->width,
		     wsize->height, wsize->vts, wsize->fps_fixed);

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

	mutex_init(&info->lock);

	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET2 | MIPI_NORMAL_MODE;
//	info->combo_mode = MIPI_NORMAL_MODE;
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
