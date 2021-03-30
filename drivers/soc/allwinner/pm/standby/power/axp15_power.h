/*
 * axp192 for standby driver
 *
 * Copyright (C) 2015 allwinnertech Ltd.
 * Author: Ming Li <liming@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __AXP15_POWER_H__
#define __AXP15_POWER_H__

#include "axp_power.h"

/* Unified sub device IDs for AXP */
enum {
	AXP15_ID_LDO0,
	AXP15_ID_LDO1,
	AXP15_ID_LDO2,
	AXP15_ID_LDO3,
	AXP15_ID_LDO4,
	AXP15_ID_LDO5,
	AXP15_ID_LDOIO0,
	AXP15_ID_DCDC1,
	AXP15_ID_DCDC2,
	AXP15_ID_DCDC3,
	AXP15_ID_DCDC4,
	AXP15_ID_MAX,
};

#define AXP15_ADDR                  (0x30)

#define AXP15_STATUS                (0x00)
#define AXP15_LDO0OUT_VOL           (0x15)
#define AXP15_LDO34OUT_VOL          (0x28)
#define AXP15_LDO5OUT_VOL           (0x29)
#define AXP15_LDO6OUT_VOL           (0x2A)
#define AXP15_GPIO0_VOL             (0x96)
#define AXP15_DC1OUT_VOL            (0x26)
#define AXP15_DC2OUT_VOL            (0x23)
#define AXP15_DC3OUT_VOL            (0x27)
#define AXP15_DC4OUT_VOL            (0x2B)
#define AXP15_LDO0_CTL              (0x15)
#define AXP15_LDO3456_DC1234_CTL    (0x12)
#define AXP15_GPIO2_CTL             (0x92)

/* AXP15 Regulator Registers */
#define AXP15_LDO0         AXP15_LDO0OUT_VOL
#define AXP15_RTC          AXP15_STATUS
#define AXP15_ALDO1        AXP15_LDO34OUT_VOL
#define AXP15_ALDO2        AXP15_LDO34OUT_VOL
#define AXP15_DLDO1        AXP15_LDO5OUT_VOL
#define AXP15_DLDO2        AXP15_LDO6OUT_VOL
#define AXP15_LDOIO0       AXP15_GPIO0_VOL

#define AXP15_DCDC1        AXP15_DC1OUT_VOL
#define AXP15_DCDC2        AXP15_DC2OUT_VOL
#define AXP15_DCDC3        AXP15_DC3OUT_VOL
#define AXP15_DCDC4        AXP15_DC4OUT_VOL

#define AXP15_LDO0EN       AXP15_LDO0_CTL
#define AXP15_RTCLDOEN     AXP15_STATUS
#define AXP15_ALDO1EN      AXP15_LDO3456_DC1234_CTL
#define AXP15_ALDO2EN      AXP15_LDO3456_DC1234_CTL
#define AXP15_DLDO1EN      AXP15_LDO3456_DC1234_CTL
#define AXP15_DLDO2EN      AXP15_LDO3456_DC1234_CTL
#define AXP15_LDOI0EN      AXP15_GPIO2_CTL

#define AXP15_DCDC1EN      AXP15_LDO3456_DC1234_CTL
#define AXP15_DCDC2EN      AXP15_LDO3456_DC1234_CTL
#define AXP15_DCDC3EN      AXP15_LDO3456_DC1234_CTL
#define AXP15_DCDC4EN      AXP15_LDO3456_DC1234_CTL

#endif				/* __AXP15_POWER_H__ */
