/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include "axp517_charger.h"

#define DIVIDE_BY_64(n) (((n) >> 6) << 6)

struct axp517_bat_power {
	char			*name;
	struct device		*dev;
	struct regmap		*regmap;
	struct power_supply	*bat_supply;
	struct delayed_work	bat_supply_mon;
	struct axp_config_info	dts_info;
	struct mutex		lock;

	/* bat_temp_process */
	struct delayed_work	bat_temp_init;
	int					bat_temp_calib;
	int			bat_charge_ltf;
	int			bat_charge_htf;
	int			bat_shutdown_ltf;
	int			bat_shutdown_htf;

	/* power supply notifier */
	struct notifier_block	bat_nb;

	/* extcon_dev */
	struct extcon_dev         *edev;

	/* charge_limit_process */
	atomic_t	 	pmu_limit_status;

	/* fake bat soc */
	struct delayed_work	bat_power_curve;
	struct wakeup_source	*ws;
	atomic_t		bat_radio_check;
	int			axp517_suspend_flag;

	/* power debugfs */
	struct			sunxi_power_debug_data *debug;
};

static enum power_supply_property axp517_bat_props[] = {
	/* real_time */
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CAPACITY_LEVEL,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
	POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CYCLE_COUNT,
	/* static */
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_TEMP_ALERT_MIN,
	POWER_SUPPLY_PROP_TEMP_ALERT_MAX,
	POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MIN,
	POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MAX,
	POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX,
	POWER_SUPPLY_PROP_MANUFACTURE_YEAR,
	POWER_SUPPLY_PROP_MANUFACTURE_MONTH,
	POWER_SUPPLY_PROP_MANUFACTURE_DAY,
};

static const unsigned int axp517_pd_extcon_cable[] = {
	EXTCON_CHG_USB_PD,
	EXTCON_NONE,
};

static int axp517_reset_mcu(struct axp517_bat_power *bat_power);

static inline int axp517_vbat_to_mV(u32 reg)
{
	return (int)(reg & 0x3FFF);
}

static inline int axp517_ibat_to_mA(u16 reg)
{
	int val;

	if (reg & 0x8000)
		val = (int)((reg - 0xFFFF) & 0x3FFF);
	else
		val = (int)(reg & 0x3FFF);

	return val / 4;
}

static inline int axp517_vts_to_mV(u32 reg)
{
	return (int)(reg & 0x3FFF) / 2;
}

static inline u32 axp517_get_adc_data_raw(struct regmap *regmap, int type)
{
	unsigned int reg_value;
	u32 ts_res;
	u8 temp_val[2];
	int ret = 0;

	if (type < 0)
		return ret;

	ret = regmap_read(regmap, AXP517_ADC_CONTROL, &reg_value);
	if (ret < 0)
		return ret;

	if ((reg_value & 0xf) != type) {
		reg_value &= ~(0xf);
		reg_value |= type;
		ret = regmap_write(regmap, AXP517_ADC_CONTROL, reg_value);
		if (ret < 0)
			return ret;
		mdelay(1);
	}

	ret = regmap_bulk_read(regmap, AXP517_ADC_RES, temp_val, 2);
	if (ret < 0)
		return ret;

	ts_res = (temp_val[0] << 8) | temp_val[1];

	return ts_res;
}

static int axp517_ichg(struct regmap *regmap)
{
	int ibat;
	u32 ibat_adc;

	ibat_adc = axp517_get_adc_data_raw(regmap, AXP517_ADC_ICHG);
	ibat = axp517_ibat_to_mA(ibat_adc);

	return ibat;
}

static int axp517_disichg(struct regmap *regmap)
{
	int disibat;
	u32 ibat_adc;

	ibat_adc = axp517_get_adc_data_raw(regmap, AXP517_ADC_IDCHG);
	disibat = axp517_ibat_to_mA(ibat_adc);

	return disibat;
}

static int axp517_bat_is_use_fake_curve_status(struct regmap *regmap)
{
	unsigned int reg_value;
	int ret = 0;

	ret = regmap_read(regmap, AXP517_DATA_BUFF, &reg_value);
	if (ret < 0)
		return ret;

	if (reg_value & 0x80) {
		return (reg_value & 0x7F);
	} else {
		return -1;
	}
}

static int axp517_get_bat_present(struct axp517_bat_power *bat_power)
{
	struct regmap *regmap = bat_power->regmap;
	unsigned int reg_value;
	int ret = 0;

	ret = regmap_read(regmap, AXP517_STATUS0, &reg_value);
	if (ret < 0)
		return 0;

	return !!(reg_value & AXP517_MASK_BAT_STAT);
}

static int _axp517_get_soc(struct axp517_bat_power *bat_power)
{
	struct regmap *regmap = bat_power->regmap;
	unsigned int reg_value;
	int ret = 0;

	if (!axp517_get_bat_present(bat_power)) {
		return -1;
	}

	ret = axp517_bat_is_use_fake_curve_status(regmap);
	if (ret >= 0) {
		return ret;
	}

	ret = regmap_read(regmap, AXP517_GAUGE_SOC, &reg_value);
	if (ret < 0)
		return ret;

	return (int)(reg_value & 0x7F);
}

static int axp517_get_soc(struct axp517_bat_power *bat_power)
{
	static int old_val;

	if (!atomic_read(&bat_power->bat_radio_check))
		old_val = _axp517_get_soc(bat_power);

	return old_val;
}

/* read temperature */
static inline int axp_vts_to_temp(int data,
		const struct axp_config_info *axp_config)
{
	int temp;

	if (!axp_config->pmu_bat_temp_enable)
		return 300;
	else if (data < axp_config->pmu_bat_temp_para16)
		return 800;
	else if (data <= axp_config->pmu_bat_temp_para15) {
		temp = 700 + (axp_config->pmu_bat_temp_para15-data) * 100 /
		(axp_config->pmu_bat_temp_para15-axp_config->pmu_bat_temp_para16);
	} else if (data <= axp_config->pmu_bat_temp_para14) {
		temp = 600 + (axp_config->pmu_bat_temp_para14-data) * 100 /
		(axp_config->pmu_bat_temp_para14-axp_config->pmu_bat_temp_para15);
	} else if (data <= axp_config->pmu_bat_temp_para13) {
		temp = 550 + (axp_config->pmu_bat_temp_para13-data) * 50 /
		(axp_config->pmu_bat_temp_para13-axp_config->pmu_bat_temp_para14);
	} else if (data <= axp_config->pmu_bat_temp_para12) {
		temp = 500 + (axp_config->pmu_bat_temp_para12-data) * 50 /
		(axp_config->pmu_bat_temp_para12-axp_config->pmu_bat_temp_para13);
	} else if (data <= axp_config->pmu_bat_temp_para11) {
		temp = 450 + (axp_config->pmu_bat_temp_para11-data) * 50 /
		(axp_config->pmu_bat_temp_para11-axp_config->pmu_bat_temp_para12);
	} else if (data <= axp_config->pmu_bat_temp_para10) {
		temp = 400 + (axp_config->pmu_bat_temp_para10-data) * 50 /
		(axp_config->pmu_bat_temp_para10-axp_config->pmu_bat_temp_para11);
	} else if (data <= axp_config->pmu_bat_temp_para9) {
		temp = 300 + (axp_config->pmu_bat_temp_para9-data) * 100 /
		(axp_config->pmu_bat_temp_para9-axp_config->pmu_bat_temp_para10);
	} else if (data <= axp_config->pmu_bat_temp_para8) {
		temp = 200 + (axp_config->pmu_bat_temp_para8-data) * 100 /
		(axp_config->pmu_bat_temp_para8-axp_config->pmu_bat_temp_para9);
	} else if (data <= axp_config->pmu_bat_temp_para7) {
		temp = 100 + (axp_config->pmu_bat_temp_para7-data) * 100 /
		(axp_config->pmu_bat_temp_para7-axp_config->pmu_bat_temp_para8);
	} else if (data <= axp_config->pmu_bat_temp_para6) {
		temp = 50 + (axp_config->pmu_bat_temp_para6-data) * 50 /
		(axp_config->pmu_bat_temp_para6-axp_config->pmu_bat_temp_para7);
	} else if (data <= axp_config->pmu_bat_temp_para5) {
		temp = 0 + (axp_config->pmu_bat_temp_para5-data) * 50 /
		(axp_config->pmu_bat_temp_para5-axp_config->pmu_bat_temp_para6);
	} else if (data <= axp_config->pmu_bat_temp_para4) {
		temp = -50 + (axp_config->pmu_bat_temp_para4-data) * 50 /
		(axp_config->pmu_bat_temp_para4-axp_config->pmu_bat_temp_para5);
	} else if (data <= axp_config->pmu_bat_temp_para3) {
		temp = -100 + (axp_config->pmu_bat_temp_para3-data) * 50 /
		(axp_config->pmu_bat_temp_para3-axp_config->pmu_bat_temp_para4);
	} else if (data <= axp_config->pmu_bat_temp_para2) {
		temp = -150 + (axp_config->pmu_bat_temp_para2-data) * 50 /
		(axp_config->pmu_bat_temp_para2-axp_config->pmu_bat_temp_para3);
	} else if (data <= axp_config->pmu_bat_temp_para1) {
		temp = -250 + (axp_config->pmu_bat_temp_para1-data) * 100 /
		(axp_config->pmu_bat_temp_para1-axp_config->pmu_bat_temp_para2);
	} else
		temp = -250;
	return temp;
}

