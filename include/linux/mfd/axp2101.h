/*
 * Functions and registers to access AXP20X power management chip.
 *
 * Copyright (C) 2013, Carlo Caione <carlo@caione.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LINUX_MFD_AXP20X_H
#define __LINUX_MFD_AXP20X_H

#include <linux/regmap.h>

enum {
	AXP152_ID = 0,
	AXP202_ID,
	AXP209_ID,
	AXP221_ID,
	AXP223_ID,
	AXP288_ID,
	AXP806_ID,
	AXP809_ID,
	AXP2101_ID,
	AXP15_ID,
	AXP1530_ID,
	AXP858_ID,
	AXP803_ID,
	AXP2202_ID,
	AXP2585_ID,
	NR_AXP20X_VARIANTS,
};

enum {
	AXP_SPLY = 1U << 0,
	AXP_REGU = 1U << 1,
	AXP_INT  = 1U << 2,
	AXP_CHG  = 1U << 3,
	AXP_MISC = 1U << 4,
};

extern int axp_debug_mask;

#define AXP20X_DATACACHE(m)		(0x04 + (m))

/* Power supply */
#define AXP152_PWR_OP_MODE		0x01
#define AXP152_LDO3456_DC1234_CTRL	0x12
#define AXP152_ALDO_OP_MODE		0x13
#define AXP152_LDO0_CTRL		0x15
#define AXP152_DCDC2_V_OUT		0x23
#define AXP152_DCDC2_V_SCAL		0x25
#define AXP152_DCDC1_V_OUT		0x26
#define AXP152_DCDC3_V_OUT		0x27
#define AXP152_ALDO12_V_OUT		0x28
#define AXP152_DLDO1_V_OUT		0x29
#define AXP152_DLDO2_V_OUT		0x2a
#define AXP152_DCDC4_V_OUT		0x2b
#define AXP152_V_OFF			0x31
#define AXP152_OFF_CTRL			0x32
#define AXP152_PEK_KEY			0x36
#define AXP152_DCDC_FREQ		0x37
#define AXP152_DCDC_MODE		0x80

#define AXP20X_PWR_INPUT_STATUS		0x00
#define AXP20X_PWR_OP_MODE		0x01
#define AXP20X_USB_OTG_STATUS		0x02
#define AXP20X_PWR_OUT_CTRL		0x12
#define AXP20X_DCDC2_V_OUT		0x23
#define AXP20X_DCDC2_LDO3_V_SCAL	0x25
#define AXP20X_DCDC3_V_OUT		0x27
#define AXP20X_LDO24_V_OUT		0x28
#define AXP20X_LDO3_V_OUT		0x29
#define AXP20X_VBUS_IPSOUT_MGMT		0x30
#define AXP20X_V_OFF			0x31
#define AXP20X_OFF_CTRL			0x32
#define AXP20X_CHRG_CTRL1		0x33
#define AXP20X_CHRG_CTRL2		0x34
#define AXP20X_CHRG_BAK_CTRL		0x35
#define AXP20X_PEK_KEY			0x36
#define AXP20X_DCDC_FREQ		0x37
#define AXP20X_V_LTF_CHRG		0x38
#define AXP20X_V_HTF_CHRG		0x39
#define AXP20X_APS_WARN_L1		0x3a
#define AXP20X_APS_WARN_L2		0x3b
#define AXP20X_V_LTF_DISCHRG		0x3c
#define AXP20X_V_HTF_DISCHRG		0x3d

#define AXP22X_PWR_OUT_CTRL1		0x10
#define AXP22X_PWR_OUT_CTRL2		0x12
#define AXP22X_PWR_OUT_CTRL3		0x13
#define AXP22X_DLDO1_V_OUT		0x15
#define AXP22X_DLDO2_V_OUT		0x16
#define AXP22X_DLDO3_V_OUT		0x17
#define AXP22X_DLDO4_V_OUT		0x18
#define AXP22X_ELDO1_V_OUT		0x19
#define AXP22X_ELDO2_V_OUT		0x1a
#define AXP22X_ELDO3_V_OUT		0x1b
#define AXP22X_DC5LDO_V_OUT		0x1c
#define AXP22X_DCDC1_V_OUT		0x21
#define AXP22X_DCDC2_V_OUT		0x22
#define AXP22X_DCDC3_V_OUT		0x23
#define AXP22X_DCDC4_V_OUT		0x24
#define AXP22X_DCDC5_V_OUT		0x25
#define AXP22X_DCDC23_V_RAMP_CTRL	0x27
#define AXP22X_ALDO1_V_OUT		0x28
#define AXP22X_ALDO2_V_OUT		0x29
#define AXP22X_ALDO3_V_OUT		0x2a
#define AXP22X_CHRG_CTRL3		0x35

#define AXP806_STARTUP_SRC		0x00
#define AXP806_CHIP_ID			0x03
#define AXP806_PWR_OUT_CTRL1		0x10
#define AXP806_PWR_OUT_CTRL2		0x11
#define AXP806_DCDCA_V_CTRL		0x12
#define AXP806_DCDCB_V_CTRL		0x13
#define AXP806_DCDCC_V_CTRL		0x14
#define AXP806_DCDCD_V_CTRL		0x15
#define AXP806_DCDCE_V_CTRL		0x16
#define AXP806_ALDO1_V_CTRL		0x17
#define AXP806_ALDO2_V_CTRL		0x18
#define AXP806_ALDO3_V_CTRL		0x19
#define AXP806_DCDC_MODE_CTRL1		0x1a
#define AXP806_DCDC_MODE_CTRL2		0x1b
#define AXP806_DCDC_FREQ_CTRL		0x1c
#define AXP806_BLDO1_V_CTRL		0x20
#define AXP806_BLDO2_V_CTRL		0x21
#define AXP806_BLDO3_V_CTRL		0x22
#define AXP806_BLDO4_V_CTRL		0x23
#define AXP806_CLDO1_V_CTRL		0x24
#define AXP806_CLDO2_V_CTRL		0x25
#define AXP806_CLDO3_V_CTRL		0x26
#define AXP806_VREF_TEMP_WARN_L		0xf3
#define AXP806_REG_ADDR_EXT		0xFF

#define AXP806_STARTUP_SOURCE      (0x00)
#define AXP806_IC_TYPE             (0x03)
#define AXP806_DATA_BUFFER1        (0x04)
#define AXP806_DATA_BUFFER2        (0x05)
#define AXP806_DATA_BUFFER3        (0x06)
#define AXP806_DATA_BUFFER4        (0x07)
#define AXP806_ONOFF_CTRL1         (0x10)
#define AXP806_ONOFF_CTRL2         (0x11)
#define AXP806_DCAOUT_VOL          (0x12)
#define AXP806_DCBOUT_VOL          (0x13)
#define AXP806_DCCOUT_VOL          (0x14)
#define AXP806_DCDOUT_VOL          (0x15)
#define AXP806_DCEOUT_VOL          (0x16)
#define AXP806_ALDO1OUT_VOL        (0x17)
#define AXP806_ALDO2OUT_VOL        (0x18)
#define AXP806_ALDO3OUT_VOL        (0x19)
#define AXP806_DCDC_DVM_CTRL       (0x1A)
#define AXP806_DCDC_MODE_CTRL      (0x1B)
#define AXP806_DCDC_FREQSET        (0x1C)
#define AXP806_DCDC_MON_CTRL       (0x1D)
#define AXP806_IFQ_PWROK_SET       (0x1F)
#define AXP806_BLDO1OUT_VOL        (0x20)
#define AXP806_BLDO2OUT_VOL        (0x21)
#define AXP806_BLDO3OUT_VOL        (0x22)
#define AXP806_BLDO4OUT_VOL        (0x23)
#define AXP806_CLDO1OUT_VOL        (0x24)
#define AXP806_CLDO2OUT_VOL        (0x25)
#define AXP806_CLDO3OUT_VOL        (0x26)
#define AXP806_VOFF_SET            (0x31)
#define AXP806_OFF_CTL             (0x32)
#define AXP806_WAKEUP_PIN_CTRL     (0x35)
#define AXP806_POK_SET             (0x36)
#define AXP806_INTERFACE_MODE      (0x3E)
#define AXP806_SPECIAL_CTRL        (0x3F)

#define AXP806_INTEN1              (0x40)
#define AXP806_INTEN2              (0x41)
#define AXP806_INTSTS1             (0x48)
#define AXP806_INTSTS2             (0x49)
#define AXP806_REG_ADDR_EXT		0xFF

/* Interrupt */
#define AXP152_IRQ1_EN			0x40
#define AXP152_IRQ2_EN			0x41
#define AXP152_IRQ3_EN			0x42
#define AXP152_IRQ1_STATE		0x48
#define AXP152_IRQ2_STATE		0x49
#define AXP152_IRQ3_STATE		0x4a

#define AXP20X_IRQ1_EN			0x40
#define AXP20X_IRQ2_EN			0x41
#define AXP20X_IRQ3_EN			0x42
#define AXP20X_IRQ4_EN			0x43
#define AXP20X_IRQ5_EN			0x44
#define AXP20X_IRQ6_EN			0x45
#define AXP20X_IRQ1_STATE		0x48
#define AXP20X_IRQ2_STATE		0x49
#define AXP20X_IRQ3_STATE		0x4a
#define AXP20X_IRQ4_STATE		0x4b
#define AXP20X_IRQ5_STATE		0x4c
#define AXP20X_IRQ6_STATE		0x4d

