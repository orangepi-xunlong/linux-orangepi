/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's errcode
 *
 * Copyright (c) 2023, lvda <lvda@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef __SUNXI_ERR_H__
#define __SUNXI_ERR_H__

/*
 * SUNXI ERRCODE format:
 * |	bit31	|	bit30:bit20	|	bit19:bit16	|	bit15:bit0	|
 * |------------|-----------------------|-----------------------|-----------------------|
 * |	1	|	MOD + SUBMOD	|	BUG TYPE	|	USER-DEF	|
 */
#define E_MOD_OFFSET	20
#define E_TYPE_OFFSET	16
#define E_USER_OFFSET	0

/* Error code is negative */
#define E_MOD(mod)	(BIT(31) | ((mod & 0x7FF) << E_MOD_OFFSET))
#define E_TYPE(type)	((type & 0xF) << E_TYPE_OFFSET)
#define E_USER(user)	((user & 0xFFFF) << E_USER_OFFSET)

/*
 * SUNXI ERR type:
 * E_TYPE_HW:		IP error, include abnormal interruption
 * E_TYPE_SW_DEP:	The module that it depends on has issues
 * E_TYPE_SW_SYS:	The system interface has error
 * E_TYPE_SW_ARG:	The parameter passing error, including system call parameter, DTS parameters
 *
 */
enum sunxi_err_type {
	E_TYPE_HW = 0,
	E_TYPE_SW_DEP,
	E_TYPE_SW_SYS,
	E_TYPE_SW_ARG,

	E_TYPE_OTHER = 15,
};

/*
 * SUNXI module names
 */
enum sunxi_err_mod {
	/* basic-system */
	E_MOD_SYS	= 0x0,
	E_MOD_CLK	= 0x40,
	E_MOD_CLK_NG,
	E_MOD_PIN	= 0x50,
	E_MOD_INT	= 0x60,
	E_MOD_UART	= 0x70,
	E_MOD_UART_NG,
	E_MOD_TIMER	= 0x80,
	E_MOD_TIMER_HS,
	E_MOD_WDT	= 0x90,
	E_MOD_DMA	= 0xA0,
	E_MOD_RTC	= 0xB0,
	E_MOD_PMIC	= 0xC0,
	/* highspeed-peripheral-system */
	E_MOD_GMAC	= 0x100,
	E_MOD_PCIE	= 0x140,
	E_MOD_USB	= 0x180,
	E_MOD_MMC	= 0x1C0,
	/* lowspeed-peripheral-system */
	E_MOD_PWM	= 0x200,
	E_MOD_IR	= 0x210,
	E_MOD_TWI	= 0x220,
	E_MOD_ADC	= 0x230,
	E_MOD_LEDC	= 0x240,
	E_MOD_SPI	= 0x250,
	/* AMP system */
	E_MOD_HWLOCK	= 0x260,
	E_MOD_MSGBOX	= 0x270,
	/* secure system */
	E_MOD_CE	= 0x280,
	E_MOD_SID	= 0x2C0,
	/* graphic system */
	E_MOD_G2D	= 0x300,
	E_MOD_DE	= 0x340,
	E_MOD_GPU	= 0x380,
	/* multimedia system */
	E_MOD_VE	= 0x3C0,
	E_MOD_CSI	= 0x400,
	E_MOD_ISP	= 0x440,
	/* audio system */
	E_MOD_AUDIO	 = 0x480,
	E_MOD_AUDIOCODEC = 0x481,
	E_MOD_I2S	 = 0x482,
	E_MOD_DMIC	 = 0x483,
	E_MOD_OWA	 = 0x484,
	E_MOD_AHUB 	 = 0x485,

	E_MOD_END	= 0x800, /* Do not use */
};

enum sunxi_err_code {
	E_SYS_HW_ERR0		= E_MOD(E_MOD_SYS) | E_TYPE(E_TYPE_HW) | 0x0,

	E_CLK_HW_ERR0		= E_MOD(E_MOD_CLK) | E_TYPE(E_TYPE_HW) | 0x0,

