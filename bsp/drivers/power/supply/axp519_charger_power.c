/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include "axp519_charger_power.h"

enum usb_state {
	USB_STATE_DISCONNECTED = 0,
	USB_STATE_CONNECTED,
	USB_STATE_CONFIGURED,
};

struct axp519_power {
	char                      	*name;
	struct device             	*dev;
	struct regmap             	*regmap;
	struct power_supply       	*charger_supply;
	struct bmu_ext_config_info  dts_info;
	struct delayed_work         charger_supply_mon;
	struct delayed_work         usb_chg_state;
	int 						vbus_set_limit;
	int 						charge_set_limit;
	int 						battery_exist;

	int 						charge_done;
	int 						health;
	int 						vbus_set_vol_limit;

	struct power_supply         *bat_supply;
	struct device_node          *bat_supply_np;

	int set_offline;
	struct delayed_work         vbus_exist_mon;
	enum usb_state              usb_state;
	enum usb_state              prev_state;
};

static enum power_supply_property axp519_charger_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
	POWER_SUPPLY_PROP_MANUFACTURER,

	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
	POWER_SUPPLY_PROP_CALIBRATE,
	POWER_SUPPLY_PROP_TEMP_ALERT_MIN,
	POWER_SUPPLY_PROP_TEMP_ALERT_MAX,
	POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MIN,
	POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MAX,
};

static const char * const usb_state_name[] = {
	[USB_STATE_DISCONNECTED]	= "USB_STATE=DISCONNECTED",
	[USB_STATE_CONNECTED]		= "USB_STATE=CONNECTED",
	[USB_STATE_CONFIGURED]		= "USB_STATE=CONFIGURED",
};

static int axp519_charger_input_limit(struct axp519_power *charger_power, int mA)
{
	struct bmu_ext_config_info *bmp_dts_info = &charger_power->dts_info;
	int input_vol, input_cur;

	input_vol = charger_power->vbus_set_vol_limit;
	input_cur = mA;

	if (input_cur * input_vol > bmp_dts_info->bmu_ext_max_power)
		input_cur = bmp_dts_info->bmu_ext_max_power / input_vol;

	PMIC_DEBUG("input_cur:%d input_vol:%d max_power:%d\n", input_cur, input_vol, bmp_dts_info->bmu_ext_max_power);

	return input_cur;
}


static inline int axp_vts_to_temp(int data,
		const struct bmu_ext_config_info *bmp_dts_info)
{
	int temp;

	if (!bmp_dts_info->pmu_bat_temp_enable)
		return 300;
	else if (data < bmp_dts_info->pmu_bat_temp_para16)
		return 650;
	else if (data <= bmp_dts_info->pmu_bat_temp_para15) {
		temp = 550 + (bmp_dts_info->pmu_bat_temp_para15 - data) * 100/
		(bmp_dts_info->pmu_bat_temp_para15 - bmp_dts_info->pmu_bat_temp_para16);
	} else if (data <= bmp_dts_info->pmu_bat_temp_para14) {
		temp = 500 + (bmp_dts_info->pmu_bat_temp_para14 - data) * 50/
		(bmp_dts_info->pmu_bat_temp_para14 - bmp_dts_info->pmu_bat_temp_para15);
	} else if (data <= bmp_dts_info->pmu_bat_temp_para13) {
		temp = 450 + (bmp_dts_info->pmu_bat_temp_para13 - data) * 50/
		(bmp_dts_info->pmu_bat_temp_para13 - bmp_dts_info->pmu_bat_temp_para14);
	} else if (data <= bmp_dts_info->pmu_bat_temp_para12) {
		temp = 400 + (bmp_dts_info->pmu_bat_temp_para12 - data) * 50/
		(bmp_dts_info->pmu_bat_temp_para12 - bmp_dts_info->pmu_bat_temp_para13);
	} else if (data <= bmp_dts_info->pmu_bat_temp_para11) {
		temp = 300 + (bmp_dts_info->pmu_bat_temp_para11 - data) * 100/
		(bmp_dts_info->pmu_bat_temp_para11 - bmp_dts_info->pmu_bat_temp_para12);
	} else if (data <= bmp_dts_info->pmu_bat_temp_para10) {
		temp = 250 + (bmp_dts_info->pmu_bat_temp_para10 - data) * 50/
		(bmp_dts_info->pmu_bat_temp_para10 - bmp_dts_info->pmu_bat_temp_para11);
	} else if (data <= bmp_dts_info->pmu_bat_temp_para9) {
		temp = 200 + (bmp_dts_info->pmu_bat_temp_para9 - data) * 50/
		(bmp_dts_info->pmu_bat_temp_para9 - bmp_dts_info->pmu_bat_temp_para10);
	} else if (data <= bmp_dts_info->pmu_bat_temp_para8) {
		temp = 150 + (bmp_dts_info->pmu_bat_temp_para8 - data) * 50/
		(bmp_dts_info->pmu_bat_temp_para8 - bmp_dts_info->pmu_bat_temp_para9);
	} else if (data <= bmp_dts_info->pmu_bat_temp_para7) {
		temp = 100 + (bmp_dts_info->pmu_bat_temp_para7 - data) * 50/
		(bmp_dts_info->pmu_bat_temp_para7 - bmp_dts_info->pmu_bat_temp_para8);
	} else if (data <= bmp_dts_info->pmu_bat_temp_para6) {
		temp = 50 + (bmp_dts_info->pmu_bat_temp_para6 - data) * 50/
		(bmp_dts_info->pmu_bat_temp_para6 - bmp_dts_info->pmu_bat_temp_para7);
	} else if (data <= bmp_dts_info->pmu_bat_temp_para5) {
		temp = 0 + (bmp_dts_info->pmu_bat_temp_para5 - data) * 50/
		(bmp_dts_info->pmu_bat_temp_para5 - bmp_dts_info->pmu_bat_temp_para6);
	} else if (data <= bmp_dts_info->pmu_bat_temp_para4) {
		temp = -50 + (bmp_dts_info->pmu_bat_temp_para4 - data) * 50/
		(bmp_dts_info->pmu_bat_temp_para4 - bmp_dts_info->pmu_bat_temp_para5);
	} else if (data <= bmp_dts_info->pmu_bat_temp_para3) {
		temp = -100 + (bmp_dts_info->pmu_bat_temp_para3 - data) * 50/
		(bmp_dts_info->pmu_bat_temp_para3 - bmp_dts_info->pmu_bat_temp_para4);
	} else if (data <= bmp_dts_info->pmu_bat_temp_para2) {
		temp = -150 + (bmp_dts_info->pmu_bat_temp_para2 - data) * 50/
		(bmp_dts_info->pmu_bat_temp_para2 - bmp_dts_info->pmu_bat_temp_para3);
	} else if (data <= bmp_dts_info->pmu_bat_temp_para1) {
		temp = -250 + (bmp_dts_info->pmu_bat_temp_para1 - data) * 100/
		(bmp_dts_info->pmu_bat_temp_para1 - bmp_dts_info->pmu_bat_temp_para2);
	} else
		temp = -250;
	return temp;
}


