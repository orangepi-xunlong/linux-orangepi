#ifndef __AXP80_H_
#define __AXP80_H_

/* For AXP806 */
#define AXP80_STARTUP_SOURCE      (0x00)
#define AXP80_IC_TYPE             (0x03)
#define AXP80_DATA_BUFFER1        (0x04)
#define AXP80_DATA_BUFFER2        (0x05)
#define AXP80_DATA_BUFFER3        (0x06)
#define AXP80_DATA_BUFFER4        (0x07)
#define AXP80_ONOFF_CTRL1         (0x10)
#define AXP80_ONOFF_CTRL2         (0x11)
#define AXP80_DCAOUT_VOL          (0x12)
#define AXP80_DCBOUT_VOL          (0x13)
#define AXP80_DCCOUT_VOL          (0x14)
#define AXP80_DCDOUT_VOL          (0x15)
#define AXP80_DCEOUT_VOL          (0x16)
#define AXP80_ALDO1OUT_VOL        (0x17)
#define AXP80_ALDO2OUT_VOL        (0x18)
#define AXP80_ALDO3OUT_VOL        (0x19)
#define AXP80_DCDC_DVM_CTRL       (0x1A)
#define AXP80_DCDC_MODE_CTRL      (0x1B)
#define AXP80_DCDC_FREQSET        (0x1C)
#define AXP80_DCDC_MON_CTRL       (0x1D)
#define AXP80_IFQ_PWROK_SET       (0x1F)
#define AXP80_BLDO1OUT_VOL        (0x20)
#define AXP80_BLDO2OUT_VOL        (0x21)
#define AXP80_BLDO3OUT_VOL        (0x22)
#define AXP80_BLDO4OUT_VOL        (0x23)
#define AXP80_CLDO1OUT_VOL        (0x24)
#define AXP80_CLDO2OUT_VOL        (0x25)
#define AXP80_CLDO3OUT_VOL        (0x26)
#define AXP80_VOFF_SET            (0x31)
#define AXP80_OFF_CTL             (0x32)
#define AXP80_WAKEUP_PIN_CTRL     (0x35)
#define AXP80_POK_SET             (0x36)
#define AXP80_INTERFACE_MODE      (0x3E)
#define AXP80_SPECIAL_CTRL        (0x3F)

#define AXP80_INTEN1              (0x40)
#define AXP80_INTEN2              (0x41)
#define AXP80_INTSTS1             (0x48)
#define AXP80_INTSTS2             (0x49)

/* bit definitions for AXP events ,irq event */
/*  AXP80  */
#define AXP80_IRQ_OVER_WARN1      (0)
#define AXP80_IRQ_OVER_WARN2      (1)
#define AXP80_IRQ_DCDCA_LOW_85P   (3)
#define AXP80_IRQ_DCDCB_LOW_85P   (4)
#define AXP80_IRQ_DCDCC_LOW_85P   (5)
#define AXP80_IRQ_DCDCD_LOW_85P   (6)
#define AXP80_IRQ_DCDCE_LOW_85P   (7)

#define AXP80_IRQ_PEKLO           (8)
#define AXP80_IRQ_PEKSH           (9)
#define AXP80_IRQ_WS_EN_WAKEUPIN  (12)
#define AXP80_IRQ_PEKFE           (13)
#define AXP80_IRQ_PEKRE           (14)

extern s32 axp_debug;
extern struct axp_config_info axp80_config;

#endif /* __AXP80_H_ */
