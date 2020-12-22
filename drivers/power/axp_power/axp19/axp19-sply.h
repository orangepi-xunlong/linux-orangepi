#ifndef	_LINUX_AXP19_SPLY_H_
#define	_LINUX_AXP19_SPLY_H_
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/mfd/axp-mfd.h>
#include <linux/power_supply.h>


/*      AXP19      */
#define AXP19_CHARGE_STATUS                             POWER19_STATUS
#define AXP19_IN_CHARGE                                 (1 << 6)

#define AXP19_CHARGE_CONTROL1                           POWER19_CHARGE1
#define AXP19_CHARGER_ENABLE                            (1 << 7)
#define AXP19_CHARGE_CONTROL2                           POWER19_CHARGE2
#define AXP19_BUCHARGE_CONTROL                          POWER19_BACKUP_CHG
#define AXP19_BUCHARGER_ENABLE                          (1 << 7)


#define AXP19_FAULT_LOG1                                POWER19_MODE_CHGSTATUS
#define AXP19_FAULT_LOG_CHA_CUR_LOW                     (1 << 2)
#define AXP19_FAULT_LOG_BATINACT                        (1 << 3)

#define AXP19_FAULT_LOG_OVER_TEMP                       (1 << 7)

#define AXP19_FAULT_LOG2                                POWER19_INTSTS2
#define AXP19_FAULT_LOG_COLD                            (1 << 0)

#define AXP19_FINISH_CHARGE                             (1 << 2)


#define AXP19_ADC_CONTROL1                              POWER19_ADC_EN1
#define AXP19_ADC_BATVOL_ENABLE                         (1 << 7)
#define AXP19_ADC_BATCUR_ENABLE                         (1 << 6)
#define AXP19_ADC_DCINVOL_ENABLE                        (1 << 5)
#define AXP19_ADC_DCINCUR_ENABLE                        (1 << 4)
#define AXP19_ADC_USBVOL_ENABLE                         (1 << 3)
#define AXP19_ADC_USBCUR_ENABLE                         (1 << 2)
#define AXP19_ADC_APSVOL_ENABLE                         (1 << 1)
#define AXP19_ADC_TSVOL_ENABLE                          (1 << 0)
#define AXP19_ADC_CONTROL2                              POWER19_ADC_EN2
#define AXP19_ADC_INTERTEM_ENABLE                       (1 << 7)

#define AXP19_ADC_GPIO0_ENABLE                          (1 << 3)
#define AXP19_ADC_GPIO1_ENABLE                          (1 << 2)
#define AXP19_ADC_GPIO2_ENABLE                          (1 << 1)
#define AXP19_ADC_GPIO3_ENABLE                          (1 << 0)
#define AXP19_ADC_CONTROL3                              POWER19_ADC_SPEED


#define AXP19_VACH_RES                                  POWER19_ACIN_VOL_H8
#define AXP19_VACL_RES                                  POWER19_ACIN_VOL_L4
#define AXP19_IACH_RES                                  POWER19_ACIN_CUR_H8
#define AXP19_IACL_RES                                  POWER19_ACIN_CUR_L4
#define AXP19_VUSBH_RES                                 POWER19_VBUS_VOL_H8
#define AXP19_VUSBL_RES                                 POWER19_VBUS_VOL_L4
#define AXP19_IUSBH_RES                                 POWER19_VBUS_CUR_H8
#define AXP19_IUSBL_RES                                 POWER19_VBUS_CUR_L4
#define AXP19_TICH_RES                                  (0x5E)
#define AXP19_TICL_RES                                  (0x5F)

#define AXP19_TSH_RES                                   (0x62)
#define AXP19_ISL_RES                                   (0x63)
#define AXP19_VGPIO0H_RES                               (0x64)
#define AXP19_VGPIO0L_RES                               (0x65)
#define AXP19_VGPIO1H_RES                               (0x66)
#define AXP19_VGPIO1L_RES                               (0x67)
#define AXP19_VGPIO2H_RES                               (0x68)
#define AXP19_VGPIO2L_RES                               (0x69)
#define AXP19_VGPIO3H_RES                               (0x6A)
#define AXP19_VGPIO3L_RES                               (0x6B)

#define AXP19_PBATH_RES                                 POWER19_BAT_POWERH8
#define AXP19_PBATM_RES                                 POWER19_BAT_POWERM8
#define AXP19_PBATL_RES                                 POWER19_BAT_POWERL8

#define AXP19_VBATH_RES                                 POWER19_BAT_AVERVOL_H8
#define AXP19_VBATL_RES                                 POWER19_BAT_AVERVOL_L4
#define AXP19_ICHARH_RES                                POWER19_BAT_AVERCHGCUR_H8
#define AXP19_ICHARL_RES                                POWER19_BAT_AVERCHGCUR_L5
#define AXP19_IDISCHARH_RES                             POWER19_BAT_AVERDISCHGCUR_H8
#define AXP19_IDISCHARL_RES                             POWER19_BAT_AVERDISCHGCUR_L5
#define AXP19_VAPSH_RES                                 POWER19_APS_AVERVOL_H8
#define AXP19_VAPSL_RES                                 POWER19_APS_AVERVOL_L4

#define AXP19_COULOMB_CONTROL                           POWER19_COULOMB_CTL
#define AXP19_COULOMB_ENABLE                            (1 << 7)
#define AXP19_COULOMB_SUSPEND                           (1 << 6)
#define AXP19_COULOMB_CLEAR                             (1 << 5)

