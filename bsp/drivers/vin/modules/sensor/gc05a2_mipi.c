/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for GC05A2 Raw cameras.
 *
 * Copyright (c) 2022 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Liu Chensheng <liuchensheng@allwinnertech.com>
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

MODULE_AUTHOR("lcs");
MODULE_DESCRIPTION("A low-level driver for gc05a2 sensors");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR 0x05A2

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The GC05A2 i2c address
 */
#define I2C_ADDR 0x6e

#define SENSOR_NUM	0x2
#define SENSOR_NAME "gc05a2_mipi"
#define SENSOR_NAME_2 "gc05a2_mipi_2"

/* SENSOR MIRROR FLIP INFO */
#define GC05A2_MIRROR_NORMAL    1
#define GC05A2_MIRROR_H         0
#define GC05A2_MIRROR_V         0
#define GC05A2_MIRROR_HV        0

#if GC05A2_MIRROR_NORMAL
#define GC05A2_MIRROR	        0x00
#elif GC05A2_MIRROR_H
#define GC05A2_MIRROR	        0x01
#elif GC05A2_MIRROR_V
#define GC05A2_MIRROR	        0x02
#elif GC05A2_MIRROR_HV
#define GC05A2_MIRROR	        0x03
#else
#define GC05A2_MIRROR	        0x00
#endif

/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {
};
static struct regval_list sensor_2592x1944p30_regs[] = {
/*system*/
	{0x0315, 0xd4},
	{0x0d06, 0x01},
	{0x0a70, 0x80},
	{0x031a, 0x00},
	{0x0314, 0x00},
	{0x0130, 0x08},
	{0x0132, 0x01},
	{0x0135, 0x01},
	{0x0136, 0x38},
	{0x0137, 0x03},
	{0x0134, 0x5b},
	{0x031c, 0xe0},
	{0x0d82, 0x14},
	{0x0dd1, 0x56},
/*gate_mode*/
	{0x0af4, 0x01},
	{0x0002, 0x10},
	{0x00c3, 0x34},
/*pre_setting*/
	{0x0084, 0x21},
	{0x0d05, 0xcc},
	{0x0218, 0x00},
	{0x005e, 0x48},
	{0x0d06, 0x01},
	{0x0007, 0x16},
	{0x0101, 0x00},
/*analog*/
	{0x0342, 0x07},
	{0x0343, 0x28},
	{0x0220, 0x07},
	{0x0221, 0xd0},
	{0x0202, 0x07},
	{0x0203, 0x32},
	{0x0340, 0x07},
	{0x0341, 0xf0},
	{0x0219, 0x10},
	{0x0346, 0x00},
	{0x0347, 0x04},
	{0x0d14, 0x00},
	{0x0d13, 0x05},
	{0x0d16, 0x05},
	{0x0d15, 0x1d},
	{0x00c0, 0x0a},
	{0x00c1, 0x30},
	{0x034a, 0x07},
	{0x034b, 0xa8},
	{0x0e0a, 0x00},
	{0x0e0b, 0x00},
	{0x0e0e, 0x03},
	{0x0e0f, 0x00},
	{0x0e06, 0x0a},
	{0x0e23, 0x15},
	{0x0e24, 0x15},
	{0x0e2a, 0x10},
	{0x0e2b, 0x10},
	{0x0e17, 0x49},
	{0x0e1b, 0x1c},
	{0x0e3a, 0x36},
	{0x0d11, 0x84},
	{0x0e52, 0x14},
	{0x000b, 0x10},
	{0x0008, 0x08},
	{0x0223, 0x17},
	{0x0d27, 0x39},
	{0x0d22, 0x00},
	{0x03f6, 0x0d},
	{0x0d04, 0x07},
	{0x03f3, 0x72},
	{0x03f4, 0xb8},
	{0x03f5, 0xbc},
	{0x0d02, 0x73},
/*auto load start*/
	{0x00c4, 0x00},
	{0x00c5, 0x01},
	{0x0af6, 0x00},
	{0x0ba0, 0x17},
	{0x0ba1, 0x00},
	{0x0ba2, 0x00},
	{0x0ba3, 0x00},
	{0x0ba4, 0x03},
	{0x0ba5, 0x00},
	{0x0ba6, 0x00},
	{0x0ba7, 0x00},
	{0x0ba8, 0x40},
	{0x0ba9, 0x00},
	{0x0baa, 0x00},
	{0x0bab, 0x00},
	{0x0bac, 0x40},
	{0x0bad, 0x00},
	{0x0bae, 0x00},
	{0x0baf, 0x00},
	{0x0bb0, 0x02},
	{0x0bb1, 0x00},
	{0x0bb2, 0x00},
	{0x0bb3, 0x00},
	{0x0bb8, 0x02},
	{0x0bb9, 0x00},
	{0x0bba, 0x00},
	{0x0bbb, 0x00},
	{0x0a70, 0x80},
	{0x0a71, 0x00},
	{0x0a72, 0x00},
	{0x0a66, 0x00},
	{0x0a67, 0x80},
	{0x0a4d, 0x4e},
	{0x0a50, 0x00},
	{0x0a4f, 0x0c},
	{0x0a66, 0x00},
	{0x00ca, 0x00},
	{0x00cb, 0x00},
	{0x00cc, 0x00},
	{0x00cd, 0x00},
/*auto load CH_GAIN*/
	{0x0aa1, 0x00},
	{0x0aa2, 0xe0},
	{0x0aa3, 0x00},
	{0x0aa4, 0x40},
	{0x0a90, 0x03},
	{0x0a91, 0x0e},
	{0x0a94, 0x80},
/*standby*/
	{0x0af6, 0x20},
	{0x0b00, 0x91},
	{0x0b01, 0x17},
	{0x0b02, 0x01},
	{0x0b03, 0x00},
	{0x0b04, 0x01},
	{0x0b05, 0x17},
	{0x0b06, 0x01},
	{0x0b07, 0x00},