static int axp519_battery_power_get(struct axp519_power *charger_power)
{
	struct device_node *np = NULL;
	int ret = 0;

	if (charger_power->bat_supply && (!IS_ERR(charger_power->bat_supply)))
		return ret;

	if (of_find_property(charger_power->dev->of_node, "det_battery_supply", NULL)) {
		np = of_parse_phandle(charger_power->dev->of_node, "det_battery_supply", 0);
		if (!of_device_is_available(np)) {
			charger_power->bat_supply =  NULL;
			charger_power->bat_supply_np =  NULL;
			ret = -1;
		} else {
			charger_power->bat_supply = devm_power_supply_get_by_phandle(charger_power->dev,
							"det_battery_supply");
			charger_power->bat_supply_np = np;
			if (!(charger_power->bat_supply) || (IS_ERR(charger_power->bat_supply))) {
				charger_power->bat_supply =  NULL;
				charger_power->bat_supply_np =  NULL;
				ret = -1;
			}
		}
	} else {
			charger_power->bat_supply =  NULL;
			charger_power->bat_supply_np =  NULL;
			ret = -1;
	}

	return ret;
}

static int axp519_ntc_set(struct axp519_power *charger_power)
{
	struct bmu_ext_config_info *bmp_dts_info = &charger_power->dts_info;
	struct regmap *regmap = charger_power->regmap;
	int reg_val = 0, temp;
	int ret = 0;

	temp = axp_vts_to_temp(bmp_dts_info->pmu_bat_charge_ltf, bmp_dts_info);
	if (temp > 5) {
		reg_val |= 0x01;
	} else if (temp > 0) {
		reg_val |= 0x02;
	} else if (temp > -5) {
		reg_val |= 0x00;
	} else {
		reg_val |= 0x03;
	}
	temp = axp_vts_to_temp(bmp_dts_info->pmu_bat_charge_htf, bmp_dts_info);
	if (temp > 50) {
		reg_val |= 0x0c;
	} else if (temp > 45) {
		reg_val |= 0x08;
	} else if (temp > 40) {
		reg_val |= 0x00;
	} else {
		reg_val |= 0x04;
	}
	temp = axp_vts_to_temp(bmp_dts_info->pmu_bat_work_ltf, bmp_dts_info);
	if (temp > -5) {
		reg_val |= 0x20;
	} else if (temp > -10) {
		reg_val |= 0x10;
	} else if (temp > -20) {
		reg_val |= 0x00;
	} else {
		reg_val |= 0x03;
	}
	temp = axp_vts_to_temp(bmp_dts_info->pmu_bat_work_htf, bmp_dts_info);
	if (temp > 60) {
		reg_val |= 0xc0;
	} else if (temp > 55) {
		reg_val |= 0x80;
	} else if (temp > 50) {
		reg_val |= 0x00;
	} else {
		reg_val |= 0x40;
	}
	ret = regmap_write(regmap, AXP519_NTC_SET1, reg_val);
	ret = regmap_read(regmap, AXP519_NTC_SET1, &reg_val);
	PMIC_DEBUG("%s:%d:AXP519_NTC_SET1:0x%x\n", __func__, __LINE__, reg_val);
	return ret;
}

