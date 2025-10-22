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


#ifndef _DW_SCDC_H_
#define _DW_SCDC_H_

#include "dw_dev.h"

#ifndef SUPPORT_ONLY_HDMI14
/* SCDC Registers */
#define SCDC_SLAVE_ADDRESS             0x54

#define SCDC_SINK_VER                  0x01	/* sink version */
#define SCDC_SOURCE_VER                0x02	/* source version */
#define SCDC_UPDATE_0                  0x10	/* Update_0 */
	#define BIT_SCDC_UPDATE_0_STATUS           0x01 /* Status update flag */
	#define BIT_SCDC_UPDATE_0_CED              0x02 /* Character error update flag */
	#define BIT_SCDC_UPDATE_0_RR_TEST          0x04 /* Read request test */
#define SCDC_UPDATE_1                  0x11	/* Update_1 */
#define SCDC_UPDATE_RESERVED           0x12	/* 0x12-0x1f - Reserved for Update Related Uses */
#define SCDC_TMDS_CONFIG               0x20	/* TMDS_Config */
#define SCDC_SCRAMBLER_STAT            0x21	/* Scrambler_Status */
#define SCDC_CONFIG_0                  0x30	/* Config_0 */
#define SCDC_CONFIG_RESERVED           0x31	/* 0x31-0x3f - Reserved for configuration */
#define SCDC_STATUS_FLAG_0             0x40	/* Status_Flag_0 */
#define SCDC_STATUS_FLAG_1             0x41	/* Status_Flag_1 */
#define SCDC_STATUS_RESERVED           0x42	/* 0x42-0x4f - Reserved for Status Related Uses */
#define SCDC_ERR_DET_0_L               0x50	/* Err_Det_0_L */
#define SCDC_ERR_DET_0_H               0x51	/* Err_Det_0_H */
#define SCDC_ERR_DET_1_L               0x52	/* Err_Det_1_L */
#define SCDC_ERR_DET_1_H               0x53	/* Err_Det_1_H */
#define SCDC_ERR_DET_2_L               0x54	/* Err_Det_2_L */
#define SCDC_ERR_DET_2_H               0x55	/* Err_Det_2_H */
#define SCDC_ERR_DET_CHKSUM            0x56	/* Err_Det_Checksum */
#define SCDC_TEST_CFG_0                0xc0	/* Test_config_0 */
#define SCDC_TEST_RESERVED             0xc1	/* 0xc1-0xcf - Reserved for test features */
#define SCDC_MAN_OUI_3RD               0xd0	/* Manufacturer IEEE OUI, Third Octet */
#define SCDC_MAN_OUI_2ND               0xd1	/* Manufacturer IEEE OUI, Second Octet */
#define SCDC_MAN_OUI_1ST               0xd2	/* Manufacturer IEEE OUI, First Octet */
#define SCDC_DEVICE_ID                 0xd3	/* 0xd3-0xdd - Device ID */
#define SCDC_MAN_SPECIFIC              0xde	/* 0xde-0xff - ManufacturerSpecific */

/**
 * @desc: scdc read function
 * @addr: scdc offset address
 * @size: scdc read size. (unit: byte)
 * @data: scdc read data point
 * @return: 0 - success
*/
int dw_scdc_read(dw_hdmi_dev_t *dev, u8 addr, u8 size, u8 *data);
/**
 * @desc: scdc write function
 * @addr: scdc offset address
 * @size: scdc write size. (unit: byte)
 * @data: scdc write data point
 * @return: 0 - success
*/
int dw_scdc_write(dw_hdmi_dev_t *dev, u8 addr, u8 size, u8 *data);
/**
 * @desc: set scdc tmds config - Scrambling_Enable
 * @enable: 1 - enable scrambling in the Sink
 *          0 - disable scrambling in the Sink
 * @return: 0 - successs
*/
int dw_scdc_set_scramble(dw_hdmi_dev_t *dev, u8 enable);
/**
 * @desc: set scdc tmds config - TMDS_Bit_Clock_Ratio
 * @enable: 1 - use ratio is 1/40
 *          0 - use ratio is 1/10
 * @return: 0 - successs
*/
int dw_scdc_set_tmds_clk_ratio(dw_hdmi_dev_t *dev, u8 enable);

#endif
#endif	/* _DW_SCDC_H_ */
