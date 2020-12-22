/*
 * Battery charger driver for AW-POWERS
 *
 * Copyright (C) 2014 ALLWINNERTECH.
 *  Ming Li <liming@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/power_supply.h>
#include "axp-cfg.h"
#include <linux/mfd/axp-mfd.h>
#ifdef CONFIG_AW_AXP81X
#include "axp81x/axp81x-sply.h"
#include "axp81x/axp81x-common.h"
static const struct axp_config_info *axp_config = &axp81x_config;
#endif

static aw_charge_type axp_usbcurflag = CHARGE_AC;
static aw_charge_type axp_usbvolflag = CHARGE_AC;

int axp_usbvol(aw_charge_type type)
{
	axp_usbvolflag = type;
	return 0;
}
EXPORT_SYMBOL_GPL(axp_usbvol);

int axp_usbcur(aw_charge_type type)
{
	axp_usbcurflag = type;
	mod_timer(&axp_charger->usb_status_timer,jiffies + msecs_to_jiffies(0));
	return 0;
}
EXPORT_SYMBOL_GPL(axp_usbcur);

int axp_usb_det(void)
{
	uint8_t ret = 0;

	if(axp_charger == NULL || axp_charger->master == NULL)
	{
		return ret;
	}
	axp_read(axp_charger->master,AXP_CHARGE_STATUS,&ret);
	if(ret & 0x10)/*usb or usb adapter can be used*/
		return 1;
	else/*no usb or usb adapter*/
		return 0;
}
EXPORT_SYMBOL_GPL(axp_usb_det);

static void axp_usb_ac_check_status(struct axp_charger *charger)
{
	if (!axp_config->pmu_init_bc_en) {
		charger->usb_valid = (((CHARGE_USB_20 == axp_usbcurflag) || (CHARGE_USB_30 == axp_usbcurflag)) &&
					charger->ext_valid);
		charger->usb_adapter_valid = ((0 == charger->ac_valid) && (CHARGE_USB_20 != axp_usbcurflag) &&
						(CHARGE_USB_30 != axp_usbcurflag) && (charger->ext_valid));
		if(charger->in_short) {
			charger->ac_valid = ((charger->usb_adapter_valid == 0) && (charger->usb_valid == 0) &&
						(charger->ext_valid));
		}
	} else {
		charger->usb_adapter_valid = 0;
	}
	power_supply_changed(&charger->ac);
	power_supply_changed(&charger->usb);

	DBG_PSY_MSG(DEBUG_CHG, "usb_valid=%d ac_valid=%d usb_adapter_valid=%d\n",charger->usb_valid,
				charger->ac_valid, charger->usb_adapter_valid);
	DBG_PSY_MSG(DEBUG_CHG, "usb_det=%d ac_det=%d \n",charger->usb_det,
				charger->ac_det);
}

int axp_read_ac_chg(void)
{
	int ac_chg = 0;

	if (axp_charger->ac_valid || axp_charger->usb_adapter_valid)
		ac_chg = 1;
	else
		ac_chg = 0;

	return ac_chg;
}
EXPORT_SYMBOL_GPL(axp_read_ac_chg);

static void axp_charger_update_usb_state(unsigned long data)
{
	struct axp_charger * charger = (struct axp_charger *)data;

	axp_usb_ac_check_status(charger);
	schedule_delayed_work(&(charger->usbwork), 0);
}

