/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Private header file for sunxi-rtc driver
 *
 * Copyright (C) 2018 Allwinner.
 *
 * Martin <wuyan@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _RTC_SUNXI_H_
#define _RTC_SUNXI_H_

#include <linux/rtc.h>
#include <linux/clk.h>
#include <linux/reset.h>

#define FAKE_POWEROFF_RTC_FLAG			0xf
#define NORMAL_POWEROFF_RTC_FLAG		0xe

enum sunxi_rtc_type {
	SUNXI_RTC_TYPE_A,  /* TimeReg: DAY_HMS, AlarmReg: DAY_HMS */
	SUNXI_RTC_TYPE_B,  /* TimeReg: YMD_HMS, AlarmReg: SECONDS */
};

struct sunxi_rtc_data {
	enum sunxi_rtc_type type;

	/* Minimal real year allowed. eg: 1970
	 * For "type == SUNXI_RTC_TYPE_A" or "time_type == SUNXI_RTC_TYPE_B"
	 * this val can be set to any value that >=1970.
	 * See the requirements in the func rtc_valid_tm() in drivers/rtc/lib.c.
	 */
	u32 min_year;

	/* Maximum real year allowed, max_year = min_year + hw_max_year.
	 * For "type == SUNXI_RTC_TYPE_A", the val can be caculated by TYPE_A_REAL_YEAR_MAX(min_year),
	 * For "type == SUNXI_RTC_TYPE_B", the val can be caculated by TYPE_B_REAL_YEAR_MAX(min_year),
	 */
	u32 max_year;

	/* Only Valid when "time_type == SUNXI_RTC_TYPE_B" */
	u32 year_mask;	/* bit mask  of YEAR field */
	u32 year_shift;	/* bit shift of YEAR field */
	u32 leap_shift;	/* bit shift of LEAP-YEAR field */

	u32 gpr_offset; /* Offset to General-Purpose-Register */
	u32 gpr_len;    /* Number of General-Purpose-Register */
	bool has_dcxo_ictrl; /* Support modifying rtc dcxo current control value and creating dcxo_ictrl sysfs or not */
	u32 dcxo_ictrl_val; /* For rtc dcxo current control value when has_dcxo_ictrl = true */
	u32 gpr_base_address;	/* Gernel-Purpose-Register base address, gpr_base_address = 0 represent Genernal-Purpose-Register base address is same with rtc */
	u32 day_access_bit;	/* represents the bit position of RTC_DAY_ACCE in CTRL_REG */
	u32 hhmmss_access_bit;	/* represents the bit position of RTC_HHMMSS_ACCE in CTRL_REG */
};

struct sunxi_rtc_dev {
	struct rtc_device *rtc;
	struct device *dev;
	struct sunxi_rtc_data *data;
	struct clk *clk;
	struct clk *clk_bus;
	struct clk *clk_spi;
	struct reset_control *reset;
	void __iomem *base;
	void __iomem *gpr_base;
	int irq;
	bool gpr_only;
};

#endif /* end of _RTC_SUNXI_H_ */
