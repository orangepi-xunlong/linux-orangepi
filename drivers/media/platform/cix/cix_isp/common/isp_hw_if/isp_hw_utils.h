/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ISP_HW_UTILS__
#define __ISP_HW_UTILS__
#include "armcb_isp.h"
#include <linux/i2c.h>

#define ARMCN_I2CSEND_BUFLENS_MAX 255

struct armcb_i2c_reg_ctrl {
	struct i2c_client *client;
	unsigned int data_bytes;
	unsigned int addr_bytes;
	unsigned int reg_data;
	unsigned int reg_addr;
	unsigned int slave_addr;
	/*fpga hardware channel*/
	unsigned int hw_chnl;
};

struct armcb_spi_reg_ctrl {
	struct spi_device *client;
	unsigned int data_bytes;
	unsigned int addr_bytes;
	unsigned int reg_data;
	unsigned int reg_addr;
	unsigned char hw_chnl;
};

/*fpga i2c channel select*/
void armcb_hwchnnel_select(u8 hwchnl);
/*fpga get current i2c channel*/
unsigned char armcb_get_sensor_hwchnl(void);

void armcb_spi_set_hwchnl(unsigned char ucTarget);
unsigned char armcb_spi_get_hwchnl(void);

int armcb_i2c_register_read(struct i2c_client *client,
			    struct cmd_i2c_setting *i2c_settings);
int armcb_i2c_register_write(struct i2c_client *client,
			     struct cmd_i2c_setting *i2c_settings);
int armcb_spi_register_read(struct spi_device *client,
			    struct cmd_spi_setting *spi_settings);
int armcb_spi_register_write(struct spi_device *client,
			     struct cmd_spi_setting *spi_settings);
void armcb_route_i2c1toslavex(u32 unSlaveNo);

#endif