#ifdef CONFIG_AW_AXP81X
static void axp_usb(struct work_struct *work)
{
	int var;
	uint8_t tmp,val;

	DBG_PSY_MSG(DEBUG_CHG, "[axp_usb]axp_usbcurflag = %d\n",axp_usbcurflag);

	axp_read(axp_charger->master, AXP_CHARGE_STATUS, &val);
	if ((val & 0x02)) { /* usb and ac in short*/
		if((val & 0x10) == 0x00){/*usb or usb adapter can not be used*/
			DBG_PSY_MSG(DEBUG_CHG, "USB not insert!\n");
			tmp = 0x10;
			axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
		}else if(CHARGE_USB_20 == axp_usbcurflag){
			DBG_PSY_MSG(DEBUG_CHG, "set usbcur_pc %d mA\n",axp_config->pmu_usbcur_pc);
			if(axp_config->pmu_usbcur_pc){
				var = axp_config->pmu_usbcur_pc;
				if(var < 500) {
					tmp = 0x00;   /* 100mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else if (var < 900) {
					tmp = 0x10;   /* 500mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else if (var < 1500) {
					tmp = 0x20;   /* 900mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else if (var < 2000) {
					tmp = 0x30;   /* 1500mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else if (var < 2500) {
					tmp = 0x40;   /* 2000mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else if (var < 3000) {
					tmp = 0x50;   /* 2500mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else if (var < 3500) {
					tmp = 0x60;   /* 3000mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else if (var < 3500) {
					tmp = 0x70;   /* 3500mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else {
					tmp = 0x80;   /* 4000mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				}
			} else {
				tmp = 0x10;   /* 500mA */
				axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
			}
		}else if (CHARGE_USB_30 == axp_usbcurflag){
			tmp = 0x20; /* 900mA */
			axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
		}else {
			DBG_PSY_MSG(DEBUG_CHG, "set usbcur %d mA\n",axp_config->pmu_usbcur);
			if((axp_config->pmu_usbcur) && (axp_config->pmu_usbcur_limit)){
				var = axp_config->pmu_usbcur;
				if(var < 500) {
					tmp = 0x00;   /* 100mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else if (var < 900) {
					tmp = 0x10;   /* 500mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else if (var < 1500) {
					tmp = 0x20;   /* 900mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else if (var < 2000) {
					tmp = 0x30;   /* 1500mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else if (var < 2500) {
					tmp = 0x40;   /* 2000mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else if (var < 3000) {
					tmp = 0x50;   /* 2500mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else if (var < 3500) {
					tmp = 0x60;   /* 3000mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else if (var < 4000) {
					tmp = 0x70;   /* 3500mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else {
					tmp = 0x80;   /* 4000mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				}
			} else {
				tmp = 0x50;   /* 2500mA */
				DBG_PSY_MSG(DEBUG_CHG, "%s: %d,set usbcur 2500 mA\n",__func__, __LINE__);
				axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
			}
		}

		if(!vbus_curr_limit_debug){ //usb current not limit
			DBG_PSY_MSG(DEBUG_CHG, "vbus_curr_limit_debug = %d\n",vbus_curr_limit_debug);
			tmp = 0x50;   /* 2500mA */
			axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
		}

		if(CHARGE_USB_20 == axp_usbvolflag){
			DBG_PSY_MSG(DEBUG_CHG, "set usbvol_pc %d mV\n",axp_config->pmu_usbvol_pc);
			if(axp_config->pmu_usbvol_pc){
				var = axp_config->pmu_usbvol_pc * 1000;
				if(var >= 4000000 && var <=4700000){
					tmp = (var - 4000000)/100000;
					val = tmp << 3;
					axp_update(axp_charger->master, AXP_CHARGE_VBUS, val,0x38);
				}else
					DBG_PSY_MSG(DEBUG_CHG, "set usb limit voltage error,%d mV\n",axp_config->pmu_usbvol_pc);
			}
		}else if(CHARGE_USB_30 == axp_usbvolflag) {
			val = 7 << 3;
			axp_update(axp_charger->master, AXP_CHARGE_VBUS, val,0x38);
		}else {
			DBG_PSY_MSG(DEBUG_CHG, "set usbvol %d mV\n",axp_config->pmu_usbvol);
			if((axp_config->pmu_usbvol) && (axp_config->pmu_usbvol_limit)){
				var = axp_config->pmu_usbvol * 1000;
				if(var >= 4000000 && var <=4700000){
					tmp = (var - 4000000)/100000;
					val = tmp << 3;
					axp_update(axp_charger->master, AXP_CHARGE_VBUS, val,0x38);
				}else
					DBG_PSY_MSG(DEBUG_CHG, "set usb limit voltage error,%d mV\n",axp_config->pmu_usbvol);
			}
		}
	}else {
		if((val & 0x50) == 0x00){/*usb and ac can not be used*/
			DBG_PSY_MSG(DEBUG_CHG, "USB not insert!\n");
			tmp = 0x10; /* 500mA */
			axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
			tmp = 0x0; /* 1500mA */
			axp_update(axp_charger->master, AXP_CHARGE_AC, tmp,0x07);
		}else if(CHARGE_USB_20 == axp_usbcurflag){
			DBG_PSY_MSG(DEBUG_CHG, "set usbcur_pc %d mA\n",axp_config->pmu_usbcur_pc);
			if(axp_config->pmu_usbcur_pc){
				var = axp_config->pmu_usbcur_pc;
				if(var < 500) {
					tmp = 0x00;   /* 100mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else if (var < 900) {
					tmp = 0x10;   /* 500mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else if (var < 1500) {
					tmp = 0x20;   /* 900mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else if (var < 2000) {
					tmp = 0x30;   /* 1500mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else if (var < 2500) {
					tmp = 0x40;   /* 2000mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else if (var < 3000) {
					tmp = 0x50;   /* 2500mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else if (var < 3500) {
					tmp = 0x60;   /* 3000mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else if (var < 4000) {
					tmp = 0x70;   /* 3500mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				} else {
					tmp = 0x80;   /* 4000mA */
					axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				}
			} else {
				tmp = 0x10;   /* 500mA */
				axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
			}
		}else if (CHARGE_USB_30 == axp_usbcurflag){
			tmp = 0x20; /* 900mA */
			axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
		}else {
			DBG_PSY_MSG(DEBUG_CHG, "set usbcur %d mA\n",axp_config->pmu_usbcur);
			if((axp_config->pmu_usbcur) && (axp_config->pmu_usbcur_limit)){
				var = axp_config->pmu_usbcur;
				if (var < 2000) {
					tmp = 0x01;   /* 1500mA */
					axp_update(axp_charger->master, AXP_CHARGE_AC, tmp,0x07);
				} else if (var < 2500) {
					tmp = 0x02;   /* 2000mA */
					axp_update(axp_charger->master, AXP_CHARGE_AC, tmp,0x07);
				} else if (var < 3000) {
					tmp = 0x03;   /* 2500mA */
					axp_update(axp_charger->master, AXP_CHARGE_AC, tmp,0x07);
				} else if (var < 3500) {
					tmp = 0x04;   /* 3000mA */
					axp_update(axp_charger->master, AXP_CHARGE_AC, tmp,0x07);
				} else if (var < 3500) {
					tmp = 0x05;   /* 3500mA */
					axp_update(axp_charger->master, AXP_CHARGE_AC, tmp,0x07);
				} else {
					tmp = 0x06;   /* 4000mA */
					axp_update(axp_charger->master, AXP_CHARGE_AC, tmp,0x07);
				}
			} else {
				tmp = 0x05;   /* 4000mA */
				axp_update(axp_charger->master, AXP_CHARGE_AC, tmp,0x07);
			}
			if (1 == axp_charger->usb_adapter_valid) {
				if((axp_config->pmu_usbcur) && (axp_config->pmu_usbcur_limit)){
					var = axp_config->pmu_usbcur;
					if(var < 500) {
						tmp = 0x00;   /* 100mA */
						axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
					} else if (var < 900) {
						tmp = 0x10;   /* 500mA */
						axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
					} else if (var < 1500) {
						tmp = 0x20;   /* 900mA */
						axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
					} else if (var < 2000) {
						tmp = 0x30;   /* 1500mA */
						axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
					} else if (var < 2500) {
						tmp = 0x40;   /* 2000mA */
						axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
					} else if (var < 3000) {
						tmp = 0x50;   /* 2500mA */
						axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
					} else if (var < 3500) {
						tmp = 0x60;   /* 3000mA */
						axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
					} else if (var < 4000) {
						tmp = 0x70;   /* 3500mA */
						axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
					} else {
						tmp = 0x80;   /* 4000mA */
						axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
					}
				} else {
						tmp = 0x50;   /* 2500mA */
						DBG_PSY_MSG(DEBUG_CHG, "%s: %d,set usbcur 2500 mA\n",__func__, __LINE__);
						axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
				}
			}
		}

		if(!vbus_curr_limit_debug){ //usb current not limit
			DBG_PSY_MSG(DEBUG_CHG, "vbus_curr_limit_debug = %d\n",vbus_curr_limit_debug);
			tmp = 0x03;   /* 2500mA */
			axp_update(axp_charger->master, AXP_CHARGE_AC, tmp,0xf0);
		}

		if(CHARGE_USB_20 == axp_usbvolflag){
			DBG_PSY_MSG(DEBUG_CHG, "set usbvol_pc %d mV\n",axp_config->pmu_usbvol_pc);
			if(axp_config->pmu_usbvol_pc){
				var = axp_config->pmu_usbvol_pc * 1000;
				if(var >= 4000000 && var <=4700000){
					tmp = (var - 4000000)/100000;
					val = tmp << 3;
					axp_update(axp_charger->master, AXP_CHARGE_VBUS, val,0x38);
				}else
					DBG_PSY_MSG(DEBUG_CHG, "set usb limit voltage error,%d mV\n",axp_config->pmu_usbvol_pc);
			}
		}else if(CHARGE_USB_30 == axp_usbvolflag) {
			val |= 7 << 3;
			axp_update(axp_charger->master, AXP_CHARGE_VBUS, val,0x38);
		}else {
			DBG_PSY_MSG(DEBUG_CHG, "set usbvol %d mV\n",axp_config->pmu_usbvol);
			if((axp_config->pmu_usbvol) && (axp_config->pmu_usbvol_limit)){
				var = axp_config->pmu_usbvol * 1000;
				if(var >= 4000000 && var <=4700000){
					tmp = (var - 4000000)/100000;
					val = tmp << 3;
					axp_update(axp_charger->master, AXP_CHARGE_AC, val,0x38);
				}else
					DBG_PSY_MSG(DEBUG_CHG, "set usb limit voltage error,%d mV\n",axp_config->pmu_usbvol);
			}
		}
	}
}

#else
static void axp_usb(struct work_struct *work)
{
	int var;
	uint8_t tmp,val;

	DBG_PSY_MSG(DEBUG_CHG, "[axp_usb]axp_usbcurflag = %d\n",axp_usbcurflag);

	axp_read(axp_charger->master, AXP_CHARGE_STATUS, &val);
	if((val & 0x10) == 0x00){/*usb or usb adapter can not be used*/
		DBG_PSY_MSG(DEBUG_CHG, "USB not insert!\n");
		axp_clr_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x02);
		axp_set_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x01);
	}else if(CHARGE_USB_20 == axp_usbcurflag){
		DBG_PSY_MSG(DEBUG_CHG, "set usbcur_pc %d mA\n",axp_config->pmu_usbcur_pc);
		if(axp_config->pmu_usbcur_pc){
			var = axp_config->pmu_usbcur_pc * 1000;
			if(var >= 900000)
				axp_clr_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x03);
			else if (var < 900000){
				axp_clr_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x02);
				axp_set_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x01);
			}
		}else//not limit
			axp_set_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x03);
	}else if (CHARGE_USB_30 == axp_usbcurflag){
		axp_clr_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x03);
	}else {
		DBG_PSY_MSG(DEBUG_CHG, "set usbcur %d mA\n",axp_config->pmu_usbcur);
		if((axp_config->pmu_usbcur) && (axp_config->pmu_usbcur_limit)){
			var = axp_config->pmu_usbcur * 1000;
			if(var >= 900000)
				axp_clr_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x03);
			else if (var < 900000){
				axp_clr_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x02);
				axp_set_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x01);
			}
		}else //not limit
			axp_set_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x03);
	}

	if(!vbus_curr_limit_debug){ //usb current not limit
		DBG_PSY_MSG(DEBUG_CHG, "vbus_curr_limit_debug = %d\n",vbus_curr_limit_debug);
		axp_set_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x03);
	}

	if(CHARGE_USB_20 == axp_usbvolflag){
		DBG_PSY_MSG(DEBUG_CHG, "set usbvol_pc %d mV\n",axp_config->pmu_usbvol_pc);
		if(axp_config->pmu_usbvol_pc){
			axp_set_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x40);
			var = axp_config->pmu_usbvol_pc * 1000;
			if(var >= 4000000 && var <=4700000){
				tmp = (var - 4000000)/100000;
				axp_read(axp_charger->master, AXP_CHARGE_VBUS,&val);
				val &= 0xC7;
				val |= tmp << 3;
				axp_write(axp_charger->master, AXP_CHARGE_VBUS,val);
			}else
				DBG_PSY_MSG(DEBUG_CHG, "set usb limit voltage error,%d mV\n",axp_config->pmu_usbvol_pc);
		}else
		    axp_clr_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x40);
	}else if(CHARGE_USB_30 == axp_usbvolflag) {
		axp_read(axp_charger->master, AXP_CHARGE_VBUS,&val);
		val &= 0xC7;
		val |= 7 << 3;
		axp_write(axp_charger->master, AXP_CHARGE_VBUS,val);
	}else {
		DBG_PSY_MSG(DEBUG_CHG, "set usbvol %d mV\n",axp_config->pmu_usbvol);
		if((axp_config->pmu_usbvol) && (axp_config->pmu_usbvol_limit)){
			axp_set_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x40);
			var = axp_config->pmu_usbvol * 1000;
			if(var >= 4000000 && var <=4700000){
				tmp = (var - 4000000)/100000;
				axp_read(axp_charger->master, AXP_CHARGE_VBUS,&val);
				val &= 0xC7;
				val |= tmp << 3;
				axp_write(axp_charger->master, AXP_CHARGE_VBUS,val);
			}else
				DBG_PSY_MSG(DEBUG_CHG, "set usb limit voltage error,%d mV\n",axp_config->pmu_usbvol);
		}else
		    axp_clr_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x40);
	}
}
#endif