static inline int axp517_get_bat_temp_adc_raw(struct regmap *regmap)
{
	unsigned char temp_val[2];
	int bat_temp_mv;
	u32 bat_temp_adc;
	int ret = 0;

	ret = regmap_bulk_read(regmap, AXP517_TS_H, temp_val, 2);
	if (ret < 0)
		return ret;

	temp_val[0] &= GENMASK(5, 0);
	bat_temp_adc = (temp_val[0] << 8) | temp_val[1];
	bat_temp_mv = axp517_vts_to_mV(bat_temp_adc);

	return bat_temp_mv;
}

static inline int axp517_get_bat_temp_adc(struct axp517_bat_power *bat_power)
{
	struct regmap *regmap = bat_power->regmap;
	int calib = bat_power->bat_temp_calib;
	int bat_temp_mv, data;
	bool bat_charging;

	regmap_read(regmap, AXP517_STATUS1, &data);
	bat_charging = ((((data & AXP517_MASK_CHARGE) > 0))
			&& ((data & AXP517_MASK_CHARGE) < AXP517_CHARGING_DONE)) ? 1 : 0;

	bat_temp_mv = axp517_get_bat_temp_adc_raw(regmap);

	if (!bat_charging)
		bat_temp_mv += (calib * axp517_disichg(regmap) / 1000);
	else
		bat_temp_mv -= (calib * axp517_ichg(regmap) / 1000);

	return bat_temp_mv;
}

static int _axp517_get_bat_temp_raw(struct axp517_bat_power *bat_power)
{
	struct axp_config_info *axp_config = &bat_power->dts_info;

	int i = 0, temp, old_temp;

	old_temp = axp_vts_to_temp(axp517_get_bat_temp_adc(bat_power), axp_config);

	/* read until abs(old_temp - temp) < 10*/
	temp = axp_vts_to_temp(axp517_get_bat_temp_adc(bat_power), axp_config);

	PMIC_DEBUG("old_temp:%d, temp:%d\n", old_temp, temp);
	while ((abs(old_temp - temp) > 100) && (i < 10)) {
		old_temp = temp;
		temp = axp_vts_to_temp(axp517_get_bat_temp_adc(bat_power), axp_config);
		i++;
		PMIC_DEBUG("turn[%d]:old_temp:%d, temp:%d\n", i, old_temp, temp);
	}

	return temp;
}

static int axp517_get_bat_temp(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp517_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct axp_config_info *axp_config = &bat_power->dts_info;

	if (!axp_config->pmu_bat_temp_enable) {
		val->intval = 300;
	} else {
		val->intval = _axp517_get_bat_temp_raw(bat_power);
	}

	return 0;
}

static int axp517_get_bat_health(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp517_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	unsigned int reg_value;
	int ret;

	if (!axp517_get_bat_present(bat_power)) {
		val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;
		return 0;
	}

	ret = regmap_read(regmap, AXP517_IRQ2, &reg_value);
	if (ret < 0) {
		val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;
		return ret;
	}

	if (reg_value & BIT(2)) {
		val->intval = POWER_SUPPLY_HEALTH_DEAD;
		return 0;
	}

	ret = regmap_read(regmap, AXP517_ILIM_TYPE, &reg_value);
	if (ret < 0) {
		val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;
		return ret;
	}

	switch ((reg_value & GENMASK(6, 4)) >> 4) {
	case 0x2:
	case 0x6:
		val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
		break;
	case 0x1:
	case 0x5:
		val->intval = POWER_SUPPLY_HEALTH_COLD;
		break;
	default:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
	}

	return 0;
}

static int axp517_get_bat_status(struct power_supply *ps,
				  union power_supply_propval *val)
{
	struct axp517_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	unsigned int data;
	int ret;

	if (!axp517_get_bat_present(bat_power)) {
		val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		return 0;
	}

	ret = regmap_read(regmap, AXP517_STATUS1, &data);
	if (ret < 0) {
		val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		PMIC_DEBUG("error read AXP517_COM_STAT1\n");
		return ret;
	}

	/* chg_stat = bit[2:0] */
	switch (data & 0x07) {
	case AXP517_CHARGING_TRI:
	case AXP517_CHARGING_NCHG:
		val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		break;
	case AXP517_CHARGING_PRE:
	case AXP517_CHARGING_CC:
	case AXP517_CHARGING_CV:
		val->intval = POWER_SUPPLY_STATUS_CHARGING;
		break;
	case AXP517_CHARGING_DONE:
		val->intval = POWER_SUPPLY_STATUS_FULL;
		break;
	default:
		val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		break;
	}

	ret = regmap_read(bat_power->regmap, AXP517_STATUS0, &data);
	if (ret < 0) {
		val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;
		return ret;
	}

	if (data & AXP517_MASK_VBUS_STAT) {
		data = axp517_get_soc(bat_power);
		if (data == 100)
			val->intval = POWER_SUPPLY_STATUS_FULL;
	}

	return 0;
}

static int axp517_get_vbat_vol(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp517_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	uint8_t tmp[2];
	u32 res;
	int ret;

	ret = regmap_bulk_read(regmap, AXP517_VBAT_H, tmp, 2);
	if (ret < 0)
		return ret;
	tmp[0] &= GENMASK(5, 0);
	res = (tmp[0] << 8) | tmp[1];

	val->intval = axp517_vbat_to_mV(res) * 1000;

	return 0;
}

static int axp517_get_ichg(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp517_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	uint8_t tmp[2];
	u32 res;
	int ret;

	ret = regmap_bulk_read(regmap, AXP517_IBAT_H, tmp, 2);
	if (ret < 0)
		return ret;
	res = (tmp[0] << 8) | tmp[1];

	val->intval = axp517_ibat_to_mA(res) * 1000;

	return 0;
}

static int axp517_bat_get_max_voltage(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp517_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct axp_config_info *axp_config = &bat_power->dts_info;

	val->intval = axp_config->pmu_init_chgvol;

	return 0;
}

static int _axp517_get_ichg_lim(struct regmap *regmap)
{
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, AXP517_ICC_CFG, &data);
	if (ret < 0)
		return ret;

	data &= GENMASK(6, 0);
	if (data > 80)
		data = 80;
	data = data * 64;

	return data;
}

static int axp517_get_ichg_lim(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp517_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	int ret = 0;

	ret = _axp517_get_ichg_lim(regmap);
	if (ret < 0)
		return ret;

	val->intval = ret;

	return 0;
}

static int _axp517_get_cycle_count(struct regmap *regmap)
{
	uint8_t tmp[2];
	u32 res;

	regmap_bulk_read(regmap, AXP517_CYCLE_H, tmp, 2);
	res = (tmp[0] << 8) | tmp[1];

	return res;
}

static int axp517_get_cycle_count(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp517_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;

	val->intval = _axp517_get_cycle_count(regmap);

	return 0;
}

