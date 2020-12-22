/*
 * Battery charger driver for allwinnertech AXP81X
 *
 * Copyright (C) 2014 ALLWINNERTECH.
 *  Ming Li <liming@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/reboot.h>
#include <linux/delay.h>
#include "../axp-cfg.h"
#include "axp20-sply.h"
#include <linux/mfd/axp-mfd.h>
int use_cou = 0;

void axp20_power_off(int power_start)
{
	uint8_t val;
	struct axp_dev *axp;
	axp = axp_dev_lookup(AXP20);
	if (NULL == axp) {
		printk("%s: axp data is null\n", __func__);
		return;
	}
	if(axp20_config.pmu_pwroff_vol >= 2600 && axp20_config.pmu_pwroff_vol <= 3300){
		if (axp20_config.pmu_pwroff_vol > 3200){
			val = 0x7;
		}
		else if (axp20_config.pmu_pwroff_vol > 3100){
			val = 0x6;
		}
		else if (axp20_config.pmu_pwroff_vol > 3000){
			val = 0x5;
		}
		else if (axp20_config.pmu_pwroff_vol > 2900){
			val = 0x4;
		}
		else if (axp20_config.pmu_pwroff_vol > 2800){
			val = 0x3;
		}
		else if (axp20_config.pmu_pwroff_vol > 2700){
			val = 0x2;
		}
		else if (axp20_config.pmu_pwroff_vol > 2600){
			val = 0x1;
		}
		else
			val = 0x0;

		axp_update(axp->dev, POWER20_VOFF_SET, val, 0x7);
	}
	val = 0xff;
	if (!use_cou){
		axp_read(axp->dev, POWER20_COULOMB_CTL, &val);
		val &= 0x3f;
		axp_write(axp->dev, POWER20_COULOMB_CTL, val);
		val |= 0x80;
		val &= 0xbf;
		axp_write(axp->dev, POWER20_COULOMB_CTL, val);
	}
	//led auto
	axp_clr_bits(axp->dev,0x32,0x38);
	axp_clr_bits(axp->dev,0xb9,0x80);

	printk("[axp] send power-off command!\n");
	mdelay(20);
	if(axp20_config.power_start != 1){
		axp_write(axp->dev, POWER20_INTSTS3, 0x03);
		axp_read(axp->dev, POWER20_STATUS, &val);
		if(val & 0xF0){
			axp_read(axp->dev, POWER20_MODE_CHGSTATUS, &val);
		    	if(val & 0x20){
				printk("[axp] set flag!\n");
		        	axp_write(axp->dev, POWER20_DATA_BUFFERC, 0x0f);
	            		mdelay(20);
			    	printk("[axp] reboot!\n");
			    	machine_restart(NULL);
			    	printk("[axp] warning!!! arch can't ,reboot, maybe some error happend!\n");
		    	}
		}
	}
	axp_write(axp->dev, POWER20_DATA_BUFFERC, 0x00);
	//axp_write(&axp->dev, 0xf4, 0x06);
	//axp_write(&axp->dev, 0xf2, 0x04);
	//axp_write(&axp->dev, 0xff, 0x01);
	//axp_write(&axp->dev, 0x04, 0x01);
	//axp_clr_bits(&axp->dev, 0x03, 0xc0);
	//axp_write(&axp->dev, 0xff, 0x00);
	//mdelay(20);
	axp_set_bits(axp->dev, POWER20_OFF_CTL, 0x80);
	mdelay(20);
	printk("[axp] warning!!! axp can't power-off, maybe some error happend!\n");
}

