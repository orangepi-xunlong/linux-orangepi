/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef _AXP515_CHARGER_H_
#define _AXP515_CHARGER_H_

#include "sunxi-power-supply.h"
#include "axp2101.h"

#define AXP515_VBAT_MAX        (4608)
#define AXP515_VBAT_MIN        (3840)
#define AXP515_SOC_MAX         (100)
#define AXP515_SOC_MIN         (0)
#define AXP515_MAX_PARAM       32
#define AXP515_DEFAULT_TEMP  300
#define AXP515_MANUFACTURER  "xpower,axp515"

/* reg AXP515_COMM_STAT0  */
#define AXP515_MASK_CHARGE           (7 << 2)
#define AXP515_CHARGE_MAX            (5 << 2)
#define AXP515_CHARGE_DONE           (5 << 2)
#define AXP515_MASK_VBUS_STAT        BIT(1)
#define AXP515_MASK_BATFET_STAT      BIT(0)

/* reg AXP515_COMM_STAT1  */
#define AXP515_MASK_BAT_STAT         BIT(3)
#define AXP515_MASK_BAT_DET_STAT     BIT(4)

/* reg AXP515_CAP  */
#define AXP515_CAP_EN         BIT(7)

/* reg AXP515_OCV_PERCENT  */
#define AXP515_OCV_PER_EN         BIT(7)

/* reg AXP515_COULOMB_CTL  */
#define AXP515_GAUGE_EN          BIT(7)

/* reg AXP515_TIMER2_SET  */
#define AXP515_BAT_DET_EN          BIT(3)

/* reg AXP515_TS_PIN_CONTROL */
#define AXP515_MASK_ADC_STAT       (7 << 4)
#define AXP515_ADC_BATVOL_ENABLE   (1 << 4)
#define AXP515_ADC_TSVOL_ENABLE    (1 << 5)
#define AXP515_ADC_BATCUR_ENABLE   (1 << 6)
#define AXP515_ADC_DIETMP_ENABLE   (1 << 7)

/* reg AXP515_TS_CFG  */
#define AXP515_TS_ENABLE_MARK     BIT(7)

/* reg AXP515_IPRECHG_CFG  */
#define AXP515_CHG_EN          BIT(7)

/* reg AXP515_COMM_FAULT  */
#define AXP515_MASK_BAT_ACT_STAT          0x30
#define AXP515_MASK_BAT_WOT               BIT(0)
#define AXP515_MASK_BAT_WUT               BIT(1)

struct axp515_model_data {
	uint8_t *model;
	size_t model_size;
};

struct axp_config_info {
	/* usb */
	u32 pmu_bc12_en;
	u32 pmu_usbpc_vol;
	u32 pmu_usbpc_cur;
	u32 pmu_usbad_vol;
	u32 pmu_usbad_cur;
	u32 pmu_boost_en;
	u32 pmu_boost_vol;
	u32 pmu_usb_typec_used;

	/* qc */
	u32 pmu_bat_charge_control_lim;

	/* ac */
	u32 pmu_ac_vol;
	u32 pmu_ac_cur;

	/* battery capacity */
	u32 pmu_battery_cap;
	u32 pmu_battery_warning_level1;
	u32 pmu_battery_warning_level2;

	/* battery cycle */
	u32 pmu_bat_cycle_life;
	u32 pmu_bat_cycle_cap_reduce;

	/* battery manufacture date */
	u32 pmu_bat_manufacture_year;
	u32 pmu_bat_manufacture_month;
	u32 pmu_bat_manufacture_day;

	/* battery chgvol */
	u32 pmu_init_chgvol;
	u32 pmu_rechg_vol;

	/* battery chgcur */
	u32 pmu_runtime_chgcur;
	u32 pmu_suspend_chgcur;
	u32 pmu_shutdown_chgcur;
	u32 pmu_prechg_chgcur;
	u32 pmu_terminal_chgcur;

	/* battery chgled */
	u32 pmu_chgled_func;
	u32 pmu_chgled_type;

