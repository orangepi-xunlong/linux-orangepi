/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

/*
 * A V4L2 driver for N5 YUV cameras.
 *
 * Copyright (c) 2019 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Hong Yi <hongyi@allwinnertech.com>
 *           Cao FuMing <caofuming@allwinnertech.com>
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

MODULE_AUTHOR("hy");
MODULE_DESCRIPTION("A low-level driver for N5 sensors");
MODULE_LICENSE("GPL");

#define MCLK              (27*1000*1000)
#define CLK_POL V4L2_MBUS_PCLK_SAMPLE_FALLING  //V4L2_MBUS_PCLK_SAMPLE_FALLING | V4L2_MBUS_PCLK_SAMPLE_RISING
#define V4L2_IDENT_SENSOR 0xe0

#define BT656   0
#define BT1120  1

typedef enum _n5_outmode_sel {
	N5_OUTMODE_1MUX_SD = 0,
	N5_OUTMODE_1MUX_HD,
	N5_OUTMODE_1MUX_FHD,
	N5_OUTMODE_1MUX_FHD_HALF,
	N5_OUTMODE_2MUX_SD,
	N5_OUTMODE_2MUX_HD,
	N5_OUTMODE_2MUX_FHD,
	N5_OUTMODE_1MUX_BT1120S,
	N5_OUTMODE_2MUX_BT1120S_720P,
	N5_OUTMODE_2MUX_BT1120S_1080P,
	N5_OUTMODE_BUTT
} N5_OUTMODE_SEL;

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 25

/*
 * The N5 sits on i2c with ID 0x64 or 0x66
 * SAD-low:0x64 SAD-high:0x66
 */
#define I2C_ADDR 0x64
#define SENSOR_NAME "n5"

