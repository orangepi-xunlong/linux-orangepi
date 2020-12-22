#ifndef __LINUX_AXP_MFD_22_H_
#define __LINUX_AXP_MFD_22_H_

/* Unified sub device IDs for AXP */
/* LDO0 For RTCLDO ,LDO1-3 for ALDO,LDO*/
enum {
	AXP22_ID_DCDC1,
	AXP22_ID_DCDC2,
	AXP22_ID_DCDC3,
	AXP22_ID_DCDC4,
	AXP22_ID_DCDC5,
	AXP22_ID_LDO1,   //RTCLDO
	AXP22_ID_LDO2,   //ALDO1
	AXP22_ID_LDO3,   //ALDO2
	AXP22_ID_LDO4,   //ALDO3
	AXP22_ID_LDO5,   //DLDO1
	AXP22_ID_LDO6,   //DLDO2
	AXP22_ID_LDO7,   //DLDO3
	AXP22_ID_LDO8,   //DLDO4
	AXP22_ID_LDO9,   //ELDO1
	AXP22_ID_LDO10,  //ELDO2
	AXP22_ID_LDO11,  //ELDO3
	AXP22_ID_LDO12,  //DC5LDO
	AXP22_ID_LDOIO0,
	AXP22_ID_LDOIO1,
#ifdef CONFIG_AXP809
	AXP22_ID_SW0,
#endif
	AXP22_ID_SW1,
	AXP22_ID_SUPPLY,
	AXP22_ID_GPIO,	
};

/*For AXP22*/ 
#define AXP22                     (22)
#define AXP22_STATUS              (0x00)
#define AXP22_MODE_CHGSTATUS      (0x01)
#define AXP22_IC_TYPE		  (0x03)
#define AXP22_BUFFER1             (0x04)
#define AXP22_BUFFER2             (0x05)
#define AXP22_BUFFER3             (0x06)
#define AXP22_BUFFER4             (0x07)
#define AXP22_BUFFER5             (0x08)
#define AXP22_BUFFER6             (0x09)
#define AXP22_BUFFER7             (0x0A)
#define AXP22_BUFFER8             (0x0B)
#define AXP22_BUFFER9             (0x0C)
#define AXP22_BUFFERA             (0x0D)
#define AXP22_BUFFERB             (0x0E)
#define AXP22_BUFFERC             (0x0F)
#define AXP22_IPS_SET             (0x30)
#define AXP22_VOFF_SET            (0x31)
#define AXP22_OFF_CTL             (0x32)
#define AXP22_CHARGE1             (0x33)
#define AXP22_CHARGE2             (0x34)
#define AXP22_CHARGE3             (0x35)
#define AXP22_POK_SET             (0x36)
#define AXP22_INTEN1              (0x40)
#define AXP22_INTEN2              (0x41)
#define AXP22_INTEN3              (0x42)
#define AXP22_INTEN4              (0x43)
#define AXP22_INTEN5              (0x44)
#define AXP22_INTSTS1             (0x48)
#define AXP22_INTSTS2             (0x49)
#define AXP22_INTSTS3             (0x4A)
#define AXP22_INTSTS4             (0x4B)
#define AXP22_INTSTS5             (0x4C)

#define AXP22_LDO_DC_EN1          (0X10)
#define AXP22_LDO_DC_EN2          (0X12)
#define AXP22_LDO_DC_EN3          (0X13)
#define AXP22_DLDO1OUT_VOL        (0x15)
#define AXP22_DLDO2OUT_VOL        (0x16)
#define AXP22_DLDO3OUT_VOL        (0x17)
#define AXP22_DLDO4OUT_VOL        (0x18)
#define AXP22_ELDO1OUT_VOL        (0x19)
#define AXP22_ELDO2OUT_VOL        (0x1A)
#define AXP22_ELDO3OUT_VOL        (0x1B)
#define AXP22_DC5LDOOUT_VOL       (0x1C)
#define AXP22_DC1OUT_VOL          (0x21)
#define AXP22_DC2OUT_VOL          (0x22)
#define AXP22_DC3OUT_VOL          (0x23)
#define AXP22_DC4OUT_VOL          (0x24)
#define AXP22_DC5OUT_VOL          (0x25)
#define AXP22_GPIO0LDOOUT_VOL     (0x91)
#define AXP22_GPIO1LDOOUT_VOL     (0x93)
#define AXP22_ALDO1OUT_VOL        (0x28)
#define AXP22_ALDO2OUT_VOL        (0x29)
#define AXP22_ALDO3OUT_VOL        (0x2A)

#define AXP22_DCDC_MODESET        (0x80)
#ifdef CONFIG_AXP809
#define AXP22_POK_DELAY_SET       (0x37)
#define AXP22_DCDC_FREQSET        (0x3B)
#define AXP22_DCDC_MONITOR        (0x81)
#else
#define AXP22_DCDC_FREQSET        (0x37)
#endif
#define AXP22_ADC_EN              (0x82)
#define AXP22_HOTOVER_CTL         (0x8F)