static int axp517_get_charger_count(struct power_supply *ps,
		union power_supply_propval *val)
{
	struct axp517_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct axp_config_info *axp_config = &bat_power->dts_info;
	struct regmap *regmap = bat_power->regmap;
	unsigned int data;
	int charge_cycle_count = 0, reduce_cap;

	charge_cycle_count = _axp517_get_cycle_count(regmap);

	data = axp_config->pmu_battery_cap * 1000;
	reduce_cap = charge_cycle_count * axp_config->pmu_bat_cycle_cap_reduce;
	data -= reduce_cap;

	val->intval = data;

	return 0;
}

static int axp517_get_charger_count_current(struct power_supply *ps,
		union power_supply_propval *val)
{
	struct axp517_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct axp_config_info *axp_config = &bat_power->dts_info;
	unsigned int data[2];

	data[0] = axp_config->pmu_battery_cap * 1000;

	data[1] = axp517_get_soc(bat_power);
	data[1] = data[1] * data[0] / 100;

	val->intval = data[1];

	return 0;
}

static int axp517_get_lowsocth(struct power_supply *ps,
				 union power_supply_propval *val)
{
	struct axp517_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, AXP517_GAUGE_THLD, &data);
	if (ret < 0)
		return ret;

	val->intval = (data >> 4) + 5;

	return 0;
}


static int axp517_set_lowsocth(struct regmap *regmap, int v)
{
	unsigned int data;
	int ret = 0;

	data = v;

	if (data > 20 || data < 5)
		return -EINVAL;

	data = (data - 5);
	ret = regmap_update_bits(regmap, AXP517_GAUGE_THLD, GENMASK(7, 4),
				 data);
	if (ret < 0)
		return ret;

	return 0;
}

static int axp517_set_gauge_thld(struct regmap *regmap, int level1_v, int level2_v)
{
	level1_v = clamp_val(level1_v - 5, 0, 15);
	axp517_set_lowsocth(regmap, level1_v);

	if (level2_v > -1) {
		level2_v = clamp_val(level2_v, 0, 15);
		regmap_update_bits(regmap, AXP517_GAUGE_THLD, GENMASK(3, 0),
				 level2_v);
	}

	return 0;
}

static int _axp517_set_ichg(struct regmap *regmap, int mA)
{
	mA = mA / 64;
	if (mA > 80)
		mA = 80;
	/* bit 5:0 is the ctrl bit */
	regmap_update_bits(regmap, AXP517_ICC_CFG, GENMASK(6, 0), mA);

	return 0;
}

static int axp517_set_ichg(struct axp517_bat_power *bat_power, int mA)
{
	struct regmap *regmap = bat_power->regmap;

	_axp517_set_ichg(regmap, mA);
	SUNXI_POWER_LOG_INFO(bat_power->debug, "set ichg:%d", mA);

	return 0;
}

static int axp517_set_bat_max_voltage(struct regmap *regmap, int mV)
{
	unsigned int data;

	data = mV;

	if (data < 4100) {
		regmap_update_bits(regmap, AXP517_VTERM_CFG, 0x07, AXP517_CHRG_CTRL1_TGT_4_0V);
	} else if (data < 4200) {
		regmap_update_bits(regmap, AXP517_VTERM_CFG, 0x07, AXP517_CHRG_CTRL1_TGT_4_1V);
	} else if (data < 4350) {
		regmap_update_bits(regmap, AXP517_VTERM_CFG, 0x07, AXP517_CHRG_CTRL1_TGT_4_2V);
	} else if (data < 4400) {
		regmap_update_bits(regmap, AXP517_VTERM_CFG, 0x07, AXP517_CHRG_CTRL1_TGT_4_35V);
	} else if (data < 5000) {
		regmap_update_bits(regmap, AXP517_VTERM_CFG, 0x07, AXP517_CHRG_CTRL1_TGT_4_4V);
	} else {
		regmap_update_bits(regmap, AXP517_VTERM_CFG, 0x07, AXP517_CHRG_CTRL1_TGT_5_0V);
	}

	return 0;
}

static int axp517_set_status(struct axp517_bat_power *bat_power, int status)
{
	struct regmap *regmap = bat_power->regmap;

	if (!status) {
		PMIC_INFO("disable charge\n");
		if (!IS_ERR_OR_NULL(bat_power->edev))
			extcon_set_state_sync(bat_power->edev, EXTCON_CHG_USB_PD, false);
		regmap_update_bits(regmap, AXP517_MODULE_EN, BIT(3), 0);
		regmap_update_bits(regmap, AXP517_MODULE_EN, BIT(1), 0);
		mdelay(1000);
		regmap_update_bits(regmap, AXP517_MODULE_EN, BIT(3), BIT(3));
	} else {
		PMIC_INFO("enable charge\n");
		if (!IS_ERR_OR_NULL(bat_power->edev))
			extcon_set_state_sync(bat_power->edev, EXTCON_CHG_USB_PD, true);
		regmap_update_bits(regmap, AXP517_MODULE_EN, BIT(3), BIT(3));
		regmap_update_bits(regmap, AXP517_MODULE_EN, BIT(1), BIT(1));
	}

	return 0;
}

static int _axp517_reset_mcu(struct regmap *regmap)
{
	int ret = 0;

	ret = regmap_update_bits(regmap, AXP517_RESET_CFG, AXP517_MODE_RSTMCU,
				 AXP517_MODE_RSTMCU);
	if (ret < 0)
		return ret;

	ret = regmap_update_bits(regmap, AXP517_RESET_CFG, AXP517_MODE_RSTMCU,
				 0);
	if (ret < 0)
		return ret;

	PMIC_INFO("reset mcu\n");
	return 0;
}

static int axp517_reset_mcu(struct axp517_bat_power *bat_power)
{
	int ret = 0;
	struct regmap *regmap = bat_power->regmap;

	axp517_set_status(bat_power, 0);
	msleep(500);

	ret = _axp517_reset_mcu(regmap);

	msleep(500);
	axp517_set_status(bat_power, 1);

	return ret;
}

/**
 * axp517_get_param - get battery config from dts
 *
 * is not get battery config parameter from dts,
 * then it use the default config.
 */
static int axp517_get_param(struct axp517_bat_power *bat_power, uint8_t *para,
			     unsigned int *len)
{
	struct device_node *n_para, *r_para;
	const char *pparam;
	int cnt;

	n_para = of_parse_phandle(bat_power->dev->of_node, "param", 0);
	if (!n_para)
		goto e_n_para;

	if (of_property_read_string(n_para, "select", &pparam))
		goto e_para;

	r_para = of_get_child_by_name(n_para, pparam);
	if (!r_para)
		goto e_para;

	cnt = of_property_read_variable_u8_array(r_para, "parameter", para, 1,
						 *len);
	if (cnt <= 0)
		goto e_n_parameter;
	*len = cnt;

	of_node_put(r_para);
	of_node_put(n_para);

	return 0;

e_n_parameter:
	of_node_put(r_para);
e_para:
	of_node_put(n_para);
e_n_para:
	return -ENODATA;
}