static int axp519_battery_temp_para_get(struct axp519_power *charger_power)
{
	struct bmu_ext_config_info *bmp_dts_info = &charger_power->dts_info;
	struct regmap *regmap = charger_power->regmap;
	int ret = 0;
	static int check_count = 1;

	if (axp519_battery_power_get(charger_power)) {
		return -1;
	}

	if (check_count == 2)
		return 0;

	of_property_read_u32(charger_power->bat_supply_np, "pmu_bat_temp_enable", &bmp_dts_info->pmu_bat_temp_enable);
	if (bmp_dts_info->pmu_bat_temp_enable) {
		of_property_read_u32(charger_power->bat_supply_np, "pmu_bat_charge_ltf", &bmp_dts_info->pmu_bat_charge_ltf);
		of_property_read_u32(charger_power->bat_supply_np, "pmu_bat_charge_htf", &bmp_dts_info->pmu_bat_charge_htf);
		of_property_read_u32(charger_power->bat_supply_np, "pmu_bat_work_ltf", &bmp_dts_info->pmu_bat_work_ltf);
		of_property_read_u32(charger_power->bat_supply_np, "pmu_bat_work_htf", &bmp_dts_info->pmu_bat_work_htf);
		of_property_read_u32(charger_power->bat_supply_np, "pmu_bat_temp_para1", &bmp_dts_info->pmu_bat_temp_para1);
		of_property_read_u32(charger_power->bat_supply_np, "pmu_bat_temp_para2", &bmp_dts_info->pmu_bat_temp_para2);
		of_property_read_u32(charger_power->bat_supply_np, "pmu_bat_temp_para3", &bmp_dts_info->pmu_bat_temp_para3);
		of_property_read_u32(charger_power->bat_supply_np, "pmu_bat_temp_para4", &bmp_dts_info->pmu_bat_temp_para4);
		of_property_read_u32(charger_power->bat_supply_np, "pmu_bat_temp_para5", &bmp_dts_info->pmu_bat_temp_para5);
		of_property_read_u32(charger_power->bat_supply_np, "pmu_bat_temp_para6", &bmp_dts_info->pmu_bat_temp_para6);
		of_property_read_u32(charger_power->bat_supply_np, "pmu_bat_temp_para7", &bmp_dts_info->pmu_bat_temp_para7);
		of_property_read_u32(charger_power->bat_supply_np, "pmu_bat_temp_para8", &bmp_dts_info->pmu_bat_temp_para8);
		of_property_read_u32(charger_power->bat_supply_np, "pmu_bat_temp_para9", &bmp_dts_info->pmu_bat_temp_para9);
		of_property_read_u32(charger_power->bat_supply_np, "pmu_bat_temp_para10", &bmp_dts_info->pmu_bat_temp_para10);
		of_property_read_u32(charger_power->bat_supply_np, "pmu_bat_temp_para11", &bmp_dts_info->pmu_bat_temp_para11);
		of_property_read_u32(charger_power->bat_supply_np, "pmu_bat_temp_para12", &bmp_dts_info->pmu_bat_temp_para12);
		of_property_read_u32(charger_power->bat_supply_np, "pmu_bat_temp_para13", &bmp_dts_info->pmu_bat_temp_para13);
		of_property_read_u32(charger_power->bat_supply_np, "pmu_bat_temp_para14", &bmp_dts_info->pmu_bat_temp_para14);
		of_property_read_u32(charger_power->bat_supply_np, "pmu_bat_temp_para15", &bmp_dts_info->pmu_bat_temp_para15);
		of_property_read_u32(charger_power->bat_supply_np, "pmu_bat_temp_para16", &bmp_dts_info->pmu_bat_temp_para16);
		axp519_ntc_set(charger_power);
	} else {
		ret = regmap_update_bits(regmap, AXP519_DISCHG_SET1, AXP519_DISCHARG_NTC_MASK, AXP519_DISCHARG_NTC_MASK);
		ret = regmap_update_bits(regmap, AXP519_CHG_SET3, AXP519_CHARG_NTC_MASK, AXP519_CHARG_NTC_MASK);
	}

	check_count = 2;
	return 0;
}

static int axp519_get_vbus_online(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct axp519_power *charger_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = charger_power->regmap;
	unsigned int data[2];
	int ret = 0;

	ret = regmap_read(regmap, AXP519_SYS_STATUS, &data[0]);
	if (ret < 0)
		return ret;

	ret = regmap_read(regmap, AXP519_WORK_CFG, &data[1]);
	if (ret < 0)
		return ret;

	/* vbus is good when vbus state set */
	if (!(data[1] & BIT(0)))
		val->intval = !!(data[0] & AXP519_VBUS_GD_MASK);
	else
		val->intval = 0;

	return ret;
}

static int axp519_get_bat_status(struct power_supply *ps,
				  union power_supply_propval *val)
{
	struct axp519_power *charger_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = charger_power->regmap;
	unsigned int data[2];
	int ret;

	if (!charger_power->battery_exist) {
		val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		return 0;
	}

	ret = regmap_read(regmap, AXP519_SYS_STATUS, &data[0]);
	if (ret < 0)
		return ret;

	ret = regmap_read(regmap, AXP519_WORK_CFG, &data[1]);
	if (ret < 0)
		return ret;

	if ((data[0] & AXP519_VBUS_GD_MASK) && (!(data[1] & BIT(0)))) {
		if (charger_power->charge_done == 2) {
			val->intval = POWER_SUPPLY_STATUS_FULL;
		} else {
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		}
	} else {
		val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
	}

	return 0;
}

