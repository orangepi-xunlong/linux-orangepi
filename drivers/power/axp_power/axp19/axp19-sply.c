/*
 * Battery charger driver for Dialog Semiconductor DA9030
 *
 * Copyright (C) 2008 Compulab, Ltd.
 * 	Mike Rapoport <mike@compulab.co.il>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>

#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/slab.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/input.h>
#include <linux/mfd/axp-mfd.h>
#include "../axp-cfg.h"
#include "axp19-sply.h"

static uint8_t coulomb_flag;
static struct task_struct *main_task;
struct axp_charger *axp_charger;

int axp_usbvol(aw_charge_type type)
{
	return 0;
}
EXPORT_SYMBOL_GPL(axp_usbvol);

int axp_usbcur(aw_charge_type type)
{
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
	axp_read(axp_charger->master,AXP19_CHARGE_STATUS,&ret);
	if(ret & 0x10)/*usb or usb adapter can be used*/
		return 1;
	else/*no usb or usb adapter*/
		return 0;
}
EXPORT_SYMBOL_GPL(axp_usb_det);


static enum power_supply_property axp_battery_props[] = {
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
	POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
};

static enum power_supply_property axp_ac_props[] = {
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
};

static enum power_supply_property axp_usb_props[] = {
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
};

static void axp_battery_check_status(struct axp_charger *charger,
				    union power_supply_propval *val)
{
	if (charger->bat_det) {
		if (charger->is_on)
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else if( charger->rest_vol == 100 && charger->ext_valid)
			  val->intval = POWER_SUPPLY_STATUS_FULL;
		else if( charger->ext_valid )
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		else
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
	}
	else
		val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
}

static void axp_battery_check_health(struct axp_charger *charger,
				    union power_supply_propval *val)
{
    if (charger->fault & AXP19_FAULT_LOG_BATINACT)
		val->intval = POWER_SUPPLY_HEALTH_DEAD;
	else if (charger->fault & AXP19_FAULT_LOG_OVER_TEMP)
		val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
	else if (charger->fault & AXP19_FAULT_LOG_COLD)
		val->intval = POWER_SUPPLY_HEALTH_COLD;
	else
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
}

static int axp_battery_get_property(struct power_supply *psy,
				   enum power_supply_property psp,
				   union power_supply_propval *val)
{
	struct axp_charger *charger;
	int ret = 0;
	charger = container_of(psy, struct axp_charger, batt);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		axp_battery_check_status(charger, val);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		axp_battery_check_health(charger, val);
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = charger->battery_info->technology;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = charger->battery_info->voltage_max_design;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = charger->battery_info->voltage_min_design;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = charger->vbat * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = charger->ibat * 1000;
		break;
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = charger->batt.name;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		val->intval = charger->battery_info->charge_full_design;
        break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = charger->rest_vol;
		break;
	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW:
		if(charger->bat_det && !(charger->is_on) && !(charger->ext_valid))
			val->intval = charger->rest_time;
		else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		if(charger->bat_det && charger->is_on)
			val->intval = charger->rest_time;
		else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = (!charger->is_on)&&(charger->bat_det) && (! charger->ext_valid);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = charger->bat_det;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int axp_ac_get_property(struct power_supply *psy,
				   enum power_supply_property psp,
				   union power_supply_propval *val)
{
	struct axp_charger *charger;
	int ret = 0;
	charger = container_of(psy, struct axp_charger, ac);

	switch(psp){
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = charger->ac.name;break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = charger->ac_det;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = charger->ac_valid;break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = charger->vac * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = charger->iac * 1000;
		break;
	default:
		ret = -EINVAL;
		break;
	}
   return ret;
}

static int axp_usb_get_property(struct power_supply *psy,
				   enum power_supply_property psp,
				   union power_supply_propval *val)
{
	struct axp_charger *charger;
	int ret = 0;
	charger = container_of(psy, struct axp_charger, usb);

