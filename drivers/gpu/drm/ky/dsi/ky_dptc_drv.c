// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#include "ky_dptc_drv.h"

#define I2C_NO 0
#define TOP_SLAVE_ADDR 0x30
#define DSI_SLAVE_ADDR 0x33

#ifdef DPTC_DPHY_TEST

static struct twsi_data dptc_i2c_data;
static DEFINE_MUTEX(cmd_mutex);

static int twsi_write_i2c(struct twsi_data *data)
{
	int ret = 0;
	struct i2c_adapter *adapter;
	struct i2c_msg msg;
	u8 val[8];
	int i ,j =0;

	if (!data || !data->addr || !data->reg_len || !data->val_len) {
		pr_err("Error: %s, %d", __func__, __LINE__);
		return -EINVAL;
	}

	msg.addr = data->addr;
	msg.flags = 0;
	msg.len = data->reg_len + data->val_len;
	msg.buf = val;

	adapter = i2c_get_adapter(data->twsi_no);
	if (adapter == NULL)
		return -1;

	mutex_lock(&cmd_mutex);
	for (i = 0; i < data->reg_len; i++)
		val[j++]=((u8*)(&data->reg))[i];
	for (i = 0; i < data->val_len; i++)
		val[j++]=((u8*)(&data->val))[i];
	ret = i2c_transfer(adapter, &msg, 1);
	if (ret < 0) {
		mutex_unlock(&cmd_mutex);
		return ret;
	}
	mutex_unlock(&cmd_mutex);

	return ret;
}

static int twsi_read_i2c(struct twsi_data *data)
{
	struct i2c_adapter *adapter;
	struct i2c_msg msg;
	int ret = 0;
	u8 val[4];

	if (!data || !data->addr || !data->reg_len || !data->val_len) {
		pr_err("%s, error param", __func__);
		return -EINVAL;
	}

	msg.addr = data->addr;
	msg.flags = 0;
	msg.len = data->reg_len;
	msg.buf = val;

	adapter = i2c_get_adapter(data->twsi_no);
	if (adapter == NULL)
		return -1;

	mutex_lock(&cmd_mutex);
	if (data->reg_len == I2C_8BIT)
		val[0] = data->reg & 0xff;
	else if (data->reg_len == I2C_16BIT) {
		val[0] = (data->reg >> 8) & 0xff;
		val[1] = data->reg & 0xff;
	}
	msg.len = data->reg_len;
	ret = i2c_transfer(adapter, &msg, 1);
	if (ret < 0) {
		mutex_unlock(&cmd_mutex);
		goto err;
	}

	msg.flags = I2C_M_RD;
	msg.len = data->val_len;
	ret = i2c_transfer(adapter, &msg, 1);
	if (ret < 0) {
		mutex_unlock(&cmd_mutex);
		goto err;
	}

	if (data->val_len == I2C_8BIT)
		data->val = val[0];
	else if (data->val_len == I2C_16BIT)
		data->val = (val[0] << 8) + val[1];
	else if (data->val_len == I2C_32BIT)
		data->val = (val[3] << 24) + (val[2] << 16) + (val[1] << 8) + val[0];
	//pr_info("twsi_read_i2c: val[0]=0x%x,val[1]=0x%x,val[2]=0x%x,val[3]=0x%x\n",val[0],val[1],val[2],val[3]);
	mutex_unlock(&cmd_mutex);

	return 0;

err:
	pr_info("Failed reading register 0x%02x!", data->reg);
	return ret;
}

static void TWSI_Init(I2C_FAST_MODE mode, unsigned int i2c_no)
{
	dptc_i2c_data.twsi_no = i2c_no;
	dptc_i2c_data.addr = 0x6c;
	dptc_i2c_data.reg_len = 1;//I2C_8BIT;
	dptc_i2c_data.val_len = 4;//I2C_32BIT;
}

static int TWSI_REG_WRITE_DPTC(uint8_t i2c_no, uint8_t slaveaddress, uint8_t addr, uint32_t data)
{
	int ret=0;

	dptc_i2c_data.twsi_no = i2c_no;
	dptc_i2c_data.addr = slaveaddress;
	dptc_i2c_data.reg = addr;
	dptc_i2c_data.val = data;
	ret = twsi_write_i2c(&dptc_i2c_data);
	if(0==ret)
		return 1;
	else
		return -1;
}

static int TWSI_REG_READ_DPTC(uint8_t i2c_no, uint8_t slaveaddress, uint8_t addr)
{
	int ret=0;

	dptc_i2c_data.twsi_no = i2c_no;
	dptc_i2c_data.addr = slaveaddress;
	dptc_i2c_data.reg = addr;
	dptc_i2c_data.val = 0x00;
	ret = twsi_read_i2c(&dptc_i2c_data);
	if(0==ret)
		return dptc_i2c_data.val;
	else
		return -1;
}


void dptc_top_write(uint32_t reg, uint32_t data)
{
	int ret = 0;
	uint32_t rd_data;

	ret = TWSI_REG_WRITE_DPTC(I2C_NO, TOP_SLAVE_ADDR, reg >> 2, data);
	rd_data = TWSI_REG_READ_DPTC(I2C_NO, TOP_SLAVE_ADDR, reg >> 2);
	if(rd_data != data)
		pr_err("fb: %s failed,  [0x%x] = 0x%x\n", __func__, reg, data);
}

uint32_t dptc_top_read(uint32_t reg)
{
	uint32_t data = 0;

	data = TWSI_REG_READ_DPTC(I2C_NO, TOP_SLAVE_ADDR, reg >> 2);
	pr_debug("fb: %s [0x%x] = 0x%x\n", __func__, reg, data);

	return data;
}

void dptc_dsi_write(uint32_t reg, uint32_t data)
{
	int ret = 0;
	uint32_t rd_data;

	ret = TWSI_REG_WRITE_DPTC(I2C_NO, DSI_SLAVE_ADDR, reg >> 2, data);
	rd_data = TWSI_REG_READ_DPTC(I2C_NO, DSI_SLAVE_ADDR, reg >> 2);
	if(rd_data != data)
		pr_err("fb: %s failed,  [0x%x] = 0x%x\n", __func__, reg, data);
}

uint32_t dptc_dsi_read(uint32_t reg)
{
	uint32_t data = 0;

	data = TWSI_REG_READ_DPTC(I2C_NO, DSI_SLAVE_ADDR, reg >> 2);

	pr_debug("fb: %s [0x%x] = 0x%x\n", __func__, reg, data);
	return data;
}

void dptc_board_init(void)
{
	uint32_t data;

	TWSI_Init(STANDARD_MODE, I2C_NO);
	msleep(100);

	data = dptc_top_read(0x04);    // enter function 4
	data |= 0x4;
	dptc_top_write(0x4, data);

	data = dptc_top_read(0x8);    // pll control
	data |= 0x1;
	dptc_top_write(0x8, data);

	data = dptc_top_read(0x18);    // clk reset
	data |= 0x4;
	dptc_top_write(0x18, data);

	dptc_top_write(0x0c, 0x3e627627);// setting pll   1.62G // success
	dptc_top_write(0x10, 0x428D4420);

	data = dptc_top_read(0x14);    // enable ck_dsi
	data |= (0xff00);            // ???
	dptc_top_write(0x14, data);

	data = dptc_top_read(0x08);    // power up PLL
	data |= 0x02;
}

#endif
