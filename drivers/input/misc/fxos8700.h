/*
 *  fxos8700.h - Linux kernel modules for 3-Axis M+G Sensor
 *
 *  Copyright (C) 2012 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef SENSOR_FXOS8700_H
#define SENSOR_FXOS8700_H

#define FXOS8700_I2C_ADDR		0x1E
#define FXOS8700_DEVICE_ID		0xC7
#define POLL_INTERVAL_MIN		1
#define POLL_INTERVAL_MAX		500
#define POLL_INTERVAL			100 	/* msecs */
#define FXOS8700_DEV_NAME		"fxos8700"

// if sensor is standby ,set POLL_STOP_TIME to slow down the poll
#define POLL_STOP_TIME			100

#define MODE_CHANGE_DELAY_MS		100
#define FXOS8700_DATA_BUF_SIZE		6

/*register define*/
#define FXOS8700_STATUS 		0x00
#define FXOS8700_OUT_X_MSB 		0x01
#define FXOS8700_OUT_X_LSB 		0x02
#define FXOS8700_OUT_Y_MSB 		0x03
#define FXOS8700_OUT_Y_LSB 		0x04
#define FXOS8700_OUT_Z_MSB 		0x05
#define FXOS8700_OUT_Z_LSB 		0x06
#define FXOS8700_F_SETUP 		0x09
#define FXOS8700_TRIG_CFG 		0x0a
#define FXOS8700_SYSMOD			0x0B
#define FXOS8700_INT_SOURCE		0x0c
#define FXOS8700_WHO_AM_I		0x0d
#define FXOS8700_XYZ_DATA_CFG		0x0e
#define FXOS8700_HP_FILTER_CUTOFF	0x0f
#define FXOS8700_PL_STATUS 		0x10
#define FXOS8700_PL_CFG 		0x11
#define FXOS8700_PL_COUNT		0x12
#define FXOS8700_PL_BF_ZCOMP		0x13
#define FXOS8700_PL_P_L_THS_REG		0x14
#define FXOS8700_FFMT_CFG		0x15
#define FXOS8700_FFMT_SRC		0x16
#define FXOS8700_FFMT_THS		0x17
#define FXOS8700_FFMT_COUNT		0x18
#define FXOS8700_TRANSIDENT1_CFG	0x1d
#define FXOS8700_TRANSIDENT1_SRC	0x1e
#define FXOS8700_TRANSIDENT1_THS	0x1f
#define FXOS8700_TRANSIDENT1_COUNT	0x20
#define FXOS8700_PULSE_CFG		0x21
#define FXOS8700_PULSE_SRC		0x22
#define FXOS8700_PULSE_THSX		0x23
#define FXOS8700_PULSE_THSY		0x24
#define FXOS8700_PULSE_THSZ		0x25
#define FXOS8700_PULSE_TMLT		0x26
#define FXOS8700_PULSE_LTCY		0x27
#define FXOS8700_PULSE_WIND		0x28
#define FXOS8700_ATSLP_COUNT		0x29
#define FXOS8700_CTRL_REG1		0x2a
#define FXOS8700_CTRL_REG2		0x2b
#define FXOS8700_CTRL_REG3		0x2c
#define FXOS8700_CTRL_REG4		0x2d
#define FXOS8700_CTRL_REG5		0x2e
#define FXOS8700_OFF_X			0x2f
#define FXOS8700_OFF_Y			0x30
#define FXOS8700_OFF_Z			0x31
#define FXOS8700_M_DR_STATUS		0x32
#define FXOS8700_M_OUT_X_MSB       	0x33
#define FXOS8700_M_OUT_X_LSB       	0x34
#define FXOS8700_M_OUT_Y_MSB       	0x35
#define FXOS8700_M_OUT_Y_LSB       	0x36
#define FXOS8700_M_OUT_Z_MSB       	0x37
#define FXOS8700_M_OUT_Z_LSB       	0x38
#define FXOS8700_CMP_X_MSB       	0x39
#define FXOS8700_CMP_X_LSB       	0x3a
#define FXOS8700_CMP_Y_MSB       	0x3b
#define FXOS8700_CMP_Y_LSB       	0x3c
#define FXOS8700_CMP_Z_MSB       	0x3d
#define FXOS8700_CMP_Z_LSB       	0x3e
#define FXOS8700_M_OFF_X_MSB       	0x3f
#define FXOS8700_M_OFF_X_LSB       	0x40
#define FXOS8700_M_OFF_Y_MSB       	0x41
#define FXOS8700_M_OFF_Y_LSB       	0x42
#define FXOS8700_M_OFF_Z_MSB       	0x43
#define FXOS8700_M_OFF_Z_LSB       	0x44
#define FXOS8700_MAX_X_MSB       	0x45
#define FXOS8700_MAX_X_LSB      	0x46
#define FXOS8700_MAX_Y_MSB       	0x47
#define FXOS8700_MAX_Y_LSB       	0x48
#define FXOS8700_MAX_Z_MSB       	0x49
#define FXOS8700_MAX_Z_LSB       	0x4a
#define FXOS8700_MIN_X_MSB       	0x4b
#define FXOS8700_MIN_X_LSB       	0x4c  //product 0x1e
#define FXOS8700_MIN_Y_MSB       	0x4d  //product 0x1d
#define FXOS8700_MIN_Y_LSB       	0x4e  //product 0x1c
#define FXOS8700_MIN_Z_MSB       	0x4f  //product 0x1f
#define FXOS8700_MIN_Z_LSB       	0x50
#define FXOS8700_M_TEMP       		0x51
#define FXOS8700_MAG_THS_CFG 		0x52
#define FXOS8700_MAG_THS_SRC 		0x53
#define FXOS8700_MAG_THS_THS_X1 	0x54
#define FXOS8700_MAG_THS_THS_X0 	0x55
#define FXOS8700_MAG_THS_THS_Y1 	0x56
#define FXOS8700_MAG_THS_THS_Y0 	0x57
#define FXOS8700_MAG_THS_THS_Z1 	0x58
#define FXOS8700_MAG_THS_THS_Z0 	0x59
#define FXOS8700_MAG_THS_CUNT		0x5a
#define FXOS8700_M_CTRL_REG1		0x5b
#define FXOS8700_M_CTRL_REG2		0x5c
#define FXOS8700_M_CTRL_REG3		0x5d
#define FXOS8700_M_INT_SOURCE		0x5e
#define FXOS8700_G_VECM_CFG  		0x5f
#define FXOS8700_G_VECM_THS_MSB  	0x60
#define FXOS8700_G_VECM_THS_LSB  	0x61
#define FXOS8700_G_VECM_CNT  		0x62
#define FXOS8700_G_VECM_INITX_MSB  	0x63
#define FXOS8700_G_VECM_INITX_LSB  	0x64
#define FXOS8700_G_VECM_INITY_MSB  	0x65
#define FXOS8700_G_VECM_INITY_LSB  	0x66
#define FXOS8700_G_VECM_INITZ_MSB  	0x67
#define FXOS8700_G_VECM_INITZ_LSB  	0x68
#define FXOS8700_M_VECM_CFG  		0x69
#define FXOS8700_M_VECM_THS_MSB 	0x6a
#define FXOS8700_M_VECM_THS_LSB  	0x6b
#define FXOS8700_M_VECM_CNT  		0x6d
#define FXOS8700_M_VECM_INITX_MSB  	0x6d
#define FXOS8700_M_VECM_INITX_LSB  	0x6e
#define FXOS8700_M_VECM_INITY_MSB  	0x6f
#define FXOS8700_M_VECM_INITY_LSB  	0x70
#define FXOS8700_M_VECM_INITZ_MSB  	0x71
#define FXOS8700_M_VECM_INITZ_LSB  	0x72
#define FXOS8700_G_FFMT_THS_X1  	0x73
#define FXOS8700_G_FFMT_THS_X0 		0x74
#define FXOS8700_G_FFMT_THS_Y1  	0x75
#define FXOS8700_G_FFMT_THS_Y0  	0x76
#define FXOS8700_G_FFMT_THS_Z1  	0x77
#define FXOS8700_G_FFMT_THS_Z0  	0x78
#define FXOS8700_G_TRAN_INIT_MSB  	0x79
#define FXOS8700_G_TRAN_INIT_LSB_X 	0x7a
#define FXOS8700_G_TRAN_INIT_LSB_Y 	0x7b
#define FXOS8700_G_TRAN_INIT_LSB_Z 	0x7d
#define FXOS8700_TM_NVM_LOCK 		0x7e
#define FXOS8700_NVM_DATA0_35		0x80
#define FXOS8700_NVM_DATA_BNK3		0xa4
#define FXOS8700_NVM_DATA_BNK2		0xa5
#define FXOS8700_NVM_DATA_BNK1		0xa6
#define FXOS8700_NVM_DATA_BNK0		0xa7 

#endif