void axp_usbac_checkst(struct axp_charger *charger)
{
	uint8_t val;

	if (axp_config->pmu_init_bc_en) {
		if(charger->in_short) {
			axp_read(axp_charger->master, AXP_BC_DET_STATUS, &val);
			spin_lock(&charger->charger_lock);
			if (0x20 == (val&0xe0)) {
				charger->usb_valid = 1;
			} else {
				charger->usb_valid = 0;
			}
			if (0x60 == (val&0xe0)) {
				charger->ac_valid = 1;
			} else {
				charger->ac_valid = 0;
			}
			spin_unlock(&charger->charger_lock);
		}
	}
}

void axp_usbac_in(struct axp_charger *charger)
{
	u32 var;
	u8 tmp;

	if (!axp_config->pmu_init_bc_en) {
		DBG_PSY_MSG(DEBUG_CHG, "axp ac/usb in!\n");
		if(timer_pending(&charger->usb_status_timer))
			del_timer_sync(&charger->usb_status_timer);
		/* must limit the current now,and will again fix it while usb/ac detect finished! */
		if(axp_config->pmu_usbcur_pc){
			var = axp_config->pmu_usbcur_pc;
			if(var < 500) {
				tmp = 0x00;   /* 100mA */
				axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
			} else if (var < 900) {
				tmp = 0x10;   /* 500mA */
				axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
			} else if (var < 1500) {
				tmp = 0x20;   /* 900mA */
				axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
			} else if (var < 2000) {
				tmp = 0x30;   /* 1500mA */
				axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
			} else if (var < 2500) {
				tmp = 0x40;   /* 2000mA */
				axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
			} else if (var < 3000) {
				tmp = 0x50;   /* 2500mA */
				axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
			} else if (var < 3500) {
				tmp = 0x60;   /* 3000mA */
				axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
			} else if (var < 4000) {
				tmp = 0x70;   /* 3500mA */
				axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
			} else {
				tmp = 0x80;   /* 4000mA */
				axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
			}
		} else {
			tmp = 0x10;   /* 500mA */
			axp_update(axp_charger->master, AXP_CHARGE_CONTROL3, tmp,0xf0);
		}
		/* this is about 3.5s,while the flag set in usb drivers after usb plugged */
		mod_timer(&charger->usb_status_timer,jiffies + msecs_to_jiffies(5000));
	}
	axp_usb_ac_check_status(charger);
}

