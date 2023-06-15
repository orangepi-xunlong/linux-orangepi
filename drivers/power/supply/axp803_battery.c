#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/fs.h>
#include <linux/ktime.h>
#include <linux/of.h>
#include <linux/timekeeping.h>
#include <linux/types.h>
#include <linux/string.h>
#include <asm/irq.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/pm_runtime.h>
#include <linux/gpio/consumer.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/err.h>
#include <linux/mfd/axp2101.h>
#include "axp803_charger.h"

struct axp803_bat_power {
	char                      *name;
	struct device             *dev;
	struct axp_config_info     dts_info;
	struct regmap             *regmap;
	struct power_supply       *bat_supply;
	struct delayed_work        bat_supply_mon;
};

static bool charger_debug;

static int axp803_get_bat_health(struct axp803_bat_power *bat_power)
{
	unsigned int reg_value;
	int ret = 0;

	ret = regmap_read(bat_power->regmap, AXP803_MODE_CHGSTATUS, &reg_value);
	if (reg_value & AXP803_FAULT_LOG_BATINACT)
		return POWER_SUPPLY_HEALTH_DEAD;
	else if (reg_value & AXP803_FAULT_LOG_OVER_TEMP)
		return POWER_SUPPLY_HEALTH_OVERHEAT;
	else if (reg_value & AXP803_FAULT_LOG_COLD)
		return POWER_SUPPLY_HEALTH_COLD;
	else
		return POWER_SUPPLY_HEALTH_GOOD;
}

static inline int axp803_vbat_to_mV(unsigned int reg)
{
	return ((int)(((reg >> 8) << 4) | (reg & 0x000F))) * 1100 / 1000;
}

static int axp803_get_vbat(struct axp803_bat_power *bat_power)
{
	unsigned char temp_val[2];
	unsigned int res;
	int ret = 0;

	ret = regmap_bulk_read(bat_power->regmap, AXP803_VBATH_RES, temp_val, 2);
	if (ret < 0)
		return ret;

	res = (temp_val[0] << 8) | temp_val[1];

	return axp803_vbat_to_mV(res);
}

static inline int axp803_icharge_to_mA(unsigned int reg)
{
	return (int)(((reg >> 8) << 4) | (reg & 0x000F));
}

static int axp803_get_ibat(struct axp803_bat_power *bat_power)
{
	unsigned char tmp[2];
	unsigned int res;

	regmap_bulk_read(bat_power->regmap, AXP803_IBATH_REG, tmp, 2);
	res = (tmp[0] << 8) | tmp[1];

	return axp803_icharge_to_mA(res);
}

static int axp803_get_disibat(struct axp803_bat_power *bat_power)
{
	unsigned char tmp[2];
	unsigned int dis_res;

	regmap_bulk_read(bat_power->regmap, AXP803_DISIBATH_REG, tmp, 2);
	dis_res = (tmp[0] << 8) | tmp[1];

	return axp803_icharge_to_mA(dis_res);
}

static int axp803_set_bat_chg_cur(struct axp803_bat_power *bat_power, int cur)
{
	uint8_t tmp = 0;
	struct regmap *map = bat_power->regmap;

	if (cur == 0)
		regmap_update_bits(map, AXP803_CHARGE1, 0x80, 0x00);
	else
		regmap_update_bits(map, AXP803_CHARGE1, 0x80, 0x80);

	if (cur >= 200 && cur <= 2800) {
		tmp = (cur - 200) / 200;
		regmap_update_bits(map, AXP803_CHARGE1, 0x0f, tmp);
	} else if (cur < 200) {
		regmap_update_bits(map, AXP803_CHARGE1, 0x0f, 0x00);
	} else {
		regmap_update_bits(map, AXP803_CHARGE1, 0x0f, 0x0d);
	}

	return 0;
}

static int axp803_get_rest_cap(struct axp803_bat_power *bat_power)
{
	unsigned char temp_val[2];
	unsigned int reg_value;
	int batt_max_cap, coulumb_counter;
	int rest_vol = 0;
	int ocv_vol = 0;
	int rdc = 0;
	int ret = 0;

	int charging = 0;
	int ocv_pct = 0;
	int coul_pct = 0;
	int ac_valid = 0;
	int usb_valid = 0;
	static int pre_rest_vol, invalid_count;

	struct axp_config_info *axp_config = &bat_power->dts_info;

	ret = regmap_read(bat_power->regmap, AXP803_CAP, &reg_value);
	if (ret)
		return ret;

	if (reg_value & 0x80) {
		rest_vol = (int)(reg_value & 0x7F);
		pre_rest_vol = rest_vol;
		invalid_count = 0;
	} else {
		rest_vol = pre_rest_vol;
		invalid_count++;

		if (invalid_count == 30) {
			invalid_count = 0;
			rest_vol = 100;
		}

		return rest_vol;
	}

	/* read chatging status */
	ret = regmap_read(bat_power->regmap, AXP803_MODE_CHGSTATUS, &reg_value);
	if (ret)
		return ret;
	charging = (reg_value & (1 << 6)) ? 1 : 0;

	/* read adaptor valid status */
	ret = regmap_read(bat_power->regmap, AXP803_STATUS, &reg_value);
	if (ret)
		return ret;
	ac_valid = (reg_value & (1 << 6)) ? 1 : 0;

	ret = regmap_read(bat_power->regmap, AXP803_STATUS, &reg_value);
	if (ret)
		return ret;
	usb_valid = (reg_value & (1 << 4)) ? 1 : 0;

	/* read ocv percentage */
	ret = regmap_read(bat_power->regmap, AXP803_OCV_PERCENT, &reg_value);
	if (ret)
		return ret;
	if (reg_value & 0x80)
		ocv_pct = (int)(reg_value & 0x7F);

	/* read coul percentage */
	ret = regmap_read(bat_power->regmap, AXP803_COU_PERCENT, &reg_value);
	if (ret)
		return ret;
	if (reg_value & 0x80)
		coul_pct = (int)(reg_value & 0x7F);

	if (ocv_pct == 100 && charging == 0 && rest_vol == 99
		&& (ac_valid == 1 || usb_valid == 1)) {

		ret = regmap_read(bat_power->regmap, AXP803_COULOMB_CTL, &reg_value);
		if (ret)
			return ret;
		regmap_write(bat_power->regmap, AXP803_COULOMB_CTL, (reg_value & 0x7f));
		regmap_write(bat_power->regmap, AXP803_COULOMB_CTL, (reg_value | 0x80));
		rest_vol = 100;
	}
	if (ocv_pct == 100 && coul_pct == 100 && axp_config->ocv_coulumb_100 == 1) {
		rest_vol = 100;
	}

	ret = regmap_bulk_read(bat_power->regmap, AXP803_COUCNT0, temp_val, 2);
	if (ret < 0)
		return ret;
	coulumb_counter = (((temp_val[0] & 0x7f) << 8) + temp_val[1])
						* 1456 / 1000;

	ret = regmap_bulk_read(bat_power->regmap, AXP803_BATCAP0, temp_val, 2);
	if (ret < 0)
		return ret;
	batt_max_cap = (((temp_val[0] & 0x7f) << 8) + temp_val[1])
						* 1456 / 1000;

	if (charger_debug) {
		ret = regmap_bulk_read(bat_power->regmap, AXP803_OCVBATH_RES, temp_val, 2);
		if (ret < 0)
			return ret;
		ocv_vol  =  ((temp_val[0] << 4) | (temp_val[1] & 0xF)) * 1100 / 1000;

		ret = regmap_bulk_read(bat_power->regmap, AXP803_RDC0, temp_val, 2);
		if (ret < 0)
			return ret;
		rdc  =  (((temp_val[0] & 0x1f) << 8) + temp_val[1]) * 10742 / 10000;

		pr_debug("calc_info: ocv_vol:%d rdc:%d coulumb_counter:%d batt_max_cap:%d\n",
			ocv_vol, rdc, coulumb_counter, batt_max_cap);
	}

	return rest_vol;
}

