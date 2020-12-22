#ifndef __LINUX_AXP_MFD_20_H_
#define __LINUX_AXP_MFD_20_H_
#include <linux/mfd/axp-mfd-20.h>

/* Unified sub device IDs for AXP */
/* LDO0 For RTCLDO ,LDO1-3 for ALDO,LDO*/
enum {///qin
	AXP20_ID_DCDC2,
	AXP20_ID_DCDC3,
	
	AXP20_ID_LDO1,
	AXP20_ID_LDO2,
	AXP20_ID_LDO3,
	AXP20_ID_LDO4,

	AXP20_ID_LDOIO0,

	AXP20_ID_SUPPLY,

	AXP20_ID_GPIO,
};

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

//axp 20 adc data register
#define POWER20_BAT_AVERVOL_H8          (0x78)
#define POWER20_BAT_AVERVOL_L4          (0x79)
#define POWER20_BAT_AVERCHGCUR_H8       (0x7A)
#define POWER20_BAT_AVERCHGCUR_L5       (0x7B)
#define POWER20_ACIN_VOL_H8             (0x56)
#define POWER20_ACIN_VOL_L4             (0x57)
#define POWER20_ACIN_CUR_H8             (0x58)
#define POWER20_ACIN_CUR_L4             (0x59)
#define POWER20_VBUS_VOL_H8             (0x5A)
#define POWER20_VBUS_VOL_L4             (0x5B)
#define POWER20_VBUS_CUR_H8             (0x5C)
#define POWER20_VBUS_CUR_L4             (0x5D)

#define POWER20_BAT_AVERDISCHGCUR_H8    (0x7C)
#define POWER20_BAT_AVERDISCHGCUR_L5    (0x7D)
#define POWER20_APS_AVERVOL_H8          (0x7E)
#define POWER20_APS_AVERVOL_L4          (0x7F)
#define POWER20_BAT_CHGCOULOMB3         (0xB0)
#define POWER20_BAT_CHGCOULOMB2         (0xB1)
#define POWER20_BAT_CHGCOULOMB1         (0xB2)
#define POWER20_BAT_CHGCOULOMB0         (0xB3)
#define POWER20_BAT_DISCHGCOULOMB3      (0xB4)
#define POWER20_BAT_DISCHGCOULOMB2      (0xB5)
#define POWER20_BAT_DISCHGCOULOMB1      (0xB6)
#define POWER20_BAT_DISCHGCOULOMB0      (0xB7)
#define POWER20_COULOMB_CTL             (0xB8)
#define POWER20_BAT_POWERH8             (0x70)
#define POWER20_BAT_POWERM8             (0x71)
#define POWER20_BAT_POWERL8             (0x72)

/*  AXP20  */
#define	AXP20_IRQ_USBLO		( 1 <<  1)
#define	AXP20_IRQ_USBRE		( 1 <<  2)
#define	AXP20_IRQ_USBIN		( 1 <<  3)
#define	AXP20_IRQ_USBOV     ( 1 <<  4)
#define	AXP20_IRQ_ACRE     ( 1 <<  5)
#define	AXP20_IRQ_ACIN     ( 1 <<  6)
#define	AXP20_IRQ_ACOV     ( 1 <<  7)
#define	AXP20_IRQ_TEMLO      ( 1 <<  8)
#define	AXP20_IRQ_TEMOV      ( 1 <<  9)
#define	AXP20_IRQ_CHAOV		( 1 << 10)
#define	AXP20_IRQ_CHAST 	    ( 1 << 11)
#define	AXP20_IRQ_BATATOU    ( 1 << 12)
#define	AXP20_IRQ_BATATIN  	( 1 << 13)
#define AXP20_IRQ_BATRE		( 1 << 14)
#define AXP20_IRQ_BATIN		( 1 << 15)
#define	AXP20_IRQ_PEKLO		( 1 << 16)
#define	AXP20_IRQ_PEKSH	    ( 1 << 17)

#define AXP20_IRQ_DCDC3LO    ( 1 << 19)
#define AXP20_IRQ_DCDC2LO    ( 1 << 20)
#define AXP20_IRQ_DCDC1LO    ( 1 << 21)
#define AXP20_IRQ_CHACURLO   ( 1 << 22)
#define AXP20_IRQ_ICTEMOV    ( 1 << 23)
#define AXP20_IRQ_EXTLOWARN1  ( 1 << 24)
#define AXP20_IRQ_EXTLOWARN2  ( 1 << 25)
#define AXP20_IRQ_USBSESUN  ( 1 << 26)
#define AXP20_IRQ_USBSESVA  ( 1 << 27)
#define AXP20_IRQ_USBUN     ( 1 << 28)
#define AXP20_IRQ_USBVA     ( 1 << 29)
#define AXP20_IRQ_NOECLO     ( 1 << 30)
#define AXP20_IRQ_NOEOPE     ( 1 << 31)
#define AXP20_IRQ_GPIO0TG     ((uint64_t)1 << 32)
#define AXP20_IRQ_GPIO1TG     ((uint64_t)1 << 33)
#define AXP20_IRQ_GPIO2TG     ((uint64_t)1 << 34)
#define AXP20_IRQ_GPIO3TG     ((uint64_t)1 << 35)

#define AXP20_IRQ_PEKFE     ((uint64_t)1 << 37)
#define AXP20_IRQ_PEKRE     ((uint64_t)1 << 38)
#define AXP20_IRQ_TIMER     ((uint64_t)1 << 39)

/*  AXP20  */
#define AXP20_STATUS_SOURCE    ( 1 <<  0)
#define AXP20_STATUS_ACUSBSH ( 1 <<  1)
#define AXP20_STATUS_BATCURDIR ( 1 <<  2)
#define AXP20_STATUS_USBLAVHO ( 1 <<  3)
#define AXP20_STATUS_USBVA    ( 1 <<  4)
#define AXP20_STATUS_USBEN    ( 1 <<  5)
#define AXP20_STATUS_ACVA	    ( 1 <<  6)
#define AXP20_STATUS_ACEN	    ( 1 <<  7)


#define AXP20_STATUS_CHACURLOEXP (1 << 10)
#define AXP20_STATUS_BATINACT  ( 1 << 11)

#define AXP20_STATUS_BATEN     ( 1 << 13)
#define AXP20_STATUS_INCHAR    ( 1 << 14)
#define AXP20_STATUS_ICTEMOV   ( 1 << 15)


#endif /* __LINUX_AXP_MFD_23_H_ */

