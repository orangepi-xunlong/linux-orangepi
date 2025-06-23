// SPDX-License-Identifier: GPL-2.0
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

#include "armcb_camera_io_drv.h"
#include "armcb_platform.h"
#include "armcb_register.h"
#include "isp_hw_ops.h"
#include "system_logger.h"
#include <linux/acpi.h>
#include <linux/clk.h>
#include <linux/compat.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_COMMON
#endif

static unsigned int bytes_of_reg_data(enum drv_data_type data_type)
{
	unsigned int bytes = 0;

	switch (data_type) {
	case DRV_DATA_TYPE_BYTE:
		bytes = 1;
		break;
	case DRV_DATA_TYPE_WORD:
		bytes = 2;
		break;
	case DRV_DATA_TYPE_WORD_REVERSE:
		bytes = 2;
		break;
	case DRV_DATA_TYPE_DWORD:
		bytes = 4;
		break;
	default:
		LOG(LOG_ERR, "Invalid i2c data bytes.");
		break;
	}

	return bytes;
}

static unsigned int bytes_of_reg_addr(enum drv_addr_type addr_type)
{
	unsigned int bytes = 0;

	switch (addr_type) {
	case DRV_ADDR_TYPE_BYTE:
		bytes = 1;
		break;
	case DRV_ADDR_TYPE_WORD:
		bytes = 2;
		break;
	case DRV_ADDR_TYPE_WORD_REVERSE:
		bytes = 2;
		break;
	default:
		LOG(LOG_ERR, "Invalid i2c address bytes. %d", addr_type);
		break;
	}

	return bytes;
}

static u8 g_cur_spi_ch;

// Select SPI slave to access for different sensor input channels.
void armcb_spi_set_hwchnl(unsigned char ch)
{
	u32 tmp = 0;

	if (ch == g_cur_spi_ch)
		return;

	tmp = armcb_apb2_read_reg(0x04);

	if (ch == 0) {
		tmp &= (~(0x01 << 6)); // Bit 6 for spi.
		g_cur_spi_ch = 0;
	} else {
		tmp |= ((0x01 << 6));
		g_cur_spi_ch = 1;
	}

	armcb_apb2_write_reg(0x04, tmp);
}

unsigned char armcb_spi_get_hwchnl(void)
{
	return g_cur_spi_ch;
}

int armcb_spi_register_read(struct spi_device *client,
			    struct cmd_spi_setting *spi_settings)
{
	unsigned short buf_addr = spi_settings->reg_addr;
	int ret = 0;

	struct spi_message msg;
	struct spi_transfer tx[] = {
		{
			.tx_buf = &buf_addr,
			.len = bytes_of_reg_addr(spi_settings->reg_addr_type),
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
			.delay.value = 1,
			.delay.unit = SPI_DELAY_UNIT_USECS,
#else
			.delay_usecs = 1,
#endif
		},
		{
			.rx_buf = &spi_settings->val,
			.len = bytes_of_reg_data(spi_settings->reg_data_type),
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
			.delay.value = 1,
			.delay.unit = SPI_DELAY_UNIT_USECS,
#else
			.delay_usecs = 1,
#endif
		},
	};

	armcb_spi_set_hwchnl(spi_settings->channel);

	spi_message_init(&msg);
	spi_message_add_tail(&tx[0], &msg);
	spi_message_add_tail(&tx[1], &msg);
	ret = spi_sync(client, &msg);

	return ret;
}

int armcb_spi_register_write(struct spi_device *client,
			     struct cmd_spi_setting *spi_settings)
{
	unsigned short buf_addr = spi_settings->reg_addr;
	unsigned short buf_value = spi_settings->val;
	int ret = 0;
	struct spi_message msg;

	struct spi_transfer tx[] = {
		{
			.tx_buf = &buf_addr,
			.len = bytes_of_reg_addr(spi_settings->reg_addr_type),
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
			.delay.value = 1,
			.delay.unit = SPI_DELAY_UNIT_USECS,
#else
			.delay_usecs = 1,
#endif
		},
		{
			.tx_buf = &buf_value,
			.len = bytes_of_reg_data(spi_settings->reg_data_type),
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
			.delay.value = 1,
			.delay.unit = SPI_DELAY_UNIT_USECS,
#else
			.delay_usecs = 1,
#endif
		},
	};

	armcb_spi_set_hwchnl(spi_settings->channel);

	spi_message_init(&msg);
	spi_message_add_tail(&tx[0], &msg);
	spi_message_add_tail(&tx[1], &msg);
	ret = spi_sync(client, &msg);

	return ret;
}

/* store current i2c channel */
static unsigned int g_cur_i2c_ch;
void armcb_hwchnnel_select(unsigned char hwchnl)
{
	unsigned int tmp = 0;

	if (hwchnl == g_cur_i2c_ch)
		return;
	/* for fpga to select channel*/
	tmp = armcb_apb2_read_reg(0x04);
	tmp &= (~(0x05 << 5));

	if (hwchnl == 1) {
		tmp |= (0x01 << 5);
		g_cur_i2c_ch = 1;
	} else if (hwchnl == 2) {
		tmp |= (0x04 << 5);
		g_cur_i2c_ch = 2;
	} else if (hwchnl == 3) {
		tmp |= (0x05 << 5);
		g_cur_i2c_ch = 3;
	} else {
		g_cur_i2c_ch = 0;
	}

	armcb_apb2_write_reg(0x04, tmp);
}

unsigned char armcb_get_sensor_hwchnl(void)
{
	return g_cur_i2c_ch;
}