void n5_regs_init_common(struct v4l2_subdev *sd)
{
	sensor_write(sd, 0xff, 0x00);
	sensor_write(sd, 0x00, 0x10);
	sensor_write(sd, 0x01, 0x10);
	sensor_write(sd, 0x18, 0x3f);
	sensor_write(sd, 0x19, 0x3f);
	sensor_write(sd, 0x22, 0x0b);
	sensor_write(sd, 0x23, 0x71);
	sensor_write(sd, 0x26, 0x0b);
	sensor_write(sd, 0x27, 0x71);
	sensor_write(sd, 0x54, 0x00);
	sensor_write(sd, 0xa0, 0x05);
	sensor_write(sd, 0xa1, 0x05);
	sensor_write(sd, 0xff, 0x05);
	sensor_write(sd, 0x00, 0xf0);
	sensor_write(sd, 0x01, 0x22);
	sensor_write(sd, 0x05, 0x04);
	sensor_write(sd, 0x08, 0x55);
	sensor_write(sd, 0x1b, 0x08);
	sensor_write(sd, 0x25, 0xdc);
	sensor_write(sd, 0x28, 0x80);
	sensor_write(sd, 0x2f, 0x00);
	sensor_write(sd, 0x30, 0xe0);
	sensor_write(sd, 0x31, 0x43);
	sensor_write(sd, 0x32, 0xa2);
	sensor_write(sd, 0x57, 0x00);
	sensor_write(sd, 0x58, 0x77);
	sensor_write(sd, 0x5b, 0x41);
	sensor_write(sd, 0x5c, 0x78);
	sensor_write(sd, 0x5f, 0x00);
	sensor_write(sd, 0x7b, 0x11);
	sensor_write(sd, 0x7c, 0x01);
	sensor_write(sd, 0x7d, 0x80);
	sensor_write(sd, 0x80, 0x00);
	sensor_write(sd, 0x90, 0x01);
	sensor_write(sd, 0xa9, 0x00);
	sensor_write(sd, 0xb5, 0x00);
	sensor_write(sd, 0xb9, 0x72);
	sensor_write(sd, 0xd1, 0x00);
	sensor_write(sd, 0xd5, 0x80);
	sensor_write(sd, 0xff, 0x06);
	sensor_write(sd, 0x00, 0xf0);
	sensor_write(sd, 0x01, 0x22);
	sensor_write(sd, 0x05, 0x04);
	sensor_write(sd, 0x08, 0x55);
	sensor_write(sd, 0x1b, 0x08);
	sensor_write(sd, 0x25, 0xdc);
	sensor_write(sd, 0x28, 0x80);
	sensor_write(sd, 0x2f, 0x00);
	sensor_write(sd, 0x30, 0xe0);
	sensor_write(sd, 0x31, 0x43);
	sensor_write(sd, 0x32, 0xa2);
	sensor_write(sd, 0x57, 0x00);
	sensor_write(sd, 0x58, 0x77);
	sensor_write(sd, 0x5b, 0x41);
	sensor_write(sd, 0x5c, 0x78);
	sensor_write(sd, 0x5f, 0x00);
	sensor_write(sd, 0x7b, 0x11);
	sensor_write(sd, 0x7c, 0x01);
	sensor_write(sd, 0x7d, 0x80);
	sensor_write(sd, 0x80, 0x00);
	sensor_write(sd, 0x90, 0x01);
	sensor_write(sd, 0xa9, 0x00);
	sensor_write(sd, 0xb5, 0x00);
	sensor_write(sd, 0xb9, 0x72);
	sensor_write(sd, 0xd1, 0x00);
	sensor_write(sd, 0xd5, 0x80);
	sensor_write(sd, 0xff, 0x09);
	sensor_write(sd, 0x50, 0x30);
	sensor_write(sd, 0x51, 0x6f);
	sensor_write(sd, 0x52, 0x67);
	sensor_write(sd, 0x53, 0x48);
	sensor_write(sd, 0x54, 0x30);
	sensor_write(sd, 0x55, 0x6f);
	sensor_write(sd, 0x56, 0x67);
	sensor_write(sd, 0x57, 0x48);
	sensor_write(sd, 0x96, 0x00);
	sensor_write(sd, 0x9e, 0x00);
	sensor_write(sd, 0xb6, 0x00);
	sensor_write(sd, 0xbe, 0x00);
	sensor_write(sd, 0xff, 0x0a);
	sensor_write(sd, 0x25, 0x10);
	sensor_write(sd, 0x27, 0x1e);
	sensor_write(sd, 0x30, 0xac);
	sensor_write(sd, 0x31, 0x78);
	sensor_write(sd, 0x32, 0x17);
	sensor_write(sd, 0x33, 0xc1);
	sensor_write(sd, 0x34, 0x40);
	sensor_write(sd, 0x35, 0x00);
	sensor_write(sd, 0x36, 0xc3);
	sensor_write(sd, 0x37, 0x0a);
	sensor_write(sd, 0x38, 0x00);
	sensor_write(sd, 0x39, 0x02);
	sensor_write(sd, 0x3a, 0x00);
	sensor_write(sd, 0x3b, 0xb2);
	sensor_write(sd, 0xa5, 0x10);
	sensor_write(sd, 0xa7, 0x1e);
	sensor_write(sd, 0xb0, 0xac);
	sensor_write(sd, 0xb1, 0x78);
	sensor_write(sd, 0xb2, 0x17);
	sensor_write(sd, 0xb3, 0xc1);
	sensor_write(sd, 0xb4, 0x40);
	sensor_write(sd, 0xb5, 0x00);
	sensor_write(sd, 0xb6, 0xc3);
	sensor_write(sd, 0xb7, 0x0a);
	sensor_write(sd, 0xb8, 0x00);
	sensor_write(sd, 0xb9, 0x02);
	sensor_write(sd, 0xba, 0x00);
	sensor_write(sd, 0xbb, 0xb2);
	sensor_write(sd, 0x77, 0x8F);
	sensor_write(sd, 0xF7, 0x8F);
	sensor_write(sd, 0xff, 0x13);
	sensor_write(sd, 0x07, 0x47);
	sensor_write(sd, 0x12, 0x04);
	sensor_write(sd, 0x1e, 0x1f);
	sensor_write(sd, 0x1f, 0x27);
	sensor_write(sd, 0x2e, 0x10);
	sensor_write(sd, 0x2f, 0xc8);
	sensor_write(sd, 0x30, 0x00);
	sensor_write(sd, 0x31, 0xff);
	sensor_write(sd, 0x32, 0x00);
	sensor_write(sd, 0x33, 0x00);
	sensor_write(sd, 0x3a, 0xff);
	sensor_write(sd, 0x3b, 0xff);
	sensor_write(sd, 0x3c, 0xff);
	sensor_write(sd, 0x3d, 0xff);
	sensor_write(sd, 0x3e, 0xff);
	sensor_write(sd, 0x3f, 0x0f);
	sensor_write(sd, 0x70, 0x00);
	sensor_write(sd, 0x72, 0x05);
	sensor_write(sd, 0x7A, 0xf0);
	sensor_write(sd, 0xff, 0x01);
	sensor_write(sd, 0x97, 0x00);
	sensor_write(sd, 0x97, 0x0f);
	sensor_write(sd, 0x7A, 0x0f);
	sensor_write(sd, 0xff, 0x00); //8x8 color block test pattern
	sensor_write(sd, 0x78, 0x22); //0xba -> 0x22
	sensor_write(sd, 0xff, 0x05);
	sensor_write(sd, 0x2c, 0x08);
	sensor_write(sd, 0x6a, 0x80);
	sensor_write(sd, 0xff, 0x06);
	sensor_write(sd, 0x2c, 0x08);
	sensor_write(sd, 0x6a, 0x80);
}