	E_PIN_HW_ERR0		= E_MOD(E_MOD_PIN) | E_TYPE(E_TYPE_HW) | 0x0,

	E_INT_HW_ERR0		= E_MOD(E_MOD_INT) | E_TYPE(E_TYPE_HW) | 0x0,

	E_UART_HW_ERR0		= E_MOD(E_MOD_UART) | E_TYPE(E_TYPE_HW) | 0x0,

	E_TIMER_HW_ERR0		= E_MOD(E_MOD_TIMER) | E_TYPE(E_TYPE_HW) | 0x0,

	E_WDT_HW_ERR0		= E_MOD(E_MOD_WDT) | E_TYPE(E_TYPE_HW) | 0x0,

	E_DMA_HW_ERR0		= E_MOD(E_MOD_DMA) | E_TYPE(E_TYPE_HW) | 0x0,

	E_RTC_HW_ERR0		= E_MOD(E_MOD_RTC) | E_TYPE(E_TYPE_HW) | 0x0,

	E_PMIC_HW_ERR0		= E_MOD(E_MOD_PMIC) | E_TYPE(E_TYPE_HW) | 0x0,

	E_GMAC_HW_ERR0		= E_MOD(E_MOD_GMAC) | E_TYPE(E_TYPE_HW) | 0x0,

	E_PCIE_HW_ERR0		= E_MOD(E_MOD_PCIE) | E_TYPE(E_TYPE_HW) | 0x0,

	E_USB_HW_ERR0		= E_MOD(E_MOD_USB) | E_TYPE(E_TYPE_HW) | 0x0,

	E_MMC_HW_ERR0		= E_MOD(E_MOD_MMC) | E_TYPE(E_TYPE_HW) | 0x0,

	E_PWM_HW_ERR0		= E_MOD(E_MOD_PWM) | E_TYPE(E_TYPE_HW) | 0x0,

	E_IR_HW_ERR0		= E_MOD(E_MOD_IR) | E_TYPE(E_TYPE_HW) | 0x0,

	E_TWI_HW_ERR0		= E_MOD(E_MOD_TWI) | E_TYPE(E_TYPE_HW) | 0x0,

	E_ADC_HW_ERR0		= E_MOD(E_MOD_ADC) | E_TYPE(E_TYPE_HW) | 0x0,

	E_LEDC_HW_ERR0		= E_MOD(E_MOD_LEDC) | E_TYPE(E_TYPE_HW) | 0x0,

	E_SPI_HW_ERR0		= E_MOD(E_MOD_SPI) | E_TYPE(E_TYPE_HW) | 0x0,

	E_HWLOCK_HW_ERR0	= E_MOD(E_MOD_HWLOCK) | E_TYPE(E_TYPE_HW) | 0x0,

	E_MSGBOX_HW_ERR0	= E_MOD(E_MOD_MSGBOX) | E_TYPE(E_TYPE_HW) | 0x0,

	E_CE_HW_ERR0		= E_MOD(E_MOD_CE) | E_TYPE(E_TYPE_HW) | 0x0,

	E_SID_HW_ERR0		= E_MOD(E_MOD_SID) | E_TYPE(E_TYPE_HW) | 0x0,

	E_G2D_HW_ERR0		= E_MOD(E_MOD_G2D) | E_TYPE(E_TYPE_HW) | 0x0,

	E_DE_HW_ERR0		= E_MOD(E_MOD_DE) | E_TYPE(E_TYPE_HW) | 0x0,

	E_GPU_HW_ERR0		= E_MOD(E_MOD_GPU) | E_TYPE(E_TYPE_HW) | 0x0,

	E_VE_HW_ERR0		= E_MOD(E_MOD_VE) | E_TYPE(E_TYPE_HW) | 0x0,

	E_CSI_HW_ERR0		= E_MOD(E_MOD_CSI) | E_TYPE(E_TYPE_HW) | 0x0,

	E_ISP_HW_ERR0		= E_MOD(E_MOD_ISP) | E_TYPE(E_TYPE_HW) | 0x0,