static int axp519_set_bat_status(struct regmap *regmap, int status)
{
	unsigned int data;
	int ret = 0;

	if (!status)
		return 0;

	data = status - 1;
	ret = regmap_update_bits(regmap, AXP519_WORK_CFG, BIT(4), data << 4);
	if (ret < 0)
		return ret;

	return 0;
}

static int axp519_get_bat_health(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp519_power *charger_power = power_supply_get_drvdata(ps);
	int ret = 0;

	if (!charger_power->battery_exist) {
		val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;
		return ret;
	}

	if (charger_power->health)
		val->intval = charger_power->health;

	val->intval = POWER_SUPPLY_HEALTH_GOOD;

	return ret;
}

static int axp519_get_temp(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp519_power *charger_power = power_supply_get_drvdata(ps);
	struct bmu_ext_config_info *bmp_dts_info = &charger_power->dts_info;
	struct regmap *regmap = charger_power->regmap;
	uint8_t data[2];
	uint32_t temp;
	int ret = 0, tmp, cur = 4;

	if ((!charger_power->battery_exist) || axp519_battery_temp_para_get(charger_power)) {
		val->intval = 300;
		return ret;
	}

	ret = regmap_update_bits(regmap, AXP519_ADC_CFG, AXP519_ADC_MASK, AXP519_ADC_NTC); /* ADC channel select */
	if (ret < 0)
		return ret;
	mdelay(1);

	ret = regmap_bulk_read(regmap, AXP519_ADC_H, data, 2);
	if (ret < 0)
		return ret;
	temp = (data[0] << 4) | (data[1] & 0x0F);
	tmp = temp * 110 / cur;
	if (tmp < charger_power->dts_info.pmu_bat_charge_ltf) {
		charger_power->health = POWER_SUPPLY_HEALTH_OVERHEAT;
	} else if  (tmp > charger_power->dts_info.pmu_bat_charge_htf) {
		charger_power->health = POWER_SUPPLY_HEALTH_COLD;
	}
	val->intval = axp_vts_to_temp(tmp, bmp_dts_info);
	return ret;
}

static int axp519_get_ovp_vol(struct power_supply *ps,
			     union power_supply_propval *val)
{
	return 0;
}

static int axp519_set_ovp_vol(struct regmap *regmap, int mV)
{
	return 0;
}

static int axp519_get_iin_limit(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct axp519_power *charger_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = charger_power->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, AXP519_LINLIM_SET, &data);
	if (ret < 0)
		return ret;
	data &= AXP519_IINLIM_MASK;
	data = (data * 50) + 500;

	if (data > AXP519_LINLIM_MAX)
		data = AXP519_LINLIM_MAX;
	if (data < AXP519_LINLIM_MIN)
		data = AXP519_LINLIM_MIN;

	val->intval = data;
	return ret;
}

static int axp519_set_iin_limit(struct axp519_power *charger_power, int mA)
{
	struct regmap *regmap = charger_power->regmap;
	unsigned int data;
	int ret = 0;

	data = axp519_charger_input_limit(charger_power, mA);

	PMIC_DEBUG("%s:%d limit:%d\n", __func__, __LINE__, mA);
	if (data > AXP519_LINLIM_MAX)
		data = AXP519_LINLIM_MAX;
	if (data < AXP519_LINLIM_MIN)
		data = AXP519_LINLIM_MIN;

	data = ((data - AXP519_LINLIM_MIN) / 50);

	ret = regmap_update_bits(regmap, AXP519_LINLIM_SET, AXP519_IINLIM_MASK, data);
	if (ret < 0)
		return ret;

	return 0;
}

static int axp519_get_ichg_limit(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct axp519_power *charger_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = charger_power->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, AXP519_VBATLIM_SET, &data);
	if (ret < 0)
		return ret;
	data &= AXP519_VBATLIM_MASK;
	data = (data * 100) + 100;

	if (data > AXP519_VBATLIM_MAX)
		data = AXP519_VBATLIM_MAX;
	if (data < AXP519_VBATLIM_MIN)
		data = AXP519_VBATLIM_MIN;

	val->intval = data;
	return ret;
}

static int axp519_set_ichg_limit(struct regmap *regmap, int mA)
{
	unsigned int data;
	int ret = 0;

	data = mA;
	if (data > AXP519_VBATLIM_MAX)
		data = AXP519_VBATLIM_MAX;
	if (data < AXP519_VBATLIM_MIN)
		data = AXP519_VBATLIM_MIN;
	data = ((data - AXP519_VBATLIM_MIN) / 100);
	ret = regmap_update_bits(regmap, AXP519_VBATLIM_SET, AXP519_VBATLIM_MASK,
				 data);
	if (ret < 0)
		return ret;

	return 0;
}

static int axp519_get_time_to_full(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct axp519_power *charger_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = charger_power->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, AXP519_CHG_SET3, &data);
	if (ret < 0)
		return ret;

	if (data & AXP519_CHG_TIMER_EN_MASK) {
		val->intval = 0;
		return ret;
	}

	switch (data & AXP519_CHG_TIMER_MASK) {
	case 1:
		val->intval = 24;
		break;
	case 2:
		val->intval = 48;
		break;
	case 3:
		val->intval = 72;
		break;
	default:
		val->intval = 12;
		break;
	}

	return ret;
}