	/* battery temp */
	u32 pmu_bat_temp_enable;
	u32 pmu_bat_ts_current;
	u32 pmu_bat_charge_ltf;
	u32 pmu_bat_charge_htf;
	u32 pmu_bat_shutdown_ltf;
	u32 pmu_bat_shutdown_htf;
	u32 pmu_jetia_en;
	u32 pmu_jetia_cool;
	u32 pmu_jetia_warm;
	u32 pmu_jcool_ifall;
	u32 pmu_jwarm_ifall;
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

	u32 pmu_bat_temp_comp;

	/* For Gauge2.0 */
	u32 pmu_battery_rdc;
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

	/* legacy function */
	u32 pmu_used;
	u32 pmu_id;
	u32 pmu_as_slave;

	u32 pmu_batdeten;
	u32 pmu_button_bat_en;
	u32 pmu_bat_unused;
	u32 pmu_init_chgend_rate;
	u32 pmu_init_chg_enabled;
	u32 pmu_init_bc_en;
	u32 pmu_init_adc_freq;
	u32 pmu_init_adcts_freq;
	u32 pmu_init_chg_pretime;
	u32 pmu_init_chg_csttime;
	u32 pmu_batt_cap_correct;
	u32 pmu_restvol_adjust_time;
	u32 pmu_ocv_cou_adjust_time;
	u32 pmu_ocv_en;
	u32 pmu_cou_en;
	u32 pmu_chg_end_on_en;
	u32 ocv_coulumb_100;

	u32 pmu_gpio_vol;
	u32 pmu_gpio_cur;
	u32 pmu_vbusen_func;
	u32 pmu_update_min_time;
	u32 pmu_cc_logic_en;
	u32 pmu_chg_ic_temp;

	/* wakeup function */
	u32 wakeup_usb_in;
	u32 wakeup_usb_out;
	u32 wakeup_typec_in;
	u32 wakeup_typec_out;

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
	u32 wakeup_new_soc;
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
#define INTCHGCSTTIME   720         /* set initial pre-charging time */
#define BATMAXVOL       4200000     /* set battery max design volatge */
#define BATMINVOL       3500000     /* set battery min design volatge */
#define UPDATEMINTIME   30          /* set bat percent update min time */

#define OCVREG0         0x00        /* 2.99V */
#define OCVREG1         0x00        /* 3.13V */
#define OCVREG2         0x00        /* 3.27V */
#define OCVREG3         0x00        /* 3.34V */
#define OCVREG4         0x00        /* 3.41V */
#define OCVREG5         0x00        /* 3.48V */
#define OCVREG6         0x00        /* 3.52V */
#define OCVREG7         0x00        /* 3.55V */
#define OCVREG8         0x04        /* 3.57V */
#define OCVREG9         0x05        /* 3.59V */
#define OCVREGA         0x06        /* 3.61V */
#define OCVREGB         0x07        /* 3.63V */
#define OCVREGC         0x0a        /* 3.64V */
#define OCVREGD         0x0d        /* 3.66V */
#define OCVREGE         0x1a        /* 3.70V */
#define OCVREGF         0x24        /* 3.73V */
#define OCVREG10        0x29        /* 3.77V */
#define OCVREG11        0x2e        /* 3.78V */
#define OCVREG12        0x32        /* 3.80V */
#define OCVREG13        0x35        /* 3.84V */
#define OCVREG14        0x39        /* 3.85V */
#define OCVREG15        0x3d        /* 3.87V */
#define OCVREG16        0x43        /* 3.91V */
#define OCVREG17        0x49        /* 3.94V */
#define OCVREG18        0x4f        /* 3.98V */
#define OCVREG19        0x54        /* 4.01V */
#define OCVREG1A        0x58        /* 4.05V */
#define OCVREG1B        0x5c        /* 4.08V */
#define OCVREG1C        0x5e        /* 4.10V */
#define OCVREG1D        0x60        /* 4.12V */
#define OCVREG1E        0x62        /* 4.14V */
#define OCVREG1F        0x64        /* 4.15V */

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

struct axp_gpio_para {
	int	gpio;
	int irq_num;
};

#define SUNXI_PINCFG_PACK(type, value)  (((value) << 8) | (type & 0xFF))

#endif