/* ADC */
#define AXP20X_ACIN_V_ADC_H		0x56
#define AXP20X_ACIN_V_ADC_L		0x57
#define AXP20X_ACIN_I_ADC_H		0x58
#define AXP20X_ACIN_I_ADC_L		0x59
#define AXP20X_VBUS_V_ADC_H		0x5a
#define AXP20X_VBUS_V_ADC_L		0x5b
#define AXP20X_VBUS_I_ADC_H		0x5c
#define AXP20X_VBUS_I_ADC_L		0x5d
#define AXP20X_TEMP_ADC_H		0x5e
#define AXP20X_TEMP_ADC_L		0x5f
#define AXP20X_TS_IN_H			0x62
#define AXP20X_TS_IN_L			0x63
#define AXP20X_GPIO0_V_ADC_H		0x64
#define AXP20X_GPIO0_V_ADC_L		0x65
#define AXP20X_GPIO1_V_ADC_H		0x66
#define AXP20X_GPIO1_V_ADC_L		0x67
#define AXP20X_PWR_BATT_H		0x70
#define AXP20X_PWR_BATT_M		0x71
#define AXP20X_PWR_BATT_L		0x72
#define AXP20X_BATT_V_H			0x78
#define AXP20X_BATT_V_L			0x79
#define AXP20X_BATT_CHRG_I_H		0x7a
#define AXP20X_BATT_CHRG_I_L		0x7b
#define AXP20X_BATT_DISCHRG_I_H		0x7c
#define AXP20X_BATT_DISCHRG_I_L		0x7d
#define AXP20X_IPSOUT_V_HIGH_H		0x7e
#define AXP20X_IPSOUT_V_HIGH_L		0x7f

/* Power supply */
#define AXP20X_DCDC_MODE		0x80
#define AXP20X_ADC_EN1			0x82
#define AXP20X_ADC_EN2			0x83
#define AXP20X_ADC_RATE			0x84
#define AXP20X_GPIO10_IN_RANGE		0x85
#define AXP20X_GPIO1_ADC_IRQ_RIS	0x86
#define AXP20X_GPIO1_ADC_IRQ_FAL	0x87
#define AXP20X_TIMER_CTRL		0x8a
#define AXP20X_VBUS_MON			0x8b
#define AXP20X_OVER_TMP			0x8f

#define AXP22X_PWREN_CTRL1		0x8c
#define AXP22X_PWREN_CTRL2		0x8d

/* GPIO */
#define AXP152_GPIO0_CTRL		0x90
#define AXP152_GPIO1_CTRL		0x91
#define AXP152_GPIO2_CTRL		0x92
#define AXP152_GPIO3_CTRL		0x93
#define AXP152_LDOGPIO2_V_OUT		0x96
#define AXP152_GPIO_INPUT		0x97
#define AXP152_PWM0_FREQ_X		0x98
#define AXP152_PWM0_FREQ_Y		0x99
#define AXP152_PWM0_DUTY_CYCLE		0x9a
#define AXP152_PWM1_FREQ_X		0x9b
#define AXP152_PWM1_FREQ_Y		0x9c
#define AXP152_PWM1_DUTY_CYCLE		0x9d

#define AXP20X_GPIO0_CTRL		0x90
#define AXP20X_LDO5_V_OUT		0x91
#define AXP20X_GPIO1_CTRL		0x92
#define AXP20X_GPIO2_CTRL		0x93
#define AXP20X_GPIO20_SS		0x94
#define AXP20X_GPIO3_CTRL		0x95

#define AXP22X_LDO_IO0_V_OUT		0x91
#define AXP22X_LDO_IO1_V_OUT		0x93
#define AXP22X_GPIO_STATE		0x94
#define AXP22X_GPIO_PULL_DOWN		0x95

/* Battery */
#define AXP20X_CHRG_CC_31_24		0xb0
#define AXP20X_CHRG_CC_23_16		0xb1
#define AXP20X_CHRG_CC_15_8		0xb2
#define AXP20X_CHRG_CC_7_0		0xb3
#define AXP20X_DISCHRG_CC_31_24		0xb4
#define AXP20X_DISCHRG_CC_23_16		0xb5
#define AXP20X_DISCHRG_CC_15_8		0xb6
#define AXP20X_DISCHRG_CC_7_0		0xb7
#define AXP20X_CC_CTRL			0xb8
#define AXP20X_FG_RES			0xb9

/* OCV */
#define AXP20X_RDC_H			0xba
#define AXP20X_RDC_L			0xbb
#define AXP20X_OCV(m)			(0xc0 + (m))
#define AXP20X_OCV_MAX			0xf

/* Hot shutdoen */

#define AXP20X_OVER_TMP_VAL		0xf3

/* AXP22X specific registers */
#define AXP22X_BATLOW_THRES1		0xe6

/* AXP288 specific registers */
#define AXP288_PMIC_ADC_H               0x56
#define AXP288_PMIC_ADC_L               0x57
#define AXP288_ADC_TS_PIN_CTRL          0x84
#define AXP288_PMIC_ADC_EN              0x84

/* Fuel Gauge */
#define AXP288_FG_RDC1_REG          0xba
#define AXP288_FG_RDC0_REG          0xbb
#define AXP288_FG_OCVH_REG          0xbc
#define AXP288_FG_OCVL_REG          0xbd
#define AXP288_FG_OCV_CURVE_REG     0xc0
#define AXP288_FG_DES_CAP1_REG      0xe0
#define AXP288_FG_DES_CAP0_REG      0xe1
#define AXP288_FG_CC_MTR1_REG       0xe2
#define AXP288_FG_CC_MTR0_REG       0xe3
#define AXP288_FG_OCV_CAP_REG       0xe4
#define AXP288_FG_CC_CAP_REG        0xe5
#define AXP288_FG_LOW_CAP_REG       0xe6
#define AXP288_FG_TUNE0             0xe8
#define AXP288_FG_TUNE1             0xe9
#define AXP288_FG_TUNE2             0xea
#define AXP288_FG_TUNE3             0xeb
#define AXP288_FG_TUNE4             0xec
#define AXP288_FG_TUNE5             0xed

#define AXP2101_COMM_STAT0      (0x00)
#define AXP2101_COMM_STAT1      (0x01)
#define AXP2101_CHIP_ID         (0x03)
#define AXP2101_DATA_BUFFER0    (0x04)
#define AXP2101_DATA_BUFFER1    (0x05)
#define AXP2101_DATA_BUFFER2    (0x06)
#define AXP2101_DATA_BUFFER3    (0x07)
#define AXP2101_COMM_FAULT      (0x08)
#define AXP2101_COMM_CFG        (0X10)
#define AXP2101_BATFET_CTRL     (0X12)
#define AXP2101_DIE_TEMP_CFG    (0X13)
#define AXP2101_VSYS_MIN        (0x14)
#define AXP2101_VINDPM_CFG      (0x15)
#define AXP2101_IIN_LIM         (0x16)
#define AXP2101_RESET_CFG       (0x17)
#define AXP2101_MODULE_EN       (0x18)
#define AXP2101_WATCHDOG_CFG    (0x19)
#define AXP2101_GAUGE_THLD      (0x1A)
#define AXP2101_GPIO12_CTRL     (0x1B)
#define AXP2101_GPIO34_CTRL     (0x1C)
#define AXP2101_BUS_MODE_SEL    (0x1D)
#define AXP2101_PWRON_STAT      (0x20)
#define AXP2101_PWROFF_STAT     (0x21)
#define AXP2101_PWROFF_EN       (0x22)
#define AXP2101_DCDC_PWROFF_EN  (0x23)
#define AXP2101_VOFF_THLD       (0x24)
#define AXP2101_PWR_TIME_CTRL   (0x25)
#define AXP2101_SLEEP_CFG       (0x26)
#define AXP2101_PONLEVEL        (0x27)
#define AXP2101_FAST_PWRON_CFG0 (0x28)
#define AXP2101_FAST_PWRON_CFG1 (0x29)
#define AXP2101_FAST_PWRON_CFG2 (0x2A)
#define AXP2101_FAST_PWRON_CFG3 (0x2B)
#define AXP2101_ADC_CH_EN0      (0x30)
#define AXP2101_ADC_CH_EN1      (0x31)
#define AXP2101_ADC_CH_EN2      (0x32)
#define AXP2101_ADC_CH_EN3      (0x33)
#define AXP2101_VBAT_H          (0x34)
#define AXP2101_VBAT_L          (0x35)
#define AXP2101_TS_H            (0x36)
#define AXP2101_TS_L            (0x37)
#define AXP2101_VBUS_H          (0x38)
#define AXP2101_VBUS_L          (0x39)
#define AXP2101_VSYS_H          (0x3A)
#define AXP2101_VSYS_L          (0x3B)
#define AXP2101_TDIE_H          (0x3C)
#define AXP2101_TDIE_L          (0x3D)
#define AXP2101_GPADC_H         (0x3E)
#define AXP2101_GPADC_L         (0x3F)
#define AXP2101_INTEN1          (0x40)
#define AXP2101_INTEN2          (0x41)
#define AXP2101_INTEN3          (0x42)
#define AXP2101_INTSTS1         (0x48)
#define AXP2101_INTSTS2         (0x49)
#define AXP2101_INTSTS3         (0x4A)
#define AXP2101_TS_CFG          (0x50)