static int axp519_set_time_to_full(struct regmap *regmap, int hour)
{
	unsigned int data;
	int ret = 0;

	data = hour;

	if (data == 0) {
		ret = regmap_update_bits(regmap, AXP519_CHG_SET3, AXP519_CHG_TIMER_EN_MASK, AXP519_CHG_TIMER_EN_MASK);
		if (ret < 0)
			return ret;

		return 0;
	} else {
		ret = regmap_update_bits(regmap, AXP519_CHG_SET3, AXP519_CHG_TIMER_EN_MASK, 0);
	}

	if (data > 48)
		data = 3;
	else if (data > 24)
		data = 2;
	else if (data > 12)
		data = 1;
	else
		data = 0;
	ret = regmap_update_bits(regmap, AXP519_CHG_SET3, AXP519_CHG_TIMER_MASK, data);
	if (ret < 0)
		return ret;

	return 0;
}

static int axp519_charger_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	struct axp519_power *charger_power = power_supply_get_drvdata(psy);
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (charger_power->set_offline == 1)
			val->intval = 0;
		else
			ret = axp519_get_vbus_online(psy, val);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		if (charger_power->set_offline == 1)
			val->intval = 0;
		else
			ret = axp519_get_vbus_online(psy, val);
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = AXP519_MANUFACTURER;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		ret = axp519_get_bat_status(psy, val);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		ret = axp519_get_ovp_vol(psy, val);
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = axp519_get_iin_limit(psy, val);
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		ret = axp519_get_time_to_full(psy, val);
		break;
	case POWER_SUPPLY_PROP_CALIBRATE:
		val->intval = charger_power->battery_exist;
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = axp519_get_ichg_limit(psy, val);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		ret = axp519_get_temp(psy, val);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		ret = axp519_get_bat_health(psy, val);
		break;
	case POWER_SUPPLY_PROP_TEMP_ALERT_MIN:
		val->intval = -200000;
		break;
	case POWER_SUPPLY_PROP_TEMP_ALERT_MAX:
		val->intval = 200000;
		break;
	case POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MIN:
		if (!charger_power->dts_info.pmu_bat_temp_enable) {
			val->intval = -200000;
			break;
		}
		ret = charger_power->dts_info.pmu_bat_charge_ltf;
		val->intval = axp_vts_to_temp(ret, &charger_power->dts_info);
		break;
	case POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MAX:
		if (!charger_power->dts_info.pmu_bat_temp_enable) {
			val->intval = 200000;
			break;
		}
		ret = charger_power->dts_info.pmu_bat_charge_htf;
		val->intval = axp_vts_to_temp(ret, &charger_power->dts_info);
		break;
	default:
		break;
	}

	return ret;
}

static int axp519_charger_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct axp519_power *charger_power = power_supply_get_drvdata(psy);
	struct regmap *regmap = charger_power->regmap;
	int ret = 0, lim_cur;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (val->intval == 0) {
			charger_power->set_offline = 1;
			schedule_delayed_work(&charger_power->vbus_exist_mon, msecs_to_jiffies(300));
		}
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		pr_debug("%s L%d set input current limit %dmV %d mA [%d]", __func__, __LINE__,
			 charger_power->vbus_set_limit, val->intval, irqs_disabled());
		lim_cur = val->intval;
		if (!charger_power->battery_exist)
			break;
		charger_power->prev_state = charger_power->usb_state;
		schedule_delayed_work(&charger_power->usb_chg_state, msecs_to_jiffies(5 * 1000));
		if ((val->intval == 2000 && charger_power->vbus_set_limit == 5000) ||
		    (val->intval == 2000 && charger_power->vbus_set_limit == 0) /* USB Suspend */
		    || (lim_cur == 0)) {
			charger_power->usb_state = USB_STATE_DISCONNECTED;
		} else if (charger_power->usb_state != USB_STATE_CONFIGURED) {
			charger_power->usb_state = USB_STATE_CONNECTED;
		}
		/*
		 * Section 1.4.13 Standard Downstream Port of the USB battery charging
		 * specification v1.2 states that a device connected on a SDP shall only
		 * draw at max 100mA while in a connected, but unconfigured state.
		 */
		if (val->intval == 100000 || val->intval == 500000) { /* USB Reset */
			charger_power->usb_state = USB_STATE_CONFIGURED;
		}
		if (irqs_disabled() || (lim_cur == 0))
			break;
		charger_power->vbus_set_vol_limit = val->intval;
		ret = axp519_set_iin_limit(charger_power, val->intval);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		pr_debug("%s L%d set input voltage limit %d mV", __func__, __LINE__, val->intval);
		charger_power->vbus_set_limit = val->intval;
		if (val->intval == 0)
			break;
		ret = axp519_set_ovp_vol(regmap, val->intval);
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		ret = axp519_set_time_to_full(regmap, val->intval);
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = axp519_set_ichg_limit(regmap, val->intval);
		charger_power->charge_set_limit = val->intval;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		ret = axp519_set_bat_status(regmap, val->intval);
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static int axp519_power_property_is_writeable(struct power_supply *psy,
			     enum power_supply_property psp)
{
	int ret = 0;
	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = 0;
		break;
	default:
		ret = -EINVAL;
	}
	return ret;

}