static int axp517_model_update(struct axp517_bat_power *bat_power)
{
	struct regmap *regmap = bat_power->regmap;
	int ret = 0;
	unsigned int data;
	unsigned int len;
	uint8_t i;
	uint8_t *param;

	/* reset and open brom */
	ret = regmap_update_bits(regmap, AXP517_GAUGE_CONFIG,
				 AXP517_BROMUP_EN, 0);
	if (ret < 0)
		goto UPDATE_ERR;

	ret = regmap_update_bits(regmap, AXP517_GAUGE_CONFIG,
				 AXP517_BROMUP_EN, AXP517_BROMUP_EN);
	if (ret < 0)
		goto UPDATE_ERR;

	/* down load battery parameters */
	len = AXP517_MAX_PARAM;
	param = devm_kzalloc(bat_power->dev, AXP517_MAX_PARAM, GFP_KERNEL);
	if (!param) {
		PMIC_ERR("can not find memory for param\n");
		goto UPDATE_ERR;
	}
	ret = axp517_get_param(bat_power, param, &len);
	if (ret < 0)
		goto err_param;

	for (i = 0; i < len; i++) {
		ret = regmap_write(regmap, AXP517_GAUGE_BROM, param[i]);
		if (ret < 0)
			goto err_param;
	}
	/* reset and open brom */
	ret = regmap_update_bits(regmap, AXP517_GAUGE_CONFIG,
				 AXP517_BROMUP_EN, 0);
	if (ret < 0)
		goto err_param;

	ret = regmap_update_bits(regmap, AXP517_GAUGE_CONFIG,
				 AXP517_BROMUP_EN, AXP517_BROMUP_EN);
	if (ret < 0)
		goto err_param;

	/* check battery parameters is ok ? */
	for (i = 0; i < len; i++) {
		ret = regmap_read(regmap, AXP517_GAUGE_BROM, &data);
		if (ret < 0)
			goto err_param;

		if (data != param[i]) {
			PMIC_ERR("model param check %02x error!\n", i);
			PMIC_ERR("data:0x%x param[%d]:0x%x\n", data, i, param[i]);
			goto err_param;
		}
	}

	devm_kfree(bat_power->dev, param);

	/* close brom and set battery update flag */
	ret = regmap_update_bits(regmap, AXP517_GAUGE_CONFIG, AXP517_BROMUP_EN,
				 0);
	if (ret < 0)
		goto UPDATE_ERR;

	ret = regmap_update_bits(regmap, AXP517_GAUGE_CONFIG,
				 AXP517_CFG_UPDATE_MARK,
				 AXP517_CFG_UPDATE_MARK);
	if (ret < 0)
		goto UPDATE_ERR;

	ret = regmap_read(regmap, AXP517_GAUGE_CONFIG, &data);
	if (ret < 0)
		goto UPDATE_ERR;

	/* reset_mcu */
	ret = axp517_reset_mcu(bat_power);
	if (ret < 0)
		goto UPDATE_ERR;
	_axp517_set_ichg(regmap, bat_power->dts_info.pmu_runtime_chgcur);

	/* update ok */
	return 0;

err_param:
	devm_kfree(bat_power->dev, param);

UPDATE_ERR:
	regmap_update_bits(regmap, AXP517_GAUGE_CONFIG, AXP517_BROMUP_EN, 0);
	axp517_reset_mcu(bat_power);

	return ret;
}

static bool axp517_model_update_check(struct regmap *regmap)
{
	int ret = 0;
	unsigned int data;

	ret = regmap_read(regmap, AXP517_GAUGE_CONFIG, &data);
	if (ret < 0)
		goto CHECK_ERR;

	if ((data & AXP517_CFG_UPDATE_MARK) == 0)
		goto CHECK_ERR;


	return true;

CHECK_ERR:
	regmap_update_bits(regmap, AXP517_GAUGE_CONFIG, AXP517_BROMUP_EN, 0);
	return false;
}

static int axp517_bat_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	int ret = 0;

	struct axp517_bat_power *bat_power = power_supply_get_drvdata(psy);

	mutex_lock(&bat_power->lock);

	switch (psp) {
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = psy->desc->name;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		ret = axp517_bat_get_max_voltage(psy, val);
		break;
	case POWER_SUPPLY_PROP_CAPACITY_LEVEL: // customer modify
		ret = axp517_get_soc(bat_power);
		if (ret == 100)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_FULL;
		else if (ret > 80)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_HIGH;
		else if (ret > bat_power->dts_info.pmu_battery_warning_level1)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_NORMAL;
		else if (ret > bat_power->dts_info.pmu_battery_warning_level2)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_LOW;
		else if (ret >= 0)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL;
		else
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_UNKNOWN;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		ret = axp517_get_bat_status(psy, val);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = axp517_get_bat_present(bat_power);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		ret = axp517_get_vbat_vol(psy, val);
		break;
	case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
		val->intval = bat_power->dts_info.pmu_battery_cap * 1000;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		val->intval = bat_power->dts_info.pmu_battery_cap * 1000;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = axp517_get_soc(bat_power); // unit %;
		if (val->intval < 0) {
			val->intval = 0;
		}
		break;
	case POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN:
		ret = axp517_get_lowsocth(psy, val);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		ret = axp517_get_bat_temp(psy, val);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		ret = axp517_get_ichg(psy, val);
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = axp517_get_ichg_lim(psy, val);
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		val->intval = atomic_read(&bat_power->pmu_limit_status);
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX:
		val->intval = 2;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		ret = axp517_get_bat_health(psy, val);
		break;
	case POWER_SUPPLY_PROP_TEMP_ALERT_MIN:
		val->intval = bat_power->bat_shutdown_ltf;
		if (!bat_power->dts_info.pmu_bat_temp_enable)
			val->intval = -200000;
		break;
	case POWER_SUPPLY_PROP_TEMP_ALERT_MAX:
		val->intval = bat_power->bat_shutdown_htf;
		if (!bat_power->dts_info.pmu_bat_temp_enable)
			val->intval = 200000;
		break;
	case POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MIN:
		val->intval = bat_power->bat_charge_ltf;
		if (!bat_power->dts_info.pmu_bat_temp_enable)
			val->intval = -200000;
		break;
	case POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MAX:
		val->intval = bat_power->bat_charge_htf;
		if (!bat_power->dts_info.pmu_bat_temp_enable)
			val->intval = 200000;
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = AXP517_MANUFACTURER;
		break;
	case POWER_SUPPLY_PROP_CHARGE_COUNTER:
		ret = axp517_get_charger_count_current(psy, val);
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		ret = axp517_get_charger_count(psy, val);
		break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		ret = axp517_get_cycle_count(psy, val);
		break;
	case POWER_SUPPLY_PROP_MANUFACTURE_YEAR:
		val->intval = bat_power->dts_info.pmu_bat_manufacture_year;
		break;
	case POWER_SUPPLY_PROP_MANUFACTURE_MONTH:
		val->intval = bat_power->dts_info.pmu_bat_manufacture_month;
		break;
	case POWER_SUPPLY_PROP_MANUFACTURE_DAY:
		val->intval = bat_power->dts_info.pmu_bat_manufacture_day;
		break;
	default:
		ret = -EINVAL;
	}

	mutex_unlock(&bat_power->lock);

	return ret;
}

static int axp517_bat_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct axp517_bat_power *bat_power = power_supply_get_drvdata(psy);
	struct regmap *regmap = bat_power->regmap;
	int ret = 0, lim_cur = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN:
		ret = axp517_set_lowsocth(regmap, val->intval);
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = _axp517_set_ichg(regmap, val->intval);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		ret = axp517_set_bat_max_voltage(regmap, val->intval);
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		atomic_set(&bat_power->pmu_limit_status, val->intval);
		if (val->intval == PAUSE_CHARGING_STATE) {
			lim_cur = 0;
		} else if (val->intval == LIMIT_CUR_STATE) {
			lim_cur = bat_power->dts_info.pmu_bat_charge_control_lim;
		} else if (val->intval == NORMAL_STATE) {
			lim_cur = bat_power->dts_info.pmu_runtime_chgcur;
		} else {
			break;
		}
		ret = axp517_set_ichg(bat_power, lim_cur);
		break;
	case POWER_SUPPLY_PROP_STATUS:
		ret = axp517_set_status(bat_power, val->intval);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int axp517_usb_power_property_is_writeable(struct power_supply *psy,
			     enum power_supply_property psp)
{
	int ret = 0;
	switch (psp) {
	case POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		ret = 1;
		break;
	default:
		ret = -EINVAL;
	}
	return ret;

}

static const struct power_supply_desc axp517_bat_desc = {
	.name = "axp517-battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.get_property = axp517_bat_get_property,
	.set_property = axp517_bat_set_property,
	.properties = axp517_bat_props,
	.num_properties = ARRAY_SIZE(axp517_bat_props),
	.property_is_writeable = axp517_usb_power_property_is_writeable,
};

static void axp517_edev_func_init(struct axp517_bat_power *bat_power)
{
	int ret;

	bat_power->edev = devm_extcon_dev_allocate(bat_power->dev, axp517_pd_extcon_cable);
	if (IS_ERR(bat_power->edev)) {
		PMIC_DEV_ERR(bat_power->dev, "failed to allocate extcon device\n");
		bat_power->edev = NULL;
		return;
	}

	ret = devm_extcon_dev_register(bat_power->dev, bat_power->edev);
	if (ret < 0) {
		PMIC_DEV_ERR(bat_power->dev, "failed to register extcon device\n");
		bat_power->edev = NULL;
	}
}

