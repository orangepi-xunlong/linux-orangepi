/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 lvda@allwinnertech.com
 */

#ifndef _CCU_SUN50IW9_RTC_H_
#define _CCU_SUN50IW9_RTC_H_

#include <dt-bindings/clock/sun50iw9-ccu-rtc.h>

#define LOSC_CTRL_REG			0x00
#define KEY_FIELD_MAGIC			0x16AA
#define INTOSC_CLK_AUTO_CALI_REG	0x0C
#define LOSC_OUT_GATING_REG		0x60  /* Or: 32K_FOUT_CTRL_GATING_REG */
#define XO_CTRL_REG			0x160  /* XO Control register */
#define CALI_CTRL_REG			0x164

#define CLK_NUMBER			(CLK_OSC32K_OUT + 1)

#endif /* _CCU_SUN50IW9_RTC_H_ */

