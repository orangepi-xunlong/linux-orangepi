/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * A V4L2 driver for tc358743 HDMI to MIPI.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors: Li Huiyu <lihuiyu@allwinnertrch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "../../../utility/vin_log.h"
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

#include "../camera.h"
#include "../sensor_helper.h"
#include "../../../vin-cci/cci_helper.h"
#include "tc358743xbg_reg.h"

//DEFINE_MUTEX(tc_i2c_lock);

static u32 lowestBitSet(u32 x)
{
	u32 result = 0;

	/* x=0 is not properly handled by while-loop */
	if (x == 0)
		return 0;

	while ((x & 1) == 0) {
		x >>= 1;
		result++;
}

return result;
}

static void tc_filp(unsigned int *data, unsigned char len)
{
	if (len == 2) {
		*data = ((*data & 0xff00) >> 8) | ((*data & 0xff) << 8);
	} else if (len == 4) {
		*data = ((*data & 0xff00) << 8) | ((*data & 0xff) << 24)
			|  ((*data & 0xff0000) >> 8) | ((*data & 0xff000000) >> 24);
	}
}

int tc_write8(struct v4l2_subdev *sd, unsigned short addr, unsigned char data)
{
	int ret = 0;
	if (!sd)
		return -1;

//	mutex_lock(&tc_i2c_lock);
	ret = cci_write_a16_d8(sd, addr, data);
	if (ret < 0)
		sensor_err("%s failed, addr:%04x value:0x%02x\n", __func__, addr, data);
//	mutex_unlock(&tc_i2c_lock);

	return ret;
}

int tc_write16(struct v4l2_subdev *sd, unsigned short addr, unsigned int data)
{
	int ret = 0;

	if (!sd)
		return -1;

	tc_filp(&data, 2);

//	mutex_lock(&tc_i2c_lock);
	ret = cci_write_a16_d16(sd, addr, data);
	if (ret < 0)
		sensor_err("%s failed, addr:%04x value:0x%04x\n", __func__, addr, data);
//	mutex_unlock(&tc_i2c_lock);

	return ret;
}

int tc_write32(struct v4l2_subdev *sd, unsigned short addr, unsigned int data)
{
	int ret = 0;

	if (!sd)
		return -1;

	tc_filp(&data, 4);

//	mutex_lock(&tc_i2c_lock);
	ret = cci_write_a16_d32(sd, addr, data);
	if (ret < 0)
		sensor_err("%s failed, addr:%04x value:0x%08x\n", __func__, addr, data);
//	mutex_unlock(&tc_i2c_lock);

	return ret;
}

int tc_reg_read(struct v4l2_subdev *sd, unsigned short addr,
		int *data, unsigned char data_len, unsigned char dir)
{
	int ret = -1;

	if (!sd || !data)
		return ret;

	if (data_len != 2 && data_len != 4)
		return ret;

/*	if (dir) {
		tc_filp((unsigned int *)&addr, 2);
	}
*/
//	mutex_lock(&tc_i2c_lock);
	if (data_len == 2)
		ret = cci_read_a16_d16(sd, addr, (unsigned short *)data);
	else  if (data_len == 4)
		ret = cci_read_a16_d32(sd, addr, data);
//	mutex_unlock(&tc_i2c_lock);

	if (ret < 0)
		sensor_err("%s failed, addr:%04x\n", __func__, addr);

	if (dir)
		tc_filp(data, data_len);

	return ret;
}

unsigned char tc_read8(struct v4l2_subdev *sd, unsigned short addr)
{
	unsigned int data = 0;
	int ret;

//mutex_lock(&tc_i2c_lock);
	ret = cci_read_a16_d8(sd, addr, (unsigned char *)&data);
	if (ret < 0)
		sensor_err("%s failed, addr:%04x\n", __func__, addr);
//	mutex_unlock(&tc_i2c_lock);

	return (unsigned char)data;
}

unsigned short tc_read16(struct v4l2_subdev *sd, unsigned short addr)
{
	unsigned int data = 0;
    int ret;

//	mutex_lock(&tc_i2c_lock);
	ret = cci_read_a16_d16(sd, addr, (unsigned short *)&data);
	if (ret < 0)
		sensor_err("%s failed, addr:%04x\n", __func__, addr);
//	mutex_unlock(&tc_i2c_lock);

	tc_filp(&data,  2);

	return (unsigned short)data;
}

unsigned int tc_read32(struct v4l2_subdev *sd, unsigned short addr)
{
	unsigned int data = 0;
    int ret;

//	mutex_lock(&tc_i2c_lock);
	ret = cci_read_a16_d32(sd, addr, &data);
	if (ret < 0)
		sensor_err("%s failed, addr:%04x\n", __func__, addr);
//	mutex_unlock(&tc_i2c_lock);

	tc_filp(&data,  4);

	return data;
}

void tc_write8_mask(struct v4l2_subdev *sd, u16 reg,
		u8 mask, u8 val)
{
	u8 temp = 0;
	u32 shift = lowestBitSet(mask);

	temp = tc_read8(sd, reg);
	temp &= ~(mask);
	temp |= (mask & (val << shift));
	tc_write8(sd, reg, temp);
}

void tc_write16_mask(struct v4l2_subdev *sd, u16 reg,
		u16 mask, u16 val)
{
	u16 temp = 0;
	u32 shift = lowestBitSet(mask);

	temp = tc_read16(sd, reg);
	temp &= ~(mask);
	temp |= (mask & (val << shift));
	tc_write16(sd, reg, temp);
}

void tc_write32_mask(struct v4l2_subdev *sd, u16 reg,
		u32 mask, u32 val)
{
	u32 temp = 0;
	u32 shift = lowestBitSet(mask);

	temp = tc_read32(sd, reg);
	temp &= ~(mask);
	temp |= (mask & (val << shift));
	tc_write32(sd, reg, temp);
}
