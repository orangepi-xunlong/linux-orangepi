/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : standby_power.h
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-31 14:34
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __AXP209_POWER_H__
#define __AXP209_POWER_H__

#include "axp_power.h"

enum power_vol_type_e{
    AXP20_ID_DCDC2 = 0,
    AXP20_ID_DCDC3,
    AXP20_ID_LDO1,
    AXP20_ID_LDO2,
    AXP20_ID_LDO3,
    AXP20_ID_LDO4,
    AXP20_ID_MAX,
};

#define AXP20_ADDR      (0x34)
#define AXP_IICBUS      (0)
#define AXP20_DCDC_SUM  (0x2)

/*For AXP20*/
#define AXP20                       20
#define POWER20_STATUS              (0x00)
#define POWER20_MODE_CHGSTATUS      (0x01)
#define POWER20_OTG_STATUS          (0x02)
#define POWER20_IC_TYPE             (0x03)
#define POWER20_DATA_BUFFER1        (0x04)
#define POWER20_DATA_BUFFER2        (0x05)
#define POWER20_DATA_BUFFER3        (0x06)
#define POWER20_DATA_BUFFER4        (0x07)
#define POWER20_DATA_BUFFER5        (0x08)
#define POWER20_DATA_BUFFER6        (0x09)
#define POWER20_DATA_BUFFER7        (0x0A)
#define POWER20_DATA_BUFFER8        (0x0B)
#define POWER20_DATA_BUFFER9        (0x0C)
#define POWER20_DATA_BUFFERA        (0x0D)
#define POWER20_DATA_BUFFERB        (0x0E)
#define POWER20_DATA_BUFFERC        (0x0F)
#define POWER20_LDO234_DC23_CTL     (0x12)
#define POWER20_DC2OUT_VOL          (0x23)
#define POWER20_LDO3_DC2_DVM        (0x25)
#define POWER20_DC3OUT_VOL          (0x27)
#define POWER20_LDO24OUT_VOL        (0x28)
#define POWER20_LDO3OUT_VOL         (0x29)
#define POWER20_IPS_SET             (0x30)
#define POWER20_VOFF_SET            (0x31)
#define POWER20_OFF_CTL             (0x32)
#define POWER20_CHARGE1             (0x33)
#define POWER20_CHARGE2             (0x34)
#define POWER20_BACKUP_CHG          (0x35)
#define POWER20_PEK_SET             (0x36)
#define POWER20_DCDC_FREQSET        (0x37)
#define POWER20_VLTF_CHGSET         (0x38)
#define POWER20_VHTF_CHGSET         (0x39)
#define POWER20_APS_WARNING1        (0x3A)
#define POWER20_APS_WARNING2        (0x3B)
#define POWER20_TLTF_DISCHGSET      (0x3C)
#define POWER20_THTF_DISCHGSET      (0x3D)
#define POWER20_DCDC_MODESET        (0x80)
#define POWER20_ADC_EN1             (0x82)
#define POWER20_ADC_EN2             (0x83)
#define POWER20_ADC_SPEED           (0x84)
#define POWER20_ADC_INPUTRANGE      (0x85)
#define POWER20_ADC_IRQ_RETFSET     (0x86)
#define POWER20_ADC_IRQ_FETFSET     (0x87)
#define POWER20_TIMER_CTL           (0x8A)
#define POWER20_VBUS_DET_SRP        (0x8B)
#define POWER20_HOTOVER_CTL         (0x8F)
#define POWER20_GPIO0_CTL           (0x90)
#define POWER20_GPIO0_VOL           (0x91)
#define POWER20_GPIO1_CTL           (0x92)
#define POWER20_GPIO2_CTL           (0x93)
#define POWER20_GPIO012_SIGNAL      (0x94)
#define POWER20_GPIO3_CTL           (0x95)
#define POWER20_INTEN1              (0x40)
#define POWER20_INTEN2              (0x41)
#define POWER20_INTEN3              (0x42)
#define POWER20_INTEN4              (0x43)
#define POWER20_INTEN5              (0x44)
#define POWER20_INTSTS1             (0x48)
#define POWER20_INTSTS2             (0x49)
#define POWER20_INTSTS3             (0x4A)
#define POWER20_INTSTS4             (0x4B)
#define POWER20_INTSTS5             (0x4C)


/* AXP20 Regulator Registers */
#define AXP20_LDO1		POWER20_STATUS
#define AXP20_LDO2		POWER20_LDO24OUT_VOL
#define AXP20_LDO3		POWER20_LDO3OUT_VOL
#define AXP20_LDO4		POWER20_LDO24OUT_VOL
#define AXP20_BUCK2		POWER20_DC2OUT_VOL
#define AXP20_BUCK3		POWER20_DC3OUT_VOL
#define AXP20_LDOIO0	POWER20_GPIO0_VOL

#define AXP20_LDO1EN		POWER20_STATUS
#define AXP20_LDO2EN		POWER20_LDO234_DC23_CTL
#define AXP20_LDO3EN		POWER20_LDO234_DC23_CTL
#define AXP20_LDO4EN		POWER20_LDO234_DC23_CTL
#define AXP20_BUCK2EN		POWER20_LDO234_DC23_CTL
#define AXP20_BUCK3EN		POWER20_LDO234_DC23_CTL
#define AXP20_LDOIOEN		POWER20_GPIO0_CTL


#endif  /* __AXP209_POWER_H__ */