	{0x0ae9, 0x01},
	{0x0aea, 0x02},
	{0x0ae8, 0x53},
	{0x0ae8, 0x43},
/*gain_partition*/
	{0x0af6, 0x30},
	{0x0b00, 0x08},
	{0x0b01, 0x0f},
	{0x0b02, 0x00},

	{0x0b04, 0x1c},
	{0x0b05, 0x30},
	{0x0b06, 0x00},

	{0x0b08, 0x0e},
	{0x0b09, 0x2a},
	{0x0b0a, 0x00},

	{0x0b0c, 0x0e},
	{0x0b0d, 0x2b},
	{0x0b0e, 0x00},

	{0x0b10, 0x0e},
	{0x0b11, 0x23},
	{0x0b12, 0x00},

	{0x0b14, 0x0e},
	{0x0b15, 0x24},
	{0x0b16, 0x00},

	{0x0b18, 0x0c},
	{0x0b19, 0x0c},
	{0x0b1a, 0x00},

	{0x0b1c, 0x03},
	{0x0b1d, 0x03},
	{0x0b1e, 0x00},

	{0x0b20, 0x0c},
	{0x0b21, 0x0c},
	{0x0b22, 0x00},

	{0x0b24, 0x03},
	{0x0b25, 0x03},
	{0x0b26, 0x00},

	{0x0b28, 0x12},
	{0x0b29, 0x12},
	{0x0b2a, 0x00},

	{0x0b2c, 0x08},
	{0x0b2d, 0x08},
	{0x0b2e, 0x00},

	{0x0b30, 0x14},
	{0x0b31, 0x14},
	{0x0b32, 0x00},

	{0x0b34, 0x10},
	{0x0b35, 0x10},
	{0x0b36, 0x00},

	{0x0b38, 0x18},
	{0x0b39, 0x18},
	{0x0b3a, 0x00},

	{0x0b3c, 0x16},
	{0x0b3d, 0x16},
	{0x0b3e, 0x00},

	{0x0b80, 0x01},
	{0x0b81, 0x00},
	{0x0b82, 0x00},
	{0x0b84, 0x00},
	{0x0b85, 0x00},
	{0x0b86, 0x00},

	{0x0b88, 0x01},
	{0x0b89, 0x6a},
	{0x0b8a, 0x00},
	{0x0b8c, 0x00},
	{0x0b8d, 0x01},
	{0x0b8e, 0x00},

	{0x0b90, 0x01},
	{0x0b91, 0xf6},
	{0x0b92, 0x00},
	{0x0b94, 0x00},
	{0x0b95, 0x02},
	{0x0b96, 0x00},