#define AXP2101_TS_HYSHL2H      (0x52)
#define AXP2101_TS_HYSH21       (0x53)
#define AXP2101_VLTF_CHG        (0x54)
#define AXP2101_VHTF_CHG        (0x55)
#define AXP2101_VLTF_WORK       (0x56)
#define AXP2101_VHTF_WORK       (0x57)
#define AXP2101_JEITA_CFG       (0x58)
#define AXP2101_JEITA_CV_CFG    (0x59)
#define AXP2101_JEITA_COOL      (0x5A)
#define AXP2101_JEITA_WARM      (0x5B)
#define AXP2101_TS_CFG_DATA_H   (0x5C)
#define AXP2101_TS_CFG_DATA_L   (0x5D)
#define AXP2101_CHG_CFG         (0x60)
#define AXP2101_IPRECHG_CFG     (0x61)
#define AXP2101_ICC_CFG         (0x62)
#define AXP2101_ITERM_CFG       (0x63)
#define AXP2101_CHG_V_CFG       (0x64)
#define AXP2101_TREGU_THLD      (0x65)
#define AXP2101_CHG_FREQ        (0x66)
#define AXP2101_CHG_TMR_CFG     (0x67)
#define AXP2101_BAT_DET         (0x68)
#define AXP2101_CHGLED_CFG      (0x69)
#define AXP2101_BTN_CHG_CFG     (0x6A)
#define AXP2101_SW_TEST_CFG     (0x7B)
#define AXP2101_DCDC_CFG0       (0x80)
#define AXP2101_DCDC_CFG1       (0x81)
#define AXP2101_DCDC1_CFG       (0x82)
#define AXP2101_DCDC2_CFG       (0x83)
#define AXP2101_DCDC3_CFG       (0x84)
#define AXP2101_DCDC4_CFG       (0x85)
#define AXP2101_DCDC5_CFG       (0x86)
#define AXP2101_DCDC_OC_CFG     (0x87)
#define AXP2101_LDO_EN_CFG0     (0x90)
#define AXP2101_LDO_EN_CFG1     (0x91)
#define AXP2101_ALDO1_CFG       (0x92)
#define AXP2101_ALDO2_CFG       (0x93)
#define AXP2101_ALDO3_CFG       (0x94)
#define AXP2101_ALDO4_CFG       (0x95)
#define AXP2101_BLDO1_CFG       (0x96)
#define AXP2101_BLDO2_CFG       (0x97)
#define AXP2101_CPUSLD_CFG      (0x98)
#define AXP2101_DLDO1_CFG       (0x99)
#define AXP2101_DLDO2_CFG       (0x9A)
#define AXP2101_IP_VER          (0xA0)
#define AXP2101_BROM            (0xA1)
#define AXP2101_CONFIG          (0xA2)
#define AXP2101_TEMPERATURE     (0xA3)
#define AXP2101_SOC             (0xA4)
#define AXP2101_TIME2EMPTY_H    (0xA6)
#define AXP2101_TIME2EMPTY_L    (0xA7)
#define AXP2101_TIME2FULL_H     (0xA8)
#define AXP2101_TIME2FULL_L     (0xA9)
#define AXP2101_FW_VERSION      (0xAB)
#define AXP2101_INT0_FLAG       (0xAC)
#define AXP2101_COUTER_PERIOD   (0xAD)
#define AXP2101_BG_TRIM         (0xAE)
#define AXP2101_OSC_TRIM        (0xAF)
#define AXP2101_FG_ADDR         (0xB0)
#define AXP2101_FG_DATA_H       (0xB2)
#define AXP2101_FG_DATA_L       (0xB3)
#define AXP2101_RAM_MBIST       (0xB4)
#define AXP2101_ROM_TEST        (0xB5)
#define AXP2101_ROM_TEST_RT0    (0xB6)
#define AXP2101_ROM_TEST_RT1    (0xB7)
#define AXP2101_ROM_TEST_RT2    (0xB8)
#define AXP2101_ROM_TEST_RT3    (0xB9)
#define AXP2101_WD_CLR_DIS      (0xBA)

#define AXP2101_BUFFERC         (0xff)
#define AXP2101_COMM_CFG0       (0x100)

/* For AXP15 */
#define AXP15_STATUS              (0x00)
#define AXP15_MODE_CHGSTATUS      (0x01)
#define AXP15_OTG_STATUS          (0x02)
#define AXP15_IC_TYPE             (0x03)
#define AXP15_DATA_BUFFER1        (0x04)
#define AXP15_DATA_BUFFER2        (0x05)
#define AXP15_DATA_BUFFER3        (0x06)
#define AXP15_DATA_BUFFER4        (0x07)
#define AXP15_DATA_BUFFER5        (0x08)
#define AXP15_DATA_BUFFER6        (0x09)
#define AXP15_DATA_BUFFER7        (0x0A)
#define AXP15_DATA_BUFFER8        (0x0B)
#define AXP15_DATA_BUFFER9        (0x0C)
#define AXP15_DATA_BUFFERA        (0x0D)
#define AXP15_DATA_BUFFERB        (0x0E)
#define AXP15_DATA_BUFFERC        (0x0F)
#define AXP15_LDO3456_DC1234_CTL  (0x12)
#define AXP15_LDO0OUT_VOL         (0x15)
#define AXP15_DC2OUT_VOL          (0x23)
#define AXP15_DCDC2_DVM_CTRL      (0x25)
#define AXP15_DC1OUT_VOL          (0x26)
#define AXP15_DC3OUT_VOL          (0x27)
#define AXP15_LDO34OUT_VOL        (0x28)
#define AXP15_LDO5OUT_VOL         (0x29)
#define AXP15_LDO6OUT_VOL         (0x2A)
#define AXP15_DC4OUT_VOL          (0x2B)
#define AXP15_IPS_SET             (0x30)
#define AXP15_VOFF_SET            (0x31)
#define AXP15_OFF_CTL             (0x32)
#define AXP15_CHARGE1             (0x33)
#define AXP15_CHARGE2             (0x34)
#define AXP15_BACKUP_CHG          (0x35)
#define AXP15_POK_SET             (0x36)
#define AXP15_DCDC_FREQSET        (0x37)
#define AXP15_VLTF_CHGSET         (0x38)
#define AXP15_VHTF_CHGSET         (0x39)
#define AXP15_APS_WARNING1        (0x3A)
#define AXP15_APS_WARNING2        (0x3B)
#define AXP15_TLTF_DISCHGSET      (0x3C)
#define AXP15_THTF_DISCHGSET      (0x3D)
#define AXP15_INTEN1              (0x40)
#define AXP15_INTEN2              (0x41)
#define AXP15_INTEN3              (0x42)
#define AXP15_INTSTS1             (0x48)
#define AXP15_INTSTS2             (0x49)
#define AXP15_INTSTS3             (0x4A)
#define AXP15_DCDC_MODESET        (0x80)
#define AXP15_ADC_EN1             (0x82)
#define AXP15_ADC_EN2             (0x83)
#define AXP15_ADC_SPEED           (0x84)
#define AXP15_ADC_INPUTRANGE      (0x85)
#define AXP15_ADC_IRQ_RETFSET     (0x86)
#define AXP15_ADC_IRQ_FETFSET     (0x87)
#define AXP15_TIMER_CTL           (0x8A)
#define AXP15_VBUS_DET_SRP        (0x8B)
#define AXP15_HOTOVER_CTL         (0x8F)
#define AXP15_GPIO0_CTL           (0x90)
#define AXP15_GPIO1_CTL           (0x91)
#define AXP15_GPIO2_CTL           (0x92)
#define AXP15_GPIO3_CTL           (0x93)
#define AXP15_GPIO012_SIGNAL      (0x94)
#define AXP15_GPIO0_VOL           (0x96)
#define AXP15_GPIO0123_SIGNAL     (0x97)

/* For AXP1530 */
#define AXP1530_ON_INDICATE          (0x00)
#define AXP1530_OFF_INDICATE         (0x01)
#define AXP1530_IC_TYPE              (0x03)
#define AXP1530_OUTPUT_CONTROL       (0x10)
#define AXP1530_DCDC_DVM_PWM         (0x12)
#define AXP1530_DCDC1_CONRTOL        (0x13)
#define AXP1530_DCDC2_CONRTOL        (0x14)
#define AXP1530_DCDC3_CONRTOL        (0x15)
#define AXP1530_ALDO1_CONRTOL        (0x16)
#define AXP1530_DLDO1_CONRTOL        (0x17)
#define AXP1530_POWER_STATUS         (0x1A)
#define AXP1530_PWROK_SET            (0x1B)
#define AXP1530_WAKEUP_CONRTOL       (0x1C)
#define AXP1530_OUTOUT_MONITOR       (0x1D)
#define AXP1530_POK_CONRTOL          (0x1E)
#define AXP1530_IRQ_ENABLE1          (0x20)
#define AXP1530_IRQ_STATUS1          (0x21)
#define AXP1530_LOCK_REG71           (0x70)
#define AXP1530_EPROM_SET            (0x71)
#define AXP1530_DCDC12_DEFAULT       (0x80)
#define AXP1530_DCDC3_A1D1_DEFAULT   (0x81)
#define AXP1530_STARTUP_SEQ          (0x82)
#define AXP1530_STARTUP_RTCLDO       (0x83)
#define AXP1530_BIAS_I2C_ADDR        (0x84)
#define AXP1530_VREF_VRPN            (0x85)
#define AXP1530_VREF_VOL             (0x86)
#define AXP1530_FREQUENCY            (0x87)

/* For AXP858 */
#define AXP858_ON_INDICATE          (0x00)
#define AXP858_OFF_INDICATE         (0x01)
#define AXP858_IC_TYPE              (0x03)
#define AXP858_DATA_BUFFER1        (0x04)
#define AXP858_DATA_BUFFER2        (0x05)
#define AXP858_DATA_BUFFER3        (0x06)
#define AXP858_DATA_BUFFER4        (0x07)

