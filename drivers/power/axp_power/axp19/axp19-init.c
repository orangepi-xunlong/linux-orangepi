/*
 * Battery charger driver for X-POWERS AXP19X
 *
 * Copyright (C) 2014 X-POWERS Ltd.
 *  Ming Li <liming@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/reboot.h>
#include "../axp-cfg.h"
#include "axp19-sply.h"

void axp19_power_off(int power_start)
{
	struct axp_dev *axp;
	axp = axp_dev_lookup(AXP19);
	if (NULL == axp) {
		printk("%s: axp data is null\n", __func__);
		return;
	}

	axp_set_bits(axp->dev, POWER19_OFF_CTL, 0x80);
	return;
}

#if defined  (CONFIG_AXP_CHARGEINIT)
static void axp_set_charge(struct axp_charger *charger)
{
	uint8_t val=0x00;
	uint8_t	tmp=0x00;
	uint8_t var[3];
		if(charger->chgvol < 4150)
			val &= ~(3 << 5);
		else if (charger->chgvol<4200){
			val &= ~(3 << 5);
			val |= 1 << 5;
			}
		else if (charger->chgvol<4360){
			val &= ~(3 << 5);
			val |= 1 << 6;
			}
		else
			val |= 3 << 5;

		if(charger->chgcur< 100)
			charger->chgcur =100;

		val |= (charger->chgcur - 100) / 100 ;
		if(charger ->chgend == 10){
			val &= ~(1 << 4);
		}
		else {
			val |= 1 << 4;
		}
		val &= 0x7F;
		val |= charger->chgen << 7;
	    if(charger->chgpretime < 30)
			charger->chgpretime = 30;
		if(charger->chgcsttime < 420)
			charger->chgcsttime = 420;
		if(charger->chgextcur < 300)
			charger->chgextcur = 300;

		tmp = ((charger->chgpretime - 30) / 10) << 6  \
			| (charger->chgcsttime - 420) / 60 | \
			(charger->chgexten << 2) | ((charger->chgextcur - 300) / 100 << 3);

	var[0] = val;
	var[1] = AXP19_CHARGE_CONTROL2;
	var[2] = tmp;
	axp_writes(charger->master, AXP19_CHARGE_CONTROL1,3, var);
}
#else
static void axp_set_charge(struct axp_charger *charger)
{

}
#endif

#if defined  (CONFIG_AXP_CHARGEINIT)
static int axp_battery_adc_set(struct axp_charger *charger)
{
	 int ret ;
	 uint8_t val;

	/*enable adc and set adc */
	val= AXP19_ADC_BATVOL_ENABLE | AXP19_ADC_BATCUR_ENABLE
	| AXP19_ADC_DCINCUR_ENABLE | AXP19_ADC_DCINVOL_ENABLE
	| AXP19_ADC_USBVOL_ENABLE | AXP19_ADC_USBCUR_ENABLE;

	ret = axp_write(charger->master, AXP19_ADC_CONTROL1, val);
	if (ret)
		return ret;
    ret = axp_read(charger->master, AXP19_ADC_CONTROL3, &val);
	switch (charger->sample_time/25){
	case 1: val &= ~(3 << 6);break;
	case 2: val &= ~(3 << 6);val |= 1 << 6;break;
	case 4: val &= ~(3 << 6);val |= 2 << 6;break;
	case 8: val |= 3 << 6;break;
	default: break;
	}
	ret = axp_write(charger->master, AXP19_ADC_CONTROL3, val);
	if (ret)
		return ret;

	return 0;
}
#else
static int axp_battery_adc_set(struct axp_charger *charger)
{
	return 0;
}
#endif

static int axp_battery_first_init(struct axp_charger *charger)
{
	int ret;
	uint8_t val;
	axp_set_charge(charger);
	ret = axp_battery_adc_set(charger);
	if(ret)
		return ret;

	ret = axp_read(charger->master, AXP19_ADC_CONTROL3, &val);
	switch ((val >> 6) & 0x03){
	case 0: charger->sample_time = 25;break;
	case 1: charger->sample_time = 50;break;
	case 2: charger->sample_time = 100;break;
	case 3: charger->sample_time = 200;break;
	default:break;
	}
  return ret;
}

int axp19_init(struct axp_charger *charger)
{
	int ret = 0;

	ret = axp_battery_first_init(charger);
	if (ret)
		goto err_charger_init;

	return ret;
err_charger_init:
	return ret;

}

void axp19_exit(struct axp_charger *charger)
{
	return;
}