static inline int axp_vts_to_mV(u16 reg)
{
	return ((int)(((reg >> 8) << 4) | (reg & 0x000F))) * 800 / 1000;
}

static inline int axp_vts_to_temp(int data,
		const struct axp_config_info *axp_config)
{
	int temp;

	if (data < 80 || !axp_config->pmu_bat_temp_enable)
		return 30;
	else if (data < axp_config->pmu_bat_temp_para16)
		return 80;
	else if (data <= axp_config->pmu_bat_temp_para15) {
		temp = 70 + (axp_config->pmu_bat_temp_para15-data)*10/
		(axp_config->pmu_bat_temp_para15-axp_config->pmu_bat_temp_para16);
	} else if (data <= axp_config->pmu_bat_temp_para14) {
		temp = 60 + (axp_config->pmu_bat_temp_para14-data)*10/
		(axp_config->pmu_bat_temp_para14-axp_config->pmu_bat_temp_para15);
	} else if (data <= axp_config->pmu_bat_temp_para13) {
		temp = 55 + (axp_config->pmu_bat_temp_para13-data)*5/
		(axp_config->pmu_bat_temp_para13-axp_config->pmu_bat_temp_para14);
	} else if (data <= axp_config->pmu_bat_temp_para12) {
		temp = 50 + (axp_config->pmu_bat_temp_para12-data)*5/
		(axp_config->pmu_bat_temp_para12-axp_config->pmu_bat_temp_para13);
	} else if (data <= axp_config->pmu_bat_temp_para11) {
		temp = 45 + (axp_config->pmu_bat_temp_para11-data)*5/
		(axp_config->pmu_bat_temp_para11-axp_config->pmu_bat_temp_para12);
	} else if (data <= axp_config->pmu_bat_temp_para10) {
		temp = 40 + (axp_config->pmu_bat_temp_para10-data)*5/
		(axp_config->pmu_bat_temp_para10-axp_config->pmu_bat_temp_para11);
	} else if (data <= axp_config->pmu_bat_temp_para9) {
		temp = 30 + (axp_config->pmu_bat_temp_para9-data)*10/
		(axp_config->pmu_bat_temp_para9-axp_config->pmu_bat_temp_para10);
	} else if (data <= axp_config->pmu_bat_temp_para8) {
		temp = 20 + (axp_config->pmu_bat_temp_para8-data)*10/
		(axp_config->pmu_bat_temp_para8-axp_config->pmu_bat_temp_para9);
	} else if (data <= axp_config->pmu_bat_temp_para7) {
		temp = 10 + (axp_config->pmu_bat_temp_para7-data)*10/
		(axp_config->pmu_bat_temp_para7-axp_config->pmu_bat_temp_para8);
	} else if (data <= axp_config->pmu_bat_temp_para6) {
		temp = 5 + (axp_config->pmu_bat_temp_para6-data)*5/
		(axp_config->pmu_bat_temp_para6-axp_config->pmu_bat_temp_para7);
	} else if (data <= axp_config->pmu_bat_temp_para5) {
		temp = 0 + (axp_config->pmu_bat_temp_para5-data)*5/
		(axp_config->pmu_bat_temp_para5-axp_config->pmu_bat_temp_para6);
	} else if (data <= axp_config->pmu_bat_temp_para4) {
		temp = -5 + (axp_config->pmu_bat_temp_para4-data)*5/
		(axp_config->pmu_bat_temp_para4-axp_config->pmu_bat_temp_para5);
	} else if (data <= axp_config->pmu_bat_temp_para3) {
		temp = -10 + (axp_config->pmu_bat_temp_para3-data)*5/
		(axp_config->pmu_bat_temp_para3-axp_config->pmu_bat_temp_para4);
	} else if (data <= axp_config->pmu_bat_temp_para2) {
		temp = -15 + (axp_config->pmu_bat_temp_para2-data)*5/
		(axp_config->pmu_bat_temp_para2-axp_config->pmu_bat_temp_para3);
	} else if (data <= axp_config->pmu_bat_temp_para1) {
		temp = -25 + (axp_config->pmu_bat_temp_para1-data)*10/
		(axp_config->pmu_bat_temp_para1-axp_config->pmu_bat_temp_para2);
	} else
		temp = -25;
	return temp;
}

static int axp803_get_bat_temp(struct axp803_bat_power *bat_power)
{
	unsigned char temp_val[2];
	unsigned short ts_res;
	int bat_temp_mv, bat_temp;
	int ret = 0;

	struct axp_config_info *axp_config = &bat_power->dts_info;

	ret = regmap_bulk_read(bat_power->regmap, AXP803_VTS_RES, temp_val, 2);
	if (ret < 0)
		return ret;

	ts_res = ((unsigned short) temp_val[0] << 8) | temp_val[1];
	bat_temp_mv = axp_vts_to_mV(ts_res);
	bat_temp = axp_vts_to_temp(bat_temp_mv, axp_config);

	pr_debug("bat_temp: %d\n", bat_temp);

	return bat_temp;
}

static int axp803_bat_get_max_voltage(struct axp803_bat_power *bat_power)
{
	int ret, reg;
	int val = 0;

	ret = regmap_read(bat_power->regmap, AXP803_CHARGE1, &reg);
	if (ret)
		return ret;

	switch (reg & AXP803_CHRG_CTRL1_TGT_VOLT) {
	case AXP803_CHRG_CTRL1_TGT_4_1V:
		val = 4100000;
		break;
	case AXP803_CHRG_CTRL1_TGT_4_15V:
		val = 4150000;
		break;
	case AXP803_CHRG_CTRL1_TGT_4_2V:
		val = 4200000;
		break;
	case AXP803_CHRG_CTRL1_TGT_4_35V:
		val = 4350000;
		break;
	default:
		return -EINVAL;
	}

	return val;
}