void n5_set_chn_1080p_30(struct v4l2_subdev *sd, unsigned char chn)
{
	data_type val;

	printk("%s chn=%d\n", __func__, chn);
	sensor_write(sd, 0xff, 0x00);
	sensor_write(sd, 0x08+chn, 0x00);
	sensor_write(sd, 0x34+chn, 0x00);
	sensor_write(sd, 0x81+chn, 0x02);
	sensor_write(sd, 0x85+chn, 0x00);
	sensor_read(sd, 0x54, &val);
	sensor_write(sd, 0x54, val&(~(0x10<<chn)));
	sensor_write(sd, 0x18+chn, 0x3f);
	sensor_write(sd, 0x58+chn, 0x78);
	sensor_write(sd, 0x5c+chn, 0x80);
	sensor_write(sd, 0x64+chn, 0x01);
	sensor_write(sd, 0x89+chn, 0x10);
	sensor_write(sd, chn+0x8e, 0x00);
	sensor_write(sd, 0x30+chn, 0x12);
	sensor_write(sd, 0xa0+chn, 0x05);

	sensor_write(sd, 0xff, 0x01);
	sensor_write(sd, 0x84+chn, 0x00);
	sensor_write(sd, 0x8c+chn, 0x40);
	sensor_read(sd, 0xed, &val);
	sensor_write(sd, 0xed, val&(~(0x10<<chn)));

	sensor_write(sd, 0xff, 0x05+chn);
	sensor_write(sd, 0x20, 0x84);
	sensor_write(sd, 0x25, 0xdc);
	sensor_write(sd, 0x28, 0x80);
	sensor_write(sd, 0x2b, 0xa8);
	sensor_write(sd, 0x47, 0xee);
	sensor_write(sd, 0x50, 0xc6);
	sensor_write(sd, 0x58, 0x77);
	sensor_write(sd, 0x69, 0x00);
	sensor_write(sd, 0x7b, 0x11);
	sensor_write(sd, 0xb8, 0x39);

#if 0
	sensor_write(sd, 0xff, 0x09);
	sensor_write(sd, 0x96, 0x00);
	sensor_write(sd, 0x97, 0x00);
	sensor_write(sd, 0x98, 0x00);
	sensor_write(sd, 0xb6, 0x00);
	sensor_write(sd, 0xb7, 0x00);
	sensor_write(sd, 0xb8, 0x00);
#endif

	//sensor_write(sd, 0xff, 0x13);
	//sensor_write(sd, 0x70, gpio_i2c_read(sd, 0x70)|(0x01<<chn));
}

