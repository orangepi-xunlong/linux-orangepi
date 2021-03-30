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

#ifndef __AXP22_POWER_H__
#define __AXP22_POWER_H__

#include "axp_power.h"

#define AXP22_DCDC_SUM  (5)

/* Unified sub device IDs for AXP */
enum {
	AXP22_ID_DCDC1 = 0,
	AXP22_ID_DCDC2,
	AXP22_ID_DCDC3,
	AXP22_ID_DCDC4,
	AXP22_ID_DCDC5,
	AXP22_ID_ALDO1,
	AXP22_ID_ALDO2,
	AXP22_ID_ALDO3,
	AXP22_ID_DLDO1,
	AXP22_ID_DLDO2,
	AXP22_ID_DLDO3,
	AXP22_ID_DLDO4,
	AXP22_ID_ELDO1,
	AXP22_ID_ELDO2,
	AXP22_ID_ELDO3,
	AXP22_ID_DC5LDO,
	AXP22_ID_LDOIO0,
	AXP22_ID_LDOIO1,
	AXP22_ID_DC1SW,
	AXP22_ID_RTC,
	AXP22_ID_MAX,
};

#define AXP22_ADDR                 (0x34)

/* AXP22 Regulator Enable Registers */
#define AXP22_STATUS              (0x00)
#define AXP22_LDO_DC_EN1          (0X10)
#define AXP22_LDO_DC_EN2          (0X12)
#define AXP22_LDO_DC_EN3          (0X13)
#define AXP22_GPIO0_CTL           (0x90)
#define AXP22_GPIO1_CTL           (0x92)

#define AXP22_DCDC1EN             AXP22_LDO_DC_EN1
#define AXP22_DCDC2EN             AXP22_LDO_DC_EN1
#define AXP22_DCDC3EN             AXP22_LDO_DC_EN1
#define AXP22_DCDC4EN             AXP22_LDO_DC_EN1
#define AXP22_DCDC5EN             AXP22_LDO_DC_EN1
#define AXP22_LDO1EN              AXP22_STATUS
#define AXP22_LDO2EN              AXP22_LDO_DC_EN1
#define AXP22_LDO3EN              AXP22_LDO_DC_EN1
#define AXP22_LDO4EN              AXP22_LDO_DC_EN3
#define AXP22_LDO5EN              AXP22_LDO_DC_EN2
#define AXP22_LDO6EN              AXP22_LDO_DC_EN2
#define AXP22_LDO7EN              AXP22_LDO_DC_EN2
#define AXP22_LDO8EN              AXP22_LDO_DC_EN2
#define AXP22_LDO9EN              AXP22_LDO_DC_EN2
#define AXP22_LDO10EN             AXP22_LDO_DC_EN2
#define AXP22_LDO11EN             AXP22_LDO_DC_EN2
#define AXP22_LDO12EN             AXP22_LDO_DC_EN1
#define AXP22_LDOIO0EN            AXP22_GPIO0_CTL
#define AXP22_LDOIO1EN            AXP22_GPIO1_CTL
#define AXP22_DC1SWEN             AXP22_LDO_DC_EN2

/* AXP22 Regulator Voltage Registers */
#define AXP22_DC1OUT_VOL          (0x21)
#define AXP22_DC2OUT_VOL          (0x22)
#define AXP22_DC3OUT_VOL          (0x23)
#define AXP22_DC4OUT_VOL          (0x24)
#define AXP22_DC5OUT_VOL          (0x25)
#define AXP22_ALDO1OUT_VOL        (0x28)
#define AXP22_ALDO2OUT_VOL        (0x29)
#define AXP22_ALDO3OUT_VOL        (0x2A)
#define AXP22_DLDO1OUT_VOL        (0x15)
#define AXP22_DLDO2OUT_VOL        (0x16)
#define AXP22_DLDO3OUT_VOL        (0x17)
#define AXP22_DLDO4OUT_VOL        (0x18)
#define AXP22_ELDO1OUT_VOL        (0x19)
#define AXP22_ELDO2OUT_VOL        (0x1A)
#define AXP22_ELDO3OUT_VOL        (0x1B)
#define AXP22_DC5LDOOUT_VOL       (0x1C)
#define AXP22_GPIO0LDOOUT_VOL     (0x91)
#define AXP22_GPIO1LDOOUT_VOL     (0x93)

#define AXP22_DCDC1               AXP22_DC1OUT_VOL
#define AXP22_DCDC2               AXP22_DC2OUT_VOL
#define AXP22_DCDC3               AXP22_DC3OUT_VOL
#define AXP22_DCDC4               AXP22_DC4OUT_VOL
#define AXP22_DCDC5               AXP22_DC5OUT_VOL
#define AXP22_LDO2                AXP22_ALDO1OUT_VOL
#define AXP22_LDO3                AXP22_ALDO2OUT_VOL
#define AXP22_LDO4                AXP22_ALDO3OUT_VOL
#define AXP22_LDO5                AXP22_DLDO1OUT_VOL
#define AXP22_LDO6                AXP22_DLDO2OUT_VOL
#define AXP22_LDO7                AXP22_DLDO3OUT_VOL
#define AXP22_LDO8                AXP22_DLDO4OUT_VOL
#define AXP22_LDO9                AXP22_ELDO1OUT_VOL
#define AXP22_LDO10               AXP22_ELDO2OUT_VOL
#define AXP22_LDO11               AXP22_ELDO3OUT_VOL
#define AXP22_LDO12               AXP22_DC5LDOOUT_VOL
#define AXP22_LDOIO0              AXP22_GPIO0LDOOUT_VOL
#define AXP22_LDOIO1              AXP22_GPIO1LDOOUT_VOL
#define AXP22_DC1SW               AXP22_STATUS
#define AXP22_LDO1                AXP22_STATUS

#endif /* __AXP22_POWER_H__ */
