// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef _DPTC_DRV_H_
#define _DPTC_DRV_H_


#ifdef CONFIG_KY_FPGA
#define DPTC_DPHY_TEST 1
#endif

typedef enum
{
	STANDARD_MODE = 0,  /*100Kbps*/
	FAST_MODE = 1,      /*400Kbps*/
	HS_MODE = 2,        /*3.4 Mbps slave/3.3 Mbps master,standard mode when not doing a high speed transfer*/
	HS_MODE_FAST = 3,   /*3.4 Mbps slave/3.3 Mbps master,fast mode when not doing a high speed transfer*/
} I2C_FAST_MODE;

struct twsi_data {
	u8 twsi_no;
	u8 reg_len;/* byte num*/
	u8 val_len;/* byte num*/
	u8 addr; /* 7 bit i2c address*/
	u16 reg;
	u32 val;
};

enum i2c_len {
	I2C_8BIT = 1,
	I2C_16BIT = 2,
	I2C_24BIT = 3,
	I2C_32BIT = 4,
};

void dptc_dsi_write(uint32_t reg, uint32_t data);
uint32_t dptc_dsi_read(uint32_t reg);
void dptc_board_init(void);

#endif