static void axp517_ntc_func_init(struct axp517_bat_power *bat_power)
{
	struct axp_config_info *axp_config = &bat_power->dts_info;
	struct regmap *regmap = bat_power->regmap;
	int val;

	/* disable ts sets */
	if (!axp_config->pmu_bat_temp_enable) {
		axp_config->pmu_jetia_en = 0;
		axp_config->wakeup_untemp_chg = false;
		axp_config->wakeup_ovtemp_chg = false;
		regmap_update_bits(regmap, AXP517_TS_CFG, BIT(4), BIT(4));
		return;
	}

	/* enable ntc:                     */
	/*   1.enable ts & adc             */
	/*   2.set ts cur                  */
	/*   3.set ntc-discharge vol       */
	/*   4.set ntc-work vol            */
	/*   5.change ntc vol's to temp    */

	/* enable ts & adc */
	regmap_update_bits(regmap, AXP517_ADC_CH_EN0, BIT(1), BIT(1));
	regmap_update_bits(regmap, AXP517_TS_CFG, BIT(4), 0);

	/* set ts cur */
	regmap_read(regmap, AXP517_TS_CFG, &val);
	val &= 0xFC;
	if (axp_config->pmu_bat_ts_current < 40)
		val |= 0x00;
	else if (axp_config->pmu_bat_ts_current < 50)
		val |= 0x01;
	else if (axp_config->pmu_bat_ts_current < 60)
		val |= 0x02;
	else
		val |= 0x03;
	regmap_write(regmap, AXP517_TS_CFG, val);

	/* set ntc-discharge vol */
	if (axp_config->pmu_bat_charge_ltf) {
		if (axp_config->pmu_bat_charge_ltf < axp_config->pmu_bat_charge_htf)
			axp_config->pmu_bat_charge_ltf = axp_config->pmu_bat_charge_htf;

		val = axp_config->pmu_bat_charge_ltf / 32;
		regmap_write(regmap, AXP517_VLTF_CHG, val);
	}

	if (axp_config->pmu_bat_charge_htf) {
		if (axp_config->pmu_bat_charge_htf > 510)
			axp_config->pmu_bat_charge_htf = 510;

		val = axp_config->pmu_bat_charge_htf / 2;
		regmap_write(regmap, AXP517_VHTF_CHG, val);
	}

	/* set work vol */
	if (axp_config->pmu_bat_shutdown_ltf) {
		if (axp_config->pmu_bat_shutdown_ltf < axp_config->pmu_bat_charge_ltf)
			axp_config->pmu_bat_shutdown_ltf = axp_config->pmu_bat_charge_ltf;

		val = axp_config->pmu_bat_shutdown_ltf / 32;
		regmap_write(regmap, AXP517_VLTF_WORK, val);
	}

	if (axp_config->pmu_bat_shutdown_htf) {
		if (axp_config->pmu_bat_shutdown_htf > axp_config->pmu_bat_charge_htf)
			axp_config->pmu_bat_shutdown_htf = axp_config->pmu_bat_charge_htf;

		val = axp_config->pmu_bat_shutdown_htf / 2;
		regmap_write(regmap, AXP517_VHTF_WORK, val);
	}

	/* init temp para */
	bat_power->bat_charge_ltf = axp_vts_to_temp(axp_config->pmu_bat_charge_ltf, axp_config);
	bat_power->bat_charge_htf = axp_vts_to_temp(axp_config->pmu_bat_charge_htf, axp_config);
	bat_power->bat_shutdown_ltf = axp_vts_to_temp(axp_config->pmu_bat_shutdown_ltf, axp_config);
	bat_power->bat_shutdown_htf = axp_vts_to_temp(axp_config->pmu_bat_shutdown_htf, axp_config);

	/* disable jeita */
	if (!axp_config->pmu_jetia_en) {
		regmap_update_bits(regmap, AXP517_JEITA_CFG, BIT(0), 0);
		return;
	}

	/* enable jeita:                   */
	/*   1.enable jeita                */
	/*   2.set jeita vol               */
	/*   3.set jeita cur config        */

	/* enable jeita */
	regmap_update_bits(regmap, AXP517_JEITA_CFG, BIT(0), BIT(0));

	/* set jeita cool vol */
	if (axp_config->pmu_jetia_cool) {
		if (axp_config->pmu_jetia_cool < axp_config->pmu_jetia_warm)
			axp_config->pmu_jetia_cool = axp_config->pmu_jetia_warm;

		val = axp_config->pmu_jetia_cool / 16;
		regmap_write(regmap, AXP517_JEITA_COOL, val);
	}

	/* set jeita warm vol */
	if (axp_config->pmu_jetia_warm) {
		if (axp_config->pmu_jetia_warm > 2040)
			axp_config->pmu_jetia_warm = 2040;

		val = axp_config->pmu_jetia_warm / 8;
		regmap_write(regmap, AXP517_JEITA_WARM, val);
	}

	/* set jeita config */
	regmap_read(regmap, AXP517_JEITA_CV_CFG, &val);
	val &= 0x0F;
	if (axp_config->pmu_jwarm_ifall)
		val |= axp_config->pmu_jwarm_ifall << 6;

	if (axp_config->pmu_jcool_ifall)
		val |= axp_config->pmu_jcool_ifall << 4;

	regmap_write(regmap, AXP517_JEITA_CV_CFG, val);
}

static int axp517_init_chip(struct axp517_bat_power *bat_power)
{
	struct axp_config_info *axp_config = &bat_power->dts_info;
	struct regmap *regmap = bat_power->regmap;
	int ret = 0;
	int val;
	unsigned int battery_exist = 1;

	if (bat_power == NULL)
		return -ENODEV;

	if (ret < 0) {
		PMIC_DEV_ERR(bat_power->dev, "axp517 reg update, i2c communication err!\n");
		return ret;
	}

	/* get battery exist */
	battery_exist = axp517_get_bat_present(bat_power);

	/* init bat cycle */
	axp_config->pmu_bat_cycle_cap_reduce *= axp_config->pmu_battery_cap * 10 / axp_config->pmu_bat_cycle_life;

	/* update battery model */
	if (battery_exist && !axp517_model_update_check(regmap)) {
		PMIC_DEV_ERR(bat_power->dev, "axp517 model need update!\n");
		ret = axp517_model_update(bat_power);
		if (ret < 0) {
			PMIC_DEV_ERR(bat_power->dev, "axp517 model update fail!\n");
			return ret;
		}
	}
	PMIC_DEV_DEBUG(bat_power->dev, "axp517 model update ok:battery_exist:%d\n", battery_exist);
	/*end of update battery model */

	/* set full-charge voltage */
	axp_config->pmu_init_chgvol = clamp_val(axp_config->pmu_init_chgvol, AXP517_VBAT_MIN, AXP517_VBAT_MAX);
	axp517_set_bat_max_voltage(regmap, axp_config->pmu_init_chgvol);

	/* init ntc function */
	if (!battery_exist)
		axp_config->pmu_bat_temp_enable = 0;

	axp517_ntc_func_init(bat_power);

	/* enable bat adc */
	regmap_update_bits(regmap, AXP517_ADC_CH_EN0, BIT(6), BIT(6));
	regmap_update_bits(regmap, AXP517_ADC_CH_EN0, BIT(5), BIT(5));
	regmap_update_bits(regmap, AXP517_ADC_CH_EN0, BIT(2), BIT(2));
	regmap_update_bits(regmap, AXP517_ADC_CH_EN0, BIT(0), BIT(0));

	/* set pre-charge current to 128mA*/
	val = 0x02;
	regmap_update_bits(regmap, AXP517_IPRECHG_CFG, 0x0f, val);

	/* set terminal charge current */
	if (axp_config->pmu_terminal_chgcur < 64)
		val = 0x01;
	else if (axp_config->pmu_terminal_chgcur > 960)
		val = 0x0f;
	else
		val = axp_config->pmu_terminal_chgcur / 64;
	regmap_update_bits(regmap, AXP517_ITERM_CFG, 0x0f, val);

	/*  charger change current can be divided by 64 */
	axp_config->pmu_runtime_chgcur = clamp_val(axp_config->pmu_runtime_chgcur, 0, 3072);
	axp_config->pmu_suspend_chgcur = clamp_val(axp_config->pmu_suspend_chgcur, 0, 3072);
	axp_config->pmu_shutdown_chgcur = clamp_val(axp_config->pmu_shutdown_chgcur, 0, 3072);
	axp_config->pmu_bat_charge_control_lim = clamp_val(axp_config->pmu_bat_charge_control_lim, 0, 3072);

	axp_config->pmu_runtime_chgcur = DIVIDE_BY_64(axp_config->pmu_runtime_chgcur);
	axp_config->pmu_suspend_chgcur = DIVIDE_BY_64(axp_config->pmu_suspend_chgcur);
	axp_config->pmu_shutdown_chgcur = DIVIDE_BY_64(axp_config->pmu_shutdown_chgcur);
	axp_config->pmu_bat_charge_control_lim = DIVIDE_BY_64(axp_config->pmu_bat_charge_control_lim);

	/*  set charger charge current */
	ret = _axp517_set_ichg(regmap, axp_config->pmu_runtime_chgcur);

	/* set gauge_thld */
	axp517_set_gauge_thld(regmap, axp_config->pmu_battery_warning_level1, axp_config->pmu_battery_warning_level2);

	/* set CHGLED */
	if (axp_config->pmu_chgled_func) {
		regmap_update_bits(regmap, AXP517_CHGLED_CFG, GENMASK(2, 0), axp_config->pmu_chgled_type);
	}

	axp517_edev_func_init(bat_power);

	return ret;
}