static int axp803_get_bat_status(struct power_supply *psy,
					union power_supply_propval *val)
{
	bool bat_det, bat_charging;
	bool ac_valid, vbus_valid;
	unsigned int rest_vol;
	unsigned int reg_value;
	int ret;

	struct axp803_bat_power *bat_power = power_supply_get_drvdata(psy);

	ret = regmap_read(bat_power->regmap, AXP803_MODE_CHGSTATUS, &reg_value);
	if (ret)
		return ret;
	bat_det = !!(reg_value & AXP803_CHGSTATUS_BAT_PST_VALID) &&
		!!(reg_value & AXP803_CHGSTATUS_BAT_PRESENT);
	bat_charging = !!(reg_value & AXP803_CHGSTATUS_BAT_CHARGING);

	ret = regmap_read(bat_power->regmap, AXP803_STATUS, &reg_value);
	if (ret)
		return ret;
	ac_valid = !!(reg_value & AXP803_STATUS_AC_USED);
	vbus_valid = !!(reg_value & AXP803_STATUS_VBUS_USED);

	rest_vol = axp803_get_rest_cap(bat_power);

	if (ac_valid || vbus_valid) {
		if (bat_det) {
			if (rest_vol == 100)
				val->intval = POWER_SUPPLY_STATUS_FULL;
			else if (bat_charging)
				val->intval = POWER_SUPPLY_STATUS_CHARGING;
			else
				val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		} else {
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		}
	} else {
		ret = regmap_read(bat_power->regmap, AXP803_MODE_CHGSTATUS, &reg_value);
		if (ret)
			return ret;
		bat_det = !!(reg_value & AXP803_CHGSTATUS_BAT_PRESENT);
		if (bat_det)
			val->intval =  POWER_SUPPLY_STATUS_DISCHARGING;
		else
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
	}

	return 0;
}

static enum power_supply_property axp803_bat_props[] = {
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
};

static int axp803_bat_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	int ret = 0;
	unsigned int reg_value;
	unsigned char temp_val[2];
	struct axp803_bat_power *bat_power = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = psy->desc->name;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		ret = axp803_get_bat_status(psy, val);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = axp803_get_bat_health(bat_power);
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CHARGE_COUNTER:
		ret = regmap_bulk_read(bat_power->regmap, AXP803_COUCNT0, temp_val, 2);
		if (ret < 0)
			return ret;
		val->intval = (((temp_val[0] & 0x7f) << 8) + temp_val[1]) * 1456;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = axp803_bat_get_max_voltage(bat_power);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		ret = regmap_read(bat_power->regmap, AXP803_VOFF_SET, &reg_value);
		if (ret)
			return ret;
		val->intval = 2600000 + 100000 * (reg_value & AXP803_V_OFF_MASK);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		ret = regmap_read(bat_power->regmap, AXP803_STATUS, &reg_value);
		if (ret)
			return ret;
		val->intval = !(reg_value & AXP803_STATUS_BAT_CUR_DIRCT);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		ret = regmap_read(bat_power->regmap, AXP803_MODE_CHGSTATUS, &reg_value);
		if (ret)
			return ret;
		val->intval = (reg_value & AXP803_CHGSTATUS_BAT_PRESENT) >> 5;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = axp803_get_vbat(bat_power) * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = (axp803_get_ibat(bat_power)
				- axp803_get_disibat(bat_power)) * 1000;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		ret = regmap_bulk_read(bat_power->regmap, AXP803_BATCAP0, temp_val, 2);
		if (ret < 0)
			return ret;
		val->intval = (((temp_val[0] & 0x7f) << 8) + temp_val[1]) * 1456;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = axp803_get_rest_cap(bat_power);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = axp803_get_bat_temp(bat_power) * 10;
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

static const struct power_supply_desc axp803_bat_desc = {
	.name = "axp803-battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.get_property = axp803_bat_get_property,
	.properties = axp803_bat_props,
	.num_properties = ARRAY_SIZE(axp803_bat_props),
};