static void axp519_usb_set_current_fsm(struct work_struct *work)
{
	struct axp519_power *charger_power =
	    container_of(work, struct axp519_power, usb_chg_state.work);

	pr_debug("%s L%d usb_state %d %s\n", __func__, __LINE__,
		 charger_power->usb_state, usb_state_name[charger_power->usb_state]);

	switch (charger_power->usb_state) {
	case USB_STATE_CONFIGURED:
		charger_power->vbus_set_vol_limit = charger_power->dts_info.pmu_usbpc_input_cur;
		axp519_set_iin_limit(charger_power, charger_power->dts_info.pmu_usbpc_input_cur);
		pr_info("current limit setted: usb pc type\n");
		break;
	case USB_STATE_CONNECTED:
		if (charger_power->vbus_set_limit <= 5000) { /* Non quick charger */
			charger_power->vbus_set_vol_limit = charger_power->dts_info.pmu_usbad_input_cur;
			axp519_set_iin_limit(charger_power, charger_power->dts_info.pmu_usbad_input_cur);
		}
		pr_info("current limit not set: usb adapter type\n");
		break;
	case USB_STATE_DISCONNECTED:
		if (!charger_power->battery_exist)
			break;
		fallthrough;
	default:
		charger_power->vbus_set_vol_limit = charger_power->dts_info.pmu_usbpc_input_cur;
		axp519_set_iin_limit(charger_power, charger_power->dts_info.pmu_usbpc_input_cur);
		pr_info("current limit not set: usb default type\n");
		break;
	}
}

static const struct power_supply_desc axp519_charger_desc = {
	.name = "axp519-charger",
	.type = POWER_SUPPLY_TYPE_USB,
	.get_property = axp519_charger_get_property,
	.properties = axp519_charger_props,
	.set_property = axp519_charger_set_property,
	.num_properties = ARRAY_SIZE(axp519_charger_props),
	.property_is_writeable = axp519_power_property_is_writeable,
};

static irqreturn_t axp519_irq_handler_usb_in(int irq, void *data)
{
	struct axp519_power *charger_power = data;
	unsigned int val;

	power_supply_changed(charger_power->charger_supply);

	if (charger_power->battery_exist) {
		regmap_read(charger_power->regmap, AXP519_WORK_CFG, &val);
		if (!(val & BIT(0)))
			regmap_update_bits(charger_power->regmap, AXP519_WORK_CFG, AXP519_CHG_EN_MASK, AXP519_CHG_EN_MASK);
	}

	return IRQ_HANDLED;
}

static irqreturn_t axp519_irq_handler_usb_out(int irq, void *data)
{
	struct axp519_power *charger_power = data;

	power_supply_changed(charger_power->charger_supply);
	charger_power->charge_done = 0;

	return IRQ_HANDLED;
}

static irqreturn_t axp519_irq_handler_ntc(int irq, void *data)
{
	struct axp519_power *charger_power = data;
	union power_supply_propval val;
	int tmp;

	power_supply_changed(charger_power->charger_supply);

	axp519_get_temp(charger_power->charger_supply, &val);
	tmp = val.intval;

	return IRQ_HANDLED;
}

static irqreturn_t axp519_irq_handler_status(int irq, void *data)
{
	struct irq_desc *id = irq_to_desc(irq);
	struct axp519_power *charger_power = data;

	power_supply_changed(charger_power->charger_supply);
	PMIC_DEBUG("%s: enter interrupt %d\n", __func__, irq);

	switch (id->irq_data.hwirq) {
	case AXP519_IRQ_COVT:
		charger_power->health = POWER_SUPPLY_HEALTH_DEAD;
		PMIC_DEBUG("interrupt:charge_over_time");
		break;
	case AXP519_IRQ_CHGDN:
		charger_power->charge_done = 2;
		PMIC_DEBUG("interrupt:charge_done");
		break;
	default:
		PMIC_DEBUG("interrupt:others");
		break;
	}
	return IRQ_HANDLED;
}

enum axp519_usb_virq_index {
	AXP519_VIRQ_USB_IN,
	AXP519_VIRQ_USB_OUT,
	AXP519_VIRQ_NTC,
	AXP519_VIRQ_CHGDN,
	AXP519_VIRQ_COVT,

	AXP519_USB_VIRQ_MAX_VIRQ,
};

static struct axp_interrupts axp_usb_irq[] = {
	[AXP519_VIRQ_USB_IN] = { "b_insert", axp519_irq_handler_usb_in },
	[AXP519_VIRQ_USB_OUT] = { "b_remove", axp519_irq_handler_usb_out },

	[AXP519_VIRQ_NTC] = { "battery_ntc_reaching", axp519_irq_handler_ntc },
	[AXP519_VIRQ_CHGDN] = { "battery_charge_done", axp519_irq_handler_status },
	[AXP519_VIRQ_COVT] = { "charge_over_time", axp519_irq_handler_status },

};

static void axp519_power_monitor(struct work_struct *work)
{
	struct axp519_power *charger_power =
		container_of(work, typeof(*charger_power), charger_supply_mon.work);
	schedule_delayed_work(&charger_power->charger_supply_mon, msecs_to_jiffies(1 * 1000));
}

static void axp519_vbus_exist_monitor(struct work_struct *work)
{
	struct axp519_power *charger_power =
		container_of(work, typeof(*charger_power), vbus_exist_mon.work);

	charger_power->set_offline = 0;
}

