#ifndef AXP22_CHARGER_H
#define AXP22_CHARGER_H
#include "axp22.h"

/* AXP22 */
#define AXP22_CHARGE_STATUS             AXP22_STATUS
#define AXP22_IN_CHARGE                 (1 << 6)
#define AXP22_PDBC                      (0x32)
#define AXP22_CHARGE_CONTROL1           AXP22_CHARGE1
#define AXP22_CHARGER_ENABLE            (1 << 7)
#define AXP22_CHARGE_CONTROL2           AXP22_CHARGE2
#define AXP22_CHARGE_VBUS               AXP22_IPS_SET
#define AXP22_CAP                       (0xB9)
#define AXP22_BATCAP0                   (0xe0)
#define AXP22_BATCAP1                   (0xe1)
#define AXP22_RDC0                      (0xba)
#define AXP22_RDC1                      (0xbb)
#define AXP22_WARNING_LEVEL             (0xe6)
#define AXP22_ADJUST_PARA               (0xe8)
#define AXP22_FAULT_LOG1                AXP22_MODE_CHGSTATUS
#define AXP22_FAULT_LOG_CHA_CUR_LOW     (1 << 2)
#define AXP22_FAULT_LOG_BATINACT        (1 << 3)
#define AXP22_FAULT_LOG_OVER_TEMP       (1 << 7)
#define AXP22_FAULT_LOG2                AXP22_INTSTS2
#define AXP22_FAULT_LOG_COLD            (1 << 0)
#define AXP22_FINISH_CHARGE             (1 << 2)
#define AXP22_COULOMB_CONTROL           AXP22_COULOMB_CTL
#define AXP22_COULOMB_ENABLE            (1 << 7)
#define AXP22_COULOMB_SUSPEND           (1 << 6)
#define AXP22_COULOMB_CLEAR             (1 << 5)

#define AXP22_ADC_CONTROL               AXP22_ADC_EN
#define AXP22_ADC_BATVOL_ENABLE         (1 << 7)
#define AXP22_ADC_BATCUR_ENABLE         (1 << 6)
#define AXP22_ADC_DCINVOL_ENABLE        (1 << 5)
#define AXP22_ADC_DCINCUR_ENABLE        (1 << 4)
#define AXP22_ADC_USBVOL_ENABLE         (1 << 3)
#define AXP22_ADC_USBCUR_ENABLE         (1 << 2)
#define AXP22_ADC_APSVOL_ENABLE         (1 << 1)
#define AXP22_ADC_TSVOL_ENABLE          (1 << 0)
#define AXP22_ADC_INTERTEM_ENABLE       (1 << 7)
#define AXP22_ADC_GPIO0_ENABLE          (1 << 3)
#define AXP22_ADC_GPIO1_ENABLE          (1 << 2)
#define AXP22_ADC_GPIO2_ENABLE          (1 << 1)
#define AXP22_ADC_GPIO3_ENABLE          (1 << 0)
#define AXP22_ADC_CONTROL3              (0x84)
#define AXP22_VBATH_RES                 (0x78)
#define AXP22_IBATH_REG                 (0x7a)
#define AXP22_DISIBATH_REG              (0x7c)
#define AXP22_VTS_RES                   (0x58)
#define AXP22_VBATL_RES                 (0x79)
#define AXP22_OCVBATH_RES               (0xBC)
#define AXP22_OCVBATL_RES               (0xBD)
#define AXP22_INTTEMP                   (0x56)
#define AXP22_DATA_BUFFER0              AXP22_BUFFER1
#define AXP22_DATA_BUFFER1              AXP22_BUFFER2
#define AXP22_DATA_BUFFER2              AXP22_BUFFER3
#define AXP22_DATA_BUFFER3              AXP22_BUFFER4
#define AXP22_DATA_BUFFER4              AXP22_BUFFER5
#define AXP22_DATA_BUFFER5              AXP22_BUFFER6
#define AXP22_DATA_BUFFER6              AXP22_BUFFER7
#define AXP22_DATA_BUFFER7              AXP22_BUFFER8
#define AXP22_DATA_BUFFER8              AXP22_BUFFER9
#define AXP22_DATA_BUFFER9              AXP22_BUFFERA
#define AXP22_DATA_BUFFERA              AXP22_BUFFERB
#define AXP22_DATA_BUFFERB              AXP22_BUFFERC

#define AXP_CHG_ATTR(_name)                     \
{                                               \
	.attr = { .name = #_name, .mode = 0644 },    \
	.show =  _name##_show,                      \
	.store = _name##_store,                     \
}

#define AXP22_VOL_MAX       1       /* capability buffer length */
#define AXP22_TIME_MAX      20
#define AXP22_AVER_MAX      10
#define AXP22_RDC_COUNT     10

#endif
