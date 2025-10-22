/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 rengaomin@allwinnertech.com
 */

#ifndef _CCU_SUN60IW2_RTC_H_
#define _CCU_SUN60IW2_RTC_H_

#include <dt-bindings/clock/sun60iw2-rtc.h>

#define LOSC_CTRL_REG			0x00
#define KEY_FIELD_MAGIC_NUM_RTC		0x16AA0000
#define LOSC_OUT_GATING_REG		0x60  /* Or: 32K_FOUT_CTRL_GATING_REG */
#define XO_CTRL_REG			0x160
#define DCXO_WAKEUP_KEY_FIELD		0x16AA
#define LOSC_AUTO_SWT_STA_REG		0x04

#define CLK_NUMBER                      (CLK_RTC_MAX_NO)

#endif /* _CCU_SUN60IW2_RTC_H_ */
