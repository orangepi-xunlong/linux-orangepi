/*
 * A V4L2 driver for nvp6324 cameras and AHD Coax protocol.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Li Huiyu <lihuiyu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/string.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

extern struct i2c_client *jaguar1_client;

void __I2CWriteByte8(unsigned char chip_addr, unsigned char reg_addr, unsigned char value)
{
	int ret;
	unsigned char buf[2];
	struct i2c_client *client = jaguar1_client;

	client->addr = chip_addr>>1;

	buf[0] = reg_addr;
	buf[1] = value;

	ret = i2c_master_send(client, buf, 2);
	udelay(300);
}

unsigned char __I2CReadByte8(unsigned char chip_addr, unsigned char reg_addr)
{
	struct i2c_client *client = jaguar1_client;

	client->addr = chip_addr>>1;

	return i2c_smbus_read_byte_data(client, reg_addr);
}