static int axp803_bat_power_init(struct axp803_bat_power *bat_power)
{
	unsigned char ocv_cap[32];
	unsigned int val;
	int cur_coulomb_counter, rdc;
	int rest_pct;
	int i;
	int update_min_times[8] = {30, 60, 120, 164, 0, 5, 10, 20};
	int ocv_cou_adjust_time[4] = {60, 120, 15, 30};

	struct axp_config_info *axp_config = &bat_power->dts_info;
	struct regmap *map = bat_power->regmap;

	if (axp_config->pmu_init_chgend_rate == 10)
		val = 0;
	else
		val = 1;
	val <<= 4;
	regmap_update_bits(map, AXP803_CHARGE1, 0x10, val);

	if (axp_config->pmu_init_chg_pretime < 40)
		axp_config->pmu_init_chg_pretime = 40;

	if (axp_config->pmu_init_chg_csttime < 360)
		axp_config->pmu_init_chg_csttime = 360;

	val = ((((axp_config->pmu_init_chg_pretime - 40) / 10) << 6)
			| ((axp_config->pmu_init_chg_csttime - 360) / 120));
	regmap_update_bits(map, AXP803_CHARGE2, 0xc2, val);

	/* adc set */
	val = AXP803_ADC_BATVOL_ENABLE | AXP803_ADC_BATCUR_ENABLE;
	if (axp_config->pmu_bat_temp_enable != 0)
		val = val | AXP803_ADC_TSVOL_ENABLE;
	regmap_update_bits(map, AXP803_ADC_EN,
			AXP803_ADC_BATVOL_ENABLE
			| AXP803_ADC_BATCUR_ENABLE
			| AXP803_ADC_TSVOL_ENABLE,
			val);

	regmap_read(map, AXP803_ADC_SPEED_SET, &val);
	switch (axp_config->pmu_init_adc_freq / 100) {
	case 1:
		val &= ~(0x3 << 4);
		break;
	case 2:
		val &= ~(0x3 << 4);
		val |= 0x1 << 4;
		break;
	case 4:
		val &= ~(0x3 << 4);
		val |= 0x2 << 4;
		break;
	case 8:
		val |= 0x3 << 4;
		break;
	default:
		break;
	}

	if (axp_config->pmu_bat_temp_enable != 0)
		val &= (~(0x1 << 2));
	regmap_write(map, AXP803_ADC_SPEED_SET, val);

	regmap_read(map, AXP803_ADC_TS_CTL, &val);
	if (axp_config->pmu_bat_temp_enable != 0) {
		val &= 0xF8;
		val |= 0x02;
	} else {
		val &= 0xF8;
		val |= 0x04;
	}
	regmap_write(map, AXP803_ADC_TS_CTL, val);

	/* bat para */
	regmap_write(map, AXP803_WARNING_LEVEL,
		((axp_config->pmu_battery_warning_level1 - 5) << 4)
		+ axp_config->pmu_battery_warning_level2);

	/* set target voltage */
	if (axp_config->pmu_init_chgvol < 4150) {
		val = 0;
	} else if (axp_config->pmu_init_chgvol < 4200) {
		val = 1;
	} else if (axp_config->pmu_init_chgvol < 4350) {
		val = 2;
	} else {
		val = 3;
	}
	val <<= 5;
	regmap_update_bits(map, AXP803_CHARGE1, 0x60, val);

	ocv_cap[0]  = axp_config->pmu_bat_para1;
	ocv_cap[1]  = axp_config->pmu_bat_para2;
	ocv_cap[2]  = axp_config->pmu_bat_para3;
	ocv_cap[3]  = axp_config->pmu_bat_para4;
	ocv_cap[4]  = axp_config->pmu_bat_para5;
	ocv_cap[5]  = axp_config->pmu_bat_para6;
	ocv_cap[6]  = axp_config->pmu_bat_para7;
	ocv_cap[7]  = axp_config->pmu_bat_para8;
	ocv_cap[8]  = axp_config->pmu_bat_para9;
	ocv_cap[9]  = axp_config->pmu_bat_para10;
	ocv_cap[10] = axp_config->pmu_bat_para11;
	ocv_cap[11] = axp_config->pmu_bat_para12;
	ocv_cap[12] = axp_config->pmu_bat_para13;
	ocv_cap[13] = axp_config->pmu_bat_para14;
	ocv_cap[14] = axp_config->pmu_bat_para15;
	ocv_cap[15] = axp_config->pmu_bat_para16;
	ocv_cap[16] = axp_config->pmu_bat_para17;
	ocv_cap[17] = axp_config->pmu_bat_para18;
	ocv_cap[18] = axp_config->pmu_bat_para19;
	ocv_cap[19] = axp_config->pmu_bat_para20;
	ocv_cap[20] = axp_config->pmu_bat_para21;
	ocv_cap[21] = axp_config->pmu_bat_para22;
	ocv_cap[22] = axp_config->pmu_bat_para23;
	ocv_cap[23] = axp_config->pmu_bat_para24;
	ocv_cap[24] = axp_config->pmu_bat_para25;
	ocv_cap[25] = axp_config->pmu_bat_para26;
	ocv_cap[26] = axp_config->pmu_bat_para27;
	ocv_cap[27] = axp_config->pmu_bat_para28;
	ocv_cap[28] = axp_config->pmu_bat_para29;
	ocv_cap[29] = axp_config->pmu_bat_para30;
	ocv_cap[30] = axp_config->pmu_bat_para31;
	ocv_cap[31] = axp_config->pmu_bat_para32;
	regmap_bulk_write(map, AXP803_OCVCAP, ocv_cap, 32);

	/* Init CHGLED function */
	if (axp_config->ocv_coulumb_100 == 1) {
		rest_pct = axp803_get_rest_cap(bat_power);
		if (rest_pct == 100)
			regmap_update_bits(map, AXP803_OFF_CTL, 0x08, 0x00); /* disable CHGLED when force 100 */
		else {
			if (axp_config->pmu_chgled_func)
				regmap_update_bits(map, AXP803_OFF_CTL, 0x08, 0x08); /* by charger */
			else
				regmap_update_bits(map, AXP803_OFF_CTL, 0x08, 0x00); /* drive MOTO */
		}
	} else {
		if (axp_config->pmu_chgled_func)
			regmap_update_bits(map, AXP803_OFF_CTL, 0x08, 0x08); /* by charger */
		else
			regmap_update_bits(map, AXP803_OFF_CTL, 0x08, 0x00); /* drive MOTO */
	}

	/* set CHGLED Indication Type */
	if (axp_config->pmu_chgled_type)
		regmap_update_bits(map, AXP803_CHARGE2, 0x10, 0x10); /* Type B */
	else
		regmap_update_bits(map, AXP803_CHARGE2, 0x10, 0x00); /* Type A */

	/* Init battery capacity correct function */
	if (axp_config->pmu_batt_cap_correct)
		regmap_update_bits(map, AXP803_COULOMB_CTL, 0x20, 0x20);
	else
		regmap_update_bits(map, AXP803_COULOMB_CTL, 0x20, 0x00);

	/* Init battery regulator enable or not when charge finish */
	if (axp_config->pmu_chg_end_on_en)
		regmap_update_bits(map, AXP803_CHARGE2, 0x20, 0x20);
	else
		regmap_update_bits(map, AXP803_CHARGE2, 0x20, 0x00);

	if (axp_config->pmu_batdeten)
		regmap_update_bits(map, AXP803_OFF_CTL, 0x40, 0x40);
	else
		regmap_update_bits(map, AXP803_OFF_CTL, 0x40, 0x00);

	/* RDC initial */
	regmap_read(map, AXP803_RDC0, &val);
	if ((axp_config->pmu_battery_rdc) && (!(val & 0x40))) {
		rdc = (axp_config->pmu_battery_rdc * 10000 + 5371) / 10742;
		regmap_write(map, AXP803_RDC0, ((rdc >> 8) & 0x1F)|0x80);
		regmap_write(map, AXP803_RDC1, rdc & 0x00FF);
	}

	regmap_read(map, AXP803_BATCAP0, &val);
	if ((axp_config->pmu_battery_cap) && (!(val & 0x80))) {
		cur_coulomb_counter = axp_config->pmu_battery_cap
					* 1000 / 1456;
		regmap_write(map, AXP803_BATCAP0, ((cur_coulomb_counter >> 8) | 0x80));
		regmap_write(map, AXP803_BATCAP1, cur_coulomb_counter & 0x00FF);
	} else if (!axp_config->pmu_battery_cap) {
		regmap_write(map, AXP803_BATCAP0, 0x00);
		regmap_write(map, AXP803_BATCAP1, 0x00);
	}

	/*
	 * As datasheet decripted:
	 * TS_VOL = reg_value * 16 * 10K * 80ua
	 */
	if (axp_config->pmu_bat_temp_enable == 1) {
		regmap_write(map, AXP803_VLTF_CHARGE,
				axp_config->pmu_bat_charge_ltf * 10 / 128);
		regmap_write(map, AXP803_VHTF_CHARGE,
				axp_config->pmu_bat_charge_htf * 10 / 128);
		regmap_write(map, AXP803_VLTF_WORK,
				axp_config->pmu_bat_shutdown_ltf * 10 / 128);
		regmap_write(map, AXP803_VHTF_WORK,
				axp_config->pmu_bat_shutdown_htf * 10 / 128);
	}

	if (axp_config->pmu_ocv_en == 0) {
		pr_warn("axp803 ocv must be enabled\n");
		axp_config->pmu_ocv_en = 1;
	}
	if (axp_config->pmu_init_bc_en == 1) {
		regmap_update_bits(map, AXP803_BC_CTL, 0x01, 0x01);
	} else {
		regmap_update_bits(map, AXP803_BC_CTL, 0x01, 0x00);
	}

	if (axp_config->pmu_cou_en == 1) {
		/* use ocv and cou */
		regmap_update_bits(map, AXP803_COULOMB_CTL, 0x80, 0x80);
		regmap_update_bits(map, AXP803_COULOMB_CTL, 0x40, 0x40);
	} else if (axp_config->pmu_cou_en == 0) {
		/* only use ocv */
		regmap_update_bits(map, AXP803_COULOMB_CTL, 0x80, 0x80);
		regmap_update_bits(map, AXP803_COULOMB_CTL, 0x40, 0x00);
	}

	for (i = 0; i < ARRAY_SIZE(update_min_times); i++) {
		if (update_min_times[i] == axp_config->pmu_update_min_time)
			break;
	}
	regmap_update_bits(map, AXP803_ADJUST_PARA, 0x07, i);
	for (i = 0; i < ARRAY_SIZE(ocv_cou_adjust_time); i++) {
		if (ocv_cou_adjust_time[i] == axp_config->pmu_ocv_cou_adjust_time)
			break;
	}
	i <<= 6;
	regmap_update_bits(map, AXP803_ADJUST_PARA1, 0xc0, i);

	axp803_set_bat_chg_cur(bat_power, axp_config->pmu_runtime_chgcur);

	return 0;
}

