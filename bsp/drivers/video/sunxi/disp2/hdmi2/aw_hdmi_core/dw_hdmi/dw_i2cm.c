/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <sunxi-log.h>
#include <linux/delay.h>
#include "dw_mc.h"
#include "dw_access.h"
#include "dw_i2cm.h"

#define I2C_DIV_FACTOR	 100000

/**
 * calculate the fast sped high time counter - round up
 */
static u16 _i2cm_scl_calc(u16 sfrClock, u16 sclMinTime)
{
	unsigned long tmp_scl_period = 0;

	if (((sfrClock * sclMinTime) % I2C_DIV_FACTOR) != 0) {
		tmp_scl_period = (unsigned long)((sfrClock * sclMinTime)
			+ (I2C_DIV_FACTOR
			- ((sfrClock * sclMinTime) % I2C_DIV_FACTOR)))
			/ I2C_DIV_FACTOR;
	} else {
		tmp_scl_period = (unsigned long)(sfrClock * sclMinTime)
							/ I2C_DIV_FACTOR;
	}
	return (u16)(tmp_scl_period);
}

static void _i2cm_fast_speed_high_clk_ctrl(dw_hdmi_dev_t *dev, u16 value)
{
	LOG_TRACE2(I2CM_FS_SCL_HCNT_1_ADDR, value);

	dev_write(dev, I2CM_FS_SCL_HCNT_1_ADDR, (u8) (value >> 8));
	dev_write(dev, I2CM_FS_SCL_HCNT_0_ADDR, (u8) (value >> 0));
}

static void _i2cm_fast_speed_low_clk_ctrl(dw_hdmi_dev_t *dev, u16 value)
{
	LOG_TRACE2((I2CM_FS_SCL_LCNT_1_ADDR), value);
	dev_write(dev, I2CM_FS_SCL_LCNT_1_ADDR, (u8) (value >> 8));
	dev_write(dev, I2CM_FS_SCL_LCNT_0_ADDR, (u8) (value >> 0));
}

static void _i2cm_standard_speed_high_clk_ctrl(dw_hdmi_dev_t *dev, u16 value)
{
	LOG_TRACE2((I2CM_SS_SCL_HCNT_1_ADDR), value);
	dev_write(dev, I2CM_SS_SCL_HCNT_1_ADDR, (u8) (value >> 8));
	dev_write(dev, I2CM_SS_SCL_HCNT_0_ADDR, (u8) (value >> 0));
}

static void _i2cm_standard_speed_low_clk_ctrl(dw_hdmi_dev_t *dev, u16 value)
{
	LOG_TRACE2((I2CM_SS_SCL_LCNT_1_ADDR), value);
	dev_write(dev, I2CM_SS_SCL_LCNT_1_ADDR, (u8) (value >> 8));
	dev_write(dev, I2CM_SS_SCL_LCNT_0_ADDR, (u8) (value >> 0));
}

static int _i2cm_write(dw_hdmi_dev_t *dev, u8 i2cAddr, u8 addr, u8 data)
{
	int timeout = I2CDDC_TIMEOUT;
	u32 status = 0;

	LOG_TRACE1(addr);

	dev_write_mask(dev, I2CM_SLAVE, I2CM_SLAVE_SLAVEADDR_MASK, i2cAddr);
	dev_write(dev, I2CM_ADDRESS, addr);
	dev_write(dev, I2CM_DATAO, data);
	dev_write(dev, I2CM_OPERATION, I2CM_OPERATION_WR_MASK);
	do {
		udelay(10);
		status = dw_mc_get_i2cm_irq_mask(dev);
	} while (status == 0 && (timeout--));

	dev_write(dev, IH_I2CM_STAT0, status); /* clear read status */

	if (status & IH_I2CM_STAT0_I2CMASTERERROR_MASK) {
		hdmi_err("%s: I2C DDC Write failed for i2cAddr 0x%x addr 0x%x data 0x%x\n",
				__func__, i2cAddr, addr, data);
		return -1;
	}

	if (status & IH_I2CM_STAT0_I2CMASTERDONE_MASK)
		return 0;

	hdmi_err("%s: ASSERT I2C DDC Write timeout! status: 0x%x\n", __func__, status);
	return -1;
}

static int _i2cm_read(dw_hdmi_dev_t *dev, u8 i2cAddr, u8 segment,
					u8 pointer, u8 addr, u8 *value)
{
	int timeout = I2CDDC_TIMEOUT;
	u32 status = 0;

	LOG_TRACE1(addr);
	dev_write_mask(dev, I2CM_SLAVE, I2CM_SLAVE_SLAVEADDR_MASK, i2cAddr);
	dev_write(dev, I2CM_ADDRESS, addr);
	dev_write(dev, I2CM_SEGADDR, segment);
	dev_write(dev, I2CM_SEGPTR, pointer);

	if (pointer)
		dev_write(dev, I2CM_OPERATION, I2CM_OPERATION_RD_EXT_MASK);
	else
		dev_write(dev, I2CM_OPERATION, I2CM_OPERATION_RD_MASK);

	do {
		udelay(10);
		status = dev_read_mask(dev, IH_I2CM_STAT0,
					IH_I2CM_STAT0_I2CMASTERERROR_MASK
					| IH_I2CM_STAT0_I2CMASTERDONE_MASK);
	} while (status == 0 && (timeout--));

	dev_write(dev, IH_I2CM_STAT0, status); /* clear read status */

	if (status & IH_I2CM_STAT0_I2CMASTERERROR_MASK) {
		hdmi_err("%s: I2C DDC Read failed for i2cAddr 0x%x seg 0x%x pointer 0x%x addr 0x%x\n",
				__func__, i2cAddr, segment, pointer, addr);
		return -1;
	}

	if (status & IH_I2CM_STAT0_I2CMASTERDONE_MASK) {
		*value = (u8) dev_read(dev, I2CM_DATAI);
		return 0;
	}

	hdmi_err("%s: ASSERT I2C DDC Read timeout! status: 0x%x\n", __func__, status);
	return -1;
}