static irqreturn_t axp517_irq_handler_bat_stat_change(int irq, void *data)
{
	struct axp517_bat_power *bat_power = data;

	PMIC_DEBUG("%s: enter interrupt %d\n", __func__, irq);

	power_supply_changed(bat_power->bat_supply);

	return IRQ_HANDLED;
}

static irqreturn_t axp517_irq_handler_bat_soc_change(int irq, void *data)
{
	struct axp517_bat_power *bat_power = data;
	int radio;

	radio = axp517_get_soc(bat_power);

	SUNXI_POWER_LOG_INFO(bat_power->debug, "BAT radio:%d%%", radio);

	PMIC_DEBUG("%s: enter interrupt %d\n", __func__, irq);

	power_supply_changed(bat_power->bat_supply);

	return IRQ_HANDLED;
}

static irqreturn_t axp517_irq_handler_bat_temp_change(int irq, void *data)
{
	struct axp517_bat_power *bat_power = data;

	PMIC_DEBUG("%s: enter interrupt %d\n", __func__, irq);

	power_supply_changed(bat_power->bat_supply);

	return IRQ_HANDLED;
}

enum axp517_bat_virq_index {
	AXP517_VIRQ_BAT_IN,
	AXP517_VIRQ_BAT_OUT,
	AXP517_VIRQ_BAT_OV,
	/* charge irq */
	AXP517_VIRQ_CHARGING,
	AXP517_VIRQ_CHARGE_OVER,
	/* battery capacity irq */
	AXP517_VIRQ_LOW_WARNING1,
	AXP517_VIRQ_LOW_WARNING2,
	AXP517_VIRQ_BAT_NEW_SOC,
	/* battery temperature irq */
	AXP517_VIRQ_BAT_UNTEMP_WORK,
	AXP517_VIRQ_BAT_OVTEMP_WORK,
	AXP517_VIRQ_BAT_UNTEMP_CHG,
	AXP517_VIRQ_BAT_OVTEMP_CHG,
	AXP517_VIRQ_MAX_VIRQ,
};

static struct axp_interrupts axp_bat_irq[] = {
	[AXP517_VIRQ_BAT_IN] = { "battery_insert",
				  axp517_irq_handler_bat_stat_change },
	[AXP517_VIRQ_BAT_OUT] = { "battery_remove",
				   axp517_irq_handler_bat_stat_change },
	[AXP517_VIRQ_CHARGING] = { "battery_charge_start",
				    axp517_irq_handler_bat_stat_change },
	[AXP517_VIRQ_CHARGE_OVER] = { "battery_charge_done",
				       axp517_irq_handler_bat_stat_change },
	[AXP517_VIRQ_LOW_WARNING1] = { "soc_drop_w1",
					axp517_irq_handler_bat_stat_change },
	[AXP517_VIRQ_LOW_WARNING2] = { "soc_drop_w2",
					axp517_irq_handler_bat_stat_change },
	[AXP517_VIRQ_BAT_UNTEMP_WORK] = { "battery_under_temp_work",
					   axp517_irq_handler_bat_temp_change },
	[AXP517_VIRQ_BAT_OVTEMP_WORK] = { "battery_over_temp_work",
					   axp517_irq_handler_bat_temp_change },
	[AXP517_VIRQ_BAT_UNTEMP_CHG] = { "battery_under_temp_chg",
					  axp517_irq_handler_bat_temp_change },
	[AXP517_VIRQ_BAT_OVTEMP_CHG] = { "battery_over_temp_chg",
					  axp517_irq_handler_bat_temp_change },
	[AXP517_VIRQ_BAT_OV] = { "battery_over_voltage",
					  axp517_irq_handler_bat_stat_change },
	[AXP517_VIRQ_BAT_NEW_SOC] = { "gauge_new_soc",
					  axp517_irq_handler_bat_soc_change },
};

static int axp517_bat_dt_parse(struct device_node *node,
			 struct axp_config_info *axp_config)
{
	if (!of_device_is_available(node)) {
		PMIC_ERR("%s: failed\n", __func__);
		return -1;
	}

	AXP_OF_PROP_READ(pmu_battery_cap,                4000);
	AXP_OF_PROP_READ(pmu_runtime_chgcur,              500);
	AXP_OF_PROP_READ(pmu_suspend_chgcur,             1200);
	AXP_OF_PROP_READ(pmu_shutdown_chgcur,            1200);
	AXP_OF_PROP_READ(pmu_bat_charge_control_lim,      600);
	AXP_OF_PROP_READ(pmu_init_chgvol,                4200);
	AXP_OF_PROP_READ(pmu_battery_warning_level1,       15);
	AXP_OF_PROP_READ(pmu_battery_warning_level2,        0);
	AXP_OF_PROP_READ(pmu_chgled_func,                   1);
	AXP_OF_PROP_READ(pmu_chgled_type,                   0);

	AXP_OF_PROP_READ(pmu_bat_temp_enable,               0);
	AXP_OF_PROP_READ(pmu_bat_temp_comp,                11);
	AXP_OF_PROP_READ(pmu_bat_ts_current,               60);
	AXP_OF_PROP_READ(pmu_bat_charge_ltf,             1312);
	AXP_OF_PROP_READ(pmu_bat_charge_htf,              176);
	AXP_OF_PROP_READ(pmu_bat_shutdown_ltf,           1984);
	AXP_OF_PROP_READ(pmu_bat_shutdown_htf,            152);

	AXP_OF_PROP_READ(pmu_jetia_en,                      0);
	AXP_OF_PROP_READ(pmu_jetia_cool,                  880);
	AXP_OF_PROP_READ(pmu_jetia_warm,                  240);
	AXP_OF_PROP_READ(pmu_jcool_ifall,                   0);
	AXP_OF_PROP_READ(pmu_jwarm_ifall,                   0);

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

	AXP_OF_PROP_READ(pmu_bat_cycle_life,              800);
	AXP_OF_PROP_READ(pmu_bat_cycle_cap_reduce,         20);

	AXP_OF_PROP_READ(pmu_bat_manufacture_year,       2024);
	AXP_OF_PROP_READ(pmu_bat_manufacture_month,         1);
	AXP_OF_PROP_READ(pmu_bat_manufacture_day,           1);


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
	axp_config->wakeup_bat_ov =
		of_property_read_bool(node, "wakeup_bat_ov");
	axp_config->wakeup_new_soc =
		of_property_read_bool(node, "wakeup_new_soc");

	return 0;
}