static irqreturn_t axp803_bat_power_irq(int irq, void *data)
{
	struct axp803_bat_power *bat_power = data;

	power_supply_changed(bat_power->bat_supply);

	return IRQ_HANDLED;
}

enum axp803_bat_power_virqs {
	AXP803_VIRQ_CHAST,
	AXP803_VIRQ_CHAOV,
	AXP803_VIRQ_BATIN,
	AXP803_VIRQ_BATRE,
	AXP803_VIRQ_BATINWORK,
	AXP803_VIRQ_BATOVWORK,
	AXP803_VIRQ_BATINCHG,
	AXP803_VIRQ_BATOVCHG,
	AXP803_VIRQ_LOWN2,
	AXP803_VIRQ_LOWN1,

	AXP803_BAT_VIRQ_MAX_VIRQ,
};

static struct axp_interrupts axp803_bat_irq[] = {
	[AXP803_VIRQ_CHAST] = {"charging", axp803_bat_power_irq},
	[AXP803_VIRQ_CHAOV] = {"charge over", axp803_bat_power_irq},
	[AXP803_VIRQ_BATIN] = {"bat in", axp803_bat_power_irq},
	[AXP803_VIRQ_BATRE] = {"bat out", axp803_bat_power_irq},
	[AXP803_VIRQ_BATINWORK] = {"bat untemp work", axp803_bat_power_irq},
	[AXP803_VIRQ_BATOVWORK] = {"bat ovtemp work", axp803_bat_power_irq},
	[AXP803_VIRQ_BATINCHG] = {"bat untemp chg", axp803_bat_power_irq},
	[AXP803_VIRQ_BATOVCHG] = {"bat ovtemp chg", axp803_bat_power_irq},
	[AXP803_VIRQ_LOWN2] = {"low warning2", axp803_bat_power_irq},
	[AXP803_VIRQ_LOWN1] = {"low warning1", axp803_bat_power_irq},
};