	{0x0b98, 0x02},
	{0x0b99, 0xc4},
	{0x0b9a, 0x00},
	{0x0b9c, 0x00},
	{0x0b9d, 0x03},
	{0x0b9e, 0x00},

	{0x0ba0, 0x03},
	{0x0ba1, 0xd8},
	{0x0ba2, 0x00},
	{0x0ba4, 0x00},
	{0x0ba5, 0x04},
	{0x0ba6, 0x00},

	{0x0ba8, 0x05},
	{0x0ba9, 0x4d},
	{0x0baa, 0x00},
	{0x0bac, 0x00},
	{0x0bad, 0x05},
	{0x0bae, 0x00},

	{0x0bb0, 0x07},
	{0x0bb1, 0x3e},
	{0x0bb2, 0x00},
	{0x0bb4, 0x00},
	{0x0bb5, 0x06},
	{0x0bb6, 0x00},

	{0x0bb8, 0x0a},
	{0x0bb9, 0x1a},
	{0x0bba, 0x00},
	{0x0bbc, 0x09},
	{0x0bbd, 0x36},
	{0x0bbe, 0x00},

	{0x0bc0, 0x0e},
	{0x0bc1, 0x66},
	{0x0bc2, 0x00},
	{0x0bc4, 0x10},
	{0x0bc5, 0x06},
	{0x0bc6, 0x00},

	{0x02c1, 0xe0},
	{0x0207, 0x04},
	{0x02c2, 0x10},
	{0x02c3, 0x54},
	{0x02c5, 0x09},

 /*auto load CH_GAIN*/
	{0x0aa1, 0x15},
	{0x0aa2, 0x50},
	{0x0aa3, 0x00},
	{0x0aa4, 0x09},
	{0x0a90, 0x25},
	{0x0a91, 0x0e},
	{0x0a94, 0x80},

/*ISP*/
	{0x0050, 0x00},
	{0x0089, 0x83},
	{0x005a, 0x40},
	{0x00c3, 0x35},
	{0x00c4, 0x80},
	{0x0080, 0x10},
	{0x0040, 0x12},
	{0x0053, 0x0a},
	{0x0054, 0x44},
	{0x0055, 0x32},
	{0x004a, 0x03},
	{0x0048, 0xf0},
	{0x0049, 0x0f},
	{0x0041, 0x20},
	{0x0043, 0x0a},
	{0x009d, 0x08},

/*gain*/
	{0x0204, 0x04},
	{0x0205, 0x00},
	{0x02b3, 0x00},
	{0x02b4, 0x00},
	{0x009e, 0x01},
	{0x009f, 0x94},

/*OUT 2592x1944*/
	{0x0350, 0x01},
	{0x0353, 0x00},
	{0x0354, 0x08},
	{0x034c, 0x0a},
	{0x034d, 0x20},
	{0x021f, 0x14},

/*auto load REG*/
	{0x0aa1, 0x10},
	{0x0aa2, 0xf8},
	{0x0aa3, 0x00},
	{0x0aa4, 0x0a},
	{0x0a90, 0x11},
	{0x0a91, 0x0e},
	{0x0a94, 0x80},
	{0x03fe, 0x00},
	{0x03fe, 0x00},
	{0x03fe, 0x00},
	{0x03fe, 0x00},
	{0x0a94, 0x00},
	{0x0a70, 0x00},
	{0x0a67, 0x00},
	{0x0af4, 0x29},

/*DPHY */
	{0x0d80, 0x07},
	{0x0dd3, 0x18},

/*MIPI*/
	{0x0107, 0x05},
	{0x0117, 0x01},
	{0x0d81, 0x00},
	{0x0d84, 0x0c},
	{0x0d85, 0xa8},
	{0x0d86, 0x06},
	{0x0d87, 0x55},
	{0x0db3, 0x06},
	{0x0db4, 0x08},
	{0x0db5, 0x1e},
	{0x0db6, 0x02},
	{0x0db8, 0x12},
	{0x0db9, 0x0a},
	{0x0d93, 0x06},
	{0x0d94, 0x09},
	{0x0d95, 0x0d},
	{0x0d99, 0x0b},
	{0x0084, 0x01},

/*CISCTL_Reset*/
	{0x031c, 0x80},
	{0x03fe, 0x30},
	{0x0d17, 0x06},
	{0x03fe, 0x00},
	{0x0d17, 0x00},
	{0x031c, 0x93},
	{0x03fe, 0x00},
	{0x03fe, 0x00},
	{0x03fe, 0x00},
	{0x03fe, 0x00},
	{0x031c, 0x80},
	{0x03fe, 0x30},
	{0x0d17, 0x06},
	{0x03fe, 0x00},
	{0x0d17, 0x00},
	{0x031c, 0x93},
/*OUT*/
	{0x0110, 0x01},
	{0x0100, 0x01},

};