	E_AUDIO_HW_ERR0			= E_MOD(E_MOD_AUDIO) | E_TYPE(E_TYPE_HW)     | 0x0,
	E_AUDIO_SW_DEP_ERR0		= E_MOD(E_MOD_AUDIO) | E_TYPE(E_TYPE_SW_DEP) | 0x0,
	E_AUDIO_SW_SYS_ERR0		= E_MOD(E_MOD_AUDIO) | E_TYPE(E_TYPE_SW_SYS) | 0x0,
	E_AUDIO_SW_ARG_ERR0		= E_MOD(E_MOD_AUDIO) | E_TYPE(E_TYPE_SW_ARG) | 0x0,
	E_AUDIOCODEC_HW_ERR0		= E_MOD(E_MOD_AUDIOCODEC) | E_TYPE(E_TYPE_HW)     | 0x0,
	E_AUDIOCODEC_SW_DEP_ERR0	= E_MOD(E_MOD_AUDIOCODEC) | E_TYPE(E_TYPE_SW_DEP) | 0x0,
	E_AUDIOCODEC_SW_SYS_ERR0	= E_MOD(E_MOD_AUDIOCODEC) | E_TYPE(E_TYPE_SW_SYS) | 0x0,
	E_AUDIOCODEC_SW_ARG_ERR0	= E_MOD(E_MOD_AUDIOCODEC) | E_TYPE(E_TYPE_SW_ARG) | 0x0,
	E_I2S_HW_ERR0			= E_MOD(E_MOD_I2S)   | E_TYPE(E_TYPE_HW)     | 0x0,
	E_I2S_SW_DEP_ERR0		= E_MOD(E_MOD_I2S)   | E_TYPE(E_TYPE_SW_DEP) | 0x0,
	E_I2S_SW_SYS_ERR0		= E_MOD(E_MOD_I2S)   | E_TYPE(E_TYPE_SW_SYS) | 0x0,
	E_I2S_SW_ARG_ERR0		= E_MOD(E_MOD_I2S)   | E_TYPE(E_TYPE_SW_ARG) | 0x0,
	E_DMIC_HW_ERR0			= E_MOD(E_MOD_DMIC)  | E_TYPE(E_TYPE_HW)     | 0x0,
	E_DMIC_SW_DEP_ERR0		= E_MOD(E_MOD_DMIC)  | E_TYPE(E_TYPE_SW_DEP) | 0x0,
	E_DMIC_SW_SYS_ERR0		= E_MOD(E_MOD_DMIC)  | E_TYPE(E_TYPE_SW_SYS) | 0x0,
	E_DMIC_SW_ARG_ERR0		= E_MOD(E_MOD_DMIC)  | E_TYPE(E_TYPE_SW_ARG) | 0x0,
	E_OWA_HW_ERR0			= E_MOD(E_MOD_OWA)   | E_TYPE(E_TYPE_HW)     | 0x0,
	E_OWA_SW_DEP_ERR0		= E_MOD(E_MOD_OWA)   | E_TYPE(E_TYPE_SW_DEP) | 0x0,
	E_OWA_SW_SYS_ERR0		= E_MOD(E_MOD_OWA)   | E_TYPE(E_TYPE_SW_SYS) | 0x0,
	E_OWA_SW_ARG_ERR0		= E_MOD(E_MOD_OWA)   | E_TYPE(E_TYPE_SW_ARG) | 0x0,
	E_AHUB_HW_ERR0			= E_MOD(E_MOD_AHUB)  | E_TYPE(E_TYPE_HW)     | 0x0,
	E_AHUB_SW_DEP_ERR0		= E_MOD(E_MOD_AHUB)  | E_TYPE(E_TYPE_SW_DEP) | 0x0,
	E_AHUB_SW_SYS_ERR0		= E_MOD(E_MOD_AHUB)  | E_TYPE(E_TYPE_SW_SYS) | 0x0,
	E_AHUB_SW_ARG_ERR0		= E_MOD(E_MOD_AHUB)  | E_TYPE(E_TYPE_SW_ARG) | 0x0,
};

#include "sunxi-err-audio.h"

#endif