static void axp803_bat_power_monitor(struct work_struct *work)
{
	int rest_pct;
	unsigned int reg_value, reg_value1;
	int ocv_pct, col_pct;

	struct axp803_bat_power *bat_power =
		container_of(work, typeof(*bat_power), bat_supply_mon.work);

	struct axp_config_info *axp_config = &bat_power->dts_info;

	power_supply_changed(bat_power->bat_supply);

	rest_pct = axp803_get_rest_cap(bat_power);

	/* force CHGLED disable when percentage is 100%, and turn on by else */
	if (axp_config->ocv_coulumb_100 == 1) {
		if (rest_pct == 100)
			regmap_update_bits(bat_power->regmap, AXP803_OFF_CTL, 0x08, 0x00); /* disable CHGLED */
		else {
			if (axp_config->pmu_chgled_func)
				regmap_update_bits(bat_power->regmap, AXP803_OFF_CTL, 0x08, 0x08); /* by charger */
			else
				regmap_update_bits(bat_power->regmap, AXP803_OFF_CTL, 0x08, 0x00); /* drive MOTO */
		}
	}

	/* force DISCHARGEING when percentage is 100%, and turn on when 99% */
	regmap_read(bat_power->regmap, AXP803_OCV_PERCENT, &reg_value);
	if (reg_value & 0x80)
		ocv_pct = (int)(reg_value & 0x7F);
	else
		ocv_pct = 0;

	regmap_read(bat_power->regmap, AXP803_COU_PERCENT, &reg_value);
	if (reg_value & 0x80)
		col_pct = (int)(reg_value & 0x7F);
	else
		col_pct = 0;

	regmap_read(bat_power->regmap, AXP803_STATUS, &reg_value);
	reg_value &= 0x24;
	regmap_read(bat_power->regmap, AXP803_MODE_CHGSTATUS, &reg_value1);
	reg_value1 &= 0x32;
	if (reg_value == 36 || reg_value == 32 || reg_value == 4) {
		if (ocv_pct == 100 && col_pct == 100) {
			rest_pct = 100;
			regmap_read(bat_power->regmap, AXP803_CAP, &reg_value);
			reg_value &= 0x7F;
			reg_value |= rest_pct;
			regmap_write(bat_power->regmap, AXP803_CAP, reg_value); /* rest_pct == 100 */
			regmap_read(bat_power->regmap, AXP803_OCV_PERCENT, &reg_value);
			reg_value &= 0x7F;
			reg_value |= 100;
			regmap_write(bat_power->regmap, AXP803_OCV_PERCENT, reg_value);/* ocv_pct == 100 */
			regmap_read(bat_power->regmap, AXP803_COU_PERCENT, &reg_value);
			reg_value &= 0x7F;
			reg_value |= 100;
			regmap_write(bat_power->regmap, AXP803_COU_PERCENT, reg_value);/* col_pct == 100 */
			regmap_update_bits(bat_power->regmap, AXP803_CHARGE1, 0x80, 0x00); /* discharge */
		} else if (rest_pct <= 99) {
			regmap_update_bits(bat_power->regmap, AXP803_CHARGE1, 0x80, 0x80); /* charge */
		}
	} else if (reg_value1) {
	regmap_update_bits(bat_power->regmap, AXP803_CHARGE1, 0x80, 0x80); /* charge */
	}

	/* debug info */
	if (unlikely(axp_debug_mask & AXP_CHG)) {

		unsigned char temp_val[2];
		unsigned int tmp;
		unsigned int col_ctl_reg;
		int ic_temp, vbat, ocv_vol;
		int charge_ibat, dis_ibat, ibat;
		int ocv_pct, col_pct;
		int rdc, batt_max_cap, coulumb_counter;
		bool bat_cur_dir, usb_det, ac_det, ext_valid;

		regmap_bulk_read(bat_power->regmap, AXP803_INTTEMP, temp_val, 2);
		tmp = (temp_val[0] << 4) + (temp_val[1] & 0x0F);
		ic_temp = (int)(tmp * 1063 / 10000  - 2667 / 10);

		vbat = axp803_get_vbat(bat_power);

		charge_ibat = axp803_get_ibat(bat_power);
		dis_ibat = axp803_get_disibat(bat_power);
		ibat = charge_ibat - dis_ibat;

		regmap_bulk_read(bat_power->regmap, AXP803_OCVBATH_RES, temp_val, 2);
		ocv_vol = ((temp_val[0] << 4) | (temp_val[1] & 0xF)) * 1100 / 1000;

		rest_pct = axp803_get_rest_cap(bat_power);

		regmap_read(bat_power->regmap, AXP803_OCV_PERCENT, &reg_value);
		if (reg_value & 0x80)
			ocv_pct = (int)(reg_value & 0x7F);
		else
			ocv_pct = 0;

		regmap_read(bat_power->regmap, AXP803_COU_PERCENT, &reg_value);
		if (reg_value & 0x80)
			col_pct = (int)(reg_value & 0x7F);
		else
			col_pct = 0;

		regmap_bulk_read(bat_power->regmap, AXP803_RDC0, temp_val, 2);
		rdc = (((temp_val[0] & 0x1f) << 8) + temp_val[1]) * 10742 / 10000;

		regmap_bulk_read(bat_power->regmap, AXP803_BATCAP0, temp_val, 2);
		batt_max_cap = (((temp_val[0] & 0x7f) << 8) + temp_val[1])
							* 1456 / 1000;

		regmap_bulk_read(bat_power->regmap, AXP803_COUCNT0, temp_val, 2);
		coulumb_counter = (((temp_val[0] & 0x7f) << 8) + temp_val[1])
							* 1456 / 1000;

		regmap_read(bat_power->regmap, AXP803_STATUS, &reg_value);
		bat_cur_dir = (reg_value & 0x04) ? 1 : 0;
		ac_det = (reg_value & 0x80) ? 1 : 0;
		usb_det = (reg_value & 0x20) ? 1 : 0;
		ext_valid = ac_det || usb_det;

		regmap_read(bat_power->regmap, AXP803_COULOMB_CTL, &reg_value);
		col_ctl_reg = reg_value;

		printk("ic_temp = %d\n", ic_temp);
		printk("vbat = %d\n", vbat);
		printk("ibat = %d\n", ibat);
		printk("charge_ibat = %d\n", charge_ibat);
		printk("dis_ibat = %d\n", dis_ibat);
		printk("ocv = %d\n", ocv_vol);
		printk("rest_vol = %d\n", rest_pct);
		printk("rdc = %d\n", rdc);
		printk("batt_max_cap = %d\n", batt_max_cap);
		printk("coulumb_counter = %d\n", coulumb_counter);
		printk("AXP803_COULOMB_CTL = 0x%x\n", col_ctl_reg);
		printk("ocv_percentage = %d\n", ocv_pct);
		printk("col_percentage = %d\n", col_pct);
		printk("bat_current_direction = %d\n", bat_cur_dir);
		printk("ext_valid = %d\n", ext_valid);
	}

	schedule_delayed_work(&bat_power->bat_supply_mon, msecs_to_jiffies(10 * 1000));
}