#define AXP858_OUTPUT_CONTROL1       (0x10)
#define AXP858_OUTPUT_CONTROL2      (0x11)
#define AXP858_OUTPUT_CONTROL3        (0x12)
#define AXP858_DCDC1_CONTROL        (0x13)
#define AXP858_DCDC2_CONTROL        (0x14)
#define AXP858_DCDC3_CONTROL        (0x15)
#define AXP858_DCDC4_CONTROL        (0x16)
#define AXP858_DCDC5_CONTROL         (0x17)
#define AXP858_DCDC6_CONTROL            (0x18)
#define AXP858_ALDO1_CONTROL       (0x19)
#define AXP858_DCDC_MODE1       (0x1A)
#define AXP858_DCDC_MODE2          (0x1B)
#define AXP858_DCDC_MODE3          (0x1C)
#define AXP858_DCDC_FREQUENCY          (0x1D)
#define AXP858_OUTPUT_MONITOR           (0x1E)
#define AXP858_IRQ_PWROK_VOFF            (0x1F)
#define AXP858_ALDO2_CTL       (0x20)
#define AXP858_ALDO3_CTL   (0x21)
#define AXP858_ALDO4_CTL          (0x22)
#define AXP858_ALDO5_CTL       (0x23)
#define AXP858_BLDO1_CTL        (0x24)
#define AXP858_BLDO2_CTL            (0x25)
#define AXP858_BLDO3_CTL             (0x26)
#define AXP858_BLDO4_CTL            (0x27)
#define AXP858_BLDO5_CTL            (0x28)
#define AXP858_CLDO1_CTL            (0x29)
#define AXP858_CLDO2_CTL            (0x2A)
#define AXP858_CLDO3_GPIO1_CTL            (0x2B)
#define AXP858_CLDO4_GPIO2_CTL            (0x2C)
#define AXP858_CLDO4_CTL            (0x2D)
#define AXP858_CPUSLDO_CTL            (0x2E)
#define AXP858_WKP_CTL_OC_IRQ            (0x31)
#define AXP858_POWER_DOWN_DIS            (0x32)
#define AXP858_POK_SET            (0x36)
#define AXP858_TWI_OR_RSB            (0x3E)
#define AXP858_IRQ_EN1            (0x40)
#define AXP858_IRQ_EN2            (0x41)
#define AXP858_IRQ_STS1            (0x48)
#define AXP858_IRQ_STS2            (0x49)
#define AXP858_DIGITAL_PAT1            (0xF0)
#define AXP858_DIGITAL_PAT2            (0xF1)
#define AXP858_EPROM_SET            (0xF2)
#define AXP858_VREF_TEM_SET            (0xF3)
#define AXP858_LOCK_F0125            (0xF4)
#define AXP858_EPROM_TUNE            (0xF5)
#define AXP858_ADDR_EXTEN            (0xFF)
#define AXP858_DCDC1_PWRON_DEF          (0x100)
#define AXP858_DCDC2_DEF          (0x101)
#define AXP858_DCDC3_DEF          (0x102)
#define AXP858_DCDC4_DEF          (0x103)
#define AXP858_DCDC5_DEF          (0x104)
#define AXP858_DCDC6_DEF          (0x105)
#define AXP858_ALDO12_DEF          (0x106)
#define AXP858_ALDO23_DEF          (0x107)
#define AXP858_ALDO45_DEF          (0x108)
#define AXP858_ALDO5_BLDO1_DEF          (0x109)
#define AXP858_BLDO12_DEF          (0x10A)
#define AXP858_BLDO23_DEF          (0x10B)
#define AXP858_BLDO45_DEF          (0x10C)
#define AXP858_BLDO5_CLDO1_DEF          (0x10D)
#define AXP858_CLDO23_DEF          (0x10E)
#define AXP858_CLDO34_DEF          (0x10F)
#define AXP858_START_DCDC123          (0x110)
#define AXP858_START_DCDC456          (0x111)
#define AXP858_START_ALDO12          (0x112)
#define AXP858_START_ALDO345          (0x113)
#define AXP858_START_BLDO123          (0x114)
#define AXP858_START_BLDO45          (0x115)
#define AXP858_START_CLDO123          (0x116)
#define AXP858_START_CLDO34_CPUS          (0x117)
#define AXP858_TWI_RSB_SET1          (0x118)
#define AXP858_TWI_RSB_SET2          (0x119)
#define AXP858_TWI_SET          (0x11A)
#define AXP858_VREF_TC_ALDO3          (0x140)
#define AXP858_VREF_VOL          (0x141)
#define AXP858_INTERNAL_ALDO2          (0x142)
#define AXP858_FREQUENCY_ALDO2          (0x143)

/* For AXP803 */
#define AXP803_STATUS              (0x00)
#define AXP803_MODE_CHGSTATUS      (0x01)
#define AXP803_IC_TYPE             (0x03)
#define AXP803_BUFFER1             (0x04)
#define AXP803_BUFFER2             (0x05)
#define AXP803_BUFFER3             (0x06)
#define AXP803_BUFFER4             (0x07)
#define AXP803_BUFFER5             (0x08)
#define AXP803_BUFFER6             (0x09)
#define AXP803_BUFFER7             (0x0A)
#define AXP803_BUFFER8             (0x0B)
#define AXP803_BUFFER9             (0x0C)
#define AXP803_BUFFERA             (0x0D)
#define AXP803_BUFFERB             (0x0E)
#define AXP803_BUFFERC             (0x0F)
#define AXP803_LDO_DC_EN1          (0X10)
#define AXP803_LDO_DC_EN2          (0X12)
#define AXP803_LDO_DC_EN3          (0X13)
#define AXP803_DLDO1OUT_VOL        (0x15)
#define AXP803_DLDO2OUT_VOL        (0x16)
#define AXP803_DLDO3OUT_VOL        (0x17)
#define AXP803_DLDO4OUT_VOL        (0x18)
#define AXP803_ELDO1OUT_VOL        (0x19)
#define AXP803_ELDO2OUT_VOL        (0x1A)
#define AXP803_ELDO3OUT_VOL        (0x1B)
#define AXP803_FLDO1OUT_VOL        (0x1C)
#define AXP803_FLDO2OUT_VOL        (0x1D)
#define AXP803_DC1OUT_VOL          (0x20)
#define AXP803_DC2OUT_VOL          (0x21)
#define AXP803_DC3OUT_VOL          (0x22)
#define AXP803_DC4OUT_VOL          (0x23)
#define AXP803_DC5OUT_VOL          (0x24)
#define AXP803_DC6OUT_VOL          (0x25)
#define AXP803_DC7OUT_VOL          (0x26)
#define AXP803_DCDC_DVM_CTL        (0x27)
#define AXP803_ALDO1OUT_VOL        (0x28)
#define AXP803_ALDO2OUT_VOL        (0x29)
#define AXP803_ALDO3OUT_VOL        (0x2A)
#define AXP803_BC_CTL              (0X2C)
#define AXP803_IPS_SET             (0x30)
#define AXP803_VOFF_SET            (0x31)
#define AXP803_OFF_CTL             (0x32)
#define AXP803_CHARGE1             (0x33)
#define AXP803_CHARGE2             (0x34)
#define AXP803_CHARGE3             (0x35)
#define AXP803_POK_SET             (0x36)
#define AXP803_VLTF_CHARGE         (0x38)
#define AXP803_VHTF_CHARGE         (0x39)
#define AXP803_CHARGE_AC_SET       (0x3A)
#define AXP803_DCDC_FREQSET        (0x3B)
#define AXP803_VLTF_WORK           (0x3C)
#define AXP803_VHTF_WORK           (0x3D)
#define AXP803_INTEN1              (0x40)
#define AXP803_INTEN2              (0x41)
#define AXP803_INTEN3              (0x42)
#define AXP803_INTEN4              (0x43)
#define AXP803_INTEN5              (0x44)
#define AXP803_INTEN6              (0x45)
#define AXP803_INTSTS1             (0x48)
#define AXP803_INTSTS2             (0x49)
#define AXP803_INTSTS3             (0x4A)
#define AXP803_INTSTS4             (0x4B)
#define AXP803_INTSTS5             (0x4C)
#define AXP803_INTSTS6             (0x4D)
#define AXP803_INTTEMP             (0x56)
#define AXP803_VTS_RES             (0x58)
#define AXP803_VBATH_RES           (0x78)
#define AXP803_VBATL_RES           (0x79)
#define AXP803_IBATH_REG           (0x7A)
#define AXP803_DISIBATH_REG        (0x7C)
#define AXP803_DCDC_MODESET        (0x80)
#define AXP803_ADC_EN              (0x82)
#define AXP803_ADC_TS_CTL          (0x84)
#define AXP803_ADC_SPEED_SET       (0x85)
#define AXP803_HOTOVER_CTL         (0x8F)
#define AXP803_GPIO0_CTL           (0x90)
#define AXP803_GPIO0LDOOUT_VOL     (0x91)
#define AXP803_GPIO1_CTL           (0x92)
#define AXP803_GPIO1LDOOUT_VOL     (0x93)
#define AXP803_GPIO01_SIGNAL       (0x94)
#define AXP803_BAT_CHGCOULOMB3     (0xB0)
#define AXP803_BAT_CHGCOULOMB2     (0xB1)
#define AXP803_BAT_CHGCOULOMB1     (0xB2)
#define AXP803_BAT_CHGCOULOMB0     (0xB3)
#define AXP803_BAT_DISCHGCOULOMB3  (0xB4)
#define AXP803_BAT_DISCHGCOULOMB2  (0xB5)
#define AXP803_BAT_DISCHGCOULOMB1  (0xB6)
#define AXP803_BAT_DISCHGCOULOMB0  (0xB7)
#define AXP803_COULOMB_CTL         (0xB8)
#define AXP803_CAP                 (0xB9)
#define AXP803_RDC0                (0xBA)
#define AXP803_RDC1                (0xBB)
#define AXP803_OCVBATH_RES         (0xBC)
#define AXP803_OCVBATL_RES         (0xBD)
#define AXP803_OCVCAP              (0xC0)
#define AXP803_BATCAP0             (0xE0)
#define AXP803_BATCAP1             (0xE1)
#define AXP803_COUCNT0             (0xE2)
#define AXP803_COUCNT1             (0xE3)
#define AXP803_OCV_PERCENT         (0xE4)
#define AXP803_COU_PERCENT         (0xE5)
#define AXP803_WARNING_LEVEL       (0xE6)
#define AXP803_ADJUST_PARA         (0xE8)
#define AXP803_ADJUST_PARA1        (0xE9)
#define AXP803_HOTOVER_VAL         (0xF3)
#define AXP803_REG_ADDR_EXT        (0xFF)

/*
 * axp2202 define
 */
#define AXP2202_COMM_STAT0          (0x00)
#define AXP2202_COMM_STAT1          (0x01)
#define AXP2202_CHIP_ID             (0x03)
#define AXP2202_CHIP_VER            (0x04)
#define AXP2202_BC_DECT             (0x05)
#define AXP2202_ILIM_TYPE           (0x06)
#define AXP2202_COMM_FAULT          (0x08)
#define AXP2202_ICO_CFG             (0x0a)
#define AXP2202_CLK_EN              (0x0b)
#define AXP2202_CURVE_CHECK         (0x0c)
#define AXP2202_VBUS_TYPE           (0x0f)

#define AXP2202_COMM_CFG            (0x10)
#define AXP2202_BATFET_CTRL         (0x12)
#define AXP2202_RBFET_CTRL          (0x13)
#define AXP2202_DIE_TEMP_CFG        (0x14)
#define AXP2202_VSYS_MIN            (0x15)
#define AXP2202_VINDPM_CFG          (0x16)
#define AXP2202_IIN_LIM             (0x17)
#define AXP2202_RESET_CFG           (0x18)
#define AXP2202_MODULE_EN           (0x19)
#define AXP2202_WATCHDOG_CFG        (0x1a)
#define AXP2202_GAUGE_THLD          (0x1b)
#define AXP2202_GPIO_CTRL           (0x1c)
#define AXP2202_LOW_POWER_CFG       (0x1d)
#define AXP2202_BST_CFG0            (0x1e)
#define AXP2202_BST_CFG1            (0x1f)