static int axp519_battery_exist_get(struct axp519_power *charger_power)
{
	struct regmap *regmap = charger_power->regmap;
	int ret = 0, data;

	ret = regmap_read(regmap, AXP519_CHG_SET1, &data);
	if (data & AXP519_FULL_DISCHG_MASK) {
		charger_power->battery_exist = 0;
	} else {
		charger_power->battery_exist = 1;
	}

	return ret;
}

static void axp519_power_init(struct axp519_power *charger_power)
{
	struct bmu_ext_config_info *bmp_dts_info = &charger_power->dts_info;
	struct regmap *regmap = charger_power->regmap;
	unsigned int data = 0;
	uint8_t reg_val[2];
	uint16_t reg;

	/* set default input voltage limit to 5V*/
	axp519_set_ovp_vol(charger_power->regmap, 5000);

	/* set default charge time */
	axp519_set_time_to_full(charger_power->regmap, bmp_dts_info->bmu_ext_save_charge_time);

	if (!bmp_dts_info->battery_exist_sets) {
		axp519_battery_exist_get(charger_power);
	} else {
		charger_power->battery_exist = bmp_dts_info->battery_exist_sets - 1;
	}
	if (!charger_power->battery_exist) {
		axp519_set_iin_limit(charger_power, charger_power->dts_info.pmu_usbad_input_cur);
		charger_power->vbus_set_vol_limit = charger_power->dts_info.pmu_usbad_input_cur;
	}

	/* set default status */
	charger_power->health = 0;
	charger_power->charge_done = 0;

	/* set charger voltage limit */
	data = bmp_dts_info->pmu_init_chgvol;
	if (!axp519_battery_power_get(charger_power)) {
		if (of_property_read_u32(charger_power->bat_supply_np, "pmu_init_chgvol", &data) < 0)
			data = bmp_dts_info->pmu_init_chgvol;
		PMIC_DEBUG("bmu-ext:pmu_init_chgvol:%d\n", data);
	}

	if (data > AXP519_VTERM_MAX)
		data = AXP519_VTERM_MAX;
	if (data < AXP519_VTERM_MIN)
		data = AXP519_VTERM_MIN;

	reg = ((data - AXP519_VTERM_MIN) / 10);
	reg_val[0] = reg >> 3;
	reg_val[1] = reg & 0x07;
	regmap_bulk_write(regmap, AXP519_VTERM_CFG_H, reg_val, 2);

	/* set chgcur lim */
	axp519_set_ichg_limit(regmap, bmp_dts_info->pmu_runtime_chgcur);
	charger_power->charge_set_limit = bmp_dts_info->pmu_runtime_chgcur;
	PMIC_INFO("bmu-ext:chip init finish\n");

	/* get variable information from battery_power*/
	axp519_battery_temp_para_get(charger_power);
	charger_power->set_offline = 0;
}

int axp519_charger_dt_parse(struct device_node *node,
			 struct bmu_ext_config_info *bmp_ext_config)
{
	if (!of_device_is_available(node)) {
		PMIC_ERR("%s: failed\n", __func__);
		return -1;
	}

	BMU_EXT_OF_PROP_READ(pmu_usbpc_input_vol,                    5000);
	BMU_EXT_OF_PROP_READ(pmu_usbpc_input_cur,                    500);
	BMU_EXT_OF_PROP_READ(pmu_usbad_input_vol,                    5000);
	BMU_EXT_OF_PROP_READ(pmu_usbad_input_cur,                    2400);
	BMU_EXT_OF_PROP_READ(bmu_ext_save_charge_time,               10);
	BMU_EXT_OF_PROP_READ(bmu_ext_max_power,                      45000000);
	BMU_EXT_OF_PROP_READ(battery_exist_sets,               		 0);

	BMU_EXT_OF_PROP_READ(pmu_runtime_chgcur,             		 2000);
	BMU_EXT_OF_PROP_READ(pmu_suspend_chgcur,            		 3000);
	BMU_EXT_OF_PROP_READ(pmu_shutdown_chgcur,           		 3000);
	BMU_EXT_OF_PROP_READ(pmu_init_chgvol,           		 	 8700);

	return 0;
}

static void axp519_charger_parse_device_tree(struct axp519_power *charger_power)
{
	int ret;
	struct bmu_ext_config_info *cfg;

	if (!charger_power->dev->of_node) {
		PMIC_INFO("can not find device tree\n");
		return;
	}

	cfg = &charger_power->dts_info;
	ret = axp519_charger_dt_parse(charger_power->dev->of_node, cfg);
	if (ret) {
		PMIC_INFO("can not parse device tree err\n");
		return;
	}

	/*init axp519 charger by device tree*/
	axp519_power_init(charger_power);
}