#if VIN_FALSE
static struct regval_list sensor_1296x972p30_regs[] = {
/*system*/
	{0x0315, 0xd4},
	{0x0d06, 0x01},
	{0x0a70, 0x80},
	{0x031a, 0x00},
	{0x0314, 0x00},
	{0x0130, 0x08},
	{0x0132, 0x01},
	{0x0135, 0x05},
	{0x0136, 0x38},
	{0x0137, 0x03},
	{0x0134, 0x5b},
	{0x031c, 0xe0},
	{0x0d82, 0x14},
	{0x0dd1, 0x56},
/*gate_mode*/
	{0x0af4, 0x01},
	{0x0002, 0x10},
	{0x00c3, 0x34},
/*pre_setting*/
	{0x0084, 0x31},
	{0x0d05, 0xcc},
	{0x0218, 0x80},
	{0x005e, 0x49},
	{0x0d06, 0x81},
	{0x0007, 0x16},
	{0x0101, GC05A2_MIRROR},
/*analog*/
	{0x0342, 0x07},
	{0x0343, 0x10},
	{0x0220, 0x0f},
	{0x0221, 0xe0},
	{0x0202, 0x03},
	{0x0203, 0x32},
	{0x0340, 0x07},
	{0x0341, 0xf0},
	{0x0219, 0x00},
	{0x0346, 0x00},
	{0x0347, 0x04},
	{0x00c0, 0x0a},
	{0x00c1, 0x30},
	{0x034a, 0x07},
	{0x034b, 0xa8},
	{0x0e0a, 0x00},
	{0x0e0b, 0x00},
	{0x0e0e, 0x03},
	{0x0e0f, 0x00},
	{0x0e06, 0x0a},
	{0x0e23, 0x15},
	{0x0e24, 0x15},
	{0x0e2a, 0x10},
	{0x0e2b, 0x10},
	{0x0e17, 0x49},
	{0x0e1b, 0x1c},
	{0x0e3a, 0x36},
	{0x0d11, 0x84},
	{0x0e52, 0x14},
	{0x000b, 0x13},
	{0x0223, 0x16},
	{0x0d27, 0x39},
	{0x0d22, 0x00},
	{0x03f6, 0x0d},
	{0x0d04, 0x07},
	{0x03f3, 0x72},
	{0x03f4, 0xb8},
	{0x03f5, 0xbc},
	{0x0d02, 0x73},
/*auto load start*/
	{0x00c4, 0x00},
	{0x00c5, 0x01},
	{0x0af6, 0x00},
	{0x0ba0, 0x17},
	{0x0ba1, 0x00},
	{0x0ba2, 0x00},
	{0x0ba3, 0x00},
	{0x0ba4, 0x03},
	{0x0ba5, 0x00},
	{0x0ba6, 0x00},
	{0x0ba7, 0x00},
	{0x0ba8, 0x05},
	{0x0ba9, 0x00},
	{0x0baa, 0x00},
	{0x0bab, 0x00},
	{0x0bac, 0x05},
	{0x0bad, 0x00},
	{0x0bae, 0x00},
	{0x0baf, 0x00},
	{0x0bb0, 0x02},
	{0x0bb1, 0x00},
	{0x0bb2, 0x00},
	{0x0bb3, 0x00},
	{0x0bb8, 0x02},
	{0x0bb9, 0x00},
	{0x0bba, 0x00},
	{0x0bbb, 0x00},
	{0x0a70, 0x80},
	{0x0a71, 0x00},
	{0x0a72, 0x00},
	{0x0a66, 0x00},
	{0x0a67, 0x80},
	{0x0a4d, 0x46},
	{0x0a50, 0x00},
	{0x0a4f, 0x0c},
	{0x0a66, 0x00},
	{0x0aa1, 0x00},
	{0x0aa2, 0xe0},
	{0x0aa3, 0x00},
	{0x0aa4, 0x40},
	{0x0a90, 0x03},
	{0x0a91, 0x0e},
	{0x0a94, 0x80},
/*gain_partition*/
	{0x0af6, 0x30},
	{0x0b00, 0x08},
	{0x0b01, 0x0f},
	{0x0b02, 0x00},
	{0x0b04, 0x1c},
	{0x0b05, 0x30},
	{0x0b06, 0x00},
	{0x0b08, 0x0e},
	{0x0b09, 0x2a},
	{0x0b0a, 0x00},
	{0x0b0c, 0x0e},
	{0x0b0d, 0x2b},
	{0x0b0e, 0x00},
	{0x0b10, 0x0e},
	{0x0b11, 0x23},
	{0x0b12, 0x00},
	{0x0b14, 0x0e},
	{0x0b15, 0x24},
	{0x0b16, 0x00},
	{0x0b18, 0x0c},
	{0x0b19, 0x0c},
	{0x0b1a, 0x00},
	{0x0b1c, 0x03},
	{0x0b1d, 0x03},
	{0x0b1e, 0x00},
	{0x0b20, 0x0c},
	{0x0b21, 0x0c},
	{0x0b22, 0x00},
	{0x0b24, 0x03},
	{0x0b25, 0x03},
	{0x0b26, 0x00},
	{0x0b28, 0x12},
	{0x0b29, 0x12},
	{0x0b2a, 0x00},
	{0x0b2c, 0x08},
	{0x0b2d, 0x08},
	{0x0b2e, 0x00},
	{0x0b30, 0x14},
	{0x0b31, 0x14},
	{0x0b32, 0x00},
	{0x0b34, 0x10},
	{0x0b35, 0x10},
	{0x0b36, 0x00},
	{0x0b38, 0x18},
	{0x0b39, 0x18},
	{0x0b3a, 0x00},
	{0x0b3c, 0x16},
	{0x0b3d, 0x16},
	{0x0b3e, 0x00},
	{0x0b80, 0x01},
	{0x0b81, 0x00},
	{0x0b82, 0x00},
	{0x0b84, 0x00},
	{0x0b85, 0x00},
	{0x0b86, 0x00},
	{0x0b88, 0x01},
	{0x0b89, 0x68},
	{0x0b8a, 0x00},
	{0x0b8c, 0x00},
	{0x0b8d, 0x01},
	{0x0b8e, 0x00},
	{0x0b90, 0x01},
	{0x0b91, 0xf6},
	{0x0b92, 0x00},
	{0x0b94, 0x00},
	{0x0b95, 0x02},
	{0x0b96, 0x00},
	{0x0b98, 0x02},
	{0x0b99, 0xcd},
	{0x0b9a, 0x00},
	{0x0b9c, 0x00},
	{0x0b9d, 0x03},
	{0x0b9e, 0x00},
	{0x0ba0, 0x03},
	{0x0ba1, 0xdc},
	{0x0ba2, 0x00},
	{0x0ba4, 0x00},
	{0x0ba5, 0x04},
	{0x0ba6, 0x00},
	{0x0ba8, 0x05},
	{0x0ba9, 0x53},
	{0x0baa, 0x00},
	{0x0bac, 0x00},
	{0x0bad, 0x05},
	{0x0bae, 0x00},
	{0x0bb0, 0x07},
	{0x0bb1, 0x16},
	{0x0bb2, 0x00},
	{0x0bb4, 0x00},
	{0x0bb5, 0x06},
	{0x0bb6, 0x00},
	{0x0bb8, 0x09},
	{0x0bb9, 0xcc},
	{0x0bba, 0x00},
	{0x0bbc, 0x09},
	{0x0bbd, 0x36},
	{0x0bbe, 0x00},
	{0x0bc0, 0x0d},
	{0x0bc1, 0x82},
	{0x0bc2, 0x00},
	{0x0bc4, 0x10},
	{0x0bc5, 0x06},
	{0x0bc6, 0x00},
	{0x02c1, 0xe0},
	{0x0207, 0x04},
	{0x02c2, 0x10},
	{0x02c3, 0x54},
	{0x02C5, 0x09},
/*auto load CH_GAIN*/
	{0x0aa1, 0x15},
	{0x0aa2, 0x50},
	{0x0aa3, 0x00},
	{0x0aa4, 0x09},
	{0x0a90, 0x25},
	{0x0a91, 0x0e},
	{0x0a94, 0x80},
/*ISP*/
	{0x0050, 0x00},
	{0x0089, 0x83},
	{0x005a, 0x40},
	{0x00c3, 0x35},
	{0x00c4, 0x80},
	{0x0080, 0x10},
	{0x0040, 0x12},
/*gain*/
	{0x0204, 0x04},
	{0x0205, 0x00},
	{0x02b3, 0x00},
	{0x02b4, 0x00},
	{0x009e, 0x01},
	{0x009f, 0x94},
/*OUT 1296x972*/
	{0x0350, 0x01},
	{0x0353, 0x00},
	{0x0354, 0x04},
	{0x034c, 0x05},
	{0x034d, 0x10},
	{0x021f, 0x14},
/*auto load REG*/
	{0x0aa1, 0x10},
	{0x0aa2, 0xf8},
	{0x0aa3, 0x00},
	{0x0aa4, 0x0a},
	{0x0a90, 0x11},
	{0x0a91, 0x0e},
	{0x0a94, 0x80},
	{0x03fe, 0x00},
	{0x03fe, 0x00},
	{0x03fe, 0x00},
	{0x03fe, 0x00},
	{0x0a94, 0x00},
	{0x0af4, 0x29},
/*DPHY*/
	{0x0d80, 0x07},
	{0x0dd3, 0x18},
/*MIPI*/
	{0x0107, 0x05},
	{0x0117, 0x01},
	{0x0d81, 0x00},
	{0x0d84, 0x06},
	{0x0d85, 0x54},
	{0x0d86, 0x03},
	{0x0d87, 0x2b},
	{0x0db3, 0x03},
	{0x0db4, 0x04},
	{0x0db5, 0x0d},
	{0x0db6, 0x01},
	{0x0db8, 0x04},
	{0x0db9, 0x06},
	{0x0d93, 0x03},
	{0x0d94, 0x04},
	{0x0d95, 0x05},
	{0x0d99, 0x06},
	{0x0084, 0x11},
/*CISCTL_Reset*/
	{0x031c, 0x12},
	{0x031c, 0x80},
	{0x03fe, 0x10},
	{0x03fe, 0x00},
	{0x031c, 0x93},
	{0x03fe, 0x00},
	{0x03fe, 0x00},
	{0x03fe, 0x00},
	{0x03fe, 0x00},
	{0x031c, 0x80},
	{0x03fe, 0x10},
	{0x03fe, 0x00},
	{0x031c, 0x93},
/*OUT*/
	{0x0110, 0x01},
	{0x0117, 0x91},
};
#endif

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