#define AXP2202_PWRON_STAT          (0x20)
#define AXP2202_PWROFF_STAT         (0x21)
#define AXP2202_PWROFF_EN           (0x22)
#define AXP2202_DCDC_PWROFF_EN      (0x23)
#define AXP2202_PWR_TIME_CTRL       (0x24)
#define AXP2202_SLEEP_CFG           (0x25)
#define AXP2202_PONLEVEL            (0x26)
#define AXP2202_SOFT_PWROFF         (0x27)
#define AXP2202_AUTO_SLP_MAP0       (0x28)
#define AXP2202_AUTOSLP_MAP1        (0x29)
#define AXP2202_AUTOSLP_MAP2        (0x2a)
#define AXP2202_FAST_PWRON_CFG0     (0x2b)
#define AXP2202_FAST_PWRON_CFG1     (0x2c)
#define AXP2202_FAST_PWRON_CFG2     (0x2d)
#define AXP2202_FAST_PWRON_CFG3     (0x2e)
#define AXP2202_FAST_PWRON_CFG4     (0x2f)
#define AXP2202_I2C_CFG             (0x30)
#define AXP2202_BUS_MODE_SEL        (0x3e)

#define AXP2202_IRQ_EN0             (0x40)
#define AXP2202_IRQ_EN1             (0x41)
#define AXP2202_IRQ_EN2             (0x42)
#define AXP2202_IRQ_EN3             (0x43)
#define AXP2202_IRQ_EN4             (0x44)
#define AXP2202_IRQ0                (0x48)
#define AXP2202_IRQ1                (0x49)
#define AXP2202_IRQ2                (0x4a)
#define AXP2202_IRQ3                (0x4b)
#define AXP2202_IRQ4                (0x4c)

#define AXP2202_TS_CFG              (0x50)
#define AXP2202_TS_HYSL2H           (0x52)
#define AXP2202_TS_HYSH2L           (0x53)
#define AXP2202_VLTF_CHG            (0x54)
#define AXP2202_VHTF_CHG            (0x55)
#define AXP2202_VLTF_WORK           (0x56)
#define AXP2202_VHTF_WORK           (0x57)
#define AXP2202_JEITA_CFG           (0x58)
#define AXP2202_JEITA_CV_CFG        (0x59)
#define AXP2202_JEITA_COOL          (0x5a)
#define AXP2202_JEITA_WARM          (0x5b)
#define AXP2202_TS_CFG_DATA_H       (0x5c)
#define AXP2202_TS_CFG_DATA_L       (0x5d)

#define AXP2202_RECHG_CFG           (0x60)
#define AXP2202_IPRECHG_CFG         (0x61)
#define AXP2202_ICC_CFG             (0x62)
#define AXP2202_ITERM_CFG           (0x63)
#define AXP2202_VTERM_CFG           (0x64)
#define AXP2202_TREGU_THLD          (0x65)
#define AXP2202_CHG_FREQ            (0x66)
#define AXP2202_CHG_TMR_CFG         (0x67)
#define AXP2202_BAT_DET             (0x68)
#define AXP2202_IR_COMP             (0x69)
#define AXP2202_BTN_CHG_CFG         (0x6a)
#define AXP2202_SW_TEST_CFG         (0x6b)

#define AXP2202_CHGLED_CFG          (0x70)
#define AXP2202_LOW_NUM             (0x72)
#define AXP2202_HIGH_NUM            (0x73)
#define AXP2202_TRANS_NUM           (0x74)
#define AXP2202_DUTY_STEP           (0x76)
#define AXP2202_DUTY_MIN            (0x77)
#define AXP2202_PWN_PERIOD          (0x78)

#define AXP2202_DCDC_CFG0           (0x80)
#define AXP2202_DCDC_CFG1           (0x81)
#define AXP2202_DCDC_CFG2           (0x82)
#define AXP2202_DCDC1_CFG           (0x83)
#define AXP2202_DCDC2_CFG           (0x84)
#define AXP2202_DCDC3_CFG           (0x85)
#define AXP2202_DCDC4_CFG           (0x86)
#define AXP2202_DVM_STAT            (0x87)
#define AXP2202_DCDC_OC_CFG         (0x88)
#define AXP2202_DCDC_VDSDT_ADJ      (0x89)

#define AXP2202_LDO_EN_CFG0         (0x90)
#define AXP2202_LDO_EN_CFG1         (0x91)
#define AXP2202_ALDO1_CFG           (0x93)
#define AXP2202_ALDO2_CFG           (0x94)
#define AXP2202_ALDO3_CFG           (0x95)
#define AXP2202_ALDO4_CFG           (0x96)
#define AXP2202_BLDO1_CFG           (0x97)
#define AXP2202_BLDO2_CFG           (0x98)
#define AXP2202_BLDO3_CFG           (0x99)
#define AXP2202_BLDO4_CFG           (0x9a)
#define AXP2202_CLDO1_CFG           (0x9b)
#define AXP2202_CLDO2_CFG           (0x9c)
#define AXP2202_CLDO3_CFG           (0x9d)
#define AXP2202_CLDO4_CFG           (0x9e)
#define AXP2202_CPUSLDO_CFG         (0x9f)

#define AXP2202_GAUGE_IP_VER        (0xa0)
#define AXP2202_GAUGE_BROM          (0xa1)
#define AXP2202_GAUGE_CONFIG        (0xa2)
#define AXP2202_GAUGE_TEMP   (0xa3)
#define AXP2202_GAUGE_SOC           (0xa4)
#define AXP2202_GAUGE_TIME2EMPTY_H  (0xa6)
#define AXP2202_GAUGE_TIME2EMPTY_L  (0xa7)
#define AXP2202_GAUGE_TIME2FULL_H   (0xa8)
#define AXP2202_GAUGE_TIME2FULL_L   (0xa9)
#define AXP2202_GAUGE_FW_VERSION    (0xab)
#define AXP2202_GAUGE_INT0_FLAG     (0xac)
#define AXP2202_GAUGE_COUTER_PERIOD (0xad)
#define AXP2202_GAUGE_FG_ADDR       (0xb0)
#define AXP2202_GAUGE_FG_DATA_H     (0xb2)
#define AXP2202_GAUGE_FG_DATA_L     (0xb3)
#define AXP2202_GAUGE_RAM_MBIST     (0xb4)
#define AXP2202_GAUGE_ROM_TEST      (0xb5)
#define AXP2202_GAUGE_ROM_TEST_RT0  (0xb6)
#define AXP2202_GAUGE_ROM_TEST_RT1  (0xb7)
#define AXP2202_GAUGE_ROM_TEST_RT2  (0xb8)
#define AXP2202_GAUGE_ROM_TEST_RT3  (0xb9)
#define AXP2202_GAUGE_WD_CLR_DIS    (0xba)

#define AXP2202_ADC_CH_EN0          (0xc0)
#define AXP2202_ADC_CH_EN1          (0xc1)
#define AXP2202_ADC_CH_EN2          (0xc2)
#define AXP2202_ADC_CH_EN3          (0xc3)
#define AXP2202_VBAT_H              (0xc4)
#define AXP2202_VBAT_L              (0xc5)
#define AXP2202_VBUS_H              (0xc6)
#define AXP2202_VBUS_L              (0xc7)
#define AXP2202_VSYS_H              (0xc8)
#define AXP2202_VSYS_L              (0xc9)
#define AXP2202_ICHG_H              (0xca)
#define AXP2202_ICHG_L              (0xcb)
#define AXP2202_CH_DBG_SEL          (0xcc)
#define AXP2202_ADC_DATA_SEL        (0xcd)
#define AXP2202_ADC_DATA_H          (0xce)
#define AXP2202_ADC_DATA_L          (0xcf)

#define AXP2202_BC_CFG0             (0xd0)
#define AXP2202_BC_CFG1             (0xd1)
#define AXP2202_BC_CFG2             (0xd2)
#define AXP2202_BC_CFG3             (0xd3)

#define AXP2202_CC_VERSION          (0xe0)
#define AXP2202_CC_GLB_CTRL         (0xe1)
#define AXP2202_CC_LP_CTRL          (0xe2)
#define AXP2202_CC_MODE_CTRL        (0xe3)
#define AXP2202_CC_TGL_CTRL0        (0xe4)
#define AXP2202_CC_TGL_CTRL1        (0xe5)
#define AXP2202_CC_ANA_CTRL         (0xe6)
#define AXP2202_CC_STAT0            (0xe7)
#define AXP2202_CC_STAT1            (0xe8)
#define AXP2202_CC_STAT2            (0xe9)
#define AXP2202_CC_STAT3            (0xea)
#define AXP2202_CC_STAT4            (0xeb)
#define AXP2202_CC_ANA_STAT0        (0xec)
#define AXP2202_CC_ANA_STAT1        (0xed)
#define AXP2202_CC_ANA_STAT2        (0xee)

#define AXP2202_EFUS_OP_CFG         (0xf0)
#define AXP2202_EFREG_CTRL          (0xf1)
#define AXP2202_TWI_ADDR_EXT        (0xff)
/*
 * end of define axp2202
 */