void n5_set_chn_720p_25(struct v4l2_subdev *sd, unsigned char chn)
{
	data_type val;

	printk("%s chn=%d\n", __func__, chn);

	sensor_write(sd, 0xff, 0x00);
	sensor_write(sd, 0x08+chn, 0x00);
	sensor_write(sd, 0x34+chn, 0x00);
	sensor_write(sd, 0x81+chn, 0x07);
	sensor_write(sd, 0x85+chn, 0x00);
    sensor_read(sd, 0x54, &val);
	sensor_write(sd, 0x54, val&(~(0x10<<chn)));
	sensor_write(sd, 0x18+chn, 0x3f);
	sensor_write(sd, 0x58+chn, 0x74);
	sensor_write(sd, 0x5c+chn, 0x80);
	sensor_write(sd, 0x64+chn, 0x01);
	sensor_write(sd, 0x89+chn, 0x00);
	sensor_write(sd, chn+0x8e, 0x00);
	sensor_write(sd, 0x30+chn, 0x12);
	sensor_write(sd, 0xa0+chn, 0x05);

	sensor_write(sd, 0xff, 0x01);
	sensor_write(sd, 0x84+chn, 0x08);
	sensor_write(sd, 0x8c+chn, 0x08);
    sensor_read(sd, 0xed, &val);
	sensor_write(sd, 0xed, val&(~(0x01<<chn)));

	sensor_write(sd, 0xff, 0x05+chn);
	sensor_write(sd, 0x20, 0x84);
	sensor_write(sd, 0x25, 0xdc);
	sensor_write(sd, 0x28, 0x80);
	sensor_write(sd, 0x2b, 0xa8);
	sensor_write(sd, 0x47, 0x04);
	sensor_write(sd, 0x50, 0x84);
	sensor_write(sd, 0x58, 0x70);  // 0x70
	sensor_write(sd, 0x69, 0x00);
	sensor_write(sd, 0x7b, 0x11);
	sensor_write(sd, 0xb8, 0xb9);
}