static int axp803_bat_power_dt_parse(struct axp803_bat_power *bat_power)
{
	struct axp_config_info *axp_config = &bat_power->dts_info;
	struct device_node *node = bat_power->dev->of_node;

	if (!of_device_is_available(node)) {
		pr_err("%s: failed\n", __func__);
		return -1;
	}

	AXP_OF_PROP_READ(pmu_battery_rdc,              BATRDC);
	AXP_OF_PROP_READ(pmu_battery_cap,                4000);
	AXP_OF_PROP_READ(pmu_batdeten,                      1);
	AXP_OF_PROP_READ(pmu_chg_ic_temp,                   0);
	AXP_OF_PROP_READ(pmu_runtime_chgcur, INTCHGCUR / 1000);
	AXP_OF_PROP_READ(pmu_suspend_chgcur,             1200);
	AXP_OF_PROP_READ(pmu_shutdown_chgcur,            1200);
	AXP_OF_PROP_READ(pmu_init_chgvol,    INTCHGVOL / 1000);
	AXP_OF_PROP_READ(pmu_init_chgend_rate,  INTCHGENDRATE);
	AXP_OF_PROP_READ(pmu_init_chg_enabled,              1);
	AXP_OF_PROP_READ(pmu_init_bc_en,                    0);
	AXP_OF_PROP_READ(pmu_init_adc_freq,        INTADCFREQ);
	AXP_OF_PROP_READ(pmu_init_adcts_freq,     INTADCFREQC);
	AXP_OF_PROP_READ(pmu_init_chg_pretime,  INTCHGPRETIME);
	AXP_OF_PROP_READ(pmu_init_chg_csttime,  INTCHGCSTTIME);
	AXP_OF_PROP_READ(pmu_batt_cap_correct,              1);
	AXP_OF_PROP_READ(pmu_chg_end_on_en,                 0);
	AXP_OF_PROP_READ(ocv_coulumb_100,                   0);
	AXP_OF_PROP_READ(pmu_bat_para1,               OCVREG0);
	AXP_OF_PROP_READ(pmu_bat_para2,               OCVREG1);
	AXP_OF_PROP_READ(pmu_bat_para3,               OCVREG2);
	AXP_OF_PROP_READ(pmu_bat_para4,               OCVREG3);
	AXP_OF_PROP_READ(pmu_bat_para5,               OCVREG4);
	AXP_OF_PROP_READ(pmu_bat_para6,               OCVREG5);
	AXP_OF_PROP_READ(pmu_bat_para7,               OCVREG6);
	AXP_OF_PROP_READ(pmu_bat_para8,               OCVREG7);
	AXP_OF_PROP_READ(pmu_bat_para9,               OCVREG8);
	AXP_OF_PROP_READ(pmu_bat_para10,              OCVREG9);
	AXP_OF_PROP_READ(pmu_bat_para11,              OCVREGA);
	AXP_OF_PROP_READ(pmu_bat_para12,              OCVREGB);
	AXP_OF_PROP_READ(pmu_bat_para13,              OCVREGC);
	AXP_OF_PROP_READ(pmu_bat_para14,              OCVREGD);
	AXP_OF_PROP_READ(pmu_bat_para15,              OCVREGE);
	AXP_OF_PROP_READ(pmu_bat_para16,              OCVREGF);
	AXP_OF_PROP_READ(pmu_bat_para17,             OCVREG10);
	AXP_OF_PROP_READ(pmu_bat_para18,             OCVREG11);
	AXP_OF_PROP_READ(pmu_bat_para19,             OCVREG12);
	AXP_OF_PROP_READ(pmu_bat_para20,             OCVREG13);
	AXP_OF_PROP_READ(pmu_bat_para21,             OCVREG14);
	AXP_OF_PROP_READ(pmu_bat_para22,             OCVREG15);
	AXP_OF_PROP_READ(pmu_bat_para23,             OCVREG16);
	AXP_OF_PROP_READ(pmu_bat_para24,             OCVREG17);
	AXP_OF_PROP_READ(pmu_bat_para25,             OCVREG18);
	AXP_OF_PROP_READ(pmu_bat_para26,             OCVREG19);
	AXP_OF_PROP_READ(pmu_bat_para27,             OCVREG1A);
	AXP_OF_PROP_READ(pmu_bat_para28,             OCVREG1B);
	AXP_OF_PROP_READ(pmu_bat_para29,             OCVREG1C);
	AXP_OF_PROP_READ(pmu_bat_para30,             OCVREG1D);
	AXP_OF_PROP_READ(pmu_bat_para31,             OCVREG1E);
	AXP_OF_PROP_READ(pmu_bat_para32,             OCVREG1F);
	AXP_OF_PROP_READ(pmu_pwroff_vol,                 3300);
	AXP_OF_PROP_READ(pmu_pwron_vol,                  2900);
	AXP_OF_PROP_READ(pmu_battery_warning_level1,       15);
	AXP_OF_PROP_READ(pmu_battery_warning_level2,        0);
	AXP_OF_PROP_READ(pmu_restvol_adjust_time,          30);
	AXP_OF_PROP_READ(pmu_ocv_cou_adjust_time,          60);
	AXP_OF_PROP_READ(pmu_chgled_func,                   0);
	AXP_OF_PROP_READ(pmu_chgled_type,                   0);
	AXP_OF_PROP_READ(pmu_bat_temp_enable,               0);
	AXP_OF_PROP_READ(pmu_bat_charge_ltf,             0xA5);
	AXP_OF_PROP_READ(pmu_bat_charge_htf,             0x1F);
	AXP_OF_PROP_READ(pmu_bat_shutdown_ltf,           0xFC);
	AXP_OF_PROP_READ(pmu_bat_shutdown_htf,           0x16);
	AXP_OF_PROP_READ(pmu_bat_temp_para1,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para2,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para3,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para4,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para5,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para6,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para7,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para8,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para9,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para10,               0);
	AXP_OF_PROP_READ(pmu_bat_temp_para11,               0);
	AXP_OF_PROP_READ(pmu_bat_temp_para12,               0);
	AXP_OF_PROP_READ(pmu_bat_temp_para13,               0);
	AXP_OF_PROP_READ(pmu_bat_temp_para14,               0);
	AXP_OF_PROP_READ(pmu_bat_temp_para15,               0);
	AXP_OF_PROP_READ(pmu_bat_temp_para16,               0);
	AXP_OF_PROP_READ(pmu_bat_unused,                    0);
	AXP_OF_PROP_READ(power_start,                       0);
	AXP_OF_PROP_READ(pmu_ocv_en,                        1);
	AXP_OF_PROP_READ(pmu_cou_en,                        1);
	AXP_OF_PROP_READ(pmu_update_min_time,   UPDATEMINTIME);

	axp_config->wakeup_bat_in =
		of_property_read_bool(node, "wakeup_bat_in");
	axp_config->wakeup_bat_out =
		of_property_read_bool(node, "wakeup_bat_out");
	axp_config->wakeup_bat_charging =
		of_property_read_bool(node, "wakeup_bat_charging");
	axp_config->wakeup_bat_charge_over =
		of_property_read_bool(node, "wakeup_bat_charge_over");
	axp_config->wakeup_low_warning1 =
		of_property_read_bool(node, "wakeup_low_warning1");
	axp_config->wakeup_low_warning2 =
		of_property_read_bool(node, "wakeup_low_warning2");
	axp_config->wakeup_bat_untemp_work =
		of_property_read_bool(node, "wakeup_bat_untemp_work");
	axp_config->wakeup_bat_ovtemp_work =
		of_property_read_bool(node, "wakeup_bat_ovtemp_work");
	axp_config->wakeup_untemp_chg =
		of_property_read_bool(node, "wakeup_bat_untemp_chg");
	axp_config->wakeup_ovtemp_chg =
		of_property_read_bool(node, "wakeup_bat_ovtemp_chg");

	return 0;
}