/* For axp2585 */
#define AXP2585_STATUS              (0x00)
#define AXP2585_IC_TYPE             (0x03)
#define AXP2585_ILIMIT              (0x10)
#define AXP2585_RBFET_SET           (0x11)
#define AXP2585_POK_SET             (0x15)
#define AXP2585_GPIO1_CTL           (0x18)
#define AXP2585_GPIO2_CTL           (0x19)
#define AXP2585_GPIO1_SIGNAL        (0x1A)
#define AXP2585_CC_EN               (0x22)
#define AXP2585_ADC_EN              (0x24)
#define AXP2585_OFF_CTL             (0x28)
#define AXP2585_CC_LOW_POWER_CTRL   (0x32)
#define AXP2585_CC_MODE_CTRL        (0x33)
#define AXP2585_CC_STATUS0          (0x37)
#define AXP2585_INTEN1              (0x40)
#define AXP2585_INTEN2              (0x41)
#define AXP2585_INTEN3              (0x42)
#define AXP2585_INTEN4              (0x43)
#define AXP2585_INTEN5              (0x44)
#define AXP2585_INTEN6              (0x45)
#define AXP2585_INTSTS1             (0x48)
#define AXP2585_INTSTS2             (0x49)
#define AXP2585_INTSTS3             (0x4A)
#define AXP2585_INTSTS4             (0x4B)
#define AXP2585_INTSTS5             (0x4C)
#define AXP2585_INTSTS6             (0x4D)
#define AXP2585_VBATH_REG           (0x78)
#define AXP2585_IBATH_REG           (0x7A)
#define AXP2585_DISIBATH_REG        (0x7c)
#define AXP2585_ADC_CONTROL         (0x80)
#define AXP2585_TS_PIN_CONTROL      (0x81)
#define AXP2585_VLTF_CHARGE         (0x84)
#define AXP2585_VHTF_CHARGE         (0x85)
#define AXP2585_VLTF_WORK           (0x86)
#define AXP2585_VHTF_WORK           (0x87)
#define AXP2585_ICC_CFG             (0x8B)
#define AXP2585_CHARGE_CONTROL2     (0x8C)
#define AXP2585_TIMER2_SET          (0x8E)
#define AXP2585_COULOMB_CTL         (0xB8)
#define AXP2585_CAP                 (0xB9)
#define AXP2585_RDC0                (0xBA)
#define AXP2585_RDC1               (0xBB)
#define AXP2585_BATCAP0             (0xE0)
#define AXP2585_BATCAP1             (0xE1)
#define AXP2585_WARNING_LEVEL       (0xE6)
#define AXP2585_ADJUST_PARA         (0xE8)
#define AXP2585_ADJUST_PARA1        (0xE9)
#define AXP2585_ADDR_EXTENSION      (0xFF)

/* Regulators IDs */
enum {
	AXP152_DCDC1 = 0,
	AXP152_DCDC2,
	AXP152_DCDC3,
	AXP152_DCDC4,
	AXP152_ALDO1,
	AXP152_ALDO2,
	AXP152_DLDO1,
	AXP152_DLDO2,
	AXP152_LDO0,
	AXP152_GPIO2_LDO,
	AXP152_RTC13,
	AXP152_RTC18,
	AXP152_REG_ID_MAX,
};

enum {
	AXP20X_LDO1 = 0,
	AXP20X_LDO2,
	AXP20X_LDO3,
	AXP20X_LDO4,
	AXP20X_LDO5,
	AXP20X_DCDC2,
	AXP20X_DCDC3,
	AXP20X_REG_ID_MAX,
};

enum {
	AXP22X_DCDC1 = 0,
	AXP22X_DCDC2,
	AXP22X_DCDC3,
	AXP22X_DCDC4,
	AXP22X_DCDC5,
	AXP22X_DC1SW,
	AXP22X_DC5LDO,
	AXP22X_ALDO1,
	AXP22X_ALDO2,
	AXP22X_ALDO3,
	AXP22X_ELDO1,
	AXP22X_ELDO2,
	AXP22X_ELDO3,
	AXP22X_DLDO1,
	AXP22X_DLDO2,
	AXP22X_DLDO3,
	AXP22X_DLDO4,
	AXP22X_RTC_LDO,
	AXP22X_LDO_IO0,
	AXP22X_LDO_IO1,
	AXP22X_REG_ID_MAX,
};

enum {
	AXP806_DCDCA = 0,
	AXP806_DCDCB,
	AXP806_DCDCC,
	AXP806_DCDCD,
	AXP806_DCDCE,
	AXP806_ALDO1,
	AXP806_ALDO2,
	AXP806_ALDO3,
	AXP806_BLDO1,
	AXP806_BLDO2,
	AXP806_BLDO3,
	AXP806_BLDO4,
	AXP806_CLDO1,
	AXP806_CLDO2,
	AXP806_CLDO3,
	AXP806_SW,
	AXP806_REG_ID_MAX,
};

enum {
	AXP809_DCDC1 = 0,
	AXP809_DCDC2,
	AXP809_DCDC3,
	AXP809_DCDC4,
	AXP809_DCDC5,
	AXP809_DC1SW,
	AXP809_DC5LDO,
	AXP809_ALDO1,
	AXP809_ALDO2,
	AXP809_ALDO3,
	AXP809_ELDO1,
	AXP809_ELDO2,
	AXP809_ELDO3,
	AXP809_DLDO1,
	AXP809_DLDO2,
	AXP809_RTC_LDO,
	AXP809_LDO_IO0,
	AXP809_LDO_IO1,
	AXP809_SW,
	AXP809_REG_ID_MAX,
};

enum {
	AXP2101_DCDC1 = 0,
	AXP2101_DCDC2,
	AXP2101_DCDC3,
	AXP2101_DCDC4,
	AXP2101_DCDC5,
	AXP2101_LDO1,  /* RTCLDO */
	AXP2101_LDO2,  /* RTCLDO1 */
	AXP2101_LDO3,  /* ALDO1 */
	AXP2101_LDO4,  /* ALDO2 */
	AXP2101_LDO5,  /* ALDO3 */
	AXP2101_LDO6,  /* ALDO4 */
	AXP2101_LDO7,  /* BLDO1 */
	AXP2101_LDO8,  /* BLDO2 */
	AXP2101_LDO9,  /* DLDO1 */
	AXP2101_LDO10, /* DLDO2 */
	AXP2101_LDO11, /* CPULDOS */
	AXP2101_REG_ID_MAX,
};

enum {
	AXP15_DCDC1 = 0,
	AXP15_DCDC2,
	AXP15_DCDC3,
	AXP15_DCDC4,
	AXP15_DCDC5,
	AXP15_LDO1,  /* RTCLDO */
	AXP15_LDO2,  /* RTCLDO1 */
	AXP15_LDO3,  /* ALDO1 */
	AXP15_LDO4,  /* ALDO2 */
	AXP15_LDO5,  /* ALDO3 */
	AXP15_LDO6,  /* ALDO4 */
	AXP15_LDO7,  /* BLDO1 */
	AXP15_REG_ID_MAX,
};

enum {
	AXP1530_DCDC1 = 0,
	AXP1530_DCDC2,
	AXP1530_DCDC3,
	AXP1530_LDO1,  /* RTCLDO */
	AXP1530_LDO2,  /* RTCLDO1 */
	AXP1530_REG_ID_MAX,
};

enum {
	AXP858_DCDC1 = 0,
	AXP858_DCDC2,
	AXP858_DCDC3,
	AXP858_DCDC4,
	AXP858_DCDC5,
	AXP858_DCDC6,
	AXP858_ALDO1,
	AXP858_ALDO2,
	AXP858_ALDO3,
	AXP858_ALDO4,
	AXP858_ALDO5,
	AXP858_BLDO1,
	AXP858_BLDO2,
	AXP858_BLDO3,
	AXP858_BLDO4,
	AXP858_BLDO5,
	AXP858_CLDO1,
	AXP858_CLDO2,
	AXP858_CLDO3,
	AXP858_CLDO4,
	AXP858_CPUSLDO,
	AXP858_DC1SW,
	AXP858_REG_ID_MAX,
};

enum {
	AXP803_DCDC1 = 0,
	AXP803_DCDC2,
	AXP803_DCDC3,
	AXP803_DCDC4,
	AXP803_DCDC5,
	AXP803_DCDC6,
	AXP803_DCDC7,
	AXP803_RTCLDO,
	AXP803_ALDO1,
	AXP803_ALDO2,
	AXP803_ALDO3,
	AXP803_DLDO1,
	AXP803_DLDO2,
	AXP803_DLDO3,
	AXP803_DLDO4,
	AXP803_ELDO1,
	AXP803_ELDO2,
	AXP803_ELDO3,
	AXP803_FLDO1,
	AXP803_FLDO2,
	AXP803_LDOIO0,
	AXP803_LDOIO1,
	AXP803_DC1SW,
	AXP803_REG_ID_MAX,
};

enum {
	AXP2585_REG_ID_MAX = 0,
};

/* IRQs */
enum {
	AXP152_IRQ_LDO0IN_CONNECT = 1,
	AXP152_IRQ_LDO0IN_REMOVAL,
	AXP152_IRQ_ALDO0IN_CONNECT,
	AXP152_IRQ_ALDO0IN_REMOVAL,
	AXP152_IRQ_DCDC1_V_LOW,
	AXP152_IRQ_DCDC2_V_LOW,
	AXP152_IRQ_DCDC3_V_LOW,
	AXP152_IRQ_DCDC4_V_LOW,
	AXP152_IRQ_PEK_SHORT,
	AXP152_IRQ_PEK_LONG,
	AXP152_IRQ_TIMER,
	AXP152_IRQ_PEK_RIS_EDGE,
	AXP152_IRQ_PEK_FAL_EDGE,
	AXP152_IRQ_GPIO3_INPUT,
	AXP152_IRQ_GPIO2_INPUT,
	AXP152_IRQ_GPIO1_INPUT,
	AXP152_IRQ_GPIO0_INPUT,
};

enum {
	AXP20X_IRQ_ACIN_OVER_V = 1,
	AXP20X_IRQ_ACIN_PLUGIN,
	AXP20X_IRQ_ACIN_REMOVAL,
	AXP20X_IRQ_VBUS_OVER_V,
	AXP20X_IRQ_VBUS_PLUGIN,
	AXP20X_IRQ_VBUS_REMOVAL,
	AXP20X_IRQ_VBUS_V_LOW,
	AXP20X_IRQ_BATT_PLUGIN,
	AXP20X_IRQ_BATT_REMOVAL,
	AXP20X_IRQ_BATT_ENT_ACT_MODE,
	AXP20X_IRQ_BATT_EXIT_ACT_MODE,
	AXP20X_IRQ_CHARG,
	AXP20X_IRQ_CHARG_DONE,
	AXP20X_IRQ_BATT_TEMP_HIGH,
	AXP20X_IRQ_BATT_TEMP_LOW,
	AXP20X_IRQ_DIE_TEMP_HIGH,
	AXP20X_IRQ_CHARG_I_LOW,
	AXP20X_IRQ_DCDC1_V_LONG,
	AXP20X_IRQ_DCDC2_V_LONG,
	AXP20X_IRQ_DCDC3_V_LONG,
	AXP20X_IRQ_PEK_SHORT = 22,
	AXP20X_IRQ_PEK_LONG,
	AXP20X_IRQ_N_OE_PWR_ON,
	AXP20X_IRQ_N_OE_PWR_OFF,
	AXP20X_IRQ_VBUS_VALID,
	AXP20X_IRQ_VBUS_NOT_VALID,
	AXP20X_IRQ_VBUS_SESS_VALID,
	AXP20X_IRQ_VBUS_SESS_END,
	AXP20X_IRQ_LOW_PWR_LVL1,
	AXP20X_IRQ_LOW_PWR_LVL2,
	AXP20X_IRQ_TIMER,
	AXP20X_IRQ_PEK_RIS_EDGE,
	AXP20X_IRQ_PEK_FAL_EDGE,
	AXP20X_IRQ_GPIO3_INPUT,
	AXP20X_IRQ_GPIO2_INPUT,
	AXP20X_IRQ_GPIO1_INPUT,
	AXP20X_IRQ_GPIO0_INPUT,
};

