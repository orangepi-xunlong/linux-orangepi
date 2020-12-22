#ifndef __LINUX_AXP_MFD_15_H_
#define __LINUX_AXP_MFD_15_H_

/* Unified sub device IDs for AXP */
/* LDO0 For RTCLDO ,LDO1-3 for ALDO,LDO*/
enum {
	AXP15_ID_DCDC1,
	AXP15_ID_DCDC2,
	AXP15_ID_DCDC3,
	AXP15_ID_DCDC4,
	AXP15_ID_DCDC5,
	AXP15_ID_LDO1,   //ALDO1
	AXP15_ID_LDO2,   //ALDO2
	AXP15_ID_LDO3,   //ALDO3
	AXP15_ID_LDO4,   //BLDO1
	AXP15_ID_LDO5,   //BLDO2
	AXP15_ID_LDO6,   //BLDO3
	AXP15_ID_LDO7,   //BLDO4
	AXP15_ID_LDO8,   //CLDO1
	AXP15_ID_LDO9,   //CLDO2
	AXP15_ID_LDO10,  //CLDO3
	AXP15_ID_SW0,
	AXP15_ID_SUPPLY,
};

/*For AXP15*/ 
#define AXP15                     (15)
#define AXP15_STATUS              (0x00)
#define AXP15_IC_TYPE		  (0x03)
#define AXP15_BUFFER1             (0x04)
#define AXP15_BUFFER2             (0x05)
#define AXP15_BUFFER3             (0x06)
#define AXP15_BUFFER4             (0x07)
#define AXP15_VOFF_SET            (0x31)
#define AXP15_PR_DN_SEQU     	  (0x32)
#define AXP15_WAKEUP_FUNC         (0x35)
#define AXP15_POK_SET             (0x36)
#define AXP15_INFACE_MODE         (0x3E)
#define AXP15_SPECIAL_REG         (0x3F)
#define AXP15_INTEN1              (0x40)
#define AXP15_INTEN2              (0x41)
#define AXP15_INTSTS1             (0x48)
#define AXP15_INTSTS2             (0x49)

#define AXP15_DC_ALDO_EN          (0X10)
#define AXP15_BC_LDO_EN           (0X11)
#define AXP15_DCA_VOL	          (0X12)
#define AXP15_DCB_VOL 	          (0X13)
#define AXP15_DCC_VOL	          (0x14)
#define AXP15_DCD_VOL	          (0x15)
#define AXP15_DCE_VOL  	          (0x16)
#define AXP15_ALDO1_VOL	          (0x17)
#define AXP15_ALDO2_VOL           (0x18)
#define AXP15_ALDO3_VOL           (0x19)
#define AXP15_DC_MODE1            (0x1A)
#define AXP15_DC_MODE2            (0x1B)
#define AXP15_DC_FREQ	          (0x1C)
#define AXP15_OUTPUT_MONI         (0x1D)
#define AXP15_PWROK_SET           (0x1F)
#define AXP15_BLDO1_VOL           (0x20)
#define AXP15_BLDO2_VOL           (0x21)
#define AXP15_BLDO3_VOL           (0x22)
#define AXP15_BLDO4_VOL           (0x23)
#define AXP15_CLDO1_VOL           (0x24)
#define AXP15_CLDO2_VOL           (0x25)
#define AXP15_CLDO3_VOL           (0x26)

/* bit definitions for AXP events ,irq event */
/*  AXP15  */
#define	AXP15_IRQ_TEMOVWARN1		( 1 <<  0)
#define	AXP15_IRQ_TEMOVWARN2		( 1 <<  1)

#define	AXP15_IRQ_DCALVOL     		( 1 <<  3)
#define	AXP15_IRQ_DCBLVOL     		( 1 <<  4)
#define	AXP15_IRQ_DCCLVOL     		( 1 <<  5)
#define	AXP15_IRQ_DCDLVOL     		( 1 <<  6)
#define	AXP15_IRQ_DCELVOL      		( 1 <<  7)
#define	AXP15_IRQ_POKLO      		( 1 <<  8)
#define	AXP15_IRQ_POKSH			( 1 << 9)


#define	AXP15_IRQ_WAKEUP		( 1 << 12)
#define	AXP15_IRQ_POKN  		( 1 << 13)
#define AXP15_IRQ_POKP			( 1 << 14)


/* Status Query Interface */
/*  AXP15  */
#define AXP15_STATUS_ALDOIN    		( 1 <<  0)
#define AXP15_STATUS_SPESEQ 		( 1 <<  1)
#define AXP15_STATUS_PWRON 		( 1 <<  2)
#define AXP15_STATUS_IRQ 		( 1 <<  3)
#define AXP15_STATUS_EN    		( 1 <<  4)
#define AXP15_STATUS_ALDOINGOOD		( 1 <<  5)
#define AXP15_STATUS_CHIPMODE1	   	( 1 <<  6)
#define AXP15_STATUS_CHIPMODE2	    	( 1 <<  7)

#endif /* __LINUX_AXP_MFD_15_H_ */