static int axp803_bat_power_probe(struct platform_device *pdev)
{
	struct axp20x_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct power_supply_config psy_cfg = {};
	struct axp803_bat_power *bat_power;
	int i, irq;
	int ret = 0;

	if (!axp_dev->irq) {
		pr_err("can not register axp803 bat without irq\n");
		return -EINVAL;
	}

	bat_power = devm_kzalloc(&pdev->dev, sizeof(*bat_power), GFP_KERNEL);
	if (!bat_power) {
		pr_err("axp803 bat power alloc failed\n");
		ret = -ENOMEM;
		return ret;
	}

	bat_power->name = "axp803-bat-power";
	bat_power->dev = &pdev->dev;
	bat_power->regmap = axp_dev->regmap;

	platform_set_drvdata(pdev, bat_power);

	ret = axp803_bat_power_dt_parse(bat_power);
	if (ret) {
		pr_err("%s parse device tree err\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	ret = axp803_bat_power_init(bat_power);
	if (ret < 0) {
		pr_err("axp210x init bat fail!\n");
		ret = -ENODEV;
		return ret;
	}

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = bat_power;

	bat_power->bat_supply = devm_power_supply_register(bat_power->dev,
			&axp803_bat_desc, &psy_cfg);

	if (IS_ERR(bat_power->bat_supply)) {
		pr_err("axp803 failed to register bat power\n");
		ret = PTR_ERR(bat_power->bat_supply);
		return ret;
	}

	for (i = 0; i < ARRAY_SIZE(axp803_bat_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp803_bat_irq[i].name);
		if (irq < 0) {
			dev_warn(&pdev->dev, "No IRQ for %s: %d\n",
				 axp803_bat_irq[i].name, irq);
			continue;
		}
		irq = regmap_irq_get_virq(axp_dev->regmap_irqc, irq);
		ret = devm_request_any_context_irq(&pdev->dev, irq,
						   axp803_bat_irq[i].isr, 0,
						   axp803_bat_irq[i].name, bat_power);
		if (ret < 0)
			dev_warn(&pdev->dev, "Error requesting %s IRQ %d: %d\n",
				axp803_bat_irq[i].name, irq, ret);

		dev_dbg(&pdev->dev, "Requested %s IRQ %d: %d\n",
			axp803_bat_irq[i].name, irq, ret);

		/* we use this variable to suspend irq */
		axp803_bat_irq[i].irq = irq;
	}


	INIT_DELAYED_WORK(&bat_power->bat_supply_mon, axp803_bat_power_monitor);
	schedule_delayed_work(&bat_power->bat_supply_mon, msecs_to_jiffies(10 * 1000));

	return 0;
}

static int axp803_bat_power_remove(struct platform_device *pdev)
{
	struct axp803_bat_power *bat_power = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&bat_power->bat_supply_mon);

	return 0;
}

static inline void axp_irq_set(unsigned int irq, bool enable)
{
	if (enable)
		enable_irq(irq);
	else
		disable_irq(irq);
}

static void axp803_bat_virq_dts_set(struct axp803_bat_power *bat_power, bool enable)
{
	struct axp_config_info *dts_info = &bat_power->dts_info;

	if (!dts_info->wakeup_bat_in)
		axp_irq_set(axp803_bat_irq[AXP803_VIRQ_BATIN].irq,
				enable);
	if (!dts_info->wakeup_bat_out)
		axp_irq_set(axp803_bat_irq[AXP803_VIRQ_BATRE].irq,
				enable);
	if (!dts_info->wakeup_bat_charging)
		axp_irq_set(axp803_bat_irq[AXP803_VIRQ_CHAST].irq,
				enable);
	if (!dts_info->wakeup_bat_charge_over)
		axp_irq_set(axp803_bat_irq[AXP803_VIRQ_CHAOV].irq,
				enable);
	if (!dts_info->wakeup_low_warning1)
		axp_irq_set(axp803_bat_irq[AXP803_VIRQ_LOWN1].irq,
				enable);
	if (!dts_info->wakeup_low_warning2)
		axp_irq_set(axp803_bat_irq[AXP803_VIRQ_LOWN2].irq,
				enable);
	if (!dts_info->wakeup_bat_untemp_work)
		axp_irq_set(
			axp803_bat_irq[AXP803_VIRQ_BATINWORK].irq,
			enable);
	if (!dts_info->wakeup_bat_ovtemp_work)
		axp_irq_set(
			axp803_bat_irq[AXP803_VIRQ_BATOVWORK].irq,
			enable);
	if (!dts_info->wakeup_untemp_chg)
		axp_irq_set(
			axp803_bat_irq[AXP803_VIRQ_BATINCHG].irq,
			enable);
	if (!dts_info->wakeup_ovtemp_chg)
		axp_irq_set(
			axp803_bat_irq[AXP803_VIRQ_BATOVCHG].irq,
			enable);
}

static void axp803_bat_power_shutdown(struct platform_device *pdev)
{
	struct axp803_bat_power *bat_power = platform_get_drvdata(pdev);

	axp803_set_bat_chg_cur(bat_power, bat_power->dts_info.pmu_shutdown_chgcur);
}


static int axp803_bat_power_suspend(struct platform_device *pdev, pm_message_t state)
{
	int rest_pct;

	struct axp803_bat_power *bat_power = platform_get_drvdata(pdev);
	struct axp_config_info *axp_config = &bat_power->dts_info;

	rest_pct = axp803_get_rest_cap(bat_power);

	/* force CHGLED disable when percentage is 100%, and turn on by else */
	if (axp_config->ocv_coulumb_100 == 1) {
		if (rest_pct == 100)
			regmap_update_bits(bat_power->regmap, AXP803_OFF_CTL, 0x08, 0x00); /* disable CHGLED */
		else {
			if (axp_config->pmu_chgled_func)
				regmap_update_bits(bat_power->regmap, AXP803_OFF_CTL, 0x08, 0x08); /* by charger */
			else
				regmap_update_bits(bat_power->regmap, AXP803_OFF_CTL, 0x08, 0x00); /* drive MOTO */
		}
	}

	axp803_set_bat_chg_cur(bat_power, bat_power->dts_info.pmu_suspend_chgcur);

	axp803_bat_virq_dts_set(bat_power, false);

	return 0;
}

static int axp803_bat_power_resume(struct platform_device *pdev)
{
	int rest_pct;

	struct axp803_bat_power *bat_power = platform_get_drvdata(pdev);
	struct axp_config_info *axp_config = &bat_power->dts_info;

	power_supply_changed(bat_power->bat_supply);

	rest_pct = axp803_get_rest_cap(bat_power);

	/* force CHGLED disable when percentage is 100%, and turn on by else */
	if (axp_config->ocv_coulumb_100 == 1) {
		if (rest_pct == 100)
			regmap_update_bits(bat_power->regmap, AXP803_OFF_CTL, 0x08, 0x00); /* disable CHGLED */
		else {
			if (axp_config->pmu_chgled_func)
				regmap_update_bits(bat_power->regmap, AXP803_OFF_CTL, 0x08, 0x08); /* by charger */
			else
				regmap_update_bits(bat_power->regmap, AXP803_OFF_CTL, 0x08, 0x00); /* drive MOTO */
		}
	}

	axp803_set_bat_chg_cur(bat_power, bat_power->dts_info.pmu_runtime_chgcur);

	axp803_bat_virq_dts_set(bat_power, true);

	return 0;
}

static const struct of_device_id axp803_bat_power_match[] = {
	{
		.compatible = "x-powers,axp803-battery-power-supply",
		.data = (void *)AXP803_ID,
	}, { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, axp803_bat_power_match);

static struct platform_driver axp803_bat_power_driver = {
	.driver = {
		.name = "axp803-battery-power-supply",
		.of_match_table = axp803_bat_power_match,
	},
	.probe = axp803_bat_power_probe,
	.remove = axp803_bat_power_remove,
	.shutdown = axp803_bat_power_shutdown,
	.suspend = axp803_bat_power_suspend,
	.resume = axp803_bat_power_resume,
};

module_platform_driver(axp803_bat_power_driver);

MODULE_AUTHOR("wangxiaoliang <wangxiaoliang@x-powers.com>");
MODULE_DESCRIPTION("axp803 bat power driver");
MODULE_LICENSE("GPL");