	switch(psp){
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = charger->usb.name;break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = charger->usb_det;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = charger->usb_valid;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = charger->vusb * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = charger->iusb * 1000;
		break;
	default:
		ret = -EINVAL;
		break;
	}
   return ret;
}

static char *supply_list[] = {
	"battery",
};

static void axp_battery_setup_psy(struct axp_charger *charger)
{
	struct power_supply *batt = &charger->batt;
	struct power_supply *ac = &charger->ac;
	struct power_supply *usb = &charger->usb;
	struct power_supply_info *info = charger->battery_info;

	batt->name = "battery";
	batt->use_for_apm = info->use_for_apm;
	batt->type = POWER_SUPPLY_TYPE_BATTERY;
	batt->get_property = axp_battery_get_property;

	batt->properties = axp_battery_props;
	batt->num_properties = ARRAY_SIZE(axp_battery_props);

	ac->name = "ac";
	ac->type = POWER_SUPPLY_TYPE_MAINS;
	ac->get_property = axp_ac_get_property;

	ac->supplied_to = supply_list,
	ac->num_supplicants = ARRAY_SIZE(supply_list),

	ac->properties = axp_ac_props;
	ac->num_properties = ARRAY_SIZE(axp_ac_props);

	usb->name = "usb";
	usb->type = POWER_SUPPLY_TYPE_USB;
	usb->get_property = axp_usb_get_property;

	usb->supplied_to = supply_list,
	usb->num_supplicants = ARRAY_SIZE(supply_list),

	usb->properties = axp_usb_props;
	usb->num_properties = ARRAY_SIZE(axp_usb_props);
};

static int axp_get_rdc(struct axp_charger *charger)
{
    uint8_t val[2];
    unsigned int i,temp,pre_temp;
    int averPreVol = 0, averPreCur = 0,averNextVol = 0,averNextCur = 0;

	axp_reads(charger->master,AXP19_DATA_BUFFER2,2,val);

	pre_temp = (((val[0] & 0x07) << 8 ) + val[1]);

	if(!charger->bat_det){
        return pre_temp;
	}
	if( charger->ext_valid){
		for(i = 0; i< AXP19_RDC_COUNT; i++){
            axp_charger_update(charger, &axp19_config);
			averPreVol += charger->vbat;
			averPreCur += charger->ibat;
			msleep(200);
        }
        averPreVol /= AXP19_RDC_COUNT;
        averPreCur /= AXP19_RDC_COUNT;
		axp_clr_bits(charger->master,AXP19_CHARGE_CONTROL1,0x80);
		msleep(3000);
		for(i = 0; i< AXP19_RDC_COUNT; i++){
            axp_charger_update(charger, &axp19_config);
			averNextVol += charger->vbat;
			averNextCur += charger->ibat;
			msleep(200);
        }
		averNextVol /= AXP19_RDC_COUNT;
		averNextCur /= AXP19_RDC_COUNT;
		axp_set_bits(charger->master,AXP19_CHARGE_CONTROL1,0x80);
		if(ABS(averPreCur - averNextCur) > 200){
            temp = 1000 * ABS(averPreVol - averNextVol) / ABS(averPreCur - averNextCur);
			if((temp < 5) || (temp > 5000)){
                return pre_temp;
			}
			else {
				temp += pre_temp;
				temp >>= 1;
				axp_write(charger->master,AXP19_DATA_BUFFER2,((temp & 0xFF00) | 0x800) >> 8);
				axp_write(charger->master,AXP19_DATA_BUFFER3,temp & 0x00FF);
				return temp;
			}
		}
		else {
			return pre_temp;
		}
	}
	else {
        return pre_temp;
	}
}

static int axp_bat_vol(bool Flag,int Bat_Vol,int Bat_Cur,uint16_t Rdc)
{
    if(Flag)
    {
        return Bat_Vol- (Bat_Cur*(int)Rdc/1000);
    }
    else
    {
        return Bat_Vol+ (Bat_Cur*(int)Rdc/1000);
    }
}

static int axp_get_coulomb(struct axp_charger *charger)
{
	uint64_t rValue1,rValue2,rValue;
	uint8_t IC_type;
	uint8_t temp[8];
	axp_read(charger->master,03, &temp[0]);
	if( (temp[0] & 0x0f) == 0x03){
		IC_type = 1;
	}
	else{
		IC_type = 0;
	}
	axp_reads(charger->master,AXP19_CCHAR3_RES,8,temp);
	if(IC_type){
		rValue1 = 65536 * ((((uint64_t)temp[0]) << 24) + (((uint64_t)temp[1]) << 16) +
		(((uint64_t)temp[2]) << 8) + ((uint64_t)temp[3]));
    rValue2 = 65536 * ((((uint64_t)temp[4] )<< 24) + (((uint64_t)temp[5]) << 16) +
		(((uint64_t)temp[6]) << 8) + ((uint64_t)temp[7]));
	}
	else{
		rValue1 = ((((uint64_t)temp[0]) << 24) + (((uint64_t)temp[1]) << 16) +
		(((uint64_t)temp[2]) << 8) + ((uint64_t)temp[3]));
    rValue2 = ((((uint64_t)temp[4] )<< 24) + (((uint64_t)temp[5]) << 16) +
		(((uint64_t)temp[6]) << 8) + ((uint64_t)temp[7]));
	}
    if(rValue1 > rValue2){
	coulomb_flag = 1;
        rValue = rValue1 - rValue2 ;
    }
    else{
        coulomb_flag = 0;
        rValue = rValue2 - rValue1 ;
    }

    return (int) rValue /charger->sample_time/ 3600 / 2;
}

static uint8_t axp_vol_rate(int Bat_Ocv_Vol)
{
    if(Bat_Ocv_Vol > FUELGUAGE_TOP_VOL)         //4160
    {
        return FUELGUAGE_TOP_LEVEL;
    }
    else if(Bat_Ocv_Vol < FUELGUAGE_LOW_VOL)    //<3400
    {
        return FUELGUAGE_LOW_LEVEL;
    }
    else if(Bat_Ocv_Vol < FUELGUAGE_VOL1)       //3500
    {
        return FUELGUAGE_LOW_LEVEL + (FUELGUAGE_LEVEL1 - FUELGUAGE_LOW_LEVEL) * ((int)Bat_Ocv_Vol - FUELGUAGE_LOW_VOL) / (FUELGUAGE_VOL1 - FUELGUAGE_LOW_VOL);
    }
    else if(Bat_Ocv_Vol < FUELGUAGE_VOL2)       //3600
    {
        return FUELGUAGE_LEVEL1 + (FUELGUAGE_LEVEL2 - FUELGUAGE_LEVEL1) * ((int)Bat_Ocv_Vol - FUELGUAGE_VOL1) / (FUELGUAGE_VOL2 - FUELGUAGE_VOL1);
    }
    else if(Bat_Ocv_Vol < FUELGUAGE_VOL3)       //3700
    {
        return FUELGUAGE_LEVEL2 + (FUELGUAGE_LEVEL3 - FUELGUAGE_LEVEL2) * ((int)Bat_Ocv_Vol - FUELGUAGE_VOL2) / (FUELGUAGE_VOL3 - FUELGUAGE_VOL2);
    }
    else if(Bat_Ocv_Vol < FUELGUAGE_VOL4)       //3800
    {
        return FUELGUAGE_LEVEL3 + (FUELGUAGE_LEVEL4 - FUELGUAGE_LEVEL3) * ((int)Bat_Ocv_Vol - FUELGUAGE_VOL3) / (FUELGUAGE_VOL4 - FUELGUAGE_VOL3);
    }
    else if(Bat_Ocv_Vol < FUELGUAGE_VOL5)       //3900
    {
        return FUELGUAGE_LEVEL4 + (FUELGUAGE_LEVEL5 - FUELGUAGE_LEVEL4) * ((int)Bat_Ocv_Vol - FUELGUAGE_VOL4) / (FUELGUAGE_VOL5 - FUELGUAGE_VOL4);
    }
    else if(Bat_Ocv_Vol < FUELGUAGE_VOL6)       //4000
    {
        return FUELGUAGE_LEVEL5 + (FUELGUAGE_LEVEL6 - FUELGUAGE_LEVEL5) * ((int)Bat_Ocv_Vol - FUELGUAGE_VOL5) / (FUELGUAGE_VOL6 - FUELGUAGE_VOL5);
    }
    else if(Bat_Ocv_Vol < FUELGUAGE_VOL7)       //4100
    {
        return FUELGUAGE_LEVEL6 + (FUELGUAGE_LEVEL7 - FUELGUAGE_LEVEL6) * ((int)Bat_Ocv_Vol - FUELGUAGE_VOL6) / (FUELGUAGE_VOL7 - FUELGUAGE_VOL6);
    }
    else if(Bat_Ocv_Vol < FUELGUAGE_TOP_VOL)    //4100
    {
        return FUELGUAGE_LEVEL7 + (FUELGUAGE_TOP_LEVEL - FUELGUAGE_LEVEL7) * ((int)Bat_Ocv_Vol - FUELGUAGE_VOL7) / (FUELGUAGE_TOP_VOL - FUELGUAGE_VOL7);
    }
    else
    {
        return 0;
    }
}


static int axp_cal_resttime(struct axp_charger *charger,uint8_t chg_status, uint16_t Bat_Ocv_Vol, uint16_t Rdc)
{
	uint8_t Tcv_Rest_Vol = 0;
    uint16_t Iconst_current = 1;
    unsigned int rest_time = 0;
	int Tcv = 0;
	if(charger->ibat == 0)  charger->ibat = 1;
	if(chg_status > 0x03){
		if(charger->vbat < 4195){
			Tcv_Rest_Vol = axp_vol_rate(4200 - charger->ibat *(int) Rdc / 1000);
			Iconst_current = charger->ibat;
			if(Tcv_Rest_Vol < 70){
				Tcv = 60 * (100 - (int)Tcv_Rest_Vol) * charger->battery_info->energy_full_design/ (45 * charger->ibat);
			}
			else{
                Tcv = 60 * (100 - (int)Tcv_Rest_Vol) * charger->battery_info->energy_full_design / (35 * charger->ibat);
			}
			rest_time = 6 * charger->battery_info->energy_full_design * ABS(Tcv_Rest_Vol - charger->rest_vol) / charger->ibat / 10 + Tcv ;
		}
		else{
			if(Iconst_current == 1){
				Iconst_current = charger->chgcur;
			}
			if(Tcv == 0){
				Tcv_Rest_Vol =axp_vol_rate(4200 - charger->chgcur * (int)Rdc / 1000);
				if(Tcv_Rest_Vol < 70){
                    Tcv = 60 * (100 - (int)Tcv_Rest_Vol) * charger->chgcur / (45 * charger->chgcur);
                }
                else{
                    Tcv = 60 * (100 - (int)Tcv_Rest_Vol) * charger->chgcur / (35 * charger->chgcur);
                }
			}
			if(charger->ibat < (charger->chgcur *charger->chgend/100)){
                rest_time = 1;
            }
            else{
                rest_time = (unsigned int)Tcv * (90 + 110 * (charger->chgcur *charger->chgend/100) / (unsigned int)charger->ibat) * (90 +100 * (charger->chgcur *charger->chgend/100)
					/ (unsigned int)charger->ibat) * ABS(charger->ibat - (charger->chgcur *charger->chgend/100)) / (unsigned int)Iconst_current /10000;
            }
		}
	}
	else  //�ŵ�
    {
        __u8  End_Vol_Rate = axp_vol_rate(END_VOLTAGE_APS + (charger->ibat * ((int)Rdc + 110) / 1000));

        if(charger->pbat)
        {
            rest_time = BAT_AVER_VOL * charger->battery_info->energy_full_design
				* ABS(charger->rest_vol- (int)End_Vol_Rate) / charger->pbat * 6 / 10  ;
        }
        if(Bat_Ocv_Vol)
        {
            rest_time *= charger->vbat;                                            //��OCV��������
            rest_time /= (unsigned int)Bat_Ocv_Vol;

        }
        rest_time *= 100;           //�Ե�ص�ѹ��ͺ�Ч����ߵ�����
        rest_time /= 99;

    }

    return rest_time;
}



static int axp_main_task(void *arg)
{
    struct axp_charger *charger = arg;
	uint8_t temp_value[8];
    uint8_t Bat_Cap_Buffer[AXP19_VOL_MAX];
    uint16_t Bat_Time_Buffer[AXP19_TIME_MAX];
    uint32_t Bat_Power_Buffer[AXP19_AVER_MAX];
    int     Cur_CoulombCounter;
    uint8_t  Pre_rest_cap=0,Pre_ocv_rest_cap=0,Pre_Cur_Cal_rest_cap=0;
    uint16_t		Bat_Rdc,Bat_Vol,Bat_Ocv_Vol;
	uint16_t		i = 0,j = 0,k = 0,m = 0;
	uint32_t		Total_Cap = 0,Total_Time = 0,Total_Power = 0;
	uint8_t		Rdc_Flag = 0,Pre_Rdc_Flag = 0;
    uint8_t		Cou_Correction_Flag = 0;
	uint8_t		Real_Cou_Flag = 0;
	int rt_rest_vol, ocv_rest_vol, cou_rest_vol;
	uint8_t rt_charge_status;
	bool status_usb, pre_status_usb;
    bool status_ac, pre_status_ac;
    bool status_bat, pre_status_bat;
    int pre_rest_vol;
    unsigned long power_sply = 0;
    pre_rest_vol = 0;
	status_usb = 0;
    pre_status_usb = 0;
    status_ac = 0;
    pre_status_ac = 0;
    status_bat = 0;
    pre_status_bat =0;

	axp_write(charger->master,AXP19_TIMER_CTL,0x80);
	axp_reads(charger->master,AXP19_DATA_BUFFER1,2,temp_value);
	Real_Cou_Flag = (temp_value[0] & 0x80);

	if(Real_Cou_Flag)
		charger->battery_info->energy_full_design=5 * (((temp_value[0] & 0x7f) << 4) + ((temp_value[1] & 0xf0) >> 4));
	axp_reads(charger->master,AXP19_DATA_BUFFER2,2,temp_value);
	Bat_Rdc = ((temp_value[0] & 0x07) << 8) + temp_value[1];

    Pre_Rdc_Flag = temp_value[0] & 0x08;
	if(Pre_Rdc_Flag){
      //  Bat_Rdc = (Bat_Rdc & 0x7ff) * 3;
    }
    else{
        Bat_Rdc = 250;
    }

	memset(Bat_Cap_Buffer, 0, sizeof(Bat_Cap_Buffer));
	memset(Bat_Time_Buffer, 0, sizeof(Bat_Time_Buffer));
	memset(Bat_Power_Buffer, 0, sizeof(Bat_Power_Buffer));

	while(1){
		if(kthread_should_stop()) break;
        axp_charger_update_state(charger);
        axp_charger_update(charger, &axp19_config);

		status_ac = charger->ac_valid;
		status_usb = charger->usb_valid;
        status_bat = (!charger->is_on)&&(charger->bat_det);

        if(status_usb != pre_status_usb || status_ac != pre_status_ac || status_bat != pre_status_bat )
        {
            power_supply_changed(&charger->batt);
			pre_status_ac =  status_ac;
			pre_status_usb = status_usb;
			pre_status_bat = status_bat;
         }

		if(charger->bat_current_direction && charger->is_on \
			&& (charger->ibat > 100) && (!Rdc_Flag)){
            if(Pre_Rdc_Flag){
                Bat_Rdc += axp_get_rdc(charger);
                Bat_Rdc /= 2;
            }
            else{
                Bat_Rdc = axp_get_rdc(charger);
            }

            Rdc_Flag = 1;

		}
		charger->pbat = charger->ibat * charger->vbat;
		Total_Power -= Bat_Power_Buffer[m];
		Bat_Power_Buffer[m] = charger->pbat;
		Total_Power += Bat_Power_Buffer[m];
		m++;
		if(m == AXP19_AVER_MAX)
		{
			m = 0;
		}
        charger->pbat = (int)Total_Power / AXP19_AVER_MAX;
		Bat_Vol = (uint16_t)charger->vbat;
        Bat_Ocv_Vol =(uint16_t) axp_bat_vol(charger->ext_valid && charger->bat_current_direction,\
			(int) Bat_Vol,charger->ibat,Bat_Rdc);//��ȡ��·��ѹ
        rt_rest_vol = axp_vol_rate( Bat_Ocv_Vol);
		rt_charge_status = (charger->ext_valid << 2 )| (charger->bat_det << 1) | \
			(charger->is_on);
		Total_Cap -= Bat_Cap_Buffer[i];
		Bat_Cap_Buffer[i] = rt_rest_vol;
		Total_Cap += Bat_Cap_Buffer[i];
		i++;
		if(i == AXP19_VOL_MAX){
		    i = 0;
		}
		if(j < AXP19_VOL_MAX){
			j++;
		}
		ocv_rest_vol = Total_Cap / j;

		if((j == AXP19_VOL_MAX) && (charger->bat_det == 1)){
            Cur_CoulombCounter = axp_get_coulomb(charger);
			 if((ocv_rest_vol < 10) && Rdc_Flag && (rt_charge_status == 7) \
				&& (!Cou_Correction_Flag))    {
                 Cou_Correction_Flag = 0x01;
                 axp_set_bits(charger->master,AXP19_COULOMB_CONTROL,AXP19_COULOMB_CLEAR);
                 Pre_rest_cap = ocv_rest_vol;
                 Pre_Cur_Cal_rest_cap = ocv_rest_vol;
             }
             if(Cou_Correction_Flag && (rt_charge_status == 6) && (ocv_rest_vol == 100)){
                 charger->battery_info->energy_full_design = Cur_CoulombCounter;
                 charger->battery_info->energy_full_design *= 100;
                 charger->battery_info->energy_full_design /= (100 - (int)Pre_Cur_Cal_rest_cap);
                 temp_value[0] = ((((charger->battery_info->energy_full_design /5) & 0xff0) | 0x800) >> 4);
                 temp_value[1] &= 0x0f;
                 temp_value[1] |= (((charger->battery_info->energy_full_design /5) & 0x0f) << 4) ;
                 axp_write(charger->master,AXP19_DATA_BUFFER1,temp_value[0]);
                 axp_write(charger->master,AXP19_DATA_BUFFER1,temp_value[1] );
                 Cou_Correction_Flag = 0x00;
                 Real_Cou_Flag = 0x01;
            }
			if(coulomb_flag){  //���
                cou_rest_vol = (Pre_rest_cap + (100 * Cur_CoulombCounter /
					charger->battery_info->energy_full_design));
            }
            else{//�ŵ�
                if(Pre_rest_cap < (100 * Cur_CoulombCounter /
					charger->battery_info->energy_full_design)){
                    cou_rest_vol = 0;
                }
                else{
                    cou_rest_vol = ((int)Pre_rest_cap - (100 * Cur_CoulombCounter /
						charger->battery_info->energy_full_design));
                }
            }
			if(((ocv_rest_vol > Pre_ocv_rest_cap) && (rt_charge_status < 0x04))
				|| (ocv_rest_vol < (Pre_ocv_rest_cap - 2))){//�ŵ�ʱ������������
                ocv_rest_vol = (int)Pre_ocv_rest_cap;
            }
            else if(((ocv_rest_vol < Pre_ocv_rest_cap) && (rt_charge_status > 0x03))
				||(ocv_rest_vol > (Pre_ocv_rest_cap + 2))){//���ʱ�������ܼ���
                ocv_rest_vol = (int)Pre_ocv_rest_cap;
            }
            Pre_ocv_rest_cap = (uint8_t)ocv_rest_vol;

			if(cou_rest_vol > 100){
                if(Real_Cou_Flag){
                    charger->rest_vol = ocv_rest_vol  + (3 * 100);   //�������У�����������������ص�������ռ3/4������1/4
                }
                else{
                    charger->rest_vol = 2 * ocv_rest_vol  + 200;
                }
            }
            else{
                if(Real_Cou_Flag)
                    charger->rest_vol = ocv_rest_vol  + (3 * cou_rest_vol);
                else
                    charger->rest_vol = 2 * ocv_rest_vol  + 2 * cou_rest_vol;
            }

            charger->rest_vol /= 4;
		/*when charging , capacity is less than 100 */
		if (charger->rest_vol >= 99 && charger->is_on == 1 )
			charger->rest_vol = 99;


		if(((charger->rest_vol > pre_rest_vol) && (rt_charge_status < 0x04))){//�ŵ�ʱ������������
			charger->rest_vol = pre_rest_vol;
		}
		else if((charger->rest_vol < pre_rest_vol) && (rt_charge_status > 0x03)){//���ʱ�������ܼ���
			charger->rest_vol = pre_rest_vol;
		}
            charger->rest_time = axp_cal_resttime(charger,rt_charge_status,Bat_Ocv_Vol,Bat_Rdc);

            Total_Time -= Bat_Time_Buffer[k];
            Bat_Time_Buffer[k] = charger->rest_time;
            Total_Time += Bat_Time_Buffer[k];
            k++;
            if(k == AXP19_TIME_MAX){
                k = 0;
            }
            charger->rest_time = Total_Time / AXP19_TIME_MAX;
		}
		else if(j < AXP19_VOL_MAX){
			charger->rest_vol = ocv_rest_vol;
            Pre_rest_cap = ocv_rest_vol;
            Pre_ocv_rest_cap = ocv_rest_vol;
			//pre_rest_vol = charger->rest_vol;
			cou_rest_vol = 0;
            if(j == AXP19_VOL_MAX - 1){
		axp_set_bits(charger->master,AXP19_COULOMB_CONTROL,0xA0);
            }
        }
	/* if battery volume changed, inform uevent */
        if(charger->rest_vol - pre_rest_vol){
			printk("battery vol change: %d, %d \n", pre_rest_vol, charger->rest_vol);
			pre_rest_vol = charger->rest_vol;
			power_supply_changed(&charger->batt);
        }

	if (axp_debug) {
		DBG_PSY_MSG(DEBUG_SPLY, "charger->ic_temp = %d\n",charger->ic_temp);
		DBG_PSY_MSG(DEBUG_SPLY, "charger->bat_temp = %d\n",charger->bat_temp);
		DBG_PSY_MSG(DEBUG_SPLY, "charger->vbat = %d\n",charger->vbat);
		DBG_PSY_MSG(DEBUG_SPLY, "charger->ibat = %d\n",charger->ibat);
		DBG_PSY_MSG(DEBUG_SPLY, "charger->disvbat = %d\n",charger->disvbat);
		DBG_PSY_MSG(DEBUG_SPLY, "charger->disibat = %d\n",charger->disibat);
		power_sply = charger->disvbat * charger->disibat;
		if (0 != power_sply)
			power_sply = power_sply/1000;
		DBG_PSY_MSG(DEBUG_SPLY, "power_sply = %ld mW\n",power_sply);
		DBG_PSY_MSG(DEBUG_SPLY, "charger->is_on = %d\n",charger->is_on);
		DBG_PSY_MSG(DEBUG_SPLY, "charger->rest_vol = %d\n",charger->rest_vol);
		DBG_PSY_MSG(DEBUG_SPLY, "charger->bat_current_direction = %d\n",charger->bat_current_direction);
		DBG_PSY_MSG(DEBUG_SPLY, "charger->charge_on = %d\n",charger->charge_on);
		DBG_PSY_MSG(DEBUG_SPLY, "charger->ext_valid = %d\n",charger->ext_valid);
		ssleep(5);
	} else
		ssleep(1);

    }
	return 0;
}

/*
static void axp_charging_monitor(struct work_struct *work)
{
	struct axp_charger *charger;

	charger = container_of(work, struct axp_charger, work.work);

	axp_charger_update_state(charger);
	axp_charger_update(charger, &axp19_config);


	schedule_delayed_work(&charger->work, charger->interval);
}
*/
static int axp_battery_probe(struct platform_device *pdev)
{
	struct axp_charger *charger;
	struct axp_supply_init_data *pdata = pdev->dev.platform_data;
	int ret;

	if (pdata == NULL)
		return -EINVAL;

	if (pdata->chgcur > 1600 ||
	    pdata->chgvol < 4100 ||
	    pdata->chgvol > 4360){
            printk("charger milliamp is too high or target voltage is over range\n");
		    return -EINVAL;
		}

	if (pdata->chgpretime < 30 || pdata->chgpretime >60 ||
		pdata->chgcsttime < 420 || pdata->chgcsttime > 600){
            printk("prechaging time or constant current charging time is over range\n");
		    return -EINVAL;
		}

	charger = kzalloc(sizeof(*charger), GFP_KERNEL);
	if (charger == NULL)
		return -ENOMEM;

	charger->master = pdev->dev.parent;

	charger->chgcur      = pdata->chgcur;
	charger->chgvol     = pdata->chgvol;
	charger->chgend           = pdata->chgend; //axp199
	charger->sample_time          = pdata->sample_time;
	charger->chgen                   = pdata->chgen;
	charger->chgpretime      = pdata->chgpretime;
	charger->chgcsttime = pdata->chgcsttime;
	charger->battery_info         = pdata->battery_info;
	charger->battery_low          = pdata->battery_low;
	charger->battery_critical     = pdata->battery_critical;

	axp_charger = charger;

	ret = axp19_init(charger);
	if (ret) {
		goto err_ps_register;
	}

	axp_battery_setup_psy(charger);
	ret = power_supply_register(&pdev->dev, &charger->batt);
	if (ret)
		goto err_ps_register;

	ret = power_supply_register(&pdev->dev, &charger->ac);
	if (ret){
		power_supply_unregister(&charger->batt);
		goto err_ps_register;
	}
	ret = power_supply_register(&pdev->dev, &charger->usb);
	if (ret){
		power_supply_unregister(&charger->ac);
		power_supply_unregister(&charger->batt);
		goto err_ps_register;
	}

	ret = axp_charger_create_attrs(&charger->batt);
	if(ret){
		return ret;
	}


	platform_set_drvdata(pdev, charger);
	main_task = kthread_run(axp_main_task,charger,"kaxp19");
	if(IS_ERR(main_task)){
		printk("Unable to start main task.\n");
		ret = PTR_ERR(main_task);
		main_task = NULL;

		return ret;
	}
/*
		charger->interval = msecs_to_jiffies(1 * 1000);

		INIT_DELAYED_WORK(&charger->work, axp_charging_monitor);
		schedule_delayed_work(&charger->work, charger->interval);
*/
	ret = axp_irq_init(charger, pdev);
	if(ret){
		printk("cat notaxp_charger_create_attrs!!!===\n ");
		return ret;
	}

	class_register(&axppower_class);

    return ret;


err_ps_register:
	axp19_exit(charger);
	kfree(charger);

	return ret;
}

static int axp_battery_remove(struct platform_device *dev)
{
	struct axp_charger *charger = platform_get_drvdata(dev);

	if(main_task){

                kthread_stop(main_task);

                main_task = NULL;

    }

	axp_irq_exit(charger);
	cancel_delayed_work(&charger->work);
	power_supply_unregister(&charger->usb);
	power_supply_unregister(&charger->ac);
	power_supply_unregister(&charger->batt);

	axp19_exit(charger);
	kfree(charger);

	return 0;
}

static struct platform_driver axp_battery_driver = {
	.driver	= {
		.name	= "axp19-supplyer",
		.owner	= THIS_MODULE,
	},
	.probe = axp_battery_probe,
	.remove = axp_battery_remove,
};

static int axp_battery_init(void)
{
	return platform_driver_register(&axp_battery_driver);
}

static void axp_battery_exit(void)
{
	platform_driver_unregister(&axp_battery_driver);
}

module_init(axp_battery_init);
module_exit(axp_battery_exit);

MODULE_DESCRIPTION("AXP19 battery charger driver");
MODULE_AUTHOR("Kyle Cheung");
MODULE_LICENSE("GPL");