static void axp517_bat_parse_device_tree(struct axp517_bat_power *bat_power)
{
	int ret;
	struct axp_config_info *axp_config;

	if (!bat_power->dev->of_node) {
		PMIC_INFO("can not find device tree\n");
		return;
	}

	axp_config = &bat_power->dts_info;
	ret = axp517_bat_dt_parse(bat_power->dev->of_node, axp_config);
	if (ret) {
		PMIC_INFO("can not parse device tree err\n");
		return;
	}
}

static void axp517_bat_power_monitor(struct work_struct *work)
{
	struct axp517_bat_power *bat_power =
		container_of(work, typeof(*bat_power), bat_supply_mon.work);
	struct regmap *regmap = bat_power->regmap;
	int soc_cal, pctnow, radio, reg_value, ret;
	uint8_t tmp[2];

	atomic_set(&bat_power->bat_radio_check, 1);

	ret = regmap_read(regmap, AXP517_GAUGE_SOC, &reg_value);
	if (ret < 0)
		return;

	radio = (int)(reg_value & 0x7F);

	/* axp517 fg debug */
	regmap_write(regmap, AXP517_FG_ADDR, 0x80);

	mdelay(2 * 1000);

	ret = regmap_read(regmap, AXP517_FG_DATA_H, &reg_value);
	if (ret < 0)
		return;

	soc_cal = (int)(reg_value & 0x7F);

	/* check status */
	if ((abs(radio - soc_cal)) >= 2) {
		PMIC_INFO("radio:%d soc_cal:%d\n", radio, soc_cal);
		axp517_reset_mcu(bat_power);
		mdelay(1000);
	}

	/* axp517 fg debug */
	regmap_write(regmap, AXP517_FG_ADDR, 0x8E);

	mdelay(2 * 1000);

	ret = regmap_bulk_read(regmap, AXP517_FG_DATA_H, tmp, 2);
	if (ret < 0)
		return;

	if (tmp[0] & BIT(7)) {
		axp517_reset_mcu(bat_power);
		mdelay(1000);
		PMIC_INFO("AXP517_FG_DATA_H:0x%x\n", tmp[1]);
	} else {
		pctnow = (tmp[0] << 8) | tmp[1];

		if (pctnow > 400) {
			PMIC_INFO("pctnow:%d\n", pctnow);
			axp517_reset_mcu(bat_power);
			mdelay(1000);
		}
	}

	atomic_set(&bat_power->bat_radio_check, 0);
	power_supply_changed(bat_power->bat_supply);

	schedule_delayed_work(&bat_power->bat_supply_mon, msecs_to_jiffies(10 * 1000));
}

static void axp517_temp_process_init(struct work_struct *work)
{
	struct axp517_bat_power *bat_power =
		container_of(work, typeof(*bat_power), bat_temp_init.work);
	struct axp_config_info *axp_config = &bat_power->dts_info;
	struct regmap *regmap = bat_power->regmap;
	int vts1, vts2, charge_cur, temp_calib;

	axp_config->pmu_bat_temp_enable = 0;

	axp517_set_status(bat_power, 0);
	mdelay(500);
	vts1 = axp517_get_bat_temp_adc_raw(regmap);
	axp517_set_status(bat_power, 1);
	mdelay(500);
	vts2 = axp517_get_bat_temp_adc_raw(regmap);

	charge_cur = axp517_ichg(regmap);

	temp_calib = (vts2 - vts1) / charge_cur;
	bat_power->bat_temp_calib = clamp_val(temp_calib, axp_config->pmu_bat_temp_comp, axp_config->pmu_battery_rdc);
	axp_config->pmu_bat_temp_enable = 1;
}

static int axp517_blance_ratio(struct regmap *regmap)
{
	int ratio_in_reg, fake_ratio;
	int ret = 0;

	fake_ratio = axp517_bat_is_use_fake_curve_status(regmap);
	if (fake_ratio < 0)
		return 0;

	ret = regmap_read(regmap, AXP517_GAUGE_SOC, &ratio_in_reg);
	if (ret < 0)
		return 0;

	if (ratio_in_reg > AXP517_SOC_MAX)
		ratio_in_reg = AXP517_SOC_MAX;
	else if (ratio_in_reg < AXP517_SOC_MIN)
		ratio_in_reg = AXP517_SOC_MIN;

	return ratio_in_reg - fake_ratio;

}

static void axp517_bat_power_curve_monitor(struct work_struct *work)
{
	struct axp517_bat_power *bat_power =
		container_of(work, typeof(*bat_power), bat_power_curve.work);
	struct regmap *regmap = bat_power->regmap;
	static int rest_ratio, blance_ratio;
	unsigned int reg_value;

	power_supply_changed(bat_power->bat_supply);

	rest_ratio = axp517_bat_is_use_fake_curve_status(regmap);

	if (rest_ratio >= 0) {
		blance_ratio = axp517_blance_ratio(regmap);
		PMIC_INFO("blance_ratio = %d\n", blance_ratio);

		if (blance_ratio >= 1) {
			regmap_read(regmap, AXP517_STATUS0, &reg_value);
			reg_value &= AXP517_MASK_VBUS_STAT;
			if (reg_value)
				rest_ratio++;

			PMIC_INFO("%s:rest_ratio:%d\n", __func__, rest_ratio);
			regmap_update_bits(regmap, AXP517_DATA_BUFF, GENMASK(6, 0), rest_ratio);
			schedule_delayed_work(&bat_power->bat_power_curve, msecs_to_jiffies(30 * 1000));
		} else {
			PMIC_INFO("release wake lock:rest_ratio:%d\n", rest_ratio);
			__pm_relax(bat_power->ws);
			regmap_write(regmap, AXP517_DATA_BUFF, 0);
		}
	} else {
		PMIC_INFO("release wake lock:rest_ratio:%d\n", rest_ratio);
		__pm_relax(bat_power->ws);
		regmap_write(regmap, AXP517_DATA_BUFF, 0);
	}
}

static int axp517_battery_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i = 0, irq;

	struct axp517_bat_power *bat_power;
	struct power_supply_config psy_cfg = {};
	struct sunxi_power_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct device_node *node = pdev->dev.of_node;

	if (!of_device_is_available(node)) {
		PMIC_ERR("axp517-battery device is not configed\n");
		return -ENODEV;
	}

	if (!axp_dev->irq) {
		PMIC_ERR("can not register axp517-battery without irq\n");
		return -EINVAL;
	}

	bat_power = devm_kzalloc(&pdev->dev, sizeof(*bat_power), GFP_KERNEL);
	if (bat_power == NULL) {
		PMIC_ERR("axp517_bat_power alloc failed\n");
		ret = -ENOMEM;
		goto err;
	}

	bat_power->name = "axp517_battery";
	bat_power->dev = &pdev->dev;
	bat_power->regmap = axp_dev->regmap;

	/* for device tree parse */
	axp517_bat_parse_device_tree(bat_power);

	mutex_init(&bat_power->lock);
	ret = axp517_init_chip(bat_power);
	if (ret < 0) {
		PMIC_DEV_ERR(bat_power->dev, "axp517 init chip fail!\n");
		ret = -ENODEV;
		goto err;
	}

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = bat_power;

	bat_power->bat_supply = devm_power_supply_register(bat_power->dev,
			&axp517_bat_desc, &psy_cfg);

	if (IS_ERR(bat_power->bat_supply)) {
		PMIC_ERR("axp517 failed to register bat power\n");
		ret = PTR_ERR(bat_power->bat_supply);
		return ret;
	}

	/* add thermal cooling */
	sunxi_power_register_cooler(bat_power->bat_supply);

	for (i = 0; i < ARRAY_SIZE(axp_bat_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp_bat_irq[i].name);
		if (irq < 0)
			continue;

		irq = regmap_irq_get_virq(axp_dev->regmap_irqc, irq);
		if (irq < 0) {
			PMIC_DEV_ERR(&pdev->dev, "can not get irq\n");
			return irq;
		}
		/* we use this variable to suspend irq */
		axp_bat_irq[i].irq = irq;
		ret = devm_request_any_context_irq(&pdev->dev, irq,
						   axp_bat_irq[i].isr, 0,
						   axp_bat_irq[i].name, bat_power);
		if (ret < 0) {
			PMIC_DEV_ERR(&pdev->dev, "failed to request %s IRQ %d: %d\n",
				axp_bat_irq[i].name, irq, ret);
			return ret;
		} else {
			ret = 0;
		}

		PMIC_DEV_DEBUG(&pdev->dev, "Requested %s IRQ %d: %d\n",
			axp_bat_irq[i].name, irq, ret);
	}
	platform_set_drvdata(pdev, bat_power);

	INIT_DELAYED_WORK(&bat_power->bat_supply_mon, axp517_bat_power_monitor);
	schedule_delayed_work(&bat_power->bat_supply_mon, msecs_to_jiffies(10 * 1000));

	if (axp517_bat_is_use_fake_curve_status(bat_power->regmap) >= 0) {
		PMIC_DEBUG("bat curve smoothing: hold wake lock,blance_vol = %d\n", axp517_blance_ratio(bat_power->regmap));
		bat_power->ws = wakeup_source_register(&pdev->dev, "bat_curve_smooth");
		__pm_stay_awake(bat_power->ws);
		INIT_DELAYED_WORK(&bat_power->bat_power_curve, axp517_bat_power_curve_monitor);
		schedule_delayed_work(&bat_power->bat_power_curve, msecs_to_jiffies(30 * 1000));
	}

	if (bat_power->dts_info.pmu_bat_temp_enable) {
		INIT_DELAYED_WORK(&bat_power->bat_temp_init, axp517_temp_process_init);
		schedule_delayed_work(&bat_power->bat_temp_init, 0);
	}

	bat_power->debug = sunxi_power_debugfs_init(&pdev->dev);
	if (IS_ERR_OR_NULL(bat_power->debug))
		dev_warn(&pdev->dev, "Failed to init debugfs\n");

	SUNXI_POWER_LOG_INFO(bat_power->debug, "BAT power driver initialized");

	return ret;