static int gc05a2_sensor_vts;
static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	unsigned char explow, exphigh;
	unsigned int all_exp, frame_length;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_set_exp = %d, Done!\n", exp_val);
	all_exp = exp_val / 16;
	if (all_exp > 0x7fef)
		all_exp = 0x7fef;
	else if (all_exp < 1)
		all_exp = 1;

	frame_length = (all_exp + 16) > 2032 ? (all_exp + 16) : 2032;

	if (frame_length > 0x7fff)
		frame_length = 0x7fff;

	exphigh  = (unsigned char) ((frame_length >> 8) & 0xff);
	explow  = (unsigned char) (frame_length & 0xff);
	sensor_write(sd, 0x0340, exphigh);
	sensor_write(sd, 0x0341, explow);

	sensor_write(sd, 0x0202, (all_exp >> 8) & 0xff);
	sensor_write(sd, 0x0203, all_exp & 0xff);

	sensor_dbg("sensor_s_exp %d\n", all_exp);

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
	struct sensor_info *info = to_state(sd);
	unsigned int All_gain;

	if (gain_val < 16)
		gain_val = 16;
	if (gain_val > 16 * 64)
		gain_val = 16 * 64;

	if (gain_val == 16 * 64)
		All_gain = 0xffff;
	else
		All_gain = gain_val << 6;

	sensor_write(sd, 0x0204, (All_gain >> 8) & 0xff);
	sensor_write(sd, 0x0205, All_gain & 0xff);
	sensor_dbg("sensor_s_gain %d\n", All_gain);

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

	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);
	sensor_dbg("sensor_set_gain exp = %d, gain = %d Done!\n", exp_val, gain_val);
	info->exp = exp_val;
	info->gain = gain_val;
	return 0;
}

