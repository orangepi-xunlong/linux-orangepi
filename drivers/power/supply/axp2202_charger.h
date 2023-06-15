#ifndef _AXP2202_CHARGER_H_
#define _AXP2202_CHARGER_H_

#include "linux/types.h"

#define AXP2202_VBAT_MAX        (8000)
#define AXP2202_VBAT_MIN        (2000)
#define AXP2202_SOC_MAX         (100)
#define AXP2202_SOC_MIN         (0)
#define AXP2202_MAX_PARAM       512
#define AXP2202_MANUFACTURER  "xpower,axp2202"

/* reg AXP2202_COMM_STAT0  */
#define AXP2202_MASK_VBUS_STAT        BIT(5)
#define AXP2202_MASK_BATFET_STAT      BIT(4)
#define AXP2202_MASK_BAT_STAT         BIT(3)
#define AXP2202_MASK_BAT_ACT_STAT     BIT(2)
#define AXP2202_MASK_THREM_STAT       BIT(1)
#define AXP2202_MASK_ILIM_STAT        BIT(0)

/* reg AXP2202_COMM_FAULT  */
#define AXP2202_MASK_BAT_WOT          BIT(1)
#define AXP2202_MASK_BAT_WUT          BIT(0)

/* reg AXP2202_RESET_CFG  */
#define AXP2202_MODE_RSTGAUGE        BIT(3)
#define AXP2202_MODE_RSTMCU          BIT(2)
#define AXP2202_MODE_POR             BIT(0)

/* reg AXP2202_ROM_SEL  */
#define AXP2202_BROMUP_EN           BIT(0)
#define AXP2202_CFG_UPDATE_MARK     BIT(4)
#define AXP2202_CFG_WDT_EN          BIT(5)

/* reg AXP2202_COMM_STAT1  */
#define AXP2202_STATUS_BAT_CUR_DIRCT GENMASK(6, 5)
#define AXP2202_CHARGING_TRI  (0)
#define AXP2202_CHARGING_PRE  (1)
#define AXP2202_CHARGING_CC   (2)
#define AXP2202_CHARGING_CV   (3)
#define AXP2202_CHARGING_DONE (4)
#define AXP2202_CHARGING_NCHG (5)

/* reg AXP2202_ADC_CH_EN0  */
#define AXP2202_ADC_TS_SEL	(0x00)
#define AXP2202_ADC_TDIE_SEL	(0x01)
#define AXP2202_ADC_VMID_SEL	(0x02)
#define AXP2202_ADC_VBK_SEL	(0x03)

/* reg AXP2202_VTERM_CFG  */
#define AXP2202_CHRG_TGT_VOLT		GENMASK(2, 0)
#define AXP2202_CHRG_CTRL1_TGT_4_0V	(0)
#define AXP2202_CHRG_CTRL1_TGT_4_1V	(1)
#define AXP2202_CHRG_CTRL1_TGT_4_2V	(2)
#define AXP2202_CHRG_CTRL1_TGT_4_35V	(3)
#define AXP2202_CHRG_CTRL1_TGT_4_4V	(4)
#define AXP2202_CHRG_CTRL1_TGT_5_0V	(7)

/* reg AXP2202_TS_CFG  */
#define AXP2202_TS_ENABLE_MARK     BIT(4)
#define AXP2202_TS_CURR_MARK       BIT(0)

/* reg AXP2202_JEITA_CFG  */
#define AXP2202_JEITA_ENABLE_MARK  BIT(0)

/* reg AXP2202_ACIN_CC_OTG  */
#define AXP2202_OTG_MARK  (0x0F)
#define AXP2202_CHARGER_ENABLE_MARK BIT(1)

/* AXP2202 v4_35 battery */
#define AXP2202_BATTERY_v4_35 0

struct axp2202_model_data {
	uint8_t *model;
	size_t model_size;
};


struct axp_config_info {
	u32 pmu_used;
	u32 pmu_id;
	u32 pmu_battery_rdc;
	u32 pmu_battery_cap;
	u32 pmu_batdeten;
	u32 pmu_button_bat_en;
	u32 pmu_chg_ic_temp;
	u32 pmu_runtime_chgcur;
	u32 pmu_suspend_chgcur;
	u32 pmu_shutdown_chgcur;
	u32 pmu_prechg_chgcur;
	u32 pmu_terminal_chgcur;
	u32 pmu_init_chgvol;
	u32 pmu_init_chgend_rate;
	u32 pmu_init_chg_enabled;
	u32 pmu_init_bc_en;
	u32 pmu_init_adc_freq;
	u32 pmu_init_adcts_freq;
	u32 pmu_init_chg_pretime;
	u32 pmu_init_chg_csttime;
	u32 pmu_batt_cap_correct;
	u32 pmu_chg_end_on_en;
	u32 ocv_coulumb_100;

	u32 pmu_bat_ts_current;

	u32 pmu_jetia_en;
	u32 pmu_jetia_cool;
	u32 pmu_jetia_warm;
	u32 pmu_jcool_ifall;
	u32 pmu_jwarm_ifall;

