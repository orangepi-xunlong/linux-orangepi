/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2021-2031 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __SUNXI_WIEGAND_H
#define __SUNXI_WIEGAND_H

#define WIEGAND_VER		"v1.0.0"

#define WIEGAND_WMR		(0x04)
#define WIEGAND_TP		(0x08)
#define WIEGAND_TW		(0x0c)
#define WIEGAND_CLK_DIV		(0x10)
#define WIEGAND_FUN_EN		(0x14)
#define WIEGAND_ICR		(0x1c)
#define WIEGAND_ISR		(0x20)
#define WIEGAND_CBSR		(0x24)
#define WIEGAND_CBP		(0x28)
#define WIEGAND_DBP		(0x2c)
#define WIEGAND_DSR		(0x34)
#define WIEGAND_TDNR		(0x38)
#define WIEGAND_TDR		(0x3c)
#define WIEGAND_RXD		(0x100)
#define WIEGAND_RXC		(0x200)

#define WIEGAND_WMR_VALUE	(0x00)
#define WIEGAND_CLK_DIV_VALUE	(0x18)
#define WIEGAND_TP_VALUE	(0x01)
#define WIEGAND_TW_VALUE	(0x01)
#define WIEGAND_ICR_VALUE	(0x03)
#define WIEGAND_CBP_VALUE	(0x03)
#define WIEGAND_DBP_VALUE	(0x00)
#define WIEGAND_FUN_EN_VALUE	(0x01)

#define WIEGAND_ISR_RIS		BIT(0)
#define WIEGAND_ISR_TRIS	BIT(1)

#define WIEGAND_CLK	24000000
#define TIME_DELAY	20

#define WIEGAND_ISR_RECEIVE_DATA	(0x01)
#define WIEGAND_ISR_TIME_OUT		(0x02)

#define WIEGAND_CBSR_CHECK_OK		(0x00)
#define WIEGAND_CBSR_EVEN_FAILED	(0x01)
#define WIEGAND_CBSR_ODD_FAILED		(0x02)
#define WIEGAND_CBSR_ODD_EVEN_FAILED	(0x03)

#define SUNXI_WIEGAND_DRIVER_NAME	"wiegand_driver"

struct sunxi_wiegand_config {
	u32 protocol_type;		/* select protocol-type; 0:26bit, 1:34bit */
	u32 data_polar;			/* select data bit polarity 0:normal; 1:inverse */
	u32 high_parity_polar;		/* select high parity bit polarity 1:odd parity; 0:even parity */
	u32 low_parity_polar;		/* select low parity bit polarity 1:odd parity; 0:even parity */
	u32 signal_duration;		/* duty cycle, range is determined by clock_div */
	u32 signal_period;		/* cycle length, range is determined by clock_div */
	u32 clock_div;			/* set the clock division ratio, range: 0 - 48 */
};

#endif /* End of __SUNXI_WIEGAND_H */