static void sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{

}

/*
 * Stuff that knows about the sensor.
 */
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	switch (on) {
	case STBY_ON:
		sensor_dbg("STBY_ON!\n");
		cci_lock(sd);
		sensor_s_sw_stby(sd, STBY_ON);
		usleep_range(1000, 1200);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		sensor_dbg("STBY_OFF!\n");
		cci_lock(sd);
		usleep_range(1000, 1200);
		sensor_s_sw_stby(sd, STBY_OFF);
		cci_unlock(sd);
		break;
	case PWR_ON:
		sensor_dbg("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		//vin_set_pmu_vol(sd, DVDD, VDD_1200MV);
		usleep_range(100, 120);

		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_set_pmu_channel(sd, CAMERAVDD, ON);
		usleep_range(12000, 14000);
		vin_set_pmu_channel(sd, IOVDD, ON);
		usleep_range(100, 120);

		vin_set_pmu_channel(sd, DVDD, ON);
		usleep_range(100, 120);

		vin_set_pmu_channel(sd, AVDD, ON);
		//vin_set_pmu_channel(sd, CAMERAVDD, ON);
		usleep_range(200, 220);

		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(100, 120);

		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(100, 120);

		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(300, 310);

		//vin_set_pmu_channel(sd, CAMERAVDD, ON);/*AFVCC ON*/
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");
		cci_lock(sd);
		usleep_range(100, 120);

		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(100, 120);

		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(100, 120);

		vin_set_mclk(sd, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		usleep_range(100, 120);

		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		vin_set_pmu_channel(sd, CAMERAVDD, OFF);/*AFVCC ON*/
		//vin_set_pmu_channel(sd, AFVDD, OFF);
		usleep_range(100, 120);

		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(100, 120);


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
	sensor_read(sd, 0x03f0, &rdval);
	if (rdval != (V4L2_IDENT_SENSOR >> 8)) {
		sensor_err(" read 0x03f0 return 0x%x\n", rdval);
		return -ENODEV;
	}

	sensor_read(sd, 0x03f1, &rdval);
	if (rdval != (V4L2_IDENT_SENSOR & 0xff)) {
		sensor_err(" read 0x03f1 return 0x%x\n", rdval);
		return -ENODEV;
	}

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
	info->width = 2592;
	info->height = 1944;
	info->hflip = 0;
	info->vflip = 0;

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
		.mbus_code = MEDIA_BUS_FMT_SGRBG10_1X10,
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
		.width      = 2592,
		.height     = 1944,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 5862,
		.vts        = 2032,
		.pclk       = 357347520,
		.mipi_bps   = 896*1000*1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 2032 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 64 << 4,
		.regs       = sensor_2592x1944p30_regs,
		.regs_size  = ARRAY_SIZE(sensor_2592x1944p30_regs),
		.set_size   = NULL,
	},
#if VIN_FALSE
	{
		.width      = 1296,
		.height     = 972,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 5862,
		.vts        = 2032,
		.pclk       = 357347520,
		.mipi_bps   = 448*1000*1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 2032 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 64 << 4,
		.regs       = sensor_1296x972p30_regs,
		.regs_size  = ARRAY_SIZE(sensor_1296x972p30_regs),
		.set_size   = NULL,
	},
#endif
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_CSI2_DPHY;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	cfg->bus.mipi_csi2.num_data_lanes = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
#else
	cfg->flags = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
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
	struct sensor_exp_gain exp_gain;

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
	gc05a2_sensor_vts = wsize->vts;
	exp_gain.exp_val = info->exp;
	exp_gain.gain_val = info->gain;
	if (exp_gain.exp_val == 0 || exp_gain.gain_val == 0) {
		exp_gain.exp_val = 16000;
		exp_gain.gain_val = 256;
	}
	sensor_s_exp_gain(sd, &exp_gain);
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

static int sensor_dev_id;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
static int sensor_probe(struct i2c_client *client)
#else
static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
#endif
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	int i;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;

	mutex_init(&info->lock);
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
#if IS_ENABLED(CONFIG_SAME_I2C)
	info->sensor_i2c_addr = I2C_ADDR >> 1;
#endif
	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET3 | MIPI_NORMAL_MODE;
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->af_first_flag = 1;

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
static int sensor_remove(struct i2c_client *client)
#else
static void sensor_remove(struct i2c_client *client)
#endif
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

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
	return 0;
#endif
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