static int _i2cm_read8(dw_hdmi_dev_t *dev, u8 i2cAddr, u8 segment,
					u8 pointer, u8 addr, u8 *value)
{
	int timeout = I2CDDC_TIMEOUT;
	u32 status = 0;

	LOG_TRACE1(addr);
	dev_write_mask(dev, I2CM_SLAVE, I2CM_SLAVE_SLAVEADDR_MASK, i2cAddr);
	dev_write(dev, I2CM_SEGADDR, segment);
	dev_write(dev, I2CM_SEGPTR, pointer);
	dev_write(dev, I2CM_ADDRESS, addr);

	if (pointer)
		dev_write(dev, I2CM_OPERATION, I2CM_OPERATION_RD8_EXT_MASK);
	else
		dev_write(dev, I2CM_OPERATION, I2CM_OPERATION_RD8_MASK);

	do {
		udelay(10);
		status = dev_read_mask(dev, IH_I2CM_STAT0,
				IH_I2CM_STAT0_I2CMASTERERROR_MASK |
				IH_I2CM_STAT0_I2CMASTERDONE_MASK);
	} while (status == 0 && (timeout--));

	dev_write(dev, IH_I2CM_STAT0, status); /* clear read status */

	if (status & IH_I2CM_STAT0_I2CMASTERERROR_MASK) {
		hdmi_err("%s: I2C DDC Read8 extended failed for i2cAddr 0x%x seg 0x%x pointer 0x%x addr 0x%x\n",
				__func__, i2cAddr, segment, pointer, addr);
		return -1;
	}

	if (status & IH_I2CM_STAT0_I2CMASTERDONE_MASK) {
		int i = 0;

		while (i < 8) { /* read 8 bytes */
			value[i] = (u8) dev_read(dev,
					I2CM_READ_BUFF0 + (4 * i));
			i += 1;
		}
		return 0;
	}

	hdmi_err("%s: ASSERT I2C DDC Read extended timeout! status: 0x%x\n", __func__, status);
	return -1;
}

void dw_i2cm_ddc_clk_config(dw_hdmi_dev_t *dev, u16 sfrClock,
	u16 ss_low_ckl, u16 ss_high_ckl, u16 fs_low_ckl, u16 fs_high_ckl)
{
	_i2cm_standard_speed_low_clk_ctrl(dev, _i2cm_scl_calc(sfrClock, ss_low_ckl));
	_i2cm_standard_speed_high_clk_ctrl(dev, _i2cm_scl_calc(sfrClock, ss_high_ckl));
	_i2cm_fast_speed_low_clk_ctrl(dev, _i2cm_scl_calc(sfrClock, fs_low_ckl));
	_i2cm_fast_speed_high_clk_ctrl(dev, _i2cm_scl_calc(sfrClock, fs_high_ckl));
}

void ctrl_i2cm_ddc_fast_mode(dw_hdmi_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	/* bit 4 selects between high and standard speed operation */
	dev_write_mask(dev, I2CM_DIV, I2CM_DIV_FAST_STD_MODE_MASK, value);
}

void dw_i2cm_ddc_interrupt_mask(dw_hdmi_dev_t *dev, u8 mask)
{
	LOG_TRACE1(mask);

	dev_write_mask(dev, I2CM_INT, I2CM_INT_DONE_MASK, mask ? 1 : 0);
	dev_write_mask(dev, I2CM_CTLINT,
				I2CM_CTLINT_ARBITRATION_MASK, mask ? 1 : 0);
	dev_write_mask(dev, I2CM_CTLINT, I2CM_CTLINT_NACK_MASK, mask ? 1 : 0);
}


int dw_i2cm_ddc_write(dw_hdmi_dev_t *dev, u8 i2cAddr, u8 addr, u8 len, u8 *data)
{
	int i, status = 0;

	for (i = 0; i < len; i++) {
		int tries = 3;

		do {
			status = _i2cm_write(dev, i2cAddr, addr, data[i]);
		} while (status && tries--);

		if (status) /* Error after 3 failed writes */
			return status;
	}
	return 0;
}

int dw_i2cm_ddc_read(dw_hdmi_dev_t *dev, u8 i2cAddr, u8 segment,
		u8 pointer, u8 addr, u8 len, u8 *data)
{
	int i, status = 0;

	for (i = 0; i < len;) {
		int tries = 3;

		if ((len - i) >= 8) {
			do {
				status = _i2cm_read8(dev, i2cAddr, segment,
						pointer, addr + i,  &(data[i]));
			} while (status && tries--);

			if (status) /* Error after 3 failed writes */
				return status;
			i += 8;
		} else {
			do {
				status = _i2cm_read(dev, i2cAddr, segment,
						pointer, addr + i,  &(data[i]));
			} while (status && tries--);

			if (status) /* Error after 3 failed writes */
				return status;
			i++;
		}
	}
	return 0;
}