void n5_set_portmode(struct v4l2_subdev *sd, unsigned char port, unsigned char muxmode, unsigned char is_bt601)
{
	data_type val_1xc8, val_1xca, val_0x54;
	unsigned char clk_freq_array[4] = {0x83, 0x03, 0x43, 0x63}; //clk_freq: 0~3:37.125M/74.25M/148.5M/297M
	sensor_write(sd, 0xff, 0x00);
	sensor_read(sd, 0x54, &val_0x54);
	if ((N5_OUTMODE_2MUX_SD == muxmode)  || (N5_OUTMODE_2MUX_HD == muxmode) ||
		(N5_OUTMODE_2MUX_FHD == muxmode) || (N5_OUTMODE_2MUX_BT1120S_720P == muxmode) ||
		(N5_OUTMODE_2MUX_BT1120S_1080P == muxmode))
		val_0x54 |= 0x01;
	else
		val_0x54 &= 0xFE;
	sensor_write(sd, 0x54, val_0x54);

	sensor_write(sd, 0xff, 0x01);
	sensor_read(sd, 0xc8, &val_1xc8);
	switch (muxmode) {
	case N5_OUTMODE_1MUX_SD:
		sensor_write(sd, 0xA0 + port, 0x00);
		sensor_write(sd, 0xC0, 0x00);
		sensor_write(sd, 0xC2, 0x11);
		val_1xc8 &= (1 == port ? 0x0F : 0xF0);
		sensor_write(sd, 0xC8, val_1xc8);
		sensor_write(sd, 0xCC + port, clk_freq_array[0]);
	break;
	case N5_OUTMODE_1MUX_HD:
		sensor_write(sd, 0xA0 + port, 0x00);
		sensor_write(sd, 0xC0, 0x00);
		sensor_write(sd, 0xC2, 0x11);
		val_1xc8 &= (1 == port ? 0x0F : 0xF0);
		sensor_write(sd, 0xC8, val_1xc8);
		sensor_write(sd, 0xCC + port, clk_freq_array[1]);
	break;
	case N5_OUTMODE_1MUX_FHD:
		sensor_write(sd, 0xA0 + port, 0x00);
		sensor_write(sd, 0xC0, 0x00);
		sensor_write(sd, 0xC2, 0x11);
		val_1xc8 &= (1 == port ? 0x0F : 0xF0);
		sensor_write(sd, 0xC8, val_1xc8);
		sensor_write(sd, 0xCC + port, clk_freq_array[2]);
	break;
	case N5_OUTMODE_1MUX_FHD_HALF:
		sensor_write(sd, 0xA0 + port, 0x00);
		sensor_write(sd, 0xC0, 0x88);
		sensor_write(sd, 0xC2, 0x99);
		val_1xc8 &= (1 == port ? 0x0F : 0xF0);
		sensor_write(sd, 0xC8, val_1xc8);
		sensor_write(sd, 0xCC + port, clk_freq_array[1]);
	break;
	case N5_OUTMODE_2MUX_SD:
		sensor_write(sd, 0xA0 + port, 0x20);
		sensor_write(sd, 0xC0, 0x10);
		sensor_write(sd, 0xC2, 0x10);
		val_1xc8 &= (1 == port ? 0x0F : 0xF0);
		val_1xc8 |= (1 == port ? 0x20 : 0x02);
		sensor_write(sd, 0xC8, val_1xc8);
		sensor_write(sd, 0xCC + port, clk_freq_array[1]);
	break;
	case N5_OUTMODE_2MUX_HD:
		sensor_write(sd, 0xA0 + port, 0x20);
		sensor_write(sd, 0xC0, 0x10);
		sensor_write(sd, 0xC2, 0x10);
		val_1xc8 &= (1 == port ? 0x0F : 0xF0);
		val_1xc8 |= (1 == port ? 0x20 : 0x02);
		sensor_write(sd, 0xC8, val_1xc8);
		sensor_write(sd, 0xCC + port, clk_freq_array[2]);
	break;
	case N5_OUTMODE_2MUX_FHD:
		sensor_write(sd, 0xA0 + port, 0x00);
		sensor_write(sd, 0xC0, 0x10);
		sensor_write(sd, 0xC2, 0x10);
		val_1xc8 &= (1 == port ? 0x0F : 0xF0);
		val_1xc8 |= (1 == port ? 0x20 : 0x02);
		sensor_write(sd, 0xC8, val_1xc8);
		sensor_write(sd, 0xCC + port, clk_freq_array[2]);
	break;
	case N5_OUTMODE_1MUX_BT1120S:
		sensor_write(sd, 0xA0, 0x00);
		sensor_write(sd, 0xA1, 0x00);
		sensor_write(sd, 0xC0, 0xCC);
		sensor_write(sd, 0xC1, 0xCC);
		sensor_write(sd, 0xC2, 0x44);
		sensor_write(sd, 0xC3, 0x44);
		sensor_write(sd, 0xC8, 0x00);
		sensor_write(sd, 0xCA, 0x33); //two ports are enabled
		sensor_write(sd, 0xCC, clk_freq_array[2]);
	break;
	case N5_OUTMODE_2MUX_BT1120S_720P:
		sensor_write(sd, 0xA0, 0x00);
		sensor_write(sd, 0xA1, 0x00);
		sensor_write(sd, 0xC0, 0xDC);  //C data
		sensor_write(sd, 0xC1, 0xDC);
		sensor_write(sd, 0xC2, 0x54); //Y data
		sensor_write(sd, 0xC3, 0x54);
		sensor_write(sd, 0xC8, 0x22);
		sensor_write(sd, 0xCA, 0x33); //two ports are enabled
		sensor_write(sd, 0xCC, clk_freq_array[1]);
	break;
	case N5_OUTMODE_2MUX_BT1120S_1080P:
		sensor_write(sd, 0xA0, 0x20);
		sensor_write(sd, 0xA1, 0x20);
		sensor_write(sd, 0xC0, 0xDC);  //C data
		sensor_write(sd, 0xC1, 0xDC);
		sensor_write(sd, 0xC2, 0x54); //Y data
		sensor_write(sd, 0xC3, 0x54);
		sensor_write(sd, 0xC8, 0x22);
		sensor_write(sd, 0xCA, 0x33); //two ports are enabled
		sensor_write(sd, 0xCC, clk_freq_array[1]);
	break;
	}
	if (1 == is_bt601) {
		sensor_write(sd, 0xA8 + port, 0x90 + (port*0x10));	//h/v0 sync enabled
		//sensor_write(sd, 0xA9, 0xA0); //h/v1 sync enabled
		//sensor_write(sd, 0xBC, 0x10);	//h/v0 swap enabled
		//sensor_write(sd, 0xBD, 0x10);
		//sensor_write(sd, 0xBE, 0x10);	//h/v1 swap enabled
		//sensor_write(sd, 0xBF, 0x10);
	} else {
		sensor_write(sd, 0xA8, 0x00);
		//sensor_write(sd, 0xA9, 0x00);  //h/v sync disable.
	}

	if (N5_OUTMODE_2MUX_BT1120S_720P == muxmode) {
		sensor_write(sd, 0xE4, 0x11);
		sensor_write(sd, 0xE5, 0x11);
	} else {
		sensor_write(sd, 0xE4, 0x00);
		sensor_write(sd, 0xE5, 0x00);
	}

	sensor_read(sd, 0xca, &val_1xca);
	val_1xca |= (0x11 << port); //enable port
	sensor_write(sd, 0xCA, val_1xca);
}