#define AXP19_CCHAR3_RES                                POWER19_BAT_CHGCOULOMB3
#define AXP19_CCHAR2_RES                                POWER19_BAT_CHGCOULOMB2
#define AXP19_CCHAR1_RES                                POWER19_BAT_CHGCOULOMB1
#define AXP19_CCHAR0_RES                                POWER19_BAT_CHGCOULOMB0
#define AXP19_CDISCHAR3_RES                             POWER19_BAT_DISCHGCOULOMB3
#define AXP19_CDISCHAR2_RES                             POWER19_BAT_DISCHGCOULOMB2
#define AXP19_CDISCHAR1_RES                             POWER19_BAT_DISCHGCOULOMB1
#define AXP19_CDISCHAR0_RES                             POWER19_BAT_DISCHGCOULOMB0

#define AXP19_DATA_BUFFER0                              POWER19_DATA_BUFFER1
#define AXP19_DATA_BUFFER1                              POWER19_DATA_BUFFER2
#define AXP19_DATA_BUFFER2                              POWER19_DATA_BUFFER3
#define AXP19_DATA_BUFFER3                              POWER19_DATA_BUFFER4

#define AXP19_CHARGE_VBUS                               POWER19_IPS_SET

#define AXP19_CHARGE_LED                                POWER19_OFF_CTL

#define AXP19_TIMER_CTL                                 POWER19_TIMER_CTL

#define AXP19_INTTEMP                                   (0x5e)

#define AXP19_VOL_MAX                                   (50)
#define AXP19_TIME_MAX                                  (20)
#define AXP19_AVER_MAX                                  (10)
#define AXP19_RDC_COUNT                                 (10)

#define END_VOLTAGE_APS         3350
#define BAT_AVER_VOL            3820	//Aver Vol:3.82V

#define FUELGUAGE_LOW_VOL       3400	//<3.4v,2%
#define FUELGUAGE_VOL1          3500    //<3.5v,3%
#define FUELGUAGE_VOL2          3600
#define FUELGUAGE_VOL3          3700
#define FUELGUAGE_VOL4          3800
#define FUELGUAGE_VOL5          3900
#define FUELGUAGE_VOL6          4000
#define FUELGUAGE_VOL7          4100
#define FUELGUAGE_TOP_VOL       4160	//>4.16v,100%

#define FUELGUAGE_LOW_LEVEL     2		//<3.4v,2%
#define FUELGUAGE_LEVEL1        3		//<3.5v,3%
#define FUELGUAGE_LEVEL2        5
#define FUELGUAGE_LEVEL3        16
#define FUELGUAGE_LEVEL4        46
#define FUELGUAGE_LEVEL5        66
#define FUELGUAGE_LEVEL6        83
#define FUELGUAGE_LEVEL7        93
#define FUELGUAGE_TOP_LEVEL     100     //>4.16v,100%

#define AXP_CHG_ATTR(_name)					\
{								\
	.attr = { .name = #_name,.mode = 0644 },		\
	.show =  _name##_show,					\
	.store = _name##_store,					\
}

struct axp_adc_res {//struct change
	uint16_t vbat_res;
	uint16_t ocvbat_res;
	uint16_t ibat_res;
	uint16_t ichar_res;
	uint16_t idischar_res;
	uint16_t vac_res;
	uint16_t iac_res;
	uint16_t vusb_res;
	uint16_t iusb_res;
	uint16_t ts_res;
};

struct axp_charger {
	/*power supply sysfs*/
	struct power_supply batt;
	struct power_supply	ac;
	struct power_supply	usb;
	struct power_supply bubatt;

	/*i2c device*/
	struct device *master;

	/* adc */
	struct axp_adc_res *adc;
	unsigned int sample_time;

	/*monitor*/
	struct delayed_work work;
	unsigned int interval;

	/*battery info*/
	struct power_supply_info *battery_info;

	/*charger control*/
	bool chgen;
	bool limit_on;
	unsigned int chgcur;
	unsigned int chgvol;
	unsigned int chgend;

	/*charger time */
	int chgpretime;
	int chgcsttime;

	/*external charger*/
	bool chgexten;
	int chgextcur;

	/* charger status */
	bool bat_det;
	bool is_on;
	bool is_finish;
	bool ac_not_enough;
	bool ac_det;
	bool usb_det;
	bool ac_valid;
	bool usb_valid;
	bool ext_valid;
	bool bat_current_direction;
	bool in_short;
	bool batery_active;
	bool low_charge_current;
	bool int_over_temp;
	uint8_t fault;
	int charge_on;

	int vbat;
	int ibat;
	int pbat;
	int vac;
	int iac;
	int vusb;
	int iusb;
	int ocv;

	int disvbat;
	int disibat;

	/*rest time*/
	int rest_vol;
	int ocv_rest_vol;
	int base_restvol;
	int rest_time;

	/*ic temperature*/
	int ic_temp;
	int bat_temp;

	/*irq*/
	struct notifier_block nb;

	/* platform callbacks for battery low and critical events */
	void (*battery_low)(void);
	void (*battery_critical)(void);

	struct dentry *debug_file;

	spinlock_t charger_lock;
};

extern const struct axp_config_info axp19_config;
extern struct class axppower_class;
extern struct axp_charger *axp_charger;

extern int axp_charger_create_attrs(struct power_supply *psy);
extern void axp_charger_update_state(struct axp_charger *charger);
extern void axp_charger_update(struct axp_charger *charger, const struct axp_config_info *axp_config);
extern int axp19_init(struct axp_charger *charger);
extern void axp19_exit(struct axp_charger *charger);
extern int axp_irq_init(struct axp_charger *charger, struct platform_device *pdev);
extern void axp_irq_exit(struct axp_charger *charger);
extern int axp_enable_irq(struct axp_charger *charger);
extern int axp_disable_irq(struct axp_charger *charger);

#endif

