#ifndef AXP259_H_
#define AXP259_H_

/* For AXP259 */
#define AXP259_STATUS              (0x00)
#define AXP259_IC_TYPE             (0x03)

#define AXP259_CHARGE1             (0x20)
#define AXP259_CHARGE2             (0x21)
#define AXP259_CHARGE3             (0x22)
#define AXP259_CHARGE4             (0x23)
#define AXP259_ADC_EN              (0x24)
#define AXP259_TS_PIN_CONTROL      (0x25)
#define AXP259_POK_SET             (0x27)
#define AXP259_OFF_CTL             (0x28)
#define AXP259_INTEN1              (0x40)
#define AXP259_INTEN2              (0x41)
#define AXP259_INTEN3              (0x42)
#define AXP259_INTEN4              (0x43)
#define AXP259_INTEN5              (0x44)
#define AXP259_INTSTS1             (0x48)
#define AXP259_INTSTS2             (0x49)
#define AXP259_INTSTS3             (0x4A)
#define AXP259_INTSTS4             (0x4B)
#define AXP259_INTSTS5             (0x4C)
#define AXP259_WARNING_LEVEL       (0xE6)
#define AXP259_ADDR_EXTENSION      (0xFF)


/* bit definitions for AXP events ,irq event */
/* AXP259 */
#define AXP259_IRQ_ICTEMOV       (0)
#define AXP259_IRQ_PMOSEN        (1)
#define AXP259_IRQ_BUCKLO        (2)
#define AXP259_IRQ_BUCKHI        (3)
#define AXP259_IRQ_ACRE          (4)
#define AXP259_IRQ_ACIN          (5)
#define AXP259_IRQ_ACOV          (6)
#define AXP259_IRQ_VACIN         (7)
#define AXP259_IRQ_LOWN2         (8)
#define AXP259_IRQ_LOWN1         (9)
#define AXP259_IRQ_CHAOV         (10)
#define AXP259_IRQ_CHAST         (11)
#define AXP259_IRQ_BATSAFE_QUIT  (12)
#define AXP259_IRQ_BATSAFE_ENTER (13)
#define AXP259_IRQ_BATRE         (14)
#define AXP259_IRQ_BATIN         (15)
#define AXP259_IRQ_QBWUT         (16)
#define AXP259_IRQ_BWUT          (17)
#define AXP259_IRQ_QBWOT         (18)
#define AXP259_IRQ_BWOT          (19)
#define AXP259_IRQ_QBCUT         (20)
#define AXP259_IRQ_BCUT          (21)
#define AXP259_IRQ_QBCOT         (22)
#define AXP259_IRQ_BCOT          (23)
#define AXP259_IRQ_GPIO0         (24)
#define AXP259_IRQ_BATCHG        (25)
#define AXP259_IRQ_POKOFF        (26)
#define AXP259_IRQ_POKLO         (27)
#define AXP259_IRQ_POKSH         (28)
#define AXP259_IRQ_PEKFE         (29)
#define AXP259_IRQ_PEKRE         (30)
#define AXP259_IRQ_BUCKOV_6V6    (32)

extern int axp_debug;
extern struct axp_config_info axp259_config;

#endif /* AXP259_H_ */
