#ifndef AXP20_SPLY_H
#define AXP20_SPLY_H
#include "axp20.h"

/*      AXP20      */
#define AXP20_CHARGE_STATUS                 AXP20_STATUS
#define AXP20_IN_CHARGE                     (1 << 6)

#define AXP20_CHARGE_CONTROL1               AXP20_CHARGE1
#define AXP20_CHARGER_ENABLE                (1 << 7)
#define AXP20_CHARGE_CONTROL2               AXP20_CHARGE2
#define AXP20_BUCHARGE_CONTROL              AXP20_BACKUP_CHG
#define AXP20_BUCHARGER_ENABLE              (1 << 7)


#define AXP20_FAULT_LOG1                    AXP20_MODE_CHGSTATUS
#define AXP20_FAULT_LOG_CHA_CUR_LOW         (1 << 2)
#define AXP20_FAULT_LOG_BATINACT            (1 << 3)

#define AXP20_FAULT_LOG_OVER_TEMP           (1 << 7)

#define AXP20_FAULT_LOG2                    AXP20_INTSTS2
#define AXP20_FAULT_LOG_COLD                (1 << 0)

#define AXP20_FINISH_CHARGE                 (1 << 2)


#define AXP20_ADC_CONTROL1                  AXP20_ADC_EN1
#define AXP20_ADC_BATVOL_ENABLE             (1 << 7)
#define AXP20_ADC_BATCUR_ENABLE             (1 << 6)
#define AXP20_ADC_DCINVOL_ENABLE            (1 << 5)
#define AXP20_ADC_DCINCUR_ENABLE            (1 << 4)
#define AXP20_ADC_USBVOL_ENABLE             (1 << 3)
#define AXP20_ADC_USBCUR_ENABLE             (1 << 2)
#define AXP20_ADC_APSVOL_ENABLE             (1 << 1)
#define AXP20_ADC_TSVOL_ENABLE              (1 << 0)
#define AXP20_ADC_CONTROL2                  AXP20_ADC_EN2
#define AXP20_ADC_INTERTEM_ENABLE           (1 << 7)

#define AXP20_ADC_GPIO0_ENABLE              (1 << 3)
#define AXP20_ADC_GPIO1_ENABLE              (1 << 2)
#define AXP20_ADC_GPIO2_ENABLE              (1 << 1)
#define AXP20_ADC_GPIO3_ENABLE              (1 << 0)
#define AXP20_ADC_CONTROL3                  AXP20_ADC_SPEED


#define AXP20_VACH_RES                      AXP20_ACIN_VOL_H8
#define AXP20_VACL_RES                      AXP20_ACIN_VOL_L4
#define AXP20_IACH_RES                      AXP20_ACIN_CUR_H8
#define AXP20_IACL_RES                      AXP20_ACIN_CUR_L4
#define AXP20_VUSBH_RES                     AXP20_VBUS_VOL_H8
#define AXP20_VUSBL_RES                     AXP20_VBUS_VOL_L4
#define AXP20_IUSBH_RES                     AXP20_VBUS_CUR_H8
#define AXP20_IUSBL_RES                     AXP20_VBUS_CUR_L4
#define AXP20_TICH_RES                      (0x5E)
#define AXP20_TICL_RES                      (0x5F)

#define AXP20_TSH_RES                       (0x62)
#define AXP20_ISL_RES                       (0x63)
#define AXP20_VGPIO0H_RES                   (0x64)
#define AXP20_VGPIO0L_RES                   (0x65)
#define AXP20_VGPIO1H_RES                   (0x66)
#define AXP20_VGPIO1L_RES                   (0x67)
#define AXP20_VGPIO2H_RES                   (0x68)
#define AXP20_VGPIO2L_RES                   (0x69)
#define AXP20_VGPIO3H_RES                   (0x6A)
#define AXP20_VGPIO3L_RES                   (0x6B)
#define AXP20_IBATH_REG                     (0x7a)
#define AXP20_DISIBATH_REG                  (0x7c)

#define AXP20_PBATH_RES                     AXP20_BAT_POWERH8
#define AXP20_PBATM_RES                     AXP20_BAT_POWERM8
#define AXP20_PBATL_RES                     AXP20_BAT_POWERL8

#define AXP20_VBATH_RES                     AXP20_BAT_AVERVOL_H8
#define AXP20_VBATL_RES                     AXP20_BAT_AVERVOL_L4
#define AXP20_ICHARH_RES                    AXP20_BAT_AVERCHGCUR_H8
#define AXP20_ICHARL_RES                    AXP20_BAT_AVERCHGCUR_L5
#define AXP20_IDISCHARH_RES                 AXP20_BAT_AVERDISCHGCUR_H8
#define AXP20_IDISCHARL_RES                 AXP20_BAT_AVERDISCHGCUR_L5
#define AXP20_VAPSH_RES                     AXP20_APS_AVERVOL_H8
#define AXP20_VAPSL_RES                     AXP20_APS_AVERVOL_L4


#define AXP20_COULOMB_CONTROL               AXP20_COULOMB_CTL
#define AXP20_COULOMB_ENABLE                (1 << 7)
#define AXP20_COULOMB_SUSPEND               (1 << 6)
#define AXP20_COULOMB_CLEAR                 (1 << 5)

#define AXP20_CCHAR3_RES                    AXP20_BAT_CHGCOULOMB3
#define AXP20_CCHAR2_RES                    AXP20_BAT_CHGCOULOMB2
#define AXP20_CCHAR1_RES                    AXP20_BAT_CHGCOULOMB1
#define AXP20_CCHAR0_RES                    AXP20_BAT_CHGCOULOMB0
#define AXP20_CDISCHAR3_RES                 AXP20_BAT_DISCHGCOULOMB3
#define AXP20_CDISCHAR2_RES                 AXP20_BAT_DISCHGCOULOMB2
#define AXP20_CDISCHAR1_RES                 AXP20_BAT_DISCHGCOULOMB1
#define AXP20_CDISCHAR0_RES                 AXP20_BAT_DISCHGCOULOMB0

#define AXP20_CAP                           (0xB9)
#define AXP20_RDC1                          (0xba)
#define AXP20_RDC0                          (0xbb)

#define AXP20_CHARGE_VBUS                   AXP20_IPS_SET

#define AXP20_INTTEMP                       (0x5E)


#define AXP_CHG_ATTR(_name)                 \
{                                   \
	.attr = { .name = #_name, .mode = 0644 },\
	.show =  _name##_show,              \
	.store = _name##_store, \
}

#endif /* AXP20_SPLY_H */
