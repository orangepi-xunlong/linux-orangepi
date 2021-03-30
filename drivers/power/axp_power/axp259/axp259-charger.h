#ifndef AXP259_CHARGER_H
#define AXP259_CHARGER_H
#include "axp259.h"

/* AXP259 */
#define AXP259_CHARGE_STATUS            AXP259_STATUS
#define AXP259_CHARGE_CONTROL1          AXP259_CHARGE1
#define AXP259_CHARGE_CONTROL2          AXP259_CHARGE2
#define AXP259_CAP                      (0xB9)
#define AXP259_BATCAP0                  (0xe0)
#define AXP259_BATCAP1                  (0xe1)
#define AXP259_RDC0                     (0xba)
#define AXP259_RDC1                     (0xbb)
#define AXP259_VLTF_CHARGE              (0x38)
#define AXP259_VHTF_CHARGE              (0x39)
#define AXP259_VLTF_WORK                (0x3C)
#define AXP259_VHTF_WORK                (0x3D)

#define AXP259_ADC_CONTROL              AXP259_ADC_EN
#define AXP259_ADC_BATVOL_ENABLE        (1 << 7)
#define AXP259_ADC_BATCUR_ENABLE        (1 << 6)
#define AXP259_ADC_DCINVOL_ENABLE       (1 << 5)
#define AXP259_ADC_DCINCUR_ENABLE       (1 << 4)
#define AXP259_ADC_DIETMP_ENABLE        (1 << 3)
#define AXP259_ADC_TSVOL_ENABLE         (1 << 0)

#define AXP259_VBATH_RES                (0x78)
#define AXP259_IBATH_REG                (0x7a)
#define AXP259_DISIBATH_REG             (0x7c)

#define AXP_CHG_ATTR(_name)                     \
{                                               \
	.attr = { .name = #_name, .mode = 0644 },    \
	.show =  _name##_show,                      \
	.store = _name##_store,                     \
}

#endif