enum axp22x_irqs {
	AXP22X_IRQ_ACIN_OVER_V = 1,
	AXP22X_IRQ_ACIN_PLUGIN,
	AXP22X_IRQ_ACIN_REMOVAL,
	AXP22X_IRQ_VBUS_OVER_V,
	AXP22X_IRQ_VBUS_PLUGIN,
	AXP22X_IRQ_VBUS_REMOVAL,
	AXP22X_IRQ_VBUS_V_LOW,
	AXP22X_IRQ_BATT_PLUGIN,
	AXP22X_IRQ_BATT_REMOVAL,
	AXP22X_IRQ_BATT_ENT_ACT_MODE,
	AXP22X_IRQ_BATT_EXIT_ACT_MODE,
	AXP22X_IRQ_CHARG,
	AXP22X_IRQ_CHARG_DONE,
	AXP22X_IRQ_BATT_TEMP_HIGH,
	AXP22X_IRQ_BATT_TEMP_LOW,
	AXP22X_IRQ_DIE_TEMP_HIGH,
	AXP22X_IRQ_PEK_SHORT,
	AXP22X_IRQ_PEK_LONG,
	AXP22X_IRQ_LOW_PWR_LVL1,
	AXP22X_IRQ_LOW_PWR_LVL2,
	AXP22X_IRQ_TIMER,
	AXP22X_IRQ_PEK_RIS_EDGE,
	AXP22X_IRQ_PEK_FAL_EDGE,
	AXP22X_IRQ_GPIO1_INPUT,
	AXP22X_IRQ_GPIO0_INPUT,
};

enum axp288_irqs {
	AXP288_IRQ_VBUS_FALL     = 2,
	AXP288_IRQ_VBUS_RISE,
	AXP288_IRQ_OV,
	AXP288_IRQ_FALLING_ALT,
	AXP288_IRQ_RISING_ALT,
	AXP288_IRQ_OV_ALT,
	AXP288_IRQ_DONE          = 10,
	AXP288_IRQ_CHARGING,
	AXP288_IRQ_SAFE_QUIT,
	AXP288_IRQ_SAFE_ENTER,
	AXP288_IRQ_ABSENT,
	AXP288_IRQ_APPEND,
	AXP288_IRQ_QWBTU,
	AXP288_IRQ_WBTU,
	AXP288_IRQ_QWBTO,
	AXP288_IRQ_WBTO,
	AXP288_IRQ_QCBTU,
	AXP288_IRQ_CBTU,
	AXP288_IRQ_QCBTO,
	AXP288_IRQ_CBTO,
	AXP288_IRQ_WL2,
	AXP288_IRQ_WL1,
	AXP288_IRQ_GPADC,
	AXP288_IRQ_OT            = 31,
	AXP288_IRQ_GPIO0,
	AXP288_IRQ_GPIO1,
	AXP288_IRQ_POKO,
	AXP288_IRQ_POKL,
	AXP288_IRQ_POKS,
	AXP288_IRQ_POKN,
	AXP288_IRQ_POKP,
	AXP288_IRQ_TIMER,
	AXP288_IRQ_MV_CHNG,
	AXP288_IRQ_BC_USB_CHNG,
};

enum axp806_irqs {
	AXP806_IRQ_DIE_TEMP_HIGH_LV1,
	AXP806_IRQ_DIE_TEMP_HIGH_LV2,
	AXP806_IRQ_DCDCA_V_LOW = 3,
	AXP806_IRQ_DCDCB_V_LOW,
	AXP806_IRQ_DCDCC_V_LOW,
	AXP806_IRQ_DCDCD_V_LOW,
	AXP806_IRQ_DCDCE_V_LOW,
	AXP806_IRQ_PWROK_LONG,
	AXP806_IRQ_PWROK_SHORT,
	AXP806_IRQ_WAKEUP = 12,
	AXP806_IRQ_PWROK_FALL,
	AXP806_IRQ_PWROK_RISE,
};

enum axp809_irqs {
	AXP809_IRQ_ACIN_OVER_V = 1,
	AXP809_IRQ_ACIN_PLUGIN,
	AXP809_IRQ_ACIN_REMOVAL,
	AXP809_IRQ_VBUS_OVER_V,
	AXP809_IRQ_VBUS_PLUGIN,
	AXP809_IRQ_VBUS_REMOVAL,
	AXP809_IRQ_VBUS_V_LOW,
	AXP809_IRQ_BATT_PLUGIN,
	AXP809_IRQ_BATT_REMOVAL,
	AXP809_IRQ_BATT_ENT_ACT_MODE,
	AXP809_IRQ_BATT_EXIT_ACT_MODE,
	AXP809_IRQ_CHARG,
	AXP809_IRQ_CHARG_DONE,
	AXP809_IRQ_BATT_CHG_TEMP_HIGH,
	AXP809_IRQ_BATT_CHG_TEMP_HIGH_END,
	AXP809_IRQ_BATT_CHG_TEMP_LOW,
	AXP809_IRQ_BATT_CHG_TEMP_LOW_END,
	AXP809_IRQ_BATT_ACT_TEMP_HIGH,
	AXP809_IRQ_BATT_ACT_TEMP_HIGH_END,
	AXP809_IRQ_BATT_ACT_TEMP_LOW,
	AXP809_IRQ_BATT_ACT_TEMP_LOW_END,
	AXP809_IRQ_DIE_TEMP_HIGH,
	AXP809_IRQ_LOW_PWR_LVL1,
	AXP809_IRQ_LOW_PWR_LVL2,
	AXP809_IRQ_TIMER,
	AXP809_IRQ_PEK_RIS_EDGE,
	AXP809_IRQ_PEK_FAL_EDGE,
	AXP809_IRQ_PEK_SHORT,
	AXP809_IRQ_PEK_LONG,
	AXP809_IRQ_PEK_OVER_OFF,
	AXP809_IRQ_GPIO1_INPUT,
	AXP809_IRQ_GPIO0_INPUT,
};

enum axp2101_irqs {
	/* irq0 */
	AXP2101_IRQ_BWUT,
	AXP2101_IRQ_BWOT,
	AXP2101_IRQ_BCUT,
	AXP2101_IRQ_BCOT,
	AXP2101_IRQ_NEWSOC,
	AXP2101_IRQ_GWDT,
	AXP2101_IRQ_SOCWL1,
	AXP2101_IRQ_SOCWL2,
	/* irq1 */
	AXP2101_IRQ_PONP,
	AXP2101_IRQ_PONN,
	AXP2101_IRQ_PONL,
	AXP2101_IRQ_PONS,
	AXP2101_IRQ_BREMOV,
	AXP2101_IRQ_BINSERT,
	AXP2101_IRQ_VREMOV,
	AXP2101_IRQ_VINSET,
	/* irq2 */
	AXP2101_IRQ_BOVP,
	AXP2101_IRQ_CHGTE,
	AXP2101_IRQ_DOTL1,
	AXP2101_IRQ_CHGST,
	AXP2101_IRQ_CHGDN,
	AXP2101_IRQ_BOCP,
	AXP2101_IRQ_LDOOC,
	AXP2101_IRQ_WDEXP,
};

enum axp15_irqs {
	/* irq0 */
	AXP15_IRQ_ALDOIN_H2L = 2,
	AXP15_IRQ_ALDOIN_L2H,
	AXP15_IRQ_LDO0IN_H2L = 5,
	AXP15_IRQ_LDO0IN_L2H,
	/* irq1 */
	AXP15_IRQ_PEKLO = 8,
	AXP15_IRQ_PEKSH,
	AXP15_IRQ_DCDC4_V_LOW,
	AXP15_IRQ_DCDC3_V_LOW,
	AXP15_IRQ_DCDC2_V_LOW,
	AXP15_IRQ_DCDC1_V_LOW,

	/* irq2 */
	AXP15_IRQ_GPIO0 = 16,
	AXP15_IRQ_GPIO1,
	AXP15_IRQ_GPIO2,
	AXP15_IRQ_GPIO3,
	AXP15_IRQ_PEKFE = 21,
	AXP15_IRQ_PEKRE,
	AXP15_IRQ_EVENT_TIMEOUT,
};

enum axp1530_irqs {
	/* irq0 */
	AXP1530_IRQ_TEMP_OVER,
	AXP1530_IRQ_DCDC2_UNDER = 2,
	AXP1530_IRQ_DCDC3_UNDER,
	AXP1530_IRQ_POKLIRQ_EN,
	AXP1530_IRQ_POKSIRQ_EN,
	AXP1530_IRQ_KEY_L2H_EN,
	AXP1530_IRQ_KEY_H2L_EN,
};

enum axp858_irqs {
	/* irq0 */
	AXP858_IRQ_TEMP_OVER1 = 0,
	AXP858_IRQ_TEMP_OVER2,
	AXP858_IRQ_DCDC1_UNDER,
	AXP858_IRQ_DCDC2_UNDER,
	AXP858_IRQ_DCDC3_UNDER,
	AXP858_IRQ_DCDC4_UNDER,
	AXP858_IRQ_DCDC5_UNDER,
	AXP858_IRQ_DCDC6_UNDER,
	/* irq1 */
	AXP858_IRQ_POKLIRQ_EN,
	AXP858_IRQ_POKSIRQ_EN,
	AXP858_IRQ_GPIO1_EN,
	AXP858_IRQ_POKNIRQ_EN,
	AXP858_IRQ_POKPIRQ_EN,
	AXP858_IRQ_GPIO2_EN,
	AXP858_IRQ_DCDC2_CUR_OVER,
	AXP858_IRQ_DCDC3_CUR_OVER,
};