int armcb_i2c_register_read(struct i2c_client *client,
			    struct cmd_i2c_setting *i2c_settings)
{
	int res = 0;
	char i2c_data[ARMCN_I2CSEND_BUFLENS_MAX] = { 0 };
	unsigned int addr_bytes = 0;
	unsigned int data_bytes = 0;
	unsigned int data_size = 0;
	char rev_data[4] = { 0 };

	if (!client || !i2c_settings) {
		LOG(LOG_ERR, "Invalid i2c arg !");
		return -EINVAL;
	}

	client->addr = i2c_settings->slave_addr;

	addr_bytes = bytes_of_reg_addr(i2c_settings->reg_addr_type);
	data_bytes = bytes_of_reg_data(i2c_settings->reg_data_type);

	switch (addr_bytes) {
	case 1:
		i2c_data[data_size++] = i2c_settings->reg_addr & 0xFF;
		break;
	case 2:
		i2c_data[data_size++] = (i2c_settings->reg_addr >> 8) & 0xFF;
		i2c_data[data_size++] = i2c_settings->reg_addr & 0xFF;
		break;
	case 4:
		i2c_data[data_size++] = (i2c_settings->reg_addr >> 24) & 0xFF;
		i2c_data[data_size++] = (i2c_settings->reg_addr >> 16) & 0xFF;
		i2c_data[data_size++] = (i2c_settings->reg_addr >> 8) & 0xFF;
		i2c_data[data_size++] = (i2c_settings->reg_addr >> 0) & 0xFF;
		break;
	default:
		LOG(LOG_ERR, "Invalid addr bytes,  bytes = %d!", addr_bytes);
		return -EINVAL;
	}

	if (i2c_master_send(client, (const char *)i2c_data, data_size) < 0) {
		LOG(LOG_ERR, "failed i2c_master_send, clinet %p", client);
		return -EIO;
	}

	if (i2c_master_recv(client, (char *)rev_data, data_bytes) < 0) {
		LOG(LOG_ERR, "failed i2c_master_recv, clinet %p", client);
		return -EIO;
	}

	switch (data_bytes) {
	case 1:
		i2c_settings->val = rev_data[0] & 0xFF;
		break;
	case 2:
		i2c_settings->val = (rev_data[0] << 8) | (rev_data[1] & 0xff);
		break;
	default:
		LOG(LOG_ERR, "Invalid databytes,  bytes = %d!", data_bytes);
		return -EINVAL;
	}

	return res;
}

int armcb_i2c_register_write(struct i2c_client *client,
			     struct cmd_i2c_setting *i2c_settings)
{
	int res = 0;
	char i2c_data[ARMCN_I2CSEND_BUFLENS_MAX] = { 0 };
	unsigned int addr_bytes = 0;
	unsigned int data_bytes = 0;
	unsigned int data_size = 0;

	if (!client || !i2c_settings) {
		LOG(LOG_ERR, "Invalid i2c arg !");
		return -EINVAL;
	}

	client->addr = i2c_settings->slave_addr;

	addr_bytes = bytes_of_reg_addr(i2c_settings->reg_addr_type);
	data_bytes = bytes_of_reg_data(i2c_settings->reg_data_type);

	switch (addr_bytes) {
	case 1:
		i2c_data[data_size++] = i2c_settings->reg_addr & 0xFF;
		break;
	case 2:
		i2c_data[data_size++] = (i2c_settings->reg_addr >> 8) & 0xFF;
		i2c_data[data_size++] = i2c_settings->reg_addr & 0xFF;
		break;
	case 4:
		i2c_data[data_size++] = (i2c_settings->reg_addr >> 24) & 0xFF;
		i2c_data[data_size++] = (i2c_settings->reg_addr >> 16) & 0xFF;
		i2c_data[data_size++] = (i2c_settings->reg_addr >> 8) & 0xFF;
		i2c_data[data_size++] = (i2c_settings->reg_addr >> 0) & 0xFF;
		break;
	default:
		LOG(LOG_ERR, "Invalid addr bytes,  bytes = %d!", addr_bytes);
		return -EINVAL;
	}

	switch (data_bytes) {
	case 1:
		i2c_data[data_size++] = i2c_settings->val & 0xFF;
		break;
	case 2:
		i2c_data[data_size++] = (i2c_settings->val >> 8) & 0xFF;
		i2c_data[data_size++] = i2c_settings->val & 0xFF;
		break;
	default:
		LOG(LOG_ERR, "Invalid databytes,  bytes = %d!", data_bytes);
		return -EINVAL;
	}

	if (i2c_master_send(client, (const char *)i2c_data, data_size) < 0) {
		pr_err("failed i2c_master_send, clinet %p", client);
		return -EIO;
	}

	return res;
}

//-------------------------------------------------------------------------
// Function name: armcb_route_i2c1toslavex()
// Description:   Mux I2C1 to Slave 0-9 on the XCVU440 FPGA board.
// Parameters:
//	u32 unSlaveNo: Slave No on the board.
//				   0x00 = adjclk1
//				   0x01 = adjclk2
//				   0x02 = adjclk3
//				   0x03 = adjclk4
//				   0x04 = adjclk5
//				   0x05 = adjclk6
//				   0x06 = J1_SCL
//				   0x07 = J2_SCL
//				   0x08 = J7_SCL
//				   0x0A = XCVU440.
//
//-------------------------------------------------------------------------
void armcb_route_i2c1toslavex(u32 unSlaveNo)
{
	armcb_register_set_int32(XPAR_REGCTRL16_0_S00_AXI_BASEADDR + 0x1C,
				 unSlaveNo);
}