void n5_1080pX1(struct v4l2_subdev *sd)
{

	printk("----------------------%s-------------------\n", __func__);
	n5_regs_init_common(sd);
	//n5_set_chn_1080p_25(sd, 0);
	//n5_set_chn_1080p_25(sd, 1);
	n5_set_chn_1080p_30(sd, 0);
	//n5_set_chn_1080p_30(sd, 1);
	n5_set_portmode(sd, 0, N5_OUTMODE_1MUX_FHD, BT656);
	//n5_set_portmode(sd, 1, N5_OUTMODE_1MUX_FHD, BT656);
}

void n5_720pX2(struct v4l2_subdev *sd)
{

	printk("----------------------%s-------------------\n", __func__);
	n5_regs_init_common(sd);
	n5_set_chn_720p_25(sd, 0);
	n5_set_chn_720p_25(sd, 1);
	n5_set_portmode(sd, 0, N5_OUTMODE_2MUX_HD, BT656);
	n5_set_portmode(sd, 1, N5_OUTMODE_2MUX_HD, BT656);
}

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	if (on_off)
		vfe_gpio_write(sd, RESET, CSI_GPIO_LOW);
	else
		vfe_gpio_write(sd, RESET, CSI_GPIO_HIGH);
	return 0;
}

static int sensor_power(struct v4l2_subdev *sd, int on)
{
	switch (on) {
	case CSI_SUBDEV_STBY_ON:
		vfe_print("CSI_SUBDEV_STBY_ON!\n");
		sensor_s_sw_stby(sd, ON);
		break;
	case CSI_SUBDEV_STBY_OFF:
		vfe_print("CSI_SUBDEV_STBY_OFF!\n");
		sensor_s_sw_stby(sd, OFF);
		break;
	case CSI_SUBDEV_PWR_ON:
		vfe_print("CSI_SUBDEV_PWR_ON!\n");
		cci_lock(sd);
//		vfe_gpio_set_status(sd, PWDN, 1);
		vfe_gpio_set_status(sd, RESET, 1);
//		vfe_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vfe_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		vfe_set_mclk_freq(sd, MCLK);
		vfe_set_mclk(sd, ON);
		vfe_set_pmu_channel(sd, IOVDD, ON);
//		vfe_set_pmu_channel(sd, AVDD, ON);
//		vfe_set_pmu_channel(sd, DVDD, ON);
		usleep_range(10000, 12000);
		vfe_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(30000, 31000);
		cci_unlock(sd);
		break;
	case CSI_SUBDEV_PWR_OFF:
		vfe_print("CSI_SUBDEV_PWR_OFF!\n");
		cci_lock(sd);
		vfe_set_mclk(sd, OFF);
//		vfe_set_pmu_channel(sd, DVDD, OFF);
//		vfe_set_pmu_channel(sd, AVDD, OFF);
		vfe_set_pmu_channel(sd, IOVDD, OFF);
		usleep_range(100, 120);
//		vfe_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vfe_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vfe_gpio_set_status(sd, RESET, 0);
//		vfe_gpio_set_status(sd, PWDN, 0);
		cci_unlock(sd);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	vfe_print("sensor_reset--val = %d !\n", val);
	vfe_gpio_write(sd, RESET, CSI_GPIO_LOW);
	usleep_range(5000, 6000);
	vfe_gpio_write(sd, RESET, CSI_GPIO_HIGH);
	usleep_range(5000, 6000);
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	data_type id0;

	vfe_print("sensor_detect!\n");

	sensor_write(sd, 0xFF, 0x00);
	usleep_range(5000, 5100);
	sensor_read(sd, 0xF4, &id0);
	vfe_print("chip_id = 0x%x\n", id0);

	if (id0 != V4L2_IDENT_SENSOR) {
		vfe_print(KERN_DEBUG "sensor error,read id is 0x%x.\n", id0);
		return -ENODEV;
	} else {
		vfe_print(KERN_DEBUG "find n5 csi camera sensor success.\n");
		return 0;
	}
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	vfe_print("sensor_init\n");

	/*Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		vfe_print("chip found is not an target chip.\n");
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
			       sizeof(struct sensor_win_size));
			vfe_print("get wins success!\n");
			ret = 0;
		} else {
			vfe_print("empty wins!\n");
			ret = -1;
		}
		break;
	case SET_FPS:
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
	.desc = "BT656 2CH",
	.mbus_code = MEDIA_BUS_FMT_UYVY8_2X8,
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
#if 1
	{
	 .width = HD1080_WIDTH,
	 .height = HD1080_HEIGHT,
	 .hoffset = 0,
	 .voffset = 0,
	 .fps_fixed = 30,
	 .regs = NULL,
	 .regs_size = 0,
	 .set_size = NULL,
	},
#else
	{
	 .width = HD720_WIDTH,
	 .height = HD720_HEIGHT,
	 .hoffset = 0,
	 .voffset = 0,
	 .fps_fixed = 25,
	 .regs = NULL,
	 .regs_size = 0,
	 .set_size = NULL,
	},
#endif
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_enum_code(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg,
		 struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index >= N_FMTS)
		return -EINVAL;

	code->code = sensor_formats[code->index].mbus_code;
	return 0;
}

static int sensor_enum_frame_size(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_frame_size_enum *fse)
{
	if (fse->index > N_WIN_SIZES-1)
		return -EINVAL;

	fse->min_width = sensor_win_sizes[fse->index].width;
	fse->max_width = fse->min_width;
	fse->min_height = sensor_win_sizes[fse->index].height;
	fse->max_height = fse->min_height;

	return 0;
}

static int sensor_g_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				struct v4l2_mbus_config *cfg)
{
	struct sensor_info *info = to_state(sd);

	cfg->type = V4L2_MBUS_BT656;

	if (1920 == info->current_wins->width && 1080 == info->current_wins->height)
		cfg->flags = CLK_POL | CSI_CH_0;
	else
		cfg->flags = CLK_POL | CSI_CH_0 | CSI_CH_1;

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

	vfe_print("sensor_reg_init\n");

	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;

	if (1920 == info->width && 1080 == info->height)
		n5_1080pX1(sd);
	else
		n5_720pX2(sd);

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);

	vfe_print("%s on = %d, %d*%d %x\n", __func__, enable,
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
	.enum_mbus_code = sensor_enum_code,
	.enum_frame_size = sensor_enum_frame_size,
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

// static int sensor_init_controls(struct v4l2_subdev *sd,
// 				const struct v4l2_ctrl_ops *ops)
// {
// 	struct sensor_info *info = to_state(sd);
// 	struct v4l2_ctrl_handler *handler = &info->handler;
// 	struct v4l2_ctrl *ctrl;
// 	int ret = 0;

// 	v4l2_ctrl_handler_init(handler, 2);

// 	v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 1600,
// 			      256 * 1600, 1, 1 * 1600);
// 	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 0,
// 			      65536 * 16, 1, 0);
// 	if (ctrl != NULL)
// 		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

// 	if (handler->error) {
// 		ret = handler->error;
// 		v4l2_ctrl_handler_free(handler);
// 	}

// 	sd->ctrl_handler = handler;

// 	return ret;
// }

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	struct sensor_win_size *wsize;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);
//	sensor_init_controls(sd, &sensor_ctrl_ops);
	mutex_init(&info->lock);
	wsize = sensor_win_sizes;

#ifdef CONFIG_SAME_I2C
	info->sensor_i2c_addr = I2C_ADDR >> 1;
#endif
	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	info->current_wins = wsize;

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

//late_initcall(init_sensor);
module_init(init_sensor);
module_exit(exit_sensor);
