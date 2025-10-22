/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 panzhijian@allwinnertech.com
 */

#ifndef _CCU_SUN55IW6_RTC_H_
#define _CCU_SUN55IW6_RTC_H_

#include <dt-bindings/clock/sun55iw6-rtc.h>

#define LOSC_CTRL_REG			0x00
#define KEY_FIELD_MAGIC_NUM_RTC		0x16AA0000
#define INTOSC_CLK_AUTO_CALI_REG	0x0C
#define LOSC_OUT_GATING_REG		0x60  /* Or: 32K_FOUT_CTRL_GATING_REG */
#define XO_CTRL_REG			0x160
#define CALI_CTRL_REG                   0x164

#define CLK_NUMBER			(CLK_RTC_MAX_NO + 1)

#endif /* _CCU_SUN55IW6_RTC_H_ */