	/* For Gauge2.0 */
	u32 pmu_bat_para1;
	u32 pmu_bat_para2;
	u32 pmu_bat_para3;
	u32 pmu_bat_para4;
	u32 pmu_bat_para5;
	u32 pmu_bat_para6;
	u32 pmu_bat_para7;
	u32 pmu_bat_para8;
	u32 pmu_bat_para9;
	u32 pmu_bat_para10;
	u32 pmu_bat_para11;
	u32 pmu_bat_para12;
	u32 pmu_bat_para13;
	u32 pmu_bat_para14;
	u32 pmu_bat_para15;
	u32 pmu_bat_para16;
	u32 pmu_bat_para17;
	u32 pmu_bat_para18;
	u32 pmu_bat_para19;
	u32 pmu_bat_para20;
	u32 pmu_bat_para21;
	u32 pmu_bat_para22;
	u32 pmu_bat_para23;
	u32 pmu_bat_para24;
	u32 pmu_bat_para25;
	u32 pmu_bat_para26;
	u32 pmu_bat_para27;
	u32 pmu_bat_para28;
	u32 pmu_bat_para29;
	u32 pmu_bat_para30;
	u32 pmu_bat_para31;
	u32 pmu_bat_para32;

	u32 pmu_ac_vol;
	u32 pmu_ac_cur;
	u32 pmu_usbpc_vol;
	u32 pmu_usbpc_cur;
	u32 pmu_usbad_vol;
	u32 pmu_usbad_cur;
	u32 pmu_gpio_vol;
	u32 pmu_gpio_cur;
	u32 pmu_usb_typec_used;
	u32 pmu_pwroff_vol;
	u32 pmu_pwron_vol;
	u32 pmu_powkey_off_time;
	u32 pmu_powkey_off_en;
	u32 pmu_powkey_off_delay_time;
	u32 pmu_powkey_off_func;
	u32 pmu_powkey_long_time;
	u32 pmu_powkey_on_time;
	u32 pmu_powkey_wakeup_irq;
	u32 pmu_pwrok_time;
	u32 pmu_pwrnoe_time;
	u32 pmu_reset_shutdown_en;
	u32 pmu_battery_warning_level1;
	u32 pmu_battery_warning_level2;
	u32 pmu_restvol_adjust_time;
	u32 pmu_ocv_cou_adjust_time;
	u32 pmu_chgled_func;
	u32 pmu_chgled_type;
	u32 pmu_vbusen_func;
	u32 pmu_reset;
	u32 pmu_irq_wakeup;
	u32 pmu_hot_shutdown;
	u32 pmu_inshort;
	u32 power_start;
	u32 pmu_as_slave;
	u32 pmu_bat_unused;
	u32 pmu_ocv_en;
	u32 pmu_cou_en;
	u32 pmu_update_min_time;

	u32 pmu_bat_temp_enable;
	u32 pmu_bat_charge_ltf;
	u32 pmu_bat_charge_htf;
	u32 pmu_bat_shutdown_ltf;
	u32 pmu_bat_shutdown_htf;
	u32 pmu_bat_temp_para1;
	u32 pmu_bat_temp_para2;
	u32 pmu_bat_temp_para3;
	u32 pmu_bat_temp_para4;
	u32 pmu_bat_temp_para5;
	u32 pmu_bat_temp_para6;
	u32 pmu_bat_temp_para7;
	u32 pmu_bat_temp_para8;
	u32 pmu_bat_temp_para9;
	u32 pmu_bat_temp_para10;
	u32 pmu_bat_temp_para11;
	u32 pmu_bat_temp_para12;
	u32 pmu_bat_temp_para13;
	u32 pmu_bat_temp_para14;
	u32 pmu_bat_temp_para15;
	u32 pmu_bat_temp_para16;

	u32 wakeup_usb_in;
	u32 wakeup_usb_out;
	u32 wakeup_bat_in;
	u32 wakeup_bat_out;
	u32 wakeup_bat_charging;
	u32 wakeup_bat_charge_over;
	u32 wakeup_low_warning1;
	u32 wakeup_low_warning2;
	u32 wakeup_bat_untemp_work;
	u32 wakeup_bat_ovtemp_work;
	u32 wakeup_untemp_chg;
	u32 wakeup_ovtemp_chg;
	u32 wakeup_bat_ov;
	u32 wakeup_gpio;

	u32 pmu_bc12_en;
	u32 pmu_cc_logic_en;
	u32 pmu_boost_en;
	u32 pmu_boost_vol;
};



#define BATRDC          100
#define INTCHGCUR       300000      /* set initial charging current limite */
#define SUSCHGCUR       1000000     /* set suspend charging current limite */
#define RESCHGCUR       INTCHGCUR   /* set resume charging current limite */
#define CLSCHGCUR       SUSCHGCUR   /* set shutdown charging current limite */
#define INTCHGVOL       4200000     /* set initial charing target voltage */
#define INTCHGENDRATE   10          /* set initial charing end current rate */
#define INTCHGENABLED   1           /* set initial charing enabled */
#define INTADCFREQ      25          /* set initial adc frequency */
#define INTADCFREQC     100         /* set initial coulomb adc coufrequency */
#define INTCHGPRETIME   50          /* set initial pre-charging time */
#define INTCHGCSTTIME   480         /* set initial pre-charging time */
#define BATMAXVOL       4200000     /* set battery max design volatge */
#define BATMINVOL       3500000     /* set battery min design volatge */
#define UPDATEMINTIME   30          /* set bat percent update min time */

#define AXP_OF_PROP_READ(name, def_value)\
do {\
	if (of_property_read_u32(node, #name, &axp_config->name))\
		axp_config->name = def_value;\
} while (0)

struct axp_interrupts {
	char *name;
	irq_handler_t isr;
	int irq;
};

#endif