err:
	PMIC_ERR("%s,probe fail, ret = %d\n", __func__, ret);

	return ret;
}

static int axp517_battery_remove(struct platform_device *pdev)
{
	struct axp517_bat_power *bat_power = platform_get_drvdata(pdev);

	PMIC_DEV_DEBUG(&pdev->dev, "==============AXP517 unegister==============\n");
	if (bat_power->bat_supply) {
		power_supply_unregister(bat_power->bat_supply);
		sunxi_power_unregister_cooler(bat_power->bat_supply);
		mutex_destroy(&bat_power->lock);
	}
	sunxi_power_debugfs_exit(bat_power->debug);
	PMIC_DEV_DEBUG(&pdev->dev, "axp517 teardown battery dev\n");

	return 0;
}

static inline void axp517_bat_irq_set(unsigned int irq, bool enable)
{
	if (enable)
		enable_irq(irq);
	else
		disable_irq(irq);
}

static void axp517_bat_virq_dts_set(struct axp517_bat_power *bat_power, bool enable)
{
	struct axp_config_info *dts_info = &bat_power->dts_info;

	if (!dts_info->wakeup_bat_in)
		axp517_bat_irq_set(axp_bat_irq[AXP517_VIRQ_BAT_IN].irq,
				enable);
	if (!dts_info->wakeup_bat_out)
		axp517_bat_irq_set(axp_bat_irq[AXP517_VIRQ_BAT_OUT].irq,
				enable);
	if (!dts_info->wakeup_bat_charging)
		axp517_bat_irq_set(axp_bat_irq[AXP517_VIRQ_CHARGING].irq,
				enable);
	if (!dts_info->wakeup_bat_charge_over)
		axp517_bat_irq_set(axp_bat_irq[AXP517_VIRQ_CHARGE_OVER].irq,
				enable);
	if (!dts_info->wakeup_low_warning1)
		axp517_bat_irq_set(axp_bat_irq[AXP517_VIRQ_LOW_WARNING1].irq,
				enable);
	if (!dts_info->wakeup_low_warning2)
		axp517_bat_irq_set(axp_bat_irq[AXP517_VIRQ_LOW_WARNING2].irq,
				enable);
	if (!dts_info->wakeup_bat_untemp_work)
		axp517_bat_irq_set(
			axp_bat_irq[AXP517_VIRQ_BAT_UNTEMP_WORK].irq,
			enable);
	if (!dts_info->wakeup_bat_ovtemp_work)
		axp517_bat_irq_set(
			axp_bat_irq[AXP517_VIRQ_BAT_OVTEMP_WORK].irq,
			enable);
	if (!dts_info->wakeup_untemp_chg)
		axp517_bat_irq_set(
			axp_bat_irq[AXP517_VIRQ_BAT_UNTEMP_CHG].irq,
			enable);
	if (!dts_info->wakeup_ovtemp_chg)
		axp517_bat_irq_set(
			axp_bat_irq[AXP517_VIRQ_BAT_OVTEMP_CHG].irq,
			enable);
	if (!dts_info->wakeup_bat_ov)
		axp517_bat_irq_set(
			axp_bat_irq[AXP517_VIRQ_BAT_OV].irq,
			enable);
	if (!dts_info->wakeup_new_soc)
		axp517_bat_irq_set(
			axp_bat_irq[AXP517_VIRQ_BAT_NEW_SOC].irq,
			enable);

}

static void axp517_battery_delayed_work_set(struct axp517_bat_power *bat_power, bool enable)
{
	if (enable) {
		schedule_delayed_work(&bat_power->bat_supply_mon, 0);
		if (axp517_bat_is_use_fake_curve_status(bat_power->regmap) >= 0)
			schedule_delayed_work(&bat_power->bat_power_curve, 0);
	} else {
		cancel_delayed_work_sync(&bat_power->bat_supply_mon);
		if (axp517_bat_is_use_fake_curve_status(bat_power->regmap) >= 0)
			cancel_delayed_work_sync(&bat_power->bat_power_curve);
	}
}

static void axp517_bat_shutdown(struct platform_device *pdev)
{
	struct axp517_bat_power *bat_power = platform_get_drvdata(pdev);
	struct regmap *regmap = bat_power->regmap;

	if (axp517_get_bat_present(bat_power)) {
		regmap_update_bits(regmap, AXP517_MODULE_EN, AXP517_BUCK_EN, 0);
		mdelay(100);
		regmap_update_bits(regmap, AXP517_MODULE_EN, AXP517_BUCK_EN, AXP517_BUCK_EN);
	}

	axp517_battery_delayed_work_set(bat_power, false);
	cancel_delayed_work_sync(&bat_power->bat_supply_mon);

	_axp517_set_ichg(bat_power->regmap, bat_power->dts_info.pmu_shutdown_chgcur);

}

static int axp517_bat_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct axp517_bat_power *bat_power = platform_get_drvdata(pdev);

	axp517_battery_delayed_work_set(bat_power, false);

	atomic_set(&bat_power->pmu_limit_status, 0);
	axp517_set_ichg(bat_power, bat_power->dts_info.pmu_suspend_chgcur);
	axp517_bat_virq_dts_set(bat_power, false);

	return 0;
}

static int axp517_bat_resume(struct platform_device *pdev)
{
	struct axp517_bat_power *bat_power = platform_get_drvdata(pdev);

	power_supply_changed(bat_power->bat_supply);

	axp517_battery_delayed_work_set(bat_power, true);

	axp517_set_ichg(bat_power, bat_power->dts_info.pmu_runtime_chgcur);
	axp517_bat_virq_dts_set(bat_power, true);

	return 0;
}

static const struct of_device_id axp517_bat_power_match[] = {
	{
		.compatible = "x-powers,axp517-bat-power-supply",
		.data = (void *)AXP517_ID,
	}, {/* sentinel */}
};
MODULE_DEVICE_TABLE(of, axp517_bat_power_match);

static struct platform_driver axp517_bat_power_driver = {
	.driver = {
		.name = "axp517-bat-power-supply",
		.of_match_table = axp517_bat_power_match,
	},
	.probe = axp517_battery_probe,
	.remove = axp517_battery_remove,
	.shutdown = axp517_bat_shutdown,
	.suspend = axp517_bat_suspend,
	.resume = axp517_bat_resume,
};

module_platform_driver(axp517_bat_power_driver);

MODULE_AUTHOR("wangxiaoliang <wangxiaoliang@x-powers.com>");
MODULE_DESCRIPTION("axp517 battery driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.9");