#define AXP22_GPIO0_CTL           (0x90)
#define AXP22_GPIO1_CTL           (0x92)
#define AXP22_GPIO01_SIGNAL       (0x94)
#define AXP22_BAT_CHGCOULOMB3     (0xB0)
#define AXP22_BAT_CHGCOULOMB2     (0xB1)
#define AXP22_BAT_CHGCOULOMB1     (0xB2)
#define AXP22_BAT_CHGCOULOMB0     (0xB3)
#define AXP22_BAT_DISCHGCOULOMB3  (0xB4)
#define AXP22_BAT_DISCHGCOULOMB2  (0xB5)
#define AXP22_BAT_DISCHGCOULOMB1  (0xB6)
#define AXP22_BAT_DISCHGCOULOMB0  (0xB7)
#define AXP22_COULOMB_CTL         (0xB8)



/* bit definitions for AXP events ,irq event */
/*  AXP22  */
#define	AXP22_IRQ_USBLO		( 1 <<  1)
#define	AXP22_IRQ_USBRE		( 1 <<  2)
#define	AXP22_IRQ_USBIN		( 1 <<  3)
#define	AXP22_IRQ_USBOV     	( 1 <<  4)
#define	AXP22_IRQ_ACRE		( 1 <<  5)
#define	AXP22_IRQ_ACIN		( 1 <<  6)
#define	AXP22_IRQ_ACOV		( 1 <<  7)
#define	AXP22_IRQ_TEMLO      	( 1 <<  8)
#define	AXP22_IRQ_TEMOV      	( 1 <<  9)
#define	AXP22_IRQ_CHAOV		( 1 << 10)
#define	AXP22_IRQ_CHAST		( 1 << 11)
#define	AXP22_IRQ_BATATOU    	( 1 << 12)
#define	AXP22_IRQ_BATATIN	( 1 << 13)
#define AXP22_IRQ_BATRE		( 1 << 14)
#define AXP22_IRQ_BATIN		( 1 << 15)
#ifdef CONFIG_AXP809
#define AXP22_IRQ_QBATINWORK	( 1 << 16)
#define AXP22_IRQ_BATINWORK	( 1 << 17)
#define AXP22_IRQ_QBATOVWORK	( 1 << 18)
#define AXP22_IRQ_BATOVWORK	( 1 << 19)
#define AXP22_IRQ_QBATINCHG	( 1 << 20)
#define AXP22_IRQ_BATINCHG	( 1 << 21)
#define AXP22_IRQ_QBATOVCHG	( 1 << 22)
#define AXP22_IRQ_BATOVCHG	( 1 << 23)
#define AXP22_IRQ_EXTLOWARN2  	( 1 << 24)
#define AXP22_IRQ_EXTLOWARN1  	( 1 << 25)
#define AXP22_IRQ_ICTEMOV    	( 1 << 31)
#define AXP22_IRQ_GPIO0TG     	((uint64_t)1 << 32)
#define AXP22_IRQ_GPIO1TG     	((uint64_t)1 << 33)
#define AXP22_IRQ_POKLO     	((uint64_t)1 << 35)
#define AXP22_IRQ_POKSH     	((uint64_t)1 << 36)
#else
#define	AXP22_IRQ_POKLO		( 1 << 16)
#define	AXP22_IRQ_POKSH	    	( 1 << 17)
#define AXP22_IRQ_ICTEMOV    	( 1 << 23)
#define AXP22_IRQ_EXTLOWARN2  	( 1 << 24)
#define AXP22_IRQ_EXTLOWARN1  	( 1 << 25)
#define AXP22_IRQ_GPIO0TG     	((uint64_t)1 << 32)
#define AXP22_IRQ_GPIO1TG     	((uint64_t)1 << 33)
#define AXP22_IRQ_GPIO2TG     	((uint64_t)1 << 34)
#define AXP22_IRQ_GPIO3TG     	((uint64_t)1 << 35)
#endif
#define AXP22_IRQ_PEKFE     	((uint64_t)1 << 37)
#define AXP22_IRQ_PEKRE     	((uint64_t)1 << 38)
#define AXP22_IRQ_TIMER     	((uint64_t)1 << 39)

/* Status Query Interface */
/*  AXP22  */
#define AXP22_STATUS_SOURCE    	( 1 <<  0)
#define AXP22_STATUS_ACUSBSH 	( 1 <<  1)
#define AXP22_STATUS_BATCURDIR 	( 1 <<  2)
#define AXP22_STATUS_USBLAVHO 	( 1 <<  3)
#define AXP22_STATUS_USBVA    	( 1 <<  4)
#define AXP22_STATUS_USBEN    	( 1 <<  5)
#define AXP22_STATUS_ACVA	( 1 <<  6)
#define AXP22_STATUS_ACEN	( 1 <<  7)

#define AXP22_STATUS_BATINACT  	( 1 << 11)
                               	
#define AXP22_STATUS_BATEN     	( 1 << 13)
#define AXP22_STATUS_INCHAR    	( 1 << 14)
#define AXP22_STATUS_ICTEMOV   	( 1 << 15)


#endif /* __LINUX_AXP_MFD_22_H_ */