enum axp803_irqs {
	/* irq0 */
	AXP803_IRQ_USBRE = 2,
	AXP803_IRQ_USBIN,
	AXP803_IRQ_USBOV,
	AXP803_IRQ_ACRE,
	AXP803_IRQ_ACIN,
	AXP803_IRQ_ACOV,
	/* irq1 */
	AXP803_IRQ_CHAOV = 10,
	AXP803_IRQ_CHAST,
	AXP803_IRQ_BATATOU,
	AXP803_IRQ_BATATIN,
	AXP803_IRQ_BATRE,
	AXP803_IRQ_BATIN,
	/* irq2 */
	AXP803_IRQ_QBATINWORK,
	AXP803_IRQ_BATINWORK,
	AXP803_IRQ_QBATOVWORK,
	AXP803_IRQ_BATOVWORK,
	AXP803_IRQ_QBATINCHG,
	AXP803_IRQ_BATINCHG,
	AXP803_IRQ_QBATOVCHG,
	AXP803_IRQ_BATOVCHG,
	/* irq3 */
	AXP803_IRQ_LOWN2,
	AXP803_IRQ_LOWN1,
	/* irq4 */
	AXP803_IRQ_GPIO0 = 32,
	AXP803_IRQ_GPIO1,
	AXP803_IRQ_POKLO = 35,
	AXP803_IRQ_POKSH,
	AXP803_IRQ_PEKFE,
	AXP803_IRQ_PEKRE,
	AXP803_IRQ_TIMER,
};

enum {
	AXP2202_DCDC1 = 0,
	AXP2202_DCDC2,
	AXP2202_DCDC3,
	AXP2202_DCDC4,
	AXP2202_ALDO1,
	AXP2202_ALDO2,
	AXP2202_ALDO3,
	AXP2202_ALDO4,
	AXP2202_BLDO1,
	AXP2202_BLDO2,
	AXP2202_BLDO3,
	AXP2202_BLDO4,
	AXP2202_CLDO1,
	AXP2202_CLDO2,
	AXP2202_CLDO3,
	AXP2202_CLDO4,
	AXP2202_RTCLDO,
	AXP2202_CPUSLDO,
	AXP2202_REG_ID_MAX,
	AXP2202_VBUS,
};

enum axp2202_irqs {
	/* irq0 */
	AXP2202_IRQ_SOCWL2,
	AXP2202_IRQ_SOCWL1,
	AXP2202_IRQ_GWDT,
	AXP2202_IRQ_NEWSOC,
	AXP2202_IRQ_BST_OV,
	AXP2202_IRQ_VBUS_OV,
	AXP2202_IRQ_VBUS_FAULT,
	/* irq1 */
	AXP2202_IRQ_VINSERT,
	AXP2202_IRQ_VREMOVE,
	AXP2202_IRQ_BINSERT,
	AXP2202_IRQ_BREMOVE,
	AXP2202_IRQ_PONS,
	AXP2202_IRQ_PONL,
	AXP2202_IRQ_PONN,
	AXP2202_IRQ_PONP,
	/* irq2 */
	AXP2202_IRQ_WDEXP,
	AXP2202_IRQ_LDOOC,
	AXP2202_IRQ_BOCP,
	AXP2202_IRQ_CHGDN,
	AXP2202_IRQ_CHGST,
	AXP2202_IRQ_DOTL1,
	AXP2202_IRQ_CHGTE,
	AXP2202_IRQ_BOVP,
	/* irq3 */
	AXP2202_IRQ_BC_DONE,
	AXP2202_IRQ_BC_CHNG,
	AXP2202_IRQ_RID_CHNG,
	AXP2202_IRQ_BCOTQ,
	AXP2202_IRQ_BCOT,
	AXP2202_IRQ_BCUT,
	AXP2202_IRQ_BWOT,
	AXP2202_IRQ_BWUT,
	/* irq4 */
	AXP2202_IRQ_CREMOVE,
	AXP2202_IRQ_CINSERT,
	AXP2202_IRQ_TOGGLE_DONE,
	AXP2202_IRQ_VBUS_SAFE5V,
	AXP2202_IRQ_VBUS_SAFE0V,
	AXP2202_IRQ_ERR_GEN,
	AXP2202_IRQ_PWR_CHNG,

};

enum axp2585_irqs {
	AXP2585_IRQ_Q_DROP2,  //7l2
	AXP2585_IRQ_Q_DROP1,  //6l1
	AXP2585_IRQ_Q_CHANGE,
	AXP2585_IRQ_Q_GOOD,
	AXP2585_IRQ_BAT_DECT,
	AXP2585_IRQ_BOOST_OVP,
	AXP2585_IRQ_BOOST_OCP,
	AXP2585_IRQ_BAT_OCP,
	AXP2585_IRQ_BCOT,       //15
	AXP2585_IRQ_QBCOT,      //14
	AXP2585_IRQ_BCUT,       //13
	AXP2585_IRQ_QBCUT,
	AXP2585_IRQ_BWOT,       //11
	AXP2585_IRQ_QBWOT,
	AXP2585_IRQ_BWUT,       //9
	AXP2585_IRQ_QBWUT,
	AXP2585_IRQ_VBUS_INSERT,  //23ac
	AXP2585_IRQ_VBUS_REMOVE,  //22ac
	AXP2585_IRQ_BAT_INSERT,  //21
	AXP2585_IRQ_BAT_REMOVE,  //20
	AXP2585_IRQ_BAT_DB2GD,
	AXP2585_IRQ_TJ_OTP,
	AXP2585_IRQ_BAT_SMODE,
	AXP2585_IRQ_VBUS_OVP,
	AXP2585_IRQ_SIRQ,
	AXP2585_IRQ_LIRQ,
	AXP2585_IRQ_NIRQ,       //29
	AXP2585_IRQ_PIRQ,       //28
	AXP2585_IRQ_GPADC_BWOT,
	AXP2585_IRQ_GPADC_QBWOT,
	AXP2585_IRQ_GPADC_BWUT,
	AXP2585_IRQ_GPADC_QBWUT,
	AXP2585_IRQ_CHGBG,     //39
	AXP2585_IRQ_CHGDONE,   //38
	AXP2585_IRQ_BC_OK,
	AXP2585_IRQ_BC_CHANGE,
	AXP2585_IRQ_RID_CHANGE,
	AXP2585_IRQ_BAT_OVP,
	AXP2585_IRQ_REMOVE,  //47tc
	AXP2585_IRQ_INSERT,  //46tc
	AXP2585_IRQ_TOGGLE_DONE,
	AXP2585_IRQ_VBUS_SAFE5V,
	AXP2585_IRQ_ERROR_GEN,
	AXP2585_IRQ_POW_CHNG,
};
#define AXP288_TS_ADC_H		0x58
#define AXP288_TS_ADC_L		0x59
#define AXP288_GP_ADC_H		0x5a
#define AXP288_GP_ADC_L		0x5b

struct axp20x_dev {
	struct device			*dev;
	int				irq;
	struct regmap			*regmap;
	struct regmap_irq_chip_data	*regmap_irqc;
	long				variant;
	int                             nr_cells;
	struct mfd_cell                 *cells;
	const struct regmap_config	*regmap_cfg;
	const struct regmap_irq_chip	*regmap_irq_chip;
	void (*dts_parse)(struct axp20x_dev *);
};

#define BATTID_LEN				64
#define OCV_CURVE_SIZE			32
#define MAX_THERM_CURVE_SIZE	25
#define PD_DEF_MIN_TEMP			0
#define PD_DEF_MAX_TEMP			55

struct axp20x_fg_pdata {
	char battid[BATTID_LEN + 1];
	int design_cap;
	int min_volt;
	int max_volt;
	int max_temp;
	int min_temp;
	int cap1;
	int cap0;
	int rdc1;
	int rdc0;
	int ocv_curve[OCV_CURVE_SIZE];
	int tcsz;
	int thermistor_curve[MAX_THERM_CURVE_SIZE][2];
};

struct axp20x_chrg_pdata {
	int max_cc;
	int max_cv;
	int def_cc;
	int def_cv;
};

struct axp288_extcon_pdata {
	/* GPIO pin control to switch D+/D- lines b/w PMIC and SOC */
	struct gpio_desc *gpio_mux_cntl;
};

/* generic helper function for reading 9-16 bit wide regs */
static inline int axp20x_read_variable_width(struct regmap *regmap,
	unsigned int reg, unsigned int width)
{
	unsigned int reg_val, result;
	int err;

	err = regmap_read(regmap, reg, &reg_val);
	if (err)
		return err;

	result = reg_val << (width - 8);

	err = regmap_read(regmap, reg + 1, &reg_val);
	if (err)
		return err;

	result |= reg_val;

	return result;
}

/**
 * axp20x_match_device(): Setup axp20x variant related fields
 *
 * @axp20x: axp20x device to setup (.dev field must be set)
 * @dev: device associated with this axp20x device
 *
 * This lets the axp20x core configure the mfd cells and register maps
 * for later use.
 */
int axp20x_match_device(struct axp20x_dev *axp20x);

/**
 * axp20x_device_probe(): Probe a configured axp20x device
 *
 * @axp20x: axp20x device to probe (must be configured)
 *
 * This function lets the axp20x core register the axp20x mfd devices
 * and irqchip. The axp20x device passed in must be fully configured
 * with axp20x_match_device, its irq set, and regmap created.
 */
int axp20x_device_probe(struct axp20x_dev *axp20x);

/**
 * axp20x_device_probe(): Remove a axp20x device
 *
 * @axp20x: axp20x device to remove
 *
 * This tells the axp20x core to remove the associated mfd devices
 */
int axp20x_device_remove(struct axp20x_dev *axp20x);

#endif /* __LINUX_MFD_AXP20X_H */
