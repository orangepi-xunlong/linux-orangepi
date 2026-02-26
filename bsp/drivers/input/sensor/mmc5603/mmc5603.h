/*
 * Copyright (C) 2010 AKM, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;
 *
 */

/*
 * Definitions for MMC5603X magnetic sensor chip.
 */
#ifndef __MMC5603X_H__
#define __MMC5603X_H__

#include <linux/ioctl.h>

#define MMC5603X_I2C_NAME		"mmc5603x"


#define MMC5603X_REG_DATA		    0x00
#define MMC5603X_REG_STATUS1	  0x18
#define MMC5603X_REG_STATUS0		0x19

#define MMC5603X_REG_ODR				0x1A
#define MMC5603X_REG_CTRL0			0x1B
#define MMC5603X_REG_CTRL1			0x1C
#define MMC5603X_REG_CTRL2			0x1D

#define MMC5603X_REG_X_THD			0x1E
#define MMC5603X_REG_Y_THD			0x1F
#define MMC5603X_REG_Z_THD			0x20

#define MMC5603X_REG_ST_X_VAL		0x27
#define MMC5603X_REG_ST_Y_VAL		0x28
#define MMC5603X_REG_ST_Z_VAL		0x29


#define MMC5603X_REG_PRODUCTID_1		0x39

/* Bit definition for control register ODR 0x1A */
#define MMC5603X_CMD_ODR_1HZ			0x01
#define MMC5603X_CMD_ODR_5HZ			0x05
#define MMC5603X_CMD_ODR_10HZ		0x0A
#define MMC5603X_CMD_ODR_50HZ		0x32
#define MMC5603X_CMD_ODR_100HZ		0x64
#define MMC5603X_CMD_ODR_200HZ		0xC8
#define MMC5603X_CMD_ODR_255HZ		0xFF

/* Bit definition for control register 0 0x1B */
#define MMC5603X_CMD_TMM				0x01
#define MMC5603X_CMD_TMT         	0x02
#define MMC5603X_CMD_START_MDT		0x04
#define MMC5603X_CMD_SET				0x08
#define MMC5603X_CMD_RESET			0x10
#define MMC5603X_CMD_AUTO_SR_EN		0x20
#define MMC5603X_CMD_AUTO_ST_EN		0x40
#define MMC5603X_CMD_CMM_FREQ_EN		0x80

/* Bit definition for control register 1 0x1C */
#define MMC5603X_CMD_BW00			0x00
#define MMC5603X_CMD_BW01			0x01
#define MMC5603X_CMD_BW10			0x02
#define MMC5603X_CMD_BW11			0x03
#define MMC5603X_CMD_ST_ENP			0x20
#define MMC5603X_CMD_ST_ENM			0x40
#define MMC5603X_CMD_SW_RST			0x80

/* Bit definition for control register 2 0x1D */
#define MMC5603X_CMD_PART_SET1		0x00
#define MMC5603X_CMD_PART_SET25		0x01
#define MMC5603X_CMD_PART_SET75		0x02
#define MMC5603X_CMD_PART_SET100		0x03
#define MMC5603X_CMD_PART_SET250		0x04
#define MMC5603X_CMD_PART_SET500		0x05
#define MMC5603X_CMD_PART_SET1000	0x06
#define MMC5603X_CMD_PART_SET2000	0x07
#define MMC5603X_CMD_EN_PART_SET		0x08
#define MMC5603X_CMD_CMM_EN			0x10
#define MMC5603X_CMD_INT_MDT_EN		0x20
#define MMC5603X_CMD_INT_MD_EN		0x40
#define MMC5603X_CMD_HPOWER			0x80

#define MMC5603X_PRODUCT_ID			0x10
#define MMC5603X_MM_DONE_INT			0x01
#define MMC5603X_MT_DONE_INT			0x02
#define MMC5603X_MDT_FLAG_INT		0x04
#define MMC5603X_ST_FAIL_INT			0x08
#define MMC5603X_OTP_READ_DONE		0x10
#define MMC5603X_SAT_SENSOR			0x20
#define MMC5603X_MM_DONE				0x40
#define MMC5603X_MT_DONE				0x80

// 16-bit mode, null field output (32768)
#define	MMC5603X_16BIT_OFFSET		32768
#define	MMC5603X_16BIT_SENSITIVITY	1024



#define CONVERT_M			25
#define CONVERT_M_DIV		8192
#define CONVERT_O			45
#define CONVERT_O_DIV		8192

/* sensitivity 512 count = 1 Guass = 100uT*/

#define MMC5603X_OFFSET_X		32768
#define MMC5603X_OFFSET_Y		32768
#define MMC5603X_OFFSET_Z		32768
#define MMC5603X_SENSITIVITY_X		1024
#define MMC5603X_SENSITIVITY_Y		1024
#define MMC5603X_SENSITIVITY_Z		1024

#define MSENSOR						   0x83
#define MMC5603X_IOC_SET_DELAY         _IOW(MSENSOR, 0x29, short)
#define MMC31XX_IOC_TM					_IO(MSENSOR, 0x18)
#define MMC31XX_IOC_SET					_IO(MSENSOR, 0x19)
#define MMC31XX_IOC_RM					_IO(MSENSOR, 0x25)
#define MMC31XX_IOC_RESET				_IO(MSENSOR, 0x1a)
#define MMC31XX_IOC_RRM					_IO(MSENSOR, 0x26)
#define MMC31XX_IOC_READ				_IOR(MSENSOR, 0x1b, int[3])
#define MMC31XX_IOC_READXYZ				_IOR(MSENSOR, 0x1c, int[3])
#define ECOMPASS_IOC_GET_DELAY			_IOR(MSENSOR, 0x1d, int)
#define ECOMPASS_IOC_SET_YPR			_IOW(MSENSOR, 0x21, int[12])
#define ECOMPASS_IOC_GET_OPEN_STATUS	_IOR(MSENSOR, 0x20, int)
#define ECOMPASS_IOC_GET_MFLAG			_IOR(MSENSOR, 0x1e, short)
#define	ECOMPASS_IOC_GET_OFLAG			_IOR(MSENSOR, 0x1f, short)
#define MSENSOR_IOCTL_READ_SENSORDATA	_IOR(MSENSOR, 0x03, int)
#define ECOMPASS_IOC_GET_LAYOUT			_IOR(MSENSOR, 0X22, int)
#define MMC5603X_IOC_READ_REG		    _IOWR(MSENSOR, 0x23, unsigned char)
#define MMC5603X_IOC_WRITE_REG		    _IOW(MSENSOR,  0x24, unsigned char[2])
#define MMC5603X_IOC_READ_REGS		    _IOWR(MSENSOR, 0x25, unsigned char[10])
#define MSENSOR_IOCTL_SENSOR_ENABLE         _IOW(MSENSOR, 0x51, int)
#define ECOMPASS_IOC_SET_ACC_DATA               _IOW(MSENSOR, 0x52, int[3])
#define ECOMPASS_IOC_GET_ACC_DATA               _IOR(MSENSOR, 0x53, int[3])
#define MSENSOR_IOCTL_READ_FACTORY_SENSORDATA  _IOW(MSENSOR, 0x54, int)
#define MSENSOR_IOCTL_MSENSOR_ENABLE             _IOW(MSENSOR, 0x55, int)
#define MSENSOR_IOCTL_OSENSOR_ENABLE             _IOW(MSENSOR, 0x56, int)


#endif /* __MMC5603X_H__ */