void axp_usbac_out(struct axp_charger *charger)
{
	if (!axp_config->pmu_init_bc_en) {
		DBG_PSY_MSG(DEBUG_CHG, "axp22 ac/usb out!\n");
		if(timer_pending(&charger->usb_status_timer))
			del_timer_sync(&charger->usb_status_timer);
		/* if we plugged usb & ac at the same time,then unpluged ac quickly while the usb driver */
		/* do not finished detecting,the charger type is error!So delay the charger type report 2s */
		mod_timer(&charger->usb_status_timer,jiffies + msecs_to_jiffies(2000));
	}
	axp_usb_ac_check_status(charger);
}

int axp_chg_init(struct axp_charger *charger)
{
	int ret = 0;

	if (axp_config->pmu_init_bc_en) {
		/* enable BC */
		axp_set_bits(charger->master, AXP_BC_SET, 0x01);
	} else {
		setup_timer(&charger->usb_status_timer, axp_charger_update_usb_state, (unsigned long)charger);
		/* set usb cur-vol limit*/
		INIT_DELAYED_WORK(&(charger->usbwork), axp_usb);
		axp_usb_ac_check_status(charger);
		power_supply_changed(&charger->ac);
		power_supply_changed(&charger->usb);
		schedule_delayed_work(&(charger->usbwork), msecs_to_jiffies(30* 1000));
	}
	return ret;
}

void axp_chg_exit(struct axp_charger *charger)
{
	if (axp_config->pmu_init_bc_en) {
		/* enable BC */
		axp_clr_bits(charger->master, AXP_BC_SET, 0x01);
	} else {
		cancel_delayed_work_sync(&(charger->usbwork));
		del_timer_sync(&(charger->usb_status_timer));
	}
	return;
}