static int axp519_charger_probe(struct platform_device *pdev)
{
	struct axp519_power *charger_power;
	struct bmu_ext_dev *ext = dev_get_drvdata(pdev->dev.parent);
	struct power_supply_config psy_cfg = {};
	int ret = 0, i, irq;

	PMIC_ERR("axp519_charger_probe\n");
	charger_power = devm_kzalloc(&pdev->dev, sizeof(*charger_power), GFP_KERNEL);
	if (charger_power == NULL) {
		PMIC_ERR("axp519_power alloc failed\n");
		ret = -ENOMEM;
		goto err;
	}

	charger_power->name = "axp519_charger";
	charger_power->dev = &pdev->dev;
	charger_power->regmap = ext->regmap;

	/* parse device tree and set register */
	axp519_charger_parse_device_tree(charger_power);

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = charger_power;

	charger_power->charger_supply = devm_power_supply_register(charger_power->dev,
			&axp519_charger_desc, &psy_cfg);

	if (IS_ERR(charger_power->charger_supply)) {
		PMIC_ERR("axp519 failed to register charger power\n");
		ret = PTR_ERR(charger_power->charger_supply);
		return ret;
	}

	for (i = 0; i < ARRAY_SIZE(axp_usb_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp_usb_irq[i].name);
		if (irq < 0)
			continue;

		irq = regmap_irq_get_virq(ext->regmap_irqc, irq);
		if (irq < 0) {
			PMIC_DEV_ERR(&pdev->dev, "can not get irq\n");
			return irq;
		}
		/* we use this variable to suspend irq */
		axp_usb_irq[i].irq = irq;
		ret = devm_request_any_context_irq(&pdev->dev, irq,
						   axp_usb_irq[i].isr, 0,
						   axp_usb_irq[i].name, charger_power);
		if (ret < 0) {
			PMIC_DEV_ERR(&pdev->dev, "failed to request %s IRQ %d: %d\n",
				axp_usb_irq[i].name, irq, ret);
			return ret;
		} else {
			ret = 0;
		}

		PMIC_DEV_DEBUG(&pdev->dev, "Requested %s IRQ %d: %d\n",
			axp_usb_irq[i].name, irq, ret);
	}

	INIT_DELAYED_WORK(&charger_power->charger_supply_mon, axp519_power_monitor);
	INIT_DELAYED_WORK(&charger_power->vbus_exist_mon, axp519_vbus_exist_monitor);
	INIT_DELAYED_WORK(&charger_power->usb_chg_state, axp519_usb_set_current_fsm);
	schedule_delayed_work(&charger_power->charger_supply_mon, msecs_to_jiffies(500));

	platform_set_drvdata(pdev, charger_power);

	return ret;

err:
	PMIC_ERR("%s,probe fail, ret = %d\n", __func__, ret);

	return ret;
}

static int axp519_charger_remove(struct platform_device *pdev)
{
	struct axp519_power *charger_power = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&charger_power->charger_supply_mon);
	cancel_delayed_work_sync(&charger_power->usb_chg_state);

	PMIC_DEV_DEBUG(&pdev->dev, "==============axp519 charger unegister==============\n");
	if (charger_power->charger_supply)
		power_supply_unregister(charger_power->charger_supply);
	PMIC_DEV_DEBUG(&pdev->dev, "axp519 teardown charger dev\n");

	return 0;
}

static void axp519_charger_shutdown(struct platform_device *pdev)
{
	struct axp519_power *charger_power = platform_get_drvdata(pdev);
	struct bmu_ext_config_info *bmp_dts_info = &charger_power->dts_info;

	if (charger_power->charge_set_limit >= bmp_dts_info->pmu_runtime_chgcur)
		axp519_set_ichg_limit(charger_power->regmap, bmp_dts_info->pmu_shutdown_chgcur);

	cancel_delayed_work_sync(&charger_power->charger_supply_mon);
	cancel_delayed_work_sync(&charger_power->vbus_exist_mon);
	cancel_delayed_work_sync(&charger_power->usb_chg_state);
}

static int axp519_charger_suspend(struct platform_device *p, pm_message_t state)
{
	struct axp519_power *charger_power = platform_get_drvdata(p);
	struct bmu_ext_config_info *bmp_dts_info = &charger_power->dts_info;

	if (charger_power->charge_set_limit >= bmp_dts_info->pmu_runtime_chgcur)
		axp519_set_ichg_limit(charger_power->regmap, bmp_dts_info->pmu_suspend_chgcur);

	if (charger_power->prev_state != charger_power->usb_state)
		charger_power->usb_state = charger_power->prev_state;
	cancel_delayed_work_sync(&charger_power->charger_supply_mon);
	cancel_delayed_work_sync(&charger_power->vbus_exist_mon);
	cancel_delayed_work_sync(&charger_power->usb_chg_state);

	return 0;
}

static int axp519_charger_resume(struct platform_device *p)
{
	struct axp519_power *charger_power = platform_get_drvdata(p);

	axp519_set_ichg_limit(charger_power->regmap, charger_power->charge_set_limit);
	schedule_delayed_work(&charger_power->charger_supply_mon, 0);
	schedule_delayed_work(&charger_power->vbus_exist_mon, 0);
	schedule_delayed_work(&charger_power->usb_chg_state, 0);

	return 0;
}

static const struct of_device_id axp519_charger_power_match[] = {
	{
		.compatible = "x-powers-ext,axp519-charger-power-supply",
		.data = (void *)AXP519_ID,
	}, {/* sentinel */}
};
MODULE_DEVICE_TABLE(of, axp519_charger_power_match);

static struct platform_driver axp519_charger_power_driver = {
	.driver = {
		.name = "axp519-charger-power-supply",
		.of_match_table = axp519_charger_power_match,
	},
	.probe = axp519_charger_probe,
	.remove = axp519_charger_remove,
	.shutdown = axp519_charger_shutdown,
	.suspend = axp519_charger_suspend,
	.resume = axp519_charger_resume,
};

module_platform_driver(axp519_charger_power_driver);

MODULE_AUTHOR("xinouyang <xinouyang@allwinnertech.com>");
MODULE_DESCRIPTION("axp519 charger driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
